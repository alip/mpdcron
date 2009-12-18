#!/usr/bin/env ruby
# vim: set sw=2 sts=2 tw=100 et nowrap fenc=utf-8 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

# Dump playlists
# Playlists are listed using environment variables like:
# MPD_PLAYLIST_%d_PATH = /path/to/playlist
# MPD_PLAYLIST_%d_LAST_MODIFIED = PLAYLIST_LAST_MODIFIED_DATE

puts "=== MPD PLAYLISTS ==="
i = 1
loop do
  envp  = "MPD_PLAYLIST_#{i}_PATH"
  envlm = "MPD_PLAYLIST_#{i}_LAST_MODIFIED"
  break unless ENV[envp]
  puts "playlist[#{i}].path = #{ENV[envp]}"
  puts "playlist[#{i}].last_modified = #{ENV[envlm]}"
  i += 1
end
puts "====================="
