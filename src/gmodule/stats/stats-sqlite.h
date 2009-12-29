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

#ifndef MPDCRON_GUARD_STATS_SQLITE_H
#define MPDCRON_GUARD_STATS_SQLITE_H 1

#include <stdbool.h>

#include <glib.h>
#include <sqlite3.h>
#include <mpd/client.h>

enum dback {
	ACK_ERROR_DATABASE_OPEN = 200,
	ACK_ERROR_DATABASE_CREATE = 201,
	ACK_ERROR_DATABASE_VERSION = 202,
	ACK_ERROR_DATABASE_INSERT = 203,
	ACK_ERROR_DATABASE_SELECT = 204,
	ACK_ERROR_DATABASE_UPDATE = 205,

	ACK_ERROR_SONG_NO_TAGS = 250,
};

/**
 * Database Interface
 */
sqlite3 *db_init(const char *path, GError **error);
void db_close(sqlite3 *db);
bool db_process(sqlite3 *db, const struct mpd_song *song, bool increment, GError **error);
bool db_love_song_uri(sqlite3 *db, const char *uri, bool love, int *value, GError **error);

#endif /* !MPDCRON_GUARD_STATS_SQLITE_H */
