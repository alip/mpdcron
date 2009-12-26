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
 * Place, Suite 330, Boston, MA  021111307  USA
 */

#include "eugene-defs.h"

#include <stdbool.h>
#include <stdlib.h>

#include <glib.h>
#include <mpd/client.h>
#include <sqlite3.h>

static int optv = 0;

static GOptionEntry options[] = {
	{"verbose", 'v', 0, G_OPTION_ARG_NONE, &optv, "Be verbose", NULL},
	{"dbpath", 'd', 0, G_OPTION_ARG_FILENAME, &euconfig.dbpath, "Path to the database", NULL},
	{ NULL, 0, 0, 0, NULL, NULL, NULL },
};

static int run_update(sqlite3 *db, const char *path)
{
	int nsongs;
	struct mpd_connection *conn;
	struct mpd_entity *entity;
	const struct mpd_song *song;

	if ((conn = mpd_connection_new(euconfig.hostname, atoi(euconfig.port), 0)) == NULL) {
		eulog(LOG_ERR, "Error creating mpd connection: out of memory");
		return 1;
	}

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
		eulog(LOG_ERR, "Failed to connect to Mpd: %s",
				mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		return 1;
	}

	if (euconfig.password != NULL) {
		if (!mpd_run_password(conn, euconfig.password)) {
			eulog(LOG_ERR, "Authentication failed: %s",
					mpd_connection_get_error_message(conn));
			mpd_connection_free(conn);
			return 1;
		}
	}

	if (!mpd_send_list_all_meta(conn, path)) {
		eulog(LOG_ERR, "Failed to list Mpd database: %s",
				mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		return 1;
	}

	nsongs = 0;
	while ((entity = mpd_recv_entity(conn)) != NULL) {
		if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
			song = mpd_entity_get_song(entity);
			if (!db_process(db, song, false))
				return 1;
			++nsongs;
		}
		mpd_entity_free(entity);
	}

	mpd_response_finish(conn);
	mpd_connection_free(conn);
	return nsongs;
}

int cmd_update(int argc, char **argv)
{
	int ret;
	char *errmsg;
	GOptionContext *ctx;
	GError *parse_err = NULL;
	sqlite3 *db;

	ctx = g_option_context_new("[PATH ...]");
	g_option_context_add_main_entries(ctx, options, "eugene-update");
	g_option_context_set_summary(ctx, "eugene-update-"VERSION GITHEAD" - Create/Update database");

	if (!g_option_context_parse(ctx, &argc, &argv, &parse_err)) {
		g_printerr("Option parsing failed: %s\n", parse_err->message);
		g_error_free(parse_err);
		g_option_context_free(ctx);
		return 1;
	}
	g_option_context_free(ctx);

	if (optv)
		euconfig.verbosity = LOG_DEBUG;

	if (euconfig.dbpath == NULL)
		load_paths();

	if ((db = db_init(euconfig.dbpath)) == NULL)
		return 1;

	if (sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, &errmsg) != SQLITE_OK) {
		eulog(LOG_ERR, "Failed to begin transaction: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return 1;
	}

	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			eulog(LOG_NOTICE, "Updating %s", argv[i]);
			if ((ret = run_update(db, argv[i])) < 0)
				break;
		}
	}
	else {
		eulog(LOG_NOTICE, "Updating /");
		ret = run_update(db, NULL);
	}

	if (sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, &errmsg) != SQLITE_OK) {
		eulog(LOG_ERR, "Failed to end transaction: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return 1;
	}
	sqlite3_close(db);

	if (ret > 0) {
		eulog(LOG_NOTICE, "Successfully processed %d songs", ret);
		return 0;
	}
	return 1;
}
