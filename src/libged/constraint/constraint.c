/*                    C O N S T R A I N T . C
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

#include "common.h"

#include "bu.h"
#include "cmd.h"
#include "ged.h"


HIDDEN int
constraint_set(void *datap, int argc, const char *argv[])
{
    struct directory *dp;
    struct ged *gedp = (struct ged *)datap;
    if (!gedp || argc < 1 || !argv)
	return BRLCAD_ERROR;

    /* load the constraint object */
    GED_DB_LOOKUP(gedp, dp, argv[2], LOOKUP_QUIET, BRLCAD_ERROR);

    bu_vls_printf(gedp->ged_result_str, "<<constraint set here>>\n");

    return BRLCAD_OK;
}


HIDDEN int
constraint_get(void *datap, int argc, const char *argv[])
{
    struct directory *dp;
    struct ged *gedp = (struct ged *)datap;
    if (!gedp || argc < 1 || !argv)
	return BRLCAD_ERROR;

    /* load the constraint object */
    GED_DB_LOOKUP(gedp, dp, argv[2], LOOKUP_QUIET, BRLCAD_ERROR);

    bu_vls_printf(gedp->ged_result_str, "<<constraint get here>>\n");

    return BRLCAD_OK;
}


HIDDEN int
constraint_show(void *datap, int argc, const char *argv[])
{
    struct directory *dp;
    struct ged *gedp = (struct ged *)datap;
    if (!gedp || argc < 1 || !argv)
	return BRLCAD_ERROR;

    /* load the constraint object */
    GED_DB_LOOKUP(gedp, dp, argv[2], LOOKUP_QUIET, BRLCAD_ERROR);

    bu_vls_printf(gedp->ged_result_str, "Constraint %s:\n", argv[2]);
    bu_vls_printf(gedp->ged_result_str, "<<constraint show here>>\n");

    return BRLCAD_OK;
}


HIDDEN int
constraint_rm(void *datap, int argc, const char *argv[])
{
    struct directory *dp;
    struct ged *gedp = (struct ged *)datap;
    if (!gedp || argc < 1 || !argv)
	return BRLCAD_ERROR;

    /* load the constraint object */
    GED_DB_LOOKUP(gedp, dp, argv[2], LOOKUP_QUIET, BRLCAD_ERROR);

    bu_vls_printf(gedp->ged_result_str, "Removing %s constraint\n", argv[2]);

    return BRLCAD_OK;
}


HIDDEN int
constraint_eval(void *datap, int argc, const char *argv[])
{
    struct directory *dp;
    struct ged *gedp = (struct ged *)datap;
    if (!gedp || argc < 1 || !argv)
	return BRLCAD_ERROR;

    /* load the constraint object */
    GED_DB_LOOKUP(gedp, dp, argv[2], LOOKUP_QUIET, BRLCAD_ERROR);

    bu_vls_printf(gedp->ged_result_str, "Evaluating %s constraint\n", argv[2]);
    bu_vls_printf(gedp->ged_result_str, "<<constraint eval here>>\n");

    return BRLCAD_OK;
}


HIDDEN int
constraint_auto(void *datap, int argc, const char *argv[])
{
    struct directory *dp;
    struct ged *gedp = (struct ged *)datap;
    if (!gedp || argc < 1 || !argv)
	return BRLCAD_ERROR;

    /* load the constraint object */
    GED_DB_LOOKUP(gedp, dp, argv[2], LOOKUP_QUIET, BRLCAD_ERROR);

    bu_vls_printf(gedp->ged_result_str, "Autoconstraining %s\n", argv[2]);
    bu_vls_printf(gedp->ged_result_str, "<<constraint auto here>>\n");

    return BRLCAD_OK;
}


HIDDEN void
constraint_usage(struct bu_vls *vp, const char *argv0)
{
    static const char *usage = "{set|get|show|rm|eval|auto} constraint_name [expression[=value] ...]";

    bu_vls_printf(vp, "Usage: %s %s\n", argv0, usage);
    bu_vls_printf(vp, "  or   %s help [command]\n", argv0);
}


HIDDEN int
constraint_help(void *datap, int argc, const char *argv[])
{
    struct ged *gedp = (struct ged *)datap;
    if (!gedp || argc < 1 || !argv)
	return BRLCAD_ERROR;

    bu_vls_printf(gedp->ged_result_str, "Help for the %s command\n\n", argv[0]);

    constraint_usage(gedp->ged_result_str, argv[0]);

    bu_vls_printf(gedp->ged_result_str, "\nConstraint Expressions:\n");
    bu_vls_printf(gedp->ged_result_str, "\tcoincident {{point point}|{point edge}|{edge edge}}\n");
    bu_vls_printf(gedp->ged_result_str, "\t\tspecified entities stay connected\n");
    bu_vls_printf(gedp->ged_result_str, "\tcolinear {line line}\n");
    bu_vls_printf(gedp->ged_result_str, "\t\tstraight edges are segments along the same line\n");
    bu_vls_printf(gedp->ged_result_str, "\tconcentric {circle circle}\n");
    bu_vls_printf(gedp->ged_result_str, "\t\tcircle/arc curves maintain equal center point\n");
    bu_vls_printf(gedp->ged_result_str, "\tfixed {point|edge|face|object}\n");
    bu_vls_printf(gedp->ged_result_str, "\t\tspecified entity is immutable w.r.t. global coordinate system\n");
    bu_vls_printf(gedp->ged_result_str, "\tparallel {{edge edge}|{face face}|{object object}}\n");
    bu_vls_printf(gedp->ged_result_str, "\t\tspecified entities are parallel to each other\n");
    bu_vls_printf(gedp->ged_result_str, "\tperpendicular {{edge edge}|{face face}|{object object}}\n");
    bu_vls_printf(gedp->ged_result_str, "\t\tspecified entities are perpendicular (90 degrees angle to each other\n");
    bu_vls_printf(gedp->ged_result_str, "\thorizontal {{point point}|edge|vector}\n");
    bu_vls_printf(gedp->ged_result_str, "\t\tspecified entities have X/Y values all set to match each other\n");
    bu_vls_printf(gedp->ged_result_str, "\tvertical {{point point}|edge|vector}\n");
    bu_vls_printf(gedp->ged_result_str, "\t\tspecified entities have Z values all set to match each other\n");
    bu_vls_printf(gedp->ged_result_str, "\ttangent {point|edge|face|object} {point|edge|face|object}\n");
    bu_vls_printf(gedp->ged_result_str, "\t\tspecified entities maintain contact (point sets overlap without intersecting)\n");
    bu_vls_printf(gedp->ged_result_str, "\tsymmetric {{point point}|{curve curve}} line\n");
    bu_vls_printf(gedp->ged_result_str, "\t\tspecified symmetry about an axis\n");
    bu_vls_printf(gedp->ged_result_str, "\tequal {point|edge|face|object} {point|edge|face|object}\n");
    bu_vls_printf(gedp->ged_result_str, "\t\tspecified values are set equal in magnitude/length/area to each other\n");

    bu_vls_printf(gedp->ged_result_str, "\nEntity Functions:\n");
    bu_vls_printf(gedp->ged_result_str, "\tbisect(curve) => point\n");
    bu_vls_printf(gedp->ged_result_str, "\tcenter(surface) => point\n");
    bu_vls_printf(gedp->ged_result_str, "\tcentroid(object) => point\n");
    bu_vls_printf(gedp->ged_result_str, "\tcurvature(curve1, t) => vector\n");

    bu_vls_printf(gedp->ged_result_str, "\nValue Functions:\n");
    bu_vls_printf(gedp->ged_result_str, "\tangle(curve1, curve2) => value\n");
    bu_vls_printf(gedp->ged_result_str, "\tarea(surface) => value\n");
    bu_vls_printf(gedp->ged_result_str, "\tdiameter(arccurve) => value\n");
    bu_vls_printf(gedp->ged_result_str, "\tdistance(entity, entity) => value\n");
    bu_vls_printf(gedp->ged_result_str, "\thdistance(entity, entity) => value\n");
    bu_vls_printf(gedp->ged_result_str, "\tlength(curve) => value\n");
    bu_vls_printf(gedp->ged_result_str, "\tmagnitude(vector) => value\n");
    bu_vls_printf(gedp->ged_result_str, "\tradius(arccurve) => value\n");
    bu_vls_printf(gedp->ged_result_str, "\tvdistance(entity, entity) => value\n");

    bu_vls_printf(gedp->ged_result_str, "\nExamples:\n");
    bu_vls_printf(gedp->ged_result_str, "\t%s set c1 coincident ell.V arb8.P[4]\n", argv[0]);
    bu_vls_printf(gedp->ged_result_str, "\t%s set c2 colinear arb8.E[1] rpc.E[0]\n", argv[0]);
    bu_vls_printf(gedp->ged_result_str, "\t%s set c3 tangent car terrain.r\n", argv[0]);
    bu_vls_printf(gedp->ged_result_str, "\t%s set c4 equal ell.R[1] arb8.E[2]\n", argv[0]);
    bu_vls_printf(gedp->ged_result_str, "\t%s set p0 expression ell.R[0]=10.0\n", argv[0]);
    bu_vls_printf(gedp->ged_result_str, "\t%s set p1 expression eto.A=40.2\n", argv[0]);
    bu_vls_printf(gedp->ged_result_str, "\t%s set p2 expression 4\n", argv[0]);
    bu_vls_printf(gedp->ged_result_str, "\t%s set p3 expression ell.R[1] * p2\n", argv[0]);
    bu_vls_printf(gedp->ged_result_str, "\t%s set p4 expression sph.R[0]=p3-10.5*magnitude(ell.V, argv[0])\n", argv[0]);
    bu_vls_printf(gedp->ged_result_str, "\t%s get c1 c3\n", argv[0]);
    bu_vls_printf(gedp->ged_result_str, "\t%s show c1 c2 c3 c4\n", argv[0]);
    bu_vls_printf(gedp->ged_result_str, "\t%s rm c1 tangent\n", argv[0]);

    return BRLCAD_OK;
}


int
ged_constraint(struct ged *gedp, int argc, const char *argv[])
{
    /* Potential constrainat attributes:
     *
     * attr set c1 cad:description "this keeps our car on the ground"
     * attr set c1 cad:plot 0|1
     * attr set c1 cad:reference 0|1
     * attr set c1 cad:format value|name|expression (name=value)
     */

    static struct bu_cmdtab pc_cmds[] = {
	{"set", constraint_set},
	{"get", constraint_get},
	{"show", constraint_show},
	{"rm", constraint_rm},
	{"eval", constraint_eval},
	{"auto", constraint_auto},
	{"help", constraint_help},
	{(const char *)NULL, BU_CMD_NULL}
    };

    int ret;
    int cmdret;

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    if (argc < 2) {
	/* must be wanting help */
	constraint_usage(gedp->ged_result_str, argv[0]);
	return GED_HELP;
    }
    if (BU_STR_EQUIV(argv[1], "help")) {
        constraint_help(gedp, argc, argv);
	return GED_OK;
    }

    if (argc < 3) {
	/* must be confused */
	constraint_usage(gedp->ged_result_str, argv[0]);
	return GED_HELP;
    }

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);

    /* this is only valid for v5 databases */
    if (db_version(gedp->ged_wdbp->dbip) < 5) {
	bu_vls_printf(gedp->ged_result_str, "Attributes are not available for this database format.\nPlease upgrade your database format using \"dbupgrade\" to enable attributes.");
	return GED_ERROR;
    }

    /* run our command */
    ret = bu_cmd(pc_cmds, argc, argv, 1, gedp, &cmdret);
    if (ret != BRLCAD_OK) {
	constraint_usage(gedp->ged_result_str, argv[0]);
	return GED_ERROR;
    }
    if (cmdret != BRLCAD_OK)
	return GED_ERROR;

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
