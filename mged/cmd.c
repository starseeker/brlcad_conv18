/*
 *			C M D . C
 *
 * Functions -
 *	f_press		hook for displays with no buttons
 *	f_summary	do directory summary
 *	mged_cmd		Check arg counts, run a command
 *
 *  Authors -
 *	Michael John Muuss
 *	Charles M. Kennedy
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
#include <signal.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <sys/time.h>
#include <time.h>

#include "tcl.h"
#include "tk.h"

#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "rtgeom.h"
#include "externs.h"
#include "./ged.h"
#include "./mged_solid.h"
#include "./mged_dm.h"
#include "./sedit.h"

#include "./mgedtcl.h"

int cmd_mged_glob();
int cmd_init();
int cmd_set();
int cmd_get();
int get_more_default();
int tran(), irot();
void set_tran(), mged_setup(), cmd_setup(), mged_compat();

extern mat_t    ModelDelta;

extern int gui_setup();
extern int cmd_stuff_str();
extern int f_nmg_simplify();
extern int f_make_bb();
extern int f_whatid();
extern int f_which_air();
extern int f_eac();

extern short earb4[5][18];
extern short earb5[9][18];
extern short earb6[10][18];
extern short earb7[12][18];
extern short earb8[12][18];
extern struct rt_db_internal es_int;
extern short int fixv; /* used in ECMD_ARB_ROTATE_FACE,f_eqn(): fixed vertex */

struct cmd_list head_cmd_list;
struct cmd_list *curr_cmd_list;
point_t e_axes_pos;
void set_e_axes_pos();


/* Carl Nuzman experimental */
#if 1
extern int cmd_vdraw();
extern int cmd_viewget();
extern int cmd_viewset();
extern int cmd_who();
#endif

extern Tcl_CmdProc	cmd_fhelp;

extern void sync();
int	inpara;			/* parameter input from keyboard */

int glob_compat_mode = 1;
int output_as_return = 1;

int mged_cmd();
struct bu_vls tcl_output_hook;

Tcl_Interp *interp = NULL;
Tk_Window tkwin;


/*
 *			C M D _ L E F T _ M O U S E
 *
 *  Default old-MGED binding for left mouse button.
 */
int
cmd_left_mouse(clientData, interp, argc, argv)
ClientData	clientData;
Tcl_Interp	*interp;
int		argc;
char		*argv[];
{
	static char	cmd[] = "zoom 0.5\n";

	if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
	  return TCL_ERROR;

	if( atoi(argv[1]) != 0 )
	  return Tcl_Eval( interp, cmd );

	return TCL_OK;
}

/*
 *			C M D _ R I G H T _ M O U S E
 *
 *  Default old-MGED binding for right mouse button.
 */
int
cmd_right_mouse(clientData, interp, argc, argv)
ClientData	clientData;
Tcl_Interp	*interp;
int		argc;
char		*argv[];
{
	static char	cmd[] = "zoom 2\n";

	if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
	  return TCL_ERROR;

	if( atoi(argv[1]) != 0 )
		return Tcl_Eval( interp, cmd );
	return TCL_OK;
}

static struct funtab funtab[] = {
"", "", "Primary command Table.",
        0, 0, 0, FALSE,
"?", "", "summary of available commands",
        cmd_fhelp,0,MAXARGS,TRUE,
"%", "", "escape to interactive shell",
	f_comm,1,1,TRUE,
"3ptarb", "", "makes arb given 3 pts, 2 coord of 4th pt, and thickness",
	f_3ptarb, 1, 27,TRUE,
"adc", "[<a1|a2|dst|dh|dv|hv|dx|dy|dz|xyz|reset|help> value(s)]",
	"control the angle/distance cursor",
        f_adc, 1, 5, TRUE,
"ae", "azim elev", "set view using az and elev angles",
	f_aeview, 3, 3, TRUE,
"aim", "[command_window [pathName]]", "aims command_window at pathName",
        f_aim, 1, 3, TRUE,
"aip", "[fb]", "advance illumination pointer or path position forward or backward",
        f_aip, 1, 2, TRUE,
"analyze", "[arbname]", "analyze faces of ARB",
	f_analyze,1,MAXARGS,TRUE,
"apropos", "keyword", "finds commands whose descriptions contain the given keyword",
        cmd_apropos, 2, 2, TRUE,
"arb", "name rot fb", "make arb8, rotation + fallback",
	f_arbdef,4,4,TRUE,
"arced", "a/b ...anim_command...", "edit matrix or materials on combination's arc",
	f_arced, 3,MAXARGS,TRUE,
"area", "[endpoint_tolerance]", "calculate presented area of view",
	f_area, 1, 2, TRUE,
"attach", "device screen", "attach to a display processor on screen",
	f_attach,1,MAXARGS,TRUE,
"B", "<objects>", "clear screen, edit objects",
	f_blast,2,MAXARGS,TRUE,
"bev",	"[-t] [-P#] new_obj obj1 op obj2 op obj3 op ...", "Boolean evaluation of objects via NMG's",
	f_bev, 2, MAXARGS, TRUE,
#if 0
"button", "number", "simulates a button press, not intended for the user",
	f_button, 2, 2, FALSE,
#endif
"c", "[-gr] comb_name [boolean_expr]", "create or extend a combination using standard notation",
	f_comb_std,3,MAXARGS,TRUE,
"cat", "<objects>", "list attributes (brief)",
	f_cat,2,MAXARGS,TRUE,
"center", "x y z", "set view center",
	f_center, 4,4, TRUE,
"color", "low high r g b str", "make color entry",
	f_color, 7, 7, TRUE,
"comb", "comb_name <operation solid>", "create or extend combination w/booleans",
	f_comb,4,MAXARGS,TRUE,
"comb_color", "comb R G B", "assign a color to a combination (like 'mater')",
	f_comb_color, 5,5,TRUE,
"copyeval", "new_solid path_to_old_solid (seperate path components with spaces, not /)",
	"copy an 'evaluated' path solid",
	f_copyeval, 1, 27, TRUE,
"cp", "from to", "copy [duplicate] object",
	f_copy,3,3, TRUE,
"cpi", "from to", "copy cylinder and position at end of original cylinder",
	f_copy_inv,3,3,TRUE,
"d", "<objects>", "delete list of objects",
	f_delobj,2,MAXARGS,TRUE,
"db", "command", "database manipulation routines",
	cmd_db, 1, MAXARGS, TRUE,
"dbconcat", "file [prefix]", "concatenate 'file' onto end of present database.  Run 'dup file' first.",
	f_concat, 2, 3, TRUE,
"debugdir", "", "Print in-memory directory, for debugging",
	f_debugdir, 1, 1, TRUE,
"debuglib", "[hex_code]", "Show/set debugging bit vector for librt",
	f_debuglib,1,2,TRUE,
"debugmem", "", "Print librt memory use map",
	f_debugmem, 1, 1, TRUE,
"debugnmg", "[hex code]", "Show/set debugging bit vector for NMG",
	f_debugnmg,1,2,TRUE,
"delay", "sec usec", "Delay for the specified amount of time",
	f_delay,3,3,TRUE,
"dm", "set var [val]", "Do display-manager specific command",
	f_dm, 2, MAXARGS, TRUE,
"dup", "file [prefix]", "check for dup names in 'file'",
	f_dup, 2, 3, TRUE,
"E", "<objects>", "evaluated edit of objects",
	f_evedit,2,MAXARGS,TRUE,
"e", "<objects>", "edit objects",
	f_edit,2,MAXARGS,TRUE,
"eac", "Air_code(s)", "display all regions with given air code",
	f_eac, 2, MAXARGS,TRUE,
"echo", "[text]", "echo arguments back",
	cmd_echo, 1, MAXARGS, TRUE,
"edcodes", "object(s)", "edit region ident codes",
	f_edcodes, 2, MAXARGS, TRUE,
"edmater", "comb(s)", "edit combination materials",
	f_edmater, 2, MAXARGS, TRUE,
"edcolor", "", "text edit color table",
	f_edcolor, 1, 1, TRUE,
"edcomb", "combname Regionflag regionid air los [GIFTmater]", "edit combination record info",
	f_edcomb,6,7,TRUE,
"edgedir", "[delta_x delta_y delta_z]|[rot fb]", "define direction of ARB edge being moved",
	f_edgedir, 3, 4, TRUE,
"ev",	"[-dnqstuvwT] [-P #] <objects>", "evaluate objects via NMG tessellation",
	f_ev, 2, MAXARGS, TRUE,
"eqn", "A B C", "planar equation coefficients",
	f_eqn, 4, 4, TRUE,
"exit", "", "exit",
	f_quit,1,1,TRUE,
"extrude", "#### distance", "extrude dist from face",
	f_extrude,3,3,TRUE,
"expand", "wildcard expression", "expands wildcard expression",
        cmd_expand, 1, MAXARGS, TRUE,
"facedef", "####", "define new face for an arb",
	f_facedef, 2, MAXARGS, TRUE,
"facetize", "[-tT] [-P#] new_obj old_obj(s)", "convert objects to faceted NMG objects at current tol",
	f_facetize, 3, MAXARGS, TRUE,
"find", "<objects>", "find all references to objects",
	f_find, 1, MAXARGS, TRUE,
"fix", "", "fix display after hardware error",
	f_fix,1,1,TRUE,
"fracture", "NMGsolid [prefix]", "fracture an NMG solid into many NMG solids, each containing one face\n",
	f_fracture, 2, 3, TRUE,
"g", "groupname <objects>", "group objects",
	f_group,3,MAXARGS,TRUE,
"getknob", "knobname", "Gets the current setting of the given knob",
        cmd_getknob, 2, 2, TRUE,
"output_hook", "output_hook_name",
       "All output is sent to the Tcl procedure \"output_hook_name\"",
	cmd_output_hook, 1, 2, TRUE,
#ifdef HIDELINE
"H", "plotfile [step_size %epsilon]", "produce hidden-line unix-plot",
	f_hideline,2,4,TRUE,
#endif
"help", "[commands]", "give usage message for given commands",
	f_help,0,MAXARGS,TRUE,
"history", "[-delays]", "list command history",
	f_history, 1, 4,TRUE,
"hist_prev", "", "Returns previous command in history",
        cmd_prev, 1, 1, TRUE,
"hist_next", "", "Returns next command in history",
        cmd_next, 1, 1, TRUE,
"hist_add", "[command]", "Adds command to the history (without executing it)",
        cmd_hist_add, 1, 2, TRUE,
"i", "obj combination [operation]", "add instance of obj to comb",
	f_instance,3,4,TRUE,
"idents", "file object(s)", "make ascii summary of region idents",
	f_tables, 3, MAXARGS, TRUE,
"iknob", "id [val]", "increment knob value",
       f_knob,2,3, TRUE,
"ill", "name", "illuminate object",
	f_ill,2,2,TRUE,
"in", "[-f] [-s] parameters...", "keyboard entry of solids.  -f for no drawing, -s to enter solid edit",
	f_in, 1, MAXARGS, TRUE,
"inside", "", "finds inside solid per specified thicknesses",
	f_inside, 1, MAXARGS, TRUE,
#if 0
"irot", "x y z", "incremental/relative rotate",
        f_irot, 4, 4, TRUE,
#endif
"item", "region item [air]", "change item # or air code",
	f_itemair,3,4,TRUE,
#if 0
"itran", "x y z", "incremental/relative translate using normalized screen coordinates",
        f_tran, 4, 4,TRUE,
#endif
"joint", "command [options]", "articualtion/animation commands",
	f_joint, 1, MAXARGS, TRUE,
"journal", "fileName", "record all commands and timings to journal",
	f_journal, 1, 2, TRUE,
"keep", "keep_file object(s)", "save named objects in specified file",
	f_keep, 3, MAXARGS, TRUE,
"keypoint", "[x y z | reset]", "set/see center of editing transformations",
	f_keypoint,1,4, TRUE,
"kill", "[-f] <objects>", "delete object[s] from file",
	f_kill,2,MAXARGS,TRUE,
"killall", "<objects>", "kill object[s] and all references",
	f_killall, 2, MAXARGS,TRUE,
"killtree", "<object>", "kill complete tree[s] - BE CAREFUL",
	f_killtree, 2, MAXARGS, TRUE,
"knob", "id [val]", "emulate knob twist",
	f_knob,2,3, TRUE,
"l", "<objects>", "list attributes (verbose)",
	cmd_list,2,MAXARGS, TRUE,
"L",  "1|0 xpos ypos", "handle a left mouse event",
	cmd_left_mouse, 4,4, TRUE,
"labelvert", "object[s]", "label vertices of wireframes of objects",
	f_labelvert, 2, MAXARGS, TRUE,
"listeval", "", "lists 'evaluated' path solids",
	f_pathsum, 1, MAXARGS, TRUE,
"load_dv", "", "Initializes the view matrices",
        f_load_dv, 1, 1, TRUE,
"loadtk", "[DISPLAY]", "Initializes Tk window library",
        cmd_tk, 1, 2, TRUE,
"ls", "", "table of contents",
	dir_print,1,MAXARGS, TRUE,
"M", "1|0 xpos ypos", "handle a middle mouse event",
	f_mouse, 4,4, TRUE,
"make", "name <arb8|sph|ellg|tor|tgc|rpc|rhc|epa|ehy|eto|part|grip|half|nmg|pipe>", "create a primitive",
	f_make,3,3,TRUE,
"make_bb", "new_rpp_name obj1_or_path1 [list of objects or paths ...]", "make a bounding box solid enclosing specified objects/paths",
	f_make_bb, 3, MAXARGS, TRUE,
"mater", "comb [material]", "assign/delete material to combination",
	f_mater,2,8,TRUE,
"matpick", "# or a/b", "select arc which has matrix to be edited, in O_PATH state",
	f_matpick, 2,2,TRUE,
"memprint", "", "print memory maps",
	f_memprint, 1, 1,TRUE,
"mirface", "#### axis", "mirror an ARB face",
	f_mirface,3,3,TRUE,
"mirror", "old new axis", "Arb mirror ??",
	f_mirror,4,4,TRUE,
"mv", "old new", "rename object",
	f_name,3,3,TRUE,
"mvall", "oldname newname", "rename object everywhere",
	f_mvall, 3, 3,TRUE,
"nirt", "", "trace a single ray from current view",
	f_nirt,1,MAXARGS,TRUE,
"nmg_simplify", "[arb|tgc|ell|poly] new_solid nmg_solid", "simplify nmg_solid, if possible",
	f_nmg_simplify, 3,4,TRUE,
"oed", "path_lhs path_rhs", "Go from view to object_edit of path_lhs/path_rhs",
	cmd_oed, 3, 3, TRUE,
"opendb", "database.g", "Close current .g file, and open new .g file",
	f_opendb, 2, 3, TRUE,
"orientation", "x y z w", "Set view direction from quaternion",
	f_orientation, 5, 5,TRUE,
"orot", "xdeg ydeg zdeg", "rotate object being edited",
	f_rot_obj, 4, 4,TRUE,
"oscale", "factor", "scale object by factor",
	f_sc_obj,2,2,TRUE,
"overlay", "file.plot [name]", "Read UNIX-Plot as named overlay",
	f_overlay, 2, 3,TRUE,
"p", "dx [dy dz]", "set parameters",
	f_param,2,4,TRUE,
"paths", "pattern", "lists all paths matching input path",
	f_pathsum, 1, MAXARGS,TRUE,
"pathlist", "name(s)", "list all paths from name(s) to leaves",
	cmd_pathlist, 1, MAXARGS,TRUE,
"permute", "tuple", "permute vertices of an ARB",
	f_permute,2,2,TRUE,
"plot", "[-float] [-zclip] [-2d] [-grid] [out_file] [|filter]", "make UNIX-plot of view",
	f_plot, 2, MAXARGS,TRUE,
"pl", "[-float] [-zclip] [-2d] [-grid] [out_file] [|filter]", "Experimental - uses dm-plot:make UNIX-plot of view",
	f_pl, 2, MAXARGS,TRUE,
"polybinout", "file", "store vlist polygons into polygon file (experimental)",
	f_polybinout, 2, 2,TRUE,
"pov", "args", "experimental:  set point-of-view",
	f_pov, 3+4+1, MAXARGS,TRUE,
"prcolor", "", "print color&material table",
	f_prcolor, 1, 1,TRUE,
"prefix", "new_prefix object(s)", "prefix each occurrence of object name(s)",
	f_prefix, 3, MAXARGS,TRUE,
"preview", "[-v] [-d sec_delay] rt_script_file", "preview new style RT animation script",
	f_preview, 2, MAXARGS,TRUE,
"press", "button_label", "emulate button press",
	f_press,2,MAXARGS,TRUE,
"ps", "[-f font] [-t title] [-c creator] [-s size in inches] [-l linewidth] file", "creates a postscript file of the current view",
        f_ps, 2, MAXARGS,TRUE,
"push", "object[s]", "pushes object's path transformations to solids",
	f_push, 2, MAXARGS,TRUE,
"putmat", "a/b {I | m0 m1 ... m16}", "replace matrix on combination's arc",
	f_putmat, 3,MAXARGS,TRUE,
"q", "", "quit",
	f_quit,1,1,TRUE,
"quit", "", "quit",
	f_quit,1,1,TRUE,
"qorot", "x y z dx dy dz theta", "rotate object being edited about specified vector",
	f_qorot, 8, 8,TRUE,
"qvrot", "dx dy dz theta", "set view from direction vector and twist angle",
	f_qvrot, 5, 5,TRUE,
"r", "region <operation solid>", "create or extend a Region combination",
	f_region,4,MAXARGS,TRUE,
"R",  "1|0 xpos ypos", "handle a right mouse event",
	cmd_right_mouse, 4,4, TRUE,
"rcodes", "filename", "read region ident codes from filename",
        f_rcodes, 2, 2, TRUE,
"red", "object", "edit a group or region using a text editor",
	f_red, 2, 2,TRUE,
"refresh", "", "send new control list",
	f_refresh, 1,1,TRUE,
"regdebug", "", "toggle register print",
	f_regdebug, 1,2,TRUE,
"regdef", "item [air] [los] [GIFTmaterial]", "change next region default codes",
	f_regdef, 2, 5,TRUE,
"regions", "file object(s)", "make ascii summary of regions",
	f_tables, 3, MAXARGS,TRUE,
"release", "[name]", "release display processor",
	f_release,1,2,TRUE,
"rfarb", "", "makes arb given point, 2 coord of 3 pts, rot, fb, thickness",
	f_rfarb, 1, 27,TRUE,
"rm", "comb <members>", "remove members from comb",
	f_rm,3,MAXARGS,TRUE,
"rmater", "filename", "read combination materials from filename",
        f_rmater, 2, 2, TRUE,
"rmats", "file", "load views from file (experimental)",
	f_rmats,2,MAXARGS,TRUE,
"rotobj", "xdeg ydeg zdeg", "rotate object being edited",
	f_rot_obj, 4, 4,TRUE,
"rrt", "prog [options]", "invoke prog with view",
	f_rrt,2,MAXARGS,TRUE,
"rt", "[options]", "do raytrace of view",
	f_rt,1,MAXARGS,TRUE,
"rtcheck", "[options]", "check for overlaps in current view",
	f_rtcheck,1,MAXARGS,TRUE,
#if 0
"savedit", "", "save current edit and remain in edit state",
	f_savedit, 1, 1,FALSE,
#endif
"savekey", "file [time]", "save keyframe in file (experimental)",
	f_savekey,2,MAXARGS,TRUE,
"saveview", "file [args]", "save view in file for RT",
	f_saveview,2,MAXARGS,TRUE,
"showmats", "path", "show xform matrices along path",
	f_showmats,2,2,TRUE,
"sed", "<path>", "solid-edit named solid",
	f_sed,2,2,TRUE,
"setview", "x y z", "set the view given angles x, y, and z in degrees",
        f_setview,4,4,TRUE,
"shells", "nmg_model", "breaks model into seperate shells",
	f_shells, 2,2,TRUE,
"shader", "comb material [arg(s)]", "assign materials (like 'mater')",
	f_shader, 3,MAXARGS,TRUE,
"size", "size", "set view size",
	f_view, 2,2,TRUE,
#if 0
"slider", "slider number, value", "adjust sliders using keyboard",
	f_slider, 3,3,FALSE,
#endif
"sliders", "[{on|off}]", "turns the sliders on or off, or reads current state",
        cmd_sliders, 1, 2, TRUE,
"solids", "file object(s)", "make ascii summary of solid parameters",
	f_tables, 3, MAXARGS,TRUE,
"solids_on_ray", "h v", "List all displayed solids along a ray",
        cmd_solids_on_ray, 1, 3, TRUE,
"status", "", "get view status",
	f_status, 1,1,TRUE,
"summary", "[s r g]", "count/list solid/reg/groups",
	f_summary,1,2,TRUE,
"sv", "x y", "Move view center to (x, y)",
	f_slewview, 3, 3,TRUE,
"sync",	"",	"forces UNIX sync",
	f_sync, 1, 1,TRUE,
"t", "", "table of contents",
	dir_print,1,MAXARGS,TRUE,
"tab", "object[s]", "tabulates objects as stored in database",
	f_tabobj, 2, MAXARGS,TRUE,
"ted", "", "text edit a solid's parameters",
	f_tedit,1,1,TRUE,
"tie", "pathName1 pathName2", "tie display manager pathName1 to display manager pathName2",
        f_tie, 3,3,TRUE,
"title", "string", "change the title",
	f_title,1,MAXARGS,TRUE,
"tol", "[abs #] [rel #] [norm #] [dist #] [perp #]", "show/set tessellation and calculation tolerances",
	f_tol, 1, 11,TRUE,
"tops", "", "find all top level objects",
	f_tops,1,1,TRUE,
"track", "<parameters>", "adds tracks to database",
	f_amtrack, 1, 27,TRUE,
#if 0
"tran", "x y z", "absolute translate using view coordinates",
        f_tran, 4, 4,TRUE,
#endif
"translate", "x y z", "trans object to x,y, z",
	f_tr_obj,4,4,TRUE,
"tree",	"object(s)", "print out a tree of all members of an object",
	f_tree, 2, MAXARGS,TRUE,
"units", "[mm|cm|m|in|ft|...]", "change units",
	f_units,1,2,TRUE,
"untie", "pathName", "untie display manager pathName",
        f_untie, 2,2,TRUE,
"vars",	"[var=opt]", "assign/display mged variables",
	f_set,1,2,TRUE,
#if 1
"vdraw", "write|insert|delete|read|length|show [args]", "Expermental drawing (cnuzman)",
	cmd_vdraw, 2, 7, TRUE,
"viewget", "center|size|eye|ypr|quat", "Experimental - return high-precision view parameters.",
	cmd_viewget, 2, 2, TRUE,
"viewset","center|eye|size|ypr|quat|aet","Experimental - set several view parameters at once.",
	cmd_viewset, 3, MAXARGS, TRUE,
#endif
"vrmgr", "host {master|slave|overview}", "link with Virtual Reality manager",
	f_vrmgr, 3, MAXARGS,TRUE,
"vrot", "xdeg ydeg zdeg", "rotate viewpoint",
	f_vrot,4,4,TRUE,
"vrot_center", "v|m x y z", "set center point of viewpoint rotation, in model or view coords",
	f_vrot_center, 5, 5,TRUE,
"wcodes", "filename object(s)", "write region ident codes to filename",
        f_wcodes, 3, MAXARGS, TRUE,
"whatid", "region_name", "display ident number for region",
	f_whatid, 2, MAXARGS,TRUE,
"whichair", "air_codes(s)", "lists all regions with given air code",
	f_which_air, 2, MAXARGS,TRUE,
"whichid", "ident(s)", "lists all regions with given ident code",
	f_which_id, 2, MAXARGS,TRUE,
"who", "[r(eal)|p(hony)|b(oth)]", "list the displayed objects",
	cmd_who,1,2,TRUE,
"winset", "pathname", "sets the current display manager to pathname",
        f_winset, 1, 2, TRUE,
"wmater", "filename comb(s)", "write combination materials to filename",
        f_wmater, 3, MAXARGS, TRUE,
"x", "lvl", "print solid table & vector list",
	f_debug, 1,2,TRUE,
"xpush", "object", "Experimental Push Command",
	f_xpush, 2,2,TRUE,
"Z", "", "zap all objects off screen",
	f_zap,1,1,TRUE,
"zoom", "scale_factor", "zoom view in or out",
	f_zoom, 2,2,TRUE,
0, 0, 0,
	0, 0, 0, 0
};


/*
 *                        O U T P U T _ C A T C H
 *
 * Gets the output from bu_log and appends it to clientdata vls.
 */

HIDDEN int
output_catch(clientdata, str)
genptr_t clientdata;
genptr_t str;
{
    register struct bu_vls *vp = (struct bu_vls *)clientdata;
    register int len;

    BU_CK_VLS(vp);
    len = bu_vls_strlen(vp);
    bu_vls_strcat(vp, str);
    len = bu_vls_strlen(vp) - len;

    return len;
}

/*
 *                 S T A R T _ C A T C H I N G _ O U T P U T
 *
 * Sets up hooks to bu_log so that all output is caught in the given vls.
 *
 */

void
start_catching_output(vp)
struct bu_vls *vp;
{
    bu_add_hook(output_catch, (genptr_t)vp);
}

/*
 *                 S T O P _ C A T C H I N G _ O U T P U T
 *
 * Turns off the output catch hook.
 */

void
stop_catching_output(vp)
struct bu_vls *vp;
{
    bu_delete_hook(output_catch, (genptr_t)vp);
}

/*
 *	T C L _ A P P I N I T
 *
 *	Called by the Tcl/Tk libraries for initialization.
 *	Unncessary in our case; cmd_setup does all the work.
 */


int
Tcl_AppInit(interp)
Tcl_Interp *interp;		/* Interpreter for application. */
{
    return TCL_OK;
}


/*			C M D _ W R A P P E R
 *
 * Translates between MGED's "CMD_OK/BAD/MORE" result codes to ones that
 * Tcl can understand.
 */

int
cmd_wrapper(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
    int status;
    size_t len;
    struct bu_vls result;
    int catch_output;

    argv[0] = ((struct funtab *)clientData)->ft_name;

    /* We now leave the world of Tcl where everything prints its results
       in the interp->result field.  Here, stuff gets printed with the
       bu_log command; hence, we must catch such output and stuff it into
       the result string.  Do this *only* if "output_as_return" global
       variable is set.  Make a local copy of this variable in case it's
       changed by our command. */

    catch_output = output_as_return;
    
    bu_vls_init(&result);

    if (catch_output)
	start_catching_output(&result);
    status = mged_cmd(argc, argv, funtab);
    if (catch_output)
	stop_catching_output(&result);

    /* Remove any trailing newlines. */

    if (catch_output) {
	len = bu_vls_strlen(&result);
	while (len > 0 && bu_vls_addr(&result)[len-1] == '\n')
	    bu_vls_trunc(&result, --len);
    }
    
    switch (status) {
    case CMD_OK:
	if (catch_output)
	    Tcl_SetResult(interp, bu_vls_addr(&result), TCL_VOLATILE);
	status = TCL_OK;
	break;
    case CMD_BAD:
	if (catch_output)
	    Tcl_SetResult(interp, bu_vls_addr(&result), TCL_VOLATILE);
	status = TCL_ERROR;
	break;
    case CMD_MORE:
	Tcl_SetResult(interp, MORE_ARGS_STR, TCL_STATIC);
	if (catch_output) {
	    Tcl_AppendResult(interp, bu_vls_addr(&result), (char *)NULL);
	}
	status = TCL_ERROR;
	break;
    default:
	Tcl_SetResult(interp, "error executing mged routine::", TCL_STATIC);
	if (catch_output) {
	    Tcl_AppendResult(interp, bu_vls_addr(&result), (char *)NULL);
	}
	status = TCL_ERROR;
	break;
    }
    
    bu_vls_free(&result);
    return status;
}

/*
 *                            G U I _ O U T P U T
 *
 * Used as a hook for bu_log output.  Sends output to the Tcl procedure whose
 * name is contained in the vls "tcl_output_hook".  Useful for user interface
 * building.
 */

int
gui_output(clientData, str)
genptr_t clientData;
genptr_t str;
{
    Tcl_DString tclcommand;
    static int level = 0;

    if (level > 50) {
	bu_delete_hook(gui_output, clientData);
	/* Now safe to run bu_log? */
    	bu_log("Ack! Something horrible just happened recursively.\n");
	return 0;
    }

    Tcl_DStringInit(&tclcommand);
    (void)Tcl_DStringAppendElement(&tclcommand, bu_vls_addr(&tcl_output_hook));
    (void)Tcl_DStringAppendElement(&tclcommand, str);

    ++level;
    Tcl_Eval((Tcl_Interp *)clientData, Tcl_DStringValue(&tclcommand));
    --level;

    Tcl_DStringFree(&tclcommand);
    return strlen(str);
}

/*
 *                     C M D _ T K
 *
 *  Command for initializing the Tk window and defaults.
 *
 *  Usage:  loadtk [displayname[.screennum]]
 */
int
cmd_tk(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
  int status;

  if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
    return TCL_ERROR;

  if(argc == 1)
    status = gui_setup((char *)NULL);
  else
    status = gui_setup(argv[1]);

  return status;
}    

/*
 *	G E T K N O B
 *
 *	Procedure called by the Tcl/Tk interface code to find the values
 *	of the knobs/sliders.
 */

int
cmd_getknob(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
    register int i;
#if 1
    static struct {
	char *knobname;
	fastf_t variable;
    } knobs[] = {
	"ax", 0, 
	"ay", 0,
	"az", 0,
	"aX", 0,
	"aY", 0,
	"aZ", 0,
	"aS", 0,
	"x", 0,
	"y", 0,
	"z", 0,
	"X", 0,
	"Y", 0,
	"Z", 0,
	"S", 0,
	"xadc", 0,
	"yadc", 0,
	"ang1", 0,
	"ang2", 0,
	"distadc", 0,
    };

    if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
      return TCL_ERROR;

    knobs[0].variable = absolute_rotate[X];
    knobs[1].variable = absolute_rotate[Y];
    knobs[2].variable = absolute_rotate[Z];
    knobs[3].variable = absolute_slew[X];
    knobs[4].variable = absolute_slew[Y];
    knobs[5].variable = absolute_slew[Z];
    knobs[6].variable = absolute_zoom;
    knobs[7].variable = rate_rotate[X];
    knobs[8].variable = rate_rotate[Y];
    knobs[9].variable = rate_rotate[Z];
    knobs[10].variable = rate_slew[X];
    knobs[11].variable = rate_slew[Y];
    knobs[12].variable = rate_slew[Z];
    knobs[13].variable = rate_zoom;
    knobs[14].variable = (fastf_t)dv_xadc;
    knobs[15].variable = (fastf_t)dv_yadc;
    knobs[16].variable = (fastf_t)dv_1adc;
    knobs[17].variable = (fastf_t)dv_2adc;
    knobs[18].variable = (fastf_t)dv_distadc;
	
    if( argc < 2 ) {
	Tcl_AppendResult(interp, "getknob: need a knob name", (char *)NULL);
	return TCL_ERROR;
    }

    for (i = 0; i < 19; ++i)
      if (strcmp(knobs[i].knobname, argv[1]) == 0) {
	sprintf(interp->result, "%lf", knobs[i].variable);
	return TCL_OK;
      }
#else
    static struct {
	char *knobname;
	fastf_t *variable;
    } knobs[] = {
	"ax", (fastf_t *)NULL, 
	"ay", (fastf_t *)NULL,
	"az", (fastf_t *)NULL,
	"aX", (fastf_t *)NULL,
	"aY", (fastf_t *)NULL,
	"aZ", (fastf_t *)NULL,
	"aS", (fastf_t *)NULL,
	"x", (fastf_t *)NULL,
	"y", (fastf_t *)NULL,
	"z", (fastf_t *)NULL,
	"X", (fastf_t *)NULL,
	"Y", (fastf_t *)NULL,
	"Z", (fastf_t *)NULL,
	"S", (fastf_t *)NULL,
	"xadc", (fastf_t *)NULL,
	"yadc", (fastf_t *)NULL,
	"ang1", (fastf_t *)NULL,
	"ang2", (fastf_t *)NULL,
	"distadc", (fastf_t *)NULL,
    };

    if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
      return TCL_ERROR;

    knobs[0].variable = &absolute_rotate[X];
    knobs[1].variable = &absolute_rotate[Y];
    knobs[2].variable = &absolute_rotate[Z];
    knobs[3].variable = &absolute_slew[X];
    knobs[4].variable = &absolute_slew[Y];
    knobs[5].variable = &absolute_slew[Z];
    knobs[6].variable = &absolute_zoom;
    knobs[7].variable = &rate_rotate[X];
    knobs[8].variable = &rate_rotate[Y];
    knobs[9].variable = &rate_rotate[Z];
    knobs[10].variable = &rate_slew[X];
    knobs[11].variable = &rate_slew[Y];
    knobs[12].variable = &rate_slew[Z];
    knobs[13].variable = &rate_zoom;
    knobs[14].variable = (fastf_t *)&dv_xadc;
    knobs[15].variable = (fastf_t *)&dv_yadc;
    knobs[16].variable = (fastf_t *)&dv_1adc;
    knobs[17].variable = (fastf_t *)&dv_2adc;
    knobs[18].variable = (fastf_t *)&dv_distadc;
	
    if( argc < 2 ) {
	Tcl_AppendResult(interp, "getknob: need a knob name", (char *)NULL);
	return TCL_ERROR;
    }

    for (i = 0; i < 19; ++i)
      if (strcmp(knobs[i].knobname, argv[1]) == 0) {
	sprintf(interp->result, "%lf", *(knobs[i].variable));
	return TCL_OK;
      }
#endif    
    Tcl_AppendResult(interp, "getknob: invalid knob name", (char *)NULL);
    return TCL_ERROR;
}

/*
 *   C M D _ O U T P U T _ H O O K
 *
 *   Hooks the output to the given output hook.
 *   Removes the existing output hook!
 */

int
cmd_output_hook(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;    
int argc;
char **argv;
{
    struct bu_vls infocommand;
    int status;

    if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
      return TCL_ERROR;

    if (argc > 2) {
	Tcl_AppendResult(interp,
			 "too many args: should be \"output_hook [hookName]\"",
			 (char *)NULL);
	return TCL_ERROR;
    }

    bu_delete_hook(gui_output, (genptr_t)interp);/* Delete the existing hook */

    if (argc < 2)
	return TCL_OK;

    /* Make sure the command exists before putting in the hook! */
    
    bu_vls_init(&infocommand);
    bu_vls_strcat(&infocommand, "info commands ");
    bu_vls_strcat(&infocommand, argv[1]);
    status = Tcl_Eval(interp, bu_vls_addr(&infocommand));
    bu_vls_free(&infocommand);

    if (status != TCL_OK || interp->result[0] == '\0') {
      Tcl_AppendResult(interp, "command does not exist", (char *)NULL);
      return TCL_ERROR;
    }

    /* Also, don't allow silly infinite loops. */

    if (strcmp(argv[1], argv[0]) == 0) {
	Tcl_AppendResult(interp, "Don't be silly.", (char *)NULL);
	return TCL_ERROR;
    }

    /* Set up the hook! */

    bu_vls_strcpy(&tcl_output_hook, argv[1]);
    bu_add_hook(gui_output, (genptr_t)interp);
    
    Tcl_ResetResult(interp);
    return TCL_OK;
}


/*
 *			C M D _ N O P
 */
int
cmd_nop(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
    return TCL_OK;
}


/*
 *
 * Sets up the Tcl interpreter
 */ 
void
mged_setup()
{
  struct bu_vls str;
  char *filename;

  /* The following is for GUI output hooks: contains name of function to
     run with output */
  bu_vls_init(&tcl_output_hook);

  /* Create the interpreter */
  interp = Tcl_CreateInterp();

  /* This runs the init.tcl script */
  if( Tcl_Init(interp) == TCL_ERROR )
    bu_log("Tcl_Init error %s\n", interp->result);

  /* register commands */
  cmd_setup();

  /* Initialize the menu mechanism to be off, but ready. */
  mmenu_init();
  btn_head_menu(0,0,0);		/* unlabeled menu */

  history_setup();
  mged_variable_setup(interp);

#ifdef MGED_LIBRARY
  bu_vls_init(&str);
  filename = getenv("MGED_TCL_LIBRARY");
  bu_vls_printf(&str, "set auto_path \\[linsert $auto_path 0 %s\\];\
set junk \"\"", filename ? filename : MGED_LIBRARY);
  (void)cmdline(&str, False);
  bu_vls_free(&str);
  Tcl_ResetResult(interp);
#endif
}

/* 			C M D _ S E T U P
 * Register all the MGED commands.
 */
void
cmd_setup()
{
    register struct funtab *ftp;
    struct bu_vls temp;

    bu_vls_init(&temp);

    for (ftp = funtab+1; ftp->ft_name != NULL; ftp++) {
#if 0
	bu_vls_strcpy(&temp, "info commands ");
	bu_vls_strcat(&temp, ftp->ft_name);
	if (Tcl_Eval(interp, bu_vls_addr(&temp)) != TCL_OK ||
	    interp->result[0] != '\0') {
	    bu_log("WARNING:  '%s' name collision (%s)\n", ftp->ft_name,
		   interp->result);
	}
#endif
	bu_vls_strcpy(&temp, "_mged_");
	bu_vls_strcat(&temp, ftp->ft_name);
	
	if (ftp->tcl_converted) {
	    (void)Tcl_CreateCommand(interp, ftp->ft_name, ftp->ft_func,
				   (ClientData)ftp, (Tcl_CmdDeleteProc *)NULL);
	    (void)Tcl_CreateCommand(interp, bu_vls_addr(&temp), ftp->ft_func,
				   (ClientData)ftp, (Tcl_CmdDeleteProc *)NULL);
	} else {
#if 0
	    (void)Tcl_CreateCommand(interp, ftp->ft_name, cmd_wrapper, 	    
			           (ClientData)ftp, (Tcl_CmdDeleteProc *)NULL);
	    (void)Tcl_CreateCommand(interp, bu_vls_addr(&temp), cmd_wrapper,
				   (ClientData)ftp, (Tcl_CmdDeleteProc *)NULL);
#else
	    bu_log("cmd_setup: %s needs to be Tcl converted\n", ftp->ft_name);
#endif
	}
    }

#if 1
    (void)Tcl_CreateCommand(interp, "mged_glob", cmd_mged_glob, (ClientData)NULL,
			    (Tcl_CmdDeleteProc *)NULL);
    (void)Tcl_CreateCommand(interp, "cmd_init", cmd_init, (ClientData)NULL,
			    (Tcl_CmdDeleteProc *)NULL);
    (void)Tcl_CreateCommand(interp, "cmd_set", cmd_set, (ClientData)NULL,
			    (Tcl_CmdDeleteProc *)NULL);
    (void)Tcl_CreateCommand(interp, "cmd_get", cmd_get, (ClientData)NULL,
			    (Tcl_CmdDeleteProc *)NULL);
    (void)Tcl_CreateCommand(interp, "get_more_default", get_more_default, (ClientData)NULL,
			    (Tcl_CmdDeleteProc *)NULL);
    (void)Tcl_CreateCommand(interp, "stuff_str", cmd_stuff_str, (ClientData)NULL,
			    (Tcl_CmdDeleteProc *)NULL);
#endif

#if 0
    /* Link to some internal variables */
    Tcl_LinkVar(interp, "mged_center_x", (char *)&toViewcenter[MDX],
		TCL_LINK_DOUBLE);
    Tcl_LinkVar(interp, "mged_center_y", (char *)&toViewcenter[MDY],
		TCL_LINK_DOUBLE);
    Tcl_LinkVar(interp, "mged_center_z", (char *)&toViewcenter[MDZ],
		TCL_LINK_DOUBLE);

#endif

    Tcl_LinkVar(interp, "glob_compat_mode", (char *)&glob_compat_mode,
		TCL_LINK_BOOLEAN);
    Tcl_LinkVar(interp, "output_as_return", (char *)&output_as_return,
		TCL_LINK_BOOLEAN);

	math_setup();

    bu_vls_free(&temp);
    tkwin = NULL;
}


/* initialize a new command window */
int
cmd_init(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
  struct cmd_list *clp;
  int name_not_used = 1;

  if(argc != 3){
    Tcl_AppendResult(interp, "Usage: cmd_init name id", (char *)NULL);
    return TCL_ERROR;
  }

#if 1
  /* First, search to see if there exists a command window with the name
     in argv[1] */
  for( BU_LIST_FOR(clp, cmd_list, &head_cmd_list.l) )
    if(!strcmp(argv[1], bu_vls_addr(&clp->name))){
      name_not_used = 0;
      break;
    }

  if(name_not_used){
    clp = (struct cmd_list *)bu_malloc(sizeof(struct cmd_list), "cmd_list");
    bzero((void *)clp, sizeof(struct cmd_list));
    BU_LIST_APPEND(&head_cmd_list.l, &clp->l);
    clp->cur_hist = head_cmd_list.cur_hist;
    bu_vls_init(&clp->more_default);
    bu_vls_init(&clp->name);
    bu_vls_strcpy(&clp->name, argv[1]);
  }else{
    clp->cur_hist = head_cmd_list.cur_hist;

    if(clp->aim != NULL){
      clp->aim->aim = CMD_LIST_NULL;
      clp->aim = DM_LIST_NULL;
    }
  }

#else

  clp = (struct cmd_list *)bu_malloc(sizeof(struct cmd_list), "cmd_list");
  bzero((void *)clp, sizeof(struct cmd_list));
  BU_LIST_APPEND(&head_cmd_list.l, &clp->l);
  clp->cur_hist = head_cmd_list.cur_hist;
  bu_vls_init(&clp->more_default);
  bu_vls_init(&clp->name);
  bu_vls_strcpy(&clp->name, argv[1]);

#endif

  return TCL_OK;
}


/* returns a list of ids associated with the current command window */
int
cmd_get(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
  struct dm_list *p;

  /* The current command window is not tied to a display manager so,
     simply return the id of the current command window */
  if(!curr_cmd_list->aim){
    Tcl_AppendElement(interp, bu_vls_addr(&curr_cmd_list->name));
    return TCL_OK;
  }

  /* return all ids associated with the current command window */
  for( BU_LIST_FOR(p, dm_list, &head_dm_list.l) ){
    /* The display manager tied to the current command window shares
       information with display manager p */
    if(curr_cmd_list->aim->s_info == p->s_info)
      /* This display manager is tied to a command window */
      if(p->aim)
	Tcl_AppendElement(interp, bu_vls_addr(&p->aim->name));
  }

  return TCL_OK;
}


/* given an id sets the current command window */
int
cmd_set(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
  struct cmd_list *p;

  if(argc != 2){
    Tcl_AppendResult(interp, "Usage: cmd_set id", (char *)NULL);
    return TCL_ERROR;
  }

  for( BU_LIST_FOR(p, cmd_list, &head_cmd_list.l) ){
    if(strcmp(bu_vls_addr(&p->name), argv[1]))
      continue;

    curr_cmd_list = p;
    break;
  }

  if(p == &head_cmd_list)
    curr_cmd_list = &head_cmd_list;

  if(curr_cmd_list->aim)
    curr_dm_list = curr_cmd_list->aim;

  bu_vls_trunc(&curr_cmd_list->more_default, 0);
  return TCL_OK;
}

int
get_more_default(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
  struct cmd_list *p;

  if(argc != 1){
    Tcl_AppendResult(interp, "Usage: get_more_default", (char *)NULL);
    return TCL_ERROR;
  }

  Tcl_AppendResult(interp, bu_vls_addr(&curr_cmd_list->more_default), (char *)NULL);
  return TCL_OK;
}


int
cmd_mged_glob(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
  struct bu_vls dest, src;

  if(argc != 2){
    Tcl_AppendResult(interp, "cmd_mged_glob: There must be only one argument.", (char *)NULL);
    return TCL_ERROR;
  }

  bu_vls_init(&src);
  bu_vls_init(&dest);
  bu_vls_strcpy(&src, argv[1]);
  mged_compat( &dest, &src );
  Tcl_AppendResult(interp, bu_vls_addr(&dest), (char *)NULL);
  bu_vls_free(&src);
  bu_vls_free(&dest);

  return TCL_OK;
}


/*
 * debackslash, backslash_specials, mged_compat: routines for original
 *   mged emulation mode
 */

void
debackslash( dest, src )
struct bu_vls *dest, *src;
{
    char *ptr;

    ptr = bu_vls_addr(src);
    while( *ptr ) {
	if( *ptr == '\\' )
	    ++ptr;
	if( *ptr == '\0' )
	    break;
	bu_vls_putc( dest, *ptr++ );
    }
}

void
backslash_specials( dest, src )
struct bu_vls *dest, *src;
{
    int backslashed;
    char *ptr, buf[2];

    buf[1] = '\0';
    backslashed = 0;
    for( ptr = bu_vls_addr( src ); *ptr; ptr++ ) {
	if( *ptr == '[' && !backslashed )
	    bu_vls_strcat( dest, "\\[" );
	else if( *ptr == ']' && !backslashed )
	    bu_vls_strcat( dest, "\\]" );
	else if( backslashed ) {
	    bu_vls_strcat( dest, "\\" );
	    buf[0] = *ptr;
	    bu_vls_strcat( dest, buf );
	    backslashed = 0;
	} else if( *ptr == '\\' )
	    backslashed = 1;
	else {
	    buf[0] = *ptr;
	    bu_vls_strcat( dest, buf );
	}
    }
}

/*                    M G E D _ C O M P A T
 *
 * This routine is called to perform wildcard expansion and character quoting
 * on the given vls (typically input from the keyboard.)
 */

void
mged_compat( dest, src )
struct bu_vls *dest, *src;
{
    char *start, *end;          /* Start and ends of words */
    int regexp;                 /* Set to TRUE when word is a regexp */
    int backslashed;
    int firstword;
    struct bu_vls word;         /* Current word being processed */
    struct bu_vls temp;

    bu_vls_init( &word );
    bu_vls_init( &temp );
    
    start = end = bu_vls_addr( src );
    firstword = 1;
    while( *end != '\0' ) {            /* Run through entire string */

	/* First, pass along leading whitespace. */

	start = end;                   /* Begin where last word ended */
	while( *start != '\0' ) {
	    if( *start == ' '  ||
	        *start == '\t' ||
	        *start == '\n' )
		bu_vls_putc( dest, *start++ );
	    else
		break;
	}
	if( *start == '\0' )
	    break;

	/* Next, advance "end" pointer to the end of the word, while adding
	   each character to the "word" vls.  Also make a note of any
	   unbackslashed wildcard characters. */

	end = start;
	bu_vls_trunc( &word, 0 );
	regexp = 0;
	backslashed = 0;
	while( *end != '\0' ) {
	    if( *end == ' '  ||
	        *end == '\t' ||
		*end == '\n' )
		break;
	    if( (*end == '*' || *end == '?' || *end == '[') && !backslashed )
		regexp = 1;
	    if( *end == '\\' && !backslashed )
		backslashed = 1;
	    else
		backslashed = 0;
	    bu_vls_putc( &word, *end++ );
	}

	if( firstword )
	    regexp = 0;

	/* Now, if the word was suspected of being a wildcard, try to match
	   it to the database. */

	if( regexp ) {
	    bu_vls_trunc( &temp, 0 );
	    if( regexp_match_all(&temp, bu_vls_addr(&word)) == 0 ) {
		debackslash( &temp, &word );
		backslash_specials( dest, &temp );
	    } else
		bu_vls_vlscat( dest, &temp );
	} else {
	    debackslash( dest, &word );
	}

	firstword = 0;
    }

    bu_vls_free( &temp );
    bu_vls_free( &word );
}
    
/*
 *			C M D L I N E
 *
 *  This routine is called to process a vls full of commands.
 *  Each command is newline terminated.
 *  The input string will not be altered in any way.
 *
 *  Returns -
 *	!0	when a prompt needs to be printed.
 *	 0	no prompt needed.
 */

int
cmdline(vp, record)
struct bu_vls *vp;
int record;
{
    int	status;
    struct bu_vls globbed;
    struct timeval start, finish;
    size_t len;
    extern struct bu_vls mged_prompt;
    char *cp;

    BU_CK_VLS(vp);

    if (bu_vls_strlen(vp) <= 0)
      return CMD_OK;
		
    bu_vls_init(&globbed);


    /* MUST MAKE A BACKUP OF THE INPUT STRING AND USE THAT IN THE CALL TO
       Tcl_Eval!!!
       
       You never know who might change the string (append to it...)
       (f_mouse is notorious for adding things to the input string)
       If it were to change while it was still being evaluated, Horrible Things
       could happen.
    */

    
    if (glob_compat_mode)
	mged_compat(&globbed, vp);
    else
	bu_vls_vlscat(&globbed, vp);

    gettimeofday(&start, (struct timezone *)NULL);
    status = Tcl_Eval(interp, bu_vls_addr(&globbed));
    gettimeofday(&finish, (struct timezone *)NULL);

    /* Contemplate the result reported by the Tcl interpreter. */

    switch (status) {
    case TCL_OK:
      if( setjmp( jmp_env ) == 0 ){
	(void)signal( SIGINT, sig3);  /* allow interupts */
	len = strlen(interp->result);

	/* If the command had something to say, print it out. */	     
	if (len > 0)
	  bu_log("%s%s", interp->result,
		 interp->result[len-1] == '\n' ? "" : "\n");

	/* A user typed this command so let everybody see, then record
	   it in the history. */
	if (record && tkwin != NULL) {
	  struct bu_vls tmp_vls;

	  bu_vls_init(&tmp_vls);
	  bu_vls_printf(&tmp_vls, "distribute_text {} {%s} {%s}",
			bu_vls_addr(&globbed), interp->result);
	  Tcl_Eval(interp, bu_vls_addr(&tmp_vls));
	  Tcl_SetResult(interp, "", TCL_STATIC);
	  bu_vls_free(&tmp_vls);
	}
      }else
	bu_log("\n");
      

      if(record)
	history_record(vp, &start, &finish, CMD_OK);

      bu_vls_free(&globbed);
      bu_vls_strcpy(&mged_prompt, MGED_PROMPT);
      return CMD_OK;

    case TCL_ERROR:
    default:

    /* First check to see if it's a secret message from cmd_wrapper. */

	if ((cp = strstr(interp->result, MORE_ARGS_STR)) != NULL) {
	    if(cp == interp->result){
	      bu_vls_trunc(&mged_prompt, 0);
	      bu_vls_printf(&mged_prompt, "\r%s",
			    interp->result+sizeof(MORE_ARGS_STR)-1);
	    }else{
	      len = cp - interp->result;
	      bu_log("%*s%s", len, interp->result, interp->result[len-1] == '\n' ? "" : "\n");
	      bu_vls_trunc(&mged_prompt, 0);
	      bu_vls_printf(&mged_prompt, "\r%s",
			    interp->result+sizeof(MORE_ARGS_STR)-1+len);
	    }

	    bu_vls_free(&globbed);
	    return CMD_MORE;
	}

    /* Otherwise, it's just a regular old error. */    

	len = strlen(interp->result);
	if (len > 0) bu_log("%s%s", interp->result,
			    interp->result[len-1] == '\n' ? "" : "\n");

	if (record) history_record(vp, &start, &finish, CMD_BAD);
	bu_vls_free(&globbed);
	bu_vls_strcpy(&mged_prompt, MGED_PROMPT);
	return CMD_BAD;
    }
}

/*
 *			M G E D _ C M D
 *
 *  Check a table for the command, check for the correct minimum and maximum
 *  number of arguments, and pass control to the proper function.  If the
 *  number of arguments is incorrect, print out a short help message.
 */

int
mged_cmd(argc, argv, in_functions)
int argc;
char **argv;
struct funtab in_functions[];
{
    register struct funtab *ftp;
    struct funtab *functions;

    if (argc == 0)
	return CMD_OK;	/* No command entered, that's fine */

    /* if no function table is provided, use the default mged function table */
    if( in_functions == (struct funtab *)NULL )
	functions = funtab;
    else
	functions = in_functions;

    for (ftp = &functions[1]; ftp->ft_name; ftp++) {
	if (strcmp(ftp->ft_name, argv[0]) != 0)
	    continue;
	/* We have a match */
	if ((ftp->ft_min <= argc) && (argc <= ftp->ft_max)) {
	    /* Input has the right number of args.
	     * Call function listed in table, with
	     * main(argc, argv) style args
	     */

	    switch (ftp->ft_func(argc, argv)) {
	    case CMD_OK:
		return CMD_OK;
	    case CMD_BAD:
		return CMD_BAD;
	    case CMD_MORE:
		return CMD_MORE;
	    default:
	      Tcl_AppendResult(interp, "mged_cmd(): Invalid return from ",
			       ftp->ft_name, "\n", (char *)NULL);
	      return CMD_BAD;
	    }
	}

	Tcl_AppendResult(interp, "Usage: ", functions[0].ft_name, ftp->ft_name,
			 " ", ftp->ft_parms, "\n\t(", ftp->ft_comment,
			 ")\n", (char *)NULL);
	return CMD_BAD;
    }

    Tcl_AppendResult(interp, functions[0].ft_name, argv[0],
		     ": no such command, type '", functions[0].ft_name,
		     "?' for help\n", (char *)NULL);
    return CMD_BAD;
}

int
mged_cmd_arg_check(argc, argv, in_functions)
int argc;
char **argv;
struct funtab in_functions[];
{
    register struct funtab *ftp;
    struct funtab *functions;
    char *cp;

    if(argc == 0)
      return 0; /* Good */

    /* if no function table is provided, use the default mged function table */
    if( in_functions == (struct funtab *)NULL )
	functions = funtab;
    else
    	functions = in_functions;

    if((cp = strstr(argv[0], "_mged_")) == argv[0])
      cp = cp + 6;
    else
      cp = argv[0];

    for (ftp = &functions[1]; ftp->ft_name; ftp++) {
	if (strcmp(ftp->ft_name, cp) != 0)
	    continue;

	/* We have a match */
	if ((ftp->ft_min <= argc) && (argc <= ftp->ft_max))
	  return 0;  /* Good */

	Tcl_AppendResult(interp, "Usage: ", functions[0].ft_name, ftp->ft_name,
			 " ", ftp->ft_parms, "\n\t(", ftp->ft_comment,
			 ")\n", (char *)NULL);

	return 1;    /* Bad */
    }

    return 0; /* Good */
}



/*
 *               C M D _ A P R O P O S
 *
 * Returns a list of commands whose descriptions contain the given keyword
 * contained in argv[1].  This version is case-sensitive.
 */

int
cmd_apropos(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
    register struct funtab *ftp;
    char *keyword;

    if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
      return TCL_ERROR;

    if( argc < 2 )  {
      Tcl_AppendResult(interp, "apropos: insufficient args", (char *)NULL);
      return TCL_ERROR;
    }

    keyword = argv[1];
    if (keyword == NULL)
	keyword = "";
    
    for (ftp = funtab+1; ftp->ft_name != NULL; ftp++)
	if (strstr(ftp->ft_name, keyword) != NULL ||
	    strstr(ftp->ft_parms, keyword) != NULL ||
	    strstr(ftp->ft_comment, keyword) != NULL)
	    Tcl_AppendElement(interp, ftp->ft_name);
	
    return TCL_OK;
}


/* Let the user temporarily escape from the editor */
/* Format: %	*/

int
f_comm(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int	argc;
char	**argv;
{

	register int pid, rpid;
	int retcode;

	if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
	  return TCL_ERROR;

	(void)signal( SIGINT, SIG_IGN );
	if ( ( pid = fork()) == 0 )  {
		(void)signal( SIGINT, SIG_DFL );
		(void)execl("/bin/sh","-",(char *)NULL);
		perror("/bin/sh");
		mged_finish( 11 );
	}

	while ((rpid = wait(&retcode)) != pid && rpid != -1)
		;

#if 0
	(void)signal(SIGINT, cur_sigint);
#endif
	Tcl_AppendResult(interp, "!\n", (char *)NULL);

	return TCL_OK;
}

/* Quit and exit gracefully */
/* Format: q	*/

int
f_quit(clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int	argc;
char	**argv;
{
  if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
    return TCL_ERROR;

  if( state != ST_VIEW )
    button( BE_REJECT );

  quit();			/* Exiting time */
  /* NOTREACHED */
}

/* wrapper for sync() */

int
f_sync(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
    register int i;

    if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
      return TCL_ERROR;

    sync();
    
    return TCL_OK;
}

/*
 *			H E L P C O M M
 *
 *  Common code for help commands
 */

static int
helpcomm( argc, argv, functions)
int	argc;
char	**argv;
struct funtab *functions;
{
  register struct funtab *ftp;
  register int	i, bad;

  bad = 0;
	
  /* Help command(s) */
  for( i=1; i<argc; i++ )  {
    for( ftp = functions+1; ftp->ft_name; ftp++ )  {
      if( strcmp( ftp->ft_name, argv[i] ) != 0 )
	continue;

      Tcl_AppendResult(interp, "Usage: ", functions->ft_name, ftp->ft_name,
		       " ", ftp->ft_parms, "\n\t(", ftp->ft_comment, ")\n", (char *)NULL);
      break;
    }
    if( !ftp->ft_name ) {
      Tcl_AppendResult(interp, functions->ft_name, argv[i],
		       ": no such command, type '", functions->ft_name,
		       "?' for help\n", (char *)NULL);
      bad = 1;
    }
  }

  return bad ? TCL_ERROR : TCL_OK;
}

/*
 *			F _ H E L P
 *
 *  Print a help message, two lines for each command.
 *  Or, help with the indicated commands.
 */
int f_help2();

int
f_help(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int	argc;
char	**argv;
{
#if 0
	/* There needs to be a better way to trigger this */
	if( argc <= 1 )  {
		/* User typed just "help" */
		system("Mosaic http://ftp.arl.mil/ftp/brl-cad/html/mged &");
	}
#endif
	if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
	  return TCL_ERROR;

	return f_help2(argc, argv, &funtab[0]);
}

int
f_help2(argc, argv, functions)
int argc;
char **argv;
struct funtab *functions;
{
	register struct funtab *ftp;

	if( argc <= 1 )  {
	  Tcl_AppendResult(interp, "The following commands are available:\n", (char *)NULL);
	  for( ftp = functions+1; ftp->ft_name; ftp++ )  {
	    Tcl_AppendResult(interp,  functions->ft_name, ftp->ft_name, " ",
			     ftp->ft_parms, "\n\t(", ftp->ft_comment, ")\n", (char *)NULL);
	  }
	  return TCL_OK;
	}
	return helpcomm( argc, argv, functions );
}

/*
 *			F _ F H E L P
 *
 *  Print a fast help message;  just tabulate the commands available.
 */
int
cmd_fhelp( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int	argc;
char	**argv;
{
	register struct funtab *ftp;
	struct bu_vls		str;

	bu_vls_init(&str);
	for( ftp = &funtab[1]; ftp->ft_name; ftp++ )  {
		vls_col_item( &str, ftp->ft_name);
	}
	vls_col_eol( &str );

	/* XXX should be bu_vls_strgrab() */
	Tcl_AppendResult(interp, bu_vls_strdup( &str), (char *)NULL);

	bu_vls_free(&str);
	return TCL_OK;
}

int
f_fhelp2( argc, argv, functions)
int	argc;
char	**argv;
struct funtab *functions;
{
	register struct funtab *ftp;
	struct bu_vls		str;

	if( argc <= 1 )  {
	  bu_vls_init(&str);
	  Tcl_AppendResult(interp, "The following ", functions->ft_name,
			   " commands are available:\n", (char *)NULL);
	  for( ftp = functions+1; ftp->ft_name; ftp++ )  {
	    vls_col_item( &str, ftp->ft_name);
	  }
	  vls_col_eol( &str );
	  Tcl_AppendResult(interp, bu_vls_addr( &str ), (char *)NULL);
	  bu_vls_free(&str);
	  return TCL_OK;
	}
	return helpcomm( argc, argv, functions );
}

/* Hook for displays with no buttons */
int
f_press(clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int	argc;
char	**argv;
{
  register int i;

  if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
    return TCL_ERROR;

  for( i = 1; i < argc; i++ )
    press( argv[i] );

  return TCL_OK;
}

int
f_summary(clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int	argc;
char	**argv;
{
	register char *cp;
	int flags = 0;
	int bad;

	if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
	  return TCL_ERROR;

	bad = 0;
	if( argc <= 1 )  {
		dir_summary(0);
		return TCL_OK;
	}
	cp = argv[1];
	while( *cp )  switch( *cp++ )  {
		case 's':
			flags |= DIR_SOLID;
			break;
		case 'r':
			flags |= DIR_REGION;
			break;
		case 'g':
			flags |= DIR_COMB;
			break;
		default:
		   Tcl_AppendResult(interp, "summary:  S R or G are only valid parmaters\n",
				    (char *)NULL);
		   bad = 1;
		   break;
	}

	dir_summary(flags);
	return bad ? TCL_ERROR : TCL_OK;
}

/*
 *                          C M D _ E C H O
 *
 * Concatenates its arguments and "bu_log"s the resulting string.
 */

int
cmd_echo(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int	argc;
char	*argv[];
{
    register int i;

    if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
      return TCL_ERROR;

    for( i=1; i < argc; i++ )  {
      Tcl_AppendResult(interp, i==1 ? "" : " ", argv[i], (char *)NULL);
    }

    Tcl_AppendResult(interp, "\n", (char *)NULL);

    return TCL_OK;
}

#if 0
int
f_savedit(argc, argv)
int	argc;
char	*argv[];
{
  struct bu_vls str;
  char	line[35];
  int o_ipathpos;
  register struct solid *o_illump;

  o_illump = illump;
  bu_vls_init(&str);

  if(state == ST_S_EDIT){
    bu_vls_strcpy( &str, "press accept\npress sill\n" );
    cmdline(&str, 0);
    illump = o_illump;
    bu_vls_strcpy( &str, "M 1 0 0\n");
    cmdline(&str, 0);
    return CMD_OK;
  }else if(state == ST_O_EDIT){
    o_ipathpos = ipathpos;
    bu_vls_strcpy( &str, "press accept\npress oill\n" );
    cmdline(&str, 0);
    (void)chg_state( ST_O_PICK, ST_O_PATH, "savedit");
    illump = o_illump;
    ipathpos = o_ipathpos;
    bu_vls_strcpy( &str, "M 1 0 0\n");
    cmdline(&str, 0);
    return CMD_OK;
  }

  bu_log( "Savedit will only work in an edit state\n");
  bu_vls_free(&str);
  return CMD_BAD;
}
#endif

int
f_aim(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char *argv[];
{
  struct cmd_list *p_cmd;
  struct dm_list *p_dm;

  if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
        return TCL_ERROR;

  if(argc == 1){
    for( BU_LIST_FOR(p_cmd, cmd_list, &head_cmd_list.l) )
      if(p_cmd->aim)
	Tcl_AppendResult(interp, bu_vls_addr(&p_cmd->name), " ---> ",
			 bu_vls_addr(&p_cmd->aim->_dmp->dmr_pathName),
			 "\n", (char *)NULL);
      else
	Tcl_AppendResult(interp, bu_vls_addr(&p_cmd->name), " ---> ",
			 "\n", (char *)NULL);

    if(p_cmd->aim)
      Tcl_AppendResult(interp, bu_vls_addr(&p_cmd->name), " ---> ",
		       bu_vls_addr(&p_cmd->aim->_dmp->dmr_pathName),
		       "\n", (char *)NULL);
    else
      Tcl_AppendResult(interp, bu_vls_addr(&p_cmd->name), " ---> ",
		       "\n", (char *)NULL);

    return TCL_OK;
  }

  for( BU_LIST_FOR(p_cmd, cmd_list, &head_cmd_list.l) )
    if(!strcmp(bu_vls_addr(&p_cmd->name), argv[1]))
      break;

  if(p_cmd == &head_cmd_list &&
     (strcmp(bu_vls_addr(&head_cmd_list.name), argv[1]))){
    Tcl_AppendResult(interp, "f_aim: unrecognized command_window - ", argv[1],
		     "\n", (char *)NULL);
    return TCL_ERROR;
  }

  /* print out the display manager being aimed at */
  if(argc == 2){
    if(p_cmd->aim)
      Tcl_AppendResult(interp, bu_vls_addr(&p_cmd->name), " ---> ",
		       bu_vls_addr(&p_cmd->aim->_dmp->dmr_pathName),
		       "\n", (char *)NULL);
    else
      Tcl_AppendResult(interp, bu_vls_addr(&p_cmd->name), " ---> ", "\n", (char *)NULL);

    return TCL_OK;
  }

  for( BU_LIST_FOR(p_dm, dm_list, &head_dm_list.l) )
    if(!strcmp(argv[2], bu_vls_addr(&p_dm->_dmp->dmr_pathName)))
      break;

  if(p_dm == &head_dm_list &&
     strcmp(argv[2], bu_vls_addr(&head_dm_list._dmp->dmr_pathName))){
    Tcl_AppendResult(interp, "f_aim: unrecognized pathName - ", argv[2],
		     "\n", (char *)NULL);

    return TCL_ERROR;
  }

  /* already aiming */
  if(p_cmd->aim)
    p_cmd->aim->aim = (struct cmd_list *)NULL;

  p_cmd->aim = p_dm;

  /* already being aimed at */
  if(p_dm->aim)
    p_dm->aim->aim = (struct dm_list *)NULL;

  p_dm->aim = p_cmd;
  Tcl_AppendResult(interp, bu_vls_addr(&p_cmd->name), " ---> ",
		   bu_vls_addr(&p_cmd->aim->_dmp->dmr_pathName),
		   "\n", (char *)NULL);
  
  return TCL_OK;
}

int
f_ps(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char *argv[];
{
  int i;
  int status;
  char **av;
  struct dm_list *dml;
  struct shared_info *sip;

  if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
    return TCL_ERROR;

  dml = curr_dm_list;
  av = (char **)bu_malloc(sizeof(char *) * (argc + 2), "f_ps: av");

  /* Load the argument vector */
  av[0] = "attach";
  av[1] = "ps";
  for(i = 2; i < argc + 1; ++i)
    av[i] = argv[i - 1];
  av[i] = NULL;

  status = f_attach(clientData, interp, i - 1, av);
  if(status == TCL_ERROR){
    bu_free((genptr_t)av, "f_ps: av");
    return TCL_ERROR;
  }

  sip = curr_dm_list->s_info;  /* save state info pointer */
  curr_dm_list->s_info = dml->s_info;  /* use dml's state info */

  dirty = 1;
  refresh();

  curr_dm_list->s_info = sip;  /* restore state info pointer */
  av[0] = "release";
  av[1] = NULL;
  status = f_release(clientData, interp, 1, av);

  bu_free((genptr_t)av, "f_ps: av");
  return status;
}

/*
 * Experimental - like f_plot except we attach to dm-plot, passing along
 *                any arguments.
 */
int
f_pl(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int argc;
char *argv[];
{
  int i;
  int status;
  char **av;
  struct dm_list *dml;
  struct shared_info *sip;

  if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
    return TCL_ERROR;

  dml = curr_dm_list;
  av = (char **)bu_malloc(sizeof(char *) * (argc + 2), "f_pl: av");

  /* Load the argument vector */
  av[0] = "attach";
  av[1] = "plot";
  for(i = 2; i < argc + 1; ++i)
    av[i] = argv[i - 1];
  av[i] = NULL;

  status = f_attach(clientData, interp, i - 1, av);
  if(status == TCL_ERROR){
    bu_free((genptr_t)av, "f_pl: av");
    return TCL_ERROR;
  }

  sip = curr_dm_list->s_info;  /* save state info pointer */
  curr_dm_list->s_info = dml->s_info;  /* use dml's state info */

  dirty = 1;
  refresh();

  curr_dm_list->s_info = sip;  /* restore state info pointer */
  av[0] = "release";
  av[1] = NULL;
  status = f_release(clientData, interp, 1, av);

  bu_free((genptr_t)av, "f_pl: av");
  return status;
}

int
f_setview(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    *argv[];
{
  double x, y, z;

  if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
    return TCL_ERROR;

  if(sscanf(argv[1], "%lf", &x) < 1){
    Tcl_AppendResult(interp, "f_setview: bad x value - ",
		     argv[1], "\n", (char *)NULL);
    return TCL_ERROR;
  }

  if(sscanf(argv[2], "%lf", &y) < 1){
    Tcl_AppendResult(interp, "f_setview: bad y value - ",
		     argv[2], "\n", (char *)NULL);
    return TCL_ERROR;
  }

  if(sscanf(argv[3], "%lf", &z) < 1){
    Tcl_AppendResult(interp, "f_setview: bad z value - ",
		     argv[3], "\n", (char *)NULL);
    return TCL_ERROR;
  }

  setview(x, y, z);

  return TCL_OK;
}

int
f_slewview(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int	argc;
char	*argv[];
{
  int x, y;
  vect_t tabvec;


  if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
    return TCL_ERROR;

  if(sscanf(argv[1], "%d", &x) < 1){
    Tcl_AppendResult(interp, "f_slewview: bad x value - ",
		     argv[1], "\n", (char *)NULL);
    return TCL_ERROR;
  }

  if(sscanf(argv[2], "%d", &y) < 1){
    Tcl_AppendResult(interp, "f_slewview: bad y value - ",
		     argv[2], "\n", (char *)NULL);
    return TCL_ERROR;
  }

  tabvec[X] =  x / 2047.0;
  tabvec[Y] =  y / 2047.0;
  tabvec[Z] = 0;
  slewview( tabvec );

  return TCL_OK;
}

void
set_e_axes_pos()
{
  int	i;

#if 0
  update_views = 1;

  if(EDIT_TRAN) {
    VMOVE(e_axes_pos, es_keypoint);
    MAT4X3PNT( absolute_slew, model2view, es_keypoint );
  }else{
    point_t new_pos;

    VSET(new_pos, -orig_pos[X], -orig_pos[Y], -orig_pos[Z]);
    MAT4X3PNT(absolute_slew, model2view, new_pos);

    if(EDIT_ROTATE)
      VSETALL( absolute_rotate, 0.0 );
  }
#else
  update_views = 1;
  switch(es_int.idb_type){
  case	ID_ARB8:
  case	ID_ARBN:
    if(state == ST_O_EDIT)
      i = 0;
    else
      switch(es_edflag){
      case	STRANS:
	i = 0;
	break;
      case	EARB:
	switch(es_type){
	case	ARB5:
	  i = earb5[es_menu][0];
	  break;
	case	ARB6:
	  i = earb6[es_menu][0];
	  break;
	case	ARB7:
	  i = earb7[es_menu][0];
	  break;
	case	ARB8:
	  i = earb8[es_menu][0];
	  break;
	default:
	  i = 0;
	  break;
	}
	break;
      case	PTARB:
	switch(es_type){
	case    ARB4:
	  i = es_menu;	/* index for point 1,2,3 or 4 */
	  break;
	case    ARB5:
	case	ARB7:
	  i = 4;	/* index for point 5 */
	  break;
	case    ARB6:
	  i = es_menu;	/* index for point 5 or 6 */
	  break;
	default:
	  i = 0;
	  break;
	}
	break;
      case ECMD_ARB_MOVE_FACE:
	switch(es_type){
	case	ARB4:
	  i = arb_faces[0][es_menu * 4];
	  break;
	case	ARB5:
	  i = arb_faces[1][es_menu * 4];  		
	  break;
	case	ARB6:
	  i = arb_faces[2][es_menu * 4];  		
	  break;
	case	ARB7:
	  i = arb_faces[3][es_menu * 4];  		
	  break;
	case	ARB8:
	  i = arb_faces[4][es_menu * 4];  		
	  break;
	default:
	  i = 0;
	  break;
	}
	break;
      case ECMD_ARB_ROTATE_FACE:
	i = fixv;
	break;
      default:
	i = 0;
	break;
      }

    VMOVE(e_axes_pos, ((struct rt_arb_internal *)es_int.idb_ptr)->pt[i]);
    break;
  case ID_TGC:
  case ID_REC:
    if(es_edflag == ECMD_TGC_MV_H ||
       es_edflag == ECMD_TGC_MV_HH){
      struct rt_tgc_internal  *tgc = (struct rt_tgc_internal *)es_int.idb_ptr;

      VADD2(e_axes_pos, tgc->h, tgc->v);
      break;
    }
  default:
    VMOVE(e_axes_pos, es_keypoint);
    break;
  }

  if(EDIT_TRAN) {
    MAT4X3PNT( absolute_slew, model2view, e_axes_pos );
  }else{
    point_t new_pos;

    VSET(new_pos, -orig_pos[X], -orig_pos[Y], -orig_pos[Z]);
    MAT4X3PNT(absolute_slew, model2view, new_pos);

    if(EDIT_ROTATE)
      VSETALL( absolute_rotate, 0.0 );
  }
#endif
}

int
f_winset(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int     argc;
char    **argv;
{
  register struct dm_list *p;

  if(mged_cmd_arg_check(argc, argv, (struct funtab *)NULL))
    return TCL_ERROR;

  /* print pathname of drawing window with primary focus */
  if( argc == 1 ){
    Tcl_AppendResult(interp, bu_vls_addr(&pathName), "\n", (char *)NULL);
    return TCL_OK;
  }

  /* change primary focus to window argv[1] */
  for( BU_LIST_FOR(p, dm_list, &head_dm_list.l ) ){
    if( !strcmp( argv[1], bu_vls_addr( &p->_dmp->dmr_pathName ) ) ){
      curr_dm_list = p;

      if(curr_dm_list->aim)
	curr_cmd_list = curr_dm_list->aim;
      else
	curr_cmd_list = &head_cmd_list;

      return TCL_OK;
    }
  }

  Tcl_AppendResult(interp, "Unrecognized pathname - ", argv[1],
		    "\n", (char *)NULL);
  return TCL_ERROR;
}


/*
 *                         S E T _ T R A N
 *
 * Calculate the values for absolute_slew.
 *
 *
 */
void
set_tran(x, y, z)
fastf_t x, y, z;
{
  point_t diff;
  point_t old_pos;
  point_t new_pos;
  point_t view_pos;

  diff[X] = x - e_axes_pos[X];
  diff[Y] = y - e_axes_pos[Y];
  diff[Z] = z - e_axes_pos[Z];
  
  /* If there is more than one active view, then absolute_slew
     needs to be initialized for each view. */
  MAT_DELTAS_GET_NEG(old_pos, toViewcenter);
  VADD2(new_pos, old_pos, diff);
  MAT4X3PNT(view_pos, model2view, new_pos);
  VMOVE( absolute_slew, view_pos );
}


int
tran(view_pos)
vect_t view_pos;
{
  point_t old_pos;
  point_t new_pos;
  point_t diff;

  if(state == ST_S_EDIT && !SEDIT_ROTATE && es_edflag > IDLE){
    tran_set = 1;
    sedit_mouse(view_pos);
    tran_set = 0;
  }else if(state == ST_O_EDIT && !OEDIT_ROTATE && edobj){
    tran_set = 1;
    objedit_mouse(view_pos);
    tran_set = 0;
  }else{/* slew the view */
    MAT4X3PNT( new_pos, view2model, view_pos );
    MAT_DELTAS_GET_NEG( old_pos, toViewcenter );
    VSUB2( diff, new_pos, old_pos );
    VADD2(new_pos, orig_pos, diff);
    MAT_DELTAS_VEC( toViewcenter, new_pos);
    MAT_DELTAS_VEC( ModelDelta, new_pos);
    new_mats();
  }

  return CMD_OK;
}

int
irot(x, y, z)
fastf_t x, y, z;
{
  point_t model_pos;
  point_t new_pos;
  int status;
  char *av[5];
  struct bu_vls xval, yval, zval;

  bu_vls_init(&xval);
  bu_vls_init(&yval);
  bu_vls_init(&zval);
  bu_vls_printf(&xval, "%f", x);
  bu_vls_printf(&yval, "%f", y);
  bu_vls_printf(&zval, "%f", z);
  av[1] = bu_vls_addr(&xval);
  av[2] = bu_vls_addr(&yval);
  av[3] = bu_vls_addr(&zval);
  av[4] = NULL;

  /* if in view state or not doing a solid rotate then allow the view to be rotated */
  if(state == ST_VIEW || !EDIT_ROTATE){
    if(EDIT_TRAN)
      MAT4X3PNT(model_pos, view2model, absolute_slew);

    av[0] = "vrot";
    status = f_vrot((ClientData)NULL, interp, 4, av);

    if(EDIT_TRAN){
      MAT4X3PNT(absolute_slew, model2view, model_pos);
    }else{
      VSET(new_pos, -orig_pos[X], -orig_pos[Y], -orig_pos[Z]);
      MAT4X3PNT(absolute_slew, model2view, new_pos);
    }
  }else if(state == ST_S_EDIT){
    av[0] = "p";
    status = f_param((ClientData)NULL, interp, 4, av);
  }else if(state == ST_O_EDIT){
    av[0] = "rotobj";
    status = f_rot_obj((ClientData)NULL, interp, 4, av);
  }

  bu_vls_free(&xval);
  bu_vls_free(&yval);
  bu_vls_free(&zval);

  if(status == TCL_OK)
    return CMD_OK;

  return CMD_BAD;
}

