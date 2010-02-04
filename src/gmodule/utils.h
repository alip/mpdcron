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

#ifndef MPDCRON_GUARD_UTILS_H
#define MPDCRON_GUARD_UTILS_H 1

#include <stdbool.h>

#include <glib.h>

static GQuark
kf_quark(void)
{
	return g_quark_from_static_string("keyfile");
}

G_GNUC_UNUSED
static bool
load_string(GKeyFile *fd, const char *grp, const char *name, bool mustload,
		char **value_r, GError **error)
{
	char *value;
	GError *e = NULL;

	if (*value_r != NULL) {
		/* already set */
		return true;
	}

	value = g_key_file_get_string(fd, grp, name, &e);
	if (e != NULL) {
		switch (e->code) {
			case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
			case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
				if (!mustload) {
					g_error_free(e);
					value = NULL;
					break;
				}
				/* fall through */
			default:
				g_set_error(error, kf_quark(), e->code,
						"Failed to load string %s.%s: %s",
						grp, name, e->message);
				g_error_free(e);
				return false;
		}
	}

	*value_r = value;
	return true;
}

G_GNUC_UNUSED
static bool
load_integer(GKeyFile *fd, const char *grp, const char *name, int mustload,
		int *value_r, GError **error)
{
	int value;
	GError *e = NULL;

	if (*value_r != -1) {
		/* already set */
		return true;
	}

	value = g_key_file_get_integer(fd, grp, name, &e);
	if (e != NULL) {
		switch (e->code) {
			case G_KEY_FILE_ERROR_GROUP_NOT_FOUND:
			case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
				if (!mustload) {
					g_error_free(e);
					value = -1;
					break;
				}
				/* fall through */
			default:
				g_set_error(error, kf_quark(), e->code,
						"Failed to load integer %s.%s: %s",
						grp, name, e->message);
				g_error_free(e);
				return false;
		}
	}

	*value_r = value;
	return true;
}

#endif /* !MPDCRON_GUARD_UTILS_H */
