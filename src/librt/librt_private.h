/*                 L I B R T _ P R I V A T E . H
 * BRL-CAD
 *
 * Copyright (c) 2011 United States Government as represented by
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
/** @file librt_private.h
 *
 * These are declarations for functions that are for internal use by
 * LIBRT but are not public API.  NO CODE outside of LIBRT should use
 * these functions.
 *
 * Non-public functions should NOT have an rt_*() or db_*() prefix.
 * Consider using an _rt_, _db_, or other file-identifying prefix
 * accordingly (e.g., ell_*() for functions defined in primitives/ell)
 *
 */

/**
 * db_flip.c
 * function similar to ntohs() but always flips the bytes
 * used for v4 compatibility.
 */
extern short flip_short(const short s);



/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
