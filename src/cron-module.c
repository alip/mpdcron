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

typedef void (*initfunc_t) (void);
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

static void module_init_list(const char *name, GSList **list)
{
	int len;
	char *path, *initsym;
	const char *entry;
	GDir *mdir;
	GError *direrr = NULL;
	GModule *mod;
	initfunc_t initfunc = NULL;

	if ((mdir = g_dir_open(mod_path, 0, &direrr)) == NULL) {
		switch (direrr->code) {
			case G_FILE_ERROR_NOENT:
				/* skip */
				break;
			default:
				crlog(LOG_WARNING, "Failed to open modules directory `%s': %s",
						mod_path, direrr->message);
				break;
		}
		g_error_free(direrr);
	}

	len = strlen(name);
	while ((entry = g_dir_read_name(mdir)) != NULL) {
		if (strncmp(entry, name, len) == 0) {
			/* Check if the suffix is what we want. */
			char *dot = strrchr(entry, '.');
			if (dot != NULL && strncmp(++dot, G_MODULE_SUFFIX, strlen(G_MODULE_SUFFIX) + 1) == 0) {
				path = g_build_filename(mod_path, entry, NULL);
				if ((mod = g_module_open(path, G_MODULE_BIND_LAZY)) == NULL) {
					crlog(LOG_WARNING, "Error loading module `%s': %s", path, g_module_error());
					g_free(path);
					continue;
				}
				/* Run the init function if there's any */
				initsym = g_strdup_printf("%s_init", name);
				if (g_module_symbol(mod, initsym, (gpointer *)&initfunc) && initfunc != NULL)
					initfunc();
				g_free(initsym);
				/* Add to the list of modules */
				*list = g_slist_prepend(*list, path);
				crlogv(LOG_DEBUG, "Loaded `%s' to module list: %s", path, name);
				g_free(path);
			}
		}
	}
	g_dir_close(mdir);
}

static void module_close_list(const char *name, GSList **list)
{
	char *closesym;
	GSList *walk;
	GModule *mod;
	closefunc_t closefunc = NULL;

	closesym = g_strdup_printf("%s_close", name);
	for (walk = *list; walk != NULL; walk = g_slist_next(walk)) {
		mod = (GModule *) walk->data;
		/* Run the close function if there's any */
		if (g_module_symbol(mod, closesym, (gpointer *)&closefunc) && closefunc != NULL)
			closefunc();
		g_module_close(mod);
	}
	g_free(closesym);
	g_slist_free(*list);
	*list = NULL;
}

void module_init(void)
{
	module_init_list(mpd_idle_name(MPD_IDLE_DATABASE), &modules_database);
	module_init_list(mpd_idle_name(MPD_IDLE_STORED_PLAYLIST), &modules_stored_playlist);
	module_init_list(mpd_idle_name(MPD_IDLE_QUEUE), &modules_queue);
	module_init_list(mpd_idle_name(MPD_IDLE_PLAYER), &modules_player);
	module_init_list(mpd_idle_name(MPD_IDLE_MIXER), &modules_mixer);
	module_init_list(mpd_idle_name(MPD_IDLE_OUTPUT), &modules_output);
	module_init_list(mpd_idle_name(MPD_IDLE_OPTIONS), &modules_options);
	module_init_list(mpd_idle_name(MPD_IDLE_UPDATE), &modules_update);
}

void module_close(void)
{
	module_close_list(mpd_idle_name(MPD_IDLE_DATABASE), &modules_database);
	module_close_list(mpd_idle_name(MPD_IDLE_STORED_PLAYLIST), &modules_stored_playlist);
	module_close_list(mpd_idle_name(MPD_IDLE_QUEUE), &modules_queue);
	module_close_list(mpd_idle_name(MPD_IDLE_PLAYER), &modules_player);
	module_close_list(mpd_idle_name(MPD_IDLE_MIXER), &modules_mixer);
	module_close_list(mpd_idle_name(MPD_IDLE_OUTPUT), &modules_output);
	module_close_list(mpd_idle_name(MPD_IDLE_OPTIONS), &modules_options);
	module_close_list(mpd_idle_name(MPD_IDLE_UPDATE), &modules_update);
}

int module_database_run(const struct mpd_connection *conn, const struct mpd_stats *stats)
{
	int ret;
	char *funcsym;
	GSList *walk;
	database_func_t func = NULL;

	funcsym = g_strdup_printf("%s_run", mpd_idle_name(MPD_IDLE_DATABASE));
	for (walk = modules_database; walk != NULL; walk = g_slist_next(walk)) {
		if (g_module_symbol((GModule *)walk->data, funcsym, (gpointer *)&func) && func != NULL) {
			if ((ret = func(conn, stats)) < 0)
				return ret;
		}
	}
	g_free(funcsym);
	return 0;
}

int module_stored_playlist_run(const struct mpd_connection *conn)
{
	int ret;
	char *funcsym;
	GSList *walk;
	stored_playlist_func_t func = NULL;

	funcsym = g_strdup_printf("%s_run", mpd_idle_name(MPD_IDLE_STORED_PLAYLIST));
	for (walk = modules_stored_playlist; walk != NULL; walk = g_slist_next(walk)) {
		if (g_module_symbol((GModule *)walk->data, funcsym, (gpointer *)&func) && func != NULL) {
			if ((ret = func(conn)) < 0)
				return ret;
		}
	}
	g_free(funcsym);
	return 0;
}

int module_queue_run(const struct mpd_connection *conn)
{
	int ret;
	char *funcsym;
	GSList *walk;
	queue_func_t func = NULL;

	funcsym = g_strdup_printf("%s_run", mpd_idle_name(MPD_IDLE_QUEUE));
	for (walk = modules_queue; walk != NULL; walk = g_slist_next(walk)) {
		if (g_module_symbol((GModule *)walk->data, funcsym, (gpointer *)&func) && func != NULL) {
			if ((ret = func(conn)) < 0)
				return ret;
		}
	}
	g_free(funcsym);
	return 0;
}

extern int module_player_run(const struct mpd_connection *conn, const struct mpd_song *song,
		const struct mpd_status *status)
{
	int ret;
	char *funcsym;
	GSList *walk;
	player_func_t func = NULL;

	funcsym = g_strdup_printf("%s_run", mpd_idle_name(MPD_IDLE_PLAYER));
	for (walk = modules_player; walk != NULL; walk = g_slist_next(walk)) {
		if (g_module_symbol((GModule *)walk->data, funcsym, (gpointer *)&func) && func != NULL) {
			if ((ret = func(conn, song, status)) < 0)
				return ret;
		}
	}
	return 0;
}

int module_mixer_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	int ret;
	char *funcsym;
	GSList *walk;
	mixer_func_t func = NULL;

	funcsym = g_strdup_printf("%s_run", mpd_idle_name(MPD_IDLE_MIXER));
	for (walk = modules_mixer; walk != NULL; walk = g_slist_next(walk)) {
		if (g_module_symbol((GModule *)walk->data, funcsym, (gpointer *)&func) && func != NULL) {
			if ((ret = func(conn, status)) < 0)
				return ret;
		}
	}
	return 0;
}

int module_output_run(const struct mpd_connection *conn)
{
	int ret;
	char *funcsym;
	GSList *walk;
	output_func_t func = NULL;

	funcsym = g_strdup_printf("%s_run", mpd_idle_name(MPD_IDLE_OUTPUT));
	for (walk = modules_output; walk != NULL; walk = g_slist_next(walk)) {
		if (g_module_symbol((GModule *)walk->data, funcsym, (gpointer *)&func) && func != NULL) {
			if ((ret = func(conn)) < 0)
				return ret;
		}
	}
	g_free(funcsym);
	return 0;
}

int module_options_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	int ret;
	char *funcsym;
	GSList *walk;
	options_func_t func = NULL;

	funcsym = g_strdup_printf("%s_run", mpd_idle_name(MPD_IDLE_OPTIONS));
	for (walk = modules_options; walk != NULL; walk = g_slist_next(walk)) {
		if (g_module_symbol((GModule *)walk->data, funcsym, (gpointer *)&func) && func != NULL) {
			if ((ret = func(conn, status)) < 0)
				return ret;
		}
	}
	return 0;
}

int module_update_run(const struct mpd_connection *conn, const struct mpd_status *status)
{
	int ret;
	char *funcsym;
	GSList *walk;
	update_func_t func = NULL;

	funcsym = g_strdup_printf("%s_run", mpd_idle_name(MPD_IDLE_UPDATE));
	for (walk = modules_update; walk != NULL; walk = g_slist_next(walk)) {
		if (g_module_symbol((GModule *)walk->data, funcsym, (gpointer *)&func) && func != NULL) {
			if ((ret = func(conn, status)) < 0)
				return ret;
		}
	}
	return 0;
}
