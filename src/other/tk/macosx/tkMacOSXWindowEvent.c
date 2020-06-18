/*
 * tkMacOSXWindowEvent.c --
 *
 *	This file defines the routines for both creating and handling Window
 *	Manager class events for Tk.
 *
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2005-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright (c) 2015 Kevin Walzer/WordTech Communications LLC.
 * Copyright (c) 2015 Marc Culler.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkMacOSXWm.h"
#include "tkMacOSXEvent.h"
#include "tkMacOSXDebug.h"
#include "tkMacOSXConstants.h"

/*
#ifdef TK_MAC_DEBUG
#define TK_MAC_DEBUG_EVENTS
#define TK_MAC_DEBUG_DRAWING
#endif
*/

/*
 * Declaration of functions used only in this file
 */

static int		GenerateUpdates(HIShapeRef updateRgn,
			   CGRect *updateBounds, TkWindow *winPtr);
static int		GenerateActivateEvents(TkWindow *winPtr,
			    int activeFlag);
static void		DoWindowActivate(ClientData clientData);

#pragma mark TKApplication(TKWindowEvent)

#ifdef TK_MAC_DEBUG_NOTIFICATIONS
extern NSString *NSWindowWillOrderOnScreenNotification;
extern NSString *NSWindowDidOrderOnScreenNotification;
extern NSString *NSWindowDidOrderOffScreenNotification;
#endif


@implementation TKApplication(TKWindowEvent)

- (void) windowActivation: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
    BOOL activate = [[notification name]
	    isEqualToString:NSWindowDidBecomeKeyNotification];
    NSWindow *w = [notification object];
    TkWindow *winPtr = TkMacOSXGetTkWindow(w);

    if (winPtr && Tk_IsMapped(winPtr)) {
	GenerateActivateEvents(winPtr, activate);
    }
}

- (void) windowBoundsChanged: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
    BOOL movedOnly = [[notification name]
	    isEqualToString:NSWindowDidMoveNotification];
    NSWindow *w = [notification object];
    TkWindow *winPtr = TkMacOSXGetTkWindow(w);

    if (winPtr) {
	WmInfo *wmPtr = winPtr->wmInfoPtr;
	NSRect bounds = [w frame];
	int x, y, width = -1, height = -1, flags = 0;

	x = bounds.origin.x;
	y = TkMacOSXZeroScreenHeight() - (bounds.origin.y + bounds.size.height);
	if (winPtr->changes.x != x || winPtr->changes.y != y) {
	    flags |= TK_LOCATION_CHANGED;
	} else {
	    x = y = -1;
	}
	if (!movedOnly && (winPtr->changes.width != bounds.size.width ||
		winPtr->changes.height !=  bounds.size.height)) {
	    width = bounds.size.width - wmPtr->xInParent;
	    height = bounds.size.height - wmPtr->yInParent;
	    flags |= TK_SIZE_CHANGED;
	}
	if (Tcl_GetServiceMode() != TCL_SERVICE_NONE) {
	    /*
	     * Propagate geometry changes immediately.
	     */

	    flags |= TK_MACOSX_HANDLE_EVENT_IMMEDIATELY;
	}

	TkGenWMConfigureEvent((Tk_Window) winPtr, x, y, width, height, flags);
    }

}

- (void) windowExpanded: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
    NSWindow *w = [notification object];
    TkWindow *winPtr = TkMacOSXGetTkWindow(w);

    if (winPtr) {
	winPtr->wmInfoPtr->hints.initial_state =
		TkMacOSXIsWindowZoomed(winPtr) ? ZoomState : NormalState;
	Tk_MapWindow((Tk_Window) winPtr);
	if (Tcl_GetServiceMode() != TCL_SERVICE_NONE) {

	    /*
	     * Process all Tk events generated by Tk_MapWindow().
	     */

	    while (Tcl_ServiceEvent(0)) {}
	    while (Tcl_DoOneEvent(TCL_IDLE_EVENTS|TCL_DONT_WAIT)) {}

	    /*
	     * NSWindowDidDeminiaturizeNotification is received after
	     * NSWindowDidBecomeKeyNotification, so activate manually
	     */

	    GenerateActivateEvents(winPtr, 1);
	} else {
	    Tcl_DoWhenIdle(DoWindowActivate, winPtr);
	}
    }
}

- (NSRect)windowWillUseStandardFrame:(NSWindow *)window
                        defaultFrame:(NSRect)newFrame
{
    /*
     * This method needs to be implemented in order for [NSWindow isZoomed] to
     * give the correct answer. But it suffices to always validate every
     * request.
     */

    return newFrame;
}

- (NSSize)window:(NSWindow *)window
  willUseFullScreenContentSize:(NSSize)proposedSize
{
    /*
     * We don't need to change the proposed size, but we do need to implement
     * this method.  Otherwise the full screen window will be sized to the
     * screen's visibleFrame, leaving black bands at the top and bottom.
     */

    return proposedSize;
}

- (void) windowEnteredFullScreen: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
    [(TKWindow *)[notification object] tkLayoutChanged];
}

- (void) windowExitedFullScreen: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
    [(TKWindow *)[notification object] tkLayoutChanged];
}

- (void) windowCollapsed: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
    NSWindow *w = [notification object];
    TkWindow *winPtr = TkMacOSXGetTkWindow(w);

    if (winPtr) {
	Tk_UnmapWindow((Tk_Window) winPtr);
    }
}

- (BOOL) windowShouldClose: (NSWindow *) w
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, w);
#endif
    TkWindow *winPtr = TkMacOSXGetTkWindow(w);

    if (winPtr) {
	TkGenWMDestroyEvent((Tk_Window) winPtr);
    }

    /*
     * If necessary, TkGenWMDestroyEvent() handles [close]ing the window, so
     * can always return NO from -windowShouldClose: for a Tk window.
     */

    return (winPtr ? NO : YES);
}

#ifdef TK_MAC_DEBUG_NOTIFICATIONS

- (void) windowDragStart: (NSNotification *) notification
{
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
}

- (void) windowLiveResize: (NSNotification *) notification
{
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
    //BOOL start = [[notification name] isEqualToString:NSWindowWillStartLiveResizeNotification];
}

- (void) windowMapped: (NSNotification *) notification
{
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
    NSWindow *w = [notification object];
    TkWindow *winPtr = TkMacOSXGetTkWindow(w);

    if (winPtr) {
	//Tk_MapWindow((Tk_Window) winPtr);
    }
}

- (void) windowBecameVisible: (NSNotification *) notification
{
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
}

- (void) windowUnmapped: (NSNotification *) notification
{
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
    NSWindow *w = [notification object];
    TkWindow *winPtr = TkMacOSXGetTkWindow(w);

    if (winPtr) {
	//Tk_UnmapWindow((Tk_Window) winPtr);
    }
}

#endif /* TK_MAC_DEBUG_NOTIFICATIONS */

- (void) _setupWindowNotifications
{
    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];

#define observe(n, s) \
	[nc addObserver:self selector:@selector(s) name:(n) object:nil]

    observe(NSWindowDidBecomeKeyNotification, windowActivation:);
    observe(NSWindowDidResignKeyNotification, windowActivation:);
    observe(NSWindowDidMoveNotification, windowBoundsChanged:);
    observe(NSWindowDidResizeNotification, windowBoundsChanged:);
    observe(NSWindowDidDeminiaturizeNotification, windowExpanded:);
    observe(NSWindowDidMiniaturizeNotification, windowCollapsed:);

#if !(MAC_OS_X_VERSION_MAX_ALLOWED < 1070)
    observe(NSWindowDidEnterFullScreenNotification, windowEnteredFullScreen:);
    observe(NSWindowDidExitFullScreenNotification, windowExitedFullScreen:);
#endif

#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    observe(NSWindowWillMoveNotification, windowDragStart:);
    observe(NSWindowWillStartLiveResizeNotification, windowLiveResize:);
    observe(NSWindowDidEndLiveResizeNotification, windowLiveResize:);
    observe(NSWindowWillOrderOnScreenNotification, windowMapped:);
    observe(NSWindowDidOrderOnScreenNotification, windowBecameVisible:);
    observe(NSWindowDidOrderOffScreenNotification, windowUnmapped:);
#endif
#undef observe

}
@end

#pragma mark TKApplication(TKApplicationEvent)

@implementation TKApplication(TKApplicationEvent)

- (void) applicationActivate: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
    [NSApp tkCheckPasteboard];

    /*
     * When the application is activated with Command-Tab it will create a
     * zombie window for every Tk window which has been withdrawn.  So iterate
     * through the list of windows and order out any withdrawn window.
     */

    for (NSWindow *win in [NSApp windows]) {
	TkWindow *winPtr = TkMacOSXGetTkWindow(win);
	if (!winPtr || !winPtr->wmInfoPtr) {
	    continue;
	}
	if (winPtr->wmInfoPtr->hints.initial_state == WithdrawnState) {
	    [win orderOut:nil];
	}
    }
}

- (void) applicationDeactivate: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender
                    hasVisibleWindows:(BOOL)flag
{
    /*
     * Allowing the default response means that withdrawn windows will get
     * displayed on the screen with unresponsive title buttons.  We don't
     * really want that.  Besides, we can write our own code to handle this
     * with ::tk::mac::ReopenApplication.  So we just say NO.
     */

    return NO;
}


- (void) applicationShowHide: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
    const char *cmd = ([[notification name] isEqualToString:
	    NSApplicationDidUnhideNotification] ?
	    "::tk::mac::OnShow" : "::tk::mac::OnHide");

    if (_eventInterp && Tcl_FindCommand(_eventInterp, cmd, NULL, 0)) {
	int code = Tcl_EvalEx(_eventInterp, cmd, -1, TCL_EVAL_GLOBAL);

	if (code != TCL_OK) {
	    Tcl_BackgroundException(_eventInterp, code);
	}
	Tcl_ResetResult(_eventInterp);
    }
}

- (void) displayChanged: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
    TkDisplay *dispPtr = TkGetDisplayList();

    if (dispPtr) {
	TkMacOSXDisplayChanged(dispPtr->display);
    }
}
@end

#pragma mark -

/*
 *----------------------------------------------------------------------
 *
 * TkpAppIsDrawing --
 *
 *      A widget display procedure can call this to determine whether it is
 *      being run inside of the drawRect method. This is needed for some tests,
 *      especially of the Text widget, which record data in a global Tcl
 *      variable and assume that display procedures will be run in a
 *      predictable sequence as Tcl idle tasks.
 *
 * Results:
 *	True only while running the drawRect method of a TKContentView;
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

MODULE_SCOPE Bool
TkpAppIsDrawing(void) {
    return [NSApp isDrawing];
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateUpdates --
 *
 *	Given a Macintosh update region and a Tk window this function geneates
 *	an X Expose event for the window if it meets the update region. The
 *	function will then recursivly have each damaged window generate Expose
 *	events for its child windows.
 *
 * Results:
 *	True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

static int
GenerateUpdates(
    HIShapeRef updateRgn,
    CGRect *updateBounds,
    TkWindow *winPtr)
{
    TkWindow *childPtr;
    XEvent event;
    CGRect bounds, damageBounds;
    HIShapeRef boundsRgn, damageRgn;

    TkMacOSXWinCGBounds(winPtr, &bounds);
    if (!CGRectIntersectsRect(bounds, *updateBounds)) {
	return 0;
    }
    if (!HIShapeIntersectsRect(updateRgn, &bounds)) {
	return 0;
    }

    /*
     * Compute the bounding box of the area that the damage occured in.
     */

    boundsRgn = HIShapeCreateWithRect(&bounds);
    damageRgn = HIShapeCreateIntersection(updateRgn, boundsRgn);
    if (HIShapeIsEmpty(damageRgn)) {
	CFRelease(damageRgn);
	CFRelease(boundsRgn);
	return 0;
    }
    HIShapeGetBounds(damageRgn, &damageBounds);

    CFRelease(damageRgn);
    CFRelease(boundsRgn);

    event.xany.serial = LastKnownRequestProcessed(Tk_Display(winPtr));
    event.xany.send_event = false;
    event.xany.window = Tk_WindowId(winPtr);
    event.xany.display = Tk_Display(winPtr);
    event.type = Expose;
    event.xexpose.x = damageBounds.origin.x - bounds.origin.x;
    event.xexpose.y = damageBounds.origin.y - bounds.origin.y;
    event.xexpose.width = damageBounds.size.width;
    event.xexpose.height = damageBounds.size.height;
    event.xexpose.count = 0;
    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);

#ifdef TK_MAC_DEBUG_DRAWING
    TKLog(@"Expose %p {{%d, %d}, {%d, %d}}", event.xany.window, event.xexpose.x,
	event.xexpose.y, event.xexpose.width, event.xexpose.height);
#endif

    /*
     * Generate updates for the children of this window
     */

    for (childPtr = winPtr->childList; childPtr != NULL;
	    childPtr = childPtr->nextPtr) {
	if (!Tk_IsMapped(childPtr) || Tk_IsTopLevel(childPtr)) {
	    continue;
	}
	GenerateUpdates(updateRgn, updateBounds, childPtr);
    }

    /*
     * Generate updates for any contained windows
     */

    if (Tk_IsContainer(winPtr)) {
	childPtr = TkpGetOtherWindow(winPtr);
	if (childPtr != NULL && Tk_IsMapped(childPtr)) {
	    GenerateUpdates(updateRgn, updateBounds, childPtr);
	}

	/*
	 * TODO: Here we should handle out of process embedding.
	 */
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateActivateEvents --
 *
 *	Given a Macintosh window activate event this function generates all the
 *	X Activate events needed by Tk.
 *
 * Results:
 *	True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

int
GenerateActivateEvents(
    TkWindow *winPtr,
    int activeFlag)
{
    TkGenerateActivateEvents(winPtr, activeFlag);
    if (activeFlag || ![NSApp isActive]) {
	TkMacOSXGenerateFocusEvent(winPtr, activeFlag);
    }
    return true;
}

/*
 *----------------------------------------------------------------------
 *
 * DoWindowActivate --
 *
 *	Idle handler that calls GenerateActivateEvents().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

void
DoWindowActivate(
    ClientData clientData)
{
    GenerateActivateEvents(clientData, 1);
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGenerateFocusEvent --
 *
 *	Given a Macintosh window activate event this function generates all
 *	the X Focus events needed by Tk.
 *
 * Results:
 *	True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

MODULE_SCOPE int
TkMacOSXGenerateFocusEvent(
    TkWindow *winPtr,		/* Root X window for event. */
    int activeFlag)
{
    XEvent event;

    /*
     * Don't send focus events to windows of class help or to windows with the
     * kWindowNoActivatesAttribute.
     */

    if (winPtr->wmInfoPtr && (winPtr->wmInfoPtr->macClass == kHelpWindowClass ||
	    winPtr->wmInfoPtr->attributes & kWindowNoActivatesAttribute)) {
	return false;
    }

    /*
     * Generate FocusIn and FocusOut events. This event is only sent to the
     * toplevel window.
     */

    if (activeFlag) {
	event.xany.type = FocusIn;
    } else {
	event.xany.type = FocusOut;
    }

    event.xany.serial = LastKnownRequestProcessed(Tk_Display(winPtr));
    event.xany.send_event = False;
    event.xfocus.display = Tk_Display(winPtr);
    event.xfocus.window = winPtr->window;
    event.xfocus.mode = NotifyNormal;
    event.xfocus.detail = NotifyDetailNone;

    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
    return true;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGenWMConfigureEvent --
 *
 *	Generate a ConfigureNotify event for Tk. Depending on the value of flag
 *	the values of width/height, x/y, or both may be changed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A ConfigureNotify event is sent to Tk.
 *
 *----------------------------------------------------------------------
 */

void
TkGenWMConfigureEvent(
    Tk_Window tkwin,
    int x, int y,
    int width, int height,
    int flags)
{
    XEvent event;
    WmInfo *wmPtr;
    TkWindow *winPtr = (TkWindow *) tkwin;

    if (tkwin == NULL) {
	return;
    }

    event.type = ConfigureNotify;
    event.xconfigure.serial = LastKnownRequestProcessed(Tk_Display(tkwin));
    event.xconfigure.send_event = False;
    event.xconfigure.display = Tk_Display(tkwin);
    event.xconfigure.event = Tk_WindowId(tkwin);
    event.xconfigure.window = Tk_WindowId(tkwin);
    event.xconfigure.border_width = winPtr->changes.border_width;
    event.xconfigure.override_redirect = winPtr->atts.override_redirect;
    if (winPtr->changes.stack_mode == Above) {
	event.xconfigure.above = winPtr->changes.sibling;
    } else {
	event.xconfigure.above = None;
    }

    if (!(flags & TK_LOCATION_CHANGED)) {
	x = Tk_X(tkwin);
	y = Tk_Y(tkwin);
    }
    if (!(flags & TK_SIZE_CHANGED)) {
	width = Tk_Width(tkwin);
	height = Tk_Height(tkwin);
    }
    event.xconfigure.x = x;
    event.xconfigure.y = y;
    event.xconfigure.width = width;
    event.xconfigure.height = height;

    if (flags & TK_MACOSX_HANDLE_EVENT_IMMEDIATELY) {
	Tk_HandleEvent(&event);
    } else {
	Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
    }

    /*
     * Update window manager information.
     */

    if (Tk_IsTopLevel(winPtr)) {
	wmPtr = winPtr->wmInfoPtr;
	if (flags & TK_LOCATION_CHANGED) {
	    wmPtr->x = x;
	    wmPtr->y = y;
	}
	if ((flags & TK_SIZE_CHANGED) && !(wmPtr->flags & WM_SYNC_PENDING) &&
		((width != Tk_Width(tkwin)) || (height != Tk_Height(tkwin)))) {
	    if ((wmPtr->width == -1) && (width == winPtr->reqWidth)) {
		/*
		 * Don't set external width, since the user didn't change it
		 * from what the widgets asked for.
		 */
	    } else if (wmPtr->gridWin != NULL) {
		wmPtr->width = wmPtr->reqGridWidth
			+ (width - winPtr->reqWidth)/wmPtr->widthInc;
		if (wmPtr->width < 0) {
		    wmPtr->width = 0;
		}
	    } else {
		wmPtr->width = width;
	    }

	    if ((wmPtr->height == -1) && (height == winPtr->reqHeight)) {
		/*
		 * Don't set external height, since the user didn't change it
		 * from what the widgets asked for.
		 */
	    } else if (wmPtr->gridWin != NULL) {
		wmPtr->height = wmPtr->reqGridHeight
			+ (height - winPtr->reqHeight)/wmPtr->heightInc;
		if (wmPtr->height < 0) {
		    wmPtr->height = 0;
		}
	    } else {
		wmPtr->height = height;
	    }

	    wmPtr->configWidth = width;
	    wmPtr->configHeight = height;
	}
    }

    /*
     * Now set up the changes structure. Under X we wait for the
     * ConfigureNotify to set these values. On the Mac we know imediatly that
     * this is what we want - so we just set them. However, we need to make
     * sure the windows clipping region is marked invalid so the change is
     * visible to the subwindow.
     */

    winPtr->changes.x = x;
    winPtr->changes.y = y;
    winPtr->changes.width = width;
    winPtr->changes.height = height;
    TkMacOSXInvalClipRgns(tkwin);
}

/*
 *----------------------------------------------------------------------
 *
 * TkGenWMDestroyEvent --
 *
 *	Generate a WM Destroy event for Tk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A WM_PROTOCOL/WM_DELETE_WINDOW event is sent to Tk.
 *
 *----------------------------------------------------------------------
 */

void
TkGenWMDestroyEvent(
    Tk_Window tkwin)
{
    XEvent event;

    event.xany.serial = LastKnownRequestProcessed(Tk_Display(tkwin));
    event.xany.send_event = False;
    event.xany.display = Tk_Display(tkwin);

    event.xclient.window = Tk_WindowId(tkwin);
    event.xclient.type = ClientMessage;
    event.xclient.message_type = Tk_InternAtom(tkwin, "WM_PROTOCOLS");
    event.xclient.format = 32;
    event.xclient.data.l[0] = Tk_InternAtom(tkwin, "WM_DELETE_WINDOW");
    Tk_HandleEvent(&event);
}

/*
 *----------------------------------------------------------------------
 *
 * TkWmProtocolEventProc --
 *
 *	This procedure is called by the Tk_HandleEvent whenever a ClientMessage
 *	event arrives whose type is "WM_PROTOCOLS". This procedure handles the
 *	message from the window manager in an appropriate fashion.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on what sort of handler, if any, was set up for the protocol.
 *
 *----------------------------------------------------------------------
 */

void
TkWmProtocolEventProc(
    TkWindow *winPtr,		/* Window to which the event was sent. */
    XEvent *eventPtr)		/* X event. */
{
    WmInfo *wmPtr;
    ProtocolHandler *protPtr;
    Tcl_Interp *interp;
    Atom protocol;
    int result;

    wmPtr = winPtr->wmInfoPtr;
    if (wmPtr == NULL) {
	return;
    }
    protocol = (Atom) eventPtr->xclient.data.l[0];
    for (protPtr = wmPtr->protPtr; protPtr != NULL;
	    protPtr = protPtr->nextPtr) {
	if (protocol == protPtr->protocol) {
	    Tcl_Preserve(protPtr);
	    interp = protPtr->interp;
	    Tcl_Preserve(interp);
	    result = Tcl_EvalEx(interp, protPtr->command, -1, TCL_EVAL_GLOBAL);
	    if (result != TCL_OK) {
		Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
			"\n    (command for \"%s\" window manager protocol)",
			Tk_GetAtomName((Tk_Window) winPtr, protocol)));
		Tcl_BackgroundException(interp, result);
	    }
	    Tcl_Release(interp);
	    Tcl_Release(protPtr);
	    return;
	}
    }

    /*
     * No handler was present for this protocol. If this is a WM_DELETE_WINDOW
     * message then just destroy the window.
     */

    if (protocol == Tk_InternAtom((Tk_Window) winPtr, "WM_DELETE_WINDOW")) {
	Tk_DestroyWindow((Tk_Window) winPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MacOSXIsAppInFront --
 *
 *	Returns 1 if this app is the foreground app.
 *
 * Results:
 *	1 if app is in front, 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tk_MacOSXIsAppInFront(void)
{
    return ([NSRunningApplication currentApplication].active == true);
}

#pragma mark TKContentView

#import <ApplicationServices/ApplicationServices.h>

/*
 * Custom content view for use in Tk NSWindows.
 *
 * Since Tk handles all drawing of widgets, we only use the AppKit event loop
 * as a source of input events.  To do this, we overload the NSView drawRect
 * method with a method which generates Expose events for Tk but does no
 * drawing.  The redrawing operations are then done when Tk processes these
 * events.
 *
 * Earlier versions of Mac Tk used subclasses of NSView, e.g. NSButton, as the
 * basis for Tk widgets.  These would then appear as subviews of the
 * TKContentView.  To prevent the AppKit from redrawing and corrupting the Tk
 * Widgets it was necessary to use Apple private API calls.  In order to avoid
 * using private API calls, the NSView-based widgets have been replaced with
 * normal Tk widgets which draw themselves as native widgets by using the
 * HITheme API.
 *
 */

/*
 * Restrict event processing to Expose events.
 */

static Tk_RestrictAction
ExposeRestrictProc(
    ClientData arg,
    XEvent *eventPtr)
{
    return (eventPtr->type==Expose && eventPtr->xany.serial==PTR2UINT(arg)
	    ? TK_PROCESS_EVENT : TK_DEFER_EVENT);
}

/*
 * Restrict event processing to ConfigureNotify events.
 */

static Tk_RestrictAction
ConfigureRestrictProc(
    ClientData arg,
    XEvent *eventPtr)
{
    return (eventPtr->type==ConfigureNotify ? TK_PROCESS_EVENT : TK_DEFER_EVENT);
}

/*
 * If a window gets mapped inside the drawRect method, this will be run as an
 * idle task, after drawRect returns, to clean up the mess.
 */

static void
RedisplayView(
    ClientData clientdata)
{
    NSView *view = (NSView *) clientdata;

    /*
     * Make sure that we are not trying to displaying a view that no longer
     * exists.
     */

    for (NSWindow *w in [NSApp orderedWindows]) {
	if ([w contentView] == view) {
	    [view setNeedsDisplay:YES];
	    break;
	}
    }
}

@implementation TKContentView(TKWindowEvent)

- (void) drawRect: (NSRect) rect
{
    const NSRect *rectsBeingDrawn;
    NSInteger rectsBeingDrawnCount;

#ifdef TK_MAC_DEBUG_DRAWING
    TkWindow *winPtr = TkMacOSXGetTkWindow([self window]);
    if (winPtr) fprintf(stderr, "drawRect: drawing %s\n",
			Tk_PathName(winPtr));
#endif

    /*
     * We do not allow recursive calls to drawRect, but we only log them on OSX
     * > 10.13, where they should never happen.
     */

    if ([NSApp isDrawing]) {
	if ([NSApp macMinorVersion] > 13) {
	    TKLog(@"WARNING: a recursive call to drawRect was aborted.");
	}
	return;
    }

    [NSApp setIsDrawing: YES];

    [self getRectsBeingDrawn:&rectsBeingDrawn count:&rectsBeingDrawnCount];
    CGFloat height = [self bounds].size.height;
    HIMutableShapeRef drawShape = HIShapeCreateMutable();

    while (rectsBeingDrawnCount--) {
	CGRect r = NSRectToCGRect(*rectsBeingDrawn++);

#ifdef TK_MAC_DEBUG_DRAWING
	fprintf(stderr, "drawRect: %dx%d@(%d,%d)\n", (int)r.size.width,
	       (int)r.size.height, (int)r.origin.x, (int)r.origin.y);
#endif

	r.origin.y = height - (r.origin.y + r.size.height);
	HIShapeUnionWithRect(drawShape, &r);
    }
    [self generateExposeEvents:(HIShapeRef)drawShape];
    CFRelease(drawShape);
    [NSApp setIsDrawing: NO];

    if ([self needsRedisplay]) {
	[self setNeedsRedisplay:NO];
	Tcl_DoWhenIdle(RedisplayView, self);
    }

#ifdef TK_MAC_DEBUG_DRAWING
    fprintf(stderr, "drawRect: done.\n");
#endif
}

-(void) setFrameSize: (NSSize)newsize
{
    [super setFrameSize: newsize];
    NSWindow *w = [self window];
    TkWindow *winPtr = TkMacOSXGetTkWindow(w);
    Tk_Window tkwin = (Tk_Window) winPtr;

    if (![self inLiveResize] &&
	[w respondsToSelector: @selector (tkLayoutChanged)]) {
	[(TKWindow *)w tkLayoutChanged];
    }

    if (winPtr) {
	unsigned int width = (unsigned int)newsize.width;
	unsigned int height=(unsigned int)newsize.height;
	ClientData oldArg;
    	Tk_RestrictProc *oldProc;

	/*
	 * This can be called from outside the Tk event loop.  Since it calls
	 * Tcl_DoOneEvent, we need to make sure we don't clobber the
	 * AutoreleasePool set up by the caller.
	 */

	[NSApp _lockAutoreleasePool];

	/*
	 * Disable Tk drawing until the window has been completely configured.
	 */

	TkMacOSXSetDrawingEnabled(winPtr, 0);

	 /*
	  * Generate and handle a ConfigureNotify event for the new size.
	  */

	TkGenWMConfigureEvent(tkwin, Tk_X(tkwin), Tk_Y(tkwin), width, height,
		TK_SIZE_CHANGED | TK_MACOSX_HANDLE_EVENT_IMMEDIATELY);
    	oldProc = Tk_RestrictEvents(ConfigureRestrictProc, NULL, &oldArg);
    	Tk_RestrictEvents(oldProc, oldArg, &oldArg);

	/*
	 * Now that Tk has configured all subwindows, create the clip regions.
	 */

	TkMacOSXSetDrawingEnabled(winPtr, 1);
	TkMacOSXInvalClipRgns(tkwin);
	TkMacOSXUpdateClipRgn(winPtr);

	 /*
	  * Generate and process expose events to redraw the window.
	  */

	HIRect bounds = NSRectToCGRect([self bounds]);
	HIShapeRef shape = HIShapeCreateWithRect(&bounds);
	[self generateExposeEvents: shape];
	[w displayIfNeeded];

	/*
	 * Finally, unlock the main autoreleasePool.
	 */

	[NSApp _unlockAutoreleasePool];
    }
}

/*
 * Core method of this class: generates expose events for redrawing.  The
 * expose events are immediately removed from the Tcl event loop and processed.
 * This causes drawing procedures to be scheduled as idle events.  Then all
 * pending idle events are processed so the drawing will actually take place.
 */

- (void) generateExposeEvents: (HIShapeRef) shape
{
    unsigned long serial;
    CGRect updateBounds;
    int updatesNeeded;
    TkWindow *winPtr = TkMacOSXGetTkWindow([self window]);
    ClientData oldArg;
    Tk_RestrictProc *oldProc;
    if (!winPtr) {
	return;
    }

    /*
     * Generate Tk Expose events.
     */

    HIShapeGetBounds(shape, &updateBounds);

    /*
     * All of these events will share the same serial number.
     */

    serial = LastKnownRequestProcessed(Tk_Display(winPtr));
    updatesNeeded = GenerateUpdates(shape, &updateBounds, winPtr);

    if (updatesNeeded) {

	/*
	 * First process all of the Expose events.
	 */

    	oldProc = Tk_RestrictEvents(ExposeRestrictProc, UINT2PTR(serial), &oldArg);
    	while (Tcl_ServiceEvent(TCL_WINDOW_EVENTS)) {};
    	Tk_RestrictEvents(oldProc, oldArg, &oldArg);

	/*
	 * Starting with OSX 10.14, which uses Core Animation to draw windows,
	 * all drawing must be done within the drawRect method.  (The CGContext
	 * which draws to the backing CALayer is created by the NSView before
	 * calling drawRect, and destroyed when drawRect returns.  Drawing done
	 * with the current CGContext outside of the drawRect method has no
	 * effect.)
	 *
	 * Fortunately, Tk schedules all drawing to be done while Tcl is idle.
	 * So we can do the drawing by processing all of the idle events that
	 * were created when the expose events were processed.
	 */
	while (Tcl_DoOneEvent(TCL_IDLE_EVENTS)) {}
    }
}

/*
 * This method is called when a user changes between light and dark mode. The
 * implementation here generates a Tk virtual event which can be bound to a
 * function that redraws the window in an appropriate style.
 */

- (void) viewDidChangeEffectiveAppearance
{
    XVirtualEvent event;
    int x, y;
    NSWindow *w = [self window];
    TkWindow *winPtr = TkMacOSXGetTkWindow(w);
    Tk_Window tkwin = (Tk_Window) winPtr;

    if (!winPtr) {
	return;
    }
    bzero(&event, sizeof(XVirtualEvent));
    event.type = VirtualEvent;
    event.serial = LastKnownRequestProcessed(Tk_Display(tkwin));
    event.send_event = false;
    event.display = Tk_Display(tkwin);
    event.event = Tk_WindowId(tkwin);
    event.root = XRootWindow(Tk_Display(tkwin), 0);
    event.subwindow = None;
    event.time = TkpGetMS();
    XQueryPointer(NULL, winPtr->window, NULL, NULL,
    		  &event.x_root, &event.y_root, &x, &y, &event.state);
    Tk_TopCoordsToWindow(tkwin, x, y, &event.x, &event.y);
    event.same_screen = true;
    if (TkMacOSXInDarkMode(tkwin)) {
	event.name = Tk_GetUid("DarkAqua");
    } else {
        event.name = Tk_GetUid("LightAqua");
    }
    Tk_QueueWindowEvent((XEvent *) &event, TCL_QUEUE_TAIL);
}

/*
 * This is no-op on 10.7 and up because Apple has removed this widget, but we
 * are leaving it here for backwards compatibility.
 */

- (void) tkToolbarButton: (id) sender
{
#ifdef TK_MAC_DEBUG_EVENTS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd);
#endif
    XVirtualEvent event;
    int x, y;
    TkWindow *winPtr = TkMacOSXGetTkWindow([self window]);
    Tk_Window tkwin = (Tk_Window) winPtr;
    if (!winPtr){
	return;
    }
    bzero(&event, sizeof(XVirtualEvent));
    event.type = VirtualEvent;
    event.serial = LastKnownRequestProcessed(Tk_Display(tkwin));
    event.send_event = false;
    event.display = Tk_Display(tkwin);
    event.event = Tk_WindowId(tkwin);
    event.root = XRootWindow(Tk_Display(tkwin), 0);
    event.subwindow = None;
    event.time = TkpGetMS();
    XQueryPointer(NULL, winPtr->window, NULL, NULL,
	    &event.x_root, &event.y_root, &x, &y, &event.state);
    Tk_TopCoordsToWindow(tkwin, x, y, &event.x, &event.y);
    event.same_screen = true;
    event.name = Tk_GetUid("ToolbarButton");
    Tk_QueueWindowEvent((XEvent *) &event, TCL_QUEUE_TAIL);
}

- (BOOL) isOpaque
{
    NSWindow *w = [self window];
      return (w && (([w styleMask] & NSTexturedBackgroundWindowMask) ||
    	    ![w isOpaque]) ? NO : YES);
}

- (BOOL) wantsDefaultClipping
{
    return NO;
}

- (BOOL) acceptsFirstResponder
{
    return YES;
}

/*
 * This keyDown method does nothing, which is a huge improvement over the
 * default keyDown method which beeps every time a key is pressed.
 */

- (void) keyDown: (NSEvent *) theEvent
{
#ifdef TK_MAC_DEBUG_EVENTS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, theEvent);
#endif
}

/*
 * When the services menu is opened this is called for each Responder in
 * the Responder chain until a service provider is found.  The TkContentView
 * should be the first (and generally only) Responder in the chain.  We
 * return the TkServices object that was created in TkpInit.
 */

- (id)validRequestorForSendType:(NSString *)sendType
		     returnType:(NSString *)returnType
{
    if ([sendType isEqualToString:@"NSStringPboardType"] ||
	[sendType isEqualToString:@"NSPasteboardTypeString"]) {
	return [NSApp servicesProvider];
    }
    return [super validRequestorForSendType:sendType returnType:returnType];
}

@end

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
