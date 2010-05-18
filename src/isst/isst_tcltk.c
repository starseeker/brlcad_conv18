/*
 * Copyright (c) 2005-2010 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file main.c
 *
 *  Tcl/Tk wrapper for adrt/tie functionality, used by Tcl/Tk version
 *  of ISST.
 *
 */

#include <stdio.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "tcl.h"
#include "tk.h"

#define USE_TOGL_STUBS

#include "togl.h"

#include "tie.h"
#include "adrt.h"
#include "adrt_struct.h"
#include "camera.h"
#include "isst.h"

void resize_isst(struct isst_s *);

/* new window size or exposure */
static int
reshape(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Togl   *togl;
    struct isst_s *isst;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "pathName");
	return TCL_ERROR;
    }

    if (Togl_GetToglFromObj(interp, objv[1], &togl) != TCL_OK) {
	return TCL_ERROR;
    }

    isst = Togl_GetClientData(togl);

    isst->w = Togl_Width(togl);
    isst->h = Togl_Height(togl);

    resize_isst(isst);

    return TCL_OK;
}

void
resize_isst(struct isst_s *isst)
{
    switch(isst->gs) {
            case 0:
                isst->camera.w = isst->tile.size_x = isst->w;
                isst->camera.h = isst->tile.size_y = isst->h;
                break;
            case 1:
                isst->camera.w = isst->tile.size_x = 320;
                isst->camera.h = isst->tile.size_y = isst->camera.w * isst->h / isst->w;
                break;
            case 2:
                isst->camera.w = isst->tile.size_x = 40;
                isst->camera.h = isst->tile.size_y = isst->camera.w * isst->h / isst->w;
                break;
            default:
                bu_log("Unknown level...\n");
    }
    isst->tile.format = RENDER_CAMERA_BIT_DEPTH_24;
    TIENET_BUFFER_SIZE(isst->buffer_image, 3 * isst->camera.w * isst->camera.h);
    glClearColor (0.0, 0, 0.0, 1);
    glBindTexture (GL_TEXTURE_2D, isst->texid);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    isst->texdata = realloc(isst->texdata, isst->camera.w * isst->camera.h * 3);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, isst->camera.w, isst->camera.h, 0, GL_RGB, GL_UNSIGNED_BYTE, isst->texdata);
    glDisable(GL_LIGHTING);

    glViewport(0,0,isst->w, isst->h);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glOrtho(0, isst->w, isst->h, 0, -1, 1);
    glMatrixMode (GL_MODELVIEW);

    glClear(GL_COLOR_BUFFER_BIT);
}


static int
isst_load_g(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    struct isst_s *isst;
    char *argstring;
    char **argv;
    int argc = 2;
    double az, el;
    struct bu_vls tclstr;
    bu_vls_init(&tclstr);    

    vect_t vec;
    Togl   *togl;

    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "load_g pathname object");
        return TCL_ERROR;
    }

    bu_log("objv[1]: %s\n", Tcl_GetString(objv[1]));

    if (Togl_GetToglFromObj(interp, objv[1], &togl) != TCL_OK) {
        return TCL_ERROR;
    }

    argv = (char **)bu_malloc(sizeof(char *)*(argc + 1), "isst tcltk");
    argv[0] = Tcl_GetString(objv[3]);
    argv[1] = NULL;
    isst = (struct isst_s *) Togl_GetClientData(togl);

    load_g(isst->tie, Tcl_GetString(objv[2]), argc-1, (const char **)argv, &(isst->meshes));

    bu_free((genptr_t)argv, "isst tcltk"); 

    VSETALL(isst->camera.pos.v, isst->tie->radius);
    VMOVE(isst->camera.focus.v, isst->tie->mid);
    VMOVE(isst->camera_pos_init, isst->camera.pos.v);
    VMOVE(isst->camera_focus_init, isst->camera.focus.v);

    /* Set the inital az and el values in Tcl/Tk */
    VSUB2(vec, isst->camera.pos.v, isst->camera.focus.v);
    VUNITIZE(vec);
    AZEL_FROM_V3DIR(az, el, vec);
    az = az * -DEG2RAD;
    el = el * -DEG2RAD;
    bu_vls_sprintf(&tclstr, "%f", az);
    Tcl_SetVar(interp, "az", bu_vls_addr(&tclstr), 0);
    bu_vls_sprintf(&tclstr, "%f", el);
    Tcl_SetVar(interp, "el", bu_vls_addr(&tclstr), 0);
    bu_vls_free(&tclstr);

    render_phong_init(&isst->camera.render, NULL);

    isst->ogl = 1;
    isst->w = Togl_Width(togl);
    isst->h = Togl_Height(togl);

    resize_isst(isst);

    gettimeofday(&(isst->t1), NULL);
    gettimeofday(&(isst->t2), NULL);

    return TCL_OK;
}

static int
paint_window(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    struct isst_s *isst;
    Togl   *togl;
    double dt = 1.0;
    int glclrbts = GL_DEPTH_BUFFER_BIT;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "pathName");
        return TCL_ERROR;
    }

    if (Togl_GetToglFromObj(interp, objv[1], &togl) != TCL_OK) {
        return TCL_ERROR;
    }
    
    isst = (struct isst *) Togl_GetClientData(togl);

    gettimeofday(&(isst->t2), NULL);

    dt = (((double)isst->t2.tv_sec+(double)isst->t2.tv_usec/(double)1e6) - ((double)isst->t1.tv_sec+(double)isst->t1.tv_usec/(double)1e6));

    if (dt > 0.08) {
    isst->buffer_image.ind = 0;

    render_camera_prep(&isst->camera);
    render_camera_render(&isst->camera, isst->tie, &isst->tile, &isst->buffer_image);

    gettimeofday(&(isst->t1), NULL);

    glClear(glclrbts);
    glLoadIdentity();
    glColor3f(1,1,1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, isst->texid);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, isst->camera.w, isst->camera.h, GL_RGB, GL_UNSIGNED_BYTE, isst->buffer_image.data + sizeof(camera_tile_t));
    glBegin(GL_TRIANGLE_STRIP);

    glTexCoord2d(0, 0); glVertex3f(0, 0, 0);
    glTexCoord2d(0, 1); glVertex3f(0, isst->h, 0);
    glTexCoord2d(1, 0); glVertex3f(isst->w, 0, 0);
    glTexCoord2d(1, 1); glVertex3f(isst->w, isst->h, 0);

    glEnd();

    Togl_SwapBuffers(togl);
    }
    return TCL_OK;
}

static int
idle(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    struct isst_s *isst;
    Togl   *togl;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "pathName");
        return TCL_ERROR;
    }

    if (Togl_GetToglFromObj(interp, objv[1], &togl) != TCL_OK) {
        return TCL_ERROR;
    }

    isst = (struct isst *) Togl_GetClientData(togl);
    Togl_PostRedisplay(togl);

    return TCL_OK;
}


static int
isst_init(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    struct isst_s *isst;
    Togl   *togl;
    static GLfloat pos[4] = { 5, 5, 10, 0 };

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "pathName");
        return TCL_ERROR;
    }

    if (Togl_GetToglFromObj(interp, objv[1], &togl) != TCL_OK) {
        return TCL_ERROR;
    }

    isst = (struct isst_s *)bu_calloc(1, sizeof(struct isst_s), isst);
    isst->ui = 0;
    isst->uic = 0;
    isst->tie = (struct tie_s *)bu_calloc(1,sizeof(struct tie_s), "tie");
    TIENET_BUFFER_INIT(isst->buffer_image);
    render_camera_init(&isst->camera, bu_avail_cpus());
    isst->camera.type = RENDER_CAMERA_PERSPECTIVE;
    isst->camera.fov = 25;

    Togl_SetClientData(togl, (ClientData) isst);

    return TCL_OK;
}

static int
isst_zap(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    struct isst_s *isst;
    Togl   *togl;
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "pathName");
        return TCL_ERROR;
    }

    if (Togl_GetToglFromObj(interp, objv[1], &togl) != TCL_OK) {
        return TCL_ERROR;
    }

    isst = (struct isst *) Togl_GetClientData(togl);

    bu_free(isst, "isst free");
}

static int
position(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    struct isst_s *isst;
    char    Result[100];
    Togl   *togl;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "pathName");
        return TCL_ERROR;
    }

    if (Togl_GetToglFromObj(interp, objv[1], &togl) != TCL_OK) {
        return TCL_ERROR;
    }

    isst = (struct isst *) Togl_GetClientData(togl);

    /* Let result string equal value */
    sprintf(Result, "TODO?");

    Tcl_SetResult(interp, Result, TCL_VOLATILE);
    return TCL_OK;
}

static int
look(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    struct isst_s *isst;
    Togl   *togl;

    if (objc != 8) {
        Tcl_WrongNumArgs(interp, 1, objv, "pathName posX posY posZ focX focY focZ");
        return TCL_ERROR;
    }

    if (Togl_GetToglFromObj(interp, objv[1], &togl) != TCL_OK)
        return TCL_ERROR;

    isst = (struct isst_s *) Togl_GetClientData(togl);

    if (Tcl_GetDoubleFromObj(interp, objv[2], &isst->camera.pos.v[0]) != TCL_OK)
        return TCL_ERROR;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &isst->camera.pos.v[1]) != TCL_OK)
        return TCL_ERROR;
    if (Tcl_GetDoubleFromObj(interp, objv[4], &isst->camera.pos.v[2]) != TCL_OK)
        return TCL_ERROR;

    if (Tcl_GetDoubleFromObj(interp, objv[5], &isst->camera.focus.v[0]) != TCL_OK)
        return TCL_ERROR;
    if (Tcl_GetDoubleFromObj(interp, objv[6], &isst->camera.focus.v[1]) != TCL_OK)
        return TCL_ERROR;
    if (Tcl_GetDoubleFromObj(interp, objv[7], &isst->camera.focus.v[2]) != TCL_OK)
        return TCL_ERROR;

    Togl_PostRedisplay(togl);

    return TCL_OK;
}

static int
render_mode(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    struct isst_s *isst;
    Togl *togl;
    char buf[BUFSIZ];

    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "pathName mode [arguments]");
        return TCL_ERROR;
    }

    if (Togl_GetToglFromObj(interp, objv[1], &togl) != TCL_OK)
        return TCL_ERROR;

    isst = (struct isst_s *) Togl_GetClientData(togl);

    /* pack the 'rest' into buf - probably should use a vls for this*/

    if(render_shader_init(&isst->camera.render, Tcl_GetString(objv[2]), *buf?buf:NULL) != 0)
	return TCL_ERROR;
    return TCL_OK;
}


static int
zero_view(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    struct isst_s *isst;
    Togl *togl;
    vect_t vec;

    if (Togl_GetToglFromObj(interp, objv[1], &togl) != TCL_OK)
        return TCL_ERROR;

    isst = (struct isst_s *) Togl_GetClientData(togl);

    VMOVE(isst->camera.focus.v, isst->camera_focus_init);
    VMOVE(isst->camera.pos.v, isst->camera_pos_init);

    return TCL_OK;
}



static int
aetolookat(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    struct isst_s *isst;
    Togl *togl;
    vect_t vec;
    double az, el;

    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "pathName az el");
        return TCL_ERROR;
    }

    if (Togl_GetToglFromObj(interp, objv[1], &togl) != TCL_OK)
        return TCL_ERROR;

    isst = (struct isst_s *) Togl_GetClientData(togl);

    if (Tcl_GetDoubleFromObj(interp, objv[2], &az) != TCL_OK)
        return TCL_ERROR;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &el) != TCL_OK)
        return TCL_ERROR;

    V3DIR_FROM_AZEL(vec, az, el);
    VADD2(isst->camera.focus.v, isst->camera.pos.v, vec);

    return TCL_OK;
}

static int
aerotate(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    struct isst_s *isst;
    Togl *togl;
    vect_t vec;
    double x, y;
    double az, el;
    double mag;

    if (objc < 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "pathName x y");
        return TCL_ERROR;
    }

    if (Togl_GetToglFromObj(interp, objv[1], &togl) != TCL_OK)
        return TCL_ERROR;

    isst = (struct isst_s *) Togl_GetClientData(togl);

    if (Tcl_GetDoubleFromObj(interp, objv[2], &x) != TCL_OK)
        return TCL_ERROR;
    if (Tcl_GetDoubleFromObj(interp, objv[3], &y) != TCL_OK)
        return TCL_ERROR;

    mag = fabs(DIST_PT_PT(isst->camera.pos.v, isst->camera.focus.v));

    VSUB2(vec, isst->camera.focus.v, isst->camera.pos.v);
    VUNITIZE(vec);
    AZEL_FROM_V3DIR(az, el, vec);
    az = az * -DEG2RAD - x;
    el = el * -DEG2RAD + y;

   /* clamp to sane values */
    while(az > 2*M_PI) az -= 2*M_PI;
    while(az < 0) az += 2*M_PI;
    if(el>M_PI_2) el=M_PI_2 - 0.001;
    if(el<-M_PI_2) el=-M_PI_2 + 0.001;

    V3DIR_FROM_AZEL(vec, az, el);
    VSCALE(vec, vec, mag);
    VADD2(isst->camera.pos.v, isst->camera.focus.v, vec);

    return TCL_OK;
}
int
Isst_Init(Tcl_Interp *interp)
{
    if (Tcl_PkgProvide(interp, "isst", "0.1") != TCL_OK) {
        return TCL_ERROR;
    }

    /* 
     * Initialize Tcl and the Togl widget module.
     */
    if (Tcl_InitStubs(interp, "8.1", 0) == NULL
            || Togl_InitStubs(interp, "2.0", 0) == NULL) {
        return TCL_ERROR;
    }

    /* 
     * Specify the C callback functions for widget creation, display,
     * and reshape.
     */
    Tcl_CreateObjCommand(interp, "isst_init", isst_init, NULL, NULL);
    Tcl_CreateObjCommand(interp, "isst_zap", isst_zap, NULL, NULL);
    Tcl_CreateObjCommand(interp, "refresh_ogl", paint_window, NULL, NULL);
    Tcl_CreateObjCommand(interp, "reshape", reshape, NULL, NULL);
    Tcl_CreateObjCommand(interp, "load_g", isst_load_g, NULL, NULL);
    Tcl_CreateObjCommand(interp, "idle", idle, NULL, NULL);
    Tcl_CreateObjCommand(interp, "look", look, NULL, NULL);
    Tcl_CreateObjCommand(interp, "aetolookat", aetolookat, NULL, NULL);
    Tcl_CreateObjCommand(interp, "aerotate", aerotate, NULL, NULL);
    Tcl_CreateObjCommand(interp, "reset", zero_view, NULL, NULL);
    Tcl_CreateObjCommand(interp, "position", position, NULL, NULL);
    Tcl_CreateObjCommand(interp, "render_mode", render_mode, NULL, NULL);

    return TCL_OK;
}



/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
