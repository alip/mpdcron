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
