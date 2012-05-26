/*
 * kmscon - KMS Console
 *
 * Copyright (c) 2012 David Herrmann <dh.herrmann@googlemail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signalfd.h>
#include "conf.h"
#include "eloop.h"
#include "input.h"
#include "log.h"
#include "misc.h"
#include "ui.h"
#include "uterm.h"
#include "vt.h"

struct kmscon_app {
	struct ev_eloop *eloop;
	struct ev_eloop *vt_eloop;
	struct kmscon_vt *vt;
	bool exit;
	struct uterm_video *video;
	struct kmscon_input *input;
	struct kmscon_ui *ui;

	struct uterm_vt_master *vtm;
	struct uterm_monitor *mon;
	struct kmscon_dlist seats;
};

struct kmscon_seat {
	struct kmscon_dlist list;
	struct kmscon_app *app;

	struct uterm_monitor_seat *useat;
	char *sname;

	/*
	 * Session Data
	 * We currently allow only a single session on each seat. That is, only
	 * one kmscon instance can run in a single process on each seat. If you
	 * want multiple instances, then start kmscon twice.
	 * We also allow only a single graphics card per seat.
	 * TODO: support multiple sessions per seat in a single kmscon process
	 */

	struct uterm_monitor_dev *vdev;
	struct uterm_video *video;
};

static void sig_generic(struct ev_eloop *eloop, struct signalfd_siginfo *info,
			void *data)
{
	struct kmscon_app *app = data;

	ev_eloop_exit(app->eloop);
	log_info("terminating due to caught signal %d", info->ssi_signo);
}

static bool vt_switch(struct kmscon_vt *vt,
			enum kmscon_vt_action action,
			void *data)
{
	struct kmscon_app *app = data;
	int ret;

	if (action == KMSCON_VT_ENTER) {
		ret = uterm_video_wake_up(app->video);
		if (ret) {
			log_err("cannot wake-up video system");
		} else {
			kmscon_input_wake_up(app->input);
		}
	} else if (action == KMSCON_VT_LEAVE) {
		kmscon_input_sleep(app->input);
		uterm_video_sleep(app->video);
		if (app->exit)
			ev_eloop_exit(app->vt_eloop);
	}

	return true;
}

static void seat_new(struct kmscon_app *app,
		     struct uterm_monitor_seat *useat,
		     const char *sname)
{
	struct kmscon_seat *seat;

	seat = malloc(sizeof(*seat));
	if (!seat)
		return;
	memset(seat, 0, sizeof(*seat));
	seat->app = app;
	seat->useat = useat;

	seat->sname = strdup(sname);
	if (!seat->sname) {
		log_err("cannot allocate memory for seat name");
		goto err_free;
	}

	uterm_monitor_set_seat_data(seat->useat, seat);
	kmscon_dlist_link(&app->seats, &seat->list);

	log_info("new seat %s", seat->sname);
	return;

err_free:
	free(seat);
}

static void seat_free(struct kmscon_seat *seat)
{
	log_info("free seat %s", seat->sname);

	kmscon_dlist_unlink(&seat->list);
	uterm_monitor_set_seat_data(seat->useat, NULL);
	free(seat->sname);
	free(seat);
}

static void seat_add_video(struct kmscon_seat *seat,
			   struct uterm_monitor_dev *dev,
			   const char *node)
{
	int ret;

	if (seat->video)
		return;

	ret = uterm_video_new(&seat->video, seat->app->eloop, UTERM_VIDEO_DRM,
			      node);
	if (ret)
		return;

	seat->vdev = dev;

	log_debug("new graphics device on seat %s", seat->sname);
}

static void seat_rm_video(struct kmscon_seat *seat,
			  struct uterm_monitor_dev *dev)
{
	if (!seat->video || seat->vdev != dev)
		return;

	log_debug("free graphics device on seat %s", seat->sname);

	seat->vdev = NULL;
	uterm_video_unref(seat->video);
	seat->video = NULL;
}

static void monitor_event(struct uterm_monitor *mon,
			  struct uterm_monitor_event *ev,
			  void *data)
{
	struct kmscon_app *app = data;

	switch (ev->type) {
	case UTERM_MONITOR_NEW_SEAT:
		seat_new(app, ev->seat, ev->seat_name);
		break;
	case UTERM_MONITOR_FREE_SEAT:
		if (ev->seat_data)
			seat_free(ev->seat_data);
		break;
	case UTERM_MONITOR_NEW_DEV:
		if (!ev->seat_data)
			break;
		if (ev->dev_type == UTERM_MONITOR_DRM)
			seat_add_video(ev->seat_data, ev->dev, ev->dev_node);
		break;
	case UTERM_MONITOR_FREE_DEV:
		if (!ev->seat_data)
			break;
		if (ev->dev_type == UTERM_MONITOR_DRM)
			seat_rm_video(ev->seat_data, ev->dev);
		break;
	case UTERM_MONITOR_HOTPLUG_DEV:
		break;
	}
}

static void destroy_app(struct kmscon_app *app)
{
	kmscon_ui_free(app->ui);
	kmscon_input_unref(app->input);
	uterm_video_unref(app->video);
	kmscon_vt_unref(app->vt);
	uterm_monitor_unref(app->mon);
	uterm_vt_master_unref(app->vtm);
	ev_eloop_unregister_signal_cb(app->eloop, SIGINT, sig_generic, app);
	ev_eloop_unregister_signal_cb(app->eloop, SIGTERM, sig_generic, app);
	ev_eloop_rm_eloop(app->vt_eloop);
	ev_eloop_unref(app->eloop);
}

static int setup_app(struct kmscon_app *app)
{
	int ret;

	ret = ev_eloop_new(&app->eloop);
	if (ret)
		goto err_app;

	ret = ev_eloop_register_signal_cb(app->eloop, SIGTERM,
						sig_generic, app);
	if (ret)
		goto err_app;

	ret = ev_eloop_register_signal_cb(app->eloop, SIGINT,
						sig_generic, app);
	if (ret)
		goto err_app;

	ret = ev_eloop_new_eloop(app->eloop, &app->vt_eloop);
	if (ret)
		goto err_app;

	ret = uterm_vt_master_new(&app->vtm, app->vt_eloop);
	if (ret)
		goto err_app;

	kmscon_dlist_init(&app->seats);

	ret = uterm_monitor_new(&app->mon, app->eloop, monitor_event, app);
	if (ret)
		goto err_app;

	ret = kmscon_vt_new(&app->vt, vt_switch, app);
	if (ret)
		goto err_app;

	ret = uterm_video_new(&app->video,
				app->eloop,
				UTERM_VIDEO_DRM,
				"/dev/dri/card0");
	if (ret)
		goto err_app;

	ret = uterm_video_use(app->video);
	if (ret)
		goto err_app;

	ret = kmscon_input_new(&app->input);
	if (ret)
		goto err_app;

	ret = kmscon_input_connect_eloop(app->input, app->eloop);
	if (ret)
		goto err_app;

	ret = kmscon_vt_open(app->vt, KMSCON_VT_NEW, app->vt_eloop);
	if (ret)
		goto err_app;

	ret = kmscon_ui_new(&app->ui, app->eloop, app->video, app->input);
	if (ret)
		goto err_app;

	uterm_monitor_scan(app->mon);

	return 0;

err_app:
	destroy_app(app);
	return ret;
}

int main(int argc, char **argv)
{
	int ret;
	struct kmscon_app app;

	ret = conf_parse_argv(argc, argv);
	if (ret)
		goto err_out;

	if (conf_global.exit)
		return EXIT_SUCCESS;

	if (!conf_global.debug && !conf_global.verbose && conf_global.silent)
		log_set_config(&LOG_CONFIG_WARNING(0, 0, 0, 0));
	else
		log_set_config(&LOG_CONFIG_INFO(conf_global.debug,
						conf_global.verbose));

	log_print_init("kmscon");

	memset(&app, 0, sizeof(app));
	ret = setup_app(&app);
	if (ret)
		goto err_out;

	if (conf_global.switchvt) {
		ret = kmscon_vt_enter(app.vt);
		if (ret)
			log_warn("cannot enter VT");
	}

	ev_eloop_run(app.eloop, -1);

	if (conf_global.switchvt) {
		/* The VT subsystem needs to acknowledge the VT-leave so if it
		 * returns -EINPROGRESS we need to wait for the VT-leave SIGUSR2
		 * signal to arrive. Therefore, we use a separate eloop object
		 * which is used by the VT system only. Therefore, waiting on
		 * this eloop allows us to safely wait 50ms for the SIGUSR2 to
		 * arrive.
		 * We use a timeout of 100ms to avoid haning on exit.
		 * We could also wait on app.eloop but this would allow other
		 * subsystems to continue receiving events and this is not what
		 * we want.
		 */
		app.exit = true;
		ret = kmscon_vt_leave(app.vt);
		if (ret == -EINPROGRESS)
			ev_eloop_run(app.vt_eloop, 50);
	}

	destroy_app(&app);
	log_info("exiting");

	return EXIT_SUCCESS;

err_out:
	log_err("cannot initialize kmscon, errno %d: %s", ret, strerror(-ret));
	return -ret;
}
