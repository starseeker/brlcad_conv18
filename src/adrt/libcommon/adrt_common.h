/*                     A D R T _ C O M M O N . H
 * BRL-CAD
 *
 * Copyright (C) 2002-2005 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file adrt_common.h
 *                     A D R T _ C O M M O N . H
 *
 *  Common Library - Main Include
 *
 *  Author -
 *      Justin L. Shumaker
 *
 *  Source -
 *      The U. S. Army Research Laboratory
 *      Aberdeen Proving Ground, Maryland  21005-5068  USA
 *
 * $Id$
 */

#ifndef _COMMON_H
#define _COMMON_H


#include "tie.h"
#include "texture.h"
#include "render.h"

#define COMMON_BIT_DEPTH_24	0
#define	COMMON_BIT_DEPTH_128	1

/* Work */
typedef struct common_work_s {
  short orig_x;
  short orig_y;
  short size_x;
  short size_y;
  short format;
} common_work_t;


/* Properties */
typedef struct common_prop_s {
  TIE_3 color; /* base color of the material */
  tfloat density; /* density of the material, x-ray/vulnerability stuff */
  tfloat gloss; /* smoothness of the surface, ability to reflect */
  tfloat emission; /* emission, power of light source */
  tfloat ior; /* index of refraction */
} common_prop_t;


/* Triangle */
struct common_mesh_s;
typedef struct common_triangle_s {
  struct common_mesh_s *mesh;
  tfloat *normals;
} common_triangle_t;


/* Mesh */
typedef struct common_mesh_s {
  int flags;
  char name[256];
  TIE_3 min, max;
  tfloat matrix[16];
  tfloat matinv[16];
  common_prop_t *prop;
  struct texture_s *texture;
  int tri_num;
  common_triangle_t *tri_list;
} common_mesh_t;


typedef struct common_anim_transform_s {
  char mesh_name[256];
  tfloat matrix[16];
} common_anim_transform_t;


typedef struct common_anim_frame_s {
  TIE_3 pos;
  TIE_3 focus;
  tfloat tilt;
  tfloat fov;
  tfloat dof;
  int tnum;
  common_anim_transform_t *tlist;
} common_anim_frame_t;


typedef struct common_anim_s {
  int frame_num;
  common_anim_frame_t *frame_list;
} common_anim_t;


typedef struct common_env_s {
  render_t render;
  int rm; /* render method */

  int img_vw;	/* virtual width */
  int img_vh;	/* virtual height */
  int img_w;	/* actual width */
  int img_h;	/* actual height */
  int img_hs;	/* hypersamples */
  int tile_w;	/* tile size width */
  int tile_h;	/* tile size height */
} common_env_t;


/* Database */
typedef struct common_db_s {
  int mesh_num;
  common_mesh_t **mesh_list;

  common_env_t env;
  common_anim_t anim;
} common_db_t;


#endif
