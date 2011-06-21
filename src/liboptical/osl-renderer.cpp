/*                O S L - R E N D E R E R . C P P
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
/** @file osl-renderer.cpp
 *
 * This code represents an interface to OSL system. Through it, one can
 * add OSL shaders and query colors, given some global parameters
 */

#include "osl-renderer.h"

OSLRenderer::OSLRenderer(){

    shadingsys = ShadingSystem::create(&rend, NULL, &errhandler);
    
    ssi = (ShadingSystemImpl *)shadingsys;
    
    handle = ssi->create_thread_info();
    ctx = ssi->get_context(handle);
}

OSLRenderer::~OSLRenderer(){

    fprintf(stderr, "[DEB] destructor\n");
    ssi->release_context(ctx, handle);
    ssi->destroy_thread_info(handle);
    ShadingSystem::destroy(shadingsys);
}
void OSLRenderer::AddShader(const char *shadername){

    OSLShader osl_sh;

    shadingsys->ShaderGroupBegin();
    shadingsys->Shader("surface", shadername, NULL);
    shadingsys->ShaderGroupEnd();

    osl_sh.name = shadername;
    osl_sh.state = shadingsys->state();
    shadingsys->clear_state();

    shaders.push_back(osl_sh);
}

Color3 OSLRenderer::QueryColor(RenderInfo *info){

    if(info->depth >= 5){
	//printf("[DEB] maximum level of recursion reached\n");
	return Color3(0.0f);
    }

    fastf_t y = info->screen_y;

    Xi[0] = rand();
    Xi[1] = rand();
    Xi[2] = rand();

    // execute shader
    ShaderGlobals globals;
    const ClosureColor *closure = ExecuteShaders(globals, info);

    if(closure == NULL){
	fprintf(stderr, "closure is null\n");
    }

    Color3 weight = Color3(0.0f);
    // sample primitive from closure tree
    const ClosurePrimitive *prim = SamplePrimitive(weight, closure, 0.5);

    if(prim) {
	if(prim->category() == OSL::ClosurePrimitive::BSDF) {
	    // sample BSDF closure
	    BSDFClosure *bsdf = (BSDFClosure*)prim;
	    Vec3 omega_in, zero(0.0f);
	    Color3 eval;
	    float pdf = 0.0;

	    bsdf->sample(globals.Ng, globals.I, zero, zero,
			 erand48(Xi), erand48(Xi),
			 omega_in, zero, zero, pdf, eval);

	    if(pdf != 0.0f) {
		OSLRenderer::Vec3toPoint_t(globals.P, info->out_ray.origin);
		OSLRenderer::Vec3toPoint_t(omega_in, info->out_ray.dir);
		info->doreflection = 1;
		return weight*eval/pdf;
	    }
	}
	else if(prim->category() == OSL::ClosurePrimitive::Emissive) {
	    // evaluate emissive closure

	    EmissiveClosure *emissive = (EmissiveClosure*)prim;
	    Color3 l = weight*emissive->eval(globals.Ng, globals.I);
	    /*
	      printf("[DEB] Returning a light: %.2lf %.2lf %.2lf\n",
	      l[0], l[1], l[2]);
	    */

	    return l;
	}
	else if(prim->category() == OSL::ClosurePrimitive::Background) {
	    // background closure just returns weight
	    printf("[Background]\n");
	    return weight;
	}
    }
    return Color3(0.0f);
}
/* -----------------------------------------------
 * Private methods
 * ----------------------------------------------- */

// void OSLRenderer::InitShaders(const char *shadername){

//     shadingsys->attribute("optimize", 2);
//     shadingsys->attribute("lockgeom", 1);

//     shadingsys->ShaderGroupBegin();
//     shadingsys->Shader("surface", shadername, NULL);
//     shadingsys->ShaderGroupEnd();

//     fprintf(stderr, "[DEB] inicializando shader state\n");

//     shaderstate = shadingsys->state();

//     shadingsys->clear_state();
// }
const ClosureColor * OSLRenderer::
ExecuteShaders(ShaderGlobals &globals, RenderInfo *info){

    /* Search for the given shader */
    int sh_id = -1;
    for(size_t i = 0; i < shaders.size(); i++){
	if (strcmp(shaders[i].name, info->shadername) == 0){
	    sh_id = i;
	    break;
	}
    }
    if(sh_id == -1){
	printf("[DEB] shader not found\n");
	return NULL;
    }

    memset(&globals, 0, sizeof(globals));

    VMOVE(globals.P, info->P);
    VMOVE(globals.I, info->I);
    VMOVE(globals.Ng, info->N);

    globals.N = globals.Ng;

    // u-v coordinates
    globals.u = info->u;
    globals.v = info->v;
    globals.u = 0;
    globals.v = 0;

    // u-v tangents
    VMOVE(globals.dPdu, info->dPdu);
    VMOVE(globals.dPdv, info->dPdv);

    // other
    globals.raytype |= ((info->depth == 0) & (1 << 1));
    globals.surfacearea = info->surfacearea;
    globals.backfacing = (globals.Ng.dot(globals.I) < 0.0f);
    globals.Ci = NULL;

    // execute shader
    ctx->execute(ShadUseSurface, *(shaders[sh_id].state),
		 globals);

    return globals.Ci;
}

const ClosurePrimitive * OSLRenderer::SamplePrimitive(Color3& weight, const ClosureColor *closure, float r){

    if(closure) {
	const ClosurePrimitive *prim = NULL;
	float totw = 0.0f;

	SamplePrimitiveRecurse(prim, weight, closure, Color3(1.0f), totw, r);
	weight *= totw;

	return prim;
    }
    return NULL;
}
void OSLRenderer::SamplePrimitiveRecurse(const ClosurePrimitive*& r_prim, Color3& r_weight, const ClosureColor *closure,
					 const Color3& weight, float& totw, float& r){

    if(closure->type == ClosureColor::COMPONENT) {

	ClosureComponent *comp = (ClosureComponent*)closure;
	ClosurePrimitive *prim = (ClosurePrimitive*)comp->data();
	float p, w = fabsf(weight[0]) + fabsf(weight[1]) + fabsf(weight[2]);

	if(w == 0.0f)
	    return;

	totw += w;

	if(!r_prim) {
	    // no primitive was found yet, so use this
	    r_prim = prim;
	    r_weight = weight/w;
	}
	else {

	    p = w/totw;

	    if(r < p) {
		// pick other primitive
		r_prim = prim;
		r_weight = weight/w;

		r = r/p;
	    }
	    else {
		// keep existing primitive
		r = (r + p)/(1.0f - p);
	    }
	}
    }
    else if(closure->type == ClosureColor::MUL) {
	ClosureMul *mul = (ClosureMul*)closure;

	SamplePrimitiveRecurse(r_prim, r_weight, mul->closure, mul->weight * weight, totw, r);
    }
    else if(closure->type == ClosureColor::ADD) {
	ClosureAdd *add = (ClosureAdd*)closure;
	
	SamplePrimitiveRecurse(r_prim, r_weight, add->closureA, weight, totw, r);
	SamplePrimitiveRecurse(r_prim, r_weight, add->closureB, weight, totw, r);
    }
}

/* -----------------------------------------------
 * Wrapper methods
 * ----------------------------------------------- */
OSLRenderer* oslrenderer_init(){
    OSLRenderer* oslr = new OSLRenderer();
    return oslr;
}
void oslrenderer_add_shader(OSLRenderer *oslr, const char *shadername){
    oslr->AddShader(shadername);
}
void oslrenderer_query_color(OSLRenderer *oslr, RenderInfo *info){
    Color3 pc = oslr->QueryColor(info);
    info->pc[0] = pc[0];
    info->pc[1] = pc[1];
    info->pc[2] = pc[2];
    //if(pc[0] < 0.5) fprintf(stderr, "[DEB] Returning black point\n");
}
void oslrenderer_free(OSLRenderer **osl){
    delete (*osl);
    *osl = NULL;
}

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8
