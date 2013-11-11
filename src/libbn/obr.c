/*                            O B R . C
 * BRL-CAD
 *
 * Copyright (c) 2013 United States Government as represented by
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

#include "bn.h"

#define F_NONE -1
#define F_BOTTOM 0
#define F_RIGHT 1
#define F_TOP 2
#define F_LEFT 3

HIDDEN int
pnt2d_array_get_dimension(const point_t *pnts, int pnt_cnt, point_t *p_center, point_t *p1, point_t *p2) {
    int i = 0;
    point_t min = VINIT_ZERO;
    point_t max = VINIT_ZERO;
    point_t center = VINIT_ZERO;
    point_t curr_pnt;
    point_t min_x_pt;
    point_t min_y_pt;
    point_t max_x_pt;
    point_t max_y_pt;
    point_t A;
    point_t B;
    fastf_t d[4];
    fastf_t dmax = 0.0;

    VMOVE(curr_pnt, pnts[0]);
    VMOVE(min_x_pt, curr_pnt);
    VMOVE(min_y_pt, curr_pnt);
    VMOVE(max_x_pt, curr_pnt);
    VMOVE(max_y_pt, curr_pnt);
    while (i < pnt_cnt) {
	VMOVE(curr_pnt,pnts[i]);
	center[0] += curr_pnt[0];
	center[1] += curr_pnt[1];
	VMINMAX(min, max, curr_pnt);
	if (min_x_pt[0] > curr_pnt[0]) VMOVE(min_x_pt, curr_pnt);
	if (min_y_pt[1] > curr_pnt[1]) VMOVE(min_y_pt, curr_pnt);
	if (max_x_pt[0] < curr_pnt[0]) VMOVE(max_x_pt, curr_pnt);
	if (max_y_pt[1] < curr_pnt[1]) VMOVE(max_y_pt, curr_pnt);
	i++;
    }
    center[0] = center[0] / i;
    center[1] = center[1] / i;
    VMOVE(*p_center, center);
    VMOVE(*p1, min);
    VMOVE(*p2, max);
    /* If the bbox is nearly a point, return 0 */
    if (NEAR_ZERO(max[0] - min[0], BN_TOL_DIST) && NEAR_ZERO(max[1] - min[1], BN_TOL_DIST)) return 0;

    /* Test if the point set is (nearly) a line */
    d[0] = DIST_PT_PT(min_x_pt, max_x_pt);
    d[1] = DIST_PT_PT(min_x_pt, max_y_pt);
    d[2] = DIST_PT_PT(min_y_pt, max_x_pt);
    d[3] = DIST_PT_PT(min_y_pt, max_y_pt);
    for (i = 0; i < 4; i++) {
	if (d[i] > dmax) {
	    dmax = d[i];
	    if (i == 1) {VMOVE(A,min_x_pt); VMOVE(B,max_x_pt);}
	    if (i == 2) {VMOVE(A,min_x_pt); VMOVE(B,max_y_pt);}
	    if (i == 3) {VMOVE(A,min_y_pt); VMOVE(B,max_x_pt);}
	    if (i == 4) {VMOVE(A,min_y_pt); VMOVE(B,max_y_pt);}
	}
    }
    i = 0;
    while (i < pnt_cnt) {
	const struct bn_tol tol = {BN_TOL_MAGIC, BN_TOL_DIST/2, BN_TOL_DIST*BN_TOL_DIST/4, 1e-6, 1-1e-6};
	fastf_t dist_sq;
	fastf_t pca[2];
	i++;
	VMOVE(curr_pnt,pnts[i]);
	/* If we're off the line, it's 2D. */
	if (bn_dist_pt2_lseg2(&dist_sq, pca, A, B, curr_pnt, &tol) > 2) return 2;
    }
    /* If we've got a line, make sure p1 and p2 are the extreme points */
    VMOVE(*p1, A);
    VMOVE(*p2, B);
    return 1;
}

struct obr_vals {
    fastf_t area;
    vect_t u;
    vect_t v;
    fastf_t extent0;
    fastf_t extent1;
    point_t center;
};

/* The oriented bounding rectangle must be calculated from the points and 2D vectors provided by
 * the rotating calipers, and the smallest resultant area tracked.  Those functions are handled
 * by this routine.*/
HIDDEN void
UpdateBox(struct obr_vals *obr, point_t LPoint, point_t RPoint, point_t BPoint, point_t TPoint, vect_t u)
{
    vect_t RLDiff, TBDiff, LBDiff;
    vect_t U, V, v, vz;
    fastf_t extent0, extent1, area;
    VSET(vz, 0, 0, -1);
    VCROSS(v, u, vz);

    VSUB2(RLDiff, RPoint, LPoint);
    VSUB2(TBDiff, TPoint, BPoint);
    extent0 = 0.5 * VDOT(u,RLDiff);
    extent1 = 0.5 * VDOT(v,TBDiff);
    area = extent0 * extent1 * 4;
    if (area < obr->area) {
	/*
	bu_log("LPoint: %f, %f, %f\n", LPoint[0], LPoint[1], LPoint[2]);
	bu_log("BPoint: %f, %f, %f\n", BPoint[0], BPoint[1], BPoint[2]);
	bu_log("RPoint: %f, %f, %f\n", RPoint[0], RPoint[1], RPoint[2]);
	bu_log("TPoint: %f, %f, %f\n", TPoint[0], TPoint[1], TPoint[2]);
	bu_log("u: %f, %f, %f\n", u[0], u[1], u[2]);
	bu_log("v: %f, %f, %f\n", v[0], v[1], v[2]);
	bu_log("RLDiff: %f, %f, %f\n", RLDiff[0], RLDiff[1], RLDiff[2]);
	bu_log("TBDiff: %f, %f, %f\n", TBDiff[0], TBDiff[1], TBDiff[2]);
	bu_log("extent0: %f\n", extent0);
	bu_log("extent1: %f\n", extent1);
	bu_log("area: %f\n", area);
*/
	obr->area = area;
	VMOVE(obr->u, u);
	obr->extent0 = extent0;
	obr->extent1 = extent1;
	/* TODO translate this to vmath.h routines...
	mMinBox.Center = LPoint + U*extent0 + V*(extent1 - V.Dot(LBDiff));*/
	VSUB2(LBDiff, LPoint, BPoint);
	/*bu_log("LBDiff: %f, %f, %f\n", LBDiff[0], LBDiff[1], LBDiff[2]);*/
	VSCALE(U, u, extent0);
	/*bu_log("U: %f, %f, %f\n", U[0], U[1], U[2]);*/
	/*bu_log("VDOT(v, LBDiff): %f\n", VDOT(v,LBDiff));*/
	VSCALE(V, v, extent1 - VDOT(v,LBDiff));
	/*bu_log("V: %f, %f, %f\n", V[0], V[1], V[2]);*/
	VADD3(obr->center, LPoint, U, V);
	bu_log("center: %f, %f, %f\n\n", obr->center[0], obr->center[1], obr->center[2]);
	VSCALE(V, v, extent1);
    }
}

/* Three consecutive colinear points will cause a problem (per a comment in the original code)
 * Consequently, we're going to have to build the convex hull for all inputs in order to
 * make sure we don't get colinear points from the NMG inputs.*/
HIDDEN int
bn_obr_calc(const point_t *pnts, int pnt_cnt, struct obr_vals *obr)
{
    int i = 0;
    int dim = 0;
    int done = 0;
    int hull_pnt_cnt = 0;
    point_t *hull_pnts = NULL;
    vect_t *edge_unit_vects = NULL;
    int *visited = NULL;
    point_t center, pmin, pmax;
    vect_t u;
    vect_t vline, vz, vdelta;
    VSET(vz, 0, 0, 1);
    dim = pnt2d_array_get_dimension(pnts, pnt_cnt, &center, &pmin, &pmax);
    switch (dim) {
	case 0:
	    /* Bound point */
	    VSET(obr->center, center[0] - BN_TOL_DIST, center[1] - BN_TOL_DIST, 0);
	    VSET(obr->u, 1 , 0, 0);
	    VSET(obr->v, 0, 1, 0);
	    obr->extent0 = BN_TOL_DIST;
	    obr->extent1 = BN_TOL_DIST;
	    break;
	case 1:
	    /* Bound line */
	    VSUB2(vline, pmax, pmin);
	    obr->extent0 = MAGNITUDE(vline) * 0.5;
	    obr->extent1 = BN_TOL_DIST;
	    VSET(obr->center, vline[0]/2, vline[1]/2, vline[2]/2);
	    VSUB2(vline, pmax, obr->center);
	    VUNITIZE(vline);
	    VMOVE(obr->u,vline);
	    VCROSS(vdelta, vline, vz);
	    VUNITIZE(vdelta);
	    VMOVE(obr->v,vdelta);
	    break;
	case 2:
	    /* Bound convex hull using rotating calipers */

	    /* 1.  Get convex hull */
	    hull_pnt_cnt = bn_polyline_2D_hull(&hull_pnts, pnts, pnt_cnt);

	    /* 2.  Get edge unit vectors */
	    edge_unit_vects = (vect_t *)bu_calloc(hull_pnt_cnt + 1, sizeof(fastf_t) * 3, "unit vects for edges");
	    visited = (int *)bu_calloc(hull_pnt_cnt + 1, sizeof(int), "visited flags");
	    for (i = 0; i < hull_pnt_cnt - 1; ++i) {
		VSUB2(edge_unit_vects[i], hull_pnts[i + 1], hull_pnts[i]);
		VUNITIZE(edge_unit_vects[i]);
	    }
	    VSUB2(edge_unit_vects[hull_pnt_cnt - 1], hull_pnts[0], hull_pnts[hull_pnt_cnt - 1]);
	    VUNITIZE(edge_unit_vects[hull_pnt_cnt - 1]);

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
	    obr->center[2] = 0.0;
	    VSET(obr->u, 1, 0, 0);
	    obr->extent0 = (xmax - xmin);
	    obr->extent1 = (ymax - ymin);
	    obr->area = obr->extent0 * obr->extent1;

	    /* 3. The rotating calipers algorithm */
	    done = 0;
	    VSET(u, 1, 0, 0);

	    while (!done) {
		/* Determine the edge that forms the smallest angle with the current
		   box edges. */
		int flag = F_NONE;
		fastf_t maxDot = 0.0;
		fastf_t dot = 0.0;
		vect_t t, tmp;
		VMOVE(t, u);


		dot = VDOT(t,edge_unit_vects[BIndex]);
		/*bu_log("t: %f, %f\n", t[0], t[1]);
		bu_log("edge_unit_vects[%d]: %f, %f\n", BIndex, edge_unit_vects[BIndex][0], edge_unit_vects[BIndex][1]);
		bu_log("b_dot: %f\n", dot);*/
		if (dot > maxDot) {
		    maxDot = dot;
		    flag = F_BOTTOM;
		}

		VSET(t, -u[1], u[0], 0);
		dot = VDOT(t,edge_unit_vects[RIndex]);
		/*bu_log("t: %f, %f\n", t[0], t[1]);
		bu_log("edge_unit_vects[%d]: %f, %f\n", BIndex, edge_unit_vects[RIndex][0], edge_unit_vects[RIndex][1]);
		bu_log("r_dot: %f\n", dot);*/
		if (dot > maxDot) {
		    maxDot = dot;
		    flag = F_RIGHT;
		}

		VSET(tmp, -t[1], t[0], 0);
		VMOVE(t, tmp);
		dot = VDOT(t, edge_unit_vects[TIndex]);
		/*bu_log("t: %f, %f\n", t[0], t[1]);
		bu_log("edge_unit_vects[%d]: %f, %f\n", BIndex, edge_unit_vects[TIndex][0], edge_unit_vects[TIndex][1]);
		bu_log("t_dot: %f\n", dot);*/
		if (dot > maxDot) {
		    maxDot = dot;
		    flag = F_TOP;
		}

		VSET(tmp, -t[1], t[0], 0);
		VMOVE(t, tmp);
		dot = VDOT(t, edge_unit_vects[LIndex]);
		/*bu_log("t: %f, %f\n", t[0], t[1]);
		bu_log("edge_unit_vects[%d]: %f, %f\n", BIndex, edge_unit_vects[LIndex][0], edge_unit_vects[LIndex][1]);
		bu_log("l_dot: %f\n", dot);*/
		if (dot > maxDot) {
		    maxDot = dot;
		    flag = F_LEFT;
		}

		switch (flag) {
		    case F_BOTTOM:
			bu_log("F_BOTTOM: %d\n", BIndex);
			if (visited[BIndex]) {
			    done = 1;
			} else {
			    /* Compute box axes with E[B] as an edge.*/
			    VMOVE(t,edge_unit_vects[BIndex]);
			    VMOVE(u, t);
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
			bu_log("F_RIGHT: %d\n", RIndex);
			if (visited[RIndex]) {
			    done = 1;
			} else {
			    /* Compute box axes with E[R] as an edge. */
			    VMOVE(t,edge_unit_vects[RIndex]);
			    VSET(u, t[1], -t[0], 0);
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
			bu_log("F_TOP: %d\n", TIndex);
			if (visited[TIndex]) {
			    done = 1;
			} else {
			    /* Compute box axes with E[T] as an edge. */
			    VMOVE(t,edge_unit_vects[TIndex]);
			    VSCALE(t,t,-1);
			    VMOVE(u,t);
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
			bu_log("F_LEFT: %d\n", LIndex);
			if (visited[LIndex]) {
			    done = 1;
			} else {
			    /* Compute box axes with E[L] as an edge. */
			    VMOVE(t,edge_unit_vects[LIndex]);
			    VSET(u, -t[1], t[0], 0);
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
bn_obr(const point_t *pnts, int pnt_cnt, point_t *center, vect_t *x, vect_t *y){
    struct obr_vals obr;
    vect_t a1;
    int i = 0;
    int points_2d = 1;
    /* var not being used at the moment
    point_t *pnts2d = NULL;
    */
    VSET(obr.v, 0.0, 0.0, 0.0);

    if (!pnts) return -1;

    /* See if we already have 2D points */
    i = 0;
    while (i < pnt_cnt) {
	if (!NEAR_ZERO(pnts[i][2], BN_TOL_DIST)) points_2d = 0;
	i++;
    }
    if (!points_2d) {
	/* Test for coplanar 3D pnts */
	point_t center_pnt, dmax;
	fastf_t dist = 0.0;
	fastf_t dist2 = 0.0;
	fastf_t curr_dist = 0.0;
	/*plane_t plane;*/
	VSET(dmax, 0, 0, 0);
	VSET(center_pnt, 0.0, 0.0, 0.0);
	for (i = 0; i < pnt_cnt; i++) {
	    VADD2(center_pnt, center_pnt, pnts[i]);
	}
	VSCALE(center_pnt, center_pnt, 1/pnt_cnt);
	for (i = 0; i < pnt_cnt; i++) {
	    curr_dist = DIST_PT_PT(center_pnt, pnts[i]);
	    if (curr_dist > dist) {
		VMOVE(dmax, pnts[i]);
		dist = curr_dist;
	    } else {
		if (curr_dist > dist2) {
		    /* VMOVE(dmax2, pnts[i]); */
		    dist2 = curr_dist;
		}
	    }
	}

	/*
	if (bn_mk_plane_3pts(center_pnt, dmax, dmax2, )) return -1;
	for (i = 0; i < pnt_cnt; i++) {
	    IF (DIST_PT_PLANE(PT, PLANE) > ) RETURN -1;
	}*/

	/* If we've gotten this far, we're coplanar - prepare a
	 * 2D point array by projecting points onto the plane */
	VSUB2(a1, dmax, center_pnt);
	VUNITIZE(a1);
	/*
	pnts2d = bu_malloc();
	for (i = 0; i < pnt_cnt; i++) {
	    vect_t tmp;
	    VSUB2(tmp, pnts[i], center_pnt);
	    VPROJECT(tmp,a1,v1,u1);
	    VSET(pnts2d[i],MAG(u1),MAG(v1),0);
	}
	*/

    } else {
	/* Don't need a new array - just re-use existing one */
    /* var not being used at the moment
	pnts2d = pnts;
    */
    }

    bn_obr_calc(pnts, pnt_cnt, &obr);

    if (!points_2d) {
    } else {
	/* If we started with 2D, we can just report the results */
	VMOVE((*center), obr.center);
	VMOVE((*x), obr.u);
	VUNITIZE((*x));
	VSCALE((*x), (*x), obr.extent0);
	VMOVE((*y), obr.v);
	VUNITIZE((*y));
	VSCALE((*y), (*y), obr.extent1);
    }

    /* pnts2d is no longer heap allocated
    if (!points_2d)
	bu_free(pnts2d, "free array holding 2D projections of coplanar points");
    */

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

