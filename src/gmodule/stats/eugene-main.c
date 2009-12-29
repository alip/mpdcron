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

#include "eugene-defs.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <glib.h>
#include <gio/gio.h>
#include <mpd/client.h>

#define ENV_MPDCRON_HOST	"MPDCRON_HOST"
#define ENV_MPDCRON_PORT	"MPDCRON_PORT"
#define ENV_MPDCRON_PASSWORD	"MPDCRON_PASSWORD"
#define DEFAULT_HOSTNAME	"localhost"
#define DEFAULT_PORT		6601

static int verbosity = LOG_DEBUG;

static void about(void)
{
	printf("eugene-"VERSION GITHEAD "\n");
}

G_GNUC_NORETURN
static void usage(FILE *outf, int exitval)
{
	fprintf(outf, ""
"eugene -- mpdcron statistics client\n"
"eugene COMMAND [OPTIONS]\n"
"\n"
"Commands:\n"
"help          Display help and exit\n"
"version       Display version and exit\n"
"love          Love song/artist/album/genre\n"
"\n"
"See eugene COMMAND --help for more information\n");
	exit(exitval);
}

static int cmd_love(int argc, char **argv)
{
	int port;
	int opta = 0;
	const char *hostname, *password;
	char *expr = NULL, *name = NULL;
	GError *error = NULL;
	GOptionEntry options[] = {
		{"expr", 'e', 0, G_OPTION_ARG_STRING, &expr,
			"Love song/artist/album/genre matching the given expression", NULL},
		{"name", 'n', 0, G_OPTION_ARG_STRING, &name,
			"Love song/artist/album/genre with the given name "
				"(uri in case of song)", NULL},
		{"artist", 'a', 0, G_OPTION_ARG_NONE, &opta,
			"Love artists instead of songs", NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL },
	};
	GOptionContext *ctx;
	GSList *values, *walk;
	struct mpdcron_connection *conn;

	ctx = g_option_context_new("");
	g_option_context_add_main_entries(ctx, options, "eugene-love");
	g_option_context_set_summary(ctx, "eugene-love-"VERSION GITHEAD" - Love song/artist/album/genre");
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

	values = NULL;
	if (opta) {
		if (expr != NULL) {
			if (!mpdcron_love_artist_expr(conn, name, true, &values)) {
				eulog(LOG_ERR, "Failed to love artist: %s", conn->error->message);
				mpdcron_connection_free(conn);
				return 1;
			}
		}
		else if (name != NULL) {
			if (!mpdcron_love_artist(conn, name, true, &values)) {
				eulog(LOG_ERR, "Failed to love artist: %s", conn->error->message);
				mpdcron_connection_free(conn);
				return 1;
			}
		}
		mpdcron_connection_free(conn);
		for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
			struct mpdcron_artist *s = (struct mpdcron_artist *)walk->data;
			printf("%s %d\n", s->name, s->love);
			g_free(s->name);
			g_free(s);
		}
		g_slist_free(values);
		return 0;
	}

	if (expr != NULL) {
		if (!mpdcron_love_expr(conn, expr, true, &values)) {
			eulog(LOG_ERR, "Failed to love song: %s", conn->error->message);
			mpdcron_connection_free(conn);
			return 1;
		}
	}
	else if (name != NULL) {
		if (!mpdcron_love_uri(conn, name, true, &values)) {
			eulog(LOG_ERR, "Failed to love song: %s", conn->error->message);
			mpdcron_connection_free(conn);
			return 1;
		}
	}

	mpdcron_connection_free(conn);
	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_song *s = (struct mpdcron_song *)walk->data;
		printf("%s %d\n", s->uri, s->love);
		g_free(s->uri);
		g_free(s);
	}
	g_slist_free(values);
	return 0;
}

static int run_cmd(int argc, char **argv)
{
	if (strncmp(argv[0], "help", 4) == 0)
		usage(stdout, 0);
	else if (strncmp(argv[0], "version", 8) == 0) {
		about();
		return 0;
	}
	else if (strncmp(argv[0], "love", 5) == 0)
		return cmd_love(argc, argv);
	fprintf(stderr, "Unknown command `%s'\n", argv[0]);
	usage(stderr, 1);
}

void eulog(int level, const char *fmt, ...)
{
	va_list args;

	if (level > verbosity)
		return;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fputc('\n', stderr);
}

int main(int argc, char **argv)
{
	g_type_init();

	if (argc < 2)
		usage(stderr, 1);
	return run_cmd(argc - 1, argv + 1);
}
