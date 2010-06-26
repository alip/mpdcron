---
layout: default
title: Hooks
---

Hooks are executed depending on the event received from <tt>mpd</tt>. Hooks are
stored under MPDCRON\_DIR/hooks where **MPDCRON\_DIR** is ~/.mpdcron by default.
Here's a list of hooks and commands run before them:  
  
<table border="1">
    <tr>
        <th>Hook</th>
        <th>Command</th>
    </tr>
    <tr>
        <td>hooks/database</td>
        <td>stats</td>
    </tr>
    <tr>
        <td>hooks/stored_playlist</td>
        <td>-</td>
        <!-- _  -->
    </tr>
    <tr>
        <td>hooks/playlist</td>
        <td>-</td>
        <!-- _ -->
    </tr>
    <tr>
        <td>hooks/player</td>
        <td>status</td>
    </tr>
    <tr>
        <td>hooks/mixer</td>
        <td>status</td>
    </tr>
    <tr>
        <td>hooks/output</td>
        <td>-</td>
    </tr>
    <tr>
        <td>hooks/options</td>
        <td>status</td>
    </tr>
    <tr>
        <td>hooks/update</td>
        <td>status</td>
    </tr>
</table>
  
Note: As of version 0.4, <tt>mpdcron</tt> doesn't send any commands to the
server before events **stored\_playlist**, **playlist** and **outputs** because
the number of environment variables may result in huge numbers causing
<tt>mpdcron</tt> to hang. See
[#3](http://github.com/alip/mpdcron/issues#issue/3) for more details.

## Environment variables

Here's a list of environment variables <tt>mpdcron</tt> sets depending on the command sent:

### currentsong
<table border="1">
    <tr>
        <th>Environment variable</th>
        <th>Explanation</th>
    </tr>
    <tr>
        <td>MPD_SONG_URI</td>
        <!-- _ -->
        <td>URI of the song</td>
    </tr>
    <tr>
        <td>MPD_SONG_LAST_MODIFIED</td>
        <!-- _ -->
        <td>Time of last modification<br />(format: "%Y-%m-%d %H-%M-%S %Z")</td>
    </tr>
    <tr>
        <td>MPD_SONG_DURATION</td>
        <!-- _ -->
        <td>Duration of the song in seconds</td>
    </tr>
    <tr>
        <td>MPD_SONG_DURATION</td>
        <!-- _ -->
        <td>Duration of the song in seconds</td>
    </tr>
    <tr>
        <td>MPD_SONG_POS</td>
        <!-- _ -->
        <td>Position of the song in the queue</td>
    </tr>
    <tr>
        <td>MPD_SONG_ID</td>
        <!-- _ -->
        <td>Song ID</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_ARTIST</td>
        <!-- _ -->
        <td>Artist tag</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_ALBUM</td>
        <!-- _ -->
        <td>Album tag</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_ALBUM_ARTIST</td>
        <!-- _ -->
        <td>Album artist tag</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_TITLE</td>
        <!-- _ -->
        <td>Title tag</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_TRACK</td>
        <!-- _ -->
        <td>Track tag</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_NAME</td>
        <!-- _ -->
        <td>Name tag</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_GENRE</td>
        <!-- _ -->
        <td>Genre tag</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_DATE</td>
        <!-- _ -->
        <td>Date tag</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_COMPOSER</td>
        <!-- _ -->
        <td>Composer tag</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_COMMENT</td>
        <!-- _ -->
        <td>Comment tag</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_DISC</td>
        <!-- _ -->
        <td>Disc tag</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_MUSICBRAINZ_ARTISTID</td>
        <!-- _ -->
        <td>Musicbrainz artist ID</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_MUSICBRAINZ_ALBUMID</td>
        <!-- _ -->
        <td>Musicbrainz album ID</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_MUSICBRAINZ_ALBUMARTISTID</td>
        <!-- _ -->
        <td>Musicbrainz album artist ID</td>
    </tr>
    <tr>
        <td>MPD_SONG_TAG_MUSICBRAINZ_TRACKID</td>
        <!-- _ -->
        <td>Musicbrainz track ID</td>
    </tr>
</table>

### stats
<table border="1">
    <tr>
        <th>Environment variable</th>
        <th>Explanation</th>
    </tr>
    <tr>
        <td>MPD_DATABASE_UPDATE_TIME</td>
        <!-- _ -->
        <td>Date specifying last update time<br />(format: "%Y-%m-%d %H-%M-%S %Z")</td>
    </tr>
    <tr>
        <td>MPD_DATABASE_ARTISTS</td>
        <!-- _ -->
        <td>Number of artists</td>
    </tr>
    <tr>
        <td>MPD_DATABASE_ALBUMS</td>
        <!-- _ -->
        <td>Number of albums</td>
    </tr>
    <tr>
        <td>MPD_DATABASE_SONGS</td>
        <!-- _ -->
        <td>Number of songs</td>
    </tr>
    <tr>
        <td>MPD_DATABASE_PLAY_TIME</td>
        <!-- _ -->
        <td>Accumulated time <tt>mpd</tt> was playing music since the process was started</td>
    </tr>
    <tr>
        <td>MPD_DATABASE_UPTIME</td>
        <!-- _ -->
        <td>Uptime of <tt>mpd</tt> in seconds</td>
    </tr>
    <tr>
        <td>MPD_DATABASE_DB_PLAY_TIME</td>
        <!-- _ -->
        <td>Accumulated duration of all songs</td>
    </tr>
</table>

### status
<table border="1">
    <tr>
        <th>Environment variable</th>
        <th>Explanation</th>
    </tr>
    <tr>
        <td>MPD_STATUS_VOLUME</td>
        <!-- _ -->
        <td>Volume</td>
    </tr>
    <tr>
        <td>MPD_STATUS_REPEAT</td>
        <!-- _ -->
        <td>Repeat<br />(boolean, 0 or 1)</td>
    </tr>
    <tr>
        <td>MPD_STATUS_RANDOM</td>
        <!-- _ -->
        <td>Random<br />(boolean, 0 or 1)</td>
    </tr>
    <tr>
        <td>MPD_STATUS_SINGLE</td>
        <!-- _ -->
        <td>Single<br />(boolean, 0 or 1)</td>
    </tr>
    <tr>
        <td>MPD_STATUS_CONSUME</td>
        <!-- _ -->
        <td>Consume<br />(boolean, 0 or 1)</td>
    </tr>
    <tr>
        <td>MPD_STATUS_QUEUE_LENGTH</td>
        <!-- _ -->
        <td>Queue/Playlist length</td>
    </tr>
    <tr>
        <td>MPD_STATUS_CROSSFADE</td>
        <!-- _ -->
        <td>Crossfade in seconds</td>
    </tr>
    <tr>
        <td>MPD_STATUS_SONG_POS</td>
        <!-- _ -->
        <td>Position of the current playing song</td>
    </tr>
    <tr>
        <td>MPD_STATUS_SONG_ID</td>
        <!-- _ -->
        <td>ID of the current playing song</td>
    </tr>
    <tr>
        <td>MPD_STATUS_ELAPSED_TIME</td>
        <!-- _ -->
        <td>Elapsed time in seconds</td>
    </tr>
    <tr>
        <td>MPD_STATUS_ELAPSED_MS</td>
        <!-- _ -->
        <td>Elapsed time in milliseconds</td>
    </tr>
    <tr>
        <td>MPD_STATUS_TOTAL_TIME</td>
        <!-- _ -->
        <td>Total time in seconds</td>
    </tr>
    <tr>
        <td>MPD_STATUS_KBIT_RATE</td>
        <!-- _ -->
        <td>Current bit rate in kbps</td>
    </tr>
    <tr>
        <td>MPD_STATUS_UPDATE_ID</td>
        <!-- _ -->
        <td>ID of the update</td>
    </tr>
    <tr>
        <td>MPD_STATUS_STATE</td>
        <!-- _ -->
        <td>State, one of play, pause, stop or unknown</td>
    </tr>
    <tr>
        <td>MPD_STATUS_AUDIO_FORMAT</td>
        <!-- _ -->
        <td>Specified whether audio format is available<br />(boolean, 0 or 1)</td>
    </tr>
    <tr>
        <td>MPD_STATUS_AUDIO_FORMAT_SAMPLE_RATE</td>
        <!-- _ -->
        <td>Sample rate in Hz</td>
    </tr>
    <tr>
        <td>MPD_STATUS_AUDIO_FORMAT_BITS</td>
        <!-- _ -->
        <td>Number of significant bits per sample</td>
    </tr>
    <tr>
        <td>MPD_STATUS_AUDIO_FORMAT_CHANNELS</td>
        <!-- _ -->
        <td>Number of channels<br />(1 for mono, 2 for stereo)</td>
    </tr>
</table>

<!-- vim: set tw=80 ft=mkd spell spelllang=en sw=4 sts=4 et : -->
