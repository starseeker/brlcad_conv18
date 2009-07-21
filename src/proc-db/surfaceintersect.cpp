/*           S U R F A C E I N T E R S E C T . C P P
 * BRL-CAD
 *
 * Copyright (c) 2008-2009 United States Government as represented by
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
/** @file surfaceintersect.cpp
 *
 */

/* Until further notice this code is in a state of heavy flux as part of
 * GSoC 2009 as such it would be very foolish to write anything that
 * depends on it right now
 * This code is written and maintained by Joe Doliner jdoliner@gmail.com */

#include "surfaceintersect.h"
#define MIN(a, b) (((a) > (b)) ? (a) : (b))

/**
 *       ClosestValue
 *
 * @brief returns the value that is closest to the given value but in the given interval
 */
double ClosestValue(
	double value,
	ON_Interval interval
	)
{
    if (interval.Includes(value, true)) {
	return value;
    } else if (value < interval.Min()) {
	return interval.Min();
    } else {
	return interval.Max();
    }
}

/**
 *        step
 *
 * @brief advances s1, s2, t1, t2 along the curve of intersection of the two surfaces
 * by a distance of step size.
 */
int Step(
	ON_Surface *surf1,
	ON_Surface *surf2,
	double *s1,
	double *s2,
	double *t1,
	double *t2,
	double stepsize
	)
{
    ON_3dVector Norm1 = surf1->NormalAt(*s1, *t1);
    ON_3dVector Norm2 = surf2->NormalAt(*s2, *t2);
    ON_3dVector step = ON_CrossProduct(Norm1, Norm2);
    double Magnitude = ON_ArrayMagnitude(3, step);
    double vec[3] = {0.0, 0.0, 0.0};
    ON_3dVector stepscaled =  vec;
    ON_ArrayScale(3, sqrt(stepsize/Magnitude), step, stepscaled);
    ON_3dPoint value1, value2;
    ON_3dVector ds1, ds2, dt1, dt2; /* the partial derivatives */
    assert(surf1->Ev1Der(*s1, *t1, value1, ds1, dt1));
    assert(surf2->Ev1Der(*s2, *t2, value2, ds2, dt2));
    double delta_s1, delta_s2, delta_t1, delta_t2;
    ON_DecomposeVector(stepscaled, ds1, dt1, &delta_s1, &delta_t1);
    ON_DecomposeVector(stepscaled, ds2, dt2, &delta_s2, &delta_t2);
    *s1 += delta_s1;
    *s2 += delta_s2;
    *t1 += delta_t1;
    *t2 += delta_t2;
}

void WalkIntersection(
	ON_Surface *surf1,
	ON_Surface *surf2,
	double s1,
	double s2,
	double t1,
	double t2,
	double stepsize,
	ON_BezierCurve *out
	)
{
    ON_Polyline intersectionPoints;
    double olds1, olds2, oldt1, oldt2;
    while (surf1->Domain(0).Includes(s1, true) && surf1->Domain(1).Includes(t1, true) && surf2->Domain(0).Includes(s2, true) && surf2->Domain(1).Includes(t2, true) && !intersectionPoints.IsClosed(stepsize)) {
	intersectionPoints.Append(surf1->PointAt(s1, t1));
	olds1 = s1;
	olds2 = s2;
	oldt1 = t1;
	oldt2 = t2;
	Step(surf1, surf2, &s1, &s2, &t1, &t2, stepsize);
    }
    if (!intersectionPoints.IsClosed(stepsize)) {
	/* we stepped off the edge of our domain, we need to get a point right on the edge */
	double news1, news2, newt1, newt2;
	news1 = ClosestValue(s1, surf1->Domain(0));
	news2 = ClosestValue(s2, surf2->Domain(1));
	newt1 = ClosestValue(t1, surf1->Domain(0));
	newt2 = ClosestValue(t2, surf2->Domain(1));
	news1 = olds1 + MIN((news1 - olds1) / (s1 - olds1), (newt1 - oldt1) / (t1 - oldt1)) * (s1 - olds1);
	news2 = olds2 + MIN((news2 - olds2) / (s2 - olds2), (newt2 - oldt2) / (t2 - oldt2)) * (s2 - olds2);
	newt1 = oldt1 + MIN((news1 - olds1) / (s1 - olds1), (newt1 - oldt1) / (t1 - oldt1)) * (t1 - oldt1);
	newt2 = oldt2 + MIN((news2 - olds2) / (s2 - olds2), (newt2 - oldt2) / (t2 - oldt2)) * (t2 - oldt2);
	double newstep = MIN(surf1->PointAt(olds1, oldt1).DistanceTo(surf1->PointAt(news1, newt1)), surf2->PointAt(olds2, oldt2).DistanceTo(surf2->PointAt(news2, newt2)));
	Step(surf1, surf2, &s1, &s2, &t1, &t2, newstep);
	intersectionPoints.Append(surf1->PointAt(s1, t1));
    }
    *out = ON_BezierCurve(intersectionPoints);
}

int main()
{
    return 0;
}

/** @} */
/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
