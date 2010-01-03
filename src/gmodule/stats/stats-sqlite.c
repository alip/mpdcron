/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009-2010 Ali Polatel <alip@exherbo.org>
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
#include <string.h>

#include <glib.h>
#include <libdaemon/dlog.h>
#include <mpd/client.h>
#include <sqlite3.h>

static sqlite3 *gdb = NULL;

enum {
	SQL_SET_VERSION,
	SQL_GET_VERSION,
	SQL_SET_ENCODING,

	SQL_DB_CREATE_SONG,
	SQL_DB_CREATE_ARTIST,
	SQL_DB_CREATE_ALBUM,
	SQL_DB_CREATE_GENRE,
};

enum {
	SQL_BEGIN_TRANSACTION,
	SQL_END_TRANSACTION,

	SQL_PRAGMA_SYNC_ON,
	SQL_PRAGMA_SYNC_OFF,

	SQL_VACUUM,

	SQL_HAS_SONG,
	SQL_HAS_ARTIST,
	SQL_HAS_ALBUM,
	SQL_HAS_GENRE,

	SQL_INSERT_SONG,
	SQL_INSERT_ARTIST,
	SQL_INSERT_ALBUM,
	SQL_INSERT_GENRE,

	SQL_UPDATE_SONG,
	SQL_UPDATE_ARTIST,
	SQL_UPDATE_ALBUM,
	SQL_UPDATE_GENRE,
};

#define DB_VERSION	10
static const char * const db_sql_maint[] = {
	[SQL_SET_VERSION] = "PRAGMA user_version = 10;",
	[SQL_GET_VERSION] = "PRAGMA user_version;",

	[SQL_SET_ENCODING] = "PRAGMA encoding = \"UTF-8\";",

	[SQL_DB_CREATE_SONG] =
		"create table song(\n"
			"\tid              INTEGER PRIMARY KEY,\n"
			"\tplay_count      INTEGER,\n"
			"\tlove            INTEGER,\n"
			"\tkill            INTEGER,\n"
			"\trating          INTEGER,\n"
			"\ttags            TEXT NOT NULL,\n"
			"\turi             TEXT UNIQUE NOT NULL,\n"
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
			"\tmb_trackid      TEXT);\n",
	[SQL_DB_CREATE_ARTIST] =
		"create table artist(\n"
			"\tid              INTEGER PRIMARY KEY,\n"
			"\tplay_count      INTEGER,\n"
			"\ttags            TEXT NOT NULL,\n"
			"\tname            TEXT UNIQUE NOT NULL,\n"
			"\tlove            INTEGER,\n"
			"\tkill            INTEGER,\n"
			"\trating          INTEGER);\n",
	[SQL_DB_CREATE_ALBUM] =
		"create table album(\n"
			"\tid              INTEGER PRIMARY KEY,\n"
			"\tplay_count      INTEGER,\n"
			"\ttags            TEXT NOT NULL,\n"
			"\tartist          TEXT,\n"
			"\tname            TEXT UNIQUE NOT NULL,\n"
			"\tlove            INTEGER,\n"
			"\tkill            INTEGER,\n"
			"\trating          INTEGER);\n",
	[SQL_DB_CREATE_GENRE] =
		"create table genre(\n"
			"\tid              INTEGER PRIMARY KEY,\n"
			"\tplay_count      INTEGER,\n"
			"\ttags            TEXT NOT NULL,\n"
			"\tname            TEXT UNIQUE NOT NULL,\n"
			"\tlove            INTEGER,\n"
			"\tkill            INTEGER,\n"
			"\trating          INTEGER);",
};
static sqlite3_stmt *db_stmt_maint[G_N_ELEMENTS(db_sql_maint)] = { NULL };

static const char * const db_sql[] = {
	[SQL_BEGIN_TRANSACTION] = "BEGIN TRANSACTION;",
	[SQL_END_TRANSACTION] = "END TRANSACTION;",

	[SQL_PRAGMA_SYNC_ON] = "PRAGMA synchronous=ON;",
	[SQL_PRAGMA_SYNC_OFF] = "PRAGMA synchronous=OFF;",

	[SQL_VACUUM] = "VACUUM;",

	[SQL_HAS_SONG] = "select id from song where uri=?",
	[SQL_HAS_ARTIST] = "select id from artist where name=?",
	[SQL_HAS_ALBUM] = "select id from album where name=?",
	[SQL_HAS_GENRE] = "select id from genre where name=?",

	[SQL_INSERT_SONG] =
			"insert into song ("
				"play_count,"
				"love, kill, rating, tags,"
				"uri, duration, last_modified,"
				"artist, album, title,"
				"track, name, genre,"
				"date, composer, performer, disc,"
				"mb_artistid, mb_albumid, mb_trackid)"
				" values (?, 0, 0, 0, ':',"
					"?, ?, ?, ?,"
					"?, ?, ?, ?,"
					"?, ?, ?, ?,"
					"?, ?, ?, ?);",
	[SQL_INSERT_ARTIST] =
			"insert into artist ("
				"play_count, name,"
				"love, kill, rating, tags)"
				" values (?, ?, 0, 0, 0, ':');",
	[SQL_INSERT_ALBUM] =
			"insert into album ("
				"play_count, artist, name,"
				"love, kill, rating, tags)"
				" values (?, ?, ?, 0, 0, 0, ':');",
	[SQL_INSERT_GENRE] =
			"insert into genre ("
				"play_count, name,"
				"love, kill, rating, tags)"
				" values (?, ?, 0, 0, 0, ':');",

	[SQL_UPDATE_SONG] =
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
				" where id=?;",
	[SQL_UPDATE_ARTIST] =
			"update artist "
			"set play_count = play_count + ?,"
			"name=? where id=?;",
	[SQL_UPDATE_ALBUM] =
			"update album "
			"set play_count = play_count + ?,"
			"artist=?, name=? where id=?;",
	[SQL_UPDATE_GENRE] =
			"update genre "
			"set play_count = play_count + ?,"
			"name=? where id=?;",
};
static sqlite3_stmt *db_stmt[G_N_ELEMENTS(db_sql)] = { NULL };

/**
 * Utility Functions
 */
static GQuark
db_quark(void)
{
	return g_quark_from_static_string("database");
}

G_GNUC_UNUSED
static char *
escape_string(const char *src)
{
	char *q, *dest;

	q = sqlite3_mprintf("%Q", src);
	dest = g_strdup(q);
	sqlite3_free(q);
	return dest;
}

static int
db_step(sqlite3_stmt *stmt)
{
	int ret;

	do {
		ret = sqlite3_step(stmt);
	} while (ret == SQLITE_BUSY);

	return ret;
}

static bool
validate_tag(const char *tag, GError **error)
{
	if (tag[0] == '\0' || strchr(tag, ':') != NULL) {
		g_set_error(error, db_quark(),
				ACK_ERROR_INVALID_TAG,
				"Invalid tag `%s'", tag);
		return false;
	}
	return true;
}

static char *
remove_tag(const char *tags, const char *tag)
{
	int len;
	char **list;
	GString *new;

	len = strlen(tag) + 1;
	new = g_string_new(":");
	list = g_strsplit(tags, ":", -1);

	for (unsigned int i = 0; list[i] != NULL; i++) {
		if (list[i][0] == '\0')
			continue;
		else if (strncmp(list[i], tag, len) == 0)
			continue;
		g_string_append_printf(new, "%s:", list[i]);
	}
	g_strfreev(list);

	return g_string_free(new, FALSE);
}

void
db_generic_data_free(struct db_generic_data *data)
{
	g_strfreev(data->tags);
	g_free(data->name);
	g_free(data->artist);
	g_free(data);
}

void
db_song_data_free(struct db_song_data *song)
{
	g_strfreev(song->tags);
	g_free(song->uri);
	g_free(song->artist);
	g_free(song->album);
	g_free(song->title);
	g_free(song->track);
	g_free(song->name);
	g_free(song->genre);
	g_free(song->date);
	g_free(song->composer);
	g_free(song->performer);
	g_free(song->genre);
	g_free(song->mb_artist_id);
	g_free(song->mb_album_id);
	g_free(song->mb_track_id);
	g_free(song);
}

/**
 * Database Queries
 */
static int
db_has_artist(const char *name, GError **error)
{
	int id, ret;

	g_assert(gdb != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(db_stmt[SQL_HAS_ARTIST]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return -2;
	}

	/* Bind the argument */
	if (sqlite3_bind_text(db_stmt[SQL_HAS_ARTIST], 1, name,
			-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(gdb));
		return -2;
	}

	/* Step and get the id */
	id = -1;
	do {
		ret = sqlite3_step(db_stmt[SQL_HAS_ARTIST]);
		if (ret == SQLITE_ROW)
			id = sqlite3_column_int(db_stmt[SQL_HAS_ARTIST], 0);
	} while (ret == SQLITE_BUSY || ret == SQLITE_ROW);

	if (ret != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_SELECT,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return -2;
	}

	return id;
}

static int
db_has_album(const char *name, GError **error)
{
	int id, ret;

	g_assert(gdb != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(db_stmt[SQL_HAS_ALBUM]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return -2;
	}

	/* Bind the argument */
	if (sqlite3_bind_text(db_stmt[SQL_HAS_ALBUM], 1, name,
			-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(gdb));
		return -2;
	}

	/* Step and get the id */
	id = -1;
	do {
		ret = sqlite3_step(db_stmt[SQL_HAS_ALBUM]);
		if (ret == SQLITE_ROW)
			id = sqlite3_column_int(db_stmt[SQL_HAS_ALBUM], 0);
	} while (ret == SQLITE_BUSY || ret == SQLITE_ROW);

	if (ret != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_SELECT,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return -2;
	}

	return id;
}

static int
db_has_genre(const char *name, GError **error)
{
	int id, ret;

	g_assert(gdb != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(db_stmt[SQL_HAS_GENRE]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return -2;
	}

	/* Bind the argument */
	if (sqlite3_bind_text(db_stmt[SQL_HAS_GENRE], 1, name,
			-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(gdb));
		return -2;
	}

	/* Step and get the id */
	id = -1;
	do {
		ret = sqlite3_step(db_stmt[SQL_HAS_GENRE]);
		if (ret == SQLITE_ROW)
			id = sqlite3_column_int(db_stmt[SQL_HAS_GENRE], 0);
	} while (ret == SQLITE_BUSY || ret == SQLITE_ROW);

	if (ret != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_SELECT,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return -2;
	}

	return id;
}

static int
db_has_song(const char *uri, GError **error)
{
	int id, ret;

	g_assert(gdb != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(db_stmt[SQL_HAS_SONG]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return -2;
	}

	/* Bind the argument */
	if (sqlite3_bind_text(db_stmt[SQL_HAS_SONG], 1, uri,
			-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(gdb));
		return -2;
	}

	/* Step and get the id */
	id = -1;
	do {
		ret = sqlite3_step(db_stmt[SQL_HAS_SONG]);
		if (ret == SQLITE_ROW)
			id = sqlite3_column_int(db_stmt[SQL_HAS_SONG], 0);
	} while (ret == SQLITE_BUSY || ret == SQLITE_ROW);

	if (ret != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_SELECT,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return -2;
	}

	return id;
}

/**
 * Database Inserts/Updates
 */
static bool
db_insert_artist(const struct mpd_song *song, bool increment,
		GError **error)
{
	g_assert(gdb != NULL);
	g_assert(song != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(db_stmt[SQL_INSERT_ARTIST]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (sqlite3_bind_int(db_stmt[SQL_INSERT_ARTIST],
				1, increment ? 1 : 0) != SQLITE_OK
			|| sqlite3_bind_text(db_stmt[SQL_INSERT_ARTIST],
				2, mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (db_step(db_stmt[SQL_INSERT_ARTIST]) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	return true;
}

static bool
db_insert_album(const struct mpd_song *song, bool increment,
		GError **error)
{
	g_assert(gdb != NULL);
	g_assert(song != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(db_stmt[SQL_INSERT_ALBUM]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (sqlite3_bind_int(db_stmt[SQL_INSERT_ALBUM],
				1, increment ? 1 : 0) != SQLITE_OK
			|| sqlite3_bind_text(db_stmt[SQL_INSERT_ALBUM],
				2, mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				-1, SQLITE_STATIC) != SQLITE_OK
			|| sqlite3_bind_text(db_stmt[SQL_INSERT_ALBUM],
				3, mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (db_step(db_stmt[SQL_INSERT_ALBUM]) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	return true;
}

static bool
db_insert_genre(const struct mpd_song *song, bool increment,
		GError **error)
{
	g_assert(gdb != NULL);
	g_assert(song != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(db_stmt[SQL_INSERT_GENRE]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (sqlite3_bind_int(db_stmt[SQL_INSERT_GENRE],
				1, increment ? 1 : 0) != SQLITE_OK
			|| sqlite3_bind_text(db_stmt[SQL_INSERT_GENRE],
				2, mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
				-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (db_step(db_stmt[SQL_INSERT_GENRE]) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	return true;
}

static bool
db_insert_song(const struct mpd_song *song, bool increment,
		GError **error)
{
	g_assert(gdb != NULL);
	g_assert(song != NULL);

	if (sqlite3_reset(db_stmt[SQL_INSERT_SONG]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (sqlite3_bind_int(db_stmt[SQL_INSERT_SONG], 1, increment ? 1 : 0) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 2,
			mpd_song_get_uri(song),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_int(db_stmt[SQL_INSERT_SONG], 3,
			mpd_song_get_duration(song)) != SQLITE_OK
		|| sqlite3_bind_int(db_stmt[SQL_INSERT_SONG], 4,
			mpd_song_get_last_modified(song)) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 5,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 6,
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 7,
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 8,
			mpd_song_get_tag(song, MPD_TAG_TRACK, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 9,
			mpd_song_get_tag(song, MPD_TAG_NAME, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 10,
			mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 11,
			mpd_song_get_tag(song, MPD_TAG_DATE, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 12,
			mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 13,
			mpd_song_get_tag(song, MPD_TAG_PERFORMER, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 14,
			mpd_song_get_tag(song, MPD_TAG_DISC, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 15,
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ARTISTID, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 16,
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMID, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_INSERT_SONG], 17,
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_TRACKID, 0),
			-1, SQLITE_STATIC) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (db_step(db_stmt[SQL_INSERT_SONG]) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	return true;
}

static bool
db_update_artist(const struct mpd_song *song, int id,
		bool increment, GError **error)
{
	g_assert(gdb != NULL);
	g_assert(song != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(db_stmt[SQL_UPDATE_ARTIST]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (sqlite3_bind_int(db_stmt[SQL_UPDATE_ARTIST],
				1, increment ? 1 : 0) != SQLITE_OK
			|| sqlite3_bind_text(db_stmt[SQL_UPDATE_ARTIST],
				2, mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				-1, SQLITE_STATIC) != SQLITE_OK
			|| sqlite3_bind_int(db_stmt[SQL_UPDATE_ARTIST],
				3, id) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (db_step(db_stmt[SQL_UPDATE_ARTIST]) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	return true;
}

static bool
db_update_album(const struct mpd_song *song, int id,
		bool increment, GError **error)
{
	g_assert(gdb != NULL);
	g_assert(song != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(db_stmt[SQL_UPDATE_ALBUM]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (sqlite3_bind_int(db_stmt[SQL_UPDATE_ALBUM],
				1, increment ? 1 : 0) != SQLITE_OK
			|| sqlite3_bind_text(db_stmt[SQL_UPDATE_ALBUM],
				2, mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
				-1, SQLITE_STATIC) != SQLITE_OK
			|| sqlite3_bind_text(db_stmt[SQL_UPDATE_ALBUM],
				3, mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				-1, SQLITE_STATIC) != SQLITE_OK
			|| sqlite3_bind_int(db_stmt[SQL_UPDATE_ALBUM],
				4, id) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (db_step(db_stmt[SQL_UPDATE_ALBUM]) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	return true;
}

static bool
db_update_genre(const struct mpd_song *song, int id,
		bool increment, GError **error)
{
	g_assert(gdb != NULL);
	g_assert(song != NULL);

	/* Reset the statement to its initial state */
	if (sqlite3_reset(db_stmt[SQL_UPDATE_GENRE]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (sqlite3_bind_int(db_stmt[SQL_UPDATE_GENRE],
				1, increment ? 1 : 0) != SQLITE_OK
			|| sqlite3_bind_text(db_stmt[SQL_UPDATE_GENRE],
				2, mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
				-1, SQLITE_STATIC) != SQLITE_OK
			|| sqlite3_bind_int(db_stmt[SQL_UPDATE_GENRE],
				3, id) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (db_step(db_stmt[SQL_UPDATE_GENRE]) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	return true;
}

static bool
db_update_song(const struct mpd_song *song, int id,
		bool increment, GError **error)
{
	int ret;

	g_assert(gdb != NULL);
	g_assert(song != NULL);

	if (sqlite3_reset(db_stmt[SQL_UPDATE_SONG]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (sqlite3_bind_int(db_stmt[SQL_UPDATE_SONG], 1, increment ? 1 : 0) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 2,
			mpd_song_get_uri(song),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_int(db_stmt[SQL_UPDATE_SONG], 3,
			mpd_song_get_duration(song)) != SQLITE_OK
		|| sqlite3_bind_int(db_stmt[SQL_UPDATE_SONG], 4,
			mpd_song_get_last_modified(song)) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 5,
			mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 6,
			mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 7,
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 8,
			mpd_song_get_tag(song, MPD_TAG_TRACK, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 9,
			mpd_song_get_tag(song, MPD_TAG_NAME, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 10,
			mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 11,
			mpd_song_get_tag(song, MPD_TAG_DATE, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 12,
			mpd_song_get_tag(song, MPD_TAG_COMPOSER, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 13,
			mpd_song_get_tag(song, MPD_TAG_PERFORMER, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 14,
			mpd_song_get_tag(song, MPD_TAG_DISC, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 15,
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ARTISTID, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 16,
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_ALBUMID, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_text(db_stmt[SQL_UPDATE_SONG], 17,
			mpd_song_get_tag(song, MPD_TAG_MUSICBRAINZ_TRACKID, 0),
			-1, SQLITE_STATIC) != SQLITE_OK
		|| sqlite3_bind_int(db_stmt[SQL_UPDATE_SONG], 18, id)) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_BIND,
				"sqlite3_bind: %s", sqlite3_errmsg(gdb));
		return false;
	}

	do {
		ret = sqlite3_step(db_stmt[SQL_UPDATE_SONG]);
	} while (ret == SQLITE_BUSY);

	if (ret != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}
	return true;
}

/**
 * Database Updates
 */
static bool
sql_update_entry(const char *tbl, const char *stmt,
		const char *expr, GError **error)
{
	char *sql;
	sqlite3_stmt *sql_stmt;

	g_assert(gdb != NULL);
	g_assert(tbl != NULL);
	g_assert(stmt != NULL);
	g_assert(expr != NULL);

	sql = g_strdup_printf("update %s set %s where %s ;", tbl, stmt, expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &sql_stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	if (db_step(sql_stmt) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		sqlite3_finalize(sql_stmt);
		return false;
	}
	sqlite3_finalize(sql_stmt);
	return true;
}

static inline bool
sql_update_artist(const char *stmt, const char *expr,
		GError **error)
{
	return sql_update_entry("artist", stmt, expr, error);
}

static inline bool
sql_update_album(const char *stmt, const char *expr,
		GError **error)
{
	return sql_update_entry("album", stmt, expr, error);
}

static inline bool
sql_update_genre(const char *stmt, const char *expr,
		GError **error)
{
	return sql_update_entry("genre", stmt, expr, error);
}

static inline bool
sql_update_song(const char *stmt, const char *expr,
		GError **error)
{
	return sql_update_entry("song", stmt, expr, error);
}

/**
 * Database Maintenance
 */
static bool
db_create(GError **error)
{
	g_assert(gdb != NULL);
	g_assert(db_stmt_maint[SQL_DB_CREATE_SONG] != NULL);
	g_assert(db_stmt_maint[SQL_DB_CREATE_ARTIST] != NULL);
	g_assert(db_stmt_maint[SQL_DB_CREATE_ALBUM] != NULL);
	g_assert(db_stmt_maint[SQL_DB_CREATE_GENRE] != NULL);
	g_assert(db_stmt_maint[SQL_SET_ENCODING] != NULL);
	g_assert(db_stmt_maint[SQL_SET_VERSION] != NULL);

	/**
	 * Create tables
	 */
	if (db_step(db_stmt_maint[SQL_DB_CREATE_SONG]) != SQLITE_DONE
		|| db_step(db_stmt_maint[SQL_DB_CREATE_ARTIST]) != SQLITE_DONE
		|| db_step(db_stmt_maint[SQL_DB_CREATE_ALBUM]) != SQLITE_DONE
		|| db_step(db_stmt_maint[SQL_DB_CREATE_GENRE]) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_CREATE,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	/**
	 * Set encoding
	 */
	if (db_step(db_stmt_maint[SQL_SET_ENCODING]) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_CREATE,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	/**
	 * Set database version
	 */
	if (db_step(db_stmt_maint[SQL_SET_VERSION]) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_CREATE,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	return true;
}

static bool
db_check_ver(GError **error)
{
	int ret, version;

	g_assert(gdb != NULL);
	g_assert(db_stmt_maint[SQL_GET_VERSION] != NULL);

	/**
	 * Check version
	 */
	do {
		ret = sqlite3_step(db_stmt_maint[SQL_GET_VERSION]);
		if (ret == SQLITE_ROW)
			version = sqlite3_column_int(db_stmt_maint[SQL_GET_VERSION], 0);
	} while (ret == SQLITE_BUSY || ret == SQLITE_ROW);

	if (ret != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_CREATE,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
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

bool
db_initialized(void)
{
	return (gdb != NULL);
}

bool
db_init(const char *path, bool create, bool readonly,
		GError **error)
{
	int flags;
	gboolean new;

	g_assert(gdb == NULL);

	new = !g_file_test(path, G_FILE_TEST_EXISTS);

	flags = 0;
	if (create && readonly) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_OPEN,
				"Invalid flags");
		return false;
	}
	flags |= (readonly ? SQLITE_OPEN_READONLY : SQLITE_OPEN_READWRITE);
	if (create)
		flags |= SQLITE_OPEN_CREATE;

	if (sqlite3_open_v2(path, &gdb, flags, NULL) != 0) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_OPEN,
				"sqlite3_open_v2: %s", sqlite3_errmsg(gdb));
		gdb = NULL;
		return false;
	}

	if (new) {
		for (unsigned int i = 0; i < G_N_ELEMENTS(db_sql_maint); i++) {
			if (sqlite3_prepare_v2(gdb, db_sql_maint[i], -1,
					&db_stmt_maint[i], NULL) != SQLITE_OK) {
				g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
						"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
				db_close();
				return false;
			}
		}

		if (!db_create(error)) {
			db_close();
			return false;
		}
	}
	else {
		if (sqlite3_prepare_v2(gdb, db_sql_maint[SQL_GET_VERSION], -1,
					&db_stmt_maint[SQL_GET_VERSION],
					NULL) != SQLITE_OK) {
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
					"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
			db_close();
			return false;
		}
		if (!db_check_ver(error)) {
			db_close();
			return false;
		}
		sqlite3_finalize(db_stmt_maint[SQL_GET_VERSION]);
		db_stmt_maint[SQL_GET_VERSION] = NULL;
	}

	/* Prepare common statements */
	for (unsigned int i = 0; i < G_N_ELEMENTS(db_sql); i++) {
		g_assert(db_stmt[i] == NULL);
		if (sqlite3_prepare_v2(gdb, db_sql[i], -1,
				&db_stmt[i], NULL) != SQLITE_OK) {
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
					"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
			db_close();
			return false;
		}
	}

	return true;
}

void
db_close(void)
{
	for (unsigned int i = 0; i < G_N_ELEMENTS(db_sql_maint); i++) {
		if (db_stmt_maint[i] != NULL) {
			sqlite3_finalize(db_stmt_maint[i]);
			db_stmt_maint[i] = NULL;
		}
	}
	for (unsigned int i = 0; i < G_N_ELEMENTS(db_sql); i++) {
		if (db_stmt[i] != NULL) {
			sqlite3_finalize(db_stmt[i]);
			db_stmt[i] = NULL;
		}
	}
	sqlite3_close(gdb);
	gdb = NULL;
}

bool
db_set_authorizer(int (*xAuth)(void *, int, const char *, const char *,
			const char *,const char *),
		void *userdata, GError **error)
{
	g_assert(gdb != NULL);

	if (sqlite3_set_authorizer(gdb, xAuth, userdata) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_AUTH,
				"sqlite3_set_authorizer: %s",
				sqlite3_errmsg(gdb));
		return false;
	}
	return true;
}

/**
 * Database Interaction
 */
bool
db_start_transaction(GError **error)
{
	g_assert(gdb != NULL);

	if (sqlite3_reset(db_stmt[SQL_BEGIN_TRANSACTION]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (db_step(db_stmt[SQL_BEGIN_TRANSACTION]) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	return true;
}

bool
db_end_transaction(GError **error)
{
	g_assert(gdb != NULL);

	if (sqlite3_reset(db_stmt[SQL_END_TRANSACTION]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (db_step(db_stmt[SQL_END_TRANSACTION]) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	return true;
}

bool
db_set_sync(bool on, GError **error)
{
	sqlite3_stmt *stmt;

	g_assert(gdb != NULL);

	stmt = on ? db_stmt[SQL_PRAGMA_SYNC_ON] : db_stmt[SQL_PRAGMA_SYNC_OFF];

	if (sqlite3_reset(stmt) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (db_step(stmt) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	return true;
}

bool
db_vacuum(GError **error)
{
	g_assert(gdb != NULL);

	if (sqlite3_reset(db_stmt[SQL_VACUUM]) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_RESET,
				"sqlite3_reset: %s", sqlite3_errmsg(gdb));
		return false;
	}

	if (db_step(db_stmt[SQL_VACUUM]) != SQLITE_DONE) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
				"sqlite3_step: %s", sqlite3_errmsg(gdb));
		return false;
	}

	return true;
}

bool
db_process(const struct mpd_song *song, bool increment,
		GError **error)
{
	int id;

	g_assert(gdb != NULL);
	g_assert(song != NULL);

	if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL ||
			mpd_song_get_tag(song, MPD_TAG_TITLE, 0) == NULL) {
		g_set_error(error, db_quark(), ACK_ERROR_NO_TAGS,
				"Song (%s) doesn't have required tags",
				mpd_song_get_uri(song));
		return true;
	}

	if ((id = db_has_song(mpd_song_get_uri(song), error)) < -1)
		return false;
	else if (id == -1) {
		if (!db_insert_song(song, increment, error))
			return false;
	}
	else {
		if (!db_update_song(song, id, increment, error))
			return false;
	}

	if ((id = db_has_artist(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0), error)) < -1)
		return false;
	else if (id == -1) {
		if (!db_insert_artist(song, increment, error))
			return false;
	}
	else {
		if (!db_update_artist(song, id, increment, error))
			return false;
	}

	if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) != NULL) {
		if ((id = db_has_album(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0),
				error)) < -1)
			return false;
		else if (id == -1) {
			if (!db_insert_album(song, increment, error))
				return false;
		}
		else {
			if (!db_update_album(song, id, increment, error))
				return false;
		}
	}

	if (mpd_song_get_tag(song, MPD_TAG_GENRE, 0) != NULL) {
		if ((id = db_has_genre(mpd_song_get_tag(song, MPD_TAG_GENRE, 0),
				error)) < -1)
			return false;
		else if (id == -1) {
			if (!db_insert_genre(song, increment, error))
				return false;
		}
		else {
			if (!db_update_genre(song, id, increment, error))
				return false;
		}
	}

	return true;
}

/**
 * Main Interface
 */

/**
 * List song/artist/album/genre
 */
bool
db_list_artist_expr(const char *expr, GSList **values,
		GError **error)
{
	int ret;
	char *sql;
	sqlite3_stmt *stmt;
	struct db_generic_data *data;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	sql = g_strdup_printf("select id, name from artist where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	do {
		ret = sqlite3_step(stmt);
		switch (ret) {
		case SQLITE_ROW:
			data = g_new0(struct db_generic_data, 1);
			data->id = sqlite3_column_int(stmt, 0);
			data->name = g_strdup((const char *)sqlite3_column_text(stmt, 1));
			*values = g_slist_prepend(*values, data);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(stmt);
	return true;
}

bool
db_list_album_expr(const char *expr, GSList **values,
		GError **error)
{
	int ret;
	char *sql;
	struct db_generic_data *data;
	sqlite3_stmt *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	sql = g_strdup_printf("select id, name, artist from album where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	do {
		ret = sqlite3_step(stmt);
		switch (ret) {
		case SQLITE_ROW:
			data = g_new0(struct db_generic_data, 1);
			data->id = sqlite3_column_int(stmt, 0);
			data->name = g_strdup((const char *)sqlite3_column_text(stmt, 1));
			data->artist = g_strdup((const char *)sqlite3_column_text(stmt, 2));
			*values = g_slist_prepend(*values, data);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(stmt);
	return true;
}

bool
db_list_genre_expr(const char *expr, GSList **values,
		GError **error)
{
	int ret;
	char *sql;
	struct db_generic_data *data;
	sqlite3_stmt *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	sql = g_strdup_printf("select id, name from genre where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	do {
		ret = sqlite3_step(stmt);
		switch (ret) {
		case SQLITE_ROW:
			data = g_new0(struct db_generic_data, 1);
			data->id = sqlite3_column_int(stmt, 0);
			data->name = g_strdup((const char *)sqlite3_column_text(stmt, 1));
			*values = g_slist_prepend(*values, data);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(stmt);
	return true;
}

bool
db_list_song_expr(const char *expr, GSList **values,
		GError **error)
{
	int ret;
	char *sql;
	struct db_song_data *song;
	sqlite3_stmt *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	sql = g_strdup_printf("select id, uri from song where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	do {
		ret = sqlite3_step(stmt);
		switch (ret) {
		case SQLITE_ROW:
			song = g_new0(struct db_song_data, 1);
			song->id = sqlite3_column_int(stmt, 0);
			song->uri = g_strdup((const char *)sqlite3_column_text(stmt, 1));
			*values = g_slist_prepend(*values, song);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(stmt);
	return true;
}

bool
db_listinfo_artist_expr(const char *expr, GSList **values,
		GError **error)
{
	int ret;
	char *sql;
	sqlite3_stmt *stmt;
	struct db_generic_data *data;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	sql = g_strdup_printf("select "
			"id, play_count, name, love, kill, rating "
			"from artist where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	do {
		ret = sqlite3_step(stmt);
		switch (ret) {
		case SQLITE_ROW:
			data = g_new0(struct db_generic_data, 1);
			data->id = sqlite3_column_int(stmt, 0);
			data->play_count = sqlite3_column_int(stmt, 1);
			data->name = g_strdup((const char *)sqlite3_column_text(stmt, 2));
			data->love = sqlite3_column_int(stmt, 3);
			data->kill = sqlite3_column_int(stmt, 4);
			data->rating = sqlite3_column_int(stmt, 5);
			*values = g_slist_prepend(*values, data);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(stmt);
	return true;
}

bool
db_listinfo_album_expr(const char *expr, GSList **values,
		GError **error)
{
	int ret;
	char *sql;
	sqlite3_stmt *stmt;
	struct db_generic_data *data;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	sql = g_strdup_printf("select "
			"id, play_count, name, artist, love, kill, rating "
			"from album where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	do {
		ret = sqlite3_step(stmt);
		switch (ret) {
		case SQLITE_ROW:
			data = g_new0(struct db_generic_data, 1);
			data->id = sqlite3_column_int(stmt, 0);
			data->play_count = sqlite3_column_int(stmt, 1);
			data->name = g_strdup((const char *)sqlite3_column_text(stmt, 2));
			data->artist = g_strdup((const char *)sqlite3_column_text(stmt, 3));
			data->love = sqlite3_column_int(stmt, 4);
			data->kill = sqlite3_column_int(stmt, 5);
			data->rating = sqlite3_column_int(stmt, 6);
			*values = g_slist_prepend(*values, data);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(stmt);
	return true;
}

bool
db_listinfo_genre_expr(const char *expr, GSList **values,
		GError **error)
{
	int ret;
	char *sql;
	sqlite3_stmt *stmt;
	struct db_generic_data *data;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	sql = g_strdup_printf("select "
			"id, play_count, name, love, kill, rating "
			"from genre where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	do {
		ret = sqlite3_step(stmt);
		switch (ret) {
		case SQLITE_ROW:
			data = g_new0(struct db_generic_data, 1);
			data->id = sqlite3_column_int(stmt, 0);
			data->play_count = sqlite3_column_int(stmt, 1);
			data->name = g_strdup((const char *)sqlite3_column_text(stmt, 2));
			data->love = sqlite3_column_int(stmt, 3);
			data->kill = sqlite3_column_int(stmt, 4);
			data->rating = sqlite3_column_int(stmt, 5);
			*values = g_slist_prepend(*values, data);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(stmt);
	return true;
}

bool
db_listinfo_song_expr(const char *expr, GSList **values,
		GError **error)
{
	int ret;
	char *sql;
	struct db_song_data *song;
	sqlite3_stmt *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	sql = g_strdup_printf("select "
			"id, play_count, love, kill, rating, uri "
			"from song where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	do {
		ret = sqlite3_step(stmt);
		switch (ret) {
		case SQLITE_ROW:
			song = g_new0(struct db_song_data, 1);
			song->id = sqlite3_column_int(stmt, 0);
			song->play_count = sqlite3_column_int(stmt, 1);
			song->love = sqlite3_column_int(stmt, 2);
			song->kill = sqlite3_column_int(stmt, 3);
			song->rating = sqlite3_column_int(stmt, 4);
			song->uri = g_strdup((const char *)sqlite3_column_text(stmt, 5));
			*values = g_slist_prepend(*values, song);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(stmt);
	return true;
}

/**
 * Increase/Decrease play count of song/artist/album/genre
 */
bool
db_count_artist_expr(const char *expr, int count, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("play_count = play_count + (%d)", count);
	if (!sql_update_artist(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_count_album_expr(const char *expr, int count, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("play_count = play_count + (%d)", count);
	if (!sql_update_album(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_count_genre_expr(const char *expr, int count, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("play_count = play_count + (%d)", count);
	if (!sql_update_genre(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_count_song_expr(const char *expr, int count, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("play_count = play_count + (%d)", count);
	if (!sql_update_song(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

/**
 * Love/Hate song/artist/album/genre
 */
bool
db_love_artist_expr(const char *expr, bool love, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!sql_update_artist(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_love_album_expr(const char *expr, bool love, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!sql_update_album(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_love_genre_expr(const char *expr, bool love, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!sql_update_genre(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_love_song_expr(const char *expr, bool love, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("love = love %s 1", love ? "+" : "-");
	if (!sql_update_song(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

/**
 * Kill/Unkill song/artist/album/genre
 */
bool
db_kill_artist_expr(const char *expr, bool kkill, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!sql_update_artist(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_kill_album_expr(const char *expr, bool kkill, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!sql_update_album(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_kill_genre_expr(const char *expr, bool kkill, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!sql_update_genre(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_kill_song_expr(const char *expr, bool kkill, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("kill = %s", kkill ? "kill + 1" : "0");
	if (!sql_update_song(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

/**
 * Rate song/artist/album/genre
 */
bool
db_rate_artist_expr(const char *expr, int rating, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("rating = rating + (%d)", rating);
	if (!sql_update_artist(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_rate_album_expr(const char *expr, int rating, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("rating = rating + (%d)", rating);
	if (!sql_update_album(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_rate_genre_expr(const char *expr, int rating, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("rating = rating + (%d)", rating);
	if (!sql_update_genre(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_rate_song_expr(const char *expr, int rating, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	stmt = g_strdup_printf("rating = rating + (%d)", rating);
	if (!sql_update_song(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

/**
 * Tags management
 */
bool
db_add_artist_tag_expr(const char *expr, const char *tag, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	if (!validate_tag(tag, error))
		return false;

	stmt = g_strdup_printf("tags = tags || '%s:'", tag);
	if (!sql_update_artist(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_add_album_tag_expr(const char *expr, const char *tag, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	if (!validate_tag(tag, error))
		return false;

	stmt = g_strdup_printf("tags = tags || '%s:'", tag);
	if (!sql_update_album(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_add_genre_tag_expr(const char *expr, const char *tag, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	if (!validate_tag(tag, error))
		return false;

	stmt = g_strdup_printf("tags = tags || '%s:'", tag);
	if (!sql_update_genre(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_add_song_tag_expr(const char *expr, const char *tag, int *changes, GError **error)
{
	char *stmt;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	if (!validate_tag(tag, error))
		return false;

	stmt = g_strdup_printf("tags = tags || '%s:'", tag);
	if (!sql_update_song(stmt, expr, error)) {
		g_free(stmt);
		return false;
	}
	g_free(stmt);

	if (changes != NULL)
		*changes = sqlite3_changes(gdb);

	return true;
}

bool
db_remove_artist_tag_expr(const char *expr, const char *tag, int *changes, GError **error)
{
	int ret;
	char *stmt, *sql;
	GSList *ids, *walk;
	sqlite3_stmt *sql_stmt;
	struct map {
		int id;
		char *tags;
	} *map;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	if (!validate_tag(tag, error))
		return false;

	sql = g_strdup_printf("select id, tags from artist where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &sql_stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	ids = NULL;
	do {
		ret = sqlite3_step(sql_stmt);
		switch (ret) {
		case SQLITE_ROW:
			map = g_new(struct map, 1);
			map->id = sqlite3_column_int(sql_stmt, 0);
			map->tags = remove_tag((const char *)sqlite3_column_text(sql_stmt, 1), tag);
			ids = g_slist_prepend(ids, map);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(sql_stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(sql_stmt);

	if (changes != NULL)
		*changes = 0;

	db_set_sync(false, NULL);
	db_start_transaction(NULL);
	ret = true;
	for (walk = ids; walk != NULL; walk = g_slist_next(walk)) {
		map = (struct map *) walk->data;
		if (ret) {
			char *esc_tags = escape_string(map->tags);
			stmt = g_strdup_printf("tags = %s", esc_tags);
			g_free(esc_tags);

			sql = g_strdup_printf("id = %d", map->id);
			ret = sql_update_artist(stmt, sql, error);
			g_free(sql);

			if (changes != NULL)
				*changes += sqlite3_changes(gdb);
		}
		g_free(map->tags);
		g_free(map);
	}
	g_slist_free(ids);
	db_end_transaction(NULL);
	db_set_sync(true, NULL);

	return ret;
}

bool
db_remove_album_tag_expr(const char *expr, const char *tag, int *changes, GError **error)
{
	int ret;
	char *stmt, *sql;
	GSList *ids, *walk;
	sqlite3_stmt *sql_stmt;
	struct map {
		int id;
		char *tags;
	} *map;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	if (!validate_tag(tag, error))
		return false;

	sql = g_strdup_printf("select id, tags from album where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &sql_stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	ids = NULL;
	do {
		ret = sqlite3_step(sql_stmt);
		switch (ret) {
		case SQLITE_ROW:
			map = g_new(struct map, 1);
			map->id = sqlite3_column_int(sql_stmt, 0);
			map->tags = remove_tag((const char *)sqlite3_column_text(sql_stmt, 1), tag);
			ids = g_slist_prepend(ids, map);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(sql_stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(sql_stmt);

	if (changes != NULL)
		*changes = 0;

	db_set_sync(false, NULL);
	db_start_transaction(NULL);
	ret = true;
	for (walk = ids; walk != NULL; walk = g_slist_next(walk)) {
		map = (struct map *) walk->data;
		if (ret) {
			char *esc_tags = escape_string(map->tags);
			stmt = g_strdup_printf("tags = %s'", esc_tags);
			g_free(esc_tags);

			sql = g_strdup_printf("id = %d", map->id);
			ret = sql_update_album(stmt, sql, error);
			g_free(sql);

			if (changes != NULL)
				*changes += sqlite3_changes(gdb);
		}
		g_free(map->tags);
		g_free(map);
	}
	g_slist_free(ids);
	db_end_transaction(NULL);
	db_set_sync(true, NULL);

	return ret;
}

bool
db_remove_genre_tag_expr(const char *expr, const char *tag, int *changes, GError **error)
{
	int ret;
	char *stmt, *sql;
	GSList *ids, *walk;
	sqlite3_stmt *sql_stmt;
	struct map {
		int id;
		char *tags;
	} *map;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	if (!validate_tag(tag, error))
		return false;

	sql = g_strdup_printf("select id, tags from genre where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &sql_stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	ids = NULL;
	do {
		ret = sqlite3_step(sql_stmt);
		switch (ret) {
		case SQLITE_ROW:
			map = g_new(struct map, 1);
			map->id = sqlite3_column_int(sql_stmt, 0);
			map->tags = remove_tag((const char *)sqlite3_column_text(sql_stmt, 1), tag);
			ids = g_slist_prepend(ids, map);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(sql_stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(sql_stmt);

	if (changes != NULL)
		*changes = 0;

	db_set_sync(false, NULL);
	db_start_transaction(NULL);
	ret = true;
	for (walk = ids; walk != NULL; walk = g_slist_next(walk)) {
		map = (struct map *) walk->data;
		if (ret) {
			char *esc_tags = escape_string(map->tags);
			stmt = g_strdup_printf("tags = %s", esc_tags);
			g_free(esc_tags);

			sql = g_strdup_printf("id = %d", map->id);
			ret = sql_update_genre(stmt, sql, error);
			g_free(sql);

			if (changes != NULL)
				*changes += sqlite3_changes(gdb);
		}
		g_free(map->tags);
		g_free(map);
	}
	g_slist_free(ids);
	db_end_transaction(NULL);
	db_set_sync(true, NULL);

	return ret;
}

bool
db_remove_song_tag_expr(const char *expr, const char *tag, int *changes, GError **error)
{
	int ret;
	char *stmt, *sql;
	GSList *ids, *walk;
	sqlite3_stmt *sql_stmt;
	struct map {
		int id;
		char *tags;
	} *map;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);

	if (!validate_tag(tag, error))
		return false;

	sql = g_strdup_printf("select id, tags from song where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &sql_stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	ids = NULL;
	do {
		ret = sqlite3_step(sql_stmt);
		switch (ret) {
		case SQLITE_ROW:
			map = g_new(struct map, 1);
			map->id = sqlite3_column_int(sql_stmt, 0);
			map->tags = remove_tag((const char *)sqlite3_column_text(sql_stmt, 1), tag);
			ids = g_slist_prepend(ids, map);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(sql_stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(sql_stmt);

	if (changes != NULL)
		*changes = 0;

	db_set_sync(false, NULL);
	db_start_transaction(NULL);
	ret = true;
	for (walk = ids; walk != NULL; walk = g_slist_next(walk)) {
		map = (struct map *) walk->data;
		if (ret) {
			char *esc_tags = escape_string(map->tags);
			stmt = g_strdup_printf("tags = %s", esc_tags);
			g_free(esc_tags);

			sql = g_strdup_printf("id = %d", map->id);
			ret = sql_update_song(stmt, sql, error);
			g_free(sql);

			if (changes != NULL)
				*changes += sqlite3_changes(gdb);
		}
		g_free(map->tags);
		g_free(map);
	}
	g_slist_free(ids);
	db_end_transaction(NULL);
	db_set_sync(true, NULL);

	return ret;
}

bool
db_list_artist_tag_expr(const char *expr, GSList **values, GError **error)
{
	int ret;
	char *sql;
	sqlite3_stmt *stmt;
	struct db_generic_data *data;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	sql = g_strdup_printf("select id, name, tags from artist where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	do {
		ret = sqlite3_step(stmt);
		switch (ret) {
		case SQLITE_ROW:
			data = g_new0(struct db_generic_data, 1);
			data->id = sqlite3_column_int(stmt, 0);
			data->name = g_strdup((const char *)sqlite3_column_text(stmt, 1));
			data->tags = g_strsplit((const char *)sqlite3_column_text(stmt, 2), ":", -1);
			*values = g_slist_prepend(*values, data);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(stmt);
	return true;
}

bool
db_list_album_tag_expr(const char *expr, GSList **values, GError **error)
{
	int ret;
	char *sql;
	sqlite3_stmt *stmt;
	struct db_generic_data *data;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	sql = g_strdup_printf("select id, name, artist, tags from album where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	do {
		ret = sqlite3_step(stmt);
		switch (ret) {
		case SQLITE_ROW:
			data = g_new0(struct db_generic_data, 1);
			data->id = sqlite3_column_int(stmt, 0);
			data->name = g_strdup((const char *)sqlite3_column_text(stmt, 1));
			data->artist = g_strdup((const char *)sqlite3_column_text(stmt, 2));
			data->tags = g_strsplit((const char *)sqlite3_column_text(stmt, 3), ":", -1);
			*values = g_slist_prepend(*values, data);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(stmt);
	return true;
}

bool
db_list_genre_tag_expr(const char *expr, GSList **values, GError **error)
{
	int ret;
	char *sql;
	sqlite3_stmt *stmt;
	struct db_generic_data *data;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	sql = g_strdup_printf("select id, name, tags from genre where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	do {
		ret = sqlite3_step(stmt);
		switch (ret) {
		case SQLITE_ROW:
			data = g_new0(struct db_generic_data, 1);
			data->id = sqlite3_column_int(stmt, 0);
			data->name = g_strdup((const char *)sqlite3_column_text(stmt, 1));
			data->tags = g_strsplit((const char *)sqlite3_column_text(stmt, 2), ":", -1);
			*values = g_slist_prepend(*values, data);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(stmt);
	return true;
}

bool
db_list_song_tag_expr(const char *expr, GSList **values, GError **error)
{
	int ret;
	char *sql;
	sqlite3_stmt *stmt;
	struct db_song_data *song;

	g_assert(gdb != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	sql = g_strdup_printf("select id, uri, tags from song where %s ;", expr);
	if (sqlite3_prepare_v2(gdb, sql, -1, &stmt, NULL) != SQLITE_OK) {
		g_set_error(error, db_quark(), ACK_ERROR_DATABASE_PREPARE,
				"sqlite3_prepare_v2: %s", sqlite3_errmsg(gdb));
		g_free(sql);
		return false;
	}
	g_free(sql);

	do {
		ret = sqlite3_step(stmt);
		switch (ret) {
		case SQLITE_ROW:
			song = g_new0(struct db_song_data, 1);
			song->id = sqlite3_column_int(stmt, 0);
			song->uri = g_strdup((const char *)sqlite3_column_text(stmt, 1));
			song->tags = g_strsplit((const char *)sqlite3_column_text(stmt, 2), ":", -1);
			*values = g_slist_prepend(*values, song);
			break;
		case SQLITE_DONE:
			break;
		case SQLITE_BUSY:
			/* no-op */
			break;
		default:
			g_set_error(error, db_quark(), ACK_ERROR_DATABASE_STEP,
					"sqlite3_step: %s", sqlite3_errmsg(gdb));
			sqlite3_finalize(stmt);
			return false;
		}
	} while (ret != SQLITE_DONE);

	sqlite3_finalize(stmt);
	return true;
}
