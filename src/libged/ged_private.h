/*                   G E D _ P R I V A T E . H
 * BRL-CAD
 *
 * Copyright (c) 2008 United States Government as represented by
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

#include "ged.h"

__BEGIN_DECLS

#define GED_MAX_LEVELS 12
#define GED_CPEVAL	0
#define GED_LISTPATH	1
#define GED_LISTEVAL	2
#define GED_EVAL_ONLY	3

/*
 * rt_comb_ifree() should NOT be used here because
 * it doesn't know how to free attributes.
 * rt_db_free_internal() should be used instead.
 */
#define USE_RT_COMB_IFREE 0

#define GED_WIREFRAME 0
#define GED_SHADED_MODE_BOTS 1
#define GED_SHADED_MODE_ALL 2
#define GED_BOOL_EVAL 3

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

/* defined in draw.c */
BU_EXTERN (void ged_color_soltab,
	   (struct solid *hsp));

BU_EXTERN (void ged_cvt_vlblock_to_solids,
	   (struct ged *gedp,
	    struct bn_vlblock *vbp,
	    char *name,
	    int copy));


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

/* defined in trace.c */
BU_EXTERN (void ged_trace,
	   (register struct directory	*dp,
	    int				pathpos,
	    const mat_t			old_xlate,
	    struct ged_trace_data	*gtdp));

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

/* defined in rt.c */
void
BU_EXTERN (ged_rt_set_eye_model,
	   (struct ged *gedp,
	    vect_t eye_model));

/* defined in vutil.c */
BU_EXTERN (void ged_view_update,
	   (struct ged_view *gvp));


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
