#
# - Find perplex executable and provides macros to generate custom build rules
# The module defines the following variables
#
#  PERPLEX_EXECUTABLE - path to the perplex program
#
# #====================================================================
#  Example:
#
#   find_package(LEMON)
#   find_package(RE2C)
#   find_package(PERPLEX)
#
#   LEMON_TARGET(MyParser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
#   PERPLEX_TARGET(MyScanner scanner.re  ${CMAKE_CURRENT_BIANRY_DIR}/scanner.cpp ${CMAKE_CURRENT_BINARY_DIR}/scanner_header.hpp)
#   ADD_PERPLEX_LEMON_DEPENDENCY(MyScanner MyParser)
#
#   include_directories(${CMAKE_CURRENT_BINARY_DIR})
#   add_executable(Foo
#      Foo.cc
#      ${LEMON_MyParser_OUTPUTS}
#      ${PERPLEX_MyScanner_OUTPUTS}
#   )
#  ====================================================================
# 
#=============================================================================
#               F I N D P E R P L E X . C M A K E
#
# Originally based off of FindBISON.cmake from Kitware's CMake distribution
#
# Copyright (c) 2010-2012 United States Government as represented by
#                the U.S. Army Research Laboratory.
# Copyright 2009 Kitware, Inc.
# Copyright 2006 Tristan Carel
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# 
# * The names of the authors may not be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

find_program(PERPLEX_EXECUTABLE perplex_path DOC "path to the perplex executable")
mark_as_advanced(PERPLEX_EXECUTABLE)
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PERPLEX DEFAULT_MSG PERPLEX_EXECUTABLE)

#============================================================
# FindPERPLEX.cmake ends here

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
