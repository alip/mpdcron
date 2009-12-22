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

#ifndef MPDCRON_GUARD_NOTIFICATION_DEFS_H
#define MPDCRON_GUARD_NOTIFICATION_DEFS_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* !HAVE_CONFIG_H */

#define MPDCRON_EVENT_PLAYER 1
#include "../gmodule.h"

#include <glib.h>

#define NOTIFICATION_LOG_PREFIX		"[notification] "

extern int optnd;

char *cover_find(const char *path, const char *suffix, const char *artist, const char *album);

void mcnotify_send(char **hints, const char *urgency, const char *timeout,
		const char *type, const char *cover,
		const char *artist, const char *title,
		const char *album, const char *uri);

#endif /* !MPDCRON_GUARD_NOTIFICATION_DEFS_H */
