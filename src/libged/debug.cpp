/*                         D E B U G B U . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2019 United States Government as represented by
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
/** @file libged/debug.c
 *
 * The debug command.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "bu/debug.h"
#include "rt/debug.h"
#include "nmg/debug.h"
#include "optical/debug.h"
#include "./ged_private.h"

#include "./debug_vars.cpp"

int
ged_debug(struct ged *gedp, int argc, const char **argv)
{
    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    if (debug_cmd(gedp->ged_result_str, gedp->ged_result_str, argc, argv)) {
	return GED_ERROR;
    }

    return GED_OK;
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
