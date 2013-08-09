/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009, 2010 Ali Polatel <alip@exherbo.org>
 *
 * This file is part of the mpdcron mpd client. mpdcron is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * mpdcron is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "cron-defs.h"

#include <time.h>

#include <glib.h>
#include <mpd/client.h>

static const char *
env_strstate(enum mpd_state state)
{
	const char *strstate = NULL;
	switch (state) {
		case MPD_STATE_UNKNOWN:
			strstate = "unknown";
			break;
		case MPD_STATE_STOP:
			strstate = "stop";
			break;
		case MPD_STATE_PLAY:
			strstate = "play";
			break;
		case MPD_STATE_PAUSE:
			strstate = "pause";
			break;
		default:
			g_assert_not_reached();
			break;
	}
	return strstate;
}

static void
env_export_status(struct mpd_status *status)
{
	char *envstr;
	const struct mpd_audio_format *fmt;

	envstr = g_strdup_printf("%d", mpd_status_get_volume(status));
	g_setenv("MPD_STATUS_VOLUME", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%d", mpd_status_get_repeat(status));
	g_setenv("MPD_STATUS_REPEAT", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%d", mpd_status_get_random(status));
	g_setenv("MPD_STATUS_RANDOM", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%d", mpd_status_get_single(status));
	g_setenv("MPD_STATUS_SINGLE", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%d", mpd_status_get_consume(status));
	g_setenv("MPD_STATUS_CONSUME", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%d", mpd_status_get_queue_length(status));
	g_setenv("MPD_STATUS_QUEUE_LENGTH", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%d", mpd_status_get_queue_version(status));
	g_setenv("MPD_STATUS_QUEUE_VERSION", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%d", mpd_status_get_crossfade(status));
	g_setenv("MPD_STATUS_CROSSFADE", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%d", mpd_status_get_song_pos(status));
	g_setenv("MPD_STATUS_SONG_POS", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%d", mpd_status_get_song_id(status));
	g_setenv("MPD_STATUS_SONG_ID", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%u", mpd_status_get_elapsed_time(status));
	g_setenv("MPD_STATUS_ELAPSED_TIME", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%u", mpd_status_get_elapsed_ms(status));
	g_setenv("MPD_STATUS_ELAPSED_MS", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%u", mpd_status_get_total_time(status));
	g_setenv("MPD_STATUS_TOTAL_TIME", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%u", mpd_status_get_kbit_rate(status));
	g_setenv("MPD_STATUS_KBIT_RATE", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%u", mpd_status_get_update_id(status));
	g_setenv("MPD_STATUS_UPDATE_ID", envstr, 1);
	g_free(envstr);

	g_setenv("MPD_STATUS_STATE", env_strstate(mpd_status_get_state(status)), 1);

	if ((fmt = mpd_status_get_audio_format(status)) == NULL)
		g_setenv("MPD_STATUS_AUDIO_FORMAT", "0", 1);
	else {
		g_setenv("MPD_STATUS_AUDIO_FORMAT", "1", 1);

		envstr = g_strdup_printf("%u", fmt->sample_rate);
		g_setenv("MPD_STATUS_AUDIO_FORMAT_SAMPLE_RATE", envstr, 1);
		g_free(envstr);

		envstr = g_strdup_printf("%u", fmt->bits);
		g_setenv("MPD_STATUS_AUDIO_FORMAT_BITS", envstr, 1);
		g_free(envstr);

		envstr = g_strdup_printf("%u", fmt->channels);
		g_setenv("MPD_STATUS_AUDIO_FORMAT_CHANNELS", envstr, 1);
		g_free(envstr);
	}
}

static void
env_export_song(struct mpd_song *song)
{
	const char *tag;
	char *envstr;
	time_t t;
	struct tm tm;
	char date[DEFAULT_DATE_FORMAT_SIZE] = { 0 };

	g_setenv("MPD_SONG_URI", mpd_song_get_uri(song), 1);

	t = mpd_song_get_last_modified(song);
	if (localtime_r(&t, &tm)) {
		strftime(date, DEFAULT_DATE_FORMAT_SIZE, DEFAULT_DATE_FORMAT, &tm);
		g_setenv("MPD_SONG_LAST_MODIFIED", date, 1);
	}

	envstr = g_strdup_printf("%u", mpd_song_get_duration(song));
	g_setenv("MPD_SONG_DURATION", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%u", mpd_song_get_pos(song));
	g_setenv("MPD_SONG_POS", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%u", mpd_song_get_id(song));
	g_setenv("MPD_SONG_ID", envstr, 1);
	g_free(envstr);

	/* Export tags. FIXME: For now we just export the first tag value to
	 * the environment.
	 */
	if ((tag = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_ARTIST", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_ALBUM", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_ALBUM_ARTIST", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_TITLE, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_TITLE", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_TRACK, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_TRACK", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_NAME, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_NAME", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_GENRE, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_GENRE", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_DATE, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_DATE", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_COMPOSER", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_PERFORMER, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_PERFORMER", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_COMMENT, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_COMMENT", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_DISC, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_DISC", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ARTISTID, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_MUSICBRAINZ_ARTISTID", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMID, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_MUSICBRAINZ_ALBUMID", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMARTISTID, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_MUSICBRAINZ_ALBUMARTISTID", tag, 1);
	if ((tag = mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_TRACKID, 0)) != NULL)
		g_setenv("MPD_SONG_TAG_MUSICBRAINZ_TRACKID", tag, 1);
}

void
env_stats(struct mpd_stats *stats)
{
	char *envstr;
	time_t t;
	struct tm tm;
	char date[DEFAULT_DATE_FORMAT_SIZE] = { 0 };

	t = mpd_stats_get_db_update_time(stats);
	if (localtime_r(&t, &tm)) {
		strftime(date, DEFAULT_DATE_FORMAT_SIZE, DEFAULT_DATE_FORMAT, &tm);
		g_setenv("MPD_DATABASE_UPDATE_TIME", date, 1);
	}

	envstr = g_strdup_printf("%d", mpd_stats_get_number_of_artists(stats));
	g_setenv("MPD_DATABASE_ARTISTS", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%d", mpd_stats_get_number_of_albums(stats));
	g_setenv("MPD_DATABASE_ALBUMS", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%d", mpd_stats_get_number_of_songs(stats));
	g_setenv("MPD_DATABASE_SONGS", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%lu", mpd_stats_get_play_time(stats));
	g_setenv("MPD_DATABASE_PLAY_TIME", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%lu", mpd_stats_get_uptime(stats));
	g_setenv("MPD_DATABASE_UPTIME", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%lu", mpd_stats_get_db_play_time(stats));
	g_setenv("MPD_DATABASE_DB_PLAY_TIME", envstr, 1);
	g_free(envstr);
}

void
env_status(struct mpd_status *status)
{
	env_export_status(status);
}

void
env_status_currentsong(struct mpd_song *song, struct mpd_status *status)
{
	env_export_status(status);
	if (song)
		env_export_song(song);
}

void
env_clearenv(void)
{
	/* status */
	g_unsetenv("MPD_STATUS_VOLUME");
	g_unsetenv("MPD_STATUS_REPEAT");
	g_unsetenv("MPD_STATUS_RENDOM");
	g_unsetenv("MPD_STATUS_SINGLE");
	g_unsetenv("MPD_STATUS_CONSUME");
	g_unsetenv("MPD_STATUS_QUEUE_LENGTH");
	g_unsetenv("MPD_STATUS_QUEUE_VERSION");
	g_unsetenv("MPD_STATUS_CROSSFADE");
	g_unsetenv("MPD_STATUS_SONG_POS");
	g_unsetenv("MPD_STATUS_SONG_ID");
	g_unsetenv("MPD_STATUS_ELAPSED_TIME");
	g_unsetenv("MPD_STATUS_ELAPSED_MS");
	g_unsetenv("MPD_STATUS_TOTAL_TIME");
	g_unsetenv("MPD_STATUS_KBIT_RATE");
	g_unsetenv("MPD_STATUS_UPDATE_ID");
	g_unsetenv("MPD_STATUS_STATE");
	g_unsetenv("MPD_STATUS_AUDIO_FORMAT");
	g_unsetenv("MPD_STATUS_AUDIO_FORMAT_SAMPLE_RATE");
	g_unsetenv("MPD_STATUS_AUDIO_FORMAT_BITS");
	g_unsetenv("MPD_STATUS_AUDIO_FORMAT_CHANNELS");

	/* song */
	g_unsetenv("MPD_SONG_URI");
	g_unsetenv("MPD_SONG_TAG_ARTIST");
	g_unsetenv("MPD_SONG_TAG_ALBUM");
	g_unsetenv("MPD_SONG_TAG_ALBUM_ARTIST");
	g_unsetenv("MPD_SONG_TAG_TITLE");
	g_unsetenv("MPD_SONG_TAG_TRACK");
	g_unsetenv("MPD_SONG_TAG_NAME");
	g_unsetenv("MPD_SONG_TAG_GENRE");
	g_unsetenv("MPD_SONG_TAG_DATE");
	g_unsetenv("MPD_SONG_TAG_COMPOSER");
	g_unsetenv("MPD_SONG_TAG_PERFORMER");
	g_unsetenv("MPD_SONG_TAG_COMMENT");
	g_unsetenv("MPD_SONG_TAG_DISC");
	g_unsetenv("MPD_SONG_TAG_MUSICBRAINZ_ARTISTID");
	g_unsetenv("MPD_SONG_TAG_MUSICBRAINZ_ALBUMID");
	g_unsetenv("MPD_SONG_TAG_MUSICBRAINZ_ALBUMARTISTID");
	g_unsetenv("MPD_SONG_TAG_MUSICBRAINZ_TRACKID");
	g_unsetenv("MPD_SONG_LAST_MODIFIED");
	g_unsetenv("MPD_SONG_POS");
	g_unsetenv("MPD_SONG_ID");

	/* stats */
	g_unsetenv("MPD_DATABASE_UPDATE_TIME");
	g_unsetenv("MPD_DATABASE_ARTISTS");
	g_unsetenv("MPD_DATABASE_ALBUMS");
	g_unsetenv("MPD_DATABASE_SONGS");
	g_unsetenv("MPD_DATABASE_PLAY_TIME");
	g_unsetenv("MPD_DATABASE_UPTIME");
	g_unsetenv("MPD_DATABASE_DB_PLAY_TIME");
}
