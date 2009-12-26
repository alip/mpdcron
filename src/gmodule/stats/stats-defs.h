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
#include <sqlite3.h>

extern char *dbpath;

sqlite3 *db_init(const char *path);
bool db_process(sqlite3 *db, const struct mpd_song *song, bool increment);
bool db_love_artist(sqlite3 *db, const struct mpd_song *song, bool love, bool wantcount);
bool db_love_artist_expr(sqlite3 *db, const char *expr, bool love, bool wantcount);
bool db_love_album(sqlite3 *db, const struct mpd_song *song, bool love, bool wantcount);
bool db_love_album_expr(sqlite3 *db, const char *expr, bool love, bool wantcount);
bool db_love_genre(sqlite3 *db, const struct mpd_song *song, bool love, bool wantcount);
bool db_love_genre_expr(sqlite3 *db, const char *expr, bool love, bool wantcount);
bool db_love_song(sqlite3 *db, const struct mpd_song *song, bool love, bool wantcount);
bool db_love_song_expr(sqlite3 *db, const char *expr, bool love, bool wantcount);
bool db_kill_artist(sqlite3 *db, const struct mpd_song *song, bool kkill);
bool db_kill_artist_expr(sqlite3 *db, const char *expr, bool kkill, bool wantcount);
bool db_kill_album(sqlite3 *db, const struct mpd_song *song, bool kkill);
bool db_kill_album_expr(sqlite3 *db, const char *expr, bool kkill, bool wantcount);
bool db_kill_genre(sqlite3 *db, const struct mpd_song *song, bool kkill);
bool db_kill_genre_expr(sqlite3 *db, const char *expr, bool kkill, bool wantcount);
bool db_kill_song(sqlite3 *db, const struct mpd_song *song, bool kkill);
bool db_kill_song_expr(sqlite3 *db, const char *expr, bool kkill, bool wantcount);
bool db_rate_artist(sqlite3 *db, const struct mpd_song *song, long rating, bool add, bool wantcount);
bool db_rate_artist_expr(sqlite3 *db, const char *expr, long rating, bool add, bool wantcount);
bool db_rate_album(sqlite3 *db, const struct mpd_song *song, long rating, bool add, bool wantcount);
bool db_rate_album_expr(sqlite3 *db, const char *expr, long rating, bool add, bool wantcount);
bool db_rate_genre(sqlite3 *db, const struct mpd_song *song, long rating, bool add, bool wantcount);
bool db_rate_genre_expr(sqlite3 *db, const char *expr, long rating, bool add, bool wantcount);
bool db_rate_song(sqlite3 *db, const struct mpd_song *song, long rating, bool add, bool wantcount);
bool db_rate_song_expr(sqlite3 *db, const char *expr, long rating, bool add, bool wantcount);
bool db_load_artist_expr(sqlite3 *db, const char *expr, GSList **list_r);
bool db_load_album_expr(sqlite3 *db, const char *expr, GSList **list_r);
bool db_load_genre_expr(sqlite3 *db, const char *expr, GSList **list_r);
bool db_load_song_expr(sqlite3 *db, const char *expr, GSList **list_r);

int file_load(const struct mpdcron_config *conf, GKeyFile *fd);
void file_cleanup(void);

#endif /* !MPDCRON_GUARD_STATS_DEFS_H */
