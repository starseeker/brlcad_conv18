/*
 *			D M - X . C
 *
 *  An X Window System Display Manager.
 *
 *  Author -
 *	Phillip Dykstra
 *	Robert G. Parker
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1988 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"
#include <stdio.h>
#include <limits.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include "tk.h"
#include <X11/Xutil.h>

#ifdef HAVE_XOSDEFS_H
#include <X11/Xfuncproto.h>
#include <X11/Xosdefs.h>
#endif
#include <X11/extensions/XInput.h>
#if defined(linux)
#	undef   X_NOT_STDC_ENV
#	undef   X_NOT_POSIX
#endif
#define XLIB_ILLEGAL_ACCESS	/* necessary on facist SGI 5.0.1 */

#include "machine.h"
#include "externs.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "raytrace.h"
#include "dm.h"
#include "dm-X.h"
#include "dm_xvars.h"
#include "solid.h"

void     X_configureWindowShape();

static void	label();
static void	draw();
static void     x_var_init();
static XVisualInfo *X_choose_visual();

/* Display Manager package interface */

#define PLOTBOUND	1000.0	/* Max magnification in Rot matrix */
struct dm	*X_open();
static int	X_close();
static int	X_drawBegin(), X_drawEnd();
static int	X_normal(), X_loadMatrix();
static int      X_drawString2D();
static int	X_drawLine2D();
static int      X_drawPoint2D();
static int	X_drawVList();
static int      X_setColor(), X_setLineAttr();
static int	X_setWinBounds(), X_debug();

struct dm dm_X = {
  X_close,
  X_drawBegin,
  X_drawEnd,
  X_normal,
  X_loadMatrix,
  X_drawString2D,
  X_drawLine2D,
  X_drawPoint2D,
  X_drawVList,
  X_setColor,
  X_setLineAttr,
  X_setWinBounds,
  X_debug,
  Nu_int0,
  Nu_int0,
  Nu_int0,
  Nu_int0,
  0,
  0,				/* no displaylist */
  0,                            /* no stereo */
  PLOTBOUND,
  "X",
  "X Window System (X11)",
  DM_TYPE_X,
  1,
  0,
  0,
  0,
  0,
  1.0, /* aspect ratio */
  0,
  {0, 0},
  0,
  0,
  0
};

fastf_t min_short = (fastf_t)SHRT_MIN;
fastf_t max_short = (fastf_t)SHRT_MAX;

/* Currently, the application must define these. */
extern Tk_Window tkwin;

static mat_t xmat;

/*
 *			X _ O P E N
 *
 * Fire up the display manager, and the display processor.
 *
 */
struct dm *
X_open(argc, argv)
int argc;
char *argv[];
{
  static int count = 0;
  int i, j, k;
  int a_screen;
  int make_square = -1;
  XGCValues gcv;
  XColor a_color;
  Visual *a_visual;
  Colormap  a_cmap;
  int ndevices;
  int nclass = 0;
  XDeviceInfoPtr olist, list;
  XDevice *dev;
  XEventClass e_class[15];
  XInputClassInfo *cip;
  struct bu_vls str;
  struct bu_vls top_vls;
  struct bu_vls init_proc_vls;
  Display *tmp_dpy;
  struct dm *dmp;

  BU_GETSTRUCT(dmp, dm);
  if(dmp == DM_NULL)
    return DM_NULL;

  *dmp = dm_X; /* struct copy */

  dmp->dm_vars.pub_vars = (genptr_t)bu_calloc(1, sizeof(struct dm_xvars), "X_open: dm_xvars");
  if(dmp->dm_vars.pub_vars == (genptr_t)NULL){
    bu_free(dmp, "X_open: dmp");
    return DM_NULL;
  }

  dmp->dm_vars.priv_vars = (genptr_t)bu_calloc(1, sizeof(struct x_vars), "X_open: x_vars");
  if(dmp->dm_vars.priv_vars == (genptr_t)NULL){
    bu_free(dmp->dm_vars.pub_vars, "X_open: dmp->dm_vars.pub_vars");
    bu_free(dmp, "X_open: dmp");
    return DM_NULL;
  }

  bu_vls_init(&dmp->dm_pathName);
  bu_vls_init(&dmp->dm_tkName);
  bu_vls_init(&dmp->dm_dName);
  bu_vls_init(&init_proc_vls);

  i = dm_processOptions(dmp, &init_proc_vls, --argc, ++argv);

  if(bu_vls_strlen(&dmp->dm_pathName) == 0)
    bu_vls_printf(&dmp->dm_pathName, ".dm_X%d", count);

  ++count;
  if(bu_vls_strlen(&dmp->dm_dName) == 0){
    char *dp;

    dp = getenv("DISPLAY");
    if(dp)
      bu_vls_strcpy(&dmp->dm_dName, dp);
    else
      bu_vls_strcpy(&dmp->dm_dName, ":0.0");
  }
  if(bu_vls_strlen(&init_proc_vls) == 0)
    bu_vls_strcpy(&init_proc_vls, "bind_dm");

  /* initialize dm specific variables */
  ((struct dm_xvars *)dmp->dm_vars.pub_vars)->devmotionnotify = LASTEvent;
  ((struct dm_xvars *)dmp->dm_vars.pub_vars)->devbuttonpress = LASTEvent;
  ((struct dm_xvars *)dmp->dm_vars.pub_vars)->devbuttonrelease = LASTEvent;
  dmp->dm_aspect = 1.0;

  /* initialize modifiable variables */
  ((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.zclip = 1;

  ((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct = NULL;

  bu_vls_init(&top_vls);
  if(dmp->dm_top){
    /* Make xtkwin a toplevel window */
    ((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin = Tk_CreateWindowFromPath(interp, tkwin,
						      bu_vls_addr(&dmp->dm_pathName),
						      bu_vls_addr(&dmp->dm_dName));
    ((struct dm_xvars *)dmp->dm_vars.pub_vars)->top = ((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin;
    bu_vls_printf(&top_vls, "%S", &dmp->dm_pathName);
  }else{
    char *cp;

    cp = strrchr(bu_vls_addr(&dmp->dm_pathName), (int)'.');
    if(cp == bu_vls_addr(&dmp->dm_pathName)){
      ((struct dm_xvars *)dmp->dm_vars.pub_vars)->top = tkwin;
      bu_vls_strcpy(&top_vls, ".");
    }else{
      bu_vls_printf(&top_vls, "%*s", cp - bu_vls_addr(&dmp->dm_pathName),
		    bu_vls_addr(&dmp->dm_pathName));
      ((struct dm_xvars *)dmp->dm_vars.pub_vars)->top =
	Tk_NameToWindow(interp, bu_vls_addr(&top_vls), tkwin);
    }

    bu_vls_free(&top_vls);

    /* Make xtkwin an embedded window */
    ((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin =
      Tk_CreateWindow(interp, ((struct dm_xvars *)dmp->dm_vars.pub_vars)->top,
		      cp + 1, (char *)NULL);
  }

  if(((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin == NULL){
    bu_log("X_open: Failed to open %s\n", bu_vls_addr(&dmp->dm_pathName));
    (void)X_close(dmp);
    return DM_NULL;
  }

  bu_vls_printf(&dmp->dm_tkName, "%s",
		(char *)Tk_Name(((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin));

  bu_vls_init(&str);
  bu_vls_printf(&str, "_init_dm %S %S\n",
		&init_proc_vls,
		&dmp->dm_pathName);

  if(Tcl_Eval(interp, bu_vls_addr(&str)) == TCL_ERROR){
    bu_vls_free(&str);
    (void)X_close(dmp);

    return DM_NULL;
  }

  bu_vls_free(&init_proc_vls);
  bu_vls_free(&str);

  ((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy =
    Tk_Display(((struct dm_xvars *)dmp->dm_vars.pub_vars)->top);

  if(dmp->dm_width == 0){
    dmp->dm_width =
      DisplayWidth(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		   DefaultScreen(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy)) - 30;
    ++make_square;
  }

  if(dmp->dm_height == 0){
    dmp->dm_height =
      DisplayHeight(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		    DefaultScreen(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy)) - 30;
    ++make_square;
  }

  if(make_square > 0){
    /* Make window square */
    if(dmp->dm_height <
       dmp->dm_width)
      dmp->dm_width = dmp->dm_height;
    else
      dmp->dm_height = dmp->dm_width;
  }

  Tk_GeometryRequest(((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin,
		     dmp->dm_width, 
		     dmp->dm_height);

#if 0
  /*XXX For debugging purposes */
  XSynchronize(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy, TRUE);
#endif

  a_screen = Tk_ScreenNumber(((struct dm_xvars *)dmp->dm_vars.pub_vars)->top);

  /* must do this before MakeExist */
  if((((struct dm_xvars *)dmp->dm_vars.pub_vars)->vip = X_choose_visual(dmp)) == NULL){
    bu_log("X_open: Can't get an appropriate visual.\n");
    (void)X_close(dmp);
    return DM_NULL;
  }

  ((struct dm_xvars *)dmp->dm_vars.pub_vars)->depth = ((struct dm_xvars *)dmp->dm_vars.pub_vars)->vip->depth;

  Tk_MakeWindowExist(((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin);
  ((struct dm_xvars *)dmp->dm_vars.pub_vars)->win =
      Tk_WindowId(((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin);
  dmp->dm_id = ((struct dm_xvars *)dmp->dm_vars.pub_vars)->win;

  ((struct x_vars *)dmp->dm_vars.priv_vars)->pix =
    Tk_GetPixmap(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		 DefaultRootWindow(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy),
		 dmp->dm_width,
		 dmp->dm_height,
		 Tk_Depth(((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin));

  if(((struct x_vars *)dmp->dm_vars.priv_vars)->is_trueColor){
    XColor fg, bg;

    fg.red = 255 << 8;
    fg.green = fg.blue = 0;
    XAllocColor(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		((struct dm_xvars *)dmp->dm_vars.pub_vars)->cmap,
		&fg);
    ((struct x_vars *)dmp->dm_vars.priv_vars)->fg = fg.pixel;

    bg.red = bg.green = bg.blue = 0;
    XAllocColor(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		((struct dm_xvars *)dmp->dm_vars.pub_vars)->cmap,
		&bg);
    ((struct x_vars *)dmp->dm_vars.priv_vars)->bg = bg.pixel;
  }else{
    dm_allocate_color_cube( ((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
			    ((struct dm_xvars *)dmp->dm_vars.pub_vars)->cmap,
			    ((struct x_vars *)dmp->dm_vars.priv_vars)->pixels,
			    /* cube dimension, uses XStoreColor */
			    6, CMAP_BASE, 1 );

    ((struct x_vars *)dmp->dm_vars.priv_vars)->bg = dm_get_pixel(DM_BLACK,
						       ((struct x_vars *)dmp->dm_vars.priv_vars)->pixels,
						       CUBE_DIMENSION);
    ((struct x_vars *)dmp->dm_vars.priv_vars)->fg = dm_get_pixel(DM_RED,
						       ((struct x_vars *)dmp->dm_vars.priv_vars)->pixels,
						       CUBE_DIMENSION);
  }  

  gcv.background = ((struct x_vars *)dmp->dm_vars.priv_vars)->bg;
  gcv.foreground = ((struct x_vars *)dmp->dm_vars.priv_vars)->fg;
  ((struct x_vars *)dmp->dm_vars.priv_vars)->gc = XCreateGC(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
						  ((struct dm_xvars *)dmp->dm_vars.pub_vars)->win,
						  (GCForeground|GCBackground), &gcv);

  /* First see if the server supports XInputExtension */
  {
    int return_val;

    if(!XQueryExtension(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		     "XInputExtension", &return_val, &return_val, &return_val))
      goto Skip_dials;
  }

  /*
   * Take a look at the available input devices. We're looking
   * for "dial+buttons".
   */
  olist = list =
    (XDeviceInfoPtr)XListInputDevices(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
				      &ndevices);

  if( list == (XDeviceInfoPtr)NULL ||
      list == (XDeviceInfoPtr)1 )  goto Done;

  for(j = 0; j < ndevices; ++j, list++){
    if(list->use == IsXExtensionDevice){
      if(!strcmp(list->name, "dial+buttons")){
	if((dev = XOpenDevice(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
			      list->id)) == (XDevice *)NULL){
	  bu_log("X_open: Couldn't open the dials+buttons\n");
	  goto Done;
	}

	for(cip = dev->classes, k = 0; k < dev->num_classes;
	    ++k, ++cip){
	  switch(cip->input_class){
#if IR_BUTTONS
	  case ButtonClass:
	    DeviceButtonPress(dev, ((struct dm_xvars *)dmp->dm_vars.pub_vars)->devbuttonpress,
			      e_class[nclass]);
	    ++nclass;
	    DeviceButtonRelease(dev, ((struct dm_xvars *)dmp->dm_vars.pub_vars)->devbuttonrelease,
				e_class[nclass]);
	    ++nclass;
	    break;
#endif
#if IR_KNOBS
	  case ValuatorClass:
	    DeviceMotionNotify(dev, ((struct dm_xvars *)dmp->dm_vars.pub_vars)->devmotionnotify,
			       e_class[nclass]);
	    ++nclass;
	    break;
#endif
	  default:
	    break;
	  }
	}

	XSelectExtensionEvent(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
			      ((struct dm_xvars *)dmp->dm_vars.pub_vars)->win, e_class, nclass);
	goto Done;
      }
    }
  }
Done:
  XFreeDeviceList(olist);

Skip_dials:
#ifndef CRAY2
  X_configureWindowShape(dmp);
#endif

  Tk_SetWindowBackground(((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin,
			 ((struct x_vars *)dmp->dm_vars.priv_vars)->bg);
  Tk_MapWindow(((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin);

  bn_mat_idn(xmat);

  return dmp;
}

/*
 *  			X _ C L O S E
 *  
 *  Gracefully release the display.
 */
static int
X_close(dmp)
struct dm *dmp;
{
  if(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy){
    if(((struct x_vars *)dmp->dm_vars.priv_vars)->gc)
      XFreeGC(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
	      ((struct x_vars *)dmp->dm_vars.priv_vars)->gc);

    if(((struct x_vars *)dmp->dm_vars.priv_vars)->pix)
      Tk_FreePixmap(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		    ((struct x_vars *)dmp->dm_vars.priv_vars)->pix);

    /*XXX Possibly need to free the colormap */
    if(((struct dm_xvars *)dmp->dm_vars.pub_vars)->cmap)
      XFreeColormap(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		    ((struct dm_xvars *)dmp->dm_vars.pub_vars)->cmap);

    if(((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin)
      Tk_DestroyWindow(((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin);

#if 0
    XCloseDisplay(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy);
#endif
  }

  bu_vls_free(&dmp->dm_pathName);
  bu_vls_free(&dmp->dm_tkName);
  bu_vls_free(&dmp->dm_dName);
  bu_free(dmp->dm_vars.priv_vars, "X_close: x_vars");
  bu_free(dmp->dm_vars.pub_vars, "X_close: dm_xvars");
  bu_free(dmp, "X_close: dmp");

  return TCL_OK;
}

/*
 *			X _ P R O L O G
 *
 * There are global variables which are parameters to this routine.
 */
static int
X_drawBegin(dmp)
struct dm *dmp;
{
  XGCValues       gcv;

  if (((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug)
    bu_log("X_drawBegin()\n");

  /* clear pixmap */
  gcv.foreground = ((struct x_vars *)dmp->dm_vars.priv_vars)->bg;
  XChangeGC(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
	    ((struct x_vars *)dmp->dm_vars.priv_vars)->gc,
	    GCForeground, &gcv);
  XFillRectangle(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		 ((struct x_vars *)dmp->dm_vars.priv_vars)->pix,
		 ((struct x_vars *)dmp->dm_vars.priv_vars)->gc, 0,
		 0, dmp->dm_width + 1,
		 dmp->dm_height + 1);

  return TCL_OK;
}

/*
 *			X _ E P I L O G
 */
static int
X_drawEnd(dmp)
struct dm *dmp;
{
  if (((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug)
    bu_log("X_drawEnd()\n");

  XCopyArea(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
	    ((struct x_vars *)dmp->dm_vars.priv_vars)->pix,
	    ((struct dm_xvars *)dmp->dm_vars.pub_vars)->win,
	    ((struct x_vars *)dmp->dm_vars.priv_vars)->gc,
	      0, 0, dmp->dm_width,
	    dmp->dm_height, 0, 0);

  /* Prevent lag between events and updates */
  XSync(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy, 0);

  return TCL_OK;
}

/*
 *  			X _ L O A D M A T R I X
 *
 *  Load a new transformation matrix.  This will be followed by
 *  many calls to X_drawVList().
 */
/* ARGSUSED */
static int
X_loadMatrix(dmp, mat, which_eye)
struct dm *dmp;
mat_t mat;
int which_eye;
{
  if(((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug){
    bu_log("X_loadMatrix()\n");

    bu_log("which eye = %d\t", which_eye);
    bu_log("transformation matrix = \n");
#if 1
    bu_log("%g %g %g %g\n", mat[0], mat[1], mat[2], mat[3]);
    bu_log("%g %g %g %g\n", mat[4], mat[5], mat[6], mat[7]);
    bu_log("%g %g %g %g\n", mat[8], mat[9], mat[10], mat[11]);
    bu_log("%g %g %g %g\n", mat[12], mat[13], mat[14], mat[15]);
#else
    bu_log("%g %g %g %g\n", mat[0], mat[4], mat[8], mat[12]);
    bu_log("%g %g %g %g\n", mat[1], mat[5], mat[9], mat[13]);
    bu_log("%g %g %g %g\n", mat[2], mat[6], mat[10], mat[14]);
    bu_log("%g %g %g %g\n", mat[3], mat[7], mat[11], mat[15]);
#endif
  }

  bn_mat_copy(xmat, mat);
  return TCL_OK;
}

/*
 *  			X _ D R A W V L I S T
 *  
 */

static int
X_drawVList( dmp, vp, perspective )
struct dm *dmp;
register struct rt_vlist *vp;
double perspective;
{
    static vect_t spnt, lpnt, pnt;
    register struct rt_vlist *tvp;
    XSegment segbuf[1024];		/* XDrawSegments list */
    XSegment *segp;			/* current segment */
    XGCValues gcv;
    int	nseg;			        /* number of segments */
    fastf_t delta;
    register point_t *pt_prev = NULL;
    fastf_t 	 dist_prev=1.0;

    if(((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug)
      bu_log("X_drawVList()\n");

    /* delta is used in clipping to insure clipped endpoint is slightly
     * in front of eye plane (perspective mode only).
     * This value is a SWAG that seems to work OK.
     */
    delta = xmat[15]*0.0001;
    if( delta < 0.0 )
	delta = -delta;
    if( delta < SQRT_SMALL_FASTF )
	delta = SQRT_SMALL_FASTF;

    nseg = 0;
    segp = segbuf;
    for( BU_LIST_FOR( tvp, rt_vlist, &vp->l ) )  {
	register int	i;
	register int	nused = tvp->nused;
	register int	*cmd = tvp->cmd;
	register point_t *pt = tvp->pt;
    	fastf_t 	 dist;

	/* Viewing region is from -1.0 to +1.0 */
	/* 2^31 ~= 2e9 -- dynamic range of a long int */
	/* 2^(31-11) = 2^20 ~= 1e6 */
	/* Integerize and let the X server do the clipping */
	for( i = 0; i < nused; i++,cmd++,pt++ )  {
	    switch( *cmd )  {
	    case RT_VLIST_POLY_START:
	    case RT_VLIST_POLY_VERTNORM:
		continue;
	    case RT_VLIST_POLY_MOVE:
	    case RT_VLIST_LINE_MOVE:
		/* Move, not draw */
		if(((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug){
		  bu_log("before transformation:\n");
		  bu_log("pt - %lf %lf %lf\n", V3ARGS(*pt));
		}

		if( perspective > 0.0 )
	    	{
	    		/* cannot apply perspective transformation to
			 * points behind eye plane!!!!
	    		 */
	    		dist = VDOT( *pt, &xmat[12] ) + xmat[15];
	    		if( dist <= 0.0 )
	    		{
	    			pt_prev = pt;
	    			dist_prev = dist;
	    			continue;
	    		}
	    		else
	    		{
	    			MAT4X3PNT( lpnt, xmat, *pt );
	    			dist_prev = dist;
	    			pt_prev = pt;
	    		}
	    	}
		else
			MAT4X3PNT( lpnt, xmat, *pt );

#ifdef USE_RT_ASPECT
		lpnt[0] *= 2047;
		lpnt[1] *= 2047 * dmp->dm_aspect;
#else
		lpnt[0] *= 2047 * dmp->dm_aspect;
		lpnt[1] *= 2047;
#endif
		lpnt[2] *= 2047;
		continue;
	    case RT_VLIST_POLY_DRAW:
	    case RT_VLIST_POLY_END:
	    case RT_VLIST_LINE_DRAW:
		/* draw */
		if(((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug){
		  bu_log("before transformation:\n");
		  bu_log("pt - %lf %lf %lf\n", V3ARGS(*pt));
		}

		if( perspective > 0.0 )
	    	{
	    		/* cannot apply perspective transformation to
			 * points behind eye plane!!!!
	    		 */
	    		dist = VDOT( *pt, &xmat[12] ) + xmat[15];
			if(((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug )
	    			bu_log( "dist=%g, dist_prev=%g\n", dist, dist_prev );
	    		if( dist <= 0.0 )
	    		{
	    			if( dist_prev <= 0.0 )
	    			{
	    				/* nothing to plot */
		    			dist_prev = dist;
		    			pt_prev = pt;
		    			continue;
	    			}
	    			else
	    			{
	    				fastf_t alpha;
	    				vect_t diff;
	    				point_t tmp_pt;

	    				/* clip this end */
	    				VSUB2( diff, *pt, *pt_prev );
	    				alpha = (dist_prev - delta) / ( dist_prev - dist );
	    				VJOIN1( tmp_pt, *pt_prev, alpha, diff );
	    				MAT4X3PNT( pnt, xmat, tmp_pt );
	    			}
	    		}
	    		else
	    		{
	    			if( dist_prev <= 0.0 )
	    			{
	    				fastf_t alpha;
	    				vect_t diff;
	    				point_t tmp_pt;

	    				/* clip other end */
	    				VSUB2( diff, *pt, *pt_prev );
	    				alpha = (-dist_prev + delta) / ( dist - dist_prev );
	    				VJOIN1( tmp_pt, *pt_prev, alpha, diff );
	    				MAT4X3PNT( lpnt, xmat, tmp_pt );
	    				lpnt[0] *= 2047;
	    				lpnt[1] *= 2047 * dmp->dm_aspect;
	    				lpnt[2] *= 2047;
	    				MAT4X3PNT( pnt, xmat, *pt );
	    			}
	    			else
	    			{
	    				MAT4X3PNT( pnt, xmat, *pt );
	    			}
	    		}
	    	}
		else
			MAT4X3PNT( pnt, xmat, *pt );

#ifdef USE_RT_ASPECT
		pnt[0] *= 2047;
		pnt[1] *= 2047 * dmp->dm_aspect;
#else
		pnt[0] *= 2047 * dmp->dm_aspect;
		pnt[1] *= 2047;
#endif
		pnt[2] *= 2047;

		/* save pnt --- it might get changed by clip() */
		VMOVE(spnt, pnt);
	    	pt_prev = pt;
	    	dist_prev = dist;

		if(((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug) {
		  bu_log("before clipping:\n");
		  bu_log("clipmin - %lf %lf %lf\n",
			 ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmin[X],
			 ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmin[Y],
			 ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmin[Z]);
		  bu_log("clipmax - %lf %lf %lf\n",
			 ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmax[X],
			 ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmax[Y],
			 ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmax[Z]);
		  bu_log("pt1 - %lf %lf %lf\n", lpnt[X], lpnt[Y], lpnt[Z]);
		  bu_log("pt2 - %lf %lf %lf\n", pnt[X], pnt[Y], pnt[Z]);
		}

		if(((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.zclip){
		  if(vclip(lpnt, pnt,
			   ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmin,
			   ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmax) == 0){
		    VMOVE(lpnt, spnt);
		    continue;
		  }
		}else{
		  /* Check to see if lpnt or pnt contain values that exceed
		     the capacity of a short (segbuf is an array of XSegments which
		     contain shorts). If so, do clipping now. Otherwise, let the
		     X server do the clipping */
		  if(lpnt[0] < min_short || max_short < lpnt[0] ||
		     lpnt[1] < min_short || max_short < lpnt[1] ||
		     pnt[0] < min_short || max_short < pnt[0] ||
		     pnt[1] < min_short || max_short < pnt[1]){
		    /* if the entire line segment will not be visible then ignore it */
		    if(clip(&lpnt[0], &lpnt[1], &pnt[0], &pnt[1]) == -1){
		      VMOVE(lpnt, spnt);
		      continue;
		    }
		  }
		}

		if(((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug) {
		  bu_log("after clipping:\n");
		  bu_log("pt1 - %lf %lf %lf\n", lpnt[X], lpnt[Y], lpnt[Z]);
		  bu_log("pt2 - %lf %lf %lf\n", pnt[X], pnt[Y], pnt[Z]);
		}

		/* convert to X window coordinates */
		segp->x1 = (short)GED_TO_Xx(dmp, lpnt[0]);
		segp->y1 = (short)GED_TO_Xy(dmp, lpnt[1]);
		segp->x2 = (short)GED_TO_Xx(dmp, pnt[0]);
		segp->y2 = (short)GED_TO_Xy(dmp, pnt[1]);

		nseg++;
		segp++;
		VMOVE(lpnt, spnt);

		if( nseg == 1024 ) {
		  XDrawSegments( ((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
				 ((struct x_vars *)dmp->dm_vars.priv_vars)->pix,
				 ((struct x_vars *)dmp->dm_vars.priv_vars)->gc, segbuf, nseg );

		  nseg = 0;
		  segp = segbuf;
		}
		break;
	    }
	}
    }
    if( nseg ) {
      XDrawSegments( ((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		     ((struct x_vars *)dmp->dm_vars.priv_vars)->pix,
		     ((struct x_vars *)dmp->dm_vars.priv_vars)->gc, segbuf, nseg );
    }

    return TCL_OK;
}

/*
 *			X _ N O R M A L
 *
 * Restore the display processor to a normal mode of operation
 * (ie, not scaled, rotated, displaced, etc).
 */
static int
X_normal(dmp)
struct dm *dmp;
{
  if (((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug)
    bu_log("X_normal()\n");

  return TCL_OK;
}

/*
 *			X _ D R A W S T R I N G 2 D
 *
 * Output a string into the displaylist.
 * The starting position of the beam is as specified.
 */
/* ARGSUSED */
static int
X_drawString2D(dmp, str, x, y, size, use_aspect)
struct dm *dmp;
register char *str;
fastf_t x, y;
int size;
int use_aspect;
{
  int sx, sy;

  if (((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug){
    bu_log("X_drawString2D():\n");
    bu_log("\tstr - %s\n", str);
    bu_log("\tx - %lf\n", x);
    bu_log("\ty - %lf\n", y);
    bu_log("\tsize - %d\n", size);
    if(use_aspect){
      bu_log("\tuse_aspect - %d\t\taspect ratio - %lf\n", use_aspect, dmp->dm_aspect);
    }else
      bu_log("\tuse_aspect - 0");
  }

#ifdef USE_RT_ASPECT
  sx = dm_Normal2Xx(dmp, x);
  sy = dm_Normal2Xy(dmp, y, use_aspect);
#else
  sx = dm_Normal2Xx(dmp, x, use_aspect);
  sy = dm_Normal2Xy(dmp, y);
#endif

  XDrawString( ((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
	       ((struct x_vars *)dmp->dm_vars.priv_vars)->pix,
	       ((struct x_vars *)dmp->dm_vars.priv_vars)->gc,
	       sx, sy, str, strlen(str) );

  return TCL_OK;
}

static int
X_drawLine2D( dmp, x1, y1, x2, y2)
struct dm *dmp;
fastf_t x1, y1;
fastf_t x2, y2;
{
  int	sx1, sy1, sx2, sy2;

  if (((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug)
    bu_log("X_drawLine2D()\n");

  sx1 = dm_Normal2Xx(dmp, x1, 0);
  sx2 = dm_Normal2Xx(dmp, x2, 0);
  sy1 = dm_Normal2Xy(dmp, y1);
  sy2 = dm_Normal2Xy(dmp, y2);

  XDrawLine( ((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
	     ((struct x_vars *)dmp->dm_vars.priv_vars)->pix,
	     ((struct x_vars *)dmp->dm_vars.priv_vars)->gc,
	     sx1, sy1, sx2, sy2 );

  return TCL_OK;
}

static int
X_drawPoint2D( dmp, x, y )
struct dm *dmp;
fastf_t x, y;
{
  int   sx, sy;

  if (((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug)
    bu_log("X_drawPoint2D()\n");

  sx = dm_Normal2Xx(dmp, x, 0);
  sy = dm_Normal2Xy(dmp, y);

  XDrawPoint( ((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
	      ((struct x_vars *)dmp->dm_vars.priv_vars)->pix,
	      ((struct x_vars *)dmp->dm_vars.priv_vars)->gc, sx, sy );

  return TCL_OK;
}

static int
X_setColor(dmp, r, g, b, strict)
struct dm *dmp;
register short r, g, b;
int strict;
{
  XGCValues gcv;

  if (((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug)
    bu_log("X_setColor()\n");

  if(((struct x_vars *)dmp->dm_vars.priv_vars)->is_trueColor){
    XColor color;

    color.red = r << 8;
    color.green = g << 8;
    color.blue = b << 8;
    XAllocColor(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		((struct dm_xvars *)dmp->dm_vars.pub_vars)->cmap,
		&color);
    gcv.foreground = color.pixel;
  }else
    gcv.foreground = dm_get_pixel(r, g, b, ((struct x_vars *)dmp->dm_vars.priv_vars)->pixels,
				  CUBE_DIMENSION);

  XChangeGC(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
	    ((struct x_vars *)dmp->dm_vars.priv_vars)->gc,
	    GCForeground, &gcv);

  return TCL_OK;
}

static int
X_setLineAttr(dmp, width, style)
struct dm *dmp;
int width;
int style;
{
  int linestyle;

  if (((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug)
    bu_log("X_setLineAttr()\n");

  dmp->dm_lineWidth = width;
  dmp->dm_lineStyle = style;

  if(width <= 1)
    width = 0;

  if(style == DM_DASHED_LINE)
    linestyle = LineOnOffDash;
  else
    linestyle = LineSolid;

  XSetLineAttributes( ((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		      ((struct x_vars *)dmp->dm_vars.priv_vars)->gc,
		      width, linestyle, CapButt, JoinMiter );

  return TCL_OK;
}

/* ARGSUSED */
static int
X_debug(dmp, lvl)
struct dm *dmp;
int lvl;
{
  ((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug = lvl;
  XFlush(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy);
  bu_log("flushed\n");

  return TCL_OK;
}

static int
X_setWinBounds(dmp, w)
struct dm *dmp;
register int w[6];
{
  if (((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug)
    bu_log("X_setWinBounds()\n");

  ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmin[0] = w[0];
  ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmin[1] = w[2];
  ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmin[2] = w[4];
  ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmax[0] = w[1];
  ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmax[1] = w[3];
  ((struct x_vars *)dmp->dm_vars.priv_vars)->clipmax[2] = w[5];

  return TCL_OK;
}

void
X_configureWindowShape(dmp)
struct dm *dmp;
{
  XWindowAttributes xwa;
  XFontStruct     *newfontstruct;
  XGCValues       gcv;

  if (((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.debug)
    bu_log("X_configureWindowShape()\n");

  XGetWindowAttributes( ((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
			((struct dm_xvars *)dmp->dm_vars.pub_vars)->win, &xwa );
  dmp->dm_height = xwa.height;
  dmp->dm_width = xwa.width;
#ifdef USE_RT_ASPECT
  dmp->dm_aspect = (fastf_t)dmp->dm_width / (fastf_t)dmp->dm_height;
#else
  dmp->dm_aspect =
    (fastf_t)dmp->dm_height /
    (fastf_t)dmp->dm_width;
#endif

  Tk_FreePixmap(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		((struct x_vars *)dmp->dm_vars.priv_vars)->pix);
  ((struct x_vars *)dmp->dm_vars.priv_vars)->pix =
    Tk_GetPixmap(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		 DefaultRootWindow(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy),
		 dmp->dm_width,
		 dmp->dm_height,
		 Tk_Depth(((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin));

  /* First time through, load a font or quit */
  if (((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct == NULL) {
    if ((((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct =
	 XLoadQueryFont(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy, FONT9)) == NULL ) {
      /* Try hardcoded backup font */
      if ((((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct =
	   XLoadQueryFont(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy, FONTBACK)) == NULL) {
	bu_log("dm-X: Can't open font '%s' or '%s'\n", FONT9, FONTBACK);
	return;
      }
    }

    gcv.font = ((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct->fid;
    XChangeGC(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
	      ((struct x_vars *)dmp->dm_vars.priv_vars)->gc, GCFont, &gcv);
  }

  /* Always try to choose a the font that best fits the window size.
   */

  if (dmp->dm_width < 582) {
    if (((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct->per_char->width != 5) {
      if ((newfontstruct = XLoadQueryFont(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
					  FONT5)) != NULL ) {
	XFreeFont(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		  ((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct);
	((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct = newfontstruct;
	gcv.font = ((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct->fid;
	XChangeGC(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		  ((struct x_vars *)dmp->dm_vars.priv_vars)->gc, GCFont, &gcv);
      }
    }
  } else if (dmp->dm_width < 679) {
    if (((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct->per_char->width != 6){
      if ((newfontstruct = XLoadQueryFont(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
					  FONT6)) != NULL ) {
	XFreeFont(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		  ((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct);
	((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct = newfontstruct;
	gcv.font = ((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct->fid;
	XChangeGC(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		  ((struct x_vars *)dmp->dm_vars.priv_vars)->gc, GCFont, &gcv);
      }
    }
  } else if (dmp->dm_width < 776) {
    if (((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct->per_char->width != 7){
      if ((newfontstruct = XLoadQueryFont(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
					  FONT7)) != NULL ) {
	XFreeFont(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		  ((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct);
	((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct = newfontstruct;
	gcv.font = ((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct->fid;
	XChangeGC(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		  ((struct x_vars *)dmp->dm_vars.priv_vars)->gc, GCFont, &gcv);
      }
    }
  } else if (dmp->dm_width < 873) {
    if (((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct->per_char->width != 8){
      if ((newfontstruct = XLoadQueryFont(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
					  FONT8)) != NULL ) {
	XFreeFont(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		  ((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct);
	((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct = newfontstruct;
	gcv.font = ((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct->fid;
	XChangeGC(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		  ((struct x_vars *)dmp->dm_vars.priv_vars)->gc, GCFont, &gcv);
      }
    }
  } else {
    if (((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct->per_char->width != 9){
      if ((newfontstruct = XLoadQueryFont(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
					  FONT9)) != NULL ) {
	XFreeFont(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		  ((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct);
	((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct = newfontstruct;
	gcv.font = ((struct dm_xvars *)dmp->dm_vars.pub_vars)->fontstruct->fid;
	XChangeGC(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		  ((struct x_vars *)dmp->dm_vars.priv_vars)->gc, GCFont, &gcv);
      }
    }
  }
}

static XVisualInfo *
X_choose_visual(dmp)
struct dm *dmp;
{
  XVisualInfo *vip, vitemp, *vibase, *maxvip;
  int good[256];
  int num, i, j;
  int tries, baddepth;
  int desire_trueColor = 1;

  vibase = XGetVisualInfo(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy, 0, &vitemp, &num);

  while(1){
    for (i=0, j=0, vip=vibase; i<num; i++, vip++){
      /* requirements */
      if(desire_trueColor){
	if(vip->class != TrueColor)
	  continue;
      }else if (vip->class != PseudoColor)
	continue;
			
      /* this visual meets criteria */
      good[j++] = i;
    }

    baddepth = 1000;
    for(tries = 0; tries < j; ++tries) {
      maxvip = vibase + good[0];
      for (i=1; i<j; i++) {
	vip = vibase + good[i];
	if ((vip->depth > maxvip->depth)&&(vip->depth < baddepth)){
	  maxvip = vip;
	}
      }

      /* make sure Tk handles it */
      if(desire_trueColor){
	((struct dm_xvars *)dmp->dm_vars.pub_vars)->cmap = XCreateColormap(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy, RootWindow(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy, maxvip->screen),
								maxvip->visual, AllocNone);
	((struct x_vars *)dmp->dm_vars.priv_vars)->is_trueColor = 1;
      }else{
	((struct dm_xvars *)dmp->dm_vars.pub_vars)->cmap = XCreateColormap(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy, RootWindow(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy, maxvip->screen),
								maxvip->visual, AllocAll);
	((struct x_vars *)dmp->dm_vars.priv_vars)->is_trueColor = 0;
      }

      if (Tk_SetWindowVisual(((struct dm_xvars *)dmp->dm_vars.pub_vars)->xtkwin, maxvip->visual,
			     maxvip->depth, ((struct dm_xvars *)dmp->dm_vars.pub_vars)->cmap)){
	((struct dm_xvars *)dmp->dm_vars.pub_vars)->depth = maxvip->depth;
	return maxvip; /* success */
      } else { 
	/* retry with lesser depth */
	baddepth = maxvip->depth;
	XFreeColormap(((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy, ((struct dm_xvars *)dmp->dm_vars.pub_vars)->cmap);
      }
    }

    if(desire_trueColor)
      desire_trueColor = 0;
    else
      return NULL; /* failure */
  }
}




