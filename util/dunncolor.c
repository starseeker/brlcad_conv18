/*
 *			D U N N C O L O R . C				
 *
 *	Sets the exposure values in the Dunn camera to the
 *	specified arguments.
 *
 *	dunncolor baseval redval greenval blueval
 *
 *  Author -
 *	Don Merritt
 *	August 1985
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>

extern int	fd;
extern char	cmd;
extern int	polaroid;

main(argc, argv)
int argc;
char **argv;
{

	dunnopen();

	if(!ready(5)) {
		printf("dunncolor:  camera not ready\n");
		exit(50);
	}

	if( argc > 2 && strcmp( argv[1], "-p" ) == 0 )  {
		/* Polaroid rather than external camera */
		polaroid = 1;
		argc--; argv++;
	}
	getexposure("old");
	if(!ready(5)) {
		printf("dunncolor:  camera not ready\n");
		exit(50);
	}

	/* check argument */
	if ( argc != 5 && argc != 6 ) {
		printf("usage: dunncolor [-p] baseval redval greenval blueval\n"); 
		exit(25);
	}

	dunnsend('A',atoi(*++argv));
	dunnsend('R',atoi(*++argv));
	dunnsend('G',atoi(*++argv));
	dunnsend('B',atoi(*++argv));

	getexposure("new");

	if(!ready(5)) {
		printf("dunncolor:  camera not ready\n");
		exit(50);
	}
}
