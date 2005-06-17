#include "texture_clouds.h"
#include <stdlib.h>
#include "umath.h"


void texture_clouds_init(texture_t *texture, tfloat size, int octaves, int absolute, TIE_3 scale, TIE_3 translate);
void texture_clouds_free(texture_t *texture);
void texture_clouds_work(texture_t *texture, common_mesh_t *mesh, tie_ray_t *ray, tie_id_t *id, TIE_3 *pixel);


void texture_clouds_init(texture_t *texture, tfloat size, int octaves, int absolute, TIE_3 scale, TIE_3 translate) {
  texture_clouds_t *td;

  texture->data = malloc(sizeof(texture_clouds_t));
  texture->free = texture_clouds_free;
  texture->work = texture_clouds_work;

  td = (texture_clouds_t*)texture->data;
  td->size = size;
  td->octaves = octaves;
  td->absolute = absolute;
  td->scale = scale;
  td->translate = translate;

  texture_perlin_init(&td->perlin);
}


void texture_clouds_free(texture_t *texture) {
  texture_clouds_t *td;

  td = (texture_clouds_t*)texture->data;
  texture_perlin_free(&td->perlin);
  free(texture->data);
}


void texture_clouds_work(texture_t *texture, common_mesh_t *mesh, tie_ray_t *ray, tie_id_t *id, TIE_3 *pixel) {
  texture_clouds_t *td;
  TIE_3 p, pt;


  td = (texture_clouds_t*)texture->data;

  /* Transform the Point */
  if(td->absolute) {
    p.v[0] = id->pos.v[0] * td->scale.v[0] + td->translate.v[0];
    p.v[1] = id->pos.v[1] * td->scale.v[1] + td->translate.v[1];
    p.v[2] = id->pos.v[2] * td->scale.v[2] + td->translate.v[2];
  } else {
    math_vec_transform(pt, id->pos, mesh->matinv);
    p.v[0] = (mesh->max.v[0] - mesh->min.v[0] > TIE_PREC ? (pt.v[0] - mesh->min.v[0]) / (mesh->max.v[0] - mesh->min.v[0]) : 0.0) * td->scale.v[0] + td->translate.v[0];
    p.v[1] = (mesh->max.v[1] - mesh->min.v[1] > TIE_PREC ? (pt.v[1] - mesh->min.v[1]) / (mesh->max.v[1] - mesh->min.v[1]) : 0.0) * td->scale.v[1] + td->translate.v[1];
    p.v[2] = (mesh->max.v[2] - mesh->min.v[2] > TIE_PREC ? (pt.v[2] - mesh->min.v[2]) / (mesh->max.v[2] - mesh->min.v[2]) : 0.0) * td->scale.v[2] + td->translate.v[2];
  }

  pixel->v[0] = fabs(0.5*texture_perlin_noise3(&td->perlin, p, td->size*1.0, td->octaves) + 0.5);
  pixel->v[1] = fabs(0.5*texture_perlin_noise3(&td->perlin, p, td->size*1.0, td->octaves) + 0.5);
  pixel->v[2] = fabs(0.5*texture_perlin_noise3(&td->perlin, p, td->size*1.0, td->octaves) + 0.5);
}
