/*
 * OpenGL Mathematics (GLM)
 * --------------------------------------------------------------------------------
 * GLM is licensed under The Happy Bunny License or MIT License
 *
 * BRL-CAD will use glm under the standard MIT license
 *
 * ================================================================================
 * The MIT License
 * --------------------------------------------------------------------------------
 * Copyright (c) 2005 - G-Truc Creation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/// @ref gtx
/// @file glm/gtx/scalar_multiplication.hpp
/// @author Joshua Moerman
///
/// Include <glm/gtx/scalar_multiplication.hpp> to use the features of this extension.
///
/// Enables scalar multiplication for all types
///
/// Since GLSL is very strict about types, the following (often used) combinations do not work:
///    double * vec4
///    int * vec4
///    vec4 / int
/// So we'll fix that! Of course "float * vec4" should remain the same (hence the enable_if magic)

#pragma once

#include "../detail/setup.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	ifndef GLM_ENABLE_EXPERIMENTAL
#		pragma message("GLM: GLM_GTX_scalar_multiplication is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it.")
#	else
#		pragma message("GLM: GLM_GTX_scalar_multiplication extension included")
#	endif
#endif

#include "../vec2.hpp"
#include "../vec3.hpp"
#include "../vec4.hpp"
#include "../mat2x2.hpp"
#include <type_traits>

namespace glm
{
	template<typename T, typename Vec>
	using return_type_scalar_multiplication = typename std::enable_if<
		!std::is_same<T, float>::value       // T may not be a float
		&& std::is_arithmetic<T>::value, Vec // But it may be an int or double (no vec3 or mat3, ...)
	>::type;

#define GLM_IMPLEMENT_SCAL_MULT(Vec) \
	template<typename T> \
	return_type_scalar_multiplication<T, Vec> \
	operator*(T const& s, Vec rh){ \
		return rh *= static_cast<float>(s); \
	} \
	 \
	template<typename T> \
	return_type_scalar_multiplication<T, Vec> \
	operator*(Vec lh, T const& s){ \
		return lh *= static_cast<float>(s); \
	} \
	 \
	template<typename T> \
	return_type_scalar_multiplication<T, Vec> \
	operator/(Vec lh, T const& s){ \
		return lh *= 1.0f / static_cast<float>(s); \
	}

GLM_IMPLEMENT_SCAL_MULT(vec2)
GLM_IMPLEMENT_SCAL_MULT(vec3)
GLM_IMPLEMENT_SCAL_MULT(vec4)

GLM_IMPLEMENT_SCAL_MULT(mat2)
GLM_IMPLEMENT_SCAL_MULT(mat2x3)
GLM_IMPLEMENT_SCAL_MULT(mat2x4)
GLM_IMPLEMENT_SCAL_MULT(mat3x2)
GLM_IMPLEMENT_SCAL_MULT(mat3)
GLM_IMPLEMENT_SCAL_MULT(mat3x4)
GLM_IMPLEMENT_SCAL_MULT(mat4x2)
GLM_IMPLEMENT_SCAL_MULT(mat4x3)
GLM_IMPLEMENT_SCAL_MULT(mat4)

#undef GLM_IMPLEMENT_SCAL_MULT
} // namespace glm
