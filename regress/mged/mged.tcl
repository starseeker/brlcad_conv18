#                         M G E D . T C L
# BRL-CAD
#
# Copyright (c) 2004-2009 United States Government as represented by
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

###########
#
#                     M G E D . T C L
#
# This script is the master top level script used to run a comprehensive
# regression test on all MGED commands.  It runs individual scripts with
# mged for each command and is responsible for managing outputs in such
# a fashion that they can be systematically compared to a standard
#

#
#	SETUP

set MGED_CMD /usr/brlcad/bin/mged

file delete mged.g mged.log mged.mged

proc add_test {cmdname} {
     set mgedfile [open mged.mged a]
     set testfile [open [format %s.mged $cmdname] r]
     while {[gets $testfile line] >= 0} {
        puts $mgedfile $line
     }
     close $mgedfile
     close $testfile
}

proc run_test {cmdname} {
     global MGED_CMD
     exec $MGED_CMD -c [format %s.g $cmdname] < [format %s.mged $cmdname] >>& [format %s.log $cmdname]
}

#
#	GEOMETRIC INPUT COMMANDS
#
add_test in
add_test make
add_test 3ptarb
add_test arb
add_test comb
add_test g
add_test r
add_test make_bb
add_test cp
add_test cpi
add_test mv
add_test mvall
add_test build_region
add_test clone
add_test prefix
add_test mirror

#
#	DISPLAYING GEOMETRY - COMMANDS
#





run_test mged
