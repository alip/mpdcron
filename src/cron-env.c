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
env_export_song(struct mpd_song *song, int envid)
{
	const char *tag;
	char *envname, *envstr;
	time_t t;
	char date[DEFAULT_DATE_FORMAT_SIZE] = { 0 };

	envname = (envid == 0) ? g_strdup("MPD_SONG_URI")
			       : g_strdup_printf("MPD_SONG_%d_URI", envid);
	g_setenv(envname, mpd_song_get_uri(song), 1);
	g_free(envname);

	t = mpd_song_get_last_modified(song);
	strftime(date, DEFAULT_DATE_FORMAT_SIZE, DEFAULT_DATE_FORMAT, localtime(&t));
	envname = (envid == 0) ? g_strdup("MPD_SONG_LAST_MODIFIED")
		: g_strdup_printf("MPD_SONG_%d_LAST_MODIFIED", envid);
	g_setenv(envname, date, 1);
	g_free(envname);

	envname = (envid == 0) ? g_strdup("MPD_SONG_DURATION")
		: g_strdup_printf("MPD_SONG_%d_DURATION", envid);
	envstr = g_strdup_printf("%u", mpd_song_get_duration(song));
	g_setenv(envname, envstr, 1);
	g_free(envname);
	g_free(envstr);

	envname = (envid == 0) ? g_strdup("MPD_SONG_POS")
			       : g_strdup_printf("MPD_SONG_%d_POS", envid);
	envstr = g_strdup_printf("%u", mpd_song_get_pos(song));
	g_setenv(envname, envstr, 1);
	g_free(envname);
	g_free(envstr);

	envname = (envid == 0) ? g_strdup("MPD_SONG_ID")
		: g_strdup_printf("MPD_SONG_%d_ID", envid);
	envstr = g_strdup_printf("%u", mpd_song_get_id(song));
	g_setenv(envname, envstr, 1);
	g_free(envname);
	g_free(envstr);

	/* Export tags. FIXME: For now we just export the first tag value to
	 * the environment.
	 */
	if ((tag = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_ARTIST")
			: g_strdup_printf("MPD_SONG_%d_TAG_ARTIST", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_ALBUM")
			: g_strdup_printf("MPD_SONG_%d_TAG_ALBUM", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_ALBUM_ARTIST, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_ALBUM_ARTIST")
			: g_strdup_printf("MPD_SONG_%d_TAG_ALBUM_ARTIST", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_TITLE, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_TITLE")
			: g_strdup_printf("MPD_SONG_%d_TAG_TITLE", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_TRACK, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_TRACK")
			: g_strdup_printf("MPD_SONG_%d_TAG_TRACK", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_NAME, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_NAME")
			: g_strdup_printf("MPD_SONG_%d_TAG_NAME", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_GENRE, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_GENRE")
			: g_strdup_printf("MPD_SONG_%d_TAG_GENRE", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_DATE, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_DATE")
			: g_strdup_printf("MPD_SONG_%d_TAG_DATE", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_COMPOSER")
			: g_strdup_printf("MPD_SONG_%d_TAG_COMPOSER", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_PERFORMER, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_PERFORMER")
			: g_strdup_printf("MPD_SONG_%d_TAG_PERFORMER", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_COMMENT, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_COMMENT")
			: g_strdup_printf("MPD_SONG_%d_TAG_COMMENT", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_DISC, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_DISC")
			: g_strdup_printf("MPD_SONG_%d_TAG_DISC", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ARTISTID, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_MUSICBRAINZ_ARTISTID")
			: g_strdup_printf("MPD_SONG_%d_TAG_MUSICBRAINZ_ARTISTID", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMID, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_MUSICBRAINZ_ALBUMID")
			: g_strdup_printf("MPD_SONG_%d_TAG_MUSICBRAINZ_ALBUMID", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMARTISTID, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_MUSICBRAINZ_ALBUMARTISTID")
			: g_strdup_printf("MPD_SONG_%d_TAG_MUSICBRAINZ_ALBUMARTISTID", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
	if ((tag = mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_TRACKID, 0)) != NULL) {
		envname = (envid == 0) ? g_strdup("MPD_SONG_TAG_MUSICBRAINZ_TRACKID")
			: g_strdup_printf("MPD_SONG_%d_TAG_MUSICBRAINZ_TRACKID", envid);
		g_setenv(envname, tag, 1);
		g_free(envname);
	}
}

int
env_list_all_meta(struct mpd_connection *conn)
{
	int i;
	char *envname;
	char date[DEFAULT_DATE_FORMAT_SIZE] = { 0 };
	time_t t;
	const struct mpd_playlist *pl;
	struct mpd_entity *entity;

	g_assert(conn != NULL);

	g_debug("Sending list_all_meta command to Mpd server");
	if (!mpd_send_list_meta(conn, NULL))
		return -1;

	i = 0;
	while ((entity = mpd_recv_entity(conn)) != NULL) {
		if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_PLAYLIST) {
			pl = mpd_entity_get_playlist(entity);

			envname = g_strdup_printf("MPD_PLAYLIST_%d_PATH", ++i);
			g_setenv(envname, mpd_playlist_get_path(pl), 1);
			g_free(envname);

			t = mpd_playlist_get_last_modified(pl);
			strftime(date, DEFAULT_DATE_FORMAT_SIZE, DEFAULT_DATE_FORMAT, localtime(&t));
			envname = g_strdup_printf("MPD_PLAYLIST_%d_LAST_MODIFIED", i);
			g_setenv(envname, date, 1);
			g_free(envname);
		}
		mpd_entity_free(entity);
	}

	return mpd_response_finish(conn) ? 0 : -1;
}

int
env_list_queue_meta(struct mpd_connection *conn)
{
	int i;
	struct mpd_song *song;

	g_assert(conn != NULL);

	g_debug("Sending list_queue_meta command to Mpd server");
	if (!mpd_send_list_queue_meta(conn))
		return -1;

	i = 0;
	while ((song = mpd_recv_song(conn)) != NULL) {
		env_export_song(song, ++i);
		mpd_song_free(song);
	}

	return mpd_response_finish(conn) ? 0 : -1;
}

int
env_stats(struct mpd_connection *conn, struct mpd_stats **stats)
{
	char *envstr;
	time_t t;
	char date[DEFAULT_DATE_FORMAT_SIZE] = { 0 };

	g_assert(conn != NULL);

	g_debug("Sending stats command to Mpd server");
	if ((*stats = mpd_run_stats(conn)) == NULL)
		return -1;

	t = mpd_stats_get_db_update_time(*stats);
	strftime(date, DEFAULT_DATE_FORMAT_SIZE, DEFAULT_DATE_FORMAT, localtime(&t));
	g_setenv("MPD_DATABASE_UPDATE_TIME", date, 1);

	envstr = g_strdup_printf("%d", mpd_stats_get_number_of_artists(*stats));
	g_setenv("MPD_DATABASE_ARTISTS", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%d", mpd_stats_get_number_of_albums(*stats));
	g_setenv("MPD_DATABASE_ALBUMS", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%d", mpd_stats_get_number_of_songs(*stats));
	g_setenv("MPD_DATABASE_SONGS", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%lu", mpd_stats_get_play_time(*stats));
	g_setenv("MPD_DATABASE_PLAY_TIME", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%lu", mpd_stats_get_uptime(*stats));
	g_setenv("MPD_DATABASE_UPTIME", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%lu", mpd_stats_get_db_play_time(*stats));
	g_setenv("MPD_DATABASE_DB_PLAY_TIME", envstr, 1);
	g_free(envstr);

	envstr = NULL;
	return 0;
}

int
env_status(struct mpd_connection *conn, struct mpd_status **status)
{
	g_assert(conn != NULL);

	g_debug("Sending status command to Mpd server");
	if ((*status = mpd_run_status(conn)) == NULL)
		return -1;

	env_export_status(*status);
	return 0;
}

int
env_outputs(struct mpd_connection *conn)
{
	int id;
	const char *name;
	char *envname;
	struct mpd_output *output;

	g_assert(conn != NULL);

	g_debug("Sending outputs command to Mpd server");
	if (!mpd_send_outputs(conn))
		return -1;

	while ((output = mpd_recv_output(conn)) != NULL) {
		id = mpd_output_get_id(output) + 1;
		name = mpd_output_get_name(output);

		envname = g_strdup_printf("MPD_OUTPUT_ID_%d", id);
		g_setenv(envname, name, 1);
		g_free(envname);

		envname = g_strdup_printf("MPD_OUTPUT_ID_%d_ENABLED", id);
		g_setenv(envname, mpd_output_get_enabled(output) ? "1" : "0", 1);
		g_free(envname);

		mpd_output_free(output);
	}

	return mpd_response_finish(conn) ? 0 : -1;
}

int
env_status_currentsong(struct mpd_connection *conn, struct mpd_song **song, struct mpd_status **status)
{
	g_assert (conn != NULL);

	g_debug("Sending status & currentsong commands to Mpd server");
	if (!mpd_command_list_begin(conn, true) ||
			!mpd_send_status(conn) ||
			!mpd_send_current_song(conn) ||
			!mpd_command_list_end(conn))
		return -1;

	if ((*status = mpd_recv_status(conn)) == NULL)
		return -1;

	if (mpd_status_get_state(*status) == MPD_STATE_PLAY ||
	    mpd_status_get_state(*status) == MPD_STATE_PAUSE) {
		if (!mpd_response_next(conn)) {
			mpd_status_free(*status);
			return -1;
		}

		if ((*song = mpd_recv_song(conn)) == NULL) {
			mpd_status_free(*status);
			return -1;
		}
	}

	env_export_status(*status);
	if (*song != NULL)
		env_export_song(*song, 0);

	return mpd_response_finish(conn) ? 0 : -1;
}

void
env_clearenv(void)
{
	int i;
	char *envname;

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
	for (i = 1 ;; i++) {
		envname = g_strdup_printf("MPD_SONG_%d_URI", i);
		if (g_getenv(envname) == NULL) {
			g_free(envname);
			break;
		}
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_ARTIST", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_ALBUM", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_ALBUMARTIST", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_ALBUM_ARTIST", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_TITLE", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_TRACK", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_NAME", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_GENRE", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_DATE", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_COMPOSER", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_PERFORMER", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_COMMENT", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_DISC", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_MUSICBRAINZ_ARTISTID", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_MUSICBRAINZ_ALBUMID", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_MUSICBRAINZ_ALBUMARTISTID", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_TAG_MUSICBRAINZ_TRACKID", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_LAST_MODIFIED", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_POS", i);
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_SONG_%d_ID", i);
		g_unsetenv(envname);
		g_free(envname);
	}

	/* stats */
	g_unsetenv("MPD_DATABASE_UPDATE_TIME");
	g_unsetenv("MPD_DATABASE_ARTISTS");
	g_unsetenv("MPD_DATABASE_ALBUMS");
	g_unsetenv("MPD_DATABASE_SONGS");
	g_unsetenv("MPD_DATABASE_PLAY_TIME");
	g_unsetenv("MPD_DATABASE_UPTIME");
	g_unsetenv("MPD_DATABASE_DB_PLAY_TIME");

	/* outputs */
	for (i = 1 ;; i++) {
		envname = g_strdup_printf("MPD_OUTPUT_ID_%d", i);
		if (g_getenv(envname) == NULL) {
			g_free(envname);
			break;
		}
		g_unsetenv(envname);
		g_free(envname);

		envname = g_strdup_printf("MPD_OUTPUT_ID_%d_ENABLED", i);
		g_unsetenv(envname);
		g_free(envname);
	}
}
