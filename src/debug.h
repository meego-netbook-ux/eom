/* eom: sample picture view application (Eye of Moblin)
 *
 * Copyright (C) 2009 Intel Corp.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Roger WANG <roger.wang@intel.com>
 */

#ifndef _EOM_DEBUG_H
#define _EOM_DEBUG_H

#include "config.h"
#include <stdio.h>

#ifdef HAVE_DEBUG
#define DBG(...) do { printf ( __VA_ARGS__); } while (0)
#else
#define DBG(...) do {} while (0)
#endif

#endif
