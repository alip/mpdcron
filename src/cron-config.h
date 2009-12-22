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

#ifndef MPDCRON_GUARD_CRON_CONFIG_H
#define MPDCRON_GUARD_CRON_CONFIG_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/* Configuration */
#define ENV_HOME_DIR		"MPDCRON_DIR"
#define ENV_MPD_HOST		"MPD_HOST"
#define ENV_MPD_PORT		"MPD_PORT"
#define ENV_MPD_PASSWORD	"MPD_PASSWORD"

#define DOT_MPDCRON			"." PACKAGE
#define DOT_HOOKS			"hooks"
#define DOT_MODULES			"modules"

#define MODULE_INIT_FUNC		"mpdcron_init"
#define MODULE_CLOSE_FUNC		"mpdcron_close"
#define MODULE_RUN_FUNC			"mpdcron_run"

#define DEFAULT_PID_KILL_WAIT	3
#define DEFAULT_MPD_RECONNECT	5
#define DEFAULT_MPD_TIMEOUT	0

#define DEFAULT_DATE_FORMAT		"%Y-%m-%d %H-%M-%S %Z"
#define DEFAULT_DATE_FORMAT_SIZE	64

enum module_retval {
	MODULE_RETVAL_SUCCESS = 0,
	MODULE_RETVAL_RECONNECT,
	MODULE_RETVAL_RECONNECT_NOW,
	MODULE_RETVAL_UNLOAD,
};

#endif /* !MPDCRON_GUARD_CRON_CONFIG_H */
