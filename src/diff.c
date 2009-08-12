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

#include <mpd/client.h>

#include "conf.h"
#include "diff.h"

bool mhdiff_status(struct mpd_status *status, struct mhdiff *diff)
{
    bool changed;
    gint oldvalue, newvalue;
    long long oldvalue_long, newvalue_long;

    changed = false;
    memset(diff, 0, sizeof(struct mhdiff));
    if (status == NULL)
        return false;

    newvalue = mpd_status_get_bit_rate(status);
    oldvalue = mpd_status_get_bit_rate(mhconf.status);
    if (newvalue != oldvalue) {
        changed = true;
        diff->bitrate = true;
    }

    newvalue = mpd_status_get_consume(status);
    oldvalue = mpd_status_get_consume(mhconf.status);
    if (newvalue != oldvalue) {
        changed = true;
        diff->consume = true;
    }

    newvalue = mpd_status_get_crossfade(status);
    oldvalue = mpd_status_get_crossfade(mhconf.status);
    if (newvalue != oldvalue) {
        changed = true;
        diff->crossfade = true;
    }

    if (mpd_status_get_state(status) == MPD_STATE_PLAY ||
            mpd_status_get_state(status) == MPD_STATE_PAUSE) {
        newvalue = mpd_status_get_elapsed_time(status);
        oldvalue = mpd_status_get_elapsed_time(mhconf.status);
        if (newvalue != oldvalue) {
            changed = true;
            diff->elapsed = true;
        }
    }

    newvalue_long = mpd_status_get_playlist(status);
    oldvalue_long = mpd_status_get_playlist(mhconf.status);
    if (newvalue_long != oldvalue_long) {
        changed = true;
        diff->playlist = true;
    }

    newvalue = mpd_status_get_playlist_length(status);
    oldvalue = mpd_status_get_playlist_length(mhconf.status);
    if (newvalue != oldvalue) {
        changed = true;
        diff->playlist_length = true;
    }

    newvalue = mpd_status_get_random(status);
    oldvalue = mpd_status_get_random(mhconf.status);
    if (newvalue != oldvalue) {
        changed = true;
        diff->random = true;
    }

    newvalue = mpd_status_get_repeat(status);
    oldvalue = mpd_status_get_repeat(mhconf.status);
    if (newvalue != oldvalue) {
        changed = true;
        diff->repeat = true;
    }

    newvalue = mpd_status_get_sample_rate(status);
    oldvalue = mpd_status_get_sample_rate(mhconf.status);
    if (newvalue != oldvalue) {
        changed = true;
        diff->samplerate = true;
    }

    newvalue = mpd_status_get_single(status);
    oldvalue = mpd_status_get_single(mhconf.status);
    if (newvalue != oldvalue) {
        changed = true;
        diff->single = true;
    }

    newvalue = mpd_status_get_state(status);
    oldvalue = mpd_status_get_state(mhconf.status);
    if (newvalue != oldvalue) {
        changed = true;
        diff->state = true;
    }

    newvalue = mpd_status_get_updatingdb(status);
    oldvalue = mpd_status_get_updatingdb(mhconf.status);
    if (newvalue != oldvalue) {
        changed = true;
        diff->updatingdb = true;
    }

    newvalue = mpd_status_get_volume(status);
    oldvalue = mpd_status_get_volume(mhconf.status);
    if (newvalue != oldvalue) {
        changed = true;
        diff->volume = true;
    }
    return changed;
}

bool mhdiff_song(struct mpd_entity *entity)
{
    const char *oldfile, *newfile;
    struct mpd_song *song, *oldsong;

    if (!entity)
        return false;

    song = entity->info.song;
    if (!song)
        return false;
    oldsong = mhconf.entity->info.song;

    oldfile = mpd_song_get_tag(oldsong, MPD_TAG_FILENAME, 0);
    newfile = mpd_song_get_tag(song, MPD_TAG_FILENAME, 0);
    if (strcmp(oldfile, newfile) != 0)
        return true;

    return false;
}

