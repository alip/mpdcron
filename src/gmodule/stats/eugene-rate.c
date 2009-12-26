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

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#include <glib.h>
#include <mpd/client.h>

static int optv = 0;
static int optsum = 0;
static int optsubs = 0;
static int opta = 0;
static int optA = 0;
static int optg = 0;
static char *expr = NULL;

static GOptionEntry options[] = {
	{"verbose", 'v', 0, G_OPTION_ARG_NONE, &optv, "Be verbose", NULL},
	{"dbpath", 'd', 0, G_OPTION_ARG_FILENAME, &euconfig.dbpath, "Path to the database", NULL},
	{"expr", 'e', 0, G_OPTION_ARG_STRING, &expr, "Love songs matching the given expression", NULL},
	{"add", 'a', 0, G_OPTION_ARG_NONE, &optsum, "Add the given rating, instead of setting", NULL},
	{"substract", 's', 0, G_OPTION_ARG_NONE, &optsubs, "Substract the given rating, instead of setting", NULL},
	{"artist", 0, 0, G_OPTION_ARG_NONE, &opta, "Rate artist instead of song", NULL},
	{"album", 0, 0, G_OPTION_ARG_NONE, &optA, "Rate album instead of song", NULL},
	{"genre", 0, 0, G_OPTION_ARG_NONE, &optg, "Rate genre instead of song", NULL},
	{ NULL, -1, 0, 0, NULL, NULL, NULL },
};

int cmd_rate(int argc, char **argv)
{
	bool ret;
	long rating;
	char *endptr;
	GOptionContext *ctx;
	GError *parse_err = NULL;
	struct mpd_song *song;

	ctx = g_option_context_new("RATING");
	g_option_context_add_main_entries(ctx, options, "eugene-rate");
	g_option_context_set_summary(ctx, "eugene-rate-"VERSION GITHEAD" - Rate song");
	g_option_context_set_description(ctx, ""
"By default this command works on the current playing song.\n"
"For more information about the expression syntax, see:\n"
"http://www.sqlite.org/lang_expr.html");
	if (!g_option_context_parse(ctx, &argc, &argv, &parse_err)) {
		g_printerr("Option parsing failed: %s\n", parse_err->message);
		g_error_free(parse_err);
		g_option_context_free(ctx);
		return -1;
	}
	g_option_context_free(ctx);

	if (argc < 2) {
		g_printerr("No rating given\n");
		return -1;
	}

	errno = 0;
	rating = strtoul(argv[1], &endptr, 10);
	if ((errno == ERANGE && (rating == LONG_MAX || rating == LONG_MIN))
			|| (errno != 0 && rating == 0)) {
		g_printerr("Failed to convert `%s' to number: %s\n", argv[1], g_strerror(errno));
		return -1;
	}
	if (endptr == argv[1]) {
		g_printerr("Failed to convert `%s' to number: No digits were found\n", argv[1]);
		return -1;
	}

	if (optv)
		euconfig.verbosity = LOG_DEBUG;
	if ((opta && optA && optg) || (opta && optA) || (opta && optg) || (optA && optg)) {
		g_printerr("--artist, --album and --genre options are mutually exclusive\n");
		return -1;
	}
	if (optsum && optsubs) {
		g_printerr("--add and --substract are mutually exclusive options\n");
		return -1;
	}
	else if (optsubs)
		rating = -rating;

	if (euconfig.dbpath == NULL)
		load_paths();

	if (!db_init(euconfig.dbpath))
		return -1;

	if (expr != NULL) {
		if (opta)
			return db_rate_artist_expr(euconfig.dbpath, expr, rating,
					(optsum || optsubs),
					(euconfig.verbosity > LOG_WARNING)) ? 0 : 1;
		if (optA)
			return db_rate_album_expr(euconfig.dbpath, expr, rating,
					(optsum || optsubs),
					(euconfig.verbosity > LOG_WARNING)) ? 0 : 1;
		if (optg)
			return db_rate_genre_expr(euconfig.dbpath, expr, rating,
					(optsum || optsubs),
					(euconfig.verbosity > LOG_WARNING)) ? 0 : 1;
		return db_rate_song_expr(euconfig.dbpath, expr, rating,
				(optsum || optsubs),
				(euconfig.verbosity > LOG_WARNING)) ? 0 : 1;
	}
	if ((song = load_current_song()) == NULL)
		return 1;
	if (opta)
		ret = db_rate_artist(euconfig.dbpath, song, rating,
				(optsum || optsubs),
				(euconfig.verbosity > LOG_WARNING));
	else if (optA)
		ret = db_rate_album(euconfig.dbpath, song, rating,
				(optsum || optsubs),
				(euconfig.verbosity > LOG_WARNING));
	else if (optg)
		ret = db_rate_genre(euconfig.dbpath, song, rating,
				(optsum || optsubs),
				(euconfig.verbosity > LOG_WARNING));
	else
		ret = db_rate_song(euconfig.dbpath, song, rating,
				(optsum || optsubs),
				(euconfig.verbosity > LOG_NOTICE));
	mpd_song_free(song);
	return ret ? 0 : 1;
}
