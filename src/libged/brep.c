/*                         B R E P . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2012 United States Government as represented by
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
/** @file libged/brep.c
 *
 * The brep command.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <string.h>

#include "bio.h"
#include "bu.h"
#include "rtgeom.h"
#include "wdb.h"

#include "./ged_private.h"


#if 1
RT_EXPORT extern int brep_command(struct bu_vls *vls, struct brep_specific* bs, struct rt_brep_internal* bi, struct bn_vlblock *vbp, int argc, const char *argv[], char *commtag);
RT_EXPORT extern int brep_conversion(struct rt_db_internal* intern, ON_Brep **brep);
#else
extern int brep_surface_plot(struct ged *gedp, struct brep_specific* bs, struct rt_brep_internal* bi, struct bn_vlblock *vbp, int index);
#endif

int
ged_brep(struct ged *gedp, int argc, const char *argv[])
{
    struct bn_vlblock*vbp;
    char *solid_name;
    static const char *usage = "brep obj [command|brepname] ";
    struct directory *ndp;
    struct rt_db_internal intern;
    struct rt_brep_internal* bi;
    struct brep_specific* bs;
    struct soltab *stp;
    int real_flag;
    char commtag[64];
    char namebuf[64];

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_DRAWABLE(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc < 2) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	bu_vls_printf(gedp->ged_result_str, "commands:\n");
	bu_vls_printf(gedp->ged_result_str, "\tinfo - return count information for specific BREP\n");
	bu_vls_printf(gedp->ged_result_str, "\tinfo S [index] - return information for specific BREP 'surface'\n");
	bu_vls_printf(gedp->ged_result_str, "\tinfo F [index] - return information for specific BREP 'face'\n");
	bu_vls_printf(gedp->ged_result_str, "\tplot - plot entire BREP\n");
	bu_vls_printf(gedp->ged_result_str, "\tplot S [index] - plot specific BREP 'surface'\n");
	bu_vls_printf(gedp->ged_result_str, "\tplot F [index] - plot specific BREP 'face'\n");
	bu_vls_printf(gedp->ged_result_str, "\t[brepname] - convert the non-BREP object to BREP form");
	return GED_HELP;
    }

    if (argc < 2 || argc > 6) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }

    solid_name = (char *)argv[1];
    if ((ndp = db_lookup(gedp->ged_wdbp->dbip,  solid_name, LOOKUP_NOISY)) == RT_DIR_NULL) {
	bu_vls_printf(gedp->ged_result_str, "Error: %s is not a solid or does not exist in database", solid_name);
	return GED_ERROR;
    } else {
	real_flag = (ndp->d_addr == RT_DIR_PHONY_ADDR) ? 0 : 1;
    }

    if (!real_flag) {
	/* solid doesnt exists - don't kill */
	bu_vls_printf(gedp->ged_result_str, "Error: %s is not a real solid", solid_name);
	return GED_OK;
    }

    GED_DB_GET_INTERNAL(gedp, &intern, ndp, bn_mat_identity, &rt_uniresource, GED_ERROR);


    RT_CK_DB_INTERNAL(&intern);
    bi = (struct rt_brep_internal*)intern.idb_ptr;

    if (!RT_BREP_TEST_MAGIC(bi)) {
	/* The solid is not in brep form. Covert it to brep. */

	char* bname;
	ON_Brep** brep = (ON_Brep**)bu_malloc(sizeof(ON_Brep*), "ON_Brep*");
	int ret;
	if (argc > 2) {
	    bname = (char*)bu_malloc(strlen(argv[2])+1, "char");
	    bu_strlcpy(bname, argv[2], strlen(argv[2])+1);
	} else {
	    bname = (char*)bu_malloc(strlen(solid_name)+6, "char");
	    bu_strlcpy(bname, solid_name, strlen(solid_name)+1);
	    bu_strlcat(bname, "_brep", strlen(bname)+6);
	}
	if (db_lookup(gedp->ged_wdbp->dbip, bname, LOOKUP_QUIET) != RT_DIR_NULL) {
	    bu_vls_printf(gedp->ged_result_str, "%s already exist.", bname);
	    bu_free(brep, "ON_Brep*");
	    bu_free(bname, "char");
	    return GED_OK;
	}
	ret = brep_conversion(&intern, brep);
	if (ret == -1) {
	    bu_vls_printf(gedp->ged_result_str, "%s doesn't have a brep-conversion function yet.", solid_name);
	} else {
	    ret = mk_brep(gedp->ged_wdbp, bname, *brep);
	    if (ret == 0) {
		bu_vls_printf(gedp->ged_result_str, "%s is made.", bname);
	    }
	}
	bu_free(brep, "ON_Brep*");
	bu_free(bname, "char");
	return GED_OK;
    }

    BU_GET(stp, struct soltab);

    if (argc == 2) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s\n", argv[0], usage);
	bu_vls_printf(gedp->ged_result_str, "\t%s is in brep form, please input a command.", solid_name);
	return GED_HELP;
    }

    if (!BU_STR_EQUAL(argv[2], "info") && !BU_STR_EQUAL(argv[2], "plot")) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s\n", argv[0], usage);
	bu_vls_printf(gedp->ged_result_str, "\t%s is in brep form, please input a command.", solid_name);
	return GED_HELP;
    }

    if ((bs = (struct brep_specific*)stp->st_specific) == NULL) {
	bs = (struct brep_specific*)bu_malloc(sizeof(struct brep_specific), "brep_specific");
	bs->brep = bi->brep;
	bi->brep = NULL;
	stp->st_specific = (genptr_t)bs;
    }

    vbp = rt_vlblock_init();

    brep_command(gedp->ged_result_str, bs, bi, vbp, argc, argv, commtag);

    snprintf(namebuf, 64, "%s%s_", commtag, solid_name);
    _ged_cvt_vlblock_to_solids(gedp, vbp, namebuf, 0);
    rt_vlblock_free(vbp);
    vbp = (struct bn_vlblock *)NULL;

    rt_db_free_internal(&intern);

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
