/*
 *			R T N O D E . C
 *
 *  The per-node ray-tracing engine for the real-time ray-tracing project.
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

#if IRIX == 4
#define _BSD_COMPAT	1
#endif

#include <stdio.h>
#ifdef HAVE_STDARG_H
# include <stdarg.h>
#else
# include <varargs.h>
#endif

#include <sys/time.h>

#ifndef SYSV
# include <sys/ioctl.h>
# include <sys/resource.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#undef	VMIN
#include "machine.h"
#include "vmath.h"
#include "rtlist.h"
#include "rtstring.h"
#include "externs.h"
#include "raytrace.h"
#include "pkg.h"
#include "fb.h"

#include "../librt/debug.h"
#include "../rt/material.h"
#include "../rt/ext.h"
#include "../rt/rdebug.h"

/***** Variables shared with viewing model *** */
FBIO		*fbp = FBIO_NULL;	/* Framebuffer handle */
FILE		*outfp = NULL;		/* optional pixel output file */
mat_t		view2model;
mat_t		model2view;
/***** end of sharing with viewing model *****/

extern void grid_setup();
extern void worker();

/***** variables shared with worker() ******/
struct application ap;
vect_t		left_eye_delta;
/***** end variables shared with worker() *****/

/***** variables shared with do.c *****/
char		*beginptr;		/* sbrk() at start of program */
/***** end variables shared with do.c *****/

/* Variables shared within mainline pieces */
extern fastf_t	rt_dist_tol;		/* Value for rti_tol.dist */
extern fastf_t	rt_perp_tol;		/* Value for rti_tol.perp */
int		rdebug;			/* RT program debugging (not library) */

static char idbuf[132];			/* First ID record info */

/* State flags */
static int	seen_dirbuild;
static int	seen_gettrees;
static int	seen_matrix;

static char *title_file, *title_obj;	/* name of file and first object */

#define MAX_WIDTH	(16*1024)

static int	avail_cpus;		/* # of cpus avail on this system */
static int	max_cpus;		/* max # cpus for use, <= avail_cpus */

int print_on = 1;

/*
 *  Package handlers for the RTSYNC protocol.
 *  Numbered differently, to prevent confusion with other PKG protocols.
 */
#define RTSYNCMSG_PRINT	 999	/* StoM:  Diagnostic message */
#define RTSYNCMSG_ALIVE	1001	/* StoM:  protocol version, # of processors */
#define RTSYNCMSG_POV	1007	/* MtoS:  pov, min_res, start&end lines */
#define RTSYNCMSG_HALT	1008	/* MtoS:  abandon frame & xmit, NOW */
#define RTSYNCMSG_DONE	1009	/* StoM:  halt=0/1, res, elapsed, etc... */

void	ph_default();
void	rtsync_ph_pov();
void	rtsync_ph_halt();
static struct pkg_switch rtsync_pkgswitch[] = {
	{ RTSYNCMSG_ALIVE,	ph_default,	"ALIVE" },
	{ RTSYNCMSG_POV,	rtsync_ph_pov, "POV" },
	{ RTSYNCMSG_HALT,	rtsync_ph_halt, "HALT" },
	{ RTSYNCMSG_DONE,	ph_default,	"DONE" },
	{ RTSYNCMSG_PRINT,	ph_default,	"Log Message" },
	{ 0,			0,		(char *)0 }
};

void	ph_gettrees();
void	ph_dirbuild();

struct pkg_conn *pcsrv;		/* PKG connection to server */
char		*control_host;	/* name of host running controller */
char		*tcp_port;	/* TCP port on control_host */

int debug = 0;		/* 0=off, 1=debug, 2=verbose */

char srv_usage[] = "Usage: rtnode [-d] control-host tcp-port [cmd]\n";

/*
 *			M A I N
 */
int
main(argc, argv)
int argc;
char **argv;
{
	register int	n;
	FILE		*fp;

	if( argc < 2 )  {
		fprintf(stderr, srv_usage);
		exit(1);
	}
	while( argv[1][0] == '-' )  {
		if( strcmp( argv[1], "-d" ) == 0 )  {
			debug++;
		} else if( strcmp( argv[1], "-x" ) == 0 )  {
			sscanf( argv[2], "%x", &rt_g.debug );
			argc--; argv++;
		} else if( strcmp( argv[1], "-X" ) == 0 )  {
			sscanf( argv[2], "%x", &rdebug );
			argc--; argv++;
		} else {
			fprintf(stderr, srv_usage);
			exit(3);
		}
		argc--; argv++;
	}
	if( argc != 3 && argc != 4 )  {
		fprintf(stderr, srv_usage);
		exit(2);
	}

	control_host = argv[1];
	tcp_port = argv[2];

	/* Note that the LIBPKG error logger can not be
	 * "rt_log", as that can cause rt_log to be entered recursively.
	 * Given the special version of rt_log in use here,
	 * that will result in a deadlock in RES_ACQUIRE(res_syscall)!
	 *  libpkg will default to stderr via pkg_errlog(), which is fine.
	 */
	pcsrv = pkg_open( control_host, tcp_port, "tcp", "", "",
		rtsync_pkgswitch, NULL );
	if( pcsrv == PKC_ERROR )  {
		fprintf(stderr, "rtnode: unable to contact %s, port %s\n",
			control_host, tcp_port);
		exit(1);
	}

	if( argc == 4 )  {
#if 0
		/* Slip one command to dispatcher */
		(void)pkg_send( MSG_CMD, argv[3], strlen(argv[3])+1, pcsrv );

		/* Prevent chasing the package with an immediate TCP close */
		sleep(1);

#endif
		pkg_close( pcsrv );
		exit(0);
	}

	if( (fbp = fb_open( "vj:0", 0, 0 ) ) == 0 )  {
		fprintf(stderr,"rtnode: fb_open() failed\n");
		exit(1);
	}

#if BSD == 43
	{
		int	val = 32767;
		n = setsockopt( pcsrv->pkc_fd, SOL_SOCKET,
			SO_SNDBUF, (char *)&val, sizeof(val) );
		if( n < 0 )  perror("setsockopt: SO_SNDBUF");
	}
#endif

	if( !debug )  {
		/* A fresh process */
		if (fork())
			exit(0);

		/* Go into our own process group */
		n = getpid();

		/* SysV uses setpgrp with no args and it can't fail */
#if (defined(__STDC__) || defined(SYSV)) && !defined(_BSD_COMPAT)
		if( setpgid( n, n ) < 0 )
			perror("setpgid");
#else
		if( setpgrp( n, n ) < 0 )
			perror("setpgrp");
#endif

		/* Deal with CPU limits on "those kinds" of systems */
		if( rt_cpuget() > 0 )  {
			rt_cpuset( 9999999 );
		}

		/* Close off the world */
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);

		(void)close(0);
		(void)close(1);
		(void)close(2);

		/* For stdio & perror safety, reopen 0,1,2 */
		(void)open("/dev/null", 0);	/* to fd 0 */
		(void)dup(0);			/* to fd 1 */
		(void)dup(0);			/* to fd 2 */

#ifndef SYSV
		n = open("/dev/tty", 2);
		if (n >= 0) {
			(void)ioctl(n, TIOCNOTTY, 0);
			(void)close(n);
		}
#endif
	}

	/*
	 *  Now that the fork() has been done, it is safe to initialize
	 *  the parallel processing support.
	 */

	beginptr = (char *) sbrk(0);

#define PUBLIC_CPUS	"/usr/tmp/public_cpus"
	max_cpus = avail_cpus = rt_avail_cpus();
	if( (fp = fopen(PUBLIC_CPUS, "r")) != NULL )  {
		(void)fscanf( fp, "%d", &max_cpus );
		fclose(fp);
		if( max_cpus < 0 )  max_cpus = avail_cpus + max_cpus;
		if( max_cpus > avail_cpus )  max_cpus = avail_cpus;
	} else {
		(void)unlink(PUBLIC_CPUS);
		if( (fp = fopen(PUBLIC_CPUS, "w")) != NULL )  {
			fprintf(fp, "%d\n", avail_cpus);
			fclose(fp);
			(void)chmod(PUBLIC_CPUS, 0666);
		}
	}

	/* Need to set rtg_parallel non_zero here for RES_INIT to work */
	npsw = max_cpus;
	if( npsw > 1 )  {
		rt_g.rtg_parallel = 1;
	} else
		rt_g.rtg_parallel = 0;
	RES_INIT( &rt_g.res_syscall );
	RES_INIT( &rt_g.res_worker );
	RES_INIT( &rt_g.res_stats );
	RES_INIT( &rt_g.res_results );
	RES_INIT( &rt_g.res_model );
	/* DO NOT USE rt_log() before this point! */

	rt_log("using %d of %d cpus\n",
		npsw, avail_cpus );
	if( max_cpus <= 0 )  {
		pkg_close(pcsrv);
		exit(0);
	}

	/* XXX Hack -- do this before ALIVE message */
	ph_dirbuild( 0, rt_strdup("moss.g") );
	ph_gettrees( 0, rt_strdup("all.g") );

	/* Send our version string */
#define PROTOCOL_VERSION	"Version1.0"
	{
		char buf[512];
		sprintf(buf, "%d %s", npsw, PROTOCOL_VERSION );
		if( pkg_send( RTSYNCMSG_ALIVE, buf, strlen(buf)+1, pcsrv ) < 0 )  {
			fprintf(stderr,"pkg_send RTSYNCMSG_ALIVE error\n");
			exit(1);
		}
	}
	if( debug )  fprintf(stderr, "PROTOCOL_VERSION='%s'\n", PROTOCOL_VERSION );


	for(;;)  {
		register struct pkg_queue	*lp;
		fd_set ifds;
		struct timeval tv;

		/* First, process any packages in library buffers */
		if( pkg_process( pcsrv ) < 0 )  {
			rt_log("pkg_get error\n");
			break;
		}

		/* Second, see if any input to read */
		FD_ZERO(&ifds);
		FD_SET(pcsrv->pkc_fd, &ifds);
		tv.tv_sec = 5;
		tv.tv_usec = 0L;

		/* XXX fbp */

		if( select(pcsrv->pkc_fd+1, &ifds, (fd_set *)0, (fd_set *)0,
			&tv ) != 0 )  {
			n = pkg_suckin(pcsrv);
			if( n < 0 )  {
				rt_log("pkg_suckin error\n");
				break;
			} else if( n == 0 )  {
				/* EOF detected */
				break;
			} else {
				/* All is well */
			}
		}

		/* Third, process any new packages in library buffers */
		if( pkg_process( pcsrv ) < 0 )  {
			rt_log("pkg_get error\n");
			break;
		}

		/* Finally, more work may have just arrived, check our list */
	}
}



void
ph_cd(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
	if(debug)fprintf(stderr,"ph_cd %s\n", buf);
	if( chdir( buf ) < 0 )  {
		rt_log("ph_cd: chdir(%s) failure\n", buf);
		exit(1);
	}
	(void)free(buf);
}

void
ph_restart(pc, buf)
register struct pkg_conn *pc;
char *buf;
{

	if(debug)fprintf(stderr,"ph_restart %s\n", buf);
	rt_log("Restarting\n");
	pkg_close(pcsrv);
	execlp( "rtnode", "rtnode", control_host, tcp_port, (char *)0);
	perror("rtnode");
	exit(1);
}

/*
 *			P H _ D I R B U I L D
 *
 *  The only argument is the name of the database file.
 */
void
ph_dirbuild(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
#define MAXARGS 1024
	char	*argv[MAXARGS+1];
	struct rt_i *rtip;

	if( debug )  fprintf(stderr, "ph_dirbuild: %s\n", buf );

	if( (rt_split_cmd( argv, MAXARGS, buf )) <= 0 )  {
		/* No words in input */
		(void)free(buf);
		return;
	}

	if( seen_dirbuild )  {
		rt_log("ph_dirbuild:  MSG_DIRBUILD already seen, ignored\n");
		(void)free(buf);
		return;
	}

	title_file = rt_strdup(argv[0]);

	/* Build directory of GED database */
	if( (rtip=rt_dirbuild( title_file, idbuf, sizeof(idbuf) )) == RTI_NULL )  {
		rt_log("ph_dirbuild:  rt_dirbuild(%s) failure\n", title_file);
		exit(2);
	}
	ap.a_rt_i = rtip;
	seen_dirbuild = 1;

#if 0
	if( pkg_send( MSG_DIRBUILD_REPLY,
	    idbuf, strlen(idbuf)+1, pcsrv ) < 0 )
		fprintf(stderr,"MSG_DIRBUILD_REPLY error\n");
#endif
}

/*
 *			P H _ G E T T R E E S
 *
 *  Each word in the command buffer is the name of a treetop.
 */
void
ph_gettrees(pc, buf)
register struct pkg_conn *pc;
char *buf;
{
#define MAXARGS 1024
	char	*argv[MAXARGS+1];
	int	argc;
	struct rt_i *rtip = ap.a_rt_i;

	RT_CK_RTI(rtip);

	if( debug )  fprintf(stderr, "ph_gettrees: %s\n", buf );

	/* Copy values from command line options into rtip */
	rtip->useair = use_air;
	if( rt_dist_tol > 0 )  {
		rtip->rti_tol.dist = rt_dist_tol;
		rtip->rti_tol.dist_sq = rt_dist_tol * rt_dist_tol;
	}
	if( rt_perp_tol > 0 )  {
		rtip->rti_tol.perp = rt_perp_tol;
		rtip->rti_tol.para = 1 - rt_perp_tol;
	}

	if( (argc = rt_split_cmd( argv, MAXARGS, buf )) <= 0 )  {
		/* No words in input */
		(void)free(buf);
		return;
	}
	title_obj = rt_strdup(argv[0]);

	if( rtip->needprep == 0 )  {
		/* First clean up after the end of the previous frame */
		if(debug)rt_log("Cleaning previous model\n");
		view_end( &ap );
		view_cleanup( rtip );
		rt_clean(rtip);
		if(rdebug&RDEBUG_RTMEM_END)
			rt_prmem( "After rt_clean" );
	}

	/* Load the desired portion of the model */
	if( rt_gettrees(rtip, argc, (CONST char **)argv, npsw) < 0 )
		fprintf(stderr,"rt_gettrees(%s) FAILED\n", argv[0]);

	/* In case it changed from startup time via an OPT command */
	if( npsw > 1 )  {
		rt_g.rtg_parallel = 1;
	} else
		rt_g.rtg_parallel = 0;

	beginptr = (char *) sbrk(0);

	seen_gettrees = 1;
	(void)free(buf);

	/*
	 * initialize application -- it will allocate 1 line and
	 * set buf_mode=1, as well as do mlib_init().
	 */
	(void)view_init( &ap, title_file, title_obj, 0 );

	do_prep( rtip );

	if( rtip->nsolids <= 0 )  {
		rt_log("ph_matrix: No solids remain after prep.\n");
		exit(3);
	}

#if 0
	/* Acknowledge that we are ready */
	if( pkg_send( MSG_GETTREES_REPLY,
	    title_obj, strlen(title_obj)+1, pcsrv ) < 0 )
		fprintf(stderr,"MSG_START error\n");
#endif
}

/* -------------------- */

/*
 *			R T S Y N C _ P H _ P O V
 *
 *
 *  Format:  min_res, start_line, end_line, pov...
 */
void
rtsync_ph_pov(pc, buf)
register struct pkg_conn *pc;
char			*buf;
{
	register struct rt_i *rtip = ap.a_rt_i;
	char	*argv[MAXARGS+1];
	int	argc;
	int	min_res;
	int	start_line;
	int	end_line;
	mat_t	toViewcenter;
	mat_t	Viewrot;
	quat_t	orient;
	fastf_t	viewscale;

	RT_CK_RTI(rtip);

	if( debug )  fprintf(stderr, "ph_pov: %s\n", buf );

	if( (argc = rt_split_cmd( argv, MAXARGS, buf )) <= 0 )  {
		/* No words in input */
		rt_log("Null POV command\n");
		(void)free(buf);
		return;
	}

	/* Start options in a known state */
	AmbientIntensity = 0.4;
	hypersample = 0;
	jitter = 0;
	rt_perspective = 0;
	eye_backoff = 1.414;
	aspect = 1;
	stereo = 0;
	use_air = 0;
#if 0
	width = height = 0;
	cell_width = cell_height = 0;
	lightmodel = 0;
	incr_mode = 0;
	rt_dist_tol = 0;
	rt_perp_tol = 0;

	process_cmd( buf );
#endif

	min_res = atoi(argv[0]);
	start_line = atoi(argv[1]);
	end_line = atoi(argv[2]);

	VSET( eye_model, atof(argv[3]), atof(argv[4]), atof(argv[5]) );
	VSET( orient, atof(argv[6]), atof(argv[7]), atof(argv[8]) );
	orient[3] = atof(argv[9]);
	viewscale = atof(argv[10]);

	mat_idn( toViewcenter );
	mat_idn( Viewrot );
	MAT_DELTAS_VEC_NEG( toViewcenter, eye_model );
	quat_quat2mat( Viewrot, orient );
	Viewrot[15] = viewscale;
	mat_mul( model2view, Viewrot, toViewcenter );

	seen_matrix = 1;

	grid_setup();

	/* initialize lighting */
	view_2init( &ap );

	rtip->nshots = 0;
	rtip->nmiss_model = 0;
	rtip->nmiss_tree = 0;
	rtip->nmiss_solid = 0;
	rtip->nmiss = 0;
	rtip->nhits = 0;
	rtip->rti_nrays = 0;


	rt_prep_timer();
	do_run( start_line*width, end_line*width+width-1 );
	(void)rt_read_timer( (char *)0, 0 );

	/* Signal done */

	free(buf);
}


void
rtsync_ph_halt(pc, buf)
register struct pkg_conn *pc;
char			*buf;
{
}


/*
 *			R T L O G
 *
 *  Log an error.
 *  This version buffers a full line, to save network traffic.
 */
#if (__STDC__ && !apollo)
void
rt_log( char *fmt, ... )
{
	va_list ap;
	static char buf[512];		/* a generous output line */
	static char *cp = buf+1;

	if( print_on == 0 )  return;
	RES_ACQUIRE( &rt_g.res_syscall );
	va_start( ap, fmt );
	(void)vsprintf( cp, fmt, ap );
	va_end(ap);

	while( *cp++ )  ;		/* leaves one beyond null */
	if( cp[-2] != '\n' )
		goto out;
	if( pcsrv == PKC_NULL || pcsrv == PKC_ERROR )  {
		fprintf(stderr, "%s", buf+1);
		goto out;
	}
	if(debug) fprintf(stderr, "%s", buf+1);
	if( pkg_send( RTSYNCMSG_PRINT, buf+1, strlen(buf+1)+1, pcsrv ) < 0 )  {
		fprintf(stderr,"pkg_send RTSYNCMSG_PRINT failed\n");
		exit(12);
	}
	cp = buf+1;
out:
	RES_RELEASE( &rt_g.res_syscall );
}
#else /* __STDC__ */

#if defined(sgi) && !defined(mips)
# define _sgi3d	1
#endif

#if (defined(BSD) && !defined(_sgi3d)) || defined(mips) || defined(CRAY2)
/*
 *  			R T _ L O G
 *  
 *  Log an RT library event using the Berkeley _doprnt() routine.
 */
/* VARARGS */
void
rt_log(va_alist)
va_dcl
{
	va_list		ap;
	char		*fmt;
	static char	buf[512];
	static char	*cp;
	FILE		strbuf;

	if( print_on == 0 )  return;
	if( cp == (char *)0 )  cp = buf+1;

	RES_ACQUIRE( &rt_g.res_syscall );		/* lock */
	va_start(ap);
	fmt = va_arg(ap,char *);
#if defined(mips) || (defined(alliant) && defined(i860))
	(void) vsprintf( cp, fmt, ap );
#else
	strbuf._flag = _IOWRT|_IOSTRG;
#if defined(sun)
	strbuf._ptr = (unsigned char *)cp;
#else
	strbuf._ptr = cp;
#endif
	strbuf._cnt = sizeof(buf)-(cp-buf);
	(void) _doprnt( fmt, ap, &strbuf );
	putc( '\0', &strbuf );
#endif
	va_end(ap);

	if(debug) fprintf(stderr, "%s", buf+1);
	while( *cp++ )  ;		/* leaves one beyond null */
	if( cp[-2] != '\n' )
		goto out;
	if( pcsrv == PKC_NULL || pcsrv == PKC_ERROR )  {
		fprintf(stderr, "%s", buf+1);
		goto out;
	}
	if( pkg_send( RTSYNCRTSYNCMSG_PRINT, buf+1, strlen(buf+1)+1, pcsrv ) < 0 )  {
		fprintf(stderr,"pkg_send RTSYNCRTSYNCMSG_PRINT failed\n");
		exit(12);
	}
	cp = buf+1;
out:
	RES_RELEASE( &rt_g.res_syscall );		/* unlock */
}
#else
/* VARARGS */
void
rt_log( str, a, b, c, d, e, f, g, h )
char	*str;
int	a, b, c, d, e, f, g, h;
{
	static char buf[512];		/* a generous output line */
	static char *cp = buf+1;

	if( print_on == 0 )  return;
	RES_ACQUIRE( &rt_g.res_syscall );
	(void)sprintf( cp, str, a, b, c, d, e, f, g, h );
	while( *cp++ )  ;		/* leaves one beyond null */
	if( cp[-2] != '\n' )
		goto out;
	if( pcsrv == PKC_NULL || pcsrv == PKC_ERROR )  {
		fprintf(stderr, "%s", buf+1);
		goto out;
	}
	if(debug) fprintf(stderr, "%s", buf+1);
	if( pkg_send( RTSYNCRTSYNCMSG_PRINT, buf+1, strlen(buf+1)+1, pcsrv ) < 0 )  {
		fprintf(stderr,"pkg_send RTSYNCRTSYNCMSG_PRINT failed\n");
		exit(12);
	}
	cp = buf+1;
out:
	RES_RELEASE( &rt_g.res_syscall );
}
#endif /* not BSD */
#endif /* not __STDC__ */


/*
 *			R T _ B O M B
 *  
 *  Abort the RT library
 */
void
rt_bomb(str)
char *str;
{
	char	*bomb = "RTSRV terminated by rt_bomb()\n";

	if( pkg_send( RTSYNCMSG_PRINT, str, strlen(str)+1, pcsrv ) < 0 )  {
		fprintf(stderr,"rt_bomb RTSYNCMSG_PRINT failed\n");
	}
	if( pkg_send( RTSYNCMSG_PRINT, bomb, strlen(bomb)+1, pcsrv ) < 0 )  {
		fprintf(stderr,"rt_bomb RTSYNCMSG_PRINT failed\n");
	}

	if(debug)  fprintf(stderr,"\n%s\n", str);
	fflush(stderr);
	if( rt_g.debug || rt_g.NMG_debug || debug )
		abort();	/* should dump */
	exit(12);
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
	rt_log("ctl: unable to handle %s message: len %d",
		pc->pkc_switch[i].pks_title, pc->pkc_len);
	*buf = '*';
	(void)free(buf);
}
