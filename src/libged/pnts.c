/*                         P N T S . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2018 United States Government as represented by
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
/** @file libged/pnts.c
 *
 * pnts command for simple Point Set (pnts) primitive operations.
 *
 */

/* TODO - merge make_pnts in with this... */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "bu/opt.h"
#include "bu/sort.h"
#include "rt/geom.h"
#include "wdb.h"
#include "analyze.h"
#include "./ged_private.h"


HIDDEN void
_pnts_cmd_help(struct ged *gedp, const char *usage, struct bu_opt_desc *d)
{
    struct bu_vls str = BU_VLS_INIT_ZERO;
    const char *option_help;

    bu_vls_sprintf(&str, "%s", usage);

    if ((option_help = bu_opt_describe(d, NULL))) {
	bu_vls_printf(&str, "Options:\n%s\n", option_help);
	bu_free((char *)option_help, "help str");
    }

    bu_vls_vlscat(gedp->ged_result_str, &str);
    bu_vls_free(&str);
}


HIDDEN void
_pnt_to_tri(point_t *p, vect_t *n, struct rt_bot_internal *bot_ip, fastf_t scale, unsigned long pntcnt)
{
    fastf_t ty1 =  0.57735026918962573 * scale; /* tan(PI/6) */
    fastf_t ty2 = -0.28867513459481287 * scale; /* 0.5 * tan(PI/6) */
    fastf_t tx1 = 0.5 * scale;
    point_t v1, v2, v3;
    vect_t n1;
    vect_t v1p, v2p, v3p = {0.0, 0.0, 0.0};
    vect_t v1f, v2f, v3f = {0.0, 0.0, 0.0};
    mat_t rot;
    struct bn_tol btol = {BN_TOL_MAGIC, BN_TOL_DIST, BN_TOL_DIST * BN_TOL_DIST, 1e-6, 1.0 - 1e-6 };

    VSET(n1, 0, 0, 1);
    VSET(v1, 0, ty1, 0);
    VSET(v2, -1*tx1, ty2, 0);
    VSET(v3, tx1, ty2, 0);

    VMOVE(v1p, v1);
    VMOVE(v2p, v2);
    VMOVE(v3p, v3);
    bn_mat_fromto(rot, n1, *n, &btol);
    MAT4X3VEC(v1f, rot, v1p);
    MAT4X3VEC(v2f, rot, v2p);
    MAT4X3VEC(v3f, rot, v3p);
    VADD2(v1p, v1f, *p);
    VADD2(v2p, v2f, *p);
    VADD2(v3p, v3f, *p);
    VMOVE(&bot_ip->vertices[pntcnt*3*3], v1p);
    VMOVE(&bot_ip->vertices[pntcnt*3*3+3], v2p);
    VMOVE(&bot_ip->vertices[pntcnt*3*3+6], v3p);
    bot_ip->faces[pntcnt*3] = pntcnt*3;
    bot_ip->faces[pntcnt*3+1] = pntcnt*3+1;
    bot_ip->faces[pntcnt*3+2] = pntcnt*3+2;
}

HIDDEN int
_pnts_to_bot(struct ged *gedp, int argc, const char **argv)
{
    int have_normals = 1;
    unsigned long pntcnt = 0;
    struct rt_db_internal intern, internal;
    struct rt_bot_internal *bot_ip;
    struct directory *pnt_dp;
    struct directory *dp;
    int print_help = 0;
    int opt_ret = 0;
    fastf_t scale;
    struct rt_pnts_internal *pnts = NULL;
    const char *pnt_prim= NULL;
    const char *bot_name = NULL;
    const char *usage = "Usage: pnts tri [options] <pnts> <output_bot>\n\n";
    struct bu_opt_desc d[3];
    BU_OPT(d[0], "h", "help",      "",  NULL,            &print_help,   "Print help and exit");
    BU_OPT(d[1], "s", "scale",     "#", &bu_opt_fastf_t, &scale,        "Specify scale factor to apply to unit triangle - will scale the triangle size, with the triangle centered on the original point.");
    BU_OPT_NULL(d[2]);

    argc-=(argc>0); argv+=(argc>0); /* skip command name argv[0] */

    /* must be wanting help */
    if (argc < 1) {
	_pnts_cmd_help(gedp, usage, d);
	return GED_OK;
    }

    /* parse standard options */
    opt_ret = bu_opt_parse(NULL, argc, argv, d);

    if (print_help) {
	_pnts_cmd_help(gedp, usage, d);
	return GED_OK;
    }

    /* adjust argc to match the leftovers of the options parsing */
    argc = opt_ret;

    if (argc != 2) {
	_pnts_cmd_help(gedp, usage, d);
	return GED_ERROR;
    }

    pnt_prim = argv[0];
    bot_name = argv[1];

    /* get pnt */
    GED_DB_LOOKUP(gedp, pnt_dp, pnt_prim, LOOKUP_NOISY, GED_ERROR & GED_QUIET);
    GED_DB_GET_INTERNAL(gedp, &intern, pnt_dp, bn_mat_identity, &rt_uniresource, GED_ERROR);

    if (intern.idb_major_type != DB5_MAJORTYPE_BRLCAD || intern.idb_minor_type != DB5_MINORTYPE_BRLCAD_PNTS) {
	bu_vls_printf(gedp->ged_result_str, "pnts tri: %s is not a pnts object!", pnt_prim);
	rt_db_free_internal(&intern);
	return GED_ERROR;
    }

    pnts = (struct rt_pnts_internal *)intern.idb_ptr;
    RT_PNTS_CK_MAGIC(pnts);

    /* Sanity */
    if (!bot_name || !gedp) return GED_ERROR;
    if (db_lookup(gedp->ged_wdbp->dbip, bot_name, LOOKUP_QUIET) != RT_DIR_NULL) {
	bu_vls_sprintf(gedp->ged_result_str, "Error: object %s already exists!\n", bot_name);
	return GED_ERROR;
    }

    /* For the moment, only generate BoTs when we have a normal to guide us.  Eventually,
     * we might add logic to find the avg center point and calculate normals radiating out
     * from that center, but for now skip anything that doesn't provide normals up front. */
    if (pnts->type == RT_PNT_TYPE_PNT) have_normals = 0;
    if (pnts->type == RT_PNT_TYPE_COL) have_normals = 0;
    if (pnts->type == RT_PNT_TYPE_SCA) have_normals = 0;
    if (pnts->type == RT_PNT_TYPE_COL_SCA) have_normals = 0;
    if (!have_normals) {
	bu_vls_sprintf(gedp->ged_result_str, "Error: point cloud data does not define normals\n");
	return GED_ERROR;
    }

    /* initialize */
    bu_vls_trunc(gedp->ged_result_str, 0);

    if (NEAR_ZERO(scale, SMALL_FASTF)) {
	switch (pnts->type) {
	    case RT_PNT_TYPE_SCA_NRM:
		scale = pnts->scale;
		break;
	    case RT_PNT_TYPE_COL_SCA_NRM:
		scale = pnts->scale;
		break;
	    default:
		scale = 1.0;
		break;
	}
    }

    /* Set up BoT container */
    RT_DB_INTERNAL_INIT(&internal);
    internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
    internal.idb_type = ID_BOT;
    internal.idb_meth = &OBJ[ID_BOT];
    BU_ALLOC(bot_ip, struct rt_bot_internal);
    internal.idb_ptr = (void *)bot_ip;
    bot_ip = (struct rt_bot_internal *)internal.idb_ptr;
    bot_ip->magic = RT_BOT_INTERNAL_MAGIC;
    bot_ip->mode = RT_BOT_SURFACE;
    bot_ip->orientation = 2;

    /* Allocate BoT memory */
    bot_ip->num_vertices = pnts->count * 3;
    bot_ip->num_faces = pnts->count;
    bot_ip->faces = (int *)bu_calloc(bot_ip->num_faces * 3 + 3, sizeof(int), "bot faces");
    bot_ip->vertices = (fastf_t *)bu_calloc(bot_ip->num_vertices * 3 + 3, sizeof(fastf_t), "bot vertices");
    bot_ip->thickness = (fastf_t *)NULL;
    bot_ip->face_mode = (struct bu_bitv *)NULL;

    pntcnt = 0;
    if (pnts->type == RT_PNT_TYPE_NRM) {
	struct pnt_normal *pn = NULL;
	struct pnt_normal *pl = (struct pnt_normal *)pnts->point;
	for (BU_LIST_FOR(pn, pnt_normal, &(pl->l))) {
	    _pnt_to_tri(&(pn->v), &(pn->n), bot_ip, scale, pntcnt);
	    pntcnt++;
	}
    }
    if (pnts->type == RT_PNT_TYPE_COL_NRM) {
	struct pnt_color_normal *pcn = NULL;
	struct pnt_color_normal *pl = (struct pnt_color_normal *)pnts->point;
	for (BU_LIST_FOR(pcn, pnt_color_normal, &(pl->l))) {
	    _pnt_to_tri(&(pcn->v), &(pcn->n), bot_ip, scale, pntcnt);
	    pntcnt++;
	}
    }
    if (pnts->type == RT_PNT_TYPE_SCA_NRM) {
	struct pnt_scale_normal *psn = NULL;
	struct pnt_scale_normal *pl = (struct pnt_scale_normal *)pnts->point;
	for (BU_LIST_FOR(psn, pnt_scale_normal, &(pl->l))) {
	    _pnt_to_tri(&(psn->v), &(psn->n), bot_ip, scale, pntcnt);
	    pntcnt++;
	}
    }
    if (pnts->type == RT_PNT_TYPE_COL_SCA_NRM) {
	struct pnt_color_scale_normal *pcsn = NULL;
	struct pnt_color_scale_normal *pl = (struct pnt_color_scale_normal *)pnts->point;
	for (BU_LIST_FOR(pcsn, pnt_color_scale_normal, &(pl->l))) {
	    _pnt_to_tri(&(pcsn->v), &(pcsn->n), bot_ip, scale, pntcnt);
	    pntcnt++;
	}
    }

    GED_DB_DIRADD(gedp, dp, bot_name, RT_DIR_PHONY_ADDR, 0, RT_DIR_SOLID, (void *)&internal.idb_type, GED_ERROR);
    GED_DB_PUT_INTERNAL(gedp, dp, &internal, &rt_uniresource, GED_ERROR);

    bu_vls_printf(gedp->ged_result_str, "Generated BoT object %s with %d triangles", bot_name, pntcnt);

    rt_db_free_internal(&intern);
    return GED_OK;
}

HIDDEN int
_obj_to_pnts(struct ged *gedp, int argc, const char **argv)
{
    struct directory *dp;
    int print_help = 0;
    int opt_ret = 0;
    fastf_t len_tol = 0.0;
    int pnt_mode = 0;
    struct rt_db_internal internal;
    struct bn_tol btol = {BN_TOL_MAGIC, BN_TOL_DIST, BN_TOL_DIST * BN_TOL_DIST, 1e-6, 1.0 - 1e-6 };
    struct rt_pnts_internal *pnts = NULL;
    const char *pnt_prim= NULL;
    const char *obj_name = NULL;
    const char *usage = "Usage: pnts gen [options] <obj> <output_pnts>\n\n";
    struct bu_opt_desc d[4];
    BU_OPT(d[0], "h", "help",      "",  NULL,            &print_help,   "Print help and exit");
    BU_OPT(d[1], "t", "tolerance", "#", &bu_opt_fastf_t, &len_tol,      "Specify sampling grid spacing (in mm).");
    BU_OPT(d[2], "S", "surface",   "",  NULL,            &pnt_mode,     "Save only first and last points along ray.");
    BU_OPT_NULL(d[2]);

    argc-=(argc>0); argv+=(argc>0); /* skip command name argv[0] */

    /* must be wanting help */
    if (argc < 1) {
	_pnts_cmd_help(gedp, usage, d);
	return GED_OK;
    }

    /* parse standard options */
    opt_ret = bu_opt_parse(NULL, argc, argv, d);

    if (print_help) {
	_pnts_cmd_help(gedp, usage, d);
	return GED_OK;
    }

    /* adjust argc to match the leftovers of the options parsing */
    argc = opt_ret;

    if (argc != 2) {
	_pnts_cmd_help(gedp, usage, d);
	return GED_ERROR;
    }

    obj_name = argv[0];
    pnt_prim = argv[1];

    /* Sanity */
    if (db_lookup(gedp->ged_wdbp->dbip, obj_name, LOOKUP_QUIET) == RT_DIR_NULL) {
	bu_vls_sprintf(gedp->ged_result_str, "Error: object %s doesn't exist!\n", obj_name);
	return GED_ERROR;
    }
    if (db_lookup(gedp->ged_wdbp->dbip, pnt_prim, LOOKUP_QUIET) != RT_DIR_NULL) {
	bu_vls_sprintf(gedp->ged_result_str, "Error: object %s already exists!\n", pnt_prim);
	return GED_ERROR;
    }

    /* If we don't have a tolerance, try to guess something sane from the bbox */
    if (NEAR_ZERO(len_tol, RT_LEN_TOL)) {
	point_t rpp_min, rpp_max;
	point_t obj_min, obj_max;
	VSETALL(rpp_min, INFINITY);
	VSETALL(rpp_max, -INFINITY);
	ged_get_obj_bounds(gedp, 1, (const char **)&obj_name, 0, obj_min, obj_max);
	VMINMAX(rpp_min, rpp_max, (double *)obj_min);
	VMINMAX(rpp_min, rpp_max, (double *)obj_max);
	len_tol = DIST_PT_PT(rpp_max, rpp_min) * 0.01;
	bu_log("Note - no tolerance specified, using %f\n", len_tol);
    }
    btol.dist = len_tol;

    RT_DB_INTERNAL_INIT(&internal);
    internal.idb_major_type = DB5_MAJORTYPE_BRLCAD;
    internal.idb_type = ID_PNTS;
    internal.idb_meth = &OBJ[ID_PNTS];
    BU_ALLOC(internal.idb_ptr, struct rt_pnts_internal);
    pnts = (struct rt_pnts_internal *) internal.idb_ptr;
    pnts->magic = RT_PNTS_INTERNAL_MAGIC;
    pnts->scale = 0.0;
    pnts->type = RT_PNT_TYPE_NRM;

    if (analyze_obj_to_pnts(pnts, gedp->ged_wdbp->dbip, obj_name, &btol, pnt_mode)) {
	bu_vls_sprintf(gedp->ged_result_str, "Error: point generation failed\n");
	return GED_ERROR;
    }

    GED_DB_DIRADD(gedp, dp, pnt_prim, RT_DIR_PHONY_ADDR, 0, RT_DIR_SOLID, (void *)&internal.idb_type, GED_ERROR);
    GED_DB_PUT_INTERNAL(gedp, dp, &internal, &rt_uniresource, GED_ERROR);

    bu_vls_printf(gedp->ged_result_str, "Generated pnts object %s with %d points", pnt_prim, pnts->count);

    return GED_OK;
}

HIDDEN void
_pnts_show_help(struct ged *gedp, struct bu_opt_desc *d)
{
    struct bu_vls str = BU_VLS_INIT_ZERO;
    const char *option_help;

    bu_vls_sprintf(&str, "Usage: pnts [options] [subcommand] [subcommand arguments]\n\n");

    if ((option_help = bu_opt_describe(d, NULL))) {
	bu_vls_printf(&str, "Options:\n%s\n", option_help);
	bu_free((char *)option_help, "help str");
    }
    bu_vls_printf(&str, "Subcommands:\n\n");
    bu_vls_printf(&str, "  get [-p][-n][-t dist_tol] [-k px py pz] (pnt_ind|pnt_ind_min-pnt_ind_max) <pnts> [new_pnts_obj]\n");
    bu_vls_printf(&str, "    - Get specific subset (1 or more) points from a point set and\n");
    bu_vls_printf(&str, "      print information. (todo - document subcmd options...)\n\n");
    bu_vls_printf(&str, "  read <pnts> input_file format units pnt_size\n");
    bu_vls_printf(&str, "    - Read data from an input file into a pnts object\n\n");
    bu_vls_printf(&str, "  gen [-t tol] <obj> <output_pnts>\n");
    bu_vls_printf(&str, "    - Generate a point set from the object and store the set in a points object.\n");
    bu_vls_printf(&str, "      Collects the first and last hit and normal for any given ray, generating\n");
    bu_vls_printf(&str, "      an \"outer surface\" point set.\n\n");
    bu_vls_printf(&str, "  tri [-s scale] <pnts> <output_bot>\n");
    bu_vls_printf(&str, "    - Generate unit or scaled triangles for each pnt in a point set. If no normal\n");
    bu_vls_printf(&str, "      information is present, use origin at avg of all set points to make normals.\n\n");
    bu_vls_printf(&str, "  [TODO] chull <pnts> <output_bot>\n");
    bu_vls_printf(&str, "    - Store the convex hull of the point set in <output_bot>.\n\n");
    bu_vls_printf(&str, "  [TODO] generate [-f format] <obj> [new_pnts_obj]\n");
    bu_vls_printf(&str, "    - Generate a point set from an existing database object (terminate using the stability of a volume calculation? -- might be merged with get...)\n");
    bu_vls_vlscat(gedp->ged_result_str, &str);
    bu_vls_free(&str);
}


#if 0
HIDDEN int
pnts_get(struct ged *gedp, int argc, const char *argv[], struct bu_opt_desc *d, struct rt_db_internal *ip)
{
    struct rt_pnts_internal *pnts = (struct rt_pnts_internal *)ip->idb_ptr;
    return GED_OK;
}

HIDDEN int
pnts_add(struct ged *gedp, int argc, const char *argv[], struct bu_opt_desc *d, struct rt_db_internal *ip)
{
    struct rt_pnts_internal *pnts = (struct rt_pnts_internal *)ip->idb_ptr;
    return GED_OK;
}

HIDDEN int
pnts_rm(struct ged *gedp, int argc, const char *argv[], struct bu_opt_desc *d, struct rt_db_internal *ip)
{
    struct rt_pnts_internal *pnts = (struct rt_pnts_internal *)ip->idb_ptr;
    return GED_OK;
}

HIDDEN int
pnts_chull(struct ged *gedp, int argc, const char *argv[], struct bu_opt_desc *d, struct rt_db_internal *ip)
{
    struct rt_pnts_internal *pnts = (struct rt_pnts_internal *)ip->idb_ptr;
    return GED_OK;
}

#endif


int
ged_pnts(struct ged *gedp, int argc, const char *argv[])
{
    const char *cmd = argv[0];
    size_t len;
    int i;
    int print_help = 0;
    int opt_ret = 0;
    int opt_argc = argc;
    struct bu_opt_desc d[2];
    const char * const pnt_subcommands[] = {"gen", "get", "read", "tri", NULL};
    const char * const *subcmd;

    BU_OPT(d[0], "h", "help",      "", NULL, &print_help,        "Print help and exit");
    BU_OPT_NULL(d[1]);

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_READ_ONLY(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    argc-=(argc>0); argv+=(argc>0); /* skip command name argv[0] */

    /* must be wanting help */
    if (argc < 1) {
	_pnts_show_help(gedp, d);
	return GED_OK;
    }

    /* See if we have any options to deal with.  Once we hit a subcommand, we're done */
    for (i = 0; i < argc; ++i) {
	subcmd = pnt_subcommands;
	for (; *subcmd != NULL; ++subcmd) {
	    if (BU_STR_EQUAL(argv[i], *subcmd)) {
		opt_argc = i;
		i = argc;
		break;
	    }
	}
    }

    if (opt_argc > 0) {
	/* parse standard options */
	opt_ret = bu_opt_parse(NULL, opt_argc, argv, d);
	if (opt_ret < 0) {
	    _pnts_show_help(gedp, d);
	    return GED_ERROR;
	}
    }

    if (print_help) {
	_pnts_show_help(gedp, d);
	return GED_OK;
    }

    /* shift argv to subcommand */
    argc -= opt_argc;
    argv = &argv[opt_argc];

    /* If we don't have a subcommand, we're done */
    if (argc < 1) {
	_pnts_show_help(gedp, d);
	return GED_ERROR;
    }

    len = strlen(argv[0]);
    if (bu_strncmp(argv[0], "tri", len) == 0) return _pnts_to_bot(gedp, argc, argv);

    if (bu_strncmp(argv[0], "gen", len) == 0) return _obj_to_pnts(gedp, argc, argv);

    /* If we don't have a valid subcommand, we're done */
    bu_vls_printf(gedp->ged_result_str, "%s: %s is not a known subcommand!\n", cmd, argv[0]);
    _pnts_show_help(gedp, d);
    return GED_ERROR;
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
