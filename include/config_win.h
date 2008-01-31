
/*                          C O N F I G _ W I N . H
 * BRL-CAD
 *
 * Copyright (c) 1993-2008 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @addtogroup fixme */
/** @{ */
/** @file config_win.h
 *
 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifndef IGNORE_CONFIG_H
#if defined(_WIN32)

/* XXX - This is temporary (atleast until a brlcad_config.h is
 * auto-generated on windows)
 */
#define __STDC__ 1
#define USE_PROTOTYPES 1

#pragma warning( disable : 4244 4305 4018)
/*  4244 conversion from type 1 to type 2
    4305 truncation
	4018 signed/unsigned mismatch
*/
/*
 * Ensure that Project Settings / Project Options includes
 *	/Za		for ANSI C
 */
# if !__STDC__
#	error "STDC is not properly set on WIN32 build, add /Za to Project Settings / Project Options"
# endif

#ifndef EXPAND_IN_STRING
#  define EXPAND_IN_STRING(x) EXPAND_IN_STRING_INTERN(x)
#  define EXPAND_IN_STRING_INTERN(x) #x
#endif

#define INSTALL_DIRECTORY    "C:/brlcad" MAJOR_VERSION_STRING "_" MINOR_VERSION_STRING "_" PATCH_VERSION_STRING
#define IWIDGETS_VERSION  "4.0.2"

#define REVERSE_IEEE		yes
#define HAS_OPENGL		1
#define HAVE_FCNTL_H		1
#define HAVE_PUTENV     	1
#define HAVE_GETHOSTNAME	1
#define HAVE_GETPROGNAME        1
#define HAVE_GL_GL_H		1
#define HAVE_IO_H		1
#define HAVE_MEMORY_H		1
#define HAVE_MEMSET		1
#define HAVE_OFF_T		1
#define HAVE_PROCESS_H  	1
#define HAVE_REGEX_H		1
#define HAVE_STRCHR		1
#define HAVE_STRDUP		1
#define HAVE_STRDUP_DECL	1
#define HAVE_STRTOK		1
#define HAVE_SYS_STAT_H		1
#define HAVE_SYS_TYPES_H	1
#define HAVE_TIME		1
#define HAVE_VFORK		1
#define HAVE_WINSOCK_H		1

#define HAVE_SBRK 1
#define sbrk(i) (NULL)

/* XXX - do not rely on config_win.h providing these headers.  they
 * will be removed at some point.
 */
#include <windows.h>
#include <io.h>
#include <fcntl.h>

#ifndef S_IFMT
#  define S_IFMT _S_IFMT
#endif
#ifndef S_IFDIR
#  define S_IFDIR _S_IFDIR
#endif
#ifndef S_IFCHR
#  define S_IFCHR _S_IFCHR
#endif
#ifndef S_IFREG
#  define S_IFREG _S_IFREG
#endif

#ifndef S_IREAD
#  define S_IREAD _S_IREAD
#endif
#ifndef S_IWRITE
#  define S_IWRITE _S_IWRITE
#endif
#ifndef S_IEXEC
#  define S_IEXEC _S_IEXEC
#endif

#ifndef S_IRUSR
#  define S_IRUSR      S_IREAD
#endif
#ifndef S_IWUSR
#  define S_IWUSR      S_IWRITE
#endif
#ifndef S_IXUSR
#  define S_IXUSR      S_IEXEC
#endif

#ifndef S_IRGRP
#  define S_IRGRP      ((S_IRUSR)>>3)
#endif
#ifndef S_IWGRP
#  define S_IWGRP      ((S_IWUSR)>>3)
#endif
#ifndef S_IXGRP
#  define S_IXGRP      ((S_IXUSR)>>3)
#endif
#ifndef S_IROTH
#  define S_IROTH      ((S_IRUSR)>>6)
#endif
#ifndef S_IWOTH
#  define S_IWOTH      ((S_IWUSR)>>6)
#endif
#ifndef S_IXOTH
#  define S_IXOTH      ((S_IXUSR)>>6)
#endif

#define	isnan _isnan
#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0
#define MAXPATHLEN _MAX_PATH
#define O_APPEND _O_APPEND
#define O_BINARY _O_BINARY
#define O_CREAT _O_CREAT
#define O_EXCL _O_EXCL
#define O_RDONLY _O_RDONLY
#define O_RDWR _O_RDWR
#define O_TRUNC _O_TRUNC
#define O_WRONLY _O_WRONLY
#define SIGALRM SIGTERM

#define access _access
#define alarm(_sec) -1
#define chmod _chmod
#define close _close
#define commit _commit
#define creat _creat
#define dup _dup
#define dup2 _dup2
#define execvp _execvp
#define fdopen _fdopen
#define fileno _fileno
#define fork() -1
#define fstat _fstat
#define getpid _getpid
#define getprogname() _pgmptr
#define hypot _hypot
#define ioctl ioctlsocket
#define isascii __isascii
#define isatty _isatty
#define locking _locking
#define lseek _lseek
#define off_t _off_t
#define open _open
#define pclose _pclose
#define pipe(_FD) (_pipe((_FD), 256, _O_TEXT))
#define popen _popen
#define putenv _putenv
#define read _read
#define rint(_X) (floor((_X) + 0.5))
#define setmode _setmode
#define sleep(_SECONDS) (Sleep(1000 * (_SECONDS)))
#define snprintf _snprintf
#define sopen _sopen
#define stat _stat
#define strcasecmp _stricmp
#define strdup _strdup
#define sys_errlist _sys_errlist
#define sys_nerr _sys_nerr
#define snprintf _snprintf
#define umask _umask
#define unlink _unlink
#define write _write
#undef DELETE
#undef IN
#undef OUT
#undef complex
#undef rad1
#undef rad2
#define pid_t int
#define socklen_t int
#define uid_t unsigned int
#define gid_t unsigned int
#define fmax max
#define uint8_t unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int

#if defined(_MSC_VER) && (_MSC_VER <= 1200) /* MSVC 6.0 and before */
#   define for if (0) {} else for           /* proper for-scope */
#endif

#ifdef small
#   undef small /* defined as part of the Microsoft Interface Definition Language (MIDL) */
#endif

#ifdef __cplusplus
#   ifdef min
#       undef min

        template<class _Type> inline const _Type& min(const _Type& _value1,
                                                  const _Type& _value2) {
            return (_value2 < _value1 ? _value2 : _value1);
        }
#   endif

#   ifdef max
#       undef max

        template<class Type> inline const Type& max(const Type& _value1,
                                                const Type& _value2) {
            return (_value1 < _value2 ? _value2 : _value1);
        }
#   endif
#endif /* __cplusplus */

#endif /* if defined(_WIN32) */
#endif /* ifndef IGNORE_CONFIG_H */

#endif /* __CONFIG_H__ */
/** @} */
/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
