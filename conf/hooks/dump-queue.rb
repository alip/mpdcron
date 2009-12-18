#!/usr/bin/env ruby
# vim: set sw=2 sts=2 tw=100 et nowrap fenc=utf-8 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

# Dump queue
puts "=== MPD QUEUE ==="
i = 1
loop do
  env_uri = "MPD_SONG_#{i}_URI"
  break unless ENV[env_uri]
  puts "song[#{i}].uri = #{ENV[env_uri]}"
  puts "song[#{i}].last_modified = #{ENV["MPD_SONG_#{i}_LAST_MODIFIED]"]}"
  puts "song[#{i}].duration = #{ENV["MPD_SONG_#{i}_DURATION"]}"
  puts "song[#{i}].pos = #{ENV["MPD_SONG_#{i}_POS"]}"
  puts "song[#{i}].id = #{ENV["MPD_SONG_#{i}_ID"]}"
  puts "song[#{i}].tag.artist = #{ENV["MPD_SONG_#{i}_TAG_ARTIST"]}"
  puts "song[#{i}].tag.album = #{ENV["MPD_SONG_#{i}_TAG_ALBUM"]}"
  puts "song[#{i}].tag.album_artist = #{ENV["MPD_SONG_#{i}_TAG_ALBUM_ARTIST"]}"
  puts "song[#{i}].tag.title = #{ENV["MPD_SONG_#{i}_TAG_TITLE"]}"
  puts "song[#{i}].tag.track = #{ENV["MPD_SONG_#{i}_TAG_TRACK"]}"
  puts "song[#{i}].tag.name = #{ENV["MPD_SONG_#{i}_TAG_NAME"]}"
  puts "song[#{i}].tag.genre = #{ENV["MPD_SONG_#{i}_TAG_GENRE"]}"
  puts "song[#{i}].tag.date = #{ENV["MPD_SONG_#{i}_TAG_DATE"]}"
  puts "song[#{i}].tag.composer = #{ENV["MPD_SONG_#{i}_TAG_COMPOSER"]}"
  puts "song[#{i}].tag.comment = #{ENV["MPD_SONG_#{i}_TAG_COMMENT"]}"
  puts "song[#{i}].tag.disc = #{ENV["MPD_SONG_#{i}_TAG_DISC"]}"
  puts "song[#{i}].tag.musicbrainz.artistid = #{ENV["MPD_SONG_#{i}_TAG_MUSICBRAINZ_ARTISTID"]}"
  puts "song[#{i}].tag.musicbrainz.albumid = #{ENV["MPD_SONG_#{i}_TAG_MUSICBRAINZ_ALBUMID"]}"
  puts "song[#{i}].tag.musicbrainz.albumartistid = #{ENV["MPD_SONG_#{i}_TAG_MUSICBRAINZ_ALBUMARTISTID"]}"
  puts "song[#{i}].tag.musicbrainz.trackid = #{ENV["MPD_SONG_#{i}_TAG_MUSICBRAINZ_TRACKID"]}"
  i += 1
end
puts "================="
