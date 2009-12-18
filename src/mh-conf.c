/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet ai cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
 *
 * This file is part of the mpdhooker mpd client. mpdhooker is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * mpdhooker is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mh-defs.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <libdaemon/dlog.h>
#include <libdaemon/dpid.h>

char *home_path = NULL;
char *conf_path = NULL;
char *pid_path = NULL;

const char *hostname = NULL;
const char *port = NULL;
const char *password = NULL;

const char *mhconf_pid_file_proc(void)
{
	char *name;

	if (pid_path)
		return pid_path;
	name = g_strdup_printf("%s.pid", daemon_pid_file_ident);
	pid_path = g_build_filename(home_path, name, NULL);
	g_free(name);
	return pid_path;
}

int mhconf_init(void)
{
	char *kfname;

	/* Get home directory */
	if (g_getenv(ENV_HOME_DIR))
		home_path = g_strdup(g_getenv(ENV_HOME_DIR));
	else if (g_getenv("HOME"))
		home_path = g_build_filename(g_getenv("HOME"), DOT_MPDHOOKER, NULL);
	else {
		mh_log(LOG_ERR, "Neither "ENV_HOME_DIR" nor HOME is set, exiting!");
		return -1;
	}

	/* Set keyfile path */
	kfname = g_strdup_printf("%s.conf", daemon_pid_file_ident);
	conf_path = g_build_filename(home_path, kfname, NULL);
	g_free(kfname);

	/* Get Mpd host, port, password */
	if ((hostname = g_getenv(ENV_MPD_HOST)) == NULL)
		hostname = "localhost";
	if ((port = g_getenv(ENV_MPD_PORT)) == NULL)
		port = "6600";
	password = g_getenv(ENV_MPD_PASSWORD);

	return 0;
}

void mhconf_free(void)
{
	g_free(conf_path);
	g_free(home_path);
	g_free(pid_path);
}
