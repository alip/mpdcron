/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

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
	ACK_ERROR_DATABASE_OPEN = 100,
	ACK_ERROR_DATABASE_CREATE = 101,
	ACK_ERROR_DATABASE_VERSION = 102,
	ACK_ERROR_DATABASE_INSERT = 103,
	ACK_ERROR_DATABASE_SELECT = 104,
	ACK_ERROR_DATABASE_UPDATE = 105,
	ACK_ERROR_DATABASE_PREPARE = 106,
	ACK_ERROR_DATABASE_BIND = 107,
	ACK_ERROR_DATABASE_STEP = 108,
	ACK_ERROR_DATABASE_RESET = 109,

	ACK_ERROR_NO_EXIST = 150,
	ACK_ERROR_NO_TAGS = 151,
};

/**
 * Database Interface
 */
sqlite3 *db_init(const char *path, bool create, bool readonly, GError **error);
void db_close(sqlite3 *db);
bool db_process(sqlite3 *db, const struct mpd_song *song, bool increment, GError **error);
bool db_love_artist_expr(sqlite3 *db, const char *expr, bool love, GSList **values, GError **error);
bool db_love_album_expr(sqlite3 *db, const char *expr, bool love, GSList **values, GError **error);
bool db_love_genre_expr(sqlite3 *db, const char *expr, bool love, GSList **values, GError **error);
bool db_love_song_expr(sqlite3 *db, const char *expr, bool love, GSList **values, GError **error);
bool db_kill_artist_expr(sqlite3 *db, const char *expr, bool kkill, GSList **values, GError **error);
bool db_kill_album_expr(sqlite3 *db, const char *expr, bool kkill, GSList **values, GError **error);
bool db_kill_genre_expr(sqlite3 *db, const char *expr, bool kkill, GSList **values, GError **error);
bool db_kill_song_expr(sqlite3 *db, const char *expr, bool kkill, GSList **values, GError **error);
bool db_rate_artist_expr(sqlite3 *db, const char *expr, int rating, GSList **values, GError **error);
bool db_rate_album_expr(sqlite3 *db, const char *expr, int rating, GSList **values, GError **error);
bool db_rate_genre_expr(sqlite3 *db, const char *expr, int rating, GSList **values, GError **error);
bool db_rate_song_expr(sqlite3 *db, const char *expr, int rating, GSList **values, GError **error);

#endif /* !MPDCRON_GUARD_STATS_SQLITE_H */
