SUBDIRS= .
DEFS+= -DGITHEAD=\"$(GITHEAD)\" -DLIBDIR=\"$(libdir)\"
AM_CFLAGS= @MPDCRON_CFLAGS@ $(glib_CFLAGS) $(libcurl_CFLAGS) $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS)
MODULE_DIR=$(libdir)/$(PACKAGE)-$(VERSION)/modules

noinst_HEADERS= scrobbler-defs.h
scrobbler_LTLIBRARIES= scrobbler.la
scrobblerdir=$(MODULE_DIR)
scrobbler_la_SOURCES= scrobbler-curl.c scrobbler-file.c scrobbler-journal.c \
		      scrobbler-record.c scrobbler-submit.c scrobbler-timer.c \
		      scrobbler-module.c
scrobbler_la_LDFLAGS= -module -avoid-version
scrobbler_la_LIBADD= $(glib_LIBS) $(libcurl_LIBS) $(libdaemon_LIBS) $(libmpdclient_LIBS)
