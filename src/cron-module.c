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

#include <string.h>

#include <glib.h>
#include <gmodule.h>
#include <mpd/client.h>

struct mpdcron_module {
	int event;
	int user;
	char *path;
	GModule *module;
};

typedef int (*initfunc_t) (int, GKeyFile *);
typedef void (*closefunc_t) (void);

typedef int (*database_func_t) (const struct mpd_connection *conn, const struct mpd_stats *stats);
typedef int (*stored_playlist_func_t) (const struct mpd_connection *conn);
typedef int (*queue_func_t) (const struct mpd_connection *conn);
typedef int (*player_func_t) (const struct mpd_connection *conn, const struct mpd_song *song,
		const struct mpd_status *status);
typedef int (*mixer_func_t) (const struct mpd_connection *conn, const struct mpd_status *status);
typedef int (*output_func_t) (const struct mpd_connection *conn);
typedef int (*options_func_t) (const struct mpd_connection *conn, const struct mpd_status *status);
typedef int (*update_func_t) (const struct mpd_connection *conn, const struct mpd_status *status);

GSList *modules_database = NULL;
GSList *modules_stored_playlist = NULL;
GSList *modules_queue = NULL;
GSList *modules_player = NULL;
GSList *modules_mixer = NULL;
GSList *modules_output = NULL;
GSList *modules_options = NULL;
GSList *modules_update = NULL;

static char *module_path(const char *modname, int *user)
{
	char *name, *path;

	crlogv(LOG_DEBUG, "Trying to load module: %s", modname);
	name = g_strdup_printf("%s.%s", modname, G_MODULE_SUFFIX);
	crlogv(LOG_DEBUG, "Added module suffix %s -> %s", modname, name);

	/* First check user path */
	path = g_build_filename(mod_path, name, NULL);
	crlogv(LOG_DEBUG, "Trying user configured path `%s'", path);
	if (g_file_test(path, G_FILE_TEST_EXISTS)) {
		crlogv(LOG_DEBUG, "Found %s -> `%s'", modname, path);
		g_free(name);
		*user = 1;
		return path;
	}
	g_free(path);

	/* Next check system path */
	path = g_build_filename(LIBDIR, PACKAGE"-"VERSION, DOT_MODULES, name, NULL);
	crlogv(LOG_DEBUG, "Trying system path `%s'", path);
	if (g_file_test(path, G_FILE_TEST_EXISTS)) {
		crlogv(LOG_DEBUG, "Found %s -> `%s'", modname, path);
		g_free(name);
		*user = 0;
		return path;
	}
	g_free(name);
	g_free(path);
	return NULL;
}

static int module_load_one(int event, const char *modname, GKeyFile *config_fd, GSList **list_ptr)
{
	struct mpdcron_module *mod;
	initfunc_t initfunc = NULL;

	mod = g_new0(struct mpdcron_module, 1);
	mod->event = event;
	if ((mod->path = module_path(modname, &(mod->user))) == NULL) {
		crlog(LOG_WARNING, "Error loading module %s: file not found", modname);
		g_free(mod);
		return -1;
	}
	if ((mod->module = g_module_open(mod->path, G_MODULE_BIND_LAZY)) == NULL) {
		crlog(LOG_WARNING, "Error loading module `%s': %s", mod->path, g_module_error());
		g_free(mod->path);
		g_free(mod);
		return -1;
	}

	/* Check for mpdcron_init() function, execute it and if it returns
	 * non-zero skip loading the module.
	 */
	if (g_module_symbol(mod->module, MODULE_INIT_FUNC, (gpointer *)&initfunc) && initfunc != NULL) {
		if (initfunc(optnd, config_fd) != 0) {
			crlog(LOG_WARNING,
					"Skipped loading module `%s': "MODULE_INIT_FUNC"() returned non-zero",
					mod->path);
			g_free(mod->path);
			g_module_close(mod->module);
			g_free(mod);
			return -1;
		}
	}
	crlogv(LOG_DEBUG, "Loaded module `%s'", mod->path);
	*list_ptr = g_slist_prepend(*list_ptr, mod);
	return 0;
}

static void module_close_list(GSList **list)
{
	GSList *walk;
	struct mpdcron_module *mod;
	closefunc_t closefunc = NULL;

	for (walk = *list; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *) walk->data;
		/* Run the close function if there's any */
		if (g_module_symbol(mod->module, MODULE_CLOSE_FUNC, (gpointer *)&closefunc) && closefunc != NULL)
			closefunc();
		g_free(mod->path);
		g_module_close(mod->module);
		g_free(mod);
	}
	g_slist_free(*list);
	*list = NULL;
}

int module_load(int event, const char *modname, GKeyFile *config_fd)
{
	GSList **list_ptr = NULL;

	switch (event) {
		case MPD_IDLE_DATABASE:
			list_ptr = &modules_database;
			break;
		case MPD_IDLE_STORED_PLAYLIST:
			list_ptr = &modules_stored_playlist;
			break;
		case MPD_IDLE_QUEUE:
			list_ptr = &modules_queue;
			break;
		case MPD_IDLE_PLAYER:
			list_ptr = &modules_player;
			break;
		case MPD_IDLE_MIXER:
			list_ptr = &modules_mixer;
			break;
		case MPD_IDLE_OUTPUT:
			list_ptr = &modules_output;
			break;
		case MPD_IDLE_OPTIONS:
			list_ptr = &modules_options;
			break;
		case MPD_IDLE_UPDATE:
			list_ptr = &modules_update;
			break;
		default:
			g_assert_not_reached();
			break;
	}

	return module_load_one(event, modname, config_fd, list_ptr);
}

void module_close(void)
{
	module_close_list(&modules_database);
	module_close_list(&modules_stored_playlist);
	module_close_list(&modules_queue);
	module_close_list(&modules_player);
	module_close_list(&modules_mixer);
	module_close_list(&modules_output);
	module_close_list(&modules_options);
	module_close_list(&modules_update);
}

int module_database_run(const struct mpd_connection *conn, const struct mpd_stats *stats)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	closefunc_t closefunc = NULL;
	database_func_t func = NULL;

	ret = 0;
	for (walk = modules_database; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn, stats);
			if (mret == MODULE_RETVAL_SUCCESS)
				continue;
			else if (mret == MODULE_RETVAL_RECONNECT)
				ret = -1;
			else if (mret == MODULE_RETVAL_RECONNECT_NOW)
				return -1;
			else if (mret == MODULE_RETVAL_UNLOAD) {
				crlog(LOG_INFO, "Unloading %s module `%s' (event: %s): module returned %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event),
						MODULE_RETVAL_UNLOAD);
				/* Run the close function if there's any */
				if (g_module_symbol(mod->module, MODULE_CLOSE_FUNC,
							(gpointer *)&closefunc) && closefunc != NULL)
					closefunc();
				modules_database = g_slist_remove_link(modules_database, walk);
				g_free(mod->path);
				g_module_close(mod->module);
				g_free(mod);
				g_slist_free(walk);
				continue;
			}
			else
				crlog(LOG_WARNING, "Unknown return from %s module `%s' (event: %s): %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event), mret);
		}
	}
	return ret;
}

int module_stored_playlist_run(const struct mpd_connection *conn)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	closefunc_t closefunc = NULL;
	stored_playlist_func_t func = NULL;

	ret = 0;
	for (walk = modules_stored_playlist; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn);
			if (mret == MODULE_RETVAL_SUCCESS)
				continue;
			else if (mret == MODULE_RETVAL_RECONNECT)
				ret = -1;
			else if (mret == MODULE_RETVAL_RECONNECT_NOW)
				return -1;
			else if (mret == MODULE_RETVAL_UNLOAD) {
				crlog(LOG_INFO, "Unloading %s module `%s' (event: %s): module returned %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event),
						MODULE_RETVAL_UNLOAD);
				/* Run the close function if there's any */
				if (g_module_symbol(mod->module, MODULE_CLOSE_FUNC,
							(gpointer *)&closefunc) && closefunc != NULL)
					closefunc();
				modules_stored_playlist = g_slist_remove_link(modules_stored_playlist, walk);
				g_free(mod->path);
				g_module_close(mod->module);
				g_free(mod);
				g_slist_free(walk);
				continue;
			}
			else
				crlog(LOG_WARNING, "Unknown return from %s module `%s' (event: %s): %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event), mret);
		}
	}
	return ret;
}

int module_queue_run(const struct mpd_connection *conn)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	closefunc_t closefunc = NULL;
	queue_func_t func = NULL;

	ret = 0;
	for (walk = modules_queue; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn);
			if (mret == MODULE_RETVAL_SUCCESS)
				continue;
			else if (mret == MODULE_RETVAL_RECONNECT)
				ret = -1;
			else if (mret == MODULE_RETVAL_RECONNECT_NOW)
				return -1;
			else if (mret == MODULE_RETVAL_UNLOAD) {
				crlog(LOG_INFO, "Unloading %s module `%s' (event: %s): module returned %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event),
						MODULE_RETVAL_UNLOAD);
				/* Run the close function if there's any */
				if (g_module_symbol(mod->module, MODULE_CLOSE_FUNC,
							(gpointer *)&closefunc) && closefunc != NULL)
					closefunc();
				modules_queue = g_slist_remove_link(modules_queue, walk);
				g_free(mod->path);
				g_module_close(mod->module);
				g_free(mod);
				g_slist_free(walk);
				continue;
			}
			else
				crlog(LOG_WARNING, "Unknown return from %s module `%s' (event: %s): %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event), mret);
		}
	}
	return ret;
}

extern int module_player_run(const struct mpd_connection *conn, const struct mpd_song *song,
		const struct mpd_status *status)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	closefunc_t closefunc = NULL;
	player_func_t func = NULL;

	ret = 0;
	for (walk = modules_player; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn, song, status);
			if (mret == MODULE_RETVAL_SUCCESS)
				continue;
			else if (mret == MODULE_RETVAL_RECONNECT)
				ret = -1;
			else if (mret == MODULE_RETVAL_RECONNECT_NOW)
				return -1;
			else if (mret == MODULE_RETVAL_UNLOAD) {
				crlog(LOG_INFO, "Unloading %s module `%s' (event: %s): module returned %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event),
						MODULE_RETVAL_UNLOAD);
				/* Run the close function if there's any */
				if (g_module_symbol(mod->module, MODULE_CLOSE_FUNC,
							(gpointer *)&closefunc) && closefunc != NULL)
					closefunc();
				modules_player = g_slist_remove_link(modules_player, walk);
				g_free(mod->path);
				g_module_close(mod->module);
				g_free(mod);
				g_slist_free(walk);
				continue;
			}
			else
				crlog(LOG_WARNING, "Unknown return from %s module `%s' (event: %s): %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event), mret);
		}
	}
	return ret;
}

int module_mixer_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	closefunc_t closefunc = NULL;
	mixer_func_t func = NULL;

	ret = 0;
	for (walk = modules_mixer; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn, status);
			if (mret == MODULE_RETVAL_SUCCESS)
				continue;
			else if (mret == MODULE_RETVAL_RECONNECT)
				ret = -1;
			else if (mret == MODULE_RETVAL_RECONNECT_NOW)
				return -1;
			else if (mret == MODULE_RETVAL_UNLOAD) {
				crlog(LOG_INFO, "Unloading %s module `%s' (event: %s): module returned %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event),
						MODULE_RETVAL_UNLOAD);
				/* Run the close function if there's any */
				if (g_module_symbol(mod->module, MODULE_CLOSE_FUNC,
							(gpointer *)&closefunc) && closefunc != NULL)
					closefunc();
				modules_mixer = g_slist_remove_link(modules_mixer, walk);
				g_free(mod->path);
				g_module_close(mod->module);
				g_free(mod);
				g_slist_free(walk);
				continue;
			}
			else
				crlog(LOG_WARNING, "Unknown return from %s module `%s' (event: %s): %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event), mret);
		}
	}
	return ret;
}

int module_output_run(const struct mpd_connection *conn)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	closefunc_t closefunc = NULL;
	output_func_t func = NULL;

	ret = 0;
	for (walk = modules_output; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn);
			if (mret == MODULE_RETVAL_SUCCESS)
				continue;
			else if (mret == MODULE_RETVAL_RECONNECT)
				ret = -1;
			else if (mret == MODULE_RETVAL_RECONNECT_NOW)
				return -1;
			else if (mret == MODULE_RETVAL_UNLOAD) {
				crlog(LOG_INFO, "Unloading %s module `%s' (event: %s): module returned %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event),
						MODULE_RETVAL_UNLOAD);
				/* Run the close function if there's any */
				if (g_module_symbol(mod->module, MODULE_CLOSE_FUNC,
							(gpointer *)&closefunc) && closefunc != NULL)
					closefunc();
				modules_output = g_slist_remove_link(modules_output, walk);
				g_free(mod->path);
				g_module_close(mod->module);
				g_free(mod);
				g_slist_free(walk);
				continue;
			}
			else
				crlog(LOG_WARNING, "Unknown return from %s module `%s' (event: %s): %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event), mret);
		}
	}
	return ret;
}

int module_options_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	closefunc_t closefunc = NULL;
	options_func_t func = NULL;

	ret = 0;
	for (walk = modules_options; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn, status);
			if (mret == MODULE_RETVAL_SUCCESS)
				continue;
			else if (mret == MODULE_RETVAL_RECONNECT)
				ret = -1;
			else if (mret == MODULE_RETVAL_RECONNECT_NOW)
				return -1;
			else if (mret == MODULE_RETVAL_UNLOAD) {
				crlog(LOG_INFO, "Unloading %s module `%s' (event: %s): module returned %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event),
						MODULE_RETVAL_UNLOAD);
				/* Run the close function if there's any */
				if (g_module_symbol(mod->module, MODULE_CLOSE_FUNC,
							(gpointer *)&closefunc) && closefunc != NULL)
					closefunc();
				modules_options = g_slist_remove_link(modules_options, walk);
				g_free(mod->path);
				g_module_close(mod->module);
				g_free(mod);
				g_slist_free(walk);
				continue;
			}
			else
				crlog(LOG_WARNING, "Unknown return from %s module `%s' (event: %s): %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event), mret);
		}
	}
	return ret;
}

int module_update_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	closefunc_t closefunc = NULL;
	update_func_t func = NULL;

	ret = 0;
	for (walk = modules_update; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn, status);
			if (mret == MODULE_RETVAL_SUCCESS)
				continue;
			else if (mret == MODULE_RETVAL_RECONNECT)
				ret = -1;
			else if (mret == MODULE_RETVAL_RECONNECT_NOW)
				return -1;
			else if (mret == MODULE_RETVAL_UNLOAD) {
				crlog(LOG_INFO, "Unloading %s module `%s' (event: %s): module returned %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event),
						MODULE_RETVAL_UNLOAD);
				/* Run the close function if there's any */
				if (g_module_symbol(mod->module, MODULE_CLOSE_FUNC,
							(gpointer *)&closefunc) && closefunc != NULL)
					closefunc();
				modules_update = g_slist_remove_link(modules_update, walk);
				g_free(mod->path);
				g_module_close(mod->module);
				g_free(mod);
				g_slist_free(walk);
				continue;
			}
			else
				crlog(LOG_WARNING, "Unknown return from %s module `%s' (event: %s): %d",
						mod->user ? "user" : "standard",
						mod->path, mpd_idle_name(mod->event), mret);
		}
	}
	return ret;
}
