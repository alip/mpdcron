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

#include <stdbool.h>

#include <glib.h>

#include <libdaemon/dlog.h>

#include "conf.h"
#include "keyfile.h"

bool mhkeyfile_load(void)
{
    GKeyFile *config_fd;
    GError *config_error;

    config_fd = g_key_file_new();
    config_error = NULL;
    if (!g_key_file_load_from_file(config_fd, mhconf.dir.config, G_KEY_FILE_NONE, &config_error)) {
        if (config_error->code != G_KEY_FILE_ERROR_NOT_FOUND) {
            mh_log(LOG_ERR, "Failed to parse config file `%s': %s", mhconf.dir.config, config_error->message);
            g_error_free(config_error);
            g_key_file_free(config_fd);
            return false;
        }
        else {
            mh_logv(LOG_DEBUG, "Configuration file `%s' not found, skipping...", mhconf.dir.config);
            g_error_free(config_error);
            g_key_file_free(config_fd);
            return true;
        }
    }
    else
        mh_logv(LOG_DEBUG, "Set mpd.poll to %d.", mhconf.poll);

    // Get mpd.poll
    mh_logv(LOG_DEBUG, "Reading mpd.poll from configuration file.");
    mhconf.poll = g_key_file_get_integer(config_fd, "mpd", "poll", &config_error);
    if (!mhconf.poll && config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                mh_log(LOG_WARNING, "mpd.poll not an integer: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                return false;
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                mh_logv(LOG_DEBUG, "mpd.poll not found.");
                g_error_free(config_error);
                config_error = NULL;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }
    if (mhconf.poll <= 0) {
        mh_log(LOG_WARNING, "Invalid value for mpd.poll %d, setting to default.", mhconf.poll);
        mhconf.poll = DEFAULT_MPD_POLL;
    }
    else
        mh_logv(LOG_DEBUG, "set mpd.poll=%d", mhconf.poll);

    // Get mpd.reconnect
    mh_logv(LOG_DEBUG, "Reading mpd.reconnect from configuration file.");
    mhconf.reconnect = g_key_file_get_integer(config_fd, "mpd", "reconnect", &config_error);
    if (!mhconf.reconnect && config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                mh_log(LOG_WARNING, "mpd.reconnect not an integer: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                return false;
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                mh_logv(LOG_DEBUG, "mpd.reconnect not found.");
                g_error_free(config_error);
                config_error = NULL;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }
    else
        mh_logv(LOG_DEBUG, "set mpd.reconnect=%d", mhconf.reconnect);

    // Get mpd.reconnect
    mh_logv(LOG_DEBUG, "Reading mpd.timeout from configuration file.");
    mhconf.timeout = g_key_file_get_double(config_fd, "mpd", "timeout", &config_error);
    if (config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                mh_log(LOG_WARNING, "mpd.timeout not a double: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                return false;
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                mh_logv(LOG_DEBUG, "mpd.timeout not found.");
                g_error_free(config_error);
                config_error = NULL;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }
    if (mhconf.timeout <= 0) {
        mh_log(LOG_WARNING, "Invalid value for mpd.timeout %.2lf, setting to default.", mhconf.timeout);
        mhconf.timeout = DEFAULT_MPD_TIMEOUT;
    }
    else
        mh_logv(LOG_DEBUG, "set mpd.timeout=%.2lf", mhconf.timeout);

    return true;
}

