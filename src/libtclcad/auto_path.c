/*                       A U T O _ P A T H . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2020 United States Government as represented by
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
/**
 *
 * Locate the BRL-CAD tclscripts
 *
 * TODO - superbuild presents us with a challenge here.  This logic
 * is optimized for assuming that packages are either installed with
 * the associated system Tcl/Tk or present in the BRL-CAD build
 * directory.  Superbuilds present another possibility - a system
 * Tcl/Tk, a superbuild Itcl/Itk in the superbuild's directory,
 * and a BRL-CAD build needing to reference the staging install of
 * Itcl/Itk during its build.
 *
 * Not sure what the answer is yet.  One possibility is to use the
 * environment variables during the BRL-CAD build's execution of
 * its tools, but not sure yet if that's practical.
 */

#include "common.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tcl.h"
#ifdef HAVE_TK
#  include "tk.h"
#endif

#include "bu/app.h"
#include "tclcad.h"

#define MAX_BUF 2048

/* Appends a new path to the path list, preceded by BU_PATH_SEPARATOR.
 *
 * The path is specified as a sequence of string arguments, one per
 * directory, terminated by a (const char *)NULL argument.
 *
 * BU_DIR_SEPARATOR is inserted between the string arguments (but not
 * before or after the path).
 */
static void
join_path(struct bu_vls *path_list, ...)
{
    va_list ap;
    const char *dir;

    bu_vls_putc(path_list, BU_PATH_SEPARATOR);

    va_start(ap, path_list);

    dir = va_arg(ap, const char *);
    while (dir != NULL) {
	bu_vls_printf(path_list, "%s", dir);

	dir = va_arg(ap, const char *);
	if (dir != NULL) {
	    bu_vls_putc(path_list, BU_DIR_SEPARATOR);
	}
    }
    va_end(ap);
}

/**
 * Set up the Tcl auto_path for locating various necessary BRL-CAD
 * scripting resources. Detect whether the current invocation is from
 * an installed binary or not and append to the auto_path accordingly
 * for where the needed tclscript resources should be found.
 *
 ** installed invocation paths
 * BRLCAD_ROOT/lib/tclTCL_VERSION/init.tcl
 * BRLCAD_ROOT/lib/tclTK_VERSION/tk.tcl
 * BRLCAD_ROOT/lib/itclITCL_VERSION/itcl.tcl
 * BRLCAD_ROOT/lib/itkITCL_VERSION/itk.tcl
 * BRLCAD_ROOT/lib/iwidgetsIWIDGETS_VERSION/iwidgets.tcl
 * BRLCAD_ROOT/share/tclscripts/pkgIndex.tcl and subdirs
 *
 * if TCLCAD_LIBRARY_PATH is set
 *   append to search path
 * get installation directory and invocation path
 * if being run from installation directory
 *   add installation paths to search path
 * if being run from source directory
 *   add source paths to search path
 * add installation paths to search path
 */
void
tclcad_auto_path(Tcl_Interp *interp)
{
    struct bu_vls auto_path = BU_VLS_INIT_ZERO;
    struct bu_vls lappend = BU_VLS_INIT_ZERO;
    const char *library_path = NULL;

    struct bu_vls root_buf = BU_VLS_INIT_ZERO;
    const char *root = NULL;
    const char *data = NULL;
    char buffer[MAX_BUF] = {0};

    const char *which_argv = NULL;
    const char *srcpath = NULL;
    int from_installed = 0;

    int found_init_tcl = 0;
    int found_tk_tcl = 0;
    int found_itcl_tcl = 0;
    int found_itk_tcl = 0;

    char pathsep[2] = { BU_PATH_SEPARATOR, '\0' };

    struct bu_vls initpath = BU_VLS_INIT_ZERO;
    struct bu_vls tcl = BU_VLS_INIT_ZERO;
    struct bu_vls itcl = BU_VLS_INIT_ZERO;
#ifdef HAVE_TK
    struct bu_vls tk = BU_VLS_INIT_ZERO;
    struct bu_vls itk = BU_VLS_INIT_ZERO;
    struct bu_vls iwidgets = BU_VLS_INIT_ZERO;
#endif

    if (!interp) {
	/* nothing to do */
	return;
    }

    /* If we are using an external Tcl, we need the
     * location of its init file */
    bu_vls_trunc(&initpath, 0);
#ifdef TCL_SYSTEM_INITTCL_PATH
    bu_vls_printf(&initpath, "set tcl_library {%s}", TCL_SYSTEM_INITTCL_PATH);
    if (Tcl_Eval(interp, bu_vls_addr(&initpath))) {
	bu_log("Problem initializaing tcl_library to system init.tcl path: Tcl_Eval ERROR:\n%s\n", Tcl_GetStringResult(interp));
    }
#endif

    bu_vls_printf(&tcl, "tcl%s", TCL_VERSION);
    bu_vls_printf(&itcl, "itcl%s", ITCL_VERSION);
#ifdef HAVE_TK
    bu_vls_printf(&tk, "tk%s", TK_VERSION);
    bu_vls_printf(&itk, "itk%s", ITCL_VERSION);
    bu_vls_printf(&iwidgets, "iwidgets%s", IWIDGETS_VERSION);
#endif

    root = bu_brlcad_root("", 1);
    bu_vls_printf(&root_buf, "%s", root);
    root = bu_vls_addr(&root_buf);
    data = bu_brlcad_root("share", 1);

    /* determine if TCLCAD_LIBRARY_PATH is set */
    library_path = getenv("TCLCAD_LIBRARY_PATH");
    if (library_path) {
	/* it is set, set auto_path. limit buf just because. */
	bu_vls_strncat(&auto_path, library_path, MAX_BUF);
    }

    /* make sure tcl_library path is in the auto_path */
    snprintf(buffer, MAX_BUF, "set tcl_library");
    Tcl_Eval(interp, buffer);
    bu_vls_strncat(&auto_path, Tcl_GetStringResult(interp), MAX_BUF);

    /* get string of invocation binary */
    which_argv = bu_which(bu_argv0_full_path());
    if (!which_argv) {
	which_argv = bu_argv0_full_path();
    }

    /* get name of installation binary */
    snprintf(buffer, MAX_BUF, "%s%cbin%c%s", root, BU_DIR_SEPARATOR, BU_DIR_SEPARATOR, bu_getprogname());

    /* are we running from an installed binary? if so add to path */
    if (bu_file_exists(buffer, NULL) && bu_file_same(buffer, which_argv)) {
	from_installed = 1;
	join_path(&auto_path, root, "lib", NULL);
	join_path(&auto_path, root, "lib", bu_vls_addr(&tcl), NULL);
#ifdef HAVE_TK
	join_path(&auto_path, root, "lib", bu_vls_addr(&tk), NULL);
#endif
	join_path(&auto_path, root, "lib", bu_vls_addr(&itcl), NULL);
#ifdef HAVE_TK
	join_path(&auto_path, root, "lib", bu_vls_addr(&itk), NULL);
	join_path(&auto_path, root, "lib", bu_vls_addr(&iwidgets), NULL);
#endif
	join_path(&auto_path, data, "tclscripts", NULL);
	join_path(&auto_path, data, "tclscripts", "lib", NULL);
	join_path(&auto_path, data, "tclscripts", "util", NULL);
	join_path(&auto_path, data, "tclscripts", "mged", NULL);
	join_path(&auto_path, data, "tclscripts", "geometree", NULL);
	join_path(&auto_path, data, "tclscripts", "graph", NULL);
	join_path(&auto_path, data, "tclscripts", "rtwizard", NULL);
	join_path(&auto_path, data, "tclscripts", "archer", NULL);
	join_path(&auto_path, data, "tclscripts", "boteditor", NULL);
	join_path(&auto_path, data, "tclscripts", "checker", NULL);
	join_path(&auto_path, data, "tclscripts", "lod", NULL);
    }

    /* be sure to check installation paths even if we aren't running from there */
    if (!from_installed) {
	join_path(&auto_path, root, "lib", NULL);
	join_path(&auto_path, root, "lib", bu_vls_addr(&tcl), NULL);
#ifdef HAVE_TK
	join_path(&auto_path, root, "lib", bu_vls_addr(&tk), NULL);
#endif
	join_path(&auto_path, root, "lib", bu_vls_addr(&itcl), NULL);
#ifdef HAVE_TK
	join_path(&auto_path, root, "lib", bu_vls_addr(&itk), NULL);
	join_path(&auto_path, root, "lib", bu_vls_addr(&iwidgets), NULL);
#endif
	join_path(&auto_path, data, "tclscripts", NULL);
	join_path(&auto_path, data, "tclscripts", "lib", NULL);
	join_path(&auto_path, data, "tclscripts", "util", NULL);
	join_path(&auto_path, data, "tclscripts", "mged", NULL);
	join_path(&auto_path, data, "tclscripts", "geometree", NULL);
	join_path(&auto_path, data, "tclscripts", "graph", NULL);
	join_path(&auto_path, data, "tclscripts", "rtwizard", NULL);
	join_path(&auto_path, data, "tclscripts", "archer", NULL);
	join_path(&auto_path, data, "tclscripts", "boteditor", NULL);
	join_path(&auto_path, data, "tclscripts", "checker", NULL);
	join_path(&auto_path, data, "tclscripts", "lod", NULL);
    }

    /*    printf("AUTO_PATH IS %s\n", bu_vls_addr(&auto_path)); */

    /* see if user already set ITCL_LIBRARY override */
    library_path = getenv("ITCL_LIBRARY");
    if (!found_itcl_tcl && library_path) {
	snprintf(buffer, MAX_BUF, "%s%citcl.tcl", library_path, BU_DIR_SEPARATOR);
	if (bu_file_exists(buffer, NULL)) {
	    found_itcl_tcl=1;
	}
    }

    /* see if user already set ITK_LIBRARY override */
    library_path = getenv("ITK_LIBRARY");
    if (!found_itk_tcl && library_path) {
	snprintf(buffer, MAX_BUF, "%s%citk.tcl", library_path, BU_DIR_SEPARATOR);
	if (bu_file_exists(buffer, NULL)) {
	    found_itk_tcl=1;
	}
    }

    /* iterate over the auto_path list and modify the real Tcl auto_path */
    for (srcpath = strtok(bu_vls_addr(&auto_path), pathsep);
	 srcpath;
	 srcpath = strtok(NULL, pathsep)) {

	/* make sure it exists before appending */
	if (bu_file_exists(srcpath, NULL)) {
	    /*		printf("APPENDING: %s\n", srcpath); */
	    bu_vls_sprintf(&lappend, "lappend auto_path {%s}", srcpath);
	    (void)Tcl_Eval(interp, bu_vls_addr(&lappend));
	} else {
	    /*		printf("NOT APPENDING: %s\n", srcpath); */
	    continue;
	}

	/* specifically look for init.tcl so we can set tcl_library */
	if (!found_init_tcl) {
	    snprintf(buffer, MAX_BUF, "%s%cinit.tcl", srcpath, BU_DIR_SEPARATOR);
	    if (bu_file_exists(buffer, NULL)) {
		/* this really sets it */
		snprintf(buffer, MAX_BUF, "set tcl_library {%s}", srcpath);
		if (Tcl_Eval(interp, buffer)) {
		    bu_log("Tcl_Eval ERROR:\n%s\n", Tcl_GetStringResult(interp));
		} else {
		    found_init_tcl=1;
		}

		/* extra measures necessary for "create interp":
		 * determine if TCL_LIBRARY is set, and set it if not.
		 */
		library_path = getenv("TCL_LIBRARY");
		if (!library_path) {
		    /* this REALLY sets it */
		    snprintf(buffer, MAX_BUF, "set env(TCL_LIBRARY) {%s}", srcpath);
		    if (Tcl_Eval(interp, buffer)) {
			bu_log("Tcl_Eval ERROR:\n%s\n", Tcl_GetStringResult(interp));
		    }
		}
	    }
	}

	/* specifically look for tk.tcl so we can set tk_library */
	if (!found_tk_tcl) {
	    snprintf(buffer, MAX_BUF, "%s%ctk.tcl", srcpath, BU_DIR_SEPARATOR);
	    if (bu_file_exists(buffer, NULL)) {
		/* this really sets it */
		snprintf(buffer, MAX_BUF, "set tk_library {%s}", srcpath);
		if (Tcl_Eval(interp, buffer)) {
		    bu_log("Tcl_Eval ERROR:\n%s\n", Tcl_GetStringResult(interp));
		} else {
		    found_tk_tcl=1;
		}
	    }
	}

	/* specifically look for itcl.tcl so we can set ITCL_LIBRARY */
	if (!found_itcl_tcl) {
	    snprintf(buffer, MAX_BUF, "%s%citcl.tcl", srcpath, BU_DIR_SEPARATOR);
	    if (bu_file_exists(buffer, NULL)) {
		/* this really sets it */
		snprintf(buffer, MAX_BUF, "set env(ITCL_LIBRARY) {%s}", srcpath);
		if (Tcl_Eval(interp, buffer)) {
		    bu_log("Tcl_Eval ERROR:\n%s\n", Tcl_GetStringResult(interp));
		} else {
		    found_itcl_tcl=1;
		}
	    }
	}

	/* specifically look for itk.tcl so we can set ITK_LIBRARY */
	if (!found_itk_tcl) {
	    snprintf(buffer, MAX_BUF, "%s%citk.tcl", srcpath, BU_DIR_SEPARATOR);
	    if (bu_file_exists(buffer, NULL)) {
		/* this really sets it */
		snprintf(buffer, MAX_BUF, "set env(ITK_LIBRARY) {%s}", srcpath);
		if (Tcl_Eval(interp, buffer)) {
		    bu_log("Tcl_Eval ERROR:\n%s\n", Tcl_GetStringResult(interp));
		} else {
		    found_itk_tcl=1;
		}
	    }
	}
    }

    which_argv = NULL;
    bu_vls_free(&tcl);
    bu_vls_free(&itcl);
#ifdef HAVE_TK
    bu_vls_free(&tk);
    bu_vls_free(&itk);
    bu_vls_free(&iwidgets);
#endif
    bu_vls_free(&auto_path);
    bu_vls_free(&lappend);
    bu_vls_free(&root_buf);

    return;
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
