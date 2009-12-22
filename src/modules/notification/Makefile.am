AM_CFLAGS= -DGITHEAD=\"$(GITHEAD)\" \
	   $(glib_CFLAGS) $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS) $(libnotify_CFLAGS) \
	   -DLIBDIR=\"$(libdir)\" @MPDCRON_CFLAGS@
SUBDIRS= .
MODULE_DIR=$(libdir)/$(PACKAGE)-$(VERSION)/modules

noinst_HEADERS= notification-defs.h
notification_LTLIBRARIES= notification.la
notificationdir=$(MODULE_DIR)
notification_la_SOURCES= cover.c notification.c main.c
notification_la_LDFLAGS= -module -avoid-version
notification_la_LIBADD= $(glib_LIBS) $(libdaemon_LIBS) $(libmpdclient_LIBS) $(libnotify_LIBS)
