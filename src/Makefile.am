AM_CFLAGS= -DGITHEAD=\"$(GITHEAD)\" \
	   $(glib_CFLAGS) $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS) \
	   @MPDHOOKER_CFLAGS@

LIBMPD_SRC= resolver.c resolver.h song.c song.h str_pool.c str_pool.h \
	    libmpdclient.c libmpdclient.h
bin_PROGRAMS= mpdhooker
mpdhooker_SOURCES= conf.h conf.c diff.h diff.c hooker.h hooker.c \
		   keyfile.h keyfile.c loop.h loop.c mpd.h mpd.c \
		   $(LIBMPD_SRC) main.c
mpdhooker_LDADD= $(glib_LIBS) $(libdaemon_LIBS) $(libmpdclient_LIBS)

