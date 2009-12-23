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

#ifndef MPDCRON_GUARD_INOTIFY_DEFS_H
#define MPDCRON_GUARD_INOTIFY_DEFS_H

#define MPDCRON_EVENT_GENERIC 1
#include "../gmodule.h"

#include <sys/inotify.h>

#include <glib.h>

#define INOTIFY_LOG_PREFIX		"[inotify] "
#define INOTIFY_MAX_DIRS		8192
#define INOTIFY_EVENTS			(\
		IN_CLOSE_WRITE |\
		IN_CREATE |\
		IN_DELETE |\
		IN_MOVE_SELF |\
		IN_MOVED_FROM |\
		IN_MOVED_TO)

struct config {
	int events;
	int limit;
	char *root;
	char **patterns;
	GHashTable *directories;
};

extern int ifd;
extern struct config file_config;

int file_load(GKeyFile *fd);
void file_cleanup(void);

bool pattern_check(const char *name, int len);

bool update_mpd(const char *path);

#endif /* !MPDCRON_GUARD_INOTIFY_DEFS_H */
