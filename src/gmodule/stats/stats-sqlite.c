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

#include "stats-sqlite.h"

#include <stdbool.h>
#include <stdlib.h>

#include <glib.h>
#include <libdaemon/dlog.h>
#include <mpd/client.h>
#include <sqlite3.h>

#define DB_VERSION	3
const char SQL_SET_VERSION[] = "PRAGMA user_version = 3";
const char SQL_VERSION[] = "PRAGMA user_version;";
const char SQL_SET_ENCODING[] = "PRAGMA encoding = \"UTF-8\";";
const char SQL_DB_CREATE[] =
	"create table SONG(\n"
		"\tdb_id           INTEGER PRIMARY KEY,\n"
		"\tdb_last_update  DATE,\n"
		"\tplay_count      INTEGER,\n"
		"\tlove            INTEGER,\n"
		"\tkill            INTEGER,\n"
		"\trating          INTEGER,\n"
		"\turi             TEXT,\n"
		"\tduration        INTEGER,\n"
		"\tlast_modified   INTEGER,\n"
		"\tartist          TEXT,\n"
		"\talbum           TEXT,\n"
		"\ttitle           TEXT,\n"
		"\ttrack           TEXT,\n"
		"\tname            TEXT,\n"
		"\tgenre           TEXT,\n"
		"\tdate            TEXT,\n"
		"\tcomposer        TEXT,\n"
		"\tperformer       TEXT,\n"
		"\tdisc            TEXT,\n"
		"\tmb_artistid     TEXT,\n"
		"\tmb_albumid      TEXT,\n"
		"\tmb_trackid      TEXT);\n"
	"create trigger insert_last_song_update after insert on SONG\n"
	"begin\n"
	"    update SONG set db_last_update = DATETIME('NOW')\n"
	"        where rowid = new.rowid;\n"
	"end;\n"
	"create table ARTIST(\n"
		"\tdb_id           INTEGER_PRIMARY_KEY,\n"
		"\tdb_last_update  DATE,\n"
		"\tplay_count      INTEGER,\n"
		"\tname            TEXT,\n"
		"\tlove            INTEGER,\n"
		"\tkill            INTEGER,\n"
		"\trating          INTEGER);\n"
	"create trigger insert_last_artist_update after insert on ARTIST\n"
	"begin\n"
	"    update ARTIST set db_last_update = DATETIME('NOW')\n"
	"        where rowid = new.rowid;\n"
	"end;\n"
	"create table ALBUM(\n"
		"\tdb_id           INTEGER_PRIMARY_KEY,\n"
		"\tdb_last_update  DATE,\n"
		"\tplay_count      INTEGER,\n"
		"\tartist          TEXT,\n"
		"\tname            TEXT,\n"
		"\tlove            INTEGER,\n"
		"\tkill            INTEGER,\n"
		"\trating          INTEGER);\n"
	"create trigger insert_last_album_update after insert on ALBUM\n"
	"begin\n"
	"    update ALBUM set db_last_update = DATETIME('NOW')\n"
	"        where rowid = new.rowid;\n"
	"end;\n"
	"create table GENRE(\n"
		"\tdb_id           INTEGER_PRIMARY_KEY,\n"
		"\tdb_last_update  DATE,\n"
		"\tplay_count      INTEGER,\n"
		"\tname            TEXT,\n"
		"\tlove            INTEGER,\n"
		"\tkill            INTEGER,\n"
		"\trating          INTEGER);"
	"create trigger insert_last_genre_update after insert on GENRE\n"
	"begin\n"
	"    update GENRE set db_last_update = DATETIME('NOW')\n"
	"        where rowid = new.rowid;\n"
	"end;\n";

/**
 * Utility Functions
 */
static GQuark db_quark(void)
{
	return g_quark_from_static_string("database");
}

static char *escape_string(const char *src)
{
	char *q, *dest;

	q = sqlite3_mprintf("%Q", src);
	dest = g_strdup(q);
	sqlite3_free(q);
	return dest;
}

static bool check_tags(const struct mpd_song *song, bool checkalbum, bool checkgenre)
{
	if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL ||
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0) == NULL ||
			(checkalbum && mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) == NULL) ||
			(checkgenre && mpd_song_get_tag(song, MPD_TAG_GENRE, 0) == NULL)) {
		return false;
	}
	return true;
}

static char *make_expr(const char *name, bool like)
{
	char *esc_name, *expr;

	g_assert(name != NULL);

	esc_name = escape_string(name);
	expr = g_strdup_printf("name%s%s", like ? " like " : "=", esc_name);
	g_free(esc_name);

	return expr;
}

static char *make_expr_song(const char *uri, const char *artist,
		const char *title, bool like)
{
	int n;
	char *expr_str;
	char *esc_uri, *esc_artist, *esc_title;
	GString *expr;

	g_assert(uri != NULL || artist != NULL || title != NULL);

	expr = g_string_new("");
	n = 0;
	if (uri != NULL) {
		esc_uri = escape_string(uri);
		g_string_append_printf(expr, "uri%s%s", like ? " like " : "=", esc_uri);
		g_free(esc_uri);
		++n;
	}
	if (artist != NULL) {
		esc_artist = escape_string(artist);
		g_string_append_printf(expr, "%s artist%s%s ",
				(n > 0) ? "and" : "",
				like ? " like " : "=",
				esc_artist);
		g_free(esc_artist);
		++n;
	}
	if (title != NULL) {
		esc_title = escape_string(title);
		g_string_append_printf(expr, "%s title%s%s ",
				(n > 0) ? "and" : "",
				like ? " like " : "=",
				esc_title);
		g_free(esc_title);
	}
	g_string_append(expr, ";");
	expr_str = expr->str;
	g_string_free(expr, FALSE);
	return expr_str;
}

/**
 * Callbacks
 */
static int cb_integer_first(void *pArg, G_GNUC_UNUSED int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	unsigned *id = (unsigned *) pArg;
	*id = (argv[0] != NULL) ? atoi(argv[0]) : 0;
	return SQLITE_OK;
}

static int cb_check_entry(void *pArg, G_GNUC_UNUSED int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	unsigned *id = (unsigned *) pArg;
	*id = (argv[0] != NULL) ? atoi(argv[0]) : 0;
	return SQLITE_OK;
}

static int cb_song_love_save(void *pArg, int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	char **message;
	GSList **values = (GSList **) pArg;

	g_assert(argc == 2);

	message = g_new(char *, 3);
	message[0] = g_strdup(argv[0]);
	message[1] = g_strdup(argv[1]);
	message[2] = NULL;

	*values = g_slist_prepend(*values, message);
	return SQLITE_OK;
}

/**
 * Database Queries
 */
static int db_has_name(sqlite3 *db, const char *tbl, const char *name, GError **error)
{
	int id;
	char *errmsg, *esc_name, *sql;

	g_assert(db != NULL);
	g_assert(tbl != NULL);
	g_assert(name != NULL);

	esc_name = escape_string(name);
	sql = g_strdup_printf("select db_id from %s where name=%s", tbl, esc_name);
	g_free(esc_name);

	/* The ID may be zero. */
	id = -1;
	if (sqlite3_exec(db, sql, cb_check_entry, &id, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_SELECT,
				"Error while trying to find (%s) from table %s: %s",
				name, tbl, errmsg);
		g_free(sql);
		sqlite3_free(errmsg);
		return -2;
	}
	g_free(sql);
	return id;
}

static inline int db_has_artist(sqlite3 *db, const char *name, GError **error)
{
	return db_has_name(db, "ARTIST", name, error);
}

static inline int db_has_album(sqlite3 *db, const char *name, GError **error)
{
	return db_has_name(db, "ALBUM", name, error);
}

static inline int db_has_genre(sqlite3 *db, const char *name, GError **error)
{
	return db_has_name(db, "GENRE", name, error);
}

static int db_has_song(sqlite3 *db, const char *uri, GError **error)
{
	int id;
	char *errmsg, *esc_uri, *sql;

	g_assert(db != NULL);

	esc_uri = escape_string(uri);
	sql = g_strdup_printf("select db_id from SONG"
			" where uri=%s",
			esc_uri);
	g_free(esc_uri);

	/* The ID can be zero */
	id = -1;
	if (sqlite3_exec(db, sql, cb_check_entry, &id, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_SELECT,
				"Error while trying to find song (%s): %s",
				uri, errmsg);
		g_free(sql);
		sqlite3_free(errmsg);
		return -2;
	}
	g_free(sql);
	return id;
}

/**
 * Database Inserts/Updates
 */
static bool db_insert_artist(sqlite3 *db, const struct mpd_song *song,
		bool increment, GError **error)
{
	char *errmsg, *esc_artist, *sql;

	g_assert(db != NULL);
	g_assert(song != NULL);

	esc_artist = escape_string(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
	sql = g_strdup_printf(""
			"insert into ARTIST ("
				"play_count, name,"
				"love, kill, rating)"
				" values (%d, %s, 0, 0, 0);",
				increment ? 1 : 0, esc_artist);
	g_free(esc_artist);

	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_INSERT,
				"Failed to add artist (%s) to database: %s",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				errmsg);
		sqlite3_free(errmsg);
		g_free(sql);
		return false;
	}
	g_free(sql);
	return true;
}

static bool db_insert_album(sqlite3 *db, const struct mpd_song *song,
		bool increment, GError **error)
{
	char *errmsg, *esc_album, *esc_artist, *sql;

	g_assert(db != NULL);
	g_assert(song != NULL);

	esc_album = escape_string(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
	esc_artist = escape_string(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
	sql = g_strdup_printf(""
			"insert into ALBUM ("
				"play_count, artist, name,"
				"love, kill, rating)"
				" values (%d, %s, %s, 0, 0, 0);",
				increment ? 1 : 0, esc_artist, esc_album);
	g_free(esc_album);
	g_free(esc_artist);

	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_INSERT,
				"Failed to add album (%s) to database: %s",
				mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				errmsg);
		sqlite3_free(errmsg);
		g_free(sql);
		return false;
	}
	g_free(sql);
	return true;
}

static bool db_insert_genre(sqlite3 *db, const struct mpd_song *song,
		bool increment, GError **error)
{
	char *errmsg, *esc_genre, *sql;

	g_assert(db != NULL);
	g_assert(song != NULL);

	esc_genre = escape_string(mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
	sql = g_strdup_printf(""
			"insert into GENRE ("
				"play_count, name,"
				"love, kill, rating)"
				" values (%d, %s, 0, 0, 0);",
				increment ? 1 : 0, esc_genre);
	g_free(esc_genre);

	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_INSERT,
				"Failed to add genre (%s) to database: %s",
				mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
				errmsg);
		sqlite3_free(errmsg);
		g_free(sql);
		return false;
	}
	g_free(sql);
	return true;
}

static bool db_insert_song(sqlite3 *db, const struct mpd_song *song,
		bool increment, GError **error)
{
	char *errmsg, *sql;

	g_assert(db != NULL);
	g_assert(song != NULL);

	char *esc_uri, *esc_artist, *esc_album, *esc_title;
	char *esc_track, *esc_name, *esc_genre, *esc_date;
	char *esc_composer, *esc_performer, *esc_disc;
	char *esc_mb_artistid, *esc_mb_albumid, *esc_mb_trackid;

	esc_uri = escape_string(mpd_song_get_uri(song));
	esc_artist = escape_string(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
	esc_album = escape_string(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
	esc_title = escape_string(mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
	esc_track = escape_string(mpd_song_get_tag(song, MPD_TAG_TRACK, 0));
	esc_name = escape_string(mpd_song_get_tag(song, MPD_TAG_NAME, 0));
	esc_genre = escape_string(mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
	esc_date = escape_string(mpd_song_get_tag(song, MPD_TAG_DATE, 0));
	esc_composer = escape_string(mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0));
	esc_performer = escape_string(mpd_song_get_tag(song, MPD_TAG_PERFORMER, 0));
	esc_disc = escape_string(mpd_song_get_tag(song, MPD_TAG_DISC, 0));
	esc_mb_artistid = escape_string(mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ARTISTID, 0));
	esc_mb_albumid = escape_string(mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMID, 0));
	esc_mb_trackid = escape_string(mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_TRACKID, 0));

	sql = g_strdup_printf(""
			"insert into SONG ("
				"play_count,"
				"love, kill, rating,"
				"uri, duration, last_modified,"
				"artist, album, title,"
				"track, name, genre,"
				"date, composer, performer, disc,"
				"mb_artistid, mb_albumid, mb_trackid)"
				" values (%d, 0, 0, 0,"
					"%s, %d, %lu, %s,"
					"%s, %s, %s, %s,"
					"%s, %s, %s, %s,"
					"%s, %s, %s, %s);",
			increment ? 1 : 0,
			esc_uri, mpd_song_get_duration(song), mpd_song_get_last_modified(song),
			esc_artist, esc_album, esc_title, esc_track, esc_name,
			esc_genre, esc_date, esc_composer, esc_performer, esc_disc,
			esc_mb_artistid, esc_mb_albumid, esc_mb_trackid);

	g_free(esc_uri); g_free(esc_artist); g_free(esc_album); g_free(esc_title);
	g_free(esc_track); g_free(esc_name); g_free(esc_genre); g_free(esc_date);
	g_free(esc_composer); g_free(esc_performer); g_free(esc_disc);
	g_free(esc_mb_artistid); g_free(esc_mb_albumid); g_free(esc_mb_trackid);

	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_INSERT,
				"Failed to add song (%s - %s) to database: %s",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				errmsg);
		sqlite3_free(errmsg);
		g_free(sql);
		return false;
	}
	g_free(sql);
	return true;
}

static bool db_update_artist(sqlite3 *db, const struct mpd_song *song, int id,
		bool increment, GError **error)
{
	char *errmsg, *esc_artist, *sql;

	g_assert(db != NULL);
	g_assert(song !=  NULL);

	esc_artist = escape_string(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
	sql = g_strdup_printf(""
			"update ARTIST "
			"%s "
			"name=%s"
			" where db_id=%d;",
			increment ? "set play_count = play_count + 1," : "set",
			esc_artist, id);
	g_free(esc_artist);

	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_UPDATE,
				"Failed to update information of artist (%s): %s",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				errmsg);
		sqlite3_free(errmsg);
		g_free(sql);
		return false;
	}
	g_free(sql);
	return true;
}

static bool db_update_album(sqlite3 *db, const struct mpd_song *song, int id,
		bool increment, GError **error)
{
	char *errmsg, *esc_album, *esc_artist, *sql;

	g_assert(db != NULL);
	g_assert(song != NULL);

	esc_album = escape_string(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
	esc_artist = escape_string(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
	sql = g_strdup_printf(""
			"update ALBUM "
			"%s "
			"artist=%s,"
			"name=%s"
			" where db_id=%d;",
			increment ? "set play_count = play_count + 1," : "set",
			esc_artist, esc_album, id);
	g_free(esc_album);
	g_free(esc_artist);

	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_UPDATE,
				"Failed to update information of album (%s): %s",
				mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				errmsg);
		sqlite3_free(errmsg);
		g_free(sql);
		return false;
	}
	g_free(sql);
	return true;
}

static bool db_update_genre(sqlite3 *db, const struct mpd_song *song, int id,
		bool increment, GError **error)
{
	char *errmsg, *esc_genre, *sql;

	g_assert(db != NULL);
	g_assert(song != NULL);

	esc_genre = escape_string(mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
	sql = g_strdup_printf(""
			"update GENRE "
			"%s "
			"name=%s"
			" where db_id=%d;",
			increment ? "set play_count = play_count + 1," : "set",
			esc_genre, id);
	g_free(esc_genre);

	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_UPDATE,
				"Failed to update information of genre (%s): %s",
				mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
				errmsg);
		sqlite3_free(errmsg);
		g_free(sql);
		return false;
	}
	g_free(sql);
	return true;
}

static bool db_update_song(sqlite3 *db, const struct mpd_song *song, int id,
		bool increment, GError **error)
{
	char *errmsg, *sql;

	g_assert(db != NULL);
	g_assert(song != NULL);

	char *esc_uri, *esc_artist, *esc_album, *esc_title;
	char *esc_track, *esc_name, *esc_genre, *esc_date;
	char *esc_composer, *esc_performer, *esc_disc;
	char *esc_mb_artistid, *esc_mb_albumid, *esc_mb_trackid;

	esc_uri = escape_string(mpd_song_get_uri(song));
	esc_artist = escape_string(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
	esc_album = escape_string(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
	esc_title = escape_string(mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
	esc_track = escape_string(mpd_song_get_tag(song, MPD_TAG_TRACK, 0));
	esc_name = escape_string(mpd_song_get_tag(song, MPD_TAG_NAME, 0));
	esc_genre = escape_string(mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
	esc_date = escape_string(mpd_song_get_tag(song, MPD_TAG_DATE, 0));
	esc_composer = escape_string(mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0));
	esc_performer = escape_string(mpd_song_get_tag(song, MPD_TAG_PERFORMER, 0));
	esc_disc = escape_string(mpd_song_get_tag(song, MPD_TAG_DISC, 0));
	esc_mb_artistid = escape_string(mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ARTISTID, 0));
	esc_mb_albumid = escape_string(mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMID, 0));
	esc_mb_trackid = escape_string(mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_TRACKID, 0));

	sql = g_strdup_printf(""
			"update SONG "
				"%s "
				"uri=%s,"
				"duration=%d,"
				"last_modified=%lu,"
				"artist=%s,"
				"album=%s,"
				"title=%s,"
				"track=%s,"
				"name=%s,"
				"genre=%s,"
				"date=%s,"
				"composer=%s,"
				"performer=%s,"
				"disc=%s,"
				"mb_artistid=%s,"
				"mb_albumid=%s,"
				"mb_trackid=%s"
				" where db_id=%d;",
			increment ? "set play_count = play_count + 1," : "set",
			esc_uri, mpd_song_get_duration(song), mpd_song_get_last_modified(song),
			esc_artist, esc_album, esc_title, esc_track, esc_name,
			esc_genre, esc_date, esc_composer, esc_performer, esc_disc,
			esc_mb_artistid, esc_mb_albumid, esc_mb_trackid, id);

	g_free(esc_uri); g_free(esc_artist); g_free(esc_album);
	g_free(esc_title); g_free(esc_track); g_free(esc_name);
	g_free(esc_genre); g_free(esc_date); g_free(esc_composer);
	g_free(esc_performer); g_free(esc_disc);
	g_free(esc_mb_artistid); g_free(esc_mb_albumid); g_free(esc_mb_trackid);

	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_UPDATE,
				"Failed to update information of song (%s - %s): %s",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				errmsg);
		sqlite3_free(errmsg);
		g_free(sql);
		return false;
	}
	g_free(sql);
	return true;
}

/**
 * Database Selects
 */
static bool sql_select_entry(sqlite3 *db,
		const char *tbl, const char *elem, const char *expr,
		int (*callback)(void *, int, char **, char **),
		void *data,
		GError **error)
{
	char *errmsg, *sql;

	g_assert(db != NULL);
	g_assert(tbl != NULL);
	g_assert(elem != NULL);
	g_assert(expr != NULL);

	sql = g_strdup_printf("select %s from %s where %s ;", elem, tbl, expr);
	if (sqlite3_exec(db, sql, callback, data, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_SELECT,
				"Select failed: %s",
				errmsg);
		sqlite3_free(errmsg);
		g_free(sql);
		return false;
	}
	g_free(sql);
	return true;
}

static inline bool sql_select_artist(sqlite3 *db,
		const char *elem, const char *expr,
		int (*callback)(void *, int, char **, char **),
		void *data,
		GError **error)
{
	return sql_select_entry(db, "ARTIST", elem, expr, callback, data, error);
}

static inline bool sql_select_album(sqlite3 *db,
		const char *elem, const char *expr,
		int (*callback)(void *, int, char **, char **),
		void *data,
		GError **error)
{
	return sql_select_entry(db, "ALBUM", elem, expr, callback, data, error);
}

static inline bool sql_select_genre(sqlite3 *db,
		const char *elem, const char *expr,
		int (*callback)(void *, int, char **, char **),
		void *data,
		GError **error)
{
	return sql_select_entry(db, "GENRE", elem, expr, callback, data, error);
}

static inline bool sql_select_song(sqlite3 *db,
		const char *elem, const char *expr,
		int (*callback)(void *, int, char **, char **),
		void *data,
		GError **error)
{
	return sql_select_entry(db, "SONG", elem, expr, callback, data, error);
}

static bool sql_select_artist_name(sqlite3 *db,
		const char *elem, const char *name,
		bool like,
		int (*callback)(void *, int, char **, char **),
		void *data,
		GError **error)
{
	bool ret;
	char *expr;

	expr = make_expr(name, like);
	ret = sql_select_artist(db, elem, expr, callback, data, error);
	g_free(expr);

	return ret;
}

static bool sql_select_album_name(sqlite3 *db,
		const char *elem, const char *name,
		bool like,
		int (*callback)(void *, int, char **, char **),
		void *data,
		GError **error)
{
	bool ret;
	char *expr;

	expr = make_expr(name, like);
	ret = sql_select_album(db, elem, expr, callback, data, error);
	g_free(expr);

	return ret;
}

static bool sql_select_genre_name(sqlite3 *db,
		const char *elem, const char *name,
		bool like,
		int (*callback)(void *, int, char **, char **),
		void *data,
		GError **error)
{
	bool ret;
	char *expr;

	expr = make_expr(name, like);
	ret = sql_select_genre(db, elem, expr, callback, data, error);
	g_free(expr);

	return ret;
}

static bool sql_select_song_uri(sqlite3 *db,
		const char *elem, const char *uri,
		const char *artist, const char *title,
		bool like,
		int (*callback)(void *, int, char **, char **),
		void *data,
		GError **error)
{
	bool ret;
	char *expr;

	expr = make_expr_song(uri, artist, title, like);
	ret = sql_select_song(db, elem, expr, callback, data, error);
	g_free(expr);

	return ret;
}

static bool sql_select_artist_song(sqlite3 *db, const char *elem,
		const struct mpd_song *song,
		int (*callback)(void *, int, char **, char **),
		void *data,
		GError **error)
{
	int id;

	if (!check_tags(song, false, false)) {
		g_set_error(error, db_quark(), ACK_ERROR_SONG_NO_TAGS,
				"Song (%s) doesn't have required tags",
				mpd_song_get_uri(song));
		return true;
	}

	/* Check if the artist is already in the database.
	 * Add her if she doesn't exist.
	 */
	if ((id = db_has_artist(db,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			error)) < -1)
		return false;
	else if (id == -1 && !db_insert_artist(db, song, false, error))
		return false;

	return sql_select_artist_name(db, elem,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			false, callback, data, error);
}

static bool sql_select_album_song(sqlite3 *db, const char *elem,
		const struct mpd_song *song,
		int (*callback)(void *, int, char **, char **),
		void *data,
		GError **error)
{
	int id;

	if (!check_tags(song, false, false)) {
		g_set_error(error, db_quark(), ACK_ERROR_SONG_NO_TAGS,
				"Song (%s) doesn't have required tags",
				mpd_song_get_uri(song));
		return true;
	}

	/* Check if the album is already in the database.
	 * Add it if it doesn't exist.
	 */
	if ((id = db_has_album(db,
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
			error)) < -1)
		return false;
	else if (id == -1 && !db_insert_album(db, song, false, error))
		return false;

	return sql_select_album_name(db, elem,
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
			false, callback, data, error);
}

static bool sql_select_genre_song(sqlite3 *db, const char *elem,
		const struct mpd_song *song,
		int (*callback)(void *, int, char **, char **),
		void *data,
		GError **error)
{
	int id;

	if (!check_tags(song, false, false)) {
		g_set_error(error, db_quark(), ACK_ERROR_SONG_NO_TAGS,
				"Song (%s) doesn't have required tags",
				mpd_song_get_uri(song));
		return true;
	}

	/* Check if the genre is already in the database.
	 * Add it if it doesn't exist.
	 */
	if ((id = db_has_genre(db,
			mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
			error)) < -1)
		return false;
	else if (id == -1 && !db_insert_genre(db, song, false, error))
		return false;

	return sql_select_genre_name(db, elem,
			mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
			false, callback, data, error);
}

static bool sql_select_song_song(sqlite3 *db, const char *elem,
		const struct mpd_song *song,
		int (*callback)(void *, int, char **, char **),
		void *data,
		GError **error)
{
	int id;

	if (!check_tags(song, false, false)) {
		g_set_error(error, db_quark(), ACK_ERROR_SONG_NO_TAGS,
				"Song (%s) doesn't have required tags",
				mpd_song_get_uri(song));
		return true;
	}

	/* Check if the song is already in the database.
	 * Add it if it doesn't exist.
	 */
	if ((id = db_has_song(db, mpd_song_get_uri(song), error)) < -1)
		return false;
	else if (id == -1 && !db_insert_song(db, song, false, error))
		return false;

	return sql_select_song_uri(db, elem,
			mpd_song_get_uri(song),
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			false, callback, data, error);
}

/**
 * Database Updates
 */
static bool sql_update_entry(sqlite3 *db, const char *tbl,
		const char *stmt, const char *expr,
		GError **error)
{
	char *errmsg, *sql;

	g_assert(db != NULL);
	g_assert(tbl != NULL);
	g_assert(stmt != NULL);
	g_assert(expr != NULL);

	sql = g_strdup_printf("update %s set %s where %s ;", tbl, stmt, expr);
	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_UPDATE,
				"Update failed: %s",
				errmsg);
		sqlite3_free(errmsg);
		g_free(sql);
		return false;
	}
	g_free(sql);
	return true;
}

static inline bool sql_update_artist(sqlite3 *db, const char *stmt,
		const char *expr, GError **error)
{
	return sql_update_entry(db, "ARTIST", stmt, expr, error);
}

static inline bool sql_update_album(sqlite3 *db, const char *stmt,
		const char *expr, GError **error)
{
	return sql_update_entry(db, "ALBUM", stmt, expr, error);
}

static inline bool sql_update_genre(sqlite3 *db, const char *stmt,
		const char *expr, GError **error)
{
	return sql_update_entry(db, "GENRE", stmt, expr, error);
}

static inline bool sql_update_song(sqlite3 *db, const char *stmt,
		const char *expr, GError **error)
{
	return sql_update_entry(db, "SONG", stmt, expr, error);
}

static bool sql_update_song_uri(sqlite3 *db, const char *stmt,
		const char *uri, const char *artist,
		const char *title, bool like, GError **error)
{
	bool ret;
	char *expr;

	g_assert(db != NULL);
	g_assert(uri != NULL);

	expr = make_expr_song(uri, artist, title, like);
	ret = sql_update_song(db, stmt, expr, error);
	g_free(expr);

	return ret;
}

static bool sql_update_song_song(sqlite3 *db, const char *stmt,
		const struct mpd_song *song, GError **error)
{
	int id;

	if (!check_tags(song, false, false)) {
		g_set_error(error, db_quark(), ACK_ERROR_SONG_NO_TAGS,
				"Song (%s) doesn't have required tags",
				mpd_song_get_uri(song));
		return true;
	}

	/* Check if the song is already in the database.
	 * Add it if it doesn't exist */
	if ((id = db_has_song(db, mpd_song_get_uri(song), error)) < -1)
		return false;
	else if (id == -1 && !db_insert_song(db, song, false, error))
		return false;

	return sql_update_song_uri(db, stmt,
			mpd_song_get_uri(song),
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			false, error);
}

/**
 * Database Maintenance
 */
static bool db_create(sqlite3 *db, GError **error)
{
	char *errmsg;

	g_assert(db != NULL);

	if (sqlite3_exec(db, SQL_DB_CREATE, NULL, NULL, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_CREATE,
				"Failed to create tables: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}

	/* Set encoding to UTF-8 */
	if (sqlite3_exec(db, SQL_SET_ENCODING, NULL, NULL, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_CREATE,
				"Failed to set encoding: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}

	/* Set user_version to our database version */
	if (sqlite3_exec(db, SQL_SET_VERSION, NULL, NULL, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_CREATE,
				"Failed to set user version: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}

	return true;
}

static bool db_check_ver(sqlite3 *db, GError **error)
{
	int version;
	char *errmsg;

	g_assert(db != NULL);

	/* Check version */
	if (sqlite3_exec(db, SQL_VERSION, cb_integer_first, &version, &errmsg) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_VERSION,
				"Failed to get version of database: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}
	else if (version != DB_VERSION) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_VERSION,
				"Database version mismatch: %d != %d",
				version, DB_VERSION);
		return false;
	}
	return true;
}

static sqlite3 *db_connect(const char *path, GError **error)
{
	g_assert(path != NULL);

	sqlite3 *db;
	if (sqlite3_open(path, &db) != 0) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_OPEN,
				"Failed to open database: %s",
				sqlite3_errmsg(db));
		return NULL;
	}

	/* Check version of the database */
	if (!db_check_ver(db, error)) {
		sqlite3_close(db);
		return NULL;
	}
	return db;
}

sqlite3 *db_init(const char *path, GError **error)
{
	sqlite3 *db;

	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		if (sqlite3_open(path, &db) != 0) {
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_OPEN,
					"%s", sqlite3_errmsg(db));
			return false;
		}

		if (!db_create(db, error)) {
			sqlite3_close(db);
			return NULL;
		}
		return db;
	}

	return db_connect(path, error);
}

void db_close(sqlite3 *db)
{
	sqlite3_close(db);
}

/**
 * Database Interaction
 */
bool db_process(sqlite3 *db, const struct mpd_song *song, bool increment, GError **error)
{
	int id;

	g_assert(db != NULL);
	g_assert(song != NULL);

	if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL ||
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0) == NULL) {
		g_set_error(error, db_quark(), ACK_ERROR_SONG_NO_TAGS,
				"Song (%s) doesn't have required tags",
				mpd_song_get_uri(song));
		return true;
	}

	if ((id = db_has_song(db, mpd_song_get_uri(song), error)) < -1)
		return false;
	else if (id == -1) {
		if (!db_insert_song(db, song, increment, error))
			return false;
	}
	else {
		if (!db_update_song(db, song, id, increment, error))
			return false;
	}

	if ((id = db_has_artist(db,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			error)) < -1)
		return false;
	else if (id == -1) {
		if (!db_insert_artist(db, song, increment, error))
			return false;
	}
	else
		if (!db_update_artist(db, song, id, increment, error))
			return false;

	if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) != NULL) {
		if ((id = db_has_album(db,
				mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				error)) < -1)
			return false;
		else if (id == -1) {
			if (!db_insert_album(db, song, increment, error))
				return false;
		}
		else {
			if (!db_update_album(db, song, id, increment, error))
				return false;
		}
	}

	if (mpd_song_get_tag(song, MPD_TAG_GENRE, 0) != NULL) {
		if ((id = db_has_genre(db,
				mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
				error)) < -1)
			return false;
		else if (id == -1) {
			if (!db_insert_genre(db, song, increment, error))
				return false;
		}
		else {
			if (!db_update_genre(db, song, id, increment, error))
				return false;
		}
	}

	return true;
}

/**
 * Main Interface
 */
bool db_love_song_uri(sqlite3 *db, const char *uri, bool love,
		int *value, GError **error)
{
	int newlove;
	char *stmt;

	g_assert(db != NULL);
	g_assert(uri != NULL);

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!sql_update_song_uri(db, stmt, uri, NULL, NULL, love, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (value == NULL)
		return true;

	newlove = 0;
	if (!sql_select_song_uri(db, "love", uri, NULL, NULL, false,
				cb_integer_first, &newlove, error))
		return false;

	*value = newlove;
	return true;
}

bool db_love_song_expr(sqlite3 *db, const char *expr, bool love,
		GSList **values, GError **error)
{
	char *stmt;

	g_assert(db != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!sql_update_song(db, stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (values == NULL)
		return true;

	if (!sql_select_song(db, "uri, love", expr, cb_song_love_save, values, error))
		return false;
	return true;
}

#if 0
bool db_love_song(sqlite3 *db, const struct mpd_song *song, bool love,
		int *value, GError **error)
{
	int newlove;
	char *stmt;

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!sql_update_song_song(db, stmt, song, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (value == NULL)
		return true;

	newlove = 0;
	if (!sql_select_song_song(db, "love", song, cb_integer_first,
				&newlove, error))
		return false;

	*value = newlove;
	return true;
}
#endif
