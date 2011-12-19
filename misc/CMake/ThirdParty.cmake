#                T H I R D P A R T Y . C M A K E
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
MACRO(THIRD_PARTY lower dir aliases description required_vars)
	STRING(TOUPPER ${lower} upper)
	SET(ENABLE_PKG ${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS})

	# If any of the required flags are off, this extension is a no-go.
	SET(DISABLE_TEST 0)
	FOREACH(item ${required_vars})
	    IF(NOT ${item})
		SET(ENABLE_PKG OFF)
		SET(DISABLE_TEST 1)
		SET(${CMAKE_PROJECT_NAME}_${upper}_BUILD OFF)
	    ENDIF(NOT ${item})
	ENDFOREACH(item ${required_vars})

	BRLCAD_BUNDLE_OPTION(${CMAKE_PROJECT_NAME}_BUNDLED_LIBS ENABLE_PKG ${CMAKE_PROJECT_NAME}_${upper} "" ${aliases} ${description})

	IF(NOT DISABLE_TEST)
	    FOREACH(extraarg ${ARGN})
		IF(extraarg STREQUAL "NOSYS")
		    IF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
			SET(${CMAKE_PROJECT_NAME}_${upper} "BUNDLED (AUTO)" CACHE STRING "NOSYS passed, using bundled ${lower}" FORCE)
		    ENDIF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
		ENDIF(extraarg STREQUAL "NOSYS")
	    ENDFOREACH(extraarg ${ARGN})

	    # Main search logic
	    IF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "BUNDLED")

		# BUNDLED or BUNDLED (AUTO), figure out which

		IF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
		    SET(${CMAKE_PROJECT_NAME}_${upper} "BUNDLED (AUTO)" CACHE STRING "Automatically using bundled" FORCE)
		ELSE(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
		    SET(${CMAKE_PROJECT_NAME}_${upper} "BUNDLED" CACHE STRING "Using bundled" FORCE)
		ENDIF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")

		# turn it on
		SET(${CMAKE_PROJECT_NAME}_${upper}_BUILD ON)
		SET(${upper}_LIBRARY "${lower}" CACHE STRING "set by THIRD_PARTY macro" FORCE)

	    ELSE(${CMAKE_PROJECT_NAME}_${upper} MATCHES "BUNDLED")

		# AUTO or SYSTEM (AUTO) or SYSTEM, figure out which

		# Stash the previous results (if any) so we don't repeatedly call out the tests - only report
		# if something actually changes in subsequent runs.
		SET(${upper}_FOUND_STATUS ${${upper}_FOUND})

		# Initialize (or rather, uninitialize) variables in preparation for search
		SET(${upper}_FOUND "${upper}-NOTFOUND" CACHE STRING "${upper}_FOUND" FORCE)
		MARK_AS_ADVANCED(${upper}_FOUND)
		SET(${upper}_LIBRARY "${upper}-NOTFOUND" CACHE STRING "${upper}_LIBRARY" FORCE)
		SET(${upper}_INCLUDE_DIR "${upper}-NOTFOUND" CACHE STRING "${upper}_INCLUDE_DIR" FORCE)

		# Be quiet if we're doing this over
		IF("${${upper}_FOUND_STATUS}" MATCHES "NOTFOUND")
		    SET(${upper}_FIND_QUIETLY TRUE)
		ENDIF("${${upper}_FOUND_STATUS}" MATCHES "NOTFOUND")

		# Include the Find module for the library in question
		IF(EXISTS ${${CMAKE_PROJECT_NAME}_CMAKE_DIR}/Find${upper}.cmake)
		    INCLUDE(${${CMAKE_PROJECT_NAME}_CMAKE_DIR}/Find${upper}.cmake)
		ELSE(EXISTS ${${CMAKE_PROJECT_NAME}_CMAKE_DIR}/Find${upper}.cmake)
		    INCLUDE(${CMAKE_ROOT}/Modules/Find${upper}.cmake)
		ENDIF(EXISTS ${${CMAKE_PROJECT_NAME}_CMAKE_DIR}/Find${upper}.cmake)

		# handle conversion of *AUTO to indicate whether it's
		# going to use system or bundled versions of deps
		IF(${upper}_FOUND)

		    # turn it off, found it
		    SET(${CMAKE_PROJECT_NAME}_${upper}_BUILD OFF)
		    SET(${upper}_FOUND "TRUE" CACHE STRING "${upper}_FOUND" FORCE)

		    # found system-installed 3rd-party dep
		    IF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
			SET(${CMAKE_PROJECT_NAME}_${upper} "SYSTEM (AUTO)" CACHE STRING "Automatically using system, ${lower} found" FORCE)
		    ELSE(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
			# not auto, must be SYSTEM
			SET(${CMAKE_PROJECT_NAME}_${upper} "SYSTEM" CACHE STRING "Using system, ${lower} found" FORCE)
		    ENDIF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")

		ELSE(${upper}_FOUND)

		    # turn it on, didn't find it
		    SET(${CMAKE_PROJECT_NAME}_${upper}_BUILD ON)
		    SET(${upper}_LIBRARY "${lower}" CACHE STRING "set by THIRD_PARTY macro" FORCE)

		    # did NOT find system-installed 3rd-party dep
		    IF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
			IF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "SYSTEM")
			    SET(${CMAKE_PROJECT_NAME}_${upper} "SYSTEM (AUTO)" CACHE STRING "Automatically attempting to use system even though ${lower} was NOT found" FORCE)
			    MESSAGE(WARNING "Configuring to use system ${lower} even though it was NOT found")

			    # turn it off even though we didn't find it
			    SET(${CMAKE_PROJECT_NAME}_${upper}_BUILD OFF)

			ELSE(${CMAKE_PROJECT_NAME}_${upper} MATCHES "SYSTEM")
			    SET(${CMAKE_PROJECT_NAME}_${upper} "BUNDLED (AUTO)" CACHE STRING "Automatically using bundled, ${lower} NOT found" FORCE)
			ENDIF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "SYSTEM")
		    ELSE(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
			# not auto, must be SYSTEM
			MESSAGE(FATAL_ERROR "System version of ${lower} NOT found, halting due to setting")
		    ENDIF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")

		ENDIF(${upper}_FOUND)

		# We have to remove any previously built output from enabled bundled copies of the
		# library in question, or the linker will get confused - a system lib was found and
		# system libraries are to be preferred with current options.  This is unfortunate in
		# that it may introduce extra build work just from trying configure options, but appears
		# to be essential to ensuring that the build "just works" each time.
		IF(${upper}_FOUND)
		    STRING(REGEX REPLACE "lib" "" rootname "${lower}")
		    FILE(GLOB STALE_FILES "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_SHARED_LIBRARY_PREFIX}${rootname}*${CMAKE_SHARED_LIBRARY_SUFFIX}*")

		    FOREACH(stale_file ${STALE_FILES})
			EXEC_PROGRAM(
			    ${CMAKE_COMMAND} ARGS -E remove ${stale_file}
			    OUTPUT_VARIABLE rm_out
			    RETURN_VALUE rm_retval
			    )
		    ENDFOREACH(stale_file ${STALE_FILES})

		    IF("${${upper}_FOUND_STATUS}" MATCHES "NOTFOUND")
			MESSAGE("Reconfiguring to use system ${upper}")
		    ENDIF("${${upper}_FOUND_STATUS}" MATCHES "NOTFOUND")
		ENDIF(${upper}_FOUND)
	    ENDIF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "BUNDLED")
	ENDIF(NOT DISABLE_TEST)

	IF(${CMAKE_PROJECT_NAME}_${upper}_BUILD)
		ADD_SUBDIRECTORY(${dir})
		SET(${upper}_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${dir};${CMAKE_CURRENT_BINARY_DIR}/${dir} CACHE STRING "set by THIRD_PARTY_SUBDIR macro" FORCE)

		IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist)
			INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist)
			DISTCHECK_IGNORE(${dir} ${dir}_ignore_files)
		ELSE(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist)
			MESSAGE("Bundled build, but file ${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist not found")
		ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist)
	ELSE(${CMAKE_PROJECT_NAME}_${upper}_BUILD)
		DISTCHECK_IGNORE_ITEM(${dir})
	ENDIF(${CMAKE_PROJECT_NAME}_${upper}_BUILD)

	MARK_AS_ADVANCED(${upper}_LIBRARY)
	MARK_AS_ADVANCED(${upper}_INCLUDE_DIR)
ENDMACRO(THIRD_PARTY)


#-----------------------------------------------------------------------------
MACRO(THIRD_PARTY_EXECUTABLE lower dir aliases description)
    STRING(TOUPPER ${lower} upper)
    BRLCAD_BUNDLE_OPTION(${CMAKE_PROJECT_NAME}_BUNDLED_LIBS ${CMAKE_PROJECT_NAME}_BUNDLED_LIBS ${CMAKE_PROJECT_NAME}_${upper} "" "${aliases}" "${description}")

    # For executables, it is a reasonable use case that the developer manually specifies
    # the location for an executable.  It is tricky to distinguish this situation from
    # a previously cached executable path resulting from a Find*.cmake script.  The way
    # we will proceed is to cache the value of ${upper}_EXECUTABLE if it is defined, and
    # at the end check it against the results of running the THIRD_PARTY logic.  If
    # it matches neither pattern (Bundled or System) it is assumed that the value passed
    # in is an intentional setting on the part of the developer.  This has one potential
    # drawback in that the *removal* of a system executable between searches could result
    # in a previously cached system search result being identified as a user-specified
    # result - to prevent that, the cached path is only used to override other results
    # if the file it specifies actually exists.  One additional wrinkle here - if the
    # developer has hard-specified BUNDLED for this particular executable, even a user specified
    # or cached value will be replaced with the local path.
    IF(${upper}_EXECUTABLE)
	IF(NOT "${${upper}_EXECUTABLE}" MATCHES ${CMAKE_BINARY_DIR})
	    get_filename_component(FULL_PATH_EXEC ${${upper}_EXECUTABLE} ABSOLUTE)
	    IF(EXISTS ${FULL_PATH_EXEC})
		SET(EXEC_CACHED ${${upper}_EXECUTABLE})
	    ELSE(EXISTS ${FULL_PATH_EXEC})
		# This path not being present may indicate the user specified a path 
		# and made a mistake doing so - warn that this might be the case.
		MESSAGE(WARNING "File path ${${upper}_EXECUTABLE} specified for ${upper}_EXECUTABLE does not exist - this path will not override ${lower} executable search results.")
	    ENDIF(EXISTS ${FULL_PATH_EXEC})
	ENDIF(NOT "${${upper}_EXECUTABLE}" MATCHES ${CMAKE_BINARY_DIR})
    ENDIF(${upper}_EXECUTABLE)
    
    # If we have a NOSYS flag, SYSTEM isn't expected to work.  Set bundled.
    FOREACH(extraarg ${ARGN})
	IF(extraarg STREQUAL "NOSYS")
	    IF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
		SET(${CMAKE_PROJECT_NAME}_${upper} "BUNDLED (AUTO)" CACHE STRING "NOSYS passed, using bundled ${lower}" FORCE)
	    ENDIF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
	ENDIF(extraarg STREQUAL "NOSYS")
    ENDFOREACH(extraarg ${ARGN})

    # Main search logic
    IF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "BUNDLED")

	# BUNDLED or BUNDLED (AUTO), figure out which

	IF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
	    SET(${CMAKE_PROJECT_NAME}_${upper} "BUNDLED (AUTO)" CACHE STRING "Automatically using bundled" FORCE)
	ELSE(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
	    SET(${CMAKE_PROJECT_NAME}_${upper} "BUNDLED" CACHE STRING "Using bundled" FORCE)
	ENDIF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")

	# Include the Find module for the exec in question - need macro routines.  But be quiet,
	# since we are going with the bundled option.  We do need the find search result to evaluate
	# the cached entry if we're BUNDLED (AUTO) instead of BUNDLED
	SET(${upper}_FIND_QUIETLY TRUE)
	SET(${upper}_FOUND_STATUS ${${upper}_FOUND})
	SET(${upper}_FOUND "${upper}-NOTFOUND")
	SET(${upper}_EXECUTABLE "${upper}-NOTFOUND")
	IF(EXISTS ${${CMAKE_PROJECT_NAME}_CMAKE_DIR}/Find${upper}.cmake)
	    INCLUDE(${${CMAKE_PROJECT_NAME}_CMAKE_DIR}/Find${upper}.cmake)
	ELSE(EXISTS ${${CMAKE_PROJECT_NAME}_CMAKE_DIR}/Find${upper}.cmake)
	    INCLUDE(${CMAKE_ROOT}/Modules/Find${upper}.cmake)
	ENDIF(EXISTS ${${CMAKE_PROJECT_NAME}_CMAKE_DIR}/Find${upper}.cmake)
	SET(${upper}_EXECUTABLE_FOUND_RESULT "${${upper}_EXECUTABLE}")
	SET(${upper}_FOUND "${${upper}_FOUND_STATUS}" CACHE STRING "${upper}_FOUND" FORCE)

	# turn it on.  If full-on BUNDLED, don't worry about the cached value - just clear it.
	IF(${CMAKE_PROJECT_NAME}_${upper} STREQUAL "BUNDLED")
	    SET(EXEC_CACHED "")
	ENDIF(${CMAKE_PROJECT_NAME}_${upper} STREQUAL "BUNDLED")
	SET(${CMAKE_PROJECT_NAME}_${upper}_BUILD ON)
	SET(${upper}_EXECUTABLE "${CMAKE_BINARY_DIR}/${BIN_DIR}/${lower}" CACHE STRING "set by THIRD_PARTY macro" FORCE)
	SET(${upper}_EXECUTABLE_TARGET "${lower}" CACHE STRING "set by THIRD_PARTY macro" FORCE)

    ELSE(${CMAKE_PROJECT_NAME}_${upper} MATCHES "BUNDLED")

	# AUTO or SYSTEM (AUTO) or SYSTEM, figure out which

	# Stash the previous results (if any) so we don't repeatedly call out the tests - only report
	# if something actually changes in subsequent runs.
	SET(${upper}_FOUND_STATUS ${${upper}_FOUND})

	# Initialize (or rather, uninitialize) variables in preparation for search
	SET(${upper}_FOUND "${upper}-NOTFOUND" CACHE STRING "${upper}_FOUND" FORCE)
	MARK_AS_ADVANCED(${upper}_FOUND)
	SET(${upper}_EXECUTABLE "${upper}-NOTFOUND" CACHE STRING "${upper}_EXECUTABLE" FORCE)

	# Be quiet if we're doing this over
	IF("${${upper}_FOUND_STATUS}" MATCHES "NOTFOUND")
	    SET(${upper}_FIND_QUIETLY TRUE)
	ENDIF("${${upper}_FOUND_STATUS}" MATCHES "NOTFOUND")

	# Include the Find module for the exec in question
	IF(EXISTS ${${CMAKE_PROJECT_NAME}_CMAKE_DIR}/Find${upper}.cmake)
	    INCLUDE(${${CMAKE_PROJECT_NAME}_CMAKE_DIR}/Find${upper}.cmake)
	ELSE(EXISTS ${${CMAKE_PROJECT_NAME}_CMAKE_DIR}/Find${upper}.cmake)
	    INCLUDE(${CMAKE_ROOT}/Modules/Find${upper}.cmake)
	ENDIF(EXISTS ${${CMAKE_PROJECT_NAME}_CMAKE_DIR}/Find${upper}.cmake)
	SET(${upper}_FOUND "${${upper}_FOUND}" CACHE STRING "${upper}_FOUND" FORCE)
	SET(${upper}_EXECUTABLE_FOUND_RESULT "${${upper}_EXECUTABLE}")

	# handle conversion of *AUTO to indicate whether it's
	# going to use system or bundled versions of deps
	IF(${upper}_FOUND)

	    # turn it off, found it
	    SET(${CMAKE_PROJECT_NAME}_${upper}_BUILD OFF)
	    SET(${upper}_EXECUTABLE_TARGET "" CACHE STRING "using system ${upper}_EXECUTABLE, no build target" FORCE)

	    # found system-installed 3rd-party dep
	    IF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
		SET(${CMAKE_PROJECT_NAME}_${upper} "SYSTEM (AUTO)" CACHE STRING "Automatically using system, ${lower} found" FORCE)
	    ELSE(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
		# not auto, must be SYSTEM
		SET(${CMAKE_PROJECT_NAME}_${upper} "SYSTEM" CACHE STRING "Using system, ${lower} found" FORCE)
	    ENDIF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")

	ELSE(${upper}_FOUND)

	    # did NOT find system-installed 3rd-party dep
	    IF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")
		# If we're following the toplevel system 
		IF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "SYSTEM")
		    SET(${CMAKE_PROJECT_NAME}_${upper} "SYSTEM (AUTO)" CACHE STRING "Automatically attempting to use system even though ${lower} was NOT found" FORCE)
		    IF(NOT EXEC_CACHED)
			MESSAGE(WARNING "Configuring to use system ${lower} even though it was NOT found - the build is not expected to succeed!")
		    ENDIF(NOT EXEC_CACHED)
		    # turn it off even though we didn't find it
		    SET(${CMAKE_PROJECT_NAME}_${upper}_BUILD OFF)
		    SET(${upper}_EXECUTABLE_TARGET "" CACHE STRING "using system ${upper}_EXECUTABLE, no build target" FORCE)

		ELSE(${CMAKE_PROJECT_NAME}_${upper} MATCHES "SYSTEM")

		    # unless it looks like we have a previously specified value, turn it on - we didn't find it
		    IF(NOT EXEC_CACHED)
			SET(${CMAKE_PROJECT_NAME}_${upper}_BUILD ON)
			SET(${CMAKE_PROJECT_NAME}_${upper} "BUNDLED (AUTO)" CACHE STRING "Automatically using bundled, ${lower} NOT found" FORCE)
			SET(${upper}_EXECUTABLE "${CMAKE_BINARY_DIR}/${BIN_DIR}/${lower}" CACHE STRING "set by THIRD_PARTY macro" FORCE)
			SET(${upper}_EXECUTABLE_TARGET "${lower}" CACHE STRING "set by THIRD_PARTY macro" FORCE)
		    ENDIF(NOT EXEC_CACHED)

		ENDIF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "SYSTEM")
	    ELSE(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")

		# not auto, must be SYSTEM
		IF(NOT EXEC_CACHED)
		    MESSAGE(WARNING "Configuring to use system ${lower} even though it was NOT found - the build is not expected to succeed!")
		ENDIF(NOT EXEC_CACHED)
		# turn it off even though we didn't find it
		SET(${CMAKE_PROJECT_NAME}_${upper}_BUILD OFF)
		SET(${upper}_EXECUTABLE_TARGET "" CACHE STRING "using system ${upper}_EXECUTABLE, no build target" FORCE)

	    ENDIF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "AUTO")

	ENDIF(${upper}_FOUND)

	IF(${upper}_FOUND)
	    IF("${${upper}_FOUND_STATUS}" MATCHES "NOTFOUND")
		IF(NOT EXEC_CACHED OR "${EXEC_CACHED}" STREQUAL "${${upper}_EXECUTABLE}")
		    MESSAGE("Reconfiguring to use system ${upper}: ${${upper}_EXECUTABLE}")
		ENDIF(NOT EXEC_CACHED OR "${EXEC_CACHED}" STREQUAL "${${upper}_EXECUTABLE}")
	    ENDIF("${${upper}_FOUND_STATUS}" MATCHES "NOTFOUND")
	ENDIF(${upper}_FOUND)
    ENDIF(${CMAKE_PROJECT_NAME}_${upper} MATCHES "BUNDLED")

    # Now that the routine is complete, see if we had a cached value different from any of our
    # standard results
    IF(EXEC_CACHED)
	# Is it a build target?  If so, don't cache it.
	GET_FILENAME_COMPONENT(EXEC_CACHED_ABS_PATH ${EXEC_CACHED} ABSOLUTE)
	IF ("${EXEC_CACHED_ABS_PATH}" STREQUAL "${PATH_ABS}")
	    # Is it the bundled path? (don't override if it is, the bundled option setting takes care of that)
	    IF(NOT "${EXEC_CACHED}" STREQUAL "${CMAKE_BINARY_DIR}/${BIN_DIR}/${lower}")
		# Is it the same as the found results?
		IF(NOT "${EXEC_CACHED}" STREQUAL "${${upper}_EXECUTABLE_FOUND_RESULT}")
		    SET(${upper}_EXECUTABLE ${EXEC_CACHED} CACHE STRING "Apparently a user specified path was supplied, use it" FORCE)
		    SET(${CMAKE_PROJECT_NAME}_${upper}_BUILD OFF)
		    SET(${CMAKE_PROJECT_NAME}_${upper} "SYSTEM (AUTO)" CACHE STRING "Apparently a user specified path was supplied, use it" FORCE)
		ENDIF(NOT "${EXEC_CACHED}" STREQUAL "${${upper}_EXECUTABLE_FOUND_RESULT}")
	    ENDIF(NOT "${EXEC_CACHED}" STREQUAL "${CMAKE_BINARY_DIR}/${BIN_DIR}/${lower}")
	ENDIF("${EXEC_CACHED_ABS_PATH}" STREQUAL "${PATH_ABS}")
    ENDIF(EXEC_CACHED)

    IF(${CMAKE_PROJECT_NAME}_${upper}_BUILD)
	# In the case of executables, it is possible that a directory may define more than one
	# executable target.  If this is the case, we may have already added this directory - 
	# if so, don't do it again.
	IF(SRC_OTHER_ADDED_DIRS)
	    LIST(FIND SRC_OTHER_ADDED_DIRS ${dir} ADDED_RESULT)
	    IF("${ADDED_RESULT}" STREQUAL "-1")
		ADD_SUBDIRECTORY(${dir})
		SET(SRC_OTHER_ADDED_DIRS ${SRC_OTHER_ADDED_DIRS} ${dir})
		LIST(REMOVE_DUPLICATES SRC_OTHER_ADDED_DIRS)
		SET(SRC_OTHER_ADDED_DIRS ${SRC_OTHER_ADDED_DIRS} CACHE STRING "Enabled 3rd party sub-directories" FORCE)
		MARK_AS_ADVANCED(SRC_OTHER_ADDED_DIRS)
		IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist)
		    INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist)
		    DISTCHECK_IGNORE(${dir} ${dir}_ignore_files)
		ELSE(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist)
		    MESSAGE("Bundled build, but file ${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist not found")
		ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist)
	    ENDIF("${ADDED_RESULT}" STREQUAL "-1")
	ELSE(SRC_OTHER_ADDED_DIRS)
	    ADD_SUBDIRECTORY(${dir})
	    SET(SRC_OTHER_ADDED_DIRS ${dir} CACHE STRING "Enabled 3rd party sub-directories" FORCE)
	    MARK_AS_ADVANCED(SRC_OTHER_ADDED_DIRS)
	    IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist)
		INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist)
		DISTCHECK_IGNORE(${dir} ${dir}_ignore_files)
	    ELSE(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist)
		MESSAGE("Bundled build, but file ${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist not found")
	    ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}.dist)
	ENDIF(SRC_OTHER_ADDED_DIRS)
    ELSE(${CMAKE_PROJECT_NAME}_${upper}_BUILD)
	DISTCHECK_IGNORE_ITEM(${dir})
    ENDIF(${CMAKE_PROJECT_NAME}_${upper}_BUILD)

    MARK_AS_ADVANCED(${upper}_EXECUTABLE)
    MARK_AS_ADVANCED(${upper}_EXECUTABLE_TARGET)
    MARK_AS_ADVANCED(${upper}_FOUND)
ENDMACRO(THIRD_PARTY_EXECUTABLE)

# Local Variables:
# tab-width: 8
# mode: sh
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
