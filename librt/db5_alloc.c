/*
 *			D B _ A L L O C 5 . C
 *
 *  Handle disk space allocation in the BRL-CAD v5 database.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimited.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "db5.h"
#include "raytrace.h"

#include "./debug.h"


/*
 *			D B 5 _ R E A L L O C
 *
 *  Change the size of a v5 database object.
 *
 *  If the object is getting smaller, break it into two pieces,
 *  and write out free records for both.
 *  The caller is expected to re-write new data on the first one.
 *
 *  If the object is getting larger, see if it can be extended in place.
 *  If yes, write a free record that fits the new size,
 *  and a new free record for any remaining space.
 *
 *  If the ojbect is getting larger and there is no suitable "hole"
 *  in the database, extend the file, write a free record in the
 *  new space, and write a free record in the old space.
 *
 *  Returns -
 *	0	OK
 *	-1	Failure
 */
int
db5_realloc( struct db_i *dbip, struct directory *dp, struct bu_external *ep )
{
	RT_CK_DBI(dbip);
	RT_CK_DIR(dp);
	BU_CK_EXTERNAL(ep);
	if(rt_g.debug&DEBUG_DB) bu_log("db5_realloc(%s) dbip=x%x, dp=x%x, ext_nbytes=%ld\n",
		dp->d_namep, dbip, dp, ep->ext_nbytes );


	bu_bomb("db5_realloc() not fully implemented\n");
}
