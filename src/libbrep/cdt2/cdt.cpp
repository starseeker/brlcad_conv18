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

#include "common.h"
#include "bu/malloc.h"
#include "bu/vls.h"
#include "brep/pullback.h"
#include "./cdt.h"

static int
brep_cdt_init(struct brep_cdt *s, void *bv, const char *objname)
{
    if (!s || !bv || !objname) return -1;
    s->i = new brep_cdt_impl;
    if (!s->i) return -1;
    s->i->s.orig_brep = (ON_Brep *)bv;
    s->i->s.name = std::string(objname);
    s->msgs = (struct bu_vls *)bu_calloc(1, sizeof(struct bu_vls), "message buffer");
    bu_vls_init(s->msgs);
    return 0;
}

extern "C" int
brep_cdt_create(struct brep_cdt **cdt, void *bv, const char *objname)
{
    if (!cdt || !bv) return -1;
    (*cdt) = new brep_cdt;
    return brep_cdt_init(*cdt, bv, objname);
}

static void
brep_cdt_clear(struct brep_cdt *s)
{
    delete s->i;
    bu_vls_free(s->msgs);
    bu_free(s->msgs, "message buffer free");
}

extern "C" void
brep_cdt_destroy(struct brep_cdt *s)
{
    if (!s) return;
    brep_cdt_clear(s);
    delete s;
}


// TODO - need a flag variable to enable various modes (watertightness,
// validity, overlap resolving - no need for separate functions for all that...)
int
brep_cdt_triangulate(struct brep_cdt *s_cdt, int UNUSED(face_cnt), int *UNUSED(faces))
{
    if (!s_cdt) return -1;

    // Check for any conditions that are show-stoppers
    ON_wString wonstr;
    ON_TextLog vout(wonstr);
    if (!s_cdt->i->s.orig_brep->IsValid(&vout)) {
	bu_vls_printf(s_cdt->msgs, "NOTE: brep is NOT valid, cannot produce watertight mesh\n");
    }

    // For now, edges must have 2 and only 2 trims for this to work.
    // TODO - only do this check for solid objects - plate mode objects should be processed.
    // Need to think about how to structure that - in such a case all of this logic should
    // collapse back to the original logic...
    // for that matter, if we want the fastest possible mesh regardless of quality we should
    // be able to do that...
    for (int index = 0; index < s_cdt->i->s.orig_brep->m_E.Count(); index++) {
	ON_BrepEdge& edge = s_cdt->i->s.orig_brep->m_E[index];
	if (edge.TrimCount() != 2) {
	    bu_vls_printf(s_cdt->msgs, "Edge %d trim count: %d - can't (yet) do watertight meshing\n", edge.m_edge_index, edge.TrimCount());
	    return -1;
	}
    }

    // We may be changing the ON_Brep data, so work on a copy
    // rather than the original object
    s_cdt->i->s.brep = new ON_Brep(*s_cdt->i->s.orig_brep);

    // Attempt to minimize situations where 2D and 3D distances get out of sync
    // by shrinking the surfaces down to the active area of the face
    s_cdt->i->s.brep->ShrinkSurfaces();

    ON_Brep* brep = s_cdt->i->s.brep;

    // Reparameterize the face's surface and transform the "u" and "v"
    // coordinates of all the face's parameter space trimming curves to
    // minimize distortion in the map from parameter space to 3d.
    for (int face_index = 0; face_index < brep->m_F.Count(); face_index++) {
	ON_BrepFace *face = brep->Face(face_index);
	const ON_Surface *s = face->SurfaceOf();
	double surface_width, surface_height;
	if (s->GetSurfaceSize(&surface_width, &surface_height)) {
	    face->SetDomain(0, 0.0, surface_width);
	    face->SetDomain(1, 0.0, surface_height);
	}
    }

    // Populate b_pnts with all the vertex points.
    for (int vert_index = 0; vert_index < brep->m_V.Count(); vert_index++) {
	mesh_point_t np;
	np.vert_index = vert_index;
	np.p = brep->m_V[vert_index].Point();
	np.n = ON_3dVector::UnsetVector;
	np.type = B_VERT;
	for (int j = 0; j < brep->m_V[vert_index].m_ei.Count(); j++) {
	    ON_BrepEdge &edge = brep->m_E[brep->m_V[vert_index].m_ei[j]];
	    const ON_Curve* crv = edge.EdgeCurveOf();
	    if (crv && !crv->IsLinear(BN_TOL_DIST)) {
		np.on_curved_edge = true;
		break;
	    }
	}
	s_cdt->i->s.b_pnts.push_back(np);
    }

    // Flag singular vertices.
    for (int index = 0; index < brep->m_T.Count(); index++) {
	ON_BrepTrim &trim = brep->m_T[index];
	if (trim.m_type == ON_BrepTrim::singular) {
	    s_cdt->i->s.b_pnts[trim.Vertex(0)->m_vertex_index].singular = true;
	    s_cdt->i->s.b_pnts[trim.Vertex(1)->m_vertex_index].singular = true;
	}
    }

    // Calculate normals for singular vertices - the surface normals at those
    // aren't well defined, so other nearby information must be used.
    for (size_t index = 0; index < s_cdt->i->s.b_pnts.size(); index++) {
	mesh_point_t &p = s_cdt->i->s.b_pnts[index];
	if (p.singular) {
	    p.n = singular_vert_norm(brep, p.vert_index);
	}
    }

    // Build the initial set of unordered edges.  Unordered edges on BRep face
    // edges are independent of faces - ordered edges are specific to
    // individual faces.  As yet, we're not concerned with specific faces, so
    // only create the unordered edges.
    //
    // NOTE: At this stage need the index of each b_edge to match the
    // ON_BrepEdge index value, so we can populate the loop structures without
    // having to build a std::map.  If for any reason that should not prove
    // viable, need to build and use the map.
    for (int index = 0; index < brep->m_E.Count(); index++) {
	ON_BrepEdge& edge = brep->m_E[index];
	long v0 = edge.Vertex(0)->m_vertex_index;
	long v1 = edge.Vertex(1)->m_vertex_index;
	mesh_uedge_t ue;
	ue.v[0] = (v1 > v0) ? v0 : v1;
	ue.v[1] = (v1 > v0) ? v1 : v0;
	ue.edge_ind = index;
	ue.edge_start_pnt = v0;
	ue.edge_end_pnt = v1;

	// Characterize the edge type
	ue.split_type = find_edge_type(s_cdt, edge);
	ue.type = (ue.split_type == 0) ? B_SINGULAR : B_BOUNDARY;

	// Set up the NURBS curve to be used for splitting
	const ON_Curve* crv = edge.EdgeCurveOf();
	ue.nc = crv->NurbsCurve();
	// Match the parameterization to the control polygon length, so
	// distances in t value will have some relationship to 3D distances
	ue.cp_len = ue.nc->ControlPolygonLength();
	ue.nc->SetDomain(0.0, ue.cp_len);
	ue.t_start = 0.0;
	ue.t_end = ue.cp_len;

    }

    // Set trim curve domains to match the edge curve domains.
    for (int index = 0; index < brep->m_T.Count(); index++) {
    }

    // Vertices and edges are the only two elements that are not unique to
    // faces.  However, to build up unordered edges we also want the polygon
    // edges in place for the faces.  Build initial polygons now.
    // At this point, with no splitting done, the ON_Brep indicies and brep_cdt
    // indices should still agree.

    // Build up the initial m_edges set and tree, associating ordered edges
    // and polygon edges with the uedges.  polygon edges and ordered edges have
    // an unambiguous association, so the only difficult mapping is between the
    // ordered edge and the unordered edge - we need to translate the two
    // vertex indices into an unordered edge array index (if the associated
    // unordered edge has already been created by the other edge in the
    // pairing.)  Use an RTree lookup on the uedge tree for this, since by
    // definition the two ordered edges will have the same bounding box in
    // space.

    /* Check for singular trims.  Vertices associated with such a trim are
     * categorized as singular vertices.  Get vertex normals that are the
     * average of the surface normals at the junction from faces that don't use
     * a singular trim to reference the vertex. */

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

