/*
 *			S H A D E . C
 *
 *	Ray Tracing program, lighting model shader interface.
 *
 *  Notes -
 *	The normals on all surfaces point OUT of the solid.
 *	The incomming light rays point IN.
 *
 *  Authors -
 *	Michael John Muuss
 *	Phil Dykstra
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1989 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSview[] = "@(#)$Header$ (BRL)";
#endif

#include "conf.h"

#include <stdio.h>
#include <math.h>

#include "machine.h"
#include "vmath.h"
#include "rtlist.h"
#include "raytrace.h"
#include "./ext.h"
#include "./rdebug.h"
#include "./material.h"
#include "./mathtab.h"
#include "./light.h"

extern int	light_hit(), light_miss();	/* in light.c */

HIDDEN void	shade_inputs();


/*
 *			V I E W S H A D E
 *
 *  Call the material-specific shading function, after making certain
 *  that all shadework fields desired have been provided.
 *
 *  Returns -
 *	0 on failure
 *	1 on success
 *
 *	But of course, nobody cares what this returns.
 *	Everyone calls us as (void)viewshade()
 */
int
viewshade( ap, pp, swp )
struct application *ap;
register CONST struct partition *pp;
register struct shadework *swp;
{
	register struct mfuncs *mfp;
	register struct region *rp;
	register struct light_specific *lp;
	register int	want;

	RT_AP_CHECK(ap);
	RT_CK_RTI(ap->a_rt_i);

	swp->sw_hit = *(pp->pt_inhit);		/* struct copy */

	if( (mfp = (struct mfuncs *)pp->pt_regionp->reg_mfuncs) == MF_NULL )  {
		rt_log("viewshade:  reg_mfuncs NULL\n");
		return(0);
	}
	if( mfp->mf_magic != MF_MAGIC )  {
		rt_log("viewshade:  reg_mfuncs bad magic, %x != %x\n",
			mfp->mf_magic, MF_MAGIC );
		return(0);
	}

	rp = pp->pt_regionp;
	RT_CK_REGION(rp);

	/* Default color is white (uncolored) */
	if( rp->reg_mater.ma_override )  {
		VMOVE( swp->sw_color, rp->reg_mater.ma_color );
	}
	VMOVE( swp->sw_basecolor, swp->sw_color );

	if( swp->sw_hit.hit_dist < 0.0 )
		swp->sw_hit.hit_dist = 0.0;	/* Eye inside solid */
	ap->a_cumlen += swp->sw_hit.hit_dist;

	want = mfp->mf_inputs;

	/* If light information is not needed, set the light
	 * array to "safe" values,
	 * and claim that the light is visible, in case they are used.
	 */
	if( swp->sw_xmitonly )  want &= ~MFI_LIGHT;
	if( !(want & MFI_LIGHT) )  {
		register int	i;

		for( i = ap->a_rt_i->rti_nlights*3 - 1; i >= 0; i-- )
			swp->sw_intensity[i] = 1;

		i=0;
		for( RT_LIST_FOR( lp, light_specific, &(LightHead.l) ) )  {
			RT_CK_LIGHT(lp);
			swp->sw_visible[i++] = (char *)lp;
		}
	}

	/* If optional inputs are required, have them computed */
	if( want & (MFI_HIT|MFI_NORMAL|MFI_LIGHT|MFI_UV) )  {
		VJOIN1( swp->sw_hit.hit_point, ap->a_ray.r_pt,
			swp->sw_hit.hit_dist, ap->a_ray.r_dir );
		swp->sw_inputs |= MFI_HIT;
	}
	if( (swp->sw_inputs & want) != want )
		shade_inputs( ap, pp, swp, want );

	if( rdebug&RDEBUG_SHADE ) {
		rt_log("About to shade %s: using \"%s\" shader\n",
			rp->reg_name, mfp->mf_name);
		pr_shadework( "before mf_render", swp );
	}

	/* Invoke the actual shader (may be a tree of them) */
	(void)mfp->mf_render( ap, pp, swp, rp->reg_udata );

	if( rdebug&RDEBUG_SHADE ) {
		pr_shadework( "after mf_render", swp );
		rt_log("\n");
	}

	return(1);
}

/*
 *			S H A D E _ I N P U T S
 *
 *  Compute the necessary fields in the shadework structure.
 *
 *  Note that only hit_dist is valid in pp_inhit.
 *  RT_HIT_NORM() must be called if hit_norm is needed,
 *  after which pt_inflip must be handled.
 *  RT_HIT_UVCOORD() must have hit_point computed
 *  in advance.
 *
 *  If MFI_LIGHT is not on, the presumption is that the sw_visible[]
 *  array is not needed, or has been handled elsewhere.
 */
HIDDEN void
shade_inputs( ap, pp, swp, want )
struct application *ap;
register struct partition *pp;
register struct shadework *swp;
register int	want;
{
	register struct light_specific *lp;
	register int	have;

	/* These calcuations all have MFI_HIT as a pre-requisite */
	if( want & (MFI_NORMAL|MFI_LIGHT|MFI_UV) )
		want |= MFI_HIT;

	have = swp->sw_inputs;
	want &= ~have;		/* we don't want what we already have */

	if( want & MFI_HIT )  {
		VJOIN1( swp->sw_hit.hit_point, ap->a_ray.r_pt,
			swp->sw_hit.hit_dist, ap->a_ray.r_dir );
		have |= MFI_HIT;
	}

	if( want & MFI_NORMAL )  {
		if( pp->pt_inhit->hit_dist < 0.0 )  {
			/* Eye inside solid, orthoview */
			VREVERSE( swp->sw_hit.hit_normal, ap->a_ray.r_dir );
		} else {
			FAST fastf_t f;
			/* Get surface normal for hit point */
			/* Stupid SysV CPP needs this on one line */
			RT_HIT_NORM( &(swp->sw_hit), pp->pt_inseg->seg_stp, &(ap->a_ray) );

#ifdef never
			if( swp->sw_hit.hit_normal[X] < -1.01 || swp->sw_hit.hit_normal[X] > 1.01 ||
			    swp->sw_hit.hit_normal[Y] < -1.01 || swp->sw_hit.hit_normal[Y] > 1.01 ||
			    swp->sw_hit.hit_normal[Z] < -1.01 || swp->sw_hit.hit_normal[Z] > 1.01 )  {
			    	VPRINT("shade_inputs: N", swp->sw_hit.hit_normal);
				VSET( swp->sw_color, 9, 9, 0 );	/* Yellow */
				return;
			}
#endif
			if( pp->pt_inflip )  {
				VREVERSE( swp->sw_hit.hit_normal, swp->sw_hit.hit_normal );
				pp->pt_inflip = 0;	/* shouldnt be needed now??? */
			}

			/* Temporary check to make sure normals are OK */
			if( (f=VDOT( ap->a_ray.r_dir, swp->sw_hit.hit_normal )) > 0 )  {
				rt_log("shade_inputs(%s) flip N xy=%d,%d %s surf=%d dot=%g\n",
					pp->pt_inseg->seg_stp->st_name,
					ap->a_x, ap->a_y,
					rt_functab[pp->pt_inseg->seg_stp->st_id].ft_name,
					swp->sw_hit.hit_surfno, f);
				if( rdebug&RDEBUG_SHADE ) {
					VPRINT("Dir ", ap->a_ray.r_dir);
					VPRINT("Norm", swp->sw_hit.hit_normal);
				}
			}
		}
		have |= MFI_NORMAL;
	}
	if( want & MFI_UV )  {
		if( pp->pt_inhit->hit_dist < 0.0 )  {
			/* Eye inside solid, orthoview */
			swp->sw_uv.uv_u = swp->sw_uv.uv_v = 0.5;
			swp->sw_uv.uv_du = swp->sw_uv.uv_dv = 0;
		} else {
			RT_HIT_UVCOORD(	ap, pp->pt_inseg->seg_stp,
				&(swp->sw_hit), &(swp->sw_uv) );
		}
		if( swp->sw_uv.uv_u < 0 || swp->sw_uv.uv_u > 1 ||
		    swp->sw_uv.uv_v < 0 || swp->sw_uv.uv_v > 1 )  {
			rt_log("shade_inputs:  bad u,v=%e,%e du,dv=%g,%g seg=%s %s surf=%d.  Making green.\n",
				swp->sw_uv.uv_u, swp->sw_uv.uv_v,
				swp->sw_uv.uv_du, swp->sw_uv.uv_dv,
				pp->pt_inseg->seg_stp->st_name,
		    		rt_functab[pp->pt_inseg->seg_stp->st_id].ft_name,
		    		pp->pt_inhit->hit_surfno );
			VSET( swp->sw_color, 0, 9, 0 );	/* Green */
			return;
		}
		have |= MFI_UV;
	}
	if( want & MFI_LIGHT )  {
		register int	i;
		register fastf_t *intensity, *tolight;
		register fastf_t f;
		struct application sub_ap;

		/*
		 *  Determine light visibility
		 */
		i = 0;
		intensity = swp->sw_intensity;
		tolight = swp->sw_tolight;
		for( RT_LIST_FOR( lp, light_specific, &(LightHead.l) ) )  {
			RT_CK_LIGHT(lp);
			/* compute the light direction */
			if( lp->lt_infinite ) {
				/* Infinite lights are point sources, no fuzzy penumbra */
				VMOVE( tolight, lp->lt_vec );
			} else {
				/*
				 *  Dither light pos for penumbra by +/- 0.5 light radius;
				 *  this presently makes a cubical light source distribution.
				 */
				f = lp->lt_radius * 0.9;
				tolight[X] = lp->lt_pos[X] +
					rand_half(ap->a_resource->re_randptr)*f -
					swp->sw_hit.hit_point[X];
				tolight[Y] = lp->lt_pos[Y] +
					rand_half(ap->a_resource->re_randptr)*f -
					swp->sw_hit.hit_point[Y];
				tolight[Z] = lp->lt_pos[Z] +
					rand_half(ap->a_resource->re_randptr)*f -
					swp->sw_hit.hit_point[Z];
			}

			/*
			 *  If we have a normal, test against light direction
			 */
			if( (have & MFI_NORMAL) && (swp->sw_transmit <= 0) )  {
				if( VDOT(swp->sw_hit.hit_normal,tolight) < 0 ) {
					/* backfacing, opaque */
					swp->sw_visible[i] = (char *)0;
					goto next;
				}
			}
			VUNITIZE( tolight );

			/*
			 * See if ray from hit point to light lies within light beam
			 * Note: this is should always be true for infinite lights!
			 */
			if( -VDOT(tolight, lp->lt_aim) < lp->lt_cosangle )  {
				/* dark (outside of light beam) */
				swp->sw_visible[i] = (char *)0;
				goto next;
			}
			if( !(lp->lt_shadows) )  {
				/* "fill light" in beam, don't care about shadows */
				swp->sw_visible[i] = (char *)lp;
				VSETALL( intensity, 1 );
				goto next;
			}

			/*
			 *  Fire ray at light source to check for shadowing.
			 *  (This SHOULD actually return an energy spectrum).
			 *  Advance start point slightly off surface.
			 */
			sub_ap = *ap;			/* struct copy */
			VMOVE( sub_ap.a_ray.r_dir, tolight );
			{
				register fastf_t f;
				f = ap->a_rt_i->rti_tol.dist;
				VJOIN1( sub_ap.a_ray.r_pt,
					swp->sw_hit.hit_point,
					f, tolight );
			}
			sub_ap.a_hit = light_hit;
			sub_ap.a_miss = light_miss;
			sub_ap.a_user = -1;		/* sanity */
			sub_ap.a_uptr = (genptr_t)lp;	/* so we can tell.. */
			sub_ap.a_level = 0;
			/* Will need entry & exit pts, for filter glass */
			sub_ap.a_onehit = 2;

			VSETALL( sub_ap.a_color, 1 );	/* vis intens so far */
			sub_ap.a_purpose = lp->lt_name;	/* name of light shot at */
			if( rt_shootray( &sub_ap ) )  {
				/* light visible */
				swp->sw_visible[i] = (char *)lp;
				VMOVE( intensity, sub_ap.a_color );
			} else {
				/* dark (light obscured) */
				swp->sw_visible[i] = (char *)0;
			}
next:
			/* Advance to next light */
			i++;
			intensity += 3;
			tolight += 3;
		}
		have |= MFI_LIGHT;
	}

	/* Record which fields were filled in */
	swp->sw_inputs = have;

	if( (want & have) != want )
		rt_log("shade_inputs:  unable to satisfy request for x%x\n", want);
}
