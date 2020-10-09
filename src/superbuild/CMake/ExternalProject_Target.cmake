# This is based on the logic generated by CMake for EXPORT, but customized for
# use with ExternalProject:
#
# https://cmake.org/cmake/help/latest/module/ExternalProject.html
#
# The goal is to create an imported target based on the ExternalProject
# information, and then append the necessary install logic to manage RPath
# settings in the external projects as if the external files were built by the
# main CMake project.

# TODO - need to rework this in a way that's compatible with the superbuild
# pattern.  Main BRL-CAD build won't have the ExternalProject_Add targets from
# the superbuild, so we need another way to handle getting the necessary
# info to the main build.  Maybe useful:
# https://gitlab.kitware.com/cmake/community/-/wikis/doc/tutorials/How-to-create-a-ProjectConfig.cmake-file

# The catch to this is that the external project outputs MUST be built in a way
# that is compatible with CMake's RPath handling assumptions.  See
# https://stackoverflow.com/questions/41175354/can-i-install-shared-imported-library
# for one of the issues surrounding this - file(RPATH_CHANGE) must be able to
# succeed, and it is up to the 3rd party build setups to prepare their outputs
# to be ready.  The key variable CMAKE_BUILD_RPATH comes from running the
# function cmake_set_rpath, which must be available.

# Be quite about tool outputs by default
if(NOT DEFINED EXTPROJ_VERBOSE)
  set(EXTPROJ_VERBOSE 0)
endif(NOT DEFINED EXTPROJ_VERBOSE)

# When staging files in the build directory, we have to be aware of multiple
# configurations.  This is done post-ExternalProject build, at the parent build
# time, so it needs to be a custom command. Until add_custom_command outputs
# can use $<CONFIG>, we need to handle multi-config situations manually with
# multiple copy commands.
#
# cmake -E copy follows symlinks to get the final file, which is not what we
# want in this situation.  To avoid this, we create a copy script which uses
# file(COPY) and run that script with cmake -P
file(WRITE "${CMAKE_BINARY_DIR}/CMakeFiles/cp.cmake" "get_filename_component(DDIR \${DEST} DIRECTORY)\nfile(COPY \${SRC} DESTINATION \${DDIR})")

function(fcfgcpy outvar extproj root dir ofile tfile)
  #message("extproj: ${extproj}")
  #message("root: ${root}")
  #message("dir: ${dir}")
  #message("ofile: ${ofile}")
  #message("tfile: ${tfile}")
  if (NOT CMAKE_CONFIGURATION_TYPES)
    add_custom_command(
      OUTPUT "${CMAKE_BINARY_DIR}/${dir}/${tfile}"
      COMMAND ${CMAKE_COMMAND} -DSRC="${root}/${ofile}" -DDEST="${CMAKE_BINARY_DIR}/${dir}/${tfile}" -P "${CMAKE_BINARY_DIR}/CMakeFiles/cp.cmake"
      DEPENDS ${extproj}
      )
    set(TOUT ${TOUT} "${CMAKE_BINARY_DIR}/${dir}/${tfile}")
  else (NOT CMAKE_CONFIGURATION_TYPES)
    foreach(CFG_TYPE ${CMAKE_CONFIGURATION_TYPES})
      add_custom_command(
	OUTPUT "${CMAKE_BINARY_DIR}/${CFG_TYPE}/${dir}/${tfile}"
	COMMAND ${CMAKE_COMMAND} -DSRC="${root}/${ofile}" -DDEST="${CMAKE_BINARY_DIR}/${CFG_TYPE}/${dir}/${tfile}" -P "${CMAKE_BINARY_DIR}/CMakeFiles/cp.cmake"
	DEPENDS ${extproj}
	)
      set(TOUT ${TOUT} "${CMAKE_BINARY_DIR}/${CFG_TYPE}/${dir}/${tfile}")
    endforeach(CFG_TYPE ${CMAKE_CONFIGURATION_TYPES})
  endif (NOT CMAKE_CONFIGURATION_TYPES)
  set(${outvar} ${TOUT} PARENT_SCOPE)
endfunction(fcfgcpy file)


# Custom patch utility to replace the build directory path with the install
# directory path in text files - make sure CMAKE_BINARY_DIR and
# CMAKE_INSTALL_PREFIX are finalized before generating this file!
configure_file(${CMAKE_SOURCE_DIR}/CMake/buildpath_replace.cxx.in ${CMAKE_CURRENT_BINARY_DIR}/buildpath_replace.cxx)
add_executable(buildpath_replace ${CMAKE_CURRENT_BINARY_DIR}/buildpath_replace.cxx)

function(ExternalProject_ByProducts etarg extproj extroot E_IMPORT_PREFIX target_dir)

  cmake_parse_arguments(E "FIXPATH" "" "" ${ARGN})

  if (EXTPROJ_VERBOSE)

    list(LENGTH E_UNPARSED_ARGUMENTS FCNT)
    if (E_FIXPATH)
      if (FCNT GREATER 1)
	message("${extproj}: Adding path adjustment and installation rules for ${FCNT} byproducts")
      else (FCNT GREATER 1)
	message("${extproj}: Adding path adjustment and installation rules for ${FCNT} byproduct")
      endif (FCNT GREATER 1)
    else (E_FIXPATH)
      if (FCNT GREATER 1)
	message("${extproj}: Adding install rules for ${FCNT} byproducts")
      else (FCNT GREATER 1)
	message("${extproj}: Adding install rules for ${FCNT} byproduct")
      endif (FCNT GREATER 1)
    endif (E_FIXPATH)

  endif (EXTPROJ_VERBOSE)

  if (NOT TARGET ${etarg}_stage)
    add_custom_target(${etarg}_stage ALL)
  endif (NOT TARGET ${etarg}_stage)

  set(ALL_TOUT)
  foreach (bpf ${E_UNPARSED_ARGUMENTS})
    # If a relative prefix was specified, construct the "source" file using it.
    # This is used to save verbosity in ByProducts input lists.
    if (E_IMPORT_PREFIX)
      set(ofile ${E_IMPORT_PREFIX}/${bpf})
    else (E_IMPORT_PREFIX)
      set(ofile ${bpf})
    endif (E_IMPORT_PREFIX)

    unset(TOUT)
    fcfgcpy(TOUT ${extproj} ${extroot} "${target_dir}" ${ofile} ${bpf})
    set(ALL_TOUT ${ALL_TOUT} ${TOUT})

    install(FILES "${CMAKE_BINARY_DIR}/$<CONFIG>/${target_dir}/${bpf}" DESTINATION "${target_dir}/")
    if (E_FIXPATH)
      # Note - proper quoting for install(CODE) is extremely important for CPack, see
      # https://stackoverflow.com/a/48487133
      install(CODE "execute_process(COMMAND $<TARGET_FILE:buildpath_replace> \"\${CMAKE_INSTALL_PREFIX}/${E_IMPORT_PREFIX}/${bpf}\")")
    endif (E_FIXPATH)

  endforeach (bpf ${E_UNPARSED_ARGUMENTS})

  if (ALL_TOUT)
    string(MD5 UKEY "${ALL_TOUT}")
    add_custom_target(${etarg}_${UKEY} ALL DEPENDS ${ALL_TOUT})
    add_dependencies(${etarg}_stage ${etarg}_${UKEY})
  endif (ALL_TOUT)

endfunction(ExternalProject_ByProducts)


function(ET_target_props etarg IN_IMPORT_PREFIX IN_LINK_TARGET)

  cmake_parse_arguments(ET "STATIC;EXEC" "LINK_TARGET_DEBUG" "" ${ARGN})

  if(NOT CMAKE_CONFIGURATION_TYPES)

    if(ET_STATIC)
      set(IMPORT_PREFIX "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
    elseif(ET_EXEC)
      set(IMPORT_PREFIX "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    else()
      set(IMPORT_PREFIX "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    endif(ET_STATIC)
    message("prefix: ${IMPORT_PREFIX}")
    if(IN_IMPORT_PREFIX)
      set(IMPORT_PREFIX "${IMPORT_PREFIX}/${IN_IMPORT_PREFIX}")
    endif(IN_IMPORT_PREFIX)

    set_property(TARGET ${etarg} APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
    set_target_properties(${etarg} PROPERTIES
      IMPORTED_LOCATION_NOCONFIG "${IMPORT_PREFIX}/${IN_LINK_TARGET}"
      IMPORTED_SONAME_NOCONFIG "${IN_LINK_TARGET}"
      )

    message("${IMPORT_PREFIX}/${IN_LINK_TARGET}")
    message("${IN_LINK_TARGET}")

  else(NOT CMAKE_CONFIGURATION_TYPES)

    foreach(CFG_TYPE ${CMAKE_CONFIGURATION_TYPES})
      string(TOUPPER "${CFG_TYPE}" CFG_TYPE_UPPER)

      # The config variables are the ones set in this mode.
      if(ET_STATIC)
	set(IMPORT_PREFIX "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFG_TYPE_UPPER}}")
      elseif(ET_EXEC)
	set(IMPORT_PREFIX "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG_TYPE_UPPER}}")
      else()
	set(IMPORT_PREFIX "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFG_TYPE_UPPER}}")
      endif(ET_STATIC)

      if(IN_IMPORT_PREFIX)
	set(IMPORT_PREFIX "${IMPORT_PREFIX}/${IN_IMPORT_PREFIX}")
      endif(IN_IMPORT_PREFIX)

      message("IMPORT_PREFIX: ${IMPORT_PREFIX}")

      if("${CFG_TYPE_UPPER}" STREQUAL "DEBUG")
	set(LINK_TARGET ${ET_LINK_TARGET_DEBUG})
      else("${CFG_TYPE_UPPER}" STREQUAL "DEBUG")
	set(LINK_TARGET ${IN_LINK_TARGET})
      endif("${CFG_TYPE_UPPER}" STREQUAL "DEBUG")

      message("LINK_TARGET: ${LINK_TARGET}")

      set_target_properties(${etarg} PROPERTIES
	IMPORTED_LOCATION_${CFG_TYPE_UPPER} "${IMPORT_PREFIX}/${LINK_TARGET}"
	IMPORTED_SONAME_${CFG_TYPE_UPPER} "${LINK_TARGET}"
	)

      if(NOT ET_STATIC AND NOT ET_EXEC AND MSVC)
	# For Windows, IMPORTED_IMPLIB is important for shared libraries.
	# It is that property that will tell a toplevel target what to link against
	# when building - pointing out the dll isn't enough by itself.
	string(REPLACE "${CMAKE_SHARED_LIBRARY_SUFFIX}" ".lib" IMPLIB_FILE "${LINK_TARGET}")
	set_target_properties(${etarg} PROPERTIES
	  IMPORTED_IMPLIB_${CFG_TYPE_UPPER} "${IMPORT_PREFIX}/${IMPLIB_FILE}"
	  )
      endif(NOT ET_STATIC AND NOT ET_EXEC AND MSVC)

    endforeach(CFG_TYPE ${CMAKE_CONFIGURATION_TYPES})

    # For everything except Debug, use the Release version
    #set_target_properties(TARGET ${etarg} PROPERTIES
    #  MAP_IMPORTED_CONFIG_MINSIZEREL Release
    #  MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
    #  )

  endif(NOT CMAKE_CONFIGURATION_TYPES)

endfunction(ET_target_props)


# For a given path, calculate the $ORIGIN style path needed relative
# to CMAKE_INSTALL_PREFIX
function(ET_Origin_Path POPATH INIT_PATH)

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
endfunction(ET_Origin_Path)

# Mimic the magic of the CMake install(TARGETS) form of the install command.
# This is the key to treating external project build outputs as fully managed
# CMake outputs, and requires that the external project build in such a way
# that the rpath settings in the build outputs are compatible with this
# mechanism.
function(ET_RPath LIB_DIR OUTPUT_DIR SUB_DIR E_OUTPUT_FILE)
  get_filename_component(RRPATH "${CMAKE_INSTALL_PREFIX}/${LIB_DIR}/${SUB_DIR}" REALPATH)
  set(OPATH)
  ET_Origin_Path(OPATH "${RRPATH}")
  if (NOT APPLE)
    set(NEW_RPATH "$ENV{DESTDIR}${RRPATH}:$ORIGIN/${OPATH}")
  else (NOT APPLE)
    set(NEW_RPATH "$ENV{DESTDIR}${RRPATH}:@loader_path/${OPATH}")
  endif (NOT APPLE)
  if (NOT DEFINED CMAKE_BUILD_RPATH)
    message(FATAL_ERROR "ET_RPath run without CMAKE_BUILD_RPATH defined - run cmake_set_rpath before defining external projects.")
  endif (NOT DEFINED CMAKE_BUILD_RPATH)
  if (NOT "${SUB_DIR}" STREQUAL "")
    set(OFINAL "${SUB_DIR}/${E_OUTPUT_FILE}")
  else (NOT "${SUB_DIR}" STREQUAL "")
    set(OFINAL "${E_OUTPUT_FILE}")
  endif (NOT "${SUB_DIR}" STREQUAL "")
  # Note - proper quoting for install(CODE) is extremely important for CPack, see
  # https://stackoverflow.com/a/48487133
  install(CODE "
  file(RPATH_CHANGE
    FILE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${OFINAL}\"
    OLD_RPATH \"${CMAKE_BUILD_RPATH}\"
    NEW_RPATH \"${NEW_RPATH}\")
  ")
endfunction(ET_RPath)



function(ExternalProject_Target etarg extproj extroot)

  cmake_parse_arguments(E "RPATH" "EXEC;SHARED;STATIC;LINK_TARGET;LINK_TARGET_DEBUG;STATIC_LINK_TARGET;STATIC_LINK_TARGET_DEBUG" "SYMLINKS;DEPS" ${ARGN})

  if(NOT TARGET ${extproj})
    message(FATAL_ERROR "${extprog} is not a target")
  endif(NOT TARGET ${extproj})

  # Protect against redefinition of already defined targets.
  if(TARGET ${etarg})
    message(FATAL_ERROR "Target ${etarg} is already defined\n")
  endif(TARGET ${etarg})
  if(E_STATIC AND TARGET ${etarg}-static)
    message(FATAL_ERROR "Target ${etarg}-static is already defined\n")
  endif(E_STATIC AND TARGET ${etarg}-static)

  message("${extproj}: Adding target \"${etarg}\"")

  if (E_LINK_TARGET AND NOT MSVC)
    set(LINK_TARGET "${E_LINK_TARGET}")
  endif (E_LINK_TARGET AND NOT MSVC)
  if (NOT MSVC)
    if (E_LINK_TARGET_DEBUG)
      set(LINK_TARGET_DEBUG "${E_LINK_TARGET_DEBUG}")
    elseif (E_LINK_TARGET)
      set(LINK_TARGET_DEBUG "${E_LINK_TARGET}")
    else (E_LINK_TARGET_DEBUG)
      set(LINK_TARGET_DEBUG "${E_SHARED}")
    endif (E_LINK_TARGET_DEBUG)
  endif (NOT MSVC)
  if (E_STATIC_LINK_TARGET AND NOT MSVC AND BUILD_STATIC_LIBS)
    set(STATIC_LINK_TARGET "${E_STATIC_LINK_TARGET}")
  endif (E_STATIC_LINK_TARGET AND NOT MSVC AND BUILD_STATIC_LIBS)
  if (E_STATIC_LINK_TARGET_DEBUG AND NOT MSVC AND BUILD_STATIC_LIBS)
    set(STATIC_LINK_TARGET_DEBUG "${E_STATIC_LINK_TARGET_DEBUG}")
  endif (E_STATIC_LINK_TARGET_DEBUG AND NOT MSVC AND BUILD_STATIC_LIBS)

  # Create imported target - need to both make the target itself
  # and set the necessary properties.  See also
  # https://gitlab.kitware.com/cmake/community/wikis/doc/tutorials/Exporting-and-Importing-Targets

  # Because the outputs are not properly build target outputs of the primary
  # CMake project, we need to install as either FILES or PROGRAMS
  set(TOUT)

  message("Adding target: ${etarg}")

  # Handle shared library
  if (E_SHARED)
    add_library(${etarg} SHARED IMPORTED GLOBAL)
    string(REPLACE "${LIB_DIR}/" ""  ENAME ${E_SHARED})
    fcfgcpy(TOUT ${extproj} ${extroot} ${LIB_DIR} ${E_SHARED} ${ENAME})
    if (E_LINK_TARGET AND NOT MSVC)
      ET_target_props(${etarg} "${E_IMPORT_PREFIX}" ${E_LINK_TARGET} LINK_TARGET_DEBUG "${LINK_TARGET_DEBUG}")
      string(REPLACE "${LIB_DIR}/" ""  ENAME ${E_LINK_TARGET})
    else (E_LINK_TARGET AND NOT MSVC)
      ET_target_props(${etarg} "${E_IMPORT_PREFIX}" ${E_SHARED} LINK_TARGET_DEBUG "${LINK_TARGET_DEBUG}")
    endif (E_LINK_TARGET AND NOT MSVC)

    install(FILES "${CMAKE_BINARY_DIR}/$<CONFIG>/${E_SHARED}" DESTINATION ${LIB_DIR}/${E_SUBDIR})

    # Perform RPath magic
    if (E_RPATH AND NOT MSVC)
      ET_RPath("${LIB_DIR}" "${LIB_DIR}" "" "${E_SHARED}")
    endif (E_RPATH AND NOT MSVC)

  endif (E_SHARED)

  # If we do have a static lib as well, handle that
  if (E_STATIC AND BUILD_STATIC_LIBS)
    add_library(${etarg}-static STATIC IMPORTED GLOBAL)
    if (E_STATIC_LINK_TARGET AND NOT MSVC)
      ET_target_props(${etarg}-static "${E_IMPORT_PREFIX}" ${E_STATIC_LINK_TARGET} STATIC_LINK_TARGET_DEBUG "${STATIC_LINK_TARGET_DEBUG}" STATIC)
    else (E_STATIC_LINK_TARGET AND NOT MSVC)
      ET_target_props(${etarg}-static "${E_IMPORT_PREFIX}" ${E_STATIC_OUTPUT_FILE} STATIC_LINK_TARGET_DEBUG "${STATIC_LINK_TARGET_DEBUG}" STATIC)
    endif (E_STATIC_LINK_TARGET AND NOT MSVC)
    if (MSVC)
      string(REPLACE "${BIN_DIR}/" ""  ENAME ${E_STATIC})
      fcfgcpy(TOUT ${extproj} ${extroot} ${BIN_DIR} ${E_STATIC} ${ENAME})
      install(FILES "${CMAKE_BINARY_DIR}/$<CONFIG>/${E_STATIC}" DESTINATION ${BIN_DIR}/${E_SUBDIR})
    else (MSVC)
      string(REPLACE "${LIB_DIR}/" ""  ENAME ${E_STATIC})
      fcfgcpy(TOUT ${extproj} ${extroot} ${LIB_DIR} ${E_STATIC} ${ENAME})
      install(FILES "${CMAKE_BINARY_DIR}/$<CONFIG>/${E_STATIC}" DESTINATION ${LIB_DIR}/${E_SUBDIR})
    endif (MSVC)
    # Let CMake know there is a target dependency here, despite this being an import target
    add_dependencies(${etarg}-static ${extproj})
  endif (E_STATIC AND BUILD_STATIC_LIBS)

  if (E_EXEC)
    add_executable(${etarg} IMPORTED GLOBAL)

    string(REPLACE "${BIN_DIR}/" ""  ENAME ${E_EXEC})
    fcfgcpy(TOUT ${extproj} ${extroot} ${BIN_DIR} ${E_EXEC} ${ENAME})
    ET_target_props(${etarg} "${E_IMPORT_PREFIX}" ${E_EXEC} EXEC)
    install(PROGRAMS "${CMAKE_BINARY_DIR}/$<CONFIG>/${E_EXEC}" DESTINATION ${BIN_DIR}/${E_SUBDIR})

    # Let CMake know there is a target dependency here, despite this being an import target
    add_dependencies(${etarg} ${extproj})

    # Perform RPath magic
    if (E_RPATH AND NOT MSVC)
      ET_RPath("${LIB_DIR}" "${BIN_DIR}" "" "${E_EXEC}")
    endif (E_RPATH AND NOT MSVC)
  endif (E_EXEC)

    # Add install rules for any symlinks the caller has listed
  if(E_SYMLINKS AND NOT MSVC)
    foreach(slink ${E_SYMLINKS})
      string(REPLACE "${LIB_DIR}/" ""  ENAME ${slink})
      if (NOT BUILD_STATIC_LIBS)
	if (NOT "${slink}" MATCHES ".*${CMAKE_STATIC_LIBRARY_SUFFIX}")
	  fcfgcpy(TOUT ${extproj} ${extroot} ${LIB_DIR} ${slink} ${ENAME})
	  install(FILES "${CMAKE_BINARY_DIR}/$<CONFIG>/${LIB_DIR}/${ENAME}" DESTINATION ${LIB_DIR})
	endif (NOT "${slink}" MATCHES ".*${CMAKE_STATIC_LIBRARY_SUFFIX}")
      else (NOT BUILD_STATIC_LIBS)
	fcfgcpy(TOUT ${extproj} ${extroot} ${LIB_DIR} ${slink} ${ENAME})
	install(FILES "${CMAKE_BINARY_DIR}/$<CONFIG>/${LIB_DIR}/${ENAME}" DESTINATION ${LIB_DIR})
      endif (NOT BUILD_STATIC_LIBS)
    endforeach(slink ${E_SYMLINKS})
  endif(E_SYMLINKS AND NOT MSVC)

  # Let CMake know there is a target dependency here, despite this being an import target

  if (TOUT)
    if (NOT TARGET ${etarg}_stage)
      add_custom_target(${etarg}_stage ALL DEPENDS ${TOUT})
    else (NOT TARGET ${etarg}_stage)
      add_dependencies(${etarg}_stage ${TOUT})
    endif (NOT TARGET ${etarg}_stage)
  endif (TOUT)

  if (TARGET ${etarg})
    add_dependencies(${etarg} ${etarg}_stage ${extproj})
  endif (TARGET ${etarg})
  if (TARGET ${etarg}-static AND BUILD_STATIC_LIBS)
    add_dependencies(${etarg}-static ${etarg}_stage ${extproj})
  endif (TARGET ${etarg}-static AND BUILD_STATIC_LIBS)

endfunction(ExternalProject_Target)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8

