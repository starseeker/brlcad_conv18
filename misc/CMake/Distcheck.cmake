# Distribution checking is the process of verifying that a source repository
# is in a valid, working state.  This file defines the top level macros that
# set up and control the process.  The distcheck targets need
# to have values substituted prior to inclusion in order to produce a command
# that will execute in parallel when using Make, so the distcheck target template
# is set up as a cmake.in file - configure_file and include are used to generate
# and source the final target definition for each case.

if(NOT BRLCAD_IS_SUBBUILD)
  if("${CMAKE_VERBOSE_DISTCHECK}" STREQUAL "")
    set(CMAKE_VERBOSE_DISTCHECK OFF)
  endif("${CMAKE_VERBOSE_DISTCHECK}" STREQUAL "")

  find_program(CPACK_EXEC cpack)
  mark_as_advanced(CPACK_EXEC)

  # We'll always want the repo and source distcheck targets defined - they are contants
  # for distcheck regardless of the bulid configurations used.

  # Set up the script that will be used to verify the source archives
  configure_file(${BRLCAD_CMAKE_DIR}/distcheck_repo_verify.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp/distcheck_repo_verify.cmake @ONLY)

  # Define the repository verification build target
  add_custom_target(distcheck-repo-verify
    COMMAND ${CMAKE_COMMAND} -E echo "*** Check files in Source Repository against files specified in Build Logic ***"
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp/distcheck_repo_verify.cmake)

  # Define the source archive creation target - should only happen once repository is verified
  add_custom_target(distcheck-source-archives
    COMMAND ${CMAKE_COMMAND} -E echo "*** Create source tgz, tbz2 and zip archives from toplevel archive ***"
    COMMAND ${CPACK_EXEC} --config ${CMAKE_CURRENT_BINARY_DIR}/CPackSourceConfig.cmake
    DEPENDS distcheck-repo-verify)

  # Utility macro for defining individual distcheck targets
  macro(CREATE_DISTCHECK TARGET_SUFFIX CMAKE_OPTS)
    if(NOT "${ARGV2}" STREQUAL "")
      message("Defing target using custom template ${BRLCAD_CMAKE_DIR}/${ARGV2}")
      set(distcheck_template_file "${BRLCAD_CMAKE_DIR}/${ARGV2}")
    else(NOT "${ARGV2}" STREQUAL "")
      set(distcheck_template_file "${BRLCAD_CMAKE_DIR}/distcheck_target.cmake.in")
    endif(NOT "${ARGV2}" STREQUAL "")
    # If we've already got a particular distcheck target, don't try to create it again.
    get_target_property(not_in_all distcheck-${TARGET_SUFFIX} EXCLUDE_FROM_ALL)
    if(NOT not_in_all)
      # Need to set these locally so configure_file will pick them up...
      SET(TARGET_SUFFIX ${TARGET_SUFFIX})
      SET(CMAKE_OPTS ${CMAKE_OPTS})

      # Determine how to trigger the build in the distcheck target
      if(NOT "${ARGV3}" STREQUAL "")
	set(DISTCHECK_BUILD_CMD "${ARGV3}")
      else(NOT "${ARGV3}" STREQUAL "")
      if("${CMAKE_GENERATOR}" MATCHES "Make")
	if(NOT CMAKE_VERBOSE_DISTCHECK)
	  set(TARGET_REDIRECT " >> distcheck-${TARGET_SUFFIX}.log ")
	  DISTCLEAN(${CMAKE_CURRENT_BINARY_DIR}/distcheck-${TARGET_SUFFIX}.log)
	endif(NOT CMAKE_VERBOSE_DISTCHECK)
	set(DISTCHECK_BUILD_CMD "${CMAKE_COMMAND} -E chdir distcheck-${TARGET_SUFFIX}/build $(MAKE) ${TARGET_REDIRECT}")
      else("${CMAKE_GENERATOR}" MATCHES "Make")
	set(DISTCHECK_BUILD_CMD "COMMAND ${CMAKE_COMMAND} -E build distcheck-${TARGET_SUFFIX}/build")
	set(TARGET_REDIRECT "")
      endif("${CMAKE_GENERATOR}" MATCHES "Make")
      endif(NOT "${ARGV3}" STREQUAL "")

      # Based on the build command, generate a distcheck target definition from the template
      configure_file(${distcheck_template_file} ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp/distcheck_target_${TARGET_SUFFIX}.cmake @ONLY)
      include(${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp/distcheck_target_${TARGET_SUFFIX}.cmake)

      # Keep track of the distcheck targets
      set(distcheck_targets ${distcheck_targets} distcheck-${TARGET_SUFFIX})
    else(NOT not_in_all)
      message(WARNING "Distcheck target distcheck-${TARGET_SUFFIX} already defined, skipping...")
    endif(NOT not_in_all)
  endmacro(CREATE_DISTCHECK)

  # Top level macro for defining the common "standard" cases and lets a CMake build select either
  # the standard Debug + Release or all defined distchecks as the build that is triggered by the
  # distcheck target - pass either STD or FULL as an argument to select one or the other.  Default
  # if not argument is supplied is STD.
  macro(DEFINE_DISTCHECK_TARGET)
    if(${ARGC} EQUAL 1)
      string(TOUPPER ${ARGV0} ARGV0_UPPER)
      if(${ARGV0_UPPER} STREQUAL "STD" OR ${ARGV0_UPPER} STREQUAL "STANDARD")
	set(full_distcheck 0)
      endif(${ARGV0_UPPER} STREQUAL "STD" OR ${ARGV0_UPPER} STREQUAL "STANDARD")
      if(${ARGV0_UPPER} STREQUAL "FULL")
	set(full_distcheck 1)
      endif(${ARGV0_UPPER} STREQUAL "FULL")
    endif(${ARGC} EQUAL 1)

    CREATE_DISTCHECK(debug   "-DCMAKE_BUILD_TYPE=Debug -DBRLCAD_BUNDLED_LIBS=BUNDLED")
    CREATE_DISTCHECK(release "-DCMAKE_BUILD_TYPE=Release -DBRLCAD_BUNDLED_LIBS=BUNDLED")

    add_custom_target(distcheck-std
      # The source repository verification script is responsible for generating these files
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp/distcheck_remove_archives_if_invalid
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp/distcheck_message
      DEPENDS distcheck-debug distcheck-release)

    add_custom_target(distcheck-full
      # The source repository verification script is responsible for generating these files
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp/distcheck_remove_archives_if_invalid
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/CMakeTmp/distcheck_message
      DEPENDS ${distcheck_targets})

    if(full_distcheck)
      add_custom_target(distcheck DEPENDS distcheck-full)
    else(full_distcheck)
      add_custom_target(distcheck DEPENDS distcheck-std)
    endif(full_distcheck)
  endmacro(DEFINE_DISTCHECK_TARGET)
endif(NOT BRLCAD_IS_SUBBUILD)
