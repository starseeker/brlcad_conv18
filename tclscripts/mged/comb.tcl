#
#			C O M B . T C L
#
#	Widget for editing a combination.
#
#	Author - Robert G. Parker
#

check_externs "get_comb put_comb"

proc init_comb { id } {
    global player_screen
    global mged_active_dm
    global comb_control

    set top .$id.comb

    if [winfo exists $top] {
	raise $top
	return
    }

    # set the padding
    set padx 4
    set pady 2

    set comb_control($id,name) ""
    set comb_control($id,isRegion) "Yes"
    set comb_control($id,id) ""
    set comb_control($id,air) ""
    set comb_control($id,gift) ""
    set comb_control($id,los) ""
    set comb_control($id,color) ""
    set comb_control($id,inherit) ""
    set comb_control($id,comb) ""
    set comb_control($id,shader) ""
    set comb_control($id,shader_gui) ""

    toplevel $top -screen $player_screen($id)

    frame $top.gridF
    frame $top.gridF2
    frame $top.gridF3
    frame $top.gridF4
    frame $top.nameF
    frame $top.nameFF -relief sunken -bd 2
    frame $top.idF
    frame $top.idFF -relief sunken -bd 2
    frame $top.airF
    frame $top.airFF -relief sunken -bd 2
    frame $top.giftF
    frame $top.giftFF -relief sunken -bd 2
    frame $top.losF
    frame $top.losFF -relief sunken -bd 2
    frame $top.colorF
    frame $top.colorFF -relief sunken -bd 2
    frame $top.shaderF
    frame $top.shaderFF -relief sunken -bd 2
    frame $top.combF
    frame $top.combFF -relief sunken -bd 2

    label $top.nameL -text "Name" -anchor w
    entry $top.nameE -relief flat -width 12 -textvar comb_control($id,name)
    menubutton $top.nameMB -relief raised -bd 2\
	    -menu $top.nameMB.m -indicatoron 1
    menu $top.nameMB.m -tearoff 0
    $top.nameMB.m add command -label "Select from all displayed"\
	    -command "winset \$mged_active_dm($id); build_comb_menu_all"
    $top.nameMB.m add command -label "Select along ray"\
	    -command "winset \$mged_active_dm($id); set mouse_behavior c"

    label $top.idL -text "Region Id" -anchor w
    entry $top.idE -relief flat -width 12 -textvar comb_control($id,id)

    label $top.airL -text "Air Code" -anchor w
    entry $top.airE -relief flat -width 12 -textvar comb_control($id,air)

    label $top.giftL -text "Gift Material" -anchor w
    entry $top.giftE -relief flat -width 12 -textvar comb_control($id,gift)

    label $top.losL -text "LOS" -anchor w
    entry $top.losE -relief flat -width 12 -textvar comb_control($id,los)

    label $top.colorL -text "Color" -anchor w
    entry $top.colorE -relief flat -width 12 -textvar comb_control($id,color)
    menubutton $top.colorMB -relief raised -bd 2\
	    -menu $top.colorMB.m -indicatoron 1
    menu $top.colorMB.m -tearoff 0
    $top.colorMB.m add command -label black\
	     -command "set comb_control($id,color) \"0 0 0\"; comb_set_colorMB $id"
    $top.colorMB.m add command -label white\
	     -command "set comb_control($id,color) \"220 220 220\"; comb_set_colorMB $id"
    $top.colorMB.m add command -label red\
	     -command "set comb_control($id,color) \"220 0 0\"; comb_set_colorMB $id"
    $top.colorMB.m add command -label green\
	     -command "set comb_control($id,color) \"0 220 0\"; comb_set_colorMB $id"
    $top.colorMB.m add command -label blue\
	     -command "set comb_control($id,color) \"0 0 220\"; comb_set_colorMB $id"
    $top.colorMB.m add command -label yellow\
	     -command "set comb_control($id,color) \"220 220 0\"; comb_set_colorMB $id"
    $top.colorMB.m add command -label cyan\
	    -command "set comb_control($id,color) \"0 220 220\"; comb_set_colorMB $id"
    $top.colorMB.m add command -label magenta\
	    -command "set comb_control($id,color) \"220 0 220\"; comb_set_colorMB $id"
    $top.colorMB.m add separator
    $top.colorMB.m add command -label "Color Tool..."\
	    -command "comb_choose_color $id $top"

    label $top.shaderL -text "Shader" -anchor w
    entry $top.shaderE -relief flat -width 12 -textvar comb_control($id,shader)
    menubutton $top.shaderMB -relief raised -bd 2\
	    -menu $top.shaderMB.m -indicatoron 1
    menu $top.shaderMB.m -tearoff 0
    $top.shaderMB.m add command -label plastic\
	    -command "comb_shader_gui $id plastic"
    $top.shaderMB.m add command -label mirror\
	    -command "comb_shader_gui $id mirror"
    $top.shaderMB.m add command -label glass\
	    -command "comb_shader_gui $id glass"

    label $top.combL -text "Boolean Expression:" -anchor w
    text $top.combT -relief sunken -bd 2 -width 40 -height 10\
	    -yscrollcommand "$top.gridF3.s set" -setgrid true
    scrollbar $top.gridF3.s -relief flat -command "$top.combT yview"

#    button $top.selectGiftB -relief raised -text "Select Gift Material"\
#	    -command "comb_select_gift $id"

    checkbutton $top.isRegionCB -relief raised -text "Is Region"\
	    -offvalue No -onvalue Yes -variable comb_control($id,isRegion)\
	    -command "comb_toggle_isRegion $id"
    checkbutton $top.inheritCB -relief raised -text "Inherit"\
	    -offvalue No -onvalue Yes -variable comb_control($id,inherit)

    button $top.applyB -relief raised -text "Apply"\
	    -command "comb_apply $id"
    button $top.resetB -relief raised -text "Reset"\
	    -command "comb_reset $id"
    button $top.dismissB -relief raised -text "Dismiss"\
	    -command "comb_dismiss $id $top"

    grid $top.nameL -sticky "ew" -in $top.nameF
    grid $top.nameE $top.nameMB -sticky "ew" -in $top.nameFF
    grid $top.nameFF -sticky "ew" -in $top.nameF
    grid $top.idL -sticky "ew" -in $top.idF
    grid $top.idE -sticky "ew" -in $top.idFF
    grid $top.idFF -sticky "ew" -in $top.idF
    grid $top.nameF x $top.idF -sticky "ew" -in $top.gridF -pady $pady
    grid columnconfigure $top.nameF 0 -weight 1
    grid columnconfigure $top.nameFF 0 -weight 1
    grid columnconfigure $top.idF 0 -weight 1
    grid columnconfigure $top.idFF 0 -weight 1

    grid $top.colorL -sticky "ew" -in $top.colorF
    grid $top.colorE $top.colorMB -sticky "ew" -in $top.colorFF
    grid $top.colorFF -sticky "ew" -in $top.colorF
    grid $top.airL -sticky "ew" -in $top.airF
    grid $top.airE -sticky "ew" -in $top.airFF
    grid $top.airFF -sticky "ew" -in $top.airF
    grid $top.colorF x $top.airF -sticky "ew" -in $top.gridF -pady $pady
    grid columnconfigure $top.colorF 0 -weight 1
    grid columnconfigure $top.colorFF 0 -weight 1
    grid columnconfigure $top.airF 0 -weight 1
    grid columnconfigure $top.airFF 0 -weight 1

    grid $top.shaderL -sticky "ew" -in $top.shaderF
    grid $top.shaderE $top.shaderMB -sticky "ew" -in $top.shaderFF
    grid $top.shaderFF -sticky "ew" -in $top.shaderF
    grid $top.losL -sticky "ew" -in $top.losF
    grid $top.losE -sticky "ew" -in $top.losFF
    grid $top.losFF -sticky "ew" -in $top.losF
    grid $top.shaderF x $top.losF -sticky "ew" -in $top.gridF -pady $pady
    grid columnconfigure $top.shaderF 0 -weight 1
    grid columnconfigure $top.shaderFF 0 -weight 1
    grid columnconfigure $top.losF 0 -weight 1
    grid columnconfigure $top.losFF 0 -weight 1

    grid $top.giftL -sticky "ew" -in $top.giftF
    grid $top.giftE -sticky "ew" -in $top.giftFF
    grid $top.giftFF -sticky "ew" -in $top.giftF
    grid $top.giftF x x -sticky "ew" -in $top.gridF -pady $pady
#    grid $top.selectGiftB -row 3 -column 2 -sticky "sw" -in $top.gridF -pady $pady
    grid columnconfigure $top.giftF 0 -weight 1
    grid columnconfigure $top.giftFF 0 -weight 1

    grid $top.isRegionCB $top.inheritCB x -sticky "ew" -in $top.gridF2\
	    -ipadx 4 -ipady 4

    grid columnconfigure $top.gridF 0 -weight 1
    grid columnconfigure $top.gridF 1 -minsize 20
    grid columnconfigure $top.gridF 2 -weight 1
    grid columnconfigure $top.gridF2 2 -weight 1

    grid $top.combL x -sticky "w" -in $top.gridF3
    grid $top.combT $top.gridF3.s -sticky "nsew" -in $top.gridF3
    grid rowconfigure $top.gridF3 1 -weight 1
    grid columnconfigure $top.gridF3 0 -weight 1

    grid $top.applyB x $top.resetB x $top.dismissB -sticky "ew"\
	    -in $top.gridF4 -pady $pady
    grid columnconfigure $top.gridF4 1 -weight 1
    grid columnconfigure $top.gridF4 3 -weight 1

    grid $top.gridF -sticky "ew" -padx $padx -pady $pady
    grid $top.gridF2 -sticky "ew" -padx $padx -pady $pady
    grid $top.gridF3 -sticky "nsew" -padx $padx -pady $pady
    grid $top.gridF4 -sticky "ew" -padx $padx -pady $pady
    grid rowconfigure $top 2 -weight 1
    grid columnconfigure $top 0 -weight 1

    bind $top.colorE <Return> "comb_set_colorMB $id; break"
    comb_set_colorMB $id

    bind $top.nameE <Return> "comb_reset $id; break"

    set pxy [winfo pointerxy $top]
    set x [lindex $pxy 0]
    set y [lindex $pxy 1]
    wm geometry $top +$x+$y
    wm title $top "Combination Editor ($id)"
}

proc comb_apply { id } {
    global player_screen
    global comb_control

    set top .$id.comb
    set comb_control($id,comb) [$top.combT get 0.0 end]

    if {$comb_control($id,isRegion)} {
	if {$comb_control($id,id) < 0} {
	    cad_dialog .$id.combDialog $player_screen($id)\
		    "Bad region id!"\
		    "Region id must be >= 0"\
		    "" 0 OK
	    return
	}

	if {$comb_control($id,air) < 0} {
	    cad_dialog .$id.combDialog $player_screen($id)\
		    "Bad air code!"\
		    "Air code must be >= 0"\
		    "" 0 OK
	    return
	}

	if {$comb_control($id,id) == 0 && $comb_control($id,air) == 0} {
	    cad_dialog .$id.combDialog $player_screen($id)\
		    "Warning: both region id and air code are 0"\
		    "Warning: both region id and air code are 0"\
		    "" 0 OK
	}

	set result [catch {put_comb $comb_control($id,name) $comb_control($id,isRegion)\
		$comb_control($id,id) $comb_control($id,air) $comb_control($id,gift) $comb_control($id,los)\
		$comb_control($id,color) $comb_control($id,shader) $comb_control($id,inherit)\
		$comb_control($id,comb)} comb_error]

	if {$result} {
	    return $comb_error
	}

	return
    }

    set result [catch {put_comb $comb_control($id,name) $comb_control($id,isRegion)\
	    $comb_control($id,color) $comb_control($id,shader) $comb_control($id,inherit)\
	    $comb_control($id,comb)} comb_error]

    if {$result} {
	return $comb_error
    }
}

proc comb_reset { id } {
    global player_screen
    global comb_control

    set top .$id.comb

    if {$comb_control($id,name) == ""} {
	cad_dialog .$id.combDialog $player_screen($id)\
		"You must specify a region/combination name!"\
		"You must specify a region/combination name!"\
		"" 0 OK
	return
    }

    set save_isRegion $comb_control($id,isRegion)
    set result [catch {get_comb $comb_control($id,name)} comb_defs]
    if {$result == 1} {
	cad_dialog .$id.combDialog $player_screen($id)\
		"comb_reset: Error"\
		$comb_defs\
		"" 0 OK
	return
    }

    set comb_control($id,isRegion) [lindex $comb_defs 1]

    if {$comb_control($id,isRegion) == "Yes"} {
	set comb_control($id,id) [lindex $comb_defs 2]
	set comb_control($id,air) [lindex $comb_defs 3]
	set comb_control($id,gift) [lindex $comb_defs 4]
	set comb_control($id,los) [lindex $comb_defs 5]
	set comb_control($id,color) [lindex $comb_defs 6]
	set comb_control($id,shader) [lindex $comb_defs 7]
	set comb_control($id,inherit) [lindex $comb_defs 8]
	set comb_control($id,comb) [lindex $comb_defs 9]
    } else {
	set comb_control($id,color) [lindex $comb_defs 2]
	set comb_control($id,shader) [lindex $comb_defs 3]
	set comb_control($id,inherit) [lindex $comb_defs 4]
	set comb_control($id,comb) [lindex $comb_defs 5]
    }

    set_WidgetRGBColor $top.colorMB $comb_control($id,color)
    $top.combT delete 0.0 end
    $top.combT insert end $comb_control($id,comb)

    if {$save_isRegion != $comb_control($id,isRegion)} {
	comb_toggle_isRegion $id
    }
}

proc comb_dismiss { id top } {
    global comb_control

    catch { destroy $top }
    catch { destroy $comb_control($id,shader_gui) }
}

proc comb_toggle_isRegion { id } {
    global comb_control

    set padx 4
    set pady 2

    set top .$id.comb
    grid remove $top.gridF

    if {$comb_control($id,isRegion) == "Yes"} {
	grid forget $top.nameF $top.colorF $top.shaderF

	frame $top.idF
	frame $top.idFF -relief sunken -bd 2
	frame $top.airF
	frame $top.airFF -relief sunken -bd 2
	frame $top.giftF
	frame $top.giftFF -relief sunken -bd 2
	frame $top.losF
	frame $top.losFF -relief sunken -bd 2

	label $top.idL -text "Region Id" -anchor w
	entry $top.idE -relief flat -width 12 -textvar comb_control($id,id)

	label $top.airL -text "Air Code" -anchor w
	entry $top.airE -relief flat -width 12 -textvar comb_control($id,air)

	label $top.giftL -text "Gift Material" -anchor w
	entry $top.giftE -relief flat -width 12 -textvar comb_control($id,gift)

	label $top.losL -text "LOS" -anchor w
	entry $top.losE -relief flat -width 12 -textvar comb_control($id,los)

#	button $top.selectGiftB -relief raised -text "Select Gift Material"\
#		-command "comb_select_gift $id"

	grid $top.idL -sticky "ew" -in $top.idF
	grid $top.idE -sticky "ew" -in $top.idFF
	grid $top.idFF -sticky "ew" -in $top.idF
	grid $top.nameF x $top.idF -sticky "ew" -row 0 -in $top.gridF -pady $pady
	grid columnconfigure $top.idF 0 -weight 1
	grid columnconfigure $top.idFF 0 -weight 1

	grid $top.airL -sticky "ew" -in $top.airF
	grid $top.airE -sticky "ew" -in $top.airFF
	grid $top.airFF -sticky "ew" -in $top.airF
	grid $top.colorF x $top.airF -sticky "ew" -in $top.gridF -pady $pady
	grid columnconfigure $top.airF 0 -weight 1
	grid columnconfigure $top.airFF 0 -weight 1

	grid $top.losL -sticky "ew" -in $top.losF
	grid $top.losE -sticky "ew" -in $top.losFF
	grid $top.losFF -sticky "ew" -in $top.losF
	grid $top.shaderF x $top.losF -sticky "ew" -in $top.gridF -pady $pady
	grid columnconfigure $top.losF 0 -weight 1
	grid columnconfigure $top.losFF 0 -weight 1

	grid $top.giftL -sticky "ew" -in $top.giftF
	grid $top.giftE -sticky "ew" -in $top.giftFF
	grid $top.giftFF -sticky "ew" -in $top.giftF
	grid $top.giftF x x -sticky "ew" -in $top.gridF -pady $pady
#	grid $top.selectGiftB -row 3 -column 2 -sticky "sw" -in $top.gridF -pady $pady
	grid columnconfigure $top.giftF 0 -weight 1
	grid columnconfigure $top.giftFF 0 -weight 1
    } else {
	grid forget $top.nameF $top.idF $top.airF $top.giftF $top.losF\
		$top.colorF $top.shaderF
#	grid forget $top.nameF $top.idF $top.airF $top.giftF $top.losF\
#		$top.colorF $top.shaderF $top.selectGiftB

	destroy $top.idL $top.idE
	destroy $top.airL $top.airE
	destroy $top.giftL $top.giftE
	destroy $top.losL $top.losE
#	destroy $top.selectGiftB
	destroy $top.idF $top.idFF
	destroy $top.airF $top.airFF
	destroy $top.giftF $top.giftFF
	destroy $top.losF $top.losFF

	grid $top.nameF x x -sticky "ew" -in $top.gridF -pady $pady
	grid $top.colorF x x -sticky "ew" -in $top.gridF -pady $pady
	grid $top.shaderF x x -sticky "ew" -in $top.gridF -pady $pady
    }

    grid $top.gridF
}

proc comb_choose_color { id parent } {
    global player_screen

    set child color

    cadColorWidget dialog $parent $child\
	    -title "Combination Color"\
	    -initialcolor [$parent.colorMB cget -background]\
	    -ok "comb_color_ok $id $parent $parent.$child"\
	    -cancel "cadColorWidget_destroy $parent.$child"
}

proc comb_color_ok { id parent w } {
    global comb_control

    upvar #0 $w data

    $parent.colorMB configure -bg $data(finalColor)
    set comb_control($id,color) "$data(red) $data(green) $data(blue)"

    destroy $w
    unset data
}

proc comb_set_colorMB { id } {
    global comb_control

    set top .$id.comb
    set_WidgetRGBColor $top.colorMB $comb_control($id,color)
}

proc comb_shader_gui { id shader_type } {
    global comb_control

    set current_shader_type [lindex $comb_control($id,shader) 0]

    if {$current_shader_type != $shader_type} {
	set comb_control($id,shader) $shader_type
    }

    set comb_control($id,shader_gui) [do_shader comb_control($id,shader) $id]
}

#proc comb_select_gift { id } {
#    global player_screen
#    global comb_control
#
#    set top .$id.sel_gift
#
#    if [winfo exists $top] {
#	raise $top
#	return
#    }
#
#    set padx 4
#    set pady 2
#
#    toplevel $top -screen $player_screen($id)
#
#    frame $top.gridF
#    frame $top.gridF2
#    frame $top.gridF3 -relief groove -bd 2
#
#    listbox $top.giftLB -selectmode single -yscrollcommand "$top.giftS set"
#    scrollbar $top.giftS -relief flat -command "$top.giftLB yview"
#
#    label $top.giftL -text "Gift List:" -anchor w
#    entry $top.giftE -width 12 -textvar comb_control($id,gift_list)
#
#    button $top.resetB -relief raised -text "Reset"\
#	    -command "load_gift_material $id"
#    button $top.dismissB -relief raised -text "Dismiss"\
#	    -command "catch { destroy $top }"
#
#    grid $top.giftLB $top.giftS -sticky "nsew" -in $top.gridF
#    grid rowconfigure $top.gridF 0 -weight 1
#    grid columnconfigure $top.gridF 0 -weight 1
#
#    grid $top.giftL x -sticky "ew" -in $top.gridF2
#    grid $top.giftE $top.resetB -sticky "nsew" -in $top.gridF2
#    grid columnconfigure $top.gridF2 0 -weight 1
#
#    grid $top.dismissB -in $top.gridF3 -pady $pady
#
#    grid $top.gridF -sticky "nsew" -padx $padx -pady $pady
#    grid $top.gridF2 -sticky "ew" -padx $padx -pady $pady
#    grid $top.gridF3 -sticky "ew"
#    grid rowconfigure $top 0 -weight 1
#    grid columnconfigure $top 0 -weight 1
#
#    wm title $top "Select Gift Material"
#}
#
#proc load_gift_material { id } {
#    global comb_control
#}