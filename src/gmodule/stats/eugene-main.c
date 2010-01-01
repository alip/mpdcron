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

#define ENV_MPDCRON_HOST	"MPDCRON_HOST"
#define ENV_MPDCRON_PORT	"MPDCRON_PORT"
#define ENV_MPDCRON_PASSWORD	"MPDCRON_PASSWORD"
#define DEFAULT_HOSTNAME	"localhost"
#define DEFAULT_PORT		6601

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
"hate          Hate song/artist/album/genre\n"
"love          Love song/artist/album/genre\n"
"kill          Kill song/artist/album/genre\n"
"unkill        Unkill song/artist/album/genre\n"
"rate          Rate song/artist/album/genre\n"
"\n"
"See eugene COMMAND --help for more information\n");
	exit(exitval);
}

static char *
quote(const char *src)
{
	const char *p;
	GString *dest;

	g_return_val_if_fail(src != NULL, NULL);

	dest = g_string_new("'");
	p = src;

	while (*p != 0) {
		if (*p == '\'')
			g_string_append(dest, "''");
		else
			g_string_append_c(dest, *p);
		++p;
	}
	g_string_append_c(dest, '\'');
	return g_string_free(dest, FALSE);
}

static struct mpd_song *
load_current_song(void)
{
	int port;
	const char *hostname, *password;
	struct mpd_connection *conn;
	struct mpd_song *song;

	hostname = g_getenv(ENV_MPD_HOST)
		? g_getenv(ENV_MPD_HOST)
		: "localhost";
	port = g_getenv(ENV_MPD_PORT)
		? atoi(g_getenv(ENV_MPD_PORT))
		: 6600;
	password = g_getenv(ENV_MPD_PASSWORD);

	if ((conn = mpd_connection_new(hostname, port, 0)) == NULL) {
		eulog(LOG_ERR, "Error creating mpd connection: out of memory");
		return NULL;
	}

	if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
		eulog(LOG_ERR, "Failed to connect to Mpd: %s",
			mpd_connection_get_error_message(conn));
		mpd_connection_free(conn);
		return NULL;
	}

	if (password != NULL) {
		if (!mpd_run_password(conn, password)) {
			eulog(LOG_ERR, "Authentication failed: %s",
				mpd_connection_get_error_message(conn));
			mpd_connection_free(conn);
			return NULL;
		}
	}

	if ((song = mpd_run_current_song(conn)) == NULL) {
		if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
			eulog(LOG_ERR, "Failed to get current song: %s",
				mpd_connection_get_error_message(conn));
			mpd_connection_free(conn);
			return NULL;
		}
		eulog(LOG_WARNING, "No song playing at the moment");
		mpd_connection_free(conn);
		return NULL;
	}

	mpd_connection_free(conn);
	return song;
}

static int
love_artist(struct mpdcron_connection *conn, bool love, const char *expr)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_love_artist_expr(conn, love, expr, &values)) {
			eulog(LOG_ERR, "Failed to %s artist: %s",
					love ? "love" : "hate",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_artist, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no artist tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_artist = quote(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
		myexpr = g_strdup_printf("name=%s", esc_artist);
		g_free(esc_artist);
		mpd_song_free(song);

		if (!mpdcron_love_artist_expr(conn, love, myexpr, &values)) {
			eulog(LOG_ERR, "Failed to %s current playing artist: %s",
					love ? "love" : "hate",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: Love:%d %s\n", e->id, e->love, e->name);
		g_free(e->name);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
love_album(struct mpdcron_connection *conn, bool love, const char *expr)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_love_album_expr(conn, love, expr, &values)) {
			eulog(LOG_ERR, "Failed to %s album: %s",
					love ? "love" : "hate",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_album, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no album tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_album = quote(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
		myexpr = g_strdup_printf("name=%s", esc_album);
		g_free(esc_album);
		mpd_song_free(song);

		if (!mpdcron_love_album_expr(conn, love, myexpr, &values)) {
			eulog(LOG_ERR, "Failed to %s current playing album: %s",
					love ? "love" : "hate",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: Love:%d %s\n", e->id, e->love, e->name);
		g_free(e->name);
		g_free(e->artist); /* TODO: We don't print artist right now. */
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
love_genre(struct mpdcron_connection *conn, bool love, const char *expr)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_love_genre_expr(conn, love, expr, &values)) {
			eulog(LOG_ERR, "Failed to %s genre: %s",
					love ? "love" : "hate",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_genre, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_GENRE, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no genre tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_genre = quote(mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
		myexpr = g_strdup_printf("name=%s", esc_genre);
		g_free(esc_genre);
		mpd_song_free(song);

		if (!mpdcron_love_genre_expr(conn, love, myexpr, &values)) {
			eulog(LOG_ERR, "Failed to %s current playing genre: %s",
					love ? "love" : "hate",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: Love:%d %s\n", e->id, e->love, e->name);
		g_free(e->name);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
love_song(struct mpdcron_connection *conn, bool love, const char *expr)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_love_expr(conn, love, expr, &values)) {
			eulog(LOG_ERR, "Failed to %s song: %s",
					love ? "love" : "hate",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_uri, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;

		esc_uri = quote(mpd_song_get_uri(song));
		myexpr = g_strdup_printf("uri=%s", esc_uri);
		g_free(esc_uri);
		mpd_song_free(song);

		if (!mpdcron_love_expr(conn, love, myexpr, &values)) {
			eulog(LOG_ERR, "Failed to %s current playing song: %s",
					love ? "love" : "hate",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_song *e = walk->data;
		printf("%d: Love:%d %s\n", e->id, e->love, e->uri);
		g_free(e->uri);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
cmd_love_internal(bool love, const char *expr, bool artist, bool album,
		bool genre)
{
	int port, ret;
	const char *hostname, *password;
	struct mpdcron_connection *conn;

	if ((artist && album && genre) ||
			(artist && album) ||
			(artist && genre) ||
			(album && genre)) {
		g_printerr("--artist, --album and --genre options are mutually exclusive\n");
		return 1;
	}

	hostname = g_getenv(ENV_MPDCRON_HOST)
		? g_getenv(ENV_MPDCRON_HOST)
		: DEFAULT_HOSTNAME;
	port = g_getenv(ENV_MPDCRON_PORT)
		? atoi(g_getenv(ENV_MPDCRON_PORT))
		: DEFAULT_PORT;
	password = g_getenv(ENV_MPDCRON_PASSWORD);

	conn = mpdcron_connection_new(hostname, port);
	if (conn->error != NULL) {
		eulog(LOG_ERR, "Failed to connect: %s", conn->error->message);
		mpdcron_connection_free(conn);
		return 1;
	}

	if (password != NULL) {
		if (!mpdcron_password(conn, password)) {
			eulog(LOG_ERR, "Authentication failed: %s", conn->error->message);
			mpdcron_connection_free(conn);
			return 1;
		}
	}

	if (artist)
		ret = love_artist(conn, love, expr);
	else if (album)
		ret = love_album(conn, love, expr);
	else if (genre)
		ret = love_genre(conn, love, expr);
	else
		ret = love_song(conn, love, expr);
	mpdcron_connection_free(conn);
	return ret;
}

static int
kill_artist(struct mpdcron_connection *conn, bool kkill, const char *expr)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_kill_artist_expr(conn, kkill, expr, &values)) {
			eulog(LOG_ERR, "Failed to %s artist: %s",
					kkill ? "kill" : "unkill",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_artist, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no artist tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_artist = quote(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
		myexpr = g_strdup_printf("name=%s", esc_artist);
		g_free(esc_artist);
		mpd_song_free(song);

		if (!mpdcron_kill_artist_expr(conn, kkill, myexpr, &values)) {
			eulog(LOG_ERR, "Failed to %s current playing artist: %s",
					kkill ? "kill" : "unkill",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: Kill:%d %s\n", e->id, e->kill, e->name);
		g_free(e->name);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
kill_album(struct mpdcron_connection *conn, bool kkill, const char *expr)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_kill_album_expr(conn, kkill, expr, &values)) {
			eulog(LOG_ERR, "Failed to %s album: %s",
					kkill ? "kill" : "unkill",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_album, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no album tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_album = quote(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
		myexpr = g_strdup_printf("name=%s", esc_album);
		g_free(esc_album);
		mpd_song_free(song);

		if (!mpdcron_kill_album_expr(conn, kkill, myexpr, &values)) {
			eulog(LOG_ERR, "Failed to %s current playing album: %s",
					kkill ? "kill" : "unkill",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: Kill:%d %s\n", e->id, e->kill, e->name);
		g_free(e->name);
		g_free(e->artist); /* TODO: We don't print artist right now */
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
kill_genre(struct mpdcron_connection *conn, bool kkill, const char *expr)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_kill_genre_expr(conn, kkill, expr, &values)) {
			eulog(LOG_ERR, "Failed to %s genre: %s",
					kkill ? "kill" : "unkill",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_genre, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_GENRE, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no genre tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_genre = quote(mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
		myexpr = g_strdup_printf("name=%s", esc_genre);
		g_free(esc_genre);
		mpd_song_free(song);

		if (!mpdcron_kill_genre_expr(conn, kkill, myexpr, &values)) {
			eulog(LOG_ERR, "Failed to %s current playing genre: %s",
					kkill ? "kill" : "unkill",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: Kill:%d %s\n", e->id, e->kill, e->name);
		g_free(e->name);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
kill_song(struct mpdcron_connection *conn, bool kkill, const char *expr)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_kill_expr(conn, kkill, expr, &values)) {
			eulog(LOG_ERR, "Failed to %s song: %s",
					kkill ? "kill" : "unkill",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_uri, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;

		esc_uri = quote(mpd_song_get_uri(song));
		myexpr = g_strdup_printf("uri=%s", esc_uri);
		g_free(esc_uri);
		mpd_song_free(song);

		if (!mpdcron_kill_expr(conn, kkill, myexpr, &values)) {
			eulog(LOG_ERR, "Failed to %s current playing song: %s",
					kkill ? "kill" : "unkill",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_song *e = walk->data;
		printf("%d: Kill:%d %s\n", e->id, e->kill, e->uri);
		g_free(e->uri);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
cmd_kill_internal(bool kkill, const char *expr, bool artist, bool album,
		bool genre)
{
	int port, ret;
	const char *hostname, *password;
	struct mpdcron_connection *conn;

	if ((artist && album && genre) ||
			(artist && album) ||
			(artist && genre) ||
			(album && genre)) {
		g_printerr("--artist, --album and --genre options are mutually exclusive\n");
		return 1;
	}

	hostname = g_getenv(ENV_MPDCRON_HOST)
		? g_getenv(ENV_MPDCRON_HOST)
		: DEFAULT_HOSTNAME;
	port = g_getenv(ENV_MPDCRON_PORT)
		? atoi(g_getenv(ENV_MPDCRON_PORT))
		: DEFAULT_PORT;
	password = g_getenv(ENV_MPDCRON_PASSWORD);

	conn = mpdcron_connection_new(hostname, port);
	if (conn->error != NULL) {
		eulog(LOG_ERR, "Failed to connect: %s", conn->error->message);
		mpdcron_connection_free(conn);
		return 1;
	}

	if (password != NULL) {
		if (!mpdcron_password(conn, password)) {
			eulog(LOG_ERR, "Authentication failed: %s", conn->error->message);
			mpdcron_connection_free(conn);
			return 1;
		}
	}

	if (artist)
		ret = kill_artist(conn, kkill, expr);
	else if (album)
		ret = kill_album(conn, kkill, expr);
	else if (genre)
		ret = kill_genre(conn, kkill, expr);
	else
		ret = kill_song(conn, kkill, expr);
	mpdcron_connection_free(conn);
	return ret;
}

static int
rate_artist(struct mpdcron_connection *conn, const char *expr,
		const char *rating)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_rate_artist_expr(conn, expr, rating, &values)) {
			eulog(LOG_ERR, "Failed to rate artist: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_artist, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no artist tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_artist = quote(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
		myexpr = g_strdup_printf("name=%s", esc_artist);
		g_free(esc_artist);
		mpd_song_free(song);

		if (!mpdcron_rate_artist_expr(conn, myexpr, rating, &values)) {
			eulog(LOG_ERR, "Failed to rate current playing artist: %s",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: Rating:%d %s\n", e->id, e->rating, e->name);
		g_free(e->name);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
rate_album(struct mpdcron_connection *conn, const char *expr,
		const char *rating)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_rate_album_expr(conn, expr, rating, &values)) {
			eulog(LOG_ERR, "Failed to rate album: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_album, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no album tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_album = quote(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
		myexpr = g_strdup_printf("name=%s", esc_album);
		g_free(esc_album);
		mpd_song_free(song);

		if (!mpdcron_rate_album_expr(conn, myexpr, rating, &values)) {
			eulog(LOG_ERR, "Failed to rate current playing album: %s",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: Rating:%d %s\n", e->id, e->rating, e->name);
		g_free(e->name);
		g_free(e->artist); /* TODO: We don't print artist right now. */
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
rate_genre(struct mpdcron_connection *conn, const char *expr,
		const char *rating)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_rate_genre_expr(conn, expr, rating, &values)) {
			eulog(LOG_ERR, "Failed to rate genre: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_genre, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_GENRE, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no genre tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_genre = quote(mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
		myexpr = g_strdup_printf("name=%s", esc_genre);
		g_free(esc_genre);
		mpd_song_free(song);

		if (!mpdcron_rate_genre_expr(conn, myexpr, rating, &values)) {
			eulog(LOG_ERR, "Failed to rate current playing genre: %s",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: Rating:%d %s\n", e->id, e->rating, e->name);
		g_free(e->name);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
rate_song(struct mpdcron_connection *conn,
		const char *expr, const char *rating)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_rate_expr(conn, expr, rating, &values)) {
			eulog(LOG_ERR, "Failed to rate song: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_uri, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;

		esc_uri = quote(mpd_song_get_uri(song));
		myexpr = g_strdup_printf("uri=%s", esc_uri);
		g_free(esc_uri);
		mpd_song_free(song);

		if (!mpdcron_rate_expr(conn, myexpr, rating, &values)) {
			eulog(LOG_ERR, "Failed to rate current playing song: %s",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_song *e = walk->data;
		printf("%d: Rating:%d %s\n", e->id, e->rating, e->uri);
		g_free(e->uri);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
list_artist(struct mpdcron_connection *conn, const char *expr)
{
	GSList *values, *walk;

	g_assert(expr != NULL);

	values = NULL;
	if (!mpdcron_list_artist_expr(conn, expr, &values)) {
		eulog(LOG_ERR, "Failed to list artist: %s",
				conn->error->message);
		return 1;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: %s\n", e->id, e->name);
		g_free(e->name);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
list_album(struct mpdcron_connection *conn, const char *expr)
{
	GSList *values, *walk;

	g_assert(expr != NULL);

	values = NULL;
	if (!mpdcron_list_album_expr(conn, expr, &values)) {
		eulog(LOG_ERR, "Failed to list album: %s",
				conn->error->message);
		return 1;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: %s\n", e->id, e->name);
		g_free(e->name);
		g_free(e->artist); /* TODO: We don't use artist information. */
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
list_genre(struct mpdcron_connection *conn, const char *expr)
{
	GSList *values, *walk;

	g_assert(expr != NULL);

	values = NULL;
	if (!mpdcron_list_genre_expr(conn, expr, &values)) {
		eulog(LOG_ERR, "Failed to list genre: %s",
				conn->error->message);
		return 1;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: %s\n", e->id, e->name);
		g_free(e->name);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
list_song(struct mpdcron_connection *conn, const char *expr)
{
	GSList *values, *walk;

	g_assert(expr != NULL);

	values = NULL;
	if (!mpdcron_list_expr(conn, expr, &values)) {
		eulog(LOG_ERR, "Failed to list song: %s",
				conn->error->message);
		return 1;
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_song *e = walk->data;
		printf("%d: %s\n", e->id, e->uri);
		g_free(e->uri);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
cmd_rate_internal(const char *expr, const char *rating,
		bool artist, bool album, bool genre)
{
	int port, ret;
	const char *hostname, *password;
	struct mpdcron_connection *conn;

	if ((artist && album && genre) ||
			(artist && album) ||
			(artist && genre) ||
			(album && genre)) {
		g_printerr("--artist, --album and --genre options are mutually exclusive\n");
		return 1;
	}

	hostname = g_getenv(ENV_MPDCRON_HOST)
		? g_getenv(ENV_MPDCRON_HOST)
		: DEFAULT_HOSTNAME;
	port = g_getenv(ENV_MPDCRON_PORT)
		? atoi(g_getenv(ENV_MPDCRON_PORT))
		: DEFAULT_PORT;
	password = g_getenv(ENV_MPDCRON_PASSWORD);

	conn = mpdcron_connection_new(hostname, port);
	if (conn->error != NULL) {
		eulog(LOG_ERR, "Failed to connect: %s", conn->error->message);
		mpdcron_connection_free(conn);
		return 1;
	}

	if (password != NULL) {
		if (!mpdcron_password(conn, password)) {
			eulog(LOG_ERR, "Authentication failed: %s", conn->error->message);
			mpdcron_connection_free(conn);
			return 1;
		}
	}

	if (artist)
		ret = rate_artist(conn, expr, rating);
	else if (album)
		ret = rate_album(conn, expr, rating);
	else if (genre)
		ret = rate_genre(conn, expr, rating);
	else
		ret = rate_song(conn, expr, rating);
	mpdcron_connection_free(conn);
	return ret;
}

static int
cmd_list_internal(const char *expr, bool artist, bool album,
		bool genre)
{
	int port, ret;
	const char *hostname, *password;
	struct mpdcron_connection *conn;

	if ((artist && album && genre) ||
			(artist && album) ||
			(artist && genre) ||
			(album && genre)) {
		g_printerr("--artist, --album and --genre options are mutually exclusive\n");
		return 1;
	}

	hostname = g_getenv(ENV_MPDCRON_HOST)
		? g_getenv(ENV_MPDCRON_HOST)
		: DEFAULT_HOSTNAME;
	port = g_getenv(ENV_MPDCRON_PORT)
		? atoi(g_getenv(ENV_MPDCRON_PORT))
		: DEFAULT_PORT;
	password = g_getenv(ENV_MPDCRON_PASSWORD);

	conn = mpdcron_connection_new(hostname, port);
	if (conn->error != NULL) {
		eulog(LOG_ERR, "Failed to connect: %s", conn->error->message);
		mpdcron_connection_free(conn);
		return 1;
	}

	if (password != NULL) {
		if (!mpdcron_password(conn, password)) {
			eulog(LOG_ERR, "Authentication failed: %s", conn->error->message);
			mpdcron_connection_free(conn);
			return 1;
		}
	}

	if (artist)
		ret = list_artist(conn, expr);
	else if (album)
		ret = list_album(conn, expr);
	else if (genre)
		ret = list_genre(conn, expr);
	else
		ret = list_song(conn, expr);
	mpdcron_connection_free(conn);
	return ret;
}

static int
listinfo_artist(struct mpdcron_connection *conn, const char *expr)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_listinfo_artist_expr(conn, expr, &values)) {
			eulog(LOG_ERR, "Failed to list artist: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_artist, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_ARTIST, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no artist tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_artist = quote(mpd_song_get_tag(song, MPD_TAG_ARTIST, 0));
		myexpr = g_strdup_printf("name=%s", esc_artist);
		g_free(esc_artist);
		mpd_song_free(song);

		if (!mpdcron_listinfo_artist_expr(conn, myexpr, &values)) {
			eulog(LOG_ERR, "Failed to list current playing artist: %s",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: Play_Count:%d Love:%d Kill:%d Rating:%d %s\n",
				e->id, e->play_count, e->love,
				e->kill, e->rating, e->name);
		g_free(e->name);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
listinfo_album(struct mpdcron_connection *conn, const char *expr)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_listinfo_album_expr(conn, expr, &values)) {
			eulog(LOG_ERR, "Failed to list album: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_album, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_ALBUM, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no album tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_album = quote(mpd_song_get_tag(song, MPD_TAG_ALBUM, 0));
		myexpr = g_strdup_printf("name=%s", esc_album);
		g_free(esc_album);
		mpd_song_free(song);

		if (!mpdcron_listinfo_album_expr(conn, myexpr, &values)) {
			eulog(LOG_ERR, "Failed to list current playing album: %s",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: Play_Count:%d Love:%d Kill:%d Rating:%d %s\n",
				e->id, e->play_count, e->love,
				e->kill, e->rating, e->name);
		g_free(e->name);
		g_free(e->artist); /* TODO: We don't use artist information. */
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
listinfo_genre(struct mpdcron_connection *conn, const char *expr)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_listinfo_genre_expr(conn, expr, &values)) {
			eulog(LOG_ERR, "Failed to list genre: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_genre, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;
		else if (mpd_song_get_tag(song, MPD_TAG_GENRE, 0) == NULL) {
			eulog(LOG_ERR, "Current playing song has no genre tag!");
			mpd_song_free(song);
			return 1;
		}

		esc_genre = quote(mpd_song_get_tag(song, MPD_TAG_GENRE, 0));
		myexpr = g_strdup_printf("name=%s", esc_genre);
		g_free(esc_genre);
		mpd_song_free(song);

		if (!mpdcron_listinfo_genre_expr(conn, myexpr, &values)) {
			eulog(LOG_ERR, "Failed to list current playing genre: %s",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_entity *e = walk->data;
		printf("%d: Play_Count:%d Love:%d Kill:%d Rating:%d %s\n",
				e->id, e->play_count, e->love,
				e->kill, e->rating, e->name);
		g_free(e->name);
		g_free(e);
	}
	g_slist_free(values);
	return 0;
}

static int
listinfo_song(struct mpdcron_connection *conn, const char *expr)
{
	GSList *values, *walk;

	values = NULL;
	if (expr != NULL) {
		if (!mpdcron_listinfo_expr(conn, expr, &values)) {
			eulog(LOG_ERR, "Failed to list song: %s",
					conn->error->message);
			return 1;
		}
	}
	else {
		char *esc_uri, *myexpr;
		struct mpd_song *song;

		if ((song = load_current_song()) == NULL)
			return 1;

		esc_uri = quote(mpd_song_get_uri(song));
		myexpr = g_strdup_printf("uri=%s", esc_uri);
		g_free(esc_uri);
		mpd_song_free(song);

		if (!mpdcron_listinfo_expr(conn, myexpr, &values)) {
			eulog(LOG_ERR, "Failed to list current playing song: %s",
					conn->error->message);
			g_free(myexpr);
			return 1;
		}
		g_free(myexpr);
	}

	for (walk = values; walk != NULL; walk = g_slist_next(walk)) {
		struct mpdcron_song *s = walk->data;
		printf("%d: Play_Count:%d Love:%d Kill:%d Rating:%d %s\n",
				s->id, s->play_count, s->love,
				s->kill, s->rating, s->uri);
		g_free(s->uri);
		g_free(s);
	}
	g_slist_free(values);
	return 0;
}

static int
cmd_listinfo_internal(const char *expr, bool artist, bool album,
		bool genre)
{
	int port, ret;
	const char *hostname, *password;
	struct mpdcron_connection *conn;

	if ((artist && album && genre) ||
			(artist && album) ||
			(artist && genre) ||
			(album && genre)) {
		g_printerr("--artist, --album and --genre options are mutually exclusive\n");
		return 1;
	}

	hostname = g_getenv(ENV_MPDCRON_HOST)
		? g_getenv(ENV_MPDCRON_HOST)
		: DEFAULT_HOSTNAME;
	port = g_getenv(ENV_MPDCRON_PORT)
		? atoi(g_getenv(ENV_MPDCRON_PORT))
		: DEFAULT_PORT;
	password = g_getenv(ENV_MPDCRON_PASSWORD);

	conn = mpdcron_connection_new(hostname, port);
	if (conn->error != NULL) {
		eulog(LOG_ERR, "Failed to connect: %s", conn->error->message);
		mpdcron_connection_free(conn);
		return 1;
	}

	if (password != NULL) {
		if (!mpdcron_password(conn, password)) {
			eulog(LOG_ERR, "Authentication failed: %s", conn->error->message);
			mpdcron_connection_free(conn);
			return 1;
		}
	}

	if (artist)
		ret = listinfo_artist(conn, expr);
	else if (album)
		ret = listinfo_album(conn, expr);
	else if (genre)
		ret = listinfo_genre(conn, expr);
	else
		ret = listinfo_song(conn, expr);
	mpdcron_connection_free(conn);
	return ret;
}

static int
cmd_hate(int argc, char **argv)
{
	int opta = 0, optA = 0, optg = 0, ret;
	GError *error = NULL;
	GOptionEntry options[] = {
		{"artist", 'a', 0, G_OPTION_ARG_NONE, &opta,
			"Hate artists instead of songs", NULL},
		{"album", 'A', 0, G_OPTION_ARG_NONE, &optA,
			"Hate albums instead of songs", NULL},
		{"genre", 'g', 0, G_OPTION_ARG_NONE, &optA,
			"Hate genres instead of songs", NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL },
	};
	GOptionContext *ctx;

	ctx = g_option_context_new("[EXPRESSION]");
	g_option_context_add_main_entries(ctx, options, "eugene-hate");
	g_option_context_set_summary(ctx, "eugene-hate-"VERSION GITHEAD" - Hate song/artist/album/genre");
	g_option_context_set_description(ctx, ""
"By default this command works on the current playing song.\n"
"For more information about the expression syntax, see:\n"
"http://www.sqlite.org/lang_expr.html");
	if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
		g_printerr("Option parsing failed: %s\n", error->message);
		g_error_free(error);
		g_option_context_free(ctx);
		return 1;
	}
	g_option_context_free(ctx);

	if (argc > 1)
		ret = cmd_love_internal(false, argv[1], opta, optA, optg);
	else
		ret = cmd_love_internal(false, NULL, opta, optA, optg);
	return ret;
}

static int
cmd_love(int argc, char **argv)
{
	int opta = 0, optA = 0, optg = 0, ret;
	GError *error = NULL;
	GOptionEntry options[] = {
		{"artist", 'a', 0, G_OPTION_ARG_NONE, &opta,
			"Love artists instead of songs", NULL},
		{"album", 'A', 0, G_OPTION_ARG_NONE, &optA,
			"Love albums instead of songs", NULL},
		{"genre", 'g', 0, G_OPTION_ARG_NONE, &optg,
			"Love genres instead of songs", NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL },
	};
	GOptionContext *ctx;

	ctx = g_option_context_new("[EXPRESSION]");
	g_option_context_add_main_entries(ctx, options, "eugene-love");
	g_option_context_set_summary(ctx, "eugene-love-"VERSION GITHEAD" - Love song/artist/album/genre");
	g_option_context_set_description(ctx, ""
"By default this command works on the current playing song.\n"
"For more information about the expression syntax, see:\n"
"http://www.sqlite.org/lang_expr.html");
	if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
		g_printerr("Option parsing failed: %s\n", error->message);
		g_error_free(error);
		g_option_context_free(ctx);
		return 1;
	}
	g_option_context_free(ctx);

	if (argc > 1)
		ret = cmd_love_internal(true, argv[1], opta, optA, optg);
	else
		ret = cmd_love_internal(true, NULL, opta, optA, optg);
	return ret;
}

static int
cmd_kill(int argc, char **argv)
{
	int opta = 0, optA = 0, optg = 0, ret;
	GError *error = NULL;
	GOptionEntry options[] = {
		{"artist", 'a', 0, G_OPTION_ARG_NONE, &opta,
			"Kill artists instead of songs", NULL},
		{"album", 'A', 0, G_OPTION_ARG_NONE, &optA,
			"Kill albums instead of songs", NULL},
		{"genre", 'g', 0, G_OPTION_ARG_NONE, &optg,
			"Kill genres instead of songs", NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL },
	};
	GOptionContext *ctx;

	ctx = g_option_context_new("[EXPRESSION]");
	g_option_context_add_main_entries(ctx, options, "eugene-kill");
	g_option_context_set_summary(ctx, "eugene-kill-"VERSION GITHEAD" - Kill song/artist/album/genre");
	g_option_context_set_description(ctx, ""
"By default this command works on the current playing song.\n"
"For more information about the expression syntax, see:\n"
"http://www.sqlite.org/lang_expr.html");
	if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
		g_printerr("Option parsing failed: %s\n", error->message);
		g_error_free(error);
		g_option_context_free(ctx);
		return 1;
	}
	g_option_context_free(ctx);

	if (argc > 1)
		ret = cmd_kill_internal(true, argv[1], opta, optA, optg);
	else
		ret = cmd_kill_internal(true, NULL, opta, optA, optg);
	return ret;
}

static int
cmd_unkill(int argc, char **argv)
{
	int opta = 0, optA = 0, optg = 0, ret;
	GError *error = NULL;
	GOptionEntry options[] = {
		{"artist", 'a', 0, G_OPTION_ARG_NONE, &opta,
			"Unkill artists instead of songs", NULL},
		{"album", 'A', 0, G_OPTION_ARG_NONE, &optA,
			"Unkill albums instead of songs", NULL},
		{"genre", 'g', 0, G_OPTION_ARG_NONE, &optg,
			"Unkill genres instead of songs", NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL },
	};
	GOptionContext *ctx;

	ctx = g_option_context_new("[EXPRESSION]");
	g_option_context_add_main_entries(ctx, options, "eugene-unkill");
	g_option_context_set_summary(ctx, "eugene-unkill-"VERSION GITHEAD" - Unkill song/artist/album/genre");
	g_option_context_set_description(ctx, ""
"By default this command works on the current playing song.\n"
"For more information about the expression syntax, see:\n"
"http://www.sqlite.org/lang_expr.html");
	if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
		g_printerr("Option parsing failed: %s\n", error->message);
		g_error_free(error);
		g_option_context_free(ctx);
		return 1;
	}
	g_option_context_free(ctx);

	if (argc > 1)
		ret = cmd_kill_internal(false, argv[1], opta, optA, optg);
	else
		ret = cmd_kill_internal(false, NULL, opta, optA, optg);
	return ret;
}

static int
cmd_rate(int argc, char **argv)
{
	int opta = 0, optA = 0, optg = 0, ret;
	GError *error = NULL;
	GOptionEntry options[] = {
		{"artist", 'a', 0, G_OPTION_ARG_NONE, &opta,
			"Rate artists instead of songs", NULL},
		{"album", 'A', 0, G_OPTION_ARG_NONE, &optA,
			"Rate albums instead of songs", NULL},
		{"genre", 'g', 0, G_OPTION_ARG_NONE, &optg,
			"Rate genres instead of songs", NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL },
	};
	GOptionContext *ctx;

	ctx = g_option_context_new("RATING [EXPRESSION]");
	g_option_context_add_main_entries(ctx, options, "eugene-rate");
	g_option_context_set_summary(ctx, "eugene-rate-"VERSION GITHEAD" - Rate song/artist/album/genre");
	g_option_context_set_description(ctx, ""
"By default this command works on the current playing song.\n"
"For more information about the expression syntax, see:\n"
"http://www.sqlite.org/lang_expr.html");
	/* Ignore unknown options so that the user can pass negative numbers.
	 */
	g_option_context_set_ignore_unknown_options(ctx, TRUE);
	if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
		g_printerr("Option parsing failed: %s\n", error->message);
		g_error_free(error);
		g_option_context_free(ctx);
		return 1;
	}
	g_option_context_free(ctx);

	if (argc <= 1) {
		g_printerr("No rating given\n");
		ret = 1;
	}
	else if (argc > 2)
		ret = cmd_rate_internal(argv[2], argv[1], opta, optA, optg);
	else
		ret = cmd_rate_internal(NULL, argv[1], opta, optA, optg);
	return ret;
}

static int
cmd_list(int argc, char **argv)
{
	int opta = 0, optA = 0, optg = 0, ret;
	GError *error = NULL;
	GOptionEntry options[] = {
		{"artist", 'a', 0, G_OPTION_ARG_NONE, &opta,
			"List artists instead of songs", NULL},
		{"album", 'A', 0, G_OPTION_ARG_NONE, &optA,
			"List albums instead of songs", NULL},
		{"genre", 'g', 0, G_OPTION_ARG_NONE, &optg,
			"List genres instead of songs", NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL },
	};
	GOptionContext *ctx;

	ctx = g_option_context_new("EXPRESSION");
	g_option_context_add_main_entries(ctx, options, "eugene-list");
	g_option_context_set_summary(ctx, "eugene-list-"VERSION GITHEAD" - List song/artist/album/genre");
	g_option_context_set_description(ctx, ""
"For more information about the expression syntax, see:\n"
"http://www.sqlite.org/lang_expr.html");
	if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
		g_printerr("Option parsing failed: %s\n", error->message);
		g_error_free(error);
		g_option_context_free(ctx);
		return 1;
	}
	g_option_context_free(ctx);

	if (argc > 1)
		ret = cmd_list_internal(argv[1], opta, optA, optg);
	else {
		g_printerr("No expression given\n");
		ret = 1;
	}
	return ret;
}

static int
cmd_listinfo(int argc, char **argv)
{
	int opta = 0, optA = 0, optg = 0, ret;
	GError *error = NULL;
	GOptionEntry options[] = {
		{"artist", 'a', 0, G_OPTION_ARG_NONE, &opta,
			"List artists instead of songs", NULL},
		{"album", 'A', 0, G_OPTION_ARG_NONE, &optA,
			"List albums instead of songs", NULL},
		{"genre", 'g', 0, G_OPTION_ARG_NONE, &optg,
			"List genres instead of songs", NULL},
		{ NULL, 0, 0, 0, NULL, NULL, NULL },
	};
	GOptionContext *ctx;

	ctx = g_option_context_new("[EXPRESSION]");
	g_option_context_add_main_entries(ctx, options, "eugene-listinfo");
	g_option_context_set_summary(ctx, "eugene-listinfo-"VERSION GITHEAD" - List song/artist/album/genre");
	g_option_context_set_description(ctx, ""
"By default this command works on the current playing song.\n"
"For more information about the expression syntax, see:\n"
"http://www.sqlite.org/lang_expr.html");
	if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
		g_printerr("Option parsing failed: %s\n", error->message);
		g_error_free(error);
		g_option_context_free(ctx);
		return 1;
	}
	g_option_context_free(ctx);

	if (argc > 1)
		ret = cmd_listinfo_internal(argv[1], opta, optA, optg);
	else
		ret = cmd_listinfo_internal(NULL, opta, optA, optg);

	return ret;
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
