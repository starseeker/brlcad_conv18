/*                         Q U E R Y . C
 * BRL-CAD
 *
 * Copyright (c) 2004 United States Government as represented by the
 * U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
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
/** @file query.c
 *
 */

#include "common.h"


#ifdef HAVE_STRING_H
#include <string.h>
#endif

#if 0
#include "tk.h"
#else
#include "tcl.h"
#ifndef WIN32
#include <X11/Xlib.h>
#endif
#endif

#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "dm.h"

int dm_validXType(char *dpy_string, char *name);
char *dm_bestXType(char *dpy_string);

int
dm_validXType(char	*dpy_string,
	      char	*name)
{
	int val = 0;

#ifdef WIN32
  if(!strcmp(name, "ogl"))
     return 1;
#else
#if !defined(USE_MESA_GL) && defined(DM_OGL)
	int return_val;
#endif

#ifdef USE_MESA_GL

#ifdef DM_OGL
	if (!strcmp(name, "ogl"))
		return 1;
#endif
#ifdef DM_X
	if (!strcmp(name, "X"))
		return 1;
#endif

#else /* Here we assume the X server supports OpenGL */
	Display *dpy;

	if ((dpy = XOpenDisplay(dpy_string)) == NULL) {
		bu_log("dm_validXType: failed to open display - %s\n", dpy_string);
		return val;
	}

#ifdef DM_OGL
	if (!strcmp(name, "ogl") &&
	    XQueryExtension(dpy, "GLX", &return_val, &return_val, &return_val))
		val = 1;
	else
#endif  
#ifdef DM_X
		if (!strcmp(name, "X"))
			val = 1;
#endif

	XCloseDisplay(dpy);

#endif

#endif /* WIN32*/

  return val;
}

char *
dm_bestXType(char *dpy_string)
{
	char *name = (char *)NULL;

#ifdef WIN32
  return "ogl";
#else

#if !defined(USE_MESA_GL) && defined(DM_OGL)
	int return_val;
#endif

#ifdef USE_MESA_GL

#ifdef DM_OGL
	return "ogl";
#endif
#ifdef DM_X
	return "X";
#endif

#else /* Here we assume the X server supports OpenGL */
	Display *dpy;

	if ((dpy = XOpenDisplay(dpy_string)) == NULL) {
		bu_log("dm_bestXType: failed to open display - %s\n", dpy_string);
		return name;
	}

#ifdef DM_OGL
	if (XQueryExtension(dpy, "GLX", &return_val, &return_val, &return_val))
		name = "ogl";
	else
#endif  
#ifdef DM_X
		name = "X";
#endif

	XCloseDisplay(dpy);

#endif
#endif /* WIN32 */

	return name;
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
