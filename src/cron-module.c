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

#include "cron-defs.h"

#include <glib.h>
#include <gmodule.h>
#include <mpd/client.h>

typedef void (*module_initfunc_t) (void);
typedef void (*module_closefunc_t) (void);

typedef int (*module_database_func_t) (const struct mpd_connection *conn, const struct mpd_stats *stats);
typedef int (*module_stored_playlist_func_t) (const struct mpd_connection *conn);
typedef int (*module_queue_func_t) (const struct mpd_connection *conn);
typedef int (*module_player_func_t) (const struct mpd_connection *conn, const struct mpd_song *song,
		const struct mpd_status *status);
typedef int (*module_mixer_func_t) (const struct mpd_connection *conn, const struct mpd_status *status);
typedef int (*module_output_func_t) (const struct mpd_connection *conn);
typedef int (*module_options_func_t) (const struct mpd_connection *conn, const struct mpd_status *status);
typedef int (*module_update_func_t) (const struct mpd_connection *conn, const struct mpd_status *status);

GModule *module_database = NULL;
GModule *module_stored_playlist = NULL;
GModule *module_queue = NULL;
GModule *module_player = NULL;
GModule *module_mixer = NULL;
GModule *module_output = NULL;
GModule *module_options = NULL;
GModule *module_update = NULL;

static void module_init_name(const char *name, const char *initsym, GModule **module)
{
	char *sname, *path;
	module_initfunc_t initfunc = NULL;

	sname = g_strdup_printf("%s.%s", name, G_MODULE_SUFFIX);
	path = g_build_filename(home_path, DOT_MODULES, sname, NULL);
	g_free(sname);
	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		g_free(path);
		return;
	}
	else if ((*module = g_module_open(path, G_MODULE_BIND_LAZY)) == NULL) {
		crlog(LOG_WARNING, "Error loading module `%s': %s", path, g_module_error());
		g_free(path);
		return;
	}
	crlog(LOG_DEBUG, "Loaded module `%s'", path);
	g_free(path);

	/* Run the init function if there's any */
	if (g_module_symbol(*module, initsym, (gpointer *)&initfunc) && initfunc != NULL)
		initfunc();
}

static void module_close_name(const char *closesym, GModule **module)
{
	module_closefunc_t closefunc = NULL;

	if (*module != NULL) {
		/* Run the close function if there's any */
		if (g_module_symbol(*module, closesym, (gpointer *)&closefunc) && closefunc != NULL)
			closefunc();
		g_module_close(*module);
		*module = NULL;
	}
}

void module_init(void)
{
	module_init_name("database", "database_init", &module_database);
	module_init_name("stored_playlist", "stored_playlist_init", &module_stored_playlist);
	module_init_name("queue", "queue_init", &module_queue);
	module_init_name("player", "player_init", &module_player);
	module_init_name("mixer", "mixer_init", &module_mixer);
	module_init_name("output", "output_init", &module_output);
	module_init_name("options", "options_init", &module_options);
	module_init_name("update", "update_init", &module_update);
}

void module_close(void)
{
	module_close_name("database_close", &module_database);
	module_close_name("stored_playlist_close", &module_stored_playlist);
	module_close_name("playlist_close", &module_queue);
	module_close_name("player_close", &module_player);
	module_close_name("mixer_close", &module_mixer);
	module_close_name("output_close", &module_output);
	module_close_name("update_close", &module_update);
}

int module_database_run(const struct mpd_connection *conn, const struct mpd_stats *stats)
{
	module_database_func_t func = NULL;

	if (module_database == NULL)
		return 0;

	if (g_module_symbol(module_database, "database_run", (gpointer *)&func) && func != NULL)
		return func(conn, stats);

	return 0;
}

int module_stored_playlist_run(const struct mpd_connection *conn)
{
	module_stored_playlist_func_t func = NULL;

	if (module_stored_playlist == NULL)
		return 0;

	if (g_module_symbol(module_stored_playlist, "stored_playlist_run", (gpointer *)&func) && func != NULL)
		return func(conn);

	return 0;
}

int module_queue_run(const struct mpd_connection *conn)
{
	module_queue_func_t func = NULL;

	if (module_queue == NULL)
		return 0;

	if (g_module_symbol(module_queue, "queue_run", (gpointer *)&func) && func != NULL)
		return func(conn);

	return 0;
}

int module_player_run(const struct mpd_connection *conn, const struct mpd_song *song, const struct mpd_status *status)
{
	module_player_func_t func = NULL;

	if (module_player == NULL)
		return 0;

	if (g_module_symbol(module_player, "player_run", (gpointer *)&func) && func != NULL)
		return func(conn, song, status);

	return 0;
}

int module_mixer_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	module_mixer_func_t func = NULL;

	if (module_mixer == NULL)
		return 0;

	if (g_module_symbol(module_mixer, "mixer_run", (gpointer *)&func) && func != NULL)
		return func(conn, status);

	return 0;
}

int module_output_run(const struct mpd_connection *conn)
{
	module_output_func_t func = NULL;

	if (module_output == NULL)
		return 0;

	if (g_module_symbol(module_output, "output_run", (gpointer *)&func) && func != NULL)
		return func(conn);

	return 0;
}

int module_options_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	module_options_func_t func = NULL;

	if (module_options == NULL)
		return 0;

	if (g_module_symbol(module_options, "options_run", (gpointer *)&func) && func != NULL)
		return func(conn, status);

	return 0;
}

int module_update_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	module_update_func_t func = NULL;

	if (module_update == NULL)
		return 0;

	if (g_module_symbol(module_update, "update_run", (gpointer *)&func) && func != NULL)
		return func(conn, status);

	return 0;
}
