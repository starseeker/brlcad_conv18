/*                    D M - G E N E R I C . C
 * BRL-CAD
 *
 * Copyright (c) 1999-2020 United States Government as represented by
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
/** @file libdm/dm-generic.c
 *
 * Generic display manager routines.
 *
 */

#include "common.h"

#include <string.h>

#include "tcl.h"
#ifdef HAVE_TK
#  include "tk.h"
#endif

#include "vmath.h"
#include "dm.h"
#include "rt/solid.h"
#include "./include/private.h"
#include "./null/dm-Null.h"

extern struct dm *plot_open(void *interp, int argc, const char *argv[]);
extern struct dm *ps_open(void *interp, int argc, const char *argv[]);
extern struct dm *txt_open(void *interp, int argc, const char **argv);

#ifdef DM_X
#  if defined(HAVE_TK)
extern struct dm *X_open_dm(void *interp, int argc, const char **argv);
#  endif
#endif /* DM_X */

#ifdef DM_TK
extern struct dm *tk_open_dm(void *interp, int argc, const char **argv);
#endif /* DM_TK */

#ifdef DM_OGL
#  if defined(HAVE_TK)
extern struct dm *ogl_open(void *interp, int argc, const char **argv);
extern void ogl_fogHint(struct dm *dmp, int fastfog);
extern int ogl_share_dlist(struct dm *dmp1, struct dm *dmp2);
#  endif
#endif /* DM_OGL */

#ifdef DM_OSGL
extern struct dm *osgl_open(void *interp, int argc, const char **argv);
extern void osgl_fogHint(struct dm *dmp, int fastfog);
extern int osgl_share_dlist(struct dm *dmp1, struct dm *dmp2);
#endif /* DM_OSGL*/

#ifdef DM_WGL
extern struct dm *wgl_open(void *interp, int argc, const char **argv);
extern void wgl_fogHint(struct dm *dmp, int fastfog);
extern int wgl_share_dlist(struct dm *dmp1, struct dm *dmp2);
#endif /* DM_WGL */

#ifdef DM_QT
extern struct dm *qt_open(void *interp, int argc, const char **argv);
#endif /* DM_QT */

HIDDEN struct dm *
null_dm_open(void *interp, int argc, const char *argv[])
{
    struct dm *dmp = DM_NULL;

    if (argc < 0 || !argv)
	return DM_NULL;

    BU_ALLOC(dmp, struct dm);

    *dmp = dm_null;
    dmp->i->dm_interp = interp;

    return dmp;
}


struct dm *
dm_open(void *interp, const char *type, int argc, const char *argv[])
{
    if (BU_STR_EQUIV(type, "null")) {
	return null_dm_open(interp, argc, argv);
    }
    if (BU_STR_EQUIV(type, "txt")) {
	return txt_open(interp, argc, argv);
    }
    if (BU_STR_EQUIV(type, "plot")) {
	return plot_open(interp, argc, argv);
    }
    if (BU_STR_EQUIV(type, "plot")) {
	return plot_open(interp, argc, argv);
    }
    if (BU_STR_EQUIV(type, "ps")) {
	return ps_open(interp, argc, argv);
    }
    if (BU_STR_EQUIV(type, "plot")) {
	return plot_open(interp, argc, argv);
    }
#if defined(DM_X) && defined(HAVE_TK)
    if (BU_STR_EQUIV(type, "X")) {
	return X_open_dm(interp, argc, argv);
    }
#endif
#if defined(DM_TK)
    if (BU_STR_EQUIV(type, "tk")) {
	return tk_open_dm(interp, argc, argv);
    }
#endif
#if defined(DM_OGL) && defined(HAVE_TK)
    if (BU_STR_EQUIV(type, "ogl")) {
	return ogl_open(interp, argc, argv);
    }
#endif
#if defined(DM_OSGL)
    if (BU_STR_EQUIV(type, "osgl")) {
	return osgl_open(interp, argc, argv);
    }
#endif
#if defined(DM_WGL)
    if (BU_STR_EQUIV(type, "wgl")) {
	return wgl_open(interp, argc, argv);
    }
#endif
#if defined(DM_QT)
    if (BU_STR_EQUIV(type, "qt")) {
	return qt_open(interp, argc, argv);
    }
#endif
    return DM_NULL;
}

void *
dm_interp(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return NULL;
    return (void *)dmp->i->dm_interp;
}

/*
 * Provides a way to (un)share display lists. If dmp2 is
 * NULL, then dmp1 will no longer share its display lists.
 */
int
dm_share_dlist(struct dm *dmp1, struct dm *dmp2)
{
    if (UNLIKELY(!dmp1) || UNLIKELY(!dmp2)) return BRLCAD_ERROR;

    /*
     * Only display managers of the same type and using the
     * same OGL server are allowed to share display lists.
     *
     * XXX - need a better way to check if using the same OGL server.
     */
    if (dmp2 != DM_NULL)
	if (dmp1->i->dm_type != dmp2->i->dm_type ||
	    bu_vls_strcmp(&dmp1->i->dm_dName, &dmp2->i->dm_dName))
	    return BRLCAD_ERROR;

    switch (dmp1->i->dm_type) {
#ifdef DM_OGL
#  if defined(HAVE_TK)
	case DM_TYPE_OGL:
	    return ogl_share_dlist(dmp1, dmp2);
#  endif
#endif
#ifdef DM_WGL
	case DM_TYPE_WGL:
	    return wgl_share_dlist(dmp1, dmp2);
#endif
	default:
	    return BRLCAD_ERROR;
    }
}

struct bu_vls *
dm_list_types(const char separator)
{
    struct bu_vls *list;
    char sep = ' ';
    if (separator) sep = separator;
    BU_GET(list, struct bu_vls);
    bu_vls_init(list);

    bu_vls_trunc(list, 0);

#ifdef DM_OSGL
    if (strlen(bu_vls_addr(list)) > 0) bu_vls_printf(list, "%c", sep);
    bu_vls_printf(list, "osgl");
#endif /* DM_OSGL*/

#ifdef DM_WGL
    if (strlen(bu_vls_addr(list)) > 0) bu_vls_printf(list, "%c", sep);
    bu_vls_printf(list, "wgl");
#endif /* DM_WGL */

#ifdef DM_OGL
    if (strlen(bu_vls_addr(list)) > 0) bu_vls_printf(list, "%c", sep);
    bu_vls_printf(list, "ogl");
#endif /* DM_OGL */

#ifdef DM_QT
    if (strlen(bu_vls_addr(list)) > 0) bu_vls_printf(list, "%c", sep);
    bu_vls_printf(list, "Qt");
#endif /* DM_QT */

#ifdef DM_X
    if (strlen(bu_vls_addr(list)) > 0) bu_vls_printf(list, "%c", sep);
    bu_vls_printf(list, "X");
#endif /* DM_X */

#ifdef DM_X
    if (strlen(bu_vls_addr(list)) > 0) bu_vls_printf(list, "%c", sep);
    bu_vls_printf(list, "tk");
#endif /* DM_X */

    if (strlen(bu_vls_addr(list)) > 0) bu_vls_printf(list, "%c", sep);
    bu_vls_printf(list, "txt");
    bu_vls_printf(list, "%c", sep);
    bu_vls_printf(list, "plot");
    bu_vls_printf(list, "%c", sep);
    bu_vls_printf(list, "ps");
    bu_vls_printf(list, "%c", sep);
    bu_vls_printf(list, "null");
    return list;
}

int
#if !defined(DM_WGL) && !defined(DM_OGL) && !defined(DM_X)
dm_validXType(const char *UNUSED(dpy_string), const char *name)
#else
dm_validXType(const char *dpy_string, const char *name)
#endif
{
    if (BU_STR_EQUAL(name, "wgl")) {
#ifdef DM_WGL
	return 1;
#else
	bu_log("Specified display type [%s] is not available in this compilation.", name);
	return 0;
#endif /* DM_WGL */
    }
    if (BU_STR_EQUAL(name, "ogl")) {
#ifdef DM_OGL
	Display *dpy;
	int return_val;
	if ((dpy = XOpenDisplay(dpy_string)) != NULL) {
	    if (XQueryExtension(dpy, "GLX", &return_val, &return_val, &return_val)) {
		XCloseDisplay(dpy);
		return 1;
	    }
	    XCloseDisplay(dpy);
	}
#else
	bu_log("Specified display type [%s] is not available in this compilation.", name);
#endif /* DM_OGL */
	return 0;
    }

    if (BU_STR_EQUAL(name, "X")) {
#ifdef DM_X
	Display *dpy;
	if ((dpy = XOpenDisplay(dpy_string)) != NULL) {
	    XCloseDisplay(dpy);
	    return 1;
	}
#else
	bu_log("Specified display type [%s] is not available in this compilation.", name);
#endif /* DM_X */
	return 0;
    }

    if (BU_STR_EQUAL(name, "tk")) {
#ifdef DM_TK
	return 1;
#else
	bu_log("Specified display type [%s] is not available in this compilation.", name);
#endif /* DM_TK */
	return 0;
    }

    return 0;
}

/** dm_bestXType determines what mged will normally
  * use as the default display manager
  */

char *
#if !defined(DM_WGL) && !defined(DM_OGL) && !defined(DM_OSGL) && !defined(DM_X)
dm_bestXType(const char *UNUSED(dpy_string))
#else
dm_bestXType(const char *dpy_string)
#endif
{

#ifdef DM_OSGL
    return "osgl";
#endif

#ifdef DM_WGL
    /* should probably make sure wgl works */
    return "wgl";
#endif

#ifdef DM_OGL
    {
	Display *dpy;
	int return_val;
	if (dpy_string) {
	    if ((dpy = XOpenDisplay(dpy_string)) != NULL) {
		if (XQueryExtension(dpy, "GLX", &return_val, &return_val, &return_val)) {
		    XCloseDisplay(dpy);
		    return "ogl";
		}
		XCloseDisplay(dpy);
	    }
	} else {
	    return "ogl";
	}
    }
#endif

#ifdef DM_X
    {
	if (dpy_string) {
	Display *dpy;
	if ((dpy = XOpenDisplay(dpy_string)) != NULL) {
	    XCloseDisplay(dpy);
	    return "X";
	}
	} else {
	    return "X";
	}
    }
#endif

#ifdef DM_TK
    return "tk";
#endif

    return "nu";
}


/**
 * dm_default_type suggests a display manager
 */

const char *
dm_default_type()
{

#ifdef DM_OSGL
    return "osgl";
#endif

#ifdef DM_WGL
    return "wgl";
#endif

#ifdef DM_OGL
    return "ogl";
#endif

#ifdef DM_X
    return "X";
#endif

#ifdef DM_TK
    return "tk";
#endif
    return "null";
}


void
dm_fogHint(struct dm *dmp, int fastfog)
{
    if (UNLIKELY(!dmp)) {
	bu_log("WARNING: NULL display (fastfog => %d)\n", fastfog);
	return;
    }

    switch (dmp->i->dm_type) {
#ifdef DM_OGL
#  if defined(HAVE_TK)
	case DM_TYPE_OGL:
	    ogl_fogHint(dmp, fastfog);
	    return;
#  endif
#endif
#ifdef DM_WGL
	case DM_TYPE_WGL:
	    wgl_fogHint(dmp, fastfog);
	    return;
#endif
	default:
	    return;
    }
}

int
dm_write_image(struct bu_vls *msgs, FILE *fp, struct dm *dmp)
{
    if (!dmp || !dmp->i->dm_write_image) return -1;
    return dmp->i->dm_write_image(msgs, fp, dmp);
}

/* Properly generic functions */

void
dm_geometry_request(struct dm *dmp, int width, int height)
{
    if (!dmp || !dmp->i->dm_geometry_request) return;
    (void)dmp->i->dm_geometry_request(dmp, width, height);
}

void
dm_internal_var(struct bu_vls *result, struct dm *dmp, const char *key)
{
    if (!dmp || !result || !dmp->i->dm_internal_var) return;
    (void)dmp->i->dm_internal_var(result, dmp, key);
}

int
dm_event_cmp(struct dm *dmp, dm_event_t type, int event)
{
    if (!dmp || !dmp->i->dm_event_cmp) return -1;
    return dmp->i->dm_event_cmp(dmp, type, event);
}

fastf_t
dm_Xx2Normal(struct dm *dmp, int x)
{
    if (UNLIKELY(!dmp)) return 0.0;
    return ((x / (fastf_t)dmp->i->dm_width - 0.5) * 2.0);
}

int
dm_Normal2Xx(struct dm *dmp, fastf_t f)
{
    if (UNLIKELY(!dmp)) return 0.0;
    return (f * 0.5 + 0.5) * dmp->i->dm_width;
}

fastf_t
dm_Xy2Normal(struct dm *dmp, int y, int use_aspect)
{
    if (UNLIKELY(!dmp)) return 0.0;
    if (use_aspect)
	return ((0.5 - y / (fastf_t)dmp->i->dm_height) / dmp->i->dm_aspect * 2.0);
    else
	return ((0.5 - y / (fastf_t)dmp->i->dm_height) * 2.0);
}

int
dm_Normal2Xy(struct dm *dmp, fastf_t f, int use_aspect)
{
    if (UNLIKELY(!dmp)) return 0.0;
    if (use_aspect)
	return (0.5 - f * 0.5 * dmp->i->dm_aspect) * dmp->i->dm_height;
    else
	return (0.5 - f * 0.5) * dmp->i->dm_height;
}


struct dm *
dm_get()
{
    struct dm *new_dm = DM_NULL;
    BU_GET(new_dm, struct dm);
    BU_GET(new_dm->i, struct dm_impl);
    bu_vls_init(&new_dm->i->dm_pathName);
    bu_vls_init(&new_dm->i->dm_dName);

    return new_dm;
}

void
dm_put(struct dm *dmp)
{
    if (dmp && dmp != DM_NULL) {
	bu_vls_free(&dmp->i->dm_pathName);
	bu_vls_free(&dmp->i->dm_dName);
	if (dmp->i->fbp) fb_put(dmp->i->fbp);
	if (dmp->i->dm_put_internal)
	    dmp->i->dm_put_internal(dmp);
	BU_PUT(dmp->i, struct dm_impl);
	BU_PUT(dmp, struct dm);
    }
}

void
dm_set_null(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return;
    *dmp = dm_null;
}

struct fb *
dm_get_fb(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return NULL;
    if (dmp->i->fbp == FB_NULL)
	dmp->i->dm_openFb(dmp);
    return dmp->i->fbp;
}

const char *
dm_get_dm_name(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return NULL;
    return dmp->i->dm_name;
}

const char *
dm_get_dm_lname(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return NULL;
    return dmp->i->dm_lname;
}

int
dm_get_width(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_width;
}

int
dm_get_height(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_height;
}

void
dm_set_width(struct dm *dmp, int width)
{
    if (UNLIKELY(!dmp)) return;
    dmp->i->dm_width = width;
}

void
dm_set_height(struct dm *dmp, int height)
{
    if (UNLIKELY(!dmp)) return;
    dmp->i->dm_height = height;
}


int
dm_get_type(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_type;
}

int
dm_get_displaylist(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_displaylist;
}

fastf_t
dm_get_aspect(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_aspect;
}

int
dm_get_fontsize(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_fontsize;
}

void
dm_set_fontsize(struct dm *dmp, int size)
{
    if (UNLIKELY(!dmp)) return;
    dmp->i->dm_fontsize = size;
}

int
dm_close(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_close(dmp);
}

unsigned char *
dm_get_bg(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return NULL;
    return dmp->i->dm_bg;
}

int
dm_set_bg(struct dm *dmp, unsigned char r, unsigned char g, unsigned char b)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_setBGColor(dmp, r, g, b);
}

unsigned char *
dm_get_fg(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return NULL;
    return dmp->i->dm_fg;
}

int
dm_set_fg(struct dm *dmp, unsigned char r, unsigned char g, unsigned char b, int strict, fastf_t transparency)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_setFGColor(dmp, r, g, b, strict, transparency);
}

int
dm_reshape(struct dm *dmp, int width, int height)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_reshape(dmp, width, height);
}

int
dm_make_current(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_makeCurrent(dmp);
}

vect_t *
dm_get_clipmin(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return  &(dmp->i->dm_clipmin);
}


vect_t *
dm_get_clipmax(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return  &(dmp->i->dm_clipmax);
}

int
dm_get_bound_flag(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_boundFlag;
}

void
dm_set_bound(struct dm *dmp, fastf_t val)
{
    if (UNLIKELY(!dmp)) return;
    dmp->i->dm_bound = val;
}

int
dm_set_win_bounds(struct dm *dmp, fastf_t *w)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_setWinBounds(dmp, w);
}

int
dm_get_stereo(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_stereo;
}
int
dm_configure_win(struct dm *dmp, int force)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_configureWin(dmp, force);
}

struct bu_vls *
dm_get_pathname(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return NULL;
    return &(dmp->i->dm_pathName);
}


struct bu_vls *
dm_get_dname(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return NULL;
    return &(dmp->i->dm_dName);
}

struct bu_vls *
dm_get_tkname(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return NULL;
    return &(dmp->i->dm_tkName);
}

unsigned long
dm_get_id(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_id;
}

void
dm_set_id(struct dm *dmp, unsigned long new_id)
{
    if (UNLIKELY(!dmp)) return;
    dmp->i->dm_id = new_id;
}

int
dm_get_light(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_light;
}

int
dm_set_light(struct dm *dmp, int light)
{
    if (UNLIKELY(!dmp)) return 0;
    if (dmp->i->dm_setLight) {
	return dmp->i->dm_setLight(dmp, light);
    } else {
	dmp->i->dm_light = light;
    }
    return dmp->i->dm_light;
}

int
dm_get_transparency(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_transparency;
}

int
dm_set_transparency(struct dm *dmp, int transparency)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_setTransparency(dmp, transparency);
}

int
dm_get_zbuffer(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_zbuffer;
}

int
dm_set_zbuffer(struct dm *dmp, int zbuffer)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_setZBuffer(dmp, zbuffer);
}

int
dm_get_linewidth(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_lineWidth;
}

void
dm_set_linewidth(struct dm *dmp, int linewidth)
{
    if (UNLIKELY(!dmp)) return;
    dmp->i->dm_lineWidth = linewidth;
}

int
dm_get_linestyle(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_lineStyle;
}

void
dm_set_linestyle(struct dm *dmp, int linestyle)
{
    if (UNLIKELY(!dmp)) return;
    dmp->i->dm_lineStyle = linestyle;
}

int
dm_set_line_attr(struct dm *dmp, int width, int style)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_setLineAttr(dmp, width, style);
}


int
dm_get_zclip(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_zclip;
}

void
dm_set_zclip(struct dm *dmp, int zclip)
{
    if (UNLIKELY(!dmp)) return;
    dmp->i->dm_zclip = zclip;
}

int
dm_get_perspective(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_perspective;
}

void
dm_set_perspective(struct dm *dmp, fastf_t perspective)
{
    if (UNLIKELY(!dmp)) return;
    dmp->i->dm_perspective = perspective;
}

int
dm_get_display_image(struct dm *dmp, unsigned char **image)
{
    if (!dmp || !image) return 0;
    return dmp->i->dm_getDisplayImage(dmp, image);
}

int
dm_gen_dlists(struct dm *dmp, size_t range)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_genDLists(dmp, range);
}

int
dm_begin_dlist(struct dm *dmp, unsigned int list)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_beginDList(dmp, list);
}
int
dm_draw_dlist(struct dm *dmp, unsigned int list)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_drawDList(list);
}
int
dm_end_dlist(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_endDList(dmp);
}
int
dm_free_dlists(struct dm *dmp, unsigned int list, int range)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_freeDLists(dmp, list, range);
}

int
dm_draw_vlist(struct dm *dmp, struct bn_vlist *vp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_drawVList(dmp, vp);
}

int
dm_draw_vlist_hidden_line(struct dm *dmp, struct bn_vlist *vp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_drawVListHiddenLine(dmp, vp);
}
int
dm_draw_begin(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_drawBegin(dmp);
}
int
dm_draw_end(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_drawEnd(dmp);
}
int
dm_normal(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_normal(dmp);
}
int
dm_loadmatrix(struct dm *dmp, fastf_t *mat, int eye)
{
    if (!dmp || !mat) return 0;
    return dmp->i->dm_loadMatrix(dmp, mat, eye);
}
int
dm_loadpmatrix(struct dm *dmp, fastf_t *mat)
{
    if (!dmp || !mat) return 0;
    return dmp->i->dm_loadPMatrix(dmp, mat);
}
int
dm_draw_string_2d(struct dm *dmp, const char *str, fastf_t x,  fastf_t y, int size, int use_aspect)
{
    if (!dmp || !str) return 0;
    return dmp->i->dm_drawString2D(dmp, str, x, y, size, use_aspect);
}
int
dm_draw_line_2d(struct dm *dmp, fastf_t x1, fastf_t y1_2d, fastf_t x2, fastf_t y2)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_drawLine2D(dmp, x1, y1_2d, x2, y2);
}
int
dm_draw_line_3d(struct dm *dmp, point_t pt1, point_t pt2)
{
    if (UNLIKELY(!dmp)) return 0;
    if (!!pt1 || !pt2) return 0;
    return dmp->i->dm_drawLine3D(dmp, pt1, pt2);
}
int
dm_draw_lines_3d(struct dm *dmp, int npoints, point_t *points, int sflag)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_drawLines3D(dmp, npoints, points, sflag);
}
int
dm_draw_point_2d(struct dm *dmp, fastf_t x, fastf_t y)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_drawPoint2D(dmp, x, y);
}
int
dm_draw_point_3d(struct dm *dmp, point_t pt)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_drawPoint3D(dmp, pt);
}
int
dm_draw_points_3d(struct dm *dmp, int npoints, point_t *points)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_drawPoints3D(dmp, npoints, points);
}
int
dm_draw(struct dm *dmp, struct bn_vlist *(*callback)(void *), void **data)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_draw(dmp, callback, data);
}
int
dm_draw_obj(struct dm *dmp, struct display_list *obj)
{
    if (!dmp || !obj) return 0;
    return dmp->i->dm_draw_obj(dmp, obj);
}
int
dm_set_depth_mask(struct dm *dmp, int d_on)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_setDepthMask(dmp, d_on);
}
int
dm_debug(struct dm *dmp, int lvl)
{
    if (UNLIKELY(!dmp)) return 0;
    return dmp->i->dm_debug(dmp, lvl);
}
int
dm_logfile(struct dm *dmp, const char *filename)
{
    if (!dmp || !filename) return 0;
    return dmp->i->dm_logfile(dmp, filename);
}

fastf_t *
dm_get_vp(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return NULL;
    return dmp->i->dm_vp;
}

void
dm_set_vp(struct dm *dmp, fastf_t *vp)
{
    if (UNLIKELY(!dmp)) return;
    dmp->i->dm_vp = vp;
}

/* This is the generic "catch-all" hook that is used
 * to run any user supplied callbacks.  If more side
 * effects are needed at the libdm level, a task-specific
 * hook should be defined using this function as a
 * starting point and adding in whatever additional
 * logic is needed.  The full dm structure should always
 * be accessible as a slot in the modifiable variables
 * structure passed in here as "base" */
void
dm_generic_hook(const struct bu_structparse *sdp,
		const char *name, void *base, const char *value, void *data)
{
    if (data) {
	struct dm_hook_data *hook= (struct dm_hook_data *)data;

	/* Call hook function(if it exists) to carry out the
	 * application requested logic */
	if (hook && hook->dm_hook)
	    hook->dm_hook(sdp, name, base, value, hook->dmh_data);
    }
}

int
dm_set_hook(const struct bu_structparse_map *map,
	    const char *key, void *data, struct dm_hook_data *hook)
{
    if (UNLIKELY(!map || !key || !hook)) return -1;
    hook->dm_hook = BU_STRUCTPARSE_FUNC_NULL;
    hook->dmh_data = NULL;
    for (; map->sp_name != (char *)0; map++) {
	if (BU_STR_EQUAL(map->sp_name, key)) {
	    hook->dm_hook = map->sp_hook;
	    if (data)
		hook->dmh_data = data;
	    return 0;
	}
    }
    return 1;
}

struct bu_structparse *
dm_get_vparse(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return NULL;
    return dmp->i->vparse;
}

void *
dm_get_mvars(struct dm *dmp)
{
    if (UNLIKELY(!dmp)) return NULL;
    if (!dmp->i->m_vars) return (void *)dmp;
    return dmp->i->m_vars;
}


/* Routines for drawing based on a list of display_list
 * structures.  This will probably need to be a struct dm
 * entry to allow it to be customized for various dm
 * backends, but as a first step get it out of MGED
 * and into libdm. */
static int
dm_drawSolid(struct dm *dmp,
	     struct solid *sp,
	     short r,
	     short g,
	     short b,
	     int draw_style,
	     unsigned char *gdc)
{
    int ndrawn = 0;

    if (sp->s_cflag) {
	if (!DM_SAME_COLOR(r, g, b, (short)gdc[0], (short)gdc[1], (short)gdc[2])) {
	    dm_set_fg(dmp, (short)gdc[0], (short)gdc[1], (short)gdc[2], 0, sp->s_transparency);
	    DM_COPY_COLOR(r, g, b, (short)gdc[0], (short)gdc[1], (short)gdc[2]);
	}
    } else {
	if (!DM_SAME_COLOR(r, g, b, (short)sp->s_color[0], (short)sp->s_color[1], (short)sp->s_color[2])) {
	    dm_set_fg(dmp, (short)sp->s_color[0], (short)sp->s_color[1], (short)sp->s_color[2], 0, sp->s_transparency);
	    DM_COPY_COLOR(r, g, b, (short)sp->s_color[0], (short)sp->s_color[1], (short)sp->s_color[2]);
	}
    }

    if (dm_get_displaylist(dmp) && draw_style) {
	dm_draw_dlist(dmp, sp->s_dlist);
	sp->s_flag = UP;
	ndrawn++;
    } else {
	if (dm_draw_vlist(dmp, (struct bn_vlist *)&sp->s_vlist) == BRLCAD_OK) {
	    sp->s_flag = UP;
	    ndrawn++;
	}
    }

    return ndrawn;
}


int
dm_draw_display_list(struct dm *dmp,
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
		    )
{
    struct display_list *gdlp;
    struct display_list *next_gdlp;
    struct solid *sp;
    fastf_t ratio;
    int ndrawn = 0;

    gdlp = BU_LIST_NEXT(display_list, dl);
    while (BU_LIST_NOT_HEAD(gdlp, dl)) {
	next_gdlp = BU_LIST_PNEXT(display_list, gdlp);

	FOR_ALL_SOLIDS(sp, &gdlp->dl_headSolid) {
	    if (solids_down) sp->s_flag = DOWN;              /* Not drawn yet */

	    /* If part of object edit, will be drawn below */
	    if ((sp->s_iflag == UP && !draw_edit) || (sp->s_iflag != UP && draw_edit))
		continue;

	    if (!((sp->s_transparency > transparency_threshold) || (EQUAL(sp->s_transparency, transparency_threshold))))
		continue;

	    if (dm_get_bound_flag(dmp)) {
		ratio = sp->s_size * inv_viewsize;

		/*
		 * Check for this object being bigger than
		 * dmp->i->dm_bound * the window size, or smaller than a speck.
		 */
		if (ratio < 0.001)
		    continue;
	    }

	    dm_set_line_attr(dmp, line_width, sp->s_soldash);

	    if (!draw_edit) {
		ndrawn += dm_drawSolid(dmp, sp, r, g, b, draw_style, gdc);
	    } else {
		if (dm_get_displaylist(dmp) && mv_dlist) {
		    dm_draw_dlist(dmp, sp->s_dlist);
		    sp->s_flag = UP;
		    ndrawn++;
		} else {
		    /* draw in immediate mode */
		    if (dm_draw_vlist(dmp, (struct bn_vlist *)&sp->s_vlist) == BRLCAD_OK) {
			sp->s_flag = UP;
			ndrawn++;
		    }
		}
	    }
	}

	gdlp = next_gdlp;
    }

    return ndrawn;
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
