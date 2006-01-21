/*                     P K G S W I T C H . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2006 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */

/** \addtogroup libfb */
/*@{*/

/** @file pkgswitch.c
 * Package Handlers.
 */
/*@}*/

/* interface header */
#include "pkg.h"

/* message types */
#include "./pkgtypes.h"

/*
 * Package Handlers.
 */
extern int pkgfoo();	/* foobar message handler */
struct pkg_switch pkg_switch[] = {
	{ MSG_ERROR, pkgfoo, "Error Message" },
	{ MSG_CLOSE, pkgfoo, "Close Connection" }
};
int pkg_swlen = 2;

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
