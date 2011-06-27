
/*                        S H _ O S L . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2011 United States Government as represented by
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
/** @file liboptical/sh_osl.c
 *
 * This shader is an interface for OSL shadying system. More information
 * when more features are implemented.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "./osl-renderer.h"

#include "vmath.h"
#include "raytrace.h"
#include "optical.h"

#define OSL_MAGIC 0x1837    /* make this a unique number for each shader */
#define CK_OSL_SP(_p) BU_CKMAG(_p, OSL_MAGIC, "osl_specific")

/* Oslrenderer system */
OSLRenderer *oslr = NULL;

/*
 * The shader specific structure contains all variables which are unique
 * to any particular use of the shader.
 */
struct osl_specific {
    long magic;              	 /* magic # for memory validity check */
    struct bu_vls shadername;
};

/* The default values for the variables in the shader specific structure */
static const
struct osl_specific osl_defaults = {
    OSL_MAGIC,
};

#define SHDR_NULL ((struct osl_specific *)0)
#define SHDR_O(m) bu_offsetof(struct osl_specific, m)
#define SHDR_AO(m) bu_offsetofarray(struct osl_specific, m)

/* description of how to parse/print the arguments to the shader */
struct bu_structparse osl_print_tab[] = {
    {"", 0, (char *)0, 0, BU_STRUCTPARSE_FUNC_NULL, NULL, NULL }
};

/* Parse rule to arguments */
struct bu_structparse osl_parse_tab[] = {
    {"%V", 1, "shadername", SHDR_O(shadername), BU_STRUCTPARSE_FUNC_NULL, 
     NULL, NULL},
    {"", 0, (char *)0, 0, BU_STRUCTPARSE_FUNC_NULL, NULL, NULL}
};

extern "C" {

    HIDDEN int osl_setup(register struct region *rp, struct bu_vls *matparm, 
			 genptr_t *dpp, const struct mfuncs *mfp, 
			 struct rt_i *rtip);

    HIDDEN int osl_render(struct application *ap, const struct partition *pp, 
			  struct shadework *swp, genptr_t dp);
    HIDDEN void osl_print(register struct region *rp, genptr_t dp);
    HIDDEN void osl_free(genptr_t cp);

}

/* The "mfuncs" structure defines the external interface to the shader.
 * Note that more than one shader "name" can be associated with a given
 * shader by defining more than one mfuncs struct in this array.
 * See sh_phong.c for an example of building more than one shader "name"
 * from a set of source functions.  There you will find that "glass" "mirror"
 * and "plastic" are all names for the same shader with different default
 * values for the parameters.
 */
struct mfuncs osl_mfuncs[] = {
    {MF_MAGIC,	"osl",	0,	MFI_NORMAL|MFI_HIT|MFI_UV,	0,     osl_setup,	osl_render,	osl_print,	osl_free },
    {0,		(char *)0,	0,		0,		0,     0,		0,		0,		0 }
};

/* 
 * The remaining code should be hidden from C callers
 * 
 */

/* O S L _ S E T U P
 *
 * This routine is called (at prep time)
 * once for each region which uses this shader.
 * Any shader-specific initialization should be done here.
 *
 * Returns:
 * 1 success
 * 0 success, but delete region
 * -1 failure
 */

extern "C" {

HIDDEN int osl_setup(register struct region *rp, struct bu_vls *matparm, 
		     genptr_t *dpp, const struct mfuncs *mfp, 
		     struct rt_i *rtip)
{
    register struct osl_specific *osl_sp;

    /* check the arguments */
    RT_CHECK_RTI(rtip);
    BU_CK_VLS(matparm);
    RT_CK_REGION(rp);

    if (rdebug&RDEBUG_SHADE)
	bu_log("osl_setup(%s)\n", rp->reg_name);

    /* Get memory for the shader parameters and shader-specific data */
    BU_GETSTRUCT(osl_sp, osl_specific);
    *dpp = (char *)osl_sp;

    /* -----------------------------------
     * Initialize the osl specific values
     * -----------------------------------
     */

    bu_vls_init(&osl_sp->shadername);
    osl_sp->magic = OSL_MAGIC;

    /* Parse the user's arguments to fill osl specifics */
    if (bu_struct_parse(matparm, osl_parse_tab, (char *)osl_sp) < 0){
	bu_free((genptr_t)osl_sp, "osl_specific");
	return -1;
    }

    /* -----------------------------------
     * Check for errors
     * -----------------------------------
     */
    if (bu_vls_strlen(&osl_sp->shadername) <= 0){
	/* FIXME shadername was not set. Use the default value */
	fprintf(stderr, "[Error] shadername was not set");
	return -1;
    }

    /* -----------------------------------
     * Initialize osl render system
     * -----------------------------------
     */
    /* If OSL system was not initialized yet, do it */
    /* FIXME: take care of multi-thread issues */
    if (oslr == NULL){
	oslr = new OSLRenderer();
    }
    /* Add this shader to OSL system */
    oslr->AddShader(osl_sp->shadername.vls_str);
    printf("Adding shader: %s\n", osl_sp->shadername.vls_str);
  
    if (rdebug&RDEBUG_SHADE) {
	bu_struct_print(" Parameters:", osl_print_tab, (char *)osl_sp);
    }

    return 1;
}
    
/*
 * O S L _ P R I N T
 */
HIDDEN void osl_print(register struct region *rp, genptr_t dp)
{
    bu_struct_print(rp->reg_name, osl_print_tab, (char *)dp);
}


/*
 * O S L _ F R E E
 */
HIDDEN void osl_free(genptr_t cp)
{
    register struct osl_specific *osl_sp =
	(struct osl_specific *)cp;
    bu_free(cp, "osl_specific");

    /* FIXME: take care of multi-thread issues */
    if(oslr != NULL){
	delete oslr;
	oslr = NULL;
    }
}


/*
 * O S L _ R E N D E R
 *
 * This is called (from viewshade() in shade.c) once for each hit point
 * to be shaded.  The purpose here is to fill in values in the shadework
 * structure.
 */
HIDDEN int osl_render(struct application *ap, const struct partition *pp, 
		      struct shadework *swp, genptr_t dp)
/* defined in ../h/shadework.h */
/* ptr to the shader-specific struct */
{
    register struct osl_specific *osl_sp =
	(struct osl_specific *)dp;
    point_t pt;
  
    VSETALL(pt, 0);

    /* check the validity of the arguments we got */
    RT_AP_CHECK(ap);
    RT_CHECK_PT(pp);
    CK_OSL_SP(osl_sp);

    if (rdebug&RDEBUG_SHADE)
	bu_struct_print("osl_render Parameters:", osl_print_tab,
			(char *)osl_sp);

    point_t scolor;
    VSETALL(scolor, 0.0f);

    int nsamples = 4;
    for(int s=0; s<nsamples; s++){

	/* -----------------------------------
	 * Fill in all necessary information for the OSL renderer
	 * -----------------------------------
	 */
	RenderInfo info;

	/* Set hit point */
	VMOVE(info.P, swp->sw_hit.hit_point);
    
	/* Set normal at the poit */
	VMOVE(info.N, swp->sw_hit.hit_normal);
    
	/* Set incidence ray direction */
	VMOVE(info.I, ap->a_inv_dir);
    
	/* U-V mapping stuff */
	info.u = swp->sw_uv.uv_u;
	info.v = swp->sw_uv.uv_v;
	VSETALL(info.dPdu, 0.0f);
	VSETALL(info.dPdv, 0.0f);
    
	/* x and y pixel coordinates */
	info.screen_x = ap->a_x;
	info.screen_y = ap->a_y;

	info.depth = ap->a_level;
	info.isbackfacing = 0;
	info.surfacearea = 1.0f;
    
	info.shadername = osl_sp->shadername.vls_str;

	/* We only perform reflection if application decides to */
	info.doreflection = 0;
    
	Color3 weight = oslr->QueryColor(&info);

	if(info.doreflection == 1){
	
	    /* Fire another ray */
	    struct application new_ap;
	    RT_APPLICATION_INIT(&new_ap);

	    new_ap.a_rt_i = ap->a_rt_i;
	    new_ap.a_onehit = 1;
	    new_ap.a_hit = ap->a_hit;
	    new_ap.a_miss = ap->a_miss;
	    new_ap.a_level = ap->a_level + 1;

	    VMOVE(new_ap.a_ray.r_dir, info.out_ray.dir);
	    VMOVE(new_ap.a_ray.r_pt, info.out_ray.origin);

	    (void)rt_shootray(&new_ap);

	    /* The resulting color is always on ap_color, but
	       we need to update it through sw_color */
	    Color3 rec;
	    VMOVE(rec, new_ap.a_color);
	    Color3 res = rec*weight;
	    VADD2(scolor, scolor, res);
	}
	else {
	    /* Final color */
	    VADD2(scolor, scolor, weight);
	}
    }
    VSCALE(swp->sw_color, scolor, 1.0/nsamples);

    return 1;
}
}


/*
 * Local Variables:
 * mode: C++
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
