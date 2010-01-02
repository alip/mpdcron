/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009-2010 Ali Polatel <alip@exherbo.org>
 * Based in part upon libmpdclient which is:
 *   Copyright (c) 2003-2009 The Music Player Daemon Project
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

/* TODO: This file could use quite a bit of refactoring.
 */

#include "eugene-defs.h"

#define MPDCRON_WELCOME_MESSAGE "OK MPDCRON "
#define MPDCRON_BUFFER_SIZE 4096

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

struct mpdcron_parser {
	union {
		struct {
			int server;
			unsigned at;
			const char *message;
		} error;

		struct {
			const char *name, *value;
		} pair;
	} u;
};

static GQuark
connection_quark(void)
{
	return g_quark_from_static_string("connection");
}

/**
 * Receiving data
 */
static char *
check_buffer(struct mpdcron_connection *conn)
{
	size_t length;
	const char *p;
	char *newline, *line;

	p = fifo_buffer_read(conn->fifo, &length);
	if (p == NULL)
		return NULL;

	newline = memchr(p, '\n', length);
	if (newline == NULL)
		return NULL;

	*newline = 0;
	line = g_strndup(p, newline - p);
	fifo_buffer_consume(conn->fifo, newline + 1 - p);
	return g_strchomp(line);
}

static char *
mpdcron_recv_line(struct mpdcron_connection *conn)
{
	size_t length;
	gssize count;
	char *buffer, *line;
	GInputStream *input;

	input = g_io_stream_get_input_stream(G_IO_STREAM(conn->stream));

	/* Check if there's data available in buffer */
	if ((line = check_buffer(conn)) != NULL)
		return line;

	/* Prepare buffer for writing */
	if ((buffer = fifo_buffer_write(conn->fifo, &length)) == NULL) {
		g_set_error(&conn->error, connection_quark(),
				MPDCRON_ERROR_OVERFLOW,
				"Buffer overflow");
		return NULL;
	}

	count = g_input_stream_read(input, buffer, length, NULL, &conn->error);
	if (count < 0)
		return NULL;
	else if (count == 0) {
		g_set_error(&conn->error, connection_quark(),
				MPDCRON_ERROR_EOF, "EOF while trying to read line");
		return NULL;
	}
	/* Commit write operation */
	fifo_buffer_append(conn->fifo, count);
	return check_buffer(conn);
}

/**
 * Parsing received data
 */
static enum mpdcron_parser_result
mpdcron_parser_feed(struct mpdcron_parser *parser, char *line)
{
	if (strcmp(line, "OK") == 0)
		return MPDCRON_PARSER_SUCCESS;
	else if (memcmp(line, "ACK", 3) == 0) {
		char *p, *q;

		parser->u.error.server = MPDCRON_ERROR_SERVER_UNK;
		parser->u.error.at = 0;
		parser->u.error.message = NULL;

		/* parse [ACK@AT] */

		p = strchr(line + 3, '[');
		if (p == NULL)
			return MPDCRON_PARSER_ERROR;

		parser->u.error.server = strtol(p + 1, &p, 10);
		if (*p == '@')
			parser->u.error.at = strtol(p + 1, &p, 10);

		q = strchr(p, ']');
		if (q == NULL)
			return MPDCRON_PARSER_MALFORMED;

		/* skip the {COMMAND} */

		p = q + 1;
		q = strchr(p, '{');
		if (q != NULL) {
			q = strchr(p, '}');
			if (q != NULL)
				p = q + 1;
		}

		/* obtain error message */

		while (*p == ' ')
			++p;

		if (*p != 0)
			parser->u.error.message = p;

		return MPDCRON_PARSER_ERROR;
	}
	else {
		/* so this must be a name-value pair */

		char *p;

		p = strchr(line, ':');
		if (p == NULL || p[1] != ' ')
			return MPDCRON_PARSER_MALFORMED;

		*p = 0;

		parser->u.pair.name = line;
		parser->u.pair.value = p + 2;

		return MPDCRON_PARSER_PAIR;
	}
}

static bool
mpdcron_parse_single(struct mpdcron_connection *conn)
{
	int ret;
	char *line;

	line = mpdcron_recv_line(conn);
	if (line == NULL)
		return false;

	ret = mpdcron_parser_feed(conn->parser, line);
	switch (ret) {
		case MPDCRON_PARSER_SUCCESS:
			return true;
		case MPDCRON_PARSER_ERROR:
			g_set_error(&conn->error, connection_quark(),
					conn->parser->u.error.server,
					"%s", conn->parser->u.error.message);
			return false;
		case MPDCRON_PARSER_MALFORMED:
			g_set_error(&conn->error, connection_quark(),
					MPDCRON_ERROR_MALFORMED,
					"Malformed line `%s' received from server", line);
			return false;
		default:
			g_set_error(&conn->error, connection_quark(),
					MPDCRON_ERROR_MALFORMED,
					"Received unexpected name/value pair `%s'", line);
			return false;
	}
	/* never reached */
}

static bool
mpdcron_parse_albums(struct mpdcron_connection *conn, GSList **values)
{
	int ret;
	char *line;
	struct mpdcron_entity *album = NULL;

	for (;;) {
		line = mpdcron_recv_line(conn);
		if (line == NULL) {
			g_free(album);
			return false;
		}

		ret = mpdcron_parser_feed(conn->parser, line);
		switch (ret) {
		case MPDCRON_PARSER_SUCCESS:
			g_free(line);
			if (album != NULL)
				*values = g_slist_prepend(*values, album);
			return true;
		case MPDCRON_PARSER_ERROR:
			g_set_error(&conn->error, connection_quark(),
					conn->parser->u.error.server,
					"%s", conn->parser->u.error.message);
			g_free(album);
			g_free(line);
			return false;
		case MPDCRON_PARSER_MALFORMED:
			g_set_error(&conn->error, connection_quark(),
					MPDCRON_ERROR_MALFORMED,
					"Malformed line `%s' received from server", line);
			g_free(album);
			g_free(line);
			return false;
		default:
			/* We have a pair! */
			if (strcmp(conn->parser->u.pair.name, "id") == 0) {
				if (album != NULL)
					*values = g_slist_prepend(*values, album);
				album = g_new0(struct mpdcron_entity, 1);
				album->id = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Album") == 0) {
				g_assert(album != NULL);
				album->name = g_strdup(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Artist") == 0) {
				g_assert(album != NULL);
				album->artist = g_strdup(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Love") == 0) {
				g_assert(album != NULL);
				album->love = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Kill") == 0) {
				g_assert(album != NULL);
				album->kill = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Rating") == 0) {
				g_assert(album != NULL);
				album->rating = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Play Count") == 0) {
				g_assert(album != NULL);
				album->play_count = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Tag") == 0) {
				g_assert(album != NULL);
				album->tags = g_slist_prepend(album->tags, g_strdup(conn->parser->u.pair.value));
			}
			g_free(line);
			break;
		}
	}
	/* never reached */
	return false;
}

static bool
mpdcron_parse_artists(struct mpdcron_connection *conn, GSList **values)
{
	int ret;
	char *line;
	struct mpdcron_entity *artist = NULL;

	for (;;) {
		line = mpdcron_recv_line(conn);
		if (line == NULL) {
			g_free(artist);
			return false;
		}

		ret = mpdcron_parser_feed(conn->parser, line);
		switch (ret) {
		case MPDCRON_PARSER_SUCCESS:
			g_free(line);
			if (artist != NULL)
				*values = g_slist_prepend(*values, artist);
			return true;
		case MPDCRON_PARSER_ERROR:
			g_set_error(&conn->error, connection_quark(),
					conn->parser->u.error.server,
					"%s", conn->parser->u.error.message);
			g_free(artist);
			g_free(line);
			return false;
		case MPDCRON_PARSER_MALFORMED:
			g_set_error(&conn->error, connection_quark(),
					MPDCRON_ERROR_MALFORMED,
					"Malformed line `%s' received from server", line);
			g_free(artist);
			g_free(line);
			return false;
		default:
			/* We have a pair! */
			if (strcmp(conn->parser->u.pair.name, "id") == 0) {
				if (artist != NULL)
					*values = g_slist_prepend(*values, artist);
				artist = g_new0(struct mpdcron_entity, 1);
				artist->id = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Artist") == 0) {
				g_assert(artist != NULL);
				artist->name = g_strdup(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Love") == 0) {
				g_assert(artist != NULL);
				artist->love = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Kill") == 0) {
				g_assert(artist != NULL);
				artist->kill = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Rating") == 0) {
				g_assert(artist != NULL);
				artist->rating = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Play Count") == 0) {
				g_assert(artist != NULL);
				artist->play_count = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Tag") == 0) {
				g_assert(artist != NULL);
				artist->tags = g_slist_prepend(artist->tags, g_strdup(conn->parser->u.pair.value));
			}
			g_free(line);
			break;
		}
	}
	/* never reached */
	return false;
}

static bool
mpdcron_parse_genres(struct mpdcron_connection *conn, GSList **values)
{
	int ret;
	char *line;
	struct mpdcron_entity *genre = NULL;

	for (;;) {
		line = mpdcron_recv_line(conn);
		if (line == NULL) {
			g_free(genre);
			return false;
		}

		ret = mpdcron_parser_feed(conn->parser, line);
		switch (ret) {
			case MPDCRON_PARSER_SUCCESS:
				g_free(line);
				if (genre != NULL)
					*values = g_slist_prepend(*values, genre);
				return true;
			case MPDCRON_PARSER_ERROR:
				g_set_error(&conn->error, connection_quark(),
						conn->parser->u.error.server,
						"%s", conn->parser->u.error.message);
				g_free(genre);
				g_free(line);
				return false;
			case MPDCRON_PARSER_MALFORMED:
				g_set_error(&conn->error, connection_quark(),
						MPDCRON_ERROR_MALFORMED,
						"Malformed line `%s' received from server", line);
				g_free(genre);
				g_free(line);
				return false;
			default:
				/* We have a pair! */
				if (strcmp(conn->parser->u.pair.name, "id") == 0) {
					if (genre != NULL)
						*values = g_slist_prepend(*values, genre);
					genre = g_new0(struct mpdcron_entity, 1);
					genre->id = atoi(conn->parser->u.pair.value);
				}
				else if (strcmp(conn->parser->u.pair.name, "Genre") == 0) {
					g_assert(genre != NULL);
					genre->name = g_strdup(conn->parser->u.pair.value);
				}
				else if (strcmp(conn->parser->u.pair.name, "Love") == 0) {
					g_assert(genre != NULL);
					genre->love = atoi(conn->parser->u.pair.value);
				}
				else if (strcmp(conn->parser->u.pair.name, "Kill") == 0) {
					g_assert(genre != NULL);
					genre->kill = atoi(conn->parser->u.pair.value);
				}
				else if (strcmp(conn->parser->u.pair.name, "Rating") == 0) {
					g_assert(genre != NULL);
					genre->rating = atoi(conn->parser->u.pair.value);
				}
				else if (strcmp(conn->parser->u.pair.name, "Play Count") == 0) {
					g_assert(genre != NULL);
					genre->play_count = atoi(conn->parser->u.pair.value);
				}
				else if (strcmp(conn->parser->u.pair.name, "Tag") == 0) {
					g_assert(genre != NULL);
					genre->tags = g_slist_prepend(genre->tags,
							g_strdup(conn->parser->u.pair.value));
				}
				g_free(line);
				break;
		}
	}
	/* never reached */
	return false;
}

static bool
mpdcron_parse_songs(struct mpdcron_connection *conn, GSList **values)
{
	int ret;
	char *line;
	struct mpdcron_song *song = NULL;

	for (;;) {
		line = mpdcron_recv_line(conn);
		if (line == NULL) {
			g_free(song);
			return false;
		}

		ret = mpdcron_parser_feed(conn->parser, line);
		switch (ret) {
		case MPDCRON_PARSER_SUCCESS:
			g_free(line);
			if (song != NULL)
				*values = g_slist_prepend(*values, song);
			return true;
		case MPDCRON_PARSER_ERROR:
			g_set_error(&conn->error, connection_quark(),
					conn->parser->u.error.server,
					"%s", conn->parser->u.error.message);
			g_free(song);
			g_free(line);
			return false;
		case MPDCRON_PARSER_MALFORMED:
			g_set_error(&conn->error, connection_quark(),
					MPDCRON_ERROR_MALFORMED,
					"Malformed line `%s' received from server", line);
			g_free(song);
			g_free(line);
			return false;
		default:
			/* We have a pair! */
			if (strcmp(conn->parser->u.pair.name, "id") == 0) {
				if (song != NULL)
					*values = g_slist_prepend(*values, song);
				song = g_new0(struct mpdcron_song, 1);
				song->id = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "file") == 0) {
				g_assert(song != NULL);
				song->uri = g_strdup(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Love") == 0) {
				g_assert(song != NULL);
				song->love = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Kill") == 0) {
				g_assert(song != NULL);
				song->kill = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Rating") == 0) {
				g_assert(song != NULL);
				song->rating = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Play Count") == 0) {
				g_assert(song != NULL);
				song->play_count = atoi(conn->parser->u.pair.value);
			}
			else if (strcmp(conn->parser->u.pair.name, "Tag") == 0) {
				g_assert(song != NULL);
				song->tags = g_slist_prepend(song->tags, g_strdup(conn->parser->u.pair.value));
			}
			g_free(line);
			break;
		}
	}
	/* never reached */
	return false;
}

static bool
mpdcron_parse_welcome(struct mpdcron_connection *conn, const char *output)
{
	const char *tmp;
	char *test;

	if (strncmp(output,MPDCRON_WELCOME_MESSAGE,strlen(MPDCRON_WELCOME_MESSAGE))) {
		g_set_error(&conn->error, connection_quark(),
				MPDCRON_ERROR_MALFORMED,
				"Malformed connect message received");
		return false;
	}

	tmp = &output[strlen(MPDCRON_WELCOME_MESSAGE)];
	conn->version[0] = strtol(tmp, &test, 10);
	if (test == tmp) {
		g_set_error(&conn->error, connection_quark(),
				MPDCRON_ERROR_MALFORMED,
				"Malformed version number in connect message");
		return false;
	}

	if (*test == '.') {
		conn->version[1] = strtol(test + 1, &test, 10);
		if (*test == '.')
			conn->version[2] = strtol(test + 1, &test, 10);
		else
			conn->version[2] = 0;
	} else {
		conn->version[1] = 0;
		conn->version[2] = 0;
	}

	return true;
}

/**
 * Sending data
 */
static bool
mpdcron_send_commandv(struct mpdcron_connection *conn, const char *command,
		va_list args)
{
	const char *arg;
	char *esc_arg;
	GString *cmd;
	GOutputStream *output;

	cmd = g_string_new("");
	g_string_printf(cmd, "%s", command);

	while ((arg = va_arg(args, const char *)) != NULL) {
		/* Append a space separator */
		g_string_append_c(cmd, ' ');

		/* Escape and quote the argument */
		g_string_append_c(cmd, '"');

		esc_arg = g_strescape(arg, "");
		g_string_append(cmd, esc_arg);
		g_free(esc_arg);

		g_string_append_c(cmd, '"');
	}

	/* Add a newline to finish the command */
	g_string_append_c(cmd, '\n');

	/* Send the command */
	output = g_io_stream_get_output_stream(G_IO_STREAM(conn->stream));
	if (!g_output_stream_write_all(output, cmd->str, cmd->len, NULL, NULL, &conn->error)) {
		g_string_free(cmd, TRUE);
		return false;
	}
	g_string_free(cmd, TRUE);
	return true;
}

static bool
mpdcron_send_command(struct mpdcron_connection *conn, const char *command, ...)
{
	bool ret;
	va_list args;

	va_start(args, command);
	ret = mpdcron_send_commandv(conn, command, args);
	va_end(args);
	return ret;
}

/**
 * Global functions
 */
struct mpdcron_connection *
mpdcron_connection_new(const char *hostname, unsigned port)
{
	char *line;
	struct mpdcron_connection *conn;

	conn = g_new0(struct mpdcron_connection, 1);
	conn->client = g_socket_client_new();

	if (hostname[0] == '/') {
#if HAVE_GIO_UNIX
		GSocketAddress *saddr;
		saddr = g_unix_socket_address_new(hostname);

		conn->stream = g_socket_client_connect(conn->client,
				G_SOCKET_CONNECTABLE(saddr), NULL, &conn->error);
#else
		g_set_error(&conn->error, connection_quark(),
				MPDCRON_ERROR_NO_UNIX, "Unix sockets not supported");
		g_object_unref(conn->client);
		conn->client = NULL;
		return conn;
#endif /* HAVE_GIO_UNIX */
	}
	else {
		GSocketConnectable *naddr;
		naddr = g_network_address_new(hostname, port);

		conn->stream = g_socket_client_connect(conn->client,
				naddr, NULL, &conn->error);
		g_object_unref(naddr);
	}

	if (conn->stream == NULL || conn->error != NULL) {
		g_object_unref(conn->client);
		conn->client = NULL;
		return conn;
	}

	conn->fifo = fifo_buffer_new(MPDCRON_BUFFER_SIZE);
	conn->parser = g_new(struct mpdcron_parser, 1);

	/* Parse welcome message */
	if ((line = mpdcron_recv_line(conn)) == NULL) {
		g_io_stream_close(G_IO_STREAM(conn->stream), NULL, NULL);
		fifo_buffer_free(conn->fifo);
		g_free(conn->parser);
		g_object_unref(conn->client);
		conn->fifo = NULL;
		conn->parser = NULL;
		conn->stream = NULL;
		conn->client = NULL;
		return conn;
	}
	mpdcron_parse_welcome(conn, line);
	g_free(line);
	return conn;
}

void
mpdcron_connection_free(struct mpdcron_connection *conn)
{
	if (conn->error != NULL)
		g_error_free(conn->error);
	if (conn->stream != NULL)
		g_object_unref(conn->stream);
	if (conn->client != NULL)
		g_object_unref(conn->client);
	if (conn->fifo != NULL)
		fifo_buffer_free(conn->fifo);
	if (conn->parser != NULL)
		g_free(conn->parser);
	g_free(conn);
}

bool
mpdcron_password(struct mpdcron_connection *conn, const char *password)
{
	g_assert(conn != NULL);
	g_assert(password != NULL);

	if (!mpdcron_send_command(conn, "password", password, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_list_album_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values)
{
	g_assert(conn != NULL);
	g_assert(values != NULL);

	if (!mpdcron_send_command(conn, "list_album", expr, NULL))
		return false;
	return mpdcron_parse_albums(conn, values);
}

bool
mpdcron_list_artist_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values)
{
	g_assert(conn != NULL);
	g_assert(values != NULL);

	if (!mpdcron_send_command(conn, "list_artist", expr, NULL))
		return false;
	return mpdcron_parse_artists(conn, values);
}

bool
mpdcron_list_genre_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values)
{
	g_assert(conn != NULL);
	g_assert(values != NULL);

	if (!mpdcron_send_command(conn, "list_genre", expr, NULL))
		return false;
	return mpdcron_parse_genres(conn, values);
}

bool
mpdcron_list_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values)
{
	g_assert(conn != NULL);
	g_assert(values != NULL);

	if (!mpdcron_send_command(conn, "list", expr, NULL))
		return false;
	return mpdcron_parse_songs(conn, values);
}

bool
mpdcron_listinfo_album_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values)
{
	g_assert(conn != NULL);
	g_assert(values != NULL);

	if (!mpdcron_send_command(conn, "listinfo_album", expr, NULL))
		return false;
	return mpdcron_parse_albums(conn, values);
}

bool
mpdcron_listinfo_artist_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values)
{
	g_assert(conn != NULL);
	g_assert(values != NULL);

	if (!mpdcron_send_command(conn, "listinfo_artist", expr, NULL))
		return false;
	return mpdcron_parse_artists(conn, values);
}

bool
mpdcron_listinfo_genre_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values)
{
	g_assert(conn != NULL);
	g_assert(values != NULL);

	if (!mpdcron_send_command(conn, "listinfo_genre", expr, NULL))
		return false;
	return mpdcron_parse_genres(conn, values);
}

bool
mpdcron_listinfo_expr(struct mpdcron_connection *conn,
		const char *expr, GSList **values)
{
	g_assert(conn != NULL);
	g_assert(values != NULL);

	if (!mpdcron_send_command(conn, "listinfo", expr, NULL))
		return false;
	return mpdcron_parse_songs(conn, values);
}


bool
mpdcron_love_album_expr(struct mpdcron_connection *conn, bool love,
		const char *expr)
{
	g_assert(conn != NULL);

	if (!mpdcron_send_command(conn, love ? "love_album" : "hate_album", expr, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_love_artist_expr(struct mpdcron_connection *conn, bool love,
		const char *expr)
{
	g_assert(conn != NULL);

	if (!mpdcron_send_command(conn, love ? "love_artist" : "hate_artist", expr, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_love_genre_expr(struct mpdcron_connection *conn, bool love,
		const char *expr)
{
	g_assert(conn != NULL);

	if (!mpdcron_send_command(conn, love ? "love_genre" : "hate_genre", expr, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_love_expr(struct mpdcron_connection *conn, bool love,
		const char *expr)
{
	g_assert(conn != NULL);

	if (!mpdcron_send_command(conn, love ? "love" : "hate", expr, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_kill_album_expr(struct mpdcron_connection *conn, bool kkill,
		const char *expr)
{
	g_assert(conn != NULL);

	if (!mpdcron_send_command(conn, kkill ? "kill_album" : "unkill_album", expr, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_kill_artist_expr(struct mpdcron_connection *conn, bool kkill,
		const char *expr)
{
	g_assert(conn != NULL);

	if (!mpdcron_send_command(conn, kkill ? "kill_artist" : "unkill_artist", expr, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_kill_genre_expr(struct mpdcron_connection *conn, bool kkill,
		const char *expr)
{
	g_assert(conn != NULL);

	if (!mpdcron_send_command(conn, kkill ? "kill_genre" : "unkill_genre", expr, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_kill_expr(struct mpdcron_connection *conn, bool kkill,
		const char *expr)
{
	g_assert(conn != NULL);

	if (!mpdcron_send_command(conn, kkill ? "kill" : "unkill", expr, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_rate_album_expr(struct mpdcron_connection *conn, const char *expr,
		const char *rating)
{
	g_assert(conn != NULL);

	if (!mpdcron_send_command(conn, "rate_album", expr, rating, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_rate_artist_expr(struct mpdcron_connection *conn, const char *expr,
		const char *rating)
{
	g_assert(conn != NULL);

	if (!mpdcron_send_command(conn, "rate_artist", expr, rating, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_rate_genre_expr(struct mpdcron_connection *conn, const char *expr,
		const char *rating)
{
	g_assert(conn != NULL);

	if (!mpdcron_send_command(conn, "rate_genre", expr, rating, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_rate_expr(struct mpdcron_connection *conn, const char *expr,
		const char *rating)
{
	g_assert(conn != NULL);

	if (!mpdcron_send_command(conn, "rate", expr, rating, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_addtag_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag)
{
	g_assert(conn != NULL);
	g_assert(expr != NULL);
	g_assert(tag != NULL);

	if (!mpdcron_send_command(conn, "addtag", expr, tag, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_addtag_artist_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag)
{
	g_assert(conn != NULL);
	g_assert(expr != NULL);
	g_assert(tag != NULL);

	if (!mpdcron_send_command(conn, "addtag_artist", expr, tag, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_addtag_album_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag)
{
	g_assert(conn != NULL);
	g_assert(expr != NULL);
	g_assert(tag != NULL);

	if (!mpdcron_send_command(conn, "addtag_album", expr, tag, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_addtag_genre_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag)
{
	g_assert(conn != NULL);
	g_assert(expr != NULL);
	g_assert(tag != NULL);

	if (!mpdcron_send_command(conn, "addtag_genre", expr, tag, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_rmtag_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag)
{
	g_assert(conn != NULL);
	g_assert(expr != NULL);
	g_assert(tag != NULL);

	if (!mpdcron_send_command(conn, "rmtag", expr, tag, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_rmtag_artist_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag)
{
	g_assert(conn != NULL);
	g_assert(expr != NULL);
	g_assert(tag != NULL);

	if (!mpdcron_send_command(conn, "rmtag_artist", expr, tag, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_rmtag_album_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag)
{
	g_assert(conn != NULL);
	g_assert(expr != NULL);
	g_assert(tag != NULL);

	if (!mpdcron_send_command(conn, "rmtag_album", expr, tag, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_rmtag_genre_expr(struct mpdcron_connection *conn, const char *expr,
		const char *tag)
{
	g_assert(conn != NULL);
	g_assert(expr != NULL);
	g_assert(tag != NULL);

	if (!mpdcron_send_command(conn, "rmtag_genre", expr, tag, NULL))
		return false;
	return mpdcron_parse_single(conn);
}

bool
mpdcron_listtags_expr(struct mpdcron_connection *conn, const char *expr,
		GSList **values)
{
	g_assert(conn != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	if (!mpdcron_send_command(conn, "listtags", expr, NULL))
		return false;
	return mpdcron_parse_songs(conn, values);
}

bool
mpdcron_listtags_album_expr(struct mpdcron_connection *conn, const char *expr,
		GSList **values)
{
	g_assert(conn != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	if (!mpdcron_send_command(conn, "listtags_album", expr, NULL))
		return false;
	return mpdcron_parse_albums(conn, values);
}

bool
mpdcron_listtags_artist_expr(struct mpdcron_connection *conn, const char *expr,
		GSList **values)
{
	g_assert(conn != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	if (!mpdcron_send_command(conn, "listtags_artist", expr, NULL))
		return false;
	return mpdcron_parse_artists(conn, values);
}

bool
mpdcron_listtags_genre_expr(struct mpdcron_connection *conn, const char *expr,
		GSList **values)
{
	g_assert(conn != NULL);
	g_assert(expr != NULL);
	g_assert(values != NULL);

	if (!mpdcron_send_command(conn, "listtags_genre", expr, NULL))
		return false;
	return mpdcron_parse_genres(conn, values);
}
