#include "conf.h"

#include <stdio.h>

#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "raytrace.h"
#include "rtgeom.h"



int
main(argc, argv)
int	argc;
char	**argv;
{
	FILE	*fp;
	struct db_i	*dbip;
	struct directory	*dp;
	int	i;
	long	errors = 0;
	struct bn_tol tol;

        /* XXX These need to be improved */
        tol.magic = BN_TOL_MAGIC;
        tol.dist = 0.005;
        tol.dist_sq = tol.dist * tol.dist;
        tol.perp = 1e-6;
        tol.para = 1 - tol.perp;

	bu_debug = BU_DEBUG_COREDUMP;

	if( argc != 3 )  {
		fprintf(stderr, "Usage: %s v4.g v5.g\n", argv[0]);
		return 1;
	}

	if( (dbip = db_open( argv[1], "r" )) == DBI_NULL )  {
		perror( argv[1] );
		return 2;
	}

	if( (fp = fopen( argv[2], "w" )) == NULL )  {
		perror( argv[2] );
		return 3;
	}

	RT_CK_DBI(dbip);
	db_dirbuild( dbip );

	db5_fwrite_ident( fp, dbip->dbi_title, dbip->dbi_local2base );

	/* Retrieve every item in the input database */
	for( i = RT_DBNHASH-1; i >= 0; i-- )  {
		for( dp = dbip->dbi_Head[i]; dp; dp = dp->d_forw )  {
			struct rt_db_internal	intern;
			int id;
			int ret;

			fprintf(stderr, "%s\n", dp->d_namep );

			id = rt_db_get_internal( &intern, dp, dbip, NULL );
			if( id < 0 )  {
				fprintf(stderr,
					"%s: rt_db_get_internal(%s) failure, skipping\n",
					argv[0], dp->d_namep);
				errors++;
				continue;
			}
			if ( id == ID_HF ) {
				if (hf_to_dsp( &intern )) {
					fprintf(stderr,
						"%s: Conversion from HF to DSP failed for solid %s\n",
						argv[0], dp->d_namep );
					errors++;
					continue;
				}
			}
			if( id == ID_POLY)
			{
				if( pg_bot( &intern, &tol ) )
				{
					fprintf( stderr, "%s: Conversion from polysolid to BOT failed for solid %s\n",
						argv[0], dp->d_namep );
					errors++;
					continue;
				}
			}
			ret = rt_fwrite_internal5( fp, dp->d_namep, &intern, 1.0, NULL );
			if( ret < 0 )  {
				fprintf(stderr,
					"%s: rt_fwrite_internal5(%s) failure, skipping\n",
					argv[0], dp->d_namep);
				rt_db_free_internal( &intern );
				errors++;
				continue;
			}
			rt_db_free_internal( &intern );
		}
	}

	fclose( fp );
	db_close( dbip );

	fprintf(stderr, "%d objects failed to convert\n", errors);
	return 0;
}
