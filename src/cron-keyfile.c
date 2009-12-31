/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

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

int
keyfile_load(GKeyFile **cfd_r)
{
	GError *config_err = NULL;
	char *optstr;
	char **events;

	if (!g_key_file_load_from_file(*cfd_r, conf.conf_path, G_KEY_FILE_NONE, &config_err)) {
		switch (config_err->code) {
			case G_FILE_ERROR_NOENT:
			case G_KEY_FILE_ERROR_NOT_FOUND:
				mpdcron_log(LOG_DEBUG, "Configuration file `%s' not found, skipping",
						conf.conf_path);
				g_error_free(config_err);
				return 0;
			default:
				mpdcron_log(LOG_ERR, "Failed to parse configuration file `%s': %s",
						conf.conf_path, config_err->message);
				g_error_free(config_err);
				return -1;
		}
	}

	/* Get main.pidfile */
	if (conf.pid_path == NULL)
		conf.pid_path = g_key_file_get_string(*cfd_r, "main", "pidfile", NULL);

	/* Get main.killwait */
	config_err = NULL;
	killwait = g_key_file_get_integer(*cfd_r, "main", "killwait", &config_err);
	if (config_err != NULL) {
		switch (config_err->code) {
			case G_KEY_FILE_ERROR_INVALID_VALUE:
				mpdcron_log(LOG_WARNING, "main.killwait not an integer: %s", config_err->message);
				g_error_free(config_err);
				return -1;
			default:
				g_error_free(config_err);
				config_err = NULL;
				killwait = DEFAULT_PID_KILL_WAIT;
				break;
		}
	}

	if (killwait <= 0) {
		mpdcron_log(LOG_WARNING, "killwait smaller than zero, adjusting to default %d", DEFAULT_PID_KILL_WAIT);
		killwait = DEFAULT_PID_KILL_WAIT;
	}

	/* Get mpd.reconnect */
	config_err = NULL;
	reconnect = g_key_file_get_integer(*cfd_r, "mpd", "reconnect", &config_err);
	if (config_err != NULL) {
		switch (config_err->code) {
			case G_KEY_FILE_ERROR_INVALID_VALUE:
				mpdcron_log(LOG_WARNING, "mpd.reconnect not an integer: %s", config_err->message);
				g_error_free(config_err);
				return -1;
			default:
				g_error_free(config_err);
				config_err = NULL;
				reconnect = DEFAULT_MPD_RECONNECT;
				break;
		}
	}

	if (reconnect <= 0) {
		mpdcron_log(LOG_WARNING, "reconnect %s zero, adjusting to default %d",
				(reconnect == 0) ? "equal to" : "smaller than",
				DEFAULT_MPD_RECONNECT);
		reconnect = DEFAULT_MPD_RECONNECT;
	}

	optstr = g_strdup_printf("%d", reconnect);
	g_setenv("MCOPT_RECONNECT", optstr, 1);
	g_free(optstr);

	/* Get mpd.timeout */
	config_err = NULL;
	timeout = g_key_file_get_integer(*cfd_r, "mpd", "timeout", &config_err);
	if (config_err != NULL) {
		switch (config_err->code) {
			case G_KEY_FILE_ERROR_INVALID_VALUE:
				mpdcron_log(LOG_WARNING, "mpd.timeout not an integer: %s", config_err->message);
				g_error_free(config_err);
				return -1;
			default:
				g_error_free(config_err);
				config_err = NULL;
				timeout = DEFAULT_MPD_TIMEOUT;
				break;
		}
	}

	if (timeout < 0) {
		mpdcron_log(LOG_WARNING, "timeout smaller than zero, adjusting to default %d", DEFAULT_MPD_TIMEOUT);
		timeout = DEFAULT_MPD_TIMEOUT;
	}

	optstr = g_strdup_printf("%d", timeout);
	g_setenv("MCOPT_TIMEOUT", optstr, 1);
	g_free(optstr);

	/* Get mpd.events */
	if ((events = g_key_file_get_string_list(*cfd_r, "mpd", "events", NULL, NULL)) != NULL) {
		for (unsigned int i = 0; events[i] != NULL; i++) {
			enum mpd_idle parsed = mpd_idle_name_parse(events[i]);
			if (parsed == 0)
				mpdcron_log(LOG_WARNING, "Unrecognized idle event: %s", events[i]);
			else
				idle |= parsed;
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
