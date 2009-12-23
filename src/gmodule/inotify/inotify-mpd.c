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

#include "inotify-defs.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <glib.h>
#include <libdaemon/dlog.h>

bool update_mpd(const char *path)
{
	int id;
	const unsigned *version;
	const char *relative_path;
	const char *host, *port, *password;
	struct mpd_connection *conn;

	if ((host = g_getenv("MPD_HOST")) == NULL)
		host = "localhost";
	if ((port = g_getenv("MPD_PORT")) == NULL)
		port = "6600";
	password = g_getenv("MPD_PASSWORD");

	relative_path = path + strlen(file_config.root) + 1;
	if (relative_path[0] == '\0')
		relative_path = NULL;
	daemon_log(LOG_INFO, "%ssending update(%s) to Mpd server %s:%s",
			INOTIFY_LOG_PREFIX,
			relative_path ? relative_path : "/",
			host, port);

	if ((conn = mpd_connection_new(host, atoi(port), 0)) == NULL) {
		daemon_log(LOG_ERR, "%smpd_connection_new failed: out of memory",
				INOTIFY_LOG_PREFIX);
		return false;
	}
	if ((version = mpd_connection_get_server_version(conn)) != NULL)
		daemon_log(LOG_INFO, "%sconnected to Mpd server, running version %d.%d.%d",
				INOTIFY_LOG_PREFIX,
				version[0], version[1], version[2]);
	else
		daemon_log(LOG_INFO, "%sconnected to Mpd server, running version unknown",
				INOTIFY_LOG_PREFIX);

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
		daemon_log(LOG_ERR, "%serror while connecting to mpd: %s",
				INOTIFY_LOG_PREFIX,
				mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		return false;
	}

	if (password != NULL) {
		if (!mpd_run_password(conn, password)) {
			daemon_log(LOG_ERR, "%sauthentication failed: %s",
					INOTIFY_LOG_PREFIX,
					mpd_connection_get_error_message(conn));
			mpd_connection_free(conn);
			return false;
		}
	}

	if ((id = mpd_run_update(conn, relative_path)) < 0) {
		daemon_log(LOG_ERR, "%serror while running update command: %s",
				INOTIFY_LOG_PREFIX,
				mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		/* This error shouldn't be fatal or should it? */
		return true;
	}

	daemon_log(LOG_INFO, "%scommand successful, update id: %d", INOTIFY_LOG_PREFIX, id);
	mpd_connection_free(conn);
	return true;
}
