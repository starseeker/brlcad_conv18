/*
 * tkPlatDecls.h --
 *
 *	Declarations of functions in the platform-specific public Tcl API.
 *
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

#ifndef _TKPLATDECLS
#define _TKPLATDECLS

#ifdef BUILD_tk
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif

/*
 * WARNING: This file is automatically generated by the tools/genStubs.tcl
 * script.  Any modifications to the function declarations below should be made
 * in the generic/tk.decls script.
 */

/* !BEGIN!: Do not edit below this line. */

/*
 * Exported function declarations:
 */

#ifdef __WIN32__
/* 0 */
EXTERN Window		Tk_AttachHWND _ANSI_ARGS_((Tk_Window tkwin,
				HWND hwnd));
/* 1 */
EXTERN HINSTANCE	Tk_GetHINSTANCE _ANSI_ARGS_((void));
/* 2 */
EXTERN HWND		Tk_GetHWND _ANSI_ARGS_((Window window));
/* 3 */
EXTERN Tk_Window	Tk_HWNDToWindow _ANSI_ARGS_((HWND hwnd));
/* 4 */
EXTERN void		Tk_PointerEvent _ANSI_ARGS_((HWND hwnd, int x, int y));
/* 5 */
EXTERN int		Tk_TranslateWinEvent _ANSI_ARGS_((HWND hwnd,
				UINT message, WPARAM wParam, LPARAM lParam,
				LRESULT * result));
#endif /* __WIN32__ */
#ifdef MAC_TCL
/* 0 */
EXTERN void		Tk_MacSetEmbedHandler _ANSI_ARGS_((
				Tk_MacEmbedRegisterWinProc * registerWinProcPtr,
				Tk_MacEmbedGetGrafPortProc * getPortProcPtr,
				Tk_MacEmbedMakeContainerExistProc * containerExistProcPtr,
				Tk_MacEmbedGetClipProc * getClipProc,
				Tk_MacEmbedGetOffsetInParentProc * getOffsetProc));
/* 1 */
EXTERN void		Tk_MacTurnOffMenus _ANSI_ARGS_((void));
/* 2 */
EXTERN void		Tk_MacTkOwnsCursor _ANSI_ARGS_((int tkOwnsIt));
/* 3 */
EXTERN void		TkMacInitMenus _ANSI_ARGS_((Tcl_Interp * interp));
/* 4 */
EXTERN void		TkMacInitAppleEvents _ANSI_ARGS_((
				Tcl_Interp * interp));
/* 5 */
EXTERN int		TkMacConvertEvent _ANSI_ARGS_((
				EventRecord * eventPtr));
/* 6 */
EXTERN int		TkMacConvertTkEvent _ANSI_ARGS_((
				EventRecord * eventPtr, Window window));
/* 7 */
EXTERN void		TkGenWMConfigureEvent _ANSI_ARGS_((Tk_Window tkwin,
				int x, int y, int width, int height,
				int flags));
/* 8 */
EXTERN void		TkMacInvalClipRgns _ANSI_ARGS_((TkWindow * winPtr));
/* 9 */
EXTERN int		TkMacHaveAppearance _ANSI_ARGS_((void));
/* 10 */
EXTERN GWorldPtr	TkMacGetDrawablePort _ANSI_ARGS_((Drawable drawable));
#endif /* MAC_TCL */
#ifdef MAC_OSX_TK
/* 0 */
EXTERN void		Tk_MacOSXSetEmbedHandler _ANSI_ARGS_((
				Tk_MacOSXEmbedRegisterWinProc * registerWinProcPtr,
				Tk_MacOSXEmbedGetGrafPortProc * getPortProcPtr,
				Tk_MacOSXEmbedMakeContainerExistProc * containerExistProcPtr,
				Tk_MacOSXEmbedGetClipProc * getClipProc,
				Tk_MacOSXEmbedGetOffsetInParentProc * getOffsetProc));
/* 1 */
EXTERN void		Tk_MacOSXTurnOffMenus _ANSI_ARGS_((void));
/* 2 */
EXTERN void		Tk_MacOSXTkOwnsCursor _ANSI_ARGS_((int tkOwnsIt));
/* 3 */
EXTERN void		TkMacOSXInitMenus _ANSI_ARGS_((Tcl_Interp * interp));
/* 4 */
EXTERN void		TkMacOSXInitAppleEvents _ANSI_ARGS_((
				Tcl_Interp * interp));
/* 5 */
EXTERN void		TkGenWMConfigureEvent _ANSI_ARGS_((Tk_Window tkwin,
				int x, int y, int width, int height,
				int flags));
/* 6 */
EXTERN void		TkMacOSXInvalClipRgns _ANSI_ARGS_((TkWindow * winPtr));
/* 7 */
EXTERN GWorldPtr	TkMacOSXGetDrawablePort _ANSI_ARGS_((
				Drawable drawable));
/* 8 */
EXTERN ControlRef	TkMacOSXGetRootControl _ANSI_ARGS_((
				Drawable drawable));
/* 9 */
EXTERN void		Tk_MacOSXSetupTkNotifier _ANSI_ARGS_((void));
/* 10 */
EXTERN int		Tk_MacOSXIsAppInFront _ANSI_ARGS_((void));
#endif /* MAC_OSX_TK */

typedef struct TkPlatStubs {
    int magic;
    struct TkPlatStubHooks *hooks;

#ifdef __WIN32__
    Window (*tk_AttachHWND) _ANSI_ARGS_((Tk_Window tkwin, HWND hwnd)); /* 0 */
    HINSTANCE (*tk_GetHINSTANCE) _ANSI_ARGS_((void)); /* 1 */
    HWND (*tk_GetHWND) _ANSI_ARGS_((Window window)); /* 2 */
    Tk_Window (*tk_HWNDToWindow) _ANSI_ARGS_((HWND hwnd)); /* 3 */
    void (*tk_PointerEvent) _ANSI_ARGS_((HWND hwnd, int x, int y)); /* 4 */
    int (*tk_TranslateWinEvent) _ANSI_ARGS_((HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT * result)); /* 5 */
#endif /* __WIN32__ */
#ifdef MAC_TCL
    void (*tk_MacSetEmbedHandler) _ANSI_ARGS_((Tk_MacEmbedRegisterWinProc * registerWinProcPtr, Tk_MacEmbedGetGrafPortProc * getPortProcPtr, Tk_MacEmbedMakeContainerExistProc * containerExistProcPtr, Tk_MacEmbedGetClipProc * getClipProc, Tk_MacEmbedGetOffsetInParentProc * getOffsetProc)); /* 0 */
    void (*tk_MacTurnOffMenus) _ANSI_ARGS_((void)); /* 1 */
    void (*tk_MacTkOwnsCursor) _ANSI_ARGS_((int tkOwnsIt)); /* 2 */
    void (*tkMacInitMenus) _ANSI_ARGS_((Tcl_Interp * interp)); /* 3 */
    void (*tkMacInitAppleEvents) _ANSI_ARGS_((Tcl_Interp * interp)); /* 4 */
    int (*tkMacConvertEvent) _ANSI_ARGS_((EventRecord * eventPtr)); /* 5 */
    int (*tkMacConvertTkEvent) _ANSI_ARGS_((EventRecord * eventPtr, Window window)); /* 6 */
    void (*tkGenWMConfigureEvent) _ANSI_ARGS_((Tk_Window tkwin, int x, int y, int width, int height, int flags)); /* 7 */
    void (*tkMacInvalClipRgns) _ANSI_ARGS_((TkWindow * winPtr)); /* 8 */
    int (*tkMacHaveAppearance) _ANSI_ARGS_((void)); /* 9 */
    GWorldPtr (*tkMacGetDrawablePort) _ANSI_ARGS_((Drawable drawable)); /* 10 */
#endif /* MAC_TCL */
#ifdef MAC_OSX_TK
    void (*tk_MacOSXSetEmbedHandler) _ANSI_ARGS_((Tk_MacOSXEmbedRegisterWinProc * registerWinProcPtr, Tk_MacOSXEmbedGetGrafPortProc * getPortProcPtr, Tk_MacOSXEmbedMakeContainerExistProc * containerExistProcPtr, Tk_MacOSXEmbedGetClipProc * getClipProc, Tk_MacOSXEmbedGetOffsetInParentProc * getOffsetProc)); /* 0 */
    void (*tk_MacOSXTurnOffMenus) _ANSI_ARGS_((void)); /* 1 */
    void (*tk_MacOSXTkOwnsCursor) _ANSI_ARGS_((int tkOwnsIt)); /* 2 */
    void (*tkMacOSXInitMenus) _ANSI_ARGS_((Tcl_Interp * interp)); /* 3 */
    void (*tkMacOSXInitAppleEvents) _ANSI_ARGS_((Tcl_Interp * interp)); /* 4 */
    void (*tkGenWMConfigureEvent) _ANSI_ARGS_((Tk_Window tkwin, int x, int y, int width, int height, int flags)); /* 5 */
    void (*tkMacOSXInvalClipRgns) _ANSI_ARGS_((TkWindow * winPtr)); /* 6 */
    GWorldPtr (*tkMacOSXGetDrawablePort) _ANSI_ARGS_((Drawable drawable)); /* 7 */
    ControlRef (*tkMacOSXGetRootControl) _ANSI_ARGS_((Drawable drawable)); /* 8 */
    void (*tk_MacOSXSetupTkNotifier) _ANSI_ARGS_((void)); /* 9 */
    int (*tk_MacOSXIsAppInFront) _ANSI_ARGS_((void)); /* 10 */
#endif /* MAC_OSX_TK */
} TkPlatStubs;

#ifdef __cplusplus
extern "C" {
#endif
extern TkPlatStubs *tkPlatStubsPtr;
#ifdef __cplusplus
}
#endif

#if defined(USE_TK_STUBS) && !defined(USE_TK_STUB_PROCS)

/*
 * Inline function declarations:
 */

#ifdef __WIN32__
#ifndef Tk_AttachHWND
#define Tk_AttachHWND \
	(tkPlatStubsPtr->tk_AttachHWND) /* 0 */
#endif
#ifndef Tk_GetHINSTANCE
#define Tk_GetHINSTANCE \
	(tkPlatStubsPtr->tk_GetHINSTANCE) /* 1 */
#endif
#ifndef Tk_GetHWND
#define Tk_GetHWND \
	(tkPlatStubsPtr->tk_GetHWND) /* 2 */
#endif
#ifndef Tk_HWNDToWindow
#define Tk_HWNDToWindow \
	(tkPlatStubsPtr->tk_HWNDToWindow) /* 3 */
#endif
#ifndef Tk_PointerEvent
#define Tk_PointerEvent \
	(tkPlatStubsPtr->tk_PointerEvent) /* 4 */
#endif
#ifndef Tk_TranslateWinEvent
#define Tk_TranslateWinEvent \
	(tkPlatStubsPtr->tk_TranslateWinEvent) /* 5 */
#endif
#endif /* __WIN32__ */
#ifdef MAC_TCL
#ifndef Tk_MacSetEmbedHandler
#define Tk_MacSetEmbedHandler \
	(tkPlatStubsPtr->tk_MacSetEmbedHandler) /* 0 */
#endif
#ifndef Tk_MacTurnOffMenus
#define Tk_MacTurnOffMenus \
	(tkPlatStubsPtr->tk_MacTurnOffMenus) /* 1 */
#endif
#ifndef Tk_MacTkOwnsCursor
#define Tk_MacTkOwnsCursor \
	(tkPlatStubsPtr->tk_MacTkOwnsCursor) /* 2 */
#endif
#ifndef TkMacInitMenus
#define TkMacInitMenus \
	(tkPlatStubsPtr->tkMacInitMenus) /* 3 */
#endif
#ifndef TkMacInitAppleEvents
#define TkMacInitAppleEvents \
	(tkPlatStubsPtr->tkMacInitAppleEvents) /* 4 */
#endif
#ifndef TkMacConvertEvent
#define TkMacConvertEvent \
	(tkPlatStubsPtr->tkMacConvertEvent) /* 5 */
#endif
#ifndef TkMacConvertTkEvent
#define TkMacConvertTkEvent \
	(tkPlatStubsPtr->tkMacConvertTkEvent) /* 6 */
#endif
#ifndef TkGenWMConfigureEvent
#define TkGenWMConfigureEvent \
	(tkPlatStubsPtr->tkGenWMConfigureEvent) /* 7 */
#endif
#ifndef TkMacInvalClipRgns
#define TkMacInvalClipRgns \
	(tkPlatStubsPtr->tkMacInvalClipRgns) /* 8 */
#endif
#ifndef TkMacHaveAppearance
#define TkMacHaveAppearance \
	(tkPlatStubsPtr->tkMacHaveAppearance) /* 9 */
#endif
#ifndef TkMacGetDrawablePort
#define TkMacGetDrawablePort \
	(tkPlatStubsPtr->tkMacGetDrawablePort) /* 10 */
#endif
#endif /* MAC_TCL */
#ifdef MAC_OSX_TK
#ifndef Tk_MacOSXSetEmbedHandler
#define Tk_MacOSXSetEmbedHandler \
	(tkPlatStubsPtr->tk_MacOSXSetEmbedHandler) /* 0 */
#endif
#ifndef Tk_MacOSXTurnOffMenus
#define Tk_MacOSXTurnOffMenus \
	(tkPlatStubsPtr->tk_MacOSXTurnOffMenus) /* 1 */
#endif
#ifndef Tk_MacOSXTkOwnsCursor
#define Tk_MacOSXTkOwnsCursor \
	(tkPlatStubsPtr->tk_MacOSXTkOwnsCursor) /* 2 */
#endif
#ifndef TkMacOSXInitMenus
#define TkMacOSXInitMenus \
	(tkPlatStubsPtr->tkMacOSXInitMenus) /* 3 */
#endif
#ifndef TkMacOSXInitAppleEvents
#define TkMacOSXInitAppleEvents \
	(tkPlatStubsPtr->tkMacOSXInitAppleEvents) /* 4 */
#endif
#ifndef TkGenWMConfigureEvent
#define TkGenWMConfigureEvent \
	(tkPlatStubsPtr->tkGenWMConfigureEvent) /* 5 */
#endif
#ifndef TkMacOSXInvalClipRgns
#define TkMacOSXInvalClipRgns \
	(tkPlatStubsPtr->tkMacOSXInvalClipRgns) /* 6 */
#endif
#ifndef TkMacOSXGetDrawablePort
#define TkMacOSXGetDrawablePort \
	(tkPlatStubsPtr->tkMacOSXGetDrawablePort) /* 7 */
#endif
#ifndef TkMacOSXGetRootControl
#define TkMacOSXGetRootControl \
	(tkPlatStubsPtr->tkMacOSXGetRootControl) /* 8 */
#endif
#ifndef Tk_MacOSXSetupTkNotifier
#define Tk_MacOSXSetupTkNotifier \
	(tkPlatStubsPtr->tk_MacOSXSetupTkNotifier) /* 9 */
#endif
#ifndef Tk_MacOSXIsAppInFront
#define Tk_MacOSXIsAppInFront \
	(tkPlatStubsPtr->tk_MacOSXIsAppInFront) /* 10 */
#endif
#endif /* MAC_OSX_TK */

#endif /* defined(USE_TK_STUBS) && !defined(USE_TK_STUB_PROCS) */

/* !END!: Do not edit above this line. */

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

#endif /* _TKPLATDECLS */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
