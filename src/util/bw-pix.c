/*                        B W - P I X . C
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
/** @file bw-pix.c
 *
 *  Convert an 8-bit black and white file to a 24-bit
 *  color one by replicating each value three times.
 *
 *  Author -
 *	Phillip Dykstra
 *
 */
#ifndef lint
static const char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "common.h"


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

unsigned char	ibuf[1024], obuf[3*1024];

static char usage[] = "Usage: bw-pix [in.bw] [out.pix]\n";

int
main(int argc, char **argv)
{
	int	in, out, num;
	FILE	*finp, *foutp;

	/* check for input file */
	if( argc > 1 ) {
		if( (finp = fopen( argv[1], "r" )) == NULL ) {
			fprintf( stderr, "bw-pix: can't open \"%s\"\n", argv[1] );
			exit( 1 );
		}
	} else
		finp = stdin;

	/* check for output file */
	if( argc > 2 ) {
		if( (foutp = fopen( argv[2], "w" )) == NULL ) {
			fprintf( stderr, "bw-pix: can't open \"%s\"\n", argv[2] );
			exit( 2 );
		}
	} else
		foutp = stdout;

	if( argc > 3 || isatty(fileno(finp)) || isatty(fileno(foutp)) ) {
		fputs( usage, stderr );
		exit( 3 );
	}

	while( (num = fread( ibuf, sizeof( char ), 1024, finp )) > 0 ) {
		for( in = out = 0; in < num; in++, out += 3 ) {
			obuf[out] = ibuf[in];
			obuf[out+1] = ibuf[in];
			obuf[out+2] = ibuf[in];
		}
		fwrite( obuf, sizeof( char ), 3*num, foutp );
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
