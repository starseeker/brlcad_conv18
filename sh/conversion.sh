#!/bin/ksh
#                   C O N V E R S I O N . S H
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
#
# Check the status of our conversion capability on given geometry
#
###

# Ensure /bin/sh
export PATH || (echo "This isn't sh."; sh $0 $*; kill $$)

# save the precious args
ARGS="$*"
NAME_OF_THIS=`basename $0`
PATH_TO_THIS=`dirname $0`
THIS="$PATH_TO_THIS/$NAME_OF_THIS"

# sanity check
if test ! -f "$THIS" ; then
    echo "INTERNAL ERROR: $THIS does not exist"
    if test ! "x$0" = "x$THIS" ; then
	echo "INTERNAL ERROR: dirname/basename inconsistency: $0 != $THIS"
    fi
    exit 1
fi

# force locale setting to C so things like date output as expected
LC_ALL=C


#######################
# log to tty and file #
#######################
log ( ) {

    # this routine writes the provided argument(s) to a log file as
    # well as echoing them to stdout if we're not in quiet mode.

    if test $# -lt 1 ; then
	format=""
    else
	format="$1"
	shift
    fi

    if test ! "x$LOGFILE" = "x" ; then
	action="printf \"$format\n\" $* >> \"$LOGFILE\""
	eval "$action"
    fi
    if test ! "x$QUIET" = "x1" ; then
	action="printf \"$format\n\" $*"
	eval "$action"
    fi
}


##############################
# ensure a 0|1 boolean value #
##############################
booleanize ( ) {

    # this routine examines the value of the specified variable and
    # converts it to a simple zero or one value in order to simplify
    # checks later on

    for var in $* ; do

	VAR="$var"
	VALCMD="echo \$$VAR"
	VAL="`eval $VALCMD`"

	CMD=__UNSET__
	case "x$VAL" in
	    x)
		CMD="$VAR=0"
		;;
	    x0)
		CMD="$VAR=0"
		;;
	    x[nN]*)
		CMD="$VAR=0"
		;;
	    x*)
		CMD="$VAR=1"
		;;
	esac

	if test ! "x$CMD" = "x__UNSET__" ; then
	    eval $CMD
	    export $VAR
	fi

    done
}


####################
# handle arguments #
####################

# process the argument list for commands
for arg in $ARGS ; do
    case "x$arg" in
	x*[hH])
	    HELP=1
	    shift
	    ;;
	x*[hH][eE][lL][pP])
	    HELP=1
	    shift
	    ;;
	x*[iI][nN][sS][tT][rR][uU][cC][tT]*)
	    INSTRUCTIONS=1
	    shift
	    ;;
	x*[qQ][uU][iI][eE][tT])
	    QUIET=1
	    shift
	    ;;
	x*[vV][eE][rR][bB][oO][sS][eE])
	    VERBOSE=1
	    shift
	    ;;
	x*)
	    ;;
    esac
done

# validate and clean up options (all default to 0)
booleanize HELP INSTRUCTIONS VERBOSE

if test $# -eq 0 ; then
    echo "No geometry files specified."
    echo ""
    HELP=1
fi

###
# handle help before main processing
###
if test "x$HELP" = "x1" ; then
    echo "Usage: $0 [command(s)] [OPTION=value] file1.g [file2.g ...]"
    echo ""
    echo "Available commands:"
    echo "  help          (this is what you are reading right now)"
    echo "  instructions"
    echo "  quiet"
    echo "  verbose"
    echo ""
    echo "Available options:"
    echo "  GED=/path/to/geometry/editor (default mged)"
    echo "  MAXTIME=#seconds (default 30)"
    echo ""
    echo "BRL-CAD is a powerful cross-platform open source solid modeling system."
    echo "For more information about BRL-CAD, see http://brlcad.org"
    echo ""
    echo "Run '$0 instructions' for additional information."
    exit 1
fi


###
# handle instructions before main processing
###
if test "x$INSTRUCTIONS" = "x1" ; then
    cat <<EOF

This script is used to test BRL-CAD's explicit surface polygonal
conversion capabilities.  It takes a list of one or more geometry
files and for every geometry object in the file, it facetizes the
object into NMG and BoT format.  The script times, tabulates, and
reports conversion results.

The script is useful for a variety of purposes including for
developers to perform regression testing as BRL-CAD's conversion
capabilities are modified and improved.  It's also useful for users to
get a report of which objects in a given BRL-CAD geometry database
will convert, what percentage, and how long the conversion will take.

There are several environment variables that will modify how this
script behaves:

  GED - pathname to the BRL-CAD geometry editor (i.e., mged)
  MAXTIME - maximum number of seconds allowed for each conversion
  VERBOSE - turn on extra debug output for testing/development
  QUIET - turn off all printing output (writes results to log file)
  INSTRUCTIONS - display these more detailed instructions

The GED option allows you to specify a specific pathname for MGED.
The default is to search the system path for 'mged'.

The MAXTIME option specifies how many seconds are allowed to elapse
before the conversion is aborted.  Some conversions can take days or
months (or infinite in the case of a bug), so this places an upper
bound on the per-object conversion time.
EOF
    exit 0
fi


#############
# B E G I N #
#############

# where to write results
LOGFILE=conversion-$$-run.log
touch "$LOGFILE"
if test ! -w "$LOGFILE" ; then
    if test ! "x$LOGFILE" = "x/dev/null" ; then
	echo "ERROR: Unable to log to $LOGFILE"
    fi
    LOGFILE=/dev/null
fi

VERBOSE_ECHO=:
ECHO=log
if test "x$QUIET" = "x1" ; then
    if test "x$VERBOSE" = "x1" ; then
	echo "Verbose output quelled by quiet option.  Further output disabled."
    fi
else
    if test "x$VERBOSE" = "x1" ; then
	VERBOSE_ECHO=echo
	echo "Verbose output enabled"
    fi
fi

###
# ensure variable is set to something #
###
set_if_unset ( ) {
    set_if_unset_name="$1" ; shift
    set_if_unset_val="$1" ; shift

    set_if_unset_var="echo \"\$$set_if_unset_name\""
    set_if_unset_var_val="`eval ${set_if_unset_var}`"
    if test "x${set_if_unset_var_val}" = "x" ; then
	set_if_unset_var="${set_if_unset_name}=\"${set_if_unset_val}\""
	eval $set_if_unset_var
	export $set_if_unset_name
    fi

    set_if_unset_var="echo \"\$$set_if_unset_name\""
    set_if_unset_val="`eval ${set_if_unset_var}`"
}

# approximate maximum time in seconds that a given conversion is allowed to take
set_if_unset GED mged
set_if_unset MAXTIME 30

# commands that this script expects
MGED="`which $GED`"
if test ! -f "$MGED" ; then
    $ECHO "Unable to find $GED, aborting"
    exit 1
fi


$ECHO "B R L - C A D   C O N V E R S I O N"
$ECHO "==================================="
$ECHO "Running $THIS on `date`"
$ECHO "Logging output to $LOGFILE"
$ECHO "`uname -a 2>&1`"
$ECHO "Using [${MGED}] for GED"
$ECHO "Using [${MAXTIME}] for MAXTIME"
$ECHO

count=0
nmg_count=0
bot_count=0
$ECHO "%s" "-=-"
while test $# -gt 0 ; do
    file="$1"
    if ! test -f "$file" ; then
	echo "Unable to read file [$file]"
	shift
	continue
    fi

    work="${file}.conversion"
    cmd="cp \"$file\" \"$work\""
    $VERBOSE_ECHO "\$ $cmd"
    eval $cmd

    # execute in a coprocess
    cmd="$GED -c \"$work\" search ."
    objects=`eval $cmd 2>&1 | grep -v Using`
    $VERBOSE_ECHO "\$ $cmd"

    while read object ; do

	obj="`basename \"$object\"`"
	found=`$GED -c "$work" search . -name \"${obj}\" 2>&1 | grep -v Using`
	if test "x$found" != "x$object" ; then
	    echo "INTERNAL ERROR: Failed to find [$object] with [$obj] (got [$found])"
	    exit 3
	fi

	# convert NMG
	nmg=FAIL
	cmd="$GED -c "$work" facetize -n \"${obj}.nmg\" \"${obj}\""
	$VERBOSE_ECHO "\$ $cmd"
	output=`eval $cmd 2>&1 | grep -v Using`
	$VERBOSE_ECHO "$output"

	# verify NMG
	found=`$GED -c "$work" search . -name \"${obj}.nmg\" 2>&1 | grep -v Using`
	if test "x$found" = "x${object}.nmg" ; then
	    nmg=pass
	    nmg_count=`expr $nmg_count + 1`
	fi
	
	# convert BoT
	bot=FAIL
	cmd="$GED -c "$work" facetize \"${obj}.bot\" \"${obj}\""
	$VERBOSE_ECHO "\$ $cmd"
	output=`eval $cmd 2>&1 | grep -v Using`
	$VERBOSE_ECHO "$output"

	# verify BoT
	found=`$GED -c "$work" search . -name \"${obj}.bot\" 2>&1 | grep -v Using`
	if test "x$found" = "x${object}.bot" ; then
	    bot=pass
	    bot_count=`expr $bot_count + 1`
	fi
	
	$ECHO "nmg:%s bot:%s %s:%s" $nmg $bot "$file" "$object"
	count=`expr $count + 1`

    done <<EOF
$objects
EOF

    rm -f "$work"
    shift
done
$ECHO "%s" "-=-"

if test $count -eq 0 ; then
    nmg_percent=0
    bot_percent=0
    rate=0
else
    nmg_percent=`echo $nmg_count $count | awk '{print ($1/$2)*100.0}'`
    bot_percent=`echo $bot_count $count | awk '{print ($1/$2)*100.0}'`
    rate=`echo $nmg_count $bot_count $count | awk '{print ($1+$2)/($3+$3)*100.0}'`
fi

$ECHO
$ECHO "... Done."
$ECHO
$ECHO "Summary:"
$ECHO
$ECHO "NMG conversion:  %.1f%%  (%ld of %ld objects)" $nmg_percent $nmg_count $count
$ECHO "BoT conversion:  %.1f%%  (%ld of %ld objects)" $bot_percent $bot_count $count
$ECHO "Overall conversion rate:  %.1f%%" $rate
$ECHO
$ECHO "Elapsed conversion time: %lf" 0.0
$ECHO "Average conversion time: %lf" 0.0
$ECHO
$ECHO "Output was saved to $LOGFILE from `pwd`"
$ECHO "Conversion testing complete."


# Local Variables:
# tab-width: 8
# mode: sh
# sh-indentation: 4
# sh-basic-offset: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
