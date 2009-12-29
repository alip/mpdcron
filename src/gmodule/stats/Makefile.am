SUBDIRS= .
DEFS+= -DGITHEAD=\"$(GITHEAD)\" -DLIBDIR=\"$(libdir)\"
AM_CFLAGS= @MPDCRON_CFLAGS@ \
	   $(gio_unix_CFLAGS) $(glib_CFLAGS) $(gio_CFLAGS) \
	   $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS) $(sqlite_CFLAGS)
MODULE_DIR=$(libdir)/$(PACKAGE)-$(VERSION)/modules

noinst_HEADERS= fifo_buffer.h tokenizer.h stats-defs.h stats-sqlite.h
stats_LTLIBRARIES= stats.la
statsdir=$(MODULE_DIR)
stats_la_SOURCES= fifo_buffer.c tokenizer.c \
		  stats-command.c stats-file.c stats-server.c \
		  stats-mpdclient.c stats-sqlite.c stats-module.c
stats_la_LDFLAGS= -module -avoid-version
stats_la_LIBADD= $(glib_LIBS) $(gio_unix_LIBS) $(gio_LIBS) \
		 $(libdaemon_LIBS) $(libmpdclient_LIBS) $(sqlite_LIBS)

# I am the eggman!
noinst_HEADERS+= walrus-defs.h
bin_PROGRAMS= walrus
walrus_SOURCES= stats-sqlite.c walrus-utils.c walrus-main.c
# Hack to workaround the error:
#     object x created both with libtool and without.
# See: http://bit.ly/libtool_both
walrus_CFLAGS= $(AM_CFLAGS)
walrus_LDADD= $(glib_LIBS) $(libmpdclient_LIBS) $(sqlite_LIBS)

# Careful with that axe!
# aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa!
#bin_PROGRAMS= eugene
# Hack to workaround the error:
#     object x created both with libtool and without.
# See: http://bit.ly/libtool_both
#eugene_CFLAGS= $(AM_CFLAGS)
# Another hack to make log functions work without linking to libdaemon.
# See gmodule.h
#eugene_CFLAGS+= -DCAREFUL_WITH_THAT_AXE=1 -D"mpdcron_log(level, ...)=eulog((level), __VA_ARGS__)"
#eugene_LDADD= $(glib_LIBS) $(libmpdclient_LIBS) $(sqlite_LIBS)
#eugene_SOURCES= stats-sqlite.c \
#		eugene-hate.c eugene-love.c \
#		eugene-kill.c eugene-unkill.c \
#		eugene-rate.c eugene-load.c \
#		eugene-update.c eugene-utils.c eugene-main.c
