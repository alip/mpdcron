SUBDIRS= .
DEFS+= -DGITHEAD=\"$(GITHEAD)\" -DLIBDIR=\"$(libdir)\"
AM_CFLAGS= @MPDCRON_CFLAGS@ $(glib_CFLAGS) $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS)
MODULE_DIR=$(libdir)/$(PACKAGE)-$(VERSION)/modules

noinst_HEADERS= inotify-defs.h
inotify_LTLIBRARIES= inotify.la
inotifydir=$(MODULE_DIR)
inotify_la_SOURCES= inotify-file.c inotify-mpd.c  inotify-pattern.c inotify-module.c
inotify_la_LDFLAGS= -module -avoid-version
inotify_la_LIBADD= $(glib_LIBS) $(libdaemon_LIBS) $(libmpdclient_LIBS)
