/*                    B O O L E A N I Z E . C
 * BRL-CAD
 *
 * Copyright (c) 2010-2011 United States Government as represented by
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
#include <limits.h>
#include <string.h>
#include <errno.h>

#include "bu.h"


int
bu_booleanize(const char *str)
{
    size_t len;
    struct bu_vls vls;
    const char *newstr;

bu_log("BOOLEANIZING %s\n", str);
    /* no string */
    if (!str)
	return 0;

    bu_vls_init(&vls);
    bu_vls_strcpy(&vls, str);
    bu_vls_trimspace(&vls);
    newstr = bu_vls_addr(&vls);

    /* empty string */
    len = strlen(newstr);
    if (len == 0) {
	bu_vls_free(&vls);
	return 0;
    }

    /* starts with 'n', [nN]* looks like 'no' */
    if (newstr[0] == 'n' || newstr[0] == 'N') {
	bu_vls_free(&vls);
	return 0;
    }

    /* exactly "0" */
    if (BU_STR_EQUAL(newstr, "0")) {
	bu_vls_free(&vls);
	return 0;
    }

    /* done with our string */
    bu_vls_free(&vls);

bu_log("RETURNING TRUE!\n");

    /* anything else */
    return 1;
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
