#
#			A D C . T C L
#
# Author -
#	Robert G. Parker
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
# Description -
#	Control Panel for the Angle/Distance Cursor
#

proc init_adc_control { id } {
    global player_screen
    global mged_active_dm
    global mged_adc_control

    winset $mged_active_dm($id)
    set top .$id.adc_control

    if [winfo exists $top] {
	raise $top
	set mged_adc_control($id) 1

	return
    }

    set padx 4
    set pady 4

    if ![info exists mged_adc_control($id,coords)] {
	set mged_adc_control($id,coords) model
	adc reset
    }

    set mged_adc_control($id,last_coords) $mged_adc_control($id,coords)

    if ![info exists mged_adc_control($id,interpval)] {
	set mged_adc_control($id,interpval) abs
    }

    toplevel $top -screen $player_screen($id)

    frame $top.gridF1
    menubutton $top.coordsMB -textvariable mged_adc_control($id,coords_text)\
	    -menu $top.coordsMB.m -indicatoron 1
    menu $top.coordsMB.m -title "Coordinates" -tearoff 0
    $top.coordsMB.m add radiobutton -value model -variable mged_adc_control($id,coords)\
	    -label "Model" -command "adc_adjust_coords $id"
    hoc_register_menu_data "Coordinates" "Model" "Model Coordinates"\
	    { { summary "Set coordinate system type to model.
The model coordinate system is the
coordinate system that the MGED
database lives in." } }
    $top.coordsMB.m add radiobutton -value grid -variable mged_adc_control($id,coords)\
	    -label "Grid" -command "adc_adjust_coords $id"
    hoc_register_menu_data "Coordinates" "Grid" "Grid Coordinates"\
	    { { summary "Set coordinate system type to grid.
The grid coordinate system is 2D and
lives in the view plane. The origin
of this system is located by projecting
the model origin onto the view plane." } }
    menubutton $top.interpvalMB -textvariable mged_adc_control($id,interpval_text)\
	    -menu $top.interpvalMB.m -indicatoron 1
    hoc_register_data $top.interpvalMB "Value Interpretation Type"\
	    { { summary "This is a menu of the interpretation types.
There are two interpretation types - absolute
and relative. With absolute, the value is used
as is. However, with relative the value is treated
as an offset." } }
    menu $top.interpvalMB.m -title "Interpretation" -tearoff 0
    $top.interpvalMB.m add radiobutton -value abs -variable mged_adc_control($id,interpval)\
	    -label "Absolute" -command "adc_interpval $id"
    hoc_register_menu_data "Interpretation" "Absolute" "Interpret as absolute."\
	    { { summary "Interpret values at face value." } }
    $top.interpvalMB.m add radiobutton -value rel -variable mged_adc_control($id,interpval)\
	    -label "Relative" -command "adc_interpval $id"
    hoc_register_menu_data "Interpretation" "Relative" "Interpret as relative."\
	    { { summary "Interpret values as relative to current ADC values." } }
    grid $top.coordsMB x $top.interpvalMB x -sticky "nw" -in $top.gridF1 -padx $padx
    grid columnconfigure $top.gridF1 3 -weight 1

    frame $top.gridF2 -relief groove -bd 2
    frame $top.posF
    frame $top.tickF
    frame $top.a1F
    frame $top.a2F
    label $top.posL -text "Position" -anchor w
    entry $top.posE -relief sunken -textvar mged_adc_control($id,pos)

    set hoc_data { { summary "The tick distance indicates the distance (local units)
from the ADC position to one of its ticks." } }
    label $top.tickL -text "Tick Distance" -anchor w
    hoc_register_data $top.tickL "Tick Distance" $hoc_data
    entry $top.tickE -relief sunken -width 15 -textvar mged_adc_control($id,dst)
    hoc_register_data $top.tickE "Tick Distance" $hoc_data

    set hoc_data { { summary "Angle 1 is one of two axes used to measure angles." } }
    label $top.a1L -text "Angle 1" -anchor w
    hoc_register_data $top.a1L "Angle 1" $hoc_data
    entry $top.a1E -relief sunken -width 15 -textvar mged_adc_control($id,a1)
    hoc_register_data $top.a1E "Angle 1" $hoc_data

    set hoc_data { { summary "Angle 2 is one of two axes used to measure angles." } }
    label $top.a2L -text "Angle 2" -anchor w
    hoc_register_data $top.a2L "Angle 2" $hoc_data
    entry $top.a2E -relief sunken -width 15 -textvar mged_adc_control($id,a2)
    hoc_register_data $top.a2E "Angle 2" $hoc_data

    grid $top.posL -sticky "ew" -in $top.posF
    grid $top.posE -sticky "ew" -in $top.posF
    grid columnconfigure $top.posF 0 -weight 1
    grid $top.a1L -sticky "ew" -in $top.a1F
    grid $top.a1E -sticky "ew" -in $top.a1F
    grid columnconfigure $top.a1F 0 -weight 1
    grid $top.a2L -sticky "ew" -in $top.a2F
    grid $top.a2E -sticky "ew" -in $top.a2F
    grid columnconfigure $top.a2F 0 -weight 1
    grid $top.tickL -sticky "ew" -in $top.tickF
    grid $top.tickE -sticky "ew" -in $top.tickF
    grid columnconfigure $top.tickF 0 -weight 1
    grid $top.posF - - -sticky "ew" -in $top.gridF2 -padx $padx -pady $pady
    grid $top.tickF $top.a1F $top.a2F -sticky "ew" -in $top.gridF2 -padx $padx -pady $pady
    grid columnconfigure $top.gridF2 0 -weight 1
    grid columnconfigure $top.gridF2 1 -weight 1
    grid columnconfigure $top.gridF2 2 -weight 1

    frame $top.gridF3 -relief groove -bd 2
    frame $top.anchor_xyzF
    frame $top.anchor_tickF
    frame $top.anchor_a1F
    frame $top.anchor_a2F
    label $top.anchorL -text "Anchor Points"
    label $top.enableL -text "Enable"
    hoc_register_data $top.enableL "Enable"\
	    { { summary "The \"Enable\" checkbuttons are used
to toggle anchoring for the participating
ADC attributes." } }
    label $top.anchor_xyzL -text "Position" -anchor w
    entry $top.anchor_xyzE -relief sunken -textvar mged_adc_control($id,pos)
    checkbutton $top.anchor_xyzCB -relief flat\
	    -offvalue 0 -onvalue 1 -variable mged_adc_control($id,anchor_pos)
    hoc_register_data $top.anchor_xyzCB "Anchor Position"\
	    { { summary "Toggle anchoring of the ADC position.
If anchoring is enabled, the ADC will remain
positioned at the anchor point. So if the view
changes while the ADC position is anchored, the
ADC will move with respect to the view." } }
    label $top.anchor_tickL -text "Tick Distance" -anchor w
    entry $top.anchor_tickE -relief sunken -textvar mged_adc_control($id,anchor_pt_dst)
    checkbutton $top.anchor_tickCB -relief flat\
	    -offvalue 0 -onvalue 1 -variable mged_adc_control($id,anchor_dst)
    hoc_register_data $top.anchor_tickCB "Tick Distance Anchor Point"\
	    { { summary "Toggle anchoring of the tick distance.
If anchoring is enabled, the tick is drawn at
a distance from the ADC center position that is
equal to the distance between the ADC center
position and the anchor point." } }
    label $top.anchor_a1L -text "Angle 1" -anchor w
    entry $top.anchor_a1E -relief sunken -textvar mged_adc_control($id,anchor_pt_a1)
    checkbutton $top.anchor_a1CB -relief flat\
	    -offvalue 0 -onvalue 1 -variable mged_adc_control($id,anchor_a1)
    hoc_register_data $top.anchor_a1CB "Angle 1 Anchor Point"\
	    { { summary "Toggle anchoring of angle 1. If anchoring
is enabled, angle 1 is always drawn through
its anchor point." } }
    label $top.anchor_a2L -text "Angle 2" -anchor w
    entry $top.anchor_a2E -relief sunken -textvar mged_adc_control($id,anchor_pt_a2)
    checkbutton $top.anchor_a2CB -relief flat\
	    -offvalue 0 -onvalue 1 -variable mged_adc_control($id,anchor_a2)
    hoc_register_data $top.anchor_a2CB "Angle 2 Anchor Point"\
	    { { summary "Toggle anchoring of angle 2. If anchoring
is enabled, angle 2 is always drawn through
its anchor point." } }
    grid $top.anchor_xyzL -sticky "ew" -in $top.anchor_xyzF
    grid $top.anchor_xyzE -sticky "ew" -in $top.anchor_xyzF
    grid $top.anchor_tickL -sticky "ew" -in $top.anchor_tickF
    grid $top.anchor_tickE -sticky "ew" -in $top.anchor_tickF
    grid $top.anchor_a1L -sticky "ew" -in $top.anchor_a1F
    grid $top.anchor_a1E -sticky "ew" -in $top.anchor_a1F
    grid $top.anchor_a2L -sticky "ew" -in $top.anchor_a2F
    grid $top.anchor_a2E -sticky "ew" -in $top.anchor_a2F
    grid $top.anchorL $top.enableL -sticky "ew" -in $top.gridF3 -padx $padx
    grid $top.anchor_xyzF $top.anchor_xyzCB -sticky "ew" -in $top.gridF3 -padx $padx -pady $pady
    grid $top.anchor_tickF $top.anchor_tickCB -sticky "ew" -in $top.gridF3 -padx $padx -pady $pady
    grid $top.anchor_a1F $top.anchor_a1CB -sticky "ew" -in $top.gridF3 -padx $padx -pady $pady
    grid $top.anchor_a2F $top.anchor_a2CB -sticky "ew" -in $top.gridF3 -padx $padx -pady $pady
    grid columnconfigure $top.anchor_xyzF 0 -weight 1
    grid columnconfigure $top.anchor_tickF 0 -weight 1
    grid columnconfigure $top.anchor_a1F 0 -weight 1
    grid columnconfigure $top.anchor_a2F 0 -weight 1
    grid columnconfigure $top.gridF3 0 -weight 1

    frame $top.gridF4
    checkbutton $top.drawB -relief flat -text "Draw"\
	    -offvalue 0 -onvalue 1 -variable mged_adc_control($id,draw)
    hoc_register_data $top.drawB "Draw"\
	    { { summary "Toggle drawing of the angle distance cursor." } }
    grid $top.drawB -in $top.gridF4

    frame $top.gridF5
    button $top.okB -relief raised -text "Ok"\
	    -command "mged_apply $id \"adc_ok $id $top\""
    hoc_register_data $top.okB "Ok"\
	    { { summary "Apply the values in the ADC control panel
to the angle distance cursor then close
the control panel."} }
    button $top.applyB -relief raised -text "Apply"\
	    -command "mged_apply $id \"adc_apply $id\""
    hoc_register_data $top.applyB "Apply"\
	    { { summary "Apply the values in the ADC control panel
to the angle distance cursor." } }
    button $top.resetB -relief raised -text "Reset"\
	    -command "mged_apply $id \"adc_reset $id\""
    hoc_register_data $top.resetB "Reset"\
	    { { summary "Reset the angle distance cursor to its
default values." } }
    button $top.loadB -relief raised
    button $top.dismissB -relief raised -text "Dismiss"\
	    -command "catch { destroy $top; set mged_adc_control($id) 0 }"
    hoc_register_data $top.dismissB "Dismiss"\
	    { { summary "Dismiss/close the ADC control panel." } }
    grid $top.okB $top.applyB x $top.resetB $top.loadB x $top.dismissB -sticky "ew" -in $top.gridF5
    grid columnconfigure $top.gridF5 2 -weight 1
    grid columnconfigure $top.gridF5 5 -weight 1

    grid $top.gridF1 -sticky "ew" -padx $padx -pady $pady
    grid $top.gridF2 -sticky "ew" -padx $padx -pady $pady
    grid $top.gridF3 -sticky "ew" -padx $padx -pady $pady
    grid $top.gridF4 -sticky "ew" -padx $padx -pady $pady
    grid $top.gridF5 -sticky "ew" -padx $padx -pady $pady
    grid columnconfigure $top 0 -weight 1

    adc_interpval $id
    adc_adjust_coords $id

    set pxy [winfo pointerxy $top]
    set x [lindex $pxy 0]
    set y [lindex $pxy 1]

    wm protocol $top WM_DELETE_WINDOW "catch { destroy $top; set mged_adc_control($id) 0 }"
    wm geometry $top +$x+$y
    wm title $top "ADC Control Panel ($id)"
}

proc adc_ok { id top } {
    global mged_adc_control

    adc_apply $id
    catch { destroy $top }

    set mged_adc_control($id) 0
}

proc adc_apply { id } {
    global mged_active_dm
    global mged_adc_control

    winset $mged_active_dm($id)

    adc anchor_pos 0
    adc anchor_a1 0
    adc anchor_a2 0
    adc anchor_dst 0
    adc draw $mged_adc_control($id,draw)

    switch $mged_adc_control($id,interpval) {
	abs {
	    adc a1 $mged_adc_control($id,a1)
	    adc a2 $mged_adc_control($id,a2)
	    adc dst $mged_adc_control($id,dst)

	    adc_apply_abs $id
	}
	rel {
	    adc -i a1 $mged_adc_control($id,a1)
	    adc -i a2 $mged_adc_control($id,a2)
	    adc -i dst $mged_adc_control($id,dst)

	    adc_apply_rel $id
	}
    }

    switch $mged_adc_control($id,coords) {
	model {
	    if {$mged_adc_control($id,anchor_pos)} {
		adc anchor_pos 1
	    }
	}
	grid {
	    if {$mged_adc_control($id,anchor_pos)} {
		adc anchor_pos 2
	    }
	}
    }
    adc anchor_a1 $mged_adc_control($id,anchor_a1)
    adc anchor_a2 $mged_adc_control($id,anchor_a2)
    adc anchor_dst $mged_adc_control($id,anchor_dst)

    if {$mged_adc_control($id,interpval) == "abs"} {
	adc_load $id
    }
}

proc adc_apply_abs { id } {
    global mged_active_dm
    global mged_adc_control

    winset $mged_active_dm($id)

    switch $mged_adc_control($id,coords) {
	model {
	    eval adc xyz $mged_adc_control($id,pos)
	    eval adc anchorpoint_dst $mged_adc_control($id,anchor_pt_dst)
	    eval adc anchorpoint_a1 $mged_adc_control($id,anchor_pt_a1)
	    eval adc anchorpoint_a2 $mged_adc_control($id,anchor_pt_a2)
	}
	grid {
	    eval adc hv $mged_adc_control($id,pos)
	    eval adc anchorpoint_dst [eval grid2model_lu $mged_adc_control($id,anchor_pt_dst)]
	    eval adc anchorpoint_a1 [eval grid2model_lu $mged_adc_control($id,anchor_pt_a1)]
	    eval adc anchorpoint_a2 [eval grid2model_lu $mged_adc_control($id,anchor_pt_a2)]
	}
    }
}

proc adc_apply_rel { id } {
    global mged_active_dm
    global mged_adc_control

    winset $mged_active_dm($id)

    switch $mged_adc_control($id,coords) {
	model {
	    eval adc -i xyz $mged_adc_control($id,pos)
	    eval adc -i anchorpoint_dst $mged_adc_control($id,anchor_pt_dst)
	    eval adc -i anchorpoint_a1 $mged_adc_control($id,anchor_pt_a1)
	    eval adc -i anchorpoint_a2 $mged_adc_control($id,anchor_pt_a2)
	}
	grid {
	    eval adc -i xyz [eval view2model_vec $mged_adc_control($id,pos) 0.0]
	    eval adc -i anchorpoint_dst [eval view2model_vec $mged_adc_control($id,anchor_pt_dst) 0.0]
	    eval adc -i anchorpoint_a1 [eval view2model_vec $mged_adc_control($id,anchor_pt_a1) 0.0]
	    eval adc -i anchorpoint_a2 [eval view2model_vec $mged_adc_control($id,anchor_pt_a2) 0.0]
	}
    }
}

proc adc_reset { id } {
    global mged_active_dm
    global mged_adc_control

    winset $mged_active_dm($id)
    adc reset

    if {$mged_adc_control($id,interpval) == "abs"} {
	adc_load $id
    }
}

proc adc_load { id } {
    global mged_active_dm
    global mged_adc_control

    winset $mged_active_dm($id)

    set mged_adc_control($id,draw) [adc draw]
    set mged_adc_control($id,dst) [format "%.4f" [adc dst]]
    set mged_adc_control($id,a1) [format "%.4f" [adc a1]]
    set mged_adc_control($id,a2) [format "%.4f" [adc a2]]
    set mged_adc_control($id,anchor_dst) [adc anchor_dst]
    set mged_adc_control($id,anchor_a1) [adc anchor_a1]
    set mged_adc_control($id,anchor_a2) [adc anchor_a2]
    set mged_adc_control($id,anchor_pos) [adc anchor_pos]

    if {$mged_adc_control($id,anchor_pos) > 1} {
	set mged_adc_control($id,anchor_pos) 1
    }

    switch $mged_adc_control($id,coords) {
	model {
	    set mged_adc_control($id,pos) [eval format \"%.4f %.4f %.4f\" [adc xyz]]
	    set mged_adc_control($id,anchor_pt_dst) [eval format \"%.4f %.4f %.4f\" [adc anchorpoint_dst]]
	    set mged_adc_control($id,anchor_pt_a1) [eval format \"%.4f %.4f %.4f\" [adc anchorpoint_a1]]
	    set mged_adc_control($id,anchor_pt_a2) [eval format \"%.4f %.4f %.4f\" [adc anchorpoint_a2]]
	}
	grid {
	    set mged_adc_control($id,pos) [eval format \"%.4f %.4f\" [adc hv]]
	    set mged_adc_control($id,anchor_pt_dst) [eval format \"%.4f %.4f\" [eval model2grid_lu [adc anchorpoint_dst]]]
	    set mged_adc_control($id,anchor_pt_a1) [eval format \"%.4f %.4f\" [eval model2grid_lu [adc anchorpoint_a1]]]
	    set mged_adc_control($id,anchor_pt_a2) [eval format \"%.4f %.4f\" [eval model2grid_lu [adc anchorpoint_a2]]]
	}
    }
}

proc convert_coords { id } {
    global mged_active_dm
    global mged_adc_control

    if {$mged_adc_control($id,coords) == $mged_adc_control($id,last_coords)} {
	return
    }

    winset $mged_active_dm($id)

    switch $mged_adc_control($id,coords) {
	model {
	    if { $mged_adc_control($id,last_coords) == "grid" } {
		set mged_adc_control($id,pos) [eval format \"%.4f %.4f %.4f\"\
			[eval grid2model_lu $mged_adc_control($id,pos)]]
		set mged_adc_control($id,anchor_pt_dst) [eval format \"%.4f %.4f %.4f\"\
			[eval grid2model_lu $mged_adc_control($id,anchor_pt_dst)]]
		set mged_adc_control($id,anchor_pt_a1) [eval format \"%.4f %.4f %.4f\"\
			[eval grid2model_lu $mged_adc_control($id,anchor_pt_a1)]]
		set mged_adc_control($id,anchor_pt_a2) [eval format \"%.4f %.4f %.4f\"\
			[eval grid2model_lu $mged_adc_control($id,anchor_pt_a2)]]
	    }
	}
	grid {
	    if { $mged_adc_control($id,last_coords) == "model" } {
		set mged_adc_control($id,pos) [eval format \"%.4f %.4f\"\
			[eval model2grid_lu $mged_adc_control($id,pos)]]
		set mged_adc_control($id,anchor_pt_dst) [eval format \"%.4f %.4f\"\
			[eval model2grid_lu $mged_adc_control($id,anchor_pt_dst)]]
		set mged_adc_control($id,anchor_pt_a1) [eval format \"%.4f %.4f\"\
			[eval model2grid_lu $mged_adc_control($id,anchor_pt_a1)]]
		set mged_adc_control($id,anchor_pt_a2) [eval format \"%.4f %.4f\"\
			[eval model2grid_lu $mged_adc_control($id,anchor_pt_a2)]]
	    }
	}
    }

    set mged_adc_control($id,last_coords) $mged_adc_control($id,coords)
}

proc adc_adjust_coords { id } {
    global mged_adc_control

    set top .$id.adc_control

    switch $mged_adc_control($id,coords) {
	model {
	    set mged_adc_control($id,coords_text) "Model Coords"
	    hoc_register_data $top.coordsMB "Coordinate System Type"\
		    { { summary "This is a menu of the two coordinate system
types - model and grid. The current coordinate
system type is model. This is the coordinate system
type that the MGED database lives in." } }

            set hoc_data { { summary "Indicates the angle distance cursor's position
in model coordinates (local units)." } }
            hoc_register_data $top.posL "ADC Position" $hoc_data
            hoc_register_data $top.posE "ADC Position" $hoc_data

            hoc_register_data $top.anchorL "Anchor Points"\
		    { { summary "Anchor points are currently used to \"anchor\"
certain ADC attributes to a point in model space.
For example, if the ADC position is anchored to the
model origin, the angle distance cursor will always be
drawn with its center at the model origin. The following
four attributes can be anchored: position, tick distance,
angle1 and angle2." } }

            set hoc_data { { summary "Indicates the angle distance cursor's position
in model coordinates (local units). If the
ADC position is anchored, the ADC will remain
positioned at the anchor point. So if the view
changes while the ADC position is anchored, the
ADC will move with respect to the view." } }
            hoc_register_data $top.anchor_xyzL "Position" $hoc_data
            hoc_register_data $top.anchor_xyzE "Position" $hoc_data

            set hoc_data { { summary "If anchoring, the tick is drawn at
a distance from the ADC center position
that is equal to the distance between the
ADC center position and the anchor point.
Note - the anchor point is currently specified
in model coordinates (local units)." } }
            hoc_register_data $top.anchor_tickL "Tick Distance Anchor Point" $hoc_data
            hoc_register_data $top.anchor_tickE "Tick Distance Ancor Point" $hoc_data

            set hoc_data { { summary "If anchoring is enabled, then angle 1 is always
drawn through its anchor point. Note - the
anchor point is currently specified in model
coordinates (local units)." } }
            hoc_register_data $top.anchor_a1L "Angle 1 Anchor Point" $hoc_data
            hoc_register_data $top.anchor_a1E "Angle 1 Anchor Point" $hoc_data

            set hoc_data { { summary "If anchoring is enabled, then angle 2 is always
drawn through its anchor point. Note - the
anchor point is currently specified in model
coordinates (local units)." } }
            hoc_register_data $top.anchor_a2L "Angle 2 Anchor Point" $hoc_data
            hoc_register_data $top.anchor_a2E "Angle 2 Anchor Point" $hoc_data
	}
	grid {
	    set mged_adc_control($id,coords_text) "Grid Coords"
	    hoc_register_data $top.coordsMB "Coordinate System Type"\
		    { { summary "This is a menu of the two coordinate system
types - model and grid. The current coordinate
system type is grid. This coordinate system is
2D and lives in the view plane. The origin of
this system is located by projecting the model
origin onto the view plane." } }

            set hoc_data { { summary "Indicates the angle distance cursor's position
in grid coordinates (local units)." } }
            hoc_register_data $top.posL "ADC Position" $hoc_data
            hoc_register_data $top.posE "ADC Position" $hoc_data

            hoc_register_data $top.anchorL "Anchor Points"\
		    { { summary "Anchor points are currently used to \"anchor\"
certain ADC attributes to a point in grid space.
For example, if the ADC position is anchored to the
grid origin, the angle distance cursor will always be
drawn with its center at the grid origin. The following
four attributes can be anchored: position, tick distance,
angle1 and angle2." } }

            set hoc_data { { summary "Indicates the angle distance cursor's position
in grid coordinates (local units)." } }
            hoc_register_data $top.anchor_xyzL "Position" $hoc_data
            hoc_register_data $top.anchor_xyzE "Position" $hoc_data

            set hoc_data { { summary "If anchoring is enabled, the tick is always
drawn at a distance from the ADC center
position that is equal to the distance between the
the ADC center position and the anchor point.
Note - the anchor point is currently specified
in grid coordinates (local units)." } }
            hoc_register_data $top.anchor_tickL "Tick Distance Anchor Point" $hoc_data
	    hoc_register_data $top.anchor_tickE "Tick Distance Ancor Point" $hoc_data

            set hoc_data { { summary "If anchoring is enabled, then angle 1 is always
drawn through its anchor point. Note - the
anchor point is currently specified in grid
coordinates (local units)." } }
            hoc_register_data $top.anchor_a1L "Angle 1 Anchor Point" $hoc_data
            hoc_register_data $top.anchor_a1E "Angle 1 Anchor Point" $hoc_data

            set hoc_data { { summary "If anchoring is enabled, then angle 2 is always
drawn through its anchor point. Note - the
anchor point is currently specified in grid
coordinates (local units)." } }
            hoc_register_data $top.anchor_a2L "Angle 2 Anchor Point" $hoc_data
            hoc_register_data $top.anchor_a2E "Angle 2 Anchor Point" $hoc_data
	}
    }

    if {$mged_adc_control($id,interpval) == "abs"} {
	convert_coords $id
    } else {
	adc_clear $id
    }
}

proc adc_interpval { id } {
    global mged_adc_control

    set top .$id.adc_control

    switch $mged_adc_control($id,interpval) {
	abs {
	    $top.loadB configure -text "Load"\
		    -command "mged_apply $id \"adc_load $id\""
	    set mged_adc_control($id,interpval_text) "Absolute"
	    adc_load $id

	    hoc_register_data $top.loadB "Load"\
		    { { summary "Load the ADC control panel with
values from the angle distance cursor." } }
	}
	rel {
	    $top.loadB configure -text "Clear"\
		    -command "mged_apply $id \"adc_clear $id\""
	    set mged_adc_control($id,interpval_text) "Relative"
	    adc_clear $id

	    hoc_register_data $top.loadB "Clear"\
		    { { summary "Clear all relative values to zero." } }
	}
    }
}

proc adc_clear { id } {
    global mged_adc_control

    if {$mged_adc_control($id,coords) == "grid"} {
	set mged_adc_control($id,pos) "0.0 0.0"
	set mged_adc_control($id,anchor_pt_dst) "0.0 0.0"
	set mged_adc_control($id,anchor_pt_a1) "0.0 0.0"
	set mged_adc_control($id,anchor_pt_a2) "0.0 0.0"
    } else {
	set mged_adc_control($id,pos) "0.0 0.0 0.0"
	set mged_adc_control($id,anchor_pt_dst) "0.0 0.0 0.0"
	set mged_adc_control($id,anchor_pt_a1) "0.0 0.0 0.0"
	set mged_adc_control($id,anchor_pt_a2) "0.0 0.0 0.0"
    }

    set mged_adc_control($id,a1) "0.0"
    set mged_adc_control($id,a2) "0.0"
    set mged_adc_control($id,dst) "0.0"

    set mged_adc_control($id,anchor_pos) 0
    set mged_adc_control($id,anchor_a1) 0
    set mged_adc_control($id,anchor_a2) 0
    set mged_adc_control($id,anchor_dst) 0
}
