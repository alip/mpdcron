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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>

#include <libdaemon/dfork.h>
#include <libdaemon/dlog.h>
#include <libdaemon/dpid.h>

#include "conf.h"
#include "diff.h"
#include "hooker.h"
#include "libmpdclient.h"
#include "loop.h"
#include "mpd.h"

G_GNUC_NORETURN static gint mhloop_main_loop(void)
{
    mpd_Status *status;
    mpd_InfoEntity *entity;
    struct mhdiff diff;
    bool status_changed, song_changed;

    mhmpd_connect();
    for (;;) {
        status = NULL;
        entity = NULL;
        if (mhmpd_status_song(&status, &entity) < 0) {
            mpd_closeConnection(mhconf.conn);
            mhmpd_connect();
            mh_logv(LOG_DEBUG, "Sleeping for %d seconds", mhconf.poll);
            sleep(mhconf.poll);
            continue;
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

        if (mhconf.status != NULL)
            mpd_freeStatus(mhconf.status);
        if (mhconf.entity != NULL)
            mpd_freeInfoEntity(mhconf.entity);

        mhconf.status = status;
        mhconf.entity = entity;

        mh_logv(LOG_DEBUG, "Sleeping for %d seconds", mhconf.poll);
        sleep(mhconf.poll);
    }
    /* not reached */
    assert(false);
}

static gint mhloop_main_foreground(void)
{
    return mhloop_main_loop();
}

static gint mhloop_main_background(void)
{
    int pid, ret;

    pid = daemon_pid_file_is_running();
    if (pid >= 0) {
         mh_log(LOG_ERR, "Daemon already running on PID %u", pid);
         return EXIT_FAILURE;
    }

    daemon_retval_init();
    pid = daemon_fork();
    if (pid < 0) {
        mh_log(LOG_ERR, "Failed to fork: %s", strerror(errno));
        daemon_retval_done();
        return EXIT_FAILURE;
    }
    else if (pid != 0) { /* Parent */
        ret = daemon_retval_wait(2);
        if (ret < 0) {
            mh_log(LOG_ERR, "Could not recieve return value from daemon process: %s", strerror(errno));
            return 255;
        }

        mh_log(ret != 0 ? LOG_ERR : LOG_INFO, "Daemon returned %i as return value.", ret);
        return ret;
    }
    else { /* Daemon */
        /* Close FDs */
        if (daemon_close_all(-1) < 0) {
            mh_log(LOG_ERR, "Failed to close all file descriptors: %s", strerror(errno));

            /* Send the error condition to the parent process */
            daemon_retval_send(1);
            return EXIT_FAILURE;
        }

        /* Create the PID file */
        if (daemon_pid_file_create() < 0) {
            mh_log(LOG_ERR, "Failed to create PID file: %s", strerror(errno));
            daemon_retval_send(2);
            return EXIT_FAILURE;
        }

        /* Send OK to parent process */
        daemon_retval_send(0);
        mh_log(LOG_INFO, "Successfully started");

        return mhloop_main_loop();
    }
    return EXIT_SUCCESS;
}

gint mhloop_main(void)
{
    if (mhconf.opt_no_daemonize)
        return mhloop_main_foreground();
    else
        return mhloop_main_background();
}

