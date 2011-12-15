#            T H I R D P A R T Y _ T C L . C M A K E
# BRL-CAD
#
# Copyright (c) 2011 United States Government as represented by
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

#-----------------------------------------------------------------------------
MACRO(THIRD_PARTY_TCL_PACKAGE packagename dir wishcmd depends)
    STRING(TOUPPER ${packagename} PKGNAME_UPPER)
    SET(ENABLE_PKG ${${CMAKE_PROJECT_NAME}_TCL_BUILD})
    SET(NEEDS_TK 0) 
    FOREACH(item ${depends})
	IF(${item} STREQUAL "tk")
	    SET(NEEDS_TK 1)
	ENDIF(${item} STREQUAL "tk")
    ENDFOREACH(item ${depends})
    # If we need Tk and it's not available, this extension is a no-go
    IF(NEEDS_TK AND NOT BRLCAD_ENABLE_TK)
	SET(ENABLE_PKG OFF)
	SET(DISABLE_TEST 1)
	SET(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD OFF)
    ENDIF(NEEDS_TK AND NOT BRLCAD_ENABLE_TK)

    # If we are doing a local Tcl build, default to bundled unless we need Tk and Tk isn't enabled.
    BUNDLE_OPTION(${CMAKE_PROJECT_NAME}_TCL ENABLE_PKG ${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} "") 

    IF(NOT DISABLE_TEST)
	IF(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} MATCHES "BUNDLED")
	    SET(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD ON)
	ELSE(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} MATCHES "BUNDLED")
	    SET(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD OFF)

	    # Stash the previous results (if any) so we don't repeatedly call out the tests - only report
	    # if something actually changes in subsequent runs.
	    SET(${PKGNAME_UPPER}_FOUND_STATUS ${${PKGNAME_UPPER}_FOUND})

	    IF(${wishcmd} STREQUAL "")
		SET(${PKGNAME_UPPER}_FOUND "${PKGNAME_UPPER}-NOTFOUND" CACHE STRING "${PKGNAME_UPPER}_FOUND" FORCE)

		IF(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} MATCHES "SYSTEM")
		    IF("${${PKGNAME_UPPER}_FOUND_STATUS}" MATCHES "NOTFOUND" AND NOT ${PKGNAME_UPPER}_FOUND)
			MESSAGE(WARNING "No tclsh/wish command available for testing, but system version of ${packagename} is requested - assuming availability of package.")
		    ENDIF("${${PKGNAME_UPPER}_FOUND_STATUS}" MATCHES "NOTFOUND" AND NOT ${PKGNAME_UPPER}_FOUND)
		ELSE(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} MATCHES "SYSTEM")
		    # Can't test and we're not forced to system by either local settings or the toplevel - turn it on 
		    SET(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD ON)
		ENDIF(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} MATCHES "SYSTEM")
	    ELSE(${wishcmd} STREQUAL "")
		SET(packagefind_script "
catch {package require ${packagename}}
set packageversion NOTFOUND
set packageversion [lindex [lsort -decreasing [package versions ${packagename}]] 0]
set filename \"${CMAKE_BINARY_DIR}/CMakeTmp/${PKGNAME_UPPER}_PKG_VERSION\"
set fileId [open $filename \"w\"]
puts $fileId $packageversion
close $fileId
exit
")
		SET(packagefind_scriptfile "${CMAKE_BINARY_DIR}/CMakeTmp/${packagename}_packageversion.tcl")
		FILE(WRITE ${packagefind_scriptfile} ${packagefind_script})
		EXEC_PROGRAM(${wishcmd} ARGS ${packagefind_scriptfile} OUTPUT_VARIABLE EXECOUTPUT)
		FILE(READ ${CMAKE_BINARY_DIR}/CMakeTmp/${PKGNAME_UPPER}_PKG_VERSION pkgversion)
		STRING(REGEX REPLACE "\n" "" ${PKGNAME_UPPER}_PACKAGE_VERSION ${pkgversion})

		IF(${PKGNAME_UPPER}_PACKAGE_VERSION)
		    SET(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD OFF)
		    SET(${PKGNAME_UPPER}_FOUND "TRUE" CACHE STRING "${PKGNAME_UPPER}_FOUND" FORCE)
		ELSE(${PKGNAME_UPPER}_PACKAGE_VERSION)
		    SET(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD ON)
		    SET(${PKGNAME_UPPER}_FOUND "${PKGNAME_UPPER}-NOTFOUND" CACHE STRING "${PKGNAME_UPPER}_FOUND" FORCE)
		ENDIF(${PKGNAME_UPPER}_PACKAGE_VERSION)

		# Be quiet if we're doing this over
		IF("${${PKGNAME_UPPER}_FOUND_STATUS}" MATCHES "${PKGNAME_UPPER}-NOTFOUND" AND NOT ${PKGNAME_UPPER}_FOUND)
		    SET(${PKGNAME_UPPER}_FIND_QUIETLY TRUE)
		ENDIF("${${PKGNAME_UPPER}_FOUND_STATUS}" MATCHES "${PKGNAME_UPPER}-NOTFOUND" AND NOT ${PKGNAME_UPPER}_FOUND)

		FIND_PACKAGE_HANDLE_STANDARD_ARGS(${PKGNAME_UPPER} DEFAULT_MSG ${PKGNAME_UPPER}_PACKAGE_VERSION)
	    ENDIF(${wishcmd} STREQUAL "")
	ENDIF(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER} MATCHES "BUNDLED")
    ENDIF(NOT DISABLE_TEST)

    IF(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD)
	STRING(TOLOWER ${packagename} PKGNAME_LOWER)
	add_subdirectory(${dir})

	FOREACH(dep ${depends})
	    string(TOUPPER ${dep} DEP_UPPER)
	    if(BRLCAD_BUILD_${DEP_UPPER})
		add_dependencies(${packagename} ${dep})
	    endif(BRLCAD_BUILD_${DEP_UPPER})
	ENDFOREACH(dep ${depends})

	INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/${PKGNAME_LOWER}.dist)
	DISTCHECK_IGNORE(${dir} ${PKGNAME_LOWER}_ignore_files)
    ELSE(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD)
	DISTCHECK_IGNORE_ITEM(${dir})
    ENDIF(${CMAKE_PROJECT_NAME}_${PKGNAME_UPPER}_BUILD)

    MARK_AS_ADVANCED(${PKGNAME_UPPER}_FOUND)
ENDMACRO(THIRD_PARTY_TCL_PACKAGE)

# Local Variables:
# tab-width: 8
# mode: sh
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
