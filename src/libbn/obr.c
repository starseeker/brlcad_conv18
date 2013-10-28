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
 *
 * Copyright 2001 softSurfer, 2012 Dan Sunday
 * This code may be freely used and modified for any purpose
 * providing that this copyright notice is included with it.
 * SoftSurfer makes no warranty for this code, and cannot be held
 * liable for any real or imagined damage resulting from its use.
 * Users of this code must verify correctness for their application.
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
 * The implementation of Melkman's algorithm for convex hulls of simple
 * polylines is a translation of softSurfer's implementation:
 * http://geomalgorithms.com/a12-_hull-3.html
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

/* isLeft(): test if a point is Left|On|Right of an infinite line.
 *    Input:  three points L0, L1, and p
 *    Return: >0 for p left of the line through L0 and L1
 *            =0 for p on the line
 *            <0 for p right of the line
 */
#define isLeft(L0, L1, p) ((L1[X] - L0[X])*(p[Y] - L0[Y]) - (p[X] - L0[X])*(L1[Y] - L0[Y]))


/* melkman_hull(): Melkman's 2D simple polyline O(n) convex hull algorithm
 *    Input:  polyline[] = array of 2D vertex points for a simple polyline
 *            n   = the number of points in V[]
 *    Output: hull[] = output convex hull array of vertices in ccw orientation (max is n)
 *    Return: h   = the number of points in hull[]
 */
int
melkman_hull(const point_t* polyline, int n, point_t** hull)
{
    int i;

    /* initialize a deque D[] from bottom to top so that the
       1st three vertices of P[] are a ccw triangle */
    point_t* D = (point_t *)bu_calloc(2*n+1, sizeof(fastf_t)*3, "dequeue");

    /* hull vertex counter */
    int h;

    /* initial bottom and top deque indices */
    int bot = n-2;
    int top = bot+3;

    /* 3rd vertex is a both bot and top */
    VMOVE(D[top], polyline[2]);
    VMOVE(D[bot], D[top]);
    if (isLeft(polyline[0], polyline[1], polyline[2]) > 0) {
        VMOVE(D[bot+1],polyline[0]);
        VMOVE(D[bot+2],polyline[1]);   /* ccw vertices are: 2,0,1,2 */
    }
    else {
        VMOVE(D[bot+1],polyline[1]);
        VMOVE(D[bot+2],polyline[0]);   /* ccw vertices are: 2,1,0,2 */
    }

    /* compute the hull on the deque D[] */
    for (i = 3; i < n; i++) {   /* process the rest of vertices */
        /* test if next vertex is inside the deque hull */
        if ((isLeft(D[bot], D[bot+1], polyline[i]) > 0) &&
            (isLeft(D[top-1], D[top], polyline[i]) > 0) )
                 continue;         /* skip an interior vertex */

        /* incrementally add an exterior vertex to the deque hull
           get the rightmost tangent at the deque bot */
        while (isLeft(D[bot], D[bot+1], polyline[i]) <= 0)
            bot = bot + 1;                      /* remove bot of deque */
        VMOVE(D[bot-1],polyline[i]);    /* insert P[i] at bot of deque */
	bot = bot - 1;

        /* get the leftmost tangent at the deque top */
        while (isLeft(D[top-1], D[top], polyline[i]) <= 0)
            top = top - 1;                      /* pop top of deque */
        VMOVE(D[top+1],polyline[i]);    /* push P[i] onto top of deque */
	top = top + 1;
    }

    /* transcribe deque D[] to the output hull array hull[] */

    (*hull) = (point_t *)bu_calloc(top - bot + 2, sizeof(fastf_t)*3, "hull");
    for (h=0; h <= (top-bot); h++)
        VMOVE((*hull)[h],D[bot + h]);

    bu_free(D, "free queue");
    return h-1;
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
UpdateBox(struct obr_vals *obr, point_t LPoint, point_t RPoint, point_t BPoint, point_t TPoint, vect_t u, vect_t v)
{
    vect_t RLDiff, TBDiff, LBDiff;
    vect_t U, V;
    fastf_t extent0, extent1, area;
    VSUB2(RLDiff, RPoint, LPoint);
    VSUB2(TBDiff, TPoint, BPoint);
    extent0 = VDOT(u,RLDiff);
    extent1 = VDOT(v,TBDiff);
    area = extent0*extent1;
    if (area < obr->area)
    {
	obr->area = area;
	VMOVE(obr->u, u);
	VMOVE(obr->v, v);
	obr->extent0 = extent0;
	obr->extent1 = extent1;
	/* TODO translate this to vmath.h routines...
	mMinBox.Center = LPoint + U*extent0 + V*(extent1 - V.Dot(LBDiff));*/
	VSUB2(LBDiff, BPoint, LPoint);
	VSCALE(U, u, extent0);
	VSCALE(V, v, extent1 + VDOT(V,LBDiff));
	VADD3(obr->center, LPoint, U, V);
    }
}

/* Three consecutive colinear points will cause a problem (per a comment in the original code)
 * Consequently, we're going to have to build the convex hull for all inputs in order to
 * make sure we don't get colinear points from the NMG inputs.*/
int
bn_obr(const point_t *pnts, int pnt_cnt, point_t *p1, point_t *p2,
	point_t *p3, point_t *p4)
{
    int i = 0;
    int dim = 0;
    int done = 0;
    int hull_pnt_cnt = 0;
    point_t *hull_pnts = NULL;
    vect_t *edge_unit_vects = NULL;
    int *visited = NULL;
    point_t center, pmin, pmax;
    vect_t vline, vz, vdelta, neg_vdelta, neg_vline;
    struct obr_vals obr;
    VSET(vz, 0, 0, 1);
    if (!pnts || !p1 || !p2 || !p3 || !p4) return -1;
    /* Need 2d points for this to work */
    while (i < pnt_cnt) {
	if (!NEAR_ZERO(pnts[i][2], BN_TOL_DIST)) return -1;
	i++;
    }
    dim = pnt2d_array_get_dimension(pnts, pnt_cnt, &center, &pmin, &pmax);
    switch (dim) {
	case 0:
	    /* Bound point */
	    VSET(*p1, center[0] - BN_TOL_DIST, center[1] - BN_TOL_DIST, 0);
	    VSET(*p2, center[0] + BN_TOL_DIST, center[1] - BN_TOL_DIST, 0);
	    VSET(*p3, center[0] + BN_TOL_DIST, center[1] + BN_TOL_DIST, 0);
	    VSET(*p4, center[0] - BN_TOL_DIST, center[1] + BN_TOL_DIST, 0);
	    break;
	case 1:
	    /* Bound line */
	    VSUB2(vline, pmax, pmin);
	    VUNITIZE(vline);
	    VCROSS(vdelta, vline, vz);
	    VUNITIZE(vdelta);
	    VSCALE(vdelta, vdelta, BN_TOL_DIST);
	    VSET(neg_vdelta, -vdelta[0], -vdelta[1], 0);
	    VSET(neg_vline, -vline[0], -vline[1], 0);
	    VADD3(*p1, pmin, neg_vline, neg_vdelta);
	    VADD3(*p2, pmax, vline, neg_vdelta);
	    VADD3(*p3, pmax, vline, vdelta);
	    VADD3(*p4, pmin, neg_vline, vdelta);
	    break;
	case 2:
	    /* Bound convex hull using rotating calipers */

	    /* 1.  Get convex hull */
	    hull_pnt_cnt = melkman_hull(pnts, pnt_cnt, &hull_pnts);

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
	    obr.center[0] = 0.5 * (xmin + xmax);
	    obr.center[1] = 0.5 * (ymin + ymax);
	    obr.center[2] = 0.0;
	    VSET(obr.u, 1, 0, 0);
	    VSET(obr.v, 0, 1, 0);
	    obr.extent0 = (xmax - xmin);
	    obr.extent1 = (ymax - ymin);
	    obr.area = obr.extent0 * obr.extent1;

	    /* 3. The rotating calipers algorithm */
	    done = 0;
	    while (!done) {
		/* Determine the edge that forms the smallest angle with the current
		   box edges. */
		int flag = F_NONE;
		vect_t u, v;
		vect_t negu, negv;
		fastf_t maxDot = 0.0;
		fastf_t dot = 0.0;
		VSET(u, 1, 0, 0);
		VSET(v, 0, 1, 0);
		VSET(negu, -1, 0, 0);
		VSET(negv, 0, -1, 0);

		dot = VDOT(u,edge_unit_vects[BIndex]);
		if (dot > maxDot) {
		    maxDot = dot;
		    flag = F_BOTTOM;
		}

		dot = VDOT(v,edge_unit_vects[RIndex]);
		if (dot > maxDot) {
		    maxDot = dot;
		    flag = F_RIGHT;
		}

		dot = VDOT(negu, edge_unit_vects[TIndex]);
		if (dot > maxDot) {
		    maxDot = dot;
		    flag = F_TOP;
		}

		dot = VDOT(negv, edge_unit_vects[LIndex]);
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
			    VMOVE(u,edge_unit_vects[BIndex]);
			    VCROSS(v,u,vz);
			    VSCALE(v,v,-1);
			    UpdateBox(&obr, hull_pnts[LIndex], hull_pnts[RIndex],
				    hull_pnts[BIndex], hull_pnts[TIndex], u, v);

			    /* Mark edge visited and rotate the calipers. */
			    visited[BIndex] = 1;
			    if (++BIndex == hull_pnt_cnt) BIndex = 0;
			}
			break;
		    case F_RIGHT:
			if (visited[RIndex]) {
			    done = 1;
			} else {
			    /* Compute box axes with E[R] as an edge. */
			    VMOVE(v,edge_unit_vects[RIndex]);
			    VCROSS(u,v,vz);
			    UpdateBox(&obr, hull_pnts[LIndex], hull_pnts[RIndex],
				    hull_pnts[BIndex], hull_pnts[TIndex], u, v);

			    /* Mark edge visited and rotate the calipers. */
			    visited[RIndex] = 1;
			    if (++RIndex == hull_pnt_cnt) RIndex = 0;
			}
			break;
		    case F_TOP:
			if (visited[TIndex]) {
			    done = 1;
			} else {
			    /* Compute box axes with E[T] as an edge. */
			    VMOVE(u,edge_unit_vects[TIndex]);
			    VSCALE(u,u,-1);
			    VCROSS(v,u,vz);
			    VSCALE(v,v,-1);
			    UpdateBox(&obr, hull_pnts[LIndex], hull_pnts[RIndex],
				    hull_pnts[BIndex], hull_pnts[TIndex], u, v);

			    /* Mark edge visited and rotate the calipers. */
			    visited[TIndex] = 1;
			    if (++TIndex == hull_pnt_cnt) TIndex = 0;
			}
			break;
		    case F_LEFT:
			if (visited[LIndex]) {
			    done = 1;
			} else {
			    /* Compute box axes with E[L] as an edge. */
			    VMOVE(v,edge_unit_vects[LIndex]);
			    VSCALE(v,v,-1);
			    VCROSS(u,v,vz);
			    UpdateBox(&obr, hull_pnts[LIndex], hull_pnts[RIndex],
				    hull_pnts[BIndex], hull_pnts[TIndex], u, v);

			    /* Mark edge visited and rotate the calipers. */
			    visited[LIndex] = 1;
			    if (++LIndex == hull_pnt_cnt) LIndex = 0;
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

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */

