CLEANFILES= *~
MAINTAINERCLEANFILES= Makefile.in configure aclocal.m4 \
		      config.h config.h.in INSTALL
ACLOCAL_AMFLAGS= -I m4
AUTOMAKE_OPTIONS= dist-bzip2 no-dist-gzip std-options foreign
EXTRA_DIST= autogen.sh README.mkd NEWS.mkd\
	    conf/mpdcron.conf \
	    conf/hooks/database conf/hooks/mixer conf/hooks/options \
	    conf/hooks/output conf/hooks/player conf/hooks/playlist \
	    conf/hooks/queue conf/hooks/stored_playlist conf/hooks/update \
	    conf/hooks/dump-outputs.rb conf/hooks/dump-playlists.rb \
	    conf/hooks/dump-queue.rb conf/hooks/dump-song.bash \
	    conf/hooks/dump-stats.bash conf/hooks/dump-status.bash
SUBDIRS = src .
doc_DATA = README.mkd NEWS.mkd

checksum: dist
	@echo "SHA1 $(PACKAGE)-$(VERSION).tar.bz2"
	sha1sum $(PACKAGE)-$(VERSION).tar.bz2 > $(PACKAGE)-$(VERSION).tar.bz2.sha1sum
	@echo "SIGN $(PACKAGE)-$(VERSION).tar.bz2"
	gpg --detach-sign --armor $(PACKAGE)-$(VERSION).tar.bz2

upload:
	@echo "UPLOAD $(PACKAGE)-$(VERSION).tar.bz2*"
	scp $(PACKAGE)-$(VERSION).tar.bz2* bach.exherbo.org:public_html/mpdcron
