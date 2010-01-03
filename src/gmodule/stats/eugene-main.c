/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009-2010 Ali Polatel <alip@exherbo.org>
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
#include <gio/gio.h>
#include <mpd/client.h>
#include <sqlite3.h>

static int verbosity = LOG_DEBUG;

static void
about(void)
{
	printf("eugene-"VERSION GITHEAD "\n");
}

G_GNUC_NORETURN
static void
usage(FILE *outf, int exitval)
{
	fprintf(outf, ""
"eugene -- mpdcron statistics client\n"
"eugene COMMAND [OPTIONS]\n"
"\n"
"Commands:\n"
"help          Display help and exit\n"
"version       Display version and exit\n"
"list          List song/artist/album/genre\n"
"listinfo      List song/artist/album/genre\n"
"count         Change play count of song/artist/album/genre\n"
"hate          Hate song/artist/album/genre\n"
"love          Love song/artist/album/genre\n"
"kill          Kill song/artist/album/genre\n"
"unkill        Unkill song/artist/album/genre\n"
"rate          Rate song/artist/album/genre\n"
"addtag        Add tag to song/artist/album/genre\n"
"rmtag         Remove tag from song/artist/album/genre\n"
"listtags      List tags of song/artist/album/genre\n"
"\n"
"See eugene COMMAND --help for more information\n");
	exit(exitval);
}

static int
run_cmd(int argc, char **argv)
{
	if (strncmp(argv[0], "help", 4) == 0)
		usage(stdout, 0);
	else if (strncmp(argv[0], "version", 8) == 0) {
		about();
		return 0;
	}
	else if (strncmp(argv[0], "hate", 5) == 0)
		return cmd_hate(argc, argv);
	else if (strncmp(argv[0], "love", 5) == 0)
		return cmd_love(argc, argv);
	else if (strncmp(argv[0], "kill", 5) == 0)
		return cmd_kill(argc, argv);
	else if (strncmp(argv[0], "unkill", 7) == 0)
		return cmd_unkill(argc, argv);
	else if (strncmp(argv[0], "rate", 5) == 0)
		return cmd_rate(argc, argv);
	else if (strncmp(argv[0], "list", 5) == 0)
		return cmd_list(argc, argv);
	else if (strncmp(argv[0], "listinfo", 9) == 0)
		return cmd_listinfo(argc, argv);
	else if (strncmp(argv[0], "addtag", 7) == 0)
		return cmd_addtag(argc, argv);
	else if (strncmp(argv[0], "rmtag", 6) == 0)
		return cmd_rmtag(argc, argv);
	else if (strncmp(argv[0], "listtags", 9) == 0)
		return cmd_listtags(argc, argv);
	else if (strncmp(argv[0], "count", 6) == 0)
		return cmd_count(argc, argv);
	fprintf(stderr, "Unknown command `%s'\n", argv[0]);
	usage(stderr, 1);
}

void
eulog(int level, const char *fmt, ...)
{
	va_list args;

	if (level > verbosity)
		return;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fputc('\n', stderr);
}

int
main(int argc, char **argv)
{
	g_type_init();

	if (argc < 2)
		usage(stderr, 1);
	return run_cmd(argc - 1, argv + 1);
}
