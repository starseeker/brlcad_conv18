/*
 *			T I T L E S . C
 *
 * Functions -
 *	dotitles	Output GED "faceplate" & titles.
 *
 *  Author -
 *	Michael John Muuss
 *	Kieth A. Applin
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

#include "tcl.h"

#include "machine.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "externs.h"
#include "./ged.h"
#include "./titles.h"
#include "./solid.h"
#include "./sedit.h"
#include "./mgedtcl.h"
#include "./dm.h"

int	state;
char	*state_str[] = {
	"-ZOT-",
	"VIEWING",
	"SOL PICK",
	"SOL EDIT",
	"OBJ PICK",
	"OBJ PATH",
	"OBJ EDIT",
	"VERTPICK",
	"UNKNOWN",
};

#ifdef XMGED
void    (*dotitles_hook)();
#endif

extern double			frametime;	/* from ged.c */
extern struct rt_db_internal	es_int;


/*
 *			C R E A T E _ T E X T _ O V E R L A Y
 *
 *  Prepare the numerical display of the currently edited solid/object.
 */
void
create_text_overlay( vp )
register struct rt_vls	*vp;
{
	struct directory	*dp;
	register int	i;

	RT_VLS_CHECK(vp);

	/*
	 * Set up for character output.  For the best generality, we
	 * don't assume that the display can process a CRLF sequence,
	 * so each line is written with a separate call to dmp->dmr_puts().
	 */

	/* print solid info at top of screen */
	if( es_edflag >= 0 ) {
		dp = illump->s_path[illump->s_last];

		rt_vls_strcat( vp, "** SOLID -- " );
		rt_vls_strcat( vp, dp->d_namep );
		rt_vls_strcat( vp, ": ");

		vls_solid( vp, &es_int, rt_identity );

		if(illump->s_last) {
			rt_vls_strcat( vp, "\n** PATH --  ");
			for(i=0; i <= illump->s_last; i++) {
				rt_vls_strcat( vp, "/" );
				rt_vls_strcat( vp, illump->s_path[i]->d_namep);
			}
			rt_vls_strcat( vp, ": " );

			/* print the evaluated (path) solid parameters */
			vls_solid( vp, &es_int, es_mat );
		}
	}

	/* display path info for object editing also */
	if( state == ST_O_EDIT ) {
		rt_vls_strcat( vp, "** PATH --  ");
		for(i=0; i <= illump->s_last; i++) {
			rt_vls_strcat( vp, "/" );
			rt_vls_strcat( vp, illump->s_path[i]->d_namep);
		}
		rt_vls_strcat( vp, ": " );

		/* print the evaluated (path) solid parameters */
		if( illump->s_Eflag == 0 ) {
			mat_t	new_mat;
			/* NOT an evaluated region */
			/* object edit option selected */
			mat_mul(new_mat, modelchanges, es_mat);

			vls_solid( vp, &es_int, new_mat );
		}
	}
}

/*
 *			S C R E E N _ V L S
 *
 *  Output a vls string to the display manager,
 *  as a text overlay on the graphics area (ugh).
 *
 * Set up for character output.  For the best generality, we
 * don't assume that the display can process a CRLF sequence,
 * so each line is written with a separate call to dmp->dmr_puts().
 */
void
screen_vls( xbase, ybase, vp )
int	xbase;
int	ybase;
register struct rt_vls	*vp;
{
	register char	*start;
	register char	*end;
	register int	y;

	RT_VLS_CHECK( vp );
	y = ybase;

	start = rt_vls_addr( vp );
	while( *start != '\0' )  {
		if( (end = strchr( start, '\n' )) == NULL )  return;

		*end = '\0';
		dmp->dmr_puts( start, xbase, y, 0, DM_YELLOW );
		start = end+1;
		y += TEXT0_DY;
	}
}

/*
 *			D O T I T L E S
 *
 * Produce titles, etc, on the screen.
 * NOTE that this routine depends on being called AFTER dozoom();
 */
void
dotitles(call_dm)
int call_dm;
{
	register int    i;
	register int    y;			/* for menu computations */
	static vect_t   work, work1;		/* work vector */
	static vect_t   temp, temp1;
	register int    yloc, xloc;
	auto fastf_t	az, el, tw;
	int		scroll_ybot;
	struct rt_vls   vls;
	typedef char    c_buf[80];
	auto c_buf      cent_x, cent_y, cent_z, size, azimuth, elevation,
	                twist, ang_x, ang_y, ang_z;

	rt_vls_init(&vls);
	
	/* Set the Tcl variables to the appropriate values. */

	Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "state", state_str[state],
		    TCL_GLOBAL_ONLY);
	if (illump != SOLID_NULL) {
	    struct rt_vls path_lhs, path_rhs;

	    rt_vls_init(&path_lhs);
	    rt_vls_init(&path_rhs);
	    for (i = 0; i < ipathpos; i++)
		rt_vls_printf(&path_lhs, "/%s", illump->s_path[i]->d_namep);
	    for (; i <= illump->s_last; i++)
		rt_vls_printf(&path_rhs, "/%s", illump->s_path[i]->d_namep);
	    Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "path_lhs",
			rt_vls_addr(&path_lhs), TCL_GLOBAL_ONLY);
	    Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "path_rhs",
			rt_vls_addr(&path_rhs), TCL_GLOBAL_ONLY);
	    rt_vls_free(&path_rhs);
	    rt_vls_free(&path_lhs);
	} else {
	    Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "path_lhs", "",
			TCL_GLOBAL_ONLY);
	    Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "path_rhs", "",
			TCL_GLOBAL_ONLY);
	}

	sprintf(cent_x, "%.3f", -toViewcenter[MDX]*base2local);
	sprintf(cent_y, "%.3f", -toViewcenter[MDY]*base2local);
	sprintf(cent_z, "%.3f", -toViewcenter[MDZ]*base2local);

	rt_vls_printf(&vls, "%s %s %s", cent_x, cent_y, cent_z);
	Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "center", rt_vls_addr(&vls),
		    TCL_GLOBAL_ONLY);
	rt_vls_trunc(&vls, 0);

	sprintf(size, "%.3f", VIEWSIZE*base2local);
	Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "size", size, TCL_GLOBAL_ONLY);
	rt_vls_trunc(&vls, 0);

	Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "units",
		    (char *)rt_units_string(dbip->dbi_local2base),
		    TCL_GLOBAL_ONLY);

	/* Find current azimuth, elevation, and twist angles */
	VSET( work , 0 , 0 , 1 );	/* view z-direction */
	MAT4X3VEC( temp , view2model , work );
	VSET( work1 , 1 , 0 , 0 );	/* view x-direction */
	MAT4X3VEC( temp1 , view2model , work1 );

	/* calculate angles using accuracy of 0.005, since display
	 * shows 2 digits right of decimal point */
	mat_aet_vec( &az , &el , &tw , temp , temp1 , (fastf_t)0.005 );

	sprintf(azimuth,   "%3.2f", az);
	sprintf(elevation, "%2.2f", el);
	sprintf(twist,     "%3.2f", tw);
	
	Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "azimuth", azimuth,
		    TCL_GLOBAL_ONLY);
	Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "elevation", elevation,
		    TCL_GLOBAL_ONLY);
	Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "twist", twist, TCL_GLOBAL_ONLY);

	sprintf(ang_x, "%.2f", rate_rotate[X]);
	sprintf(ang_y, "%.2f", rate_rotate[Y]);
	sprintf(ang_z, "%.2f", rate_rotate[Z]);

	rt_vls_printf(&vls, "%s %s %s", ang_x, ang_y, ang_z);
	Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "ang", rt_vls_addr(&vls),
		    TCL_GLOBAL_ONLY);
	rt_vls_trunc(&vls, 0);

	Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "adc", "", TCL_GLOBAL_ONLY);
	Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "keypoint", "", TCL_GLOBAL_ONLY);
	Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "fps", "", TCL_GLOBAL_ONLY);

	if (!call_dm) {
	    rt_vls_free(&vls);
	    return;
	}
	
#ifdef XMGED
	if(dotitles_hook){
	    (*dotitles_hook)();
	    rt_vls_free(&vls);
	    return;
	}
#endif
	/* Enclose window in decorative box.  Mostly for alignment. */
	dmp->dmr_2d_line( XMIN, YMIN, XMAX, YMIN, 0 );
	dmp->dmr_2d_line( XMAX, YMIN, XMAX, YMAX, 0 );
	dmp->dmr_2d_line( XMAX, YMAX, XMIN, YMAX, 0 );
	dmp->dmr_2d_line( XMIN, YMAX, XMIN, YMIN, 0 );

	/* Line across the bottom, above two bottom status lines */
	dmp->dmr_2d_line( XMIN, TITLE_YBASE-TEXT1_DY, XMAX,
			  TITLE_YBASE-TEXT1_DY, 0 );

	/* Display scroll bars */
	scroll_ybot = scroll_display( SCROLLY ); 
	y = MENUY;

	/* Display state and local unit in upper right corner, boxed */

	dmp->dmr_puts(state_str[state], MENUX, MENUY - MENU_DY, 1, DM_WHITE );
#define YPOS	(MENUY - MENU_DY - 75 )
	dmp->dmr_2d_line(MENUXLIM, YPOS, MENUXLIM, YMAX, 0);	/* vert. */
#undef YPOS

	/*
	 * Print information about object illuminated
	 */
	if( illump != SOLID_NULL &&
	    (state==ST_O_PATH || state==ST_O_PICK || state==ST_S_PICK) )  {
		for( i=0; i <= illump->s_last; i++ )  {
			if( i == ipathpos  &&  state == ST_O_PATH )  {
				dmp->dmr_puts( "[MATRIX]", MENUX, y, 0,
					       DM_WHITE );
				y += MENU_DY;
			}
			dmp->dmr_puts( illump->s_path[i]->d_namep, MENUX, y, 0,
				       DM_YELLOW );
			y += MENU_DY;
		}
	}

	/*
	 * The top of the menu (if any) begins at the Y value specified.
	 */
	mmenu_display( y );

	/* print parameter locations on screen */
	if( state == ST_O_EDIT && illump->s_Eflag ) {
		/* region is a processed region */
		MAT4X3PNT(temp, model2objview, es_keypoint);
		xloc = (int)(temp[X]*2048);
		yloc = (int)(temp[Y]*2048);
		dmp->dmr_2d_line(xloc-TEXT0_DY, yloc+TEXT0_DY, xloc+TEXT0_DY,
				 yloc-TEXT0_DY, 0);
		dmp->dmr_2d_line(xloc-TEXT0_DY, yloc-TEXT0_DY, xloc+TEXT0_DY,
				 yloc+TEXT0_DY, 0);
		dmp->dmr_2d_line(xloc+TEXT0_DY, yloc+TEXT0_DY, xloc-TEXT0_DY,
				 yloc+TEXT0_DY, 0);
		dmp->dmr_2d_line(xloc+TEXT0_DY, yloc-TEXT0_DY, xloc-TEXT0_DY,
				 yloc-TEXT0_DY, 0);
		dmp->dmr_2d_line(xloc+TEXT0_DY, yloc+TEXT0_DY, xloc+TEXT0_DY,
				 yloc-TEXT0_DY, 0);
		dmp->dmr_2d_line(xloc-TEXT0_DY, yloc+TEXT0_DY, xloc-TEXT0_DY,
				 yloc-TEXT0_DY, 0);
	}

	/* Label the vertices of the edited solid */
	if(es_edflag >= 0 || (state == ST_O_EDIT && illump->s_Eflag == 0))  {
		mat_t			xform;
		struct rt_point_labels	pl[8+1];

		mat_mul( xform, model2objview, es_mat );

		label_edited_solid( pl, 8+1, xform, &es_int );

		for( i=0; i<8+1; i++ )  {
			if( pl[i].str[0] == '\0' )  break;
			dmp->dmr_puts( pl[i].str,
				((int)(pl[i].pt[X]*2048))+15,
				((int)(pl[i].pt[Y]*2048))+15,
				0, DM_WHITE );
		}
	}

	/*
	 * Prepare the numerical display of the currently edited solid/object.
	 */
	create_text_overlay( &vls );
	screen_vls( SOLID_XBASE, scroll_ybot+TEXT0_DY, &vls );

	/*
	 * General status information on the next to last line
	 */
	rt_vls_printf(&vls,
		      " cent=(%s, %s, %s), sz=%s %s, ", cent_x, cent_y, cent_z,
		      size, rt_units_string(dbip->dbi_local2base));

	rt_vls_printf(&vls,
		       "az=%s el=%s tw=%s ang=(%s, %s, %s)",
		      azimuth, elevation, twist, ang_x, ang_y, ang_z);
	dmp->dmr_puts( rt_vls_addr(&vls), TITLE_XBASE, TITLE_YBASE, 1,
		       DM_WHITE );
	rt_vls_trunc(&vls, 0);
	/*
	 * Second status line
	 */

	/* Priorities for what to display:
	 *	1.  adc info
	 *	2.  keypoint
	 *	3.  illuminated path
	 *
	 * This way the adc info will be displayed during editing
	 */

	if( adcflag ) {
		/* Angle/Distance cursor */
		point_t	pt1, pt2, pt3;
		point_t	center_model;

		VSET(pt1, 
		    (curs_x / 2047.0) *Viewscale,
		    (curs_y / 2047.0) *Viewscale, 0.0);
		VSET(center_model, 
		    -toViewcenter[MDX], -toViewcenter[MDY],
		    -toViewcenter[MDZ]);
		MAT4X3VEC(pt2, Viewrot, center_model);
		VADD2(pt3, pt1, pt2);
		rt_vls_printf( &vls,
" curs:  a1=%.1f,  a2=%.1f,  dst=%.3f,  cent=(%.3f, %.3f),  delta=(%.3f, %.3f)",
			angle1 * radtodeg, angle2 * radtodeg,
			(c_tdist / 2047.0) *Viewscale*base2local,
			pt3[X]*base2local, pt3[Y]*base2local,
			(curs_x / 2047.0) *Viewscale*base2local,
			(curs_y / 2047.0) *Viewscale*base2local );
		dmp->dmr_puts( rt_vls_addr(&vls), TITLE_XBASE,
			      TITLE_YBASE + TEXT1_DY, 1, DM_YELLOW );
		Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "adc",
			    rt_vls_addr(&vls), TCL_GLOBAL_ONLY);
	} else if( state == ST_S_EDIT || state == ST_O_EDIT )  {
	        rt_vls_printf( &vls,
			" Keypoint: %s %s: (%g, %g, %g)",
			rt_functab[es_int.idb_type].ft_name+3,	/* Skip ID_ */
			es_keytag,
			es_keypoint[X] * base2local,
			es_keypoint[Y] * base2local,
			es_keypoint[Z] * base2local);
		dmp->dmr_puts( rt_vls_addr(&vls), TITLE_XBASE,
			       TITLE_YBASE + TEXT1_DY, 1, DM_YELLOW );
		Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "keypoint",
			    rt_vls_addr(&vls), TCL_GLOBAL_ONLY);
	} else if( illump != SOLID_NULL )  {
		/* Illuminated path */
	        rt_vls_strcat(&vls, " Path: ");
		for( i=0; i <= illump->s_last; i++ )  {
			if( i == ipathpos  &&
			    (state == ST_O_PATH || state == ST_O_EDIT) )
				rt_vls_strcat( &vls, "/__MATRIX__" );
			rt_vls_printf(&vls, "/%s", illump->s_path[i]->d_namep);
		}
		dmp->dmr_puts( rt_vls_addr(&vls), TITLE_XBASE,
			       TITLE_YBASE + TEXT1_DY, 1, DM_YELLOW );
	} else {
		rt_vls_printf(&vls, "%.2f fps", 1/frametime );
		dmp->dmr_puts( rt_vls_addr(&vls), TITLE_XBASE,
			       TITLE_YBASE + TEXT1_DY, 1, DM_YELLOW );
		Tcl_SetVar2(interp, MGED_DISPLAY_VAR, "fps",
			    rt_vls_addr(&vls), TCL_GLOBAL_ONLY);
	}

	rt_vls_free(&vls);
}
