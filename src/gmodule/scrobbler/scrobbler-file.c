/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
 * Based in part upon mpdscribble which is:
 *   Copyright (C) 2008-2009 The Music Player Daemon Project
 *   Copyright (C) 2005-2008 Kuno Woudt <kuno@frob.nl>
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

#include "scrobbler-defs.h"

#include <stdbool.h>
#include <string.h>

#include <glib.h>
#include <libdaemon/dlog.h>

#include "../utils.h"

struct config file_config;

static struct scrobbler_config *
load_scrobbler(GKeyFile *fd, const char *grp)
{
	char *p;
	struct scrobbler_config *scrobbler = g_new(struct scrobbler_config, 1);
	GError *cerr = NULL;

	if (!g_key_file_has_group(fd, grp))
		return NULL;

	scrobbler->name = g_strdup(grp);
	scrobbler->url = g_key_file_get_string(fd, grp, "url", &cerr);
	if (cerr != NULL) {
		mpdcron_log(LOG_ERR, "Error while reading url from group %s: %s",
				grp, cerr->message);
		g_free(scrobbler);
		g_error_free(cerr);
		return NULL;
	}
	scrobbler->username = g_key_file_get_string(fd, grp, "username", &cerr);
	if (cerr != NULL) {
		mpdcron_log(LOG_ERR, "Error while reading username from group %s: %s",
				grp, cerr->message);
		g_free(scrobbler);
		g_error_free(cerr);
		return NULL;
	}
	scrobbler->password = g_key_file_get_string(fd, grp, "password", &cerr);
	if (cerr != NULL) {
		mpdcron_log(LOG_ERR, "Error while reading password from group %s: %s",
				grp, cerr->message);
		g_free(scrobbler);
		g_error_free(cerr);
		return NULL;
	}
	/* Parse password */
	if (strncmp(scrobbler->password, "md5:", 4) == 0) {
		p = g_strdup(scrobbler->password + 4);
		g_free(scrobbler->password);
		scrobbler->password = p;
	}
	else {
		p = g_compute_checksum_for_string(G_CHECKSUM_MD5, scrobbler->password, -1);
		g_free(scrobbler->password);
		scrobbler->password = p;
	}

	scrobbler->journal = g_key_file_get_string(fd, grp, "journal", NULL);
	return scrobbler;
}

static void
scrobbler_config_free_callback(gpointer data, G_GNUC_UNUSED gpointer user_data)
{
	struct scrobbler_config *scrobbler = data;

	g_free(scrobbler->name);
	g_free(scrobbler->url);
	g_free(scrobbler->username);
	g_free(scrobbler->password);
	g_free(scrobbler->journal);
	g_free(scrobbler);
}

int
file_load(GKeyFile *fd)
{
	int s = 0;
	struct scrobbler_config *scrobbler;
	GError *error;

	memset(&file_config, 0, sizeof(struct config));
	file_config.journal_interval = -1;

	error = NULL;
	if (!load_string(fd, MPDCRON_MODULE, "proxy", false, &file_config.proxy, &error)) {
		mpdcron_log(LOG_ERR, "Failed to load "MPDCRON_MODULE".proxy: %s", error->message);
		g_error_free(error);
		return -1;
	}
	if (!load_integer(fd, MPDCRON_MODULE, "journal_interval", false, &file_config.journal_interval, &error)) {
		mpdcron_log(LOG_ERR, "Failed to load "MPDCRON_MODULE".journal_interval: %s", error->message);
		g_error_free(error);
		return -1;
	}
	else if (file_config.journal_interval <= 0)
		file_config.journal_interval = 60;

	if ((scrobbler = load_scrobbler(fd, "libre.fm")) != NULL) {
		file_config.scrobblers = g_slist_prepend(file_config.scrobblers, scrobbler);
		++s;
	}
	if ((scrobbler = load_scrobbler(fd, "last.fm")) != NULL) {
		file_config.scrobblers = g_slist_prepend(file_config.scrobblers, scrobbler);
		++s;
	}

	if (s == 0) {
		mpdcron_log(LOG_ERR, "Neither last.fm nor libre.fm group defined");
		return -1;
	}

	if (file_config.proxy == NULL && g_getenv("http_proxy"))
		file_config.proxy = g_strdup(g_getenv("http_proxy"));

	return 0;
}

void
file_cleanup(void)
{
	g_free(file_config.proxy);
	g_slist_foreach(file_config.scrobblers, scrobbler_config_free_callback, NULL);
	g_slist_free(file_config.scrobblers);
}
