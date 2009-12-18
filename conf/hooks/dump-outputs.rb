#!/usr/bin/env ruby
# vim: set sw=2 sts=2 tw=100 et nowrap fenc=utf-8 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Distributed under the terms of the GNU General Public License v2

# Dump outputs
puts "=== MPD OUTPUTS ==="
i = 1
loop do
  env_output = "MPD_OUTPUT_ID_#{i}"
  break unless ENV[env_output]
  puts "output[#{i}].id = #{ENV[env_output]}"
  puts "output[#{i}].enabled = #{ENV["MPD_OUTPUT_ID_#{i}_ENABLED"]}"
  i += 1
end
puts "==================="
