/*                        C D T . C P P
 * BRL-CAD
 *
 * Copyright (c) 2007-2019 United States Government as represented by
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
/** @addtogroup libbrep */
/** @{ */
/** @file cdt.cpp
 *
 * Constrained Delaunay Triangulation of NURBS B-Rep objects.
 *
 */

/* TODO:
 *
 * Refactor checking routines (degen triangles, zero area triangles, normal checks)
 * into routines that can run per-face if possible.
 *
 * Need a process for how to modify edges and faces, and in what order.  Need to
 * come up with heuristics for applying different "fixing" strategies:
 *
 * - remove problematic surface point from face tessellation
 * - insert new point(s) at midpoints of edges of problem face
 * - insert new point at the center of the problem face
 * - split edge involved with problem face and retessellate both faces on that edge
 *
 * Different strategies may be appropriate depending on the characteristics of the
 * "bad" triangle - for example, a triangle with all three brep normals nearly perpendicular
 * to the triangle normal and having only one non-edge point we should probably try to
 * handle by removing the surface point.  On the other hand, if the angle is not as
 * perpendicular and the surface area is significant, we might want to add a new point
 * on the longest edge of the triangle (which might be a shared edge with another face.)
 *
 * Also, consider whether to retessellate whole face or try to assemble "local" set of
 * faces involved, build bounding polygon and do new local tessellation.  Latter
 * is a lot more work but might avoid re-introducing new problems elsewhere in a
 * mesh during a full re-CDT.
 *
 * Need to be able to introduce new edge points in specific subranges of an edge.
 *
 * Try to come up with a system that is more systematic, rather than the somewhat
 * ad-hoc routines we have below.  Will have to be iterative, and we can't assume
 * faces that were previously "clean" but which have to be reprocessed due to a new
 * edge point have remained clean.
 *
 * Remember there will be a variety of per-face containers that need to be reset
 * for this operation to work - may want to rework cdt state container to be more
 * per-face sets as opposed to maps-of-sets per face index, so we can wipe and reset
 * a face more easily.
 *
 * These are actually the same operations that will be needed for resolving overlaps,
 * so this is worth solving correctly.
 */


#include "common.h"
#include "./cdt.h"

#define BREP_PLANAR_TOL 0.05

static void
Process_Loop_Edges(
	struct ON_Brep_CDT_State *s_cdt,
	ON_SimpleArray<BrepTrimPoint> *points,
	const ON_BrepFace &face,
	const ON_BrepLoop *loop,
	fastf_t max_dist
	)
{
    int trim_count = loop->TrimCount();

    for (int lti = 0; lti < trim_count; lti++) {
	ON_BrepTrim *trim = loop->Trim(lti);
	ON_BrepEdge *edge = trim->Edge();

	/* Provide 2D points for p2d, but we need to be aware that this will result
	 * in degenerate 3D faces that we need to filter out when assembling a mesh */
	if (trim->m_type == ON_BrepTrim::singular) {
	    BrepTrimPoint btp;
	    const ON_BrepVertex& v1 = face.Brep()->m_V[trim->m_vi[0]];
	    ON_3dPoint *p3d = (*s_cdt->vert_pnts)[v1.m_vertex_index];
	    (*s_cdt->faces)[face.m_face_index]->strim_pnts->insert(std::make_pair(trim->m_trim_index, p3d));
	    ON_3dPoint *n3d = (*s_cdt->vert_avg_norms)[v1.m_vertex_index];
	    if (n3d) {
		(*s_cdt->faces)[face.m_face_index]->strim_norms->insert(std::make_pair(trim->m_trim_index, n3d));
	    }
	    double delta =  trim->Domain().Length() / 10.0;
	    ON_Interval trim_dom = trim->Domain();

	    for (int i = 1; i <= 10; i++) {
		btp.p3d = p3d;
		btp.n3d = n3d;
		btp.p2d = v1.Point();
		btp.t = trim->Domain().m_t[0] + (i - 1) * delta;
		btp.p2d = trim->PointAt(btp.t);
		btp.e = ON_UNSET_VALUE;
		points->Append(btp);
	    }
	    // skip last point of trim if not last trim
	    if (lti < trim_count - 1)
		continue;

	    const ON_BrepVertex& v2 = face.Brep()->m_V[trim->m_vi[1]];
	    btp.p3d = p3d;
	    btp.n3d = n3d;
	    btp.p2d = v2.Point();
	    btp.t = trim->Domain().m_t[1];
	    btp.p2d = trim->PointAt(btp.t);
	    btp.e = ON_UNSET_VALUE;
	    points->Append(btp);

	    continue;
	}

	if (!trim->m_trim_user.p) {
	    (void)getEdgePoints(s_cdt, edge, *trim, max_dist, DBL_MAX);
	    //bu_log("Initialized trim->m_trim_user.p: Trim %d (associated with Edge %d) point count: %zd\n", trim->m_trim_index, trim->Edge()->m_edge_index, m->size());
	}
	if (trim->m_trim_user.p) {
	    std::map<double, BrepTrimPoint *> *param_points3d = (std::map<double, BrepTrimPoint *> *) trim->m_trim_user.p;
	    //bu_log("Trim %d (associated with Edge %d) point count: %zd\n", trim->m_trim_index, trim->Edge()->m_edge_index, param_points3d->size());

	    ON_3dPoint boxmin;
	    ON_3dPoint boxmax;

	    if (trim->GetBoundingBox(boxmin, boxmax, false)) {
		double t0, t1;

		std::map<double, BrepTrimPoint*>::const_iterator i;
		std::map<double, BrepTrimPoint*>::const_iterator ni;

		trim->GetDomain(&t0, &t1);
		for (i = param_points3d->begin(); i != param_points3d->end();) {
		    BrepTrimPoint *btp = (*i).second;
		    ni = ++i;
		    // skip last point of trim if not last trim
		    if (ni == param_points3d->end()) {
			if (lti < trim_count - 1) {
			    continue;
			}
		    }
		    points->Append(*btp);
		}
	    }
	}
    }
}


static int
ON_Brep_CDT_Face(struct ON_Brep_CDT_Face_State *f, std::map<const ON_Surface *, double> *s_to_maxdist)
{
    struct ON_Brep_CDT_State *s_cdt = f->s_cdt;
    int face_index = f->ind;
    ON_RTree rt_trims;
    ON_2dPointArray on_surf_points;
    ON_BrepFace &face = f->s_cdt->brep->m_F[face_index];
    const ON_Surface *s = face.SurfaceOf();
    int loop_cnt = face.LoopCount();
    ON_2dPointArray on_loop_points;
    ON_SimpleArray<BrepTrimPoint> *brep_loop_points = new ON_SimpleArray<BrepTrimPoint>[loop_cnt];
    std::map<p2t::Point *, ON_3dPoint *> *pointmap = (*s_cdt->faces)[face.m_face_index]->p2t_to_on3_map;
    std::map<ON_3dPoint *, std::set<p2t::Point *>> *on3_to_tri = (*s_cdt->faces)[face.m_face_index]->on3_to_tri_map;
    std::map<p2t::Point *, ON_3dPoint *> *normalmap = (*s_cdt->faces)[face_index]->p2t_to_on3_norm_map;
    std::vector<p2t::Point*> polyline;
    p2t::CDT* cdt = NULL;

    // Use the edge curves and loops to generate an initial set of trim polygons.
    for (int li = 0; li < loop_cnt; li++) {
	double max_dist = 0.0;
	const ON_BrepLoop *loop = face.Loop(li);
	if (s_to_maxdist->find(face.SurfaceOf()) != s_to_maxdist->end()) {
	    max_dist = (*s_to_maxdist)[face.SurfaceOf()];
	}
	Process_Loop_Edges(s_cdt, &brep_loop_points[li], face, loop, max_dist);
    }

    // Handle a variety of situations that complicate loop handling on closed surfaces
    std::list<std::map<double, ON_3dPoint *> *> bridgePoints;
    if (s->IsClosed(0) || s->IsClosed(1)) {
	PerformClosedSurfaceChecks(s_cdt, s, face, brep_loop_points, BREP_SAME_POINT_TOLERANCE);
    }

    // Find for this face, find the minimum and maximum edge polyline segment lengths
    (*s_cdt->max_edge_seg_len)[face.m_face_index] = 0.0;
    (*s_cdt->min_edge_seg_len)[face.m_face_index] = DBL_MAX;
    for (int li = 0; li < loop_cnt; li++) {
	int num_loop_points = brep_loop_points[li].Count();
	if (num_loop_points > 1) {
	    ON_3dPoint *p1 = (brep_loop_points[li])[0].p3d;
	    ON_3dPoint *p2 = NULL;
	    for (int i = 1; i < num_loop_points; i++) {
		p2 = p1;
		p1 = (brep_loop_points[li])[i].p3d;
		fastf_t dist = p1->DistanceTo(*p2);
		if (dist > (*s_cdt->max_edge_seg_len)[face.m_face_index]) {
		    (*s_cdt->max_edge_seg_len)[face.m_face_index] = dist;
		}
		if ((dist > SMALL_FASTF) && (dist < (*s_cdt->min_edge_seg_len)[face.m_face_index]))  {
		    (*s_cdt->min_edge_seg_len)[face.m_face_index] = dist;
		}
	    }
	}
    }

    // Process through loops, building Poly2Tri polygons for facetization.
    bool outer = true;
    for (int li = 0; li < loop_cnt; li++) {
	int num_loop_points = brep_loop_points[li].Count();
	if (num_loop_points > 2) {
	    for (int i = 1; i < num_loop_points; i++) {
		// map point to last entry to 3d point
		p2t::Point *p = new p2t::Point((brep_loop_points[li])[i].p2d.x, (brep_loop_points[li])[i].p2d.y);
		polyline.push_back(p);
		(*((*s_cdt->faces)[face.m_face_index]->p2t_trim_ind))[p] = (brep_loop_points[li])[i].trim_ind;
		(*pointmap)[p] = (brep_loop_points[li])[i].p3d;
		(*on3_to_tri)[(brep_loop_points[li])[i].p3d].insert(p);
		(*normalmap)[p] = (brep_loop_points[li])[i].n3d;
	    }
	    for (int i = 1; i < brep_loop_points[li].Count(); i++) {
		// map point to last entry to 3d point
		ON_Line *line = new ON_Line((brep_loop_points[li])[i - 1].p2d, (brep_loop_points[li])[i].p2d);
		ON_BoundingBox bb = line->BoundingBox();

		bb.m_max.x = bb.m_max.x + ON_ZERO_TOLERANCE;
		bb.m_max.y = bb.m_max.y + ON_ZERO_TOLERANCE;
		bb.m_max.z = bb.m_max.z + ON_ZERO_TOLERANCE;
		bb.m_min.x = bb.m_min.x - ON_ZERO_TOLERANCE;
		bb.m_min.y = bb.m_min.y - ON_ZERO_TOLERANCE;
		bb.m_min.z = bb.m_min.z - ON_ZERO_TOLERANCE;

		rt_trims.Insert2d(bb.Min(), bb.Max(), line);
	    }
	    if (outer) {
		cdt = new p2t::CDT(polyline);
		outer = false;
	    } else {
		cdt->AddHole(polyline);
	    }
	    polyline.clear();
	}
    }
    // Using this in surface calculations, so assign now
    (*s_cdt->faces)[face.m_face_index]->face_loop_points = brep_loop_points;

    // TODO - we may need to add 2D points on trims that the edges didn't know
    // about.  Since 3D points must be shared along edges and we're using
    // synchronized numbers of parametric domain ordered 2D and 3D points to
    // make that work, we will need to track new 2D points and update the
    // corresponding 3D edge based data structures.  More than that - we must
    // also update all 2D structures in all other faces associated with the
    // edge in question to keep things in overall sync.

    // TODO - if we are going to apply clipper boolean resolution to sets of
    // face loops, that would come here - once we have assembled the loop
    // polygons for the faces. That also has the potential to generate "new" 3D
    // points on edges that are driven by 2D boolean intersections between
    // trimming loops, and may require another update pass as above.

    if (outer) {
	std::cerr << "Error: Face(" << face.m_face_index << ") cannot evaluate its outer loop and will not be facetized." << std::endl;
	return -1;
    }

    // Sample the surface, independent of the trimming curves, to get points that
    // will tie the mesh to the interior surface.
    getSurfacePoints(s_cdt, face, on_surf_points, &rt_trims);

    // Strip out points from the surface that are on the trimming curves.  Trim
    // points require special handling for watertightness and introducing them
    // from the surface also runs the risk of adding duplicate 2D points, which
    // aren't allowed for facetization.
    for (int i = 0; i < on_surf_points.Count(); i++) {
	ON_SimpleArray<void*> results;
	const ON_2dPoint *p = on_surf_points.At(i);

	rt_trims.Search2d((const double *) p, (const double *) p, results);

	if (results.Count() > 0) {
	    bool on_edge = false;
	    for (int ri = 0; ri < results.Count(); ri++) {
		double dist;
		const ON_Line *l = (const ON_Line *) *results.At(ri);
		dist = l->MinimumDistanceTo(*p);
		if (NEAR_ZERO(dist, s_cdt->dist)) {
		    on_edge = true;
		    break;
		}
	    }
	    if (!on_edge) {
		cdt->AddPoint(new p2t::Point(p->x, p->y));
	    }
	} else {
	    cdt->AddPoint(new p2t::Point(p->x, p->y));
	}
    }

    ON_SimpleArray<void*> results;
    ON_BoundingBox bb = rt_trims.BoundingBox();

    rt_trims.Search2d((const double *) bb.m_min, (const double *) bb.m_max,
	    results);

    if (results.Count() > 0) {
	for (int ri = 0; ri < results.Count(); ri++) {
	    const ON_Line *l = (const ON_Line *)*results.At(ri);
	    delete l;
	}
    }
    rt_trims.RemoveAll();

    // All preliminary steps are complete, perform the triangulation using
    // Poly2Tri's triangulation.  NOTE: it is important that the inputs to
    // Poly2Tri satisfy its constraints - failure here could cause a crash.
    cdt->Triangulate(true, -1);

    /* Calculate any 3D points we don't already have from the loop processing */
    std::vector<p2t::Triangle*> tris = cdt->GetTriangles();
    for (size_t i = 0; i < tris.size(); i++) {

	p2t::Triangle *t = tris[i];

	for (size_t j = 0; j < 3; j++) {

	    p2t::Point *p = t->GetPoint(j);
	    if (!p) {
		bu_log("Face %d: p2t face without proper point info...\n", face.m_face_index);
		continue;
	    }

	    ON_3dPoint *op = (*pointmap)[p];
	    ON_3dPoint *onorm = (*normalmap)[p];
	    if (op && onorm) {
		// We have what we need
		continue;
	    }

	    const ON_Surface *surf = face.SurfaceOf();
	    ON_3dPoint pnt;
	    ON_3dVector norm;
	    if (surface_EvNormal(surf, p->x, p->y, pnt, norm)) {
		// Have point and normal from surface - store
		if (face.m_bRev) {
		    norm = norm * -1.0;
		}
		if (!op) {
		    op = new ON_3dPoint(pnt);
		    CDT_Add3DPnt(s_cdt, op, face.m_face_index, -1, -1, -1, p->x, p->y);
		    (*pointmap)[p] = op;
		}
		if (!onorm) {
		    onorm = new ON_3dPoint(norm);
		    s_cdt->w3dnorms->push_back(onorm);
		    (*normalmap)[p] = onorm;
		}
	    } else {
		// surface_EvNormal failed - fall back on PointAt
		if (!op) {
		    pnt = s->PointAt(p->x, p->y);
		    op = new ON_3dPoint(pnt);
		    CDT_Add3DPnt(s_cdt, op, face.m_face_index, -1, -1, -1, p->x, p->y);
		}
		(*pointmap)[p] = op;
	    }

	}

    }

    /* Stash mappings for BoT reassembly.  Because there may be subsequent
     * refinement in overlap clearing operations, we avoid immediately
     * generating the mesh. */
    (*s_cdt->faces)[face_index]->cdt = cdt;
    return 0;
}

static ON_3dVector
calc_trim_vnorm(ON_BrepVertex& v, ON_BrepTrim *trim)
{
    ON_3dPoint t1, t2;
    ON_3dVector v1 = ON_3dVector::UnsetVector;
    ON_3dVector v2 = ON_3dVector::UnsetVector;
    ON_3dVector trim_norm = ON_3dVector::UnsetVector;

    ON_Interval trange = trim->Domain();
    ON_3dPoint t_2d1 = trim->PointAt(trange[0]);
    ON_3dPoint t_2d2 = trim->PointAt(trange[1]);

    ON_Plane fplane;
    const ON_Surface *s = trim->SurfaceOf();
    if (s->IsPlanar(&fplane, BREP_PLANAR_TOL)) {
	trim_norm = fplane.Normal();
	if (trim->Face()->m_bRev) {
	    trim_norm = trim_norm * -1;
	}
    } else {
	int ev1 = 0;
	int ev2 = 0;
	if (surface_EvNormal(s, t_2d1.x, t_2d1.y, t1, v1)) {
	    if (trim->Face()->m_bRev) {
		v1 = v1 * -1;
	    }
	    if (v.Point().DistanceTo(t1) < ON_ZERO_TOLERANCE) {
		ev1 = 1;
		trim_norm = v1;
	    }
	}
	if (surface_EvNormal(s, t_2d2.x, t_2d2.y, t2, v2)) {
	    if (trim->Face()->m_bRev) {
		v2 = v2 * -1;
	    }
	    if (v.Point().DistanceTo(t2) < ON_ZERO_TOLERANCE) {
		ev2 = 1;
		trim_norm = v2;
	    }
	}
	// If we got both of them, go with the closest one
	if (ev1 && ev2) {
#if 0
	    if (ON_DotProduct(v1, v2) < 0) {
		bu_log("Vertex %d(%f %f %f), trim %d: got both normals\n", v.m_vertex_index, v.Point().x, v.Point().y, v.Point().z, trim->m_trim_index);
		bu_log("v1(%f)(%f %f)(%f %f %f): %f %f %f\n", v.Point().DistanceTo(t1), t_2d1.x, t_2d1.y, t1.x, t1.y, t1.z, v1.x, v1.y, v1.z);
		bu_log("v2(%f)(%f %f)(%f %f %f): %f %f %f\n", v.Point().DistanceTo(t2), t_2d2.x, t_2d2.y, t2.x, t2.y, t2.z, v2.x, v2.y, v2.z);
	    }
#endif
	    trim_norm = (v.Point().DistanceTo(t1) < v.Point().DistanceTo(t2)) ? v1 : v2;
	}
    }

    return trim_norm;
}

int
ON_Brep_CDT_Tessellate(struct ON_Brep_CDT_State *s_cdt, int face_cnt, int *faces)
{

    ON_wString wstr;
    ON_TextLog tl(wstr);

    // Check for any conditions that are show-stoppers
    ON_wString wonstr;
    ON_TextLog vout(wonstr);
    if (!s_cdt->orig_brep->IsValid(&vout)) {
	bu_log("brep is NOT valid, cannot produce watertight mesh\n");
	//return -1;
    }

    // For now, edges must have 2 and only 2 trims for this to work.
    for (int index = 0; index < s_cdt->orig_brep->m_E.Count(); index++) {
        ON_BrepEdge& edge = s_cdt->orig_brep->m_E[index];
        if (edge.TrimCount() != 2) {
	    bu_log("Edge %d trim count: %d - can't (yet) do watertight meshing\n", edge.m_edge_index, edge.TrimCount());
            return -1;
        }
    }

    // We may be changing the ON_Brep data, so work on a copy
    // rather than the original object
    if (!s_cdt->brep) {
	s_cdt->brep = new ON_Brep(*s_cdt->orig_brep);
    }
    ON_Brep* brep = s_cdt->brep;

    // Have observed at least one case (NIST2 face 237) where a small face on a
    // large surface resulted in a valid 2D triangulation that produced a
    // self-intersecting 3D mesh.  Attempt to minimize situations where 2D and
    // 3D distances get out of sync by shrinking the surfaces down to the
    // active area of the face
    //
    // TODO - we may want to do this only for faces where the surface size is
    // much larger than the bounded trimming curves in 2D, rather than just
    // shrinking everything...
    brep->ShrinkSurfaces();

    // Reparameterize the face's surface and transform the "u" and "v"
    // coordinates of all the face's parameter space trimming curves to
    // minimize distortion in the map from parameter space to 3d..
    if (!s_cdt->w3dpnts->size()) {
	for (int face_index = 0; face_index < brep->m_F.Count(); face_index++) {
	    ON_BrepFace *face = brep->Face(face_index);
	    const ON_Surface *s = face->SurfaceOf();
	    double surface_width, surface_height;
	    if (s->GetSurfaceSize(&surface_width, &surface_height)) {
		face->SetDomain(0, 0.0, surface_width);
		face->SetDomain(1, 0.0, surface_height);
	    }
	}
    }

    /* We want to use ON_3dPoint pointers and BrepVertex points, but
     * vert->Point() produces a temporary address.  If this is our first time
     * through, make stable copies of the Vertex points. */
    if (!s_cdt->w3dpnts->size()) {
	for (int index = 0; index < brep->m_V.Count(); index++) {
	    ON_BrepVertex& v = brep->m_V[index];
	    (*s_cdt->vert_pnts)[index] = new ON_3dPoint(v.Point());
	    CDT_Add3DPnt(s_cdt, (*s_cdt->vert_pnts)[index], -1, v.m_vertex_index, -1, -1, -1, -1);
	    // topologically, any vertex point will be on edges
	    s_cdt->edge_pnts->insert((*s_cdt->vert_pnts)[index]);
	}
    }

    /* If this is the first time through, get vertex normals that are the
     * average of the surface normals at the junction from faces that don't use
     * a singular trim to reference the vertex.  When subdividing the edges, we
     * use the normals as a curvature guide.
     */
    if (!s_cdt->vert_avg_norms->size()) {
	for (int index = 0; index < brep->m_V.Count(); index++) {
	    ON_BrepVertex& v = brep->m_V[index];
	    int have_calculated = 0;
	    ON_3dVector vnrml(0.0, 0.0, 0.0);

	    for (int eind = 0; eind != v.EdgeCount(); eind++) {
		ON_3dVector trim1_norm = ON_3dVector::UnsetVector;
		ON_3dVector trim2_norm = ON_3dVector::UnsetVector;
		ON_BrepEdge& edge = brep->m_E[v.m_ei[eind]];
		if (edge.TrimCount() != 2) {
		    // Don't know what to do with this yet... skip.
		    continue;
		}
		ON_BrepTrim *trim1 = edge.Trim(0);
		ON_BrepTrim *trim2 = edge.Trim(1);

		if (trim1->m_type != ON_BrepTrim::singular) {
		    trim1_norm = calc_trim_vnorm(v, trim1);
		}
		if (trim2->m_type != ON_BrepTrim::singular) {
		    trim2_norm = calc_trim_vnorm(v, trim2);
		}

		// If one of the normals is unset and the other comes from a plane, use it
		if (trim1_norm == ON_3dVector::UnsetVector && trim2_norm != ON_3dVector::UnsetVector) {
		    const ON_Surface *s2 = trim2->SurfaceOf();
		    if (!s2->IsPlanar(NULL, ON_ZERO_TOLERANCE)) {
			continue;
		    }
		    trim1_norm = trim2_norm;
		}
		if (trim1_norm != ON_3dVector::UnsetVector && trim2_norm == ON_3dVector::UnsetVector) {
		    const ON_Surface *s1 = trim1->SurfaceOf();
		    if (!s1->IsPlanar(NULL, ON_ZERO_TOLERANCE)) {
			continue;
		    }
		    trim2_norm = trim1_norm;
		}

		// If we have disagreeing normals and one of them is from a planar surface, go
		// with that one
		if (NEAR_EQUAL(ON_DotProduct(trim1_norm, trim2_norm), -1, VUNITIZE_TOL)) {
		    const ON_Surface *s1 = trim1->SurfaceOf();
		    const ON_Surface *s2 = trim2->SurfaceOf();
		    if (!s1->IsPlanar(NULL, ON_ZERO_TOLERANCE) && !s2->IsPlanar(NULL, ON_ZERO_TOLERANCE)) {
			// Normals severely disagree, no planar surface to fall back on - can't use this
			continue;
		    }
		    if (s1->IsPlanar(NULL, ON_ZERO_TOLERANCE) && s2->IsPlanar(NULL, ON_ZERO_TOLERANCE)) {
			// Two disagreeing planes - can't use this
			continue;
		    }
		    if (s1->IsPlanar(NULL, ON_ZERO_TOLERANCE)) {
			trim2_norm = trim1_norm;
		    }
		    if (s2->IsPlanar(NULL, ON_ZERO_TOLERANCE)) {
			trim1_norm = trim2_norm;
		    }
		}

		// Stash normals coming from non-singular trims at vertices for faces.  If a singular trim
		// needs a normal in 3D, want to use one of these
		int mfaceind1 = trim1->Face()->m_face_index;
		ON_3dPoint *t1pnt = new ON_3dPoint(trim1_norm);
		if (s_cdt->faces->find(mfaceind1) == s_cdt->faces->end()) {
		    struct ON_Brep_CDT_Face_State *f = ON_Brep_CDT_Face_Create(s_cdt, mfaceind1);
		    (*s_cdt->faces)[mfaceind1] = f;
		}
		(*(*s_cdt->faces)[mfaceind1]->vert_face_norms)[v.m_vertex_index].insert(t1pnt);
		(*s_cdt->faces)[mfaceind1]->w3dnorms->push_back(t1pnt);
		int mfaceind2 = trim2->Face()->m_face_index;
		ON_3dPoint *t2pnt = new ON_3dPoint(trim2_norm);
		if (s_cdt->faces->find(mfaceind2) == s_cdt->faces->end()) {
		    struct ON_Brep_CDT_Face_State *f = ON_Brep_CDT_Face_Create(s_cdt, mfaceind2);
		    (*s_cdt->faces)[mfaceind2] = f;
		}
		(*(*s_cdt->faces)[mfaceind2]->vert_face_norms)[v.m_vertex_index].insert(t1pnt);
		(*s_cdt->faces)[mfaceind2]->w3dnorms->push_back(t2pnt);

		// Add the normals to the vnrml total
		vnrml += trim1_norm;
		vnrml += trim2_norm;
		have_calculated = 1;

	    }
	    if (!have_calculated) {
		continue;
	    }

	    // Average all the successfully calculated normals into a new unit normal
	    vnrml.Unitize();

	    // We store this as a point to keep C++ happy...  If we try to
	    // propagate the ON_3dVector type through all the CDT logic it
	    // triggers issues with the compile.
	    (*s_cdt->vert_avg_norms)[index] = new ON_3dPoint(vnrml);
	    s_cdt->w3dnorms->push_back((*s_cdt->vert_avg_norms)[index]);
	}
    }

#if 0
    /* When we're working on edges, we need to have some sense of how fine we need
     * to go to make sure we don't end up with severely distorted triangles - use
     * the minimum length from the loop edges */
    for (int li = 0; li < s_cdt->brep->m_L.Count(); li++) {
	const ON_BrepLoop &loop = brep->m_L[li];
    }
#endif

    /* To generate watertight meshes, the faces must share 3D edge points.  To ensure
     * a uniform set of edge points, we first sample all the edges and build their
     * point sets */
    std::map<const ON_Surface *, double> s_to_maxdist;
    for (int index = 0; index < brep->m_E.Count(); index++) {
        ON_BrepEdge& edge = brep->m_E[index];
        ON_BrepTrim& trim1 = brep->m_T[edge.Trim(0)->m_trim_index];
        ON_BrepTrim& trim2 = brep->m_T[edge.Trim(1)->m_trim_index];

        // Get distance tolerances from the surface sizes
        fastf_t max_dist = 0.0;
        fastf_t min_dist = DBL_MAX;
	fastf_t mw, mh;
        fastf_t md1, md2 = 0.0;
        double sw1, sh1, sw2, sh2;
        const ON_Surface *s1 = trim1.Face()->SurfaceOf();
        const ON_Surface *s2 = trim2.Face()->SurfaceOf();
        if (s1->GetSurfaceSize(&sw1, &sh1) && s2->GetSurfaceSize(&sw2, &sh2)) {
            if ((sw1 < s_cdt->dist) || (sh1 < s_cdt->dist) || sw2 < s_cdt->dist || sh2 < s_cdt->dist) {
                return -1;
            }
            md1 = sqrt(sw1 * sh1 + sh1 * sw1) / 10.0;
            md2 = sqrt(sw2 * sh2 + sh2 * sw2) / 10.0;
            max_dist = (md1 < md2) ? md1 : md2;
            mw = (sw1 < sw2) ? sw1 : sw2;
            mh = (sh1 < sh2) ? sh1 : sh2;
            min_dist = (mw < mh) ? mw : mh;
            s_to_maxdist[s1] = max_dist;
            s_to_maxdist[s2] = max_dist;
        }

        // Generate the BrepTrimPoint arrays for both trims associated with this edge
	//
	// TODO - need to make this robust to changed tolerances on refinement
	// runs - if pre-existing solutions for "high level" splits exist,
	// reuse them and dig down to find where we need further refinement to
	// create new points.
        (void)getEdgePoints(s_cdt, &edge, trim1, max_dist, min_dist);

    }


    // Process all of the faces we have been instructed to process, or (default) all faces.
    // Keep track of failures and successes.
    int face_failures = 0;
    int face_successes = 0;

    if ((face_cnt == 0) || !faces) {

	for (int face_index = 0; face_index < s_cdt->brep->m_F.Count(); face_index++) {
	    if (s_cdt->faces->find(face_index) == s_cdt->faces->end()) {
		struct ON_Brep_CDT_Face_State *f = ON_Brep_CDT_Face_Create(s_cdt, face_index);
		(*s_cdt->faces)[face_index] = f;
	    }
	    if (ON_Brep_CDT_Face((*s_cdt->faces)[face_index],&s_to_maxdist)) {
		face_failures++;
	    } else {
		face_successes++;
	    }

	}
    } else {
	for (int i = 0; i < face_cnt; i++) {
	    if (faces[i] < s_cdt->brep->m_F.Count()) {
		if (s_cdt->faces->find(faces[i]) == s_cdt->faces->end()) {
		    struct ON_Brep_CDT_Face_State *f = ON_Brep_CDT_Face_Create(s_cdt, faces[i]);
		    (*s_cdt->faces)[faces[i]] = f;
		}
		if (ON_Brep_CDT_Face((*s_cdt->faces)[faces[i]], &s_to_maxdist)) {
		    face_failures++;
		} else {
		    face_successes++;
		}
	    }
	}
    }

    // If we only tessellated some of the faces, we don't have the
    // full solid mesh yet (by definition).  Return accordingly.
    if (face_failures || !face_successes || face_successes < s_cdt->brep->m_F.Count()) {
	return (face_successes) ? 1 : -1;
    }

    /* We've got face meshes and no reported failures - check to see if we have a
     * solid mesh */
    int valid_fcnt, valid_vcnt;
    int *valid_faces = NULL;
    fastf_t *valid_vertices = NULL;

    if (ON_Brep_CDT_Mesh(&valid_faces, &valid_fcnt, &valid_vertices, &valid_vcnt, NULL, NULL, NULL, NULL, s_cdt) < 0) {
	return -1;
    }

    struct bg_trimesh_solid_errors se = BG_TRIMESH_SOLID_ERRORS_INIT_NULL;
    int invalid = bg_trimesh_solid2(valid_vcnt, valid_fcnt, valid_vertices, valid_faces, &se);

    if (!invalid) {
	goto cdt_done;
    }

    if (se.degenerate.count > 0) {
	std::set<int> problem_pnts;
	std::set<int>::iterator pp_it;
	bu_log("%d degenerate faces\n", se.degenerate.count);
	for (int i = 0; i < se.degenerate.count; i++) {
	    int face = se.degenerate.faces[i];
	    bu_log("dface %d: %d %d %d :  %f %f %f->%f %f %f->%f %f %f \n", face, valid_faces[face*3], valid_faces[face*3+1], valid_faces[face*3+2],
		    valid_vertices[valid_faces[face*3]*3], valid_vertices[valid_faces[face*3]*3+1],valid_vertices[valid_faces[face*3]*3+2],
		    valid_vertices[valid_faces[face*3+1]*3], valid_vertices[valid_faces[face*3+1]*3+1],valid_vertices[valid_faces[face*3+1]*3+2],
		    valid_vertices[valid_faces[face*3+2]*3], valid_vertices[valid_faces[face*3+2]*3+1],valid_vertices[valid_faces[face*3+2]*3+2]);
	    problem_pnts.insert(valid_faces[face*3]);
	    problem_pnts.insert(valid_faces[face*3+1]);
	    problem_pnts.insert(valid_faces[face*3+2]);
	}
	for (pp_it = problem_pnts.begin(); pp_it != problem_pnts.end(); pp_it++) {
	    int pind = (*pp_it);
	    ON_3dPoint *p = (*s_cdt->vert_to_on)[pind];
	    if (!p) {
		bu_log("unmapped point??? %d\n", pind);
	    } else {
		struct cdt_audit_info *paudit = (*s_cdt->pnt_audit_info)[p];
		if (!paudit) {
		    bu_log("point with no audit info??? %d\n", pind);
		} else {
		    bu_log("point %d: Face(%d) Vert(%d) Trim(%d) Edge(%d) UV(%f,%f)\n", pind, paudit->face_index, paudit->vert_index, paudit->trim_index, paudit->edge_index, paudit->surf_uv.x, paudit->surf_uv.y);
		}
	    }
	}
    }
    if (se.excess.count > 0) {
	bu_log("extra edges???\n");
    }
    if (se.unmatched.count > 0) {
	std::set<int> problem_pnts;
	std::set<int>::iterator pp_it;

	bu_log("%d unmatched edges\n", se.unmatched.count);
	for (int i = 0; i < se.unmatched.count; i++) {
	    int v1 = se.unmatched.edges[i*2];
	    int v2 = se.unmatched.edges[i*2+1];
	    bu_log("%d->%d: %f %f %f->%f %f %f\n", v1, v2,
		    valid_vertices[v1*3], valid_vertices[v1*3+1], valid_vertices[v1*3+2],
		    valid_vertices[v2*3], valid_vertices[v2*3+1], valid_vertices[v2*3+2]
		  );
	    for (int j = 0; j < valid_fcnt; j++) {
		std::set<std::pair<int,int>> fedges;
		fedges.insert(std::pair<int,int>(valid_faces[j*3],valid_faces[j*3+1]));
		fedges.insert(std::pair<int,int>(valid_faces[j*3+1],valid_faces[j*3+2]));
		fedges.insert(std::pair<int,int>(valid_faces[j*3+2],valid_faces[j*3]));
		int has_edge = (fedges.find(std::pair<int,int>(v1,v2)) != fedges.end()) ? 1 : 0;
		if (has_edge) {
		    int face = j;
		    bu_log("eface %d: %d %d %d :  %f %f %f->%f %f %f->%f %f %f \n", face, valid_faces[face*3], valid_faces[face*3+1], valid_faces[face*3+2],
			    valid_vertices[valid_faces[face*3]*3], valid_vertices[valid_faces[face*3]*3+1],valid_vertices[valid_faces[face*3]*3+2],
			    valid_vertices[valid_faces[face*3+1]*3], valid_vertices[valid_faces[face*3+1]*3+1],valid_vertices[valid_faces[face*3+1]*3+2],
			    valid_vertices[valid_faces[face*3+2]*3], valid_vertices[valid_faces[face*3+2]*3+1],valid_vertices[valid_faces[face*3+2]*3+2]);
		    problem_pnts.insert(valid_faces[j*3]);
		    problem_pnts.insert(valid_faces[j*3+1]);
		    problem_pnts.insert(valid_faces[j*3+2]);
		}
	    }
	}
	for (pp_it = problem_pnts.begin(); pp_it != problem_pnts.end(); pp_it++) {
	    int pind = (*pp_it);
	    ON_3dPoint *p = (*s_cdt->vert_to_on)[pind];
	    if (!p) {
		bu_log("unmapped point??? %d\n", pind);
	    } else {
		struct cdt_audit_info *paudit = (*s_cdt->pnt_audit_info)[p];
		if (!paudit) {
		    bu_log("point with no audit info??? %d\n", pind);
		} else {
		    bu_log("point %d: Face(%d) Vert(%d) Trim(%d) Edge(%d) UV(%f,%f)\n", pind, paudit->face_index, paudit->vert_index, paudit->trim_index, paudit->edge_index, paudit->surf_uv.x, paudit->surf_uv.y);
		}
	    }
	}
    }
    if (se.misoriented.count > 0) {
	std::set<int> problem_pnts;
	std::set<int>::iterator pp_it;

	bu_log("%d misoriented edges\n", se.misoriented.count);
	for (int i = 0; i < se.misoriented.count; i++) {
	    int v1 = se.misoriented.edges[i*2];
	    int v2 = se.misoriented.edges[i*2+1];
	    bu_log("%d->%d: %f %f %f->%f %f %f\n", v1, v2,
		    valid_vertices[v1*3], valid_vertices[v1*3+1], valid_vertices[v1*3+2],
		    valid_vertices[v2*3], valid_vertices[v2*3+1], valid_vertices[v2*3+2]
		  );
	    for (int j = 0; j < valid_fcnt; j++) {
		std::set<std::pair<int,int>> fedges;
		fedges.insert(std::pair<int,int>(valid_faces[j*3],valid_faces[j*3+1]));
		fedges.insert(std::pair<int,int>(valid_faces[j*3+1],valid_faces[j*3+2]));
		fedges.insert(std::pair<int,int>(valid_faces[j*3+2],valid_faces[j*3]));
		int has_edge = (fedges.find(std::pair<int,int>(v1,v2)) != fedges.end()) ? 1 : 0;
		if (has_edge) {
		    int face = j;
		    bu_log("eface %d: %d %d %d :  %f %f %f->%f %f %f->%f %f %f \n", face, valid_faces[face*3], valid_faces[face*3+1], valid_faces[face*3+2],
			    valid_vertices[valid_faces[face*3]*3], valid_vertices[valid_faces[face*3]*3+1],valid_vertices[valid_faces[face*3]*3+2],
			    valid_vertices[valid_faces[face*3+1]*3], valid_vertices[valid_faces[face*3+1]*3+1],valid_vertices[valid_faces[face*3+1]*3+2],
			    valid_vertices[valid_faces[face*3+2]*3], valid_vertices[valid_faces[face*3+2]*3+1],valid_vertices[valid_faces[face*3+2]*3+2]);
		    problem_pnts.insert(valid_faces[j*3]);
		    problem_pnts.insert(valid_faces[j*3+1]);
		    problem_pnts.insert(valid_faces[j*3+2]);
		}
	    }
	}
	for (pp_it = problem_pnts.begin(); pp_it != problem_pnts.end(); pp_it++) {
	    int pind = (*pp_it);
	    ON_3dPoint *p = (*s_cdt->vert_to_on)[pind];
	    if (!p) {
		bu_log("unmapped point??? %d\n", pind);
	    } else {
		struct cdt_audit_info *paudit = (*s_cdt->pnt_audit_info)[p];
		if (!paudit) {
		    bu_log("point with no audit info??? %d\n", pind);
		} else {
		    bu_log("point %d: Face(%d) Vert(%d) Trim(%d) Edge(%d) UV(%f,%f)\n", pind, paudit->face_index, paudit->vert_index, paudit->trim_index, paudit->edge_index, paudit->surf_uv.x, paudit->surf_uv.y);
		}
	    }
	}
    }

cdt_done:
    bu_free(valid_faces, "faces");
    bu_free(valid_vertices, "vertices");

    if (invalid) {
	return 1;
    }

    return 0;

}


// Generate a BoT with normals.
int
ON_Brep_CDT_Mesh(
	int **faces, int *fcnt,
	fastf_t **vertices, int *vcnt,
	int **face_normals, int *fn_cnt,
	fastf_t **normals, int *ncnt,
	struct ON_Brep_CDT_State *s_cdt)
{
     if (!faces || !fcnt || !vertices || !vcnt || !s_cdt) {
	 return -1;
     }

     /* We can ignore the face normals if we want, but if some of the
      * return variables are non-NULL they all need to be non-NULL */
     if (face_normals || fn_cnt || normals || ncnt) {
	 if (!face_normals || !fn_cnt || !normals || !ncnt) {
	     return -1;
	 }
     }

     struct on_brep_mesh_data md;
     md.e2f = new EdgeToTri;

    for (int face_index = 0; face_index != s_cdt->brep->m_F.Count(); face_index++) {
	triangles_first_pass(s_cdt, &md, face_index);
    }

#if 0
    // Do a second pass over the remaining non-degenerate faces, looking for
    // near-zero length edges.  These indicate that there may be 3D points we
    // need to consolidate.  Ideally shouldn't get this from edge points, but
    // the surface points could conceivably introduce such points.
    int consolidated_points = 0;
    for (int face_index = 0; face_index != s_cdt->brep->m_F.Count(); face_index++) {
	p2t::CDT *cdt = s_cdt->p2t_faces[face_index];
	std::map<p2t::Point *, ON_3dPoint *> *pointmap = s_cdt->tri_to_on3_maps[face_index];
	std::vector<p2t::Triangle*> tris = cdt->GetTriangles();
	triangle_cnt += tris.size();
	for (size_t i = 0; i < tris.size(); i++) {
	    std::set<ON_3dPoint *> c_cand;
	    p2t::Triangle *t = tris[i];
	    if (tris_degen.find(t) != tris_degen.end()) {
		continue;
	    }
	    ON_3dPoint *tpnts[3] = {NULL, NULL, NULL};
	    for (size_t j = 0; j < 3; j++) {
		p2t::Point *p = t->GetPoint(j);
		ON_3dPoint *op = (*pointmap)[p];
		tpnts[j] = op;
	    }
	    fastf_t d1 = tpnts[0]->DistanceTo(*tpnts[1]);
	    fastf_t d2 = tpnts[1]->DistanceTo(*tpnts[2]);
	    fastf_t d3 = tpnts[2]->DistanceTo(*tpnts[0]);
	    if (d1 < s_cdt->dist || d2 < s_cdt->dist || d3 < s_cdt->dist) {
		bu_log("found consolidation candidate\n");
		consolidated_points = 1;
	    }
	}
    }
    // After point consolidation, re-check for trivially degenerate triangles
    if (consolidated_points) {
	return 0;
    }
#endif

    // We may still have a situation where three colinear points form a zero
    // area face.  This is more complex to deal with, as it requires modifying
    // non-degenerate faces in the neighborhood to incorporate different points.
    for (int face_index = 0; face_index != s_cdt->brep->m_F.Count(); face_index++) {
	triangles_scrub_colinear(s_cdt, &md, face_index);
    }

    // TODO - it's even possible in principle for a triangulation to form a non-zero,
    // non degenerate face from 3 edge points that will "close" the solid but result
    // in a face with all of its vertices normals pointing in a very strange direction
    // compared to the surface normals at those points (not to mention looking "wrong"
    // in that the "proper" surface of the mesh will look like it is flattened in the
    // area of that triangle.  If we can recognize these cases and splitting them will
    // improve the situation, we want to handle them just as we would a zero area
    // triangle...

    /* For the non-zero-area involved triangles, we need to build new polygons
     * from each triangle and the "orphaned" points along its edge(s).  We then
     * re-tessellate in the triangle's parametric space.
     *
     * An "involved" triangle is a triangle with two of its three points in the
     * face's degen_pnts set.
     */
    for (int face_index = 0; face_index != s_cdt->brep->m_F.Count(); face_index++) {
	std::set<p2t::Point *> *fdp = (*s_cdt->faces)[face_index]->degen_pnts;
	if (!fdp) {
	    continue;
	}
   	p2t::CDT *cdt = (*s_cdt->faces)[face_index]->cdt;
	std::map<p2t::Point *, ON_3dPoint *> *pointmap = (*s_cdt->faces)[face_index]->p2t_to_on3_map;
	std::map<p2t::Point *, ON_3dPoint *> *normalmap = (*s_cdt->faces)[face_index]->p2t_to_on3_norm_map;

	std::vector<p2t::Triangle *> *tri_add;

	tri_add = (*s_cdt->faces)[face_index]->p2t_extra_faces;

	std::vector<p2t::Triangle*> tris = cdt->GetTriangles();
	for (size_t i = 0; i < tris.size(); i++) {
	    p2t::Triangle *t = tris[i];
	    int involved_pnt_cnt = 0;
	    if (md.tris_degen.find(t) != md.tris_degen.end()) {
		continue;
	    }
	    p2t::Point *t2dpnts[3] = {NULL, NULL, NULL};
	    for (size_t j = 0; j < 3; j++) {
		p2t::Point *p = t->GetPoint(j);
		t2dpnts[j] = p;
		if (fdp->find(p) != fdp->end()) {
		    involved_pnt_cnt++;
		}
	    }
	    if (involved_pnt_cnt > 1) {

		// TODO - construct the plane of this face, project 3D points
		// into that plane to get new 2D coordinates specific to this
		// face, and then make new p2t points from those 2D coordinates.
		// Won't be using the NURBS 2D for this part, so existing
		// p2t points won't help.
		std::vector<p2t::Point *>new_2dpnts;
		std::map<p2t::Point *, p2t::Point *> new2d_to_old2d;
		std::map<p2t::Point *, p2t::Point *> old2d_to_new2d;
		std::map<ON_3dPoint *, p2t::Point *> on3d_to_new2d;
		ON_3dPoint *t3dpnts[3] = {NULL, NULL, NULL};
		for (size_t j = 0; j < 3; j++) {
		    t3dpnts[j] = (*pointmap)[t2dpnts[j]];
		}
		int normal_backwards = 0;
		ON_Plane fplane(*t3dpnts[0], *t3dpnts[1], *t3dpnts[2]);

		// To verify sanity, check fplane normal against face normals
		for (size_t j = 0; j < 3; j++) {
		    ON_3dVector pv = (*(*normalmap)[t2dpnts[j]]);
		    if (ON_DotProduct(pv, fplane.Normal()) < 0) {
			normal_backwards++;
		    }
		}
		if (normal_backwards > 0) {
		    if (normal_backwards == 3) {
			fplane.Flip();
			bu_log("flipped plane\n");
		    } else {
			bu_log("Only %d of the face normals agree with the plane normal??\n", 3 - normal_backwards);
		    }
		}

		// Project the 3D face points onto the plane
		double ucent = 0;
		double vcent = 0;
		for (size_t j = 0; j < 3; j++) {
		    double u,v;
		    fplane.ClosestPointTo(*t3dpnts[j], &u, &v);
		    ucent += u;
		    vcent += v;
		    p2t::Point *np = new p2t::Point(u, v);
		    new2d_to_old2d[np] = t2dpnts[j];
		    old2d_to_new2d[t2dpnts[j]] = np;
		    on3d_to_new2d[t3dpnts[j]] = np;
		}
		ucent = ucent/3;
		vcent = vcent/3;
		ON_2dPoint tcent(ucent, vcent);

		// Any or all of the edges of the triangle may have orphan
		// points associated with them - if both edge points are
		// involved, we have to check.
		std::vector<p2t::Point *> polyline;
		for (size_t j = 0; j < 3; j++) {
		    p2t::Point *p1 = t2dpnts[j];
		    p2t::Point *p2 = (j < 2) ? t2dpnts[j+1] : t2dpnts[0];
		    ON_3dPoint *op1 = (*pointmap)[p1];
		    polyline.push_back(old2d_to_new2d[p1]);
		    if (fdp->find(p1) != fdp->end() && fdp->find(p2) != fdp->end()) {

			// Calculate a vector to use to perturb middle points off the
			// line in 2D.  Triangulation routines don't like collinear
			// points.  Since we're not using these 2D coordinates for
			// anything except the tessellation itself, as long as we
			// don't change the ordering of the polyline we should be
			// able to nudge the points off the line without ill effect.
			ON_2dPoint p2d1(p1->x, p1->y);
			ON_2dPoint p2d2(p2->x, p2->y);
			ON_2dPoint pmid = (p2d2 + p2d1)/2;
			ON_2dVector ptb = pmid - tcent;
			fastf_t edge_len = p2d1.DistanceTo(p2d2);
			ptb.Unitize();
			ptb = ptb * edge_len;

			std::set<ON_3dPoint *> edge_3d_pnts;
			std::map<double, ON_3dPoint *> ordered_new_pnts;
			ON_3dPoint *op2 = (*pointmap)[p2];
			edge_3d_pnts.insert(op1);
			edge_3d_pnts.insert(op2);
			ON_Line eline3d(*op1, *op2);
			double t1, t2;
			eline3d.ClosestPointTo(*op1, &t1);
			eline3d.ClosestPointTo(*op2, &t2);
			std::set<p2t::Point *>::iterator fdp_it;
			for (fdp_it = fdp->begin(); fdp_it != fdp->end(); fdp_it++) {
			    p2t::Point *p = *fdp_it;
			    if (p != p1 && p != p2) {
				ON_3dPoint *p3d = (*pointmap)[p];
				if (edge_3d_pnts.find(p3d) == edge_3d_pnts.end()) {
				    if (eline3d.DistanceTo(*p3d) < s_cdt->dist) {
					double tp;
					eline3d.ClosestPointTo(*p3d, &tp);
					if (tp > t1 && tp < t2) {
					    edge_3d_pnts.insert(p3d);
					    ordered_new_pnts[tp] = p3d;
					    double u,v;
					    fplane.ClosestPointTo(*p3d, &u, &v);

					    // Make a 2D point and nudge it off of the length
					    // of the edge off the edge.  Use the line parameter
					    // value to get different nudge factors for each point.
					    ON_2dPoint uvp(u, v);
					    uvp = uvp + (t2-tp)/(t2-t1)*0.01*ptb;

					    // Define the p2t point and update maps
					    p2t::Point *np = new p2t::Point(uvp.x, uvp.y);
					    new2d_to_old2d[np] = p;
					    old2d_to_new2d[p] = np;
					    on3d_to_new2d[p3d] = np;

					}
				    }
				}
			    }
			}

			// Have all new points on edge, add to polyline in edge order
			if (t1 < t2) {
			    std::map<double, ON_3dPoint *>::iterator m_it;
			    for (m_it = ordered_new_pnts.begin(); m_it != ordered_new_pnts.end(); m_it++) {
				ON_3dPoint *p3d = (*m_it).second;
				polyline.push_back(on3d_to_new2d[p3d]);
			    }
			} else {
			    std::map<double, ON_3dPoint *>::reverse_iterator m_it;
			    for (m_it = ordered_new_pnts.rbegin(); m_it != ordered_new_pnts.rend(); m_it++) {
				ON_3dPoint *p3d = (*m_it).second;
				polyline.push_back(on3d_to_new2d[p3d]);
			    }
			}
		    }

		    // I think(?) - need to close the loop
		    if (j == 2) {
			polyline.push_back(old2d_to_new2d[p2]);
		    }
		}

		// Have polyline, do CDT
		if (polyline.size() > 5) {
		    bu_log("%d polyline cnt: %zd\n", face_index, polyline.size());
		}
		if (polyline.size() > 4) {

		    // First, try ear clipping method
		    std::map<size_t, p2t::Point *> ec_to_p2t;
		    point2d_t *ec_pnts = (point2d_t *)bu_calloc(polyline.size(), sizeof(point2d_t), "ear clipping point array");
		    for (size_t k = 0; k < polyline.size()-1; k++) {
			p2t::Point *p2tp = polyline[k];
			V2SET(ec_pnts[k], p2tp->x, p2tp->y);
			ec_to_p2t[k] = p2tp;
		    }

		    int *ecfaces;
		    int num_faces;

		    // TODO - we need to check if we're getting zero area triangles back from these routines.  Unless all three
		    // triangle edges somehow have extra points, we can construct a triangle set that meets our needs by walking
		    // the edges in order and building triangles "by hand" to meet our need.  Calling the "canned" routines
		    // is an attempt to avoid that bookkeeping work, but if the tricks in place to avoid collinear point issues
		    // don't work reliably we'll need to just go with a direct build.
		    if (bg_polygon_triangulate(&ecfaces, &num_faces, NULL, NULL, ec_pnts, polyline.size()-1, EAR_CLIPPING)) {

			// Didn't work, see if poly2tri can handle it
			//bu_log("ec triangulate failed\n");
			p2t::CDT *fcdt = new p2t::CDT(polyline);
			fcdt->Triangulate(true, -1);
			std::vector<p2t::Triangle*> ftris = fcdt->GetTriangles();
			if (ftris.size() > polyline.size()) {
#if 0
			    bu_log("weird face count: %zd\n", ftris.size());
			    for (size_t k = 0; k < polyline.size(); k++) {
				bu_log("polyline[%zd]: %0.17f, %0.17f 0\n", k, polyline[k]->x, polyline[k]->y);
			    }

			    std::string pfile = std::string("polyline.plot3");
			    plot_polyline(&polyline, pfile.c_str());
			    for (size_t k = 0; k < ftris.size(); k++) {
				std::string tfile = std::string("tri-") + std::to_string(k) + std::string(".plot3");
				plot_tri(ftris[k], tfile.c_str());
				p2t::Point *p0 = ftris[k]->GetPoint(0);
				p2t::Point *p1 = ftris[k]->GetPoint(1);
				p2t::Point *p2 = ftris[k]->GetPoint(2);
				bu_log("tri[%zd]: %f %f -> %f %f -> %f %f\n", k, p0->x, p0->y, p1->x, p1->y, p2->x, p2->y);
			    }
#endif

			    bu_log("EC and Poly2Tri failed - aborting\n");
			    return -1;
			} else {
			    bu_log("Poly2Tri: found %zd faces\n", ftris.size());
			    //std::vector<p2t::Triangle *> *tri_add = s_cdt->p2t_extra_faces[face_index];
			    // TODO - translate face 2D triangles to mesh 2D triangles
			}

		    } else {
			for (int k = 0; k < num_faces; k++) {
			    p2t::Point *p2_1 = new2d_to_old2d[ec_to_p2t[ecfaces[3*k]]];
			    p2t::Point *p2_2 = new2d_to_old2d[ec_to_p2t[ecfaces[3*k+1]]];
			    p2t::Point *p2_3 = new2d_to_old2d[ec_to_p2t[ecfaces[3*k+2]]];
			    p2t::Triangle *nt = new p2t::Triangle(*p2_1, *p2_2, *p2_3);
			    tri_add->push_back(nt);
			}
			// We split the original triangle, so it's now replaced/degenerate in the tessellation
			md.tris_degen.insert(t);
			md.triangle_cnt--;
		    }
		} else {
		    // Point count doesn't indicate any need to split, we should be good...
		    //bu_log("Not enough points in polyline to require splitting\n");
		}
	    }
	}
    }

    for (int face_index = 0; face_index != s_cdt->brep->m_F.Count(); face_index++) {
	triangles_incorrect_normals(s_cdt, &md, face_index);
    }

    bu_log("tri_cnt_init: %zd\n", md.triangle_cnt);
    for (int face_index = 0; face_index != s_cdt->brep->m_F.Count(); face_index++) {
	std::vector<p2t::Triangle *> *tri_add = (*s_cdt->faces)[face_index]->p2t_extra_faces;
	//bu_log("adding %zd faces from %d\n", tri_add->size(), face_index);
	md.triangle_cnt += tri_add->size();
    }
    bu_log("tri_cnt_init+: %zd\n", md.triangle_cnt);

#if 0
    // Validate normals using the triangle itself
    for (int face_index = 0; face_index != s_cdt->brep->m_F.Count(); face_index++) {
	p2t::CDT *cdt = s_cdt->p2t_faces[face_index];
	std::vector<p2t::Triangle*> tris = cdt->GetTriangles();
	validate_face_normals(s_cdt, &tris, &md.tris_degen, face_index);
    }
    for (int face_index = 0; face_index != s_cdt->brep->m_F.Count(); face_index++) {
	std::vector<p2t::Triangle *> *tri_add = s_cdt->p2t_extra_faces[face_index];
	if (!tri_add) {
	    continue;
	}
	validate_face_normals(s_cdt, tri_add, NULL, face_index);
    }
#endif

    // Know how many faces and points now - initialize BoT container.
    *fcnt = (int)md.triangle_cnt;
    *faces = (int *)bu_calloc(md.triangle_cnt*3, sizeof(int), "new faces array");
    *vcnt = (int)md.vfpnts.size();
    *vertices = (fastf_t *)bu_calloc(md.vfpnts.size()*3, sizeof(fastf_t), "new vert array");
    if (normals) {
	*ncnt = (int)md.vfnormals.size();
	*normals = (fastf_t *)bu_calloc(md.vfnormals.size()*3, sizeof(fastf_t), "new normals array");
	*fn_cnt = (int)md.triangle_cnt;
	*face_normals = (int *)bu_calloc(md.triangle_cnt*3, sizeof(int), "new face_normals array");
    }

    for (size_t i = 0; i < md.vfpnts.size(); i++) {
	(*vertices)[i*3] = md.vfpnts[i]->x;
	(*vertices)[i*3+1] = md.vfpnts[i]->y;
	(*vertices)[i*3+2] = md.vfpnts[i]->z;
    }

    if (normals) {
	for (size_t i = 0; i < md.vfnormals.size(); i++) {
	    (*normals)[i*3] = md.vfnormals[i]->x;
	    (*normals)[i*3+1] = md.vfnormals[i]->y;
	    (*normals)[i*3+2] = md.vfnormals[i]->z;
	}
    }

    // Iterate over faces, adding points and faces to BoT container.  Note: all
    // 3D points should be geometrically unique in this final container.
    int face_cnt = 0;
    md.triangle_cnt = 0;
    for (int face_index = 0; face_index != s_cdt->brep->m_F.Count(); face_index++) {
	p2t::CDT *cdt = (*s_cdt->faces)[face_index]->cdt;
	std::map<p2t::Point *, ON_3dPoint *> *pointmap = (*s_cdt->faces)[face_index]->p2t_to_on3_map;
	std::vector<p2t::Triangle*> tris = cdt->GetTriangles();
	md.triangle_cnt += tris.size();
	int active_tris = 0;
	for (size_t i = 0; i < tris.size(); i++) {
	    p2t::Triangle *t = tris[i];
	    if (md.tris_degen.size() > 0 && md.tris_degen.find(t) != md.tris_degen.end()) {
		md.triangle_cnt--;
		continue;
	    }
	    active_tris++;
	    for (size_t j = 0; j < 3; j++) {
		p2t::Point *p = t->GetPoint(j);
		ON_3dPoint *op = (*pointmap)[p];
		int ind = md.on_pnt_to_bot_pnt[op];
		(*faces)[face_cnt*3 + j] = ind;
		if (normals) {
		    int nind = md.on_pnt_to_bot_norm[op];
		    (*face_normals)[face_cnt*3 + j] = nind;
		}
	    }
	    if (s_cdt->brep->m_F[face_index].m_bRev) {
		int ftmp = (*faces)[face_cnt*3 + 1];
		(*faces)[face_cnt*3 + 1] = (*faces)[face_cnt*3 + 2];
		(*faces)[face_cnt*3 + 2] = ftmp;
		if (normals) {
		    int fntmp = (*face_normals)[face_cnt*3 + 1];
		    (*face_normals)[face_cnt*3 + 1] = (*face_normals)[face_cnt*3 + 2];
		    (*face_normals)[face_cnt*3 + 2] = fntmp;
		}
	    }

	    face_cnt++;
	}
	//bu_log("initial face count for %d: %d\n", face_index, active_tris);
    }
    //bu_log("tri_cnt_1: %zd\n", md.triangle_cnt);
    //bu_log("face_cnt: %d\n", face_cnt);

    for (int face_index = 0; face_index != s_cdt->brep->m_F.Count(); face_index++) {
	std::vector<p2t::Triangle *> *tri_add = (*s_cdt->faces)[face_index]->p2t_extra_faces;
	std::map<p2t::Point *, ON_3dPoint *> *pointmap = (*s_cdt->faces)[face_index]->p2t_to_on3_map;
	if (!tri_add) {
	    continue;
	}
	md.triangle_cnt += tri_add->size();
	for (size_t i = 0; i < tri_add->size(); i++) {
	    p2t::Triangle *t = (*tri_add)[i];
	    for (size_t j = 0; j < 3; j++) {
		p2t::Point *p = t->GetPoint(j);
		ON_3dPoint *op = (*pointmap)[p];
		int ind = md.on_pnt_to_bot_pnt[op];
		(*faces)[face_cnt*3 + j] = ind;
		if (normals) {
		    int nind = md.on_pnt_to_bot_norm[op];
		    (*face_normals)[face_cnt*3 + j] = nind;
		}
	    }
	    if (s_cdt->brep->m_F[face_index].m_bRev) {
		int ftmp = (*faces)[face_cnt*3 + 1];
		(*faces)[face_cnt*3 + 1] = (*faces)[face_cnt*3 + 2];
		(*faces)[face_cnt*3 + 2] = ftmp;
		if (normals) {
		    int fntmp = (*face_normals)[face_cnt*3 + 1];
		    (*face_normals)[face_cnt*3 + 1] = (*face_normals)[face_cnt*3 + 2];
		    (*face_normals)[face_cnt*3 + 2] = fntmp;
		}
	    }

	    face_cnt++;
	}
	//bu_log("added faces for %d: %zd\n", face_index, tri_add->size());
	//bu_log("tri_cnt_2: %zd\n", md.triangle_cnt);
	//bu_log("face_cnt_2: %d\n", face_cnt);


    }


    /* Clear out extra faces so they don't pollute another pass.
     * TODO - need a better way to do this, incorporated with reset */
    for (int i = 0; i < s_cdt->brep->m_F.Count(); i++) {
	std::vector<p2t::Triangle *> *ef = (*s_cdt->faces)[i]->p2t_extra_faces;
	std::vector<p2t::Triangle *>::iterator trit;
	for (trit = ef->begin(); trit != ef->end(); trit++) {
	    p2t::Triangle *t = *trit;
	    delete t;
	}
	ef->clear();
    }


    return 0;
}


/** @} */

// Local Variables:
// mode: C++
// tab-width: 8
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8

