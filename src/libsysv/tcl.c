/*
 * Author :
 *        Bob Parker (SURVICE Engineering Company)
 *
 * Description:
 *        This file contains the Tcl initialization routine for libsysv.
 */

#include "common.h"

#include "tcl.h"
#include <stdio.h>
#include "machine.h"


#ifndef SYSV_EXPORT
#  if defined(WIN32) && !defined(__CYGWIN__)
#    ifdef SYSV_EXPORT_DLL
#      define SYSV_EXPORT __declspec(dllexport)
#    else
#      define SYSV_EXPORT __declspec(dllimport)
#    endif
#  else
#    define SYSV_EXPORT
#  endif
#endif


SYSV_EXPORT int
Sysv_Init(Tcl_Interp *interp)
{
    return TCL_OK;
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
