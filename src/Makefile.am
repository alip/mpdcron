AM_CFLAGS= -DGITHEAD=\"$(GITHEAD)\" \
	   $(glib_CFLAGS) $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS) \
	   @MPDCRON_CFLAGS@

bin_PROGRAMS= mpdcron
noinst_HEADERS= cron-defs.h
mpdcron_SOURCES= cron-conf.c cron-env.c cron-event.c cron-hooker.c \
		   cron-keyfile.c cron-loop.c cron-main.c
mpdcron_LDADD= $(glib_LIBS) $(libdaemon_LIBS) $(libmpdclient_LIBS)
