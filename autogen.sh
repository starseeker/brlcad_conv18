#!/bin/sh
#                        a u t o g e n . s h
#
# Copyright (c) 2005-2007 United States Government as represented by
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
# Script for automatically preparing the sources for compilation by
# performing the myrid of necessary steps.  The script attempts to
# detect proper version support, and outputs warnings about particular
# systems that have autotool peculiarities.
#
# Basically, if everything is set up and installed correctly, the
# script will validate that minimum versions of the GNU Build System
# tools are installed, account for several common configuration
# issues, and then simply run autoreconf for you.
#
# If autoreconf fails, which can happen for many valid configurations,
# this script proceeds to run manual preparation steps effectively
# providing a POSIX shell script (mostly complete) reimplementation of
# autoreconf.
#
# The AUTORECONF, AUTOCONF, AUTOMAKE, LIBTOOLIZE, ACLOCAL, AUTOHEADER
# environment variables may be used to override the default detected
# applications.
#
# Author: Christopher Sean Morrison
#
######################################################################

# set to minimum acceptible version of autoconf
if [ "x$AUTOCONF_VERSION" = "x" ] ; then
    AUTOCONF_VERSION=2.52
fi
# set to minimum acceptible version of automake
if [ "x$AUTOMAKE_VERSION" = "x" ] ; then
    AUTOMAKE_VERSION=1.6.0
fi
# set to minimum acceptible version of libtool
if [ "x$LIBTOOL_VERSION" = "x" ] ; then
    LIBTOOL_VERSION=1.4.2
fi


##################
# ident function #
##################
ident ( ) {
    # extract copyright from header
    __copyright="`grep Copyright $AUTOGEN_SH | head -${HEAD_N}1 | awk '{print $4}'`"
    if [ "x$__copyright" = "x" ] ; then
	__copyright="`date +%Y`"
    fi

    # extract version from CVS Id string
    __id="$Id$"
    __version="`echo $__id | sed 's/.*\([0-9][0-9][0-9][0-9]\)\/\([0-9][0-9]\)\/\([0-9][0-9]\).*/\1\2\3/'`"
    if [ "x$__version" = "x" ] ; then
	__version=""
    fi

    echo "autogen.sh build preparation script by Christopher Sean Morrison"
    echo "revised 3-clause BSD-style license, copyright (c) $__copyright"
    echo "script version $__version, ISO/IEC 9945 POSIX shell script"
}


##################
# USAGE FUNCTION #
##################
usage ( ) {
    echo "Usage: $AUTOGEN_SH [-h|--help] [-v|--verbose] [-q|--quiet] [--version]"
    echo "    --help     Help on $NAME_OF_AUTOGEN usage"
    echo "    --verbose  Verbose progress output"
    echo "    --quiet    Quiet suppressed progress output"
    echo "    --version  Only perform GNU Build System version checks"
    echo
    echo "Description: This script will validate that minimum versions of the"
    echo "GNU Build System tools are installed and then run autoreconf for you."
    echo "Should autoreconf fail, manual preparation steps will be run"
    echo "potentially accounting for several common preparation issues.  The"

    echo "AUTORECONF, AUTOCONF, AUTOMAKE, LIBTOOLIZE, ACLOCAL, AUTOHEADER,"
    echo "PROJECT, & CONFIGURE environment variables and corresponding _OPTIONS"
    echo "variables (e.g. AUTORECONF_OPTIONS) may be used to override the"
    echo "default automatic detection behavior."
    echo

    ident

    return 0
}


##########################
# VERSION_ERROR FUNCTION #
##########################
version_error ( ) {
    if [ "x$1" = "x" ] ; then
	echo "INTERNAL ERROR: version_error was not provided a version"
	exit 1
    fi
    if [ "x$2" = "x" ] ; then
	echo "INTERNAL ERROR: version_error was not provided an application name"
	exit 1
    fi
    $ECHO
    $ECHO "ERROR:  To prepare the ${PROJECT} build system from scratch,"
    $ECHO "        at least version $1 of $2 must be installed."
    $ECHO
    $ECHO "$NAME_OF_AUTOGEN does not need to be run on the same machine that will"
    $ECHO "run configure or make.  Either the GNU Autotools will need to be installed"
    $ECHO "or upgraded on this system, or $NAME_OF_AUTOGEN must be run on the source"
    $ECHO "code on another system and then transferred to here. -- Cheers!"
    $ECHO
}

##########################
# VERSION_CHECK FUNCTION #
##########################
version_check ( ) {
    if [ "x$1" = "x" ] ; then
	echo "INTERNAL ERROR: version_check was not provided a minimum version"
	exit 1
    fi
    _min="$1"
    if [ "x$2" = "x" ] ; then
	echo "INTERNAL ERROR: version check was not provided a comparison version"
	exit 1
    fi
    _cur="$2"

    _min_major="`echo ${_min}. | cut -d. -f1 | sed 's/[^0-9]//g'`"
    _min_minor="`echo ${_min}. | cut -d. -f2 | sed 's/[^0-9]//g'`"
    _min_patch="`echo ${_min}. | cut -d. -f3 | sed 's/[^0-9]//g'`"

    _cur_major="`echo ${_cur}. | cut -d. -f1 | sed 's/[^0-9]//g'`"
    _cur_minor="`echo ${_cur}. | cut -d. -f2 | sed 's/[^0-9]//g'`"
    _cur_patch="`echo ${_cur}. | cut -d. -f3 | sed 's/[^0-9]//g'`"

    if [ "x$_min_major" = "x" ] ; then
	_min_major=0
    fi
    if [ "x$_min_minor" = "x" ] ; then
	_min_minor=0
    fi
    if [ "x$_min_patch" = "x" ] ; then
	_min_patch=0
    fi
    if [ "x$_cur_minor" = "x" ] ; then
	_cur_major=0
    fi
    if [ "x$_cur_minor" = "x" ] ; then
	_cur_minor=0
    fi
    if [ "x$_cur_patch" = "x" ] ; then
	_cur_patch=0
    fi

    if [ $_min_major -lt $_cur_major ] ; then
	return 0
    elif [ $_min_major -eq $_cur_major ] ; then
	if [ $_min_minor -lt $_cur_minor ] ; then
	    return 0
	elif [ $_min_minor -eq $_cur_minor ] ; then
	    if [ $_min_patch -lt $_cur_patch ] ; then
		return 0
	    fi
	fi
    fi
    return 1
}


##################
# argument check #
##################
ARGS="$*"
PATH_TO_AUTOGEN="`dirname $0`"
NAME_OF_AUTOGEN="`basename $0`"
AUTOGEN_SH="$PATH_TO_AUTOGEN/$NAME_OF_AUTOGEN"

LIBTOOL_M4="${PATH_TO_AUTOGEN}/misc/libtool.m4"

if [ "x$HELP" = "x" ] ; then
    HELP=no
fi
if [ "x$QUIET" = "x" ] ; then
    QUIET=no
fi
if [ "x$VERBOSE" = "x" ] ; then
    VERBOSE=no
fi
if [ "x$VERSION_ONLY" = "x" ] ; then
    VERSION_ONLY=no
fi
if [ "x$AUTORECONF_OPTIONS" = "x" ] ; then
    AUTORECONF_OPTIONS="-i -f"
fi
if [ "x$AUTOCONF_OPTIONS" = "x" ] ; then
    AUTOCONF_OPTIONS="-f"
fi
if [ "x$AUTOMAKE_OPTIONS" = "x" ] ; then
    AUTOMAKE_OPTIONS="-a -c -f"
fi
ALT_AUTOMAKE_OPTIONS="-a -c"
if [ "x$LIBTOOLIZE_OPTIONS" = "x" ] ; then
    LIBTOOLIZE_OPTIONS="--automake -c -f"
fi
ALT_LIBTOOLIZE_OPTIONS="--automake --copy --force"
if [ "x$ACLOCAL_OPTIONS" = "x" ] ; then
    ACLOCAL_OPTIONS=""
fi
if [ "x$AUTOHEADER_OPTIONS" = "x" ] ; then
    AUTOHEADER_OPTIONS=""
fi
for arg in $ARGS ; do
    case "x$arg" in
	x--help) HELP=yes ;;
	x-[hH]) HELP=yes ;;
	x--quiet) QUIET=yes ;;
	x-[qQ]) QUIET=yes ;;
	x--verbose) VERBOSE=yes ;;
	x-[vV]) VERBOSE=yes ;;
	x--version) VERSION_ONLY=yes ;;
	*)
	    echo "Unknown option: $arg"
	    echo
	    usage
	    exit 1
	    ;;
    esac
done


#####################
# environment check #
#####################

# sanity check before recursions potentially begin
if [ ! -f "$AUTOGEN_SH" ] ; then
    echo "INTERNAL ERROR: $AUTOGEN_SH does not exist"
    if [ ! "x$0" = "x$AUTOGEN_SH" ] ; then
	echo "INTERNAL ERROR: dirname/basename inconsistency: $0 != $AUTOGEN_SH"
    fi
    exit 1
fi

# force locale setting to C so things like date output as expected
LC_ALL=C

# commands that this script expects
for __cmd in echo head tail ; do
    echo "test" | $__cmd > /dev/null 2>&1
    if [ $? != 0 ] ; then
	echo "INTERNAL ERROR: '${__cmd}' command is required"
	exit 2
    fi
done

# determine the behavior of echo
case `echo "testing\c"; echo 1,2,3`,`echo -n testing; echo 1,2,3` in
    *c*,-n*) ECHO_N= ECHO_C='
' ECHO_T='	' ;;
    *c*,*  ) ECHO_N=-n ECHO_C= ECHO_T= ;;
    *)       ECHO_N= ECHO_C='\c' ECHO_T= ;;
esac

# determine the behavior of head
case "x`echo 'head' | head -n 1 2>&1`" in
    *xhead*) HEAD_N="n " ;;
    *) HEAD_N="" ;;
esac

# determine the behavior of tail
case "x`echo 'tail' | tail -n 1 2>&1`" in
    *xtail*) TAIL_N="n " ;;
    *) TAIL_N="" ;;
esac

VERBOSE_ECHO=:
ECHO=:
if [ "x$QUIET" = "xyes" ] ; then
    if [ "x$VERBOSE" = "xyes" ] ; then
	echo "Verbose output quelled by quiet option.  Further output disabled."
    fi
else
    ECHO=echo
    if [ "x$VERBOSE" = "xyes" ] ; then
	echo "Verbose output enabled"
	VERBOSE_ECHO=echo
    fi
fi

_have_sed="`echo no | sed 's/no/yes/' 2>/dev/null`"
HAVE_SED=no
if [ $? = 0 ] ; then
    [ "x$_have_sed" = "xyes" ] && HAVE_SED=yes
fi
if [ "x$HAVE_SED" = "xno" ] ; then
    $ECHO "Warning:  sed does not appear to be available."
    $ECHO "GNU Autotools version checks are disabled."
fi

# allow a recursive run to disable further recursions
if [ "x$RUN_RECURSIVE" = "x" ] ; then
    RUN_RECURSIVE=yes
fi


################################################
# check for help arg and bypass version checks #
################################################
if [ "x$HAVE_SED" = "xyes" ] ; then
    if [ "x`echo $ARGS | sed 's/.*[hH][eE][lL][pP].*/help/'`" = "xhelp" ] ; then
	HELP=yes
    fi
fi
if [ "x$HELP" = "xyes" ] ; then
    usage
    $ECHO "---"
    $ECHO "Help was requested.  No preparation or configuration will be performed."
    exit 0
fi


#############################
# look for a configure file #
#############################
if [ "x$CONFIGURE" = "x" ] ; then
    CONFIGURE=/dev/null
    if test -f configure.ac ; then
	CONFIGURE=configure.ac
    elif test -f configure.in ; then
	CONFIGURE=configure.in
    fi
else
    $ECHO "Using CONFIGURE environment variable override: $CONFIGURE"
fi


####################
# get project name #
####################
if [ "x$PROJECT" = "x" ] ; then
    PROJECT="`grep AC_INIT $CONFIGURE | grep -v '.*#.*AC_INIT' | tail -${TAIL_N}1 | sed 's/^[ 	]*AC_INIT(\([^,)]*\).*/\1/' | sed 's/.*\[\(.*\)\].*/\1/'`"
    if [ "x$PROJECT" = "x" ] ; then
	PROJECT="project"
    else
	$VERBOSE_ECHO "Detected project name: $PROJECT"
    fi
else
    $ECHO "Using PROJECT environment variable override: $PROJECT"
fi
$ECHO "Preparing the $PROJECT build system...please wait"
$ECHO


########################
# check for autoreconf #
########################
HAVE_AUTORECONF=no
if [ "x$AUTORECONF" = "x" ] ; then
    for AUTORECONF in autoreconf ; do
	$VERBOSE_ECHO "Checking autoreconf version: $AUTORECONF --version"
	$AUTORECONF --version > /dev/null 2>&1
	if [ $? = 0 ] ; then
	    HAVE_AUTORECONF=yes
	    break
	fi
    done
else
    HAVE_AUTORECONF=yes
    $ECHO "Using AUTORECONF environment variable override: $AUTORECONF"
fi


##########################
# autoconf version check #
##########################
_acfound=no
if [ "x$AUTOCONF" = "x" ] ; then
    for AUTOCONF in autoconf ; do
	$VERBOSE_ECHO "Checking autoconf version: $AUTOCONF --version"
	$AUTOCONF --version > /dev/null 2>&1
	if [ $? = 0 ] ; then
	    _acfound=yes
	    break
	fi
    done
else
    _acfound=yes
    $ECHO "Using AUTOCONF environment variable override: $AUTOCONF"
fi

_report_error=no
if [ ! "x$_acfound" = "xyes" ] ; then
    $ECHO "ERROR:  Unable to locate GNU Autoconf."
    _report_error=yes
else
    _version="`$AUTOCONF --version | head -${HEAD_N}1 | sed 's/[^0-9]*\([0-9\.][0-9\.]*\)/\1/'`"
    if [ "x$_version" = "x" ] ; then
	_version="0.0.0"
    fi
    $ECHO "Found GNU Autoconf version $_version"
    version_check "$AUTOCONF_VERSION" "$_version"
    if [ $? -ne 0 ] ; then
	_report_error=yes
    fi
fi
if [ "x$_report_error" = "xyes" ] ; then
    version_error "$AUTOCONF_VERSION" "GNU Autoconf"
    exit 1
fi


##########################
# automake version check #
##########################
_amfound=no
if [ "x$AUTOMAKE" = "x" ] ; then
    for AUTOMAKE in automake ; do
	$VERBOSE_ECHO "Checking automake version: $AUTOMAKE --version"
	$AUTOMAKE --version > /dev/null 2>&1
	if [ $? = 0 ] ; then
	    _amfound=yes
	    break
	fi
    done
else
    _amfound=yes
    $ECHO "Using AUTOMAKE environment variable override: $AUTOMAKE"
fi


_report_error=no
if [ ! "x$_amfound" = "xyes" ] ; then
    $ECHO
    $ECHO "ERROR: Unable to locate GNU Automake."
    _report_error=yes
else
    _version="`$AUTOMAKE --version | head -${HEAD_N}1 | sed 's/[^0-9]*\([0-9\.][0-9\.]*\)/\1/'`"
    if [ "x$_version" = "x" ] ; then
	_version="0.0.0"
    fi
    $ECHO "Found GNU Automake version $_version"
    version_check "$AUTOMAKE_VERSION" "$_version" 
    if [ $? -ne 0 ] ; then
	_report_error=yes
    fi
fi
if [ "x$_report_error" = "xyes" ] ; then
    version_error "$AUTOMAKE_VERSION" "GNU Automake"
    exit 1
fi


########################
# check for libtoolize #
########################
HAVE_LIBTOOLIZE=yes
HAVE_ALT_LIBTOOLIZE=no
_ltfound=no
if [ "x$LIBTOOLIZE" = "x" ] ; then
    LIBTOOLIZE=libtoolize
    $VERBOSE_ECHO "Checking libtoolize version: $LIBTOOLIZE --version"
    $LIBTOOLIZE --version > /dev/null 2>&1
    if [ ! $? = 0 ] ; then
	HAVE_LIBTOOLIZE=no
	$ECHO
	if [ "x$HAVE_AUTORECONF" = "xno" ] ; then
	    $ECHO "Warning:  libtoolize does not appear to be available."
	else
	    $ECHO "Warning:  libtoolize does not appear to be available.  This means that"
	    $ECHO "the automatic build preparation via autoreconf will probably not work."
	    $ECHO "Preparing the build by running each step individually, however, should"
	    $ECHO "work and will be done automatically for you if autoreconf fails."
	fi

	# look for some alternates
	for tool in glibtoolize libtoolize15 libtoolize14 libtoolize13 ; do
	    $VERBOSE_ECHO "Checking libtoolize alternate: $tool --version"
	    _glibtoolize="`$tool --version > /dev/null 2>&1`"
	    if [ $? = 0 ] ; then
		$VERBOSE_ECHO "Found $tool --version"
		_glti="`which $tool`"
		if [ "x$_glti" = "x" ] ; then
		    $VERBOSE_ECHO "Cannot find $tool with which"
		    continue;
		fi
		if test ! -f "$_glti" ; then
		    $VERBOSE_ECHO "Cannot use $tool, $_glti is not a file"
		    continue;
		fi
		_gltidir="`dirname $_glti`"
		if [ "x$_gltidir" = "x" ] ; then
		    $VERBOSE_ECHO "Cannot find $tool path with dirname of $_glti"
		    continue;
		fi
		if test ! -d "$_gltidir" ; then
		    $VERBOSE_ECHO "Cannot use $tool, $_gltidir is not a directory"
		    continue;
		fi
		HAVE_ALT_LIBTOOLIZE=yes
		LIBTOOLIZE="$tool"
		$ECHO
		$ECHO "Fortunately, $tool was found which means that your system may simply"
		$ECHO "have a non-standard or incomplete GNU Autotools install.  If you have"
		$ECHO "sufficient system access, it may be possible to quell this warning by"
		$ECHO "running:"
		$ECHO
		sudo -V > /dev/null 2>&1
		if [ $? = 0 ] ; then
		    $ECHO "   sudo ln -s $_glti $_gltidir/libtoolize"
		    $ECHO
		else
		    $ECHO "   ln -s $_glti $_gltidir/libtoolize"
		    $ECHO
		    $ECHO "Run that as root or with proper permissions to the $_gltidir directory"
		    $ECHO
		fi
		_ltfound=yes
		break
	    fi
	done
    else
	_ltfound=yes
    fi
else
    _ltfound=yes
    $ECHO "Using LIBTOOLIZE environment variable override: $LIBTOOLIZE"
fi


############################
# libtoolize version check #
############################
_report_error=no
if [ ! "x$_ltfound" = "xyes" ] ; then
    $ECHO
    $ECHO "ERROR: Unable to locate GNU Libtool."
    _report_error=yes
else
    _version="`$LIBTOOLIZE --version | head -${HEAD_N}1 | sed 's/[^0-9]*\([0-9\.][0-9\.]*\)/\1/'`"
    if [ "x$_version" = "x" ] ; then
	_version="0.0.0"
    fi
    $ECHO "Found GNU Libtool version $_version"
    version_check "$LIBTOOL_VERSION" "$_version" 
    if [ $? -ne 0 ] ; then
	_report_error=yes
    fi
fi
if [ "x$_report_error" = "xyes" ] ; then
    version_error "$LIBTOOL_VERSION" "GNU Libtool"
    exit 1
fi


#####################
# check for aclocal #
#####################
if [ "x$ACLOCAL" = "x" ] ; then
    for ACLOCAL in aclocal ; do
	$VERBOSE_ECHO "Checking aclocal version: $ACLOCAL --version"
	$ACLOCAL --version > /dev/null 2>&1
	if [ $? = 0 ] ; then
	    break
	fi
    done
else
    $ECHO "Using ACLOCAL environment variable override: $ACLOCAL"
fi


########################
# check for autoheader #
########################
if [ "x$AUTOHEADER" = "x" ] ; then
    for AUTOHEADER in autoheader ; do
	$VERBOSE_ECHO "Checking autoheader version: $AUTOHEADER --version"
	$AUTOHEADER --version > /dev/null 2>&1
	if [ $? = 0 ] ; then
	    break
	fi
    done
else
    $ECHO "Using AUTOHEADER environment variable override: $AUTOHEADER"
fi


#########################
# check if version only #
#########################
$VERBOSE_ECHO "Checking whether to only output version information"
if [ "x$VERSION_ONLY" = "xyes" ] ; then
    $ECHO
    ident
    $ECHO "---"
    $ECHO "Version requested.  No preparation or configuration will be performed."
    exit 0
fi


#######################
# INITIALIZE FUNCTION #
#######################
initialize ( ) {

    #####################
    # detect an aux dir #
    #####################
    _aux_dir=.
    if test "x$HAVE_SED" = "xyes" ; then
	_aux_dir="`grep AC_CONFIG_AUX_DIR $CONFIGURE | grep -v '.*#.*AC_CONFIG_AUX_DIR' | tail -${TAIL_N}1 | sed 's/^[ 	]*AC_CONFIG_AUX_DIR(\(.*\)).*/\1/' | sed 's/.*\[\(.*\)\].*/\1/'`"
	if test ! -d "$_aux_dir" ; then
	    _aux_dir=.
	else
	    $VERBOSE_ECHO "Detected auxillary directory: $_aux_dir"
	fi
    fi


    ################################
    # detect a recursive configure #
    ################################
    _det_config_subdirs="`grep AC_CONFIG_SUBDIRS $CONFIGURE | grep -v '.*#.*AC_CONFIG_SUBDIRS' | sed 's/^[ 	]*AC_CONFIG_SUBDIRS(\(.*\)).*/\1/' | sed 's/.*\[\(.*\)\].*/\1/'`"
    CONFIG_SUBDIRS=""
    for dir in $_det_config_subdirs ; do
	if test -d "$dir" ; then
	    $VERBOSE_ECHO "Detected recursive configure directory: $dir"
	    CONFIG_SUBDIRS="$CONFIG_SUBDIRS $dir"
	fi
    done


    ##########################################
    # make sure certain required files exist #
    ##########################################
    for file in AUTHORS COPYING ChangeLog INSTALL NEWS README ; do
	if test ! -f $file ; then
	    $VERBOSE_ECHO "Touching ${file} since it does not exist"
	    touch $file
	fi
    done


    ########################################################
    # protect COPYING & INSTALL from overwrite by automake #
    ########################################################
    COPYING_BACKUP=__none__
    if test -f COPYING ; then
        # determine if they actually fit in memory
	COPYING_BACKUP="`cat COPYING`"
	if [ "x$COPYING_BACKUP" = "x`cat COPYING`" ] ; then
	    $VERBOSE_ECHO "Stashing an in-memory backup of COPYING"
	    export COPYING_BACKUP
	else
	    COPYING_BACKUP="__none__"
	fi
    fi
    INSTALL_BACKUP=__none__
    if test -f INSTALL ; then
	INSTALL_BACKUP="`cat INSTALL`"
	if [ "x$INSTALL_BACKUP" = "x`cat INSTALL`" ] ; then
	    $VERBOSE_ECHO "Stashing an in-memory backup of INSTALL"
	    export INSTALL_BACKUP
	else
	    INSTALL_BACKUP="__none__"
	fi
    fi


    ##################################################
    # make sure certain generated files do not exist #
    ##################################################
    for file in config.guess config.sub ltmain.sh ; do
	if test -f "${_aux_dir}/${file}" ; then
	    $VERBOSE_ECHO "mv -f \"${_aux_dir}/${file}\" \"${_aux_dir}/${file}.backup\""
	    mv -f "${_aux_dir}/${file}" "${_aux_dir}/${file}.backup"
	fi
    done


    ############################
    # search alternate m4 dirs #
    ############################
    SEARCH_DIRS=""
    for dir in m4 ; do
	if [ -d $dir ] ; then
	    $VERBOSE_ECHO "Found extra aclocal search directory: $dir"
	    SEARCH_DIRS="$SEARCH_DIRS -I $dir"
	fi
    done


    ######################################
    # remove any previous build products #
    ######################################
    if test -d autom4te.cache ; then
	$VERBOSE_ECHO "Found an autom4te.cache directory, deleting it"
	$VERBOSE_ECHO "rm -rf autom4te.cache"
	rm -rf autom4te.cache
    fi
    if test -f aclocal.m4 ; then
	$VERBOSE_ECHO "Found an aclocal.m4 file, deleting it"
	$VERBOSE_ECHO "rm -f aclocal.m4"
	rm -f aclocal.m4
    fi

} # end of initialize()


##############
# initialize #
##############

# stash path
_prev_path="`pwd`"

# Before running autoreconf or manual steps, some prep detection work
# is necessary or useful.  Only needs to occur once per directory.
initialize


############################################
# prepare build via autoreconf or manually #
############################################
reconfigure_manually=no
if [ "x$HAVE_AUTORECONF" = "xyes" ] ; then
    $ECHO
    $ECHO $ECHO_N "Automatically preparing build ... $ECHO_C"

    $VERBOSE_ECHO "$AUTORECONF $SEARCH_DIRS $AUTORECONF_OPTIONS"
    autoreconf_output="`$AUTORECONF $SEARCH_DIRS $AUTORECONF_OPTIONS 2>&1`"
    ret=$?
    $VERBOSE_ECHO "$autoreconf_output"

    if [ ! $ret = 0 ] ; then
	$ECHO "Warning: $AUTORECONF failed"

	if test -f ltmain.sh ; then
	    $ECHO "libtoolize being run by autoreconf is not creating ltmain.sh in the auxillary directory like it should"
	fi

	$ECHO "Attempting to run the preparation steps individually"
	reconfigure_manually=yes
    fi
else
    reconfigure_manually=yes
fi


############################
# LIBTOOL_FAILURE FUNCTION #
############################
libtool_failure ( ) {
    _autoconf_output="$1"

    if [ "x$RUN_RECURSIVE" = "xno" ] ; then
	exit 5
    fi

    if test -f "$LIBTOOL_M4" ; then
	found_libtool="`$ECHO $_autoconf_output | grep AC_PROG_LIBTOOL`"
	if test ! "x$found_libtool" = "x" ; then
	    if test -f acinclude.m4 ; then
		if test ! -f acinclude.m4.backup ; then
		    $VERBOSE_ECHO cp acinclude.m4 acinclude.m4.backup
		    cp acinclude.m4 acinclude.m4.backup
		fi
	    fi
	    $VERBOSE_ECHO cat "$LIBTOOL_M4" >> acinclude.m4
	    cat "$LIBTOOL_M4" >> acinclude.m4

	    # don't keep doing this
	    RUN_RECURSIVE=no
	    export RUN_RECURSIVE

	    $ECHO
	    $ECHO "Restarting the preparation steps with a local libtool.m4"
	    $VERBOSE_ECHO sh $AUTOGEN_SH "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9"
	    sh "$AUTOGEN_SH" "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9"
	    exit $?
	fi
    fi
}


###########################
# MANUAL_AUTOGEN FUNCTION #
###########################
manual_autogen ( ) {
    ##################################################
    # Manual preparation steps taken are as follows: #
    #   aclocal [-I m4]                              #
    #   libtoolize --automake -c -f                  #
    #   aclocal [-I m4]                              #
    #   autoconf -f                                  #
    #   autoheader                                   #
    #   automake -a -c -f                            #
    ##################################################
    $ECHO
    $ECHO $ECHO_N "Preparing build ... $ECHO_C"

    if ! test -f configure.in -o -f configure.ac ; then
	$ECHO
	$ECHO
	$ECHO "A configure.ac or configure.in file could not be located implying"
	$ECHO "that the GNU Build System is at least not used in this directory.  In"
	$ECHO "any case, there is nothing to do here without one of those files."
	$ECHO
	$ECHO "ERROR: No configure.in or configure.ac file found."
	exit 1
    fi

    ###########
    # aclocal #
    ###########
    $VERBOSE_ECHO "$ACLOCAL $SEARCH_DIRS $ACLOCAL_OPTIONS"
    aclocal_output="`$ACLOCAL $SEARCH_DIRS $ACLOCAL_OPTIONS 2>&1`"
    ret=$?
    $VERBOSE_ECHO "$aclocal_output"
    if [ ! $ret = 0 ] ; then $ECHO "ERROR: $ACLOCAL failed" && exit 2 ; fi

    ##############
    # libtoolize #
    ##############
    if [ "x$HAVE_LIBTOOLIZE" = "xyes" ] ; then
	$VERBOSE_ECHO "$LIBTOOLIZE $LIBTOOLIZE_OPTIONS"
	libtoolize_output="`$LIBTOOLIZE $LIBTOOLIZE_OPTIONS 2>&1`"
	ret=$?
	$VERBOSE_ECHO "$libtoolize_output"

	if [ ! $ret = 0 ] ; then $ECHO "ERROR: $LIBTOOLIZE failed" && exit 2 ; fi
    else
	if [ "x$HAVE_ALT_LIBTOOLIZE" = "xyes" ] ; then
	    $VERBOSE_ECHO "$LIBTOOLIZE $ALT_LIBTOOLIZE_OPTIONS"
	    libtoolize_output="`$LIBTOOLIZE $ALT_LIBTOOLIZE_OPTIONS 2>&1`"
	    ret=$?
	    $VERBOSE_ECHO "$libtoolize_output"

	    if [ ! $ret = 0 ] ; then $ECHO "ERROR: $LIBTOOLIZE failed" && exit 2 ; fi
	fi
    fi

    ###########
    # aclocal #
    ###########
    # re-run again as instructed by libtoolize
    $VERBOSE_ECHO "$ACLOCAL $SEARCH_DIRS $ACLOCAL_OPTIONS"
    aclocal_output="`$ACLOCAL $SEARCH_DIRS $ACLOCAL_OPTIONS 2>&1`"
    ret=$?
    $VERBOSE_ECHO "$aclocal_output"

    # libtoolize might put ltmain.sh in the wrong place
    if test -f ltmain.sh ; then
	if test ! -f "${_aux_dir}/ltmain.sh" ; then
	    $ECHO
	    $ECHO "Warning:  $LIBTOOLIZE is creating ltmain.sh in the wrong directory"
	    $ECHO
	    $ECHO "Fortunately, the problem can be worked around by simply copying the"
	    $ECHO "file to the appropriate location (${_aux_dir}/).  This has been done for you."
	    $ECHO
	    $VERBOSE_ECHO "cp ltmain.sh \"${_aux_dir}/ltmain.sh\""
	    cp ltmain.sh "${_aux_dir}/ltmain.sh"
	    $ECHO $ECHO_N "Continuing build preparation ... $ECHO_C"
	fi
    fi

    ############
    # autoconf #
    ############
    $VERBOSE_ECHO
    $VERBOSE_ECHO "$AUTOCONF $AUTOCONF_OPTIONS"
    autoconf_output="`$AUTOCONF $AUTOCONF_OPTIONS 2>&1`"
    ret=$?
    $VERBOSE_ECHO "$autoconf_output"

    if [ ! $ret = 0 ] ; then
	# retry without the -f and check for usage of macros that are too new
	if test "x$HAVE_SED" = "xyes" ; then

	    ac2_59_macros="AC_C_RESTRICT AC_INCLUDES_DEFAULT AC_LANG_ASSERT AC_LANG_WERROR AS_SET_CATFILE"
	    ac2_55_macros="AC_COMPILER_IFELSE AC_FUNC_MBRTOWC AC_HEADER_STDBOOL AC_LANG_CONFTEST AC_LANG_SOURCE AC_LANG_PROGRAM AC_LANG_CALL AC_LANG_FUNC_TRY_LINK AC_MSG_FAILURE AC_PREPROC_IFELSE"
	    ac2_54_macros="AC_C_BACKSLASH_A AC_CONFIG_LIBOBJ_DIR AC_GNU_SOURCE AC_PROG_EGREP AC_PROG_FGREP AC_REPLACE_FNMATCH AC_FUNC_FNMATCH_GNU AC_FUNC_REALLOC AC_TYPE_MBSTATE_T"

	    macros_to_search=""
	    if [ $AUTOCONF_MAJOR_VERSION -lt 2 ] ; then
		macros_to_search="$ac2_59_macros $ac2_55_macros $ac2_54_macros"
	    else
		if [ $AUTOCONF_MINOR_VERSION -lt 54 ] ; then
		    macros_to_search="$ac2_59_macros $ac2_55_macros $ac2_54_macros"
		elif [ $AUTOCONF_MINOR_VERSION -lt 55 ] ; then
		    macros_to_search="$ac2_59_macros $ac2_55_macros"
		elif [ $AUTOCONF_MINOR_VERSION -lt 59 ] ; then
		    macros_to_search="$ac2_59_macros"
		fi
	    fi

	    configure_ac_macros=__none__
	    for feature in $macros_to_search ; do
		$VERBOSE_ECHO "Searching for $feature in $CONFIGURE"
		found="`grep \"^$feature.*\" $CONFIGURE`"
		if [ ! "x$found" = "x" ] ; then
		    if [ "x$configure_ac_macros" = "x__none__" ] ; then
			configure_ac_macros="$feature"
		    else
			configure_ac_macros="$feature $configure_ac_macros"
		    fi
		fi
	    done
	    if [ ! "x$configure_ac_macros" = "x__none__" ] ; then
		$ECHO
		$ECHO "Warning:  Unsupported macros were found in $CONFIGURE"
		$ECHO
		$ECHO "The $CONFIGURE file was scanned in order to determine if any"
		$ECHO "unsupported macros are used that exceed the minimum version"
		$ECHO "settings specified within this file.  As such, the following macros"
		$ECHO "should be removed from configure.ac or the version numbers in this"
		$ECHO "file should be increased:"
		$ECHO
		$ECHO "$configure_ac_macros"
		$ECHO
		$ECHO $ECHO_N "Ignorantly continuing build preparation ... $ECHO_C"
	    fi
	fi


	###################
	# autoconf, retry #
	###################
	$VERBOSE_ECHO
	$VERBOSE_ECHO "$AUTOCONF"
	autoconf_output="`$AUTOCONF 2>&1`"
	ret=$?
	$VERBOSE_ECHO "$autoconf_output"

	if [ ! $ret = 0 ] ; then
	    # test if libtool is busted
	    libtool_failure "$autoconf_output"

	    # let the user know what went wrong
	    cat <<EOF
$autoconf_output
EOF
	    $ECHO "ERROR: $AUTOCONF failed"
	    exit 2
	else
	    # autoconf sans -f and possibly sans unsupported options succeed so warn verbosely
	    $ECHO
	    $ECHO "Warning: autoconf seems to have succeeded by removing the following options:"
	    $ECHO "	AUTOCONF_OPTIONS=\"$AUTOCONF_OPTIONS\""
	    $ECHO
	    $ECHO "Removing those options should not be necessary and indicate some other"
	    $ECHO "problem with the build system.  The build preparation is highly suspect"
	    $ECHO "and may result in configuration or compilation errors.  Consider"
	    if [ "x$VERBOSE_ECHO" = "x:" ] ; then
		$ECHO "rerunning the build preparation with verbose output enabled."
		$ECHO "	$AUTOGEN_SH --verbose"
	    else
		$ECHO "reviewing the minimum GNU Autotools version settings contained in"
		$ECHO "this script along with the macros being used in your $CONFIGURE file."
	    fi
	    $ECHO
	    $ECHO $ECHO_N "Continuing build preparation ... $ECHO_C"
	fi
    fi

    ##############
    # autoheader #
    ##############
    $VERBOSE_ECHO "$AUTOHEADER $AUTOHEADER_OPTIONS"
    autoheader_output="`$AUTOHEADER $AUTOHEADER_OPTIONS 2>&1`"
    ret=$?
    $VERBOSE_ECHO "$autoheader_output"
    if [ ! $ret = 0 ] ; then $ECHO "ERROR: $AUTOHEADER failed" && exit 2 ; fi

    ############
    # automake #
    ############
    $VERBOSE_ECHO "$AUTOMAKE $AUTOMAKE_OPTIONS"
    automake_output="`$AUTOMAKE $AUTOMAKE_OPTIONS 2>&1`"
    ret=$?
    $VERBOSE_ECHO "$automake_output"

    if [ ! $ret = 0 ] ; then
	###################
	# automake, retry #
	###################
	$VERBOSE_ECHO
	$VERBOSE_ECHO "$AUTOMAKE $ALT_AUTOMAKE_OPTIONS"
	# retry without the -f
	automake_output="`$AUTOMAKE $ALT_AUTOMAKE_OPTIONS 2>&1`"
	ret=$?
	$VERBOSE_ECHO "$automake_output"

	if [ ! $ret = 0 ] ; then

	    # test if libtool is busted
	    libtool_failure "$automake_output"

	    # let the user know what went wrong
	    cat <<EOF
$automake_output
EOF
	fi
	$ECHO "ERROR: $AUTOMAKE failed"
	exit 2
    fi
}


##################################
# run manual preparation steps #
##################################
if [ "x$reconfigure_manually" = "xyes" ] ; then

    # XXX if this is a recursive configure, manual steps don't work
    # yet .. assume it's the libtool/glibtool problem.
    if [ ! "x$CONFIG_SUBDIRS" = "x" ] ; then
	$ECHO "Running the preparation steps individually does not yet work"
	$ECHO "well with a recursive configure."
	if [ ! "x$HAVE_ALT_LIBTOOLIZE" = "xyes" ] ; then
	    exit 3
	fi
	$ECHO "Assuming this is a libtoolize problem..."
	export LIBTOOLIZE
	RUN_RECURSIVE=no
	export RUN_RECURSIVE
	$ECHO
	$ECHO "Restarting the preparation steps with LIBTOOLIZE set to $LIBTOOLIZE"
	$VERBOSE_ECHO sh $AUTOGEN_SH "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9"
	sh "$AUTOGEN_SH" "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9"
	exit $?
    fi

    # run the build preparation steps manually for this directory
    manual_autogen

    # for projects using recursive configure, run the build
    # preparation steps for the subdirectories.
    for dir in $CONFIG_SUBDIRS ; do
	$VERBOSE_ECHO "Processing recursive configure in $dir"
	cd "$_prev_path"
	cd "$dir"
	manual_autogen
    done
fi


#########################################
# restore COPYING & INSTALL from backup #
#########################################
spacer=no
if test -f COPYING ; then
    # compare entire content, restore if needed
    if test "x$COPYING_BACKUP" != "x__none__" -a "x`cat COPYING`" != "x$COPYING_BACKUP" ; then
	if test "x$spacer" = "xno" ; then
	    $VERBOSE_ECHO
	    spacer=yes
	fi
	# restore the backup
	$VERBOSE_ECHO "Restoring COPYING from backup (automake -f likely clobbered it)"
	$VERBOSE_ECHO "echo \"\$COPYING_BACKUP\" > COPYING"
	echo "$COPYING_BACKUP" > COPYING
	# release content
	COPYING_BACKUP=""
	export COPYING_BACKUP
    fi
fi
if test -f INSTALL ; then
    # compare entire content, restore if needed
    if test "x$INSTALL_BACKUP" != "x__none__" -a "x`cat INSTALL`" != "x$INSTALL_BACKUP" ; then
	if test "x$spacer" = "xno" ; then
	    $VERBOSE_ECHO
	    spacer=yes
	fi
	# restore the backup
	$VERBOSE_ECHO "Restoring INSTALL from backup (automake -f likely clobbered it)"
	$VERBOSE_ECHO "echo \"\$INSTALL_BACKUP\" > INSTALL"
	echo "$INSTALL_BACKUP" > INSTALL
	# release content
	INSTALL_BACKUP=""
	export INSTALL_BACKUP
    fi
fi


#########################
# restore and summarize #
#########################
cd "$_prev_path"
$ECHO "done"
$ECHO
$ECHO "The $PROJECT build system is now prepared.  To build here, run:"
$ECHO "  $PATH_TO_AUTOGEN/configure"
$ECHO "  make"


# Local Variables:
# mode: sh
# tab-width: 8
# sh-basic-offset: 4
# sh-indentation: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
