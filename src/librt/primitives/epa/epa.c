/*                           E P A . C
 * BRL-CAD
 *
 * Copyright (c) 1990-2012 United States Government as represented by
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
/** @file primitives/epa/epa.c
 *
 * Intersect a ray with an Elliptical Paraboloid.
 *
 * Algorithm -
 *
 * Given V, H, R, and B, there is a set of points on this epa
 *
 * { (x, y, z) | (x, y, z) is on epa }
 *
 * Through a series of Affine Transformations, this set of points will
 * be transformed into a set of points on an epa located at the origin
 * with a semi-major axis R1 along the +Y axis, a semi-minor axis R2
 * along the -X axis, a height H along the -Z axis, and a vertex V at
 * the origin.
 *
 *
 * { (x', y', z') | (x', y', z') is on epa at origin }
 *
 * The transformation from X to X' is accomplished by:
 *
 * X' = S(R(X - V))
 *
 * where R(X) = (R1/(-|R1|))
 *  		(R2/(|R2|)) . X
 * 		(H /(-|H |))
 *
 * and S(X) =	(1/|R1|   0     0)
 *  		(0    1/|R2|   0) . X
 * 		(0      0   1/|H |)
 *
 * To find the intersection of a line with the surface of the epa,
 * consider the parametric line L:
 *
 * L : { P(n) | P + t(n) . D }
 *
 * Call W the actual point of intersection between L and the epa.  Let
 * W' be the point of intersection between L' and the unit epa.
 *
 * L' : { P'(n) | P' + t(n) . D' }
 *
 * W = invR(invS(W')) + V
 *
 * Where W' = k D' + P'.
 *
 * If Dy' and Dz' are both 0, then there is no hit on the epa; but the
 * end plates need checking.  If there is now only 1 hit point, the
 * top plate needs to be checked as well.
 *
 * Line L' hits the infinitely long epa at W' when
 *
 * A * k**2 + B * k + C = 0
 *
 * where
 *
 * A = Dx'**2 + Dy'**2
 * B = 2 * (Dx' * Px' + Dy' * Py') - Dz'
 * C = Px'**2 + Py'**2 - Pz' - 1
 * b = |Breadth| = 1.0
 * h = |Height| = 1.0
 * r = 1.0
 *
 * The quadratic formula yields k (which is constant):
 *
 * k = [ -B +/- sqrt(B**2 - 4 * A * C)] / (2.0 * A)
 *
 * Now, D' = S(R(D))
 * and  P' = S(R(P - V))
 *
 * Substituting,
 *
 * W = V + invR(invS[ k *(S(R(D))) + S(R(P - V)) ])
 *   = V + invR((k * R(D)) + R(P - V))
 *   = V + k * D + P - V
 *   = k * D + P
 *
 * Note that ``k'' is constant, and is the same in the formulations
 * for both W and W'.
 *
 * The hit at ``k'' is a hit on the canonical epa IFF Wz' <= 0.
 *
 * NORMALS.  Given the point W on the surface of the epa, what is the
 * vector normal to the tangent plane at that point?
 *
 * Map W onto the unit epa, ie:  W' = S(R(W - V)).
 *
 * Plane on unit epa at W' has a normal vector N' where
 *
 * N' = <Wx', Wy', -.5>.
 *
 * The plane transforms back to the tangent plane at W, and this new
 * plane (on the original epa) has a normal vector of N, viz:
 *
 * N = inverse[ transpose(inverse[ S o R ]) ] (N')
 *
 * because if H is perpendicular to plane Q, and matrix M maps from Q
 * to Q', then inverse[ transpose(M) ] (H) is perpendicular to Q'.
 * Here, H and Q are in "prime space" with the unit sphere.  [Somehow,
 * the notation here is backwards].  So, the mapping matrix M =
 * inverse(S o R), because S o R maps from normal space to the unit
 * sphere.
 *
 * N = inverse[ transpose(inverse[ S o R ]) ] (N')
 *   = inverse[ transpose(invR o invS) ] (N')
 *   = inverse[ transpose(invS) o transpose(invR) ] (N')
 *   = inverse[ inverse(S) o R ] (N')
 *   = invR o S (N')
 *
 * because inverse(R) = transpose(R), so R = transpose(invR),
 * and S = transpose(S).
 *
 * Note that the normal vector produced above will not have unit
 * length.
 *
 * THE TOP PLATE.
 *
 * If Dz' == 0, line L' is parallel to the top plate, so there is no
 * hit on the top plate.  Otherwise, rays intersect the top plate with
 * k = (0 - Pz')/Dz'.  The solution is within the top plate IFF Wx'**2
 * + Wy'**2 <= 1.
 *
 * The normal for a hit on the top plate is -Hunit.
 *
 */

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


struct epa_specific {
    point_t epa_V;		/* vector to epa origin */
    vect_t epa_Hunit;		/* unit H vector */
    vect_t epa_Aunit;		/* unit vector along semi-major axis */
    vect_t epa_Bunit;		/* unit vector, A x H */
    fastf_t epa_h;		/* |H| */
    fastf_t epa_inv_r1sq;	/* 1/(r1 * r1) */
    fastf_t epa_inv_r2sq;	/* 1/(r2 * r2) */
    mat_t epa_SoR;		/* Scale(Rot(vect)) */
    mat_t epa_invRoS;		/* invRot(Scale(vect)) */
};


const struct bu_structparse rt_epa_parse[] = {
    { "%f", 3, "V",   bu_offsetof(struct rt_epa_internal, epa_V[X]),  BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { "%f", 3, "H",   bu_offsetof(struct rt_epa_internal, epa_H[X]),  BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { "%f", 3, "A",   bu_offsetof(struct rt_epa_internal, epa_Au[X]), BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { "%f", 1, "r_1", bu_offsetof(struct rt_epa_internal, epa_r1),    BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { "%f", 1, "r_2", bu_offsetof(struct rt_epa_internal, epa_r2),    BU_STRUCTPARSE_FUNC_NULL, NULL, NULL },
    { {'\0', '\0', '\0', '\0'}, 0, (char *)NULL, 0, BU_STRUCTPARSE_FUNC_NULL, NULL, NULL }
};

/**
 * R T _ E P A _ B B O X
 *
 * Create a bounding RPP for an epa
 */
int
rt_epa_bbox(struct rt_db_internal *ip, point_t *min, point_t *max, const struct bn_tol *UNUSED(tol)) {
    struct rt_epa_internal *xip;
    vect_t epa_A, epa_B, epa_An, epa_Bn, epa_H;
    vect_t pt1, pt2, pt3, pt4, pt5, pt6, pt7, pt8;
    RT_CK_DB_INTERNAL(ip);
    xip = (struct rt_epa_internal *)ip->idb_ptr;
    RT_EPA_CK_MAGIC(xip);

    VMOVE(epa_H, xip->epa_H);
    VUNITIZE(epa_H);
    VMOVE(epa_A, xip->epa_Au);
    VCROSS(epa_B, epa_A, epa_H);

    VSETALL((*min), MAX_FASTF);
    VSETALL((*max), -MAX_FASTF);

    VSCALE(epa_A, epa_A, xip->epa_r1);
    VSCALE(epa_B, epa_B, xip->epa_r2);
    VREVERSE(epa_An, epa_A);
    VREVERSE(epa_Bn, epa_B);

    VADD3(pt1, xip->epa_V, epa_A, epa_B);
    VADD3(pt2, xip->epa_V, epa_A, epa_Bn);
    VADD3(pt3, xip->epa_V, epa_An, epa_B);
    VADD3(pt4, xip->epa_V, epa_An, epa_Bn);
    VADD4(pt5, xip->epa_V, epa_A, epa_B, xip->epa_H);
    VADD4(pt6, xip->epa_V, epa_A, epa_Bn, xip->epa_H);
    VADD4(pt7, xip->epa_V, epa_An, epa_B, xip->epa_H);
    VADD4(pt8, xip->epa_V, epa_An, epa_Bn, xip->epa_H);

    /* Find the RPP of the rotated axis-aligned epa bbox - that is,
     * the bounding box the given epa would have if its height
     * vector were in the positive Z direction. This does not give
     * us an optimal bbox except in the case where the epa is
     * actually axis aligned to start with, but it's usually
     * at least a bit better than the bounding sphere RPP. */
    VMINMAX((*min), (*max), pt1);
    VMINMAX((*min), (*max), pt2);
    VMINMAX((*min), (*max), pt3);
    VMINMAX((*min), (*max), pt4);
    VMINMAX((*min), (*max), pt5);
    VMINMAX((*min), (*max), pt6);
    VMINMAX((*min), (*max), pt7);
    VMINMAX((*min), (*max), pt8);
    return 0;
}


/**
 * R T _ E P A _ P R E P
 *
 * Given a pointer to a GED database record, and a transformation
 * matrix, determine if this is a valid EPA, and if so, precompute
 * various terms of the formula.
 *
 * Returns -
 * 0 EPA is OK
 * !0 Error in description
 *
 * Implicit return -
 * A struct epa_specific is created, and its address is stored in
 * stp->st_specific for use by epa_shot().
 */
int
rt_epa_prep(struct soltab *stp, struct rt_db_internal *ip, struct rt_i *rtip)
{
    struct rt_epa_internal *xip;
    struct epa_specific *epa;

    fastf_t magsq_h;
    fastf_t mag_a, mag_h;
    fastf_t f, r1, r2;
    mat_t R;
    mat_t Rinv;
    mat_t S;

    RT_CK_DB_INTERNAL(ip);

    xip = (struct rt_epa_internal *)ip->idb_ptr;
    RT_EPA_CK_MAGIC(xip);

    /* compute |A| |H| */
    mag_a = sqrt(MAGSQ(xip->epa_Au));
    mag_h = sqrt(magsq_h = MAGSQ(xip->epa_H));
    r1 = xip->epa_r1;
    r2 = xip->epa_r2;
    /* Check for |H| > 0, |A| == 1, r1 > 0, r2 > 0 */
    if (NEAR_ZERO(mag_h, RT_LEN_TOL)
	|| !NEAR_EQUAL(mag_a, 1.0, RT_LEN_TOL)
	|| r1 < 0.0 || r2 < 0.0) {
	return 1;		/* BAD, too small */
    }

    /* Check for A.H == 0 */
    f = VDOT(xip->epa_Au, xip->epa_H) / mag_h;
    if (! NEAR_ZERO(f, RT_DOT_TOL)) {
	return 1;		/* BAD */
    }

    /*
     * EPA is ok
     */
    stp->st_id = ID_EPA;	/* set soltab ID */
    stp->st_meth = &rt_functab[ID_EPA];

    BU_GET(epa, struct epa_specific);
    stp->st_specific = (genptr_t)epa;

    epa->epa_h = mag_h;
    epa->epa_inv_r1sq = 1 / (r1 * r1);
    epa->epa_inv_r2sq = 1 / (r2 * r2);

    /* make unit vectors in A, H, and BxH directions */
    VMOVE(epa->epa_Hunit, xip->epa_H);
    VUNITIZE(epa->epa_Hunit);
    VMOVE(epa->epa_Aunit, xip->epa_Au);
    VCROSS(epa->epa_Bunit, epa->epa_Aunit, epa->epa_Hunit);

    VMOVE(epa->epa_V, xip->epa_V);

    /* Compute R and Rinv matrices */
    MAT_IDN(R);
    VREVERSE(&R[0], epa->epa_Bunit);
    VMOVE(&R[4], epa->epa_Aunit);
    VREVERSE(&R[8], epa->epa_Hunit);
    bn_mat_trn(Rinv, R);			/* inv of rot mat is trn */

    /* Compute S */
    MAT_IDN(S);
    S[ 0] = 1.0/r2;
    S[ 5] = 1.0/r1;
    S[10] = 1.0/mag_h;

    /* Compute SoR and invRoS */
    bn_mat_mul(epa->epa_SoR, S, R);
    bn_mat_mul(epa->epa_invRoS, Rinv, S);

    /* Compute bounding sphere */
    /* bounding sphere center */
    VJOIN1(stp->st_center, epa->epa_V, mag_h / 2.0, epa->epa_Hunit);
    /* bounding radius */
    stp->st_bradius = sqrt(0.25*magsq_h + r2*r2 + r1*r1);
    /* approximate bounding radius */
    stp->st_aradius = stp->st_bradius;

    /* Calcuate bounding box (RPP) */
    if (rt_epa_bbox(ip, &(stp->st_min), &(stp->st_max), &rtip->rti_tol)) return 1;

    return 0;			/* OK */
}


/**
 * R T _ E P A _ P R I N T
 */
void
rt_epa_print(const struct soltab *stp)
{
    const struct epa_specific *epa =
	(struct epa_specific *)stp->st_specific;

    VPRINT("V", epa->epa_V);
    VPRINT("Hunit", epa->epa_Hunit);
    VPRINT("Aunit", epa->epa_Aunit);
    VPRINT("Bunit", epa->epa_Bunit);
    bn_mat_print("S o R", epa->epa_SoR);
    bn_mat_print("invR o S", epa->epa_invRoS);
}


/* hit_surfno is set to one of these */
#define EPA_NORM_BODY (1)		/* compute normal */
#define EPA_NORM_TOP (2)		/* copy epa_N */


/**
 * R T _ E P A _ S H O T
 *
 * Intersect a ray with a epa.  If an intersection occurs, a struct
 * seg will be acquired and filled in.
 *
 * Returns -
 * 0 MISS
 * >0 HIT
 */
int
rt_epa_shot(struct soltab *stp, struct xray *rp, struct application *ap, struct seg *seghead)
{
    struct epa_specific *epa =
	(struct epa_specific *)stp->st_specific;
    vect_t dprime;		/* D' */
    vect_t pprime;		/* P' */
    fastf_t k1, k2;		/* distance constants of solution */
    vect_t xlated;		/* translated vector */
    struct hit hits[3];		/* 2 potential hit points */
    struct hit *hitp;	/* pointer to hit point */

    hitp = &hits[0];

    /* out, Mat, vect */
    MAT4X3VEC(dprime, epa->epa_SoR, rp->r_dir);
    VSUB2(xlated, rp->r_pt, epa->epa_V);
    MAT4X3VEC(pprime, epa->epa_SoR, xlated);

    /* Find roots of the equation, using formula for quadratic */
    {
	fastf_t a, b, c;	/* coeffs of polynomial */
	fastf_t disc;		/* disc of radical */

	a = dprime[X] * dprime[X] + dprime[Y] * dprime[Y];
	b = 2*(dprime[X] * pprime[X] + dprime[Y] * pprime[Y])
	    - dprime[Z];
	c = pprime[X] * pprime[X]
	    + pprime[Y] * pprime[Y] - pprime[Z] - 1.0;
	if (!NEAR_ZERO(a, RT_PCOEF_TOL)) {
	    disc = b*b - 4 * a * c;
	    if (disc <= 0)
		goto check_plates;
	    disc = sqrt(disc);

	    k1 = (-b + disc) / (2.0 * a);
	    k2 = (-b - disc) / (2.0 * a);

	    /* k1 and k2 are potential solutions to intersection with
	     * side.  See if they fall in range.
	     */
	    VJOIN1(hitp->hit_vpriv, pprime, k1, dprime);	/* hit' */
	    if (hitp->hit_vpriv[Z] <= 0.0) {
		hitp->hit_magic = RT_HIT_MAGIC;
		hitp->hit_dist = k1;
		hitp->hit_surfno = EPA_NORM_BODY;	/* compute N */
		hitp++;
	    }

	    VJOIN1(hitp->hit_vpriv, pprime, k2, dprime);	/* hit' */
	    if (hitp->hit_vpriv[Z] <= 0.0) {
		hitp->hit_magic = RT_HIT_MAGIC;
		hitp->hit_dist = k2;
		hitp->hit_surfno = EPA_NORM_BODY;	/* compute N */
		hitp++;
	    }
	} else if (!NEAR_ZERO(b, RT_PCOEF_TOL)) {
	    k1 = -c/b;
	    VJOIN1(hitp->hit_vpriv, pprime, k1, dprime);	/* hit' */
	    if (hitp->hit_vpriv[Z] <= 0.0) {
		hitp->hit_magic = RT_HIT_MAGIC;
		hitp->hit_dist = k1;
		hitp->hit_surfno = EPA_NORM_BODY;	/* compute N */
		hitp++;
	    }
	}
    }


    /*
     * Check for hitting the top plate.
     */
 check_plates:
    /* check top plate */
    if (hitp == &hits[1]  &&  !ZERO(dprime[Z])) {
	/* 1 hit so far, this is worthwhile */
	k1 = -pprime[Z] / dprime[Z];		/* top plate */

	VJOIN1(hitp->hit_vpriv, pprime, k1, dprime);	/* hit' */
	if (hitp->hit_vpriv[X] * hitp->hit_vpriv[X] +
	    hitp->hit_vpriv[Y] * hitp->hit_vpriv[Y] <= 1.0) {
	    hitp->hit_magic = RT_HIT_MAGIC;
	    hitp->hit_dist = k1;
	    hitp->hit_surfno = EPA_NORM_TOP;	/* -H */
	    hitp++;
	}
    }

    if (hitp != &hits[2])
	return 0;	/* MISS */

    if (hits[0].hit_dist < hits[1].hit_dist) {
	/* entry is [0], exit is [1] */
	struct seg *segp;

	RT_GET_SEG(segp, ap->a_resource);
	segp->seg_stp = stp;
	segp->seg_in = hits[0];		/* struct copy */
	segp->seg_out = hits[1];	/* struct copy */
	BU_LIST_INSERT(&(seghead->l), &(segp->l));
    } else {
	/* entry is [1], exit is [0] */
	struct seg *segp;

	RT_GET_SEG(segp, ap->a_resource);
	segp->seg_stp = stp;
	segp->seg_in = hits[1];		/* struct copy */
	segp->seg_out = hits[0];	/* struct copy */
	BU_LIST_INSERT(&(seghead->l), &(segp->l));
    }
    return 2;			/* HIT */
}


/**
 * R T _ E P A _ N O R M
 *
 * Given ONE ray distance, return the normal and entry/exit point.
 */
void
rt_epa_norm(struct hit *hitp, struct soltab *stp, struct xray *rp)
{
    fastf_t scale;
    vect_t can_normal;	/* normal to canonical epa */
    struct epa_specific *epa =
	(struct epa_specific *)stp->st_specific;

    VJOIN1(hitp->hit_point, rp->r_pt, hitp->hit_dist, rp->r_dir);
    switch (hitp->hit_surfno) {
	case EPA_NORM_BODY:
	    VSET(can_normal,
		 hitp->hit_vpriv[X],
		 hitp->hit_vpriv[Y],
		 -0.5);
	    MAT4X3VEC(hitp->hit_normal, epa->epa_invRoS, can_normal);
	    scale = 1.0 / MAGNITUDE(hitp->hit_normal);
	    VSCALE(hitp->hit_normal, hitp->hit_normal, scale);

	    /* tuck away this scale for the curvature routine */
	    hitp->hit_vpriv[X] = scale;
	    break;
	case EPA_NORM_TOP:
	    VREVERSE(hitp->hit_normal, epa->epa_Hunit);
	    break;
	default:
	    bu_log("rt_epa_norm: surfno=%d bad\n", hitp->hit_surfno);
	    break;
    }
}


/**
 * R T _ E P A _ C U R V E
 *
 * Return the curvature of the epa.
 */
void
rt_epa_curve(struct curvature *cvp, struct hit *hitp, struct soltab *stp)
{
    fastf_t a, b, c, scale;
    mat_t M1, M2;
    struct epa_specific *epa =
	(struct epa_specific *)stp->st_specific;
    vect_t u, v;		/* basis vectors (with normal) */
    vect_t vec1, vec2;		/* eigen vectors */
    vect_t tmp;

    switch (hitp->hit_surfno) {
	case EPA_NORM_BODY:
	    /* choose a tangent plane coordinate system (u, v, normal)
	     * form a right-handed triple
	     */
	    bn_vec_ortho(u, hitp->hit_normal);
	    VCROSS(v, hitp->hit_normal, u);

	    /* get the saved away scale factor */
	    scale = - hitp->hit_vpriv[X];

	    MAT_IDN(M1);
	    M1[10] = 0;	/* M1[3, 3] = 0 */
	    /* M1 = invR * S * M1 * S * R */
	    bn_mat_mul(M2, epa->epa_invRoS, M1);
	    bn_mat_mul(M1, M2, epa->epa_SoR);

	    /* find the second fundamental form */
	    MAT4X3VEC(tmp, M1, u);
	    a = VDOT(u, tmp) * scale;
	    b = VDOT(v, tmp) * scale;
	    MAT4X3VEC(tmp, M1, v);
	    c = VDOT(v, tmp) * scale;

	    bn_eigen2x2(&cvp->crv_c1, &cvp->crv_c2, vec1, vec2, a, b, c);
	    VCOMB2(cvp->crv_pdir, vec1[X], u, vec1[Y], v);
	    VUNITIZE(cvp->crv_pdir);
	    break;
	case EPA_NORM_TOP:
	    cvp->crv_c1 = cvp->crv_c2 = 0;
	    /* any tangent direction */
	    bn_vec_ortho(cvp->crv_pdir, hitp->hit_normal);
	    break;
    }
}


/**
 * R T _ E P A _ U V
 *
 * For a hit on the surface of an epa, return the (u, v) coordinates
 * of the hit point, 0 <= u, v <= 1.
 *
 * u = azimuth
 * v = elevation
 */
void
rt_epa_uv(struct application *ap, struct soltab *stp, struct hit *hitp, struct uvcoord *uvp)
{
    struct epa_specific *epa =
	(struct epa_specific *)stp->st_specific;

    vect_t work;
    vect_t pprime;
    fastf_t len;

    if (ap) RT_CK_APPLICATION(ap);

    /* hit_point is on surface; project back to unit epa, creating a
     * vector from vertex to hit point.
     */
    VSUB2(work, hitp->hit_point, epa->epa_V);
    MAT4X3VEC(pprime, epa->epa_SoR, work);

    switch (hitp->hit_surfno) {
	case EPA_NORM_BODY:
	    /* top plate, polar coords */
	    if (ZERO(pprime[Z] + 1.0)) { /* i.e., == -1.0 */
		/* bottom pt of body */
		uvp->uv_u = 0;
	    } else {
		len = sqrt(pprime[X]*pprime[X] + pprime[Y]*pprime[Y]);
		uvp->uv_u = acos(pprime[X]/len) * bn_inv2pi;
	    }
	    uvp->uv_v = -pprime[Z];
	    break;
	case EPA_NORM_TOP:
	    /* top plate, polar coords */
	    len = sqrt(pprime[X]*pprime[X] + pprime[Y]*pprime[Y]);
	    uvp->uv_u = acos(pprime[X]/len) * bn_inv2pi;
	    uvp->uv_v = 1.0 - len;
	    break;
    }
    /* Handle other half of acos() domain */
    if (pprime[Y] < 0)
	uvp->uv_u = 1.0 - uvp->uv_u;

    /* uv_du should be relative to rotation, uv_dv relative to height */
    uvp->uv_du = uvp->uv_dv = 0;
}


/**
 * R T _ E P A _ F R E E
 */
void
rt_epa_free(struct soltab *stp)
{
    struct epa_specific *epa =
	(struct epa_specific *)stp->st_specific;


    bu_free((char *)epa, "epa_specific");
}


/**
 * R T _ E P A _ C L A S S
 */
int
rt_epa_class(void)
{
    return 0;
}

/* Assume the existence of a line and a parabola at the origin, both in the y-z
 * plane (x = 0.0). The parabola and line are described by arguments p and m,
 * from the respective equations (z = y^2 / 4p) and (z = my + b).
 *
 * The line is assumed to intersect the parabola at the origin and one other
 * point (i.e. m > 0, m < infinity).
 *
 * The portion of the parabola between these two intersection points takes on a
 * single maximum value with respect to the intersecting line when its slope is
 * the same as the line's (i.e. when the tangent line and intersecting line are
 * parallel).
 *
 * The first derivate slope equation z' = y / 2p implies that the parabola has
 * slope m when y = 2pm. So we calculate y from p and m, and then z from y.
 */
static void
parabola_point_farthest_from_line(point_t max_point, fastf_t p, fastf_t m)
{
    fastf_t y = 2.0 * p * m;

    max_point[X] = 0.0;
    max_point[Y] = y;
    max_point[Z] = (y * y) / ( 4.0 * p);
}

/* Calculate the length of the shortest distance between a point and a line in
 * the y-z plane.
 */
static fastf_t
distance_point_to_line(point_t p, fastf_t slope, fastf_t intercept)
{
    /* input line:
     *   z = slope * y + intercept
     *
     * implicit form is:
     *   az + by + c = 0,  where a = -slope, b = 1, c = -intercept
     *
     * standard 2D point-line distance calculation:
     *   d = |aPy + bPz + c| / sqrt(a^2 + b^2)
     */
    return fabs(-slope * p[Y] + p[Z] - intercept) / sqrt(slope * slope + 1);
}

/* Approximate part of the parabola (y - h)^2 = 4p(z - k) lying in the y-z
 * plane.
 *
 * pts->p: the vertex (0, h, k)
 * pts->next->p: another point on the parabola
 * pts->next->next: NULL
 * p: the constant from the above equation
 *
 * This routine inserts num_new_points points between the two input points to
 * better approximate the parabolic curve passing between them.
 *
 * Returns number of points successfully added.
 */
static int
approximate_parabolic_curve(struct rt_pt_node *pts, fastf_t p, int num_new_points)
{
    fastf_t error, max_error, seg_slope, seg_intercept;
    point_t v, point, p0, p1, new_point = VINIT_ZERO;
    struct rt_pt_node *node, *worst_node, *new_node;
    int i;

    if (pts == NULL || pts->next == NULL || num_new_points < 1) {
	return 0;
    }

    VMOVE(v, pts->p);

    for (i = 0; i < num_new_points; ++i) {
	worst_node = node = pts;
	max_error = 0.0;

	/* Find the least accurate line segment, and calculate a new parabola
	 * point to insert between it's endpoints.
	 */
	while (node->next != NULL) {
	    VMOVE(p0, node->p);
	    VMOVE(p1, node->next->p);

	    seg_slope = (p1[Z] - p0[Z]) / (p1[Y] - p0[Y]);
	    seg_intercept = p0[Z] - seg_slope * p0[Y];

	    /* get farthest point on the equivalent parabola at the origin */
	    parabola_point_farthest_from_line(point, p, seg_slope);

	    /* translate result to actual parabola position */
	    point[Y] += v[Y];
	    point[Z] += v[Z];

	    /* see if the maximum distance between the parabola and the line
	     * (the error) is larger than the largest recorded error
	     * */
	    error = distance_point_to_line(point, seg_slope, seg_intercept);

	    if (error > max_error) {
		max_error = error;
		worst_node = node;
		VMOVE(new_point, point);
	    }

	    node = node->next;
	}

	/* insert new point between endpoints of the least accurate segment */
	new_node = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node),
		    "rt_pt_node");
	VMOVE(new_node->p, new_point);
	new_node->next = worst_node->next;
	worst_node->next = new_node;
    }

    return num_new_points;
}

static struct rt_pt_node *
epa_parabolic_curve(fastf_t mag_h, fastf_t r, int num_points)
{
    int count;
    struct rt_pt_node *curve;

    curve = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");
    curve->next = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");

    curve->next->next = NULL;
    VSET(curve->p,       0, 0, -mag_h);
    VSET(curve->next->p, 0, r, 0);

    count = approximate_parabolic_curve(curve, (r * r) / (4 * mag_h), num_points - 2);

    if (count != (num_points - 2)) {
	return NULL;
    }

    return curve;
}

/* Plot the elliptical cross section of the given epa at distance h along the
 * epa height vector (h >= 0, h <= |H|) consisting of num_points points.
 */
static void
epa_plot_ellipse(struct bu_list *vhead, struct rt_epa_internal *epa, fastf_t h, fastf_t num_points)
{
    int i;
    point_t p;
    vect_t V, Hu, Au, Bu, A, B, cross_section_plane;
    fastf_t mag_H, rad, radian_step;

    VMOVE(V, epa->epa_V);

    mag_H = MAGNITUDE(epa->epa_H);
    VSCALE(Hu, epa->epa_H, 1.0 / mag_H);

    VMOVE(Au, epa->epa_Au);
    VCROSS(Bu, Au, Hu);

    /* calculate semi-major and semi-minor axis for the elliptical
     * cross-section at distance h along H
     */
    VSCALE(A, Au, epa->epa_r1 * sqrt(1 - h / mag_H));
    VSCALE(B, Bu, epa->epa_r2 * sqrt(1 - h / mag_H));

    VJOIN1(cross_section_plane, V, h, Hu);
    radian_step = bn_twopi / num_points;

    rad = radian_step * (num_points - 1);
    VJOIN2(p, cross_section_plane, cos(rad), A, sin(rad), B);
    RT_ADD_VLIST(vhead, p, BN_VLIST_LINE_MOVE);

    rad = 0;
    for (i = 0; i < num_points; ++i) {
	VJOIN2(p, cross_section_plane, cos(rad), A, sin(rad), B);
	RT_ADD_VLIST(vhead, p, BN_VLIST_LINE_DRAW);
	rad += radian_step;
    }
}

static void
epa_plot_parabola(
	struct bu_list *vhead,
	struct rt_epa_internal *epa,
	struct rt_pt_node *pts)
{
    point_t p;
    vect_t epa_V;
    fastf_t mag_H;
    struct rt_pt_node *node;

    VMOVE(epa_V, epa->epa_V);
    mag_H = MAGNITUDE(epa->epa_H);

    VADD2(p, pts->p, epa_V);
    RT_ADD_VLIST(vhead, p, BN_VLIST_LINE_MOVE);

    node = pts->next;
    while (node != NULL) {
	VADD2(p, node->p, epa_V);
	RT_ADD_VLIST(vhead, p, BN_VLIST_LINE_DRAW);

	node = node->next;
    }
}

int
rt_epa_adaptive_plot(struct rt_db_internal *ip, const struct rt_view_info *info)
{
    vect_t epa_V, epa_H, translate;
    fastf_t mag_H, z, r1, r2;
    int i, num_curve_points, num_ellipse_points;
    struct rt_epa_internal *epa;
    struct rt_pt_node *pts_r1, *pts_r2, *node1, *node2;

    num_curve_points = 4;
    num_ellipse_points = 16;

    BU_CK_LIST_HEAD(info->vhead);
    RT_CK_DB_INTERNAL(ip);
    epa = (struct rt_epa_internal *)ip->idb_ptr;
    RT_EPA_CK_MAGIC(epa);

    VMOVE(epa_V, epa->epa_V);
    VMOVE(epa_H, epa->epa_H);
    mag_H = MAGNITUDE(epa_H);
    VADD2(translate, epa_V, epa_H);

    r1 = epa->epa_r1;
    r2 = epa->epa_r2;

    pts_r1 = epa_parabolic_curve(mag_H, r1, num_curve_points);
    pts_r2 = epa_parabolic_curve(mag_H, r2, num_curve_points);

    if (pts_r1 == NULL || pts_r2 == NULL) {
	return -1;
    }

    node1 = pts_r1;
    node2 = pts_r2;
    for (i = 0; i < num_curve_points; ++i) {
	/* Select cross-section to draw by averaging the z values and flip over y-axis
	 * to get a distance along H.
	 */
	z = (node1->p[Z] + node2->p[Z]) / 2.0;
	epa_plot_ellipse(info->vhead, epa, -z, num_ellipse_points);

	/* recalculate parabola points for plotting */
	node1->p[X] = 0.0;
	node2->p[X] = r2 * sqrt(z / mag_H + 1);

	node1->p[Y] = r1 * sqrt(z / mag_H + 1);
	node2->p[Y] = 0.0;

	node1->p[Z] = node2->p[Z] = -z;

	node1 = node1->next;
	node2 = node2->next;
    }

    /* plot one half of each curve */
    epa_plot_parabola(info->vhead, epa, pts_r1);
    epa_plot_parabola(info->vhead, epa, pts_r2);

    /* plot the other half of each curve */
    node1 = pts_r1;
    node2 = pts_r2;
    for (i = 0; i < num_curve_points; ++i) {
	node2->p[X] *= -1.0;
	node1->p[Y] *= -1.0;

	node1 = node1->next;
	node2 = node2->next;
    }

    epa_plot_parabola(info->vhead, epa, pts_r1);
    epa_plot_parabola(info->vhead, epa, pts_r2);

    return 0;
}

/**
 * R T _ E P A _ P L O T
 */
int
rt_epa_plot(struct bu_list *vhead, struct rt_db_internal *ip, const struct rt_tess_tol *ttol, const struct bn_tol *UNUSED(tol), const struct rt_view_info *UNUSED(info))
{
    fastf_t dtol, f, mag_a, mag_h, ntol, r1, r2;
    fastf_t **ellipses, theta_new, theta_prev;
    int *pts_dbl, i, j, nseg;
    int jj, na, nb, nell, recalc_b;
    mat_t R;
    mat_t invR;
    struct rt_epa_internal *xip;
    point_t p1;
    struct rt_pt_node *pos_a, *pos_b, *pts_a, *pts_b;
    vect_t A, Au, B, Bu, Hu, V, Work;

    BU_CK_LIST_HEAD(vhead);
    RT_CK_DB_INTERNAL(ip);
    xip = (struct rt_epa_internal *)ip->idb_ptr;
    RT_EPA_CK_MAGIC(xip);

    /*
     * make sure epa description is valid
     */

    /* compute |A| |H| */
    mag_a = MAGSQ(xip->epa_Au);	/* should already be unit vector */
    mag_h = MAGNITUDE(xip->epa_H);
    r1 = xip->epa_r1;
    r2 = xip->epa_r2;
    /* Check for |H| > 0, |A| == 1, r1 > 0, r2 > 0 */
    if (NEAR_ZERO(mag_h, RT_LEN_TOL)
	|| !NEAR_EQUAL(mag_a, 1.0, RT_LEN_TOL)
	|| r1 <= 0.0 || r2 <= 0.0) {
	return -2;		/* BAD */
    }

    /* Check for A.H == 0 */
    f = VDOT(xip->epa_Au, xip->epa_H) / mag_h;
    if (! NEAR_ZERO(f, RT_DOT_TOL)) {
	return -2;		/* BAD */
    }

    /* make unit vectors in A, H, and BxH directions */
    VMOVE(Hu, xip->epa_H);
    VUNITIZE(Hu);
    VMOVE(Au, xip->epa_Au);
    VCROSS(Bu, Au, Hu);

    /* Compute R and Rinv matrices */
    MAT_IDN(R);
    VREVERSE(&R[0], Bu);
    VMOVE(&R[4], Au);
    VREVERSE(&R[8], Hu);
    bn_mat_trn(invR, R);			/* inv of rot mat is trn */

    dtol = primitive_get_absolute_tolerance(ttol, 2.0 * r2);

    /* To ensure normal tolerance, remain below this angle */
    if (ttol->norm > 0.0)
	ntol = ttol->norm;
    else
	/* tolerate everything */
	ntol = bn_pi;

    /*
     * build epa from 2 parabolas
     */

    /* approximate positive half of parabola along semi-minor axis */
    pts_b = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");
    pts_b->next = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");
    pts_b->next->next = NULL;
    VSET(pts_b->p,       0, 0, -mag_h);
    VSET(pts_b->next->p, 0, r2, 0);
    /* 2 endpoints in 1st approximation */
    nb = 2;

    /* recursively break segment 'til within error tolerances */
    nb += rt_mk_parabola(pts_b, r2, mag_h, dtol, ntol);
    nell = nb - 1;	/* # of ellipses needed */

    /* construct positive half of parabola along semi-major axis of
     * epa using same z coords as parab along semi-minor axis
     */
    pts_a = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");
    VMOVE(pts_a->p, pts_b->p);	/* 1st pt is the apex */
    pts_a->next = NULL;
    pos_b = pts_b->next;
    pos_a = pts_a;
    while (pos_b) {
	/* copy node from b_parabola to a_parabola */
	pos_a->next = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");
	pos_a = pos_a->next;
	pos_a->p[Z] = pos_b->p[Z];
	/* at given z, find y on parabola */
	pos_a->p[Y] = r1*sqrt(pos_a->p[Z] / mag_h + 1);
	pos_a->p[X] = 0;
	pos_b = pos_b->next;
    }
    pos_a->next = NULL;

    /* see if any segments need further breaking up */
    recalc_b = 0;
    na = 0;
    pos_a = pts_a;
    while (pos_a->next) {
	na = rt_mk_parabola(pos_a, r1, mag_h, dtol, ntol);
	if (na != 0) {
	    recalc_b = 1;
	    nell += na;
	}
	pos_a = pos_a->next;
    }
    /* if any were broken, recalculate parabola 'a' */
    if (recalc_b) {
	/* free mem for old approximation of parabola */
	pos_b = pts_b;
	while (pos_b) {
	    struct rt_pt_node *next;

	    /* get next node before freeing */
	    next = pos_b->next;
	    bu_free((char *)pos_b, "rt_pt_node");
	    pos_b = next;
	}
	/* construct parabola along semi-major axis of epa using same
	 * z coords as parab along semi-minor axis
	 */
	pts_b = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");
	pts_b->p[Z] = pts_a->p[Z];
	pts_b->next = NULL;
	pos_a = pts_a->next;
	pos_b = pts_b;
	while (pos_a) {
	    /* copy node from a_parabola to b_parabola */
	    pos_b->next = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");
	    pos_b = pos_b->next;
	    pos_b->p[Z] = pos_a->p[Z];
	    /* at given z, find y on parabola */
	    pos_b->p[Y] = r2*sqrt(pos_b->p[Z] / mag_h + 1);
	    pos_b->p[X] = 0;
	    pos_a = pos_a->next;
	}
	pos_b->next = NULL;
    }

    /* make array of ptrs to epa ellipses */
    ellipses = (fastf_t **)bu_malloc(nell * sizeof(fastf_t *), "fastf_t ell[]");
    /* keep track of whether pts in each ellipse are doubled or not */
    pts_dbl = (int *)bu_malloc(nell * sizeof(int), "dbl ints");

    /* make ellipses at each z level */
    i = 0;
    nseg = 0;
    theta_prev = bn_twopi;
    pos_a = pts_a->next;	/* skip over apex of epa */
    pos_b = pts_b->next;
    while (pos_a) {
	VSCALE(A, Au, pos_a->p[Y]);	/* semimajor axis */
	VSCALE(B, Bu, pos_b->p[Y]);	/* semiminor axis */
	VJOIN1(V, xip->epa_V, -pos_a->p[Z], Hu);

	VSET(p1, 0., pos_b->p[Y], 0.);
	theta_new = ell_angle(p1, pos_a->p[Y], pos_b->p[Y], dtol, ntol);
	if (nseg == 0) {
	    nseg = (int)(bn_twopi / theta_new) + 1;
	    pts_dbl[i] = 0;
	} else if (theta_new < theta_prev) {
	    nseg *= 2;
	    pts_dbl[i] = 1;
	} else
	    pts_dbl[i] = 0;
	theta_prev = theta_new;

	ellipses[i] = (fastf_t *)bu_malloc(3*(nseg+1)*sizeof(fastf_t),
					   "pts ell");
	rt_ell(ellipses[i], V, A, B, nseg);

	i++;
	pos_a = pos_a->next;
	pos_b = pos_b->next;
    }
    /* Draw the top ellipse */
    RT_ADD_VLIST(vhead,
		 &ellipses[nell-1][(nseg-1)*ELEMENTS_PER_VECT],
		 BN_VLIST_LINE_MOVE);
    for (i = 0; i < nseg; i++) {
	RT_ADD_VLIST(vhead,
		     &ellipses[nell-1][i*ELEMENTS_PER_VECT],
		     BN_VLIST_LINE_DRAW);
    }

    /* connect ellipses */
    for (i = nell-2; i >= 0; i--) {
	/* skip top ellipse */
	int bottom, top;

	top = i + 1;
	bottom = i;
	if (pts_dbl[top])
	    nseg /= 2;	/* # segs in 'bottom' ellipse */

	/* Draw the current ellipse */
	RT_ADD_VLIST(vhead,
		     &ellipses[bottom][(nseg-1)*ELEMENTS_PER_VECT],
		     BN_VLIST_LINE_MOVE);
	for (j = 0; j < nseg; j++) {
	    RT_ADD_VLIST(vhead,
			 &ellipses[bottom][j*ELEMENTS_PER_VECT],
			 BN_VLIST_LINE_DRAW);
	}

	/* make connections between ellipses */
	for (j = 0; j < nseg; j++) {
	    if (pts_dbl[top])
		jj = j + j;	/* top ellipse index */
	    else
		jj = j;
	    RT_ADD_VLIST(vhead,
			 &ellipses[bottom][j*ELEMENTS_PER_VECT],
			 BN_VLIST_LINE_MOVE);
	    RT_ADD_VLIST(vhead,
			 &ellipses[top][jj*ELEMENTS_PER_VECT],
			 BN_VLIST_LINE_DRAW);
	}
    }

    VADD2(Work, xip->epa_V, xip->epa_H);
    for (i = 0; i < nseg; i++) {
	/* Draw connector */
	RT_ADD_VLIST(vhead, Work, BN_VLIST_LINE_MOVE);
	RT_ADD_VLIST(vhead,
		     &ellipses[0][i*ELEMENTS_PER_VECT],
		     BN_VLIST_LINE_DRAW);
    }

    /* free mem */
    for (i = 0; i < nell; i++) {
	bu_free((char *)ellipses[i], "pts ell");
    }
    bu_free((char *)ellipses, "fastf_t ell[]");
    bu_free((char *)pts_dbl, "dbl ints");

    return 0;
}


#define ELLOUT(n) ov+(n-1)*3

void
rt_ell_norms(fastf_t *ov, fastf_t *A, fastf_t *B, fastf_t *h_vec, fastf_t t, int sides)
{
    fastf_t ang, theta, x, y, sqrt_1mt;
    int n;
    vect_t partial_t, partial_ang;

    sqrt_1mt = sqrt(1.0 - t);
    if (sqrt_1mt <= SMALL_FASTF)
	bu_bomb("rt_epa_tess: rt_ell_norms: sqrt(1.0 -t) is zero\n");
    theta = 2 * bn_pi / sides;
    ang = 0.;

    for (n = 1; n <= sides; n++, ang += theta) {
	x = cos(ang);
	y = sin(ang);
	VJOIN2(partial_t, h_vec, -x/(2.0*sqrt_1mt), A, -y/(2.0*sqrt_1mt), B);
	VBLEND2(partial_ang, x*sqrt_1mt, B, -y*sqrt_1mt, A);
	VCROSS(ELLOUT(n), partial_t, partial_ang);
	VUNITIZE(ELLOUT(n));
    }
    VMOVE(ELLOUT(n), ELLOUT(1));
}


/**
 * R T _ E L L
 *
 * Generate an ellipsoid with the specified number of sides approximating it.
 */
void
rt_ell(fastf_t *ov, const fastf_t *V, const fastf_t *A, const fastf_t *B, int sides)
{
    fastf_t ang, theta, x, y;
    int n;

    theta = 2 * bn_pi / sides;
    ang = 0.;
    /* make ellipse regardless of whether it meets req's */
    for (n = 1; n <= sides; n++, ang += theta) {
	x = cos(ang);
	y = sin(ang);
	VJOIN2(ELLOUT(n), V, x, A, y, B);
    }
    VMOVE(ELLOUT(n), ELLOUT(1));
}


/**
 * R T _ E P A _ T E S S
 *
 * Returns -
 * -1 failure
 * 0 OK.  *r points to nmgregion that holds this tessellation.
 */
int
rt_epa_tess(struct nmgregion **r, struct model *m, struct rt_db_internal *ip, const struct rt_tess_tol *ttol, const struct bn_tol *tol)
{
    fastf_t dtol, f, mag_a, mag_h, ntol, r1, r2;
    fastf_t **ellipses, **normals, theta_new, theta_prev;
    int *pts_dbl, face, i, j, nseg;
    int *segs_per_ell;
    int jj, na, nb, nell, recalc_b;
    mat_t R;
    mat_t invR;
    struct rt_epa_internal *xip;
    point_t p1;
    struct rt_pt_node *pos_a, *pos_b, *pts_a, *pts_b;
    struct shell *s;
    struct faceuse **outfaceuses = NULL;
    struct vertex *vertp[3];
    struct vertex ***vells = (struct vertex ***)NULL;
    vect_t A, Au, B, Bu, Hu, V;
    vect_t apex_norm, rev_norm;
    vect_t A_orig, B_orig;
    struct vertex *apex_v;
    struct vertexuse *vu;
    struct faceuse *fu;

    RT_CK_DB_INTERNAL(ip);
    xip = (struct rt_epa_internal *)ip->idb_ptr;
    RT_EPA_CK_MAGIC(xip);

    /*
     * make sure epa description is valid
     */

    /* compute |A| |H| */
    mag_a = MAGSQ(xip->epa_Au);	/* should already be unit vector */
    mag_h = MAGNITUDE(xip->epa_H);
    r1 = xip->epa_r1;
    r2 = xip->epa_r2;
    /* Check for |H| > 0, |A| == 1, r1 > 0, r2 > 0 */
    if (NEAR_ZERO(mag_h, RT_LEN_TOL)
	|| !NEAR_EQUAL(mag_a, 1.0, RT_LEN_TOL)
	|| r1 <= 0.0 || r2 <= 0.0) {
	return -2;		/* BAD */
    }

    /* Check for A.H == 0 */
    f = VDOT(xip->epa_Au, xip->epa_H) / mag_h;
    if (! NEAR_ZERO(f, RT_DOT_TOL)) {
	return -2;		/* BAD */
    }

    /* make unit vectors in A, H, and BxH directions */
    VMOVE(Hu, xip->epa_H);
    VUNITIZE(Hu);
    VMOVE(Au, xip->epa_Au);
    VCROSS(Bu, Au, Hu);

    VSCALE(A_orig, Au, xip->epa_r1);
    VSCALE(B_orig, Bu, xip->epa_r2);

    /* Compute R and Rinv matrices */
    MAT_IDN(R);
    VREVERSE(&R[0], Bu);
    VMOVE(&R[4], Au);
    VREVERSE(&R[8], Hu);
    bn_mat_trn(invR, R);			/* inv of rot mat is trn */

    dtol = primitive_get_absolute_tolerance(ttol, 2.0 * r2);

    /* To ensure normal tolerance, remain below this angle */
    if (ttol->norm > 0.0)
	ntol = ttol->norm;
    else
	/* tolerate everything */
	ntol = bn_pi;

    /*
     * build epa from 2 parabolas
     */

    /* approximate positive half of parabola along semi-minor axis */
    pts_b = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");
    pts_b->next = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");
    pts_b->next->next = NULL;
    VSET(pts_b->p,       0, 0, -mag_h);
    VSET(pts_b->next->p, 0, r2, 0);
    /* 2 endpoints in 1st approximation */
    nb = 2;
    /* recursively break segment 'til within error tolerances */
    nb += rt_mk_parabola(pts_b, r2, mag_h, dtol, ntol);
    nell = nb - 1;	/* # of ellipses needed */

    /* construct positive half of parabola along semi-major axis of
     * epa using same z coords as parab along semi-minor axis
     */
    pts_a = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");
    VMOVE(pts_a->p, pts_b->p);	/* 1st pt is the apex */
    pts_a->next = NULL;
    pos_b = pts_b->next;
    pos_a = pts_a;
    while (pos_b) {
	/* copy node from b_parabola to a_parabola */
	pos_a->next = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");
	pos_a = pos_a->next;
	pos_a->p[Z] = pos_b->p[Z];
	/* at given z, find y on parabola */
	pos_a->p[Y] = r1*sqrt(pos_a->p[Z] / mag_h + 1);
	pos_a->p[X] = 0;
	pos_b = pos_b->next;
    }
    pos_a->next = NULL;

    /* see if any segments need further breaking up */
    recalc_b = 0;
    na = 0;
    pos_a = pts_a;
    while (pos_a->next) {
	na = rt_mk_parabola(pos_a, r1, mag_h, dtol, ntol);
	if (na != 0) {
	    recalc_b = 1;
	    nell += na;
	}
	pos_a = pos_a->next;
    }
    /* if any were broken, recalculate parabola 'a' */
    if (recalc_b) {
	/* free mem for old approximation of parabola */
	pos_b = pts_b;
	while (pos_b) {
	    struct rt_pt_node *tmp;

	    tmp = pos_b->next;
	    bu_free((char *)pos_b, "rt_pt_node");
	    pos_b = tmp;
	}
	/* construct parabola along semi-major axis of epa
	 * using same z coords as parab along semi-minor axis
	 */
	pts_b = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");
	pts_b->p[Z] = pts_a->p[Z];
	pts_b->next = NULL;
	pos_a = pts_a->next;
	pos_b = pts_b;
	while (pos_a) {
	    /* copy node from a_parabola to b_parabola */
	    pos_b->next = (struct rt_pt_node *)bu_malloc(sizeof(struct rt_pt_node), "rt_pt_node");
	    pos_b = pos_b->next;
	    pos_b->p[Z] = pos_a->p[Z];
	    /* at given z, find y on parabola */
	    pos_b->p[Y] = r2*sqrt(pos_b->p[Z] / mag_h + 1);
	    pos_b->p[X] = 0;
	    pos_a = pos_a->next;
	}
	pos_b->next = NULL;
    }

    /* make array of ptrs to epa ellipses */
    ellipses = (fastf_t **)bu_malloc(nell * sizeof(fastf_t *), "fastf_t ell[]");
    /* keep track of whether pts in each ellipse are doubled or not */
    pts_dbl = (int *)bu_malloc(nell * sizeof(int), "dbl ints");
    /* I don't understand this pts_dbl, so here is an array containing
     * the length of each ellipses array
     */
    segs_per_ell = (int *)bu_calloc(nell, sizeof(int), "rt_epa_tess: segs_per_ell");

    /* and an array of normals */
    normals = (fastf_t **)bu_malloc(nell * sizeof(fastf_t *), "fastf_t normals[]");

    /* make ellipses at each z level */
    i = 0;
    nseg = 0;
    theta_prev = bn_twopi;
    pos_a = pts_a->next;	/* skip over apex of epa */
    pos_b = pts_b->next;
    while (pos_a) {
	fastf_t t;

	t = (-pos_a->p[Z] / mag_h);
	VSCALE(A, Au, pos_a->p[Y]);	/* semimajor axis */
	VSCALE(B, Bu, pos_b->p[Y]);	/* semiminor axis */
	VJOIN1(V, xip->epa_V, -pos_a->p[Z], Hu);

	VSET(p1, 0., pos_b->p[Y], 0.);
	theta_new = ell_angle(p1, pos_a->p[Y], pos_b->p[Y], dtol, ntol);
	if (nseg == 0) {
	    nseg = (int)(bn_twopi / theta_new) + 1;
	    pts_dbl[i] = 0;
	    /* maximum number of faces needed for epa */
	    face = nseg*(1 + 3*((1 << (nell-1)) - 1));
	    /* array for each triangular face */
	    outfaceuses = (struct faceuse **)
		bu_malloc((face+1) * sizeof(struct faceuse *), "faceuse []");
	} else if (theta_new < theta_prev) {
	    nseg *= 2;
	    pts_dbl[i] = 1;
	} else {
	    pts_dbl[i] = 0;
	}
	theta_prev = theta_new;

	ellipses[i] = (fastf_t *)bu_malloc(3*(nseg+1)*sizeof(fastf_t),
					   "pts ell");
	segs_per_ell[i] = nseg;
	normals[i] = (fastf_t *)bu_malloc(3*(nseg+1)*sizeof(fastf_t), "rt_epa_tess_ normals");
	rt_ell(ellipses[i], V, A, B, nseg);
	rt_ell_norms(normals[i], A_orig, B_orig, xip->epa_H, t, nseg);

	i++;
	pos_a = pos_a->next;
	pos_b = pos_b->next;
    }

    /*
     * put epa geometry into nmg data structures
     */

    *r = nmg_mrsv(m);	/* Make region, empty shell, vertex */
    s = BU_LIST_FIRST(shell, &(*r)->s_hd);

    /* vertices of ellipses of epa */
    vells = (struct vertex ***)
	bu_malloc(nell*sizeof(struct vertex **), "vertex [][]");
    j = nseg;
    for (i = nell-1; i >= 0; i--) {
	vells[i] = (struct vertex **)
	    bu_malloc(j*sizeof(struct vertex *), "vertex []");
	if (i && pts_dbl[i])
	    j /= 2;
    }

    /* top face of epa */
    for (i = 0; i < nseg; i++)
	vells[nell-1][i] = (struct vertex *)0;
    face = 0;
    if ((outfaceuses[face++] = nmg_cface(s, vells[nell-1], nseg)) == 0) {
	bu_log("rt_epa_tess() failure, top face\n");
	goto fail;
    }
    for (i = 0; i < nseg; i++) {
	NMG_CK_VERTEX(vells[nell-1][i]);
	nmg_vertex_gv(vells[nell-1][i], &ellipses[nell-1][3*i]);
    }

    /* Mark the edges of this face as real, this is the only real edge */
    (void)nmg_mark_edges_real(&outfaceuses[0]->l.magic);

    /* connect ellipses with triangles */
    for (i = nell-2; i >= 0; i--) {
	/* skip top ellipse */
	int bottom, top;

	top = i + 1;
	bottom = i;
	if (pts_dbl[top])
	    nseg /= 2;	/* # segs in 'bottom' ellipse */
	vertp[0] = (struct vertex *)0;

	/* make triangular faces */
	for (j = 0; j < nseg; j++) {
	    jj = j + j;	/* top ellipse index */

	    if (pts_dbl[top]) {
		/* first triangle */
		vertp[1] = vells[top][jj+1];
		vertp[2] = vells[top][jj];
		if ((outfaceuses[face++] = nmg_cface(s, vertp, 3)) == 0) {
		    bu_log("rt_epa_tess() failure\n");
		    goto fail;
		}
		if (j == 0)
		    vells[bottom][j] = vertp[0];

		/* second triangle */
		vertp[2] = vertp[1];
		if (j == nseg-1)
		    vertp[1] = vells[bottom][0];
		else
		    vertp[1] = (struct vertex *)0;
		if ((outfaceuses[face++] = nmg_cface(s, vertp, 3)) == 0) {
		    bu_log("rt_epa_tess() failure\n");
		    goto fail;
		}
		if (j != nseg-1)
		    vells[bottom][j+1] = vertp[1];

		/* third triangle */
		vertp[0] = vertp[1];
		if (j == nseg-1)
		    vertp[1] = vells[top][0];
		else
		    vertp[1] = vells[top][jj+2];
		if ((outfaceuses[face++] = nmg_cface(s, vertp, 3)) == 0) {
		    bu_log("rt_epa_tess() failure\n");
		    goto fail;
		}
	    } else {
		/* first triangle */
		if (j == nseg-1)
		    vertp[1] = vells[top][0];
		else
		    vertp[1] = vells[top][j+1];
		vertp[2] = vells[top][j];
		if ((outfaceuses[face++] = nmg_cface(s, vertp, 3)) == 0) {
		    bu_log("rt_epa_tess() failure\n");
		    goto fail;
		}
		if (j == 0)
		    vells[bottom][j] = vertp[0];

		/* second triangle */
		vertp[2] = vertp[0];
		if (j == nseg-1)
		    vertp[0] = vells[bottom][0];
		else
		    vertp[0] = (struct vertex *)0;
		if ((outfaceuses[face++] = nmg_cface(s, vertp, 3)) == 0) {
		    bu_log("rt_epa_tess() failure\n");
		    goto fail;
		}
		if (j != nseg-1)
		    vells[bottom][j+1] = vertp[0];
	    }
	}

	/* associate geometry with each vertex */
	for (j = 0; j < nseg; j++) {
	    NMG_CK_VERTEX(vells[bottom][j]);
	    nmg_vertex_gv(vells[bottom][j],
			  &ellipses[bottom][3*j]);
	}
    }

    /* connect bottom of ellipse to apex of epa */
    VADD2(V, xip->epa_V, xip->epa_H);
    vertp[0] = (struct vertex *)0;
    vertp[1] = vells[0][1];
    vertp[2] = vells[0][0];
    if ((outfaceuses[face++] = nmg_cface(s, vertp, 3)) == 0) {
	bu_log("rt_epa_tess() failure\n");
	goto fail;
    }
    /* associate geometry with topology */
    NMG_CK_VERTEX(vertp[0]);
    nmg_vertex_gv(vertp[0], (fastf_t *)V);
    apex_v = vertp[0];
    /* create rest of faces around apex */
    for (i = 1; i < nseg; i++) {
	vertp[2] = vertp[1];
	if (i == nseg-1)
	    vertp[1] = vells[0][0];
	else
	    vertp[1] = vells[0][i+1];
	if ((outfaceuses[face++] = nmg_cface(s, vertp, 3)) == 0) {
	    bu_log("rt_epa_tess() failure\n");
	    goto fail;
	}
    }

    /* Associate the face geometry */
    for (i=0; i < face; i++) {
	if (nmg_fu_planeeqn(outfaceuses[i], tol) < 0)
	    goto fail;
    }

    /* Associate vertexuse normals */
    for (i=0; i<nell; i++) {
	for (j=0; j<segs_per_ell[i]; j++) {
	    VREVERSE(rev_norm, &normals[i][j*3]);
	    for (BU_LIST_FOR(vu, vertexuse, &vells[i][j]->vu_hd)) {

		fu = nmg_find_fu_of_vu(vu);
		NMG_CK_FACEUSE(fu);

		if (fu == outfaceuses[0] || fu->fumate_p == outfaceuses[0])
		    continue;	/* don't assign normals to top faceuse (flat) */

		if (fu->orientation == OT_SAME)
		    nmg_vertexuse_nv(vu, &normals[i][j*3]);
		else if (fu->orientation == OT_OPPOSITE)
		    nmg_vertexuse_nv(vu, rev_norm);
	    }
	}
    }
    /* and don't forget the apex */
    VMOVE(apex_norm, xip->epa_H);
    VUNITIZE(apex_norm);
    VREVERSE(rev_norm, apex_norm);
    for (BU_LIST_FOR(vu, vertexuse, &apex_v->vu_hd)) {
	NMG_CK_VERTEXUSE(vu);
	fu = nmg_find_fu_of_vu(vu);
	NMG_CK_FACEUSE(fu);
	if (fu->orientation == OT_SAME)
	    nmg_vertexuse_nv(vu, apex_norm);
	else if (fu->orientation == OT_OPPOSITE)
	    nmg_vertexuse_nv(vu, rev_norm);
    }

    /* Glue the edges of different outward pointing face uses together */
    nmg_gluefaces(outfaceuses, face, tol);

    /* Compute "geometry" for region and shell */
    nmg_region_a(*r, tol);

    /* XXX just for testing, to make up for loads of triangles ... */
    nmg_shell_coplanar_face_merge(s, tol, 1);

    /* free mem */
    bu_free((char *)outfaceuses, "faceuse []");
    for (i = 0; i < nell; i++) {
	bu_free((char *)ellipses[i], "pts ell");
	bu_free((char *)vells[i], "vertex []");
    }
    bu_free((char *)ellipses, "fastf_t ell[]");
    bu_free((char *)pts_dbl, "dbl ints");
    bu_free((char *)vells, "vertex [][]");

    return 0;

 fail:
    /* free mem */
    bu_free((char *)outfaceuses, "faceuse []");
    for (i = 0; i < nell; i++) {
	bu_free((char *)ellipses[i], "pts ell");
	bu_free((char *)vells[i], "vertex []");
    }
    bu_free((char *)ellipses, "fastf_t ell[]");
    bu_free((char *)pts_dbl, "dbl ints");
    bu_free((char *)vells, "vertex [][]");

    return -1;
}


/**
 * R T _ E P A _ I M P O R T
 *
 * Import an EPA from the database format to the internal format.
 * Apply modeling transformations as well.
 */
int
rt_epa_import4(struct rt_db_internal *ip, const struct bu_external *ep, const fastf_t *mat, const struct db_i *dbip)
{
    struct rt_epa_internal *xip;
    union record *rp;
    vect_t v1, v2, v3;

    if (dbip) RT_CK_DBI(dbip);

    BU_CK_EXTERNAL(ep);
    rp = (union record *)ep->ext_buf;
    /* Check record type */
    if (rp->u_id != ID_SOLID) {
	bu_log("rt_epa_import4: defective record\n");
	return -1;
    }

    RT_CK_DB_INTERNAL(ip);
    ip->idb_major_type = DB5_MAJORTYPE_BRLCAD;
    ip->idb_type = ID_EPA;
    ip->idb_meth = &rt_functab[ID_EPA];
    ip->idb_ptr = bu_malloc(sizeof(struct rt_epa_internal), "rt_epa_internal");
    xip = (struct rt_epa_internal *)ip->idb_ptr;
    xip->epa_magic = RT_EPA_INTERNAL_MAGIC;

    /* Warning:  type conversion */
    if (mat == NULL) mat = bn_mat_identity;

    if (dbip->dbi_version < 0) {
	flip_fastf_float(v1, &rp->s.s_values[0*3], 1, 1);
	flip_fastf_float(v2, &rp->s.s_values[1*3], 1, 1);
	flip_fastf_float(v3, &rp->s.s_values[2*3], 1, 1);
    } else {
	VMOVE(v1, &rp->s.s_values[0*3]);
	VMOVE(v2, &rp->s.s_values[1*3]);
	VMOVE(v3, &rp->s.s_values[2*3]);
    }

    MAT4X3PNT(xip->epa_V, mat, v1);
    MAT4X3VEC(xip->epa_H, mat, v2);
    MAT4X3VEC(xip->epa_Au, mat, v3);

    VUNITIZE(xip->epa_Au);

    if (dbip->dbi_version < 0) {
	v1[X] = flip_dbfloat(rp->s.s_values[3*3+0]);
	v1[Y] = flip_dbfloat(rp->s.s_values[3*3+1]);
    } else {
	v1[X] = rp->s.s_values[3*3+0];
	v1[Y] = rp->s.s_values[3*3+1];
    }

    xip->epa_r1 = v1[X] / mat[15];
    xip->epa_r2 = v1[Y] / mat[15];

    if (xip->epa_r1 <= SMALL_FASTF || xip->epa_r2 <= SMALL_FASTF) {
	bu_log("rt_epa_import4: r1 or r2 are zero\n");
	bu_free((char *)ip->idb_ptr, "rt_epa_import4: ip->idb_ptr");
	return -1;
    }

    return 0;			/* OK */
}


/**
 * R T _ E P A _ E X P O R T
 *
 * The name is added by the caller, in the usual place.
 */
int
rt_epa_export4(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip)
{
    struct rt_epa_internal *xip;
    union record *epa;
    fastf_t mag_h;

    if (dbip) RT_CK_DBI(dbip);

    RT_CK_DB_INTERNAL(ip);
    if (ip->idb_type != ID_EPA) return -1;
    xip = (struct rt_epa_internal *)ip->idb_ptr;
    RT_EPA_CK_MAGIC(xip);

    BU_CK_EXTERNAL(ep);
    ep->ext_nbytes = sizeof(union record);
    ep->ext_buf = (genptr_t)bu_calloc(1, ep->ext_nbytes, "epa external");
    epa = (union record *)ep->ext_buf;

    epa->s.s_id = ID_SOLID;
    epa->s.s_type = EPA;

    if (!NEAR_EQUAL(MAGNITUDE(xip->epa_Au), 1.0, RT_LEN_TOL)) {
	bu_log("rt_epa_export4: Au not a unit vector!\n");
	return -1;
    }

    mag_h = MAGNITUDE(xip->epa_H);

    if (mag_h < RT_LEN_TOL
	|| xip->epa_r1 < RT_LEN_TOL
	|| xip->epa_r2 < RT_LEN_TOL) {
	bu_log("rt_epa_export4: not all dimensions positive!\n");
	return -1;
    }

    if (!NEAR_ZERO(VDOT(xip->epa_Au, xip->epa_H)/mag_h, RT_DOT_TOL)) {
	bu_log("rt_epa_export4: Au and H are not perpendicular!\n");
	return -1;
    }

    if (xip->epa_r2 > xip->epa_r1) {
	bu_log("rt_epa_export4: semi-minor axis cannot be longer than semi-major axis!\n");
	return -1;
    }

    /* Warning:  type conversion */
    VSCALE(&epa->s.s_values[0*3], xip->epa_V, local2mm);
    VSCALE(&epa->s.s_values[1*3], xip->epa_H, local2mm);
    VMOVE(&epa->s.s_values[2*3], xip->epa_Au); /* don't scale a unit vector */
    epa->s.s_values[3*3] = xip->epa_r1 * local2mm;
    epa->s.s_values[3*3+1] = xip->epa_r2 * local2mm;

    return 0;
}


/**
 * R T _ E P A _ I M P O R T 5
 *
 * Import an EPA from the database format to the internal format.
 * Apply modeling transformations as well.
 */
int
rt_epa_import5(struct rt_db_internal *ip, const struct bu_external *ep, const fastf_t *mat, const struct db_i *dbip)
{
    struct rt_epa_internal *xip;
    fastf_t vec[11];

    if (dbip) RT_CK_DBI(dbip);
    BU_CK_EXTERNAL(ep);

    BU_ASSERT_LONG(ep->ext_nbytes, ==, SIZEOF_NETWORK_DOUBLE * 11);

    RT_CK_DB_INTERNAL(ip);
    ip->idb_major_type = DB5_MAJORTYPE_BRLCAD;
    ip->idb_type = ID_EPA;
    ip->idb_meth = &rt_functab[ID_EPA];
    ip->idb_ptr = bu_malloc(sizeof(struct rt_epa_internal), "rt_epa_internal");

    xip = (struct rt_epa_internal *)ip->idb_ptr;
    xip->epa_magic = RT_EPA_INTERNAL_MAGIC;

    /* Convert from database (network) to internal (host) format */
    ntohd((unsigned char *)vec, ep->ext_buf, 11);

    /* Apply modeling transformations */
    if (mat == NULL) mat = bn_mat_identity;
    MAT4X3PNT(xip->epa_V, mat, &vec[0*3]);
    MAT4X3VEC(xip->epa_H, mat, &vec[1*3]);
    MAT4X3VEC(xip->epa_Au, mat, &vec[2*3]);
    VUNITIZE(xip->epa_Au);
    xip->epa_r1 = vec[3*3] / mat[15];
    xip->epa_r2 = vec[3*3+1] / mat[15];

    if (xip->epa_r1 <= SMALL_FASTF || xip->epa_r2 <= SMALL_FASTF) {
	bu_log("rt_epa_import4: r1 or r2 are zero\n");
	bu_free((char *)ip->idb_ptr, "rt_epa_import4: ip->idb_ptr");
	return -1;
    }

    return 0;			/* OK */
}


/**
 * R T _ E P A _ E X P O R T 5
 *
 * The name is added by the caller, in the usual place.
 */
int
rt_epa_export5(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip)
{
    struct rt_epa_internal *xip;
    fastf_t vec[11];
    fastf_t mag_h;

    if (dbip) RT_CK_DBI(dbip);

    RT_CK_DB_INTERNAL(ip);
    if (ip->idb_type != ID_EPA) return -1;
    xip = (struct rt_epa_internal *)ip->idb_ptr;
    RT_EPA_CK_MAGIC(xip);

    BU_CK_EXTERNAL(ep);
    ep->ext_nbytes = SIZEOF_NETWORK_DOUBLE * 11;
    ep->ext_buf = (genptr_t)bu_malloc(ep->ext_nbytes, "epa external");

    if (!NEAR_EQUAL(MAGNITUDE(xip->epa_Au), 1.0, RT_LEN_TOL)) {
	bu_log("rt_epa_export4: Au not a unit vector!\n");
	return -1;
    }

    mag_h = MAGNITUDE(xip->epa_H);

    if (mag_h < RT_LEN_TOL
	|| xip->epa_r1 < RT_LEN_TOL
	|| xip->epa_r2 < RT_LEN_TOL) {
	bu_log("rt_epa_export4: not all dimensions positive!\n");
	return -1;
    }

    if (!NEAR_ZERO(VDOT(xip->epa_Au, xip->epa_H)/mag_h, RT_DOT_TOL)) {
	bu_log("rt_epa_export4: Au and H are not perpendicular!\n");
	return -1;
    }

    if (xip->epa_r2 > xip->epa_r1) {
	bu_log("rt_epa_export4: semi-minor axis cannot be longer than semi-major axis!\n");
	return -1;
    }

    /* scale 'em into local buffer */
    VSCALE(&vec[0*3], xip->epa_V, local2mm);
    VSCALE(&vec[1*3], xip->epa_H, local2mm);
    VMOVE(&vec[2*3], xip->epa_Au); /* don't scale a unit vector */
    vec[3*3] = xip->epa_r1 * local2mm;
    vec[3*3+1] = xip->epa_r2 * local2mm;

    /* Convert from internal (host) to database (network) format */
    htond(ep->ext_buf, (unsigned char *)vec, 11);

    return 0;
}


/**
 * R T _ E P A _ D E S C R I B E
 *
 * Make human-readable formatted presentation of this solid.  First
 * line describes type of solid.  Additional lines are indented one
 * tab, and give parameter values.
 */
int
rt_epa_describe(struct bu_vls *str, const struct rt_db_internal *ip, int verbose, double mm2local)
{
    struct rt_epa_internal *xip = (struct rt_epa_internal *)ip->idb_ptr;
    char buf[256];

    RT_EPA_CK_MAGIC(xip);
    bu_vls_strcat(str, "Elliptical Paraboloid (EPA)\n");

    sprintf(buf, "\tV (%g, %g, %g)\n",
	    INTCLAMP(xip->epa_V[X] * mm2local),
	    INTCLAMP(xip->epa_V[Y] * mm2local),
	    INTCLAMP(xip->epa_V[Z] * mm2local));
    bu_vls_strcat(str, buf);

    sprintf(buf, "\tH (%g, %g, %g) mag=%g\n",
	    INTCLAMP(xip->epa_H[X] * mm2local),
	    INTCLAMP(xip->epa_H[Y] * mm2local),
	    INTCLAMP(xip->epa_H[Z] * mm2local),
	    INTCLAMP(MAGNITUDE(xip->epa_H) * mm2local));
    bu_vls_strcat(str, buf);

    if (!verbose)
	return 0;

    sprintf(buf, "\tA=%g\n", INTCLAMP(xip->epa_r1 * mm2local));
    bu_vls_strcat(str, buf);

    sprintf(buf, "\tB=%g\n", INTCLAMP(xip->epa_r2 * mm2local));
    bu_vls_strcat(str, buf);

    return 0;
}


/**
 * R T _ E P A _ I F R E E
 *
 * Free the storage associated with the rt_db_internal version of this
 * solid.
 */
void
rt_epa_ifree(struct rt_db_internal *ip)
{
    struct rt_epa_internal *xip;

    RT_CK_DB_INTERNAL(ip);

    xip = (struct rt_epa_internal *)ip->idb_ptr;
    RT_EPA_CK_MAGIC(xip);
    xip->epa_magic = 0;		/* sanity */

    bu_free((char *)xip, "epa ifree");
    ip->idb_ptr = GENPTR_NULL;	/* sanity */
}


/**
 * R T _ E P A _ P A R A M S
 *
 */
int
rt_epa_params(struct pc_pc_set *ps, const struct rt_db_internal *ip)
{
    if (!ps) return 0;
    if (ip) RT_CK_DB_INTERNAL(ip);

    return 0;			/* OK */
}


/**
 * R T _ E P A _ V O L U M E
 */
void
rt_epa_volume(fastf_t *vol, const struct rt_db_internal *ip)
{
    fastf_t mag_h;
    struct rt_epa_internal *xip = (struct rt_epa_internal *)ip->idb_ptr;
    RT_EPA_CK_MAGIC(xip);

    mag_h = MAGNITUDE(xip->epa_H);
    *vol = 0.5 * M_PI * xip->epa_r1 * xip->epa_r2 * mag_h;
}


/**
 * R T _ E P A _ C E N T R O I D
 */
void
rt_epa_centroid(point_t *cent, const struct rt_db_internal *ip)
{
    struct rt_epa_internal *xip = (struct rt_epa_internal *)ip->idb_ptr;
    RT_EPA_CK_MAGIC(xip);
    VJOIN1(*cent, xip->epa_V, 1.0/3.0, xip->epa_H);
}


/**
 * R T _ E P A _ S U R F _ A R E A
 */
void
rt_epa_surf_area(fastf_t *area, const struct rt_db_internal *ip)
{
    fastf_t magsq_h, m;
    struct rt_epa_internal *xip = (struct rt_epa_internal *)ip->idb_ptr;
    RT_EPA_CK_MAGIC(xip);

    magsq_h = MAGSQ(xip->epa_H);
    m = sqrt(1.0 + (4.0 * magsq_h) / (xip->epa_r1 * xip->epa_r2));
    *area = 2.0/3.0 * M_PI * xip->epa_r1 * xip->epa_r2 * (m + (1.0 / (m + 1.0)));
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
