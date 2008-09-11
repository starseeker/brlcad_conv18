/*                       G E D _ O B J . C
 * BRL-CAD
 *
 * Copyright (c) 2000-2008 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @addtogroup libged */
/** @{ */
/** @file ged_obj.c
 *
 * A quasi-object-oriented database interface.
 *
 * A GED object contains the attributes and methods for
 * controlling a BRL-CAD geometry edit object.
 *
 */
/** @} */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include "bio.h"

#include "tcl.h"

#include "bu.h"
#include "bn.h"
#include "cmd.h"
#include "vmath.h"
#include "db.h"
#include "rtgeom.h"
#include "wdb.h"
#include "mater.h"
#include "tclcad.h"

#include "solid.h"
#include "dm.h"
#include "dm_xvars.h"

#if defined(DM_X) || defined(DM_TK)
#  include "tk.h"
#  include <X11/Xutil.h>
#endif /* DM_X or DM_TK*/

#ifdef DM_X
#  include "dm-X.h"
#endif /* DM_X */

#ifdef DM_TK
#  include "dm-tk.h"
#endif /* DM_TK */

#ifdef DM_OGL
#  include "dm-ogl.h"
#endif /* DM_OGL */

#ifdef DM_WGL
#  include <tkwinport.h>
#  include "dm-wgl.h"
#endif /* DM_WGL */

#if 1
/*XXX Temporary */
#include "dg.h"
#endif

static char go_grid_syntax[] = "\
 grid vname			toggle display of grid\n\
 grid vname vars		print a list of all variables (i.e. var = val)\n\
 grid vname draw [0|1]		set or get the draw parameter\n\
 grid vname snap [0|1]		set or get the snap parameter\n\
 grid vname rh [fval]		set or get the resolution (horizontal)\n\
 grid vname rv [fval]		set or get the resolution (vertical)\n\
 grid vname mrh [ival]		set or get the major resolution (horizontal)\n\
 grid vname mrv [ival]		set or get the major resolution (vertical)\n\
 grid vname color [r g b]	set or get the color\n\
 grid vname help		prints this help message\n\
";

static char go_rect_syntax[] = "\
 grid vname			toggle display of rectangle\n\
 grid vname color [r g b]	set or get the color\n\
 grid vname dim	w h		set or get the rectangle dimension\n\
 grid vname draw [0|1]		set or get the draw parameter\n\
 grid vname help		prints this help message\n\
 grid vname lstyle [0|1]	set or get the line style, 0 - solid, 1 - dashed\n\
 grid vname lwidth w		set or get the line width\n\
 grid vname pos	x y		set or get the rectangle position\n\
 grid vname vars		print a list of all variables (i.e. var = val)\n\
";


static int go_open_tcl(ClientData clientData,
		       Tcl_Interp *interp,
		       int argc,
		       const char **argv);
static int go_autoview(struct ged	*gedp,
		       int		argc,
		       const char	*argv[],
		       ged_func_ptr	func,
		       const char	*usage,
		       int		maxargs);
static int go_blast(struct ged		*gedp,
		    int			argc,
		    const char		*argv[],
		    ged_func_ptr	func,
		    const char		*usage,
		    int			maxargs);
static int go_configure(struct ged	*gedp,
			int		argc,
			const char	*argv[],
			ged_func_ptr	func,
			const char	*usage,
			int		maxargs);
static int go_constrain_rmode(struct ged	*gedp,
			      int		argc,
			      const char	*argv[],
			      ged_func_ptr	func,
			      const char	*usage,
			      int		maxargs);
static int go_constrain_tmode(struct ged	*gedp,
			      int		argc,
			      const char	*argv[],
			      ged_func_ptr	func,
			      const char	*usage,
			      int		maxargs);
static int go_delete_view(struct ged	*gedp,
			  int		argc,
			  const char	*argv[],
			  ged_func_ptr	func,
			  const char	*usage,
			  int		maxargs);
static int go_grid(struct ged	*gedp,
		   int		argc,
		   const char	*argv[],
		   ged_func_ptr	func,
		   const char	*usage,
		   int		maxargs);
static int go_idle_mode(struct ged	*gedp,
			int		argc,
			const char	*argv[],
			ged_func_ptr	func,
			const char	*usage,
			int		maxargs);
static int go_light(struct ged		*gedp,
		    int			argc,
		    const char		*argv[],
		    ged_func_ptr	func,
		    const char		*usage,
		    int			maxargs);
static int go_list_views(struct ged	*gedp,
			 int		argc,
			 const char	*argv[],
			 ged_func_ptr	func,
			 const char	*usage,
			 int		maxargs);

static int go_listen(struct ged		*gedp,
		     int		argc,
		     const char		*argv[],
		     ged_func_ptr	func,
		     const char		*usage,
		     int		maxargs);

static int go_make(struct ged	*gedp,
		   int		argc,
		   const char	*argv[],
		   ged_func_ptr	func,
		   const char	*usage,
		   int		maxargs);
static int go_mirror(struct ged		*gedp,
		     int		argc,
		     const char		*argv[],
		     ged_func_ptr	func,
		     const char		*usage,
		     int		maxargs);
static int go_mouse_constrain_rot(struct ged	*gedp,
				  int		argc,
				  const char	*argv[],
				  ged_func_ptr	func,
				  const char	*usage,
				  int		maxargs);
static int go_mouse_constrain_trans(struct ged		*gedp,
				    int			argc,
				    const char		*argv[],
				    ged_func_ptr	func,
				    const char		*usage,
				    int			maxargs);
static int go_mouse_rot(struct ged	*gedp,
			int		argc,
			const char	*argv[],
			ged_func_ptr	func,
			const char	*usage,
			int		maxargs);
static int go_mouse_scale(struct ged	*gedp,
			  int		argc,
			  const char	*argv[],
			  ged_func_ptr	func,
			  const char	*usage,
			  int		maxargs);
static int go_mouse_trans(struct ged	*gedp,
			  int		argc,
			  const char	*argv[],
			  ged_func_ptr	func,
			  const char	*usage,
			  int		maxargs);
static int go_new_view(struct ged	*gedp,
		       int		argc,
		       const char	*argv[],
		       ged_func_ptr	func,
		       const char	*usage,
		       int		maxargs);
static int go_paint_rect_area(struct ged	*gedp,
			      int		argc,
			      const char	*argv[],
			      ged_func_ptr	func,
			      const char	*usage,
			      int		maxargs);
static int go_rect(struct ged	*gedp,
		   int		argc,
		   const char	*argv[],
		   ged_func_ptr	func,
		   const char	*usage,
		   int		maxargs);
static int go_rect_zoom(struct ged	*gedp,
			int		argc,
			const char	*argv[],
			ged_func_ptr	func,
			const char	*usage,
			int		maxargs);
static int go_refresh(struct ged	*gedp,
		      int		argc,
		      const char	*argv[],
		      ged_func_ptr	func,
		      const char	*usage,
		      int		maxargs);
static int go_refresh_all(struct ged	*gedp,
			  int		argc,
			  const char	*argv[],
			  ged_func_ptr	func,
			  const char	*usage,
			  int		maxargs);
static int go_rotate_mode(struct ged	*gedp,
			  int		argc,
			  const char	*argv[],
			  ged_func_ptr	func,
			  const char	*usage,
			  int		maxargs);
static int go_rt_rect_area(struct ged	*gedp,
			   int		argc,
			   const char	*argv[],
			   ged_func_ptr	func,
			   const char	*usage,
			   int		maxargs);
static int go_rt_gettrees(struct ged	*gedp,
			  int		argc,
			  const char	*argv[],
			  ged_func_ptr	func,
			  const char	*usage,
			  int		maxargs);
static int go_scale_mode(struct ged	*gedp,
			 int		argc,
			 const char	*argv[],
			 ged_func_ptr	func,
			 const char	*usage,
			 int		maxargs);
static int go_set_coord(struct ged	*gedp,
			int		argc,
			const char	*argv[],
			ged_func_ptr	func,
			const char	*usage,
			int		maxargs);
static int go_set_fb_mode(struct ged	*gedp,
			  int		argc,
			  const char	*argv[],
			  ged_func_ptr	func,
			  const char	*usage,
			  int		maxargs);
static int go_translate_mode(struct ged		*gedp,
			     int		argc,
			     const char		*argv[],
			     ged_func_ptr	func,
			     const char		*usage,
			     int		maxargs);
static int go_vmake(struct ged		*gedp,
		    int			argc,
		    const char		*argv[],
		    ged_func_ptr	func,
		    const char		*usage,
		    int			maxargs);
static int go_vslew(struct ged		*gedp,
		    int			argc,
		    const char		*argv[],
		    ged_func_ptr	func,
		    const char		*usage,
		    int			maxargs);
static int go_vsnap(struct ged		*gedp,
		    int			argc,
		    const char		*argv[],
		    ged_func_ptr	func,
		    const char		*usage,
		    int			maxargs);
static int go_zbuffer(struct ged	*gedp,
		      int		argc,
		      const char	*argv[],
		      ged_func_ptr	func,
		      const char	*usage,
		      int		maxargs);
static int go_zclip(struct ged		*gedp,
		    int			argc,
		    const char		*argv[],
		    ged_func_ptr	func,
		    const char		*usage,
		    int			maxargs);

/* Wrapper Functions */
static int go_autoview_func(struct ged	*gedp,
			   int		argc,
			   const char	*argv[],
			   ged_func_ptr	func,
			   const char	*usage,
			   int		maxargs);
static int go_pass_through_func(struct ged	*gedp,
				int		argc,
				const char	*argv[],
				ged_func_ptr	func,
				const char	*usage,
				int		maxargs);
static int go_pass_through_and_refresh_func(struct ged		*gedp,
					    int			argc,
					    const char		*argv[],
					    ged_func_ptr	func,
					    const char		*usage,
					    int			maxargs);
static int go_view_func(struct ged	*gedp,
			int		argc,
			const char	*argv[],
			ged_func_ptr	func,
			const char	*usage,
			int		maxargs);

/* Utility Functions */
static void go_drawSolid(struct dm *dmp, struct solid *sp);
static int go_drawSList(struct dm *dmp, struct bu_list *hsp);

static int go_close_fbs(struct ged_dm_view *gdvp);
static void go_fbs_callback();
static int go_open_fbs(struct ged_dm_view *gdvp, Tcl_Interp *interp);

static void go_refresh_view(struct ged_dm_view *gdvp);
static void go_refresh_all_views(struct ged_obj *gop);
static void go_autoview_view(struct ged_dm_view *gdvp);
static void go_autoview_all_views(struct ged_obj *gop);

static void go_output_handler(struct ged *gedp, char *line);

typedef int (*go_wrapper_func_ptr)(struct ged *, int, const char *[], ged_func_ptr, const char *, int);
#define GO_WRAPPER_FUNC_PTR_NULL (go_wrapper_func_ptr)0

#define GO_MAX_RT_ARGS 64

static struct ged_obj HeadGedObj;
static struct ged_obj *go_current_gop = GED_OBJ_NULL;

#define GO_MAX_RT_ARGS 64

struct go_cmdtab {
    char	 *go_name;
    char	 *go_usage;
    int		 go_maxargs;
    go_wrapper_func_ptr	go_wrapper_func;
    ged_func_ptr go_func;
};

static struct go_cmdtab go_cmds[] = {
    {"3ptarb",	(char *)0, MAXARGS, go_pass_through_func, ged_3ptarb},
    {"adc",	"vname args", 7, go_view_func, ged_adc},
    {"adjust",	(char *)0, MAXARGS, go_pass_through_func, ged_adjust},
    {"ae2dir",	(char *)0, MAXARGS, go_pass_through_func, ged_ae2dir},
    {"aet",	"vname [[-i] az el [tw]]", 6, go_view_func, ged_aet},
    {"analyze",	(char *)0, MAXARGS, go_pass_through_func, ged_analyze},
    {"arb",	(char *)0, MAXARGS, go_pass_through_func, ged_arb},
    {"arced",	(char *)0, MAXARGS, go_pass_through_func, ged_arced},
    {"arot",	"vname x y z angle", 6, go_view_func, ged_arot},
    {"attr",	(char *)0, MAXARGS, go_pass_through_func, ged_attr},
    {"autoview",	"vname", MAXARGS, go_autoview, GED_FUNC_PTR_NULL},
    {"bev",	(char *)0, MAXARGS, go_pass_through_func, ged_bev},
    {"bo",	(char *)0, MAXARGS, go_pass_through_func, ged_binary},
    {"blast",	(char *)0, MAXARGS, go_blast, GED_FUNC_PTR_NULL},
    {"bot_condense",	(char *)0, MAXARGS, go_pass_through_func, ged_bot_condense},
    {"bot_decimate",	(char *)0, MAXARGS, go_pass_through_func, ged_bot_decimate},
    {"bot_face_fuse",	(char *)0, MAXARGS, go_pass_through_func, ged_bot_face_fuse},
    {"bot_face_sort",	(char *)0, MAXARGS, go_pass_through_func, ged_bot_face_sort},
    {"bot_merge",	(char *)0, MAXARGS, go_pass_through_func, ged_bot_merge},
    {"bot_smooth",	(char *)0, MAXARGS, go_pass_through_func, ged_bot_smooth},
    {"bot_split",	(char *)0, MAXARGS, go_pass_through_func, ged_bot_split},
    {"bot_vertex_fuse",	(char *)0, MAXARGS, go_pass_through_func, ged_bot_vertex_fuse},
    {"c",	(char *)0, MAXARGS, go_pass_through_func, ged_comb_std},
    {"cat",	(char *)0, MAXARGS, go_pass_through_func, ged_cat},
    {"center",	"vname [x y z]", 5, go_view_func, ged_center},
    {"clear",	(char *)0, MAXARGS, go_pass_through_and_refresh_func, ged_zap},
    {"color",	(char *)0, MAXARGS, go_pass_through_func, ged_color},
    {"comb",	(char *)0, MAXARGS, go_pass_through_func, ged_comb},
    {"comb_color",	(char *)0, MAXARGS, go_pass_through_func, ged_comb_color},
    {"concat",	(char *)0, MAXARGS, go_pass_through_func, ged_concat},
    {"configure",	"vname", MAXARGS, go_configure, GED_FUNC_PTR_NULL},
    {"constrain_rmode",	"vname x|y|z x y", MAXARGS, go_constrain_rmode, GED_FUNC_PTR_NULL},
    {"constrain_tmode",	"vname x|y|z x y", MAXARGS, go_constrain_tmode, GED_FUNC_PTR_NULL},
    {"copyeval",	(char *)0, MAXARGS, go_pass_through_func, ged_copyeval},
    {"copymat",	(char *)0, MAXARGS, go_pass_through_func, ged_copymat},
    {"cp",	(char *)0, MAXARGS, go_pass_through_func, ged_copy},
    {"cpi",	(char *)0, MAXARGS, go_pass_through_func, ged_cpi},
    {"d",	(char *)0, MAXARGS, go_pass_through_and_refresh_func, ged_erase},
    {"dall",	(char *)0, MAXARGS, go_pass_through_and_refresh_func, ged_erase_all},
    {"dbip",	(char *)0, MAXARGS, go_pass_through_func, ged_dbip},
    {"decompose",	(char *)0, MAXARGS, go_pass_through_func, ged_decompose},
    {"delay",	(char *)0, MAXARGS, go_pass_through_func, ged_delay},
    {"delete_view",	"vname", MAXARGS, go_delete_view, GED_FUNC_PTR_NULL},
    {"dir2ae",	(char *)0, MAXARGS, go_pass_through_func, ged_dir2ae},
    {"draw",	(char *)0, MAXARGS, go_autoview_func, ged_draw},
    {"dump",	(char *)0, MAXARGS, go_pass_through_func, ged_dump},
    {"dup",	(char *)0, MAXARGS, go_pass_through_func, ged_dup},
    {"E",	(char *)0, MAXARGS, go_autoview_func, ged_E},
    {"e",	(char *)0, MAXARGS, go_autoview_func, ged_draw},
    {"eac",	(char *)0, MAXARGS, go_autoview_func, ged_eac},
    {"edcodes",	(char *)0, MAXARGS, go_pass_through_func, ged_edcodes},
    {"edcomb",	(char *)0, MAXARGS, go_pass_through_func, ged_edcomb},
    {"edmater",	(char *)0, MAXARGS, go_pass_through_func, ged_edmater},
    {"erase",	(char *)0, MAXARGS, go_pass_through_and_refresh_func, ged_erase},
    {"erase_all",	(char *)0, MAXARGS, go_pass_through_and_refresh_func, ged_erase_all},
    {"ev",	(char *)0, MAXARGS, go_autoview_func, ged_ev},
    {"expand",	(char *)0, MAXARGS, go_pass_through_func, ged_expand},
    {"eye",	"vname [x y z]", 5, go_view_func, ged_eye},
    {"eye_pos",	"vname [x y z]", 5, go_view_func, ged_eye_pos},
    {"facetize",	(char *)0, MAXARGS, go_pass_through_func, ged_facetize},
    {"find",	(char *)0, MAXARGS, go_pass_through_func, ged_find},
    {"form",	(char *)0, MAXARGS, go_pass_through_func, ged_form},
    {"fracture",	(char *)0, MAXARGS, go_pass_through_func, ged_fracture},
    {"g",	(char *)0, MAXARGS, go_pass_through_func, ged_group},
    {"get",	(char *)0, MAXARGS, go_pass_through_func, ged_get},
    {"get_autoview",	(char *)0, MAXARGS, go_pass_through_func, ged_get_autoview},
    {"get_comb",	(char *)0, MAXARGS, go_pass_through_func, ged_get_comb},
    {"get_eyemodel",	"vname", 2, go_view_func, ged_get_eyemodel},
    {"get_type",	(char *)0, MAXARGS, go_pass_through_func, ged_get_type},
    {"glob",	(char *)0, MAXARGS, go_pass_through_func, ged_glob},
    {"grid",	go_grid_syntax, MAXARGS, go_grid, GED_FUNC_PTR_NULL},
    {"hide",	(char *)0, MAXARGS, go_pass_through_func, ged_hide},
    {"how",	(char *)0, MAXARGS, go_pass_through_func, ged_how},
    {"i",	(char *)0, MAXARGS, go_pass_through_func, ged_instance},
    {"idents",	(char *)0, MAXARGS, go_pass_through_func, ged_tables},
    {"idle_mode",	"vname", MAXARGS, go_idle_mode, GED_FUNC_PTR_NULL},
    {"illum",	(char *)0, MAXARGS, go_pass_through_and_refresh_func, ged_illum},
    {"importFg4Section",	(char *)0, MAXARGS, go_pass_through_func, ged_importFg4Section},
    {"isize",	"vname", 2, go_view_func, ged_isize},
    {"item",	(char *)0, MAXARGS, go_pass_through_func, ged_item},
    {"keep",	(char *)0, MAXARGS, go_pass_through_func, ged_keep},
    {"keypoint",	"vname [x y z]", 5, go_view_func, ged_keypoint},
    {"kill",	(char *)0, MAXARGS, go_pass_through_and_refresh_func, ged_kill},
    {"killall",	(char *)0, MAXARGS, go_pass_through_and_refresh_func, ged_killall},
    {"killrefs",	(char *)0, MAXARGS, go_pass_through_and_refresh_func, ged_killrefs},
    {"killtree",	(char *)0, MAXARGS, go_pass_through_and_refresh_func, ged_killtree},
    {"l",	(char *)0, MAXARGS, go_pass_through_func, ged_list},
    {"light",	"vname [0|1]", MAXARGS, go_light, GED_FUNC_PTR_NULL},
    {"list_views",	(char *)0, MAXARGS, go_list_views, GED_FUNC_PTR_NULL},
    {"listen",	"vname [port]", MAXARGS, go_listen, GED_FUNC_PTR_NULL},
    {"listeval",	(char *)0, MAXARGS, go_pass_through_func, ged_pathsum},
    {"loadview",	"vname filename", 3, go_view_func, ged_loadview},
    {"log",	(char *)0, MAXARGS, go_pass_through_func, ged_log},
    {"lookat",	"vname x y z", 5, go_view_func, ged_lookat},
    {"ls",	(char *)0, MAXARGS, go_pass_through_func, ged_ls},
    {"lt",	(char *)0, MAXARGS, go_pass_through_func, ged_lt},
    {"m2v_point",	"vname x y z", 5, go_view_func, ged_m2v_point},
    {"make",	(char *)0, MAXARGS, go_make, GED_FUNC_PTR_NULL},
    {"make_bb",	(char *)0, MAXARGS, go_pass_through_func, ged_make_bb},
    {"make_name",	(char *)0, MAXARGS, go_pass_through_func, ged_make_name},
    {"match",	(char *)0, MAXARGS, go_pass_through_func, ged_match},
    {"mater",	(char *)0, MAXARGS, go_pass_through_func, ged_mater},
    {"mirror",	(char *)0, MAXARGS, go_mirror, GED_FUNC_PTR_NULL},
    {"model2view",	"vname", 2, go_view_func, ged_model2view},
    {"move_arb_edge",	(char *)0, MAXARGS, go_pass_through_func, ged_move_arb_edge},
    {"move_arb_face",	(char *)0, MAXARGS, go_pass_through_func, ged_move_arb_face},
    {"mouse_constrain_rot",	"vname coord x y", MAXARGS, go_mouse_constrain_rot, GED_FUNC_PTR_NULL},
    {"mouse_constrain_trans",	"vname coord x y", MAXARGS, go_mouse_constrain_trans, GED_FUNC_PTR_NULL},
    {"mouse_rot",	"vname x y", MAXARGS, go_mouse_rot, GED_FUNC_PTR_NULL},
    {"mouse_scale",	"vname x y", MAXARGS, go_mouse_scale, GED_FUNC_PTR_NULL},
    {"mouse_trans",	"vname x y", MAXARGS, go_mouse_trans, GED_FUNC_PTR_NULL},
    {"mv",	(char *)0, MAXARGS, go_pass_through_func, ged_move},
    {"mvall",	(char *)0, MAXARGS, go_pass_through_func, ged_move_all},
    {"new_view",	"vname type [args]", MAXARGS, go_new_view, GED_FUNC_PTR_NULL},
    {"nirt",	"vname [args]", GO_MAX_RT_ARGS, go_view_func, ged_nirt},
    {"nmg_collapse",	(char *)0, MAXARGS, go_pass_through_func, ged_nmg_collapse},
    {"nmg_simplify",	(char *)0, MAXARGS, go_pass_through_func, ged_nmg_simplify},
    {"ocenter",	(char *)0, MAXARGS, go_pass_through_func, ged_ocenter},
    {"open",	(char *)0, MAXARGS, go_pass_through_and_refresh_func, ged_reopen},
    {"orient",	"vname quat", 6, go_view_func, ged_orient},
    {"orotate",	(char *)0, MAXARGS, go_pass_through_func, ged_orotate},
    {"oscale",	(char *)0, MAXARGS, go_pass_through_func, ged_oscale},
    {"otranslate",	(char *)0, MAXARGS, go_pass_through_func, ged_otranslate},
    {"overlay",	(char *)0, MAXARGS, go_autoview_func, ged_overlay},
    {"paint_rect_area",	"vname", MAXARGS, go_paint_rect_area, GED_FUNC_PTR_NULL},
    {"pathlist",	(char *)0, MAXARGS, go_pass_through_func, ged_pathlist},
    {"paths",	(char *)0, MAXARGS, go_pass_through_func, ged_pathsum},
    {"perspective",	"vname [angle]", 3, go_view_func, ged_perspective},
    {"plot",	(char *)0, MAXARGS, go_pass_through_func, ged_plot},
    {"pmat",	"vname [mat]", 3, go_view_func, ged_pmat},
    {"pmodel2view",	"vname", 2, go_view_func, ged_pmodel2view},
    {"pov",	"vname center quat scale eye_pos perspective", 7, go_view_func, ged_pmat},
    {"prcolor",	(char *)0, MAXARGS, go_pass_through_func, ged_prcolor},
    {"prefix",	(char *)0, MAXARGS, go_pass_through_func, ged_prefix},
    {"push",	(char *)0, MAXARGS, go_pass_through_func, ged_push},
    {"put",	(char *)0, MAXARGS, go_pass_through_func, ged_put},
    {"put_comb",	(char *)0, MAXARGS, go_pass_through_func, ged_put_comb},
    {"putmat",	(char *)0, MAXARGS, go_pass_through_func, ged_putmat},
    {"qray",	(char *)0, MAXARGS, go_pass_through_func, ged_qray},
    {"quat",	"vname a b c d", 6, go_view_func, ged_quat},
    {"qvrot",	"vname x y z angle", 6, go_view_func, ged_qvrot},
    {"r",	(char *)0, MAXARGS, go_pass_through_func, ged_region},
    {"rcodes",	(char *)0, MAXARGS, go_pass_through_func, ged_rcodes},
    {"rect",	go_rect_syntax, MAXARGS, go_rect, GED_FUNC_PTR_NULL},
    {"rect_zoom",	"vname", MAXARGS, go_rect_zoom, GED_FUNC_PTR_NULL},
    {"red",	(char *)0, MAXARGS, go_pass_through_func, ged_red},
    {"refresh",	"vname", MAXARGS, go_refresh, GED_FUNC_PTR_NULL},
    {"refresh_all",	(char *)0, MAXARGS, go_refresh_all, GED_FUNC_PTR_NULL},
    {"regdef",	(char *)0, MAXARGS, go_pass_through_func, ged_regdef},
    {"regions",	(char *)0, MAXARGS, go_pass_through_func, ged_tables},
    {"report",	(char *)0, MAXARGS, go_pass_through_func, ged_report},
    {"rfarb",	(char *)0, MAXARGS, go_pass_through_func, ged_rfarb},
    {"rm",	(char *)0, MAXARGS, go_pass_through_func, ged_remove},
    {"rmap",	(char *)0, MAXARGS, go_pass_through_func, ged_rmap},
    {"rmat",	"vname [mat]", 3, go_view_func, ged_rmat},
    {"rmater",	(char *)0, MAXARGS, go_pass_through_func, ged_rmater},
    {"rot",	"vname [-m|-v] x y z", 6, go_view_func, ged_rot},
    {"rot_about",	"vname [e|k|m|v]", 3, go_view_func, ged_rotate_about},
    {"rot_point",	"vname x y z", 5, go_view_func, ged_rot_point},
    {"rotate_arb_face",	(char *)0, MAXARGS, go_pass_through_func, ged_rotate_arb_face},
    {"rotate_mode",	"vname x y", MAXARGS, go_rotate_mode, GED_FUNC_PTR_NULL},
    {"rt",	"vname [args]", GO_MAX_RT_ARGS, go_view_func, ged_rt},
    {"rt_rect_area",	"vname", MAXARGS, go_rt_rect_area, GED_FUNC_PTR_NULL},
    {"rt_gettrees",	"[-i] [-u] pname object", MAXARGS, go_rt_gettrees, GED_FUNC_PTR_NULL},
    {"rtarea",	"vname [args]", GO_MAX_RT_ARGS, go_view_func, ged_rt},
    {"rtabort",	(char *)0, GO_MAX_RT_ARGS, go_pass_through_func, ged_rtabort},
    {"rtedge",	"vname [args]", GO_MAX_RT_ARGS, go_view_func, ged_rt},
    {"rtcheck",	"vname [args]", GO_MAX_RT_ARGS, go_view_func, ged_rtcheck},
    {"rtweight",	"vname [args]", GO_MAX_RT_ARGS, go_view_func, ged_rt},
    {"savekey",	"vname filename", 3, go_view_func, ged_savekey},
    {"saveview",	"vname filename", 3, go_view_func, ged_saveview},
    {"sca",	"vname sf", 3, go_view_func, ged_scale},
    {"scale_mode",	"vname x y", MAXARGS, go_scale_mode, GED_FUNC_PTR_NULL},
    {"set_coord",	"vname [m|v]", MAXARGS, go_set_coord, GED_FUNC_PTR_NULL},
    {"set_fb_mode",	"vname [mode]", MAXARGS, go_set_fb_mode, GED_FUNC_PTR_NULL},
    {"set_output_script",	"[script]", MAXARGS, go_pass_through_func, ged_set_output_script},
    {"set_transparency",	(char *)0, MAXARGS, go_pass_through_and_refresh_func, ged_set_transparency},
    {"set_uplotOutputMode",	(char *)0, MAXARGS, go_pass_through_func, ged_set_uplotOutputMode},
    {"setview",	"vname x y z", 5, go_view_func, ged_setview},
    {"shaded_mode",	(char *)0, MAXARGS, go_pass_through_func, ged_shaded_mode},
    {"shader",	(char *)0, MAXARGS, go_pass_through_func, ged_shader},
    {"shells",	(char *)0, MAXARGS, go_pass_through_func, ged_shells},
    {"showmats",	(char *)0, MAXARGS, go_pass_through_func, ged_showmats},
    {"size",	"vname [size]", 3, go_view_func, ged_size},
    {"slew",	"vname x y [z]", 5, go_view_func, ged_slew},
    {"solids",	(char *)0, MAXARGS, go_pass_through_func, ged_tables},
    {"solids_on_ray",	(char *)0, MAXARGS, go_pass_through_func, ged_solids_on_ray},
    {"summary",	(char *)0, MAXARGS, go_pass_through_func, ged_summary},
    {"title",	(char *)0, MAXARGS, go_pass_through_func, ged_title},
    {"tol",	(char *)0, MAXARGS, go_pass_through_func, ged_tol},
    {"tops",	(char *)0, MAXARGS, go_pass_through_func, ged_tops},
    {"tra",	"vname [-m|-v] x y z", 6, go_view_func, ged_tra},
    {"track",	(char *)0, MAXARGS, go_pass_through_func, ged_track},
    {"translate_mode",	"vname x y", MAXARGS, go_translate_mode, GED_FUNC_PTR_NULL},
    {"tree",	(char *)0, MAXARGS, go_pass_through_func, ged_tree},
    {"unhide",	(char *)0, MAXARGS, go_pass_through_func, ged_unhide},
    {"units",	(char *)0, MAXARGS, go_pass_through_func, ged_units},
    {"v2m_point",	"vname x y z", 5, go_view_func, ged_v2m_point},
    {"vdraw",	(char *)0, MAXARGS, go_autoview_func, ged_vdraw},
    {"version",	(char *)0, MAXARGS, go_pass_through_func, ged_version},
    {"view",	"vname quat|ypr|aet|center|eye|size [args]", 3, go_view_func, ged_view},
    {"view2model",	"vname", 2, go_view_func, ged_view2model},
    {"viewdir",	"vname [-i]", 3, go_view_func, ged_viewdir},
    {"vmake",	"vname pname ptype", MAXARGS, go_vmake, GED_FUNC_PTR_NULL},
    {"vnirt",	"vname [args]", GO_MAX_RT_ARGS, go_view_func, ged_vnirt},
    {"vslew",	"vname x y", MAXARGS, go_vslew, GED_FUNC_PTR_NULL},
    {"vsnap",	"vname", MAXARGS, go_vsnap, GED_FUNC_PTR_NULL},
    {"wcodes",	(char *)0, MAXARGS, go_pass_through_func, ged_wcodes},
    {"whatid",	(char *)0, MAXARGS, go_pass_through_func, ged_whatid},
    {"which_shader",	(char *)0, MAXARGS, go_pass_through_func, ged_which_shader},
    {"whichair",	(char *)0, MAXARGS, go_pass_through_func, ged_which},
    {"whichid",	(char *)0, MAXARGS, go_pass_through_func, ged_which},
    {"who",	(char *)0, MAXARGS, go_pass_through_func, ged_who},
    {"wmater",	(char *)0, MAXARGS, go_pass_through_func, ged_wmater},
    {"xpush",	(char *)0, MAXARGS, go_pass_through_func, ged_xpush},
    {"ypr",	"vname yaw pitch roll", 5, go_view_func, ged_ypr},
    {"zap",	(char *)0, MAXARGS, go_pass_through_and_refresh_func, ged_zap},
    {"zbuffer",	"vname [0|1]", MAXARGS, go_zbuffer, GED_FUNC_PTR_NULL},
    {"zclip",	"vname [0|1]", MAXARGS, go_zclip, GED_FUNC_PTR_NULL},
    {"zoom",	"vname sf", 3, go_view_func, ged_zoom},
    {(char *)0,	(char *)0, 0, GO_WRAPPER_FUNC_PTR_NULL, GED_FUNC_PTR_NULL}
};


/**
 * @brief create the Tcl command for go_open
 *
 */
int
Go_Init(Tcl_Interp *interp)
{
    /*XXX Use of brlcad_interp is temporary */
    brlcad_interp = interp;

    BU_LIST_INIT(&HeadGedObj.l);
    (void)Tcl_CreateCommand(interp, (const char *)"go_open", go_open_tcl,
			    (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

#if 1
    /*XXX Temporary */
    /* initialize database objects */
    Wdb_Init(interp);

    /* initialize drawable geometry objects */
    Dgo_Init(interp);

    /* initialize view objects */
    Vo_Init(interp);
#endif

    return TCL_OK;
}

/**
 *			G O _ C M D
 *@brief
 * Generic interface for database commands.
 *
 * @par Usage:
 *        procname cmd ?args?
 *
 * @return result of ged command.
 */
static int
go_cmd(ClientData	clientData,
	Tcl_Interp	*interp,
	int		argc,
	char		**argv)
{
    register struct go_cmdtab *ctp;
    struct ged_obj *gop = (struct ged_obj *)clientData;
    Tcl_DString ds;
    int ret;
#if 0
    char flags[128];

    GED_CHECK_OBJ(gop);
#endif

    Tcl_DStringInit(&ds);

    if (argc < 2) {
	Tcl_DStringAppend(&ds, "subcommand not specfied; must be one of: ", -1);
	for (ctp = go_cmds; ctp->go_name != (char *)NULL; ctp++) {
	    Tcl_DStringAppend(&ds, " ", -1);
	    Tcl_DStringAppend(&ds, ctp->go_name, -1);
	}
	Tcl_DStringAppend(&ds, "\n", -1);
	Tcl_DStringResult(interp, &ds);

	return TCL_ERROR;
    }

    go_current_gop = gop;

    for (ctp = go_cmds; ctp->go_name != (char *)0; ctp++) {
	if (ctp->go_name[0] == argv[1][0] &&
	    !strcmp(ctp->go_name, argv[1])) {
	    ret = (*ctp->go_wrapper_func)(gop->go_gedp, argc-1, (const char **)argv+1, ctp->go_func, ctp->go_usage, ctp->go_maxargs);
	    break;
	}
    }

    /* Command not found. */
    if (ctp->go_name == (char *)0) {
	Tcl_DStringAppend(&ds, "unknown subcommand: ", -1);
	Tcl_DStringAppend(&ds, argv[1], -1);
	Tcl_DStringAppend(&ds, "; must be one of: ", -1);

	for (ctp = go_cmds; ctp->go_name != (char *)NULL; ctp++) {
	    Tcl_DStringAppend(&ds, " ", -1);
	    Tcl_DStringAppend(&ds, ctp->go_name, -1);
	}
	Tcl_DStringAppend(&ds, "\n", -1);
	Tcl_DStringResult(interp, &ds);

	return TCL_ERROR;
    }

    Tcl_DStringAppend(&ds, bu_vls_addr(&gop->go_gedp->ged_result_str), -1);
    Tcl_DStringResult(interp, &ds);

    if (ret == BRLCAD_ERROR)
	return TCL_ERROR;

    return TCL_OK;
}


/**
 * @brief
 * Called by Tcl when the object is destroyed.
 */
void
go_deleteProc(ClientData clientData)
{
    struct ged_obj *gop = (struct ged_obj *)clientData;
    struct ged_dm_view *gdvp;

    if (go_current_gop == gop)
	go_current_gop = GED_OBJ_NULL;

#if 0
    GED_CHECK_OBJ(gop);
#endif
    BU_LIST_DEQUEUE(&gop->l);
    bu_vls_free(&gop->go_name);
    ged_close(gop->go_gedp);
#if 1
    while (BU_LIST_WHILE(gdvp, ged_dm_view, &gop->go_head_views.l)) {
	BU_LIST_DEQUEUE(&(gdvp->l));
	bu_vls_free(&gdvp->gdv_name);
	DM_CLOSE(gdvp->gdv_dmp);
	bu_free((genptr_t)gdvp->gdv_view, "ged_view");

	go_close_fbs(gdvp);

	bu_free((genptr_t)gdvp, "ged_dm_view");
    }
#else
    for (i = 0; i < GED_OBJ_NUM_VIEWS; ++i)
	bu_free((genptr_t)gop->go_views[i], "struct ged_view");
    if (gop->go_dmp != DM_NULL)
	DM_CLOSE(gop->go_dmp);
#endif
    bu_free((genptr_t)gop, "struct ged_obj");
}

/**
 * @brief
 * Create a command named "oname" in "interp" using "gedp" as its state.
 *
 */
int
go_create_cmd(Tcl_Interp	*interp,
	      struct ged_obj	*gop,	/* pointer to object */
	      const char	*oname)	/* object name */
{
    if (gop == GED_OBJ_NULL) {
	Tcl_AppendResult(interp, "go_create_cmd ", oname, " failed", NULL);
	return TCL_ERROR;
    }

    /* Instantiate the newprocname, with clientData of gop */
    /* Beware, returns a "token", not TCL_OK. */
    (void)Tcl_CreateCommand(interp, oname, (Tcl_CmdProc *)go_cmd,
			    (ClientData)gop, go_deleteProc);

    /* Return new function name as result */
    Tcl_AppendResult(interp, oname, (char *)NULL);

    return TCL_OK;
}

#if 0
/**
 * @brief
 * Create an command/object named "oname" in "interp" using "gop" as
 * its state.  It is presumed that the gop has already been opened.
 */
int
go_init_obj(Tcl_Interp		*interp,
	     struct ged_obj	*gop,	/* pointer to object */
	     const char		*oname)	/* object name */
{
    if (gop == GED_OBJ_NULL) {
	Tcl_AppendResult(interp, "ged_init_obj ", oname, " failed (ged_init_obj)", NULL);
	return TCL_ERROR;
    }

    /* initialize ged_obj */
    bu_vls_init(&gop->go_name);
    bu_vls_strcpy(&gop->go_name, oname);

    BU_LIST_INIT(&gop->go_observers.l);
    gop->go_interp = interp;

    /* append to list of ged_obj */
    BU_LIST_APPEND(&HeadGedObj.l, &gop->l);

    return TCL_OK;
}
#endif

/**
 *			G E D _ O P E N _ T C L
 *@brief
 *  A TCL interface to wdb_fopen() and wdb_dbopen().
 *
 *  @par Implicit return -
 *	Creates a new TCL proc which responds to get/put/etc. arguments
 *	when invoked.  clientData of that proc will be ged_obj pointer
 *	for this instance of the database.
 *	Easily allows keeping track of multiple databases.
 *
 *  @return wdb pointer, for more traditional C-style interfacing.
 *
 *  @par Example -
 *	set gop [go_open .inmem inmem $dbip]
 *@n	.inmem get box.s
 *@n	.inmem close
 *
 *@n	go_open db file "bob.g"
 *@n	db get white.r
 *@n	db close
 */
static int
go_open_tcl(ClientData	clientData,
	     Tcl_Interp	*interp,
	     int	argc,
	     const char	**argv)
{
    struct ged_obj *gop;
    struct ged *gedp;

    if (argc == 1) {
	/* get list of database objects */
	for (BU_LIST_FOR(gop, ged_obj, &HeadGedObj.l))
	    Tcl_AppendResult(interp, bu_vls_addr(&gop->go_name), " ", (char *)NULL);

	return TCL_OK;
    }

    if (argc < 3 || 4 < argc) {
	Tcl_AppendResult(interp, "\
Usage: go_open\n\
       go_open newprocname file filename\n\
       go_open newprocname disk $dbip\n\
       go_open newprocname disk_append $dbip\n\
       go_open newprocname inmem $dbip\n\
       go_open newprocname inmem_append $dbip\n\
       go_open newprocname db filename\n\
       go_open newprocname filename\n",
			 NULL);
	return TCL_ERROR;
    }

    /* Delete previous proc (if any) to release all that memory, first */
    (void)Tcl_DeleteCommand(interp, argv[1]);

    if (argc == 3 || strcmp(argv[2], "db") == 0) {
	if (argc == 3)
	    gedp = ged_open("filename", argv[2]); 
	else
	    gedp = ged_open("db", argv[3]); 
    } else
	gedp = ged_open(argv[2], argv[3]); 

    /* initialize ged_obj */
    BU_GETSTRUCT(gop, ged_obj);
    gop->go_gedp = gedp;
    gop->go_gedp->ged_output_handler = go_output_handler;
    bu_vls_init(&gop->go_name);
    bu_vls_strcpy(&gop->go_name, argv[1]);
    BU_LIST_INIT(&gop->go_observers.l);
    gop->go_interp = interp;

    BU_LIST_INIT(&gop->go_head_views.l);

    /* append to list of ged_obj */
    BU_LIST_APPEND(&HeadGedObj.l, &gop->l);

    return go_create_cmd(interp, gop, argv[1]);
}


/*************************** Local Command Functions ***************************/
static int
go_autoview(struct ged		*gedp,
	    int			argc,
	    const char		*argv[],
	    ged_func_ptr	func,
	    const char		*usage,
	    int			maxargs)
{
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    if (argc != 2) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    go_autoview_view(gdvp);

    return BRLCAD_OK;
}

static int
go_blast(struct ged	*gedp,
	 int		argc,
	 const char	*argv[],
	 ged_func_ptr	func,
	 const char	*usage,
	 int		maxargs)
{
    int ret;

    ret = ged_blast(gedp, argc, argv);

    if (ret != BRLCAD_OK)
	return ret;

    go_autoview_all_views(go_current_gop);

    return ret;
}

static int
go_configure(struct ged		*gedp,
	     int		argc,
	     const char		*argv[],
	     ged_func_ptr	func,
	     const char		*usage,
	     int		maxargs)
{
    struct ged_dm_view *gdvp;
    int	status;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    if (argc != 2) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    /* configure the display manager window */
    status = DM_CONFIGURE_WIN(gdvp->gdv_dmp);

    /* configure the framebuffer window */
    if (gdvp->gdv_fbs.fbs_fbp != FBIO_NULL)
	fb_configureWindow(gdvp->gdv_fbs.fbs_fbp,
			   gdvp->gdv_dmp->dm_width,
			   gdvp->gdv_dmp->dm_height);

    if (status == TCL_OK) {
	go_refresh_view(gdvp);
	return BRLCAD_OK;
    }

    return BRLCAD_ERROR;
}

static int
go_constrain_rmode(struct ged	*gedp,
		   int		argc,
		   const char	*argv[],
		   ged_func_ptr	func,
		   const char	*usage,
		   int		maxargs)
{
    fastf_t x, y;
    struct bu_vls bindings;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 5) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if ((argv[2][0] != 'x' &&
	 argv[2][0] != 'y' &&
	 argv[2][0] != 'z') || argv[2][1] != '\0') {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_OK;
    }

    if (sscanf(argv[3], "%lf", &x) != 1 ||
	sscanf(argv[4], "%lf", &y) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    gdvp->gdv_view->gv_prevMouseX = x;
    gdvp->gdv_view->gv_prevMouseY = y;
    gdvp->gdv_view->gv_mode = GED_CONSTRAINED_ROTATE_MODE;

    bu_vls_init(&bindings);
    bu_vls_printf(&bindings, "bind %V <Motion> {%V mouse_constrain_rot %V %s %%x %%y}; break",
		  &gdvp->gdv_dmp->dm_pathName,
		  &go_current_gop->go_name,
		  &gdvp->gdv_name,
		  argv[2]);
    Tcl_Eval(go_current_gop->go_interp, bu_vls_addr(&bindings));
    bu_vls_free(&bindings);

    return BRLCAD_OK;
}

static int
go_constrain_tmode(struct ged	*gedp,
		   int		argc,
		   const char	*argv[],
		   ged_func_ptr	func,
		   const char	*usage,
		   int		maxargs)
{
    fastf_t x, y;
    struct bu_vls bindings;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 5) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if ((argv[2][0] != 'x' &&
	 argv[2][0] != 'y' &&
	 argv[2][0] != 'z') || argv[2][1] != '\0') {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_OK;
    }

    if (sscanf(argv[3], "%lf", &x) != 1 ||
	sscanf(argv[4], "%lf", &y) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    gdvp->gdv_view->gv_prevMouseX = x;
    gdvp->gdv_view->gv_prevMouseY = y;
    gdvp->gdv_view->gv_mode = GED_CONSTRAINED_TRANSLATE_MODE;

    bu_vls_init(&bindings);
    bu_vls_printf(&bindings, "bind %V <Motion> {%V mouse_constrain_trans %V %s %%x %%y}; break",
		  &gdvp->gdv_dmp->dm_pathName,
		  &go_current_gop->go_name,
		  &gdvp->gdv_name,
		  argv[2]);
    Tcl_Eval(go_current_gop->go_interp, bu_vls_addr(&bindings));
    bu_vls_free(&bindings);

    return BRLCAD_OK;
}

static int
go_delete_view(struct ged	*gedp,
	       int		argc,
	       const char	*argv[],
	       ged_func_ptr	func,
	       const char	*usage,
	       int		maxargs)
{
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 2) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    BU_LIST_DEQUEUE(&(gdvp->l));
    bu_vls_free(&gdvp->gdv_name);
    DM_CLOSE(gdvp->gdv_dmp);
    bu_free((genptr_t)gdvp->gdv_view, "ged_view");
    bu_free((genptr_t)gdvp, "ged_dm_view");

    return BRLCAD_OK;
}

static int
go_grid(struct ged	*gedp,
	int		argc,
	const char	*argv[],
	ged_func_ptr	func,
	const char	*usage,
	int		maxargs)
{
    char *command;
    char *parameter;
    char **argp = (char **)argv;
    point_t user_pt;		/* Value(s) provided by user */
    int i;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    if (argc < 2 || 6 < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if (argc == 2) {
	if (gdvp->gdv_view->gv_grid.ggs_draw)
	    gdvp->gdv_view->gv_grid.ggs_draw = 0;
	else
	    gdvp->gdv_view->gv_grid.ggs_draw = 1;

	go_refresh_view(gdvp);
	return BRLCAD_OK;
    }

    command = (char *)argv[0];
    parameter = (char *)argv[2];
    argc -= 3;
    argp += 3;

    for (i = 0; i < argc; ++i)
	user_pt[i] = atof(argp[i]);

    if (strcmp(parameter, "draw") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%d", gdvp->gdv_view->gv_grid.ggs_draw);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    i = (int)user_pt[X];

	    if (i)
		gdvp->gdv_view->gv_grid.ggs_draw = 1;
	    else
		gdvp->gdv_view->gv_grid.ggs_draw = 0;

	    go_refresh_view(gdvp);
	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s draw' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "snap") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%d", gdvp->gdv_view->gv_grid.ggs_snap);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    i = (int)user_pt[X];

	    if (i)
		gdvp->gdv_view->gv_grid.ggs_snap = 1;
	    else
		gdvp->gdv_view->gv_grid.ggs_snap = 0;

	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s draw' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "rh") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%g",
			  gdvp->gdv_view->gv_grid.ggs_res_h * gedp->ged_wdbp->dbip->dbi_base2local);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    gdvp->gdv_view->gv_grid.ggs_res_h = user_pt[X] * gedp->ged_wdbp->dbip->dbi_local2base;
	    go_refresh_view(gdvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s rh' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "rv") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%g",
			  gdvp->gdv_view->gv_grid.ggs_res_v * gedp->ged_wdbp->dbip->dbi_base2local);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    gdvp->gdv_view->gv_grid.ggs_res_v = user_pt[X] * gedp->ged_wdbp->dbip->dbi_local2base;
	    go_refresh_view(gdvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s rv' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "mrh") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%d", gdvp->gdv_view->gv_grid.ggs_res_major_h);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    gdvp->gdv_view->gv_grid.ggs_res_major_h = (int)user_pt[X];
	    go_refresh_view(gdvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s mrh' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "mrv") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%d", gdvp->gdv_view->gv_grid.ggs_res_major_v);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    gdvp->gdv_view->gv_grid.ggs_res_major_v = (int)user_pt[X];
	    go_refresh_view(gdvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s mrv' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "anchor") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%g %g %g",
			  gdvp->gdv_view->gv_grid.ggs_anchor[X] * gedp->ged_wdbp->dbip->dbi_base2local,
			  gdvp->gdv_view->gv_grid.ggs_anchor[Y] * gedp->ged_wdbp->dbip->dbi_base2local,
			  gdvp->gdv_view->gv_grid.ggs_anchor[Z] * gedp->ged_wdbp->dbip->dbi_base2local);
	    return BRLCAD_OK;
	} else if (argc == 3) {
	    gdvp->gdv_view->gv_grid.ggs_anchor[0] = user_pt[X] * gedp->ged_wdbp->dbip->dbi_local2base;
	    gdvp->gdv_view->gv_grid.ggs_anchor[1] = user_pt[Y] * gedp->ged_wdbp->dbip->dbi_local2base;
	    gdvp->gdv_view->gv_grid.ggs_anchor[2] = user_pt[Z] * gedp->ged_wdbp->dbip->dbi_local2base;
	    go_refresh_view(gdvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s color' command requires 0 or 3 arguments\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "color") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%d %d %d",
			  gdvp->gdv_view->gv_grid.ggs_color[X],
			  gdvp->gdv_view->gv_grid.ggs_color[Y],
			  gdvp->gdv_view->gv_grid.ggs_color[Z]);
	    return BRLCAD_OK;
	} else if (argc == 3) {
	    gdvp->gdv_view->gv_grid.ggs_color[0] = (int)user_pt[X];
	    gdvp->gdv_view->gv_grid.ggs_color[1] = (int)user_pt[Y];
	    gdvp->gdv_view->gv_grid.ggs_color[2] = (int)user_pt[Z];
	    go_refresh_view(gdvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s color' command requires 0 or 3 arguments\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "vars") == 0) {
	ged_grid_vls_print(&gdvp->gdv_view->gv_grid, gdvp->gdv_view, gedp->ged_wdbp->dbip->dbi_base2local, &gedp->ged_result_str);
	return BRLCAD_OK;
    }

    if (strcmp(parameter, "help") == 0) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", command, usage);
	return BRLCAD_HELP;
    }

    bu_vls_printf(&gedp->ged_result_str, "%s: unrecognized command '%s'\nUsage: %s %s\n",
		  command, parameter, command, usage);
    return BRLCAD_ERROR;
}

static int
go_idle_mode(struct ged		*gedp,
	     int		argc,
	     const char		*argv[],
	     ged_func_ptr	func,
	     const char		*usage,
	     int		maxargs)
{
    struct bu_vls bindings;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 2) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    bu_vls_init(&bindings);
    bu_vls_printf(&bindings, "bind %V <Motion> {}",
		  &gdvp->gdv_dmp->dm_pathName);
    Tcl_Eval(go_current_gop->go_interp, bu_vls_addr(&bindings));
    bu_vls_free(&bindings);

    if (gdvp->gdv_view->gv_grid.ggs_snap &&
	(gdvp->gdv_view->gv_mode == GED_TRANSLATE_MODE ||
	 gdvp->gdv_view->gv_mode == GED_CONSTRAINED_TRANSLATE_MODE)) {
	ged_snap_view_center_to_grid(&gdvp->gdv_view->gv_grid, gdvp->gdv_view, gedp->ged_wdbp->dbip->dbi_base2local);
	go_refresh_view(gdvp);
    }

    gdvp->gdv_view->gv_mode = GED_IDLE_MODE;

    return BRLCAD_OK;
}

static int
go_light(struct ged	*gedp,
	 int		argc,
	 const char	*argv[],
	 ged_func_ptr	func,
	 const char	*usage,
	 int		maxargs)
{
    int light;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (3 < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    /* get light flag */
    if (argc == 2) {
	bu_vls_printf(&gedp->ged_result_str, "%d", gdvp->gdv_dmp->dm_light);
	return BRLCAD_OK;
    }

    /* set light flag */
    if (sscanf(argv[2], "%d", &light) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    if (light < 0)
	light = 0;
    else if (1 < light)
	light = 1;

    DM_SET_LIGHT(gdvp->gdv_dmp, light);
    go_refresh_view(gdvp);

    return BRLCAD_OK;
}

static int
go_list_views(struct ged	*gedp,
	      int		argc,
	      const char	*argv[],
	      ged_func_ptr	func,
	      const char	*usage,
	      int		maxargs)
{
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    if (argc != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s", argv[0]);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l))
	bu_vls_printf(&gedp->ged_result_str, "%V ", &gdvp->gdv_name);

    return BRLCAD_OK;
}

static int
go_listen(struct ged	*gedp,
	  int		argc,
	  const char	*argv[],
	  ged_func_ptr	func,
	  const char	*usage,
	  int		maxargs)
{
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (3 < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if (gdvp->gdv_fbs.fbs_fbp == FBIO_NULL) {
	bu_vls_printf(&gedp->ged_result_str, "%s listen: framebuffer not open!\n", argv[0]);
	return BRLCAD_ERROR;
    }

    /* return the port number */
    if (argc == 2) {
	bu_vls_printf(&gedp->ged_result_str, "%d", gdvp->gdv_fbs.fbs_listener.fbsl_port);
	return BRLCAD_OK;
    }

    if (argc == 3) {
	int port;

	if (sscanf(argv[2], "%d", &port) != 1) {
	    bu_vls_printf(&gedp->ged_result_str, "listen: bad value - %s\n", argv[2]);
	    return BRLCAD_ERROR;
	}

	if (port >= 0)
	    fbs_open(&gdvp->gdv_fbs, port);
	else {
	    fbs_close(&gdvp->gdv_fbs);
	}
	bu_vls_printf(&gedp->ged_result_str, "%d", gdvp->gdv_fbs.fbs_listener.fbsl_port);
	return BRLCAD_OK;
    }

    bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
    return BRLCAD_ERROR;
}

static int
go_make(struct ged	*gedp,
	int		argc,
	const char	*argv[],
	ged_func_ptr	func,
	const char	*usage,
	int		maxargs)
{
    int ret;
    char *av[3];

    ret = ged_make(gedp, argc, argv);

    if (ret == BRLCAD_OK) {
	av[0] = "draw";
	av[1] = (char *)argv[argc-2];
	av[2] = (char *)0;
	go_autoview_func(gedp, 2, (const char **)av, ged_draw, (char *)0, MAXARGS);
    }

    return ret;
}

static int
go_mirror(struct ged	*gedp,
	  int		argc,
	  const char	*argv[],
	  ged_func_ptr	func,
	  const char	*usage,
	  int		maxargs)
{
    int ret;
    char *av[3];

    ret = ged_mirror(gedp, argc, argv);

    if (ret == BRLCAD_OK) {
	av[0] = "draw";
	av[1] = (char *)argv[argc-1];
	av[2] = (char *)0;
	go_autoview_func(gedp, 2, (const char **)av, ged_draw, (char *)0, MAXARGS);
    }

    return ret;
}

static int
go_mouse_constrain_rot(struct ged	*gedp,
		       int		argc,
		       const char	*argv[],
		       ged_func_ptr	func,
		       const char	*usage,
		       int		maxargs)
{
    int ret;
    int ac;
    char *av[3];
    fastf_t x, y;
    fastf_t dx, dy;
    fastf_t sf;
    struct bu_vls rot_vls;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 5) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if ((argv[2][0] != 'x' && argv[2][0] != 'y' && argv[2][0] != 'z') || argv[2][1] != '\0') {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[3], "%lf", &x) != 1 ||
	sscanf(argv[4], "%lf", &y) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    dx = x - gdvp->gdv_view->gv_prevMouseX;
    dy = gdvp->gdv_view->gv_prevMouseY - y;

    gdvp->gdv_view->gv_prevMouseX = x;
    gdvp->gdv_view->gv_prevMouseY = y;

    if (dx < gdvp->gdv_view->gv_minMouseDelta)
	dx = gdvp->gdv_view->gv_minMouseDelta;
    else if (gdvp->gdv_view->gv_maxMouseDelta < dx)
	dx = gdvp->gdv_view->gv_maxMouseDelta;

    if (dy < gdvp->gdv_view->gv_minMouseDelta)
	dy = gdvp->gdv_view->gv_minMouseDelta;
    else if (gdvp->gdv_view->gv_maxMouseDelta < dy)
	dy = gdvp->gdv_view->gv_maxMouseDelta;

    dx *= gdvp->gdv_view->gv_rscale;
    dy *= gdvp->gdv_view->gv_rscale;

    if (fabs(dx) > fabs(dy))
	sf = dx;
    else
	sf = dy;

    bu_vls_init(&rot_vls);
    switch (argv[2][0]) {
    case 'x':
	bu_vls_printf(&rot_vls, "%lf 0 0", sf);
    case 'y':
	bu_vls_printf(&rot_vls, "0 %lf 0", sf);
    case 'z':
	bu_vls_printf(&rot_vls, "0 0 %lf", sf);
    }

    gedp->ged_gvp = gdvp->gdv_view;
    ac = 2;
    av[0] = "rot";
    av[1] = bu_vls_addr(&rot_vls);
    av[2] = (char *)0;

    ret = ged_rot(gedp, ac, (const char **)av);
    bu_vls_free(&rot_vls);

    if (ret == BRLCAD_OK)
	go_refresh_view(gdvp);

    return BRLCAD_OK;
}

static int
go_mouse_constrain_trans(struct ged	*gedp,
			 int		argc,
			 const char	*argv[],
			 ged_func_ptr	func,
			 const char	*usage,
			 int		maxargs)
{
    int ret;
    int ac;
    char *av[3];
    fastf_t x, y;
    fastf_t dx, dy;
    fastf_t sf;
    fastf_t inv_width;
    struct bu_vls tran_vls;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 5) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if ((argv[2][0] != 'x' && argv[2][0] != 'y' && argv[2][0] != 'z') || argv[2][1] != '\0') {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[3], "%lf", &x) != 1 ||
	sscanf(argv[4], "%lf", &y) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    dx = x - gdvp->gdv_view->gv_prevMouseX;
    dy = gdvp->gdv_view->gv_prevMouseY - y;

    gdvp->gdv_view->gv_prevMouseX = x;
    gdvp->gdv_view->gv_prevMouseY = y;

    if (dx < gdvp->gdv_view->gv_minMouseDelta)
	dx = gdvp->gdv_view->gv_minMouseDelta;
    else if (gdvp->gdv_view->gv_maxMouseDelta < dx)
	dx = gdvp->gdv_view->gv_maxMouseDelta;

    if (dy < gdvp->gdv_view->gv_minMouseDelta)
	dy = gdvp->gdv_view->gv_minMouseDelta;
    else if (gdvp->gdv_view->gv_maxMouseDelta < dy)
	dy = gdvp->gdv_view->gv_maxMouseDelta;

    inv_width = 1.0 / gdvp->gdv_dmp->dm_width;
    dx *= inv_width * gdvp->gdv_view->gv_size * gedp->ged_wdbp->dbip->dbi_local2base;
    dy *= inv_width * gdvp->gdv_view->gv_size * gedp->ged_wdbp->dbip->dbi_local2base;

    if (fabs(dx) > fabs(dy))
	sf = dx;
    else
	sf = dy;

    bu_vls_init(&tran_vls);
    switch (argv[2][0]) {
    case 'x':
	bu_vls_printf(&tran_vls, "%lf 0 0", sf);
    case 'y':
	bu_vls_printf(&tran_vls, "0 %lf 0", sf);
    case 'z':
	bu_vls_printf(&tran_vls, "0 0 %lf", sf);
    }

    gedp->ged_gvp = gdvp->gdv_view;
    ac = 2;
    av[0] = "tra";
    av[1] = bu_vls_addr(&tran_vls);
    av[2] = (char *)0;

    ret = ged_tra(gedp, ac, (const char **)av);
    bu_vls_free(&tran_vls);

    if (ret == BRLCAD_OK)
	go_refresh_view(gdvp);

    return BRLCAD_OK;
}

static int
go_mouse_rot(struct ged		*gedp,
	     int		argc,
	     const char		*argv[],
	     ged_func_ptr	func,
	     const char		*usage,
	     int		maxargs)
{
    int ret;
    int ac;
    char *av[4];
    fastf_t x, y;
    fastf_t dx, dy;
    struct bu_vls rot_vls;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 4) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[2], "%lf", &x) != 1 ||
	sscanf(argv[3], "%lf", &y) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    dx = gdvp->gdv_view->gv_prevMouseY - y;
    dy = gdvp->gdv_view->gv_prevMouseX - x;

    gdvp->gdv_view->gv_prevMouseX = x;
    gdvp->gdv_view->gv_prevMouseY = y;

    if (dx < gdvp->gdv_view->gv_minMouseDelta)
	dx = gdvp->gdv_view->gv_minMouseDelta;
    else if (gdvp->gdv_view->gv_maxMouseDelta < dx)
	dx = gdvp->gdv_view->gv_maxMouseDelta;

    if (dy < gdvp->gdv_view->gv_minMouseDelta)
	dy = gdvp->gdv_view->gv_minMouseDelta;
    else if (gdvp->gdv_view->gv_maxMouseDelta < dy)
	dy = gdvp->gdv_view->gv_maxMouseDelta;

    dx *= gdvp->gdv_view->gv_rscale;
    dy *= gdvp->gdv_view->gv_rscale;

    bu_vls_init(&rot_vls);
    bu_vls_printf(&rot_vls, "%lf %lf 0", dx, dy);

    gedp->ged_gvp = gdvp->gdv_view;
    ac = 3;
    av[0] = "rot";
    av[1] = "-v";
    av[2] = bu_vls_addr(&rot_vls);
    av[3] = (char *)0;

    ret = ged_rot(gedp, ac, (const char **)av);
    bu_vls_free(&rot_vls);

    if (ret == BRLCAD_OK)
	go_refresh_view(gdvp);

    return BRLCAD_OK;
}

static int
go_mouse_scale(struct ged	*gedp,
	       int		argc,
	       const char	*argv[],
	       ged_func_ptr	func,
	       const char	*usage,
	       int		maxargs)
{
    int ret;
    int ac;
    char *av[3];
    fastf_t x, y;
    fastf_t dx, dy;
    fastf_t sf;
    fastf_t inv_width;
    struct bu_vls zoom_vls;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 4) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[2], "%lf", &x) != 1 ||
	sscanf(argv[3], "%lf", &y) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    dx = x - gdvp->gdv_view->gv_prevMouseX;
    dy = gdvp->gdv_view->gv_prevMouseY - y;

    gdvp->gdv_view->gv_prevMouseX = x;
    gdvp->gdv_view->gv_prevMouseY = y;

    if (dx < gdvp->gdv_view->gv_minMouseDelta)
	dx = gdvp->gdv_view->gv_minMouseDelta;
    else if (gdvp->gdv_view->gv_maxMouseDelta < dx)
	dx = gdvp->gdv_view->gv_maxMouseDelta;

    if (dy < gdvp->gdv_view->gv_minMouseDelta)
	dy = gdvp->gdv_view->gv_minMouseDelta;
    else if (gdvp->gdv_view->gv_maxMouseDelta < dy)
	dy = gdvp->gdv_view->gv_maxMouseDelta;

    inv_width = 1.0 / gdvp->gdv_dmp->dm_width;
    dx *= inv_width * gdvp->gdv_view->gv_sscale;
    dy *= inv_width * gdvp->gdv_view->gv_sscale;

    if (fabs(dx) > fabs(dy))
	sf = 1.0 + dx;
    else
	sf = 1.0 + dy;

    bu_vls_init(&zoom_vls);
    bu_vls_printf(&zoom_vls, "%lf", sf);

    gedp->ged_gvp = gdvp->gdv_view;
    ac = 2;
    av[0] = "zoom";
    av[1] = bu_vls_addr(&zoom_vls);
    av[2] = (char *)0;

    ret = ged_zoom(gedp, ac, (const char **)av);
    bu_vls_free(&zoom_vls);

    if (ret == BRLCAD_OK)
	go_refresh_view(gdvp);

    return BRLCAD_OK;
}

static int
go_mouse_trans(struct ged	*gedp,
	       int		argc,
	       const char	*argv[],
	       ged_func_ptr	func,
	       const char	*usage,
	       int		maxargs)
{
    int ret;
    int ac;
    char *av[4];
    fastf_t x, y;
    fastf_t dx, dy;
    fastf_t inv_width;
    struct bu_vls trans_vls;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 4) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[2], "%lf", &x) != 1 ||
	sscanf(argv[3], "%lf", &y) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    dx = gdvp->gdv_view->gv_prevMouseX - x;
    dy = y - gdvp->gdv_view->gv_prevMouseY;

    gdvp->gdv_view->gv_prevMouseX = x;
    gdvp->gdv_view->gv_prevMouseY = y;

    if (dx < gdvp->gdv_view->gv_minMouseDelta)
	dx = gdvp->gdv_view->gv_minMouseDelta;
    else if (gdvp->gdv_view->gv_maxMouseDelta < dx)
	dx = gdvp->gdv_view->gv_maxMouseDelta;

    if (dy < gdvp->gdv_view->gv_minMouseDelta)
	dy = gdvp->gdv_view->gv_minMouseDelta;
    else if (gdvp->gdv_view->gv_maxMouseDelta < dy)
	dy = gdvp->gdv_view->gv_maxMouseDelta;

    inv_width = 1.0 / gdvp->gdv_dmp->dm_width;
    dx *= inv_width * gdvp->gdv_view->gv_size * gedp->ged_wdbp->dbip->dbi_local2base;
    dy *= inv_width * gdvp->gdv_view->gv_size * gedp->ged_wdbp->dbip->dbi_local2base;

    bu_vls_init(&trans_vls);
    bu_vls_printf(&trans_vls, "%lf %lf 0", dx, dy);

    gedp->ged_gvp = gdvp->gdv_view;
    ac = 3;
    av[0] = "tra";
    av[1] = "-v";
    av[2] = bu_vls_addr(&trans_vls);
    av[3] = (char *)0;

    ret = ged_tra(gedp, ac, (const char **)av);
    bu_vls_free(&trans_vls);

    if (ret == BRLCAD_OK)
	go_refresh_view(gdvp);

    return BRLCAD_OK;
}

static int
go_new_view(struct ged		*gedp,
	    int			argc,
	    const char		*argv[],
	    ged_func_ptr	func,
	    const char		*usage,
	    int			maxargs)
{
    struct ged_dm_view *new_gdvp = BU_LIST_LAST(ged_dm_view, &go_current_gop->go_head_views.l);
    static const int name_index = 1;
    int type = DM_TYPE_BAD;

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc < 3) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    /* find display manager type */
#ifdef DM_X
    if (argv[2][0] == 'X' || argv[2][0] == 'x')
	type = DM_TYPE_X;
#endif /* DM_X */

#ifdef DM_TK
    if (!strcmp(argv[2], "tk"))
	type = DM_TYPE_TK;
#endif /* DM_TK */

#ifdef DM_OGL
    if (!strcmp(argv[2], "ogl"))
	type = DM_TYPE_OGL;
#endif /* DM_OGL */

#ifdef DM_WGL
    if (!strcmp(argv[2], "wgl"))
	type = DM_TYPE_WGL;
#endif /* DM_WGL */

    if (type == DM_TYPE_BAD) {
	bu_vls_printf(&gedp->ged_result_str, "Unsupported display manager type - %s\n", argv[2]);
	return BRLCAD_ERROR;
    }

    BU_GETSTRUCT(new_gdvp, ged_dm_view);
    BU_GETSTRUCT(new_gdvp->gdv_view, ged_view);

    {
	int i;
	int arg_start = 3;
	int newargs = 0;
	int ac;
	char **av;

	ac = argc + newargs;
	av = (char **)bu_malloc(sizeof(char *) * (ac+1), "go_new_view: av");
	av[0] = (char *)argv[0];

	/*
	 * Stuff name into argument list.
	 */
	av[1] = "-n";
	av[2] = (char *)argv[name_index];

	/* copy the rest */
	for (i = arg_start; i < argc; ++i)
	    av[i+newargs] = (char *)argv[i];
	av[i+newargs] = (char *)NULL;

	if ((new_gdvp->gdv_dmp = dm_open(go_current_gop->go_interp, type, ac, av)) == DM_NULL) {
	    bu_free((genptr_t)new_gdvp->gdv_view, "ged_view");
	    bu_free((genptr_t)new_gdvp, "ged_dm_view");
	    bu_free((genptr_t)av, "go_new_view: av");

	    bu_vls_printf(&gedp->ged_result_str, "Failed to create %s\n", argv[1]);
	    return BRLCAD_ERROR;
	}

	bu_free((genptr_t)av, "go_new_view: av");

    }

    new_gdvp->gdv_gop = go_current_gop;
    bu_vls_init(&new_gdvp->gdv_name);
    bu_vls_printf(&new_gdvp->gdv_name, argv[name_index]);
    ged_view_init(new_gdvp->gdv_view);
    BU_LIST_INSERT(&go_current_gop->go_head_views.l, &new_gdvp->l);

    bu_vls_printf(&gedp->ged_result_str, "%s", argv[name_index]);

    new_gdvp->gdv_fbs.fbs_listener.fbsl_fbsp = &new_gdvp->gdv_fbs;
    new_gdvp->gdv_fbs.fbs_listener.fbsl_fd = -1;
    new_gdvp->gdv_fbs.fbs_listener.fbsl_port = -1;
    new_gdvp->gdv_fbs.fbs_fbp = FBIO_NULL;
    new_gdvp->gdv_fbs.fbs_callback = go_fbs_callback;
    new_gdvp->gdv_fbs.fbs_clientData = new_gdvp;
    new_gdvp->gdv_fbs.fbs_interp = go_current_gop->go_interp;

    new_gdvp->gdv_view->gv_adc.gas_a1 = 45.0;
    new_gdvp->gdv_view->gv_adc.gas_a2 = 45.0;
    new_gdvp->gdv_view->gv_adc.gas_line_color[0] = 255;
    new_gdvp->gdv_view->gv_adc.gas_line_color[1] = 255;
    new_gdvp->gdv_view->gv_adc.gas_line_color[2] = 0;
    new_gdvp->gdv_view->gv_adc.gas_tick_color[0] = 255;
    new_gdvp->gdv_view->gv_adc.gas_tick_color[1] = 255;
    new_gdvp->gdv_view->gv_adc.gas_tick_color[2] = 255;

    new_gdvp->gdv_view->gv_grid.ggs_anchor[0] = 0.0;
    new_gdvp->gdv_view->gv_grid.ggs_anchor[1] = 0.0;
    new_gdvp->gdv_view->gv_grid.ggs_anchor[2] = 0.0;
    new_gdvp->gdv_view->gv_grid.ggs_res_h = 1.0;
    new_gdvp->gdv_view->gv_grid.ggs_res_v = 1.0;
    new_gdvp->gdv_view->gv_grid.ggs_res_major_h = 5;
    new_gdvp->gdv_view->gv_grid.ggs_res_major_v = 5;
    new_gdvp->gdv_view->gv_grid.ggs_color[0] = 255;
    new_gdvp->gdv_view->gv_grid.ggs_color[1] = 255;
    new_gdvp->gdv_view->gv_grid.ggs_color[2] = 255;

    new_gdvp->gdv_view->gv_rect.grs_draw = 1;
    new_gdvp->gdv_view->gv_rect.grs_pos[0] = 128;
    new_gdvp->gdv_view->gv_rect.grs_pos[1] = 128;
    new_gdvp->gdv_view->gv_rect.grs_dim[0] = 256;
    new_gdvp->gdv_view->gv_rect.grs_dim[1] = 256;
    new_gdvp->gdv_view->gv_rect.grs_color[0] = 255;
    new_gdvp->gdv_view->gv_rect.grs_color[1] = 255;
    new_gdvp->gdv_view->gv_rect.grs_color[2] = 255;
    /*XXX this needs to move to configure */
    ged_rect_image2view(&new_gdvp->gdv_view->gv_rect, 512, 512, 1); 

    /* open the framebuffer */
    go_open_fbs(new_gdvp, go_current_gop->go_interp);

    /* Set default bindings */
    {
	struct bu_vls bindings;

	bu_vls_init(&bindings);
	bu_vls_printf(&bindings, "bind %V <Configure> {%V configure %V; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Expose> {%V refresh %V; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "wm protocol %V WM_DELETE_WINDOW {%V delete_view %V; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <2> {%V vslew %V %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <1> {%V zoom %V 0.5; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <3> {%V zoom %V 2.0; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);

	/* Idle Mode */
	bu_vls_printf(&bindings, "bind %V <ButtonRelease> {%V idle_mode %V; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <KeyRelease-Control_L> {%V idle_mode %V; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <KeyRelease-Control_R> {%V idle_mode %V; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <KeyRelease-Shift_L> {%V idle_mode %V; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <KeyRelease-Shift_R> {%V idle_mode %V; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <KeyRelease-Alt_L> {%V idle_mode %V; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <KeyRelease-Alt_R> {%V idle_mode %V; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);

	/* Rotate Mode */
	bu_vls_printf(&bindings, "bind %V <Control-ButtonPress-1> {%V rotate_mode %V %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Control-ButtonPress-2> {%V rotate_mode %V %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Control-ButtonPress-3> {%V rotate_mode %V %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);

	/* Translate Mode */
	bu_vls_printf(&bindings, "bind %V <Shift-ButtonPress-1> {%V translate_mode %V %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Shift-ButtonPress-2> {%V translate_mode %V %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Shift-ButtonPress-3> {%V translate_mode %V %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);

	/* Scale Mode */
	bu_vls_printf(&bindings, "bind %V <Control-Shift-ButtonPress-1> {%V scale_mode %V %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Control-Shift-ButtonPress-2> {%V scale_mode %V %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Control-Shift-ButtonPress-3> {%V scale_mode %V %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Alt-Control-Shift-ButtonPress-1> {%V scale_mode %V %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Alt-Control-Shift-ButtonPress-2> {%V scale_mode %V %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Alt-Control-Shift-ButtonPress-3> {%V scale_mode %V %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);

	/* Constrained Rotate Mode */
	bu_vls_printf(&bindings, "bind %V <Alt-Control-ButtonPress-1> {%V constrain_rmode %V x %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Alt-Control-ButtonPress-2> {%V constrain_rmode %V y %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Alt-Control-ButtonPress-3> {%V constrain_rmode %V z %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);

	/* Constrained Translate Mode */
	bu_vls_printf(&bindings, "bind %V <Alt-Shift-ButtonPress-1> {%V constrain_tmode %V x %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Alt-Shift-ButtonPress-2> {%V constrain_tmode %V y %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V <Alt-Shift-ButtonPress-3> {%V constrain_tmode %V z %%x %%y; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);

	/* Key Bindings */
	bu_vls_printf(&bindings, "bind %V 3 {%V aet %V 35 25; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V 4 {%V aet %V 45 45; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V f {%V aet %V 0 0; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V R {%V aet %V 180 0; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V r {%V aet %V 270 0; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V l {%V aet %V 90 0; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V t {%V aet %V 0 90; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "bind %V b {%V aet %V 0 270; break}; ",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);
	bu_vls_printf(&bindings, "event generate %V <Configure>; %V autoview %V",
		      &new_gdvp->gdv_dmp->dm_pathName,
		      &go_current_gop->go_name,
		      &new_gdvp->gdv_name);

	Tcl_Eval(go_current_gop->go_interp, bu_vls_addr(&bindings));
	bu_vls_free(&bindings);
    }

    return BRLCAD_OK;
}

static int
go_paint_rect_area(struct ged	*gedp,
		   int		argc,
		   const char	*argv[],
		   ged_func_ptr	func,
		   const char	*usage,
		   int		maxargs)
{
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }
    if (argc < 2 || 7 < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    (void)fb_refresh(gdvp->gdv_fbs.fbs_fbp, gdvp->gdv_view->gv_rect.grs_pos[X], gdvp->gdv_view->gv_rect.grs_pos[Y],
		     gdvp->gdv_view->gv_rect.grs_dim[X], gdvp->gdv_view->gv_rect.grs_dim[Y]);

    return BRLCAD_OK;
}

static int
go_rect(struct ged	*gedp,
	int		argc,
	const char	*argv[],
	ged_func_ptr	func,
	const char	*usage,
	int		maxargs)
{
    char *command;
    char *parameter;
    char **argp = (char **)argv;
    point_t user_pt;		/* Value(s) provided by user */
    int i;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }
    if (argc < 2 || 7 < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if (argc == 2) {
	if (gdvp->gdv_view->gv_rect.grs_draw)
	    gdvp->gdv_view->gv_rect.grs_draw = 0;
	else
	    gdvp->gdv_view->gv_rect.grs_draw = 1;

	go_refresh_view(gdvp);
	return BRLCAD_OK;
    }

    command = (char *)argv[0];
    parameter = (char *)argv[2];
    argc -= 3;
    argp += 3;

    for (i = 0; i < argc; ++i)
	user_pt[i] = atof(argp[i]);

    if (strcmp(parameter, "draw") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%d", gdvp->gdv_view->gv_rect.grs_draw);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    i = (int)user_pt[X];

	    if (i)
		gdvp->gdv_view->gv_rect.grs_draw = 1;
	    else
		gdvp->gdv_view->gv_rect.grs_draw = 0;

	    go_refresh_view(gdvp);
	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s draw' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "dim") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%d %d",
			  gdvp->gdv_view->gv_rect.grs_dim[X],
			  gdvp->gdv_view->gv_rect.grs_dim[Y]);
	    return BRLCAD_OK;
	} else if (argc == 2) {
	    gdvp->gdv_view->gv_rect.grs_dim[X] = user_pt[X];
	    gdvp->gdv_view->gv_rect.grs_dim[Y] = user_pt[Y];

	    ged_rect_image2view(&gdvp->gdv_view->gv_rect,
				gdvp->gdv_dmp->dm_width,
				gdvp->gdv_dmp->dm_height,
				gdvp->gdv_dmp->dm_aspect);

	    if (gdvp->gdv_view->gv_rect.grs_draw)
		go_refresh_view(gdvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s dim' command requires 0 or 2 arguments\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "pos") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%d %d",
			  gdvp->gdv_view->gv_rect.grs_pos[X],
			  gdvp->gdv_view->gv_rect.grs_pos[Y]);
	    return BRLCAD_OK;
	} else if (argc == 2) {
	    gdvp->gdv_view->gv_rect.grs_pos[X] = user_pt[X];
	    gdvp->gdv_view->gv_rect.grs_pos[Y] = user_pt[Y];

	    ged_rect_image2view(&gdvp->gdv_view->gv_rect,
				gdvp->gdv_dmp->dm_width,
				gdvp->gdv_dmp->dm_height,
				gdvp->gdv_dmp->dm_aspect);

	    if (gdvp->gdv_view->gv_rect.grs_draw)
		go_refresh_view(gdvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s pos' command requires 0 or 2 arguments\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "color") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%d %d %d",
			  gdvp->gdv_view->gv_rect.grs_color[X],
			  gdvp->gdv_view->gv_rect.grs_color[Y],
			  gdvp->gdv_view->gv_rect.grs_color[Z]);
	    return BRLCAD_OK;
	} else if (argc == 3) {
	    gdvp->gdv_view->gv_rect.grs_color[0] = (int)user_pt[X];
	    gdvp->gdv_view->gv_rect.grs_color[1] = (int)user_pt[Y];
	    gdvp->gdv_view->gv_rect.grs_color[2] = (int)user_pt[Z];

	    if (gdvp->gdv_view->gv_rect.grs_draw)
		go_refresh_view(gdvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s color' command requires 0 or 3 arguments\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "lstyle") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%d", gdvp->gdv_view->gv_rect.grs_linestyle);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    i = (int)user_pt[X];

	    if (i <= 0)
		gdvp->gdv_view->gv_rect.grs_linestyle = 0;
	    else
		gdvp->gdv_view->gv_rect.grs_linestyle = 1;

	    if (gdvp->gdv_view->gv_rect.grs_draw)
		go_refresh_view(gdvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s lw' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "lwidth") == 0) {
	if (argc == 0) {
	    bu_vls_printf(&gedp->ged_result_str, "%d", gdvp->gdv_view->gv_rect.grs_linewidth);
	    return BRLCAD_OK;
	} else if (argc == 1) {
	    i = (int)user_pt[X];

	    if (i <= 0)
		gdvp->gdv_view->gv_rect.grs_linewidth = 1;
	    else
		gdvp->gdv_view->gv_rect.grs_linewidth = i;

	    if (gdvp->gdv_view->gv_rect.grs_draw)
		go_refresh_view(gdvp);

	    return BRLCAD_OK;
	}

	bu_vls_printf(&gedp->ged_result_str, "The '%s lw' command accepts 0 or 1 argument\n", command);
	return BRLCAD_ERROR;
    }

    if (strcmp(parameter, "vars") == 0) {
	ged_rect_vls_print(&gdvp->gdv_view->gv_rect, &gedp->ged_result_str);
	return BRLCAD_OK;
    }

    if (strcmp(parameter, "help") == 0) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", command, usage);
	return BRLCAD_HELP;
    }

    bu_vls_printf(&gedp->ged_result_str, "%s: unrecognized command '%s'\nUsage: %s %s\n",
		  command, parameter, command, usage);
    return BRLCAD_ERROR;
}

static int
go_rect_zoom(struct ged		*gedp,
	     int		argc,
	     const char		*argv[],
	     ged_func_ptr	func,
	     const char		*usage,
	     int		maxargs)
{
    int ret;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 2) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "%s: View not found - %s", argv[0], argv[1]);
	return BRLCAD_ERROR;
    }

    ret = ged_zoom_rect_area(gedp, &gdvp->gdv_view->gv_rect,
			     gdvp->gdv_dmp->dm_width,
			     gdvp->gdv_dmp->dm_height,
			     gdvp->gdv_dmp->dm_aspect);

    go_refresh_view(gdvp);

    return ret;
}

static int
go_refresh(struct ged	*gedp,
	   int		argc,
	   const char	*argv[],
	   ged_func_ptr	func,
	   const char	*usage,
	   int		maxargs)
{
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 2) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

#if 0
    GED_CHECK_DRAWABLE(gedp, BRLCAD_ERROR);
#endif

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    go_refresh_view(gdvp);

    return BRLCAD_OK;
}

static int
go_refresh_all(struct ged	*gedp,
	       int		argc,
	       const char	*argv[],
	       ged_func_ptr	func,
	       const char	*usage,
	       int		maxargs)
{
    if (argc != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s", argv[0]);
	return BRLCAD_ERROR;
    }

    go_refresh_all_views(go_current_gop);

    return BRLCAD_OK;
}

static int
go_rotate_mode(struct ged	*gedp,
	       int		argc,
	       const char	*argv[],
	       ged_func_ptr	func,
	       const char	*usage,
	       int		maxargs)
{
    fastf_t x, y;
    struct bu_vls bindings;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 4) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[2], "%lf", &x) != 1 ||
	sscanf(argv[3], "%lf", &y) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    gdvp->gdv_view->gv_prevMouseX = x;
    gdvp->gdv_view->gv_prevMouseY = y;
    gdvp->gdv_view->gv_mode = GED_ROTATE_MODE;

    bu_vls_init(&bindings);
    bu_vls_printf(&bindings, "bind %V <Motion> {%V mouse_rot %V %%x %%y}",
		  &gdvp->gdv_dmp->dm_pathName,
		  &go_current_gop->go_name,
		  &gdvp->gdv_name);
    Tcl_Eval(go_current_gop->go_interp, bu_vls_addr(&bindings));
    bu_vls_free(&bindings);

    return BRLCAD_OK;
}

/**
 *			GO _ D E L E T E P R O C _ R T
 *@brief
 *  Called when the named proc created by rt_gettrees() is destroyed.
 */
static void
go_deleteProc_rt(ClientData clientData)
{
    struct application	*ap = (struct application *)clientData;
    struct rt_i		*rtip;

    RT_AP_CHECK(ap);
    rtip = ap->a_rt_i;
    RT_CK_RTI(rtip);

    rt_free_rti(rtip);
    ap->a_rt_i = (struct rt_i *)NULL;

    bu_free( (genptr_t)ap, "struct application" );
}

int
go_rt_rect_area(struct ged	*gedp,
		int		argc,
		const char	*argv[],
		ged_func_ptr	func,
		const char	*usage,
		int		maxargs)
{
    int ret;
    struct ged_dm_view *gdvp;
    int color[3] = {0, 0, 0};

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 2) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "%s: View not found - %s", argv[0], argv[1]);
	return BRLCAD_ERROR;
    }

    ret = ged_rt_rect_area(gedp, &gdvp->gdv_view->gv_rect,
			   gdvp->gdv_dmp->dm_width,
			   gdvp->gdv_dmp->dm_height,
			   gdvp->gdv_dmp->dm_aspect,
			   gdvp->gdv_fbs.fbs_listener.fbsl_port,
			   color);

    go_refresh_view(gdvp);

    return ret;
}

/**
 *			G O _ R T _ G E T T R E E S
 *@brief
 *  Given an instance of a database and the name of some treetops,
 *  create a named "ray-tracing" object (proc) which will respond to
 *  subsequent operations.
 *  Returns new proc name as result.
 *
 * @par Example:
 *	.inmem rt_gettrees .rt all.g light.r
 */
int
go_rt_gettrees(struct ged	*gedp,
	       int		argc,
	       const char	*argv[],
	       ged_func_ptr	func,
	       const char	*usage,
	       int		maxargs)
{
    struct rt_i		*rtip;
    struct application	*ap;
    struct resource	*resp;
    char		*newprocname;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc < 3) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    rtip = rt_new_rti(gedp->ged_wdbp->dbip);
    newprocname = (char *)argv[1];

    /* Delete previous proc (if any) to release all that memory, first */
    (void)Tcl_DeleteCommand(go_current_gop->go_interp, newprocname);

    while (argv[2][0] == '-') {
	if (strcmp( argv[2], "-i") == 0) {
	    rtip->rti_dont_instance = 1;
	    argc--;
	    argv++;
	    continue;
	}
	if (strcmp(argv[2], "-u") == 0) {
	    rtip->useair = 1;
	    argc--;
	    argv++;
	    continue;
	}
	break;
    }

    if (rt_gettrees(rtip, argc-2, (const char **)&argv[2], 1) < 0) {
	bu_vls_printf(&gedp->ged_result_str, "rt_gettrees() returned error");
	rt_free_rti(rtip);
	return TCL_ERROR;
    }

    /* Establish defaults for this rt_i */
    rtip->rti_hasty_prep = 1;	/* Tcl isn't going to fire many rays */

    /*
     *  In case of multiple instances of the library, make sure that
     *  each instance has a separate resource structure,
     *  because the bit vector lengths depend on # of solids.
     *  And the "overwrite" sequence in Tcl is to create the new
     *  proc before running the Tcl_CmdDeleteProc on the old one,
     *  which in this case would trash rt_uniresource.
     *  Once on the rti_resources list, rt_clean() will clean 'em up.
     */
    BU_GETSTRUCT(resp, resource);
    rt_init_resource(resp, 0, rtip);
    BU_ASSERT_PTR( BU_PTBL_GET(&rtip->rti_resources, 0), !=, NULL );

    ap = (struct application *)bu_malloc(sizeof(struct application), "go_rt_gettrees: ap");
    RT_APPLICATION_INIT(ap);
    ap->a_magic = RT_AP_MAGIC;
    ap->a_resource = resp;
    ap->a_rt_i = rtip;
    ap->a_purpose = "Conquest!";

    rt_ck(rtip);

    /* Instantiate the proc, with clientData of wdb */
    /* Beware, returns a "token", not TCL_OK. */
    (void)Tcl_CreateCommand(go_current_gop->go_interp, newprocname, rt_tcl_rt,
			    (ClientData)ap, go_deleteProc_rt);

    /* Return new function name as result */
    bu_vls_printf(&gedp->ged_result_str, "%s", newprocname);

    return TCL_OK;
}

static int
go_scale_mode(struct ged	*gedp,
	      int		argc,
	      const char	*argv[],
	      ged_func_ptr	func,
	      const char	*usage,
	      int		maxargs)
{
    fastf_t x, y;
    struct bu_vls bindings;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 4) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[2], "%lf", &x) != 1 ||
	sscanf(argv[3], "%lf", &y) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    gdvp->gdv_view->gv_prevMouseX = x;
    gdvp->gdv_view->gv_prevMouseY = y;
    gdvp->gdv_view->gv_mode = GED_SCALE_MODE;

    bu_vls_init(&bindings);
    bu_vls_printf(&bindings, "bind %V <Motion> {%V mouse_scale %V %%x %%y}",
		  &gdvp->gdv_dmp->dm_pathName,
		  &go_current_gop->go_name,
		  &gdvp->gdv_name);
    Tcl_Eval(go_current_gop->go_interp, bu_vls_addr(&bindings));
    bu_vls_free(&bindings);

    return BRLCAD_OK;
}

static int
go_set_coord(struct ged		*gedp,
	     int		argc,
	     const char		*argv[],
	     ged_func_ptr	func,
	     const char		*usage,
	     int		maxargs)
{
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (3 < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    /* Get coord */
    if (argc == 2) {
	bu_vls_printf(&gedp->ged_result_str, "%c", gdvp->gdv_view->gv_coord);
	return BRLCAD_OK;
    }

    /* Set coord */
    if ((argv[2][0] != 'm' && argv[2][0] != 'v') || argv[2][1] != '\0') {
	bu_vls_printf(&gedp->ged_result_str, "set_coord: bad value - %s\n", argv[2]);
	return BRLCAD_ERROR;
    }

    gdvp->gdv_view->gv_coord = argv[2][0];

    return BRLCAD_OK;
}

static int
go_set_fb_mode(struct ged	*gedp,
	       int		argc,
	       const char	*argv[],
	       ged_func_ptr	func,
	       const char	*usage,
	       int		maxargs)
{
    int mode;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (3 < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    /* Get fb mode */
    if (argc == 2) {
	bu_vls_printf(&gedp->ged_result_str, "%d", gdvp->gdv_fbs.fbs_mode);
	return BRLCAD_OK;
    }

    /* Set fb mode */
    if (sscanf(argv[2], "%d", &mode) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "set_fb_mode: bad value - %s\n", argv[2]);
	return BRLCAD_ERROR;
    }

    if (mode < 0)
	mode = 0;
    else if (GED_OBJ_FB_MODE_OVERLAY < mode)
	mode = GED_OBJ_FB_MODE_OVERLAY;

    gdvp->gdv_fbs.fbs_mode = mode;
    go_refresh_view(gdvp);

    return BRLCAD_OK;
}

static int
go_translate_mode(struct ged	*gedp,
		  int		argc,
		  const char	*argv[],
		  ged_func_ptr	func,
		  const char	*usage,
		  int		maxargs)
{
    fastf_t x, y;
    struct bu_vls bindings;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 4) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[2], "%lf", &x) != 1 ||
	sscanf(argv[3], "%lf", &y) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    gdvp->gdv_view->gv_prevMouseX = x;
    gdvp->gdv_view->gv_prevMouseY = y;
    gdvp->gdv_view->gv_mode = GED_TRANSLATE_MODE;

    bu_vls_init(&bindings);
    bu_vls_printf(&bindings, "bind %V <Motion> {%V mouse_trans %V %%x %%y}",
		  &gdvp->gdv_dmp->dm_pathName,
		  &go_current_gop->go_name,
		  &gdvp->gdv_name);
    Tcl_Eval(go_current_gop->go_interp, bu_vls_addr(&bindings));
    bu_vls_free(&bindings);

    return BRLCAD_OK;
}

static int
go_vmake(struct ged	*gedp,
	 int		argc,
	 const char	*argv[],
	 ged_func_ptr	func,
	 const char	*usage,
	 int		maxargs)
{
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 4) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    {
	int ret;
	char *av[8];
	char center[512];
	char scale[128];

	sprintf(center, "%lf %lf %lf",
		-gdvp->gdv_view->gv_center[MDX],
		-gdvp->gdv_view->gv_center[MDY],
		-gdvp->gdv_view->gv_center[MDZ]);
	sprintf(scale, "%lf", gdvp->gdv_view->gv_scale * 2.0);

	av[0] = (char *)argv[0];
	av[1] = "-o";
	av[2] = center;
	av[3] = "-s";
	av[4] = scale;
	av[5] = (char *)argv[2];
	av[6] = (char *)argv[3];
	av[7] = (char *)0;

	ret = ged_make(gedp, 7, (const char **)av);

	if (ret == BRLCAD_OK) {
	    av[0] = "draw";
	    av[1] = (char *)argv[2];
	    av[2] = (char *)0;
	    go_autoview_func(gedp, 2, (const char **)av, ged_draw, (char *)0, MAXARGS);
	}

	return ret;
    }
}

static int
go_vslew(struct ged	*gedp,
	 int		argc,
	 const char	*argv[],
	 ged_func_ptr	func,
	 const char	*usage,
	 int		maxargs)
{
    int ret;
    int ac;
    char *av[3];
    fastf_t x1, y1;
    fastf_t x2, y2;
    fastf_t sf;
    struct bu_vls slew_vec;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 4) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    if (sscanf(argv[2], "%lf", &x1) != 1 ||
	sscanf(argv[3], "%lf", &y1) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    x2 = 0.5 * gdvp->gdv_dmp->dm_width;
    y2 = 0.5 * gdvp->gdv_dmp->dm_height;
    sf = 2.0 / gdvp->gdv_dmp->dm_width;

    bu_vls_init(&slew_vec);
    bu_vls_printf(&slew_vec, "%lf %lf", (x1 - x2) * sf, (y2 - y1) * sf);

    gedp->ged_gvp = gdvp->gdv_view;
    ac = 2;
    av[0] = (char *)argv[0];
    av[1] = bu_vls_addr(&slew_vec);
    av[2] = (char *)0;

    ret = ged_slew(gedp, ac, (const char **)av);
    bu_vls_free(&slew_vec);

    if (ret == BRLCAD_OK) {
	if (gdvp->gdv_view->gv_grid.ggs_snap)
	    ged_snap_view_center_to_grid(&gdvp->gdv_view->gv_grid, gdvp->gdv_view, gedp->ged_wdbp->dbip->dbi_base2local);
	go_refresh_view(gdvp);
    }

    return ret;
}

static int
go_vsnap(struct ged	*gedp,
	 int		argc,
	 const char	*argv[],
	 ged_func_ptr	func,
	 const char	*usage,
	 int		maxargs)
{
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (argc != 2) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    ged_snap_view_center_to_grid(&gdvp->gdv_view->gv_grid, gdvp->gdv_view, gedp->ged_wdbp->dbip->dbi_base2local);
    go_refresh_view(gdvp);

    return BRLCAD_OK;
}

static int
go_zbuffer(struct ged	*gedp,
	   int		argc,
	   const char	*argv[],
	   ged_func_ptr	func,
	   const char	*usage,
	   int		maxargs)
{
    int zbuffer;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (3 < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    /* get zbuffer flag */
    if (argc == 2) {
	bu_vls_printf(&gedp->ged_result_str, "%d", gdvp->gdv_dmp->dm_zbuffer);
	return BRLCAD_OK;
    }

    /* set zbuffer flag */
    if (sscanf(argv[2], "%d", &zbuffer) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    if (zbuffer < 0)
	zbuffer = 0;
    else if (1 < zbuffer)
	zbuffer = 1;

    DM_SET_ZBUFFER(gdvp->gdv_dmp, zbuffer);
    go_refresh_view(gdvp);

    return BRLCAD_OK;
}

static int
go_zclip(struct ged	*gedp,
	   int		argc,
	   const char	*argv[],
	   ged_func_ptr	func,
	   const char	*usage,
	   int		maxargs)
{
    int zclip;
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (3 < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    /* get zclip flag */
    if (argc == 2) {
	bu_vls_printf(&gedp->ged_result_str, "%d", gdvp->gdv_dmp->dm_zclip);
	return BRLCAD_OK;
    }

    /* set zclip flag */
    if (sscanf(argv[2], "%d", &zclip) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    if (zclip < 0)
	zclip = 0;
    else if (1 < zclip)
	zclip = 1;

    gdvp->gdv_dmp->dm_zclip = zclip;
    go_refresh_view(gdvp);

    return BRLCAD_OK;
}


/*************************** Wrapper Functions ***************************/
static int
go_autoview_func(struct ged	*gedp,
		int		argc,
		const char	*argv[],
		ged_func_ptr	func,
		const char	*usage,
		int		maxargs)
{
    int ret;
    char *av[2];
    int aflag = 0;

    av[0] = "who";
    av[1] = (char *)0;
    ret = ged_who(gedp, 1, (const char **)av);

    if (ret == BRLCAD_OK && strlen(bu_vls_addr(&gedp->ged_result_str)) == 0)
	aflag = 1;

    ret = (*func)(gedp, argc, (const char **)argv);

    if (ret == BRLCAD_OK) {
	if (aflag)
	    go_autoview_all_views(go_current_gop);
	else
	    go_refresh_all_views(go_current_gop);
    }

    return ret;
}

static int
go_pass_through_func(struct ged		*gedp,
		     int		argc,
		     const char		*argv[],
		     ged_func_ptr	func,
		     const char		*usage,
		     int		maxargs)
{
    return (*func)(gedp, argc, argv);
}

static int
go_pass_through_and_refresh_func(struct ged	*gedp,
				 int		argc,
				 const char	*argv[],
				 ged_func_ptr	func,
				 const char	*usage,
				 int		maxargs)
{
    int ret;

    ret = (*func)(gedp, argc, argv);

    if (ret == BRLCAD_OK)
	go_refresh_all_views(go_current_gop);

    return ret;
}

static int
go_view_func(struct ged		*gedp,
	     int		argc,
	     const char		*argv[],
	     ged_func_ptr	func,
	     const char		*usage,
	     int		maxargs)
{
    register int i;
    int ret;
    int ac;
    char *av[MAXARGS];
    struct ged_dm_view *gdvp;

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    if (MAXARGS < maxargs || maxargs < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    for (BU_LIST_FOR(gdvp, ged_dm_view, &go_current_gop->go_head_views.l)) {
	if (!strcmp(bu_vls_addr(&gdvp->gdv_name), argv[1]))
	    break;
    }

    if (BU_LIST_IS_HEAD(&gdvp->l, &go_current_gop->go_head_views.l)) {
	bu_vls_printf(&gedp->ged_result_str, "View not found - %s", argv[1]);
	return BRLCAD_ERROR;
    }

    /* Copy argv into av while skipping argv[1] (i.e. the view name) */
    gedp->ged_gvp = gdvp->gdv_view;
    av[0] = (char *)argv[0];
    ac = argc-1;
    for (i = 2; i < argc; ++i)
	av[i-1] = (char *)argv[i];
    av[i-1] = (char *)0;
    ret = (*func)(gedp, ac, (const char **)av);

    /* Keep the view's perspective in sync with its corresponding display manager */
    gdvp->gdv_dmp->dm_perspective = gdvp->gdv_view->gv_perspective;

    if (ret == BRLCAD_OK)
	go_refresh_view(gdvp);

    return ret;
}


/*************************** Local Utility Functions ***************************/
static void
go_drawSolid(struct dm *dmp, struct solid *sp)
{
    if (sp->s_iflag == UP)
	DM_SET_FGCOLOR(dmp, 255, 255, 255, 0, sp->s_transparency);
    else
	DM_SET_FGCOLOR(dmp,
		       (unsigned char)sp->s_color[0],
		       (unsigned char)sp->s_color[1],
		       (unsigned char)sp->s_color[2], 0, sp->s_transparency);

    DM_DRAW_VLIST(dmp, (struct bn_vlist *)&sp->s_vlist);
}

/* Draw all solids in the list */
static int
go_drawSList(struct dm *dmp, struct bu_list *hsp)
{
    struct solid *sp;
    int linestyle = -1;

    if (dmp->dm_transparency) {
	/* First, draw opaque stuff */
	FOR_ALL_SOLIDS(sp, hsp) {
	    if (sp->s_transparency < 1.0)
		continue;

	    if (linestyle != sp->s_soldash) {
		linestyle = sp->s_soldash;
		DM_SET_LINE_ATTR(dmp, dmp->dm_lineWidth, linestyle);
	    }

	    go_drawSolid(dmp, sp);
	}

	/* disable write to depth buffer */
	DM_SET_DEPTH_MASK(dmp, 0);

	/* Second, draw transparent stuff */
	FOR_ALL_SOLIDS(sp, hsp) {
	    /* already drawn above */
	    if (sp->s_transparency == 1.0)
		continue;

	    if (linestyle != sp->s_soldash) {
		linestyle = sp->s_soldash;
		DM_SET_LINE_ATTR(dmp, dmp->dm_lineWidth, linestyle);
	    }

	    go_drawSolid(dmp, sp);
	}

	/* re-enable write to depth buffer */
	DM_SET_DEPTH_MASK(dmp, 1);
    } else {

	FOR_ALL_SOLIDS(sp, hsp) {
	    if (linestyle != sp->s_soldash) {
		linestyle = sp->s_soldash;
		DM_SET_LINE_ATTR(dmp, dmp->dm_lineWidth, linestyle);
	    }

	    go_drawSolid(dmp, sp);
	}
    }

    return BRLCAD_OK;
}

static void
go_fbs_callback(genptr_t clientData)
{
    struct ged_dm_view *gdvp = (struct ged_dm_view *)clientData;

    go_refresh_view(gdvp);
}

static int
go_close_fbs(struct ged_dm_view *gdvp)
{
    if (gdvp->gdv_fbs.fbs_fbp == FBIO_NULL)
	return TCL_OK;

    _fb_pgflush(gdvp->gdv_fbs.fbs_fbp);

    switch (gdvp->gdv_dmp->dm_type) {
#ifdef DM_X
	case DM_TYPE_X:
	    X24_close_existing(gdvp->gdv_fbs.fbs_fbp);
	    break;
#endif
#ifdef DM_TK
/* XXX TJM: not ready yet
   case DM_TYPE_TK:
   tk_close_existing(gdvp->gdv_fbs.fbs_fbp);
   break;
*/
#endif
#ifdef DM_OGL
	case DM_TYPE_OGL:
	    ogl_close_existing(gdvp->gdv_fbs.fbs_fbp);
	    break;
#endif
#ifdef DM_WGL
	case DM_TYPE_WGL:
	    wgl_close_existing(gdvp->gdv_fbs.fbs_fbp);
	    break;
#endif
    }

    /* free framebuffer memory */
    if (gdvp->gdv_fbs.fbs_fbp->if_pbase != PIXEL_NULL)
	free((void *)gdvp->gdv_fbs.fbs_fbp->if_pbase);
    free((void *)gdvp->gdv_fbs.fbs_fbp->if_name);
    free((void *)gdvp->gdv_fbs.fbs_fbp);
    gdvp->gdv_fbs.fbs_fbp = FBIO_NULL;

    return TCL_OK;
}

/*
 * Open/activate the display managers framebuffer.
 */
static int
go_open_fbs(struct ged_dm_view *gdvp, Tcl_Interp *interp)
{

    /* already open */
    if (gdvp->gdv_fbs.fbs_fbp != FBIO_NULL)
	return TCL_OK;

    /* don't use bu_calloc so we can fail slightly more gradefully */
    if ((gdvp->gdv_fbs.fbs_fbp = (FBIO *)calloc(sizeof(FBIO), 1)) == FBIO_NULL) {
	Tcl_Obj	*obj;

	obj = Tcl_GetObjResult(interp);
	if (Tcl_IsShared(obj))
	    obj = Tcl_DuplicateObj(obj);

	Tcl_AppendStringsToObj(obj, "openfb: failed to allocate framebuffer memory\n",
			       (char *)NULL);

	Tcl_SetObjResult(interp, obj);
	return TCL_ERROR;
    }

    switch (gdvp->gdv_dmp->dm_type) {
#ifdef DM_X
	case DM_TYPE_X:
	    *gdvp->gdv_fbs.fbs_fbp = X24_interface; /* struct copy */

	    gdvp->gdv_fbs.fbs_fbp->if_name = bu_malloc((unsigned)strlen("/dev/X")+1, "if_name");
	    bu_strlcpy(gdvp->gdv_fbs.fbs_fbp->if_name, "/dev/X", strlen("/dev/X")+1);

	    /* Mark OK by filling in magic number */
	    gdvp->gdv_fbs.fbs_fbp->if_magic = FB_MAGIC;

	    _X24_open_existing(gdvp->gdv_fbs.fbs_fbp,
			       ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->dpy,
			       ((struct x_vars *)gdvp->gdv_dmp->dm_vars.priv_vars)->pix,
			       ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->win,
			       ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->cmap,
			       ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->vip,
			       gdvp->gdv_dmp->dm_width,
			       gdvp->gdv_dmp->dm_height,
			       ((struct x_vars *)gdvp->gdv_dmp->dm_vars.priv_vars)->gc);
	    break;
#endif
#ifdef DM_TK
#if 0
/* XXX TJM implement _tk_open_existing */
	case DM_TYPE_TK:
	    *gdvp->gdv_fbs.fbs_fbp = tk_interface; /* struct copy */

	    gdvp->gdv_fbs.fbs_fbp->if_name = bu_malloc((unsigned)strlen("/dev/tk")+1, "if_name");
	    bu_strlcpy(gdvp->gdv_fbs.fbs_fbp->if_name, "/dev/tk", strlen("/dev/tk")+1);

	    /* Mark OK by filling in magic number */
	    gdvp->gdv_fbs.fbs_fbp->if_magic = FB_MAGIC;

	    _tk_open_existing(gdvp->gdv_fbs.fbs_fbp,
			      ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->dpy,
			      ((struct x_vars *)gdvp->gdv_dmp->dm_vars.priv_vars)->pix,
			      ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->win,
			      ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->cmap,
			      ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->vip,
			      gdvp->gdv_dmp->dm_width,
			      gdvp->gdv_dmp->dm_height,
			      ((struct x_vars *)gdvp->gdv_dmp->dm_vars.priv_vars)->gc);
	    break;
#endif
#endif

#ifdef DM_OGL
	case DM_TYPE_OGL:
	    *gdvp->gdv_fbs.fbs_fbp = ogl_interface; /* struct copy */

	    gdvp->gdv_fbs.fbs_fbp->if_name = bu_malloc((unsigned)strlen("/dev/ogl")+1, "if_name");
	    bu_strlcpy(gdvp->gdv_fbs.fbs_fbp->if_name, "/dev/ogl", strlen("/dev/ogl")+1);

	    /* Mark OK by filling in magic number */
	    gdvp->gdv_fbs.fbs_fbp->if_magic = FB_MAGIC;

	    _ogl_open_existing(gdvp->gdv_fbs.fbs_fbp,
			       ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->dpy,
			       ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->win,
			       ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->cmap,
			       ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->vip,
			       gdvp->gdv_dmp->dm_width,
			       gdvp->gdv_dmp->dm_height,
			       ((struct ogl_vars *)gdvp->gdv_dmp->dm_vars.priv_vars)->glxc,
			       ((struct ogl_vars *)gdvp->gdv_dmp->dm_vars.priv_vars)->mvars.doublebuffer,
			       0);
	    break;
#endif
#ifdef DM_WGL
	case DM_TYPE_WGL:
	    *gdvp->gdv_fbs.fbs_fbp = wgl_interface; /* struct copy */

	    gdvp->gdv_fbs.fbs_fbp->if_name = bu_malloc((unsigned)strlen("/dev/wgl")+1, "if_name");
	    bu_strlcpy(gdvp->gdv_fbs.fbs_fbp->if_name, "/dev/wgl", strlen("/dev/wgl")+1);

	    /* Mark OK by filling in magic number */
	    gdvp->gdv_fbs.fbs_fbp->if_magic = FB_MAGIC;

	    _wgl_open_existing(gdvp->gdv_fbs.fbs_fbp,
			       ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->dpy,
			       ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->win,
			       ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->cmap,
			       ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->vip,
			       ((struct dm_xvars *)gdvp->gdv_dmp->dm_vars.pub_vars)->hdc,
			       gdvp->gdv_dmp->dm_width,
			       gdvp->gdv_dmp->dm_height,
			       ((struct wgl_vars *)gdvp->gdv_dmp->dm_vars.priv_vars)->glxc,
			       ((struct wgl_vars *)gdvp->gdv_dmp->dm_vars.priv_vars)->mvars.doublebuffer,
			       0);
	    break;
#endif
    }

    return TCL_OK;
}

static void
go_draw(struct ged_dm_view *gdvp)
{
    mat_t new;
    matp_t mat;
    mat_t perspective_mat;

    mat = gdvp->gdv_view->gv_model2view;

    if (0 < gdvp->gdv_view->gv_perspective) {
#if 1
	point_t l, h;

	VSET(l, -1.0, -1.0, -1.0);
	VSET(h, 1.0, 1.0, 200.0);

	if (gdvp->gdv_view->gv_eye_pos[Z] == 1.0) {
	    /* This way works, with reasonable Z-clipping */
	    ged_persp_mat(perspective_mat, gdvp->gdv_view->gv_perspective,
			  (fastf_t)1.0f, (fastf_t)0.01f, (fastf_t)1.0e10f, (fastf_t)1.0f);
	} else {
	    /* This way does not have reasonable Z-clipping,
	     * but includes shear, for GDurf's testing.
	     */
	    ged_deering_persp_mat(perspective_mat, l, h, gdvp->gdv_view->gv_eye_pos);
	}
#else
	    /*
	     *  There are two strategies that could be used:
	     *  1)  Assume a standard head location w.r.t. the
	     *  screen, and fix the perspective angle.
	     *  2)  Based upon the perspective angle, compute
	     *  where the head should be to achieve that field of view.
	     *  Try strategy #2 for now.
	     */
	    fastf_t	to_eye_scr;	/* screen space dist to eye */
	    point_t	l, h, eye;

	    /* Determine where eye should be */
	    to_eye_scr = 1 / tan(gdvp->gdv_view->gv_perspective * bn_degtorad * 0.5);

	    VSET(l, -1.0, -1.0, -1.0);
	    VSET(h, 1.0, 1.0, 200.0);
	    VSET(eye, 0.0, 0.0, to_eye_scr);

	    /* Non-stereo case */
	    ged_mike_persp_mat(perspective_mat, gdvp->gdv_view->gv_eye_pos);
#endif

	bn_mat_mul(new, perspective_mat, mat);
	mat = new;
    }

    DM_LOADMATRIX(gdvp->gdv_dmp, mat, 0);
    go_drawSList(gdvp->gdv_dmp, &gdvp->gdv_gop->go_gedp->ged_gdp->gd_headSolid);
}

static void
go_refresh_view(struct ged_dm_view *gdvp)
{
    DM_DRAW_BEGIN(gdvp->gdv_dmp);

    if (gdvp->gdv_fbs.fbs_mode == GED_OBJ_FB_MODE_OVERLAY) {
	if (gdvp->gdv_view->gv_rect.grs_draw) {
	    go_draw(gdvp);
	    fb_refresh(gdvp->gdv_fbs.fbs_fbp,
		       gdvp->gdv_view->gv_rect.grs_pos[X], gdvp->gdv_view->gv_rect.grs_pos[Y],
		       gdvp->gdv_view->gv_rect.grs_dim[X], gdvp->gdv_view->gv_rect.grs_dim[Y]);

	    /* Restore to non-rotated, full brightness */
	    DM_NORMAL(gdvp->gdv_dmp);
	    dm_draw_rect(gdvp->gdv_dmp, &gdvp->gdv_view->gv_rect, gdvp->gdv_view);

	} else
	    fb_refresh(gdvp->gdv_fbs.fbs_fbp, 0, 0,
		       gdvp->gdv_dmp->dm_width, gdvp->gdv_dmp->dm_height);

	DM_DRAW_END(gdvp->gdv_dmp);
	return;
    } else if (gdvp->gdv_fbs.fbs_mode == GED_OBJ_FB_MODE_INTERLAY) {
	go_draw(gdvp);

	if (gdvp->gdv_view->gv_rect.grs_draw) {
	    go_draw(gdvp);
	    fb_refresh(gdvp->gdv_fbs.fbs_fbp,
		       gdvp->gdv_view->gv_rect.grs_pos[X], gdvp->gdv_view->gv_rect.grs_pos[Y],
		       gdvp->gdv_view->gv_rect.grs_dim[X], gdvp->gdv_view->gv_rect.grs_dim[Y]);
	} else
	    fb_refresh(gdvp->gdv_fbs.fbs_fbp, 0, 0,
		       gdvp->gdv_dmp->dm_width, gdvp->gdv_dmp->dm_height);
    } else {
	if (gdvp->gdv_fbs.fbs_mode == GED_OBJ_FB_MODE_UNDERLAY) {
	    if (gdvp->gdv_view->gv_rect.grs_draw) {
		fb_refresh(gdvp->gdv_fbs.fbs_fbp,
			   gdvp->gdv_view->gv_rect.grs_pos[X], gdvp->gdv_view->gv_rect.grs_pos[Y],
			   gdvp->gdv_view->gv_rect.grs_dim[X], gdvp->gdv_view->gv_rect.grs_dim[Y]);
	    } else
		fb_refresh(gdvp->gdv_fbs.fbs_fbp, 0, 0,
			   gdvp->gdv_dmp->dm_width, gdvp->gdv_dmp->dm_height);
	}

	go_draw(gdvp);
    } 

    /* Restore to non-rotated, full brightness */
    DM_NORMAL(gdvp->gdv_dmp);

#if 1
    /* Draw center dot */
    DM_SET_FGCOLOR(gdvp->gdv_dmp,
		   255, 255, 0, 1, 1.0);
#else
    /* Draw center dot */
    DM_SET_FGCOLOR(gdvp->gdv_dmp,
		   color_scheme->cs_center_dot[0],
		   color_scheme->cs_center_dot[1],
		   color_scheme->cs_center_dot[2], 1, 1.0);
#endif
    DM_DRAW_POINT_2D(gdvp->gdv_dmp, 0.0, 0.0);

    /*XXX Whether or not and how things are drawn needs to be application settable.
     *    For the moment, things are hardwired.
     */
    /* Draw view axes */
    {
	point_t origin = {0.0, 0.0, 0.0};
	int axes_color[3] = {100, 100, 255};
	int axes_label_color[3] = {255, 255, 0};

	dmo_drawAxes_cmd(gdvp->gdv_dmp,
			 gdvp->gdv_view->gv_size,
			 gdvp->gdv_view->gv_rotation,
			 origin,
			 0.25,
			 axes_color,
			 axes_label_color,
			 0, /* line width */
			 0, /* positive direction only */
			 0, /* three colors (i.e. X-red, Y-green, Z-blue) */
			 0, /* no ticks */
			 0, /* tick len */
			 0, /* major tick len */
			 0, /* tick interval */
			 0, /* ticks per major */
			 NULL, /* tick color */
			 NULL, /* major tick color */
			 0 /* tick threshold */);
    }

    /* Draw the angle distance cursor */
    if (gdvp->gdv_view->gv_adc.gas_draw)
	dm_draw_adc(gdvp->gdv_dmp, gdvp->gdv_view);

    /* Draw grid */
    if (gdvp->gdv_view->gv_grid.ggs_draw)
	dm_draw_grid(gdvp->gdv_dmp, &gdvp->gdv_view->gv_grid, gdvp->gdv_view, gdvp->gdv_gop->go_gedp->ged_wdbp->dbip->dbi_base2local);

    /* Draw rect */
    if (gdvp->gdv_view->gv_rect.grs_draw)
	dm_draw_rect(gdvp->gdv_dmp, &gdvp->gdv_view->gv_rect, gdvp->gdv_view);

    DM_DRAW_END(gdvp->gdv_dmp);
}

static void
go_refresh_all_views(struct ged_obj *gop)
{
    struct ged_dm_view *gdvp;

    for (BU_LIST_FOR(gdvp, ged_dm_view, &gop->go_head_views.l)) {
	go_refresh_view(gdvp);
    }
}

static void
go_autoview_view(struct ged_dm_view *gdvp)
{
    int ret;
    char *av[2];

    gdvp->gdv_gop->go_gedp->ged_gvp = gdvp->gdv_view;
    av[0] = "autoview";
    av[1] = (char *)0;
    ret = ged_autoview(gdvp->gdv_gop->go_gedp, 1, (const char **)av);

    if (ret == BRLCAD_OK)
	go_refresh_view(gdvp);
}

static void
go_autoview_all_views(struct ged_obj *gop)
{
    struct ged_dm_view *gdvp;

    for (BU_LIST_FOR(gdvp, ged_dm_view, &gop->go_head_views.l)) {
	go_autoview_view(gdvp);
    }
}

static void
go_output_handler(struct ged *gedp, char *line)
{
    if (gedp->ged_output_script != (char *)0) {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "%s \"%s\"", gedp->ged_output_script, line);
	Tcl_Eval(go_current_gop->go_interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
    } else {
	struct bu_vls vls;

	bu_vls_init(&vls);
	bu_vls_printf(&vls, "puts \"%s\"", line);
	Tcl_Eval(go_current_gop->go_interp, bu_vls_addr(&vls));
	bu_vls_free(&vls);
    }
}


/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
