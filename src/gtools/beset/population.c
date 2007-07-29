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

#include <string.h>
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


//narsty globals -- move to main() ?
float *idx;
int shape_number; 


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
    (*p)->db_p = db_create("gen000", 5); //FIXME: variable names
    (*p)->db_p->dbi_wdbp = wdb_dbopen((*p)->db_p, RT_WDB_TYPE_DB_DISK);
    (*p)->db_c = DBI_NULL;

#define SEED 33
    // init in main() bn_rand_init(idx, SEED);
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
	r1 = 1+3*pop_rand();

	p2[0] = -10+pop_rand()*10;
	p2[1] = -10+pop_rand()*10;
	p2[2] = -10+pop_rand()*10;
	r2 = 1+3*pop_rand();


	p->parent[i].fitness = 0.0;

	snprintf(p->parent[i].id, 256, "gen%.3dind%.3d-%.3d", 0,i,0);
	mk_sph(db_fp, p->parent[i].id, p1, r1);
	mk_addmember(p->parent[i].id, &wm_hd.l, NULL, WMOP_UNION);


	snprintf(p->parent[i].id, 256, "gen%.3dind%.3d-%.3d", 0,i,1);
	mk_sph(db_fp, p->parent[i].id, p2, r2);
	mk_addmember(p->parent[i].id, &wm_hd.l, NULL, WMOP_UNION);


	snprintf(p->parent[i].id, 256, "gen%.3dind%.3d", 0, i);
	mk_lcomb(db_fp, p->parent[i].id, &wm_hd, 1, NULL, NULL, NULL, 0);
    }

/*
 * reload the db so we dont
 * have to do any extra checks
 * in the main looop
 */
    wdb_close(p->db_p->dbi_wdbp);
    if((p->db_p = db_open("gen000", "r")) == DBI_NULL)
	bu_bomb("Failed to re-open initial population");
    if(db_dirbuild(p->db_p) < 0)
	bu_bomb("Failed to load initial database");
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
    rindex =pop_rand() * total_fitness;

    psum = i[n].fitness;
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
    if(i < 0.1)
	return REPRODUCE;
    return CROSSOVER;
}

int node_idx;
int crossover_node;
int crossover;
int crossover_op;
int num_nodes;
union tree *crossover_point;
union tree **crossover_parent;
struct node *node;

int
pop_find_nodes(	union tree *tp)
{
    int n1, n2;
    struct node *add;

    if(!tp)
	return 0;
    
    switch(tp->tr_op){
	case OP_DB_LEAF:
	    return 1;
	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
	    crossover_parent = &tp->tr_b.tb_left;
	    n1 = pop_find_nodes(tp->tr_b.tb_left);
	    if(n1 == crossover_node){
		if(tp->tr_b.tb_left->tr_op & crossover_op){
		add = bu_malloc(sizeof(struct node), "node");
		add->s_parent = &tp->tr_b.tb_left;
		add->s_child = tp->tr_b.tb_left;
		BU_LIST_INSERT(&node->l, &add->l);
		++num_nodes;
		}
	    }
	    crossover_parent = &tp->tr_b.tb_right;
	    n2 = pop_find_nodes(tp->tr_b.tb_right);
	    if(n2 == crossover_node){
		if(tp->tr_b.tb_right->tr_op & crossover_op){
		add = bu_malloc(sizeof(struct node), "node");
		add->s_parent = &tp->tr_b.tb_right;
		add->s_child = tp->tr_b.tb_right;
		BU_LIST_INSERT(&node->l, &add->l);
		++num_nodes;
		}
	    }
	    return 1+n1 + n2;
    }
}








void
pop_functree(struct db_i *dbi_p, struct db_i *dbi_c,
		    union tree *tp,
		    struct resource *resp,
		    char *name
	)
{
    struct directory *dp;
    struct rt_db_internal in;
    char shape[256];

    if( !tp )
	return;
    if(crossover){
	if(node_idx > crossover_node) return;
	if(node_idx == crossover_node){
	    crossover_point = tp;
	    ++node_idx;
	    return;
	}
	else
	    ++node_idx;
    }

    switch( tp->tr_op )  {

	case OP_DB_LEAF:
	    if(crossover)
		return;

	    if( !rt_db_lookup_internal(dbi_p, tp->tr_l.tl_name, &dp, &in, LOOKUP_NOISY, resp))
		bu_bomb("Failed to read parent");

	    //rename tree
	    snprintf(shape, 256, "%s-%.3d", name, shape_number++);
	    bu_free(tp->tr_l.tl_name, "bu_strdup");
	    tp->tr_l.tl_name = bu_strdup(shape);

	    if((dp=db_diradd(dbi_c, shape, -1, 0, dp->d_flags, (genptr_t)&dp->d_minor_type)) == DIR_NULL)
		bu_bomb("Failed to add new object to the database");
	    if(rt_db_put_internal(dp, dbi_c, &in, resp) < 0)
		bu_bomb("Failed to write new individual to databse");
	    rt_db_free_internal(&in, resp);//error check?

	    break;


	case OP_UNION:
	case OP_INTERSECT:
	case OP_SUBTRACT:
	case OP_XOR:
	    if(crossover && node_idx == crossover_node)
		crossover_parent = &tp->tr_b.tb_left;
	    pop_functree( dbi_p, dbi_c, tp->tr_b.tb_left, resp, name);
	    if(crossover && node_idx == crossover_node)
		crossover_parent = &tp->tr_b.tb_right;
	    pop_functree( dbi_p, dbi_c, tp->tr_b.tb_right, resp, name);
	    break;
	default:
	    bu_log( "pop_functree: unrecognized operator %d\n", tp->tr_op );
	    bu_bomb( "pop_functree: unrecognized operator\n" );
    }
}

void
pop_gop(int gop, char *parent1_id, char *parent2_id, char *child1_id, char *child2_id, struct db_i *dbi_p, struct db_i *dbi_c, struct resource *resp)
{
    RT_CHECK_DBI( dbi_p );
    RT_CHECK_DBI( dbi_c );
    RT_CK_RESOURCE( resp );

    struct rt_db_internal in1, in2;
    struct rt_comb_internal *parent1;
    struct rt_comb_internal *parent2, *swap;
    struct directory *dp;
    union tree *cpoint, **tmp, **cross_parent;
    struct node *add;
    int i = 0;
    
    struct node *chosen_node;
    int rand_node;
    if( !rt_db_lookup_internal(dbi_p, parent1_id, &dp, &in1, LOOKUP_NOISY, &rt_uniresource))
	bu_bomb("Failed to read parent1");
    shape_number =num_nodes= 0;
    parent1 = (struct rt_comb_internal *)in1.idb_ptr;

    switch(gop){
	case REPRODUCE:
	    pop_functree(dbi_p, dbi_c, parent1->tree, resp, child1_id);
	    break;
	case CROSSOVER:
	    crossover = 1;
	    //load other parent
	    if( !rt_db_lookup_internal(dbi_p, parent2_id, &dp, &in2, LOOKUP_NOISY, resp))
		bu_bomb("Failed to read parent2");
	    parent2 = (struct rt_comb_internal *)in2.idb_ptr;

	    //temp: swap left trees
	    //pick two random nodes
	    //head node is the node we want to cross
	    node = bu_malloc(sizeof(struct node), "node");
	    BU_LIST_INIT(&node->l);

	    	   
	    
	    chosen_node = NULL;
	    do{
		num_nodes = 0;
		crossover_parent = &parent1->tree;
		crossover_node = (int)(pop_rand() * db_count_tree_nodes(parent1->tree,0));
		node_idx = 0;
		pop_functree(dbi_p, dbi_c, parent1->tree, resp, NULL);
		cross_parent = crossover_parent;
		cpoint = crossover_point;


		crossover_op = crossover_point->tr_op;
#define MASK (OP_UNION | OP_XOR | OP_SUBTRACT|OP_INTERSECT)
		if(crossover_op & MASK)crossover_op = MASK;
		crossover_node = db_count_tree_nodes(crossover_point, 0);
		if(pop_find_nodes(parent2->tree) == crossover_node){
		    add = bu_malloc(sizeof(struct node), "node");
		    add->s_parent = &parent2->tree;
		    add->s_child = parent2->tree;
		    BU_LIST_INSERT(&node->l, &add->l);
		    ++num_nodes;
		}
		if(num_nodes > 0){
		    rand_node = (int)(pop_rand() * num_nodes);
		    for(BU_LIST_FOR(add, node, &node->l)){
			if(i++ == rand_node){
			    chosen_node = add;
			    //BREAK CLEANLY!?!?
			}
		    }
		}
	    }while(chosen_node == NULL);


	    
	    //TODO: MEMORY LEAKS
	    *cross_parent = db_dup_subtree(chosen_node->s_child,resp);
	    *chosen_node->s_parent =db_dup_subtree(cpoint,resp);

	    while(BU_LIST_WHILE(add, node, &node->l)){
		BU_LIST_DEQUEUE(&add->l);
		bu_free(add, "node");
	    }
	    bu_free(node, "node");




	    crossover = 0;

	    //duplicate shapes held in trees
	    pop_functree(dbi_p, dbi_c, parent1->tree, resp, child1_id);
	    shape_number = 0;
	    pop_functree(dbi_p, dbi_c, parent2->tree, resp, child2_id);


	    if((dp = db_diradd(dbi_c, child2_id, -1, 0, dp->d_flags, (genptr_t)&dp->d_minor_type)) == DIR_NULL)
		bu_bomb("Failed to add new individual to child database");
	    rt_db_put_internal(dp, dbi_c, &in2, resp);
	    rt_db_free_internal(&in2, resp);

	    break;
	case MUTATE:
	    break;
	    /*
	    //random node to mutate
	    n = (int)(pop_rand() * db_count_tree_nodes(parent1->tree,0));
	    s_parent = &parent1->tree;
	    s_node = n;
	    node = 0;
	    //find node
	    pop_functree(dbi_p, dbi_c, parent1->tree, resp, NULL);
*/




	default:
	    bu_log("illegal genetic operator");
	    bu_bomb("failed to execute genetic op");
    }


    if((dp=db_diradd(dbi_c, child1_id, -1, 0, dp->d_flags, (genptr_t)&dp->d_minor_type)) == DIR_NULL){
	bu_bomb("Failed to add new individual to child database");
    }
    rt_db_put_internal(dp, dbi_c,  &in1, resp);
    rt_db_free_internal(&in1, resp);
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
