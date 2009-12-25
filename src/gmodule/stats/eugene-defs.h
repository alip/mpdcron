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

#ifndef MPDCRON_GUARD_STATS_CLIENT_DEFS_H
#define MPDCRON_GUARD_STATS_CLIENT_DEFS_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include "../../cron-config.h"
#include "stats-defs.h"

#include <mpd/client.h>

struct eu_config {
	int verbosity;
	char *homepath;
	char *confpath;
	char *dbpath;
	const char *hostname;
	const char *port;
	const char *password;
};

extern struct eu_config euconfig;

void eulog(int level, const char *fmt, ...);
void load_paths(void);
struct mpd_song *load_current_song(void);
int cmd_update(int argc, char **argv);
int cmd_hate_artist(int argc, char **argv);
int cmd_hate_album(int argc, char **argv);
int cmd_hate_genre(int argc, char **argv);
int cmd_hate_song(int argc, char **argv);
int cmd_love_artist(int argc, char **argv);
int cmd_love_album(int argc, char **argv);
int cmd_love_genre(int argc, char **argv);
int cmd_love_song(int argc, char **argv);
int cmd_kill_song(int argc, char **argv);
int cmd_unkill_song(int argc, char **argv);
int cmd_rate_song(int argc, char **argv);
#endif /* !MPDCRON_GUARD_STATS_CLIENT_DEFS_H */
