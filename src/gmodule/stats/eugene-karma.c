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
karma_song(struct mpdcron_connection *conn, const char *expr, const char *karma)
{
	int changes;
	char *esc_uri, *query_uri;
	struct mpd_song *song;

	if (expr != NULL) {
		if (!mpdcron_karma_expr(conn, expr, karma, &changes)) {
			eulog(LOG_ERR, "Failed to change karma of song: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		if ((song = load_current_song()) == NULL)
			return 1;

		esc_uri = quote(mpd_song_get_uri(song));
		query_uri = g_strdup_printf("uri=%s", esc_uri);
		g_free(esc_uri);
		mpd_song_free(song);

		if (!mpdcron_karma_expr(conn, query_uri, karma, &changes)) {
			eulog(LOG_ERR, "Failed to change karma of current "
					"playing song: %s",
					conn->error->message);
			g_free(query_uri);
			return 1;
		}
		g_free(query_uri);
	}
	printf("Modified %d entries\n", changes);
	return 0;
}

static int
cmd_karma_internal(const char *expr, const char *karma)
{
	int port, ret;
	const char *hostname, *password;
	struct mpdcron_connection *conn;

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

	ret = karma_song(conn, expr, karma);
	mpdcron_connection_free(conn);
	return ret;
}

int
cmd_karma(int argc, char **argv)
{
	int ret;
	GError *error = NULL;
	GOptionContext *ctx;

	ctx = g_option_context_new("KARMA [EXPRESSION]");
	g_option_context_set_summary(ctx, "eugene-karma-"VERSION GITHEAD
			" - Set karma rating of song");
	g_option_context_set_description(ctx,
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
		g_printerr("No karma given\n");
		ret = 1;
	}
	else if (argc > 2)
		ret = cmd_karma_internal(argv[2], argv[1]);
	else
		ret = cmd_karma_internal(NULL, argv[1]);
	return ret;
}
