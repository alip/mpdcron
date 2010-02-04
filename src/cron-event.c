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
	struct mpd_stats *stats = NULL;

	/* Song database has been updated.
	 * Send stats command and add the variables to the environment.
	 */
	name = mpd_idle_name(MPD_IDLE_DATABASE);

	if (env_stats(conn, &stats) < 0)
		return -1;

	ret = 0;
#ifdef HAVE_GMODULE
	ret = module_database_run(conn, stats);
#endif /* HAVE_GMODULE */
	mpd_stats_free(stats);
	hooker_run_hook(name);
	return ret;
}

static int
event_stored_playlist(struct mpd_connection *conn)
{
	int ret;
	const char *name;

	/* A playlist has been updated, modified or deleted.
	 * Send list_all_meta command and add the variables to the environment.
	 */
	name = mpd_idle_name(MPD_IDLE_STORED_PLAYLIST);

	if (env_list_all_meta(conn) < 0)
		return -1;

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
	 * Send list_queue_meta command and add the variables to the
	 * environment.
	 */
	name = mpd_idle_name(MPD_IDLE_QUEUE);

	if (env_list_queue_meta(conn) < 0)
		return -1;

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
	struct mpd_song *song = NULL;
	struct mpd_status *status = NULL;

	/* The player state has changed.
	 * Send status & currentsong command and add the variables to the
	 * environment.
	 */
	name = mpd_idle_name(MPD_IDLE_PLAYER);

	if (env_status_currentsong(conn, &song, &status) < 0)
		return -1;

	ret = 0;
#ifdef HAVE_GMODULE
	ret = module_player_run(conn, song, status);
#endif /* HAVE_GMODULE */
	if (song != NULL)
		mpd_song_free(song);
	mpd_status_free(status);
	hooker_run_hook(name);
	return ret;
}

static int
event_mixer(struct mpd_connection *conn)
{
	int ret;
	const char *name;
	struct mpd_status *status = NULL;

	/* The volume has been modified.
	 * Send status command and add the variables to the environment.
	 */
	name = mpd_idle_name(MPD_IDLE_MIXER);

	if (env_status(conn, &status) < 0)
		return -1;

	ret = 0;
#ifdef HAVE_GMODULE
	ret = module_mixer_run(conn, status);
#endif /* HAVE_GMODULE */
	mpd_status_free(status);
	hooker_run_hook(name);
	return ret;
}

static int
event_output(struct mpd_connection *conn)
{
	int ret;
	const char *name;

	/* Outputs have been modified.
	 * Send outputs command and add the variables to the environment.
	 */
	name = mpd_idle_name(MPD_IDLE_OUTPUT);

	if (env_outputs(conn) < 0)
		return -1;

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
	struct mpd_status *status = NULL;

	/* One of the options has been modified.
	 * Send status command and add the variables to the environment.
	 */
	name = mpd_idle_name(MPD_IDLE_OPTIONS);

	if (env_status(conn, &status) < 0)
		return -1;

	ret = 0;
#ifdef HAVE_GMODULE
	ret = module_options_run(conn, status);
#endif /* HAVE_GMODULE */
	mpd_status_free(status);
	hooker_run_hook(name);
	return ret;
}

static int
event_update(struct mpd_connection *conn)
{
	int ret;
	const char *name;
	struct mpd_status *status = NULL;

	/* A database update has started or finished.
	 * Send status command and add the variables to the environment.
	 */
	name = mpd_idle_name(MPD_IDLE_OPTIONS);

	if (env_status(conn, &status) < 0)
		return -1;

	ret = 0;
#ifdef HAVE_GMODULE
	ret = module_update_run(conn, status);
#endif /* HAVE_GMODULE */
	mpd_status_free(status);
	hooker_run_hook(name);
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
