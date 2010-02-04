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
addtag_artist(struct mpdcron_connection *conn, const char *tag, const char *expr)
{
	int changes;

	if (expr != NULL) {
		if (!mpdcron_addtag_artist_expr(conn, expr, tag, &changes)) {
			eulog(LOG_ERR, "Failed to add tag to artist: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_artist, *myexpr;
		struct mpd_song *song;

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

		if (!mpdcron_addtag_artist_expr(conn, myexpr, tag, &changes)) {
			eulog(LOG_ERR, "Failed to add tag to current playing artist: %s",
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
addtag_album(struct mpdcron_connection *conn, const char *tag, const char *expr)
{
	int changes;

	if (expr != NULL) {
		if (!mpdcron_addtag_album_expr(conn, expr, tag, &changes)) {
			eulog(LOG_ERR, "Failed to add tag to album: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_album, *esc_artist, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no artist tag!");
			mpd_song_free(song);
			return 1;
		}
		else if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no album tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_album = quote(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
		esc_artist = quote(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
		myexpr = g_strdup_printf("name=%s and artist=%s", esc_album, esc_artist);
		g_free(esc_album);
		g_free(esc_artist);
		mpd_song_free(song);

		if (!mpdcron_addtag_album_expr(conn, myexpr, tag, &changes)) {
			eulog(LOG_ERR, "Failed to add tag to current playing album: %s",
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
addtag_genre(struct mpdcron_connection *conn, const char *tag, const char *expr)
{
	int changes;

	if (expr != NULL) {
		if (!mpdcron_addtag_genre_expr(conn, expr, tag, &changes)) {
			eulog(LOG_ERR, "Failed to add tag to genre: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_genre, *myexpr;
		struct mpd_song *song;

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

		if (!mpdcron_addtag_genre_expr(conn, myexpr, tag, &changes)) {
			eulog(LOG_ERR, "Failed to add tag to current playing genre: %s",
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
addtag_song(struct mpdcron_connection *conn, const char *tag, const char *expr)
{
	int changes;

	if (expr != NULL) {
		if (!mpdcron_addtag_expr(conn, expr, tag, &changes)) {
			eulog(LOG_ERR, "Failed to add tag to song: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_uri, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;

		esc_uri = quote(mpd_song_get_uri(song));
		myexpr = g_strdup_printf("uri=%s", esc_uri);
		g_free(esc_uri);
		mpd_song_free(song);

		if (!mpdcron_addtag_expr(conn, myexpr, tag, &changes)) {
			eulog(LOG_ERR, "Failed to add tag to current playing song: %s",
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
cmd_addtag_internal(const char *tag, const char *expr,
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
		ret = addtag_artist(conn, tag, expr);
	else if (album)
		ret = addtag_album(conn, tag, expr);
	else if (genre)
		ret = addtag_genre(conn, tag, expr);
	else
		ret = addtag_song(conn, tag, expr);
	mpdcron_connection_free(conn);
	return ret;
}

int
cmd_addtag(int argc, char **argv)
{
	int opta = 0, optA = 0, optg = 0, ret;
	GError *error = NULL;
	GOptionEntry options[] = {
		{"artist", 'a', 0, G_OPTION_ARG_NONE, &opta,
			"Add tag to artists instead of songs", NULL},
		{"album", 'A', 0, G_OPTION_ARG_NONE, &optA,
			"Add tag to albums instead of songs", NULL},
		{"genre", 'g', 0, G_OPTION_ARG_NONE, &optg,
			"Add tag to genres instead of songs", NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL },
	};
	GOptionContext *ctx;

	ctx = g_option_context_new("TAG [EXPRESSION]");
	g_option_context_add_main_entries(ctx, options, "eugene-addtag");
	g_option_context_set_summary(ctx, "eugene-addtag-"VERSION GITHEAD" - Add tag to song/artist/album/genre");
	g_option_context_set_description(ctx, ""
"By default this command works on the current playing song.\n"
"For more information about the expression syntax, see:\n"
"http://www.sqlite.org/lang_expr.html");
	if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
		g_printerr("Option parsing failed: %s\n", error->message);
		g_error_free(error);
		g_option_context_free(ctx);
		return 1;
	}
	g_option_context_free(ctx);

	if (argc <= 1) {
		g_printerr("No tag given\n");
		return 1;
	}

	if (argc > 2)
		ret = cmd_addtag_internal(argv[1], argv[2], opta, optA, optg);
	else
		ret = cmd_addtag_internal(argv[1], NULL, opta, optA, optg);
	return ret;
}
