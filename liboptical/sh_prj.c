/*
 *	S H _ P R J . C
 *
 *	Projection shader
 *
 *	The one parameter to this shader is a filename.  The named file
 *	contains the REAL parameters to the shader.  The v4 database format
 *	is far too anemic to support this sort of shader.
 *
 */
#include "conf.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "raytrace.h"
#include "shadefuncs.h"
#include "shadework.h"
#include "../rt/mathtab.h"
#include "../rt/rdebug.h"

#define prj_MAGIC 0x70726a00	/* "prj" */
#define CK_prj_SP(_p) RT_CKMAG(_p, prj_MAGIC, "prj_specific")

struct img_specific {
	struct bu_list	l;
	unsigned long	junk;
	struct bu_vls	i_file;
	struct bu_mapped_file *i_data;
	unsigned char 	*i_img;
	int		i_width;
	int		i_height;
	fastf_t		i_viewsize;
	point_t		i_eye_pt;
	quat_t		i_orient;
	mat_t		i_mat;		/* computed from i_orient */
	mat_t		i_mat_inv;	/* computed (for debug) */
	plane_t		i_plane;	/* dir/plane of projection */
	mat_t		i_sh_to_img;	/* transform used in prj_render() */
};
#define img_MAGIC	0x696d6700	/* "img" */

/*
 * the shader specific structure contains all variables which are unique
 * to any particular use of the shader.
 */
struct prj_specific {
	unsigned long		magic;
	struct img_specific	prj_images;
	mat_t			prj_m_to_sh;
	FILE			*prj_plfd;
};
#if 0
static void 
noop_hook( sdp, name, base, value )
register CONST struct bu_structparse	*sdp;	/* structure description */
register CONST char			*name;	/* struct member name */
char					*base;	/* begining of structure */
CONST char				*value;	/* string containing value */
{
	struct img_specific *img_sp = (struct img_specific *)base;

	BU_CK_LIST_HEAD(&img_sp->l);

	bu_log("%s \"%s\"\n", sdp->sp_name, value);

	BU_CK_VLS(&img_sp->i_file);
}
#endif
/* 
 * This routine is responsible for duplicating the image list head to make
 * a new list element.  It also reads in the pixel data for the image, and
 * computes the matrix from the view quaternion.  
 *
 * XXX "orient" MUST ALWAYS BE THE LAST PARAMETER SPECIFIED FOR EACH IMAGE.
 */
static void 
orient_hook( sdp, name, base, value )
register CONST struct bu_structparse	*sdp;	/* structure description */
register CONST char			*name;	/* struct member name */
char					*base;	/* begining of structure */
CONST char				*value;	/* string containing value */
{
	struct prj_specific	*prj_sp;
	struct img_specific	*img_sp = (struct img_specific *)base;
	struct img_specific	*img_new;
	FILE			*fd;
	char			*img_file;
	unsigned		buf_size;
	size_t			n;
	mat_t			trans, scale, tmp, xform;
	vect_t			v_tmp;
	point_t			p_tmp;
	unsigned long		*ulp;


	BU_CK_LIST_HEAD(&img_sp->l);

	/* create a new img_specific struct,
	 * copy the parameters from the "head" into the new
	 * img_specific struct
	 */
	GETSTRUCT(img_new, img_specific);
	memcpy(img_new, img_sp, sizeof(struct img_specific));
	BU_CK_VLS(&img_sp->i_file);


	/* zero the filename for the next iteration */
	bu_vls_init(&img_sp->i_file);

	/* Generate matrix from the quaternion */
	quat_quat2mat(img_new->i_mat, img_new->i_orient);



	/* compute matrix to transform region coordinates into 
	 * shader projection coordinates:
	 *
	 *	prj_coord = scale * rot * translate * region_coord
	 */
	bn_mat_idn(trans);
	MAT_DELTAS_VEC_NEG(trans, img_new->i_eye_pt);

	bn_mat_idn(scale);
	MAT_SCALE_ALL(scale, img_new->i_viewsize);

	mat_mul(tmp, img_new->i_mat, trans);
	mat_mul(img_new->i_sh_to_img, scale, tmp);



	if( rdebug&RDEBUG_SHADE) {
		point_t pt;		

		VSET(v_tmp, 0.0, 0.0, 1.0);

		/* compute inverse */
		mat_inv(img_new->i_mat_inv, img_new->i_mat);
		mat_inv(xform, img_new->i_sh_to_img);

		MAT4X3VEC(img_new->i_plane, xform, v_tmp);
#if 0
		VUNITIZE(img_new->i_plane);
		img_new->i_plane[H] = 
			VDOT(img_new->i_plane, img_new->i_eye_pt);
#endif
		prj_sp = (struct prj_specific *)
			(base - (offsetof(struct prj_specific, prj_images)));
		CK_prj_SP(prj_sp);

		if (!prj_sp->prj_plfd)
			bu_bomb("prj shader prj_plfd should be open\n");

		/* plot out the extent of the image frame */
		pl_color(prj_sp->prj_plfd, 255, 0, 0);

		VSET(v_tmp, -0.5, -0.5, 0.0);
		MAT4X3PNT(pt, xform, v_tmp);
		pdv_3move(prj_sp->prj_plfd, pt);

		VSET(v_tmp, 0.5, -0.5, 0.0);
		MAT4X3PNT(p_tmp, xform, v_tmp);
		pdv_3cont(prj_sp->prj_plfd, p_tmp);

		VSET(v_tmp, 0.5, 0.5, 0.0);
		MAT4X3PNT(p_tmp, xform, v_tmp);
		pdv_3cont(prj_sp->prj_plfd, p_tmp);

		VSET(v_tmp, -0.5, 0.5, 0.0);
		MAT4X3PNT(p_tmp, xform, v_tmp);
		pdv_3cont(prj_sp->prj_plfd, p_tmp);

		pdv_3cont(prj_sp->prj_plfd, pt);

		VSET(v_tmp, 0.0, 0.0, 0.0);
		MAT4X3PNT(p_tmp, xform, v_tmp);
		pdv_3move(prj_sp->prj_plfd, p_tmp);
		VREVERSE(pt, img_new->i_plane);
		VADD2(p_tmp, p_tmp, pt);
		pdv_3cont(prj_sp->prj_plfd, p_tmp);

	}








	/* read in the pixel data */
	img_new->i_data = bu_open_mapped_file(bu_vls_addr(&img_new->i_file),
				(char *)NULL);
	if ( ! img_new->i_data)
		bu_bomb("shader prj: orient_hook() can't get pixel data... bombing\n");

	img_new->i_img = (unsigned char *)img_new->i_data->buf;

	BU_LIST_MAGIC_SET(&img_new->l, img_MAGIC);
	BU_LIST_APPEND(&img_sp->l, &img_new->l);
}

#define IMG_O(m)	offsetof(struct img_specific, m)
#define IMG_AO(m)	offsetofarray(struct img_specific, m)

/* description of how to parse/print the arguments to the shader
 * There is at least one line here for each variable in the shader specific
 * structure above
 */
struct bu_structparse img_parse_tab[] = {
	{"%S",	1, "image",		IMG_O(i_file),		FUNC_NULL},
	{"%S",	1, "end",		IMG_O(i_file),		FUNC_NULL},
	{"%d",	1, "w",			IMG_O(i_width),		FUNC_NULL},
	{"%d",	1, "n",			IMG_O(i_height),	FUNC_NULL},
	{"%f",	1, "viewsize",		IMG_O(i_viewsize),	FUNC_NULL},
	{"%f",	3, "eye_pt",		IMG_AO(i_eye_pt),	FUNC_NULL},
	{"%f",	4, "orientation",	IMG_AO(i_orient),	orient_hook},
	{"",	0, (char *)0,		0,			FUNC_NULL}
};
struct bu_structparse img_print_tab[] = {
	{"i",	bu_byteoffset(img_parse_tab[0]), "img_parse_tab", 0, FUNC_NULL },
	{"%f",	4, "i_plane",		IMG_AO(i_plane),	FUNC_NULL},
	{"",	0, (char *)0,		0,			FUNC_NULL}
};



HIDDEN int	prj_setup(), prj_render();
HIDDEN void	prj_print(), prj_free();

/* The "mfuncs" structure defines the external interface to the shader.
 * Note that more than one shader "name" can be associated with a given
 * shader by defining more than one mfuncs struct in this array.
 * See sh_phong.c for an example of building more than one shader "name"
 * from a set of source functions.  There you will find that "glass" "mirror"
 * and "plastic" are all names for the same shader with different default
 * values for the parameters.
 */
struct mfuncs prj_mfuncs[] = {
	{MF_MAGIC,	"prj",		0,		MFI_NORMAL|MFI_HIT|MFI_UV,	0,
	prj_setup,	prj_render,	prj_print,	prj_free },

	{0,		(char *)0,	0,		0,		0,
	0,		0,		0,		0 }
};


/*	P R J _ S E T U P
 *
 *	This routine is called (at prep time)
 *	once for each region which uses this shader.
 *	Any shader-specific initialization should be done here.
 */
HIDDEN int
prj_setup( rp, matparm, dpp, mfp, rtip)
register struct region	*rp;
struct rt_vls		*matparm;
char			**dpp;	/* pointer to reg_udata in *rp */
struct mfuncs		*mfp;
struct rt_i		*rtip;	/* New since 4.4 release */
{
	register struct prj_specific	*prj_sp;
	char 				*fname;
	struct bu_vls 			parameter_data;
	struct bu_mapped_file		*parameter_file;

	/* check the arguments */
	RT_CHECK_RTI(rtip);
	RT_VLS_CHECK( matparm );
	RT_CK_REGION(rp);

	if( rdebug&RDEBUG_SHADE)
		rt_log("prj_setup(%s) matparm:\"%s\"\n",
			rp->reg_name, bu_vls_addr(matparm));

	/* Get memory for the shader parameters and shader-specific data */
	GETSTRUCT( prj_sp, prj_specific );
	*dpp = (char *)prj_sp;

	prj_sp->magic = prj_MAGIC;
	BU_LIST_INIT(&prj_sp->prj_images.l);


	if( rdebug&RDEBUG_SHADE) {
		if ((prj_sp->prj_plfd=fopen("prj.pl", "w")) == (FILE *)NULL) {
			bu_log("ERROR creating plot file prj.pl");
		}
	} else
		prj_sp->prj_plfd = (FILE *)NULL;


	fname = bu_vls_addr(matparm);
#if 1
	if (! isspace(*fname) )
		bu_log("------ Stack shader fixed?  Remove hack from prj shader ----\n");
	while (isspace(*fname)) fname++; /* XXX Hack till stack shader fixed */

#endif
	if (! *fname)
		bu_bomb("Null prj shaderfile?\n");

	parameter_file = bu_open_mapped_file( fname, (char *)NULL );
	if (! parameter_file)
		bu_bomb("prj_setup can't get shaderfile... bombing\n");

	bu_vls_init(&parameter_data);
	bu_vls_strncpy( &parameter_data, (char *)parameter_file->buf,
		parameter_file->buflen );

	if( rdebug&RDEBUG_SHADE ) {
		bu_log("parsing: %s\n", bu_vls_addr(&parameter_data));
	}

	bu_close_mapped_file( parameter_file );

	if(bu_struct_parse( &parameter_data, img_parse_tab, 
	    (char *)&prj_sp->prj_images) < 0)
		return -1;

	bu_vls_free( &parameter_data );

	/* The shader needs to operate in a coordinate system which stays
	 * fixed on the region when the region is moved (as in animation)
	 * we need to get a matrix to perform the appropriate transform(s).
	 *
	 * db_region_mat returns a matrix which maps points on/in the region
	 * as it exists where the region is defined (as opposed to the 
	 * (possibly transformed) one we are rendering.
	 */
	db_region_mat(prj_sp->prj_m_to_sh, rtip->rti_dbip, rp->reg_name);


	if( rdebug&RDEBUG_SHADE) {

		prj_print(rp, (char *)prj_sp );
	}

	return(1);
}

/*
 *	P R J _ P R I N T
 */
HIDDEN void
prj_print( rp, dp )
register struct region *rp;
char	*dp;
{
	struct prj_specific *prj_sp = (struct prj_specific *)dp;
	struct img_specific *img_sp;

	for (BU_LIST_FOR(img_sp, img_specific, &prj_sp->prj_images.l)) {
		bu_struct_print( rp->reg_name, img_print_tab, (char *)img_sp );
	}
}

/*
 *	P R J _ F R E E
 */
HIDDEN void
prj_free( cp )
char *cp;
{
	struct prj_specific *prj_sp = (struct prj_specific *)cp;

	struct img_specific *img_sp;

	while (BU_LIST_WHILE(img_sp, img_specific, &prj_sp->prj_images.l)) {

		img_sp->i_img = (unsigned char *)0;
		bu_close_mapped_file( img_sp->i_data );
		bu_vls_vlsfree(&img_sp->i_file);

		BU_LIST_DEQUEUE( &img_sp->l );
		bu_free( (char *)img_sp, "img_specific");
	}

	if ( prj_sp->prj_plfd ) {
		bu_semaphore_acquire( BU_SEM_SYSCALL );
		fclose( prj_sp->prj_plfd );
		bu_semaphore_release( BU_SEM_SYSCALL );
	}

	bu_free( cp, "prj_specific" );
}

/*
 *	P R J _ R E N D E R
 *
 *	This is called (from viewshade() in shade.c) once for each hit point
 *	to be shaded.  The purpose here is to fill in values in the shadework
 *	structure.
 */
int
prj_render( ap, pp, swp, dp )
struct application	*ap;
struct partition	*pp;
struct shadework	*swp;	/* defined in material.h */
char			*dp;	/* ptr to the shader-specific struct */
{
	register struct prj_specific *prj_sp =
		(struct prj_specific *)dp;
	const static point_t delta = {0.5, 0.5, 0.0};
	point_t r_pt;
	vect_t	r_N;
	point_t sh_pt;
	static CONST double	cs = (1.0/255.0);
	point_t	img_v;
	int x, y;
	unsigned char *pixel;
	struct img_specific *img_sp;
	point_t	sh_color;
	point_t	final_color;
	point_t tmp_pt;
	fastf_t	divisor;


	/* check the validity of the arguments we got */
	RT_AP_CHECK(ap);
	RT_CHECK_PT(pp);
	CK_prj_SP(prj_sp);

	if( rdebug&RDEBUG_SHADE) {
		bu_log("shading with prj\n");
		prj_print(pp->pt_regionp, dp);
	}
	/* If we are performing the shading in "region" space, we must 
	 * transform the hit point from "model" space to "region" space.
	 * See the call to db_region_mat in prj_setup().
	 */
	MAT4X3PNT(r_pt, prj_sp->prj_m_to_sh, swp->sw_hit.hit_point);
	MAT4X3VEC(r_N, prj_sp->prj_m_to_sh, swp->sw_hit.hit_normal);

	


	if( rdebug&RDEBUG_SHADE) {
		rt_log("prj_render()  model:(%g %g %g) shader:(%g %g %g)\n", 
		V3ARGS(swp->sw_hit.hit_point),
		V3ARGS(r_pt) );
	}


	VSET(final_color, 0.0, 0.0, 0.0);
	divisor = 0.0;
	for (BU_LIST_FOR(img_sp, img_specific, &prj_sp->prj_images.l)) {
		if (VDOT(r_N, img_sp->i_plane) < 0.0) {
			/* normal and projection dir don't match, skip on */

			if( rdebug&RDEBUG_SHADE && prj_sp->prj_plfd) {
				/* plot hit normal */
				pl_color(prj_sp->prj_plfd, 255, 255, 255);
				pdv_3move(prj_sp->prj_plfd, r_pt);
				VADD2(tmp_pt, r_pt, r_N);
				pdv_3cont(prj_sp->prj_plfd, tmp_pt);

				/* plot projection direction */
				pl_color(prj_sp->prj_plfd, 255, 255, 0);
				pdv_3move(prj_sp->prj_plfd, r_pt);
				VADD2(tmp_pt, r_pt, img_sp->i_plane);
				pdv_3cont(prj_sp->prj_plfd, tmp_pt);
			}
			continue;
		}
		
		MAT4X3PNT(sh_pt, img_sp->i_sh_to_img, r_pt);
		VADD2(img_v, sh_pt, delta);
		if( rdebug&RDEBUG_SHADE) {
			VPRINT("sh_pt", sh_pt);
			VPRINT("img_v", img_v);
		}
		x = img_v[X] * (img_sp->i_width-1);
		y = img_v[Y] * (img_sp->i_height-1);
		pixel = &img_sp->i_img[x*3 + y*img_sp->i_width*3];


		if (x >= img_sp->i_width || x < 0 ||
		    y >= img_sp->i_height || y < 0 ||
		    sh_pt[Z] > 0.0) {
		    	/* for some reason we're out of bounds */
#if 0
			if( rdebug&RDEBUG_SHADE && prj_sp->prj_plfd) {
				/* plot projection direction */
				int colour[3];
				VSCALE(colour, swp->sw_color, 255.0);

				pl_color(prj_sp->prj_plfd, V3ARGS(colour));

				pdv_3move(prj_sp->prj_plfd, r_pt);
				VSCALE(tmp_pt, img_sp->i_plane, -sh_pt[Z]);
				VADD2(tmp_pt, r_pt, tmp_pt);
				pdv_3cont(prj_sp->prj_plfd, tmp_pt);
			}
#endif
			continue;
		}

		if( rdebug&RDEBUG_SHADE && prj_sp->prj_plfd) {
			/* plot projection direction */
			pl_color(prj_sp->prj_plfd, V3ARGS(pixel));
			pdv_3move(prj_sp->prj_plfd, r_pt);
			VMOVE(tmp_pt, r_pt);

			VSCALE(tmp_pt, img_sp->i_plane, -sh_pt[Z]);
			VADD2(tmp_pt, r_pt, tmp_pt);
			pdv_3cont(prj_sp->prj_plfd, tmp_pt);
		}

		VMOVE(sh_color, pixel);	/* int/float conversion */
		VSCALE(sh_color, sh_color, cs);
		VADD2(final_color, final_color, sh_color);
		divisor++;
	}
	if (divisor > 0.0) {
		divisor = 1.0 / divisor;
		VSCALE(swp->sw_color, final_color, divisor);
	} 
	return 1;
}
