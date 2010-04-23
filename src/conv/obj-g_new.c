/*                     O B J - G _ N E W . C
 * BRL-CAD
 *
 * Copyright (c) 2010 United States Government as represented by
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
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "bu.h"
#include "vmath.h"
#include "nmg.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "wdb.h"
#include "obj_parser.h"

/* grouping type */
#define GRP_NONE     0 /* perform no grouping */
#define GRP_GROUP    1 /* create bot for each obj file 'g' grouping */
#define GRP_OBJECT   2 /* create bot for each obj file 'o' grouping */
#define GRP_MATERIAL 3 /* create bot for each obj file 'usemtl' grouping */
#define GRP_TEXTURE  4 /* create bot for eacg obj file 'usemap' grouping */

/* face type */
#define FACE_V    1 /* polygonal faces only identified by vertices */
#define FACE_TV   2 /* textured polygonal faces */
#define FACE_NV   3 /* oriented polygonal faces */
#define FACE_TNV  4 /* textured oriented polygonal faces */

typedef const size_t (**arr_1D_t);    /* 1 dimensional array type */
typedef const size_t (**arr_2D_t)[2]; /* 2 dimensional array type */
typedef const size_t (**arr_3D_t)[3]; /* 3 dimensional array type */

/* grouping face indices type */
struct gfi_t { 
    void   *index_arr_faces;           /* face indices into vertex, normal ,texture vertex lists */
    size_t *num_vertices_arr;          /* number of vertices for each face within index_arr_faces */
    size_t *obj_file_face_idx_arr;     /* corresponds to the index of the face within the obj file. this
                                          value is useful to trace face errors back to the obj file. */
    struct  bu_vls *raw_grouping_name; /* raw name of grouping from obj file; group/object/material/texture */
    size_t  num_faces;                 /* number of faces represented by index_arr_faces and num_vertices_arr */
    size_t  max_faces;                 /* maximum number of faces based on current memory allocation */
    size_t  tot_vertices;              /* sum of contents of num_vertices_arr. note: if the face_type
                                          includes normals and/or texture vertices, each vertex must have
                                          an associated normal and/or texture vertice. therefore this
                                          total is also the total of the associated normals and/or texture
                                          vertices. */
    int     face_type;                 /* i.e. FACE_V, FACE_TV, FACE_NV or FACE_TNV */
    int     grouping_type;             /* i.e. GRP_NONE, GRP_GROUP, GRP_OBJECT, GRP_MATERIAL or GRP_TEXTURE */
    size_t  grouping_index;            /* corresponds to the index of the grouping name within the obj file.  
                                          this value is useful to append to the raw_grouping_name to
                                          ensure the resulting name is unique. */
};

/* obj file global attributes type */
struct ga_t {                                    /* assigned by ... */
    const obj_polygonal_attributes_t *polyattr_list; /* obj_polygonal_attributes */
    obj_parser_t         parser;                 /* obj_parser_create */
    obj_contents_t       contents;               /* obj_fparse */
    size_t               numPolyAttr;            /* obj_polygonal_attributes */
    size_t               numGroups;              /* obj_groups */
    size_t               numObjects;             /* obj_objects */
    size_t               numMaterials;           /* obj_materials */
    size_t               numTexmaps;             /* obj_texmaps */
    size_t               numVerts;               /* obj_vertices */
    size_t               numNorms;               /* obj_normals */
    size_t               numTexCoords;           /* obj_texture_coord */
    size_t               numNorFaces;            /* obj_polygonal_nv_faces */
    size_t               numFaces;               /* obj_polygonal_v_faces */
    size_t               numTexFaces;            /* obj_polygonal_tv_faces */ 
    size_t               numTexNorFaces;         /* obj_polygonal_tnv_faces */ 
    const char * const  *str_arr_obj_groups;     /* obj_groups */
    const char * const  *str_arr_obj_objects;    /* obj_objects */
    const char * const  *str_arr_obj_materials;  /* obj_materials */
    const char * const  *str_arr_obj_texmaps;    /* obj_texmaps */
    const float        (*vert_list)[4];          /* obj_vertices */
    const float        (*norm_list)[3];          /* obj_normals */
    const float        (*texture_coord_list)[3]; /* obj_texture_coord */
    const size_t        *attindex_arr_nv_faces;  /* obj_polygonal_nv_faces */
    const size_t        *attindex_arr_v_faces;   /* obj_polygonal_v_faces */
    const size_t        *attindex_arr_tv_faces;  /* obj_polygonal_tv_faces */
    const size_t        *attindex_arr_tnv_faces; /* obj_polygonal_tnv_faces */
};

    /* global definition */
    size_t *tmp_ptr = NULL ;
    size_t (*triangle_indexes)[3][2] = NULL ;

void collect_global_obj_file_attributes(struct ga_t *ga) {
    size_t i = 0;

    bu_log("running obj_polygonal_attributes\n");
    ga->numPolyAttr = obj_polygonal_attributes(ga->contents, &ga->polyattr_list);

    bu_log("running obj_groups\n");
    ga->numGroups = obj_groups(ga->contents, &ga->str_arr_obj_groups);
    bu_log("total number of groups in OBJ file; numGroups = (%lu)\n", ga->numGroups);

    bu_log("list of all groups i.e. 'g' in OBJ file\n");
    for (i = 0 ; i < ga->numGroups ; i++) {
        bu_log("(%lu)(%s)\n", i, ga->str_arr_obj_groups[i]);
    }

    bu_log("running obj_objects\n");
    ga->numObjects = obj_objects(ga->contents, &ga->str_arr_obj_objects);
    bu_log("total number of object groups in OBJ file; numObjects = (%lu)\n", ga->numObjects);

    bu_log("list of all object groups i.e. 'o' in OBJ file\n");
    for (i = 0 ; i < ga->numObjects ; i++) {
        bu_log("(%lu)(%s)\n", i, ga->str_arr_obj_objects[i]);
    }

    bu_log("running obj_materials\n");
    ga->numMaterials = obj_materials(ga->contents, &ga->str_arr_obj_materials);
    bu_log("total number of material names in OBJ file; numMaterials = (%lu)\n", ga->numMaterials);

    bu_log("list of all material names i.e. 'usemtl' in OBJ file\n");
    for (i = 0 ; i < ga->numMaterials ; i++) {
        bu_log("(%lu)(%s)\n", i, ga->str_arr_obj_materials[i]);
    }

    bu_log("running obj_texmaps\n");
    ga->numTexmaps = obj_texmaps(ga->contents, &ga->str_arr_obj_texmaps);
    bu_log("total number of texture map names in OBJ file; numTexmaps = (%lu)\n", ga->numTexmaps);

    bu_log("list of all texture map names i.e. 'usemap' in OBJ file\n");
    for (i = 0 ; i < ga->numTexmaps ; i++) {
        bu_log("(%lu)(%s)\n", i, ga->str_arr_obj_texmaps[i]);
    }

    bu_log("running obj_vertices\n");
    ga->numVerts = obj_vertices(ga->contents, &ga->vert_list);
    bu_log("numVerts = (%lu)\n", ga->numVerts);

    bu_log("running obj_normals\n");
    ga->numNorms = obj_normals(ga->contents, &ga->norm_list);
    bu_log("numNorms = (%lu)\n", ga->numNorms);

    bu_log("running obj_texture_coord\n");
    ga->numTexCoords = obj_texture_coord(ga->contents, &ga->texture_coord_list);
    bu_log("numTexCoords = (%lu)\n", ga->numTexCoords);

    bu_log("running obj_polygonal_nv_faces\n");
    ga->numNorFaces = obj_polygonal_nv_faces(ga->contents, &ga->attindex_arr_nv_faces);
    bu_log("number of oriented polygonal faces; numNorFaces = (%lu)\n", ga->numNorFaces);

    bu_log("running obj_polygonal_v_faces\n");
    ga->numFaces = obj_polygonal_v_faces(ga->contents, &ga->attindex_arr_v_faces);
    bu_log("number of polygonal faces only identifed by vertices; numFaces = (%lu)\n", ga->numFaces);

    bu_log("running obj_polygonal_tv_faces\n");
    ga->numTexFaces = obj_polygonal_tv_faces(ga->contents, &ga->attindex_arr_tv_faces);
    bu_log("number of textured polygonal faces; numTexFaces = (%lu)\n", ga->numTexFaces);

    bu_log("running obj_polygonal_tnv_faces\n");
    ga->numTexNorFaces = obj_polygonal_tnv_faces(ga->contents, &ga->attindex_arr_tnv_faces);
    bu_log("number of oriented textured polygonal faces; numTexNorFaces = (%lu)\n", ga->numTexNorFaces);

    return;
}

void cleanup_name(struct bu_vls *outputObjectName_ptr) {
    char *temp_str;
    int outputObjectName_length;
    int i;

    temp_str = bu_vls_addr(outputObjectName_ptr);

    /* length does not include null */
    outputObjectName_length = bu_vls_strlen(outputObjectName_ptr);

    for ( i = 0 ; i < outputObjectName_length ; i++ ) {
        if (temp_str[i] == '/') {
            temp_str[i] = '_';
        }
    }

    return;
}

/* compare function for bsearch for triangle indexes */
static int comp_b(const void *p1, const void *p2) {
   size_t i = * (size_t *) p1;
   size_t j = * (size_t *) p2;

   return (int)(i - j);
}

/* compare function for qsort for triangle indexes */
static int comp(const void *p1, const void *p2) {
   size_t i = * (size_t *) p1;
   size_t j = * (size_t *) p2;

   return (int)(tmp_ptr[i] - tmp_ptr[j]);
}

/* retrieves the coordinates and obj file indexes for the 
   vertex, normal and texture vertex for the current grouping
   and indicated by the face index number of grouping and
   vertex index number of face into the grouping array.
   note: if the face_type indicates a type which some of 
   this information is not applicable, the not applicable
   information will not be set. */             /* inputs: */
void retrieve_coord_index(struct   ga_t *ga,   /* obj file global attributes */
                          struct   gfi_t *gfi, /* grouping face indices */
                          size_t   fi,         /* face index number of grouping */
                          size_t   vi,         /* vertex index number of face */
                                               /* outputs: */
                          fastf_t *vc,         /* vertex coordinates */
                          fastf_t *nc,         /* normal coordinates */
                          fastf_t *tc,         /* texture vertex coordinates */
                          fastf_t *w,          /* vertex weight */
                          size_t  *vofi,       /* vertex obj file index */
                          size_t  *nofi,       /* normal obj file index */
                          size_t  *tofi) {     /* texture vertex obj file index */

    const size_t (*index_arr_v_faces) = NULL ;      /* used by v_faces */
    const size_t (*index_arr_tv_faces)[2] = NULL ;  /* used by tv_faces */
    const size_t (*index_arr_nv_faces)[2] = NULL ;  /* used by nv_faces */
    const size_t (*index_arr_tnv_faces)[3] = NULL ; /* used by tnv_faces */

    arr_1D_t index_arr_faces_1D = NULL; 
    arr_2D_t index_arr_faces_2D = NULL; 
    arr_3D_t index_arr_faces_3D = NULL; 

    size_t  fofi = gfi->obj_file_face_idx_arr[fi]; /* face obj file index */

    switch (gfi->face_type) {
        case FACE_V :
            index_arr_faces_1D = (arr_1D_t)(gfi->index_arr_faces);
            /* copy current vertex coordinates into vc */
            VMOVE(vc, ga->vert_list[index_arr_faces_1D[fi][vi]]);
            /* copy current vertex weight into w */
            *w = ga->vert_list[index_arr_faces_1D[fi][vi]][3] ;
            /* copy current vertex obj file index into vofi */ 
            *vofi = index_arr_faces_1D[fi][vi] ;
            bu_log("fi=(%lu)vi=(%lu)fofi=(%lu)vofi=(%lu)v=(%f)(%f)(%f)w=(%f)\n", 
               fi, vi, fofi+1, *vofi+1, vc[0], vc[1], vc[2], *w);
            break;
        case FACE_TV :
            index_arr_faces_2D = (arr_2D_t)(gfi->index_arr_faces);
            /* copy current vertex coordinates into vc */
            VMOVE(vc, ga->vert_list[index_arr_faces_2D[fi][vi][0]]);
            /* copy current vertex weight into w */
            *w = ga->vert_list[index_arr_faces_2D[fi][vi][0]][3] ;
            /* copy current texture coordinate into tc */
            VMOVE(tc, ga->texture_coord_list[index_arr_faces_2D[fi][vi][1]]);
            /* copy current vertex obj file index into vofi */ 
            *vofi = index_arr_faces_2D[fi][vi][0];
            /* copy current texture coordinate obj file index into tofi */ 
            *tofi = index_arr_faces_2D[fi][vi][1];
            bu_log("fi=(%lu)vi=(%lu)fofi=(%lu)vofi=(%lu)tofi=(%lu)v=(%f)(%f)(%f)w=(%f)t=(%f)(%f)(%f)\n", 
               fi, vi, fofi+1, *vofi+1, *tofi+1, vc[0], vc[1], vc[2], *w, tc[0], tc[1], tc[2]);
            break;
        case FACE_NV :
            index_arr_faces_2D = (arr_2D_t)(gfi->index_arr_faces);
            /* copy current vertex coordinates into vc */
            VMOVE(vc, ga->vert_list[index_arr_faces_2D[fi][vi][0]]);
            /* copy current vertex weight into w */
            *w = ga->vert_list[index_arr_faces_2D[fi][vi][0]][3] ;
            /* copy current normal into nc */
            VMOVE(nc, ga->norm_list[index_arr_faces_2D[fi][vi][1]]);
            /* copy current vertex obj file index into vofi */ 
            *vofi = index_arr_faces_2D[fi][vi][0];
            /* copy current normal obj file index into nofi */ 
            *nofi = index_arr_faces_2D[fi][vi][1];
            bu_log("fi=(%lu)vi=(%lu)fofi=(%lu)vofi=(%lu)nofi=(%lu)v=(%f)(%f)(%f)w=(%f)n=(%f)(%f)(%f)\n", 
               fi, vi, fofi+1, *vofi+1, *nofi+1, vc[0], vc[1], vc[2], *w, nc[0], nc[1], nc[2]);
            break;
        case FACE_TNV :
            index_arr_faces_3D = (arr_3D_t)(gfi->index_arr_faces);
            /* copy current vertex coordinates into vc */
            VMOVE(vc, ga->vert_list[index_arr_faces_3D[fi][vi][0]]);
            /* copy current vertex weight into w */
            *w = ga->vert_list[index_arr_faces_3D[fi][vi][0]][3] ;
            /* copy current texture coordinate into tc */
            VMOVE(tc, ga->texture_coord_list[index_arr_faces_3D[fi][vi][1]]);
            /* copy current normal into nc */
            VMOVE(nc, ga->norm_list[index_arr_faces_3D[fi][vi][2]]);
            /* copy current vertex obj file index into vofi */ 
            *vofi = index_arr_faces_3D[fi][vi][0];
            /* copy current texture coordinate obj file index into tofi */ 
            *tofi = index_arr_faces_3D[fi][vi][1];
            /* copy current normal obj file index into nofi */ 
            *nofi = index_arr_faces_3D[fi][vi][2];
            bu_log("fi=(%lu)vi=(%lu)fofi=(%lu)vofi=(%lu)tofi=(%lu)nofi=(%lu)v=(%f)(%f)(%f)w=(%f)t=(%f)(%f)(%f)n=(%f)(%f)(%f)\n", 
               fi, vi, fofi+1, *vofi+1, *tofi+1, *nofi+1, vc[0], vc[1], vc[2], *w, 
               tc[0], tc[1], tc[2], nc[0], nc[1], nc[2]);
            break;
        default:
            bu_log("ERROR: logic error, invalid face_type in function 'retrieve_coord_index'\n");
            return;
    }
    return;
}

void test_face(struct ga_t *ga,
               struct gfi_t *gfi,
               size_t face_idx,
               fastf_t conv_factor,  /* conversion factor from obj file units to mm */
               struct bn_tol *tol,
               int *degenerate_face) {

    fastf_t tmp_v_o[3] = { 0.0, 0.0, 0.0 }; /* temporary vertex, referenced from outer loop */
    fastf_t tmp_v_i[3] = { 0.0, 0.0, 0.0 }; /* temporary vertex, referenced from inner loop */
    fastf_t tmp_w = 0.0 ; /* temporary weight */
    fastf_t tmp_n[3] = { 0.0, 0.0, 0.0 }; /* temporary normal */
    fastf_t tmp_t[3] = { 0.0, 0.0, 0.0 }; /* temporary texture vertex */
    size_t  nofi = 0; /* normal obj file index */
    size_t  tofi = 0; /* texture vertex obj file index */
    fastf_t distance_between_vertices = 0.0 ;
    size_t  vofi_o = 0; /* vertex obj file index, referenced from outer loop */
    size_t  vofi_i = 0; /* vertex obj file index, referenced from inner loop */

    size_t vert = 0;
    size_t vert2 = 0;

    *degenerate_face = 0;
    /* added 1 to internal index values so warning message index values
       matches obj file index numbers. this is because obj file indexes
       start at 1, internally indexes start at 0.  changed the warning
       message if grouping_type is GRP_NONE because the group name and
       grouping index has no meaning to the user if grouping type is
       GRP_NONE */

    bu_log("test_face start\n");
    if ( gfi->num_vertices_arr[face_idx] < 3 ) {
        *degenerate_face = 1;
        if ( gfi->grouping_type != GRP_NONE ) 
            bu_log("WARNING: removed degenerate face (reason: < 3 vertices); obj file face group name = (%s) obj file face grouping index = (%lu) obj file face index = (%lu)\n",
               bu_vls_addr(gfi->raw_grouping_name), gfi->grouping_index,
               gfi->obj_file_face_idx_arr[face_idx] + 1);
        else
            bu_log("WARNING: removed degenerate face (reason: < 3 vertices); obj file face index = (%lu)\n",
               gfi->obj_file_face_idx_arr[face_idx] + 1);
    }

    while ( (vert < gfi->num_vertices_arr[face_idx]) && !*degenerate_face ) {
        vert2 = vert+1;
        while ( (vert2 < gfi->num_vertices_arr[face_idx]) && !*degenerate_face ) {
            retrieve_coord_index(ga, gfi, face_idx, vert, tmp_v_o,
                                 tmp_n, tmp_t, &tmp_w, &vofi_o, &nofi, &tofi); 
            retrieve_coord_index(ga, gfi, face_idx, vert2, tmp_v_i,
                                 tmp_n, tmp_t, &tmp_w, &vofi_i, &nofi, &tofi); 
            if ( vofi_o == vofi_i ) {
                /* test for duplicate vertex indexes in face */
                *degenerate_face = 1;
                if ( gfi->grouping_type != GRP_NONE )
                    bu_log("WARNING: removed degenerate face (reason: duplicate vertex index); obj file face group name = (%s) obj file face grouping index = (%lu) obj file face index = (%lu) obj file vertex index = (%lu)\n",
                       bu_vls_addr(gfi->raw_grouping_name), gfi->grouping_index,
                       gfi->obj_file_face_idx_arr[face_idx] + 1, vofi_o + 1);
                else
                    bu_log("WARNING: removed degenerate face (reason: duplicate vertex index); obj file face index = (%lu) obj file vertex index = (%lu)\n",
                       gfi->obj_file_face_idx_arr[face_idx] + 1, vofi_o + 1);
            } else {
                /* test for vertices closer than tol.dist */
                /* tol.dist is assumed to be mm */
                VSCALE(tmp_v_o, tmp_v_o, conv_factor);
                VSCALE(tmp_v_i, tmp_v_i, conv_factor);
                distance_between_vertices = DIST_PT_PT(tmp_v_o, tmp_v_i) ;
                if ( bn_pt3_pt3_equal(tmp_v_o, tmp_v_i, tol) ) {
                    *degenerate_face = 1;
                    if ( gfi->grouping_type != GRP_NONE )
                        bu_log("WARNING: removed degenerate face (reason: vertices too close); obj file face group name = (%s) obj file face grouping index = (%lu) obj file face index = (%lu) obj file vertice indexes (%lu) vs (%lu) tol.dist = (%lfmm) dist = (%fmm)\n",
                           bu_vls_addr(gfi->raw_grouping_name), gfi->grouping_index,
                           gfi->obj_file_face_idx_arr[face_idx] + 1, vofi_o + 1,
                           vofi_i + 1, tol->dist, distance_between_vertices);
                    else
                        bu_log("WARNING: removed degenerate face (reason: vertices too close); obj file face index = (%lu) obj file vertice indexes (%lu) vs (%lu) tol.dist = (%lfmm) dist = (%fmm)\n",
                           gfi->obj_file_face_idx_arr[face_idx] + 1, vofi_o + 1, vofi_i + 1,
                           tol->dist, distance_between_vertices);
                }
            }
            vert2++;
        }
        vert++;
    }
    bu_log("test_face end\n");
}

#if 0
=================================
int triangulate_face(const size_t *index_arr_faces, /* n-dimensional array of vertex indexes */
                     int index_arr_faces_dim,       /* dimension of index_arr_faces array */
                     size_t numVert,                /* number of vertices in polygon */
                     size_t *numTri,                /* number of triangles in current bot */
                     size_t *triangle_indexes_size, /* current allocated size of triangle_indexes */
                     size_t i) {                    /* libobj face index */

    /* triangle_indexes is global */
    /* size_t (*triangle_indexes)[3][2] */

    int idx = 0; 
    int numTriangles = 0; /* number of triangle faces which need to be created */
    size_t (*triangle_indexes_tmp)[3][2] = NULL ;

    /* size is the number of vertices in the current polygon */
    if ( numVert > 3 ) {
        numTriangles = numVert - 2 ;
    } else {
        numTriangles = 1 ;
    }

    /* numTriangles are the number of resulting triangles after triangulation */
    /* this loop triangulates the current polygon, only works if convex */
    for (idx = 0 ; idx < numTriangles ; idx++) {
        /* for each iteration of this loop, write all 6 indexes for the current triangle */

        /* triangle vertices indexes */
        triangle_indexes[*numTri][0][0] = index_arr_nv_faces[0][0] ;
        triangle_indexes[*numTri][1][0] = index_arr_nv_faces[idx+1][0] ;
        triangle_indexes[*numTri][2][0] = index_arr_nv_faces[idx+2][0] ;

        /* triangle vertices normals indexes */
        triangle_indexes[*numTri][0][1] = index_arr_nv_faces[0][1] ;
        triangle_indexes[*numTri][1][1] = index_arr_nv_faces[idx+1][1] ;
        triangle_indexes[*numTri][2][1] = index_arr_nv_faces[idx+2][1] ;

        bu_log("(%lu)(%lu)(%lu)(%lu)(%lu)(%lu)(%lu)(%lu)\n",
           i,
           *numTri,
           triangle_indexes[*numTri][0][0],
           triangle_indexes[*numTri][1][0],
           triangle_indexes[*numTri][2][0],
           triangle_indexes[*numTri][0][1],
           triangle_indexes[*numTri][1][1],
           triangle_indexes[*numTri][2][1]); 

        /* increment number of triangles in current grouping (i.e. current bot) */
        (*numTri)++;

        /* test if size of triangle_indexes needs to be increased */
        if (*numTri >= (triangle_indexes_size / num_indexes_per_triangle)) {
            /* compute how large to make next alloc */
            triangle_indexes_size = triangle_indexes_size +
                                              (num_triangles_per_alloc * num_indexes_per_triangle);

            triangle_indexes_tmp = bu_realloc(triangle_indexes, 
                                              sizeof(*triangle_indexes) * triangle_indexes_size,
                                              "triangle_indexes_tmp");
            triangle_indexes = triangle_indexes_tmp;
        }
    } /* this loop exits when all the triangles from the triangulated polygon 
         are written to the triangle_indexes array */
    return 0;
}
=================================
#endif

void free_gfi(struct gfi_t **gfi) {
    if (*gfi != NULL ) {
        bu_vls_free((*gfi)->raw_grouping_name);
        bu_free((*gfi)->raw_grouping_name, "(*gfi)->raw_grouping_name");
        bu_free((*gfi)->index_arr_faces, "(*gfi)->index_arr_faces");
        bu_free((*gfi)->num_vertices_arr, "(*gfi)->num_vertices_arr");
        bu_free((*gfi)->obj_file_face_idx_arr, "(*gfi)->obj_file_face_idx_arr");
        bu_free(*gfi, "*gfi");
        *gfi = NULL;
    }
    return;
}

/* this function allocates all memory needed for the
   gfi structure and its contents. gfi is expected to
   be a null pointer when passed to this function. the
   gfi structure and its contents is expected to be
   freed outside this function.
 */
void collect_grouping_faces_indexes(struct ga_t *ga,
                                    struct gfi_t **gfi,
                                    int face_type,
                                    int grouping_type,
                                    size_t grouping_index) { /* grouping_index is ignored if grouping_type
                                                                is set to GRP_NONE */

    size_t numFaces = 0; /* number of faces of the current face_type in the entire obj file */
    size_t i = 0;
    const size_t *attindex_arr_faces = (const size_t *)NULL;
    int found = 0;
    const char *name_str = (char *)NULL;
    size_t setsize = 0 ;
    const size_t *indexset_arr;
    size_t groupid = 0;
    size_t *num_vertices_arr_tmp = NULL ;
    size_t *obj_file_face_idx_arr_tmp = NULL ;

    const size_t (*index_arr_v_faces) = NULL ;      /* used by v_faces */
    const size_t (*index_arr_tv_faces)[2] = NULL ;  /* used by tv_faces */
    const size_t (*index_arr_nv_faces)[2] = NULL ;  /* used by nv_faces */
    const size_t (*index_arr_tnv_faces)[3] = NULL ; /* used by tnv_faces */

    arr_1D_t index_arr_faces_1D = NULL; 
    arr_2D_t index_arr_faces_2D = NULL; 
    arr_3D_t index_arr_faces_3D = NULL; 

    /* number of faces of the current face_type from the entire obj file
       which is found in the current grouping_type and current group */
    size_t numFacesFound = 0;

    /* number of additional elements to allocate memory
       for when the currently allocated memory is exhausted */ 
    const size_t max_faces_increment = 128;

    if ( *gfi != NULL ) {
        bu_log("ERROR: function collect_grouping_faces_indexes passed non-null for gfi\n");
        return;
    }

    switch (face_type) {
        case FACE_V :
            numFaces = ga->numFaces; 
            attindex_arr_faces = ga->attindex_arr_v_faces;
            break;
        case FACE_TV :
            numFaces = ga->numTexFaces; 
            attindex_arr_faces = ga->attindex_arr_tv_faces;
            break;
        case FACE_NV :
            numFaces = ga->numNorFaces;
            attindex_arr_faces = ga->attindex_arr_nv_faces;
            break;
        case FACE_TNV :
            numFaces = ga->numTexNorFaces;
            attindex_arr_faces = ga->attindex_arr_tnv_faces;
            break;
        default:
            bu_log("ERROR: logic error, invalid face_type in function 'collect_grouping_faces_indexes'\n");
            return;
    }

    /* traverse list of all faces in OBJ file of current face_type */
    for (i = 0 ; i < numFaces ; i++) {

        const obj_polygonal_attributes_t *face_attr;
        face_attr = ga->polyattr_list + attindex_arr_faces[i];

        /* reset for next face */
        found = 0;

        /* for each type of grouping, check if current face is in current grouping */
        switch (grouping_type) {
            case GRP_NONE :
                found = 1;
                /* since there is no grouping, still need a somewhat useful name 
                   for the brlcad primitive and region, set the name to the face_type
                   which is the inherent grouping */
                switch (face_type) {
                    case FACE_V :
                        name_str = "v" ;
                        break;
                    case FACE_TV :
                        name_str = "tv" ;
                        break;
                    case FACE_NV :
                        name_str = "nv" ;
                        break;
                    case FACE_TNV :
                        name_str = "tnv" ;
                        break;
                    default:
                        bu_log("ERROR: logic error, invalid face_type in function 'collect_grouping_faces_indexes'\n");
                        return;
                }
                break;
            case GRP_GROUP :
                /* setsize is the number of groups the current nv_face belongs to */
                setsize = obj_groupset(ga->contents,face_attr->groupset_index,&indexset_arr);

                /* loop through each group this face is in */
                for (groupid = 0 ; groupid < setsize ; groupid++) {
                    /* if true, current face is in current group grouping */
                    if ( grouping_index == indexset_arr[groupid] ) {
                        found = 1;
                        name_str = ga->str_arr_obj_groups[indexset_arr[groupid]];
                    }
                }
                break;
            case GRP_OBJECT :
                /* if true, current face is in current object grouping */
                if ( grouping_index == face_attr->object_index ) {
                    found = 1;
                    name_str = ga->str_arr_obj_objects[face_attr->object_index];
                }
                break;
            case GRP_MATERIAL :
                /* if true, current face is in current material grouping */
                if ( grouping_index == face_attr->material_index ) {
                    found = 1;
                    name_str = ga->str_arr_obj_materials[face_attr->material_index];
                }
                break;
            case GRP_TEXTURE :
                /* if true, current face is in current texture map grouping */
                if ( grouping_index == face_attr->texmap_index ) {
                    found = 1;
                    name_str = ga->str_arr_obj_texmaps[face_attr->texmap_index];
                }
                break;
            default:
                bu_log("ERROR: logic error, invalid grouping_type in function 'collect_grouping_faces_indexes'\n");
                return;
        }

        /* if found the first face allocate the output structure and initial allocation
           of the index_arr_faces, num_vertices_arr and obj_file_face_idx_arr arrays */
        if ( found && (numFacesFound == 0)) {

             bu_log("allocating memory for gfi structure and initial size of internal arrays\n");

            /* allocate memory for gfi structure */
            *gfi = (struct gfi_t *)bu_calloc(1, sizeof(struct gfi_t), "gfi");

            /* initialize gfi structure */
            memset((void *)*gfi, 0, sizeof(struct gfi_t)); /* probably redundant */
            (*gfi)->index_arr_faces = (void *)NULL;
            (*gfi)->num_vertices_arr = (size_t *)NULL;
            (*gfi)->obj_file_face_idx_arr = (size_t *)NULL;
            (*gfi)->raw_grouping_name = (struct  bu_vls *)NULL;
            (*gfi)->num_faces = 0;
            (*gfi)->max_faces = 0;
            (*gfi)->tot_vertices = 0;
            (*gfi)->face_type = 0;
            (*gfi)->grouping_type = 0;
            (*gfi)->grouping_index = 0;

            /* set face_type, grouping_type, grouping_index inside gfi structure,
               the purpose of this is so functions called later do not need to pass
               this in seperately */
            (*gfi)->face_type = face_type;
            (*gfi)->grouping_type = grouping_type;
            if ( grouping_type != GRP_NONE ) {
                (*gfi)->grouping_index = grouping_index;
            } else {
                /* set grouping_index to face_type since if grouping_type is
                   GRP_NONE, inherently we are grouping by face_type and we
                   need a number for unique naming of the brlcad objects. */
                (*gfi)->grouping_index = (size_t)abs(face_type);

            }
            /* allocate and initialize variable length string (vls) for raw_grouping_name */
            (*gfi)->raw_grouping_name = (struct bu_vls *)bu_calloc(1, sizeof(struct bu_vls), "raw_grouping_name");
            bu_vls_init((*gfi)->raw_grouping_name);

            /* only need to copy in the grouping name for the first face found within the
               grouping since all the faces in the grouping will have the same grouping name */
            bu_vls_strcpy((*gfi)->raw_grouping_name, name_str);

            /* sets initial number of elements to allocate memory for */
            (*gfi)->max_faces = max_faces_increment;

            (*gfi)->num_vertices_arr = (size_t *)bu_calloc((*gfi)->max_faces, 
                                                           sizeof(size_t), "num_vertices_arr");

            (*gfi)->obj_file_face_idx_arr = (size_t *)bu_calloc((*gfi)->max_faces, 
                                                           sizeof(size_t), "obj_file_face_idx_arr");

            /* allocate initial memory for (*gfi)->index_arr_faces based on face_type */
            switch (face_type) {
                case FACE_V :
                    (*gfi)->index_arr_faces = (void *)bu_calloc((*gfi)->max_faces, 
                                                                sizeof(arr_1D_t), "index_arr_faces");
                    index_arr_faces_1D = (arr_1D_t)((*gfi)->index_arr_faces);
                    break;
                case FACE_TV :
                case FACE_NV :
                    (*gfi)->index_arr_faces = (void *)bu_calloc((*gfi)->max_faces, 
                                                                sizeof(arr_2D_t), "index_arr_faces");
                    index_arr_faces_2D = (arr_2D_t)((*gfi)->index_arr_faces);
                    break;
                case FACE_TNV :
                    (*gfi)->index_arr_faces = (void *)bu_calloc((*gfi)->max_faces, 
                                                                sizeof(arr_3D_t), "index_arr_faces");
                    index_arr_faces_3D = (arr_3D_t)((*gfi)->index_arr_faces);
                    break;
                default:
                    bu_log("ERROR: logic error, invalid face_type in function 'collect_grouping_faces_indexes'\n");
                    return;
            }
        }

        if ( found ) {

            /* assign obj file face index into array for tracking errors back
               to the face within the obj file */
            (*gfi)->obj_file_face_idx_arr[numFacesFound] = attindex_arr_faces[i];

            switch (face_type) {
                case FACE_V :
                    (*gfi)->num_vertices_arr[numFacesFound] = \
                            obj_polygonal_v_face_vertices(ga->contents,i,&index_arr_v_faces);
                    index_arr_faces_1D[numFacesFound] = index_arr_v_faces;
                    break;
                case FACE_TV :
                    (*gfi)->num_vertices_arr[numFacesFound] = \
                            obj_polygonal_tv_face_vertices(ga->contents,i,&index_arr_tv_faces);
                    index_arr_faces_2D[numFacesFound] = index_arr_tv_faces;
                    break;
                case FACE_NV :
                    (*gfi)->num_vertices_arr[numFacesFound] = \
                            obj_polygonal_nv_face_vertices(ga->contents,i,&index_arr_nv_faces);
                    index_arr_faces_2D[numFacesFound] = index_arr_nv_faces;
                    break;
                case FACE_TNV :
                    (*gfi)->num_vertices_arr[numFacesFound] = \
                            obj_polygonal_tnv_face_vertices(ga->contents,i,&index_arr_tnv_faces);
                    index_arr_faces_3D[numFacesFound] = index_arr_tnv_faces;
                    break;
                default:
                    bu_log("ERROR: logic error, invalid face_type in function 'collect_grouping_faces_indexes'\n");
                    return;
            }

            (*gfi)->tot_vertices += (*gfi)->num_vertices_arr[numFacesFound];

            /* if needed, increase size of (*gfi)->num_vertices_arr and (*gfi)->index_arr_faces */
            if ( numFacesFound >= (*gfi)->max_faces ) {
                (*gfi)->max_faces += max_faces_increment;

                bu_log("realloc\n");

                num_vertices_arr_tmp = (size_t *)bu_realloc((*gfi)->num_vertices_arr,
                                       sizeof(size_t) * (*gfi)->max_faces, "num_vertices_arr_tmp");
                (*gfi)->num_vertices_arr = num_vertices_arr_tmp;

                obj_file_face_idx_arr_tmp = (size_t *)bu_realloc((*gfi)->obj_file_face_idx_arr,
                                       sizeof(size_t) * (*gfi)->max_faces, "obj_file_face_idx_arr_tmp");
                (*gfi)->obj_file_face_idx_arr = obj_file_face_idx_arr_tmp;

                switch (face_type) {
                    case FACE_V :
                        (*gfi)->index_arr_faces = (void *)bu_realloc(index_arr_faces_1D,
                                                  sizeof(arr_1D_t) * (*gfi)->max_faces, "index_arr_faces");
                        index_arr_faces_1D = (arr_1D_t)((*gfi)->index_arr_faces);
                        break;
                    case FACE_TV :
                    case FACE_NV :
                        (*gfi)->index_arr_faces = (void *)bu_realloc(index_arr_faces_2D,
                                                  sizeof(arr_2D_t) * (*gfi)->max_faces, "index_arr_faces");
                        index_arr_faces_2D = (arr_2D_t)((*gfi)->index_arr_faces);
                        break;
                    case FACE_TNV :
                        (*gfi)->index_arr_faces = (void *)bu_realloc(index_arr_faces_3D,
                                                  sizeof(arr_3D_t) * (*gfi)->max_faces, "index_arr_faces");
                        index_arr_faces_3D = (arr_3D_t)((*gfi)->index_arr_faces);
                        break;
                    default:
                        bu_log("ERROR: logic error, invalid face_type in function 'collect_grouping_faces_indexes'\n");
                        return;
                }
            }

            numFacesFound++; /* increment this at the end since arrays start at zero */
        }

    }  /* numFaces loop, when loop exits, all faces have been reviewed */

    if ( numFacesFound ) { 
        (*gfi)->num_faces = numFacesFound ;
    } else {
        *gfi = NULL ; /* this should already be null if no faces were found */
    }

    return;
}

void output_to_nmg(struct ga_t *ga,
                   struct gfi_t *gfi,
                   struct rt_wdb *outfp, 
                   fastf_t conv_factor, 
                   struct bn_tol *tol) {

    struct model *m = (struct model *)NULL;
    struct nmgregion *r = (struct nmgregion *)NULL;
    struct shell *s = (struct shell *)NULL;
    struct bu_ptbl faces;  /* table of faces for one element */
    struct faceuse *fu;
    struct vertexuse *vu = NULL;
    struct vertexuse *vu2 = NULL;
    struct loopuse *lu = NULL;
    struct edgeuse *eu = NULL;
    struct loopuse *lu2 = NULL;
    struct edgeuse *eu2 = NULL;
    size_t face_idx = 0; /* index into faces within for-loop */
    size_t vert_idx = 0; /* index into vertices within for-loop */
    size_t shell_vert_idx = 0; /* index into vertices for entire nmg shell */
    int total_fused_vertex = 0 ;
    plane_t pl; /* plane equation for face */
    fastf_t tmp_v[3] = { 0.0, 0.0, 0.0 }; /* temporary vertex */
    fastf_t tmp_w = 0.0 ; /* temporary weight */
    fastf_t tmp_rn[3] = { 0.0, 0.0, 0.0 }; /* temporary reverse normal */
    fastf_t tmp_n[3] = { 0.0, 0.0, 0.0 }; /* temporary normal */
    fastf_t tmp_t[3] = { 0.0, 0.0, 0.0 }; /* temporary texture vertex */
    vect_t  norm;
    fastf_t dot;
    size_t  vofi = 0; /* vertex obj file index */
    size_t  nofi = 0; /* normal obj file index */
    size_t  tofi = 0; /* texture vertex obj file index */
    int degenerate_face = 0;

    struct vertex  **verts = NULL;

    size_t num_faces_killed = 0 ; /* number of degenerate faces killed in the current shell */

    m = nmg_mm();
    r = nmg_mrsv(m);
    s = BU_LIST_FIRST(shell, &r->s_hd);
    NMG_CK_MODEL(m);
    NMG_CK_REGION(r);
    NMG_CK_SHELL(s);

    /* initialize tables */
    bu_ptbl_init(&faces, 64, " &faces ");

    verts = (struct vertex **)bu_calloc(gfi->tot_vertices, sizeof(struct vertex *), "verts");
    memset((void *)verts, 0, sizeof(struct vertex *) * gfi->tot_vertices);

    shell_vert_idx = 0;
    bu_log("about to run chk shell just before assign fu for-loops\n");
    NMG_CK_SHELL(s);
    /* loop thru all the polygons (i.e. faces) to be placed in the current shell/region/model */
    for ( face_idx = 0 ; face_idx < gfi->num_faces ; face_idx++ ) {
        bu_log("history: num vertices in current polygon = (%lu)\n", gfi->num_vertices_arr[face_idx]);

        test_face(ga, gfi, face_idx, conv_factor, tol, &degenerate_face);

        if ( degenerate_face != 1 ) {

        fu = nmg_cface(s, (struct vertex **)&(verts[shell_vert_idx]), (int)(gfi->num_vertices_arr[face_idx]));
        lu = BU_LIST_FIRST(loopuse, &fu->lu_hd);
        eu = BU_LIST_FIRST(edgeuse, &lu->down_hd);
        lu2 = BU_LIST_FIRST(loopuse, &fu->fumate_p->lu_hd);
        eu2 = BU_LIST_FIRST(edgeuse, &lu2->down_hd);

        for ( vert_idx = 0 ; vert_idx < gfi->num_vertices_arr[face_idx] ; vert_idx++ ) {

            retrieve_coord_index(ga, gfi, face_idx, vert_idx, tmp_v, tmp_n, tmp_t,
                                 &tmp_w, &vofi, &nofi, &tofi); 

            VSCALE(tmp_v, tmp_v, conv_factor);
            VUNITIZE(tmp_n);
            VREVERSE(tmp_rn, tmp_n);

            bu_log("about to run nmg_vertex_gv\n");
            NMG_CK_VERTEX(eu->vu_p->v_p);
            nmg_vertex_gv(eu->vu_p->v_p, tmp_v);
#if 1
            switch (gfi->face_type) {
                case FACE_NV :
                case FACE_TNV :
                    /* assign this normal to all uses of this vertex */
                    for (BU_LIST_FOR(vu, vertexuse, &eu->vu_p->v_p->vu_hd)) {
                        NMG_CK_VERTEXUSE(vu);
                        bu_log("about to run nmg_vertex_nv\n");
                        nmg_vertexuse_nv(vu, tmp_n);
                    }
                    for (BU_LIST_FOR(vu2, vertexuse, &eu2->vu_p->v_p->vu_hd)) {
                        NMG_CK_VERTEXUSE(vu2);
                        bu_log("about to run nmg_vertex_nv\n");
                        nmg_vertexuse_nv(vu2, tmp_rn);
                    }
                    break;
                default:
                    break;
            }
#endif
            eu = BU_LIST_NEXT(edgeuse, &eu->l);
            eu2 = BU_LIST_NEXT(edgeuse, &eu2->l);
            shell_vert_idx++;

        } /* this loop exits when all the vertices and their normals
             for the current polygon/faceuse has been inserted into
             their appropriate structures */

#if 1
        if (nmg_fu_planeeqn(fu, tol) < 0) {
            bu_log("Failed nmg_fu_planeeqn\n");
            nmg_pr_fu_briefly( fu, "" );
            nmg_kfu(fu);
        }
#endif

#if 0
        if ( nmg_calc_face_g( fu ) )
            bu_log( "Failed to calculate plane eqn\n" );
#endif

        } /* close of degenerate_face if test */

    } /* loop exits when all polygons within the current grouping
         has been placed within one nmg shell, inside one nmg region
         and inside one nmg model */

    /* create the face equations here, which is after all the faceuse
       have been created and the vertex coordinates and their normals
       have been assigned */


    /* run nmg_model_vertex_fuse before nmg_calc_face_g ?? */
    /* run nmg_rebound before nmg_model_vertex_fuse */
    bu_log("about to run nmg_model_vertex_fuse\n");
    total_fused_vertex = nmg_model_vertex_fuse(m, tol);
    bu_log("total_fused_vertex = (%d)\n", total_fused_vertex);

    struct shell *s1;
    /* calculate plane equations for faces */
    for (BU_LIST_FOR(s1, shell, &r->s_hd))
    {
        NMG_CK_SHELL( s1 );
        for (BU_LIST_FOR(fu, faceuse, &s1->fu_hd))
        {
            NMG_CK_FACEUSE( fu );
            if ( fu->orientation == OT_SAME )
            {
                if ( nmg_calc_face_g( fu ) )
                    bu_log( "Failed to calculate plane eqn\n" );

                /* save the face in a table */
                bu_ptbl_ins( &faces, (long *)fu );
            }
        }
    }

#if 0
        NMG_CK_FACEUSE(fu);
        bu_log("about to run nmg_loop_plane_area\n");
        /* verify the current polygon is valid */
        if (nmg_loop_plane_area(BU_LIST_FIRST(loopuse, &fu->lu_hd), pl) < 0.0) {
            bu_log("Failed nmg_loop_plane_area\n");
            bu_log("about to run nmg_kfu just after neg area from nmg_loop_plane_area\n");
            nmg_pr_fu_briefly( fu, "" );
            nmg_kfu(fu);
            fu = (struct faceuse *)NULL;
            num_faces_killed++;
        } else {
            bu_log("about to run nmg_face_g\n");
            nmg_face_g(fu, pl); /* return is void */

            /* don't run nmg_fu_planeeqn, will fix
               polygons with vertices off the face 
               with function nmg_make_faces_within_tol */
            nmg_face_bb(fu->f_p, tol); /* return is void */
            bu_ptbl_ins(&faces, (long *)fu);
#if 0
            bu_log("about to run nmg_fu_planeeqn\n");
            if (nmg_fu_planeeqn(fu, tol) < 0) {
                bu_log("Failed nmg_fu_planeeqn\n");
                nmg_pr_fu_briefly( fu, "" );
                nmg_kfu(fu);
                fu = (struct faceuse *)NULL;
                num_faces_killed++;
            } else {
                bu_log("about to run nmg_face_bb\n");
                nmg_face_bb(fu->f_p, tol); /* return is void */
                bu_ptbl_ins(&faces, (long *)fu);
            }
#endif
        }
#endif

    bu_log("history: num_faces_killed = (%lu)\n", num_faces_killed);

    if (BU_PTBL_END(&faces)){

        bu_log("about to run nmg_break_long_edges\n");
        nmg_break_long_edges(s, tol);

        /* run nmg_model_vertex_fuse before nmg_calc_face_g ?? */
        /* run nmg_rebound before nmg_model_vertex_fuse */
        bu_log("about to run nmg_model_vertex_fuse\n");
        total_fused_vertex = nmg_model_vertex_fuse(m, tol);
        bu_log("total_fused_vertex = (%d)\n", total_fused_vertex);

        /* run nmg_gluefaces before nmg_make_faces_within_tol */
        /* run nmg_model_vertex_fuse before nmg_gluefaces */
        bu_log("about to run nmg_gluefaces\n");
        nmg_gluefaces((struct faceuse **)BU_PTBL_BASEADDR(&faces), BU_PTBL_END(&faces), tol);

        /* Mark edges as real */
        bu_log("about to run nmg_mark_edges_real\n");
        (void)nmg_mark_edges_real(&s->l.magic);

        /* Compute "geometry" for region and shell */
        bu_log("about to run nmg_region_a\n");
        nmg_region_a(r, tol);

        bu_log("about to run nmg_kill_cracks\n");
        nmg_kill_cracks(s);

        /* Some arbs may not be within tolerance, so triangulate faces where needed */
        /* run nmg_gluefaces before nmg_make_faces_within_tol */
        bu_log("about to run nmg_make_faces_within_tol\n");
        nmg_make_faces_within_tol(s, tol);

        bu_log("about to run nmg_rebound 1\n");
        nmg_rebound(m, tol);

#if 0
        /* run nmg_rebound before nmg_fix_normals */
        /* run nmg_model_edge_fuse before nmg_fix_normals */
        /* run nmg_gluefaces before nmg_fix_normals */
        bu_log("about to run nmg_fix_normals\n");
        nmg_fix_normals(s, tol);
#endif

#if 0
        bu_log("about to run nmg_shell_coplanar_face_merge\n");   
        nmg_shell_coplanar_face_merge(s, tol, 1);    
#endif

        bu_log("about to run nmg_rebound 2\n");
        nmg_rebound(m, tol);

#if 0
        bu_log("about to run nmg_close_shell\n");
        nmg_close_shell(s, tol);
#endif

#if 0
        /* testing nmg_invert_shell function */
        if (gfi->grouping_index == 82 || gfi->grouping_index == 17 || gfi->grouping_index == 81) {
            bu_log("about to run nmg_invert_shell for grouping_index 82,17,81\n");
            nmg_invert_shell(s);
        }
#endif

        /* run nmg_model_vertex_fuse before nmg_check_closed_shell */
        if ( nmg_check_closed_shell(s, tol) ) {
            /* make bot for a open shell */
            bu_vls_printf(gfi->raw_grouping_name, ".%lu.bot.s", gfi->grouping_index);
            cleanup_name(gfi->raw_grouping_name);
            bu_log("about to mk_bot_from_nmg\n");
            mk_bot_from_nmg(outfp, bu_vls_addr(gfi->raw_grouping_name), s);
            nmg_km(m);
        } else {
            /* make nmg for a closed shell */
            bu_vls_printf(gfi->raw_grouping_name, ".%lu.nmg.s", gfi->grouping_index);
            cleanup_name(gfi->raw_grouping_name);

#if 0
            bu_log("about to run nmg_fix_normals\n");
            nmg_fix_normals(s, tol);
#endif

            bu_log("about to run mk_nmg\n");
            /* the model (m) is freed when mk_nmg completes */
            if (mk_nmg(outfp, bu_vls_addr(gfi->raw_grouping_name), m)) {
                bu_log("mk_nmg failed\n");
            }
        }

#if 0
        bu_log("about to mk_bot_from_nmg\n");
        mk_bot_from_nmg(outfp, bu_vls_addr(gfi->raw_grouping_name), s);
        nmg_km(m);
#endif

#if 0
        bu_log("about to run mk_nmg\n");
        /* the model (m) is freed when mk_nmg completes */
        if (mk_nmg(outfp, bu_vls_addr(gfi->raw_grouping_name), m)) {
            bu_log("mk_nmg failed\n");
        }
#endif
    } else {
        bu_log("Object %s has no faces\n", bu_vls_addr(gfi->raw_grouping_name));
    } 

    bu_free(verts,"verts");
    bu_ptbl_reset(&faces);
}

int process_nv_faces(struct ga_t *ga, 
                     struct rt_wdb *outfp, 
                     int grp_mode, 
                     size_t grp_indx,
                     fastf_t conversion_factor, 
                     struct bn_tol *tol) {

    size_t size = 0 ;
    size_t setsize = 0 ;
    size_t vert = 0;
    size_t vert2 = 0;
    const size_t (*index_arr_nv_faces)[2]; /* used by nv_faces */
    const size_t *indexset_arr;
    size_t groupid = 0;
    size_t numNorTriangles_in_current_bot = 0;
    /* NMG2s */
    struct model *m = (struct model *)NULL;
    struct nmgregion *r = (struct nmgregion *)NULL;
    struct shell *s = (struct shell *)NULL;
    const size_t (**index_arr_nv_faces_history)[2]; /* used by nv_faces */
    const size_t (**index_arr_nv_faces_history_tmp)[2]; /* used by nv_faces */
    size_t *size_history = NULL ; 
    size_t *size_history_tmp = NULL ;
    size_t history_arrays_size = 0; 
    size_t numNorPolygons_in_current_shell = 0;
    size_t numNorPolygonVertices_in_current_nmg = 0 ;
    struct bu_ptbl faces;            /* table of faces for one element */
    struct bu_ptbl names2;            /* table of element names */
    struct faceuse *fu;
    size_t idx2 = 0;
    long   tmp_long = 0 ;
    /* NMG2e */
    int found = 0;
    size_t i = 0;
    int ret_val = 0;  /* 0=success !0=fail */

    fastf_t *bot_vertices = NULL;
    int *bot_faces_array = NULL; /* bot face vertex index array, passed to mk_bot function */
    fastf_t *thickness = NULL;
    fastf_t *normals = NULL;
    int *face_normals = NULL;
    size_t j = 0;

    struct bu_vls outputObjectName;

    int skip_degenerate_faces = 1; /* boolean */
    int degenerate_face = 0; /* boolean */

    size_t bot_vertex_array_size = 0;
    size_t bot_normals_array_size = 0;

    const size_t num_triangles_per_alloc = 128 ;
    const size_t num_indexes_per_triangle = 6 ; /* 3 vert/tri, 1 norm/vert, 2 idx/vert */

    size_t (*triangle_indexes_tmp)[3][2] = NULL ;
    size_t triangle_indexes_size = 0;

    /* NMG2s */

    /* initial number of history elements to create, one is required
       for each polygon in the current nmg */
    history_arrays_size = 128 ; 

    /* allocate memory for initial index_arr_nv_faces_history array */
    index_arr_nv_faces_history = bu_calloc(history_arrays_size,
                                                             sizeof(size_t *) * 2,
                                                             "index_arr_nv_faces_history");

    /* allocate memory for initial size_history array */
    size_history = (size_t *)bu_calloc(history_arrays_size, sizeof(size_t), "size_history");
    /* NMG2e */

    /* compute memory required for initial triangle_indexes array */
    triangle_indexes_size = num_triangles_per_alloc * num_indexes_per_triangle ;

    /* allocate memory for initial triangle_indexes array */
    triangle_indexes = bu_calloc(triangle_indexes_size, sizeof(*triangle_indexes), "triangle_indexes");

    bu_vls_init(&outputObjectName);

    /* traverse list of all nv_faces in OBJ file */
    for (i = 0 ; i < ga->numNorFaces ; i++) {

        const obj_polygonal_attributes_t *face_attr;
        face_attr = ga->polyattr_list + ga->attindex_arr_nv_faces[i];

        /* reset for next face */
        found = 0;

        /* for each type of grouping, check if current face is in current group */
        switch (grp_mode) {
            case GRP_GROUP :
                /* setsize is the number of groups the current nv_face belongs to */
                setsize = obj_groupset(ga->contents,face_attr->groupset_index,&indexset_arr);

                /* loop through each group this face is in */
                for (groupid = 0 ; groupid < setsize ; groupid++) {
                    /* if true, current face is in current group */
                    if ( grp_indx == indexset_arr[groupid] ) {
                        found = 1;
                        bu_vls_sprintf(&outputObjectName, "%s.%lu.surface.s", 
                           ga->str_arr_obj_groups[indexset_arr[groupid]], indexset_arr[groupid] );
                        cleanup_name(&outputObjectName);
                    }
                }
                break;
            case GRP_OBJECT :
                /* if true, current face is in current object group */
                if ( grp_indx == face_attr->object_index ) {
                    found = 1;
                    bu_vls_sprintf(&outputObjectName, "%s.%lu.surface.s", 
                       ga->str_arr_obj_objects[face_attr->object_index], face_attr->object_index );
                    cleanup_name(&outputObjectName);
                }
                break;
            case GRP_MATERIAL :
                /* if true, current face is in current material group */
                if ( grp_indx == face_attr->material_index ) {
                    found = 1;
                    bu_vls_sprintf(&outputObjectName, "%s.%lu.surface.s", 
                       ga->str_arr_obj_materials[face_attr->material_index], face_attr->material_index );
                    cleanup_name(&outputObjectName);
                }
                break;
            case GRP_TEXTURE :
                break;
            default:
                bu_log("ERROR: logic error, invalid grp_mode in function 'process_nv_faces'\n");
                ret_val = 1;
                break;
        }

        /* only find number of vertices for current face if the current face */
        /* is in the current group */
        if ( found ) {
            size = obj_polygonal_nv_face_vertices(ga->contents,i,&index_arr_nv_faces);
            if (size < 3) {
                found = 0; /* i.e. unfind this face */
                           /* all faces must have at least 3 vertices */
            }
        }

	if ( found && !numNorPolygons_in_current_shell) {
            m = nmg_mm();
            r = nmg_mrsv(m);
            s = BU_LIST_FIRST(shell, &r->s_hd);
            NMG_CK_MODEL(m);
            NMG_CK_REGION(r);
            NMG_CK_SHELL(s);
        }

        /* test for and force the skip of degenerate faces */
        /* in this case degenerate faces are those with duplicate vertices */
        if ( found && skip_degenerate_faces ) {
            /* within the current face, compares vertice indices for duplicates */
            /* stops looking after found 1st duplicate */
            degenerate_face = 0;
            vert = 0;
            while ( (vert < size) && !degenerate_face ) {
                vert2 = vert+1;
                while ( (vert2 < size) && !degenerate_face ) {
                    if ( index_arr_nv_faces[vert][0] == index_arr_nv_faces[vert2][0] ) {
                        found = 0; /* i.e. unfind this face */
                        degenerate_face = 1;
                        /* add 1 to internal index value so warning message index value */
                        /* matches obj file index number. this is because obj file indexes */
                        /* start at 1, internally indexes start at 0. */
                        bu_log("WARNING: removed degenerate face (reason dup index); grp_indx (%lu) face (%lu) vert index (%lu) = (%lu)\n",
                           grp_indx,
                           i+1, 
                           (index_arr_nv_faces[vert][0])+1, 
                           (index_arr_nv_faces[vert2][0])+1); 
                    } else {
                        /* test distance between vertices. faces with
                           vertices <= tol.dist adjusted to mm, will be
                           dropped */
                        fastf_t distance_between_vertices = 0.0 ;
                        distance_between_vertices = \
                           DIST_PT_PT(ga->vert_list[index_arr_nv_faces[vert][0]], \
                                      ga->vert_list[index_arr_nv_faces[vert2][0]]) ;
                        if ( 0 && (distance_between_vertices * conversion_factor) <= (tol->dist * outfp->dbip->dbi_local2base)) {
                            found = 0; /* i.e. unfind this face */
                            degenerate_face = 1;
                            bu_log("WARNING: removed degenerate face (reason vertices too close); grp_indx (%lu) face (%lu) vert index (%lu) = (%lu) tol.dist = (%lfmm) dist = (%fmm)\n",
                               grp_indx,
                               i+1, 
                               (index_arr_nv_faces[vert][0])+1, 
                               (index_arr_nv_faces[vert2][0])+1, 
                               tol->dist * outfp->dbip->dbi_local2base,
                               distance_between_vertices * conversion_factor);
                        }
                    }
                    vert2++;
                }
                vert++;
            }
        }

        /* NMG2s */
        if ( found ) {

            index_arr_nv_faces_history[numNorPolygons_in_current_shell] = index_arr_nv_faces;
            size_history[numNorPolygons_in_current_shell] = size;

            numNorPolygonVertices_in_current_nmg = numNorPolygonVertices_in_current_nmg + size ;

                /* if needed, increase size of index_arr_nv_faces_history and size_history */
                if ( numNorPolygons_in_current_shell >= history_arrays_size ) {
                    history_arrays_size = history_arrays_size + 128 ;

                    index_arr_nv_faces_history_tmp = bu_realloc(index_arr_nv_faces_history,
                                                     sizeof(index_arr_nv_faces_history) * history_arrays_size,
                                                     "index_arr_nv_faces_history_tmp");

                    index_arr_nv_faces_history = index_arr_nv_faces_history_tmp;

                    size_history_tmp = bu_realloc(size_history, sizeof(size_t) * history_arrays_size,
                                       "size_history_tmp");

                    size_history = size_history_tmp;
                }

            /* increment number of polygons in current grouping (i.e. current nmg) */
            numNorPolygons_in_current_shell++;
        }
        /* NMG2e */

        /* execute this code when there is a face to process */
        if ( found ) {

            /* if found then size must be >= 3 */
            if ( size > 3 ) {
#if 0
                if (triangulate_face((const size_t *)index_arr_nv_faces,
                                     2,
                                     size,
                                     &numNorTriangles_in_current_bot,
                                     &triangle_indexes_size,
                                     i)) {
                    bu_log("error in triangulate_face\n");
                }
#endif
            } else {
                triangle_indexes[numNorTriangles_in_current_bot][0][0] = index_arr_nv_faces[0][0] ;
                triangle_indexes[numNorTriangles_in_current_bot][1][0] = index_arr_nv_faces[1][0] ;
                triangle_indexes[numNorTriangles_in_current_bot][2][0] = index_arr_nv_faces[2][0] ;

                triangle_indexes[numNorTriangles_in_current_bot][0][1] = index_arr_nv_faces[0][1] ;
                triangle_indexes[numNorTriangles_in_current_bot][1][1] = index_arr_nv_faces[1][1] ;
                triangle_indexes[numNorTriangles_in_current_bot][2][1] = index_arr_nv_faces[2][1] ;

                bu_log("(%lu)(%lu)(%lu)(%lu)(%lu)(%lu)(%lu)(%lu)\n",
                   i,
                   numNorTriangles_in_current_bot,
                   triangle_indexes[numNorTriangles_in_current_bot][0][0],
                   triangle_indexes[numNorTriangles_in_current_bot][1][0],
                   triangle_indexes[numNorTriangles_in_current_bot][2][0],
                   triangle_indexes[numNorTriangles_in_current_bot][0][1],
                   triangle_indexes[numNorTriangles_in_current_bot][1][1],
                   triangle_indexes[numNorTriangles_in_current_bot][2][1]); 

                /* increment number of triangles in current grouping (i.e. current bot) */
                numNorTriangles_in_current_bot++;

                /* test if size of triangle_indexes needs to be increased */
                if (numNorTriangles_in_current_bot >= (triangle_indexes_size / num_indexes_per_triangle)) {
                    /* compute how large to make next alloc */
                    triangle_indexes_size = triangle_indexes_size +
                                                      (num_triangles_per_alloc * num_indexes_per_triangle);

                    triangle_indexes_tmp = bu_realloc(triangle_indexes, 
                                                      sizeof(*triangle_indexes) * triangle_indexes_size,
                                                      "triangle_indexes_tmp");
                    triangle_indexes = triangle_indexes_tmp;
                }

            }
        }

    }  /* numNorFaces loop, when loop exits, all nv_faces have been reviewed */

    /* need to process the triangle_indexes to find only the indexes needed */

    if ( numNorTriangles_in_current_bot > 0 ) {
    size_t num_indexes = 0; /* for vertices and normals */

    /* num_indexes is the number of vertex indexes in triangle_indexes array */
    /* num_indexes is also the number of vertex normal indexes in triangle_indexes array */
    num_indexes = (numNorTriangles_in_current_bot * num_indexes_per_triangle) / 2 ;
    bu_log("#ntri (%lu) #ni/tri (%lu) #elems (%lu)\n", 
       numNorTriangles_in_current_bot,
       num_indexes_per_triangle,
       num_indexes);

    /* replace "some_ints" with "triangle_indexes" */
    /* where the data-type for "some_ints" is used, change to "size_t" */

    size_t last = 0 ;
    size_t k = 0 ;

    size_t counter = 0;

    size_t num_unique_vertex_indexes = 0 ;
    size_t num_unique_vertex_normal_indexes = 0 ;

    /* array to store sorted unique libobj vertex index values for current bot */
    size_t *unique_vertex_indexes = NULL ;

    /* array to store sorted unique libobj vertex normal index values for current bot */
    size_t *unique_vertex_normal_indexes = NULL ;

    size_t *vertex_sort_index = NULL ; 
    size_t *vertex_normal_sort_index = NULL ;

    vertex_sort_index = (size_t *)bu_calloc(num_indexes, sizeof(size_t), "vertex_sort_index");

    vertex_normal_sort_index = (size_t *)bu_calloc(num_indexes, sizeof(size_t), "vertex_normal_sort_index");

    /* populate index arrays */
    for (k = 0 ; k < num_indexes ; k++) {
        vertex_sort_index[k] = k*2 ;
        vertex_normal_sort_index[k] = (k*2)+1 ;
    }

    tmp_ptr = (size_t *)triangle_indexes;

    bu_log("non-sorted vertex_sort_index index contents ...\n");
    for (k = 0; k < num_indexes ; ++k) {
        bu_log("(%lu)\n", vertex_sort_index[k]);
    }

    /* sort vertex_sort_index containing indexes into vertex
       indexes within triangle_indexes array */
    qsort(vertex_sort_index, num_indexes, sizeof vertex_sort_index[0],
         (int (*)(const void *a, const void *b))comp);

    /* sort vertex_normal_sort_index containing indexes into vertex normal
       indexes within triangle_indexes array */
    qsort(vertex_normal_sort_index, num_indexes, sizeof vertex_normal_sort_index[0],
         (int (*)(const void *a, const void *b))comp);

    /* count sorted and unique libobj vertex indexes */
    last = tmp_ptr[vertex_sort_index[0]];
    num_unique_vertex_indexes = 1;
    for (k = 1; k < num_indexes ; ++k) {
        if (tmp_ptr[vertex_sort_index[k]] != last) {
            last = tmp_ptr[vertex_sort_index[k]];
            num_unique_vertex_indexes++;
        }
    }
    bu_log("num_unique_vertex_indexes = (%lu)\n", num_unique_vertex_indexes);

    /* count sorted and unique libobj vertex normal indexes */
    last = tmp_ptr[vertex_normal_sort_index[0]];
    num_unique_vertex_normal_indexes = 1;
    for (k = 1; k < num_indexes ; ++k) {
        if (tmp_ptr[vertex_normal_sort_index[k]] != last) {
            last = tmp_ptr[vertex_normal_sort_index[k]];
            num_unique_vertex_normal_indexes++;
        }
    }
    bu_log("num_unique_vertex_normal_indexes = (%lu)\n", num_unique_vertex_normal_indexes);

    unique_vertex_indexes = (size_t *)bu_calloc(num_unique_vertex_indexes, 
                                                sizeof(size_t), "unique_vertex_indexes");

    unique_vertex_normal_indexes = (size_t *)bu_calloc(num_unique_vertex_normal_indexes, 
                                                sizeof(size_t), "unique_vertex_normal_indexes");

    /* store sorted and unique libobj vertex indexes */
    bu_log("storing sorted and unique libobj vertex indexes\n");
    counter = 0;
    last = tmp_ptr[vertex_sort_index[0]];
    unique_vertex_indexes[counter] = last ;
    for (k = 1; k < num_indexes ; ++k) {
        if (tmp_ptr[vertex_sort_index[k]] != last) {
            last = tmp_ptr[vertex_sort_index[k]];
            counter++;
            unique_vertex_indexes[counter] = last ;
        }
    }

    /* store sorted and unique libobj vertex normal indexes */
    bu_log("storing sorted and unique libobj vertex normal indexes\n");
    counter = 0;
    last = tmp_ptr[vertex_normal_sort_index[0]];
    unique_vertex_normal_indexes[counter] = last ;
    for (k = 1; k < num_indexes ; ++k) {
        if (tmp_ptr[vertex_normal_sort_index[k]] != last) {
            last = tmp_ptr[vertex_normal_sort_index[k]];
            counter++;
            unique_vertex_normal_indexes[counter] = last ;
        }
    }

    /* output stored sorted and unique libobj vertex indexes */
    bu_log("stored sorted and unique libobj vertex indexes\n");
    for (k = 0; k < num_unique_vertex_indexes ; ++k)
        bu_log("(%lu)\n", unique_vertex_indexes[k]);

    /* output stored sorted and unique libobj vertex normal indexes */
    bu_log("stored sorted and unique libobj vertex normal indexes\n");
    for (k = 0; k < num_unique_vertex_normal_indexes ; ++k)
        bu_log("(%lu)\n", unique_vertex_normal_indexes[k]);

    bu_log("sorted vertex_sort_index & vertex_normal_sort_index index contents ...\n");
    for (k = 0; k < num_indexes ; ++k) {
        bu_log("(%lu)(%lu)\n", vertex_sort_index[k], vertex_normal_sort_index[k]);
    }

    bu_log("raw triangle_indexes contents ...\n");
    for (k = 0; k < (num_indexes * 2) ; ++k) {
        bu_log("(%lu)\n", tmp_ptr[k]);
    }

    bu_log("triangle_indexes vertex index contents ...\n");
    for (k = 0; k < (num_indexes * 2) ; k=k+2) {
        bu_log("(%lu)\n", tmp_ptr[k]);
    }

    bu_log("triangle_indexes vertex index normal contents ...\n");
    for (k = 1; k < (num_indexes * 2) ; k=k+2) {
        bu_log("(%lu)\n", tmp_ptr[k]);
    }

    /* compute memory required for bot vertices array */
    bot_vertex_array_size = num_unique_vertex_indexes * 3 ; /* i.e. 3 coordinates per vertex index */

    /* compute memory required for bot vertices normals array */
    bot_normals_array_size = num_unique_vertex_normal_indexes * 3 ; /* i.e. 3 fastf_t per normal */

    /* allocate memory for bot vertices array */
    bot_vertices = (fastf_t *)bu_calloc((size_t)bot_vertex_array_size, sizeof(fastf_t), "bot_vertices");

    /* allocate memory for bot normals array */
    normals = (fastf_t *)bu_calloc((size_t)bot_normals_array_size, sizeof(fastf_t), "normals");

    /* populate bot vertex array */
    /* places xyz vertices into bot structure */
    j = 0;
    for (k = 0 ; k < bot_vertex_array_size ; k=k+3 ) {
        bot_vertices[k] =    (ga->vert_list[unique_vertex_indexes[j]][0]) * conversion_factor;
        bot_vertices[k+1] =  (ga->vert_list[unique_vertex_indexes[j]][1]) * conversion_factor;
        bot_vertices[k+2] =  (ga->vert_list[unique_vertex_indexes[j]][2]) * conversion_factor;
        j++;
    }

    /* populate bot normals array */
    /* places normals into bot structure */
    j = 0;
    for (k = 0 ; k < bot_normals_array_size ; k=k+3 ) {
        normals[k] =    (ga->norm_list[unique_vertex_normal_indexes[j]][0]) ;
        normals[k+1] =  (ga->norm_list[unique_vertex_normal_indexes[j]][1]) ;
        normals[k+2] =  (ga->norm_list[unique_vertex_normal_indexes[j]][2]) ;
        j++;
    }

    bu_log("raw populated bot vertices contents\n");
    for (k = 0 ; k < bot_vertex_array_size ; k=k+3 ) {
        bu_log("(%lu) (%f) (%f) (%f)\n", k, bot_vertices[k], bot_vertices[k+1], bot_vertices[k+2]);
    }

    bu_log("raw populated bot normals contents\n");
    for (k = 0 ; k < bot_normals_array_size ; k=k+3 ) {
        bu_log("(%lu) (%f) (%f) (%f)\n", k, normals[k], normals[k+1], normals[k+2]);
    }

    /* allocate memory for bot face vertex index array (bot_faces_array) */
    bot_faces_array = (int *)bu_calloc(numNorTriangles_in_current_bot * 3, sizeof(int), "bot_faces_array");

    /* allocate memory for bot face thickness array */
    thickness = (fastf_t *)bu_calloc(numNorTriangles_in_current_bot * 3, sizeof(fastf_t), "thickness");

    /* allocate memory for bot face_normals, i.e. indices into normals array */
    face_normals = (int *)bu_calloc(numNorTriangles_in_current_bot * 3, sizeof(int), "face_normals");

    size_t *res0 = NULL;
    size_t *res1 = NULL;
    size_t *res2 = NULL;

    /* for each triangle, map libobj vertex indexes to bot vertex
       indexes, i.e. populate bot faces array */
    for (k = 0 ; k < numNorTriangles_in_current_bot ; k++ ) {

        res0 = bsearch(&(triangle_indexes[k][0][0]),unique_vertex_indexes,
                            num_unique_vertex_indexes, sizeof(size_t),
                            (int (*)(const void *a, const void *b))comp_b) ;
        res1 = bsearch(&(triangle_indexes[k][1][0]),unique_vertex_indexes,
                            num_unique_vertex_indexes, sizeof(size_t),
                            (int (*)(const void *a, const void *b))comp_b) ;
        res2 = bsearch(&(triangle_indexes[k][2][0]),unique_vertex_indexes,
                            num_unique_vertex_indexes, sizeof(size_t),
                            (int (*)(const void *a, const void *b))comp_b) ;

        /* should not need to test for null return from bsearch since we
           know all values are in the list we just don't know where */
        if ( res0 == NULL || res1 == NULL || res2 == NULL ) {
            bu_log("ERROR: bsearch returned null\n");
            return EXIT_FAILURE;
        }

        /* bsearch returns pointer to matching element, but need the index of
           the element, pointer subtraction computes the correct index value
           of the element */
        bot_faces_array[k*3] = (int) (res0 - unique_vertex_indexes);
        bot_faces_array[(k*3)+1] = (int) (res1 - unique_vertex_indexes);
        bot_faces_array[(k*3)+2] = (int) (res2 - unique_vertex_indexes);
        thickness[(k*3)] = thickness[(k*3)+1] = thickness[(k*3)+2] = 1.0;

        bu_log("libobj to bot vert idx mapping (%lu), (%lu) --> (%d), (%lu) --> (%d), (%lu) --> (%d)\n",
           k,
           triangle_indexes[k][0][0], bot_faces_array[k*3],
           triangle_indexes[k][1][0], bot_faces_array[(k*3)+1],
           triangle_indexes[k][2][0], bot_faces_array[(k*3)+2] );
    }

    /* write bot to ".g" file */
#if 0
    ret_val = mk_bot(outfp, bu_vls_addr(&outputObjectName), RT_BOT_SURFACE, RT_BOT_UNORIENTED, 0, 
                     numNorTriangles_in_current_bot*3, numNorTriangles_in_current_bot, bot_vertices,
                     bot_faces_array, (fastf_t *)NULL, (struct bu_bitv *)NULL);
#endif

    /* NMG2s */
    /* initialize tables */
    bu_ptbl_init(&faces, 64, " &faces ");
    bu_ptbl_init(&names2, 64, " &names2 ");

    struct vertex  **nmg_verts = NULL;
    nmg_verts = (struct vertex **)bu_calloc(numNorPolygonVertices_in_current_nmg,
                                              sizeof(struct vertex *), "nmg_verts");
    memset((void *)nmg_verts, 0, sizeof(struct vertex *) * numNorPolygonVertices_in_current_nmg);

    size_t polygon3 = 0;
    size_t vertex4 = 0;
    size_t counter2 = 0;
    size_t end_count = numNorPolygons_in_current_shell ;
    struct vertexuse *vu = NULL;
    struct loopuse *lu = NULL;
    struct edgeuse *eu = NULL;
    int total_fused_vertex = 0 ;
    plane_t pl; /* plane equation for face */
    fastf_t tmp_v[3] = { 0.0, 0.0, 0.0 };
    fastf_t tmp_n[3] = { 0.0, 0.0, 0.0 };
    counter2 = 0;
    bu_log("about to run chk shell just before assign fu for-loops\n");
    NMG_CK_SHELL(s);
    bu_log("history: numNorPolygons_in_current_shell = (%lu)\n", numNorPolygons_in_current_shell);
    /* loop thru all the polygons (i.e. faces) to be placed in the current shell/region/model */
    for ( polygon3 = 0 ; polygon3 < end_count ; polygon3++ ) {
        bu_log("history: num vertices in current polygon = (%lu)\n", size_history[polygon3]);
        fu = nmg_cface(s, (struct vertex **)&(nmg_verts[counter2]), (int)size_history[polygon3] );
        lu = BU_LIST_FIRST(loopuse, &fu->lu_hd);
        eu = BU_LIST_FIRST(edgeuse, &lu->down_hd);
        for ( vertex4 = 0 ; vertex4 < size_history[polygon3] ; vertex4++ ) {
            bu_log("history: (%lu)(%lu)(%lu)(%f)(%f)(%f)(%f)(%f)(%f)(%f)\n", 
               polygon3,
               vertex4,
               index_arr_nv_faces_history[polygon3][vertex4][0],
               (ga->vert_list[index_arr_nv_faces_history[polygon3][vertex4][0]][0]) * conversion_factor,
               (ga->vert_list[index_arr_nv_faces_history[polygon3][vertex4][0]][1]) * conversion_factor,
               (ga->vert_list[index_arr_nv_faces_history[polygon3][vertex4][0]][2]) * conversion_factor,
               ga->vert_list[index_arr_nv_faces_history[polygon3][vertex4][0]][3],
               ga->norm_list[index_arr_nv_faces_history[polygon3][vertex4][1]][0],
               ga->norm_list[index_arr_nv_faces_history[polygon3][vertex4][1]][1],
               ga->norm_list[index_arr_nv_faces_history[polygon3][vertex4][1]][2]);

            /* convert to mm and copy current vertex into tmp_v */
            VSCALE(tmp_v, (fastf_t *)ga->vert_list[ \
                                     index_arr_nv_faces_history[polygon3][vertex4][0]], \
                                     conversion_factor);

            bu_log("about to run nmg_vertex_gv\n");
            NMG_CK_VERTEX(eu->vu_p->v_p);
            nmg_vertex_gv(eu->vu_p->v_p, tmp_v);

            /* copy current normal into tmp_n */
            VMOVE(tmp_n, (fastf_t *)ga->vert_list[index_arr_nv_faces_history[polygon3][vertex4][1]]);

            /* assign this normal to all uses of this vertex */
            for (BU_LIST_FOR(vu, vertexuse, &eu->vu_p->v_p->vu_hd)) {
                NMG_CK_VERTEXUSE(vu);
                bu_log("about to run nmg_vertex_nv\n");
                nmg_vertexuse_nv(vu, tmp_n);
            }

            eu = BU_LIST_NEXT(edgeuse, &eu->l);
            counter2++;
        } /* this loop exits when all the vertices and their normals
             for the current polygon/faceuse has been inserted into
             their appropriate structures */

        bu_log("about to run nmg_loop_plane_area\n");
        /* verify the current polygon is valid */
        if (nmg_loop_plane_area(BU_LIST_FIRST(loopuse, &fu->lu_hd), pl) < 0.0) {
            bu_log("Failed planeeq\n");
            bu_log("about to run nmg_kfu just after neg area from nmg_loop_plane_area\n");
            nmg_kfu(fu);
            numNorPolygons_in_current_shell--;
        } else {
            bu_log("about to run nmg_face_g\n");
            nmg_face_g(fu, pl); /* return is void */
            bu_log("about to run nmg_face_bb\n");
            nmg_face_bb(fu->f_p, tol); /* return is void */
            bu_ptbl_ins(&faces, (long *)fu);
        }

    } /* loop exits when all polygons within the current grouping
         has been placed within one nmg shell, inside one nmg region
         and inside one nmg model */

    if (BU_PTBL_END(&faces)) {

        bu_log("about to run nmg_model_vertex_fuse\n");
        total_fused_vertex = nmg_model_vertex_fuse(m, tol);
        bu_log("total_fused_vertex = (%d)\n", total_fused_vertex);

        bu_log("about to run nmg_gluefaces\n");
        nmg_gluefaces((struct faceuse **)BU_PTBL_BASEADDR(&faces), BU_PTBL_END(&faces), tol);

        bu_log("about to run nmg_rebound 1\n");
        nmg_rebound(m, tol);

        bu_log("about to run nmg_fix_normals\n");
        nmg_fix_normals(s, tol);

        bu_log("about to run nmg_shell_coplanar_face_merge\n");   
        nmg_shell_coplanar_face_merge(s, tol, 1);    

        bu_log("about to run nmg_rebound 2\n");
        nmg_rebound(m, tol);

#if 0
        bu_log("about to mk_bot_from_nmg\n");
        mk_bot_from_nmg(outfp, bu_vls_addr(&outputObjectName), s);
        nmg_km(m);
#endif

#if 1
        bu_log("about to run mk_nmg\n");
        /* the model (m) is freed when mk_nmg completes */
        if (mk_nmg(outfp, bu_vls_addr(&outputObjectName), m)) {
            bu_log("mk_nmg failed\n");
        }
#endif
    } else {
        bu_log("Object %s has no faces\n", bu_vls_addr(&outputObjectName));
    } 
    /* NMG2e */

    bu_free(nmg_verts,"nmg_verts");
    bu_free(index_arr_nv_faces_history,"index_arr_nv_faces_history");
    bu_free(size_history,"size_history");
    bu_free(vertex_sort_index,"vertex_sort_index");
    bu_free(vertex_normal_sort_index,"vertex_normal_sort_index");
    bu_free(unique_vertex_indexes,"unique_vertex_indexes");
    bu_free(bot_vertices,"bot_vertices");
    bu_free(bot_faces_array,"bot_faces_array");
    bu_free(thickness,"thickness");
    bu_free(normals,"normals");
    bu_free(face_normals,"face_normals");
    bu_free(triangle_indexes, "triangle_indexes");

    bu_vls_free(&outputObjectName);

    bu_ptbl_reset(&faces);

    }

    return ret_val;
}

int
main(int argc, char **argv)
{
    int face_type_idx = 0;
    char *prog = *argv, buf[BUFSIZ];
    FILE *fd_in;	/* input file */
    struct rt_wdb *fd_out;	/* Resulting BRL-CAD file */
    struct region_s *region = NULL;
    int ret_val = 0;
    FILE *my_stream;
    struct ga_t ga;
    struct gfi_t *gfi = (struct gfi_t *)NULL; /* grouping face indices */
    size_t i = 0;
    int c;
    char grouping_option = 'o'; /* to be selected by user from command line */
    fastf_t conv_factor = 1000.0; /* to be selected by user from command line */
    int weiss_result;
    const char *parse_messages = (char *)0;
    int parse_err = 0;
    struct bn_tol tol_struct ;
    struct bn_tol *tol ;

    rt_g.NMG_debug = rt_g.NMG_debug & DEBUG_TRI ;

    /* initialize ga structure */
    memset((void *)&ga, 0, sizeof(struct ga_t));

    tol = &tol_struct;
    tol->magic = BN_TOL_MAGIC;
    tol->dist = 0.005 ;   /* to be selected by user from command line */
    tol->dist_sq = tol->dist * tol->dist;
    tol->perp = 1e-6;
    tol->para = 1 - tol->perp;

    bu_log("running fopen\n");
    if ((my_stream = fopen("/home/rweiss/diamond.obj","r")) == NULL) {
        bu_log("Unable to open file.\n");
        perror(prog);
        return EXIT_FAILURE;
    }

    bu_log("running obj_parser_create\n");
    if ((ret_val = obj_parser_create(&ga.parser)) != 0) {
        if (ret_val == ENOMEM) {
            bu_log("Can not allocate an obj_parser_t object, Out of Memory.\n");
        } else {
            bu_log("Can not allocate an obj_parser_t object, Undefined Error (%d)\n", ret_val);
        }

        /* it is unclear if obj_parser_destroy must be run if obj_parser_create failed */
        bu_log("obj_parser_destroy\n");
        obj_parser_destroy(ga.parser);

        bu_log("running fclose\n");
        if (fclose(my_stream) != 0) {
            bu_log("Unable to close file.\n");
        }

        perror(prog);
        return EXIT_FAILURE;
    }

    bu_log("running obj_fparse\n");
    if (parse_err = obj_fparse(my_stream,ga.parser,&ga.contents)) {
        if ( parse_err < 0 ) {
            /* syntax error */
            parse_messages = obj_parse_error(ga.parser); 
            bu_log("obj_fparse, Syntax Error.\n");
            bu_log("%s\n", parse_messages); 
        } else {
            /* parser error */
            if (parse_err == ENOMEM) {
                bu_log("obj_fparse, Out of Memory.\n");
            } else {
                bu_log("obj_fparse, Other Error.\n");
            }
        }

        /* it is unclear if obj_contents_destroy must be run if obj_fparse failed */
        bu_log("obj_contents_destroy\n");
        obj_contents_destroy(ga.contents);

        bu_log("obj_parser_destroy\n");
        obj_parser_destroy(ga.parser);

        bu_log("running fclose\n");
        if (fclose(my_stream) != 0) {
            bu_log("Unable to close file.\n");
        }

        perror(prog);
        return EXIT_FAILURE;
    }

    if ((fd_out = wdb_fopen("diamond.g")) == NULL) {
        bu_log("Cannot create new BRL-CAD file (%s)\n", "diamond.g");
        perror(prog);
        bu_exit(1, NULL);
    }

    bu_log("local2base=(%lf) base2local=(%lf)\n", fd_out->dbip->dbi_local2base, fd_out->dbip->dbi_base2local);

#if 0
    fd_out->wdb_tol.dist    = tol->dist ;
    fd_out->wdb_tol.dist_sq = fd_out->wdb_tol.dist * fd_out->wdb_tol.dist ;
    fd_out->wdb_tol.perp    = tol->perp ;
    fd_out->wdb_tol.para    = 1 - fd_out->wdb_tol.perp ;
#endif

    collect_global_obj_file_attributes(&ga);

    switch (grouping_option) {
        case 'n':
            bu_log("ENTERED 'n' PROCESSING\n");
            for ( face_type_idx = FACE_V ; face_type_idx <= FACE_TNV ; face_type_idx++ ) {
                collect_grouping_faces_indexes(&ga, &gfi, face_type_idx, GRP_NONE, 0);
                if (gfi != NULL) {
                    bu_log("name=(%s) #faces=(%lu)\n", bu_vls_addr(gfi->raw_grouping_name),
                       gfi->num_faces);
                    output_to_nmg(&ga, gfi, fd_out, conv_factor, tol);
                    free_gfi(&gfi);
                }
            }
            bu_log("EXITED 'n' PROCESSING\n");
            break;
        case 'g':
            bu_log("ENTERED 'g' PROCESSING\n");
            for (i = 0 ; i < ga.numGroups ; i++) {
                for ( face_type_idx = FACE_V ; face_type_idx <= FACE_TNV ; face_type_idx++ ) {
                    collect_grouping_faces_indexes(&ga, &gfi, face_type_idx, GRP_GROUP, i);
                    if (gfi != NULL) {
                        bu_log("name=(%s) #faces=(%lu)\n", bu_vls_addr(gfi->raw_grouping_name),
                           gfi->num_faces);
                        output_to_nmg(&ga, gfi, fd_out, conv_factor, tol);
                        free_gfi(&gfi);
                    }
                }
            }
            bu_log("EXITED 'g' PROCESSING\n");
            break;
        case 'o':
            bu_log("ENTERED 'o' PROCESSING\n");
            for (i = 0 ; i < ga.numObjects ; i++) {
                for ( face_type_idx = FACE_V ; face_type_idx <= FACE_TNV ; face_type_idx++ ) {
                    collect_grouping_faces_indexes(&ga, &gfi, face_type_idx, GRP_OBJECT, i);
                    if (gfi != NULL) {
                        bu_log("name=(%s) #faces=(%lu)\n", bu_vls_addr(gfi->raw_grouping_name),
                           gfi->num_faces);
                        output_to_nmg(&ga, gfi, fd_out, conv_factor, tol);
                        free_gfi(&gfi);
                    }
                }
            }
            bu_log("EXITED 'o' PROCESSING\n");
            break;
        case 'm':
            bu_log("ENTERED 'm' PROCESSING\n");
            for (i = 0 ; i < ga.numMaterials ; i++) {
                for ( face_type_idx = FACE_V ; face_type_idx <= FACE_TNV ; face_type_idx++ ) {
                    collect_grouping_faces_indexes(&ga, &gfi, face_type_idx, GRP_MATERIAL, i);
                    if (gfi != NULL) {
                        bu_log("name=(%s) #faces=(%lu)\n", bu_vls_addr(gfi->raw_grouping_name),
                           gfi->num_faces);
                        output_to_nmg(&ga, gfi, fd_out, conv_factor, tol);
                        free_gfi(&gfi);
                    }
                }
            }
            bu_log("EXITED 'm' PROCESSING\n");
            break;
        case 't':
            bu_log("ENTERED 't' PROCESSING\n");
            for (i = 0 ; i < ga.numTexmaps ; i++) {
                for ( face_type_idx = FACE_V ; face_type_idx <= FACE_TNV ; face_type_idx++ ) {
                    collect_grouping_faces_indexes(&ga, &gfi, face_type_idx, GRP_TEXTURE, i);
                    if (gfi != NULL) {
                        bu_log("name=(%s) #faces=(%lu)\n", bu_vls_addr(gfi->raw_grouping_name),
                           gfi->num_faces);
                        output_to_nmg(&ga, gfi, fd_out, conv_factor, tol);
                        free_gfi(&gfi);
                    }
                }
            }
            bu_log("EXITED 't' PROCESSING\n");
            break;
        default:
            break;
    }

    /* running cleanup functions */
    bu_log("obj_contents_destroy\n");
    obj_contents_destroy(ga.contents);

    bu_log("obj_parser_destroy\n");
    obj_parser_destroy(ga.parser);

    bu_log("running fclose\n");
    if (fclose(my_stream) != 0) {
        bu_log("Unable to close file.\n");
        perror(prog);
        return EXIT_FAILURE;
    }

    wdb_close(fd_out);

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
