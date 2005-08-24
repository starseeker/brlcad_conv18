/*
 *			S T R C H R
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "common.h"

/*
 * defined for folks that don't have a system strchr()
 */
#ifndef HAVE_STRCHR

char *
strchr(register char *sp, register char c)
{
	do {
		if( *sp == c )
			return( sp );
	}  while( *sp++ );
	return( (char *)0 );
}

#endif

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
