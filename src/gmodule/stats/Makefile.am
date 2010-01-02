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
		  stats-sqlite.c stats-module.c
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
noinst_HEADERS+= eugene-defs.h
bin_PROGRAMS+= eugene
eugene_SOURCES= fifo_buffer.c eugene-connection.c eugene-addtag.c \
		eugene-kill.c eugene-list.c eugene-listinfo.c \
		eugene-listtags.c eugene-love.c eugene-rate.c \
		eugene-rmtag.c eugene-utils.c eugene-main.c
# Hack to workaround the error:
#     object x created both with libtool and without.
# See: http://bit.ly/libtool_both
eugene_CFLAGS= $(AM_CFLAGS)
eugene_LDADD= $(gio_unix_LIBS) $(gio_LIBS) $(glib_LIBS) $(libmpdclient_LIBS)
