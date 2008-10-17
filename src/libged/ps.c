/*                         P S . C
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
/** @file ps.c
 *
 * The ps command.
 *
 */

#include "common.h"
#include "bio.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "solid.h"
#include "ged_private.h"

static fastf_t ged_default_ps_ppi = 72.0;
static fastf_t ged_default_ps_scale = 4.5 * 72.0 / 4096.0;
static fastf_t ged_ps_color_sf = 1.0/255.0;

#define GED_TO_PS(_x) ((int)((_x)+2048))
#define GED_TO_PS_COLOR(_c) ((_c)*ged_ps_color_sf)

static void
ged_draw_ps_header(FILE *fp, char *font, char *title, char *creator, int linewidth, fastf_t scale, int xoffset, int yoffset)
{
    fprintf(fp, "%%!PS-Adobe-1.0\n\
%%begin(plot)\n\
%%%%DocumentFonts:  %s\n",
	    font);

    fprintf(fp, "%%%%Title: %s\n", title);

    fprintf(fp, "\
%%%%Creator: %s\n\
%%%%BoundingBox: 0 0 324 324	%% 4.5in square, for TeX\n\
%%%%EndComments\n\
\n",
	    creator);

    fprintf(fp, "\
%d setlinewidth\n\
\n\
%% Sizes, made functions to avoid scaling if not needed\n\
/FntH /%s findfont 80 scalefont def\n\
/DFntL { /FntL /%s findfont 73.4 scalefont def } def\n\
/DFntM { /FntM /%s findfont 50.2 scalefont def } def\n\
/DFntS { /FntS /%s findfont 44 scalefont def } def\n\
\n\
%% line styles\n\
/NV { [] 0 setdash } def	%% normal vectors\n\
/DV { [8] 0 setdash } def	%% dotted vectors\n\
/DDV { [8 8 32 8] 0 setdash } def	%% dot-dash vectors\n\
/SDV { [32 8] 0 setdash } def	%% short-dash vectors\n\
/LDV { [64 8] 0 setdash } def	%% long-dash vectors\n\
\n\
/NEWPG {\n\
	%d %d translate\n\
	%f %f scale	%% 0-4096 to 324 units (4.5 inches)\n\
} def\n\
\n\
FntH  setfont\n\
NEWPG\n\
",
	    linewidth, font, font, font, font, scale, scale, xoffset, yoffset);
}

static void
ged_draw_ps_solid(struct ged *gedp, FILE *fp, struct solid *sp, matp_t psmat)
{
    static vect_t		last;
    point_t			clipmin = {-1.0, -1.0, -MAX_FASTF};
    point_t			clipmax = {1.0, 1.0, MAX_FASTF};
    register struct bn_vlist	*tvp;
    register point_t		*pt_prev=NULL;
    register fastf_t		dist_prev=1.0;
    register fastf_t		dist;
    register struct bn_vlist 	*vp = (struct bn_vlist *)&sp->s_vlist;
    fastf_t			delta;
    int 			useful = 0;

    fprintf(fp, "%f %f %f setrgbcolor\n",
	    GED_TO_PS_COLOR(sp->s_color[0]),
	    GED_TO_PS_COLOR(sp->s_color[1]),
	    GED_TO_PS_COLOR(sp->s_color[2]));

    /* delta is used in clipping to insure clipped endpoint is slightly
     * in front of eye plane (perspective mode only).
     * This value is a SWAG that seems to work OK.
     */
    delta = psmat[15]*0.0001;
    if ( delta < 0.0 )
	delta = -delta;
    if ( delta < SQRT_SMALL_FASTF )
	delta = SQRT_SMALL_FASTF;

    for ( BU_LIST_FOR( tvp, bn_vlist, &vp->l ) )  {
	register int	i;
	register int	nused = tvp->nused;
	register int	*cmd = tvp->cmd;
	register point_t *pt = tvp->pt;
	for ( i = 0; i < nused; i++, cmd++, pt++ )  {
	    static vect_t	start, fin;
	    switch ( *cmd )  {
		case BN_VLIST_POLY_START:
		case BN_VLIST_POLY_VERTNORM:
		    continue;
		case BN_VLIST_POLY_MOVE:
		case BN_VLIST_LINE_MOVE:
		    /* Move, not draw */
		    if (gedp->ged_gvp->gv_perspective > 0) {
			/* cannot apply perspective transformation to
			 * points behind eye plane!!!!
			 */
			dist = VDOT( *pt, &psmat[12] ) + psmat[15];
			if ( dist <= 0.0 )
			{
			    pt_prev = pt;
			    dist_prev = dist;
			    continue;
			}
			else
			{
			    MAT4X3PNT( last, psmat, *pt );
			    dist_prev = dist;
			    pt_prev = pt;
			}
		    }
		    else
			MAT4X3PNT( last, psmat, *pt );
		    continue;
		case BN_VLIST_POLY_DRAW:
		case BN_VLIST_POLY_END:
		case BN_VLIST_LINE_DRAW:
		    /* draw */
		    if (gedp->ged_gvp->gv_perspective > 0) {
			/* cannot apply perspective transformation to
			 * points behind eye plane!!!!
			 */
			dist = VDOT( *pt, &psmat[12] ) + psmat[15];
			if ( dist <= 0.0 )
			{
			    if ( dist_prev <= 0.0 )
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
				MAT4X3PNT( fin, psmat, tmp_pt );
			    }
			}
			else
			{
			    if ( dist_prev <= 0.0 )
			    {
				fastf_t alpha;
				vect_t diff;
				point_t tmp_pt;

				/* clip other end */
				VSUB2( diff, *pt, *pt_prev );
				alpha = (-dist_prev + delta) / ( dist - dist_prev );
				VJOIN1( tmp_pt, *pt_prev, alpha, diff );
				MAT4X3PNT( last, psmat, tmp_pt );
				MAT4X3PNT( fin, psmat, *pt );
			    }
			    else
			    {
				MAT4X3PNT( fin, psmat, *pt );
			    }
			}
		    }
		    else
			MAT4X3PNT( fin, psmat, *pt );
		    VMOVE( start, last );
		    VMOVE( last, fin );
		    break;
	    }

	    if (ged_vclip(start, fin, clipmin, clipmax) == 0)
		continue;

	    fprintf(fp,
		    "newpath %d %d moveto %d %d lineto stroke\n",
		    GED_TO_PS( start[0] * 2047 ),
		    GED_TO_PS( start[1] * 2047 ),
		    GED_TO_PS( fin[0] * 2047 ),
		    GED_TO_PS( fin[1] * 2047 ) );
	    useful = 1;
	}
    }
}

static void
ged_draw_ps_body(struct ged *gedp, FILE *fp)
{
    mat_t new;
    matp_t mat;
    mat_t perspective_mat;
    struct solid *sp;

    mat = gedp->ged_gvp->gv_model2view;

    if (0 < gedp->ged_gvp->gv_perspective) {
	point_t l, h;

	VSET(l, -1.0, -1.0, -1.0);
	VSET(h, 1.0, 1.0, 200.0);

	if (gedp->ged_gvp->gv_eye_pos[Z] == 1.0) {
	    /* This way works, with reasonable Z-clipping */
	    ged_persp_mat(perspective_mat, gedp->ged_gvp->gv_perspective,
			  (fastf_t)1.0f, (fastf_t)0.01f, (fastf_t)1.0e10f, (fastf_t)1.0f);
	} else {
	    /* This way does not have reasonable Z-clipping,
	     * but includes shear, for GDurf's testing.
	     */
	    ged_deering_persp_mat(perspective_mat, l, h, gedp->ged_gvp->gv_eye_pos);
	}

	bn_mat_mul(new, perspective_mat, mat);
	mat = new;
    }

    FOR_ALL_SOLIDS(sp, &gedp->ged_gdp->gd_headSolid) {
	ged_draw_ps_solid(gedp, fp, sp, mat);
    }
}

static void
ged_draw_ps_footer(FILE *fp)
{
    fputs("% showpage	% uncomment to use raw file\n", fp);
    fputs("%end(plot)\n", fp);
}

int
ged_ps(struct ged *gedp, int argc, const char *argv[])
{
    FILE *fp;
    struct bu_vls creator;
    struct bu_vls font;
    struct bu_vls title;
    fastf_t scale = ged_default_ps_scale;
    int linewidth = 4;
    int xoffset = 0;
    int yoffset = 0;
    int zclip = 0;
    int k;
    static const char *usage = "[-c creator] [-f font] [-s size] [-t title] [-x offset] [-y offset] file";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);
    GED_CHECK_VIEW(gedp, BRLCAD_ERROR);
    GED_CHECK_DRAWABLE(gedp, BRLCAD_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc == 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_HELP;
    }

    /* Initialize var defaults */
    bu_vls_init(&font);
    bu_vls_init(&title);
    bu_vls_init(&creator);
    bu_vls_printf(&font, "Courier");
    bu_vls_printf(&title, "No Title");
    bu_vls_printf(&creator, "LIBGED ps");

    /* Process options */
    bu_optind = 1;
    while ((k = bu_getopt(argc, (char * const *)argv, "c:f:s:t:x:y:")) != EOF) {
	fastf_t tmp_f;

	switch (k) {
	    case 'c':
		bu_vls_trunc(&creator, 0);
		bu_vls_printf(&creator, bu_optarg);

		break;
	    case 'f':
		bu_vls_trunc(&font, 0);
		bu_vls_printf(&font, bu_optarg);

		break;
	    case 's':
		if (sscanf(bu_optarg, "%lf", &tmp_f) != 1) {
		    bu_vls_printf(&gedp->ged_result_str, "%s: bad size - %s", argv[0], bu_optarg);
		    goto bad;
		}

		if (tmp_f < 0.0 || NEAR_ZERO(tmp_f, 0.1)) {
		    bu_vls_printf(&gedp->ged_result_str, "%s: bad size - %s, must be greater than 0.1 inches\n", argv[0], bu_optarg);
		    goto bad;
		}

		scale = tmp_f * ged_default_ps_ppi / 4096.0;

		break;
	    case 't':
		bu_vls_trunc(&title, 0);
		bu_vls_printf(&title, bu_optarg);

		break;
	    case 'x':
		if (sscanf(bu_optarg, "%lf", &tmp_f) != 1) {
		    bu_vls_printf(&gedp->ged_result_str, "%s: bad x offset - %s", argv[0], bu_optarg);
		    goto bad;
		}
		xoffset = (int)(tmp_f * ged_default_ps_ppi);

		break;
	    case 'y':
		if (sscanf(bu_optarg, "%lf", &tmp_f) != 1) {
		    bu_vls_printf(&gedp->ged_result_str, "%s: bad y offset - %s", argv[0], bu_optarg);
		    goto bad;
		}
		yoffset = (int)(tmp_f * ged_default_ps_ppi);

		break;
	default:
	    bu_vls_printf(&gedp->ged_result_str, "%s: Unrecognized option - %s", argv[0], bu_optarg);
	    goto bad;
	}
    }

    if ((argc - bu_optind) != 1) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	goto bad;
    }

    if ((fp = fopen(argv[bu_optind], "wb")) == NULL) {
	bu_vls_printf(&gedp->ged_result_str, "%s: Error opening file - %s\n", argv[0], argv[bu_optind]);
	goto bad;
    }

    ged_draw_ps_header(fp, bu_vls_addr(&font), bu_vls_addr(&title), bu_vls_addr(&creator), linewidth, scale, xoffset, yoffset);
    ged_draw_ps_body(gedp, fp);
    ged_draw_ps_footer(fp);

    fclose(fp);

    bu_vls_free(&font);
    bu_vls_free(&title);
    bu_vls_free(&creator);

    return BRLCAD_OK;

  bad:
    bu_vls_free(&font);
    bu_vls_free(&title);
    bu_vls_free(&creator);

    return BRLCAD_ERROR;
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
