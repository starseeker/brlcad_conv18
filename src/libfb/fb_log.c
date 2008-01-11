/*                        F B _ L O G . C
 * BRL-CAD
 *
 * Copyright (c) 1986-2008 United States Government as represented by
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
/** @addtogroup fb */
/** @{ */
/** @file fb_log.c
 *
 * Log a framebuffer library event in the Standard way.
    Author -
    Gary S. Moss

*/
/** @} */

#ifndef lint
static const char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "common.h"

#include <stdio.h>
#include <stdarg.h>

#include "machine.h"
#include "fb.h"


/*
 *  			F B _ L O G
 *
 *  Log a framebuffer library event in the Standard way.
 */
void
fb_log( const char *fmt, ... )
{
    va_list ap;

    va_start( ap, fmt );
    (void)vfprintf( stderr, fmt, ap );
    va_end(ap);
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
