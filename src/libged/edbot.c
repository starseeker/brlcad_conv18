/*                        E D B O T . C
 * BRL-CAD
 *
 * Copyright (c) 1995-2012 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file libged/edbot.c
 *
 */

#include "common.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "bio.h"
#include "vmath.h"
#include "nmg.h"
#include "rtgeom.h"
#include "ged.h"
#include "nurb.h"
#include "wdb.h"



int
ged_bot_face_split(struct ged *gedp, int argc, const char *argv[])
{
    static const char *usage = "bot face";
    struct directory *dp;
    static fastf_t sf = 1.0 / 3.0;
    struct rt_db_internal intern;
    struct rt_bot_internal *botip;
    mat_t mat;
    char *last;
    size_t face_i;
    size_t last_vi;
    size_t save_vi;
    point_t new_pt;

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_HELP;
    }

    if (argc != 3) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }

    if ((last = strrchr(argv[1], '/')) == NULL)
	last = (char *)argv[1];
    else
	++last;

    if (last[0] == '\0') {
	bu_vls_printf(gedp->ged_result_str, "%s: illegal input - %s", argv[0], argv[1]);
	return GED_ERROR;
    }

    dp = db_lookup(gedp->ged_wdbp->dbip, last, LOOKUP_QUIET);
    if (dp == RT_DIR_NULL) {
	bu_vls_printf(gedp->ged_result_str, "%s: failed to find %s", argv[0], argv[1]);
	return GED_ERROR;
    }

    if (sscanf(argv[2], "%zu", &face_i) != 1) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad bot vertex index - %s", argv[0], argv[2]);
	return GED_ERROR;
    }

    if (wdb_import_from_path2(gedp->ged_result_str, &intern, argv[1], gedp->ged_wdbp, mat) == GED_ERROR) {
	bu_vls_printf(gedp->ged_result_str, "%s: failed to find %s", argv[0], argv[1]);
	return GED_ERROR;
    }

    if (intern.idb_major_type != DB5_MAJORTYPE_BRLCAD ||
	intern.idb_minor_type != DB5_MINORTYPE_BRLCAD_BOT) {
	bu_vls_printf(gedp->ged_result_str, "Object is not a BOT");
	rt_db_free_internal(&intern);

	return GED_ERROR;
    }

    botip = (struct rt_bot_internal *)intern.idb_ptr;
    last_vi = botip->num_vertices;

    if (face_i >= botip->num_faces) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad bot face index - %s", argv[0], argv[2]);
	rt_db_free_internal(&intern);
	return GED_ERROR;
    }

    /* Create the new point, modify face_i and hook in the two extra faces */
    /* First, create some space */
    botip->num_vertices++;
    botip->num_faces += 2;
    botip->vertices = bu_realloc((genptr_t)botip->vertices, botip->num_vertices*3*sizeof(fastf_t), "realloc bot vertices");
    botip->faces = bu_realloc((genptr_t)botip->faces, botip->num_faces*3*sizeof(int), "realloc bot faces");

    /* Create the new point. For the moment, we're using the average of the face_i's points */
    VADD3(new_pt,
	  &botip->vertices[botip->faces[face_i*3]*3],
	  &botip->vertices[botip->faces[face_i*3+1]*3],
	  &botip->vertices[botip->faces[face_i*3+2]*3]);
    VSCALE(new_pt, new_pt, sf);
    
    /* Add the new point to the last position in the list of vertices. */
    VMOVE(&botip->vertices[last_vi*3], new_pt);

    /* Update face_i */
    save_vi = botip->faces[face_i*3+2];
    botip->faces[face_i*3+2] = last_vi;

    /* Initialize the two new faces */
    botip->faces[(botip->num_faces-2)*3] = botip->faces[face_i*3+1];
    botip->faces[(botip->num_faces-2)*3+1] = save_vi;
    botip->faces[(botip->num_faces-2)*3+2] = last_vi;
    botip->faces[(botip->num_faces-1)*3] = save_vi;
    botip->faces[(botip->num_faces-1)*3+1] = botip->faces[face_i*3];
    botip->faces[(botip->num_faces-1)*3+2] = last_vi;

    bu_vls_printf(gedp->ged_result_str, "%zu", last_vi);

    GED_DB_PUT_INTERNAL(gedp, dp, &intern, &rt_uniresource, GED_ERROR);
    rt_db_free_internal(&intern);
    return GED_OK;
}


int
ged_find_botpt_nearest_pt(struct ged *gedp, int argc, const char *argv[])
{
    static const char *usage = "bot view_xyz";
    struct rt_db_internal intern;
    struct rt_bot_internal *botip;
    mat_t mat;
    char *last;
    vect_t view;
    int nearest_pt;

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_VIEW(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_HELP;
    }

    if (argc != 3) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }

    if ((last = strrchr(argv[1], '/')) == NULL)
	last = (char *)argv[1];
    else
	++last;

    if (last[0] == '\0') {
	bu_vls_printf(gedp->ged_result_str, "%s: illegal input - %s", argv[0], argv[1]);
	return GED_ERROR;
    }

    if (sscanf(argv[2], "%lf %lf %lf", &view[X], &view[Y], &view[Z]) != 3) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad view location - %s", argv[0], argv[2]);
	return GED_ERROR;
    }

    if (wdb_import_from_path2(gedp->ged_result_str, &intern, argv[1], gedp->ged_wdbp, mat) == GED_ERROR) {
	bu_vls_printf(gedp->ged_result_str, "%s: failed to find %s", argv[0], argv[1]);
	return GED_ERROR;
    }

    if (intern.idb_major_type != DB5_MAJORTYPE_BRLCAD ||
	intern.idb_minor_type != DB5_MINORTYPE_BRLCAD_BOT) {
	bu_vls_printf(gedp->ged_result_str, "Object is not a BOT");
	rt_db_free_internal(&intern);

	return GED_ERROR;
    }

    botip = (struct rt_bot_internal *)intern.idb_ptr;
    nearest_pt = rt_bot_find_v_nearest_pt2(botip, view, gedp->ged_gvp->gv_model2view);
    bu_vls_printf(gedp->ged_result_str, "%d", nearest_pt);

    rt_db_free_internal(&intern);
    return GED_OK;
}


int
ged_move_botpt(struct ged *gedp, int argc, const char *argv[])
{
    static const char *usage = "[-r] bot vertex_i pt";
    struct directory *dp;
    struct rt_db_internal intern;
    struct rt_bot_internal *botip;
    mat_t mat;
    point_t pt;
    size_t vertex_i;
    int rflag = 0;
    char *last;

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_HELP;
    }

    if (argc < 4 || 5 < argc) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }

    if (argc == 5) {
	if (argv[1][0] != '-' || argv[1][1] != 'r' || argv[1][2] != '\0') {
	    bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	    return GED_ERROR;
	}

	rflag = 1;
	--argc;
	++argv;
    }

    if ((last = strrchr(argv[1], '/')) == NULL)
	last = (char *)argv[1];
    else
	++last;

    if (last[0] == '\0') {
	bu_vls_printf(gedp->ged_result_str, "%s: illegal input - %s", argv[0], argv[1]);
	return GED_ERROR;
    }

    dp = db_lookup(gedp->ged_wdbp->dbip, last, LOOKUP_QUIET);
    if (dp == RT_DIR_NULL) {
	bu_vls_printf(gedp->ged_result_str, "%s: failed to find %s", argv[0], argv[1]);
	return GED_ERROR;
    }

    if (sscanf(argv[2], "%zu", &vertex_i) != 1) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad bot vertex index - %s", argv[0], argv[2]);
	return GED_ERROR;
    }

    if (sscanf(argv[3], "%lf %lf %lf", &pt[X], &pt[Y], &pt[Z]) != 3) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad point - %s", argv[0], argv[3]);
	return GED_ERROR;
    }

    VSCALE(pt, pt, gedp->ged_wdbp->dbip->dbi_local2base);

    if (wdb_import_from_path2(gedp->ged_result_str, &intern, argv[1], gedp->ged_wdbp, mat) == GED_ERROR) {
	bu_vls_printf(gedp->ged_result_str, "%s: failed to find %s", argv[0], argv[1]);
	return GED_ERROR;
    }

    if (intern.idb_major_type != DB5_MAJORTYPE_BRLCAD ||
	intern.idb_minor_type != DB5_MINORTYPE_BRLCAD_BOT) {
	bu_vls_printf(gedp->ged_result_str, "Object is not a BOT");
	rt_db_free_internal(&intern);

	return GED_ERROR;
    }

    botip = (struct rt_bot_internal *)intern.idb_ptr;

    if (vertex_i >= botip->num_vertices) {
	bu_vls_printf(gedp->ged_result_str, "%s: bad bot vertex index - %s", argv[0], argv[2]);
	rt_db_free_internal(&intern);
	return GED_ERROR;
    }

    if (rflag) {
	VADD2(&botip->vertices[vertex_i*3], pt, &botip->vertices[vertex_i*3]);
    } else {
	VMOVE(&botip->vertices[vertex_i*3], pt);
    }

    {
	mat_t invmat;
	point_t curr_pt;
	size_t idx;

	bn_mat_inv(invmat, mat);
	for (idx = 0; idx < botip->num_vertices; idx++) {
	    MAT4X3PNT(curr_pt, invmat, &botip->vertices[idx*3]);
	    VMOVE(&botip->vertices[idx*3], curr_pt);
	}
    }

    GED_DB_PUT_INTERNAL(gedp, dp, &intern, &rt_uniresource, GED_ERROR);
    rt_db_free_internal(&intern);

    return GED_OK;
}



/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
