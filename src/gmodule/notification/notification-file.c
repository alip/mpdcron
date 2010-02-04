/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009, 2010 Ali Polatel <alip@exherbo.org>
 * Based in part upon mpdscribble which is:
 *   Copyright (C) 2008-2009 The Music Player Daemon Project
 *   Copyright (C) 2005-2008 Kuno Woudt <kuno@frob.nl>
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

#include <stdbool.h>
#include <string.h>

#include <glib.h>

#include "../utils.h"

struct config file_config;

int
file_load(GKeyFile *fd)
{
	int event;
	char **values;
	GError *error;

	memset(&file_config, 0, sizeof(struct config));

	error = NULL;
	if (!load_string(fd, MPDCRON_MODULE, "cover_path", false, &file_config.cover_path, &error)) {
		g_critical("Failed to load "MPDCRON_MODULE".cover_path: %s", error->message);
		g_error_free(error);
		return -1;
	}
	if (!load_string(fd, MPDCRON_MODULE, "cover_suffix", false, &file_config.cover_suffix, &error)) {
		g_critical("Failed to load "MPDCRON_MODULE".cover_suffix: %s", error->message);
		g_error_free(error);
		return -1;
	}
	if (!load_string(fd, MPDCRON_MODULE, "timeout", false, &file_config.timeout, &error)) {
		g_critical("Failed to load "MPDCRON_MODULE".timeout: %s", error->message);
		g_error_free(error);
		return -1;
	}
	if (!load_string(fd, MPDCRON_MODULE, "type", false, &file_config.type, &error)) {
		g_critical("Failed to load "MPDCRON_MODULE".type: %s", error->message);
		g_error_free(error);
		return -1;
	}
	if (!load_string(fd, MPDCRON_MODULE, "urgency", false, &file_config.urgency, &error)) {
		g_critical("Failed to load "MPDCRON_MODULE".urgency: %s", error->message);
		g_error_free(error);
		return -1;
	}

	error = NULL;
	file_config.hints = g_key_file_get_string_list(fd, MPDCRON_MODULE, "hints", NULL, &error);
	if (error != NULL) {
		switch (error->code) {
		case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
		case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
			g_error_free(error);
			break;
		default:
			g_critical("Failed to load %s.hints: %s",
					MPDCRON_MODULE, error->message);
			g_error_free(error);
			return -1;
		}
	}

	error = NULL;
	values = g_key_file_get_string_list(fd, MPDCRON_MODULE, "events", NULL, &error);
	if (error != NULL) {
		switch (error->code) {
			case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
			case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
				g_error_free(error);
				break;
			default:
				g_critical("Failed to load "MPDCRON_MODULE".events: %s",
						error->message);
				g_error_free(error);
				return -1;
		}
	}

	if (values != NULL) {
		for (unsigned int i = 0; values[i] != NULL; i++) {
			if ((event = mpd_idle_name_parse(values[i])) < 0)
				g_warning("Invalid value `%s' in "MPDCRON_MODULE".events",
						values[i]);
			else if (event == MPD_IDLE_STORED_PLAYLIST ||
					event == MPD_IDLE_QUEUE ||
					event == MPD_IDLE_OUTPUT)
				g_warning("Event `%s' not a supported event",
						values[i]);
			else
				file_config.events |= event;
		}
		g_strfreev(values);
	}

	if (file_config.events == 0)
		file_config.events = MPD_IDLE_DATABASE | MPD_IDLE_PLAYER |
			MPD_IDLE_MIXER | MPD_IDLE_OPTIONS | MPD_IDLE_UPDATE;

	if (file_config.cover_path == NULL && g_getenv("HOME") != NULL)
		file_config.cover_path = g_build_filename(g_getenv("HOME"), ".covers", NULL);
	if (file_config.cover_suffix == NULL)
		file_config.cover_suffix = g_strdup("jpg");
	return 0;
}

void
file_cleanup(void)
{
	g_free(file_config.cover_path);
	g_free(file_config.cover_suffix);
	g_free(file_config.timeout);
	g_free(file_config.type);
	g_free(file_config.urgency);
	if (file_config.hints != NULL)
		g_strfreev(file_config.hints);
}
