/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet ai cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
 *
 * This file is part of the mpdhooker mpd client. mpdhooker is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * mpdhooker is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mh-defs.h"

#include <time.h>

#include <glib.h>
#include <libdaemon/dlog.h>
#include <mpd/client.h>

static const char *mhenv_strstate(enum mpd_state state)
{
	char *strstate = NULL;
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

static void mhenv_export_status(struct mpd_status *status)
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

	g_setenv("MPD_STATUS_STATE", mhenv_strstate(mpd_status_get_state(status)), 1);

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

int mhenv_stats(struct mpd_connection *conn)
{
	char *envstr;
	time_t t;
	struct mpd_stats *stats;

	g_assert(conn != NULL);
	mh_log(LOG_DEBUG, "Sending stats command to Mpd server");
	if ((stats = mpd_run_stats(conn)) == NULL)
		return -1;

	t = mpd_stats_get_db_update_time(stats);

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

	envstr = g_strdup_printf("%s", ctime(&t));
	g_setenv("MPD_DATABASE_UPDATE_TIME", envstr, 1);
	g_free(envstr);

	envstr = g_strdup_printf("%lu", mpd_stats_get_db_play_time(stats));
	g_setenv("MPD_DATABASE_DB_PLAY_TIME", envstr, 1);
	g_free(envstr);

	envstr = NULL;
	mpd_stats_free(stats);
	return 0;
}

int mhenv_status(struct mpd_connection *conn)
{
	struct mpd_status *status;

	g_assert(conn != NULL);
	mh_log(LOG_DEBUG, "Sending status command to Mpd server");
	if ((status = mpd_run_status(conn)) == NULL)
		return -1;

	mhenv_export_status(status);
	mpd_status_free(status);
	return 0;
}

int mhenv_outputs(struct mpd_connection *conn)
{
	int id;
	const char *name;
	char *envname;
	struct mpd_output *output;

	g_assert(conn != NULL);
	mh_log(LOG_DEBUG, "Sending outputs command to Mpd server");
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
