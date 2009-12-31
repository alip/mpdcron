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

#include <stdlib.h>

#include <glib.h>

#include "../utils.h"

char *
xload_dbpath(void)
{
	char *homepath, *confpath, *dbpath;
	GKeyFile *kf;
	GError *error;

	/* Get home directory */
	if (g_getenv(ENV_HOME_DIR))
		homepath = g_strdup(g_getenv(ENV_HOME_DIR));
	else if (g_getenv("HOME"))
		homepath = g_build_filename(g_getenv("HOME"), DOT_MPDCRON, NULL);
	else {
		g_printerr("Neither "ENV_HOME_DIR" nor HOME is set, exiting!");
		exit(1);
	}

	/* Set keyfile path */
	confpath = g_build_filename(homepath, PACKAGE".conf", NULL);

	/* Load key file */
	dbpath = NULL;
	error = NULL;
	kf = g_key_file_new();
	if (!g_key_file_load_from_file(kf, confpath, G_KEY_FILE_NONE, &error)) {
		switch (error->code) {
			case G_FILE_ERROR_NOENT:
			case G_KEY_FILE_ERROR_NOT_FOUND:
				g_error_free(error);
				g_key_file_free(kf);
				goto skip;
			default:
				g_printerr("Failed to parse configuration file `%s': %s",
					confpath, error->message);
				g_error_free(error);
				g_key_file_free(kf);
				exit(1);
		}
	}

	error = NULL;
	if (!load_string(kf, "stats", "dbpath", false, &dbpath, &error)) {
		g_printerr("%s\n", error->message);
		g_error_free(error);
		g_free(confpath);
		g_free(homepath);
		g_key_file_free(kf);
		exit(1);
	}
	g_key_file_free(kf);

skip:
	if (dbpath == NULL)
		dbpath = g_build_filename(homepath, "stats.db", NULL);
	g_free(confpath);
	g_free(homepath);

	return dbpath;
}
