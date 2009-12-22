DEFS+= -DGITHEAD=\"$(GITHEAD)\" -DLIBDIR=\"$(libdir)\"
AM_CFLAGS= @MPDCRON_CFLAGS@ $(glib_CFLAGS) $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS)

bin_PROGRAMS= mpdcron
noinst_HEADERS= cron-config.h cron-defs.h
mpdcron_SOURCES= cron-conf.c cron-env.c cron-event.c cron-hooker.c \
		 cron-keyfile.c cron-loop.c cron-main.c
mpdcron_LDADD= $(glib_LIBS) $(libdaemon_LIBS) $(libmpdclient_LIBS)
if HAVE_GMODULE
AM_CFLAGS+= $(gmodule_CFLAGS)
mpdcron_LDADD+= $(gmodule_LIBS)
mpdcron_SOURCES+= cron-gmodule.c
endif

SUBDIRS= .
if HAVE_GMODULE
SUBDIRS+= gmodule
endif
