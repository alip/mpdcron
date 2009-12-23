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

#include <fnmatch.h>
#include <stdbool.h>
#include <string.h>

#include <glib.h>
#include <libdaemon/dlog.h>

bool pattern_check(const char *name, int len)
{
	char *abspath;

	if (len <= 0) {
		/* event->len <= 0, means we didn't get an event name. */
		return true;
	}
	else if (file_config.patterns == NULL)
		return true;

	/* We only check files for patterns not directories.
	 * So check if what we have is a directory.
	 */
	abspath = g_build_filename(file_config.root, name, NULL);
	if (g_file_test(abspath, G_FILE_TEST_IS_DIR)) {
		daemon_log(LOG_DEBUG, "%sname: `%s' is a directory, skipping pattern check",
				INOTIFY_LOG_PREFIX, name);
		g_free(abspath);
		return true;
	}
	g_free(abspath);

	daemon_log(LOG_DEBUG, "%sinotify.patterns defined, checking for patterns",
			INOTIFY_LOG_PREFIX);
	for (unsigned int i = 0; file_config.patterns[i] != NULL; i++) {
		if (fnmatch(file_config.patterns[i], name, FNM_PATHNAME) == 0) {
			daemon_log(LOG_DEBUG, "%sname: `%s' matches pattern: `%s'",
					INOTIFY_LOG_PREFIX, name,
					file_config.patterns[i]);
			return true;
		}
	}
	daemon_log(LOG_DEBUG, "%sname: `%s' doesn't match any pattern",
			INOTIFY_LOG_PREFIX, name);
	return false;
}
