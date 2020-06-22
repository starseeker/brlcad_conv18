/*                      D A T A _ L I N E S . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2020 United States Government as represented by
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
/** @file libged/data_lines.c
 *
 * Logic for drawing arbitary lines not associated with geometry.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "bu/cmd.h"
#include "bu/opt.h"
#include "bu/str.h"
#include "bu/vls.h"
#include "../ged_private.h"

struct view_dlines_state {
    struct ged *gedp;
    struct bview_data_line_state *gdlsp;
};

static int
_view_dlines_cmd_draw(void *bs, int argc, const char **argv)
{
    struct view_dlines_state *vs = (struct view_dlines_state *)bs;
    struct ged *gedp = vs->gedp;
    struct bview_data_line_state *gdlsp = vs->gdlsp;
    if (argc == 2) {
	bu_vls_printf(gedp->ged_result_str, "%d", gdlsp->gdls_draw);
	return GED_OK;
    }

    if (argc == 3) {
	int i;

	if (bu_sscanf(argv[2], "%d", &i) != 1) return GED_ERROR;

	gdlsp->gdls_draw = (i) ? 1 : 0;

	gedp->ged_gvp->gv_refresh = 1;
	return GED_OK;
    }

    return GED_ERROR;
}

static int
_view_dlines_cmd_color(void *bs, int argc, const char **argv)
{
    struct view_dlines_state *vs = (struct view_dlines_state *)bs;
    struct ged *gedp = vs->gedp;
    struct bview_data_line_state *gdlsp = vs->gdlsp;

    if (argc == 2) {
	bu_vls_printf(gedp->ged_result_str, "%d %d %d", V3ARGS(gdlsp->gdls_color));
	return GED_OK;
    }

    if (argc == 5) {
	int r, g, b;

	/* set background color */
	if (bu_sscanf(argv[2], "%d", &r) != 1 ||
		bu_sscanf(argv[3], "%d", &g) != 1 ||
		bu_sscanf(argv[4], "%d", &b) != 1)
	    return GED_ERROR;

	/* validate color */
	if (r < 0 || 255 < r ||
		g < 0 || 255 < g ||
		b < 0 || 255 < b)
	    return GED_ERROR;

	VSET(gdlsp->gdls_color, r, g, b);

	gedp->ged_gvp->gv_refresh = 1;
	return GED_OK;
    }

    return GED_ERROR;
}

static int
_view_dlines_cmd_line_width(void *bs, int argc, const char **argv)
{
    struct view_dlines_state *vs = (struct view_dlines_state *)bs;
    struct ged *gedp = vs->gedp;
    struct bview_data_line_state *gdlsp = vs->gdlsp;

    if (argc == 2) {
	bu_vls_printf(gedp->ged_result_str, "%d", gdlsp->gdls_line_width);
	return GED_OK;
    }

    if (argc == 3) {
	int line_width;

	if (bu_sscanf(argv[2], "%d", &line_width) != 1)
	    return GED_ERROR;

	gdlsp->gdls_line_width = line_width;

	gedp->ged_gvp->gv_refresh = 1;
	return GED_OK;
    }

    return GED_ERROR;
}

static int
_view_dlines_cmd_points(void *bs, int argc, const char **argv)
{
    struct view_dlines_state *vs = (struct view_dlines_state *)bs;
    struct ged *gedp = vs->gedp;
    struct bview_data_line_state *gdlsp = vs->gdlsp;
    int i;

    if (argc == 2) {
	for (i = 0; i < gdlsp->gdls_num_points; ++i) {
	    bu_vls_printf(gedp->ged_result_str, " {%lf %lf %lf} ", V3ARGS(gdlsp->gdls_points[i]));
	}
	return GED_OK;
    }

    if (argc == 3) {
	int ac;
	const char **av;

	if (bu_argv_from_tcl_list(argv[2], &ac, &av)) {
	    bu_vls_printf(gedp->ged_result_str, "failed to parse list");
	    return GED_ERROR;
	}

	if (ac % 2) {
	    bu_vls_printf(gedp->ged_result_str, "%s: must be an even number of points", argv[0]);
	    return GED_ERROR;
	}

	if (gdlsp->gdls_num_points) {
	    bu_free((void *)gdlsp->gdls_points, "data points");
	    gdlsp->gdls_points = (point_t *)0;
	    gdlsp->gdls_num_points = 0;
	}

	/* Clear out data points */
	if (ac < 1) {
	    gedp->ged_gvp->gv_refresh = 1;
	    bu_free((char *)av, "av");
	    return GED_OK;
	}

	gdlsp->gdls_num_points = ac;
	gdlsp->gdls_points = (point_t *)bu_calloc(ac, sizeof(point_t), "data points");
	for (i = 0; i < ac; ++i) {
	    double scan[3];

	    if (bu_sscanf(av[i], "%lf %lf %lf", &scan[X], &scan[Y], &scan[Z]) != 3) {
		bu_vls_printf(gedp->ged_result_str, "bad data point - %s\n", av[i]);

		bu_free((void *)gdlsp->gdls_points, "data points");
		gdlsp->gdls_points = (point_t *)0;
		gdlsp->gdls_num_points = 0;

		gedp->ged_gvp->gv_refresh = 1;
		bu_free((char *)av, "av");
		return GED_ERROR;
	    }
	    /* convert double to fastf_t */
	    VMOVE(gdlsp->gdls_points[i], scan);
	}

	gedp->ged_gvp->gv_refresh = 1;
	bu_free((char *)av, "av");
	return GED_OK;
    }

    return GED_ERROR;
}

const struct bu_cmdtab _view_dline_cmds[] = {
    { "draw",       _view_dlines_cmd_draw},
    { "color",      _view_dlines_cmd_color},
    { "line_width", _view_dlines_cmd_line_width},
    { "points",     _view_dlines_cmd_points},
    { (char *)NULL,      NULL}
};

int
ged_view_data_lines(struct ged *gedp, int argc, const char *argv[])
{
    struct view_dlines_state vs;
    const char *usage = "view [s]data_lines [subcommand]";

    vs.gedp = gedp;

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_HELP;
    }

    if (6 < argc) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }

    if (argv[0][0] == 's') {
	vs.gdlsp = &gedp->ged_gvp->gv_sdata_lines;
    } else {
	vs.gdlsp = &gedp->ged_gvp->gv_data_lines;
    }

    if (bu_cmd_valid(_view_dline_cmds, argv[1]) != BRLCAD_OK) {
    	bu_vls_printf(gedp->ged_result_str, "invalid subcommand: %s", argv[1]);
	return GED_ERROR;
    }

    int ret;
    if (bu_cmd(_view_dline_cmds, argc, argv, 0, (void *)&vs, &ret) == BRLCAD_OK) {
        return GED_OK;
    }

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
