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

#include <glib.h>
#include <mpd/client.h>

int
keyfile_load(GKeyFile **cfd_r)
{
	char *optstr;
	char **events;
	GError *error;

	error = NULL;
	if (!g_key_file_load_from_file(*cfd_r, conf.conf_path, G_KEY_FILE_NONE, &error)) {
		switch (error->code) {
		case G_FILE_ERROR_NOENT:
		case G_KEY_FILE_ERROR_NOT_FOUND:
			g_debug("Configuration file `%s' not found, skipping",
					conf.conf_path);
			g_error_free(error);

			/* Set defaults */
			conf.killwait = DEFAULT_PID_KILL_WAIT;
			conf.log_level = DEFAULT_LOG_LEVEL;
			conf.reconnect = DEFAULT_MPD_RECONNECT;
			conf.timeout = DEFAULT_MPD_TIMEOUT;

			return 0;
		default:
			g_critical("Failed to parse configuration file `%s': %s",
					conf.conf_path, error->message);
			g_error_free(error);
			return -1;
		}
	}

	/* Get main.pidfile */
	if (conf.pid_path == NULL)
		conf.pid_path = g_key_file_get_string(*cfd_r, "main", "pidfile", NULL);

	/* Get main.killwait */
	error = NULL;
	conf.killwait = g_key_file_get_integer(*cfd_r, "main", "killwait", &error);
	if (error != NULL) {
		switch (error->code) {
		case G_KEY_FILE_ERROR_INVALID_VALUE:
			g_warning("main.killwait not an integer: %s", error->message);
			g_error_free(error);
			return -1;
		default:
			g_error_free(error);
			conf.killwait = DEFAULT_PID_KILL_WAIT;
			break;
		}
	}

	if (conf.killwait <= 0) {
		g_warning("killwait smaller than zero, adjusting to default %d",
				DEFAULT_PID_KILL_WAIT);
		conf.killwait = DEFAULT_PID_KILL_WAIT;
	}

	/* Get main.log_level */
	error = NULL;
	conf.log_level = g_key_file_get_integer(*cfd_r, "main", "log_level", &error);
	if (error != NULL) {
		switch (error->code) {
		case G_KEY_FILE_ERROR_INVALID_VALUE:
			g_warning("main.log_level not an integer: %s", error->message);
			g_error_free(error);
			return -1;
		default:
			g_error_free(error);
			conf.log_level = DEFAULT_LOG_LEVEL;
			break;
		}
	}

	/* Get mpd.reconnect */
	error = NULL;
	conf.reconnect = g_key_file_get_integer(*cfd_r, "mpd", "reconnect", &error);
	if (error != NULL) {
		switch (error->code) {
		case G_KEY_FILE_ERROR_INVALID_VALUE:
			g_debug("mpd.reconnect not an integer: %s", error->message);
			g_error_free(error);
			return -1;
		default:
			g_error_free(error);
			conf.reconnect = DEFAULT_MPD_RECONNECT;
			break;
		}
	}

	if (conf.reconnect <= 0) {
		g_warning("reconnect %s zero, adjusting to default %d",
			(conf.reconnect == 0) ? "equal to" : "smaller than",
			DEFAULT_MPD_RECONNECT);
		conf.reconnect = DEFAULT_MPD_RECONNECT;
	}

	optstr = g_strdup_printf("%d", conf.reconnect);
	g_setenv("MCOPT_RECONNECT", optstr, 1);
	g_free(optstr);

	/* Get mpd.timeout */
	error = NULL;
	conf.timeout = g_key_file_get_integer(*cfd_r, "mpd", "timeout", &error);
	if (error != NULL) {
		switch (error->code) {
		case G_KEY_FILE_ERROR_INVALID_VALUE:
			g_warning("mpd.timeout not an integer: %s", error->message);
			g_error_free(error);
			return -1;
		default:
			g_error_free(error);
			conf.timeout = DEFAULT_MPD_TIMEOUT;
			break;
		}
	}

	if (conf.timeout < 0) {
		g_warning("timeout smaller than zero, adjusting to default %d",
				DEFAULT_MPD_TIMEOUT);
		conf.timeout = DEFAULT_MPD_TIMEOUT;
	}

	optstr = g_strdup_printf("%d", conf.timeout);
	g_setenv("MCOPT_TIMEOUT", optstr, 1);
	g_free(optstr);

	/* Get mpd.events */
	if ((events = g_key_file_get_string_list(*cfd_r, "mpd", "events", NULL, NULL)) != NULL) {
		for (unsigned int i = 0; events[i] != NULL; i++) {
			enum mpd_idle parsed = mpd_idle_name_parse(events[i]);
			if (parsed == 0)
				g_warning("Unrecognized idle event: %s", events[i]);
			else
				conf.idle |= parsed;
		}
		g_strfreev(events);
	}

	return 0;
}

#ifdef HAVE_GMODULE
int
keyfile_load_modules(GKeyFile **cfd_r)
{
	char **modules;

	g_assert(*cfd_r != NULL);

	/* Load modules */
	if ((modules = g_key_file_get_string_list(*cfd_r, "main", "modules", NULL, NULL)) != NULL) {
		for (unsigned int i = 0; modules[i] != NULL; i++)
			module_load(modules[i], *cfd_r);
		g_strfreev(modules);
	}
	return 0;
}

#endif /* HAVE_GMODULE */
