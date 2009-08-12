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
            daemon_log(LOG_ERR, "Failed to parse config file `%s': %s", mhconf.dir.config, config_error->message);
            g_error_free(config_error);
            g_key_file_free(config_fd);
            return false;
        }
        else {
            g_error_free(config_error);
            g_key_file_free(config_fd);
            return true;
        }
    }

    // Get mpd.poll
    mhconf.poll = g_key_file_get_integer(config_fd, "mpd", "poll", &config_error);
    if (!mhconf.poll && config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                daemon_log(LOG_WARNING, "mpd.poll not an integer: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                return false;
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }
    if (mhconf.poll <= 0) {
        daemon_log(LOG_WARNING, "Invalid value for mpd.poll %d, setting to default", mhconf.poll);
        mhconf.poll = 1;
    }

    // Get mpd.reconnect
    mhconf.reconnect = g_key_file_get_integer(config_fd, "mpd", "reconnect", &config_error);
    if (!mhconf.reconnect && config_error) {
        switch (config_error->code) {
            case G_KEY_FILE_ERROR_INVALID_VALUE:
                daemon_log(LOG_WARNING, "mpd.reconnect not an integer: %s", config_error->message);
                g_error_free(config_error);
                g_key_file_free(config_fd);
                return false;
            case G_KEY_FILE_ERROR_KEY_NOT_FOUND:
                g_error_free(config_error);
                config_error = NULL;
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }

    return true;
}

