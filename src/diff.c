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

#include <string.h>

#include <glib.h>

#include "conf.h"
#include "diff.h"
#include "libmpdclient.h"

bool mhdiff_status(mpd_Status *status, struct mhdiff *diff)
{
    bool changed;

    changed = false;
    memset(diff, 0, sizeof(struct mhdiff));
    if (status == NULL || mhconf.status == NULL)
        return false;

    if (status->bitRate != mhconf.status->bitRate) {
        changed = true;
        diff->bitrate = true;
    }

    if (status->consume != mhconf.status->consume) {
        changed = true;
        diff->consume = true;
    }

    if (status->crossfade != mhconf.status->crossfade) {
        changed = true;
        diff->crossfade = true;
    }

    if (status->state == MPD_STATUS_STATE_PLAY || status->state == MPD_STATUS_STATE_PAUSE) {
        if (status->elapsedTime != mhconf.status->elapsedTime) {
            changed = true;
            diff->elapsed = true;
        }
    }

    if (status->playlist != mhconf.status->playlist) {
        changed = true;
        diff->playlist = true;
    }

    if (status->playlistLength != mhconf.status->playlistLength) {
        changed = true;
        diff->playlist_length = true;
    }

    if (status->random != mhconf.status->random) {
        changed = true;
        diff->random = true;
    }

    if (status->repeat != mhconf.status->repeat) {
        changed = true;
        diff->repeat = true;
    }

    if (status->sampleRate != mhconf.status->sampleRate) {
        changed = true;
        diff->samplerate = true;
    }

    if (status->single != mhconf.status->single) {
        changed = true;
        diff->single = true;
    }

    if (status->state != mhconf.status->state) {
        changed = true;
        diff->state = true;
    }

    if (status->updatingDb != status->updatingDb) {
        changed = true;
        diff->updatingdb = true;
    }

    if (status->volume != status->volume) {
        changed = true;
        diff->volume = true;
    }
    return changed;
}

bool mhdiff_song(mpd_InfoEntity *entity)
{
    struct mpd_song *song, *oldsong;

    if (!entity || !mhconf.entity)
        return false;

    song = entity->info.song;
    oldsong = mhconf.entity->info.song;
    if (!song || !oldsong)
        return false;

    if (strcmp(song->file, oldsong->file) != 0)
        return true;

    return false;
}

