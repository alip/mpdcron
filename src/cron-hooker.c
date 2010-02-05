/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009, 2010 Ali Polatel <alip@exherbo.org>
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

#include <string.h>

#include <glib.h>

/* Count the number of hook calls */
static struct hook_calls {
	const char *name;
	const char *env;
	unsigned int ncalls;
} calls[] = {
	{"database",		"MC_CALLS_DATABASE",		0},
	{"stored_playlist",	"MC_CALLS_STORED_PLAYLIST",	0},
	{"queue",		"MC_CALLS_QUEUE",		0},
	{"playlist",		"MC_CALLS_PLAYLIST",		0},
	{"player",		"MC_CALLS_PLAYER",		0},
	{"mixer",		"MC_CALLS_MIXER",		0},
	{"output",		"MC_CALLS_OUTPUT",		0},
	{"options",		"MC_CALLS_OPTIONS",		0},
	{"update",		"MC_CALLS_UPDATE",		0},
	{NULL,			NULL,				0},
};

static void
hooker_increment(const char *name)
{
	char *envstr;

	for (unsigned int i = 0; calls[i].name != NULL; i++) {
		if (strcmp(name, calls[i].name) == 0) {
			envstr = g_strdup_printf("%u", ++calls[i].ncalls);
			g_setenv(calls[i].env, envstr, 1);
			g_debug("Setting environment variable %s=%s", calls[i].env, envstr);
			g_free(envstr);
			break;
		}
	}
}

int
hooker_run_hook(const char *name)
{
	gchar **myargv;
	GError *error;

	hooker_increment(name);

	myargv = g_malloc(2 * sizeof(gchar *));
	myargv[0] = g_build_filename(DOT_HOOKS, name, NULL);
	myargv[1] = NULL;

	g_debug("Running hook: %s home directory: %s", myargv[0], conf.home_path);

	error = NULL;
	if (!g_spawn_async(conf.home_path, myargv, NULL,
				G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_CHILD_INHERITS_STDIN,
				NULL, NULL, NULL, &error)) {
		if (error->code != G_SPAWN_ERROR_NOENT && error->code != G_SPAWN_ERROR_NOEXEC)
			g_warning("Failed to execute hook %s: %s", name, error->message);
		else
			g_debug("Failed to execute hook %s: %s", name, error->message);
		g_free(myargv[0]);
		g_free(myargv);
		g_error_free(error);
		return -1;
	}
	g_free(myargv[0]);
	g_free(myargv);
	return 0;
}
