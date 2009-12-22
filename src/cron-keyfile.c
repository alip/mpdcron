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

#include <glib.h>
#include <mpd/client.h>

int timeout = DEFAULT_MPD_TIMEOUT;
int reconnect = DEFAULT_MPD_RECONNECT;
int killwait = DEFAULT_PID_KILL_WAIT;
enum mpd_idle idle = 0;

int keyfile_load(
#ifndef HAVE_MODULE
		G_GNUC_UNUSED
#endif /* !HAVE_MODULE */
		int load_modules)
{
	GKeyFile *config_fd;
	GError *config_err = NULL;
	char *optstr;
	char **events;
#ifdef HAVE_MODULE
	char **modules;
#endif /* HAVE_MODULE */

	config_fd = g_key_file_new();
	if (!g_key_file_load_from_file(config_fd, conf_path, G_KEY_FILE_NONE, &config_err)) {
		switch (config_err->code) {
			case G_FILE_ERROR_NOENT:
			case G_KEY_FILE_ERROR_NOT_FOUND:
				daemon_log(LOG_DEBUG, "Configuration file `%s' not found, skipping", conf_path);
				g_error_free(config_err);
				g_key_file_free(config_fd);
				return 0;
			default:
				daemon_log(LOG_ERR, "Failed to parse configuration file `%s': %s",
						conf_path, config_err->message);
				g_error_free(config_err);
				g_key_file_free(config_fd);
				return -1;
		}
	}

	/* Get main.pidfile */
	if (pid_path == NULL)
		pid_path = g_key_file_get_string(config_fd, "main", "pidfile", NULL);

	/* Get main.killwait */
	config_err = NULL;
	killwait = g_key_file_get_integer(config_fd, "main", "killwait", &config_err);
	if (config_err != NULL) {
		switch (config_err->code) {
			case G_KEY_FILE_ERROR_INVALID_VALUE:
				daemon_log(LOG_WARNING, "main.killwait not an integer: %s", config_err->message);
				g_error_free(config_err);
				g_key_file_free(config_fd);
				return -1;
			default:
				g_error_free(config_err);
				config_err = NULL;
				killwait = DEFAULT_PID_KILL_WAIT;
				break;
		}
	}

	if (killwait <= 0) {
		daemon_log(LOG_WARNING, "killwait smaller than zero, adjusting to default %d", DEFAULT_PID_KILL_WAIT);
		killwait = DEFAULT_PID_KILL_WAIT;
	}

	/* Get mpd.reconnect */
	config_err = NULL;
	reconnect = g_key_file_get_integer(config_fd, "mpd", "reconnect", &config_err);
	if (config_err != NULL) {
		switch (config_err->code) {
			case G_KEY_FILE_ERROR_INVALID_VALUE:
				daemon_log(LOG_WARNING, "mpd.reconnect not an integer: %s", config_err->message);
				g_error_free(config_err);
				g_key_file_free(config_fd);
				return -1;
			default:
				g_error_free(config_err);
				config_err = NULL;
				reconnect = DEFAULT_MPD_RECONNECT;
				break;
		}
	}

	if (reconnect <= 0) {
		daemon_log(LOG_WARNING, "reconnect %s zero, adjusting to default %d",
				(reconnect == 0) ? "equal to" : "smaller than",
				DEFAULT_MPD_RECONNECT);
		reconnect = DEFAULT_MPD_RECONNECT;
	}

	optstr = g_strdup_printf("%d", reconnect);
	g_setenv("MCOPT_RECONNECT", optstr, 1);
	g_free(optstr);

	/* Get mpd.timeout */
	config_err = NULL;
	timeout = g_key_file_get_integer(config_fd, "mpd", "timeout", &config_err);
	if (config_err != NULL) {
		switch (config_err->code) {
			case G_KEY_FILE_ERROR_INVALID_VALUE:
				daemon_log(LOG_WARNING, "mpd.timeout not an integer: %s", config_err->message);
				g_error_free(config_err);
				g_key_file_free(config_fd);
				return -1;
			default:
				g_error_free(config_err);
				config_err = NULL;
				timeout = DEFAULT_MPD_TIMEOUT;
				break;
		}
	}

	if (timeout < 0) {
		daemon_log(LOG_WARNING, "timeout smaller than zero, adjusting to default %d", DEFAULT_MPD_TIMEOUT);
		timeout = DEFAULT_MPD_TIMEOUT;
	}

	optstr = g_strdup_printf("%d", timeout);
	g_setenv("MCOPT_TIMEOUT", optstr, 1);
	g_free(optstr);

	/* Get mpd.events */
	if ((events = g_key_file_get_string_list(config_fd, "mpd", "events", NULL, NULL)) != NULL) {
		for (unsigned int i = 0; events[i] != NULL; i++) {
			enum mpd_idle parsed = mpd_idle_name_parse(events[i]);
			if (parsed == 0)
				daemon_log(LOG_WARNING, "Unrecognized idle event: %s", events[i]);
			else
				idle |= parsed;
		}
		g_strfreev(events);
	}

#ifdef HAVE_MODULE
	if (!load_modules) {
		g_key_file_free(config_fd);
		return 0;
	}
	/* Load modules */
	if (idle | MPD_IDLE_DATABASE) {
		if ((modules = g_key_file_get_string_list(config_fd,
						mpd_idle_name(MPD_IDLE_DATABASE), "modules",
						NULL, NULL)) != NULL) {
			for (unsigned int i = 0; modules[i] != NULL; i++)
				module_load(MPD_IDLE_DATABASE, modules[i], config_fd);
			g_strfreev(modules);
		}
	}
	if (idle | MPD_IDLE_STORED_PLAYLIST) {
		if ((modules = g_key_file_get_string_list(config_fd,
						mpd_idle_name(MPD_IDLE_STORED_PLAYLIST), "modules",
						NULL, NULL)) != NULL) {
			for (unsigned int i = 0; modules[i] != NULL; i++)
				module_load(MPD_IDLE_STORED_PLAYLIST, modules[i], config_fd);
			g_strfreev(modules);
		}
	}
	if (idle | MPD_IDLE_QUEUE) {
		if ((modules = g_key_file_get_string_list(config_fd,
						mpd_idle_name(MPD_IDLE_QUEUE), "modules",
						NULL, NULL)) != NULL) {
			for (unsigned int i = 0; modules[i] != NULL; i++)
				module_load(MPD_IDLE_QUEUE, modules[i], config_fd);
			g_strfreev(modules);
		}
	}
	if (idle | MPD_IDLE_PLAYER) {
		if ((modules = g_key_file_get_string_list(config_fd,
						mpd_idle_name(MPD_IDLE_PLAYER), "modules",
						NULL, NULL)) != NULL) {
			for (unsigned int i = 0; modules[i] != NULL; i++)
				module_load(MPD_IDLE_PLAYER, modules[i], config_fd);
			g_strfreev(modules);
		}
	}
	if (idle | MPD_IDLE_MIXER) {
		if ((modules = g_key_file_get_string_list(config_fd,
						mpd_idle_name(MPD_IDLE_MIXER), "modules",
						NULL, NULL)) != NULL) {
			for (unsigned int i = 0; modules[i] != NULL; i++)
				module_load(MPD_IDLE_MIXER, modules[i], config_fd);
			g_strfreev(modules);
		}
	}
	if (idle | MPD_IDLE_OUTPUT) {
		if ((modules = g_key_file_get_string_list(config_fd,
						mpd_idle_name(MPD_IDLE_OUTPUT), "modules",
						NULL, NULL)) != NULL) {
			for (unsigned int i = 0; modules[i] != NULL; i++)
				module_load(MPD_IDLE_OUTPUT, modules[i], config_fd);
			g_strfreev(modules);
		}
	}
	if (idle | MPD_IDLE_OPTIONS) {
		if ((modules = g_key_file_get_string_list(config_fd,
						mpd_idle_name(MPD_IDLE_OPTIONS), "modules",
						NULL, NULL)) != NULL) {
			for (unsigned int i = 0; modules[i] != NULL; i++)
				module_load(MPD_IDLE_OPTIONS, modules[i], config_fd);
			g_strfreev(modules);
		}
	}
	if (idle | MPD_IDLE_UPDATE) {
		if ((modules = g_key_file_get_string_list(config_fd,
						mpd_idle_name(MPD_IDLE_UPDATE), "modules",
						NULL, NULL)) != NULL) {
			for (unsigned int i = 0; modules[i] != NULL; i++)
				module_load(MPD_IDLE_UPDATE, modules[i], config_fd);
			g_strfreev(modules);
		}
	}
#endif /* HAVE_MODULE */

	g_key_file_free(config_fd);
	return 0;
}
