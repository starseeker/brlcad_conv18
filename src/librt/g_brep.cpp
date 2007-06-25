/*                     G _ B R E P . C P P
 * BRL-CAD
 *
 * Copyright (c) 2007 United States Government as represented by
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
/** @addtogroup g_  */
/** @{ */
/** @file g_brep.cpp
 *
 */

#include "common.h"
#include "brep.h"
#include "raytrace.h"
#include "rtgeom.h"
#include "vector.h"

#ifdef write
#   undef write
#endif

#ifdef read
#   undef read
#endif

#include <vector>
#include <list>

#ifdef __cplusplus
extern "C" {
#endif

int
rt_brep_prep(struct soltab *stp, struct rt_db_internal* ip, struct rt_i* rtip);
void
rt_brep_print(register const struct soltab *stp);
int
rt_brep_shot(struct soltab *stp, register struct xray *rp, struct application *ap, struct seg *seghead);
void
rt_brep_norm(register struct hit *hitp, struct soltab *stp, register struct xray *rp);
void
rt_brep_curve(register struct curvature *cvp, register struct hit *hitp, struct soltab *stp);
int
rt_brep_class();
void
rt_brep_uv(struct application *ap, struct soltab *stp, register struct hit *hitp, register struct uvcoord *uvp);
void
rt_brep_free(register struct soltab *stp);
int
rt_brep_plot(struct bu_list *vhead, struct rt_db_internal *ip, const struct rt_tess_tol *ttol, const struct bn_tol *tol);
int
rt_brep_tess(struct nmgregion **r, struct model *m, struct rt_db_internal *ip, const struct rt_tess_tol *ttol, const struct bn_tol *tol);
int
rt_brep_export5(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip);
int
rt_brep_import5(struct rt_db_internal *ip, const struct bu_external *ep, register const fastf_t *mat, const struct db_i *dbip);
void
rt_brep_ifree(struct rt_db_internal *ip);
int
rt_brep_describe(struct bu_vls *str, const struct rt_db_internal *ip, int verbose, double mm2local);
int
rt_brep_tclget(Tcl_Interp *interp, const struct rt_db_internal *intern, const char *attr);
int
rt_brep_tcladjust(Tcl_Interp *interp, struct rt_db_internal *intern, int argc, char **argv);

#ifdef __cplusplus
  }
#endif

/********************************************************************************
 * Auxiliary functions
 ********************************************************************************/

#include <list>
#include <iostream>
#include <algorithm>

using namespace brlcad;

ON_Ray toXRay(struct xray* rp) {
    ON_3dPoint pt(rp->r_pt);
    ON_3dVector dir(rp->r_dir);
    return ON_Ray(pt, dir);
}

//--------------------------------------------------------------------------------
// specific
struct brep_specific*
brep_specific_new()
{
    return (struct brep_specific*)bu_calloc(1, sizeof(struct brep_specific),"brep_specific_new");
}

void
brep_specific_delete(struct brep_specific* bs)
{
    if (bs != NULL) {
	delete bs->bvh;
	bu_free(bs,"brep_specific_delete");
    }
}




//--------------------------------------------------------------------------------
// prep

/**
 * Given a list of face bounding volumes for an entire brep, build up
 * an appropriate hierarchy to make it efficient (binary may be a
 * reasonable choice, for example).
 */
void
brep_bvh_subdivide(BBNode* parent, const std::list<SurfaceTree*>& face_trees)
{
    // XXX this needs to handle a threshold and some reasonable space
    // partitioning
//     for (BVList::const_iterator i = face_bvs.begin(); i != face_bvs.end(); i++) {
// 	parent->gs_insert(*i);
//     }
    for (std::list<SurfaceTree*>::const_iterator i = face_trees.begin(); i != face_trees.end(); i++) {
	parent->addChild((*i)->getRootNode());
    }
}

inline void 
distribute(const int count, const ON_3dVector* v, double x[], double y[], double z[])
{
    for (int i = 0; i < count; i++) {
	x[i] = v[i].x;
	y[i] = v[i].y;
	z[i] = v[i].z;
    }
}

bool 
brep_pt_trimmed(pt2d_t pt, const ON_BrepFace& face) {
    TRACE1("brep_pt_trimmed: " << PT2(pt));
    // for each loop
    const ON_Surface* surf = face.SurfaceOf();
    double umin, umax;
    ON_2dPoint from, to;
    from.x = pt[0];
    from.y = to.y = pt[1];
    surf->GetDomain(0, &umin, &umax);    
    to.x = umax + 1;
    ON_Line ray(from,to);
    int intersections = 0;
    for (int i = 0; i < face.LoopCount(); i++) {
	ON_BrepLoop* loop = face.Loop(i);
	// for each trim
	for (int j = 0; j < loop->m_ti.Count(); j++) {
	    ON_BrepTrim& trim = face.Brep()->m_T[loop->m_ti[j]];
	    const ON_Curve* trimCurve = trim.TrimCurveOf();
	    // intersections += brep_count_intersections(ray, trimCurve);	    
	    //ray.IntersectCurve(trimCurve, intersections, 0.0001);
	    intersections += trimCurve->NumIntersectionsWith(ray);
	}
    }
    // the point is trimmed if the # of intersections is even
    return (intersections % 2) == 0;
}

// XXX - most of this function is broken :-( except for the bezier span caching
// need to fix it! - could provide real performance benefits...
void 
brep_preprocess_trims(const ON_BrepFace& face, SurfaceTree* tree) {

    list<BBNode*> leaves;
    tree->getLeaves(leaves);

    for (list<BBNode*>::iterator i = leaves.begin(); i != leaves.end(); i++) {
	SubsurfaceBBNode* bb = dynamic_cast<SubsurfaceBBNode*>(*i);
	
	
	// XXX - TODO: check to see if this portion of the surface
	// needs to be checked for trims
	pt2d_t test[] = {{bb->m_u.Min(),bb->m_v.Min()},
			 {bb->m_u.Max(),bb->m_v.Min()},
			 {bb->m_u.Max(),bb->m_v.Max()},
			 {bb->m_u.Min(),bb->m_v.Max()}};


	// check to see if the bbox encloses a trim
	ON_3dPoint uvmin(bb->m_u.Min(),bb->m_v.Min(),0);
	ON_3dPoint uvmax(bb->m_u.Max(),bb->m_v.Max(),0);
	ON_BoundingBox bbox(uvmin,uvmax);
	bool internalTrim = false;
	for (int i = 0; i < face.Brep()->m_L.Count(); i++) {
	    ON_BrepLoop& loop = face.Brep()->m_L[i];
	    // for each trim
	    for (int j = 0; j < loop.m_ti.Count(); j++) {	    
		ON_BrepTrim& trim = face.Brep()->m_T[loop.m_ti[j]];
		if (bbox.Intersection(trim.m_pbox)) internalTrim = true;
		
		// tell the NURBS curves to cache their Bezier spans
		// (used in trimming routines, and not thread safe)
		const ON_Curve* c = trim.TrimCurveOf();
		if (c->ClassId()->IsDerivedFrom(&ON_NurbsCurve::m_ON_NurbsCurve_class_id)) {
		    ON_NurbsCurve::Cast(c)->CacheBezierSpans();
		}
	    }
	}
	bb->m_checkTrim = true; // XXX - ack, hardcode for now
	// for this node to be completely trimmed, all for corners
	// must be trimmed and the depth of the tree needs to be > 0,
	// since 0 means there is just a single leaf - and since
	// "internal" outer loops will be make a single node seem
	// trimmed, we must account for it.
	bb->m_trimmed = false; // XXX - ack, hardcode for now
    }
}

int
brep_build_bvh(struct brep_specific* bs, struct rt_brep_internal* bi)
{
    ON_TextLog tl(stderr);
    ON_Brep* brep = bs->brep;
    if (brep == NULL || !brep->IsValid(&tl)) {
	bu_log("brep is NOT valid");
	return -1;
    }

    bs->bvh = new BBNode(brep->BoundingBox());

    // need to extract faces, and build bounding boxes for each face,
    // then combine the face BBs back up, combining them together to
    // better split the hierarchy
    std::list<SurfaceTree*> surface_trees;
    ON_BrepFaceArray& faces = brep->m_F;
    for (int i = 0; i < faces.Count(); i++) {
        TRACE1("Face: " << i);
	ON_BrepFace& face = faces[i];
// 	const ON_Surface* surf = face.SurfaceOf();
// 	TRACE1("Surf: " << surf);

// 	ON_Interval u = surf->Domain(0);
// 	ON_Interval v = surf->Domain(1);
// 	BoundingVolume* bv = brep_surface_subdivide(face, u, v, 0);

	SurfaceTree* st = new SurfaceTree(&face);
	brep_preprocess_trims(face, st);

	// add the surface bounding volumes to a list, so we can build
	// down a hierarchy from the brep bounding volume
	surface_trees.push_back(st);
    }

    brep_bvh_subdivide(bs->bvh, surface_trees);

    return 0;
}

void
brep_calculate_cdbitems(struct brep_specific* bs, struct rt_brep_internal* bi)
{

}


/********************************************************************************
 * BRL-CAD Primitive interface
 ********************************************************************************/
/**
 *   			R T _ B R E P _ P R E P
 *
 *  Given a pointer of a GED database record, and a transformation
 *  matrix, determine if this is a valid NURB, and if so, prepare the
 *  surface so the intersections will work.
 */
int
rt_brep_prep(struct soltab *stp, struct rt_db_internal* ip, struct rt_i* rtip)
{
    TRACE1("rt_brep_prep");
    /* This prepares the NURBS specific data structures to be used
       during intersection... i.e. acceleration data structures and
       whatever else is needed.

       Abert's paper (Direct and Fast Ray Tracing of NURBS Surfaces)
       suggests using a bounding volume hierarchy (instead of KD-tree)
       and building it down to a satisfactory flatness criterion
       (which they do not give information about).
    */
    struct rt_brep_internal* bi;
    struct brep_specific* bs;

    RT_CK_DB_INTERNAL(ip);
    bi = (struct rt_brep_internal*)ip->idb_ptr;
    RT_BREP_CK_MAGIC(bi);

    if ((bs = (struct brep_specific*)stp->st_specific) == NULL) {
	bs = (struct brep_specific*)bu_malloc(sizeof(struct brep_specific), "brep_specific");
	bs->brep = bi->brep;
	bi->brep = NULL;
	stp->st_specific = (genptr_t)bs;
    }

    if (brep_build_bvh(bs, bi) < 0) {
	return -1;
    }

    point_t adjust;
    VSETALL(adjust,0.02);
    bs->bvh->GetBBox(stp->st_min, stp->st_max);
    // expand outer bounding box...
    VSUB2(stp->st_min,stp->st_min,adjust);
    VADD2(stp->st_max,stp->st_max,adjust);
    VADD2SCALE(stp->st_center, stp->st_min, stp->st_max, 0.5);
    vect_t work;
    VSUB2SCALE(work, stp->st_max, stp->st_min, 0.5);
    fastf_t f = work[X];
    V_MAX(f,work[Y]);
    V_MAX(f,work[Z]);
    stp->st_aradius = f;
    stp->st_bradius = MAGNITUDE(work);

    brep_calculate_cdbitems(bs, bi);

    
    return 0;
}


/**
 *			R T _ B R E P _ P R I N T
 */
void
rt_brep_print(register const struct soltab *stp)
{
}

//================================================================================
// shot support

class plane_ray {
public:
    vect_t n1;
    fastf_t d1;
    
    vect_t n2;
    fastf_t d2;
};

// void 
// brep_intersect_bv(IsectList& inters, struct xray* r, BoundingVolume* bv) 
// {
//     fastf_t tnear, tfar;
//     bool intersects = bv->intersected_by(r,&tnear,&tfar);
//     if (intersects && bv->is_leaf() && !dynamic_cast<SurfaceBV*>(bv)->isTrimmed()) {
// 	inters.push_back(IRecord(bv,tnear));
//     }
//     else if (intersects)
// 	for (BVList::iterator i = bv->children.begin(); i != bv->children.end(); i++) {
// 	    brep_intersect_bv(inters, r, *i);
// 	}
// }

void 
brep_get_plane_ray(struct xray* r, plane_ray& pr)
{
    vect_t v1;
    VMOVE(v1, r->r_dir);
    fastf_t min = MAX_FASTF;
    int index = -1;
    for (int i = 0; i < 3; i++) { // find the smallest component
	if (fabs(v1[i]) < min) {
	    min = fabs(v1[i]);
	    index = i;
	}
    }
    v1[index] += 1; // alter the smallest component
    VCROSS(pr.n1, v1, r->r_dir); // n1 is perpendicular to v1
    VUNITIZE(pr.n1);
    VCROSS(pr.n2, pr.n1, r->r_dir);       // n2 is perpendicular to v1 and n1
    VUNITIZE(pr.n2);
    pr.d1 = VDOT(pr.n1,r->r_pt);
    pr.d2 = VDOT(pr.n2,r->r_pt);
    TRACE1("n1:" << ON_PRINT3(pr.n1) << " n2:" << ON_PRINT3(pr.n2) << " d1:" << pr.d1 << " d2:" << pr.d2);
}


// XXX - todo: use static allocation instead
class brep_hit {
public:
    const ON_BrepFace& face;
    point_t point;
    vect_t  normal;
    pt2d_t  uv;
    
    brep_hit(const ON_BrepFace& f, point_t p, vect_t n, pt2d_t _uv) 
	: face(f)
    {
	VMOVE(point, p);
	VMOVE(normal, n);
	move(uv, _uv);
    }

    brep_hit(const brep_hit& h) 
	: face(h.face)
    {
	VMOVE(point, h.point);
	VMOVE(normal, h.normal);
	move(uv, h.uv);
    }

    brep_hit& operator=(const brep_hit& h) {
	const_cast<ON_BrepFace&>(face) = h.face;
	VMOVE(point, h.point);
	VMOVE(normal, h.normal);
	move(uv, h.uv);
    }
};
typedef std::list<brep_hit> HitList;

void
brep_r(const ON_Surface* surf, plane_ray& pr, pt2d_t uv, ON_3dPoint& pt, ON_3dVector& su, ON_3dVector& sv, pt2d_t R)
{
    surf->Ev1Der(uv[0], uv[1], pt, su, sv);
//     TRACE1("\tpt" << ON_PRINT3(pt) << ", su" << ON_PRINT3(su) << ", sv" ON_PRINT3(sv));
    R[0] = VDOT(pr.n1,((fastf_t*)pt)) - pr.d1;
    R[1] = VDOT(pr.n2,((fastf_t*)pt)) - pr.d2;
}

void
brep_newton_iterate(const ON_Surface* surf, plane_ray& pr, pt2d_t R, ON_3dVector& su, ON_3dVector& sv, pt2d_t uv, pt2d_t out_uv)
{
//     TRACE1("brep_newton_iterate: " << ON_PRINT2(uv));

    mat2d_t jacob = { VDOT(pr.n1,((fastf_t*)su)), VDOT(pr.n1,((fastf_t*)sv)),
		      VDOT(pr.n2,((fastf_t*)su)), VDOT(pr.n2,((fastf_t*)sv)) };
    mat2d_t inv_jacob;
    if (mat2d_inverse(inv_jacob, jacob)) { // check inverse validity
	pt2d_t tmp;
	mat2d_pt2d_mul(tmp, inv_jacob, R);
// 	TRACE1("\tuv:"<<ON_PRINT2(uv)<<" tmp:"<<ON_PRINT2(tmp));
	pt2dsub(out_uv, uv, tmp);
    }
    else {
	TRACE1("inverse failed"); // XXX how to handle this?
	move(out_uv,uv);
    }
}

#define BREP_INTERSECT_ROOT_ITERATION_LIMIT  	-3
#define BREP_INTERSECT_ROOT_DIVERGED            -2
#define BREP_INTERSECT_OOB       		-1
#define BREP_INTERSECT_TRIMMED     		0
#define BREP_INTERSECT_FOUND   	    		1

static const char* BREP_INTERSECT_REASONS[] = {"hit root iteration limit",
					       "root diverged",
					       "out of subsurface bounds",
					       "trimmed",
					       "found"};
#define BREP_INTERSECT_GET_REASON(i) BREP_INTERSECT_REASONS[(i)+3]

int 
brep_intersect(const SubsurfaceBBNode* sbv, const ON_BrepFace* face, const ON_Surface* surf, pt2d_t uv, plane_ray& pr, HitList& hits)
{
    int found = BREP_INTERSECT_ROOT_ITERATION_LIMIT;
    fastf_t Dlast = MAX_FASTF;
    pt2d_t Rcurr;
    pt2d_t new_uv;
    ON_3dPoint pt;
    ON_3dVector su;
    ON_3dVector sv;
    for (int i = 0; i < BREP_MAX_ITERATIONS; i++) {
	brep_r(surf,pr,uv,pt,su,sv,Rcurr);
	fastf_t d = v2mag(Rcurr);
	if (d < BREP_INTERSECTION_ROOT_EPSILON) {
	    TRACE1("R:"<<ON_PRINT2(Rcurr));
	    found = BREP_INTERSECT_FOUND; break; 
	} else if (d > Dlast) {
	    found = BREP_INTERSECT_ROOT_DIVERGED; break;
	}	
	brep_newton_iterate(surf, pr, Rcurr, su, sv, uv, new_uv);
	move(uv, new_uv);
	Dlast = d;
    }    
    if (found > 0) {
	fastf_t l,h;

	if (!sbv->m_u.Includes(uv[0])) return BREP_INTERSECT_OOB;
	if (!sbv->m_v.Includes(uv[1])) return BREP_INTERSECT_OOB;

	if (sbv->doTrimming() && brep_pt_trimmed(uv, *face)) return BREP_INTERSECT_TRIMMED;

	ON_3dPoint _pt;
	ON_3dVector _norm;
	surf->EvNormal(uv[0],uv[1],_pt,_norm);
	hits.push_back(brep_hit(*face, (fastf_t*)_pt,(fastf_t*)_norm, uv));
    }    
    return found;
}

class HitSorter : public std::greater<brep_hit>
{
    point_t m_origin;
public:
    HitSorter(point_t origin) {
	VMOVE(m_origin, origin);
    }
    
    bool operator()(brep_hit& left, brep_hit& right) {
	return DIST_PT_PT(left.point, m_origin) < DIST_PT_PT(right.point, m_origin);
    }
};

/**
 *  			R T _ B R E P _ S H O T
 *
 *  Intersect a ray with a brep.
 *  If an intersection occurs, a struct seg will be acquired
 *  and filled in.
 *
 *  Returns -
 *  	0	MISS
 *	>0	HIT
 */
int
rt_brep_shot(struct soltab *stp, register struct xray *rp, struct application *ap, struct seg *seghead)
{
    TRACE1("rt_brep_shot origin:" << ON_PRINT3(rp->r_pt) << " dir:" << ON_PRINT3(rp->r_dir));
    vect_t invdir;
    struct brep_specific* bs = (struct brep_specific*)stp->st_specific;

    // check the hierarchy to see if we have a hit at a leaf node
    BBNode::IsectList inters;
    ON_Ray r = toXRay(rp);
    bs->bvh->intersectsHierarchy(r, &inters);
    //inters.sort();
    TRACE1("found " << inters.size() << " intersections!");    

    if (inters.size() == 0) return 0; // MISS

    plane_ray pr;
    brep_get_plane_ray(rp, pr);    

    // find all the hits (XXX very inefficient right now!)
    HitList hits;
    for (BBNode::IsectList::iterator i = inters.begin(); i != inters.end(); i++) {
	const SubsurfaceBBNode* sbv = dynamic_cast<SubsurfaceBBNode*>((*i).m_node);
	const ON_BrepFace* f = sbv->m_face;
	const ON_Surface* surf = f->SurfaceOf();       
	brep_hit* hit; 
	pt2d_t uv = {sbv->m_u.Mid(),sbv->m_v.Mid()};
	int status = brep_intersect(sbv, f, surf, uv, pr, hits);
	if (status < 0 && status > BREP_INTERSECT_ROOT_ITERATION_LIMIT) {
	    TRACE("no intersection: " << BREP_INTERSECT_GET_REASON(status));
	}
    }

    // sort the hits
    HitSorter hs(rp->r_pt);
    hits.sort(hs);
    
    bool hit = false;
    if (hits.size() > 0) {
	if (hits.size() % 2 == 0) {
	    // take each pair as a segment
	    for (HitList::iterator i = hits.begin(); i != hits.end(); ++i) {
		brep_hit& in = *i;
		i++;
		brep_hit& out = *i;
		
		register struct seg* segp;
		RT_GET_SEG(segp, ap->a_resource);
		segp->seg_stp = stp;

		VMOVE(segp->seg_in.hit_point, in.point);
		VMOVE(segp->seg_in.hit_normal, in.normal);
		segp->seg_in.hit_dist = DIST_PT_PT(rp->r_pt,in.point);
		segp->seg_in.hit_surfno = in.face.m_face_index;
		VSET(segp->seg_in.hit_vpriv,in.uv[0],in.uv[1],0.0);

		VMOVE(segp->seg_out.hit_point, out.point);
		VMOVE(segp->seg_out.hit_normal, out.normal);
		segp->seg_out.hit_dist = DIST_PT_PT(rp->r_pt,out.point);
		segp->seg_out.hit_surfno = out.face.m_face_index;
		VSET(segp->seg_out.hit_vpriv,out.uv[0],out.uv[1],0.0);

		BU_LIST_INSERT( &(seghead->l), &(segp->l) );		
	    }
	    hit = true;
	} 
	else {
	    int num = 0;
	    for (HitList::iterator i = hits.begin(); i != hits.end(); i++) {
	        TRACE("hit " << num << ": " << ON_PRINT3(i->point));
		num++;
	    }
	}
    }
    
    return (hit) ? hits.size() : 0; // MISS
}


/**
 *  			R T _ B R E P _ N O R M
 *
 *  Given ONE ray distance, return the normal and entry/exit point.
 */
void
rt_brep_norm(register struct hit *hitp, struct soltab *stp, register struct xray *rp)
{
    
}


/**
 *			R T _ B R E P _ C U R V E
 *
 *  Return the curvature of the nurb.
 */
void
rt_brep_curve(register struct curvature *cvp, register struct hit *hitp, struct soltab *stp)
{
    /* XXX todo */
}

/**
 *                      R T _ B R E P _ C L A S S
 *
 *  Don't know what this is supposed to do...
 *
 *  Looking at g_arb.c, seems the actual signature is:
 *    class(const struct soltab* stp,
 *          const fastf_t* min,
 *          const fastf_t* max,
 *          const struct bn_tol* tol)
 *
 *  Hmmm...
 */
int
rt_brep_class()
{
  return RT_CLASSIFY_UNIMPLEMENTED;
}


/**
 *  			R T _ B R E P _ U V
 *
 *  For a hit on the surface of an nurb, return the (u,v) coordinates
 *  of the hit point, 0 <= u,v <= 1.
 *  u = azimuth
 *  v = elevation
 */
void
rt_brep_uv(struct application *ap, struct soltab *stp, register struct hit *hitp, register struct uvcoord *uvp)
{
    uvp->uv_u = hitp->hit_vpriv[0];
    uvp->uv_v = hitp->hit_vpriv[1];
}


/**
 *		R T _ B R E P _ F R E E
 */
void
rt_brep_free(register struct soltab *stp)
{
    TRACE1("rt_brep_free");
    struct brep_specific* bs = (struct brep_specific*)stp->st_specific;
    brep_specific_delete(bs);
}


void
plot_bbnode(BBNode* node, struct bu_list* vhead) {
  ON_3dPoint min = node->m_node.m_min;
  ON_3dPoint max = node->m_node.m_max;
  point_t verts[] = {{min[0],min[1],min[2]},
		     {min[0],max[1],min[2]},
		     {min[0],max[1],max[2]},
		     {min[0],min[1],max[2]},
		     {max[0],min[1],min[2]},
		     {max[0],max[1],min[2]},
		     {max[0],max[1],max[2]},
		     {max[0],min[1],max[2]}};

  if (node->isLeaf()) {

  for (int i = 0; i <= 4; i++) {
    RT_ADD_VLIST(vhead, verts[i%4], (i == 0) ? BN_VLIST_LINE_MOVE : BN_VLIST_LINE_DRAW);
  }
  for (int i = 0; i <= 4; i++) {
    RT_ADD_VLIST(vhead, verts[(i%4)+4], (i == 0) ? BN_VLIST_LINE_MOVE : BN_VLIST_LINE_DRAW);
  }
  for (int i = 0; i < 4; i++) {
    RT_ADD_VLIST(vhead, verts[i], BN_VLIST_LINE_MOVE);
    RT_ADD_VLIST(vhead, verts[i+4], BN_VLIST_LINE_DRAW);
  }

  }

  for (int i = 0; i < node->m_children.size(); i++) {
    plot_bbnode(node->m_children[i], vhead);
  }
}

/**
 *			R T _ B R E P _ P L O T
 */
int
rt_brep_plot(struct bu_list *vhead, struct rt_db_internal *ip, const struct rt_tess_tol *ttol, const struct bn_tol *tol)
{
    TRACE1("rt_brep_plot");
    struct rt_brep_internal* bi;

    RT_CK_DB_INTERNAL(ip);
    bi = (struct rt_brep_internal*)ip->idb_ptr;
    RT_BREP_CK_MAGIC(bi);

    // XXX below does NOT work for non-trivial faces, in addition to
    // the fact that openNURBS does NOT support meshes! 
    //
    // XXX currently not handling the tolerance
    //     ON_MeshParameters mp;
    //     mp.JaggedAndFasterMeshParameters();
    
    //     ON_SimpleArray<ON_Mesh*> mesh_list;
    //     bi->brep->CreateMesh(mp, mesh_list);
    
    //     point_t pt1, pt2;
    //     ON_SimpleArray<ON_2dex> edges;
    //     for (int i = 0; i < mesh_list.Count(); i++) {
    // 	const ON_Mesh* mesh = mesh_list[i];
    // 	mesh->GetMeshEdges(edges);
    // 	for (int j = 0; j < edges.Count(); j++) {
    // 	    ON_MeshVertexRef v1 = mesh->VertexRef(edges[j].i);
    // 	    ON_MeshVertexRef v2 = mesh->VertexRef(edges[j].j);
    // 	    VSET(pt1, v1.Point().x, v1.Point().y, v1.Point().z);
    // 	    VSET(pt2, v2.Point().x, v2.Point().y, v2.Point().z);
    // 	    RT_ADD_VLIST(vhead, pt1, BN_VLIST_LINE_MOVE);
    // 	    RT_ADD_VLIST(vhead, pt2, BN_VLIST_LINE_DRAW);
    // 	}
    // 	edges.Empty();
    //     }

    // So we'll do it by hand by grabbing each topological edge from
    // the brep and rendering it that way...
    ON_Brep* brep = bi->brep;    

    point_t pt1, pt2;
//     for (int i = 0; i < brep->m_F.Count(); i++) {
//       ON_BrepFace& f = brep->m_F[i];
//       SurfaceTree st(&f);
//       plot_bbnode(st.getRootNode(), vhead);
//     }

    for (int i = 0; i < bi->brep->m_E.Count(); i++) {
	ON_BrepEdge& e = brep->m_E[i];
	const ON_Curve* crv = e.EdgeCurveOf();

	if (crv->IsLinear()) {
	    ON_BrepVertex& v1 = brep->m_V[e.m_vi[0]];
	    ON_BrepVertex& v2 = brep->m_V[e.m_vi[1]];
	    VMOVE(pt1, v1.Point());
	    VMOVE(pt2, v2.Point());
	    RT_ADD_VLIST(vhead, pt1, BN_VLIST_LINE_MOVE);
	    RT_ADD_VLIST(vhead, pt2, BN_VLIST_LINE_DRAW);
	} else {
	    ON_Interval dom = crv->Domain();
	    // XXX todo: dynamically sample the curve
	    for (int i = 0; i <= 10; i++) {
		ON_3dPoint p = crv->PointAt(dom.ParameterAt((double)i/10.0));
		VMOVE(pt1, p);
		if (i == 0) {
		    RT_ADD_VLIST(vhead, pt1, BN_VLIST_LINE_MOVE);
		}
		else {
		    RT_ADD_VLIST(vhead, pt1, BN_VLIST_LINE_DRAW);
		}
	    }
	}
    }
    
    return 0;
}


/**
 *			R T _ B R E P _ T E S S
 */
int
rt_brep_tess(struct nmgregion **r, struct model *m, struct rt_db_internal *ip, const struct rt_tess_tol *ttol, const struct bn_tol *tol)
{
    return -1;
}


/* XXX In order to facilitate exporting the ON_Brep object without a
 * whole lot of effort, we're going to (for now) extend the
 * ON_BinaryArchive to support an "in-memory" representation of a
 * binary archive. Currently, the openNURBS library only supports
 * file-based archiving operations. This implies the
 */

class ON_CLASS RT_MemoryArchive : public ON_BinaryArchive
{
public:
  RT_MemoryArchive();
  RT_MemoryArchive(genptr_t memory, size_t len);
  virtual ~RT_MemoryArchive();

  // ON_BinaryArchive overrides
  size_t CurrentPosition() const;
  bool SeekFromCurrentPosition(int);
  bool SeekFromStart(size_t);
  bool AtEnd() const;

  size_t Size() const;
  /**
   * Generate a byte-array copy of this memory archive.
   * Allocates memory using bu_malloc, so must be freed with bu_free
   */
  genptr_t CreateCopy() const;

protected:
  size_t Read(size_t, void*);
  size_t Write(size_t, const void*);
  bool Flush();

private:
  size_t pos;
  std::vector<char> m_buffer;
};

RT_MemoryArchive::RT_MemoryArchive()
  : ON_BinaryArchive(ON::write3dm), pos(0)
{
}

RT_MemoryArchive::RT_MemoryArchive(genptr_t memory, size_t len)
  : ON_BinaryArchive(ON::read3dm), pos(0)
{
  m_buffer.reserve(len);
  for (int i = 0; i < len; i++) {
    m_buffer.push_back(((char*)memory)[i]);
  }
}

RT_MemoryArchive::~RT_MemoryArchive()
{
}

size_t
RT_MemoryArchive::CurrentPosition() const
{
  return pos;
}

bool
RT_MemoryArchive::SeekFromCurrentPosition(int seek_to)
{
  if (pos + seek_to > m_buffer.size()) return false;
  pos += seek_to;
  return true;
}

bool
RT_MemoryArchive::SeekFromStart(size_t seek_to)
{
  if (seek_to > m_buffer.size()) return false;
  pos = seek_to;
  return true;
}

bool
RT_MemoryArchive::AtEnd() const
{
  return pos >= m_buffer.size();
}

size_t
RT_MemoryArchive::Size() const
{
  return m_buffer.size();
}

genptr_t
RT_MemoryArchive::CreateCopy() const
{
  genptr_t memory = (genptr_t)bu_malloc(m_buffer.size()*sizeof(char),"rt_memoryarchive createcopy");
  const int size = m_buffer.size();
  for (int i = 0; i < size; i++) {
    ((char*)memory)[i] = m_buffer[i];
  }
  return memory;
}

size_t
RT_MemoryArchive::Read(size_t amount, void* buf)
{
    const int read_amount = (pos + amount > m_buffer.size()) ? m_buffer.size()-pos : amount;
    const int start = pos;
    for (; pos < (start+read_amount); pos++) {
	((char*)buf)[pos-start] = m_buffer[pos];
    }
    return read_amount;
}

size_t
RT_MemoryArchive::Write(const size_t amount, const void* buf)
{
    // the write can come in at any position!
    const int start = pos;
    // resize if needed to support new data
    if (m_buffer.size() < (start+amount)) {
	m_buffer.resize(start+amount);
    }
    for (; pos < (start+amount); pos++) {
	m_buffer[pos] = ((char*)buf)[pos-start];
    }
    return amount;
}

bool
RT_MemoryArchive::Flush()
{
  return true;
}

#include <iostream>
/**
 *			R T _ B R E P _ E X P O R T 5
 */
int
rt_brep_export5(struct bu_external *ep, const struct rt_db_internal *ip, double local2mm, const struct db_i *dbip)
{
    TRACE1("rt_brep_export5");
    struct rt_brep_internal* bi;

    RT_CK_DB_INTERNAL(ip);
    if (ip->idb_type != ID_BREP) return -1;
    bi = (struct rt_brep_internal*)ip->idb_ptr;
    RT_BREP_CK_MAGIC(bi);

    BU_INIT_EXTERNAL(ep);

    RT_MemoryArchive archive;
    /* XXX what to do about the version */
    ONX_Model model;

    {
	ON_Layer default_layer;
	default_layer.SetLayerIndex(0);
	default_layer.SetLayerName("Default");
	model.m_layer_table.Reserve(1);
	model.m_layer_table.Append(default_layer);
    }

    ONX_Model_Object& mo = model.m_object_table.AppendNew();
    mo.m_object = bi->brep;
    mo.m_attributes.m_layer_index = 0;
    mo.m_attributes.m_name = "brep";
    fprintf(stderr, "m_object_table count: %d\n", model.m_object_table.Count());

    model.m_properties.m_RevisionHistory.NewRevision();
    model.m_properties.m_Application.m_application_name = "BRL-CAD B-Rep primitive";

    model.Polish();
    ON_TextLog err(stderr);
    bool ok = model.Write(archive, 4, "export5", &err);
    if (ok) {
	ep->ext_nbytes = archive.Size();
	ep->ext_buf = archive.CreateCopy();
	return 0;
    } else {
	return -1;
    }
}


/**
 *			R T _ B R E P _ I M P O R T 5
 */
int
rt_brep_import5(struct rt_db_internal *ip, const struct bu_external *ep, register const fastf_t *mat, const struct db_i *dbip)
{
    ON::Begin();
    TRACE1("rt_brep_import5");
    struct rt_brep_internal* bi;
    BU_CK_EXTERNAL(ep);
    RT_CK_DB_INTERNAL(ip);
    ip->idb_major_type = DB5_MAJORTYPE_BRLCAD;
    ip->idb_type = ID_BREP;
    ip->idb_meth = &rt_functab[ID_BREP];
    ip->idb_ptr = bu_malloc(sizeof(struct rt_brep_internal), "rt_brep_internal");
    
    bi = (struct rt_brep_internal*)ip->idb_ptr;
    bi->magic = RT_BREP_INTERNAL_MAGIC;
    
    RT_MemoryArchive archive(ep->ext_buf, ep->ext_nbytes);
    ONX_Model model;
    ON_TextLog dump(stdout);
    //archive.Dump3dmChunk(dump);
    model.Read(archive, &dump);
    
    if (model.IsValid(&dump)) {
	ONX_Model_Object mo = model.m_object_table[0];
	// XXX does openNURBS force us to copy? it seems the answer is
	// YES due to the const-ness
	bi->brep = new ON_Brep(*ON_Brep::Cast(mo.m_object));
	return 0;
    } else {
	return -1;
    }
}


/**
 *			R T _ B R E P _ I F R E E
 */
void
rt_brep_ifree(struct rt_db_internal *ip)
{
    TRACE1("rt_brep_ifree");
    struct rt_brep_internal* bi;
    RT_CK_DB_INTERNAL(ip);
    bi = (struct rt_brep_internal*)ip->idb_ptr;
    RT_BREP_CK_MAGIC(bi);
    if (bi->brep != NULL)
	delete bi->brep;
    bu_free(bi, "rt_brep_internal free");
    ip->idb_ptr = GENPTR_NULL;
}


/**
 *			R T _ B R E P _ D E S C R I B E
 */
int
rt_brep_describe(struct bu_vls *str, const struct rt_db_internal *ip, int verbose, double mm2local)
{
    return 0;
}

/**
 *                      R T _ B R E P _ T C L G E T
 */
int
rt_brep_tclget(Tcl_Interp *interp, const struct rt_db_internal *intern, const char *attr)
{
    return 0;
}


/**
 *                      R T _ B R E P _ T C L A D J U S T
 */
int
rt_brep_tcladjust(Tcl_Interp *interp, struct rt_db_internal *intern, int argc, char **argv)
{
    return 0;
}

/** @} */

/*
 * Local Variables:
 * mode: C++
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
