/*
 * kmscon - Character-Device Session
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

/*
 * Character-Device Session
 */

#define FUSE_USE_VERSION 29

#include <errno.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <termio.h>
#include <termios.h>
#include <unistd.h>
#include "kmscon_cdev.h"
#include "kmscon_seat.h"
#include "log.h"
#include "shl_dlist.h"
#include "shl_ring.h"
#include "tsm_screen.h"
#include "tsm_vte.h"
#include "uterm.h"

#include <fuse/fuse.h>
#include <fuse/fuse_common.h>
#include <fuse/fuse_lowlevel.h>
#include <fuse/fuse_opt.h>
#include <fuse/cuse_lowlevel.h>

#define LOG_SUBSYSTEM "cdev"

struct kmscon_cdev {
	struct kmscon_seat *seat;
	struct ev_eloop *eloop;
	struct uterm_input *input;
	struct kmscon_session *s;
	struct ev_fd *efd;

	struct fuse_session *session;
	int fd;
	struct fuse_chan *channel;

	size_t bufsize;
	char *buf;

	struct shl_dlist clients;
};

struct cdev_client {
	struct shl_dlist list;
	struct kmscon_cdev *cdev;

	struct tsm_screen *screen;
	struct tsm_vte *vte;

	bool active;
	struct kmscon_session *s;

	struct fuse_pollhandle *ph;
	struct shl_ring *ring;
	struct shl_dlist readers;

	long kdmode;
	long kbmode;
};

struct cdev_reader {
	struct shl_dlist list;
	bool killed;
	fuse_req_t req;
	size_t len;
};

/*
 * Cdev Clients
 * As opposed to kernel VTs, we only provide one single char-dev per seat and
 * each client that opens it is managed separately. Hence, it is not possible to
 * use such a VT shared by two clients except if you pass the FD between the
 * clients. This has several advantages and avoids many bugs in the kernel VT
 * implementation.
 * For every user opening a /dev/ttyF<seat> device, a separate cdev_client is
 * created. A cdev_client emulates a single kernel VT but is managed as a
 * dedicated kmscon_session on its seat. We start a kmscon_terminal as backend
 * so you actually can run "agetty" on this fake-VT. When set into graphical
 * mode, the terminal is suspended and you can run an XServer on it. We emulate
 * the VT-switching signal-API, too. We release all DRM devices if a fake-VT is
 * active and reqcuire them afterwards. This allows the clients to actually
 * implement graphical terminals. However, if you fail to release the DRM
 * devices, we actually try to kill the fake-VT so we can get access again.
 */

static void reader_free(struct cdev_reader *reader, int error);

static void reader_interrupt(fuse_req_t req, void *data)
{
	struct cdev_reader *reader = data;

	if (!reader)
		return;

	reader->killed = true;
}

static int reader_new(struct cdev_reader **out, struct cdev_client *client,
		      fuse_req_t req)
{
	struct cdev_reader *reader;

	if (fuse_req_interrupted(req))
		return -ENOENT;

	reader = malloc(sizeof(*reader));
	if (!reader)
		return -ENOMEM;
	memset(reader, 0, sizeof(*reader));
	reader->req = req;
	fuse_req_interrupt_func(req, reader_interrupt, reader);
	if (reader->killed) {
		fuse_req_interrupt_func(req, NULL, NULL);
		free(reader);
		return -ENOENT;
	}

	shl_dlist_link_tail(&client->readers, &reader->list);
	*out = reader;
	return 0;
}

static void reader_free(struct cdev_reader *reader, int error)
{
	shl_dlist_unlink(&reader->list);
	if (reader->req) {
		fuse_req_interrupt_func(reader->req, NULL, NULL);
		fuse_reply_err(reader->req, -error);
	}
	free(reader);
}

static int reader_release(struct cdev_reader *reader, const char *buf,
			  size_t len)
{
	int ret;

	fuse_req_interrupt_func(reader->req, NULL, NULL);
	ret = fuse_reply_buf(reader->req, buf, len);
	reader->req = NULL;
	reader_free(reader, 0);
	return ret;
}

static void client_vte_event(struct tsm_vte *vte, const char *u8, size_t len,
			     void *data)
{
	struct cdev_client *client = data;
	struct cdev_reader *reader;
	int ret;
	bool was_empty;
	const char *buf;
	size_t size;

	/* TODO: we should have a maximum buffer size here */
	was_empty = shl_ring_is_empty(client->ring);
	ret = shl_ring_write(client->ring, u8, len);
	if (ret)
		log_warning("cannot resize buffer for cdev client: %d", ret);

	if (shl_ring_is_empty(client->ring))
		return;

	if (was_empty && client->ph) {
		fuse_notify_poll(client->ph);
		fuse_pollhandle_destroy(client->ph);
		client->ph = NULL;
	}

	while (!shl_dlist_empty(&client->readers)) {
		reader = shl_dlist_entry(client->readers.next,
					 struct cdev_reader, list);
		if (reader->killed)
			continue;

		/* TODO: fix filling the whole buffer instead of returning
		 * partial buffers when the ring data is split */
		buf = shl_ring_peek(client->ring, &size, 0);
		if (!size)
			break;
		if (size > reader->len)
			size = reader->len;

		ret = reader_release(reader, buf, size);
		if (ret < 0)
			continue;
		shl_ring_drop(client->ring, size);
	}
}

static void client_input_event(struct uterm_input *input,
			       struct uterm_input_event *ev,
			       void *data)
{
	struct cdev_client *client = data;

	if (!client->active || ev->handled)
		return;

	/* we drop all input in K_OFF mode */
	if (client->kbmode == K_OFF)
		return;

	/* TODO: see kmscon_terminal on how this is special. We need to fix this
	 * when xkbcommon provides the first multi-sym events. */
	if (ev->num_syms > 1)
		return;

	if (tsm_vte_handle_keyboard(client->vte, ev->keysyms[0], ev->ascii,
				    ev->mods, ev->codepoints[0])) {
		tsm_screen_sb_reset(client->screen);
		ev->handled = true;
	}
}

static void client_free(struct cdev_client *client)
{
	struct cdev_reader *reader;

	log_debug("free fake TTY client %p", client);

	shl_dlist_unlink(&client->list);
	if (client->ph)
		fuse_pollhandle_destroy(client->ph);

	while (!shl_dlist_empty(&client->readers)) {
		reader = shl_dlist_entry(client->readers.next,
					 struct cdev_reader, list);
		reader_free(reader, -EPIPE);
	}

	uterm_input_unregister_cb(client->cdev->input, client_input_event,
				  client);
	tsm_vte_unref(client->vte);
	tsm_screen_unref(client->screen);
	shl_ring_free(client->ring);
	free(client);
}

static int client_session_event(struct kmscon_session *s,
				struct kmscon_session_event *ev,
				void *data)
{
	struct cdev_client *client = data;

	switch (ev->type) {
	case KMSCON_SESSION_ACTIVATE:
		client->active = true;
		break;
	case KMSCON_SESSION_DEACTIVATE:
		client->active = false;
		break;
	case KMSCON_SESSION_UNREGISTER:
		client_free(client);
		break;
	}

	return 0;
}

static int client_new(struct cdev_client **out, struct kmscon_cdev *cdev)
{
	struct cdev_client *client;
	int ret;

	client = malloc(sizeof(*client));
	if (!client)
		return -ENOMEM;
	memset(client, 0, sizeof(*client));
	client->cdev = cdev;
	client->kdmode = KD_TEXT;
	client->kbmode = K_UNICODE;
	shl_dlist_init(&client->readers);

	log_debug("new fake TTY client %p", client);

	ret = shl_ring_new(&client->ring);
	if (ret) {
		log_error("cannot create ring buffer for new cdev client: %d",
			  ret);
		goto err_free;
	}

	/* TODO: Share the terminal-handling with the terminal-session. We
	 * currently just create the screen/vte obejcts here to get meaningful
	 * parsers. However, we should also correctly handled the terminal as is
	 * and draw it to the screen if in text-mode.
	 * This is nearly identical to the terminal-session so we should share
	 * the implementation between both instead of doing everything ourself
	 * here. */

	ret = tsm_screen_new(&client->screen, log_llog);
	if (ret) {
		log_error("cannot create TSM screen for new cdev client: %d",
			  ret);
		goto err_ring;
	}

	ret = tsm_vte_new(&client->vte, client->screen, client_vte_event,
			  client, log_llog);
	if (ret) {
		log_error("cannot create TSM VTE for new cdev client: %d",
			  ret);
		goto err_screen;
	}

	ret = uterm_input_register_cb(cdev->input, client_input_event, client);
	if (ret) {
		log_error("cannot register input callback for cdev client: %d",
			  ret);
		goto err_vte;
	}

	ret = kmscon_seat_register_session(cdev->seat, &client->s,
					   client_session_event, client);
	if (ret) {
		log_error("cannot register session for cdev client: %d", ret);
		goto err_input;
	}

	shl_dlist_link(&cdev->clients, &client->list);
	*out = client;
	return 0;

err_input:
	uterm_input_unregister_cb(cdev->input, client_input_event, client);
err_vte:
	tsm_vte_unref(client->vte);
err_screen:
	tsm_screen_unref(client->screen);
err_ring:
	shl_ring_free(client->ring);
err_free:
	free(client);
	return ret;
}

static void client_destroy(struct cdev_client *client)
{
	kmscon_session_unregister(client->s);
}

/* This must be called after each event dispatch round. It cleans up all
 * interrupted/killed readers. The readers cannot be released right away due to
 * heavy locking inside of FUSE. We have to delay these tasks and clean up after
 * each dispatch round. */
static void client_cleanup(struct cdev_client *client)
{
	struct shl_dlist *i, *tmp;
	struct cdev_reader *reader;

	shl_dlist_for_each_safe(i, tmp, &client->readers) {
		reader = shl_dlist_entry(i, struct cdev_reader, list);
		if (reader->killed)
			reader_free(reader, -ENOENT);
	}
}

/*
 * FUSE low-level ops
 * This implements all the file-system operations on the character-device. It is
 * important that we handle interrupts correctly (ENOENT) and never loose any
 * data. This is all single threaded as it is not performance critical at all.
 */

static void ll_open(fuse_req_t req, struct fuse_file_info *fi)
{
	struct kmscon_cdev *cdev = fuse_req_userdata(req);
	struct cdev_client *client;
	int ret;

	ret = client_new(&client, cdev);
	if (ret)
		goto err_out;

	fi->fh = (long)client;
	fi->nonseekable = 1;
	fi->direct_io = 1;
	ret = fuse_reply_open(req, fi);
	if (ret < 0)
		client_destroy(client);
	else
		kmscon_session_enable(client->s);

	return;

err_out:
	fuse_reply_err(req, -ret);
}

static void ll_release(fuse_req_t req, struct fuse_file_info *fi)
{
	struct cdev_client *client = (void*)fi->fh;

	if (!client) {
		fuse_reply_err(req, EINVAL);
		return;
	}

	client_destroy(client);
	fuse_reply_err(req, 0);
}

static void ll_read(fuse_req_t req, size_t size, off_t off,
		    struct fuse_file_info *fi)
{
	struct cdev_client *client = (void*)fi->fh;
	struct cdev_reader *reader;
	const char *buf;
	size_t len;
	int ret;

	if (!client) {
		fuse_reply_err(req, EINVAL);
		return;
	}

	if (off != 0) {
		fuse_reply_err(req, EINVAL);
		return;
	}

	if (!size) {
		fuse_reply_buf(req, "", 0);
		return;
	}

	/* TODO: use a proper intermediate buffer as this might return only
	 * partial data */
	buf = shl_ring_peek(client->ring, &len, 0);
	if (!len) {
		if (fi->flags & O_NONBLOCK) {
			fuse_reply_err(req, EAGAIN);
			return;
		}

		ret = reader_new(&reader, client, req);
		if (ret) {
			fuse_reply_err(req, -ret);
			return;
		}

		reader->len = size;
		return;
	}

	if (len > size)
		len = size;
	ret = fuse_reply_buf(req, buf, len);
	if (ret < 0)
		return;
	shl_ring_drop(client->ring, len);
}

static void ll_write(fuse_req_t req, const char *buf, size_t size, off_t off,
		     struct fuse_file_info *fi)
{
	struct cdev_client *client = (void*)fi->fh;
	int ret;

	if (!client) {
		fuse_reply_err(req, EINVAL);
		return;
	}

	ret = fuse_reply_write(req, size);
	if (ret < 0)
		return;
	tsm_vte_input(client->vte, buf, size);
}

static void ll_poll(fuse_req_t req, struct fuse_file_info *fi,
		    struct fuse_pollhandle *ph)
{
	struct cdev_client *client = (void*)fi->fh;
	unsigned int flags;

	if (!client) {
		fuse_reply_err(req, EINVAL);
		return;
	}

	if (client->ph) {
		fuse_pollhandle_destroy(client->ph);
		client->ph = ph;
	}

	flags = EPOLLOUT | EPOLLWRNORM;
	if (!shl_ring_is_empty(client->ring))
		flags |= EPOLLIN | EPOLLRDNORM;

	fuse_reply_poll(req, flags);
}

static void ioctl_TCFLSH(struct cdev_client *client, fuse_req_t req, int val)
{
	switch (val) {
	case TCIFLUSH:
		shl_ring_flush(client->ring);
		break;
	case TCIOFLUSH:
		shl_ring_flush(client->ring);
		/* fallthrough */
	case TCOFLUSH:
		/* nothing to do; we have no output queue */
		break;
	default:
		fuse_reply_err(req, EINVAL);
		return;
	}

	fuse_reply_ioctl(req, 0, NULL, 0);
}

static void ioctl_KDGETMODE(struct cdev_client *client, fuse_req_t req)
{
	fuse_reply_ioctl(req, 0, &client->kdmode, sizeof(long));
}

static void ioctl_KDSETMODE(struct cdev_client *client, fuse_req_t req,
			    long val)
{
	switch (val) {
	case KD_TEXT:
	case KD_GRAPHICS:
		/* TODO: forward this to the current seat */
		client->kdmode = val;
		fuse_reply_ioctl(req, 0, NULL, 0);
		break;
	default:
		fuse_reply_err(req, EINVAL);
		break;
	}
}

static void ioctl_KDGKBMODE(struct cdev_client *client, fuse_req_t req)
{
	fuse_reply_ioctl(req, 0, &client->kbmode, sizeof(long));
}

static void ioctl_KDSKBMODE(struct cdev_client *client, fuse_req_t req,
			    long val)
{
	switch (val) {
	case K_RAW:
	case K_UNICODE:
	case K_OFF:
		/* TODO: we handle K_RAW/K_UNICODE the same way as it is unclear
		 * what K_RAW should do? */
		client->kbmode = val;
		fuse_reply_ioctl(req, 0, NULL, 0);
		break;
	case K_XLATE:
	case K_MEDIUMRAW:
		/* TODO: what do these do? */
		fuse_reply_err(req, EOPNOTSUPP);
		break;
	default:
		fuse_reply_err(req, EINVAL);
		break;
	}
}

static bool ioctl_param(fuse_req_t req, void *arg, size_t in_want,
			size_t in_have, size_t out_want, size_t out_have)
{
	bool retry;
	struct iovec in, out;
	size_t in_num, out_num;

	retry = false;
	memset(&in, 0, sizeof(in));
	in_num = 0;
	memset(&out, 0, sizeof(out));
	out_num = 0;

	if (in_want) {
		if (!in_have) {
			retry = true;
		} else if (in_have < in_want) {
			fuse_reply_err(req, EFAULT);
			return true;
		}

		in.iov_base = arg;
		in.iov_len = in_want;
		in_num = 1;
	}
	if (out_want) {
		if (!out_have) {
			retry = true;
		} else if (out_have < out_want) {
			fuse_reply_err(req, EFAULT);
			return true;
		}

		out.iov_base = arg;
		out.iov_len = out_want;
		out_num = 1;
	}

	if (retry)
		fuse_reply_ioctl_retry(req, in_num ? &in : NULL, in_num,
				       out_num ? &out : NULL, out_num);
	return retry;
}

static void ll_ioctl(fuse_req_t req, int cmd, void *arg,
		     struct fuse_file_info *fi, unsigned int flags,
		     const void *in_buf, size_t in_bufsz, size_t out_bufsz)
{
	struct cdev_client *client = (void*)fi->fh;
	bool compat;

	if (!client) {
		fuse_reply_err(req, EINVAL);
		return;
	}

	/* TODO: fix compat-ioctls */
	compat = !!(flags & FUSE_IOCTL_COMPAT);
	if (compat) {
		fuse_reply_err(req, EOPNOTSUPP);
		return;
	}

	switch (cmd) {
	case TCFLSH:
		if (ioctl_param(req, arg, sizeof(int), in_bufsz, 0, out_bufsz))
			return;
		ioctl_TCFLSH(client, req, *(int*)in_buf);
		break;
	case VT_ACTIVATE:
		if (ioctl_param(req, arg, sizeof(int), in_bufsz, 0, out_bufsz))
			return;
		fuse_reply_err(req, EOPNOTSUPP);
		break;
	case VT_WAITACTIVE:
		if (ioctl_param(req, arg, sizeof(int), in_bufsz, 0, out_bufsz))
			return;
		fuse_reply_err(req, EOPNOTSUPP);
		break;
	case VT_GETSTATE:
		if (ioctl_param(req, arg, 0, in_bufsz,
				sizeof(struct vt_stat), out_bufsz))
			return;
		fuse_reply_err(req, EOPNOTSUPP);
		break;
	case VT_OPENQRY:
		if (ioctl_param(req, arg, 0, in_bufsz,
				sizeof(int), out_bufsz))
			return;
		fuse_reply_err(req, EOPNOTSUPP);
		break;
	case VT_GETMODE:
		if (ioctl_param(req, arg, 0, in_bufsz,
				sizeof(struct vt_mode), out_bufsz))
			return;
		fuse_reply_err(req, EOPNOTSUPP);
		break;
	case VT_SETMODE:
		if (ioctl_param(req, arg, sizeof(struct vt_mode), in_bufsz,
				0, out_bufsz))
			return;
		fuse_reply_err(req, EOPNOTSUPP);
		break;
	case KDGETMODE:
		if (ioctl_param(req, arg, 0, in_bufsz,
				sizeof(long), out_bufsz))
			return;
		ioctl_KDGETMODE(client, req);
		break;
	case KDSETMODE:
		if (ioctl_param(req, arg, sizeof(long), in_bufsz,
				0, out_bufsz))
			return;
		ioctl_KDSETMODE(client, req, *(long*)in_buf);
		break;
	case KDGKBMODE:
		if (ioctl_param(req, arg, 0, in_bufsz,
				sizeof(long), out_bufsz))
			return;
		ioctl_KDGKBMODE(client, req);
		break;
	case KDSKBMODE:
		if (ioctl_param(req, arg, sizeof(long), in_bufsz,
				0, out_bufsz))
			return;
		ioctl_KDSKBMODE(client, req, *(long*)in_buf);
		break;
	case TCGETS:
		if (ioctl_param(req, arg, 0, in_bufsz,
				sizeof(struct termios), out_bufsz))
			return;
		fuse_reply_err(req, EOPNOTSUPP);
		break;
	case TCSETS:
		if (ioctl_param(req, arg, sizeof(struct termios), in_bufsz,
				0, out_bufsz))
			return;
		fuse_reply_err(req, EOPNOTSUPP);
		break;
	case TCSETSW:
		if (ioctl_param(req, arg, sizeof(struct termios), in_bufsz,
				0, out_bufsz))
			return;
		fuse_reply_err(req, EOPNOTSUPP);
		break;
	case TCSETSF:
		if (ioctl_param(req, arg, sizeof(struct termios), in_bufsz,
				0, out_bufsz))
			return;
		fuse_reply_err(req, EOPNOTSUPP);
		break;
	default:
		fuse_reply_err(req, EINVAL);
		break;
	}
}

static void ll_destroy(void *data) {
	struct kmscon_cdev *cdev = data;
	struct cdev_client *client;

	/* on unexpected shutdown this releases all currently open clients */
	while (!shl_dlist_empty(&cdev->clients)) {
		client = shl_dlist_entry(cdev->clients.next,
					 struct cdev_client, list);
		client_destroy(client);
	}
}

static const struct cuse_lowlevel_ops ll_ops = {
	.init = NULL,
	.destroy = ll_destroy,
	.open = ll_open,
	.release = ll_release,
	.read = ll_read,
	.write = ll_write,
	.poll = ll_poll,
	.ioctl = ll_ioctl,
	.flush = NULL,
	.fsync = NULL,
};

/*
 * FUSE channel ops
 * The connection to the FUSE kernel module is done via a file-descriptor.
 * Writing to it is synchronous, so the commands that we write are _immediately_
 * executed and return the result to us. Furthermore, write() is always
 * non-blocking and always succeeds so no reason to watch for EAGAIN.
 * Reading from the FD, on the other hand, may block if there is no data
 * available. However, we only read if the FD was signaled readable so we can
 * use a blocking FD to avoid any side-effects. The kernel maintains an
 * event-queue that we read from. So there may be pending events that we haven't
 * read but which affect the calls that we write to the kernel. This is
 * important when handling interrupts.
 * chan_receive() and chan_send() handle I/O to the kernel module and are hooked
 * up into a fuse-channel.
 */

static int chan_receive(struct fuse_chan **chp, char *buf, size_t size)
{
	struct fuse_chan *ch = *chp;
	struct kmscon_cdev *cdev = fuse_chan_data(ch);
	struct fuse_session *se = fuse_chan_session(ch);
	int fd = fuse_chan_fd(ch);
	ssize_t res;

	if (!se || !cdev)
		return -EINVAL;

	if (!size)
		return 0;

restart:
	if (fuse_session_exited(se))
		return 0;

	res = read(fd, buf, size);
	if (!res) {
		/* EOF on cuse file */
		log_error("fuse channel shut down");
		fuse_session_exit(se);
		return 0;
	} else if (res < 0) {
		/* ENOENT is returned if the operation was interrupted, it's
		 * safe to restart */
		if (errno == ENOENT)
			goto restart;

		/* ENODEV is returned if the FS got unmounted. This shouldn't
		 * occur for CUSE devices. Anyway, exit if this happens. */
		if (errno == ENODEV) {
			fuse_session_exit(se);
			return 0;
		}

		/* EINTR and EAGAIN are simply forwarded to the caller. */
		if (errno == EINTR || errno == EAGAIN)
			return -errno;

		log_error("fuse channel read error (%d): %m", errno);
		fuse_session_exit(se);
		return -errno;
	}

	return res;
}

static int chan_send(struct fuse_chan *ch, const struct iovec iov[],
		     size_t count)
{
	struct kmscon_cdev *cdev = fuse_chan_data(ch);
	struct fuse_session *se = fuse_chan_session(ch);
	int fd = fuse_chan_fd(ch);
	int ret;

	if (!cdev || !se)
		return -EINVAL;
	if (!iov || !count)
		return 0;

	ret = writev(fd, iov, count);
	if (ret < 0) {
		/* ENOENT is returned on interruptions */
		if (!fuse_session_exited(se) && errno != ENOENT) {
			log_error("cannot write to fuse-channel (%d): %m",
				  errno);
			fuse_session_exit(se);
		}
		return -errno;
	}

	return 0;
}

static const struct fuse_chan_ops chan_ops = {
	.receive = chan_receive,
	.send = chan_send,
	.destroy = NULL,
};

/*
 * Character Device
 * This creates the high-level character-device driver and registers a
 * fake-session that is used to control each fake-VT session.
 * channel_event() is a callback when I/O is possible on the FUSE FD and
 * performs all outstanding tasks.
 * On error, the fake-session is unregistered and deleted which also destroys
 * _all_ client fake-sessions.
 */

static void channel_event(struct ev_fd *fd, int mask, void *data)
{
	struct kmscon_cdev *cdev = data;
	int ret;
	struct fuse_buf buf;
	struct fuse_chan *ch;
	struct shl_dlist *i;
	struct cdev_client *client;

	if (mask & (EV_HUP | EV_ERR)) {
		log_error("HUP/ERR on fuse channel");
		kmscon_session_unregister(cdev->s);
		return;
	}

	if (!(mask & EV_READABLE))
		return;

	memset(&buf, 0, sizeof(buf));
	buf.mem = cdev->buf;
	buf.size = cdev->bufsize;
	ch = cdev->channel;
	ret = fuse_session_receive_buf(cdev->session, &buf, &ch);
	if (ret == -EINTR || ret == -EAGAIN) {
		return;
	} else if (ret < 0) {
		log_error("fuse channel read error: %d", ret);
		kmscon_session_unregister(cdev->s);
		return;
	}

	fuse_session_process_buf(cdev->session, &buf, ch);
	if (fuse_session_exited(cdev->session)) {
		log_error("fuse session exited");
		kmscon_session_unregister(cdev->s);
		return;
	}

	/* Readers can get interrupted asynchronously. Due to heavy locking
	 * inside of FUSE, we cannot release them right away. So cleanup all
	 * killed readers after we processed all buffers. */
	shl_dlist_for_each(i, &cdev->clients) {
		client = shl_dlist_entry(i, struct cdev_client, list);
		client_cleanup(client);
	}
}

static int kmscon_cdev_init(struct kmscon_cdev *cdev)
{
	static const char prefix[] = "DEVNAME=";
	static const char fname[] = "/dev/cuse";
	int ret;
	size_t bufsize;
	struct cuse_info ci;
	const char *dev_info_argv[1];
	char *name;

	/* TODO: libfuse makes sure that fd 0, 1 and 2 are available as standard
	 * streams, otherwise they fail. This is awkward and we should check
	 * whether this is really needed and _why_?
	 * If it is needed, fix upstream to stop that crazy! */

	shl_dlist_init(&cdev->clients);

	ret = asprintf(&name, "%sttyF%s", prefix,
		       kmscon_seat_get_name(cdev->seat));
	if (ret <= 0) {
		log_error("cannot allocate memory for fuse-devname");
		return -ENOMEM;
	}

	log_info("initializing fake VT TTY device /dev/%s",
		 &name[sizeof(prefix) - 1]);

	dev_info_argv[0] = name;
	memset(&ci, 0, sizeof(ci));
	ci.dev_major = 0;
	ci.dev_minor = 0;
	ci.dev_info_argc = 1;
	ci.dev_info_argv = dev_info_argv;
	ci.flags = CUSE_UNRESTRICTED_IOCTL;

	cdev->session = cuse_lowlevel_new(NULL, &ci, &ll_ops, cdev);
	free(name);

	if (!cdev->session) {
		log_error("cannot create fuse-ll session");
		return -ENOMEM;
	}

	cdev->fd = open(fname, O_RDWR | O_CLOEXEC);
	if (cdev->fd < 0) {
		log_error("cannot open %s (%d): %m", fname, errno);
		ret = -EFAULT;
		goto err_session;
	}

	bufsize = getpagesize() + 0x1000;
	if (bufsize < 0x21000)
		bufsize = 0x21000;

	cdev->bufsize = bufsize;
	cdev->buf = malloc(bufsize);
	if (!cdev->buf) {
		log_error("cannot allocate memory for buffer of size %zu",
			  bufsize);
		ret = -ENOMEM;
		goto err_fd;
	}

	/* Argh! libfuse does not use "const" for the "chan_ops" pointer so we
	 * actually have to cast it. Their implementation does not write into it
	 * so we can safely use a constant storage for it.
	 * TODO: Fix libfuse upstream! */
	cdev->channel = fuse_chan_new((void*)&chan_ops, cdev->fd, bufsize,
				      cdev);
	if (!cdev->channel) {
		log_error("cannot allocate fuse-channel");
		ret = -ENOMEM;
		goto err_buf;
	}

	ret = ev_eloop_new_fd(cdev->eloop, &cdev->efd, cdev->fd, EV_READABLE,
			      channel_event, cdev);
	if (ret) {
		log_error("cannot create fd-object in eloop: %d", ret);
		goto err_chan;
	}

	fuse_session_add_chan(cdev->session, cdev->channel);
	return 0;

err_chan:
	fuse_chan_destroy(cdev->channel);
err_buf:
	free(cdev->buf);
err_fd:
	close(cdev->fd);
err_session:
	fuse_session_destroy(cdev->session);
	return ret;
}

void kmscon_cdev_destroy(struct kmscon_cdev *cdev)
{
	if (!cdev)
		return;

	fuse_session_destroy(cdev->session);
	ev_eloop_rm_fd(cdev->efd);
	free(cdev->buf);
	close(cdev->fd);
}

static int session_event(struct kmscon_session *session,
			 struct kmscon_session_event *ev, void *data)
{
	struct kmscon_cdev *cdev = data;

	switch (ev->type) {
	case KMSCON_SESSION_UNREGISTER:
		log_debug("destroy cdev session");
		kmscon_cdev_destroy(cdev);
		free(cdev);
		break;
	}

	return 0;
}

int kmscon_cdev_register(struct kmscon_session **out,
			 struct kmscon_seat *seat)
{
	struct kmscon_cdev *cdev;
	int ret;

	if (!out || !seat)
		return -EINVAL;

	cdev = malloc(sizeof(*cdev));
	if (!cdev)
		return -ENOMEM;
	memset(cdev, 0, sizeof(*cdev));
	cdev->seat = seat;
	cdev->eloop = kmscon_seat_get_eloop(seat);
	cdev->input = kmscon_seat_get_input(seat);

	ret = kmscon_cdev_init(cdev);
	if (ret)
		goto err_free;

	ret = kmscon_seat_register_session(seat, &cdev->s, session_event, cdev);
	if (ret) {
		log_error("cannot register session for cdev: %d", ret);
		goto err_cdev;
	}

	*out = cdev->s;
	return 0;

err_cdev:
	kmscon_cdev_destroy(cdev);
err_free:
	free(cdev);
	return ret;
}
