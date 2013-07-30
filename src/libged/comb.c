/*                  C O M B . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2013 United States Government as represented by
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
/** @file libged/comb.c
 *
 * The comb command.
 *
 */

#include "common.h"

#include "bio.h"

#include "cmd.h"
#include "wdb.h"

#include "./ged_private.h"

int
_ged_set_region_flag(struct ged *gedp, struct directory *dp) {
    struct bu_attribute_value_set avs;
    bu_avs_init_empty(&avs);
    if (db5_get_attributes(gedp->ged_wdbp->dbip, &avs, dp)) {
	bu_vls_printf(gedp->ged_result_str, "Cannot get attributes for object %s\n", dp->d_namep);
	return GED_ERROR;
    }
    db5_standardize_avs(&avs);
    dp->d_flags |= RT_DIR_REGION;
    (void)bu_avs_add(&avs, "region", "R");
    if (db5_update_attributes(dp, &avs, gedp->ged_wdbp->dbip)) {
	bu_vls_printf(gedp->ged_result_str,
		"Error: failed to update attributes\n");
	bu_avs_free(&avs);
	return GED_ERROR;
    }
    return GED_OK;
}

int
_ged_clear_region_flag(struct ged *gedp, struct directory *dp) {
    struct bu_attribute_value_set avs;
    bu_avs_init_empty(&avs);
    if (db5_get_attributes(gedp->ged_wdbp->dbip, &avs, dp)) {
	bu_vls_printf(gedp->ged_result_str, "Cannot get attributes for object %s\n", dp->d_namep);
	return GED_ERROR;
    }
    db5_standardize_avs(&avs);
    dp->d_flags = dp->d_flags & ~(RT_DIR_REGION);
    (void)bu_avs_remove(&avs, "region");
    if (db5_replace_attributes(dp, &avs, gedp->ged_wdbp->dbip)) {
	bu_vls_printf(gedp->ged_result_str,
		"Error: failed to update attributes\n");
	bu_avs_free(&avs);
	return GED_ERROR;
    }
    return GED_OK;
}

int
_ged_clear_color_shader(struct ged *gedp, struct directory *dp) {
    struct bu_attribute_value_set avs;
    bu_avs_init_empty(&avs);
    if (db5_get_attributes(gedp->ged_wdbp->dbip, &avs, dp)) {
	bu_vls_printf(gedp->ged_result_str, "Cannot get attributes for object %s\n", dp->d_namep);
	return GED_ERROR;
    }
    db5_standardize_avs(&avs);
    (void)bu_avs_remove(&avs, "rgb");
    (void)bu_avs_remove(&avs, "color");
    (void)bu_avs_remove(&avs, "shader");
    (void)bu_avs_remove(&avs, "oshader");
    if (db5_replace_attributes(dp, &avs, gedp->ged_wdbp->dbip)) {
	bu_vls_printf(gedp->ged_result_str,
		"Error: failed to update attributes\n");
	bu_avs_free(&avs);
	return GED_ERROR;
    }
    return GED_OK;
}

int
_ged_wrap_comb(struct ged *gedp, struct directory *dp) {

    struct bu_vls orig_name, comb_child_name;
    struct bu_external external;
    struct rt_db_internal intern;
    struct rt_comb_internal *comb;
    struct directory *orig_dp = dp;
    struct directory *new_dp;

    bu_vls_init(&comb_child_name);
    bu_vls_init(&orig_name);

    bu_vls_sprintf(&orig_name, "%s", dp->d_namep);
    bu_vls_sprintf(&comb_child_name, "%s.c", dp->d_namep);

    /* First, make sure the target comb name for wrapping doesn't already exist */
    if ((new_dp=db_lookup(gedp->ged_wdbp->dbip, bu_vls_addr(&comb_child_name), LOOKUP_QUIET)) != RT_DIR_NULL) {
	bu_vls_printf(gedp->ged_result_str, "ERROR: %s already exists in the database, cannot wrap %s", bu_vls_addr(&comb_child_name), dp->d_namep);
	bu_vls_free(&comb_child_name);
	bu_vls_free(&orig_name);
	return GED_ERROR;
    }

    /* Create a copy of the comb using a new name */
    if (db_get_external(&external, dp, gedp->ged_wdbp->dbip)) {
	bu_vls_printf(gedp->ged_result_str, "Wrapping %s: Database read error retrieving external, aborting\n", dp->d_namep);
	bu_vls_free(&comb_child_name);
	bu_vls_free(&orig_name);
	return GED_ERROR;
    }
    if (wdb_export_external(gedp->ged_wdbp, &external, bu_vls_addr(&comb_child_name), dp->d_flags, dp->d_minor_type) < 0) {
	bu_free_external(&external);
	bu_vls_printf(gedp->ged_result_str, "Failed to write new object (%s) to database - aborting!!\n", bu_vls_addr(&comb_child_name));
	bu_vls_free(&comb_child_name);
	bu_vls_free(&orig_name);
	return GED_ERROR;
    }
    bu_free_external(&external);

    /* Load new obj.c comb and clear its region flag, if any */
    if ((new_dp=db_lookup(gedp->ged_wdbp->dbip, bu_vls_addr(&comb_child_name), LOOKUP_QUIET)) == RT_DIR_NULL) {
	bu_vls_printf(gedp->ged_result_str, "Wrapping %s: creation of %s failed!", dp->d_namep, bu_vls_addr(&comb_child_name));
	bu_vls_free(&comb_child_name);
	bu_vls_free(&orig_name);
	return GED_ERROR;
    }
    if (_ged_clear_region_flag(gedp, new_dp) == GED_ERROR) {
	bu_vls_free(&comb_child_name);
	bu_vls_free(&orig_name);
	return GED_ERROR;
    }
    if (_ged_clear_color_shader(gedp, new_dp) == GED_ERROR) {
	bu_vls_free(&comb_child_name);
	bu_vls_free(&orig_name);
	return GED_ERROR;
    }

    /* Clear the tree from the original object */
    GED_DB_GET_INTERNAL(gedp, &intern, dp, (matp_t)NULL, &rt_uniresource, GED_ERROR);
    RT_CK_DB_INTERNAL(&intern);
    comb = (struct rt_comb_internal *)(&intern)->idb_ptr;
    RT_CK_COMB(comb);
    db_free_tree(comb->tree, &rt_uniresource);
    comb->tree = TREE_NULL;
    db5_sync_comb_to_attr(&((&intern)->idb_avs), comb);
    db5_standardize_avs(&((&intern)->idb_avs));
    if (wdb_put_internal(gedp->ged_wdbp, dp->d_namep, &intern, 1.0) < 0) {
	bu_vls_printf(gedp->ged_result_str, "wdb_export(%s) failure", dp->d_namep);
	rt_db_free_internal(&intern);
	bu_vls_free(&comb_child_name);
	bu_vls_free(&orig_name);
	return GED_ERROR;
    }
    rt_db_free_internal(&intern);
    if ((orig_dp=db_lookup(gedp->ged_wdbp->dbip, bu_vls_addr(&orig_name), LOOKUP_QUIET)) == RT_DIR_NULL) {
	bu_vls_printf(gedp->ged_result_str, "ERROR: %s tree clearing failed", bu_vls_addr(&orig_name));
	bu_vls_free(&comb_child_name);
	bu_vls_free(&orig_name);
	return GED_ERROR;
    }

    /* add "child" comb to the newly cleared parent */
    if (_ged_combadd(gedp, new_dp, orig_dp->d_namep, 0, WMOP_UNION, 0, 0) == RT_DIR_NULL) {
	bu_vls_printf(gedp->ged_result_str, "Error adding '%s' (with op '%c') to '%s'\n", bu_vls_addr(&comb_child_name), WMOP_UNION, dp->d_namep);
	bu_vls_free(&comb_child_name);
	bu_vls_free(&orig_name);
	return GED_ERROR;
    }
    bu_vls_free(&comb_child_name);
    bu_vls_free(&orig_name);

    return GED_OK;
}


/* Approach - define a search string that searches for all solid objects
 * in the tree, and returns the bu_ptbl list of unique solids.  Do the
 * same for combs.  
 *
 * Need to make sure that all objects in the tree are unioned in - 
 * it's not clear what it would mean to at the moment "flatten" a 
 * tree with intersections or subtractions.  
 * Once boolean evaluations are ready, we could probably
 * evaluate such objects to get a single representation that can be unioned
 * into a flat tree...
 *
 * Clear the tree of the comb, and add all the solids in the bu_ptbl.
 *
 * Because it's a hard flatten, kill all the comb trees in the comb
 * list.  For a "soft" kill, a third search is needed for all objects
 * outside the comb tree - hopefully, that bu_ptbl can then be searched
 * for the candidate deletion pointers and only those *not* found will
 * be deleted - i.e only delete objects uniquely in this comb tree.
 *
 * bu_ptbl_locate is how to check for a given pointer in the table - -1 returned if not found
 */
/*
int
_ged_flatten_comb(struct ged *gedp, struct directory *dp) {
}

int
_ged_hard_flatten_comb(struct ged *gedp, struct directory *dp) {

}
*/

int
ged_comb(struct ged *gedp, int argc, const char *argv[])
{
    struct directory *dp;
    const char *cmd_name;
    char *comb_name;
    int i,c;
    char oper;
    int set_region = 0;
    int set_comb = 0;
    int standard_comb_build = 1;
    int wrap_comb = 0;
    int flatten_comb = 0;
    int alter_existing = 1;
    static const char *usage = "[-c/-r] [-w/-f] [-S] comb_name [<operation object>]";

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_READ_ONLY(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);

    cmd_name = argv[0];

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_HELP;
    }

    if (argc < 3) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }

    /* First, handle options, if any */

    bu_optind = 1;
    /* Grab any arguments off of the argv list */
    while ((c = bu_getopt(argc, (char **)argv, "crwfS")) != -1) {
        switch (c) {
            case 'c' :
                set_comb = 1;
                break;
            case 'r' :
                set_region = 1;
                break;
	    case 'w' :
		wrap_comb = 1;
		standard_comb_build = 0;
		break;
	    case 'f' :
		flatten_comb = 1;
		standard_comb_build = 0;
		break;
            case 'S' :
		alter_existing = 0;
		break;
	    default :
                break;
        }
    }

    argc -= bu_optind - 1;
    argv += bu_optind - 1;

    if (set_comb && set_region) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], cmd_name);
	return GED_ERROR;
    }

    if ((wrap_comb || flatten_comb) && argc != 2) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return GED_ERROR;
    }

    /* Get target combination info */
    comb_name = (char *)argv[1];
    if ((dp=db_lookup(gedp->ged_wdbp->dbip, comb_name, LOOKUP_QUIET)) != RT_DIR_NULL) {
	if (!(dp->d_flags & RT_DIR_COMB)) {
	    bu_vls_printf(gedp->ged_result_str, "ERROR: %s is not a combination", comb_name);
	    return GED_ERROR;
	}
        if ((dp != RT_DIR_NULL) && !alter_existing) {
	    bu_vls_printf(gedp->ged_result_str, "ERROR: %s already exists.", comb_name);
        }
    }

    /* If we aren't performing one of the option operations,
     * proceed with the standard comb build */
    if (standard_comb_build) {

	/* Now, we're ready to process operation/object pairs, if any */
	/* Check for odd number of arguments */
	if (argc & 01) {
	    bu_vls_printf(gedp->ged_result_str, "error in number of args!");
	    return GED_ERROR;
	}

	/* Get operation and solid name for each solid */
	for (i = 2; i < argc; i += 2) {
	    /* they come in pairs */
	    if (i+1 >= argc) {
		bu_vls_printf(gedp->ged_result_str, "Invalid syntax near '%s', ignored.  Expecting object name after operator.\n", argv[i+1]);
		return GED_ERROR;
	    }

	    /* ops are 1-char */
	    if (argv[i][1] != '\0') {
		bu_vls_printf(gedp->ged_result_str, "Invalid operation '%s' before object '%s'\n", argv[i], argv[i+1]);
		continue;
	    }
	    oper = argv[i][0];
	    if (oper != WMOP_UNION && oper != WMOP_SUBTRACT && oper != WMOP_INTERSECT) {
		bu_vls_printf(gedp->ged_result_str, "Unknown operator '%c' encountered, invalid syntax.\n", oper);
		continue;
	    }

	    /* object name comes after op */
	    if ((dp = db_lookup(gedp->ged_wdbp->dbip,  argv[i+1], LOOKUP_NOISY)) == RT_DIR_NULL) {
		bu_vls_printf(gedp->ged_result_str, "Object '%s does not exist.\n", argv[i+1]);
		continue;
	    }

	    /* add it to the comb immediately */
	    if (_ged_combadd(gedp, dp, comb_name, 0, oper, 0, 0) == RT_DIR_NULL) {
		bu_vls_printf(gedp->ged_result_str, "Error adding '%s' (with op '%c') to '%s'\n", dp->d_namep, oper, comb_name);
		return GED_ERROR;
	    }
	}
    }

    /* Handle the -w option for "wrapping" the contents of the comb */
    if (wrap_comb) {
	if (!dp || dp == RT_DIR_NULL) {
	    bu_vls_printf(gedp->ged_result_str, "Combination '%s does not exist.\n", comb_name);
	    return GED_ERROR;
	}
	if (_ged_wrap_comb(gedp, dp) == GED_ERROR) {
	    return GED_ERROR;
	} else {
	    if ((dp=db_lookup(gedp->ged_wdbp->dbip, comb_name, LOOKUP_QUIET)) == RT_DIR_NULL) {
		bu_vls_printf(gedp->ged_result_str, "ERROR: wrap of %s failed", comb_name);
		return GED_ERROR;
	    }
	}
    }

    if (flatten_comb) {
	if (!dp || dp == RT_DIR_NULL) {
	    bu_vls_printf(gedp->ged_result_str, "Combination '%s does not exist.\n", comb_name);
	    return GED_ERROR;
	}
/*	if (_ged_hard_flatten_comb(gedp, dp) == GED_ERROR) {
	    return GED_ERROR;
	} else {
	    if ((dp=db_lookup(gedp->ged_wdbp->dbip, comb_name, LOOKUP_QUIET)) == RT_DIR_NULL) {
		bu_vls_printf(gedp->ged_result_str, "ERROR: flattening of %s failed", comb_name);
		return GED_ERROR;
	    }
	}
*/
    }

    /* Make sure the region flag is set appropriately */
    if (set_comb || set_region) {
	if ((dp = db_lookup(gedp->ged_wdbp->dbip, comb_name, LOOKUP_NOISY)) != RT_DIR_NULL) {
	    if (set_region) {
		if (_ged_set_region_flag(gedp, dp) == GED_ERROR)
		    return GED_ERROR;
	    }
	    if (set_comb) {
		if (_ged_clear_region_flag(gedp, dp) == GED_ERROR)
		    return GED_ERROR;
	    }
	}
    }

    return GED_OK;
}


/*
 * G E D _ C O M B A D D
 *
 * Add an instance of object 'objp' to combination 'name'.
 * If the combination does not exist, it is created.
 * region_flag is 1 (region), or 0 (group).
 *
 * Preserves the GIFT semantics.
 */
struct directory *
_ged_combadd(struct ged *gedp,
	     struct directory *objp,
	     char *combname,
	     int region_flag,	/* true if adding region */
	     int relation,	/* = UNION, SUBTRACT, INTERSECT */
	     int ident,		/* "Region ID" */
	     int air		/* Air code */)
{
    int ac = 1;
    const char *av[2];

    av[0] = objp->d_namep;
    av[1] = NULL;

    if (_ged_combadd2(gedp, combname, ac, av, region_flag, relation, ident, air) == GED_ERROR)
	return RT_DIR_NULL;

    return db_lookup(gedp->ged_wdbp->dbip, combname, LOOKUP_QUIET);
}


/*
 * G E D _ C O M B A D D
 *
 * Add an instance of object 'objp' to combination 'name'.
 * If the combination does not exist, it is created.
 * region_flag is 1 (region), or 0 (group).
 *
 * Preserves the GIFT semantics.
 */
int
_ged_combadd2(struct ged *gedp,
	      char *combname,
	      int argc,
	      const char *argv[],
	      int region_flag,	/* true if adding region */
	      int relation,	/* = UNION, SUBTRACT, INTERSECT */
	      int ident,	/* "Region ID" */
	      int air		/* Air code */)
{
    struct directory *dp;
    struct directory *objp;
    struct rt_db_internal intern;
    struct rt_comb_internal *comb;
    union tree *tp;
    struct rt_tree_array *tree_list;
    size_t node_count;
    size_t actual_count;
    size_t curr_count;
    int i;

    if (argc < 1)
	return GED_ERROR;

    /*
     * Check to see if we have to create a new combination
     */
    if ((dp = db_lookup(gedp->ged_wdbp->dbip,  combname, LOOKUP_QUIET)) == RT_DIR_NULL) {
	int flags;

	if (region_flag)
	    flags = RT_DIR_REGION | RT_DIR_COMB;
	else
	    flags = RT_DIR_COMB;

	RT_DB_INTERNAL_INIT(&intern);
	intern.idb_major_type = DB5_MAJORTYPE_BRLCAD;
	intern.idb_type = ID_COMBINATION;
	intern.idb_meth = &rt_functab[ID_COMBINATION];

	GED_DB_DIRADD(gedp, dp, combname, -1, 0, flags, (genptr_t)&intern.idb_type, 0);

	BU_ALLOC(comb, struct rt_comb_internal);
	RT_COMB_INTERNAL_INIT(comb);

	intern.idb_ptr = (genptr_t)comb;

	if (region_flag) {
	    comb->region_flag = 1;
	    comb->region_id = ident;
	    comb->aircode = air;
	    comb->los = gedp->ged_wdbp->wdb_los_default;
	    comb->GIFTmater = gedp->ged_wdbp->wdb_mat_default;
	    bu_vls_printf(gedp->ged_result_str,
			  "Creating region id=%d, air=%d, GIFTmaterial=%d, los=%d\n",
			  ident, air,
			  gedp->ged_wdbp->wdb_mat_default,
			  gedp->ged_wdbp->wdb_los_default);
	} else {
	    comb->region_flag = 0;
	}

	goto addmembers;
    } else if (!(dp->d_flags & RT_DIR_COMB)) {
	bu_vls_printf(gedp->ged_result_str, "%s exists, but is not a combination\n", dp->d_namep);
	return GED_ERROR;
    }

    /* combination exists, add a new member */
    GED_DB_GET_INTERNAL(gedp, &intern, dp, (fastf_t *)NULL, &rt_uniresource, 0);

    comb = (struct rt_comb_internal *)intern.idb_ptr;
    RT_CK_COMB(comb);

    if (region_flag && !comb->region_flag) {
	bu_vls_printf(gedp->ged_result_str, "%s: not a region\n", dp->d_namep);
	return GED_ERROR;
    }

addmembers:
    if (comb->tree && db_ck_v4gift_tree(comb->tree) < 0) {
	db_non_union_push(comb->tree, &rt_uniresource);
	if (db_ck_v4gift_tree(comb->tree) < 0) {
	    bu_vls_printf(gedp->ged_result_str, "Cannot flatten tree for editing\n");
	    rt_db_free_internal(&intern);
	    return GED_ERROR;
	}
    }

    /* make space for an extra leaf */
    curr_count = db_tree_nleaves(comb->tree);
    node_count = curr_count + argc;
    tree_list = (struct rt_tree_array *)bu_calloc(node_count, sizeof(struct rt_tree_array), "tree list");

    /* flatten tree */
    if (comb->tree) {
	actual_count = argc + (struct rt_tree_array *)db_flatten_tree(tree_list, comb->tree, OP_UNION, 1, &rt_uniresource) - tree_list;
	BU_ASSERT_SIZE_T(actual_count, ==, node_count);
	comb->tree = TREE_NULL;
    }

    for (i = 0; i < argc; ++i) {
	if ((objp = db_lookup(gedp->ged_wdbp->dbip, argv[i], LOOKUP_NOISY)) == RT_DIR_NULL) {
	    bu_vls_printf(gedp->ged_result_str, "skip member %s\n", argv[i]);
	    continue;
	}

	/* insert new member at end */
	switch (relation) {
	case '+':
	    tree_list[curr_count].tl_op = OP_INTERSECT;
	    break;
	case '-':
	    tree_list[curr_count].tl_op = OP_SUBTRACT;
	    break;
	default:
	    if (relation != 'u') {
		bu_vls_printf(gedp->ged_result_str, "unrecognized relation (assume UNION)\n");
	    }
	    tree_list[curr_count].tl_op = OP_UNION;
	    break;
	}

	/* make new leaf node, and insert at end of list */
	RT_GET_TREE(tp, &rt_uniresource);
	tree_list[curr_count].tl_tree = tp;
	tp->tr_l.tl_op = OP_DB_LEAF;
	tp->tr_l.tl_name = bu_strdup(objp->d_namep);
	tp->tr_l.tl_mat = (matp_t)NULL;

	++curr_count;
    }

    /* rebuild the tree */
    comb->tree = (union tree *)db_mkgift_tree(tree_list, node_count, &rt_uniresource);

    /* and finally, write it out */
    GED_DB_PUT_INTERNAL(gedp, dp, &intern, &rt_uniresource, 0);

    bu_free((char *)tree_list, "combadd: tree_list");

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
