---
layout: default
title: Home
---

## About
[mpdcron](/mpdcron) is a cron like daemon for [mpd](http://mpd.wikia.com/).

## Features
- Uses <tt>mpd</tt>'s idle mode.
- Calls hooks depending on the event.
- Sets special environment variables to pass data to the hooks.
- Optional support for modules via
  [GModule](http://library.gnome.org/devel/glib/unstable/glib-Dynamic-Loading-of-Modules.html).

## Installation
[mpdcron](/mpdcron) uses [autotools](http://sources.redhat.com/autobook/) and
the installation procedure is the standard:

    ./configure
    make
    sudo make install

Apart from the standard configure options you can pass the options below to
change the behaviour of [mpdcron](/mpdcron):

    ./configure --help
    [...]
    --enable-gmodule    enable support for C modules (via GModule)
    --with-standard-modules=foo,bar,...
         Build the specified standard modules:
         all              Build all available standard modules
         notification     Build the notification module
         scrobbler        Build the scrobbler module
         stats            Build the stats module
    [...]

## Requirements
Here's a list of packages you need to have in your system to build
[mpdcron](/mpdcron):
- [libdaemon](http://0pointer.de/lennart/projects/libdaemon/)-0.13 or newer
- [libmpdclient](http://mpd.wikia.com/wiki/ClientLib:libmpdclient)-2.1 or newer
- [glib](http://library.gnome.org/devel/glib/)-2.18 or newer

Some modules have additional requirements:

### notification

* Requires <tt>notify-send</tt> which comes with
  [libnotify](http://www.galago-project.org/)

### scrobbler

* [libcurl](http://curl.haxx.se/)

### stats

* [sqlite](http://www.sqlite.org)-3 or newer
* [GIO](http://library.gnome.org/devel/gio/)-2.22 or newer
* The <tt>homescrape</tt> script requires [ruby](http://www.ruby-lang.org/) and
  [nokogiri](http://nokogiri.org/). Optionally
  [chronic](http://chronic.rubyforge.org/) can be installed to parse dates in a
  huge variety of date and time formats, for the **--since** option.

## Packages
Some distributions have <tt>mpdcron</tt> packaged so you can install it
easily using their package management system:
<table border="1">
    <tr>
        <th>Distribution</th>
        <th>Package/Repository</th>
    </tr>
    <tr>
        <td><a href="http://www.archlinux.org/">Arch Linux</a></td>
        <td>
            <a
            href="http://aur.archlinux.org/packages.php?ID=32890">
                mpdcron-git (aur)
            </a>
        </td>
    </tr>
    <tr>
        <td><a href="http://www.exherbo.org/">Exherbo</a></td>
        <td>
            <a
            href="http://git.exherbo.org/summer/packages/media-sound/mpdcron/">
                media-sound/mpdcron
            </a>
        </td>
    </tr>
    <tr>
        <td><a href="http://www.ubuntu.com/">Ubuntu</a></td>
        <td>
            <a
            href="https://launchpad.net/~gmpc-trunk/+archive/mpd-trunk">
                mpd-trunk (ppa)
            </a>
        </td>
    </tr>
</table>

## Contribute
Clone [git://github.com/alip/mpdcron.git](git://github.com/alip/mpdcron.git).  
Format patches are preferred. Either mail me or poke me on irc.  
My personal email address is [alip@exherbo.org](mailto:alip@exherbo.org).  
I'm available on IRC as <tt>alip</tt> on [Freenode](http://www.freenode.net/) and [OFTC](http://www.oftc.net/).

<!-- vim: set tw=80 ft=mkd spell spelllang=en sw=4 sts=4 et : -->
