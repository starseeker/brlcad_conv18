##                 D I S P L A Y . T C L
#
# Author -
#	Bob Parker
#
# Source -
#	The U. S. Army Research Laboratory
#	Aberdeen Proving Ground, Maryland  21005
#
# Distribution Notice -
#	Re-distribution of this software is restricted, as described in
#       your "Statement of Terms and Conditions for the Release of
#       The BRL-CAD Package" agreement.
#
# Copyright Notice -
#       This software is Copyright (C) 1998-2004 by the United States Army
#       in all countries except the USA.  All rights reserved.
#
# Description -
#	The Display class inherits from View and Dm. This
#       class also maintains a list of drawable geometry objects
#       which it can display. It now becomes possible to bind
#       view commands to window events to automatically update the
#       Dm window when the view changes.
#

#
# Usual options.
#
itk::usual Display {
    keep -linewidth
    keep -rscale
    keep -sscale
    keep -type

    keep -centerDotEnable
    keep -centerDotColor

    keep -modelAxesEnable
    keep -modelAxesLineWidth
    keep -modelAxesPosition
    keep -modelAxesSize
    keep -modelAxesColor
    keep -modelAxesLabelColor
    keep -modelAxesTripleColor

    keep -modelAxesTickEnable
    keep -modelAxesTickLength
    keep -modelAxesTickMajorLength
    keep -modelAxesTickInterval
    keep -modelAxesTicksPerMajor
    keep -modelAxesTickColor
    keep -modelAxesTickMajorColor
    keep -modelAxesTickThreshold

    keep -viewAxesEnable
    keep -viewAxesLineWidth
    keep -viewAxesPosition
    keep -viewAxesSize
    keep -viewAxesColor
    keep -viewAxesLabelColor
    keep -viewAxesTripleColor
}

class Display {
    inherit Dm View

    itk_option define -rscale rscale Rscale 0.4
    itk_option define -sscale sscale Sscale 2.0

    itk_option define -centerDotEnable centerDotEnable CenterDotEnable 1
    itk_option define -centerDotColor centerDotColor CenterDotColor {255 255 0}

    itk_option define -modelAxesEnable modelAxesEnable AxesEnable 0
    itk_option define -modelAxesLineWidth modelAxesLineWidth AxesLineWidth 0
    itk_option define -modelAxesPosition modelAxesPosition AxesPosition {0 0 0}
    itk_option define -modelAxesSize modelAxesSize AxesSize 2.0
    itk_option define -modelAxesColor modelAxesColor AxesColor {255 255 255}
    itk_option define -modelAxesLabelColor modelAxesLabelColor AxesLabelColor {255 255 0}
    itk_option define -modelAxesTripleColor modelAxesTripleColor AxesTripleColor 0

    itk_option define -modelAxesTickEnable modelAxesTickEnable AxesTickEnable 1
    itk_option define -modelAxesTickLength modelAxesTickLength AxesTickLength 4
    itk_option define -modelAxesTickMajorLength modelAxesTickMajorLength AxesTickMajorLength 8
    itk_option define -modelAxesTickInterval modelAxesTickInterval AxesTickInterval 100
    itk_option define -modelAxesTicksPerMajor modelAxesTicksPerMajor AxesTicksPerMajor 10
    itk_option define -modelAxesTickColor modelAxesTickColor AxesTickColor {255 255 0}
    itk_option define -modelAxesTickMajorColor modelAxesTickMajorColor AxesTickMajorColor {255 0 0}
    itk_option define -modelAxesTickThreshold modelAxesTickThreshold AxesTickThreshold 8

    itk_option define -viewAxesEnable viewAxesEnable AxesEnable 1
    itk_option define -viewAxesLineWidth viewAxesLineWidth AxesLineWidth 0
    itk_option define -viewAxesPosition viewAxesPosition AxesPosition {-0.85 -0.85 0}
    itk_option define -viewAxesSize viewAxesSize AxesSize 0.2
    itk_option define -viewAxesColor viewAxesColor AxesColor {255 255 255}
    itk_option define -viewAxesLabelColor viewAxesLabelColor AxesLabelColor {255 255 0}
    itk_option define -viewAxesTripleColor viewAxesTripleColor AxesTripleColor 1

    constructor {args} {
	Dm::constructor
	View::constructor
    } {}
    destructor {}

    public method mouse_nirt {_x _y {gi 0}}
    public method nirt {args}
    public method vnirt {vx vy {gi 0}}
    public method qray {args}
    public method refresh {}
    public method rt {args}
    public method rtabort {{gi 0}}
    public method rtcheck {args}
    public method rtedge {args}
    public method autoview {{g_index 0}}
    public method attach_view {}
    public method attach_drawable {dg}
    public method detach_view {}
    public method detach_drawable {dg}
    public method update {obj}

    # methods for maintaining the list of geometry objects
    public method add {glist}
    public method contents {}
    public method remove {glist}

    # methods that override methods inherited from View
    public method slew {args}
    public method perspective_angle {args}

    # methods that override methods inherited from Dm
    public method bounds {args}
    public method depthMask {args}
    public method perspective {args}
    public method fb_active {args}
    public method light {args}
    public method transparency {args}
    public method zbuffer {args}
    public method zclip {args}

    public method toggle_modelAxesEnable {}
    public method toggle_modelAxesTickEnable {}
    public method toggle_viewAxesEnable {}
    public method toggle_centerDotEnable {}

    public method ? {}
    public method apropos {key}
    public method help {args}
    public method getUserCmds {}

    protected method toggle_zclip {}
    protected method toggle_zbuffer {}
    protected method toggle_light {}
    protected method toggle_perspective {}
    protected method toggle_perspective_angle {}
    protected method toggle_transparency {}
    protected method idle_mode {}

    public method rotate_mode {x y}
    public method translate_mode {x y}
    public method scale_mode {x y}

    protected method constrain_rmode {coord x y}
    protected method constrain_tmode {coord x y}
    protected method handle_rotation {x y}
    protected method handle_translation {x y}
    protected method handle_scale {x y}
    protected method handle_constrain_rot {coord x y}
    protected method handle_constrain_tran {coord x y}
    protected method handle_configure {}
    protected method handle_expose {}
    protected method doBindings {}
    public method resetBindings {}

    protected variable minScale 0.0001
    protected variable minAxesSize 0.1
    protected variable minAxesLineWidth 0
    protected variable minAxesTickLength 1
    protected variable minAxesTickMajorLength 1

    private variable x ""
    private variable y ""
    private variable geolist ""
    private variable perspective_angle_index 0
    private variable perspective_angles {90 60 45 30}
    private variable doingInit 1
}

########################### Public/Interface Methods ###########################

body Display::constructor {args} {
    attach_view
    doBindings
    handle_configure
    eval itk_initialize $args
    set doingInit 0
}

configbody Display::rscale {
    if {$itk_option(-rscale) < $minScale} {
	error "rscale must be >= $minScale"
    }
}

configbody Display::sscale {
    if {$itk_option(-sscale) < $minScale} {
	error "sscale must be >= $minScale"
    }
}

configbody Display::centerDotEnable {
    if {$itk_option(-centerDotEnable) != 0 &&
        $itk_option(-centerDotEnable) != 1} {
	error "value must be 0, 1"
    }

    refresh
}

configbody Display::centerDotColor {
    if {[llength $itk_option(-centerDotColor)] != 3} {
	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    set r [lindex $itk_option(-centerDotColor) 0]
    set g [lindex $itk_option(-centerDotColor) 1]
    set b [lindex $itk_option(-centerDotColor) 2]

    # validate color
    if {![string is digit $r] ||
	![string is digit $g] ||
        ![string is digit $b] ||
        $r < 0 || 255 < $r ||
        $g < 0 || 255 < $g ||
        $b < 0 || 255 < $b} {

	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    refresh
}

configbody Display::viewAxesEnable {
    if {$itk_option(-viewAxesEnable) != 0 &&
        $itk_option(-viewAxesEnable) != 1} {
	error "value must be 0, 1"
    }

    refresh
}

configbody Display::modelAxesEnable {
    if {$itk_option(-modelAxesEnable) != 0 &&
        $itk_option(-modelAxesEnable) != 1} {
	error "value must be 0, 1"
    }

    refresh
}

configbody Display::viewAxesSize {
    # validate size
    if {![string is double $itk_option(-viewAxesSize)] ||
        $itk_option(-viewAxesSize) < $minAxesSize} {
	    error "-viewAxesSize must be >= $minAxesSize"
    }

    refresh
}

configbody Display::modelAxesSize {
    # validate size
    if {![string is double $itk_option(-modelAxesSize)] ||
        $itk_option(-modelAxesSize) < $minAxesSize} {
	    error "-modelAxesSize must be >= $minAxesSize"
    }

    refresh
}

configbody Display::viewAxesPosition {
    if {[llength $itk_option(-viewAxesPosition)] != 3} {
	error "values must be {x y z} where x, y and z are numeric"
    }

    set x [lindex $itk_option(-viewAxesPosition) 0]
    set y [lindex $itk_option(-viewAxesPosition) 1]
    set z [lindex $itk_option(-viewAxesPosition) 2]

    # validate center
    if {![string is double $x] ||
	![string is double $y] ||
        ![string is double $z]} {

	error "values must be {x y z} where x, y and z are numeric"
    }

    refresh
}

configbody Display::modelAxesPosition {
    if {[llength $itk_option(-modelAxesPosition)] != 3} {
	error "values must be {x y z} where x, y and z are numeric"
    }

    set x [lindex $itk_option(-modelAxesPosition) 0]
    set y [lindex $itk_option(-modelAxesPosition) 1]
    set z [lindex $itk_option(-modelAxesPosition) 2]

    # validate center
    if {![string is double $x] ||
	![string is double $y] ||
        ![string is double $z]} {

	error "values must be {x y z} where x, y and z are numeric"
    }

    # convert to mm
    set local2mm [local2base]
    set itk_option(-modelAxesPosition) [list [expr {$local2mm * $x}] \
	                                     [expr {$local2mm * $y}] \
					     [expr {$local2mm * $z}]]

    refresh
}

configbody Display::viewAxesLineWidth {
    # validate line width
    if {![string is digit $itk_option(-viewAxesLineWidth)] ||
        $itk_option(-viewAxesLineWidth) < $minAxesLineWidth} {
	    error "-viewAxesLineWidth must be >= $minAxesLineWidth"
    }

    refresh
}

configbody Display::modelAxesLineWidth {
    # validate line width
    if {![string is digit $itk_option(-modelAxesLineWidth)] ||
        $itk_option(-modelAxesLineWidth) < $minAxesLineWidth} {
	    error "-modelAxesLineWidth must be >= $minAxesLineWidth"
    }

    refresh
}

configbody Display::viewAxesTripleColor {
    if {$itk_option(-viewAxesTripleColor) != 0 &&
        $itk_option(-viewAxesTripleColor) != 1} {
	error "value must be 0 or 1"
    }

    refresh
}

configbody Display::modelAxesTripleColor {
    if {$itk_option(-modelAxesTripleColor) != 0 &&
        $itk_option(-modelAxesTripleColor) != 1} {
	error "value must be 0 or 1"
    }

    refresh
}

configbody Display::viewAxesColor {
    if {[llength $itk_option(-viewAxesColor)] != 3} {
	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    set r [lindex $itk_option(-viewAxesColor) 0]
    set g [lindex $itk_option(-viewAxesColor) 1]
    set b [lindex $itk_option(-viewAxesColor) 2]

    # validate color
    if {![string is digit $r] ||
	![string is digit $g] ||
        ![string is digit $b] ||
        $r < 0 || 255 < $r ||
        $g < 0 || 255 < $g ||
        $b < 0 || 255 < $b} {

	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    refresh
}

configbody Display::modelAxesColor {
    if {[llength $itk_option(-modelAxesColor)] != 3} {
	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    set r [lindex $itk_option(-modelAxesColor) 0]
    set g [lindex $itk_option(-modelAxesColor) 1]
    set b [lindex $itk_option(-modelAxesColor) 2]

    # validate color
    if {![string is digit $r] ||
	![string is digit $g] ||
	![string is digit $b] ||
        $r < 0 || 255 < $r ||
        $g < 0 || 255 < $g ||
        $b < 0 || 255 < $b} {

	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    refresh
}

configbody Display::viewAxesLabelColor {
    if {[llength $itk_option(-viewAxesLabelColor)] != 3} {
	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    set r [lindex $itk_option(-viewAxesLabelColor) 0]
    set g [lindex $itk_option(-viewAxesLabelColor) 1]
    set b [lindex $itk_option(-viewAxesLabelColor) 2]

    # validate color
    if {![string is digit $r] ||
	![string is digit $g] ||
        ![string is digit $b] ||
        $r < 0 || 255 < $r ||
        $g < 0 || 255 < $g ||
        $b < 0 || 255 < $b} {

	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    refresh
}

configbody Display::modelAxesLabelColor {
    if {[llength $itk_option(-modelAxesLabelColor)] != 3} {
	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    set r [lindex $itk_option(-modelAxesLabelColor) 0]
    set g [lindex $itk_option(-modelAxesLabelColor) 1]
    set b [lindex $itk_option(-modelAxesLabelColor) 2]

    # validate color
    if {![string is digit $r] ||
	![string is digit $g] ||
	![string is digit $b] ||
        $r < 0 || 255 < $r ||
        $g < 0 || 255 < $g ||
        $b < 0 || 255 < $b} {

	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    refresh
}

configbody Display::modelAxesTickEnable {
    if {$itk_option(-modelAxesTickEnable) != 0 &&
        $itk_option(-modelAxesTickEnable) != 1} {
	error "value must be 0, 1"
    }

    refresh
}

configbody Display::modelAxesTickLength {
    # validate tick length
    if {![string is digit $itk_option(-modelAxesTickLength)] ||
        $itk_option(-modelAxesTickLength) < $minAxesTickLength} {
	    error "-modelAxesTickLength must be >= $minAxesTickLength"
    }

    refresh
}

configbody Display::modelAxesTickMajorLength {
    # validate major tick length
    if {![string is digit $itk_option(-modelAxesTickMajorLength)] ||
        $itk_option(-modelAxesTickMajorLength) < $minAxesTickMajorLength} {
	    error "-modelAxesTickMajorLength must be >= $minAxesTickMajorLength"
    }

    refresh
}

configbody Display::modelAxesTickInterval {
    if {![string is double $itk_option(-modelAxesTickInterval)] ||
        $itk_option(-modelAxesTickInterval) <= 0} {
	error "-modelAxesTickInterval must be > 0"
    }

    # convert to mm
    set itk_option(-modelAxesTickInterval) [expr {[local2base] * $itk_option(-modelAxesTickInterval)}]
    refresh
}

configbody Display::modelAxesTicksPerMajor {
    if {![string is digit $itk_option(-modelAxesTicksPerMajor)]} {
	error "-modelAxesTicksPerMajor must be > 0"
    }

    refresh
}

configbody Display::modelAxesTickColor {
    if {[llength $itk_option(-modelAxesTickColor)] != 3} {
	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    set r [lindex $itk_option(-modelAxesTickColor) 0]
    set g [lindex $itk_option(-modelAxesTickColor) 1]
    set b [lindex $itk_option(-modelAxesTickColor) 2]

    # validate color
    if {![string is digit $r] ||
	![string is digit $g] ||
	![string is digit $b] ||
        $r < 0 || 255 < $r ||
        $g < 0 || 255 < $g ||
        $b < 0 || 255 < $b} {

	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    refresh
}

configbody Display::modelAxesTickMajorColor {
    if {[llength $itk_option(-modelAxesTickMajorColor)] != 3} {
	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    set r [lindex $itk_option(-modelAxesTickMajorColor) 0]
    set g [lindex $itk_option(-modelAxesTickMajorColor) 1]
    set b [lindex $itk_option(-modelAxesTickMajorColor) 2]

    # validate color
    if {![string is digit $r] ||
	![string is digit $g] ||
	![string is digit $b] ||
        $r < 0 || 255 < $r ||
        $g < 0 || 255 < $g ||
        $b < 0 || 255 < $b} {

	error "values must be {r g b} where 0 <= r/g/b <= 255"
    }

    refresh
}

configbody Display::modelAxesTickThreshold {
    if {![string is digit $itk_option(-modelAxesTickThreshold)]} {
	error "-modelAxesTickThreshold must be > 1"
    }

    refresh
}

body Display::update {obj} {
    refresh
}

body Display::refresh {} {
    if {$doingInit} {
	return
    }

    Dm::drawBegin

    if {$itk_option(-perspective)} {
	Dm::loadmat [View::pmodel2view] 0
    } else {
	Dm::loadmat [View::model2view] 0
    }

    if {$itk_option(-fb_active) < 2} {
	if {$itk_option(-fb_active)} {
	    # underlay
	    Dm::refreshfb
	}

	foreach geo $geolist {
	    Dm::drawGeom $geo
	}

	Dm::normal

	if {$itk_option(-viewAxesEnable) ||
	    $itk_option(-modelAxesEnable)} {
		set vsize [expr {[View::local2base] * [View::size]}]
		set rmat [View::rmat]
		set model2view [View::model2view]
	
		if {$itk_option(-viewAxesEnable)} {
		    set x [lindex $itk_option(-viewAxesPosition) 0]
		    set y [lindex $itk_option(-viewAxesPosition) 1]
		    set z [lindex $itk_option(-viewAxesPosition) 2]
		    set y [expr {$y * $invAspect}]
		    set modVAP "$x $y $z"

		    Dm::drawViewAxes $vsize $rmat $modVAP \
			    $itk_option(-viewAxesSize) $itk_option(-viewAxesColor) \
			    $itk_option(-viewAxesLabelColor) $itk_option(-viewAxesLineWidth) \
			    1 $itk_option(-viewAxesTripleColor)
		}

		if {$itk_option(-modelAxesEnable)} {
		    Dm::drawModelAxes $vsize $rmat $itk_option(-modelAxesPosition) \
			    $itk_option(-modelAxesSize) $itk_option(-modelAxesColor) \
			    $itk_option(-modelAxesLabelColor) $itk_option(-modelAxesLineWidth) \
			    0 $itk_option(-modelAxesTripleColor) \
			    $model2view \
			    $itk_option(-modelAxesTickEnable) \
			    $itk_option(-modelAxesTickLength) \
			    $itk_option(-modelAxesTickMajorLength) \
			    $itk_option(-modelAxesTickInterval) \
			    $itk_option(-modelAxesTicksPerMajor) \
			    $itk_option(-modelAxesTickColor) \
			    $itk_option(-modelAxesTickMajorColor) \
			    $itk_option(-modelAxesTickThreshold)
		}
	}

	if {$itk_option(-centerDotEnable)} {
	    Dm::drawCenterDot $itk_option(-centerDotColor)
	}

    } else {
	# overlay
	Dm::refreshfb
    }
    Dm::drawEnd
}

body Display::mouse_nirt {_x _y {gi 0}} {
    set geo [lindex $geolist $gi]

    if {$geo == ""} {
	return "mouse_nirt: bad geometry index - $gi"
    }

    # transform X screen coordinates into normalized view coordinates
    set nvx [expr ($_x * $invWidth - 0.5) * 2.0]
    set nvy [expr (0.5 - $_y * $invHeight) * 2.0 * $invAspect]

    # transform normalized view coordinates into model coordinates
    set mc [mat4x3pnt [view2model] "$nvx $nvy 0"]
    set mc [vscale $mc [base2local]]

    # finally, call nirt (backing out of geometry)
    set v_obj [View::get_viewname]
    eval $geo nirt $v_obj -b $mc
}

body Display::nirt {args} {
    set len [llength $args]

    if {$len > 1 && [lindex $args 0] == "-geo"} {
	set index [lindex $args 1]
	set args [lrange $args 2 end]
	set geo [lindex $geolist $index]
    } else {
	set geo [lindex $geolist 0]
    }

    if {$geo == ""} {
	return "nirt: bad geometry index"
    }

    set v_obj [View::get_viewname]

    eval $geo nirt $v_obj $args
}

body Display::vnirt {vx vy {gi 0}} {
    set geo [lindex $geolist $gi]

    if {$geo == ""} {
	return "vnirt: bad geometry index - $gi"
    }

    # finally, call vnirt (backing out of geometry)
    set v_obj [View::get_viewname]
    eval $geo vnirt $v_obj -b $vx $vy
}

body Display::qray {args} {
    set len [llength $args]

    if {$len > 1 && [lindex $args 0] == "-geo"} {
	set index [lindex $args 1]
	set args [lrange $args 2 end]
	set geo [lindex $geolist $index]
    } else {
	set geo [lindex $geolist 0]
    }

    if {$geo == ""} {
	return "qray: bad geometry index"
    }

    eval $geo qray $args
}

body Display::rt {args} {
#    if {$itk_option(-listen) < 0} {
#	return "rt: not listening"
#    }

    set len [llength $args]

    if {$len > 1 && [lindex $args 0] == "-geo"} {
	set index [lindex $args 1]
	set args [lrange $args 2 end]
	set geo [lindex $geolist $index]
    } else {
	set geo [lindex $geolist 0]
    }

    if {$geo == ""} {
	return "rt: bad geometry index"
    }

    set v_obj [View::get_viewname]
    eval $geo rt $v_obj -F $itk_option(-listen) -w $width -n $height -V $aspect $args
}

body Display::rtabort {{gi 0}} {
    set geo [lindex $geolist $gi]

    if {$geo == ""} {
	return "rtabort: bad geometry index"
    }

    $geo rtabort
}

body Display::rtcheck {args} {
    if {$itk_option(-listen) < 0} {
	return "rtcheck: not listening"
    }

    set len [llength $args]

    if {$len > 1 && [lindex $args 0] == "-geo"} {
	set index [lindex $args 1]
	set args [lrange $args 2 end]
	set geo [lindex $geolist $index]
    } else {
	set geo [lindex $geolist 0]
    }

    if {$geo == ""} {
	return "rtcheck: bad geometry index"
    }

    set v_obj [View::get_viewname]
    eval $geo rtcheck $v_obj -F $itk_option(-listen) $args
}

body Display::rtedge {args} {
    set len [llength $args]

    if {$len > 1 && [lindex $args 0] == "-geo"} {
	set index [lindex $args 1]
	set args [lrange $args 2 end]
	set geo [lindex $geolist $index]
    } else {
	set geo [lindex $geolist 0]
    }

    if {$geo == ""} {
	return "rtedge: bad geometry index"
    }

    set v_obj [View::get_viewname]
    eval $geo rtedge $v_obj -F $itk_option(-listen) -w $width -n $height -V $aspect $args
}

body Display::autoview {{g_index 0}} {
    if {$g_index < [llength $geolist]} {
	set geo [lindex $geolist $g_index]
	set aview [$geo get_autoview]
	eval [lrange $aview 0 1]
	eval [lrange $aview 2 3]
    }
}

body Display::attach_view {} {
    View::observer attach $this
}

body Display::attach_drawable {dg} {
    $dg observer attach $this
}

body Display::detach_view {} {
    View::observer detach $this
}

body Display::detach_drawable {dg} {
    $dg observer detach $this
}

body Display::add {glist} {
    if [llength $geolist] {
	set blank 0
    } else {
	set blank 1
    }

    foreach geo $glist {
	set index [lsearch $geolist $geo]

	# already in list
	if {$index != -1} {
	    continue
	}

	lappend geolist $geo
	attach_drawable $geo
    }

    if {$blank} {
	detach_view
	autoview
	attach_view
    }

    refresh
}

body Display::remove {glist} {
    foreach geo $glist {
	set index [lsearch $geolist $geo]
	if {$index == -1} {
	    continue
	}

	set geolist [lreplace $geolist $index $index]
	detach_drawable $geo
    }

    refresh
}

body Display::contents {} {
    return $geolist
}

########################### Public Methods That Override ###########################
body Display::slew {args} {
    if {[llength $args] == 2} {
	set x1 [lindex $args 0]
	set y1 [lindex $args 1]

	set x2 [expr $width * 0.5]
	set y2 [expr $height * 0.5]
	set sf [expr 2.0 * $invWidth]

	set _x [expr ($x1 - $x2) * $sf]
	set _y [expr ($y2 - $y1) * $sf]
	View::slew $_x $_y
    } else {
	eval View::slew $args
    }
}

body Display::perspective_angle {args} {
    if {$args == ""} {
	# get perspective angle
	return $perspective_angle
    } else {
	# set perspective angle
	View::perspective $args
    }

    if {$perspective_angle > 0} {
	# turn perspective mode on
	Dm::perspective 1
    } else {
	# turn perspective mode off
	Dm::perspective 0
    }

    refresh
    return $perspective_angle
}

body Display::perspective {args} {
    eval Dm::perspective $args

    if {$itk_option(-perspective)} {
	View::perspective [lindex $perspective_angles $perspective_angle_index]
    } else {
	View::perspective -1
    }

    refresh
    return $itk_option(-perspective)
}

body Display::fb_active {args} {
    if {$args == ""} {
	return $itk_option(-fb_active)
    } else {
	eval Dm::fb_active $args
	refresh
    }
}

body Display::light {args} {
    eval Dm::light $args
    refresh
    return $itk_option(-light)
}

body Display::transparency {args} {
    eval Dm::transparency $args
    refresh
    return $itk_option(-transparency)
}

body Display::bounds {args} {
    if {$args == ""} {
	return [Dm::bounds]
    }

    eval Dm::bounds $args
    refresh
}

body Display::depthMask {args} {
    eval Dm::depthMask $args
    refresh
    return $itk_option(-depthMask)
}

body Display::zbuffer {args} {
    eval Dm::zbuffer $args
    refresh
    return $itk_option(-zbuffer)
}

body Display::zclip {args} {
    eval Dm::zclip $args
    refresh
    return $itk_option(-zclip)
}

########################### Protected Methods ###########################
body Display::toggle_modelAxesEnable {} {
    if {$itk_option(-modelAxesEnable)} {
	set itk_option(-modelAxesEnable) 0
    } else {
	set itk_option(-modelAxesEnable) 1
    }

    refresh
}

body Display::toggle_modelAxesTickEnable {} {
    if {$itk_option(-modelAxesTickEnable)} {
	set itk_option(-modelAxesTickEnable) 0
    } else {
	set itk_option(-modelAxesTickEnable) 1
    }

    refresh
}

body Display::toggle_viewAxesEnable {} {
    if {$itk_option(-viewAxesEnable)} {
	set itk_option(-viewAxesEnable) 0
    } else {
	set itk_option(-viewAxesEnable) 1
    }

    refresh
}

body Display::toggle_centerDotEnable {} {
    if {$itk_option(-centerDotEnable)} {
	set itk_option(-centerDotEnable) 0
    } else {
	set itk_option(-centerDotEnable) 1
    }

    refresh
}

body Display::? {} {
    return "[View::?][Dm::?]"
}

body Display::apropos {key} {
    return "[View::apropos $key] [Dm::apropos $key]"
}

body Display::help {args} {
    if {[llength $args] && [lindex $args 0] != {}} {
	if {[catch {eval View::help $args} result]} {
	    set result [eval Dm::help $args]
	}

	return $result
    }

    # list all help messages for QuadDisplay and Db
    return "[View::help][Dm::help]"
}

body Display::getUserCmds {} {
    eval lappend cmds [View::getUserCmds] [Dm::getUserCmds]
    return $cmds
}

body Display::toggle_zclip {} {
    Dm::toggle_zclip
    refresh
    return $itk_option(-zclip)
}

body Display::toggle_zbuffer {} {
    Dm::toggle_zbuffer
    refresh
    return $itk_option(-zbuffer)
}

body Display::toggle_light {} {
    Dm::toggle_light
    refresh
    return $itk_option(-light)
}

body Display::toggle_perspective {} {
    Dm::toggle_perspective

    if {$itk_option(-perspective)} {
	View::perspective [lindex $perspective_angles $perspective_angle_index]
    } else {
	View::perspective -1
    }

    refresh
    return $itk_option(-perspective)
}

body Display::toggle_perspective_angle {} {
    if {$perspective_angle_index == 3} {
	set perspective_angle_index 0
    } else {
	incr perspective_angle_index
    }

    if {$itk_option(-perspective)} {
	View::perspective [lindex $perspective_angles $perspective_angle_index]
    }
}

body Display::toggle_transparency {} {
    Dm::toggle_transparency
    refresh
    return $itk_option(-transparency)
}

body Display::idle_mode {} {
    # stop receiving motion events
    bind $itk_component(dm) <Motion> {}
}

body Display::rotate_mode {_x _y} {
    set x $_x
    set y $_y

    # start receiving motion events
    bind $itk_component(dm) <Motion> "[code $this handle_rotation %x %y]; break"
}

body Display::translate_mode {_x _y} {
    set x $_x
    set y $_y

    # start receiving motion events
    bind $itk_component(dm) <Motion> "[code $this handle_translation %x %y]; break"
}

body Display::scale_mode {_x _y} {
    set x $_x
    set y $_y

    # start receiving motion events
    bind $itk_component(dm) <Motion> "[code $this handle_scale %x %y]; break"
}

body Display::constrain_rmode {coord _x _y} {
    set x $_x
    set y $_y

    # start receiving motion events
    bind $itk_component(dm) <Motion> "[code $this handle_constrain_rot $coord %x %y]; break"
}

body Display::constrain_tmode {coord _x _y} {
    set x $_x
    set y $_y

    # start receiving motion events
    bind $itk_component(dm) <Motion> "[code $this handle_constrain_tran $coord %x %y]; break"
}

body Display::handle_rotation {_x _y} {
    set dx [expr ($y - $_y) * $itk_option(-rscale)]
    set dy [expr ($x - $_x) * $itk_option(-rscale)]
    vrot $dx $dy 0

    #update instance variables x and y
    set x $_x
    set y $_y
}

body Display::handle_translation {_x _y} {
    set dx [expr {($x - $_x) * $invWidth * [View::size]}]
    set dy [expr {($_y - $y) * $invWidth * [View::size]}]
    vtra $dx $dy 0

    #update instance variables x and y
    set x $_x
    set y $_y
}

body Display::handle_scale {_x _y} {
    set dx [expr {($_x - $x) * $invWidth * $itk_option(-sscale)}]
    set dy [expr {($y - $_y) * $invWidth * $itk_option(-sscale)}]

    if {[expr {abs($dx) > abs($dy)}]} {
	set f [expr 1.0 + $dx]
    } else {
	set f [expr 1.0 + $dy]
    }

    zoom $f

    #update instance variables x and y
    set x $_x
    set y $_y
}

body Display::handle_constrain_rot {coord _x _y} {
    set dx [expr {($x - $_x) * $itk_option(-rscale)}]
    set dy [expr {($_y - $y) * $itk_option(-rscale)}]

    if [expr abs($dx) > abs($dy)] {
	set f $dx
    } else {
	set f $dy
    }
    switch $coord {
	x {
	    rot $f 0 0
	}
	y {
	    rot 0 $f 0
	}
	z {
	    rot 0 0 $f
	}
    }

    #update instance variables x and y
    set x $_x
    set y $_y
}

body Display::handle_constrain_tran {coord _x _y} {
    set dx [expr {($x - $_x) * $invWidth * [View::size]}]
    set dy [expr {($_y - $y) * $invWidth * [View::size]}]

    if {[expr {abs($dx) > abs($dy)}]} {
	set f $dx
    } else {
	set f $dy
    }
    switch $coord {
	x {
	    tra $f 0 0
	}
	y {
	    tra 0 $f 0
	}
	z {
	    tra 0 0 $f
	}
    }

    #update instance variables x and y
    set x $_x
    set y $_y
}

body Display::handle_configure {} {
    Dm::handle_configure
    refresh
}

body Display::handle_expose {} {
    refresh
}

body Display::doBindings {} {
    bind $itk_component(dm) <Enter> "focus $itk_component(dm);"
    bind $itk_component(dm) <Configure> "[code $this handle_configure]; break"
    bind $itk_component(dm) <Expose> "[code $this handle_expose]; break"

    # Mouse Bindings
    bind $itk_component(dm) <1> "$this zoom 0.5; break"
    bind $itk_component(dm) <2> "$this slew %x %y; break"
    bind $itk_component(dm) <3> "$this zoom 2.0; break"

    # Idle Mode
    bind $itk_component(dm) <ButtonRelease> "[code $this idle_mode]; break"
    bind $itk_component(dm) <KeyRelease-Control_L> "[code $this idle_mode]; break"
    bind $itk_component(dm) <KeyRelease-Control_R> "[code $this idle_mode]; break"
    bind $itk_component(dm) <KeyRelease-Shift_L> "[code $this idle_mode]; break"
    bind $itk_component(dm) <KeyRelease-Shift_R> "[code $this idle_mode]; break"
    bind $itk_component(dm) <KeyRelease-Alt_L> "[code $this idle_mode]; break"
    bind $itk_component(dm) <KeyRelease-Alt_R> "[code $this idle_mode]; break"

    # Rotate Mode
    bind $itk_component(dm) <Control-ButtonPress-1> "[code $this rotate_mode %x %y]; break"
    bind $itk_component(dm) <Control-ButtonPress-2> "[code $this rotate_mode %x %y]; break"
    bind $itk_component(dm) <Control-ButtonPress-3> "[code $this rotate_mode %x %y]; break"

    # Translate Mode
    bind $itk_component(dm) <Shift-ButtonPress-1> "[code $this translate_mode %x %y]; break"
    bind $itk_component(dm) <Shift-ButtonPress-2> "[code $this translate_mode %x %y]; break"
    bind $itk_component(dm) <Shift-ButtonPress-3> "[code $this translate_mode %x %y]; break"

    # Scale Mode
    bind $itk_component(dm) <Control-Shift-ButtonPress-1> "[code $this scale_mode %x %y]; break"
    bind $itk_component(dm) <Control-Shift-ButtonPress-2> "[code $this scale_mode %x %y]; break"
    bind $itk_component(dm) <Control-Shift-ButtonPress-3> "[code $this scale_mode %x %y]; break"

    # Constrained Rotate Mode
    bind $itk_component(dm) <Alt-Control-ButtonPress-1> "[code $this constrain_rmode x %x %y]; break"
    bind $itk_component(dm) <Alt-Control-ButtonPress-2> "[code $this constrain_rmode y %x %y]; break"
    bind $itk_component(dm) <Alt-Control-ButtonPress-3> "[code $this constrain_rmode z %x %y]; break"

    # Constrained Translate Mode
    bind $itk_component(dm) <Alt-Shift-ButtonPress-1> "[code $this constrain_tmode x %x %y]; break"
    bind $itk_component(dm) <Alt-Shift-ButtonPress-2> "[code $this constrain_tmode y %x %y]; break"
    bind $itk_component(dm) <Alt-Shift-ButtonPress-3> "[code $this constrain_tmode z %x %y]; break"

    # Constrained Scale Mode
    bind $itk_component(dm) <Alt-Control-Shift-ButtonPress-1> "[code $this scale_mode %x %y]; break"
    bind $itk_component(dm) <Alt-Control-Shift-ButtonPress-2> "[code $this scale_mode %x %y]; break"
    bind $itk_component(dm) <Alt-Control-Shift-ButtonPress-3> "[code $this scale_mode %x %y]; break"

    # Key Bindings
    bind $itk_component(dm) 3 "$this ae \"35 25 0\"; break"
    bind $itk_component(dm) 4 "$this ae \"45 45 0\"; break"
    bind $itk_component(dm) f "$this ae \"0 0 0\"; break"
    bind $itk_component(dm) R "$this ae \"180 0 0\"; break"
    bind $itk_component(dm) r "$this ae \"270 0 0\"; break"
    bind $itk_component(dm) l "$this ae \"90 0 0\"; break"
    bind $itk_component(dm) t "$this ae \"0 90 0\"; break"
    bind $itk_component(dm) b "$this ae \"0 270 0\"; break"
    bind $itk_component(dm) m "[code $this toggle_modelAxesEnable]; break"
    bind $itk_component(dm) T "[code $this toggle_modelAxesTickEnable]; break"
    bind $itk_component(dm) v "[code $this toggle_viewAxesEnable]; break"
    bind $itk_component(dm) <F2> "[code $this toggle_zclip]; break"
    bind $itk_component(dm) <F3> "[code $this toggle_perspective]; break"
    bind $itk_component(dm) <F4> "[code $this toggle_zbuffer]; break"
    bind $itk_component(dm) <F5> "[code $this toggle_light]; break"
    bind $itk_component(dm) <F6> "[code $this toggle_perspective_angle]; break"
    bind $itk_component(dm) <F10> "[code $this toggle_transparency]; break"
}

body Display::resetBindings {} {
    Dm::doBindings
    doBindings
}
