set(utahrle_DESCRIPTION "
Option for enabling and disabling compilation of the Utah Raster
Toolkit library provided with BRL-CAD's source code.  Default is AUTO,
responsive to the toplevel BRLCAD_BUNDLED_LIBS option and testing
first for a system version if BRLCAD_BUNDLED_LIBS is also AUTO.
")
THIRD_PARTY(libutahrle UTAHRLE utahrle utahrle_DESCRIPTION REQUIRED_VARS BRLCAD_LEVEL3 ALIASES ENABLE_UTAHRLE FLAGS NOSYS)

if (BRLCAD_UTAHRLE_BUILD)

  set(UTAHRLE_MAJOR_VERSION 19)
  set(UTAHRLE_MINOR_VERSION 0)
  set(UTAHRLE_PATCH_VERSION 1)
  set(UTAHRLE_VERSION ${UTAHRLE_MAJOR_VERSION}.${UTAHRLE_MINOR_VERSION}.${UTAHRLE_PATCH_VERSION})

  if (MSVC)
    set(UTAHRLE_BASENAME utahrle)
    set(UTAHRLE_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
  else (MSVC)
    set(UTAHRLE_BASENAME libutahrle)
    set(UTAHRLE_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX}.${UTAHRLE_VERSION})
  endif (MSVC)


  ExternalProject_Add(UTAHRLE_BLD
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../other/libutahrle"
    BUILD_ALWAYS ${EXTERNAL_BUILD_UPDATE} ${LOG_OPTS}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DLIB_DIR=${LIB_DIR} -DBIN_DIR=${BIN_DIR}
               -DCMAKE_INSTALL_RPATH=${CMAKE_BUILD_RPATH} -DBUILD_STATIC_LIBS=${BUILD_STATIC_LIBS}
    )

  # Tell the parent build about files and libraries
  file(APPEND "${SUPERBUILD_OUT}" "
  ExternalProject_Target(utahrle UTAHRLE_BLD
    OUTPUT_FILE ${UTAHRLE_BASENAME}${UTAHRLE_SUFFIX}
    STATIC_OUTPUT_FILE ${UTAHRLE_BASENAME}${CMAKE_STATIC_LIBRARY_SUFFIX}
    SYMLINKS \"${UTAHRLE_BASENAME}${CMAKE_SHARED_LIBRARY_SUFFIX};${UTAHRLE_BASENAME}${CMAKE_SHARED_LIBRARY_SUFFIX}.${UTAHRLE_MAJOR_VERSION}\"
    LINK_TARGET \"${UTAHRLE_BASENAME}${CMAKE_SHARED_LIBRARY_SUFFIX}\"
    STATIC_LINK_TARGET \"${UTAHRLE_BASENAME}${CMAKE_STATIC_LIBRARY_SUFFIX}\"
    RPATH
    )
  ExternalProject_ByProducts(UTAHRLE_BLD ${INCLUDE_DIR}
    rle.h
    rle_code.h
    rle_config.h
    rle_put.h
    rle_raw.h
    )
  \n")

  list(APPEND BRLCAD_DEPS UTAHRLE_BLD)

  set(UTAHRLE_LIBRARIES utahrle CACHE STRING "Building bundled libutahrle" FORCE)
  set(UTAHRLE_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/${INCLUDE_DIR}" CACHE STRING "Directory containing utahrle headers." FORCE)

  SetTargetFolder(UTAHRLE_BLD "Third Party Libraries")
  SetTargetFolder(utahrle "Third Party Libraries")

endif (BRLCAD_UTAHRLE_BUILD)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8

