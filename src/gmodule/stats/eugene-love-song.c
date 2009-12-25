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

#include <stdlib.h>
#include <stdio.h>

#include <glib.h>
#include <mpd/client.h>

static int optc = 0;
static int optd = 0;
static int optp = 0;
static int optv = 0;
static char *uri = NULL;
static char *expr = NULL;

static GOptionEntry options[] = {
	{"verbose", 'v', 0, G_OPTION_ARG_NONE, &optv, "Be verbose", NULL},
	{"debug", 'D', 0, G_OPTION_ARG_NONE, &optd, "Be even more verbose", NULL},
	{"dbpath", 'd', 0, G_OPTION_ARG_FILENAME, &euconfig.dbpath, "Path to the database", NULL},
	{"current", 'c', 0, G_OPTION_ARG_NONE, &optc, "Love current playing song", NULL},
	{"uri", 'u', 0, G_OPTION_ARG_STRING, &uri, "Love song with the given uri", NULL},
	{"pattern", 'p', 0, G_OPTION_ARG_NONE, &optp, "Treat string as pattern for --uri", NULL},
	{"expr", 'e', 0, G_OPTION_ARG_STRING, &expr, "Love songs matching the given expression", NULL},
	{ NULL, -1, 0, 0, NULL, NULL, NULL },
};

static int love_current(void)
{
	int ret;
	struct mpd_song *song;

	if ((song = load_current_song()) == NULL)
		return 1;
	ret = db_love(euconfig.dbpath, song, true);
	mpd_song_free(song);
	return ret ? 0 : 1;
}

int cmd_love_song(int argc, char **argv)
{
	GOptionContext *ctx;
	GError *parse_err = NULL;

	ctx = g_option_context_new("");
	g_option_context_add_main_entries(ctx, options, "eugene-love-song");
	g_option_context_set_summary(ctx, "eugene-love-song-"VERSION GITHEAD" - love command");

	if (!g_option_context_parse(ctx, &argc, &argv, &parse_err)) {
		g_printerr("eugene-love-song: option parsing failed: %s\n", parse_err->message);
		g_error_free(parse_err);
		g_option_context_free(ctx);
		return -1;
	}
	g_option_context_free(ctx);

	if (optv)
		euconfig.verbosity = LOG_INFO;
	if (optd)
		euconfig.verbosity = LOG_DEBUG;

	if (euconfig.dbpath == NULL)
		load_paths();

	if (!db_init(euconfig.dbpath))
		return -1;

	if (optc)
		return love_current();
	else if (uri != NULL)
		return db_love_uri(euconfig.dbpath, uri, true, optp,
				(euconfig.verbosity > LOG_NOTICE)) ? 0 : 1;
	else if (expr != NULL)
		return db_love_expr(euconfig.dbpath, expr, true,
				(euconfig.verbosity > LOG_NOTICE)) ? 0 : 1;

	fprintf(stderr, "Neither --current nor --uri nor --expr specified\n");
	return -1;
}
