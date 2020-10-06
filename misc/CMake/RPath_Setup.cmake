# Copyright (c) 2010-2020 United States Government as represented by
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

#---------------------------------------------------------------------
# The following logic is what allows binaries to run successfully in
# the build directory AND install directory.
# http://www.cmake.org/Wiki/CMake_RPATH_handling

# TODO - investigate using BUILD_RPATH and INSTALL_RPATH on a per
# target basis for more precise control (for example, when setting
# up paths for libs in subdirs...  not sure yet if it could be helpful
# but should be explored.

# Note in particular that BUILD_RPATH supports generator expressions,
# in case that's of use to pull other properties on which to base
# rpaths...


include(CMakeParseArguments)

# For a given path, calculate the $ORIGIN style path needed relative
# to CMAKE_INSTALL_PREFIX
function(SUFFIX_STRING POPATH INIT_PATH)

  get_filename_component(CPATH "${INIT_PATH}" REALPATH)
  set(RELDIRS)
  set(FPATH)
  while (NOT "${CPATH}" STREQUAL "${CMAKE_INSTALL_PREFIX}")
    get_filename_component(CDIR "${CPATH}" NAME)
    get_filename_component(CPATH "${CPATH}" DIRECTORY)
    if (NOT "${RELDIRS}" STREQUAL "")
      set(RELDIRS "${CDIR}/${RELDIRS}")
      set(FPATH "../${FPATH}")
    else (NOT "${RELDIRS}" STREQUAL "")
      set(RELDIRS "${CDIR}")
      set(FPATH "../")
    endif (NOT "${RELDIRS}" STREQUAL "")
  endwhile()

  set(FPATH "${FPATH}${RELDIRS}")

  set(${POPATH} ${FPATH} PARENT_SCOPE)
endfunction(SUFFIX_STRING)


if(NOT COMMAND cmake_set_rpath)

  function(cmake_set_rpath)

    # See if we have a suffix for the paths
    cmake_parse_arguments(R "" "SUFFIX" "" ${ARGN})

    # We want the full RPATH set in the build tree so we can run programs without
    # needing to set LD_LIBRARY_PATH
    set(CMAKE_SKIP_BUILD_RPATH FALSE PARENT_SCOPE)

    # We DON'T want the final install directory RPATH set in the build directory
    # - it should only be set to the installation value when actually installed.
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE PARENT_SCOPE)

    # Add the automatically determined parts of the RPATH which point to
    # directories outside the build tree to the install RPATH
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE PARENT_SCOPE)

    # Set an initial RPATH to use for length calculations
    if(NOT R_SUFFIX)
      set(LSUFFIX "${LIB_DIR}")
    else(NOT R_SUFFIX)
      set(LSUFFIX "${LIB_DIR}/${R_SUFFIX}")
    endif(NOT R_SUFFIX)

    # Calculate how many ../ offsets are needed to return from this directory
    # to the install origin
    set(OPATH)
    SUFFIX_STRING(OPATH "${CMAKE_INSTALL_PREFIX}/${LSUFFIX}")

    # Set RPATH value to use when installing.  This should be set to always
    # prefer the version in the installed path when possible, but fall back on a
    # location relative to the loading file's path if the installed version is
    # not present.  How to do so is platform specific.
    if(NOT APPLE)
      set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LSUFFIX}:\$ORIGIN/${OPATH}/${LSUFFIX}")
    else(NOT APPLE)
      set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LSUFFIX};@loader_path/${OPATH}/${LSUFFIX}")
    endif(NOT APPLE)

    # Determine what the build time RPATH will be that is used to support
    # CMake's RPATH manipulation, so it can be used in external projects.

    # TODO - need to check behavior when the BUILD_RPATH is longer than the INSTALL_RPATH -
    # it should work, but the install rpath won't overwrite the extra chars with : as is - we
    # may need to do that to make sure only the correct paths are present
    set(CMAKE_BUILD_RPATH "${CMAKE_BINARY_DIR}/${LSUFFIX}")
    string(LENGTH "${CMAKE_INSTALL_RPATH}" INSTALL_LEN)
    string(LENGTH "${CMAKE_BUILD_RPATH}" CURR_LEN)
    while("${CURR_LEN}" LESS "${INSTALL_LEN}")
      # This is the key to the process - the ":" characters appended to the
      # build time path result in a path string in the compile outputs that
      # has sufficient length to hold the install directory, while is what
      # allows CMake's file command to manipulate the paths.  At the same time,
      # the colon lengthened paths do not break the functioning of the shorter
      # build path.  Normally this is an internal CMake detail, but we need it
      # to supply to external build systems so their outputs can be manipulated
      # as if they were outputs of our own build.
      set(CMAKE_BUILD_RPATH "${CMAKE_BUILD_RPATH}:")
      string(LENGTH "${CMAKE_BUILD_RPATH}" CURR_LEN)
    endwhile("${CURR_LEN}" LESS "${INSTALL_LEN}")

    # Done - let the parent know what the answers are
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}" PARENT_SCOPE)
    set(CMAKE_BUILD_RPATH "${CMAKE_BUILD_RPATH}" PARENT_SCOPE)

  endfunction(cmake_set_rpath)

endif(NOT COMMAND cmake_set_rpath)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
