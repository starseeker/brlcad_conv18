/*                        C O M M O N . H
 * BRL-CAD
 *
 * Copyright (C) 2004-2005 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file common.h
 *
 *  Header file for the BRL-CAD common definitions.
 *
 *  This header wraps the system-specific encapsulation of config.h
 *  and removes need to conditionally include config.h everywhere.
 *  The common definitions are symbols common to the platform being
 *  built that are either detected via configure or hand crafted, as
 *  is the case for the win32 platform.
 *
 *  Author -
 *	Christopher Sean Morrison
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  $Header$
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#ifndef PACKAGE
#  ifdef HAVE_CONFIG_H
#    include "config.h"
#  else
#    include <brlcad/config.h>
#  endif
#endif

#ifdef __win32
#  include "config_win.h"
#endif

#endif  /* __COMMON_H__ */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
