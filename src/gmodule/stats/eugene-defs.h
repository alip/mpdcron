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

#ifndef MPDCRON_EUGENE_DEFS_H
#define MPDCRON_EUGENE_DEFS_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "fifo_buffer.h"

#include <stdbool.h>
#include <syslog.h>

#include <glib.h>
#include <gio/gio.h>

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
	char *uri;
	int love;
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
	 * Client read buffer
	 */
	struct fifo_buffer *fifo;

	/**
	 * Parser
	 */
	struct mpdcron_parser *parser;
};

struct mpdcron_connection *mpdcron_connection_new(const char *hostname, unsigned port);
void mpdcron_connection_free(struct mpdcron_connection *conn);
bool mpdcron_password(struct mpdcron_connection *conn, const char *password);
bool mpdcron_love_expr(struct mpdcron_connection *conn, const char *expr, bool love, GSList **values);
bool mpdcron_love_uri(struct mpdcron_connection *conn, const char *uri, bool love, GSList **values);

void eulog(int level, const char *fmt, ...);

#endif /* !MPDCRON_EUGENE_DEFS_H */
