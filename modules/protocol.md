---
layout: default
title: Stats Protocol
---

The procotol is very similar to mpd's protocol with minor differences.

## Requests
If arguments contain spaces, they should be surrounded by double quotation marks.
All data between the client and the server is encoded in UTF-8.
The command syntax is:

    COMMAND [ARG...]

## Responses
A command returns <tt>OK</tt> on completion or <tt>ACK</tt> some error on failure.  
These denote the end of command execution.

### ACK Responses

Syntax:

    ACK [<int errorid>] {command} description

where:

- errorid:  
An integer indicating which kind of error occured.  

- description:  
A short explanation of the error. Maybe void.

## Command Reference
Expression arguments are direct [sqlite](http://www.sqlite.org/) statements.
See [http://www.sqlite.org/lang_expr.html](http://www.sqlite.org/lang_expr.html)
for the expression syntax.

### Querying the database
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
<table border="1">
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

### Updating the database

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
