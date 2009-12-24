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

#include <stdbool.h>

#include <glib.h>
#include <mpd/client.h>

enum mpdcron_init_retval {
	MPDCRON_INIT_SUCCESS = 0,
	MPDCRON_INIT_FAILURE,
};

enum mpdcron_run_retval {
	MPDCRON_RUN_SUCCESS = 0, /** Success **/
	MPDCRON_RUN_RECONNECT, /** Schedule a reconnection to mpd server **/
	MPDCRON_RUN_RECONNECT_NOW, /** Schedule a reconnection to mpd server immediately. **/
	MPDCRON_RUN_UNLOAD, /** Unload the module **/
};

struct mpdcron_module {
	/** Name of the module */
	const char *name;

	/** Set this to true, if the module is a generic module. */
	bool generic;

	/** List of events the module supports (for non-generic modules) */
	int events;

	/** Initialization function */
	int (*init) (int, GKeyFile *);

	/** Cleanup function */
	void (*destroy) (void);

	/** Function for database event */
	int (*event_database) (const struct mpd_connection *conn, const struct mpd_stats *);

	/** Function for stored playlist event */
	int (*event_stored_playlist) (const struct mpd_connection *);

	/** Function for queue event */
	int (*event_queue) (const struct mpd_connection *);

	/** Function for player event */
	int (*event_player) (const struct mpd_connection *, const struct mpd_song *,
			const struct mpd_status *);

	/** Function for mixer event */
	int (*event_mixer) (const struct mpd_connection *, const struct mpd_status *);

	/** Function for output event */
	int (*event_output) (const struct mpd_connection *);

	/** Function for options event */
	int (*event_options) (const struct mpd_connection *, const struct mpd_status *);

	/** Function for update event */
	int (*event_update) (const struct mpd_connection *, const struct mpd_status *);
};

#ifndef MPDCRON_INTERNAL
extern struct mpdcron_module module;
#endif /* !MPDCRON_INTERNAL */

#endif /* !MPDCRON_GUARD_MODULE_H */
