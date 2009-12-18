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

#include <glib.h>
#include <libdaemon/dlog.h>

int mhhooker_run_hook(const char *name)
{
	int pid;
	gchar **myargv;
	GError *hook_err = NULL;

	myargv = g_malloc(2 * sizeof(gchar *));
	myargv[0] = g_build_filename(DOT_HOOKS, name, NULL);
	myargv[1] = NULL;

	mh_logv(LOG_DEBUG, "Running hook: %s home directory: %s", myargv[0], home_path);
	if (!g_spawn_async(home_path, myargv, NULL,
				G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_CHILD_INHERITS_STDIN,
				NULL, NULL, &pid, &hook_err)) {
		if (hook_err->code != G_SPAWN_ERROR_NOENT && hook_err->code != G_SPAWN_ERROR_NOEXEC)
			mh_log(LOG_WARNING, "Failed to execute hook %s: %s", name, hook_err->message);
		g_free(myargv[0]);
		g_free(myargv);
		g_error_free(hook_err);
		return -1;
	}
	g_free(myargv[0]);
	g_free(myargv);
	return 0;
}
