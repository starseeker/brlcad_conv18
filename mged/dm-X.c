/*
 *			D M - X . C
 *
 *  Routines specific to MGED's use of LIBDM's X display manager.
 *
 *  Author -
 *	Robert G. Parker
 *
 *  Source -
 *	SLAD CAD Team
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *
 */

#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <math.h>
#include "tk.h"
#include <X11/Xutil.h>

#include "machine.h"
#include "externs.h"
#include "bu.h"
#include "vmath.h"
#include "mater.h"
#include "raytrace.h"
#include "dm_xvars.h"
#include "dm-X.h"
#include "./ged.h"
#include "./sedit.h"
#include "./mged_dm.h"

extern int common_dm();			/* defined in dm-generic.c */
extern void dm_var_init();		/* defined in attach.c */

static int X_dm();
static void dirty_hook();

#ifdef USE_PROTOTYPES
static Tk_GenericProc X_doevent;
#else
static int X_doevent();
#endif

struct bu_structparse X_vparse[] = {
  {"%d",  1, "zclip",             X_MV_O(zclip),            dirty_hook },
  {"%d",  1, "debug",             X_MV_O(debug),            BU_STRUCTPARSE_FUNC_NULL },
  {"",    0, (char *)0,           0,                        BU_STRUCTPARSE_FUNC_NULL }
};

int
X_dm_init(o_dm_list, argc, argv)
struct dm_list *o_dm_list;
int argc;
char *argv[];
{
  int i;
  struct bu_vls vls;

  dm_var_init(o_dm_list);

  /* register application provided routines */
  cmd_hook = X_dm;

  Tk_DeleteGenericHandler(doEvent, (ClientData)NULL);
  if((dmp = dm_open(DM_TYPE_X, argc-1, argv)) == DM_NULL)
    return TCL_ERROR;

  zclip_ptr = &((struct x_vars *)dmp->dm_vars.priv_vars)->mvars.zclip;
  eventHandler = X_doevent;
  Tk_CreateGenericHandler(doEvent, (ClientData)NULL);
  dm_configureWindowShape(dmp);

  bu_vls_init(&vls);
  bu_vls_printf(&vls, "mged_bind_dm %s", bu_vls_addr(&pathName));
  Tcl_Eval(interp, bu_vls_addr(&vls));
  bu_vls_free(&vls);

  return TCL_OK;
}

void
X_fb_open()
{
  int status;
  struct bu_vls vls;

  bu_vls_init(&vls);
  bu_vls_printf(&vls, "fb_open_existing /dev/X %lu %lu %lu %lu %d %d %lu",
		(unsigned long)((struct dm_xvars *)dmp->dm_vars.pub_vars)->dpy,
		(unsigned long)((struct x_vars *)dmp->dm_vars.priv_vars)->pix,
		(unsigned long)((struct dm_xvars *)dmp->dm_vars.pub_vars)->cmap,
		(unsigned long)((struct dm_xvars *)dmp->dm_vars.pub_vars)->vip,
		dmp->dm_width, dmp->dm_height,
		(unsigned long)((struct x_vars *)dmp->dm_vars.priv_vars)->gc);
  status = Tcl_Eval(interp, bu_vls_addr(&vls));

  if(status == TCL_OK){
    if(sscanf(interp->result, "%lu", (unsigned long *)&fbp) != 1){
      fbp = (FBIO *)0;   /* sanity */
      Tcl_AppendResult(interp, "X_fb_open: failed to get framebuffer pointer\n",
		       (char *)NULL);
    }
  }else
    Tcl_AppendResult(interp, "X_fb_open: failed to get framebuffer\n", (char *)NULL);

  bu_vls_free(&vls);
}

/*
   This routine is being called from doEvent().
   It does not handle mouse button or key events. The key
   events are being processed via the TCL/TK bind command or are being
   piped to ged.c/stdin_input(). Eventually, I'd also like to have the
   dials+buttons bindable. That would leave this routine to handle only
   events like Expose and ConfigureNotify.
*/
static int
X_doevent(clientData, eventPtr)
ClientData clientData;
XEvent *eventPtr;
{
  if (eventPtr->type == Expose && eventPtr->xexpose.count == 0){
    dirty = 1;

    /* no further processing of this event */
    return TCL_RETURN;
  }

  /* allow further processing of this event */
  return TCL_OK;
}
	    
static int
X_dm(argc, argv)
int argc;
char *argv[];
{
  struct bu_vls	vls;
  int status;
  char *av[6];
  char xstr[32];
  char ystr[32];

  if( !strcmp( argv[0], "set" ) )  {
    struct bu_vls tmp_vls;

    bu_vls_init(&vls);
    bu_vls_init(&tmp_vls);
    start_catching_output(&tmp_vls);

    if( argc < 2 )  {
      /* Bare set command, print out current settings */
      bu_struct_print("dm_X internal variables", X_vparse, (CONST char *)&((struct x_vars *)dmp->dm_vars.priv_vars)->mvars );
    } else if( argc == 2 ) {
      bu_vls_struct_item_named( &vls, X_vparse, argv[1], (CONST char *)&((struct x_vars *)dmp->dm_vars.priv_vars)->mvars, ',');
      bu_log( "%s\n", bu_vls_addr(&vls) );
    } else {
      bu_vls_printf( &vls, "%s=\"", argv[1] );
      bu_vls_from_argv( &vls, argc-2, argv+2 );
      bu_vls_putc( &vls, '\"' );
      bu_struct_parse( &vls, X_vparse, (char *)&((struct x_vars *)dmp->dm_vars.priv_vars)->mvars );
    }

    bu_vls_free(&vls);

    stop_catching_output(&tmp_vls);
    Tcl_AppendResult(interp, bu_vls_addr(&tmp_vls), (char *)NULL);
    bu_vls_free(&tmp_vls);
    return TCL_OK;
  }

  return common_dm(argc, argv);
}

static void
dirty_hook()
{
  dirty = 1;
}
