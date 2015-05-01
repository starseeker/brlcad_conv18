/*                            O B R . C
 * BRL-CAD
 *
 * Copyright (c) 2013-2014 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * Geometric Tools, LLC
 * Copyright (c) 1998-2013

 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
/** @file obr.c
 *
 * This file implements the Rotating Calipers algorithm for finding the
 * minimum oriented bounding rectangle for a convex hull:
 *
 * Shamos, Michael, "Analysis of the complexity of fundamental geometric
 *    algorithms such as area computation, sweep-line algorithms, polygon
 *    intersection, Voronoi diagram construction, and minimum spanning tree."
 *    Phd Thesis, Yale, 1978.
 *
 * Godfried T. Toussaint, "Solving geometric problems with the rotating
 * calipers," Proceedings of IEEE MELECON'83, Athens, Greece, May 1983.
 *
 * This is a translation of the Geometric Tools MinBox2 implementation:
 * http://www.geometrictools.com/LibMathematics/Containment/Wm5ContMinBox2.cpp
 * http://www.geometrictools.com/LibMathematics/Algebra/Wm5Vector2.inl
 *
 */


#include "common.h"
#include <stdlib.h>

#include "bu/malloc.h"
#include "bn/tol.h"
#include "bn/plane_calc.h"
#include "bgeom/chull.h"
#include "bgeom/obr.h"
#include "./bgeom_private.h"

#define F_NONE -1
#define F_BOTTOM 0
#define F_RIGHT 1
#define F_TOP 2
#define F_LEFT 3

HIDDEN int
pnt2d_array_get_dimension(const point2d_t *pnts, int pnt_cnt, point2d_t *p_center, point2d_t *p1, point2d_t *p2)
{
    int i = 0;
    point2d_t min = V2INIT_ZERO;
    point2d_t max = V2INIT_ZERO;
    point2d_t center = V2INIT_ZERO;
    point2d_t curr_pnt = V2INIT_ZERO;
    point2d_t min_x_pt = V2INIT_ZERO;
    point2d_t min_y_pt = V2INIT_ZERO;
    point2d_t max_x_pt = V2INIT_ZERO;
    point2d_t max_y_pt = V2INIT_ZERO;
    point2d_t A = V2INIT_ZERO;
    point2d_t B = V2INIT_ZERO;
    fastf_t dmax, dist_minmax;

    V2MOVE(curr_pnt, pnts[0]);
    V2MOVE(min_x_pt, curr_pnt);
    V2MOVE(min_y_pt, curr_pnt);
    V2MOVE(max_x_pt, curr_pnt);
    V2MOVE(max_y_pt, curr_pnt);

    while (i < pnt_cnt) {
	V2MOVE(curr_pnt, pnts[i]);
	center[0] += curr_pnt[0];
	center[1] += curr_pnt[1];
	V2MINMAX(min, max, curr_pnt);
	if (min_x_pt[0] > curr_pnt[0]) V2MOVE(min_x_pt, curr_pnt);
	if (min_y_pt[1] > curr_pnt[1]) V2MOVE(min_y_pt, curr_pnt);
	if (max_x_pt[0] < curr_pnt[0]) V2MOVE(max_x_pt, curr_pnt);
	if (max_y_pt[1] < curr_pnt[1]) V2MOVE(max_y_pt, curr_pnt);
	i++;
    }
    center[0] = center[0] / i;
    center[1] = center[1] / i;
    V2MOVE(*p_center, center);
    V2MOVE(*p1, min);
    V2MOVE(*p2, max);
    /* If the bbox is nearly a point, return 0 */
    if (NEAR_ZERO(max[0] - min[0], BN_TOL_DIST) && NEAR_ZERO(max[1] - min[1], BN_TOL_DIST)) return 0;

    /* Test if the point set is (nearly) a line */

    /* find the min A and max B with the greatest distance between them */
    dmax = 0.0;
    dist_minmax = DIST_PT2_PT2(min_x_pt, max_x_pt);
    if (dist_minmax > dmax) {
	V2MOVE(A, min_x_pt);
	V2MOVE(B, min_x_pt);
    }
#define MAXDIST_AB(_minpt, _maxpt) { \
    fastf_t md_dist = DIST_PT2_PT2(_minpt, _maxpt); \
    if (md_dist > dmax) { \
	dmax = md_dist; \
	V2MOVE(A, _minpt); \
	V2MOVE(B, _maxpt); \
    } \
}
    MAXDIST_AB(min_x_pt, max_x_pt);
    MAXDIST_AB(min_x_pt, max_y_pt);
    MAXDIST_AB(min_y_pt, max_x_pt);
    MAXDIST_AB(min_y_pt, max_y_pt);

    i = 0;
    while (i < pnt_cnt) {
	const struct bn_tol tol = {BN_TOL_MAGIC, BN_TOL_DIST / 2, BN_TOL_DIST *BN_TOL_DIST / 4, 1e-6, 1 - 1e-6};
	fastf_t dist_sq;
	fastf_t pca[2];
	point_t A_3D, B_3D, curr_pnt_3D;
	i++;
	VSET(A_3D, A[0], A[1], 0.0);
	VSET(B_3D, B[0], B[1], 0.0);
	V2MOVE(curr_pnt, pnts[i]);
	VSET(curr_pnt_3D, curr_pnt[0], curr_pnt[1], 0.0);
	/* If we're off the line, it's 2D. */
	if (bn_dist_pt2_lseg2(&dist_sq, pca, A_3D, B_3D, curr_pnt_3D, &tol) > 2) return 2;
    }
    /* If we've got a line, make sure p1 and p2 are the extreme points */
    V2MOVE(*p1, A);
    V2MOVE(*p2, B);
    return 1;
}

struct obr_vals {
    fastf_t area;
    vect2d_t u;
    vect2d_t v;
    fastf_t extent0;
    fastf_t extent1;
    point2d_t center;
};

/* The oriented bounding rectangle must be calculated from the points and 2D vectors provided by
 * the rotating calipers, and the smallest resultant area tracked.  Those functions are handled
 * by this routine.*/
HIDDEN void
UpdateBox(struct obr_vals *obr, point2d_t left_pnt, point2d_t right_pnt, point2d_t bottom_pnt,
	  point2d_t top_pnt, vect2d_t u)
{
    vect2d_t right_left_diff, top_bottom_diff, left_bottom_diff;
    vect2d_t U, V, v;
    fastf_t extent0, extent1, area;
    V2SET(v, -u[1], u[0]);

    V2SUB2(right_left_diff, right_pnt, left_pnt);
    V2SUB2(top_bottom_diff, top_pnt, bottom_pnt);
    extent0 = 0.5 * V2DOT(u, right_left_diff);
    extent1 = 0.5 * V2DOT(v, top_bottom_diff);
    area = extent0 * extent1 * 4;

    if (area < obr->area) {
	obr->area = area;
	V2MOVE(obr->u, u);
	V2MOVE(obr->v, v);
	obr->extent0 = extent0;
	obr->extent1 = extent1;
	/* Note: translated the following to vmath.h routines...
	mMinBox.Center = left_pnt + U*extent0 + V*(extent1 - V.Dot(left_bottom_diff));*/
	V2SUB2(left_bottom_diff, left_pnt, bottom_pnt);
	V2SCALE(U, u, extent0);
	V2SCALE(V, v, extent1 - V2DOT(v, left_bottom_diff));
	V2ADD3(obr->center, left_pnt, U, V);
	V2SCALE(V, v, extent1);
    }
}

/* Three consecutive collinear points will cause a problem (as per a comment in the original code)
 * Consequently, we're going to have to build the convex hull for all non-trivial inputs in order to
 * make sure we don't get collinear points from the NMG inputs.*/
HIDDEN int
bgeom_obr_calc(const point2d_t *pnts, int pnt_cnt, struct obr_vals *obr)
{
    int i = 0;
    int dim = 0;
    int done = 0;
    int hull_pnt_cnt = 0;
    point2d_t *hull_pnts = NULL;
    vect2d_t *edge_unit_vects = NULL;
    int *visited = NULL;
    point2d_t center, pmin, pmax;
    vect2d_t u;
    vect2d_t vline;
    dim = pnt2d_array_get_dimension(pnts, pnt_cnt, &center, &pmin, &pmax);
    switch (dim) {
	case 0:
	    /* Bound point */
	    V2SET(obr->center, center[0] - BN_TOL_DIST, center[1] - BN_TOL_DIST);
	    V2SET(obr->u, 1 , 0);
	    V2SET(obr->v, 0, 1);
	    obr->extent0 = BN_TOL_DIST;
	    obr->extent1 = BN_TOL_DIST;
	    break;
	case 1:
	    /* Bound line */
	    V2SUB2(vline, pmax, pmin);
	    obr->extent0 = MAGNITUDE2(vline) * 0.5;
	    obr->extent1 = BN_TOL_DIST;
	    V2SET(obr->center, center[0] / 2, center[1] / 2);
	    V2SUB2(vline, pmax, center);
	    V2UNITIZE(vline);
	    V2SET(obr->u, vline[0], vline[1]);
	    V2SET(obr->v, -vline[1], vline[0]);
	    V2UNITIZE(obr->v);
	    break;
	case 2:
	    /* Bound convex hull using rotating calipers */

	    /* 1.  Get convex hull */
	    hull_pnt_cnt = bgeom_2d_chull(&hull_pnts, pnts, pnt_cnt);

	    /* 2.  Get edge unit vectors */
	    edge_unit_vects = (vect2d_t *)bu_calloc(hull_pnt_cnt + 1, sizeof(fastf_t) * 3, "unit vects for edges");
	    visited = (int *)bu_calloc(hull_pnt_cnt + 1, sizeof(int), "visited flags");
	    for (i = 0; i < hull_pnt_cnt - 1; ++i) {
		V2SUB2(edge_unit_vects[i], hull_pnts[i + 1], hull_pnts[i]);
		V2UNITIZE(edge_unit_vects[i]);
	    }
	    V2SUB2(edge_unit_vects[hull_pnt_cnt - 1], hull_pnts[0], hull_pnts[hull_pnt_cnt - 1]);
	    V2UNITIZE(edge_unit_vects[hull_pnt_cnt - 1]);

	    /* 3. Find the points involved with the AABB */
	    /* Find the smallest axis-aligned box containing the points.  Keep track */
	    /* of the extremum indices, L (left), R (right), B (bottom), and T (top) */
	    /* so that the following constraints are met:                            */
	    /*   V[L][0] <= V[i][0] for all i and V[(L+1)%N][0] > V[L][0]        */
	    /*   V[R][0] >= V[i][0] for all i and V[(R+1)%N][0] < V[R][0]        */
	    /*   V[B][1] <= V[i][1] for all i and V[(B+1)%N][1] > V[B][1]        */
	    /*   V[T][1] >= V[i][1] for all i and V[(T+1)%N][1] < V[T][1]        */
	    {
	    fastf_t xmin = hull_pnts[0][0];
	    fastf_t xmax = xmin;
	    fastf_t ymin = hull_pnts[0][1];
	    fastf_t ymax = ymin;
	    int LIndex = 0;
	    int RIndex = 0;
	    int BIndex = 0;
	    int TIndex = 0;
	    for (i = 1; i < hull_pnt_cnt; ++i) {
		if (hull_pnts[i][0] <= xmin) {
		    xmin = hull_pnts[i][0];
		    LIndex = i;
		}
		if (hull_pnts[i][0] >= xmax) {
		    xmax = hull_pnts[i][0];
		    RIndex = i;
		}

		if (hull_pnts[i][1] <= ymin) {
		    ymin = hull_pnts[i][1];
		    BIndex = i;
		}
		if (hull_pnts[i][1] >= ymax) {
		    ymax = hull_pnts[i][1];
		    TIndex = i;
		}
	    }

	    /* Apply wrap-around tests to ensure the constraints mentioned above are satisfied.*/
	    if ((LIndex == (hull_pnt_cnt - 1)) && (hull_pnts[0][0] <= xmin)) {
		xmin = hull_pnts[0][0];
		LIndex = 0;
	    }

	    if ((RIndex == (hull_pnt_cnt - 1)) && (hull_pnts[0][0] >= xmax)) {
		xmax = hull_pnts[0][0];
		RIndex = 0;
	    }

	    if ((BIndex == (hull_pnt_cnt - 1)) && (hull_pnts[0][1] <= ymin)) {
		ymin = hull_pnts[0][1];
		BIndex = 0;
	    }

	    if ((TIndex == (hull_pnt_cnt - 1)) && (hull_pnts[0][1] >= ymax)) {
		ymax = hull_pnts[0][1];
		TIndex = 0;
	    }

	    /* initialize with AABB */
	    obr->center[0] = 0.5 * (xmin + xmax);
	    obr->center[1] = 0.5 * (ymin + ymax);
	    V2SET(obr->u, obr->center[0], 0);
	    V2UNITIZE(obr->u);
	    V2SET(obr->v, -obr->u[1], obr->u[0]);
	    V2UNITIZE(obr->v);
	    obr->extent0 = 0.5 * (xmax - xmin);
	    obr->extent1 = 0.5 * (ymax - ymin);
	    obr->area = obr->extent0 * obr->extent1 * 4;

	    /* 3. The rotating calipers algorithm */
	    done = 0;
	    V2SET(u, 1, 0);

	    while (!done) {
		/* Determine the edge that forms the smallest angle with the current
		   box edges. */
		int flag = F_NONE;
		fastf_t maxDot = 0.0;
		fastf_t dot = 0.0;
		vect2d_t t, tmp;
		V2MOVE(t, u);


		dot = V2DOT(t, edge_unit_vects[BIndex]);
		/*bu_log("t: %f, %f\n", t[0], t[1]);
		bu_log("edge_unit_vects[%d]: %f, %f\n", BIndex, edge_unit_vects[BIndex][0], edge_unit_vects[BIndex][1]);
		bu_log("b_dot: %f\n", dot);*/
		if (dot > maxDot) {
		    maxDot = dot;
		    flag = F_BOTTOM;
		}

		V2SET(t, -u[1], u[0]);
		dot = V2DOT(t, edge_unit_vects[RIndex]);
		/*bu_log("t: %f, %f\n", t[0], t[1]);
		bu_log("edge_unit_vects[%d]: %f, %f\n", BIndex, edge_unit_vects[RIndex][0], edge_unit_vects[RIndex][1]);
		bu_log("r_dot: %f\n", dot);*/
		if (dot > maxDot) {
		    maxDot = dot;
		    flag = F_RIGHT;
		}

		V2SET(tmp, -t[1], t[0]);
		V2MOVE(t, tmp);
		dot = V2DOT(t, edge_unit_vects[TIndex]);
		/*bu_log("t: %f, %f\n", t[0], t[1]);
		bu_log("edge_unit_vects[%d]: %f, %f\n", BIndex, edge_unit_vects[TIndex][0], edge_unit_vects[TIndex][1]);
		bu_log("t_dot: %f\n", dot);*/
		if (dot > maxDot) {
		    maxDot = dot;
		    flag = F_TOP;
		}

		V2SET(tmp, -t[1], t[0]);
		V2MOVE(t, tmp);
		dot = V2DOT(t, edge_unit_vects[LIndex]);
		/*bu_log("t: %f, %f\n", t[0], t[1]);
		bu_log("edge_unit_vects[%d]: %f, %f\n", BIndex, edge_unit_vects[LIndex][0], edge_unit_vects[LIndex][1]);
		bu_log("l_dot: %f\n", dot);*/
		if (dot > maxDot) {
		    maxDot = dot;
		    flag = F_LEFT;
		}

		switch (flag) {
		    case F_BOTTOM:
			if (visited[BIndex]) {
			    done = 1;
			} else {
			    /* Compute box axes with E[B] as an edge.*/
			    V2MOVE(t, edge_unit_vects[BIndex]);
			    V2MOVE(u, t);
			    UpdateBox(obr, hull_pnts[LIndex], hull_pnts[RIndex],
				      hull_pnts[BIndex], hull_pnts[TIndex], u);

			    /* Mark edge visited and rotate the calipers. */
			    visited[BIndex] = 1;
			    if ((BIndex + 1) == hull_pnt_cnt) {
				BIndex = 0;
			    } else {
				BIndex = BIndex + 1;
			    }
			}
			break;
		    case F_RIGHT:
			if (visited[RIndex]) {
			    done = 1;
			} else {
			    /* Compute box axes with E[R] as an edge. */
			    V2MOVE(t, edge_unit_vects[RIndex]);
			    V2SET(u, t[1], -t[0]);
			    UpdateBox(obr, hull_pnts[LIndex], hull_pnts[RIndex],
				      hull_pnts[BIndex], hull_pnts[TIndex], u);

			    /* Mark edge visited and rotate the calipers. */
			    visited[RIndex] = 1;
			    if ((RIndex + 1) == hull_pnt_cnt) {
				RIndex = 0;
			    } else {
				RIndex = RIndex + 1;
			    }
			}
			break;
		    case F_TOP:
			if (visited[TIndex]) {
			    done = 1;
			} else {
			    /* Compute box axes with E[T] as an edge. */
			    V2MOVE(t, edge_unit_vects[TIndex]);
			    V2SCALE(t, t, -1);
			    V2MOVE(u, t);
			    UpdateBox(obr, hull_pnts[LIndex], hull_pnts[RIndex],
				      hull_pnts[BIndex], hull_pnts[TIndex], u);

			    /* Mark edge visited and rotate the calipers. */
			    visited[TIndex] = 1;
			    if ((TIndex + 1) == hull_pnt_cnt) {
				TIndex = 0;
			    } else {
				TIndex = TIndex + 1;
			    }

			}
			break;
		    case F_LEFT:
			if (visited[LIndex]) {
			    done = 1;
			} else {
			    /* Compute box axes with E[L] as an edge. */
			    V2MOVE(t, edge_unit_vects[LIndex]);
			    V2SET(u, -t[1], t[0]);
			    UpdateBox(obr, hull_pnts[LIndex], hull_pnts[RIndex],
				      hull_pnts[BIndex], hull_pnts[TIndex], u);

			    /* Mark edge visited and rotate the calipers. */
			    visited[LIndex] = 1;
			    if ((LIndex + 1) == hull_pnt_cnt) {
				LIndex = 0;
			    } else {
				LIndex = LIndex + 1;
			    }

			}
			break;
		    case F_NONE:
			/* The polygon is a rectangle. */
			done = 1;
			break;
		}
	    }
	    bu_free(visited, "free visited");
	    bu_free(edge_unit_vects, "free visited");
	    bu_free(hull_pnts, "free visited");
	    }
    }
    return dim;
}

int
bgeom_2d_obr(point2d_t *center, vect2d_t *u, vect2d_t *v, const point2d_t *pnts, int pnt_cnt)
{
    struct obr_vals obr;
    V2SET(obr.v, 0.0, 0.0);

    V2SET(*center, 0.0, 0.0);
    V2SET(*u, 0.0, 0.0);
    V2SET(*v, 0.0, 0.0);

    if (!pnts) return -1;
    if (pnt_cnt <= 0) return -1;

    (void)bgeom_obr_calc(pnts, pnt_cnt, &obr);

    V2SET(*center, obr.center[0], obr.center[1]);
    V2SET(*u, obr.u[0], obr.u[1]);
    V2UNITIZE(*u);
    V2SCALE(*u, *u, obr.extent0);
    V2SET(*v, obr.v[0], obr.v[1]);
    V2UNITIZE(*v);
    V2SCALE(*v, *v, obr.extent1);

    return 0;
}

int
bgeom_3d_coplanar_obr(point_t *center, vect_t *v1, vect_t *v2, const point_t *pnts, int pnt_cnt)
{
    int ret = 0;
    point_t origin_pnt;
    vect_t u_axis, v_axis;
    point2d_t obr_2d_center;
    point2d_t obr_2d_v1, obr_2d_v2;
    point2d_t *points_obr = NULL;
    point_t *points_obr_3d = NULL;
    point2d_t *points_tmp = NULL;
    point_t tmp3d;
    const point2d_t *const_points_tmp;

    if (!pnts || pnt_cnt <= 0) return -1;

    points_obr = (point2d_t *)bu_calloc(3 + 1, sizeof(point2d_t), "points_2d");
    points_obr_3d = (point_t *)bu_calloc(3 + 1, sizeof(point_t), "points_3d");
    points_tmp = (point2d_t *)bu_calloc(pnt_cnt + 1, sizeof(point2d_t), "points_2d");

    ret += coplanar_2d_coord_sys(&origin_pnt, &u_axis, &v_axis, pnts, pnt_cnt);
    ret += coplanar_3d_to_2d(&points_tmp, (const point_t *)&origin_pnt, (const vect_t *)&u_axis, (const vect_t *)&v_axis, pnts, pnt_cnt);
    if (ret)
	return 0;

    const_points_tmp = (const point2d_t *)points_tmp;
    ret = bgeom_2d_obr(&obr_2d_center, &obr_2d_v1, &obr_2d_v2, const_points_tmp, pnt_cnt);

    /* Set up the 2D point list so converting it will result in useful 3D points */
    V2MOVE(points_obr[0], obr_2d_center);
    V2ADD2(points_obr[1], obr_2d_v1, obr_2d_center);
    V2ADD2(points_obr[2], obr_2d_v2, obr_2d_center);

    ret = coplanar_2d_to_3d(&points_obr_3d, (const point_t *)&origin_pnt, (const vect_t *)&u_axis, (const vect_t *)&v_axis, (const point2d_t *)points_obr, 3);

    VMOVE(*center, points_obr_3d[0]);

    /* Want to be able to add v1 and v2 to the 3D center to get obr points,
     * so subtract the center out up front */
    VSUB2(tmp3d, points_obr_3d[1], *center);
    VMOVE(*v1, tmp3d);
    VSUB2(tmp3d, points_obr_3d[2], *center);
    VMOVE(*v2, tmp3d);

    bu_free(points_obr, "free 2d obr pnts");
    bu_free(points_obr_3d, "free 3d obr pnts");
    bu_free(points_tmp, "free 2d pnt projections");

    return 0;
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
