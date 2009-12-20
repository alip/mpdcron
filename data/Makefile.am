EXTRA_DIST = mpdcron.1.pdc mpdcron.1
BUILT_SOURCES = mpdcron.1
dist_man_MANS= mpdcron.1

all: mpdcron.1

mpdcron.1: mpdcron.1.pdc
	pandoc -s -S --from=markdown --to=man $< -o $@
