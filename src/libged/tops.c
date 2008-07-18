/*                         T O P S . C
 * BRL-CAD
 *
 * Copyright (c) 2008 United States Government as represented by
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
/** @file tops.c
 *
 * The tops command.
 *
 */

#include "common.h"

#include <string.h>

#include "bio.h"
#include "cmd.h"
#include "ged_private.h"

static struct directory **
ged_dir_getspace(struct db_i	*dbip,
		 register int	num_entries);

int
ged_tops(struct ged *gedp, int argc, const char *argv[])
{
    register struct directory *dp;
    register int i;
    struct directory **dirp;
    struct directory **dirp0 = (struct directory **)NULL;
    int c;
#ifdef NEW_TOPS_BEHAVIOR
    int aflag = 0;
    int hflag = 0;
    int pflag = 0;
#else
    int gflag = 0;
    int uflag = 0;
#endif
    int no_decorate = 0;
#ifdef NEW_TOPS_BEHAVIOR
    static const char *usage = "[-a|-h|-n|-p]";
#else
    static const char *usage = "[-g|-n|-u]";
#endif

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);
    gedp->ged_result = GED_RESULT_NULL;
    gedp->ged_result_flags = 0;

    /* process any options */
    bu_optind = 1;	/* re-init bu_getopt() */
#ifdef NEW_TOPS_BEHAVIOR
    while ((c = bu_getopt(argc, (char * const *)argv, "ahnp")) != EOF) {
	switch (c) {
	    case 'a':
		aflag = 1;
		break;
	    case 'h':
		hflag = 1;
		break;
	    case 'n':
		no_decorate = 1;
		break;
	    case 'p':
		pflag = 1;
		break;
	    default:
		break;
	}
    }
#else
    while ((c = bu_getopt(argc, (char * const *)argv, "gun")) != EOF) {
	switch (c) {
	    case 'g':
		gflag = 1;
		break;
	    case 'u':
		uflag = 1;
		break;
	    case 'n':
		no_decorate = 1;
		break;
	    default:
		break;
	}
    }
#endif

    argc -= (bu_optind - 1);
    argv += (bu_optind - 1);

    /* Can this be executed only sometimes?
       Perhaps a "dirty bit" on the database? */
    db_update_nref(gedp->ged_wdbp->dbip, &rt_uniresource);

    /*
     * Find number of possible entries and allocate memory
     */
    dirp = ged_dir_getspace(gedp->ged_wdbp->dbip, 0);
    dirp0 = dirp;

    if (gedp->ged_wdbp->dbip->dbi_version < 5) {
	for (i = 0; i < RT_DBNHASH; i++)
	    for (dp = gedp->ged_wdbp->dbip->dbi_Head[i];
		 dp != DIR_NULL;
		 dp = dp->d_forw)  {
		if (dp->d_nref == 0)
		    *dirp++ = dp;
	    }
    } else {
	for (i = 0; i < RT_DBNHASH; i++)
	    for (dp = gedp->ged_wdbp->dbip->dbi_Head[i];
		 dp != DIR_NULL;
		 dp = dp->d_forw)  {
#ifdef NEW_TOPS_BEHAVIOR
		if (dp->d_nref == 0 &&
		    (aflag ||
		     (hflag && (dp->d_flags & DIR_HIDDEN)) ||
		     (pflag && dp->d_addr == RT_DIR_PHONY_ADDR) ||
		     (!aflag && !hflag && !pflag &&
		      !(dp->d_flags & DIR_HIDDEN) &&
		      (dp->d_addr != RT_DIR_PHONY_ADDR))))
		    *dirp++ = dp;
#else
		if (dp->d_nref == 0 &&
		    ((!gflag || (gflag && dp->d_major_type == DB5_MAJORTYPE_BRLCAD)) &&
		     (!uflag || (uflag && !(dp->d_flags & DIR_HIDDEN)))))
		    *dirp++ = dp;
#endif
	    }
    }

    ged_vls_col_pr4v(&gedp->ged_result_str, dirp0, (int)(dirp - dirp0), no_decorate);
    bu_free((genptr_t)dirp0, "wdb_tops_cmd: wdb_dir_getspace");

    return BRLCAD_OK;
}

/*
 *			G E D _ D I R _ G E T S P A C E
 *
 * This routine walks through the directory entry list and mallocs enough
 * space for pointers to hold:
 *  a) all of the entries if called with an argument of 0, or
 *  b) the number of entries specified by the argument if > 0.
 */
static struct directory **
ged_dir_getspace(struct db_i	*dbip,
		 register int	num_entries)
{
    register struct directory *dp;
    register int i;
    register struct directory **dir_basep;

    if (num_entries < 0) {
	bu_log( "dir_getspace: was passed %d, used 0\n",
		num_entries);
	num_entries = 0;
    }
    if (num_entries == 0) {
	/* Set num_entries to the number of entries */
	for (i = 0; i < RT_DBNHASH; i++)
	    for (dp = dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw)
		num_entries++;
    }

    /* Allocate and cast num_entries worth of pointers */
    dir_basep = (struct directory **) bu_malloc((num_entries+1) * sizeof(struct directory *),
						"dir_getspace *dir[]");
    return dir_basep;
}


/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
