SUBDIRS= .
if WANT_INOTIFY
SUBDIRS+= inotify
endif
if WANT_NOTIFICATION
SUBDIRS+= notification
endif
if WANT_SCROBBLER
SUBDIRS+= scrobbler
endif

mpdcron_module_includedir=$(includedir)/mpdcron
mpdcron_module_include_HEADERS= gmodule.h
