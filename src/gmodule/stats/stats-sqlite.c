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

#ifdef CAREFUL_WITH_THAT_AXE
#include "eugene-defs.h"
#endif /* CAREFUL_WITH_THAT_AXE */
#include "stats-defs.h"

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
		mpdcron_log(LOG_WARNING, "Song (%s) doesn't have required tags, skipping",
				mpd_song_get_uri(song));
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

static int cb_generic_love_count_matches(void *pArg, int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	unsigned *id = (unsigned *) pArg;

	g_assert(argc == 2);

	mpdcron_log(LOG_DEBUG, "name: (%s) love: %s",
			argv[0] ? argv[0] : "NULL",
			argv[1] ? argv[1] : "NULL");

	*id += 1;
	return SQLITE_OK;
}

static int cb_song_love_count_matches(void *pArg, int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	unsigned *id = (unsigned *) pArg;

	g_assert(argc == 2);

	mpdcron_log(LOG_DEBUG, "uri: (%s) love: %s",
			argv[0] ? argv[0] : "NULL",
			argv[1] ? argv[1] : "NULL");

	*id += 1;
	return SQLITE_OK;
}

static int cb_generic_kill_count_matches(void *pArg, int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	unsigned *id = (unsigned *) pArg;

	g_assert(argc == 2);

	mpdcron_log(LOG_DEBUG, "name: (%s) kill: %s",
			argv[0] ? argv[0] : "NULL",
			argv[1] ? argv[1] : "NULL");

	*id += 1;
	return SQLITE_OK;
}

static int cb_song_kill_count_matches(void *pArg, int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	unsigned *id = (unsigned *) pArg;

	g_assert(argc == 2);

	mpdcron_log(LOG_DEBUG, "uri: (%s) kill: %s",
			argv[0] ? argv[0] : "NULL",
			argv[1] ? argv[1] : "NULL");

	*id += 1;
	return SQLITE_OK;
}

static int cb_generic_rating_count_matches(void *pArg, int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	unsigned *id = (unsigned *) pArg;

	g_assert(argc == 2);

	mpdcron_log(LOG_DEBUG, "name: (%s) rating: %s",
			argv[0] ? argv[0] : "NULL",
			argv[1] ? argv[1] : "NULL");

	*id += 1;
	return SQLITE_OK;
}

static int cb_song_rating_count_matches(void *pArg, int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	unsigned *id = (unsigned *) pArg;

	g_assert(argc == 2);

	mpdcron_log(LOG_DEBUG, "uri: (%s) rating: %s",
			argv[0] ? argv[0] : "NULL",
			argv[1] ? argv[1] : "NULL");

	*id += 1;
	return SQLITE_OK;
}

static bool db_create(sqlite3 *db)
{
	char *errmsg;

	g_assert(db != NULL);

	if (sqlite3_exec(db, SQL_DB_CREATE, NULL, NULL, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Failed to create tables: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}

	/* Set encoding to UTF-8 */
	if (sqlite3_exec(db, SQL_SET_ENCODING, NULL, NULL, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Failed to set encoding: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}

	/* Set user_version to our database version */
	if (sqlite3_exec(db, SQL_SET_VERSION, NULL, NULL, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Failed to set user_version: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}

	return true;
}

static bool db_check_ver(sqlite3 *db)
{
	int version;
	char *errmsg;

	g_assert(db != NULL);

	/* Check version */
	if (sqlite3_exec(db, SQL_VERSION, cb_integer_first, &version, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Failed to get version of database: %s", errmsg);
		sqlite3_free(errmsg);
		return false;
	}
	else if (version != DB_VERSION) {
		mpdcron_log(LOG_ERR, "Database version %d doesn't match current version %d",
				version, DB_VERSION);
		return false;
	}
	return true;
}

static sqlite3 *db_connect(const char *path)
{
	g_assert(path != NULL);

	sqlite3 *db;
	if (sqlite3_open(path, &db) != 0) {
		mpdcron_log(LOG_ERR, "Failed to open database `%s': %s",
				path, sqlite3_errmsg(db));
		return NULL;
	}

	/* Check version of the database */
	if (!db_check_ver(db)) {
		sqlite3_close(db);
		return NULL;
	}
	return db;
}

static int db_has_name(sqlite3 *db, const char *tbl, const char *name)
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
		mpdcron_log(LOG_ERR, "Error while trying to find artist (%s): %s",
				name, errmsg);
		g_free(sql);
		sqlite3_free(errmsg);
		return -2;
	}
	g_free(sql);
	return id;
}

static inline int db_has_artist(sqlite3 *db, const char *name)
{
	return db_has_name(db, "ARTIST", name);
}

static inline int db_has_album(sqlite3 *db, const char *name)
{
	return db_has_name(db, "ALBUM", name);
}

static inline int db_has_genre(sqlite3 *db, const char *name)
{
	return db_has_name(db, "GENRE", name);
}

static int db_has_song(sqlite3 *db, const char *uri,
		const char *artist, const char *title)
{
	int id;
	char *errmsg, *esc_uri, *esc_artist, *esc_title, *sql;

	g_assert(db != NULL);
	g_assert(artist != NULL && title != NULL);

	esc_uri = escape_string(uri);
	esc_artist = escape_string(artist);
	esc_title = escape_string(title);

	sql = g_strdup_printf("select db_id from SONG"
			" where uri=%s and artist=%s and title=%s",
			esc_uri, esc_artist, esc_title);

	g_free(esc_uri);
	g_free(esc_artist);
	g_free(esc_title);

	/* The ID can be zero */
	id = -1;
	if (sqlite3_exec(db, sql, cb_check_entry, &id, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Error while trying to find song (%s - %s): %s",
				artist, title, errmsg);
		g_free(sql);
		sqlite3_free(errmsg);
		return -2;
	}
	g_free(sql);
	return id;
}

static bool db_insert(sqlite3 *db, const struct mpd_song *song, bool increment)
{
	char *errmsg;
	GString *sql;

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

	sql = g_string_new("");
	g_string_printf(sql,
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

	g_string_append_printf(sql,
			"insert into ARTIST ("
				"play_count, name,"
				"love, kill, rating)"
				" values (%d, %s, 0, 0, 0);",
				increment ? 1 : 0, esc_artist);

	if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) != NULL)
		g_string_append_printf(sql,
				"insert into ALBUM ("
					"play_count, artist, name,"
					"love, kill, rating)"
					" values (%d, %s, %s, 0, 0, 0);",
					increment ? 1 : 0, esc_artist, esc_album);
	if (mpd_song_get_tag(song, MPD_TAG_GENRE, 0) != NULL)
		g_string_append_printf(sql,
				"insert into GENRE ("
					"play_count, name,"
					"love, kill, rating)"
					" values (%d, %s, 0, 0, 0);",
					increment ? 1 : 0, esc_genre);

	g_free(esc_uri); g_free(esc_artist); g_free(esc_album); g_free(esc_track);
	g_free(esc_name); g_free(esc_genre); g_free(esc_date); g_free(esc_composer);
	g_free(esc_performer); g_free(esc_disc);
	g_free(esc_mb_artistid); g_free(esc_mb_albumid); g_free(esc_mb_trackid);

	if (sqlite3_exec(db, sql->str, NULL, NULL, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Failed to add song (%s - %s) to database: %s",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				errmsg);
		sqlite3_free(errmsg);
		g_string_free(sql, TRUE);
		return false;
	}
	g_string_free(sql, TRUE);

	mpdcron_log(LOG_INFO, "Added song (%s - %s) to database",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
	return true;
}

static bool db_update(sqlite3 *db, const struct mpd_song *song, int id, bool increment)
{
	char *errmsg;
	char *esc_uri, *esc_artist, *esc_album, *esc_title;
	char *esc_track, *esc_name, *esc_genre, *esc_date;
	char *esc_composer, *esc_performer, *esc_disc;
	char *esc_mb_artistid, *esc_mb_albumid, *esc_mb_trackid;
	GString *sql;

	g_assert(db != NULL);
	g_assert(song != NULL);

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

	sql = g_string_new("");
	g_string_printf(sql,
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

	g_string_append_printf(sql,
			"update ARTIST "
			"%s "
			"name=%s"
			" where db_id=%d;",
			increment ? "set play_count = play_count + 1," : "set",
			esc_artist, id);

	if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) != NULL)
		g_string_append_printf(sql,
				"update ALBUM "
					"%s "
					"artist=%s,"
					"name=%s"
					" where db_id=%d;",
				increment ? "set play_count = play_count + 1," : "set",
				esc_artist, esc_album, id);
	if (mpd_song_get_tag(song, MPD_TAG_GENRE, 0) != NULL)
		g_string_append_printf(sql,
				"update GENRE "
				"%s "
				"name=%s"
				" where db_id=%d;",
				increment ? "set play_count = play_count + 1," : "set",
				esc_genre, id);

	g_free(esc_uri); g_free(esc_artist); g_free(esc_album);
	g_free(esc_title); g_free(esc_track); g_free(esc_name);
	g_free(esc_genre); g_free(esc_date); g_free(esc_composer);
	g_free(esc_performer); g_free(esc_disc);
	g_free(esc_mb_artistid); g_free(esc_mb_albumid); g_free(esc_mb_trackid);

	if (sqlite3_exec(db, sql->str, NULL, NULL, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Failed to update information of song (%s - %s): %s",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				errmsg);
		sqlite3_free(errmsg);
		g_string_free(sql, TRUE);
		return false;
	}
	g_string_free(sql, TRUE);

	mpdcron_log(LOG_INFO, "Updated song (%s - %s)",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
	return true;
}

static bool db_select_entry(sqlite3 *db,
		const char *tbl, const char *elem, const char *expr,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	char *sql;

	g_assert(db != NULL);
	g_assert(tbl != NULL);
	g_assert(elem != NULL);
	g_assert(expr != NULL);

	sql = g_strdup_printf("select %s from %s where %s ;", elem, tbl, expr);
	if (sqlite3_exec(db, sql, callback, data, errmsg_r) != SQLITE_OK) {
		g_free(sql);
		return false;
	}
	g_free(sql);
	return true;
}

static inline bool db_select_artist(sqlite3 *db,
		const char *elem, const char *expr,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	return db_select_entry(db, "ARTIST", elem, expr, callback, data, errmsg_r);
}

static inline bool db_select_album(sqlite3 *db,
		const char *elem, const char *expr,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	return db_select_entry(db, "ALBUM", elem, expr, callback, data, errmsg_r);
}

static inline bool db_select_genre(sqlite3 *db,
		const char *elem, const char *expr,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	return db_select_entry(db, "GENRE", elem, expr, callback, data, errmsg_r);
}

static inline bool db_select_song(sqlite3 *db,
		const char *elem, const char *expr,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	return db_select_entry(db, "SONG", elem, expr, callback, data, errmsg_r);
}

static bool db_select_artist_name(sqlite3 *db,
		const char *elem, const char *name,
		bool like,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	bool ret;
	char *expr;

	expr = make_expr(name, like);
	ret = db_select_artist(db, elem, expr, callback, data, errmsg_r);
	g_free(expr);

	return ret;
}

static bool db_select_album_name(sqlite3 *db,
		const char *elem, const char *name,
		bool like,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	bool ret;
	char *expr;

	expr = make_expr(name, like);
	ret = db_select_album(db, elem, expr, callback, data, errmsg_r);
	g_free(expr);

	return ret;
}

static bool db_select_genre_name(sqlite3 *db,
		const char *elem, const char *name,
		bool like,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	bool ret;
	char *expr;

	expr = make_expr(name, like);
	ret = db_select_genre(db, elem, expr, callback, data, errmsg_r);
	g_free(expr);

	return ret;
}

static bool db_select_song_uri(sqlite3 *db,
		const char *elem, const char *uri,
		const char *artist, const char *title,
		bool like,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	bool ret;
	char *expr;

	expr = make_expr_song(uri, artist, title, like);
	ret = db_select_song(db, elem, expr, callback, data, errmsg_r);
	g_free(expr);

	return ret;
}

static bool db_select_artist_song(sqlite3 *db, const char *elem,
		const struct mpd_song *song,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	int id;

	if (!check_tags(song, false, false)) {
		/* No tags, ignore... */
		return true;
	}

	/* Check if the artist is already in the database.
	 * Add her if she doesn't exist.
	 */
	if ((id = db_has_artist(db,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0))) < -1)
		return false;
	else if (id == -1 && !db_insert(db, song, false))
		return false;

	return db_select_artist_name(db, elem,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			false, callback, data, errmsg_r);
}

static bool db_select_album_song(sqlite3 *db, const char *elem,
		const struct mpd_song *song,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	int id;

	if (!check_tags(song, false, false)) {
		/* No tags, ignore... */
		return true;
	}

	/* Check if the album is already in the database.
	 * Add it if it doesn't exist.
	 */
	if ((id = db_has_album(db,
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0))) < -1)
		return false;
	else if (id == -1 && !db_insert(db, song, false))
		return false;

	return db_select_album_name(db, elem,
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
			false, callback, data, errmsg_r);
}

static bool db_select_genre_song(sqlite3 *db, const char *elem,
		const struct mpd_song *song,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	int id;

	if (!check_tags(song, false, false)) {
		/* No tags, ignore... */
		return true;
	}

	/* Check if the genre is already in the database.
	 * Add it if it doesn't exist.
	 */
	if ((id = db_has_genre(db,
			mpd_song_get_tag(song, MPD_TAG_GENRE, 0))) < -1)
		return false;
	else if (id == -1 && !db_insert(db, song, false))
		return false;

	return db_select_genre_name(db, elem,
			mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
			false, callback, data, errmsg_r);
}

static bool db_select_song_song(sqlite3 *db, const char *elem,
		const struct mpd_song *song,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	int id;

	if (!check_tags(song, false, false)) {
		/* No tags, ignore... */
		return true;
	}

	/* Check if the song is already in the database.
	 * Add it if it doesn't exist.
	 */
	if ((id = db_has_song(db,
			mpd_song_get_uri(song),
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0))) < -1)
		return false;
	else if (id == -1 && !db_insert(db, song, false))
		return false;

	return db_select_song_uri(db, elem,
			mpd_song_get_uri(song),
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			false, callback, data, errmsg_r);
}

static bool db_update_entry(sqlite3 *db, const char *tbl,
		const char *stmt, const char *expr,
		char **errmsg_r)
{
	char *sql;

	g_assert(db != NULL);
	g_assert(tbl != NULL);
	g_assert(stmt != NULL);
	g_assert(expr != NULL);

	sql = g_strdup_printf("update %s set %s where %s ;", tbl, stmt, expr);
	if (sqlite3_exec(db, sql, NULL, NULL, errmsg_r) != SQLITE_OK) {
		g_free(sql);
		return false;
	}
	g_free(sql);
	return true;
}

static inline bool db_update_artist(sqlite3 *db, const char *stmt,
		const char *expr, char **errmsg_r)
{
	return db_update_entry(db, "ARTIST", stmt, expr, errmsg_r);
}

static inline bool db_update_album(sqlite3 *db, const char *stmt,
		const char *expr, char **errmsg_r)
{
	return db_update_entry(db, "ALBUM", stmt, expr, errmsg_r);
}

static inline bool db_update_genre(sqlite3 *db, const char *stmt,
		const char *expr, char **errmsg_r)
{
	return db_update_entry(db, "GENRE", stmt, expr, errmsg_r);
}

static inline bool db_update_song(sqlite3 *db, const char *stmt,
		const char *expr, char **errmsg_r)
{
	return db_update_entry(db, "SONG", stmt, expr, errmsg_r);
}

static bool db_update_song_uri(sqlite3 *db, const char *stmt,
		const char *uri, const char *artist,
		const char *title, bool like, char **errmsg_r)
{
	bool ret;
	char *expr;

	g_assert(db != NULL);
	g_assert(uri != NULL || artist != NULL || title != NULL);

	expr = make_expr_song(uri, artist, title, like);
	ret = db_update_song(db, stmt, expr, errmsg_r);
	g_free(expr);

	return ret;
}

static bool db_update_artist_song(sqlite3 *db, const char *stmt,
		const struct mpd_song *song, char **errmsg_r)
{
	bool ret;
	int id;
	char *esc_artist, *expr;

	if (!check_tags(song, false, false)) {
		/* No tags, ignore... */
		return true;
	}

	/* Check if the artist is already in the database.
	 * Add her if she doesn't exist.
	 */
	if ((id = db_has_artist(db,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0))) < -1)
		return false;
	else if (id == -1 && !db_insert(db, song, false))
		return false;

	esc_artist = escape_string(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
	expr = g_strdup_printf("name=%s", esc_artist);
	g_free(esc_artist);
	ret = db_update_artist(db, stmt, expr, errmsg_r);
	g_free(expr);
	return ret;
}

static bool db_update_album_song(sqlite3 *db, const char *stmt,
		const struct mpd_song *song, char **errmsg_r)
{
	bool ret;
	int id;
	char *esc_album, *expr;

	if (!check_tags(song, true, false)) {
		/* No tags, ignore... */
		return true;
	}

	/* Check if the album is already in the database.
	 * Add it if it doesn't exist.
	 */
	if ((id = db_has_album(db,
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0))) < -1)
		return false;
	else if (id == -1 && !db_insert(db, song, false))
		return false;

	esc_album = escape_string(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
	expr = g_strdup_printf("name=%s", esc_album);
	g_free(esc_album);
	ret = db_update_album(db, stmt, expr, errmsg_r);
	g_free(expr);
	return ret;
}

static bool db_update_genre_song(sqlite3 *db, const char *stmt,
		const struct mpd_song *song, char **errmsg_r)
{
	bool ret;
	int id;
	char *esc_genre, *expr;

	if (!check_tags(song, false, true)) {
		/* No tags, ignore... */
		return true;
	}

	/* Check if the genre is already in the database.
	 * Add it if it doesn't exist.
	 */
	if ((id = db_has_genre(db,
			mpd_song_get_tag(song, MPD_TAG_GENRE, 0))) < -1)
		return false;
	else if (id == -1 && !db_insert(db, song, false))
		return false;

	esc_genre = escape_string(mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
	expr = g_strdup_printf("name=%s", esc_genre);
	g_free(esc_genre);
	ret = db_update_genre(db, stmt, expr, errmsg_r);
	g_free(expr);
	return ret;
}


static bool db_update_song_song(sqlite3 *db, const char *stmt,
		const struct mpd_song *song, char **errmsg_r)
{
	int id;

	if (!check_tags(song, false, false)) {
		/* No tags, ignore... */
		return true;
	}

	/* Check if the song is already in the database.
	 * Add it if it doesn't exist */
	if ((id = db_has_song(db,
			mpd_song_get_uri(song),
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0))) < -1)
		return false;
	else if (id == -1 && !db_insert(db, song, false))
		return false;

	return db_update_song_uri(db, stmt,
			mpd_song_get_uri(song),
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			false, errmsg_r);
}

bool db_init(const char *path)
{
	bool ret;
	sqlite3 *db;

	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		mpdcron_log(LOG_NOTICE, "Creating database `%s'", path);
		if (sqlite3_open(path, &db) != 0) {
			mpdcron_log(LOG_ERR, "Failed to open database `%s': %s",
					path, sqlite3_errmsg(db));
			return false;
		}

		ret = db_create(db);
		sqlite3_close(db);
		return ret;
	}
	return true;
}

bool db_process(const char *path, const struct mpd_song *song, bool increment)
{
	bool ret;
	int id;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(song != NULL);

	if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL ||
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0) == NULL) {
		mpdcron_log(LOG_WARNING, "Song `%s' doesn't have required tags,"
				" not adding to database",
				mpd_song_get_uri(song));
		return true;
	}

	if ((db = db_connect(path)) == NULL)
		return false;

	if ((id = db_has_song(db,
			mpd_song_get_uri(song),
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0))) < -1) {
		sqlite3_close(db);
		return false;
	}

	if (id == -1)
		ret = db_insert(db, song, increment);
	else
		ret = db_update(db, song, id, increment);
	sqlite3_close(db);
	return ret;
}

bool db_love_artist(const char *path, const struct mpd_song *song, bool love, bool wantcount)
{
	int newlove;
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!db_update_artist_song(db, stmt, song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s artist (%s): %s",
				love ? "love" : "hate",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!wantcount) {
		sqlite3_close(db);
		mpdcron_log(LOG_NOTICE, "%sd current artist (%s)",
				love ? "Love" : "Hate",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
		return true;
	}

	newlove = 0;
	if (!db_select_artist_song(db, "love", song,
				cb_integer_first, &newlove, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to get love: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	mpdcron_log(LOG_NOTICE, "%sd current artist (%s), love: %d",
			love ? "Love" : "Hate",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0), newlove);
	return true;
}

bool db_love_artist_expr(const char *path, const char *expr, bool love, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!db_update_artist(db, stmt, expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s: %s", love ? "love" : "hate", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_select_artist(db, "name, love", expr,
				cb_generic_love_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression (%s) didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "%sd %d artists matching expression (%s)",
			love ? "Love" : "Hate",
			count, expr);
	return true;
}

bool db_love_album(const char *path, const struct mpd_song *song, bool love, bool wantcount)
{
	int newlove;
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!db_update_album_song(db, stmt, song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s album (%s): %s",
				love ? "love" : "hate",
				mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!wantcount) {
		sqlite3_close(db);
		mpdcron_log(LOG_NOTICE, "%sd current album (%s)",
				love ? "Love" : "Hate",
				mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
		return true;
	}

	newlove = 0;
	if (!db_select_album_song(db, "love", song,
				cb_integer_first, &newlove, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to get love: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	mpdcron_log(LOG_NOTICE, "%sd current album (%s), love: %d",
			love ? "Love" : "Hate",
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0), newlove);
	return true;
}

bool db_love_album_expr(const char *path, const char *expr, bool love, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!db_update_album(db, stmt, expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s: %s", love ? "love" : "hate", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_select_album(db, "name, love", expr,
				cb_generic_love_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression (%s) didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "%sd %d albums matching expression (%s)",
			love ? "Love" : "Hate",
			count, expr);
	return true;
}

bool db_love_genre(const char *path, const struct mpd_song *song, bool love, bool wantcount)
{
	int newlove;
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!db_update_genre_song(db, stmt, song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s genre (%s): %s",
				love ? "love" : "hate",
				mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
				errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!wantcount) {
		sqlite3_close(db);
		mpdcron_log(LOG_NOTICE, "%sd current album (%s)",
				love ? "Love" : "Hate",
				mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
		return true;
	}

	newlove = 0;
	if (!db_select_genre_song(db, "love", song,
				cb_integer_first, &newlove, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to get love: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	mpdcron_log(LOG_NOTICE, "%sd current genre (%s), love: %d",
			love ? "Love" : "Hate",
			mpd_song_get_tag(song, MPD_TAG_GENRE, 0), newlove);
	return true;
}

bool db_love_genre_expr(const char *path, const char *expr, bool love, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!db_update_genre(db, stmt, expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s: %s", love ? "love" : "hate", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_select_genre(db, "name, love", expr,
				cb_generic_love_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression (%s) didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "%sd %d genres matching expression (%s)",
			love ? "Love" : "Hate",
			count, expr);
	return true;
}

bool db_love_song(const char *path, const struct mpd_song *song, bool love, bool wantcount)
{
	int newlove;
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!db_update_song_song(db, stmt, song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s song (%s - %s): %s",
				love ? "love" : "hate",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!wantcount) {
		sqlite3_close(db);
		mpdcron_log(LOG_NOTICE, "%sd current playing song (%s - %s), id: %u, pos: %u",
				love ? "Love" : "Hate",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				mpd_song_get_id(song),
				mpd_song_get_pos(song));
		return true;
	}

	newlove = 0;
	if (!db_select_song_song(db, "love", song,
				cb_integer_first, &newlove, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to get love: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	mpdcron_log(LOG_NOTICE, "%sd current playing song (%s - %s), id: %u, pos: %u, love: %d",
			love ? "Love" : "Hate",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			mpd_song_get_id(song),
			mpd_song_get_pos(song), newlove);
	return true;
}

bool db_love_song_expr(const char *path, const char *expr, bool love, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!db_update_song(db, stmt, expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s: %s", love ? "love" : "hate", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_select_song(db, "uri, love", expr, cb_song_love_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression (%s) didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "%sd %d songs matching expression (%s)",
			love ? "Love" : "Hate",
			count, expr);
	return true;
}

bool db_kill_artist(const char *path, const struct mpd_song *song, bool kkill)
{
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!db_update_artist_song(db, stmt, song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s artist (%s): %s",
				kkill ? "kill" : "unkill",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);
	sqlite3_close(db);

	mpdcron_log(LOG_NOTICE, "%sed current playing artist (%s)",
			kkill ? "Kill" : "Unkill",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
	return true;
}

bool db_kill_artist_expr(const char *path, const char *expr, bool kkill, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!db_update_artist(db, stmt, expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s: %s", kkill ? "kill" : "unkill", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_select_artist(db, "uri, kill", expr, cb_generic_kill_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression (%s) didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "%sed %d artists matching expression (%s)",
			kkill ? "Kill" : "Unkill",
			count, expr);
	return true;
}

bool db_kill_album(const char *path, const struct mpd_song *song, bool kkill)
{
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!db_update_album_song(db, stmt, song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s album (%s): %s",
				kkill ? "kill" : "unkill",
				mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);
	sqlite3_close(db);

	mpdcron_log(LOG_NOTICE, "%sed current playing album (%s)",
			kkill ? "Kill" : "Unkill",
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
	return true;
}

bool db_kill_album_expr(const char *path, const char *expr, bool kkill, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!db_update_album(db, stmt, expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s: %s", kkill ? "kill" : "unkill", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_select_album(db, "uri, kill", expr, cb_generic_kill_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression (%s) didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "%sed %d albums matching expression (%s)",
			kkill ? "Kill" : "Unkill",
			count, expr);
	return true;
}

bool db_kill_genre(const char *path, const struct mpd_song *song, bool kkill)
{
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!db_update_genre_song(db, stmt, song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s genre (%s): %s",
				kkill ? "kill" : "unkill",
				mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
				errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	sqlite3_close(db);

	mpdcron_log(LOG_NOTICE, "%sed current playing genre (%s)",
			kkill ? "Kill" : "Unkill",
			mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
	return true;
}

bool db_kill_genre_expr(const char *path, const char *expr, bool kkill, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!db_update_genre(db, stmt, expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s: %s", kkill ? "kill" : "unkill", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_select_genre(db, "uri, kill", expr, cb_generic_kill_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression (%s) didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "%sed %d genres matching expression (%s)",
			kkill ? "Kill" : "Unkill",
			count, expr);
	return true;
}

bool db_kill_song(const char *path, const struct mpd_song *song, bool kkill)
{
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!db_update_song_song(db, stmt, song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s song (%s - %s): %s",
				kkill ? "kill" : "unkill",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);
	sqlite3_close(db);

	mpdcron_log(LOG_NOTICE, "%sed current playing song (%s - %s), id: %u, pos: %u",
			kkill ? "Kill" : "Unkill",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			mpd_song_get_id(song),
			mpd_song_get_pos(song));
	return true;
}

bool db_kill_song_expr(const char *path, const char *expr, bool kkill, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!db_update_song(db, stmt, expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s: %s", kkill ? "kill" : "unkill", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_select_song(db, "uri, kill", expr, cb_song_kill_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression (%s) didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "%sed %d songs matching expression (%s)",
			kkill ? "Kill" : "Unkill",
			count, expr);
	return true;
}

bool db_rate_artist(const char *path, const struct mpd_song *song, long rating,
		bool add, bool wantcount)
{
	int newrating;
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = (!add)
		? g_strdup_printf("rating = %ld", rating)
		: g_strdup_printf("rating = rating + (%ld)", rating);
	if (!db_update_artist_song(db, stmt, song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to rate artist (%s): %s",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!add && !wantcount) {
		sqlite3_close(db);
		mpdcron_log(LOG_NOTICE, "Rated current playing artist (%s), rating: %ld",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				rating);
		return true;
	}

	/* Get the rating of the song. */
	newrating = 0;
	if (!db_select_artist_song(db, "rating", song,
				cb_integer_first, &newrating, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to get rating: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	mpdcron_log(LOG_NOTICE, "Rated current playing artist (%s), rating: %d",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			newrating);

	return true;
}

bool db_rate_artist_expr(const char *path, const char *expr,
		long rating, bool add, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = (!add)
		? g_strdup_printf("rating = %ld", rating)
		: g_strdup_printf("rating = rating + (%ld)", rating);
	if (!db_update_artist(db, stmt, expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to rate: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!add && !wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_select_artist(db, "name, rating", expr, cb_generic_rating_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression (%s) didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "Rated %d artists matching expression (%s)", count, expr);
	return true;
}

bool db_rate_album(const char *path, const struct mpd_song *song, long rating,
		bool add, bool wantcount)
{
	int newrating;
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = (!add)
		? g_strdup_printf("rating = %ld", rating)
		: g_strdup_printf("rating = rating + (%ld)", rating);
	if (!db_update_album_song(db, stmt, song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to rate album (%s): %s",
				mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!add && !wantcount) {
		sqlite3_close(db);
		mpdcron_log(LOG_NOTICE, "Rated current playing album (%s), rating: %ld",
				mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				rating);
		return true;
	}

	/* Get the rating of the song. */
	newrating = 0;
	if (!db_select_album_song(db, "rating", song,
				cb_integer_first, &newrating, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to get rating: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	mpdcron_log(LOG_NOTICE, "Rated current playing album (%s), rating: %d",
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
			newrating);

	return true;
}

bool db_rate_album_expr(const char *path, const char *expr,
		long rating, bool add, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = (!add)
		? g_strdup_printf("rating = %ld", rating)
		: g_strdup_printf("rating = rating + (%ld)", rating);
	if (!db_update_album(db, stmt, expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to rate: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!add && !wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_select_album(db, "name, rating", expr, cb_generic_rating_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression (%s) didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "Rated %d albums matching expression (%s)", count, expr);
	return true;
}

bool db_rate_genre(const char *path, const struct mpd_song *song, long rating,
		bool add, bool wantcount)
{
	int newrating;
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = (!add)
		? g_strdup_printf("rating = %ld", rating)
		: g_strdup_printf("rating = rating + (%ld)", rating);
	if (!db_update_genre_song(db, stmt, song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to rate genre (%s): %s",
				mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
				errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!add && !wantcount) {
		sqlite3_close(db);
		mpdcron_log(LOG_NOTICE, "Rated current playing genre (%s), rating: %ld",
				mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
				rating);
		return true;
	}

	/* Get the rating of the song. */
	newrating = 0;
	if (!db_select_genre_song(db, "rating", song,
				cb_integer_first, &newrating, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to get rating: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	mpdcron_log(LOG_NOTICE, "Rated current playing genre (%s), rating: %d",
			mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
			newrating);

	return true;
}

bool db_rate_genre_expr(const char *path, const char *expr,
		long rating, bool add, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = (!add)
		? g_strdup_printf("rating = %ld", rating)
		: g_strdup_printf("rating = rating + (%ld)", rating);
	if (!db_update_genre(db, stmt, expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to rate: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!add && !wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_select_genre(db, "name, rating", expr, cb_generic_rating_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression (%s) didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "Rated %d genres matching expression (%s)", count, expr);
	return true;
}

bool db_rate_song(const char *path, const struct mpd_song *song, long rating,
		bool add, bool wantcount)
{
	int newrating;
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = (!add)
		? g_strdup_printf("rating = %ld", rating)
		: g_strdup_printf("rating = rating + (%ld)", rating);
	if (!db_update_song_song(db, stmt, song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to rate song (%s - %s): %s",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!add && !wantcount) {
		sqlite3_close(db);
		mpdcron_log(LOG_NOTICE, "Rated current playing song (%s - %s), id: %u, pos: %u, rating: %ld",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				mpd_song_get_id(song),
				mpd_song_get_pos(song),
				rating);
		return true;
	}

	/* Get the rating of the song. */
	newrating = 0;
	if (!db_select_song_song(db, "rating", song,
				cb_integer_first, &newrating, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to get rating: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	mpdcron_log(LOG_NOTICE, "Rated current playing song (%s - %s), id: %u, pos: %u, rating: %d",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			mpd_song_get_id(song),
			mpd_song_get_pos(song),
			newrating);

	return true;
}

bool db_rate_song_expr(const char *path, const char *expr,
		long rating, bool add, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = (!add)
		? g_strdup_printf("rating = %ld", rating)
		: g_strdup_printf("rating = rating + (%ld)", rating);
	if (!db_update_song(db, stmt, expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to rate: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (!add && !wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_select_song(db, "uri, rating", expr, cb_song_rating_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression (%s) didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "Rated %d songs matching expression (%s)", count, expr);
	return true;
}
