EXTRA_DIST = mpdcron.1.pdc mpdcron.1 eugene.1.pdf eugene.1
BUILT_SOURCES = mpdcron.1 eugene.1
dist_man_MANS= mpdcron.1 eugene.1

all: mpdcron.1 eugene.1

%.1: %.1.pdc
	pandoc -s -S --from=markdown --to=man $< -o $@
