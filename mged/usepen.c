/*
 *			U S E P E N . C
 *
 * Functions -
 *	usepen		Use x,y data from data tablet
 *	buildHrot	Generate rotation matrix
 *	wrt_view	Modify xform matrix with respect to current view
 *
 *  Author -
 *	Michael John Muuss
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

#include <stdio.h>
#include <math.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "externs.h"
#include "./ged.h"
#include "./titles.h"
#include "./solid.h"
#include "./menu.h"
#include "./dm.h"

#ifdef XMGED
#include <errno.h>
#include "./sedit.h"

extern int update_views;
extern int using_dmX;
extern void (*tran_hook)();
extern void (*set_tran_hook)();
extern double tran_x;
extern double tran_y;
extern double tran_z;

int ignore_scroll_and_menu = 0;

#endif
int f_aip();

/*	Degree <-> Radian conversion factors	*/
double	degtorad =  0.01745329251994329573;
double	radtodeg = 57.29577951308232098299;

struct solid	*illump;	/* == 0 if none, else points to ill. solid */
int		ipathpos;	/* path index of illuminated element */
				/* set by e9.c, cleared here */
void		wrt_view(), wrt_point();
static void	illuminate();

/*
 *			F _ M O U S E
 *
 * X and Y are expected to be in -2048 <= x,y <= +2047 range.
 * The "up" flag is 1 on the not-pressed to pressed transition,
 * and 0 on the pressed to not-pressed transition.
 *
 * Note -
 *  The mouse is the focus of much of the editing activity in GED.
 *  The editor operates in one of seven basic editing states, recorded
 *  in the variable called "state".  When no editing is taking place,
 *  the editor is in state ST_VIEW.  There are two paths out of ST_VIEW:
 *  
 *  BE_S_ILLUMINATE, when pressed, takes the editor into ST_S_PICK,
 *  where the mouse is used to pick a solid to edit, using our
 *  unusual "illuminate" technique.  Moving the mouse varies the solid
 *  being illuminated.  When the mouse is pressed, the editor moves into
 *  state ST_S_EDIT, and solid editing may begin.  Solid editing is
 *  terminated via BE_ACCEPT and BE_REJECT.
 *  
 *  BE_O_ILLUMINATE, when pressed, takes the editor into ST_O_PICK,
 *  again performing the illuminate procedure.  When the mouse is pressed,
 *  the editor moves into state ST_O_PATH.  Now, moving the mouse allows
 *  the user to choose the portion of the path relation to be edited.
 *  When the mouse is pressed, the editor moves into state ST_O_EDIT,
 *  and object editing may begin.  Object editing is terminated via
 *  BE_ACCEPT and BE_REJECT.
 *  
 *  The only way to exit the intermediate states (non-VIEW, non-EDIT)
 *  is by completing the sequence, or pressing BE_REJECT.
 */
int
f_mouse( argc, argv )
int	argc;
char	**argv;
{
	vect_t	mousevec;		/* float pt -1..+1 mouse pos vect */
	int	isave;
	int	up = atoi(argv[1]);
	int	xpos = atoi(argv[2]);
	int	ypos = atoi(argv[3]);

	/* Build floating point mouse vector, -1 to +1 */
	mousevec[X] =  xpos / 2047.0;
	mousevec[Y] =  ypos / 2047.0;
	mousevec[Z] = 0;

	if (mged_variables.faceplate) {

#ifdef XMGED
	if(!ignore_scroll_and_menu){
#endif
	/*
	 * If mouse press is in scroll area, see if scrolling, and if so,
	 * divert this mouse press.
	 */
	if( (xpos >= MENUXLIM) && up )  {
		register int i;

		if( (i = scroll_select(xpos, ypos )) < 0 )  {
			rt_log("mouse press outside valid scroll area\n");
			return CMD_BAD;
		} 
		if( i > 0 )  {
			/* Scroller bars claimed button press */
			return CMD_OK;
		}
		/* Otherwise, fall through */
	}

	/*
	 * If menu is active, and mouse press is in menu area,
	 * divert this mouse press for menu purposes.
	 */
	if( xpos < MENUXLIM && up )  {
		register int i;
		if( (i = mmenu_select( ypos )) < 0 )  {
			rt_log("mouse press outside valid menu\n");
			return CMD_BAD;
		}
		if( i > 0 )  {
			/* Menu claimed button press */
			return CMD_OK;
		}
		/* Otherwise, fall through */
	}

#ifdef XMGED
      }
#endif


    }
	/*
	 *  In the best of all possible worlds, nothing should happen
	 *  when the mouse is not pressed;  this would relax the requirement
	 *  for the host being informed when the mouse changes position.
	 *  However, for now, illuminate mode makes this impossible.
	 */
	if( up == 0 )  switch( state )  {

	case ST_VIEW:
	case ST_S_EDIT:
	case ST_O_EDIT:
	default:
		return CMD_OK;		/* Take no action in these states */

	case ST_O_PICK:
	case ST_S_PICK:
		/*
		 * Use the mouse for illuminating a solid
		 */
		illuminate( ypos );
		return CMD_OK;

	case ST_O_PATH:
		/*
		 * Convert DT position to path element select
		 */
		isave = ipathpos;
		ipathpos = illump->s_last - (
			(ypos+2048L) * (illump->s_last+1) / 4096);
		if( ipathpos != isave )
			dmaflag++;
#ifdef XMGED
		else
		  dmaflag = 0;
#endif
		return CMD_OK;

	} else switch( state )  {

	case ST_VIEW:
		/*
		 * Use the DT for moving view center.
		 * Make indicated point be new view center (NEW).
		 */
		slewview( mousevec );
		return CMD_OK;

	case ST_O_PICK:
		ipathpos = 0;
		(void)chg_state( ST_O_PICK, ST_O_PATH, "mouse press");
		dmaflag = 1;
		return CMD_OK;

	case ST_S_PICK:
		/* Check details, Init menu, set state */
		init_sedit();		/* does chg_state */
		dmaflag = 1;
		return CMD_OK;

	case ST_S_EDIT:
		sedit_mouse( mousevec );
#ifdef XMGED
		if(es_edflag >= STRANS && es_edflag <= PTARB && tran_hook){
		  point_t new_pos;
		  point_t old_pos;
		  point_t diff;

		  MAT4X3PNT( new_pos, view2model, mousevec);
		  set_tran(new_pos[X], new_pos[Y], new_pos[Z]);
/*		  (*tran_hook)();*/
		}
#endif
		return CMD_OK;

	case ST_O_PATH:
		/*
		 * Set combination "illuminate" mode.  This code
		 * assumes that the user has already illuminated
		 * a single solid, and wishes to move a collection of
		 * objects of which the illuminated solid is a part.
		 * The whole combination will not illuminate (to save
		 * vector drawing time), but all the objects should
		 * move/scale in unison.
		 */
		{
			char	*av[3];
			char	num[8];
			(void)sprintf(num, "%d", ipathpos);
			av[0] = "matpick";
			av[1] = num;
			av[2] = (char *)NULL;
			(void)f_matpick( 2, av );
			/* How to record this in the journal file? */
			return CMD_OK;
		}

	case ST_S_VPICK:
		sedit_vpick( mousevec );
		return CMD_OK;

	case ST_O_EDIT:
		objedit_mouse( mousevec );
#ifdef XMGED
		if(edobj == BE_O_X || edobj == BE_O_Y || edobj == BE_O_XY){
		  point_t new_pos;

		  MAT4X3PNT( new_pos, view2model, mousevec);
		  set_tran(new_pos[X], new_pos[Y], new_pos[Z]);
/*		  (*tran_hook)();*/
		}
#endif
		return CMD_OK;

	default:
		state_err( "mouse press" );
		return CMD_BAD;
	}
	/* NOTREACHED */
}

/*
 *			I L L U M I N A T E
 *
 *  All solids except for the illuminated one have s_iflag set to DOWN.
 *  The illuminated one has s_iflag set to UP, and also has the global
 *  variable "illump" pointing at it.
 */
static void
illuminate( y )  {
	register int count;
	register struct solid *sp;
	register struct solid *saveillump;

	saveillump = illump;

	/*
	 * Divide the mouse into 'ndrawn' VERTICAL zones, and use the
	 * zone number as a sequential position among solids
	 * which are drawn.
	 */
	count = ( (fastf_t) y + 2048.0 ) * ndrawn / 4096.0;

	FOR_ALL_SOLIDS( sp )  {
		/* Only consider solids which are presently in view */
		if( sp->s_flag == UP )  {
			if( count-- == 0 && illump != sp )  {
				sp->s_iflag = UP;
				dmp->dmr_viewchange( DM_CHGV_ILLUM, sp );
				illump = sp;
			}  else  {
				/* All other solids have s_iflag set DOWN */
				sp->s_iflag = DOWN;
			}
		}
	}
#ifdef Xmged
	if( saveillump != illump ){
	  update_views = 1;
	  dmaflag++;
	}else
		--dmaflag;	/* illiminates unnecessary redrawing */
#else
	if( saveillump != illump )
		dmaflag++;
#endif
}

/*
 *                        A I L L
 *
 *   advance illump or ipathpos
 */
int
f_aip(argc, argv)
int argc;
char *argv[];
{
  register struct solid *sp;
  static count = -1;
  int i;

  if(!ndrawn || (state != ST_S_PICK && state != ST_O_PICK  && state != ST_O_PATH))
    return CMD_BAD;

  if(state == ST_O_PATH){
    if(argc == 1 || *argv[1] == 'f'){
      ++ipathpos;
      if(ipathpos > illump->s_last)
	ipathpos = 0;
    }else if(*argv[1] == 'b'){
      --ipathpos;
      if(ipathpos < 0)
	ipathpos = illump->s_last;
    }else{
      rt_log("aill: bad parameter - %s\n", argv[1]);
      return CMD_BAD;
    }
  }else{
    sp = illump;
    sp->s_iflag = DOWN;
    if(argc == 1 || *argv[1] == 'f'){
      if(sp->s_forw == &HeadSolid)
	sp = HeadSolid.s_forw;
      else
	sp = sp->s_forw;
    }else if(*argv[1] == 'b'){
      if(sp->s_back == &HeadSolid)
	sp = HeadSolid.s_back;
      else
	sp = sp->s_back;
    }else{
      rt_log("aill: bad parameter - %s\n", argv[1]);
      return CMD_BAD;
    }

    sp->s_iflag = UP;
    dmp->dmr_viewchange( DM_CHGV_ILLUM, sp );
    illump = sp;
  }

#if 0
  update_views = 1;
#endif
  dmaflag = 1;
  return CMD_OK;
}

/*
 *			B U I L D H R O T
 *
 * This routine builds a Homogeneous rotation matrix, given
 * alpha, beta, and gamma as angles of rotation.
 *
 * NOTE:  Only initialize the rotation 3x3 parts of the 4x4
 * There is important information in dx,dy,dz,s .
 */
void
buildHrot( mat, alpha, beta, ggamma )
register matp_t mat;
double alpha, beta, ggamma;
{
	static fastf_t calpha, cbeta, cgamma;
	static fastf_t salpha, sbeta, sgamma;

	calpha = cos( alpha );
	cbeta = cos( beta );
	cgamma = cos( ggamma );

	salpha = sin( alpha );
	sbeta = sin( beta );
	sgamma = sin( ggamma );

	/*
	 * compute the new rotation to apply to the previous
	 * viewing rotation.
	 * Alpha is angle of rotation about the X axis, and is done third.
	 * Beta is angle of rotation about the Y axis, and is done second.
	 * Gamma is angle of rotation about Z axis, and is done first.
	 */
#ifdef m_RZ_RY_RX
	/* view = model * RZ * RY * RX (Neuman+Sproul, premultiply) */
	mat[0] = cbeta * cgamma;
	mat[1] = -cbeta * sgamma;
	mat[2] = -sbeta;

	mat[4] = -salpha * sbeta * cgamma + calpha * sgamma;
	mat[5] = salpha * sbeta * sgamma + calpha * cgamma;
	mat[6] = -salpha * cbeta;

	mat[8] = calpha * sbeta * cgamma + salpha * sgamma;
	mat[9] = -calpha * sbeta * sgamma + salpha * cgamma;
	mat[10] = calpha * cbeta;
#endif
	/* This is the correct form for this version of GED */
	/* view = RX * RY * RZ * model (Rodgers, postmultiply) */
	/* Point thumb along axis of rotation.  +Angle as hand closes */
	mat[0] = cbeta * cgamma;
	mat[1] = -cbeta * sgamma;
	mat[2] = sbeta;

	mat[4] = salpha * sbeta * cgamma + calpha * sgamma;
	mat[5] = -salpha * sbeta * sgamma + calpha * cgamma;
	mat[6] = -salpha * cbeta;

	mat[8] = -calpha * sbeta * cgamma + salpha * sgamma;
	mat[9] = calpha * sbeta * sgamma + salpha * cgamma;
	mat[10] = calpha * cbeta;
}


/*
 *  			W R T _ V I E W
 *  
 *  Given a model-space transformation matrix "change",
 *  return a matrix which applies the change with-respect-to
 *  the view center.
 */
void
wrt_view( out, change, in )
register matp_t out, change, in;
{
	static mat_t t1, t2;

	mat_mul( t1, toViewcenter, in );
	mat_mul( t2, change, t1 );

	/* Build "fromViewcenter" matrix */
	mat_idn( t1 );
	MAT_DELTAS( t1, -toViewcenter[MDX], -toViewcenter[MDY], -toViewcenter[MDZ] );
	mat_mul( out, t1, t2 );
}

/*
 *  			W R T _ P O I N T
 *  
 *  Given a model-space transformation matrix "change",
 *  return a matrix which applies the change with-respect-to
 *  "point".
 */
void
wrt_point( out, change, in, point )
register matp_t out, change, in;
register vect_t point;
{
	static mat_t t1, t2, pt_to_origin, origin_to_pt;

	/* build "point to origin" matrix */
	mat_idn( pt_to_origin );
	MAT_DELTAS(pt_to_origin, -point[X], -point[Y], -point[Z]);

	/* build "origin to point" matrix */
	mat_idn( origin_to_pt );
	MAT_DELTAS(origin_to_pt, point[X], point[Y], point[Z]);

	/* t1 = pt_to_origin * in */
	mat_mul( t1, pt_to_origin, in );

	/* apply change matrix: t2 = change * pt_to_origin * in */
	mat_mul( t2, change, t1 );

	/* apply origin_to_pt matrix:
	 *	out = origin_to_pt * change * pt_to_origin * in
	 */
	mat_mul( out, origin_to_pt, t2 );
}

/*
 *  			W R T _ P O I N T _ D I R E C
 *  
 *  Given a model-space transformation matrix "change",
 *  return a matrix which applies the change with-respect-to
 *  given "point" and "direc".
 */
void
wrt_point_direc( out, change, in, point, direc )
register matp_t out, change, in;
register vect_t point, direc;
{
	static mat_t	t1, t2;
	static mat_t	pt_to_origin, origin_to_pt;
	static mat_t	d_to_zaxis, zaxis_to_d;
	static vect_t	zaxis;

	/* build "point to origin" matrix */
	mat_idn( pt_to_origin );
	MAT_DELTAS(pt_to_origin, -point[X], -point[Y], -point[Z]);

	/* build "origin to point" matrix */
	mat_idn( origin_to_pt );
	MAT_DELTAS(origin_to_pt, point[X], point[Y], point[Z]);

	/* build "direc to zaxis" matrix */
	VSET(zaxis, 0, 0, 1);
	mat_fromto(d_to_zaxis, direc, zaxis);

	/* build "zaxis to direc" matrix */
	mat_inv(zaxis_to_d, d_to_zaxis);

	/* t1 = pt_to_origin * in */
	mat_mul( t1, pt_to_origin, in );

	/* t2 = d_to_zaxis * pt_to_origin * in */
	mat_mul( t2, d_to_zaxis, t1 );

	/* apply change matrix...
	 *	t1 = change * d_to_zaxis * pt_to_origin * in
	 */
	mat_mul( t1, change, t2 );

	/* apply zaxis_to_d matrix:
	 *	t2 = zaxis_to_d * change * d_to_zaxis * pt_to_origin * in
	 */
	mat_mul( t2, zaxis_to_d, t1 );

	/* apply origin_to_pt matrix:
	 *	out = origin_to_pt * zaxis_to_d * change *
	 *		d_to_zaxis * pt_to_origin * in
	 */
	mat_mul( out, origin_to_pt, t2 );
}

/*
 *			F _ M A T P I C K
 *
 *  When in O_PATH state, select the arc which contains the matrix
 *  which is going to be "object edited".
 *  The choice is recorded in variable "ipathpos".
 *
 *  There are two syntaxes:
 *	matpick a/b	Pick arc between a and b.
 *	matpick #	Similar to internal interface.
 *			0 = top level object is a solid.
 *			n = edit arc from s_path[n-1] to [n]
 */
int
f_matpick( argc, argv )
int	argc;
char	**argv;
{
	register struct solid	*sp;
	char			*cp;
	register int		j;

	if( not_state( ST_O_PATH, "Object Edit matrix pick" ) )  return CMD_BAD;

	if( cp = strchr( argv[1], '/' ) )  {
		struct directory	*d0, *d1;
		if( (d1 = db_lookup( dbip, cp+1, LOOKUP_NOISY )) == DIR_NULL )
			return CMD_BAD;
		*cp = '\0';		/* modifies argv[1] */
		if( (d0 = db_lookup( dbip, argv[1], LOOKUP_NOISY )) == DIR_NULL )
			return CMD_BAD;
		/* Find arc on illump path which runs from d0 to d1 */
		for( j=1; j <= illump->s_last; j++ )  {
			if( illump->s_path[j-1] != d0 )  continue;
			if( illump->s_path[j-0] != d1 )  continue;
			ipathpos = j;
			goto got;
		}
		rt_log("matpick: unable to find arc %s/%s in current selection.  Re-specify.\n",
			d0->d_namep, d1->d_namep );
		return CMD_BAD;
	} else {
		ipathpos = atoi(argv[1]);
		if( ipathpos < 0 )  ipathpos = 0;
		else if( ipathpos > illump->s_last )  ipathpos = illump->s_last;
	}
got:
	dmp->dmr_light( LIGHT_ON, BE_ACCEPT );
	dmp->dmr_light( LIGHT_ON, BE_REJECT );
	dmp->dmr_light( LIGHT_OFF, BE_O_ILLUMINATE );

	/* Include all solids with same tree top */
	FOR_ALL_SOLIDS( sp )  {
		for( j = 0; j <= ipathpos; j++ )  {
			if( sp->s_path[j] != illump->s_path[j] )
				break;
		}
		/* Only accept if top of tree is identical */
		if( j == ipathpos+1 )
			sp->s_iflag = UP;
	}
	(void)chg_state( ST_O_PATH, ST_O_EDIT, "mouse press" );
	chg_l2menu(ST_O_EDIT);

	/* begin object editing - initialize */
	init_objedit();

	dmaflag++;
	return CMD_OK;
}
