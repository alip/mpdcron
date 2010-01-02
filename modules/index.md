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
    # Password can be specified in two ways: either bare or in the form md5:MD5-HASH
    password = <libre.fm password here>

    [last.fm]
    url = http://post.audioscrobbler.com
    username = <last.fm username here>
    password = <last.fm password here>

### stats
This module saves song data to a [sqlite](http://www.sqlite.org/) database.

#### Features
- Supports loving, killing, rating and tagging songs, artists, albums and genres.
- Tracks play count of songs, artist, albums and genres.
- Implements a simple server protocol for remote clients to receive data.

#### Configuration

    # mpdcron configuration file
    ...
    [main]
    modules = stats

    [stats]
    # Path to the database, default is MPDCRON_DIR/stats.db where MPDCRON_DIR is
    # ~/.mpdcron by default.
    dbpath = /path/to/database
    # Semi-colon delimited list of addresses to bind.
    # By default this module binds on any interface.
    bind_to_addresses = localhost;/home/alip/.mpdcron/stats.socket
    # Port to bind to, default port is 6601
    port = 6601
    # Default permissions for accessing the database.
    # This is a semi-colon delimited list of permissions.
    # Available permissions are:
    # select: Allows the client to do a select on the database.
    # update: Allows the client to do updates on the database.
    # The default is select;update
    default_permissions = select;update
    # Passwords to access the database.
    # This is a semi-colon delimited list of passwords in the form
    # password@permission.
    passwords = needvodka@update;needbeer@select

#### Creating/Updating the database
This module comes with a client called <tt>walrus</tt> to create or update the
statistics database. See **walrus --help** output for more information.

#### Interacting with the database
This module comes with a client called <tt>eugene</tt> which can be used to
interact with the statistics database. See **eugene --help** output for more
information.

#### Protocol
The procotol is very similar to mpd's protocol with minor differences.

##### General protocol syntax

##### Requests
If arguments contain spaces, they should be surrounded by double quotation marks.
All data between the client and the server is encoded in UTF-8.
The command syntax is:

    COMMAND [ARG...]

##### Responses
A command returns <tt>OK</tt> on completion or <tt>ACK</tt> some error on failure.  
These denote the end of command execution.

#### Command Reference
Expression arguments are direct [sqlite](http://www.sqlite.org/) statements.
See [http://www.sqlite.org/lang_expr.html](http://www.sqlite.org/lang_expr.html)
for expression syntax.

##### Querying the database
- list EXPRESSION
Lists the songs matching the given expression.
Returns:
<table border="1">
    <tr>
        <th>Name</th>
        <th>Type</th>
        <th>Value</th>
    </tr>
    <tr>
        <td>id</td>
        <td>Integer</td>
        <td>The database ID of the song</td>
    </tr>
    <tr>
        <td>file</td>
        <td>String</td>
        <td>URI of the song</td>
    </tr>
</table>
Example:

    alip@harikalardiyari> echo list '"id=83"' | netcat localhost 6601
    OK MPDCRON 0.2
    id: 83
    file: aerosmith/big_ones/11-the_other_side.ogg
    OK

- list\_album EXPRESSION
Lists the albums matching the given expression.
Returns:
<table border="1">
    <tr>
        <th>Name</th>
        <th>Type</th>
        <th>Explanation</th>
    </tr>
    <tr>
        <td>id</td>
        <td>Integer</td>
        <td>The database ID of the album</td>
    </tr>
    <tr>
        <td>Album</td>
        <td>String</td>
        <td>The name of the album</td>
    </tr>
    <tr>
        <td>Artist</td>
        <td>String</td>
        <td>The name of the artist</td>
    </tr>
</table>

Example:

    alip@harikalardiyari> echo list_album '"id=83"' |netcat localhost 6601
    OK MPDCRON 0.2
    id: 83
    Album: The Silent Enigma
    Artist: Anathema
    OK

- list\_artist EXPRESSION
Lists the artists matching the given expression.
Returns:
<table border="1">
    <tr>
        <th>Name</th>
        <th>Type</th>
        <th>Explanation</th>
    </tr>
    <tr>
        <td>id</td>
        <td>Integer</td>
        <td>The database ID of the artist</td>
    </tr>
    <tr>
        <td>Artist</td>
        <td>String</td>
        <td>The name of the artist</td>
    </tr>
</table>

Example:

    alip@harikalardiyari> echo list_artist '"id=83"' | netcat localhost 6601
    OK MPDCRON 0.2
    id: 83
    Artist: The Beatles
    OK

- list\_genre EXPRESSION
Lists the genres matching the given expression.
Returns:
<table border="1">
    <tr>
        <th>Name</th>
        <th>Type</th>
        <th>Explanation</th>
    </tr>
    <tr>
        <td>id</td>
        <td>Integer</td>
        <td>The database ID of the genre</td>
    </tr>
    <tr>
        <td>Genre</td>
        <td>String</td>
        <td>The name of the genre</td>
    </tr>
</table>

Example:

    alip@harikalardiyari> echo list_genre '"id=83"' |netcat localhost 6601
    OK MPDCRON 0.2
    id: 83
    Genre: Vocal
    OK


- listinfo EXPRESSION
Lists information of the songs matching the given expression.
Returns:
<table>
    <tr>
        <th>Name</th>
        <th>Type</th>
        <th>Explanation</th>
    </tr>
    <tr>
        <td>id</td>
        <td>Integer</td>
        <td>The database ID of the song</td>
    </tr>
    <tr>
        <td>file</td>
        <td>String</td>
        <td>The URI of the song</td>
    </tr>
    <tr>
        <td>Play Count</td>
        <td>Integer</td>
        <td>The play count of the song</td>
    </tr>
    <tr>
        <td>Love</td>
        <td>Integer</td>
        <td>The love count of the song</td>
    </tr>
    <tr>
        <td>Kill</td>
        <td>Integer</td>
        <td>The kill count of the song</td>
    </tr>
    <tr>
        <td>Rating</td>
        <td>Integer</td>
        <td>The rating count of the song</td>
    </tr>
</table>

Example:

    alip@harikalardiyari> echo listinfo '"id=102"' |netcat localhost 6601
    OK MPDCRON 0.2
    id: 102
    file: aerosmith/ultimate_hits/disc_1/15-love_in_an_elevator.ogg
    Play Count: 3
    Love: 1
    Kill: 0
    Rating: 10
    OK

- listinfo\_album EXPRESSION
Lists information of the albums matching the given expression.
Returns:
<table>
    <tr>
        <th>Name</th>
        <th>Type</th>
        <th>Explanation</th>
    </tr>
    <tr>
        <td>id</td>
        <td>Integer</td>
        <td>The database ID of the album</td>
    </tr>
    <tr>
        <td>Album</td>
        <td>String</td>
        <td>The name of the album</td>
    </tr>
    <tr>
        <td>Artist</td>
        <td>String</td>
        <td>The name of the artist</td>
    </tr>
    <tr>
        <td>Play Count</td>
        <td>Integer</td>
        <td>The play count of the album</td>
    </tr>
    <tr>
        <td>Love</td>
        <td>Integer</td>
        <td>The love count of the album</td>
    </tr>
    <tr>
        <td>Kill</td>
        <td>Integer</td>
        <td>The kill count of the album</td>
    </tr>
    <tr>
        <td>Rating</td>
        <td>Integer</td>
        <td>The rating count of the album</td>
    </tr>
</table>

Example:

    alip@harikalardiyari> echo listinfo_album "\"name='Animals'\"" |netcat localhost 6601
    OK MPDCRON 0.2
    id: 722
    Album: Animals
    Artist: Pink Floyd
    Play Count: 5
    Love: 1000
    Kill: 0
    Rating: 1000
    OK


- listinfo\_artist EXPRESSION
Lists information of the artists matching the given expression.
Returns:
<table>
    <tr>
        <th>Name</th>
        <th>Type</th>
        <th>Explanation</th>
    </tr>
    <tr>
        <td>id</td>
        <td>Integer</td>
        <td>The database ID of the artist</td>
    </tr>
    <tr>
        <td>Artist</td>
        <td>String</td>
        <td>The name of the artist</td>
    </tr>
    <tr>
        <td>Play Count</td>
        <td>Integer</td>
        <td>The play count of the artist</td>
    </tr>
    <tr>
        <td>Love</td>
        <td>Integer</td>
        <td>The love count of the artist</td>
    </tr>
    <tr>
        <td>Kill</td>
        <td>Integer</td>
        <td>The kill count of the artist</td>
    </tr>
    <tr>
        <td>Rating</td>
        <td>Integer</td>
        <td>The rating count of the artist</td>
    </tr>
</table>

Example:

    alip@harikalardiyari> echo listinfo_artist '"id=102"' |netcat localhost 6601
    OK MPDCRON 0.2
    id: 102
    Artist: Can
    Play Count: 10
    Love: 5
    Kill: 0
    Rating: 100
    OK

- listinfo\_genre EXPRESSION
Lists information of the genres matching the given expression.
Returns:
<table>
    <tr>
        <th>Name</th>
        <th>Type</th>
        <th>Explanation</th>
    </tr>
    <tr>
        <td>id</td>
        <td>Integer</td>
        <td>The database ID of the genre</td>
    </tr>
    <tr>
        <td>Genre</td>
        <td>String</td>
        <td>The name of the genre</td>
    </tr>
    <tr>
        <td>Play Count</td>
        <td>Integer</td>
        <td>The play count of the genre</td>
    </tr>
    <tr>
        <td>Love</td>
        <td>Integer</td>
        <td>The love count of the genre</td>
    </tr>
    <tr>
        <td>Kill</td>
        <td>Integer</td>
        <td>The kill count of the genre</td>
    </tr>
    <tr>
        <td>Rating</td>
        <td>Integer</td>
        <td>The rating count of the genre</td>
    </tr>
</table>

Example:

    alip@harikalardiyari> echo listinfo_genre "\"name like '%Psych%'\"" | netcat localhost 6601
    OK MPDCRON 0.2
    id: 89
    Genre: Psychadelic Trance
    Play Count: 0
    Love: 0
    Kill: 0
    Rating: 0
    OK

##### Updating the database
- hate EXPRESSION
Decreases the love count of songs matching the given expression by one.
Returns:
<table border="1">
    <tr>
        <th>Name</th>
        <th>Type</th>
        <th>Explanation</th>
    </tr>
    <tr>
        <td>changes</td>
        <td>Integer</td>
        <td>Number of modified entries</td>
    </tr>
</table>

Example:

    alip@harikalardiyari> echo hate "\"genre='Pop'\"" | netcat localhost 6601
    OK MPDCRON 0.2
    changes: 58
    OK

- hate\_album EXPRESSION
Decreases the love count of albums matching the given expression by one.
Returns: (same as <tt>hate</tt>)

- hate\_artist EXPRESSION
Decreases the love count of artists matching the given expression by one.
Returns: (same as <tt>hate</tt>)

- hate\_genre EXPRESSION
Decreases the love count of genres matching the given expression by one.
Returns: (same as <tt>hate</tt>)

- love EXPRESSION
Increases the love count of songs matching the given expression by one.
Returns: (same as <tt>hate</tt>)

- love\_album EXPRESSION
Increases the love count of albums matching the given expression by one.
Returns: (same as <tt>hate</tt>)

- love\_artist EXPRESSION
Increases the love count of artists matching the given expression by one.
Returns: (same as <tt>hate</tt>)

- love\_genre EXPRESSION
Increases the love count of genres matching the given expression by one.
Returns: (same as <tt>hate</tt>)

- kill EXPRESSION
Increases the kill count of songs matching the given expression by one.
Returns: (same as <tt>hate</tt>)

- kill\_album EXPRESSION
Increases the kill count of albums matching the given expression by one.
Returns: (same as <tt>hate</tt>)

- kill\_artist EXPRESSION
Increases the kill count of artists matching the given expression by one.
Returns: (same as <tt>hate</tt>)

- kill\_genre EXPRESSION
Increases the kill count of genres matching the given expression by one.
Returns: (same as <tt>hate</tt>)

- unkill EXPRESSION
Decreases the kill count of songs matching the given expression to zero.
Returns: (same as <tt>hate</tt>)

- unkill\_album EXPRESSION
Decreases the kill count of albums matching the given expression to zero.
Returns: (same as <tt>hate</tt>)

- unkill\_artist EXPRESSION
Decreases the kill count of artists matching the given expression to zero.
Returns: (same as <tt>hate</tt>)

- unkill\_genre EXPRESSION
Decreases the kill count of genres matching the given expression to zero.
Returns: (same as <tt>hate</tt>)

- rate EXPRESSION NUMBER
Adds the given number to the rating of songs matching the given expression.
Use a negative number to decrease the rating.
Returns: (same as <tt>hate</tt>)

- rate\_album EXPRESSION NUMBER
Adds the given number to the rating of albums matching the given expression.
Use a negative number to decrease the rating.
Returns: (same as <tt>hate</tt>)

- rate\_artist EXPRESSION NUMBER
Adds the given number to the rating of artists matching the given expression.
Use a negative number to decrease the rating.
Returns: (same as <tt>hate</tt>)

- rate\_genre EXPRESSION NUMBER
Adds the given number to the rating of genres matching the given expression.
Use a negative number to decrease the rating.
Returns: (same as <tt>hate</tt>)

<!-- vim: set tw=80 ft=mkd spell spelllang=en sw=4 sts=4 et : -->
