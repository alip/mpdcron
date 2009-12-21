/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet ai cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
 * Based in part upon mpdscribble which is:
 *   Copyright (C) 2008-2009 The Music Player Daemon Project
 *   Copyright (C) 2005-2008 Kuno Woudt <kuno@frob.nl>
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

#ifndef MPDCRON_GUARD_SCROBBLER_DEFS_H
#define MPDCRON_GUARD_SCROBBLER_DEFS_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdbool.h>
#include <stddef.h>

#include <glib.h>

#define SCROBBLER_LOG_PREFIX	"[scrobbler] "

struct record {
	char *artist;
	char *track;
	char *album;
	char *mbid;
	char *time;
	int length;
	const char *source;
};

typedef void http_client_callback_t(size_t, const char *, void *);

struct scrobbler_config {
	/**
	 * The name of the mpdscribble.conf section.  It is used in
	 * log messages.
	 */
	char *name;

	char *url;
	char *username;
	char *password;

	/**
	 * The path of the journal file.  It contains records which
	 * have not been submitted yet.
	 */
	char *journal;
};

extern int optnd;
extern char *proxy;

/**
 * Copies attributes from one record to another.  Does not free
 * existing values in the destination record.
 */
void record_copy(struct record *dest, const struct record *src);
/**
 * Duplicates a record object.
 */
struct record *record_dup(const struct record *src);
/**
 * Deinitializes a record object, freeing all members.
 */
void record_deinit(struct record *record);
/**
 * Frees a record object: free all members with record_deinit(), and
 * free the record pointer itself.
 */
void record_free(struct record *record);
void record_clear(struct record *record);

static inline bool record_is_defined(const struct record *record)
{
	return record->artist != NULL && record->track != NULL;
}

bool journal_write(const char *path, GQueue *queue);
void journal_read(const char *path, GQueue *queue);

/**
 * Perform global initialization on the HTTP client library.
 */
void http_client_init(void);
/**
 * Global deinitializaton.
 */
void http_client_finish(void);
/**
 * Escapes URI parameters with '%'.  Free the return value with
 * g_free().
 */
char *http_client_uri_escape(const char *src);
void http_client_request(const char *url, const char *post_data,
		http_client_callback_t * callback, void *data);


void as_init(GSList *scrobbler_configs);
void as_cleanup(void);
void as_now_playing(const char *artist, const char *track,
		const char *album, const char *mbid, const int length);
void as_songchange(const char *file, const char *artist, const char *track,
		const char *album, const char *mbid, const int length,
		const char *time);
void as_save_cache(void);
char *as_timestamp(void);

int file_load(GKeyFile *fd, GSList **scrobblers_ptr);
void vlog(int level, const char *fmt, ...);
#endif /* !MPDCRON_GUARD_SCROBBLER_DEFS_H */
