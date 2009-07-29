/*                          H U M A N . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2009 United States Government as represented by
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
/** @file human.c
 *
 * Generator for human models based on height, and other stuff eventually
 *
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

/* Pi */
#define PI 3.1415926536

/*
 * Default height is 5 feet, 8 inches, arbitrarily
 */
#define DEFAULT_HEIGHT_INCHES 68.0 
#define DEFAULT_FILENAME "human.g"

#define MAXLENGTH 64	/*Maxlength of things like object names, filenames */
#define IN2MM	25.4	/*Convert an inch measurement to millimeters */
#define CM2MM	10.0	/*Convert centimeters to millimeters */

char *progname = "Human Model";
char filename[MAXLENGTH]=DEFAULT_FILENAME;

/** Human information structures */
/** Location of all joints on the body located here */
struct jointInfo
{
        point_t headJoint;
        point_t neckJoint;
        point_t leftShoulderJoint;
        point_t rightShoulderJoint;
        point_t elbowJoint;
        point_t wristJoint;
        point_t abdomenJoint;
        point_t pelvisJoint;
        point_t leftThighJoint;
        point_t rightThighJoint;
        point_t kneeJoint;
        point_t ankleJoint;
};

/** Information for building the head */
struct headInfo
{
        fastf_t headSize;
        fastf_t neckLength, neckWidth;

	vect_t headVector;
        vect_t neckVector;
};

/** All information needed to build the torso lies here */
struct torsoInfo
{
        fastf_t torsoLength;
        fastf_t topTorsoLength, lowTorsoLength;
        fastf_t shoulderWidth;
        fastf_t abWidth;
        fastf_t pelvisWidth;

        vect_t topTorsoVector;
        vect_t lowTorsoVector;
};

/** All information needed to build the arms lie here */
struct armInfo
{
        fastf_t armLength;
        fastf_t upperArmWidth;
        fastf_t upperArmLength;
        fastf_t lowerArmLength, elbowWidth;
        fastf_t wristWidth;
        fastf_t handLength, handWidth;

	vect_t armVector;
        vect_t lArmDirection;   /* Arm direction vectors */
        vect_t rArmDirection;
        vect_t lElbowDirection;
        vect_t rElbowDirection;
        vect_t lWristDirection;
        vect_t rWristDirection;
};

/** All information needed to build the legs lie here */
struct legInfo
{
        fastf_t legLength;
        fastf_t thighLength, thighWidth;
        fastf_t calfLength, kneeWidth;
        fastf_t footLength, ankleWidth;
        fastf_t toeWidth;

        vect_t thighVector;
        vect_t calfVector;
        vect_t footVector;
        vect_t lLegDirection;   /* Leg direction vectors */
        vect_t rLegDirection;
        vect_t lKneeDirection;
        vect_t rKneeDirection;
        vect_t lFootDirection;
        vect_t rFootDirection;
};

enum genders { male, female };
enum ethnicities { generic, white, black, hispanic, asian, other }; /* divisions taken from army demographic sheet */

struct human_data_t
{
        fastf_t height;         	/* Height of person standing, inches */
        int age;                	/* Age of person, (still relevant?) */
        enum genders gender;    	/* Gender of person */
        enum ethnicities ethnicity;	/* Ethnicity of person */

	/* Various part lengths */
	struct headInfo head;
	struct torsoInfo torso;
	struct armInfo arms;
	struct legInfo legs;
	struct jointInfo joints;
};

/**
 * This function takes in a vector, then applies a new direction to it
 * using x, y, and z degrees, and exports it to resultVect, and returns
 * the distance of the vector.
 */
fastf_t setDirection(fastf_t *inVect, fastf_t *resultVect, fastf_t *outMatrix, fastf_t x, fastf_t y, fastf_t z)
{
	vect_t outVect;
	mat_t rotMatrix;
	MAT_ZERO(rotMatrix);

	/*
	 * x y z placed inside rotation matrix and applied to vector.
         * bn_mat_angles(rotMatrix, x, y, z) matrix rot in degrees
	 * MAT4X3VEC(outVect, rotMatrix, inVect);
	 */
	bn_mat_angles(rotMatrix, x, y, z);
	MAT3X3VEC(outVect, rotMatrix, inVect);

/* Print rotation matrix
 *	int i=0;
 *	for(i=1; i<=16; i++){
 *		bu_log("%3.4f\t", rotMatrix[(i-1)]);
 *		if(i%4==0)
 *			bu_log("\n");
 *	}
 */
	VMOVE(resultVect, outVect);
	/*return MAGNITUDE(outVect);*/
	MAT_COPY(outMatrix, rotMatrix);
	return *rotMatrix;
}

void vectorTest(struct rt_wdb *file)
{
    /*
     * This code here takes a direction vector, and then redirects it based on the angles given
     * so it is as follows : startingVector, resultVector, xdegrees, ydegrees, zdegrees.
     * and this will be used to position the arms and legs so they are joined yet flexable.
     * Just a test with an rcc.
     */

    /*Vector shape modifying test */
    vect_t test1, test2;
    point_t testpoint;
    mat_t rotMatrix;
    VSET(testpoint, 0.0, 0.0, 0.0);
    VSET(test1, 0, 0, 200);
    setDirection(test1, test2, rotMatrix, 0, 90, 0);
    bu_log("%f, %f, %f\n", test1[X], test1[Y], test1[Z]);
    bu_log("%f, %f, %f\n", test2[X], test2[Y], test2[Z]);
    mk_rcc(file, "NormalTest.s", testpoint, test1, (5*IN2MM));
    mk_rcc(file, "ChangeTest.s", testpoint, test2, (5*IN2MM));
    /* See, now wasn't that easy? */
}

/* Find the hypotenuse of 2 lengths / length vectors */
fastf_t findVector(fastf_t x, fastf_t y)
{
        fastf_t w;
        fastf_t v;
        v = x*x;
        w = y*y;
        return sqrt(v + w);
}

/** Create a bounding box around the individual part, this one has only 1 value for depth and width */
void boundingBox(struct rt_wdb *file, char *name, fastf_t *startPoint, fastf_t *lengthVector, fastf_t partWidth, fastf_t *rotMatrix)
{
	/* Make the arb8/rpp that will bound the part as it were straight up and down, 
	 * And then rotate it to the current position as given in rotMatrix,
	 * followed by naming it by taking name, and cat-ing BOX to the end of it.
	 */
	vect_t vects[8];
	vect_t newVects[8];
	point_t finalPoints[8];
	vect_t distance;
	char newName[MAXLENGTH] = "a";	
	char debug[MAXLENGTH] = "a";
	int i = 0;
	bu_strlcpy(newName, name, MAXLENGTH);
	bu_strlcpy(debug, name, MAXLENGTH);
	bu_strlcat(newName, "Box", MAXLENGTH);
	bu_strlcat(debug, "Joint", MAXLENGTH);

	VADD2(distance, startPoint, lengthVector);
	VSET(distance, distance[X], distance[Y], startPoint[Z]-lengthVector[Z]);

	VSET(vects[0], (-partWidth+startPoint[X]), (-partWidth+startPoint[Y]), (startPoint[Z]));
	VSET(vects[1], (-partWidth+startPoint[X]), (partWidth+startPoint[Y]), (startPoint[Z]));
	VSET(vects[2], (partWidth+startPoint[X]), (partWidth+startPoint[Y]), (startPoint[Z]));
	VSET(vects[3], (partWidth+startPoint[X]), (-partWidth+startPoint[Y]), (startPoint[Z]));
	VSET(vects[4], (-partWidth+startPoint[X]), (-partWidth+startPoint[Y]), (distance[Z]));
	VSET(vects[5], (-partWidth+startPoint[X]), (partWidth+startPoint[Y]), (distance[Z]));
	VSET(vects[6], (partWidth+startPoint[X]), (partWidth+startPoint[Y]), (distance[Z]));
 	VSET(vects[7], (partWidth+startPoint[X]), (-partWidth+startPoint[Y]), (distance[Z]));

	for(i = 0; i<8; i++)
	{
		vects[i][Y]*=-1;
	}

/* Print rotation matrix */
	int w=0;
	for(w=1; w<=16; w++){
	/*Z rotation matrix */
/*		if(w==1 || w==2 || w== 5 || w== 6 || w==11)
			rotMatrix[(w-1)] = rotMatrix[(w-1)] * -1;
*/
	/*Y rotation Matrix */
/*		if(w==1 || w==3 || w== 6 || w==9 || w==11)
			rotMatrix[(w-1)] = rotMatrix[(w-1)] * -1;
*/
	/*X rotation Matrix */
		if(w==1 || w==6 || w== 7 || w==10 || w==11)
			rotMatrix[(w-1)] = rotMatrix[(w-1)] * -1;
/*
		bu_log("%3.4f\t", rotMatrix[(w-1)]);
		if(w%4==0)
			bu_log("\n");
*/
	}
/*	bu_log("-------------------------------+\n");
*/
	/* MAT4X3VEC, rotate a vector about a center point, by a rotmatrix, MAT4X3VEC(new, rotmatrix, old) */
	for(i = 0; i < 8; i++){
	/*
		MAT4X3VEC(newVects[i], rotMatrix, vects[i]);
	*/
		VEC3X3MAT(newVects[i], vects[i], rotMatrix);
	}
	point_t endPoint;
	MAT4X3PNT(endPoint, rotMatrix, startPoint);

	/* Set points to be at end of each vector */
	for(i = 0; i < 8; i++){
		VMOVE(finalPoints[i], newVects[i]);
	}
/*	mk_trc_h(file, debug, endPoint, lengthVector, 2, 2); */
	mk_arb8(file, newName, *finalPoints);
}

/** Create a bounding rectangle around the individual part, and this one has 2 separate values for depth and width */
void boundingRectangle(struct rt_wdb *file, char *name, fastf_t *startPoint, fastf_t *lengthVector, fastf_t partWidth, fastf_t partDepth, fastf_t *rotMatrix)
{
        /* Make the arb8/rpp that will bound the part as it were straight up and down,
         * And then rotate it to the current position as given in rotMatrix,
         * followed by naming it by taking name, and cat-ing BOX to the end of it.
         */
	vect_t vects[8];
        vect_t distance;
        char newName[MAXLENGTH] = "a";

        bu_strlcpy(newName, name, MAXLENGTH);
        bu_strlcat(newName, "Box", MAXLENGTH);

        VADD2(distance, startPoint, lengthVector);
	VSET(distance, distance[X], distance[Y], startPoint[Z]-lengthVector[Z]);

	/* Set first 4 points to be on the same plane as the starting point, and the last 4 points to be the distance vector point plane */
/*
*	fastf_t length=findVector(partWidth, partWidth);
*/
        VSET(vects[0], (-partDepth+startPoint[X]), (-partWidth+startPoint[Y]), (startPoint[Z]));
        VSET(vects[1], (-partDepth+startPoint[X]), (partWidth+startPoint[Y]), (startPoint[Z]));
        VSET(vects[2], (partDepth+startPoint[X]), (partWidth+startPoint[Y]), (startPoint[Z]));
        VSET(vects[3], (partDepth+startPoint[X]), (-partWidth+startPoint[Y]), (startPoint[Z]));
        VSET(vects[4], (-partDepth+startPoint[X]), (-partWidth+startPoint[Y]), (distance[Z]));
        VSET(vects[5], (-partDepth+startPoint[X]), (partWidth+startPoint[Y]), (distance[Z]));
        VSET(vects[6], (partDepth+startPoint[X]), (partWidth+startPoint[Y]), (distance[Z]));
        VSET(vects[7], (partDepth+startPoint[X]), (-partWidth+startPoint[Y]), (distance[Z]));

        mk_arb8(file, newName, *vects);
}

/******************************************/
/***** Body Parts, from the head down *****/
/******************************************/

fastf_t makeHead(struct rt_wdb (*file), char *name, struct human_data_t *dude, fastf_t *direction, fastf_t showBoxes)
{
	fastf_t head = dude->head.headSize / 2;
	vect_t startVector, lengthVector;
	/*Length vector is just the diameter of the head, currently*/
	VSET(lengthVector, 0, 0, dude->head.headSize);
	VSET(startVector, 0, 0, 0);
	mat_t rotMatrix;
	setDirection(startVector, dude->head.headVector, rotMatrix, direction[X], direction[Y], direction[Z]);
	mk_sph(file, name, dude->joints.headJoint, head);

	point_t headFix;
	VSET(headFix, dude->joints.headJoint[X], dude->joints.headJoint[Y], dude->joints.headJoint[Z]+head);
	
	if(showBoxes){
		boundingBox(file, name, headFix, lengthVector, (lengthVector[Z]/2), rotMatrix); 
	}
	return 0;
}

fastf_t makeNeck(struct rt_wdb *file, char *name, struct human_data_t *dude, fastf_t *direction, fastf_t showBoxes)
{
	vect_t startVector;
	mat_t rotMatrix;
	dude->head.neckWidth = dude->head.headSize / 4;
	VSET(startVector, 0, 0, dude->head.neckLength);
	setDirection(startVector, dude->head.neckVector, rotMatrix, direction[X], direction[Y], direction[Z]);
	VADD2(dude->joints.neckJoint, dude->joints.headJoint, dude->head.neckVector);
	mk_rcc(file, name, dude->joints.headJoint, dude->head.neckVector, dude->head.neckWidth);
	
	if(showBoxes){
		boundingBox(file, name, dude->joints.headJoint, startVector, dude->head.neckWidth, rotMatrix);
	}
	return dude->head.neckWidth;
}

fastf_t makeUpperTorso(struct rt_wdb *file, char *name, struct human_data_t *dude, fastf_t *direction, fastf_t showBoxes)
{
	vect_t startVector;
	vect_t a, b, c, d;
	vect_t leftVector, rightVector;
	mat_t rotMatrix;

	/* Set length of top torso portion */
	VSET(startVector, 0, 0, dude->torso.topTorsoLength);
	setDirection(startVector, dude->torso.topTorsoVector, rotMatrix, direction[X], direction[Y], direction[Z]);
	VADD2(dude->joints.abdomenJoint, dude->joints.neckJoint, dude->torso.topTorsoVector);
	
	/* change shoulder joints to match up to torso */
	VSET(leftVector, 0, (dude->torso.shoulderWidth+(dude->torso.shoulderWidth/3)), 0);
	VSET(rightVector, 0, (dude->torso.shoulderWidth+(dude->torso.shoulderWidth/3))*-1, 0);

	VADD2(dude->joints.leftShoulderJoint, dude->joints.neckJoint, leftVector);
	VADD2(dude->joints.rightShoulderJoint, dude->joints.neckJoint, rightVector);

	/*Take shoulder width, and abWidth and convert values to vectors for tgc */
	VSET(a, (dude->torso.shoulderWidth/2), 0, 0);
	VSET(b, 0, dude->torso.shoulderWidth, 0);
	VSET(c, (dude->torso.abWidth/2), 0, 0);
	VSET(d, 0, (dude->torso.abWidth), 0);

	/* Torso will be an ellipsoidal tgc, for more realistic shape */
	mk_tgc(file, name, dude->joints.neckJoint, dude->torso.topTorsoVector, a, b, c, d);

	if(showBoxes){
		boundingRectangle(file, name, dude->joints.neckJoint, startVector, dude->torso.shoulderWidth, (dude->torso.shoulderWidth/2), rotMatrix);
	}
	return dude->torso.abWidth;
}

fastf_t makeLowerTorso(struct rt_wdb *file, char *name, struct human_data_t *dude, fastf_t *direction, fastf_t showBoxes)
{
	vect_t startVector, leftVector, rightVector;
	vect_t a, b, c, d;
	mat_t rotMatrix;

	VSET(startVector, 0, 0, dude->torso.lowTorsoLength);
	setDirection(startVector, dude->torso.lowTorsoVector, rotMatrix, direction[X], direction[Y], direction[Z]);
	VADD2(dude->joints.pelvisJoint, dude->joints.abdomenJoint, dude->torso.lowTorsoVector);

	/* Set locations of legs */
	VSET(leftVector, 0, (dude->torso.pelvisWidth/2), 0);
	VSET(rightVector, 0, ((dude->torso.pelvisWidth/2)*-1), 0);
	VADD2(dude->joints.leftThighJoint, dude->joints.pelvisJoint, leftVector);
	VADD2(dude->joints.rightThighJoint, dude->joints.pelvisJoint, rightVector);

	VSET(a, (dude->torso.abWidth/2), 0, 0);
	VSET(b, 0, dude->torso.abWidth, 0);
	VSET(c, (dude->torso.pelvisWidth/2), 0, 0);
	VSET(d, 0, dude->torso.pelvisWidth, 0);

	mk_tgc(file, name, dude->joints.abdomenJoint, dude->torso.lowTorsoVector, a, b, c, d);
/*
 *	mk_trc_h(file, name, abdomenJoint, lowTorsoVector, abWidth, pelvisWidth);
 */
	if(showBoxes){
		boundingRectangle(file, name, dude->joints.abdomenJoint, startVector, dude->torso.pelvisWidth, (dude->torso.pelvisWidth/2), rotMatrix);
	}
	return dude->torso.pelvisWidth;
}

fastf_t makeShoulderJoint(struct rt_wdb *file, fastf_t isLeft, char *name, struct human_data_t *dude, fastf_t showBoxes)
{
	vect_t startVector, lengthVector;
	VSET(startVector, 0, 0, dude->arms.upperArmWidth*2);
	VSET(lengthVector, 0, 0, dude->arms.upperArmWidth);
	mat_t rotMatrix;
	fastf_t shoulder = dude->arms.upperArmWidth;

	if(isLeft)
		mk_sph(file, name, dude->joints.leftShoulderJoint, (dude->arms.upperArmWidth));
	else
		mk_sph(file, name, dude->joints.rightShoulderJoint, (dude->arms.upperArmWidth));

 	point_t leftFix, rightFix;

	if(showBoxes){
		if(isLeft){
			setDirection(startVector, lengthVector, rotMatrix, dude->arms.lArmDirection[X], dude->arms.lArmDirection[Y], dude->arms.lArmDirection[Z]);
			VSET(leftFix, dude->joints.leftShoulderJoint[X], dude->joints.leftShoulderJoint[Y], (dude->joints.leftShoulderJoint[Z]-shoulder));
			boundingBox(file, name, leftFix, lengthVector, (lengthVector[Z]/2), rotMatrix); 
		}
		else{
			setDirection(startVector, lengthVector, rotMatrix, dude->arms.rArmDirection[X], dude->arms.rArmDirection[Y], dude->arms.rArmDirection[Z]);
			VSET(rightFix, dude->joints.rightShoulderJoint[X], dude->joints.rightShoulderJoint[Y], (dude->joints.rightShoulderJoint[Z]-shoulder));
			boundingBox(file, name, rightFix, lengthVector, (lengthVector[Z]/2), rotMatrix);
		}
	} 
	return dude->torso.shoulderWidth;
}

fastf_t makeShoulder(struct rt_wdb *file, fastf_t isLeft, char *partName, struct human_data_t *dude, fastf_t showBoxes)
{
	return 1;
}

fastf_t makeUpperArm(struct rt_wdb *file, fastf_t isLeft, char *partName, struct human_data_t *dude, fastf_t showBoxes)
{
	vect_t startVector;
	mat_t rotMatrix;

	dude->arms.upperArmLength = (dude->height / 4.5) * IN2MM;
	VSET(startVector, 0, 0, dude->arms.upperArmLength);
	if(isLeft){
		setDirection(startVector, dude->arms.armVector, rotMatrix, dude->arms.lArmDirection[X], dude->arms.lArmDirection[Y], dude->arms.lArmDirection[Z]); /* set y to 180 to point down */
		VADD2(dude->joints.elbowJoint, dude->joints.leftShoulderJoint, dude->arms.armVector);
		mk_trc_h(file, partName, dude->joints.leftShoulderJoint, dude->arms.armVector, dude->arms.upperArmWidth, dude->arms.elbowWidth);
	}
	else{
		setDirection(startVector, dude->arms.armVector, rotMatrix, dude->arms.rArmDirection[X], dude->arms.rArmDirection[Y], dude->arms.rArmDirection[Z]); /* set y to 180 to point down */
		VADD2(dude->joints.elbowJoint, dude->joints.rightShoulderJoint, dude->arms.armVector);
		mk_trc_h(file, partName, dude->joints.rightShoulderJoint, dude->arms.armVector, dude->arms.upperArmWidth, dude->arms.elbowWidth);
	}	

/* Vectors and equations for making TGC arms 
 *	vect_t a, b, c, d;
 *	VSET(a, upperArmWidth, 0 , 0);
 *	VSET(b, 0, upperArmWidth, 0);
 *	VSET(c, ((elbowWidth*3)/4), 0, 0)
 *	VSET(d, 0, elbowWidth, 0);
 *	
 *	mk_tgc(file, partName, ShoulderJoint, armVector, a, b, c, d);
 */

	if(showBoxes){
		if(isLeft)
			boundingBox(file, partName, dude->joints.leftShoulderJoint, startVector, dude->arms.upperArmWidth, rotMatrix);
		else
			boundingBox(file, partName, dude->joints.rightShoulderJoint, startVector, dude->arms.upperArmWidth, rotMatrix);
	}
	return dude->arms.elbowWidth;
}

fastf_t makeElbow(struct rt_wdb *file, fastf_t isLeft, char *name, struct human_data_t *dude)
{
	vect_t a, b, c;
	VSET(a, (dude->arms.elbowWidth), 0, 0);
	VSET(b, 0, (dude->arms.elbowWidth), 0);
	VSET(c, 0, 0, dude->arms.elbowWidth);
	
	mk_ell(file, name, dude->joints.elbowJoint, a, b, c);
/*
*	mk_sph(file, name, dude->joints.elbowJoint, dude->arms.elbowWidth);
*/
	return dude->arms.elbowWidth;
}

fastf_t makeLowerArm(struct rt_wdb *file, fastf_t isLeft, char *name, struct human_data_t *dude, fastf_t showBoxes)
{
	vect_t startVector;
	mat_t rotMatrix;

	dude->arms.lowerArmLength = (dude->height / 4.5) * IN2MM;
	VSET(startVector, 0, 0, dude->arms.lowerArmLength);
	if(isLeft){
		setDirection(startVector, dude->arms.armVector, rotMatrix, dude->arms.lElbowDirection[X], dude->arms.lElbowDirection[Y], dude->arms.lElbowDirection[Z]);
		VADD2(dude->joints.wristJoint, dude->joints.elbowJoint, dude->arms.armVector);
	}
	else{
		setDirection(startVector, dude->arms.armVector, rotMatrix, dude->arms.rElbowDirection[X], dude->arms.rElbowDirection[Y], dude->arms.rElbowDirection[Z]);
		VADD2(dude->joints.wristJoint, dude->joints.elbowJoint, dude->arms.armVector);
	}

/* vectors for building a tgc arm (which looks weird when rotated) */
/*	vect_t a, b, c, d;
 *
 *	VSET(a, ((elbowWidth*3)/4), 0, 0);
 *	VSET(b, 0, (elbowWidth), 0);
 *	VSET(c, wristWidth, 0, 0);
 *	VSET(d, 0, wristWidth, 0);
 *
 *	mk_tgc(file, name, elbowJoint, armVector, a, b, c, d);
 */
	mk_trc_h(file, name, dude->joints.elbowJoint, dude->arms.armVector, dude->arms.elbowWidth, dude->arms.wristWidth);

        if(showBoxes){
                if(isLeft)
                        boundingBox(file, name, dude->joints.elbowJoint, startVector, dude->arms.elbowWidth, rotMatrix);
                else   
                        boundingBox(file, name, dude->joints.elbowJoint, startVector, dude->arms.elbowWidth, rotMatrix);
        }

	return dude->arms.wristWidth;
}

fastf_t makeWrist(struct rt_wdb *file, fastf_t isLeft, char *name, struct human_data_t *dude)
{
	mk_sph(file, name, dude->joints.wristJoint, dude->arms.wristWidth);
	return dude->arms.wristWidth;
}

void makeHand(struct rt_wdb *file, fastf_t isLeft, char *name, struct human_data_t *dude, fastf_t showBoxes)
{
	mat_t rotMatrix;
	vect_t startVector;
	dude->arms.handLength = (dude->height / 16) * IN2MM;
	dude->arms.handWidth = (dude->height / 32) * IN2MM;
	VSET(startVector, 0, 0, dude->arms.handLength);
	if(isLeft){
		setDirection(startVector, dude->arms.armVector, rotMatrix, dude->arms.lWristDirection[X], dude->arms.lWristDirection[Y], dude->arms.lWristDirection[Z]);
	}
	else{
		setDirection(startVector, dude->arms.armVector, rotMatrix, dude->arms.rWristDirection[X], dude->arms.rWristDirection[Y], dude->arms.rWristDirection[Z]);
	}

	mk_sph(file, name, dude->joints.wristJoint, dude->arms.wristWidth);

	if(showBoxes)
	{
		boundingRectangle(file, name, dude->joints.wristJoint, startVector, dude->arms.handWidth, dude->arms.handLength, rotMatrix);
	}
}

fastf_t makeThighJoint(struct rt_wdb *file, fastf_t isLeft, char *name, struct human_data_t *dude)
{
	if(isLeft)
		mk_sph(file, name, dude->joints.leftThighJoint, dude->legs.thighWidth);
	else
		mk_sph(file, name, dude->joints.rightThighJoint, dude->legs.thighWidth);

	return dude->legs.thighWidth;
}

fastf_t makeThigh(struct rt_wdb *file, fastf_t isLeft, char *name, struct human_data_t *dude, fastf_t showBoxes)
{
	vect_t startVector;
	VSET(startVector, 0, 0, dude->legs.thighLength);
	mat_t rotMatrix;

	if(isLeft){
		setDirection(startVector, dude->legs.thighVector, rotMatrix, dude->legs.lLegDirection[X], dude->legs.lLegDirection[Y], dude->legs.lLegDirection[Z]);
		VADD2(dude->joints.kneeJoint, dude->joints.leftThighJoint, dude->legs.thighVector);
		mk_trc_h(file, name, dude->joints.leftThighJoint, dude->legs.thighVector, dude->legs.thighWidth, dude->legs.kneeWidth);
	}
	else{
		setDirection(startVector, dude->legs.thighVector, rotMatrix, dude->legs.rLegDirection[X], dude->legs.rLegDirection[Y], dude->legs.rLegDirection[Z]);
		VADD2(dude->joints.kneeJoint, dude->joints.rightThighJoint, dude->legs.thighVector);
		mk_trc_h(file, name, dude->joints.rightThighJoint, dude->legs.thighVector, dude->legs.thighWidth, dude->legs.kneeWidth);
	}
        if(showBoxes){
                if(isLeft)
                        boundingBox(file, name, dude->joints.leftThighJoint, startVector, dude->legs.thighWidth, rotMatrix);
                else   
                        boundingBox(file, name, dude->joints.rightThighJoint, startVector, dude->legs.thighWidth, rotMatrix);
        }
	return dude->legs.kneeWidth;
}

fastf_t makeKnee(struct rt_wdb *file, fastf_t isLeft, char *name, struct human_data_t *dude)
{
	mk_sph(file, name, dude->joints.kneeJoint, dude->legs.kneeWidth);	
	return dude->legs.kneeWidth;
}

fastf_t makeCalf(struct rt_wdb *file, fastf_t isLeft, char *name, struct human_data_t *dude, fastf_t showBoxes)
{
	vect_t startVector;
	mat_t rotMatrix;
	VSET(startVector, 0, 0, dude->legs.calfLength);

/*
* Find a better way of error checking to see if the legs go thru the ground.
*	if(ankleJoint[Y] <= 0){
*                ankleJoint[Y] = ankleWidth;
*        }
*/
	if(isLeft)
		setDirection(startVector, dude->legs.calfVector, rotMatrix, dude->legs.lKneeDirection[X], dude->legs.lKneeDirection[Y], dude->legs.lKneeDirection[Z]);
	else
		setDirection(startVector, dude->legs.calfVector, rotMatrix, dude->legs.rKneeDirection[X], dude->legs.rKneeDirection[Y], dude->legs.rKneeDirection[Z]);

        VADD2(dude->joints.ankleJoint, dude->joints.kneeJoint, dude->legs.calfVector);
	mk_trc_h(file, name, dude->joints.kneeJoint, dude->legs.calfVector, dude->legs.kneeWidth, dude->legs.ankleWidth);

	if(showBoxes){
                if(isLeft)
                        boundingBox(file, name, dude->joints.kneeJoint, startVector, dude->legs.kneeWidth, rotMatrix);
                else
                        boundingBox(file, name, dude->joints.kneeJoint, startVector, dude->legs.kneeWidth, rotMatrix);
	}
	return dude->legs.ankleWidth;
}

fastf_t makeAnkle(struct rt_wdb *file, fastf_t isLeft, char *name, struct human_data_t *dude)
{
	mk_sph(file, name, dude->joints.ankleJoint, dude->legs.ankleWidth);
	return dude->legs.ankleWidth;
}

fastf_t makeFoot(struct rt_wdb *file, fastf_t isLeft, char *name, struct human_data_t *dude, fastf_t showBoxes)
{
	vect_t startVector, boxVector;
	mat_t rotMatrix;
	dude->legs.footLength = dude->legs.ankleWidth * 3;
	dude->legs.toeWidth = dude->legs.ankleWidth * 1.2;
	VSET(startVector, 0, 0, dude->legs.footLength);
	VSET(boxVector, 0, 0, dude->legs.footLength + dude->legs.toeWidth);

	if(isLeft)
        	setDirection(startVector, dude->legs.footVector, rotMatrix, dude->legs.lFootDirection[X], dude->legs.lFootDirection[Y], dude->legs.lFootDirection[Z]);
	else
        	setDirection(startVector, dude->legs.footVector, rotMatrix, dude->legs.rFootDirection[X], dude->legs.rFootDirection[Y], dude->legs.rFootDirection[Z]);

	mk_particle(file, name, dude->joints.ankleJoint, dude->legs.footVector, dude->legs.ankleWidth, dude->legs.toeWidth);

	if(showBoxes){
                if(isLeft)
                        boundingBox(file, name, dude->joints.ankleJoint, boxVector, dude->legs.toeWidth, rotMatrix);
                else  
                        boundingBox(file, name, dude->joints.ankleJoint, boxVector, dude->legs.toeWidth, rotMatrix);
	}
	return 0;
}

/**
 * Make profile makes the head and neck of the body
 */
void makeProfile(struct rt_wdb (*file), char *suffix, struct human_data_t *dude, fastf_t *direction, fastf_t showBoxes)
{
	char headName[MAXLENGTH]="Head.s";
	char neckName[MAXLENGTH]="Neck.s";
	bu_strlcat(headName, suffix, MAXLENGTH);
	bu_strlcat(neckName, suffix, MAXLENGTH);
	dude->head.neckLength = dude->head.headSize / 2;
	makeHead(file, headName, dude, direction, showBoxes);
	makeNeck(file, neckName, dude, direction, showBoxes);
}

/*
 * Create all the torso parts, and set joint locations for each arm, and each leg.
 */
void makeTorso(struct rt_wdb (*file), char *suffix, struct human_data_t *dude, fastf_t *direction, fastf_t showBoxes)
{
	char upperTorsoName[MAXLENGTH]="UpperTorso.s";
	char lowerTorsoName[MAXLENGTH]="LowerTorso.s";
	char leftShoulderName[MAXLENGTH]="LeftShoulder.s";
	char rightShoulderName[MAXLENGTH]="RightShoulder.s";

	bu_strlcat(upperTorsoName, suffix, MAXLENGTH);
	bu_strlcat(lowerTorsoName, suffix, MAXLENGTH);
	bu_strlcat(leftShoulderName, suffix, MAXLENGTH);
	bu_strlcat(rightShoulderName, suffix, MAXLENGTH);

	dude->torso.topTorsoLength = (dude->torso.torsoLength *5) / 8;
	dude->torso.lowTorsoLength = (dude->torso.torsoLength *3) / 8;

	dude->torso.shoulderWidth = (dude->height / 8) *IN2MM;
	dude->torso.abWidth=(dude->height / 9) * IN2MM;
	dude->torso.pelvisWidth=(dude->height / 8) * IN2MM;

        makeUpperTorso(file, upperTorsoName, dude, direction, showBoxes);

	makeShoulder(file, 1, leftShoulderName, dude, showBoxes);
	makeShoulder(file, 0, rightShoulderName, dude, showBoxes);

        makeLowerTorso(file, lowerTorsoName, dude, direction, showBoxes);	
}

/**
 * Make the 3 components of the arm:the upper arm, the lower arm, and the hand.
 */
void makeArm(struct rt_wdb (*file), char *suffix, int isLeft, struct human_data_t *dude, fastf_t showBoxes)
{
	dude->arms.upperArmWidth = dude->arms.armLength / 12;
	dude->arms.elbowWidth = dude->arms.armLength / 13;
	dude->arms.wristWidth = dude->arms.armLength / 15;

	char shoulderJointName[MAXLENGTH];
	char upperArmName[MAXLENGTH];
	char elbowName[MAXLENGTH];
	char lowerArmName[MAXLENGTH];
	char wristName[MAXLENGTH];
	char handName[MAXLENGTH];

        if(isLeft){
		bu_strlcpy(shoulderJointName, "LeftShoulderJoint.s", MAXLENGTH);
                bu_strlcpy(upperArmName, "LeftUpperArm.s", MAXLENGTH);
                bu_strlcpy(elbowName, "LeftElbowJoint.s", MAXLENGTH);
                bu_strlcpy(lowerArmName, "LeftLowerArm.s", MAXLENGTH);
                bu_strlcpy(wristName, "LeftWristJoint.s", MAXLENGTH);
                bu_strlcpy(handName, "LeftHand.s", MAXLENGTH);
        }
        else{
		bu_strlcpy(shoulderJointName, "RightShoulderJoint.s", MAXLENGTH);
                bu_strlcpy(upperArmName, "RightUpperArm.s", MAXLENGTH);
                bu_strlcpy(elbowName, "RightElbowJoint.s", MAXLENGTH);
                bu_strlcpy(lowerArmName, "RightLowerArm.s", MAXLENGTH);
                bu_strlcpy(wristName, "RightWristJoint.s", MAXLENGTH);
                bu_strlcpy(handName, "RightHand.s", MAXLENGTH);
        }

	bu_strlcat(shoulderJointName, suffix, MAXLENGTH);
	bu_strlcat(upperArmName, suffix, MAXLENGTH);
	bu_strlcat(elbowName, suffix, MAXLENGTH);
	bu_strlcat(lowerArmName, suffix, MAXLENGTH);
	bu_strlcat(wristName, suffix, MAXLENGTH);
	bu_strlcat(handName, suffix, MAXLENGTH);

        makeShoulderJoint(file, isLeft, shoulderJointName, dude, showBoxes);
	makeUpperArm(file, isLeft, upperArmName, dude, showBoxes);
        makeElbow(file, isLeft, elbowName, dude);

        makeLowerArm(file, isLeft, lowerArmName, dude, showBoxes);
        makeWrist(file, isLeft, wristName, dude);
        makeHand(file, isLeft, handName, dude, showBoxes);
}

/**
 * Create the leg to be length 'legLength' by making a thigh, calf, and foot to meet requirements.
 */
void makeLeg(struct rt_wdb (*file), char *suffix, int isLeft, struct human_data_t *dude, fastf_t showBoxes)
{
	char thighJointName[MAXLENGTH];
	char thighName[MAXLENGTH];
	char kneeName[MAXLENGTH];
	char calfName[MAXLENGTH];
	char ankleName[MAXLENGTH];
	char footName[MAXLENGTH];

	if(isLeft){
		bu_strlcpy(thighJointName, "LeftThighJoint.s", MAXLENGTH);
                bu_strlcpy(thighName, "LeftThigh.s", MAXLENGTH);
                bu_strlcpy(kneeName, "LeftKneeJoint.s", MAXLENGTH);
                bu_strlcpy(calfName, "LeftCalf.s", MAXLENGTH);
                bu_strlcpy(ankleName, "LeftAnkleJoint.s", MAXLENGTH);
                bu_strlcpy(footName, "LeftFoot.s", MAXLENGTH);
	}
        else{
		bu_strlcpy(thighJointName, "RightThighJoint.s", MAXLENGTH);
                bu_strlcpy(thighName, "RightThigh.s", MAXLENGTH);
                bu_strlcpy(kneeName, "RightKneeJoint.s", MAXLENGTH);
                bu_strlcpy(calfName, "RightCalf.s", MAXLENGTH);
                bu_strlcpy(ankleName, "RightAnkleJoint.s", MAXLENGTH);
                bu_strlcpy(footName, "RightFoot.s", MAXLENGTH);
	}
	bu_strlcat(thighJointName, suffix, MAXLENGTH);
	bu_strlcat(thighName, suffix, MAXLENGTH);
	bu_strlcat(kneeName, suffix, MAXLENGTH);
	bu_strlcat(calfName, suffix, MAXLENGTH);
	bu_strlcat(ankleName, suffix, MAXLENGTH);
	bu_strlcat(footName, suffix, MAXLENGTH);

	/* divvy up the length of the leg to the leg parts */
	dude->legs.thighLength = dude->legs.legLength / 2;
	dude->legs.calfLength = dude->legs.legLength / 2;
	dude->legs.thighWidth = dude->legs.thighLength / 5;
	dude->legs.kneeWidth = dude->legs.thighLength / 6;
	dude->legs.ankleWidth = dude->legs.calfLength / 8;

	makeThighJoint(file, isLeft, thighJointName, dude);
	makeThigh(file, isLeft, thighName, dude, showBoxes);
	makeKnee(file, isLeft, kneeName, dude);
	makeCalf(file, isLeft, calfName, dude, showBoxes);
	makeAnkle(file, isLeft, ankleName, dude);
	makeFoot(file, isLeft, footName, dude, showBoxes);
}

/**
 * Make the head, shoulders knees and toes, so to speak.
 * Head, neck, torso, arms, hands, legs, feet.
 * And dude, a very technical term, is the human_data in a shorter, more readable name
 */
void makeBody(struct rt_wdb (*file), char *suffix, struct human_data_t *dude, fastf_t *location, fastf_t showBoxes)
{
	bu_log("Making Body\n");
	vect_t direction;
	dude->torso.torsoLength = 0;
	dude->head.headSize = (dude->height / 8) * IN2MM;
	dude->arms.armLength = (dude->height / 2) * IN2MM;
	dude->legs.legLength = ((dude->height * 4) / 8) * IN2MM;
	dude->torso.torsoLength = ((dude->height * 3) / 8) * IN2MM;

	/* 
	 * Make sure that vectors, points, and widths are sent to each function 
	 * for direction, location, and correct sizing, respectivly.
	 */
	bu_log("Setting Direction\n");
	VSET(dude->joints.headJoint, location[X], location[Y], (location[Z]+((dude->height*IN2MM)-(dude->head.headSize/2))));
	VSET(direction, 180, 0, 0); /*Make the body build down, from head to toe. Or else it's upsidedown */

	/**
	 * Head Parts
	 */
	/*makeProfile makes the head and the neck */
	makeProfile(file, suffix, dude, direction, showBoxes);

	/**
	 * Torso Parts
	 */
	makeTorso(file, suffix, dude, direction, showBoxes);

	/**
	 * Arm Parts
	 */
	/*The second argument is whether or not it is the left side, 1 = yes, 0 = no) */
	makeArm(file, suffix, 1, dude, showBoxes);
	makeArm(file, suffix, 0, dude, showBoxes);
	
	/**
	 * Leg Parts
	 */
	makeLeg(file, suffix, 1, dude, showBoxes);
	makeLeg(file, suffix, 0, dude, showBoxes);
	bu_log("Body Built\n");
}	

/**
 * MakeArmy makes a square of persons n by n large, where n is the number of persons entered using -N
 * if N is large (>= 20) Parts start disappearing, oddly enough.
 */
void makeArmy(struct rt_wdb (*file), struct human_data_t dude, int number, fastf_t showBoxes)
{
	point_t locations;
	VSET(locations, 0, 0, 0); /* Starting location */
	int x = 0;
	int y = 0;
	int num;
	char testname[10]={'0'};
	num = 0.0;
	char suffix[MAXLENGTH];

	for(x = 0; x<number; x++){
		for(y=0; y<number; y++){
			sprintf(testname, "%d", num);
			bu_strlcpy(suffix, testname, MAXLENGTH);
			makeBody(file, suffix, &dude, locations, showBoxes); 
			VSET(locations, (locations[X]-(48*IN2MM)), locations[Y], 0);
			num++;
		}
		VSET(locations, 0, (locations[Y]-(36*IN2MM)), 0);
	}
}
void grabCoordinates(fastf_t *positions)
{
        printf("X: ");
        scanf("%lf", &positions[X]);
        fflush(stdin);
        printf("Y: ");
        scanf("%lf", &positions[Y]);
        fflush(stdin);
        printf("Z: ");
        scanf("%lf", &positions[Z]);
        fflush(stdin);
}

void manualPosition(struct human_data_t *dude)
{
	vect_t positions;
	printf("Manually Setting All limb Positions in degrees\n");
	printf("Remember, a 180 value for Y means pointing down\nSo for your left arm, xyz of 0 180 0 would be on the side\n");
	printf("Head is always at standing height, at origin\n");

	printf("Left Arm\n");
	printf("Upper Arm\n");
	grabCoordinates(positions);
	VMOVE(dude->arms.lArmDirection, positions);
	printf("Lower Arm\n");
	grabCoordinates(positions);
	VMOVE(dude->arms.lElbowDirection, positions);
	printf("Hand\n");
	grabCoordinates(positions);
	VMOVE(dude->arms.lWristDirection, positions);

	printf("Right Arm\n");	
        printf("Upper Arm\n");
        grabCoordinates(positions);
        VMOVE(dude->arms.rArmDirection, positions);
        printf("Lower Arm\n");
        grabCoordinates(positions);
        VMOVE(dude->arms.rElbowDirection, positions);
        printf("Hand\n");
        grabCoordinates(positions);
        VMOVE(dude->arms.rWristDirection, positions);

	printf("Left Leg\n");
	printf("Thigh\n");
	grabCoordinates(positions);
	VMOVE(dude->legs.lLegDirection, positions);
	printf("Calf\n");
	grabCoordinates(positions);
	VMOVE(dude->legs.lKneeDirection, positions);
	printf("Foot\n");
	grabCoordinates(positions);
	VMOVE(dude->legs.lFootDirection, positions);

	printf("Right Leg\n");	
        printf("Thigh\n");
        grabCoordinates(positions);
        VMOVE(dude->legs.rLegDirection, positions);
        printf("Calf\n");
        grabCoordinates(positions);
        VMOVE(dude->legs.rKneeDirection, positions);
        printf("Foot\n");
        grabCoordinates(positions);
        VMOVE(dude->legs.rFootDirection, positions);
}

/* position limbs to fit stance input in command line (or just default to standing) */
void setStance(fastf_t stance, struct human_data_t *dude)
{
	vect_t downVect, forwardVect, rightVect, leftVect; /* generic vectors for holding temp values */

	/**
	 * The stances are the way the body is positioned via
	 * adjusting limbs and whatnot. THEY ARE:
	 * 0: Standing
	 * 1: Sitting
	 * 2: Driving
	 * 3: Arms Out
	 * 4: The Letterman
	 * 5: The Captain
	 * #: and more as needed
	 * 999: Custom (done interactivly)
	 */

	VSET(downVect, 0, 180, 0); /*straight down*/
	VSET(forwardVect, 0, 90, 0); /*forwards, down X axis */
	VSET(rightVect, 90, 0, 0); /*Right, down Y axis */
	VSET(leftVect, -90, 0, 0); /*Left, up Y axis */

	switch((int)stance)
	{
	case 0:
		bu_log("Making it stand\n");
		VMOVE(dude->arms.lArmDirection, downVect);
		VMOVE(dude->arms.rArmDirection, downVect);
		VMOVE(dude->arms.lElbowDirection, downVect);
		VMOVE(dude->arms.rElbowDirection, downVect);
		VMOVE(dude->arms.lWristDirection, downVect);
		VMOVE(dude->arms.rWristDirection, downVect);
		VMOVE(dude->legs.lLegDirection, downVect);
		VMOVE(dude->legs.rLegDirection, downVect);
		VMOVE(dude->legs.lKneeDirection, downVect);
		VMOVE(dude->legs.rKneeDirection, downVect);		
		VMOVE(dude->legs.lFootDirection, forwardVect);
		VMOVE(dude->legs.rFootDirection, forwardVect);
		bu_log("Standing\n");
		break;
	case 1:
		bu_log("Making it sit\n");
		VMOVE(dude->arms.lArmDirection, downVect);
		VMOVE(dude->arms.rArmDirection, downVect);
                VMOVE(dude->arms.lElbowDirection, downVect);
                VMOVE(dude->arms.rElbowDirection, downVect);
                VMOVE(dude->arms.lWristDirection, downVect);
                VMOVE(dude->arms.rWristDirection, downVect);
		VMOVE(dude->legs.lLegDirection, forwardVect);
		VMOVE(dude->legs.rLegDirection, forwardVect);
		VMOVE(dude->legs.lKneeDirection, downVect);
		VMOVE(dude->legs.rKneeDirection, downVect);
		VMOVE(dude->legs.lFootDirection, forwardVect);
		VMOVE(dude->legs.rFootDirection, forwardVect);
		break;
	case 2:
		bu_log("Making it Drive\n"); /* it's like sitting, but with the arms extended */
		VMOVE(dude->arms.lArmDirection, downVect);
		VMOVE(dude->arms.rArmDirection, downVect);
                VMOVE(dude->arms.lElbowDirection, forwardVect);
                VMOVE(dude->arms.rElbowDirection, forwardVect);
                VMOVE(dude->arms.lWristDirection, forwardVect);
                VMOVE(dude->arms.rWristDirection, forwardVect);
                VMOVE(dude->legs.lLegDirection, forwardVect);
                VMOVE(dude->legs.rLegDirection, forwardVect);
                VMOVE(dude->legs.lKneeDirection, downVect);
                VMOVE(dude->legs.rKneeDirection, downVect);
                VMOVE(dude->legs.lFootDirection, forwardVect);
                VMOVE(dude->legs.rFootDirection, forwardVect);
		break;
	case 3:
		bu_log("Making arms out (standing)\n");
                VMOVE(dude->arms.lArmDirection, leftVect);
                VMOVE(dude->arms.rArmDirection, rightVect);
                VMOVE(dude->arms.lElbowDirection, leftVect);
                VMOVE(dude->arms.rElbowDirection, rightVect);
                VMOVE(dude->arms.lWristDirection, leftVect);
                VMOVE(dude->arms.rWristDirection, rightVect);
                VMOVE(dude->legs.lLegDirection, downVect);
                VMOVE(dude->legs.rLegDirection, downVect);
                VMOVE(dude->legs.lKneeDirection, downVect);
                VMOVE(dude->legs.rKneeDirection, downVect);
                VMOVE(dude->legs.lFootDirection, forwardVect);
                VMOVE(dude->legs.rFootDirection, forwardVect);		
		break;
	case 4:
		bu_log("Making the Letterman\n");
		vect_t larm4, rarm4, knee4, lleg4;
		VSET(larm4, -32, 135, 0);
		VSET(rarm4, 32, 135, 0);
		VSET(knee4, 90, 5, 0);		VSET(lleg4, 0, 75, 0);
                VMOVE(dude->arms.lArmDirection, larm4);
                VMOVE(dude->arms.rArmDirection, rarm4);
                VMOVE(dude->arms.lElbowDirection, larm4);
                VMOVE(dude->arms.rElbowDirection, rarm4);
                VMOVE(dude->arms.lWristDirection, larm4);
                VMOVE(dude->arms.rWristDirection, rarm4);
                VMOVE(dude->legs.lLegDirection, lleg4);
                VMOVE(dude->legs.rLegDirection, forwardVect);
                VMOVE(dude->legs.lKneeDirection, knee4);
                VMOVE(dude->legs.rKneeDirection, downVect);
                VMOVE(dude->legs.lFootDirection, forwardVect);
                VMOVE(dude->legs.rFootDirection, forwardVect);
		break;
	case 5:
		bu_log("Making the Captain\n");
		vect_t larm5, rarm5, llower5, rlower5;
		vect_t rthigh5;
		VSET(larm5, 45, 180, 5);
		VSET(rarm5, -45, 180, -5);
		VSET(llower5, -45, 180, 5);
		VSET(rlower5, 45, 180, -5);
		VSET(rthigh5, 0, 85, 0);
		VMOVE(dude->arms.lArmDirection, larm5);
		VMOVE(dude->arms.lElbowDirection, llower5);
		VMOVE(dude->arms.rArmDirection, rarm5);
		VMOVE(dude->arms.rElbowDirection, rlower5);
		VMOVE(dude->arms.lWristDirection, downVect);
		VMOVE(dude->arms.rWristDirection, downVect);
		VMOVE(dude->legs.lLegDirection, downVect);
		VMOVE(dude->legs.rLegDirection, rthigh5);
		VMOVE(dude->legs.lKneeDirection, downVect);
		VMOVE(dude->legs.rKneeDirection, downVect);
		VMOVE(dude->legs.lFootDirection, forwardVect);
		VMOVE(dude->legs.rFootDirection, forwardVect);
		break;

	/*Following cases are tests */
	case 10:
		bu_log("Test1 15degree incs\n");
		vect_t test1;
		vect_t test2;
		vect_t test3;
		vect_t test4;
		vect_t test5;
		VSET(test1, 0, 0, 0);
		VSET(test2, 0, 15, 0);
		VSET(test3, 0, 30, 0);
		VSET(test4, 0, 60, 0);
		VSET(test5, 0, 90, 0);
                VMOVE(dude->arms.lArmDirection, test1);
                VMOVE(dude->arms.lElbowDirection, test1);
                VMOVE(dude->arms.rArmDirection, test2);
                VMOVE(dude->arms.rElbowDirection, test2);
                VMOVE(dude->arms.lWristDirection, test1);
                VMOVE(dude->arms.rWristDirection, test2);
                VMOVE(dude->legs.lLegDirection, test3);
                VMOVE(dude->legs.rLegDirection, test4);
                VMOVE(dude->legs.lKneeDirection, test3);
                VMOVE(dude->legs.rKneeDirection, test4);
                VMOVE(dude->legs.lFootDirection, test5);
                VMOVE(dude->legs.rFootDirection, test5);
		break;

	/* Additional Positions go here*/

	case 999:
		/* interactive position-setter-thingermajiger */
		manualPosition(dude);
		break;
	default:
                bu_log("Bad Input, defaulting to Stand\n");
                VMOVE(dude->arms.lArmDirection, downVect);
                VMOVE(dude->arms.rArmDirection, downVect);
                VMOVE(dude->arms.lElbowDirection, downVect);
                VMOVE(dude->arms.rElbowDirection, downVect);
                VMOVE(dude->arms.lWristDirection, downVect);
                VMOVE(dude->arms.rWristDirection, downVect);
                VMOVE(dude->legs.lLegDirection, downVect);
                VMOVE(dude->legs.rLegDirection, downVect);
                VMOVE(dude->legs.lKneeDirection, downVect);
                VMOVE(dude->legs.rKneeDirection, downVect);
                VMOVE(dude->legs.lFootDirection, forwardVect);
                VMOVE(dude->legs.rFootDirection, forwardVect);
		break;
	}
	bu_log("Exiting stance maker\n");
}

/**
 * Goes through the human struct and sets all measurements to needed measurements,
 * i.e. if certain percentile person is needed, those measurements are set. 
 */
void setMeasurements(struct human_data_t *dude, fastf_t percentile)
{
	/* If percentile, load data from database or something */

	/* Standing height from this point on will be derived from gathered values
	 * so it will be a combination of leglength, torsolength, and headsize. So standing
	 * height is now mostly irrelevant
	 */
	bu_log("Setting %.0f percentile data\n", percentile);

/*	dude->head.headSize=
	dude->head.neckLength=
	dude->head.neckWidth=

	dude->torso.topTorsoLength=
	dude->torso.lowTorsoLength=
	dude->torso.shoulderWidth=
	dude->torso.abWidth=
	dude->torso.pelvisWidth=
	dude->torso.torsoLength= dude->torso.topTorsoLength + dude->torso.lowTorsoLength;

	dude->arms.upperArmWidth=
	dude->arms.upperArmLength=
	dude->arms.lowerArmLength=
	dude->arms.elbowWidth=
	dude->arms.wristWidth=
	dude->arms.handLength=
	dude->arms.handWidth=
	dude->arms.armLength=dude->arms.upperArmLength + dude->arms.lowerArmLength + dude->arms.handLength;

	dude->legs.thighLength=
	dude->legs.thighWidth=
	dude->legs.calfLength=
	dude->legs.kneeWidth=
	dude->legs.footLength=
	dude->legs.ankleWidth=
	dude->legs.toeWidth=
	dude->legs.legLength=dude->legs.thighLength + dude->legs.calfLength;

	dude.height=(dude->torso.torsoLength + dude->legs.legLength + dude->head.headSize);
*/
}	

/**
 * Help message printed when -h/-? option is supplied
 */
void show_help(const char *name, const char *optstr)
{
    struct bu_vls str;
    const char *cp = optstr;

    bu_vls_init(&str);
    while (cp && *cp != '\0') {
	if (*cp == ':') {
	    cp++;
	    continue;
	}
	bu_vls_strncat(&str, cp, 1);
	cp++;
    }

    bu_log("usage: %s [%s]\n", name, bu_vls_addr(&str));
    bu_log("options:\n"
	   "\t-h\t\tShow help\n"
	   "\t-?\t\tShow help\n"
	   "\t-A\t\tAutoMake defaults\n"
	   "\t-H\t\tSet Height in inches\n"
	   "\t-L\t\tSet Center Point in inches, at feet (default 0 0 0)\n"
	   "\t-o\t\tSet output file name\n"
	   "\t-b\t\tShow bounding Boxes\n"
	   "\t-N\t\tNumber to make (square)\n"
	   "\t-s\t\tStance to take 0-Stand 1-Sit 2-Drive 3-Arms out 4-Letterman 5-Captain 999-Custom\n"
	   "\t-p\t\tSet Percentile (not implemented yet) 1-99\n"
	);

    bu_vls_free(&str);
    return;
}

void getLocation(fastf_t *location)
{
    fastf_t x, y, z;
    bu_log("Enter center point\n");
    bu_log("X: ");
    scanf("%lf", &x);
    bu_log("Y: ");
    scanf("%lf", &y);
    bu_log("Z: ");
    scanf("%lf", &z);
    x*= IN2MM;
    y*= IN2MM;
    z*= IN2MM;
    VSET(location, x, y, z);
    fflush(stdin);
}

/* Process command line arguments */
int read_args(int argc, char **argv, struct human_data_t *dude, fastf_t *percentile, fastf_t *location, fastf_t *stance, fastf_t *troops, fastf_t *showBoxes)
{
    char c = 'a';
    char *options="AbH:hLlN:O:o:p:s:w";
    float height=0;
    int soldiers=0;
    int pose=0;
    int percent=50;

    /* don't report errors */
    bu_opterr = 0;

    while ((c=bu_getopt(argc, argv, options)) != EOF) {
	/*bu_log("%c \n", c); Testing to see if args are getting read */
	switch (c) {
	    case 'A':
		bu_log("AutoMode, making 50 percentile man\n");
		dude->height = DEFAULT_HEIGHT_INCHES;
		*percentile=50;
		fflush(stdin);
		break;

            case 'b':
                *showBoxes = 1;                                             
                bu_log("Drawing bounding boxes\n");
		fflush(stdin);
                break;	

            case 'H':
                sscanf(bu_optarg, "%f", &height);
                if(height <= 0.0)
                {
                        bu_log("Impossible height, setting default height!\n");
                        height = DEFAULT_HEIGHT_INCHES;
                        dude->height = DEFAULT_HEIGHT_INCHES;
                        bu_log("%.2f = height in inches\n", height);
                }
                else
                {
                        dude->height = height;
                        bu_log("%.2f = height in inches\n", height);
                }
		fflush(stdin);
                break;

            case 'h':
            case '?':
                show_help(*argv, options);
                bu_exit(EXIT_SUCCESS, NULL);
		fflush(stdin);
                break;

            case 'L':
	    case 'l':
		bu_log("Location\n");
                getLocation(location);
		fflush(stdin);
                break;

            case 'N':
                sscanf(bu_optarg, "%d", &soldiers);
                if(soldiers <= 1){
                        bu_log("Only 1 person. Making 16\n");
                        soldiers = 4;
                }
                bu_log("Auto %d (squared) troop formation\n", soldiers);
                *troops = (float)soldiers;
		fflush(stdin);
                break;

	    case 'o':
	    case 'O':
		memset(filename, 0, MAXLENGTH);
		bu_strlcpy(filename, bu_optarg, MAXLENGTH);
		fflush(stdin);
		break;

	    case 'p':
		sscanf(bu_optarg, "%d", &percent);
		if(percent < 1)
			percent=1;
		if(percent > 99)
			percent=99;
		*percentile=percent;
		fflush(stdin);
		break;

	    case 's':
		sscanf(bu_optarg, "%d", &pose);
		if(pose < 0)
			pose = 0;
		*stance = (float)pose;
		fflush(stdin);
		break;

	    default:
		show_help(*argv, options);
		bu_log("%s: illegal option -- %c\n", bu_getprogname(), c);
	    	bu_exit(EXIT_SUCCESS, NULL);
		fflush(stdin);
		break;
	}
    }
    fflush(stdout);
    return(bu_optind);
}

int main(int ac, char *av[])
{
    struct rt_wdb *db_fp;
    struct wmember human;
    struct wmember boxes;
    struct wmember hollow;
    struct wmember crowd;
    progname = *av;
    struct bu_vls name;
    struct bu_vls str;
    struct human_data_t human_data;
    human_data.height = DEFAULT_HEIGHT_INCHES;
    fastf_t showBoxes = 0, troops = 0, stance = 0, percentile=50;
    char suffix[MAXLENGTH]= "";
    point_t location;
    VSET(location, 0, 0, 0); /* Default standing location */
    bu_vls_init(&name);
    bu_vls_trunc(&name, 0);
    bu_vls_init(&str);
    bu_vls_trunc(&str, 0);

    /* Process command line arguments */
    read_args(ac, av, &human_data, &percentile, location, &stance, &troops, &showBoxes);
    db_fp = wdb_fopen(filename);

    bu_log("Center Location: ");
    bu_log("%.2f %.2f %.2f\n", location[X], location[Y], location[Z]);

/******MAGIC******/
/*Magically set pose, and apply pose to human geometry*/ 
    setMeasurements(&human_data, percentile);
    setStance(stance, &human_data); 
    if(!troops){
    	makeBody(db_fp, suffix, &human_data, location, showBoxes);
	mk_id_units(db_fp, "A single Human", "in");
    }
    if(troops){
	makeArmy(db_fp, human_data, troops, showBoxes);
    	mk_id_units(db_fp, "An army of people", "in");	
    }
/****End Magic****/

/** Make the Regions (.r's) of the body */
/* Make the .r for the real body */
    int is_region = 0;
    unsigned char rgb[3], rgb2[3], rgb3[3];

    if(!troops){    
    BU_LIST_INIT(&human.l);
    (void)mk_addmember("Head.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("Neck.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("UpperTorso.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("LowerTorso.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftUpperArm.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightUpperArm.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftShoulderJoint.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightShoulderJoint.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftElbowJoint.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightElbowJoint.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftLowerArm.s",&human.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightLowerArm.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftWristJoint.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightWristJoint.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftHand.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightHand.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftThigh.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightThigh.s", &human.l, NULL, WMOP_UNION);   
    (void)mk_addmember("LeftThighJoint.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightThighJoint.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftKneeJoint.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightKneeJoint.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftCalf.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightCalf.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftAnkleJoint.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightAnkleJoint.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftFoot.s", &human.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightFoot.s", &human.l, NULL, WMOP_UNION);

    is_region = 1;
    VSET(rgb, 128, 255, 128); /* some wonky bright green color */
    mk_lcomb(db_fp,
	     "Body.r",
	     &human,
	     is_region,
	     "plastic",
	     "di=.99 sp=.01",
	     rgb,
	     0);

/* make the .r for the bounding boxes */
    if(showBoxes){
    /*
     * Create opaque bounding boxes for representaions of where the person model
     * may lay up next to another model
     */
    BU_LIST_INIT(&boxes.l)
    (void)mk_addmember("Head.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("Neck.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("UpperTorso.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("LowerTorso.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftUpperArm.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightUpperArm.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftLowerArm.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightLowerArm.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftHand.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightHand.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftThigh.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightThigh.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftCalf.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightCalf.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftFoot.sBox", &boxes.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightFoot.sBox", &boxes.l, NULL, WMOP_UNION); 
    is_region = 1;
    VSET(rgb2, 255, 128, 128); /* redish color */
        mk_lcomb(db_fp,   
             "Boxes.r",
             &boxes,
             is_region,
             "plastic",
             "di=0.5 sp=0.5",
             rgb2,
             0);
    /*
     * Creating a hollow box that would allow for a person to see inside the
     * bounding boxes to the actual body representation inside.
     */

    BU_LIST_INIT(&hollow.l)
    (void)mk_addmember("Head.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("Head.s", &hollow.l, NULL, WMOP_SUBTRACT);

    (void)mk_addmember("Neck.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("Neck.s", &hollow.l, NULL, WMOP_SUBTRACT);   
    (void)mk_addmember("Head.s", &hollow.l, NULL, WMOP_SUBTRACT);

    (void)mk_addmember("UpperTorso.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("UpperTorso.s", &hollow.l, NULL, WMOP_SUBTRACT);

    (void)mk_addmember("LowerTorso.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("LowerTorso.s", &hollow.l, NULL, WMOP_SUBTRACT);

    (void)mk_addmember("LeftShoulderJoint.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftShoulderJoint.s", &hollow.l, NULL, WMOP_SUBTRACT);

    (void)mk_addmember("RightShoulderJoint.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightShoulderJoint.s", &hollow.l, NULL, WMOP_SUBTRACT);
   
    (void)mk_addmember("LeftUpperArm.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftUpperArm.s", &hollow.l, NULL, WMOP_SUBTRACT);
    
    (void)mk_addmember("RightUpperArm.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightUpperArm.s", &hollow.l, NULL, WMOP_SUBTRACT);
    
    (void)mk_addmember("LeftLowerArm.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftLowerArm.s", &hollow.l, NULL, WMOP_SUBTRACT);
    
    (void)mk_addmember("RightLowerArm.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightLowerArm.s", &hollow.l, NULL, WMOP_SUBTRACT);
    
    (void)mk_addmember("LeftHand.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftHand.s", &hollow.l, NULL, WMOP_SUBTRACT);
    
    (void)mk_addmember("RightHand.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightHand.s", &hollow.l, NULL, WMOP_SUBTRACT);
    
    (void)mk_addmember("LeftThigh.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftThigh.s", &hollow.l, NULL, WMOP_SUBTRACT);
    
    (void)mk_addmember("RightThigh.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightThigh.s", &hollow.l, NULL, WMOP_SUBTRACT);
    
    (void)mk_addmember("LeftCalf.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftCalf.s", &hollow.l, NULL, WMOP_SUBTRACT);
    
    (void)mk_addmember("RightCalf.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightCalf.s", &hollow.l, NULL, WMOP_SUBTRACT);
    
    (void)mk_addmember("LeftFoot.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("LeftFoot.s", &hollow.l, NULL, WMOP_SUBTRACT);
    
    (void)mk_addmember("RightFoot.sBox", &hollow.l, NULL, WMOP_UNION);
    (void)mk_addmember("RightFoot.s", &hollow.l, NULL, WMOP_SUBTRACT);

    is_region = 1;
    VSET(rgb3, 128, 128, 255); /* blueish color */
        mk_lcomb(db_fp,
             "Hollow.r",
             &hollow,
             is_region,
             "glass",
             "di=0.5 sp=0.5 tr=0.75 ri=1",
             rgb3,
             0);
    }
    }
    if(troops){
	bu_log("Naming\n");
    	/*Build body regions for each troop*/
    	/*append number to end of part name, (Head.s0, LeftElbowJoint.s99, etc) */
	int num=0;
	int w=0;
	int x=0;
	char holder[10]={'0'};
	char suffix[MAXLENGTH];	

	for(w=0; w<(troops*troops); w++){
        	char names[MAXLENGTH][MAXLENGTH]={"Head.s", "Neck.s", "UpperTorso.s","LowerTorso.s", "LeftShoulderJoint.s","LeftUpperArm.s","LeftElbowJoint.s",
			"LeftLowerArm.s","LeftWristJoint.s","LeftHand.s","RightShoulderJoint.s","RightUpperArm.s","RightElbowJoint.s","RightLowerArm.s",
			"RightWristJoint.s","RightHand.s","LeftThighJoint.s","LeftThigh.s","LeftKneeJoint.s","LeftCalf.s","LeftAnkleJoint.s","LeftFoot.s",
			"RightThighJoint.s","RightThigh.s","RightKneeJoint.s","RightCalf.s","RightAnkleJoint.s","RightFoot.s","0"};
        	char body[MAXLENGTH][MAXLENGTH]={"Body.r",};
		char box[MAXLENGTH][MAXLENGTH]={"Box.r",};

		bu_log("%d\n", w);

		sprintf(holder, "%d", num);
		bu_strlcpy(suffix, holder, MAXLENGTH);
       	        bu_strlcat(body[0], suffix, MAXLENGTH);
		bu_strlcat(box[0], suffix, MAXLENGTH);
		bu_log("Adding Members\n");
    		BU_LIST_INIT(&human.l);
		BU_LIST_INIT(&crowd.l);
		if(showBoxes){
			BU_LIST_INIT(&boxes.l);
		}
 		
		/*This value is the number of items in char names */
		while(x<28){
			bu_log("%s : ", names[x]);
			bu_strlcat(names[x], suffix, MAXLENGTH);
			(void)mk_addmember(names[x], &human.l, NULL, WMOP_UNION);
			if(showBoxes){
				bu_strlcat(names[x], "Box", MAXLENGTH);
				bu_log("%s : ", names[x]);
				(void)mk_addmember(names[x], &boxes.l, NULL, WMOP_UNION);
			}
			x++;
		}
		x=0;
		VSET(rgb, 128, 255, 128); /* some wonky bright green color */
		bu_log("Combining\n");
		is_region = 1;
		mk_lcomb(db_fp,
       		    body[0],
		    &human,
		    is_region,
		    "plastic",
		    "di=.99 sp=.01",
		    rgb,
		    0);

		if(showBoxes){
			VSET(rgb2, 255, 128, 128); /* redish color */
			mk_lcomb(db_fp,   
             		box[0],
             		&boxes,
             		is_region,
             		"plastic",
             		"di=0.5 sp=0.5",
             		rgb2,
             		0);
		}
		bu_log("%s\n",body[0]);
		num++;
	}
        is_region = 0;
	int z=0;
	char thing[10]="0";
	char thing2[10]="0";
	for(z=0; z<(troops*troops); z++){
		char comber[MAXLENGTH]="Body.r";
		sprintf(thing, "%d", z);
                bu_strlcpy(thing2, thing, MAXLENGTH);
                bu_strlcat(comber, thing2, MAXLENGTH);
		(void)mk_addmember(comber, &crowd.l, NULL, WMOP_UNION);
    	}
    }
        	mk_lcomb(db_fp, "Crowd.c", &crowd, 0, NULL, NULL, NULL, 0);



    /* Close database */
    wdb_close(db_fp);

    bu_vls_free(&name);
    bu_vls_free(&str);

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
