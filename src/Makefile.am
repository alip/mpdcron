AM_CFLAGS= -DGITHEAD=\"$(GITHEAD)\" \
	   $(glib_CFLAGS) $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS) \
	   @MPDHOOKER_CFLAGS@

bin_PROGRAMS= mpdhooker
noinst_HEADERS= mh-defs.h
mpdhooker_SOURCES= mh-conf.c mh-env.c mh-event.c mh-hooker.c \
		   mh-keyfile.c mh-loop.c mh-main.c
mpdhooker_LDADD= $(glib_LIBS) $(libdaemon_LIBS) $(libmpdclient_LIBS)
