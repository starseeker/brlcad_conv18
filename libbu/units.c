/*
 *			U N I T S . C
 * 
 *  Module of libbu to handle units conversion between strings and mm.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>
#include <ctype.h>

#include "machine.h"
#include "externs.h"	/* needed for things like strtod() */
#include "bu.h"

static CONST struct cvt_tab {
	double	val;
	char	name[32];
} bu_units_tab[] = {
	0.0,		"none",
	1.0e-7,		"angstrom",
	1.0e-6,		"nm",
	1.0e-6,		"nanometer",
	1.0e-3,		"um",
	1.0e-3,		"micrometer",
	1.0e-3,		"micron",
	1.0,		"mm",
	1.0,		"millimeter",
	10.0,		"cm",
	10.0,		"centimeter",
	1000.0,		"m",
	1000.0,		"meter",
	1000000.0,	"km",
	1000000.0,	"kilometer",
	25.4,		"in",
	25.4,		"inch",
	25.4,		"inche",		/* for plural */
	304.8,		"ft",
	304.8,		"foot",
	304.8,		"feet",
	456.2,		"cubit",
	914.4,		"yd",
	914.4,		"yard",
	5029.2,		"rd",
	5029.2,		"rod",
	1609344.0,	"mi",
	1609344.0,	"mile",
	1852000.0,	"nmile",
	1852000.0,	"nautical mile",
	1.495979e+14,	"AU",
	1.495979e+14,	"astronomical unit",
	9.460730e+18,	"lightyear",
	3.085678e+19,	"pc",
	3.085678e+19,	"parsec",
	0.0,		""			/* LAST ENTRY */
};
#define BU_UNITS_TABLE_SIZE (sizeof(bu_units_tab) / sizeof(struct cvt_tab) - 1)

/*
 *			B U _ U N I T S _ C O N V E R S I O N
 *
 *  Given a string representation of a unit of distance (eg, "feet"),
 *  return the multiplier which will convert that unit into millimeters.
 *
 *  Returns -
 *	0.0	error
 *	>0.0	success
 */
double
bu_units_conversion(str)
CONST char	*str;
{
	register char	*ip;
	register int	c;
	register CONST struct cvt_tab	*tp;
	char		ubuf[64];
	int		len;

	strncpy( ubuf, str, sizeof(ubuf)-1 );
	ubuf[sizeof(ubuf)-1] = '\0';

	/* Copy the given string, making it lower case */
	ip = ubuf;
	while( (c = *ip) )  {
		if( !isascii(c) )
			*ip++ = '_';
		else if( isupper(c) )
			*ip++ = tolower(c);
		else
			ip++;
	}

	/* Remove any trailing "s" (plural) */
	len = strlen(ubuf);
	if( ubuf[len-1] == 's' )  ubuf[len-1] = '\0';

	/* Search for this string in the table */
	for( tp=bu_units_tab; tp->name[0]; tp++ )  {
		if( ubuf[0] != tp->name[0] )  continue;
		if( strcmp( ubuf, tp->name ) != 0 )  continue;
		return( tp->val );
	}
	return(0.0);		/* Unable to find it */
}

CONST char *
bu_units_string(mm)
register CONST double	mm;
{
	register CONST struct cvt_tab	*tp;

	/* Search for this string in the table */
	for( tp=bu_units_tab; tp->name[0]; tp++ )  {
		if( mm != tp->val )  continue;
		return( tp->name );
	}
	return (char *)0;
}

/* Given a string of the form "25cm" or "5.2ft" returns the 
 * corresponding distance in mm
 */
double
bu_mm_value(s)
CONST char *s;
{
	double v;
	char *ptr;	
	register CONST struct cvt_tab	*tp;

	v = strtod(s, &ptr);

	if (ptr == s) return 0.0;
	if ( ! *ptr) return v;

	for (tp=bu_units_tab; tp->name[0]; tp++ )  {
		if( *ptr != tp->name[0] )  continue;
		if( strcmp( ptr, tp->name ) == 0 ) {
			v *= tp->val;
			break;
		}
	}

	return v;
}
/*	B U _ M M _ C V T
 *
 *  Used primarily as a hooked function for bu_structparse tables
 *  to allow input of floating point values in other units.
 */
void
bu_mm_cvt( sdp, name, base, value )
register CONST struct bu_structparse	*sdp;	/* structure description */
register CONST char			*name;	/* struct member name */
char					*base;	/* begining of structure */
CONST char				*value;	/* string containing value */
{
	register double *p = (double *)(base+sdp->sp_offset);

	/* reconvert with optional units */
	*p = bu_mm_value(value);
}

#if 0
CONST char *
bu_units_id_string(id)
	int id;
{
	if (id >= BU_UNITS_TABLE_SIZE)
		return bu_units_tab[0].name; /* none */

	return bu_units_tab[id].name;
}

int
bu_units_id_valid(id)
	int id;
{
	if (0 <= id && id < BU_UNITS_TABLE_SIZE)
		return 1;	/* valid */

	return 0;		/* not valid */
}
#endif
