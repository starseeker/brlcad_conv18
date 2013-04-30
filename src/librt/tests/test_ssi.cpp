/*                   T E S T _ S S I . C P P
 * BRL-CAD
 *
 * Copyright (c) 2013 United States Government as represented by
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

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>
#include "common.h"
#include "vmath.h"
#include "brep.h"
#include "raytrace.h"

#include "rtgeom.h"
#include "wdb.h"

int
get_surface(const char *name, struct db_i *dbip, struct rt_db_internal *intern, struct rt_brep_internal **brep_ip) {

    struct directory *dp;
    (*brep_ip) = NULL;


    dp = db_lookup(dbip, name, LOOKUP_QUIET);
    if (dp == RT_DIR_NULL) {
	bu_log("ERROR: Unable to look up object %s\n", name);
	return -1;
    }

    if (rt_db_get_internal(intern, dp, dbip, NULL, &rt_uniresource) < 0) {
	bu_log("ERROR: Unable to get internal representation of %s\n", name);
	return -1;
    }

    RT_CK_DB_INTERNAL(intern);

    if (intern->idb_minor_type != DB5_MINORTYPE_BRLCAD_BREP) {
	bu_log("ERROR: object %s does not appear to be of type BRep\n", name);
	return -1;
    } else {
	(*brep_ip) = (struct rt_brep_internal *)(intern->idb_ptr);
    }

    RT_BREP_CK_MAGIC(*brep_ip);
    return 0;
}

int
main(int argc, char** argv)
{
    int ret = 0;
    struct bu_vls intersect_name, name;

    if (argc != 6 && argc != 7) {
	bu_log("Usage: %s %s\n", argv[0], "<file> <obj1> <obj2> <surf_i> <surf_j> [curve_name]");
	return -1;
    }

    // Read the file
    struct db_i *dbip;
    dbip = db_open(argv[1], "r+w");
    if (dbip == DBI_NULL) {
	bu_log("ERROR: Unable to read from %s\n", argv[1]);
	return -1;
    }

    if (db_dirbuild(dbip) < 0){
	bu_log("ERROR: Unable to read from %s\n", argv[1]);
	return -1;
    }

    // Get surfaces
    struct rt_db_internal obj1_intern, obj2_intern;
    struct rt_brep_internal *obj1_brep_ip = NULL, *obj2_brep_ip = NULL;

    RT_DB_INTERNAL_INIT(&obj1_intern);
    RT_DB_INTERNAL_INIT(&obj2_intern);
    if (get_surface(argv[2], dbip, &obj1_intern, &obj1_brep_ip)) return -1;
    if (get_surface(argv[3], dbip, &obj2_intern, &obj2_brep_ip)) return -1;

    // Manipulate the openNURBS objects
    ON_Brep *brep1 = obj1_brep_ip->brep;
    ON_Brep *brep2 = obj2_brep_ip->brep;

    ON_NurbsSurface surf1;
    ON_NurbsSurface surf2;
    ON_SimpleArray<ON_NurbsCurve*> curve;
    ON_SimpleArray<ON_NurbsCurve*> curve_uv;
    ON_SimpleArray<ON_NurbsCurve*> curve_st;

    int a = atoi(argv[4]), b = atoi(argv[5]);
    if (a < 0 || a >= brep1->m_S.Count() || b < 0 || b >= brep2->m_S.Count()) {
	bu_log("Out of range: \n");
	bu_log("\t0 <= i <= %d\n", brep1->m_S.Count() - 1);
	bu_log("\t0 <= j <= %d\n", brep2->m_S.Count() - 1);
	return -1;
    }

    brep1->m_S[a]->GetNurbForm(surf1);
    brep2->m_S[b]->GetNurbForm(surf2);

    // Run the intersection (max_dis = 0)
    if (brlcad::surface_surface_intersection(&surf1, &surf2, curve, curve_uv, curve_st, 0)) {
	bu_log("Intersection failed\n");
	return -1;
    }

    bu_log("%d intersection segments.\n", curve.Count());
    if (curve.Count() == 0)
	return 0;

    bu_vls_init(&intersect_name);
    if (argc == 7) {
	bu_vls_sprintf(&intersect_name, "%s_", argv[6]);
    } else {
	bu_vls_sprintf(&intersect_name, "%s_%s_", argv[2], argv[3]);
    }
    
    // Print the information of an intersection curve, using
    // the ON_NurbsCurve::Dump() method. Later, after ON_SSX_EVENT
    // is improved, we will use ON_SSX_EVENT::Dump() instead.
    bu_log("*** 2D Intersection Curves on Surface A: ***\n");
    for (int i = 0; i < curve_uv.Count(); i++) {
	ON_wString wstr;
	ON_TextLog textlog(wstr);
	curve_uv[i]->Dump(textlog);
	ON_String str = ON_String(wstr);
	char *c_str = str.Array();
	bu_log("Intersection curve %d:\n %s", i + 1, c_str);
    }

    bu_log("*** 2D Intersection Curves on Surface B: ***\n");
    for (int i = 0; i < curve_uv.Count(); i++) {
	ON_wString wstr;
	ON_TextLog textlog(wstr);
	curve_st[i]->Dump(textlog);
	ON_String str = ON_String(wstr);
	char *c_str = str.Array();
	bu_log("Intersection curve %d:\n %s", i + 1, c_str);
    }

    bu_log("*** 3D Intersection Curves: ***\n");
    for (int i = 0; i < curve.Count(); i++) {
	ON_wString wstr;
	ON_TextLog textlog(wstr);
	curve[i]->Dump(textlog);
	ON_String str = ON_String(wstr);
	char *c_str = str.Array();
	bu_log("Intersection curve %d:\n %s", i + 1, c_str);

	// Use a pipe primitive to represent a curve.
	// The CVs of the curve are used as vertexes of the pipe.
	struct rt_db_internal intern;
	RT_DB_INTERNAL_INIT(&intern);
	intern.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	intern.idb_type = ID_PIPE;
	intern.idb_meth = &rt_functab[ID_PIPE];
	BU_ALLOC(intern.idb_ptr, struct rt_pipe_internal);
	struct rt_pipe_internal *pi;
	pi = (struct rt_pipe_internal *)intern.idb_ptr;
	pi->pipe_magic = RT_PIPE_INTERNAL_MAGIC;
	pi->pipe_count = curve.Count();
	BU_LIST_INIT(&(pi->pipe_segs_head));
	struct wdb_pipept *ps;
	
	fastf_t od = curve[i]->BoundingBox().Diagonal().Length() * 0.05;
	for (int j = 0; j < curve[i]->CVCount(); j++) {
	    BU_ALLOC(ps, struct wdb_pipept);
	    ps->l.magic = WDB_PIPESEG_MAGIC;
	    ps->l.back = NULL;
	    ps->l.forw = NULL;
	    ON_3dPoint p;
	    curve[i]->GetCV(j, p);
	    VSET(ps->pp_coord, p.x, p.y, p.z);
	    ps->pp_id = 0.0;
	    ps->pp_od = od;
	    ps->pp_bendradius = 0;
	    BU_LIST_INSERT(&pi->pipe_segs_head, &ps->l);
	}

	bu_vls_init(&name);
	bu_vls_sprintf(&name, "%s%d", bu_vls_addr(&intersect_name), i);

	struct directory *dp;
	dp = db_diradd(dbip, bu_vls_addr(&name), RT_DIR_PHONY_ADDR, 0, RT_DIR_SOLID, (genptr_t)&intern.idb_type);
	ret = rt_db_put_internal(dp, dbip, &intern, &rt_uniresource);
	if (ret)
	    bu_log("ERROR: failure writing [%s] to disk\n", bu_vls_addr(&name));
	else
	    bu_log("%s is written to file.\n", bu_vls_addr(&name));
	bu_vls_free(&name);
    }

    // Free the memory.
    for (int i = 0; i < curve.Count(); i++)
	delete curve[i];
    for (int i = 0; i < curve_uv.Count(); i++)
	delete curve_uv[i];
    for (int i = 0; i < curve_st.Count(); i++)
	delete curve_st[i];

    bu_vls_free(&intersect_name);

    return ret;
}


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
