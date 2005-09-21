/*
 * pngtcl.c --
 *
 *  Generic interface to XML parsers.
 *
 * Copyright (c) 2002 Andreas Kupries <andreas_kupries@users.sourceforge.net>
 *
 * Zveno Pty Ltd makes this software and associated documentation
 * available free of charge for any purpose.  You may make copies
 * of the software but you must include all of this notice on any copy.
 *
 * Zveno Pty Ltd does not warrant that this software is error free
 * or fit for any purpose.  Zveno Pty Ltd disclaims any liability for
 * all claims, expenses, losses, damages and costs any user may incur
 * as a result of using, copying or modifying the software.
 *
 * $Id$
 *
 */

#include "zlibtcl.h"
#include "pngtcl.h"

#define TCL_DOES_STUBS \
    (TCL_MAJOR_VERSION > 8 || TCL_MAJOR_VERSION == 8 && (TCL_MINOR_VERSION > 1 || \
    (TCL_MINOR_VERSION == 1 && TCL_RELEASE_LEVEL == TCL_FINAL_RELEASE)))

/*
 * Declarations for externally visible functions.
 */

#undef TCL_STORAGE_CLASS
#ifdef BUILD_pngtcl
# define TCL_STORAGE_CLASS DLLEXPORT
#else
# ifdef USE_PNGTCL_STUBS
#  define TCL_STORAGE_CLASS
# else
#  define TCL_STORAGE_CLASS DLLIMPORT
# endif
#endif

#ifdef _DEBUG
EXTERN int Png_d_Init     _ANSI_ARGS_((Tcl_Interp *interp));
EXTERN int Png_d_SafeInit _ANSI_ARGS_((Tcl_Interp *interp));
#else
EXTERN int Png_Init     _ANSI_ARGS_((Tcl_Interp *interp));
EXTERN int Png_SafeInit _ANSI_ARGS_((Tcl_Interp *interp));
#endif

#undef  TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

/*
 * Prototypes for procedures defined later in this file:
 */

/*
 *----------------------------------------------------------------------------
 *
 * Png_Init --
 *
 *  Initialisation routine for loadable module
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Creates commands in the interpreter,
 *  loads xml package.
 *
 *----------------------------------------------------------------------------
 */

int
#ifdef _DEBUG
Png_d_Init (interp)
#else
Png_Init (interp)
#endif
      Tcl_Interp *interp; /* Interpreter to initialise. */
{
#if TCL_DOES_STUBS
  extern PngtclStubs pngtclStubs;
#endif

#if 0
#ifdef USE_TCL_STUBS
  if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
    return TCL_ERROR;
  }
#endif
#ifdef USE_ZLIBTCL_STUBS
  if (Z_InitStubs(interp, ZLIBTCL_VERSION, 1) == NULL) {
    return TCL_ERROR;
  }
#endif
#endif

#if TCL_DOES_STUBS
  if (Tcl_PkgProvideEx(interp, PNG_PACKAGE_NAME, PNGTCL_VERSION,
		       (ClientData) &pngtclStubs) != TCL_OK) {
    return TCL_ERROR;
  }
#else
  if (Tcl_PkgProvide(interp, PNG_PACKAGE_NAME, PNGTCL_VERSION) != TCL_OK) {
    return TCL_ERROR;
  }
#endif

  return TCL_OK;
}

/*
 *----------------------------------------------------------------------------
 *
 * Png_SafeInit --
 *
 *  Initialisation routine for loadable module in a safe interpreter.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Creates commands in the interpreter,
 *  loads xml package.
 *
 *----------------------------------------------------------------------------
 */

int
#ifdef _DEBUG
Png_d_SafeInit (interp)
#else
Png_SafeInit (interp)
#endif
      Tcl_Interp *interp; /* Interpreter to initialise. */
{
    return Png_Init(interp);
}

/*
 *----------------------------------------------------------------------------
 *
 * Png_XXX --
 *
 *  Wrappers around the zlib functionality.
 *
 * Results:
 *  Depends on function.
 *
 * Side effects:
 *  Depends on function.
 *
 *----------------------------------------------------------------------------
 */

/*
 * No wrappers are required. Due to intelligent definition of the stub
 * table using the function names of the libz sources the stub table
 * contains jumps to the actual functionality.
 */
