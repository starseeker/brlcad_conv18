/*
 *			R T S Y N C . C
 *
 *  Main program to tightly synchronize a network distributed array of
 *  processors.  Interfaces with MGED via the VRMGR protocol,
 *  and with RTNODE.
 *
 *  The heart of the "real time ray-tracing" project.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Package" agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 1995 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (ARL)";
#endif

#include "conf.h"

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <math.h>
#ifdef USE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*
 *  The situation with sys/time.h and time.h is crazy.
 *  We need sys/time.h for struct timeval,
 *  and time.h for struct tm.
 *
 *  on BSD (and SGI 4D), sys/time.h includes time.h,
 *  on the XMP (UNICOS 3 & 4), time.h includes sys/time.h,
 *  on the Cray-2, there is no automatic including.
 *
 *  Note that on many SYSV machines, the Cakefile has to set BSD
 */
#if BSD && !SYSV
#  include <sys/time.h>		/* includes <time.h> */
#else
#  if CRAY1 && !__STDC__
#	include <time.h>	/* includes <sys/time.h> */
#  else
#	include <sys/time.h>
#	include <time.h>
#  endif
#endif

#include "machine.h"
#include "vmath.h"
#include "rtstring.h"
#include "rtlist.h"
#include "raytrace.h"
#include "pkg.h"
#include "fb.h"
#include "tcl.h"
#include "tk.h"
#include "externs.h"

#include "./ihost.h"

extern int	pkg_permport;	/* libpkg/pkg_permserver() listen port */

/*
 *  Package Handlers for the VRMGR protocol
 */
#define VRMSG_ROLE	1	/* from MGED: Identify role of machine */
#define VRMSG_CMD	2	/* to MGED: Command to process */
#define VRMSG_EVENT	3	/* from MGED: device event */
#define VRMSG_POV	4	/* from MGED: point of view info */
#define VRMSG_VLIST	5	/* transfer binary vlist block */
#define VRMSG_CMD_REPLY	6	/* from MGED: reply to VRMSG_CMD */

void	ph_default();	/* foobar message handler */
void	vrmgr_ph_role();
void	vrmgr_ph_event();
void	vrmgr_ph_pov();
static struct pkg_switch vrmgr_pkgswitch[] = {
	{ VRMSG_ROLE,		vrmgr_ph_role,	"Declare role" },
	{ VRMSG_CMD,		ph_default,	"to MGED:  command" },
	{ VRMSG_EVENT,		vrmgr_ph_event,	"MGED device event" },
	{ VRMSG_POV,		vrmgr_ph_pov,	"MGED point of view" },
	{ VRMSG_VLIST,		ph_default,	"binary vlist block" },
	{ 0,			0,		(char *)0 }
};

int			vrmgr_listen_fd;	/* for new connections */
struct pkg_conn		*vrmgr_pc;		/* connection to VRMGR */
struct ihost		*vrmgr_ihost;		/* host of vrmgr_pc */
char			*pending_pov;		/* pending new POV */
char			*last_pov;		/* last POV sent */
int			print_on = 1;

/*
 *  Package handlers for the RTSYNC protocol.
 *  Numbered differently, to prevent confusion with other PKG protocols.
 */
#define RTSYNCMSG_PRINT	 999	/* StoM:  Diagnostic message */
#define RTSYNCMSG_ALIVE	1001	/* StoM:  protocol version, # of processors */
#define RTSYNCMSG_OPENFB 1002	/* both:  width height framebuffer */
#define RTSYNCMSG_DIRBUILD 1003	/* both:  database */
#define RTSYNCMSG_GETTREES 1004	/* both:  treetop(s) */
#define RTSYNCMSG_CMD	1006	/* MtoS:  Any Tcl command */
#define RTSYNCMSG_POV	1007	/* MtoS:  pov, min_res, start&end lines */
#define RTSYNCMSG_HALT	1008	/* MtoS:  abandon frame & xmit, NOW */
#define RTSYNCMSG_DONE	1009	/* StoM:  halt=0/1, res, elapsed, etc... */

void	rtsync_ph_alive();
void	rtsync_ph_openfb();
void	rtsync_ph_dirbuild();
void	rtsync_ph_gettrees();
void	rtsync_ph_done();
void	ph_print();
static struct pkg_switch rtsync_pkgswitch[] = {
	{ RTSYNCMSG_DONE,	rtsync_ph_done, "RTNODE assignment done" },
	{ RTSYNCMSG_ALIVE,	rtsync_ph_alive, "RTNODE is alive" },
	{ RTSYNCMSG_OPENFB,	rtsync_ph_openfb, "RTNODE open(ed) fb" },
	{ RTSYNCMSG_DIRBUILD,	rtsync_ph_dirbuild, "RTNODE dirbuilt/built" },
	{ RTSYNCMSG_GETTREES,	rtsync_ph_gettrees, "RTNODE prep(ed) db" },
	{ RTSYNCMSG_POV,	ph_default,	"POV" },
	{ RTSYNCMSG_HALT,	ph_default,	"HALT" },
	{ RTSYNCMSG_CMD,	ph_default,	"CMD" },
	{ RTSYNCMSG_PRINT,	ph_print,	"Log Message" },
	{ 0,			0,		(char *)0 }
};

int			rtsync_listen_fd;	/* for new connections */

#define STATE_NEWBORN	0		/* No packages received yet */
#define STATE_ALIVE	1		/* ALIVE pkg recv'd */
#define STATE_OPENFB	2		/* OPENFB ack pkg received */
#define STATE_DIRBUILT	3		/* DIRBUILD ack pkg received */
#define STATE_PREPPED	4		/* GETTREES ack pkg received */
CONST char *states[] = {
	"NEWBORN",
	"ALIVE",
	"OPENFB",
	"DIRBUILT",
	"PREPPED",
	"n+1"
};

/*
 *			R T N O D E
 *
 *  One per compute server host.
 */
struct rtnode {
	int		fd;
	struct pkg_conn	*pkg;
	struct ihost	*host;
	int		state;
	int		ncpus;		/* Ready when > 0, for now */
	int		busy;		/* !0 -> still working */
	int		lump;		/* # of lines in last assignment */
	int		finish_order;	/* 1st in the race? */
	struct timeval	time_start;	/* elapsed time of last assignment (sec) */
	fastf_t		time_delta;
	fastf_t		i_lps;		/* instantaneous # lines per sec */
	fastf_t		w_lps;		/* weighted # lines per sec */
	int		pr_percent;	/* reprojection pixel percentage */
	fastf_t		pr_time;	/* elapsed time for reprojection (ms) */
	fastf_t		rt_time;	/* elapsed time for ray-tracing (ms) */
	fastf_t		fb_time;	/* elapsed time for fb_write() (ms) */
	fastf_t		ck_time;	/* elapsed time for fb sync check (ms) */
};
#define MAX_NODES	32
struct rtnode	rtnodes[MAX_NODES];

/* Timing data */
int		finish_position;
struct timeval	frame_start;
struct timeval	t_assigned;
struct timeval	t_1st_done;
struct timeval	t_last_done;
struct timeval	t_all_done;

/* Cooked values in ms from this past frame */
fastf_t		ms_assigned;
int		ms_1st_done;
int		ms_all_done;
int		ms_flush;
int		ms_total;
fastf_t		sec_since_last;

fastf_t		ms_rt_min;		/* ms it took fastest ray-tracer */
fastf_t		ms_rt_max;		/* ms it took slowest ray-tracer */

fastf_t		ms_total_min;		/* ms it took fastest worker */
fastf_t		ms_total_max;

fastf_t		average_mbps = 0;	/* mbps */
fastf_t		burst_mbps = 0;		/* mbps -- a lower bound */
int		network_overhead = 0;	/* integer percentage */
char		last_host_done[32];	/* short ident of last host done */
double		variation;		/* slowest -vs- fastest */

static fd_set	select_list;			/* master copy */
static int	max_fd;

static	FBIO	*fbp;
static	char	*framebuffer;
static	int	width = 0;		/* use default size */
static	int	height = 0;
int		debug = 0;

/* Variables linked to Tcl/Tk */

Tcl_Interp	*interp = NULL;
Tk_Window	tkwin;

CONST char	*database;
struct bu_vls	treetops;
double		blend1 = 0.8;		/* weight to give older timing data */
int		update_status_every_frame = 0;
int		debugimage = 0;

char		*node_search_path;


BU_EXTERN(void dispatcher, (ClientData clientData));
void	new_rtnode();
void	drop_rtnode();
void	setup_socket();
char	*stamp();

BU_EXTERN(int cmd_drop_rtnode, (ClientData clientData, Tcl_Interp *interp, int argc, char **argv));


static char usage[] = "\
Usage:  rtsync [-d#] [-h] [-S squaresize] [-W width] [-N height] [-F framebuffer]\n\
	model.g treetop(s)\n\
";

/*
 *			G E T _ A R G S
 */
get_args( argc, argv )
register char **argv;
{
	register int c;

	while ( (c = getopt( argc, argv, "d:hF:s:w:n:S:W:N:" )) != EOF )  {
		switch( c )  {
		case 'd':
			debug = atoi(optarg);
			break;
		case 'h':
			/* high-res */
			height = width = 1024;
			break;
		case 'F':
			framebuffer = optarg;
			break;
		case 's':
		case 'S':
			height = width = atoi(optarg);
			break;
		case 'w':
		case 'W':
			width = atoi(optarg);
			break;
		case 'n':
		case 'N':
			height = atoi(optarg);
			break;

		default:		/* '?' */
			return(0);
		}
	}

	return(1);		/* OK */
}

/*
 *			T V D I F F
 *
 *  Return t1 - t0, as a floating-point number of seconds.
 */
double
tvdiff(t1, t0)
struct timeval	*t1, *t0;
{
	return( (t1->tv_sec - t0->tv_sec) +
		(t1->tv_usec - t0->tv_usec) / 1000000. );
}

/*
 *			L I S T _ R T N O D E S
 *
 *  Give list of valid rtnode structure indices
 */
int
list_rtnodes( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	int	i;

	for( i=0; i < MAX_NODES; i++ )  {
		char	buf[32];

		if( rtnodes[i].fd <= 0 )  continue;
		sprintf(buf, "%d", i);
		Tcl_AppendResult(interp, " ", buf, NULL);
	}
	return TCL_OK;
}

/*
 *			G E T _ R T N O D E
 *
 *  Print status of a given rtnode structure, for TCL script consumption.
 */
int
get_rtnode( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	int	i;
	struct bu_vls	str;
	CONST char	*message;

	if( argc != 2 )  {
		Tcl_AppendResult(interp, "Usage: get_rtnode ###\n", NULL);
		return TCL_ERROR;
	}
	i = atoi(argv[1]);
	if( i < 0 || i >= MAX_NODES )  {
		/* Use this as a signal to generate a title. */
		Tcl_AppendResult(interp,
			"##: CP ord i_lps w_lps lump tot=pr/rt/fb/ck state", NULL);
		return TCL_OK;
	}
	if( rtnodes[i].fd <= 0 )  {
		Tcl_AppendResult(interp, "get_rtnode ",
			argv[1], " index not assigned\n", NULL);
		return TCL_ERROR;
	}

	if( rtnodes[i].state == STATE_PREPPED )
		message = rtnodes[i].busy ? "BUSY" : "wait";
	else
		message = states[rtnodes[i].state];

	bu_vls_init(&str);
	bu_vls_printf(&str,
		"%2.2d: %2.2d %2.2d %5.5d %5.5d %4.4d %d=%d/%d/%d/%d %s %s",
		i,
		rtnodes[i].ncpus,
		rtnodes[i].finish_order,
		(int)rtnodes[i].i_lps,
		(int)rtnodes[i].w_lps,
		rtnodes[i].lump,
		(int)(rtnodes[i].time_delta * 1000),
		(int)rtnodes[i].pr_time,
		(int)rtnodes[i].rt_time,
		(int)rtnodes[i].fb_time,
		(int)rtnodes[i].ck_time,
		message,
		rtnodes[i].host->ht_name
		);
	Tcl_AppendResult(interp, bu_vls_addr(&str), NULL);
	bu_vls_free(&str);
	return TCL_OK;
}

/*
 *			N O D E _ S E N D
 *
 *  Arrange to send a string to ALL rtnode processes,
 *  for them to run as a TCL command.
 */
int
node_send( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	struct bu_vls	cmd;
	int		i;

	if( argc < 2 )  {
		Tcl_AppendResult(interp, "Usage: node_send command(s)\n", NULL);
		return TCL_ERROR;
	}

	bu_vls_init(&cmd);
	bu_vls_from_argv( &cmd, argc-1, argv+1 );

	bu_log("node_send: %s\n", bu_vls_addr(&cmd));

	for( i = MAX_NODES-1; i >= 0; i-- )  {
		if( rtnodes[i].fd <= 0 )  continue;
		if( rtnodes[i].ncpus <= 0 )  continue;
		if( pkg_send_vls( RTSYNCMSG_CMD, &cmd, rtnodes[i].pkg ) < 0 )  {
			drop_rtnode(i);
			continue;
		}
	}
	bu_vls_free(&cmd);
	return TCL_OK;
}

/*
 *			O N E _ N O D E _ S E N D
 *
 *  Arrange to send a string to one specific rtnode process,
 *  for it to run as a TCL command.
 *  Used primarily for debugging, or adjusting number of processors.
 */
int
one_node_send( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	struct bu_vls	cmd;
	int		node;

	if( argc < 3 )  {
		Tcl_AppendResult(interp, "Usage: one_node_send node# command(s)\n", NULL);
		return TCL_ERROR;
	}

	if( (node = get_rtnode_by_name(argv[1])) < 0 )  {
		Tcl_AppendResult(interp, "one_node_send ", argv[1],
			": bad node specification\n", NULL);
		return TCL_ERROR;
	}
	if( !rtnodes[node].host || !rtnodes[node].pkg ) {
		Tcl_AppendResult(interp, "one_node_send ", argv[1],
			": NULL host or pkg pointer?\n", NULL);
		return TCL_ERROR;
	}

	bu_vls_init(&cmd);
	bu_vls_from_argv( &cmd, argc-2, argv+2 );

	bu_log("one_node_send(%s) %s\n",
		rtnodes[node].host->ht_name,
		bu_vls_addr(&cmd));

	if( pkg_send_vls( RTSYNCMSG_CMD, &cmd, rtnodes[node].pkg ) < 0 )  {
		Tcl_AppendResult(interp, "pkg_send to ", argv[1], " failed.\n", NULL);
		drop_rtnode(node);
	}
	bu_vls_free(&cmd);
	return TCL_OK;
}

/*
 *			V R M G R _ H O S T N A M E
 *
 *  Returns name of VRMGR host, or null string if none.
 */
int
vrmgr_hostname( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	if( vrmgr_pc )  {
		Tcl_AppendResult(interp, vrmgr_ihost->ht_name, (char *)NULL);
	}
	return TCL_OK;
}

/*
 *			V R M G R _ S E N D
 *
 *  Arrange to send a string to the VRMGR process
 *  for it to run as a TCL command.
 */
int
vrmgr_send( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	struct bu_vls	cmd;
	char		*reply;
	int		i;

	if( argc < 2 )  {
		Tcl_AppendResult(interp, "Usage: vrmgr_send command(s)\n", NULL);
		return TCL_ERROR;
	}

	if( !vrmgr_pc )  {
		Tcl_AppendResult(interp, "vrmgr is not presently connected\n", NULL);
		return TCL_ERROR;
	}

	bu_vls_init(&cmd);
	bu_vls_from_argv( &cmd, argc-1, argv+1 );
	bu_vls_putc( &cmd, '\n');

	if( pkg_send_vls( VRMSG_CMD, &cmd, vrmgr_pc ) < 0 )  {
		pkg_close(vrmgr_pc);
		vrmgr_pc = 0;
		Tcl_AppendResult(interp, "Error writing to vrmgr\n", NULL);
		bu_vls_free(&cmd);
		return TCL_ERROR;
	}
	bu_vls_free(&cmd);

	reply = pkg_bwaitfor( VRMSG_CMD_REPLY, vrmgr_pc );
	if( !reply )  {
		Tcl_AppendResult(interp, "Error reading reply from vrmgr\n", NULL);
		return TCL_ERROR;
	}

	/* Bring across result string from other side, as our result. */
	Tcl_AppendResult( interp, reply+1, NULL );
	if( reply[0] == 'Y' )  {
		(void)free(reply);
		return TCL_OK;
	}
	(void)free(reply);
	return TCL_ERROR;
}

/*
 *			A L L _ S E N D
 *
 *  Arrange to send a string to ALL rtnode processes,
 *  and over the VGMGR link to MGED.
 *
 *  This exists as a built-in primarily to keep down the amount of traffic
 *  that Dynamic Geometry clients need to "send" to us.
 */
int
all_send( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	if( argc < 2 )  {
		Tcl_AppendResult(interp, "Usage: all_send command(s)\n", NULL);
		return TCL_ERROR;
	}

	(void)node_send( clientData, interp, argc, argv );
	return vrmgr_send( clientData, interp, argc, argv );
}

/*
 *			R E P R E P
 *
 *  Make all the nodes re-prep.
 */
int
reprep( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	int		i;
	struct bu_vls	cmd;

	if( argc != 1 )  {
		Tcl_AppendResult(interp, "Usage: reprep\n", NULL);
		return TCL_ERROR;
	}

	bu_vls_init(&cmd);
	bu_vls_strcpy( &cmd, "rt_clean");

	bu_log("reprep\n");

	for( i = MAX_NODES-1; i >= 0; i-- )  {
		if( rtnodes[i].fd <= 0 )  continue;
		if( rtnodes[i].ncpus <= 0 )  continue;
		if( rtnodes[i].state != STATE_PREPPED )  {
			Tcl_AppendResult(interp, "reprep: host ",
				rtnodes[i].host->ht_name,
				" is in state ",
				states[rtnodes[i].state],
				"\n", NULL);
			drop_rtnode(i);
			continue;
		}
		/* change back to STATE_DIRBUILT and send GETTREES commands */
		if( change_state( i, STATE_PREPPED, STATE_DIRBUILT ) < 0 )  {
			drop_rtnode(i);
			continue;
		}

		/* Receipt of GETTREES will automatically "clean" previous model first */
		if( pkg_send_vls( RTSYNCMSG_GETTREES, &treetops,
		     rtnodes[i].pkg ) < 0 )  {
			drop_rtnode(i);
		     	continue;
		}
	}
	return TCL_OK;
}

/*
 *			R E F R E S H
 *
 *  If there isn't a pending POV message, cause the last POV to be
 *  re-ray-traced.
 *  Used primarily when GUI is changing viewing parameters but POV
 *  isn't changing.
 */
int
refresh( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	if( pending_pov )  return TCL_OK;
	if( last_pov )  {
		pending_pov = bu_strdup( last_pov );
		return TCL_OK;
	}
	Tcl_AppendResult(interp, "refresh:  no last_pov, ignored\n", NULL);
	return TCL_ERROR;
}

/*
 *			G E T _ S T A T S
 *  Arg -
 *	-2	Get summary line rtsync stats, like what's printed per frame.
 *	-1	Get overall rtsync time-breakdown stats, in ms.
 *	0..n	Get rtnode-specific stats for indicated node.
 */
int
get_stats( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	int		i;
	struct bu_vls	str;

	if( argc != 2 )  {
		Tcl_AppendResult(interp, "Usage: get_stats #\n", NULL);
		return TCL_ERROR;
	}
	i = atoi(argv[1]);
	if( i >= MAX_NODES )  {
		Tcl_AppendResult(interp, "get_stats ", argv[1], " out of range\n", NULL);
		return TCL_ERROR;
	}

	if( i < 0 )  switch(i) {
	case -1:
		bu_vls_init(&str);
		/* All values reported in ms */
		bu_vls_printf(&str,
			"lag %.1f asgn %.1f 1st %d all %d flush %d total %d",
			sec_since_last * 1000,
			ms_assigned,
			ms_1st_done,
			ms_all_done,
			ms_flush,
			ms_total
		    );
		Tcl_AppendResult(interp, bu_vls_addr(&str), NULL);
		bu_vls_free(&str);
		return TCL_OK;
	case -2:
		bu_vls_init(&str);
		/* All values reported in ms */
		bu_vls_printf(&str,
			"rt_min %.1f rt_max %.1f tot_min %.1f tot_max %.1f",
			ms_rt_min,
			ms_rt_max,
			ms_total_min,
			ms_total_max
		    );
		Tcl_AppendResult(interp, bu_vls_addr(&str), NULL);
		bu_vls_free(&str);
		return TCL_OK;
	case -3:
		bu_vls_init(&str);
		/* All values reported in ms */
		bu_vls_printf(&str,
			"mbps %.1f net%% %d var %.1f last %s",
			burst_mbps,
			network_overhead,
			variation,
			last_host_done
		    );
		Tcl_AppendResult(interp, bu_vls_addr(&str), NULL);
		bu_vls_free(&str);
		return TCL_OK;
	case -4:
		bu_vls_init(&str);
		bu_vls_printf(&str,
			"rtsync %s vrmgr %s fb %s",
			get_our_hostname(),
			vrmgr_pc ? vrmgr_ihost->ht_name : "{NIL}",
			framebuffer );
		Tcl_AppendResult(interp, bu_vls_addr(&str), NULL);
		bu_vls_free(&str);
		return TCL_OK;
	case -5:
		bu_vls_init(&str);
		bu_vls_strcat(&str, database);
		bu_vls_putc(&str, ' ');
		bu_vls_vlscat(&str, &treetops);
		Tcl_AppendResult(interp, bu_vls_addr(&str), NULL);
		bu_vls_free(&str);
		return TCL_OK;
	default:
		bu_vls_free(&str);
		Tcl_AppendResult(interp, "get_stats ", argv[1], " out of range\n", NULL);
		return TCL_ERROR;
	}
	if( rtnodes[i].fd <= 0 )  {
		Tcl_AppendResult(interp, "get_stats ",
			argv[1], " index not assigned\n", NULL);
		return TCL_ERROR;
	}
	if( rtnodes[i].state != STATE_PREPPED )  {
		Tcl_AppendResult(interp, "get_stats ",
			argv[1], " not prepped yet\n", NULL);
		return TCL_ERROR;
	}

	/* Report on the indicated node */
	bu_vls_init(&str);
	bu_vls_printf(&str,
		"total %g rt %g fb %g ck %g",
		rtnodes[i].time_delta * 1000,
		rtnodes[i].rt_time,
		rtnodes[i].fb_time,
		rtnodes[i].ck_time
	    );
	Tcl_AppendResult(interp, bu_vls_addr(&str), NULL);
	bu_vls_free(&str);
	return TCL_OK;
}

/**********************************************************************/

/*
 *			S T D I N _ E V E N T _ H A N D L E R
 *
 *  Read Tcl commands from a "tty" file descriptor.
 *  Called from the TCL/Tk event handler
 */
void
stdin_event_handler(clientData, mask)
ClientData	clientData;	/* fd */
int		mask;
{
	char	buf[511+1];
	int	fd;
	int	got;

	fd = (int)clientData;

	got = read(fd, buf, 511);
	if( got >= 0 && got < 511 )  buf[got] = '\0';
	else	buf[0] = '\0';

	if( got <= 0 )  {
		bu_log("EOF on stdin\n");
		Tcl_DeleteFileHandler( fd );
		return;
	}

	/* Do something here.  Eventually, feed off to Tcl interp. */
	if( Tcl_Eval( interp, buf ) == TCL_OK )  {
		bu_log("%s\n", interp->result);

		Tcl_DoWhenIdle( dispatcher, (ClientData)0 );
	} else {
		bu_log("ERROR %s\n", interp->result);
	}
}

/*
 *			P K G _ E V E N T _ H A N D L E R
 *
 *  Generic event handler to read from a LIBPKG connection.
 *  Called from the TCL/Tk event handler
 */
void
pkg_event_handler(clientData, mask)
ClientData	clientData;	/* *pc */
int		mask;
{
	struct pkg_conn	*pc;
	int	val;

	pc = (struct pkg_conn *)clientData;

	val = pkg_suckin(pc);
	if( val < 0 ) {
		bu_log("pkg_suckin() error\n");
	} else if( val == 0 )  {
		bu_log("EOF on pkg connection\n");
	}
	if( val <= 0 )  {
		Tcl_DeleteFileHandler( pc->pkc_fd );
		pkg_close(pc);
		return;
	}
	if( pkg_process( pc ) < 0 )
		bu_log("pc:  pkg_process error encountered\n");

	Tcl_DoWhenIdle( dispatcher, (ClientData)0 );
}

/*
 *			V R M G R _ E V E N T _ H A N D L E R
 *
 *  Event handler to read VRMGR commands from a LIBPKG connection.
 *  Called from the TCL/Tk event handler.
 */
void
vrmgr_event_handler(clientData, mask)
ClientData	clientData;	/* *pc */
int		mask;
{
	struct pkg_conn	*pc;
	int	val;

	pc = (struct pkg_conn *)clientData;

	val = pkg_suckin(pc);
	if( val < 0 ) {
		bu_log("vrmgr: pkg_suckin() error\n");
	} else if( val == 0 )  {
		bu_log("vrmgr: EOF on pkg connection\n");
	}
	if( val <= 0 )  {
		Tcl_DeleteFileHandler( pc->pkc_fd );
		pkg_close(pc);
		vrmgr_pc = 0;
		vrmgr_ihost = IHOST_NULL;
		return;
	}
	if( pkg_process( pc ) < 0 )
		bu_log("vrmgr:  pkg_process error encountered\n");

	Tcl_DoWhenIdle( dispatcher, (ClientData)0 );
}

/*
 *			R T N O D E _ E V E N T _ H A N D L E R
 *
 *  Read from a LIBPKG connection associated with an rtnode.
 *  Called from the TCL/Tk event handler
 */
void
rtnode_event_handler(clientData, mask)
ClientData	clientData;	/* subscript to rtnodes[] */
int		mask;
{
	int	i;

	i = (int)clientData;
	if( rtnodes[i].fd == 0 )  {
		bu_log("rtnode_event_handler(%d) no fd?\n", i);
		return;
	}
	if( pkg_process( rtnodes[i].pkg ) < 0 ) {
		bu_log("pkg_process error encountered (1)\n");
	}
	if( pkg_suckin( rtnodes[i].pkg ) <= 0 )  {
		/* Probably EOF */
		drop_rtnode( i );
		return;
	}
	if( pkg_process( rtnodes[i].pkg ) < 0 ) {
		bu_log("pkg_process error encountered (2)\n");
	}

	Tcl_DoWhenIdle( dispatcher, (ClientData)0 );
}

/*
 *			F B _ E V E N T _ H A N D L E R
 */
void
fb_event_handler(clientData, mask)
ClientData	clientData;	/* FBIO * */
int		mask;
{
	FBIO	*fbp;
bu_log("fb_event_handler() called\n");

	fbp = (FBIO *)clientData;
	fb_poll(fbp);

	Tcl_DoWhenIdle( dispatcher, (ClientData)0 );
}

/*
 *			V R M G R _ L I S T E N _ H A N D L E R
 */
void
vrmgr_listen_handler(clientData, mask)
ClientData	clientData;	/* fd */
int		mask;
{

	/* Accept any new VRMGR connections.  Only one at a time is permitted. */
	if( vrmgr_pc )  {
		bu_log("New VRMGR connection received with one still active, dropping old one.\n");
		Tcl_DeleteFileHandler( vrmgr_pc->pkc_fd );
		pkg_close( vrmgr_pc );
		vrmgr_pc = 0;
		vrmgr_ihost = IHOST_NULL;
	}
	vrmgr_pc = pkg_getclient( vrmgr_listen_fd, vrmgr_pkgswitch, bu_log, 0 );
	vrmgr_ihost = host_lookup_of_fd(vrmgr_pc->pkc_fd);
	if( vrmgr_ihost == IHOST_NULL )  {
		bu_log("Unable to get hostname of VRMGR, abandoning it\n");
		pkg_close( vrmgr_pc );
		vrmgr_pc = 0;
	} else {
		bu_log("%s VRMGR link with %s, fd=%d\n",
			stamp(),
			vrmgr_ihost->ht_name, vrmgr_pc->pkc_fd);
		Tcl_CreateFileHandler(
			vrmgr_pc->pkc_fd,
			TCL_READABLE|TCL_EXCEPTION, vrmgr_event_handler,
			(ClientData)vrmgr_pc );
		FD_SET(vrmgr_pc->pkc_fd, &select_list);
		if( vrmgr_pc->pkc_fd > max_fd )  max_fd = vrmgr_pc->pkc_fd;
		setup_socket( vrmgr_pc->pkc_fd );
	}

	Tcl_DoWhenIdle( dispatcher, (ClientData)0 );
}

/*
 *			R T S Y N C _ L I S T E N _ H A N D L E R
 */
void
rtsync_listen_handler(clientData, mask)
ClientData	clientData;	/* fd */
int		mask;
{
	new_rtnode( pkg_getclient( (int)clientData,
		rtsync_pkgswitch, bu_log, 0 ) );

	Tcl_DoWhenIdle( dispatcher, (ClientData)0 );
}

/**********************************************************************/

/*
 *			M A I N
 */
main(argc, argv)
int	argc;
char	*argv[];
{
	width = height = 256;	/* keep it modest, by default */

	if ( !get_args( argc, argv ) ) {
		(void)fputs(usage, stderr);
		exit( 1 );
	}

	if( optind >= argc )  {
		fprintf(stderr,"rtsync:  MGED database not specified\n");
		(void)fputs(usage, stderr);
		exit(1);
	}
	database = argv[optind++];
	if( optind >= argc )  {
		fprintf(stderr,"rtsync:  tree top(s) not specified\n");
		(void)fputs(usage, stderr);
		exit(1);
	}

	bu_vls_init( &treetops );
	bu_vls_from_argv( &treetops, argc - optind, argv+optind );
	bu_log("DB: %s %s\n", database, bu_vls_addr(&treetops) );

	BU_LIST_INIT( &rt_g.rtg_vlfree );

	/* Initialize the Tcl interpreter */
	interp = Tcl_CreateInterp();

	/* This runs the init.tcl script */
	if( Tcl_Init(interp) == TCL_ERROR )
		bu_log("Tcl_Init error %s\n", interp->result);
	bu_tcl_setup(interp);
	bn_tcl_setup(interp);
	rt_tcl_setup(interp);
	Tcl_SetVar(interp, "cpu_count", "0", TCL_GLOBAL_ONLY );
	/* Don't allow unknown commands to be fed to the shell */
	Tcl_SetVar( interp, "tcl_interactive", "0", TCL_GLOBAL_ONLY );

	Tcl_LinkVar( interp, "framebuffer", (char *)&framebuffer, TCL_LINK_STRING | TCL_LINK_READ_ONLY );
	Tcl_LinkVar( interp, "database", (char *)&database, TCL_LINK_STRING | TCL_LINK_READ_ONLY );
	Tcl_LinkVar( interp, "debug", (char *)&debug, TCL_LINK_INT );
	Tcl_LinkVar( interp, "debugimage", (char *)&debugimage, TCL_LINK_INT );
	Tcl_LinkVar( interp, "update_status_every_frame", (char *)&update_status_every_frame, TCL_LINK_INT );
	Tcl_LinkVar( interp, "blend1", (char *)&blend1, TCL_LINK_DOUBLE );

	/* This string may be supplemented by the Tcl runtime */
	Tcl_LinkVar( interp, "node_search_path", (char *)&node_search_path, TCL_LINK_STRING );
	Tcl_SetVar( interp, "node_search_path",
"/m/cad /m/cad/db /n/vapor/m/cad /n/vapor/m/cad/db \
~/mike ~mike/cad/db ~/butler ~butler/cad/db \
/home/army/mike/SGI/cad /var/tmp /tmp",
		TCL_GLOBAL_ONLY );

	/* This runs the tk.tcl script */
	if(Tk_Init(interp) == TCL_ERROR)  bu_bomb("Try setting TK_LIBRARY environment variable\n");
	if((tkwin = Tk_MainWindow(interp)) == NULL)
		bu_bomb("Tk_MainWindow failed\n");
	/* Don't insist on a click to position the window, just make it */
	(void)Tcl_Eval( interp, "wm geometry . =+1+1");

	/* Incorporate built-in commands.  BEFORE running script. */
	(void)Tcl_CreateCommand(interp, "all_send", all_send,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	(void)Tcl_CreateCommand(interp, "vrmgr_hostname", vrmgr_hostname,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	(void)Tcl_CreateCommand(interp, "vrmgr_send", vrmgr_send,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	(void)Tcl_CreateCommand(interp, "node_send", node_send,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	(void)Tcl_CreateCommand(interp, "one_node_send", one_node_send,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	(void)Tcl_CreateCommand(interp, "reprep", reprep,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	(void)Tcl_CreateCommand(interp, "refresh", refresh,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	(void)Tcl_CreateCommand(interp, "get_stats", get_stats,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	(void)Tcl_CreateCommand(interp, "list_rtnodes", list_rtnodes,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	(void)Tcl_CreateCommand(interp, "get_rtnode", get_rtnode,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
	(void)Tcl_CreateCommand(interp, "drop_rtnode", cmd_drop_rtnode,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);

	/* Let main window pop up before running script */
	while( Tcl_DoOneEvent(TCL_DONT_WAIT) != 0 ) ;
# if 0
	(void)Tcl_Eval( interp, "wm withdraw .");
# endif
	if( Tcl_EvalFile( interp, "/m/cad/remrt/rtsync.tcl" ) != TCL_OK )  {
		bu_log("%s\n",
			Tcl_GetVar(interp,"errorInfo", TCL_GLOBAL_ONLY) );
		bu_log("\n*** rtsync.tcl Startup Script Aborted ***\n\n");
	}

	/* Accept commands on stdin */
	if( isatty(fileno(stdin)) )  {
		Tcl_CreateFileHandler(
			fileno(stdin),
			TCL_READABLE|TCL_EXCEPTION, stdin_event_handler,
			(ClientData)fileno(stdin) );
		bu_log("rtsync accepting commands on stdin\n");
	}


	/* Connect up with framebuffer, for control & size purposes */
	if( !framebuffer )  framebuffer = getenv("FB_FILE");
	if( !framebuffer )  rt_bomb("rtsync:  No -F and $FB_FILE not set\n");
	if( (fbp = fb_open(framebuffer, width, height)) == FBIO_NULL )
		exit(1);

	if( width <= 0 || fb_getwidth(fbp) < width )
		width = fb_getwidth(fbp);
	if( height <= 0 || fb_getheight(fbp) < height )
		height = fb_getheight(fbp);

	{
		unsigned char	flash[4];
		flash[0] = 224;
		flash[1] = 255;
		flash[2] = 224;
		(void)fb_clear(fbp, flash);
		(void)fb_flush(fbp);
	}

	/* Listen for VRMGR Master PKG connections from MGED */
	if( (vrmgr_listen_fd = pkg_permserver("5555", "tcp", 8, bu_log)) < 0 )  {
		bu_log("Unable to listen on 5555\n");
		exit(1);
	}
	Tcl_CreateFileHandler(
		vrmgr_listen_fd,
		TCL_READABLE|TCL_EXCEPTION, vrmgr_listen_handler,
		(ClientData)vrmgr_listen_fd);

	/* Listen for our RTSYNC client's PKG connections */
	if( (rtsync_listen_fd = pkg_permserver("rtsrv", "tcp", 8, bu_log)) < 0 )  {
		int	i;
		char	num[8];
		/* Do it by the numbers */
		for(i=0; i<10; i++ )  {
			sprintf( num, "%d", 4446+i );
			if( (rtsync_listen_fd = pkg_permserver(num, "tcp", 8, bu_log)) < 0 )
				continue;
			break;
		}
		if( i >= 10 )  {
			bu_log("Unable to find a port to listen on\n");
			exit(1);
		}
	}
	Tcl_CreateFileHandler(
		rtsync_listen_fd,
		TCL_READABLE|TCL_EXCEPTION, rtsync_listen_handler,
		(ClientData)rtsync_listen_fd);
	/* Now, pkg_permport has tcp port number */
	bu_log("%s RTSYNC listening on %s port %d\n",
		stamp(),
		get_our_hostname(),
		pkg_permport);

	(void)signal( SIGPIPE, SIG_IGN );

	if( fbp && fbp->if_selfd > 0 )  {
		Tcl_CreateFileHandler(
			fbp->if_selfd,
			TCL_READABLE|TCL_EXCEPTION, fb_event_handler,
			(ClientData)fbp );
	}

	Tcl_DoWhenIdle( dispatcher, (ClientData)0 );

	for(;;)  {
		Tcl_DoOneEvent(0);
	}
}

/*
 *			D I S P A T C H E R
 *
 *  Where all the work gets sent out.
 *  
 *  This is only called once and then evaporates;
 *  the event-handlers are responsible for queueing up more when
 *  something might have happened, with:
 *	Tcl_DoWhenIdle( dispatcher, (ClientData)0 );
 */
void
dispatcher(clientData)
ClientData clientData;
{
	register int	i;
	int		cpu_count = 0;
	int		start_line;
	int		lowest_index = 0;
	char		buf[32];
	double		total_lps;
	int		nlines;

	if( !pending_pov )  return;

	cpu_count = 0;
	for( i = MAX_NODES-1; i >= 0; i-- )  {
		if( rtnodes[i].fd <= 0 )  continue;
		if( rtnodes[i].state != STATE_PREPPED )  continue;
		if( rtnodes[i].ncpus <= 0 )  continue;
		if( rtnodes[i].busy )  return;	/* Still working on last one */
		cpu_count += rtnodes[i].ncpus;
		lowest_index = i;
	}
	if( debug )  {
		bu_log("%s dispatcher() has %d cpus\n", stamp(), cpu_count);
	}
	sprintf(buf, "%d", cpu_count);
	Tcl_SetVar(interp, "cpu_count", buf, TCL_GLOBAL_ONLY );
	if( cpu_count <= 0 )  return;

	/* Record starting time for this frame */
	(void)gettimeofday( &frame_start, (struct timezone *)NULL );
	finish_position = 0;

	/* Determine how long it's been since end of last frame */
	sec_since_last = tvdiff( &frame_start, &t_all_done );

	/* Keep track of the POV used for this assignment.  For refresh. */
	if( last_pov )  {
		bu_free( last_pov, "last POV pkg" );
		last_pov = NULL;
	}
	last_pov = pending_pov;
	pending_pov = NULL;

	/* We have some CPUS! Parcel up 'height' scanlines. */
	/*
	 *  First, tally up the measurements of the weighted lines/sec
	 *  to determine total compute capability.
	 */
	total_lps = 0;
	for( i = MAX_NODES-1; i >= 0; i-- )  {
		if( rtnodes[i].fd <= 0 )  continue;
		if( rtnodes[i].state != STATE_PREPPED )  continue;
		total_lps += rtnodes[i].w_lps;
	}

	/* Second, allocate work as a fraction of that capability */
	nlines = 0;
	for( i = MAX_NODES-1; i >= 0; i-- )  {
		if( rtnodes[i].fd <= 0 )  continue;
		if( rtnodes[i].state != STATE_PREPPED )  continue;
		rtnodes[i].lump = (int)ceil(height * rtnodes[i].w_lps / total_lps);
		nlines += rtnodes[i].lump;
	}
	if( nlines < height )  bu_log("ERROR: nlines=%d, height=%d\n", nlines, height);

	/* Third, actually dispatch the work */
	start_line = 0;
	for( i = MAX_NODES-1; i >= 0; i-- )  {
		int	end_line;
		struct bu_vls	msg;

		if( start_line >= height )  break;
		if( rtnodes[i].fd <= 0 )  continue;
		if( rtnodes[i].state != STATE_PREPPED )  continue;

		end_line = start_line + rtnodes[i].lump-1;
		if( end_line > height-1 )  end_line = height-1;

		bu_vls_init( &msg );
		bu_vls_printf( &msg, "%d %d %d %s\n",
			256,
			start_line, end_line,
			last_pov+4 );
		if( pkg_send_vls( RTSYNCMSG_POV, &msg, rtnodes[i].pkg ) < 0 )  {
			drop_rtnode( i );
			bu_vls_free(&msg);
			continue;	/* Don't update start_line */
		}
		(void)gettimeofday( &rtnodes[i].time_start, (struct timezone *)NULL );
		if( debug )
			bu_log("%s sending %d..%d to %s\n", stamp(), start_line, end_line, rtnodes[i].host->ht_name);

		bu_vls_free(&msg);
		rtnodes[i].busy = 1;
		start_line = end_line + 1;
	}
	/* Record time that all assignments went out */
	(void)gettimeofday( &t_assigned, (struct timezone *)NULL );
}

/*
 *			N E W _ R T N O D E
 */
void
new_rtnode(pcp)
struct pkg_conn	*pcp;
{
	register int	i;
	struct ihost	*host;

	if( pcp == PKC_ERROR )
		return;

	if( !(host = host_lookup_of_fd(pcp->pkc_fd)) )  {
		bu_log("%s Unable to get host name of new connection, dropping\n", stamp() );
		pkg_close(pcp);
		return;
	}
	bu_log("%s Connection from %s\n", stamp(), host->ht_name);

	/* Make this scan go low-to-high, so that new machines get
	 * added in on the bottom.
	 * Since new work is allocated bottom-to-top, it makes sure
	 * new machines get a change to have their speed measured.
	 */
	for( i=0; i < MAX_NODES; i++ )  {
		if( rtnodes[i].fd != 0 )  continue;
		/* Found an available slot */
		bzero( (char *)&rtnodes[i], sizeof(rtnodes[0]) );
		rtnodes[i].state = STATE_NEWBORN;
		rtnodes[i].pkg = pcp;
		rtnodes[i].fd = pcp->pkc_fd;
		FD_SET(pcp->pkc_fd, &select_list);
		if( pcp->pkc_fd > max_fd )  max_fd = pcp->pkc_fd;
		setup_socket( pcp->pkc_fd );
		rtnodes[i].host = host;
		Tcl_CreateFileHandler(
			pcp->pkc_fd,
			TCL_READABLE|TCL_EXCEPTION, rtnode_event_handler,
			(ClientData)i );
		return;
	}
	bu_log("rtsync: too many rtnode clients.  My cup runneth over!\n");
	pkg_close(pcp);
}

/*
 *			D R O P _ R T N O D E
 */
void
drop_rtnode( sub )
int	sub;
{
	bu_log("%s Dropping %s\n", stamp(), rtnodes[sub].host->ht_name);

	if( rtnodes[sub].pkg != PKC_NULL )  {
		Tcl_DeleteFileHandler( rtnodes[sub].pkg->pkc_fd );
		pkg_close( rtnodes[sub].pkg );
		rtnodes[sub].pkg = PKC_NULL;
	}
	if( rtnodes[sub].fd != 0 )  {
		FD_CLR( rtnodes[sub].fd, &select_list );
		(void)close( rtnodes[sub].fd );
		rtnodes[sub].fd = 0;
	}

	Tcl_Eval( interp, "update_cpu_status" );
}

/*
 *			G E T _ R T N O D E _ B Y _ N A M E
 *
 *  Accepts hostname, or numeric table index.
 */
int
get_rtnode_by_name( str )
char *str;
{
	struct ihost	*ihp;
	int		i;

	if( isdigit( *str ) )  {
		int	i;
		i = atoi( str );
		if( i < 0 || i >= MAX_NODES )  return -1;
		if( rtnodes[i].fd <= 0 )  return -4;
		return i;
	}

	if( (ihp = host_lookup_by_name( str, 0 )) == IHOST_NULL )
		return -2;

	for( i = MAX_NODES-1; i >= 0; i-- )  {
		if( rtnodes[i].fd <= 0 )  continue;
		if( rtnodes[i].host == ihp )  return i;
	}
	return -3;
}

/*
 *			C M D  _ D R O P _ R T N O D E
 */
int
cmd_drop_rtnode( clientData, interp, argc, argv )
ClientData clientData;
Tcl_Interp *interp;
int argc;
char **argv;
{
	int	node;

	if( argc != 2 )  {
		Tcl_AppendResult(interp, "Usage: drop_rtnode hostname\n        drop_rtnode slot#\n", NULL);
		return TCL_ERROR;
	}
	node = get_rtnode_by_name( argv[1] );
	if( node < 0 )  {
		Tcl_AppendResult(interp, "drop_rtnode ", argv[1],
			": server not found\n");
		return TCL_ERROR;
	}
	drop_rtnode(node);
	return TCL_OK;
}

/*
 *			S E T U P _ S O C K E T
 */
void
setup_socket(fd)
int	fd;
{
	int	on = 1;

#if defined(SO_KEEPALIVE)
	if( setsockopt( fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on)) < 0 ) {
		perror( "setsockopt (SO_KEEPALIVE)");
	}
#endif
#if defined(SO_RCVBUF)
	/* try to set our buffers up larger */
	{
		int	m, n;
		int	val;
		int	size;

		for( size = 256; size > 16; size /= 2 )  {
			val = size * 1024;
			m = setsockopt( fd, SOL_SOCKET, SO_RCVBUF,
				(char *)&val, sizeof(val) );
			val = size * 1024;
			n = setsockopt( fd, SOL_SOCKET, SO_SNDBUF,
				(char *)&val, sizeof(val) );
			if( m >= 0 && n >= 0 )  break;
		}
		if( m < 0 || n < 0 )  perror("rtsync setsockopt()");
	}
#endif
}

/*
 *			C H A N G E _ S T A T E
 */
int
change_state( i, old, new )
int	i;
int	old;
int	new;
{
	bu_log("%s %s %s --> %s\n", stamp(),
		rtnodes[i].host->ht_name,
		states[rtnodes[i].state], states[new] );
	if( rtnodes[i].state != old )  {
		bu_log("%s was in state %s, should have been %s, dropping\n",
			rtnodes[i].host->ht_name,
			states[rtnodes[i].state], states[old] );
		drop_rtnode(i);
		return -1;
	}
	rtnodes[i].state = new;

	Tcl_Eval( interp, "update_cpu_status" );

	return 0;
}

/*
 *			V R M G R _ P H _ R O L E
 *
 *  The ROLE package should be the first thing that MGED says to
 *  the VRMGR (i.e. to us).
 *  There is no need to strictly check the protocol;
 *  if we get a ROLE package, it better be of type "master".
 *  If no ROLE package is sent, no big deal.
 */
void
vrmgr_ph_role(pc, buf)
register struct pkg_conn *pc;
char			*buf;
{
#define MAXARGS 32
	char		*argv[MAXARGS];
	int		argc;

	bu_log("%s VRMGR host %s, role %s\n",
		stamp(),
		vrmgr_ihost->ht_name, buf);

	argc = rt_split_cmd( argv, MAXARGS, buf );
	if( argc < 1 )  {
		bu_log("bad role command\n");
	}

	if( strcmp( argv[0], "master" ) != 0 )  {
		bu_log("ERROR %s: bad role '%s', dropping vrmgr\n",
			vrmgr_ihost->ht_name, buf );
		FD_CLR(vrmgr_pc->pkc_fd, &select_list);
		pkg_close( vrmgr_pc );
		vrmgr_pc = 0;
		vrmgr_ihost = 0;
	}

	Tcl_Eval( interp, "update_cpu_status" );

	if(buf) (void)free(buf);
}

/*
 *			V R M G R _ P H _ E V E N T
 *
 *  These are from slave MGEDs for relay to the master MGED.
 *  We don't expect any of these.
 */
void
vrmgr_ph_event(pc, buf)
register struct pkg_conn *pc;
char			*buf;
{
	register struct servers	*sp;

	bu_log("%s VRMGR unexpectely got event '%s'", stamp(), buf );
	if( buf )  free(buf);
}

/*
 *			V R M G R _ P H _ P O V
 *
 *  Accept a new point-of-view from the MGED master.
 *  If there is an existing POV which has not yet been rendered,
 *  drop it, and use the new one instead, to catch up.
 *
 *  We retain the buffer from LIBPKG until the POV is processed.
 */
void
vrmgr_ph_pov(pc, buf)
register struct pkg_conn *pc;
char			*buf;
{
	if( debug )  bu_log("%s %s\n", stamp(), buf);
	if( pending_pov )  free(pending_pov);
	pending_pov = buf;
}

/*
 *			P H _ D E F A U L T
 */
void
ph_default(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
	register int i;

	for( i=0; pc->pkc_switch[i].pks_handler != NULL; i++ )  {
		if( pc->pkc_switch[i].pks_type == pc->pkc_type )  break;
	}
	bu_log("rtsync: unable to handle %s message: len %d",
		pc->pkc_switch[i].pks_title, pc->pkc_len);
	*buf = '*';
	if( buf )  free(buf);
}

/*
 *			R T S Y N C _ P H _ A L I V E
 */
void
rtsync_ph_alive(pc, buf)
register struct pkg_conn *pc;
char			*buf;
{
	register int	i;
	int		ncpu;
	struct bu_vls	cmd;

	ncpu = atoi(buf);
	if( buf )  free(buf);

	for( i = MAX_NODES-1; i >= 0; i-- )  {
		if( rtnodes[i].pkg != pc )  continue;

		/* Found it */
		bu_log("%s %s ALIVE %d cpus\n", stamp(),
			rtnodes[i].host->ht_name,
			ncpu );

		rtnodes[i].ncpus = ncpu;

		if( change_state( i, STATE_NEWBORN, STATE_ALIVE ) < 0 )
			return;

		bu_vls_init( &cmd );

		/* Send across some initial state, via TCL commands. */
		bu_vls_printf( &cmd, "set node_search_path {%s}\n",
			node_search_path );
		if( pkg_send_vls( RTSYNCMSG_CMD, &cmd, rtnodes[i].pkg ) < 0 )  {
			drop_rtnode(i);
			continue;
		}

		/* Now try to get framebuffer open.  Reply message advances state, later. */
		bu_vls_trunc( &cmd, 0 );
		bu_vls_printf( &cmd, "%d %d %s", width, height, framebuffer );

		if( pkg_send_vls( RTSYNCMSG_OPENFB, &cmd, rtnodes[i].pkg ) < 0 )  {
			bu_vls_free( &cmd );
			drop_rtnode(i);
		     	return;
		}
		bu_vls_free( &cmd );
		return;
	}
	rt_bomb("ALIVE Message received from phantom pkg?\n");
}

/*
 *			R T S Y N C _ P H _ O P E N F B
 */
void
rtsync_ph_openfb(pc, buf)
register struct pkg_conn *pc;
char			*buf;
{
	register int	i;
	int		ncpu;

	if( buf )  free(buf);

	for( i = MAX_NODES-1; i >= 0; i-- )  {
		if( rtnodes[i].pkg != pc )  continue;

		/* Found it */
		if( change_state( i, STATE_ALIVE, STATE_OPENFB ) < 0 )
			return;

		if( pkg_send( RTSYNCMSG_DIRBUILD, database, strlen(database)+1,
		     rtnodes[i].pkg ) < 0 )  {
			drop_rtnode(i);
		     	return;
		}
		return;
	}
	rt_bomb("OPENFB Message received from phantom pkg?\n");
}

/*
 *			R T S Y N C _ P H _ D I R B U I L D
 *
 *  Reply contains database title string.
 */
void
rtsync_ph_dirbuild(pc, buf)
register struct pkg_conn *pc;
char			*buf;
{
	register int	i;
	int		ncpu;

	for( i = MAX_NODES-1; i >= 0; i-- )  {
		if( rtnodes[i].pkg != pc )  continue;

		/* Found it */
		bu_log("%s %s %s\n", stamp(),
			rtnodes[i].host->ht_name, buf );
		if( buf )  free(buf);

		if( change_state( i, STATE_OPENFB, STATE_DIRBUILT ) < 0 )
			return;

		if( pkg_send_vls( RTSYNCMSG_GETTREES, &treetops,
		     rtnodes[i].pkg ) < 0 )  {
			drop_rtnode(i);
		     	return;
		}
		return;
	}
	rt_bomb("DIRBUILD Message received from phantom pkg?\n");
}

/*
 *			R T S Y N C _ P H _ G E T T R E E S
 *
 *  Reply contains name of first treetop.
 */
void
rtsync_ph_gettrees(pc, buf)
register struct pkg_conn *pc;
char			*buf;
{
	register int	i;
	int		ncpu;

	for( i = MAX_NODES-1; i >= 0; i-- )  {
		if( rtnodes[i].pkg != pc )  continue;

		/* Found it */
		bu_log("%s %s %s\n", stamp(),
			rtnodes[i].host->ht_name, buf );
		if( buf )  free(buf);

		if( change_state( i, STATE_DIRBUILT, STATE_PREPPED ) < 0 )
			return;

		/* Initialize some key variables */
		rtnodes[i].busy = 0;
		rtnodes[i].lump = 0;
		rtnodes[i].w_lps = 1;

		/* No more dialog, next pkg will be a POV */

		return;
	}
	rt_bomb("GETTREES Message received from phantom pkg?\n");
}

/*
 *			R T S Y N C _ P H _ D O N E
 *
 *  This message indicates that RTNODE has successfully sent
 *  all it's pixels to the screen.
 */
void
rtsync_ph_done(pc, buf)
register struct pkg_conn *pc;
char			*buf;
{
	register int	i;
	struct timeval	time_end;
	double		interval;
	double		blend2;
	int		new_npsw;
	double		t1, t2, t3, t4;
	int		nfields;
	int		sched_update = 0;
	int		last_i;
	int		reproj_percent;
	long		total_bits;

	blend2 = 1 - blend1;	/* blend1 may change via Tcl interface */

	for( i = MAX_NODES-1; i >= 0; i-- )  {
		if( rtnodes[i].pkg != pc )  continue;

		/* Found it */
		(void)gettimeofday( &time_end, (struct timezone *)NULL );
		interval = tvdiff( &time_end, &rtnodes[i].time_start );
		if( interval <= 0 )  interval = 999;
		rtnodes[i].i_lps = rtnodes[i].lump / interval;
		rtnodes[i].w_lps = blend1 * rtnodes[i].w_lps +
				   blend2 * rtnodes[i].i_lps;
		rtnodes[i].time_delta = interval;

		rtnodes[i].finish_order = ++finish_position;
		if( finish_position == 1 )  {
			t_1st_done = time_end;	/* struct copy */
		}

		new_npsw = -1;
		t1 = t2 = t3 = -1;
		nfields = sscanf(buf, "%d %lf %lf %lf %lf %d",
			&new_npsw, &t1, &t2, &t3, &t4, &reproj_percent );

		if( nfields < 1 )  {
			bu_log(" %s %s reply message had insufficient (%d) fields\n",
				stamp(), rtnodes[i].host->ht_name,
				nfields);
		}

		if( nfields >= 1 && rtnodes[i].ncpus != new_npsw )  {
			bu_log(" %s %s NCPUs changed from %d to %d\n",
				stamp(), rtnodes[i].host->ht_name,
				rtnodes[i].ncpus, new_npsw);
			rtnodes[i].ncpus = new_npsw;
			sched_update = 1;
		}

		rtnodes[i].pr_time = t1;
		rtnodes[i].rt_time = t2;
		rtnodes[i].fb_time = t3;
		rtnodes[i].ck_time = t4;
		rtnodes[i].pr_percent = reproj_percent;

		if( debug )  {
			bu_log("%s DONE %s (%g ms) rt=%g fb=%g ck=%g buf=%s\n",
				stamp(),
				rtnodes[i].host->ht_name,
				interval * 1000.0,
				t1, t2, t3, buf
				);
		}
		rtnodes[i].busy = 0;
		last_i = i;
		if( buf )  free(buf);
		goto check_others;
	}
	rt_bomb("DONE Message received from phantom pkg?\n");

check_others:
	for( i = MAX_NODES-1; i >= 0; i-- )  {
		if( rtnodes[i].fd <= 0 )  continue;
		if( rtnodes[i].state != STATE_PREPPED )  continue;
		if( rtnodes[i].ncpus <= 0 )  continue;
		if( rtnodes[i].busy )  return;	/* Still working on last one */
	}

	/*
	 *  Frame is entirely done, this was the last assignemnt outstanding.
	 */
	strncpy( last_host_done, rtnodes[last_i].host->ht_name, 6 );
	last_host_done[6] = '\0';
	
	/* Record time that final assignment came back */
	(void)gettimeofday( &t_last_done, (struct timezone *)NULL );

	/* This frame is now done, flush to screen */
	fb_flush(fbp);
	(void)gettimeofday( &t_all_done, (struct timezone *)NULL );

	/* Compute total time for this frame */
	ms_assigned = tvdiff( &t_assigned, &frame_start ) * 1000;
	ms_1st_done = tvdiff( &t_1st_done, &frame_start ) * 1000;
	ms_all_done = tvdiff( &t_last_done, &frame_start ) * 1000;
	ms_flush = tvdiff( &t_all_done, &t_last_done ) * 1000;
	interval = tvdiff( &t_all_done, &frame_start );
	if( interval <= 0 )  interval = 999;
	ms_total = interval * 1000;

	/* Find max and min of ray-tracing time */
	ms_rt_min =  9999999;
	ms_rt_max = -9999999;
	ms_total_min =  9999999;
	ms_total_max = -9999999;
	for( i = MAX_NODES-1; i >= 0; i-- )  {
		double		rtt;
		double		tot;

		if( rtnodes[i].state != STATE_PREPPED )  continue;
		if( rtnodes[i].ncpus <= 0 )  continue;

		rtt = rtnodes[i].rt_time;
		if( rtt < ms_rt_min )  ms_rt_min = rtt;
		if( rtt > ms_rt_max )  ms_rt_max = rtt;

		tot = rtnodes[i].time_delta * 1000;
		if( tot < ms_total_min )  ms_total_min = tot;
		if( tot > ms_total_max )  ms_total_max = tot;
	}

	/* Calculate network bandwidth consumed in non-raytracing time */
	total_bits = width * height * 3 * 8;
	if( debugimage )  total_bits *= 2;
	average_mbps = total_bits / interval / 1000000.0;
	/* burst_mbps is still too low; it's a lower bound. */
	burst_mbps = total_bits / (interval - ms_rt_min/1000) / 1000000.0;

	/* Calculate "network overhead" as % of total time */
	network_overhead = (int)(((double)ms_total - ms_rt_max) / (double)ms_total * 100);

	/* Calculate assignment duration variation. 1X is optimum. */
	variation = ms_total_max / ms_total_min;

	bu_log("%s%6d ms, %5.1f Mbps, %4.1f fps, %2d%%net %.1fx %s%s\n",
		stamp(),
		ms_total,
		burst_mbps,
		1.0/interval,
		network_overhead,
		variation,
		variation >= 2 ? "***> " : "",
		last_host_done
	    );

	/* Trigger TCL code to auto-update cpu status window on major changes */
	if( sched_update || update_status_every_frame )
		Tcl_Eval( interp, "update_cpu_status" );
}

/*
 *			S T A M P
 *
 *  Return a string suitable for use as a timestamp.
 *  Mostly for stamping log messages with.
 */
char *
stamp()
{
	static char	buf[128];
	time_t		now;
	struct tm	*tmp;
	register char	*cp;

	(void)time( &now );
	tmp = localtime( &now );
	sprintf( buf, "%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
		tmp->tm_mon+1, tmp->tm_mday,
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec );

	return(buf);
}

/*
 *			P H _ P R I N T
 */
void
ph_print(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
	if(print_on)  {
		struct ihost	*ihp = host_lookup_of_fd(pc->pkc_fd);

		bu_log("%s %s: %s",
			stamp(),
			ihp ? ihp->ht_name : "NONAME",
			buf );
		if( buf[strlen(buf)-1] != '\n' )
			bu_log("\n");
	}
	if(buf) (void)free(buf);
}
