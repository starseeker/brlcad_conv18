/*                         B O T _ V E R T E X _ F U S E . C
 * BRL-CAD
 *
 * Copyright (c) 2008 United States Government as represented by
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
/** @file bot_vertex_fuse.c
 *
 * The bot_vertex_fuse command.
 *
 */

#include "common.h"
#include "bio.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "rtgeom.h"
#include "ged_private.h"

int
ged_bot_vertex_fuse(struct ged *gedp, int argc, const char *argv[])
{
    struct directory *old_dp, *new_dp;
    struct rt_db_internal intern;
    struct rt_bot_internal *bot;
    int count1=0;
    static const char *usage = "new_bot old_bot";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_READ_ONLY(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 3) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    if ( (old_dp = db_lookup( gedp->ged_wdbp->dbip, argv[2], LOOKUP_NOISY )) == DIR_NULL )
	return BRLCAD_ERROR;

    if ( rt_db_get_internal( &intern, old_dp, gedp->ged_wdbp->dbip, bn_mat_identity, &rt_uniresource ) < 0 )
    {
	bu_vls_printf(&gedp->ged_result_str, "%s: rt_db_get_internal(%s) error\n", argv[0], argv[2]);
	return BRLCAD_ERROR;
    }

    if ( intern.idb_type != ID_BOT )
    {
	bu_vls_printf(&gedp->ged_result_str, "%s: %s is not a BOT solid!!!\n", argv[0], argv[2]);
	return BRLCAD_ERROR;
    }

    bot = (struct rt_bot_internal *)intern.idb_ptr;
    RT_BOT_CK_MAGIC( bot );

    count1 = rt_bot_vertex_fuse( bot );
    if ( count1 )
	(void)rt_bot_condense( bot );

    if ( (new_dp=db_diradd( gedp->ged_wdbp->dbip, argv[1], -1L, 0, DIR_SOLID, (genptr_t)&intern.idb_type)) == DIR_NULL )
    {
	bu_vls_printf(&gedp->ged_result_str, "%s: Cannot add %s to directory\n", argv[0], argv[1]);
	return BRLCAD_ERROR;
    }

    if ( rt_db_put_internal( new_dp, gedp->ged_wdbp->dbip, &intern, &rt_uniresource ) < 0 )
    {
	rt_db_free_internal( &intern, &rt_uniresource );
	bu_vls_printf(&gedp->ged_result_str, "%s: Database write error, aborting\n", argv[0]);
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
