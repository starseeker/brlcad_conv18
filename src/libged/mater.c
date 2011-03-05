/*                         M A T E R . C
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
/** @file mater.c
 *
 * The mater command.
 *
 */

#include "ged.h"

#include <string.h>


int
ged_mater(struct ged *gedp, int argc, const char *argv[])
{
    static const char *usage = "object_name shader r [g b] inherit";
    static const char *prompt[] = {
	"Name of combination to edit? ", /* unused */
	"Specify shader.  Enclose spaces within quotes. E.g., \"light invisible=1\"\nShader? ('del' to delete, '.' to skip) ",
	"R, G, B color values (0 to 255)? ('del' to delete, '.' to skip) ",
	"G component color value? ('.' to skip) ",
	"B component color value? ('.' to skip) ",
	"Should this object's shader override lower nodes? ('.' to skip) "
    };

    struct directory *dp = NULL;
    int r=0, g=0, b=0;
    struct rt_comb_internal *comb = NULL;
    struct rt_db_internal intern;
    struct bu_vls vls;
    int offset = 0;

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_READ_ONLY(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_HELP;
    }

    GED_DB_LOOKUP(gedp, dp, argv[1], LOOKUP_NOISY, GED_ERROR);
    GED_CHECK_COMB(gedp, dp, GED_ERROR);
    GED_DB_GET_INTERNAL(gedp, &intern, dp, (fastf_t *)NULL, &rt_uniresource, GED_ERROR);

    comb = (struct rt_comb_internal *)intern.idb_ptr;
    RT_CK_COMB(comb);

    /* deleting the color means we don't need to prompt for the green
     * and blue color channels, so offset our argument indices.
     */
    if (argc > 3) {
	if (strncmp(argv[3], "del", 3) == 0) {
	    offset=2;
	}
    }

    /* need more arguments */
    if ((!offset && argc < 7) || (offset && argc < 5)) {
	/* help, let them know the old value */
	if (argc == 2) {
	    bu_vls_printf(&gedp->ged_result_str, "Current shader string = %V\n", &comb->shader);
	} else if (argc == 3) {
	    if (!comb->rgb_valid)
		bu_vls_printf(&gedp->ged_result_str, "Current color = (No color specified)\n");
	    else
		bu_vls_printf(&gedp->ged_result_str, "Current color = %d %d %d\n", V3ARGS(comb->rgb));
	} else if (!offset && argc == 4) {
	    if (comb->rgb_valid)
		bu_vls_printf(&gedp->ged_result_str, "Current green color value = %d\n", comb->rgb[1]);
	} else if (!offset && argc == 5) {
	    if (comb->rgb_valid)
		bu_vls_printf(&gedp->ged_result_str, "Current blue color value = %d\n", comb->rgb[2]);
	} else if ((!offset && argc == 6) || (offset && argc == 4)) {
	    if (comb->inherit)
		bu_vls_printf(&gedp->ged_result_str, "Current inheritance = 1: this node overrides lower nodes\n");
	    else
		bu_vls_printf(&gedp->ged_result_str, "Current inheritance = 0: lower nodes (towards leaves) override\n");
	}

	bu_vls_printf(&gedp->ged_result_str, "%s", prompt[argc+offset-1]);
	return GED_MORE;
    }


    /* too much */
    if ((!offset && argc > 7) || (offset && argc > 5)) {
	bu_vls_printf(&gedp->ged_result_str, "Too many arguments.\nUsage: %s %s", argv[0], usage);
	return GED_ERROR;
    }

    /* Material */
    bu_vls_init(&vls);
    bu_vls_strcat(&vls, argv[2]);
    bu_vls_trimspace(&vls);
    if (bu_vls_strlen(&vls) == 0 || strncmp(bu_vls_addr(&vls), "del", 3) == 0) {
	/* delete the current shader string */
	bu_vls_free(&comb->shader);
    } else {
	if (!BU_STR_EQUAL(bu_vls_addr(&vls), ".")) {
	    bu_vls_trunc(&comb->shader, 0);
	    if (bu_shader_to_tcl_list(bu_vls_addr(&vls), &comb->shader)) {
		bu_vls_printf(&gedp->ged_result_str, "Problem with shader string [%s]", argv[2]);
		rt_db_free_internal(&intern);
		bu_vls_free(&vls);
		return GED_ERROR;
	    }
	}
    }
    bu_vls_free(&vls);

    /* Color */
    if (offset) {  /* means strncmp(argv[3], "del", 3) is 0 */
	/* remove the color */
	comb->rgb_valid = 0;
	comb->rgb[0] = comb->rgb[1] = comb->rgb[2] = 0;
    } else {
	if (BU_STR_EQUAL(argv[3], ".")) {
	    if (!comb->rgb_valid) {
		bu_vls_printf(&gedp->ged_result_str, "Color is not set, cannot skip by using existing color");
		rt_db_free_internal(&intern);
		return GED_ERROR;
	    }
	} else {
	    if (sscanf(argv[3], "%d", &r) != 1 || r < 0 || 255 < r) {
		bu_vls_printf(&gedp->ged_result_str, "Bad color value - %s", argv[3]);
		rt_db_free_internal(&intern);
		return GED_ERROR;
	    }
	    comb->rgb[0] = r;
	}

	if (BU_STR_EQUAL(argv[4], ".")) {
	    if (!comb->rgb_valid) {
		bu_vls_printf(&gedp->ged_result_str, "Color is not set, cannot skip by using existing color");
		rt_db_free_internal(&intern);
		return GED_ERROR;
	    }
	} else {
	    if (sscanf(argv[4], "%d", &g) != 1 || g < 0 || 255 < g) {
		bu_vls_printf(&gedp->ged_result_str, "Bad color value - %s", argv[4]);
		rt_db_free_internal(&intern);
		return GED_ERROR;
	    }
	    comb->rgb[1] = g;
	}

	if (BU_STR_EQUAL(argv[5], ".")) {
	    if (!comb->rgb_valid) {
		bu_vls_printf(&gedp->ged_result_str, "Color is not set, cannot skip by using existing color");
		rt_db_free_internal(&intern);
		return GED_ERROR;
	    }
	} else {
	    if (sscanf(argv[5], "%d", &b) != 1 || b < 0 || 255 < b) {
		bu_vls_printf(&gedp->ged_result_str, "Bad color value - %s", argv[5]);
		rt_db_free_internal(&intern);
		return GED_ERROR;
	    }
	    comb->rgb[2] = b;
	}

	comb->rgb_valid = 1;
    }

    if (!BU_STR_EQUAL(argv[6], ".")) {
	comb->inherit = bu_str_true(argv[6]);
	if (comb->inherit > 1) {
	    bu_vls_printf(&gedp->ged_result_str, "Inherit value should be 0 or 1");
	    rt_db_free_internal(&intern);
	    return GED_ERROR;
	}
    }

    GED_DB_PUT_INTERNAL(gedp, dp, &intern, &rt_uniresource, GED_ERROR);

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
