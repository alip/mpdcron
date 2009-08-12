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

#include <mpd/client.h>

#include "conf.h"
#include "mpd.h"

gint mhmpd_status(struct mpd_status **status)
{
    mh_logv(LOG_DEBUG, "Sending command `status' to mpd...");
    mpd_send_status(mhconf.conn);
    if (mpd_get_error(mhconf.conn) != MPD_ERROR_SUCCESS) {
        mh_log(LOG_ERR, "Connection error: %s!", mpd_get_error_message(mhconf.conn));
        return mpd_get_error(mhconf.conn);
    }
    *status = mpd_get_status(mhconf.conn);
    mpd_response_finish(mhconf.conn);

    if (!(*status)) {
        mh_log(LOG_ERR, "Failed to get status information: %s!", mpd_get_error_message(mhconf.conn));
        return mpd_get_error(mhconf.conn);
    }
    else if (mpd_status_get_error(*status)) {
        mh_log(LOG_ERR, "Status error: %s!", mpd_status_get_error(*status));
        return -1;
    }
    mh_logv(LOG_DEBUG, "Command successful.");
    return MPD_ERROR_SUCCESS;
}

gint mhmpd_currentsong(struct mpd_entity **entity)
{
    struct mpd_song *song;

    mh_logv(LOG_DEBUG, "Sending command `currentsong'");
    mpd_send_currentsong(mhconf.conn);
    if (mpd_get_error(mhconf.conn) != MPD_ERROR_SUCCESS) {
        mh_log(LOG_ERR, "Connection error: %s", mpd_get_error_message(mhconf.conn));
        return mpd_get_error(mhconf.conn);
    }
    *entity = mpd_get_next_entity(mhconf.conn);
    mpd_response_finish(mhconf.conn);

    if (*entity) {
        song = (*entity)->info.song;
        if ((*entity)->type != MPD_ENTITY_TYPE_SONG || !song) {
            mh_log(LOG_ERR, "Entity doesn't have the expected type `song': %d", (*entity)->type);
            mpd_entity_free(*entity);
            return -1;
        }
        mh_logv(LOG_DEBUG, "Command successful");
        return MPD_ERROR_SUCCESS;
    }
    mh_logv(LOG_DEBUG, "Command failed");
    return -1;
}

gint mhmpd_connect(void)
{
    for (unsigned int try = 1 ; ; try++) {
        mh_log(LOG_INFO, "Connecting to `%s' on port %s with timeout: %.2lf (try: %d)",
                mhconf.hostname, mhconf.port, mhconf.timeout, try);
        mhconf.conn = mpd_connection_new(mhconf.hostname, atoi(mhconf.port), mhconf.timeout);
        if (!mhconf.conn) {
            if (mhconf.reconnect > 0) {
                mh_log(LOG_ERR, "Failed to connect, retrying in %d seconds!", mhconf.reconnect);
                sleep(mhconf.reconnect);
                continue;
            }
            else {
                mh_log(LOG_ERR, "Failed to connect, exiting!");
                exit(EXIT_FAILURE);
            }
        }
        else if (mpd_get_error(mhconf.conn) != MPD_ERROR_SUCCESS) {
            if (mhconf.reconnect > 0) {
                mh_log(LOG_ERR, "Failed to connect: %s, retrying in %d seconds!",
                        mpd_get_error_message(mhconf.conn), mhconf.reconnect);
                mpd_connection_free(mhconf.conn);
                sleep(mhconf.reconnect);
                continue;
            }
            else {
                mh_log(LOG_ERR, "Failed to connect, exiting!");
                exit(EXIT_FAILURE);
            }
        }
        mh_log(LOG_INFO, "Connected to `%s' on port %s.", mhconf.hostname, mhconf.port);
        if (mhconf.password != NULL) {
            mh_log(LOG_INFO, "Sending password...");
            if (!mpd_send_password(mhconf.conn, mhconf.password)) {
                mh_log(LOG_ERR, "Authentication failed: %s!", mpd_get_error_message(mhconf.conn));
                exit(EXIT_FAILURE);
            }
            mpd_response_finish(mhconf.conn);
        }
        return 0;
    }
    return 0;
}

