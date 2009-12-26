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
#include <sqlite3.h>

static int optv = 0;
static int opta = 0;
static int optA = 0;
static int optg = 0;
static int optc = 0;
static int optr = 0;
static char *expr = NULL;

static GOptionEntry options[] = {
	{"verbose", 'v', 0, G_OPTION_ARG_NONE, &optv, "Be verbose", NULL},
	{"dbpath", 'd', 0, G_OPTION_ARG_FILENAME, &euconfig.dbpath, "Path to the database", NULL},
	{"expr", 'e', 0, G_OPTION_ARG_STRING, &expr, "Load songs matching the given expression", NULL},
	{"clear", 'c', 0, G_OPTION_ARG_NONE, &optc, "Clear playlist before loading", NULL},
	{"restore", 'r', 0, G_OPTION_ARG_NONE, &optr, "Restore mpd's playing state after loading", NULL},
	{"artist", 0, 0, G_OPTION_ARG_NONE, &opta, "Expression matches artists instead of songs", NULL},
	{"album", 0, 0, G_OPTION_ARG_NONE, &optA, "Expression matches albums instead of songs", NULL},
	{"genre", 0, 0, G_OPTION_ARG_NONE, &optg, "Expression matches genres instead of songs", NULL},
	{ NULL, 0, 0, 0, NULL, NULL, NULL },
};

int cmd_load(int argc, char **argv)
{
	int ret;
	GOptionContext *ctx;
	GError *parse_err = NULL;
	GSList *song_list = NULL;
	sqlite3 *db;

	ctx = g_option_context_new("");
	g_option_context_add_main_entries(ctx, options, "eugene-load");
	g_option_context_set_summary(ctx, "eugene-load-"VERSION GITHEAD" - Load songs");
	g_option_context_set_description(ctx, ""
"For more information about the expression syntax, see:\n"
"http://www.sqlite.org/lang_expr.html");
	if (!g_option_context_parse(ctx, &argc, &argv, &parse_err)) {
		g_printerr("Option parsing failed: %s\n", parse_err->message);
		g_error_free(parse_err);
		g_option_context_free(ctx);
		return 1;
	}
	g_option_context_free(ctx);

	if (optv)
		euconfig.verbosity = LOG_DEBUG;
	if ((opta && optA && optg) || (opta && optA) || (opta && optg) || (optA && optg)) {
		g_printerr("--artist, --album and --genre options are mutually exclusive\n");
		return 1;
	}
	if (expr == NULL) {
		g_printerr("No expression given\n");
		return 1;
	}

	if (euconfig.dbpath == NULL)
		load_paths();

	if ((db = db_init(euconfig.dbpath)) == NULL)
		return 1;

	ret = 1;
	if (opta) {
		if (db_load_artist_expr(db, expr, &song_list))
			ret = play_songs(song_list, optc, optr);
		g_slist_free(song_list);
		sqlite3_close(db);
		return ret;
	}
	if (optA) {
		if (db_load_album_expr(db, expr, &song_list))
			ret = play_songs(song_list, optc, optr);
		g_slist_free(song_list);
		sqlite3_close(db);
		return ret;
	}
	if (optg) {
		if (db_load_genre_expr(db, expr, &song_list))
			ret = play_songs(song_list, optc, optr);
		g_slist_free(song_list);
		sqlite3_close(db);
		return 1;
	}

	if (db_load_song_expr(db, expr, &song_list))
		ret = play_songs(song_list, optc, optr);
	g_slist_free(song_list);
	sqlite3_close(db);
	return ret;
}
