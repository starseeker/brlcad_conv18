#                     C H E C K E R . T C L
# BRL-CAD
#
# Copyright (c) 2016 United States Government as represented by
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
# Description -
#
# This is the Geometry Checker GUI.  It presents a list of geometry
# objects that overlap or have other problems.
#

package require Tk
package require Itcl
package require Itk


# go ahead and blow away the class if we are reloading
catch {delete class GeometryChecker} error


::itcl::class GeometryChecker {
    inherit ::itk::Widget

    constructor { args } {}
    destructor {}

    public {
	variable forceSubtract false {}

	method loadOverlaps { args } {}
	method sortBy { col direction } {}

	method goPrev {} {}
	method goNext {} {}

	method subLeft {} {}
	method subRight {} {}

	method display {} {}

	method registerWhoCallback { callback } {}
	method registerDrawCallbacks { left_callback right_callback } {}
	method registerEraseCallback { callback } {}
	method registerOverlapCallback { callback } {}
    }

    private {
	variable _ol_dir ""
	variable _ol_prefix ""

	variable _ck
	variable _status
	variable _commandText
	variable _progressValue
	variable _progressButtonInvoked
	variable _abort

	variable _count

	variable _arrowUp
	variable _arrowDown
	variable _arrowOff
	variable _lastSort "Size 1"

	variable _colorOdd
	variable _colorEven
	variable _colorMarked

	variable _subLeftCommand
	variable _subRightCommand
	variable _drawLeftCommand
	variable _drawRightCommand

	variable _drew

	variable _whoCallback
	variable _leftDrawCallback
	variable _rightDrawCallback
	variable _eraseCallback
	variable _overlapCallback

	variable _afterCommands {}

	method abortCommands {}

	method writeMarks {}

	method changeMarkOnOverlap {id tag_cmd}
	method markOverlap {id}
	method unmarkOverlap {id}

	method changeMarkOnSelection {tag_cmd}
	method markSelection {}
	method unmarkSelection {}

	method handleProgressButton {}
	method handleCheckListSelect {}
    }
}

::itcl::body GeometryChecker::handleProgressButton {} {
    set _progressButtonInvoked true

    while {$_commandText != "Stopped."} {
	update
	set _commandText "Stopped."
	lappend _afterCommands [after 500 "[code set [scope _commandText] "Stopped."]"]
    }

    lappend _afterCommands [after 1500 "[code set [scope _commandText] ""]"]
}

body GeometryChecker::handleCheckListSelect {} {
    set found_marked false

    set sset [$_ck selection]
    foreach item $sset {
	if {[lsearch [$_ck item $item -tags] "marked"] != -1} {
	    set found_marked true
	    break
	}
    }

    if {$found_marked} {
	$itk_component(buttonLeft) state disabled
	$itk_component(buttonRight) state disabled
    } else {
	$itk_component(buttonLeft) state !disabled
	$itk_component(buttonRight) state !disabled
    }

    $this display
}

###########
# begin constructor/destructor
###########

::itcl::body GeometryChecker::constructor {args} {

    set _abort false
    set _drew {}

    set _subLeftCommand "puts subtract the left"
    set _subRightCommand "puts subtract the right"

    set _drawLeftCommand "puts draw -C255/0/0"
    set _drawRightCommand "puts draw -C0/0/255"

    image create photo _leftColorSquare
    _leftColorSquare put \#ff0000 -to 0 0 11 11

    image create photo _rightColorSquare
    _rightColorSquare put \#0000ff -to 0 0 11 11

    image create bitmap _arrowUp -data {
    #define up_width 11
    #define up_height 11
	static char up_bits = {
        0x00, 0x00, 0x20, 0x00, 0x70, 0x00, 0xf8, 0x00, 0xfc, 0x01, 0xfe,
        0x03, 0x70, 0x00, 0x70, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00
	}
    }
    image create bitmap _arrowDown -data {
    #define down_width 11
    #define down_height 11
	static char down_bits = {
        0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 0x00, 0xfe,
        0x03, 0xfc, 0x01, 0xf8, 0x00, 0x70, 0x00, 0x20, 0x00, 0x00, 0x00
	}
    }
    image create bitmap _arrowOff -data {
    #define arrowOff_width 11
    #define arrowOff_height 11
	static char arrowOff_bits[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};
    }

    itk_component add headerFrame {
    	ttk::frame $itk_interior.headerFrame -padding 2
    } {}
    itk_component add headerLabelIntro {
    	ttk::label $itk_component(headerFrame).headerLabelIntro -text "This tool is intended to help resolve geometry overlaps and is a work-in-progress"
    } {}
    itk_component add headerLabelStatus {
    	ttk::label $itk_component(headerFrame).headerLabelStatus -text "Data Not Yet Loaded" -padding 2
    } {}

    itk_component add checkFrame {
    	ttk::frame $itk_interior.checkFrame -padding 2 
    } {}
    itk_component add checkList {
    	ttk::treeview $itk_component(checkFrame).checkList \
	    -columns "ID Left Right Size" \
	    -show headings \
	    -yscroll [ code $itk_component(checkFrame).checkScroll set]
    } {}
    itk_component add checkMenu {
	menu $itk_component(checkList).checkMenu -tearoff false
    } {}
    itk_component add checkScroll {
	ttk::scrollbar $itk_component(checkFrame).checkScroll -orient vertical -command [ code $itk_component(checkFrame).checkList yview ]
    } {}

    itk_component add commandLabel {
	ttk::label $itk_interior.commandLabel \
	    -textvariable [scope _commandText] \
    } {}

    itk_component add progressFrame {
    	ttk::frame $itk_interior.progressFrame -padding 0
    } {}
    itk_component add progressBar {
	ttk::progressbar $itk_component(progressFrame).progressBar -variable [scope _progressValue]
    } {}
    itk_component add progressButton {
	ttk::button $itk_component(progressFrame).progressButton \
	-text "X" \
	-width 1 \
	-command [code $this handleProgressButton]
    } {}

    itk_component add checkGrip {
    	ttk::sizegrip $itk_interior.checkGrip
    } {}

    itk_component add checkButtonFrame {
    	ttk::frame $itk_interior.checkButtonFrame -padding 0
    } {}
    itk_component add buttonLeft {
	ttk::button $itk_component(checkButtonFrame).buttonLeft -compound left -image _leftColorSquare -text "Subtract Left" -padding 10 -command [ list $this subLeft ]
    } {}
    itk_component add buttonRight {
	ttk::button $itk_component(checkButtonFrame).buttonRight -compound left -image _rightColorSquare -text "Subtract Right" -padding 10 -command [ list $this subRight ]
    } {}
    itk_component add buttonBoth {
	ttk::button $itk_component(checkButtonFrame).buttonBoth -text "Subtract Both" -padding 10 -state disabled
    } {}
    itk_component add buttonPrev {
	ttk::button $itk_component(checkButtonFrame).buttonPrev -text "Prev" -padding 10 -command [ list $this goPrev ]
    } {}
    itk_component add buttonNext {
	ttk::button $itk_component(checkButtonFrame).buttonNext -text "Next" -padding 10 -command [ list $this goNext ]
    } {}

    eval itk_initialize $args

    set _colorOdd \#ffffff
    set _colorEven \#f0fdf0
    set _colorMarked \#888888

    set _ck $itk_component(checkList)
    set _status $itk_component(headerLabelStatus)

    $_ck column ID -anchor center -width 100 -minwidth 50
    $_ck column Left -anchor w -width 200 -minwidth 100
    $_ck column Right -anchor w -width 200 -minwidth 100
    $_ck column Size -anchor e -width 100 -minwidth 50
    $_ck heading ID -image _arrowDown -anchor w -command [list $this sortBy ID 1]
    $_ck heading Left -text "Left" -image _arrowOff -anchor center -command [list $this sortBy Left 0]
    $_ck heading Right -text "Right" -image _arrowOff -anchor center -command [list $this sortBy Right 0]
    $_ck heading Size -text "Vol. Est." -image _arrowOff -anchor e -command [list $this sortBy Size 0]

    set font [::ttk::style lookup [$_ck cget -style] -font]
    $itk_component(checkMenu) configure -font $font
    $itk_component(checkMenu) add command -label "Mark Selected" -command [code $this markSelection]
    $itk_component(checkMenu) add command -label "Unmark Selected" -command [code $this unmarkSelection]

    pack $itk_component(headerFrame) -side top -fill both
    pack $itk_component(headerLabelStatus) -side top -anchor nw

    pack $itk_component(checkFrame) -expand true -fill both -anchor center
    pack $itk_component(checkFrame).checkScroll -side right -fill y 
    pack $itk_component(checkFrame).checkList -expand 1 -fill both -padx {16 0}

    pack $itk_component(commandLabel) -side top -fill x -padx 2 -anchor nw

    pack $itk_component(progressFrame) -side top -fill x
    pack $itk_component(progressBar) -side left -expand true -fill x -padx {20 2}
    pack $itk_component(progressButton) -side left -padx 2

    pack $itk_component(checkButtonFrame) -side top -fill both
    pack $itk_component(buttonLeft) -side left -padx 4 -pady {16 0} -anchor e -expand true
    pack $itk_component(buttonRight) -side left -padx 4 -pady {16 0} -anchor w -expand true

    pack $itk_component(checkGrip) -side right -anchor se

    bind $itk_component(checkButtonFrame).buttonPrev <Up> [list $this goPrev]
    bind $itk_component(checkButtonFrame).buttonPrev <Down> [list $this goNext]
    bind $itk_component(checkButtonFrame).buttonNext <Up> [list $this goPrev]
    bind $itk_component(checkButtonFrame).buttonNext <Down> [list $this goNext]

    bind $_ck <<TreeviewSelect>> [code $this handleCheckListSelect]
    bind $_ck <ButtonRelease-3> [code tk_popup $itk_component(checkMenu) %X %Y]
}

::itcl::body GeometryChecker::abortCommands {} {
    set _abort true

    # kill any in-progress after commands
    foreach cmd $_afterCommands {
	after cancel $cmd
    }

    # resolve vwait
    set _commandText ""
    update
}

::itcl::body GeometryChecker::destructor {} {
    $this abortCommands
}

###########
# end constructor/destructor
###########


###########
# begin public methods
###########

body GeometryChecker::loadOverlaps {{filename ""}} {

    if {[catch {opendb} db_path]} {
	puts "ERROR: no database seems to be open"
	return -code 1
    }

    if {$filename == ""} {
        # overlap file not specified, search current directory
	set dir [file dirname $db_path]
	set name [file tail $db_path]
	set ol_path [file join $dir "${name}.ck" "ck.${name}.overlaps"]

	if {[file exists $ol_path]} {
	    set filename $ol_path
	}
    }

    if {$filename == ""} {
	puts "ERROR: No overlap file was specified. None was found in the current directory."
	return -code 1
    }

    if {$filename == "" || ![file exists $filename]} {
	puts "ERROR: unable to open $filename"
	return -code 1
    }
    set _ol_prefix "ck.[file tail $db_path]"
    set _ol_dir [file dirname $filename]

    puts "Loading from $filename"
    update

    set ovfile [open $filename "r"]
    fconfigure "$ovfile" -encoding utf-8

    set font [::ttk::style lookup [$_ck cget -style] -font]

    set count 0
    set width_right 1
    set width_left 1
    set width_size 1
    set comment_or_empty {^[[:blank:]]*#|^[[:blank:]]*$}
    while {[gets "$ovfile" line] >= 0} {
	if {[regexp "$comment_or_empty" "$line"]} {
	    continue
	}
	set left [lindex $line 0]
	set right [lindex $line 1]
	set size [lindex $line 2]

	# put smaller volume on the left
	if {[info exists bb_vol($left)]} {
	    set vol_left $bb_vol($left)
	} else {
	    set vol_left [::tcl::mathfunc::double [lindex [bb -q -v $left] 3]]
	    set bb_vol($left) $vol_left
	}

	if {[info exists bb_vol($right)]} {
	    set vol_right $bb_vol($right)
	} else {
	    set vol_right [::tcl::mathfunc::double [lindex [bb -q -v $right] 3]]
	    set bb_vol($right) $vol_right
	}

	if {$vol_left > $vol_right} {
	    set tmp $left
	    set left $right
	    set right $tmp
	}

	set len_left [font measure $font $left]
	set len_right [font measure $font $right]
	set len_size [font measure $font $size]

	if { $width_left < $len_left } {
	    set width_left $len_left
	}
	if { $width_right < $len_right } {
	    set width_right $len_right
	}
	if { $width_size < $len_size } {
	    set width_size $len_size
	}

	incr count
	if {$count % 2 == 0} {
	    set tag "even"
	} else {
	    set tag "odd"
	}
	$_ck insert {} end -id $count -text $count -values [list $count $left $right [format %.2f $size]] -tags "$tag"
	if {$count % 100 == 0} {
	    puts "."
	    update
	}
    }
    set _count $count
    set width_count [font measure $font $count]
    set width_wm [lindex [wm maxsize .] 0]

    if {$width_count + $width_left + $width_right + $width_size > $width_wm} {
	set width_left [expr ($width_wm - $width_count - $width_size - 100) / 2]
	set width_right [expr ($width_wm - $width_count - $width_size - 100) / 2]
    }
    $_ck column ID -anchor center -width [expr $width_count + 20] -minwidth $width_count
    $_ck column Left -anchor w -width [expr $width_left + 20] -minwidth [expr $width_left / 10]
    $_ck column Right -anchor w -width [expr $width_right + 20] -minwidth [expr $width_right / 10]
    $_ck column Size -anchor e -width [expr $width_size + 20] -minwidth $width_size

    $_ck tag configure "odd" -background $_colorOdd
    $_ck tag configure "even" -background $_colorEven
    $_ck tag configure "marked" -foreground $_colorMarked
    close $ovfile

    # read and apply saved marks
    set mark_file [file join $_ol_dir "${_ol_prefix}.marked"]
    if {[file exists $mark_file]} {
	set mfile [open $mark_file "r"]
	fconfigure $mfile -encoding utf-8

	set items [$_ck children {}]
	while {[gets $mfile line] >= 0} {
	    foreach item $items {
		if {[$_ck set $item "Left"] == [lindex $line 0] && \
		    [$_ck set $item "Right"] == [lindex $line 1]} \
		{
		    $_ck tag add "marked" $item
		}
	    }
	}
    }

    $this sortBy {*}$_lastSort

    if {$count == 1} {
	$_status configure -text "$count overlap"
    } else {
	$_status configure -text "$count overlaps"
    }
}


# sortBy
#
# toggles sorting of a particular table column
#
body GeometryChecker::sortBy {column direction} {
    # find marked and unmarked items
    set marked {}
    set unmarked {}

    foreach item [$_ck children {}] {
	if {[$_ck tag has "marked" $item]} {
	    lappend marked $item
	} else {
	    lappend unmarked $item
	}
    }

    # pull out the column being sorted on
    set unmarked_data {}
    foreach row $unmarked {
	lappend unmarked_data [list [$_ck set $row $column] $row]
    }

    set marked_data {}
    foreach row $marked {
	lappend marked_data [list [$_ck set $row $column] $row]
    }

    # sort that column and reshuffle all rows to match
    set r -1
    set dir [expr {$direction ? "-decreasing" : "-increasing"}]
    foreach info [lsort -dictionary -index 0 $dir $unmarked_data] {
	$_ck move [lindex $info 1] {} [incr r]
    }

    foreach info [lsort -dictionary -index 0 $dir $marked_data] {
	$_ck move [lindex $info 1] {} [incr r]
    }

    # update header code so we sort opposite direction next time
    $_ck heading $column -command [list $this sortBy $column [expr {!$direction}]]

    # update the header arrows
    set idx -1
    foreach col [$_ck cget -columns] {
	incr idx
	if {$col eq $column} {
	    set arrow [expr {$direction ? "_arrowUp" : "_arrowDown"}]
	    $_ck heading $idx -image $arrow
	} else {
	    $_ck heading $idx -image _arrowOff
	}
    }

    # update row tags (colors)
    set on 1
    foreach id [$_ck children {}] {
	$_ck item $id -tags [expr {$on ? "odd" : "even"}]
	set on [expr !$on]
    }

    foreach id $marked {
	$_ck tag add "marked" $id
    }

    set _lastSort "$column $direction"
}


# goPrev
#
# select the previous node
#
body GeometryChecker::goPrev {} {
    set sset [$_ck selection]
    set slen [llength $sset]
    set alln [$_ck children {}]
    set num0 [lindex $alln 0]

    if { $slen == 0 } {
	set curr $num0
	set prev -1
    } else {
	set curr [lindex $alln [lsearch $alln [lindex $sset 0]]]
	set prev [lindex $alln [expr [lsearch $alln [lindex $sset 0]] - 1]]
    }

    if {$curr != $num0} {
	$_status configure -text "$_count overlaps, drawing #$prev"
	$_ck see $prev
	$_ck selection set $prev
    } else {
	$_status configure -text "$_count overlaps, at the beginning of the list"
	$_ck see $num0
	$_ck selection set {}
    }
}


# goNext
#
# select the next node
#
body GeometryChecker::goNext {} {
    set sset [$_ck selection]
    set slen [llength $sset]
    set alln [$_ck children {}]
    set num0 [lindex $alln 0]

    if { $slen == 0 } {
	set curr -1
	set next $num0
    } else {
	set curr [lindex $alln [lsearch $alln [lindex $sset end]]]
	set next [lindex $alln [expr [lsearch $alln [lindex $sset end]] + 1]]
    }

    set last [lindex $alln end]
    if {$curr != $last} {
	$_status configure -text "$_count overlaps, drawing #$next"
	$_ck see $next
	$_ck selection set $next
    } else {
	$_status configure -text "$_count overlaps, at the end of the list"
	$_ck see $last
    }
}

body GeometryChecker::writeMarks {} {
    set mark_file [file join $_ol_dir "${_ol_prefix}.marked"]
    set tmp_mark_file "${mark_file}.tmp"

    set marked_items [$_ck tag has "marked"]

    # write temp mark file
    set tmp_chan [open $tmp_mark_file "w"]
    fconfigure $tmp_chan -encoding utf-8

    foreach item $marked_items {
	puts $tmp_chan "[$_ck set $item "Left"] [$_ck set $item "Right"]"
    }
    close $tmp_chan

    # verify write is correct
    set tmp_chan [open $tmp_mark_file "r"]
    fconfigure $tmp_chan -encoding utf-8

    foreach item $marked_items {
	if {[gets $tmp_chan line] < 0 ||\
	    $line != "[$_ck set $item "Left"] [$_ck set $item "Right"]"} \
	{
	    puts "ERROR: write to mark file failed"
	    return
	}
    }
    close $tmp_chan

    # copy temp to proper mark file
    file copy -force $tmp_mark_file $mark_file

    # verify copy matches temp
    if {[file size $tmp_mark_file] != [file size $tmp_mark_file]} {
	puts "ERROR: overwrite of mark file failed"
	return
    }

    # delete temp
    file delete $tmp_mark_file
}

body GeometryChecker::changeMarkOnOverlap {id tag_cmd} {
    $_ck tag $tag_cmd "marked" $id

    # remove id from selection set
    set sset [$_ck selection]
    set id_idx [lsearch $sset $id]
    if {$id_idx != -1} {
	$_ck selection set [lreplace $sset $id_idx $id_idx]
    }

    # move marked to bottom of checklist
    $this sortBy {*}$_lastSort
}

body GeometryChecker::markOverlap {id} {
    $this changeMarkOnOverlap $id "add"
    $this writeMarks
}

body GeometryChecker::unmarkOverlap {id} {
    $this changeMarkOnOverlap $id "remove"
    $this writeMarks
}

body GeometryChecker::changeMarkOnSelection {tag_cmd} {
    # Multi-select will have initiated a draw.
    # Abort because we're about to change the selection.
    $this abortCommands

    # set marks
    set sset [$_ck selection]
    foreach item $sset {
	$_ck tag $tag_cmd "marked" $item
    }
    $this writeMarks

    # clear selection
    $_ck selection set ""

    # move marked to bottom of checklist
    $this sortBy {*}$_lastSort
}

body GeometryChecker::markSelection {} {
    $this changeMarkOnSelection "add"
}

body GeometryChecker::unmarkSelection {} {
    $this changeMarkOnSelection "remove"
}

# subLeft
#
# subtract the currently selected left nodes from the right ones
#
body GeometryChecker::subLeft {} {
    set sset [$_ck selection]
    foreach item $sset {
	foreach {id_lbl id left_lbl left right_lbl right size_lbl size} [$_ck set $item] {
	    if {![catch {$_overlapCallback $right $left $forceSubtract} err]} {
		$this markOverlap $id
	    } else {
		puts "$err"
	    }
	}
    }
}

# subRight
#
# subtract the currently selected right nodes from the left ones
#
body GeometryChecker::subRight {} {
    set sset [$_ck selection]
    foreach item $sset {
	foreach {id_lbl id left_lbl left right_lbl right size_lbl size} [$_ck set $item] {
	    if {![catch {$_overlapCallback $left $right $forceSubtract} err]} {
		$this markOverlap $id
	    } else {
		puts "$err"
	    }
	}
    }
}

# display
#
# draw the currently selected geometry
#
body GeometryChecker::display {} {

    set sset [$_ck selection]

    # get list of what we intend to draw
    set drawing ""
    foreach item $sset {
	foreach {id_lbl id left_lbl left right_lbl right size_lbl size} [$_ck set $item] {
	    lappend drawing $left $right
	}
    }

    # erase anything we drew previously that we don't still need
    foreach obj $_drew {
	if {[lsearch -exact $drawing $obj] == -1} {
	    $_eraseCallback $obj
	}
    }

    # draw them again
    set done false
    set count 0
    set total [llength $drawing]
    set _progressValue 1 ;# indicate drawing initiated
    set _progressButtonInvoked false
    foreach item $sset {
	foreach {id_lbl id left_lbl left right_lbl right size_lbl size} [$_ck set $item] {
            # TODO: pack forget, pack after on button?

	    set _commandText "Drawing $left"
	    update

	    if {$total > 2 && $count == 0} {
                # If we're drawing multiple overlaps, give user a
		# chance to abort before the first draw.
		lappend _afterCommands [after 3000 \
		    "if {! \[set \"[scope _progressButtonInvoked]\"\]} { \
		        [code $_leftDrawCallback $left]; \
			[code set [scope _commandText] ""] \
		    }"]

		# wait for left draw to finish before starting right
		#
		# _commandText is set by one of:
		#  - the above after command
		#  - handleProgressButton
		#  - destructor (also sets _abort)
		vwait [scope _commandText]
		if {$_abort} {
		    set done true
		    break
		}
	    } elseif {! $_progressButtonInvoked} {
		$_leftDrawCallback $left
	    }

	    if {! $_progressButtonInvoked} {
		incr count
		set _progressValue [expr $count / [expr $total + 1.0] * 100]

		set _commandText "Drawing $right"
		update

		$_rightDrawCallback $right

		incr count
		set _progressValue [expr $count / [expr $total + 1.0] * 100]
		update
	    } else {
		set done true
		break
	    }
	}
	if {$done} {
	    break
	}
    }
    set _commandText ""
    set _progressValue 0
    set _drew $drawing
}


# registerWhoCallback
#
body GeometryChecker::registerWhoCallback {callback} {
    set _whoCallback $callback
    set _who [$_whoCallback]
}

# registerDrawCallback
#
body GeometryChecker::registerDrawCallbacks {left_callback right_callback} {
    set _leftDrawCallback $left_callback
    set _rightDrawCallback $right_callback
}

# registerEraseCallback
#
body GeometryChecker::registerEraseCallback {callback} {
    set _eraseCallback $callback
}

# registerOverlapCallback
#
body GeometryChecker::registerOverlapCallback {callback} {
    set _overlapCallback $callback
}


##########
# end public methods
##########

proc treeReplaceLeafWithSub {tree leaf sub} {
    # leaf
    if {[lindex $tree 0] == "l"} {
	if {[lindex $tree 1] == $leaf} {
	    set lleaf [list l $leaf]
	    set lsub [list l $sub]
	    set newtree [list - $lleaf $lsub]
	    return $newtree
	} else {
	    return $tree
	}
    }
    
    # op node
    set op [lindex $tree 0]
    set newleft [treeReplaceLeafWithSub [lindex $tree 1] $leaf $sub]
    set newright [treeReplaceLeafWithSub [lindex $tree 2] $leaf $sub]
    return [list $op $newleft $newright]
}

proc subtractRightFromLeft {left right {forceSubtract false}} {
    if [ catch { opendb } dbname ] {
	puts ""
	puts "ERROR: no database seems to be open"
	return -code 1
    }
    if [ exists $left ] {
	puts ""
	puts "ERROR: unable to find $left"
	return -code 1
    }
    if [ exists $right ] {
	puts ""
	puts "ERROR: unable to find $right"
	return -code 1
    }
    if { $left eq $right } {
	puts ""
	puts "ERROR: left and right are the same region"
	set leftcomb [lindex [file split $left] end-1]
	set rightreg [file tail $right]
	puts ""
	puts "There is probably a duplicate $rightreg in $leftcomb"
	puts "You will need to resolve it manually."
	return -code 1
    }

    if {$forceSubtract} {
        # if always subtracting, we'll choose the first unioned solid in right
	set solids [search $right -type shape -bool u]
    } else {
	# simplify right to solid, or report error
	set solids [search $right -type shape]
	set nsolids [llength $solids]
	if {$nsolids > 1} {
	    puts ""
	    puts "ERROR: $right isn't reducible to a single solid. Refusing to subtract a comb."
	    puts "       Run check command with -F option to override."
	    return -code 1
	}
    }
    set rightsub [string trim [file tail [lindex $solids 0]]]

    # if left already contains right, report error
    if {[llength [search $left -name $rightsub]] > 0} {
	puts ""
	puts "WARNING: attempting to subtract $rightsub from $left again"
	puts ""
	puts "An attempted manual resolution may already exist."
	return -code 1
    }

    # search /left -bool u -type shape to find positives
    # adjust each shape's parent to subtract right from all u members
    puts "Subtracting $rightsub in $right from unioned solids of $left."
    foreach { solid } [search $left -bool u -type shape] {
	set solidparent [file tail [file dirname $solid]]
	set tree [get $solidparent tree]
	set leaf [file tail $solid]
	set newtree [treeReplaceLeafWithSub $tree $leaf $rightsub]
	adjust $solidparent tree $newtree
    }
    return
}


proc drawLeft {path} {
    if [ catch { opendb } dbname ] {
	puts ""
	puts "ERROR: no database seems to be open"
	return
    }
    if [ exists $path ] {
	puts ""
	puts "WARNING: unable to find $path"
    }

    if [catch {draw -C255/0/0 $path} path_error] {
	puts "ERROR: $path_error"
    }
}

proc drawRight {path} {
    if [ catch { opendb } dbname ] {
	puts ""
	puts "ERROR: no database seems to be open"
	return
    }
    if [ exists $path ] {
	puts ""
	puts "WARNING: unable to find $path"
    }

    if [catch {draw -C0/0/255 $path} path_error] {
	puts "ERROR: $path_error"
    }
}


# All GeometryChecker stuff is in the GeometryChecker namespace
proc check {{args}} {
    set parent ""
    set filename ""

    set usage false
    set force false
    if {[llength $args] == 1} {
	if {[lindex $args 0] == "-F"} {
	    set force true
	} else {
	    set filename $args
	}
    } elseif {[llength $args] == 2} {
	if {[lindex $args 0] == "-F"} {
	    set force true
	    set filename [lindex $args 1]
	} else {
	    set usage true
	}
    } elseif {[llength $args] > 3} {
	set usage true
    }

    if {$usage} {
	puts {Usage: check [-F] [overlaps_file]}
	return -code 1
    }

    if {$force} {
	puts "WARNING: Running with -F means regions containing multiple solids will be"
	puts "         subtracted as the first unioned solid found in the region. This may"
	puts "         cause the wrong volume to be subtracted."
    }

    if {[winfo exists $parent.checker]} {
	destroy $parent.checker
    }

    if {[catch {opendb} db_path]} {
	puts "ERROR: no database seems to be open"
	return
    }

    set checkerWindow [toplevel $parent.checker]
    set checker [GeometryChecker $checkerWindow.ck]

    $checker configure -forceSubtract $force
    $checker registerWhoCallback [code who]
    $checker registerDrawCallbacks [code drawLeft] [code drawRight]
    $checker registerEraseCallback [code erase]
    $checker registerOverlapCallback [code subtractRightFromLeft]

    if {[catch {$checker loadOverlaps $filename}]} {
	wm withdraw $checkerWindow
	update
	destroy $checkerWindow
	return
    }

    wm title $checkerWindow "Geometry Checker"
    pack $checker -expand true -fill both

    # ensure window isn't too narrow
    update
    set geom [split [wm geometry $checkerWindow] "=x+-"]
    if {[lindex $geom 0] > [lindex $geom 1]} {
	lreplace $geom 1 1 [lindex $geom 0]
    }
    wm geometry $checkerWindow "=[::tcl::mathfunc::round [expr 1.62 * [lindex $geom 1]]]x[lindex $geom 1]"
}


# Local Variables:
# mode: Tcl
# tab-width: 8
# c-basic-offset: 4
# tcl-indent-level: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
