/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009, 2010 Ali Polatel <alip@exherbo.org>
 * Based in part upon mpd which is:
 *   Copyright (C) 2003-2009 The Music Player Daemon Project
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

#include "stats-defs.h"
#include "tokenizer.h"

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>
#include <gio/gio.h>

#define PROTOCOL_OK	"OK"
#define PROCOTOL_ACK	"ACK"

/* if min: -1 don't check args *
 * if max: -1 no max args      */
struct command {
	const char *cmd;
	unsigned permission;
	int min;
	int max;
	enum command_return (*handler)(struct client *client, int argc, char **argv);
};

static const char *current_command;

static int
command_authorizer(void *userdata, int what,
		G_GNUC_UNUSED const char *arg1,
		const char *arg2,
		G_GNUC_UNUSED const char *dbname,
		G_GNUC_UNUSED const char *view)
{
	struct client *client = (struct client *) userdata;

	switch (what) {
	case SQLITE_READ:
	case SQLITE_SELECT:
		return SQLITE_OK;
	case SQLITE_FUNCTION:
		/* We allow everything but load_extension() */
		if (strcmp(arg2, "load_extension") == 0)
			return SQLITE_DENY;
		return SQLITE_OK;
	case SQLITE_UPDATE:
		if (client->perm & PERMISSION_UPDATE)
			return SQLITE_OK;
		/* fall through */
	default:
		return SQLITE_DENY;
	}
}

static void
command_ok(struct client *client)
{
	g_debug("[%d]> "PROTOCOL_OK, client->id);
	server_schedule_write(client, PROTOCOL_OK"\n", sizeof(PROTOCOL_OK"\n") - 1);
	server_flush_write(client);
}

static void
command_putv(struct client *client, const char *fmt, va_list args)
{
	GString *message;

	g_assert(client != NULL);

	message = g_string_new("");
	g_string_append_vprintf(message, fmt, args);

	g_debug("[%d]> %s", client->id, message->str);
	g_string_append_c(message, '\n');

	server_schedule_write(client, message->str, message->len);
	g_string_free(message, TRUE);
}

G_GNUC_PRINTF(2, 3)
static void
command_puts(struct client *client, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	command_putv(client, fmt, args);
	va_end(args);
}

static void
command_error_v(struct client *client, enum ack error, const char *fmt,
		va_list args)
{
	GString *message;

	g_assert(client != NULL);
	g_assert(current_command != NULL);

	message = g_string_new("");
	g_string_printf(message, PROCOTOL_ACK" [%i] {%s} ",
			(int)error, current_command);
	g_string_append_vprintf(message, fmt, args);

	g_debug("[%d]> %s", client->id, message->str);
	g_string_append_c(message, '\n');

	server_schedule_write(client, message->str, message->len);
	g_string_free(message, TRUE);
	current_command = NULL;

	server_flush_write(client);
}

G_GNUC_PRINTF(3, 4)
static void
command_error(struct client *client, enum ack error, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	command_error_v(client, error, fmt, args);
	va_end(args);
}

G_GNUC_UNUSED
static bool
check_bool(struct client *client, bool *value_r, const char *s)
{
	long value;
	char *endptr;

	value = strtol(s, &endptr, 10);
	if (*endptr != 0 || (value != 0 && value != 1)) {
		command_error(client, ACK_ERROR_ARG,
				"Boolean (0/1) expected: %s", s);
		return false;
	}

	*value_r = !!value;
	return true;
}

static enum command_return
handle_kill(struct client *client, int argc, char **argv)
{
	bool kkill;
	int changes;
	GError *error;

	g_assert(argc == 2);

	kkill = (strcmp(argv[0], "kill") == 0);

	error = NULL;
	if (!db_kill_song_expr(argv[1], kkill, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_kill_album(struct client *client, int argc, char **argv)
{
	bool kkill;
	int changes;
	GError *error;

	g_assert(argc == 2);

	kkill = (strcmp(argv[0], "kill_album") == 0);

	error = NULL;
	if (!db_kill_album_expr(argv[1], kkill, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_kill_artist(struct client *client, int argc, char **argv)
{
	bool kkill;
	int changes;
	GError *error;

	g_assert(argc == 2);

	kkill = (strcmp(argv[0], "kill_artist") == 0);

	error = NULL;
	if (!db_kill_artist_expr(argv[1], kkill, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_kill_genre(struct client *client, int argc, char **argv)
{
	bool kkill;
	int changes;
	GError *error;

	g_assert(argc == 2);

	kkill = (strcmp(argv[0], "kill_genre") == 0);

	error = NULL;
	if (!db_kill_genre_expr(argv[1], kkill, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_list(struct client *client, int argc, char **argv)
{
	GError *error;
	GSList *values, *walk;

	g_assert(argc == 2);

	error = NULL;
	values = NULL;
	if (!db_list_song_expr(argv[1], &values, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct db_song_data *song = (struct db_song_data *) walk->data;
		command_puts(client, "id: %d", song->id);
		command_puts(client, "file: %s", song->uri);
		db_song_data_free(song);
	}
	g_slist_free(values);

	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_list_artist(struct client *client, int argc, char **argv)
{
	GError *error;
	GSList *values, *walk;

	g_assert(argc == 2);

	error = NULL;
	values = NULL;
	if (!db_list_artist_expr(argv[1], &values, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct db_generic_data *data = (struct db_generic_data *) walk->data;
		command_puts(client, "id: %d", data->id);
		command_puts(client, "Artist: %s", data->name);
		db_generic_data_free(data);
	}
	g_slist_free(values);

	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_list_album(struct client *client, int argc, char **argv)
{
	GError *error;
	GSList *values, *walk;

	g_assert(argc == 2);

	error = NULL;
	values = NULL;
	if (!db_list_album_expr(argv[1], &values, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct db_generic_data *data = (struct db_generic_data *) walk->data;
		command_puts(client, "id: %d", data->id);
		command_puts(client, "Album: %s", data->name);
		command_puts(client, "Artist: %s", data->artist);
		db_generic_data_free(data);
	}
	g_slist_free(values);

	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_list_genre(struct client *client, int argc, char **argv)
{
	GError *error;
	GSList *values, *walk;

	g_assert(argc == 2);

	error = NULL;
	values = NULL;
	if (!db_list_genre_expr(argv[1], &values, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct db_generic_data *data = (struct db_generic_data *) walk->data;
		command_puts(client, "id: %d", data->id);
		command_puts(client, "Genre: %s", data->name);
		db_generic_data_free(data);
	}
	g_slist_free(values);

	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_listinfo(struct client *client, int argc, char **argv)
{
	GError *error;
	GSList *values, *walk;
	struct tm *utc;
	char last_played[25];

	g_assert(argc == 2);

	error = NULL;
	values = NULL;
	if (!db_listinfo_song_expr(argv[1], &values, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct db_song_data *song = (struct db_song_data *) walk->data;
		command_puts(client, "id: %d", song->id);
		command_puts(client, "file: %s", song->uri);
		command_puts(client, "Play Count: %d", song->play_count);
		command_puts(client, "Love: %d", song->love);
		command_puts(client, "Kill: %d", song->kill);
		command_puts(client, "Rating: %d", song->rating);
		command_puts(client, "Karma: %d", song->karma);
		if (song->last_played != 0) {
			utc = gmtime(&(song->last_played));
			strftime(last_played, sizeof(last_played),
					"%Y-%m-%dT%H:%M:%S%z", utc);
			command_puts(client, "Last Played: %s", last_played);
		}
		db_song_data_free(song);
	}
	g_slist_free(values);

	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_listinfo_artist(struct client *client, int argc, char **argv)
{
	GError *error;
	GSList *values, *walk;

	g_assert(argc == 2);

	error = NULL;
	values = NULL;
	if (!db_listinfo_artist_expr(argv[1], &values, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct db_generic_data *data = (struct db_generic_data *) walk->data;
		command_puts(client, "id: %d", data->id);
		command_puts(client, "Artist: %s", data->name);
		command_puts(client, "Play Count: %d", data->play_count);
		command_puts(client, "Love: %d", data->love);
		command_puts(client, "Kill: %d", data->kill);
		command_puts(client, "Rating: %d", data->rating);
		db_generic_data_free(data);
	}
	g_slist_free(values);

	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_listinfo_album(struct client *client, int argc, char **argv)
{
	GError *error;
	GSList *values, *walk;

	g_assert(argc == 2);

	error = NULL;
	values = NULL;
	if (!db_listinfo_album_expr(argv[1], &values, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct db_generic_data *data = (struct db_generic_data *) walk->data;
		command_puts(client, "id: %d", data->id);
		command_puts(client, "Album: %s", data->name);
		command_puts(client, "Artist: %s", data->artist);
		command_puts(client, "Play Count: %d", data->play_count);
		command_puts(client, "Love: %d", data->love);
		command_puts(client, "Kill: %d", data->kill);
		command_puts(client, "Rating: %d", data->rating);
		db_generic_data_free(data);
	}
	g_slist_free(values);

	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_listinfo_genre(struct client *client, int argc, char **argv)
{
	GError *error;
	GSList *values, *walk;

	g_assert(argc == 2);

	error = NULL;
	values = NULL;
	if (!db_listinfo_genre_expr(argv[1], &values, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct db_generic_data *data = (struct db_generic_data *) walk->data;
		command_puts(client, "id: %d", data->id);
		command_puts(client, "Genre: %s", data->name);
		command_puts(client, "Play Count: %d", data->play_count);
		command_puts(client, "Love: %d", data->love);
		command_puts(client, "Kill: %d", data->kill);
		command_puts(client, "Rating: %d", data->rating);
		db_generic_data_free(data);
	}
	g_slist_free(values);

	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_love(struct client *client, int argc, char **argv)
{
	bool love;
	int changes;
	GError *error;

	g_assert(argc == 2);

	love = (strcmp(argv[0], "love") == 0);

	error = NULL;
	if (!db_love_song_expr(argv[1], love, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_love_album(struct client *client, int argc, char **argv)
{
	bool love;
	int changes;
	GError *error;

	g_assert(argc == 2);

	love = (strcmp(argv[0], "love_album") == 0);

	error = NULL;
	if (!db_love_album_expr(argv[1], love, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_love_artist(struct client *client, int argc, char **argv)
{
	bool love;
	int changes;
	GError *error;

	g_assert(argc == 2);

	love = (strcmp(argv[0], "love_artist") == 0);

	error = NULL;
	if (!db_love_artist_expr(argv[1], love, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_love_genre(struct client *client, int argc, char **argv)
{
	bool love;
	int changes;
	GError *error;

	g_assert(argc == 2);

	love = (strcmp(argv[0], "love_genre") == 0);

	error = NULL;
	if (!db_love_genre_expr(argv[1], love, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_rate(struct client *client, int argc, char **argv)
{
	int changes;
	long rating;
	char *endptr;
	GError *error;

	g_assert(argc == 3);

	/* Convert second argument to number */
	errno = 0;
	endptr = NULL;
	rating = strtol(argv[2], &endptr, 10);
	if (errno != 0) {
		command_error(client, ACK_ERROR_ARG,
				"Failed to convert to number: %s",
				g_strerror(errno));
		return COMMAND_RETURN_ERROR;
	}
	else if (endptr == argv[2]) {
		command_error(client, ACK_ERROR_ARG, "No digits found");
		return COMMAND_RETURN_ERROR;
	}
	else if (rating > INT_MAX || rating < INT_MIN) {
		command_error(client, ACK_ERROR_ARG,
				"Number too %s",
				(rating > INT_MAX) ? "big" : "small");
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	if (!db_rate_song_expr(argv[1], (int)rating, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_rate_artist(struct client *client, int argc, char **argv)
{
	int changes;
	long rating;
	char *endptr;
	GError *error;

	g_assert(argc == 3);

	/* Convert second argument to number */
	errno = 0;
	endptr = NULL;
	rating = strtol(argv[2], &endptr, 10);
	if (errno != 0) {
		command_error(client, ACK_ERROR_ARG,
				"Failed to convert to number: %s",
				g_strerror(errno));
		return COMMAND_RETURN_ERROR;
	}
	else if (endptr == argv[2]) {
		command_error(client, ACK_ERROR_ARG, "No digits found");
		return COMMAND_RETURN_ERROR;
	}
	else if (rating > INT_MAX || rating < INT_MIN) {
		command_error(client, ACK_ERROR_ARG,
				"Number too %s",
				(rating > INT_MAX) ? "big" : "small");
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	if (!db_rate_artist_expr(argv[1], (int)rating, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_rate_album(struct client *client, int argc, char **argv)
{
	int changes;
	long rating;
	char *endptr;
	GError *error;

	g_assert(argc == 3);

	/* Convert second argument to number */
	errno = 0;
	endptr = NULL;
	rating = strtol(argv[2], &endptr, 10);
	if (errno != 0) {
		command_error(client, ACK_ERROR_ARG,
				"Failed to convert to number: %s",
				g_strerror(errno));
		return COMMAND_RETURN_ERROR;
	}
	else if (endptr == argv[2]) {
		command_error(client, ACK_ERROR_ARG, "No digits found");
		return COMMAND_RETURN_ERROR;
	}
	else if (rating > INT_MAX || rating < INT_MIN) {
		command_error(client, ACK_ERROR_ARG,
				"Number too %s",
				(rating > INT_MAX) ? "big" : "small");
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	if (!db_rate_album_expr(argv[1], (int)rating, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_rate_genre(struct client *client, int argc, char **argv)
{
	int changes;
	long rating;
	char *endptr;
	GError *error;

	g_assert(argc == 3);

	/* Convert second argument to number */
	errno = 0;
	endptr = NULL;
	rating = strtol(argv[2], &endptr, 10);
	if (errno != 0) {
		command_error(client, ACK_ERROR_ARG,
				"Failed to convert to number: %s",
				g_strerror(errno));
		return COMMAND_RETURN_ERROR;
	}
	else if (endptr == argv[2]) {
		command_error(client, ACK_ERROR_ARG, "No digits found");
		return COMMAND_RETURN_ERROR;
	}
	else if (rating > INT_MAX || rating < INT_MIN) {
		command_error(client, ACK_ERROR_ARG,
				"Number too %s",
				(rating > INT_MAX) ? "big" : "small");
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	if (!db_rate_genre_expr(argv[1], (int)rating, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_rate_absolute(struct client *client, int argc, char **argv)
{
	int changes;
	long rating;
	char *endptr;
	GError *error;

	g_assert(argc == 3);

	/* Convert second argument to number */
	errno = 0;
	endptr = NULL;
	rating = strtol(argv[2], &endptr, 10);
	if (errno != 0) {
		command_error(client, ACK_ERROR_ARG,
				"Failed to convert to number: %s",
				g_strerror(errno));
		return COMMAND_RETURN_ERROR;
	}
	else if (endptr == argv[2]) {
		command_error(client, ACK_ERROR_ARG, "No digits found");
		return COMMAND_RETURN_ERROR;
	}
	else if (rating > INT_MAX || rating < INT_MIN) {
		command_error(client, ACK_ERROR_ARG,
				"Number too %s",
				(rating > INT_MAX) ? "big" : "small");
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	if (!db_rate_absolute_song_expr(argv[1], (int)rating, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_rate_absolute_artist(struct client *client, int argc, char **argv)
{
	int changes;
	long rating;
	char *endptr;
	GError *error;

	g_assert(argc == 3);

	/* Convert second argument to number */
	errno = 0;
	endptr = NULL;
	rating = strtol(argv[2], &endptr, 10);
	if (errno != 0) {
		command_error(client, ACK_ERROR_ARG,
				"Failed to convert to number: %s",
				g_strerror(errno));
		return COMMAND_RETURN_ERROR;
	}
	else if (endptr == argv[2]) {
		command_error(client, ACK_ERROR_ARG, "No digits found");
		return COMMAND_RETURN_ERROR;
	}
	else if (rating > INT_MAX || rating < INT_MIN) {
		command_error(client, ACK_ERROR_ARG,
				"Number too %s",
				(rating > INT_MAX) ? "big" : "small");
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	if (!db_rate_absolute_artist_expr(argv[1], (int)rating, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_rate_absolute_album(struct client *client, int argc, char **argv)
{
	int changes;
	long rating;
	char *endptr;
	GError *error;

	g_assert(argc == 3);

	/* Convert second argument to number */
	errno = 0;
	endptr = NULL;
	rating = strtol(argv[2], &endptr, 10);
	if (errno != 0) {
		command_error(client, ACK_ERROR_ARG,
				"Failed to convert to number: %s",
				g_strerror(errno));
		return COMMAND_RETURN_ERROR;
	}
	else if (endptr == argv[2]) {
		command_error(client, ACK_ERROR_ARG, "No digits found");
		return COMMAND_RETURN_ERROR;
	}
	else if (rating > INT_MAX || rating < INT_MIN) {
		command_error(client, ACK_ERROR_ARG,
				"Number too %s",
				(rating > INT_MAX) ? "big" : "small");
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	if (!db_rate_absolute_album_expr(argv[1], (int)rating, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_rate_absolute_genre(struct client *client, int argc, char **argv)
{
	int changes;
	long rating;
	char *endptr;
	GError *error;

	g_assert(argc == 3);

	/* Convert second argument to number */
	errno = 0;
	endptr = NULL;
	rating = strtol(argv[2], &endptr, 10);
	if (errno != 0) {
		command_error(client, ACK_ERROR_ARG,
				"Failed to convert to number: %s",
				g_strerror(errno));
		return COMMAND_RETURN_ERROR;
	}
	else if (endptr == argv[2]) {
		command_error(client, ACK_ERROR_ARG, "No digits found");
		return COMMAND_RETURN_ERROR;
	}
	else if (rating > INT_MAX || rating < INT_MIN) {
		command_error(client, ACK_ERROR_ARG,
				"Number too %s",
				(rating > INT_MAX) ? "big" : "small");
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	if (!db_rate_absolute_genre_expr(argv[1], (int)rating, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_addtag(struct client *client, int argc, char **argv)
{
	int changes;
	GError *error;

	g_assert(argc == 3);

	error = NULL;
	if (!db_add_song_tag_expr(argv[1], argv[2], &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_addtag_album(struct client *client, int argc, char **argv)
{
	int changes;
	GError *error;

	g_assert(argc == 3);

	error = NULL;
	if (!db_add_album_tag_expr(argv[1], argv[2], &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_addtag_artist(struct client *client, int argc, char **argv)
{
	int changes;
	GError *error;

	g_assert(argc == 3);

	error = NULL;
	if (!db_add_artist_tag_expr(argv[1], argv[2], &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_addtag_genre(struct client *client, int argc, char **argv)
{
	int changes;
	GError *error;

	g_assert(argc == 3);

	error = NULL;
	if (!db_add_genre_tag_expr(argv[1], argv[2], &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_rmtag(struct client *client, int argc, char **argv)
{
	int changes;
	GError *error;

	g_assert(argc == 3);

	error = NULL;
	if (!db_remove_song_tag_expr(argv[1], argv[2], &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_rmtag_album(struct client *client, int argc, char **argv)
{
	int changes;
	GError *error;

	g_assert(argc == 3);

	error = NULL;
	if (!db_remove_album_tag_expr(argv[1], argv[2], &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_rmtag_artist(struct client *client, int argc, char **argv)
{
	int changes;
	GError *error;

	g_assert(argc == 3);

	error = NULL;
	if (!db_remove_artist_tag_expr(argv[1], argv[2], &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_rmtag_genre(struct client *client, int argc, char **argv)
{
	int changes;
	GError *error;

	g_assert(argc == 3);

	error = NULL;
	if (!db_remove_genre_tag_expr(argv[1], argv[2], &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_listtags(struct client *client, int argc, char **argv)
{
	GError *error;
	GSList *values, *walk;

	g_assert(argc == 2);

	error = NULL;
	values = NULL;
	if (!db_list_song_tag_expr(argv[1], &values, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct db_song_data *song = (struct db_song_data *)walk->data;
		command_puts(client, "id: %d", song->id);
		command_puts(client, "file: %s", song->uri);
		for (unsigned int i = 0; song->tags[i] != NULL; i++) {
			if (song->tags[i][0] != '\0')
				command_puts(client, "Tag: %s", song->tags[i]);
		}
		db_song_data_free(song);
	}
	g_slist_free(values);

	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_listtags_album(struct client *client, int argc, char **argv)
{
	GError *error;
	GSList *values, *walk;

	g_assert(argc == 2);

	error = NULL;
	values = NULL;
	if (!db_list_album_tag_expr(argv[1], &values, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct db_generic_data *data = (struct db_generic_data *)walk->data;
		command_puts(client, "id: %d", data->id);
		command_puts(client, "Album: %s", data->name);
		command_puts(client, "Artist: %s", data->artist);
		for (unsigned int i = 0; data->tags[i] != NULL; i++) {
			if (data->tags[i][0] != '\0')
				command_puts(client, "Tag: %s", data->tags[i]);
		}
		db_generic_data_free(data);
	}
	g_slist_free(values);

	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_listtags_artist(struct client *client, int argc, char **argv)
{
	GError *error;
	GSList *values, *walk;

	g_assert(argc == 2);

	error = NULL;
	values = NULL;
	if (!db_list_artist_tag_expr(argv[1], &values, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct db_generic_data *data = (struct db_generic_data *)walk->data;
		command_puts(client, "id: %d", data->id);
		command_puts(client, "Artist: %s", data->name);
		for (unsigned int i = 0; data->tags[i] != NULL; i++) {
			if (data->tags[i][0] != '\0')
				command_puts(client, "Tag: %s", data->tags[i]);
		}
		db_generic_data_free(data);
	}
	g_slist_free(values);

	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_listtags_genre(struct client *client, int argc, char **argv)
{
	GError *error;
	GSList *values, *walk;

	g_assert(argc == 2);

	error = NULL;
	values = NULL;
	if (!db_list_genre_tag_expr(argv[1], &values, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct db_generic_data *data = (struct db_generic_data *)walk->data;
		command_puts(client, "id: %d", data->id);
		command_puts(client, "Genre: %s", data->name);
		for (unsigned int i = 0; data->tags[i] != NULL; i++) {
			if (data->tags[i][0] != '\0')
				command_puts(client, "Tag: %s", data->tags[i]);
		}
		db_generic_data_free(data);
	}
	g_slist_free(values);

	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_count(struct client *client, int argc, char **argv)
{
	int changes;
	long count;
	char *endptr;
	GError *error;

	g_assert(argc == 3);

	/* Convert second argument to number */
	errno = 0;
	endptr = NULL;
	count = strtol(argv[2], &endptr, 10);
	if (errno != 0) {
		command_error(client, ACK_ERROR_ARG,
				"Failed to convert to number: %s",
				g_strerror(errno));
		return COMMAND_RETURN_ERROR;
	}
	else if (endptr == argv[2]) {
		command_error(client, ACK_ERROR_ARG, "No digits found");
		return COMMAND_RETURN_ERROR;
	}
	else if (count > INT_MAX || count < INT_MIN) {
		command_error(client, ACK_ERROR_ARG,
				"Number too %s",
				(count > INT_MAX) ? "big" : "small");
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	if (!db_count_song_expr(argv[1], (int)count, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_count_artist(struct client *client, int argc, char **argv)
{
	int changes;
	long count;
	char *endptr;
	GError *error;

	g_assert(argc == 3);

	/* Convert second argument to number */
	errno = 0;
	endptr = NULL;
	count = strtol(argv[2], &endptr, 10);
	if (errno != 0) {
		command_error(client, ACK_ERROR_ARG,
				"Failed to convert to number: %s",
				g_strerror(errno));
		return COMMAND_RETURN_ERROR;
	}
	else if (endptr == argv[2]) {
		command_error(client, ACK_ERROR_ARG, "No digits found");
		return COMMAND_RETURN_ERROR;
	}
	else if (count > INT_MAX || count < INT_MIN) {
		command_error(client, ACK_ERROR_ARG,
				"Number too %s",
				(count > INT_MAX) ? "big" : "small");
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	if (!db_count_artist_expr(argv[1], (int)count, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_count_album(struct client *client, int argc, char **argv)
{
	int changes;
	long count;
	char *endptr;
	GError *error;

	g_assert(argc == 3);

	/* Convert second argument to number */
	errno = 0;
	endptr = NULL;
	count = strtol(argv[2], &endptr, 10);
	if (errno != 0) {
		command_error(client, ACK_ERROR_ARG,
				"Failed to convert to number: %s",
				g_strerror(errno));
		return COMMAND_RETURN_ERROR;
	}
	else if (endptr == argv[2]) {
		command_error(client, ACK_ERROR_ARG, "No digits found");
		return COMMAND_RETURN_ERROR;
	}
	else if (count > INT_MAX || count < INT_MIN) {
		command_error(client, ACK_ERROR_ARG,
				"Number too %s",
				(count > INT_MAX) ? "big" : "small");
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	if (!db_count_album_expr(argv[1], (int)count, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_count_genre(struct client *client, int argc, char **argv)
{
	int changes;
	long count;
	char *endptr;
	GError *error;

	g_assert(argc == 3);

	/* Convert second argument to number */
	errno = 0;
	endptr = NULL;
	count = strtol(argv[2], &endptr, 10);
	if (errno != 0) {
		command_error(client, ACK_ERROR_ARG,
				"Failed to convert to number: %s",
				g_strerror(errno));
		return COMMAND_RETURN_ERROR;
	}
	else if (endptr == argv[2]) {
		command_error(client, ACK_ERROR_ARG, "No digits found");
		return COMMAND_RETURN_ERROR;
	}
	else if (count > INT_MAX || count < INT_MIN) {
		command_error(client, ACK_ERROR_ARG,
				"Number too %s",
				(count > INT_MAX) ? "big" : "small");
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	if (!db_count_genre_expr(argv[1], (int)count, &changes, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}
	command_puts(client, "changes: %d", changes);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return
handle_password(struct client *client, G_GNUC_UNUSED int argc, char **argv)
{
	gpointer perm = g_hash_table_lookup(globalconf.passwords, argv[1]);
	if (perm != NULL) {
		client->perm |= GPOINTER_TO_INT(perm);
		command_ok(client);
		return COMMAND_RETURN_OK;
	}
	command_error(client, ACK_ERROR_PASSWORD, "Invalid password");
	return COMMAND_RETURN_ERROR;
}

/* This has to be sorted! */
static const struct command commands[] = {
	{ "addtag", PERMISSION_UPDATE, 2, 2, handle_addtag },
	{ "addtag_album", PERMISSION_UPDATE, 2, 2, handle_addtag_album },
	{ "addtag_artist", PERMISSION_UPDATE, 2, 2, handle_addtag_artist },
	{ "addtag_genre", PERMISSION_UPDATE, 2, 2, handle_addtag_genre },

	{ "count", PERMISSION_UPDATE, 2, 2, handle_count },
	{ "count_album", PERMISSION_UPDATE, 2, 2, handle_count_album },
	{ "count_artist", PERMISSION_UPDATE, 2, 2, handle_count_artist },
	{ "count_genre", PERMISSION_UPDATE, 2, 2, handle_count_genre },

	{ "hate", PERMISSION_UPDATE, 1, 1, handle_love },
	{ "hate_album", PERMISSION_UPDATE, 1, 1, handle_love_album },
	{ "hate_artist", PERMISSION_UPDATE, 1, 1, handle_love_artist },
	{ "hate_genre", PERMISSION_UPDATE, 1, 1, handle_love_genre },

	{ "kill", PERMISSION_UPDATE, 1, 1, handle_kill },
	{ "kill_album", PERMISSION_UPDATE, 1, 1, handle_kill_album },
	{ "kill_artist", PERMISSION_UPDATE, 1, 1, handle_kill_artist },
	{ "kill_genre", PERMISSION_UPDATE, 1, 1, handle_kill_genre },

	{ "list", PERMISSION_SELECT, 1, 1, handle_list },
	{ "list_album", PERMISSION_SELECT, 1, 1, handle_list_album },
	{ "list_artist", PERMISSION_SELECT, 1, 1, handle_list_artist },
	{ "list_genre", PERMISSION_SELECT, 1, 1, handle_list_genre },

	{ "listinfo", PERMISSION_SELECT, 1, 1, handle_listinfo },
	{ "listinfo_album", PERMISSION_SELECT, 1, 1, handle_listinfo_album },
	{ "listinfo_artist", PERMISSION_SELECT, 1, 1, handle_listinfo_artist },
	{ "listinfo_genre", PERMISSION_SELECT, 1, 1, handle_listinfo_genre },

	{ "listtags", PERMISSION_SELECT, 1, 1, handle_listtags },
	{ "listtags_album", PERMISSION_SELECT, 1, 1, handle_listtags_album },
	{ "listtags_artist", PERMISSION_SELECT, 1, 1, handle_listtags_artist },
	{ "listtags_genre", PERMISSION_SELECT, 1, 1, handle_listtags_genre },

	{ "love", PERMISSION_UPDATE, 1, 1, handle_love },
	{ "love_album", PERMISSION_UPDATE, 1, 1, handle_love_album },
	{ "love_artist", PERMISSION_UPDATE, 1, 1, handle_love_artist },
	{ "love_genre", PERMISSION_UPDATE, 1, 1, handle_love_genre },

	{ "password", PERMISSION_NONE, 1, 1, handle_password },

	{ "rate", PERMISSION_UPDATE, 2, 2, handle_rate },
	{ "rate_album", PERMISSION_UPDATE, 2, 2, handle_rate_album },
	{ "rate_artist", PERMISSION_UPDATE, 2, 2, handle_rate_artist },
	{ "rate_genre", PERMISSION_UPDATE, 2, 2, handle_rate_genre },

	{ "rate_absolute", PERMISSION_UPDATE, 2, 2, handle_rate_absolute },
	{ "rate_absolute_album", PERMISSION_UPDATE, 2, 2, handle_rate_absolute_album },
	{ "rate_absolute_artist", PERMISSION_UPDATE, 2, 2, handle_rate_absolute_artist },
	{ "rate_absolute_genre", PERMISSION_UPDATE, 2, 2, handle_rate_absolute_genre },

	{ "rmtag", PERMISSION_UPDATE, 2, 2, handle_rmtag },
	{ "rmtag_album", PERMISSION_UPDATE, 2, 2, handle_rmtag_album },
	{ "rmtag_artist", PERMISSION_UPDATE, 2, 2, handle_rmtag_artist },
	{ "rmtag_genre", PERMISSION_UPDATE, 2, 2, handle_rmtag_genre },

	{ "unkill", PERMISSION_UPDATE, 1, 1, handle_kill },
	{ "unkill_album", PERMISSION_UPDATE, 1, 1, handle_kill_album },
	{ "unkill_artist", PERMISSION_UPDATE, 1, 1, handle_kill_artist },
	{ "unkill_genre", PERMISSION_UPDATE, 1, 1, handle_kill_genre },
};

static const unsigned num_commands = sizeof(commands) / sizeof(commands[0]);

static const struct command *
command_lookup(const char *name)
{
	unsigned a = 0, b = num_commands, i;
	int cmp;

	/* binary search */
	do {
		i = (a + b) / 2;

		cmp = strcmp(name, commands[i].cmd);
		if (cmp == 0)
			return &commands[i];
		else if (cmp < 0)
			b = i;
		else if (cmp > 0)
			a = i + 1;
	} while (a < b);

	return NULL;
}

static bool
command_check_request(const struct command *cmd, struct client *client,
		unsigned permission, int argc, char **argv)
{
	int min = cmd->min + 1;
	int max = cmd->max + 1;

	if (cmd->permission != (permission & cmd->permission)) {
		if (client != NULL)
			command_error(client, ACK_ERROR_PERMISSION,
					"you don't have permission for \"%s\"",
					cmd->cmd);
		return false;
	}

	if (min == 0)
		return true;

	if (min == max && max != argc) {
		if (client != NULL)
			command_error(client, ACK_ERROR_ARG,
					"wrong number of arguments for \"%s\"",
					argv[0]);
		return false;
	}
	else if (argc < min) {
		if (client != NULL)
			command_error(client, ACK_ERROR_ARG,
					"too few arguments for \"%s\"", argv[0]);
		return false;
	}
	else if (argc > max && max /* != 0 */ ) {
		if (client != NULL)
			command_error(client, ACK_ERROR_ARG,
					"too many arguments for \"%s\"", argv[0]);
		return false;
	}
	else
		return true;
}


static const struct command *
command_checked_lookup(struct client *client, unsigned permission, int argc,
		char **argv)
{
	static char unknown[] = "";
	const struct command *cmd;

	current_command = unknown;

	if (argc == 0)
		return NULL;

	cmd = command_lookup(argv[0]);
	if (cmd == NULL) {
		if (client != NULL)
			command_error(client, ACK_ERROR_UNKNOWN,
					"unknown command \"%s\"", argv[0]);
		return NULL;
	}

	current_command = cmd->cmd;

	if (!command_check_request(cmd, client, permission, argc, argv))
		return NULL;

	return cmd;
}

enum command_return
command_process(struct client *client, char *line)
{
	int argc;
	char *argv[COMMAND_ARGV_MAX] = { NULL };
	enum command_return ret = COMMAND_RETURN_ERROR;
	const struct command *cmd;
	GError *error = NULL;

	argv[0] = tokenizer_next_word(&line, &error);
	if (argv[0] == NULL) {
		current_command = "";
		if (line[0] == '\0')
			command_error(client, ACK_ERROR_UNKNOWN,
					"No command given");
		else {
			command_error(client, ACK_ERROR_UNKNOWN,
					"%s", error->message);
			g_error_free(error);
		}
		current_command = NULL;

		return COMMAND_RETURN_ERROR;
	}

	argc = 1;

	/* now parse the arguments (quoted or unquoted) */
	while (argc < (int)G_N_ELEMENTS(argv) &&
		(argv[argc] = tokenizer_next_param(&line, &error)) != NULL)
		++argc;

	/* Some error checks; we have to set current_command because
	 * command_error() expects it to be set.
	 */
	current_command = argv[0];

	if (argc >= (int)G_N_ELEMENTS(argv)) {
		command_error(client, ACK_ERROR_ARG, "Too many arguments");
		current_command = NULL;
		return COMMAND_RETURN_ERROR;
	}

	if (*line != 0) {
		command_error(client, ACK_ERROR_ARG, "%s", error->message);
		current_command = NULL;
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	/* Remove the previous authorizer */
	if (!db_set_authorizer(NULL, NULL, &error) ||
			!db_set_authorizer(command_authorizer, client, &error)) {
		command_error(client, error->code, "%s", error->message);
		current_command = NULL;
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	/* Look up and invoke the command handler. */
	cmd = command_checked_lookup(client, client->perm, argc, argv);
	if (cmd)
		ret = cmd->handler(client, argc, argv);

	/* Disable the authorizer again */
	if (!db_set_authorizer(NULL, NULL, &error)) {
		command_error(client, error->code, "%s", error->message);
		current_command = NULL;
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	current_command = NULL;
	return ret;
}
