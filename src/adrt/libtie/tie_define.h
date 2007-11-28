/*
 *  tie_define.h
 *
 *  RCSid:          $Id$
*/

#ifndef	_TIE_DEFINE_H
#define	_TIE_DEFINE_H

#include <math.h>

#define TIE_SINGLE_PREC		0		/* Use Single Precision Math */
#define	TIE_TAB1		"\1\0\0\2\2\1"	/* Triangle Index Table */
#define	TIE_KDTREE_NODE_MAX	4		/* Maximum number of triangles that can reside in a given node until it should be split */
#define	TIE_KDTREE_DEPTH_K1	1.4		/* K1 Depth Constant Coefficient */
#define	TIE_KDTREE_DEPTH_K2	1		/* K2 Contant */
#define TIE_CHECK_DEGENERATE	1

#define MAX_SLICES	100
#define MIN_SLICES	35
#define MIN_DENSITY	0.01
#define MIN_SPAN	0.15
#define SCALE_COEF	1.80

#define TIE_KDTREE_FAST		0x0
#define TIE_KDTREE_OPTIMAL	0x1

/* Type to use for floating precision */
#if TIE_SINGLE_PREC
#define tfloat float
#else
#define tfloat double
#endif


#define TCOPY(_t, _fv, _fi, _tv, _ti) { \
	*(_t *)&((uint8_t *)_tv)[_ti] = *(_t *)&((uint8_t *)_fv)[_fi]; }


#define	MATH_MIN3(_a, _b, _c, _d) { \
	_a = _b < _c ? _b < _d ? _b : _d : _c < _d ? _c : _d; }

#define MATH_MAX3(_a, _b, _c, _d) { \
	_a = _b > _c ? _b > _d ? _b : _d : _c > _d ? _c : _d; }

#define	MATH_VEC_SET(_a, _b, _c, _d) { \
	_a.v[0] = _b; \
	_a.v[1] = _c; \
	_a.v[2] = _d; }

#define MATH_VEC_MIN(_a, _b) { \
	_a.v[0] = _a.v[0] < _b.v[0] ? _a.v[0] : _b.v[0]; \
	_a.v[1] = _a.v[1] < _b.v[1] ? _a.v[1] : _b.v[1]; \
	_a.v[2] = _a.v[2] < _b.v[2] ? _a.v[2] : _b.v[2]; }

#define MATH_VEC_MAX(_a, _b) { \
	_a.v[0] = _a.v[0] > _b.v[0] ? _a.v[0] : _b.v[0]; \
	_a.v[1] = _a.v[1] > _b.v[1] ? _a.v[1] : _b.v[1]; \
	_a.v[2] = _a.v[2] > _b.v[2] ? _a.v[2] : _b.v[2]; }

#define MATH_VEC_CROSS(_a, _b, _c) { \
	_a.v[0] = _b.v[1]*_c.v[2] - _b.v[2]*_c.v[1]; \
	_a.v[1] = _b.v[2]*_c.v[0] - _b.v[0]*_c.v[2]; \
	_a.v[2] = _b.v[0]*_c.v[1] - _b.v[1]*_c.v[0]; }

#define	MATH_VEC_UNITIZE(_a) { \
	tfloat _b = 1/sqrt(_a.v[0]*_a.v[0] + _a.v[1]*_a.v[1] + _a.v[2]*_a.v[2]); \
	_a.v[0] *= _b; _a.v[1] *= _b; _a.v[2] *= _b; }

#define MATH_VEC_SQ(_a, _b) { \
	_a.v[0] = _b.v[0] * _b.v[0]; \
	_a.v[1] = _b.v[1] * _b.v[1]; \
	_a.v[2] = _b.v[2] * _b.v[2]; }

#define MATH_VEC_DOT(_a, _b, _c) { \
	_a = _b.v[0]*_c.v[0] + _b.v[1]*_c.v[1] + _b.v[2]*_c.v[2]; }

#define	MATH_VEC_ADD(_a, _b, _c) { \
	_a.v[0] = _b.v[0] + _c.v[0]; \
	_a.v[1] = _b.v[1] + _c.v[1]; \
	_a.v[2] = _b.v[2] + _c.v[2]; }

#define MATH_VEC_SUB(_a, _b, _c) { \
	_a.v[0] = _b.v[0] - _c.v[0]; \
	_a.v[1] = _b.v[1] - _c.v[1]; \
	_a.v[2] = _b.v[2] - _c.v[2]; }

#define	MATH_VEC_MUL(_a, _b, _c) { \
	_a.v[0] = _b.v[0] * _c.v[0]; \
	_a.v[1] = _b.v[1] * _c.v[1]; \
	_a.v[2] = _b.v[2] * _c.v[2]; }

#define MATH_VEC_MUL_SCALAR(_a, _b, _c) { \
	_a.v[0] = _b.v[0] * _c; \
	_a.v[1] = _b.v[1] * _c; \
	_a.v[2] = _b.v[2] * _c; }

#define	MATH_VEC_DIV(_a, _b, _c) { \
	_a.v[0] = _b.v[0] / _c.v[0]; \
	_a.v[1] = _b.v[1] / _c.v[1]; \
	_a.v[2] = _b.v[2] / _c.v[2]; }

#define MATH_BBOX(_a, _b, _c, _d, _e) { \
	MATH_MIN3(_a.v[0], _c.v[0], _d.v[0], _e.v[0]); \
	MATH_MIN3(_a.v[1], _c.v[1], _d.v[1], _e.v[1]); \
	MATH_MIN3(_a.v[2], _c.v[2], _d.v[2], _e.v[2]); \
	MATH_MAX3(_b.v[0], _c.v[0], _d.v[0], _e.v[0]); \
	MATH_MAX3(_b.v[1], _c.v[1], _d.v[1], _e.v[1]); \
	MATH_MAX3(_b.v[2], _c.v[2], _d.v[2], _e.v[2]); }

/* ======================== X-tests ======================== */
#define AXISTEST_X01(a, b, fa, fb) \
	p.v[0] = a*v0.v[1] - b*v0.v[2]; \
	p.v[2] = a*v2.v[1] - b*v2.v[2]; \
        if (p.v[0] < p.v[2]) { min = p.v[0]; max = p.v[2]; } else { min = p.v[2]; max = p.v[0]; } \
	rad = fa * half_size -> v[1] + fb * half_size -> v[2]; \
	if (min > rad || max < -rad) return 0; \

#define AXISTEST_X2(a, b, fa, fb) \
	p.v[0] = a*v0.v[1] - b*v0.v[2]; \
	p.v[1] = a*v1.v[1] - b*v1.v[2]; \
        if (p.v[0] < p.v[1]) { min = p.v[0]; max = p.v[1]; } else { min = p.v[1]; max = p.v[0]; } \
	rad = fa * half_size -> v[1] + fb * half_size -> v[2]; \
	if (min > rad || max < -rad) return 0;

/* ======================== Y-tests ======================== */
#define AXISTEST_Y02(a, b, fa, fb) \
	p.v[0] = -a*v0.v[0] + b*v0.v[2]; \
	p.v[2] = -a*v2.v[0] + b*v2.v[2]; \
        if (p.v[0] < p.v[2]) { min = p.v[0]; max = p.v[2]; } else { min = p.v[2]; max = p.v[0]; } \
	rad = fa * half_size -> v[0] + fb * half_size -> v[2]; \
	if (min > rad || max < -rad) return 0;

#define AXISTEST_Y1(a, b, fa, fb) \
	p.v[0] = -a*v0.v[0] + b*v0.v[2]; \
	p.v[1] = -a*v1.v[0] + b*v1.v[2]; \
        if (p.v[0] < p.v[1]) { min = p.v[0]; max = p.v[1]; } else { min = p.v[1]; max = p.v[0]; } \
	rad = fa * half_size -> v[0] + fb * half_size -> v[2]; \
	if (min > rad || max < -rad) return 0;

/* ======================== Z-tests ======================== */
#define AXISTEST_Z12(a, b, fa, fb) \
	p.v[1] = a*v1.v[0] - b*v1.v[1]; \
	p.v[2] = a*v2.v[0] - b*v2.v[1]; \
        if (p.v[2] < p.v[1]) { min = p.v[2]; max = p.v[1]; } else { min = p.v[1]; max = p.v[2]; } \
	rad = fa * half_size -> v[0] + fb * half_size -> v[1]; \
	if (min > rad || max < -rad) return 0;

#define AXISTEST_Z0(a, b, fa, fb) \
	p.v[0] = a*v0.v[0] - b*v0.v[1]; \
	p.v[1] = a*v1.v[0] - b*v1.v[1]; \
        if (p.v[0] < p.v[1]) { min = p.v[0]; max = p.v[1]; } else { min = p.v[1]; max = p.v[0]; } \
	rad = fa * half_size -> v[0] + fb * half_size -> v[1]; \
	if (min > rad || max < -rad) return 0;

#endif
