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
#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "rtstring.h"
#include "rtlist.h"
#include "rtgeom.h"
#include "externs.h"
#include "./ged.h"
#include "./solid.h"
#include "./dm.h"
#include "./sedit.h"

#include "./mgedtcl.h"

#define MORE_ARGS_STR    "more arguments needed::"

#ifdef XMGED

#define DEFSHELL "/bin/sh"
#define TIME_STR_SIZE 32
#define NFUNC   ( (sizeof(funtab)) / (sizeof(struct funtab)) )

extern void (*dotitles_hook)();
extern FILE     *ps_fp;
extern struct dm dm_PS;
extern char     ps_ttybuf[];
extern int      in_middle;
extern mat_t    ModelDelta;
extern short earb4[5][18];
extern short earb5[9][18];
extern short earb6[10][18];
extern short earb7[12][18];
extern short earb8[12][18];
extern FILE	*journal_file;
extern int	journal;	/* initialize to off */
extern int update_views;
extern struct rt_db_internal es_int;
extern short int fixv;         /* used in ECMD_ARB_ROTATE_FACE,f_eqn(): fixed vertex */

typedef struct _cmd{
	struct _cmd	*prev;
	struct _cmd	*next;
	char	*cmd;
	char	time[TIME_STR_SIZE];
	int	num;
}Cmd, *CmdList;

typedef struct _alias{
	struct _alias	*left;
	struct _alias	*right;
	char	*name;	/* name of the alias */
	char	*def;	/* definition of the alias */
	int	marked;
}Alias, *AliasList;

CmdList	hhead=NULL, htail=NULL, hcurr=NULL;	/* for history list */
AliasList atop = NULL;
AliasList alias_free = NULL;

int 	savedit = 0;
point_t	orig_pos;
point_t e_axis_pos;
int irot_set = 0;
double irot_x = 0;
double irot_y = 0;
double irot_z = 0;
int tran_set = 0;
double tran_x = 0;
double tran_y = 0;
double tran_z = 0;

void set_e_axis_pos();
void set_tran();
int     mged_wait();
int chg_state();

static void	addtohist();
static int	parse_history();
static void	print_alias(), load_alias_def();
static void	balance_alias_tree(), free_alias_node();
static AliasList	get_alias_node();
static int	extract_alias_def();
static void    make_command();
int	f_history(), f_alias(), f_unalias();
int	f_journal(), f_button(), f_savedit(), f_slider();
int	f_slewview(), f_openw(), f_closew();
int	(*button_hook)(), (*slider_hook)();
int	(*openw_hook)(), (*closew_hook)();
int	(*knob_hook)();
int (*cue_hook)(), (*zclip_hook)(), (*zbuffer_hook)();
int (*light_hook)(), (*perspective_hook)();
int (*tran_hook)(), (*rot_hook)();
int (*set_tran_hook)();
int (*bindkey_hook)();

int     f_perspective(), f_cue(), f_light(), f_zbuffer(), f_zclip();
int     f_tran(), f_irot();
int     f_aip(), f_ps();
int     f_bindkey();
#endif /* XMGED */

/* Carl Nuzman experimental */
#if 1
extern int cmd_vdraw();
extern int cmd_read_center();
extern int cmd_read_scale();
#endif

extern Tcl_CmdProc	cmd_fhelp;

extern void sync();
int	inpara;			/* parameter input from keyboard */

int glob_compat_mode = 1;
int output_as_return = 0;

int mged_cmd();
struct rt_vls tcl_output_hook;

Tcl_Interp *interp;
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
	if( atoi(argv[1]) != 0 )
		return Tcl_Eval( interp, cmd );
	return TCL_OK;
}


struct funtab {
    char *ft_name;
    char *ft_parms;
    char *ft_comment;
    int (*ft_func)();
    int ft_min;
    int ft_max;
    int tcl_converted;
};

static struct funtab funtab[] = {
"", "", "Primary command Table.",
        0, 0, 0, FALSE,
"?", "", "summary of available commands",
        cmd_fhelp,0,MAXARGS,TRUE,
"%", "", "escape to interactive shell",
	f_comm,1,1,FALSE,
"3ptarb", "", "makes arb given 3 pts, 2 coord of 4th pt, and thickness",
	f_3ptarb, 1, 27,FALSE,
"adc", "[<a1|a2|dst|dh|dv|hv|dx|dy|dz|xyz|reset|help> value(s)]",
	"control the angle/distance cursor",
        f_adc, 1, 5, FALSE,
"ae", "azim elev", "set view using az and elev angles",
	f_aeview, 3, 3, FALSE,
#ifdef XMGED
"aip", "[fb]", "advance illumination pointer or path position forward or backward",
        f_aip, 1, 2, FALSE,
"alias", "[name definition]", "lists or creates an alias",
	f_alias, 1, MAXARGS, FALSE,
#endif
"analyze", "[arbname]", "analyze faces of ARB",
	f_analyze,1,MAXARGS,FALSE,
"apropos", "keyword", "finds commands whose descriptions contain the given keyword",
        cmd_apropos, 2, 2, TRUE,
"arb", "name rot fb", "make arb8, rotation + fallback",
	f_arbdef,4,4,FALSE,
"arced", "a/b ...anim_command...", "edit matrix or materials on combination's arc",
	f_arced, 3,MAXARGS,FALSE,
"area", "[endpoint_tolerance]", "calculate presented area of view",
	f_area, 1, 2, FALSE,
"attach", "[device]", "attach to a display processor, or NU",
	f_attach,1,2,FALSE,
"B", "<objects>", "clear screen, edit objects",
	f_blast,2,MAXARGS,FALSE,
"bev",	"[-t] [-P#] new_obj obj1 op obj2 op obj3 op ...", "Boolean evaluation of objects via NMG's",
	f_bev, 2, MAXARGS, FALSE,
#ifdef XMGED
"bindkey", "[key] [command]", "bind key to a command",
        f_bindkey, 1, MAXARGS, FALSE,
"button", "number", "simulates a button press, not intended for the user",
	f_button, 2, 2, FALSE,
#endif
"c", "[-gr] comb_name [boolean_expr]", "create or extend a combination using standard notation",
	f_comb_std,3,MAXARGS,FALSE,
"cat", "<objects>", "list attributes (brief)",
	f_cat,2,MAXARGS,FALSE,
"center", "x y z", "set view center",
	f_center, 4,4, FALSE,
#ifdef XMGED
"closew", "[host]", "close drawing window associated with host",
	f_closew, 1, 2, FALSE,
#endif
"color", "low high r g b str", "make color entry",
	f_color, 7, 7, FALSE,
"comb", "comb_name <operation solid>", "create or extend combination w/booleans",
	f_comb,4,MAXARGS,FALSE,
"dbconcat", "file [prefix]", "concatenate 'file' onto end of present database.  Run 'dup file' first.",
	f_concat, 2, 3, FALSE,
"copyeval", "new_solid path_to_old_solid (seperate path components with spaces, not /)",
	"copy an 'evaluated' path solid",
	f_copyeval, 1, 27, FALSE,
"cp", "from to", "copy [duplicate] object",
	f_copy,3,3, FALSE,
#ifdef XMGED
"cue", "", "toggle cueing",
        f_cue, 1, 1, FALSE,
#endif
"cpi", "from to", "copy cylinder and position at end of original cylinder",
	f_copy_inv,3,3,FALSE,
"d", "<objects>", "delete list of objects",
	f_delobj,2,MAXARGS,FALSE,
"db", "command", "database manipulation routines",
	cmd_db, 1, MAXARGS, TRUE,
"debugdir", "", "Print in-memory directory, for debugging",
	f_debugdir, 1, 1, FALSE,
"debuglib", "[hex_code]", "Show/set debugging bit vector for librt",
	f_debuglib,1,2,FALSE,
"debugmem", "", "Print librt memory use map",
	f_debugmem, 1, 1, FALSE,
"debugnmg", "[hex code]", "Show/set debugging bit vector for NMG",
	f_debugnmg,1,2,FALSE,
"delay", "sec usec", "Delay for the specified amount of time",
	f_delay,3,3,FALSE,
"dm", "set var [val]", "Do display-manager specific command",
	f_dm, 2, MAXARGS, FALSE,
"dup", "file [prefix]", "check for dup names in 'file'",
	f_dup, 2, 3, FALSE,
"E", "<objects>", "evaluated edit of objects",
	f_evedit,2,MAXARGS,FALSE,
"e", "<objects>", "edit objects",
	f_edit,2,MAXARGS,FALSE,
"echo", "[text]", "echo arguments back",
	cmd_echo, 1, MAXARGS, TRUE,
"edcodes", "object(s)", "edit region ident codes",
	f_edcodes, 2, MAXARGS, FALSE,
"edcolor", "", "text edit color table",
	f_edcolor, 1, 1, FALSE,
"edcomb", "combname Regionflag regionid air los [GIFTmater]", "edit combination record info",
	f_edcomb,6,7,FALSE,
"edgedir", "[delta_x delta_y delta_z]|[rot fb]", "define direction of ARB edge being moved",
	f_edgedir, 3, 4, FALSE,
"ev",	"[-dnqsuvwT] [-P #] <objects>", "evaluate objects via NMG tessellation",
	f_ev, 2, MAXARGS, FALSE,
"eqn", "A B C", "planar equation coefficients",
	f_eqn, 4, 4, FALSE,
"exit", "", "exit",
	f_quit,1,1,FALSE,
"extrude", "#### distance", "extrude dist from face",
	f_extrude,3,3,FALSE,
"expand", "wildcard expression", "expands wildcard expression",
        cmd_expand, 1, MAXARGS, TRUE,
"facedef", "####", "define new face for an arb",
	f_facedef, 2, MAXARGS, FALSE,
"facetize", "[-t] [-P#] new_obj old_obj(s)", "convert objects to faceted NMG objects at current tol",
	f_facetize, 3, MAXARGS, FALSE,
"find", "<objects>", "find all references to objects",
	f_find, 1, MAXARGS, FALSE,
"fix", "", "fix display after hardware error",
	f_fix,1,1,FALSE,
"fracture", "NMGsolid [prefix]", "fracture an NMG solid into many NMG solids, each containing one face\n",
	f_fracture, 2, 3, FALSE,
"g", "groupname <objects>", "group objects",
	f_group,3,MAXARGS,FALSE,
"getknob", "knobname", "Gets the current setting of the given knob",
        cmd_getknob, 2, 2, TRUE,
"output_hook", "output_hook_name",
       "All output is sent to the Tcl procedure \"output_hook_name\"",
	cmd_output_hook, 1, 2, TRUE,
#ifdef HIDELINE
"H", "plotfile [step_size %epsilon]", "produce hidden-line unix-plot",
	f_hideline,2,4,FALSE,
#endif
"help", "[commands]", "give usage message for given commands",
	f_help,0,MAXARGS,FALSE,
#ifdef XMGED
"history", "[N]", "print out history of commands or last N commands",
	f_history,1, 2,FALSE,
#else
"history", "[-delays]", "list command history",
	f_history, 1, 4,FALSE,
"hist_prev", "", "Returns previous command in history",
        cmd_prev, 1, 1, TRUE,
"hist_next", "", "Returns next command in history",
        cmd_next, 1, 1, TRUE,
"hist_add", "", "Adds command to the history (without executing it)",
        cmd_hist_add, 1, 1, TRUE,
#endif
"i", "obj combination [operation]", "add instance of obj to comb",
	f_instance,3,4,FALSE,
"idents", "file object(s)", "make ascii summary of region idents",
	f_tables, 3, MAXARGS, FALSE,
#ifdef XMGED
"iknob", "id [val]", "increment knob value",
       f_knob,2,3, FALSE,
#endif
"ill", "name", "illuminate object",
	f_ill,2,2,FALSE,
"in", "[-f] [-s] parameters...", "keyboard entry of solids.  -f for no drawing, -s to enter solid edit",
	f_in, 1, MAXARGS, FALSE,
"inside", "", "finds inside solid per specified thicknesses",
	f_inside, 1, MAXARGS, FALSE,
#ifdef XMGED
"irot", "x y z", "incremental/relative rotate",
        f_irot, 4, 4, FALSE,
#endif
"item", "region item [air]", "change item # or air code",
	f_itemair,3,4,FALSE,
#ifdef XMGED
"itran", "x y z", "incremental/relative translate using normalized screen coordinates",
        f_tran, 4, 4,FALSE,
#endif
"joint", "command [options]", "articualtion/animation commands",
	f_joint, 1, MAXARGS, FALSE,
#ifdef XMGED
"journal", "[file]", "toggle journaling on or off",
	f_journal, 1, 2, FALSE,
#else
"journal", "fileName", "record all commands and timings to journal",
	f_journal, 1, 2, FALSE,
#endif
"keep", "keep_file object(s)", "save named objects in specified file",
	f_keep, 3, MAXARGS, FALSE,
"keypoint", "[x y z | reset]", "set/see center of editing transformations",
	f_keypoint,1,4, FALSE,
"kill", "[-f] <objects>", "delete object[s] from file",
	f_kill,2,MAXARGS,FALSE,
"killall", "<objects>", "kill object[s] and all references",
	f_killall, 2, MAXARGS,FALSE,
"killtree", "<object>", "kill complete tree[s] - BE CAREFUL",
	f_killtree, 2, MAXARGS, FALSE,
"knob", "id [val]", "emulate knob twist",
	f_knob,2,3, FALSE,
"l", "<objects>", "list attributes (verbose)",
	cmd_list,2,MAXARGS, TRUE,
"L",  "1|0 xpos ypos", "handle a left mouse event",
	cmd_left_mouse, 4,4, TRUE,
"labelvert", "object[s]", "label vertices of wireframes of objects",
	f_labelvert, 2, MAXARGS, FALSE,
#ifdef XMGED
"light", "", "toggle lighting",
        f_light, 1, 1, FALSE,
#endif
"listeval", "", "lists 'evaluated' path solids",
	f_pathsum, 1, MAXARGS, FALSE,
"loadtk", "[DISPLAY]", "Initializes Tk window library",
        cmd_tk, 1, 2, TRUE,
"ls", "", "table of contents",
	dir_print,1,MAXARGS, FALSE,
"M", "1|0 xpos ypos", "handle a middle mouse event",
	f_mouse, 4,4, FALSE,
"make", "name <arb8|sph|ellg|tor|tgc|rpc|rhc|epa|ehy|eto|part|grip|half|nmg|pipe>", "create a primitive",
	f_make,3,3,FALSE,
"mater", "comb [material]", "assign/delete material to combination",
	f_mater,2,3,FALSE,
"matpick", "# or a/b", "select arc which has matrix to be edited, in O_PATH state",
	f_matpick, 2,2,FALSE,
"memprint", "", "print memory maps",
	f_memprint, 1, 1,FALSE,
"mirface", "#### axis", "mirror an ARB face",
	f_mirface,3,3,FALSE,
"mirror", "old new axis", "Arb mirror ??",
	f_mirror,4,4,FALSE,
"mv", "old new", "rename object",
	f_name,3,3,FALSE,
"mvall", "oldname newname", "rename object everywhere",
	f_mvall, 3, 3,FALSE,
"nirt", "", "trace a single ray from current view",
	f_nirt,1,MAXARGS,FALSE,
"oed", "path_lhs path_rhs", "Go from view to object_edit of path_lhs/path_rhs",
	cmd_oed, 3, 3, TRUE,
"opendb", "database.g", "Close current .g file, and open new .g file",
	f_opendb, 2, 2,FALSE,
#ifdef XMGED
"openw", "[host]", "open a drawing window on host",
	f_openw, 1, 2,FALSE,
#endif
"orientation", "x y z w", "Set view direction from quaternion",
	f_orientation, 5, 5,FALSE,
"orot", "xdeg ydeg zdeg", "rotate object being edited",
	f_rot_obj, 4, 4,FALSE,
"overlay", "file.plot [name]", "Read UNIX-Plot as named overlay",
	f_overlay, 2, 3,FALSE,
"p", "dx [dy dz]", "set parameters",
	f_param,2,4,FALSE,
"paths", "pattern", "lists all paths matching input path",
	f_pathsum, 1, MAXARGS,FALSE,
"pathlist", "name(s)", "list all paths from name(s) to leaves",
	cmd_pathlist, 1, MAXARGS,TRUE,
"permute", "tuple", "permute vertices of an ARB",
	f_permute,2,2,FALSE,
#ifdef XMGED
"perspective", "[n]", "toggle perspective",
        f_perspective, 1, 2,FALSE,
#endif
"plot", "[-float] [-zclip] [-2d] [-grid] [out_file] [|filter]", "make UNIX-plot of view",
	f_plot, 2, MAXARGS,FALSE,
"polybinout", "file", "store vlist polygons into polygon file (experimental)",
	f_polybinout, 2, 2,FALSE,
"pov", "args", "experimental:  set point-of-view",
	f_pov, 3+4+1, MAXARGS,FALSE,
"prcolor", "", "print color&material table",
	f_prcolor, 1, 1,FALSE,
"prefix", "new_prefix object(s)", "prefix each occurrence of object name(s)",
	f_prefix, 3, MAXARGS,FALSE,
"preview", "[-v] [-d sec_delay] rt_script_file", "preview new style RT animation script",
	f_preview, 2, MAXARGS,FALSE,
"press", "button_label", "emulate button press",
	f_press,2,MAXARGS,FALSE,
#ifdef XMGED
"ps", "[f] file", "create postscript file of current view with or without the faceplate",
        f_ps, 2, 3,FALSE,
#endif
"push", "object[s]", "pushes object's path transformations to solids",
	f_push, 2, MAXARGS,FALSE,
"q", "", "quit",
	f_quit,1,1,FALSE,
"quit", "", "quit",
	f_quit,1,1,FALSE,
"qorot", "x y z dx dy dz theta", "rotate object being edited about specified vector",
	f_qorot, 8, 8,FALSE,
"qvrot", "dx dy dz theta", "set view from direction vector and twist angle",
	f_qvrot, 5, 5,FALSE,
"r", "region <operation solid>", "create or extend a Region combination",
	f_region,4,MAXARGS,FALSE,
"R",  "1|0 xpos ypos", "handle a right mouse event",
	cmd_right_mouse, 4,4, TRUE,
"red", "object", "edit a group or region using a text editor",
	f_red, 2, 2,FALSE,
"refresh", "", "send new control list",
	f_refresh, 1,1,FALSE,
"regdebug", "", "toggle register print",
	f_regdebug, 1,2,FALSE,
"regdef", "item [air] [los] [GIFTmaterial]", "change next region default codes",
	f_regdef, 2, 5,FALSE,
"regions", "file object(s)", "make ascii summary of regions",
	f_tables, 3, MAXARGS,FALSE,
"release", "", "release current display processor [attach NU]",
	f_release,1,1,FALSE,
"rfarb", "", "makes arb given point, 2 coord of 3 pts, rot, fb, thickness",
	f_rfarb, 1, 27,FALSE,
"rm", "comb <members>", "remove members from comb",
	f_rm,3,MAXARGS,FALSE,
"rmats", "file", "load views from file (experimental)",
	f_rmats,2,MAXARGS,FALSE,
"rotobj", "xdeg ydeg zdeg", "rotate object being edited",
	f_rot_obj, 4, 4,FALSE,
"rrt", "prog [options]", "invoke prog with view",
	f_rrt,2,MAXARGS,FALSE,
"rt", "[options]", "do raytrace of view",
	f_rt,1,MAXARGS,FALSE,
"rtcheck", "[options]", "check for overlaps in current view",
	f_rtcheck,1,MAXARGS,FALSE,
#ifdef XMGED
"savedit", "", "save current edit and remain in edit state",
	f_savedit, 1, 1,FALSE,
#endif
"savekey", "file [time]", "save keyframe in file (experimental)",
	f_savekey,2,MAXARGS,FALSE,
"saveview", "file [args]", "save view in file for RT",
	f_saveview,2,MAXARGS,FALSE,
"oscale", "factor", "scale object by factor",
	f_sc_obj,2,2,FALSE,
"sed", "<path>", "solid-edit named solid",
	f_sed,2,2,FALSE,
"vars",	"[var=opt]", "assign/display mged variables",
	f_set,1,2,FALSE,
"shells", "nmg_model", "breaks model into seperate shells",
	f_shells, 2,2,FALSE,
"shader", "comb material [arg(s)]", "assign materials (like 'mater')",
	f_shader, 3,MAXARGS,FALSE,
"size", "size", "set view size",
	f_view, 2,2,FALSE,
#ifdef XMGED
"slider", "slider number, value", "adjust sliders using keyboard",
	f_slider, 3,3,FALSE,
#endif
"sliders", "[{on|off}]", "turns the sliders on or off, or reads current state",
        cmd_sliders, 1, 2, TRUE,
"solids", "file object(s)", "make ascii summary of solid parameters",
	f_tables, 3, MAXARGS,FALSE,
"solids_on_ray", "h v", "List all displayed solids along a ray",
        cmd_solids_on_ray, 1, 3, TRUE,
"status", "", "get view status",
	f_status, 1,1,FALSE,
"summary", "[s r g]", "count/list solid/reg/groups",
	f_summary,1,2,FALSE,
#ifdef XMGED
"sv", "x y", "Move view center to (x, y, 0)",
	f_slewview, 3, 3,FALSE,
#endif
"sync",	"",	"forces UNIX sync",
	f_sync, 1, 1,FALSE,
"t", "", "table of contents",
	dir_print,1,MAXARGS,FALSE,
"tab", "object[s]", "tabulates objects as stored in database",
	f_tabobj, 2, MAXARGS,FALSE,
"ted", "", "text edit a solid's parameters",
	f_tedit,1,1,FALSE,
"title", "string", "change the title",
	f_title,1,MAXARGS,FALSE,
"tol", "[abs #] [rel #] [norm #] [dist #] [perp #]", "show/set tessellation and calculation tolerances",
	f_tol, 1, 11,FALSE,
"tops", "", "find all top level objects",
	f_tops,1,1,FALSE,
"track", "<parameters>", "adds tracks to database",
	f_amtrack, 1, 27,FALSE,
#ifdef XMGED
"tran", "x y z", "absolute translate using view coordinates",
        f_tran, 4, 4,FALSE,
#endif
"translate", "x y z", "trans object to x,y, z",
	f_tr_obj,4,4,FALSE,
"tree",	"object(s)", "print out a tree of all members of an object",
	f_tree, 2, MAXARGS,FALSE,
#ifdef XMGED
"unalias","name/s", "deletes an alias or aliases",
	f_unalias, 2, MAXARGS,FALSE,
#endif
"units", "[mm|cm|m|in|ft|...]", "change units",
	f_units,1,2,FALSE,
#if 1
"vdraw", "write|insert|delete|read|length|show [args]", "Expermental drawing (cnuzman)",
	cmd_vdraw, 2, 7, TRUE,
"read_center", "", "Experimental - return coords of view center",
	cmd_read_center, 1, 1, TRUE,
"read_scale", "", "Experimental - return coords of view scale",
	cmd_read_scale, 1, 1, TRUE,
#endif
"vrmgr", "host {master|slave|overview}", "link with Virtual Reality manager",
	f_vrmgr, 3, MAXARGS,FALSE,
"vrot", "xdeg ydeg zdeg", "rotate viewpoint",
	f_vrot,4,4,FALSE,
"vrot_center", "v|m x y z", "set center point of viewpoint rotation, in model or view coords",
	f_vrot_center, 5, 5,FALSE,
"whichid", "ident(s)", "lists all regions with given ident code",
	f_which_id, 2, MAXARGS,FALSE,
"x", "lvl", "print solid table & vector list",
	f_debug, 1,2,FALSE,
"xpush", "object", "Experimental Push Command",
	f_xpush, 2,2,FALSE,
"Z", "", "zap all objects off screen",
	f_zap,1,1,FALSE,
#ifdef XMGED
"zbuffer", "", "toggle zbuffer",
        f_zbuffer, 1, 1,FALSE,
"zclip", "", "toggle zclipping",
        f_zclip, 1, 1,FALSE,
#endif
"zoom", "scale_factor", "zoom view in or out",
	f_zoom, 2,2,FALSE,
0, 0, 0,
	0, 0, 0, 0
};


/*
 *                        O U T P U T _ C A T C H
 *
 * Gets the output from rt_log and appends it to clientdata vls.
 */

HIDDEN int
output_catch(clientdata, str)
genptr_t clientdata;
char *str;
{
    register struct rt_vls *vp = (struct rt_vls *)clientdata;
    register int len;

    len = rt_vls_strlen(vp);
    rt_vls_strcat(vp, str);
    len = rt_vls_strlen(vp) - len;

    return len;
}

/*
 *                 S T A R T _ C A T C H I N G _ O U T P U T
 *
 * Sets up hooks to rt_log so that all output is caught in the given vls.
 *
 */

void
start_catching_output(vp)
struct rt_vls *vp;
{
    rt_add_hook(output_catch, (genptr_t)vp);
}

/*
 *                 S T O P _ C A T C H I N G _ O U T P U T
 *
 * Turns off the output catch hook.
 */

void
stop_catching_output(vp)
struct rt_vls *vp;
{
    rt_delete_hook(output_catch, (genptr_t)vp);
}

#ifdef XMGED
int
f_journal(argc, argv)
int	argc;
char	*argv[];
{
	char	*path;

	journal = journal ? 0 : 1;

	if(journal){
		if(argc == 2)
			if( (journal_file = fopen(argv[1], "w")) != NULL )
				return CMD_OK;
			
		if( (path = getenv("MGED_JOURNAL")) != (char *)NULL )
			if( (journal_file = fopen(path, "w")) != NULL )
				return CMD_OK;

		if( (journal_file = fopen( "mged.journal", "w" )) != NULL )
			return CMD_OK;

		journal = 0;	/* could not open a file so turn off */
		rt_log( "Could not open a file for journalling.\n");

		return CMD_BAD;
	}

	fclose(journal_file);
	return CMD_OK;
}

int
f_history(argc, argv)
int	argc;
char	*argv[];
{
	register int	i;
	int	N = 0;
	CmdList	ptr;


	if(argc == 1){	/* print entire history list */
		ptr = hhead;
	}else{	/* argc == 2;  print last N commands */
		sscanf(argv[1], "%d", &N);
		ptr = htail->prev;
		for(i = 1; i < N && ptr != NULL; ++i, ptr = ptr->prev);

		if(ptr == NULL)
			ptr = hhead;
	}

	for(; ptr != htail; ptr = ptr->next)
		rt_log("%d\t%s\t%s\n", ptr->num, ptr->time,
				ptr->cmd);

	return CMD_OK;
}

/*   The node at the end of the list(pointed to by htail) always
   contains a null cmd string. And hhead points to the
   beginning(oldest) of the list. */
static void
addtohist(cmd)
char *cmd;
{
	static int count = 0;
	time_t	clock;

	if(hhead != htail){	/* more than one node */
		htail->prev->next = (CmdList)malloc(sizeof (Cmd));
		htail->prev->next->next = htail;
		htail->prev->next->prev = htail->prev;
		htail->prev = htail->prev->next;
		clock = time((time_t *) 0);
		strftime(htail->prev->time, TIME_STR_SIZE, "%I:%M%p", localtime(&clock));
		htail->prev->cmd =  malloc(strlen(cmd));   /* allocate space for cmd */
		(void)strncpy(htail->prev->cmd, cmd, strlen(cmd) - 1);
		htail->prev->cmd[strlen(cmd) - 1] = NULL;
		htail->prev->num = count;
		++count;
	}else if(!count){	/* no nodes */
		hhead = htail = hcurr = (CmdList)malloc(sizeof (Cmd));
		htail->prev = NULL;	/* initialize pointers */
		htail->next = NULL;
		clock = time((time_t *) 0);
		strftime(htail->time, TIME_STR_SIZE, "%I:%M%p", localtime(&clock));
		htail->cmd = malloc(1);	/* allocate space */
		htail->cmd[0] = NULL;	/* first node will have null command string */
		++count;
		addtohist(cmd);
	}else{		/* only one node */
		htail->prev = (CmdList)malloc(sizeof (Cmd));
		htail->prev->next = htail;
		htail->prev->prev = NULL;
		hhead = htail->prev;
		clock = time((time_t *) 0);
		strftime(hhead->time, TIME_STR_SIZE, "%I:%M%p", localtime(&clock));
		hhead->cmd =  malloc(strlen(cmd));   /* allocate space for cmd */
		(void)strncpy(hhead->cmd, cmd, strlen(cmd) - 1);
		hhead->cmd[strlen(cmd) - 1] = NULL;
		hhead->num = count;
		++count;
	}
}

static int
parse_history(cmd, str)
struct rt_vls	*cmd;
char	*str;
{
	register int	i;
	int	N;
	CmdList	ptr;

	if(*str == '@'){	/* @@	last command */
		if(htail == NULL || htail->prev == NULL){
			rt_log( "No events in history list yet!\n");
			return(1);	/* no command in history list yet */
		}else{
			rt_vls_strcpy(cmd, htail->prev->cmd);
			rt_vls_strncat(cmd, "\n", 1);
			return(0);
		}
	}else if(*str >= '0' && *str <= '9'){	/* @N	command N */
		sscanf(str, "%d", &N);
		for(i = 1, ptr = hhead; i < N && ptr != NULL; ++i, ptr = ptr->next);
	
		if(ptr != NULL){
			rt_vls_strcpy(cmd, ptr->cmd);
			rt_vls_strncat(cmd, "\n", 1);
			return(0);
		}else{
			rt_log("%d: Event not found\n", N);
			return(1);
		}
	}else if(*str == '-'){
		++str;
		if(*str >= '0' && *str <= '9'){
			sscanf(str, "%d", &N);
			for(i = 0, ptr = htail; i < N && ptr != NULL; ++i, ptr = ptr->prev);
			if(ptr != NULL){
	                        rt_vls_strcpy(cmd, ptr->cmd);
				rt_vls_strncat(cmd, "\n", 1);
	                        return(0);
			}else{
	                        rt_log("%s: Event not found\n", str);
	                        return(1);
			}
		}else{
			rt_log("%s: Event not found\n", str);
			return(1);
		}
	}else{	/* assuming a character string for now */
		for( ptr = htail; ptr != NULL; ptr = ptr->prev ){
			if( strncmp( str, ptr->cmd, strlen( str ) - 1 ) )
				continue;
			else	/* found a match */
				break;
		}
		if( ptr != NULL ){
                        rt_vls_strcpy( cmd, ptr->cmd );
			rt_vls_strncat( cmd, "\n", 1 );
                        return( 0 );
		}else{
                        rt_log("%s: Event not found\n", str );
                        return( 1 );
		}
	}
}
#endif

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
    struct rt_vls result;
    int catch_output;

    argv[0] = ((struct funtab *)clientData)->ft_name;

    /* We now leave the world of Tcl where everything prints its results
       in the interp->result field.  Here, stuff gets printed with the
       rt_log command; hence, we must catch such output and stuff it into
       the result string.  Do this *only* if "output_as_return" global
       variable is set.  Make a local copy of this variable in case it's
       changed by our command. */

    catch_output = output_as_return;
    
    rt_vls_init(&result);

    if (catch_output)
	start_catching_output(&result);
    status = mged_cmd(argc, argv, funtab);
    if (catch_output)
	stop_catching_output(&result);

    /* Remove any trailing newlines. */

    if (catch_output) {
	len = rt_vls_strlen(&result);
	while (len > 0 && rt_vls_addr(&result)[len-1] == '\n')
	    rt_vls_trunc(&result, --len);
    }
    
    switch (status) {
    case CMD_OK:
	if (catch_output)
	    Tcl_SetResult(interp, rt_vls_addr(&result), TCL_VOLATILE);
	status = TCL_OK;
	break;
    case CMD_BAD:
	if (catch_output)
	    Tcl_SetResult(interp, rt_vls_addr(&result), TCL_VOLATILE);
	status = TCL_ERROR;
	break;
    case CMD_MORE:
	Tcl_SetResult(interp, MORE_ARGS_STR, TCL_STATIC);
	if (catch_output) {
	    Tcl_AppendResult(interp, rt_vls_addr(&result), (char *)NULL);
	}
	status = TCL_ERROR;
	break;
    default:
	Tcl_SetResult(interp, "error executing mged routine::", TCL_STATIC);
	if (catch_output) {
	    Tcl_AppendResult(interp, rt_vls_addr(&result), (char *)NULL);
	}
	status = TCL_ERROR;
	break;
    }
    
    rt_vls_free(&result);
    return status;
}

/*
 *                            G U I _ O U T P U T
 *
 * Used as a hook for rt_log output.  Sends output to the Tcl procedure whose
 * name is contained in the vls "tcl_output_hook".  Useful for user interface
 * building.
 */

int
gui_output(clientData, str)
genptr_t clientData;
char *str;
{
    Tcl_DString tclcommand;
    static int level = 0;

    if (level > 50) {
	rt_delete_hook(gui_output, clientData);
	/* Now safe to run rt_log? */
    	rt_log("Ack! Something horrible just happened recursively.\n");
	return 0;
    }

    Tcl_DStringInit(&tclcommand);
    (void)Tcl_DStringAppendElement(&tclcommand, rt_vls_addr(&tcl_output_hook));
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
    /* XXX Screen name should be same as attachment, or should ask, or
       should be settable from "-display" option. */

    if (tkwin != NULL)  {
	Tcl_SetResult(interp, "loadtk: already attached to display", TCL_STATIC);
	return TCL_ERROR;
    }

    if( argc != 2 )  {
	tkwin = Tk_CreateMainWindow(interp, (char *)NULL, "TkMGED", "tkMGED");
    } else {
	tkwin = Tk_CreateMainWindow(interp, argv[1], "TkMGED", "tkMGED");
    }
    if (tkwin == NULL)
	return TCL_ERROR;

    if (tkwin != NULL) {
	Tk_GeometryRequest(tkwin, 100, 20);

	/* This runs the tk.tcl script */
	if (Tk_Init(interp) == TCL_ERROR)
	    return TCL_ERROR;
    }
    
    /* Handle any delayed events which result */
    while (Tk_DoOneEvent(TK_DONT_WAIT | TK_ALL_EVENTS))
	;

    return TCL_OK;
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

    static struct {
	char *knobname;
	double *variable;
    } knobs[] = {
	"ax", &absolute_rotate[X],
	"ay", &absolute_rotate[Y],
	"az", &absolute_rotate[Z],
	"aX", &absolute_slew[X],
	"aY", &absolute_slew[Y],
	"aZ", &absolute_slew[Z],
	"aS", &absolute_zoom,
	"x", &rate_rotate[X],
	"y", &rate_rotate[Y],
	"z", &rate_rotate[Z],
	"X", &rate_slew[X],
	"Y", &rate_slew[Y],
	"Z", &rate_slew[Z],
	"S", &rate_zoom
    };
	
    if( argc < 2 ) {
	Tcl_SetResult(interp, "getknob: need a knob name", TCL_STATIC);
	return TCL_ERROR;
    }

    for (i = 0; i < sizeof(knobs); i++)
	if (strcmp(knobs[i].knobname, argv[1]) == 0) {
	    sprintf(interp->result, "%lf", *(knobs[i].variable));
	    return TCL_OK;
	}
    
    Tcl_SetResult(interp, "getknob: invalid knob name", TCL_STATIC);
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
    struct rt_vls infocommand;
    int status;

    if (argc > 2) {
	Tcl_SetResult(interp,
		      "too many args: should be \"output_hook [hookName]\"",
		      TCL_STATIC);
	return TCL_ERROR;
    }

    rt_delete_hook(gui_output, (genptr_t)interp);/* Delete the existing hook */

    if (argc < 2)
	return TCL_OK;

    /* Make sure the command exists before putting in the hook! */
    
    rt_vls_init(&infocommand);
    rt_vls_strcat(&infocommand, "info commands ");
    rt_vls_strcat(&infocommand, argv[1]);
    status = Tcl_Eval(interp, rt_vls_addr(&infocommand));
    rt_vls_free(&infocommand);

    if (status != TCL_OK || interp->result[0] == '\0') {
	Tcl_SetResult(interp, "command does not exist", TCL_STATIC);
	return TCL_ERROR;
    }

    /* Also, don't allow silly infinite loops. */

    if (strcmp(argv[1], argv[0]) == 0) {
	Tcl_SetResult(interp, "Don't be silly.", TCL_STATIC);
	return TCL_ERROR;
    }

    /* Set up the hook! */

    rt_vls_strcpy(&tcl_output_hook, argv[1]);
    rt_add_hook(gui_output, (genptr_t)interp);
    
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


/* 			C M D _ S E T U P
 *
 * Sets up the Tcl interpreter and calls other setup functions.
 */ 

void
cmd_setup(interactive)
int interactive;
{
    register struct funtab *ftp;
    struct rt_vls temp;

    rt_vls_init(&temp);

    /* The following is for GUI output hooks: contains name of function to
       run with output */
    rt_vls_init(&tcl_output_hook);

    /* Create the interpreter */

    interp = Tcl_CreateInterp();

#if 0
    Tcl_SetVar(interp, "tcl_interactive", interactive ? "1" : "0",
	       TCL_GLOBAL_ONLY);
#endif

    /* This runs the init.tcl script */
    if( Tcl_Init(interp) == TCL_ERROR )
	rt_log("Tcl_Init error %s\n", interp->result);

    /* Finally, add in all the MGED commands.  Warn if they conflict */
    for (ftp = funtab+1; ftp->ft_name != NULL; ftp++) {
#if 0
	rt_vls_strcpy(&temp, "info commands ");
	rt_vls_strcat(&temp, ftp->ft_name);
	if (Tcl_Eval(interp, rt_vls_addr(&temp)) != TCL_OK ||
	    interp->result[0] != '\0') {
	    rt_log("WARNING:  '%s' name collision (%s)\n", ftp->ft_name,
		   interp->result);
	}
#endif
	rt_vls_strcpy(&temp, "_mged_");
	rt_vls_strcat(&temp, ftp->ft_name);
	
	if (ftp->tcl_converted) {
	    (void)Tcl_CreateCommand(interp, ftp->ft_name, ftp->ft_func,
				   (ClientData)ftp, (Tcl_CmdDeleteProc *)NULL);
	    (void)Tcl_CreateCommand(interp, rt_vls_addr(&temp), ftp->ft_func,
				   (ClientData)ftp, (Tcl_CmdDeleteProc *)NULL);
	} else {
	    (void)Tcl_CreateCommand(interp, ftp->ft_name, cmd_wrapper, 	    
			           (ClientData)ftp, (Tcl_CmdDeleteProc *)NULL);
	    (void)Tcl_CreateCommand(interp, rt_vls_addr(&temp), cmd_wrapper,
				   (ClientData)ftp, (Tcl_CmdDeleteProc *)NULL);
	}
    }

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

    rt_vls_free(&temp);
    tkwin = NULL;

    history_setup();
    mged_variable_setup(interp);
}


/*
 * debackslash, backslash_specials, mged_compat: routines for original
 *   mged emulation mode
 */

void
debackslash( dest, src )
struct rt_vls *dest, *src;
{
    char *ptr;

    ptr = rt_vls_addr(src);
    while( *ptr ) {
	if( *ptr == '\\' )
	    ++ptr;
	if( *ptr == '\0' )
	    break;
	rt_vls_putc( dest, *ptr++ );
    }
}

void
backslash_specials( dest, src )
struct rt_vls *dest, *src;
{
    int backslashed;
    char *ptr, buf[2];

    buf[1] = '\0';
    backslashed = 0;
    for( ptr = rt_vls_addr( src ); *ptr; ptr++ ) {
	if( *ptr == '[' && !backslashed )
	    rt_vls_strcat( dest, "\\[" );
	else if( *ptr == ']' && !backslashed )
	    rt_vls_strcat( dest, "\\]" );
	else if( backslashed ) {
	    rt_vls_strcat( dest, "\\" );
	    buf[0] = *ptr;
	    rt_vls_strcat( dest, buf );
	    backslashed = 0;
	} else if( *ptr == '\\' )
	    backslashed = 1;
	else {
	    buf[0] = *ptr;
	    rt_vls_strcat( dest, buf );
	}
    }
}

/*                    M G E D _ C O M P A T
 *
 * This routine is called to perform wildcard expansion and character quoting
 * on the given vls (typically input from the keyboard.)
 */

int
mged_compat( dest, src )
struct rt_vls *dest, *src;
{
    char *start, *end;          /* Start and ends of words */
    int regexp;                 /* Set to TRUE when word is a regexp */
    int backslashed;
    int firstword;
    struct rt_vls word;         /* Current word being processed */
    struct rt_vls temp;

    rt_vls_init( &word );
    rt_vls_init( &temp );
    
    start = end = rt_vls_addr( src );
    firstword = 1;
    while( *end != '\0' ) {            /* Run through entire string */

	/* First, pass along leading whitespace. */

	start = end;                   /* Begin where last word ended */
	while( *start != '\0' ) {
	    if( *start == ' '  ||
	        *start == '\t' ||
	        *start == '\n' )
		rt_vls_putc( dest, *start++ );
	    else
		break;
	}
	if( *start == '\0' )
	    break;

	/* Next, advance "end" pointer to the end of the word, while adding
	   each character to the "word" vls.  Also make a note of any
	   unbackslashed wildcard characters. */

	end = start;
	rt_vls_trunc( &word, 0 );
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
	    rt_vls_putc( &word, *end++ );
	}

	if( firstword )
	    regexp = 0;

	/* Now, if the word was suspected of being a wildcard, try to match
	   it to the database. */

	if( regexp ) {
	    rt_vls_trunc( &temp, 0 );
	    if( regexp_match_all(&temp, rt_vls_addr(&word)) == 0 ) {
		debackslash( &temp, &word );
		backslash_specials( dest, &temp );
	    } else
		rt_vls_vlscat( dest, &temp );
	} else {
	    debackslash( dest, &word );
	}

	firstword = 0;
    }

    rt_vls_free( &temp );
    rt_vls_free( &word );
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
struct rt_vls *vp;
int record;
{
    int	status;
    struct rt_vls globbed;
    struct timeval start, finish;
    size_t len;
    extern struct rt_vls mged_prompt;

    RT_VLS_CHECK(vp);

    if (rt_vls_strlen(vp) <= 0) return 0;
		
    rt_vls_init(&globbed);


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
	rt_vls_vlscat(&globbed, vp);

    gettimeofday(&start, (struct timezone *)NULL);
    status = Tcl_Eval(interp, rt_vls_addr(&globbed));
    gettimeofday(&finish, (struct timezone *)NULL);

    /* Contemplate the result reported by the Tcl interpreter. */

    switch (status) {
    case TCL_OK:
	len = strlen(interp->result);

    /* If the command had something to say, print it out. */	     

	if (len > 0) rt_log("%s%s", interp->result,
			    interp->result[len-1] == '\n' ? "" : "\n");

    /* Then record it in the history, if desired. */

	if (record) history_record(vp, &start, &finish, CMD_OK);

	rt_vls_free(&globbed);
#ifdef XMGED
	addtohist(rt_vls_addr(vp));
	hcurr = htail;
#endif
	rt_vls_strcpy(&mged_prompt, MGED_PROMPT);
	return CMD_OK;

    case TCL_ERROR:
    default:

    /* First check to see if it's a secret message from cmd_wrapper. */

	if (strstr(interp->result, MORE_ARGS_STR) == interp->result) {
	    rt_vls_strcpy(&mged_prompt,interp->result+sizeof(MORE_ARGS_STR)-1);
	    Tcl_SetResult(interp, rt_vls_addr(&mged_prompt), TCL_VOLATILE);
	    rt_vls_free(&globbed);
	    return CMD_MORE;
	}

    /* Otherwise, it's just a regular old error. */    

	len = strlen(interp->result);
	if (len > 0) rt_log("%s%s", interp->result,
			    interp->result[len-1] == '\n' ? "" : "\n");

	if (record) history_record(vp, &start, &finish, CMD_BAD);
	rt_vls_free(&globbed);
#ifdef XMGED
	addtohist(rt_vls_addr(vp));
	hcurr = htail;
#endif
	rt_vls_strcpy(&mged_prompt, MGED_PROMPT);
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
mged_cmd(argc, argv, functions)
int argc;
char **argv;
struct funtab functions[];
{
    register struct funtab *ftp;
#ifdef XMGED
    AliasList	curr;
    struct rt_vls	cmd;
    int result;
    int	i, cmp;
    int 	save_journal;
#endif

    if (argc == 0)
	return CMD_OK;	/* No command entered, that's fine */

#ifdef XMGED
    /* check for aliases first */
    for(curr = atop; curr != NULL;){
	if((cmp = strcmp(argv[0], curr->name)) == 0){
	    if(curr->marked)
      /* alias has same name as real command, so call real command */
		break;

      /* repackage alias commands with any arguments and call cmdline again */
	    save_journal = journal;
	    journal = 0;	/* temporarily shut off journalling */
	    rt_vls_init( &cmd );
	    curr->marked = 1;
	    if(!extract_alias_def(&cmd, curr, argc, argv))
		(void)cmdline(&cmd, False);
	    
	    rt_vls_free( &cmd );
	    curr->marked = 0;
	    journal = save_journal;	/* restore journal state */
	    return CMD_OK;
	}else if(cmp > 0)
	    curr = curr->right;
	else
	    curr = curr->left;
    }
#endif

    for (ftp = &functions[1]; ftp->ft_name; ftp++) {
	if (strcmp(ftp->ft_name, argv[0]) != 0)
	    continue;
	/* We have a match */
	if ((ftp->ft_min <= argc) && (argc <= ftp->ft_max)) {
	    /* Input has the right number of args.
	     * Call function listed in table, with
	     * main(argc, argv) style args
	     */
#ifdef XMGED
	    result = ftp->ft_func(argc, argv);

/*  This needs to be done here in order to handle multiple commands within
    an alias.  */
	    if( sedraw > 0) {
		sedit();
		sedraw = 0;
		dmaflag = 1;
	    }
	    return result;
#else
	    switch (ftp->ft_func(argc, argv)) {
	    case CMD_OK:
		return CMD_OK;
	    case CMD_BAD:
		return CMD_BAD;
	    case CMD_MORE:
		return CMD_MORE;
	    default:
		rt_log("mged_cmd(): Invalid return from %s\n",
		       ftp->ft_name);
		return CMD_BAD;
	    }
#endif
	}
	rt_log("Usage: %s%s %s\n\t(%s)\n",functions[0].ft_name,
	       ftp->ft_name, ftp->ft_parms, ftp->ft_comment);
	return CMD_BAD;
    }
    rt_log("%s%s: no such command, type '%s?' for help\n",
	   functions[0].ft_name, argv[0], functions[0].ft_name);
    return CMD_BAD;
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
f_comm( argc, argv )
int	argc;
char	**argv;
{

	register int pid, rpid;
	int retcode;

	(void)signal( SIGINT, SIG_IGN );
	if ( ( pid = fork()) == 0 )  {
		(void)signal( SIGINT, SIG_DFL );
		(void)execl("/bin/sh","-",(char *)NULL);
		perror("/bin/sh");
		mged_finish( 11 );
	}

#ifdef XMGED
	while ((rpid = mged_wait(&retcode, pid)) != pid && rpid != -1)
#else
	while ((rpid = wait(&retcode)) != pid && rpid != -1)
#endif
		;
	(void)signal(SIGINT, cur_sigint);
	rt_log("!\n");

	return CMD_OK;  /* ? */
}

/* Quit and exit gracefully */
/* Format: q	*/

int
f_quit( argc, argv )
int	argc;
char	**argv;
{
	if( state != ST_VIEW )
		button( BE_REJECT );
	quit();			/* Exiting time */
	/* NOTREACHED */
}

/* wrapper for sync() */

int
f_sync(argc, argv)
int argc;
char **argv;
{

    register int i;
    sync();
    
    return CMD_OK;
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
			rt_log("Usage: %s%s %s\n\t(%s)\n", functions->ft_name,
			    ftp->ft_name, ftp->ft_parms, ftp->ft_comment);
			break;
		}
		if( !ftp->ft_name ) {
			rt_log("%s%s: no such command, type '%s?' for help\n",
			    functions->ft_name, argv[i], functions->ft_name);
			bad = 1;
		}
	}

	return bad ? CMD_BAD : CMD_OK;
}

/*
 *			F _ H E L P
 *
 *  Print a help message, two lines for each command.
 *  Or, help with the indicated commands.
 */
int f_help2();

int
f_help( argc, argv )
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
		rt_log("The following commands are available:\n");
		for( ftp = functions+1; ftp->ft_name; ftp++ )  {
			rt_log("%s%s %s\n\t(%s)\n", functions->ft_name,
			    ftp->ft_name, ftp->ft_parms, ftp->ft_comment);
		}
		return CMD_OK;
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
	struct rt_vls		str;

	rt_vls_init(&str);
	for( ftp = &funtab[1]; ftp->ft_name; ftp++ )  {
		vls_col_item( &str, ftp->ft_name);
	}
	vls_col_eol( &str );

	/* XXX should be rt_vls_strgrab() */
	Tcl_SetResult(interp, rt_vls_strdup( &str), TCL_DYNAMIC);

	rt_vls_free(&str);
	return TCL_OK;
}

int
f_fhelp2( argc, argv, functions)
int	argc;
char	**argv;
struct funtab *functions;
{
	register struct funtab *ftp;
	struct rt_vls		str;

	if( argc <= 1 )  {
		rt_vls_init(&str);
		rt_log("The following %scommands are available:\n",
		    functions->ft_name);
		for( ftp = functions+1; ftp->ft_name; ftp++ )  {
			vls_col_item( &str, ftp->ft_name);
		}
		vls_col_eol( &str );
		rt_log("%s", rt_vls_addr( &str ) );
		rt_vls_free(&str);
		return CMD_OK;
	}
	return helpcomm( argc, argv, functions );
}

/* Hook for displays with no buttons */
int
f_press( argc, argv )
int	argc;
char	**argv;
{
	register int i;

	for( i = 1; i < argc; i++ )
		press( argv[i] );

	return CMD_OK;
}

int
f_summary( argc, argv )
int	argc;
char	**argv;
{
	register char *cp;
	int flags = 0;
	int bad;

	bad = 0;
	if( argc <= 1 )  {
		dir_summary(0);
		return CMD_OK;
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
			rt_log("summary:  S R or G are only valid parmaters\n");
			bad = 1;
			break;
	}
	dir_summary(flags);
	return bad ? CMD_BAD : CMD_OK;
}

#ifdef XMGED
/*
 *			S O U R C E _ F I L E
 *
 */
void
mged_source_file(fp, option)
register FILE	*fp;
char option;
{
	struct rt_vls	str, cmd;
	int		len;
	int             cflag = 0;
	int record;

	switch(option){
	case 'h':
	case 'H':
		rt_vls_init(&str);
		rt_vls_init(&cmd);

		while( (len = rt_vls_gets( &str, fp )) >= 0 )  {
		  if(str.vls_str[len - 1] == '\\'){ /* continuation */
			str.vls_str[len - 1] = ' ';
                        cflag = 1;
                  }

                  rt_vls_strcat(&cmd, str.vls_str);

                  if(!cflag){/* no continuation, so add to history list */
                    rt_vls_strcat( &str, "\n" );

                    if( cmd.vls_len > 0 ){
                       addtohist( rt_vls_addr(&cmd) );
                       rt_vls_trunc( &cmd, 0 );
		    }
		  }else
                    cflag = 0;

                  rt_vls_trunc( &str, 0 );
		}

		rt_vls_free(&str);
		rt_vls_free(&cmd);
		break;
	case 'e':
        case 'E':
        case 'b':
        case 'B':
		if(option == 'e' || option == 'E')
		  record = 0;
		else
		  record = 1;

		rt_vls_init(&str);
		rt_vls_init(&cmd);

		while( (len = rt_vls_gets( &str, fp )) >= 0 ){
		  if(str.vls_str[len - 1] == '\\'){ /* continuation */
			str.vls_str[len - 1] = ' ';
                        cflag = 1;
                  }

                  rt_vls_strcat(&cmd, str.vls_str);

                  if(!cflag){/* no continuation, so execute command */
                    rt_vls_strcat( &cmd, "\n" );

                    if( cmd.vls_len > 0 ){
                      cmdline( &cmd, record );
                      rt_vls_trunc( &cmd, 0 );
                    }
                  }else
		    cflag = 0;

                  rt_vls_trunc( &str, 0 );
		}

		rt_vls_free(&str);
		rt_vls_free(&cmd);
		break;
	default:
		rt_log( "Unknown option: %c\n", option);
		break;
	}
}
#endif

/*
 *                          C M D _ E C H O
 *
 * Concatenates its arguments and "rt_log"s the resulting string.
 */

int
cmd_echo(clientData, interp, argc, argv)
ClientData clientData;
Tcl_Interp *interp;
int	argc;
char	*argv[];
{
    register int i;

    for( i=1; i < argc; i++ )  {
	rt_log( i==1 ? "%s" : " %s", argv[i] );
    }
    rt_log("\n");

    return TCL_OK;
}

#ifdef XMGED
int
f_alias(argc, argv)
int	argc;
char	*argv[];
{
	int	i;
	int	namelen, deflen;
	int	cmp;
	AliasList curr, prev;

	if(argc == 1){
		print_alias(True, NULL, atop);
		return CMD_BAD;
	}

	if(argc == 2){
		print_alias(False, argv[1], atop);
		return CMD_BAD;
	}

	namelen = strlen(argv[1]) + 1;

	/* find length of alias definition */
	for(deflen = 0, i = 2; i < argc; ++i)
		deflen += strlen(argv[i]) + 1;

	if(atop != NULL){
		/* find a spot in the binary tree for the new node */
		for(curr = atop; curr != NULL;){
			if((cmp = strcmp(argv[1], curr->name)) == 0){
			/* redefine the alias */
				free((void *)curr->def);
				curr->def = (char *)malloc(deflen + 1);
				load_alias_def(curr->def, argc - 2, argv + 2);
				return CMD_OK;
			}else if(cmp > 0){
				prev = curr;
				curr = curr->right;
			}else{
				prev = curr;
				curr = curr->left;
			}
		}
		/* create the new node and initialize */
		if(cmp > 0){
			prev->right = get_alias_node();
			curr = prev->right;
		}else{
			prev->left = get_alias_node();
			curr = prev->left;
		}
	}else {	/* atop == NULL */
		atop = get_alias_node();
		curr = atop;
	}

	curr->name = (char *)malloc(namelen + 1);
	curr->def = (char *)malloc(deflen + 1);
	strcpy(curr->name, argv[1]);
	load_alias_def(curr->def, argc - 2, argv + 2);

	balance_alias_tree();

	return CMD_OK;
}

int
f_unalias(argc, argv)
int	argc;
char	*argv[];
{
	int	i;
	int	cmp, o_cmp;
	AliasList	prev, curr;
	AliasList	right;

	if(atop == NULL)
		return CMD_BAD;

	for(i = 1; i < argc; ++i){
		prev = curr = atop;
		while(curr != NULL){
			if((cmp = strcmp(argv[1], curr->name)) == 0){
				if(prev == curr){	/* tree top gets deleted */
					if(curr->left != NULL){
						atop = curr->left;
						right = curr->right;
						free_alias_node(curr);
						for(curr = atop; curr->right != NULL; curr = curr->right);
						curr->right = right;
					}else{
						atop = curr->right;
						free_alias_node(curr);
					}
				}else{
					if(curr->left != NULL){
						if(o_cmp > 0)
							prev->right = curr->left;
						else
							prev->left = curr->left;

						prev = curr->left;
						right = curr->right;
						free_alias_node(curr);
						for(; prev->right != NULL; prev = prev->right);
						prev->right = right;
					}else{
						if(o_cmp > 0)
							prev->right = curr->right;
						else
							prev->left = curr->right;

						free_alias_node(curr);
					}
				}
				break;
			}else if(cmp > 0){
				prev = curr;
				curr = curr->right;
			}else{
				prev = curr;
				curr = curr->left;
			}

			o_cmp = cmp;
		}/* while */
	}/* for */

	balance_alias_tree();
	return CMD_OK;
}


static void
print_alias(printall, alias, curr)
int	printall;
char	*alias;
AliasList	curr;
{
	int	cmp;

	if(curr == NULL)
		return;

	if(printall){
		print_alias(printall, alias, curr->left);
		rt_log( "%s\t%s\n", curr->name, curr->def);
		print_alias(printall, alias, curr->right);
	}else{	/* print only one */
		while(curr != NULL){
			if((cmp = strcmp(alias, curr->name)) == 0){
				rt_log( "%s\t%s\n", curr->name, curr->def);
				return;
			}else if(cmp > 0)
				curr = curr->right;
			else
				curr = curr->left;
		}
	}
}


static void
load_alias_def(def, num, params)
char	*def;
int	num;
char	*params[];
{
	int	i;

	strcpy(def, params[0]);

	for(i = 1; i < num; ++i){
		strcat(def, " ");
		strcat(def, params[i]);
	}
}


static AliasList
get_alias_node()
{
	AliasList	curr;

	if(alias_free == NULL)
		curr = (AliasList)malloc(sizeof(Alias));
	else{
		curr = alias_free;
		alias_free = alias_free->right;	/* using only right in this freelist */
	}

	curr->left = NULL;
	curr->right = NULL;
	curr->marked = 0;
	return(curr);
}


static void
free_alias_node(node)
AliasList	node;
{
	free((void *)node->name);
	free((void *)node->def);
	node->right = alias_free;
	alias_free = node;
}


static void
balance_alias_tree()
{
	int	i = 0;
}

/*
 *	Extract the alias definition and insert arguments that where called for.
 * If there are any left over arguments, tack them on the end.
 */
static int
extract_alias_def(cmd, curr, argc, argv)
struct rt_vls *cmd;
AliasList	curr;
int	argc;
char	*argv[];
{
	int	i = 0;
	int	j;
	int	start = 0;	/* used for range of arguments */
	int	end = 0;
	int	*arg_used;	/* used to keep track of which arguments have been used */
	char	*p;

p = curr->def;
arg_used = (int *)calloc(argc, sizeof(int));

while(p[i] != '\0'){
	/* copy a command name or other stuff embedded in the alias */
	for(j = i; p[j] != '$' && p[j] != ';'
		&& p[j] != '\0'; ++j);

	rt_vls_strncat( cmd, p + i, j - i);

	i = j;

	if(p[i] == '\0')
		break;
	else if(p[i] == ';'){
		rt_vls_strcat(cmd, "\n");
		++i;
	}else{		/* parse the variable argument string */
		++i;
		if(p[i] < '0' || p[i] > '9'){
			rt_log( "%s\n", curr->def);
			rt_log( "Range must be numeric.\n");
			return(1);	/* Bad */
		}

		sscanf(p + i, "%d", &start);
		if(start < 1 || start > argc){
			rt_log( "%s\n", curr->def);
			rt_log( "variable argument number is out of range.\n");
                        return(1);      /* Bad */
		}
		if(p[++i] == '-'){
			++i;
			if(p[i] < '0' || p[i] > '9')	/* put rest of args here */
				end = argc;
			else{
				sscanf(p + i, "%d", &end);
				++i;
			}

			if(end < start || end > argc){
				rt_log( "%s\n", curr->def);
				rt_log( "variable argument number is out of range.\n");
	                        return(1);      /* Bad */
			}
		}else
			end = start;

		for(; start <= end; ++start){
			arg_used[start] = 1;	/* mark used */
			rt_vls_strcat(cmd, argv[start]);
			rt_vls_strcat(cmd, " ");
		}
	}
}/* while */

/* tack unused arguments on the end */
for(j = 1; j < argc; ++j){
	if(!arg_used[j]){
		rt_vls_strcat(cmd, " ");
		rt_vls_strcat(cmd, argv[j]);
	}
}

rt_vls_strcat(cmd, "\n");
return(0);
}

int
f_button(argc, argv)
int	argc;
char	*argv[];
{
  int	save_journal;
  int result;

  if(button_hook){
    save_journal = journal;
    journal = 0;
    result = (*button_hook)(atoi(argv[1]));
    journal = save_journal;
    return result;
  }

  rt_log( "button: currently not available for this display manager\n");
  return CMD_BAD;
}

int
f_slider(argc, argv)
int	argc;
char	*argv[];
{
  int	save_journal;
  int result;

  if(slider_hook){
    save_journal = journal;
    journal = 0;
    result = (*slider_hook)(atoi(argv[1]), atoi(argv[2]));
    journal = save_journal;
    return result;
  }

  rt_log( "slider: currently not available for this display manager\n");
  return CMD_BAD;
}

int
f_savedit(argc, argv)
int	argc;
char	*argv[];
{
  struct rt_vls str;
  char	line[35];
  int o_ipathpos;
  register struct solid *o_illump;

  o_illump = illump;
  rt_vls_init(&str);

  if(state == ST_S_EDIT){
    rt_vls_strcpy( &str, "press accept\npress sill\n" );
    cmdline(&str, 0);
    illump = o_illump;
    rt_vls_strcpy( &str, "M 1 0 0\n");
    cmdline(&str, 0);
    return CMD_OK;
  }else if(state == ST_O_EDIT){
    o_ipathpos = ipathpos;
    rt_vls_strcpy( &str, "press accept\npress oill\n" );
    cmdline(&str, 0);
    (void)chg_state( ST_O_PICK, ST_O_PATH, "savedit");
    illump = o_illump;
    ipathpos = o_ipathpos;
    rt_vls_strcpy( &str, "M 1 0 0\n");
    cmdline(&str, 0);
    return CMD_OK;
  }

  rt_log( "Savedit will only work in an edit state\n");
  rt_vls_free(&str);
  return CMD_BAD;
}

int
f_slewview(argc, argv)
int	argc;
char	*argv[];
{
	int x, y;
	vect_t tabvec;

	sscanf(argv[1], "%d", &x);
	sscanf(argv[2], "%d", &y);

	tabvec[X] =  x / 2047.0;
	tabvec[Y] =  y / 2047.0;
	tabvec[Z] = 0;
	slewview( tabvec );

	return CMD_OK;
}

int
f_openw(argc, argv)
int     argc;
char    *argv[];
{
  if(openw_hook){
  	if(argc == 1)
		return (*openw_hook)(NULL);
  	else
		return (*openw_hook)(argv[1]);
  }

  rt_log( "openw: currently not available\n");
  return CMD_BAD;
}

int
f_closew(argc, argv)
int     argc;
char    *argv[];
{
  if(closew_hook)
    return (*closew_hook)(argv[1]);

  rt_log( "closew: currently not available\n");
  return CMD_BAD;
}

int
f_cue(argc, argv)
int     argc;
char    *argv[];
{
  if(cue_hook)
    return (*cue_hook)();


  rt_log( "cue: currently not available\n");
  return CMD_BAD;
}

int
f_zclip(argc, argv)
int     argc;
char    *argv[];
{
  if(zclip_hook)
    return (*zclip_hook)();

  rt_log( "zclip: currently not available\n");
  return CMD_BAD;
}

int
f_zbuffer(argc, argv)
int     argc;
char    *argv[];
{
  if(zbuffer_hook)
    return (*zbuffer_hook)();

  rt_log( "zbuffer: currently not available\n");
  return CMD_BAD;
}

int
f_light(argc, argv)
int     argc;
char    *argv[];
{
  if(light_hook)
    return (*light_hook)();

  rt_log( "light: currently not available\n");
  return CMD_BAD;
}

int
f_perspective(argc, argv)
int     argc;
char    *argv[];
{
  int i;
  static int perspective_angle = 0;
  static int perspective_mode = 0;
  static int perspective_table[] = { 30, 45, 60, 90 };

  if(argc == 2){
    perspective_mode = 1;
    sscanf(argv[1], "%d", &i);
    if(i < 0 || i > 3){
      if (--perspective_angle < 0) perspective_angle = 3;
    }else
      perspective_angle = i;

    rt_vls_printf( &dm_values.dv_string,
		  "set perspective %d\n",
		  perspective_table[perspective_angle] );
  }else{
    perspective_mode = 1 - perspective_mode;
    rt_vls_printf( &dm_values.dv_string,
		  "set perspective %d\n",
		  perspective_mode ? perspective_table[perspective_angle] : -1 );
  }

  update_views = 1;
  return CMD_OK;
}

static void
make_command(line, diff)
char	*line;
point_t	diff;
{

  if(state == ST_O_EDIT)
    (void)sprintf(line, "translate %f %f %f\n",
		  (e_axis_pos[X] + diff[X]) * base2local,
		  (e_axis_pos[Y] + diff[Y]) * base2local,
		  (e_axis_pos[Z] + diff[Z]) * base2local);
  else
    (void)sprintf(line, "p %f %f %f\n",
		  (e_axis_pos[X] + diff[X]) * base2local,
		  (e_axis_pos[Y] + diff[Y]) * base2local,
		  (e_axis_pos[Z] + diff[Z]) * base2local);

}

/*
 *                         S E T _ T R A N
 *
 * Calculate the values for tran_x, tran_y, and tran_z.
 *
 *
 */
void
set_tran(x, y, z)
fastf_t x, y, z;
{
  point_t diff;

  diff[X] = x - e_axis_pos[X];
  diff[Y] = y - e_axis_pos[Y];
  diff[Z] = z - e_axis_pos[Z];
  
  /* If there is more than one active view, then tran_x/y/z
     needs to be initialized for each view. */
  if(set_tran_hook)
    (*set_tran_hook)(diff);
  else{
    point_t old_pos;
    point_t new_pos;
    point_t view_pos;

    MAT_DELTAS_GET_NEG(old_pos, toViewcenter);
    VADD2(new_pos, old_pos, diff);
    MAT4X3PNT(view_pos, model2view, new_pos);

    tran_x = view_pos[X];
    tran_y = view_pos[Y];
    tran_z = view_pos[Z];
  }
}

void
set_e_axis_pos()
{
  int	i;

  /* If there is more than one active view, then tran_x/y/z
     needs to be initialized for each view. */
  if(set_tran_hook){
    point_t pos;

    VSETALL(pos, 0.0);
    (*set_tran_hook)(pos);
  }else{
    tran_x = tran_y = tran_z = 0;
  }

  if(rot_hook)
    (*rot_hook)();

  irot_x = irot_y = irot_z = 0;
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

    VMOVE(e_axis_pos, ((struct rt_arb_internal *)es_int.idb_ptr)->pt[i]);
    break;
  case ID_TGC:
  case ID_REC:
    if(es_edflag == ECMD_TGC_MV_H ||
       es_edflag == ECMD_TGC_MV_HH){
      struct rt_tgc_internal  *tgc = (struct rt_tgc_internal *)es_int.idb_ptr;

      VADD2(e_axis_pos, tgc->h, tgc->v);
      break;
    }
  default:
    VMOVE(e_axis_pos, es_keypoint);
    break;
  }
}

int
f_tran(argc, argv)
int     argc;
char    *argv[];
{
  double x, y, z;
  int itran;
  char cmd[128];
  vect_t view_pos;
  point_t old_pos;
  point_t new_pos;
  point_t diff;
  struct rt_vls str;
  int save_journal = journal;

  rt_vls_init(&str);
  
  sscanf(argv[1], "%lf", &x);
  sscanf(argv[2], "%lf", &y);
  sscanf(argv[3], "%lf", &z);

  itran = !strcmp(argv[0], "itran");

  if(itran){
    tran_x += x;
    tran_y += y;
    tran_z += z;
  }else{
    tran_x = x;
    tran_y = y;
    tran_z = z;
  }

  VSET(view_pos, tran_x, tran_y, tran_z);
  MAT4X3PNT( new_pos, view2model, view_pos );
  MAT_DELTAS_GET_NEG(old_pos, toViewcenter);
  VSUB2( diff, new_pos, old_pos );

  if(state == ST_O_EDIT)
    make_command(cmd, diff);
  else if(state == ST_S_EDIT)
    make_command(cmd, diff);
  else{
    VADD2(new_pos, orig_pos, diff);
    MAT_DELTAS_VEC( toViewcenter, new_pos);
    MAT_DELTAS_VEC( ModelDelta, new_pos);
    new_mats();

    if(tran_hook)
      (*tran_hook)();

    rt_vls_free(&str);
    save_journal = journal;
    return CMD_OK;
  }

  tran_set = 1;
  rt_vls_strcpy( &str, cmd );
  cmdline(&str, False);
  rt_vls_free(&str);
  tran_set = 0;
  save_journal = journal;

  /* If there is more than one active view, then tran_x/y/z
     needs to be initialized for each view. */
  if(set_tran_hook)
    (*set_tran_hook)(diff);

  return CMD_OK;
}

int
f_irot(argc, argv)
int argc;
char *argv[];
{
  int save_journal = journal;
  double x, y, z;
  char cmd[128];
  struct rt_vls str;
  vect_t view_pos;
  point_t new_pos;
  point_t old_pos;
  point_t diff;

  journal = 0;

  rt_vls_init(&str);

  sscanf(argv[1], "%lf", &x);
  sscanf(argv[2], "%lf", &y);
  sscanf(argv[3], "%lf", &z);

  irot_x += x;
  irot_y += y;
  irot_z += z;

  MAT_DELTAS_GET(old_pos, toViewcenter);

  if(state == ST_VIEW)
    sprintf(cmd, "vrot %f %f %f\n", x, y, z);
  else if(state == ST_O_EDIT)
    sprintf(cmd, "rotobj %f %f %f\n", irot_x, irot_y, irot_z);
  else if(state == ST_S_EDIT)
    sprintf(cmd, "p %f %f %f\n", irot_x, irot_y, irot_z);

  irot_set = 1;
  rt_vls_strcpy( &str, cmd );
  cmdline(&str, False);
  rt_vls_free(&str);
  irot_set = 0;

  if(state == ST_VIEW){
    MAT_DELTAS_GET_NEG(new_pos, toViewcenter);
    VSUB2(diff, new_pos, orig_pos);
    VADD2(new_pos, old_pos, diff);
    VSET(view_pos, new_pos[X], new_pos[Y], new_pos[Z]);
    MAT4X3PNT( new_pos, model2view, view_pos);
    tran_x = new_pos[X];
    tran_y = new_pos[Y];
    tran_z = new_pos[Z];

    if(tran_hook)
      (*tran_hook)();
  }

  journal = save_journal;
  return CMD_OK;
}

int
f_ps(argc, argv)
int argc;
char *argv[];
{
  struct dm *o_dmp;
  void (*o_dotitles_hook)();
  int o_faceplate;
  static int windowbounds[] = {
    2047, -2048, 2047, -2048, 2047, -2048
  };

  o_faceplate = mged_variables.faceplate;
  if(argc == 3){
    if(*argv[1] == 'f'){
      mged_variables.faceplate = 1;
      ++argv;
    }else{
      rt_log( "Usage: ps filename [f]\n");
      return CMD_BAD;
    }
  }else
    mged_variables.faceplate = 0;

  if( (ps_fp = fopen( argv[1], "w" )) == NULL )  {
	perror(argv[1]);
	return CMD_BAD;
  }

  o_dotitles_hook = dotitles_hook;
  dotitles_hook = NULL;
  o_dmp = dmp;
  dmp = &dm_PS;
  dmp->dmr_window(windowbounds);
#if 0
  if(dmp->dmr_open())
	goto clean_up;
#else

	setbuf( ps_fp, ps_ttybuf );

	mged_fputs( "%!PS-Adobe-1.0\n\
%begin(plot)\n\
%%DocumentFonts:  Courier\n", ps_fp );
	fprintf(ps_fp, "%%%%Title: %s\n", argv[1] );
	mged_fputs( "\
%%Creator: MGED dm-ps.c\n\
%%BoundingBox: 0 0 324 324	% 4.5in square, for TeX\n\
%%EndComments\n\
\n", ps_fp );

	mged_fputs( "\
4 setlinewidth\n\
\n\
% Sizes, made functions to avoid scaling if not needed\n\
/FntH /Courier findfont 80 scalefont def\n\
/DFntL { /FntL /Courier findfont 73.4 scalefont def } def\n\
/DFntM { /FntM /Courier findfont 50.2 scalefont def } def\n\
/DFntS { /FntS /Courier findfont 44 scalefont def } def\n\
\n\
% line styles\n\
/NV { [] 0 setdash } def	% normal vectors\n\
/DV { [8] 0 setdash } def	% dotted vectors\n\
/DDV { [8 8 32 8] 0 setdash } def	% dot-dash vectors\n\
/SDV { [32 8] 0 setdash } def	% short-dash vectors\n\
/LDV { [64 8] 0 setdash } def	% long-dash vectors\n\
\n\
/NEWPG {\n\
	.0791 .0791 scale	% 0-4096 to 324 units (4.5 inches)\n\
} def\n\
\n\
FntH  setfont\n\
NEWPG\n\
", ps_fp);

	in_middle = 0;
#endif

  color_soltab();
  dmaflag = 1;
  refresh();
  dmp->dmr_close();
clean_up:
  dmp = o_dmp;
  dotitles_hook = o_dotitles_hook;
  mged_variables.faceplate = o_faceplate;
  return CMD_OK;
}

int
f_bindkey(argc, argv)
int argc;
char *argv[];
{
  if(bindkey_hook)
    return (*bindkey_hook)(argc, argv);

  rt_log( "bindkey: currently not available\n");
  return CMD_BAD;
}
#endif

