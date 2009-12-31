/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
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

/* Notes about the module:
 * Using libnotify directly doesn't work due to many reasons.
 * That's why we spawn notify-send.
 */

#include "notification-defs.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include <glib.h>
#include <libdaemon/dlog.h>
#include <mpd/client.h>

static bool was_paused;
static unsigned last_id = -1;
static GTimer *timer = NULL;

/* Utility functions */
static void
song_paused(void)
{
	if (!was_paused)
		g_timer_stop(timer);
	was_paused = true;
}

static void
song_stopped(void)
{
	last_id = -1;
	was_paused = false;
}

static void
song_continued(void)
{
	g_timer_continue(timer);
}

static void
song_changed(const struct mpd_song *song)
{
	const char *summary;
	char *cpath, *body;

	assert(song != NULL);
	g_timer_start(timer);

	cpath = cover_find(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
	if (cpath == NULL)
		mpdcron_log(LOG_DEBUG, "Failed to find cover for album (%s - %s), suffix: %s",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				file_config.cover_suffix);

	mpdcron_log(LOG_DEBUG, "Sending notify for song (%s - %s), id: %u, pos: %u",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			mpd_song_get_id(song), mpd_song_get_pos(song));

	summary = mpd_song_get_tag(song, MPD_TAG_TITLE, 0)
		? mpd_song_get_tag(song, MPD_TAG_TITLE, 0)
		: mpd_song_get_uri(song);
	body = g_strdup_printf("by %s - %s",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0)
			? mpd_song_get_tag(song, MPD_TAG_ARTIST, 0)
			: "Unknown",
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0)
			? mpd_song_get_tag(song, MPD_TAG_ALBUM, 0)
			: "Unknown");
	notify_send(cpath, summary, body);
	g_free(body);
	g_free(cpath);
}

static void
song_started(const struct mpd_song *song)
{
	song_changed(song);
}

static void
song_playing(const struct mpd_song *song, unsigned elapsed)
{
	unsigned prev_elapsed = g_timer_elapsed(timer, NULL);
	if (prev_elapsed > elapsed) {
		mpdcron_log(LOG_DEBUG, "Repeated song detected");
		song_started(song);
	}
}

static int
init(G_GNUC_UNUSED const struct mpdcron_config *conf, GKeyFile *fd)
{
	was_paused = false;
	last_id = -1;

	/* Parse configuration */
	if (file_load(fd) < 0)
		return MPDCRON_INIT_FAILURE;

	timer = g_timer_new();
	mpdcron_log(LOG_INFO, "Initialized");
	return MPDCRON_INIT_SUCCESS;
}

static void
destroy(void)
{
	mpdcron_log(LOG_INFO, "Exiting");
	file_cleanup();
	g_timer_destroy(timer);
}

static int
event_database(G_GNUC_UNUSED const struct mpd_connection *conn,
		const struct mpd_stats *stats)
{
	time_t t;
	const char *summary;
	char *play_time, *uptime, *db_play_time;
	char *body;

	g_assert(stats != NULL);

	if ((file_config.events | MPD_IDLE_DATABASE) == 0)
		return MPDCRON_EVENT_SUCCESS;

	play_time = dhms(mpd_stats_get_play_time(stats));
	uptime = dhms(mpd_stats_get_uptime(stats));
	db_play_time = dhms(mpd_stats_get_db_play_time(stats));
	t = mpd_stats_get_db_update_time(stats);

	summary = "Mpd Database has been updated";
	body = g_strdup_printf("Artists: %u\n"
			"Albums: %u\n"
			"Songs: %u\n"
			"\n"
			"Play Time: %s\n"
			"Uptime: %s\n"
			"DB Updated: %s"
			"DB Play Time: %s",
			mpd_stats_get_number_of_artists(stats),
			mpd_stats_get_number_of_albums(stats),
			mpd_stats_get_number_of_songs(stats),
			play_time,
			uptime,
			ctime(&t),
			db_play_time);

	notify_send(NULL, summary, body);
	g_free(play_time);
	g_free(uptime);
	g_free(db_play_time);
	g_free(body);
	return MPDCRON_EVENT_SUCCESS;
}

static int
event_player(G_GNUC_UNUSED const struct mpd_connection *conn,
		const struct mpd_song *song, const struct mpd_status *status)
{
	enum mpd_state state;

	g_assert(status != NULL);

	if ((file_config.events | MPD_IDLE_PLAYER) == 0)
		return MPDCRON_EVENT_SUCCESS;

	state = mpd_status_get_state(status);
	assert(song != NULL || state != MPD_STATE_PLAY);

	if (state == MPD_STATE_PAUSE) {
		song_paused();
		return MPDCRON_EVENT_SUCCESS;
	}
	else if (state != MPD_STATE_PLAY) {
		song_stopped();
		return MPDCRON_EVENT_SUCCESS;
	}

	if (was_paused) {
		if (song != NULL && mpd_song_get_id(song) == last_id)
			song_continued();
		was_paused = false;
	}

	if (song != NULL) {
		if (mpd_song_get_id(song) != last_id) {
			/* New song */
			song_started(song);
			last_id = mpd_song_get_id(song);
		}
		else {
			/* Still playing the previous song */
			song_playing(song, mpd_status_get_elapsed_time(status));
		}
	}

	return MPDCRON_EVENT_SUCCESS;
}

static int
event_mixer(G_GNUC_UNUSED const struct mpd_connection *conn,
		const struct mpd_status *status)
{
	char *summary;

	g_assert(status != NULL);

	if ((file_config.events | MPD_IDLE_MIXER) == 0)
		return MPDCRON_EVENT_SUCCESS;

	summary = g_strdup_printf("Mpd Volume: %d%%", mpd_status_get_volume(status));
	notify_send(NULL, summary, "");
	g_free(summary);
	return MPDCRON_EVENT_SUCCESS;
}

static int
event_options(G_GNUC_UNUSED const struct mpd_connection *conn,
		const struct mpd_status *status)
{
	char *body;

	g_assert(status != NULL);

	if ((file_config.events | MPD_IDLE_OPTIONS) == 0)
		return MPDCRON_EVENT_SUCCESS;

	body = g_strdup_printf("Repeat: %s\n"
			"Random: %s\n"
			"Single: %s\n"
			"Consume: %s\n"
			"Crossfade: %u",
			mpd_status_get_repeat(status) ? "on" : "off",
			mpd_status_get_random(status) ? "on" : "off",
			mpd_status_get_single(status) ? "on" : "off",
			mpd_status_get_consume(status) ? "on" : "off",
			mpd_status_get_crossfade(status));
	notify_send(NULL, "Mpd Options have changed!", body);
	g_free(body);
	return MPDCRON_EVENT_SUCCESS;
}

static int
event_update(G_GNUC_UNUSED const struct mpd_connection *conn,
		const struct mpd_status *status)
{
	char *summary;

	g_assert(status != NULL);

	if ((file_config.events | MPD_IDLE_UPDATE) == 0)
		return MPDCRON_EVENT_SUCCESS;

	summary = g_strdup_printf("Mpd Update ID: %u",
			mpd_status_get_update_id(status));
	notify_send(NULL, summary, "");
	return MPDCRON_EVENT_SUCCESS;
}

struct mpdcron_module module = {
	.name = "Notification",
	.init = init,
	.destroy = destroy,
	.event_database = event_database,
	.event_player = event_player,
	.event_mixer = event_mixer,
	.event_options = event_options,
	.event_update = event_update,
};
