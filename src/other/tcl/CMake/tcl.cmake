# CMake version of the tcl.m4 logic, insofar as it maps to CMake and has been
# needed.

INCLUDE(CheckCCompilerFlag)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckTypeSize)
INCLUDE(CheckLibraryExists)
INCLUDE(CheckStructHasMember)
INCLUDE(CheckCSourceCompiles)
INCLUDE(ResolveCompilerPaths)
INCLUDE(CheckPrototypeExists)
INCLUDE(CheckCSourceRuns)
INCLUDE(CheckCFileRuns)

INCLUDE(ac_std_funcs)

MACRO(ADD_TCL_CFLAG TCL_CFLAG)
	SET(TCL_CFLAGS "${TCL_CFLAGS} -D${TCL_CFLAG}=1" CACHE STRING "TCL CFLAGS" FORCE)
ENDMACRO(ADD_TCL_CFLAG)

# Note - for these path and load functions, should move the FindTCL.cmake
# logic that applies to here

#------------------------------------------------------------------------
# SC_PATH_TCLCONFIG
#------------------------------------------------------------------------

# TODO

#------------------------------------------------------------------------
# SC_PATH_TKCONFIG
#------------------------------------------------------------------------

# TODO

#------------------------------------------------------------------------
# SC_LOAD_TCLCONFIG
#------------------------------------------------------------------------

# TODO

#------------------------------------------------------------------------
# SC_LOAD_TKCONFIG
#------------------------------------------------------------------------

# TODO

#------------------------------------------------------------------------
# SC_PROG_TCLSH
#------------------------------------------------------------------------

# TODO

#------------------------------------------------------------------------
# SC_BUILD_TCLSH
#------------------------------------------------------------------------

# TODO

#------------------------------------------------------------------------
# SC_ENABLE_SHARED
#------------------------------------------------------------------------

# This will probably be handled by CMake variables rather than a
# specific SC command

#------------------------------------------------------------------------
# SC_ENABLE_FRAMEWORK
#------------------------------------------------------------------------

# Not immediately clear how this will work in CMake


#------------------------------------------------------------------------
# SC_ENABLE_THREADS
#------------------------------------------------------------------------
MACRO(SC_ENABLE_THREADS)
	OPTION(TCL_THREADS "Enable Tcl Thread support" ON)
	IF(TCL_THREADS)
		ADD_TCL_CFLAG(TCL_THREADS)
		ADD_TCL_CFLAG(USE_THREAD_ALLOC)
		ADD_TCL_CFLAG(_REENTRANT)
		ADD_TCL_CFLAG(_THREAD_SAFE)
		IF(${CMAKE_SYSTEM_NAME} MATCHES "^SunOS$")
			ADD_TCL_CFLAG(_POSIX_PTHREAD_SEMANTICS)
		ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "^SunOS$")
		FIND_PACKAGE(Threads)
		CHECK_FUNCTION_EXISTS(pthread_attr_setstacksize HAVE_PTHREAD_ATTR_SETSTACKSIZE)
		IF(HAVE_PTHREAD_ATTR_SETSTACKSIZE)
			ADD_TCL_CFLAG(HAVE_PTHREAD_ATTR_SETSTACKSIZE)
		ENDIF(HAVE_PTHREAD_ATTR_SETSTACKSIZE)
		CHECK_FUNCTION_EXISTS(pthread_attr_get_np HAVE_PTHREAD_ATTR_GET_NP)
		CHECK_FUNCTION_EXISTS(pthread_getattr_np HAVE_PTHREAD_GETATTR_NP)
		IF(HAVE_PTHREAD_ATTR_GET_NP)
			ADD_TCL_CFLAG(HAVE_PTHREAD_ATTR_GET_NP)
		ELSEIF(HAVE_PTHREAD_GETATTR_NP)
			ADD_TCL_CFLAG(HAVE_PTHREAD_GETATTR_NP)
		ENDIF(HAVE_PTHREAD_ATTR_GET_NP)
		IF(NOT HAVE_PTHREAD_ATTR_GET_NP AND NOT HAVE_PTHREAD_GETATTR_NP)
			CHECK_FUNCTION_EXISTS(pthread_get_stacksize_np HAVE_PTHREAD_GET_STACKSIZE_NP)
			IF(HAVE_PTHREAD_GET_STACKSIZE_NP)
				ADD_TCL_CFLAG(HAVE_PTHREAD_GET_STACKSIZE_NP)
			ENDIF(HAVE_PTHREAD_GET_STACKSIZE_NP)
		ENDIF(NOT HAVE_PTHREAD_ATTR_GET_NP AND NOT HAVE_PTHREAD_GETATTR_NP)
	ENDIF(TCL_THREADS)
ENDMACRO(SC_ENABLE_THREADS)

#------------------------------------------------------------------------
# SC_ENABLE_SYMBOLS
#------------------------------------------------------------------------

# TODO - this may be replaced by other CMake mechanisms

#------------------------------------------------------------------------
# SC_ENABLE_LANGINFO
#------------------------------------------------------------------------
MACRO(SC_ENABLE_LANGINFO)
	OPTION(ENABLE_LANGINFO "Trigger use of nl_langinfo if available." ON)
	IF(ENABLE_LANGINFO)
		CHECK_INCLUDE_FILE(langinfo.h HAVE_LANGINFO)
		IF(HAVE_LANGINFO)
			SET(LANGINFO_SRC "
			#include <langinfo.h>
			int main() {
			nl_langinfo(CODESET);
			}
			")
			CHECK_C_SOURCE_COMPILES("${LANGINFO_SRC}" LANGINFO_COMPILES)
			IF(LANGINFO_COMPILES)
				ADD_TCL_FLAG(HAVE_LANGINFO)
			ELSE(LANGINFO_COMPILES)
				SET(ENABLE_LANGINFO OFF CACHE STRING "Langinfo off" FORCE)
			ENDIF(LANGINFO_COMPILES)
		ELSE(LANGINFO)
			SET(ENABLE_LANGINFO OFF CACHE STRING "Langinfo off" FORCE)
		ENDIF(LANGINFO)
	ENDIF(ENABLE_LANGINFO)
ENDMACRO(SC_ENABLE_LANGINFO)

#--------------------------------------------------------------------
# SC_CONFIG_MANPAGES
#--------------------------------------------------------------------

# TODO

#--------------------------------------------------------------------
# SC_CONFIG_SYSTEM
#--------------------------------------------------------------------

# Replaced by CMake functionality


#--------------------------------------------------------------------
# SC_CONFIG_CFLAGS
#--------------------------------------------------------------------

# TODO - many of these are either automatically handled or handled
# elsewhere, but should still handle what we need to

#--------------------------------------------------------------------
# SC_SERIAL_PORT
#--------------------------------------------------------------------
MACRO(SC_SERIAL_PORT)
	SET(TERMIOS_SRC_1 "
#include <termios.h>
int main() {
struct termios t;
if (tcgetattr(0, &t) == 0) {
	cfsetospeed(&t, 0);
	t.c_cflag |= PARENB | PARODD | CSIZE | CSTOPB;
	return 0;
}
	return 1;
}
	")
	SET(TERMIOS_SRC_2 "
#include <termios.h>
#include <errno.h>

int main() {
struct termios t;
if (tcgetattr(0, &t) == 0
	|| errno == ENOTTY || errno == ENXIO || errno == EINVAL) {
	cfsetospeed(&t, 0);
	t.c_cflag |= PARENB | PARODD | CSIZE | CSTOPB;
	return 0;
	}
	return 1;
}
	")
	SET(TERMIO_SRC_1 "
#include <termio.h>
int main() {
struct termio t;
if (ioctl(0, TCGETA, &t) == 0) {
	t.c_cflag |= CBAUD | PARENB | PARODD | CSIZE | CSTOPB;
	return 0;
}
	return 1;
}
   ")
	SET(TERMIO_SRC_2 "
#include <termio.h>
#include <errno.h>

int main() {
struct termio t;
if (ioctl(0, TCGETA, &t) == 0
	|| errno == ENOTTY || errno == ENXIO || errno == EINVAL) {
	t.c_cflag |= CBAUD | PARENB | PARODD | CSIZE | CSTOPB;
	return 0;
	}
	return 1;
}
	")
	SET(SGTTY_SRC_1 "
#include <sgtty.h>
int main() {
struct sgttyb t;
if (ioctl(0, TIOCGETP, &t) == 0) {
	t.sg_ospeed = 0;
	t.sg_flags |= ODDP | EVENP | RAW;
	return 0;
}
	return 1;
}
   ")
	SET(SGTTY_SRC_2 "
#include <sgtty.h>
#include <errno.h>

int main() {
struct sgttyb t;
if (ioctl(0, TIOCGETP, &t) == 0
	|| errno == ENOTTY || errno == ENXIO || errno == EINVAL) {
	t.sg_ospeed = 0;
	t.sg_flags |= ODDP | EVENP | RAW;
	return 0;
	}
	return 1;
}
	")
	CHECK_C_SOURCE_RUNS("${TERMIOS_SRC_1}" HAVE_TERMIOS)
	IF(NOT HAVE_TERMIOS)
		CHECK_C_SOURCE_RUNS("${TERMIO_SRC_1}" HAVE_TERMIO)
	ENDIF(NOT HAVE_TERMIOS)
	IF(NOT HAVE_TERMIO AND NOT HAVE_TERMIOS)
		CHECK_C_SOURCE_RUNS("${SGTTY_SRC_1}" HAVE_SGTTY)
	ENDIF(NOT HAVE_TERMIO AND NOT HAVE_TERMIOS)
	IF(NOT HAVE_TERMIO AND NOT HAVE_TERMIOS AND NOT HAVE_SGTTY)
		CHECK_C_SOURCE_RUNS("${TERMIOS_SRC_2}" HAVE_TERMIOS)
		IF(NOT HAVE_TERMIOS)
			CHECK_C_SOURCE_RUNS("${TERMIO_SRC_2}" HAVE_TERMIO)
		ENDIF(NOT HAVE_TERMIOS)
		IF(NOT HAVE_TERMIO AND NOT HAVE_TERMIOS)
			CHECK_C_SOURCE_RUNS("${SGTTY_SRC_2}" HAVE_SGTTY)
		ENDIF(NOT HAVE_TERMIO AND NOT HAVE_TERMIOS)
	ENDIF(NOT HAVE_TERMIO AND NOT HAVE_TERMIOS AND NOT HAVE_SGTTY)

	IF(HAVE_TERMIOS)
		ADD_TCL_CFLAG(USE_TERMIOS)
	ELSE(HAVE_TERMIOS)
		IF(HAVE_TERMIO)
		   ADD_TCL_CFLAG(USE_TERMIO)
		ELSE(HAVE_TERMIO)
			IF(HAVE_SGTTY)
				ADD_TCL_CFLAG(USE_SGTTY)
			ENDIF(HAVE_SGTTY)
		ENDIF(HAVE_TERMIO)
	ENDIF(HAVE_TERMIOS)
ENDMACRO(SC_SERIAL_PORT)


#--------------------------------------------------------------------
# SC_MISSING_POSIX_HEADERS
#--------------------------------------------------------------------
MACRO(SC_MISSING_POSIX_HEADERS)
	CHECK_INCLUDE_FILE_D(dirent.h HAVE_DIRENT_H)
	IF(NOT HAVE_DIRENT_H)
		SET(TCL_CFLAGS "${TCL_CFLAGS} -DNO_DIRENT_H=1")
	ENDIF(NOT HAVE_DIRENT_H)
	CHECK_INCLUDE_FILE_USABILITY_D(float.h FLOAT_H)
	CHECK_INCLUDE_FILE_USABILITY_D(values.h VALUES_H)
	CHECK_INCLUDE_FILE_USABILITY_D(limits.h LIMITS_H)
	CHECK_INCLUDE_FILE_D(stdlib.h HAVE_STDLIB_H)
	CHECK_INCLUDE_FILE_D(string.h HAVE_STRING_H)
	IF(NOT HAVE_STRING_H)
		SET(TCL_CFLAGS "${TCL_CFLAGS} -DNO_STRING_H=1")
	ENDIF(NOT HAVE_STRING_H)
	CHECK_INCLUDE_FILE_USABILITY_D(sys/wait.h SYS_WAIT_H)
	CHECK_INCLUDE_FILE_USABILITY_D(dlfcn.h DLFCN_H)
	CHECK_INCLUDE_FILE_USABILITY_D(sys/param.h SYS_PARAM_H)
ENDMACRO(SC_MISSING_POSIX_HEADERS)


#--------------------------------------------------------------------
# SC_PATH_X
#--------------------------------------------------------------------

# Replaced by CMake's FindX11

#--------------------------------------------------------------------
# SC_BLOCKING_STYLE
#--------------------------------------------------------------------

# TODO

#--------------------------------------------------------------------
# SC_TIME_HANLDER
#
# The TEA version of this macro calls AC_HEADER_TIME, but Autotools
# docs list it as obsolete.
#
# TODO - tzname testing is incomplete
#
#--------------------------------------------------------------------
MACRO(SC_TIME_HANDLER)
	CHECK_INCLUDE_FILE_USABILITY_D(sys/time.h HAVE_SYS_TIME_H)
	CHECK_STRUCT_HAS_MEMBER_D("struct tm" tm_zone time.h HAVE_STRUCT_TM_TM_ZONE)
	IF(HAVE_STRUCT_TM_TM_ZONE)
		ADD_TCL_CFLAG(HAVE_TM_ZONE)
	ELSE(HAVE_STRUCT_TM_TM_ZONE)
		SET(TZNAME_SRC "
#include <time.h>
int main () {
#ifndef tzname
  (void) tzname;
#endif
return 0;
}")
      CHECK_C_SOURCE_COMPILES("${TZNAME_SRC}" HAVE_TZNAME)
		IF(HAVE_TZNAME)
			ADD_TCL_CFLAG(HAVE_DECL_TZNAME)
		ENDIF(HAVE_TZNAME)
	ENDIF(HAVE_STRUCT_TM_TM_ZONE)
	CHECK_FUNCTION_EXISTS_D(gmtime_r HAVE_GMTIME_R)
	CHECK_FUNCTION_EXISTS_D(localtime_r HAVE_LOCALTIME_R)
	CHECK_FUNCTION_EXISTS_D(mktime HAVE_MKTIME)
	CHECK_STRUCT_HAS_MEMBER_D("struct tm" tm_tzadj time.h HAVE_TM_TZADJ)
	CHECK_STRUCT_HAS_MEMBER_D("struct tm" tm_gmtoff time.h HAVE_TM_GMTOFF)
ENDMACRO(SC_TIME_HANDLER)



MACRO(CHECK_FD_SET_IN_TYPES_D)
	SET(TEST_SRC "
	#include <sys/types.h>
	int main ()
	{
	fd_set readMask, writeMask;
	return 0;
	}
	")
	CHECK_C_SOURCE_COMPILES("${TEST_SRC}" FD_SET_IN_TYPES_H)
ENDMACRO(CHECK_FD_SET_IN_TYPES_D)

MACRO(CHECK_COMPILER_SUPPORTS_HIDDEN_D)
	SET(TEST_SRC"
	#define MODULE_SCOPE extern __attribute__((__visibility__("hidden")))
	main(){};
	")
	CHECK_C_SOURCE_COMPILES("${TEST_SRC}" COMPILER_SUPPORTS_HIDDEN)
ENDMACRO(CHECK_COMPILER_SUPPORTS_HIDDEN_D)

MACRO(CHECK_GETADDERINFO_WORKING_D)
	SET(GETADDERINFO_SRC "
	#include <netdb.h>
	int main () {
	const char *name, *port;
	struct addrinfo *aiPtr, hints;
	(void)getaddrinfo(name,port, &hints, &aiPtr);
	(void)freeaddrinfo(aiPtr);
	return 0;
	}")
	CHECK_C_SOURCE_COMPILES("${GETADDERINFO_SRC}" WORKING_GETADDERINFO)
	IF(WORKING_GETADDERINFO)
		SET(${CFLAGS_NAME}_CFLAGS "${${CFLAGS_NAME}_CFLAGS} -DHAVE_GETADDERINFO=1" CACHE STRING "TCL CFLAGS" FORCE)
	ENDIF(WORKING_GETADDERINFO)
ENDMACRO(CHECK_GETADDERINFO_WORKING_D)


