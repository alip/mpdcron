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
 * Place, Suite 330, Boston, MA  02111false307  USA
 */

#include "stats-defs.h"

#include <stdbool.h>
#include <string.h>

#include <glib.h>
#include <libdaemon/dlog.h>

#include "../utils.h"

struct config globalconf;

bool
file_load(const struct mpdcron_config *conf, GKeyFile *fd)
{
	char **values;
	GError *error;

	memset(&globalconf, 0, sizeof(struct config));

	/* Load database path */
	error = NULL;
	if (!load_string(fd, MPDCRON_MODULE, "dbpath", false, &globalconf.dbpath, &error)) {
		mpdcron_log(LOG_ERR, "%s", error->message);
		g_error_free(error);
		return false;
	}
	if (globalconf.dbpath == NULL)
		globalconf.dbpath = g_build_filename(conf->home_path, "stats.db", NULL);

	/* Load port */
	error = NULL;
	globalconf.port = -1;
	if (!load_integer(fd, MPDCRON_MODULE, "port", false, &globalconf.port, &error)) {
		mpdcron_log(LOG_ERR, "%s", error->message);
		g_error_free(error);
		return false;
	}
	if (globalconf.port <= 0)
		globalconf.port = DEFAULT_PORT;

	/* Load default permissions */
	error = NULL;
	values = g_key_file_get_string_list(fd, MPDCRON_MODULE, "default_permissions",
			NULL, &error);
	if (error != NULL) {
		switch (error->code) {
			case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
			case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
				g_error_free(error);
				break;
			default:
				mpdcron_log(LOG_ERR, "Failed to load "
						MPDCRON_MODULE".default_permissions: %s",
						error->message);
				g_error_free(error);
				g_free(globalconf.dbpath);
				return false;
		}
	}
	if (values != NULL) {
		for (unsigned int i = 0; values[i] != NULL; i++) {
			if (strncmp(values[i], "select", 7) == 0)
				globalconf.default_permissions |= PERMISSION_SELECT;
			else if (strncmp(values[i], "update", 7) == 0)
				globalconf.default_permissions |= PERMISSION_UPDATE;
			else if (strncmp(values[i], "none", 5) == 0)
				globalconf.default_permissions = 0;
			else
				mpdcron_log(LOG_WARNING, "Invalid value in "
						MPDCRON_MODULE".default_permissions `%s'",
						values[i]);
		}
		g_strfreev(values);
	}
	else
		globalconf.default_permissions = PERMISSION_SELECT | PERMISSION_UPDATE;

	/* Load passwords */
	error = NULL;
	values = g_key_file_get_string_list(fd, MPDCRON_MODULE, "passwords",
			NULL, &error);
	if (error != NULL) {
		switch (error->code) {
			case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
			case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
				g_error_free(error);
				break;
			default:
				mpdcron_log(LOG_ERR, "Failed to load "
						MPDCRON_MODULE".passwords: %s",
						error->message);
				g_error_free(error);
				g_free(globalconf.dbpath);
				return false;
		}
	}

	globalconf.passwords = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	if (values != NULL) {
		char **split;

		for (unsigned int i = 0; values[i] != NULL; i++) {
			split = g_strsplit(values[i], "@", 2);
			if (g_strv_length(split) != 2) {
				mpdcron_log(LOG_WARNING, "Invalid value in "
						MPDCRON_MODULE".passwords `%s'",
						values[i]);
				g_strfreev(split);
				continue;
			}
			if (strncmp(split[1], "select", 7) == 0) {
				g_free(split[1]);
				g_hash_table_insert(globalconf.passwords, split[0],
						GINT_TO_POINTER(PERMISSION_SELECT));
				globalconf.default_permissions = globalconf.default_permissions & ~PERMISSION_SELECT;
			}
			else if (strncmp(split[1], "update", 7) == 0) {
				g_free(split[1]);
				g_hash_table_insert(globalconf.passwords, split[0],
						GINT_TO_POINTER(PERMISSION_UPDATE));
				globalconf.default_permissions = globalconf.default_permissions & ~PERMISSION_UPDATE;
			}
			else if (strncmp(split[1], "all", 4) == 0) {
				g_free(split[1]);
				g_hash_table_insert(globalconf.passwords, split[0],
						GINT_TO_POINTER(PERMISSION_ALL));
				globalconf.default_permissions = globalconf.default_permissions & ~PERMISSION_ALL;
			}
			else {
				mpdcron_log(LOG_WARNING, "Invalid value in "
						MPDCRON_MODULE".passwords `%s'",
						values[i]);
				g_strfreev(split);
			}
		}
		g_strfreev(values);
	}

	/* Load addresses */
	error = NULL;
	globalconf.addrs = g_key_file_get_string_list(fd, MPDCRON_MODULE, "bind_to_addresses", NULL, &error);
	if (error != NULL) {
		switch (error->code) {
			case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
			case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
				g_error_free(error);
				break;
			default:
				mpdcron_log(LOG_ERR, "Failed to load "
						MPDCRON_MODULE".bind_to_address: %s",
						error->message);
				g_error_free(error);
				g_free(globalconf.dbpath);
				return false;
		}
	}
	if (globalconf.addrs == NULL) {
		globalconf.addrs = g_new0(char *, 2);
		globalconf.addrs[0] = g_strdup(DEFAULT_HOST);
	}

	/* Information about Mpd */
	globalconf.mpd_hostname = g_strdup(conf->hostname);
	globalconf.mpd_port = g_strdup(conf->port);
	globalconf.mpd_password = g_strdup(conf->password);

	return true;
}

void
file_cleanup(void)
{
	g_free(globalconf.dbpath);
	g_free(globalconf.mpd_hostname);
	g_free(globalconf.mpd_port);
	g_free(globalconf.mpd_password);
	g_strfreev(globalconf.addrs);
	g_hash_table_destroy(globalconf.passwords);
}
