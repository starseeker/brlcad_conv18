/*
 *			D B _ I O . C
 *
 * Functions -
 *	db_getmrec	Read all records into malloc()ed memory chunk
 *	db_get		Get records from database
 *	db_put		Put records to database
 *
 *
 *  Authors -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1988 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "externs.h"
#include "vmath.h"
#include "db.h"
#include "raytrace.h"

#include "./debug.h"

/*
 *  			D B _ G E T M R E C
 *
 *  Retrieve all records in the database pertaining to an object,
 *  and place them in malloc()'ed storage, which the caller is
 *  responsible for free()'ing.
 *
 *  Returns -
 *	union record *		OK
 *	(union record *)0	failure
 */
union record *
db_getmrec( dbip, dp )
struct db_i		*dbip;
CONST struct directory	*dp;
{
	union record	*where;

	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_getmrec:  bad dbip\n");
	if(rt_g.debug&DEBUG_DB) rt_log("db_getmrec(%s) x%x, x%x\n",
		dp->d_namep, dbip, dp );

	if( dp->d_addr < 0 )
		return( (union record *)0 );	/* was dummy DB entry */
	where = (union record *)rt_malloc(
		dp->d_len * sizeof(union record),
		"db_getmrec record[]");

	if( db_read( dbip, (char *)where,
	    (long)dp->d_len * sizeof(union record),
	    dp->d_addr ) < 0 )  {
		rt_free( (char *)where, "db_getmrec record[]" );
		return( (union record *)0 );	/* VERY BAD */
	}
	return( where );
}

/*
 *  			D B _ G E T
 *
 *  Retrieve 'len' records from the database,
 *  "offset" granules into this entry.
 *
 *  Returns -
 *	 0	OK
 *	-1	failure
 */
int
db_get( dbip, dp, where, offset, len )
struct db_i	*dbip;
CONST struct directory *dp;
union record	*where;
int		offset;
int		len;
{

	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_get:  bad dbip\n");
	if(rt_g.debug&DEBUG_DB) rt_log("db_get(%s) x%x, x%x x%x off=%d len=%d\n",
		dp->d_namep, dbip, dp, where, offset, len );

	if( dp->d_addr < 0 )  {
		where->u_id = '\0';	/* undefined id */
		return(-1);
	}
	if( offset < 0 || offset+len > dp->d_len )  {
		rt_log("db_get(%s):  xfer %d..%x exceeds 0..%d\n",
			dp->d_namep, offset, offset+len, dp->d_len );
		where->u_id = '\0';	/* undefined id */
		return(-1);
	}

	if( db_read( dbip, (char *)where, (long)len * sizeof(union record),
	    dp->d_addr + offset * sizeof(union record) ) < 0 )  {
		where->u_id = '\0';	/* undefined id */
		return(-1);
	}
	return(0);			/* OK */
}

/*
 *  			D B _ P U T
 *
 *  Store 'len' records to the database,
 *  "offset" granules into this entry.
 *
 *  Returns -
 *	 0	OK
 *	-1	failure
 */
int
db_put( dbip, dp, where, offset, len )
struct db_i	*dbip;
CONST struct directory *dp;
union record	*where;
int		offset;
int		len;
{

	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_put:  bad dbip\n");
	if(rt_g.debug&DEBUG_DB) rt_log("db_put(%s) x%x, x%x x%x off=%d len=%d\n",
		dp->d_namep, dbip, dp, where, offset, len );

	if( dbip->dbi_read_only )  {
		rt_log("db_put(%s):  READ-ONLY file\n",
			dbip->dbi_filename);
		return(-1);
	}
	if( offset < 0 || offset+len > dp->d_len )  {
		rt_log("db_put(%s):  xfer %d..%x exceeds 0..%d\n",
			dp->d_namep, offset, offset+len, dp->d_len );
		return(-1);
	}

	if( db_write( dbip, (char *)where, (long)len * sizeof(union record),
	    dp->d_addr + offset * sizeof(union record) ) < 0 )  {
		return(-1);
	}
	return(0);
}

/*
 *			D B _ R E A D
 *
 *  Reads 'count' bytes at file offset 'offset' into buffer at 'addr'.
 *  A wrapper for the UNIX read() sys-call that takes into account
 *  syscall semaphores, stdio-only machines, and in-memory buffering.
 *
 *  Returns -
 *	 0	OK
 *	-1	failure
 */
int
db_read( dbip, addr, count, offset )
struct db_i	*dbip;
genptr_t	addr;
long		count;		/* byte count */
long		offset;		/* byte offset from start of file */
{
	register int	got;
	register long	s;

	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_read:  bad dbip\n");
	if(rt_g.debug&DEBUG_DB)  {
		rt_log("db_read(dbip=x%x, addr=x%x, count=%d., offset=%d.)\n",
			dbip, addr, count, offset );
	}
	if( count <= 0 || offset < 0 )  {
		return(-1);
	}
	if( dbip->dbi_inmem )  {
		if( offset+count > dbip->dbi_eof )  {
			/* Attempt to read off the end of the (mapped) file */
			rt_log("db_read(%s) ERROR offset=%d, count=%d, dbi_eof=%d\n",
				dbip->dbi_filename,
				offset, count, dbip->dbi_eof );
			return -1;
		}
		memcpy( addr, dbip->dbi_inmem + offset, count );
		return(0);
	}
	RES_ACQUIRE( &rt_g.res_syscall );
#ifdef HAVE_UNIX_IO
	if ((s=(long)lseek( dbip->dbi_fd, (off_t)offset, 0 )) != offset) {
		rt_log("db_read: lseek returns %d not %d\n", s, offset);
		rt_bomb("Goodbye");
	}
	got = read( dbip->dbi_fd, addr, count );
#else
	if (fseek( dbip->dbi_fp, offset, 0 ))
		rt_bomb("db_read: fseek error\n");
	got = fread( addr, 1, count, dbip->dbi_fp );
#endif
	RES_RELEASE( &rt_g.res_syscall );

	if( got != count )  {
		perror("db_read");
		rt_log("db_read(%s):  read error.  Wanted %d, got %d bytes\n",
			dbip->dbi_filename, count, got );
		return(-1);
	}
	return(0);			/* OK */
}

/*
 *			D B _ W R I T E
 *
 *  Writes 'count' bytes into at file offset 'offset' from buffer at 'addr'.
 *  A wrapper for the UNIX write() sys-call that takes into account
 *  syscall semaphores, stdio-only machines, and in-memory buffering.
 *
 *  Returns -
 *	 0	OK
 *	-1	failure
 */
int
db_write( dbip, addr, count, offset )
struct db_i	*dbip;
CONST genptr_t	addr;
long		count;
long		offset;
{
	register int	got;

	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_write:  bad dbip\n");
	if(rt_g.debug&DEBUG_DB)  {
		rt_log("db_write(dbip=x%x, addr=x%x, count=%d., offset=%d.)\n",
			dbip, addr, count, offset );
	}
	if( dbip->dbi_read_only )  {
		rt_log("db_write(%s):  READ-ONLY file\n",
			dbip->dbi_filename);
		return(-1);
	}
	if( count <= 0 || offset < 0 )  {
		return(-1);
	}
	if( dbip->dbi_inmem )  {
		rt_log("db_write() in memory?\n");
		return(-1);
	}
	RES_ACQUIRE( &rt_g.res_syscall );
#ifdef HAVE_UNIX_IO
	(void)lseek( dbip->dbi_fd, offset, 0 );
	got = write( dbip->dbi_fd, addr, count );
#else
	(void)fseek( dbip->dbi_fp, offset, 0 );
	got = fwrite( addr, 1, count, dbip->dbi_fp );
#endif
	RES_RELEASE( &rt_g.res_syscall );
	if( got != count )  {
		perror("db_write");
		rt_log("db_write(%s):  write error.  Wanted %d, got %d bytes\n",
			dbip->dbi_filename, count, got );
		return(-1);
	}
	return(0);			/* OK */
}

/*
 *			D B _ G E T _ E X T E R N A L
 *
 *  Returns -
 *	-1	error
 *	 0	success
 */
int
db_get_external( ep, dp, dbip )
register struct rt_external	*ep;
CONST struct directory		*dp;
struct db_i			*dbip;
{
	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_get_external:  bad dbip\n");
	if(rt_g.debug&DEBUG_DB) rt_log("db_get_external(%s) ep=x%x, dbip=x%x, dp=x%x\n",
		dp->d_namep, ep, dbip, dp );

	if( dp->d_addr < 0 )
		return( -1 );		/* was dummy DB entry */

	RT_INIT_EXTERNAL(ep);
	ep->ext_nbytes = dp->d_len * sizeof(union record);
	ep->ext_buf = (genptr_t)rt_malloc(
		ep->ext_nbytes, "db_get_ext ext_buf");

	if( db_read( dbip, (char *)ep->ext_buf,
	    (long)ep->ext_nbytes, dp->d_addr ) < 0 )  {
		rt_free( (char *)ep->ext_buf, "db_get_ext ext_buf" );
	    	ep->ext_buf = (genptr_t)NULL;
	    	ep->ext_nbytes = 0;
		return( -1 );	/* VERY BAD */
	}
	return(0);
}

/*
 *
 *			D B _ P U T _ E X T E R N A L
 *
 *  Add name from dp->d_namep to external representation of solid,
 *  and write it into the database, obtaining different storage if
 *  the size has changed since last write.
 *
 *  Caller is responsible for freeing memory of external representation,
 *  using db_free_external().
 */
int
db_put_external( ep, dp, dbip )
struct rt_external	*ep;
struct directory	*dp;
struct db_i		*dbip;
{
	union record		*rec;
	int	ngran;

	if( dbip->dbi_magic != DBI_MAGIC )  rt_bomb("db_put_external:  bad dbip\n");
	if(rt_g.debug&DEBUG_DB) rt_log("db_put_external(%s) ep=x%x, dbip=x%x, dp=x%x\n",
		dp->d_namep, ep, dbip, dp );

	RT_CK_EXTERNAL(ep);

	ngran = (ep->ext_nbytes+sizeof(union record)-1)/sizeof(union record);
	if( ngran != dp->d_len )  {
		if( ngran < dp->d_len )  {
			if( db_trunc( dbip, dp, dp->d_len - ngran ) < 0 )
			    	return(-2);
		} else if( ngran > dp->d_len )  {
			if( db_delete( dbip, dp ) < 0 || 
			    db_alloc( dbip, dp, ngran ) < 0 )  {
			    	return(-3);
			}
		}
	}
	/* Sanity check */
	if( ngran != dp->d_len )  {
		rt_log("db_put_external(%s) ngran=%d != dp->d_len %d\n",
			dp->d_namep, ngran, dp->d_len );
		rt_bomb("db_io.c: db_put_external()");
	}

	/* Add name.  Depends on solid names always being in the same place */
	rec = (union record *)ep->ext_buf;
	NAMEMOVE( dp->d_namep, rec->s.s_name );

	if( db_put( dbip, dp, (union record *)(ep->ext_buf), 0, ngran ) < 0 )
		return(-1);
	return(0);
}

/*
 *			D B _ F R E E _ E X T E R N A L
 */
void
db_free_external( ep )
register struct rt_external	*ep;
{
	RT_CK_EXTERNAL(ep);
	if( ep->ext_buf )  {
		rt_free( ep->ext_buf, "db_get_ext ext_buf" );
		ep->ext_buf = GENPTR_NULL;
	}
}
