---
layout: default
title: Modules
---

To load a module, the user has to mention its name in the event section of the
configuration file. <tt>mpdcron</tt> looks for the modules first under
MPDCRON\_DIR/modules/ where **MPDCRON\_DIR** is ~/.mpdcron by default. Next it
looks at LIBDIR/mpdcron-VERSION/modules where **LIBDIR** is /usr/lib on most
systems.

### Configuration
To load modules <tt>notification</tt> and <tt>scrobbler</tt> add this to your configuration file:

    [player]
    # modules is a semicolon delimited list of modules to load.
    modules = notification;scrobbler

### Writing Modules
Check [mpdcron/gmodule.h](http://github.com/alip/mpdcron/blob/master/src/gmodule/gmodule.h) and
[example.c](http://github.com/alip/mpdcron/blob/master/conf/modules/example.c) to learn how to write
<tt>mpdcron</tt> modules.

<!-- vim: set tw=80 ft=mkd spell spelllang=en sw=4 sts=4 et : -->
