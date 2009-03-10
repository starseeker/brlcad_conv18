/*                         W H A T I D . C
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
/** @file whatid.c
 *
 * The whatid command.
 *
 */

#include "common.h"

#include <string.h>
#include "bio.h"

#include "cmd.h"

#include "./ged_private.h"


int
ged_whatid(struct ged *gedp, int argc, const char *argv[])
{
    struct directory	*dp;
    struct rt_db_internal	intern;
    struct rt_comb_internal	*comb;
    static const char *usage = "region";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 2) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }


    if ((dp=db_lookup(gedp->ged_wdbp->dbip, argv[1], LOOKUP_NOISY)) == DIR_NULL)
	return BRLCAD_ERROR;

    if (!(dp->d_flags & DIR_REGION)) {
	bu_vls_printf(&gedp->ged_result_str, "%s is not a region", argv[1]);
	return BRLCAD_ERROR;
    }

    if (rt_db_get_internal(&intern, dp, gedp->ged_wdbp->dbip, (fastf_t *)NULL, &rt_uniresource) < 0)
	return BRLCAD_ERROR;
    comb = (struct rt_comb_internal *)intern.idb_ptr;

    bu_vls_printf(&gedp->ged_result_str, "%d", comb->region_id);
#if USE_RT_COMB_IFREE
    rt_comb_ifree(&intern, &rt_uniresource);
#else
    rt_db_free_internal(&intern, &rt_uniresource);
#endif

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
