#!../src/bltwish

package require BLT
# --------------------------------------------------------------------------
# Starting with Tcl 8.x, the BLT commands are stored in their own 
# namespace called "blt".  The idea is to prevent name clashes with
# Tcl commands and variables from other packages, such as a "table"
# command in two different packages.  
#
# You can access the BLT commands in a couple of ways.  You can prefix
# all the BLT commands with the namespace qualifier "blt::"
#  
#    blt::graph .g
#    blt::table . .g -resize both
# 
# or you can import all the command into the global namespace.
#
#    namespace import blt::*
#    graph .g
#    table . .g -resize both
#
# --------------------------------------------------------------------------

if { $tcl_version >= 8.0 } {
    namespace import blt::*
    namespace import -force blt::tile::*
}

source scripts/demo.tcl
source scripts/stipples.tcl
source scripts/patterns.tcl

bitmap define hobbes { 
#define hobbes_width 25
#define hobbes_height 25
static char hobbes_bits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x00,
    0x78, 0xe0, 0x07, 0x00, 0xfc, 0xf8, 0x07, 0x00, 0xcc, 0x07, 0x04, 0x00,
    0x0c, 0xf0, 0x0b, 0x00, 0x7c, 0x1c, 0x06, 0x00, 0x38, 0x00, 0x00, 0x00,
    0xe0, 0x03, 0x10, 0x00, 0xe0, 0x41, 0x11, 0x00, 0x20, 0x40, 0x11, 0x00,
    0xe0, 0x07, 0x10, 0x00, 0xe0, 0xc1, 0x17, 0x00, 0x10, 0xe0, 0x2f, 0x00,
    0x20, 0xe0, 0x6f, 0x00, 0x18, 0xe0, 0x2f, 0x00, 0x20, 0xc6, 0x67, 0x00,
    0x18, 0x84, 0x2b, 0x00, 0x20, 0x08, 0x64, 0x00, 0x70, 0xf0, 0x13, 0x00,
    0x80, 0x01, 0x08, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};
}

bitmap define gort { 
#define gort_width 64
#define gort_height 64
static char gort_bits[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0xf0, 
    0x3f, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0xff, 0xff, 0x07, 0x00, 0x80,
    0x01, 0x00, 0xc0, 0xdf, 0x0f, 0x1e, 0x00, 0x80, 0x01, 0x00, 0x60, 0xdf, 
    0x7f, 0x38, 0x00, 0x80, 0x01, 0x00, 0x30, 0x84, 0xfd, 0x67, 0x00, 0x80,
    0x01, 0x00, 0x18, 0x04, 0xf6, 0xef, 0x00, 0x80, 0x01, 0x00, 0x0c, 0x86, 
    0xe1, 0xc7, 0x01, 0x80, 0x01, 0x00, 0x06, 0x06, 0xc0, 0x96, 0x01, 0x80,
    0x01, 0x00, 0x06, 0x06, 0x00, 0x97, 0x03, 0x80, 0x01, 0x00, 0x06, 0x06, 
    0x00, 0x3c, 0x03, 0x80, 0x01, 0x00, 0x03, 0x06, 0x00, 0x5c, 0x07, 0x80,
    0x01, 0x00, 0x03, 0x02, 0x00, 0xd8, 0x06, 0x80, 0x01, 0x00, 0x03, 0x02, 
    0x00, 0xd8, 0x06, 0x80, 0x01, 0x00, 0x43, 0x02, 0x00, 0xb0, 0x0c, 0x80,
    0x01, 0x80, 0x31, 0x03, 0x00, 0xe0, 0x0d, 0x80, 0x01, 0x80, 0x61, 0x03, 
    0x00, 0xe0, 0x0d, 0x80, 0x01, 0x80, 0x1b, 0x03, 0x00, 0xf0, 0x0c, 0x80,
    0x01, 0x80, 0xb3, 0x03, 0xff, 0xff, 0x1d, 0x80, 0x01, 0xc0, 0xeb, 0xfb, 
    0xff, 0xff, 0x1d, 0x80, 0x01, 0xe0, 0xc5, 0x7f, 0xfe, 0x7f, 0x3f, 0x01,
    0xe0, 0xeb, 0xe3, 0xff, 0xff, 0x6e, 0x80, 0x01, 0xd0, 0x3b, 0xfe, 0x01, 
    0x80, 0x40, 0xcf, 0x80, 0x01, 0xf0, 0xf7, 0x07, 0x00, 0x30, 0x8e, 0x80,
    0x01, 0xf0, 0xf4, 0x00, 0x00, 0x1b, 0x98, 0x80, 0x01, 0x70, 0x14, 0x00, 
    0x00, 0x1f, 0xdc, 0x80, 0x01, 0x30, 0xfe, 0xff, 0x1f, 0xc8, 0xff, 0x80,
    0x01, 0x20, 0xee, 0xff, 0xff, 0xff, 0xff, 0x80, 0x01, 0x20, 0xf7, 0xff, 
    0x7f, 0xfe, 0x7f, 0x80, 0x01, 0xc0, 0xe1, 0xff, 0xff, 0xfe, 0x3f, 0x80,
    0x01, 0x80, 0xed, 0xff, 0xff, 0xff, 0x19, 0x80, 0x01, 0x80, 0x99, 0xff, 
    0xff, 0xff, 0x18, 0x80, 0x01, 0x00, 0x63, 0x83, 0xff, 0x7f, 0x08, 0x80,
    0x01, 0x00, 0xc3, 0x06, 0x00, 0x00, 0x0c, 0x80, 0x01, 0x00, 0x9b, 0x07, 
    0x00, 0x00, 0x0c, 0x80, 0x01, 0x00, 0xb6, 0x07, 0x00, 0x10, 0x0c, 0x80,
    0x01, 0x00, 0xc6, 0x07, 0x00, 0x10, 0x04, 0x80, 0x01, 0x00, 0x36, 0x06, 
    0x00, 0x18, 0x06, 0x80, 0x01, 0x00, 0x66, 0x06, 0x00, 0x18, 0x06, 0x80,
    0x01, 0x00, 0x8c, 0x0d, 0x00, 0x18, 0x02, 0x80, 0x01, 0x00, 0x18, 0x0e, 
    0x00, 0x18, 0x03, 0x80, 0x01, 0x00, 0xf0, 0x0c, 0x00, 0x18, 0x03, 0x80,
    0x01, 0x00, 0x30, 0x0f, 0x00, 0x98, 0x01, 0x80, 0x01, 0x00, 0xb0, 0x1f, 
    0x01, 0x98, 0x01, 0x80, 0x01, 0x00, 0x60, 0x1f, 0x03, 0xdc, 0x00, 0x80,
    0x01, 0x00, 0xe0, 0x3f, 0x03, 0xdc, 0x00, 0x80, 0x01, 0x00, 0xe0, 0x3b, 
    0x07, 0xee, 0x00, 0x80, 0x01, 0x00, 0x70, 0xf8, 0xff, 0xff, 0x01, 0x80,
    0x01, 0x00, 0xf0, 0x80, 0xff, 0x37, 0x03, 0x80, 0x01, 0x00, 0xfc, 0x00, 
    0x08, 0xd8, 0x03, 0x80, 0x01, 0x00, 0xfe, 0x03, 0xf8, 0x7f, 0x07, 0x80,
    0x01, 0xc0, 0x87, 0x07, 0xe0, 0x3f, 0x1c, 0x80, 0x01, 0xf0, 0x03, 0x00, 
    0x00, 0x00, 0xf8, 0x8f, 0x81, 0xff, 0x00, 0x38, 0x00, 0xf6, 0xf9, 0xff,
    0xfd, 0x3f, 0x00, 0xe0, 0x00, 0x83, 0x8f, 0xff, 0xff, 0x00, 0x00, 0x80, 
    0x01, 0x00, 0xfc, 0xc0, 0x07, 0x0e, 0x00, 0x38, 0xe0, 0x00, 0xe0, 0x9f,
    0xf9, 0x07, 0x00, 0x00, 0x80, 0x03, 0x00, 0xff, 0x7f, 0x80, 0x01, 0x00, 
    0xc0, 0x00, 0x00, 0xf0, 0x7f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x0f, 0xf0, 0x00, 0x38, 0x00, 0x00, 0x00, 0x80, 0x01, 0x30, 0x00, 0x00, 
    0x38, 0xc0, 0xc0, 0x80, 0x01, 0x1c, 0xe0, 0x00, 0x0c, 0xc0, 0x83, 0x81,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
}

bitmap define xbob {
#define bob_x_hot 30
#define bob_y_hot 37
#define bob_width 61
#define bob_height 75
static char bob_bits[] = {            
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff,
   0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0x1f, 0x00, 0x00,
   0x00, 0x80, 0xff, 0xff, 0xff, 0xfb, 0x00, 0x00, 0x00, 0xc0, 0xff, 0xcf,
   0x9f, 0xd1, 0x03, 0x00, 0x00, 0xf0, 0x7f, 0x8c, 0x33, 0x91, 0x07, 0x00,
   0x00, 0xf8, 0xa7, 0x18, 0x27, 0xb1, 0x06, 0x00, 0x00, 0xfc, 0x47, 0x31,
   0x4e, 0xa6, 0x0e, 0x00, 0x00, 0xfe, 0x4f, 0x21, 0x4c, 0xae, 0x3d, 0x00,
   0x00, 0xff, 0xdf, 0x23, 0x8d, 0xbe, 0x7d, 0x00, 0x80, 0xff, 0xff, 0x67,
   0xbd, 0xfe, 0xff, 0x01, 0x80, 0xff, 0xff, 0x7f, 0xbf, 0xff, 0xff, 0x03,
   0xc0, 0xff, 0xff, 0xff, 0xbf, 0xff, 0xf8, 0x07, 0xc0, 0xff, 0xff, 0xff,
   0xbf, 0x3f, 0xf8, 0x07, 0xc0, 0xff, 0xff, 0xff, 0xff, 0x07, 0xf8, 0x0f,
   0xc0, 0xff, 0xff, 0xff, 0x3f, 0x00, 0xf8, 0x0f, 0xe0, 0x7f, 0x00, 0xf8,
   0x07, 0x00, 0xf0, 0x0f, 0xe0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x07,
   0xe0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x07, 0xe0, 0x3f, 0x00, 0x00,
   0x00, 0x00, 0xf4, 0x07, 0xe0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xe4, 0x07,
   0xe0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xe4, 0x07, 0xe0, 0x3f, 0x00, 0x00,
   0x00, 0x00, 0xe6, 0x07, 0xe0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xe7, 0x07,
   0xe0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xe6, 0x07, 0xe0, 0x3f, 0x00, 0x00,
   0x00, 0x00, 0xe6, 0x07, 0xe0, 0x3f, 0x00, 0x00, 0x00, 0x00, 0xe6, 0x07,
   0xc0, 0x3f, 0x00, 0x00, 0x00, 0x78, 0xf6, 0x07, 0xa0, 0xbf, 0xff, 0x00,
   0x00, 0xff, 0xf7, 0x07, 0x70, 0x9f, 0xff, 0x01, 0x80, 0xff, 0xef, 0x07,
   0xf0, 0x1c, 0x80, 0x03, 0xe0, 0x01, 0xef, 0x07, 0xf0, 0x1f, 0xbe, 0x07,
   0xf0, 0x3f, 0xee, 0x07, 0xe0, 0x9d, 0x83, 0x1f, 0xf8, 0xe1, 0xdc, 0x07,
   0xe0, 0xc1, 0x7f, 0x1f, 0xfc, 0xff, 0xc8, 0x07, 0xe0, 0xc1, 0x69, 0x1e,
   0x7e, 0xca, 0xc0, 0x03, 0xe0, 0x81, 0xb8, 0x1f, 0xc0, 0x0e, 0xc0, 0x03,
   0xe0, 0x01, 0xc0, 0x1b, 0xc0, 0xcf, 0xc1, 0x03, 0xc0, 0x03, 0xf7, 0x11,
   0x00, 0x7f, 0xc0, 0x03, 0xc0, 0x03, 0x7c, 0x18, 0x00, 0x1c, 0xc0, 0x02,
   0xc0, 0x02, 0x30, 0x08, 0x00, 0x00, 0x40, 0x03, 0x40, 0x03, 0x00, 0x08,
   0x00, 0x00, 0x40, 0x02, 0x40, 0x13, 0x00, 0x0c, 0x00, 0x00, 0x60, 0x02,
   0x40, 0x12, 0x00, 0x0e, 0x00, 0x00, 0xc0, 0x03, 0x80, 0x33, 0x80, 0x0e,
   0x00, 0x00, 0xa8, 0x01, 0x00, 0x33, 0x40, 0x0f, 0xa0, 0x03, 0x2c, 0x00,
   0x00, 0x74, 0x30, 0x0f, 0x38, 0x07, 0x2e, 0x00, 0x00, 0x74, 0x98, 0x1f,
   0x1e, 0x1e, 0x2f, 0x00, 0x00, 0xfc, 0x8f, 0xff, 0x0f, 0xfc, 0x2f, 0x00,
   0x00, 0xf8, 0xe3, 0xff, 0x03, 0xf8, 0x2f, 0x00, 0x00, 0xf8, 0xfd, 0xff,
   0x81, 0xff, 0x3f, 0x00, 0x00, 0xb8, 0xf9, 0x1f, 0xf8, 0x0f, 0x1e, 0x00,
   0x00, 0x30, 0xf1, 0xf0, 0x0f, 0x03, 0x0e, 0x00, 0x00, 0x30, 0xf1, 0x01,
   0x80, 0x01, 0x0f, 0x00, 0x00, 0x20, 0xf1, 0xf7, 0xff, 0x00, 0x07, 0x00,
   0x00, 0x60, 0xe3, 0x01, 0x60, 0x80, 0x07, 0x00, 0x00, 0x60, 0xc3, 0xef,
   0x3f, 0x80, 0x03, 0x00, 0x00, 0x40, 0xc2, 0xff, 0x0f, 0xc0, 0x03, 0x00,
   0x00, 0xc0, 0xe6, 0x1f, 0x00, 0xc0, 0x01, 0x00, 0x00, 0x80, 0xf4, 0xfe,
   0x3f, 0xe0, 0x00, 0x00, 0x00, 0x80, 0x79, 0xfe, 0x1f, 0xe0, 0x00, 0x00,
   0xc0, 0x01, 0x3d, 0x3e, 0x00, 0x70, 0x00, 0x00, 0x30, 0x06, 0x3e, 0x0f,
   0x00, 0x38, 0x00, 0x00, 0xc8, 0x8c, 0x1f, 0x07, 0x00, 0x38, 0x00, 0x00,
   0xf4, 0xcc, 0x8f, 0x07, 0x00, 0x1c, 0x00, 0x00, 0x72, 0xee, 0xf7, 0x07,
   0x00, 0x0e, 0x00, 0x00, 0x02, 0xff, 0xe3, 0x07, 0x00, 0x07, 0x00, 0x00,
   0x32, 0xfe, 0xc1, 0xff, 0x8f, 0x03, 0x00, 0x00, 0x3e, 0xfe, 0x80, 0xff,
   0xff, 0x01, 0x00, 0x00, 0x7e, 0x7c, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00,
   0x7c, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x1c, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xf8, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xf0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x07, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
}

blt::bitmap compose text "Text String" -font {courier 12}

. configure -bg grey80
label .angle -text "Angle" -font { Helvetica 12 bold } -bg grey80
label .scale -text "Scale" -font { Helvetica 12 bold } -bg grey80

blt::table . \
    0,0 .angle -fill both \
    0,5 .scale -fill both 

set row 1
foreach angle { 0 90 180 270 360 45 -45 101 } {
    label .angle$angle -text $angle -bg grey80
    blt::table . \
	$row,0 .angle$angle -fill both
    set column 1
    foreach bitmap { hobbes gort xbob text } {
	set data [blt::bitmap data $bitmap]
	blt::bitmap define $bitmap$row$column $data -rotate $angle
	label .$bitmap$row$column -bitmap $bitmap$row$column -bg white
	blt::table . \
	    $row,$column .$bitmap$row$column
	incr column
    }
    incr row
}

set row 1
foreach scale { 1.0 0.5 0.75 1.4 3.0 } {
    label .scale$row -text $scale -bg grey80
    blt::table . \
	$row,5 .scale$row -fill both
    set column 6
    foreach bitmap { hobbes gort xbob text } {
	set data [blt::bitmap data $bitmap]
	blt::bitmap define $bitmap$row$column $data -scale $scale
	label .$bitmap$row$column -bitmap $bitmap$row$column -bg white
	blt::table . \
	    $row,$column .$bitmap$row$column
	incr column
    }
    incr row
}

foreach scale { 2.0 0.8 1.2 } angle { 45 -45 101 } {
    label .scale$row -text "$scale/$angle" -bg grey80
    blt::table . \
	$row,5 .scale$row -fill both
    set column 6
    foreach bitmap { hobbes gort xbob text } {
	set data [blt::bitmap data $bitmap]
	blt::bitmap define $bitmap$row $data -scale $scale -rotate $angle
	label .$bitmap$row -bitmap $bitmap$row -bg white
	blt::table . \
	    $row,$column .$bitmap$row
	incr column
    }
    incr row
}

blt::table configure . c* -padx 2