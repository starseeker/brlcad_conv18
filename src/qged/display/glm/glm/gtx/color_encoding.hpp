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

/// @ref gtx_color_encoding
/// @file glm/gtx/color_encoding.hpp
///
/// @see core (dependence)
/// @see gtx_color_encoding (dependence)
///
/// @defgroup gtx_color_encoding GLM_GTX_color_encoding
/// @ingroup gtx
///
/// Include <glm/gtx/color_encoding.hpp> to use the features of this extension.
///
/// @brief Allow to perform bit operations on integer values

#pragma once

// Dependencies
#include "../detail/setup.hpp"
#include "../detail/qualifier.hpp"
#include "../vec3.hpp"
#include <limits>

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	ifndef GLM_ENABLE_EXPERIMENTAL
#		pragma message("GLM: GLM_GTC_color_encoding is an experimental extension and may change in the future. Use #define GLM_ENABLE_EXPERIMENTAL before including it, if you really want to use it.")
#	else
#		pragma message("GLM: GLM_GTC_color_encoding extension included")
#	endif
#endif

namespace glm
{
	/// @addtogroup gtx_color_encoding
	/// @{

	/// Convert a linear sRGB color to D65 YUV.
	template<typename T, qualifier Q>
	GLM_FUNC_DECL vec<3, T, Q> convertLinearSRGBToD65XYZ(vec<3, T, Q> const& ColorLinearSRGB);

	/// Convert a linear sRGB color to D50 YUV.
	template<typename T, qualifier Q>
	GLM_FUNC_DECL vec<3, T, Q> convertLinearSRGBToD50XYZ(vec<3, T, Q> const& ColorLinearSRGB);

	/// Convert a D65 YUV color to linear sRGB.
	template<typename T, qualifier Q>
	GLM_FUNC_DECL vec<3, T, Q> convertD65XYZToLinearSRGB(vec<3, T, Q> const& ColorD65XYZ);

	/// Convert a D65 YUV color to D50 YUV.
	template<typename T, qualifier Q>
	GLM_FUNC_DECL vec<3, T, Q> convertD65XYZToD50XYZ(vec<3, T, Q> const& ColorD65XYZ);

	/// @}
} //namespace glm

#include "color_encoding.inl"
