/*                         I F _ T O G L . C
 * BRL-CAD
 *
 * Copyright (c) 2007-2010 United States Government as represented by
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
/** @addtogroup if */
/** @{ */
/** @file if_togl.c
 *
 * Togl libfb interface.
 *
 */
/** @} */

#include "common.h"

#ifdef IF_TOGL

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif

#include "fb.h"
#include "tk.h"

#define USE_TOGL_STUBS

#include "togl.h"

struct togl_fb {
   Tcl_Interp *toglfbinterp;
   Tk_Window toglfbwin;
   Togl *fbtogl;
   int texid;
   void *texdata;
};

HIDDEN int fb_togl_open(FBIO *ifp, char *file, int width, int height),
    fb_togl_close(FBIO *ifp),
    togl_clear(FBIO *ifp, unsigned char *pp),
    togl_read(FBIO *ifp, int x, int y, unsigned char *pixelp, size_t count),
    togl_write(FBIO *ifp, int x, int y, const unsigned char *pixelp, size_t count),
    togl_rmap(FBIO *ifp, ColorMap *cmp),
    togl_wmap(FBIO *ifp, const ColorMap *cmp),
    togl_view(FBIO *ifp, int xcenter, int ycenter, int xzoom, int yzoom),
    togl_getview(FBIO *ifp, int *xcenter, int *ycenter, int *xzoom, int *yzoom),
    togl_setcursor(FBIO *ifp, const unsigned char *bits, int xbits, int ybits, int xorig, int yorig),
    togl_cursor(FBIO *ifp, int mode, int x, int y),
    togl_getcursor(FBIO *ifp, int *mode, int *x, int *y),
    togl_readrect(FBIO *ifp, int xmin, int ymin, int width, int height, unsigned char *pp),
    togl_writerect(FBIO *ifp, int xmin, int ymin, int width, int height, const unsigned char *pp),
    togl_bwreadrect(FBIO *ifp, int xmin, int ymin, int width, int height, unsigned char *pp),
    togl_bwwriterect(FBIO *ifp, int xmin, int ymin, int width, int height, const unsigned char *pp),
    togl_poll(FBIO *ifp),
    togl_flush(FBIO *ifp),
    togl_free(FBIO *ifp),
    togl_help(FBIO *ifp);

/* This is the ONLY thing that we "export" */
FBIO togl_interface = {
    0,
    fb_togl_open,
    fb_togl_close,
    togl_clear,
    togl_read,
    togl_write,
    togl_rmap,
    togl_wmap,
    togl_view,
    togl_getview,
    togl_setcursor,
    togl_cursor,
    togl_getcursor,
    togl_readrect,
    togl_writerect,
    togl_bwreadrect,
    togl_bwwriterect,
    togl_poll,
    togl_flush,
    togl_free,
    togl_help,
    "Debugging Interface",
    32*1024,		/* max width */
    32*1024,		/* max height */
    "/dev/togl",
    512,			/* current/default width */
    512,			/* current/default height */
    -1,			/* select fd */
    -1,			/* file descriptor */
    1, 1,			/* zoom */
    256, 256,		/* window center */
    0, 0, 0,		/* cursor */
    PIXEL_NULL,		/* page_base */
    PIXEL_NULL,		/* page_curp */
    PIXEL_NULL,		/* page_endp */
    -1,			/* page_no */
    0,			/* page_ref */
    0L,			/* page_curpos */
    0L,			/* page_pixels */
    0,			/* debug */
    {0}, /* u1 */
    {0}, /* u2 */
    {0}, /* u3 */
    {0}, /* u4 */
    {0}, /* u5 */
    {0}  /* u6 */
};

#define TOGLFB(ptr) ((struct togl_fb *)((ptr)->u6.p))
#define TOGLFBL(ptr) ((ptr)->u6.p) /* left hand side version */

void togl_configureWindow(FBIO *ifp, int width, int height) {
    printf("width: %d height: %d\n",  width, height);
    ifp->if_width = width;
    ifp->if_height = height;
    Togl_MakeCurrent(TOGLFB(ifp)->fbtogl);
    glBindTexture (GL_TEXTURE_2D, TOGLFB(ifp)->texid);
    TOGLFB(ifp)->texdata = realloc(TOGLFB(ifp)->texdata, width * height * 3);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, TOGLFB(ifp)->texdata);
    Togl_SwapBuffers(TOGLFB(ifp)->fbtogl);
    return 0;
}

int
togl_refresh(FBIO *ifp, int x, int y, int w, int h)
{
    int mm;

    glGetIntegerv(GL_MATRIX_MODE, &mm);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glViewport(0, 0, Togl_Width(TOGLFB(ifp)->fbtogl), Togl_Height(TOGLFB(ifp)->fbtogl));
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(mm);

    glFlush();
    return 0;
}

int
_togl_open_existing(FBIO *ifp, Togl *togl) {
    char *tclcmd[255] = {'\0'};

    FB_CK_FBIO(ifp);
    /* Set up the togl_fb structure */
    if ((TOGLFBL(ifp) = (char *)calloc(1, sizeof(struct togl_fb))) == NULL) {
        fb_log("fb_togl_open:  togl_fb malloc failed\n");
        return -1;
    }

    ifp->if_width = Togl_Width(togl);
    ifp->if_height = Togl_Height(togl);

    TOGLFB(ifp)->toglfbinterp = Togl_Interp(togl);
    TOGLFB(ifp)->toglfbwin = Tk_Parent(Togl_TkWin(togl));
    TOGLFB(ifp)->fbtogl = togl;  
    Togl_MakeCurrent(togl);
    glClearColor (0.0, 0, 0.0, 1);
    glBindTexture (GL_TEXTURE_2D, TOGLFB(ifp)->texid);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    TOGLFB(ifp)->texdata = realloc(TOGLFB(ifp)->texdata, ifp->if_width * ifp->if_height * 3);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, ifp->if_width, ifp->if_height, 0, GL_RGB, GL_UNSIGNED_BYTE, TOGLFB(ifp)->texdata);
    return 0;
}

int
togl_open_existing(FBIO *ifp, int argc, char **argv)
{
    Togl *togl;

    if (argc != 10)
        return -1;

    if (sscanf(argv[1], "%p", (void *)&togl) != 1)
        return -1;

    return _togl_open_existing(ifp, togl);
}


HIDDEN int
fb_togl_open(FBIO *ifp, char *file, int width, int height)
{
    Togl *togl;
    char *tclcmd[255] = {'\0'};

    FB_CK_FBIO(ifp);
    if (file == (char *)NULL)
	fb_log("fb_open(0x%lx, NULL, %d, %d)\n",
	       (unsigned long)ifp, width, height);
    else
	fb_log("fb_open(0x%lx, \"%s\", %d, %d)\n",
	       (unsigned long)ifp, file, width, height);

    /* Set up the togl_fb structure */
    if ((TOGLFBL(ifp) = (char *)calloc(1, sizeof(struct togl_fb))) == NULL) {
        fb_log("fb_togl_open:  togl_fb malloc failed\n");
        return -1;
    }

    /* check for default size */
    if (width <= 0)
	width = ifp->if_width;
    if (height <= 0)
	height = ifp->if_height;

    /* set debug bit vector */
    if (file != NULL) {
	char *cp;
	for (cp = file; *cp != '\0' && !isdigit(*cp); cp++)
	    ;
	sscanf(cp, "%d", &ifp->if_debug);
    } else {
	ifp->if_debug = 0;
    }

    /* Give the user whatever width was asked for */
    ifp->if_width = width;
    ifp->if_height = height;

    TOGLFB(ifp)->toglfbinterp = Tcl_CreateInterp();

    if (Tcl_Init(TOGLFB(ifp)->toglfbinterp) == TCL_ERROR) {
	fb_log("Tcl_Init returned error in fb_open.");
    }

    sprintf(tclcmd, "package require Tk");

    if (Tcl_Eval(TOGLFB(ifp)->toglfbinterp, tclcmd) != TCL_OK) {
	fb_log("Error returned attempting to start tk in fb_open.");
    }

    if (Tcl_InitStubs(TOGLFB(ifp)->toglfbinterp, "8.1", 0) == NULL
        || Togl_InitStubs(TOGLFB(ifp)->toglfbinterp, "2.0", 0) == NULL) {
	fb_log("Error during Togl init\n");
    } 

    TOGLFB(ifp)->toglfbwin = Tk_MainWindow(TOGLFB(ifp)->toglfbinterp);

    Tk_GeometryRequest(TOGLFB(ifp)->toglfbwin, width, height);

    Tk_MakeWindowExist(TOGLFB(ifp)->toglfbwin);

    Tk_MapWindow(TOGLFB(ifp)->toglfbwin);
    
    sprintf(tclcmd, "package require Togl; togl %stogl -width %d -height %d -rgba true -double true", (char *)Tk_PathName(TOGLFB(ifp)->toglfbwin), width, height);

    printf("tclcmd: %s\n", tclcmd);
    if (Tcl_Eval(TOGLFB(ifp)->toglfbinterp, tclcmd) != TCL_OK) {
	fb_log("Error returned attempting to create togl window in fb_open.");
    }

    sprintf(tclcmd, "pack %stogl -expand true -fill both; update", (char *)Tk_PathName(TOGLFB(ifp)->toglfbwin));

    printf("tclcmd: %s\n", tclcmd);
    if (Tcl_Eval(TOGLFB(ifp)->toglfbinterp, tclcmd) != TCL_OK) {
	fb_log("Error returned attemping to pack togl window in fb_open.");
    }

    sprintf(tclcmd, "%stogl", (char *)Tk_PathName(TOGLFB(ifp)->toglfbwin));
    printf("tclcmd: %s\n", tclcmd);
    Togl_GetToglFromObj(TOGLFB(ifp)->toglfbinterp, Tcl_NewStringObj(tclcmd, -1), &togl);
    TOGLFB(ifp)->fbtogl = togl;

    /* Set our Tcl variable pertaining to whether a
     * window closing event has been seen from the
     * Window manager.  WM_DELETE_WINDOW will be
     * bound to a command setting this variable to
     * the string "close", and a vwait watching
     * for a change to the CloseWindow variable ensures
     * a "lingering" tk window.
     */
    Tcl_SetVar(TOGLFB(ifp)->toglfbinterp, "CloseWindow", "open", 0);
    sprintf(tclcmd, "wm protocol . WM_DELETE_WINDOW {set CloseWindow \"close\"}");
    if (Tcl_Eval(TOGLFB(ifp)->toglfbinterp, tclcmd) != TCL_OK) {
	fb_log("Error binding WM_DELETE_WINDOW.");
    }
    sprintf(tclcmd, "bind . <Button-3> {set CloseWindow \"close\"}");
    if (Tcl_Eval(TOGLFB(ifp)->toglfbinterp, tclcmd) != TCL_OK) {
	fb_log("Error binding right mouse button.");
    }
    while (Tcl_DoOneEvent(TCL_ALL_EVENTS|TCL_DONT_WAIT));

    Togl_MakeCurrent(togl);
    glClearColor (0.0, 0, 0.0, 1);
    glBindTexture (GL_TEXTURE_2D, TOGLFB(ifp)->texid);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    TOGLFB(ifp)->texdata = realloc(TOGLFB(ifp)->texdata, width * height * 3);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, TOGLFB(ifp)->texdata);
    glDisable(GL_LIGHTING);

    glViewport(0,0, width, height);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode (GL_MODELVIEW);

    glClear(GL_COLOR_BUFFER_BIT);

    return 0;
}


HIDDEN int
fb_togl_close(FBIO *ifp)
{
    FB_CK_FBIO(ifp);
    fb_log( "fb_close( 0x%lx )\n", (unsigned long)ifp );
    fclose(stdin);
    /* Wait for CloseWindow to be changed by the WM_DELETE_WINDOW
       binding set up in fb_togl_open */
    Tcl_Eval(TOGLFB(ifp)->toglfbinterp, "vwait CloseWindow");
    if (!strcmp(Tcl_GetVar(TOGLFB(ifp)->toglfbinterp, "CloseWindow", 0),"close")) {
	Tcl_Eval(TOGLFB(ifp)->toglfbinterp, "destroy .");
    }
    return 0;
}


HIDDEN int
togl_clear(FBIO *ifp, unsigned char *pp)
{
    FB_CK_FBIO(ifp);
    if (pp == 0)
	fb_log("fb_clear(0x%lx, NULL)\n", (unsigned long)ifp);
    else
	fb_log("fb_clear(0x%lx, &[%d %d %d])\n",
	       (unsigned long)ifp,
	       (int)(pp[RED]), (int)(pp[GRN]),
	       (int)(pp[BLU]));
    return 0;
}


HIDDEN int
togl_read(FBIO *ifp, int x, int y, unsigned char *pixelp, size_t count)
{
    FB_CK_FBIO(ifp);
    fb_log("fb_read(0x%lx, %4d, %4d, 0x%lx, %ld)\n",
	   (unsigned long)ifp, x, y,
	   (unsigned long)pixelp, (long)count);
    return (int)count;
}


HIDDEN int
togl_write(FBIO *ifp, int xstart, int ystart, const unsigned char *pixelp, size_t count)
{
    printf("writing:  xstart: %d, ystart %d, count: %d\n", xstart, ystart, count);
    Togl *togl;
    togl = TOGLFB(ifp)->fbtogl;
    Togl_MakeCurrent(togl);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, TOGLFB(ifp)->texid);
    glTexSubImage2D(GL_TEXTURE_2D, 0, xstart, ifp->if_height-ystart, count, 1, GL_RGB, GL_UNSIGNED_BYTE, pixelp);
    printf("writing:  if_width: %d, if_height %d\n", ifp->if_width, ifp->if_height);
    glBegin(GL_TRIANGLE_STRIP);

    glTexCoord2d(0, 0); glVertex3f(0, 0, 0);
    glTexCoord2d(0, 1); glVertex3f(0, ifp->if_height, 0);
    glTexCoord2d(1, 0); glVertex3f(ifp->if_width, 0, 0);
    glTexCoord2d(1, 1); glVertex3f(ifp->if_width, ifp->if_height, 0);

    glEnd();

    Togl_SwapBuffers(togl);

    return (int)count;
}


HIDDEN int
togl_rmap(FBIO *ifp, ColorMap *cmp)
{
    FB_CK_FBIO(ifp);
    fb_log("fb_rmap(0x%lx, 0x%lx)\n",
	   (unsigned long)ifp, (unsigned long)cmp);
    return 0;
}


HIDDEN int
togl_wmap(FBIO *ifp, const ColorMap *cmp)
{
    int i;

    FB_CK_FBIO(ifp);
    if (cmp == NULL)
	fb_log("fb_wmap(0x%lx, NULL)\n",
	       (unsigned long)ifp);
    else
	fb_log("fb_wmap(0x%lx, 0x%lx)\n",
	       (unsigned long)ifp, (unsigned long)cmp);

    if (ifp->if_debug & FB_DEBUG_CMAP && cmp != NULL) {
	for (i = 0; i < 256; i++) {
	    fb_log("%3d: [ 0x%4lx, 0x%4lx, 0x%4lx ]\n",
		   i,
		   (unsigned long)cmp->cm_red[i],
		   (unsigned long)cmp->cm_green[i],
		   (unsigned long)cmp->cm_blue[i]);
	}
    }

    return 0;
}


HIDDEN int
togl_view(FBIO *ifp, int xcenter, int ycenter, int xzoom, int yzoom)
{
    FB_CK_FBIO(ifp);
    fb_log("fb_view(0x%lx, %4d, %4d, %4d, %4d)\n",
	   (unsigned long)ifp, xcenter, ycenter, xzoom, yzoom);
    fb_sim_view(ifp, xcenter, ycenter, xzoom, yzoom);
    return 0;
}


HIDDEN int
togl_getview(FBIO *ifp, int *xcenter, int *ycenter, int *xzoom, int *yzoom)
{
    FB_CK_FBIO(ifp);
    fb_log("fb_getview(0x%lx, 0x%x, 0x%x, 0x%x, 0x%x)\n",
	   (unsigned long)ifp, xcenter, ycenter, xzoom, yzoom);
    fb_sim_getview(ifp, xcenter, ycenter, xzoom, yzoom);
    fb_log(" <= %d %d %d %d\n",
	   *xcenter, *ycenter, *xzoom, *yzoom);
    return 0;
}


HIDDEN int
togl_setcursor(FBIO *ifp, const unsigned char *bits, int xbits, int ybits, int xorig, int yorig)
{
    FB_CK_FBIO(ifp);
    fb_log("fb_setcursor(0x%lx, 0x%lx, %d, %d, %d, %d)\n",
	   (unsigned long)ifp, bits, xbits, ybits, xorig, yorig);
    return 0;
}


HIDDEN int
togl_cursor(FBIO *ifp, int mode, int x, int y)
{
    fb_log("fb_cursor(0x%lx, %d, %4d, %4d)\n",
	   (unsigned long)ifp, mode, x, y);
    fb_sim_cursor(ifp, mode, x, y);
    return 0;
}


HIDDEN int
togl_getcursor(FBIO *ifp, int *mode, int *x, int *y)
{
    FB_CK_FBIO(ifp);
    fb_log("fb_getcursor(0x%lx, 0x%x, 0x%x, 0x%x)\n",
	   (unsigned long)ifp, mode, x, y);
    fb_sim_getcursor(ifp, mode, x, y);
    fb_log(" <= %d %d %d\n", *mode, *x, *y);
    return 0;
}


HIDDEN int
togl_readrect(FBIO *ifp, int xmin, int ymin, int width, int height, unsigned char *pp)
{
    FB_CK_FBIO(ifp);
    fb_log("fb_readrect(0x%lx, (%4d, %4d), %4d, %4d, 0x%lx)\n",
	   (unsigned long)ifp, xmin, ymin, width, height,
	   (unsigned long)pp);
    return width*height;
}


HIDDEN int
togl_writerect(FBIO *ifp, int xmin, int ymin, int width, int height, const unsigned char *pp)
{
    FB_CK_FBIO(ifp);
    fb_log("fb_writerect(0x%lx, %4d, %4d, %4d, %4d, 0x%lx)\n",
	   (unsigned long)ifp, xmin, ymin, width, height,
	   (unsigned long)pp);
    return width*height;
}


HIDDEN int
togl_bwreadrect(FBIO *ifp, int xmin, int ymin, int width, int height, unsigned char *pp)
{
    FB_CK_FBIO(ifp);
    fb_log("fb_bwreadrect(0x%lx, (%4d, %4d), %4d, %4d, 0x%lx)\n",
	   (unsigned long)ifp, xmin, ymin, width, height,
	   (unsigned long)pp);
    return width*height;
}


HIDDEN int
togl_bwwriterect(FBIO *ifp, int xmin, int ymin, int width, int height, const unsigned char *pp)
{
    FB_CK_FBIO(ifp);
    fb_log("fb_bwwriterect(0x%lx, %4d, %4d, %4d, %4d, 0x%lx)\n",
	   (unsigned long)ifp, xmin, ymin, width, height,
	   (unsigned long)pp);
    return width*height;
}


HIDDEN int
togl_poll(FBIO *ifp)
{
    FB_CK_FBIO(ifp);
    while (Tcl_DoOneEvent(TCL_ALL_EVENTS|TCL_DONT_WAIT));
    fb_log("fb_poll(0x%lx)\n", (unsigned long)ifp);
    return 0;
}


HIDDEN int
togl_flush(FBIO *ifp)
{
    FB_CK_FBIO(ifp);
    fb_log("if_flush(0x%lx)\n", (unsigned long)ifp);
    return 0;
}


HIDDEN int
togl_free(FBIO *ifp)
{
    FB_CK_FBIO(ifp);
    fb_log("fb_free(0x%lx)\n", (unsigned long)ifp);
    return 0;
}


/*ARGSUSED*/
HIDDEN int
togl_help(FBIO *ifp)
{
    FB_CK_FBIO(ifp);
    fb_log("Description: %s\n", togl_interface.if_type);
    fb_log("Device: %s\n", ifp->if_name);
    fb_log("Max width/height: %d %d\n",
	   togl_interface.if_max_width,
	   togl_interface.if_max_height);
    fb_log("Default width/height: %d %d\n",
	   togl_interface.if_width,
	   togl_interface.if_height);
    fb_log("\
Usage: /dev/togl[#]\n\
  where # is a optional bit vector from:\n\
    1 debug buffered I/O calls\n\
    2 show colormap entries in rmap/wmap calls\n\
    4 show actual pixel values in read/write calls\n");
    /*8 buffered read/write values - ifdef'd out*/

    return 0;
}


#else

/* quell empty-compilation unit warnings */
static const int unused = 0;

#endif /* IF_TOGL */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
