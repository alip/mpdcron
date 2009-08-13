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

#ifndef MPDHOOKER_GUARD_KEYFILE_H
#define MPDHOOKER_GUARD_KEYFILE_H 1

#include <stdbool.h>

/* Defaults */
#define DEFAULT_MPD_POLL        1
#define DEFAULT_MPD_RECONNECT   10
#define DEFAULT_MPD_TIMEOUT     10.0

bool mhkeyfile_load(void);

#endif // MPDHOOKER_GUARD_KEYFILE_H

