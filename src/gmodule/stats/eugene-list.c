/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009-2010 Ali Polatel <alip@exherbo.org>
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
list_artist(struct mpdcron_connection *conn, const char *expr)
{
	GSList *values, *walk;

	g_assert(expr != NULL);

	values = NULL;
	if (!mpdcron_list_artist_expr(conn, expr, &values)) {
		eulog(LOG_ERR, "Failed to list artist: %s",
				conn->error->message);
		return 1;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: %s\n", e->id, e->name);
		g_free(e->name);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
list_album(struct mpdcron_connection *conn, const char *expr)
{
	GSList *values, *walk;

	g_assert(expr != NULL);

	values = NULL;
	if (!mpdcron_list_album_expr(conn, expr, &values)) {
		eulog(LOG_ERR, "Failed to list album: %s",
				conn->error->message);
		return 1;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: %s\n", e->id, e->name);
		g_free(e->name);
		g_free(e->artist); /* TODO: We don't use artist information. */
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
list_genre(struct mpdcron_connection *conn, const char *expr)
{
	GSList *values, *walk;

	g_assert(expr != NULL);

	values = NULL;
	if (!mpdcron_list_genre_expr(conn, expr, &values)) {
		eulog(LOG_ERR, "Failed to list genre: %s",
				conn->error->message);
		return 1;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: %s\n", e->id, e->name);
		g_free(e->name);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
list_song(struct mpdcron_connection *conn, const char *expr)
{
	GSList *values, *walk;

	g_assert(expr != NULL);

	values = NULL;
	if (!mpdcron_list_expr(conn, expr, &values)) {
		eulog(LOG_ERR, "Failed to list song: %s",
				conn->error->message);
		return 1;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_song *e = walk->data;
		printf("%d: %s\n", e->id, e->uri);
		g_free(e->uri);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
cmd_list_internal(const char *expr, bool artist, bool album,
		bool genre)
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
		ret = list_artist(conn, expr);
	else if (album)
		ret = list_album(conn, expr);
	else if (genre)
		ret = list_genre(conn, expr);
	else
		ret = list_song(conn, expr);
	mpdcron_connection_free(conn);
	return ret;
}

int
cmd_list(int argc, char **argv)
{
	int opta = 0, optA = 0, optg = 0, ret;
	GError *error = NULL;
	GOptionEntry options[] = {
		{"artist", 'a', 0, G_OPTION_ARG_NONE, &opta,
			"List artists instead of songs", NULL},
		{"album", 'A', 0, G_OPTION_ARG_NONE, &optA,
			"List albums instead of songs", NULL},
		{"genre", 'g', 0, G_OPTION_ARG_NONE, &optg,
			"List genres instead of songs", NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL },
	};
	GOptionContext *ctx;

	ctx = g_option_context_new("EXPRESSION");
	g_option_context_add_main_entries(ctx, options, "eugene-list");
	g_option_context_set_summary(ctx, "eugene-list-"VERSION GITHEAD" - List song/artist/album/genre");
	g_option_context_set_description(ctx, ""
"For more information about the expression syntax, see:\n"
"http://www.sqlite.org/lang_expr.html");
	if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
		g_printerr("Option parsing failed: %s\n", error->message);
		g_error_free(error);
		g_option_context_free(ctx);
		return 1;
	}
	g_option_context_free(ctx);

	if (argc > 1)
		ret = cmd_list_internal(argv[1], opta, optA, optg);
	else {
		g_printerr("No expression given\n");
		ret = 1;
	}
	return ret;
}
