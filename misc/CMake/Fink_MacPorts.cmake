#             F I N K _ M A C P O R T S . C M A K E
# BRL-CAD
#
# Copyright (c) 2011-2012 United States Government as represented by
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
# Fink and/or Macports complicate searching for libraries
# on OSX.  Provide a way to specify whether to search using
# them (if available) or just use System paths.

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	FIND_PROGRAM(PORT_EXEC port)
	MARK_AS_ADVANCED(PORT_EXEC)
	FIND_PROGRAM(FINK_EXEC fink)
	MARK_AS_ADVANCED(FINK_EXEC)
	IF(PORT_EXEC OR FINK_EXEC)
		IF(NOT CMAKE_SEARCH_OSX_PATHS)
			SET(OSX_PATH_OPTIONS "System")
			IF(PORT_EXEC)
				LIST(APPEND OSX_PATH_OPTIONS "MacPorts")
				IF(NOT FINK_EXEC)
					SET(CMAKE_SEARCH_OSX_PATHS "MacPorts" CACHE STRING "Use Macports")
				ENDIF(NOT FINK_EXEC)
			ENDIF(PORT_EXEC)
			IF(FINK_EXEC)
				LIST(APPEND OSX_PATH_OPTIONS "Fink")
				IF(NOT PORT_EXEC)
					SET(CMAKE_SEARCH_OSX_PATHS "Fink" CACHE STRING "Use Fink")
				ENDIF(NOT PORT_EXEC)
			ENDIF(FINK_EXEC)
			IF(NOT CMAKE_SEARCH_OSX_PATHS)
				SET(CMAKE_SEARCH_OSX_PATHS "System" CACHE STRING "Use System")
			ENDIF(NOT CMAKE_SEARCH_OSX_PATHS)
			set_property(CACHE CMAKE_SEARCH_OSX_PATHS PROPERTY STRINGS ${OSX_PATH_OPTIONS})
		ENDIF(NOT CMAKE_SEARCH_OSX_PATHS)
		STRING(TOUPPER "${CMAKE_SEARCH_OSX_PATHS}" paths_upper)
		SET(CMAKE_SEARCH_OSX_PATHS ${paths_upper})
		IF(NOT ${CMAKE_SEARCH_OSX_PATHS} STREQUAL "SYSTEM" AND NOT ${CMAKE_SEARCH_OSX_PATHS} STREQUAL "MACPORTS" AND NOT ${CMAKE_SEARCH_OSX_PATHS} STREQUAL "FINK")
			MESSAGE(WARNING "Unknown value ${CMAKE_SEARCH_OSX_PATHS} supplied for CMAKE_SEARCH_OSX_PATHS - defaulting to System")
			SET(CMAKE_SEARCH_OSX_PATHS "SYSTEM")
		ENDIF(NOT ${CMAKE_SEARCH_OSX_PATHS} STREQUAL "SYSTEM" AND NOT ${CMAKE_SEARCH_OSX_PATHS} STREQUAL "MACPORTS" AND NOT ${CMAKE_SEARCH_OSX_PATHS} STREQUAL "FINK")
		IF(${CMAKE_SEARCH_OSX_PATHS} STREQUAL "MACPORTS" AND NOT PORT_EXEC) 
			MESSAGE(WARNING "CMAKE_SEARCH_OSX_PATHS set to MacPorts, but port executable not found - defaulting to System")
			SET(CMAKE_SEARCH_OSX_PATHS "SYSTEM")
		ENDIF(${CMAKE_SEARCH_OSX_PATHS} STREQUAL "MACPORTS" AND NOT PORT_EXEC) 
		IF(${CMAKE_SEARCH_OSX_PATHS} STREQUAL "FINK" AND NOT FINK_EXEC) 
			MESSAGE(WARNING "CMAKE_SEARCH_OSX_PATHS set to Fink, but fink executable not found - defaulting to System")
			SET(CMAKE_SEARCH_OSX_PATHS "SYSTEM")
		ENDIF(${CMAKE_SEARCH_OSX_PATHS} STREQUAL "FINK" AND NOT FINK_EXEC)
		IF(${CMAKE_SEARCH_OSX_PATHS} STREQUAL "MACPORTS")
			get_filename_component(port_binpath ${PORT_EXEC} PATH)
			get_filename_component(port_path ${port_binpath} PATH)
			get_filename_component(port_path_normalized ${port_path} ABSOLUTE)
			SET(CMAKE_LIBRARY_PATH ${port_path_normalized}/lib ${CMAKE_LIBRARY_PATH})
			SET(CMAKE_INCLUDE_PATH ${port_path_normalized}/include ${CMAKE_INCLUDE_PATH})
			IF(CMAKE_SYSTEM_IGNORE_PATH)
				LIST(REMOVE_ITEM CMAKE_SYSTEM_IGNORE_PATH "${port_path_normalized}/lib")
			ENDIF(CMAKE_SYSTEM_IGNORE_PATH)
			IF(FINK_EXEC)
				get_filename_component(fink_binpath ${FINK_EXEC} PATH)
				get_filename_component(fink_path ${fink_binpath} PATH)
				get_filename_component(fink_path_normalized ${fink_path} ABSOLUTE)
				IF("${fink_path_normalized}" STREQUAL "${port_path_normalized}")
					MESSAGE(WARNING "Both Fink and MacPorts appear to be installed in ${fink_path_normalized}, search results unpredictable")
				ELSE("${fink_path_normalized}" STREQUAL "${port_path_normalized}")
					SET(CMAKE_SYSTEM_IGNORE_PATH ${CMAKE_SYSTEM_IGNORE_PATH} ${fink_path_normalized}/lib)
					IF(CMAKE_LIBRARY_PATH)
						LIST(REMOVE_ITEM CMAKE_LIBRARY_PATH "${fink_path_normalized}/lib")
					ENDIF(CMAKE_LIBRARY_PATH)
					IF(CMAKE_INCLUDE_PATH)
						LIST(REMOVE_ITEM CMAKE_INCLUDE_PATH "${fink_path_normalized}/include")
					ENDIF(CMAKE_INCLUDE_PATH)
				ENDIF("${fink_path_normalized}" STREQUAL "${port_path_normalized}")
			ENDIF(FINK_EXEC)
		ENDIF(${CMAKE_SEARCH_OSX_PATHS} STREQUAL "MACPORTS")
		IF(${CMAKE_SEARCH_OSX_PATHS} STREQUAL "FINK")
			get_filename_component(fink_binpath ${FINK_EXEC} PATH)
			get_filename_component(fink_path ${fink_binpath} PATH)
			get_filename_component(fink_path_normalized ${fink_path} ABSOLUTE)
			SET(CMAKE_LIBRARY_PATH ${fink_path_normalized}/lib ${CMAKE_LIBRARY_PATH})
			SET(CMAKE_INCLUDE_PATH ${fink_path_normalized}/include ${CMAKE_INCLUDE_PATH})
			IF(CMAKE_SYSTEM_IGNORE_PATH)
				LIST(REMOVE_ITEM CMAKE_SYSTEM_IGNORE_PATH "${fink_path_normalized}/lib")
			ENDIF(CMAKE_SYSTEM_IGNORE_PATH)
			IF(PORT_EXEC)
				get_filename_component(port_binpath ${PORT_EXEC} PATH)
				get_filename_component(port_path ${port_binpath} PATH)
				get_filename_component(port_path_normalized ${port_path} ABSOLUTE)
				IF("${fink_path_normalized}" STREQUAL "${port_path_normalized}")
					MESSAGE(WARNING "Both Fink and MacPorts appear to be installed in ${fink_path_normalized}, search results unpredictable")
				ELSE("${fink_path_normalized}" STREQUAL "${port_path_normalized}")
					SET(CMAKE_SYSTEM_IGNORE_PATH ${CMAKE_SYSTEM_IGNORE_PATH} ${port_path_normalized}/lib)
					IF(CMAKE_LIBRARY_PATH)
						LIST(REMOVE_ITEM CMAKE_LIBRARY_PATH "${port_path_normalized}/lib")
					ENDIF(CMAKE_LIBRARY_PATH)
					IF(CMAKE_INCLUDE_PATH)
						LIST(REMOVE_ITEM CMAKE_INCLUDE_PATH "${port_path_normalized}/include")
					ENDIF(CMAKE_INCLUDE_PATH)
				ENDIF("${fink_path_normalized}" STREQUAL "${port_path_normalized}")
			ENDIF(PORT_EXEC)
		ENDIF(${CMAKE_SEARCH_OSX_PATHS} STREQUAL "FINK")
		IF(${CMAKE_SEARCH_OSX_PATHS} STREQUAL "SYSTEM")
			IF(FINK_EXEC)
				get_filename_component(fink_binpath ${FINK_EXEC} PATH)
				get_filename_component(fink_path ${fink_binpath} PATH)
				get_filename_component(fink_path_normalized ${fink_path} ABSOLUTE)
				SET(CMAKE_SYSTEM_IGNORE_PATH ${CMAKE_SYSTEM_IGNORE_PATH} ${fink_path_normalized}/lib)
				SET(CMAKE_SYSTEM_IGNORE_PATH ${CMAKE_SYSTEM_IGNORE_PATH} ${fink_path_normalized}/include)
				IF(CMAKE_LIBRARY_PATH)
					LIST(REMOVE_ITEM CMAKE_LIBRARY_PATH "${fink_path_normalized}/lib")
				ENDIF(CMAKE_LIBRARY_PATH)
				IF(CMAKE_INCLUDE_PATH)
					LIST(REMOVE_ITEM CMAKE_INCLUDE_PATH "${fink_path_normalized}/include")
				ENDIF(CMAKE_INCLUDE_PATH)
			ENDIF(FINK_EXEC)
			IF(PORT_EXEC)
				get_filename_component(port_binpath ${PORT_EXEC} PATH)
				get_filename_component(port_path ${port_binpath} PATH)
				get_filename_component(port_path_normalized ${port_path} ABSOLUTE)
				SET(CMAKE_SYSTEM_IGNORE_PATH ${CMAKE_SYSTEM_IGNORE_PATH} ${port_path_normalized}/lib)
				SET(CMAKE_SYSTEM_IGNORE_PATH ${CMAKE_SYSTEM_IGNORE_PATH} ${port_path_normalized}/include)
				IF(CMAKE_LIBRARY_PATH)
					LIST(REMOVE_ITEM CMAKE_LIBRARY_PATH "${port_path_normalized}/lib")
				ENDIF(CMAKE_LIBRARY_PATH)
				IF(CMAKE_INCLUDE_PATH)
					LIST(REMOVE_ITEM CMAKE_INCLUDE_PATH "${port_path_normalized}/include")
				ENDIF(CMAKE_INCLUDE_PATH)
			ENDIF(PORT_EXEC)
		ENDIF(${CMAKE_SEARCH_OSX_PATHS} STREQUAL "SYSTEM")
	ENDIF(PORT_EXEC OR FINK_EXEC)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=4 tabstop=8
