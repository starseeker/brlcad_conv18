/*                           T B L . H
 * BRL-CAD
 *
 * Copyright (c) 2020 United States Government as represented by
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

#ifndef BU_TBL_H
#define BU_TBL_H

#include "common.h"

#include "bu/defines.h"
#include "bu/vls.h"


__BEGIN_DECLS

/**
 * @addtogroup bu_tbl
 *
 * @brief Routines for generally (i.e., non-mathematically) handling
 * numbers, strings, and other data for tabular printing.
 */
/** @{ */
/** @file bu/tbl.h */

/**
 * this is pulled from num.c where the guts to the table printer
 * currently resides, moved here for easy reference.
 *
 @code
 double vals[16] = {-1.123123123123, 0, 0, 1.0/0.0, 0, 123, 0, 0, 0, -2345, 123123.123123123123, 0, 123123123.123123123, 21, 1.0/0.0, 1};


 bu_num_print(vals, 16, 4, "my matrix\n[\n", "\t[", NULL, ", ", "]\n", "]\n]\n");
 -------------------------------------------------------------------------------
my matrix
[
	[-1.1231231231229999,     0,                  0, inf]
	[                  0,   123,                  0,   0]
	[                  0, -2345, 123123.12312312312,   0]
	[ 123123123.12312312,    21,                inf,   1]
]

 bu_num_print(vals+5, 3, 3, "POINT { ", NULL, NULL, " ", NULL, " }\n");
 ---------------------------------------------------------------------
POINT { 123 0 0 }

 bu_num_print(vals, 16, 4, "MATRIX [", " [", NULL, ", ", "]\n", "] ]\n");
 -----------------------------------------------------------------------
MATRIX [ [-1.1231231231229999,     0,                  0, inf]
         [                  0,   123,                  0,   0]
         [                  0, -2345, 123123.12312312312,   0]
         [ 123123123.12312312,    21,                inf,   1] ]

 bu_num_print(vals, 16, 4, NULL, NULL, NULL, " ", "    ", "\n");
 --------------------------------------------------------------
-1.1231231231229999 0 0 inf    0 123 0 0    0 -2345 123123.12312312312 0    123123123.12312312 21 inf 1

 bu_num_print(vals, 4, 4, NULL, NULL, NULL, NULL, NULL, NULL);
 ------------------------------------------------------------
-1.1231231231229999 0 0 inf

 bu_num_print(vals, 16, 4, NULL, NULL, "%017.2lf", " ", "\n", "\n");
 ------------------------------------------------------------------
-0000000000001.12 00000000000000.00 00000000000000.00               inf
00000000000000.00 00000000000123.00 00000000000000.00 00000000000000.00
00000000000000.00 -0000000002345.00 00000000123123.12 00000000000000.00
00000123123123.12 00000000000021.00               inf 00000000000001.00

 @endcode
 *
 */


struct bu_tbl;


/**
 * static-initializer for bu_tbl objects on the stack
 */
#define BU_TBL_INIT_ZERO {0}

/**
 * always returns a pointer to a newly allocated table
 */
BU_EXPORT extern struct bu_tbl *
bu_tbl_create();


/**
 * releases all dynamic memory associated with the specified table
 */
BU_EXPORT extern void
bu_tbl_destroy(struct bu_tbl *);


/**
 * erases all cells in a table, but doesn't erase the table itself
 */
BU_EXPORT int
bu_tbl_clear(struct bu_tbl *);


enum bu_tbl_style {
    BU_TBL_BORDER_NONE,
    BU_TBL_BORDER_SINGLE,
    BU_TBL_BORDER_DOUBLE,
    BU_TBL_ALIGN_LEFT,
    BU_TBL_ALIGN_CENTER,
    BU_TBL_ALIGN_RIGHT,
    BU_TBL_END
};


/**
 * sets table styling or formatting on cells printed next.
 */
BU_EXPORT struct bu_tbl *
bu_tbl_style(struct bu_tbl *, enum bu_tbl_style);


/**
 * set the table insertion point.
 */
BU_EXPORT struct bu_tbl *
bu_tbl_index(struct bu_tbl *, size_t row, size_t col);


/**
 * print values into the table at the current insertion point.
 *
 * each column of the 'fmt' printf-style format specfier must be
 * delimited by a '|' character.
 *
 * callers can specify bu_tbl_style parameters as arguments after
 * 'fmt' in order to affect cell presentation.  BU_TBL_END denotes the
 * end of a row.  any existing values will be overwritten.
 */
BU_EXPORT struct bu_tbl *
bu_tbl_printf(struct bu_tbl *, const char *fmt, ...);


/**
 * print a table into a vls
 */
BU_EXPORT extern void
bu_tbl_vls(struct bu_vls *str, const struct bu_tbl *t);


__END_DECLS

/** @} */

#endif  /* BU_NUM_H */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
