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

#include "stats-defs.h"

#include <stdlib.h>

#include <glib.h>
#include <mpd/client.h>

static GQuark mpd_quark(void)
{
	return g_quark_from_static_string("mpd");
}

struct mpd_song *mpdclient_current_song(GError **error)
{
	struct mpd_connection *conn;
	struct mpd_song *song;

	if ((conn = mpd_connection_new(globalconf.mpd_hostname,
					atoi(globalconf.mpd_port), 0)) == NULL) {
		g_set_error(error, mpd_quark(), MPD_ERROR_OOM,
				"Failed to create mpd connection");
		return NULL;
	}

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
		g_set_error(error, mpd_quark(), mpd_connection_get_error(conn),
				"%s", mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		return NULL;
	}

	if (globalconf.mpd_password != NULL) {
		if (!mpd_run_password(conn, globalconf.mpd_password)) {
			g_set_error(error, mpd_quark(), mpd_connection_get_error(conn),
					"%s", mpd_connection_get_error_message(conn));
			mpd_connection_free(conn);
			return NULL;
		}
	}

	if ((song = mpd_run_current_song(conn)) == NULL) {
		if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
			g_set_error(error, mpd_quark(), mpd_connection_get_error(conn),
					"%s", mpd_connection_get_error_message(conn));
			mpd_connection_free(conn);
			return NULL;
		}
		mpd_connection_free(conn);
		return NULL;
	}

	mpd_connection_free(conn);
	return song;
}
