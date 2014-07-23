/*                      S O L I D I T Y . H
 * BRL-CAD
 *
 * Copyright (c) 2014 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file solidity.h
 *
 * Library of algorithms to determine whether a BoT is solid.
 *
 */


#include "common.h"


#include "rtgeom.h"


#ifdef __cplusplus
extern "C" {
#endif


int bot_is_closed(const struct rt_bot_internal *bot);

int bot_is_orientable(const struct rt_bot_internal *bot);

int bot_is_manifold(const struct rt_bot_internal *bot);

int bot_is_solid(const struct rt_bot_internal *bot);


#ifdef __cplusplus
}
#endif


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
