/*                    T E S T _ V L S _ V P R I N T F. C
 * BRL-CAD
 *
 * Copyright (c) 2011-2012 United States Government as represented by
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

#include "common.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

#include "bu.h"

#include "./vls_internals.h"

/* Test against sprintf */
int
test_vls(const char *fmt, ...)
{
    int status        = 0; /* okay */
    struct bu_vls vls = BU_VLS_INIT_ZERO;
    char output[80]   = {0};
    char buffer[1024] = {0};
    va_list ap;

    va_start(ap, fmt);
    /* use the libc version */
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    va_start(ap, fmt);
    /* use BRL-CAD bu_vls version for comparison */
    bu_vls_vprintf(&vls, fmt, ap);
    va_end(ap);

    snprintf(output, sizeof(output), "%-24s -> '%s'", fmt, bu_vls_addr(&vls));
    if (BU_STR_EQUAL(buffer, bu_vls_addr(&vls))
	&& strlen(buffer) == bu_vls_strlen(&vls)) {
	printf("%-*s[PASS]\n", 60, output);
    } else {
	printf("%-*s[FAIL]  (should be: '%s')\n", 60, output, buffer);
	status = 1;
    }

    bu_vls_free(&vls);

    return status;
}

int
check_format_chars()
{
  int status = 0; /* assume okay */
  int i, flags;
  vflags_t f;

  for (i = 0; i < 255; ++i) {
    unsigned char c = (unsigned char)i;
    if (!isprint(c))
      continue;
    flags = format_part_status(c);
    if (flags & VP_VALID) {
      /* we need a valid part handler */
      int vp_part = flags & VP_PARTS;

      /* for the moment we only have one such handler */
      if (vp_part ^ VP_LENGTH_MOD) /* same as !(vp_part & VP_LENGTH_MOD) */
        continue;

      if (!handle_format_part(vp_part, &f, c, VP_NOPRINT)) {
        /* tell user */
        printf("Unhandled valid char '%c'                                    [FAIL]\n", c);
        status = 1;
      }
    } else if (flags & VP_OBSOLETE) {
      /* we need an obsolete part handler */
      if (!handle_obsolete_format_char(c, VP_NOPRINT)) {
        /* tell user */
        printf("Unhandled obsolete char '%c'                                 [FAIL]\n", c);
        status = 1;
      }
    }
  }

  return status;
}

int
main(int ac, char *av[])
{
    int fails    = 0; /* track unexpected failures */
    int expfails = 0; /* track expected failures */

    int f = 0;
    int p = 0;
    const char *word = "Lawyer";

    printf("Testing bu_vls_vprintf...\n");

    /* ======================================================== */
    /* TESTS EXPECTED TO PASS
     *
     *   (see expected failures section below)
     */
    /* ======================================================== */

    /* first check that we handle all known format chars */
    printf("\n");
    printf("Testing format char handlers...\n\n");
    fails += check_format_chars();

    printf("\n");
    printf("Testing format conversions ...\n\n");

    /* various types */
    printf("An empty string (\"\"):\n");
    fails += test_vls("");

    printf("A newline (\"\\n\"):\n");
    fails += test_vls("\n");

    fails += test_vls("hello");
    fails += test_vls("%s", "hello");
    fails += test_vls("%d", 123);
    fails += test_vls("%u", -123);
    fails += test_vls("%e %E", 1.23, -3.21);
    fails += test_vls("%g %G", 1.23, -3.21);
    fails += test_vls("%x %X", 1.23, -3.21);
    fails += test_vls("%x %X", 123, -321);
    fails += test_vls("%o", 1.23);
    fails += test_vls("%c%c%c", '1', '2', '3');
    fails += test_vls("%p", (void *)av);
    fails += test_vls("%%%d%%", ac);

    /* various lengths */
    fails += test_vls("%zu %zd", (size_t)123, (ssize_t)-123);
    fails += test_vls("%jd %td", (intmax_t)123, (ptrdiff_t)-123);

    /* various widths */
    fails += test_vls("he%*so", 2, "ll");
    fails += test_vls("he%*so", 2, "llll");
    fails += test_vls("he%*so", 4, "ll");

    /* various precisions */
    fails += test_vls("he%.*so", 2, "ll");
    fails += test_vls("he%.-1-o", 123);
    fails += test_vls("%6.-3f", 123);

    /* various flags */
    fails += test_vls("%010d", 123);
    fails += test_vls("%#-.10lx", 123);
    fails += test_vls("%#lf", 123.0);

    /* two-character length modifiers */
    fails += test_vls("he%10dllo", 123);
    fails += test_vls("he%-10ullo", 123);
    fails += test_vls("he%#-12.10tullo", (ptrdiff_t)0x1234);
    fails += test_vls("he%+-6.3ld%-+3.6dllo", 123, 321);
    fails += test_vls("he%.10dllo", 123);
    fails += test_vls("he%.-10ullo", 123);
    fails += test_vls("%hd %hhd", 123, -123);

    /* combinations, e.g., bug ID 3475562, fixed at rev 48958 */
    /* left justify, right justify, in wider fields than the strings */
    f = p = 2;
    fails += test_vls("|%-*.*s|%*.*s|", f, p, "t", f, p, "t");
    fails += test_vls("|%*s|%-*s|", f, "test", f, "test");
    fails += test_vls("|%*s|%-*s|", f, word, f, word);

    /* min field width; max string length ('precision'); string */
    f = 2; p = 4;
    printf("fw=%d, prec=%d, '%s': '%%%d.%ds'\n", f, p, word, f, p);
    fails += test_vls("%*.*s", f, p, word);

    f = 4; p = 2;
    printf("fw=%d, prec=%d, '%s': '%%%d.%ds'\n", f, p, word, f, p);
    fails += test_vls("%*.*s", f, p, word);

    f = 4; p = 8;
    printf("fw=%d, prec=%d, '%s': '%%%d.%ds'\n", f, p, word, f, p);
    fails += test_vls("%*.*s", f, p, word);

    f = 0; p = 8;
    printf("fw=%d, prec=%d, '%s': '%%%d.%ds'\n", f, p, word, f, p);
    fails += test_vls("%*.*s", f, p, word);

    f = 8; p = 0;
    printf("fw=%d, prec=%d, '%s': '%%%d.%ds'\n", f, p, word, f, p);
    fails += test_vls("%*.*s", f, p, word);

    /* mged bug at rev 48989 */
    f = 8; p = 0;
    printf("fw=%d, '%s': '%%%ds'\n", f, word, f);
    fails += test_vls("%*s", f, word);

    /* same but left justify */

    f = 2; p = 4;
    printf("fw=%d, prec=%d, '%s': '%%-%d.%ds'\n", f, p, word, f, p);
    fails += test_vls("%-*.*s", f, p, word);

    f = 4; p = 2;
    printf("fw=%d, prec=%d, '%s': '%%-%d.%ds'\n", f, p, word, f, p);
    fails += test_vls("%-*.*s", f, p, word);

    f = 4; p = 8;
    printf("fw=%d, prec=%d, '%s': '%%-%d.%ds'\n", f, p, word, f, p);
    fails += test_vls("%-*.*s", f, p, word);

    f = 0; p = 8;
    printf("fw=%d, prec=%d, '%s': '%%-%d.%ds'\n", f, p, word, f, p);
    fails += test_vls("%-*.*s", f, p, word);

    f = 8; p = 0;
    printf("fw=%d, prec=%d, '%s': '%%-%d.%ds'\n", f, p, word, f, p);
    fails += test_vls("%-*.*s", f, p, word);

    /* from "various types" */
    fails += test_vls("%f %F", 1.23, -3.21);

    /* from "two-character length modifiers" */
    fails += test_vls("%ld %lld", 123, -123LL);

    /* unsigned variant */
    fails += test_vls("%lu %llu", 123, 123ULL);

    /* from "two-character length modifiers" */
    fails += test_vls("%ld %lld", 123, -123);

    /* unsigned variant */
    fails += test_vls("%lu %llu", 123, 123);

    fails += test_vls("%hd %hhd", 123, -123);

    /* misc */
    fails += test_vls("% d % d", 123, -123);

    fails += test_vls("% 05d % d", 123, -123);

    fails += test_vls("%'d", 123000);

    fails += test_vls("%c", 'r');

    fails += test_vls("%H", 123);

    /* obsolete but usable */
/*
    fails += test_vls("%S", (wchar_t *)"hello");
*/
/*
    fails += test_vls("%qd %qd", 123, -123);
*/

    /* other */

    /* ======================================================== */
    /* EXPECTED FAILURES ONLY BELOW HERE                        */
    /* ======================================================== */
    /* EXPECTED FAILURES:
     *
     * Notes:
     *
     *   1. For these tests have the return value increment 'expfails'.
     *   2. Test with both 'make vsl-regress' and 'make regress' because
     *        some other tests use this function in unpredictable ways.
     *   3. After a test is fixed, change the return value to increment
     *        'fails', move it to the EXPECTED PASS group above, and add
     *        some info about it as necessary to help those who may be
     *        forced to revisit this.
     *
     */

/* uncomment if using expected failures */
/* #define EXP_FAILS */

    printf("\nExpected failures (don't use in production code):\n");

#if defined (EXP_FAILS)
    /* obsolete - expected failures */
    expfails += test_vls("%C", 'N');
    expfails += test_vls("%D %D", 123, -123);
    expfails += test_vls("%O %O", 123, -123);
    expfails += test_vls("%U %U", 123, -123);
#else
    printf("  NONE AT THIS TIME\n");
#endif

    /* report results */
    fprintf(stderr, "%d", expfails);


    printf("\n%s: testing complete\n", av[0]);

    if (fails != 0) {
      /* as long as fails is < 127 the STATUS will be the number of unexpected failures */
	return fails;
    }

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
