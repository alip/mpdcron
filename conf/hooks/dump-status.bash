#!/usr/bin/env bash
# vim: set sw=4 et sts=4 tw=80 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

echo "=========="
echo "MPD STATUS"
echo "=========="
echo "Volume: $MPD_STATUS_VOLUME"
echo "Repeat: $MPD_STATUS_REPEAT" # Boolean, 0 or 1
echo "Random: $MPD_STATUS_RANDOM" # Boolean, 0 or 1
echo "Single: $MPD_STATUS_SINGLE" # Boolean, 0 or 1
echo "Consume: $MPD_STATUS_CONSUME" # Boolean, 0 or 1
echo "Queue version: $MPD_STATUS_QUEUE_VERSION"
echo "Crossfade: $MPD_STATUS_CROSSFADE"
echo "Song position: $MPD_STATUS_SONG_POS"
echo "Song ID: $MPD_STATUS_SONG_ID"
echo "Elapsed time: $MPD_STATUS_ELAPSED_TIME"
echo "Elapsed milliseconds: $MPD_STATUS_ELAPSED_MS"
echo "Total time: $MPD_STATUS_TOTAL_TIME"
echo "Kbit Rate: $MPD_STATUS_KBIT_RATE"
echo "Update id: $MPD_STATUS_UPDATE_ID"
echo "State: $MPD_STATUS_STATE" # One of play, pause, stop, unknown
echo "Audio format: $MPD_STATUS_AUDIO_FORMAT" # Boolean, 0 or 1
if [[ "$MPD_STATUS_AUDIO_FORMAT" == 1 ]]; then
    echo "Samplerate: $MPD_STATUS_AUDIO_FORMAT_SAMPLE_RATE"
    echo "Bits: $MPD_STATUS_AUDIO_FORMAT_BITS"
    echo "Channels: $MPD_STATUS_AUDIO_FORMAT_CHANNELS"
fi
echo "=========="
