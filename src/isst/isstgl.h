/*                        I S S T G L . H
 * BRL-CAD
 *
 * Copyright (c) 2021 United States Government as represented by
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
/** @file isstgl.h
 *
 * Brief description
 *
 */

#ifndef ISSTGL_H
#define ISSTGL_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
extern "C" {
#include "rt/tie.h"
#include "adrt.h"
#include "adrt_struct.h"
#include "librender/camera.h"
}

// Use QOpenGLFunctions so we don't have to prefix all OpenGL calls with "f->"
class isstGL : public QOpenGLWidget
{
    public:
	isstGL();

	struct tie_s *tie = NULL; // From parent app

	void render();

#if 0
    protected:
	void resizeGL(int w, int h) override;
	void paintGL() override;
#endif


    public:
	struct render_camera_s camera;
	struct camera_tile_s tile;
	tienet_buffer_t buffer_image;
	void *texdata = NULL;
	GLuint texid;
};

#endif /* ISSTGL_H */

// Local Variables:
// tab-width: 8
// mode: C++
// c-basic-offset: 4
// indent-tabs-mode: t
// c-file-style: "stroustrup"
// End:
// ex: shiftwidth=4 tabstop=8

