CLEANFILES= *~
MAINTAINERCLEANFILES= Makefile.in configure aclocal.m4 \
		      config.h config.h.in INSTALL
ACLOCAL_AMFLAGS= -I m4
AUTOMAKE_OPTIONS= dist-bzip2 no-dist-gzip std-options foreign
EXTRA_DIST= autogen.sh README.mkd NEWS.mkd\
	    conf/mpdhooker.conf \
	    conf/hooks/database conf/hooks/mixer conf/hooks/options \
	    conf/hooks/output conf/hooks/player conf/hooks/playlist \
	    conf/hooks/queue conf/hooks/stored_playlist conf/hooks/update \
	    conf/hooks/dump-outputs.rb conf/hooks/dump-playlists.rb \
	    conf/hooks/dump-queue.rb conf/hooks/dump-song.bash \
	    conf/hooks/dump-stats.bash conf/hooks/dump-status.bash
SUBDIRS = src .

