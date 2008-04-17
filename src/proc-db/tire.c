/*                          T I R E . C
 * BRL-CAD
 *
 * Copyright (c) 2008 United States Government as represented by
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
/** @file tire.c
 *
 * Tire Generator
 *
 * Program to create basic tire shapes.
 */

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "raytrace.h"
#include "wdb.h"

#define D2R(x) (x * DEG2RAD)
#define R2D(x) (x / DEG2RAD)

#define ROWS 5
#define COLS 5
#define F_PI 3.1415926535897932384626433832795029L 


void getYRotMat(mat_t (*t), fastf_t theta)
{
    fastf_t sin_ = sin(theta);
    fastf_t cos_ = cos(theta);
    mat_t r;
    MAT_ZERO(r);
    r[0] = cos_;
    r[2] = sin_;
    r[5] = 1;
    r[8] = -sin_;
    r[10] = cos_;
    r[15] = 1;
    memcpy(*t, r, sizeof(*t));
}

void printVec(fastf_t *result1, int c, char *strname)
{
    int i=0;
    bu_log("\n\n----%s------\n", strname);
    for (i = 0; i < c; i++) {
	bu_log("%.16f ", result1[i]);
    }
    bu_log("\n-------------------------\n\n");
}

/* Evaluate Partial Derivative of Ellipse Equation at a point */
fastf_t GetEllPartialAtPoint(fastf_t *inarray, fastf_t x, fastf_t y)
{
    fastf_t A,B,C,D,E,F,partial;
    A = inarray[0];
    B = inarray[1];
    C = inarray[2];
    D = inarray[3];
    E = inarray[4];
    F = -1;
    partial = -(D+y*B+2*x*A)/(E+2*y*C+x*B);
    return partial;
}

/* Evaluate z value of Ellipse Equation at a point */
fastf_t GetValueAtZPoint(fastf_t *inarray, fastf_t y)
{
    fastf_t A,B,C,D,E,F,z;
    A = inarray[0];
    B = inarray[1];
    C = inarray[2];
    D = inarray[3];
    E = inarray[4];
    F = 1;
    z = (sqrt(-4*A*F-4*y*A*E+D*D+2*y*B*D-4*y*y*A*C+y*y*B*B)-D-y*B)/(2*A); 
    return z;
}

/* Create General Conic Matrix for Ellipse describing tire slick surface */
 void Create_Ell1_Mat(fastf_t **mat, fastf_t dytred, fastf_t dztred, fastf_t d1, fastf_t ztire) 
{
    fastf_t y1,z1,y2,z2,y3,z3,y4,z4,y5,z5;

    y1 = dytred/2;
    z1 = ztire-dztred;
    y2 = dytred/2;
    z2 = ztire - (dztred+2*(d1-dztred));
    y3 = 0.0;
    z3 = ztire;
    y4 = 0.0;
    z4 = ztire-2*d1;
    y5 = -dytred/2;
    z5 = ztire-dztred;

    mat[0][0] = y1*y1;
    mat[0][1] = y1*z1;
    mat[0][2] = z1*z1;
    mat[0][3] = y1;
    mat[0][4] = z1;
    mat[0][5] = -1;
    mat[1][0] = y2*y2;
    mat[1][1] = y2*z2;
    mat[1][2] = z2*z2;
    mat[1][3] = y2;
    mat[1][4] = z2;
    mat[1][5] = -1;
    mat[2][0] = y3*y3;
    mat[2][1] = y3*z3;
    mat[2][2] = z3*z3;
    mat[2][3] = y3;
    mat[2][4] = z3;
    mat[2][5] = -1;
    mat[3][0] = y4*y4;
    mat[3][1] = y4*z4;
    mat[3][2] = z4*z4;
    mat[3][3] = y4;
    mat[3][4] = z4;
    mat[3][5] = -1;
    mat[4][0] = y5*y5;
    mat[4][1] = y5*z5;
    mat[4][2] = z5*z5;
    mat[4][3] = y5;
    mat[4][4] = z5;
    mat[4][5] = -1;
}
 
/* Create General Conic Matrix for Ellipse describing the tire side */
void Create_Ell2_Mat(fastf_t **mat, fastf_t dytred, fastf_t dztred, fastf_t dyside1, fastf_t zside1, fastf_t ztire, fastf_t dyhub, fastf_t zhub, fastf_t ell1partial) 
{
    mat[0][0] = (dyside1 / 2) * (dyside1 / 2);
    mat[0][1] = (zside1 * dyside1 / 2);
    mat[0][2] = zside1 * zside1;
    mat[0][3] = dyside1 / 2;
    mat[0][4] = zside1;
    mat[0][5] = -1;
    mat[1][0] = (dytred / 2) * (dytred / 2);
    mat[1][1] = (ztire - dztred) * (dytred / 2);
    mat[1][2] = (ztire - dztred) * (ztire - dztred);
    mat[1][3] = (dytred / 2);
    mat[1][4] = (ztire - dztred);
    mat[1][5] = -1;
    mat[2][0] = (dyhub/2) * (dyhub/2);
    mat[2][1] = (dyhub/2) * zhub;
    mat[2][2] = zhub * zhub;
    mat[2][3] = dyhub/2;
    mat[2][4] = zhub;
    mat[2][5] = -1;
    mat[3][0] = 2 * (dytred / 2);
    mat[3][1] = (ztire - dztred) + (dytred / 2) * ell1partial;
    mat[3][2] = 2*(ztire - dztred) * ell1partial;
    mat[3][3] = 1;
    mat[3][4] = ell1partial;
    mat[3][5] = 0;
    mat[4][0] = 2 * (dyhub / 2);
    mat[4][1] = zhub + (dyhub / 2) * -ell1partial;
    mat[4][2] = 2*zhub*-ell1partial;
    mat[4][3] = 1;
    mat[4][4] = -ell1partial;
    mat[4][5] = 0;
}


/* Sort Rows of 5x6 matrix - for use in Gaussian Elimination */
void SortRows(fastf_t **mat, int colnum)
{
    int high_row, exam_row;
    int colcount;
    fastf_t temp_elem;
    high_row = colnum;
    for (exam_row = high_row + 1; exam_row < 5; exam_row++) {
	if (fabs(mat[exam_row][colnum]) > fabs(mat[high_row][colnum]))
	    high_row = exam_row;
    }
    if (high_row != colnum) {
	for(colcount = colnum; colcount < 6; colcount++){
	    temp_elem = mat[colnum][colcount];
	    mat[colnum][colcount] = mat[high_row][colcount];
	    mat[high_row][colcount] = temp_elem;
	}
    }
}

/* Convert 5x6 matrix to Reduced Echelon Form */
void Echelon(fastf_t **mat)
{
    int i,j,k;
    fastf_t pivot,rowmult;
    for(i = 0; i < 5; i++){
	SortRows(mat,i);
	pivot = mat[i][i];
	for (j = i; j < 6; j++){
	    mat[i][j] = mat[i][j] / pivot;}
	for (k = i + 1; k < 5 ; k++){
	    rowmult=mat[k][i];
	    for (j = i; j < 6; j++){
		mat[k][j] -= rowmult*mat[i][j];
	    }
	}
     }
}
	   
/* Take Reduced Echelon form of Matrix and solve for 
 * General Conic Coefficients
 */ 
void SolveEchelon(fastf_t **mat, fastf_t *result1)
{
    int i,j;
    fastf_t inter;
    for(i = 4; i >= 0; i--){
	inter = mat[i][5];
	for(j = 4; j > i; j--){
	    inter -= mat[i][j]*result1[j];
	}
	result1[i]=inter;
    }
}

/* Using the coefficients of the General Conic Equation
 * solution, calculate input values required by the
 * BRL-CAD mk_eto command.
 */
void CalcInputVals(fastf_t *inarray, fastf_t *outarray, int orientation)
{
    fastf_t A,B,C,D,E,Fp;
    fastf_t App, Bpp,Cpp;
    fastf_t x0,y0;
    fastf_t theta;
    fastf_t length1, length2;
    fastf_t semimajor, semiminor;
    fastf_t semimajorx,semimajory;

    A = inarray[0];
    B = inarray[1];
    C = inarray[2];
    D = inarray[3];
    E = inarray[4];

    /* Translation to Center of Ellipse */
    x0 = -(B*E-2*C*D)/(4*A*C-B*B);
    y0 = -(B*D-2*A*E)/(4*A*C-B*B);

    /* Translate to the Origin for Rotation */
    Fp = 1-y0*E-x0*D+y0*y0*C+x0*y0*B+x0*x0*A;

    /* Rotation Angle */
    theta = .5*atan(1000*B/(1000*A-1000*C));
    
    /* Calculate A'', B'' and C'' - B'' is zero with above theta choice */
    App = A*cos(theta)*cos(theta)+B*cos(theta)*sin(theta)+C*sin(theta)*sin(theta);
    Bpp = 2*(C-A)*cos(theta)*sin(theta)+B*cos(theta)*cos(theta)-B*sin(theta)*sin(theta);
    Cpp = A*sin(theta)*sin(theta)-B*sin(theta)*cos(theta)+C*cos(theta)*cos(theta);

    /* Solve for semimajor and semiminor lengths*/
    length1 = sqrt(-Fp/App);
    length2 = sqrt(-Fp/Cpp);

    /* BRL-CAD's eto primitive requires that C is the semimajor */
    if (length1 > length2) {
	semimajor = length1;
	semiminor = length2;
    } else {
	semimajor = length2;
	semiminor = length1;
    }

    /* Based on orientation of Ellipse, largest component of C is either in the
     * y direction (0) or z direction (1) - find the components.
     */

    if (orientation == 0){
	semimajorx = semimajor*cos(-theta);
	semimajory = semimajor*sin(-theta);
    }else{
	semimajorx = semimajor*sin(-theta);
	semimajory = semimajor*cos(-theta);
    }

    /* Return final BRL-CAD input parameters */
    outarray[0] = -x0;
    outarray[1] = -y0;
    outarray[2] = semimajorx;
    outarray[3] = semimajory;
    outarray[4] = semiminor;
}
/* Function to create a wheel rim to go with a particular tire. */

void MakeWheelRims(struct rt_wdb (*file), char *suffix, fastf_t dyhub, fastf_t zhub, int bolts, fastf_t bolt_diam, fastf_t bolt_circ_diam, fastf_t spigot_diam, fastf_t fixing_offset, fastf_t bead_height, fastf_t bead_width, fastf_t rim_thickness)
{
    struct wmember tireleftbead, tirerightbead, tireleftflat, tirerightflat, tirefixingface, hubhole,hubholes;
    struct wmember tirebeadlefttrans, tirebeadrighttrans, wheelrim, innerhub, wheel;
    fastf_t inner_width, left_width, right_width, fixing_width, inner_width_left_start, inner_width_right_start;
    fastf_t fixing_width_left_trans, fixing_width_right_trans, fixing_width_middle;
    fastf_t fixing_start_left, fixing_start_right, fixing_start_middle;
    struct wmember bolthole,boltholes;
    int i;
    mat_t y;
    vect_t normal, height;
    point_t origin, vertex, C;
    struct bu_vls str;
    bu_vls_init(&str);
    
    /* Insert primitives */
    VSET(vertex, 0, -dyhub/2, 0);
    VSET(height, 0, bead_width, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Bead%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, zhub);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Bead-Cut%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, zhub - rim_thickness);
    VSET(vertex, 0, dyhub/2, 0);
    VSET(height, 0, -bead_width, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Bead%s.s", suffix); 
    mk_rcc(file, bu_vls_addr(&str), vertex, height, zhub);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Bead-Cut%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, zhub - rim_thickness);
    VSET(vertex, 0, -dyhub/2+bead_width, 0);
    VSET(normal, 0, 1, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Bead-Trans%s.s", suffix);	
    mk_cone(file, bu_vls_addr(&str), vertex, normal, bead_width, zhub, zhub-bead_height);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Bead-Trans-cut%s.s", suffix);
    mk_cone(file, bu_vls_addr(&str), vertex, normal, bead_width, zhub-rim_thickness, zhub-bead_height-rim_thickness);
    VSET(vertex, 0, dyhub/2-bead_width, 0);
    VSET(normal, 0, -1, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Bead-Trans%s.s", suffix);	
    mk_cone(file, bu_vls_addr(&str), vertex, normal, bead_width, zhub, zhub-bead_height);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Bead-Trans-cut%s.s", suffix);
    mk_cone(file, bu_vls_addr(&str), vertex, normal, bead_width, zhub-rim_thickness, zhub-bead_height-rim_thickness);
    
    /* At this point, some intermediate values are calculated to ease the bookkeeping */

    inner_width = dyhub-4*bead_width;
    inner_width_left_start = -dyhub/2 + 2*bead_width;
    inner_width_right_start = dyhub/2 - 2*bead_width;
    fixing_width = .25*inner_width;
    fixing_width_left_trans = .25*fixing_width;
    fixing_width_right_trans = .25*fixing_width;
    fixing_width_middle = .5*fixing_width;
    fixing_start_middle = fixing_offset - fixing_width_middle/2;
    fixing_start_left = fixing_start_middle - fixing_width_left_trans;
    fixing_start_right = fixing_offset + fixing_width_middle;
    right_width = inner_width_right_start - fixing_start_left - fixing_width;
    left_width = inner_width_left_start - fixing_start_left;

    VSET(vertex, 0, inner_width_left_start, 0);
    VSET(height, 0, -left_width, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "LeftInnerRim%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, zhub - bead_height);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "LeftInnerRim-cut%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, zhub - rim_thickness - bead_height);
    VSET(vertex, 0, inner_width_right_start, 0);
    VSET(height, 0, -right_width, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "RightInnerRim%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, zhub - bead_height);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "RightInnerRim-cut%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, zhub - rim_thickness - bead_height);
    VSET(vertex, 0, fixing_start_left, 0);
    VSET(normal, 0, 1, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Fixing-Trans%s.s", suffix);	
    mk_cone(file, bu_vls_addr(&str), vertex, normal, fixing_width_left_trans, zhub - bead_height, zhub-2*bead_height);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Fixing-Trans-cut%s.s", suffix);
    mk_cone(file, bu_vls_addr(&str), vertex, normal, fixing_width_left_trans, zhub- bead_height -rim_thickness, zhub-2*bead_height-rim_thickness);
    VSET(vertex, 0, fixing_start_right, 0);
    VSET(normal, 0, -1, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Fixing-Trans%s.s", suffix);	
    mk_cone(file, bu_vls_addr(&str), vertex, normal, fixing_width_right_trans, zhub - bead_height, zhub-2*bead_height);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Fixing-Trans-cut%s.s", suffix);
    mk_cone(file, bu_vls_addr(&str), vertex, normal, fixing_width_right_trans, zhub- bead_height -rim_thickness, zhub-2*bead_height-rim_thickness);
    VSET(vertex, 0, fixing_start_middle, 0);
    VSET(height, 0, fixing_width_middle, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Inner-Fixing%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, zhub - 2*bead_height);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Inner-Fixing-cut%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, zhub - rim_thickness - 2*bead_height);

    VSET(origin, 0, fixing_start_right, 0);
    VSET(normal, 0, -1, 0);
    VSET(C, 0, -fixing_width/2,(zhub-2*bead_height-rim_thickness)/2+rim_thickness*.4)
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Inner-Hub%s.s", suffix);	
    mk_eto(file, bu_vls_addr(&str), origin, normal, C, (zhub-2*bead_height-rim_thickness)/2+rim_thickness*.4, 20);   
   
    VSET(origin, 0,fixing_start_right-10, 0);
    VSET(normal, 0, -1, 0);
    VSET(C, 0, -fixing_width/2,(zhub-2*bead_height-rim_thickness)/2+rim_thickness*.4)
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Inner-Hub-Cut1%s.s", suffix);	
    mk_eto(file, bu_vls_addr(&str), origin, normal, C, (zhub-2*bead_height-rim_thickness)/2+rim_thickness*.4, 20);   

    VSET(origin, 0,0, 0);
    VSET(height, 0, dyhub/2, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Inner-Hub-Cut2%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), origin, height, spigot_diam/2);   

    /* Make the circular pattern of holes in the hub - this involves the creation of
     * one primitive, a series of transformed combinations which use that primitive,
     * and a combination combining all of the previous combinations.  Since it requires
     * both primitives and combinations it is placed between the two sections.
     */

    VSET(vertex, 0, 0, (zhub - (bolt_circ_diam/2+bolt_diam/2))/1.25);
    VSET(height, 0, dyhub/2, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Hub-Hole%s.s", suffix); 
    mk_rcc(file, bu_vls_addr(&str), vertex, height,(zhub - (bolt_circ_diam/2+bolt_diam/2))/6.5 );


    BU_LIST_INIT(&hubhole.l);
    BU_LIST_INIT(&hubholes.l);
    for (i=1; i<=10; i++) {
	bu_vls_trunc(&str,0);
	bu_vls_printf(&str, "Hub-Hole%s.s",suffix);
	getYRotMat(&y,D2R(i*360/10));
	(void)mk_addmember(bu_vls_addr(&str), &hubhole.l, y, WMOP_UNION);
	bu_vls_trunc(&str,0);
	bu_vls_printf(&str, "Hub-Hole-%1.0d%s.c", i, suffix);
	mk_lcomb(file, bu_vls_addr(&str), &hubhole, 0, NULL, NULL, NULL, 0);
	(void)mk_addmember(bu_vls_addr(&str), &hubholes.l, NULL, WMOP_UNION);
	(void)BU_LIST_POP_T(&hubhole.l,struct wmember);
    }
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Hub-Holes%s.c",suffix);
    mk_lcomb(file, bu_vls_addr(&str), &hubholes, 0, NULL, NULL, NULL, 0);

    /* Make the bolt holes in the hub 
     */

    VSET(vertex, 0, 0, bolt_circ_diam/2);
    VSET(height, 0, dyhub/2, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Bolt-Hole%s.s", suffix); 
    mk_rcc(file, bu_vls_addr(&str), vertex, height, bolt_diam/2 );


    BU_LIST_INIT(&bolthole.l);
    BU_LIST_INIT(&boltholes.l);
    for (i=1; i<=bolts; i++) {
	bu_vls_trunc(&str,0);
	bu_vls_printf(&str, "Bolt-Hole%s.s",suffix);
	getYRotMat(&y,D2R(i*360/bolts));
	(void)mk_addmember(bu_vls_addr(&str), &bolthole.l, y, WMOP_UNION);
	bu_vls_trunc(&str,0);
	bu_vls_printf(&str, "Bolt-Hole-%1.0d%s.c", i, suffix);
	mk_lcomb(file, bu_vls_addr(&str), &bolthole, 0, NULL, NULL, NULL, 0);
	(void)mk_addmember(bu_vls_addr(&str), &boltholes.l, NULL, WMOP_UNION);
	(void)BU_LIST_POP_T(&bolthole.l,struct wmember);
    }
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Bolt-Holes%s.c",suffix);
    mk_lcomb(file, bu_vls_addr(&str), &boltholes, 0, NULL, NULL, NULL, 0);

    /*Make combination for Left bead*/ 
    BU_LIST_INIT(&tireleftbead.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Bead%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tireleftbead.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Bead-Cut%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tireleftbead.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Bead%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tireleftbead, 0, NULL, NULL, NULL, 0);

    /*Make combination for Right bead*/ 
    BU_LIST_INIT(&tirerightbead.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Bead%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirerightbead.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Bead-Cut%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirerightbead.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Bead%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tirerightbead, 0, NULL, NULL, NULL, 0);

    /*Make combination for Left bead transition*/ 
    BU_LIST_INIT(&tirebeadlefttrans.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Bead-Trans%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirebeadlefttrans.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Bead-Trans-cut%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirebeadlefttrans.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Bead-Trans%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tirebeadlefttrans, 0, NULL, NULL, NULL, 0);

    /*Make combination for Right bead transition*/ 
    BU_LIST_INIT(&tirebeadrighttrans.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Bead-Trans%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirebeadrighttrans.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Bead-Trans-cut%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirebeadrighttrans.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Bead-Trans%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tirebeadrighttrans, 0, NULL, NULL, NULL, 0);

    /*Make combination for Left Flat*/ 
    BU_LIST_INIT(&tireleftflat.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "LeftInnerRim%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tireleftflat.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "LeftInnerRim-cut%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tireleftflat.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Inner-Rim%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tireleftflat, 0, NULL, NULL, NULL, 0);

    /*Make combination for Right Flat*/ 
    BU_LIST_INIT(&tirerightflat.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "RightInnerRim%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirerightflat.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "RightInnerRim-cut%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirerightflat.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Inner-Rim%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tirerightflat, 0, NULL, NULL, NULL, 0);

    /*Make combination for Fixing */
    BU_LIST_INIT(&tirefixingface.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Fixing-Trans%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirefixingface.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Fixing-Trans-cut%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirefixingface.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Fixing-Trans%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirefixingface.l, NULL, WMOP_UNION);
     bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Fixing-Trans-cut%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirefixingface.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Inner-Fixing%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirefixingface.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Inner-Fixing-cut%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirefixingface.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Fixing%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tirefixingface, 0, NULL, NULL, NULL, 0);

    /*Make wheel rim combination*/
    BU_LIST_INIT(&wheelrim.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Bead%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &wheelrim.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Bead%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &wheelrim.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Bead-Trans%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &wheelrim.l, NULL, WMOP_UNION);
     bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Bead-Trans%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &wheelrim.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Left-Inner-Rim%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &wheelrim.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Right-Inner-Rim%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &wheelrim.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Fixing%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &wheelrim.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Wheel-Rim%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &wheelrim, 0, NULL, NULL, NULL, 0);

    /*Make combination for Inner-Hub*/ 
    BU_LIST_INIT(&innerhub.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Inner-Hub%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &innerhub.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Inner-Hub-Cut1%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &innerhub.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Inner-Hub-Cut2%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &innerhub.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Hub-Holes%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &innerhub.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Bolt-Holes%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &innerhub.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Inner-Hub%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &innerhub, 0, NULL, NULL, NULL, 0);


    /*Other inner hub cuts/bolts/etc here*/




    /*Make combination for Wheel*/ 
    BU_LIST_INIT(&wheel.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Inner-Hub%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &wheel.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Wheel-Rim%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &wheel.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "wheel%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &wheel, 0, NULL, NULL, NULL, 0);
    

}


/* Function which actually defines tire tread pattern.  This is one example - 
 * many tread patterns can be defined in this fashion.
 */

void PatternPoints(fastf_t *pointlist,fastf_t pattern_back, fastf_t pattern_front, fastf_t z_base, fastf_t z_height, \
		   fastf_t ystart, fastf_t negxoffset,fastf_t posxoffset, fastf_t negxwidth, fastf_t posxwidth)
{
    pointlist[0] = pattern_back;
    pointlist[1] = ystart+negxoffset;
    pointlist[2] = z_base;
    pointlist[3] = pattern_front;
    pointlist[4] = ystart+posxoffset;
    pointlist[5] = z_base;
    pointlist[6] = pattern_front;
    pointlist[7] = ystart+posxoffset;
    pointlist[8] = z_height;
    pointlist[9] = pattern_back;
    pointlist[10] = ystart+negxoffset;
    pointlist[11] = z_height;
    pointlist[12] = pattern_back;
    pointlist[13] = ystart+negxoffset+negxwidth;
    pointlist[14] = z_base;
    pointlist[15] = pattern_front;
    pointlist[16] = ystart+posxoffset+posxwidth;
    pointlist[17] = z_base;
    pointlist[18] = pattern_front;
    pointlist[19] = ystart+posxoffset+posxwidth;
    pointlist[20] = z_height;
    pointlist[21] = pattern_back;
    pointlist[22] = ystart+negxoffset+negxwidth;
    pointlist[23] =  z_height;

}

void MakeTreadPattern(struct rt_wdb (*file), char *suffix, fastf_t dwidth, fastf_t z_base, fastf_t ztire)
{
    struct bu_vls str;
    bu_vls_init(&str);
    struct wmember treadpattern, tread, treadrotated;
    fastf_t circumference;
    fastf_t spacing,patternwidth;
    mat_t y;
    int number_of_patterns;
    fastf_t pointlist[24];
    int i;
    unsigned char rgb[3];
    VSET(rgb, 40, 40, 40);

    circumference = F_PI*2*ztire;
    spacing = 0;
    number_of_patterns = 200;
    patternwidth=(circumference-spacing*number_of_patterns)/number_of_patterns/2;
    
    PatternPoints(pointlist,-patternwidth,0,z_base,ztire,-dwidth/2,0,dwidth/20,dwidth/20,dwidth/20);
    mk_arb8(file,"patterncomponent1.s",&pointlist[0]);
    PatternPoints(pointlist,0,patternwidth,z_base,ztire,-dwidth/2,dwidth/20,0,dwidth/20,dwidth/20);
    mk_arb8(file,"patterncomponent11.s",&pointlist[0]);
    
    PatternPoints(pointlist,-patternwidth,0,z_base,ztire,-dwidth/2+dwidth/10,0,dwidth/20,dwidth/20,dwidth/20);
    mk_arb8(file,"patterncomponent2.s",&pointlist[0]);
    PatternPoints(pointlist,0,patternwidth,z_base,ztire,-dwidth/2+dwidth/10,dwidth/20,0,dwidth/20,dwidth/20);
    mk_arb8(file,"patterncomponent12.s",&pointlist[0]);
 
    PatternPoints(pointlist,-patternwidth,0,z_base,ztire,-dwidth/2+2*dwidth/10,0,dwidth/20,dwidth/20,dwidth/20);
    mk_arb8(file,"patterncomponent3.s",&pointlist[0]);
    PatternPoints(pointlist,0,patternwidth,z_base,ztire,-dwidth/2+2*dwidth/10,dwidth/20,0,dwidth/20,dwidth/20);
    mk_arb8(file,"patterncomponent13.s",&pointlist[0]);
    
    PatternPoints(pointlist,-patternwidth,0,z_base,ztire,-dwidth/2+3*dwidth/10,0,dwidth/20,dwidth/20,dwidth/20);
    mk_arb8(file,"patterncomponent4.s",&pointlist[0]);
    PatternPoints(pointlist,0,patternwidth,z_base,ztire,-dwidth/2+3*dwidth/10,dwidth/20,0,dwidth/20,dwidth/20);
    mk_arb8(file,"patterncomponent14.s",&pointlist[0]);
    
    PatternPoints(pointlist,-patternwidth,0,z_base,ztire,-dwidth/2+4*dwidth/10,0,dwidth/20,dwidth/20,dwidth/20);
    mk_arb8(file,"patterncomponent5.s",&pointlist[0]);
    PatternPoints(pointlist,0,patternwidth,z_base,ztire,-dwidth/2+4*dwidth/10,dwidth/20,0,dwidth/20,dwidth/20);
    mk_arb8(file,"patterncomponent15.s",&pointlist[0]);
    PatternPoints(pointlist,-patternwidth,0,z_base,ztire,-dwidth/2,0,dwidth/20,dwidth/20,dwidth/20);

    PatternPoints(pointlist,-patternwidth,0,z_base,ztire,dwidth/2,0,-dwidth/20,-dwidth/20,-dwidth/20);
    mk_arb8(file,"patterncomponent6.s",&pointlist[0]);
    PatternPoints(pointlist,0,patternwidth,z_base,ztire,dwidth/2,-dwidth/20,0,-dwidth/20,-dwidth/20);
    mk_arb8(file,"patterncomponent16.s",&pointlist[0]);
    
    PatternPoints(pointlist,-patternwidth,0,z_base,ztire,dwidth/2-dwidth/10,0,-dwidth/20,-dwidth/20,-dwidth/20);
    mk_arb8(file,"patterncomponent7.s",&pointlist[0]);
    PatternPoints(pointlist,0,patternwidth,z_base,ztire,dwidth/2-dwidth/10,-dwidth/20,0,-dwidth/20,-dwidth/20);
    mk_arb8(file,"patterncomponent17.s",&pointlist[0]);
   
    PatternPoints(pointlist,-patternwidth,0,z_base,ztire,dwidth/2-2*dwidth/10,0,-dwidth/20,-dwidth/20,-dwidth/20);
    mk_arb8(file,"patterncomponent8.s",&pointlist[0]);
    PatternPoints(pointlist,0,patternwidth,z_base,ztire,dwidth/2-2*dwidth/10,-dwidth/20,0,-dwidth/20,-dwidth/20);
    mk_arb8(file,"patterncomponent18.s",&pointlist[0]);
    
    PatternPoints(pointlist,-patternwidth,0,z_base,ztire,dwidth/2-3*dwidth/10,0,-dwidth/20,-dwidth/20,-dwidth/20);
    mk_arb8(file,"patterncomponent9.s",&pointlist[0]);
    PatternPoints(pointlist,0,patternwidth,z_base,ztire,dwidth/2-3*dwidth/10,-dwidth/20,0,-dwidth/20,-dwidth/20);
    mk_arb8(file,"patterncomponent19.s",&pointlist[0]);
    
    PatternPoints(pointlist,-patternwidth,0,z_base,ztire,dwidth/2-4*dwidth/10,0,-dwidth/20,-dwidth/20,-dwidth/20);
    mk_arb8(file,"patterncomponent10.s",&pointlist[0]);
    PatternPoints(pointlist,0,patternwidth,z_base,ztire,dwidth/2-4*dwidth/10,-dwidth/20,0,-dwidth/20,-dwidth/20);
    mk_arb8(file,"patterncomponent20.s",&pointlist[0]);

    BU_LIST_INIT(&treadpattern.l);
    for (i = 1; i <= 20; i++){
	bu_vls_trunc(&str,0);
	bu_vls_printf(&str, "patterncomponent%d.s",i);
	(void)mk_addmember(bu_vls_addr(&str), &treadpattern.l, NULL, WMOP_UNION);
    }
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tread_master.c");
    mk_lcomb(file, bu_vls_addr(&str), &treadpattern, 0, NULL, NULL, NULL, 0);

    BU_LIST_INIT(&tread.l);
    BU_LIST_INIT(&treadrotated.l);
    for (i=1; i<=number_of_patterns; i++) {
	bu_vls_trunc(&str,0);
	bu_vls_printf(&str, "tread_master.c");
	getYRotMat(&y,D2R(i*360/number_of_patterns));
	(void)mk_addmember("tire-tread-shape.c",&tread.l, NULL, WMOP_UNION);
	(void)mk_addmember(bu_vls_addr(&str), &tread.l, y, WMOP_INTERSECT);
	bu_vls_trunc(&str,0);
	bu_vls_printf(&str, "tread-component-%d.c", i);
	mk_lcomb(file, bu_vls_addr(&str), &tread, 0, NULL, NULL, NULL, 0);
	(void)mk_addmember(bu_vls_addr(&str), &treadrotated.l, NULL, WMOP_UNION);
	(void)BU_LIST_POP_T(&tread.l,struct wmember);
    }
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tread.c");
    mk_lcomb(file, bu_vls_addr(&str), &treadrotated, 1, "plastic", "di=.8 sp=.2", rgb,0);
}



/* Function to actually insert BRL-CAD tire primitives into a designated file and form
 * the correct combinations.  Takes a suffix argument to allow definition of unique
 * tree names.
 */
void MakeTireSurface(struct rt_wdb (*file), char *suffix, fastf_t *ell1cadparams, fastf_t *ell2cadparams, fastf_t ztire, fastf_t dztred, fastf_t dytred, fastf_t dyhub, fastf_t zhub, fastf_t dyside1)
{
    struct wmember tiresideoutercutright, tiresideoutercutleft, tiresideinnercutright, tiresideinnercutleft,tirecuttopcyl;
    struct wmember tiretred, tiresides, tiresurface;
    struct wmember innersolid;
    struct wmember tire; 
    struct bu_vls str;
    bu_vls_init(&str);
    vect_t vertex,height;
    point_t origin,normal,C;
   
    /* Insert primitives */
    VSET(origin, 0, ell1cadparams[0], 0);
    VSET(normal, 0, 1, 0);
    VSET(C, 0, ell1cadparams[2],ell1cadparams[3]);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Ellipse1%s.s", suffix);	
    mk_eto(file, bu_vls_addr(&str), origin, normal, C, ell1cadparams[1], ell1cadparams[4]);
    VSET(origin, 0, ell2cadparams[0], 0);
    VSET(normal, 0, ell2cadparams[2]/sqrt(ell2cadparams[2]*ell2cadparams[2]), 0);
    VSET(C, 0, ell2cadparams[2],ell2cadparams[3]);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Ellipse2%s.s", suffix);	
    mk_eto(file, bu_vls_addr(&str), origin, normal, C, ell2cadparams[1], ell2cadparams[4]);
    VSET(origin, 0, -ell2cadparams[0], 0);
    VSET(normal, 0, -ell2cadparams[2]/sqrt(ell2cadparams[2]*ell2cadparams[2]), 0);
    VSET(C, 0, -ell2cadparams[2],-ell2cadparams[3]);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Ellipse3%s.s", suffix);	
    mk_eto(file, bu_vls_addr(&str), origin, normal, C, ell2cadparams[1], ell2cadparams[4]);
    VSET(vertex, 0, 0, 0);
    VSET(height, 0, ell1cadparams[2]+ell1cadparams[2]*.01, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "TopClipR%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, ztire - dztred);
    VSET(vertex, 0, 0, 0);
    VSET(height, 0, -ell1cadparams[2]-ell1cadparams[2]*.01, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "TopClipL%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, ztire - dztred);
    VSET(vertex, 0, -dyside1/2,0);
    VSET(height, 0, dyside1, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "SideClipInner%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, zhub);
    VSET(vertex, 0, -dytred/2, 0);
    VSET(height, 0, dytred, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "SideClipOuter1%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, ztire - dztred);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "SideClipOuter2%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, ztire);
 
    /* Insert primitives to ensure a solid interior - based
     * on dimensions, either add or subtract cones from the
     * sides of the solid.
     */
    VSET(vertex, 0, -dytred/2, 0);
    VSET(height, 0, dytred, 0);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "InnerSolid%s.s", suffix);	
    mk_rcc(file, bu_vls_addr(&str), vertex, height, ztire-dztred);
    if (!NEAR_ZERO(dytred/2 - dyhub/2, SMALL_FASTF)){
	VSET(normal, 0, 1, 0);
	bu_vls_trunc(&str,0);
    	bu_vls_printf(&str, "LeftCone%s.s", suffix);	
    	mk_cone(file, bu_vls_addr(&str), vertex, normal, dytred/2 - dyhub/2, ztire-dztred,zhub);
    	VSET(vertex, 0, dytred/2, 0);
    	VSET(normal, 0, -1, 0);
    	bu_vls_trunc(&str,0);
    	bu_vls_printf(&str, "RightCone%s.s", suffix);	
    	mk_cone(file, bu_vls_addr(&str), vertex, normal, dytred/2 - dyhub/2, ztire-dztred,zhub);
    }	

    /* Define the tire tread surface trimming volume */
    BU_LIST_INIT(&tirecuttopcyl.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "SideClipOuter2%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirecuttopcyl.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "SideClipOuter1%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tirecuttopcyl.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0) ;
    bu_vls_printf(&str, "tire-outer-sides-trim%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tirecuttopcyl, 0, NULL, NULL, NULL, 0);
    
    /* Define outer cut for right (positive y) side of tire */
    BU_LIST_INIT(&tiresideoutercutright.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Ellipse2%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresideoutercutright.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-outer-sides-trim%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresideoutercutright.l, NULL, WMOP_INTERSECT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-side-outer-cut-right%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tiresideoutercutright, 0, NULL, NULL, NULL, 0);
 
    /* Define outer cut for left (negative y) side of tire */
    BU_LIST_INIT(&tiresideoutercutleft.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Ellipse3%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresideoutercutleft.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-outer-sides-trim%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresideoutercutleft.l, NULL, WMOP_INTERSECT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-side-outer-cut-left%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tiresideoutercutleft, 0, NULL, NULL, NULL, 0);

    /* Define inner cut for right (positive y) side of tire */
    BU_LIST_INIT(&tiresideinnercutright.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Ellipse2%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresideinnercutright.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "SideClipInner%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresideinnercutright.l, NULL, WMOP_INTERSECT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-side-inner-cut-right%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tiresideinnercutright, 0, NULL, NULL, NULL, 0);

    /* Define inner cut for left (negative y) side of tire */
    BU_LIST_INIT(&tiresideinnercutleft.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Ellipse3%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresideinnercutleft.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "SideClipInner%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresideinnercutleft.l, NULL, WMOP_INTERSECT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-side-inner-cut-left%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tiresideinnercutleft, 0, NULL, NULL, NULL, 0);

    /* Combine cuts and primitives to make final tire side surfaces */
    BU_LIST_INIT(&tiresides.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Ellipse2%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresides.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-side-inner-cut-right%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresides.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-side-outer-cut-right%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresides.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Ellipse3%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresides.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-side-inner-cut-left%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresides.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-side-outer-cut-left%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresides.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-sides%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tiresides, 0, NULL, NULL, NULL, 0);

    /* Combine cuts and primitives to make final tire tread surface */
    BU_LIST_INIT(&tiretred.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "Ellipse1%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiretred.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "TopClipR%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiretred.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "TopClipL%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiretred.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-tred-surface%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tiretred, 0, NULL, NULL, NULL, 0);

    /* Combine tire tread surface and tire side surfaces to make final
     * tire surface.
     */
    BU_LIST_INIT(&tiresurface.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-sides%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresurface.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-tred-surface%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tiresurface.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-surface%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tiresurface, 0, NULL, NULL, NULL, 0);

    /* Combine inner solid, cones, and cuts to ensure a filled inner volume. */
    BU_LIST_INIT(&innersolid.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "InnerSolid%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &innersolid.l, NULL, WMOP_UNION);
    if ((dytred/2 - dyhub/2) > 0 && !NEAR_ZERO(dytred/2 - dyhub/2, SMALL_FASTF)) {
    	bu_log("Subtracting cones\n");
    	bu_vls_trunc(&str,0);
    	bu_vls_printf(&str, "LeftCone%s.s", suffix);	
	(void)mk_addmember(bu_vls_addr(&str), &innersolid.l, NULL, WMOP_SUBTRACT);
    	bu_vls_trunc(&str,0);
    	bu_vls_printf(&str, "RightCone%s.s", suffix);	
	(void)mk_addmember(bu_vls_addr(&str), &innersolid.l, NULL, WMOP_SUBTRACT);
    }
    if ((dytred/2 - dyhub/2) < 0 && !NEAR_ZERO(dytred/2 - dyhub/2, SMALL_FASTF)) {
    	bu_log("Adding cones\n");
    	bu_vls_trunc(&str,0);
    	bu_vls_printf(&str, "LeftCone%s.s", suffix);	
	(void)mk_addmember(bu_vls_addr(&str), &innersolid.l, NULL, WMOP_UNION);
    	bu_vls_trunc(&str,0);
    	bu_vls_printf(&str, "RightCone%s.s", suffix);	
	(void)mk_addmember(bu_vls_addr(&str), &innersolid.l, NULL, WMOP_UNION);
    }
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "SideClipInner%s.s", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &innersolid.l, NULL, WMOP_SUBTRACT);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-solid%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &innersolid, 0, NULL, NULL, NULL, 0);

    /* Assemble surfaces and interior fill to make final tire shape */
    BU_LIST_INIT(&tire.l);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-surface%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tire.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire-solid%s.c", suffix);	
    (void)mk_addmember(bu_vls_addr(&str), &tire.l, NULL, WMOP_UNION);
    bu_vls_trunc(&str,0);
    bu_vls_printf(&str, "tire%s.c", suffix);	
    mk_lcomb(file, bu_vls_addr(&str), &tire, 0, NULL, NULL, NULL, 0);
  
    bu_vls_free(&str);
}


/* Use the Gaussian Elimination Routines and MakeTireSurface routine to solve
 * for and insert the shapes needed for a hollow tire*/
void MakeTireCore(struct rt_wdb (*file), fastf_t dytred, fastf_t dztred, fastf_t d1, fastf_t dyside1, fastf_t zside1, fastf_t ztire, fastf_t dyhub, fastf_t zhub, fastf_t thickness)
{
    int i;
    fastf_t ell1partial,elltredpartial;
    fastf_t ell1coefficients[5],ell2coefficients[5],ell1tredcoefficients[5],ell2tredcoefficients[5];
    fastf_t ell1cadparams[5],ell2cadparams[5],ell1tredcadparams[5],ell2tredcadparams[5];
    fastf_t **matrixell1,**matrixell2,**matrixelltred1, **matrixelltred2;
    fastf_t cut_dytred,cut_dztred,cut_d1,cut_dyside1,cut_zside1,cut_ztire,cut_dyhub,cut_zhub;
    fastf_t cut1partial;
    fastf_t cut1coefficients[5],cut2coefficients[5];
    fastf_t cut1cadparams[5],cut2cadparams[5];
    fastf_t **matrixcut1,**matrixcut2;
    fastf_t ztire_with_offset,d1_intercept;
    struct wmember tiretred;

    ztire_with_offset = ztire-11.0/32.0*bu_units_conversion("in");

    matrixell1 = (fastf_t **)bu_malloc(5 * sizeof(fastf_t *),"matrixrows");
    for (i = 0; i < 5; i++)
	matrixell1[i] = (fastf_t *)bu_malloc(6 * sizeof(fastf_t),"matrixcols");
    
    matrixell2 = (fastf_t **)bu_malloc(5 * sizeof(fastf_t *),"matrixrows");
    for (i = 0; i < 5; i++)
	matrixell2[i] = (fastf_t *)bu_malloc(6 * sizeof(fastf_t),"matrixcols");


    matrixelltred1 = (fastf_t **)bu_malloc(5 * sizeof(fastf_t *),"matrixrows");
    for (i = 0; i < 5; i++)
	matrixelltred1[i] = (fastf_t *)bu_malloc(6 * sizeof(fastf_t),"matrixcols");

    matrixelltred2 = (fastf_t **)bu_malloc(5 * sizeof(fastf_t *),"matrixrows");
    for (i = 0; i < 5; i++)
	matrixelltred2[i] = (fastf_t *)bu_malloc(6 * sizeof(fastf_t),"matrixcols");


    matrixcut1 = (fastf_t **)bu_malloc(5 * sizeof(fastf_t *),"matrixrows");
    for (i = 0; i < 5; i++)
	matrixcut1[i] = (fastf_t *)bu_malloc(6 * sizeof(fastf_t),"matrixcols");
    
    matrixcut2 = (fastf_t **)bu_malloc(5 * sizeof(fastf_t *),"matrixrows");
    for (i = 0; i < 5; i++)
	matrixcut2[i] = (fastf_t *)bu_malloc(6 * sizeof(fastf_t),"matrixcols");

    /* Find slick surface ellipse equation */
    Create_Ell1_Mat(matrixell1, dytred, dztred, d1, ztire_with_offset);
    Echelon(matrixell1);
    SolveEchelon(matrixell1,ell1coefficients);
    ell1partial = GetEllPartialAtPoint(ell1coefficients,dytred/2,ztire_with_offset-dztred);
    /* Find outer side ellipse equation */
    Create_Ell2_Mat(matrixell2, dytred, dztred, dyside1, zside1, ztire_with_offset, dyhub, zhub, ell1partial);
    Echelon(matrixell2);
    SolveEchelon(matrixell2,ell2coefficients);
    /* Calculate BRL-CAD input parameters for outer tread ellipse */
    CalcInputVals(ell1coefficients,ell1cadparams,0);
    /* Calculate BRL-CAD input parameters for outer side ellipse */
    CalcInputVals(ell2coefficients,ell2cadparams,1);
    /* Insert outer tire volume */
    MakeTireSurface(file,"-solid",ell1cadparams,ell2cadparams,ztire_with_offset,dztred,dytred,dyhub,zhub,dyside1);

    /* Find tread surface */
    d1_intercept = GetValueAtZPoint(ell2coefficients,ztire-d1);
    bu_log("d1_intercept = %6.4f\n",d1_intercept);
    Create_Ell1_Mat(matrixelltred1, dytred, dztred, d1, ztire);
    Echelon(matrixelltred1);
    SolveEchelon(matrixelltred1,ell1tredcoefficients);
    CalcInputVals(ell1tredcoefficients,ell1tredcadparams,0);
    elltredpartial = GetEllPartialAtPoint(ell1tredcoefficients,dytred/2,ztire-dztred);
    printVec(ell1tredcoefficients,5,"Ellipse 1 Tred Coefficients");
    Create_Ell2_Mat(matrixelltred2, dytred, dztred, d1_intercept*2, ztire-d1, ztire, dyhub, zhub, elltredpartial);
    Echelon(matrixelltred2);
    SolveEchelon(matrixelltred2,ell2tredcoefficients);
    printVec(ell2tredcoefficients,5,"Ellipse 2 Tred Coefficients");
    CalcInputVals(ell2tredcoefficients,ell2tredcadparams,1);
    printVec(ell2tredcadparams,5,"Ellipse 2 Tred Input Parameters");
    MakeTireSurface(file,"-tread-outer",ell1tredcadparams,ell2tredcadparams,ztire,dztred,dytred,dyhub,zhub,d1_intercept*2);

    /* The tire tread shape needed is the subtraction of the slick surface from the tread shape,
     * which is handled here to supply the correct shape for later tread work.
     */
    BU_LIST_INIT(&tiretred.l);
    (void)mk_addmember("tire-tread-outer.c", &tiretred.l, NULL, WMOP_UNION);
    (void)mk_addmember("tire-solid.c", &tiretred.l, NULL, WMOP_SUBTRACT);
    mk_lcomb(file, "tire-tread-shape.c", &tiretred, 0, NULL, NULL, NULL, 0);


    /* Call function to generate primitives and combinations for actual tread pattern */
    MakeTreadPattern(file, "-tread1", d1_intercept*2, ztire-d1, ztire);

    /* Calculate input parameters for inner cut*/
    cut_ztire = ztire_with_offset-thickness;
    cut_dyside1 = dyside1-thickness*2;
    cut_zside1 = zside1;
    cut_d1 = d1;
    cut_dytred = dytred*cut_dyside1/dyside1;
    cut_dztred = dztred*cut_ztire/ztire_with_offset;
    cut_dyhub = dyhub - thickness*2;
    cut_zhub = zhub;

    /* Find inner tread cut ellipse equation */
    Create_Ell1_Mat(matrixcut1, cut_dytred, cut_dztred, cut_d1, cut_ztire);
    Echelon(matrixcut1);
    SolveEchelon(matrixcut1,cut1coefficients);
    cut1partial = GetEllPartialAtPoint(cut1coefficients,cut_dytred/2,cut_ztire-cut_dztred);
    /* Find inner cut side ellipse equation */
    Create_Ell2_Mat(matrixcut2, cut_dytred, cut_dztred, cut_dyside1, cut_zside1, cut_ztire, cut_dyhub, cut_zhub, cut1partial);
    Echelon(matrixcut2);
    SolveEchelon(matrixcut2,cut2coefficients);
    /* Calculate BRL-CAD input parameters for inner cut tread ellipse */
    CalcInputVals(cut1coefficients,cut1cadparams,0);
    printVec(cut1cadparams,5,"Cut 1 Input Parameters");
    /* Calculate BRL-CAD input parameters for inner cut side ellipse */
    CalcInputVals(cut2coefficients,cut2cadparams,1);
    printVec(cut2cadparams,5,"Cut 2 Input Parameters");
    /* Insert inner tire cut volume */
    MakeTireSurface(file,"-cut",cut1cadparams,cut2cadparams,cut_ztire,cut_dztred,cut_dytred,cut_dyhub,zhub,dyside1);

    for (i = 0; i < 5; i++)
    	bu_free((char *)matrixell1[i], "matrixell1 element");
    bu_free((char *)matrixell1,"matrixell1");
    for (i = 0; i < 5; i++)
    	bu_free((char *)matrixell2[i], "matrixell2 element");
    bu_free((char *)matrixell2,"matrixell2");

    for (i = 0; i < 5; i++)
    	bu_free((char *)matrixelltred1[i], "matrixell1 element");
    bu_free((char *)matrixelltred1,"matrixell1");
    for (i = 0; i < 5; i++)
    	bu_free((char *)matrixelltred2[i], "matrixell2 element");
    bu_free((char *)matrixelltred2,"matrixell2");

    for (i = 0; i < 5; i++)
    	bu_free((char *)matrixcut1[i], "matrixcut1 element");
    bu_free((char *)matrixcut1,"matrixcut1");
    for (i = 0; i < 5; i++)
    	bu_free((char *)matrixcut2[i], "matrixell2 element");
    bu_free((char *)matrixcut2,"matrixell2");


}

/* Process command line arguments */
int ReadArgs(int argc, char **argv, fastf_t *isoarray)
{
    int c = 0;
    char *options="d:";
    int d1, d2, d3;
    char spacer1,tiretype;
    bu_log("Reading args\n");

    bu_opterr = 0;

    while ((c=bu_getopt(argc, argv, options)) != -1) {
	bu_log("in while loop -> %c\n",c);
        switch (c) {
            case 'd' :
		sscanf(bu_optarg,"%d%c%d%c%d",&d1,&spacer1,&d2,&tiretype,&d3);
		bu_log("Dimensions: Width=%2.0dmm, Ratio=%2.0d, Wheel Diameter=%2.0din\n",d1,d2,d3);
		isoarray[0] = d1;
		isoarray[1] = d2;
		isoarray[2] = d3;
                break;
	   }
	 }
	 return(bu_optind);
 }


int main(int ac, char *av[])
{
    struct rt_wdb *db_fp;
    fastf_t dytred,dztred,dyside1,zside1,ztire,dyhub,zhub,d1;
    fastf_t width, ratio, wheeldiam, thickness;
    int bolts;
    fastf_t bolt_diam, bolt_circ_diam, spigot_diam, fixing_offset, bead_height, bead_width, rim_thickness;
    struct wmember tire;
    unsigned char rgb[3];
    fastf_t isoarray[3];

    /* Set Default Parameters - 255/40R18 */ 
    isoarray[0] = 255;
    isoarray[1] = 40;
    isoarray[2] = 18;

    /* Process arguments */
    ReadArgs(ac, av, isoarray);
 
    /* Create/Open file name if supplied,
     * else use "tire.g"
     */
    db_fp = wdb_fopen( av[bu_optind] );
    if (db_fp == NULL){
	db_fp = wdb_fopen("tire.g");
        mk_id(db_fp, "Tire");
    }

    /* Set Tire color */
    VSET(rgb, 40, 40, 40);

    /*Automatic conversion from std dimension info to geometry*/
    width = isoarray[0];
    ratio = isoarray[1];
    wheeldiam = isoarray[2]*bu_units_conversion("in");

    dyside1 = width;
    ztire = ((width*ratio/100)*2+wheeldiam)/2;
    zhub = ztire-width*ratio/100;
    zside1 = ztire-((ztire-zhub)/2*1.2);
    dztred = .001*ratio*zside1;
    dytred = .8 * width;
    dyhub = dytred;
    d1 = (ztire-zhub)/2.5;
    thickness = 15;

    /* Call routine to actually make the tire geometry*/
    MakeTireCore(db_fp, dytred, dztred, d1, dyside1, zside1, ztire, dyhub, zhub, thickness);
     
   /* Combine the tire solid and tire cutout into the
    * final tire region.
    */
    BU_LIST_INIT(&tire.l);
    (void)mk_addmember("tire-solid.c", &tire.l, NULL, WMOP_UNION);
    (void)mk_addmember("tire-cut.c", &tire.l, NULL, WMOP_SUBTRACT);
    mk_lcomb(db_fp, "tire.c", &tire, 1,  "plastic", "di=.8 sp=.2", rgb, 0);

    bolts = 5;
    bolt_diam = 10;
    bolt_circ_diam = 75;
    spigot_diam = 40;
    fixing_offset = 15;
    bead_height = 8;
    bead_width = 8;
    rim_thickness = 5;

    /* Make wheel rim */
    MakeWheelRims(db_fp, "", dyhub, zhub, bolts, bolt_diam, bolt_circ_diam, spigot_diam, fixing_offset, bead_height, bead_width, rim_thickness);




    /* Close database */
    wdb_close(db_fp);

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
