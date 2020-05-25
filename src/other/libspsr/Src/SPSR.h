/*
Copyright (c) 2006, Michael Kazhdan and Matthew Bolitho
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of
conditions and the following disclaimer. Redistributions in binary form must reproduce
the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution.

Neither the name of the Johns Hopkins University nor the names of its contributors
may be used to endorse or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/

#ifndef SPSR_INCLUDED
#define SPSR_INCLUDED

#include "cvertex.h"

#if defined(_WIN32)
# define COMPILER_DLLEXPORT __declspec(dllexport)
# define COMPILER_DLLIMPORT __declspec(dllimport)
#else
# define COMPILER_DLLEXPORT __attribute__ ((visibility ("default")))
# define COMPILER_DLLIMPORT __attribute__ ((visibility ("default")))
#endif

#ifndef SPSR_EXPORT
#  if defined(SPSR_DLL_EXPORTS) && defined(SPSR_DLL_IMPORTS)
#    error "Only SPSR_DLL_EXPORTS or SPSR_DLL_IMPORTS can be defined, not both."
#  elif defined(SPSR_DLL_EXPORTS)
#    define SPSR_EXPORT COMPILER_DLLEXPORT
#  elif defined(SPSR_DLL_IMPORTS)
#    define SPSR_EXPORT COMPILER_DLLIMPORT
#  else
#    define SPSR_EXPORT
#  endif
#endif

#ifndef __BEGIN_DECLS
#  ifdef __cplusplus
#    define __BEGIN_DECLS   extern "C" {
#    define __END_DECLS     }
#  else
#    define __BEGIN_DECLS
#    define __END_DECLS
#  endif
#endif

__BEGIN_DECLS

struct spsr_options {
	const char *xform;
	const char *voxelgrid;
	const char *confidence;
	const char *normalweights;
	/* boolean */
	int nonManifold;
	int polygon;
	/* int */
	int depth;
	int cgdepth;
	int kerneldepth;
	int adaptiveexponent;
	int iters;
	int voxeldepth;
	int fulldepth;
	int mindepth;
	int maxsolvedepth;
	int boundarytype;
	int thread_cnt;

	double samples_per_node;
	double scale;
	double cssolveraccuracy;
	double pointweight;
};

#define SPSR_OPTIONS_DEFAULT_INIT { NULL, NULL, NULL, NULL, 0, 0, 8, 0, 6, 1, 8, -1, 5, 0, 8, 1, 1, 1.0, 1.1, 0.001, 4.0 }

SPSR_EXPORT extern int
spsr_surface_build(int **faces, int *num_faces, double **points, int *num_pnts,
		const struct cvertex **verts, int cnt, struct spsr_options *opts);

__END_DECLS

#endif /* SPSR_INCLUDE */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
