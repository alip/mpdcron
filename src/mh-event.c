/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet ai cin fdm=syntax : */

/*
 * Copyright (c) 2009 Ali Polatel <alip@exherbo.org>
 *
 * This file is part of the mpdhooker mpd client. mpdhooker is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
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

#include "mh-defs.h"

#include <libdaemon/dlog.h>
#include <mpd/client.h>

static int mhevent_database(void)
{
	return 0;
}

static int mhevent_stored_playlist(void)
{
	return 0;
}

static int mhevent_queue(void)
{
	return 0;
}

static int mhevent_player(void)
{
	return 0;
}

static int mhevent_mixer(void)
{
	return 0;
}

static int mhevent_output(void)
{
	return 0;
}

static int mhevent_options(void)
{
	return 0;
}

static int mhevent_update(void)
{
	return 0;
}

int mhevent_run(enum mpd_idle event)
{
	switch (event) {
		case MPD_IDLE_DATABASE:
			return mhevent_database();
		case MPD_IDLE_STORED_PLAYLIST:
			return mhevent_stored_playlist();
		case MPD_IDLE_PLAYER:
			return mhevent_player();
		case MPD_IDLE_QUEUE:
			return mhevent_queue();
		case MPD_IDLE_MIXER:
			return mhevent_mixer();
		case MPD_IDLE_OUTPUT:
			return mhevent_output();
		case MPD_IDLE_OPTIONS:
			return mhevent_options();
		case MPD_IDLE_UPDATE:
			return mhevent_update();
		default:
			mh_log(LOG_WARNING, "Unknown event 0x%x", event);
			return 0;
	}
}
