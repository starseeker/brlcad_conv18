/*                         B U R S T . C
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
/** @file burst/burst.c
 *
 */

#include "common.h"

#include <assert.h>
#include <stdio.h>
#include <signal.h>

#include "bu/app.h"
#include "bu/getopt.h"
#include "bu/file.h"
#include "bu/opt.h"
#include "bu/str.h"
#include "bu/exit.h"
#include "bu/log.h"
#include "bu/vls.h"

#include "./burst.h"
#include "./trie.h"
#include "./extern.h"
#include "./ascii.h"


#define DEBUG_BURST 0 /* 1 enables debugging for this module */

/*
  int getCommand(char *name, char *buf, int len, FILE *fp)

  Read next command line into buf and stuff the command name into name
  from input stream fp.  buf must be at least len bytes long.

  RETURN:	1 for success

  0 for end of file
*/
static int
getCommand(char *name, char *buf, int len, FILE *fp)
{
    assert(name != NULL);
    assert(buf != NULL);
    assert(fp != NULL);
    while (bu_fgets(buf, len, fp) != NULL) {
	if (buf[0] != CHAR_COMMENT) {
	    if (sscanf(buf, "%1330s", name) == 1) {
		/* LNBUFSZ */
		buf[strlen(buf)-1] = NUL; /* clobber newline */
		return 1;
	    } else /* Skip over blank lines. */
		continue;
	} else {
	    /* Generate comment command. */
	    bu_strlcpy(name, CMD_COMMENT, LNBUFSZ);
	    return 1;
	}
    }
    return 0; /* EOF */
}


/*
  void setupSigs(void)

  Initialize all signal handlers.
*/

static void
setupSigs(void)
{
    int i;
    for (i = 0; i < NSIG; i++)
	switch (i) {
	    case SIGINT :
		if ((norml_sig = signal(i, SIG_IGN)) == SIG_IGN)
		    abort_sig = SIG_IGN;
		else {
		    norml_sig = intr_sig;
		    abort_sig = abort_RT;
		    (void) signal(i,  norml_sig);
		}
		break;
#ifdef SIGPIPE
	    case SIGPIPE :
		(void) signal(i, SIG_IGN);
		break;
#endif
	    default:
		break;
	}
    return;
}

/*
  void readBatchInput(FILE *fp)

  Read and execute commands from input stream fp.
*/
void
readBatchInput(FILE *fp)
{
    assert(fp != (FILE *) NULL);
    batchmode = 1;
    while (getCommand(cmdname, cmdbuf, LNBUFSZ, fp)) {
	Func *cmdfunc;
	if ((cmdfunc = getTrie(cmdname, cmdtrie)) == NULL) {
	    int i, len = strlen(cmdname);
	    brst_log("ERROR -- command syntax:\n");
	    brst_log("\t%s\n", cmdbuf);
	    brst_log("\t");
	    for (i = 0; i < len; i++)
		brst_log(" ");
	    brst_log("^\n");
	} else
	    if (BU_STR_EQUAL(cmdname, CMD_COMMENT)) {
		/* special handling for comments */
		cmdptr = cmdbuf;
		cmdbuf[strlen(cmdbuf)-1] = '\0'; /* clobber newline */
		(*cmdfunc)((HmItem *) 0);
	    } else {
		/* Advance pointer past nul at end of
		   command name. */
		cmdptr = cmdbuf + strlen(cmdname) + 1;
		(*cmdfunc)((HmItem *) 0);
	    }
    }
    batchmode = 0;
    return;
}

static const char usage[] =
    "Usage: burst [-p|-P] [file]\n"
    "\tThe -p/-P options specifies whether to plot points or lines."
;

/*
  int main(int argc, char *argv[])
*/
int
main(int argc, const char *argv[])
{
    const char *bfile = NULL;
    int burst_opt; /* unused, for option compatibility */
    int plot_lines = 0;
    int plot_points = 0;
    int ret_ac;

    bu_setprogname(argv[0]);

    struct bu_opt_desc d[4];
    struct bu_vls pmsg = BU_VLS_INIT_ZERO;

    BU_OPT(d[0],  "p", "", "",  NULL,   &plot_points, "Plot points");
    BU_OPT(d[1],  "P", "", "",  NULL,   &plot_lines,  "Plot lines");
    BU_OPT(d[2],  "b", "", "",  NULL,   &burst_opt,   "Batch mode");
    BU_OPT_NULL(d[3]);

    /* Interactive mode is gone - until we strip all the leftovers out
     * of the code, let it know we're not in tty mode */
    tty = 0;

    bu_setlinebuf(stderr);

    /* Skip first arg */
    argv++; argc--;

    /* no options imply a request for usage */
    if (argc < 1 || !argv || argv[0] == NULL) {
	(void)fprintf(stderr, "%s\n", usage);
	return EXIT_SUCCESS;
    }

    /* Process options */
    ret_ac = bu_opt_parse(&pmsg, argc, argv, d);
    if (ret_ac < 0) {
	(void)fprintf(stderr, "%s\n", bu_vls_cstr(&pmsg));
	bu_vls_free(&pmsg);
	(void)fprintf(stderr, "%s\n", usage);
	return EXIT_FAILURE;
    }
    bu_vls_free(&pmsg);

    if (ret_ac) {
	if (!bu_file_exists(argv[0], NULL)) {
	    (void)fprintf(stderr, "ERROR: Input file [%s] does not exist!\n", argv[0]);
	    (void)fprintf(stderr, "%s\n", usage);
	    return EXIT_FAILURE;
	} else {
	    bfile = argv[0];
	}
    }

    tmpfp = bu_temp_file(tmpfname, TIMER_LEN);
    if (!tmpfp) {
	bu_exit(EXIT_FAILURE, "ERROR: Unable to create temporary file.\n");
    }

    setupSigs();

    /* must be called before any output is produced */
    if (!initUi()) {
	fclose(tmpfp);
	return EXIT_FAILURE;
    }

#if DEBUG_BURST
    prntTrie(cmdtrie, 0);
#endif
    assert(airids.i_next == NULL);
    assert(armorids.i_next == NULL);
    assert(critids.i_next == NULL);

    if (bfile) {
	FILE *fp = fopen(bfile, "rb");
	readBatchInput(fp);
	fclose(fp);
    } else {
	readBatchInput(stdin);
    }

    fclose(tmpfp);
    return EXIT_SUCCESS;
}


/*
  void exitCleanly(int code)

  Should be only exit from program after success of initUi().
*/
void
exitCleanly(int code)
{
    if (tty)
	closeUi(); /* keep screen straight */
    (void) fclose(tmpfp);
    if (!bu_file_delete(tmpfname))
	locPerror(tmpfname);
    exit(code);
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
