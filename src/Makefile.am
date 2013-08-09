DEFS+= -DGITHEAD=\"$(GITHEAD)\" -DLIBDIR=\"$(libdir)\"
AM_CFLAGS= @MPDCRON_CFLAGS@ $(glib_CFLAGS) $(libdaemon_CFLAGS) $(libmpdclient_CFLAGS)

bin_PROGRAMS= mpdcron
noinst_HEADERS= cron-config.h cron-defs.h
mpdcron_SOURCES= cron-conf.c cron-env.c cron-event.c cron-hooker.c \
		 cron-keyfile.c cron-log.c cron-loop.c cron-main.c
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

CPPCHECK=cppcheck
cppcheck:
	$(CPPCHECK) $(mpdcron_SOURCES) $(DEFS) --std=c99 --std=posix --enable=all
.PHONY: cppcheck

SPARSE=sparse
SPARSE_CPPFLAGS= $(DEFAULT_INCLUDES) \
		 -D__STDC_VERSION__=199901L \
		 -Wbitwise -Wcast-to-as -Wdefault-bitfield-sign \
		 -Wparen-string -Wptr-subtraction-blows \
		 -Wreturn-void -Wshadow -Wtypesign -Wundef \
		 -I$(shell $(CC) -print-file-name=include) \
		 -I$(shell $(CC) -print-file-name=include-fixed)
# Fix this flag for your architecture!
SPARSE_CPPFLAGS+= -D__x86_64__=1

sparse-check:
	for src in $(mpdcron_SOURCES); \
	do \
		$(SPARSE) $(DEFS) $(AM_CFLAGS) $(SPARSE_CPPFLAGS) $$src || exit 1; \
	done
.PHONY: sparse-check
