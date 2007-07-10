/*                    P O P U L A T I O N . C
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
/** @file population.c
 *
 * routines to manipulate the population
 *
 * Author -
 *   Ben Poole
 */

#include "common.h"

#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

#include "machine.h"
#include "bu.h"
#include "bn.h"
#include "vmath.h"
#include "raytrace.h"
#include "wdb.h"

#include "population.h"
#include "beset.h" // GOP options

//cut when pop_random() updated
float *idx;
int shape_number; //ugly but keeping

/**
 *	P O P _ I N I T --- initialize a population of a given size
 */
void 
pop_init (struct population **p, int size)
{
    *p = bu_malloc(sizeof(struct population), "population");

    (*p)->parent = bu_malloc(sizeof(struct individual) * size, "parent");
    (*p)->child  = bu_malloc(sizeof(struct individual) * size, "child");
    (*p)->size = size;
    (*p)->db_p = db_create("gen00", 5); //FIXME: variable names
    (*p)->db_p->dbi_wdbp = wdb_dbopen((*p)->db_p, RT_WDB_TYPE_DB_DISK);
    (*p)->db_c = DBI_NULL;

#define SEED 33
    // init in main() bn_rand_init(idx, SEED);
    // fix this and modularize somehow
    bn_rand_init(idx, SEED);
}

/**
 *	P O P _ C L E A N  --- cleanup population struct
 */
void 
pop_clean (struct population *p)
{
    bu_free(p->parent, "parent");
    bu_free(p->child, "child");
    bu_free(p, "population");
}

/**
 *	P O P _ S P A W N --- spawn a new population
 *	TODO: generalize/modularize somehow to allow adding more shapes and primitives
 *	also use variable/defined rates, intersection with bounding box, etc...
 */
void 
pop_spawn (struct population *p, struct rt_wdb *db_fp)
{
    int i;
    point_t p1, p2;
    struct wmember wm_hd;
    double r1, r2;

    for(i = 0; i < p->size; i++)
    {

	BU_LIST_INIT(&wm_hd.l);

	p1[0] = -10+pop_rand()*10;
	p1[1] = -10+pop_rand()*10;
	p1[2] = -10+pop_rand()*10;
	r1 = 10*pop_rand();

	p2[0] = -10+pop_rand()*10;
	p2[1] = -10+pop_rand()*10;
	p2[2] = -10+pop_rand()*10;
	r2 = 10*pop_rand();


	p->parent[i].fitness = 0.0;

	snprintf(p->parent[i].id, 256, "gen%.3dind%.3d-%.3d", 0,i,0);
	mk_sph(db_fp, p->parent[i].id, p1, r1);
	mk_addmember(p->parent[i].id, &wm_hd.l, NULL, WMOP_UNION);


	snprintf(p->parent[i].id, 256, "gen%.3dind%.3d-%.3d", 0,i,1);
	mk_sph(db_fp, p->parent[i].id, p2, r2);
	mk_addmember(p->parent[i].id, &wm_hd.l, NULL, WMOP_UNION);


	snprintf(p->parent[i].id, 256, "gen%.3dind%.3d", 0, i);
	mk_lcomb(db_fp, p->parent[i].id, &wm_hd, 0, NULL, NULL, NULL, 0);
    }
}

/**
 *	P O P _ A D D --- add an parent to othe database
 *	TODO: Don't overwrite previous parents, one .g file per generation
 */
/*
void 
pop_add(struct individual *i, struct rt_wdb *db_fp)
{
    switch(i->type)
    {
    case GEO_SPHERE:
	mk_sph(db_fp, i->id, i->p, i->r);
    }
}
*/


/**
 *	P O P _ W R A N D -- weighted random index of parent
 */
int
pop_wrand_ind(struct individual *i, int size, fastf_t total_fitness)
{
    int n = 0;
    fastf_t rindex, psum = 0;
    rindex = bn_rand0to1(idx) * total_fitness;

    psum += i[n].fitness;
    for(n = 1; n < size; n++) {
	psum += i[n].fitness;
	if( rindex <= psum )
	    return n-1;
    }
    return size-1;
}

/**
 *	P O P _ R A N D --- random number (0,1)
 */
fastf_t
pop_rand (void)
{
    return bn_rand0to1(idx);
}

/**
 *	P O P _ R A N D _ G O P --- return a random genetic operation
 *	TODO: implement other operations, weighted (like wrand) op selection
 */
int 
pop_wrand_gop(void)
{
    float i = bn_rand0to1(idx);
    if(i < 0.5)
	return MUTATE_MOD;
    return MUTATE_RAND;
}

void
pop_functree(struct db_i *dbi_p, struct db_i *dbi_c,
		    union tree *tp,
		    struct resource *resp,
		    char *name
	)
{
    struct directory *dp;
    struct rt_db_internal intern;
    char shape[256];

    if( !tp )
	return;

    switch( tp->tr_op )  {

	case OP_DB_LEAF:
	    printf("leaf\n");
	    if( (dp=db_lookup( dbi_p, tp->tr_l.tl_name, LOOKUP_NOISY )) == DIR_NULL )
		return;
	    //rename tree
	    snprintf(shape, 256, "%s-%d", name, shape_number++);
	    printf("he\n");
	    bu_free(tp->tr_l.tl_name, "bu_strdup");
	    tp->tr_l.tl_name = bu_strdup(shape);
	    printf("%s\n", shape);

	    db_rename(dbi_p, dp, shape);

	    if( rt_db_get_internal(&intern, dp, dbi_p, NULL, resp) < 0)
		bu_bomb("failed to read a leaf");
	    if(rt_db_put_internal(dp, dbi_c, &intern, resp) < 0)
		bu_bomb("failed to write leaf");
	    rt_db_free_internal(&intern, resp);
	    printf("copied leaf\n");
	    break;

	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
	    pop_functree( dbi_p, dbi_c, tp->tr_b.tb_left, resp, name);
	    pop_functree( dbi_p, dbi_c, tp->tr_b.tb_right, resp, name);
	    break;
	default:
	    bu_log( "pop_functree: unrecognized operator %d\n", tp->tr_op );
	    bu_bomb( "pop_functree: unrecognized operator\n" );
    }
}

void
pop_dup(char *parent, char *child, struct db_i *dbi_p, struct db_i *dbi_c, struct resource *resp)
{
    RT_CHECK_DBI( dbi_p );
    RT_CHECK_DBI( dbi_c );
    RT_CK_RESOURCE( resp );

    struct rt_db_internal in;
    struct rt_comb_internal *comb;
    struct directory *dp;

    printf("looking up %s\n", parent);
    if( (dp = db_lookup(dbi_p, parent, LOOKUP_NOISY)) == DIR_NULL)
	bu_bomb("db_lookup(parent) failed");
    if(rt_db_get_internal(&in, dp, dbi_p, NULL, resp) < 0 )
	bu_bomb("pop_dup: faled to load");

    shape_number = 0;
    comb = (struct rt_comb_internal *)in.idb_ptr;
    //rename combination
    db_rename(dbi_p, dp, child);
    printf("calling pop_functree\n");
    pop_functree(dbi_p, dbi_c, comb->tree, resp, child);
    rt_db_put_internal(dp, dbi_c, &in, resp);
    rt_db_free_internal(&in, resp);
}

/** @} */
/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
