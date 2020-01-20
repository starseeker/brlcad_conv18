/*                   C D T _ O V L P S . C P P
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
/** @file cdt_ovlps.cpp
 *
 * Constrained Delaunay Triangulation of NURBS B-Rep objects.
 *
 * This file is logic specifically for refining meshes to clear
 * overlaps between sets of objects.
 *
 */

#include "common.h"
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <string>
#include "bg/chull.h"
#include "bg/tri_pt.h"
#include "bg/tri_tri.h"
#include "./cdt.h"


// Functions to build sets of active meshes from the active pairs set

std::set<omesh_t *>
active_omeshes(std::set<std::pair<cdt_mesh_t *, cdt_mesh_t *>> &check_pairs)
{
    std::set<omesh_t *> a_omesh;
    std::set<omesh_t *>::iterator a_it;
    std::set<std::pair<cdt_mesh_t *, cdt_mesh_t *>>::iterator cp_it;
    for (cp_it = check_pairs.begin(); cp_it != check_pairs.end(); cp_it++) {
	omesh_t *omesh1 = cp_it->first->omesh;
	omesh_t *omesh2 = cp_it->second->omesh;
	a_omesh.insert(omesh1);
	a_omesh.insert(omesh2);
    }

    return a_omesh;
}

/******************************************************************************
 * As a first step, use the face bboxes to narrow down where we have potential
 * interactions between breps
 ******************************************************************************/

struct nf_info {
    std::set<std::pair<cdt_mesh_t *, cdt_mesh_t *>> *check_pairs;
    cdt_mesh_t *cmesh;
};

static bool NearFacesPairsCallback(void *data, void *a_context) {
    struct nf_info *nf = (struct nf_info *)a_context;
    cdt_mesh_t *omesh = (cdt_mesh_t *)data;
    std::pair<cdt_mesh_t *, cdt_mesh_t *> p1(nf->cmesh, omesh);
    std::pair<cdt_mesh_t *, cdt_mesh_t *> p2(omesh, nf->cmesh);
    if ((nf->check_pairs->find(p1) == nf->check_pairs->end()) && (nf->check_pairs->find(p1) == nf->check_pairs->end())) {
	nf->check_pairs->insert(p1);
    }
    return true;
}

std::set<std::pair<cdt_mesh_t *, cdt_mesh_t *>>
possibly_interfering_face_pairs(struct ON_Brep_CDT_State **s_a, int s_cnt)
{
    std::set<std::pair<cdt_mesh_t *, cdt_mesh_t *>> check_pairs;
    RTree<void *, double, 3> rtree_fmeshes;
    for (int i = 0; i < s_cnt; i++) {
	struct ON_Brep_CDT_State *s_i = s_a[i];
	for (int i_fi = 0; i_fi < s_i->brep->m_F.Count(); i_fi++) {
	    const ON_BrepFace *i_face = s_i->brep->Face(i_fi);
	    ON_BoundingBox bb = i_face->BoundingBox();
	    cdt_mesh_t *fmesh = &s_i->fmeshes[i_fi];
	    struct nf_info nf;
	    nf.cmesh = fmesh;
	    nf.check_pairs = &check_pairs;
	    double fMin[3];
	    fMin[0] = bb.Min().x;
	    fMin[1] = bb.Min().y;
	    fMin[2] = bb.Min().z;
	    double fMax[3];
	    fMax[0] = bb.Max().x;
	    fMax[1] = bb.Max().y;
	    fMax[2] = bb.Max().z;
	    // Check the new box against the existing tree, and add any new
	    // interaction pairs to check_pairs
	    rtree_fmeshes.Search(fMin, fMax, NearFacesPairsCallback, (void *)&nf);
	    // Add the new box to the tree so any additional boxes can check
	    // against it as well
	    rtree_fmeshes.Insert(fMin, fMax, (void *)fmesh);
	}
    }

    return check_pairs;
}

void
tri_uedges(std::set<uedge_t> &uedges, std::set<uedge_t> &uskip, triangle_t &t, double lthresh)
{
    for (int i = 0; i < 3; i++) {
	if (t.uedge_len(i) > lthresh) {
	    uedge_t ue = t.uedge(i);
	    split_uedges_init.insert(ue);
	}
    }
    // Remove the shortest edge from splitting, if present.  if we do need to
    // split this edge, do so after getting longer edges.
    uedge_t ue = t.shortest_edge();
    uskip.insert(ue);
}

std::set<bedge_seg_t *>
omesh_ovlps(
	std::map<omesh_t *, std::set<uedge_t>> *iedges,
	std::set<std::pair<cdt_mesh_t *, cdt_mesh_t *>> check_pairs, int mode, double lthresh)
{
    std::set<bedge_seg_t *> bsegs;
    std::map<cdt_mesh_t *, std::set<uedge_t>> iedges_init;
    std::map<cdt_mesh_t *, std::set<uedge_t>> iedges_skip;
    std::map<cdt_mesh_t *, std::set<long>> itris;

    iedges->clear();

    size_t tri_isects = 0;

    // Assemble the set of mesh faces that (per the check_pairs sets) we
    // may need to process.
    std::set<omesh_t *> a_omesh = active_omeshes(check_pairs);
    std::set<omesh_t *>::iterator a_it;

    // Scrub the old data in active mesh containers (if any)
    for (a_it = a_omesh.begin(); a_it != a_omesh.end(); a_it++) {
	omesh_t *am = *a_it;
	am->itris.clear();
	am->intruding_tris.clear();
    }
    a_omesh.clear();

    // Intersect first the triangle RTrees, then any potentially
    // overlapping triangles found within the leaves.
    std::set<std::pair<cdt_mesh_t *, cdt_mesh_t *>>::iterator cp_it;
    for (cp_it = check_pairs.begin(); cp_it != check_pairs.end(); cp_it++) {
	cdt_mesh_t *fmesh1 = cp_it->first;
	cdt_mesh_t *fmesh2 = cp_it->second;
	struct ON_Brep_CDT_State *s_cdt1 = (struct ON_Brep_CDT_State *)fmesh1->p_cdt;
	struct ON_Brep_CDT_State *s_cdt2 = (struct ON_Brep_CDT_State *)fmesh2->p_cdt;
	if (s_cdt1 != s_cdt2) {
	    std::set<std::pair<size_t, size_t>> tris_prelim;
	    size_t ovlp_cnt = fmesh1->tris_tree.Overlaps(fmesh2->tris_tree, &tris_prelim);
	    if (ovlp_cnt) {
		std::set<std::pair<size_t, size_t>>::iterator tb_it;
		for (tb_it = tris_prelim.begin(); tb_it != tris_prelim.end(); tb_it++) {
		    triangle_t t1 = fmesh1->tris_vect[tb_it->first];
		    triangle_t t2 = fmesh2->tris_vect[tb_it->second];
		    int isect = tri_isect(t1, t2, mode);
		    if (isect) {
			itris[t1.m].insert(t1.ind);
			itris[t2.m].insert(t2.ind);
			tri_isects++;
		    }
		}
	    }
	}
    }

    // Build up the initial edge sets
    std::map<cdt_mesh_t *, std::set<long>>::iterator i_it;
    for (i_it = itris.begin(); i_it != itris.end(); i_it++) {
	std::set<long>::iterator l_it;
	for (l_it = i_it->second.begin(); l_it != i_it->second.end(); l_it++) {
	    triangle_t t = i_it->first->tris_vect[*l_it];
	    triangle_uedges(iedges_init[t.m], iedges_skip[t.m], t, lthresh);
	}
    }

    // Build up the final edge sets
    std::map<omesh_t *, std::set<uedge_t>>::iterator ie_it;
    for (ie_it = iedges_init.begin(); ie_it != iedges_init.end(); ie_it++) {
	cdt_mesh_t *fmesh = ie_it->first;
	std::set<uedge_t>::iterator u_it;
	for (u_it = ie_it->second.begin(); u_it != ie_it->second.end(); u_it++) {
	    if (iedges_skip[fmesh].find(*u_it) == iedges_skip[fmesh].end()) {
		if (fmesh->brep_edges.find(*u_it) == mesh->brep_edges.end()) {
		    (*iedges)[fmesh].insert(*u_it);
		} else {
		    bedge_seg_t *bseg = om->fmesh->ue2b_map[*u_it];
		    bsegs.insert(bseg);
		}
	    }
	}
    }

    return bsegs;
}

int
ovlp_split_interior_edge(overt_t **nv, std::set<long> &ntris, omesh_t *omesh, uedge_t &ue)
{
    std::set<overt_t *> overts_to_update;

    // Find the two triangles that we will be using to form the outer polygon
    std::set<size_t> rtris = omesh->fmesh->uedges2tris[ue];
    if (rtris.size() != 2) {
	std::cout << "Error - could not associate uedge with two triangles??\n";
	return -1;
    }
    std::set<size_t> crtris = rtris;
    triangle_t tri1 = omesh->fmesh->tris_vect[*crtris.begin()];
    crtris.erase(crtris.begin());
    triangle_t tri2 = omesh->fmesh->tris_vect[*crtris.begin()];
    crtris.erase(crtris.begin());

    cpolygon_t *polygon = omesh->fmesh->uedge_polygon(ue);

    // Add interior point.
    ON_3dPoint epnt = (*omesh->fmesh->pnts[ue.v[0]] + *omesh->fmesh->pnts[ue.v[1]]) * 0.5;
    double spdist = omesh->fmesh->pnts[ue.v[0]]->DistanceTo(*omesh->fmesh->pnts[ue.v[1]]);
    ON_3dPoint spnt;
    ON_3dVector sn;
    if (!omesh->fmesh->closest_surf_pnt(spnt, sn, &epnt, spdist*10)) {
	std::cerr << "Error - couldn't find closest point\n";
	delete polygon;
	return -1;
    }
    // Base the bbox on the edge length, since we don't have any topology yet.
    ON_BoundingBox sbb(spnt,spnt);
    ON_3dPoint sbb1 = spnt - ON_3dPoint(.1*spdist, .1*spdist, .1*spdist);
    ON_3dPoint sbb2 = spnt + ON_3dPoint(.1*spdist, .1*spdist, .1*spdist);
    sbb.Set(sbb1, true);
    sbb.Set(sbb2, true);

    // We're going to use it - add new 3D point
    long f3ind = omesh->fmesh->add_point(new ON_3dPoint(spnt));
    long fnind = omesh->fmesh->add_normal(new ON_3dPoint(sn));
    struct ON_Brep_CDT_State *s_cdt = (struct ON_Brep_CDT_State *)omesh->fmesh->p_cdt;
    CDT_Add3DPnt(s_cdt, omesh->fmesh->pnts[f3ind], omesh->fmesh->f_id, -1, -1, -1, 0, 0);
    CDT_Add3DNorm(s_cdt, omesh->fmesh->normals[fnind], omesh->fmesh->pnts[f3ind], omesh->fmesh->f_id, -1, -1, -1, 0, 0);
    omesh->fmesh->nmap[f3ind] = fnind;
    overt_t *nvrt = omesh->vert_add(f3ind, &sbb);
    overts_to_update.insert(nvrt);

    // The 'old' triangles may themselves have been newly introduced - we're now going to remove
    // them from the mesh regardless, so make sure the ntris set doesn't include them any more.
    ntris.erase(tri1.ind);
    ntris.erase(tri2.ind);

    // Split the old triangles, perform the removals and add the new triangles
    int new_tris = 0;
    std::set<triangle_t>::iterator s_it;
    std::set<triangle_t> t1_tris = tri1.split(ue, f3ind, false);
    omesh->fmesh->tri_remove(tri1);
    for (s_it = t1_tris.begin(); s_it != t1_tris.end(); s_it++) {
	triangle_t vt = *s_it;
	omesh->fmesh->tri_add(vt);
	ntris.insert(omesh->fmesh->tris_vect.size() - 1);
	// In addition to the genuinely new vertices, altered triangles
	// may need updated vert bboxes
	for (int i = 0; i < 3; i++) {
	    overts_to_update.insert(omesh->overts[vt.v[i]]);
	}
	new_tris++;
    }
    std::set<triangle_t> t2_tris = tri2.split(ue, f3ind, false);
    omesh->fmesh->tri_remove(tri2);
    for (s_it = t2_tris.begin(); s_it != t2_tris.end(); s_it++) {
	triangle_t vt = *s_it;
	omesh->fmesh->tri_add(vt);
	ntris.insert(omesh->fmesh->tris_vect.size() - 1);
	// In addition to the genuinely new vertices, altered triangles
	// may need updated vert bboxes
	for (int i = 0; i < 3; i++) {
	    overts_to_update.insert(omesh->overts[vt.v[i]]);
	}
	new_tris++;
    }

    // Have new triangles, update overts
    std::set<overt_t *>::iterator n_it;
    for (n_it = overts_to_update.begin(); n_it != overts_to_update.end(); n_it++) {
	(*n_it)->update();
    }

    (*nv) = nvrt;

    //omesh->fmesh->valid(1);

    return new_tris;
}


static void
split_tri(cdt_mesh_t &fmesh, size_t t_id, long np_id, uedge_t &split_edge)
{
    triangle_t &t = fmesh.tris_vect[t_id];
    std::set<triangle_t> ntris = t.split(split_edge, np_id, false);

    fmesh.tri_remove(t);
    std::set<triangle_t>::iterator t_it;
    for (t_it = ntris.begin(); t_it != ntris.end(); t_it++) {
	triangle_t nt = *t_it;
	fmesh.tri_add(nt);
    }
}

int
internal_split_edge(
	cdt_mesh_t *fmesh,
	uedge_t &ue
	)
{
    // Find the two triangles that we will be using to form the outer polygon
    std::set<size_t> rtris = fmesh->uedges2tris[ue];
    if (rtris.size() != 2) {
        std::cout << "Error - could not associate uedge with two triangles??\n";
        return -1;
    }

    std::set<size_t> crtris = rtris;
    triangle_t tri1 = fmesh->tris_vect[*crtris.begin()];
    crtris.erase(crtris.begin());
    triangle_t tri2 = fmesh->tris_vect[*crtris.begin()];
    crtris.erase(crtris.begin());

    // Add interior point.
    ON_3dPoint epnt = (*fmesh->pnts[ue.v[0]] + *fmesh->pnts[ue.v[1]]) * 0.5;
    double spdist = fmesh->pnts[ue.v[0]]->DistanceTo(*fmesh->pnts[ue.v[1]]);
    ON_3dPoint spnt;
    ON_3dVector sn;
    if (!fmesh->closest_surf_pnt(spnt, sn, &epnt, spdist*10)) {
        std::cerr << "Error - couldn't find closest point\n";
        return -1;
    }

    // We're going to use it - add new 3D point
    long f3ind = fmesh->add_point(new ON_3dPoint(spnt));
    long fnind = fmesh->add_normal(new ON_3dPoint(sn));
    struct ON_Brep_CDT_State *s_cdt = (struct ON_Brep_CDT_State *)fmesh->p_cdt;
    CDT_Add3DPnt(s_cdt, fmesh->pnts[f3ind], fmesh->f_id, -1, -1, -1, 0, 0);
    CDT_Add3DNorm(s_cdt, fmesh->normals[fnind], fmesh->pnts[f3ind], fmesh->f_id, -1, -1, -1, 0, 0);
    fmesh->nmap[f3ind] = fnind;

    // Split the old triangles, perform the removals and add the new triangles
    split_tri(*fmesh, tri1.ind, f3ind, ue);
    split_tri(*fmesh, tri2.ind, f3ind, ue);
   
    return 0;
}

int
brep_split_edge(
	bedge_seg_t *eseg, double t
	)
{
    int replaced_tris = 0;

    std::vector<std::pair<cdt_mesh_t *,uedge_t>> uedges = eseg->uedges();

    cdt_mesh_t *fmesh_f1 = uedges[0].first;
    cdt_mesh_t *fmesh_f2 = uedges[1].first;

    int f_id1 = fmesh_f1->f_id;
    int f_id2 = fmesh_f2->f_id;

    uedge_t ue1 = uedges[0].second;
    uedge_t ue2 = uedges[1].second;

    std::set<size_t> f1_tris = fmesh_f1->uedges2tris[ue1];
    std::set<size_t> f2_tris = fmesh_f2->uedges2tris[ue2];

    size_t tris_cnt = 0;
    if (fmesh_f1->f_id == fmesh_f2->f_id) {
	std::set<size_t> f_alltris;
	f_alltris.insert(f1_tris.begin(), f1_tris.end());
	f_alltris.insert(f2_tris.begin(), f2_tris.end());
	tris_cnt = f_alltris.size();
    } else {
	tris_cnt = f1_tris.size() + f2_tris.size();
    }

    // We're replacing the edge and its triangles - clear old data
    // from the containers
    fmesh_f1->brep_edges.erase(ue1);
    fmesh_f2->brep_edges.erase(ue2);
    fmesh_f1->ue2b_map.erase(ue1);
    fmesh_f2->ue2b_map.erase(ue2);

    int eind = eseg->edge_ind;
    struct ON_Brep_CDT_State *s_cdt_edge = (struct ON_Brep_CDT_State *)eseg->p_cdt;
    std::set<bedge_seg_t *> epsegs = s_cdt_edge->e2polysegs[eind];
    epsegs.erase(eseg);
    std::set<bedge_seg_t *> esegs_split = split_edge_seg(s_cdt_edge, eseg, 1, &t, 1);
    if (!esegs_split.size()) {
	std::cerr << "SPLIT FAILED!\n";
	return -1;
    }

    epsegs.insert(esegs_split.begin(), esegs_split.end());
    s_cdt_edge->e2polysegs[eind].clear();
    s_cdt_edge->e2polysegs[eind] = epsegs;
    std::set<bedge_seg_t *>::iterator es_it;
    for (es_it = esegs_split.begin(); es_it != esegs_split.end(); es_it++) {
	bedge_seg_t *es = *es_it;

	std::vector<std::pair<cdt_mesh_t *,uedge_t>> nuedges = es->uedges();
	cdt_mesh_t *f1 = nuedges[0].first;
	cdt_mesh_t *f2 = nuedges[1].first;
	uedge_t ue_1 = nuedges[0].second;
	uedge_t ue_2 = nuedges[1].second;

	f1->brep_edges.insert(ue_1);
	f2->brep_edges.insert(ue_2);
	f1->ue2b_map[ue_1] = es;
	f2->ue2b_map[ue_2] = es;
    }

    long np_id;
    if (f_id1 == f_id2) {
	std::set<size_t> ftris;
	std::set<size_t>::iterator tr_it;
	uedge_t ue;
	ue = (f1_tris.size()) ? ue1 : ue2;
	ftris.insert(f1_tris.begin(), f1_tris.end());
	ftris.insert(f2_tris.begin(), f2_tris.end());
	np_id = fmesh_f1->pnts.size() - 1;
	fmesh_f1->ep.insert(np_id);
	for (tr_it = ftris.begin(); tr_it != ftris.end(); tr_it++) {
	    replace_edge_split_tri(*fmesh_f1, *tr_it, np_id, ue);
	    replaced_tris++;
	}
    } else {
	np_id = fmesh_f1->pnts.size() - 1;
	fmesh_f1->ep.insert(np_id);
	replace_edge_split_tri(*fmesh_f1, *f1_tris.begin(), np_id, ue1);
	replaced_tris++;

	np_id = fmesh_f2->pnts.size() - 1;
	fmesh_f2->ep.insert(np_id);
	replace_edge_split_tri(*fmesh_f2, *f2_tris.begin(), np_id, ue2);
	replaced_tris++;
    }

    return replaced_tris;
}

// TODO - need to add a check for triangles with all three vertices on the
// same brep face edge - c.s face 4 appears to have some triangles appearing
// of that sort, which are messing with the ovlp resolution...
bool
check_faces_validity(std::set<std::pair<cdt_mesh_t *, cdt_mesh_t *>> &check_pairs)
{
    int verbosity = 0;
    std::set<cdt_mesh_t *> fmeshes;
    std::set<struct ON_Brep_CDT_State *> cdt_states;
    std::set<std::pair<cdt_mesh_t *, cdt_mesh_t *>>::iterator cp_it;
    for (cp_it = check_pairs.begin(); cp_it != check_pairs.end(); cp_it++) {
	cdt_mesh_t *fmesh1 = cp_it->first;
	cdt_mesh_t *fmesh2 = cp_it->second;
	fmeshes.insert(fmesh1);
	fmeshes.insert(fmesh2);
	struct ON_Brep_CDT_State *s_cdt1 = (struct ON_Brep_CDT_State *)fmesh1->p_cdt;
	struct ON_Brep_CDT_State *s_cdt2 = (struct ON_Brep_CDT_State *)fmesh2->p_cdt;
	cdt_states.insert(s_cdt1);
	cdt_states.insert(s_cdt2);
    }
    if (verbosity > 0) {
	std::cout << "Full face validity check results:\n";
    }
    bool valid = true;

    std::set<struct ON_Brep_CDT_State *>::iterator s_it;
    for (s_it = cdt_states.begin(); s_it != cdt_states.end(); s_it++) {
	struct ON_Brep_CDT_State *s_cdt = *s_it;
	if (!CDT_Audit(s_cdt)) {
	    std::cerr << "Invalid: " << s_cdt->name << " edge data\n";
	    valid = false;
	    bu_exit(1, "urk\n");
	}
    }

    std::set<cdt_mesh_t *>::iterator f_it;
    for (f_it = fmeshes.begin(); f_it != fmeshes.end(); f_it++) {
	cdt_mesh_t *fmesh = *f_it;
	if (!fmesh->valid(verbosity)) {
	    valid = false;
#if 1
	    struct ON_Brep_CDT_State *s_cdt = (struct ON_Brep_CDT_State *)fmesh->p_cdt;
	    std::string fpname = std::string(s_cdt->name) + std::string("_face_") + std::to_string(fmesh->f_id) + std::string(".plot3");
	    fmesh->tris_plot(fpname.c_str());
	    std::cerr << "Invalid: " << s_cdt->name << " face " << fmesh->f_id << "\n";
#endif
	}
    }
    if (!valid) {
#if 1
	for (f_it = fmeshes.begin(); f_it != fmeshes.end(); f_it++) {
	    cdt_mesh_t *fmesh = *f_it;
	    fmesh->valid(2);
	}
#endif
	std::cout << "fatal mesh damage\n";
	//bu_exit(1, "fatal mesh damage");
    }

    return valid;
}

int
ON_Brep_CDT_Ovlp_Resolve(struct ON_Brep_CDT_State **s_a, int s_cnt, double lthresh)
{
    if (!s_a) return -1;
    if (s_cnt < 1) return 0;

    // Get the bounding boxes of all faces of all breps in s_a, and find
    // possible interactions
    std::set<std::pair<cdt_mesh_t *, cdt_mesh_t *>> check_pairs;
    check_pairs = possibly_interfering_face_pairs(s_a, s_cnt);

    //std::cout << "Found " << check_pairs.size() << " potentially interfering face pairs\n";
    if (!check_pairs.size()) return 0;

    // Sanity check - are we valid?
    if (!check_faces_validity(check_pairs)) {
	bu_log("ON_Brep_CDT_Ovlp_Resolve:  invalid inputs - not attempting overlap processing!\n");
	return -1;
    }

    std::map<cdt_mesh_t *, std::set<uedge_t>> otsets;
    std::set<bedge_seg_t *> bsegs; 

    bsegs = omesh_ovlps(&otsets, check_pairs, 1);
    while (otsets.size() || bsegs.size()) {
	// bsegs first - they impact more than one face
	std::set<bedge_seg_t *>::iterator b_it; 
	for (b_it = bsegs.begin(); b_it != bsegs.end(); b_it++) {
	    bedge_seg_t *bseg = *b_it;
	    double t = bseg->edge_start + ((bseg->edge_end - bseg->edge_start) * 0.5);
	    brep_split_edge(*b_it, t);
	}

	std::map<cdt_mesh_t *, std::set<uedge_t>>::iterator ot_it;
	for (ot_it = otsets.begin(); ot_it != otsets.end(); ot_it++) {
	    std::set<uedge_t>::iterator u_it;
	    for (u_it = ot_it->second.begin(); u_it != ot_it->second.end(); u_it++) {
		internal_split_edge(ot_it->first, *u_it);
	    }
	}

	bsegs = omesh_ovlps(&otsets, check_pairs, 1);
    }

    // Make sure everything is still OK and do final overlap check
    bool final_valid = check_faces_validity(check_pairs);
    bu_log("Active set sizes: %d\n", bsegs.size() + otsets.size());

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

