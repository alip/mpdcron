SUBDIRS= .
DEFS+= -DGITHEAD=\"$(GITHEAD)\" -DLIBDIR=\"$(libdir)\"
AM_CFLAGS= @MPDCRON_CFLAGS@ $(glib_CFLAGS) $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS)
MODULE_DIR=$(libdir)/$(PACKAGE)-$(VERSION)/modules

noinst_HEADERS= notification-defs.h
notification_LTLIBRARIES= notification.la
notificationdir=$(MODULE_DIR)
notification_la_SOURCES= notification-cover.c notification-file.c notification-spawn.c notification-module.c
notification_la_LDFLAGS= -module -avoid-version
notification_la_LIBADD= $(glib_LIBS) $(libdaemon_LIBS) $(libmpdclient_LIBS)
