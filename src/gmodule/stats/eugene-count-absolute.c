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

#include "eugene-defs.h"

#include <stdio.h>
#include <stdlib.h>

#include <glib.h>

static int
count_absolute_artist(struct mpdcron_connection *conn, const char *expr,
		const char *rating)
{
	int changes;
	char *esc_artist, *myexpr;
	struct mpd_song *song;

	if (expr != NULL) {
		if (!mpdcron_count_absolute_artist_expr(conn, expr, rating, &changes)) {
			eulog(LOG_ERR, "Failed to rate artist: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no artist tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_artist = quote(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
		myexpr = g_strdup_printf("name=%s", esc_artist);
		g_free(esc_artist);
		mpd_song_free(song);

		if (!mpdcron_count_absolute_artist_expr(conn, myexpr, rating, &changes)) {
			eulog(LOG_ERR, "Failed to rate current playing artist: %s",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}
	printf("Modified %d entries\n", changes);
	return 0;
}

static int
count_absolute_album(struct mpdcron_connection *conn, const char *expr,
		const char *rating)
{
	int changes;
	char *esc_album, *myexpr;
	struct mpd_song *song;

	if (expr != NULL) {
		if (!mpdcron_count_absolute_album_expr(conn, expr, rating, &changes)) {
			eulog(LOG_ERR, "Failed to rate album: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no album tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_album = quote(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
		myexpr = g_strdup_printf("name=%s", esc_album);
		g_free(esc_album);
		mpd_song_free(song);

		if (!mpdcron_count_absolute_album_expr(conn, myexpr, rating, &changes)) {
			eulog(LOG_ERR, "Failed to rate current playing album: %s",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}
	printf("Modified %d entries\n", changes);
	return 0;
}

static int
count_absolute_genre(struct mpdcron_connection *conn, const char *expr,
		const char *rating)
{
	int changes;
	char *esc_genre, *myexpr;
	struct mpd_song *song;

	if (expr != NULL) {
		if (!mpdcron_count_absolute_genre_expr(conn, expr, rating, &changes)) {
			eulog(LOG_ERR, "Failed to rate genre: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_GENRE, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no genre tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_genre = quote(mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
		myexpr = g_strdup_printf("name=%s", esc_genre);
		g_free(esc_genre);
		mpd_song_free(song);

		if (!mpdcron_count_absolute_genre_expr(conn, myexpr, rating, &changes)) {
			eulog(LOG_ERR, "Failed to rate current playing genre: %s",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}
	printf("Modified %d entries\n", changes);
	return 0;
}

static int
count_absolute_song(struct mpdcron_connection *conn,
		const char *expr, const char *rating)
{
	int changes;
	char *esc_uri, *myexpr;
	struct mpd_song *song;

	if (expr != NULL) {
		if (!mpdcron_count_absolute_expr(conn, expr, rating, &changes)) {
			eulog(LOG_ERR, "Failed to rate song: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		if ((song = load_current_song()) == NULL)
			return 1;

		esc_uri = quote(mpd_song_get_uri(song));
		myexpr = g_strdup_printf("uri=%s", esc_uri);
		g_free(esc_uri);
		mpd_song_free(song);

		if (!mpdcron_count_absolute_expr(conn, myexpr, rating, &changes)) {
			eulog(LOG_ERR, "Failed to rate current playing song: %s",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}
	printf("Modified %d entries\n", changes);
	return 0;
}

static int
cmd_count_absolute_internal(const char *expr, const char *rating,
		bool artist, bool album, bool genre)
{
	int port, ret;
	const char *hostname, *password;
	struct mpdcron_connection *conn;

	if ((artist && album && genre) ||
			(artist && album) ||
			(artist && genre) ||
			(album && genre)) {
		g_printerr("--artist, --album and --genre options are mutually exclusive\n");
		return 1;
	}

	hostname = g_getenv(ENV_MPDCRON_HOST)
		? g_getenv(ENV_MPDCRON_HOST)
		: DEFAULT_HOSTNAME;
	port = g_getenv(ENV_MPDCRON_PORT)
		? atoi(g_getenv(ENV_MPDCRON_PORT))
		: DEFAULT_PORT;
	password = g_getenv(ENV_MPDCRON_PASSWORD);

	conn = mpdcron_connection_new(hostname, port);
	if (conn->error != NULL) {
		eulog(LOG_ERR, "Failed to connect: %s", conn->error->message);
		mpdcron_connection_free(conn);
		return 1;
	}

	if (password != NULL) {
		if (!mpdcron_password(conn, password)) {
			eulog(LOG_ERR, "Authentication failed: %s", conn->error->message);
			mpdcron_connection_free(conn);
			return 1;
		}
	}

	if (artist)
		ret = count_absolute_artist(conn, expr, rating);
	else if (album)
		ret = count_absolute_album(conn, expr, rating);
	else if (genre)
		ret = count_absolute_genre(conn, expr, rating);
	else
		ret = count_absolute_song(conn, expr, rating);
	mpdcron_connection_free(conn);
	return ret;
}

int
cmd_count_absolute(int argc, char **argv)
{
	int opta = 0, optA = 0, optg = 0, ret;
	GError *error = NULL;
	GOptionEntry options[] = {
		{"artist", 'a', 0, G_OPTION_ARG_NONE, &opta,
			"Rate artists instead of songs", NULL},
		{"album", 'A', 0, G_OPTION_ARG_NONE, &optA,
			"Rate albums instead of songs", NULL},
		{"genre", 'g', 0, G_OPTION_ARG_NONE, &optg,
			"Rate genres instead of songs", NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL },
	};
	GOptionContext *ctx;

	ctx = g_option_context_new("RATING [EXPRESSION]");
	g_option_context_add_main_entries(ctx, options, "eugene-count-absolute");
	g_option_context_set_summary(ctx, "eugene-count-absolute"VERSION GITHEAD" - Rate (absolute fashion) song/artist/album/genre");
	g_option_context_set_description(ctx, ""
"By default this command works on the current playing song.\n"
"For more information about the expression syntax, see:\n"
"http://www.sqlite.org/lang_expr.html");
	/* Ignore unknown options so that the user can pass negative numbers.
	 */
	g_option_context_set_ignore_unknown_options(ctx, TRUE);
	if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
		g_printerr("Option parsing failed: %s\n", error->message);
		g_error_free(error);
		g_option_context_free(ctx);
		return 1;
	}
	g_option_context_free(ctx);

	if (argc <= 1) {
		g_printerr("No rating given\n");
		ret = 1;
	}
	else if (argc > 2)
		ret = cmd_count_absolute_internal(argv[2], argv[1], opta, optA, optg);
	else
		ret = cmd_count_absolute_internal(NULL, argv[1], opta, optA, optg);
	return ret;
}
