---
layout: default
title: Hooks
---

## Paths

<tt>mpdcron</tt> executes hooks depending on the event received from
<tt>mpd</tt>. Hooks are stored under MPDCRON\_DIR/hooks where **MPDCRON\_DIR**
is ~/.mpdcron by default. Here's a list of hooks and commands run before them: 
 
- **hooks/database**: <tt>mpdcron</tt> calls **stats** before this and updates the environment.
- **hooks/stored\_playlist**: <tt>mpdcron</tt> calls **list\_all\_meta** command and updates the environment.
- **hooks/playlist**: <tt>mpdcron</tt> calls **list\_queue\_meta** and updates the environment.
- **hooks/player**: <tt>mpdcron</tt> calls **status** and **currentsong** and updates the environment.
- **hooks/mixer**: <tt>mpdcron</tt> calls **status** and updates the environment.
- **hooks/output**: <tt>mpdcron</tt> calls **outputs** and updates the environment.
- **hooks/options**: <tt>mpdcron</tt> calls **status** and updates the environment.
- **hooks/update**: <tt>mpdcron</tt> calls **status** and updates the environment.

## Environment variables

Here's a list of environment variables <tt>mpdcron</tt> sets depending on the command sent:

### currentsong
* **MPD\_SONG\_URI**: URI of the song.
* **MPD\_SONG\_LAST\_MODIFIED**: Time of last modification.  
  (in format: "%Y-%m-%d %H-%M-%S %Z")
* **MPD\_SONG\_DURATION**: Duration in seconds of the song.
* **MPD\_SONG\_POS**: Position of this song in the queue.
* **MPD\_SONG\_ID**: ID of the song.
* **MPD\_SONG\_TAG\_ARTIST**: Artist tag of the song.
* **MPD\_SONG\_TAG\_ALBUM**: Album tag of the song.
* **MPD\_SONG\_TAG\_ALBUM\_ARTIST**: Album artist tag of the song.
* **MPD\_SONG\_TAG\_TITLE**: Title tag of the song.
* **MPD\_SONG\_TAG\_TRACK**: Track number tag of the song.
* **MPD\_SONG\_TAG\_NAME**: Name tag of the song.
* **MPD\_SONG\_TAG\_GENRE**: Genre tag of the song.
* **MPD\_SONG\_TAG\_DATE**: Date tag of the song.
* **MPD\_SONG\_TAG\_COMPOSER**: Composer tag of the song.
* **MPD\_SONG\_TAG\_PERFORMER**: Performer tag of the song.
* **MPD\_SONG\_TAG\_COMMENT**: Comment tag of the song.
* **MPD\_SONG\_TAG\_DISC**: Disc tag of the song.
* **MPD\_SONG\_TAG\_MUSICBRAINZ\_ARTISTID**: Musicbrainz Artist ID tag of the song.
* **MPD\_SONG\_TAG\_MUSICBRAINZ\_ALBUMID**: Musicbrainz Album ID tag of the song.
* **MPD\_SONG\_TAG\_MUSICBRAINZ\_ALBUMARTISTID**: Musicbrainz Album artist ID tag of the song.
* **MPD\_SONG\_TAG\_MUSICBRAINZ\_TRACKID**: Musicbrainz Track ID tag of the song.
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
