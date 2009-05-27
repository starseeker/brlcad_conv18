/*                   G E D _ P R I V A T E . H
 * BRL-CAD
 *
 * Copyright (c) 2008-2009 United States Government as represented by
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
/** @file ged_private.h
 *
 * Private header for libged.
 *
 */

#ifndef __GED_PRIVATE_H__
#define __GED_PRIVATE_H__

#include "common.h"

#include <time.h>

#include "db.h"
#include "mater.h"
#include "rtgeom.h"
#include "ged.h"

__BEGIN_DECLS

#define GED_V4_MAXNAME	NAMESIZE
#define GED_TERMINAL_WIDTH 80
#define GED_COLUMNS ((GED_TERMINAL_WIDTH + GED_V4_MAXNAME - 1) / GED_V4_MAXNAME)

#define GED_MAX_LEVELS 12
#define GED_CPEVAL	0
#define GED_LISTPATH	1
#define GED_LISTEVAL	2
#define GED_EVAL_ONLY	3

#define GED_WIREFRAME        0
#define GED_SHADED_MODE_BOTS 1
#define GED_SHADED_MODE_ALL  2
#define GED_BOOL_EVAL        3

struct ged_id_names {
    struct bu_list l;
    struct bu_vls name;		/**< name associated with region id */
};

struct ged_id_to_names {
    struct bu_list l;
    int id;				/**< starting id (i.e. region id or air code) */
    struct ged_id_names headName;	/**< head of list of names */
};

struct ged_client_data {
    struct ged	       	*gedp;
    int			wireframe_color_override;
    int			wireframe_color[3];
    int			draw_nmg_only;
    int			nmg_triangulate;
    int			draw_wireframes;
    int			draw_normals;
    int			draw_solid_lines_only;
    int			draw_no_surfaces;
    int			shade_per_vertex_normals;
    int			draw_edge_uses;
    int			fastpath_count;			/* statistics */
    int			do_not_draw_nmg_solids_during_debugging;
    struct bn_vlblock	*draw_edge_uses_vbp;
    int			shaded_mode_override;
    fastf_t		transparency;
    int			dmode;
    /* bigE related members */
    struct application	*ap;
    struct bu_ptbl	leaf_list;
    struct rt_i		*rtip;
    time_t		start_time;
    time_t		etime;
    long		nvectors;
    int			do_polysolids;
    int			num_halfs;
};

struct ged_rt_client_data {
    struct ged_run_rt 	*rrtp;
    struct ged	       	*gedp;
};

struct ged_trace_data {
    struct ged	      *gtd_gedp;
    struct directory  *gtd_path[GED_MAX_LEVELS];
    struct directory  *gtd_obj[GED_MAX_LEVELS];
    mat_t	      gtd_xform;
    int		      gtd_objpos;
    int		      gtd_prflag;
    int		      gtd_flag;
};

/* defined in globals.c */
extern struct solid FreeSolid;

/* defined in ged.c */
BU_EXTERN (void ged_print_node,
	   (struct ged		*gedp,
	    register struct directory *dp,
	    int			pathpos,
	    int			indentSize,
	    char		prefix,
	    int			cflag,
	    int                 displayDepth,
	    int                 currdisplayDepth));
BU_EXTERN (struct db_i *ged_open_dbip,
	   (const char *filename,
	    int existing_only));

/* defined in clip.c */
BU_EXTERN (int ged_clip,
	   (fastf_t *xp1,
	    fastf_t *yp1,
	    fastf_t *xp2,
	    fastf_t *yp2));
BU_EXTERN (int ged_vclip,
	   (vect_t a,
	    vect_t b,
	    register fastf_t *min,
	    register fastf_t *max));

/* defined in color.c */
BU_EXTERN (void ged_color_putrec,
	   (struct ged			*gedp,
	    register struct mater	*mp));
	    
BU_EXTERN (void ged_color_zaprec,
	   (struct ged			*gedp,
	    register struct mater	*mp));

/* defined in comb.c */
BU_EXTERN (struct directory *ged_combadd,
	   (struct ged			*gedp,
	    register struct directory	*objp,
	    char			*combname,
	    int				region_flag,
	    int				relation,
	    int				ident,
	    int				air));

/* defined in draw.c */
BU_EXTERN (void ged_color_soltab,
	   (struct solid *hsp));

BU_EXTERN (void ged_cvt_vlblock_to_solids,
	   (struct ged *gedp,
	    struct bn_vlblock *vbp,
	    char *name,
	    int copy));
BU_EXTERN (int ged_invent_solid,
	   (struct ged		*gedp,
	    char		*name,
	    struct bu_list	*vhead,
	    long int		rgb,
	    int			copy,
	    fastf_t		transparency,
	    int			dmode));
BU_EXTERN (int ged_drawtrees,
	   (struct ged *gedp,
	    int argc,
	    const char *argv[],
	    int kind,
	    struct ged_client_data *_dgcdp));


/* defined in erase.c */
BU_EXTERN (void ged_eraseobjpath,
	   (struct ged	*gedp,
	    int		argc,
	    const char	*argv[],
	    int		noisy,
	    int		all));
BU_EXTERN (void ged_eraseobjall,
	   (struct ged			*gedp,
	    register struct directory	**dpp));
BU_EXTERN (void ged_eraseobj,
	   (struct ged			*gedp,
	    register struct directory	**dpp));

/* defined in get_comb.c */
BU_EXTERN(void ged_vls_print_matrix,
	  (struct bu_vls *vls,
	   matp_t matrix));

/* defined in get_obj_bounds.c */
BU_EXTERN (int ged_get_obj_bounds,
	   (struct ged		*gedp,
	    int			argc,
	    const char		*argv[],
	    int			use_air,
	    point_t		rpp_min,
	    point_t		rpp_max));

BU_EXTERN (int ged_get_obj_bounds2,
	   (struct ged			*gedp,
	    int				argc,
	    const char			*argv[],
	    struct ged_trace_data	*gtdp,
	    point_t			rpp_min,
	    point_t			rpp_max));

/* defined in how.c */
BU_EXTERN (struct directory **ged_build_dpp,
	   (struct ged *gedp,
	    const char *path));

/* defined in list.c */
BU_EXTERN(void ged_do_list,
	  (struct ged			*gedp,
	   register struct directory	*dp,
	   int				verbose));

/* defined in loadview.c */
extern vect_t ged_eye_model;
extern mat_t ged_viewrot;
extern struct ged *ged_current_gedp;
BU_EXTERN (int ged_cm_vsize,
	   (int argc,
	    char **argv));
BU_EXTERN (int ged_cm_eyept,
	   (int argc,
	    char **argv));
BU_EXTERN (int ged_cm_lookat_pt,
	   (int argc,
	    char **argv));
BU_EXTERN (int ged_cm_vrot,
	   (int argc,
	    char **argv));
BU_EXTERN (int ged_cm_orientation,
	   (int argc,
	    char **argv));
BU_EXTERN (int ged_cm_set,
	   (int argc,
	    char **argv));
BU_EXTERN (int ged_cm_null,
	   (int argc,
	    char **argv));


/* defined in ls.c */
BU_EXTERN(void ged_vls_col_pr4v,
	  (struct bu_vls	*vls,
	   struct directory	**list_of_names,
	   int			num_in_list,
	   int			no_decorate));
BU_EXTERN(void ged_vls_long_dpp,
	  (struct bu_vls	*vls,
	   struct directory	**list_of_names,
	   int			num_in_list,
	   int			aflag,		/* print all objects */
	   int			cflag,		/* print combinations */
	   int			rflag,		/* print regions */
	   int			sflag));	/* print solids */
BU_EXTERN(void ged_vls_line_dpp,
	  (struct bu_vls	*vls,
	   struct directory	**list_of_names,
	   int			num_in_list,
	   int			aflag,	/* print all objects */
	   int			cflag,	/* print combinations */
	   int			rflag,	/* print regions */
	   int			sflag));	/* print solids */
BU_EXTERN(struct directory ** ged_getspace,
	  (struct db_i	*dbip,
	   register int	num_entries));

/* defined in preview.c */
BU_EXTERN (void ged_setup_rt,
	   (struct ged *gedp,
	    register char **vp,
	    int printcmd));

/* defined in red.c */
extern char ged_tmpfil[MAXPATHLEN];
extern char ged_tmpcomb[17];
extern char *ged_tmpcomb_init;
extern char delims[];
BU_EXTERN(int ged_make_tree,
	  (struct ged *gedp,
	   struct rt_comb_internal *comb,
	   struct directory *dp,
	   int node_count,
	   const char *old_name,
	   const char *new_name,
	   struct rt_tree_array *rt_tree_array,
	   int tree_index));
BU_EXTERN(int ged_save_comb,
	  (struct ged *gedp,
	   struct directory *dpold));
BU_EXTERN(void ged_restore_comb,
	  (struct ged *gedp,
	   struct directory *dp));
BU_EXTERN(void ged_print_matrix,
	  (FILE *fp, matp_t matrix));

/* defined in rt.c */
BU_EXTERN (void ged_rt_set_eye_model,
	   (struct ged *gedp,
	    vect_t eye_model));
BU_EXTERN (int ged_run_rt,
	   (struct ged *gdp));
BU_EXTERN (void ged_rt_write,
	   (struct ged *gedp,
	    FILE *fp,
	    vect_t eye_model));
BU_EXTERN (void ged_rt_output_handler,
	   (ClientData clientData,
	    int	 mask));
BU_EXTERN (int ged_build_tops,
	   (struct ged	*gedp,
	    char		**start,
	    register char	**end));

/* defined in rtcheck.c */
BU_EXTERN (void ged_wait_status,
	   (struct bu_vls *log,
	    int status));

/* defined in rotate_eto.c */
BU_EXTERN (int ged_rotate_eto,
	   (struct ged *gedp,
	    struct rt_eto_internal *eto,
	    const char *attribute,
	    matp_t rmat));

/* defined in rotate_extrude.c */
BU_EXTERN (int ged_rotate_extrude,
	   (struct ged *gedp,
	    struct rt_extrude_internal *extrude,
	    const char *attribute,
	    matp_t rmat));

/* defined in rotate_hyp.c */
BU_EXTERN (int ged_rotate_hyp,
	   (struct ged *gedp,
	    struct rt_hyp_internal *hyp,
	    const char *attribute,
	    matp_t rmat));

/* defined in rotate_tgc.c */
BU_EXTERN (int ged_rotate_tgc,
	   (struct ged *gedp,
	    struct rt_tgc_internal *tgc,
	    const char *attribute,
	    matp_t rmat));

/* defined in scale_ehy.c */
BU_EXTERN (int ged_scale_ehy,
	   (struct ged *gedp,
	    struct rt_ehy_internal *ehy,
	    const char *attribute,
	    fastf_t sf));

/* defined in scale_ell.c */
BU_EXTERN (int ged_scale_ell,
	   (struct ged *gedp,
	    struct rt_ell_internal *ell,
	    const char *attribute,
	    fastf_t sf));

/* defined in scale_epa.c */
BU_EXTERN (int ged_scale_epa,
	   (struct ged *gedp,
	    struct rt_epa_internal *epa,
	    const char *attribute,
	    fastf_t sf));

/* defined in scale_eto.c */
BU_EXTERN (int ged_scale_eto,
	   (struct ged *gedp,
	    struct rt_eto_internal *eto,
	    const char *attribute,
	    fastf_t sf));

/* defined in scale_extrude.c */
BU_EXTERN (int ged_scale_extrude,
	   (struct ged *gedp,
	    struct rt_extrude_internal *extrude,
	    const char *attribute,
	    fastf_t sf));

/* defined in scale_hyp.c */
BU_EXTERN (int ged_scale_hyp,
	   (struct ged *gedp,
	    struct rt_hyp_internal *hyp,
	    const char *attribute,
	    fastf_t sf));

/* defined in scale_part.c */
BU_EXTERN (int ged_scale_part,
	   (struct ged *gedp,
	    struct rt_part_internal *part,
	    const char *attribute,
	    fastf_t sf));

/* defined in scale_rhc.c */
BU_EXTERN (int ged_scale_rhc,
	   (struct ged *gedp,
	    struct rt_rhc_internal *rhc,
	    const char *attribute,
	    fastf_t sf));

/* defined in scale_rpc.c */
BU_EXTERN (int ged_scale_rpc,
	   (struct ged *gedp,
	    struct rt_rpc_internal *rpc,
	    const char *attribute,
	    fastf_t sf));

/* defined in scale_superell.c */
BU_EXTERN (int ged_scale_superell,
	   (struct ged *gedp,
	    struct rt_superell_internal *superell,
	    const char *attribute,
	    fastf_t sf));

/* defined in scale_tgc.c */
BU_EXTERN (int ged_scale_tgc,
	   (struct ged *gedp,
	    struct rt_tgc_internal *tgc,
	    const char *attribute,
	    fastf_t sf));

/* defined in scale_tor.c */
BU_EXTERN (int ged_scale_tor,
	   (struct ged *gedp,
	    struct rt_tor_internal *tor,
	    const char *attribute,
	    fastf_t sf));

/* defined in tops.c */
struct directory **
ged_dir_getspace(struct db_i	*dbip,
		 register int	num_entries);

/* defined in trace.c */
BU_EXTERN (void ged_trace,
	   (register struct directory	*dp,
	    int				pathpos,
	    const mat_t			old_xlate,
	    struct ged_trace_data	*gtdp));

/* defined in translate_extrude.c */
BU_EXTERN (int ged_translate_extrude,
	   (struct ged *gedp,
	    struct rt_extrude_internal *extrude,
	    const char *attribute,
	    vect_t tvec,
	    int rflag));

/* defined in translate_tgc.c */
BU_EXTERN (int ged_translate_tgc,
	   (struct ged *gedp,
	    struct rt_tgc_internal *tgc,
	    const char *attribute,
	    vect_t tvec,
	    int rflag));

/* defined in vutil.c */
BU_EXTERN (void ged_view_update,
	   (struct ged_view *gvp));
BU_EXTERN (void ged_mat_aet,
	   (struct ged_view *gvp));
BU_EXTERN (int ged_do_rot,
	   (struct ged	*gedp,
	    char	coord,
	    mat_t	rmat,
	    int		(*func)()));
BU_EXTERN (int ged_do_slew,
	   (struct ged	*gedp,
	    vect_t	svec));
BU_EXTERN (int ged_do_tra,
	   (struct ged	*gedp,
	    char	coord,
	    vect_t	tvec,
	    int		(*func)()));
BU_EXTERN (int ged_do_zoom,
	   (struct ged	*gedp,
	    fastf_t	sf));



__END_DECLS

#endif /* __GED_PRIVATE_H__ */

/** @} */
/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
