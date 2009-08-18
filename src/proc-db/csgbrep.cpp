/*                           C S G B R E P . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2009 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file csgbrep.c
 * 
 *  
 * 
 */

#include "common.h"
#include "bu.h"
#include "opennurbs.h"

#define OBJ_BREP
/* without OBJ_BREP, this entire procedural example is disabled */
#ifdef OBJ_BREP

#include "bio.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "vmath.h"		/* BRL-CAD Vector macros */
#include "wdb.h"
extern void rt_sph_brep(ON_Brep **bi, struct rt_db_internal *ip, const struct bn_tol *tol);
extern void rt_ell_brep(ON_Brep **bi, struct rt_db_internal *ip, const struct bn_tol *tol);
extern void rt_tor_brep(ON_Brep **bi, struct rt_db_internal *ip, const struct bn_tol *tol);

#ifdef __cplusplus
}
#endif

int
main(int argc, char** argv)
{
    struct rt_wdb* outfp;
    struct rt_db_internal *tmp_internal = (struct rt_db_internal *) bu_malloc(sizeof( struct rt_db_internal), "allocate structure");
    RT_INIT_DB_INTERNAL(tmp_internal);
    struct bn_tol tmptol;
    tmptol.dist = 0.005;
    const struct bn_tol *tol = &tmptol;
    point_t center;
    vect_t a, b, c;
    ON_TextLog error_log;

    ON::Begin();

    outfp = wdb_fopen("csgbrep.g");
    const char* id_name = "CSG B-Rep Examples";
    mk_id(outfp, id_name);

    bu_log("Writing a Spherical  b-rep...\n");
    ON_Brep* sphbrep = ON_Brep::New();
    struct  rt_ell_internal *sph;
    BU_GETSTRUCT( sph, rt_ell_internal);
    sph->magic = RT_ELL_INTERNAL_MAGIC;
    VSET(center, 0, 0, 0);
    VSET(a, 5, 0, 0);
    VSET(b, 0, 5, 0);
    VSET(c, 0, 0, 5); 
    VMOVE( sph->v, center );
    VMOVE( sph->a, a );
    VMOVE( sph->b, b );
    VMOVE( sph->c, c );
    tmp_internal->idb_ptr = (genptr_t)sph;
    rt_sph_brep(&sphbrep, tmp_internal, tol);
    const char* sph_name = "sph_nurb.s";
    mk_brep(outfp, sph_name, sphbrep);
    delete sphbrep;

    bu_log("Writing an Ellipsoidal b-rep...\n");
    ON_Brep* ellbrep = ON_Brep::New();
    struct  rt_ell_internal *ell;
    BU_GETSTRUCT(ell, rt_ell_internal);
    ell->magic = RT_ELL_INTERNAL_MAGIC;
    VSET(center, 0, 0, 0);
    VSET(a, 5, 0, 0);
    VSET(b, 0, 3, 0);
    VSET(c, 0, 0, 1); 
    VMOVE( ell->v, center );
    VMOVE( ell->a, a );
    VMOVE( ell->b, b );
    VMOVE( ell->c, c );
    tmp_internal->idb_ptr = (genptr_t)ell;
    rt_ell_brep(&ellbrep, tmp_internal, tol);
    const char* ell_name = "ell_nurb.s";
    mk_brep(outfp, ell_name, ellbrep);
    delete ellbrep;
    
    bu_log("Writing a Torus b-rep...\n");
    ON_Brep* torbrep = ON_Brep::New();
    struct rt_tor_internal *tor;
    BU_GETSTRUCT( tor, rt_tor_internal );
    tor->magic = RT_TOR_INTERNAL_MAGIC;
    VSET(center, 0, 0, 0);
    VSET(a, 0, 0, 1);
    VMOVE( tor->v, center );
    VMOVE( tor->h, a );
    tor->r_a = 5.0;
    tor->r_h = 2.0;
    tmp_internal->idb_ptr = (genptr_t)tor;
    rt_tor_brep(&torbrep, tmp_internal, tol);
    const char* tor_name = "tor_nurb.s";
    mk_brep(outfp, tor_name, torbrep);
    delete torbrep;

    bu_free(tmp_internal, "free tmp_internal");
    wdb_close(outfp);

    ON::End();

    return 0;
}

#else /* !OBJ_BREP */

int
main(int argc, char *argv[])
{
    bu_log("ERROR: Boundary Representation object support is not available with\n"
	   "       this compilation of BRL-CAD.\n");
    return 1;
}

#endif /* OBJ_BREP */

/*
 * Local Variables:
 * tab-width: 8
 * mode: C++
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
