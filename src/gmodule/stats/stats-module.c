/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009, 2010, 2016 Ali Polatel <alip@exherbo.org>
 * Based in part upon mpdscribble which is:
 *   Copyright (C) 2008-2009 The Music Player Daemon Project
 *   Copyright (C) 2005-2008 Kuno Woudt <kuno@frob.nl>
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

#include "stats-defs.h"

#include <string.h>

#include <glib.h>
#include <sqlite3.h>

static unsigned last_id = -1;
static bool was_paused = 0;
static struct mpd_song *prev = NULL;
static GTimer *timer = NULL;

static bool
played_long_enough(int elapsed, int length)
{
	return elapsed > 240 || (length >= 30 && elapsed > length / 2);
}

static void
song_changed(const struct mpd_song *song)
{
	g_assert(song != NULL);

	g_timer_start(timer);

	g_debug("New song detected (%s - %s), id: %u, pos: %u",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			mpd_song_get_id(song), mpd_song_get_pos(song));
}

static void
song_started(const struct mpd_song *song)
{
	song_changed(song);
}

static void
song_ended(const struct mpd_song *song)
{
	bool long_enough;
	int elapsed, song_duration, percent_played;
	GError *error;

	g_assert(song != NULL);

	elapsed = g_timer_elapsed(timer, NULL);
	song_duration = mpd_song_get_duration(song);
	long_enough = played_long_enough(elapsed, song_duration);
	percent_played = elapsed * 100 / song_duration;

	g_debug("Saving old song (%s - %s), id: %u, pos: %u",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			mpd_song_get_id(song), mpd_song_get_pos(song));

	error = NULL;
	if (!db_process(song, long_enough, percent_played, &error)) {
		g_warning("Saving old song failed: %s", error->message);
		g_error_free(error);
	}
	else if (error != NULL) {
		g_warning("Skipped saving old song: %s", error->message);
		g_error_free(error);
	}
}

static void
song_playing(const struct mpd_song *song, unsigned elapsed)
{
	unsigned prev_elapsed = g_timer_elapsed(timer, NULL);
	if (prev_elapsed > elapsed) {
		g_debug("Repeated song detected");
		song_ended(song);
		song_started(song);
	}
}

static void
song_continued(void)
{
	g_timer_continue(timer);
}

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

/* Module functions */
static int
init(const struct mpdcron_config *conf, GKeyFile *fd)
{
	GError *error;
	g_debug("Initializing");

	/* Load configuration */
	if (!file_load(conf, fd))
		return MPDCRON_INIT_FAILURE;

	/* Initialize database */
	error = NULL;
	if (!db_init(globalconf.dbpath, true, false, &error)) {
		g_critical("Failed to initialize database `%s': %s",
				globalconf.dbpath, error->message);
		g_error_free(error);
		file_cleanup();
		return MPDCRON_INIT_FAILURE;
	}

	/* Initialize, bind and start the server */
	server_init();
	for (unsigned int i = 0; globalconf.addrs[i] != NULL; i++) {
		if (strncmp(globalconf.addrs[i], "any", 4) == 0)
			server_bind(NULL, globalconf.port);
		else if (globalconf.addrs[i][0] == '/')
			server_bind(globalconf.addrs[i], -1);
		else
			server_bind(globalconf.addrs[i], globalconf.port);
	}
	server_start();

	timer = g_timer_new();
	return MPDCRON_INIT_SUCCESS;
}

static void
destroy(void)
{
	g_message("Exiting");
	if (prev != NULL)
		mpd_song_free(prev);
	g_timer_destroy(timer);
	server_close();
	db_close();
	file_cleanup();
}

static int
event_player(G_GNUC_UNUSED const struct mpd_connection *conn,
		const struct mpd_song *song, const struct mpd_status *status)
{
	enum mpd_state state;

	g_assert(status != NULL);

	state = mpd_status_get_state(status);

	if (state == MPD_STATE_PAUSE) {
		song_paused();
		return MPDCRON_EVENT_SUCCESS;
	}
	else if (state != MPD_STATE_PLAY)
		song_stopped();

	if (was_paused) {
		if (song != NULL && mpd_song_get_id(song) == last_id)
			song_continued();
		was_paused = false;
	}

	/* Submit the previous song */
	if (prev != NULL && (song == NULL || mpd_song_get_id(prev) != mpd_song_get_id(song)))
		song_ended(prev);

	if (song != NULL) {
		if (mpd_song_get_id(song) != last_id) {
			/* New song. */
			song_started(song);
			last_id = mpd_song_get_id(song);
		}
		else {
			/* still playing the previous song */
			song_playing(song, mpd_status_get_elapsed_time(status));
		}
	}

	if (prev != NULL) {
		mpd_song_free(prev);
		prev = NULL;
	}
	if (song != NULL) {
		if ((prev = mpd_song_dup(song)) == NULL) {
			g_critical("mpd_song_dup failed: out of memory");
			return MPDCRON_EVENT_UNLOAD;
		}
	}
	return MPDCRON_EVENT_SUCCESS;
}

struct mpdcron_module module = {
	.name = "Stats",
	.init = init,
	.destroy = destroy,
	.event_player = event_player,
};
