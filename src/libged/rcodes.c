/*                         R C O D E S . C
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
/** @file rcodes.c
 *
 * The rcodes command.
 *
 */

#include "common.h"
#include "bio.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "ged_private.h"

#define LINELEN 256

int
ged_rcodes(struct ged *gedp, int argc, const char *argv[])
{
    int item, air, mat, los;
    char name[256];
    char line[LINELEN];
    char *cp;
    FILE *fp;
    register struct directory *dp;
    struct rt_db_internal intern;
    struct rt_comb_internal *comb;
    static const char *usage = "filename";

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

    if (argc != 2) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    if ((fp = fopen(argv[1], "r")) == NULL) {
	bu_vls_printf(&gedp->ged_result_str, "%s: Failed to read file - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    while (bu_fgets(line, LINELEN, fp) != NULL) {
	int changed;

	if (sscanf(line, "%d%d%d%d%256s", &item, &air, &mat, &los, name) != 5)
	    continue; /* not useful */

	/* skip over the path */
	if ((cp = strrchr(name, (int)'/')) == NULL)
	    cp = name;
	else
	    ++cp;

	if (*cp == '\0')
	    continue;

	if ((dp = db_lookup( gedp->ged_wdbp->dbip, cp, LOOKUP_NOISY )) == DIR_NULL) {
	    bu_vls_printf(&gedp->ged_result_str, "%s: Warning - %s not found in database.\n", argv[1], cp);
	    continue;
	}

	if (!(dp->d_flags & DIR_REGION)) {
	    bu_vls_printf(&gedp->ged_result_str, "%s: Warning - %s not a region\n", argv[1], cp);
	    continue;
	}

	if (rt_db_get_internal(&intern, dp, gedp->ged_wdbp->dbip, (matp_t)NULL, &rt_uniresource) != ID_COMBINATION) {
	    bu_vls_printf(&gedp->ged_result_str, "%s: Warning - %s not a region\n", argv[1], cp);
	    continue;
	}

	comb = (struct rt_comb_internal *)intern.idb_ptr;

	/* make the changes */
	changed = 0;
	if (comb->region_id != item) {
	    comb->region_id = item;
	    changed = 1;
	}
	if (comb->aircode != air) {
	    comb->aircode = air;
	    changed = 1;
	}
	if (comb->GIFTmater != mat) {
	    comb->GIFTmater = mat;
	    changed = 1;
	}
	if (comb->los != los) {
	    comb->los = los;
	    changed = 1;
	}

	if (changed) {
	    /* write out all changes */
	    if (rt_db_put_internal(dp, gedp->ged_wdbp->dbip, &intern, &rt_uniresource)) {
		bu_vls_printf(&gedp->ged_result_str, "Database write error, aborting.\n");
		bu_vls_printf(&gedp->ged_result_str,
			      "The in-memory table of contents may not match the status of the on-disk\ndatabase.  The on-disk database should still be intact.  For safety,\nyou should exit now, and resolve the I/O problem, before continuing.\n");

		rt_db_free_internal(&intern, &rt_uniresource);
		return BRLCAD_ERROR;
	    }
	}

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
