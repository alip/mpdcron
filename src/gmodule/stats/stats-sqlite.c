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

#define DB_VERSION	1
const char SQL_SET_VERSION[] = "PRAGMA user_version = 1";
const char SQL_VERSION[] = "PRAGMA user_version;";
const char SQL_SET_ENCODING[] = "PRAGMA encoding = \"UTF-8\";";
const char SQL_DB_CREATE[] =
	"create table SONG(\n"
		"\tdb_id           INTEGER PRIMARY KEY,\n"
		"\tdb_last_update  DATE,\n"
		"\tdb_play_count   INTEGER,\n"
		"\tuser_love       INTEGER,\n"
		"\tuser_hate       INTEGER,\n"
		"\tuser_killed     INTEGER,\n"
		"\tuser_rating     INTEGER,\n"
		"\turi             TEXT,\n"
		"\tduration        INTEGER,\n"
		"\tlast_modified   INTEGER,\n"
		"\ttag_artist      TEXT,\n"
		"\ttag_album       TEXT,\n"
		"\ttag_title       TEXT,\n"
		"\ttag_track       TEXT,\n"
		"\ttag_name        TEXT,\n"
		"\ttag_genre       TEXT,\n"
		"\ttag_date        TEXT,\n"
		"\ttag_composer    TEXT,\n"
		"\ttag_performer   TEXT,\n"
		"\ttag_disc        TEXT,\n"
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
		"\tdb_play_count   INTEGER,\n"
		"\tname            TEXT,\n"
		"\tuser_love       INTEGER,\n"
		"\tuser_hate       INTEGER,\n"
		"\tuser_killed     INTEGER,\n"
		"\tuser_rating     INTEGER);\n"
	"create trigger insert_last_artist_update after insert on ARTIST\n"
	"begin\n"
	"    update ARTIST set db_last_update = DATETIME('NOW')\n"
	"        where rowid = new.rowid;\n"
	"end;\n"
	"create table ALBUM(\n"
		"\tdb_id           INTEGER_PRIMARY_KEY,\n"
		"\tdb_last_update  DATE,\n"
		"\tdb_play_count   INTEGER,\n"
		"\tartist          TEXT,\n"
		"\tname            TEXT,\n"
		"\tuser_love       INTEGER,\n"
		"\tuser_hate       INTEGER,\n"
		"\tuser_killed     INTEGER,\n"
		"\tuser_rating     INTEGER);\n"
	"create trigger insert_last_album_update after insert on ALBUM\n"
	"begin\n"
	"    update ALBUM set db_last_update = DATETIME('NOW')\n"
	"        where rowid = new.rowid;\n"
	"end;\n"
	"create table GENRE(\n"
		"\tdb_id           INTEGER_PRIMARY_KEY,\n"
		"\tdb_last_update  DATE,\n"
		"\tdb_play_count   INTEGER,\n"
		"\tname            TEXT,\n"
		"\tuser_love       INTEGER,\n"
		"\tuser_hate       INTEGER,\n"
		"\tuser_killed     INTEGER,\n"
		"\tuser_rating     INTEGER);"
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

static int cb_version(void *pArg, G_GNUC_UNUSED int argc,
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

	mpdcron_log(LOG_DEBUG, "Processed (uri='%s' user_love=%s)",
			argv[0] ? argv[0] : "NULL",
			argv[1] ? argv[1] : "NULL");

	*id += 1;
	return SQLITE_OK;
}

static int cb_hate_count_matches(void *pArg, int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	unsigned *id = (unsigned *) pArg;

	g_assert(argc == 2);

	mpdcron_log(LOG_DEBUG, "Processed (uri='%s' user_hate=%s)",
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
	if (sqlite3_exec(db, SQL_VERSION, cb_version, &version, &errmsg) != SQLITE_OK) {
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
			" where uri=%s and tag_artist=%s and tag_title=%s",
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

	char *esc_uri, *esc_tag_artist, *esc_tag_album, *esc_tag_title;
	char *esc_tag_track, *esc_tag_name, *esc_tag_genre, *esc_tag_date;
	char *esc_tag_composer, *esc_tag_performer, *esc_tag_disc;
	char *esc_mb_artistid, *esc_mb_albumid, *esc_mb_trackid;

	esc_uri = escape_string(mpd_song_get_uri(song));
	esc_tag_artist = escape_string(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
	esc_tag_album = escape_string(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
	esc_tag_title = escape_string(mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
	esc_tag_track = escape_string(mpd_song_get_tag(song, MPD_TAG_TRACK, 0));
	esc_tag_name = escape_string(mpd_song_get_tag(song, MPD_TAG_NAME, 0));
	esc_tag_genre = escape_string(mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
	esc_tag_date = escape_string(mpd_song_get_tag(song, MPD_TAG_DATE, 0));
	esc_tag_composer = escape_string(mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0));
	esc_tag_performer = escape_string(mpd_song_get_tag(song, MPD_TAG_PERFORMER, 0));
	esc_tag_disc = escape_string(mpd_song_get_tag(song, MPD_TAG_DISC, 0));
	esc_mb_artistid = escape_string(mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ARTISTID, 0));
	esc_mb_albumid = escape_string(mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMID, 0));
	esc_mb_trackid = escape_string(mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_TRACKID, 0));

	stmt = g_string_new("");
	g_string_printf(stmt,
			"insert into SONG ("
				"db_play_count,"
				"user_love, user_hate, user_killed, user_rating,"
				"uri, duration, last_modified,"
				"tag_artist, tag_album, tag_title,"
				"tag_track, tag_name, tag_genre,"
				"tag_date, tag_composer, tag_performer, tag_disc,"
				"mb_artistid, mb_albumid, mb_trackid)"
				" values (%d, 0, 0, 0, 0,"
					"%s, %d, %lu, %s,"
					"%s, %s, %s, %s,"
					"%s, %s, %s, %s,"
					"%s, %s, %s, %s);",
			increment ? 1 : 0,
			esc_uri, mpd_song_get_duration(song), mpd_song_get_last_modified(song),
			esc_tag_artist, esc_tag_album, esc_tag_title, esc_tag_track, esc_tag_name,
			esc_tag_genre, esc_tag_date, esc_tag_composer, esc_tag_performer, esc_tag_disc,
			esc_mb_artistid, esc_mb_albumid, esc_mb_trackid);

	g_string_append_printf(stmt,
			"insert into ARTIST ("
				"db_play_count, name,"
				"user_love, user_hate, user_killed, user_rating)"
				" values (%d, %s, 0, 0, 0, 0);",
				increment ? 1 : 0, esc_tag_artist);

	if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) != NULL)
		g_string_append_printf(stmt,
				"insert into ALBUM ("
					"db_play_count, artist, name,"
					"user_love, user_hate, user_killed, user_rating)"
					" values (%d, %s, %s, 0, 0, 0, 0);",
					increment ? 1 : 0, esc_tag_artist, esc_tag_album);
	if (mpd_song_get_tag(song, MPD_TAG_GENRE, 0) != NULL)
		g_string_append_printf(stmt,
				"insert into GENRE ("
					"db_play_count, name,"
					"user_love, user_hate, user_killed, user_rating)"
					" values (%d, %s, 0, 0, 0, 0);",
					increment ? 1 : 0, esc_tag_genre);

	g_free(esc_uri); g_free(esc_tag_artist); g_free(esc_tag_album); g_free(esc_tag_track);
	g_free(esc_tag_name); g_free(esc_tag_genre); g_free(esc_tag_date); g_free(esc_tag_composer);
	g_free(esc_tag_performer); g_free(esc_tag_disc);
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
	char *esc_uri, *esc_tag_artist, *esc_tag_album, *esc_tag_title;
	char *esc_tag_track, *esc_tag_name, *esc_tag_genre, *esc_tag_date;
	char *esc_tag_composer, *esc_tag_performer, *esc_tag_disc;
	char *esc_mb_artistid, *esc_mb_albumid, *esc_mb_trackid;
	GString *stmt;

	g_assert(db != NULL);
	g_assert(song != NULL);

	esc_uri = escape_string(mpd_song_get_uri(song));
	esc_tag_artist = escape_string(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
	esc_tag_album = escape_string(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
	esc_tag_title = escape_string(mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
	esc_tag_track = escape_string(mpd_song_get_tag(song, MPD_TAG_TRACK, 0));
	esc_tag_name = escape_string(mpd_song_get_tag(song, MPD_TAG_NAME, 0));
	esc_tag_genre = escape_string(mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
	esc_tag_date = escape_string(mpd_song_get_tag(song, MPD_TAG_DATE, 0));
	esc_tag_composer = escape_string(mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0));
	esc_tag_performer = escape_string(mpd_song_get_tag(song, MPD_TAG_PERFORMER, 0));
	esc_tag_disc = escape_string(mpd_song_get_tag(song, MPD_TAG_DISC, 0));
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
				"tag_artist=%s,"
				"tag_album=%s,"
				"tag_title=%s,"
				"tag_track=%s,"
				"tag_name=%s,"
				"tag_genre=%s,"
				"tag_date=%s,"
				"tag_composer=%s,"
				"tag_performer=%s,"
				"tag_disc=%s,"
				"mb_artistid=%s,"
				"mb_albumid=%s,"
				"mb_trackid=%s"
				" where db_id=%d;",
			increment ? "set db_play_count = db_play_count + 1," : "set",
			esc_uri, mpd_song_get_duration(song), mpd_song_get_last_modified(song),
			esc_tag_artist, esc_tag_album, esc_tag_title, esc_tag_track, esc_tag_name,
			esc_tag_genre, esc_tag_date, esc_tag_composer, esc_tag_performer, esc_tag_disc,
			esc_mb_artistid, esc_mb_albumid, esc_mb_trackid, id);

	g_string_append_printf(stmt,
			"update ARTIST "
			"%s "
			"name=%s"
			" where db_id=%d;",
			increment ? "set db_play_count = db_play_count + 1," : "set",
			esc_tag_artist, id);

	if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) != NULL)
		g_string_append_printf(stmt,
				"update ALBUM "
					"%s "
					"artist=%s,"
					"name=%s"
					" where db_id=%d;",
				increment ? "set db_play_count = db_play_count + 1," : "set",
				esc_tag_artist, esc_tag_album, id);
	if (mpd_song_get_tag(song, MPD_TAG_GENRE, 0) != NULL)
		g_string_append_printf(stmt,
				"update GENRE "
				"%s "
				"name=%s"
				" where db_id=%d;",
				increment ? "set db_play_count = db_play_count + 1," : "set",
				esc_tag_genre, id);

	g_free(esc_uri); g_free(esc_tag_artist); g_free(esc_tag_album);
	g_free(esc_tag_title); g_free(esc_tag_track); g_free(esc_tag_name);
	g_free(esc_tag_genre); g_free(esc_tag_date); g_free(esc_tag_composer);
	g_free(esc_tag_performer); g_free(esc_tag_disc);
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
		const char *uri, const char *tag_artist, const char *tag_title,
		char **errmsg_r)
{
	int n;
	bool ret;
	char *esc_uri, *esc_tag_artist, *esc_tag_title;
	GString *expr;

	g_assert(db != NULL);
	g_assert(uri != NULL || tag_artist != NULL || tag_title != NULL);

	expr = g_string_new("");
	n = 0;
	if (uri != NULL) {
		esc_uri = escape_string(uri);
		g_string_append_printf(expr, "uri%s%s", isexpr ? " like " : "=", esc_uri);
		g_free(esc_uri);
		++n;
	}
	if (tag_artist != NULL) {
		esc_tag_artist = escape_string(tag_artist);
		g_string_append_printf(expr, "%s tag_artist%s%s ",
				(n > 0) ? "and" : "",
				isexpr ? " like " : "=",
				esc_tag_artist);
		g_free(esc_tag_artist);
		++n;
	}
	if (tag_title != NULL) {
		esc_tag_title = escape_string(tag_title);
		g_string_append_printf(expr, "%s tag_title%s%s ",
				(n > 0) ? "and" : "",
				isexpr ? " like " : "=",
				esc_tag_title);
		g_free(esc_tag_title);
	}
	g_string_append(expr, ";");

	ret = db_updatesong_internal(db, stmt, expr->str, errmsg_r);
	g_string_free(expr, TRUE);
	return ret;
}

static bool db_updatesong_song(const char *path, const char *stmt,
		const struct mpd_song *song, char **errmsg_r)
{
	int id;
	sqlite3 *db;

	if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL ||
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0) == NULL) {
		mpdcron_log(LOG_WARNING, "Song (%s) doesn't have required tags, skipping",
				mpd_song_get_uri(song));
		return true;
	}

	if ((db = db_connect(path)) == NULL)
		return false;

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

bool db_love(const char *path, const struct mpd_song *song)
{
	char *errmsg;

	if (!db_updatesong_song(path, "user_love = user_love + 1",
				song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to love song (%s - %s): %s",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				errmsg);
		sqlite3_free(errmsg);
		return false;
	}

	mpdcron_log(LOG_INFO, "Loved song (%s - %s)",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
	return true;
}

bool db_love_uri(const char *path, const char *uri, int isexpr, int wantcount)
{
	int count = 0;
	char *errmsg;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(uri != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	if (!db_updatesong(db, "user_love = user_love + 1", isexpr, uri, NULL, NULL, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to love %s (%s): %s",
				isexpr ? "songs matching expression" : "song",
				uri, errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}

	if (!wantcount) {
		sqlite3_close(db);
		return true;
	}

	/* Count number of matches */
	char *esc_uri = escape_string(uri);
	char *stmt = g_strdup_printf(
			"select uri,user_love from SONG"
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
	mpdcron_log(LOG_INFO, "Loved %d song%s (%s)",
			count,
			isexpr ? "s matching expression" : "s",
			uri);
	return true;
}

bool db_love_expr(const char *path, const char *expr, int wantcount)
{
	int count = 0;
	char *errmsg;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	if (!db_updatesong_internal(db, "user_love = user_love + 1", expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to love: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}

	if (!wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_selectsong_internal(db, "uri, user_love", expr,
				cb_love_count_matches, &count, &errmsg)) {
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

	mpdcron_log(LOG_NOTICE, "Loved %d songs matching expression (%s)",
			count, expr);
	return true;
}

bool db_hate(const char *path, const struct mpd_song *song)
{
	char *errmsg;

	if (!db_updatesong_song(path, "user_hate = user_hate + 1",
				song, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to hate song (%s - %s): %s",
				mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
				errmsg);
		sqlite3_free(errmsg);
		return false;
	}

	mpdcron_log(LOG_INFO, "Hated song (%s - %s)",
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
	return true;
}

bool db_hate_uri(const char *path, const char *uri, int isexpr, int wantcount)
{
	int count = 0;
	char *errmsg;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(uri != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	if (!db_updatesong(db, "user_hate = user_hate + 1", isexpr, uri, NULL, NULL, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to hate %s (%s): %s",
				isexpr ? "songs matching expression" : "song",
				uri, errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}

	if (!wantcount) {
		sqlite3_close(db);
		return true;
	}

	/* Count number of matches */
	char *esc_uri = escape_string(uri);
	char *stmt = g_strdup_printf(
			"select uri,user_hate from SONG"
				" where uri%s%s ;", isexpr ? " like " : "=", esc_uri);
	g_free(esc_uri);

	if (sqlite3_exec(db, stmt, cb_hate_count_matches, &count, &errmsg) != SQLITE_OK) {
		mpdcron_log(LOG_ERR, "Failed to count matches: %s", errmsg);
		sqlite3_free(errmsg);
		g_free(stmt);
		return false;
	}
	g_free(stmt);
	sqlite3_close(db);
	mpdcron_log(LOG_INFO, "Hated %d song%s (%s)",
			count,
			isexpr ? "s matching expression" : "s",
			uri);
	return true;
}

bool db_hate_expr(const char *path, const char *expr, int wantcount)
{
	int count = 0;
	char *errmsg;
	sqlite3 *db;

	g_assert(path != NULL);
	g_assert(expr != NULL);

	if ((db = db_connect(path)) == NULL)
		return false;

	if (!db_updatesong_internal(db, "user_hate = user_hate + 1", expr, &errmsg)) {
		mpdcron_log(LOG_ERR, "Failed to hate: %s", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return false;
	}

	if (!wantcount) {
		sqlite3_close(db);
		return true;
	}

	if (!db_selectsong_internal(db, "uri, user_hate", expr,
				cb_hate_count_matches, &count, &errmsg)) {
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

	mpdcron_log(LOG_NOTICE, "Hated %d songs matching expression (%s)",
			count, expr);
	return true;
}
