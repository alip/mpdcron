---
layout: default
title: Modules
---

To load a module, the user has to mention its name in the event section of the
configuration file. <tt>mpdcron</tt> looks for the modules first under
MPDCRON\_DIR/modules/ where **MPDCRON\_DIR** is ~/.mpdcron by default. Next it
looks at LIBDIR/mpdcron-VERSION/modules where **LIBDIR** is /usr/lib on most
systems.

## Configuration
To load modules <tt>notification</tt> and <tt>scrobbler</tt> add this to your configuration file:

    [player]
    # modules is a semicolon delimited list of modules to load.
    modules = notification;scrobbler

## Writing Modules
Check [mpdcron/gmodule.h](http://github.com/alip/mpdcron/blob/master/src/gmodule/gmodule.h) and
[example.c](http://github.com/alip/mpdcron/blob/master/conf/modules/example.c) to learn how to write
<tt>mpdcron</tt> modules.

## Standard Modules
Here is a list of <tt>mpdcron</tt>'s standard modules:

### notification

#### Features
- Uses notify-send to send notifications.
- Can detect repeated songs.

#### Configuration

    # mpdcron configuration file
    ...
    [main]
    modules = notification

    [notification]
    # Covers path, defaults to ~/.covers
    cover_path = /path/to/cover/path
    # Cover suffix, defaults to jpg
    cover_suffix = png
    # Notification timeout in milliseconds.
    timeout = 50000
    # Notification type
    type = mpd
    # Notification urgency, one of low, normal, critical
    urgency = normal
    # Notification hints in format TYPE:NAME:VALUE, specifies basic extra data
    # to pass. Valid types are int, double, string and byte
    hints =

### scrobbler
This module uses **curl** to submit songs to [Last.fm](http://last.fm) or
[Libre.fm](http://libre.fm). Here's an example configuration:

#### Features
- Uses [libcurl](http://curl.haxx.se/)
- last.fm protocol 1.2 (including "now playing")
- supports seeking, crossfading, repeated songs

#### Configuration

    # mpdcron configuration file
    ...
    [main]
    modules = scrobbler

    [scrobbler]
    # Http proxy to use, the module also respects http_proxy environment
    # variable.
    proxy = http://127.0.0.1:8080
    # Journal interval in seconds
    journal_interval = 60

    [libre.fm]
    url = http://turtle.libre.fm
    username = <libre.fm username here>
    # Password can be specified in two ways: either bare or in the form md5:MD5_HASH
    password = <libre.fm password here>

    [last.fm]
    url = http://post.audioscrobbler.com
    username = <last.fm username here>
    password = <last.fm password here>

### stats
This module saves song data to a [sqlite](http://www.sqlite.org/) database and
tracks play count.

#### Configuration

    # mpdcron configuration file
    ...
    [main]
    modules = stats

    [stats]
    # Path to the database default is MPDCRON_DIR/stats.db where MPDCRON_DIR is
    # ~/.mpdcron by default.
    dbpath = /path/to/database

#### Eugene
This module comes with a client called <tt>eugene</tt> which can be used to
interact with the statistics database. See **eugene --help** output for more
information.

<!-- vim: set tw=80 ft=mkd spell spelllang=en sw=4 sts=4 et : -->
