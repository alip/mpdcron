/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet ai cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
 *
 * This file is part of the mpdhooker mpd client. mpdhooker is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * mpdhooker is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MPDHOOKER_GUARD_MH_DEFS_H
#define MPDHOOKER_GUARD_MH_DEFS_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <glib.h>
#include <libdaemon/dlog.h>
#include <mpd/client.h>

/* Configuration */
#define ENV_HOME_DIR		"MPDHOOKER_DIR"
#define ENV_MPD_HOST		"MPD_HOST"
#define ENV_MPD_PORT		"MPD_PORT"
#define ENV_MPD_PASSWORD	"MPD_PASSWORD"

#define DOT_MPDHOOKER			"." PACKAGE
#define DOT_HOOKS			"hooks"

#define DEFAULT_PID_KILL_WAIT	3
#define DEFAULT_MPD_RECONNECT	5
#define DEFAULT_MPD_TIMEOUT	0

#define DEFAULT_DATE_FORMAT		"%Y-%m-%d %H-%M-%S %Z"
#define DEFAULT_DATE_FORMAT_SIZE	64

extern char *home_path;
extern char *conf_path;
extern char *pid_path;

extern const char *hostname;
extern const char *port;
extern const char *password;

extern int timeout;
extern int reconnect;
extern int killwait;
extern enum mpd_idle idle;

extern int optnd;
extern GMainLoop *loop;

#define mh_log(level, ...) daemon_log(level, __VA_ARGS__)
#define mh_logv(level, ...)				\
	do {						\
		if (optnd)				\
			daemon_log(level, __VA_ARGS__);	\
	} while(0)

extern const char *mhconf_pid_file_proc(void);
extern int mhconf_init(void);
extern void mhconf_free(void);
extern void mhenv_clearenv(void);
extern int mhenv_list_all_meta(struct mpd_connection *conn);
extern int mhenv_stats(struct mpd_connection *conn);
extern int mhenv_status(struct mpd_connection *conn);
extern int mhenv_status_currentsong(struct mpd_connection *conn);
extern int mhenv_outputs(struct mpd_connection *conn);
extern int mhevent_run(struct mpd_connection *conn, enum mpd_idle event);
extern int mhhooker_run_hook(const char *name);
extern int mhkeyfile_load(void);
extern void mhloop_connect(void);
extern void mhloop_disconnect(void);

#endif /* !MPDHOOKER_GUARD_MH_DEFS_H */
