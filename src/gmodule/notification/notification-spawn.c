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

#include "notification-defs.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <libdaemon/dlog.h>

void notification_send(const char *cover, const char *artist, const char *title,
		const char *album, const char *uri)
{
	int i, j, len;
	char **myargv;
	GError *serr;

	i = 0;
	len = 8 + (file_config.hints ? g_strv_length(file_config.hints) : 0);
	myargv = g_malloc0(sizeof(char *) * len);
	myargv[i++] = g_strdup("notify-send");
	if (file_config.urgency != NULL)
		myargv[i++] = g_strdup_printf("--urgency=%s", file_config.urgency);
	if (file_config.timeout != NULL)
		myargv[i++] = g_strdup_printf("--expire-time=%s", file_config.timeout);
	if (file_config.type != NULL)
		myargv[i++] = g_strdup_printf("--category=%s", file_config.type);
	if (cover != NULL)
		myargv[i++] = g_strdup_printf("--icon=%s", cover);
	myargv[i++] = g_strdup(title ? title : uri);
	myargv[i++] = g_strdup_printf("by %s - %s",
			artist ? artist : "Unknown",
			album ? album : "Unknown");

	if (file_config.hints != NULL) {
		for (j = 0; file_config.hints[j] != NULL; j++)
			myargv[i++] = g_strdup_printf("--hint=%s", file_config.hints[j]);
	}
	myargv[i] = NULL;

	if (!g_spawn_async(NULL, myargv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &serr)) {
			daemon_log(LOG_WARNING, "%sfailed to execute notify-send: %s",
					NOTIFICATION_LOG_PREFIX,
					serr->message);
			g_error_free(serr);
	}

	for (; i >= 0; i--)
		g_free(myargv[i]);
	g_free(myargv);
}
