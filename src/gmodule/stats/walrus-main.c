/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

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

#include "walrus-defs.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <glib.h>
#include <mpd/client.h>

static char *dbpath = NULL;
static int keepgoing = 0;

static GOptionEntry options[] = {
	{"dbpath", 'd', 0, G_OPTION_ARG_FILENAME, &dbpath, "Path to the database", NULL},
	{"keep-going", 'k', 0, G_OPTION_ARG_NONE, &keepgoing, "Keep going in case of database errors", NULL},
	{ NULL, 0, 0, 0, NULL, NULL, NULL },
};

static bool
run_update(int kg, const char *path)
{
	const char *hostname;
	const char *port;
	const char *password;
	GError *error;
	struct mpd_connection *conn;
	struct mpd_entity *entity;
	const struct mpd_song *song;

	if ((hostname = g_getenv(ENV_MPD_HOST)) == NULL)
		hostname = "localhost";
	if ((port = g_getenv(ENV_MPD_PORT)) == NULL)
		port = "6600";
	password = g_getenv(ENV_MPD_PASSWORD);

	if ((conn = mpd_connection_new(hostname, atoi(port), 0)) == NULL) {
		g_printerr("Error creating mpd connection: out of memory\n");
		return false;
	}

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
		g_printerr("Failed to connect to Mpd: %s\n",
				mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		return false;
	}

	if (password != NULL) {
		if (!mpd_run_password(conn, password)) {
			g_printerr("Authentication failed: %s\n",
					mpd_connection_get_error_message(conn));
			mpd_connection_free(conn);
			return false;
		}
	}

	if (!mpd_send_list_all_meta(conn, path)) {
		g_printerr("Failed to list Mpd database: %s\n",
				mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		return false;
	}

	while ((entity = mpd_recv_entity(conn)) != NULL) {
		if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
			song = mpd_entity_get_song(entity);
			error = NULL;
			if (!db_process(song, false, &error)) {
				g_printerr("Failed to process song %s: %s\n",
						mpd_song_get_uri(song),
						error->message);
				g_error_free(error);
				if (kg)
					continue;
				return false;
			}
			printf("%s\n", mpd_song_get_uri(song));
		}
		mpd_entity_free(entity);
	}

	mpd_response_finish(conn);
	mpd_connection_free(conn);
	return true;
}

int
main(int argc, char **argv)
{
	GOptionContext *ctx;
	GError *error;

	ctx = g_option_context_new("[PATH...]");
	g_option_context_add_main_entries(ctx, options, "walrus");
	g_option_context_set_summary(ctx, "walrus-"VERSION GITHEAD" - Create/Update mpdcron-stats database");

	if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
		g_printerr("Option parsing failed: %s\n", error->message);
		g_error_free(error);
		g_option_context_free(ctx);
		return 1;
	}
	g_option_context_free(ctx);

	if (dbpath == NULL)
		dbpath = xload_dbpath();

	error = NULL;
	if (!db_init(dbpath, true, false, &error)) {
		g_printerr("Failed to load database `%s': %s\n", dbpath, error->message);
		g_error_free(error);
		g_free(dbpath);
		return 1;
	}
	g_free(dbpath);

	error = NULL;
	if (!db_start_transaction(&error)) {
		g_printerr("Failed to begin transaction: %s\n", error->message);
		g_error_free(error);
		db_close();
		return 1;
	}

	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			fprintf(stderr, "* Updating %s\n", argv[i]);
			if (!run_update(keepgoing, argv[i])) {
				db_close();
				return 1;
			}
		}
	}
	else {
		fprintf(stderr, "* Updating /\n");
		if (!run_update(keepgoing, NULL)) {
			db_close();
			return 1;
		}
	}

	error = NULL;
	if (!db_end_transaction(&error)) {
		g_printerr("Failed to end transaction: %s\n", error->message);
		g_error_free(error);
		db_close();
		return 1;
	}

	db_close();
	return 0;
}
