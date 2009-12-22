/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet ai cin fdm=syntax : */

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

#include <string.h>

#include <glib.h>
#include <libdaemon/dlog.h>

static struct scrobbler_config *file_load_scrobbler(GKeyFile *fd, const char *grp)
{
	char *p;
	struct scrobbler_config *scrobbler = g_new(struct scrobbler_config, 1);
	GError *cerr = NULL;

	scrobbler->name = g_strdup(grp);
	scrobbler->url = g_key_file_get_string(fd, grp, "url", &cerr);
	if (cerr != NULL) {
		daemon_log(LOG_ERR, "%serror while reading url from group %s: %s",
				SCROBBLER_LOG_PREFIX,
				grp, cerr->message);
		g_free(scrobbler);
		g_error_free(cerr);
		return NULL;
	}
	scrobbler->username = g_key_file_get_string(fd, grp, "username", &cerr);
	if (cerr != NULL) {
		daemon_log(LOG_ERR, "%serror while reading username from group %s: %s",
				SCROBBLER_LOG_PREFIX,
				grp, cerr->message);
		g_free(scrobbler);
		g_error_free(cerr);
		return NULL;
	}
	scrobbler->password = g_key_file_get_string(fd, grp, "password", &cerr);
	if (cerr != NULL) {
		daemon_log(LOG_ERR, "%serror while reading password from group %s: %s",
				SCROBBLER_LOG_PREFIX,
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

int file_load(GKeyFile *fd, GSList **scrobblers_ptr)
{
	int s = 0;
	struct scrobbler_config *scrobbler;

	if ((scrobbler = file_load_scrobbler(fd, "libre.fm")) != NULL) {
		*scrobblers_ptr = g_slist_prepend(*scrobblers_ptr, scrobbler);
		++s;
	}
	if ((scrobbler = file_load_scrobbler(fd, "last.fm")) != NULL) {
		*scrobblers_ptr = g_slist_prepend(*scrobblers_ptr, scrobbler);
		++s;
	}

	if (s == 0) {
		daemon_log(LOG_ERR,
			"%sneither last.fm nor libre.fm group defined",
			SCROBBLER_LOG_PREFIX);
		return -1;
	}

	proxy = g_key_file_get_string(fd, "scrobbler", "proxy", NULL);
	return 0;
}
