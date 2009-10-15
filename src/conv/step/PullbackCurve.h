#ifndef PULLBACK_CURVE
#define PULLBACK_CURVE

#include "opennurbs.h"


/*                  p u l l b a c k _ c u r v e
*
* Pull an arbitrary model-space *curve* onto the given *surface* as a
* curve within the surface's domain when, for each point c = C(t) on
* the curve and the closest point s = S(u, v) on the surface, we have:
* distance(c, s) <= *tolerance*.
*
* The resulting 2-dimensional curve will be approximated using the
* following process:
*
* 1. Adaptively sample the 3d curve in the domain of the surface
* (ensure tolerance constraint). Sampling terminates when the
* following flatness criterion is met:
*    given two parameters on the curve t1 and t2 (which map to points p1 and p2 on the curve)
*    let m be a parameter randomly chosen near the middle of the interval [t1, t2]
*                                                              ____
*    then the curve between t1 and t2 is flat if distance(C(m), p1p2) < flatness
*
* 2. Use the sampled points to perform a global interpolation using
*    universal knot generation to build a B-Spline curve.
*
* 3. If the curve is a line or an arc (determined with openNURBS routines),
*    return the appropriate ON_Curve subclass (otherwise, return an ON_NurbsCurve).
*
*
*/

enum seam_direction {
	NORTH_SEAM,
	EAST_SEAM,
	SOUTH_SEAM,
	WEST_SEAM,
	UNKNOWN_SEAM_DIRECTION
};

#define NEAR_EQUAL(_a, _b, _tol) (fabs((_a) - (_b)) <= _tol)
#define PBC_TOL 0.000001
#define PBC_FROM_OFFSET 0.001

typedef struct pbc_data {
  double tolerance;
  double flatness;
  const ON_Curve* curve;
  brlcad::SurfaceTree* surftree;
  list<ON_2dPointArray *> segments;
  const ON_BrepEdge* edge;
  bool order_reversed;
} PBCData;

extern enum seam_direction seam_direction(ON_2dPoint uv1,ON_2dPoint uv2);

extern ON_Curve*
interpolateCurve(ON_2dPointArray& samples);

extern ON_Curve*
test1_pullback_curve(const brlcad::SurfaceTree* surfacetree,
		const ON_Curve* curve,
		double tolerance = 1.0e-6,
		double flatness = 1.0e-3);

extern ON_Curve*
test2_pullback_curve(const brlcad::SurfaceTree* surfacetree,
		const ON_Curve* curve,
		double tolerance = 1.0e-6,
		double flatness = 1.0e-3);

extern PBCData *
test2_pullback_samples(const brlcad::SurfaceTree* surfacetree,
		const ON_Curve* curve,
		double tolerance = 1.0e-6,
		double flatness = 1.0e-3);

extern bool
check_pullback_data( list<PBCData*> &pbcs);

extern int
check_pullback_singularity_bridge(const ON_Surface *surf,const ON_2dPoint &p1,const ON_2dPoint &p2);

extern int
check_pullback_seam_bridge(const ON_Surface *surf,const ON_2dPoint &p1,const ON_2dPoint &p2);

extern ON_Curve*
pullback_curve(const brlcad::SurfaceTree* surfacetree,
const ON_Curve* curve,
double tolerance = 1.0e-6,
double flatness = 1.0e-3);

extern ON_Curve*
pullback_seam_curve(enum seam_direction seam_dir,
		const brlcad::SurfaceTree* surfacetree,
		const ON_Curve* curve,
		double tolerance = 1.0e-6,
		double flatness = 1.0e-3);

extern bool
toUV(brlcad::SurfaceTree *surftree, const ON_Curve *curve,  ON_2dPoint& out_pt, double t, double knudge);

#endif
