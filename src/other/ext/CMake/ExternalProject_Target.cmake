# This is based on the logic generated by CMake for EXPORT, but customized for
# use with ExternalProject:
#
# https://cmake.org/cmake/help/latest/module/ExternalProject.html
#
# The goal is to create an imported target based on the ExternalProject
# information, and then append the necessary install logic to manage RPath
# settings in the external projects as if the external files were built by the
# main CMake project.

# The catch to this is that the external project outputs MUST be built in a way
# that is compatible with CMake's RPath handling assumptions.  See
# https://stackoverflow.com/questions/41175354/can-i-install-shared-imported-library
# for one of the issues surrounding this - file(RPATH_CHANGE) must be able to
# succeed, and it is up to the 3rd party build setups to prepare their outputs
# to be ready.  The key variable CMAKE_BUILD_RPATH must be set correctly ahead
# of time - see RPath_Setup.cmake

# Be quite about tool outputs by default
if(NOT DEFINED EXTPROJ_VERBOSE)
  set(EXTPROJ_VERBOSE 0)
endif(NOT DEFINED EXTPROJ_VERBOSE)

# cmake -E copy follows symlinks to get the final file, which is not what we
# want in this situation.  To avoid this, we create a copy script which uses
# file(COPY) and run that script with cmake -P
file(WRITE "${CMAKE_BINARY_DIR}/CMakeFiles/cp.cmake" "get_filename_component(DDIR \${DEST} DIRECTORY)\nfile(COPY \${SRC} DESTINATION \${DDIR})")

# When staging files in the build directory, we have to be aware of multiple
# configurations.  This is done post-ExternalProject build, at the parent build
# time, so it needs to be a custom command. Until add_custom_command outputs
# can use $<CONFIG>, we use the old-school CMAKE_CFG_INTDIR variable in the OUTPUT
# path.  This is key, because it allows the add_custom_command to be aware that
# it needs to execute the correct copy command for a current configuration at build
# time.
function(fcfgcpy outvar extproj root ofile dir tfile)
  string(REPLACE "${CMAKE_BINARY_DIR}/" "" rdir "${dir}")
  if (CMAKE_CONFIGURATION_TYPES)
    add_custom_command(
      OUTPUT "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${rdir}/${tfile}"
      COMMAND ${CMAKE_COMMAND} -DSRC="${root}/${rdir}/${ofile}" -DDEST="${CMAKE_BINARY_DIR}/$<CONFIG>/${rdir}/${tfile}" -P "${CMAKE_BINARY_DIR}/CMakeFiles/cp.cmake"
      DEPENDS ${extproj}
      )
    set(TOUT ${TOUT} "${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${rdir}/${tfile}")
  else (CMAKE_CONFIGURATION_TYPES)
    add_custom_command(
      OUTPUT "${CMAKE_BINARY_DIR}/${rdir}/${tfile}"
      COMMAND ${CMAKE_COMMAND} -DSRC="${root}/${rdir}/${ofile}" -DDEST="${CMAKE_BINARY_DIR}/${rdir}/${tfile}" -P "${CMAKE_BINARY_DIR}/CMakeFiles/cp.cmake"
      DEPENDS ${extproj}
      )
    set(TOUT ${TOUT} "${CMAKE_BINARY_DIR}/${rdir}/${tfile}")
  endif (CMAKE_CONFIGURATION_TYPES)
  set(${outvar} ${TOUT} PARENT_SCOPE)
endfunction(fcfgcpy file)

function(ExternalProject_ByProducts etarg extproj extroot dir)

  cmake_parse_arguments(E "FIXPATH;NOINSTALL" "" "" ${ARGN})

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

    unset(TOUT)
    fcfgcpy(TOUT ${extproj} ${extroot} ${bpf} "${dir}" ${bpf})
    set(ALL_TOUT ${ALL_TOUT} ${TOUT})

    if (NOT E_NOINSTALL)
      install(FILES "${CMAKE_BINARY_ROOT}/${dir}/${bpf}" DESTINATION "${dir}/")
      if (E_FIXPATH)
	# Note - proper quoting for install(CODE) is extremely important for CPack, see
	# https://stackoverflow.com/a/48487133
	#install(CODE "execute_process(COMMAND $<TARGET_FILE:buildpath_replace> \"\${CMAKE_INSTALL_PREFIX}/${dir}/${bpf}\")")

	# The proper way to do this is the line above with generator
	# expressions, but support for using them in INSTALL(CODE) wasn't added
	# until CMake 3.14.  The workaround is to manually specify he output
	# path and force the buildpath_replace target to output in a specific
	# location via its RUNTIME_OUTPUT_DIRECTORY properties.
	get_filename_component(EXE_EXT "${CMAKE_COMMAND}" EXT)
	install(CODE "execute_process(COMMAND ${CMAKE_BINARY_DIR}/CMakeTmp/buildpath_replace${EXE_EXT} \"\${CMAKE_INSTALL_PREFIX}/${dir}/${bpf}\")")

      endif (E_FIXPATH)
    endif (NOT E_NOINSTALL)

  endforeach (bpf ${E_UNPARSED_ARGUMENTS})

  if (ALL_TOUT)
    string(MD5 UKEY "${ALL_TOUT}")
    add_custom_target(${etarg}_${UKEY} ALL DEPENDS ${ALL_TOUT})
    add_dependencies(${etarg}_stage ${etarg}_${UKEY})
  endif (ALL_TOUT)

endfunction(ExternalProject_ByProducts)


function(ET_target_props etarg REL_DIR LINK_TARGET)

  cmake_parse_arguments(ET "SHARED" "LINK_TARGET_DEBUG" "" ${ARGN})

  if(NOT CMAKE_CONFIGURATION_TYPES)

    # Note: per https://stackoverflow.com/a/49390802 need to set
    # IMPORTED_NO_SONAME to get working linking for what we're trying to do
    # here.  Without that property set, build dir copies of libraries will use
    # an incorrect relative link and fail to find the library.
    set_target_properties(${etarg} PROPERTIES
      IMPORTED_NO_SONAME TRUE
      IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/${REL_DIR}/${LINK_TARGET}"
      IMPORTED_SONAME "${LINK_TARGET}"
      )

    # For Windows, IMPORTED_IMPLIB is important for shared libraries.
    # It is that property that will tell a toplevel target what to link against
    # when building - pointing out the dll isn't enough by itself.
    if(ET_SHARED AND MSVC)
      string(REPLACE "${CMAKE_SHARED_LIBRARY_SUFFIX}" ".lib" IMPLIB_FILE "${LINK_TARGET}")
      string(REPLACE "${SHARED_DIR}" "${LIB_DIR}" REL_DIR "${REL_DIR}")
      set_target_properties(${etarg} PROPERTIES
	IMPORTED_IMPLIB "${CMAKE_BINARY_DIR}/${REL_DIR}/${IMPLIB_FILE}"
	)
    endif(ET_SHARED AND MSVC)

  else(NOT CMAKE_CONFIGURATION_TYPES)

    # TODO - see if we need to set any DLL import/export information onto these
    # targets to replace information previously passed in the CMake build targets
    # on Windows...

    # If no config is set for multiconfig, default to Debug
    set_target_properties(${etarg} PROPERTIES
      IMPORTED_NO_SONAME TRUE
      IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/Debug/${REL_DIR}/${LINK_TARGET}"
      IMPORTED_SONAME "${LINK_TARGET}"
      )

    foreach(CFG_TYPE ${CMAKE_CONFIGURATION_TYPES})
      string(TOUPPER "${CFG_TYPE}" CFG_TYPE_UPPER)

      # The config variables need to be set in this mode.
      if("${CFG_TYPE_UPPER}" STREQUAL "DEBUG" AND ET_LINK_TARGET_DEBUG)
	set(C_LINK_TARGET ${ET_LINK_TARGET_DEBUG})
      else()
	set(C_LINK_TARGET ${LINK_TARGET})
      endif()

      # If we're multiconfig, define properties for each configuration
      set_target_properties(${etarg} PROPERTIES
	IMPORTED_CONFIGURATIONS "${CMAKE_CONFIGURATION_TYPES}"
	IMPORTED_NO_SONAME_${CFG_TYPE_UPPER} TRUE
	IMPORTED_LOCATION_${CFG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/${CFG_TYPE}/${REL_DIR}/${LINK_TARGET}"
	IMPORTED_SONAME_${CFG_TYPE_UPPER} "${LINK_TARGET}"
	)

      # For Windows, IMPORTED_IMPLIB is important for shared libraries.
      # It is that property that will tell a toplevel target what to link against
      # when building - pointing out the dll isn't enough by itself.
      if(ET_SHARED AND MSVC)
	string(REPLACE "${CMAKE_SHARED_LIBRARY_SUFFIX}" ".lib" IMPLIB_FILE "${LINK_TARGET}")
	string(REPLACE "${SHARED_DIR}" "${LIB_DIR}" REL_DIR "${REL_DIR}")
	set_target_properties(${etarg} PROPERTIES
	  IMPORTED_IMPLIB_${CFG_TYPE_UPPER} "${CMAKE_BINARY_DIR}/${CFG_TYPE}/${REL_DIR}/${IMPLIB_FILE}"
	  )
      endif(ET_SHARED AND MSVC)

    endforeach(CFG_TYPE ${CMAKE_CONFIGURATION_TYPES})

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
#
# When specifying OFILE, the path should be given relative to the root directory:
# For example:
#
# /usr/lib/libfoo.so -> lib/libfoo.so
# /usr/lib/bar-1.0/libbar.so -> lib/foo-1.0/libfoo.so
# /usr/bin/baz -> bin/baz
# /usr/bin/mypkg/baz -> bin/mypkg/baz
#
function(ET_RPath OFILE)
  get_filename_component(OFPATH "${OFILE}" DIRECTORY)
  get_filename_component(RRPATH "${CMAKE_INSTALL_PREFIX}/${OFPATH}" REALPATH)
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
  # Note - proper quoting for install(CODE) is extremely important for CPack, see
  # https://stackoverflow.com/a/48487133
  install(CODE "
  file(RPATH_CHANGE
    FILE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${OFILE}\"
    OLD_RPATH \"${CMAKE_BUILD_RPATH}\"
    NEW_RPATH \"${NEW_RPATH}\")
  ")
endfunction(ET_RPath)


# etype = EXEC;SHARED;STATIC
#
function(ExternalProject_Target etype etarg extproj extroot fname)

  cmake_parse_arguments(E "RPATH" "LINK_TARGET;LINK_TARGET_DEBUG;SUBDIR" "SYMLINKS" ${ARGN})

  #message("etype: ${etype}")
  #message("etarg: ${etarg}")
  #message("extproj: ${extproj}")
  #message("extroot: ${extroot}")
  #message("fname: ${fname}")

  # If we have a static target but BUILD_STATIC_LIBS is off, we're done
  if ("${etype}" STREQUAL "STATIC" AND NOT BUILD_STATIC_LIBS)
    return()
  endif ("${etype}" STREQUAL "STATIC" AND NOT BUILD_STATIC_LIBS)

  # If we have something that's not a target, fatal error - this is a problem with the build logic
  if(NOT TARGET ${extproj})
    message(FATAL_ERROR "${extprog} is not a target")
  endif(NOT TARGET ${extproj})

  # Protect against redefinition of already defined targets - each etarg name should be unique, corresponding
  # to a single executable or library from the foreign build.
  if(TARGET ${etarg})
    message(FATAL_ERROR "Target ${etarg} is already defined\n")
  endif(TARGET ${etarg})

  # For some platforms, we may want to target a symbolic link rather than the
  # target itself for linking purposes.  We also (grr) may need to target a
  # configuration specific link.  Go through the permutations and figure out
  # what LINK_TARGET should be in this case.
  unset(LINK_TARGET)
  unset(LINK_TARGET_DEBUG)

  if ("${etype}" STREQUAL "SHARED")

    if (E_LINK_TARGET)
      set(LINK_TARGET "${E_LINK_TARGET}")
    endif (E_LINK_TARGET)
    if (NOT LINK_TARGET)
      # In case we have a relative path for fname, make sure we're using just
      # the filename for linking purposes.
      get_filename_component(LFILE "${fname}" NAME)
      set(LINK_TARGET "${LFILE}")
    endif (NOT LINK_TARGET)

    if (E_LINK_TARGET_DEBUG)
      set(LINK_TARGET_DEBUG "${E_LINK_TARGET_DEBUG}")
    else (LINK_TARGET)
      set(LINK_TARGET_DEBUG "${LINK_TARGET}")
    endif ()

  endif ("${etype}" STREQUAL "SHARED")

  # Create imported target - need to both make the target itself
  # and set the necessary properties.  See also
  # https://gitlab.kitware.com/cmake/community/wikis/doc/tutorials/Exporting-and-Importing-Targets

  # Because the outputs are not properly build target outputs of the primary
  # CMake project, we need to install as either FILES or PROGRAMS.  Collect
  # the file copying targets in a variable for later use.
  set(TOUT)

  # Shared library logic
  if ("${etype}" STREQUAL "SHARED")

    add_library(${etarg} SHARED IMPORTED GLOBAL)

    if (MSVC)
      set(SHARED_DIR ${BIN_DIR})
    else (MSVC)
      set(SHARED_DIR ${LIB_DIR})
    endif (MSVC)

    fcfgcpy(TOUT ${extproj} ${extroot} ${fname} ${SHARED_DIR} ${fname})

    # On Windows, we need both a .dll and a .lib file to use a shared library for compilation
    if (MSVC)
      string(REPLACE "${CMAKE_SHARED_LIBRARY_SUFFIX}" ".lib" IMPLIB_FILE "${fname}")
      fcfgcpy(TOUT ${extproj} ${extroot} ${IMPLIB_FILE} ${LIB_DIR} ${IMPLIB_FILE})
    endif (MSVC)

    # Because we're using LINK_TARGET here rather than fname, we need to take any relative
    # directories specified in fname and ppend them to SHARED_DIR. (For other cases we are
    # just feeding fname directly, so there are no such concerns.  TODO - should we just
    # require relative dirs on all the symlinks instead?
    get_filename_component(LDIR "${fname}" DIRECTORY)
    ET_target_props(${etarg} "${SHARED_DIR}/${LDIR}" ${LINK_TARGET} SHARED LINK_TARGET_DEBUG "${LINK_TARGET_DEBUG}")

    install(FILES "${CMAKE_BINARY_ROOT}/${SHARED_DIR}/${fname}" DESTINATION ${SHARED_DIR}/${E_SUBDIR})
    if (MSVC)
      install(FILES "${CMAKE_BINARY_ROOT}/${LIB_DIR}/${IMPLIB_FILE}" DESTINATION ${LIB_DIR}/${E_SUBDIR})
    endif (MSVC)

    # Let CMake know there is a target dependency here, despite this being an import target
    add_dependencies(${etarg} ${extproj})

    # additional (non-MSVC) work specific to shared libraries:
    if (NOT MSVC)

      # Perform RPath magic
      if (E_RPATH)
	ET_RPath("${SHARED_DIR}/${fname}")
      endif (E_RPATH)

      # Add install rules for any symlinks the caller has listed
      foreach(slink ${E_SYMLINKS})
	fcfgcpy(TOUT ${extproj} ${extroot} ${slink} ${SHARED_DIR} ${slink})
	install(FILES "${CMAKE_BINARY_ROOT}/${SHARED_DIR}/${slink}" DESTINATION ${SHARED_DIR}/${E_SUBDIR})
      endforeach(slink ${E_SYMLINKS})

    endif (NOT MSVC)

  endif ("${etype}" STREQUAL "SHARED")

  # Static library logic
  if ("${etype}" STREQUAL "STATIC" AND BUILD_STATIC_LIBS)

    add_library(${etarg} STATIC IMPORTED GLOBAL)

    fcfgcpy(TOUT ${extproj} ${extroot} ${fname} ${LIB_DIR} ${fname})

    ET_target_props(${etarg} "${LIB_DIR}" ${fname} STATIC LINK_TARGET_DEBUG "${LINK_TARGET_DEBUG}")
    install(FILES "${CMAKE_BINARY_ROOT}/${LIB_DIR}/${fname}" DESTINATION ${LIB_DIR}/${E_SUBDIR})

    # Let CMake know there is a target dependency here, despite this being an import target
    add_dependencies(${etarg} ${extproj})

    # additional (non-MSVC) work specific to shared libraries:
    if (NOT MSVC)

      # Add install rules for any symlinks the caller has listed
      foreach(slink ${E_SYMLINKS})
	fcfgcpy(TOUT ${extproj} ${extroot} ${slink} ${LIB_DIR} ${slink})
	install(FILES "${CMAKE_BINARY_ROOT}/${LIB_DIR}/${slink}" DESTINATION ${LIB_DIR})
      endforeach(slink ${E_SYMLINKS})

    endif (NOT MSVC)

  endif ("${etype}" STREQUAL "STATIC" AND BUILD_STATIC_LIBS)

  # Executable logic
  if ("${etype}" STREQUAL "EXEC")

    add_executable(${etarg} IMPORTED GLOBAL)

    fcfgcpy(TOUT ${extproj} ${extroot} ${fname} ${BIN_DIR} ${fname})

    ET_target_props(${etarg} "${BIN_DIR}" ${fname})

    install(PROGRAMS "${CMAKE_BINARY_ROOT}/${BIN_DIR}/${fname}" DESTINATION ${BIN_DIR}/${E_SUBDIR})

    # Let CMake know there is a target dependency here, despite this being an import target
    add_dependencies(${etarg} ${extproj})

    # additional (non-MSVC) work specific to shared libraries:
    if (NOT MSVC)

      # Perform RPath magic
      if (E_RPATH)
	ET_RPath("${BIN_DIR}/${fname}")
      endif (E_RPATH)

    endif (NOT MSVC)

  endif ("${etype}" STREQUAL "EXEC")

  # Set up the staging targets - these copy the files we are interested in from the
  # 3rd party install directory to BRL-CAD's build tree.  This allows us to achieve
  # file level dependencies despite using ExternalProject_Add, since each output
  # file uses a custom command to do the copy that depends on the external target
  # and the staging target depends on all of those output files individually.  In
  # essence, those copy commands provide our CMake system with the explicit file
  # information we could not otherwise get from ExternalProject_Add.
  if (TOUT)
    if (NOT TARGET ${etarg}_stage)
      add_custom_target(${etarg}_stage ALL DEPENDS ${TOUT})
    else (NOT TARGET ${etarg}_stage)
      add_dependencies(${etarg}_stage ${TOUT})
    endif (NOT TARGET ${etarg}_stage)
  endif (TOUT)

  # Set up dependencies for the imported target that will be used in *_LIBRARY variables
  # to depend on the staging targets.  This completes the dependency chain, and targets
  # depending on the *_LIBRARY variables will now depend on the correct execution of
  # the ExternalProject_Add system.
  add_dependencies(${etarg} ${etarg}_stage)

endfunction(ExternalProject_Target)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8

