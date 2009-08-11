#!/bin/sh
# vim: set sw=4 et sts=4 tw=80 :

# MpdHooker example hook
# Copy this file to MPD_HOOKER_DIR/hooks/HOOK where MPD_HOOKER_DIR is
# ~/.mpdhooker by default and HOOK is the name of the hook.
# Make sure to make it executable.

echo "<<< Status Information >>>"
echo ">>> Bit Rate> old: $MPD_BITRATE_OLD new: $MPD_BITRATE"
if [ $MPD_CONSUME -eq 1 ]; then
    echo ">>> Consume> on"
else
    echo ">>> Consume> off"
fi
echo ">>> Crossfade> old: $MPD_CROSSFADE_OLD new: $MPD_CROSSFADE"
echo ">>> Elapsed> old: $MPD_ELAPSED_OLD new: $MPD_ELAPSED"
echo ">>> Playlist> old: $MPD_PLAYLIST_OLD new: $MPD_PLAYLIST"
echo ">>> Playlist Length> old: $MPD_PLAYLIST_LENGTH_OLD new: $MPD_PLAYLIST_LENGTH"
if [ $MPD_RANDOM -eq 1 ]; then
    echo ">>> Random> on"
else
    echo ">>> Random> off"
fi
if [ $MPD_REPEAT -eq 1 ]; then
    echo ">>> Repeat> on"
else
    echo ">>> Repeat> off"
fi
echo ">>> Sample Rate> old: $MPD_SAMPLE_RATE_OLD new: $MPD_SAMPLE_RATE"
if [ $MPD_SINGLE -eq 1 ]; then
    echo ">>> Single> on"
else
    echo ">>> Single> off"
fi
echo ">>> State> old: $MPD_STATE_OLD new: $MPD_STATE"
echo ">>> Total time> $MPD_TOTAL_TIME"
if [ $MPD_UPDATING_DB -eq 1 ]; then
    echo ">>> Updating DB> on"
else
    echo ">>> Updating DB> off"
fi
echo ">>> Volume> old: $MPD_VOLUME_OLD new: $MPD_VOLUME"
echo
echo "<<< Song Information >>>"
echo ">>> Filename> old: '$MPD_SONG_FILENAME_OLD' new: '$MPD_SONG_FILENAME'"
echo ">>> Artist> old: '$MPD_SONG_ARTIST_OLD' new: '$MPD_SONG_ARTIST'"
echo ">>> Album> old: '$MPD_SONG_ALBUM_OLD' new: '$MPD_SONG_ALBUM'"
echo ">>> Title> old: '$MPD_SONG_TITLE_OLD' new: '$MPD_SONG_TITLE'"
echo ">>> Track> old: '$MPD_SONG_TRACK_OLD' new: '$MPD_SONG_TRACK'"
echo ">>> Name> old: '$MPD_SONG_NAME_OLD' new: '$MPD_SONG_NAME'"
echo ">>> Date> old: '$MPD_SONG_DATE_OLD' new: '$MPD_SONG_DATE'"

