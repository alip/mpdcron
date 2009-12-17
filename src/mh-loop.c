/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet ai cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
 * Based in part upon mpdscribble which is:
 *   Copyright (C) 2008-2009 The Music Player Daemon Project
 *   Copyright (C) 2005-2008 Kuno Woudt <kuno@frob.nl>
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

#include <stdbool.h>
#include <stdlib.h>

#include <glib.h>
#include <mpd/client.h>

static guint idle_sid, reconnect_sid;
static const unsigned *version = NULL;
static struct mpd_connection *conn = NULL;

static void mhmpd_schedule_reconnect(void);
static void mhmpd_schedule_idle(void);

static void mhmpd_failure(void)
{
	char *msg;

	g_assert(conn != NULL);
	msg = g_strescape(mpd_connection_get_error_message(conn), NULL);
	mh_log(LOG_WARNING, "Mpd error: %s", msg);
	g_free(msg);
	mpd_connection_free(conn);
	conn = NULL;
}

static gboolean mhmpd_reconnect(G_GNUC_UNUSED gpointer data)
{
	mh_log(LOG_INFO, "Connection to `%s' on port %s with timeout %.2lf",
			hostname, port, timeout);
	if ((conn = mpd_connection_new(hostname, atoi(port), timeout)) == NULL) {
		mh_log(LOG_ERR, "Error creating mpd connection: out of memory");
		exit(EXIT_FAILURE);
	}
	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
		mhmpd_failure();
		return TRUE;
	}

	if (password != NULL) {
		mh_log(LOG_INFO, "Sending password");
		if (!mpd_run_password(conn, password)) {
			mh_log(LOG_ERR, "Authentication failed: %s", mpd_connection_get_error_message(conn));
			mpd_connection_free(conn);
			conn = NULL;
			exit(EXIT_FAILURE);
		}
	}

	if ((version = mpd_connection_get_server_version(conn)) != NULL) {
		mh_log(LOG_INFO, "Connected to Mpd server, running version %d.%d.%d",
				version[0], version[1], version[2]);
		if (mpd_connection_cmp_server_version(conn, 0, 14, 0) < 0)
			mh_log(LOG_WARNING, "Mpd version 0.14.0 is required for idle command");
	}
	else
		mh_log(LOG_INFO, "Connected to Mpd server, running version unknown");

	mhmpd_schedule_idle();
	reconnect_sid = 0;
	return FALSE;
}

static gboolean mhmpd_idle(G_GNUC_UNUSED GIOChannel *source,
		G_GNUC_UNUSED GIOCondition condition,
		G_GNUC_UNUSED gpointer data)
{
	unsigned j;
	const char *name;
	enum mpd_idle i, myidle;

	g_assert(idle_sid != 0);
	g_assert(conn != NULL);

	idle_sid = 0;
	if ((myidle = mpd_recv_idle(conn, false)) == 0) {
		if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
			mhmpd_failure();
			mhmpd_schedule_reconnect();
			return FALSE;
		}
		mhmpd_schedule_idle();
		return FALSE;
	}

	for (j = 0 ;; j++) {
		i = 1 << j;
		if ((name = mpd_idle_name(i)) == NULL)
			break;
		if (myidle & i) {
			/* Run the appropriate event */
			if (mhevent_run(i) < 0) {
				mhmpd_failure();
				mhmpd_schedule_reconnect();
				return FALSE;
			}
			/* Run the hook */
			mhhooker_run_hook(name);
		}
	}

	mhmpd_schedule_idle();
	return FALSE;
}

static void mhmpd_schedule_reconnect(void)
{
	g_assert(reconnect_sid == 0);
	mh_log(LOG_INFO, "Waiting for %d seconds before reconnecting", reconnect);
	reconnect_sid = g_timeout_add_seconds(reconnect, mhmpd_reconnect, NULL);
}

static void mhmpd_schedule_idle(void)
{
	int fd;
	bool ret;
	GIOChannel *channel;

	g_assert(idle_sid == 0);
	g_assert(conn != NULL);

	mh_logv(LOG_DEBUG, "Sending idle command with mask 0x%x", idle);
	ret = (idle == 0) ? mpd_send_idle(conn) : mpd_send_idle_mask(conn, idle);
	if (!ret && mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
		mhmpd_failure();
		mhmpd_schedule_reconnect();
		return;
	}

	/* Add a GLib watch on the libmpdclient socket. */
	fd = mpd_connection_get_fd(conn);
	channel = g_io_channel_unix_new(fd);
	idle_sid = g_io_add_watch(channel, G_IO_IN, mhmpd_idle, NULL);
	g_io_channel_unref(channel);
}

void mhmpd_connect(void)
{
	if (mhmpd_reconnect(NULL))
		mhmpd_schedule_reconnect();
}

void mhmpd_disconnect(void)
{
	if (idle_sid != 0)
		g_source_remove(idle_sid);

	if (reconnect_sid != 0)
		g_source_remove(reconnect_sid);

	if (conn != NULL) {
		mpd_connection_free(conn);
		conn = NULL;
	}
}
