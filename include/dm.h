/*                          D M . H
 * BRL-CAD
 *
 * Copyright (c) 1993-2020 United States Government as represented by
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
/** @addtogroup libdm */
/** @{ */
/** @file dm.h
 *
 */

#ifndef DM_H
#define DM_H

#include "common.h"

#include "vmath.h"
#include "bn.h"
#include "raytrace.h"
#include "fb.h"

#include "./dm/defines.h"

__BEGIN_DECLS

/* Use fbserv */
#define USE_FBSERV 1

#define DM_NULL (struct dm *)NULL

/* Display Manager Types */
#define DM_TYPE_BAD     -1
#define DM_TYPE_NULL	0
#define DM_TYPE_PLOT	1
#define DM_TYPE_PS	2
#define DM_TYPE_X	3
#define DM_TYPE_OGL	4
#define DM_TYPE_GLX	5
#define DM_TYPE_PEX	6
#define DM_TYPE_WGL	7
#define DM_TYPE_TK	8
#define DM_TYPE_RTGL	9
#define DM_TYPE_TXT	10
#define DM_TYPE_QT	11
#define DM_TYPE_OSG	12
#define DM_TYPE_OSGL	13

#define IS_DM_TYPE_NULL(_t) ((_t) == DM_TYPE_NULL)
#define IS_DM_TYPE_PLOT(_t) ((_t) == DM_TYPE_PLOT)
#define IS_DM_TYPE_PS(_t) ((_t) == DM_TYPE_PS)
#define IS_DM_TYPE_X(_t) ((_t) == DM_TYPE_X)
#define IS_DM_TYPE_TK(_t) ((_t) == DM_TYPE_TK)
#define IS_DM_TYPE_OGL(_t) ((_t) == DM_TYPE_OGL)
#define IS_DM_TYPE_GLX(_t) ((_t) == DM_TYPE_GLX)
#define IS_DM_TYPE_PEX(_t) ((_t) == DM_TYPE_PEX)
#define IS_DM_TYPE_WGL(_t) ((_t) == DM_TYPE_WGL)
#define IS_DM_TYPE_RTGL(_t) ((_t) == DM_TYPE_RTGL)
#define IS_DM_TYPE_TXT(_t) ((_t) == DM_TYPE_TXT)
#define IS_DM_TYPE_QT(_t) ((_t) == DM_TYPE_QT)
#define IS_DM_TYPE_OSG(_t) ((_t) == DM_TYPE_OSG)
#define IS_DM_TYPE_OSGL(_t) ((_t) == DM_TYPE_OSGL)

/* the font used depends on the size of the window opened */
#define FONTBACK "-adobe-courier-medium-r-normal--10-100-75-75-m-60-iso8859-1"
#define FONT5 "5x7"
#define FONT6 "6x10"
#define FONT7 "7x13"
#define FONT8 "8x13"
#define FONT9 "9x15"
#define FONT10 "10x20"
#define FONT12 "12x24"

#if defined(_WIN32) && !defined(__CYGWIN__)
#  define DM_VALID_FONT_SIZE(_size) (14 <= (_size) && (_size) <= 29)
#else
#  define DM_VALID_FONT_SIZE(_size) (5 <= (_size) && (_size) <= 12 && (_size) != 11)
#  define DM_FONT_SIZE_TO_NAME(_size) (((_size) == 5) ? FONT5 : (((_size) == 6) ? FONT6 : (((_size) == 7) ? FONT7 : (((_size) == 8) ? FONT8 : (((_size) == 9) ? FONT9 : (((_size) == 10) ? FONT10 : FONT12))))))
#endif

/* This is how a parent application can pass a generic
 * hook function in when setting dm variables.  The dm_hook_data
 * container holds the bu_structparse hook function and data
 * needed by it in the dm_hook and dm_hook_data slots.  When
 * bu_struct_parse or one of the other libbu structure parsing
 * functions is called, the dm_hook_data container is passed
 * in as the data slot in that call.
 *
 * TODO - need example
 *
 */
struct dm_hook_data {
    void(*dm_hook)(const struct bu_structparse *, const char *, void *, const char *, void *);
    void *dmh_data;
};

/* Hide the dm structure behind a typedef */
struct dm;


DM_EXPORT extern struct dm dm_ogl;
DM_EXPORT extern struct dm dm_plot;
DM_EXPORT extern struct dm dm_ps;
DM_EXPORT extern struct dm dm_rtgl;
DM_EXPORT extern struct dm dm_tk;
DM_EXPORT extern struct dm dm_wgl;
DM_EXPORT extern struct dm dm_X;
DM_EXPORT extern struct dm dm_txt;
DM_EXPORT extern struct dm dm_qt;
DM_EXPORT extern struct dm dm_osgl;

DM_EXPORT extern int Dm_Init(void *interp);
DM_EXPORT extern struct dm *dm_open(void *interp,
			     int type,
			     int argc,
			     const char *argv[]);
DM_EXPORT extern void *dm_interp(struct dm *dmp);
DM_EXPORT extern int dm_share_dlist(struct dm *dmp1,
				    struct dm *dmp2);
DM_EXPORT extern fastf_t dm_Xx2Normal(struct dm *dmp,
				      int x);
DM_EXPORT extern int dm_Normal2Xx(struct dm *dmp,
				  fastf_t f);
DM_EXPORT extern fastf_t dm_Xy2Normal(struct dm *dmp,
				      int y,
				      int use_aspect);
DM_EXPORT extern int dm_Normal2Xy(struct dm *dmp,
				  fastf_t f,
				  int use_aspect);
DM_EXPORT extern void dm_fogHint(struct dm *dmp,
				 int fastfog);
DM_EXPORT extern int dm_processOptions(struct dm *dmp, struct bu_vls *init_proc_vls, int argc, char **argv);
DM_EXPORT extern int dm_limit(int i);
DM_EXPORT extern int dm_unlimit(int i);
DM_EXPORT extern fastf_t dm_wrap(fastf_t f);

/* adc.c */
DM_EXPORT extern void dm_draw_adc(struct dm *dmp,
				  struct bview_adc_state *adcp, mat_t view2model, mat_t model2view);

/* axes.c */
DM_EXPORT extern void dm_draw_data_axes(struct dm *dmp,
					fastf_t viewSize,
					struct bview_data_axes_state *bndasp);

DM_EXPORT extern void dm_draw_axes(struct dm *dmp,
				   fastf_t viewSize,
				   const mat_t rmat,
				   struct bview_axes_state *bnasp);

/* clip.c */
DM_EXPORT extern int clip(fastf_t *,
			  fastf_t *,
			  fastf_t *,
			  fastf_t *);
DM_EXPORT extern int vclip(fastf_t *,
			   fastf_t *,
			   fastf_t *,
			   fastf_t *);

/* grid.c */
DM_EXPORT extern void dm_draw_grid(struct dm *dmp,
				   struct bview_grid_state *ggsp,
				   fastf_t scale,
				   mat_t model2view,
				   fastf_t base2local);

/* labels.c */
DM_EXPORT extern int dm_draw_labels(struct dm *dmp,
				    struct rt_wdb *wdbp,
				    const char *name,
				    mat_t viewmat,
				    int *labelsColor,
				    int (*labelsHook)(struct dm *dmp_arg, struct rt_wdb *wdbp_arg,
						      const char *name_arg, mat_t viewmat_arg,
						      int *labelsColor_arg, ClientData labelsHookClientdata_arg),
				    ClientData labelsHookClientdata);

/* rect.c */
DM_EXPORT extern void dm_draw_rect(struct dm *dmp,
				   struct bview_interactive_rect_state *grsp);

/* scale.c */
DM_EXPORT extern void dm_draw_scale(struct dm *dmp,
				    fastf_t viewSize,
				    int *lineColor,
				    int *textColor);

/* vers.c */
DM_EXPORT extern const char *dm_version(void);



/* functions to make a dm struct hideable - will need to
 * sort these out later */

DM_EXPORT extern struct dm *dm_get();
DM_EXPORT extern void dm_put(struct dm *dmp);
DM_EXPORT extern void dm_set_null(struct dm *dmp); /* TODO - HACK, need general set mechanism */
DM_EXPORT extern const char *dm_get_dm_name(struct dm *dmp);
DM_EXPORT extern const char *dm_get_dm_lname(struct dm *dmp);
DM_EXPORT extern int dm_get_width(struct dm *dmp);
DM_EXPORT extern int dm_get_height(struct dm *dmp);
DM_EXPORT extern void dm_set_width(struct dm *dmp, int width);
DM_EXPORT extern void dm_set_height(struct dm *dmp, int height);
DM_EXPORT extern void dm_geometry_request(struct dm *dmp, int width, int height);
DM_EXPORT extern void dm_internal_var(struct bu_vls *result, struct dm *dmp, const char *key); // ick
DM_EXPORT extern fastf_t dm_get_aspect(struct dm *dmp);
DM_EXPORT extern int dm_get_type(struct dm *dmp);
DM_EXPORT extern struct bu_vls *dm_list_types(const char separator); /* free return list with bu_vls_free(list); BU_PUT(list, struct bu_vls); */
DM_EXPORT extern unsigned long dm_get_id(struct dm *dmp);
DM_EXPORT extern void dm_set_id(struct dm *dmp, unsigned long new_id);
DM_EXPORT extern int dm_get_displaylist(struct dm *dmp);
DM_EXPORT extern int dm_close(struct dm *dmp);
DM_EXPORT extern unsigned char *dm_get_bg(struct dm *dmp);
DM_EXPORT extern int dm_set_bg(struct dm *dmp, unsigned char r, unsigned char g, unsigned char b);
DM_EXPORT extern unsigned char *dm_get_fg(struct dm *dmp);
DM_EXPORT extern int dm_set_fg(struct dm *dmp, unsigned char r, unsigned char g, unsigned char b, int strict, fastf_t transparency);
DM_EXPORT extern int dm_reshape(struct dm *dmp, int width, int height);
DM_EXPORT extern int dm_make_current(struct dm *dmp);
DM_EXPORT extern vect_t *dm_get_clipmin(struct dm *dmp);
DM_EXPORT extern vect_t *dm_get_clipmax(struct dm *dmp);
DM_EXPORT extern int dm_get_bound_flag(struct dm *dmp);
DM_EXPORT extern void dm_set_bound(struct dm *dmp, fastf_t val);
DM_EXPORT extern int dm_get_stereo(struct dm *dmp);
DM_EXPORT extern int dm_set_win_bounds(struct dm *dmp, fastf_t *w);
DM_EXPORT extern int dm_configure_win(struct dm *dmp, int force);
DM_EXPORT extern struct bu_vls *dm_get_pathname(struct dm *dmp);
DM_EXPORT extern struct bu_vls *dm_get_dname(struct dm *dmp);
DM_EXPORT extern struct bu_vls *dm_get_tkname(struct dm *dmp);
DM_EXPORT extern int dm_get_fontsize(struct dm *dmp);
DM_EXPORT extern void dm_set_fontsize(struct dm *dmp, int size);
DM_EXPORT extern int dm_get_light_flag(struct dm *dmp);
DM_EXPORT extern void dm_set_light_flag(struct dm *dmp, int size);
DM_EXPORT extern int dm_set_light(struct dm *dmp, int light);
DM_EXPORT extern int dm_get_transparency(struct dm *dmp);
DM_EXPORT extern int dm_set_transparency(struct dm *dmp, int transparency);
DM_EXPORT extern int dm_get_zbuffer(struct dm *dmp);
DM_EXPORT extern int dm_set_zbuffer(struct dm *dmp, int zbuffer);
DM_EXPORT extern int dm_get_linewidth(struct dm *dmp);
DM_EXPORT extern void dm_set_linewidth(struct dm *dmp, int linewidth);
DM_EXPORT extern int dm_get_linestyle(struct dm *dmp);
DM_EXPORT extern void dm_set_linestyle(struct dm *dmp, int linestyle);
DM_EXPORT extern int dm_get_zclip(struct dm *dmp);
DM_EXPORT extern void dm_set_zclip(struct dm *dmp, int zclip);
DM_EXPORT extern int dm_get_perspective(struct dm *dmp);
DM_EXPORT extern void dm_set_perspective(struct dm *dmp, fastf_t perspective);
DM_EXPORT extern int dm_get_display_image(struct dm *dmp, unsigned char **image);
DM_EXPORT extern int dm_gen_dlists(struct dm *dmp, size_t range);
DM_EXPORT extern int dm_begin_dlist(struct dm *dmp, unsigned int list);
DM_EXPORT extern int dm_draw_dlist(struct dm *dmp, unsigned int list);
DM_EXPORT extern int dm_end_dlist(struct dm *dmp);
DM_EXPORT extern int dm_free_dlists(struct dm *dmp, unsigned int list, int range);
DM_EXPORT extern int dm_draw_vlist(struct dm *dmp, struct bn_vlist *vp);
DM_EXPORT extern int dm_draw_vlist_hidden_line(struct dm *dmp, struct bn_vlist *vp);
DM_EXPORT extern int dm_set_line_attr(struct dm *dmp, int width, int style);
DM_EXPORT extern int dm_draw_begin(struct dm *dmp);
DM_EXPORT extern int dm_draw_end(struct dm *dmp);
DM_EXPORT extern int dm_normal(struct dm *dmp);
DM_EXPORT extern int dm_loadmatrix(struct dm *dmp, fastf_t *mat, int eye);
DM_EXPORT extern int dm_loadpmatrix(struct dm *dmp, fastf_t *mat);
DM_EXPORT extern int dm_draw_string_2d(struct dm *dmp, const char *str, fastf_t x,  fastf_t y, int size, int use_aspect);
DM_EXPORT extern int dm_draw_line_2d(struct dm *dmp, fastf_t x1, fastf_t y1_2d, fastf_t x2, fastf_t y2);
DM_EXPORT extern int dm_draw_line_3d(struct dm *dmp, point_t pt1, point_t pt2);
DM_EXPORT extern int dm_draw_lines_3d(struct dm *dmp, int npoints, point_t *points, int sflag);
DM_EXPORT extern int dm_draw_point_2d(struct dm *dmp, fastf_t x, fastf_t y);
DM_EXPORT extern int dm_draw_point_3d(struct dm *dmp, point_t pt);
DM_EXPORT extern int dm_draw_points_3d(struct dm *dmp, int npoints, point_t *points);
DM_EXPORT extern int dm_draw(struct dm *dmp, struct bn_vlist *(*callback)(void *), void **data);
DM_EXPORT extern int dm_draw_obj(struct dm *dmp, struct display_list *obj);
DM_EXPORT extern int dm_set_depth_mask(struct dm *dmp, int d_on);
DM_EXPORT extern int dm_debug(struct dm *dmp, int lvl);
DM_EXPORT extern int dm_logfile(struct dm *dmp, const char *filename);
DM_EXPORT extern struct fb *dm_get_fb(struct dm *dmp);
DM_EXPORT extern int dm_get_fb_visible(struct dm *dmp);
DM_EXPORT extern int dm_set_fb_visible(struct dm *dmp, int is_fb_visible);

/* TODO - dm_vp is supposed to go away, but until we figure it out
 * expose it here to allow dm hiding */
DM_EXPORT extern fastf_t *dm_get_vp(struct dm *dmp);
DM_EXPORT extern void dm_set_vp(struct dm *dmp, fastf_t *vp);

DM_EXPORT extern int dm_set_hook(const struct bu_structparse_map *map,
				 const char *key, void *data, struct dm_hook_data *hook);

DM_EXPORT extern struct bu_structparse *dm_get_vparse(struct dm *dmp);
DM_EXPORT extern void *dm_get_mvars(struct dm *dmp);

DM_EXPORT extern int dm_draw_display_list(struct dm *dmp,
					  struct bu_list *dl,
					  fastf_t transparency_threshold,
					  fastf_t inv_viewsize,
					  short r, short g, short b,
					  int line_width,
					  int draw_style,
					  int draw_edit,
					  unsigned char *gdc,
					  int solids_down,
					  int mv_dlist
					 );

DM_EXPORT extern int dm_default_type();

__END_DECLS

#endif /* DM_H */

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
