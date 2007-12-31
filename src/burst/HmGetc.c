/*                        H M G E T C . C
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
 *
 */
/** @file HmGetc.c
 *	Author:		Gary S. Moss
 */

#if ! defined( lint )
static const char RCSid[] = "@(#)$Header$ (BRL)";
#endif
#include <stdio.h>
#include <signal.h>
#include "./burst.h"
#include "./Hm.h"
/*LINTLIBRARY*/
int
HmGetchar( void )
{
    int	c;

    while( (c = getc( HmTtyFp )) == EOF )
	;

    return c;
}

int
HmUngetchar( int c )
{
    return	ungetc( c, HmTtyFp );
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
