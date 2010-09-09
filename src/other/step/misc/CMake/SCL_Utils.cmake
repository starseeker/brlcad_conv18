MACRO(SCL_ADDEXEC execname srcs libs)
  STRING(REGEX REPLACE " " ";" srcslist "${srcs}")
  STRING(REGEX REPLACE " " ";" libslist "${libs}")
  add_executable(${execname} ${srcslist})
  target_link_libraries(${execname} ${libslist})
  INSTALL(TARGETS ${execname} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
ENDMACRO(SCL_ADDEXEC execname srcs libs)

MACRO(SCL_ADDLIB libname srcs libs)
  STRING(REGEX REPLACE " " ";" srcslist "${srcs}")
  STRING(REGEX REPLACE " " ";" libslist "${libs}")
  IF(BUILD_SHARED_LIBS)
	  add_library(${libname} SHARED ${srcslist})
	  if(NOT ${libs} MATCHES "NONE")
		  target_link_libraries(${libname} ${libslist})
	  endif(NOT ${libs} MATCHES "NONE")
	  INSTALL(TARGETS ${libname} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
  ENDIF(BUILD_SHARED_LIBS)
  IF(BUILD_STATIC_LIBS)
	  add_library(${libname}-static STATIC ${srcslist})
	  if(NOT ${libs} MATCHES "NONE")
		  target_link_libraries(${libname}-static ${libslist})
	  endif(NOT ${libs} MATCHES "NONE")
	  SET_TARGET_PROPERTIES(${libname}-static PROPERTIES OUTPUT_NAME "${libname}")
	  IF(WIN32)
		  # We need the lib prefix on win32, so add it even if our add_library
		  # wrapper function has removed it due to the target name - see
		  # http://www.cmake.org/Wiki/CMake_FAQ#How_do_I_make_my_shared_and_static_libraries_have_the_same_root_name.2C_but_different_suffixes.3F
		  SET_TARGET_PROPERTIES(${libname}-static PROPERTIES PREFIX "lib")
	  ENDIF(WIN32)
	  INSTALL(TARGETS ${libname}-static ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
  ENDIF(BUILD_STATIC_LIBS)
ENDMACRO(SCL_ADDLIB libname srcs libs)
