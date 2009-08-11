AM_CFLAGS= -DGIT_HEAD=\"$(GIT_HEAD)\" \
	   $(glib_CFLAGS) $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS) \
	   @MPDHOOKER_CFLAGS@

bin_PROGRAMS= mpdhooker
mpdhooker_SOURCES= main.c
mpdhooker_LDADD= $(glib_LIBS) $(libdaemon_LIBS) $(libmpdclient_LIBS)

