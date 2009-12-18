#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

echo "=== MPD CURRENT SONG ==="
echo "Uri: $MPD_SONG_URI"
echo "Last modified: $MPD_SONG_LAST_MODIFIED"
echo "Duration: $MPD_SONG_DURATION"
echo "Position: $MPD_SONG_POSITION"
echo "ID: $MPD_SONG_ID"
# Note: Currently only the first tags are exported
echo "--- TAGS ---"
echo "Artist: $MPD_SONG_TAG_ARTIST"
echo "Album: $MPD_SONG_TAG_ALBUM"
echo "Album_Artist: $MPD_SONG_ALBUM_ARTIST"
echo "Title: $MPD_SONG_TAG_TITLE"
echo "Track: $MPD_SONG_TAG_TRACK"
echo "Name: $MPD_SONG_TAG_NAME"
echo "Genre: $MPD_SONG_TAG_GENRE"
echo "Date: $MPD_SONG_TAG_GENRE"
echo "Composer: $MPD_SONG_TAG_COMPOSER"
echo "Performer: $MPD_SONG_TAG_PERFORMER"
echo "Comment: $MPD_SONG_TAG_COMMENT"
echo "Disc: $MPD_SONG_TAG_DISC"
echo "Musicbrainz Artist ID: $MPD_SONG_TAG_MUSICBRAINZ_ARTISTID"
echo "Musicbrainz Album ID: $MPD_SONG_TAG_MUSICBRAINZ_ALBUMID"
echo "Musicbrainz AlbumArtist ID: $MPD_SONG_TAG_MUSICBRAINZ_ALBUMARTISTID"
echo "Musicbrainz Track ID: $MPD_SONG_TAG_MUSICBRAINZ_TRACKID"
echo "------------"
echo "========================"
