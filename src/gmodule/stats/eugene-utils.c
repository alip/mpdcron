/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009, 2010, 2016 Ali Polatel <alip@exherbo.org>
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
#include "walrus-defs.h" /* To add current song to the database. */

#include <stdlib.h>
#include <syslog.h>

#include <glib.h>
#include <mpd/client.h>

char *
quote(const char *src)
{
	const char *p;
	GString *dest;

	g_return_val_if_fail(src != NULL, NULL);

	dest = g_string_new("'");
	p = src;

	while (*p != 0) {
		if (*p == '\'')
			g_string_append(dest, "''");
		else
			g_string_append_c(dest, *p);
		++p;
	}
	g_string_append_c(dest, '\'');
	return g_string_free(dest, FALSE);
}

struct mpd_song *
load_current_song(void)
{
	int port;
	const char *hostname, *password;
	struct mpd_connection *conn;
	struct mpd_song *song;

	char *dbpath;
	GError *error;

	hostname = g_getenv(ENV_MPD_HOST)
		? g_getenv(ENV_MPD_HOST)
		: "localhost";
	port = g_getenv(ENV_MPD_PORT)
		? atoi(g_getenv(ENV_MPD_PORT))
		: 6600;
	password = g_getenv(ENV_MPD_PASSWORD);

	if ((conn = mpd_connection_new(hostname, port, 0)) == NULL) {
		eulog(LOG_ERR, "Error creating mpd connection: out of memory");
		return NULL;
	}

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
		eulog(LOG_ERR, "Failed to connect to Mpd: %s",
			mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		return NULL;
	}

	if (password != NULL) {
		if (!mpd_run_password(conn, password)) {
			eulog(LOG_ERR, "Authentication failed: %s",
				mpd_connection_get_error_message(conn));
			mpd_connection_free(conn);
			return NULL;
		}
	}

	if ((song = mpd_run_current_song(conn)) == NULL) {
		if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
			eulog(LOG_ERR, "Failed to get current song: %s",
				mpd_connection_get_error_message(conn));
			mpd_connection_free(conn);
			return NULL;
		}
		eulog(LOG_WARNING, "No song playing at the moment");
		mpd_connection_free(conn);
		return NULL;
	}

	if (!getenv("EUGENE_NODB")) {
		/* ^^ Eugene may be remote to the mpdcron stats database, so let
		 * the user skip this if need be.
		 * TODO: Make this a server command. */
		if (!db_initialized()) {
			dbpath = xload_dbpath();

			error = NULL;
			if (!db_init(dbpath, true, false, &error)) {
				g_printerr("Failed to load database `%s': %s\n", dbpath, error->message);
				g_error_free(error);
				g_free(dbpath);
			}
			g_free(dbpath);
		}

		error = NULL;
		if (db_initialized() && !db_process(song, false, -1, &error)) {
			g_printerr("Failed to process song %s: %s\n",
					mpd_song_get_uri(song),
					error->message);
			g_error_free(error);
		}
	}

	mpd_connection_free(conn);
	return song;
}
