/*                   T E S T _ S S C A N F . C
 * BRL-CAD
 *
 * Copyright (c) 2012 United States Government as represented by
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
/** @file test_sscanf.c
 *
 * Test bu_sscanf routine.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

#include "bu.h"
#include "vmath.h"

#define STR_SIZE 128
#define FLOAT_TOL .0005

/* scanf specification summarized from Linux man page:
 *
 * If processing of a directive fails, no further input is read, and
 * scanf returns.
 *
 * Returns number of input items successfully matched and assigned, or
 * EOF if a read error occurs (errno and stream error indicator are set),
 * EOI is encountered before the first successful conversion, or a
 * matching failure occurs.
 *
 * Can match 0 or more whitespace chars, ordinary character, or a
 * conversion specification:
 *
 * %[*][a][max_field_width][type_modifier]<conversion_specifier>
 *
 * '*' suppresses assignment. No pointer is needed, input is discarded,
 *     and conversion isn't added to the successful conversion count.
 *
 * 'a' in GNU, this represents an extension that automatically allocates
 *     a string large enough to hold a converted string and assigns the
 *     address to the supplied char*. In C99, a is synonymous with e/E/f/g.
 *
 * max_field_width - scan stops stops reading for a conversion at the
 *     first non-matching character or when max_field_width is reached
 *     (not including discarded leading whitespace or the '\0' appended
 *     to string conversions).
 *
 * modifiers:
 *  h - expect (signed|unsigned) short int (not int) pointer
 * hh - expect (signed|unsigned) char (not int) pointer
 *  l - expect (signed|unsigned) long int (not int) pointer, or double (not
 *      float), or wide char (not char) pointer
 *  L - expect long double (not double) pointer
 *
 * C99 modifiers:
 *  j - expect (signed|unsigned) intmax_t or uintmax_t (not int) pointer
 *  t - expect ptrdiff_t (not int) pointer
 *  z - expect size_t (not int) pointer
 *
 * Note: the "long long" type extension is specified with 'q' for 4.4BSD,
 *       and 'll' or 'L' for GNU.
 *
 * conversion specifiers:
 *  % - NOT A CONVERSION. Literal %.
 *  d - signed/unsigned decimal integer
 *  i - Signed/unsigned base 8/10/16 integer. Base 16 chose if 0x or 0X is
 *      read first, base 8 if 0 is read first, and base 10 otherwise. Only
 *      characters valid for the base are read.
 *  o - unsigned octal
 *  u - unsigned decimal
 *x/X - unsigned hexadecimal
 *a/e/E/f/g - signed/unsigned float (a is C99)
 *  s - Match a sequence of non-white-space characters. Pointer must be
 *      large enough for all characters plus the '\0' scanf appends.
 *  c - Match a sequence (1 by default) of characters. Pointer must be large
 *      enough for all characters. No '\0' is appended. Leading white space
 *      IS NOT discarded.
 *[...] - Like c, but only accept the specified character class. Leading white
 *        space IS NOT discarded. To include ']' in the set, it must appear
 *        first after '[' or '^'. To include '-' in the set, it must appear
 *        last before ']'.
 *  p - pointer (printf format)
 *  n - NOT A CONVERSION. Stores number of characters read so far in the int
 *      pointer. PROBABLY shouldn't affect returned conversion count.
 */

enum {
    INT, UINT, SHORT, USHORT, SHORTSHORT, USHORTSHORT, LONG, ULONG,
    FLOAT, DOUBLE, LDOUBLE
};

static void
test_sscanf(int type, const char *src, const char *fmt) {
    int ret, bu_ret;
    void *val, *bu_val;

    ret = bu_ret = 0;
    val = bu_val = NULL;

    printf("%s, %s\n", src, fmt);

    /* call sscanf and bu_sscanf with appropriately cast pointers */
#define SSCANF_TYPE(type) \
    val = bu_malloc(sizeof(type), "test_sscanf val"); \
    bu_val = bu_malloc(sizeof(type), "test_sscanf bu_val"); \
    ret = sscanf(src, fmt, (type*)val); \
    bu_ret = bu_sscanf(src, fmt, (type*)bu_val);

    switch (type) {
    case INT:
	SSCANF_TYPE(int);
	break;
    case UINT:
	SSCANF_TYPE(unsigned);
	break;
    case SHORT:
	SSCANF_TYPE(short);
	break;
    case USHORT:
	SSCANF_TYPE(unsigned short);
	break;
    case SHORTSHORT:
	SSCANF_TYPE(char);
	break;
    case USHORTSHORT:
	SSCANF_TYPE(unsigned char);
	break;
    case LONG:
	SSCANF_TYPE(long);
	break;
    case ULONG:
	SSCANF_TYPE(unsigned long);
	break;
    case FLOAT:
	SSCANF_TYPE(float);
	break;
    case DOUBLE:
	SSCANF_TYPE(double);
	break;
    case LDOUBLE:
	SSCANF_TYPE(long double);
	break;
    }

    if (ret != 1) {
	printf("\tWarning: no assignment done.\n");
    }

    /* return values equal? */
    if (bu_ret != ret) {
	printf("\t[FAIL] sscanf returned %d but bu_sscanf returned %d.\n",
		ret, bu_ret);
	return;
    }


#define CHECK_INT(type, conv_spec) \
    if (*(type*)val != *(type*)bu_val) { \
	printf("\t[FAIL] conversion value mismatch.\n" \
		"\t(sscanf) %" # conv_spec " != %" # conv_spec " (bu_sscanf).\n", \
		*(type*)val, *(type*)bu_val); \
    }

#define CHECK_FLOAT(type, conv_spec) \
    if (!NEAR_EQUAL(*(type*)val, *(type*)bu_val, FLOAT_TOL)) { \
	printf("\t[FAIL] conversion value mismatch.\n" \
		"\t(sscanf) %" # conv_spec " != %" # conv_spec " (bu_sscanf).\n", \
		*(type*)val, *(type*)bu_val); \
    }

    /* conversion values equal? */
    if (val != NULL && bu_val != NULL) {

	switch (type) {
	case INT:
	    CHECK_INT(int, d);
	    break;
	case UINT:
	    CHECK_INT(unsigned, u);
	    break;
	case SHORT:
	    CHECK_INT(short, hd);
	    break;
	case USHORT:
	    CHECK_INT(unsigned short, hu);
	    break;
	case SHORTSHORT:
	    CHECK_INT(char, hhd);
	    break;
	case USHORTSHORT:
	    CHECK_INT(unsigned char, hhu);
	    break;
	case LONG:
	    CHECK_INT(long, ld);
	    break;
	case ULONG:
	    CHECK_INT(unsigned long, lu);
	    break;
	case FLOAT:
	    CHECK_FLOAT(float, e);
	    break;
	case DOUBLE:
	    CHECK_FLOAT(double, le);
	    break;
	case LDOUBLE:
	    CHECK_FLOAT(long double, Le);
	break;
	}
	bu_free(val, "test_sscanf val");
	val = NULL;
	bu_free(bu_val, "test_sscanf bu_val");
	bu_val = NULL;
    }
} /* test_sscanf */

/* string test routine */
static void
test_sscanf_s(const char *src, const char *fmt)
{
    int ret, bu_ret;
    char dest[STR_SIZE], bu_dest[STR_SIZE];

    ret = bu_ret = 0;
    printf("%s, %s\n", src, fmt);

    ret = sscanf(src, fmt, dest);
    bu_ret = bu_sscanf(src, fmt, bu_dest);

    if (ret != 1) {
	printf("\tWarning: no assignments done.\n");
    }

    /* return values equal? */
    if (bu_ret != ret) {
	printf("\t[FAIL] sscanf returned %d but bu_sscanf returned %d.\n",
		ret, bu_ret);
	return;
    }

    /* conversion values equal? */
    if (!BU_STR_EQUAL(dest, bu_dest)) {
	printf("\t[FAIL] conversion value mismatch.\n"
		"\t(sscanf) %s != %s (bu_sscanf).\n",
		dest, bu_dest);
    }
}

/* Here we define printable constants that should be safe on any platform.
 *
 * Note that we don't use the macros in limits.h, because they aren't
 * necessarily printable, as in:
 *     #define INT_MIN (-INT_MAX - 1)
 */
#define SMALL_SHORTSHORT -127 /*  2^7 + 1 (not assuming 2's complement) */
#define LARGE_SHORTSHORT +127 /*  2^7 - 1 */
#define LARGE_USHORTSHORT 255 /*  2^8 - 1 */

#define SMALL_SHORTSHORT_OCT -0177
#define LARGE_SHORTSHORT_OCT +0177
#define LARGE_USHORTSHORT_OCT 0377

#define SMALL_SHORTSHORT_HEX -0x7F
#define LARGE_SHORTSHORT_HEX +0x7F
#define LARGE_USHORTSHORT_HEX 0xFF

#define SMALL_INT -32767 /* -2^15 + 1 */
#define LARGE_INT +32767 /*  2^15 - 1 */
#define LARGE_UINT 65535 /*  2^16 - 1 */

#define SMALL_INT_OCT -077777
#define LARGE_INT_OCT +077777
#define LARGE_UINT_OCT 0177777

#define SMALL_INT_HEX -0x7FFF
#define LARGE_INT_HEX +0x7FFF
#define LARGE_UINT_HEX 0xFFFF

#define SMALL_LONG -2147483647L /* -2^31 + 1 */
#define LARGE_LONG +2147483647L /*  2^31 - 1 */
#define LARGE_ULONG 4294967295U /*  2^32 - 1 */

#define SMALL_LONG_OCT -17777777777L
#define LARGE_LONG_OCT +17777777777L
#define LARGE_ULONG_OCT 37777777777U

#define SMALL_LONG_HEX -0x7FFFFFFFL
#define LARGE_LONG_HEX +0x7FFFFFFFL
#define LARGE_ULONG_HEX 0xFFFFFFFFU

int
main(int argc, char *argv[])
{
    if (argc > 1) {
	printf("Warning: %s takes no arguments.\n", argv[0]);
    }

    printf("bu_sscanf: running tests\n");

/* Possible test cases:
 * 1) Simple conversions, signed and unsigned:
 *    %,d,i,o,u,x,X,a,e,E,f,g,s,c,[...],p,n
 * 2) Modified conversions:
 *    (h|hh|j|l|t|z)[diouxXn], (l|L)[aeEfg]
 * 3) Width specifiers:
 *    <width>[diouxXaeEfgsc[...]p]
 * 4) Non-conversions:
 *    *,%
 * 5) Whitespace skip; arbitrary combinations of whitespace characters
 *    used to separate strings and conversion specifiers. Preservation
 *    of leading space for %c and %[...] and discarding for all others.
 *
 */

    /* signed integer tests */

    /* d decimal tests */
    test_sscanf(SHORTSHORT, "0", "%hhd");
    test_sscanf(SHORTSHORT, bu_cpp_xstr(LARGE_SHORTSHORT), "%hhd");
    test_sscanf(SHORTSHORT, bu_cpp_xstr(SMALL_SHORTSHORT), "%hhd");

    test_sscanf(SHORT, "0", "%hd");
    test_sscanf(SHORT, bu_cpp_xstr(LARGE_INT), "%hd");
    test_sscanf(SHORT, bu_cpp_xstr(SMALL_INT), "%hd");

    test_sscanf(INT, "0", "%d");
    test_sscanf(INT, bu_cpp_xstr(LARGE_INT), "%d");
    test_sscanf(INT, bu_cpp_xstr(SMALL_INT), "%d");

    test_sscanf(LONG, "0", "%ld");
    test_sscanf(LONG, bu_cpp_xstr(LARGE_LONG), "%ld");
    test_sscanf(LONG, bu_cpp_xstr(SMALL_LONG), "%ld");

    /* i decimal tests */
    test_sscanf(SHORTSHORT, "0", "%hhi");
    test_sscanf(SHORTSHORT, bu_cpp_xstr(LARGE_SHORTSHORT), "%hhi");
    test_sscanf(SHORTSHORT, bu_cpp_xstr(SMALL_SHORTSHORT), "%hhi");

    test_sscanf(SHORT, "0", "%hi");
    test_sscanf(SHORT, bu_cpp_xstr(LARGE_INT), "%hi");
    test_sscanf(SHORT, bu_cpp_xstr(SMALL_INT), "%hi");

    test_sscanf(INT, "0", "%i");
    test_sscanf(INT, bu_cpp_xstr(LARGE_INT), "%i");
    test_sscanf(INT, bu_cpp_xstr(SMALL_INT), "%i");

    test_sscanf(LONG, "0", "%li");
    test_sscanf(LONG, bu_cpp_xstr(LARGE_LONG), "%li");
    test_sscanf(LONG, bu_cpp_xstr(SMALL_LONG), "%li");

    /* i octal tests */
    test_sscanf(SHORTSHORT, "0", "%hhi");
    test_sscanf(SHORTSHORT, bu_cpp_xstr(LARGE_SHORTSHORT_OCT), "%hhi");
    test_sscanf(SHORTSHORT, bu_cpp_xstr(SMALL_SHORTSHORT_OCT), "%hhi");

    test_sscanf(SHORT, "0", "%hi");
    test_sscanf(SHORT, bu_cpp_xstr(LARGE_INT_OCT), "%hi");
    test_sscanf(SHORT, bu_cpp_xstr(SMALL_INT_OCT), "%hi");

    test_sscanf(INT, "0", "%i");
    test_sscanf(INT, bu_cpp_xstr(LARGE_INT_OCT), "%i");
    test_sscanf(INT, bu_cpp_xstr(SMALL_INT_OCT), "%i");

    test_sscanf(LONG, "0", "%li");
    test_sscanf(LONG, bu_cpp_xstr(LARGE_LONG_OCT), "%li");
    test_sscanf(LONG, bu_cpp_xstr(SMALL_LONG_OCT), "%li");

    /* i hex tests */
    test_sscanf(SHORTSHORT, "0", "%hhi");
    test_sscanf(SHORTSHORT, bu_cpp_xstr(LARGE_SHORTSHORT_HEX), "%hhi");
    test_sscanf(SHORTSHORT, bu_cpp_xstr(SMALL_SHORTSHORT_HEX), "%hhi");

    test_sscanf(SHORT, "0", "%hi");
    test_sscanf(SHORT, bu_cpp_xstr(LARGE_INT_HEX), "%hi");
    test_sscanf(SHORT, bu_cpp_xstr(SMALL_INT_HEX), "%hi");

    test_sscanf(INT, "0", "%i");
    test_sscanf(INT, bu_cpp_xstr(LARGE_INT_HEX), "%i");
    test_sscanf(INT, bu_cpp_xstr(SMALL_INT_HEX), "%i");

    test_sscanf(LONG, "0", "%li");
    test_sscanf(LONG, bu_cpp_xstr(LARGE_LONG_HEX), "%li");
    test_sscanf(LONG, bu_cpp_xstr(SMALL_LONG_HEX), "%li");

    /* unsigned integer tests */

    /* decimal tests */
    test_sscanf(USHORTSHORT, "0", "%hhu");
    test_sscanf(USHORTSHORT, bu_cpp_xstr(LARGE_USHORTSHORT), "%hhu");

    test_sscanf(USHORT, "0", "%hu");
    test_sscanf(USHORT, bu_cpp_xstr(LARGE_UINT), "%hu");

    test_sscanf(UINT, "0", "%u");
    test_sscanf(UINT, bu_cpp_xstr(LARGE_UINT), "%u");

    test_sscanf(ULONG, "0U", "%lu");
    test_sscanf(ULONG, bu_cpp_xstr(LARGE_ULONG), "%lu");

    /* octal tests */
    test_sscanf(USHORTSHORT, "0", "%hho");
    test_sscanf(USHORTSHORT, bu_cpp_xstr(LARGE_USHORTSHORT_OCT), "%hho");

    test_sscanf(USHORT, "0", "%ho");
    test_sscanf(USHORT, bu_cpp_xstr(LARGE_UINT_OCT), "%ho");

    test_sscanf(UINT, "0", "%o");
    test_sscanf(UINT, bu_cpp_xstr(LARGE_UINT_OCT), "%o");

    test_sscanf(ULONG, "0U", "%lo");
    test_sscanf(ULONG, bu_cpp_xstr(LARGE_ULONG_OCT), "%lo");

    /* hex tests */
    test_sscanf(USHORTSHORT, "0", "%hhx");
    test_sscanf(USHORTSHORT, bu_cpp_xstr(LARGE_USHORTSHORT_HEX), "%hhx");

    test_sscanf(USHORT, "0", "%hx");
    test_sscanf(USHORT, bu_cpp_xstr(LARGE_UINT_HEX), "%hx");

    test_sscanf(UINT, "0", "%x");
    test_sscanf(UINT, bu_cpp_xstr(LARGE_UINT_HEX), "%x");

    test_sscanf(ULONG, "0U", "%lx");
    test_sscanf(ULONG, bu_cpp_xstr(LARGE_ULONG_HEX), "%lx");

    /* float tests */
    test_sscanf(FLOAT, "0.0F", "%f");
    test_sscanf(FLOAT, bu_cpp_xstr(FLT_MAX), "%f");
    test_sscanf(FLOAT, bu_cpp_xstr(FLT_MIN), "%f");

    test_sscanf(DOUBLE, "0.0", "%lf");
    test_sscanf(DOUBLE, bu_cpp_xstr(DBL_MAX), "%lf");
    test_sscanf(DOUBLE, bu_cpp_xstr(DBL_MIN), "%lf");

    test_sscanf(LDOUBLE, "0.0L", "%Lf");
    test_sscanf(LDOUBLE, bu_cpp_xstr(LDBL_MAX), "%Lf");
    test_sscanf(LDOUBLE, bu_cpp_xstr(LDBL_MIN), "%Lf");

    /* string tests */
    test_sscanf_s(" aBc \t", "%s");

    printf("bu_sscanf: testing complete\n");
    return 0;
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
