#
# Kmscon - build configuration script
# Copyright (c) 2012 David Herrmann <dh.herrmann@googlemail.com>
#

AC_PREREQ(2.68)

AC_INIT([kmscon], [5])
AC_SUBST(PACKAGE_URL, [https://github.com/dvdhrm/kmscon])
AC_CONFIG_SRCDIR([src/kmscon_main.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE([foreign 1.11 subdir-objects dist-bzip2 no-dist-gzip tar-pax -Wall -Werror])
AM_SILENT_RULES([yes])

#
# Don't add a default "-g -O2" if CFLAGS wasn't specified. For debugging it is
# often more convenient to have "-g -O0". You can still override it by
# explicitely setting it on the command line.
#

: ${CFLAGS=""}

AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

LT_PREREQ(2.2)
LT_INIT

#
# pkg-config dependencies
# This unconditionally checks for all dependencies even if they are disabled. We
# later look whether all required depedencies are met and finish the
# configuration. We group similar packages into one logical group here to avoid
# having variables for each single library.
# This, however, makes ./configure output very unintuitive error messages if a
# package is not found so we must make sure we print more verbose messages
# ourself.
#

PKG_CHECK_MODULES([XKBCOMMON], [xkbcommon],
                  [have_xkbcommon=yes], [have_xkbcommon=no])
AC_SUBST(XKBCOMMON_CFLAGS)
AC_SUBST(XKBCOMMON_LIBS)

PKG_CHECK_MODULES([WAYLAND], [wayland-client wayland-server wayland-cursor],
                  [have_wayland=yes], [have_wayland=no])
AC_SUBST(WAYLAND_CFLAGS)
AC_SUBST(WAYLAND_LIBS)

PKG_CHECK_MODULES([SYSTEMD], [libsystemd-daemon libsystemd-login],
                  [have_systemd=yes], [have_systemd=no])
AC_SUBST(SYSTEMD_CFLAGS)
AC_SUBST(SYSTEMD_LIBS)

PKG_CHECK_MODULES([UDEV], [libudev],
                  [have_udev=yes], [have_udev=no])
AC_SUBST(UDEV_CFLAGS)
AC_SUBST(UDEV_LIBS)

PKG_CHECK_MODULES([DBUS], [dbus-1],
                  [have_dbus=yes], [have_dbus=no])
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

PKG_CHECK_MODULES([DRM], [libdrm],
                  [have_drm=yes], [have_drm=no])
AC_SUBST(DRM_CFLAGS)
AC_SUBST(DRM_LIBS)

PKG_CHECK_MODULES([GBM], [gbm],
                  [have_gbm=yes], [have_gbm=no])
AC_SUBST(GBM_CFLAGS)
AC_SUBST(GBM_LIBS)

PKG_CHECK_MODULES([EGL], [egl],
                  [have_egl=yes], [have_egl=no])
AC_SUBST(EGL_CFLAGS)
AC_SUBST(EGL_LIBS)

PKG_CHECK_MODULES([GLES2], [glesv2],
                  [have_gles2=yes], [have_gles2=no])
AC_SUBST(GLES2_CFLAGS)
AC_SUBST(GLES2_LIBS)

PKG_CHECK_MODULES([FREETYPE2], [freetype2 fontconfig],
                  [have_freetype2=yes], [have_freetype2=no])
AC_SUBST(FREETYPE2_CFLAGS)
AC_SUBST(FREETYPE2_LIBS)

PKG_CHECK_MODULES([PANGO], [pango pangoft2],
                  [have_pango=yes], [have_pango=no])
AC_SUBST(PANGO_CFLAGS)
AC_SUBST(PANGO_LIBS)

PKG_CHECK_MODULES([FUSE], [fuse],
                  [have_fuse=yes], [have_fuse=no])
AC_SUBST(FUSE_CFLAGS)
AC_SUBST(FUSE_LIBS)

PKG_CHECK_MODULES([PCIACCESS], [pciaccess],
                  [have_pciaccess=yes], [have_pciaccess=no])
AC_SUBST(PCIACCESS_CFLAGS)
AC_SUBST(PCIACCESS_LIBS)

#
# Parse arguments
# This parses all arguments that are given via "--enable-XY" or "--with-XY" and
# saves the results in local variables. This does not check dependencies or
# similar but does only parse the arguments.
#

# eloop
AC_MSG_CHECKING([whether user wants eloop])
AC_ARG_ENABLE([eloop],
              [AS_HELP_STRING([--enable-eloop],
                              [build eloop library])])
if test "x$enable_eloop" = "x" ; then
        enable_eloop="no (default)"
fi
AC_MSG_RESULT([$enable_eloop])

# TSM
AC_MSG_CHECKING([whether user wants TSM])
AC_ARG_ENABLE([tsm],
              [AS_HELP_STRING([--enable-tsm],
                              [build tsm library])])
if test "x$enable_tsm" = "x" ; then
        enable_tsm="no (default)"
fi
AC_MSG_RESULT([$enable_tsm])

# uterm
AC_MSG_CHECKING([whether user wants uterm])
AC_ARG_ENABLE([uterm],
              [AS_HELP_STRING([--enable-uterm],
                              [build uterm library])])
if test "x$enable_uterm" = "x" ; then
        enable_uterm="no (default)"
fi
AC_MSG_RESULT([$enable_uterm])

# kmscon
AC_MSG_CHECKING([whether user wants kmscon])
AC_ARG_ENABLE([kmscon],
              [AS_HELP_STRING([--disable-kmscon],
                              [do not build kmscon])])
if test "x$enable_kmscon" = "x" ; then
        enable_kmscon="yes (default)"
fi
AC_MSG_RESULT([$enable_kmscon])

# wlterm
AC_MSG_CHECKING([whether user wants wlterm])
AC_ARG_ENABLE([wlterm],
              [AS_HELP_STRING([--enable-wlterm],
                              [build wlterm])])
if test "x$enable_wlterm" = "x" ; then
        enable_wlterm="yes (default)"
fi
AC_MSG_RESULT([$enable_wlterm])

# debug
AC_MSG_CHECKING([whether to build with debugging on])
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug],
                              [whether to build with debugging on])])
if test "x$enable_debug" = "x" ; then
        enable_debug="yes (default)"
fi
AC_MSG_RESULT([$enable_debug])

# optimizations
AC_MSG_CHECKING([whether to disable code optimizations])
AC_ARG_ENABLE([optimizations],
              [AS_HELP_STRING([--disable-optimizations],
                              [whether to disable code optimizations])])
if test "x$enable_optimizations" = "x" ; then
        enable_optimizations="yes (default)"
fi
AC_MSG_RESULT([$enable_optimizations])

# multi-seat
AC_MSG_CHECKING([whether user wants systemd for multi-seat support])
AC_ARG_ENABLE([multi-seat],
              [AS_HELP_STRING([--enable-multi-seat],
                              [enable multi-seat support with systemd])])
if test "x$enable_multi_seat" = "x" ; then
        enable_multi_seat="yes (default)"
fi
AC_MSG_RESULT([$enable_multi_seat])

# hotplug
AC_MSG_CHECKING([whether user wants udev for device hotplug support])
AC_ARG_ENABLE([hotplug],
              [AS_HELP_STRING([--enable-hotplug],
                              [enable device hotplug support with udev])])
if test "x$enable_hotplug" = "x" ; then
        enable_hotplug="yes (default)"
fi
AC_MSG_RESULT([$enable_hotplug])

# pciaccess
AC_MSG_CHECKING([whether user wants pciaccess device detection])
AC_ARG_ENABLE([pciaccess],
              [AS_HELP_STRING([--enable-pciaccess],
                              [enable device detection via pciaccess])])
if test "x$enable_pciaccess" = "x" ; then
        enable_pciaccess="yes (default)"
fi
AC_MSG_RESULT([$enable_pciaccess])

# eloop-dbus
AC_MSG_CHECKING([whether user wants eloop dbus support])
AC_ARG_ENABLE([eloop-dbus],
              [AS_HELP_STRING([--enable-eloop-dbus],
                              [enable eloop dbus support])])
if test "x$enable_eloop_dbus" = "x" ; then
        enable_eloop_dbus="no (default)"
fi
AC_MSG_RESULT([$enable_eloop_dbus])

# video backends
AC_MSG_CHECKING([which video backends the user wants])
AC_ARG_WITH([video],
            [AS_HELP_STRING([--with-video],
              [specify list of optional video backends])])
enable_video_fbdev="no"
enable_video_dumb="no"
enable_video_drm="no"
if test "x$with_video" = "x" ; then
        enable_video_fbdev="yes (default)"
        enable_video_dumb="yes (default)"
        enable_video_drm="yes (default)"
        with_video="fbdev,dumb,drm (default)"
else
        SAVEIFS="$IFS"
        IFS=","
        for i in $with_video ; do
                if test "x$i" = "xfbdev" ; then
                        enable_video_fbdev="yes"
                elif test "x$i" = "xdumb" ; then
                        enable_video_dumb="yes"
                elif test "x$i" = "xdrm" ; then
                        enable_video_drm="yes"
                else
                        IFS="$SAVEIFS"
                        AC_ERROR([Invalid video backend $i])
                fi
        done
        IFS="$SAVEIFS"
fi
AC_MSG_RESULT([$with_video])

# renderers
AC_MSG_CHECKING([which render backends the user wants])
AC_ARG_WITH([renderers],
            [AS_HELP_STRING([--with-renderers],
              [specify list of optional render backends])])
enable_renderer_bblit="no"
enable_renderer_bbulk="no"
enable_renderer_gltex="no"
if test "x$with_renderers" = "x" ; then
        enable_renderer_bblit="yes (default)"
        enable_renderer_bbulk="yes (default)"
        enable_renderer_gltex="yes (default)"
        with_renderers="bblit,bbulk,gltex (default)"
else
        SAVEIFS="$IFS"
        IFS=","
        for i in $with_renderers ; do
                if test "x$i" = "xbblit" ; then
                        enable_renderer_bblit="yes"
                elif test "x$i" = "xbbulk" ; then
                        enable_renderer_bbulk="yes"
                elif test "x$i" = "xgltex" ; then
                        enable_renderer_gltex="yes"
                else
                        IFS="$SAVEIFS"
                        AC_ERROR([Unknown renderer $i])
                fi
        done
        IFS="$SAVEIFS"
fi
AC_MSG_RESULT([$with_renderers])

# font backends
AC_MSG_CHECKING([which font backends the user wants])
AC_ARG_WITH([fonts],
            [AS_HELP_STRING([--with-fonts],
              [specify list of optional font backends])])
enable_font_8x16="no"
enable_font_unifont="no"
enable_font_freetype2="no"
enable_font_pango="no"
if test "x$with_fonts" = "x" ; then
        enable_font_8x16="yes (default)"
        enable_font_unifont="no (default)"
        enable_font_freetype2="yes (default)"
        enable_font_pango="yes (default)"
        with_fonts="8x16,freetype2,pango (default)"
else
        SAVEIFS="$IFS"
        IFS=","
        for i in $with_fonts ; do
                if test "x$i" = "x8x16" ; then
                        enable_font_8x16="yes"
                elif test "x$i" = "xunifont" ; then
                        enable_font_unifont="yes"
                elif test "x$i" = "xfreetype2" ; then
                        enable_font_freetype2="yes"
                elif test "x$i" = "xpango" ; then
                        enable_font_pango="yes"
                else
                        IFS="$SAVEIFS"
                        AC_ERROR([Unknown font backend $i])
                fi
        done
        IFS="$SAVEIFS"
fi
AC_MSG_RESULT([$with_fonts])

# kmscon sessions
AC_MSG_CHECKING([which sessions the user wants])
AC_ARG_WITH([sessions],
            [AS_HELP_STRING([--with-sessions],
              [specify list of optional sessions])])
enable_session_dummy="no"
enable_session_terminal="no"
enable_session_cdev="no"
enable_session_compositor="no"
if test "x$with_sessions" = "x" ; then
        enable_session_dummy="yes (default)"
        enable_session_terminal="yes (default)"
        enable_session_cdev="yes (default)"
        enable_session_compositor="no (default)"
        with_sessions="dummy,terminal,cdev (default)"
else
        SAVEIFS="$IFS"
        IFS=","
        for i in $with_sessions ; do
                if test "x$i" = "xdummy" ; then
                        enable_session_dummy="yes"
                elif test "x$i" = "xterminal" ; then
                        enable_session_terminal="yes"
                elif test "x$i" = "xcdev" ; then
                        enable_session_cdev="yes"
                elif test "x$i" = "xcompositor" ; then
                        enable_session_compositor="yes"
                else
                        IFS="$SAVEIFS"
                        AC_ERROR([Unknown session type $i])
                fi
        done
        IFS="$SAVEIFS"
fi
AC_MSG_RESULT([$with_sessions])

#
# Check what can be built
# This checks each configuration option and tests whether all dependencies are
# met. This is done from bottom up to ensure the right order.
#

# debug
debug_avail=no
if test ! "x$enable_debug" = "xno" ; then
        debug_avail=yes
fi

# optimizations
optimizations_avail=no
if test ! "x$enable_optimizations" = "xno" ; then
        optimizations_avail=yes
fi

# eloop-dbus
eloop_dbus_avail=no
if test ! "x$enable_eloop_dbus" = "xno" ; then
        if test "x$have_dbus" = "xyes" ; then
                eloop_dbus_avail=yes
        elif test "x$enable_eloop_dbus" = "xyes" ; then
                AC_ERROR([DBus libraries not found for eloop-dbus])
        fi
fi

# eloop
eloop_avail=no
if test ! "x$enable_eloop" = "xno" ; then
        eloop_avail=yes
fi

# TSM
tsm_avail=no
if test ! "x$enable_tsm" = "xno" ; then
        if test "x$have_xkbcommon" = "xyes" ; then
                tsm_avail=yes
        elif test "x$enable_tsm" = "xyes" ; then
                AC_ERROR([xkbcommon libraries not found for TSM])
        fi
fi

# video fbdev
video_fbdev_avail=no
if test ! "x$enable_video_fbdev" = "xno" ; then
        # TODO: check for kernel headers
        video_fbdev_avail=yes
fi

# video dumb
video_dumb_avail=no
if test ! "x$enable_video_dumb" = "xno" ; then
        if test "x$have_drm" = "xyes" ; then
                video_dumb_avail=yes
        elif test "x$enable_video_dumb" = "xyes" ; then
                AC_ERROR([DRM libraries not found for dumb video backend])
        fi
fi

# video drm
video_drm_avail=no
if test ! "x$enable_video_drm" = "xno" ; then
        if test "x$have_drm" = "xno" ; then
                video_drm_avail=no
        elif test "x$have_gbm" = "xno" ; then
                video_drm_avail=no
        elif test "x$have_egl" = "xno" ; then
                video_drm_avail=no
        elif test "x$have_gles2" = "xno" ; then
                video_drm_avail=no
        else
                video_drm_avail=yes
        fi

        if test "x$video_drm_avail" = "xno" ; then
                if test "x$enable_video_drm" = "xyes" ; then
                        AC_ERROR([DRM, gbm or EGL libraries not found for DRM video backend])
                fi
        fi
fi

# uterm
uterm_avail=no
if test ! "x$enable_uterm" = "xno" ; then
        uterm_avail=yes
fi

# multi-seat
multi_seat_avail=no
if test ! "x$enable_multi_seat" = "xno" ; then
        if test "x$have_systemd" = "xyes" ; then
                multi_seat_avail=yes
        elif test "x$enable_multi_seat" = "xyes" ; then
                AC_ERROR([systemd libraries not found for multi-seat support])
        fi
fi

# hotplug
hotplug_avail=no
if test ! "x$enable_hotplug" = "xno" ; then
        if test "x$have_udev" = "xyes" ; then
                hotplug_avail=yes
        elif test "x$enable_hotplug" = "xyes" ; then
                AC_ERROR([udev libraries not found for hotplug support])
        fi
fi

# pciaccess
pciaccess_avail=no
if test ! "x$enable_pciaccess" = "xno" ; then
        if test "x$have_pciaccess" = "xyes" ; then
                pciaccess_avail=yes
        elif test "x$enable_pciaccess" = "xyes" ; then
                AC_ERROR([pciaccess libraries not found for pciaccess device detection])
        fi
fi

# renderer bblit
renderer_bblit_avail=no
if test ! "x$enable_renderer_bblit" = "xno" ; then
        renderer_bblit_avail=yes
fi

# renderer bbulk
renderer_bbulk_avail=no
if test ! "x$enable_renderer_bbulk" = "xno" ; then
        renderer_bbulk_avail=yes
fi

# renderer gltex
renderer_gltex_avail=no
if test ! "x$enable_renderer_gltex" = "xno" ; then
        if test "x$have_gles2" = "xyes" ; then
                renderer_gltex_avail=yes
        elif test "x$enable_renderer_gltex" = "xyes" ; then
                AC_ERROR([OpenGLESv2 libraries not found for gltex renderer])
        fi
fi

# font 8x16
font_8x16_avail=no
if test ! "x$enable_font_8x16" = "xno" ; then
        font_8x16_avail=yes
fi

# font unifont
font_unifont_avail=no
if test ! "x$enable_font_unifont" = "xno" ; then
        font_unifont_avail=yes
fi

# font freetype2
font_freetype2_avail=no
if test ! "x$enable_font_freetype2" = "xno" ; then
        if test "x$have_freetype2" ; then
                font_freetype2_avail=yes
        elif test "x$enable_font_freetype2" = "xyes" ; then
                AC_ERROR([freetype2 or fontconfig libs not found for freetype2 font backend])
        fi
fi

# font pango
font_pango_avail=no
if test ! "x$enable_font_pango" = "xno" ; then
        if test "x$have_pango" ; then
                font_pango_avail=yes
        elif test "x$enable_font_pango" = "xyes" ; then
                AC_ERROR([pango libraries not found for pango font backend])
        fi
fi

# session dummy
session_dummy_avail=no
if test ! "x$enable_session_dummy" = "xno" ; then
        session_dummy_avail=yes
fi

# session terminal
session_terminal_avail=no
if test ! "x$enable_session_terminal" = "xno" ; then
        if test "x$tsm_avail" = "xyes" ; then
                session_terminal_avail=yes
        elif test "x$enable_session_terminal" = "xyes" ; then
                AC_ERROR([TSM cannot be built for kmscon terminal session])
        fi
fi

# session cdev
session_cdev_avail=no
if test ! "x$enable_session_cdev" = "xno" ; then
        if test "x$have_fuse" = "xno" ; then
                session_cdev_avail=no
        elif test "x$tsm_avail" = "xno" ; then
                session_cdev_avail=no
        else
                session_cdev_avail=yes
        fi

        if test "x$session_cdev_avail" = "xno" ; then
                if test "x$enable_session_cdev" = "xyes" ; then
                        AC_ERROR([TSM or FUSE libraries not built/found for kmscon cdev session])
                fi
        fi
fi

# session compositor
session_compositor_avail=no
if test ! "x$enable_session_compositor" = "xno" ; then
        if test "x$have_wayland" = "xyes" ; then
                session_compositor_avail=yes
        elif test "x$enable_session_compositor" = "xyes" ; then
                AC_ERROR([wayland libraries not found for kmscon compositor session])
        fi
fi

# kmscon
kmscon_avail=no
if test ! "x$enable_kmscon" = "xno" ; then
        if test "x$eloop_avail" = "xno" ; then
                kmscon_avail=no
        elif test "x$tsm_avail" = "xno" ; then
                kmscon_avail=no
        elif test "x$uterm_avail" = "xno" ; then
                kmscon_avail=no
        else
                kmscon_avail=yes
        fi

        if test "x$enable_kmscon" = "xyes" ; then
                if test "x$kmscon_avail" = "xno" ; then
                        AC_ERROR([eloop, TSM or uterm cannot be built for kmscon])
                fi
        fi
fi

# wlterm
wlterm_avail=no
if test ! "x$enable_wlterm" = "xno" ; then
        if test "x$tsm_avail" = "xno" ; then
                wlterm_avail=no
        elif test "x$eloop_avail" = "xno" ; then
                wlterm_avail=no
        else
                wlterm_avail=yes
        fi

        if test "x$enable_wlterm" = "xyes" ; then
                if test "x$wlterm_avail" = "xno" ; then
                        AC_ERROR([eloop or TSM cannot be built for wlterm])
                fi
        fi

        if test "x$have_wayland" = "xyes" ; then
                wlterm_avail=yes
        elif test "x$enable_wlterm" = "xyes" ; then
                AC_ERROR([wayland libraries not found for wlterm])
        fi
fi

#
# Enable all required modules
# We now know which modules can be built by checking the *_avail variables set
# above. We now only have to disable all modules that are disabled by default
# and the user didn't force-enable them and no other module that is enabled
# needs them. This is done top-down of course.
#

# wlterm
wlterm_enabled=no
if test "x$wlterm_avail" = "xyes" ; then
        if test "x${enable_wlterm% *}" = "xyes" ; then
                wlterm_enabled=yes
                enable_eloop=yes
                enable_tsm=yes
        fi
fi

# kmscon
kmscon_enabled=no
if test "x$kmscon_avail" = "xyes" ; then
        if test "x${enable_kmscon% *}" = "xyes" ; then
                kmscon_enabled=yes
                enable_eloop=yes
                enable_uterm=yes
        fi
fi

# session compositor
session_compositor_enabled=no
if test "x$session_compositor_avail" = "xyes" ; then
        if test "x${enable_session_compositor% *}" = "xyes" ; then
                session_compositor_enabled=yes
        fi
fi

# session cdev
session_cdev_enabled=no
if test "x$session_cdev_avail" = "xyes" ; then
        if test "x${enable_session_cdev% *}" = "xyes" ; then
                session_cdev_enabled=yes
        fi
fi

# session terminal
session_terminal_enabled=no
if test "x$session_terminal_avail" = "xyes" ; then
        if test "x${enable_session_terminal% *}" = "xyes" ; then
                session_terminal_enabled=yes
                enable_tsm=yes
        fi
fi

# session dummy
session_dummy_enabled=no
if test "x$session_dummy_avail" = "xyes" ; then
        if test "x${enable_session_dummy% *}" = "xyes" ; then
                session_dummy_enabled=yes
        fi
fi

# font pango
font_pango_enabled=no
if test "x$font_pango_avail" = "xyes" ; then
        if test "x${enable_font_pango% *}" = "xyes" ; then
                font_pango_enabled=yes
        fi
fi

# font freetype2
font_freetype2_enabled=no
if test "x$font_freetype2_avail" = "xyes" ; then
        if test "x${enable_font_freetype2% *}" = "xyes" ; then
                font_freetype2_enabled=yes
        fi
fi

# font unifont
font_unifont_enabled=no
if test "x$font_unifont_avail" = "xyes" ; then
        if test "x${enable_font_unifont% *}" = "xyes" ; then
                font_unifont_enabled=yes
        fi
fi

# font 8x16
font_8x16_enabled=no
if test "x$font_8x16_avail" = "xyes" ; then
        if test "x${enable_font_8x16% *}" = "xyes" ; then
                font_8x16_enabled=yes
        fi
fi

# renderer gltex
renderer_gltex_enabled=no
if test "x$renderer_gltex_avail" = "xyes" ; then
        if test "x${enable_renderer_gltex% *}" = "xyes" ; then
                renderer_gltex_enabled=yes
        fi
fi

# renderer bbulk
renderer_bbulk_enabled=no
if test "x$renderer_bbulk_avail" = "xyes" ; then
        if test "x${enable_renderer_bbulk% *}" = "xyes" ; then
                renderer_bbulk_enabled=yes
        fi
fi

# renderer bblit
renderer_bblit_enabled=no
if test "x$renderer_bblit_avail" = "xyes" ; then
        if test "x${enable_renderer_bblit% *}" = "xyes" ; then
                renderer_bblit_enabled=yes
        fi
fi

# pciaccess
pciaccess_enabled=no
if test "x$pciaccess_avail" = "xyes" ; then
        if test "x${enable_pciaccess% *}" = "xyes" ; then
                pciaccess_enabled=yes
        fi
fi

# hotplug
hotplug_enabled=no
if test "x$hotplug_avail" = "xyes" ; then
        if test "x${enable_hotplug% *}" = "xyes" ; then
                hotplug_enabled=yes
        fi
fi

# multi-seat
multi_seat_enabled=no
if test "x$multi_seat_avail" = "xyes" ; then
        if test "x${enable_multi_seat% *}" = "xyes" ; then
                multi_seat_enabled=yes
        fi
fi

# uterm
uterm_enabled=no
if test "x$uterm_avail" = "xyes" ; then
        if test "x${enable_uterm% *}" = "xyes" ; then
                uterm_enabled=yes
        fi
fi

# video drm
video_drm_enabled=no
if test "x$video_drm_avail" = "xyes" ; then
        if test "x${enable_video_drm% *}" = "xyes" ; then
                video_drm_enabled=yes
        fi
fi

# video dumb
video_dumb_enabled=no
if test "x$video_dumb_avail" = "xyes" ; then
        if test "x${enable_video_dumb% *}" = "xyes" ; then
                video_dumb_enabled=yes
        fi
fi

# video fbdev
video_fbdev_enabled=no
if test "x$video_fbdev_avail" = "xyes" ; then
        if test "x${enable_video_fbdev% *}" = "xyes" ; then
                video_fbdev_enabled=yes
        fi
fi

# tsm
tsm_enabled=no
if test "x$tsm_avail" = "xyes" ; then
        if test "x${enable_tsm% *}" = "xyes" ; then
                tsm_enabled=yes
        fi
fi

# eloop
eloop_enabled=no
if test "x$eloop_avail" = "xyes" ; then
        if test "x${enable_eloop% *}" = "xyes" ; then
                eloop_enabled=yes
        fi
fi

# eloop-dbus
eloop_dbus_enabled=no
if test "x$eloop_dbus_avail" = "xyes" ; then
        if test "x${enable_eloop_dbus% *}" = "xyes" ; then
                eloop_dbus_enabled=yes
        fi
fi

# optimizations
optimizations_enabled=no
if test "x$optimizations_avail" = "xyes" ; then
        if test "x${enable_optimizations% *}" = "xyes" ; then
                optimizations_enabled=yes
        fi
fi

# debug
debug_enabled=no
if test "x$debug_avail" = "xyes" ; then
        if test "x${enable_debug% *}" = "xyes" ; then
                debug_enabled=yes
        fi
fi

#
# Module Configuration
# We have now done all dependency checking and default-value validation and we
# now know which modules are enabled via the *_enabled variables.
# Everything below is related to the configuration of each module and setting
# the correct flags for the build process.
#

# debug
if test "x$debug_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_DEBUG], [1], [Enable debug mode])
else
        AC_DEFINE([NDEBUG], [1], [No Debug])
fi

AM_CONDITIONAL([BUILD_ENABLE_DEBUG],
               [test "x$debug_enabled" = "xyes"])

# optimizations
AM_CONDITIONAL([BUILD_ENABLE_OPTIMIZATIONS],
               [test "x$optimizations_enabled" = "xyes"])

# eloop-dbus
AM_CONDITIONAL([BUILD_ENABLE_ELOOP_DBUS],
               [test "x$eloop_dbus_enabled" = "xyes"])

# eloop
AM_CONDITIONAL([BUILD_ENABLE_ELOOP],
               [test "x$eloop_enabled" = "xyes"])

# TSM
AM_CONDITIONAL([BUILD_ENABLE_TSM],
               [test "x$tsm_enabled" = "xyes"])

# video fbdev
if test "x$video_fbdev_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_VIDEO_FBDEV], [1],
                  [Build uterm fbdev video backend])
fi

AM_CONDITIONAL([BUILD_ENABLE_VIDEO_FBDEV],
               [test "x$video_fbdev_enabled" = "xyes"])

# video dumb
if test "x$video_dumb_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_VIDEO_DUMB], [1],
                  [Build uterm dumb drm video backend])
fi

AM_CONDITIONAL([BUILD_ENABLE_VIDEO_DUMB],
               [test "x$video_dumb_enabled" = "xyes"])

# video drm
if test "x$video_drm_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_VIDEO_DRM], [1],
                  [Build uterm drm video backend])
fi

AM_CONDITIONAL([BUILD_ENABLE_VIDEO_DRM],
               [test "x$video_drm_enabled" = "xyes"])

# uterm
AM_CONDITIONAL([BUILD_ENABLE_UTERM],
               [test "x$uterm_enabled" = "xyes"])

# multi-seat
if test "x$multi_seat_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_MULTI_SEAT], [1],
                  [Use systemd for multi-seat support])
fi

AM_CONDITIONAL([BUILD_ENABLE_MULTI_SEAT],
               [test "x$multi_seat_enabled" = "xyes"])

# hotplug
if test "x$hotplug_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_HOTPLUG], [1],
                  [Use udev for hotplug support])
fi

AM_CONDITIONAL([BUILD_ENABLE_HOTPLUG],
               [test "x$hotplug_enabled" = "xyes"])

# pciaccess
if test "x$pciaccess_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_PCIACCESS], [1],
                  [Use pciaccess for device detection])
fi

AM_CONDITIONAL([BUILD_ENABLE_PCIACCESS],
               [test "x$pciaccess_enabled" = "xyes"])

# renderer bblit
if test "x$renderer_bblit_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_RENDERER_BBLIT], [1],
                  [Build bblit rendering backend])
fi

AM_CONDITIONAL([BUILD_ENABLE_RENDERER_BBLIT],
               [test "x$renderer_bblit_enabled" = "xyes"])

# renderer bbulk
if test "x$renderer_bbulk_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_RENDERER_BBULK], [1],
                  [Build bbulk rendering backend])
fi

AM_CONDITIONAL([BUILD_ENABLE_RENDERER_BBULK],
               [test "x$renderer_bbulk_enabled" = "xyes"])

# renderer gltex
if test "x$renderer_gltex_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_RENDERER_GLTEX], [1],
                  [Build gltex rendering backend])
fi

AM_CONDITIONAL([BUILD_ENABLE_RENDERER_GLTEX],
               [test "x$renderer_gltex_enabled" = "xyes"])

# font 8x16
if test "x$font_8x16_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_FONT_8X16], [1],
                  [Build static 8x16 font backend])
fi

AM_CONDITIONAL([BUILD_ENABLE_FONT_8X16],
               [test "x$font_8x16_enabled" = "xyes"])

# font unifont
if test "x$font_unifont_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_FONT_UNIFONT], [1],
                  [Build static unifont backend])
fi

AM_CONDITIONAL([BUILD_ENABLE_FONT_UNIFONT],
               [test "x$font_unifont_enabled" = "xyes"])

# font freetype2
if test "x$font_freetype2_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_FONT_FREETYPE2], [1],
                  [Build freetype2 font backend])
fi

AM_CONDITIONAL([BUILD_ENABLE_FONT_FREETYPE2],
               [test "x$font_freetype2_enabled" = "xyes"])

# font pango
if test "x$font_pango_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_FONT_PANGO], [1],
                  [Build pango font backend])
fi

AM_CONDITIONAL([BUILD_ENABLE_FONT_PANGO],
               [test "x$font_pango_enabled" = "xyes"])

# session dummy
if test "x$session_dummy_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_SESSION_DUMMY], [1],
                  [Build dummy session])
fi

AM_CONDITIONAL([BUILD_ENABLE_SESSION_DUMMY],
               [test "x$session_dummy_enabled" = "xyes"])

# session terminal
if test "x$session_terminal_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_SESSION_TERMINAL], [1],
                  [Build terminal session])
fi

AM_CONDITIONAL([BUILD_ENABLE_SESSION_TERMINAL],
               [test "x$session_terminal_enabled" = "xyes"])

# session cdev
if test "x$session_cdev_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_SESSION_CDEV], [1],
                  [Build cdev session])
fi

AM_CONDITIONAL([BUILD_ENABLE_SESSION_CDEV],
               [test "x$session_cdev_enabled" = "xyes"])

# session compositor
if test "x$session_compositor_enabled" = "xyes" ; then
        AC_DEFINE([BUILD_ENABLE_SESSION_COMPOSITOR], [1],
                  [Build compositor session])
fi

AM_CONDITIONAL([BUILD_ENABLE_SESSION_COMPOSITOR],
               [test "x$session_compositor_enabled" = "xyes"])

# kmscon
AM_CONDITIONAL([BUILD_ENABLE_KMSCON],
               [test "x$kmscon_enabled" = "xyes"])

# wlterm
AM_CONDITIONAL([BUILD_ENABLE_WLTERM],
               [test "x$wlterm_enabled" = "xyes"])

#
# Miscellaneous Checks
# All checks below are independent of module checking or depend on the results
# of it. They do not have any dependencies themself so they are not part of the
# module infrastructure.
#

# check for _Static_assert
AC_MSG_CHECKING([whether _Static_assert() is supported])
AC_LANG([C])
have_static_assert=yes
AC_COMPILE_IFELSE([AC_LANG_SOURCE([[_Static_assert(1, "What?");]])],
                  [AC_DEFINE([BUILD_HAVE_STATIC_ASSERT],
                             [1],
                             [Define to 1 if _Static_assert() is supported])],
                  [have_static_assert=no])
AC_MSG_RESULT([$have_static_assert])

# check for gbm_bo_get_pitch() function, otherwise gbm_bo_get_stride() is used
if test x$have_gbm = xyes ; then
        save_CFLAGS="$CFLAGS"
        save_LIBS="$LIBS"
        save_LDFLAGS="$LDFLAGS"
        CFLAGS="$DRM_CFLAGS $GBM_CFLAGS"
        LIBS="$DRM_LIBS $GBM_LIBS"
        LDFLAGS=""
        AC_CHECK_LIB([gbm],
                     [gbm_bo_get_pitch],
                     [AC_DEFINE([BUILD_HAVE_GBM_BO_GET_PITCH],
                                [1],
                                [Define to 1 if your libgbm provides gbm_bo_get_pitch])])
        CFLAGS="$save_CFLAGS"
        LIBS="$save_LIBS"
        LDFLAGS="$save_LDFLAGS"
fi

#
# Makefile vars
# After everything is configured, we correctly substitute the values for the
# makefiles.
#

AC_CONFIG_FILES([Makefile
                 src/libeloop.pc
                 src/libtsm.pc
                 src/libuterm.pc])
AC_OUTPUT

#
# Configuration output
# Show configuration to the user so they can check whether everything was
# configured as expected.
#

AC_MSG_NOTICE([Build configuration:

  Applications and Libraries:
               kmscon: $kmscon_enabled ($kmscon_avail)
               wlterm: $wlterm_enabled ($wlterm_avail)
                uterm: $uterm_enabled ($uterm_avail)
                  tsm: $tsm_enabled ($tsm_avail)
                eloop: $eloop_enabled ($eloop_avail)

  Miscellaneous Options:
                debug: $debug_enabled ($debug_avail)
        optimizations: $optimizations_enabled ($optimizations_avail)
           multi-seat: $multi_seat_enabled ($multi_seat_avail)
              hotplug: $hotplug_enabled ($hotplug_avail)
            pciaccess: $pciaccess_enabled ($pciaccess_avail)
           eloop-dbus: $eloop_dbus_enabled ($eloop_dbus_avail)

  Video Backends:
                fbdev: $video_fbdev_enabled ($video_fbdev_avail)
             dumb drm: $video_dumb_enabled ($video_dumb_avail)
                  drm: $video_drm_enabled ($video_drm_avail)

  Font Backends:
                 8x16: $font_8x16_enabled ($font_8x16_avail)
              unifont: $font_unifont_enabled ($font_unifont_avail)
            freetype2: $font_freetype2_enabled ($font_freetype2_avail)
                pango: $font_pango_enabled ($font_pango_avail)

  Renderers:
                bblit: $renderer_bblit_enabled ($renderer_bblit_avail)
                bbulk: $renderer_bbulk_enabled ($renderer_bbulk_avail)
                gltex: $renderer_gltex_enabled ($renderer_gltex_avail)

  Session Types:
                dummy: $session_dummy_enabled ($session_dummy_avail)
             terminal: $session_terminal_enabled ($session_terminal_avail)
                 cdev: $session_cdev_enabled ($session_cdev_avail)
           compositor: $session_compositor_enabled ($session_compositor_avail)

        Run "make" to start compilation process])
