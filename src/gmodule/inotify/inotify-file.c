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

#include "inotify-defs.h"

#include <errno.h>
#include <string.h>
#include <sys/inotify.h>

#include <glib.h>
#include <libdaemon/dlog.h>

struct config file_config;
static int ndirs;

static int parse_event_name(const char *name)
{
	if (strcasecmp(name, "all") == 0)
		return IN_ALL_EVENTS;
	else if (strcasecmp(name, "access") == 0)
		return IN_ACCESS;
	else if (strcasecmp(name, "attrib") == 0)
		return IN_ATTRIB;
	else if (strcasecmp(name, "close_write") == 0)
		return IN_CLOSE_WRITE;
	else if (strcasecmp(name, "close_nowrite") == 0)
		return IN_CLOSE_NOWRITE;
	else if (strcasecmp(name, "create") == 0)
		return IN_CREATE;
	else if (strcasecmp(name, "delete") == 0)
		return IN_DELETE;
	else if (strcasecmp(name, "delete_self") == 0)
		return IN_DELETE_SELF;
	else if (strcasecmp(name, "modify") == 0)
		return IN_MODIFY;
	else if (strcasecmp(name, "moved_from") == 0)
		return IN_MOVED_FROM;
	else if (strcasecmp(name, "moved_to") == 0)
		return IN_MOVED_TO;
	else if (strcasecmp(name, "open") == 0)
		return IN_OPEN;
	else
		return -1;
}

static void inotifyfd_destroy(gpointer data)
{
	int fd = GPOINTER_TO_INT(data);
	inotify_rm_watch(ifd, fd);
}

static bool load_integer(GKeyFile *fd, const char *name, int *value_r)
{
	int value;
	GError *e = NULL;

	if (*value_r != -1) {
		/* already set */
		return true;
	}

	value = g_key_file_get_integer(fd, "inotify", name, &e);
	if (e != NULL) {
		if (e->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND &&
				e->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
			daemon_log(LOG_ERR, "%sfailed to load inotify.%s: %s",
					INOTIFY_LOG_PREFIX, name, e->message);
			g_error_free(e);
			return false;
		}
		g_error_free(e);
	}

	*value_r = value;
	return true;
}

static bool load_string(GKeyFile *fd, const char *name, char **value_r)
{
	char *value;
	GError *e = NULL;

	if (*value_r != NULL) {
		/* already set */
		return true;
	}

	value = g_key_file_get_string(fd, "inotify", name, &e);
	if (e != NULL) {
		if (e->code  != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
			daemon_log(LOG_ERR, "%sfailed to load inotify.%s: %s",
					INOTIFY_LOG_PREFIX, name, e->message);
			g_error_free(e);
			return false;
		}
		g_error_free(e);
	}

	*value_r = value;
	return true;
}


static bool load_events(GKeyFile *fd, const char *name, int *events_r)
{
	int event;
	char **values;
	GError *e = NULL;

	if (*events_r != 0) {
		/* already set */
		return true;
	}

	values = g_key_file_get_string_list(fd, "inotify", name, NULL, &e);
	if (e != NULL) {
		if (e->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
			daemon_log(LOG_ERR, "%sfailed to load inotify.%s: %s",
					INOTIFY_LOG_PREFIX, name, e->message);
			g_error_free(e);
			return false;
		}
	}

	if (values != NULL) {
		for (unsigned int i = 0; values[i] != NULL; i++) {
			if ((event = parse_event_name(values[i])) < 0) {
				daemon_log(LOG_WARNING, "%sunrecognized event name: %s",
						INOTIFY_LOG_PREFIX, values[i]);
			}
			else
				*events_r |= event;
		}
		g_strfreev(values);
	}

	return true;
}

static void load_directory(int events, const char *path, GHashTable **table_r)
{
	int wd;
	const char *entry;
	char *rpath;
	GDir *dir;
	GError *e = NULL;

	if (ndirs == 0) {
		if (!g_file_test(path, G_FILE_TEST_IS_DIR)) {
			daemon_log(LOG_WARNING, "%sentry `%s' is not a directory",
					INOTIFY_LOG_PREFIX, path);
			return;
		}
	}

	if ((wd = inotify_add_watch(ifd, path, events)) < 0) {
		daemon_log(LOG_WARNING, "%sadding watch on `%s' failed: %s",
				INOTIFY_LOG_PREFIX, path, g_strerror(errno));
	}
	else
		g_hash_table_insert(*table_r, GINT_TO_POINTER(wd), g_strdup(path));

	if (ndirs++ >= file_config.limit) {
		/* ELOOP */
		daemon_log(LOG_WARNING, "%shit the maximum directory limit",
				INOTIFY_LOG_PREFIX);
		return;
	}

	if ((dir = g_dir_open(path, 0, &e)) == NULL) {
		daemon_log(LOG_WARNING, "%sfailed to open directory %s: %s",
				INOTIFY_LOG_PREFIX, path, e->message);
		g_error_free(e);
		return;
	}

	/* Look for other directories inside */
	while ((entry = g_dir_read_name(dir)) != NULL) {
		/* rpath is already saved in wdir->path and will be saved on
		 * exit, so it's fine to do this.
		 */
		rpath = g_build_filename(path, entry, NULL);
		if (g_file_test(rpath, G_FILE_TEST_IS_DIR))
			load_directory(events, rpath, table_r);
		g_free(rpath);
	}
	g_dir_close(dir);
}

static bool load_inotify_list(GKeyFile *fd, int events,
		const char *root, const char *name, GHashTable **table_r)
{
	char *rpath;
	char **values;
	GError *e = NULL;

	values = g_key_file_get_string_list(fd, "inotify", name, NULL, &e);
	if (e != NULL) {
		if (e->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
			daemon_log(LOG_ERR, "%sfailed to load inotify.%s: %s",
					INOTIFY_LOG_PREFIX, name, e->message);
			g_error_free(e);
			return false;
		}
		g_error_free(e);
	}

	ndirs = 0;
	if (values == NULL)
		load_directory(events, root, table_r);
	else {
		for (unsigned int i = 0; values[i] != NULL; i++) {
			rpath = g_build_filename(root, values[i], NULL);
			load_directory(events, rpath, table_r);
			g_free(rpath);
		}
		g_strfreev(values);
	}
	return true;
}

int file_load(GKeyFile *fd)
{
	GError *e = NULL;

	file_config.events = 0;
	file_config.limit = -1;
	file_config.root = NULL;
	file_config.patterns = NULL;
	file_config.directories = g_hash_table_new_full(g_direct_hash, g_direct_equal, inotifyfd_destroy, g_free);

	if (!load_string(fd, "root", &file_config.root))
		return -1;
	if (file_config.root == NULL) {
		daemon_log(LOG_ERR, "%sfailed to load inotify.root:"
				" entry doesn't exist",
				INOTIFY_LOG_PREFIX);
		return -1;
	}

	if (!load_events(fd, "events", &file_config.events))
		return -1;
	if (file_config.events == 0)
		file_config.events = INOTIFY_EVENTS;

	if (!load_integer(fd, "limit", &file_config.limit))
		return -1;
	if (file_config.limit <= 0)
		file_config.limit = INOTIFY_MAX_DIRS;

	if (!load_inotify_list(fd, file_config.events,
				file_config.root, "directories",
				&file_config.directories))
		return -1;

	file_config.patterns = g_key_file_get_string_list(fd, "inotify", "patterns", NULL, &e);
	if (e != NULL) {
		if (e->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND) {
			daemon_log(LOG_ERR, "%sfailed to load inotify.patterns: %s",
					INOTIFY_LOG_PREFIX, e->message);
			g_error_free(e);
			return -1;
		}
	}

	return 0;
}

void file_cleanup(void)
{
	g_free(file_config.root);
	if (file_config.patterns != NULL) {
		g_strfreev(file_config.patterns);
		file_config.patterns = NULL;
	}
	if (file_config.directories != NULL) {
		g_hash_table_destroy(file_config.directories);
		file_config.directories = NULL;
	}
}
