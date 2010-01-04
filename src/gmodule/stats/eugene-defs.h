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

#ifndef MPDCRON_EUGENE_DEFS_H
#define MPDCRON_EUGENE_DEFS_H 1

#include "../../cron-config.h"

#include <stdbool.h>

#include <glib.h>
#include <gio/gio.h>

#define ENV_MPDCRON_HOST	"MPDCRON_HOST"
#define ENV_MPDCRON_PORT	"MPDCRON_PORT"
#define ENV_MPDCRON_PASSWORD	"MPDCRON_PASSWORD"
#define DEFAULT_HOSTNAME	"localhost"
#define DEFAULT_PORT		6601

enum mpdcron_error {
	MPDCRON_ERROR_NO_UNIX,
	MPDCRON_ERROR_BAD_ADDRESS,
	MPDCRON_ERROR_MALFORMED,
	MPDCRON_ERROR_OVERFLOW,
	MPDCRON_ERROR_EOF,
	MPDCRON_ERROR_SERVER_UNK,
};

enum mpdcron_parser_result {
	/**
	 * Response line was not understood.
	 */
	MPDCRON_PARSER_MALFORMED,

	/**
	 * mpdcron has returned "OK".
	 */
	MPDCRON_PARSER_SUCCESS,

	/**
	 * mpdcron has returned "ACK" with an error code.  Call
	 * mpd_parser_get_server_error() to get the error code.
	 */
	MPDCRON_PARSER_ERROR,

	/**
	 * mpdcron has returned a name-value pair.  Call
	 * mpdcron_parser_get_name() and mpdcron_parser_get_value().
	 */
	MPDCRON_PARSER_PAIR,
};

struct mpdcron_song {
	int id;
	int play_count;
	char *uri;
	int love;
	int kill;
	int rating;
	GSList *tags;
};

struct mpdcron_entity {
	int id;
	int play_count;
	char *name;
	char *artist;
	int love;
	int kill;
	int rating;
	GSList *tags;
};

struct mpdcron_parser;

struct mpdcron_connection {
	/**
	 * The version number reported by mpdcron server.
	 */
	unsigned version[3];

	/**
	 * The last error occured.
	 */
	GError *error;

	/**
	 * Client object.
	 */
	GSocketClient *client;

	/**
	 * IO Stream that represents the connection.
	 */
	GSocketConnection *stream;

	/**
	 * Data Input stream
	 */
	GDataInputStream *input;

	/**
	 * Parser
	 */
	struct mpdcron_parser *parser;
};

struct mpdcron_connection *
mpdcron_connection_new(const char *hostname, unsigned port);

void
mpdcron_connection_free(struct mpdcron_connection *conn);

bool
mpdcron_password(struct mpdcron_connection *conn, const char *password);

bool
mpdcron_list_album_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values);

bool
mpdcron_list_artist_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values);

bool
mpdcron_list_genre_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values);

bool
mpdcron_list_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values);

bool
mpdcron_listinfo_album_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values);

bool
mpdcron_listinfo_artist_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values);

bool
mpdcron_listinfo_genre_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values);

bool
mpdcron_listinfo_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values);

bool
mpdcron_love_album_expr(struct mpdcron_connection *conn, bool love,
		const char *expr, int *changes);

bool
mpdcron_love_artist_expr(struct mpdcron_connection *conn, bool love,
		const char *expr, int *changes);

bool
mpdcron_love_genre_expr(struct mpdcron_connection *conn, bool love,
		const char *expr, int *changes);

bool
mpdcron_love_expr(struct mpdcron_connection *conn, bool love,
		const char *expr, int *changes);

bool
mpdcron_kill_album_expr(struct mpdcron_connection *conn, bool kkill,
		const char *expr, int *changes);

bool
mpdcron_kill_artist_expr(struct mpdcron_connection *conn, bool kkill,
		const char *expr, int *changes);

bool
mpdcron_kill_genre_expr(struct mpdcron_connection *conn, bool kkill,
		const char *expr, int *changes);

bool
mpdcron_kill_expr(struct mpdcron_connection *conn, bool kkill,
		const char *expr, int *changes);

bool
mpdcron_rate_album_expr(struct mpdcron_connection *conn,
		const char *rating, const char *expr,
		int *changes);

bool
mpdcron_rate_artist_expr(struct mpdcron_connection *conn,
		const char *rating, const char *expr,
		int *changes);

bool
mpdcron_rate_genre_expr(struct mpdcron_connection *conn,
		const char *rating, const char *expr,
		int *changes);

bool
mpdcron_rate_expr(struct mpdcron_connection *conn,
		const char *rating, const char *expr,
		int *changes);

bool
mpdcron_addtag_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag, int *changes);

bool
mpdcron_addtag_artist_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag, int *changes);

bool
mpdcron_addtag_album_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag, int *changes);

bool
mpdcron_addtag_genre_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag, int *changes);

bool
mpdcron_rmtag_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag, int *changes);

bool
mpdcron_rmtag_artist_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag, int *changes);

bool
mpdcron_rmtag_album_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag, int *changes);

bool
mpdcron_rmtag_genre_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag, int *changes);

bool
mpdcron_listtags_expr(struct mpdcron_connection *conn, const char *expr,
		GSList **values);

bool
mpdcron_listtags_album_expr(struct mpdcron_connection *conn, const char *expr,
		GSList **values);

bool
mpdcron_listtags_artist_expr(struct mpdcron_connection *conn, const char *expr,
		GSList **values);

bool
mpdcron_listtags_genre_expr(struct mpdcron_connection *conn, const char *expr,
		GSList **values);

bool
mpdcron_count_album_expr(struct mpdcron_connection *conn, const char *expr,
		const char *count, int *changes);

bool
mpdcron_count_artist_expr(struct mpdcron_connection *conn, const char *expr,
		const char *count, int *changes);

bool
mpdcron_count_genre_expr(struct mpdcron_connection *conn, const char *expr,
		const char *count, int *changes);

bool
mpdcron_count_expr(struct mpdcron_connection *conn, const char *expr,
		const char *count, int *changes);

char *
quote(const char *src);

struct mpd_song *
load_current_song(void);

int
cmd_hate(int argc, char **argv);

int
cmd_love(int argc, char **argv);

int
cmd_kill(int argc, char **argv);

int
cmd_unkill(int argc, char **argv);

int
cmd_rate(int argc, char **argv);

int
cmd_list(int argc, char **argv);

int
cmd_listinfo(int argc, char **argv);

int
cmd_addtag(int argc, char **argv);

int
cmd_rmtag(int argc, char **argv);

int
cmd_listtags(int argc, char **argv);

int
cmd_count(int argc, char **argv);

void
eulog(int level, const char *fmt, ...);

#endif /* !MPDCRON_EUGENE_DEFS_H */
