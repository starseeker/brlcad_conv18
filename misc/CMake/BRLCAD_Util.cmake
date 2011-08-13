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

# Macro for three-way options that optionally check whether a system
# resource is available.
MACRO(BUNDLE_OPTION optname default)
	IF(NOT ${optname})
		IF(default)
			SET(${optname} "${default}" CACHE STRING "Build bundled ${optname} libraries.")
		ELSE(default)
			IF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "Bundled")
				SET(${optname} "Auto (B)" CACHE STRING "Build bundled ${optname} libraries.")
			ENDIF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "Bundled")
			IF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "System")
				SET(${optname} "Auto (S)" CACHE STRING "Build bundled ${optname} libraries.")
			ENDIF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "System")
			IF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "Auto")
				SET(${optname} "Auto" CACHE STRING "Build bundled ${optname} libraries.")
			ENDIF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "Auto")
		ENDIF(default)
	ENDIF(NOT ${optname})
	IF(${optname} STREQUAL "Auto (B)" OR ${optname} STREQUAL "Auto (S)" OR ${optname} STREQUAL "Auto")
		IF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "Bundled")
			SET(${optname} "Auto (B)" CACHE STRING "Build bundled ${optname} libraries." FORCE)
		ENDIF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "Bundled")
		IF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "System")
			SET(${optname} "Auto (S)" CACHE STRING "Build bundled ${optname} libraries." FORCE)
		ENDIF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "System")
		IF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "Auto")
			SET(${optname} "Auto" CACHE STRING "Build bundled ${optname} libraries." FORCE)
		ENDIF(${${CMAKE_PROJECT_NAME}_BUNDLED_LIBS} STREQUAL "Auto")
	ENDIF(${optname} STREQUAL "Auto (B)" OR ${optname} STREQUAL "Auto (S)" OR ${optname} STREQUAL "Auto")
	set_property(CACHE ${optname} PROPERTY STRINGS Auto Bundled System)
	IF(NOT ${${optname}} STREQUAL "Auto" AND NOT ${${optname}} STREQUAL "Bundled" AND NOT ${${optname}} STREQUAL "System" AND NOT ${${optname}} STREQUAL "Auto (B)" AND NOT ${${optname}} STREQUAL "Auto (S)")
		MESSAGE(WARNING "Unknown value ${${optname}} supplied for ${optname} - defaulting to Auto")
		MESSAGE(WARNING "Valid options are Auto, Bundled and System")
		SET(${optname} "Auto" CACHE STRING "Build bundled libraries." FORCE)
	ENDIF(NOT ${${optname}} STREQUAL "Auto" AND NOT ${${optname}} STREQUAL "Bundled" AND NOT ${${optname}} STREQUAL "System" AND NOT ${${optname}} STREQUAL "Auto (B)" AND NOT ${${optname}} STREQUAL "Auto (S)")
ENDMACRO()

# Build Type aware option
MACRO(AUTO_OPTION username varname debug_state release_state)
	STRING(LENGTH "${${username}}" ${username}_SET)
	IF(NOT ${username}_SET)
		SET(${username} "Auto" CACHE STRING "Use optimized compiler settings" FORCE)
	ENDIF(NOT ${username}_SET)
	set_property(CACHE ${username} PROPERTY STRINGS Auto "ON" "OFF")
	# If the "parent" setting isn't Auto, do what it says
	IF(NOT ${${username}} STREQUAL "Auto")
		SET(${varname} ${${username}})
	ENDIF(NOT ${${username}} STREQUAL "Auto")
	# If we we don't understand the build type and have an Auto setting for the
	# optimization flags, leave them off
	IF(NOT "${CMAKE_BUILD_TYPE}" MATCHES "Release" AND NOT "${CMAKE_BUILD_TYPE}" MATCHES "Debug")
		IF(NOT ${${username}} STREQUAL "Auto")
			SET(${varname} OFF)
		ENDIF(NOT ${${username}} STREQUAL "Auto")
	ENDIF(NOT "${CMAKE_BUILD_TYPE}" MATCHES "Release" AND NOT "${CMAKE_BUILD_TYPE}" MATCHES "Debug")
	# If we DO understand the build type and have Auto, be smart
	IF("${CMAKE_BUILD_TYPE}" MATCHES "Release" AND ${${username}} STREQUAL "Auto")
		SET(${varname} ${release_state})
	ENDIF("${CMAKE_BUILD_TYPE}" MATCHES "Release" AND ${${username}} STREQUAL "Auto")
	IF("${CMAKE_BUILD_TYPE}" MATCHES "Debug" AND ${${username}} STREQUAL "Auto")
		SET(${varname} ${debug_state})
	ENDIF("${CMAKE_BUILD_TYPE}" MATCHES "Debug" AND ${${username}} STREQUAL "Auto")
ENDMACRO(AUTO_OPTION varname release_state debug_state)

# Windows builds need a DLL variable defined per-library, and BRL-CAD
# uses a fairly standard convention - try and automate the addition of
# the definition.
MACRO(DLL_DEFINE libname)
	IF(MSVC)
		STRING(REGEX REPLACE "lib" "" LOWERCORE "${libname}")
		STRING(TOUPPER ${LOWERCORE} UPPER_CORE)
		add_definitions("-D${UPPER_CORE}_EXPORT_DLL")
	ENDIF(MSVC)
ENDMACRO()

INCLUDE(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG(-Wno-error NOERROR_FLAG)

MACRO(CPP_WARNINGS srcslist)
	# We need to specify specific flags for c++ files - their warnings are
	# not usually used to hault the build, althogh BRLCAD-ENABLE_CXX_STRICT
	# can be set to on to achieve that
	IF(BRLCAD-ENABLE_STRICT AND NOT BRLCAD-ENABLE_CXX_STRICT)
		FOREACH(srcfile ${${srcslist}})
			IF(${srcfile} MATCHES "cpp$" OR ${srcfile} MATCHES "cc$")
				IF(BRLCAD-ENABLE_COMPILER_WARNINGS)
					IF(NOERROR_FLAG)
						set_source_files_properties(${srcfile} COMPILE_FLAGS "-Wno-error")
					ENDIF(NOERROR_FLAG)
				ELSE(BRLCAD-ENABLE_COMPILER_WARNINGS)
					set_source_files_properties(${srcfile} COMPILE_FLAGS "-w")
				ENDIF(BRLCAD-ENABLE_COMPILER_WARNINGS)
			ENDIF(${srcfile} MATCHES "cpp$" OR ${srcfile} MATCHES "cc$")
		ENDFOREACH(srcfile ${${srcslist}})
	ENDIF(BRLCAD-ENABLE_STRICT AND NOT BRLCAD-ENABLE_CXX_STRICT)
ENDMACRO(CPP_WARNINGS)

# Core routines for adding executables and libraries to the build and
# install lists of CMake
MACRO(BRLCAD_ADDEXEC execname srcs libs)
  STRING(REGEX REPLACE " " ";" srcslist "${srcs}")
  STRING(REGEX REPLACE " " ";" libslist1 "${libs}")
  STRING(REGEX REPLACE "-framework;" "-framework " libslist "${libslist1}")
  add_executable(${execname} ${srcslist})
  IF(MSVC)
	  SET_TARGET_PROPERTIES(${execname} PROPERTIES COMPILE_FLAGS "-DBRLCAD_DLL")
  ENDIF(MSVC)
  target_link_libraries(${execname} ${libslist})
  INSTALL(TARGETS ${execname} DESTINATION ${BIN_DIR})
  FOREACH(extraarg ${ARGN})
	  IF(${extraarg} MATCHES "NOSTRICT" AND BRLCAD-ENABLE_STRICT)
		  IF(NOERROR_FLAG)
			  SET_TARGET_PROPERTIES(${execname} PROPERTIES COMPILE_FLAGS "-Wno-error")
		  ENDIF(NOERROR_FLAG)
	  ENDIF(${extraarg} MATCHES "NOSTRICT" AND BRLCAD-ENABLE_STRICT)
  ENDFOREACH(extraarg ${ARGN})
  CPP_WARNINGS(srcslist)
ENDMACRO(BRLCAD_ADDEXEC execname srcs libs)

# Library macro handles both shared and static libs, so one "BRLCAD_ADDLIB"
# statement will cover both automatically
MACRO(BRLCAD_ADDLIB libname srcs libs)
  STRING(REGEX REPLACE " " ";" srcslist "${srcs}")
  STRING(REGEX REPLACE " " ";" libslist1 "${libs}")
  STRING(REGEX REPLACE "-framework;" "-framework " libslist "${libslist1}")
  DLL_DEFINE(${libname})
  IF(BUILD_SHARED_LIBS)
	  add_library(${libname} SHARED ${srcslist})
	  IF(MSVC)
		  SET_TARGET_PROPERTIES(${libname} PROPERTIES COMPILE_FLAGS "-DBRLCAD_DLL")
	  ENDIF(MSVC)
	  if(NOT ${libs} MATCHES "NONE")
		  target_link_libraries(${libname} ${libslist})
	  endif(NOT ${libs} MATCHES "NONE")
	  INSTALL(TARGETS ${libname} DESTINATION ${LIB_DIR})
	  FOREACH(extraarg ${ARGN})
		  IF(${extraarg} MATCHES "NOSTRICT" AND BRLCAD-ENABLE_STRICT)
			  IF(NOERROR_FLAG)
				  SET_TARGET_PROPERTIES(${libname} PROPERTIES COMPILE_FLAGS "-Wno-error")
			  ENDIF(NOERROR_FLAG)
		  ENDIF(${extraarg} MATCHES "NOSTRICT" AND BRLCAD-ENABLE_STRICT)
	  ENDFOREACH(extraarg ${ARGN})
  ENDIF(BUILD_SHARED_LIBS)
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
		  IF(${extraarg} MATCHES "NOSTRICT" AND BRLCAD-ENABLE_STRICT)
			  IF(NOERROR_FLAG)
				  SET_TARGET_PROPERTIES(${libname}-static PROPERTIES COMPILE_FLAGS "-Wno-error")
			  ENDIF(NOERROR_FLAG)
		  ENDIF(${extraarg} MATCHES "NOSTRICT" AND BRLCAD-ENABLE_STRICT)
	  ENDFOREACH(extraarg ${ARGN})
  ENDIF(BUILD_STATIC_LIBS)
  CPP_WARNINGS(srcslist)
ENDMACRO(BRLCAD_ADDLIB libname srcs libs)

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

MACRO(BRLCAD_ADDFILE filename targetdir)
	FILE(COPY ${filename} DESTINATION ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir})
	STRING(REGEX REPLACE "/" "_" targetprefix ${targetdir})
	IF(BRLCAD-ENABLE_DATA_TARGETS)
		STRING(REGEX REPLACE "/" "_" filestring ${filename})
		ADD_CUSTOM_COMMAND(
			OUTPUT ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${filename}
			COMMAND ${CMAKE_COMMAND} -E copy	${CMAKE_CURRENT_SOURCE_DIR}/${filename} ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${filename}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${filename}
			)
		ADD_CUSTOM_TARGET(${targetprefix}_${filestring}_cp ALL DEPENDS ${CMAKE_BINARY_DIR}/${DATA_DIR}/${targetdir}/${filename})
	ENDIF(BRLCAD-ENABLE_DATA_TARGETS)
	INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${filename} DESTINATION ${DATA_DIR}/${targetdir})
	FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/cmakefiles.cmake "${filename}\n")
ENDMACRO(BRLCAD_ADDFILE datalist targetdir)

MACRO(DISTCHECK_IGNORE_ITEM itemtoignore)
	IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${itemtoignore})
		FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/cmakefiles.cmake "${itemtoignore}\n")
	ELSE(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${itemtoignore})
		MESSAGE(FATAL_ERROR "Attempting to ignore non-existent file ${itemtoignore}")
	ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${itemtoignore})
ENDMACRO(DISTCHECK_IGNORE_ITEM)


MACRO(DISTCHECK_IGNORE targetdir filestoignore)
	FOREACH(ITEM ${${filestoignore}})
		get_filename_component(item_path ${ITEM} PATH)
		get_filename_component(item_name ${ITEM} NAME)
		IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${targetdir}/${ITEM})
			IF(item_path)
				IF(NOT IS_DIRECTORY(${CMAKE_CURRENT_BINARY_DIR}/${targetdir}/${item_path}))
					FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${targetdir}/${item_path})
				ENDIF()
				FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${targetdir}/${item_path}/cmakefiles.cmake "${item_name}\n")
				WHILE(item_path)
					get_filename_component(item_name ${item_path} NAME)
					get_filename_component(item_path ${item_path} PATH)
					FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${targetdir}/${item_path}/cmakepaths.cmake "${item_name}\n")
				ENDWHILE(item_path)
			ELSE(item_path)
				FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/${targetdir}/cmakefiles.cmake "${ITEM}\n")
			ENDIF(item_path)
		ELSE(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${targetdir}/${ITEM})
			MESSAGE(FATAL_ERROR "Attempting to ignore non-existent file ${CMAKE_CURRENT_SOURCE_DIR}/${targetdir}/${ITEM}")
		ENDIF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${targetdir}/${ITEM})
	ENDFOREACH(ITEM ${${filestoignore}})
ENDMACRO(DISTCHECK_IGNORE)

MACRO(ADD_MAN_PAGES num manlist)
	CMAKEFILES(${${manlist}})
	FOREACH(manpage ${${manlist}})
		install(FILES ${manpage} DESTINATION ${MAN_DIR}/man${num})
	ENDFOREACH(manpage ${${manlist}})
ENDMACRO(ADD_MAN_PAGES num manlist)

MACRO(ADD_MAN_PAGE num manfile)
	FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/cmakefiles.cmake "${manfile}\n")
	install(FILES ${manfile} DESTINATION ${MAN_DIR}/man${num})
ENDMACRO(ADD_MAN_PAGE num manfile)

