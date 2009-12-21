AM_CFLAGS= -DGITHEAD=\"$(GITHEAD)\" \
	   $(glib_CFLAGS) $(gmodule_CFLAGS) $(libcurl_CFLAGS) $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS) \
	   -DLIBDIR=\"$(libdir)\" @MPDCRON_CFLAGS@
SUBDIRS= .
MODULE_DIR=$(libdir)/$(PACKAGE)-$(VERSION)

noinst_HEADERS= scrobbler-defs.h
scrobbler_LTLIBRARIES= scrobbler.la
scrobblerdir=$(MODULE_DIR)
scrobbler_la_SOURCES= file.c http_client_curl.c journal.c record.c scrobbler.c main.c
scrobbler_la_LDFLAGS= -module -avoid-version
scrobbler_la_LIBADD= $(glib_LIBS) $(gmodule_LIBS) $(libcurl_LIBS) $(libdaemon_LIBS) $(libmpdclient_LIBS)
