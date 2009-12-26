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

#ifndef MPDCRON_GUARD_CRON_DEFS_H
#define MPDCRON_GUARD_CRON_DEFS_H 1

#include "cron-config.h"

#include <glib.h>
#include <libdaemon/dlog.h>
#include <mpd/client.h>

extern struct mpdcron_config conf;
extern GMainLoop *loop;

const char *conf_pid_file_proc(void);
int conf_init(void);
void conf_free(void);
void env_clearenv(void);
int env_list_all_meta(struct mpd_connection *conn);
int env_list_queue_meta(struct mpd_connection *conn);
int env_stats(struct mpd_connection *conn, struct mpd_stats **stats);
int env_status(struct mpd_connection *conn, struct mpd_status **status);
int env_status_currentsong(struct mpd_connection *conn, struct mpd_song **song, struct mpd_status **status);
int env_outputs(struct mpd_connection *conn);
int event_run(struct mpd_connection *conn, enum mpd_idle event);
int hooker_run_hook(const char *name);
int keyfile_load(GKeyFile **cfd_r);
#ifdef HAVE_GMODULE
int keyfile_load_modules(GKeyFile **cfd_r);
#endif /* HAVE_GMODULE */
void loop_connect(void);
void loop_disconnect(void);
#ifdef HAVE_GMODULE
int module_load(const char *modname, GKeyFile *config_fd);
void module_close(void);
int module_database_run(const struct mpd_connection *conn, const struct mpd_stats *stats);
int module_stored_playlist_run(const struct mpd_connection *conn);
int module_queue_run(const struct mpd_connection *conn);
int module_player_run(const struct mpd_connection *conn, const struct mpd_song *song,
		const struct mpd_status *status);
int module_mixer_run(const struct mpd_connection *conn, const struct mpd_status *status);
int module_output_run(const struct mpd_connection *conn);
int module_options_run(const struct mpd_connection *conn, const struct mpd_status *status);
int module_update_run(const struct mpd_connection *conn, const struct mpd_status *status);
#endif /* HAVE_GMODULE */

#endif /* !MPDCRON_GUARD_CRON_DEFS_H */
