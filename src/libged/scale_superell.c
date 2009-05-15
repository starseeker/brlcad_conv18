/*                         S C A L E _ S U P E R E L L . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2009 United States Government as represented by
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
/** @file scale_superell.c
 *
 * The scale_superell command.
 */

#include "common.h"

#include <string.h>
#include "bio.h"

#include "cmd.h"
#include "rtgeom.h"
#include "raytrace.h"

#include "./ged_private.h"

int
ged_scale_superell(struct ged *gedp, struct rt_superell_internal *superell, const char *attribute, fastf_t sf)
{
    fastf_t ma, mb;

    RT_SUPERELL_CK_MAGIC(superell);

    switch (attribute[0]) {
    case 'a':
    case 'A':
	VSCALE(superell->a, superell->a, sf);
	break;
    case 'b':
    case 'B':
	VSCALE(superell->b, superell->b, sf);
	break;
    case 'c':
    case 'C':
	VSCALE(superell->c, superell->c, sf);
	break;
    case '3':
	VSCALE(superell->a, superell->a, sf);
	VSCALE(superell->b, superell->b, sf);
	VSCALE(superell->c, superell->c, sf);
	break;
    default:
	bu_vls_printf(&gedp->ged_result_str, "bad superell attribute - %s", attribute);
	return BRLCAD_ERROR;
    }

    return BRLCAD_OK;
}


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
