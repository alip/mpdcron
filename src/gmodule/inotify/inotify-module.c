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

#include "inotify-defs.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>

#include <glib.h>
#include <libdaemon/dlog.h>
#include <mpd/client.h>

/* Inotify may send a lot of events at once.
 * So we use a 32k buffer which should be big enough.
 */
#define INOTIFY_BUFFER_LENGTH		32768

int ifd;
static int watch_sid = 0;

static gboolean watch_callback(GIOChannel *source,
		GIOCondition condition, G_GNUC_UNUSED gpointer userdata);

static void schedule_read(void)
{
	GIOChannel *channel;

	g_assert(watch_sid == 0);

	channel = g_io_channel_unix_new(ifd);
	g_io_channel_set_encoding(channel, NULL, NULL);
	g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, NULL );
	watch_sid = g_io_add_watch(channel, G_IO_IN | G_IO_PRI, watch_callback, NULL);
	g_io_channel_unref(channel);
}

static gboolean watch_callback(GIOChannel *source,
		G_GNUC_UNUSED GIOCondition condition,
		G_GNUC_UNUSED gpointer userdata)
{
	unsigned int i;
	gsize len;
	GIOStatus status;
	char *path;
	char buf[INOTIFY_BUFFER_LENGTH];
	struct inotify_event *event;
	GError *read_error = NULL;

	g_assert(ifd > 0);
	g_assert(watch_sid != 0);

	watch_sid = 0;

	if ((status = g_io_channel_read_chars(source, buf, INOTIFY_BUFFER_LENGTH,
				&len, &read_error)) != G_IO_STATUS_NORMAL) {
		switch (status) {
			case G_IO_STATUS_EOF:
				daemon_log(LOG_ERR, "%sinotify reported EOF"
						"(possibly too many events at the same time)",
						INOTIFY_LOG_PREFIX);
				g_error_free(read_error);
				return FALSE;
			default:
				daemon_log(LOG_ERR,"%sreading from inotify device failed: %s",
						INOTIFY_LOG_PREFIX, read_error->message);
				g_error_free(read_error);
				return FALSE;
		}
	}

	i = 0;
	while (i < len) {
		event = (struct inotify_event *) &buf[i];
		/* Find the path */
		path = g_hash_table_lookup(file_config.directories, GINT_TO_POINTER(event->wd));
		if (path == NULL) {
			daemon_log(LOG_WARNING, "%sunknown watch fd reported by inotify, ignoring",
					INOTIFY_LOG_PREFIX);
			i += sizeof(struct inotify_event) + event->len;
			continue;
		}
		daemon_log(LOG_INFO, "%snew inotify event, name: %s path: %s",
				INOTIFY_LOG_PREFIX,
				(event->len > 0) ? event->name : "<none>",
				path);
		if (file_config.patterns != NULL) {
			if (pattern_check(event->name, event->len))
				if (!update_mpd(path))
					return FALSE;
		}
		else {
			if (!update_mpd(path))
				return FALSE;
		}
		i += sizeof(struct inotify_event) + event->len;
	}

	schedule_read();
	return FALSE;
}

int mpdcron_init(G_GNUC_UNUSED int nodaemon, GKeyFile *fd)
{
	if ((ifd = inotify_init()) < 0) {
		daemon_log(LOG_ERR, "%sinotify_init() failed: %s", INOTIFY_LOG_PREFIX, g_strerror(errno));
		return MPDCRON_INIT_RETVAL_FAILURE;
	}
	if (file_load(fd) < 0)
		return MPDCRON_INIT_RETVAL_FAILURE;
	daemon_log(LOG_DEBUG, "%sadded inotify watch on %d directories", INOTIFY_LOG_PREFIX,
			g_hash_table_size(file_config.directories));
	schedule_read();
	return MPDCRON_INIT_RETVAL_SUCCESS;
}

void mpdcron_close(void)
{
	if (watch_sid != 0) {
		g_source_remove(watch_sid);
		watch_sid = 0;
	}

	if (ifd > 0) {
		close(ifd);
		ifd = -1;
	}

	file_cleanup();
}
