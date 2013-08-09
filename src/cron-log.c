/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2010, 2013 Ali Polatel <alip@exherbo.org>
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

#include <glib.h>
#include <libdaemon/dlog.h>

#include "cron-defs.h"

static void
log_output(const gchar *domain, GLogLevelFlags level,
	const gchar *message)
{
	int dlevel;

	g_return_if_fail(message != NULL && message[0] != '\0');

	switch (level) {
	case G_LOG_LEVEL_CRITICAL:
		dlevel = LOG_CRIT;
		break;
	case G_LOG_LEVEL_WARNING:
		dlevel = LOG_WARNING;
		break;
	case G_LOG_LEVEL_MESSAGE:
		dlevel = LOG_NOTICE;
		break;
	case G_LOG_LEVEL_INFO:
		dlevel = LOG_INFO;
		break;
	case G_LOG_LEVEL_DEBUG:
	default:
		dlevel = LOG_DEBUG;
		break;
	}

	if (domain != NULL)
		daemon_log(dlevel, "[%s] %s", domain, message);
	else
		daemon_log(dlevel, "%s", message);
}

void
log_handler(const gchar *domain, GLogLevelFlags level, const gchar *message,
	gpointer userdata)
{
	int clevel = GPOINTER_TO_INT(userdata);

	if (((level & G_LOG_LEVEL_MESSAGE) && clevel < 1) ||
		((level & G_LOG_LEVEL_INFO) && clevel < 2) ||
		((level & G_LOG_LEVEL_DEBUG) && clevel < 3))
		return;

	log_output(domain, level, message);
}
