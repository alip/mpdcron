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

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include <glib.h>

#include <libdaemon/dfork.h>
#include <libdaemon/dsignal.h>
#include <libdaemon/dlog.h>
#include <libdaemon/dpid.h>

#include <mpd/client.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

struct homedir {
    gchar *home;
    gchar *pid;
    gchar *config;
    gchar *bitrate;
    gchar *consume;
    gchar *crossfade;
    gchar *elapsed;
    gchar *playlist;
    gchar *playlist_length;
    gchar *random;
    gchar *repeat;
    gchar *samplerate;
    gchar *single;
    gchar *song;
    gchar *state;
    gchar *updatingdb;
    gchar *volume;
};

static struct globalconf {
    bool opt_version;
    bool opt_kill;
    bool opt_no_daemonize;
    gchar **envp;
    const gchar *hostname;
    const gchar *port;
    gint poll;
    gint reconnect;
    struct mpd_connection *conn;
    struct homedir dir;
} mhconf;

static struct globalinfo {
    bool initial;
    struct mpd_status *status;
    struct mpd_entity *entity;
} mhinfo;

static GOptionEntry mhoptions[] = {
    {"version",     'V',    0, G_OPTION_ARG_NONE, &mhconf.opt_version,  "Display version", NULL},
    {"kill",        'k',    0, G_OPTION_ARG_NONE, &mhconf.opt_kill,     "Kill the current running mpdhooker", NULL},
    {"no-daemon",   'n',    0, G_OPTION_ARG_NONE, &mhconf.opt_no_daemonize, "Don't detach from console", NULL},
};

static void mh_about(void)
{
    printf(PACKAGE"-"VERSION);
#if defined(GITHEAD)
    if (strlen(GITHEAD) > 0)
        printf("-"GITHEAD);
#endif
    fputc('\n', stdout);
}

static const char *mh_strstate(int state)
{
    const char *strstate;

    switch (state) {
        case MPD_STATE_PLAY:
            strstate = "play";
            break;
        case MPD_STATE_PAUSE:
            strstate = "pause";
            break;
        case MPD_STATE_STOP:
            strstate = "stop";
            break;
        case MPD_STATE_UNKNOWN:
            strstate = "unknown";
            break;
        default:
            g_assert_not_reached();
            break;
    }
    return strstate;
}

static const gchar *mh_pid_name(void)
{
    char *name = g_strdup_printf("%s.pid", daemon_pid_file_ident);
    mhconf.dir.pid = g_build_filename(mhconf.dir.home, name, NULL);
    g_free(name);
    return mhconf.dir.pid;
}

static void mh_cleanup(void)
{
    daemon_log(LOG_INFO, "Exiting...");
    if (!mhconf.opt_no_daemonize) {
        daemon_retval_send(255);
        daemon_signal_done();
        daemon_pid_file_remove();
    }
    g_free(mhconf.dir.volume);
    g_free(mhconf.dir.updatingdb);
    g_free(mhconf.dir.state);
    g_free(mhconf.dir.song);
    g_free(mhconf.dir.single);
    g_free(mhconf.dir.samplerate);
    g_free(mhconf.dir.repeat);
    g_free(mhconf.dir.random);
    g_free(mhconf.dir.playlist_length);
    g_free(mhconf.dir.playlist);
    g_free(mhconf.dir.elapsed);
    g_free(mhconf.dir.crossfade);
    g_free(mhconf.dir.consume);
    g_free(mhconf.dir.bitrate);
    g_free(mhconf.dir.home);
    g_free(mhconf.dir.pid);
    if (NULL != mhconf.conn)
        mpd_connection_free(mhconf.conn);
    if (NULL != mhinfo.status)
        mpd_status_free(mhinfo.status);
}

static bool mh_config_load(const gchar *config_file)
{
    GKeyFile *config_fd;
    GError *config_error = NULL;

    config_fd = g_key_file_new();
    if (!g_key_file_load_from_file(config_fd, config_file, G_KEY_FILE_NONE, &config_error)) {
        daemon_log(LOG_WARNING, "Failed to parse config file `%s': %s", config_file, config_error->message);
        g_error_free(config_error);
        g_key_file_free(config_fd);
        return false;
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
    if (mhconf.poll == 0) {
        daemon_log(LOG_WARNING, "Invalid value for mpd.poll 0, setting to default");
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
    if (mhconf.reconnect == 0) {
        daemon_log(LOG_WARNING, "Invalid value for mpd.reconnect 0, setting to default");
        mhconf.reconnect = 30;
    }

    g_key_file_free(config_fd);
    return true;
}

static void mh_signal(void)
{
    int fd, sig;
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    fd = daemon_signal_fd();
    FD_SET(fd, &fds);

    tv.tv_sec = 0;
    tv.tv_usec = 100;

    /* Wait for an incoming signal */
    if (select(FD_SETSIZE, &fds, 0, 0, &tv) < 0) {
        if (errno != EINTR)
            daemon_log(LOG_WARNING, "select() failed: %s", strerror(errno));
        return;
    }

    /* Check if a signal has been received */
    if (FD_ISSET(fd, &fds)) {
        /* Get signal */
        sig = daemon_signal_next();
        if (sig <= 0) {
            daemon_log(LOG_WARNING, "daemon_signal_next() failed: %s", strerror(errno));
            return;
        }

        /* Dispatch signal */
        switch (sig) {
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
                daemon_log(LOG_WARNING, "Got SIGINT, SIGQUIT or SIGTERM");
                exit(EXIT_FAILURE);
                break;
            case SIGHUP:
                daemon_log(LOG_INFO, "Got SIGHUP, reloading configuration file");
                mh_config_load(mhconf.dir.config);
                break;
            default:
                g_assert_not_reached();
                break;
        }
    }
}

static void mh_connect_block(void)
{
    gint try = 1;
    if (NULL == mhconf.hostname) {
        mhconf.hostname = g_getenv("MPD_HOST");
        if (NULL == mhconf.hostname)
            mhconf.hostname = "localhost";
    }
    if (NULL == mhconf.port) {
        mhconf.port = g_getenv("MPD_PORT");
        if (NULL == mhconf.port)
            mhconf.port = "6600";
    }

    for (;;) {
        daemon_log(LOG_INFO, "Connecting to mpd server `%s' on port %s (try: %d)", mhconf.hostname, mhconf.port, try++);
        mhconf.conn = mpd_connection_new(mhconf.hostname, atoi(mhconf.port), 10);
        if (!mhconf.conn || mpd_get_error(mhconf.conn) != MPD_ERROR_SUCCESS) {
            daemon_log(LOG_ERR, "Failed to connect: %s", mpd_get_error_message(mhconf.conn));
            mpd_connection_free(mhconf.conn);
            daemon_log(LOG_INFO, "Sleeping for %d seconds until next connection try", mhconf.reconnect);
            sleep(mhconf.reconnect);
            continue;
        }
        daemon_log(LOG_INFO, "Successfully connected to mpd server `%s' on port %s", mhconf.hostname, mhconf.port);
        break;
    }
}

static gint mh_run_hook(const char *name, gchar **argv)
{
    gint pid;
    GError *hook_error = NULL;

    if (!g_spawn_async(mhconf.dir.home, argv, mhconf.envp,
            G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_CHILD_INHERITS_STDIN,
            NULL, NULL, &pid, &hook_error)) {
        daemon_log(LOG_WARNING, "Failed to execute hook `%s': %s", name, hook_error->message);
        g_error_free(hook_error);
        return -1;
    }
    return 0;
}

static gint mh_hooker(void)
{
    gchar **argv;
    gint oldvalue, newvalue;
    long long oldvalue_long, newvalue_long;
    const gchar *oldfile, *newfile;
    struct mpd_status *status;
    struct mpd_song *song, *oldsong;
    struct mpd_entity *entity;

    mpd_send_status(mhconf.conn);
    if (mpd_get_error(mhconf.conn) != MPD_ERROR_SUCCESS) {
        daemon_log(LOG_ERR, "Connection error: %s", mpd_get_error_message(mhconf.conn));
        return -1;
    }

    status = mpd_get_status(mhconf.conn);
    if (!status) {
        daemon_log(LOG_ERR, "Failed to get status information: %s", mpd_get_error_message(mhconf.conn));
        return -1;
    }
    else if (mpd_status_get_error(status)) {
        daemon_log(LOG_ERR, "Status error: %s", mpd_status_get_error(status));
        return -1;
    }

    if (mhinfo.initial || !mhinfo.status)
        daemon_log(LOG_DEBUG, "Status information not available, not comparing data");
    else {
        newvalue = mpd_status_get_bit_rate(status);
        oldvalue = mpd_status_get_bit_rate(mhinfo.status);
        if (newvalue != oldvalue) {
            argv = g_malloc0(4 * sizeof(gchar *));
            argv[0] = g_strdup(mhconf.dir.bitrate);
            argv[1] = g_strdup_printf("%d", oldvalue);
            argv[2] = g_strdup_printf("%d", newvalue);
            mh_run_hook("bitrate", argv);
            for (int i = 0; i < 3; i++)
                g_free(argv[i]);
            g_free(argv);
        }

        newvalue = mpd_status_get_consume(status);
        oldvalue = mpd_status_get_consume(mhinfo.status);
        if (newvalue != oldvalue) {
            argv = g_malloc0(3 * sizeof(gchar *));
            argv[0] = g_strdup(mhconf.dir.consume);
            argv[1] = g_strdup_printf("%d", newvalue);
            mh_run_hook("consume", argv);
            for (int i = 0; i < 2; i++)
                g_free(argv[i]);
            g_free(argv);
        }

        newvalue = mpd_status_get_crossfade(status);
        oldvalue = mpd_status_get_crossfade(mhinfo.status);
        if (newvalue != oldvalue) {
            argv = g_malloc0(4 * sizeof(gchar *));
            argv[0] = g_strdup(mhconf.dir.crossfade);
            argv[1] = g_strdup_printf("%d", oldvalue);
            argv[2] = g_strdup_printf("%d", newvalue);
            mh_run_hook("crossfade", argv);
            for (int i = 0; i < 3; i++)
                g_free(argv[i]);
            g_free(argv);
        }

        if (mpd_status_get_state(status) == MPD_STATE_PLAY ||
                mpd_status_get_state(status) == MPD_STATE_PAUSE) {
            newvalue = mpd_status_get_elapsed_time(status);
            oldvalue = mpd_status_get_elapsed_time(mhinfo.status);
            if (newvalue != oldvalue) {
                argv = g_malloc0(5 * sizeof(gchar *));
                argv[0] = g_strdup(mhconf.dir.elapsed);
                argv[1] = g_strdup_printf("%d", oldvalue);
                argv[2] = g_strdup_printf("%d", newvalue);
                argv[3] = g_strdup_printf("%d", mpd_status_get_total_time(status));
                mh_run_hook("elapsed", argv);
                for (int i = 0; i < 4; i++)
                    g_free(argv[i]);
                g_free(argv);
            }
        }

        newvalue_long = mpd_status_get_playlist(status);
        oldvalue_long = mpd_status_get_playlist(mhinfo.status);
        if (newvalue_long != oldvalue_long) {
            argv = g_malloc0(4 * sizeof(gchar *));
            argv[0] = g_strdup(mhconf.dir.playlist);
            argv[1] = g_strdup_printf("%lld", oldvalue_long);
            argv[2] = g_strdup_printf("%lld", newvalue_long);
            mh_run_hook("playlist", argv);
            for (int i = 0; i < 3; i++)
                g_free(argv[i]);
            g_free(argv);
        }

        newvalue = mpd_status_get_playlist_length(status);
        oldvalue = mpd_status_get_playlist_length(mhinfo.status);
        if (newvalue != oldvalue) {
            argv = g_malloc0(4 * sizeof(gchar *));
            argv[0] = g_strdup(mhconf.dir.playlist_length);
            argv[1] = g_strdup_printf("%d", oldvalue);
            argv[2] = g_strdup_printf("%d", newvalue);
            mh_run_hook("playlist_length", argv);
            for (int i = 0; i < 3; i++)
                g_free(argv[i]);
            g_free(argv);
        }

        newvalue = mpd_status_get_random(status);
        oldvalue = mpd_status_get_random(mhinfo.status);
        if (newvalue != oldvalue) {
            argv = g_malloc0(3 * sizeof(gchar *));
            argv[0] = g_strdup(mhconf.dir.random);
            argv[1] = g_strdup_printf("%d", newvalue);
            mh_run_hook("random", argv);
            for (int i = 0; i < 2; i++)
                g_free(argv[i]);
            g_free(argv);
        }

        newvalue = mpd_status_get_repeat(status);
        oldvalue = mpd_status_get_repeat(mhinfo.status);
        if (newvalue != oldvalue) {
            argv = g_malloc0(3 * sizeof(gchar *));
            argv[0] = g_strdup(mhconf.dir.repeat);
            argv[1] = g_strdup_printf("%d", newvalue);
            mh_run_hook("repeat", argv);
            for (int i = 0; i < 2; i++)
                g_free(argv[i]);
            g_free(argv);
        }

        newvalue = mpd_status_get_sample_rate(status);
        oldvalue = mpd_status_get_sample_rate(mhinfo.status);
        if (newvalue != oldvalue) {
            argv = g_malloc0(4 * sizeof(gchar *));
            argv[0] = g_strdup(mhconf.dir.samplerate);
            argv[1] = g_strdup_printf("%d", oldvalue);
            argv[2] = g_strdup_printf("%d", newvalue);
            mh_run_hook("samplerate", argv);
            for (int i = 0; i < 3; i++)
                g_free(argv[i]);
            g_free(argv);
        }

        newvalue = mpd_status_get_single(status);
        oldvalue = mpd_status_get_single(mhinfo.status);
        if (newvalue != oldvalue) {
            argv = g_malloc0(3 * sizeof(gchar *));
            argv[0] = g_strdup(mhconf.dir.single);
            argv[1] = g_strdup_printf("%d", newvalue);
            mh_run_hook("single", argv);
            for (int i = 0; i < 2; i++)
                g_free(argv[i]);
            g_free(argv);
        }

        newvalue = mpd_status_get_state(status);
        oldvalue = mpd_status_get_state(mhinfo.status);
        if (newvalue != oldvalue) {
            const char *newstate = mh_strstate(newvalue);
            const char *oldstate = mh_strstate(oldvalue);
            argv = g_malloc0(4 * sizeof(gchar *));
            argv[0] = g_strdup(mhconf.dir.state);
            argv[1] = g_strdup(oldstate);
            argv[2] = g_strdup(newstate);
            mh_run_hook("state", argv);
            for (int i = 0; i < 3; i++)
                g_free(argv[i]);
            g_free(argv);
        }

        newvalue = mpd_status_get_updatingdb(status);
        oldvalue = mpd_status_get_updatingdb(mhinfo.status);
        if (newvalue != oldvalue) {
            argv = g_malloc0(3 * sizeof(gchar *));
            argv[0] = g_strdup(mhconf.dir.updatingdb);
            argv[1] = g_strdup_printf("%d", newvalue);
            mh_run_hook("updatingdb", argv);
            for (int i = 0; i < 2; i++)
                g_free(argv[i]);
            g_free(argv);
        }

        newvalue = mpd_status_get_volume(status);
        oldvalue = mpd_status_get_volume(mhinfo.status);
        if (newvalue != oldvalue) {
            argv = g_malloc0(4 * sizeof(gchar *));
            argv[0] = g_strdup(mhconf.dir.volume);
            argv[1] = g_strdup_printf("%d", oldvalue);
            argv[2] = g_strdup_printf("%d", newvalue);
            mh_run_hook("volume", argv);
            for (int i = 0; i < 3; i++)
                g_free(argv[i]);
            g_free(argv);
        }
    }
    mpd_response_finish(mhconf.conn);

    /* Get current song */
    mpd_send_currentsong(mhconf.conn);
    if (mpd_get_error(mhconf.conn) != MPD_ERROR_SUCCESS) {
        daemon_log(LOG_ERR, "Connection error: %s", mpd_get_error_message(mhconf.conn));
        goto finish;
    }

    entity = mpd_get_next_entity(mhconf.conn);
    if (entity) {
        song = entity->info.song;
        if (entity->type != MPD_ENTITY_TYPE_SONG || !song) {
            daemon_log(LOG_ERR, "Entity doesn't have the expected type `song': %d", entity->type);
            mpd_entity_free(entity);
            mpd_response_finish(mhconf.conn);
            goto finish;
        }
        else if (mhinfo.initial || !mhinfo.entity) 
            daemon_log(LOG_DEBUG, "Song entity information not available, not comparing data");
        else {
            oldsong = mhinfo.entity->info.song;
            oldfile = mpd_song_get_tag(oldsong, MPD_TAG_FILENAME, 0);
            newfile = mpd_song_get_tag(song, MPD_TAG_FILENAME, 0);
            if (strcmp(oldfile, newfile) != 0) {
                argv = g_malloc0(4 * sizeof(gchar *));
                argv[0] = g_strdup(mhconf.dir.song);
                argv[1] = g_strdup(oldfile);
                argv[2] = g_strdup(newfile);
                mh_run_hook("song", argv);
                for (int i = 0; i < 3; i++)
                    g_free(argv[i]);
                g_free(argv);
            }
        }
    }
    mpd_response_finish(mhconf.conn);

finish:
    if (mhinfo.status)
        mpd_status_free(mhinfo.status);
    if (mhinfo.entity)
        mpd_entity_free(mhinfo.entity);
    mhinfo.status = status;
    mhinfo.entity = entity;
    return 0;
}

G_GNUC_NORETURN static void mh_loop(void)
{
    mh_connect_block();
    for (;;) {
        mh_signal();
        if (0 > mh_hooker()) {
            if (NULL != mhconf.conn)
                mpd_connection_free(mhconf.conn);
            mh_connect_block();
            continue;
        }
        else if (mhinfo.initial)
            mhinfo.initial = false;
        sleep(mhconf.poll);
    }
}

int main(int argc, char **argv, char **environ)
{
    int ret;
    pid_t pid;
    GError *parse_error = NULL;
    GOptionContext *context = NULL;

    /* Reset signal handlers */
    if (daemon_reset_sigs(-1) < 0) {
        daemon_log(LOG_ERR, "Failed to reset all signal handlers: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    /* Unblock signals */
    if (daemon_unblock_sigs(-1) < 0) {
        daemon_log(LOG_ERR, "Failed to unblock all signals: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    daemon_pid_file_ident = daemon_log_ident = daemon_ident_from_argv0(argv[0]);
    daemon_pid_file_proc = mh_pid_name;

    memset(&mhconf, 0, sizeof(struct globalconf));
    memset(&mhinfo, 0, sizeof(struct globalinfo));
    mhconf.poll = 1;
    mhconf.reconnect = 30;
    mhconf.envp = environ;
    mhinfo.initial = true;

    /* Get home directory */
    if (g_getenv("MPD_HOOKER_DIR"))
        mhconf.dir.home = g_strdup(g_getenv("MPD_HOOKER_DIR"));
    else if (g_getenv("HOME"))
        mhconf.dir.home = g_build_filename(g_getenv("HOME"), ".mpdhooker", NULL);
    else {
        daemon_log(LOG_ERR, "Neither MPD_HOOKER_DIR nor HOME is set!");
        return EXIT_FAILURE;
    }

    /* Parse options */
    context = g_option_context_new("");
    g_option_context_add_main_entries(context, mhoptions, PACKAGE);
    g_option_context_set_summary(context, PACKAGE"-"VERSION GIT_HEAD " - mpd hooker");

    if (! g_option_context_parse(context, &argc, &argv, &parse_error)) {
        g_printerr("option parsing failed: %s\n", parse_error->message);
        g_option_context_free(context);
        g_error_free(parse_error);
        return EXIT_FAILURE;
    }
    g_option_context_free(context);

    if (mhconf.opt_version) {
        mh_about();
        g_free(mhconf.dir.home);
        return EXIT_SUCCESS;
    }

    if (mhconf.opt_kill) {
        /* Kill the deamon */
        ret = daemon_pid_file_kill_wait(SIGINT, 1);
        if (ret < 0) {
            daemon_log(LOG_WARNING, "Failed to kill daemon: %s", strerror(errno));
            return EXIT_FAILURE;
        }
        daemon_pid_file_remove();
        g_free(mhconf.dir.home);
        return EXIT_SUCCESS;
    }

    /* Build script paths */
    mhconf.dir.config = g_build_filename(mhconf.dir.home, "mpdhooker.conf", NULL);
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

    mh_config_load(mhconf.dir.config);

    /* Initialize signal handling */
    if (daemon_signal_init(SIGINT, SIGTERM, SIGQUIT, SIGHUP, 0) < 0) {
        daemon_log(LOG_ERR, "Failed to register signal handlers: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    if (mhconf.opt_no_daemonize) {
        g_atexit(mh_cleanup);
        mh_loop();
        return EXIT_SUCCESS;
    }

    /* Check if the deamon is already running */
    pid = daemon_pid_file_is_running();
    if (pid >= 0) {
         daemon_log(LOG_ERR, "Daemon already running on PID %u", pid);
         return EXIT_FAILURE;
    }

    daemon_retval_init();
    pid = daemon_fork();
    if (pid < 0) {
        daemon_log(LOG_ERR, "Failed to fork: %s", strerror(errno));
        daemon_retval_done();
        return EXIT_FAILURE;
    }
    else if (pid != 0) { /* Parent */
        ret = daemon_retval_wait(2);
        if (ret < 0) {
            daemon_log(LOG_ERR, "Could not recieve return value from daemon process: %s", strerror(errno));
            return 255;
        }

        daemon_log(ret != 0 ? LOG_ERR : LOG_INFO, "Daemon returned %i as return value.", ret);
        return ret;
    }
    else { /* Daemon */
        g_atexit(mh_cleanup);

        /* Close FDs */
        if (daemon_close_all(-1) < 0) {
            daemon_log(LOG_ERR, "Failed to close all file descriptors: %s", strerror(errno));

            /* Send the error condition to the parent process */
            daemon_retval_send(1);
            return EXIT_FAILURE;
        }

        /* Create the PID file */
        if (daemon_pid_file_create() < 0) {
            daemon_log(LOG_ERR, "Failed to create PID file: %s", strerror(errno));
            daemon_retval_send(2);
            return EXIT_FAILURE;
        }

        /* Send OK to parent process */
        daemon_retval_send(0);
        daemon_log(LOG_INFO, "Successfully started");
        mh_loop();
    }
    return EXIT_SUCCESS;
}

