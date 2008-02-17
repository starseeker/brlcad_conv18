/*                       G E T F O N T . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2008 United States Government as represented by
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
/** @file getfont.c
 *
 * Load a new font by reading in the header and directory.
 *
 */

#include "common.h"

#include <stdio.h>
#include <assert.h>

#include "fb.h"

#include "./font.h"

/* Variables controlling the font itself */
FILE		*ffdes;		/* File pointer for current font.	*/
long		offset;		/* Current offset to character data.	*/
struct header	hdr;		/* Header for font file.		*/
struct dispatch	dir[256];	/* Directory for character font.	*/
int		width = 0,	/* Size of current character.		*/
		height = 0;

int
get_Font(const char* fontname)
{	FILE		*newff;
		struct header	lochdr;
		static char	fname[FONTNAMESZ];
	if ( fontname == NULL )
		fontname = FONTNAME;
	if ( fontname[0] != '/' )		/* absolute path */
		(void) snprintf( fname, sizeof(fname), "%s/%s", FONTDIR, fontname );
	else
		bu_strlcpy( fname, fontname, sizeof(fname) );

	/* Open the file and read in the header information. */
	if ( (newff = fopen( fname, "rb" )) == NULL )
		{
		bu_log( "Error opening font file '%s'\n", fname );
		ffdes = NULL;
		return	0;
		}
	if ( ffdes != NULL )
		(void) fclose(ffdes);
	ffdes = newff;
	if ( fread( (char *) &lochdr, (int) sizeof(struct header), 1, ffdes ) != 1 )
		{
		bu_log( "get_Font() read failed!\n" );
		ffdes = NULL;
		return	0;
		}
	SWAB( lochdr.magic );
	SWAB( lochdr.size );
	SWAB( lochdr.maoxx );
	SWAB( lochdr.maxy );
	SWAB( lochdr.xtend );

	if ( lochdr.magic != 0436 )
		{
		bu_log( "Not a font file \"%s\": magic=0%o\n",
			fname, (int) lochdr.magic
			);
		ffdes = NULL;
		return	0;
		}
	hdr = lochdr;

	/* Read in the directory for the font. */
	if ( fread( (char *) dir, (int) sizeof(struct dispatch), 256, ffdes ) != 256 )
		{
		bu_log( "get_Font() read failed!\n" );
		ffdes = NULL;
		return	0;
		}
	/* Addresses of characters in the file are relative to
		point in the file after the directory, so grab the
		current position.
	 */
	offset = ftell( ffdes );
	return	1;
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
