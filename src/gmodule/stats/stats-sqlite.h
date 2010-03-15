/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009, 2010 Ali Polatel <alip@exherbo.org>
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
	int play_count;
	int love;
	int kill;
	int rating;

	char *name;
	char *artist;

	char **tags;
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

	char **tags;
};

enum dback {
	ACK_ERROR_DATABASE_OPEN = 50,
	ACK_ERROR_DATABASE_CREATE = 51,
	ACK_ERROR_DATABASE_VERSION = 52,
	ACK_ERROR_DATABASE_AUTH = 53,
	ACK_ERROR_DATABASE_INSERT = 54,
	ACK_ERROR_DATABASE_SELECT = 55,
	ACK_ERROR_DATABASE_UPDATE = 56,
	ACK_ERROR_DATABASE_PREPARE = 57,
	ACK_ERROR_DATABASE_BIND = 58,
	ACK_ERROR_DATABASE_STEP = 59,
	ACK_ERROR_DATABASE_RESET = 60,

	ACK_ERROR_INVALID_TAG = 101,
	ACK_ERROR_NO_TAGS = 102,
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
db_set_sync(bool on, GError **error);

bool
db_vacuum(GError **error);

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
db_listinfo_artist_expr(const char *expr, GSList **values, GError **error);

bool
db_listinfo_album_expr(const char *expr, GSList **values, GError **error);

bool
db_listinfo_genre_expr(const char *expr, GSList **values, GError **error);

bool
db_listinfo_song_expr(const char *expr, GSList **values, GError **error);

bool
db_count_artist_expr(const char *expr, int count, int *changes, GError **error);

bool
db_count_album_expr(const char *expr, int count, int *changes, GError **error);

bool
db_count_genre_expr(const char *expr, int count, int *changes, GError **error);

bool
db_count_song_expr(const char *expr, int count, int *changes, GError **error);

bool
db_love_artist_expr(const char *expr, bool love, int *changes, GError **error);

bool
db_love_album_expr(const char *expr, bool love, int *changes, GError **error);

bool
db_love_genre_expr(const char *expr, bool love, int *changes, GError **error);

bool
db_love_song_expr(const char *expr, bool love, int *changes, GError **error);

bool
db_kill_artist_expr(const char *expr, bool kkill, int *changes, GError **error);

bool
db_kill_album_expr(const char *expr, bool kkill, int *changes, GError **error);

bool
db_kill_genre_expr(const char *expr, bool kkill, int *changes, GError **error);

bool
db_kill_song_expr(const char *expr, bool kkill, int *changes, GError **error);

bool
db_rate_artist_expr(const char *expr, int rating, int *changes, GError **error);

bool
db_rate_album_expr(const char *expr, int rating, int *changes, GError **error);

bool
db_rate_genre_expr(const char *expr, int rating, int *changes, GError **error);

bool
db_rate_song_expr(const char *expr, int rating, int *changes, GError **error);

bool
db_count_absolute_artist_expr(const char *expr, int rating, int *changes, GError **error);

bool
db_count_absolute_album_expr(const char *expr, int rating, int *changes, GError **error);

bool
db_count_absolute_genre_expr(const char *expr, int rating, int *changes, GError **error);

bool
db_count_absolute_song_expr(const char *expr, int rating, int *changes, GError **error);

bool
db_add_artist_tag_expr(const char *expr, const char *tag, int *changes, GError **error);

bool
db_add_album_tag_expr(const char *expr, const char *tag, int *changes, GError **error);

bool
db_add_genre_tag_expr(const char *expr, const char *tag, int *changes, GError **error);

bool
db_add_song_tag_expr(const char *expr, const char *tag, int *changes, GError **error);

bool
db_remove_artist_tag_expr(const char *expr, const char *tag, int *changes, GError **error);

bool
db_remove_album_tag_expr(const char *expr, const char *tag, int *changes, GError **error);

bool
db_remove_genre_tag_expr(const char *expr, const char *tag, int *changes, GError **error);

bool
db_remove_song_tag_expr(const char *expr, const char *tag, int *changes, GError **error);

bool
db_list_artist_tag_expr(const char *expr, GSList **values, GError **error);

bool
db_list_album_tag_expr(const char *expr, GSList **values, GError **error);

bool
db_list_genre_tag_expr(const char *expr, GSList **values, GError **error);

bool
db_list_song_tag_expr(const char *expr, GSList **values, GError **error);

#endif /* !MPDCRON_GUARD_STATS_SQLITE_H */
