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

#include <stdbool.h>

#include <glib.h>
#include <libdaemon/dlog.h>

#include "../utils.h"

char *dbpath;

int file_load(const struct mpdcron_config *conf, GKeyFile *fd)
{
	dbpath = NULL;

	if (!load_string(fd, MPDCRON_MODULE, "dbpath", false, &dbpath))
		return -1;

	if (dbpath == NULL)
		dbpath = g_build_filename(conf->home_path, "stats.db", NULL);

	return 0;
}

void file_cleanup(void)
{
	g_free(dbpath);
}
