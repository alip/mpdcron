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

#ifndef MPDCRON_GUARD_MODULE_H
#define MPDCRON_GUARD_MODULE_H 1

/* IMPORTANT:
 * Before including this header #define exactly one of the following:
 * MPDCRON_EVENT_GENERIC
 * MPDCRON_EVENT_DATABASE
 * MPDCRON_EVENT_STORED_PLAYLIST
 * MPDCRON_EVENT_QUEUE
 * MPDCRON_EVENT_PLAYER
 * MPDCRON_EVENT_MIXER
 * MPDCRON_EVENT_OUTPUT
 * MPDCRON_EVENT_OPTIONS
 * MPDCRON_EVENT_UPDATE
 */

#include <glib.h>
#include <mpd/client.h>

enum mpdcron_init_retval {
	MPDCRON_INIT_RETVAL_SUCCESS = 0,
	MPDCRON_INIT_RETVAL_FAILURE,
};

enum mpdcron_run_retval {
	MPDCRON_RUN_RETVAL_SUCCESS = 0, /** Success **/
	MPDCRON_RUN_RETVAL_RECONNECT, /** Schedule a reconnection to mpd server **/
	MPDCRON_RUN_RETVAL_RECONNECT_NOW, /** Schedule a reconnection to mpd server immediately. **/
	MPDCRON_RUN_RETVAL_UNLOAD, /** Unload the module **/
};

#ifndef MPDCRON_INTERNAL
/**
 * Initialize function. This function is called when the module is loaded.
 *
 * @param nodaemon Non-zero if --no-daemon option is passed to mpdcron.
 * @param fd This file descriptor points to mpdcron's configuration file.
 *           Don't call g_key_file_free() on this!
 * @return @see enum mpdcron_init_retval
 */
int mpdcron_init(int nodaemon, GKeyFile *fd);

/**
 * Close function. This function is called when the module is unloaded.
 */
void mpdcron_close(void);

#if defined(MPDCRON_EVENT_DATABASE)
/**
 * Run function for database event
 */
int mpdcron_run(const struct mpd_connection *conn, const struct mpd_stats *stats);
#elif defined(MPDCRON_EVENT_STORED_PLAYLIST)
/**
 * Run function for stored playlist event
 */
int mpdcron_run(const struct mpd_connection *conn);
#elif defined(MPDCRON_EVENT_QUEUE)
/**
 * Run function for queue event
 */
int mpdcron_run(const struct mpd_connection *conn);
#elif defined(MPDCRON_EVENT_PLAYER)
/**
 * Run function for player event
 */
int mpdcron_run(const struct mpd_connection *conn, const struct mpd_song *song, const struct mpd_status *status);
#elif defined(MPDCRON_EVENT_MIXER)
/**
 * Run function for mixer event
 */
int mpdcron_run(const struct mpd_connection *conn, const struct mpd_status *status);
#elif defined(MPDCRON_EVENT_OUTPUT)
/**
 * Run function for output event
 */
int mpdcron_run(const struct mpd_connection *conn);
#elif defined(MPDCRON_EVENT_OPTIONS)
/**
 * Run function for options event
 */
int mpdcron_run(const struct mpd_connection *conn, const struct mpd_status *status);
#elif defined(MPDCRON_EVENT_UPDATE)
/**
 * Run function for update event
 */
int mpdcron_run(const struct mpd_connection *conn, const struct mpd_status *status);
#elif defined(MPDCRON_EVENT_GENERIC)
/* No mpdcron_run() for generic modules */
#else
#error None of the required MPDCRON_EVENT_* constants were defined!
#endif

#endif /* !MPDCRON_INTERNAL */

#endif /* !MPDCRON_GUARD_MODULE_H */
