/*
 *  Authors -
 *	John R. Anderson
 *	Susanne L. Muuss
 *	Earl P. Weaver
 *
 *  Source -
 *	VLD/ASB Building 1065
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */

#include "conf.h"

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "./iges_struct.h"
#include "./iges_extern.h"
#include "wdb.h"

Convinst()
{

	int			i,j,k;
	int			type;
	int			pointer;
	int			conv=0;
	int			totinst=0;
	int			no_of_assoc=0;
	int			no_of_props=0;
	int			att_de=0;
	struct brlcad_att	brl_att;
	mat_t			*rot;

	for( i=0 ; i<totentities ; i++ )
	{
		if( dir[i]->type != 430 ) /* This is not an instance */
			continue;

		totinst++;

		/* read parameters */
		if( dir[i]->param <= pstart )
		{
			printf( "Illegal parameter pointer for entity D%07d (%s)\n" ,
					dir[i]->direct , dir[i]->name );
			continue;
		}
		Readrec( dir[i]->param );
		Readint( &type , "" );
		Readint( &pointer , "" );

		/* convert pointer to a "dir" index */
		pointer = (pointer - 1)/2;
		if( pointer < 0 || pointer >= totentities )
		{
			printf( "Solid instance D%07d (%s) does not point to a legal solid\n",
				dir[i]->direct , dir[i]->name );
			continue;
		}

		/* skip over the associativities */
		Readint( &no_of_assoc , "" );
		for( k=0 ; k<no_of_assoc ; k++ )
			Readint( &j , "" );

		/* get property entity DE's */
		Readint( &no_of_props , "" );
		for( k=0 ; k<no_of_props ; k++ )
		{
			Readint( &j , "" );
			if( dir[(j-1)/2]->type == 422 &&
				 dir[(j-1)/2]->referenced == brlcad_att_de )
			{
				/* this is one of our attribute instances */
				att_de = j;
			}
		}

		Read_att( att_de , &brl_att );

		if( att_de )
		{
			/* This is actually a region or a group with just one member */
			unsigned char *rgb;
			struct wmember head;

			RT_LIST_INIT( &head.l );
			(void)mk_addmember( dir[pointer]->name , &head , WMOP_INTERSECT );

			/* Make the object */
			if( dir[i]->colorp != 0 )
				rgb = (unsigned char*)dir[i]->rgb;
			else
				rgb = (unsigned char *)0;

			mk_lrcomb( fdout , 
				dir[i]->name,		/* name */
				&head,			/* members */
				brl_att.region_flag,	/* region flag */
				brl_att.material_name,	/* material name */
				brl_att.material_params, /* material parameters */
				rgb,			/* color */
				brl_att.ident,		/* ident */
				brl_att.air_code,	/* air code */
				brl_att.material_code,	/* GIFT material */
				brl_att.los_density,	/* los density */
				brl_att.inherit );	/* inherit */

		}
		else
		{
			/* copy pointed to object info to replace instance entity */
			dir[i]->type = dir[pointer]->type;
			dir[i]->form = dir[pointer]->form;
			dir[i]->param = dir[pointer]->param;

			/* increment reference count for pointed to entity */
			dir[pointer]->referenced++;

			/* fix up transformation matrix if needed */
			if( dir[i]->trans == 0 && dir[pointer]->trans == 0 )
			{
				rt_log( "Instance and pointed to object both have no xforms\n" );
				continue;	/* nothing to do */
			}
			else if( dir[i]->trans == 0 )
			{
				dir[i]->trans = dir[pointer]->trans;	/* same as instanced */
				rt_log( "use pointed to object's xform\n" );
			}
			else if( dir[i]->trans != 0 )
			{
				/* this instance refers to a transformation entity
				   but the original instanced object does too,
				   these matrices need to be combined */

				rt_log( "Must do a concatonation\n" );

				rot = (mat_t *)rt_malloc( sizeof( mat_t ), "Convinst: rot" );
				Matmult( *(dir[i]->rot) , dir[pointer]->rot , rot );
				dir[i]->rot = rot;
			}
		}
		conv++;
	}

	printf( "\nConverted %d solid instances out of %d total instances\n" , conv , totinst );
}
