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

#include <assert.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>
#include <mpd/client.h>

#define ENV_DAEMONIZE		"MHOPT_DAEMONIZE"

#define ANSI_NORMAL         "[00;00m"
#define ANSI_MAGENTA        "[00;35m"
#define ANSI_DARK_MAGENTA   "[01;35m"

/* Configuration variables */
char *proxy = NULL;
static int colour = 1;
static GSList *scrobblers = NULL;

/* Globals */
static unsigned last_id = -1;
static bool was_paused = 0;
static struct mpd_song *prev = NULL;
static GTimer *timer = NULL;

/* Prototypes */
int mpdcron_init(GKeyFile *fd);
void mpdcron_close(void);
int mpdcron_run(const struct mpd_connection *conn, const struct mpd_song *song, const struct mpd_status *status);

/* Utility functions */
void vlog(const char *fmt, ...)
{
	va_list args;

	if (getenv(ENV_DAEMONIZE) != NULL)
		return;

	fprintf(stderr, "%sscrobbler@%ld:%s ",
			colour ? ANSI_MAGENTA : "",
			time(NULL),
			colour ? ANSI_DARK_MAGENTA : "");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "%s\n", colour ? ANSI_NORMAL : "");
}

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
	assert(song != NULL);
	if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL ||
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0) == NULL) {
		vlog("new song detected with tags missing (%s)", mpd_song_get_uri(song));
		g_timer_start(timer);
		return;
	}
	g_timer_start(timer);

	vlog("new song detected (%s - %s), id: %u, pos: %u",
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

	assert(song != NULL);
	elapsed = g_timer_elapsed(timer, NULL);

	if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL ||
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0) == NULL) {
		vlog("song (%s) has missing tags, skipping", mpd_song_get_uri(song));
		return;
	}
	else if (!played_long_enough(elapsed, mpd_song_get_duration(song))) {
		vlog("song (%s - %s), id: %u, pos: %u not played long enough, skipping",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				mpd_song_get_id(song), mpd_song_get_pos(song));
		return;
	}

	vlog("submitting old song (%s - %s), id: %u, pos: %u",
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
		vlog("Repeated song detected");
		song_ended(song);
		song_started(song);
	}
}

static void song_continued(void)
{
	g_timer_continue(timer);
	vlog("Song continued");
}

static void song_paused(void)
{
	if (!was_paused)
		g_timer_stop(timer);
	was_paused = true;
	vlog("Song paused");
}

static void song_stopped(void)
{
	last_id = -1;
	was_paused = false;
	vlog("Song stopped");
}

/* Module functions */
int mpdcron_init(G_GNUC_UNUSED GKeyFile *fd)
{
	/* Parse configuration */
	if (file_load(fd, &scrobblers) < 0)
		return -1;
	http_client_init();
	as_init(scrobblers);

	timer = g_timer_new();
	return 0;
}

void mpdcron_close(void)
{
	as_save_cache();
	as_cleanup();
	http_client_finish();
	g_free(proxy);
	g_timer_destroy(timer);
	if (prev != NULL)
		mpd_song_free(prev);
}

int mpdcron_run(G_GNUC_UNUSED const struct mpd_connection *conn,
		const struct mpd_song *song, const struct mpd_status *status)
{
	enum mpd_state state;

	state = mpd_status_get_state(status);
	assert(song != NULL || state != MPD_STATE_PLAY);

	if (state == MPD_STATE_PAUSE) {
		song_paused();
		return 0;
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
			vlog("mpd_song_dup failed: out of memory");
			return -1;
		}
	}
	return 0;
}
