/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

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

#ifndef MPDCRON_GUARD_STATS_DEFS_H
#define MPDCRON_GUARD_STATS_DEFS_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef MPDCRON_MODULE
#define MPDCRON_MODULE		"stats"
#include "../gmodule.h"
#endif /* !MPDCRON_MODULE */

#include <stdbool.h>

#include <glib.h>
#include <gio/gio.h>
#include <mpd/client.h>
#include <sqlite3.h>

#include "stats-sqlite.h"

#define PROTOCOL_VERSION "0.1"
#define DEFAULT_HOST "any"
#define DEFAULT_PORT 6601

#define PERMISSION_NONE    0
#define PERMISSION_SELECT  1
#define PERMISSION_UPDATE  2
#define PERMISSION_ALL     (PERMISSION_SELECT | PERMISSION_UPDATE)

/* Check command_process() function if you want to change this!
 */
#define COMMAND_ARGV_MAX 16

/* TODO: Make this configurable */
#define CLIENT_MAX 1024

struct client {
	int id;
	unsigned perm;
	GIOStream *stream;
	GDataInputStream *input;
	GOutputStream *output;
};

enum ack {
	ACK_ERROR_ARG = 1,
	ACK_ERROR_PASSWORD = 2,
	ACK_ERROR_PERMISSION = 3,
	ACK_ERROR_UNKNOWN = 4,
};

enum command_return {
	COMMAND_RETURN_ERROR = -1,
	COMMAND_RETURN_OK = 0,
	COMMAND_RETURN_KILL = 10,
	COMMAND_RETURN_CLOSE = 20,
};

/**
 * Configuration
 */
struct config {
	char **addrs;
	int port;
	char *dbpath;
	int default_permissions;
	GHashTable *passwords;
	char *mpd_hostname;
	char *mpd_port;
	char *mpd_password;
};

extern struct config globalconf;

bool file_load(const struct mpdcron_config *conf, GKeyFile *fd);
void file_cleanup(void);

/**
 * Remote query interface
 */
void server_init(void);
void server_bind(const char *hostname, int port);
void server_start(void);
void server_close(void);
void server_schedule_write(struct client *client, const gchar *data, gsize count);
void server_flush_write(struct client *client);

/**
 * Commands
 */
enum command_return command_process(struct client *client, char *line);

#endif /* !MPDCRON_GUARD_STATS_DEFS_H */
