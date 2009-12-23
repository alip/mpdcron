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
 * [mixer]
 * modules = example
 */

/* To do type-checking and to get some important constants
 * mpdcron/gmodule.h needs to be included.
 * Before including this file one of the MPDCRON_EVENT_* constants has to be
 * defined so that gmodule.h can define the correct prototypes for the
 * specified event.
 */
#define MPDCRON_EVENT_MIXER 1
#include <mpdcron/gmodule.h>

#include <stdio.h>

#include <glib.h>
#include <libdaemon/dlog.h>
#include <mpd/client.h>

#define EXAMPLE_LOG_PREFIX	"[example] "

static int optnd;
static int count;
static int last_volume;

/**
 * Initialization function.
 * @param nodaemon Non-zero if --no-daemon option is passed to mpdcron.
 * @param fd This file descriptor points to mpdcron's configuration file.
 *           Never call g_key_file_free() on this!
 * @return On success this function should return MPDCRON_INIT_RETVAL_SUCCESS.
 *         On failure this function should return MPDCRON_INIT_RETVAL_FAILURE.
 */
int mpdcron_init(int nodaemon, GKeyFile *fd)
{
	GError *parse_error;

	optnd = nodaemon;
	if (optnd)
		fprintf(stderr, "%sHello from example module!\n", EXAMPLE_LOG_PREFIX);

	/* Parse configuration here. */
	parse_error = NULL;
	count = g_key_file_get_integer(fd, "mixer", "count", &parse_error);
	if (parse_error != NULL) {
		if (parse_error->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
			/* Use daemon_log() as the logging function.
			 * Make sure to add a prefix so that the logs can be
			 * easily read by the user.
			 */
			daemon_log(LOG_ERR, "%sParse error: %s", EXAMPLE_LOG_PREFIX, parse_error->message);
			g_error_free(parse_error);
			return MPDCRON_INIT_RETVAL_FAILURE;
		}
	}
	if (count <= 0)
		count = 5;

	last_volume = -1;
	return MPDCRON_INIT_RETVAL_SUCCESS;
}

/**
 * Cleanup function.
 * This function should be used to free any allocated data and do other
 * cleaning up that the module has to make.
 */
void mpdcron_close(void)
{
	if (optnd)
		fprintf(stderr, "%sBye from example module!\n", EXAMPLE_LOG_PREFIX);

	/* Do the cleaning up. */
}

/**
 * Run function.
 * @param conn: mpd connection
 * @param status: mpd status
 * @return This function should return MPDCRON_RUN_RETVAL_SUCCESS on success.
 *         This function should return MPDCRON_RUN_RETVAL_RECONNECT to schedule
 *           a reconnection to mpd server.
 *         This function should return MPDCRON_RUN_RETVAL_RECONNECT_NOW to
 *           schedule a reconnection by cancelling to run any of the next
 *           modules in the queue.
 *         This function should return MPDCRON_RUN_RETVAL_UNLOAD when the
 *           module needs to be unloaded.
 */
int mpdcron_run(G_GNUC_UNUSED const struct mpd_connection *conn, const struct mpd_status *status)
{
	int volume;

	if (count-- == 0)
		return MPDCRON_RUN_RETVAL_UNLOAD;

	volume = mpd_status_get_volume(status);

	if (last_volume < 0)
		daemon_log(LOG_INFO, "%sVolume set to: %d%%", EXAMPLE_LOG_PREFIX, volume);
	else
		daemon_log(LOG_INFO, "%sVolume %s from: %d to: %d%%",
				EXAMPLE_LOG_PREFIX,
				(last_volume < volume) ? "increased" : "decreased",
				last_volume, volume);

	last_volume = volume;
	return MPDCRON_RUN_RETVAL_SUCCESS;
}
