/*                    P I X B G S T R I P . C
 * BRL-CAD
 *
 * Copyright (c) 1991-2004 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file; see the file named COPYING for more
 * information.
 */
/** @file pixbgstrip.c
 *
 * Function -
 *	Backgound Un-Maker
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 */
#ifndef lint
static const char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "common.h"

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
                                                                                                                                                                            

#include <stdio.h>	
#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "fb.h"

static unsigned char *scanline;		/* 1 scanline pixel buffer */
static int	scanbytes;		/* # of bytes of scanline */

static char	*file_name;
static FILE	*infp;
static int	fileinput = 0;		/* file of pipe on input? */

static int	autosize = 0;		/* !0 to autosize input */

static int	file_width = 512;	/* default input width */

static int	thresh = 1;
static int	bg_x_offset = 0;

static char usage[] = "\
Usage: pixbgstrip [-a -h] [-t thresh] [-x x_off for bg pixel]\n\
	[-s squarefilesize] [-w file_width] [-n file_height]\n\
	[file.pix]\n";

int
get_args(int argc, register char **argv)
{
	register int c;

	while ( (c = getopt( argc, argv, "ahs:w:n:t:x:" )) != EOF )  {
		switch( c )  {
		case 'a':
			autosize = 1;
			break;
		case 'h':
			/* high-res */
			file_width = 1024;
			autosize = 0;
			break;
		case 's':
			/* square file size */
			file_width = atoi(optarg);
			autosize = 0;
			break;
		case 'w':
			file_width = atoi(optarg);
			autosize = 0;
			break;
		case 'n':
			autosize = 0;
			break;
		case 't':
			thresh = atoi(optarg);
			break;
		case 'x':
			bg_x_offset = atoi(optarg);
			break;

		default:		/* '?' */
			return(0);
		}
	}

	if( optind >= argc )  {
		if( isatty(fileno(stdin)) )
			return(0);
		file_name = "-";
		infp = stdin;
	} else {
		file_name = argv[optind];
		if( (infp = fopen(file_name, "r")) == NULL )  {
			perror(file_name);
			(void)fprintf( stderr,
				"pixbgstrip: cannot open \"%s\" for reading\n",
				file_name );
			exit(1);
		}
		fileinput++;
	}

	if ( argc > ++optind )
		(void)fprintf( stderr, "pixbgstrip: excess argument(s) ignored\n" );

	return(1);		/* OK */
}

int
main(int argc, char **argv)
{
	register int	r,g,b;
	register int	i;

	if ( !get_args( argc, argv ) )  {
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	if( isatty(fileno(stdout)) )  {
		(void)fputs("Binary output must be redirected away from the terminal\n", stderr);
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	/* autosize input? */
	if( fileinput && autosize ) {
		int	w, h;
		if( bn_common_file_size(&w, &h, file_name, 3) ) {
			file_width = w;
		} else {
			fprintf(stderr,"pixbgstrip: unable to autosize\n");
		}
	}

	scanbytes = file_width * sizeof(RGBpixel);
	if( (scanline = (unsigned char *)malloc(scanbytes)) == (unsigned char *)NULL )  {
		fprintf(stderr,
			"pixbgstrip:  malloc(%d) failure for scanline buffer\n",
			scanbytes);
		exit(2);
	}

	while( !feof(infp) )  {
		if( fread( scanline, 1, scanbytes, infp ) != scanbytes )
			break;
		r = scanline[bg_x_offset*3+0];
		g = scanline[bg_x_offset*3+1];
		b = scanline[bg_x_offset*3+2];
		for( i=0; i<file_width; i++ )  {
			register int diff;

			diff = scanline[i*3+0] - r;
			if( diff <= -thresh || diff >= thresh ) continue;

			diff = scanline[i*3+1] - g;
			if( diff <= -thresh || diff >= thresh ) continue;

			diff = scanline[i*3+2] - b;
			if( diff <= -thresh || diff >= thresh ) continue;

			/* Input pixel matches background, set to black */
			scanline[i*3+0] =
				scanline[i*3+1] =
				scanline[i*3+2] = 0;
		}
		if( fwrite( scanline, 1, scanbytes, stdout) != scanbytes )  {
			perror("pixbgstrip: fwrite()");
			exit(1);
		}
	}
	return(0);
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
