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

#include "eugene-defs.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <glib.h>

struct eu_config euconfig;

void eulog(int level, const char *fmt, ...)
{
	va_list args;

	if (level > euconfig.verbosity)
		return;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fputc('\n', stderr);
}

static void about(void)
{
	printf("eugene-"VERSION GITHEAD "\n");
}

G_GNUC_NORETURN
static void usage(FILE *outf, int exitval)
{
	fprintf(outf, ""
"eugene -- mpdcron statistics client\n"
"eugene COMMAND [OPTIONS]\n"
"\n"
"Options:\n"
"-h, --help    Display help and exit\n"
"-V, --version Display version and exit\n"
"Commands:\n"
"update        Update database\n"
"love          Love song\n"
"hate          Hate song\n"
"\n"
"See eugene COMMAND --help for more information\n");
	exit(exitval);
}

static int run_cmd(const char *name, int argc, char **argv)
{
	/* Parse common data */
	memset(&euconfig, 0, sizeof(struct eu_config));
	euconfig.verbosity = LOG_NOTICE;
	if ((euconfig.hostname = g_getenv("MPD_HOST")) == NULL)
		euconfig.hostname = "localhost";
	if ((euconfig.port = g_getenv("MPD_PORT")) == NULL)
		euconfig.port = "6600";
	euconfig.password = g_getenv("MPD_PASSWORD");

	if (strncmp(name, "update", 7) == 0)
		return cmd_update(argc, argv);
	else if (strncmp(name, "love", 10) == 0)
		return cmd_love_song(argc, argv);
	else if (strncmp(name, "hate", 10) == 0)
		return cmd_hate_song(argc, argv);
	fprintf(stderr, "eugene: Unknown command `%s'\n", name);
	usage(stderr, 1);
}

int main(int argc, char **argv)
{
	if (argc < 2)
		usage(stderr, 1);
	else if (strncmp(argv[1], "help", 4) == 0)
		usage(stdout, 0);
	else if (strncmp(argv[1], "version", 8) == 0) {
		about();
		return 0;
	}

	return run_cmd(argv[1], argc - 1, argv + 1);
}
