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
To load module <tt>notification</tt> <tt>notification</tt> add this to your configuration file:

    [player]
    # modules is a semicolon delimited list of modules to load.
    modules = notification;scrobbler

<!-- vim: set tw=80 ft=mkd spell spelllang=en sw=4 sts=4 et : -->
