/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet ai cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>
#include <sqlite3.h>

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

static void command_error_v(struct client *client, enum ack error,
		const char *fmt, va_list args)
{
	GString *message;

	g_assert(client != NULL);
	g_assert(current_command != NULL);

	message = g_string_new("");
	g_string_printf(message, PROCOTOL_ACK" [%i] {%s} ",
			(int)error, current_command);
	g_string_append_vprintf(message, fmt, args);

	mpdcron_log(LOG_DEBUG, "[%d]> %s", client->id, message->str);
	g_string_append_c(message, '\n');

	server_schedule_write(client, message->str, strlen(message->str) + 1);
	g_string_free(message, TRUE);
	current_command = NULL;
}

G_GNUC_PRINTF(3, 4)
static void command_error(struct client *client, enum ack error,
		const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	command_error_v(client, error, fmt, args);
	va_end(args);
}

static void command_ok(struct client *client)
{
	const char *ok = PROTOCOL_OK" \n";
	mpdcron_log(LOG_DEBUG, "[%d]> "PROTOCOL_OK, client->id);
	server_schedule_write(client, ok, strlen(ok) + 1);
}

static void command_putv(struct client *client, const char *fmt, va_list args)
{
	GString *message;

	g_assert(client != NULL);

	message = g_string_new("");
	g_string_append_vprintf(message, fmt, args);

	mpdcron_log(LOG_DEBUG, "[%d]> %s", client->id, message->str);
	g_string_append_c(message, '\n');

	server_schedule_write(client, message->str, strlen(message->str) + 1);
	g_string_free(message, TRUE);
}

G_GNUC_PRINTF(2, 3)
static void command_puts(struct client *client, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	command_putv(client, fmt, args);
	va_end(args);
}

G_GNUC_UNUSED
static bool check_bool(struct client *client, bool *value_r, const char *s)
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

static enum command_return handle_love(struct client *client,
		int argc, char **argv)
{
	int value;
	GError *error;
	sqlite3 *db;

	g_assert(argc == 2);

	error = NULL;
	db = db_init(globalconf.dbpath, &error);
	if (db == NULL) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	if (!db_love_song_uri(db, argv[1], true, &value, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		sqlite3_close(db);
		return COMMAND_RETURN_ERROR;
	}
	sqlite3_close(db);

	command_puts(client, "Love: %d", value);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return handle_love_expr(struct client *client,
		int argc, char **argv)
{
	GError *error;
	GSList *values, *walk;
	sqlite3 *db;

	g_assert(argc == 2);

	error = NULL;
	db = db_init(globalconf.dbpath, &error);
	if (db == NULL) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	error = NULL;
	values = NULL;
	if (!db_love_song_expr(db, argv[1], true, &values, &error)) {
		command_error(client, error->code, "%s", error->message);
		g_error_free(error);
		return COMMAND_RETURN_ERROR;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		char **message = (char **) walk->data;
		command_puts(client, "file: %s", message[0]);
		command_puts(client, "Love: %s", message[1]);
		g_free(message[0]);
		g_free(message[1]);
		g_free(message);
	}
	g_slist_free(values);
	command_ok(client);
	return COMMAND_RETURN_OK;
}

static enum command_return handle_password(struct client *client,
		G_GNUC_UNUSED int argc, char **argv)
{
	gpointer perm = g_hash_table_lookup(globalconf.passwords, argv[1]);
	if (perm != NULL) {
		client->perm = GPOINTER_TO_INT(perm);
		command_ok(client);
		return COMMAND_RETURN_OK;
	}
	command_error(client, ACK_ERROR_PASSWORD, "Invalid password");
	return COMMAND_RETURN_ERROR;
}

static const struct command commands[] = {
	{ "love", PERMISSION_UPDATE, 1, 1, handle_love },
	{ "love_expr", PERMISSION_UPDATE, 1, 1, handle_love_expr },
	{ "password", PERMISSION_NONE, 1, 1, handle_password },
};

static const unsigned num_commands = sizeof(commands) / sizeof(commands[0]);

static const struct command *command_lookup(const char *name)
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

static bool command_check_request(const struct command *cmd,
		struct client *client, unsigned permission,
		int argc, char **argv)
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


static const struct command *command_checked_lookup(struct client *client,
		unsigned permission, int argc, char **argv)
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

enum command_return command_process(struct client *client, char *line)
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

	/* Look up and invoke the command handler. */
	cmd = command_checked_lookup(client, client->perm, argc, argv);
	if (cmd)
		ret = cmd->handler(client, argc, argv);

	current_command = NULL;
	return ret;
}
