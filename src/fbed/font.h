/*                          F O N T . H
 * BRL-CAD
 *
 * Copyright (c) 2004-2008 United States Government as represented by
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
 */
/** @file font.h
 *
 * Header file for putting fonts up.
 *
 */

#include "common.h"

#ifndef FBED_FONT_H
#define FBED_FONT_H

#if defined(WORDS_BIGENDIAN)
#  define SWAB(shrt)	(shrt=(((shrt)>>8) & 0xff) | (((shrt)<<8) & 0xff00))
#  define SWABV(shrt)	((((shrt)>>8) & 0xff) | (((shrt)<<8) & 0xff00))
#else
#  define SWAB(shrt)
#  define SWABV(shrt)	(shrt)
#endif

#define FONTBUFSZ 200
#ifndef FONTDIR
#  define FONTDIR	"/usr/lib/vfont" /* default font directory */
#endif
#define FONTNAME	"nonie.r.12"	 /* default font name */
#define FONTNAMESZ	128

#define SIGNBIT		(1<<7)
#define SIGNMASK	~SIGNBIT
#define TWOSCOMP(chr)	((~(chr)&0xff)+1)

#if defined(mips)
#  define CHARS_UNSIGNED_ONLY
#endif
#ifdef CHARS_UNSIGNED_ONLY
#  define SignedChar(chr)	(((chr)&SIGNBIT) ? -TWOSCOMP(chr) : (chr))
#else
#  define SignedChar(chr)	chr
#endif

/*	vfont.h	4.1	83/05/03 from 4.2 Berkley */
/* The structures header and dispatch define the format of a font file. */
struct header {
	short		magic;
	unsigned short	size;
	short		maxx;
	short		maxy;
	short		xtend;
};
struct dispatch
	{
	unsigned short	addr;
	short		nbytes;
	char up, down, left, right;
	short		width;
	};

/* Variables controlling the font itself. */
extern FILE *ffdes;		/* Fontfile file descriptor. */
extern long offset;		/* Offset to data in the file. */
extern struct header hdr;	/* Header for font file. */
extern struct dispatch dir[256];/* Directory for character font. */
extern int width, height;	/* Width and height of current char. */

#endif /* FBED_FONT_H */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
