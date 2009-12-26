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

#include "../utils.h"

void load_paths(void)
{
	GKeyFile *cfd;
	GError *config_err = NULL;

	/* Get home directory */
	if (g_getenv(ENV_HOME_DIR))
		euconfig.homepath = g_strdup(g_getenv(ENV_HOME_DIR));
	else if (g_getenv("HOME"))
		euconfig.homepath = g_build_filename(g_getenv("HOME"), DOT_MPDCRON, NULL);
	else {
		eulog(LOG_ERR, "Neither "ENV_HOME_DIR" nor HOME is set, exiting!");
		exit(1);
	}

	/* Set keyfile path */
	euconfig.confpath = g_build_filename(euconfig.homepath, PACKAGE".conf", NULL);

	/* Load keyfile */
	cfd = g_key_file_new();
	if (!g_key_file_load_from_file(cfd, euconfig.confpath, G_KEY_FILE_NONE, &config_err)) {
		switch (config_err->code) {
			case G_FILE_ERROR_NOENT:
			case G_KEY_FILE_ERROR_NOT_FOUND:
				eulog(LOG_DEBUG, "Configuration file `%s' not found, skipping",
					euconfig.confpath);
				g_error_free(config_err);
				g_key_file_free(cfd);
				break;
			default:
				eulog(LOG_ERR, "Failed to parse configuration file `%s': %s",
					euconfig.confpath, config_err->message);
				g_error_free(config_err);
				g_key_file_free(cfd);
				exit(1);
		}
	}

	if (!load_string(cfd, MPDCRON_MODULE, "dbpath", false, &euconfig.dbpath))
		exit(1);
	g_key_file_free(cfd);

	if (euconfig.dbpath == NULL)
		euconfig.dbpath = g_build_filename(euconfig.homepath, "stats.db", NULL);
}

struct mpd_song *load_current_song(void)
{
	struct mpd_connection *conn;
	struct mpd_song *song;

	if ((conn = mpd_connection_new(euconfig.hostname, atoi(euconfig.port), 0)) == NULL) {
		eulog(LOG_ERR, "Error creating mpd connection: out of memory");
		return NULL;
	}

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
		eulog(LOG_ERR, "Failed to connect to Mpd: %s",
			mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		return NULL;
	}

	if (euconfig.password != NULL) {
		if (!mpd_run_password(conn, euconfig.password)) {
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

	mpd_connection_free(conn);
	return song;
}

int load_songs(GSList *list, bool clear)
{
	int count;
	GSList *walk;
	struct mpd_connection *conn;

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

	if (!mpd_command_list_begin(conn, false)) {
		eulog(LOG_ERR, "Failed to begin command list: %s",
				mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		return 1;
	}

	if (clear)
		mpd_send_clear(conn);

	for (walk = list, count = 0; walk != NULL; walk = g_slist_next(walk), ++count) {
		printf("%s\n", (char *)walk->data);
		mpd_send_add(conn, walk->data);
		g_free(walk->data);
	}

	if (!mpd_command_list_end(conn) || !mpd_response_finish(conn)) {
		eulog(LOG_ERR, "Failed to end command list: %s",
				mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		return 1;
	}

	eulog(LOG_INFO, "Loaded %d songs", count);
	mpd_connection_free(conn);
	return 0;
}
