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
	int generic;
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

static GSList *modules_generic = NULL;
static GSList *modules_database = NULL;
static GSList *modules_stored_playlist = NULL;
static GSList *modules_queue = NULL;
static GSList *modules_player = NULL;
static GSList *modules_mixer = NULL;
static GSList *modules_output = NULL;
static GSList *modules_options = NULL;
static GSList *modules_update = NULL;

static char *module_path(const char *modname, int *user_r)
{
	char *name, *path;

	daemon_log(LOG_DEBUG, "Trying to load module: %s", modname);
	name = g_strdup_printf("%s.%s", modname, G_MODULE_SUFFIX);
	daemon_log(LOG_DEBUG, "Added module suffix %s -> %s", modname, name);

	/* First check user path */
	path = g_build_filename(conf.mod_path, name, NULL);
	daemon_log(LOG_DEBUG, "Trying user configured path `%s'", path);
	if (g_file_test(path, G_FILE_TEST_EXISTS)) {
		daemon_log(LOG_DEBUG, "Found %s -> `%s'", modname, path);
		g_free(name);
		*user_r = 1;
		return path;
	}
	g_free(path);

	/* Next check system path */
	path = g_build_filename(LIBDIR, PACKAGE"-"VERSION, DOT_MODULES, name, NULL);
	daemon_log(LOG_DEBUG, "Trying system path `%s'", path);
	if (g_file_test(path, G_FILE_TEST_EXISTS)) {
		daemon_log(LOG_DEBUG, "Found %s -> `%s'", modname, path);
		g_free(name);
		*user_r = 0;
		return path;
	}
	g_free(name);
	g_free(path);
	return NULL;
}

static int module_load_one(int generic, int event, const char *modname,
		GKeyFile *config_fd, GSList **list_r)
{
	struct mpdcron_module *mod;
	initfunc_t initfunc = NULL;

	mod = g_new0(struct mpdcron_module, 1);
	mod->generic = generic;
	mod->event = event;
	if ((mod->path = module_path(modname, &(mod->user))) == NULL) {
		daemon_log(LOG_WARNING, "Error loading module %s: file not found", modname);
		g_free(mod);
		return -1;
	}
	if ((mod->module = g_module_open(mod->path, G_MODULE_BIND_LAZY)) == NULL) {
		daemon_log(LOG_WARNING, "Error loading module `%s': %s", mod->path, g_module_error());
		g_free(mod->path);
		g_free(mod);
		return -1;
	}

	/* Check for mpdcron_init() function, execute it and if it returns
	 * non-zero skip loading the module.
	 */
	if (g_module_symbol(mod->module, MODULE_INIT_FUNC, (gpointer *)&initfunc) && initfunc != NULL) {
		if (initfunc(optnd, config_fd) == MPDCRON_INIT_RETVAL_FAILURE) {
			daemon_log(LOG_WARNING,
					"Skipped loading module `%s': "MODULE_INIT_FUNC"() returned %d",
					mod->path, MPDCRON_INIT_RETVAL_FAILURE);
			g_free(mod->path);
			g_module_close(mod->module);
			g_free(mod);
			return -1;
		}
	}
	daemon_log(LOG_DEBUG, "Loaded module `%s'", mod->path);
	*list_r = g_slist_prepend(*list_r, mod);
	return 0;
}

static void module_close_list(GSList **list_r)
{
	GSList *walk;
	struct mpdcron_module *mod;
	closefunc_t closefunc = NULL;

	for (walk = *list_r; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *) walk->data;
		/* Run the close function if there's any */
		if (g_module_symbol(mod->module, MODULE_CLOSE_FUNC, (gpointer *)&closefunc) && closefunc != NULL)
			closefunc();
		g_free(mod->path);
		g_module_close(mod->module);
		g_free(mod);
	}
	g_slist_free(*list_r);
	*list_r = NULL;
}

static int module_process_ret(int ret, struct mpdcron_module *mod, GSList **slink_r, GSList **slist_r)
{
	closefunc_t closefunc = NULL;

	switch (ret) {
		case MPDCRON_RUN_RETVAL_SUCCESS:
			return 0;
		case MPDCRON_RUN_RETVAL_RECONNECT:
			daemon_log(LOG_INFO, "%s module `%s' scheduled reconnect (event: %s)",
					mod->user ? "User" : "Standard",
					mod->path,
					mod->generic ? "generic" : mpd_idle_name(mod->event));
			return -1;
		case MPDCRON_RUN_RETVAL_RECONNECT_NOW:
			daemon_log(LOG_INFO, "%s module `%s' scheduled reconnect NOW! (event: %s)",
					mod->user ? "User" : "Standard",
					mod->path,
					mod->generic ? "generic" : mpd_idle_name(mod->event));
			return -1;
		case MPDCRON_RUN_RETVAL_UNLOAD:
			daemon_log(LOG_INFO, "Unloading %s module `%s' (event: %s)",
					mod->user ? "user" : "standard",
					mod->path,
					mod->generic ? "generic" : mpd_idle_name(mod->event));
			/* Run the close function if there's any */
			if (g_module_symbol(mod->module, MODULE_CLOSE_FUNC,
						(gpointer *)&closefunc) && closefunc != NULL)
				closefunc();
			*slist_r = g_slist_remove_link(*slist_r, *slink_r);
			g_free(mod->path);
			g_module_close(mod->module);
			g_free(mod);
			g_slist_free(*slink_r);
			*slink_r = *slist_r;
			return 0;
		default:
			daemon_log(LOG_WARNING, "Unknown return from %s module `%s' (event: %s): %d",
					mod->user ? "user" : "standard",
					mod->path,
					mod->generic ? "generic" : mpd_idle_name(mod->event),
					ret);
			return 0;
	}
}

int module_load(int event, const char *modname, GKeyFile *config_fd)
{
	int generic;
	GSList **list_r = NULL;

	generic = 0;
	switch (event) {
		case MPD_IDLE_DATABASE:
			list_r = &modules_database;
			break;
		case MPD_IDLE_STORED_PLAYLIST:
			list_r = &modules_stored_playlist;
			break;
		case MPD_IDLE_QUEUE:
			list_r = &modules_queue;
			break;
		case MPD_IDLE_PLAYER:
			list_r = &modules_player;
			break;
		case MPD_IDLE_MIXER:
			list_r = &modules_mixer;
			break;
		case MPD_IDLE_OUTPUT:
			list_r = &modules_output;
			break;
		case MPD_IDLE_OPTIONS:
			list_r = &modules_options;
			break;
		case MPD_IDLE_UPDATE:
			list_r = &modules_update;
			break;
		default:
			generic = 1;
			list_r = &modules_generic;
			break;
	}

	return module_load_one(generic, event, modname, config_fd, list_r);
}

void module_close(void)
{
	module_close_list(&modules_generic);
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
	database_func_t func = NULL;

	ret = 0;
	for (walk = modules_database; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn, stats);
			ret = module_process_ret(mret, mod, &walk, &modules_database);
			if (ret < 0 && mret == MPDCRON_RUN_RETVAL_RECONNECT_NOW)
				break;
		}
	}
	return ret;
}

int module_stored_playlist_run(const struct mpd_connection *conn)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	stored_playlist_func_t func = NULL;

	ret = 0;
	for (walk = modules_stored_playlist; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn);
			ret = module_process_ret(mret, mod, &walk, &modules_database);
			if (ret < 0 && mret == MPDCRON_RUN_RETVAL_RECONNECT_NOW)
				break;
		}
	}
	return ret;
}

int module_queue_run(const struct mpd_connection *conn)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	queue_func_t func = NULL;

	ret = 0;
	for (walk = modules_queue; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn);
			ret = module_process_ret(mret, mod, &walk, &modules_database);
			if (ret < 0 && mret == MPDCRON_RUN_RETVAL_RECONNECT_NOW)
				break;
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
	player_func_t func = NULL;

	ret = 0;
	for (walk = modules_player; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn, song, status);
			ret = module_process_ret(mret, mod, &walk, &modules_database);
			if (ret < 0 && mret == MPDCRON_RUN_RETVAL_RECONNECT_NOW)
				break;
		}
	}
	return ret;
}

int module_mixer_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	mixer_func_t func = NULL;

	ret = 0;
	for (walk = modules_mixer; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn, status);
			ret = module_process_ret(mret, mod, &walk, &modules_database);
			if (ret < 0 && mret == MPDCRON_RUN_RETVAL_RECONNECT_NOW)
				break;
		}
	}
	return ret;
}

int module_output_run(const struct mpd_connection *conn)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	output_func_t func = NULL;

	ret = 0;
	for (walk = modules_output; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn);
			ret = module_process_ret(mret, mod, &walk, &modules_database);
			if (ret < 0 && mret == MPDCRON_RUN_RETVAL_RECONNECT_NOW)
				break;
		}
	}
	return ret;
}

int module_options_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	options_func_t func = NULL;

	ret = 0;
	for (walk = modules_options; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn, status);
			ret = module_process_ret(mret, mod, &walk, &modules_database);
			if (ret < 0 && mret == MPDCRON_RUN_RETVAL_RECONNECT_NOW)
				break;
		}
	}
	return ret;
}

int module_update_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	int mret, ret;
	GSList *walk;
	struct mpdcron_module *mod;
	update_func_t func = NULL;

	ret = 0;
	for (walk = modules_update; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct mpdcron_module *)walk->data;
		if (g_module_symbol(mod->module, MODULE_RUN_FUNC, (gpointer *)&func) && func != NULL) {
			mret = func(conn, status);
			ret = module_process_ret(mret, mod, &walk, &modules_database);
			if (ret < 0 && mret == MPDCRON_RUN_RETVAL_RECONNECT_NOW)
				break;
		}
	}
	return ret;
}
