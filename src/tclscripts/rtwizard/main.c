/*                        M A I N . C
 * BRL-CAD
 *
 * Copyright (c) 1986-2016 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file rtwizard/main.c
 *
 */

#include "common.h"
#include "string.h"

#include "tcl.h"

#include "vmath.h"
#include "bu/color.h"
#include "bu/file.h"
#include "bu/malloc.h"
#include "bu/log.h"
#include "bu/ptbl.h"
#include "bu/opt.h"
#include "bu/str.h"
#include "tclcad.h"

/* avoid including itcl.h due to their usage of internal headers */
extern int Itcl_Init(Tcl_Interp *);

#define RTWIZARD_HAVE_GUI 0

#define RTWIZARD_SIZE_DEFAULT 512

struct rtwizard_settings {
    int use_gui;
    int no_gui;
    int verbose;

    struct bu_ptbl *color;
    struct bu_ptbl *ghost;
    struct bu_ptbl *line;

    struct bu_vls *input_file;
    struct bu_vls *output_file;
    struct bu_vls *fb_dev;
    int port;
    struct bu_vls *log_file;
    struct bu_vls *pid_file;

    size_t width;
    int width_set;
    size_t height;
    int height_set;
    size_t size; /* Assumes square - width and height - overridden by width and height */
    int size_set;

    struct bu_color *bkg_color;
    struct bu_color *line_color;
    struct bu_color *non_line_color;

    double ghosting_intensity;
    int occlusion;
    int benchmark;
    int cpus;

    /* View model */
    double viewsize;
    quat_t orientation;
    vect_t eye_pt;

    /* View settings */
    double az, el, tw;
    double perspective;
    double zoom;
    vect_t center;

};

struct rtwizard_settings * rtwizard_settings_create() {
    struct rtwizard_settings *s;
    fastf_t white[3] = {255.0, 255.0, 255.0};
    fastf_t black[3] = {0.0, 0.0, 0.0};
    BU_GET(s, struct rtwizard_settings);
    BU_GET(s->color, struct bu_ptbl);
    BU_GET(s->ghost, struct bu_ptbl);
    BU_GET(s->line,  struct bu_ptbl);
    bu_ptbl_init(s->color, 8, "color init");
    bu_ptbl_init(s->ghost, 8, "ghost init");
    bu_ptbl_init(s->line, 8, "line init");

    BU_GET(s->input_file, struct bu_vls);
    bu_vls_init(s->input_file);
    BU_GET(s->output_file, struct bu_vls);
    bu_vls_init(s->output_file);
    BU_GET(s->fb_dev, struct bu_vls);
    bu_vls_init(s->fb_dev);
    BU_GET(s->log_file, struct bu_vls);
    bu_vls_init(s->log_file);
    BU_GET(s->pid_file, struct bu_vls);
    bu_vls_init(s->pid_file);

    BU_GET(s->bkg_color, struct bu_color);
    (void)bu_color_from_rgb_floats(s->bkg_color, white);
    BU_GET(s->line_color, struct bu_color);
    (void)bu_color_from_rgb_floats(s->line_color, black);
    BU_GET(s->non_line_color, struct bu_color);
    (void)bu_color_from_rgb_floats(s->non_line_color, black);
    s->benchmark = 0;
    s->port = -1;
    s->cpus = 0;

    s->az = DBL_MAX;
    s->el = DBL_MAX;
    s->tw = DBL_MAX;
    s->perspective = DBL_MAX;
    s->zoom = DBL_MAX;
    VSETALL(s->center, DBL_MAX);

    s->viewsize = DBL_MAX;
    s->orientation[0] = DBL_MAX;
    s->orientation[1] = DBL_MAX;
    s->orientation[2] = DBL_MAX;
    s->orientation[3] = DBL_MAX;
    VSETALL(s->eye_pt, DBL_MAX);

    s->occlusion = 1;
    s->ghosting_intensity = 12.0;
    s->width = RTWIZARD_SIZE_DEFAULT;
    s->width_set = 0;
    s->height = RTWIZARD_SIZE_DEFAULT;
    s->height_set = 0;
    s->size = RTWIZARD_SIZE_DEFAULT;
    s->size_set = 0;

    s->use_gui = 0;
    s->no_gui = 0;

    s->verbose = 0;
    return s;
}

void rtwizard_settings_destroy(struct rtwizard_settings *s) {
    bu_ptbl_free(s->color);
    bu_ptbl_free(s->ghost);
    bu_ptbl_free(s->line);
    /* TODO - loop over and free table contents */
    BU_PUT(s->color, struct bu_ptbl);
    BU_PUT(s->ghost, struct bu_ptbl);
    BU_PUT(s->line,  struct bu_ptbl);

    BU_PUT(s->bkg_color, struct bu_color);
    BU_PUT(s->line_color, struct bu_color);
    BU_PUT(s->non_line_color, struct bu_color);

    bu_vls_free(s->fb_dev);
    BU_PUT(s->fb_dev, struct bu_vls);
    bu_vls_free(s->input_file);
    BU_PUT(s->input_file, struct bu_vls);
    bu_vls_free(s->output_file);
    BU_PUT(s->output_file, struct bu_vls);
    bu_vls_free(s->log_file);
    BU_PUT(s->log_file, struct bu_vls);
    bu_vls_free(s->pid_file);
    BU_PUT(s->pid_file, struct bu_vls);


    BU_PUT(s, struct rtwizard_settings);
}

#define RTW_TERR_MSG(_t,_name,_opt) "Error: picture type _t specified, but no _name objects listed.\nPlease specify _name objects using the _opt option\n"

int rtwizard_info_sufficient(struct bu_vls *msg, struct rtwizard_settings *s, char type)
{
    int ret = 1;
    if (!bu_vls_strlen(s->input_file)) {
	bu_vls_printf(msg, "Error: No input Geometry Database (.g) file specified.\n");
	ret = 0;
    }
    switch (type) {
	case 'A':
	    if (BU_PTBL_LEN(s->color) == 0) {
		bu_vls_printf(msg, "%s", RTW_TERR_MSG(type, "color", "-c"));
		ret = 0;
	    }
	    break;
	case 'B':
	    break;
	case 'C':
	    break;
	case 'D':
	    break;
	case 'E':
	    break;
	default:
	    /* If we don't have a type, make sure we've got *some* object in at
	     * least one of the object lists */
	    break;
    }

    return ret;
}


int
opt_width(struct bu_vls *msg, int argc, const char **argv, void *settings)
{
    struct rtwizard_settings *s = (struct rtwizard_settings *)settings;
    int ret = bu_opt_int(msg, argc, argv, (void *)&s->width);
    if (ret != -1) s->width_set = 1;
    return ret;
}


int
opt_height(struct bu_vls *msg, int argc, const char **argv, void *settings)
{
    struct rtwizard_settings *s = (struct rtwizard_settings *)settings;
    int ret = bu_opt_int(msg, argc, argv, (void *)&s->height);
    if (ret != -1) s->height_set = 1;
    return ret;
}

int
opt_size(struct bu_vls *msg, int argc, const char **argv, void *settings)
{
    struct rtwizard_settings *s = (struct rtwizard_settings *)settings;
    int ret = bu_opt_int(msg, argc, argv, (void *)&s->size);
    if (ret != -1) {
	s->size_set = 1;
	if (!s->width_set) s->width = s->size;
	if (!s->height_set) s->height = s->size;
    }
    return ret;
}

int
opt_objs(struct bu_vls *msg, int argc, const char **argv, void *obj_tbl)
{
    /* argv[0] should be either an object or a list. */
    int i = 0;
    char *objs = NULL;
    int acnum = 0;
    char **avnum;
    struct bu_ptbl *t = (struct bu_ptbl *)obj_tbl;

    BU_OPT_CHECK_ARGV0(msg, argc, argv, "opt_objs");

    objs = bu_strdup(argv[0]);

    while (objs[i]) {
	/* If we have a separator or a quote, replace with a space */
	if (objs[i] == ',' || objs[i] == ';' || objs[i] == '\'' || objs[i] == '\"' ) {
	    if (i == 0) objs[i] = ' ';
	    if (objs[i-1] != '\\') objs[i] = ' ';
	}
	i++;
    }

    avnum = (char **)bu_calloc(strlen(objs), sizeof(char *), "breakout array");
    acnum = bu_argv_from_string(avnum, strlen(objs), objs);

    /* TODO - use quote/unquote routines to scrub names... */

    for (i = 0; i < acnum; i++) {
	bu_ptbl_ins(t, (long *)bu_strdup(avnum[i]));
    }
    bu_free(objs, "string dup");
    bu_free(avnum, "array memory");

    return (acnum > 0) ? 1 : -1;
}

int
opt_letter(struct bu_vls *msg, int argc, const char **argv, void *l)
{
    char *letter = (char *)l;
    BU_OPT_CHECK_ARGV0(msg, argc, argv, "bu_opt_int");

    if (strlen(argv[0]) != 1) {
	if (msg) bu_vls_printf(msg, "Invalid letter specifier for rtwizard type: %s\n", argv[0]);
	return -1;
    }

    if (argv[0][0] != 'A' && argv[0][0] != 'B' && argv[0][0] != 'C' && argv[0][0] != 'D' && argv[0][0] != 'E') {
	if (msg) bu_vls_printf(msg, "Invalid letter specifier for rtwizard type: %c\n", argv[0][0]);
	return -1;
    }

    (*letter) = argv[0][0];

    return 1;
}


int
opt_quat(struct bu_vls *msg, int argc, const char **argv, void *inq)
{
    int i = 0;
    int acnum = 0;
    char *str1 = NULL;
    char *avnum[5] = {NULL, NULL, NULL, NULL, NULL};

    quat_t *q = (quat_t *)inq;
    BU_OPT_CHECK_ARGV0(msg, argc, argv, "bu_opt_int");


    /* First, see if the first string converts to a quat_t*/
    str1 = bu_strdup(argv[0]);
    while (str1[i]) {
	/* If we have a separator, replace with a space */
	if (str1[i] == ',' || str1[i] == '/') str1[i] = ' ';
	i++;
    }
    acnum = bu_argv_from_string(avnum, 5, str1);
    if (acnum == 4) {
	/* We might have four numbers - find out */
	fastf_t q1, q2, q3, q4;
	int have_four = 1;
	if (bu_opt_fastf_t(msg, 1, (const char **)&avnum[0], &q1) == -1) {
	    if (msg) bu_vls_sprintf(msg, "Not a number: %s.\n", avnum[0]);
	    have_four = 0;
	}
	if (bu_opt_fastf_t(msg, 1, (const char **)&avnum[1], &q2) == -1) {
	    if (msg) bu_vls_sprintf(msg, "Not a number: %s.\n", avnum[1]);
	    have_four = 0;
	}
	if (bu_opt_fastf_t(msg, 1, (const char **)&avnum[2], &q3) == -1) {
	    if (msg) bu_vls_sprintf(msg, "Not a number: %s.\n", avnum[2]);
	    have_four = 0;
	}
	if (bu_opt_fastf_t(msg, 1, (const char **)&avnum[3], &q4) == -1) {
	    if (msg) bu_vls_sprintf(msg, "Not a number: %s.\n", avnum[3]);
	    have_four = 0;
	}
	bu_free(str1, "free tmp str");
	/* If we got here, we do have four numbers */
	if (have_four) {
	    (*q)[0] = q1;
	    (*q)[1] = q2;
	    (*q)[2] = q3;
	    (*q)[3] = q4;
	    return 1;
	}
    } else {
	/* Can't be just the first arg */
	bu_free(str1, "free tmp str");
    }
    /* First string didn't have the numbers - maybe we have 4 args ? */
    if (argc >= 4) {
	/* We might have four numbers - find out */
	fastf_t q1, q2, q3, q4;
	if (bu_opt_fastf_t(msg, 1, &argv[0], &q1) == -1) {
	    if (msg) bu_vls_sprintf(msg, "Not a number: %s.\n", argv[0]);
	    return -1;
	}
	if (bu_opt_fastf_t(msg, 1, &argv[1], &q2) == -1) {
	    if (msg) bu_vls_sprintf(msg, "Not a number: %s.\n", argv[1]);
	    return -1;
	}
	if (bu_opt_fastf_t(msg, 1, &argv[2], &q3) == -1) {
	    if (msg) bu_vls_sprintf(msg, "Not a number: %s.\n", argv[2]);
	    return -1;
	}
	if (bu_opt_fastf_t(msg, 1, &argv[3], &q4) == -1) {
	    if (msg) bu_vls_sprintf(msg, "Not a number: %s.\n", argv[3]);
	    return -1;
	}
	(*q)[0] = q1;
	(*q)[1] = q2;
	(*q)[2] = q3;
	(*q)[3] = q4;
	return 1;
    } else {
	if (msg) bu_vls_sprintf(msg, "No valid quaternion found: %s\n", argv[0]);
	return -1;
    }
}

void print_rtwizard_state(struct rtwizard_settings *s) {
    size_t i = 0;
    struct bu_vls slog = BU_VLS_INIT_ZERO;

    bu_vls_printf(&slog, "use_gui: %d\n", s->use_gui);
    bu_vls_printf(&slog, "no_gui: %d\n", s->no_gui);
    bu_vls_printf(&slog, "verbose: %d\n", s->verbose);


    bu_vls_printf(&slog, "color objs:");
    for (i = 0; i < BU_PTBL_LEN(s->color); i++) {
	bu_vls_printf(&slog, " %s", (const char *)BU_PTBL_GET(s->color, i));
    }
    bu_vls_printf(&slog, "\nghost objs:");
    for (i = 0; i < BU_PTBL_LEN(s->ghost); i++) {
	bu_vls_printf(&slog, " %s", (const char *)BU_PTBL_GET(s->ghost, i));
    }
    bu_vls_printf(&slog, "\nline objs:");
    for (i = 0; i < BU_PTBL_LEN(s->line); i++) {
	bu_vls_printf(&slog, " %s", (const char *)BU_PTBL_GET(s->line, i));
    }
    bu_vls_printf(&slog, "\n\n");

    bu_vls_printf(&slog, "input_file: %s\n", bu_vls_addr(s->input_file));
    bu_vls_printf(&slog, "output_file: %s\n", bu_vls_addr(s->output_file));
    bu_vls_printf(&slog, "fb_dev: %s\n", bu_vls_addr(s->fb_dev));
    bu_vls_printf(&slog, "port: %d\n", s->port);
    bu_vls_printf(&slog, "log_file: %s\n", bu_vls_addr(s->log_file));
    bu_vls_printf(&slog, "pid_file: %s\n", bu_vls_addr(s->pid_file));

    bu_vls_printf(&slog, "width(%d): %d\n", s->width_set, s->width);
    bu_vls_printf(&slog, "height(%d): %d\n", s->height_set, s->height);
    bu_vls_printf(&slog, "size(%d): %d\n", s->size_set, s->size);
    bu_vls_printf(&slog, "bkg_color: %d,%d,%d\n", (int)s->bkg_color->buc_rgb[0], (int)s->bkg_color->buc_rgb[1], (int)s->bkg_color->buc_rgb[2]);
    bu_vls_printf(&slog, "line_color: %d,%d,%d\n", (int)s->line_color->buc_rgb[0], (int)s->line_color->buc_rgb[1], (int)s->line_color->buc_rgb[2]);
    bu_vls_printf(&slog, "non_line_color: %d,%d,%d\n", (int)s->non_line_color->buc_rgb[0], (int)s->non_line_color->buc_rgb[1], (int)s->non_line_color->buc_rgb[2]);

    bu_vls_printf(&slog, "\nghosting intensity: %f\n", s->ghosting_intensity);
    bu_vls_printf(&slog, "occlusion: %d\n", s->occlusion);
    bu_vls_printf(&slog, "benchmark: %d\n", s->benchmark);
    bu_vls_printf(&slog, "cpus: %d\n", s->cpus);

    bu_vls_printf(&slog, "\nviewsize: %f\n", s->viewsize);
    bu_vls_printf(&slog, "quat: %f,%f,%f,%f\n", s->orientation[0], s->orientation[1], s->orientation[2], s->orientation[3]);
    bu_vls_printf(&slog, "eye_pt: %f,%f,%f\n", s->eye_pt[0], s->eye_pt[1], s->eye_pt[2]);

    bu_vls_printf(&slog, "\naz,el,tw: %f,%f,%f\n", s->az, s->el, s->tw);
    bu_vls_printf(&slog, "perspective: %f\n", s->perspective);
    bu_vls_printf(&slog, "zoom: %f\n", s->zoom);
    bu_vls_printf(&slog, "center: %f,%f,%f\n", s->center[0], s->center[1], s->center[2]);

    bu_log("%s", bu_vls_addr(&slog));
    bu_vls_free(&slog);
}

int rtwizard_imgformat_supported(int fmt) {
    if (fmt == BU_MIME_IMAGE_DPIX) return 1;
    if (fmt == BU_MIME_IMAGE_PIX) return 1;
    if (fmt == BU_MIME_IMAGE_PNG) return 1;
    if (fmt == BU_MIME_IMAGE_PPM) return 1;
    if (fmt == BU_MIME_IMAGE_BW) return 1;
    return 0;
}

void
Init_RtWizard_Vars(Tcl_Interp *interp, struct rtwizard_settings *s)
{
    size_t i = 0;
    struct bu_vls tcl_cmd = BU_VLS_INIT_ZERO;

    (void)Tcl_Eval(interp, "namespace eval RtWizard{}");

    if (s->use_gui) {
	bu_vls_sprintf(&tcl_cmd, "set ::use_gui 1");
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (s->no_gui) {
	bu_vls_sprintf(&tcl_cmd, "set ::disable_gui 1");
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (s->verbose) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(verbose) %d", s->verbose);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (bu_vls_strlen(s->input_file)) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(dbFile) %s", bu_vls_addr(s->input_file));
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (bu_vls_strlen(s->output_file)) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(output_filename) %s", bu_vls_addr(s->output_file));
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (bu_vls_strlen(s->fb_dev)) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(fbserv_device) %s", bu_vls_addr(s->fb_dev));
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (s->port > -1) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(fbserv_port) %d", s->port);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (s->width_set) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(width) %d", s->width);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (s->height_set) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(scanlines) %d", s->height);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (s->size_set) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(size) %d", s->size);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (BU_PTBL_LEN(s->color) > 0) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(color_objlist) {}");
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
	for (i = 0; i < BU_PTBL_LEN(s->color); i++) {
	    const char *obj = (const char *)BU_PTBL_GET(s->color, i);
	    bu_vls_sprintf(&tcl_cmd, "lappend ::RtWizard::wizard_state(color_objlist) %s", obj);
	    (void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
	}
    }

    if (BU_PTBL_LEN(s->ghost) > 0) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(ghost_objlist) {}");
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
	for (i = 0; i < BU_PTBL_LEN(s->ghost); i++) {
	    const char *obj = (const char *)BU_PTBL_GET(s->ghost, i);
	    bu_vls_sprintf(&tcl_cmd, "lappend ::RtWizard::wizard_state(ghost_objlist) %s", obj);
	    (void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
	}
    }

    if (BU_PTBL_LEN(s->line) > 0) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(line_objlist) {}");
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
	for (i = 0; i < BU_PTBL_LEN(s->line); i++) {
	    const char *obj = (const char *)BU_PTBL_GET(s->line, i);
	    bu_vls_sprintf(&tcl_cmd, "lappend ::RtWizard::wizard_state(line_objlist) %s", obj);
	    (void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
	}
    }

    {
	unsigned char rgb[3];
	(void) bu_color_to_rgb_chars(s->bkg_color, (unsigned char *)rgb);
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(bg_color) %d %d %d", (int)rgb[0], (int)rgb[1], (int)rgb[2]);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }
    {
	unsigned char rgb[3];
	(void) bu_color_to_rgb_chars(s->line_color, (unsigned char *)rgb);
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(e_color) %d %d %d", (int)rgb[0], (int)rgb[1], (int)rgb[2]);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }
    {
	unsigned char rgb[3];
	(void) bu_color_to_rgb_chars(s->non_line_color, (unsigned char *)rgb);
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(ne_color) %d %d %d", (int)rgb[0], (int)rgb[1], (int)rgb[2]);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(ghosting_intensity) %0.15f", s->ghosting_intensity);
    (void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));

    bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(occmode) %d", s->occlusion);
    (void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));

    if (s->benchmark) {
	bu_vls_sprintf(&tcl_cmd, "set ::benchmark_mode 1");
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (s->cpus) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(cpu_count) %d", s->cpus);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (s->az < DBL_MAX) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(init_azimuth) %0.15f", s->az);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }
    if (s->el < DBL_MAX) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(init_elevation) %0.15f", s->el);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }
    if (s->tw < DBL_MAX) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(init_twist) %0.15f", s->tw);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }
    if (s->perspective > 0) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(perspective) %0.15f", s->perspective);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }
    if (s->zoom < DBL_MAX) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(zoom) %0.15f", s->zoom);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }
    if (s->center[0] < DBL_MAX) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(x_center) %0.15f", s->center[0]);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(y_center) %0.15f", s->center[1]);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(z_center) %0.15f", s->center[2]);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }


    if (s->viewsize < DBL_MAX) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(viewsize) %0.15f", s->viewsize);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (s->orientation[0] < DBL_MAX) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(orientation) \"%0.15f %0.15f %0.15f %0.15f\"", s->orientation[0], s->orientation[1], s->orientation[2], s->orientation[3]);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (s->eye_pt[0] < DBL_MAX) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(eye_pt) \"%0.15f %0.15f %0.15f\"", s->eye_pt[0], s->eye_pt[1], s->eye_pt[2]);
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (bu_vls_strlen(s->log_file)) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(log_file) %s", bu_vls_addr(s->log_file));
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }

    if (bu_vls_strlen(s->pid_file)) {
	bu_vls_sprintf(&tcl_cmd, "set ::RtWizard::wizard_state(pid_filename) %s", bu_vls_addr(s->pid_file));
	(void)Tcl_Eval(interp, bu_vls_addr(&tcl_cmd));
    }


}

int
main(int argc, char **argv)
{
    int need_help = 0;
    int uac = 0;
    int i = 0;
    char type = '\0';
    struct bu_vls optparse_msg = BU_VLS_INIT_ZERO;
    struct rtwizard_settings *s = rtwizard_settings_create();
    struct bu_opt_desc d[34];
    BU_OPT(d[0],  "h", "help",          "",          NULL,            &need_help,    "Print help and exit");
    BU_OPT(d[1],  "",  "gui",           "",          &bu_opt_int,     &s->use_gui,   "Force use of GUI.");
    BU_OPT(d[2],  "",  "no-gui",        "",          &bu_opt_vls,     &s->no_gui,    "Do not use GUI, even if information is insufficient.");
    BU_OPT(d[3],  "i", "input-file",    "filename",  &bu_opt_vls,     s->input_file, "Input .g database file");
    BU_OPT(d[4],  "o", "output-file",   "filename",  &bu_opt_vls,     s->output_file, "Image output file name");
    BU_OPT(d[5],  "d", "fbserv-device", "/dev/*",    &bu_opt_vls,      s->fb_dev,    "Device for framebuffer viewing");
    BU_OPT(d[6],  "p", "fbserv-port",   "#",         &bu_opt_int,     &s->port,      "Port # for framebuffer");
    BU_OPT(d[7],  "w", "width",         "#",         &opt_width,       s,            "Output image width (overrides -s)");
    BU_OPT(d[8],  "n", "height",        "#",         &opt_height,      s,            "Output image height (overrides -s)");
    BU_OPT(d[9],  "s", "size",          "#",         &opt_size,        s,            "Output width & height (for square image)");
    BU_OPT(d[10], "c", "color-objects", "obj1,...",  &opt_objs,        s->color,     "List of color objects to render");
    BU_OPT(d[11], "g", "ghost-objects", "obj1,...",  &opt_objs,        s->ghost,     "List of ghost objects to render");
    BU_OPT(d[12], "l", "line-objects",  "obj1,...",  &opt_objs,        s->line,      "List of line objects to render");
    BU_OPT(d[13], "C", "background-color", "R/G/B",  &bu_opt_color,    s->bkg_color, "Background image color");
    BU_OPT(d[14], "",  "line-color",    "R/G/B",     &bu_opt_color,    s->line_color, "Color used for line rendering");
    BU_OPT(d[15], "",  "non-line-color", "R/G/B",    &bu_opt_color,    s->non_line_color, "Color used for non-line rendering ??");
    BU_OPT(d[16], "G", "ghosting-intensity", "#[.#]", &bu_opt_fastf_t, &s->ghosting_intensity,    "Intensity of ghost objects");
    BU_OPT(d[17], "O", "occlusion",     "#",         &bu_opt_int,     &s->occlusion, "Occlusion mode");
    BU_OPT(d[18], "",  "benchmark",     "",          NULL,            &s->benchmark,    "Benchmark mode");
    BU_OPT(d[19], "",  "cpu-count",     "#",         &bu_opt_int,     &s->cpus,      "Specify the number of CPUs to use");
    BU_OPT(d[20], "a", "azimuth",       "#[.#]",     &bu_opt_fastf_t, &s->az,        "Set azimuth");
    BU_OPT(d[21], "e", "elevation",     "#[.#]",     &bu_opt_fastf_t, &s->el,        "Set elevation");
    BU_OPT(d[22], " ", "twist",         "#[.#]",     &bu_opt_fastf_t, &s->tw,        "Set twist");
    BU_OPT(d[23], "P",  "perspective",  "#[.#]",     &bu_opt_fastf_t, &s->perspective, "Set perspective");
    BU_OPT(d[24], "t", "type",          "A|B|C|D|E", &opt_letter,     &type,         "Specify RtWizard picture type");
    BU_OPT(d[25], "z", "zoom",          "#[.#] ",    &bu_opt_fastf_t, &s->zoom,      "Set zoom");
    BU_OPT(d[26], "",  "center",        "x,y,z",     &bu_opt_vect_t,  &s->center,    "Set view center");
    BU_OPT(d[27], "",  "viewsize",      "#[.#}",     &bu_opt_fastf_t, &s->viewsize,  "Set view size");
    BU_OPT(d[28], "",  "orientation",   "#[.#]/#[.#]/#[.#]/#[.#]", &opt_quat, &s->orientation,    "Set view orientation");
    BU_OPT(d[29], "",  "eye_pt",        "x,y,z",     &bu_opt_vect_t,  &s->eye_pt,    "set eye point");
    BU_OPT(d[30], "v", "verbose",       "#",         &bu_opt_int,     &s->verbose,      "Verbosity");
    BU_OPT(d[31], "",  "log-file",      "filename",  &bu_opt_vls,     s->log_file,      "Log debugging output to this file");
    BU_OPT(d[32], "",  "pid-file",      "filename",  &bu_opt_vls,     s->pid_file,      "File used to communicate PID numbers (for app developers)");
    BU_OPT_NULL(d[33]);

    /* Skip first arg */
    argv++; argc--;
    uac = bu_opt_parse(&optparse_msg, argc, (const char **)argv, d);

    if (uac == -1) {
	bu_exit(1, bu_vls_addr(&optparse_msg));
    }
    bu_vls_free(&optparse_msg);

    if (type != '\0') {
	bu_log("Image type: %c\n", type);
    }

    if (s->use_gui && s->no_gui) {
	bu_log("Warning - both -gui and -no-gui supplied - enabling gui\n");
	s->no_gui = 0;
    }

    if (bu_vls_strlen(s->input_file) && !bu_file_exists(bu_vls_addr(s->input_file), NULL)) {
	bu_exit(1, "Specified %s as .g file, but file does not exist.\n", bu_vls_addr(s->input_file));
    }

    /* Handle any leftover arguments per established conventions */
    for (i = 0; i < uac; i++) {
	struct bu_vls c = BU_VLS_INIT_ZERO;
	bu_log("av[%d]: %s\n", i, argv[i]);
	/* First, see if we have an input .g file */
	if (bu_vls_strlen(s->input_file) == 0) {
	    if (bu_path_component(&c, argv[i], BU_PATH_EXT)) {
		if (bu_file_mime(bu_vls_addr(&c), BU_MIME_MODEL) == BU_MIME_MODEL_VND_BRLCAD_PLUS_BINARY) {
		    if (bu_file_exists(argv[i],NULL)) {
			bu_vls_sprintf(s->input_file, "%s", argv[i]);
			/* This was the .g name - don't add it to the color list */
			continue;
		    } else {
			bu_exit(1, "Specified %s as .g file, but file does not exist.\n", argv[i]);
		    }
		}
	    }
	}
	bu_vls_trunc(&c, 0);
	/* Next, see if we have an image specified as an output destination */
	if (bu_vls_strlen(s->output_file) == 0 && bu_vls_strlen(s->fb_dev) == 0) {
	    if (bu_path_component(&c, argv[i], BU_PATH_EXT)) {
		if (rtwizard_imgformat_supported(bu_file_mime(bu_vls_addr(&c), BU_MIME_IMAGE))) {
		    bu_vls_sprintf(s->output_file, "%s", argv[i]);
		    /* This looks like the output image name - don't add it to the color list */
		    continue;
		}
	    }
	}
	/* If it's none of the above, assume a color object in the .g file */
	bu_ptbl_ins(s->color, (long *)bu_strdup(argv[i]));
    }

    print_rtwizard_state(s);
#if 0
    /* For now, all roads lead to Tcl. */

    /* TODO - this tcl initialization is common (more or less) to btclsh, bwish,
     * mged, and any other C programs wanting to lauch Tcl/Tk.  Should be
     * refactored into a libtclcad function. */
    {
	int status = 0;
	int try_auto_path = 0;
	int init_tcl = 1;
	int init_itcl = 1;

	struct bu_vls tcl_cmd = BU_VLS_INIT_ZERO;
	Tcl_Interp *interpreter = Tcl_CreateInterp();
	const char *rtwizard = bu_brlcad_data("tclscripts/rtwizard", 1);

	/* a two-pass init loop.  the first pass just tries default init
	 * routines while the second calls tclcad_auto_path() to help it
	 * find other, potentially uninstalled, resources.
	 */
	while (1) {

	    /* not called first time through, give Tcl_Init() a chance */
	    if (try_auto_path) {
		/* Locate the BRL-CAD-specific Tcl scripts, set the auto_path */
		tclcad_auto_path(interpreter);
	    }

	    /* Initialize Tcl */
	    Tcl_ResetResult(interpreter);
	    if (init_tcl && Tcl_Init(interpreter) == TCL_ERROR) {
		if (!try_auto_path) {
		    try_auto_path = 1;
		    continue;
		}
		bu_log("Tcl_Init ERROR:\n%s\n", Tcl_GetStringResult(interpreter));
		break;
	    }
	    init_tcl = 0;

	    /* Initialize [incr Tcl] */
	    Tcl_ResetResult(interpreter);
	    /* NOTE: Calling "package require Itcl" here is apparently
	     * insufficient without other changes elsewhere (per MGED comment,
	     * but package require should work - why doesn't it??).
	     */
	    if (init_itcl && Itcl_Init(interpreter) == TCL_ERROR) {
		if (!try_auto_path) {
		    Tcl_Namespace *nsp;

		    try_auto_path = 1;
		    /* Itcl_Init() leaves initialization in a bad state
		     * and can cause retry failures.  cleanup manually.
		     */
		    Tcl_DeleteCommand(interpreter, "::itcl::class");
		    nsp = Tcl_FindNamespace(interpreter, "::itcl", NULL, 0);
		    if (nsp)
			Tcl_DeleteNamespace(nsp);
		    continue;
		}
		bu_log("Itcl_Init ERROR:\n%s\n", Tcl_GetStringResult(interpreter));
		break;
	    }
	    init_itcl = 0;

	    /* don't actually want to loop forever */
	    break;

	} /* end iteration over Init() routines that need auto_path */
	Tcl_ResetResult(interpreter);

	/* if we haven't loaded by now, load auto_path so we find our tclscripts */
	if (!try_auto_path) {
	    /* Locate the BRL-CAD-specific Tcl scripts */
	    tclcad_auto_path(interpreter);
	}

	/*XXX FIXME: Should not be importing Itcl into the global namespace */
	/* Import [incr Tcl] commands into the global namespace. */
	if (Tcl_Import(interpreter, Tcl_GetGlobalNamespace(interpreter), "::itcl::*", /* allowOverwrite */ 1) != TCL_OK) {
	    bu_log("Tcl_Import ERROR: %s\n", Tcl_GetStringResult(interpreter));
	    Tcl_ResetResult(interpreter);
	}

	/* Initialize libbu */
	if (Bu_Init(interpreter) == TCL_ERROR) {
	    bu_log("Bu_Init ERROR:\n%s\n", Tcl_GetStringResult(interpreter));
	    Tcl_ResetResult(interpreter);
	}

	/* Initialize libbn */
	if (Bn_Init(interpreter) == TCL_ERROR) {
	    bu_log("Bn_Init ERROR:\n%s\n", Tcl_GetStringResult(interpreter));
	    Tcl_ResetResult(interpreter);
	}

	/* Initialize librt */
	if (Rt_Init(interpreter) == TCL_ERROR) {
	    bu_log("Rt_Init ERROR:\n%s\n", Tcl_GetStringResult(interpreter));
	    Tcl_ResetResult(interpreter);
	}

	/* Initialize libged */
	if (Go_Init(interpreter) == TCL_ERROR) {
	    bu_log("Ged_Init ERROR:\n%s\n", Tcl_GetStringResult(interpreter));
	    Tcl_ResetResult(interpreter);
	}

	Init_RtWizard_Vars(interpreter, s);

	bu_vls_sprintf(&tcl_cmd, "source %s", rtwizard);
	status = Tcl_Eval(interpreter, bu_vls_addr(&tcl_cmd));
    }
#endif

    /* Someday, we want to do this without Tcl via library calls unless
     * the GUI is needed... */
#if 0
    /* At this point, if we know we're supposed to launch the GUI, do it */
    if (s->use_gui) {
	/* launch gui */
    } else {
	/* Check that we know enough to make an image. */
	if (!rtwizard_info_sufficient(NULL, s, type)) {
	    /* If we *can* launch the GUI in this situation, do it */
	    if (s->no_gui) {
		bu_exit(1, "Image type %c specified, but supplied information is not sufficient to generate a type %c image.\n", type, type);
	    } else {
		/* Launch GUI */
	    }
	}
	/* We know enough - make our image */
    }
#endif
    return 0;
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
