#!/bin/sh
#                        a u t o g e n . s h
#
# Script for automatically preparing the sources for compilation by
# performing the myrid of necessary steps.  The script attempts to
# detect proper version support, and outputs warnings about particular
# systems that have autotool peculiarities.
#
# Author: Christopher Sean Morrison
# This script is in the public domain
#
######################################################################

ARGS="$*"
PATH_TO_AUTOGEN="`dirname $0`"

SUITE="BRL-CAD"

AUTOCONF_MAJOR_VERSION=2
AUTOCONF_MINOR_VERSION=50
AUTOCONF_PATCH_VERSION=0

# unused for now but informative
AUTOMAKE_MAJOR_VERSION=1
AUTOMAKE_MINOR_VERSION=4
AUTOMAKE_PATCH_VERSION=0


#####################
# environment check #
#####################
_have_sed="`echo no | sed 's/no/yes/'`"
HAVE_SED=no
if [ $? = 0 ] ; then
  [ "x$_have_sed" = "xyes" ] && HAVE_SED=yes
fi
case `echo "testing\c"; echo 1,2,3`,`echo -n testing; echo 1,2,3` in
  *c*,-n*) ECHO_N= ECHO_C='
' ECHO_T='	' ;;
  *c*,*  ) ECHO_N=-n ECHO_C= ECHO_T= ;;
  *)       ECHO_N= ECHO_C='\c' ECHO_T= ;;
esac


##########################
# autoconf version check #
##########################
_autofound=no
for AUTOCONF in autoconf ; do
  $AUTOCONF --version > /dev/null 2>&1
  if [ $? = 0 ] ; then
    _autofound=yes
    break
  fi
done

_report_error=no
if [ ! "x$_autofound" = "xyes" ] ; then
  echo "ERROR:  Unable to locate GNU Autoconf."
  _report_error=yes
else
  _version_line="`$AUTOCONF --version | head -n 1`"
  if [ "x$HAVE_SED" = "xyes" ] ; then
    _maj_version="`echo $_version_line | sed 's/.*\([0-9]\)\..*/\1/'`"
    _min_version="`echo $_version_line | sed 's/.*\.\([0-9][0-9]\).*/\1/'`"
    if [ $? = 0 ] ; then
      if [ $_maj_version -lt $AUTOCONF_MAJOR_VERSION ] ; then
	_report_error=yes
      elif [ $_min_version -lt $AUTOCONF_MINOR_VERSION ] ; then
	_report_error=yes
      fi
    fi
    echo "Found GNU Autoconf version $_maj_version.$_min_version"
  else
    echo "Warning:  sed is not available to properly detect version of GNU Autoconf"
  fi
  echo
fi
if [ "x$_report_error" = "xyes" ] ; then
  echo "ERROR:  To prepare the ${SUITE} build system from scratch,"
  echo "        at least version $AUTOCONF_MAJOR_VERSION.$AUTOCONF_MINOR_VERSION of GNU Autoconf must be installed."
  echo 
  echo "$PATH_TO_AUTOGEN/autogen.sh does not need to be run on the same machine that will"
  echo "run configure or make.  Either the GNU Autotools will need to be installed"
  echo "or upgraded on this system, or $PATH_TO_AUTOGEN/autogen.sh must be run on the source"
  echo "code on another system and then transferred to here. -- Cheers!"
  exit 1
fi


########################
# check for autoreconf #
########################
HAVE_AUTORECONF=no
for AUTORECONF in autoreconf ; do
  $AUTORECONF --version > /dev/null 2>&1
  if [ $? = 0 ] ; then
    HAVE_AUTORECONF=yes
    break
  fi
done


#####################
# check for aclocal #
#####################
for ACLOCAL in aclocal ; do
  $ACLOCAL --version > /dev/null 2>&1
  if [ $? = 0 ] ; then
    break
  fi
done


########################
# check for libtoolize #
########################
HAVE_LIBTOOLIZE=yes
HAVE_ALTLIBTOOLIZE=no
LIBTOOLIZE=libtoolize
$LIBTOOLIZE --version > /dev/null 2>&1
if [ ! $? = 0 ] ; then
  HAVE_LIBTOOLIZE=no
  if [ "x$HAVE_AUTORECONF" = "xno" ] ; then
    echo "Warning:  libtoolize does not appear to be available."
  else
    echo "Warning:  libtoolize does not appear to be available.  This means that"
    echo "autoreconf cannot be used."
  fi

  # look for some alternates
  for tool in glibtoolize libtoolize15 libtoolize13 ; do
    _glibtoolize="`$tool --version > /dev/null 2>&1`"
    if [ $? = 0 ] ; then
      HAVE_ALTLIBTOOLIZE=yes
      LIBTOOLIZE="$tool"
      echo 
      echo "Fortunately, $tool was found which means that your system may simply"
      echo "have a non-standard or incomplete GNU Autotools install.  If you have"
      echo "sufficient system access, it may be possible to quell this warning by"
      echo "running:"
      echo
      sudo -V > /dev/null 2>&1
      if [ $? = 0 ] ; then
	_glti="`which $tool`"
	_gltidir="`dirname $_glti`"
	echo "   sudo ln -s $_glti $_gltidir/libtoolize"
      else
	echo "   ln -s $glti $_gltidir/libtoolize"
	echo 
	echo "Run that as root or with proper permissions to the $_gltidir directory"
      fi
      echo
      break
    fi
  done
fi


########################
# check for autoheader #
########################
for AUTOHEADER in autoheader ; do
  $AUTOHEADER --version > /dev/null 2>&1
  if [ $? = 0 ] ; then
    break
  fi
done


######################
# check for automake #
######################
for AUTOMAKE in automake ; do
  $AUTOMAKE --version > /dev/null 2>&1
  if [ $? = 0 ] ; then
    break
  fi
done


##############
# stash path #
##############
_prev_path="`pwd`"
cd "$PATH_TO_AUTOGEN"


#####################
# detect an aux dir #
#####################
_aux_dir=.
_configure_file=/dev/null
if test -f configure.ac ; then
  _configure_file=configure.ac
elif test -f configure.in ; then
  _configure_file=configure.in
fi
_aux_dir="`cat $_configure_file | grep AC_CONFIG_AUX_DIR | tail -n 1 | sed 's/^[ ]*AC_CONFIG_AUX_DIR(\(.*\)).*/\1/'`"
if test ! -d "$_aux_dir" ; then
  _aux_dir=.
fi


##########################################
# make sure certain required files exist #
##########################################

for file in AUTHORS COPYING ChangeLog INSTALL NEWS README ; do
  if test ! -f $file ; then
    touch $file
  fi
done


############################################
# protect COPYING & INSTALL from overwrite #
############################################
for file in COPYING INSTALL ; do
  if test -f $file ; then
    if test -d "${_aux_dir}" ; then
      if test ! -f "${_aux_dir}/${file}.backup" ; then
	cp -pf ${file} "${_aux_dir}/${file}.backup"
      fi
    fi
  fi
done


##################################################
# make sure certain generated files do not exist #
##################################################
for file in config.guess config.sub ltmain.sh ; do
  if test -f "${_aux_dir}/${file}" ; then
    mv -f "${_aux_dir}/${file}" "${_aux_dir}/${file}.backup"
  fi
done


############################################
# prepare build via autoreconf or manually #
############################################
reconfigure_manually=no
if [ "x$HAVE_AUTORECONF" = "xyes" ] && [ "x$HAVE_LIBTOOLIZE" = "xyes" ] ; then
  echo $ECHO_N "Automatically preparing build ... $ECHO_C"
  $AUTORECONF -i -f > /dev/null 2>&1
  if [ ! $? = 0 ] ; then
    echo "Warning: $AUTORECONF failed"

    if test -f ltmain.sh ; then
      echo "libtoolize being run by autoreconf is not creating ltmain.sh in the auxillary directory like it should"
    fi

    echo "Attempting to run the configuration steps individually"
    reconfigure_manually=yes
  fi
else
  reconfigure_manually=yes
fi

###
# Steps taken are as follows:
#  aclocal
#  libtoolize --automake -c -f
#  aclocal          
#  autoconf -f
#  autoheader
#  automake -a -c -f
####
if [ "x$reconfigure_manually" = "xyes" ] ; then
  echo $ECHO_N "Preparing build ... $ECHO_C"

  $ACLOCAL

  [ ! $? = 0 ] && echo "ERROR: $ACLOCAL failed" && exit 2
  if [ "x$HAVE_LIBTOOLIZE" = "xyes" ] ; then 
    $LIBTOOLIZE --automake -c -f
    [ ! $? = 0 ] && echo "ERROR: $LIBTOOLIZE failed" && exit 2
  else
    if [ "x$HAVE_ALTLIBTOOLIZE" = "xyes" ] ; then
      $LIBTOOLIZE --automake --copy --force
      [ ! $? = 0 ] && echo "ERROR: $LIBTOOLIZE failed" && exit 2
    fi
  fi

  # re-run again as instructed by libtoolize
  $ACLOCAL

  # libtoolize might put ltmain.sh in the wrong place
  if test -f ltmain.sh ; then
    if test ! -f "${_aux_dir}/ltmain.sh" ; then
      echo
      echo "Warning:  $LIBTOOLIZE is creating ltmain.sh in the wrong directory"
      echo
      echo "Fortunately, the problem can be worked around by simply copying the"
      echo "file to the appropriate location (${_aux_dir}/).  This has been done for you."
      echo
      cp ltmain.sh "${_aux_dir}/ltmain.sh"
      echo $ECHO_N "Continuing build preparation ... $ECHO_C"
    fi
  fi

  $AUTOCONF -f
  [ ! $? = 0 ] && echo "ERROR: $AUTOCONF failed" && exit 2
  
  $AUTOHEADER
  [ ! $? = 0 ] && echo "ERROR: $AUTOHEADER failed" && exit 2

  $AUTOMAKE -a -c -f
  [ ! $? = 0 ] && echo "ERROR: $AUTOMAKE failed" && exit 2
fi


#########################################
# restore COPYING & INSTALL from backup #
#########################################
for file in COPYING INSTALL ; do
  if test -f $file ; then
    if test -f "${_aux_dir}/${file}.backup" ; then
      cp -pf "${_aux_dir}/${file}.backup" ${file}
    fi
  fi
done


################
# restore path #
################
cd "$_prev_path"
echo "done"
echo


###############################################
# check for help arg, and bypass running make #
###############################################
_help=no
[ "x$HAVE_SED" = "xyes" ] && [ "x`echo $ARGS | sed 's/.*help.*/help/'`" = "xhelp" ] && _help=yes
[ "x$ARGS" = "x--help" ] && _help=yes
if [ "x$_help" = "xyes" ] ; then
  echo "Help was requested.  No configuration and compilation will be done."
  echo "Running: $PATH_TO_AUTOGEN/configure $ARGS"
  $PATH_TO_AUTOGEN/configure $ARGS
  exit 0
fi


##################################################
# summarize.  actually build if arg was provided #
##################################################
if [ ! "x$ARGS" = "x" ] ; then
  echo "The ${SUITE} build system is now prepared.  Building here with:"
  echo "$PATH_TO_AUTOGEN/configure $ARGS"
  $PATH_TO_AUTOGEN/configure $ARGS
  [ ! $? = 0 ] && echo "ERROR: configure failed" && exit $?
  make
  [ ! $? = 0 ] && echo "ERROR: make failed" && exit $?
  exit 0
fi

echo "The ${SUITE} build system is now prepared.  To build here, run:"
echo "  $PATH_TO_AUTOGEN/configure"
echo "  make"

# Local Variables:
# mode: sh
# tab-width: 8
# sh-basic-offset: 4
# sh-indentation: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
