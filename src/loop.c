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

#include <glib.h>

#include <libdaemon/dlog.h>

#include <mpd/client.h>

#include "conf.h"
#include "diff.h"
#include "hooker.h"
#include "loop.h"
#include "mpd.h"

static gint mhloop_main_loop(void)
{
    int mpderr;
    struct mpd_status *status;
    struct mpd_entity *entity;
    struct mhdiff diff;
    bool status_changed, song_changed;

    mhmpd_connect();
    for (;;) {
        mpderr = mhmpd_status(&status);
        if (mpderr != MPD_ERROR_SUCCESS) {
            if (mpderr > 0) {
                /* Connection failed, reconnect */
                mpd_connection_free(mhconf.conn);
                mhmpd_connect();
                continue;
            }
            continue;
        }
        mpderr = mhmpd_currentsong(&entity);
        if (mpderr != MPD_ERROR_SUCCESS) {
            if (mpderr > 0) {
                /* Connection failed, reconnect */
                mpd_connection_free(mhconf.conn);
                mhmpd_connect();
                continue;
            }
        }

        if (mhconf.status == NULL) {
            /* Initial run, copy the data */
            mhconf.status = status;
            mhconf.entity = entity;
            continue;
        }

        status_changed = mhdiff_status(status, &diff);
        song_changed = mhdiff_song(entity);

        if (status_changed || song_changed) {
            mhhooker_setenv(status, entity);
            mhhooker_run(diff, song_changed);
        }

        if (mhconf.status != NULL) {
            mpd_status_free(mhconf.status);
            mhconf.status = status;
        }
        if (mhconf.entity != NULL) {
            mpd_entity_free(mhconf.entity);
            mhconf.entity = entity;
        }
    }
    return 0;
}

static gint mhloop_main_foreground(void)
{
    return mhloop_main_loop();
}

static gint mhloop_main_background(void)
{
    return 0;
}

gint mhloop_main(void)
{
    if (mhconf.opt_no_daemonize)
        return mhloop_main_foreground();
    else
        return mhloop_main_background();
}

