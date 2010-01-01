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
#include <time.h>

#include <glib.h>
#include <sqlite3.h>
#include <mpd/client.h>

struct db_generic_data {
	int id;
	int love;
	int kill;
	int rating;

	char *name;
	char *artist;
};

struct db_song_data {
	int id;			/** Id of the song */
	int play_count;		/** Play count of the song */
	int love;		/** Love count of the song */
	int kill;		/** Kill count of the song */
	int rating;		/** Rating of the song */

	char *uri;		/** Uri of the song */
	int duration;		/** Duration of the song */
	int last_modified;	/** Last modified date of the song */
	char *artist;		/** Artist of the song */
	char *album;		/** Album of the song */
	char *title;		/** Title of the song */
	char *track;		/** Track number of the song */
	char *name;		/** Name tag of the song */
	char *genre;		/** Genre of the song */
	char *date;		/** Date tag of the song */
	char *composer;		/** Composer of the song */
	char *performer;	/** Performer of the song */
	char *disc;		/** Disc number tag */
	char *mb_artist_id;	/** Musicbrainz artist ID */
	char *mb_album_id;	/** Musicbrainz album ID */
	char *mb_track_id;	/** Musicbrainz track ID */
};

enum dback {
	ACK_ERROR_DATABASE_OPEN = 100,
	ACK_ERROR_DATABASE_CREATE = 101,
	ACK_ERROR_DATABASE_VERSION = 102,
	ACK_ERROR_DATABASE_AUTH = 103,
	ACK_ERROR_DATABASE_INSERT = 104,
	ACK_ERROR_DATABASE_SELECT = 105,
	ACK_ERROR_DATABASE_UPDATE = 106,
	ACK_ERROR_DATABASE_PREPARE = 107,
	ACK_ERROR_DATABASE_BIND = 108,
	ACK_ERROR_DATABASE_STEP = 109,
	ACK_ERROR_DATABASE_RESET = 110,

	ACK_ERROR_NO_EXIST = 150,
	ACK_ERROR_NO_TAGS = 151,
};

/**
 * Database Interface
 */
void
db_generic_data_free(struct db_generic_data *data);

void
db_song_data_free(struct db_song_data *song);

bool
db_initialized(void);

bool
db_init(const char *path, bool create, bool readonly, GError **error);

void
db_close(void);

bool
db_set_authorizer(int (*xAuth)(void *, int, const char *, const char *, const char *,const char *),
		void *userdata, GError **error);

bool
db_start_transaction(GError **error);

bool
db_end_transaction(GError **error);

bool
db_process(const struct mpd_song *song, bool increment, GError **error);

bool
db_list_artist_expr(const char *expr, GSList **values, GError **error);

bool
db_list_album_expr(const char *expr, GSList **values, GError **error);

bool
db_list_genre_expr(const char *expr, GSList **values, GError **error);

bool
db_list_song_expr(const char *expr, GSList **values, GError **error);

bool
db_love_artist_expr(const char *expr, bool love, GSList **values, GError **error);

bool
db_love_album_expr(const char *expr, bool love, GSList **values, GError **error);

bool
db_love_genre_expr(const char *expr, bool love, GSList **values, GError **error);

bool
db_love_song_expr(const char *expr, bool love, GSList **values, GError **error);

bool
db_kill_artist_expr(const char *expr, bool kkill, GSList **values, GError **error);

bool
db_kill_album_expr(const char *expr, bool kkill, GSList **values, GError **error);

bool
db_kill_genre_expr(const char *expr, bool kkill, GSList **values, GError **error);

bool
db_kill_song_expr(const char *expr, bool kkill, GSList **values, GError **error);

bool
db_rate_artist_expr(const char *expr, int rating, GSList **values, GError **error);

bool
db_rate_album_expr(const char *expr, int rating, GSList **values, GError **error);

bool
db_rate_genre_expr(const char *expr, int rating, GSList **values, GError **error);

bool
db_rate_song_expr(const char *expr, int rating, GSList **values, GError **error);

#endif /* !MPDCRON_GUARD_STATS_SQLITE_H */
