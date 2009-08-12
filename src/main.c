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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <libdaemon/dlog.h>
#include <libdaemon/dpid.h>

#include "conf.h"
#include "keyfile.h"
#include "loop.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

static GOptionEntry options[] = {
    {"version",     'V',    0, G_OPTION_ARG_NONE, &mhconf.opt_version,  "Display version", NULL},
    {"kill",        'k',    0, G_OPTION_ARG_NONE, &mhconf.opt_kill,     "Kill the current running mpdhooker", NULL},
    {"no-daemon",   'n',    0, G_OPTION_ARG_NONE, &mhconf.opt_no_daemonize, "Don't detach from console", NULL},
};

static void about(void)
{
    printf(PACKAGE"-"VERSION);
#if defined(GITHEAD)
    if (strlen(GITHEAD) > 0)
        printf("-"GITHEAD);
#endif
    fputc('\n', stdout);
}

int main(int argc, char **argv)
{
    int ret;
    char *opt;
    GOptionContext *context;
    GError *parse_error;

    daemon_pid_file_ident = daemon_log_ident = daemon_ident_from_argv0(argv[0]);
    daemon_pid_file_proc = mhconf_pid_file_proc;
    mhconf_init();

    /* Parse command line */
    parse_error = NULL;
    context = g_option_context_new("");
    g_option_context_add_main_entries(context, options, PACKAGE);
    g_option_context_set_summary(context, PACKAGE"-"VERSION GITHEAD " - mpd hooker");

    if (! g_option_context_parse(context, &argc, &argv, &parse_error)) {
        g_printerr("option parsing failed: %s\n", parse_error->message);
        g_option_context_free(context);
        g_error_free(parse_error);
        return EXIT_FAILURE;
    }
    g_option_context_free(context);

    if (mhconf.opt_version) {
        about();
        mhconf_free();
        return EXIT_SUCCESS;
    }
    if (mhconf.opt_kill) {
        ret = daemon_pid_file_kill_wait(SIGINT, 1);
        if (ret < 0) {
            mh_log(LOG_WARNING, "Failed to kill daemon: %s", strerror(errno));
            mhconf_free();
            return EXIT_FAILURE;
        }
        daemon_pid_file_remove();
        mhconf_free();
        return EXIT_SUCCESS;
    }

    /* Parse configuration file */
    if (!mhkeyfile_load()) {
        mhconf_free();
        exit(EXIT_FAILURE);
    }

    /* Command line options to environment variables */
    if (mhconf.opt_no_daemonize)
        g_unsetenv("MHOPT_DAEMONIZE");
    else
        g_setenv("MHOPT_DAEMONIZE", "1", 1);

    /* Configuration file options to environment variables */
    opt = g_strdup_printf("%d", mhconf.poll);
    g_setenv("MHOPT_POLL", opt, 1);
    g_free(opt);

    opt = g_strdup_printf("%d", mhconf.reconnect);
    g_setenv("MHOPT_RECONNECT", opt, 1);
    g_free(opt);

    opt = g_strdup_printf("%f", mhconf.timeout);
    g_setenv("MHOPT_TIMEOUT", opt, 1);
    g_free(opt);

    g_atexit(mhconf_free);
    return mhloop_main();
}

