/*                         P U S H . C
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
/** @file libged/push.c
 *
 * The push command.
 *
 * TODO - depth limiting pushes aren't working yet...
 */

#include "common.h"

#include <algorithm>
#include <set>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "bu/cmd.h"
#include "bu/opt.h"

#include "../ged_private.h"

/* Database objects (struct directory) are unique in a database, but those
 * unique objects may be reused by multiple distinct comb tree instances.
 * When pushing matrices, the atomic unit of interest is not the database
 * object but those comb tree instances.
 *
 * The .g file format does not store such instances as discrete objects - they
 * exist as tree elements of the comb object tree definitions, consisting of a
 * reference to a directory object and a matrix associated with that instance
 * (if no explicit matrix is present, an implicit identity (IDN) matrix is
 * assumed.)
 *
 * For the purposes of matrix pushing, we need to think about these instances
 * as distinct atomic objects, whose uniqueness is defined not by the directory
 * pointer itself or by the instance in any given tree, but the combination of
 * the directory pointer and the positioning matrix.  In order to do so, we
 * define a C++ class that encapsulates the necessary information and a <
 * operator that allows for insertion of instances into C++ sets (which offer
 * sorting and a uniqueness guarantee.)
 */
class dp_i {
    public:
	struct directory *dp;  // Instance database object
	struct directory *parent_dp;  // Parent object
	mat_t mat;             // Instance matrix
	size_t ind;

	std::string iname = std::string();     // Container to hold instance name, if needed
	const struct bn_tol *ts_tol;  // Tolerance to use for matrix comparisons

	bool push_obj = true;  // Flag to determine if this instance is being pushed
	bool is_leaf = false;  // Flag to determine if this instance is a push leaf

	// If an instance is being pushed, there is one step that can be taken
	// beyond simply propagating the matrix down the tree - the solid
	// associated with the instance can have its parameters updated to
	// reflect the application of the matrix.  This completely "clears" all
	// matrix applications from the comb tree instance.
	bool apply_to_solid = false;

	// The key to the dp_i class is the less than operator, which is what
	// allows C++ sets to distinguish between separate instances with the
	// same dp pointer.  The first check is that dp pointer - if the current
	// and other dp do not match, the sorting criteria is obvious.  Likewise,
	// if one instance is instructed in the push to apply the matrix to a
	// solid and another is not, even if they would otherwise be identical
	// instances, it is necessary to distinguish them since those two
	// definitions require different comb tree instances to represent.
	//
	// If instance definitions DO match in other respects, the uniqueness
	// rests on the similarity of their matrices.  If they are equal within
	// tolerance, the instances are equal as well.
	//
	// One additional refinement is made to the sorting for processing
	// convenience - we make sure that any IDN instance is less than any
	// non-IDN matrix in sorting behavior, even if the numerics of the
	// matrices wouldn't otherwise reach that determination.
	bool operator<(const dp_i &o) const {

	    // First, check dp
	    if (dp < o.dp) return true;
	    if (o.dp < dp) return false;

	    // The application of the matrix to the solid may matter
	    // when distinguishing dp_i instances, but only if one
	    // of the matrices involved is non-IDN - otherwise, the
	    // matrix applications are no-ops and we don't want them
	    // to prompt multiple instances of objects.
	    if (!(dp->d_flags & RT_DIR_COMB)) {
		if (apply_to_solid != o.apply_to_solid) {
		    if (!bn_mat_is_equal(mat, bn_mat_identity, ts_tol) ||
			    !bn_mat_is_equal(o.mat, bn_mat_identity, ts_tol)) {
			if (apply_to_solid && !o.apply_to_solid)
			    return true;
		    }
		}
	    }

	    /* If the dp doesn't tell us and the solid application doesn't tell
	     * us, check the matrix. */
	    if (!bn_mat_is_equal(mat, o.mat, ts_tol)) {
		// We want IDN matrices to be less than any others, regardless
		// of the numerics.
		int tidn = bn_mat_is_equal(mat, bn_mat_identity, ts_tol);
		int oidn = bn_mat_is_equal(o.mat, bn_mat_identity, ts_tol);
		if (tidn && !oidn) return true;
		if (oidn && !tidn) return false;

		// If we don't have an IDN matrix involved, fall back on
		// numerical sorting to order the instances
		for (int i = 0; i < 16; i++) {
		    if (mat[i] < o.mat[i]) {
			return true;
		    }
		    if (mat[i] > o.mat[i]) {
			return false;
		    }
		}
	    }

	    /* All attempt to find non-equalities failed */
	    return false;
	}

	/* For convenience, we also define an equality operator */
	bool operator==(const dp_i &o) const {
	    if (dp != o.dp) return false;
	    if (apply_to_solid != o.apply_to_solid) {
		if (!bn_mat_is_equal(mat, bn_mat_identity, ts_tol) ||
		       	!bn_mat_is_equal(o.mat, bn_mat_identity, ts_tol))
		    return false;
	    }
	    return bn_mat_is_equal(mat, o.mat, ts_tol);
	}
};

/* Container to hold information during tree walk. */
struct push_state {

    /* Variables used for initial validity checking of specified push
     * object(s).  The target_objects set is user specified - if objects are
     * specified that are below other target objects, flag valid_push is set to
     * false and the user gets an error report. */
    bool valid_push = true;
    std::set<std::string> target_objs;

    /* User-supplied flags controlling tree walking behavior */
    int max_depth = 0;
    bool stop_at_regions = false;
    bool stop_at_shapes = false;
    bool dry_run = false;

    /* Containers for instance structures being built up during the push walk.
     * We do not need to duplicate these - the combination of matrix and dp
     * uniquely identifies a volume in space - so the C++ set container is used
     * to ensure we end up with the set of just unique dp instances after tree
     * walking. */
    std::set<dp_i> instances;

    /* Tolerance to be used for matrix comparisons.  Typically comes from the
     * database tolerances. */
    const struct bn_tol *tol;

    /* Database information */
    struct rt_wdb *wdbp;

    /* Processing info */
    std::map<struct directory *, struct rt_db_internal *> updated;

    /* Debugging related data and variables */
    int verbosity = 0;
    std::string problem_obj;
    bool walk_error = false;

};

/* Tree walks terminate at leaves.  However, what constitutes a leaf is not always
 * the same, depending on the specific stage of the push operation we are in.
 * is_push_leaf encapsulates the various possibilities. */
static bool
is_push_leaf(struct directory *dp, int depth, struct push_state *s, bool survey)
{
    if (!dp || !s || depth < 0) return true;

    if (survey) {
	// In a survey, if a leaf is one of the originally specified objects,
	// we're done - below such an object push logic is active.  A survey
	// needs to find objects not otherwise considered by the push.
	if (s->target_objs.find(std::string(dp->d_namep)) != s->target_objs.end()) {
	    return true;
	}
	return false;
    }

    /* Solids are always leaves, since there's nothing to walk */
    if (!(dp->d_flags & RT_DIR_COMB)) {
	return true;
    }

    /* Regions may be leaves, depending on user options */
    if ((dp->d_flags & RT_DIR_REGION) && s->stop_at_regions) {
	return true;
    }

    /* Depth halting may be active */
    if (s->max_depth && (depth >= s->max_depth))
	return true;

    return false;
}


/* This set of walking functions does a preliminary check to ensure there are no
 * nested specifications of push objects supplied to the push command, in the case
 * where a user supplies multiple root objects to pushed.
 *
 * Once an invalidity is found, these routines abort and record the invalidity.
 * This is for ease of implementation and performance, but can be adjusted to
 * do a more comprehensive review if that proves worthwhile. */
static void
validate_walk(struct db_i *dbip,
	struct directory *dp,
	void *client_data);

static void
validate_walk_subtree(struct db_i *dbip,
		    union tree *tp,
		    void *client_data)
{
    struct directory *dp;
    struct push_state *s = (struct push_state *)client_data;

    if (!tp || !s->valid_push)
	return;

    RT_CK_TREE(tp);

    switch (tp->tr_op) {

	case OP_DB_LEAF:

	    if ((dp=db_lookup(dbip, tp->tr_l.tl_name, LOOKUP_NOISY)) == RT_DIR_NULL)
		return;

	    if (s->target_objs.find(std::string(dp->d_namep)) != s->target_objs.end()) {
		s->valid_push = false;
		s->problem_obj = std::string(dp->d_namep);
		return;
	    }

	    if (!(dp->d_flags & RT_DIR_COMB)) {
		return;
	    }
	    validate_walk(dbip, dp, client_data);
	    break;

	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
	    validate_walk_subtree(dbip, tp->tr_b.tb_left, client_data);
	    validate_walk_subtree(dbip, tp->tr_b.tb_right, client_data);
	    break;
	default:
	    bu_log("validate_walk_subtree: unrecognized operator %d\n", tp->tr_op);
	    bu_bomb("validate_walk_subtree: unrecognized operator\n");
    }
}

static void
validate_walk(struct db_i *dbip,
	    struct directory *dp,
	    void *client_data)
{
    struct push_state *s = (struct push_state *)client_data;
    RT_CK_DBI(dbip);

    if (!dp || !s->valid_push)
	return; /* nothing to do */

    if (dp->d_flags & RT_DIR_COMB) {

	struct rt_db_internal in;
	struct rt_comb_internal *comb;

	if (rt_db_get_internal5(&in, dp, dbip, NULL, &rt_uniresource) < 0)
	    return;

	comb = (struct rt_comb_internal *)in.idb_ptr;
	validate_walk_subtree(dbip, comb->tree, client_data);
	rt_db_free_internal(&in);

    }
}


/* The primary walking routine, responsible for collecting information about
 * tree states.  Used for both a push walk and survey of the overall database.
 * */
static void
push_walk(struct directory *dp,
	int depth,
	mat_t *curr_mat,
	bool survey,
	void *client_data);

static void
push_walk_subtree(struct directory *parent_dp,
	union tree *tp,
	int depth,
	mat_t *curr_mat,
	bool survey,
	void *client_data)
{
    struct directory *dp;
    dp_i dnew;
    dnew.parent_dp = parent_dp;
    struct push_state *s = (struct push_state *)client_data;
    mat_t om, nm;

    if (!tp)
	return;

    RT_CHECK_DBI(s->wdbp->dbip);
    RT_CK_TREE(tp);

    switch (tp->tr_op) {

	case OP_DB_LEAF:

	    // Don't consider the leaf it if doesn't exist (TODO - is this always
	    // what we want to do when pushing?)
	    if ((dp=db_lookup(s->wdbp->dbip, tp->tr_l.tl_name, LOOKUP_NOISY)) == RT_DIR_NULL)
		return;

	    /* Update current matrix state to reflect the new branch of
	     * the tree. Either we have a local matrix, or we have an
	     * implicit IDN matrix. */
	    MAT_COPY(om, *curr_mat);
	    if (tp->tr_l.tl_mat) {
		MAT_COPY(nm, tp->tr_l.tl_mat);
	    } else {
		MAT_IDN(nm);
	    }
	    bn_mat_mul(*curr_mat, om, nm);

	    if (s->verbosity > 2) {
		if (tp->tr_l.tl_mat && !bn_mat_is_equal(*curr_mat, bn_mat_identity, s->tol)) {
		    bu_log("Found %s->[M]%s\n", parent_dp->d_namep, dp->d_namep);
		    if (s->verbosity > 3) {
			bn_mat_print(tp->tr_l.tl_name, *curr_mat);
		    }
		} else {
		    bu_log("Found %s->%s\n", parent_dp->d_namep, dp->d_namep);
		}
	    }

	    // Define the instance container
	    dnew.dp = dp;
	    dnew.ts_tol = s->tol;
	    dnew.push_obj = !(survey);
	    if (!survey) {
		MAT_COPY(dnew.mat, *curr_mat);
	    } else {
		MAT_COPY(dnew.mat, nm);
	    }

	    if (is_push_leaf(dp, depth, s, survey)) {

		// Flag as leaf
		dnew.is_leaf = true;

		// Leaf without parent means no work to do
		if (!parent_dp)
		    return;

		if (!survey) {
		    if (!(dp->d_flags & RT_DIR_COMB) && (!s->max_depth || depth+1 <= s->max_depth) && !s->stop_at_shapes) {
			// If dp is a solid, we're not depth limited, we're not
			// stopping above shapes, and the solid supports it we
			// apply the matrix to the primitive itself.  The comb
			// reference will use the IDN matrix.
			if (s->verbosity > 2) {
			    bu_log("Push leaf (finalize matrix or solid params): %s->%s\n", parent_dp->d_namep, dp->d_namep);
			}
			dnew.apply_to_solid = true;
		    }
		}

		// If we haven't already inserted an identical instance, record this new
		// entry.  (Note that we're not concerned with the
		// boolean set aspects of the tree's definition here, only unique volumes in
		// space, so this set does not exactly mimic the original comb tree structure.)
		if (s->instances.find(dnew) == s->instances.end()) {
		    s->instances.insert(dnew);
		}

		// Even though this is a leaf, we need to continue if we have a
		// comb, in order to build awareness of non-pushed comb
		// instances when we are depth limiting.  If a pushed matrix
		// changes a comb definition, but the same comb is in use
		// elsewhere in the tree below the max push depth, the altered
		// comb will need to be copied.
		if (dp->d_flags & RT_DIR_COMB) {
		    /* Process branch's tree */
		    if (s->verbosity > 1 && !survey) {
			bu_log("Switching from push to survey - non-shape leaf %s->%s found\n", parent_dp->d_namep, dp->d_namep);
		    }
		    push_walk(dp, depth, curr_mat, true, client_data);
		}

		/* Done with branch - put back the old matrix state */
		MAT_COPY(*curr_mat, om);
		return;
	    } else {
		// If this isn't a push leaf, this is not the termination point
		// of a push - the matrix ultimately applied will be an IDN
		// matrix, but the current tree matrix is recorded to allow
		// instance lookups later in processing.
		if (!survey && s->verbosity > 2) {
		    bu_log("Pushed comb instance: %s->%s\n", parent_dp->d_namep, dp->d_namep);
		}
		dnew.is_leaf = false;
	    }

	    if (survey && s->verbosity > 2) {
		bu_log("Survey comb instance: %s->%s\n", parent_dp->d_namep, dp->d_namep);
	    }
	    if (s->instances.find(dnew) == s->instances.end()) {
		s->instances.insert(dnew);
	    }

	    /* Process branch's tree */
	    push_walk(dp, depth, curr_mat, survey, client_data);

	    /* Done with branch - put back the old matrix state */
	    MAT_COPY(*curr_mat, om);
	    break;

	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
	    push_walk_subtree(parent_dp, tp->tr_b.tb_left, depth, curr_mat, survey, client_data);
	    push_walk_subtree(parent_dp, tp->tr_b.tb_right, depth, curr_mat, survey, client_data);
	    break;
	default:
	    bu_log("push_walk_subtree: unrecognized operator %d\n", tp->tr_op);
	    bu_bomb("push_walk_subtree: unrecognized operator\n");
    }
}

static void
push_walk(struct directory *dp,
	int depth,
	mat_t *curr_mat,
	bool survey,
	void *client_data)
{
    struct push_state *s = (struct push_state *)client_data;

    if (!dp) {
	return; /* nothing to do */
    }

    if (dp->d_flags & RT_DIR_COMB) {

	struct rt_db_internal in;
	struct rt_comb_internal *comb;

	if (rt_db_get_internal5(&in, dp, s->wdbp->dbip, NULL, &rt_uniresource) < 0)
	    return;

	comb = (struct rt_comb_internal *)in.idb_ptr;
	push_walk_subtree(dp, comb->tree, depth+1, curr_mat, survey, client_data);
	rt_db_free_internal(&in);
    }
}


/* Now that instances are uniquely defined (and inames assigned to those
 * instances where needed) analyze the comb trees to finalize what needs to be
 * updated and/or created to finalize the proper comb tree references. */
static void
tree_update_walk(
	const dp_i &dpi,
	int depth,
	mat_t *curr_mat,
	void *client_data);

static void
tree_update_walk_subtree(
	            const dp_i &parent_dpi,
		    union tree *tp,
		    union tree *wtp,
		    int depth,
		    mat_t *curr_mat,
		    bool *tree_altered,
		    void *client_data)
{
    struct directory *dp;
    struct push_state *s = (struct push_state *)client_data;
    std::set<dp_i>::iterator dpii, i_it;
    mat_t om, nm;
    dp_i ldpi;

    if (!tp)
	return;

    RT_CK_TREE(tp);

    switch (tp->tr_op) {

	case OP_DB_LEAF:

	    // If we're too deep, we're no longer creating instances to manipulate
	    if (s->max_depth && (depth > s->max_depth)) {
		return;
	    }

	    // Don't consider the leaf it if doesn't exist (TODO - is this always
	    // what we want to do when pushing?)
	    if ((dp=db_lookup(s->wdbp->dbip, tp->tr_l.tl_name, LOOKUP_NOISY)) == RT_DIR_NULL)
		return;

	    /* Update current matrix state to reflect the new branch of
	     * the tree. Either we have a local matrix, or we have an
	     * implicit IDN matrix. */
	    MAT_COPY(om, *curr_mat);
	    if (tp->tr_l.tl_mat) {
		MAT_COPY(nm, tp->tr_l.tl_mat);
	    } else {
		MAT_IDN(nm);
	    }
	    bn_mat_mul(*curr_mat, om, nm);

	    // Look up the dpi for this comb+curr_mat combination.
	    ldpi.dp = dp;
	    MAT_COPY(ldpi.mat, *curr_mat);
	    ldpi.ts_tol = s->tol;
	    if (!(dp->d_flags & RT_DIR_COMB) && (!s->max_depth || depth+1 <= s->max_depth) && !s->stop_at_shapes) {
		ldpi.apply_to_solid = true;
	    }
	    dpii = s->instances.find(ldpi);
	    if (dpii == s->instances.end()) {
		bu_log("Error - no instance found: %s->%s!\n", parent_dpi.dp->d_namep, dp->d_namep);
		dpii = s->instances.find(ldpi);
		bn_mat_print("curr_mat", *curr_mat);
		if (dpii->apply_to_solid)
		    bu_log("apply_to_solid set\n");
		for (i_it = s->instances.begin(); i_it != s->instances.end(); i_it++) {
		    const dp_i &ddpi = *i_it;
		    if (ddpi.dp == dp) {
			bn_mat_print(tp->tr_l.tl_name, ddpi.mat);
			if (ddpi.iname.length())
			    bu_log("iname: %s\n", ddpi.iname.c_str());
			if (ddpi.apply_to_solid)
			    bu_log("apply_to_solid set\n");
		    }
		}
		s->walk_error = true;
		return;
	    }

	    if (s->verbosity > 2) {
		if (tp->tr_l.tl_mat && !bn_mat_is_equal(*curr_mat, bn_mat_identity, s->tol)) {
		    bu_log("Found %s->[M]%s\n", parent_dpi.dp->d_namep, dp->d_namep);
		} else {
		    bu_log("Found %s->%s\n", parent_dpi.dp->d_namep, dp->d_namep);
		}
	    }

	    /* Tree editing operations - form the tree that will be used in the final
	     * output of this comb object.  These operations should use the writable
	     * tree (wtp) to ensure we don't cause any inadvertent issues with the
	     * tree walk.  (If it can be proven that this can never happen we could
	     * edit directly on the original tp, but until that's certain working on
	     * a copy of the tree is safer. */

	    // Comb tree edit:  if this is a push leaf, set final matrix, else
	    // set IDN and keep walking down.
	    if (dpii->is_leaf && !dpii->apply_to_solid) {
		if (wtp->tr_l.tl_mat) {
		    if (!bn_mat_is_equal(wtp->tr_l.tl_mat, dpii->mat, s->tol)) {
			MAT_COPY(wtp->tr_l.tl_mat, dpii->mat);
			(*tree_altered) = true;
		    }
		} else {
		    if (!bn_mat_is_identity(dpii->mat)) {
			wtp->tr_l.tl_mat = bn_mat_dup(dpii->mat);
			(*tree_altered) = true;
		    }
		}
	    } else {
		if (wtp->tr_l.tl_mat) {
		    // IDN matrix
		    bu_free(wtp->tr_l.tl_mat, "free mat");
		    wtp->tr_l.tl_mat = NULL;
		    (*tree_altered) = true;
		}
	    }

	    // Comb tree edit:  if we have an iname, update the tree name.
	    if (dpii->iname.length()) {
		bu_free(wtp->tr_l.tl_name, "free old name");
		wtp->tr_l.tl_name = bu_strdup(dpii->iname.c_str());
		(*tree_altered) = true;
	    }

	    /* Process */
	    tree_update_walk(*dpii, depth, curr_mat, client_data);

	    /* Done with branch - put back the old matrix state */
	    MAT_COPY(*curr_mat, om);
	    break;

	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
	    tree_update_walk_subtree(parent_dpi, tp->tr_b.tb_left, wtp->tr_b.tb_left, depth+1, curr_mat, tree_altered, client_data);
	    tree_update_walk_subtree(parent_dpi, tp->tr_b.tb_right, wtp->tr_b.tb_right, depth+1, curr_mat, tree_altered, client_data);
	    break;
	default:
	    bu_log("tree_update_walk_subtree: unrecognized operator %d\n", tp->tr_op);
	    bu_bomb("tree_update_walk_subtree: unrecognized operator\n");
    }
}

static void
tree_update_walk(
	const dp_i &dpi,
	int depth,
	mat_t *curr_mat,
	void *client_data)
{
    struct directory *dp;
    struct push_state *s = (struct push_state *)client_data;

    if (dpi.dp->d_flags & RT_DIR_COMB) {

	/* Read only copy of comb tree - use for steering the walk */
	struct rt_db_internal intern;
	struct rt_comb_internal *comb;
	bool tree_altered = false;
	if (rt_db_get_internal5(&intern, dpi.dp, s->wdbp->dbip, NULL, &rt_uniresource) < 0) {
	    return;
	}
	comb = (struct rt_comb_internal *)intern.idb_ptr;

	/* Copy of comb tree for editing - use for recording new tree */
	struct rt_db_internal *in;
	struct rt_comb_internal *wcomb;
	BU_GET(in, struct rt_db_internal);
	if (rt_db_get_internal5(in, dpi.dp, s->wdbp->dbip, NULL, &rt_uniresource) < 0) {
	    BU_PUT(in, struct rt_db_internal);
	    return;
	}
	wcomb = (struct rt_comb_internal *)in->idb_ptr;

	// Walk one tree copy, while recording updates in the other one
	tree_update_walk_subtree(dpi, comb->tree, wcomb->tree, depth + 1, curr_mat, &tree_altered, client_data);

	// Read-only copy is done
	rt_db_free_internal(&intern);

	// If we didn't alter the working comb tree, we're done.
	if (!tree_altered) {
	    rt_db_free_internal(in);
	    BU_PUT(in, struct rt_db_internal);
	    return;
	}

	// If we DID alter the comb tree (typically we will), we have to get
	// the new tree on to disk.  If this instance has an iname, we are not
	// updating the existing object but instead creating a new one.
	//
	// At this point in processing, we can either rely on the uniqueness
	// of the dp->instance mapping (vanilla push) or we use the inames to
	// create new dp objects - i.e., at this point, we have enough information
	// to make a unique 1-1 mapping between a struct directory pointer and
	// the object information needed for a pushed object instance.
	if (dpi.iname.length()) {

	    // New name, new dp
	    dp = db_lookup(s->wdbp->dbip, dpi.iname.c_str(), LOOKUP_QUIET);

	    if (dp != RT_DIR_NULL) {
		// If we've already created the dp, we don't need to do so
		// again.  Theoretically this might happen if two separate
		// branches of tree walking happen to produce the same instance
		// in their trees.
		rt_db_free_internal(in);
		BU_PUT(in, struct rt_db_internal);
		return;
	    }

	    // Haven't already created this particular instance dp - proceed,
	    // but don't (yet) alter the database by writing the internal
	    // contents. That will be done with all other file changes at the
	    // end of processing.
	    dp = db_diradd(s->wdbp->dbip, dpi.iname.c_str(), RT_DIR_PHONY_ADDR, 0, dpi.dp->d_flags, (void *)&in->idb_type);
	    if (dp == RT_DIR_NULL) {
		bu_log("Unable to add %s to the database directory", dpi.iname.c_str());
		rt_db_free_internal(in);
		BU_PUT(in, struct rt_db_internal);
		return;
	    }
	} else {
	    // We're editing the existing tree - reuse dp
	    dp = dpi.dp;
	}

	s->updated[dp] = in;

    } else {

	// If we're not copying the solid and not applying a matrix, we're done
	if (!dpi.iname.length() && bn_mat_is_identity(dpi.mat))
	    return;

	if (s->verbosity > 3) {
	    if (dpi.iname.length()) {
		bu_log("Solid process %s->%s\n", dpi.parent_dp->d_namep, dpi.iname.c_str());
	    } else {
		bu_log("Solid process %s->%s\n", dpi.parent_dp->d_namep, dpi.dp->d_namep);
	    }
	}

	struct rt_db_internal *in;
	BU_GET(in, struct rt_db_internal);
	if (dpi.apply_to_solid) {
	    // If there is a non-IDN matrix, this is where we apply it
	    if (s->verbosity > 3 && !bn_mat_is_identity(dpi.mat)) {
		bn_mat_print(dpi.dp->d_namep, dpi.mat);
		bn_mat_print("curr_mat", *curr_mat);
	    }
	    if (rt_db_get_internal(in, dpi.dp, s->wdbp->dbip, dpi.mat, &rt_uniresource) < 0) {
		bu_log("Read error fetching '%s'\n", dpi.dp->d_namep);
		return;
	    }
	} else {
	    // If there is a non-IDN matrix, this is where we apply it
	    if (rt_db_get_internal(in, dpi.dp, s->wdbp->dbip, bn_mat_identity, &rt_uniresource) < 0) {
		bu_log("Read error fetching '%s'\n", dpi.dp->d_namep);
		return;
	    }
	}
	RT_CK_DB_INTERNAL(in);

	if (dpi.iname.length()) {
	    // If we have an iname, we need a new directory pointer.
	    dp = db_lookup(s->wdbp->dbip, dpi.iname.c_str(), LOOKUP_QUIET);
	    if (dp != RT_DIR_NULL) {
		// If we've already created this, we're done
		rt_db_free_internal(in);
		BU_PUT(in, struct rt_db_internal);
		return;
	    }
	    dp = db_diradd(s->wdbp->dbip, dpi.iname.c_str(), RT_DIR_PHONY_ADDR, 0, dpi.dp->d_flags, (void *)&in->idb_type);
	    if (dp == RT_DIR_NULL) {
		bu_log("Unable to add %s to the database directory\n", dpi.iname.c_str());
		rt_db_free_internal(in);
		BU_PUT(in, struct rt_db_internal);
		return;
	    }
	    if (s->verbosity)
		bu_log("Write solid %s contents\n", dpi.iname.c_str());
	} else {
	    if (s->verbosity)
		bu_log("Write solid %s contents\n", dpi.dp->d_namep);

	    dp = dpi.dp;
	}

	s->updated[dp] = in;

    }
}


static void
npush_usage(struct bu_vls *str, struct bu_opt_desc *d) {
    char *option_help = bu_opt_describe(d, NULL);
    bu_vls_sprintf(str, "Usage: npush [options] obj\n");
    bu_vls_printf(str, "\nPushes position/rotation matrices 'down' the tree hierarchy, altering existing geometry as needed.  Default behavior clears all matrices from tree, unless push requires creation of new geometry objects.\n\n");
    if (option_help) {
	bu_vls_printf(str, "Options:\n%s\n", option_help);
	bu_free(option_help, "help str");
    }
}


extern "C" int
ged_npush_core(struct ged *gedp, int argc, const char *argv[])
{
    int print_help = 0;
    int xpush = 0;
    int to_regions = 0;
    int to_solids = 0;
    int max_depth = 0;
    int verbosity = 0;
    int local_changes_only = 0;
    int dry_run = 0;
    struct bu_opt_desc d[11];
    BU_OPT(d[0], "h", "help",      "",   NULL,         &print_help,  "Print help and exit");
    BU_OPT(d[1], "?", "",          "",   NULL,         &print_help,  "");
    BU_OPT(d[2], "v", "verbosity",  "",  &bu_opt_incr_long, &verbosity,     "Increase output verbosity (multiple specifications of -v increase verbosity)");
    BU_OPT(d[3], "f", "force",     "",   NULL,         &xpush,       "Create new objects if needed to push matrices (xpush)");
    BU_OPT(d[4], "x", "xpush",     "",   NULL,         &xpush,       "");
    BU_OPT(d[5], "r", "regions",   "",   NULL,         &to_regions,  "Halt push at regions (matrix will be above region reference)");
    BU_OPT(d[6], "s", "solids",    "",   NULL,         &to_solids,   "Halt push at solids (matrix will be above solid reference)");
    BU_OPT(d[7], "d", "max-depth", "",   &bu_opt_int,  &max_depth,   "Halt at depth # from tree root (matrix will be above item # layers deep)");
    BU_OPT(d[8], "L", "local", "",       NULL,  &local_changes_only,   "Ensure push operations do not impact geometry outside the specified trees.");
    BU_OPT(d[9], "D", "dry-run", "",       NULL,  &dry_run,   "Calculate the changes but do not apply them.");

    BU_OPT_NULL(d[10]);

    /* Skip command name */
    argc--; argv++;

    /* parse standard options */
    int opt_ret = bu_opt_parse(NULL, argc, argv, d);

    if (!argc || print_help) {
	struct bu_vls npush_help = BU_VLS_INIT_ZERO;
	npush_usage(&npush_help, d);
	bu_vls_sprintf(gedp->ged_result_str, "%s", bu_vls_cstr(&npush_help));
	bu_vls_free(&npush_help);
	return GED_OK;
    }

    /* adjust argc to match the leftovers of the options parsing */
    argc = opt_ret;

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_READ_ONLY(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);
    struct db_i *dbip = gedp->ged_wdbp->dbip;

    /* Need nref current for db_ls to work */
    db_update_nref(dbip, &rt_uniresource);

    /* initialize result */
    bu_vls_trunc(gedp->ged_result_str, 0);


    /* Based on the push arg list, walk the specified trees to identify
     * instances involved with matrices */
    struct push_state s;
    s.verbosity = verbosity;
    s.tol = &gedp->ged_wdbp->wdb_tol;
    s.max_depth = max_depth;
    s.stop_at_regions = (to_regions) ? true : false;
    s.stop_at_shapes = (to_solids) ? true : false;
    s.dry_run = (dry_run) ? true : false;
    s.wdbp = gedp->ged_wdbp;
    for (int i = 0; i < argc; i++) {
	s.target_objs.insert(std::string(argv[i]));
    }


    // Validate that no target_obj is underneath another target obj. Otherwise,
    // multiple push operations may collide.
    std::set<std::string>::iterator s_it;
    for (s_it = s.target_objs.begin(); s_it != s.target_objs.end(); s_it++) {
	struct directory *dp = db_lookup(dbip, s_it->c_str(), LOOKUP_NOISY);
	validate_walk(dbip, dp, &s);
	if (!s.valid_push) {
	    bu_vls_printf(gedp->ged_result_str, "%s has another specified target object (%s), below it.", dp->d_namep, s.problem_obj.c_str());
	    return GED_ERROR;
	}
    }

    // Do a preliminary walk down the push objects to determine what impact the
    // push operations would have on the comb trees and solids.
    mat_t m;
    for (s_it = s.target_objs.begin(); s_it != s.target_objs.end(); s_it++) {
	struct directory *dp = db_lookup(dbip, s_it->c_str(), LOOKUP_NOISY);
	if (dp != RT_DIR_NULL && (dp->d_flags & RT_DIR_COMB)) {
	    MAT_IDN(m);
	    push_walk(dp, 0, &m, false, &s);

	    /* Sanity - if we didn't end up with m back at the identity matrix,
	     * something went wrong with the walk */
	    if (!bn_mat_is_equal(m, bn_mat_identity, &gedp->ged_wdbp->wdb_tol)) {
		bu_vls_sprintf(gedp->ged_result_str, "Error - initial tree walk down %s finished with non-IDN matrix.\n", dp->d_namep);
		return GED_ERROR;
	    }
	}
    }


    if (local_changes_only) {
	/* Because pushes have potentially global consequences, we provide an
	 * option that records ALL unique object instances in the database.
	 * That information will allow the push operation to tell if any given
	 * matrix push is self contained as-is or would require an object
	 * copy+rename to avoid changing geometry elsewhere in the .g database's
	 * trees.
	 */
	struct directory **all_paths;
	int tops_cnt = db_ls(gedp->ged_wdbp->dbip, DB_LS_TOPS, NULL, &all_paths);
	for (int i = 0; i < tops_cnt; i++) {
	    struct directory *dp = all_paths[i];
	    if (s.target_objs.find(std::string(dp->d_namep)) == s.target_objs.end())
		push_walk(dp, 0, &m, true, &s);
	}
	bu_free(all_paths, "free db_ls output");
    }

    // Create a vector of unique instances for easier manipulation.
    std::vector<dp_i> uniq_instances(s.instances.size());
    std::copy(s.instances.begin(), s.instances.end(), uniq_instances.begin());

    // Iterate over unique instances and build sets of indices to instances
    // sharing a common directory pointer.  Any dp with multiple associated
    // instances can't be pushed without a copy being made, as the unique dp
    // instances represent multiple distinct volumes in space.
    std::map<struct directory *, std::vector<size_t>> i_cnt;
    for (size_t i = 0; i < uniq_instances.size(); i++) {
	uniq_instances[i].ind = i;
	i_cnt[uniq_instances[i].dp].push_back(i);
    }

    // If we don't have force-push on, and we have non-unique mapping between
    // dp pointers and instances, we can't proceed.
    if (!xpush) {
	bool conflict = false;
	struct bu_vls msgs = BU_VLS_INIT_ZERO;
	for (size_t i = 0; i < uniq_instances.size(); i++) {
	    const dp_i &dpi = uniq_instances[i];

	    // If we're not looking at a push object, or the dp mapping is
	    // unique, we don't need a force push for this case.
	    if (!dpi.push_obj || i_cnt[dpi.dp].size() < 2) {
		continue;
	    }

	    // Push object with multiple mappings - this is the condition where
	    // xpush is needed.  Since force is not enabled, we can't proceed.
	    conflict = true;

	    // If we're not being verbose, the first failure means we can just
	    // immediately bail.
	    if (!verbosity) {
		bu_vls_printf(gedp->ged_result_str, "Operation failed - force not enabled and one or more solids are being moved in conflicting directions by multiple comb instances.");
		return GED_ERROR;
	    }
	}
	if (conflict) {
	    // If we haven't already bailed, verbosity settings indicate the user
	    // will want more information about the precise nature of the failure.
	    std::set<struct directory *> reported;
	    for (size_t i = 0; i < uniq_instances.size(); i++) {
		const dp_i &dpi = uniq_instances[i];
		// If not a problem case, skip
		if (!dpi.push_obj || i_cnt[dpi.dp].size() < 2) {
		    continue;
		}

		// If we have already reported on this dp's conflicts, skip
		if (reported.find(dpi.dp) != reported.end())
		    continue;

		// Reporting - mark so we don't repeat later
		reported.insert(dpi.dp);

		// Handle various verbosity settings
		if (s.verbosity > 1) {
		    bu_vls_printf(&msgs, "Conflicting instances found: %s->%s:\n", dpi.parent_dp->d_namep, dpi.dp->d_namep);
		} else {
		    bu_vls_printf(&msgs, "Conflicting instances found: %s->%s\n", dpi.parent_dp->d_namep, dpi.dp->d_namep);
		}
		// Itemize the instances causing the failure (potentially quite
		// verbose, so require a higher verbosity setting for this...)
		if (s.verbosity > 1) {
		    for (size_t j = 0; j < i_cnt[dpi.dp].size(); j++) {
			const dp_i &dpc = uniq_instances[i_cnt[dpi.dp][j]];
			struct bu_vls imat = BU_VLS_INIT_ZERO;
			struct bu_vls ititle = BU_VLS_INIT_ZERO;
			const char *title0 = "Initial instance";
			if (j == 0) {
			    bu_vls_printf(&ititle, "%s", title0);
			} else {
			    bu_vls_printf(&ititle, "Conflicting Instance #%zd", j);
			}
			if (!dpc.push_obj) {
			    bu_vls_printf(&ititle, " (non-local)");
			}
			bn_mat_print_vls(bu_vls_cstr(&ititle), dpc.mat, &imat);
			bu_vls_printf(&msgs, "%s", bu_vls_cstr(&imat));
			bu_vls_free(&imat);
		    }
		}
	    }
	    bu_vls_printf(gedp->ged_result_str, "%s\nOperation failed - force not enabled and one or more solids are being moved in conflicting directions by multiple comb instances.", bu_vls_cstr(&msgs));
	    bu_vls_free(&msgs);
	    return GED_ERROR;
	}
	bu_vls_free(&msgs);
    }

    // We need to assign new names for the instances we will be creating.  Because we need these
    // names to not collide with any existing database names (or each other) we build up a set
    // of the current names:
    std::set<std::string> dbnames;
    for (int i = 0; i < RT_DBNHASH; i++) {
	struct directory *dp;
	for (dp = gedp->ged_wdbp->dbip->dbi_Head[i]; dp != RT_DIR_NULL; dp = dp->d_forw) {
	    if (dp->d_namep) {
		std::string dpn(dp->d_namep);
		dbnames.insert(dpn);
	    }
	}
    }

    // New names for copies are only required if we have non-unique mappings between directory
    // pointers and instances.
    //
    // This mechanism is convenient, because it is general - it handles both local (in-push)
    // and non-local (global check, if enabled) instances and only generates new names for objects
    // that will be pushed, without trying to rename "inactive" instances.
    std::map<struct directory *, std::vector<size_t>>::iterator i_it;
    for (i_it = i_cnt.begin(); i_it != i_cnt.end(); i_it++) {
	if (i_it->second.size() == 1)
	    continue;

	// If we have anything we're NOT pushing, that means we need to rename
	// all the push objects.
	bool have_nopush = false;
	for (size_t i = 0; i < i_it->second.size(); i++) {
	    dp_i &dpi = uniq_instances[i_it->second[i]];
	    if (!dpi.push_obj)
		have_nopush = true;
	}
	// Don't rename the first object in the list if we're pushing all of
	// the instances, since there is no need to rename it to avoid a
	// conflict - we're renaming everything else.
	int ilabel = 0;
	size_t istart = (have_nopush) ? 0 : 1;
	for (size_t i = istart; i < i_it->second.size(); i++) {
	    dp_i &dpi = uniq_instances[i_it->second[i]];
	    if (!dpi.push_obj)
		continue;
	    std::string iname = std::string(dpi.dp->d_namep) + std::string("_") + std::to_string(ilabel);
	    while (dbnames.find(iname) != dbnames.end() && ilabel < INT_MAX) {
		ilabel++;
		iname = std::string(dpi.dp->d_namep) + std::string("_") + std::to_string(ilabel);
	    }
	    if (ilabel == INT_MAX) {
		// Strange name generation failure...
		bu_vls_printf(gedp->ged_result_str, "Unable to generate valid push name for %s\n", dpi.dp->d_namep);
		return GED_ERROR;
	    }
	    dbnames.insert(iname);
	    dpi.iname = iname;
	}
    }

    // Clear the set and repopulate it, so any lookups into the set will find
    // dp_i containers containing any inames the above logic may have assigned.
    s.instances.clear();
    s.instances.insert(uniq_instances.begin(), uniq_instances.end());

    // For debugging purposes, with a high enough verbosity print out the state
    // of the instances set ahead of the final tree walk so we know what to
    // expect.
    if (s.verbosity > 4) {
	std::set<dp_i>::iterator in_it;
	bu_log("\n\n\nDirectory pointer instance set (final state before application walk):\n\n");
	for (in_it = s.instances.begin(); in_it != s.instances.end(); in_it++) {
	    const dp_i &dpi = *in_it;
	    if (!dpi.push_obj)
		continue;
	    if (dpi.iname.length()) {
		if (!dpi.is_leaf && (dpi.dp->d_flags & RT_DIR_COMB)) {
		    bu_log("%s tree edited && matrix to IDN\n", dpi.iname.c_str());
		}
		if (dpi.is_leaf && !bn_mat_is_equal(dpi.mat, bn_mat_identity, s.tol)) {
		    bu_log("%s\n", dpi.iname.c_str());
		    bn_mat_print("applied", dpi.mat);
		}
	    } else {
		if (!dpi.is_leaf && (dpi.dp->d_flags & RT_DIR_COMB)) {
		    bu_log("%s matrix to IDN\n", dpi.dp->d_namep);
		}
		if (dpi.is_leaf && !bn_mat_is_equal(dpi.mat, bn_mat_identity, s.tol)) {
		    bu_log("%s\n", dpi.dp->d_namep);
		    bn_mat_print("applied", dpi.mat);
		}
	    }
	}
    }


    // Walk a final time to characterize the state of the comb trees in light
    // of the changes made to handle pushed information.
    for (s_it = s.target_objs.begin(); s_it != s.target_objs.end(); s_it++) {
	struct directory *dp = db_lookup(dbip, s_it->c_str(), LOOKUP_NOISY);
	if (dp != RT_DIR_NULL && (dp->d_flags & RT_DIR_COMB)) {
	    // All walks start with an identity matrix.
	    MAT_IDN(m);
	    // We are now working with instances, not the raw directory
	    // pointers.  Because by definition the top level object is an
	    // identity matrix object, we initialize a dp_i with the identity
	    // form of the object to "seed" the tree walk.
	    dp_i ldpi;
	    ldpi.dp = dp;
	    MAT_IDN(ldpi.mat);
	    ldpi.ts_tol = s.tol;

	    // Start the walk.
	    tree_update_walk(ldpi, 0, &m, &s);
	}
    }

    std::map<struct directory *, struct rt_db_internal *>::iterator u_it;
    if (s.walk_error) {
	bu_vls_printf(gedp->ged_result_str, "Fatal error preparing new trees\n");
	for (u_it = s.updated.begin(); u_it != s.updated.end(); u_it++) {
	    struct rt_db_internal *in = u_it->second;
	    rt_db_free_internal(in);
	    BU_PUT(in, struct rt_db_internal);
	}
	return GED_ERROR;
    }

    /* We now know everything we need.  For combs and primitives that have updates
     * or are being newly created apply those changes to the .g file.  Because this
     * step is destructive, it is carried out only after all characterization steps
     * are complete in order to avoid any risk of changing what is being analyzed
     * mid-stream.  (Problems of that nature are not always simple to observe, and
     * can be *very* difficult to debug.) */
    for (u_it = s.updated.begin(); u_it != s.updated.end(); u_it++) {
	struct directory *dp = u_it->first;
	struct rt_db_internal *in = u_it->second;
	if (!s.dry_run) {
	    if (rt_db_put_internal(dp, s.wdbp->dbip, in, s.wdbp->wdb_resp) < 0) {
		bu_log("Unable to store %s to the database", dp->d_namep);
	    }
	} else {
	    // Delete the directory pointers we set up - dry run, so we're not
	    // actually creating the objects.
	    db_dirdelete(gedp->ged_wdbp->dbip, dp);
	}
	rt_db_free_internal(in);
	BU_PUT(in, struct rt_db_internal);
    }

    return GED_OK;

}

extern "C" {
#ifdef GED_PLUGIN
#include "../include/plugin.h"
struct ged_cmd_impl npush_cmd_impl = {
    "npush",
    ged_npush_core,
    GED_CMD_DEFAULT
};

const struct ged_cmd npush_cmd = { &npush_cmd_impl };
const struct ged_cmd *npush_cmds[] = { &npush_cmd, NULL };

static const struct ged_plugin pinfo = { GED_API,  npush_cmds, 1 };

COMPILER_DLLEXPORT const struct ged_plugin *ged_plugin_info()
{
    return &pinfo;
}
#endif /* GED_PLUGIN */
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
