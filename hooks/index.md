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
        <td>list_all_meta</td>
        <!-- _  -->
    </tr>
    <tr>
        <td>hooks/playlist</td>
        <td>list_queue_meta</td>
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
        <td>outputs</td>
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
* **MPD\_DATABASE\_UPDATE\_TIME**: A date specifying last update time.  
  (in format: "%Y-%m-%d %H-%M-%S %Z")
* **MPD\_DATABASE\_ARTISTS**: Number of artists in the database.
* **MPD\_DATABASE\_ALBUMS**: Number of albums in the database.
* **MPD\_DATABASE\_SONGS**: Number of songs in the database.
* **MPD\_DATABASE\_PLAY\_TIME**: Accumulated time mpd was playing music since the process was started.
* **MPD\_DATABASE\_UPTIME**: Uptime of mpd in seconds.
* **MPD\_DATABASE\_DB\_PLAY\_TIME**: Accumulated duration of all songs in the database.
### status
* **MPD\_STATUS\_VOLUME**: Volume
* **MPD\_STATUS\_REPEAT**: Repeat (boolean, 0 or 1)
* **MPD\_STATUS\_RANDOM**: Random (boolean, 0 or 1)
* **MPD\_STATUS\_SINGLE**: Single (boolean, 0 or 1)
* **MPD\_STATUS\_CONSUME**: Consume (boolean, 0 or 1)
* **MPD\_STATUS\_QUEUE\_LENGTH**: Queue/Playlist length.
* **MPD\_STATUS\_CROSSFADE**: Crossfade in seconds.
* **MPD\_STATUS\_SONG\_POS**: Position of the current playing song.
* **MPD\_STATUS\_SONG\_ID**: ID of the current playing song.
* **MPD\_STATUS\_ELAPSED\_TIME**: Elapsed time in seconds
* **MPD\_STATUS\_ELAPSED\_MS**: Elapsed time in milliseconds.
* **MPD\_STATUS\_TOTAL\_TIME**: Total time in seconds.
* **MPD\_STATUS\_KBIT\_RATE**: Current bit rate in kbps.
* **MPD\_STATUS\_UPDATE\_ID**: The ID of the update.
* **MPD\_STATUS\_STATE**: State, one of **play**, **pause**, **stop** or **unknown**
* **MPD\_STATUS\_AUDIO\_FORMAT**: Specifies whether audio format is available (boolean, 0 or 1)
* **MPD\_STATUS\_AUDIO\_FORMAT\_SAMPLE\_RATE**: The sample rate in Hz.
* **MPD\_STATUS\_AUDIO\_FORMAT\_BITS**: The number of significant bits per sample.
* **MPD\_STATUS\_AUDIO\_FORMAT\_CHANNELS**: The number of channels. 1 for mono, 2 for stereo.
### outputs
* **MPD\_OUTPUT\_ID\_%d**: Where **%d** is a number (starting from 1), specifies the name of the given output ID.
* **MPD\_OUTPUT\_ID\_%d\_ENABLED:** Where **%d** is a number (starting from 1), specifies
  whether the output is enabled (boolean, 0 or 1)
### list\_queue\_meta
* All songs in the queue are set in environment. The variables are like in **currentsong**
  except they get a number like: 
  **MPD\_SONG\_URI** becomes **MPD\_SONG\_%d\_URI** where **%d** is a number starting from 1.
### list\_all\_meta
* **MPD\_PLAYLIST\_%d\_PATH**: Where **%d** is a number starting from 1. Specifies the path of
  the playlist.
* **MPD\_PLAYLIST\_%d\_LAST\_MODIFIED**: Where **%d** is a number starting from 1. Specifies the
  last modification time (in format: "%Y-%m-%d %H-%M-%S %Z")

<!-- vim: set tw=80 ft=mkd spell spelllang=en sw=4 sts=4 et : -->
