/*
 *			I F _ A D A G E . C
 *
 *  Authors -
 *	Phil Dykstra
 *	Gary S. Moss
 *	Mike J. Muuss
 *
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *
 * Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

/*
 * This module is used when pre-setting the Ikonas FBC
 * (Frame Buffer Controller) to a known state.
 * The values for this table are derived from the
 * Ikonas-supplied program FBI, for compatability.
 * At present, three modes can be set:
 *	0 - LORES, 30 hz, interlaced
 *	1 - LORES, 60 hz, non-interlaced
 *	2 - HIRES, 30 hz, interlaced
 *
 * All that is provided is a prototype for the FBC registers;
 * the user is responsible for changing them (zooming, etc),
 * and writing them to the FBC.
 */
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include "fb.h"
#include "./fblocal.h"
#include "./adage.h"

#if defined( BSD )
#include	<sys/ioctl.h>
#endif

#if defined( SYSV )
#if defined( VLDSYSV )
#include	<sys/_ioctl.h> /* GSM : _ needed for Sys V emulation */
#else
#include	<sys/ioctl.h>
#endif
#endif

_LOCAL_ int	adage_device_open(),
		adage_device_close(),
		adage_device_clear(),
		adage_buffer_read(),
		adage_buffer_write(),
		adage_colormap_read(),
		adage_colormap_write(),
		adage_window_set(),
		adage_zoom_set(),
		adage_cinit_bitmap(),
		adage_cmemory_addr(),
		adage_cscreen_addr();

FBIO adage_interface =
		{
		adage_device_open,
		adage_device_close,
		fb_null,
		adage_device_clear,
		adage_buffer_read,
		adage_buffer_write,
		adage_colormap_read,
		adage_colormap_write,
		fb_null,
		adage_window_set,
		adage_zoom_set,
		adage_cinit_bitmap,
		adage_cmemory_addr,
		adage_cscreen_addr,
		"Adage RDS3000",
		1024,
		1024,
		"/dev/ik",
		512,
		512,
		-1,
		PIXEL_NULL,
		PIXEL_NULL,
		PIXEL_NULL,
		-1,
		0,
		0L,
		0L,
		0
		};

static struct ik_fbc ikfbc_setup[3] = {
    {
	/* 0 - LORES, 30 hz, interlaced */
	0,	32,		/* x, y, viewport */
	511,	511,		/* x, y, sizeview */
	0,	4067,		/* x, y, window */
	0,	0,		/* x, y, zoom */
	300,	560,		/* horiztime, nlines */
	0,	FBCH_PIXELCLOCK(45) | FBCH_DRIVEBPCK, /* Lcontrol, Hcontrol */
	0,	32		/* x, y, cursor */
    }, {
	/* 1 - LORES, 60 hz, non-interlaced */
	0,	68,		/* viewport */
	511,	1023,		/* sizeview */
	0,	4063,		/* window, -33 */
	0,	0,		/* zoom */
	144,	1143,		/* horiztime, nlines (was 144, 1167) */
	FBC_RS343 | FBC_NOINTERLACE, FBCH_PIXELCLOCK(18) | FBCH_DRIVEBPCK,
	0,	32		/* cursor */
    }, {
	/* 2 - HIRES, 30 hz, interlaced */
	0,	64,
	1023,	1023,
	0,	4033,		/* -63 */
	0,	0,
	144,	1144,		/* was 144, 1166 */
	FBC_HIRES | FBC_RS343, FBCH_PIXELCLOCK(19) | FBCH_DRIVEBPCK,
	0,	64
    }
};

/*
 * Per adage state information.
 */
struct	ikinfo {
	struct	ik_fbc	ikfbcmem;	/* Current FBC state */
	short	*_ikUBaddr;		/* Mapped-in Ikonas address */
	/* Current values initialized in adage_init() */
	int	x_zoom, y_zoom;
	int	x_window, y_window;	/* Ikonas, upper left of window */
	int	mode;			/* 0,1,2 */
	int	y_winoff;		/* y window correction factor */
};
#define	IKI(ptr) ((struct ikinfo *)((ptr)->u1.p))
#define	IKIL(ptr) ((ptr)->u1.p)		/* left hand side version */

static long cursor[32] =
	{
#include "./adagecursor.h"
	};

/*
 * RGBpixel -> Ikonas pixel buffer
 * Used by both buffer_read & write, AND adage_color_clear
 * so be sure it holds ADAGE_DMA_BYTES.
 */
static char *_pixbuf = NULL;
typedef unsigned char IKONASpixel[4];

#define	ADAGE_DMA_BYTES	(63*1024)
#define	ADAGE_DMA_PIXELS (ADAGE_DMA_BYTES/sizeof(IKONASpixel))

_LOCAL_ int
adage_device_open( ifp, file, width, height )
FBIO	*ifp;
char	*file;
int	width, height;
{
	register int	i;
	char	ourfile[16];
	long	xbsval[34];

	/* Only 512 and 1024 opens are available */
	if( width > 512 || height > 512 )
		width = height = 1024;
	else
		width = height = 512;

	/*
	 * Determine the device name to open:
	 *   /dev/ik0l
	 *   012345678
	 */
	if( strlen( file ) > 12 )
		return -1;
	(void)sprintf( ourfile, "%s0l", file );
	if( width > 512 || height > 512 )
		ourfile[8] = 'h';
	else
		ourfile[8] = 'l';
	ourfile[9] = '\0';

	if( (ifp->if_fd = open( ourfile, O_RDWR, 0 )) == -1 )
		return	-1;

	/* create a clean ikinfo struct */
	if( (IKIL(ifp) = (char *)calloc( 1, sizeof(struct ikinfo) )) == NULL ) {
		fb_log( "adage_device_open: ikinfo malloc failed\n" );
		return	-1;
	}
#if defined( vax )
	if( ioctl( ifp->if_fd, IKIOGETADDR, &(IKI(ifp)->_ikUBaddr) ) < 0 )
		fb_log( "adage_device_open : ioctl(IKIOGETADDR) failed.\n" );
#endif
	ifp->if_width = width;
	ifp->if_height = height;
	switch( ifp->if_width ) {
	case 512:
		IKI(ifp)->mode = 1;
		break;
	case 1024:
		IKI(ifp)->mode = 2;
		break;
	default:
		fb_log( "Bad fbsize %d.\n", ifp->if_width );
		return	-1;
	}
	if( lseek( ifp->if_fd, FBC*4L, 0 ) == -1 ) {
		fb_log( "adage_device_open : lseek failed.\n" );
		return	-1;
	}
	if( write( ifp->if_fd, (char *)&ikfbc_setup[IKI(ifp)->mode],
	    sizeof(struct ik_fbc) ) != sizeof(struct ik_fbc) ) {
		fb_log( "adage_device_open : write failed.\n" );
		return	-1;
	}
	IKI(ifp)->ikfbcmem = ikfbc_setup[IKI(ifp)->mode];/* struct copy */

	/* Build an identity for the crossbar switch */
	for( i=0; i < 34; i++ )
#ifndef pdp11
		xbsval[i] = (long)i;
#else
		xbsval[i] = (((long)i)<<16);	/* word swap.. */
#endif
	if( lseek( ifp->if_fd, XBS*4L, 0 ) == -1 ) {
		fb_log( "adage_device_open : lseek failed.\n" );
		return	-1;
	}
	if( write( ifp->if_fd, (char *) xbsval, sizeof(xbsval) )
	    != sizeof(xbsval) ) {
		fb_log( "adage_device_open : write failed.\n" );
		return	-1;
	}

	/* Initialize the LUVO crossbar switch, too */
#ifndef pdp11
	xbsval[0] = 0x24L;		/* 1:1 mapping, magic number */
#else
	xbsval[0] = (0x24L<<16);
#endif
	if( lseek( ifp->if_fd, LUVOXBS*4L, 0 ) == -1 ) {
		fb_log( "adage_device_open : lseek failed.\n" );
		return	-1;
	}
	if( write( ifp->if_fd, (char *) xbsval, sizeof(long) )
	    != sizeof(long) ) {
		fb_log( "adage_device_open : write failed.\n" );
		return	-1;
	}

	/* Dump in default cursor. */
	if( adage_cinit_bitmap( ifp, cursor ) == -1 )
		return	-1;
	/* seek to start of pixels */
	if( lseek( ifp->if_fd, 0L, 0 ) == -1 ) {
		fb_log( "adage_device_open : lseek failed.\n" );
		return	-1;
	}
	/* Create pixel buffer */
	if( _pixbuf == NULL ) {
		if( (_pixbuf = malloc( ADAGE_DMA_BYTES )) == NULL ) {
			fb_log( "adage_device_open : pixbuf malloc failed.\n" );
			return	-1;
		}
	}
	IKI(ifp)->x_zoom = 1;
	IKI(ifp)->y_zoom = 1;
	IKI(ifp)->x_window = 0;
	IKI(ifp)->y_window = 0;
	return	ifp->if_fd;
}

_LOCAL_ int
adage_device_close( ifp )
FBIO	*ifp;
{
	/* free ikinfo struct */
	if( IKIL(ifp) != NULL )
		(void) free( IKIL(ifp) );

	return	close( ifp->if_fd );
}

_LOCAL_ int
adage_device_clear( ifp, bgpp )
FBIO	*ifp;
RGBpixel	*bgpp;
{

	/* If adage_device_clear() was called with non-black color, must
	 *  use DMAs to fill the frame buffer since there is no
	 *  hardware support for this.
	 */
	if( bgpp != NULL && ((*bgpp)[RED] != 0 || (*bgpp)[GRN] != 0 || (*bgpp)[BLU] != 0) )
		return	adage_color_clear( ifp, bgpp );

	IKI(ifp)->ikfbcmem.fbc_Lcontrol |= FBC_AUTOCLEAR;

	if( lseek( ifp->if_fd, FBC*4L, 0 ) == -1 ) {
		fb_log( "adage_device_clear : lseek failed.\n" );
		return	-1;
	}
	if( write( ifp->if_fd, &(IKI(ifp)->ikfbcmem), sizeof(struct ik_fbc) )
	    != sizeof(struct ik_fbc) ) {
		fb_log( "adage_device_clear : write failed.\n" );
		return	-1;
	}

	sleep( 1 );	/* Give the FBC a chance to act */
	IKI(ifp)->ikfbcmem.fbc_Lcontrol &= ~FBC_AUTOCLEAR;
	if( lseek( ifp->if_fd, FBC*4L, 0 ) == -1 ) {
		fb_log( "adage_device_clear : lseek failed.\n" );
		return	-1;
	}
	if( write( ifp->if_fd, &(IKI(ifp)->ikfbcmem), sizeof(struct ik_fbc) )
	    !=	sizeof(struct ik_fbc) ) {
		fb_log( "adage_device_clear : write failed.\n" );
		return	-1;
	}
	return	0;
}

/*
 *  Buffered Reads and Writes:
 *  We divide the users pixel buffer into three parts, as up to
 *  three seeks and DMA's are necessary to read or write this 1st
 *  quadrant buffer in Ikonas order.
 *
 *	[....................]			tailfrag
 *	[...............................]	fullscans
 *	[...............................]
 *	             [0.................]	headfrag
 *		      ^
 *		      + start of pixelp buffer.
 */

#define	IKSEEK(x,y)	if(lseek(ifp->if_fd,((y)*ifp->if_width+(x))\
	    		*sizeof(IKONASpixel),0) == -1) return -1;

_LOCAL_ int
adage_buffer_read( ifp, x, y, pixelp, count )
FBIO	*ifp;
int	x, y;
RGBpixel *pixelp;
long	count;
{
	register int i;
	register char *out, *in;
	int	headfrag, tailfrag, fullscans;
	int	scan, doscans;
	int	maxikscans;
	int	width, pixels, topiky;

	if( count == 1 )
		return adage_read_pio_pixel( ifp, x, y, pixelp );

	width = ifp->if_width;
	pixels = count;

	topiky = ifp->if_height - 1 - y;	/* 1st quadrant */
	topiky -= ( x + count - 1 ) / width;	/* first y on screen */
	if( x + count <= width ) {
		/* all on one line */
		headfrag = count;
		goto headin;
	}
	if( x != 0 ) {
		/* doesn't start of beginning of line => headfrag */
		headfrag = width - x;
		pixels -= headfrag;
	} else
		headfrag = 0;

	fullscans = pixels / width;
	tailfrag = pixels - fullscans * width;	/* remainder */

	if( tailfrag != 0 ) {
		IKSEEK( 0, topiky );
		topiky++;
		if( read( ifp->if_fd, _pixbuf, tailfrag*sizeof(IKONASpixel) )
		    != tailfrag*sizeof(IKONASpixel) )
			return	-1;
		out = (char *) &(pixelp[count-tailfrag][RED]);
		in = _pixbuf;
		for( i = tailfrag; i > 0; i-- ) {
			/* VAX subscripting faster than ++ */
			*out++ = *in;
			*out++ = in[1];
			*out++ = in[2];
			in += sizeof(IKONASpixel);
		}
	}
	/* Do the full scanlines */
	if( fullscans > 0 ) {
		maxikscans =  ADAGE_DMA_BYTES / (ifp->if_width*sizeof(IKONASpixel));
		out = (char *) &(pixelp[count-tailfrag-width][RED]);
		IKSEEK( 0, topiky );
		topiky += fullscans;
		while( fullscans > 0 ) {
			in = _pixbuf;
			doscans = fullscans > maxikscans ? maxikscans : fullscans;
			if( read( ifp->if_fd, _pixbuf, doscans*width*sizeof(IKONASpixel) )
			    != doscans*width*sizeof(IKONASpixel) )
				return	-1;
			for( scan = doscans; scan > 0; scan-- ) {
				for( i = width; i > 0; i-- ) {
					/* VAX subscripting faster than ++ */
					*out++ = *in;
					*out++ = in[1];
					*out++ = in[2];
					in += sizeof(IKONASpixel);
				}
				out -= (width << 1) * sizeof(RGBpixel);
			}
			fullscans -= doscans;
		}
	}
headin:
	if( headfrag != 0 ) {
		IKSEEK( x, topiky );
		out = (char *) pixelp;
		in = _pixbuf;
		if( read( ifp->if_fd, _pixbuf, headfrag*sizeof(IKONASpixel) )
		    != headfrag*sizeof(IKONASpixel) )
			return	-1;
		for( i = headfrag; i > 0; i-- ) {
			/* VAX subscripting faster than ++ */
			*out++ = *in;
			*out++ = in[1];
			*out++ = in[2];
			in += sizeof(IKONASpixel);
		}
	}
	return	count;
}

_LOCAL_ int
adage_buffer_write( ifp, x, y, pixelp, count )
FBIO	*ifp;
int	x, y;
RGBpixel *pixelp;
long	count;
{
	register int i;
	register char *out, *in;
	int	headfrag, tailfrag, fullscans;
	int	scan, doscans;
	int	maxikscans;
	int	width, pixels, topiky;

	if( count == 1 )
		return adage_write_pio_pixel( ifp, x, y, pixelp );

	width = ifp->if_width;
	pixels = count;

	topiky = ifp->if_height - 1 - y;	/* 1st quadrant */
	topiky -= ( x + count - 1 ) / width;	/* first y on screen */
	if( x + count <= width ) {
		/* all on one line */
		headfrag = count;
		goto headout;
	}
	if( x != 0 ) {
		/* doesn't start of beginning of line => headfrag */
		headfrag = width - x;
		pixels -= headfrag;
	} else
		headfrag = 0;

	fullscans = pixels / width;
	tailfrag = pixels - fullscans * width;	/* remainder */

	if( tailfrag != 0 ) {
		IKSEEK( 0, topiky );
		topiky++;
		in = (char *) &(pixelp[count-tailfrag][RED]);
		out = _pixbuf;
		for( i = tailfrag; i > 0; i-- ) {
			/* VAX subscripting faster than ++ */
			*out = *in++;
			out[1] = *in++;
			out[2] = *in++;
			out += sizeof(IKONASpixel);
		}
		if( write( ifp->if_fd, _pixbuf, tailfrag*sizeof(IKONASpixel) )
		    != tailfrag*sizeof(IKONASpixel) )
			return	-1;
	}
	/* Do the full scanlines */
	if( fullscans > 0 ) {
		maxikscans =  ADAGE_DMA_BYTES / (ifp->if_width*sizeof(IKONASpixel));
		in = (char *) &(pixelp[count-tailfrag-width][RED]);
		IKSEEK( 0, topiky );
		topiky += fullscans;
		while( fullscans > 0 ) {
			out = _pixbuf;
			doscans = fullscans > maxikscans ? maxikscans : fullscans;
			for( scan = doscans; scan > 0; scan-- ) {
				for( i = width; i > 0; i-- ) {
					/* VAX subscripting faster than ++ */
					*out = *in++;
					out[1] = *in++;
					out[2] = *in++;
					out += sizeof(IKONASpixel);
				}
				in -= (width << 1) * sizeof(RGBpixel);
			}
			if( write( ifp->if_fd, _pixbuf, doscans*width*sizeof(IKONASpixel) )
			    != doscans*width*sizeof(IKONASpixel) )
				return	-1;
			fullscans -= doscans;
		}
	}
headout:
	if( headfrag != 0 ) {
		IKSEEK( x, topiky );
		in = (char *) pixelp;
		out = _pixbuf;
		for( i = headfrag; i > 0; i-- ) {
			/* VAX subscripting faster than ++ */
			*out = *in++;
			out[1] = *in++;
			out[2] = *in++;
			out += sizeof(IKONASpixel);
		}
		if( write( ifp->if_fd, _pixbuf, headfrag*sizeof(IKONASpixel) )
		    != headfrag*sizeof(IKONASpixel) )
			return	-1;
	}
	return	count;
}

/* Write 1 Ikonas pixel using PIO rather than DMA */
_LOCAL_ int
adage_write_pio_pixel( ifp, x, y, datap )
FBIO	*ifp;
int	x, y;
RGBpixel	*datap;
{
	register int i;
	register struct ikdevice *ikp = (struct ikdevice *)(IKI(ifp)->_ikUBaddr);
	long data = 0;

	((unsigned char *)&data)[RED] = (*datap)[RED];
	((unsigned char *)&data)[GRN] = (*datap)[GRN];
	((unsigned char *)&data)[BLU] = (*datap)[BLU];

	y = ifp->if_width-1-y;		/* 1st quadrant */
	i = 10000;
	while( i-- && !(ikp->ubcomreg & IKREADY) )  /* NULL */ 	;
	if( i == 0 ) {
		fb_log( "IK READY stayed low.\n" );
		return	-1;
	}

	if( ifp->if_width == 1024 ) {
		ikp->ikcomreg = IKPIX | IKWRITE | IKINCR | IKHIRES;
		ikp->ikloaddr = x;
		ikp->ikhiaddr = y;
	} else {
		ikp->ikcomreg = IKPIX | IKWRITE | IKINCR;
		ikp->ikloaddr = x<<1;
		ikp->ikhiaddr = y<<1;
	}
	ikp->ubcomreg = 1;			/* GO */
	ikp->datareg = (u_short)data;
	ikp->datareg = (u_short)(data>>16);
	if( ikp->ubcomreg & IKERROR ) {
		fb_log( "IK ERROR bit on PIO.\n" );
		return	-1;
	}
	return	1;
}

/* Read 1 Ikonas pixel using PIO rather than DMA */
_LOCAL_ int
adage_read_pio_pixel( ifp, x, y, datap )
FBIO	*ifp;
int	x, y;
RGBpixel	*datap;
{
	register int i;
	register struct ikdevice *ikp = (struct ikdevice *)(IKI(ifp)->_ikUBaddr);
	long data;

	y = ifp->if_width-1-y;		/* 1st quadrant */
	i = 10000;
	while( i-- && !(ikp->ubcomreg & IKREADY) )  /* NULL */ 	;
	if( i == 0 ) {
		fb_log( "IK READY stayed low (setup).\n" );
		return	-1;
	}

	if( ifp->if_width == 1024 ) {
		ikp->ikcomreg = IKPIX | IKINCR | IKHIRES;
		ikp->ikloaddr = x;
		ikp->ikhiaddr = y;
	} else {
		ikp->ikcomreg = IKPIX | IKINCR;
		ikp->ikloaddr = x<<1;
		ikp->ikhiaddr = y<<1;
	}
	ikp->ubcomreg = 1;			/* GO */

	i = 10000;
	while( i-- && !(ikp->ubcomreg & IKREADY) )  /* NULL */ 	;
	if( i == 0 ) {
		fb_log( "IK READY stayed low (after).\n" );
		return	-1;
	}
	data = ikp->datareg;			/* low */
	data |= (((long)ikp->datareg)<<16);	/* high */
	if( ikp->ubcomreg & IKERROR ) {
		fb_log( "IK ERROR bit on PIO.\n" );
		return	-1;
	}
	(*datap)[RED] = ((unsigned char *)&data)[RED];
	(*datap)[GRN] = ((unsigned char *)&data)[GRN];
	(*datap)[BLU] = ((unsigned char *)&data)[BLU];
	return	1;
}

/*	a d a g e _ z o o m _ s e t ( )
	Update fbc_[xy]zoom registers in FBC
 */
_LOCAL_ int
adage_zoom_set( ifp, x, y )
FBIO	*ifp;
register int	x, y;
{

	/*
	 *  While page 5-6 claims that the zoom range is 1..256:1,
	 *  testing demonstrates that the actual range is 1..16:1.
	 */
	if( x < 1 )  x=1;
	if( y < 1 )  y=1;
	if( x > 16 )  x=16;
	if( y > 16 )  y=16;

	IKI(ifp)->x_zoom = x;
	IKI(ifp)->y_zoom = y;

	/*
	 * From RDS 3000 Programming Reference Manual, June 1982, section
	 * 5.3 Notes, page 5-12.
	 * In HIRES mode, horizontal zoom must be accomplished as follows:
	 * 1.   To go from a ratio of 1:1 to 2:1 you must double the
	 * 	pixel clock rate, rather than use the zoom register.
	 * 2.   Thereafter you can increment the zoom register, while
	 * 	leaving the pixel clock rate doubled.
	 */
	if( IKI(ifp)->mode == 2 )  {
		if( x > 1 )  {
			/* PIXELCLOCK rate experimentally determined as 41 */
			IKI(ifp)->ikfbcmem.fbc_Hcontrol =
				FBCH_PIXELCLOCK(41) | FBCH_DRIVEBPCK;
			if( x >= 4 )
				IKI(ifp)->ikfbcmem.fbc_xzoom = (x>>1)-1;
			else
				IKI(ifp)->ikfbcmem.fbc_xzoom = 0;
			if( x & 1 )  fb_log("Unable to do odd X zooms properly in HIRES\n");
		} else {
			IKI(ifp)->ikfbcmem.fbc_Hcontrol =
				ikfbc_setup[2].fbc_Hcontrol;
			IKI(ifp)->ikfbcmem.fbc_xzoom = 0;
		}
		if( lseek( ifp->if_fd, FBCVC*4L, 0 ) == -1 ||
		    write( ifp->if_fd, &(IKI(ifp)->ikfbcmem.fbc_Lcontrol), 4 ) != 4 ) {
			fb_log( "adage_zoom_set : FBCVC write failed.\n" );
			return	-1;
		}
		
	} else {
		IKI(ifp)->ikfbcmem.fbc_xzoom = x-1;
	}
	IKI(ifp)->ikfbcmem.fbc_yzoom = y-1;	/* replication count */

	if( lseek( ifp->if_fd, FBCZOOM*4L, 0 ) == -1 ) {
		fb_log( "adage_zoom_set : lseek failed.\n" );
		return	-1;
	}
	if( write( ifp->if_fd, &(IKI(ifp)->ikfbcmem.fbc_xzoom), 4 ) != 4 ) {
		fb_log( "adage_zoom_set : FBCZOOM write failed.\n" );
		return	-1;
	}
	return	0;
}

static int
imax( a, b )
{
	if( a > b )
		return(a);
	return(b);
}

/*			a d a g e _ w i n d o w _ s e t ( )
 *
 *	Set FBC window location to specified values so that <x,y> are
 *	at screen center given current zoom.
 */
_LOCAL_ int
adage_window_set( ifp, x, y )
register FBIO	*ifp;
int	x, y;
{
	int ikx, iky;		/* upper left corner of view rectangle */
	int y_viewport;
	int y_window;
	int first_line;
	int top_margin;

	/*
	 *  To start with, we are given the 1st quadrant coordinates
	 *  of the CENTER of the region we wish to view.  Since the
	 *  Ikonas window is specified in terms of the upper left
	 *  corner, first find the upper left corner of the rectangle
	 *  to window in on, accounting for the zoom factor too.
	 *  Then convert from first to fourth for the Ikonas.
	 *  The order of these conversions is significant.
	 */
	ikx = x - (ifp->if_width / IKI(ifp)->x_zoom)/2;
	iky = y + (ifp->if_height / IKI(ifp)->y_zoom)/2 - 1;
	iky = ifp->if_height-1-iky;		/* q1 -> q4 */

	/* for the cursor routines, save q4 upper left */
	IKI(ifp)->x_window = ikx;
	IKI(ifp)->y_window = iky;

	/*
 	 *  These formulas are taken from section 5.2.3.2.4 (page 5-5)
	 *  of the Adage RDS-3000 programming reference manual, June 1982.
	 *  Note that the published magic numbers are off by one.
	 */
	first_line = 0;
	y_viewport = IKI(ifp)->ikfbcmem.fbc_yviewport;

	switch( IKI(ifp)->mode )  {
	case 0:
		top_margin = imax( 35, y_viewport+4 );
		y_window = first_line - top_margin + 7;
		break;
	case 1:
		top_margin = imax( 34, (y_viewport+4)/2 );
		y_window = first_line - top_margin + 3;		/* was 4 */
		break;
	case 2:
		top_margin = imax( 69, y_viewport+4 );
		y_window = first_line - top_margin + 6;		/* was 9 */
		break;
	}
	IKI(ifp)->y_winoff = y_window;
	y_window /= IKI(ifp)->y_zoom;

	if( IKI(ifp)->mode != 2 )
		IKI(ifp)->ikfbcmem.fbc_xwindow = ikx << 2;	/* lores */
	else
		IKI(ifp)->ikfbcmem.fbc_xwindow = ikx;		/* hires */

	IKI(ifp)->ikfbcmem.fbc_ywindow = iky + y_window;

	if( lseek( ifp->if_fd, FBCWL*4L, 0 ) == -1 ) {
		fb_log( "adage_window_set : lseek failed.\n" );
		return	-1;
	}

	if( write( ifp->if_fd, &(IKI(ifp)->ikfbcmem.fbc_xwindow), 4 ) != 4 ) {
		fb_log( "adage_window_set : write failed.\n" );
		return	-1;
	}
	return	0;
}

/*	a d a g e _ c u r s o r _ m o v e _ m e m o r y _ a d d r ( )
 *
 *	Place cursor at image (pixel) coordinates x and y.
 *	IMPORTANT : Adage cursor addressing is in screen space,
 *	so backwards correction must be applied.
 */
_LOCAL_ int
adage_cmemory_addr( ifp, mode, x, y )
FBIO	*ifp;
int	mode;
int	x, y;
{
#ifdef never
	register int	x_cursor_offset, y_cursor_offset;
	int x_origin = ifp->if_width / 2;	/* Odd formulas */
	int y_origin = ifp->if_height / 2;

	y = ifp->if_width-1-y;		/* 1st quadrant */
	/* Map image coordinates to screen space.			*/
	if( ifp->if_width == 1024 ) {
		switch( IKI(ifp)->x_zoom ) {
		case 16 :
			x_cursor_offset = 9;
			y_cursor_offset = 39;
			break;
		case 8 :
			x_cursor_offset = -4;
			y_cursor_offset = 43;
			break;
		case 4 :
			x_cursor_offset = -10;
			y_cursor_offset = 44;
			break;
		case 2 :
			x_cursor_offset = -17;
			y_cursor_offset = 45;
			break;
		case 1 :
			x_cursor_offset = -16;
			y_cursor_offset = 46;
			break;
		}
	} else {
		x_cursor_offset = X_CURSOR_OFFSET;
		y_cursor_offset = Y_CURSOR_OFFSET;
	}
	x = x_origin + ((x - x_origin)
		- IKI(ifp)->x_window)*IKI(ifp)->x_zoom + x_cursor_offset;
	y = y_origin + ((y - y_origin)
		- IKI(ifp)->y_window)*IKI(ifp)->y_zoom + y_cursor_offset;
#endif

	y = ifp->if_height-1-y;		/* q1 -> q4 */
	y -= IKI(ifp)->y_window;
	x -= IKI(ifp)->x_window;

	if( y < 0 )  y = 0;
	if( x < 0 )  x = 0;
#ifdef never
	if( x >= ifp->if_width/IKI(ifp)->x_zoom-1 )
		x = ifp->if_width/IKI(ifp)->x_zoom-1;
	if( y >= ifp->if_height/IKI(ifp)->y_zoom-1 )
		y = ifp->if_height/IKI(ifp)->y_zoom-1;
	y -= IKI(ifp)->y_winoff;
#else
	y *= IKI(ifp)->y_zoom;
	x *= IKI(ifp)->x_zoom;
	y -= IKI(ifp)->y_winoff;
	y += (IKI(ifp)->y_zoom-1)*2;	/* HACK to correct drift */
#endif

	if( mode )
		IKI(ifp)->ikfbcmem.fbc_Lcontrol |= FBC_CURSOR;
	else
		IKI(ifp)->ikfbcmem.fbc_Lcontrol &= ~FBC_CURSOR;
	IKI(ifp)->ikfbcmem.fbc_xcursor = x&01777;
	IKI(ifp)->ikfbcmem.fbc_ycursor = y&01777;

	if( lseek( ifp->if_fd, FBCVC*4L, 0 ) == -1 ) {
		fb_log( "adage_cmemory_addr : lseek failed.\n" );
		return	-1;
	}
	if( write( ifp->if_fd, &(IKI(ifp)->ikfbcmem.fbc_Lcontrol), 8 ) != 8 ) {
		fb_log( "adage_cmemory_addr : write failed.\n" );
		return	-1;
	}
	return	0;
}

/*	a d a g e _ c u r s o r _ m o v e _ s c r e e n _ a d d r ( )
	Place cursor at SCREEN coordinates x and y.
 */
_LOCAL_ int
adage_cscreen_addr( ifp, mode, x, y )
FBIO	*ifp;
int	mode;
int	x, y;
{
	y = ifp->if_width-1-y;		/* 1st quadrant */
	if( ifp->if_width == 1024 && IKI(ifp)->y_zoom == 1 )
		y += 30;
	if (mode)
		IKI(ifp)->ikfbcmem.fbc_Lcontrol |= FBC_CURSOR;
	else
		IKI(ifp)->ikfbcmem.fbc_Lcontrol &= ~FBC_CURSOR;
	IKI(ifp)->ikfbcmem.fbc_xcursor = x&01777;
	IKI(ifp)->ikfbcmem.fbc_ycursor = y&01777;

	if( lseek( ifp->if_fd, FBCVC*4L, 0 ) == -1 ) {
		fb_log( "adage_cscreen_addr : lseek failed.\n" );
		return	-1;
	}
	if( write( ifp->if_fd, &(IKI(ifp)->ikfbcmem.fbc_Lcontrol), 8 ) != 8 ) {
		fb_log( "adage_cscreen_addr : write failed.\n" );
		return	-1;
	}
	return	0;
}

_LOCAL_ int
adage_cinit_bitmap( ifp, bitmap )
FBIO	*ifp;
long	*bitmap;
{
	register int i;
	long	cursorarray[256];

#ifdef pdp11
	for(i = 0; i < 256; i++)
		cursorarray[i] = ((bitmap[i/8]>>((i%8)*4))&017L)<<16;
#else
	for (i = 0; i < 256; i++)
		cursorarray[i] = (bitmap[i/8]>>((i%8)*4))&017L;
#endif
	if( lseek( ifp->if_fd, FBCCD*4L, 0 ) == -1 ) {
		fb_log( "adage_cinit_bitmap : lseek failed.\n" );
		return	-1;
	}
	if( write( ifp->if_fd, cursorarray, 1024 ) != 1024 ) {
		fb_log( "adage_cinit_bitmap : write failed.\n" );
		return	-1;
	}
	return	0;
}

#ifdef never
/*	Write one color map entry.
	Page selects the color map; 0, 1, 2, or  3.
	Offset indexes into the map.
 */
_LOCAL_ int
adage_colormap_write_entry( ifp, cp, page, offset )
FBIO	*ifp;
register RGBpixel	*cp;
long	page, offset;
{
	long	lp;

	lp = RGB10( (*cp)[RED]>>6, (*cp)[GRN]>>6, (*cp)[BLU]>>6 );
	lseek( ifp->if_fd, (LUVO + page*256 + offset)*4L, 0);
	if( write( ifp->if_fd, (char *) &lp, 4 ) != 4 ) {
		fb_log( "adage_colormap_write_entry : write failed.\n" );
		return	-1;
	}
	return	0;
}
#endif never

_LOCAL_ int
adage_colormap_write( ifp, cp )
FBIO	*ifp;
register ColorMap	*cp;
{
	long cmap[1024];
	register int i, j;

	/* Note that RGB10(r,g,b) flips to cmap order (b,g,r). */
	if( cp == (ColorMap *) NULL )  {
		for( i=0; i < 256; i++ )  {
			j = i<<2;
			cmap[i] = RGB10( j, j, j );
			j = ((i+128)%255)<<2;
			cmap[i+256] = RGB10( j, j, j );
		}
	}  else  {
		for( i=0; i < 256; i++ )  {
			cmap[i] = RGB10( cp->cm_red[i]>>6,
				       cp->cm_green[i]>>6,
				       cp->cm_blue[i]>>6 );
			cmap[i+256] = RGB10( ((cp->cm_red[i]>>6)+512)%1023,
				       ((cp->cm_green[i]>>6)+512)%1023,
				       ((cp->cm_blue[i]>>6)+512)%1023 );
		}
	}

#ifdef pdp11
	/* 16-bit-word-in-long flipping for PDP's */
	for( i=0; i < 512; i++ ) {
		register struct twiddle {
			short rhs;
			short lhs;
		}  *cmp = &cmap[i];
		register short temp;
		temp = cmp->rhs;
		cmp->rhs = cmp->lhs;
		cmp->lhs = temp;
	}
#endif
	/*
	 * Replicate first copy of color map onto second copy,
	 * and also do the "overlay" portion too.
	 */
	for( i=0; i < 256*2; i++ ) {
		cmap[i+512] = cmap[i];
	}
	if( lseek( ifp->if_fd, LUVO*4L, 0) == -1 ) {
		fb_log( "adage_colormap_write : lseek failed.\n" );
		return	-1;
	}
	if( write( ifp->if_fd, cmap, 1024*4 ) != 1024*4 ) {
		fb_log( "adage_colormap_write : write failed.\n" );
		return	-1;
	}
	return	0;
}

_LOCAL_ int
adage_colormap_read( ifp, cp )
FBIO	*ifp;
register ColorMap	*cp;
{
	register int i;
	long cmap[1024];

	if( lseek( ifp->if_fd, LUVO*4L, 0) == -1 ) {
		fb_log( "adage_colormap_read : lseek failed.\n" );
		return	-1;
	}
	if( read( ifp->if_fd, cmap, 1024*4 ) != 1024*4 ) {
		fb_log( "adage_colormap_read : read failed.\n" );
		return	-1;
	}
#ifndef pdp11
	for( i=0; i < 256; i++ ) {
		cp->cm_red[i] = (cmap[i]<<(6+0))  & 0xFFC0;
		cp->cm_green[i] = (cmap[i]>>(10-6))  & 0xFFC0;
		cp->cm_blue[i] = (cmap[i]>>(20-6)) & 0xFFC0;
	}
#else
	for( i=0; i < 256; i++ ) {
		register struct twiddle {
			short rhs;
			short lhs;
		}  *cmp = &cmap[i];
		cp->cm_red[i] = (cmp->rhs & 0x3FF) << 6;
		cp->cm_green[i] = (((cmp->lhs<<6)&0x3C0) | ((cmp->rhs>>10)&0x3F)) << 6;
		cp->cm_blue[i] = ((cmp->lhs>>4) & 0x3FF) << 6;
	}
#endif
	return	0;
}

/*
 *		A D A G E _ C O L O R _ C L E A R
 *
 *  Clear the frame buffer to the given color.  There is no hardware
 *  Support for this so we do it via large DMA's.
 */
_LOCAL_ int
adage_color_clear( ifp, bpp )
register FBIO	*ifp;
register RGBpixel	*bpp;
{
	register IKONASpixel *pix_to;
	register long	i;
	long	pixelstodo;
	int	fd;

	/* Fill buffer with background color. */
	for( i = ADAGE_DMA_PIXELS, pix_to = (IKONASpixel *)_pixbuf; i > 0; i-- ) {
		COPYRGB( *pix_to, *bpp );
		pix_to++;
	}

	/* Set start of framebuffer */
	fd = ifp->if_fd;
	if( lseek( fd, 0L, 0 ) == -1 ) {
		fb_log( "adage_color_clear : seek failed.\n" );
		return	-1;
	}

	/* Send until frame buffer is full. */
	pixelstodo = ifp->if_height * ifp->if_width;
	while( pixelstodo > 0 ) {
		i = pixelstodo > ADAGE_DMA_PIXELS ? ADAGE_DMA_PIXELS : pixelstodo;
		if( write( fd, _pixbuf, i*sizeof(IKONASpixel) ) == -1 )
			return	-1;
		pixelstodo -= i;
	}

	return	0;
}
