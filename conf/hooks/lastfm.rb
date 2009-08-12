#!/usr/bin/env ruby
# vim: set sw=2 sts=2 tw=100 et nowrap fenc=utf-8 :
# Copyright 2009 Ali Polatel <polatel@gmail.com>
# Distributed under the terms of the GNU General Public License v2

=begin
MpdHooker hook to submit now playing songs to last.fm.
Requires scrobbler.
Copy this file to MPD_HOOKER_DIR/hooks/song where MPD_HOOKER_DIR is ~/.mpdhooker by default.
Make sure to make it executable.
Configuration file is MPD_HOOKER_DIR/lastfm.yaml which is a yaml file that looks like:
### Lastfm configuration file
verbose: false
lastfm:
  user: USERNAME
  password: PASSWORD
###

TODO: Implement caching of songs that couldn't be submitted.
=end

require 'yaml'
require 'rubygems'
require 'scrobbler'
include Scrobbler

MYNAME = File.basename($0, ".rb")
if MYNAME != "song"
  $stderr.puts "Unknown hook name: `#{MYNAME}'"
  $stderr.puts "This script needs to be named song"
  exit 1
end

# Parse configuration
$homedir = ENV["MPD_HOOKER_DIR"]
if $homedir.nil?
  raise RunTimeError, "Neither MPD_HOOKER_DIR nor HOME set!" unless ENV["HOME"]
  $homedir = File.join(ENV["HOME"], ".mpdhooker")
end
$confpath = File.join($homedir, "lastfm.yaml")
$config = open($confpath) {|f| YAML.load(f) }

raise RuntimeError, "Error loading YAML from `#{$confpath}'" unless $config
raise RuntimeError, "lastfm section not defined in `#{$confpath}'" unless $config['lastfm']

$user = $config['lastfm']['user']
$password = $config['lastfm']['password']
raise RuntimeError, "lastfm.user not defined in `#{$confpath}'" unless $user
raise RuntimeError, "lastfm.password not defined in `#{$confpath}'" unless $password

def log(format, *args)
  return unless $config['verbose']
  $stderr.print "lastfm@" << Time.now.to_i.to_s << ": "
  $stderr.puts(sprintf(format, *args))
  $stderr.flush
end

def authenticate(user, password)
  log("Logging in to last.fm")
  auth = SimpleAuth.new(:user => $user, :password => $password)
  begin
    auth.handshake!
  rescue BadAuthError => e
    log("Authentication failed: %s", e.message)
    return nil
  end
  log("Authentication status: #{auth.status}")
  log("Session ID: #{auth.session_id}")
  log("Now playing url: #{auth.now_playing_url}")
  log("Submission url: #{auth.submission_url}")
  return auth
end

auth = authenticate($user, $password)
exit 0 unless auth
log("Submitting now playing song `#{ENV['MPD_SONG_ARTIST']} - #{ENV['MPD_SONG_TITLE']}' to last.fm")
playing = Playing.new(:session_id => auth.session_id,
                      :now_playing_url => auth.now_playing_url,
                      :artist => ENV['MPD_SONG_ARTIST'],
                      :track => ENV['MPD_SONG_TITLE'],
                      :album => ENV['MPD_SONG_ALBUM'],
                      :length => ENV['MPD_TOTAL_TIME'].to_i,
                      :track_number => ENV['MPD_SONG_TRACK'].to_i)
playing.submit!
log("Playing submission status: #{playing.status}")

=begin
We should take some rules into account when submitting songs to last.fm
They are:
* The track must be submitted once it has finished playing. Whether it has finished playing
  naturally or has been manually stopped by the user is irrelevant.
* The track must have been played for a duration of at least 240 seconds or half the track's total
  length, whichever comes first. Skipping or pausing the track is irrelevant as long as the
  appropriate amount has been played.
* The total playback time for the track must be more than 30 seconds. Do not submit tracks shorter
  than this.
=end

log("Submitting song `#{ENV['MPD_SONG_ARTIST_OLD']} - #{ENV['MPD_SONG_TITLE_OLD']}' to last.fm")

$elapsed = ENV["MPD_ELAPSED_OLD"].to_f
$total = ENV["MPD_TOTAL_TIME_OLD"].to_f
$perc = $elapsed / $total

if $total < 30
  log("Not submitting song because it was shorter than 30 seconds, (%s seconds)", $total)
  exit 0
elsif $elapsed < 240 and $perc < 0.5
  log("Not submitting song because either 240 seconds or half of it hasn't passed")
  exit 0
end

scrobble = Scrobble.new(:session_id => auth.session_id,
                        :submission_url => auth.submission_url,
                        :artist => ENV['MPD_SONG_ARTIST_OLD'],
                        :track => ENV['MPD_SONG_TITLE_OLD'],
                        :album => ENV['MPD_SONG_ALBUM_OLD'],
                        :length => ENV['MPD_TOTAL_TIME_OLD'].to_i,
                        :track_number => ENV['MPD_SONG_TRACK_OLD'].to_i,
                        :time => Time.now - $total)
scrobble.submit!
log("Scrobble submission status: #{scrobble.status}")

