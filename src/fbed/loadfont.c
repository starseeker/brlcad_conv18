
/*                      L O A D F O N T . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2007 United States Government as represented by
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
/** @file loadfont.c
 *
 * Load a new font by reading in the header and directory.
 *
 * Author -
 *	Paul R. Stay
 * Date -
 *	Tue Jan  8 1985
 */

#include "common.h"

#include <stdio.h>

#include "machine.h"
#include "fb.h"

#include "./std.h"
#include "./ascii.h"
#include "./font.h"
#include "./try.h"
#include "./extern.h"


int loadfont(char *ff)
{
	FILE		*newff;
	struct header	lochdr;

	/* Open the file and read in the header information. */
	if( (newff = fopen( ff, "r" )) == NULL )
		{
		MvCursor( 1, 4 );
		prnt_Debug( "Error opening font file '%s'", ff );
		ffdes = NULL;
		return 0;
		}
	if( ffdes != NULL )
		(void) fclose(ffdes);
	ffdes = newff;
	if( fread( (char *) &lochdr, (int) sizeof(struct header), 1, ffdes ) < 1 )
		{
		(void) fprintf( stderr, "loadfont() read failed!\n" );
		ffdes = NULL;
		return 0;
		}

	if( lochdr.magic != 0436 )
		{
		prnt_Debug( "Not a font file: %s", ff );
		ffdes = NULL;
		return 0;
		}
	hdr = lochdr;

	/* Read in the directory for the font. */
	if( fread( (char *) dir, (int) sizeof(struct dispatch), 256, ffdes ) < 256 )
		{
		(void) fprintf( stderr, "loadfont() read failed!\n" );
		ffdes = NULL;
		return 0;
		}
	/* Addresses of characters in the file are relative to
		point in the file after the directory, so grab the
		current position.
	 */
	offset = ftell( ffdes );
	return 1;
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
