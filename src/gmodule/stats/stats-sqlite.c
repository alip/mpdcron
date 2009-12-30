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
#include <stdio.h> /* XXX debug */

#include <glib.h>
#include <libdaemon/dlog.h>
#include <mpd/client.h>
#include <sqlite3.h>

/**
 * Get/Set database version
 */
#define DB_VERSION	5
static const char SQL_SET_VERSION[] = "PRAGMA user_version = 5;";
static sqlite3_stmt *SQL_SET_VERSION_STMT = NULL;

static const char SQL_GET_VERSION[] = "PRAGMA user_version;";
static sqlite3_stmt *SQL_GET_VERSION_STMT = NULL;

/**
 * Set database encoding
 */
static const char SQL_SET_ENCODING[] = "PRAGMA encoding = \"UTF-8\";";
static sqlite3_stmt *SQL_SET_ENCODING_STMT = NULL;

/**
 * Create tables
 */
static const char SQL_DB_CREATE_SONG[] =
	"create table song(\n"
		"\tdb_id           INTEGER PRIMARY KEY,\n"
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
		"\tmb_trackid      TEXT);\n";
static sqlite3_stmt *SQL_DB_CREATE_SONG_STMT = NULL;

static const char SQL_DB_CREATE_ARTIST[] =
	"create table artist(\n"
		"\tdb_id           INTEGER_PRIMARY_KEY,\n"
		"\tplay_count      INTEGER,\n"
		"\tname            TEXT,\n"
		"\tlove            INTEGER,\n"
		"\tkill            INTEGER,\n"
		"\trating          INTEGER);\n";
static sqlite3_stmt *SQL_DB_CREATE_ARTIST_STMT = NULL;

static const char SQL_DB_CREATE_ALBUM[] =
	"create table album(\n"
		"\tdb_id           INTEGER_PRIMARY_KEY,\n"
		"\tplay_count      INTEGER,\n"
		"\tartist          TEXT,\n"
		"\tname            TEXT,\n"
		"\tlove            INTEGER,\n"
		"\tkill            INTEGER,\n"
		"\trating          INTEGER);\n";
static sqlite3_stmt *SQL_DB_CREATE_ALBUM_STMT = NULL;

static const char SQL_DB_CREATE_GENRE[] =
	"create table genre(\n"
		"\tdb_id           INTEGER_PRIMARY_KEY,\n"
		"\tplay_count      INTEGER,\n"
		"\tname            TEXT,\n"
		"\tlove            INTEGER,\n"
		"\tkill            INTEGER,\n"
		"\trating          INTEGER);";
static sqlite3_stmt *SQL_DB_CREATE_GENRE_STMT = NULL;

static const char SQL_HAS_SONG[] = "select db_id from song where uri=?";
static sqlite3_stmt *SQL_HAS_SONG_STMT = NULL;

static const char SQL_HAS_ARTIST[] = "select db_id from artist where name=?";
static sqlite3_stmt *SQL_HAS_ARTIST_STMT = NULL;

static const char SQL_HAS_ALBUM[] = "select db_id from album where name=?";
static sqlite3_stmt *SQL_HAS_ALBUM_STMT = NULL;

static const char SQL_HAS_GENRE[] = "select db_id from genre where name=?";
static sqlite3_stmt *SQL_HAS_GENRE_STMT = NULL;

static const char SQL_INSERT_SONG[] =
		"insert into song ("
			"play_count,"
			"love, kill, rating,"
			"uri, duration, last_modified,"
			"artist, album, title,"
			"track, name, genre,"
			"date, composer, performer, disc,"
			"mb_artistid, mb_albumid, mb_trackid)"
			" values (?, 0, 0, 0,"
				"?, ?, ?, ?,"
				"?, ?, ?, ?,"
				"?, ?, ?, ?,"
				"?, ?, ?, ?);";
static sqlite3_stmt *SQL_INSERT_SONG_STMT = NULL;

static const char SQL_UPDATE_SONG[] =
		"update song "
			"set play_count = play_count + ?,"
			"uri=?,"
			"duration=?,"
			"last_modified=?,"
			"artist=?,"
			"album=?,"
			"title=?,"
			"track=?,"
			"name=?,"
			"genre=?,"
			"date=?,"
			"composer=?,"
			"performer=?,"
			"disc=?,"
			"mb_artistid=?,"
			"mb_albumid=?,"
			"mb_trackid=?"
			" where db_id=?;";
static sqlite3_stmt *SQL_UPDATE_SONG_STMT = NULL;

static const char SQL_INSERT_ARTIST[] =
			"insert into artist ("
				"play_count, name,"
				"love, kill, rating)"
				" values (?, ?, 0, 0, 0);";
static sqlite3_stmt *SQL_INSERT_ARTIST_STMT = NULL;

static const char SQL_INSERT_ALBUM[] =
			"insert into album ("
				"play_count, artist, name,"
				"love, kill, rating)"
				" values (?, ?, ?, 0, 0, 0);";
static sqlite3_stmt *SQL_INSERT_ALBUM_STMT = NULL;

static const char SQL_INSERT_GENRE[] =
			"insert into genre ("
				"play_count, name,"
				"love, kill, rating)"
				" values (?, ?, 0, 0, 0);";
static sqlite3_stmt *SQL_INSERT_GENRE_STMT = NULL;

static const char SQL_UPDATE_ARTIST[] =
			"update artist "
			"set play_count = play_count + ?,"
			"name=? where db_id=?;";
static sqlite3_stmt *SQL_UPDATE_ARTIST_STMT = NULL;

static const char SQL_UPDATE_ALBUM[] =
			"update album "
			"set play_count = play_count + ?,"
			"artist=?, name=? where db_id=?;";
static sqlite3_stmt *SQL_UPDATE_ALBUM_STMT = NULL;

static const char SQL_UPDATE_GENRE[] =
			"update genre "
			"set play_count = play_count + ?,"
			"name=? where db_id=?;";
static sqlite3_stmt *SQL_UPDATE_GENRE_STMT = NULL;

/**
 * Utility Functions
 */
static GQuark db_quark(void)
{
	return g_quark_from_static_string("database");
}

G_GNUC_UNUSED
static char *escape_string(const char *src)
{
	char *q, *dest;

	q = sqlite3_mprintf("%Q", src);
	dest = g_strdup(q);
	sqlite3_free(q);
	return dest;
}

static int db_step(sqlite3_stmt *stmt)
{
	int ret;

	do {
		ret = sqlite3_step(stmt);
	} while (ret == SQLITE_BUSY);

	return ret;
}

/**
 * Callbacks
 */
static int cb_save(void *pArg, int argc,
		char **argv, G_GNUC_UNUSED char **columnName)
{
	int i;
	char **message;
	GSList **values = (GSList **) pArg;

	message = g_new(char *, argc + 1);
	for (i = 0; i < argc; i++)
		message[i] = g_strdup(argv[i]);
	message[i] = NULL;
	*values = g_slist_prepend(*values, message);
	return SQLITE_OK;
}

/**
 * Database Queries
 */
static int db_has_artist(sqlite3 *db, const char *name, GError **error)
{
	int id, ret;

	/* Reset the statement to its initial state */
	if (sqlite3_reset(SQL_HAS_ARTIST_STMT) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(db));
		return -2;
	}

	/* Bind the argument */
	if (sqlite3_bind_text(SQL_HAS_ARTIST_STMT, 1, name,
			-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(db));
		return -2;
	}

	/* Step and get the id */
	id = -1;
	do {
		ret = sqlite3_step(SQL_HAS_ARTIST_STMT);
		if (ret == SQLITE_ROW)
			id = sqlite3_column_int(SQL_HAS_ARTIST_STMT, 0);
	} while (ret == SQLITE_BUSY || ret == SQLITE_ROW);

	if (ret != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_SELECT,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return -2;
	}

	return id;
}

static int db_has_album(sqlite3 *db, const char *name, GError **error)
{
	int id, ret;

	/* Reset the statement to its initial state */
	if (sqlite3_reset(SQL_HAS_ALBUM_STMT) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(db));
		return -2;
	}

	/* Bind the argument */
	if (sqlite3_bind_text(SQL_HAS_ALBUM_STMT, 1, name,
			-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(db));
		return -2;
	}

	/* Step and get the id */
	id = -1;
	do {
		ret = sqlite3_step(SQL_HAS_ALBUM_STMT);
		if (ret == SQLITE_ROW)
			id = sqlite3_column_int(SQL_HAS_ALBUM_STMT, 0);
	} while (ret == SQLITE_BUSY || ret == SQLITE_ROW);

	if (ret != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_SELECT,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return -2;
	}

	return id;
}

static int db_has_genre(sqlite3 *db, const char *name, GError **error)
{
	int id, ret;

	/* Reset the statement to its initial state */
	if (sqlite3_reset(SQL_HAS_GENRE_STMT) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(db));
		return -2;
	}

	/* Bind the argument */
	if (sqlite3_bind_text(SQL_HAS_GENRE_STMT, 1, name,
			-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(db));
		return -2;
	}

	/* Step and get the id */
	id = -1;
	do {
		ret = sqlite3_step(SQL_HAS_GENRE_STMT);
		if (ret == SQLITE_ROW)
			id = sqlite3_column_int(SQL_HAS_GENRE_STMT, 0);
	} while (ret == SQLITE_BUSY || ret == SQLITE_ROW);

	if (ret != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_SELECT,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return -2;
	}

	return id;
}

static int db_has_song(sqlite3 *db, const char *name, GError **error)
{
	int id, ret;

	/* Reset the statement to its initial state */
	if (sqlite3_reset(SQL_HAS_SONG_STMT) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(db));
		return -2;
	}

	/* Bind the argument */
	if (sqlite3_bind_text(SQL_HAS_SONG_STMT, 1, name,
			-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(db));
		return -2;
	}

	/* Step and get the id */
	id = -1;
	do {
		ret = sqlite3_step(SQL_HAS_SONG_STMT);
		if (ret == SQLITE_ROW)
			id = sqlite3_column_int(SQL_HAS_SONG_STMT, 0);
	} while (ret == SQLITE_BUSY || ret == SQLITE_ROW);

	if (ret != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_SELECT,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return -2;
	}

	return id;
}

/**
 * Database Inserts/Updates
 */
static bool db_insert_artist(sqlite3 *db, const struct mpd_song *song,
		bool increment, GError **error)
{
	g_assert(db != NULL);
	g_assert(song != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(SQL_INSERT_ARTIST_STMT) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(db));
		return false;
	}

	if (sqlite3_bind_int(SQL_INSERT_ARTIST_STMT,
				1, increment ? 1 : 0) != SQLITE_OK
			|| sqlite3_bind_text(SQL_INSERT_ARTIST_STMT,
				2, mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(db));
		return false;
	}

	if (db_step(SQL_INSERT_ARTIST_STMT) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return false;
	}

	return true;
}

static bool db_insert_album(sqlite3 *db, const struct mpd_song *song,
		bool increment, GError **error)
{
	g_assert(db != NULL);
	g_assert(song != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(SQL_INSERT_ALBUM_STMT) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(db));
		return false;
	}

	if (sqlite3_bind_int(SQL_INSERT_ALBUM_STMT,
				1, increment ? 1 : 0) != SQLITE_OK
			|| sqlite3_bind_text(SQL_INSERT_ALBUM_STMT,
				2, mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				-1, SQLITE_STATIC) != SQLITE_OK
			|| sqlite3_bind_text(SQL_INSERT_ALBUM_STMT,
				3, mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(db));
		return false;
	}

	if (db_step(SQL_INSERT_ALBUM_STMT) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return false;
	}

	return true;
}

static bool db_insert_genre(sqlite3 *db, const struct mpd_song *song,
		bool increment, GError **error)
{
	g_assert(db != NULL);
	g_assert(song != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(SQL_INSERT_GENRE_STMT) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(db));
		return false;
	}

	if (sqlite3_bind_int(SQL_INSERT_GENRE_STMT,
				1, increment ? 1 : 0) != SQLITE_OK
			|| sqlite3_bind_text(SQL_INSERT_GENRE_STMT,
				2, mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
				-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(db));
		return false;
	}

	if (db_step(SQL_INSERT_GENRE_STMT) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return false;
	}

	return true;
}

static bool db_insert_song(sqlite3 *db, const struct mpd_song *song,
		bool increment, GError **error)
{
	g_assert(db != NULL);
	g_assert(song != NULL);

	if (sqlite3_reset(SQL_INSERT_SONG_STMT) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(db));
		return false;
	}

	if (sqlite3_bind_int(SQL_INSERT_SONG_STMT, 1, increment ? 1 : 0) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 2,
			mpd_song_get_uri(song),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_int(SQL_INSERT_SONG_STMT, 3,
			mpd_song_get_duration(song)) != SQLITE_OK
		|| sqlite3_bind_int(SQL_INSERT_SONG_STMT, 4,
			mpd_song_get_last_modified(song)) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 5,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 6,
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 7,
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 8,
			mpd_song_get_tag(song, MPD_TAG_TRACK, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 9,
			mpd_song_get_tag(song, MPD_TAG_NAME, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 10,
			mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 11,
			mpd_song_get_tag(song, MPD_TAG_DATE, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 12,
			mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 13,
			mpd_song_get_tag(song, MPD_TAG_PERFORMER, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 14,
			mpd_song_get_tag(song, MPD_TAG_DISC, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 15,
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ARTISTID, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 16,
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMID, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_INSERT_SONG_STMT, 17,
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_TRACKID, 0),
			-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(db));
		return false;
	}

	if (db_step(SQL_INSERT_SONG_STMT) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return false;
	}

	return true;
}

static bool db_update_artist(sqlite3 *db, const struct mpd_song *song,
		int id, bool increment, GError **error)
{
	g_assert(db != NULL);
	g_assert(song != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(SQL_UPDATE_ARTIST_STMT) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(db));
		return false;
	}

	if (sqlite3_bind_int(SQL_UPDATE_ARTIST_STMT,
				1, increment ? 1 : 0) != SQLITE_OK
			|| sqlite3_bind_text(SQL_UPDATE_ARTIST_STMT,
				2, mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				-1, SQLITE_STATIC) != SQLITE_OK
			|| sqlite3_bind_int(SQL_UPDATE_ARTIST_STMT,
				3, id) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(db));
		return false;
	}

	if (db_step(SQL_UPDATE_ARTIST_STMT) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return false;
	}

	return true;
}

static bool db_update_album(sqlite3 *db, const struct mpd_song *song,
		int id, bool increment, GError **error)
{
	g_assert(db != NULL);
	g_assert(song != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(SQL_UPDATE_ALBUM_STMT) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(db));
		return false;
	}

	if (sqlite3_bind_int(SQL_UPDATE_ALBUM_STMT,
				1, increment ? 1 : 0) != SQLITE_OK
			|| sqlite3_bind_text(SQL_UPDATE_ALBUM_STMT,
				2, mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				-1, SQLITE_STATIC) != SQLITE_OK
			|| sqlite3_bind_text(SQL_UPDATE_ALBUM_STMT,
				3, mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				-1, SQLITE_STATIC) != SQLITE_OK
			|| sqlite3_bind_int(SQL_UPDATE_ALBUM_STMT,
				4, id) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(db));
		return false;
	}

	if (db_step(SQL_UPDATE_ALBUM_STMT) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return false;
	}

	return true;
}

static bool db_update_genre(sqlite3 *db, const struct mpd_song *song,
		int id, bool increment, GError **error)
{
	g_assert(db != NULL);
	g_assert(song != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(SQL_UPDATE_GENRE_STMT) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(db));
		return false;
	}

	if (sqlite3_bind_int(SQL_UPDATE_GENRE_STMT,
				1, increment ? 1 : 0) != SQLITE_OK
			|| sqlite3_bind_text(SQL_UPDATE_GENRE_STMT,
				2, mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
				-1, SQLITE_STATIC) != SQLITE_OK
			|| sqlite3_bind_int(SQL_UPDATE_GENRE_STMT,
				3, id) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(db));
		return false;
	}

	if (db_step(SQL_UPDATE_GENRE_STMT) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return false;
	}

	return true;
}

static bool db_update_song(sqlite3 *db, const struct mpd_song *song, int id,
		bool increment, GError **error)
{
	int ret;

	g_assert(db != NULL);
	g_assert(song != NULL);

	if (sqlite3_reset(SQL_UPDATE_SONG_STMT) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(db));
		return false;
	}

	if (sqlite3_bind_int(SQL_UPDATE_SONG_STMT, 1, increment ? 1 : 0) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 2,
			mpd_song_get_uri(song),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_int(SQL_UPDATE_SONG_STMT, 3,
			mpd_song_get_duration(song)) != SQLITE_OK
		|| sqlite3_bind_int(SQL_UPDATE_SONG_STMT, 4,
			mpd_song_get_last_modified(song)) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 5,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 6,
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 7,
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 8,
			mpd_song_get_tag(song, MPD_TAG_TRACK, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 9,
			mpd_song_get_tag(song, MPD_TAG_NAME, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 10,
			mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 11,
			mpd_song_get_tag(song, MPD_TAG_DATE, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 12,
			mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 13,
			mpd_song_get_tag(song, MPD_TAG_PERFORMER, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 14,
			mpd_song_get_tag(song, MPD_TAG_DISC, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 15,
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ARTISTID, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 16,
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMID, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(SQL_UPDATE_SONG_STMT, 17,
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_TRACKID, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_int(SQL_UPDATE_SONG_STMT, 18, id)) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"%s", sqlite3_errmsg(db));
		return false;
	}

	do {
		ret = sqlite3_step(SQL_UPDATE_SONG_STMT);
	} while (ret == SQLITE_BUSY);

	if (ret != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"%s", sqlite3_errmsg(db));
		return false;
	}
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

/**
 * Database Maintenance
 */
static bool db_create(sqlite3 *db, GError **error)
{
	g_assert(db != NULL);

	/* Prepare statements */
	if (sqlite3_prepare_v2(db, SQL_SET_VERSION, -1,
			&SQL_SET_VERSION_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_GET_VERSION, -1,
			&SQL_GET_VERSION_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_SET_ENCODING, -1,
			&SQL_SET_ENCODING_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_DB_CREATE_SONG, -1,
			&SQL_DB_CREATE_SONG_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_DB_CREATE_ARTIST, -1,
			&SQL_DB_CREATE_ARTIST_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_DB_CREATE_ALBUM, -1,
			&SQL_DB_CREATE_ALBUM_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_DB_CREATE_GENRE, -1,
			&SQL_DB_CREATE_GENRE_STMT, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(db));
		sqlite3_close(db);
		return NULL;
	}

	/**
	 * Create tables
	 */
	if (db_step(SQL_DB_CREATE_SONG_STMT) != SQLITE_DONE
		|| db_step(SQL_DB_CREATE_ARTIST_STMT) != SQLITE_DONE
		|| db_step(SQL_DB_CREATE_ALBUM_STMT) != SQLITE_DONE
		|| db_step(SQL_DB_CREATE_GENRE_STMT) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_CREATE,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return false;
	}

	/**
	 * Set encoding
	 */
	if (db_step(SQL_SET_ENCODING_STMT) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_CREATE,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return false;
	}

	/**
	 * Set database version
	 */
	if (db_step(SQL_SET_VERSION_STMT) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_CREATE,
				"sqlite3_step: %s", sqlite3_errmsg(db));
		return false;
	}

	return true;
}

static bool db_check_ver(sqlite3 *db, GError **error)
{
	int ret, version;

	g_assert(db != NULL);

	/* Prepare statement */
	if (sqlite3_prepare_v2(db, SQL_GET_VERSION, -1,
			&SQL_GET_VERSION_STMT, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(db));
		return false;
	}

	/**
	 * Check version
	 */
	do {
		ret = sqlite3_step(SQL_GET_VERSION_STMT);
		if (ret == SQLITE_ROW)
			version = sqlite3_column_int(SQL_GET_VERSION_STMT, 0);
	} while (ret == SQLITE_BUSY || ret == SQLITE_ROW);

	if (ret != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_CREATE,
				"sqlite3_step: %s", sqlite3_errmsg(db));
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

sqlite3 *db_init(const char *path, bool create, bool readonly, GError **error)
{
	int flags;
	gboolean new;
	sqlite3 *db;

	new = !g_file_test(path, G_FILE_TEST_EXISTS);

	flags = 0;
	if (create && readonly) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_OPEN,
				"Invalid flags");
		return NULL;
	}
	flags |= (readonly ? SQLITE_OPEN_READONLY : SQLITE_OPEN_READWRITE);
	if (create)
		flags |= SQLITE_OPEN_CREATE;

	if (sqlite3_open_v2(path, &db, flags, NULL) != 0) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_OPEN,
				"sqlite3_open_v2: %s", sqlite3_errmsg(db));
		return NULL;
	}

	if (new) {
		if (!db_create(db, error)) {
			db_close(db);
			return NULL;
		}
		sqlite3_finalize(SQL_DB_CREATE_SONG_STMT);
		sqlite3_finalize(SQL_DB_CREATE_ARTIST_STMT);
		sqlite3_finalize(SQL_DB_CREATE_ALBUM_STMT);
		sqlite3_finalize(SQL_DB_CREATE_GENRE_STMT);
	}
	else {
		if (!db_check_ver(db, error)) {
			db_close(db);
			return NULL;
		}
		sqlite3_finalize(SQL_GET_VERSION_STMT);
	}

	/* Prepare common statements */
	if (sqlite3_prepare_v2(db, SQL_INSERT_SONG, -1,
			&SQL_INSERT_SONG_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_UPDATE_SONG, -1,
			&SQL_UPDATE_SONG_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_HAS_SONG, -1,
			&SQL_HAS_SONG_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_HAS_ARTIST, -1,
			&SQL_HAS_ARTIST_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_HAS_ALBUM, -1,
			&SQL_HAS_ALBUM_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_HAS_GENRE, -1,
			&SQL_HAS_GENRE_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_INSERT_ARTIST, -1,
			&SQL_INSERT_ARTIST_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_INSERT_ALBUM, -1,
			&SQL_INSERT_ALBUM_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_INSERT_GENRE, -1,
			&SQL_INSERT_GENRE_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_UPDATE_ARTIST, -1,
			&SQL_UPDATE_ARTIST_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_UPDATE_ALBUM, -1,
			&SQL_UPDATE_ALBUM_STMT, NULL) != SQLITE_OK
		|| sqlite3_prepare_v2(db, SQL_UPDATE_GENRE, -1,
			&SQL_UPDATE_GENRE_STMT, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(db));
		sqlite3_close(db);
		return NULL;
	}

	return db;
}

void db_close(sqlite3 *db)
{
	if (SQL_INSERT_SONG_STMT != NULL)
		sqlite3_finalize(SQL_INSERT_SONG_STMT);
	if (SQL_UPDATE_SONG_STMT != NULL)
		sqlite3_finalize(SQL_UPDATE_SONG_STMT);
	if (SQL_HAS_SONG_STMT != NULL)
		sqlite3_finalize(SQL_HAS_SONG_STMT);
	if (SQL_HAS_ARTIST_STMT != NULL)
		sqlite3_finalize(SQL_HAS_ARTIST_STMT);
	if (SQL_HAS_ALBUM_STMT != NULL)
		sqlite3_finalize(SQL_HAS_ALBUM_STMT);
	if (SQL_HAS_GENRE_STMT != NULL)
		sqlite3_finalize(SQL_HAS_GENRE_STMT);
	if (SQL_INSERT_ARTIST_STMT != NULL)
		sqlite3_finalize(SQL_INSERT_ARTIST_STMT);
	if (SQL_INSERT_ALBUM_STMT != NULL)
		sqlite3_finalize(SQL_INSERT_ALBUM_STMT);
	if (SQL_INSERT_GENRE_STMT != NULL)
		sqlite3_finalize(SQL_INSERT_GENRE_STMT);
	if (SQL_UPDATE_ARTIST_STMT != NULL)
		sqlite3_finalize(SQL_UPDATE_ARTIST_STMT);
	if (SQL_UPDATE_ALBUM_STMT != NULL)
		sqlite3_finalize(SQL_UPDATE_ALBUM_STMT);
	if (SQL_UPDATE_GENRE_STMT != NULL)
		sqlite3_finalize(SQL_UPDATE_GENRE_STMT);

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
		g_set_error(error, db_quark(), ACK_ERROR_NO_TAGS,
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
	else {
		if (!db_update_artist(db, song, id, increment, error))
			return false;
	}

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


/**
 * Love/Hate song/artist/album/genre
 */
bool db_love_artist_expr(sqlite3 *db, const char *expr, bool love,
		GSList **values, GError **error)
{
	char *stmt;

	g_assert(db != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!sql_update_artist(db, stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (values == NULL)
		return true;

	if (!sql_select_artist(db, "name, love", expr, cb_save, values, error))
		return false;
	return true;
}

bool db_love_album_expr(sqlite3 *db, const char *expr, bool love,
		GSList **values, GError **error)
{
	char *stmt;

	g_assert(db != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!sql_update_album(db, stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (values == NULL)
		return true;

	if (!sql_select_album(db, "name, love", expr, cb_save, values, error))
		return false;
	return true;
}

bool db_love_genre_expr(sqlite3 *db, const char *expr, bool love,
		GSList **values, GError **error)
{
	char *stmt;

	g_assert(db != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!sql_update_genre(db, stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (values == NULL)
		return true;

	if (!sql_select_genre(db, "name, love", expr, cb_save, values, error))
		return false;
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

	if (!sql_select_song(db, "uri, love", expr, cb_save, values, error))
		return false;
	return true;
}

/**
 * Kill/Unkill song/artist/album/genre
 */
bool db_kill_artist_expr(sqlite3 *db, const char *expr, bool kkill,
		GSList **values, GError **error)
{
	char *stmt;

	g_assert(db != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!sql_update_artist(db, stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (values == NULL)
		return true;

	if (!sql_select_artist(db, "name, kill", expr, cb_save, values, error))
		return false;
	return true;
}

bool db_kill_album_expr(sqlite3 *db, const char *expr, bool kkill,
		GSList **values, GError **error)
{
	char *stmt;

	g_assert(db != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!sql_update_album(db, stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (values == NULL)
		return true;

	if (!sql_select_album(db, "name, kill", expr, cb_save, values, error))
		return false;
	return true;
}

bool db_kill_genre_expr(sqlite3 *db, const char *expr, bool kkill,
		GSList **values, GError **error)
{
	char *stmt;

	g_assert(db != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!sql_update_genre(db, stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (values == NULL)
		return true;

	if (!sql_select_genre(db, "name, kill", expr, cb_save, values, error))
		return false;
	return true;
}

bool db_kill_song_expr(sqlite3 *db, const char *expr, bool kkill,
		GSList **values, GError **error)
{
	char *stmt;

	g_assert(db != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!sql_update_song(db, stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (values == NULL)
		return true;

	if (!sql_select_song(db, "uri, kill", expr, cb_save, values, error))
		return false;
	return true;
}

/**
 * Rate song/artist/album/genre
 */
bool db_rate_artist_expr(sqlite3 *db, const char *expr, int rating,
		GSList **values, GError **error)
{
	char *stmt;

	g_assert(db != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("rating = rating + (%d)", rating);
	if (!sql_update_artist(db, stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (values == NULL)
		return true;

	if (!sql_select_artist(db, "name, rating", expr, cb_save, values, error))
		return false;
	return true;
}

bool db_rate_album_expr(sqlite3 *db, const char *expr, int rating,
		GSList **values, GError **error)
{
	char *stmt;

	g_assert(db != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("rating = rating + (%d)", rating);
	if (!sql_update_album(db, stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (values == NULL)
		return true;

	if (!sql_select_album(db, "name, rating", expr, cb_save, values, error))
		return false;
	return true;
}

bool db_rate_genre_expr(sqlite3 *db, const char *expr, int rating,
		GSList **values, GError **error)
{
	char *stmt;

	g_assert(db != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("rating = rating + (%d)", rating);
	if (!sql_update_genre(db, stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (values == NULL)
		return true;

	if (!sql_select_genre(db, "name, rating", expr, cb_save, values, error))
		return false;
	return true;
}

bool db_rate_song_expr(sqlite3 *db, const char *expr, int rating,
		GSList **values, GError **error)
{
	char *stmt;

	g_assert(db != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("rating = rating + (%d)", rating);
	if (!sql_update_song(db, stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (values == NULL)
		return true;

	if (!sql_select_song(db, "uri, rating", expr, cb_save, values, error))
		return false;
	return true;
}
