/*                         A L T E R . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2011 United States Government as represented by
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
/** @file libged/alter.c
 *
 * Command to alter objects by translating, rotating, and scaling.
 */

/* Alter: to make different without changing into something else */

/*
 * translate: Proposed operations, and manual page
 *
 * NAME
 *	translate (alias for "alter translate")
 *
 * SYNOPSIS
 *	translate [FROM] TO OBJECT...
 * 	translate [[-n] -k {FROM_OBJECT | FROM_POS}] \
 * 	    [-n] [-a | -r] {TO_OBJECT | TO_POS} OBJECT ...
 *
 *	*OBJECT:
 *	    [PATH/]OBJECT [OFFSET_DIST]
 *	
 *	*POS | *DIST:
 *	    {x [y [z]]} | {[-x {x | X_OBJ}] [-y {y | Y_OBJ}] [-z {z | Z_OBJ}]}
 *
 * DESCRIPTION
 *	Used to move one or more instances of primitive or 
 *	combination objects.
 *
 *	If FROM is omited, the bounding box center of the first
 *	[PATH/]OBJECT is used instead. To use the natural origin of
 *	the first [PATH/]OBJECT as FROM, FROM_OBJECT must be manually
 *	set to [PATH/]OBJECT.
 *
 *	If FROM is "-k .", then each individual [PATH/]OBJECT argument
 *	uses its own bounding box center (or natural origin if
 *	"-n -k ." is used). Likewise, if TO is "-a ." or "-n -a ."
 *	
 *	FROM_POS and TO_POS represent 3d points in "x y z"
 *	coordinates. To specify one or more specific axis while
 *	ignoring the others, the options "-x x", "-y y", "-z z" may be
 *	used as FROM_POS or TO_POS. 
 *
 * OPTIONS
 * 	-n FROM_OBJECT | TO_OBJECT
 *	    Use the natural origin of FROM_OBJECT and/or TO_OBJECT,
 *	    rather than the default of its bounding box center.
 *
 * 	-k FROM_OBJECT | FROM_POS
 *	    Sets the keypoint to FROM_OBJECT's bounding box
 *	    center (or natural origin if -n is used). If this option
 *	    is omitted, the keypoint defaults to OBJECT's bounding
 *	    box center.
 *
 * 	-r TO_POS
 *	    Interpret TO_POS as the relative distance to move OBJECT
 *          from FROM keypoint. This is the default if TO_POS is set.
 *	    Must be omited if TO_OBJECT is specified.
 *
 * 	-a TO_POS | TO_OBJECT
 *	    Interpret TO_POS/TO_OBJECT as an absolute position. The
 *	    vector implied by FROM and TO is used to move OBJECT. This
 *	    option is required if TO_OBJECT is specified.
 *
 * VISUAL EXAMPLE:
 *	translate -n -k rcc.s -a sph.s table.c/box.c 
 *      
 *      Move the instance of box.c in table.c from the natural origin
 *	of rcc.s to the bounding box center of sph.s:
 *	
 *	   |.| <=keypoint: natural origin of rcc.s          
 *	                                                    
 *	       o <= center of sph.2                         
 *                                   [] <=box.c start       
 *	                                                    
 *	                                [] <=box.c moved    
 *                                                          
 * EXAMPLES
 *
 *	# move all instances of sph.s to x=1, y=2, z=3
 *	translate -a 1 2 3 /sph.s
 *
 *	    # these all have the same effect as above
 *	    translate -a 1 2 3 sph.s
 *	    translate -k sph.s -a 1 2 3 sph.s
 *	    translate -k . -a 1 2 3 sph.s
 *
 *      # A very practical use of 
 *	translate -k . -a . -x sph2.s sph1.s
 *
 *	# move all instances of sph.s from a point 5 units above
 *	# sph.s's center to x=1, y=2, z=3, by using OFFSET_DIST of
 *	# "-z 5".
 *	translate -a 1 2 3 sph.s -z 5
 *
 *	    # these all have the same effect as above
 *	    translate -k . -z 5 -a 1 2 3 sph.s
 *	    translate -k sph.s -z 5 -a 1 2 3 sph.s
 *
 *	# OFFSET_DIST of "-z 5" is meaningless here, since OBJECT is
 *	# not implicitly used to calculate the translation. This
 *	# produces an error.
 *	#translate -k sph.s -a 1 2 3 sph.s -z 5
 *
 *      # Place sph1.s and sph2.s at the same height as cube.s using
 *      # OFFSET_DIST of Z_OBJ, and leaving x/y positions alone.
 *      translate -a . -z sph.s sph2.s sph3.s
 *
 *      # Move sph1.s and sph2.s to a height offset 20 units above
 *      # cube.s, while leaving x/y positions alone
 *      translate -k . -z -20 -a -z cube.s sph1.s sph2.s
 *
 *          # these all have the same effect as above
 *          translate -k . -z -20 -a . -z cube.s sph1.s sph2.s
 *          translate -k . -a -z cube.s sph1.s -z -20 sph2.s -z -20
 *          translate -k -x . -y . -z -20 -a -z cube.s sph1.s sph2.s
 *          translate -k 6 7 -20 -a -x 6 -y 7 -z cube.s sph1.s sph2.s
 *
 *	# move sph2.s twice as far away from sph.s as it is now
 *	translate -k sph.s -a . sph2.s
 *
 *	# move all instances of sph.s x+1,y+2,z+3
 *	translate 1 2 3 sph.s
 *
 *	    # these all have the same effect as above
 *	    translate -r 1 2 3 sph.s
 *	    translate -k sph.s -r 1 2 3 sph.s
 *	    translate -k . -r 1 2 3 sph.s
 *
 *	# move instance of sph.s in bowl.c x+1,y+2
 *	translate 1 2 bowl.c/sph.s
 *
 *	# move instance of sph.s in bowl.c z+7
 *	translate -z 7 bowl.c/sph.s
 *
 *	    # exactly the same as above (moving x+0 y+0 z+7)
 *	    translate 0 0 7 bowl.c/sph.s
 *
 *      # move all sph.s from the bounding box center of sph.s to
 *	# the natural origin of sph.s
 *	translate -k sph.s -n -a sph.s sph.s
 *	
 *	    # these all have the same effect as above
 *	    translate -n -a . sph.s
 *	    translate -k . -n -a . sph.s
 *
 *	# move all instances of bowl.c, from sph.s's bounding
 *	# box center to y=5, without changing the x and z coodinates.
 *	translate -k sph.s -a -y 5 bowl.c	
 *
 *      # move instance of two.c, from instance of sph.s's
 *      # matrix-modified natural origin to x=5
 *      translate -n -k bowl.c/sph.s -a 5 one.c/two.c
 *
 *	# move all bowl.c instances and one instance of two.c from
 *	# x=-23,y=4,z=17 to x=9,y=2,z=1
 *	translate -k -23 4 17 -a 9 2 1 bowl.c one.c/two.c
 *
 *	    # exactly the same as above, using relative positioning
 *	    translate -k . -r 32 -2 16 bowl.c one.c/two.c
 *
 *	# these all do nothing
 *	translate -a . sph.s
 *	translate 0 sph.s
 *	translate -k 1 2 3 -a 1 2 3 sph.s
 *	translate -k 1 2 3 -r 0 sph.s
 *	translate -k . -a . sph.s
 *	translate -k sph.s -a sph.s sph.s
 *	translate -n -k . -n -a . sph.s
 *	translate -n -k sph.s -n -a sph.s sph.s
 *     
 *	# center sph1.s and sph2.s on natural origin of rcc.s
 *	translate -k . -n -a rcc.s sph1.s sph2.s
 *
 *      # center sph1.s on natural origin of rcc.s, and move sph.2
 *	# from center of sph1.s to natural origin of rcc.s (keypoint
 *	# is sph1.s's center, so sph2.s comes along for the ride)
 *      translate -n -a rcc.s sph1.s sph2.s
 *
 *	# move each of sph.s, sph2.s and sph3.s a relative x+5
 *	translate -k . -r 5 sph.s sph2.s sph3.s
 *	    
 *	    # same as above
 *	    translate -n -k . -r 5 sph.s sph2.s sph3.s
 *
 *	# move sph.s to a point z+10 above bounding box center of
 *	# /sph2.s
 *	translate -k sph2.s -r -z 10 sph.s
 *	    
 *	# move the bounding box center of all instances of sph.s
 *	# to the natural origin of rcc.s, and move sph2.s from
 *	# the bounding box center of sph.s to the natural origin of 
 *	# rcc.s (both sph.s and sph2.s stay the same relative distance
 *	# from each other; they have shifted together)
 *	translate -n -a rcc.s sph.s sph2.s
 *
 *	# move the natural origins of all instances of sph.s
 *	# and sph2.s to the natural origin of rcc.s
 *	translate -n -k . -n -a rcc.s sph.s sph2.s
 *
 *	# move instance of two.c from x=93.2 to x=-41.7
 *	translate -k 93.2 -a -41.7 one.c/two.c
 *
 *	    # all of these have the same end result as above
 *	    translate -k -x 93.2 -a -41.7 one.c/two.c
 *	    translate -k -x 93.2 -a -x -41.7 one.c/two.c
 *	    translate -k 93.2 0 0 -a -41.7 0 0 one.c/two.c
 *	    translate -k 93.2 21 32 -a -41.7 21 32 one.c/two.c
 * 
 *	    # same result as above, using a relative distance
 *	    translate -134.9 one.c/two.c
 *	    translate -r -134.9 one.c/two.c
 *	    translate -k . -r -134.9 one.c/two.c
 *
 */

/*
 * rotate: Proposed operations, and manual page
 *
 * NAME
 *	rotate (alias for "alter rotate")
 *
 * SYNOPSIS
 *	rotate [-R] [AXIS] [CENTER] ANGLE OBJECT ...
 *	rotate [-R] [[AXIS_FROM] AXIS_TO] [CENTER] [ANGLE_ORIGIN] \
 *	    [ANGLE_FROM] ANGLE_TO OBJECT ...
 * 	rotate [-R] \
 * 	    [[[-n] -k {AXIS_FROM_OBJECT | AXIS_FROM_POS}] \
 * 	    [[-n] [-a | -r] {AXIS_TO_OBJECT | AXIS_TO_POS}]] \
 * 	    [[-n] -c {CENTER_OBJECT | CENTER_POS}] \
 * 	    [[-n] -O {ANGLE_ORIGIN_OBJECT| ANGLE_ORIGIN_POS}] \
 * 	    {[[-n] -k {ANGLE_FROM_OBJECT | ANGLE_FROM_POS}] \
 * 	    [-n | -o] [-a | -r | -d] \
 * 	    {ANGLE_TO_OBJECT | ANGLE_TO_POS}} OBJECT ...
 *
 *	*OBJECT:
 *	    [PATH/]OBJECT [OFFSET_DIST]
 *	
 *	*POS | *DIST:
 *	    {x [y [z]]} | {[-x {x | X_OBJECT}] [-y {y | Y_OBJECT}] \
 *	        [-z {z | Z_OBJECT}]}
 *
 * DESCRIPTION
 *	Used to rotate one or more instances of primitive or
 *	combination objects. OBJECT rotates around CENTER at ANGLE,
 *	which is optionally constrained to rotating around AXIS. *POS
 *	represents either position, distance, degrees, or radians,
 *	depending on the context and supplied arguments. 
 *
 *	AXIS_FROM_POS is always treated as an absolute position.
 *	AXIS_TO_POS may be either an absolute position (-a), or a
 *	relative distance (-r) from AXIS_FROM. OFFSET_DIST is always a
 *	relative distance. A special case is made for ANGLE_TO_POS,
 *	which defaults to relative degrees from ANGLE_FROM_POS (-d),
 *	but may also be interpreted a as relative distance from
 *	ANGLE_FROM_POS (-r) or an absolute position (-a). See
 *	documentation of the -R option to see how it can be used to
 *	allow specification of radians. All *_TO_OBJECT arguments are
 *	always treated as absolute positions (-a).
 *
 *	By default, AXIS is interpreted as the axis to rotate upon
 *	(but does not specify where), with the rotation angle
 *	ANGLE_TO_POS in relative degrees. The default AXIS is the
 *	x-axis, with AXIS_FROM at the origin and AXIS_TO at 1,0,0.
 *
 *	Any *_OBJECT argument may be set to ".", which causes each
 *	individual OBJECT to be used in its place (a batch operation
 *	if there is more than one OBJECT).
 *
 * 	(see DESCRIPTION of translate command for other information)
 *
 * OPTIONS
 *
 * 	-R
 * 	    Interpret AXIS_TO as the center of a circle to rotate
 * 	    with, and the distance between AXIS_FROM and AXIS_TO as
 * 	    its radius. ANGLE_TO_POS is then interpreted as the amount
 * 	    to rotate OBJECT, in radians. If AXIS_TO is not provided,
 * 	    the center of the circle defaults to OBJECT. To use the
 * 	    natural origin of OBJECT as the center of the circle, use
 * 	    OBJECT as AXIS_TO_OBJECT, and enable option "-n".
 *
 *	    If -R is not given, the default is to interpret AXIS_FROM
 *	    as the "zero" point of a new axis of rotation, and AXIS_TO
 *	    as its endpoint, with ANGLE_TO_POS interpreted as relative
 *	    number of degrees to rotate from ANGLE_FROM.
 *
 *	-c CENTER_OBJECT | CENTER_POS
 *	    Set CENTER of the rotation. If omitted, CENTER defaults to
 *	    the bounding box center of OBJECT. To use the natural
 *	    origin of the first OBJECT as CENTER, CENTER_OBJECT must
 *	    be set to OBJECT. Set CENTER_OBJECT to "." to to force
 *	    each OBJECT to rotate around its own center.
 *
 *	-O ANGLE_ORIGIN_OBJECT | ANGLE_ORIGIN_POS
 *	    Sets ANGLE_ORIGIN, the origin of ANGLE. ANGLE_ORIGIN
 *	    defaults to CENTER, the center of the rotation.
 *
 * 	-n *_FROM_OBJECT | *_TO_OBJECT
 *	    Use the natural origin of *_FROM_OBJECT or *_TO_OBJECT,
 *	    rather than the default of the bounding box center.
 *
 * 	-k *_FROM_POS | *_FROM_OBJECT
 *	    Sets the keypoint for the angle or axis of rotation.
 *
 *	    If the AXIS_FROM keypoint is omitted, it defaults to the
 *	    origin (0,0,0).
 *
 *	    The line between the points ANGLE_ORIGIN and ANGLE_FROM
 *	    defines the y-axis of a custom AXIS. Therefore, if AXIS is
 *	    omitted, ANGLE_FROM defaults to a point y-offset +1 from
 *	    ANGLE_ORIGIN; the y-axis of the drawing. In essense,
 *	    ANGLE_FROM helps define the y-axis, and AXIS defines the
 *	    x-axis.
 *	    
 *	    If AXIS is provided, then ANGLE_FROM is aligned to it in
 *	    such a way that the 90 degree angle created by the
 *	    following points is maintained (as illustrated below):
 *
 *	    1) ANGLE_FROM ->
 *	    2) AXIS_TO superimposed onto ANGLE_ORIGIN ->
 *	    3) AXIS_FROM the same relative distance from the
 *	    superimposed AXIS_TO that it was from the actual AXIS_TO
 *
 *          Default:                        | Default 90 degree angle:
 *                                          |
 *                   +z    ANGLE_ORIGIN     |       AXIS_TO->ANGLE_ORIGIN
 *                    |   / (from OBJECT)   |  90  / 
 *          AXIS_FROM |  o                  |    \o   ANGLE_FROM
 *	             \|                     |     ^  /         
 *	    AXIS_TO   o     o               |   o   o          _______
 *	           \ / \     \              |    \            |compass
 *	            o   \     ANGLE_FROM    |     AXIS_FROM   |  +z
 *	         +x/   +y\    (ANGLE_ORIGIN |                 |   |
 *	                       -y 1)        |                 |+x/ \+y
 *
 *	    This is achieved in a consistent manner by starting
 *	    ANGLE_FROM at the x/y of the superimposed AXIS_FROM, and
 *	    the z coordinate of ANGLE_ORIGIN, and rotating it
 *	    counterclockwise around the z-axis of ANGLE_ORIGIN. The
 *	    first encountered 90 degree angle is used. The result is
 *	    that the line between ANGLE_ORIGIN and ANGLE_FROM defines
 *	    the y-axis of the rotation, from the origin to positive-y.
 *	    The rotation y-axis (ANGLE_FROM) is always perpendicular
 *	    to the z-axis of the coordinate system.
 *
 *	    If ANGLE_FROM is set, the default AXIS is ignored, and
 *	    the rotation to ANGLE_TO is unconstrained. This means that
 *	    OBJECT may rotate at CENTER using any ANGLE. There is one
 *	    exception: unconstrained rotations of exactly 180 degrees
 *	    are ambiguous, and will therefore fail. To bypass this
 *	    limitation, perform a relative rotation of 180 degrees in
 *	    the direction required.
 *
 *	    If both ANGLE_FROM and AXIS are set, the rotation from
 *	    ANGLE_FROM to ANGLE_TO is constrained around AXIS.
 *	    This allows for the use of reference objects that are not
 *	    perfectly lined up on AXIS.
 *
 *	-o  ANGLE_TO_POS
 *	    Overrides constraint to AXIS. Only allowed when AXIS is
 *	    supplied, and ANGLE_TO_POS is given in degrees or radians.
 *	    Use -O when the axis you would like to rotate on is
 *	    difficult to reference, but you are able to set AXIS to an
 *	    axis that is perpendicular to it. ANGLE_TO_POS options -y
 *	    or -z would then enable rotation upon the desired axis.
 *
 * 	-a *_TO_POS | *_TO_OBJECT
 *	    Interpret *_TO_POS or *_TO_OBJECT as an absolute position.
 *	    This option is required if *_TO_OBJECT is supplied.
 *
 *	    If any arguments preceed AXIS_TO or ANGLE_TO, they are
 *	    required to have a matching "-a", "-r", or, in the case of
 *	    ANGLE_TO, "-d".
 *
 * 	-r *_TO_POS
 *	    Interpret *_TO_POS as a point a relative distance from
 *	    its *_FROM_POS keypoint. This is the default if *_TO_POS
 *	    is set but *_FROM_POS is omitted. Must be omited if
 *	    *_TO_OBJECT is specified.
 *
 *	    If any arguments preceed AXIS_TO or ANGLE_TO, they are
 *	    required to have a matching "-a", "-r", or, in the case of
 *	    ANGLE_TO, "-d".
 *
 * 	-d ANGLE_TO_POS
 *	    Interpret ANGLE_TO_POS as relative degrees (or radians if
 *	    -R is also set) to rotate its ANGLE_FROM_POS keypoint.
 *	    This option implies -o (override AXIS), since constraining
 *	    to AXIS is not helpful in this case. This option (-d) is
 *	    the default, but in some cases it is required:
 *
 *	    If any arguments preceed ANGLE_TO, it is required to have
 *	    matching "-a", "-r", or "-d".
 *
 * EXAMPLES
 *
 *	The following examples assume we are facing the top of a
 *	cube, which is centered at the origin, and intersects the x
 *	and y axes perpendicularly. Two spheres will serve to
 *	illustrate how reference objects may be used to aid in complex
 *	rotations. The angle between the spheres is intended to
 *	exactly represent the angle between the points at the corners
 *	of the cube that are facing us in the 2nd and 4th quadrants.
 *
 *			  +y     cube  sphere1
 *			 ___|___ /    /
 *			|q2 | q1|    o
 *		   -x __|___|___|___________+x              
 *			|q3 | q4|        o         
 *			|___|___|       / 
 *			    |    sphere2
 *			   -y      
 *
 *	# Rotate the cube 45 degrees counterclockwise around its
 *	# bounding box center, on the z-axis. The corner in q1 will
 *	# end up on the positive y-axis, intersecting the y/z plane.
 *	rotate -z 45 cube
 *
 * 		# all of these have the same result as above
 * 		rotate 0 0 45 cube
 * 		rotate -c 0 0 0 -d 0 0 45 cube
 * 		rotate -c cube 0 0 45 cube
 * 		rotate -c . 0 0 45 cube
 * 		rotate -c -x . -y . -z . cube -z 45 cube
 * 		rotate -c . -z 17 cube -z 45 cube -z -17
 *
 *		# return it to the last position (clockwise)
 *		rotate -z -45 cube
 *
 *		# return it to its last position (counterclockwise)
 *		rotate -z 315 cube
 *
 *	# Rotate the cube (in 3d) so that the edge facing us in the
 *	# positive y quadrants is rolled directly down and towards us,
 *	# stopping where the edge in the negative y quadrants is now.
 *	rotate 90 cube.s
 *
 *	# Rotate the cube in such a way that the corner facing us in
 *	# the q3 is pointing directly at us (we are on a positive z)
 *	rotate -45 45 cube
 *
 *	# Rotate the cube on its center, from sphere1 to sphere2
 *	# (this just happens to be on the z-axis)
 *	rotate -k sphere1 -a sphere2 cube
 *
 *	    # all of these have the same result as above
 *	    rotate -c cube -k sphere1 -a sphere2 cube
 *	    rotate -c . -k sphere1 -a sphere2 cube
 *	    rotate -k -z 0 -r -z 1 -k sphere1 -a sphere2 cube
 *	    rotate -k -z 0 -a -z 1 -k sphere1 -a sphere2 cube
 *	    rotate -k -z 0 -r -z 1 -c . -k sphere1 -a sphere2 cube
 *
 *	    # all of these rotate it back
 *	    rotate -k -z 1 -r -z -1 -k sphere1 -a sphere2 cube
 *	    rotate -k -z 1 -a -z 0 -k sphere1 -a sphere2 cube
 *	    rotate -k -z 5000 -a -z 23 -k sphere1 -a sphere2 cube
 *
 *          # Note that in the examples where AXIS is specified, even
 *          # if sphere1 and sphere2 had different z-coordinates for
 *          # their centers, the rotate behavior would be the same as
 *          # above. In the examples where AXIS is not specified,
 *          # however, the cube would rotate unconstrained in the
 *          # z-axis as well.
 *
 *      # Rotate the cube around a point 25 units in front of itself
 *      # (the front is the right face in the diagram, sharing edge q1
 *      # and q4 with our top view) so that it faces the opposite
 *      # direction
 *      rotate -c . -x 25 -d 180 cube
 *
 *          # same effect (preferred method)
 *          rotate -a 180 cube -x 25
 *
 *      # Rotate the cube so that face that is facing us, is instead
 *      # facing sphere1 (a 3d rotation). Note that CENTER is using
 *      # its default of OBJECT's bounding box center, and
 *      # ANGLE_CENTER is using its default of CENTER.
 *      rotate -k . -z 1 -a sphere1 cube
 *
 *          # all of these have the same result as above
 *          rotate -k . -z 5981 -a sphere1 cube
 *          rotate -k . -z .01 -a sphere1 cube
 *
 *          # move it back
 *          rotate -k sphere1 -a . -z 1 cube
 *
 *      # Rotate cube around an AXIS from sphere2 to sphere1
 *      rotate -k sphere2 -a sphere1 -c sphere2 -d 180 cube
 *
 *          # same effect as above
 *          rotate -k sphere2 -a sphere1 -c sphere2 -d 180 cube
 *          rotate -k sphere1 -a sphere1 -c sphere1 -d 180 cube
 *
 *          # rotate cube on the same axis, so that it is in front of
 *          # the spheres
 *          rotate -k sphere2 -a wphere1 -c sphere1 -d 90 cube
 *
 *      # Rotate both spheres individually around seperate axis,
 *      # so that the same points on each sphere are still facing
 *      # cube, but they are upsidedown.
 *      rotate -k . -a cube -c . -d 180 sphere1 sphere2
 *
 *      # Rotate both spheres around an axis in the middle of them,
 *      # so that they switch places. Any point on AXIS points could
 *      # have been used as CENTER. If the spheres were at different
 *      # z-coordinates, this would still work.
 *      rotate -k sphere1 -y sphere2 -a sphere2 -y sphere1 -c sphere2
 *          -y sphere1 -d 180 sphere1 sphere2
 *
 *
 */

/*
 * scale: Proposed operations, and manual page
 *
 * NAME
 *	scale (alias for "alter scale")
 *
 * SYNOPSIS
 *	FIXME: ensure this translate syntax will work for scale
 *	scale [FROM] TO OBJECT...
 * 	scale [[-n] -k {FROM_OBJECT | FROM_POS}] \
 * 	    [-n] [-a | -r] {TO_OBJECT | TO_POS} OBJECT ...
 *
 *	*OBJECT:
 *	    [PATH/]OBJECT [OFFSET_DIST]
 *	
 *	*POS | *DIST:
 *	    {x [y [z]]} | {[-x {x | X_OBJ}] [-y {y | Y_OBJ}] [-z {z | Z_OBJ}]}
 *
 * DESCRIPTION
 *      FIXME: This is copied directly from translate
 *	Used to move one or more instances of primitive or 
 *	combination objects.
 *
 *	If FROM is omited, the bounding box center of the first
 *	[PATH/]OBJECT is used instead. To use the natural origin of
 *	the first [PATH/]OBJECT as FROM, FROM_OBJECT must be manually
 *	set to [PATH/]OBJECT.
 *
 *	If FROM is "-k .", then each individual [PATH/]OBJECT argument
 *	uses its own bounding box center (or natural origin if
 *	"-n -k ." is used). Likewise, if TO is "-a ." or "-n -a ."
 *	
 *	FROM_POS and TO_POS represent 3d points in "x y z"
 *	coordinates. To specify one or more specific axis while
 *	ignoring the others, the options "-x x", "-y y", "-z z" may be
 *	used as FROM_POS or TO_POS. 
 *
 * OPTIONS
 * 	-n FROM_OBJECT | TO_OBJECT
 *	    Use the natural origin of FROM_OBJECT and/or TO_OBJECT,
 *	    rather than the default of its bounding box center.
 *
 * 	-k FROM_OBJECT | FROM_POS
 *	    Sets the keypoint to FROM_OBJECT's bounding box
 *	    center (or natural origin if -n is used). If this option
 *	    is omitted, the keypoint defaults to OBJECT's bounding
 *	    box center.
 *
 * 	-r TO_POS
 *	    Interpret TO_POS as the relative distance to scale OBJECT
 *          from FROM keypoint. This is the default if TO_POS is set.
 *	    Must be omited if TO_OBJECT is specified.
 *
 * 	-a TO_POS | TO_OBJECT
 *	    Interpret TO_POS/TO_OBJECT as an absolute position. The
 *	    vector implied by FROM and TO is used to scale OBJECT.
 *	    This option is required if TO_OBJECT is specified.
 *
 * EXAMPLES
 *	XXX: add examples
 *                                                          
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "vmath.h"
#include "db.h"
#include "raytrace.h"
#include "ged.h"
#include "./ged_private.h"

#if 0
HIDDEN int
rotate(struct ged *gedp, vect_t *keypoint,
	  struct db_full_path *path,
	  struct directory *d_obj, vect_t delta,
	  int relative_pos_flag)
{
}

HIDDEN int
scale(struct ged *gedp, vect_t *keypoint,
	  struct db_full_path *path,
	  struct directory *d_obj, vect_t delta,
	  int relative_pos_flag)
{
}
#endif

#if 0
HIDDEN int
translate(struct ged *gedp, vect_t *keypoint,
	  struct db_full_path *path,
	  struct directory *d_obj, vect_t delta,
	  int relative_pos_flag)
{
    struct db_full_path full_obj_path;
    struct directory *d_to_modify = NULL;

    struct rt_db_internal intern;
    struct _ged_trace_data gtd;
    mat_t dmat;
    mat_t emat;
    mat_t tmpMat;
    mat_t invXform;
    point_t rpp_min;
    point_t rpp_max;

    /*
     * Validate parameters
     */

    /* perhaps relative positioning was enabled by mistake */
    if (relative_pos_flag && keypoint) {
	bu_vls_printf(gedp->ged_result_str,
		      "relative translations do not have keypoints");
	return GED_ERROR;
    }

    /* TODO: set reasonable default keypoint */
    if (!relative_pos_flag && !keypoint) {
	*keypoint[0] = 0.0;
	*keypoint[1] = 0.0;
	*keypoint[2] = 0.0;
    }

    /* verify existence of path */
    if (ged_path_validate(gedp, path) == GED_ERROR) {
	char *s_path = db_path_to_string(path);
	bu_vls_printf(gedp->ged_result_str, "path \"%s\" doesn't exist",
		      s_path);
	bu_free((genptr_t)s_path, "path string");
	return GED_ERROR;
    }

    /* verify that object exists under current directory in path */
    db_full_path_init(&full_obj_path);
    if (path->fp_len > 0) /* if there's no path, obj is at root */
	db_dup_path_tail(&full_obj_path, path, path->fp_len - (size_t)1);
    db_add_node_to_full_path(&full_obj_path, d_obj);
    if (ged_path_validate(gedp, &full_obj_path) == GED_ERROR) {
	char *s_path = db_path_to_string(path);
	bu_vls_printf(gedp->ged_result_str, "object \"%s\" not found under"
		      " path \"%s\"", d_obj->d_namep, s_path);
	bu_free((genptr_t)s_path, "path string");
	db_free_full_path(&full_obj_path);
	return GED_ERROR;
    }
    db_free_full_path(&full_obj_path);

    if (!(d_obj->d_flags & (RT_DIR_SOLID | RT_DIR_REGION | RT_DIR_COMB))) {
	bu_vls_printf(gedp->ged_result_str, "unsupported object type");
	return GED_ERROR;
    }

    /*
     * Perform translations
     */

    if (!relative_pos_flag)
	    /* 'delta' is actually an absolute position; calculate
	     * distance between it and the keypoint, so that delta
	     * really is a delta */
	    VSUB2(delta, delta, *keypoint);

    if (path->fp_len > 0) {
	/* path supplied; move obj instance only (obj's CWD
	 * modified) */
	struct rt_comb_internal *comb;
	union tree *leaf_to_modify;

	d_to_modify = DB_FULL_PATH_CUR_DIR(path);
	GED_DB_GET_INTERNAL(gedp, &intern, d_to_modify, (fastf_t *)NULL,
			    &rt_uniresource, GED_ERROR);
	comb = (struct rt_comb_internal *)intern.idb_ptr;

	leaf_to_modify = db_find_named_leaf(comb->tree, d_obj->d_namep);
	if (leaf_to_modify == TREE_NULL) {
	    bu_vls_printf(gedp->ged_result_str, "leaf not found where it"
			  " should be; this should not happen");
	    rt_db_free_internal(&intern);
	    return GED_ERROR;
	}
	
	MAT_DELTAS_ADD_VEC(leaf_to_modify->tr_l.tl_mat, delta); 
    } else {
	/* no path; move all obj instances (obj's entire tree
	 * modified) */
	d_to_modify = d_obj;
	if (_ged_get_obj_bounds2(gedp, 1, (const char **)&d_to_modify->d_namep,
				 &gtd, rpp_min, rpp_max) == GED_ERROR)
	    return GED_ERROR;
	if (!(d_to_modify->d_flags & RT_DIR_SOLID))
	    if (_ged_get_obj_bounds(gedp, 1,
				    (const char **)&d_to_modify->d_namep,
				    1, rpp_min, rpp_max) == GED_ERROR)
		return GED_ERROR;

	MAT_IDN(dmat);
	VSCALE(delta, delta, gedp->ged_wdbp->dbip->dbi_local2base);
	MAT_DELTAS_VEC(dmat, delta);

	bn_mat_inv(invXform, gtd.gtd_xform); 
	bn_mat_mul(tmpMat, invXform, dmat);
	bn_mat_mul(emat, tmpMat, gtd.gtd_xform);

	GED_DB_GET_INTERNAL(gedp, &intern, d_to_modify, emat,
			    &rt_uniresource, GED_ERROR);
    }

    RT_CK_DB_INTERNAL(&intern);
    GED_DB_PUT_INTERNAL(gedp, d_to_modify, &intern, &rt_uniresource,
			GED_ERROR);
    rt_db_free_internal(&intern);
    return GED_OK;
}
#endif

int
ged_alter(struct ged *gedp, int argc, const char *argv[])
{
    (void)gedp;
    (void)argc;
    (void)argv;
#if 0
    struct db_i *dbip = gedp->ged_wdbp->dbip;
    const char *const cmd_name = argv[0];
    static const char *usage = "{translate | rotate | scale}"
	" [[-n] -k {FROM_OBJECT|POS}]"
	" {[-n] [-a | -r] TO_OBJECT|POS}" 
	" OBJECT ..." ;
    static const char *skip_arg = ". ";

    int from_center_flag = 0;
    int from_origin_flag = 0;
    const char *s_from_primitive;
    struct db_full_path from_primitive;
    const char *kp_arg = NULL;        	/* keypoint argument */
    vect_t keypoint;

    int to_center_flag = 0;
    int to_origin_flag = 0;
    const char *s_to_primitive;
    struct db_full_path to_primitive;
    vect_t delta;			/* dist/pos to alter to */

    const char *s_obj[] = NULL;
    struct db_full_path obj[] = NULL;
    struct directory *d_obj[] = NULL;

    size_t i;				/* iterator */
    int c;				/* bu_getopt return value */
    char *endchr = NULL;		/* for strtod's */

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_READ_ONLY(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    bu_vls_trunc(gedp->ged_result_str, 0);

    /*
     * Get short arguments
     */

    /* must be wanting help; argc < 3 is wrong too, but more helpful
     * msgs are given later, by saying which args are missing */
    if (argc == 1) {
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", cmd_name, usage);
	return GED_HELP;
    }

    bu_optind = 1; /* re-init bu_getopt() */
    while ((c = bu_getopt(argc, (char * const *)argv, "o:c:k:")) != -1) {
	switch (c) {
	    case 'o':
		if (from_origin_flag) {
		    if (to_origin_flag) {
			bu_vls_printf(gedp->ged_result_str,
				      "too many -o options");
			return GED_ERROR;
		    }
		    to_origin_flag = 1;
		    s_to_primitive = bu_optarg;
		} else {
		    from_origin_flag = 1;
		    s_from_primitive = bu_optarg;
		}
		break;
	    case 'c':
		if (from_center_flag) {
		    if (to_center_flag) {
			bu_vls_printf(gedp->ged_result_str,
				      "too many -c options");
			return GED_ERROR;
		    }
		    to_origin_flag = 1;
		    s_to_primitive = bu_optarg;
		} else {
		    from_origin_flag = 1;
		    s_from_primitive = bu_optarg;
		}

		if (!(s_from_center[0] == skip_arg && kp_arg[1] == ' ')) {
		    /* that's an option, not an arg */
		    bu_vls_printf(gedp->ged_result_str,
				  "Missing argument for option -%c", bu_optopt);
		    return GED_ERROR;
		}
		break;
	    case 'k':
		rel_flag = 1;
		break;
	    default:
		/* options that require arguments */
		switch (bu_optopt) {
		    case 'k':
			bu_vls_printf(gedp->ged_result_str,
				      "Missing argument for option -%c", bu_optopt);
			return GED_ERROR;
		}

		/* unknown options */
		if (isprint(bu_optopt)) {
		    char *c2;
		    strtod((const char *)&c, &c2);
		    if (*c2 != '\0') {
			--bu_optind;
			goto no_more_args;
		    }
			/* it's neither a negative # nor an option */
			bu_vls_printf(gedp->ged_result_str,
				      "Unknown option '-%c'", bu_optopt);
			return GED_ERROR;
		} else {
		    bu_vls_printf(gedp->ged_result_str,
				  "Unknown option character '\\x%x'",
				  bu_optopt);
		    return GED_ERROR;
		}
	}
    }
no_more_args: /* for breaking out, above */

    /* 
     * Validate arguments
     */
    
    /* need to use either absolute||relative positioning; not both */
    if (abs_flag && rel_flag) {
	bu_vls_printf(gedp->ged_result_str,
		      "options '-a' and '-r' are mutually exclusive");
	return GED_ERROR;
    }

    /* set default positioning type */
    if (!abs_flag && !rel_flag)
	rel_flag = 1;
    
    /* set delta coordinates for alter */
    if ((bu_optind + 1) > argc) {
	bu_vls_printf(gedp->ged_result_str, "missing x coordinate");
	return GED_HELP;
    }
    delta[0] = strtod(argv[bu_optind], &endchr);
    if (!endchr || argv[bu_optind] == endchr) {
	bu_vls_printf(gedp->ged_result_str, "missing or invalid x coordinate");
	return GED_ERROR;
    }
    ++bu_optind;
    for (i = 1; i < 3; ++i, ++bu_optind) {
	if ((bu_optind + 1) > argc)
	    break;
	delta[i] = strtod(argv[bu_optind], &endchr);
	if (!endchr || argv[bu_optind] == endchr)
	    /* invalid y or z coord */
	    break;
    }

    /* no args left, but more are expected */
    if ((bu_optind + 1) > argc) {
	bu_vls_printf(gedp->ged_result_str,
		      "missing object argument\n");
	bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", cmd_name, usage);
	return GED_HELP;
    }

    if ((bu_optind + 1) != argc)
	/* if >1 object was supplied, the first must be a path */
	s_path = argv[bu_optind++];
    if (db_string_to_path(&path, dbip, s_path) < 0) {
	bu_vls_printf(gedp->ged_result_str, "invalid path \"%s\"", s_path);
	return GED_ERROR;
    }

    /* set object (no path accepted) */
    s_obj = argv[bu_optind++];
    if (db_string_to_path(&obj, dbip, s_obj) < 0 || obj.fp_len != (size_t)1) {
	bu_vls_printf(gedp->ged_result_str, "invalid object \"%s\"",
		      s_obj);
	db_free_full_path(&path);
	return GED_ERROR;
    }

    if ((bu_optind + 1) <= argc) {
	bu_vls_printf(gedp->ged_result_str, "multiple objects not yet"
		      " supported; ");
	db_free_full_path(&path);
	db_free_full_path(&obj);
	return GED_ERROR;
    }

    /*
     * Perform alter
     */

    d_obj = DB_FULL_PATH_ROOT_DIR(&obj);
    if (!kp_arg) {
	if (alter_translate(gedp, (vect_t *)NULL, &path, d_obj, delta,
		      rel_flag) == GED_ERROR) {
	    db_free_full_path(&path);
	    db_free_full_path(&obj);
	    bu_vls_printf(gedp->ged_result_str, "; translation failed");
	    return GED_ERROR;
	}
    } else {
	if (alter_translate(gedp, &keypoint, &path, d_obj, delta, rel_flag) ==
	    GED_ERROR) {
	    db_free_full_path(&path);
	    db_free_full_path(&obj);
	    bu_vls_printf(gedp->ged_result_str, "; translation failed");
	    return GED_ERROR;
	}
    }

    db_free_full_path(&path);
    db_free_full_path(&obj);
#endif
    bu_vls_printf(gedp->ged_result_str, "command not yet implemented");
    return GED_ERROR;
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
