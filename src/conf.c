/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <polatel@gmail.com>
 *
 * This file is part of the mpdhooker. mpdhooker is free software; you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation.
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <libdaemon/dlog.h>
#include <libdaemon/dpid.h>

#include "conf.h"

void mhconf_init(void)
{
    char *keyfile_name;

    memset(&mhconf, 0, sizeof(struct globalconf));

    /* Get home directory */
    if (g_getenv(ENV_HOME_DIR))
        mhconf.dir.home = g_strdup(g_getenv(ENV_HOME_DIR));
    else if (g_getenv("HOME"))
        mhconf.dir.home = g_build_filename(g_getenv("HOME"), ".mpdhooker", NULL);
    else {
        daemon_log(LOG_ERR, "Neither "ENV_HOME_DIR" nor HOME is set, exiting!");
        exit(EXIT_FAILURE);
    }

    /* Set keyfile path */
    keyfile_name = g_strdup_printf("%s.conf", daemon_pid_file_ident);
    mhconf.dir.config = g_build_filename(mhconf.dir.home, keyfile_name, NULL);
    g_free(keyfile_name);

    /* Build script paths */
    mhconf.dir.bitrate = g_build_filename(mhconf.dir.home, "hooks", "bitrate", NULL);
    mhconf.dir.consume = g_build_filename(mhconf.dir.home, "hooks", "consume", NULL);
    mhconf.dir.crossfade = g_build_filename(mhconf.dir.home, "hooks", "crossfade", NULL);
    mhconf.dir.elapsed = g_build_filename(mhconf.dir.home, "hooks", "elapsed", NULL);
    mhconf.dir.playlist = g_build_filename(mhconf.dir.home, "hooks", "playlist", NULL);
    mhconf.dir.playlist_length = g_build_filename(mhconf.dir.home, "hooks", "playlist_length", NULL);
    mhconf.dir.random = g_build_filename(mhconf.dir.home, "hooks", "random", NULL);
    mhconf.dir.repeat = g_build_filename(mhconf.dir.home, "hooks", "repeat", NULL);
    mhconf.dir.samplerate = g_build_filename(mhconf.dir.home, "hooks", "samplerate", NULL);
    mhconf.dir.single = g_build_filename(mhconf.dir.home, "hooks", "single", NULL);
    mhconf.dir.song = g_build_filename(mhconf.dir.home, "hooks", "song", NULL);
    mhconf.dir.state = g_build_filename(mhconf.dir.home, "hooks", "state", NULL);
    mhconf.dir.updatingdb = g_build_filename(mhconf.dir.home, "hooks", "updatingdb", NULL);
    mhconf.dir.volume = g_build_filename(mhconf.dir.home, "hooks", "volume", NULL);

    mhconf.hostname = g_getenv(ENV_MPD_HOST);
    if (mhconf.hostname == NULL)
        mhconf.hostname = "localhost";

    mhconf.port = g_getenv(ENV_MPD_PORT);
    if (mhconf.port == NULL)
        mhconf.port = "6600";

    mhconf.password = g_getenv(ENV_MPD_PASSWORD);

    mhconf.reconnect = 30;
    mhconf.poll = 1;
}

void mhconf_free(void)
{
    g_free(mhconf.dir.pid);
    g_free(mhconf.dir.config);
    g_free(mhconf.dir.home);
    mhconf.dir.pid = NULL;
    mhconf.dir.config = NULL;
    mhconf.dir.home = NULL;

    if (mhconf.entity != NULL) {
        mpd_entity_free(mhconf.entity);
        mhconf.entity = NULL;
    }
    if (mhconf.status != NULL) {
        mpd_status_free(mhconf.status);
        mhconf.status = NULL;
    }
    if (mhconf.conn != NULL) {
        mpd_connection_free(mhconf.conn);
        mhconf.conn = NULL;
    }
}

const gchar *mhconf_pid_file_proc(void)
{
    gchar *name = g_strdup_printf("%s.pid", daemon_pid_file_ident);
    mhconf.dir.pid = g_build_filename(mhconf.dir.home, name, NULL);
    g_free(name);
    return mhconf.dir.pid;
}

