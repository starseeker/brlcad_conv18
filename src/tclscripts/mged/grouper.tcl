#                        G R O U P E R . T C L
# BRL-CAD
#
# Copyright (c) 2005-2012 United States Government as represented by
# the U.S. Army Research Laboratory.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this file; see the file named COPYING for more
# information.
#
###
#
# Allows primitives to be added or removed from a group by selecting 
# them on the MGED display using a selection box (rubber band).  
# Drawing the selection box (left to right), only primitives completely
# in the box are selected. Drawing the selection box (right to left),
# primitives completely and partly in the box are selected. Primitives
# are identified by the location of their vertices on the display.
# For example, a portion of a primitive may be within the selection box
# but if none of its vertices are then it is not considered within the
# section box. To start grouper use the command 'grouper' or 'gr' and
# to finish use 'done_grouper' or 'dg'. The center mouse button is used
# to create the selection box. Primitives in the group will be
# displayed in yellow.
#

set GroupNameGlobal ""
set GrouperRunning 0

proc gr_remdup { args } {
    if { [llength $args] != 1 } {
	puts stdout "Usage: gr_remdup collection"
	return
    }
    set collection [lindex $args 0]

    if { [lindex [get $collection] 0 ] != "comb" } {
	puts stdout "This is not a collection!"
	return
    }
    set collData [lt $collection]

    foreach row $collData {
	set rowBool [lindex $row 0]
	set rowName [lindex $row 1]
	if { $rowBool == "u" } {
	    rm $collection $rowName
	    g $collection $rowName
	}
    }
}


proc gr_getObjInRectangle {} {
    set size [dm size]
    set sizeX [lindex $size 0].0
    set sizeY [lindex $size 1].0
    set adjX [expr {($sizeX / 2.0) - 1.0}]
    set adjY [expr {($sizeY / 2.0) - 1.0}]
    set dim [rset r dim]     
    set dimX [lindex $dim 0].0     
    set dimY [lindex $dim 1].0    

    if { $dimX == 0.0 || $dimY == 0.0 } {
	return "no_result"
    }

    set pos [rset r pos]     
    set posX [lindex $pos 0].0     
    set posY [lindex $pos 1].0  
    set posX [expr ($posX - $adjX) / $adjX]
    set posY [expr ($posY - $adjY) / $adjY]
    set dimX [expr ($dimX / $adjX)]
    set dimY [expr ($dimY / $adjY)]

    if { $dimX > 0 } {
	# Rectangle was created left-to-right. Select only
	# objects completely in the rectangle.
	set objs [select $posX $posY $dimX $dimY]
    } else {
	# Rectangle was created right-to-left. Select everything
	# completely and partly in the rectangle.
	set objs [select -p $posX $posY $dimX $dimY]
    }

    if { $objs == "" } {
	return "no_result"
    }

    foreach obj $objs {
	set obj2 [file tail $obj]
	lappend objs2 $obj2
    }

    return $objs2
}


proc gr { GroupName Boolean { ListLimit 0 } } {
    grouper $GroupName $Boolean $ListLimit
}


proc grouper { GroupName Boolean { ListLimit 0 } } {
    global mged_gui mged_default mged_players
    global GrouperRunning

    if {$GrouperRunning == 1} {
	return "Grouper already running, use the 'dg' command to exit grouper before restarting."
    }

    set GrouperRunning 1

    for {set i 0} {1} {incr i} {
	set id [subst $mged_default(id)]_$i
	if { [lsearch -exact $mged_players $id] != -1 } {
	    break;
	}
    }

    if {$Boolean != "+" && $Boolean != "-" } {
	set GrouperRunning 0
	return "Please provide a Boolean of either + or -"
    }
    set mged_gui($id,mouse_behavior) p     
    set_mouse_behavior $id     
    rset vars fb 0     
    rset r draw 1     

    bind $mged_gui($id,active_dm) <ButtonRelease-2> " winset $mged_gui($id,active_dm); dm idle; do_grouper $GroupName $Boolean $ListLimit" 
    bind $mged_gui($id,active_dm) <Control-ButtonRelease-2> " winset $mged_gui($id,active_dm); dm idle; done_grouper" 

    # highlight in yellow current group
    if { [search -name $GroupName] != "" } {
	e -C255/255/0 $GroupName
    }
}


proc do_grouper { GroupName Boolean ListLimit } {

    uplevel #0 set GroupNameGlobal $GroupName
    set objs [gr_getObjInRectangle]

    if { $objs == "no_result" } {
	puts stdout "Nothing selected."
	return
    }

    set objs [lsort -unique $objs]
    set tot_obj_in_rect [llength $objs]

    if { [search -name $GroupName] == "" } {
	set grp_obj_list_before ""
	set grp_obj_list_len_before 0
    } else {
	set grp_obj_list_before [search $GroupName]
	set grp_obj_list_len_before [expr [llength $grp_obj_list_before] - 1]
    }
	
    if { $Boolean == "+" } {
	foreach obj $objs {
	    g $GroupName $obj
	}
    } else {
	foreach obj $objs {
	    catch {rm $GroupName $obj} tmp_msg
	}
    }
	
    if { ([search -name $GroupName] != "") && ($Boolean == "+") } {
	gr_remdup $GroupName
    }

    if { [search -name $GroupName] == "" } {
	set grp_obj_list_after ""
	set grp_obj_list_len_after 0
    } else {
	set grp_obj_list_after [search $GroupName]
	set grp_obj_list_len_after [expr [llength $grp_obj_list_after] - 1]
    }

    if { $grp_obj_list_len_after > $grp_obj_list_len_before } {
	set objcnt_chng [expr $grp_obj_list_len_after - $grp_obj_list_len_before]
	puts stdout "$objcnt_chng of $tot_obj_in_rect selected objects added to group '$GroupName'."
    } elseif { $grp_obj_list_len_after < $grp_obj_list_len_before } {
	set objcnt_chng [expr $grp_obj_list_len_before - $grp_obj_list_len_after]
	puts stdout "$objcnt_chng of $tot_obj_in_rect selected objects removed from group '$GroupName'."
    } else {
	puts stdout "No change to group '$GroupName'."
    }

    set objcnt 0
    if { $ListLimit > 0 } {
	puts stdout "Selected objects\:"
	foreach obj $objs {
	    incr objcnt
	    if { $objcnt > $ListLimit } {
        	puts stdout "Listed $ListLimit of $tot_obj_in_rect selected objects."
		break;
	    }
	    puts stdout "$objcnt\t$obj"
	}
    }

    # highlight in yellow current group
    if { [search -name $GroupName] != "" } {
	e -C255/255/0 $GroupName
    }
}


proc dg {} {
    done_grouper
}


proc done_grouper {} {
    global mged_gui mged_default mged_players
    global GroupNameGlobal
    global GrouperRunning

    for {set i 0} {1} {incr i} {
	set id [subst $mged_default(id)]_$i
	if { [lsearch -exact $mged_players $id] != -1 } {
	    break;
	}
    }
    set mged_gui($id,mouse_behavior) d
    set_mouse_behavior $id
    rset r draw 0

    bind $mged_gui($id,active_dm) <Control-ButtonRelease-2> ""
    bind $mged_gui($id,active_dm) <ButtonRelease-2> ""

    # remove yellow highlights from the display
    erase $GroupNameGlobal
    set GrouperRunning 0

    puts stdout "Grouper done."
}


# Local Variables:
# mode: Tcl
# tab-width: 8
# c-basic-offset: 4
# tcl-indent-level: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
