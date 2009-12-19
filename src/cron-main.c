/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet ai cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
 *
 * This file is part of the mpdcron mpd client. mpdcron is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * mpdcron is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "cron-defs.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <libdaemon/dfork.h>
#include <libdaemon/dlog.h>
#include <libdaemon/dpid.h>

#include <mpd/client.h>

int optnd = 0;
GMainLoop *loop = NULL;
static int optv, optk;

static GOptionEntry options[] = {
	{"version", 'V', 0, G_OPTION_ARG_NONE, &optv, "Display version", NULL},
	{"kill", 'k', 0, G_OPTION_ARG_NONE, &optk, "Kill daemon", NULL},
	{"no-daemon", 'n', 0, G_OPTION_ARG_NONE, &optnd, "Don't detach from console", NULL},
};

static void about(void)
{
	printf(PACKAGE"-"VERSION GITHEAD "\n");
}

int main(int argc, char **argv)
{
	int pid, ret;
	char *optstr;
	GOptionContext *ctx;
	GError *parse_err = NULL;

	daemon_pid_file_ident = daemon_log_ident = daemon_ident_from_argv0(argv[0]);
	daemon_pid_file_proc = conf_pid_file_proc;
	if (conf_init() < 0)
		return EXIT_FAILURE;

	ctx = g_option_context_new("");
	g_option_context_add_main_entries(ctx, options, PACKAGE);
	g_option_context_set_summary(ctx, PACKAGE"-"VERSION GITHEAD" - mpd cron daemon");

	if (!g_option_context_parse(ctx, &argc, &argv, &parse_err)) {
		g_printerr("option parsing failed: %s\n", parse_err->message);
		g_option_context_free(ctx);
		g_error_free(parse_err);
		return EXIT_FAILURE;
	}
	g_option_context_free(ctx);

	if (optv) {
		about();
		conf_free();
		return EXIT_SUCCESS;
	}

	/* Important! Parse configuration file before killing the daemon
	 * because the configuration file has a pidfile and killwait option.
	 */
	if (keyfile_load() < 0) {
		conf_free();
		return EXIT_FAILURE;
	}

	if (optk) {
		if (daemon_pid_file_kill_wait(SIGINT, killwait) < 0) {
			crlog(LOG_WARNING, "Failed to kill daemon: %s", strerror(errno));
			conf_free();
			return EXIT_FAILURE;
		}
		daemon_pid_file_remove();
		conf_free();
		return EXIT_SUCCESS;
	}

	/* Version to environment variable */
	g_setenv("MPDCRON_PACKAGE", PACKAGE, 1);
	g_setenv("MPDCRON_VERSION", VERSION, 1);
	g_setenv("MPDCRON_GITHEAD", GITHEAD, 1);

	/* Command line options to environment variables */
	if (optnd)
		g_unsetenv("MCOPT_DAEMONIZE");
	else
		g_setenv("MCOPT_DAEMONIZE", "1", 1);

	/* Configuration file options to environment variables */
	optstr = g_strdup_printf("%d", reconnect);
	g_setenv("MCOPT_RECONNECT", optstr, 1);
	g_free(optstr);

	optstr = g_strdup_printf("%d", timeout);
	g_setenv("MCOPT_TIMEOUT", optstr, 1);
	g_free(optstr);

	/* Call mhconf_free() on exit to free allocated data */
	g_atexit(conf_free);

	if (optnd) {
		/* Connect and start the main loop */
		loop_connect();
		loop = g_main_loop_new(NULL, FALSE);
		g_main_loop_run(loop);
		return EXIT_SUCCESS;
	}

	/* Daemonize */
	if ((pid = daemon_pid_file_is_running()) > 0) {
		crlog(LOG_ERR, "Daemon already running on PID %u", pid);
		return EXIT_FAILURE;
	}

	daemon_retval_init();
	pid = daemon_fork();
	if (pid < 0) {
		crlog(LOG_ERR, "Failed to fork: %s", strerror(errno));
		daemon_retval_done();
		return EXIT_FAILURE;
	}
	else if (pid != 0) { /* Parent */
		if ((ret = daemon_retval_wait(2)) < 0) {
			crlog(LOG_ERR, "Could not receive return value from daemon process: %s",
					strerror(errno));
			return 255;
		}

		crlog((ret != 0) ? LOG_ERR : LOG_INFO, "Daemon returned %i as return value", ret);
		return ret;
	}
	else { /* Daemon */
		if (daemon_close_all(-1) < 0) {
			crlog(LOG_ERR, "Failed to close all file descriptors: %s",
					strerror(errno));
			daemon_retval_send(1);
			return EXIT_FAILURE;
		}

		if (daemon_pid_file_create() < 0) {
			crlog(LOG_ERR, "Failed to create PID file: %s", strerror(errno));
			daemon_retval_send(2);
			return EXIT_FAILURE;
		}

		/* Send OK to parent process */
		daemon_retval_send(0);
		/* Connect and start the main loop */
		loop_connect();
		loop = g_main_loop_new(NULL, FALSE);
		g_main_loop_run(loop);
		return EXIT_SUCCESS;
	}
	return EXIT_SUCCESS;
}
