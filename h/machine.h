#ifndef CONF_H
#include "conf.h"  /* Horrible but temporary hack to get things to compile */
#endif

/*
 *			M A C H I N E . H
 *
 *  This header file defines all the
 *
 *	fundamental data types (lower case names, created with "typedef")
 *
 *  and
 *
 *	fundamental manifest constants (upper case, created with "#define")
 *
 *  used throughout the BRL-CAD Package.  Virtually all other BRL-CAD
 *  header files depend on this header file being included first.
 *
 *  Many of these fundamental data types are machine (vendor) dependent.
 *  Some may assume different values on the same machine, depending on
 *  which version of the compiler is being used.
 *
 *  Additions will need to be made here when porting BRL-CAD to a new machine
 *  which is anything but a 32-bit big-endian uniprocessor.
 *
 *  General Symbols and Types Defined -
 *
 *	CONST  - deprecated - (use const)
 *		A portable way of indicating that the ANSI C "const"
 *		keyword is desired, when compiling on an ANSI compiler.
 *
 *	genptr_t -
 *		A portable way of declaring a "generic" pointer that is
 *		wide enough to point to anything, which can be used on
 *		both ANSI C and K&R C environments.
 *		On some machines, pointers to functions can be wider than
 *		pointers to data bytes, so a declaration of "char *"
 *		isn't generic enough.
 *
 *	SIGNED - deprecated - (use signed)
 *		A portable way of declaring a signed variable, since
 *		the "signed" keyword is not known in K&R compilers.  e.g.:
 *			register SIGNED int twoway;
 *
 *	fastf_t -
 *		Intended to be the fastest floating point data type on
 *		the current machine, with at least 64 bits of precision.
 *		On 16 and 32 bit machine, this is typically "double",
 *		but on 64 bit machines, it is often "float".
 *		Virtually all floating point variables (and more complicated
 *		data types, like vect_t and mat_t) are defined as fastf_t.
 *		The one exception is when a subroutine return is a floating
 *		point value;  that is always declared as "double".
 *
 *	LOCAL -
 *		The fastest storage class for local variables within a
 *		subroutine.  On parallel machines, this needs to be "auto",
 *		but on serial machines there can sometimes be a performance
 *		advantage to using "static".
 *
 *	FAST -
 *		The fastest storage class for fastf_t variables.
 *		On most machines with abundant registers, this is "register",
 *		but on machines like the VAX with only 3 "register double"s
 *		available to C programmers, it is set to LOCAL.
 *		Thus, declaring a fast temporary fastf_t variable is done like:
 *			FAST fastf_t var;
 *
 *	HIDDEN -
 *		Functions intended to be local to one module should be
 *		declared HIDDEN.  For production use, and lint, it will
 *		be defined as "static", but for debugging it can be defined
 *		as NIL, so that the routine names can be made available
 *		to the debugger.
 *
 *	MAX_FASTF -
 *		Very close to the largest value that can be held by a
 *		fastf_t without overflow.  Typically specified as an
 *		integer power of ten, to make the value easy to spot when
 *		printed.
 *
 *	SQRT_MAX_FASTF -
 *		sqrt(MAX_FASTF), or slightly smaller.  Any number larger than
 *		this, if squared, can be expected to produce an overflow.
 *
 *	SMALL_FASTF -
 *		Very close to the smallest value that can be represented
 *		while still being greater than zero.  Any number smaller
 *		than this (and non-negative) can be considered to be
 *		zero;  dividing by such a number can be expected to produce
 *		a divide-by-zero error.
 *		All divisors should be checked against this value before
 *		actual division is performed.
 *
 *	SQRT_SMALL_FASTF -
 *		sqrt(SMALL_FASTF), or slightly larger.  The value of this
 *		is quite a lot larger than that of SMALL_FASTF.
 *		Any number smaller than this, when squared, can be expected
 *		to produce a zero result.
 *
 *	bzero(ptr,n) -
 *		Defined to be the fasted system-specific method for
 *		zeroing a block of 'n' bytes, where the pointer has
 *		arbitrary byte alignment.  BSD semantics.
 *
 *	bcopy(from,to,n) -
 *		Defined to be the fastest system-specific method for
 *		copying a block of 'n' bytes, where both the "from" and
 *		"to" pointers have arbitrary byte alignment.  BSD semantics.
 *
 *	bitv_t -
 *		The widest fast integer type available, used to implement bit
 *		vectors.  On most machines, this is "long", but on some
 *		machines a vendor-specific type such as "long long" can
 *		give access to wider integers.
 *
 *	BITV_SHIFT -
 *		log2( bits_wide(bitv_t) ).  Used to determine how many
 *		bits of a bit-vector subscript are index-of-bit in bitv_t
 *		word, and how many bits of the subscript are for word index.
 *		On a 32-bit machine, BITV_SHIFT is 5.
 *
 *	XXX The BYTE_ORDER handling needs to change to match the POSIX
 *	XXX recommendations.
 *
 *  PARALLEL Symbols Defined -
 *    These are used only for applications linked with LIBRT,
 *    and interact heavily with the support routines in librt/machine.c
 *    XXX These are likely to get new, more descriptive names sometime.
 *
 *	PARALLEL -
 *		When defined, the code is being compiled for a parallel processor.
 *		This has implications for signal handling, math library
 *		exception handling, etc.
 *
 *	MAX_PSW -
 *		The maximum number of processors that can be expected on
 *		this hardware.  Used to allocate application-specific
 *		per-processor tables.
 *		The actual number of processors is found at runtime by calling
 *		rt_avail_cpus().
 *
 *	DEFAULT_PSW -
 *		The number of processors to use when the user has not
 *		specifically indicated the number of processors desired.
 *		On some machines like the Alliant, this should be MAX_PSW,
 *		because the parallel complex is allocated as a unit.
 *		On timesharing machines like the Cray, this should be 1,
 *		because running multi-tasking consumes special resources
 *		(and sometimes requires special queues/privs), so ordinary
 *		runs should just stay serial.
 *
 *	MALLOC_NOT_MP_SAFE -
 *		Defined when the system malloc() routine can not be
 *		safely used in a multi-processor (MP) execution.
 *		If defined, LIBBU will protect with BU_SEM_SYSCALL.
 *
 *  Author -
 *	Michael John Muuss
 *
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Distribution Status -
 *	This file is public domain, distribution unlimited.
 *
 *  Include Sequencing -
 *	#include <stdio.h>
 *	#include <math.h>
 *	#include "machine.h"
 *	#include "bu.h"
 *
 *  Libraries Used -
 *	LIBBU LIBBU_LIBES -lm -lc
 *
 *  $Header$
 */

#ifndef MACHINE_H
#define MACHINE_H seen

/*
 * Figure out the maximum number of files that can simultaneously be open 
 * by a process.
 */

#if !defined(FOPEN_MAX) && defined(_NFILE)
#	define FOPEN_MAX	_NFILE
#endif
#if !defined(FOPEN_MAX) && defined(NOFILE)
#	define FOPEN_MAX	NOFILE
#endif
#if !defined(FOPEN_MAX) && defined(OPEN_MAX)
#	define FOPEN_MAX	OPEN_MAX
#endif
#if !defined(FOPEN_MAX)
#	define FOPEN_MAX	32
#endif

/**********************************
 *                                *
 *  Machine specific definitions  *
 *  Choose for maximum speed      *
 *				  *
 **********************************/

#ifdef WIN32
typedef double fastf_t;	
#define LOCAL auto	
#define FAST register	
typedef long bitv_t;	
#define CONST const	 
#define BITV_SHIFT	5
// assume only one processor for now
#define MAX_PSW	4
#define DEFAULT_PSW	1
#define PARALLEL	1
#define MALLOC_NOT_MP_SAFE 1
#endif

#ifdef ipsc860
/********************************
 *				*
 *   Intel iPSC/860 Hypercube	*
 *				*
 ********************************/
/* icc compiler gets confused on const typedefs */
#define	const	/**/
#define	const	/**/
#define MALLOC_NOT_MP_SAFE 1
#endif

#if defined(SUNOS) && SUNOS >= 50
/********************************
 *				*
 *   Sun Running Solaris 2.X    *
 *   aka SunOS 5.X              *
 *				*
 ********************************/

#define IEEE_FLOAT 1		/* Uses IEEE style floating point */
typedef double	fastf_t;	/* double|float, "Fastest" float type */
#define LOCAL	auto		/* static|auto, for serial|parallel cpu */
#define FAST	register	/* LOCAL|register, for fastest floats */
typedef long	bitv_t;		/* largest integer type */
#define BITV_SHIFT	5	/* log2( bits_wide(bitv_t) ) */

#define MAX_PSW		256	/* need to increase this for Super Dragon? */
#define DEFAULT_PSW	bu_avail_cpus()
#define PARALLEL	1

#endif

#if defined(hppa) 
/********************************
 *				*
 *   HP 9000/700                *
 *   Running HP-UX 9.1          *
 *				*
 ********************************/

#define IEEE_FLOAT 1		/* Uses IEEE style floating point */
typedef double	fastf_t;	/* double|float, "Fastest" float type */
#define LOCAL	auto		/* static|auto, for serial|parallel cpu */
#define FAST	register	/* LOCAL|register, for fastest floats */
typedef long	bitv_t;		/* largest integer type */
#define BITV_SHIFT	5	/* log2( bits_wide(bitv_t) ) */

#define const   /**/            /* Does not support const keyword */
#define const   /**/            /* Does not support const keyword */

#define MAX_PSW		1	/* only one processor, max */
#define DEFAULT_PSW	1
#define MALLOC_NOT_MP_SAFE 1

#endif

#ifdef __ppc__
/********************************
 *                              *
 *      Macintosh PowerPC       *
 *                              *
 ********************************/
#define IEEE_FLOAT      1       /* Uses IEEE style floating point */
typedef double  fastf_t;        /* double|float, "Fastest" float type */
#define LOCAL   auto            /* static|auto, for serial|parallel cpu */
#define FAST    register        /* LOCAL|register, for fastest floats */
typedef long    bitv_t;         /* could use long long */
#define BITV_SHIFT      5       /* log2( bits_wide(bitv_t) ) */
#define MAX_PSW         512       /* Unused, but useful for thread debugging */
#define DEFAULT_PSW     bu_avail_cpus()	/* use as many as we can */
#define PARALLEL        1
/* #define MALLOC_NOT_MP_SAFE 1 -- not confirmed */
#endif

#ifdef __sp3__
/********************************
 *                              *
 *      IBM SP3                 *
 *                              *
 ********************************/
#define IEEE_FLOAT      1       /* Uses IEEE style floating point */
typedef double  fastf_t;        /* double|float, "Fastest" float type */
#define LOCAL   auto            /* static|auto, for serial|parallel cpu */
#define FAST    register        /* LOCAL|register, for fastest floats */
typedef long	bitv_t;		/* largest integer type */
#define BITV_SHIFT	5	/* log2( bits_wide(bitv_t) ) */

#if 1	/* Multi-CPU SP3 build */
#	define MAX_PSW		32     	/* they can go 32-way per single image */
#	define DEFAULT_PSW	bu_avail_cpus()	/* use as many as are configured by default */
#	define	PARALLEL	1
#	define	HAS_POSIX_THREADS	1
#	define	MALLOC_NOT_MP_SAFE	1	/* XXX Not sure about this */
#else	/* 1 CPU SP3 build */
#	define MAX_PSW		1	/* only one processor, max */
#	define DEFAULT_PSW	1
#endif

#endif

#ifdef linux
/********************************
 *                              *
 *        Linux on IA32         *
 *                              *
 ********************************/
#define IEEE_FLOAT      1      /* Uses IEEE style floating point */
#define BITV_SHIFT      5      /* log2( bits_wide(bitv_t) ) */

typedef double fastf_t;       /* double|float, "Fastest" float type */
typedef long bitv_t;          /* could use long long */

/*
 * Note that by default a Linux installation supports parallel using
 * pthreads. For a 1 cpu installation, toggle these blocks
 */
# if 1 /* multi-cpu linux build */

# define LOCAL auto             /* static|auto, for serial|parallel cpu */
# define FAST register          /* LOCAL|register, for fastest floats */
# define MAX_PSW         16
# define DEFAULT_PSW     bu_avail_cpus()	/* use as many processors as are available */
# define PARALLEL        1
# define HAS_POSIX_THREADS 1    /* formerly in conf.h */
# define MALLOC_NOT_MP_SAFE 1   /* uncertain, but this is safer for now */

# else  /* 1 CPU Linux build */

# define LOCAL static		/* static|auto, for serial|parallel cpu */
# define FAST LOCAL		/* LOCAL|register, for fastest floats */
# define MAX_PSW        1	/* only one processor, max */
# define DEFAULT_PSW	1

# endif
#endif /* linux */

#ifndef LOCAL
/********************************
 *				*
 * Default 32-bit uniprocessor	*
 *  VAX, Gould, SUN, SGI	*
 *				*
 ********************************/
typedef double	fastf_t;	/* double|float, "Fastest" float type */
#define LOCAL	static		/* static|auto, for serial|parallel cpu */
#define FAST	LOCAL		/* LOCAL|register, for fastest floats */
typedef long	bitv_t;		/* largest integer type */
#define BITV_SHIFT	5	/* log2( bits_wide(bitv_t) ) */

#define MAX_PSW		4	/* allow for a dual core dual */
#define DEFAULT_PSW	bu_avail_cpus()	/* use as many as are available by default */ 

#endif

/*
 *  Definitions for big-endian -vs- little-endian.
 *	BIG_ENDIAN:	Byte [0] is on left side of word (msb).
 *	LITTLE_ENDIAN:	Byte [0] is on right side of word (lsb).

 *  Bit vector mask */
#define BITV_MASK	((1<<BITV_SHIFT)-1)

/*
 *  Definition of a "generic" pointer that can hold a pointer to anything.
 *  According to tradition, a (char *) was generic, but the ANSI folks
 *  worry about machines where (int *) might be wider than (char *),
 *  so here is the clean way of handling it.
 */
#if !defined(GENPTR_NULL)
#  if __STDC__
	typedef void	*genptr_t;
#  else
	typedef char	*genptr_t;
#  endif
#  define GENPTR_NULL	((genptr_t)0)
#endif

/* A portable way of handling the ANSI C const keyword: use CONST */
#if !defined(CONST)
# if __STDC__
#	define	CONST	const
# else
#	define	CONST	/**/
# endif
#endif

/* Even in C++ not all compilers know the "bool" keyword yet */
#if !defined(BOOL_T)
# define BOOL_T	int
#endif

/* A portable way of dealing with pre-ANSI C.  Assume signed variables */
#if !defined(SIGNED)
# if __STDC__
#	define SIGNED	signed
# else
#	define SIGNED	/**/
# endif
#endif

/* Functions local to one file should be declared HIDDEN:  (nil)|static */
/* To aid in using ADB, generally leave this as nil. */
#if !defined(HIDDEN)
# if defined(lint)
#	define HIDDEN	static
# else
#	define HIDDEN	/***/
# endif
#endif

#if defined(WIN32)
#	define hypot _hypot
#endif
#if defined(SUNOS) && SUNOS >= 52
        extern double hypot(double, double);
#endif

#endif
