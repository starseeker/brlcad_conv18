/*                        D I F F . C P P
 * BRL-CAD
 *
 * Copyright (c) 2004-2020 United States Government as represented by
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
/** @file nirt.cpp
 *
 * Implementation of Natalie's Interactive Ray-Tracer (NIRT)
 * functionality specific to the diff subcommand.
 *
 */

/* BRL-CAD includes */
#include "common.h"

#include <regex>

#include "bu/cmd.h"

#include "./nirt.h"


/**
 * Design thoughts for nirt diffing:
 *
 * NIRT difference events come in two primary forms: "transition" differences
 * in the form of entry/exit hits, and differences in segments - the regions
 * between transitions.  In the context of a diff, a segment by definition
 * contains no transitions on *either* shotline path.  Even if a segment
 * contains no transitions along its own shotline, if a transition from the
 * other shot falls within its bounds the original segment will be treated as
 * two sequential segments of the same type for the purposes of comparison.
 *
 * Transitions are compared only to other transitions, and segments are
 * compared only to other segments.
 *
 * The comparison criteria are as follows:
 *
 * Transition points:
 *
 * 1.  DIST_PNT_PNT - if there is a transition on either path that does not align
 *     within the specified distance tolerance with a transition on the other
 *     path, the transition is unmatched and reported as a difference.
 * 2.  Obliquity delta - if two transition points align within the distance
 *     tolerance, the next test is the difference between their normals. If
 *     these match in direction and obliquity angle, the transition points
 *     are deemed to be matching.  Otherwise, a difference is reported on the
 *     transition point.
 *
 * Segments:
 *
 * 1.  The first comparison made between segments is their type. Type
 *     differences will always be reported as a difference.
 *
 * 2.  If types match, behavior will depend on options and the specific
 *     types being compared:
 *
 *     GAPS:       always match.
 *
 *     PARTITIONS: Match if all active criteria match.  If no criteria
 *                 are active, match.  Possible active criteria:
 *
 *                 Region name
 *                 Path name
 *                 Region ID
 *                 TODO - Attributes?
 *
 *     OVERLAPS:   Match if all active criteria match.  If no criteria
 *                 are active, match.  Possible active criteria:
 *
 *                 Overlap region set
 *                 Overlap path set
 *                 Overlap region id set
 *                 Selected "winning" partition in the overlap
 */

typedef enum {
    NIRT_PNT_BOTH = 0,
    NIRT_PNT_LEFT_ONLY,
    NIRT_PNT_RIGHT_ONLY
} n_pnt_origin_t;

typedef enum {
    NIRT_PNT_MULTI = 0,  /* used if tolerances collapse in and out points into a single transition event */
    NIRT_PNT_IN,
    NIRT_PNT_OUT
} n_pnt_transition_t;

typedef enum {
    NIRT_SEG_BOTH = 0,
    NIRT_SEG_LEFT_ONLY,
    NIRT_SEG_RIGHT_ONLY
} n_seg_origin_t;

typedef enum {
    NIRT_SEG_HIT = 0,
    NIRT_SEG_GAP,
    NIRT_SEG_OVLP
} n_seg_t;

#define NIRT_DIFF_PNT_OBLIQ           0x1
#define NIRT_DIFF_PNT_TRANS_TYPE      0x2  /* Points are geometrically the same (within tolerance) but denote opposite state transitions */

#define NIRT_DIFF_OVLPS               0x1
#define NIRT_DIFF_SEG_REGNAME         0x2
#define NIRT_DIFF_SEG_PATHNAME        0x4
#define NIRT_DIFF_SEG_ID              0x8
#define NIRT_DIFF_SEG_OVLP_SELECTED   0x16

struct nirt_diff_transition {
    n_pnt_origin_t origin;
    n_pnt_transition_t type;
    unsigned long flags;
    point_t transition_left;
    point_t transition_right;
    double obliq_left;
    double obliq_right;
    double dist;
};


struct nirt_diff_segment {
    n_seg_origin_t origin;
    n_seg_t type;
    unsigned long flags;
    struct nirt_seg *left;
    struct nirt_seg *right;
    struct nirt_diff_transition *start;
    struct nirt_diff_transition *end;
};


struct nirt_diff_event {
    n_pnt_origin_t type;
    unsigned long flags;
    point_t transition_left;
    point_t transition_right;
    vect_t obliq_left;
    vect_t obliq_right;
    struct nirt_seg unmatched;
    struct nirt_seg *left;
    struct nirt_seg *right;
};


struct nirt_diff_instance {
    n_pnt_origin_t type;
    point_t *left_pt;
    point_t *right_pt;
    struct nirt_seg *left;
    struct nirt_seg *right;
    fastf_t in_delta;
    fastf_t out_delta;
    fastf_t los_delta;
    fastf_t scaled_los_delta;
    fastf_t obliq_in_delta;
    fastf_t obliq_out_delta;
    fastf_t ov_in_delta;
    fastf_t ov_out_delta;
    fastf_t ov_los_delta;
    fastf_t gap_in_delta;
    fastf_t gap_los_delta;
};


struct nirt_diff_state;

struct nirt_diff_ray_state {
    point_t orig;
    vect_t dir;
    std::vector<struct nirt_seg> old_segs;
    std::vector<struct nirt_seg> new_segs;
    std::vector<struct nirt_diff_instance> diffs;
    struct nirt_diff_state *nds;
    long lcurr;
    long rcurr;
};


struct nirt_diff_state {

    struct bu_vls diff_file = BU_VLS_INIT_ZERO;
    struct nirt_diff_ray_state *cdiff = NULL;
    std::vector<struct nirt_diff_ray_state> rays;

    std::vector<struct nirt_diff_event> events;
    bool diff_ready = false;

    /* Reporting settings */
    bool report_partitions = true;
    bool report_misses = true;
    bool report_gaps = true;
    bool report_overlaps = true;
    bool report_partition_reg_ids = true;
    bool report_partition_reg_names = true;
    bool report_partition_path_names = true;
    bool report_partition_dists = true;
    bool report_partition_obliq = true;
    bool report_overlap_reg_names = true;
    bool report_overlap_reg_ids = true;
    bool report_overlap_dists = true;
    bool report_overlap_obliq = true;
    bool report_gap_dists = true;

    /* Tolerances */
    fastf_t dist_delta_tol = BN_TOL_DIST;
    fastf_t obliq_delta_tol = BN_TOL_DIST;
    fastf_t los_delta_tol = BN_TOL_DIST;
    fastf_t scaled_los_delta_tol = BN_TOL_DIST;

    struct nirt_state *nss;

    struct bu_opt_desc *d = NULL;
    const struct bu_cmdtab *cmds = NULL;
};


bool
_nirt_next_transition(struct nirt_diff_transition &nt, struct nirt_diff_ray_state *nr, struct nirt_diff_transition *ct)
{
    struct nirt_seg *lcurr = NULL;
    struct nirt_seg *rcurr = NULL;
    double rdist = 0.0;
    double ldist = 0.0;

    if (!ct) {
	// Initializing - find the first transition
	nr->lcurr = (nr->old_segs.size()) ? 0 : -1;
	nr->rcurr = (nr->new_segs.size()) ? 0 : -1;
	if (nr->lcurr < 0 && nr->rcurr < 0) {
	    return false;
	}
	lcurr = (nr->lcurr >= 0) ? &nr->old_segs[nr->lcurr] : NULL;
	rcurr = (nr->rcurr >= 0) ? &nr->new_segs[nr->rcurr] : NULL;
	nt.type = NIRT_PNT_IN;
	ldist = (nr->lcurr >= 0) ? DIST_PNT_PNT_SQ(nr->orig, lcurr->in) : DBL_MAX;
	rdist = (nr->rcurr >= 0) ? DIST_PNT_PNT_SQ(nr->orig, rcurr->in) : DBL_MAX;
	if (NEAR_EQUAL(ldist, rdist, nr->nds->dist_delta_tol)) {
	    nt.origin = NIRT_PNT_BOTH;
	    VMOVE(nt.transition_left, lcurr->in);
	    VMOVE(nt.transition_right, rcurr->in);
	    nt.obliq_left = lcurr->obliq_in;
	    nt.obliq_right = rcurr->obliq_in;
	} else {
	    if (ldist < rdist) {
		nt.origin = NIRT_PNT_LEFT_ONLY ;
		VMOVE(nt.transition_left, lcurr->in);
		nt.obliq_left = lcurr->obliq_in;
	    } else {
		nt.origin = NIRT_PNT_RIGHT_ONLY;
		VMOVE(nt.transition_right, rcurr->in);
		nt.obliq_right = rcurr->obliq_in;
	    }
	}
	return true;
    }

    // We are on a point - need to find the next point further from the origin (on either or both segment sets)
    lcurr = (nr->lcurr >= 0) ? &nr->old_segs[nr->lcurr] : NULL;
    rcurr = (nr->rcurr >= 0) ? &nr->new_segs[nr->rcurr] : NULL;
    double cdist = (ct->origin != NIRT_PNT_RIGHT_ONLY) ? DIST_PNT_PNT_SQ(nr->orig, ct->transition_left) :  DIST_PNT_PNT_SQ(nr->orig, ct->transition_right);
    int ncase = -1;
    bool pnt_active[4] = {false, false, false, false};
    while (ncase < 0) {
	double dists[4];
	n_pnt_transition_t types[4] = {NIRT_PNT_IN, NIRT_PNT_OUT, NIRT_PNT_IN, NIRT_PNT_OUT};
	nt.dist = DBL_MAX;
	if (lcurr->type == NIRT_OVERLAP_SEG) {
	    bu_log("overlap\n");
	}
	if (lcurr->type == NIRT_PARTITION_SEG || lcurr->type == NIRT_OVERLAP_SEG) {
	    dists[0] = (lcurr) ? DIST_PNT_PNT_SQ(nr->orig, lcurr->in)  : DBL_MAX;
	    dists[1] = (lcurr) ? DIST_PNT_PNT_SQ(nr->orig, lcurr->out) : DBL_MAX;
	}
	if (rcurr->type == NIRT_GAP_SEG || rcurr->type == NIRT_MISS_SEG) {
	    dists[0] = DBL_MAX;
	    dists[1] = DBL_MAX;
	}
	if (lcurr->type == NIRT_PARTITION_SEG || lcurr->type == NIRT_OVERLAP_SEG) {
	    dists[2] = (rcurr) ? DIST_PNT_PNT_SQ(nr->orig, rcurr->in)  : DBL_MAX;
	    dists[3] = (rcurr) ? DIST_PNT_PNT_SQ(nr->orig, rcurr->out) : DBL_MAX;
	}
	if (rcurr->type == NIRT_GAP_SEG || rcurr->type == NIRT_MISS_SEG) {
	    dists[2] = DBL_MAX;
	    dists[3] = DBL_MAX;
	}
	for (int i = 0; i < 4; i++) {
	    if (dists[i] < cdist || (NEAR_EQUAL(dists[i], cdist, nr->nds->dist_delta_tol) && types[i] == ct->type)) {
		// Don't repeat old points or an identical point (within tolerance) of the same type
	       	continue;
	    }
	    if (!(dists[i] < DBL_MAX)) {
		// Skip if we don't have a segment on one side
		continue;
	    }
	    if (dists[i] < nt.dist) {
		nt.dist = dists[i];
		ncase = i;
	    }
	}
	if (ncase < 0) {
	    // We didn't find anything in either current segment - increment the segments
	    nr->lcurr = (nr->lcurr >= 0) ? nr->lcurr + 1 : -1;
	    nr->rcurr = (nr->rcurr >= 0) ? nr->rcurr + 1 : -1;
	    nr->lcurr = (nr->lcurr >= (long)nr->old_segs.size()) ? -1 : nr->lcurr;
	    nr->rcurr = (nr->rcurr >= (long)nr->new_segs.size()) ? -1 : nr->rcurr;
	    lcurr = (nr->lcurr >= 0) ? &nr->old_segs[nr->lcurr] : NULL;
	    rcurr = (nr->rcurr >= 0) ? &nr->new_segs[nr->rcurr] : NULL;
	    if (!lcurr && !rcurr) {
		// End of the segments
		break;
	    }
	} else {
	    // We've got something, see if more than one point is at this distance
	    for (int i = 0; i < 4; i++) {
		if (NEAR_EQUAL(dists[i], nt.dist, nr->nds->dist_delta_tol)) {
		    pnt_active[i] = true;
		}
	    }
	    if (pnt_active[0] || pnt_active[2]) {
		nt.type = NIRT_PNT_IN;
	    }
	    if (pnt_active[1] || pnt_active[3]) {
		nt.type = NIRT_PNT_OUT;
	    }
	    if ((pnt_active[0] || pnt_active[2]) && (pnt_active[1] || pnt_active[3])) {
		nt.type = NIRT_PNT_MULTI;
	    }
	    if (pnt_active[0] || pnt_active[1]) {
		nt.origin = NIRT_PNT_LEFT_ONLY;
	    }
	    if (pnt_active[2] || pnt_active[3]) {
		nt.origin = NIRT_PNT_RIGHT_ONLY;
	    }
	    if ((pnt_active[0] || pnt_active[1]) && (pnt_active[2] || pnt_active[3])) {
		nt.origin = NIRT_PNT_BOTH;
	    }
	}
    }

    if (ncase == -1) {
	// There is no next point
	return false;
    }

    switch (ncase) {
	case 0:
	    VMOVE(nt.transition_left, lcurr->in);
	    nt.obliq_left = lcurr->obliq_in;
	    if (nt.origin == NIRT_PNT_BOTH) {
		if (nt.type == NIRT_PNT_IN) {
		    VMOVE(nt.transition_right, rcurr->in);
		    nt.obliq_right = rcurr->obliq_in;
		} else {
		    VMOVE(nt.transition_right, rcurr->out);
		    nt.obliq_right = rcurr->obliq_out;
		}
	    }
	    break;
	case 1:
	    VMOVE(nt.transition_left, lcurr->out);
	    nt.obliq_left = lcurr->obliq_in;
	    if (nt.origin == NIRT_PNT_BOTH) {
		if (nt.type == NIRT_PNT_OUT) {
		    VMOVE(nt.transition_right, lcurr->out);
		    nt.obliq_right = lcurr->obliq_out;
		} else {
		    VMOVE(nt.transition_left, rcurr->in);
		    nt.obliq_right = lcurr->obliq_in;
		}
	    }
	    break;
	case 2:
	    VMOVE(nt.transition_right, rcurr->in);
	    nt.obliq_right = rcurr->obliq_in;
	    if (nt.origin == NIRT_PNT_BOTH) {
		if (nt.type == NIRT_PNT_IN) {
		    VMOVE(nt.transition_left, lcurr->in);
		    nt.obliq_left = lcurr->obliq_in;
		} else {
		    VMOVE(nt.transition_left, lcurr->out);
		    nt.obliq_left = lcurr->obliq_out;
		}
	    }
	    break;
	case 3:
	    VMOVE(nt.transition_right, rcurr->out);
	    nt.obliq_right = rcurr->obliq_out;
	    if (nt.origin == NIRT_PNT_BOTH) {
		if (nt.type == NIRT_PNT_OUT) {
		    VMOVE(nt.transition_left, lcurr->out);
		    nt.obliq_left = lcurr->obliq_out;
		} else {
		    VMOVE(nt.transition_left, lcurr->in);
		    nt.obliq_left = lcurr->obliq_in;
		}
	    }
	    break;
	default:
	    return false;
    }

    return true;
}


// 1.  find first transition nt (ct == NULL);
// 2.  ct = nt;
// 3.  find next transition nt, insert into sequence;
// 4.  if nt != NULL construct segment from ct and nt;
// 5.  repeat 2 - 4 until nt == NULL;

void
_nirt_segs_analyze(struct nirt_diff_state *nds)
{
    std::map<n_pnt_origin_t, std::string> postr;
    postr[NIRT_PNT_BOTH] = "NRT_PNT_BOTH";
    postr[NIRT_PNT_LEFT_ONLY] = "NRT_PNT_LEFT_ONLY";
    postr[NIRT_PNT_RIGHT_ONLY] = "NRT_PNT_RIGHT_ONLY";
    std::map<n_pnt_transition_t, std::string> ptstr;
    ptstr[NIRT_PNT_MULTI] = "NIRT_PNT_MULTI";
    ptstr[NIRT_PNT_IN] = "NIRT_PNT_IN";
    ptstr[NIRT_PNT_OUT] = "NIRT_PNT_OUT";
    std::map<int, std::string> stype;
    stype[NIRT_MISS_SEG] = "NIRT_MISS_SEG";
    stype[NIRT_PARTITION_SEG] = "NIRT_PARTITION_SEG";
    stype[NIRT_OVERLAP_SEG] = "NIRT_OVERLAP_SEG";
    stype[NIRT_GAP_SEG] = "NIRT_GAP_SEG";

    std::vector<struct nirt_diff_ray_state> &dfs = nds->rays;
    for (size_t i = 0; i < dfs.size(); i++) {
	struct nirt_diff_ray_state *rstate = &(dfs[i]);
	struct nirt_diff_transition pt;
	bool ht = _nirt_next_transition(pt, rstate, NULL);
	long lcurr_ind = -1;
	long rcurr_ind = -1;
	bool have_first_seg = false;
	bu_log("found transition: (%s, %s) L: %f %f %f R: %f %f %f\n", postr[pt.origin].c_str(), ptstr[pt.type].c_str(), V3ARGS(pt.transition_left), V3ARGS(pt.transition_right));
	while (ht) {

	    struct nirt_diff_transition nt;
	    ht = _nirt_next_transition(nt, rstate, &pt);
	    if (ht) {
		bu_log("found transition: (%s, %s) L: %f %f %f R: %f %f %f\n", postr[nt.origin].c_str(), ptstr[nt.type].c_str(), V3ARGS(nt.transition_left), V3ARGS(nt.transition_right));
		if (!have_first_seg) {
		    lcurr_ind = (lcurr_ind == -1) ? rstate->lcurr : lcurr_ind;
		    rcurr_ind = (rcurr_ind == -1) ? rstate->rcurr : rcurr_ind;
		}
	    }

	    while (
		    (lcurr_ind != -1 && (!have_first_seg || lcurr_ind < rstate->lcurr)) ||
		    (rcurr_ind != -1 && (!have_first_seg || rcurr_ind < rstate->rcurr)) )  {
		struct nirt_seg *lcurr = (lcurr_ind >= 0) ? &rstate->old_segs[lcurr_ind] : NULL;
		struct nirt_seg *rcurr = (rcurr_ind >= 0) ? &rstate->new_segs[rcurr_ind] : NULL;
		double ldist = (lcurr) ? DIST_PNT_PNT_SQ(rstate->orig, lcurr->in) : DBL_MAX;
		double rdist = (rcurr) ? DIST_PNT_PNT_SQ(rstate->orig, rcurr->in) : DBL_MAX;

		if (ldist < nt.dist || (NEAR_EQUAL(ldist, nt.dist, rstate->nds->dist_delta_tol))) {
		    bu_log("left seg %ld(%s):\n\t\t%.15f %.15f %.15f -> %.15f %.15f %.15f\n\t\t%.15f %.15f %.15f -> %.15f %.15f %.15f\n", lcurr_ind, stype[lcurr->type].c_str(), V3ARGS(lcurr->in), V3ARGS(lcurr->out), V3ARGS(pt.transition_left), V3ARGS(nt.transition_left));
		    lcurr_ind++;
		}

		if (rdist < nt.dist || (NEAR_EQUAL(rdist, nt.dist, rstate->nds->dist_delta_tol))) {
		    bu_log("right seg %ld(%s):\n\t\t%.15f %.15f %.15f -> %.15f %.15f %.15f\n\t\t%.15f %.15f %.15f -> %.15f %.15f %.15f\n", rcurr_ind, stype[rcurr->type].c_str(), V3ARGS(rcurr->in), V3ARGS(rcurr->out), V3ARGS(pt.transition_right), V3ARGS(nt.transition_right));
		    rcurr_ind++;
		}

		have_first_seg = true;
	    }

	    if (ht) {
		pt = nt;
	    }
	}
    }
}


void
_nirt_diff_create(struct nirt_state *nss)
{
    nss->i->diff_state = new nirt_diff_state;
    nss->i->diff_state->nss = nss;
}


void
_nirt_diff_destroy(struct nirt_state *nss)
{
    bu_vls_free(&nss->i->diff_state->diff_file);
    // TODO - clean up nirt_diff objects
    delete nss->i->diff_state;
}


void
_nirt_diff_add_seg(struct nirt_state *nss, struct nirt_seg *nseg)
{
    if (nss->i->diff_state && nss->i->diff_state->cdiff) {
	nss->i->diff_state->cdiff->new_segs.push_back(*nseg);
    }
}


static bool
_nirt_partition_diff(struct nirt_diff_ray_state *ndiff, struct nirt_seg *left, struct nirt_seg *right)
{
    int have_diff = 0;
    struct nirt_diff_instance sd;

    if (!ndiff || !ndiff->nds)
	return false;

    fastf_t in_delta = DIST_PNT_PNT(left->in, right->in);
    fastf_t out_delta = DIST_PNT_PNT(left->in, right->in);
    fastf_t los_delta = fabs(left->los - right->los);
    fastf_t scaled_los_delta = fabs(left->scaled_los - right->scaled_los);
    fastf_t obliq_in_delta = fabs(left->obliq_in - right->obliq_in);
    fastf_t obliq_out_delta = fabs(left->obliq_out - right->obliq_out);

    if (left->reg_name != right->reg_name) have_diff = 1;
    if (left->path_name != right->path_name) have_diff = 1;
    if (left->reg_id != right->reg_id) have_diff = 1;
    if (in_delta > ndiff->nds->dist_delta_tol) have_diff = 1;
    if (out_delta > ndiff->nds->dist_delta_tol) have_diff = 1;
    if (los_delta > ndiff->nds->los_delta_tol) have_diff = 1;
    if (scaled_los_delta > ndiff->nds->scaled_los_delta_tol) have_diff = 1;
    if (obliq_in_delta > ndiff->nds->obliq_delta_tol) have_diff = 1;
    if (obliq_out_delta > ndiff->nds->obliq_delta_tol) have_diff = 1;
    if (have_diff) {
	sd.left = left;
	sd.right = right;
	sd.in_delta = in_delta;
	sd.out_delta = out_delta;
	sd.los_delta = los_delta;
	sd.scaled_los_delta = scaled_los_delta;
	sd.obliq_in_delta = obliq_in_delta;
	sd.obliq_out_delta = obliq_out_delta;
	ndiff->diffs.push_back(sd);
	return true;
    }
    return false;
}


static bool
_nirt_overlap_diff(struct nirt_diff_ray_state *ndiff, struct nirt_seg *left, struct nirt_seg *right)
{
    int have_diff = 0;
    struct nirt_diff_instance sd;

    if (!ndiff || !ndiff->nds)
	return false;

    fastf_t ov_in_delta = DIST_PNT_PNT(left->ov_in, right->ov_in);
    fastf_t ov_out_delta = DIST_PNT_PNT(left->ov_out, right->ov_out);
    fastf_t ov_los_delta = fabs(left->ov_los - right->ov_los);

    if (left->ov_reg1_name != right->ov_reg1_name) have_diff = 1;
    if (left->ov_reg2_name != right->ov_reg2_name) have_diff = 1;
    if (left->ov_reg1_id != right->ov_reg1_id) have_diff = 1;
    if (left->ov_reg2_id != right->ov_reg2_id) have_diff = 1;
    if (ov_in_delta > ndiff->nds->dist_delta_tol) have_diff = 1;
    if (ov_out_delta > ndiff->nds->dist_delta_tol) have_diff = 1;
    if (ov_los_delta > ndiff->nds->los_delta_tol) have_diff = 1;
    if (have_diff) {
	sd.left = left;
	sd.right = right;
	sd.ov_in_delta = ov_in_delta;
	sd.ov_out_delta = ov_out_delta;
	sd.ov_los_delta = ov_los_delta;
	ndiff->diffs.push_back(sd);
	return true;
    }
    return false;
}


static bool
_nirt_gap_diff(struct nirt_diff_ray_state *ndiff, struct nirt_seg *left, struct nirt_seg *right)
{
    int have_diff = 0;
    struct nirt_diff_instance sd;

    if (!ndiff || !ndiff->nds)
	return false;

    fastf_t gap_in_delta = DIST_PNT_PNT(left->gap_in, right->gap_in);
    fastf_t gap_los_delta = fabs(left->gap_los - right->gap_los);

    if (gap_in_delta > ndiff->nds->dist_delta_tol) have_diff = 1;
    if (gap_los_delta > ndiff->nds->los_delta_tol) have_diff = 1;
    if (have_diff) {
	sd.left = left;
	sd.right = right;
	sd.gap_in_delta = gap_in_delta;
	sd.gap_los_delta = gap_los_delta;
	ndiff->diffs.push_back(sd);
	return true;
    }
    return false;
}


static bool
_nirt_segs_diff(struct nirt_diff_ray_state *ndiff, struct nirt_seg *left, struct nirt_seg *right)
{
    if (!ndiff)
	return false;

    if (!left || !right || left->type != right->type) {
	/* Fundamental segment difference - no point going further, they're different */
	struct nirt_diff_instance sd;
	sd.left = left;
	sd.right = right;
	ndiff->diffs.push_back(sd);
	return true;
    }

    switch(left->type) {
	case NIRT_MISS_SEG:
	    /* Types don't differ and they're both misses - we're good */
	    return false;
	case NIRT_PARTITION_SEG:
	    return _nirt_partition_diff(ndiff, left, right);
	case NIRT_GAP_SEG:
	    return _nirt_gap_diff(ndiff, left, right);
	case NIRT_OVERLAP_SEG:
	    return _nirt_overlap_diff(ndiff, left, right);
	default:
	    nerr(ndiff->nds->nss, "NIRT diff error: unknown segment type: %d\n", left->type);
	    return 0;
    }
}


static const char *
_nirt_seg_string(int type)
{
    static const char *p = "Partition";
    static const char *g = "Gap";
    static const char *o = "Overlap";
    static const char *m = "Miss";

    switch (type) {
	case NIRT_MISS_SEG:
	    return m;
	case NIRT_PARTITION_SEG:
	    return p;
	case NIRT_GAP_SEG:
	    return g;
	case NIRT_OVERLAP_SEG:
	    return o;
	default:
	    return NULL;
    }
}


static int
_nirt_diff_report(struct nirt_state *nss)
{
    int reporting_diff = 0;
    int did_header = 0;
    struct bu_vls dreport = BU_VLS_INIT_ZERO;

    if (!nss) return 0;

    struct nirt_diff_state *nds = nss->i->diff_state;

    if (!nds || nds->rays.size() == 0) return 0;
    std::vector<struct nirt_diff_ray_state> &dfs = nds->rays;
    for (size_t i = 0; i < dfs.size(); i++) {
	struct nirt_diff_ray_state *d = &(dfs[i]);
	// Calculate diffs according to settings.  TODO - need to be more sophisticated about this - if a
	// segment is "missing" but the rays otherwise match, don't propagate the "offset" and report all
	// of the subsequent segments as different...
	size_t seg_max = (d->old_segs.size() > d->new_segs.size()) ? d->new_segs.size() : d->old_segs.size();
	for (size_t j = 0; j < seg_max; j++) {
	    _nirt_segs_diff(d, &d->old_segs[j], &d->new_segs[j]);
	}
	if (d->diffs.size() > 0 && !did_header) {
	    bu_vls_printf(&dreport, "Diff Results (%s):\n", bu_vls_cstr(&nds->diff_file));
	    did_header = 1;
	}

	if (d->diffs.size() > 0) bu_vls_printf(&dreport, "Found differences along Ray:\n  xyz %.17f %.17f %.17f\n  dir %.17f %.17f %.17f\n \n", V3ARGS(d->orig), V3ARGS(d->dir));
	for (size_t j = 0; j < d->diffs.size(); j++) {
	    struct nirt_diff_instance *sd = &d->diffs[j];
	    struct nirt_seg *left = sd->left;
	    struct nirt_seg *right = sd->right;
	    if (left->type != right->type) {
		bu_vls_printf(&dreport, "  Segment %zu type mismatch : Original %s, Current %s\n", j, _nirt_seg_string(sd->left->type), _nirt_seg_string(sd->right->type));
		nout(nss, "%s", bu_vls_cstr(&dreport));
		bu_vls_free(&dreport);
		return 1;
	    }
	    switch (sd->left->type) {
		case NIRT_MISS_SEG:
		    if (!nds->report_misses) continue;
		    /* TODO */
		    break;
		case NIRT_PARTITION_SEG:
		    if (!nds->report_partitions) continue;
		    bu_vls_printf(&dreport, "  Segment difference(%s)[%zu]:\n", _nirt_seg_string(sd->left->type), j);
		    if ((left->reg_name != right->reg_name) && nds->report_partition_reg_names) {
			bu_vls_printf(&dreport, "    Region Name: '%s' -> '%s'\n", left->reg_name.c_str(), right->reg_name.c_str());
			reporting_diff = 1;
		    }
		    if ((left->path_name != right->path_name) && nds->report_partition_path_names) {
			bu_vls_printf(&dreport, "    Path Name: '%s' -> '%s'\n", left->path_name.c_str(), right->path_name.c_str());
			reporting_diff = 1;
		    }
		    if (left->reg_id != right->reg_id && nds->report_partition_reg_ids) {
			bu_vls_printf(&dreport, "    Region ID: %d -> %d\n", left->reg_id, right->reg_id);
			reporting_diff = 1;
		    }
		    if (sd->in_delta > nds->dist_delta_tol && nds->report_partition_dists) {
			std::string oval = _nirt_dbl_to_str(sd->in_delta, _nirt_digits(nds->dist_delta_tol));
			bu_vls_printf(&dreport, "    DIST_PNT_PNT(in_old, in_new): %s\n", oval.c_str());
			reporting_diff = 1;
		    }
		    if (sd->out_delta > nds->dist_delta_tol && nds->report_partition_dists) {
			std::string oval = _nirt_dbl_to_str(sd->out_delta, _nirt_digits(nds->dist_delta_tol));
			bu_vls_printf(&dreport, "    DIST_PNT_PNT(out_old, out_new): %s\n", oval.c_str());
			reporting_diff = 1;
		    }
		    if (sd->los_delta > nds->los_delta_tol && nds->report_partition_dists) {
			std::string oval = _nirt_dbl_to_str(sd->los_delta, _nirt_digits(nds->los_delta_tol));
			bu_vls_printf(&dreport, "    Line-Of-Sight(LOS) %s: %s\n", delta_str, oval.c_str());
			reporting_diff = 1;
		    }
		    if (sd->scaled_los_delta > nds->scaled_los_delta_tol && nds->report_partition_dists) {
			std::string oval = _nirt_dbl_to_str(sd->scaled_los_delta, _nirt_digits(nds->los_delta_tol));
			bu_vls_printf(&dreport, "    Scaled Line-Of-Sight %s: %s\n", delta_str, oval.c_str());
			reporting_diff = 1;
		    }
		    if (sd->obliq_in_delta > nds->obliq_delta_tol && nds->report_partition_obliq) {
			std::string oval = _nirt_dbl_to_str(sd->obliq_in_delta, _nirt_digits(nds->obliq_delta_tol));
			bu_vls_printf(&dreport, "    Input Normal Obliquity %s: %s\n", delta_str, oval.c_str());
			reporting_diff = 1;
		    }
		    if (sd->obliq_out_delta > nds->obliq_delta_tol && nds->report_partition_obliq) {
			std::string oval = _nirt_dbl_to_str(sd->obliq_out_delta, _nirt_digits(nds->obliq_delta_tol));
			bu_vls_printf(&dreport, "    Output Normal Obliquity %s: %s\n", delta_str, oval.c_str());
			reporting_diff = 1;
		    }
		    break;
		case NIRT_GAP_SEG:
		    if (!nds->report_gaps) continue;
		    bu_vls_printf(&dreport, "  Segment difference(%s)[%zu]:\n", _nirt_seg_string(sd->left->type), j);
		    if (sd->gap_in_delta > nds->dist_delta_tol && nds->report_gap_dists) {
			std::string oval = _nirt_dbl_to_str(sd->gap_in_delta, _nirt_digits(nds->dist_delta_tol));
			bu_vls_printf(&dreport, "    DIST_PNT_PNT(gap_in_old, gap_in_new): %s\n", oval.c_str());
			reporting_diff = 1;
		    }
		    if (sd->gap_los_delta > nds->los_delta_tol && nds->report_gap_dists) {
			std::string oval = _nirt_dbl_to_str(sd->gap_los_delta, _nirt_digits(nds->los_delta_tol));
			bu_vls_printf(&dreport, "    Gap Line-Of-Sight (LOS) %s: %s\n", delta_str, oval.c_str());
			reporting_diff = 1;
		    }
		    break;
		case NIRT_OVERLAP_SEG:
		    if (!nds->report_overlaps) continue;
		    bu_vls_printf(&dreport, "  Segment difference(%s)[%zu]:\n", _nirt_seg_string(sd->left->type), j);
		    if ((left->ov_reg1_name != right->ov_reg1_name) && nds->report_overlap_reg_names) {
			bu_vls_printf(&dreport, "    Region 1 Name: '%s' -> '%s'\n", left->ov_reg1_name.c_str(), right->ov_reg1_name.c_str());
			reporting_diff = 1;
		    }
		    if ((left->ov_reg2_name != right->ov_reg2_name) && nds->report_overlap_reg_names) {
			bu_vls_printf(&dreport, "    Region 2 Name: '%s' -> '%s'\n", left->ov_reg2_name.c_str(), right->ov_reg2_name.c_str());
			reporting_diff = 1;
		    }
		    if (left->ov_reg1_id != right->ov_reg1_id && nds->report_overlap_reg_ids) {
			bu_vls_printf(&dreport, "    Region 1 ID: %d -> %d\n", left->ov_reg1_id, right->ov_reg1_id);
			reporting_diff = 1;
		    }
		    if (left->ov_reg2_id != right->ov_reg2_id && nds->report_overlap_reg_ids) {
			bu_vls_printf(&dreport, "    Region 2 ID: %d -> %d\n", left->ov_reg2_id, right->ov_reg2_id);
			reporting_diff = 1;
		    }
		    if (sd->ov_in_delta > nds->dist_delta_tol && nds->report_overlap_dists) {
			std::string oval = _nirt_dbl_to_str(sd->ov_in_delta, _nirt_digits(nds->dist_delta_tol));
			bu_vls_printf(&dreport, "    DIST_PNT_PNT(ov_in_old, ov_in_new): %s\n", oval.c_str());
			reporting_diff = 1;
		    }
		    if (sd->ov_out_delta > nds->dist_delta_tol && nds->report_overlap_dists) {
			std::string oval = _nirt_dbl_to_str(sd->ov_out_delta, _nirt_digits(nds->dist_delta_tol));
			bu_vls_printf(&dreport, "    DIST_PNT_PNT(ov_out_old, ov_out_new): %s\n", oval.c_str());
			reporting_diff = 1;
		    }
		    if (sd->ov_los_delta > nds->los_delta_tol && nds->report_overlap_dists) {
			std::string oval = _nirt_dbl_to_str(sd->ov_los_delta, _nirt_digits(nds->los_delta_tol));
			bu_vls_printf(&dreport, "    Overlap Line-Of-Sight (LOS) %s: %s\n", delta_str, oval.c_str());
			reporting_diff = 1;
		    }
		    break;
		default:
		    nerr(nss, "NIRT diff error: unknown segment type: %d\n", left->type);
		    return 0;
	    }
	}
	d->diffs.clear();
    }

    if (reporting_diff) {
	nout(nss, "%s", bu_vls_cstr(&dreport));
    } else {
	nout(nss, "No differences found\n");
    }
    bu_vls_free(&dreport);

    return reporting_diff;
}


static bool
parse_ray(struct nirt_diff_state *nds, std::string &line)
{
    // First up, find out what formatting version we need to deal with.
    // This is done so that as long as the RAY,#, prefix is respected,
    // we can take older diff.nrt files and use them in newer code while
    // preserving the ability to adjust the format if we need/want to.

    struct nirt_diff_ray_state df;
    df.nds = nds;

    std::regex ray_regex("RAY,([0-9]+),(.*)");

    std::smatch s1;
    if (!std::regex_search(line, s1, ray_regex)) {
	nerr(nds->nss, "Error processing ray line \"%s\"!\nUnable to identify formatting version\n", line.c_str());
	return false;
    }

    int ray_version = _nirt_str_to_int(s1[1]);
    std::string ray_data = s1[2];

    std::cerr << "version: " << ray_version << "\n";
    std::cerr << "data   : " << ray_data << "\n";

    if (ray_version == 1) {
	std::vector<std::string> substrs = _nirt_string_split(ray_data);
	if (substrs.size() != 6) {
	    nerr(nds->nss, "Error processing ray line \"%s\"!\nExpected 6 elements, found %zu\n", ray_data.c_str(), substrs.size());
	    return false;
	}
	VSET(df.orig, _nirt_str_to_dbl(substrs[0], 0), _nirt_str_to_dbl(substrs[1], 0), _nirt_str_to_dbl(substrs[2], 0));
	VSET(df.dir,  _nirt_str_to_dbl(substrs[3], 0), _nirt_str_to_dbl(substrs[4], 0), _nirt_str_to_dbl(substrs[5], 0));
#ifdef NIRT_DIFF_DEBUG
	bu_log("Found RAY:\n");
	bu_log("origin   : %0.17f, %0.17f, %0.17f\n", V3ARGS(df.orig));
	bu_log("direction: %0.17f, %0.17f, %0.17f\n", V3ARGS(df.dir));
#endif
	nds->rays.push_back(df);
	nds->cdiff = &(nds->rays[nds->rays.size() - 1]);
	return true;
    }

    nerr(nds->nss, "Error processing ray line \"%s\"!\nUnsupported version: %d\n", line.c_str(), ray_version);
    return false;
}


static bool
parse_hit(struct nirt_diff_state *nds, std::string &line)
{
    std::regex hit_regex("HIT,([0-9]+),(.*)");

    std::smatch s1;
    if (!std::regex_search(line, s1, hit_regex)) {
	nerr(nds->nss, "Error processing hit line \"%s\"!\nUnable to identify formatting version\n", line.c_str());
	return false;
    }

    if (!nds->cdiff) {
	nerr(nds->nss, "Error: Hit line found but no ray set.\n");
	return false;
    }

    int hit_version = _nirt_str_to_int(s1[1]);
    std::string hit_data = s1[2];

    if (hit_version == 1) {
	std::vector<std::string> substrs = _nirt_string_split(hit_data);

	if (substrs.size() != 15) {
	    nerr(nds->nss, "Error processing hit line \"%s\"!\nExpected 15 elements, found %zu\n", hit_data.c_str(), substrs.size());
	    return -1;
	}

	struct nirt_seg *segp = new struct nirt_seg;
	segp->type = NIRT_PARTITION_SEG;
	struct bu_vls rdecode = BU_VLS_INIT_ZERO;
	bu_vls_decode(&rdecode, substrs[0].c_str());
	segp->reg_name = std::string(bu_vls_cstr(&rdecode));
	bu_vls_free(&rdecode);
	//bu_vls_printf(segp->reg_name, "%s", substrs[0].c_str());
	struct bu_vls pdecode = BU_VLS_INIT_ZERO;
	bu_vls_decode(&pdecode, substrs[1].c_str());
	segp->path_name = std::string(bu_vls_cstr(&pdecode));
	bu_vls_free(&pdecode);
	segp->reg_id = _nirt_str_to_int(substrs[2]);
	VSET(segp->in, _nirt_str_to_dbl(substrs[3], 0), _nirt_str_to_dbl(substrs[4], 0), _nirt_str_to_dbl(substrs[5], 0));
	segp->d_in = _nirt_str_to_dbl(substrs[6], 0);
	VSET(segp->out, _nirt_str_to_dbl(substrs[7], 0), _nirt_str_to_dbl(substrs[8], 0), _nirt_str_to_dbl(substrs[9], 0));
	segp->d_out = _nirt_str_to_dbl(substrs[10], 0);
	segp->los = _nirt_str_to_dbl(substrs[11], 0);
	segp->scaled_los = _nirt_str_to_dbl(substrs[12], 0);
	segp->obliq_in = _nirt_str_to_dbl(substrs[13], 0);
	segp->obliq_out = _nirt_str_to_dbl(substrs[14], 0);
#ifdef NIRT_DIFF_DEBUG
	bu_log("Found %s:\n", line.c_str());
	bu_log("  reg_name: %s\n", bu_vls_cstr(segp->reg_name));
	bu_log("  path_name: %s\n", bu_vls_cstr(segp->path_name));
	bu_log("  reg_id: %d\n", segp->reg_id);
	bu_log("  in: %0.17f, %0.17f, %0.17f\n", V3ARGS(segp->in));
	bu_log("  out: %0.17f, %0.17f, %0.17f\n", V3ARGS(segp->out));
	bu_log("  d_in: %0.17f d_out: %0.17f\n", segp->d_in, segp->d_out);
	bu_log("  los: %0.17f  scaled_los: %0.17f\n", segp->los, segp->scaled_los);
	bu_log("  obliq_in: %0.17f  obliq_out: %0.17f\n", segp->obliq_in, segp->obliq_out);
#endif
	nds->cdiff->old_segs.push_back(*segp);
	delete segp;
	return true;
    }

    nerr(nds->nss, "Error processing hit line \"%s\"!\nUnsupported version: %d\n", line.c_str(), hit_version);
    return false;

}


static bool
parse_gap(struct nirt_diff_state *nds, std::string &line)
{
    std::regex gap_regex("GAP,([0-9]+),(.*)");

    std::smatch s1;
    if (!std::regex_search(line, s1, gap_regex)) {
	nerr(nds->nss, "Error processing gap line \"%s\"!\nUnable to identify formatting version\n", line.c_str());
	return false;
    }

    if (!nds->cdiff) {
	nerr(nds->nss, "Error: Gap line found but no ray set.\n");
	return false;
    }

    int gap_version = _nirt_str_to_int(s1[1]);
    std::string gap_data = s1[2];

    if (gap_version == 1) {
	std::vector<std::string> substrs = _nirt_string_split(gap_data);

	if (substrs.size() != 7) {
	    nerr(nds->nss, "Error processing gap line \"%s\"!\nExpected 7 elements, found %zu\n", gap_data.c_str(), substrs.size());
	    return -1;
	}
	struct nirt_seg *segp = new struct nirt_seg;
	segp->type = NIRT_GAP_SEG;
	VSET(segp->gap_in, _nirt_str_to_dbl(substrs[0], 0), _nirt_str_to_dbl(substrs[1], 0), _nirt_str_to_dbl(substrs[2], 0));
	VSET(segp->in, _nirt_str_to_dbl(substrs[3], 0), _nirt_str_to_dbl(substrs[4], 0), _nirt_str_to_dbl(substrs[5], 0));
	segp->gap_los = _nirt_str_to_dbl(substrs[6], 0);
#ifdef NIRT_DIFF_DEBUG
	bu_log("Found %s:\n", line.c_str());
	bu_log("  in: %0.17f, %0.17f, %0.17f\n", V3ARGS(segp->gap_in));
	bu_log("  out: %0.17f, %0.17f, %0.17f\n", V3ARGS(segp->in));
	bu_log("  gap_los: %0.17f\n", segp->gap_los);
#endif
	nds->cdiff->old_segs.push_back(*segp);
	delete segp;
	return true;
    }

    nerr(nds->nss, "Error processing gap line \"%s\"!\nUnsupported version: %d\n", line.c_str(), gap_version);
    return false;

}


static bool
parse_miss(struct nirt_diff_state *nds, std::string &line)
{
    std::regex miss_regex("MISS,([0-9]+),(.*)");

    std::smatch s1;
    if (!std::regex_search(line, s1, miss_regex)) {
	nerr(nds->nss, "error processing miss line \"%s\"!\nunable to identify formatting version\n", line.c_str());
	return false;
    }

    if (!nds->cdiff) {
	nerr(nds->nss, "error: miss line found but no ray set.\n");
	return false;
    }

    int miss_version = _nirt_str_to_int(s1[1]);

    if (miss_version == 1) {
	struct nirt_seg *segp = new struct nirt_seg;
	segp->type = NIRT_MISS_SEG;
#ifdef NIRT_DIFF_DEBUG
	bu_log("Found MISS\n");
#endif
	nds->cdiff->old_segs.push_back(*segp);
	delete segp;
	return true;
    }

    nerr(nds->nss, "error processing miss line \"%s\"!\nunsupported version: %d\n", line.c_str(), miss_version);
    return false;

}


static bool
parse_overlap(struct nirt_diff_state *nds, std::string &line)
{
    std::regex overlap_regex("OVERLAP,([0-9]+),(.*)");

    std::smatch s1;
    if (!std::regex_search(line, s1, overlap_regex)) {
	nerr(nds->nss, "Error processing overlap line \"%s\"!\nUnable to identify formatting version\n", line.c_str());
	return false;
    }

    if (!nds->cdiff) {
	nerr(nds->nss, "Error: Overlap line found but no ray set.\n");
	return false;
    }

    int overlap_version = _nirt_str_to_int(s1[1]);
    std::string overlap_data = s1[2];

    if (overlap_version == 1) {
	std::vector<std::string> substrs = _nirt_string_split(overlap_data);
	if (substrs.size() != 11) {
	    nerr(nds->nss, "Error processing overlap line \"%s\"!\nExpected 11 elements, found %zu\n", overlap_data.c_str(), substrs.size());
	    return false;
	}
	struct nirt_seg *segp = new struct nirt_seg;
	segp->type = NIRT_OVERLAP_SEG;
	struct bu_vls r1decode = BU_VLS_INIT_ZERO;
	bu_vls_decode(&r1decode, substrs[0].c_str());
	segp->ov_reg1_name = std::string(bu_vls_cstr(&r1decode));
	bu_vls_free(&r1decode);
	struct bu_vls r2decode = BU_VLS_INIT_ZERO;
	bu_vls_decode(&r2decode, substrs[1].c_str());
	segp->ov_reg2_name = std::string(bu_vls_cstr(&r2decode));
	bu_vls_free(&r2decode);
	segp->ov_reg1_id = _nirt_str_to_int(substrs[2]);
	segp->ov_reg2_id = _nirt_str_to_int(substrs[3]);
	VSET(segp->ov_in, _nirt_str_to_dbl(substrs[4], 0), _nirt_str_to_dbl(substrs[5], 0), _nirt_str_to_dbl(substrs[6], 0));
	VSET(segp->ov_out, _nirt_str_to_dbl(substrs[7], 0), _nirt_str_to_dbl(substrs[8], 0), _nirt_str_to_dbl(substrs[9], 0));
	segp->ov_los = _nirt_str_to_dbl(substrs[10], 0);
#ifdef NIRT_DIFF_DEBUG
	bu_log("Found %s:\n", line.c_str());
	bu_log("  ov_reg1_name: %s\n", bu_vls_cstr(&segp->ov_reg1_name));
	bu_log("  ov_reg2_name: %s\n", bu_vls_cstr(&segp->ov_reg2_name));
	bu_log("  ov_reg1_id: %d\n", segp->ov_reg1_id);
	bu_log("  ov_reg2_id: %d\n", segp->ov_reg2_id);
	bu_log("  ov_in: %0.17f, %0.17f, %0.17f\n", V3ARGS(segp->ov_in));
	bu_log("  ov_out: %0.17f, %0.17f, %0.17f\n", V3ARGS(segp->ov_out));
	bu_log("  ov_los: %0.17f\n", segp->ov_los);
#endif
	nds->cdiff->old_segs.push_back(*segp);
	delete segp;
	return true;
    }

    nerr(nds->nss, "Error processing overlap line \"%s\"!\nUnsupported version: %d\n", line.c_str(), overlap_version);
    return false;
}


static int
_nirt_cmd_msgs(void *ndsv, int argc, const char **argv, const char *us, const char *ps)
{
    struct nirt_diff_state *nds = (struct nirt_diff_state *)ndsv;
    struct nirt_state *nss = nds->nss;
    if (argc == 2 && BU_STR_EQUAL(argv[1], HELPFLAG)) {
	nout(nss, "%s\n%s\n", us, ps);
	return 1;
    }
    if (argc == 2 && BU_STR_EQUAL(argv[1], PURPOSEFLAG)) {
	nout(nss, "%s\n", ps);
	return 1;
    }
    return 0;
}


extern "C" int
_nirt_diff_cmd_clear(void *ndsv, int argc, const char **argv)
{
    struct nirt_diff_state *nds = (struct nirt_diff_state *)ndsv;

    const char *usage_string = "diff [options] clear";
    const char *purpose_string = "reset NIRT diff state";
    if (_nirt_cmd_msgs(nds, argc, argv, usage_string, purpose_string)) {
	return 0;
    }

    argv++; argc--;

    nds->rays.clear();
    nds->diff_ready = false;
    nds->cdiff = NULL;
    bu_vls_trunc(&nds->diff_file, 0);
    return 0;
}


extern "C" int
_nirt_diff_cmd_load(void *ndsv, int argc, const char **argv)
{
    size_t cmt = std::string::npos;
    struct nirt_diff_state *nds = (struct nirt_diff_state *)ndsv;
    struct nirt_state *nss = nds->nss;

    const char *usage_string = "diff [options] load <nirt_diff_file>";
    const char *purpose_string = "load pre-existing NIRT shotline information";
    if (_nirt_cmd_msgs(nds, argc, argv, usage_string, purpose_string)) {
	return 0;
    }

    argv++; argc--;

    if (argc != 1) {
	nerr(nss, "please specify a nirt diff file to load.\n");
	return -1;
    }

    if (nds->diff_ready) {
	nerr(nss, "diff file already loaded.  to reset, run \"diff clear\"\n");
	return -1;
    }

    std::string line;
    std::ifstream ifs;
    ifs.open(argv[0]);
    if (!ifs.is_open()) {
	nerr(nss, "error: could not open file %s\n", argv[0]);
	return -1;
    }

    if (nss->i->need_reprep) {
	/* if we need to (re)prep, do it now. failure is an error. */
	if (_nirt_raytrace_prep(nss)) {
	    nerr(nss, "error: raytrace prep failed!\n");
	    return -1;
	}
    } else {
	/* based on current settings, tell the ap which rtip to use */
	nss->i->ap->a_rt_i = _nirt_get_rtip(nss);
	nss->i->ap->a_resource = _nirt_get_resource(nss);
    }

    bu_vls_sprintf(&nds->diff_file, "%s", argv[0]);
    // TODO - temporarily suppress all formatting output for a silent diff run...
    while (std::getline(ifs, line)) {
	/* if part of the line is commented, skip that part */
	cmt = _nirt_find_first_unescaped(line, "#", 0);
	if (cmt != std::string::npos) {
	    line.erase(cmt);
	}

	/* if the whole line was a comment, skip it */
	_nirt_trim_whitespace(line);
	if (!line.length()) continue;

	/* not a comment - has to be a valid type, or it's an error */
	int ltype = -1;
	ltype = (ltype < 0 && !line.compare(0, 4, "RAY,")) ? 0 : ltype;
	ltype = (ltype < 0 && !line.compare(0, 4, "HIT,")) ? 1 : ltype;
	ltype = (ltype < 0 && !line.compare(0, 4, "GAP,")) ? 2 : ltype;
	ltype = (ltype < 0 && !line.compare(0, 5, "MISS,")) ? 3 : ltype;
	ltype = (ltype < 0 && !line.compare(0, 8, "OVERLAP,")) ? 4 : ltype;
	if (ltype < 0) {
	    nerr(nss, "Error processing diff file, line \"%s\"!\nUnknown line type\n", line.c_str());
	    return -1;
	}

	/* Ray */
	if (ltype == 0) {
	    if (!parse_ray(nds, line)) {
		_nirt_diff_cmd_clear((void *)nds, 0, NULL);
		return -1;
	    }
	    continue;
	}

	/* Hit */
	if (ltype == 1) {
	    if (!parse_hit(nds, line)) {
		_nirt_diff_cmd_clear((void *)nds, 0, NULL);
		return -1;
	    }
	    continue;
	}

	/* Gap */
	if (ltype == 2) {
	    if (!parse_gap(nds, line)) {
		_nirt_diff_cmd_clear((void *)nds, 0, NULL);
		return -1;
	    }
	    continue;
	}

	/* Miss */
	if (ltype == 3) {
	    if (!parse_miss(nds, line)) {
		_nirt_diff_cmd_clear((void *)nds, 0, NULL);
		return -1;
	    }
	    continue;
	}

	/* Overlap */
	if (ltype == 4) {
	    if (!parse_overlap(nds, line)) {
		_nirt_diff_cmd_clear((void *)nds, 0, NULL);
		return -1;
	    }
	    continue;
	}
#ifdef NIRT_DIFF_DEBUG
	nerr(nss, "Warning - unknown line type, skipping: %s\n", line.c_str());
#endif
    }

    ifs.close();

    // Done with if_hit and friends
    return 0;
}


// Thought - if we have rays but no pre-defined output, write out the
// expected output to stdout - in this mode diff will generate a diff
// input file from a supplied list of rays.
extern "C" int
_nirt_diff_cmd_run(void *ndsv, int argc, const char **argv)
{
    struct nirt_diff_state *nds = (struct nirt_diff_state *)ndsv;
    struct nirt_state *nss = nds->nss;

    const char *usage_string = "diff [options] run";
    const char *purpose_string = "compute new shotlines to compare with loaded data";
    if (_nirt_cmd_msgs(nds, argc, argv, usage_string, purpose_string)) {
	return 0;
    }

    argv++; argc--;

    // Clear any preexisting new_segs in case this is a repeated run
    for (unsigned int i = 0; i < nds->rays.size(); i++) {
	nds->rays[i].new_segs.clear();
    }

    for (unsigned int i = 0; i < nds->rays.size(); i++) {
	nds->cdiff = &(nds->rays[i]);
	VMOVE(nss->i->vals->dir,  nds->cdiff->dir);
	VMOVE(nss->i->vals->orig, nds->cdiff->orig);
	_nirt_targ2grid(nss);
	_nirt_dir2ae(nss);
	for (int ii = 0; ii < 3; ++ii) {
	    nss->i->ap->a_ray.r_pt[ii] = nss->i->vals->orig[ii];
	    nss->i->ap->a_ray.r_dir[ii] = nss->i->vals->dir[ii];
	}
	// TODO - rethink this container...
	_nirt_init_ovlp(nss);
	(void)rt_shootray(nss->i->ap);
    }

    // Analyze the new partition set
    _nirt_segs_analyze(nds);

    nds->diff_ready = true;
    return 0;
}


extern "C" int
_nirt_diff_cmd_report(void *ndsv, int argc, const char **argv)
{
    struct nirt_diff_state *nds = (struct nirt_diff_state *)ndsv;

    const char *usage_string = "diff [options] report";
    const char *purpose_string = "report differences (if any) between old and new shotlines";
    if (_nirt_cmd_msgs(nds, argc, argv, usage_string, purpose_string)) {
	return 0;
    }

    argv++; argc--;

    // Report diff results according to the NIRT diff settings.
    if (!nds->diff_ready) {
	nerr(nds->nss, "No diff file loaded - please load a diff file with \"diff load <filename>\"\n");
	return -1;
    } else {
	return (_nirt_diff_report(nds->nss) >= 0) ? 0 : -1;
    }
}


extern "C" int
_nirt_diff_cmd_settings(void *ndsv, int argc, const char **argv)
{
    struct nirt_diff_state *nds = (struct nirt_diff_state *)ndsv;
    struct nirt_state *nss = nds->nss;

    const char *usage_string = "diff [options] settings [key] [val]";
    const char *purpose_string = "print and change reporting and tolerance settings";
    if (_nirt_cmd_msgs(nds, argc, argv, usage_string, purpose_string)) {
	return 0;
    }

    argv++; argc--;

    if (!argc) {
	//print current settings
	nout(nss, "report_partitions:           %d\n", nds->report_partitions);
	nout(nss, "report_misses:               %d\n", nds->report_misses);
	nout(nss, "report_gaps:                 %d\n", nds->report_gaps);
	nout(nss, "report_overlaps:             %d\n", nds->report_overlaps);
	nout(nss, "report_partition_reg_ids:    %d\n", nds->report_partition_reg_ids);
	nout(nss, "report_partition_reg_names:  %d\n", nds->report_partition_reg_names);
	nout(nss, "report_partition_path_names: %d\n", nds->report_partition_path_names);
	nout(nss, "report_partition_dists:      %d\n", nds->report_partition_dists);
	nout(nss, "report_partition_obliq:      %d\n", nds->report_partition_obliq);
	nout(nss, "report_overlap_reg_names:    %d\n", nds->report_overlap_reg_names);
	nout(nss, "report_overlap_reg_ids:      %d\n", nds->report_overlap_reg_ids);
	nout(nss, "report_overlap_dists:        %d\n", nds->report_overlap_dists);
	nout(nss, "report_overlap_obliq:        %d\n", nds->report_overlap_obliq);
	nout(nss, "report_gap_dists:            %d\n", nds->report_gap_dists);
	nout(nss, "dist_delta_tol:              %g\n", nds->dist_delta_tol);
	nout(nss, "obliq_delta_tol:             %g\n", nds->obliq_delta_tol);
	nout(nss, "los_delta_tol:               %g\n", nds->los_delta_tol);
	nout(nss, "scaled_los_delta_tol:        %g\n", nds->scaled_los_delta_tol);
	return 0;
    }
    if (argc == 1) {
	//print specific setting
	if (BU_STR_EQUAL(argv[0], "report_partitions")) nout(nss, "%d\n", nds->report_partitions);
	if (BU_STR_EQUAL(argv[0], "report_misses")) nout(nss, "%d\n", nds->report_misses);
	if (BU_STR_EQUAL(argv[0], "report_gaps")) nout(nss, "%d\n", nds->report_gaps);
	if (BU_STR_EQUAL(argv[0], "report_overlaps")) nout(nss, "%d\n", nds->report_overlaps);
	if (BU_STR_EQUAL(argv[0], "report_partition_reg_ids")) nout(nss, "%d\n", nds->report_partition_reg_ids);
	if (BU_STR_EQUAL(argv[0], "report_partition_reg_names")) nout(nss, "%d\n", nds->report_partition_reg_names);
	if (BU_STR_EQUAL(argv[0], "report_partition_path_names")) nout(nss, "%d\n", nds->report_partition_path_names);
	if (BU_STR_EQUAL(argv[0], "report_partition_dists")) nout(nss, "%d\n", nds->report_partition_dists);
	if (BU_STR_EQUAL(argv[0], "report_partition_obliq")) nout(nss, "%d\n", nds->report_partition_obliq);
	if (BU_STR_EQUAL(argv[0], "report_overlap_reg_names")) nout(nss, "%d\n", nds->report_overlap_reg_names);
	if (BU_STR_EQUAL(argv[0], "report_overlap_reg_ids")) nout(nss, "%d\n", nds->report_overlap_reg_ids);
	if (BU_STR_EQUAL(argv[0], "report_overlap_dists")) nout(nss, "%d\n", nds->report_overlap_dists);
	if (BU_STR_EQUAL(argv[0], "report_overlap_obliq")) nout(nss, "%d\n", nds->report_overlap_obliq);
	if (BU_STR_EQUAL(argv[0], "report_gap_dists")) nout(nss, "%d\n", nds->report_gap_dists);
	if (BU_STR_EQUAL(argv[0], "dist_delta_tol")) nout(nss, "%g\n", nds->dist_delta_tol);
	if (BU_STR_EQUAL(argv[0], "obliq_delta_tol")) nout(nss, "%g\n", nds->obliq_delta_tol);
	if (BU_STR_EQUAL(argv[0], "los_delta_tol")) nout(nss, "%g\n", nds->los_delta_tol);
	if (BU_STR_EQUAL(argv[0], "scaled_los_delta_tol")) nout(nss, "%g\n", nds->scaled_los_delta_tol);
	return 0;
    }
    if (argc == 2) {
	//set setting
	struct bu_vls opt_msg = BU_VLS_INIT_ZERO;
	bool *setting_bool = NULL;
	fastf_t *setting_fastf_t = NULL;
	if (BU_STR_EQUAL(argv[0], "report_partitions")) setting_bool = &(nds->report_partitions);
	if (BU_STR_EQUAL(argv[0], "report_misses")) setting_bool = &(nds->report_misses);
	if (BU_STR_EQUAL(argv[0], "report_gaps")) setting_bool = &(nds->report_gaps);
	if (BU_STR_EQUAL(argv[0], "report_overlaps")) setting_bool = &(nds->report_overlaps);
	if (BU_STR_EQUAL(argv[0], "report_partition_reg_ids")) setting_bool = &(nds->report_partition_reg_ids);
	if (BU_STR_EQUAL(argv[0], "report_partition_reg_names")) setting_bool = &(nds->report_partition_reg_names);
	if (BU_STR_EQUAL(argv[0], "report_partition_path_names")) setting_bool = &(nds->report_partition_path_names);
	if (BU_STR_EQUAL(argv[0], "report_partition_dists")) setting_bool = &(nds->report_partition_dists);
	if (BU_STR_EQUAL(argv[0], "report_partition_obliq")) setting_bool = &(nds->report_partition_obliq);
	if (BU_STR_EQUAL(argv[0], "report_overlap_reg_names")) setting_bool = &(nds->report_overlap_reg_names);
	if (BU_STR_EQUAL(argv[0], "report_overlap_reg_ids")) setting_bool = &(nds->report_overlap_reg_ids);
	if (BU_STR_EQUAL(argv[0], "report_overlap_dists")) setting_bool = &(nds->report_overlap_dists);
	if (BU_STR_EQUAL(argv[0], "report_overlap_obliq")) setting_bool = &(nds->report_overlap_obliq);
	if (BU_STR_EQUAL(argv[0], "report_gap_dists")) setting_bool = &(nds->report_gap_dists);
	if (BU_STR_EQUAL(argv[0], "dist_delta_tol")) setting_fastf_t = &(nds->dist_delta_tol);
	if (BU_STR_EQUAL(argv[0], "obliq_delta_tol")) setting_fastf_t = &(nds->obliq_delta_tol);
	if (BU_STR_EQUAL(argv[0], "los_delta_tol")) setting_fastf_t = &(nds->los_delta_tol);
	if (BU_STR_EQUAL(argv[0], "scaled_los_delta_tol")) setting_fastf_t = &(nds->scaled_los_delta_tol);

	if (setting_bool) {
	    if (bu_opt_bool(&opt_msg, 1, (const char **)&argv[1], (void *)setting_bool) == -1) {
		nerr(nss, "Error: bu_opt value read failure: %s\n", bu_vls_cstr(&opt_msg));
		bu_vls_free(&opt_msg);
		return -1;
	    }
	    return 0;
	}
	if (setting_fastf_t) {
	    if (BU_STR_EQUAL(argv[1], "BN_TOL_DIST") || BU_STR_EQUIV(argv[1], "default")) {
		*setting_fastf_t = BN_TOL_DIST;
		return 0;
	    }
	    if (bu_opt_fastf_t(&opt_msg, 1, (const char **)&argv[1], (void *)setting_fastf_t) == -1) {
		nerr(nss, "Error: bu_opt value read failure: %s\n", bu_vls_cstr(&opt_msg));
		bu_vls_free(&opt_msg);
		return -1;
	    }
	    return 0;
	}
    }

    // anything else is an error
    return -1;
}


extern "C" int
_nirt_diff_help(void *ndsv, int argc, const char **argv)
{
    struct nirt_diff_state *nds = (struct nirt_diff_state *)ndsv;
    struct nirt_state *nss = nds->nss;

    if (!argc || !argv || BU_STR_EQUAL(argv[0], "help")) {
	nout(nss, "diff [options] subcommand [args]\n");
	if (nds->d) {
	    char *option_help = bu_opt_describe(nds->d, NULL);
	    if (option_help) {
		nout(nss, "Options:\n%s\n", option_help);
		bu_free(option_help, "help str");
	    }
	}
	nout(nss, "Available subcommands:\n");
	const struct bu_cmdtab *ctp = NULL;
	int ret;
	const char *helpflag[2];
	helpflag[1] = PURPOSEFLAG;
	for (ctp = nds->cmds; ctp->ct_name != (char *)NULL; ctp++) {
	    nout(nss, "  %s\t\t", ctp->ct_name);
	    if (!BU_STR_EQUAL(ctp->ct_name, "help")) {
		helpflag[0] = ctp->ct_name;
		bu_cmd(nds->cmds, 2, helpflag, 0, (void *)nds, &ret);
	    } else {
		nout(nss, "print help and exit\n");
	    }
	}
    } else {
	int ret;
	const char **helpargv = (const char **)bu_calloc(argc+1, sizeof(char *), "help argv");
	helpargv[0] = argv[0];
	helpargv[1] = HELPFLAG;
	for (int i = 1; i < argc; i++) {
	    helpargv[i+1] = argv[i];
	}
	bu_cmd(nds->cmds, argc+1, helpargv, 0, (void *)nds, &ret);
	bu_free(helpargv, "help argv");
	return ret;
    }

    return 0;
}


const struct bu_cmdtab _nirt_diff_cmds[] = {
    { "load", _nirt_diff_cmd_load},
    { "run", _nirt_diff_cmd_run},
    { "report", _nirt_diff_cmd_report},
    { "clear", _nirt_diff_cmd_clear},
    { "settings", _nirt_diff_cmd_settings},
    { (char *)NULL, NULL}
};


//#define NIRT_DIFF_DEBUG 1

extern "C" int
_nirt_cmd_diff(void *ns, int argc, const char *argv[])
{
    if (!ns) return -1;
    struct nirt_state *nss = (struct nirt_state *)ns;
    struct nirt_diff_state *nds = nss->i->diff_state;
    int help = 0;
    double tol = 0;
    struct bu_opt_desc d[3];
    // TODO - add reporting options for enabling/disabling region/path,
    // partition length, normal, and overlap ordering diffs.  For example, if
    // r1 and r2 have the same shape in space, we may want to suppress name
    // based differences and look at other aspects of the shotline.  Also need
    // sorting options for output - max partition diff first, max normal diff
    // first, max numerical delta, etc...
    BU_OPT(d[0],  "h", "help",       "",             NULL,   &help, "print help and exit");
    BU_OPT(d[1],  "t", "tol",   "<val>",  &bu_opt_fastf_t,   &tol,  "set diff tolerance");
    BU_OPT_NULL(d[2]);

    // Need for help printing
    nds->d = (struct bu_opt_desc *)d;
    nds->cmds = _nirt_diff_cmds;

    argv++; argc--;

    // Diff does everything in mm
    double l2base = nss->i->local2base;
    double base2l = nss->i->base2local;

    nss->i->local2base = 1.0;
    nss->i->base2local = 1.0;

    // High level options are only defined prior to the subcommand
    int cmd_pos = -1;
    for (int i = 0; i < argc; i++) {
	if (bu_cmd_valid(_nirt_diff_cmds, argv[i]) == BRLCAD_OK) {
	    cmd_pos = i;
	    break;
	}
    }

    int acnt = (cmd_pos >= 0) ? cmd_pos : argc;

    struct bu_vls optparse_msg = BU_VLS_INIT_ZERO;
    if (bu_opt_parse(&optparse_msg, acnt, argv, d) == -1) {
	nerr(nss, "%s", bu_vls_cstr(&optparse_msg));
	bu_vls_free(&optparse_msg);
	nss->i->local2base = l2base;
	nss->i->base2local = base2l;
	return -1;
    }
    bu_vls_free(&optparse_msg);

    if (help) {
	if (cmd_pos >= 0) {
	    argc = argc - cmd_pos;
	    argv = &argv[cmd_pos];
	    _nirt_diff_help(nds, argc, argv);
	} else {
	    _nirt_diff_help(nds, 0, NULL);
	}
	nss->i->local2base = l2base;
	nss->i->base2local = base2l;
	return 0;
    }

    // Must have a subcommand
    if (cmd_pos == -1) {
	nerr(nss, ": no valid subcommand specified\n");
	_nirt_diff_help(nds, 0, NULL);
	nss->i->local2base = l2base;
	nss->i->base2local = base2l;
	return -1;
    }

    argc = argc - cmd_pos;
    argv = &argv[cmd_pos];

    int ret;
    if (bu_cmd(_nirt_diff_cmds, argc, argv, 0, (void *)nds, &ret) == BRLCAD_OK) {
	nss->i->local2base = l2base;
	nss->i->base2local = base2l;
	return ret;
    }

    nss->i->local2base = l2base;
    nss->i->base2local = base2l;
    return -1;
}


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
