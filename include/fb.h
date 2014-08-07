/*                            F B . H
 * BRL-CAD
 *
 * Copyright (c) 2004-2014 United States Government as represented by
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
/** @addtogroup libfb */
/** @{ */
/** @file fb.h
 *
 * "Generic" Framebuffer Library Interface Defines.
 *
 */

#ifndef FB_H
#define FB_H

#include "common.h"

#include <limits.h>  /* For INT_MAX */
#include <stdlib.h>

/*
 * Needed for fd_set, avoid including sys/select.h outright since it
 * conflicts on some systems (e.g. freebsd4).
 *
 * FIXME: would be nice to decouple this interface from fd_set as it's
 * only used in one place right now.
 */
#if defined(HAVE_SYS_TYPES_H)
#  include <sys/types.h>
#endif
#if defined(HAVE_SYS_TIME_H)
#  include <sys/time.h>
#endif

#ifndef FB_EXPORT
#  if defined(FB_DLL_EXPORTS) && defined(FB_DLL_IMPORTS)
#    error "Only FB_DLL_EXPORTS or FB_DLL_IMPORTS can be defined, not both."
#  elif defined(FB_DLL_EXPORTS)
#    define FB_EXPORT __declspec(dllexport)
#  elif defined(FB_DLL_IMPORTS)
#    define FB_EXPORT __declspec(dllimport)
#  else
#    define FB_EXPORT
#  endif
#endif

#include "bu/bu_tcl.h"
#include "bu/magic.h"
#include "bu/vls.h"

/**
 * Format of disk pixels is .pix raw image files.  Formerly used as
 * arguments to many of the library routines, but has fallen into
 * disuse due to the difficulties with ANSI function prototypes, and
 * the fact that arrays are not real types in C.  The most notable
 * problem is that of passing a pointer to an array of RGBpixel.  It
 * looks doubly dimensioned, but isn't.
 */
typedef unsigned char RGBpixel[3];


/**
 * These generic color maps have up to 16 bits of significance,
 * left-justified in a short.  Think of this as fixed-point values
 * from 0 to 1.
 */
/* FIXME: ColorMap is same as RLEColorMap defined in 'orle.h' */
typedef struct {
    unsigned short cm_red[256];
    unsigned short cm_green[256];
    unsigned short cm_blue[256];
} ColorMap;


#define PIXEL_NULL (unsigned char *) 0
#define RGBPIXEL_NULL (unsigned char *) 0
#define COLORMAP_NULL (ColorMap *) 0

/* Use a typedef to hide the details of the framebuffer structure */
typedef struct fb fb_s;
#define FB_NULL (fb_s *) 0

/**
 *@brief
 * A frame-buffer IO structure.
 *
 * One of these is allocated for each active framebuffer.  A pointer
 * to this structure is the first argument to all the library
 * routines.  TODO - see if this can move to a private header.
 */
struct fb {
    uint32_t if_magic;
    /* Static information: per device TYPE.	*/
    int (*if_open)(fb_s *ifp, const char *file, int _width, int _height);			/**< @brief open device */
    int (*if_close)(fb_s *ifp);									/**< @brief close device */
    int (*if_clear)(fb_s *ifp, unsigned char *pp);						/**< @brief clear device */
    ssize_t (*if_read)(fb_s *ifp, int x, int y, unsigned char *pp, size_t count);		/**< @brief read pixels */
    ssize_t (*if_write)(fb_s *ifp, int x, int y, const unsigned char *pp, size_t count);	/**< @brief write pixels */
    int (*if_rmap)(fb_s *ifp, ColorMap *cmap);							/**< @brief read colormap */
    int (*if_wmap)(fb_s *ifp, const ColorMap *cmap);						/**< @brief write colormap */
    int (*if_view)(fb_s *ifp, int xcent, int ycent, int xzoom, int yzoom);			/**< @brief set view */
    int (*if_getview)(fb_s *ifp, int *xcent, int *ycent, int *xzoom, int *yzoom);		/**< @brief get view */
    int (*if_setcursor)(fb_s *ifp, const unsigned char *bits, int xb, int yb, int xo, int yo);	/**< @brief define cursor */
    int (*if_cursor)(fb_s *ifp, int mode, int x, int y);					/**< @brief set cursor */
    int (*if_getcursor)(fb_s *ifp, int *mode, int *x, int *y);					/**< @brief get cursor */
    int (*if_readrect)(fb_s *ifp, int xmin, int ymin, int _width, int _height, unsigned char *pp);		/**< @brief read rectangle */
    int (*if_writerect)(fb_s *ifp, int xmin, int ymin, int _width, int _height, const unsigned char *pp);	/**< @brief write rectangle */
    int (*if_bwreadrect)(fb_s *ifp, int xmin, int ymin, int _width, int _height, unsigned char *pp);		/**< @brief read monochrome rectangle */
    int (*if_bwwriterect)(fb_s *ifp, int xmin, int ymin, int _width, int _height, const unsigned char *pp);	/**< @brief write rectangle */
    int (*if_poll)(fb_s *ifp);		/**< @brief handle events */
    int (*if_flush)(fb_s *ifp);		/**< @brief flush output */
    int (*if_free)(fb_s *ifp);		/**< @brief free resources */
    int (*if_help)(fb_s *ifp);		/**< @brief print useful info */
    char *if_type;	/**< @brief what "open" calls it */
    int if_max_width;	/**< @brief max device width */
    int if_max_height;	/**< @brief max device height */
    /* Dynamic information: per device INSTANCE. */
    char *if_name;	/**< @brief what the user called it */
    int if_width;	/**< @brief current values */
    int if_height;
    int if_selfd;	/**< @brief select(fd) for input events if >= 0 */
    /* Internal information: needed by the library.	*/
    int if_fd;		/**< @brief internal file descriptor */
    int if_xzoom;	/**< @brief zoom factors */
    int if_yzoom;
    int if_xcenter;	/**< @brief pan position */
    int if_ycenter;
    int if_cursmode;	/**< @brief cursor on/off */
    int if_xcurs;	/**< @brief cursor position */
    int if_ycurs;
    unsigned char *if_pbase;/**< @brief Address of malloc()ed page buffer.	*/
    unsigned char *if_pcurp;/**< @brief Current pointer into page buffer.	*/
    unsigned char *if_pendp;/**< @brief End of page buffer.			*/
    int if_pno;		/**< @brief Current "page" in memory.		*/
    int if_pdirty;	/**< @brief Page modified flag.			*/
    long if_pixcur;	/**< @brief Current pixel number in framebuffer. */
    long if_ppixels;	/**< @brief Sizeof page buffer (pixels).		*/
    int if_debug;	/**< @brief Buffered IO debug flag.		*/
    /* State variables for individual interface modules */
    union {
	char *p;
	size_t l;
    } u1, u2, u3, u4, u5, u6;
};

/**
 * assert the integrity of an FBIO struct.
 */
#define FB_CK_FB(_p) BU_CKMAG(_p, FB_MAGIC, "FB")

/* declare all the possible interfaces */
#ifdef IF_REMOTE
FB_EXPORT extern fb_s remote_interface;	/* not in list[] */
#endif

#ifdef IF_OGL
FB_EXPORT extern fb_s ogl_interface;
#endif

#ifdef IF_WGL
FB_EXPORT extern fb_s wgl_interface;
#endif

#ifdef IF_X
FB_EXPORT extern fb_s X24_interface;
FB_EXPORT extern fb_s X_interface;
#endif

#ifdef IF_TK
FB_EXPORT extern fb_s tk_interface;
#endif

#ifdef IF_QT
FB_EXPORT extern fb_s qt_interface;
#endif

/* Always included */
FB_EXPORT extern fb_s debug_interface, disk_interface, stk_interface;
FB_EXPORT extern fb_s memory_interface, null_interface;


/* Library entry points which are macros.
 *
 * FIXME: turn these into proper functions so we can appropriately
 * avoid dereferencing a NULL _ifp or calling an invalid callback.
 */
#define fb_gettype(_ifp)		(_ifp->if_type)
#define fb_getwidth(_ifp)		(_ifp->if_width)
#define fb_getheight(_ifp)		(_ifp->if_height)
#define fb_poll(_ifp)			(*_ifp->if_poll)(_ifp)
#define fb_help(_ifp)			(*_ifp->if_help)(_ifp)
#define fb_free(_ifp)			(*_ifp->if_free)(_ifp)
#define fb_clear(_ifp, _pp)		(*_ifp->if_clear)(_ifp, _pp)
#define fb_read(_ifp, _x, _y, _pp, _ct)		(*_ifp->if_read)(_ifp, _x, _y, _pp, _ct)
#define fb_write(_ifp, _x, _y, _pp, _ct)	(*_ifp->if_write)(_ifp, _x, _y, _pp, _ct)
#define fb_rmap(_ifp, _cmap)			(*_ifp->if_rmap)(_ifp, _cmap)
#define fb_wmap(_ifp, _cmap)			(*_ifp->if_wmap)(_ifp, _cmap)
#define fb_view(_ifp, _xc, _yc, _xz, _yz)	(*_ifp->if_view)(_ifp, _xc, _yc, _xz, _yz)
#define fb_getview(_ifp, _xcp, _ycp, _xzp, _yzp)	(*_ifp->if_getview)(_ifp, _xcp, _ycp, _xzp, _yzp)
#define fb_setcursor(_ifp, _bits, _xb, _yb, _xo, _yo) 	(*_ifp->if_setcursor)(_ifp, _bits, _xb, _yb, _xo, _yo)
#define fb_cursor(_ifp, _mode, _x, _y)			(*_ifp->if_cursor)(_ifp, _mode, _x, _y)
#define fb_getcursor(_ifp, _modep, _xp, _yp)		(*_ifp->if_getcursor)(_ifp, _modep, _xp, _yp)
#define fb_readrect(_ifp, _xmin, _ymin, _width, _height, _pp)		(*_ifp->if_readrect)(_ifp, _xmin, _ymin, _width, _height, _pp)
#define fb_writerect(_ifp, _xmin, _ymin, _width, _height, _pp)		(*_ifp->if_writerect)(_ifp, _xmin, _ymin, _width, _height, _pp)
#define fb_bwreadrect(_ifp, _xmin, _ymin, _width, _height, _pp) 	(*_ifp->if_bwreadrect)(_ifp, _xmin, _ymin, _width, _height, _pp)
#define fb_bwwriterect(_ifp, _xmin, _ymin, _width, _height, _pp)	(*_ifp->if_bwwriterect)(_ifp, _xmin, _ymin, _width, _height, _pp)

__BEGIN_DECLS

/* Library entry points which are true functions. */
FB_EXPORT extern void fb_configureWindow(fb_s *, int, int);
FB_EXPORT extern fb_s *fb_open(const char *file, int _width, int _height);
FB_EXPORT extern int fb_close(fb_s *ifp);
FB_EXPORT extern int fb_close_existing(fb_s *ifp);
FB_EXPORT extern int fb_genhelp(void);
FB_EXPORT extern int fb_ioinit(fb_s *ifp);
FB_EXPORT extern int fb_seek(fb_s *ifp, int x, int y);
FB_EXPORT extern int fb_tell(fb_s *ifp, int *xp, int *yp);
FB_EXPORT extern int fb_rpixel(fb_s *ifp, unsigned char *pp);
FB_EXPORT extern int fb_wpixel(fb_s *ifp, unsigned char *pp);
FB_EXPORT extern int fb_flush(fb_s *ifp);
#if !defined(_WIN32) || defined(__CYGWIN__)
FB_EXPORT extern void fb_log(const char *fmt, ...) _BU_ATTR_PRINTF12;
#endif
FB_EXPORT extern int fb_null(fb_s *ifp);
FB_EXPORT extern int fb_null_setcursor(fb_s *ifp, const unsigned char *bits, int xbits, int ybits, int xorig, int yorig);

/* utility functions */
FB_EXPORT extern int fb_common_file_size(size_t *widthp, size_t *heightp, const char *filename, int pixel_size);
FB_EXPORT extern int fb_common_image_size(size_t *widthp, size_t *heightp, size_t npixels);
FB_EXPORT extern int fb_common_name_size(size_t *widthp, size_t *heightp, const char *name);
FB_EXPORT extern int fb_write_fp(fb_s *ifp, FILE *fp, int req_width, int req_height, int crunch, int inverse, struct bu_vls *result);
FB_EXPORT extern int fb_read_fd(fb_s *ifp, int fd,  int file_width, int file_height, int file_xoff, int file_yoff, int scr_width, int scr_height, int scr_xoff, int scr_yoff, int fileinput, char *file_name, int one_line_only, int multiple_lines, int autosize, int inverse, int clear, int zoom, struct bu_vls *result);

/* color mapping */
FB_EXPORT extern int fb_is_linear_cmap(const ColorMap *cmap);
FB_EXPORT extern void fb_make_linear_cmap(ColorMap *cmap);

/* backward compatibility hacks */
FB_EXPORT extern int fb_reset(fb_s *ifp);
FB_EXPORT extern int fb_viewport(fb_s *ifp, int left, int top, int right, int bottom);
FB_EXPORT extern int fb_window(fb_s *ifp, int xcenter, int ycenter);
FB_EXPORT extern int fb_zoom(fb_s *ifp, int xzoom, int yzoom);
FB_EXPORT extern int fb_scursor(fb_s *ifp, int mode, int x, int y);

/*
 * Some functions and variables we couldn't hide.
 * Not for general consumption.
 */
FB_EXPORT extern int _fb_disk_enable;
FB_EXPORT extern int fb_sim_readrect(fb_s *ifp, int xmin, int ymin, int _width, int _height, unsigned char *pp);
FB_EXPORT extern int fb_sim_writerect(fb_s *ifp, int xmin, int ymin, int _width, int _height, const unsigned char *pp);
FB_EXPORT extern int fb_sim_bwreadrect(fb_s *ifp, int xmin, int ymin, int _width, int _height, unsigned char *pp);
FB_EXPORT extern int fb_sim_bwwriterect(fb_s *ifp, int xmin, int ymin, int _width, int _height, const unsigned char *pp);
FB_EXPORT extern int fb_sim_view(fb_s *ifp, int xcenter, int ycenter, int xzoom, int yzoom);
FB_EXPORT extern int fb_sim_getview(fb_s *ifp, int *xcenter, int *ycenter, int *xzoom, int *yzoom);
FB_EXPORT extern int fb_sim_cursor(fb_s *ifp, int mode, int x, int y);
FB_EXPORT extern int fb_sim_getcursor(fb_s *ifp, int *mode, int *x, int *y);

/* FIXME:  these IF_* sections need to die.  they don't belong in a public header. */

#ifdef IF_X
#  ifdef HAVE_X11_XLIB_H
#    include <X11/Xlib.h>
#    include <X11/Xutil.h>
#  endif
FB_EXPORT extern int _X24_open_existing(fb_s *ifp, Display *dpy, Window win, Window cwinp, Colormap cmap, XVisualInfo *vip, int width, int height, GC gc);
#endif

#ifdef IF_OGL
#  ifdef HAVE_X11_XLIB_H
#    include <X11/Xlib.h>
#    include <X11/Xutil.h>
#  endif
/* glx.h on Mac OS X (and perhaps elsewhere) defines a slew of
 * parameter names that shadow system symbols.  protect the system
 * symbols by redefining the parameters prior to header inclusion.
 */
#  define j1 J1
#  define y1 Y1
#  define read rd
#  define index idx
#  define access acs
#  define remainder rem
#  ifdef HAVE_GL_GLX_H
#    include <GL/glx.h>
#  endif
#  undef remainder
#  undef access
#  undef index
#  undef read
#  undef y1
#  undef j1
#  ifdef HAVE_GL_GL_H
#    include <GL/gl.h>
#  endif
FB_EXPORT extern int _ogl_open_existing(fb_s *ifp, Display *dpy, Window win, Colormap cmap, XVisualInfo *vip, int width, int height, GLXContext glxc, int double_buffer, int soft_cmap);
#endif

#ifdef IF_WGL
#  include <windows.h>
#  include <tk.h>
#  ifdef HAVE_GL_GL_H
#    include <GL/gl.h>
#  endif
FB_EXPORT extern int _wgl_open_existing(fb_s *ifp, Display *dpy, Window win, Colormap cmap, PIXELFORMATDESCRIPTOR *vip, HDC hdc, int width, int height, HGLRC glxc, int double_buffer, int soft_cmap);
#endif

#ifdef IF_QT
FB_EXPORT extern int _qt_open_existing(fb_s *ifp, int width, int height, void *qapp, void *qwin, void *qpainter, void *draw, void **qimg);
#endif

/*
 * Copy one RGB pixel to another.
 */
#define COPYRGB(to, from) { (to)[RED]=(from)[RED];\
			   (to)[GRN]=(from)[GRN];\
			   (to)[BLU]=(from)[BLU]; }

/* Debug Bitvector Definition */
#define FB_DEBUG_BIO 1	/* Buffered io calls (less r/wpixel) */
#define FB_DEBUG_CMAP 2	/* Contents of colormaps */
#define FB_DEBUG_RW 4	/* Contents of reads and writes */
#define FB_DEBUG_BRW 8	/* Buffered IO rpixel and wpixel */

/* tcl.c */
/* The presence of Tcl_Interp as an arg prevents giving arg list */
FB_EXPORT extern void fb_tcl_setup(void);
FB_EXPORT extern int Fb_Init(Tcl_Interp *interp);
FB_EXPORT extern int fb_refresh(fb_s *ifp, int x, int y, int w, int h);


/**
 * report version information about LIBFB
 */
FB_EXPORT extern const char *fb_version(void);

__END_DECLS

#endif /* FB_H */

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
