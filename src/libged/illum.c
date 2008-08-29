/*                         I L L U M . C
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
/** @file illum.c
 *
 * The illum command.
 *
 */

#include "ged.h"
#include "solid.h"


/*
 * Illuminate/highlight database object
 *
 * Usage:
 *        illum [-n] obj
 *
 */
int
ged_illum(struct ged *gedp, int argc, const char *argv[])
{
    register struct solid *sp;
    int found = 0;
    int illum = 1;
    static const char *usage = "[-n] obj";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_DRAWABLE(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);
    gedp->ged_result = GED_RESULT_NULL;

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc == 3) {
	if (argv[1][0] == '-' && argv[1][1] == 'n')
	    illum = 0;
	else
	    goto bad;

	--argc;
	++argv;
    }

    if (argc != 2)
	goto bad;

    FOR_ALL_SOLIDS(sp, &gedp->ged_gdp->gd_headSolid) {
	register int i;

	for (i = 0; i < sp->s_fullpath.fp_len; ++i) {
	    if (*argv[1] == *DB_FULL_PATH_GET(&sp->s_fullpath, i)->d_namep &&
		strcmp(argv[1], DB_FULL_PATH_GET(&sp->s_fullpath, i)->d_namep) == 0) {
		found = 1;
		if (illum)
		    sp->s_iflag = UP;
		else
		    sp->s_iflag = DOWN;
	    }
	}
    }

    if (!found) {
	bu_vls_printf(&gedp->ged_result_str, "illum: %s not found", argv[1]);
	return BRLCAD_ERROR;
    }

    return BRLCAD_OK;

 bad:
    bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
    return BRLCAD_ERROR;
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
