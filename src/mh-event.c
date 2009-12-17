/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet ai cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
 *
 * This file is part of the mpdhooker mpd client. mpdhooker is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * mpdhooker is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mh-defs.h"

#include <glib.h>
#include <libdaemon/dlog.h>
#include <mpd/client.h>

static int mhevent_database(struct mpd_connection *conn)
{
	/* Song database has been updated.
	 * Send stats command and add the variables to the environment.
	 */
	return mhenv_stats(conn);
}

static int mhevent_stored_playlist(struct mpd_connection *conn G_GNUC_UNUSED)
{
	return 0;
}

static int mhevent_queue(struct mpd_connection *conn G_GNUC_UNUSED)
{
	return 0;
}

static int mhevent_player(struct mpd_connection *conn G_GNUC_UNUSED)
{
	return 0;
}

static int mhevent_mixer(struct mpd_connection *conn)
{
	/* The volume has been modified.
	 * Send status command and add the variables to the environment.
	 */
	return mhenv_status(conn);
}

static int mhevent_output(struct mpd_connection *conn G_GNUC_UNUSED)
{
	return 0;
}

static int mhevent_options(struct mpd_connection *conn)
{
	/* One of the options has been modified.
	 * Send status command and add the variables to the environment.
	 */
	return mhenv_status(conn);
}

static int mhevent_update(struct mpd_connection *conn)
{
	/* A database update has started or finished.
	 * Send status command and add the variables to the environment.
	 */
	return mhenv_status(conn);
}

int mhevent_run(struct mpd_connection *conn, enum mpd_idle event)
{
	switch (event) {
		case MPD_IDLE_DATABASE:
			return mhevent_database(conn);
		case MPD_IDLE_STORED_PLAYLIST:
			return mhevent_stored_playlist(conn);
		case MPD_IDLE_PLAYER:
			return mhevent_player(conn);
		case MPD_IDLE_QUEUE:
			return mhevent_queue(conn);
		case MPD_IDLE_MIXER:
			return mhevent_mixer(conn);
		case MPD_IDLE_OUTPUT:
			return mhevent_output(conn);
		case MPD_IDLE_OPTIONS:
			return mhevent_options(conn);
		case MPD_IDLE_UPDATE:
			return mhevent_update(conn);
		default:
			mh_log(LOG_WARNING, "Unknown event 0x%x", event);
			return 0;
	}
}
