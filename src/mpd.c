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
#include <unistd.h>

#include <glib.h>

#include "conf.h"
#include "libmpdclient.h"
#include "mpd.h"

gint mhmpd_status_song(mpd_Status **status, mpd_InfoEntity **entity)
{
    mh_logv(LOG_DEBUG, "Sending command `status && currentsong' to mpd...");
    mpd_sendCommandListOkBegin(mhconf.conn);
    if (mhconf.conn->error) {
        mh_log(LOG_ERR, "mpd_sendCommandListOkBegin() failed: %s!", mhconf.conn->errorStr);
        return -1;
    }
    mpd_sendStatusCommand(mhconf.conn);
    if (mhconf.conn->error) {
        mh_log(LOG_ERR, "mpd_sendStatusCommand() failed: %s!", mhconf.conn->errorStr);
        return -1;
    }
    mpd_sendCurrentSongCommand(mhconf.conn);
    if (mhconf.conn->error) {
        mh_log(LOG_ERR, "mpd_sendCurrentSongCommand() failed: %s!", mhconf.conn->errorStr);
        return -1;
    }
    mpd_sendCommandListEnd(mhconf.conn);
    if (mhconf.conn->error) {
        mh_log(LOG_ERR, "mpd_sendCommandListEnd() failed: %s!", mhconf.conn->errorStr);
        return -1;
    }

    *status = mpd_getStatus(mhconf.conn);
    if (mhconf.conn->error) {
        mh_log(LOG_ERR, "mpd_getStatus() failed: %s!", mhconf.conn->errorStr);
        return -1;
    }

    if ((*status)->state == MPD_STATUS_STATE_PLAY || (*status)->state == MPD_STATUS_STATE_PAUSE) {
        mpd_nextListOkCommand(mhconf.conn);
        if (mhconf.conn->error) {
            mh_log(LOG_ERR, "mpd_nextListOkCommand() failed: %s!", mhconf.conn->errorStr);
            return -1;
        }

        *entity = mpd_getNextInfoEntity(mhconf.conn);
        while (*entity) {
            if ((*entity)->type == MPD_INFO_ENTITY_TYPE_SONG)
                break;
            mpd_freeInfoEntity(*entity);
            *entity = mpd_getNextInfoEntity(mhconf.conn);
        }

        if (!(*entity)) {
            mh_log(LOG_ERR, "Failed to find any song entity although state is play or pause!");
            return -1;
        }
    }
    mpd_finishCommand(mhconf.conn);
    if (mhconf.conn->error) {
        mh_log(LOG_ERR, "mpd_finishCommand() failed: %s!", mhconf.conn->errorStr);
        return -1;
    }
    return 0;
}

gint mhmpd_connect(void)
{
    for (unsigned int try = 1 ; ; try++) {
        mh_log(LOG_INFO, "Connecting to `%s' on port %s with timeout: %.2lf (try: %d)",
                mhconf.hostname, mhconf.port, mhconf.timeout, try);
        mhconf.conn = mpd_newConnection(mhconf.hostname, atoi(mhconf.port), mhconf.timeout);
        if (!mhconf.conn || mhconf.conn->error) {
            if (mhconf.reconnect > 0) {
                mh_log(LOG_ERR, "Failed to connect: %s", mhconf.conn ? mhconf.conn->errorStr : "unknown");
                mh_log(LOG_ERR, "Retrying in %d seconds!", mhconf.reconnect);
                if (mhconf.conn)
                    mpd_closeConnection(mhconf.conn);
                sleep(mhconf.reconnect);
                continue;
            }
            else {
                mh_log(LOG_ERR, "Failed to connect: %s", mhconf.conn ? mhconf.conn->errorStr : "unknown");
                exit(EXIT_FAILURE);
            }
        }
        mh_log(LOG_INFO, "Connected to `%s' on port %s.", mhconf.hostname, mhconf.port);
        if (mhconf.password != NULL) {
            mh_log(LOG_INFO, "Sending password...");
            mpd_sendPasswordCommand(mhconf.conn, mhconf.password);
            if (mhconf.conn->error) {
                mh_log(LOG_ERR, "Authentication failed: %s!", mhconf.conn->errorStr);
                exit(EXIT_FAILURE);
            }
            mpd_finishCommand(mhconf.conn);
            if (mhconf.conn->error) {
                mh_log(LOG_ERR, "Finish command failed: %s!", mhconf.conn->errorStr);
                exit(EXIT_FAILURE);
            }
        }
        return 0;
    }
    return 0;
}

