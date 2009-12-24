/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet ai cin fdm=syntax : */

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

#include <glib.h>
#include <libdaemon/dlog.h>
#include <mpd/client.h>

static bool was_paused;
static unsigned last_id = -1;
static GTimer *timer = NULL;

/* Utility functions */
static void song_paused(void)
{
	if (!was_paused)
		g_timer_stop(timer);
	was_paused = true;
}

static void song_stopped(void)
{
	last_id = -1;
	was_paused = false;
}

static void song_continued(void)
{
	g_timer_continue(timer);
}

static void song_changed(const struct mpd_song *song)
{
	char *cpath;

	assert(song != NULL);
	g_timer_start(timer);

	cpath = cover_find(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
	if (cpath == NULL)
		daemon_log(LOG_DEBUG, "%sfailed to find cover for album (%s - %s), suffix: %s",
				NOTIFICATION_LOG_PREFIX,
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				file_config.cover_suffix);

	daemon_log(LOG_DEBUG, "%ssending notify for song (%s - %s), id: %u, pos: %u",
			NOTIFICATION_LOG_PREFIX,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			mpd_song_get_id(song), mpd_song_get_pos(song));

	notification_send(cpath,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
			mpd_song_get_uri(song));
	g_free(cpath);
}

static void song_started(const struct mpd_song *song)
{
	song_changed(song);
}

static void song_playing(const struct mpd_song *song, unsigned elapsed)
{
	unsigned prev_elapsed = g_timer_elapsed(timer, NULL);
	if (prev_elapsed > elapsed) {
		daemon_log(LOG_DEBUG, "%srepeated song detected", NOTIFICATION_LOG_PREFIX);
		song_started(song);
	}
}

static int init(G_GNUC_UNUSED int nodaemon, GKeyFile *fd)
{
	was_paused = false;
	last_id = -1;

	/* Parse configuration */
	if (file_load(fd) < 0)
		return MPDCRON_INIT_FAILURE;

	timer = g_timer_new();
	daemon_log(LOG_INFO, "%sinitialized", NOTIFICATION_LOG_PREFIX);
	return MPDCRON_INIT_SUCCESS;
}

static void destroy(void)
{
	daemon_log(LOG_INFO, "%sexiting", NOTIFICATION_LOG_PREFIX);
	file_cleanup();
	g_timer_destroy(timer);
}

static int event_player(G_GNUC_UNUSED const struct mpd_connection *conn,
		const struct mpd_song *song, const struct mpd_status *status)
{
	enum mpd_state state;

	state = mpd_status_get_state(status);
	assert(song != NULL || state != MPD_STATE_PLAY);

	if (state == MPD_STATE_PAUSE) {
		song_paused();
		return MPDCRON_RUN_SUCCESS;
	}
	else if (state != MPD_STATE_PLAY) {
		song_stopped();
		return MPDCRON_RUN_SUCCESS;
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

	return MPDCRON_RUN_SUCCESS;
}

struct mpdcron_module module = {
	.name = "Notification",
	.generic = false,
	.events = MPD_IDLE_PLAYER,
	.init = init,
	.destroy = destroy,
	.event_player = event_player,
};
