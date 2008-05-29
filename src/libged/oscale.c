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

#include "ged.h"

/*
 * Scale obj about the keypoint by sf.
 */
int
ged_oscale(struct rt_wdb *wdbp, int argc, const char *argv[])
{
    register struct directory *dp;
    struct wdb_trace_data wtd;
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

    GED_CHECK_DATABASE_OPEN(wdbp, GED_ERROR);
    GED_CHECK_READ_ONLY(wdbp, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(&wdbp->wdb_result_str, 0);
    wdbp->wdb_result = GED_RESULT_NULL;
    wdbp->wdb_result_flags = 0;

    /* must be wanting help */
    if (argc == 1) {
	wdbp->wdb_result_flags |= GED_RESULT_FLAGS_HELP_BIT;
	bu_vls_printf(&wdbp->wdb_result_str, "Usage: %s %s", argv[0], usage);
	return GED_OK;
    }

    if (argc != 3 && argc != 6) {
	bu_vls_printf(&wdbp->wdb_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }

    if (sscanf(argv[2], "%lf", &sf) != 1) {
	bu_vls_printf(&wdbp->wdb_result_str, "%s: bad sf value - %s", argv[0], argv[2]);
	return GED_ERROR;
    }

    if (argc == 3) {
	if (ged_get_obj_bounds2(wdbp, 1, argv+1, &wtd, rpp_min, rpp_max) == TCL_ERROR)
	    return TCL_ERROR;

	dp = wtd.wtd_obj[wtd.wtd_objpos-1];
	if (!(dp->d_flags & DIR_SOLID)) {
	    if (ged_get_obj_bounds(wdbp, 1, argv+1, 1, rpp_min, rpp_max) == TCL_ERROR)
		return TCL_ERROR;
	}

	VADD2(keypoint, rpp_min, rpp_max);
	VSCALE(keypoint, keypoint, 0.5);
    } else {
	/* The user has provided the keypoint. */
	MAT_IDN(wtd.wtd_xform);

	if (sscanf(argv[3], "%lf", &keypoint[X]) != 1) {
	    bu_vls_printf(&wdbp->wdb_result_str, "%s: bad kx value - %s", argv[0], argv[3]);
	    return GED_ERROR;
	}

	if (sscanf(argv[4], "%lf", &keypoint[Y]) != 1) {
	    bu_vls_printf(&wdbp->wdb_result_str, "%s: bad ky value - %s", argv[0], argv[4]);
	    return GED_ERROR;
	}

	if (sscanf(argv[5], "%lf", &keypoint[Z]) != 1) {
	    bu_vls_printf(&wdbp->wdb_result_str, "%s: bad kz value - %s", argv[0], argv[5]);
	    return GED_ERROR;
	}

	VSCALE(keypoint, keypoint, wdbp->dbip->dbi_local2base);

	if ((dp = db_lookup(wdbp->dbip, argv[1], LOOKUP_QUIET)) == DIR_NULL) {
	    bu_vls_printf(&wdbp->wdb_result_str, "%s: %s not found", argv[0], argv[1]);
	    return GED_ERROR;
	}
    }

    MAT_IDN(smat);
    bn_mat_scale_about_pt(smat, keypoint, sf);

    bn_mat_inv(invXform, wtd.wtd_xform);
    bn_mat_mul(tmpMat, invXform, smat);
    bn_mat_mul(emat, tmpMat, wtd.wtd_xform);

    GED_DB_GET_INTERNAL(wdbp, &intern, dp, emat, &rt_uniresource, GED_ERROR);
    RT_CK_DB_INTERNAL(&intern);
    GED_DB_PUT_INTERNAL(wdbp, dp, &intern, &rt_uniresource, GED_ERROR);

#if 0
    /* notify observers */
    bu_observer_notify(interp, &wdbp->wdb_observers, bu_vls_addr(&wdbp->wdb_name));
#endif

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
