/*                         H R T . C
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
/** @addtogroup primitives */
/** @{ */
/** @file primitives/hrt/hrt.c
 *
 * Intersect a ray with a Heart
 *
 * Algorithm:
 *
 * Given V, X, Y, Z and d, there is a set of points on this heart
 *
 * { (x, y, z) | (x, y, z) is on heart defined by V, X, Y, Z and d }
 *
 * Through a series of Transformations, this set will be transformed
 * into a set of points on a heart centered at the origin
 * which lies on the X-Y plane (i.e., Z is on the Z axis).
 *
 * { (x', y', z') | (x', y', z') is on heart at origin }
 *
 * The transformation from X to X' is accomplished by:
 *
 * X' = S(R(X - V))
 *
 * where R(X) = (Z/(|X|))
 *		(Y/(|Y|)) . X
 *		(Z/(|Z|))
 *
 * and S(X) =	(1/|A|   0     0)
 *		(0    1/|A|   0) . X
 *		(0      0   1/|A|)
 * where |A| = d
 *
 * To find the intersection of a line with the heart, consider the
 * parametric line L:
 *
 * L : { P(n) | P + t(n) . D }
 *
 * Call W the actual point of intersection between L and the heart.
 * Let W' be the point of intersection between L' and the heart.
 *
 * L' : { P'(n) | P' + t(n) . D' }
 *
 * W = invR(invS(W')) + V
 *
 * Where W' = k D' + P'.
 *
 * Given a line, find the equation of the
 * heart in terms of the variable 't'.
 *
 * The equation for the heart is:
 *
 *    [ X**2 + 9/4*Y**2 + Z**2 - 1 ]**3 - Y**3 * (X**2 + 9/80 * Z**3)  =  0
 *
 * First, find X, Y, and Z in terms of 't' for this line, then
 * substitute them into the equation above.
 *
 *    Wx = Dx*t + Px
 *
 *    Wx**3 = Dx**3 * t**3 + 3 * Dx**2 * Px * t**2 + 3 * Dx * Px**2 * t + Px**3
 *
 * The real roots of the equation in 't' are the intersect points
 * along the parametric line.
 *
 * NORMALS.  Given the point W on the heart, what is the vector normal
 * to the tangent plane at that point?
 *
 * Map W onto the heart, i.e.: W' = S(R(W - V)).  In this case,
 * we find W' by solving the parametric line given k.
 *
 * The gradient of the heart at W' is in fact the normal vector.
 *
 * Given that the sextic equation for the heart is:
 *
 *    [ X**2 + 9/4 * Y**2 + Z**2 - 1 ]**3 - Y**3 * (X**2 + 9/80 * Z**3)  =  0.
 *
 * let w =  X**2 + 9/4*Y**2 + Z**2 - 1 , then the sextic equation becomes:
 *
 *    w**3 - Y**3 * (X**2 + 9/80 * Y**2)  =  0.
 *
 * For f(x, y, z) = 0, the gradient of f() is (df/dx, df/dy, df/dz).
 *
 *    df/dx = 6 * x * (w**2 - y**3)
 *    df/dy = 6 * (12/27 * w**2 * y**2 - 1/2 * y**2 * (x**2 + 9 / 80*z**3))
 *    df/dz = 6 * (w**2 * z - 160 / 9 * y**3 * z**2)
 *
 * Note that the normal vector produced above will not have
 * length.  Also, to make this useful for the original heart, it will
 * have to be rotated back to the orientation of the original heart.
 *
 */
/** @} */

#include "common.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "bio.h"

#include "vmath.h"

#include "db.h"
#include "nmg.h"
#include "rtgeom.h"
#include "raytrace.h"

#include "../../librt_private.h"


const struct bu_structparse rt_hrt_parse[] = {
    { "%f", 3, "V", bu_offsetofarray(struct rt_hrt_internal, v, fastf_t, X), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { "%f", 3, "X", bu_offsetofarray(struct rt_hrt_internal, xdir, fastf_t, X), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { "%f", 3, "Y", bu_offsetofarray(struct rt_hrt_internal, ydir, fastf_t, X), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { "%f", 3, "Z", bu_offsetofarray(struct rt_hrt_internal, zdir, fastf_t, X), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { "%g", 1, "d", bu_offsetof(struct rt_hrt_internal, d), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { {'\0', '\0', '\0', '\0'}, 0, (char *)NULL, 0, BU_STRUCTPARSE_FUNC_NULL, NULL, NULL }
};


struct hrt_specific {
    vect_t hrt_V; /* Vector to center of heart */
    vect_t hrt_X; /* unit-length X vector */
    vect_t hrt_Y; /* unit-length Y vector */
    vect_t hrt_Z; /* unit-length Z vector */
    fastf_t hrt_d; /* for distance to upper and lower cusps */
    vect_t hrt_invsq; /* [ 1.0 / |X|**2, 1.0 / |Y|**2, 1.0 / |Z|**2 ] */
    mat_t hrt_SoR; /* Scale(Rot(vect)) */
    mat_t hrt_invRSSR; /* invRot(Scale(Scale(vect))) */
};


/**
 * R T _ H R T _ B B O X
 *
 * Compute the bounding RPP for a heart.
 */
int
rt_hrt_bbox(const struct bn_tol *UNUSED(tol))
{
    bu_log("rt_hrt_bbox: Not implemented yet!\n");
    return 1;
}


/**
 * R T _ H R T _ P R E P
 *
 * Given a pointer to a GED database record, and a transformation
 * matrix, determine if this is a valid heart, and if so, precompute
 * various terms of the formula.
 *
 * Returns -
 * 0 HRT is OK
 * !0 Error in description
 *
 * Implicit return -
 * A struct hrt_specific is created, and its address is stored in
 * stp->st_specific for use by rt_hrt_shot().
 */
int
rt_hrt_prep()
{
    bu_log("rt_hrt_prep: Not implemented yet!\n");
    return 0;
}


/**
 * R T _ H R T _ P R I N T
 */
void
rt_hrt_print()
{
    bu_log("rt_hrt_print: Not implemented yet!\n");
}


/**
 * R T _ H R T _ S H O T
 *
 * Intersect a ray with a heart, where all constant terms have been
 * precomputed by rt_hrt_prep().  If an intersection occurs, one or
 * two struct seg(s) will be acquired and filled in.
 *
 * NOTE: All lines in this function are represented parametrically by
 * a point, P(x0, y0, z0) and a direction normal, D = ax + by + cz.
 * Any point on a line can be expressed by one variable 't', where
 *
 * X = a*t + x0,	eg, X = Dx*t + Px
 * Y = b*t + y0,
 * Z = c*t + z0.
 *
 * ^   ^     ^
 * |   |     |
 *
 * W = D*t + P
 *
 * First, convert the line to the coordinate system of a "standard"
 * heart.  This is a heart which lies in the X-Y plane, circles the
 * origin, and whose distance to cusps is one.
 *
 * Then find the equation of that line and the heart, which
 * turns out (by substituting X, Y, Z above into the sextic equation above)
 * to be a sextic equation S in 't' given below.
 *
 * S(t)=C6*t**6 + C5*t**5 + C4*t**4 + C3*t**3 + C2*t**2 + C1*t + C06 = 0.
 *
 * where C0, C1, C2, C3, C4, C5, C6 are coefficients of the equation.
 *
 * Solve the equation using a general polynomial root finder.
 * Use those values of 't' to compute the points of intersection
 * in the original coordinate system.
 *
 * Returns -
 * 0 MISS
 * >0 HIT
 */
int
rt_hrt_shot()
{
    bu_log("rt_hrt_shot: Not implemented yet!\n");
    return 6;
}


/**
 * R T _ H R T _ V S H O T
 *
 * This is the Becker vector version
 */
void
rt_hrt_vshot()
{
    bu_log("rt_hrt_vshot: Not implemented yet!\n");
}


/**
 * R T _ H R T _ N O R M
 *
 * Compute the normal to the heart, given a point on the heart
 * centered at the origin on the X-Y plane.  The gradient of the heart
 * at that point is in fact the normal vector, which will have to be
 * given unit length.  To make this useful for the original heart, it
 * will have to be rotated back to the orientation of the original
 * heart.
 *
 * Given that the equation for the heart is:
 *
 * [ X**2 + 9/4 * Y**2 + Z**2 - 1 ]**3 - Y**3 * (X**2 + 9/80*Z**3) = 0.
 *
 * let w = X**2 + 9/4 * Y**2 + Z**2 - 1, then the equation becomes:
 *
 * w**3 - Y**3 * (X**2 + 9/80 * Y**2)  =  0.
 *
 * For f(x, y, z) = 0, the gradient of f() is (df/dx, df/dy, df/dz).
 *
 * df/dx = 6 * x * (w**2 - y**3)
 * df/dy = 6 * (12/27 * w**2 * y**2 - 1/2 * y**2 * (x**2 + 9 / 80*z**3))
 * df/dz = 6 * (w**2 * z - 160 / 9 * y**3 * z**2)
 *
 * Since we rescale the gradient (normal) to unity, we divide the
 * above equations by six here.
 */
void
rt_hrt_norm()
{
    bu_log("rt_hrt_norm: Not implemented yet!\n");
}


/**
 * R T _ H R T _ C U R V E
 *
 * Return the curvature of the heart.
 */
void
rt_hrt_curve()
{
    bu_log("rt_hrt_curve: Not implemented yet!\n");
}


/**
 * R T _ H R T _ U V
 */
void
rt_hrt_uv()
{
    bu_log("rt_hrt_uv: Not implemented yet!\n");
}


/**
 * R T _ H R T _ F R E E
 */
void
rt_hrt_free()
{
    bu_log("rt_hrt_free: Not implemented yet!\n");
}


/**
 * R T _ H R T _ C L A S S
 */
int
rt_hrt_class(void)
{
    return 0;
}


/**
 * R T _ H R T _ A D A P T I V E _ P L O T
 */
int
rt_hrt_adaptive_plot()
{
    bu_log("rt_adaptive_plot: Not implemented yet!\n");
    return 0;
}


/**
 * R T _ H R T _ P L O T
 */
int
rt_hrt_plot(const struct rt_tess_tol *UNUSED(ttol), const struct bn_tol *UNUSED(tol), const struct rt_view_info *UNUSED(info))
{
    bu_log("rt_hrt_plot: Not implemented yet!\n");
    return 0;
}


/**
 * R T _ H R T _ T E S S
 */
int
rt_hrt_tess()
{
    bu_log("rt_hrt_tess: Not implemented yet!\n");
    return 0;
}


/**
 * R T _ H R T _ I M P O R T
 *
 * Import a heart from the database format to the internal format.
 * Apply modeling transformations at the same time.
 */
int
rt_hrt_import4()
{
    bu_log("rt_hrt_import4: Not implemented yet!\n");
    return -1;
}


/**
 * R T _ H R T _ E X P O R T 5
 */
int
rt_hrt_export5()
{
    bu_log("rt_hrt_export5: Not implemented yet!\n");
    return 0;
}


/**
 * R T _ H R T _ E X P O R T
 *
 */
int
rt_hrt_export4()
{
    bu_log("rt_hrt_export4: Not implemented yet!\n");
    return 0;
}


/**
 * R T _ H R T _ I M P O R T 5
 *
 * Import a HRT from the database format to the internal format.
 *
 */
int
rt_hrt_import5()
{
    bu_log("rt_hrt_import5: Not implemented yet!\n");
    return 0;
}


/**
 * R T _ H R T _ D E S C R I B E
 *
 * Make human-readable formatted presentation of this solid.  First
 * line describes type of solid.  Additional lines are indented one
 * tab, and give parameter values.
 */
int
rt_hrt_describe()
{
    bu_log("rt_hrt_describe: Not implemented yet!\n");
    return 0;
}


/**
 * R T _ H R T _ I F R E E
 *
 * Free the storage associated with the rt_db_internal version of this
 * solid.
 */
void
rt_hrt_ifree()
{
    bu_log("rt_hrt_ifree: Not implemented yet!\n");
}


/**
 * R T _ H R T _ P A R A M S
 *
 */
int
rt_hrt_params(struct pc_pc_set *UNUSED(ps))
{
    bu_log("rt_hrt_params: Not implemented yet!\n");
    return 0;
}


/**
 * R T _ H R T _ S U R F A C E _ A R E A
 *
 */
void
rt_hrt_surf_area()
{
    bu_log("rt_hrt_surf_area: Not implemented yet!\n");
}


/**
 * R T _ H R T _ V O L U M E
 *
 */
void
rt_hrt_volume()
{
    bu_log("rt_hrt_volume: Not implemented yet!\n");
}


/**
 * R T _ H R T _ C E N T R O I D
 *
 */
void
rt_hrt_centroid()
{
    bu_log("rt_hrt_centroid: Not implemented yet!\n");
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
