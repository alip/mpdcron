SUBDIRS= .
DEFS+= -DGITHEAD=\"$(GITHEAD)\" -DLIBDIR=\"$(libdir)\"
AM_CFLAGS= @MPDCRON_CFLAGS@ $(glib_CFLAGS) $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS) $(sqlite_CFLAGS)
MODULE_DIR=$(libdir)/$(PACKAGE)-$(VERSION)/modules

noinst_HEADERS= stats-defs.h
stats_LTLIBRARIES= stats.la
statsdir=$(MODULE_DIR)
stats_la_SOURCES= stats-file.c stats-sqlite.c stats-module.c
stats_la_LDFLAGS= -module -avoid-version
stats_la_LIBADD= $(glib_LIBS) $(libdaemon_LIBS) $(libmpdclient_LIBS) $(sqlite_LIBS)

# Careful with that axe!
# aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa!
bin_PROGRAMS= eugene
# Hack to workaround the error:
#     object x created both with libtool and without.
# See: http://bit.ly/libtool_both
eugene_CFLAGS= $(AM_CFLAGS)
# Another hack to make log functions work without linking to libdaemon.
# See gmodule.h
eugene_CFLAGS+= -D"mpdcron_log(level, ...)=eulog((level), __VA_ARGS__)"
eugene_LDADD= $(glib_LIBS) $(libmpdclient_LIBS) $(sqlite_LIBS)
eugene_SOURCES= stats-sqlite.c \
		eugene-hate-song.c eugene-love-song.c \
		eugene-kill-song.c eugene-unkill-song.c \
		eugene-update.c eugene-utils.c eugene-main.c
