/*
 *			B O D Y I O . C
 *
 * Functions -
 *	bodyread - read an object's body from a file
 *	bodywrite - write an object's body to a file
 *
 *  Author -
 *	Paul J. Tanenbaum
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Pacakge" agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 2000 by the United States Army
 *	in all countries except the USA.  All rights reserved.

 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>
#include <pwd.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "db5.h"
#include "bn.h"
#include "nmg.h"
#include "raytrace.h"
#include "externs.h"
#include "./ged.h"
#include "./sedit.h"

/*
 *		C M D _ B O D Y R E A D ( )
 *
 *	Read an object's body from disk file
 *
 */
int
cmd_bodyread(clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int	argc;
char	*argv[];
{
    register struct directory	*dp;
    struct bu_external		ext;
    struct db5_raw_internal	raw;
    struct stat			stat_buf;
    unsigned char		major_code, minor_code;
    char			*foo;
    struct bu_vls		vls;

    CHECK_DBI_NULL;

    if(argc != 4){
      struct bu_vls vls;

      bu_vls_init(&vls);
      bu_vls_printf(&vls, "help %s", argv[0]);
      Tcl_Eval(interp, bu_vls_addr(&vls));
      bu_vls_free(&vls);
      return TCL_ERROR;
    }

    switch (sscanf(argv[3], "%d %d", &major_code, &minor_code)) {
	case 1:
	    /* XXX is it safe to have no minor code? */
	    minor_code = 0;
	case 2:
	    break;
	case 0:
	    if ( db5_type_codes_from_descrip(&major_code, &minor_code,
						argv[3])
	     && db5_type_codes_from_tag(&major_code, &minor_code,
						argv[3])) {
		bu_vls_init( &vls );
		bu_vls_printf( &vls,
				"Invalid data type: 'tomato'\n" );
				/*"Invalid data type: '%s'\n", argv[3] );*/
		Tcl_SetResult(interp, bu_vls_addr( &vls ), TCL_VOLATILE );
		bu_vls_free( &vls );
		mged_print_result( TCL_ERROR );
		return;
	    }
	    break;
    }
    bu_vls_init( &vls );
    if (db5_type_descrip_from_codes( &foo, major_code, minor_code )) {
	bu_vls_printf( &vls,
			"Invalid maj/min: %d %d\n",
			major_code, minor_code);
	Tcl_SetResult(interp, bu_vls_addr( &vls ), TCL_VOLATILE );
	bu_vls_free( &vls );
	mged_print_result( TCL_ERROR );
	return;
    }
    bu_vls_printf( &vls, "Type is %d %d '%s'\n", major_code, minor_code, foo);
    Tcl_SetResult(interp, bu_vls_addr( &vls ), TCL_VOLATILE );
    bu_vls_free( &vls );
    mged_print_result( TCL_OK );
    return;

#if 0
    /*
     *	Check to see if we need to create a new object
     */
    if( (dp = db_lookup( dbip, argv[1], LOOKUP_NOISY)) == DIR_NULL ) {
	/* Update the in-core directory */
	if( (dp = db_diradd( dbip, argv[1], -1, 0, 0, NULL ))
		== DIR_NULL
	 || db_put ( dbip, dp, XXX, 0, 1) < 0 )  {
	  Tcl_AppendResult(interp, "An error has occured while adding '",
			   argv[1], "' to the database.\n", (char *)NULL);
	  TCL_ERROR_RECOVERY_SUGGESTION;
	  return DIR_NULL;
	}

	BU_GETSTRUCT( comb, rt_comb_internal );
	comb->magic = RT_COMB_MAGIC;
	bu_vls_init( &comb->shader );
	bu_vls_init( &comb->material );
	comb->region_id = -1;
	comb->tree = TREE_NULL;

	RT_INIT_DB_INTERNAL( &intern );
	intern.idb_type = ID_COMBINATION;
	intern.idb_meth = &rt_functab[ID_COMBINATION];
	intern.idb_ptr = (genptr_t)comb;

    }

    /*
     *	How much data do we have to suck in?
     */
    if( stat( argv[2], &stat_buf ) ) {
	bu_vls_init( &vls );
	bu_vls_printf( &vls, "Cannot get status of file %s\n", argv[2] );
	Tcl_SetResult(interp, bu_vls_addr( &vls ), TCL_VOLATILE );
	bu_vls_free( &vls );
	mged_print_result( TCL_ERROR );
	return;
    }
    /* stat_buf.st_size */
    (void)signal( SIGINT, SIG_IGN );
#endif

    return TCL_OK;
}

/*
 *		C M D _ B O D Y W R I T E ( )
 *
 *	Write an object's body to disk file
 *
 */
int
cmd_bodywrite(clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int	argc;
char	*argv[];
{
    register struct directory	*dp;
    int				fd;
    void			*bufp;
    size_t			nbytes;
    int				written;
    struct bu_external		ext;
    struct db5_raw_internal	raw;
    struct rt_db_internal	intern;
    struct stat			stat_buf;
    struct rt_binunif_internal	*bip;
    struct bu_vls		vls;
    int				status;

    CHECK_DBI_NULL;

    if(argc != 4){
      bu_vls_init(&vls);
      bu_vls_printf(&vls, "help %s", argv[0]);
      Tcl_Eval(interp, bu_vls_addr(&vls));
      bu_vls_free(&vls);
      return TCL_ERROR;
    }

    /*
     *	Find the guy we're told to write
     */
    if( (dp = db_lookup( dbip, argv[2], LOOKUP_NOISY)) != DIR_NULL ){
	bu_vls_init( &vls );
	bu_vls_printf( &vls,
	    "Cannot find object %s for writing\n", argv[1] );
	Tcl_SetResult(interp, bu_vls_addr( &vls ), TCL_VOLATILE );
	bu_vls_free( &vls );
	mged_print_result( TCL_ERROR );
	return;
    }
    if ( db_get_external( &ext, dp, dbip ) < 0 )
    {
	(void)signal( SIGINT, SIG_IGN );
	TCL_READ_ERR_return;
    }
    if ( db5_get_raw_internal_ptr( &raw, ext.ext_buf ) < 0 )
    {
	bu_free_external( &ext );
	(void)signal( SIGINT, SIG_IGN );
	TCL_READ_ERR_return;
    }

    /*
     *	Do the writing
     */
    if( (fd = creat( argv[1], S_IRWXU | S_IRGRP | S_IROTH  )) == -1 ) {
	bu_free_external( &ext );
	bu_vls_init( &vls );
	bu_vls_printf( &vls,
	    "Cannot open file %s for writing\n", argv[1] );
	Tcl_SetResult(interp, bu_vls_addr( &vls ), TCL_VOLATILE );
	bu_vls_free( &vls );
	mged_print_result( TCL_ERROR );
	return;
    }
    switch (raw.major_type) {
	case DB5_MAJORTYPE_BINARY_UNIF:
	    if ( rt_binunif_import5( &intern,
			raw.minor_type, &ext, dbip ) ) {
		(void)signal( SIGINT, SIG_IGN );
		TCL_READ_ERR_return;
		
	    }
	    bip = (struct rt_binunif_internal *) intern.idb_ptr;
	    bufp = (void *) bip->u.uint8;
	    switch (bip -> type) {
		case DB5_MINORTYPE_BINU_FLOAT:
		    nbytes = (size_t) (bip->count * sizeof(float));
		    break;
		case DB5_MINORTYPE_BINU_DOUBLE:
		    nbytes = (size_t) (bip->count * sizeof(double));
		    break;
		case DB5_MINORTYPE_BINU_8BITINT:
		case DB5_MINORTYPE_BINU_8BITINT_U:
		    nbytes = (size_t) (bip->count);
		    break;
		case DB5_MINORTYPE_BINU_16BITINT:
		case DB5_MINORTYPE_BINU_16BITINT_U:
		    nbytes = (size_t) (bip->count * 2);
		    break;
		case DB5_MINORTYPE_BINU_32BITINT:
		case DB5_MINORTYPE_BINU_32BITINT_U:
		    nbytes = (size_t) (bip->count * 4);
		    break;
		case DB5_MINORTYPE_BINU_64BITINT:
		case DB5_MINORTYPE_BINU_64BITINT_U:
		    nbytes = (size_t) (bip->count * 8);
		    break;
	    }
	    break;
	default:
	    bufp = (void *) ext.ext_buf;
	    nbytes = (size_t) ext.ext_nbytes;
	    break;
    }
    if ( (written = write(fd, bufp, nbytes) ) != nbytes ) {
	bu_free_external( &ext );
	bu_vls_init( &vls );
	bu_vls_printf( &vls,
	    "Incomplete write of object %s to file %s\n",
	    argv[2], argv[1] );
	Tcl_SetResult(interp, bu_vls_addr( &vls ), TCL_VOLATILE );
	bu_vls_free( &vls );
	mged_print_result( TCL_ERROR );
	return;
    }

    bu_free_external( &ext );
    return TCL_OK;
}
