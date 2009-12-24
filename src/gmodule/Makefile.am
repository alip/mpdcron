SUBDIRS= .
if WANT_NOTIFICATION
SUBDIRS+= notification
endif
if WANT_SCROBBLER
SUBDIRS+= scrobbler
endif

noinst_HEADERS= utils.h

mpdcron_module_includedir=$(includedir)/mpdcron
mpdcron_module_include_HEADERS= gmodule.h
