---
layout: default
title: Configuration
---

## Connection
By default <tt>mpdcron</tt> will get the host name and port for <tt>mpd</tt>
from **MPD\_HOST** and **MPD\_PORT** environment variables. **MPD\_PASSWORD**
environment variable can be set to make <tt>mpdcron</tt> connect to a
password-protected <tt>mpd</tt>. If these environment variables aren't set,
<tt>mpdcron</tt> connects to localhost on port 6600.

## Key File
At startup mpdcron reads its configuration from the file
MPDCRON\_DIR/mpdcron.conf where **MPDCRON\_DIR** is ~/.mpdcron by default.
Configuration file is in standard .ini format.

{% highlight ini %}

    # mpdcron configuration file
    [main]
    # Path to the PID file, by default the PID file is MPDCRON_DIR/mpdcron.pid
    pidfile = /path/to/mpdcron.pid
    # Wait this many seconds after sending signal to kill the daemon.
    # Default is 3 seconds.
    killwait = 3
    # Logging level, default is 0
    # 0: Warnings and errors only
    # 1: Info and below
    # 2: Debug and below
    log_level = 0
    [mpd]
    # Semicolon delimited list of events to wait for.
    # By default mpdcron waits for all events.
    # Valid events are:
    # database: Song database has been updated.
    # stored_playlist: A stored playlist has been modified, created,
    #   deleted or renamed.
    # playlist: The queue has been modified.
    # player: The player state has been changed: play, stop, pause, seek, ...
    # mixer: The volume has been modified.
    # output: An audio output device has been enabled or disabled.
    # options: Options have changed: crossfade, random, repeat, ...
    # update: A database update has started or finished.
    events = player;mixer
    # Interval in seconds to reconnect to mpd after an error or disconnect.
    reconnect = 5
    # Timeout in milliseconds for mpd timeout, 0 for default timeout of
    # libmpdclient.
    timeout = 0

{% endhighlight %}

<!-- vim: set tw=80 ft=mkd spell spelllang=en sw=4 sts=4 et : -->
