AM_CFLAGS= -DGITHEAD=\"$(GITHEAD)\" \
	   $(glib_CFLAGS) $(gmodule_CFLAGS) $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS) \
	   -DLIBDIR=\"$(libdir)\" @MPDCRON_CFLAGS@

bin_PROGRAMS= mpdcron
noinst_HEADERS= cron-config.h cron-defs.h
mpdcron_SOURCES= cron-conf.c cron-env.c cron-event.c cron-hooker.c \
		 cron-keyfile.c cron-loop.c cron-main.c
if HAVE_GMODULE
mpdcron_SOURCES+= cron-gmodule.c
endif
mpdcron_LDADD= $(glib_LIBS) $(gmodule_LIBS) $(libdaemon_LIBS) $(libmpdclient_LIBS)

SUBDIRS= .
if HAVE_GMODULE
SUBDIRS+= gmodule
endif
