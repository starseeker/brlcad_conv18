/*
 *  Authors -
 *	John R. Anderson
 *
 *  Source -
 *	SLAD/BVLD/VMB
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1995 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "./iges_struct.h"
#include "./iges_extern.h"

int
Add_nurb_loop_to_face( s, fu, srf, loop_entityno , face_orient )
struct shell *s;
struct faceuse *fu;
struct snurb *srf;
int loop_entityno;
int face_orient;
{ 
	int i,j,k;
	int entity_type;
	int no_of_edges;
	int no_of_param_curves;
	int vert_no;
	int start_vert;
	int vert_inc;
	struct faceuse *fu_ret;
	struct loopuse *lu;
	struct edgeuse *eu;
	struct vertex **verts;
	struct iges_edge_use *edge_uses;

	if( dir[loop_entityno]->param <= pstart )
	{
		printf( "Illegal parameter pointer for entity D%07d (%s), loop ignored\n" ,
				dir[loop_entityno]->direct , dir[loop_entityno]->name );
		return( 0 );
	}

	if( dir[loop_entityno]->type != 508 )
	{
		rt_log( "Entity #%d is not a loop (it's a %s)\n" , loop_entityno , iges_type(dir[loop_entityno]->type) );
		rt_bomb( "Fatal error\n" );
	}

	Readrec( dir[loop_entityno]->param );
	Readint( &entity_type , "" );
	if( entity_type != 508 )
	{
		rt_log( "Add_nurb_loop_to_face: Entity #%d is not a loop (it's a %s)\n",
				loop_entityno , iges_type(entity_type) );
		rt_bomb( "Add_nurb_loop_to_face: Fatal error\n" );
	}

	Readint( &no_of_edges , "" );

	edge_uses = (struct iges_edge_use *)rt_calloc( no_of_edges , sizeof( struct iges_edge_use ) ,
			"Add_nurb_loop_to_face (edge_uses)" );
	for( i=0 ; i<no_of_edges ; i++ )
	{
		Readint( &edge_uses[i].edge_is_vertex , "" );
		Readint( &edge_uses[i].edge_de , "" );
		Readint( &edge_uses[i].index , "" );
		Readint( &edge_uses[i].orient , "" );
		if( !face_orient ) /* need opposite orientation of edge */
		{
			if( edge_uses[i].orient )
				edge_uses[i].orient = 0;
			else
				edge_uses[i].orient = 1;
		}
		edge_uses[i].root = (struct iges_param_curve *)NULL;
		Readint( &no_of_param_curves , "" );
		for( j=0 ; j<no_of_param_curves ; j++ )
		{
			struct iges_param_curve *new_crv;
			struct iges_param_curve *crv;

			Readint( &k , "" );	/* ignore iso-parametric flag */
			new_crv = (struct iges_param_curve *)rt_malloc( sizeof( struct iges_param_curve ),
				"Add_nurb_loop_to_face: new_crv" );
			if( edge_uses[i].root == (struct iges_param_curve *)NULL )
				edge_uses[i].root = new_crv;
			else
			{
				crv = edge_uses[i].root;
				while( crv->next != (struct iges_param_curve *)NULL )
					crv = crv->next;
				crv->next = new_crv;
			}
			Readint( &new_crv->curve_de, "" );
			new_crv->next = (struct iges_param_curve *)NULL;
		}
	}

	verts = (struct vertex **)rt_calloc( no_of_edges , sizeof( struct vertex *) ,
		"Add_nurb_loop_to_face: vertex_list **" );

	if( face_orient )
	{
		start_vert = 0;
		vert_inc = 1;
	}
	else
	{
		start_vert = no_of_edges - 1;
		vert_inc = (-1);
	}
	k = (-1);
	for( i=start_vert ; i<no_of_edges&&i>=0 ; i += vert_inc )
	{
		struct vertex **v;

		v = Get_vertex( &edge_uses[i] );
		if( *v )
			verts[++k] = (*v);
		else
			verts[++k] = (struct vertex *)NULL;
	}

	fu_ret = nmg_add_loop_to_face( s, fu, verts, no_of_edges, OT_SAME );
	k = (-1);
	for( i=start_vert ; i<no_of_edges&&i>=0 ; i += vert_inc )
	{
		struct vertex **v;

		v = Get_vertex( &edge_uses[i] );
		if( !(*v) )
		{
			if( !Put_vertex( verts[++k], &edge_uses[i] ) )
			{
				rt_log( "Cannot put vertex x%x\n", verts[k] );
				rt_bomb( "Cannot put vertex\n" );
			}
		}
		else
			++k;
	}

	lu = RT_LIST_LAST( loopuse, &fu_ret->lu_hd );
	NMG_CK_LOOPUSE( lu );
	i = no_of_edges - 1;
	k = no_of_edges - 1;
	for( RT_LIST_FOR( eu, edgeuse, &lu->down_hd ) )
	{
		struct vertex **v;
		struct iges_vert *ivert;

		v = Get_vertex( &edge_uses[i] );

		if( *v != eu->vu_p->v_p )
		{
			nmg_jv( *v, eu->vu_p->v_p );
			verts[k] = *v;
		}
		k += vert_inc;
		if( k == no_of_edges )
			k = 0;
		if( k < 0 )
			k = no_of_edges - 1;
		i++;
		if( i == no_of_edges )
			i = 0;
	}

	/* assign geometry to vertices */
	for( vert_no=0 ; vert_no < no_of_edges ; vert_no++ )
	{
		struct iges_vertex *ivert;

		ivert = Get_iges_vertex( verts[vert_no] );
		if( !ivert )
		{
			rt_log( "Vertex x%x not in vertex list\n" , verts[vert_no] );
			rt_bomb( "Can't get geometry for vertex" );
		}
		nmg_vertex_gv( ivert->v, ivert->pt );
	}

	/* assign geometry to edges */
	i = no_of_edges - 1;
	for( RT_LIST_FOR( eu, edgeuse, &lu->down_hd ) )
	{
		int next_edge_no;
		struct iges_param_curve *param;
		struct vertex *ivert,*jvert;

		next_edge_no = i + 1;
		if( next_edge_no == no_of_edges )
			next_edge_no = 0;

		ivert = (*Get_vertex( &edge_uses[i] ) );
		if( !ivert )
			rt_bomb( "Cannot get vertex for edge_use!\n" );
		jvert = (*Get_vertex( &edge_uses[next_edge_no] ) );
		if( !jvert )
			rt_bomb( "Cannot get vertex for edge_use!\n" );

		if( ivert != eu->vu_p->v_p || jvert != eu->eumate_p->vu_p->v_p )
		{
			rt_log( "ivert=x%x, jvert=x%x, eu->vu_p->v_p=x%x, eu->eumate_p->vu_p->v_p=x%x\n",
				ivert,jvert,eu->vu_p->v_p,eu->eumate_p->vu_p->v_p );
			rt_bomb( "Add_nurb_loop_to_face: Edgeuse/vertex mixup!\n" );
		}

		param = edge_uses[i].root;
		if( !param )
		{
			rt_log( "No parameter curve for eu x%x\n", eu );
			continue;
		}

		while( param )
		{
			int linear;
			int coords;
			struct cnurb *crv;
			struct vertex *v1,*v2;
			struct edgeuse *new_eu;
			point_t start_uv;
			point_t end_uv;
			hpoint_t pt_on_srf;

			v2 = (struct vertex *)NULL;

			/* Get NURB curve in parameter space for This edgeuse */
			crv = Get_cnurb_curve( param->curve_de, &linear );

			coords = RT_NURB_EXTRACT_COORDS( crv->pt_type );
			VMOVE( start_uv, crv->ctl_points );
			VMOVE( end_uv, &crv->ctl_points[(crv->c_size-1)*coords] );
			if( coords == 2 )
			{
				start_uv[2] = 1.0;
				end_uv[2] = 1.0;
			}

			if( param->next )
			{
				v1 = eu->vu_p->v_p;
				/* need to split this edge to agree with parameter curves */
				new_eu = nmg_esplit( v2, eu, 0 );

				/* evaluate srf at the parameter values for v2 to get geometry */
				if( RT_NURB_EXTRACT_PT_TYPE( crv->pt_type ) == RT_NURB_PT_UV &&
					RT_NURB_IS_PT_RATIONAL( crv->pt_type ) )
				{
					rt_nurb_s_eval( srf, end_uv[0]/end_uv[2],
						end_uv[1]/end_uv[2], pt_on_srf );
				}
				else if( coords == 2 )
					rt_nurb_s_eval( srf, end_uv[0], end_uv[1], pt_on_srf );

				if( RT_NURB_IS_PT_RATIONAL( srf->pt_type ) )
				{
					fastf_t scale;

					scale = 1.0/pt_on_srf[3];
					VSCALE( pt_on_srf, pt_on_srf, scale );
				}
				nmg_vertex_gv( v2, pt_on_srf );
			}
			else
				new_eu = (struct edgeuse *)NULL;

			if( eu->vu_p->v_p == eu->eumate_p->vu_p->v_p )
			{
				/* this edge runs to/from one vertex, need to split */
				struct rt_list split_hd;
				struct cnurb *crv1,*crv2;
				point_t start_uv, end_uv;
				hpoint_t pt_on_srf;

				RT_LIST_INIT( &split_hd );

				/* split the edge */
				v2 = (struct vertex *)NULL;
				new_eu = nmg_esplit( v2, eu, 0 );

				/* split the curve */
				rt_nurb_c_split( &split_hd, crv );
				crv1 = RT_LIST_FIRST( cnurb, &split_hd );
				crv2 = RT_LIST_LAST( cnurb, &split_hd );

				/* get geometry for new vertex */
				coords = RT_NURB_EXTRACT_COORDS( crv1->pt_type );
				VMOVE( start_uv, crv1->ctl_points );
				VMOVE( end_uv, &crv1->ctl_points[(crv1->c_size-1)*coords] );
				if( RT_NURB_EXTRACT_PT_TYPE( crv1->pt_type ) == RT_NURB_PT_UV &&
					RT_NURB_IS_PT_RATIONAL( crv1->pt_type ) )
				{
					rt_nurb_s_eval( srf, end_uv[0]/end_uv[2],
						end_uv[1]/end_uv[2], pt_on_srf );
				}
				else
				{
					start_uv[2] = 1.0;
					end_uv[2] = 1.0;
					rt_nurb_s_eval( srf, end_uv[0], end_uv[1], pt_on_srf );
				}

				if( RT_NURB_IS_PT_RATIONAL( srf->pt_type ) )
				{
					fastf_t scale;

					scale = 1.0/pt_on_srf[3];
					VSCALE( pt_on_srf, pt_on_srf, scale );
				}

				/* assign geometry */
				nmg_vertex_gv( new_eu->vu_p->v_p, pt_on_srf );
				nmg_vertexuse_a_cnurb( eu->vu_p, start_uv );
				nmg_vertexuse_a_cnurb( eu->eumate_p->vu_p, end_uv );
				if( linear )
					nmg_edge_g_cnurb_plinear( eu );
				else
					Assign_cnurb_to_eu( eu, crv1 );

				/* now the second section */
				coords = RT_NURB_EXTRACT_COORDS( crv2->pt_type );
				VMOVE( start_uv, crv2->ctl_points );
				VMOVE( end_uv, &crv2->ctl_points[(crv2->c_size-1)*coords] );
				if( RT_NURB_EXTRACT_PT_TYPE( crv2->pt_type ) == RT_NURB_PT_UV &&
					RT_NURB_IS_PT_RATIONAL( crv2->pt_type ) )
				{
					rt_nurb_s_eval( srf, start_uv[0]/start_uv[2],
						start_uv[1]/start_uv[2], pt_on_srf );
				}
				else
				{
					start_uv[2] = 1.0;
					end_uv[2] = 1.0;
					rt_nurb_s_eval( srf, start_uv[0], start_uv[1], pt_on_srf );
				}

				if( RT_NURB_IS_PT_RATIONAL( srf->pt_type ) )
				{
					fastf_t scale;

					scale = 1.0/pt_on_srf[3];
					VSCALE( pt_on_srf, pt_on_srf, scale );
				}

				/* assign geometry */
				nmg_vertexuse_a_cnurb( new_eu->vu_p, start_uv );
				nmg_vertexuse_a_cnurb( new_eu->eumate_p->vu_p, end_uv );
				if( linear )
					nmg_edge_g_cnurb_plinear( new_eu );
				else
					Assign_cnurb_to_eu( new_eu, crv2 );

				/* free memory */
				while( RT_LIST_NON_EMPTY( &split_hd ) )
				{
					struct cnurb *tmp_crv;

					tmp_crv = RT_LIST_FIRST( cnurb, &split_hd );
					RT_LIST_DEQUEUE( &tmp_crv->l );
					rt_free( (char *)tmp_crv, "Add_nurb_loop_to_face: tmp_crv" );
				}
			}
			else
			{
				nmg_vertexuse_a_cnurb( eu->vu_p, start_uv );
				nmg_vertexuse_a_cnurb( eu->eumate_p->vu_p, end_uv );

				if( linear )
					nmg_edge_g_cnurb_plinear( eu );
				else
					Assign_cnurb_to_eu( eu, crv );
			}

			rt_free( (char *)crv->knot.knots, "Add_nurb_loop_to_face: crv->knot.knots" );
			rt_free( (char *)crv->ctl_points, "Add_nurb_loop_to_face: crv->ctl_points" );
			rt_free( (char *)crv, "Add_nurb_loop_to_face: crv" );

			if( new_eu )
				eu = new_eu;

			param = param->next;
		}
		i++;
		if( i == no_of_edges )
			i = 0;
	}

	return( 1 );
err:
	for( i=0 ; i<no_of_edges ; i++ )
	{
		struct iges_param_curve *crv;

		crv = edge_uses[i].root;
		while( crv )
		{
			struct iges_param_curve *tmp_crv;

			tmp_crv = crv;
			crv = crv->next;
			rt_free( (char *)tmp_crv, "Add_nurb_loop_to_face: tmp_crv" );
		}
	}
	rt_free( (char *)edge_uses, "Add_nurb_loop_to_face: (edge list)" );
	rt_free( (char *)verts, "Add_nurb_loop_to_face: (vertex list)" );

	return( 0 );
}

struct faceuse *
Make_nurb_face( srf, s, surf_entityno )
struct snurb **srf;
struct shell *s;
int surf_entityno;
{
	struct vertex *verts[1];
	struct loopuse *lu;
	struct faceuse *fu;

	if( dir[surf_entityno]->type != 128 )
	{
		rt_log( "Make_nurb_face: Called with surface entity (%d) of type %s\n", surf_entityno,
			iges_type( dir[surf_entityno]->type ) );
		rt_log( "Make_nurb_face: Can only handle surfaces of %s, ignoring face\n", iges_type( 128 ) );
		return( (struct faceuse *)NULL );
	}

	if( ((*srf) = Get_nurb_surf( surf_entityno )) == (struct snurb *)NULL )
	{
		rt_log( "Make_nurb_face: Get_nurb_surf failed for surface entity (%d), face ignored\n",	 surf_entityno );
		return( (struct faceuse *)NULL );
	}

	verts[0] = (struct vertex *)NULL;

	fu = nmg_cface( s, verts, 1 );
	Assign_surface_to_fu( fu, *srf );

	lu = RT_LIST_FIRST( loopuse, &fu->lu_hd );
	(void)nmg_klu( lu );

	return( fu );
}
