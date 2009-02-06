/*                    N M G _ B R E P . C P P
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
/** @file nmg_brep.cpp
 *
 * b-rep support for NMG
 *
 */

#include "common.h"

#include "raytrace.h"
#include "rtgeom.h"
#include "wdb.h"
#include "bn.h"
#include "bu.h"

/**
* Simple line for edge geometry
*/
static ON_Curve* edgeCurve (const ON_BrepVertex& from, const ON_BrepVertex& to)
{
    ON_Curve *c3d = new ON_LineCurve(from.Point(), to.Point());
    c3d->SetDomain(0.0, 1.0);
    return c3d;
}

/**
* 
*/
static ON_Surface* sideSurface(const ON_3dPoint& SW, const ON_3dPoint& SE, const ON_3dPoint& NE, const ON_3dPoint& NW)
{
    ON_NurbsSurface *surf = new ON_NurbsSurface(3,FALSE, 2, 2, 2, 2);
    surf->SetCV(0,0,SW);
    surf->SetCV(1,0,SE);
    surf->SetCV(1,1,NE);
    surf->SetCV(0,1,NW);
    surf->SetKnot(0,0,0.0);
    surf->SetKnot(0,1,1.0);
    surf->SetKnot(1,0,0.0);
    surf->SetKnot(1,1,1.0);
    return surf;
}

/**
* Create Faces, Edges, and Vertices
*/
void 
makeFunnyFaces(struct model *m, ON_Brep *b, struct faceuse *fu)
{
    struct loopuse *lu;
    long brepi[m->maxindex];
    for (int i = 0; i < m->maxindex; i++) brepi[i] = -1;
    NMG_CK_FACEUSE(fu);
    for (BU_LIST_FOR(lu, loopuse, &fu->lu_hd)) {
	printf("lu #%d\n", lu->index);
	struct edgeuse *eu;
	int edges=0;
	/* loop is a single vertex */
	if (BU_LIST_FIRST_MAGIC(&lu->down_hd) != NMG_EDGEUSE_MAGIC)
	    continue;
	for (BU_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
	    ++edges;
	    /* add both vertices of this edge */
	    ON_BrepVertex from, to, tmp;
	    struct vertex_g *vg;
	    vg = eu->vu_p->v_p->vg_p;
	    NMG_CK_VERTEX_G(vg);
	    if (brepi[eu->vu_p->v_p->index] == -1) {
		printf("rt_nmg_brep: adding (%lf, %lf, %lf)\n", vg->coord[0], vg->coord[1], vg->coord[2]);
		from = b->NewVertex(vg->coord, SMALL_FASTF);
		brepi[eu->vu_p->v_p->index] = from.m_vertex_index;
	    }
	    vg = eu->eumate_p->vu_p->v_p->vg_p;
	    NMG_CK_VERTEX_G(vg);
	    if (brepi[eu->eumate_p->vu_p->v_p->index] == -1) {
		printf("rt_nmg_brep: adding (%lf, %lf, %lf)\n", vg->coord[0], vg->coord[1], vg->coord[2]);
		to = b->NewVertex(vg->coord, SMALL_FASTF);
		brepi[eu->eumate_p->vu_p->v_p->index] = to.m_vertex_index;
	    }

	    /* add this edge */
	    if(brepi[eu->e_p->index] == -1) {
		/* always add edges with the small vertex index as from */
		if (from.m_vertex_index > to.m_vertex_index) {
		    tmp = from;
		    from = to;
		    to = tmp;
		}
		ON_BrepEdge& e = b->NewEdge(from, to, eu->e_p->index);
		b->m_C3.Append(edgeCurve(from, to));
		brepi[eu->e_p->index] = e.m_edge_index;
	    }

	}//done adding edges and vertices

	
	

	/* for each loop */
	//b->m_S.Append(



    }
}

void
rt_nmg_brep(ON_Brep **b, const struct rt_db_internal *ip, const struct bn_tol *tol)
{
    struct model *m = nmg_mm();//not needed for non-tess
    struct nmgregion *r;
    struct shell *s;
    struct faceuse *fu;


    printf("rt_arb_tess\n");
    rt_arb_tess(&r, m, ip, NULL, tol);
    printf("rt_arb_tess completed\n");

    /*
    RT_CK_DB_INTERNAL(ip);
    m = (struct model *)ip->idb_ptr;
    NMG_CK_MODEL(m);
    */
    
    *b = new ON_Brep();
    for (BU_LIST_FOR(r, nmgregion, &m->r_hd)) {
	for (BU_LIST_FOR(s, shell, &r->s_hd)) {
	    for (BU_LIST_FOR(fu, faceuse, &s->fu_hd)) {
		if(fu->orientation != OT_SAME)
		    continue;
		makeFunnyFaces(m, *b, fu);
	    }
	}
    }
}



// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
