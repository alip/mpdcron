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

#include "notification-defs.h"

#include <glib.h>

char *cover_find(const char *path, const char *suffix, const char *artist, const char *album)
{
	char *cfile;
	char *cpath;

	cfile = g_strdup_printf("%s-%s.%s", artist, album, suffix);
	cpath = g_build_filename(path, cfile, NULL);
	g_free(cfile);

	if (g_file_test(cpath, G_FILE_TEST_EXISTS))
		return cpath;
	g_free(cpath);
	return NULL;
}
