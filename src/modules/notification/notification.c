/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet ai cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
 * Based in part upon notify-send which is:
 *   Copyright (C) 2004 Christian Hammond.
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
#include <libnotify/notify.h>

static gboolean mcnotify_set_hint_variant(NotifyNotification *notification,
		const gchar *type, const gchar *key,
		const gchar *value, GError **hint_error)
{
	static gboolean conv_error = FALSE;

	if (strcasecmp(type, "string") == 0)
		notify_notification_set_hint_string(notification, key, value);
	else if (strcasecmp(type, "int") == 0) {
		if (!g_ascii_isdigit(*value))
			conv_error = TRUE;
		else {
			gint h_int = (gint)g_ascii_strtoull(value, NULL, 10);
			notify_notification_set_hint_int32(notification, key, h_int);
		}
	}
	else if (strcasecmp(type, "double") == 0) {
		if (!g_ascii_isdigit(*value))
			conv_error = TRUE;
		else {
			gdouble h_double = g_strtod(value, NULL);
			notify_notification_set_hint_double(notification, key, h_double);
		}
	}
	else if (strcasecmp(type, "byte") == 0) {
		gint h_byte = (gint)g_ascii_strtoull(value, NULL, 10);

		if (h_byte < 0 || h_byte > 0xFF)
			conv_error = TRUE;
		else
			notify_notification_set_hint_byte(notification, key, (guchar)h_byte);
	}
	else {
		*hint_error = g_error_new(G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
				"Invalid hint type \"%s\". Valid types "
				"are int, double, string and byte.",
				type);
		return FALSE;
	}

	if (conv_error) {
		*hint_error = g_error_new(G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
				"Value \"%s\" of hint \"%s\" could not be "
				"parsed as type \"%s\".",
				value, key, type);
		return FALSE;
	}

	return TRUE;
}

int mcnotify_init(void)
{
	daemon_log(LOG_DEBUG, "%sinitializing libnotify library", NOTIFICATION_LOG_PREFIX);
	if (notify_init(PACKAGE) == FALSE) {
		daemon_log(LOG_ERR, "%sfailed to initialize libnotify library",
				NOTIFICATION_LOG_PREFIX);
		return -1;
	}
	return 0;
}

void mcnotify_send(char **hints, const char *urgency, const char *timeout,
		const char *type, const char *cover,
		const char *artist, const char *title,
		const char *album, const char *uri)
{
	int len;
	char *body;
	char **tokens = NULL;
	GError *hint_error = NULL;
	NotifyNotification *not;

	body = g_strdup_printf("by %s - %s", artist ? artist : "Unknown",
			album ? album : "Unknown");
	not = notify_notification_new(title ? title : uri, body, cover, NULL);
	g_free(body);

	notify_notification_set_category(not, type);

	if (strcasecmp(timeout, "never") == 0)
		notify_notification_set_timeout(not, NOTIFY_EXPIRES_NEVER);
	else if (strcasecmp(timeout, "default") == 0)
		notify_notification_set_timeout(not, NOTIFY_EXPIRES_DEFAULT);
	else
		notify_notification_set_timeout(not, atoi(timeout));

	if (strcasecmp(urgency, "low") == 0)
		notify_notification_set_urgency(not, NOTIFY_URGENCY_LOW);
	else if (strcasecmp(urgency, "normal") == 0)
		notify_notification_set_urgency(not, NOTIFY_URGENCY_NORMAL);
	else if (strcasecmp(urgency, "critical") == 0)
		notify_notification_set_urgency(not, NOTIFY_URGENCY_CRITICAL);
	else {
		daemon_log(LOG_WARNING, "%sunrecognized urgency `%s', setting to normal",
				NOTIFICATION_LOG_PREFIX,
				urgency);
		notify_notification_set_urgency(not, NOTIFY_URGENCY_NORMAL);
	}

	if (hints != NULL) {
		for (unsigned int i = 0; hints[i] != NULL; i++) {
			tokens = g_strsplit(hints[i], ":", -1);

			if ((len = g_strv_length(tokens)) != 3) {
				daemon_log(LOG_ERR, "%sinvalid hint `%s' specified, use TYPE:NAME:VALUE",
						NOTIFICATION_LOG_PREFIX,
						hints[i]);
				g_strfreev(tokens);
				continue;
			}

			if (!mcnotify_set_hint_variant(not,
						tokens[0], tokens[1], tokens[3], &hint_error)) {
				daemon_log(LOG_WARNING, "%ssetting hint variant `%s' failed: %s",
						NOTIFICATION_LOG_PREFIX,
						hints[i], hint_error->message);
				g_error_free(hint_error);
			}
			g_strfreev(tokens);
		}
	}

	notify_notification_show(not, NULL);
	g_object_unref(G_OBJECT(not));
}
