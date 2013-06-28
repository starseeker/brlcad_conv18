/*                  I N T E R S E C T . C P P
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
/** @file intersect.cpp
 *
 * Implementation of intersection routines openNURBS left out.
 *
 */

#include "common.h"

#include <assert.h>
#include <vector>
#include <algorithm>
#include "bio.h"

#include "vmath.h"
#include "bu.h"

#include "brep.h"


/* Sub-division support for a curve.
 * It's similar to generating the bounding box tree, when the Split()
 * method is called, the curve is splitted into two parts, whose bounding
 * boxes become the children of the original curve's bbox.
 */
struct Subcurve {
private:
    ON_BoundingBox m_node;
public:
    ON_Curve *m_curve;
    ON_Interval m_t;
    Subcurve *m_children[2];
    ON_BOOL32 m_islinear;

    Subcurve() : m_curve(NULL), m_islinear(false)
    {
	m_children[0] = m_children[1] = NULL;
    }
    Subcurve(const Subcurve &_scurve)
    {
	m_islinear = _scurve.m_islinear;
	m_curve = _scurve.m_curve->Duplicate();
	m_t = _scurve.m_t;
	m_children[0] = m_children[1] = NULL;
	SetBBox(_scurve.m_node);
    }
    ~Subcurve()
    {
	for (int i = 0; i < 2; i++) {
	    if (m_children[i])
		delete m_children[i];
	}
	delete m_curve;
    }
    int Split()
    {
	for (int i = 0; i < 2; i++)
	    m_children[i] = new Subcurve();

	if (m_curve->Split(m_t.Mid(), m_children[0]->m_curve, m_children[1]->m_curve) == false)
	    return -1;

	m_children[0]->m_t = ON_Interval(m_t.Min(), m_t.Mid());
	m_children[0]->SetBBox(m_children[0]->m_curve->BoundingBox());
	m_children[0]->m_islinear = m_children[0]->m_curve->IsLinear();
	m_children[1]->m_t = ON_Interval(m_t.Mid(), m_t.Max());
	m_children[1]->SetBBox(m_children[1]->m_curve->BoundingBox());
	m_children[1]->m_islinear = m_children[1]->m_curve->IsLinear();

	return 0;
    }
    void GetBBox(ON_3dPoint &min, ON_3dPoint &max)
    {
	min = m_node.m_min;
	max = m_node.m_max;
    }
    void SetBBox(const ON_BoundingBox &bbox)
    {
	m_node = bbox;
    }
    bool IsPointIn(const ON_3dPoint &pt, double tolerance = 0.0)
    {
	ON_3dVector vtol(tolerance, tolerance, tolerance);
	ON_BoundingBox new_bbox(m_node.m_min-vtol, m_node.m_max+vtol);
	return new_bbox.IsPointIn(pt);
    }
    bool Intersect(const Subcurve& other, double tolerance = 0.0) const
    {
	ON_3dVector vtol(tolerance, tolerance, tolerance);
	ON_BoundingBox new_bbox(m_node.m_min-vtol, m_node.m_max+vtol);
	ON_BoundingBox intersection;
	return intersection.Intersection(new_bbox, other.m_node);
    }
};


/* Sub-division support for a surface.
 * It's similar to generating the bounding box tree, when the Split()
 * method is called, the surface is splitted into two parts, whose bounding
 * boxes become the children of the original surface's bbox.
 */
struct Subsurface {
private:
    ON_BoundingBox m_node;
public:
    ON_Surface *m_surf;
    ON_Interval m_u, m_v;
    Subsurface *m_children[4];

    Subsurface() : m_surf(NULL)
    {
	m_children[0] = m_children[1] = m_children[2] = m_children[3] = NULL;
    }
    Subsurface(const Subsurface &_ssurf)
    {
	m_surf = _ssurf.m_surf->Duplicate();
	m_u = _ssurf.m_u;
	m_v = _ssurf.m_v;
	m_children[0] = m_children[1] = m_children[2] = m_children[3] = NULL;
	SetBBox(_ssurf.m_node);
    }
    ~Subsurface()
    {
	for (int i = 0; i < 4; i++) {
	    if (m_children[i])
		delete m_children[i];
	}
	delete m_surf;
    }
    int Split()
    {
	for (int i = 0; i < 4; i++)
	    m_children[i] = new Subsurface();
	ON_Surface *temp_surf1 = NULL, *temp_surf2 = NULL;
	ON_BOOL32 ret = true;
	ret = m_surf->Split(0, m_u.Mid(), temp_surf1, temp_surf2);
	if (!ret) {
	    delete temp_surf1;
	    delete temp_surf2;
	    return -1;
	}

	ret = temp_surf1->Split(1, m_v.Mid(), m_children[0]->m_surf, m_children[1]->m_surf);
	delete temp_surf1;
	if (!ret) {
	    delete temp_surf2;
	    return -1;
	}
	m_children[0]->m_u = ON_Interval(m_u.Min(), m_u.Mid());
	m_children[0]->m_v = ON_Interval(m_v.Min(), m_v.Mid());
	m_children[0]->SetBBox(m_children[0]->m_surf->BoundingBox());
	m_children[1]->m_u = ON_Interval(m_u.Min(), m_u.Mid());
	m_children[1]->m_v = ON_Interval(m_v.Mid(), m_v.Max());
	m_children[1]->SetBBox(m_children[1]->m_surf->BoundingBox());

	ret = temp_surf2->Split(1, m_v.Mid(), m_children[2]->m_surf, m_children[3]->m_surf);
	delete temp_surf2;
	if (!ret)
	    return -1;
	m_children[2]->m_u = ON_Interval(m_u.Mid(), m_u.Max());
	m_children[2]->m_v = ON_Interval(m_v.Min(), m_v.Mid());
	m_children[2]->SetBBox(m_children[2]->m_surf->BoundingBox());
	m_children[3]->m_u = ON_Interval(m_u.Mid(), m_u.Max());
	m_children[3]->m_v = ON_Interval(m_v.Mid(), m_v.Max());
	m_children[3]->SetBBox(m_children[3]->m_surf->BoundingBox());

	return 0;
    }
    void GetBBox(ON_3dPoint &min, ON_3dPoint &max)
    {
	min = m_node.m_min;
	max = m_node.m_max;
    }
    void SetBBox(const ON_BoundingBox &bbox)
    {
	m_node = bbox;
	/* Make sure that each dimension of the bounding box is greater than
	 * ON_ZERO_TOLERANCE.
	 * It does the same work as building the surface tree when ray tracing
	 */
	for (int i = 0; i < 3; i++) {
	    double d = m_node.m_max[i] - m_node.m_min[i];
	    if (ON_NearZero(d, ON_ZERO_TOLERANCE)) {
		m_node.m_min[i] -= 0.001;
		m_node.m_max[i] += 0.001;
	    }
	}
    }
};


/**
 * Point-point intersections (PPI)
 *
 * approach:
 *
 * - Calculate the distance of the two points.
 *
 * - If the distance is within the tolerance, the two points
 *   intersect. Thd mid-point of them is the intersection point,
 *   and half of the distance is the uncertainty radius.
 */
bool
ON_Intersect(const ON_3dPoint& pointA,
	     const ON_3dPoint& pointB,
	     ON_ClassArray<ON_PX_EVENT>& x,
	     double tolerance)
{
    if (tolerance <= 0.0)
	tolerance = ON_ZERO_TOLERANCE;

    if (pointA.DistanceTo(pointB) <= tolerance) {
	ON_PX_EVENT Event;
	Event.m_type = ON_PX_EVENT::ppx_point;
	Event.m_A = pointA;
	Event.m_B = pointB;
	Event.m_Mid = (pointA + pointB) * 0.5;
	Event.m_radius = pointA.DistanceTo(pointB) * 0.5;
	x.Append(Event);
	return true;
    }
    return false;
}


/**
 * Point-curve intersections (PCI)
 */

// The maximal depth for subdivision - trade-off between accurancy and
// performance
#define MAX_PCI_DEPTH 8

// We make the default tolerance for PSI the same as that of curve-curve
// intersections defined by openNURBS (see other/openNURBS/opennurbs_curve.h)
#define PCI_DEFAULT_TOLERANCE 0.001

// Newton-Raphson iteration needs a good start. Although we can almost get
// a good starting point every time, but in case of some unfortunate cases,
// we define a maximum iteration, preventing from infinite loop.
#define PCI_MAX_ITERATIONS 100

bool
ON_Intersect(const ON_3dPoint& pointA,
	     const ON_Curve& curveB,
	     ON_ClassArray<ON_PX_EVENT>& x,
	     double tolerance,
	     const ON_Interval* curveB_domain)
{
    if (tolerance <= 0.0)
	tolerance = PCI_DEFAULT_TOLERANCE;

    Subcurve root;
    if (curveB_domain == NULL || *curveB_domain == curveB.Domain()) {
	root.m_curve = curveB.Duplicate();
	root.m_t = curveB.Domain();
    }
    else {
	// Use ON_Curve::Split() to get the sub-curve inside curveB_domain
	ON_Curve *temp_curve1 = NULL, *temp_curve2 = NULL, *temp_curve3 = NULL;
	if (!curveB.Split(curveB_domain->Min(), temp_curve1, temp_curve2))
	    return false;
	delete temp_curve1;
	temp_curve1 = NULL;
	if (!temp_curve2->Split(curveB_domain->Max(), temp_curve1, temp_curve3))
	    return false;
	delete temp_curve1;
	delete temp_curve2;
	root.m_curve = temp_curve3;
	root.m_t = *curveB_domain;
    }

    root.SetBBox(root.m_curve->BoundingBox());
    root.m_islinear = root.m_curve->IsLinear();

    if (!root.IsPointIn(pointA, tolerance))
	return false;

    std::vector<Subcurve*> candidates, next_candidates;
    candidates.push_back(&root);

    // Find the sub-curves that are possibly intersected with the point.
    for (int i = 0; i < MAX_PCI_DEPTH; i++) {
	next_candidates.clear();
	for (unsigned int j = 0; j < candidates.size(); j++) {
	    if (candidates[j]->m_islinear) {
		next_candidates.push_back(candidates[j]);
	    } else {
		if (candidates[j]->Split() == 0) {
		    if (candidates[j]->m_children[0]->IsPointIn(pointA, tolerance))
			next_candidates.push_back(candidates[j]->m_children[0]);
		    if (candidates[j]->m_children[1]->IsPointIn(pointA, tolerance))
			next_candidates.push_back(candidates[j]->m_children[1]);
		} else
		    next_candidates.push_back(candidates[j]);
	    }
	}
	candidates = next_candidates;
    }

    for (unsigned int i = 0; i < candidates.size(); i++) {
	// Use linear approximation to get an estimated intersection point
	ON_Line line(candidates[i]->m_curve->PointAtStart(), candidates[i]->m_curve->PointAtEnd());
	double t, dis;
	line.ClosestPointTo(pointA, &t);

	// Make sure line_t belongs to [0, 1]
	double line_t;
	if (t < 0)
	    line_t = 0;
	else if (t > 1)
	    line_t = 1;
	else
	    line_t = t;

	double closest_point_t = candidates[i]->m_t.Min() + candidates[i]->m_t.Length()*line_t;
	ON_3dPoint closest_point = curveB.PointAt(closest_point_t);
	dis = closest_point.DistanceTo(pointA);

	// Use Newton-Raphson method with the starting point from linear
	// approximation.
	// It's similar to point projection (or point inversion, or get
	// closest point on curve). We calculate the root of:
	//       (curve(t) - pointA) \dot tangent(t) = 0 (*)
	// Then curve(t) may be the closest point on the curve to pointA.
	//
	// In some cases, the closest point (intersection point) is the
	// endpoints of the curve, it does not satisfy (*), but that's
	// OK because it already comes out with the linear approximation
	// above.

	double last_t = DBL_MAX;
	int iterations = 0;
	while (!ON_NearZero(last_t-closest_point_t)
	       && dis > tolerance
	       && iterations++ < PCI_MAX_ITERATIONS) {
	    ON_3dVector tangent, s_deriv;
	    last_t = closest_point_t;
	    curveB.Ev2Der(closest_point_t, closest_point, tangent, s_deriv);
	    double F = ON_DotProduct(closest_point-pointA, tangent);
	    double derivF = ON_DotProduct(closest_point-pointA, s_deriv) + ON_DotProduct(tangent, tangent);
	    if (!ON_NearZero(derivF))
		closest_point_t -= F/derivF;
	    dis = closest_point.DistanceTo(pointA);
	}
	closest_point = curveB.PointAt(closest_point_t);

	if (dis <= tolerance) {
	    ON_PX_EVENT *Event = new ON_PX_EVENT;
	    Event->m_type = ON_PX_EVENT::pcx_point;
	    Event->m_A = pointA;
	    Event->m_B = closest_point;
	    Event->m_b[0] = closest_point_t;
	    Event->m_Mid = (pointA + Event->m_B) * 0.5;
	    Event->m_radius = closest_point.DistanceTo(pointA) * 0.5;
	    x.Append(*Event);
	    return true;
	}
    }
    // All candidates have no intersection
    return false;
}


/**
 * Point-surface intersections (PSI)
 */

// We make the default tolerance for PSI the same as that of curve-surface
// intersections defined by openNURBS (see other/openNURBS/opennurbs_curve.h)
#define PSI_DEFAULT_TOLERANCE 0.001

// The default maximal depth for creating a surfacee tree (8) is way too
// much - killing performance. Since it's only used for getting an
// estimation, and we use Newton iterations afterwards, it's reasonable
// to use a smaller depth in this step.
#define MAX_PSI_DEPTH 4

bool
ON_Intersect(const ON_3dPoint& pointA,
	     const ON_Surface& surfaceB,
	     ON_ClassArray<ON_PX_EVENT>& x,
	     double tolerance,
	     const ON_Interval* surfaceB_udomain,
	     const ON_Interval* surfaceB_vdomain)
{
    if (tolerance <= 0.0)
	tolerance = PSI_DEFAULT_TOLERANCE;

    ON_Interval u_domain, v_domain;
    if (surfaceB_udomain == 0)
	u_domain = surfaceB.Domain(0);
    else
	u_domain = *surfaceB_udomain;
    if (surfaceB_vdomain == 0)
	v_domain = surfaceB.Domain(1);
    else
	v_domain = *surfaceB_vdomain;

    // We need ON_BrepFace for get_closest_point().
    // TODO: Use routines like SubSurface in SSI to reduce computation.
    ON_Brep *brep = ON_Brep::New();
    brep->AddSurface(surfaceB.Duplicate());
    brep->NewFace(0);
    ON_2dPoint closest_point_uv;
    brlcad::SurfaceTree *tree = new brlcad::SurfaceTree(brep->Face(0), false, MAX_PSI_DEPTH);
    if (brlcad::get_closest_point(closest_point_uv, brep->Face(0), pointA, tree) == false) {
	delete brep;
	delete tree;
	return false;
    }

    delete brep;
    delete tree;

    ON_3dPoint closest_point_3d = surfaceB.PointAt(closest_point_uv.x, closest_point_uv.y);
    if (pointA.DistanceTo(closest_point_3d) <= tolerance
	&& u_domain.Includes(closest_point_uv.x)
	&& v_domain.Includes(closest_point_uv.y)) {
	ON_PX_EVENT Event;
	Event.m_type = ON_PX_EVENT::psx_point;
	Event.m_A = pointA;
	Event.m_b = closest_point_uv;
	Event.m_B = closest_point_3d;
	Event.m_Mid = (pointA + Event.m_B) * 0.5;
	Event.m_radius = pointA.DistanceTo(Event.m_B) * 0.5;
	x.Append(Event);
	return true;
    }
    return false;
}


/**
 * Curve-curve intersection (CCI)
 */

// We make the default tolerance for CCI the same as that of curve-curve
// intersections defined by openNURBS (see other/openNURBS/opennurbs_curve.h)
#define CCI_DEFAULT_TOLERANCE 0.001

// The maximal depth for subdivision - trade-off between accurancy and
// performance
#define MAX_CCI_DEPTH 8

// Newton-Raphson iteration needs a good start. Although we can almost get
// a good starting point every time, but in case of some unfortunate cases,
// we define a maximum iteration, preventing from infinite loop.
#define CCI_MAX_ITERATIONS 100

// We can only test a finite number of points to determine overlap.
// Here we test 16 points uniformly distributed.
#define CCI_OVERLAP_TEST_POINTS 16

// Build the curve tree root within a given domain
bool
build_root(const ON_Curve* curve, const ON_Interval* domain, Subcurve& root)
{
    if (domain == NULL || *domain == curve->Domain()) {
	root.m_curve = curve->Duplicate();
	root.m_t = curve->Domain();
    } else {
	// Use ON_Curve::Split() to get the sub-curve inside the domain
	ON_Curve *temp_curve1 = NULL, *temp_curve2 = NULL, *temp_curve3 = NULL;
	if (!curve->Split(domain->Min(), temp_curve1, temp_curve2))
	    return false;
	delete temp_curve1;
	temp_curve1 = NULL;
	if (!temp_curve2->Split(domain->Max(), temp_curve1, temp_curve3))
	    return false;
	delete temp_curve1;
	delete temp_curve2;
	root.m_curve = temp_curve3;
	root.m_t = *domain;
    }

    root.SetBBox(root.m_curve->BoundingBox());
    root.m_islinear = root.m_curve->IsLinear();
    return true;
}


void
newton_cci(double& t_a, double& t_b, const ON_Curve* curveA, const ON_Curve* curveB)
{
    // Equations:
    //   x_a(t_a) - x_b(t_b) = 0
    //   y_a(t_a) - y_b(t_b) = 0
    //   z_a(t_a) - z_b(t_b) = 0
    // It's an over-determined system.
    // We use Newton-Raphson iterations to solve two equations of the three,
    // and use the third for checking.
    double last_t_a = DBL_MAX*.5, last_t_b = DBL_MAX*.5;
    ON_3dPoint pointA = curveA->PointAt(t_a);
    ON_3dPoint pointB = curveB->PointAt(t_b);
    int iteration = 0;
    while (fabs(last_t_a - t_a) + fabs(last_t_b - t_b) > ON_ZERO_TOLERANCE
	   && iteration++ < CCI_MAX_ITERATIONS) {
	last_t_a = t_a, last_t_b = t_b;
	ON_3dVector derivA, derivB;
	curveA->Ev1Der(t_a, pointA, derivA);
	curveB->Ev1Der(t_b, pointB, derivB);
	ON_Matrix J(2, 2), F(2, 1);
	J[0][0] = derivA.x;
	J[0][1] = -derivB.x;
	J[1][0] = derivA.y;
	J[1][1] = -derivB.y;
	F[0][0] = pointA.x - pointB.x;
	F[1][0] = pointA.y - pointB.y;
	if (!J.Invert(0.0)) {
	    // bu_log("Inverse failed.\n");
	    J[0][0] = derivA.x;
	    J[0][1] = -derivB.x;
	    J[1][0] = derivA.z;
	    J[1][1] = -derivB.z;
	    F[0][0] = pointA.x - pointB.x;
	    F[1][0] = pointA.z - pointB.z;
	    if (!J.Invert(0.0)) {
		// bu_log("Inverse failed again.\n");
		J[0][0] = derivA.y;
		J[0][1] = -derivB.y;
		J[1][0] = derivA.z;
		J[1][1] = -derivB.z;
		F[0][0] = pointA.y - pointB.y;
		F[1][0] = pointA.z - pointB.z;
		if (!J.Invert(0.0)) {
		    // bu_log("Inverse failed and never try again.\n");
		    continue;
		}
	    }
	}
	ON_Matrix Delta;
	Delta.Multiply(J, F);
	t_a -= Delta[0][0];
	t_b -= Delta[1][0];
    }

    // Make sure the solution is inside the domains
    t_a = std::min(t_a, curveA->Domain().Max());
    t_a = std::max(t_a, curveA->Domain().Min());
    t_b = std::min(t_b, curveB->Domain().Max());
    t_b = std::max(t_b, curveB->Domain().Min());
}


int
compare_by_m_a0(const ON_X_EVENT* a, const ON_X_EVENT* b)
{
    return a->m_a[0] - b->m_a[0];
}


int
ON_Intersect(const ON_Curve* curveA,
	     const ON_Curve* curveB,
	     ON_SimpleArray<ON_X_EVENT>& x,
	     double intersection_tolerance,
	     double overlap_tolerance,
	     const ON_Interval* curveA_domain,
	     const ON_Interval* curveB_domain)
{
    if (intersection_tolerance <= 0.0)
	intersection_tolerance = 0.001;
    if (overlap_tolerance <= 0.0)
	overlap_tolerance = 2*intersection_tolerance;

    Subcurve rootA, rootB;
    if (!build_root(curveA, curveA_domain, rootA))
	return 0;
    if (!build_root(curveB, curveB_domain, rootB))
	return 0;

    typedef std::vector<std::pair<Subcurve*, Subcurve*> > NodePairs;
    NodePairs candidates, next_candidates;
    if (rootA.Intersect(rootB, intersection_tolerance))
	candidates.push_back(std::make_pair(&rootA, &rootB));

    // Use sub-division and bounding box intersections first.
    for (int h = 0; h <= MAX_CCI_DEPTH; h++) {
	if (candidates.empty())
	    break;
	next_candidates.clear();
	for (NodePairs::iterator i = candidates.begin(); i != candidates.end(); i++) {
	    std::vector<Subcurve*> splittedA, splittedB;
	    if ((*i).first->m_islinear || (*i).first->Split() != 0) {
		splittedA.push_back((*i).first);
	    } else {
		splittedA.push_back((*i).first->m_children[0]);
		splittedA.push_back((*i).first->m_children[1]);
	    }
	    if ((*i).second->m_islinear || (*i).second->Split() != 0) {
		splittedB.push_back((*i).second);
	    } else {
		splittedB.push_back((*i).second->m_children[0]);
		splittedB.push_back((*i).second->m_children[1]);
	    }
	    for (unsigned int j = 0; j < splittedA.size(); j++)
		for (unsigned int k = 0; k < splittedB.size(); k++)
		    if (splittedA[j]->Intersect(*splittedB[k], intersection_tolerance))
			next_candidates.push_back(std::make_pair(splittedA[j], splittedB[k]));
	}
	candidates = next_candidates;
    }

    ON_SimpleArray<ON_X_EVENT> tmp_x;
    // For intersected bounding boxes, we calculate an accurate intersection
    // point.
    for (NodePairs::iterator i = candidates.begin(); i != candidates.end(); i++) {
	if (i->first->m_islinear && i->second->m_islinear) {
	    // Both of them are linear, we use ON_IntersectLineLine
	    ON_Line lineA(curveA->PointAt(i->first->m_t.Min()), curveA->PointAt(i->first->m_t.Max()));
	    ON_Line lineB(curveB->PointAt(i->second->m_t.Min()), curveB->PointAt(i->second->m_t.Max()));
	    if (lineA.Direction().IsParallelTo(lineB.Direction())) {
		// They are parallel
		if (lineA.MinimumDistanceTo(lineB) < intersection_tolerance) {
		    // we report a ccx_overlap event
		    double startB_on_A, endB_on_A, startA_on_B, endA_on_B;
		    lineA.ClosestPointTo(lineB.from, &startB_on_A);
		    lineA.ClosestPointTo(lineB.to, &endB_on_A);
		    lineB.ClosestPointTo(lineA.from, &startA_on_B);
		    lineB.ClosestPointTo(lineA.to, &endA_on_B);

		    if (startB_on_A > 1+intersection_tolerance || endB_on_A < -intersection_tolerance
			|| startA_on_B > 1+intersection_tolerance || endA_on_B < -intersection_tolerance)
			continue;

		    double t_a1, t_a2, t_b1, t_b2;
		    if (startB_on_A > 0.0)
			t_a1 = startB_on_A, t_b1 = 0.0;
		    else
			t_a1 = 0.0, t_b1 = startA_on_B;
		    if (endB_on_A < 1.0)
			t_a2 = endB_on_A, t_b2 = 1.0;
		    else
			t_a2 = 1.0, t_b2 = endA_on_B;

		    ON_X_EVENT* Event = new ON_X_EVENT;
		    Event->m_A[0] = lineA.PointAt(t_a1);
		    Event->m_A[1] = lineA.PointAt(t_a2);
		    Event->m_B[0] = lineB.PointAt(t_b1);
		    Event->m_B[1] = lineB.PointAt(t_b2);
		    Event->m_a[0] = i->first->m_t.ParameterAt(t_a1);
		    Event->m_a[1] = i->first->m_t.ParameterAt(t_a2);
		    Event->m_b[0] = i->second->m_t.ParameterAt(t_b1);
		    Event->m_b[1] = i->second->m_t.ParameterAt(t_b2);
		    Event->m_type = ON_X_EVENT::ccx_overlap;
		    tmp_x.Append(*Event);
		}
	    } else {
		// They are not parallel, check intersection point
		double t_lineA, t_lineB;
		double t_a, t_b;
		if (ON_IntersectLineLine(lineA, lineB, &t_lineA, &t_lineB, ON_ZERO_TOLERANCE, true)) {
		    // The line segments intersect
		    t_a = i->first->m_t.ParameterAt(t_lineA);
		    t_b = i->second->m_t.ParameterAt(t_lineB);

		    ON_X_EVENT* Event = new ON_X_EVENT;
		    Event->m_A[0] = Event->m_A[1] = lineA.PointAt(t_lineA);
		    Event->m_B[0] = Event->m_B[1] = lineB.PointAt(t_lineB);
		    Event->m_a[0] = Event->m_a[1] = t_a;
		    Event->m_b[0] = Event->m_b[1] = t_b;
		    Event->m_type = ON_X_EVENT::ccx_point;
		    tmp_x.Append(*Event);
		}
	    }
	} else {
	    // They are not both linear.

	    // Use two different start points - the two end-points of the interval
	    // If they converge to one point, it's considered an intersection
	    // point, otherwise it's considered an overlap event.
	    // FIXME: Find a better machanism to check overlapping, because this method
	    // may miss some overlap cases. (Overlap events can also converge to one
	    // point)
	    double t_a1 = i->first->m_t.Min(), t_b1 = i->second->m_t.Min();
	    newton_cci(t_a1, t_b1, curveA, curveB);
	    double t_a2 = i->first->m_t.Max(), t_b2 = i->second->m_t.Max();
	    newton_cci(t_a2, t_b2, curveA, curveB);

	    ON_3dPoint pointA1 = curveA->PointAt(t_a1);
	    ON_3dPoint pointB1 = curveB->PointAt(t_b1);
	    ON_3dPoint pointA2 = curveA->PointAt(t_a2);
	    ON_3dPoint pointB2 = curveB->PointAt(t_b2);
	    if (pointA1.DistanceTo(pointA2) < intersection_tolerance
		&& pointB1.DistanceTo(pointB2) < intersection_tolerance) {
		// it's considered the same point
		ON_3dPoint pointA = curveA->PointAt(t_a1);
		ON_3dPoint pointB = curveB->PointAt(t_b1);
		double distance = pointA.DistanceTo(pointB);
		// Check the validity of the solution
		if (distance < intersection_tolerance) {
		    ON_X_EVENT *Event = new ON_X_EVENT;
		    Event->m_A[0] = Event->m_A[1] = pointA;
		    Event->m_B[0] = Event->m_B[1] = pointB;
		    Event->m_a[0] = Event->m_a[1] = t_a1;
		    Event->m_b[0] = Event->m_b[1] = t_b1;
		    Event->m_type = ON_X_EVENT::ccx_point;
		    tmp_x.Append(*Event);
		}
	    } else {
		// Check overlap
		// bu_log("Maybe overlap.\n");
		double distance1 = pointA1.DistanceTo(pointB1);
		double distance2 = pointA2.DistanceTo(pointB2);

		// Check the validity of the solution
		if (distance1 < intersection_tolerance && distance2 < intersection_tolerance) {
		    ON_X_EVENT *Event = new ON_X_EVENT;
		    // We make sure that m_a[0] <= m_a[1]
		    if (t_a1 <= t_a2) {
			Event->m_A[0] = pointA1;
			Event->m_A[1] = pointA2;
			Event->m_B[0] = pointB1;
			Event->m_B[1] = pointB2;
			Event->m_a[0] = t_a1;
			Event->m_a[1] = t_a2;
			Event->m_b[0] = t_b1;
			Event->m_b[1] = t_b2;
		    } else {
			Event->m_A[0] = pointA2;
			Event->m_A[1] = pointA1;
			Event->m_B[0] = pointB2;
			Event->m_B[1] = pointB1;
			Event->m_a[0] = t_a2;
			Event->m_a[1] = t_a1;
			Event->m_b[0] = t_b2;
			Event->m_b[1] = t_b1;
		    }
		    int j;
		    for (j = 1; j < CCI_OVERLAP_TEST_POINTS; j++) {
			double strike = 1.0/CCI_OVERLAP_TEST_POINTS;
			ON_3dPoint test_point = curveA->PointAt(t_a1 + (t_a2-t_a1)*j*strike);
			ON_ClassArray<ON_PX_EVENT> pci_x;
			// Use point-curve intersection
			if (!ON_Intersect(test_point, *curveA, pci_x, overlap_tolerance))
			    break;
		    }
		    if (j == CCI_OVERLAP_TEST_POINTS) {
			Event->m_type = ON_X_EVENT::ccx_overlap;
			tmp_x.Append(*Event);
			continue;
		    }
		    // if j != CCI_OVERLAP_TEST_POINTS, two ccx_point events should
		    // be generated. Fall through.
		}
		if (distance1 < intersection_tolerance) {
		    // in case that the second one was not correct
		    ON_X_EVENT *Event = new ON_X_EVENT;
		    Event->m_A[0] = Event->m_A[1] = pointA1;
		    Event->m_B[0] = Event->m_B[1] = pointB1;
		    Event->m_a[0] = Event->m_a[1] = t_a1;
		    Event->m_b[0] = Event->m_b[1] = t_b1;
		    Event->m_type = ON_X_EVENT::ccx_point;
		    tmp_x.Append(*Event);
		}
		if (distance2 < intersection_tolerance) {
		    // in case that the first one was not correct
		    ON_X_EVENT *Event = new ON_X_EVENT;
		    Event->m_A[0] = Event->m_A[1] = pointA2;
		    Event->m_B[0] = Event->m_B[1] = pointB2;
		    Event->m_a[0] = Event->m_a[1] = t_a2;
		    Event->m_b[0] = Event->m_b[1] = t_b2;
		    Event->m_type = ON_X_EVENT::ccx_point;
		    tmp_x.Append(*Event);
		}
	    }
	}
    }

    ON_SimpleArray<ON_X_EVENT> points, overlap, pending;
    // Use an O(n^2) naive approach to eliminate duplications
    for (int i = 0; i < tmp_x.Count(); i++) {
	int j;
	if (tmp_x[i].m_type == ON_X_EVENT::ccx_overlap) {
	    overlap.Append(tmp_x[i]);
	    continue;
	}
	// ccx_point
	for (j = 0; j < points.Count(); j++) {
	    if (tmp_x[i].m_A[0].DistanceTo(points[j].m_A[0]) < intersection_tolerance
		&& tmp_x[i].m_A[1].DistanceTo(points[j].m_A[1]) < intersection_tolerance
		&& tmp_x[i].m_B[0].DistanceTo(points[j].m_B[0]) < intersection_tolerance
		&& tmp_x[i].m_B[1].DistanceTo(points[j].m_B[1]) < intersection_tolerance)
		break;
	}
	if (j == points.Count()) {
	    points.Append(tmp_x[i]);
	}
    }

    // Merge the overlap events that are continuous
    overlap.QuickSort(compare_by_m_a0);
    for (int i = 0; i < overlap.Count(); i++) {
	bool merged = false;
	for (int j = 0; j < pending.Count(); j++) {
	    if (pending[j].m_a[1] < overlap[i].m_a[0] - intersection_tolerance) {
		x.Append(pending[j]);
		pending.Remove(j);
		j--;
		continue;
	    }
	    if (overlap[i].m_a[0] < pending[j].m_a[1] + intersection_tolerance
		&& overlap[i].m_b[0] < pending[j].m_b[1] + intersection_tolerance) {
		// Need to merge
		merged = true;
		pending[j].m_a[1] = std::max(overlap[i].m_a[1], pending[j].m_a[1]);
		pending[j].m_b[1] = std::max(overlap[i].m_b[1], pending[j].m_b[1]);
		break;
	    }
	}
	if (merged == false)
	    pending.Append(overlap[i]);
    }
    for (int i = 0; i < pending.Count(); i++)
	x.Append(pending[i]);

    // The intersection points shouldn't be inside the overlapped parts.
    int overlap_events = x.Count();
    for (int i = 0; i < points.Count(); i++) {
	int j;
	for (j = 0; j < overlap_events; j++) {
	    if (points[i].m_a[0] > x[j].m_a[0] - intersection_tolerance
		&& points[i].m_a[0] < x[j].m_a[1] + intersection_tolerance)
		break;
	}
	if (j == overlap_events)
	    x.Append(points[i]);
    }

    return x.Count();
}


/**
 * Curve-surface intersection (CSI)
 */
int
ON_Intersect(const ON_Curve*,
	     const ON_Surface*,
	     ON_SimpleArray<ON_X_EVENT>&,
	     double,
	     double,
	     const ON_Interval*,
	     const ON_Interval*,
	     const ON_Interval*)
{
    // Implement later.
    return 0;
}


/**
 * Surface-surface intersections (SSI)
 *
 * approach:
 *
 * - Generate the bounding box of the two surfaces.
 *
 * - If their bounding boxes intersect:
 *
 * -- Split the two surfaces, both into four parts, and calculate the
 *    sub-surfaces' bounding boxes
 *
 * -- Calculate the intersection of sub-surfaces' bboxes, if they do
 *    intersect, go deeper with splitting surfaces and smaller bboxes,
 *    otherwise trace back.
 *
 * - After getting the intersecting bboxes, approximate the surface
 *   inside the bbox as two triangles, and calculate the intersection
 *   points of the triangles (both in 3d space and two surfaces' UV
 *   space)
 *
 * - Fit the intersection points into polyline curves, and then to NURBS
 *   curves. Points with distance less than max_dis are considered in
 *   one curve.
 *
 * See: Adarsh Krishnamurthy, Rahul Khardekar, Sara McMains, Kirk Haller,
 * and Gershon Elber. 2008.
 * Performing efficient NURBS modeling operations on the GPU.
 * In Proceedings of the 2008 ACM symposium on Solid and physical modeling
 * (SPM '08). ACM, New York, NY, USA, 257-268. DOI=10.1145/1364901.1364937
 * http://doi.acm.org/10.1145/1364901.1364937
 */

struct Triangle {
    ON_3dPoint A, B, C;
    inline void CreateFromPoints(ON_3dPoint &p_A, ON_3dPoint &p_B, ON_3dPoint &p_C) {
	A = p_A;
	B = p_B;
	C = p_C;
    }
    ON_3dPoint BarycentricCoordinate(ON_3dPoint &pt)
    {
	ON_Matrix M(3, 3);
	M[0][0] = A[0], M[0][1] = B[0], M[0][2] = C[0];
	M[1][0] = A[1], M[1][1] = B[1], M[1][2] = C[1];
	M[2][0] = A[2], M[2][1] = B[2], M[2][2] = C[2];
	M.Invert(0.0);
	ON_Matrix MX(3, 1);
	MX[0][0] = pt[0], MX[1][0] = pt[1], MX[2][0] = pt[2];
	M.Multiply(M, MX);
	return ON_3dPoint(M[0][0], M[1][0], M[2][0]);
    }
};


bool
triangle_intersection(const struct Triangle &TriA, const struct Triangle &TriB, ON_3dPoint &center)
{
    ON_Plane plane_a(TriA.A, TriA.B, TriA.C);
    ON_Plane plane_b(TriB.A, TriB.B, TriB.C);
    ON_Line intersect;
    if (!ON_Intersect(plane_a, plane_b, intersect))
	return false;
    ON_3dVector line_normal = ON_CrossProduct(plane_a.Normal(), intersect.Direction());

    // dpi - >0: one side of the line, <0: another side, ==0: on the line
    double dp1 = ON_DotProduct(TriA.A - intersect.from, line_normal);
    double dp2 = ON_DotProduct(TriA.B - intersect.from, line_normal);
    double dp3 = ON_DotProduct(TriA.C - intersect.from, line_normal);

    int points_on_one_side = (dp1 >= 0) + (dp2 >= 0) + (dp3 >= 0);
    if (points_on_one_side == 0 || points_on_one_side == 3)
	return false;

    double dp4 = ON_DotProduct(TriB.A - intersect.from, line_normal);
    double dp5 = ON_DotProduct(TriB.B - intersect.from, line_normal);
    double dp6 = ON_DotProduct(TriB.C - intersect.from, line_normal);
    points_on_one_side = (dp4 >= 0) + (dp5 >= 0) + (dp6 >= 0);
    if (points_on_one_side == 0 || points_on_one_side == 3)
	return false;

    double t[4];
    int count = 0;
    double t1, t2;

    // dpi*dpj < 0 - the corresponding points are on different sides
    // - the line segment between them are intersected by the plane-plane
    // intersection line
    if (dp1*dp2 < 0) {
	intersect.ClosestPointTo(TriA.A, &t1);
	intersect.ClosestPointTo(TriA.B, &t2);
	double d1 = TriA.A.DistanceTo(intersect.PointAt(t1));
	double d2 = TriA.B.DistanceTo(intersect.PointAt(t2));
	if (!ZERO(d1 + d2))
	    t[count++] = t1 + d1/(d1+d2)*(t2-t1);
    }
    if (dp1*dp3 < 0) {
	intersect.ClosestPointTo(TriA.A, &t1);
	intersect.ClosestPointTo(TriA.C, &t2);
	double d1 = TriA.A.DistanceTo(intersect.PointAt(t1));
	double d2 = TriA.C.DistanceTo(intersect.PointAt(t2));
	if (!ZERO(d1 + d2))
	    t[count++] = t1 + d1/(d1+d2)*(t2-t1);
    }
    if (dp2*dp3 < 0 && count != 2) {
	intersect.ClosestPointTo(TriA.B, &t1);
	intersect.ClosestPointTo(TriA.C, &t2);
	double d1 = TriA.B.DistanceTo(intersect.PointAt(t1));
	double d2 = TriA.C.DistanceTo(intersect.PointAt(t2));
	if (!ZERO(d1 + d2))
	    t[count++] = t1 + d1/(d1+d2)*(t2-t1);
    }
    if (count != 2)
	return false;
    if (dp4*dp5 < 0) {
	intersect.ClosestPointTo(TriB.A, &t1);
	intersect.ClosestPointTo(TriB.B, &t2);
	double d1 = TriB.A.DistanceTo(intersect.PointAt(t1));
	double d2 = TriB.B.DistanceTo(intersect.PointAt(t2));
	if (!ZERO(d1 + d2))
	    t[count++] = t1 + d1/(d1+d2)*(t2-t1);
    }
    if (dp4*dp6 < 0) {
	intersect.ClosestPointTo(TriB.A, &t1);
	intersect.ClosestPointTo(TriB.C, &t2);
	double d1 = TriB.A.DistanceTo(intersect.PointAt(t1));
	double d2 = TriB.C.DistanceTo(intersect.PointAt(t2));
	if (!ZERO(d1 + d2))
	    t[count++] = t1 + d1/(d1+d2)*(t2-t1);
    }
    if (dp5*dp6 < 0 && count != 4) {
	intersect.ClosestPointTo(TriB.B, &t1);
	intersect.ClosestPointTo(TriB.C, &t2);
	double d1 = TriB.B.DistanceTo(intersect.PointAt(t1));
	double d2 = TriB.C.DistanceTo(intersect.PointAt(t2));
	if (!ZERO(d1 + d2))
	    t[count++] = t1 + d1/(d1+d2)*(t2-t1);
    }
    if (count != 4)
	return false;
    if (t[0] > t[1])
	std::swap(t[1], t[0]);
    if (t[2] > t[3])
	std::swap(t[3], t[2]);
    double left = std::max(t[0], t[2]);
    double right = std::min(t[1], t[3]);
    if (left > right)
	return false;
    center = intersect.PointAt((left+right)/2);
    return true;
}


struct PointPair {
    int indexA, indexB;
    double distance3d, distanceA, distanceB;
    bool operator < (const PointPair &_pp) const {
	return distance3d < _pp.distance3d;
    }
};


// The maximal depth for subdivision - trade-off between accurancy and
// performance
#define MAX_SSI_DEPTH 8

int
ON_Intersect(const ON_Surface* surfA,
	     const ON_Surface* surfB,
	     ON_ClassArray<ON_SSX_EVENT>& x,
	     double,
	     double,
	     double,
	     const ON_Interval*,
	     const ON_Interval*,
	     const ON_Interval*,
	     const ON_Interval*)
{
    if (surfA == NULL || surfB == NULL) {
	return -1;
    }

    /* First step: Initialize the first two Subsurface.
     * It's just like getting the root of the surface tree.
     */
    typedef std::vector<std::pair<Subsurface*, Subsurface*> > NodePairs;
    NodePairs nodepairs;
    Subsurface *rootA = new Subsurface(), *rootB = new Subsurface();
    rootA->SetBBox(surfA->BoundingBox());
    rootA->m_u = surfA->Domain(0);
    rootA->m_v = surfA->Domain(1);
    rootA->m_surf = surfA->Duplicate();
    rootB->SetBBox(surfB->BoundingBox());
    rootB->m_u = surfB->Domain(0);
    rootB->m_v = surfB->Domain(1);
    rootB->m_surf = surfB->Duplicate();
    nodepairs.push_back(std::make_pair(rootA, rootB));
    ON_3dPointArray curvept;
    ON_2dPointArray curveuv, curvest;
    int bbox_count = 0;

    /* Second step: calculate the intersection of the bounding boxes, using
     * trigonal approximation.
     * Only the children of intersecting b-box pairs need to be considered.
     * The children will be generated only when they are needed, using the
     * method of splitting a NURBS surface.
     * So finally only a small subset of the surface tree is created.
     */
    for (int h = 0; h <= MAX_SSI_DEPTH; h++) {
	if (nodepairs.empty())
	    break;
	NodePairs tmp_pairs;
	if (h) {
	    for (NodePairs::iterator i = nodepairs.begin(); i != nodepairs.end(); i++) {
		int ret1 = 0;
		if ((*i).first->m_children[0] == NULL)
		    ret1 = (*i).first->Split();
		int ret2 = 0;
		if ((*i).second->m_children[0] == NULL)
		    ret2 = (*i).second->Split();
		if (ret1) {
		    if (ret2) {
			/* both splits failed */
			tmp_pairs.push_back(*i);
			h = MAX_SSI_DEPTH;
		    } else {
			/* the first failed */
			for (int j = 0; j < 4; j++)
			    tmp_pairs.push_back(std::make_pair((*i).first, (*i).second->m_children[j]));
		    }
		} else {
		    if (ret2) {
			/* the second failed */
			for (int j = 0; j < 4; j++)
			    tmp_pairs.push_back(std::make_pair((*i).first->m_children[j], (*i).second));
		    } else {
			/* both success */
			for (int j = 0; j < 4; j++)
			    for (int k = 0; k < 4; k++)
				tmp_pairs.push_back(std::make_pair((*i).first->m_children[j], (*i).second->m_children[k]));
		    }
		}
	    }
	} else {
	    tmp_pairs = nodepairs;
	}
	nodepairs.clear();
	for (NodePairs::iterator i = tmp_pairs.begin(); i != tmp_pairs.end(); i++) {
	    ON_BoundingBox box_a, box_b, box_intersect;
	    (*i).first->GetBBox(box_a.m_min, box_a.m_max);
	    (*i).second->GetBBox(box_b.m_min, box_b.m_max);
	    if (box_intersect.Intersection(box_a, box_b)) {
		if (h == MAX_SSI_DEPTH) {
		    // We have arrived at the bottom of the trees.
		    // Get an estimate of the intersection point lying on the intersection curve

		    // Get the corners of each surface sub-patch inside the bounding-box.
		    ON_3dPoint cornerA[4], cornerB[4];
		    double u_min = (*i).first->m_u.Min(), u_max = (*i).first->m_u.Max();
		    double v_min = (*i).first->m_v.Min(), v_max = (*i).first->m_v.Max();
		    double s_min = (*i).second->m_u.Min(), s_max = (*i).second->m_u.Max();
		    double t_min = (*i).second->m_v.Min(), t_max = (*i).second->m_v.Max();
		    cornerA[0] = surfA->PointAt(u_min, v_min);
		    cornerA[1] = surfA->PointAt(u_min, v_max);
		    cornerA[2] = surfA->PointAt(u_max, v_min);
		    cornerA[3] = surfA->PointAt(u_max, v_max);
		    cornerB[0] = surfB->PointAt(s_min, t_min);
		    cornerB[1] = surfB->PointAt(s_min, t_max);
		    cornerB[2] = surfB->PointAt(s_max, t_min);
		    cornerB[3] = surfB->PointAt(s_max, t_max);

		    /* We approximate each surface sub-patch inside the bounding-box with two
		     * triangles that share an edge.
		     * The intersection of the surface sub-patches is approximated as the
		     * intersection of triangles.
		     */
		    struct Triangle triangle[4];
		    triangle[0].CreateFromPoints(cornerA[0], cornerA[1], cornerA[2]);
		    triangle[1].CreateFromPoints(cornerA[1], cornerA[2], cornerA[3]);
		    triangle[2].CreateFromPoints(cornerB[0], cornerB[1], cornerB[2]);
		    triangle[3].CreateFromPoints(cornerB[1], cornerB[2], cornerB[3]);
		    bool is_intersect[4];
		    ON_3dPoint intersect_center[4];
		    is_intersect[0] = triangle_intersection(triangle[0], triangle[2], intersect_center[0]);
		    is_intersect[1] = triangle_intersection(triangle[0], triangle[3], intersect_center[1]);
		    is_intersect[2] = triangle_intersection(triangle[1], triangle[2], intersect_center[2]);
		    is_intersect[3] = triangle_intersection(triangle[1], triangle[3], intersect_center[3]);

		    // Calculate the intersection centers' uv (st) coordinates.
		    ON_3dPoint bcoor[8];
		    ON_2dPoint uv[4]/*surfA*/, st[4]/*surfB*/;
		    if (is_intersect[0]) {
			bcoor[0] = triangle[0].BarycentricCoordinate(intersect_center[0]);
			bcoor[1] = triangle[2].BarycentricCoordinate(intersect_center[0]);
			uv[0].x = bcoor[0].x*u_min + bcoor[0].y*u_min + bcoor[0].z*u_max;
			uv[0].y = bcoor[0].x*v_min + bcoor[0].y*v_max + bcoor[0].z*v_min;
			st[0].x = bcoor[1].x*s_min + bcoor[1].y*s_min + bcoor[1].z*s_max;
			st[0].y = bcoor[1].x*t_min + bcoor[1].y*t_max + bcoor[1].z*t_min;
		    }
		    if (is_intersect[1]) {
			bcoor[2] = triangle[0].BarycentricCoordinate(intersect_center[1]);
			bcoor[3] = triangle[3].BarycentricCoordinate(intersect_center[1]);
			uv[1].x = bcoor[2].x*u_min + bcoor[2].y*u_min + bcoor[2].z*u_max;
			uv[1].y = bcoor[2].x*v_min + bcoor[2].y*v_max + bcoor[2].z*v_min;
			st[1].x = bcoor[3].x*s_min + bcoor[3].y*s_max + bcoor[3].z*s_max;
			st[1].y = bcoor[3].x*t_max + bcoor[3].y*t_min + bcoor[3].z*t_max;
		    }
		    if (is_intersect[2]) {
			bcoor[4] = triangle[1].BarycentricCoordinate(intersect_center[2]);
			bcoor[5] = triangle[2].BarycentricCoordinate(intersect_center[2]);
			uv[2].x = bcoor[4].x*u_min + bcoor[4].y*u_max + bcoor[4].z*u_max;
			uv[2].y = bcoor[4].x*v_max + bcoor[4].y*v_min + bcoor[4].z*v_max;
			st[2].x = bcoor[5].x*s_min + bcoor[5].y*s_min + bcoor[5].z*s_max;
			st[2].y = bcoor[5].x*t_min + bcoor[5].y*t_max + bcoor[5].z*t_min;
		    }
		    if (is_intersect[3]) {
			bcoor[6] = triangle[1].BarycentricCoordinate(intersect_center[3]);
			bcoor[7] = triangle[3].BarycentricCoordinate(intersect_center[3]);
			uv[3].x = bcoor[6].x*u_min + bcoor[6].y*u_max + bcoor[6].z*u_max;
			uv[3].y = bcoor[6].x*v_max + bcoor[6].y*v_min + bcoor[6].z*v_max;
			st[3].x = bcoor[7].x*s_min + bcoor[7].y*s_max + bcoor[7].z*s_max;
			st[3].y = bcoor[7].x*t_max + bcoor[7].y*t_min + bcoor[7].z*t_max;
		    }

		    // The centroid of these intersection centers is the
		    // intersection point we want.
		    int num_intersects = 0;
		    ON_3dPoint average(0.0, 0.0, 0.0);
		    ON_2dPoint avguv(0.0, 0.0), avgst(0.0, 0.0);
		    for (int j = 0; j < 4; j++) {
			if (is_intersect[j]) {
			    average += intersect_center[j];
			    avguv += uv[j];
			    avgst += st[j];
			    num_intersects++;
			}
		    }
		    if (num_intersects) {
			average /= num_intersects;
			avguv /= num_intersects;
			avgst /= num_intersects;
			if (box_intersect.IsPointIn(average)) {
			    curvept.Append(average);
			    curveuv.Append(avguv);
			    curvest.Append(avgst);
			}
		    }
		    bbox_count++;
		} else {
		    nodepairs.push_back(*i);
		}
	    }
	}
    }
    bu_log("We get %d intersection bounding boxes.\n", bbox_count);
    bu_log("%d points on the intersection curves.\n", curvept.Count());

    if (!curvept.Count()) {
	delete rootA;
	delete rootB;
	return 0;
    }

    // Third step: Fit the points in curvept into NURBS curves.
    // Here we use polyline approximation.
    // TODO: Find a better fitting algorithm unless this is good enough.

    // We need to automatically generate a threshold.
    double max_dis;
    if (ZERO(surfA->BoundingBox().Volume())) {
	max_dis = pow(surfB->BoundingBox().Volume(), 1.0/3.0) * 0.2;
    } else if (ZERO(surfB->BoundingBox().Volume())) {
	max_dis = pow(surfA->BoundingBox().Volume(), 1.0/3.0) * 0.2;
    } else {
	max_dis = pow(surfA->BoundingBox().Volume()*surfB->BoundingBox().Volume(), 1.0/6.0) * 0.2;
    }

    double max_dis_2dA = ON_2dVector(surfA->Domain(0).Length(), surfA->Domain(1).Length()).Length() * 0.1;
    double max_dis_2dB = ON_2dVector(surfB->Domain(0).Length(), surfB->Domain(1).Length()).Length() * 0.1;
    bu_log("max_dis: %lf\n", max_dis);
    bu_log("max_dis_2dA: %lf\n", max_dis_2dA);
    bu_log("max_dis_2dB: %lf\n", max_dis_2dB);
    // NOTE: More tests are needed to find a better threshold.

    std::vector<PointPair> ptpairs;
    for (int i = 0; i < curvept.Count(); i++) {
	for (int j = i + 1; j < curvept.Count(); j++) {
	    PointPair ppair;
	    ppair.distance3d = curvept[i].DistanceTo(curvept[j]);
	    ppair.distanceA = curveuv[i].DistanceTo(curveuv[j]);
	    ppair.distanceB = curvest[i].DistanceTo(curvest[j]);
	    if (ppair.distanceA < max_dis_2dA && ppair.distanceB < max_dis_2dB) {
		ppair.indexA = i;
		ppair.indexB = j;
		ptpairs.push_back(ppair);
	    }
	}
    }
    std::sort(ptpairs.begin(), ptpairs.end());

    std::vector<ON_SimpleArray<int>*> polylines(curvept.Count());
    int *index = (int*)bu_malloc(curvept.Count() * sizeof(int), "int");
    // index[i] = j means curvept[i] is a startpoint/endpoint of polylines[j]
    int *startpt = (int*)bu_malloc(curvept.Count() * sizeof(int), "int");
    int *endpt = (int*)bu_malloc(curvept.Count() * sizeof(int), "int");
    // index of startpoints and endpoints of polylines[i]

    // Initialize each polyline with only one point.
    for (int i = 0; i < curvept.Count(); i++) {
	ON_SimpleArray<int> *single = new ON_SimpleArray<int>();
	single->Append(i);
	polylines[i] = single;
	index[i] = i;
	startpt[i] = i;
	endpt[i] = i;
    }

    // Merge polylines with distance less than max_dis.
    for (unsigned int i = 0; i < ptpairs.size(); i++) {
	int index1 = index[ptpairs[i].indexA], index2 = index[ptpairs[i].indexB];
	if (index1 == -1 || index2 == -1)
	    continue;
	index[startpt[index1]] = index[endpt[index1]] = index1;
	index[startpt[index2]] = index[endpt[index2]] = index1;
	ON_SimpleArray<int> *line1 = polylines[index1];
	ON_SimpleArray<int> *line2 = polylines[index2];
	if (line1 != NULL && line2 != NULL && line1 != line2) {
	    ON_SimpleArray<int> *unionline = new ON_SimpleArray<int>();
	    if ((*line1)[0] == ptpairs[i].indexA) {
		if ((*line2)[0] == ptpairs[i].indexB) {
		    // Case 1: endA -- startA -- startB -- endB
		    line1->Reverse();
		    unionline->Append(line1->Count(), line1->Array());
		    unionline->Append(line2->Count(), line2->Array());
		    startpt[index1] = endpt[index1];
		    endpt[index1] = endpt[index2];
		} else {
		    // Case 2: startB -- endB -- startA -- endA
		    unionline->Append(line2->Count(), line2->Array());
		    unionline->Append(line1->Count(), line1->Array());
		    startpt[index1] = startpt[index2];
		    endpt[index1] = endpt[index1];
		}
	    } else {
		if ((*line2)[0] == ptpairs[i].indexB) {
		    // Case 3: startA -- endA -- startB -- endB
		    unionline->Append(line1->Count(), line1->Array());
		    unionline->Append(line2->Count(), line2->Array());
		    startpt[index1] = startpt[index1];
		    endpt[index1] = endpt[index2];
		} else {
		    // Case 4: start -- endA -- endB -- startB
		    unionline->Append(line1->Count(), line1->Array());
		    line2->Reverse();
		    unionline->Append(line2->Count(), line2->Array());
		    startpt[index1] = startpt[index1];
		    endpt[index1] = startpt[index2];
		}
	    }
	    polylines[index1] = unionline;
	    polylines[index2] = NULL;
	    if (line1->Count() >= 2) {
		index[ptpairs[i].indexA] = -1;
	    }
	    if (line2->Count() >= 2) {
		index[ptpairs[i].indexB] = -1;
	    }
	    delete line1;
	    delete line2;
	}
    }

    // Generate NURBS curves from the polylines.
    ON_SimpleArray<ON_NurbsCurve *> intersect3d, intersect_uv2d, intersect_st2d;
    for (unsigned int i = 0; i < polylines.size(); i++) {
	if (polylines[i] != NULL) {
	    int startpoint = (*polylines[i])[0];
	    int endpoint = (*polylines[i])[polylines[i]->Count() - 1];

	    // The intersection curves in the 3d space
	    ON_3dPointArray ptarray;
	    for (int j = 0; j < polylines[i]->Count(); j++)
		ptarray.Append(curvept[(*polylines[i])[j]]);
	    // If it form a loop in the 3D space
	    if (curvept[startpoint].DistanceTo(curvept[endpoint]) < max_dis) {
		ptarray.Append(curvept[startpoint]);
	    }
	    ON_PolylineCurve curve(ptarray);
	    ON_NurbsCurve *nurbscurve = ON_NurbsCurve::New();
	    if (curve.GetNurbForm(*nurbscurve)) {
		intersect3d.Append(nurbscurve);
	    }

	    // The intersection curves in the 2d UV parameter space (surfA)
	    ptarray.Empty();
	    for (int j = 0; j < polylines[i]->Count(); j++) {
		ON_2dPoint &pt2d = curveuv[(*polylines[i])[j]];
		ptarray.Append(ON_3dPoint(pt2d.x, pt2d.y, 0.0));
	    }
	    // If it form a loop in the 2D space (happens rarely compared to 3D)
	    if (curveuv[startpoint].DistanceTo(curveuv[endpoint]) < max_dis_2dA) {
		ON_2dPoint &pt2d = curveuv[startpoint];
		ptarray.Append(ON_3dPoint(pt2d.x, pt2d.y, 0.0));
	    }
	    curve = ON_PolylineCurve(ptarray);
	    curve.ChangeDimension(2);
	    nurbscurve = ON_NurbsCurve::New();
	    if (curve.GetNurbForm(*nurbscurve)) {
		intersect_uv2d.Append(nurbscurve);
	    }

	    // The intersection curves in the 2d UV parameter space (surfB)
	    ptarray.Empty();
	    for (int j = 0; j < polylines[i]->Count(); j++) {
		ON_2dPoint &pt2d = curvest[(*polylines[i])[j]];
		ptarray.Append(ON_3dPoint(pt2d.x, pt2d.y, 0.0));
	    }
	    // If it form a loop in the 2D space (happens rarely compared to 3D)
	    if (curvest[startpoint].DistanceTo(curvest[endpoint]) < max_dis_2dB) {
		ON_2dPoint &pt2d = curvest[startpoint];
		ptarray.Append(ON_3dPoint(pt2d.x, pt2d.y, 0.0));
	    }
	    curve = ON_PolylineCurve(ptarray);
	    curve.ChangeDimension(2);
	    nurbscurve = ON_NurbsCurve::New();
	    if (curve.GetNurbForm(*nurbscurve)) {
		intersect_st2d.Append(nurbscurve);
	    }

	    delete polylines[i];
	}
    }

    bu_log("Segments: %d\n", intersect3d.Count());
    bu_free(index, "int");
    bu_free(startpt, "int");
    bu_free(endpt, "int");
    delete rootA;
    delete rootB;

    // Generate ON_SSX_EVENTs
    if (intersect3d.Count()) {
	for (int i = 0; i < intersect3d.Count(); i++) {
	    ON_SSX_EVENT *ssx = new ON_SSX_EVENT;
	    ssx->m_curve3d = intersect3d[i];
	    ssx->m_curveA = intersect_uv2d[i];
	    ssx->m_curveB = intersect_st2d[i];
	    // Now we can only have ssx_transverse
	    ssx->m_type = ON_SSX_EVENT::ssx_transverse;
	    x.Append(*ssx);
	}
    }

    return x.Count();
}


// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
