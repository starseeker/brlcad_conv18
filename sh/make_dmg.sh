#!/bin/sh
#                     M A K E _ D M G . S H
# BRL-CAD
#
# Copyright (c) 2005 United States Government as represented by
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
# Script for generating a Mac OS X Installer package (.pkg) from a
# clean package installation.  The package should be compatible with
# old and new versions of Installer.
#
# Author: Christopher Sean Morrison
#
######################################################################

NAME="$1"
MAJOR_VERSION="$2"
MINOR_VERSION="$3"
PATCH_VERSION="$4"
CONTENT="$5"

if [ "x$NAME" = "x" ] ; then
    echo "Usage: $0 title major_version minor_version patch_version [content]"
    echo "ERROR: must specify a package name"
    exit 1
fi
if [ "x$MINOR_VERSION" = "x" ] ; then
    echo "Usage: $0 title major_version minor_version patch_version [content]"
    echo "ERROR: must specify a major package version"
    exit 1
fi
if [ "x$MINOR_VERSION" = "x" ] ; then
    echo "ERROR: must specify a minor package version"
    echo "Usage: $0 title major_version minor_version patch_version [content]"
    exit 1
fi
if [ "x$PATCH_VERSION" = "x" ] ; then
    echo "Usage: $0 title major_version minor_version patch_version [content]"
    echo "ERROR: must specify a patch package version"
    exit 1
fi
if [ "x$ARCHIVE" = "x" ] ; then
    echo "Usage: $0 title major_version minor_version patch_version [content]"
    echo "ERROR: must specify an archive directory"
    exit 1
fi
if [ ! -r "$CONTENT" ] ; then
    echo "ERROR: specified content (${CONTENT}) is not readable"
    exit 1
fi

if [ "x`id -u`" != "x0" ] ; then
    echo "This script requires superuser privileges, restarting via sudo"
    sudo "$0" "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9"
    exit $?
fi

VERSION="${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}"
DMG_NAME="${NAME}-${VERSION}"


# Local Variables:
# mode: sh
# tab-width: 8
# sh-indentation: 4
# sh-basic-offset: 4
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
