#!/bin/sh
#                         I G E S . S H
# BRL-CAD
#
# Copyright (c) 2010 United States Government as represented by
# the U.S. Army Research Laboratory.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following
# disclaimer in the documentation and/or other materials provided
# with the distribution.
#
# 3. The name of the author may not be used to endorse or promote
# products derived from this software without specific prior written
# permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
###

# Ensure /bin/sh
export PATH || (echo "This isn't sh."; sh $0 $*; kill $$)

# save the precious args
ARGS="$*"
NAME_OF_THIS=`basename $0`
PATH_TO_THIS=`dirname $0`
THIS="$PATH_TO_THIS/$NAME_OF_THIS"

MGED="$1/src/mged/mged"
if test ! -f "$MGED" ; then
    MGED="$PATH_TO_THIS/../src/mged/mged"
    if test ! -f "$MGED" ; then
	MGED="../src/mged/mged"
	if test ! -f "$MGED" ; then
	    echo "Unable to find mged, aborting"
	    exit 1
	fi
    fi
fi
MGEDDIR="`dirname $MGED`"

LD_LIBRARY_PATH=$MGEDDIR/../../src/other/tcl/unix:$MGEDDIR/../../src/other/tk/unix:$1/src/other/tcl/unix:$1/src/other/tk/unix:$LD_LIBRARY_PATH
DYLD_LIBRARY_PATH=$MGEDDIR/../../src/other/tcl/unix:$MGEDDIR/../../src/other/tk/unix:$1/src/other/tcl/unix:$1/src/other/tk/unix:$DYLD_LIBRARY_PATH
export LD_LIBRARY_PATH DYLD_LIBRARY_PATH


rm -f iges.log iges.g iges_file.iges iges_stdout_new.g iges_new.g iges_stdout.iges iges_file.iges

STATUS=0

$MGED -c 2> iges.log <<EOF
opendb iges.g y

units mm
size 1000
make box.s arb8
facetize -n box.nmg box.s
kill box.s
q
EOF

../src/conv/iges/g-iges -o iges_file.iges iges.g box.nmg 2>> iges.log> /dev/null
../src/conv/iges/g-iges iges.g box.nmg > iges_stdout.iges 2>> iges.log

../src/conv/iges/iges-g -o iges_new.g -p iges_file.iges 2>> iges.log

if [ $? != 0 ] ; then
    echo g-iges/iges-g FAILED
    STATUS=-1
else
    echo g-iges/iges-g completed successfully
fi


../src/conv/iges/iges-g -o iges_stdout_new.g -p iges_stdout.iges 2>> iges.log

if [ $? != 0 ] ; then
    echo g-iges/iges-g FAILED
    STATUS=-1
else
    echo g-iges/iges-g completed successfully
fi


if [ X$STATUS = X0 ] ; then
    echo "-> iges.sh succeeded"
else
    echo "-> iges.sh FAILED"
fi

exit $STATUS

# Local Variables:
# mode: sh
# tab-width: 8
# sh-indentation: 4
# sh-basic-offset: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
