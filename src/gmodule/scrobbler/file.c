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

#include <stdbool.h>
#include <string.h>

#include <glib.h>
#include <libdaemon/dlog.h>

struct config fileconfig;

static bool load_string(GKeyFile *fd, const char *name, char **value_r)
{
	char *value;
	GError *e = NULL;

	if (*value_r != NULL) {
		/* already set */
		return true;
	}

	value = g_key_file_get_string(fd, "scrobbler", name, &e);
	if (e != NULL) {
		if (e->code  != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
			daemon_log(LOG_ERR, "%sfailed to load scrobbler.%s: %s",
					SCROBBLER_LOG_PREFIX, name, e->message);
			g_error_free(e);
			return false;
		}
	}

	*value_r = value;
	return true;
}

static bool load_integer(GKeyFile *fd, const char *name, int *value_r)
{
	int value;
	GError *e = NULL;

	if (*value_r != -1) {
		/* already set */
		return true;
	}

	value = g_key_file_get_integer(fd, "scrobbler", name, &e);
	if (e != NULL) {
		if (e->code  != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
			daemon_log(LOG_ERR, "%sfailed to load scrobbler.%s: %s",
					SCROBBLER_LOG_PREFIX, name, e->message);
			g_error_free(e);
			return false;
		}
	}

	*value_r = value;
	return true;
}

static bool load_unsigned(GKeyFile *fd, const char *name, unsigned *value_r)
{
	int value = -1;

	if (!load_integer(fd, name, &value))
		return false;

	if (value < 0) {
		daemon_log(LOG_ERR, "%ssetting scrobbler.%s must not be negative",
				SCROBBLER_LOG_PREFIX, name);
		return false;
	}

	*value_r = (unsigned)value;
	return true;
}

static struct scrobbler_config *load_scrobbler(GKeyFile *fd, const char *grp)
{
	char *p;
	struct scrobbler_config *scrobbler = g_new(struct scrobbler_config, 1);
	GError *cerr = NULL;

	if (!g_key_file_has_group(fd, grp))
		return NULL;

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

static void scrobbler_config_free_callback(gpointer data, G_GNUC_UNUSED gpointer user_data)
{
	struct scrobbler_config *scrobbler = data;

	g_free(scrobbler->name);
	g_free(scrobbler->url);
	g_free(scrobbler->username);
	g_free(scrobbler->password);
	g_free(scrobbler->journal);
	g_free(scrobbler);
}

int file_load(GKeyFile *fd)
{
	int s = 0;
	struct scrobbler_config *scrobbler;

	memset(&file_config, 0, sizeof(struct config));
	file_config.journal_interval = -1;

	if (!load_string(fd, "proxy", &file_config.proxy))
		return -1;
	if (!load_unsigned(fd, "journal_interval", &file_config.journal_interval))
		return -1;

	if ((scrobbler = load_scrobbler(fd, "libre.fm")) != NULL) {
		file_config.scrobblers = g_slist_prepend(file_config.scrobblers, scrobbler);
		++s;
	}
	if ((scrobbler = load_scrobbler(fd, "last.fm")) != NULL) {
		file_config.scrobblers = g_slist_prepend(file_config.scrobblers, scrobbler);
		++s;
	}

	if (s == 0) {
		daemon_log(LOG_ERR,
			"%sneither last.fm nor libre.fm group defined",
			SCROBBLER_LOG_PREFIX);
		return -1;
	}

	if (file_config.proxy == NULL && g_getenv("http_proxy"))
		file_config.proxy = g_strdup(g_getenv("http_proxy"));

	return 0;
}

void file_cleanup(void)
{
	g_free(file_config.proxy);
	g_slist_foreach(file_config.scrobblers, scrobbler_config_free_callback, NULL);
	g_slist_free(file_config.scrobblers);
}
