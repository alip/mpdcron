/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet ai cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
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

#include "scrobbler-defs.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>
#include <libdaemon/dlog.h>
#include <mpd/client.h>

/* Globals */
static unsigned last_id = -1;
static bool was_paused = 0;
static struct mpd_song *prev = NULL;
static GTimer *timer = NULL;
static int save_source_id = -1;

static bool played_long_enough(int elapsed, int length)
{
	/* http://www.lastfm.de/api/submissions "The track must have been
	   played for a duration of at least 240 seconds or half the track's
	   total length, whichever comes first. Skipping or pausing the
	   track is irrelevant as long as the appropriate amount has been
	   played."
	 */
	return elapsed > 240 || (length >= 30 && elapsed > length / 2);
}

static bool song_repeated(const struct mpd_song *song, int elapsed, int prev_elapsed)
{
	return elapsed < 60 && prev_elapsed > elapsed &&
		played_long_enough(prev_elapsed - elapsed,
				mpd_song_get_duration(song));
}

static void song_changed(const struct mpd_song *song)
{
	g_assert(song != NULL);

	if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL ||
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0) == NULL) {
		daemon_log(LOG_INFO, "%snew song detected with tags missing (%s)",
				SCROBBLER_LOG_PREFIX,
				mpd_song_get_uri(song));
		g_timer_start(timer);
		return;
	}
	g_timer_start(timer);

	daemon_log(LOG_DEBUG, "%snew song detected (%s - %s), id: %u, pos: %u",
			SCROBBLER_LOG_PREFIX,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			mpd_song_get_id(song), mpd_song_get_pos(song));

	as_now_playing(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_TRACKID, 0),
			mpd_song_get_duration(song));
}

static void song_started(const struct mpd_song *song)
{
	song_changed(song);
}

static void song_ended(const struct mpd_song *song)
{
	int elapsed;

	g_assert(song != NULL);

	elapsed = g_timer_elapsed(timer, NULL);

	if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL ||
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0) == NULL) {
		daemon_log(LOG_INFO, "%ssong (%s) has missing tags, skipping",
				SCROBBLER_LOG_PREFIX,
				mpd_song_get_uri(song));
		return;
	}
	else if (!played_long_enough(elapsed, mpd_song_get_duration(song))) {
		daemon_log(LOG_INFO, "%ssong (%s - %s), id: %u, pos: %u not played long enough, skipping",
				SCROBBLER_LOG_PREFIX,
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				mpd_song_get_id(song), mpd_song_get_pos(song));
		return;
	}

	daemon_log(LOG_DEBUG, "%ssubmitting old song (%s - %s), id: %u, pos: %u",
			SCROBBLER_LOG_PREFIX,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			mpd_song_get_id(song), mpd_song_get_pos(song));

	/* FIXME: libmpdclient doesn't have any way to fetch the musicbrainz id.
	 */
	as_songchange(mpd_song_get_uri(song),
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_TRACKID, 0),
			mpd_song_get_duration(song) > 0
			? mpd_song_get_duration(song)
			: g_timer_elapsed(timer, NULL),
			NULL);
}

static void song_playing(const struct mpd_song *song, unsigned elapsed)
{
	int prev_elapsed = g_timer_elapsed(timer, NULL);
	if (song_repeated(song, elapsed, prev_elapsed)) {
		daemon_log(LOG_DEBUG, "%srepeated song detected", SCROBBLER_LOG_PREFIX);
		song_ended(song);
		song_started(song);
	}
}

static void song_continued(void)
{
	g_timer_continue(timer);
}

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

/* Module functions */
static int init(G_GNUC_UNUSED const struct mpdcron_config *conf, GKeyFile *fd)
{
	/* Parse configuration */
	if (file_load(fd) < 0)
		return MPDCRON_INIT_FAILURE;
	if (http_client_init() < 0)
		return MPDCRON_INIT_FAILURE;
	as_init(file_config.scrobblers);

	timer = g_timer_new();
	save_source_id = g_timeout_add_seconds(file_config.journal_interval, timer_save_journal, NULL);

	return MPDCRON_INIT_SUCCESS;
}

static void destroy(void)
{
	daemon_log(LOG_INFO, "%sexiting", SCROBBLER_LOG_PREFIX);
	file_cleanup();
	as_save_cache();
	as_cleanup();
	http_client_finish();
	g_timer_destroy(timer);
	g_source_remove(save_source_id);
	if (prev != NULL)
		mpd_song_free(prev);
}

static int event_player(G_GNUC_UNUSED const struct mpd_connection *conn,
		const struct mpd_song *song, const struct mpd_status *status)
{
	enum mpd_state state;

	state = mpd_status_get_state(status);
	g_assert(song != NULL || state != MPD_STATE_PLAY);

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
			daemon_log(LOG_ERR, "%smpd_song_dup failed: out of memory",
					SCROBBLER_LOG_PREFIX);
			return MPDCRON_EVENT_UNLOAD;
		}
	}
	return MPDCRON_EVENT_SUCCESS;
}

struct mpdcron_module module = {
	.name = "Scrobbler",
	.init = init,
	.destroy = destroy,
	.event_player = event_player,
};
