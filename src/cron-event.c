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

static int
event_database(struct mpd_connection *conn)
{
	int ret;
	const char *name;
	struct mpd_stats *stats;

	/* Song database has been updated.
	 * Send stats command and add the variables to the environment.
	 */
	name = mpd_idle_name(MPD_IDLE_DATABASE);

	g_debug("Sending stats command to Mpd server");
	if ((stats = mpd_run_stats(conn)) == NULL)
		return -1;

	ret = 0;
#ifdef HAVE_GMODULE
	ret = module_database_run(conn, stats);
#endif /* HAVE_GMODULE */
	env_stats(stats);
	hooker_run_hook(name);
	mpd_stats_free(stats);
	return ret;
}

static int
event_stored_playlist(struct mpd_connection *conn)
{
	int ret;
	const char *name;

	/* A playlist has been updated, modified or deleted.
	 */
	name = mpd_idle_name(MPD_IDLE_STORED_PLAYLIST);

	ret = 0;
#ifdef HAVE_GMODULE
	/* TODO: Send some data to the module. */
	ret = module_stored_playlist_run(conn);
#endif /* HAVE_GMODULE */
	hooker_run_hook(name);
	return ret;
}

static int
event_queue(struct mpd_connection *conn G_GNUC_UNUSED)
{
	int ret;
	const char *name;

	/* The playlist has been changed.
	 */
	name = mpd_idle_name(MPD_IDLE_QUEUE);

	ret = 0;
#ifdef HAVE_GMODULE
	/* TODO: Send some data to the module. */
	ret = module_queue_run(conn);
#endif /* HAVE_GMODULE */
	hooker_run_hook(name);
	return ret;
}

static int
event_player(struct mpd_connection *conn)
{
	int ret;
	const char *name;
	struct mpd_song *song;
	struct mpd_status *status;

	/* The player state has changed.
	 * Send status & currentsong command and add the variables to the
	 * environment.
	 */
	name = mpd_idle_name(MPD_IDLE_PLAYER);

	g_debug("Sending status & currentsong commands to Mpd server");
	if (!mpd_command_list_begin(conn, true) ||
			!mpd_send_status(conn) ||
			!mpd_send_current_song(conn) ||
			!mpd_command_list_end(conn))
		return -1;

	if ((status = mpd_recv_status(conn)) == NULL)
		return -1;

	if (mpd_status_get_state(status) == MPD_STATE_PLAY ||
			mpd_status_get_state(status) == MPD_STATE_PAUSE) {
		if (!mpd_response_next(conn)) {
			mpd_status_free(status);
			return -1;
		}

		if ((song = mpd_recv_song(conn)) == NULL) {
			mpd_status_free(status);
			return -1;
		}
	}
	else
		song = NULL;

	if (!mpd_response_finish(conn)) {
		if (song)
			mpd_song_free(song);
		mpd_status_free(status);
		return -1;
	}

	ret = 0;
#ifdef HAVE_GMODULE
	ret = module_player_run(conn, song, status);
#endif /* HAVE_GMODULE */
	env_status_currentsong(song, status);
	hooker_run_hook(name);
	if (song)
		mpd_song_free(song);
	mpd_status_free(status);
	return ret;
}

static int
event_mixer(struct mpd_connection *conn)
{
	int ret;
	const char *name;
	struct mpd_status *status;

	/* The volume has been modified.
	 * Send status command and add the variables to the environment.
	 */
	name = mpd_idle_name(MPD_IDLE_MIXER);

	g_debug("Sending status command to Mpd server");
	if ((status = mpd_run_status(conn)) == NULL)
		return -1;

	ret = 0;
#ifdef HAVE_GMODULE
	ret = module_mixer_run(conn, status);
#endif /* HAVE_GMODULE */
	env_status(status);
	hooker_run_hook(name);
	mpd_status_free(status);
	return ret;
}

static int
event_output(struct mpd_connection *conn)
{
	int ret;
	const char *name;

	/* Outputs have been modified.
	 */
	name = mpd_idle_name(MPD_IDLE_OUTPUT);

	ret = 0;
#ifdef HAVE_GMODULE
	/* TODO: Send some data to the module. */
	ret = module_output_run(conn);
#endif /* HAVE_GMODULE */
	hooker_run_hook(name);
	return ret;
}

static int
event_options(struct mpd_connection *conn)
{
	int ret;
	const char *name;
	struct mpd_status *status;

	/* One of the options has been modified.
	 * Send status command and add the variables to the environment.
	 */
	name = mpd_idle_name(MPD_IDLE_OPTIONS);

	g_debug("Sending status command to Mpd server");
	if ((status = mpd_run_status(conn)) == NULL)
		return -1;

	ret = 0;
#ifdef HAVE_GMODULE
	ret = module_options_run(conn, status);
#endif /* HAVE_GMODULE */
	env_status(status);
	hooker_run_hook(name);
	mpd_status_free(status);
	return ret;
}

static int
event_update(struct mpd_connection *conn)
{
	int ret;
	const char *name;
	struct mpd_status *status;

	/* A database update has started or finished.
	 * Send status command and add the variables to the environment.
	 */
	name = mpd_idle_name(MPD_IDLE_OPTIONS);

	g_debug("Sending status command to Mpd server");
	if ((status = mpd_run_status(conn)) == NULL)
		return -1;

	ret = 0;
#ifdef HAVE_GMODULE
	ret = module_update_run(conn, status);
#endif /* HAVE_GMODULE */
	env_status(status);
	hooker_run_hook(name);
	mpd_status_free(status);
	return ret;
}

int
event_run(struct mpd_connection *conn, enum mpd_idle event)
{
	switch (event) {
		case MPD_IDLE_DATABASE:
			return event_database(conn);
		case MPD_IDLE_STORED_PLAYLIST:
			return event_stored_playlist(conn);
		case MPD_IDLE_PLAYER:
			return event_player(conn);
		case MPD_IDLE_QUEUE:
			return event_queue(conn);
		case MPD_IDLE_MIXER:
			return event_mixer(conn);
		case MPD_IDLE_OUTPUT:
			return event_output(conn);
		case MPD_IDLE_OPTIONS:
			return event_options(conn);
		case MPD_IDLE_UPDATE:
			return event_update(conn);
		default:
			g_warning("Unknown event 0x%x", event);
			return 0;
	}
}
