/*                       V I E W M L T . C
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
/** @file viewmlt.c
 *
 * Implementation of the Metropolis Light Transport algorithm.
 *
 */

#include "common.h"

#include <stdio.h>

#include "vmath.h"
#include "raytrace.h"

#include "rtprivate.h"

int use_air = 0;

const char title[] = "Metropolis Light Transport renderer";
const char usage[] = "\
Usage:  rtmlt [options] model.g objects... > file.pix\n\
Options:\n\
 -s #		Grid size in pixels, default 512\n\
 -a Az		Azimuth in degrees\n\
 -e Elev	Elevation in degrees\n\
 -M		Read matrix, cmds on stdin\n\
 -o file.pix	Output file name\n\
 -x #		Set librt debug flags\n\
";

int	rayhit(register struct application *ap, struct partition *PartHeadp);
int	raymiss(register struct application *ap);

struct bu_structparse view_parse[] = {
    "",	0, (char *)0,	0,	BU_STRUCTPARSE_FUNC_NULL
};

struct point_list {
    struct bu_list l;
    point_t pt_cell;
};

struct mlt_path {
    struct bu_list l;
    struct point_list * points;
};

struct mlt_app {
    struct application * ap;
    struct mlt_path * path_list;
    point_t eye;
    point_t camera;
    /* ... */
};

int
mlt_hit(register struct application *ap, struct partition *PartHeadp);

int
mlt_miss(register struct application * ap);

/*
 *  			V I E W _ I N I T
 *
 *  Called by main() at the start of a run.
 *  Returns 1 if framebuffer should be opened, else 0.
 */
view_init(register struct application *ap, char *file, char *obj, int minus_o)
{
    return(0);		/* no framebuffer needed */
}

/*
 *			V I E W _ 2 I N I T
 *
 *  The beginning of a frame.
 *  Called by do_frame() just before raytracing starts.
 */
void
view_2init(struct application *ap)
{
    ap->a_hit = mlt_hit;
    ap->a_miss = mlt_miss;
    ap->a_onehit = 1;
}

/*
 *			V I E W _ P I X E L
 *
 *  Called by worker() after the end of proccessing for each pixel.
 */
void
view_pixel(register struct application *ap)
{
}

/*
 *			V I E W _ E O L
 *
 *  Called by worker() at the end of each line.  Depricated.
 *  Any end-of-line processing should be done in view_pixel().
 */
void
view_eol(register struct application *ap)
{
}

/*
 *			V I E W _ E N D
 *
 *  Called in do_frame() at the end of a frame,
 *  just after raytracing completes.
 */
void
view_end(register struct application *ap)
{
}

/*
 *			V I E W _ S E T U P
 *
 *  Called by do_prep(), just before rt_prep() is called, in do.c
 *  This allows the lighting model to get set up for this frame,
 *  e.g., generate lights, associate materials routines, etc.
 */
void
view_setup(struct rt_i *rtip)
{
}

/*
 *			V I E W _ C L E A N U P
 *
 *  Called by "clean" command, just before rt_clean() is called, in do.c
 */
void
view_cleanup(struct rt_i *rtip)
{
}

/*
 *			R A Y H I T
 *
 *  Called via a_hit linkage from rt_shootray() when ray hits.
 */
int
mlt_hit(register struct application *ap, struct partition *PartHeadp)
{
    bu_log("hit: 0x%x\n", ap->a_resource);

    return(1);	/* report hit to main routine */
}

/*
 *			R A Y M I S S
 *
 *  Called via a_miss linkage from rt_shootray() when ray misses.
 */
int
mlt_miss(register struct application *ap)
{
    bu_log("miss: 0x%x\n", ap->a_resource);

    return(0);
}

void application_init (void) {}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
