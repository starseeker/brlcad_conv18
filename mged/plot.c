/*
 *  			P L O T . C
 *  
 *	Provide UNIX-plot output of the current view.
 *
 *  Authors -
 *  	Michael John Muuss	(This version)
 *	Douglas A. Gwyn		(3-D UNIX Plot routines)
 *  	Gary S. Moss		(Original gedplot program)
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1985 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <math.h>
#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "mater.h"
#include "rtstring.h"
#include "raytrace.h"
#include "./ged.h"
#include "externs.h"
#include "plot3.h"
#include "./solid.h"
#include "./dm.h"

/*
 *  			F _ P L O T
 *
 *  plot file [opts]
 *  potential options might include:
 *	grid, 3d w/color, |filter, infinite Z
 */
int
f_plot(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int	argc;
char	**argv;
{
	register struct solid		*sp;
	register struct rt_vlist	*vp;
	register FILE *fp;
	static vect_t clipmin, clipmax;
	static vect_t last;		/* last drawn point */
	static vect_t fin;
	static vect_t start;
	int Three_D;			/* 0=2-D -vs- 1=3-D */
	int Z_clip;			/* Z clipping */
	int Dashing;			/* linetype is dashed */
	int floating;			/* 3-D floating point plot */
	int	is_pipe = 0;

	if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
	  return TCL_ERROR;

	if( not_state( ST_VIEW, "UNIX Plot of view" ) )
	  return TCL_ERROR;

	/* Process any options */
	Three_D = 1;				/* 3-D w/color, by default */
	Z_clip = 0;				/* NO Z clipping, by default*/
	floating = 0;
	while( argv[1] != (char *)0 && argv[1][0] == '-' )  {
		switch( argv[1][1] )  {
		case 'f':
			floating = 1;
			break;
		case '3':
			Three_D = 1;
			break;
		case '2':
			Three_D = 0;		/* 2-D, for portability */
			break;
		case 'g':
		  /* do grid */
		  Tcl_AppendResult(interp, "grid unimplemented\n", (char *)NULL);
		  break;
		case 'z':
		case 'Z':
		  /* Enable Z clipping */
		  Tcl_AppendResult(interp, "Clipped in Z to viewing cube\n", (char *)NULL);
		  Z_clip = 1;
		  break;
		default:
		  Tcl_AppendResult(interp, "bad PLOT option ", argv[1], "\n", (char *)NULL);
		  break;
		}
		argv++;
	}
	if( argv[1] == (char *)0 )  {
	  Tcl_AppendResult(interp, "no filename or filter specified\n", (char *)NULL);
	  return TCL_ERROR;
	}
	if( argv[1][0] == '|' )  {
		struct rt_vls	str;
		rt_vls_init( &str );
		rt_vls_strcpy( &str, &argv[1][1] );
		while( (++argv)[1] != (char *)0 )  {
			rt_vls_strcat( &str, " " );
			rt_vls_strcat( &str, argv[1] );
		}
		if( (fp = popen( rt_vls_addr( &str ), "w" ) ) == NULL )  {
			perror( rt_vls_addr( &str ) );
			return TCL_ERROR;
		}

		Tcl_AppendResult(interp, "piped to ", rt_vls_addr( &str ),
				 "\n", (char *)NULL);
		rt_vls_free( &str );
		is_pipe = 1;
	}  else  {
		if( (fp = fopen( argv[1], "w" )) == NULL )  {
		  perror( argv[1] );
		  return TCL_ERROR;
		}

		Tcl_AppendResult(interp, "plot stored in ", argv[1], "\n", (char *)NULL);
		is_pipe = 0;
	}

	color_soltab();		/* apply colors to the solid table */

	if( floating )  {
		pd_3space( fp,
			-toViewcenter[MDX] - Viewscale,
			-toViewcenter[MDY] - Viewscale,
			-toViewcenter[MDZ] - Viewscale,
			-toViewcenter[MDX] + Viewscale,
			-toViewcenter[MDY] + Viewscale,
			-toViewcenter[MDZ] + Viewscale );
		Dashing = 0;
		pl_linmod( fp, "solid" );
		FOR_ALL_SOLIDS( sp )  {
			/* Could check for differences from last color */
			pl_color( fp,
				sp->s_color[0],
				sp->s_color[1],
				sp->s_color[2] );
			if( Dashing != sp->s_soldash )  {
				if( sp->s_soldash )
					pl_linmod( fp, "dotdashed");
				else
					pl_linmod( fp, "solid");
				Dashing = sp->s_soldash;
			}
			rt_vlist_to_uplot( fp, &(sp->s_vlist) );
		}
		goto out;
	}

	/*
	 *  Integer output version, either 2-D or 3-D.
	 *  Viewing region is from -1.0 to +1.0
	 *  which is mapped to integer space -2048 to +2048 for plotting.
	 *  Compute the clipping bounds of the screen in view space.
	 */
	clipmin[X] = -1.0;
	clipmax[X] =  1.0;
	clipmin[Y] = -1.0;
	clipmax[Y] =  1.0;
	if( Z_clip )  {
		clipmin[Z] = -1.0;
		clipmax[Z] =  1.0;
	} else {
		clipmin[Z] = -1.0e20;
		clipmax[Z] =  1.0e20;
	}

	if( Three_D )
		pl_3space( fp, -2048, -2048, -2048, 2048, 2048, 2048 );
	else
		pl_space( fp, -2048, -2048, 2048, 2048 );
	pl_erase( fp );
	Dashing = 0;
	pl_linmod( fp, "solid");
	FOR_ALL_SOLIDS( sp )  {
		if( Dashing != sp->s_soldash )  {
			if( sp->s_soldash )
				pl_linmod( fp, "dotdashed");
			else
				pl_linmod( fp, "solid");
			Dashing = sp->s_soldash;
		}
		for( RT_LIST_FOR( vp, rt_vlist, &(sp->s_vlist) ) )  {
			register int	i;
			register int	nused = vp->nused;
			register int	*cmd = vp->cmd;
			register point_t *pt = vp->pt;
			for( i = 0; i < nused; i++,cmd++,pt++ )  {
				switch( *cmd )  {
				case RT_VLIST_POLY_START:
				case RT_VLIST_POLY_VERTNORM:
					continue;
				case RT_VLIST_POLY_MOVE:
				case RT_VLIST_LINE_MOVE:
					/* Move, not draw */
					MAT4X3PNT( last, model2view, *pt );
					continue;
				case RT_VLIST_POLY_DRAW:
				case RT_VLIST_POLY_END:
				case RT_VLIST_LINE_DRAW:
					/* draw */
					MAT4X3PNT( fin, model2view, *pt );
					VMOVE( start, last );
					VMOVE( last, fin );
					break;
				}
				if(
					vclip( start, fin, clipmin, clipmax ) == 0
				)  continue;

				if( Three_D )  {
					/* Could check for differences from last color */
					pl_color( fp,
						sp->s_color[0],
						sp->s_color[1],
						sp->s_color[2] );
					pl_3line( fp,
						(int)( start[X] * 2047 ),
						(int)( start[Y] * 2047 ),
						(int)( start[Z] * 2047 ),
						(int)( fin[X] * 2047 ),
						(int)( fin[Y] * 2047 ),
						(int)( fin[Z] * 2047 ) );
				}  else  {
					pl_line( fp,
						(int)( start[0] * 2047 ),
						(int)( start[1] * 2047 ),
						(int)( fin[0] * 2047 ),
						(int)( fin[1] * 2047 ) );
				}
			}
		}
	}
out:
	if( is_pipe )
		(void)pclose( fp );
	else
		(void)fclose( fp );

	return TCL_ERROR;
}

int
f_area(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int	argc;
char	**argv;
{
	register struct solid		*sp;
	register struct rt_vlist	*vp;
	static vect_t last;
	static vect_t fin;
	char buf[128];
	FILE *fp;

	if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
	  return TCL_ERROR;

	if( not_state( ST_VIEW, "Presented Area Calculation" ) == TCL_ERROR )
		return TCL_ERROR;

	FOR_ALL_SOLIDS( sp )  {
	  if( !sp->s_Eflag && sp->s_soldash != 0 )  {
	    Tcl_AppendResult(interp, "everything in view must be 'E'ed\n", (char *)NULL);
	    return TCL_ERROR;
	  }
	}

	/* Create pipes to cad_boundp | cad_parea */
	if( argc == 2 )  {
	  sprintf( buf, "cad_boundp -t %s | cad_parea", argv[1] );
	  Tcl_AppendResult(interp, "Tolerance is ", argv[1], "\n", (char *)NULL);
	}  else  {
	  struct rt_vls tmp_vls;
	  double tol = VIEWSIZE * 0.001;

	  rt_vls_init(&tmp_vls);
	  sprintf( buf, "cad_boundp -t %e | cad_parea", tol );
	  rt_vls_printf(&tmp_vls, "Auto-tolerance of 0.1%% is %e\n", tol);
	  Tcl_AppendResult(interp, rt_vls_addr(&tmp_vls), (char *)NULL);
	  rt_vls_free(&tmp_vls);
	}

	if( (fp = popen( buf, "w" )) == NULL )  {
	  perror( buf );
	  return TCL_ERROR;
	}

	/*
	 * Write out rotated but unclipped, untranslated,
	 * and unscaled vectors
	 */
	FOR_ALL_SOLIDS( sp )  {
		for( RT_LIST_FOR( vp, rt_vlist, &(sp->s_vlist) ) )  {
			register int	i;
			register int	nused = vp->nused;
			register int	*cmd = vp->cmd;
			register point_t *pt = vp->pt;
			for( i = 0; i < nused; i++,cmd++,pt++ )  {
				switch( *cmd )  {
				case RT_VLIST_POLY_START:
				case RT_VLIST_POLY_VERTNORM:
					continue;
				case RT_VLIST_POLY_MOVE:
				case RT_VLIST_LINE_MOVE:
					/* Move, not draw */
					MAT4X3VEC( last, Viewrot, *pt );
					continue;
				case RT_VLIST_POLY_DRAW:
				case RT_VLIST_POLY_END:
				case RT_VLIST_LINE_DRAW:
					/* draw.  */
					MAT4X3VEC( fin, Viewrot, *pt );
					break;
				}

				fprintf(fp, "%.9e %.9e %.9e %.9e\n",
					last[X] * base2local,
					last[Y] * base2local,
					fin[X] * base2local,
					fin[Y] * base2local );

				VMOVE( last, fin );
			}
		}
	}

	Tcl_AppendResult(interp, "Presented area from this viewpoint, square ",
			 rt_units_string(dbip->dbi_local2base), ":\n", (char *)NULL);
	pclose( fp );

	return TCL_OK;
}
