/*                  O S L - R E N D E R E R . H
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
/** @file osl-renderer.h
 *
 * This code represents an interface to OSL system. Through it, one can
 * add OSL shaders and query colors, given some global parameters
 *
 */

#ifndef OSL_RENDERER_H
#define OSL_RENDERER_H

#include <stdio.h>
#include "vmath.h"

typedef struct Ray {
    point_t dir;
    point_t origin;
} Ray;

/* Shared struct by which the C shader and the C++ render system may
   exchange information
*/
struct RenderInfo {

    /* -- input -- */
    fastf_t screen_x;       /* Coordinates of the screen (if applicable) */
    fastf_t screen_y;
    point_t P;              /* Query point */
    point_t N;              /* Object normal */
    point_t I;              /* Incident ray direction */
    fastf_t u, v;           /* uv coordinates */
    point_t dPdu, dPdv;     /* uv tangents */
    int depth;              /* How many times the ray hit an object */
    fastf_t surfacearea;    /* FIXME */
    int isbackfacing;       /* FIXME */
    const char *shadername; /* Name of the shader we are querying */

    /* -- output -- */
    point_t pc; /* Color of the point (or multiplier) */
    int doreflection;     /* 1 if there will be reflection 0, otherwise */
    Ray out_ray;      /* output ray (in case of reflection) */
};

#  include "oslclosure.h"
#  include "render_svc.h"
/* FIXME  -- Add my own ShaderSystem? */
#  include "./oslexec_pvt.h"

using namespace OSL;

struct ThreadInfo {
    void *handle;
    ShadingContext *ctx;
    unsigned short Xi[3];
};

/* Class 'OSLRenderer' holds global information about OSL shader system.
   These information are hidden from the calling C code */
class OSLRenderer {

    const char *shadername;              /* name of the shader */
    ErrorHandler errhandler;
    ShaderGlobals globals;

    ShadingSystem *shadingsys;
    ShadingSystemImpl *ssi;
    SimpleRenderer rend;
    ShadingContext *ctx;
    void *handle;

    unsigned short Xi[3];                /* seed for RNG */

    /* Information about each shader of the renderer */
    struct OSLShader{
	const char *name;
	ShadingAttribStateRef state;
    };
    std::vector<OSLShader> shaders;

    const ClosureColor
	*ExecuteShaders(ShaderGlobals &globals, RenderInfo *info);

    /* Sample a primitive from the shaders group */
    const ClosurePrimitive* SamplePrimitive(Color3& weight,
					    const ClosureColor *closure,
					    float r);

    /* Helper function for SamplePrimitive */
    void SamplePrimitiveRecurse(const ClosurePrimitive*& r_prim,
				Color3& r_weight,
				const ClosureColor *closure,
				const Color3& weight, float& totw, float& r);

public:

    OSLRenderer();
    ~OSLRenderer();

    /* Add an OSL shader to the system */
    void AddShader(const char *shadername);

    /* Query a color */
    Color3 QueryColor(RenderInfo *info);

    static void Vec3toPoint_t(Vec3 s, point_t t){
	t[0] = s[0];
	t[1] = s[1];
	t[2] = s[2];
    }

};

/* extern "C" { */

/*     /\* Wrapper for OSLRenderer constructor *\/ */
/*     OSLRenderer * oslrenderer_init(); */

/*     /\* Wrapper for OSLRenderer::AddShader *\/ */
/*     void oslrenderer_add_shader(OSLRenderer *oslr, const char *shadername); */

/*     /\* Wrapper for OSLRenderer::QueryColor *\/ */
/*     void oslrenderer_query_color(OSLRenderer *oslr, RenderInfo *info); */

/*     /\* Wrapper for OSLRenderer destructor *\/ */
/*     void oslrenderer_free(OSLRenderer **osl); */

/* } */

#endif

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
