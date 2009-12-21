#!/usr/bin/env ruby
# vim: set sw=2 sts=2 tw=100 et nowrap fenc=utf-8 :
# Copyright 2009 Ali Polatel <alip@exherbo.org>
# Based in part upon Scrobbler which is:
#   Copyright (c) John Nunemaker
# Distributed under the terms of the GNU General Public License v2

=begin
XXX XXX XXX XXX XXX XXX XXX XXX XXX  XXX
XXX THIS SCRIPT IS DEPRECATED!       XXX
XXX USE THE SCROBBLER MODULE INSTEAD XXX
XXX XXX XXX XXX XXX XXX XXX XXX XXX  XXX
mpdcron script to submit songs to libre.fm or last.fm
Execute this hook from your player hook.
Configuration file is MPDCRON_DIR/submit.yaml which is a yaml file that looks like:
### Submit configuration file
librefm:
  user: USERNAME
  password: PASSWORD
  journal: /path/to/librefm.yaml
lastfm:
  user: USERNAME
  password: PASSWORD
  journal: /path/to/lastfm.yaml
###
where MPDCRON_DIR is ~/.mpdcron by default.
PASSWORD can be specified either bare or in the form md5:MD5_HASH_OF_THE_PASSWORD
TODO:
Use MPD_STATUS_ELAPSED_TIME
=end

%w{digest/md5 net/http uri yaml}.each {|m| require m }

# Constants
LASTFM_AUTH_URL = 'http://post.audioscrobbler.com'
LASTFM_AUTH_VER = '1.2.1'
LIBREFM_AUTH_URL = 'http://turtle.libre.fm'
LIBREFM_AUTH_VER = LASTFM_AUTH_VER

MYNAME = File.basename($0, ".rb")

HTTP_PROXY = ENV['http_proxy']

MPDCRON_PACKAGE = ENV['MPDCRON_PACKAGE']
MPDCRON_VERSION = ENV['MPDCRON_VERSION']
MPDCRON_GITHEAD = ENV['MPDCRON_GITHEAD']

MCOPT_DAEMONIZE = ENV['MCOPT_DAEMONIZE']
MC_CALLS_PLAYER = ENV['MC_CALLS_PLAYER'] ? ENV['MC_CALLS_PLAYER'].to_i : 0

MPD_STATUS_STATE = ENV['MPD_STATUS_STATE']
MPD_STATUS_TOTAL_TIME = ENV['MPD_STATUS_TOTAL_TIME'] ? ENV['MPD_STATUS_TOTAL_TIME'].to_i : 0
MPD_SONG_ID = ENV['MPD_SONG_ID'] ? ENV['MPD_SONG_ID'].to_i : -1
MPD_SONG_URI = ENV['MPD_SONG_URI']
MPD_SONG_TAG_ALBUM = ENV['MPD_SONG_TAG_ALBUM']
MPD_SONG_TAG_ARTIST = ENV['MPD_SONG_TAG_ARTIST']
MPD_SONG_TAG_TITLE = ENV['MPD_SONG_TAG_TITLE']
MPD_SONG_TAG_TRACK = ENV['MPD_SONG_TAG_TRACK']
MPD_SONG_TAG_MUSICBRAINZ_TRACKID = ENV['MPD_SONG_TAG_MUSICBRAINZ_TRACKID']

# Errors
class BannedError < StandardError; end
class BadAuthError < StandardError; end
class RequestFailedError < StandardError; end
class BadTimeError < StandardError; end
class BadSessionError < StandardError; end
class NonImplementedError < StandardError; end

def log format, *args
  return if MCOPT_DAEMONIZE
  $stderr.print MYNAME + "[" + MC_CALLS_PLAYER.to_s + "]@" + Time.now.to_i.to_s + ": "
  $stderr.puts(sprintf(format, *args))
  $stderr.flush
end

class Connection
  def initialize base_url, args = {}
    @base_url = base_url
    @username = args[:username]
    @password = args[:password]

    if HTTP_PROXY
      proxy_url = URI.parse(HTTP_PROXY)
      @proxy_host = proxy_url.host if proxy_url and proxy_url.host
      @proxy_port = proxy_url.port if proxy_url and proxy_url.port
      @proxy_user, @proxy_pass = proxy_url.userinfo.split(/:/) if proxy_url and proxy_url.userinfo
      log("Successfully set up proxy host: %s, port: %d", @proxy_host, @proxy_port)
    end
  end

  def request resource, method = 'get', args = nil
    url = URI.join(@base_url, resource)

    headers = {
      'Accept-Charset' => 'UTF-8',
      'Content-type' => 'application/x-www-form-urlencoded',
      'Host' => url.host,
      'User-Agent' => MPDCRON_PACKAGE + '/' + MPDCRON_VERSION
    }

    case method
    when 'get'
      if args
        url.query = args.map { |k,v| "%s=%s" % [escape(k.to_s), escape(v.to_s)] }.join("&")
      end
      req = Net::HTTP::Get.new(url.request_uri, headers)
    when 'post'
      req = Net::HTTP::Post.new(url.request_uri, headers)
      req.set_form_data(args) if args
    else
      raise ArgumentError, "Invalid method"
    end

    if @username and @password
      req.basic_auth(@username, @password)
    end

    http = Net::HTTP::Proxy(@proxy_host, @proxy_port,
                            @proxy_user, @proxy_pass)
    res = http.start(url.host, url.port) { |conn| conn.request(req) }
    res.body
  end

  def get resource, args = nil; request(resource, method = 'get', args) end
  def post resource, args = nil; request(resource, method = 'post', args) end

  private
  def escape(str)
    URI.escape(str, Regexp.new("[^#{URI::PATTERN::UNRESERVED}]"))
  end
end

class Scrobbler
  attr_reader :auth_url, :auth_ver
  attr_reader :status, :session_id, :now_playing_url, :submission_url
  attr_accessor :user, :password, :cid, :cver, :journal

  def initialize args = {}
    @auth_url = args[:auth_url]
    @auth_ver = args[:auth_ver]
    @user = args[:user]
    @password = args[:password]

    raise ArgumentError, "Missing required argument: auth_url" if @auth_url.nil? or @auth_url.empty?
    raise ArgumentError, "Missing required argument: auth_ver" if @auth_ver.nil? or @auth_ver.empty?
    raise ArgumentError, "Missing required argument: user" if @user.nil? or @user.empty?
    raise ArgumentError, "Missing required argument: password" if @password.nil? or @password.empty?

    @journal = args[:journal]

    # TODO: Apply for a client ID
    @cid = 'mcn'
    @cver = MPDCRON_VERSION

    # Parse password
    if @password =~ /^md5:(.*)/
      @password = "$1"
    else
      @password = Digest::MD5.hexdigest(@password)
    end
  end

  def handshake!
    timestamp = Time.now.to_i.to_s
    token = Digest::MD5.hexdigest(@password + timestamp)

    query = { :hs => 'true',
              :p => @auth_ver,
              :c => @cid,
              :v => @cver,
              :u => @user,
              :t => timestamp,
              :a => token }

    connection = Connection.new(@auth_url)
    result = connection.get('/', query)
    @status = result.split(/\n/)[0]
    case @status
    when /OK/
      @session_id, @now_playing_url, @submission_url = result.split(/\n/)[1,3]
    when /BANNED/
      raise BannedError
    when /BADAUTH/
      raise BadAuthError
    when /FAILED/
      raise RequestFailedError, @status
    when /BADTIME/
      raise BadTimeError
    else
      raise RequestFailedError, @status
    end
  end

  def playing song
    artist = song.tag['artist']
    title = song.tag['title']
    album = song.tag['album']
    length = song.total_time
    track_number = song.tag['track']
    mb_track_id = song.tag['mb_track_id']

    raise ArgumentError, 'Session ID missing' if @session_id.nil? or @session_id.empty?
    raise ArgumentError, 'Now playing URL missing' if @now_playing_url.nil? or @now_playing_url.empty?
    return false if artist.nil? or artist.empty?
    return false if title.nil? or title.empty?
    return false if not length.to_s.empty? and length.to_i <= 30

    connection = Connection.new(@now_playing_url)

    query = { :s => @session_id,
              :a => artist,
              :t => title,
              :b => album,
              :l => length,
              :n => track_number,
              :m => mb_track_id }

    @status = connection.post('', query)
    case @status
    when /OK/
      return true
    when /BADSESSION/
      raise BadSessionError
    else
      raise RequestFailedError, @status
    end
  end

  def scrobble song
    artist = song.tag['artist']
    title = song.tag['title']
    time = song.start
    source = 'P'
    length = song.total_time
    album = song.tag['album']
    track_number = song.tag['track']
    mb_track_id = song.tag['mb_track_id']

    raise ArgumentError, 'Session ID missing' if @session_id.nil? or @session_id.empty?
    raise ArgumentError, 'Submission URL missing' if @submission_url.nil? or @submission_url.empty?
    return false if artist.nil? or artist.empty?
    return false if title.nil? or title.empty?

    # When to submit (from: http://www.audioscrobbler.net/development/protocol/)
    # ...
    # 2. The track must have been played for a duration of at least 240 seconds or  half the track's
    #    total length, whichever comes first. Skipping or pausing the track is irrelevant as long as
    #    the appropriate amount has been played.
    # 3. The total playback time for the track must be more than 30 seconds. Do not submit tracks
    #    shorter than this.
    # ...

    if not length.to_s.empty? and length.to_i <= 30 # 3.
      log "Not submitting song `%s' because it's shorter than 30 seconds", song.uri
      return false
    end

    duration = (Time.now - time).to_i
    if duration < 240 && duration < (length / 2)
      log "Not submitting song `%s' because neither it was played for 240 seconds,", song.uri
      log "nor half of it (%d seconds) has passed", (length / 2)
      return false
    end

    connection = Connection.new(@submission_url)
    query = { :s => @session_id,
      'a[0]' => artist,
      't[0]' => title,
      'i[0]' => time.utc.to_i,
      'o[0]' => source,
      'r[0]' => '',
      'l[0]' => length,
      'b[0]' => album,
      'n[0]' => track_number,
      'm[0]' => mb_track_id }

    @status = connection.post('', query)
    case @status
    when /OK/
      return true
    when /BADSESSION/
      raise BadSessionError
    when /FAILED/
      raise RequestFailedError, @status
    else
      raise RequestFailedError, @status
    end
  end

  def queue song
    unless @journal
      log "No journal defined in configuration file, skipping caching"
      begin
        return scrobble(song)
      rescue BadAuthError, BadSessionError, BannedError, RequestFailedError, BadTimeError => e
        log "Scrobbling song `%s' failed: %s", song.uri, e.message
        return false
      end
    end

    newcache = []
    cache = []
    if File.exists?(@journal)
      cache = open(@journal) {|f| YAML.load(f) }
      raise RuntimeError, "Failed to load journal `#{journal}'" unless cache
    end

    log "Caching song `%s' to journal `%s'", song.uri, @journal
    cache << song
    cache.each do |s|
      log "Scrobbling cached song `%s' loaded from journal `%s'", song.uri, @journal
      begin
        scrobble(s)
      rescue BadAuthError, BadSessionError, BannedError, RequestFailedError, BadTimeError => e
        log "Scrobbling song `%s' failed, caching song for later submission: %s", song.uri, e.message
        newcache << s
        next
      end
    end
    log "Saving updated journal to `%s'", @journal
    open(@journal, 'w') {|f| f.puts newcache.to_yaml}
    return true
  end
end

class Song
  attr_reader :id, :uri, :total_time, :tag
  attr_accessor :start

  def initialize
    @id = -1
    @uri = ''
    @total_time = 0
    @start = Time.now
    @tag = {}
  end

  def load!
    @id = MPD_SONG_ID
    @uri = MPD_SONG_URI
    @total_time = MPD_STATUS_TOTAL_TIME
    @tag = {'album' => MPD_SONG_TAG_ALBUM,
      'artist' => MPD_SONG_TAG_ARTIST,
      'title' => MPD_SONG_TAG_TITLE,
      'track' => MPD_SONG_TAG_TRACK,
      'mb_track_id' => MPD_SONG_TAG_MUSICBRAINZ_TRACKID}
  end
end

class Submit
  attr_reader :config
  attr_accessor :lastfm, :librefm, :settings

  def initialize
    @confpath = "./submit.yaml"
    @path = "./.settings.yaml"
    @settings = {'pause' => false, 'song' => Song.new}
  end

  def load!
    @config = open(@confpath) {|f| YAML.load(f)}
    raise RuntimeError, "Error loading YAML from `#{@confpath}'" unless @config
    raise RuntimeError, "Neither lastfm nor librefm section defined in `#{@confpath}'" unless @config['lastfm'] or @config['librefm']

    if @config['librefm']
      raise RuntimeError, "librefm.user not defined in `#{confpath}'" unless @config['librefm']['user']
      raise RuntimeError, "librefm.password not defined in `#{confpath}'" unless @config['librefm']['password']
      @librefm = Scrobbler.new(:auth_url => LIBREFM_AUTH_URL,
                               :auth_ver => LIBREFM_AUTH_VER,
                               :user => @config['librefm']['user'],
                               :password => @config['librefm']['password'],
                               :journal => @config['librefm']['journal'])
    else
      @librefm = nil
    end

    if @config['lastfm']
      raise RuntimeError, "lastfm.user not defined in `#{confpath}'" unless @config['lastfm']['user']
      raise RuntimeError, "lastfm.password not defined in `#{confpath}'" unless @config['lastfm']['password']
      @lastfm = Scrobbler.new(:auth_url => LASTFM_AUTH_URL,
                              :auth_ver => LASTFM_AUTH_VER,
                              :user => @config['lastfm']['user'],
                              :password => @config['lastfm']['password'],
                              :journal => @config['lastfm']['journal'])
    else
      @lastfm = nil
    end

    if File.exists?(@path)
      @settings = open(@path) { |f| YAML.load(f) }
      File.delete(@path)
    end
  end

  def save
    open(@path, 'w') {|f| f.puts @settings.to_yaml}
  end

  def clear
    File.delete(@path) if File.exists?(@path)
  end
end

s = Submit.new
if MC_CALLS_PLAYER == 1
  log "First run, removing cruft..."
  s.clear
end
s.load!

csong = Song.new
csong.load!

case MPD_STATUS_STATE
when 'play'
  # Check if a paused song is continued.
  if s.settings['pause']
    if csong.id == s.settings['song'].id
      log "Paused song `%s' continued.", csong.uri
      s.settings['pause'] = false
      s.save
      exit 0
    end
  end

  if s.settings['song'].id < 0
    log "Initial song playing `%s', saving to cache", csong.uri
    csong.start = Time.now
    s.settings['song'] = csong
    s.save

    ###################################
    ### Send now playing notifications#
    ###################################
    if s.librefm
      log "Sending now playing notification for song `%s' to Libre.fm", csong.uri
      begin
        s.librefm.handshake!
        if s.librefm.playing(csong) then
          log "Successfully submitted song `%s'", csong.uri
        end
      rescue BadAuthError, BadSessionError, BannedError, RequestFailedError, BadTimeError => e
        log "Failed to submit song: %s", e.message
      end
    end
    if s.lastfm
      log "Sending now playing notification for song `%s' to Last.fm", csong.uri
      begin
        s.lastfm.handshake!
        if s.lastfm.playing(csong) then
          log "Successfully submitted song `%s' to Last.fm", csong.uri
        end
      rescue BadAuthError, BadSessionError, BannedError, RequestFailedError, BadTimeError => e
        log "Failed to submit song to Last.fm: %s", e.message
      end
    end
    ###################################
  elsif s.settings['song'].id != csong.id
    csong.start = Time.now
    psong = s.settings['song']
    s.settings['song'] = csong
    s.save

    ###################################
    ### Scrobble previous song ########
    ###################################
    if s.librefm
      log "Scrobbling previous song `%s' to Libre.fm", psong.uri
      begin
        s.librefm.handshake!
      rescue BadAuthError, BadSessionError, BannedError, RequestFailedError, BadTimeError => e
        log "Handshake with Libre.fm failed: %s", e.message
      end
        s.librefm.queue psong
    end
    if s.lastfm
      log "Submitting previous song `%s' to Last.fm", psong
      begin
        s.lastfm.handshake!
      rescue BadAuthError, BadSessionError, BannedError, RequestFailedError, BadTimeError => e
        log "Handshake with Last.fm failed: %s", e.message
      end
        s.lastfm.queue psong
    end
    ###################################

    ###################################
    ### Send now playing notifications#
    ###################################
    if s.librefm
      log "Sending now playing notification for song `%s' to Libre.fm", csong.uri
      begin
        s.librefm.handshake!
        if s.librefm.playing(csong) then
          log "Successfully submitted song `%s' to Libre.fm", csong.uri
        end
      rescue BadAuthError, BadSessionError, BannedError, RequestFailedError, BadTimeError => e
        log "Failed to submit song to Libre.fm: %s", e.message
      end
    end
    if s.lastfm
      log "Sending now playing notification for song `%s' to Last.fm", csong.uri
      begin
        s.lastfm.handshake!
        if s.lastfm.playing(csong) then
          log "Successfully submitted song `%s' to Last.fm", csong.uri
        end
      rescue BadAuthError, BadSessionError, BannedError, RequestFailedError, BadTimeError => e
        log "Failed to submit song to Last.fm: %s", e.message
      end
    end
    ###################################
  else
    log "Seek called on the song `%s'.", csong.uri
    s.save
  end
when 'pause'
  log "Song `%s' paused.", csong.uri
  s.settings['pause'] = true
  s.settings['song'] = csong
  s.save
else
  log "Stopped."
  s.settings['pause'] = false
  s.settings['song'] = csong
  s.save
end
