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

#ifndef MPDCRON_GUARD_STATS_DEFS_H
#define MPDCRON_GUARD_STATS_DEFS_H 1

#define MPDCRON_MODULE		"stats"
#include "../gmodule.h"

#include <stdbool.h>

#include <glib.h>
#include <mpd/client.h>

extern char *dbpath;

bool db_init(const char *path);
bool db_process(const char *path, const struct mpd_song *song, bool increment);
bool db_love(const char *path, const struct mpd_song *song);
bool db_love_uri(const char *path, const char *uri, int isexpr, int wantcount);
bool db_love_expr(const char *path, const char *expr, int wantcount);
bool db_hate(const char *path, const struct mpd_song *song);
bool db_hate_uri(const char *path, const char *uri, int isexpr, int wantcount);
bool db_hate_expr(const char *path, const char *expr, int wantcount);

int file_load(const struct mpdcron_config *conf, GKeyFile *fd);
void file_cleanup(void);

#endif /* !MPDCRON_GUARD_STATS_DEFS_H */
