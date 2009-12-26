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

/* mpdcron example module to print volume on stdout.
 * Compile this file like:
 * gcc -fPIC -shared \
 *     $(pkg-config --cflags --libs glib-2.0) \
 *     $(pkg-config --cflags --libs libdaemon) \
 *     $(pkg-config --cflags --libs libmpdclient) \
 *     example.c -o example.so
 * Put it under MPDCRON_DIR/modules where MPDCRON_DIR is ~/.mpdcron by default.
 * Load it from your configuration file like:
 * [main]
 * modules = example
 */

/* Define MPDCRON_MODULE before including the file.
 */
#define MPDCRON_MODULE		"example"
#include <mpdcron/gmodule.h>

#include <stdio.h>

#include <glib.h>
#include <libdaemon/dlog.h>
#include <mpd/client.h>

static int count;
static int last_volume;

/* Prototypes */
int init(const struct mpdcron_config *, GKeyFile *);
void destroy(void);
int run(const struct mpd_connection *, const struct mpd_status *);

/* Every module should define module which is of type struct mpdcron_module.
 * This is the initial symbol mpdcron looks for in the module.
 */
struct mpdcron_module module = {
	.name = "Example",
	.init = init,
	.destroy = destroy,
	.event_mixer = run,
};

/**
 * Initialization function.
 * @param conf mpdcron's global configuration.
 * @param fd This file descriptor points to mpdcron's configuration file.
 *           Never call g_key_file_free() on this!
 * @return On success this function should return MPDCRON_INIT_SUCCESS.
 *         On failure this function should return MPDCRON_INIT_FAILURE.
 */
int init(G_GNUC_UNUSED const struct mpdcron_config *conf, GKeyFile *fd)
{
	GError *parse_error;

	/* Use mpdcron_log() as logging function.
	 * It's a macro around daemon_log that adds MPDCRON_MODULE as a prefix
	 * to log strings.
	 */
	mpdcron_log(LOG_NOTICE, "Hello from example module!");

	/* Parse configuration here. */
	parse_error = NULL;
	count = g_key_file_get_integer(fd, "example", "count", &parse_error);
	if (parse_error != NULL) {
		switch (parse_error->code) {
			case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
			case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
				/* ignore */
				g_error_free(parse_error);
				break;
			default:
				mpdcron_log(LOG_ERR, "Parse error: %s", parse_error->message);
				g_error_free(parse_error);
				return MPDCRON_INIT_FAILURE;
		}
	}
	if (count <= 0)
		count = 5;

	last_volume = -1;
	return MPDCRON_INIT_SUCCESS;
}

/**
 * Destroy function.
 * This function should be used to free any allocated data and do other
 * cleaning up that the module has to make.
 */
void destroy(void)
{
	mpdcron_log(LOG_NOTICE, "Bye from example module!");
	/* Do the cleaning up. */
}

/**
 * Mixer event function.
 * @param conn: mpd connection
 * @param status: mpd status
 * @return This function should return MPDCRON_RUN_SUCCESS on success.
 *         This function should return MPDCRON_RUN_RECONNECT to schedule
 *           a reconnection to mpd server.
 *         This function should return MPDCRON_RUN_RECONNECT_NOW to
 *           schedule a reconnection by cancelling to run any of the next
 *           modules in the queue.
 *         This function should return MPDCRON_RUN_UNLOAD when the
 *           module needs to be unloaded.
 */
int run(G_GNUC_UNUSED const struct mpd_connection *conn, const struct mpd_status *status)
{
	int volume;

	if (count-- <= 0)
		return MPDCRON_EVENT_UNLOAD;

	volume = mpd_status_get_volume(status);

	if (last_volume < 0)
		mpdcron_log(LOG_INFO, "Volume set to: %d%%", volume);
	else
		mpdcron_log(LOG_INFO, "Volume %s from: %d to: %d%%",
				(last_volume < volume) ? "increased" : "decreased",
				last_volume, volume);

	last_volume = volume;
	return MPDCRON_EVENT_SUCCESS;
}
