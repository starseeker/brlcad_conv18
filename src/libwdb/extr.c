/*                          E X T R . C
 * BRL-CAD
 *
 * Copyright (c) 2000-2004 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file; see the file named COPYING for more
 * information.
 */
/** @file extr.c
 *
 * Support for extrusion solids
 *
 *  Author -
 *	John Anderson
 *
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Package" agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 2000-2004 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */
#ifndef lint
static const char extr_RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "common.h"



#include <stdio.h>
#include <math.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "machine.h"
#include "bu.h"
#include "db.h"
#include "vmath.h"
#include "bn.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "wdb.h"

int
mk_extrusion(
	struct rt_wdb *fp,
	const char *name,
	const char *sketch_name,
	const point_t V,
	const vect_t h,
	const vect_t u_vec,
	const vect_t v_vec,
	int keypoint )
{
	struct rt_extrude_internal *extr;

	BU_GETSTRUCT( extr, rt_extrude_internal );
	extr->magic = RT_EXTRUDE_INTERNAL_MAGIC;
	extr->sketch_name = bu_strdup( sketch_name );
	VMOVE( extr->V, V );
	VMOVE( extr->h, h );
	VMOVE( extr->u_vec, u_vec );
	VMOVE( extr->v_vec, v_vec );
	extr->keypoint = keypoint;
	extr->skt = (struct rt_sketch_internal *)NULL;

	return wdb_export( fp, name, (genptr_t)extr, ID_EXTRUDE, mk_conv2mm );
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
