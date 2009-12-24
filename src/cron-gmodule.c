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

#include <stdbool.h>
#include <string.h>

#include <glib.h>
#include <gmodule.h>
#include <mpd/client.h>

struct module_data {
	int user;
	char *path;
	GModule *module;
	struct mpdcron_module *data;
};

static GSList *modules = NULL;

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

static int module_init_one(const char *modname, GKeyFile *config_fd)
{
	struct module_data *mod;

	mod = g_new0(struct module_data, 1);
	if ((mod->path = module_path(modname, &(mod->user))) == NULL) {
		daemon_log(LOG_WARNING, "Error loading module %s: file not found",
				modname);
		g_free(mod);
		return -1;
	}
	if ((mod->module = g_module_open(mod->path, G_MODULE_BIND_LOCAL)) == NULL) {
		daemon_log(LOG_WARNING, "Error loading module `%s': %s",
				mod->path, g_module_error());
		g_free(mod->path);
		g_free(mod);
		return -1;
	}
	if (!g_module_symbol(mod->module, "module", (gpointer *)&mod->data) || mod->data == NULL) {
		daemon_log(LOG_WARNING, "Error loading module `%s': no module structure",
				mod->path);
		g_module_close(mod->module);
		g_free(mod->path);
		g_free(mod);
		return -1;
	}

	/* Run the init() function if there's any. */
	if (mod->data->init != NULL) {
		if ((mod->data->init)(optnd, config_fd) == MPDCRON_INIT_FAILURE) {
			daemon_log(LOG_WARNING,
					"Skipped loading module `%s': init() returned %d",
					mod->path, MPDCRON_INIT_FAILURE);
			g_free(mod->path);
			g_module_close(mod->module);
			g_free(mod);
			return -1;
		}
	}
	daemon_log(LOG_DEBUG, "Loaded module `%s'", mod->path);
	modules = g_slist_prepend(modules, mod);
	return 0;
}

static void module_destroy_one(gpointer data, G_GNUC_UNUSED gpointer userdata)
{
	struct module_data *mod;

	mod = (struct module_data *)data;
	/* Run the destroy function if there's any */
	if (mod->data->destroy != NULL)
		(mod->data->destroy)();
	g_free(mod->path);
	g_module_close(mod->module);
	g_free(mod);
}

static int module_process_ret(int ret, struct module_data *mod, GSList **slink_r, GSList **slist_r)
{
	switch (ret) {
		case MPDCRON_EVENT_SUCCESS:
			return 0;
		case MPDCRON_EVENT_RECONNECT:
			daemon_log(LOG_INFO, "%s module `%s' scheduled reconnect",
					mod->user ? "User" : "Standard",
					mod->path);
			return -1;
		case MPDCRON_EVENT_RECONNECT_NOW:
			daemon_log(LOG_INFO, "%s module `%s' scheduled reconnect NOW!",
					mod->user ? "User" : "Standard",
					mod->path);
			return -1;
		case MPDCRON_EVENT_UNLOAD:
			daemon_log(LOG_INFO, "Unloading %s module `%s'",
					mod->user ? "user" : "standard",
					mod->path);
			*slist_r = g_slist_remove_link(*slist_r, *slink_r);
			module_destroy_one(mod, NULL);
			g_slist_free(*slink_r);
			*slink_r = *slist_r;
			return 0;
		default:
			daemon_log(LOG_WARNING, "Unknown return from %s module `%s': %d",
					mod->user ? "user" : "standard",
					mod->path,
					ret);
			return 0;
	}
}

int module_load(const char *modname, GKeyFile *config_fd)
{
	return module_init_one(modname, config_fd);
}

void module_close(void)
{
	g_slist_foreach(modules, module_destroy_one, NULL);
	g_slist_free(modules);
	modules = NULL;
}

int module_database_run(const struct mpd_connection *conn, const struct mpd_stats *stats)
{
	int mret, ret;
	GSList *walk;
	struct module_data *mod;

	ret = 0;
	for (walk = modules; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct module_data *)walk->data;
		if (mod->data->event_database == NULL)
			continue;
		mret = (mod->data->event_database)(conn,stats);
		ret = module_process_ret(mret, mod, &walk, &modules);
		if (ret < 0 && mret == MPDCRON_EVENT_RECONNECT_NOW)
			break;
	}
	return ret;
}

int module_stored_playlist_run(const struct mpd_connection *conn)
{
	int mret, ret;
	GSList *walk;
	struct module_data *mod;

	ret = 0;
	for (walk = modules; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct module_data *)walk->data;
		if (mod->data->event_stored_playlist == NULL)
			continue;
		mret = (mod->data->event_stored_playlist)(conn);
		ret = module_process_ret(mret, mod, &walk, &modules);
		if (ret < 0 && mret == MPDCRON_EVENT_RECONNECT_NOW)
			break;
	}
	return ret;
}

int module_queue_run(const struct mpd_connection *conn)
{
	int mret, ret;
	GSList *walk;
	struct module_data *mod;

	ret = 0;
	for (walk = modules; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct module_data *)walk->data;
		if (mod->data->event_queue == NULL)
			continue;
		mret = (mod->data->event_queue)(conn);
		ret = module_process_ret(mret, mod, &walk, &modules);
		if (ret < 0 && mret == MPDCRON_EVENT_RECONNECT_NOW)
			break;
	}
	return ret;
}

extern int module_player_run(const struct mpd_connection *conn, const struct mpd_song *song,
		const struct mpd_status *status)
{
	int mret, ret;
	GSList *walk;
	struct module_data *mod;

	ret = 0;
	for (walk = modules; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct module_data *)walk->data;
		if (mod->data->event_player == NULL)
			continue;
		mret = (mod->data->event_player)(conn, song, status);
		ret = module_process_ret(mret, mod, &walk, &modules);
		if (ret < 0 && mret == MPDCRON_EVENT_RECONNECT_NOW)
			break;
	}
	return ret;
}

int module_mixer_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	int mret, ret;
	GSList *walk;
	struct module_data *mod;

	ret = 0;
	for (walk = modules; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct module_data *)walk->data;
		if (mod->data->event_mixer == NULL)
			continue;
		mret = (mod->data->event_mixer)(conn, status);
		ret = module_process_ret(mret, mod, &walk, &modules);
		if (ret < 0 && mret == MPDCRON_EVENT_RECONNECT_NOW)
			break;
	}
	return ret;
}

int module_output_run(const struct mpd_connection *conn)
{
	int mret, ret;
	GSList *walk;
	struct module_data *mod;

	ret = 0;
	for (walk = modules; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct module_data *)walk->data;
		if (mod->data->event_output == NULL)
			continue;
		mret = (mod->data->event_output)(conn);
		ret = module_process_ret(mret, mod, &walk, &modules);
		if (ret < 0 && mret == MPDCRON_EVENT_RECONNECT_NOW)
			break;
	}
	return ret;
}

int module_options_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	int mret, ret;
	GSList *walk;
	struct module_data *mod;

	ret = 0;
	for (walk = modules; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct module_data *)walk->data;
		if (mod->data->event_options == NULL)
			continue;
		mret = (mod->data->event_options)(conn, status);
		ret = module_process_ret(mret, mod, &walk, &modules);
		if (ret < 0 && mret == MPDCRON_EVENT_RECONNECT_NOW)
			break;
	}
	return ret;
}

int module_update_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	int mret, ret;
	GSList *walk;
	struct module_data *mod;

	ret = 0;
	for (walk = modules; walk != NULL; walk = g_slist_next(walk)) {
		mod = (struct module_data *)walk->data;
		if (mod->data->event_update == NULL)
			continue;
		mret = (mod->data->event_update)(conn, status);
		ret = module_process_ret(mret, mod, &walk, &modules);
		if (ret < 0 && mret == MPDCRON_EVENT_RECONNECT_NOW)
			break;
	}
	return ret;
}
