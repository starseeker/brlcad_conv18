/*                         O S C A L E . C
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
/** @file oscale.c
 *
 * The oscale command.
 *
 */

#include "common.h"
#include "bio.h"

#include "./ged_private.h"

/*
 * Scale obj about the keypoint by sf.
 */
int
ged_oscale(struct ged *gedp, int argc, const char *argv[])
{
    register struct directory *dp;
    struct ged_trace_data gtd;
    struct rt_db_internal intern;
    mat_t smat;
    mat_t emat;
    mat_t tmpMat;
    mat_t invXform;
    point_t rpp_min;
    point_t rpp_max;
    fastf_t sf;
    point_t keypoint;
    static const char *usage = "obj sf [kX kY kZ]";

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

    if (argc != 3 && argc != 6) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[2], "%lf", &sf) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "%s: bad sf value - %s", argv[0], argv[2]);
	return BRLCAD_ERROR;
    }

    if (argc == 3) {
	if (ged_get_obj_bounds2(gedp, 1, argv+1, &gtd, rpp_min, rpp_max) == TCL_ERROR)
	    return TCL_ERROR;

	dp = gtd.gtd_obj[gtd.gtd_objpos-1];
	if (!(dp->d_flags & DIR_SOLID)) {
	    if (ged_get_obj_bounds(gedp, 1, argv+1, 1, rpp_min, rpp_max) == TCL_ERROR)
		return TCL_ERROR;
	}

	VADD2(keypoint, rpp_min, rpp_max);
	VSCALE(keypoint, keypoint, 0.5);
    } else {
	/* The user has provided the keypoint. */
	MAT_IDN(gtd.gtd_xform);

	if (sscanf(argv[3], "%lf", &keypoint[X]) != 1) {
	    bu_vls_printf(&gedp->ged_result_str, "%s: bad kx value - %s", argv[0], argv[3]);
	    return BRLCAD_ERROR;
	}

	if (sscanf(argv[4], "%lf", &keypoint[Y]) != 1) {
	    bu_vls_printf(&gedp->ged_result_str, "%s: bad ky value - %s", argv[0], argv[4]);
	    return BRLCAD_ERROR;
	}

	if (sscanf(argv[5], "%lf", &keypoint[Z]) != 1) {
	    bu_vls_printf(&gedp->ged_result_str, "%s: bad kz value - %s", argv[0], argv[5]);
	    return BRLCAD_ERROR;
	}

	VSCALE(keypoint, keypoint, gedp->ged_wdbp->dbip->dbi_local2base);

	if ((dp = db_lookup(gedp->ged_wdbp->dbip, argv[1], LOOKUP_QUIET)) == DIR_NULL) {
	    bu_vls_printf(&gedp->ged_result_str, "%s: %s not found", argv[0], argv[1]);
	    return BRLCAD_ERROR;
	}
    }

    MAT_IDN(smat);
    bn_mat_scale_about_pt(smat, keypoint, sf);

    bn_mat_inv(invXform, gtd.gtd_xform);
    bn_mat_mul(tmpMat, invXform, smat);
    bn_mat_mul(emat, tmpMat, gtd.gtd_xform);

    GED_DB_GET_INTERNAL(gedp, &intern, dp, emat, &rt_uniresource, BRLCAD_ERROR);
    RT_CK_DB_INTERNAL(&intern);
    GED_DB_PUT_INTERNAL(gedp, dp, &intern, &rt_uniresource, BRLCAD_ERROR);

#if 0
    /* notify observers */
    bu_observer_notify(interp, &gedp->wdb_observers, bu_vls_addr(&gedp->wdb_name));
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
