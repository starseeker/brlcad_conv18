#               B R L C A D _ U T I L . C M A K E
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
# Pretty-printing macro that generates a box around a string and prints the
# resulting message.
MACRO(BOX_PRINT input_string border_string)
	STRING(LENGTH ${input_string} MESSAGE_LENGTH)
	STRING(LENGTH ${border_string} SEPARATOR_STRING_LENGTH)
	WHILE(${MESSAGE_LENGTH} GREATER ${SEPARATOR_STRING_LENGTH})
		SET(SEPARATOR_STRING "${SEPARATOR_STRING}${border_string}")
		STRING(LENGTH ${SEPARATOR_STRING} SEPARATOR_STRING_LENGTH)
	ENDWHILE(${MESSAGE_LENGTH} GREATER ${SEPARATOR_STRING_LENGTH})
	MESSAGE("${SEPARATOR_STRING}")
	MESSAGE("${input_string}")
	MESSAGE("${SEPARATOR_STRING}")
ENDMACRO()


#-----------------------------------------------------------------------------
# Macro for three-way options that optionally check whether a system
# resource is available.
MACRO(BUNDLE_OPTION trigger build_test_var optname default_raw)
	STRING(TOUPPER "${default_raw}" default)
	IF(NOT ${optname})
		IF(default)
			SET(${optname} "${default}" CACHE STRING "Using bundled ${optname}")
		ELSE(default)
			IF(${${trigger}} MATCHES "BUNDLED")
				SET(${optname} "BUNDLED (AUTO)" CACHE STRING "Using bundled ${optname}")
			ENDIF(${${trigger}} MATCHES "BUNDLED")

			IF(${${trigger}} MATCHES "SYSTEM")
				SET(${optname} "SYSTEM (AUTO)" CACHE STRING "Using system ${optname}")
			ENDIF(${${trigger}} MATCHES "SYSTEM")

			IF(${${trigger}} MATCHES "AUTO")
				SET(${optname} "AUTO" CACHE STRING "Using bundled ${optname}")
			ENDIF(${${trigger}} MATCHES "AUTO")
		ENDIF(default)
	ENDIF(NOT ${optname})

	# convert ON/OFF value to BUNDLED/SYSTEM
	STRING(TOUPPER "${${optname}}" optname_upper)
	IF(${optname_upper} MATCHES "ON")
		SET(optname_upper "BUNDLED")
	ENDIF(${optname_upper} MATCHES "ON")
	IF(${optname_upper} MATCHES "OFF")
		SET(optname_upper "SYSTEM")
	ENDIF(${optname_upper} MATCHES "OFF")
	SET(${optname} ${optname_upper})

	#convert AUTO value to indicate whether we're BUNDLED/SYSTEM
	IF(${optname} MATCHES "AUTO")
		IF(${${build_test_var}} MATCHES "BUNDLED")
			SET(${optname} "BUNDLED (AUTO)" CACHE STRING "Using bundled ${optname}" FORCE)
		ENDIF(${${build_test_var}} MATCHES "BUNDLED")

		IF(${${build_test_var}} MATCHES "SYSTEM")
			SET(${optname} "SYSTEM (AUTO)" CACHE STRING "Using system ${optname}" FORCE)
		ENDIF(${${build_test_var}} MATCHES "SYSTEM")

		IF(${${build_test_var}} MATCHES "AUTO")
			SET(${optname} "AUTO" CACHE STRING "Using bundled ${optname}" FORCE)
		ENDIF(${${build_test_var}} MATCHES "AUTO")

		IF(${${build_test_var}} MATCHES "ON")
			SET(${optname} "BUNDLED (AUTO)" CACHE STRING "Using bundled ${optname}" FORCE)
		ENDIF(${${build_test_var}} MATCHES "ON")

		IF(${${build_test_var}} MATCHES "OFF")
			SET(${optname} "SYSTEM (AUTO)" CACHE STRING "Using system ${optname}" FORCE)
		ENDIF(${${build_test_var}} MATCHES "OFF")
	ENDIF(${optname} MATCHES "AUTO")

	set_property(CACHE ${optname} PROPERTY STRINGS AUTO BUNDLED SYSTEM)

	# make sure we have a valid value
	IF(NOT ${${optname}} MATCHES "AUTO" AND NOT ${${optname}} MATCHES "BUNDLED" AND NOT ${${optname}} MATCHES "SYSTEM")
		MESSAGE(WARNING "Unknown value ${${optname}} supplied for ${optname} - defaulting to AUTO\nValid options are AUTO, BUNDLED and SYSTEM")
		SET(${optname} "AUTO" CACHE STRING "Using bundled ${optname}" FORCE)
	ENDIF(NOT ${${optname}} MATCHES "AUTO" AND NOT ${${optname}} MATCHES "BUNDLED" AND NOT ${${optname}} MATCHES "SYSTEM")
ENDMACRO()


#-----------------------------------------------------------------------------
# Build Type aware option
MACRO(AUTO_OPTION username varname debug_state release_state)
	STRING(LENGTH "${${username}}" ${username}_SET)

	IF(NOT ${username}_SET)
		SET(${username} "AUTO" CACHE STRING "auto option" FORCE)
	ENDIF(NOT ${username}_SET)

	set_property(CACHE ${username} PROPERTY STRINGS AUTO "ON" "OFF")

	STRING(TOUPPER "${${username}}" username_upper)
	SET(${username} ${username_upper})

	IF(NOT ${${username}} MATCHES "AUTO" AND NOT ${${username}} MATCHES "ON" AND NOT ${${username}} MATCHES "OFF")
		MESSAGE(WARNING "Unknown value ${${username}} supplied for ${username} - defaulting to AUTO.\nValid options are AUTO, ON and OFF")
		SET(${username} "AUTO" CACHE STRING "auto option" FORCE)
	ENDIF(NOT ${${username}} MATCHES "AUTO" AND NOT ${${username}} MATCHES "ON" AND NOT ${${username}} MATCHES "OFF")

	# If the "parent" setting isn't AUTO, do what it says
	IF(NOT ${${username}} MATCHES "AUTO")
		SET(${varname} ${${username}})
	ENDIF(NOT ${${username}} MATCHES "AUTO")

	# If we we don't understand the build type and have an AUTO setting for the
	# optimization flags, leave them off
	IF(NOT "${CMAKE_BUILD_TYPE}" MATCHES "Release" AND NOT "${CMAKE_BUILD_TYPE}" MATCHES "Debug")
		IF(NOT ${${username}} MATCHES "AUTO")
			SET(${varname} ${${username}})
		ELSE(NOT ${${username}} MATCHES "AUTO")
			SET(${varname} OFF)
			SET(${username} "OFF (AUTO)" CACHE STRING "auto option" FORCE)
		ENDIF(NOT ${${username}} MATCHES "AUTO")
	ENDIF(NOT "${CMAKE_BUILD_TYPE}" MATCHES "Release" AND NOT "${CMAKE_BUILD_TYPE}" MATCHES "Debug")

	# If we DO understand the build type and have AUTO, be smart
	IF("${CMAKE_BUILD_TYPE}" MATCHES "Release" AND ${${username}} MATCHES "AUTO")
		SET(${varname} ${release_state})
		SET(${username} "${release_state} (AUTO)" CACHE STRING "auto option" FORCE)
	ENDIF("${CMAKE_BUILD_TYPE}" MATCHES "Release" AND ${${username}} MATCHES "AUTO")

	IF("${CMAKE_BUILD_TYPE}" MATCHES "Debug" AND ${${username}} MATCHES "AUTO")
		SET(${varname} ${debug_state})
		SET(${username} "${debug_state} (AUTO)" CACHE STRING "auto option" FORCE)
	ENDIF("${CMAKE_BUILD_TYPE}" MATCHES "Debug" AND ${${username}} MATCHES "AUTO")
ENDMACRO(AUTO_OPTION varname release_state debug_state)


#-----------------------------------------------------------------------------
# For "top-level" BRL-CAD options, some extra work is in order - descriptions and
# lists of aliases are supplied, and those are automatically addressed by this
# macro.  In this context, "aliases" are variables which may be defined on the
# CMake command line that are intended to set the value of th BRL-CAD option in
# question, but use some other name.  Aliases may be common typos, different
# nomenclature, or anything that the developer things should lead to a given
# option being set.  The documentation is auto-formatted into a text document
# describing all BRL-CAD options.
MACRO(BRLCAD_OPTION default opt opt_ALIASES opt_DESCRIPTION)
    IF(NOT DEFINED ${opt})
	SET(${opt} ${default} CACHE STRING "define ${opt}" FORCE)
    ENDIF(NOT DEFINED ${opt})
    IF("${${opt}}" STREQUAL "")
	SET(${opt} ${default} CACHE STRING "define ${opt}" FORCE)
    ENDIF("${${opt}}" STREQUAL "")
    IF(${opt_ALIASES})
	FOREACH(item ${${opt_ALIASES}})
	    STRING(REPLACE "ENABLE_" "DISABLE_" inverse_item ${item})
	    SET(inverse_aliases ${inverse_aliases} ${inverse_item})
	ENDFOREACH(item ${${opt_ALIASES}})
	FOREACH(item ${${opt_ALIASES}})
	    IF(NOT "${item}" STREQUAL "${opt}")
		IF(NOT "${${item}}" STREQUAL "")
		    SET(${opt} ${${item}} CACHE STRING "setting ${opt} via alias ${item}" FORCE)
		    SET(${item} "" CACHE STRING "set ${opt} via this variable" FORCE)
		    MARK_AS_ADVANCED(${item})
		ENDIF(NOT "${${item}}" STREQUAL "")
	    ENDIF(NOT "${item}" STREQUAL "${opt}")
	ENDFOREACH(item ${${opt_ALIASES}})
	# handle DISABLE forms of options
	FOREACH(item ${inverse_aliases})
	    IF(NOT "${item}" STREQUAL "${opt}")
		IF(NOT "${${item}}" STREQUAL "")
		    SET(invert_item ${${item}})
		    IF("${${item}}" STREQUAL "ON")
			SET(invert_item "OFF")
		    ELSEIF("${invert_item}" STREQUAL "OFF")
			SET(invert_item "ON")
		    ENDIF("${${item}}" STREQUAL "ON")
		    SET(${opt} ${invert_item} CACHE STRING "setting ${opt} via alias ${item}" FORCE)
		    SET(${item} "" CACHE STRING "set ${opt} via this variable" FORCE)
		    MARK_AS_ADVANCED(${item})
		ENDIF(NOT "${${item}}" STREQUAL "")
	    ENDIF(NOT "${item}" STREQUAL "${opt}")
	ENDFOREACH(item ${inverse_aliases})
    ENDIF(${opt_ALIASES})

    FILE(APPEND ${CMAKE_BINARY_DIR}/OPTIONS "\n--- ${opt} ---\n")
    FILE(APPEND ${CMAKE_BINARY_DIR}/OPTIONS "${${opt_DESCRIPTION}}")

    SET(ALIASES_LIST "\nAliases: ")
    FOREACH(item ${${opt_ALIASES}})
	SET(ALIASES_LIST_TEST "${ALIASES_LIST}, ${item}")
	STRING(LENGTH ${ALIASES_LIST_TEST} LL)

	IF(${LL} GREATER 80)
	    FILE(APPEND ${CMAKE_BINARY_DIR}/OPTIONS "${ALIASES_LIST}\n")
	    SET(ALIASES_LIST "          ${item}")
	ELSE(${LL} GREATER 80)
	    IF(NOT ${ALIASES_LIST} MATCHES "\nAliases")
		SET(ALIASES_LIST "${ALIASES_LIST}, ${item}")
	    ELSE(NOT ${ALIASES_LIST} MATCHES "\nAliases")
		IF(NOT ${ALIASES_LIST} STREQUAL "\nAliases: ")
		    SET(ALIASES_LIST "${ALIASES_LIST}, ${item}")
		ELSE(NOT ${ALIASES_LIST} STREQUAL "\nAliases: ")
		    SET(ALIASES_LIST "${ALIASES_LIST} ${item}")
		ENDIF(NOT ${ALIASES_LIST} STREQUAL "\nAliases: ")
	    ENDIF(NOT ${ALIASES_LIST} MATCHES "\nAliases")
	ENDIF(${LL} GREATER 80)
    ENDFOREACH(item ${${opt_ALIASES}})

    IF(ALIASES_LIST)	
	STRING(STRIP ALIASES_LIST ${ALIASES_LIST})
	IF(ALIASES_LIST)	
	    FILE(APPEND ${CMAKE_BINARY_DIR}/OPTIONS "${ALIASES_LIST}")
	ENDIF(ALIASES_LIST)	
    ENDIF(ALIASES_LIST)	

    FILE(APPEND ${CMAKE_BINARY_DIR}/OPTIONS "\n\n")
ENDMACRO(BRLCAD_OPTION)

INCLUDE(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG(-Wno-error NOERROR_FLAG)


#-----------------------------------------------------------------------------
MACRO(CPP_WARNINGS srcslist)
	# We need to specify specific flags for c++ files - their warnings are
	# not usually used to hault the build, althogh BRLCAD_ENABLE_CXX_STRICT
	# can be set to on to achieve that
	IF(BRLCAD_ENABLE_STRICT AND NOT BRLCAD_ENABLE_CXX_STRICT)
		FOREACH(srcfile ${${srcslist}})
			IF(${srcfile} MATCHES "cpp$" OR ${srcfile} MATCHES "cc$")
				IF(BRLCAD_ENABLE_COMPILER_WARNINGS)
					IF(NOERROR_FLAG)
						GET_PROPERTY(previous_flags SOURCE ${srcfile} PROPERTY COMPILE_FLAGS)
						set_source_files_properties(${srcfile} COMPILE_FLAGS "-Wno-error ${previous_flags}")
					ENDIF(NOERROR_FLAG)
				ELSE(BRLCAD_ENABLE_COMPILER_WARNINGS)
					GET_PROPERTY(previous_flags SOURCE ${srcfile} PROPERTY COMPILE_FLAGS)
					set_source_files_properties(${srcfile} COMPILE_FLAGS "-w ${previous_flags}")
				ENDIF(BRLCAD_ENABLE_COMPILER_WARNINGS)
			ENDIF(${srcfile} MATCHES "cpp$" OR ${srcfile} MATCHES "cc$")
		ENDFOREACH(srcfile ${${srcslist}})
	ENDIF(BRLCAD_ENABLE_STRICT AND NOT BRLCAD_ENABLE_CXX_STRICT)
ENDMACRO(CPP_WARNINGS)

#-----------------------------------------------------------------------------
# Convenience macros for handling lists of defines at the directory and target
# levels - the latter is not needed for BRLCAD_* targets under normal 
# circumstances and is intended for cases where basic non-installed 
# add_executable calls are made.
MACRO(BRLCAD_ADD_DEF_LISTS)
	FOREACH(deflist ${ARGN})
		SET(target_def_list ${target_def_list} ${${deflist}})
	ENDFOREACH(deflist ${ARGN})
	IF(target_def_list)
		LIST(REMOVE_DUPLICATES target_def_list)
	ENDIF(target_def_list)
	FOREACH(defitem ${${deflist}})
		add_definitions(${defitem})
	ENDFOREACH(defitem ${${deflist}})
ENDMACRO(BRLCAD_ADD_DEF_LISTS)

MACRO(BRLCAD_TARGET_ADD_DEFS target)
	GET_TARGET_PROPERTY(TARGET_EXISTING_DEFS ${target} COMPILE_DEFINITIONS)
	FOREACH(defitem ${ARGN})
		SET(DEF_EXISTS "-1")
		IF(TARGET_EXISTING_DEFS)
			LIST(FIND TARGET_EXISTING_DEFS ${defitem} DEF_EXISTS)
		ENDIF(TARGET_EXISTING_DEFS)
		IF("${DEF_EXISTS}" STREQUAL "-1")
			SET_PROPERTY(TARGET ${target} APPEND PROPERTY COMPILE_DEFINITIONS "${defitem}")
		ENDIF("${DEF_EXISTS}" STREQUAL "-1")
	ENDFOREACH(defitem ${ARGN})
ENDMACRO(BRLCAD_TARGET_ADD_DEFS)

MACRO(BRLCAD_TARGET_ADD_DEF_LISTS target)
	GET_TARGET_PROPERTY(TARGET_EXISTING_DEFS ${target} COMPILE_DEFINITIONS)
	FOREACH(deflist ${ARGN})
		SET(target_def_list ${target_def_list} ${${deflist}})
	ENDFOREACH(deflist ${ARGN})
	IF(target_def_list)
		LIST(REMOVE_DUPLICATES target_def_list)
	ENDIF(target_def_list)
	FOREACH(defitem ${target_def_list})
		SET(DEF_EXISTS "-1")
		IF(TARGET_EXISTING_DEFS)
			LIST(FIND TARGET_EXISTING_DEFS ${defitem} DEF_EXISTS)
		ENDIF(TARGET_EXISTING_DEFS)
		IF("${DEF_EXISTS}" STREQUAL "-1")
			SET_PROPERTY(TARGET ${target} APPEND PROPERTY COMPILE_DEFINITIONS "${defitem}")
		ENDIF("${DEF_EXISTS}" STREQUAL "-1")
	ENDFOREACH(defitem ${${deflist}})
ENDMACRO(BRLCAD_TARGET_ADD_DEF_LISTS)

#-----------------------------------------------------------------------------
# Core routines for adding executables and libraries to the build and
# install lists of CMake
MACRO(BRLCAD_ADDEXEC execname srcs libs)
	STRING(REGEX REPLACE " " ";" srcslist "${srcs}")
	STRING(REGEX REPLACE " " ";" libslist1 "${libs}")
	STRING(REGEX REPLACE "-framework;" "-framework " libslist "${libslist1}")

	add_executable(${execname} ${srcslist})
	target_link_libraries(${execname} ${libslist})

	install(TARGETS ${execname} DESTINATION ${BIN_DIR})

	FOREACH(libitem ${libslist})
		LIST(FIND BRLCAD_LIBS ${libitem} FOUNDIT)
		IF(NOT ${FOUNDIT} STREQUAL "-1")
			STRING(REGEX REPLACE "lib" "" ITEMCORE "${libitem}")
			STRING(TOUPPER ${ITEMCORE} ITEM_UPPER_CORE)
			LIST(APPEND ${execname}_DEFINES ${${ITEM_UPPER_CORE}_DEFINES})
			IF(${execname}_DEFINES)
				LIST(REMOVE_DUPLICATES ${execname}_DEFINES)
			ENDIF(${execname}_DEFINES)
		ENDIF(NOT ${FOUNDIT} STREQUAL "-1")
	ENDFOREACH(libitem ${libslist})

	FOREACH(lib_define ${${execname}_DEFINES})
		SET_PROPERTY(TARGET ${execname} APPEND PROPERTY COMPILE_DEFINITIONS "${lib_define}")
	ENDFOREACH(lib_define ${${execname}_DEFINES})

	FOREACH(extraarg ${ARGN})
		IF(${extraarg} MATCHES "NOSTRICT" AND BRLCAD_ENABLE_STRICT)
			IF(NOERROR_FLAG)
				SET_TARGET_PROPERTIES(${execname} PROPERTIES COMPILE_FLAGS "-Wno-error")
			ENDIF(NOERROR_FLAG)
		ENDIF(${extraarg} MATCHES "NOSTRICT" AND BRLCAD_ENABLE_STRICT)
	ENDFOREACH(extraarg ${ARGN})

	CPP_WARNINGS(srcslist)
ENDMACRO(BRLCAD_ADDEXEC execname srcs libs)


#-----------------------------------------------------------------------------
# Library macro handles both shared and static libs, so one "BRLCAD_ADDLIB"
# statement will cover both automatically
MACRO(BRLCAD_ADDLIB libname srcs libs)

	# Add ${libname} to the list of BRL-CAD libraries	
	LIST(APPEND BRLCAD_LIBS ${libname})
	LIST(REMOVE_DUPLICATES BRLCAD_LIBS)
	SET(BRLCAD_LIBS "${BRLCAD_LIBS}" CACHE STRING "BRL-CAD libraries" FORCE)

	# Define convenience variables and scrub library list	
	STRING(REGEX REPLACE "lib" "" LOWERCORE "${libname}")
	STRING(TOUPPER ${LOWERCORE} UPPER_CORE)
	STRING(REGEX REPLACE " " ";" srcslist "${srcs}")
	STRING(REGEX REPLACE " " ";" libslist1 "${libs}")
	STRING(REGEX REPLACE "-framework;" "-framework " libslist "${libslist1}")

	# Collect the definitions needed by this library
	# Appending to the list to ensure that any non-template
	# definitions made in the CMakeLists.txt file are preserved.
	# In case of re-running cmake, make sure the DLL_IMPORTS define
	# for this specific library is removed, since for the actual target 
	# we need to export, not import.
	IF(${UPPER_CORE}_DEFINES)
		LIST(REMOVE_ITEM ${UPPER_CORE}_DEFINES ${UPPER_CORE}_DLL_IMPORTS)
	ENDIF(${UPPER_CORE}_DEFINES)
	FOREACH(libitem ${libslist})
	    LIST(FIND BRLCAD_LIBS ${libitem} FOUNDIT)
	    IF(NOT ${FOUNDIT} STREQUAL "-1")
		STRING(REGEX REPLACE "lib" "" ITEMCORE "${libitem}")
		STRING(TOUPPER ${ITEMCORE} ITEM_UPPER_CORE)
		LIST(APPEND ${UPPER_CORE}_DEFINES ${${ITEM_UPPER_CORE}_DEFINES})
	    ENDIF(NOT ${FOUNDIT} STREQUAL "-1")
	ENDFOREACH(libitem ${libslist})
	IF(${UPPER_CORE}_DEFINES)
	    LIST(REMOVE_DUPLICATES ${UPPER_CORE}_DEFINES)
	ENDIF(${UPPER_CORE}_DEFINES)
	MARK_AS_ADVANCED(${UPPER_CORE}_DEFINES)

	# Handle "shared" libraries (with MSVC, these would be dynamic libraries)
	IF(BUILD_SHARED_LIBS)
		add_library(${libname} SHARED ${srcslist})

		IF(CPP_DLL_DEFINES)
			SET_PROPERTY(TARGET ${libname} APPEND PROPERTY COMPILE_DEFINITIONS "${UPPER_CORE}_DLL_EXPORTS")
		ENDIF(CPP_DLL_DEFINES)

		IF(NOT ${libs} MATCHES "NONE")
			target_link_libraries(${libname} ${libslist})
		ENDIF(NOT ${libs} MATCHES "NONE")

		INSTALL(TARGETS ${libname} DESTINATION ${LIB_DIR})

		FOREACH(lib_define ${${UPPER_CORE}_DEFINES})
			SET_PROPERTY(TARGET ${libname} APPEND PROPERTY COMPILE_DEFINITIONS "${lib_define}")
		ENDFOREACH(lib_define ${${UPPER_CORE}_DEFINES})

		FOREACH(extraarg ${ARGN})
			IF(${extraarg} MATCHES "NOSTRICT" AND BRLCAD_ENABLE_STRICT)
				IF(NOERROR_FLAG)
					SET_PROPERTY(TARGET ${libname} APPEND PROPERTY COMPILE_FLAGS "-Wno-error")
				ENDIF(NOERROR_FLAG)
			ENDIF(${extraarg} MATCHES "NOSTRICT" AND BRLCAD_ENABLE_STRICT)
		ENDFOREACH(extraarg ${ARGN})
	ENDIF(BUILD_SHARED_LIBS)

	# For any BRL-CAD libraries using this library, define a variable 
	# with the needed definitions.
	IF(CPP_DLL_DEFINES)
		LIST(APPEND ${UPPER_CORE}_DEFINES ${UPPER_CORE}_DLL_IMPORTS)
	ENDIF(CPP_DLL_DEFINES)
	SET(${UPPER_CORE}_DEFINES "${${UPPER_CORE}_DEFINES}" CACHE STRING "${UPPER_CORE} library required definitions" FORCE)

	# Handle static libraries (renaming requirements to both allow unique targets and
	# respect standard naming conventions.)
	IF(BUILD_STATIC_LIBS)
		add_library(${libname}-static STATIC ${srcslist})

		IF(NOT MSVC)
			IF(NOT ${libs} MATCHES "NONE")
				target_link_libraries(${libname}-static ${libslist})
			ENDIF(NOT ${libs} MATCHES "NONE")
			SET_TARGET_PROPERTIES(${libname}-static PROPERTIES OUTPUT_NAME "${libname}")
		ENDIF(NOT MSVC)

		INSTALL(TARGETS ${libname}-static DESTINATION ${LIB_DIR})

		FOREACH(extraarg ${ARGN})
			IF(${extraarg} MATCHES "NOSTRICT" AND BRLCAD_ENABLE_STRICT)
				IF(NOERROR_FLAG)
					SET_PROPERTY(TARGET ${libname}-static APPEND PROPERTY COMPILE_FLAGS "-Wno-error")
				ENDIF(NOERROR_FLAG)
			ENDIF(${extraarg} MATCHES "NOSTRICT" AND BRLCAD_ENABLE_STRICT)
		ENDFOREACH(extraarg ${ARGN})
	ENDIF(BUILD_STATIC_LIBS)

	MARK_AS_ADVANCED(BRLCAD_LIBS)
	CPP_WARNINGS(srcslist)
ENDMACRO(BRLCAD_ADDLIB libname srcs libs)

#-----------------------------------------------------------------------------
# For situations when a local 3rd party library (say, zlib) has been chosen in 
# preference to a system version of that library, it is important to ensure 
# that the local header(s) get included before the system headers.  Normally 
# this is handled by explicitly specifying the local include paths (which, 
# being explicitly specified and non-standard, are checked prior to default 
# system locations) but there are some situations (macports being a classic 
# example) where *other* "non-standard" installed copies of libraries may 
# exist and be found if those directories are included ahead of the desired 
# local copy.  An observed case:
#
# 1.  macports is installed on OSX
# 2.  X11 is found in macports, X11 directories are set to /usr/macports based paths
# 3.  These paths are mixed into the general include path lists for some BRL-CAD libs.
# 4.  Because these paths are a) non-standard and b) contain zlib.h they result
#     in "system" versions of zlib present in macports being found first even when
#     the local zlib is enabled, if the macports paths happen to appear in the
#     include directory list before the local zlib include paths.
#
# To mitigate this problem, BRL-CAD library include directories are sorted
# according to the following hierarchy (using gcc's left-to-right search
# order as a basis: http://gcc.gnu.org/onlinedocs/cpp/Search-Path.html):
#
# 1.  If CMAKE_CURRENT_BINARY_DIR or CMAKE_CURRENT_SOURCE_DIR are in the
#     include list, they come first.
# 2.  If BRLCAD_BINARY_DIR/include or BRLCAD_SOURCE_DIR/include are present,
#     they come second.
# 3.  For remaining paths, if the "root" path matches the BRLCAD_SOURCE_DIR
#     or BRLCAD_BINARY_DIR paths, they are appended.
# 4.  Any remaining paths are appended.
MACRO(BRLCAD_SORT_INCLUDE_DIRS DIR_LIST)
    IF(${DIR_LIST})
	SET(ORDERED_ELEMENTS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${BRLCAD_BINARY_DIR}/include ${BRLCAD_SOURCE_DIR}/include)
	SET(NEW_DIR_LIST "")
	LIST(REMOVE_DUPLICATES ${DIR_LIST})
	FOREACH(element ${ORDERED_ELEMENTS})
	    SET(DEF_EXISTS "-1")
	    LIST(FIND ${DIR_LIST} ${element} DEF_EXISTS)
	    IF(NOT "${DEF_EXISTS}" STREQUAL "-1")
		SET(NEW_DIR_LIST ${NEW_DIR_LIST} ${element})
		LIST(REMOVE_ITEM ${DIR_LIST} ${element})
	    ENDIF(NOT "${DEF_EXISTS}" STREQUAL "-1")
	ENDFOREACH(element ${ORDERED_ELEMENTS})

	# paths in BRL-CAD build dir
	FOREACH(inc_path ${${DIR_LIST}})
	    IF(${inc_path} MATCHES "^${BRLCAD_BINARY_DIR}")
		SET(NEW_DIR_LIST ${NEW_DIR_LIST} ${inc_path})
		LIST(REMOVE_ITEM ${DIR_LIST} ${inc_path})
	    ENDIF(${inc_path} MATCHES "^${BRLCAD_BINARY_DIR}")
	ENDFOREACH(inc_path ${${DIR_LIST}})

	# paths in BRL-CAD source dir
	FOREACH(inc_path ${${DIR_LIST}})
	    IF(${inc_path} MATCHES "^${BRLCAD_SOURCE_DIR}")
		SET(NEW_DIR_LIST ${NEW_DIR_LIST} ${inc_path})
		LIST(REMOVE_ITEM ${DIR_LIST} ${inc_path})
	    ENDIF(${inc_path} MATCHES "^${BRLCAD_SOURCE_DIR}")
	ENDFOREACH(inc_path ${${DIR_LIST}})

	# add anything that might be left
	SET(NEW_DIR_LIST ${NEW_DIR_LIST} ${${DIR_LIST}})

	# put the results into DIR_LIST
	SET(${DIR_LIST} ${NEW_DIR_LIST})
    ENDIF(${DIR_LIST})
ENDMACRO(BRLCAD_SORT_INCLUDE_DIRS)


#-----------------------------------------------------------------------------
# Wrapper to handle include directories specific to libraries.  Removes
# duplicates and makes sure the <LIB>_INCLUDE_DIRS is in the cache
# immediately, so it can be used by other libraries.  These are not
# intended as toplevel user settable options so mark as advanced.
MACRO(BRLCAD_INCLUDE_DIRS DIR_LIST)
	STRING(REGEX REPLACE "_INCLUDE_DIRS" "" LIB_UPPER "${DIR_LIST}")
	STRING(TOLOWER ${LIB_UPPER} LIB_LOWER)

	LIST(REMOVE_DUPLICATES ${DIR_LIST})
	SET(${DIR_LIST} ${${DIR_LIST}} CACHE STRING "Include directories for lib${LIB_LOWER}" FORCE)
	MARK_AS_ADVANCED(${DIR_LIST})

	SET(UPPER_INCLUDES ${${DIR_LIST}} ${${LIB_UPPER}_LOCAL_INCLUDE_DIRS})
        BRLCAD_SORT_INCLUDE_DIRS(UPPER_INCLUDES)	
	include_directories(${UPPER_INCLUDES})
ENDMACRO(BRLCAD_INCLUDE_DIRS)


#-----------------------------------------------------------------------------
# We attempt here to strike a balance between competing needs.  Ideally, any error messages
# returned as a consequence of using data while running programs should point the developer
# back to the version controlled source code, not a copy in the build directory.  However,
# another design goal is to replicate the install tree layout in the build directory.  On
# platforms with symbolic links, we can do both by linking the source data files to their
# locations in the build directory.  On Windows, this is not possible and we have to fall
# back on an explicit file copy mechanism.  In both cases we have a build target that is
# triggered if source files are edited in order to allow the build system to take any further
# actions that may be needed (the current example is re-generating tclIndex and pkgIndex.tcl
# files when the source .tcl files change).

# In principle it may be possible to go even further and add rules to copy files in the build
# dir that are different from their source originals back into the source tree... need to
# think about complexity/robustness tradeoffs
MACRO(BRLCAD_ADDDATA datalist targetdir)
	IF(NOT WIN32)
		IF(NOT EXISTS "${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}")
			EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir})
		ENDIF(NOT EXISTS "${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}")

		FOREACH(filename ${${datalist}})
			GET_FILENAME_COMPONENT(ITEM_NAME ${filename} NAME)
			EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/${filename} ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${ITEM_NAME})
		ENDFOREACH(filename ${${datalist}})

		STRING(REGEX REPLACE "/" "_" targetprefix ${targetdir})

		ADD_CUSTOM_COMMAND(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${targetprefix}.sentinel
			COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/${targetprefix}.sentinel
			DEPENDS ${${datalist}}
			)
		ADD_CUSTOM_TARGET(${datalist}_cp ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${targetprefix}.sentinel)
	ELSE(NOT WIN32)
		FILE(COPY ${${datalist}} DESTINATION ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir})
		STRING(REGEX REPLACE "/" "_" targetprefix ${targetdir})
		SET(inputlist)

		FOREACH(filename ${${datalist}})
			SET(inputlist ${inputlist} ${CMAKE_CURRENT_SOURCE_DIR}/${filename})
		ENDFOREACH(filename ${${datalist}})

		SET(${targetprefix}_cmake_contents "
		SET(FILES_TO_COPY \"${inputlist}\")
		FILE(COPY \${FILES_TO_COPY} DESTINATION \"${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}\")
		")
		FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${targetprefix}.cmake "${${targetprefix}_cmake_contents}")
		ADD_CUSTOM_COMMAND(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${targetprefix}.sentinel
			COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/${targetprefix}.cmake
			COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/${targetprefix}.sentinel
			DEPENDS ${${datalist}}
			)
		ADD_CUSTOM_TARGET(${datalist}_cp ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${targetprefix}.sentinel)
	ENDIF(NOT WIN32)

	FOREACH(filename ${${datalist}})
		INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${filename} DESTINATION ${DATA_DIR}/${targetdir})
	ENDFOREACH(filename ${${datalist}})
	CMAKEFILES(${${datalist}})
ENDMACRO(BRLCAD_ADDDATA datalist targetdir)


#-----------------------------------------------------------------------------
MACRO(BRLCAD_ADDFILE filename targetdir)
	FILE(COPY ${filename} DESTINATION ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir})
	STRING(REGEX REPLACE "/" "_" targetprefix ${targetdir})

	IF(BRLCAD_ENABLE_DATA_TARGETS)
		STRING(REGEX REPLACE "/" "_" filestring ${filename})
		ADD_CUSTOM_COMMAND(
			OUTPUT ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${filename}
			COMMAND ${CMAKE_COMMAND} -E copy	${CMAKE_CURRENT_SOURCE_DIR}/${filename} ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${filename}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${filename}
			)
		ADD_CUSTOM_TARGET(${targetprefix}_${filestring}_cp ALL DEPENDS ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${filename})
	ENDIF(BRLCAD_ENABLE_DATA_TARGETS)

	INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${filename} DESTINATION ${DATA_DIR}/${targetdir})

	FILE(APPEND ${CMAKE_BINARY_DIR}/cmakefiles.cmake "${CMAKE_CURRENT_SOURCE_DIR}/${filename}\n")
ENDMACRO(BRLCAD_ADDFILE datalist targetdir)


#-----------------------------------------------------------------------------
MACRO(ADD_MAN_PAGES num manlist)
	CMAKEFILES(${${manlist}})
	FOREACH(manpage ${${manlist}})
		install(FILES ${manpage} DESTINATION ${MAN_DIR}/man${num})
	ENDFOREACH(manpage ${${manlist}})
ENDMACRO(ADD_MAN_PAGES num manlist)


#-----------------------------------------------------------------------------
MACRO(ADD_MAN_PAGE num manfile)
	CMAKEFILES(${manfile})
	FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/cmakefiles.cmake "${manfile}\n")
	install(FILES ${manfile} DESTINATION ${MAN_DIR}/man${num})
ENDMACRO(ADD_MAN_PAGE num manfile)


# Local Variables:
# tab-width: 8
# mode: sh
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
