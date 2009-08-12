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

#include "conf.h"
#include "diff.h"
#include "hooker.h"
#include "libmpdclient.h"

static const char *mhhooker_strstate(int state)
{
    const char *strstate;

    switch (state) {
        case MPD_STATUS_STATE_PLAY:
            strstate = "play";
            break;
        case MPD_STATUS_STATE_PAUSE:
            strstate = "pause";
            break;
        case MPD_STATUS_STATE_STOP:
            strstate = "stop";
            break;
        case MPD_STATUS_STATE_UNKNOWN:
            strstate = "unknown";
            break;
        default:
            g_assert_not_reached();
            break;
    }
    return strstate;
}

static gint mhhooker_run_hook(const gchar *name, gchar **argv)
{
    gint pid;
    GError *hook_error;

    mh_logv(LOG_DEBUG, "Running hook `%s'", name);
    hook_error = NULL;
    if (!g_spawn_async(mhconf.dir.home, argv, NULL,
            G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_CHILD_INHERITS_STDIN,
            NULL, NULL, &pid, &hook_error)) {
        if (hook_error->code != G_SPAWN_ERROR_NOENT && hook_error->code != G_SPAWN_ERROR_NOEXEC)
            mh_log(LOG_WARNING, "Failed to execute hook `%s': %s", name, hook_error->message);
        g_error_free(hook_error);
        return -1;
    }
    return 0;
}

void mhhooker_setenv(mpd_Status *status, mpd_InfoEntity *entity)
{
    gchar *oldvalue, *newvalue;
    struct mpd_song *oldsong, *newsong;

    mh_logv(LOG_DEBUG, "Setting environment");
    if (!mhconf.status || !status)
        return;

    /* Bitrate */
    oldvalue = g_strdup_printf("%d", mhconf.status->bitRate);
    newvalue = g_strdup_printf("%d", status->bitRate);
    g_setenv("MPD_BITRATE_OLD", oldvalue, 1);
    g_setenv("MPD_BITRATE", newvalue, 1);
    g_free(oldvalue) ; g_free(newvalue);

    /* Consume (boolean) */
    newvalue = g_strdup_printf("%d", status->consume);
    g_setenv("MPD_CONSUME", newvalue, 1);
    g_free(newvalue);

    /* Crossfade */
    oldvalue = g_strdup_printf("%d", mhconf.status->crossfade);
    newvalue = g_strdup_printf("%d", status->crossfade);
    g_setenv("MPD_CROSSFADE_OLD", oldvalue, 1);
    g_setenv("MPD_CROSSFADE", newvalue, 1);
    g_free(oldvalue); g_free(newvalue);

    /* Elapsed time */
    oldvalue = g_strdup_printf("%d", mhconf.status->elapsedTime);
    newvalue = g_strdup_printf("%d", status->elapsedTime);
    g_setenv("MPD_ELAPSED_OLD", oldvalue, 1);
    g_setenv("MPD_ELAPSED", newvalue, 1);
    g_free(oldvalue); g_free(newvalue);

    /* Playlist */
    oldvalue = g_strdup_printf("%lld", mhconf.status->playlist);
    newvalue = g_strdup_printf("%lld", status->playlist);
    g_setenv("MPD_PLAYLIST_OLD", oldvalue, 1);
    g_setenv("MPD_PLAYLIST", newvalue, 1);
    g_free(oldvalue); g_free(newvalue);

    /* Playlist length */
    oldvalue = g_strdup_printf("%d", mhconf.status->playlistLength);
    newvalue = g_strdup_printf("%d", status->playlistLength);
    g_setenv("MPD_PLAYLIST_LENGTH_OLD", oldvalue, 1);
    g_setenv("MPD_PLAYLIST_LENGTH", newvalue, 1);
    g_free(oldvalue); g_free(newvalue);

    /* Random (boolean) */
    newvalue = g_strdup_printf("%d", status->random);
    g_setenv("MPD_RANDOM", newvalue, 1);
    g_free(newvalue);

    /* Repeat (boolean) */
    newvalue = g_strdup_printf("%d", status->repeat);
    g_setenv("MPD_REPEAT", newvalue, 1);
    g_free(newvalue);

    /* Sample Rate */
    oldvalue = g_strdup_printf("%d", mhconf.status->sampleRate);
    newvalue = g_strdup_printf("%d", status->sampleRate);
    g_setenv("MPD_SAMPLE_RATE_OLD", oldvalue, 1);
    g_setenv("MPD_SAMPLE_RATE", newvalue, 1);
    g_free(oldvalue); g_free(newvalue);

    /* Single (boolean) */
    newvalue = g_strdup_printf("%d", status->single);
    g_setenv("MPD_SINGLE", newvalue, 1);
    g_free(newvalue);

    /* State */
    oldvalue = g_strdup(mhhooker_strstate(mhconf.status->state));
    newvalue = g_strdup(mhhooker_strstate(status->state));
    g_setenv("MPD_STATE_OLD", oldvalue, 1);
    g_setenv("MPD_STATE", newvalue, 1);
    g_free(oldvalue); g_free(newvalue);

    /* Total time */
    oldvalue = g_strdup_printf("%d", mhconf.status->totalTime);
    newvalue = g_strdup_printf("%d", status->totalTime);
    g_setenv("MPD_TOTAL_TIME_OLD", oldvalue, 1);
    g_setenv("MPD_TOTAL_TIME", newvalue, 1);
    g_free(oldvalue); g_free(newvalue);

    /* Updating DB */
    oldvalue = g_strdup_printf("%d", mhconf.status->updatingDb);
    newvalue = g_strdup_printf("%d", status->updatingDb);
    g_setenv("MPD_UPDATING_DB_OLD", oldvalue, 1);
    g_setenv("MPD_UPDATING_DB", newvalue, 1);
    g_free(oldvalue); g_free(newvalue);

    /* Volume */
    oldvalue = g_strdup_printf("%d", mhconf.status->volume);
    newvalue = g_strdup_printf("%d", status->volume);
    g_setenv("MPD_VOLUME_OLD", oldvalue, 1);
    g_setenv("MPD_VOLUME", newvalue, 1);
    g_free(oldvalue); g_free(newvalue);

    if (!entity || entity->type != MPD_INFO_ENTITY_TYPE_SONG)
        return;
    if (!mhconf.entity || mhconf.entity->type != MPD_INFO_ENTITY_TYPE_SONG)
        return;

    oldsong = mhconf.entity->info.song;
    newsong = entity->info.song;

    if (!oldsong || !newsong)
        return;

    /* Filename */
    oldvalue = g_strdup(oldsong->file);
    newvalue = g_strdup(newsong->file);
    g_setenv("MPD_SONG_FILENAME_OLD", oldvalue ? oldvalue : "", 1);
    g_setenv("MPD_SONG_FILENAME", newvalue ? newvalue : "", 1);
    g_free(oldvalue); g_free(newvalue);

    /* Artist */
    oldvalue = g_strdup(oldsong->artist);
    newvalue = g_strdup(newsong->artist);
    g_setenv("MPD_SONG_ARTIST_OLD", oldvalue ? oldvalue : "", 1);
    g_setenv("MPD_SONG_ARTIST", newvalue ? newvalue : "", 1);
    g_free(oldvalue); g_free(newvalue);

    /* Album */
    oldvalue = g_strdup(oldsong->album);
    newvalue = g_strdup(newsong->album);
    g_setenv("MPD_SONG_ALBUM_OLD", oldvalue ? oldvalue : "", 1);
    g_setenv("MPD_SONG_ALBUM", newvalue ? newvalue : "", 1);
    g_free(oldvalue); g_free(newvalue);

    /* Album */
    oldvalue = g_strdup(oldsong->title);
    newvalue = g_strdup(newsong->title);
    g_setenv("MPD_SONG_TITLE_OLD", oldvalue ? oldvalue : "", 1);
    g_setenv("MPD_SONG_TITLE", newvalue ? newvalue : "", 1);
    g_free(oldvalue); g_free(newvalue);

    /* Track */
    oldvalue = g_strdup(oldsong->title);
    newvalue = g_strdup(newsong->title);
    g_setenv("MPD_SONG_TRACK_OLD", oldvalue ? oldvalue : "", 1);
    g_setenv("MPD_SONG_TRACK", newvalue ? newvalue : "", 1);
    g_free(oldvalue); g_free(newvalue);

    /* Name */
    oldvalue = g_strdup(oldsong->name);
    newvalue = g_strdup(newsong->name);
    g_setenv("MPD_SONG_NAME_OLD", oldvalue ? oldvalue : "", 1);
    g_setenv("MPD_SONG_NAME", newvalue ? newvalue : "", 1);
    g_free(oldvalue); g_free(newvalue);

    /* Date */
    oldvalue = g_strdup(oldsong->date);
    newvalue = g_strdup(newsong->date);
    g_setenv("MPD_SONG_DATE_OLD", oldvalue ? oldvalue : "", 1);
    g_setenv("MPD_SONG_DATE", newvalue ? newvalue : "", 1);
    g_free(oldvalue); g_free(newvalue);
}

void mhhooker_run(struct mhdiff diff, bool songchanged)
{
    gchar **argv;

    argv = g_malloc0(2 * sizeof(gchar *));
    if (diff.bitrate) {
        argv[0] = g_strdup(mhconf.dir.bitrate);
        mhhooker_run_hook("bitrate", argv);
        g_free(argv[0]);
    }
    if (diff.consume) {
        argv[0] = g_strdup(mhconf.dir.consume);
        mhhooker_run_hook("consume", argv);
        g_free(argv[0]);
    }
    if (diff.crossfade) {
        argv[0] = g_strdup(mhconf.dir.crossfade);
        mhhooker_run_hook("crossfade", argv);
        g_free(argv[0]);
    }
    if (diff.elapsed) {
        argv[0] = g_strdup(mhconf.dir.elapsed);
        mhhooker_run_hook("elapsed", argv);
        g_free(argv[0]);

    }
    if (diff.playlist) {
        argv[0] = g_strdup(mhconf.dir.playlist);
        mhhooker_run_hook("playlist", argv);
        g_free(argv[0]);
    }
    if (diff.playlist_length) {
        argv[0] = g_strdup(mhconf.dir.playlist_length);
        mhhooker_run_hook("playlist_length", argv);
        g_free(argv[0]);
    }
    if (diff.random) {
        argv[0] = g_strdup(mhconf.dir.random);
        mhhooker_run_hook("random", argv);
        g_free(argv[0]);
    }
    if (diff.repeat) {
        argv[0] = g_strdup(mhconf.dir.repeat);
        mhhooker_run_hook("repeat", argv);
        g_free(argv[0]);
    }
    if (diff.samplerate) {
        argv[0] = g_strdup(mhconf.dir.samplerate);
        mhhooker_run_hook("samplerate", argv);
        g_free(argv[0]);
    }
    if (diff.single) {
        argv[0] = g_strdup(mhconf.dir.single);
        mhhooker_run_hook("single", argv);
        g_free(argv[0]);
    }
    if (diff.state) {
        argv[0] = g_strdup(mhconf.dir.state);
        mhhooker_run_hook("state", argv);
        g_free(argv[0]);
    }
    if (diff.updatingdb) {
        argv[0] = g_strdup(mhconf.dir.updatingdb);
        mhhooker_run_hook("updatingdb", argv);
        g_free(argv[0]);
    }
    if (diff.volume) {
        argv[0] = g_strdup(mhconf.dir.volume);
        mhhooker_run_hook("volume", argv);
        g_free(argv[0]);
    }
    if (songchanged) {
        argv[0] = g_strdup(mhconf.dir.song);
        mhhooker_run_hook("song", argv);
        g_free(argv[0]);
    }
    g_free(argv);
}
