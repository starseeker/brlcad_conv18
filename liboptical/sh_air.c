/*
 *	S H _ A I R . C
 *
 */
#include "conf.h"

#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
#include "./material.h"
#include "./mathtab.h"
#include "./rdebug.h"


#define air_MAGIC 0x41697200	/* "Air" */
struct air_specific {
	long	magic;
	double	d_p_m;	/* density per unit meter */
	double	scale;
	double	delta;
	double	color[3];
	int	noise_type;
	double	octaves;
	double	lacunarity;
};
#define CK_air_SP(_p) RT_CKMAG(_p, air_MAGIC, "air_specific")

static struct air_specific air_defaults = {
	air_MAGIC,
	.1,		/* d_p_m */	
	.01,		/* scale */
	0.0,		/* delta */
	{ .5, .5, .625 }/* color */
	};

#define SHDR_NULL	((struct air_specific *)0)
#define SHDR_O(m)	offsetof(struct air_specific, m)
#define SHDR_AO(m)	offsetofarray(struct air_specific, m)

struct structparse air_parse[] = {
	{"%f",  1, "dpm",		SHDR_O(d_p_m),		FUNC_NULL },
	{"%f",  1, "scale",		SHDR_O(scale),		FUNC_NULL },
	{"%f",  1, "s",			SHDR_O(scale),		FUNC_NULL },
	{"%f",  1, "delta",		SHDR_O(delta),		FUNC_NULL },
	{"%f",  1, "d",			SHDR_O(delta),		FUNC_NULL },
	{"",	0, (char *)0,		0,			FUNC_NULL }
};

HIDDEN int	air_setup(), airtest_render(), air_render(), emist_render();
HIDDEN void	air_print(), air_free();

struct mfuncs air_mfuncs[] = {
	{"airtest",	0,		0,		MFI_HIT,	0,
	air_setup,	airtest_render,	air_print,	air_free },

	{"air",		0,		0,		MFI_HIT, MFF_PROC,
	air_setup,	air_render,	air_print,	air_free },

	{"fog",		0,		0,		MFI_HIT, MFF_PROC,
	air_setup,	air_render,	air_print,	air_free },

	{"emist",	0,		0,		MFI_HIT, MFF_PROC,
	air_setup,	emist_render,	air_print,	air_free },

	{(char *)0,	0,		0,		0,		0,
	0,		0,		0,		0 }
};


/*	A I R _ S E T U P
 *
 *	This routine is called (at prep time)
 *	once for each region which uses this shader.
 *	Any shader-specific initialization should be done here.
 */
HIDDEN int
air_setup( rp, matparm, dpp, mfp, rtip)
register struct region	*rp;
struct rt_vls		*matparm;
char			**dpp;	/* pointer to reg_udata in *rp */
struct mfuncs		*mfp;
struct rt_i		*rtip;	/* New since 4.4 release */
{
	register struct air_specific	*air_sp;
	mat_t	tmp;

	if( rdebug&RDEBUG_SHADE) rt_log("air_setup\n");

	RT_CHECK_RTI(rtip);
	RT_VLS_CHECK( matparm );
	RT_CK_REGION(rp);
	GETSTRUCT( air_sp, air_specific );
	*dpp = (char *)air_sp;

	memcpy(air_sp, &air_defaults, sizeof(struct air_specific) );

#if 0
	rp->reg_regionid = -1;
	rp->reg_aircode = 1;
#endif

	if (rdebug&RDEBUG_SHADE) rt_log("\"%s\"\n", RT_VLS_ADDR(matparm) );
	if( rt_structparse( matparm, air_parse, (char *)air_sp ) < 0 )
		return(-1);

	return(1);
}

/*
 *	A I R _ P R I N T
 */
HIDDEN void
air_print( rp, dp )
register struct region *rp;
char	*dp;
{
	rt_structprint( rp->reg_name, air_parse, (char *)dp );
}

/*
 *	A I R _ F R E E
 */
HIDDEN void
air_free( cp )
char *cp;
{
	if( rdebug&RDEBUG_SHADE)
		rt_log("air_free(%s:%d)\n", __FILE__, __LINE__);
	rt_free( cp, "air_specific" );
}

/*
 *	A I R T E S T _ R E N D E R
 *
 *	This is called (from viewshade() in shade.c)
 *	once for each hit point to be shaded.
 *
 *
 */
int
airtest_render( ap, pp, swp, dp )
struct application	*ap;
struct partition	*pp;
struct shadework	*swp;
char	*dp;
{
	register struct air_specific *air_sp =
		(struct air_specific *)dp;
	point_t pt;

	RT_AP_CHECK(ap);
	RT_CHECK_PT(pp);
	CK_air_SP(air_sp);

	if( rdebug&RDEBUG_SHADE) {
		rt_structprint( "air_specific", air_parse, (char *)air_sp );

		rt_log("air in(%g) out%g)\n",
			pp->pt_inhit->hit_dist,
			pp->pt_outhit->hit_dist);
	}

	return(1);
}
/*
 *	A I R _ R E N D E R
 *
 *	This is called (from viewshade() in shade.c)
 *	once for each hit point to be shaded.
 *
 *
 *	This implements Beer's law homogeneous Fog/haze
 *
 *	Tau = optical path depth = density_per_unit_distance * distance
 *
 *	transmission = e^(-Tau)
 */
int
air_render( ap, pp, swp, dp )
struct application	*ap;
struct partition	*pp;
struct shadework	*swp;
char	*dp;
{
	register struct air_specific *air_sp =
		(struct air_specific *)dp;
	point_t pt;
	double tau;
	double dist;

	RT_AP_CHECK(ap);
	RT_CHECK_PT(pp);
	CK_air_SP(air_sp);

	if( rdebug&RDEBUG_SHADE) {
		rt_structprint( "air_specific", air_parse, (char *)air_sp );
		rt_log("air in(%g) out(%g) r_pt(%g %g %g)\n",
			pp->pt_inhit->hit_dist,
			pp->pt_outhit->hit_dist,
			V3ARGS(ap->a_ray.r_pt));
	}

	/* XXX
	 * We can't trust the reflect/refract support in rt to do the right
	 * thing so we have to take over here.
	 */


	/* Beer's Law Homogeneous Fog */

	/* get the path length right */
	if (pp->pt_inhit->hit_dist < 0.0)
		dist = pp->pt_outhit->hit_dist;
	else
		dist = (pp->pt_outhit->hit_dist - pp->pt_inhit->hit_dist);

	/* tau = optical path depth = density per mm * distance */
	tau = (air_sp->d_p_m*0.001 ) * dist;

	/* transmission = e^(-tau) */
	swp->sw_transmit = exp(-tau);

	if ( swp->sw_transmit > 1.0) swp->sw_transmit = 1.0;
	else if ( swp->sw_transmit < 0.0 ) swp->sw_transmit = 0.0;

	/* extinction = 1. - transmission.  Extinguished part replaced by
	 * the "color of the air".
	 */
#if 0
	VMOVE(swp->sw_color, air_sp->color);
	VMOVE(swp->sw_basecolor, air_sp->color);
#endif
	if( rdebug&RDEBUG_SHADE)
		rt_log("air o dist:%gmm tau:%g transmit:%g color(%g %g %g)\n",
			dist, tau, swp->sw_transmit, V3ARGS(swp->sw_color) );

	return(1);
}



/*
 *	E M I S T _ R E N D E R
 *
 *
 *
 *
 * te = dist from pt to end of ray (out hit point)
 * Zo = elevation at ray start
 * Ze = elevation at ray end
 * Zd = Z component of normalized ray vector 
 * d_p_m = overall fog density
 * B = density falloff with altitude
 *
 *
 *	delta = height at which fog starts
 *	scale = stretches exponential decay zone
 *	d_p_m = maximum density @ ground
 *
 *	This is called (from viewshade() in shade.c)
 *	once for each hit point to be shaded.
 */
int
emist_render( ap, pp, swp, dp )
struct application	*ap;
struct partition	*pp;
struct shadework	*swp;
char	*dp;
{
	register struct air_specific *air_sp =
		(struct air_specific *)dp;
	point_t in_pt, out_pt;
	double tau;
	double Zo, Ze, Zd, te;

	RT_AP_CHECK(ap);
	RT_CHECK_PT(pp);
	CK_air_SP(air_sp);

	/* compute hit point & length of ray */
	if (pp->pt_inhit->hit_dist < 0.0) {
		VMOVE(in_pt, ap->a_ray.r_pt);
		te = pp->pt_outhit->hit_dist;
	} else {
		VJOIN1(in_pt, ap->a_ray.r_pt,
			pp->pt_inhit->hit_dist, ap->a_ray.r_dir);
		te = pp->pt_outhit->hit_dist - pp->pt_inhit->hit_dist;
	}

	/* get exit point */
	VJOIN1(out_pt,
		ap->a_ray.r_pt, pp->pt_outhit->hit_dist, ap->a_ray.r_dir);

	Zo = (air_sp->delta + in_pt[Z]) * air_sp->scale;
	Ze = (air_sp->delta + out_pt[Z]) * air_sp->scale;
	Zd = ap->a_ray.r_dir[Z];

	if ( NEAR_ZERO( Zd, SQRT_SMALL_FASTF ) )
		tau = (air_sp->d_p_m*0.001) * te * exp( -Zo);
	else
		tau = (( (air_sp->d_p_m*0.001) * te) /  Zd) * ( exp(-Zo) - exp(-Ze) );

/*	XXX future
	tau *= noise_fbm(pt);
*/

	swp->sw_transmit = exp(-tau);

	if (swp->sw_transmit > 1.0) swp->sw_transmit = 1.0;
	else if (swp->sw_transmit < 0.0) swp->sw_transmit = 0.0;

	if( rdebug&RDEBUG_SHADE)
		rt_log("emist transmit = %g\n", swp->sw_transmit);

	return(1);
}
/*
 *	F B M _ E M I S T _ R E N D E R
 *
 *
 *
 *
 * te = dist from pt to end of ray (out hit point)
 * Zo = elevation at ray start
 * Ze = elevation at ray end
 * Zd = Z component of normalized ray vector 
 * d_p_m = overall fog density
 * B = density falloff with altitude
 *
 *
 *	delta = height at which fog starts
 *	scale = stretches exponential decay zone
 *	d_p_m = maximum density @ ground
 *
 *	This is called (from viewshade() in shade.c)
 *	once for each hit point to be shaded.
 */
int
emist_fbm_render( ap, pp, swp, dp )
struct application	*ap;
struct partition	*pp;
struct shadework	*swp;
char	*dp;
{
	register struct air_specific *air_sp =
		(struct air_specific *)dp;
	point_t in_pt, out_pt, curr_pt;
	vect_t dist_v;
	double dist, delta;
	double tau;
	double Zo, Ze, Zd, te;

	RT_AP_CHECK(ap);
	RT_CHECK_PT(pp);
	CK_air_SP(air_sp);

	/* compute hit point & length of ray */
	if (pp->pt_inhit->hit_dist < 0.0) {
		VMOVE(in_pt, ap->a_ray.r_pt);
		te = pp->pt_outhit->hit_dist;
	} else {
		VJOIN1(in_pt, ap->a_ray.r_pt,
			pp->pt_inhit->hit_dist, ap->a_ray.r_dir);
		te = pp->pt_outhit->hit_dist - pp->pt_inhit->hit_dist;
	}

	/* get exit point */
	VJOIN1(out_pt,
		ap->a_ray.r_pt, pp->pt_outhit->hit_dist, ap->a_ray.r_dir);


	/* we march along the ray, evaluating the atmospheric function
	 * at each step.
	 */

	VSUB2(dist_v, out_pt, in_pt);
	dist = MAGNITUDE(dist_v);

	for (delta=0 ; delta < dist ; delta += 1.0 ) {
		/* compute the current point in space */
		VJOIN1(curr_pt, in_pt, delta, ap->a_ray.r_dir);

		/* Shoot a ray down the -Z axis to find our current height 
		 * above the local terrain.
		 */

	}



	return(1);
}
