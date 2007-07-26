/*                        B W R E C T . C
 * BRL-CAD
 *
 * Copyright (c) 1986-2007 United States Government as represented by
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
/** @file bwrect.c
 *
 * Remove a portion of a potentially huge .bw file.
 *
 *  Author -
 *	Phillip Dykstra
 *	2 Oct 1985
 *
 */
#ifndef lint
static const char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "common.h"

#include <stdlib.h>
#include <stdio.h>

#include "machine.h"


int	xnum, ynum;		/* Number of pixels in new map */
int	xorig, yorig;		/* Bottom left corner to extract from */
int	linelen;
char	*buf;			/* output scanline buffer, malloc'd */

int
main(int argc, char **argv)
{
	FILE	*ifp, *ofp;
	int	row;
	long	offset;

	if (argc < 3) {
		printf("usage: bwrect infile outfile (I prompt!)\n");
		exit( 1 );
	}
	if ((ifp = fopen(argv[1], "r")) == NULL) {
		printf("pixrect: can't open %s\n", argv[1]);
		exit( 2 );
	}
	if ((ofp = fopen(argv[2], "w")) == NULL) {
		printf("pixrect: can't open %s\n", argv[1]);
		exit( 3 );
	}

	/* Get info */
	printf( "Area to extract (x, y) in pixels " );
	scanf( "%d%d", &xnum, &ynum );
	printf( "Origin to extract from (0,0 is lower left) " );
	scanf( "%d%d", &xorig, &yorig );
	printf( "Scan line length of input file " );
	scanf( "%d", &linelen );

	buf = (char *)malloc( xnum );

	/* Move all points */
	for (row = 0+yorig; row < ynum+yorig; row++) {
		offset = row * linelen + xorig;
		fseek(ifp, offset, 0);
		fread(buf, sizeof(*buf), xnum, ifp);
		fwrite(buf, sizeof(*buf), xnum, ofp);
	}
	return 0;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
