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

#include "eugene-defs.h"
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

static char *make_song_expr(const char *uri, const char *artist,
		const char *title, bool isexpr)
{
	int n;
	char *expr_str;
	char *esc_uri, *esc_artist, *esc_title;
	GString *expr;

	expr = g_string_new("");
	n = 0;
	if (uri != NULL) {
		esc_uri = escape_string(uri);
		g_string_append_printf(expr, "uri%s%s", isexpr ? " like " : "=", esc_uri);
		g_free(esc_uri);
		++n;
	}
	if (artist != NULL) {
		esc_artist = escape_string(artist);
		g_string_append_printf(expr, "%s artist%s%s ",
				(n > 0) ? "and" : "",
				isexpr ? " like " : "=",
				esc_artist);
		g_free(esc_artist);
		++n;
	}
	if (title != NULL) {
		esc_title = escape_string(title);
		g_string_append_printf(expr, "%s title%s%s ",
				(n > 0) ? "and" : "",
				isexpr ? " like " : "=",
				esc_title);
		g_free(esc_title);
	}
	g_string_append(expr, ";");
	expr_str = expr->str;
	g_string_free(expr, FALSE);
	return expr_str;
}

static int cb_get_first(void *pArg, G_GNUC_UNUSED int argc,
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

static int cb_love_count_matches(void *pArg, int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	unsigned *id = (unsigned *) pArg;

	g_assert(argc == 2);

	mpdcron_log(LOG_DEBUG, "uri='%s' love=%s",
			argv[0] ? argv[0] : "NULL",
			argv[1] ? argv[1] : "NULL");

	*id += 1;
	return SQLITE_OK;
}

static int cb_kill_count_matches(void *pArg, int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	unsigned *id = (unsigned *) pArg;

	g_assert(argc == 2);

	mpdcron_log(LOG_DEBUG, "uri='%s' kill=%s",
			argv[0] ? argv[0] : "NULL",
			argv[1] ? argv[1] : "NULL");

	*id += 1;
	return SQLITE_OK;
}

static int cb_rating_count_matches(void *pArg, int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	unsigned *id = (unsigned *) pArg;

	g_assert(argc == 2);

	mpdcron_log(LOG_DEBUG, "uri='%s' rating=%s",
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
	if (sqlite3_exec(db, SQL_VERSION, cb_get_first, &version, &errmsg) != SQLITE_OK) {
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

static int db_has_entry(sqlite3 *db, const char *uri,
		const char *artist, const char *title)
{
	int id;
	char *errmsg, *esc_uri, *esc_artist, *esc_title, *stmt;

	g_assert(db != NULL);
	g_assert(artist != NULL && title != NULL);

	esc_uri = escape_string(uri);
	esc_artist = escape_string(artist);
	esc_title = escape_string(title);

	stmt = g_strdup_printf("select db_id from SONG"
			" where uri=%s and artist=%s and title=%s",
			esc_uri, esc_artist, esc_title);

	g_free(esc_uri);
	g_free(esc_artist);
	g_free(esc_title);

	id = 0;
	if (sqlite3_exec(db, stmt, cb_check_entry, &id, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Error while trying to find entry (%s - %s): %s",
				artist, title, errmsg);
		g_free(stmt);
		sqlite3_free(errmsg);
		return -1;
	}
	g_free(stmt);
	return id;
}

static bool db_insert(sqlite3 *db, const struct mpd_song *song, bool increment)
{
	char *errmsg;
	GString *stmt;

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

	stmt = g_string_new("");
	g_string_printf(stmt,
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

	g_string_append_printf(stmt,
			"insert into ARTIST ("
				"play_count, name,"
				"love, kill, rating)"
				" values (%d, %s, 0, 0, 0);",
				increment ? 1 : 0, esc_artist);

	if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) != NULL)
		g_string_append_printf(stmt,
				"insert into ALBUM ("
					"play_count, artist, name,"
					"love, kill, rating)"
					" values (%d, %s, %s, 0, 0, 0);",
					increment ? 1 : 0, esc_artist, esc_album);
	if (mpd_song_get_tag(song, MPD_TAG_GENRE, 0) != NULL)
		g_string_append_printf(stmt,
				"insert into GENRE ("
					"play_count, name,"
					"love, kill, rating)"
					" values (%d, %s, 0, 0, 0);",
					increment ? 1 : 0, esc_genre);

	g_free(esc_uri); g_free(esc_artist); g_free(esc_album); g_free(esc_track);
	g_free(esc_name); g_free(esc_genre); g_free(esc_date); g_free(esc_composer);
	g_free(esc_performer); g_free(esc_disc);
	g_free(esc_mb_artistid); g_free(esc_mb_albumid); g_free(esc_mb_trackid);

	if (sqlite3_exec(db, stmt->str, NULL, NULL, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Failed to add song (%s - %s) to database: %s",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				errmsg);
		sqlite3_free(errmsg);
		g_string_free(stmt, TRUE);
		return false;
	}
	g_string_free(stmt, TRUE);

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
	GString *stmt;

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

	stmt = g_string_new("");
	g_string_printf(stmt,
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

	g_string_append_printf(stmt,
			"update ARTIST "
			"%s "
			"name=%s"
			" where db_id=%d;",
			increment ? "set play_count = play_count + 1," : "set",
			esc_artist, id);

	if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) != NULL)
		g_string_append_printf(stmt,
				"update ALBUM "
					"%s "
					"artist=%s,"
					"name=%s"
					" where db_id=%d;",
				increment ? "set play_count = play_count + 1," : "set",
				esc_artist, esc_album, id);
	if (mpd_song_get_tag(song, MPD_TAG_GENRE, 0) != NULL)
		g_string_append_printf(stmt,
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

	if (sqlite3_exec(db, stmt->str, NULL, NULL, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Failed to update information of song (%s - %s): %s",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				errmsg);
		sqlite3_free(errmsg);
		g_string_free(stmt, TRUE);
		return false;
	}
	g_string_free(stmt, TRUE);

	mpdcron_log(LOG_INFO, "Updated song (%s - %s)",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
	return true;
}

static bool db_selectsong_internal(sqlite3 *db,
		const char *elem, const char *expr,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	char *sql;

	g_assert(db != NULL);
	g_assert(elem != NULL);
	g_assert(expr != NULL);

	sql = g_strdup_printf("select %s from SONG where %s ;", elem, expr);
	if (sqlite3_exec(db, sql, callback, data, errmsg_r) != SQLITE_OK) {
		g_free(sql);
		return false;
	}
	return true;
}

static bool db_selectsong(sqlite3 *db, const char *elem,
		const char *uri, const char *artist,
		const char *title, int isexpr,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	bool ret;
	char *expr;

	expr = make_song_expr(uri, artist, title, isexpr);
	ret = db_selectsong_internal(db, elem, expr, callback, data, errmsg_r);
	g_free(expr);
	return ret;
}

static bool db_selectsong_song(sqlite3 *db, const char *elem, const struct mpd_song *song,
		int (*callback)(void *, int, char **, char **),
		void *data,
		char **errmsg_r)
{
	g_assert(db != NULL);
	if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL ||
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0) == NULL) {
		mpdcron_log(LOG_WARNING, "Song (%s) doesn't have required tags, skipping",
				mpd_song_get_uri(song));
		return true;
	}

	return db_selectsong(db, elem,
			mpd_song_get_uri(song),
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			false, callback, data, errmsg_r);
}

static bool db_updatesong_internal(sqlite3 *db, const char *stmt,
		const char *expr, char **errmsg_r)
{
	char *sql;

	g_assert(db != NULL);
	g_assert(stmt != NULL);
	g_assert(expr != NULL);

	sql = g_strdup_printf("update SONG set %s where %s ;", stmt, expr);
	if (sqlite3_exec(db, sql, NULL, NULL, errmsg_r) != SQLITE_OK) {
		g_free(sql);
		return false;
	}
	g_free(sql);
	return true;
}

static bool db_updatesong(sqlite3 *db, const char *stmt, int isexpr,
		const char *uri, const char *artist, const char *title,
		char **errmsg_r)
{
	bool ret;
	char *expr;

	g_assert(db != NULL);
	g_assert(uri != NULL || artist != NULL || title != NULL);

	expr = make_song_expr(uri, artist, title, isexpr);
	ret = db_updatesong_internal(db, stmt, expr, errmsg_r);
	g_free(expr);
	return ret;
}

static bool db_updatesong_song(sqlite3 *db, const char *stmt,
		const struct mpd_song *song, char **errmsg_r)
{
	int id;

	if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL ||
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0) == NULL) {
		mpdcron_log(LOG_WARNING, "Song (%s) doesn't have required tags, skipping",
				mpd_song_get_uri(song));
		return true;
	}

	/* Check if the song is already in the database */
	if ((id = db_has_entry(db,
			mpd_song_get_uri(song),
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0))) < 0) {
		sqlite3_close(db);
		return false;
	}

	/* Add the song if it doesn't exist */
	if (id == 0) {
		if (!db_insert(db, song, false)) {
			sqlite3_close(db);
			return false;
		}
	}

	return db_updatesong(db, stmt, false,
			mpd_song_get_uri(song),
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			errmsg_r);
}

bool db_init(const char *path)
{
	bool ret;
	sqlite3 *db;

	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		mpdcron_log(LOG_DEBUG, "Creating database `%s'", path);
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

	if ((id = db_has_entry(db,
			mpd_song_get_uri(song),
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0))) < 0) {
		sqlite3_close(db);
		return false;
	}

	if (id > 0)
		ret = db_update(db, song, id, increment);
	else
		ret = db_insert(db, song, increment);
	sqlite3_close(db);
	return ret;
}

bool db_lovesong(const char *path, const struct mpd_song *song, bool love)
{
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!db_updatesong_song(db, stmt,
				song, &errmsg)) {
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
	sqlite3_close(db);

	mpdcron_log(LOG_NOTICE, "%sd current playing song (%s - %s), id: %u, pos: %u",
			love ? "Love" : "Hate",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			mpd_song_get_id(song),
			mpd_song_get_pos(song));
	return true;
}

bool db_lovesong_uri(const char *path, const char *uri, bool love, bool isexpr, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(uri != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!db_updatesong(db, stmt, isexpr, uri, NULL, NULL, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s %s %s: %s",
				love ? "love" : "hate",
				isexpr ? "songs matching expression" : "song",
				uri, errmsg);
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

	/* Count number of matches */
	char *esc_uri = escape_string(uri);
	stmt = g_strdup_printf("select uri, love from SONG"
				" where uri%s%s ;", isexpr ? " like " : "=", esc_uri);
	g_free(esc_uri);

	if (sqlite3_exec(db, stmt, cb_love_count_matches, &count, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Failed to count matches: %s", errmsg);
		sqlite3_free(errmsg);
		g_free(stmt);
		return false;
	}
	g_free(stmt);
	sqlite3_close(db);
	mpdcron_log(LOG_NOTICE, "%sd %d song%s %s",
			love ? "Love" : "Hate",
			count,
			isexpr ? "s matching expression" : "s with the name",
			uri);
	return true;
}

bool db_lovesong_expr(const char *path, const char *expr, bool love, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!db_updatesong_internal(db, stmt, expr, &errmsg)) {
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

	if (!db_selectsong_internal(db, "uri, love", expr,
				cb_love_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression %s didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "%sd %d songs matching expression %s",
			love ? "Love" : "Hate",
			count, expr);
	return true;
}

bool db_killsong(const char *path, const struct mpd_song *song, bool kkill)
{
	char *errmsg, *stmt;
	sqlite3 *db;

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!db_updatesong_song(db, stmt,
				song, &errmsg)) {
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

bool db_killsong_uri(const char *path, const char *uri, bool kkill, bool isexpr, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(uri != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!db_updatesong(db, stmt, isexpr, uri, NULL, NULL, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to %s %s %s: %s",
				kkill ? "kill" : "unkill",
				isexpr ? "songs matching expression" : "song",
				uri, errmsg);
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

	/* Count number of matches */
	char *esc_uri = escape_string(uri);
	stmt = g_strdup_printf("select uri, kill from SONG"
				" where uri%s%s ;", isexpr ? " like " : "=", esc_uri);
	g_free(esc_uri);

	if (sqlite3_exec(db, stmt, cb_kill_count_matches, &count, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Failed to count matches: %s", errmsg);
		sqlite3_free(errmsg);
		g_free(stmt);
		return false;
	}
	g_free(stmt);
	sqlite3_close(db);
	mpdcron_log(LOG_NOTICE, "%sed %d song%s %s",
			kkill ? "Kill" : "Unkill",
			count,
			isexpr ? "s matching expression" : "s",
			uri);
	return true;
}

bool db_killsong_expr(const char *path, const char *expr, bool kkill, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!db_updatesong_internal(db, stmt, expr, &errmsg)) {
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

	if (!db_selectsong_internal(db, "uri, kill", expr,
				cb_kill_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list matches: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression %s didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "%sed %d songs matching expression %s",
			kkill ? "Kill" : "Unkill",
			count, expr);
	return true;
}

bool db_ratesong(const char *path, const struct mpd_song *song, long rating,
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
	if (!db_updatesong_song(db, stmt,
				song, &errmsg)) {
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
	if (!db_selectsong_song(db, "rating", song,
				cb_get_first, &newrating, &errmsg)) {
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

bool db_ratesong_uri(const char *path, const char *uri, long rating,
		bool isexpr, bool add, bool wantcount)
{
	int count = 0;
	char *errmsg, *stmt;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(uri != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	stmt = (!add)
		? g_strdup_printf("rating = %ld", rating)
		: g_strdup_printf("rating = rating + (%ld)", rating);
	if (!db_updatesong(db, stmt, isexpr, uri, NULL, NULL, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to rate %s %s: %s",
				isexpr ? "songs matching expression" : "song",
				uri, errmsg);
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

	/* Count number of matches */
	char *esc_uri = escape_string(uri);
	stmt = g_strdup_printf("select uri, rating from SONG"
				" where uri%s%s ;", isexpr ? " like " : "=", esc_uri);
	g_free(esc_uri);

	if (sqlite3_exec(db, stmt, cb_rating_count_matches, &count, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Failed to count matches: %s", errmsg);
		sqlite3_free(errmsg);
		g_free(stmt);
		return false;
	}
	g_free(stmt);
	sqlite3_close(db);
	mpdcron_log(LOG_NOTICE, "Rated %d song%s %s",
			count,
			isexpr ? "s matching expression" : "s",
			uri);
	return true;
}

bool db_ratesong_expr(const char *path, const char *expr, long rating, bool add, bool wantcount)
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
	if (!db_updatesong_internal(db, stmt, expr, &errmsg)) {
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

	if (!db_selectsong_internal(db, "uri, kill", expr,
				cb_rating_count_matches, &count, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to list ratings: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}
	sqlite3_close(db);

	if (count == 0) {
		mpdcron_log(LOG_WARNING, "Expression %s didn't match anything", expr);
		return false;
	}

	mpdcron_log(LOG_NOTICE, "Rated %d songs matching expression %s",
			count, expr);
	return true;
}
