/*	E U C L I D _ U N F O R M A T
 *
 * Converts from Euclid decoded format to no CR format
 *
 */

#ifndef lint
static char RCSid[] = "$Header$";
#endif

#include "conf.h"

#include <stdio.h>
#include <math.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "externs.h"
#include "vmath.h"
#include "nmg.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "../librt/debug.h"

#define MAX_PTS	500

main()
{
	char str[1024];
	point_t pts[MAX_PTS];
	int ident,face_type,npts,face_no;
	plane_t pl;
	int i;
	int tmp_i;
	float tmp_a;
	float a,b,c,d;
	int old_id=(-1),e;
	int face_count;

	printf( "$03" );

	while( gets( str ) )
	{
		sscanf( str, "%d %d %d %d %d %f", &face_no, &npts, &face_type, &e, &ident, &a );

		if( ident > 0 )
			old_id = ident;

		if( !gets( str ) )
			break;

		sscanf( str, "%f %f %f %f", &a, &b, &c, &d );
		QSET( pl, a, b, c, d )

		if( npts > 2 )
			printf( "%10d%3d%7.0f%5d%5d", old_id, face_type, 0.0, 1, npts );

		for( i=0 ; i<npts ; i++ )
		{
			if( i >= MAX_PTS )
			{
				fprintf( stderr , "Too many points, MAX is %d\n" , MAX_PTS );
				exit( 1 );
			}
			if( !gets( str ) )
			{
				fprintf( stderr, "Unexpected EOF\n" );
				break;
			}
			sscanf( str, "%f %f %f", &a, &b, &c );
			if( npts > 2 )
				printf( "%10d%8.1f%8.1f%8.1f", i+1, a, b, c );
		}
		if( npts > 2 )
			printf( "         1%15.5f%15.5f%15.5f%15.5f", V4ARGS( pl ) );
	}
}
