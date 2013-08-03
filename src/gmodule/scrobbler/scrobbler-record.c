/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2009, 2013 Ali Polatel <alip@exherbo.org>
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

#include "scrobbler-defs.h"

#include <glib.h>

void record_copy(struct record *dest, const struct record *src)
{
	dest->artist = g_strdup(src->artist);
	dest->track = g_strdup(src->track);
	dest->album = g_strdup(src->album);
	dest->number = g_strdup(src->number);
	dest->mbid = g_strdup(src->mbid);
	dest->time = g_strdup(src->time);
	dest->length = src->length;
	dest->source = src->source;
}

struct record *record_dup(const struct record *src)
{
	struct record *dest = g_new(struct record, 1);
	record_copy(dest, src);
	return dest;
}

void record_deinit(struct record *record)
{
	g_free(record->artist);
	g_free(record->track);
	g_free(record->album);
	g_free(record->number);
	g_free(record->mbid);
	g_free(record->time);
}

void record_free(struct record *record)
{
	record_deinit(record);
	g_free(record);
}

void record_clear(struct record *record)
{
	record->artist = NULL;
	record->track = NULL;
	record->album = NULL;
	record->number = NULL;
	record->mbid = NULL;
	record->time = NULL;
	record->length = 0;
	record->source = "P";
}
