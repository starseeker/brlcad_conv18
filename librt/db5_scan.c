/*
 *			D B 5 _ S C A N . C
 *
 *  Scan a v5 database, sending each object off to a handler.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimited.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (ARL)";
#endif

#include "conf.h"

#include <stdio.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "db5.h"
#include "raytrace.h"

#include "./debug.h"

/*
 *			D B 5 _ S C A N
 *
 * Returns -
 *	 0	Success
 *	-1	Fatal Error
 */
int
db5_scan( dbip, handler, client_data )
register struct db_i	*dbip;
void			(*handler)BU_ARGS((struct db_i *,
			CONST struct db5_raw_internal *,
			long addr, genptr_t client_data));
genptr_t		client_data;	/* argument for handler */
{
	unsigned char	header[8];
	struct db5_raw_internal	raw;
	int			got;
	long			nrec;
	long			addr;

	RT_CK_DBI(dbip);
	if(rt_g.debug&DEBUG_DB) bu_log("db5_scan( x%x, x%x )\n", dbip, handler);

	raw.magic = DB5_RAW_INTERNAL_MAGIC;
	nrec = 0L;

	/* Fast-path when file is already memory-mapped */
	if( dbip->dbi_mf )  {
		CONST unsigned char	*cp = (CONST unsigned char *)dbip->dbi_inmem;
		long	eof;

		BU_CK_MAPPED_FILE(dbip->dbi_mf);
		eof = dbip->dbi_mf->buflen;

		if( db5_header_is_valid( cp ) == 0 )  {
			bu_log("db5_scan ERROR:  %s is lacking a proper BRL-CAD v5 database header\n", dbip->dbi_filename);
		    	return -1;
		}
		cp += sizeof(header);
		addr = sizeof(header);
		while( addr < eof )  {
			if( (cp = db5_get_raw_internal_ptr( &raw, cp )) == NULL )  {
				return -1;			/* fatal error */
			}
			(*handler)(dbip, &raw, addr, client_data);
			nrec++;
			addr += raw.object_length;
		}
		dbip->dbi_eof = addr;
		BU_ASSERT_LONG( dbip->dbi_eof, ==, dbip->dbi_mf->buflen );
	}  else  {
		/* In a totally portable way, read the database with stdio */
		rewind( dbip->dbi_fp );
		if( fread( header, sizeof header, 1, dbip->dbi_fp ) != 1  ||
		    db5_header_is_valid( header ) == 0 )  {
			bu_log("db5_scan ERROR:  %s is lacking a proper BRL-CAD v5 database header\n", dbip->dbi_filename);
		    	return -1;
		}
		for(;;)  {
			addr = ftell( dbip->dbi_fp );
			if( (got = db5_get_raw_internal_fp( &raw, dbip->dbi_fp )) < 0 )  {
				if( got == -1 )  break;		/* EOF */
				return -1;			/* fatal error */
			}
			(*handler)(dbip, &raw, addr, client_data);
			nrec++;
			if(raw.buf)  {
				bu_free(raw.buf, "raw v5 object");
				raw.buf = NULL;
			}
		}
		dbip->dbi_eof = ftell( dbip->dbi_fp );
		rewind( dbip->dbi_fp );
	}

	dbip->dbi_nrec = nrec;		/* # obj in db, not inc. header */

	return 0;			/* success */
}

/*
 *			D B 5 _ D I R A D D _ H A N D L E R
 *
 * In support of db5_scan, add a named entry to the directory.
 */
void
db5_diradd_handler( dbip, rip, laddr, client_data )
register struct db_i	*dbip;
CONST struct db5_raw_internal *rip;
long			laddr;
genptr_t		client_data;	/* unused client_data from db_scan() */
{
	register struct directory **headp;
	register struct directory *dp;
	struct bu_vls	local;
	char		*cp = NULL;

	RT_CK_DBI(dbip);

	if( rip->h_dli == DB5HDR_HFLAGS_DLI_HEADER_OBJECT )  return;
	if( rip->h_dli == DB5HDR_HFLAGS_DLI_FREE_STORAGE )  {
		/* Record available free storage */
		rt_memfree( &(dbip->dbi_freep), rip->object_length, laddr );
		return;
	}
	
	/* If somehow it doesn't have a name, ignore it */
	if( rip->name.ext_buf == NULL )  return;

	if(rt_g.debug&DEBUG_DB)  {
		bu_log("db5_diradd_handler(dbip=x%x, name='%s', addr=x%x, len=%d)\n",
			dbip, rip->name, laddr, rip->object_length );
	}

	if( db_lookup( dbip, rip->name.ext_buf, LOOKUP_QUIET ) != DIR_NULL )  {
		register int	c;

		bu_vls_init(&local);
		bu_vls_strcpy( &local, "A_" );
		bu_vls_strcat( &local, rip->name.ext_buf );
		cp = bu_vls_addr(&local);

		for( c = 'A'; c <= 'Z'; c++ )  {
			*cp = c;
			if( db_lookup( dbip, cp, 0 ) == DIR_NULL )
				break;
		}
		if( c > 'Z' )  {
			bu_log("db5_diradd_handler: Duplicate of name '%s', ignored\n",
				cp );
			bu_vls_free(&local);
			return;
		}
		bu_log("db5_diradd_handler: Duplicate of '%s', given temporary name '%s'\n",
			rip->name, cp );
	}

	BU_GETSTRUCT( dp, directory );
	dp->d_magic = RT_DIR_MAGIC;
	BU_LIST_INIT( &dp->d_use_hd );
	if( cp )  {
		dp->d_namep = bu_strdup( cp );
		bu_vls_free( &local );
	} else {
		dp->d_namep = bu_strdup( rip->name.ext_buf );
	}
	dp->d_un.file_offset = laddr;
	switch( rip->major_type )  {
	case DB5_MAJORTYPE_BRLCAD:
		if( rip->minor_type == ID_COMBINATION )  {
			struct bu_attribute_value_set	avs;

			dp->d_flags = DIR_COMB;
			if( rip->attributes.ext_nbytes == 0 )  break;
			/*
			 *  Crack open the attributes to
			 *  check for the "region=" attribute.
			 */
			if( db5_import_attributes( &avs, &rip->attributes ) < 0 )  {
				bu_log("db5_diradd_handler: Bad attributes on combination '%s'\n",
					rip->name);
				break;
			}
			if( bu_avs_get( &avs, "region" ) != NULL )
				dp->d_flags = DIR_COMB|DIR_REGION;
			bu_avs_free( &avs );
		} else {
			dp->d_flags = DIR_SOLID;
		}
		break;
	case DB5_MAJORTYPE_BINARY_EXPM:
	case DB5_MAJORTYPE_BINARY_UNIF:
	case DB5_MAJORTYPE_BINARY_MIME:
		/* XXX Do we want to define extra flags for this? */
		dp->d_flags = 0;
		break;
	case DB5_MAJORTYPE_ATTRIBUTE_ONLY:
		dp->d_flags = 0;
	}
	dp->d_len = rip->object_length;		/* in bytes */

	headp = &(dbip->dbi_Head[db_dirhash(dp->d_namep)]);
	dp->d_forw = *headp;
	*headp = dp;

	return;
}

/*
 *			D B _ D I R B U I L D
 *
 *  A generic routine to determine the type of the database,
 *  and to invoke the appropriate db_scan()-like routine to
 *  build the in-memory directory.
 *
 *  It is the caller's responsibility to close the database in case of error.
 *
 *  Called from rt_dirbuild(), and g_submodel.c
 *
 *  Returns -
 *	0	OK
 *	-1	failure
 */
db_dirbuild( dbip )
struct db_i	*dbip;
{
	char	header[8];

	RT_CK_DBI(dbip);

	/* First, determine what version database this is */
	rewind(dbip->dbi_fp);
	if( fread( header, sizeof header, 1, dbip->dbi_fp ) != 1  )  {
		bu_log("db_dirbuild(%s) ERROR, file too short to be BRL-CAD database\n",
			dbip->dbi_filename);
		return -1;
	}

	if( db5_header_is_valid( header ) )  {
		struct directory	*dp;
		struct rt_db_internal	intern;

		/* File is v5 format */
bu_log("WARNING:  %s is BRL-CAD v5 format.\nWARNING:  You probably need a newer version of this program to read it.\n", dbip->dbi_filename);
		dbip->dbi_version = 5;
		if( db5_scan( dbip, db5_diradd_handler, NULL ) < 0 )  {
			bu_log("db_dirbuild(%s): db5_scan() failed\n",
				dbip->dbi_filename);
			return -1;
		}
		/* Need to retrieve _GLOBAL object and obtain title and units */
		if( (dp = db_lookup( dbip, DB5_GLOBAL_OBJECT_NAME, LOOKUP_NOISY )) == DIR_NULL )  {
			bu_log("db_dirbuild(%s): improper v5 database, no %s object\n",
				dbip->dbi_filename, DB5_GLOBAL_OBJECT_NAME );
			dbip->dbi_title = bu_strdup(DB5_GLOBAL_OBJECT_NAME);
			return 0;	/* not a fatal error, user may have deleted it */
		}
		if( rt_db_get_internal( &intern, dp, dbip, NULL ) < 0 )  {
			bu_log("db_dirbuild(%s): improper v5 database, corrupted %s object\n",
				dbip->dbi_filename, DB5_GLOBAL_OBJECT_NAME );
			return -1;	/* this is fatal */
		}
		BU_CK_AVS( &intern.idb_avs );
bu_avs_print( &intern.idb_avs, DB5_GLOBAL_OBJECT_NAME );
return -1;
	}

	/* Make a very simple check for a v4 database */
	if( header[0] == 'I' )  {
		dbip->dbi_version = 4;
		if( db_scan( dbip, (int (*)())db_diradd, 1, NULL ) < 0 )  {
			dbip->dbi_version = 0;
		    	return -1;
		}
		return 0;		/* ok */
	}

	bu_log("db_dirbuild(%s) ERROR, file is not in BRL-CAD geometry database format\n",
		dbip->dbi_filename);
	return -1;
}
