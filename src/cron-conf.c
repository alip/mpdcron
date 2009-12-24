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

#include "cron-defs.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <libdaemon/dlog.h>
#include <libdaemon/dpid.h>

struct mpdcron_config conf;

const char *conf_pid_file_proc(void)
{
	char *name;

	if (conf.pid_path)
		return conf.pid_path;
	name = g_strdup_printf("%s.pid", daemon_pid_file_ident);
	conf.pid_path = g_build_filename(conf.home_path, name, NULL);
	g_free(name);
	return conf.pid_path;
}

int conf_init(void)
{
	char *kfname;

	memset(&conf, 0, sizeof(struct mpdcron_config));

	/* Get home directory */
	if (g_getenv(ENV_HOME_DIR))
		conf.home_path = g_strdup(g_getenv(ENV_HOME_DIR));
	else if (g_getenv("HOME"))
		conf.home_path = g_build_filename(g_getenv("HOME"), DOT_MPDCRON, NULL);
	else {
		daemon_log(LOG_ERR, "Neither "ENV_HOME_DIR" nor HOME is set, exiting!");
		return -1;
	}

	/* Set keyfile path */
	kfname = g_strdup_printf("%s.conf", daemon_pid_file_ident);
	conf.conf_path = g_build_filename(conf.home_path, kfname, NULL);
	g_free(kfname);

#ifdef HAVE_GMODULE
	/* Set module path */
	conf.mod_path = g_build_filename(conf.home_path, DOT_MODULES, NULL);
#endif /* HAVE_GMODULE */

	/* Get Mpd host, port, password */
	if ((conf.hostname = g_getenv(ENV_MPD_HOST)) == NULL)
		conf.hostname = "localhost";
	if ((conf.port = g_getenv(ENV_MPD_PORT)) == NULL)
		conf.port = "6600";
	conf.password = g_getenv(ENV_MPD_PASSWORD);

	return 0;
}

void conf_free(void)
{
	g_free(conf.home_path);
	g_free(conf.conf_path);
	g_free(conf.pid_path);
#ifdef HAVE_GMODULE
	g_free(conf.mod_path);
#endif /* HAVE_GMODULE */
	memset(&conf, 0, sizeof(struct mpdcron_config));
}
