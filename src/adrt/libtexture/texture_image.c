/*                     T E X T U R E _ I M A G E . C
 * BRL-CAD / ADRT
 *
 * Copyright (c) 2002-2007 United States Government as represented by
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
/**
 * @file texture_image.c
 *
 *  Comments -
 *      Texture Library - Projects an Image onto a Surface
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

#ifdef HAVE_CONFIG_H
# include "brlcad_config.h"
#endif

#include "texture_image.h"
#include <stdlib.h>

#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include "umath.h"


void texture_image_init(texture_t *texture, short w, short h, unsigned char *image);
void texture_image_free(texture_t *texture);
void texture_image_work(texture_t *texture, common_mesh_t *mesh, tie_ray_t *ray, tie_id_t *id, TIE_3 *pixel);


void texture_image_init(texture_t *texture, short w, short h, unsigned char *image) {
  texture_image_t *td;

  texture->data = malloc(sizeof(texture_image_t));
  if (!texture->data) {
      perror("texture->data");
      exit(1);
  }
  texture->free = texture_image_free;
  texture->work = (texture_work_t *)texture_image_work;

  td = (texture_image_t *)texture->data;
  td->w = w;
  td->h = h;
  td->image = (unsigned char *)malloc(3*w*h);
  if (!td->image) {
      perror("td->image");
      exit(1);
  }
  memcpy(td->image, image, 3*w*h);
}


void texture_image_free(texture_t *texture) {
  texture_image_t *td;

  td = (texture_image_t *)texture->data;
  free(td->image);
  free(texture->data);
}


void texture_image_work(texture_t *texture, common_mesh_t *mesh, tie_ray_t *ray, tie_id_t *id, TIE_3 *pixel) {
  texture_image_t *td;
  TIE_3 pt;
  TFLOAT u, v;
  int ind;


  td = (texture_image_t *)texture->data;


  /* Transform the Point */
  MATH_VEC_TRANSFORM(pt, id->pos, mesh->matinv);
  u = mesh->max.v[0] - mesh->min.v[0] > TIE_PREC ? (pt.v[0] - mesh->min.v[0]) / (mesh->max.v[0] - mesh->min.v[0]) : 0.0;
  v = mesh->max.v[1] - mesh->min.v[1] > TIE_PREC ? (pt.v[1] - mesh->min.v[1]) / (mesh->max.v[1] - mesh->min.v[1]) : 0.0;

  ind = 3*((int)((1.0 - v)*td->h)*td->w + (int)(u*td->w));

  pixel->v[0] = td->image[ind+2] / 255.0;
  pixel->v[1] = td->image[ind+1] / 255.0;
  pixel->v[2] = td->image[ind+0] / 255.0;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
