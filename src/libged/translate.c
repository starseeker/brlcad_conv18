/*                         T R A N S L A T E . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2011 United States Government as represented by
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
/** @file libged/translate.c
 *
 * The translate command.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "vmath.h"
#include "raytrace.h"
#include "ged.h"

int
ged_translate(struct ged *gedp, int argc, const char *argv[])
{
    struct db_i *dbip = gedp->ged_wdbp->dbip;
    const char *cmd_name = argv[0];
    static const char *usage = "[-k keypoint:object]"
			       " [[-a] | [-r]]"
			       " x [y [z]]"
			       " combination"
			       " object";
    int i;				/* iterator */
    int c;				/* bu_getopt return value */
    int abs_flag = 0;			/* use absolute position */
    int rel_flag = 0;			/* use relative distance */
    char *kp_arg = NULL;        	/* keypoint argument */
    const char *s_comb;			/* combination string */
    const char *s_obj;			/* object string */
    const char *s_full_path;		/* comb + obj string */
    struct db_full_path comb;
    struct db_full_path obj;		/* FIXME: needs to handle >1 obj */
    struct db_full_path full_path;	/* comb + obj path */
    struct directory *full_dir;         /* comb + obj directory */
    struct directory *comb_dir;
    point_t keypoint;
    char *endchr = NULL;       /* for strtof's */

        /* testing */
    mat_t modelchanges, incr, old;
    vect_t model_incr, ed_sol_pt, new_vertex;

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_READ_ONLY(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help -- argc < 4 is wrong too, but more helpful
       msgs may be given later, by saying which args are missing */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", cmd_name, usage);
	return GED_HELP;
    }

    /* get short arguments */
    bu_optind = 1; /* re-init bu_getopt() */
    while ((c = bu_getopt(argc, (char * const *)argv, "ak:r")) != -1) {
	switch (c) {
	case 'a':
	    abs_flag = 1;
	    break;
	case 'k':
	    kp_arg = bu_optarg;
	    if (kp_arg[0] == '-') {
		/* that's an option, not an arg */
		bu_vls_printf(&gedp->ged_result_str,
			      "Missing argument for option -%c", bu_optopt);
		return GED_ERROR;
	    }
	    break;
	case 'r':
	    rel_flag = 1;
	    break;
	default:
	    /* options that require arguments */
	    switch (bu_optopt) {
	    case 'k':
		bu_vls_printf(&gedp->ged_result_str,
			      "Missing argument for option -%c", bu_optopt);
		return GED_ERROR;
	    }

	    /* unknown options */
	    if (isprint(bu_optopt)) {
		bu_vls_printf(&gedp->ged_result_str,
			      "Unknown option '-%c'", bu_optopt);
		return GED_ERROR;
	    } else {
		bu_vls_printf(&gedp->ged_result_str,
			      "Unknown option character '\\x%x'",
			      bu_optopt);
		return GED_ERROR;
	    }
	}
    }

    /* needs to be either absolute or relative positioning */
    if (!abs_flag && !rel_flag)
	rel_flag = 1;
    else if (abs_flag && rel_flag) {
	bu_vls_printf(&gedp->ged_result_str,
		      "options '-a' and '-r' are mutually exclusive");
	return GED_ERROR;
    }


    /* FIXME: set reasonable default keypoint */
    #if 0
    if (!keypoint)
	;
    #endif

    /* testing */
    keypoint[0] = 0;
    keypoint[1] = 0;
    keypoint[2] = 0;

    /* set 3d coords */
    if ((bu_optind + 1) > argc) {
	bu_vls_printf(&gedp->ged_result_str, "missing x coordinate");
	return GED_HELP;
    }
    new_vertex[0] = strtof(argv[bu_optind], &endchr);
    if (!endchr || argv[bu_optind] == endchr) {
	bu_vls_printf(&gedp->ged_result_str, "missing or invalid x coordinate");
	return GED_ERROR;
    }
    ++bu_optind;
    for (i = 1; i < 3; ++i, ++bu_optind) {
	if ((bu_optind + 1) > argc)
	    break;
	new_vertex[i] = strtof(argv[bu_optind], &endchr);
	if (!endchr || argv[bu_optind] == endchr)
	    /* invalid y or z coord */
	    break;
    }

    /* set combination object */
    if ((bu_optind + 1) > argc) {
	bu_vls_printf(&gedp->ged_result_str,
		      "missing 'combination' and 'object' arguments\n");
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", cmd_name, usage);
	return GED_HELP;
    }
    s_comb = argv[bu_optind++];
    if (db_string_to_path(&comb, dbip, s_comb) < 0) {
	bu_vls_printf(&gedp->ged_result_str, "invalid path to 'combination'");
	return GED_ERROR;
    }

    /* set object being translated */
    if ((bu_optind + 1) > argc) {
	/* assume the last path was the combination */
	bu_vls_printf(&gedp->ged_result_str,
		      "missing 'object' argument\n");
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", cmd_name, usage);
	return GED_HELP;
    }
    /* FIXME: needs to handle >1 obj */
    s_obj = argv[bu_optind++];
    if (db_string_to_path(&obj, dbip, s_obj) < 0) {
	db_free_full_path(&comb);
	bu_vls_printf(&gedp->ged_result_str, "bad object path");
	return GED_ERROR;
    }

    /* verify existence of combination path */
    comb_dir = db_lookup(dbip, s_comb, LOOKUP_NOISY);
    if (comb_dir == RT_DIR_NULL) {
	bu_vls_printf(&gedp->ged_result_str, "invalid path to 'combination'");
	return GED_ERROR;
    }
    /* verify existence of obj within combination path */
    db_full_path_init(&full_path);
    db_dup_full_path(&full_path, &comb);
    db_append_full_path(&full_path, &obj);
    s_full_path = db_path_to_string(&full_path);
    full_dir = db_lookup(dbip, s_full_path, LOOKUP_NOISY);
    if (full_dir == RT_DIR_NULL) {
	bu_vls_printf(&gedp->ged_result_str, "'object' is not within"
					     " 'combination'");
	return GED_ERROR;
    }

    /* FIXME: perform equivalent of mged matpick, privately */



    /* XXX disabled until functional */
    if ((bu_optind + 1) <= argc)
	bu_vls_printf(&gedp->ged_result_str, "multiple objects not yet"
					     " supported; ");
    goto disabled;

    /* do translation */
    MAT_IDN(incr);
    MAT_IDN(old);
    MAT4X3PNT(ed_sol_pt, modelchanges, keypoint);
    VSUB2(model_incr, new_vertex, ed_sol_pt);
    MAT_DELTAS_VEC(incr, model_incr);
    MAT_COPY(old, modelchanges);
    bn_mat_mul(modelchanges, incr, old);
    /* new_edit_mats(); */

    db_free_full_path(&comb);
    db_free_full_path(&obj);
    return GED_OK;

    disabled:
    db_free_full_path(&comb);
    db_free_full_path(&obj);
    bu_vls_printf(&gedp->ged_result_str, "function not yet implemented");
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
