static const char CVSID[] = "$Id: misc.c,v 1.89 2010/07/05 06:23:59 lebert Exp $";
/*******************************************************************************
*									                                                    *
* misc.c -- Miscelaneous Motif convenience functions			                   *
*									                                                    *
* Copyright (C) 1999 Mark Edel						                               *
*									                                                    *
* This is free software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version. In addition, you may distribute version of this program linked to   *
* Motif or Open Motif. See README for details.                                 *
*                                                                              *
* This software is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
* for more details.							                                        *
* 									                                                    *
* You should have received a copy of the GNU General Public License along with *
* software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
* Place, Suite 330, Boston, MA  02111-1307 USA		                            *
*									                                                    *
* Nirvana Text Editor	    				                                        *
* July 28, 1992								                                        *
*									                                                    *
* Written by Mark Edel						                                        *
*									                                                    *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "misc.h"
#include "utils.h"
#include "DialogF.h"
#include "Ne_AppContext.h"

#include <FL/Fl.H>
#include <FL/Fl_Group.H>

#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>

#ifdef __unix__
#include <sys/time.h>
#include <sys/select.h>
#endif

#ifdef __APPLE__
#ifdef __MACH__
#include <sys/select.h>
#endif
#endif

#ifdef WIN32
#include <time.h>
#include <sys/timeb.h>
#endif

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

/* structure for passing history-recall data to callbacks */
typedef struct
{
   char*** list;
   int* nItems;
   int index;
} histInfo;

// TODO: typedef Fl_Widget*(*MotifDialogCreationCall)(Fl_Widget*, const char*, ArgList, int);

/* Maximum size of a history-recall list.  Typically never invoked, since
   user must first make this many entries in the text field, limited for
   safety, to the maximum reasonable number of times user can hit up-arrow
   before carpal tunnel syndrome sets in */
#define HISTORY_LIST_TRIM_TO 1000
#define HISTORY_LIST_MAX 2000

/* flags to enable/disable delete key remapping and pointer centered dialogs */
static int RemapDeleteEnabled = true;
static int PointerCenteredDialogsEnabled = false;

/* bitmap and mask for waiting (wrist-watch) cursor */
#define watch_x_hot 7
#define watch_y_hot 7
#define watch_width 16
#define watch_height 16
static unsigned char watch_bits[] =
{
   0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07, 0x10, 0x08, 0x08, 0x11,
   0x04, 0x21, 0x04, 0x21, 0xe4, 0x21, 0x04, 0x20, 0x08, 0x10, 0x10, 0x08,
   0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07
};
#define watch_mask_width 16
#define watch_mask_height 16
static unsigned char watch_mask_bits[] =
{
   0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf8, 0x1f, 0xfc, 0x3f,
   0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfc, 0x3f, 0xf8, 0x1f,
   0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f
};

// TODO: static void addMnemonicGrabs(Fl_Widget* addTo, Fl_Widget* w, int unmodified);
// TODO: static void mnemonicCB(Fl_Widget* w, XtPointer callData, XKeyEvent* event);
// TODO: static void findAndActivateMnemonic(Fl_Widget* w, unsigned int keycode);
// TODO: static void addAccelGrabs(Fl_Widget* topWidget, Fl_Widget* w);
// TODO: static void addAccelGrab(Fl_Widget* topWidget, Fl_Widget* w);
// TODO: static int parseAccelString(Display* display, const char* string, KeySym* keysym, unsigned int* modifiers);
// TODO: static void lockCB(Fl_Widget* w, XtPointer callData, int event, bool* continueDispatch);
// TODO: static int findAndActivateAccel(Fl_Widget* w, unsigned int keyCode, unsigned int modifiers, int event);
static void removeWhiteSpace(char* string);
// TODO: static int stripCaseCmp(const char* str1, const char* str2);
// TODO: static void warnHandlerCB(const char* message);
// TODO: static void histDestroyCB(Fl_Widget* w, XtPointer clientData, XtPointer callData);
// TODO: static void histArrowKeyEH(Fl_Widget* w, XtPointer callData, int event, bool* continueDispatch);
// TODO: static ArgList addParentVisArgs(Fl_Widget* parent, ArgList arglist, int* argcount);
// TODO: static Fl_Widget* addParentVisArgsAndCall(MotifDialogCreationCall callRoutine, Fl_Widget* parent, char* name); // TODO:, ArgList arglist, int  argcount);
static void scrollDownAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void scrollUpAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void pageDownAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void pageUpAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: static long queryDesktop(Display* display, Window window, Atom deskTopAtom);
static void warning(const char* mesg);
// TODO: static void microsleep(long usecs);

/* Motif compatibility mode for FLTK */
bool NeToggleButtonGetState(Fl_Button* w)
{
   return (w->value() != 0);
}

void NeToggleButtonSetState(Fl_Button* w, bool state, bool doCallback)
{
   w->value(state);
   if (doCallback)
      w->do_callback();
}

void NeSetSensitive(Fl_Widget* w, bool state)
{
   if (state) w->activate();
   else w->deactivate();
}

void NeSetSensitive(Fl_Menu_Item* item, bool state)
{
   if (state) item->activate();
   else item->deactivate();
}

int NeStringToKeysym(const char* str)
{
   if (!strcmp("Space", str)) return ' ';
   if (!strcmp("Comma", str)) return ',';
   if (!strcmp("Parenright", str)) return ')';
   if (!strcmp("Parenleft", str)) return '(';
   if (!strcmp("KP_Add", str)) return '+';
   if (!strcmp("KP_Subtract", str)) return '-';
   if (!strcmp("F1", str)) return FL_F + 1;
   if (!strcmp("F2", str)) return FL_F + 2;
   if (!strcmp("F3", str)) return FL_F + 3;
   if (!strcmp("F4", str)) return FL_F + 4;
   if (!strcmp("F5", str)) return FL_F + 5;
   if (!strcmp("F6", str)) return FL_F + 6;
   if (!strcmp("F7", str)) return FL_F + 7;
   if (!strcmp("F8", str)) return FL_F + 8;
   if (!strcmp("F9", str)) return FL_F + 9;
   if (!strcmp("F10", str)) return FL_F + 10;
   if (!strcmp("F11", str)) return FL_F + 11;
   if (!strcmp("F12", str)) return FL_F + 12;

   if (str[1] == '\0')
   {
      // One key shortcut
      char c = tolower(str[0]);
      if (c >= 'a' && c <= 'z') return c;
      if (c >= '0' && c <= '9') return c;
   }
   return 0;
}

const char* NeKeysymToString(int keySym)
{
   static char str[20] = "";
   memset(str, '\0', sizeof(str));
   int c = keySym;

   if (c == ' ') strcpy(str, "Space");
   else if (c == ',') strcpy(str, "Comma");
   else if (c == ')') strcpy(str, "Parenright");
   else if (c == '(') strcpy(str, "Parenleft");
   else if (c == '+') strcpy(str, "KP_Add");
   else if (c == '-') strcpy(str, "KP_Subtract");
   else if (c == FL_F + 1) strcpy(str, "F1");
   else if (c == FL_F + 2) strcpy(str, "F2");
   else if (c == FL_F + 3) strcpy(str, "F3");
   else if (c == FL_F + 4) strcpy(str, "F4");
   else if (c == FL_F + 5) strcpy(str, "F5");
   else if (c == FL_F + 6) strcpy(str, "F6");
   else if (c == FL_F + 7) strcpy(str, "F7");
   else if (c == FL_F + 8) strcpy(str, "F8");
   else if (c == FL_F + 9) strcpy(str, "F9");
   else if (c == FL_F + 10) strcpy(str, "F10");
   else if (c == FL_F + 11) strcpy(str, "F11");
   else if (c == FL_F + 12) strcpy(str, "F12");
   else str[0] = char(c);

   return str;
}

// --------------------------------------------------------------------------
// Return the main window from any widget
// --------------------------------------------------------------------------
Fl_Group* WidgetToMainWindow(Fl_Widget* w)
{
   Fl_Group* parent = w->parent();
   if (!parent) return (Fl_Group*)w;
   do {
      if (!parent->parent())
         return parent;
      parent = parent->parent();
   } while(1); // breaking the infinite loop ?
}


// TODO: /*
// TODO: ** Set up closeCB to be called when the user selects close from the
// TODO: ** window menu.  The close menu item usually activates f.kill which
// TODO: ** sends a WM_DELETE_WINDOW protocol request for the window.
// TODO: */
// TODO: void AddMotifCloseCallback(Fl_Widget* shell, XtCallbackProc closeCB, void* arg)
// TODO: {
// TODO:    static Atom wmpAtom, dwAtom = 0;
// TODO:    Display* display = XtDisplay(shell);
// TODO: 
// TODO:    /* deactivate the built in delete response of killing the application */
// TODO:    XtVaSetValues(shell, XmNdeleteResponse, XmDO_NOTHING, NULL);
// TODO: 
// TODO:    /* add a delete window protocol callback instead */
// TODO:    if (dwAtom == 0)
// TODO:    {
// TODO:       wmpAtom = XmInternAtom(display, "WM_PROTOCOLS", FALSE);
// TODO:       dwAtom = XmInternAtom(display, "WM_DELETE_WINDOW", FALSE);
// TODO:    }
// TODO:    XmAddProtocolCallback(shell, wmpAtom, dwAtom, closeCB, arg);
// TODO: }
// TODO: /*
// TODO: ** This routine kludges around the problem of backspace not being mapped
// TODO: ** correctly when Motif is used between a server with a delete key in
// TODO: ** the traditional typewriter backspace position and a client that
// TODO: ** expects a backspace key in that position.  Though there are three
// TODO: ** distinct levels of key re-mapping in effect when a user is running
// TODO: ** a Motif application, none of these is really appropriate or effective
// TODO: ** for eliminating the delete v.s. backspace problem.  Our solution is,
// TODO: ** sadly, to eliminate the forward delete functionality of the delete key
// TODO: ** in favor of backwards delete for both keys.  So as not to prevent the
// TODO: ** user or the application from applying other translation table re-mapping,
// TODO: ** we apply re-map the key as a post-processing step, applied after widget
// TODO: ** creation.  As a result, the re-mapping necessarily becomes embedded
// TODO: ** throughout an application (wherever text widgets are created), and
// TODO: ** within library routines, including the Nirvana utility library.  To
// TODO: ** make this remapping optional, the SetDeleteRemap function provides a
// TODO: ** way for an application to turn this functionality on and off.  It is
// TODO: ** recommended that applications that use this routine provide an
// TODO: ** application resource called remapDeleteKey so savvy users can get
// TODO: ** their forward delete functionality back.
// TODO: */
// TODO: void RemapDeleteKey(Fl_Widget* w)
// TODO: {
// TODO:    static XtTranslations table = NULL;
// TODO:    static char* translations =
// TODO:       "~Shift~Ctrl~Meta~Alt<Key>osfDelete: delete-previous-character()\n";
// TODO: 
// TODO:    if (RemapDeleteEnabled)
// TODO:    {
// TODO:       if (table == NULL)
// TODO:          table = XtParseTranslationTable(translations);
// TODO:       XtOverrideTranslations(w, table);
// TODO:    }
// TODO: }

void SetDeleteRemap(int state)
{
   RemapDeleteEnabled = state;
}


// TODO: /*
// TODO: ** The routine adds the passed in top-level Fl_Widget*'s window to our
// TODO: ** window group.  On the first call a dummy unmapped window will
// TODO: ** be created to be our leader.  This must not be called before the
// TODO: ** Fl_Widget* has be realized and should be called before the window is
// TODO: ** mapped.
// TODO: */
// TODO: static void setWindowGroup(Fl_Widget* shell)
// TODO: {
// TODO:    static int firstTime = true;
// TODO:    static Window groupLeader;
// TODO:    Display* display = XtDisplay(shell);
// TODO:    XWMHints* wmHints;
// TODO: 
// TODO:    if (firstTime)
// TODO:    {
// TODO:       /* Create a dummy window to be the group leader for our windows */
// TODO:       const char* name, wClass;
// TODO:       XClassHint* classHint;
// TODO: 
// TODO:       groupLeader = XCreateSimpleWindow(display,
// TODO:                                         RootWindow(display, DefaultScreen(display)),
// TODO:                                         1, 1, 1, 1, 0, 0, 0);
// TODO: 
// TODO:       /* Set it's class hint so it will be identified correctly by the
// TODO:          window manager */
// TODO:       XtGetApplicationNameAndClass(display, &name, &wClass);
// TODO:       classHint = XAllocClassHint();
// TODO:       classHint->res_name = name;
// TODO:       classHint->res_class = wClass;
// TODO:       XSetClassHint(display, groupLeader, classHint);
// TODO:       XFree(classHint);
// TODO: 
// TODO:       firstTime = false;
// TODO:    }
// TODO: 
// TODO:    /* Set the window group hint for this shell's window */
// TODO:    wmHints = XGetWMHints(display, XtWindow(shell));
// TODO:    wmHints->window_group = groupLeader;
// TODO:    wmHints->flags |= WindowGroupHint;
// TODO:    XSetWMHints(display, XtWindow(shell), wmHints);
// TODO:    XFree(wmHints);
// TODO: }

/*
** This routine resolves a window manager protocol incompatibility between
** the X toolkit and several popular window managers.  Using this in place
** of XtRealizeWidget will realize the window in a way which allows the
** affected window managers to apply their own placement strategy to the
** window, as opposed to forcing the window to a specific location.
**
** One of the hints in the WM_NORMAL_HINTS protocol, PPlacement, gets set by
** the X toolkit (probably part of the Core or Shell widget) when a shell
** widget is realized to the value stored in the XmNx and XmNy resources of the
** Core widget.  While callers can set these values, there is no "unset" value
** for these resources.  On systems which are more Motif aware, a PPosition
** hint of 0,0, which is the default for XmNx and XmNy, is interpreted as,
** "place this as if no hints were specified".  Unfortunately the fvwm family
** of window managers, which are now some of the most popular, interpret this
** as "place this window at (0,0)".  This routine intervenes between the
** realizing and the mapping of the window to remove the inappropriate
** PPlacement hint.
*/
void RemovePPositionHint(Fl_Widget* shell)
{
   // TODO:    XSizeHints* hints = XAllocSizeHints();
   // TODO:    long suppliedHints;
   // TODO: 
   // TODO:    /* Get rid of the incorrect WMNormal hint */
   // TODO:    if (XGetWMNormalHints(XtDisplay(shell), XtWindow(shell), hints,
   // TODO:                          &suppliedHints))
   // TODO:    {
   // TODO:       hints->flags &= ~PPosition;
   // TODO:       XSetWMNormalHints(XtDisplay(shell), XtWindow(shell), hints);
   // TODO:    }
   // TODO: 
   // TODO:    XFree(hints);
}

// TODO: void RealizeWithoutForcingPosition(Fl_Widget* shell)
// TODO: {
// TODO:    bool mappedWhenManaged;
// TODO: 
// TODO:    /* Temporarily set value of XmNmappedWhenManaged
// TODO:       to stop the window from popping up right away */
// TODO:    XtVaGetValues(shell, XmNmappedWhenManaged, &mappedWhenManaged, NULL);
// TODO:    XtVaSetValues(shell, XmNmappedWhenManaged, false, NULL);
// TODO: 
// TODO:    /* Realize the widget in unmapped state */
// TODO:    XtRealizeWidget(shell);
// TODO: 
// TODO:    /* Remove the hint */
// TODO:    RemovePPositionHint(shell);
// TODO: 
// TODO:    /* Set WindowGroupHint so the NEdit icons can be grouped; this
// TODO:       seems to be necessary starting with Gnome 2.0  */
// TODO:    setWindowGroup(shell);
// TODO: 
// TODO:    /* Map the widget */
// TODO:    XtMapWidget(shell);
// TODO: 
// TODO:    /* Restore the value of XmNmappedWhenManaged */
// TODO:    XtVaSetValues(shell, XmNmappedWhenManaged, mappedWhenManaged, NULL);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Older X applications and X servers were mostly designed to operate with
// TODO: ** visual class PseudoColor, because older displays were at most 8 bits
// TODO: ** deep.  Modern X servers, however, usually support 24 bit depth and other
// TODO: ** color models.  Sun (and others?) still sets their default visual to
// TODO: ** 8-bit PseudoColor, because some of their X applications don't work
// TODO: ** properly with the other color models.  The problem with PseudoColor, of
// TODO: ** course, is that users run out of colors in the default colormap, and if
// TODO: ** they install additional colormaps for individual applications, colors
// TODO: ** flash and change weirdly when you change your focus from one application
// TODO: ** to another.
// TODO: **
// TODO: ** In addition to the poor choice of default, a design flaw in Xt makes it
// TODO: ** impossible even for savvy users to specify the XtNvisual resource to
// TODO: ** switch to a deeper visual.  The problem is that the colormap resource is
// TODO: ** processed independently of the visual resource, and usually results in a
// TODO: ** colormap for the default visual rather than for the user-selected one.
// TODO: **
// TODO: ** This routine should be called before creating a shell widget, to
// TODO: ** pre-process the visual, depth, and colormap resources, and return the
// TODO: ** proper values for these three resources to be passed to XtAppCreateShell.
// TODO: ** Applications which actually require a particular color model (i.e. for
// TODO: ** doing color table animation or dynamic color assignment) should not use
// TODO: ** this routine.
// TODO: **
// TODO: ** Note that a consequence of using the "best" as opposed to the default
// TODO: ** visual is that some color resources are still converted with the default
// TODO: ** visual (particularly *background), and these must be avoided by widgets
// TODO: ** which are allowed to handle any visual.
// TODO: **
// TODO: ** Returns true if the best visual is the default, false otherwise.
// TODO: */
// TODO: bool FindBestVisual(Display* display, const char* appName, const char* appClass,
// TODO:                        Visual** visual, int* depth, Colormap* colormap)
// TODO: {
// TODO:    char rsrcName[256], rsrcClass[256], *valueString, *type, *endPtr;
// TODO:    XrmValue value;
// TODO:    int screen = DefaultScreen(display);
// TODO:    int reqDepth = -1;
// TODO:    long reqID = -1; /* should hold a 'VisualID' and a '-1' ... */
// TODO:    int reqClass = -1;
// TODO:    int installColormap = FALSE;
// TODO:    int maxDepth, bestClass, bestVisual, nVis, i, j;
// TODO:    XVisualInfo visTemplate, *visList = NULL;
// TODO:    static Visual* cachedVisual = NULL;
// TODO:    static Colormap cachedColormap;
// TODO:    static int cachedDepth = 0;
// TODO:    int bestClasses[] = {StaticGray, GrayScale, StaticColor, PseudoColor,
// TODO:                         DirectColor, TrueColor
// TODO:                        };
// TODO: 
// TODO:    /* If results have already been computed, just return them */
// TODO:    if (cachedVisual != NULL)
// TODO:    {
// TODO:       *visual = cachedVisual;
// TODO:       *depth = cachedDepth;
// TODO:       *colormap = cachedColormap;
// TODO:       return (*visual == DefaultVisual(display, screen));
// TODO:    }
// TODO: 
// TODO:    /* Read the visualID and installColormap resources for the application.
// TODO:       visualID can be specified either as a number (the visual id as
// TODO:       shown by xdpyinfo), as a visual class name, or as Best or Default. */
// TODO:    sprintf(rsrcName,"%s.%s", appName, "visualID");
// TODO:    sprintf(rsrcClass, "%s.%s", appClass, "VisualID");
// TODO:    if (XrmGetResource(XtDatabase(display), rsrcName, rsrcClass, &type,
// TODO:                       &value))
// TODO:    {
// TODO:       valueString = value.addr;
// TODO:       reqID = (int)strtol(valueString, &endPtr, 0);
// TODO:       if (endPtr == valueString)
// TODO:       {
// TODO:          reqID = -1;
// TODO:          if (stripCaseCmp(valueString, "Default"))
// TODO:             reqID = DefaultVisual(display, screen)->visualid;
// TODO:          else if (stripCaseCmp(valueString, "StaticGray"))
// TODO:             reqClass = StaticGray;
// TODO:          else if (stripCaseCmp(valueString, "StaticColor"))
// TODO:             reqClass = StaticColor;
// TODO:          else if (stripCaseCmp(valueString, "TrueColor"))
// TODO:             reqClass = TrueColor;
// TODO:          else if (stripCaseCmp(valueString, "GrayScale"))
// TODO:             reqClass = GrayScale;
// TODO:          else if (stripCaseCmp(valueString, "PseudoColor"))
// TODO:             reqClass = PseudoColor;
// TODO:          else if (stripCaseCmp(valueString, "DirectColor"))
// TODO:             reqClass = DirectColor;
// TODO:          else if (!stripCaseCmp(valueString, "Best"))
// TODO:             fprintf(stderr, "Invalid visualID resource value\n");
// TODO:       }
// TODO:    }
// TODO:    sprintf(rsrcName,"%s.%s", appName, "installColormap");
// TODO:    sprintf(rsrcClass, "%s.%s", appClass, "InstallColormap");
// TODO:    if (XrmGetResource(XtDatabase(display), rsrcName, rsrcClass, &type,
// TODO:                       &value))
// TODO:    {
// TODO:       if (stripCaseCmp(value.addr, "Yes") || stripCaseCmp(value.addr, "true"))
// TODO:          installColormap = TRUE;
// TODO:    }
// TODO: 
// TODO:    visTemplate.screen = screen;
// TODO: 
// TODO:    /* Generate a list of visuals to consider.  (Note, vestigial code for
// TODO:       user-requested visual depth is left in, just in case that function
// TODO:       might be needed again, but it does nothing). */
// TODO:    if (reqID != -1)
// TODO:    {
// TODO:       visTemplate.visualid = reqID;
// TODO:       visList = XGetVisualInfo(display, VisualScreenMask|VisualIDMask,
// TODO:                                &visTemplate, &nVis);
// TODO:       if (visList == NULL)
// TODO:          fprintf(stderr, "VisualID resource value not valid\n");
// TODO:    }
// TODO:    if (visList == NULL && reqClass != -1 && reqDepth != -1)
// TODO:    {
// TODO:       // TODO      visTemplate.class = reqClass;
// TODO:       visTemplate.depth = reqDepth;
// TODO:       visList = XGetVisualInfo(display,
// TODO:                                VisualScreenMask| VisualClassMask | VisualDepthMask,
// TODO:                                &visTemplate, &nVis);
// TODO:       if (visList == NULL)
// TODO:          fprintf(stderr, "Visual class/depth combination not available\n");
// TODO:    }
// TODO:    if (visList == NULL && reqClass != -1)
// TODO:    {
// TODO:       // TODO      visTemplate.class = reqClass;
// TODO:       visList = XGetVisualInfo(display, VisualScreenMask|VisualClassMask,
// TODO:                                &visTemplate, &nVis);
// TODO:       if (visList == NULL)
// TODO:          fprintf(stderr,
// TODO:                  "Visual Class from resource \"visualID\" not available\n");
// TODO:    }
// TODO:    if (visList == NULL && reqDepth != -1)
// TODO:    {
// TODO:       visTemplate.depth = reqDepth;
// TODO:       visList = XGetVisualInfo(display, VisualScreenMask|VisualDepthMask,
// TODO:                                &visTemplate, &nVis);
// TODO:       if (visList == NULL)
// TODO:          fprintf(stderr, "Requested visual depth not available\n");
// TODO:    }
// TODO:    if (visList == NULL)
// TODO:    {
// TODO:       visList = XGetVisualInfo(display, VisualScreenMask, &visTemplate, &nVis);
// TODO:       if (visList == NULL)
// TODO:       {
// TODO:          fprintf(stderr, "Internal Error: no visuals available?\n");
// TODO:          *visual = DefaultVisual(display, screen);
// TODO:          *depth =  DefaultDepth(display, screen);
// TODO:          *colormap = DefaultColormap(display, screen);
// TODO:          return true;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* Choose among the visuals in the candidate list.  Prefer maximum
// TODO:       depth first then matching default, then largest value of bestClass
// TODO:       (I'm not sure whether we actually care about class) */
// TODO:    maxDepth = 0;
// TODO:    bestClass = 0;
// TODO:    bestVisual = 0;
// TODO:    for (i=0; i < nVis; i++)
// TODO:    {
// TODO:       /* X.Org 6.8+ 32-bit visuals (with alpha-channel) cause a lot of
// TODO:          problems, so we have to skip them. We already try this by setting
// TODO:          the environment variable XLIB_SKIP_ARGB_VISUALS at startup (in
// TODO:          nedit.c), but that doesn't cover the case where NEdit is running on
// TODO:          a host that doesn't use the X.Org X libraries but is displaying
// TODO:          remotely on an X.Org server. Therefore, this additional check is
// TODO:          added.
// TODO:          Note that this check in itself is not sufficient. There have been
// TODO:          bug reports that seemed to indicate that non-32-bit visuals with an
// TODO:          alpha-channel exist. The combined approach (env. var. + 32-bit
// TODO:          check) should cover the vast majority of the cases, though. */
// TODO:       if (visList[i].depth >= 32 &&
// TODO:             strstr(ServerVendor(display), "X.Org") != 0)
// TODO:       {
// TODO:          continue;
// TODO:       }
// TODO:       if (visList[i].depth > maxDepth)
// TODO:       {
// TODO:          maxDepth = visList[i].depth;
// TODO:          bestClass = 0;
// TODO:          bestVisual = i;
// TODO:       }
// TODO:       if (visList[i].depth == maxDepth)
// TODO:       {
// TODO:          if (visList[i].visual == DefaultVisual(display, screen))
// TODO:             bestVisual = i;
// TODO:          if (visList[bestVisual].visual != DefaultVisual(display, screen))
// TODO:          {
// TODO:             /* TODO for (j = 0; j < (int)XtNumber(bestClasses); j++)
// TODO:                         {
// TODO:                            if (visList[i].class == bestClasses[j] && j > bestClass)
// TODO:                            {
// TODO:                               bestClass = j;
// TODO:                               bestVisual = i;
// TODO:                            }
// TODO:                         }
// TODO:             */
// TODO:          }
// TODO:       }
// TODO:    }
// TODO:    *visual = cachedVisual = visList[bestVisual].visual;
// TODO:    *depth = cachedDepth = visList[bestVisual].depth;
// TODO: 
// TODO:    /* If the chosen visual is not the default, it needs a colormap allocated */
// TODO:    if (*visual == DefaultVisual(display, screen) && !installColormap)
// TODO:       *colormap = cachedColormap = DefaultColormap(display, screen);
// TODO:    else
// TODO:    {
// TODO:       *colormap = cachedColormap = XCreateColormap(display,
// TODO:                                    RootWindow(display, screen), cachedVisual, AllocNone);
// TODO:       XInstallColormap(display, cachedColormap);
// TODO:    }
// TODO:    /* printf("Chose visual with depth %d, class %d, colormap %ld, id 0x%x\n",
// TODO:       visList[bestVisual].depth, visList[bestVisual].class,
// TODO:       *colormap, cachedVisual->visualid); */
// TODO:    /* Fix memory leak */
// TODO:    if (visList != NULL)
// TODO:    {
// TODO:       XFree(visList);
// TODO:    }
// TODO: 
// TODO:    return (*visual == DefaultVisual(display, screen));
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** If you want to use a non-default visual with Motif, shells all have to be
// TODO: ** created with that visual, depth, and colormap, even if the parent has them
// TODO: ** set up properly. Substituting these routines, will append visual args copied
// TODO: ** from the parent widget (CreatePopupMenu and CreatePulldownMenu), or from the
// TODO: ** best visual, obtained via FindBestVisual above (CreateShellWithBestVis).
// TODO: */
// TODO: Fl_Widget* CreateDialogShell(Fl_Widget* parent, char* name,
// TODO:                          ArgList arglist, int  argcount)
// TODO: {
// TODO:    return addParentVisArgsAndCall(XmCreateDialogShell, parent, name, arglist,
// TODO:                                   argcount);
// TODO: }
// TODO: 
// TODO: 
// TODO: Fl_Widget* CreatePopupMenu(Fl_Widget* parent, char* name, ArgList arglist,
// TODO:                        int argcount)
// TODO: {
// TODO:    return addParentVisArgsAndCall(XmCreatePopupMenu, parent, name,
// TODO:                                   arglist, argcount);
// TODO: }
// TODO: 
// TODO: 
// TODO: Fl_Widget* CreatePulldownMenu(Fl_Widget* parent, char* name,
// TODO:                           ArgList arglist, int  argcount)
// TODO: {
// TODO:    return addParentVisArgsAndCall(XmCreatePulldownMenu, parent, name, arglist,
// TODO:                                   argcount);
// TODO: }
// TODO: 
// TODO: 
// TODO: Fl_Widget* CreatePromptDialog(Fl_Widget* parent, char* name ) // TODO:, ArgList arglist, int  argcount)
// TODO: {
// TODO:    return addParentVisArgsAndCall(XmCreatePromptDialog, parent, name); // TODO:, arglist, argcount);
// TODO: }
// TODO: 
// TODO: 
// TODO: Fl_Widget* CreateSelectionDialog(Fl_Widget* parent, char* name,
// TODO:                              ArgList arglist, int  argcount)
// TODO: {
// TODO:    Fl_Widget* dialog = addParentVisArgsAndCall(XmCreateSelectionDialog, parent, name,
// TODO:                                            arglist, argcount);
// TODO:    AddMouseWheelSupport(XmSelectionBoxGetChild(dialog, XmDIALOG_LIST));
// TODO:    return dialog;
// TODO: }
// TODO: 
// TODO: 
// TODO: Fl_Widget* CreateFormDialog(Fl_Widget* parent, char* name,
// TODO:                         ArgList arglist, int  argcount)
// TODO: {
// TODO:    return addParentVisArgsAndCall(XmCreateFormDialog, parent, name, arglist,
// TODO:                                   argcount);
// TODO: }
// TODO: 
// TODO: 
// TODO: Fl_Widget* CreateFileSelectionDialog(Fl_Widget* parent, char* name,
// TODO:                                  ArgList arglist, int  argcount)
// TODO: {
// TODO:    Fl_Widget* dialog = addParentVisArgsAndCall(XmCreateFileSelectionDialog, parent,
// TODO:                                            name, arglist, argcount);
// TODO: 
// TODO:    AddMouseWheelSupport(XmFileSelectionBoxGetChild(dialog, XmDIALOG_LIST));
// TODO:    AddMouseWheelSupport(XmFileSelectionBoxGetChild(dialog, XmDIALOG_DIR_LIST));
// TODO:    return dialog;
// TODO: }
// TODO: 
// TODO: 
// TODO: Fl_Widget* CreateQuestionDialog(Fl_Widget* parent, char* name,
// TODO:                             ArgList arglist, int  argcount)
// TODO: {
// TODO:    return addParentVisArgsAndCall(XmCreateQuestionDialog, parent, name,
// TODO:                                   arglist, argcount);
// TODO: }
// TODO: 
// TODO: 
// TODO: Fl_Widget* CreateMessageDialog(Fl_Widget* parent, char* name,
// TODO:                            ArgList arglist, int  argcount)
// TODO: {
// TODO:    return addParentVisArgsAndCall(XmCreateMessageDialog, parent, name,
// TODO:                                   arglist, argcount);
// TODO: }
// TODO: 
// TODO: 
// TODO: Fl_Widget* CreateErrorDialog(Fl_Widget* parent, char* name,
// TODO:                          ArgList arglist, int  argcount)
// TODO: {
// TODO:    return addParentVisArgsAndCall(XmCreateErrorDialog, parent, name, arglist,
// TODO:                                   argcount);
// TODO: }
// TODO: 
// TODO: Fl_Widget* CreateWidget(Fl_Widget* parent, const char* name, WidgetClass wClass,
// TODO:                     ArgList arglist, int  argcount)
// TODO: {
// TODO:    Fl_Widget* result;
// TODO:    ArgList al = addParentVisArgs(parent, arglist, &argcount);
// TODO:    result = XtCreateWidget(name, wClass, parent, al, argcount);
// TODO:    free__((char*)al);
// TODO:    return result;
// TODO: }
// TODO: 
// TODO: Fl_Widget* CreateShellWithBestVis(const char* appName, const char* appClass,
// TODO:                               WidgetClass wClass, Display* display, ArgList args, int nArgs)
// TODO: {
// TODO:    Visual* visual;
// TODO:    int depth;
// TODO:    Colormap colormap;
// TODO:    ArgList al;
// TODO:    int ac = nArgs;
// TODO:    Fl_Widget* result;
// TODO: 
// TODO:    FindBestVisual(display, appName, appClass, &visual, &depth, &colormap);
// TODO:    al = (ArgList)malloc__(sizeof(Arg) * (nArgs + 3));
// TODO:    if (nArgs != 0)
// TODO:       memcpy(al, args, sizeof(Arg) * nArgs);
// TODO:    XtSetArg(al[ac], XtNvisual, visual);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XtNdepth, depth);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XtNcolormap, colormap);
// TODO:    ac++;
// TODO:    result = XtAppCreateShell(appName, appClass, wClass, display, al, ac);
// TODO:    free__((char*)al);
// TODO:    return result;
// TODO: }
// TODO: 
// TODO: 
// TODO: Fl_Widget* CreatePopupShellWithBestVis(const char* shellName, WidgetClass wClass,
// TODO:                                    Fl_Widget* parent, ArgList arglist, int argcount)
// TODO: {
// TODO:    Fl_Widget* result;
// TODO:    ArgList al = addParentVisArgs(parent, arglist, &argcount);
// TODO:    result = XtCreatePopupShell(shellName, wClass, parent, al, argcount);
// TODO:    free__((char*)al);
// TODO:    return result;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Extends an argument list for widget creation with additional arguments
// TODO: ** for visual, colormap, and depth. The original argument list is not altered
// TODO: ** and it's the caller's responsability to free the returned list.
// TODO: */
// TODO: static ArgList addParentVisArgs(Fl_Widget* parent, ArgList arglist,
// TODO:                                 int* argcount)
// TODO: {
// TODO:    Visual* visual;
// TODO:    int depth;
// TODO:    Colormap colormap;
// TODO:    ArgList al;
// TODO:    Fl_Widget* parentShell = parent;
// TODO: 
// TODO:    /* Find the application/dialog/menu shell at the top of the widget
// TODO:       hierarchy, which has the visual resource being used */
// TODO:    while (true)
// TODO:    {
// TODO:       if (XtIsShell(parentShell))
// TODO:          break;
// TODO:       if (parentShell == NULL)
// TODO:       {
// TODO:          fprintf(stderr, "failed to find shell\n");
// TODO:          exit(EXIT_FAILURE);
// TODO:       }
// TODO:       parentShell = XtParent(parentShell);
// TODO:    }
// TODO: 
// TODO:    /* Add the visual, depth, and colormap resources to the argument list */
// TODO:    XtVaGetValues(parentShell, XtNvisual, &visual, XtNdepth, &depth,
// TODO:                  XtNcolormap, &colormap, NULL);
// TODO:    al = (ArgList)malloc__(sizeof(Arg) * ((*argcount) + 3));
// TODO:    if ((*argcount) != 0)
// TODO:       memcpy(al, arglist, sizeof(Arg) * (*argcount));
// TODO: 
// TODO:    /* For non-Lesstif versions, the visual, depth, and colormap are now set
// TODO:       globally via the resource database. So strictly spoken, it is no
// TODO:       longer necessary to set them explicitly for every shell widget.
// TODO: 
// TODO:       For Lesstif, however, this doesn't work. Luckily, Lesstif handles
// TODO:       non-default visuals etc. properly for its own shells and
// TODO:       we can take care of things for our shells (eg, call tips) here. */
// TODO:    XtSetArg(al[*argcount], XtNvisual, visual);
// TODO:    (*argcount)++;
// TODO:    XtSetArg(al[*argcount], XtNdepth, depth);
// TODO:    (*argcount)++;
// TODO:    XtSetArg(al[*argcount], XtNcolormap, colormap);
// TODO:    (*argcount)++;
// TODO:    return al;
// TODO: }
// TODO: 
// TODO: 
// TODO: /*
// TODO: ** Calls one of the Motif widget creation routines, splicing in additional
// TODO: ** arguments for visual, colormap, and depth.
// TODO: */
// TODO: static Fl_Widget* addParentVisArgsAndCall(MotifDialogCreationCall createRoutine,
// TODO:                                       Fl_Widget* parent, char* name, ArgList arglist, int argcount)
// TODO: {
// TODO:    Fl_Widget* result;
// TODO:    ArgList al = addParentVisArgs(parent, arglist, &argcount);
// TODO:    result = (*createRoutine)(parent, name, al, argcount);
// TODO:    free__((char*)al);
// TODO:    return result;
// TODO: }

/*
** ManageDialogCenteredOnPointer is used in place of XtManageChild for
** popping up a dialog to enable the dialog to be centered under the
** mouse pointer.  Whether it pops up the dialog centered under the pointer
** or in its default position centered over the parent widget, depends on
** the value set in the SetPointerCenteredDialogs call.
** Additionally, this function constrains the size of the dialog to the
** screen's size, to avoid insanely wide dialogs with obscured buttons.
*/
void ManageDialogCenteredOnPointer(Fl_Widget* dialogChild)
{
   if (PointerCenteredDialogsEnabled)
   {
      int mouseX, mouseY;
      Fl::get_mouse(mouseX, mouseY);

      int newX = mouseX - dialogChild->w() / 2;
      int newY = mouseY - dialogChild->h() / 2;

      if (newX < 30) newX = 30;
      if (newY < 50) newY = 50;

      int x, y, maxX, maxY;
      Fl::screen_xywh(x, y, maxX, maxY);
      if (newX + dialogChild->w() > maxX) newX = maxX - dialogChild->w();
      if (newY + dialogChild->y() > maxY) newY = maxY - dialogChild->h();

      dialogChild->position(newX, newY);
   }
}

/*
** Cause dialogs created by libNUtil.a routines (such as DialogF),
** and dialogs which use ManageDialogCenteredOnPointer to pop up
** over the pointer (state = true), or pop up in their default
** positions (state = false)
*/
void SetPointerCenteredDialogs(int state)
{
   PointerCenteredDialogsEnabled = state;
}


// TODO: /*
// TODO: ** Raise a window to the top and give it the input focus.  Setting input focus
// TODO: ** is important on systems which use explict (rather than pointer) focus.
// TODO: **
// TODO: ** The X alternatives XMapRaised, and XSetInputFocus both have problems.
// TODO: ** XMapRaised only gives the window the focus if it was initially not visible,
// TODO: ** and XSetInputFocus sets the input focus, but crashes if the window is not
// TODO: ** visible.
// TODO: **
// TODO: ** This routine should also be used in the case where a dialog is popped up and
// TODO: ** subsequent calls to the dialog function use a flag, or the XtIsManaged, to
// TODO: ** decide whether to create a new instance of the dialog, because on slower
// TODO: ** systems, events can intervene while a dialog is still on its way up and its
// TODO: ** window is still invisible, causing a subtle crash potential if
// TODO: ** XSetInputFocus is used.
// TODO: */
// TODO: void RaiseDialogWindow(Fl_Widget* shell)
// TODO: {
// TODO:    RaiseWindow(XtDisplay(shell), XtWindow(shell), true);
// TODO: }

void RaiseShellWindow(Fl_Widget* shell, bool focus)
{
   shell->show();// TODO:    RaiseWindow(XtDisplay(shell), XtWindow(shell), focus);
}

// TODO: void RaiseWindow(Display* display, Window w, bool focus)
// TODO: {
// TODO:    if (focus)
// TODO:    {
// TODO:       XWindowAttributes winAttr;
// TODO: 
// TODO:       XGetWindowAttributes(display, w, &winAttr);
// TODO:       if (winAttr.map_state == IsViewable)
// TODO:          XSetInputFocus(display, w, RevertToParent, CurrentTime);
// TODO:    }
// TODO: 
// TODO:    WmClientMsg(display, w, "_NET_ACTIVE_WINDOW", 0, 0, 0, 0, 0);
// TODO:    XMapRaised(display, w);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Add a handler for mnemonics in a dialog (Motif currently only handles
// TODO: ** mnemonics in menus) following the example of M.S. Windows.  To add
// TODO: ** mnemonics to a dialog, set the XmNmnemonic resource, as you would in
// TODO: ** a menu, on push buttons or toggle buttons, and call this function
// TODO: ** when the dialog is fully constructed.  Mnemonics added or changed
// TODO: ** after this call will not be noticed.  To add a mnemonic to a text field
// TODO: ** or list, set the XmNmnemonic resource on the appropriate label and set
// TODO: ** the XmNuserData resource of the label to the widget to get the focus
// TODO: ** when the mnemonic is typed.
// TODO: */
// TODO: void AddDialogMnemonicHandler(Fl_Widget* dialog, int unmodifiedToo)
// TODO: {
// TODO:    XtAddEventHandler(dialog, KeyPressMask, false, (XtEventHandler)mnemonicCB, (XtPointer)0);
// TODO:    addMnemonicGrabs(dialog, dialog, unmodifiedToo);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Removes the event handler and key-grabs added by AddDialogMnemonicHandler
// TODO: */
// TODO: void RemoveDialogMnemonicHandler(Fl_Widget* dialog)
// TODO: {
// TODO:    XtUngrabKey(dialog, AnyKey, Mod1Mask);
// TODO:    XtRemoveEventHandler(dialog, KeyPressMask, false,
// TODO:                         (XtEventHandler)mnemonicCB, (XtPointer)0);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Add additional key grabs for new menu items added to the menus, for
// TODO: ** patching around the Motif Caps/Num Lock problem. "topWidget" must be
// TODO: ** the same widget passed in the original call to AccelLockBugPatch.
// TODO: */
// TODO: void UpdateAccelLockPatch(Fl_Widget* topWidget, Fl_Widget* newButton)
// TODO: {
// TODO:    addAccelGrab(topWidget, newButton);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** PopDownBugPatch
// TODO: **
// TODO: ** Under some circumstances, popping down a dialog and its parent in
// TODO: ** rapid succession causes a crash.  This routine delays and
// TODO: ** processs events until receiving a ReparentNotify event.
// TODO: ** (I have no idea why a ReparentNotify event occurs at all, but it does
// TODO: ** mark the point where it is safe to destroy or pop down the parent, and
// TODO: ** it might have something to do with the bug.)  There is a failsafe in
// TODO: ** the form of a ~1.5 second timeout in case no ReparentNotify arrives.
// TODO: ** Use this sparingly, only when real crashes are observed, and periodically
// TODO: ** check to make sure that it is still necessary.
// TODO: */
// TODO: void PopDownBugPatch(Fl_Widget* w)
// TODO: {
// TODO:    time_t stopTime;
// TODO: 
// TODO:    stopTime = time(NULL) + 1;
// TODO:    while (time(NULL) <= stopTime)
// TODO:    {
// TODO:       XEvent event;
// TODO:       XtAppContext context = XtWidgetToApplicationContext(w);
// TODO:       XtAppPeekEvent(context, &event);
// TODO:       if (event.xany.type == ReparentNotify)
// TODO:          return;
// TODO:       XtAppProcessEvent(context, XtIMAll);
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Convert a compound string to a C style null terminated string.
// TODO: ** Returned string must be freed by the caller.
// TODO: */
// TODO: char* GetNeStringText(NeString fromString)
// TODO: {
// TODO:    XmStringContext context;
// TODO:    char* text, *toPtr, *toString, *fromPtr;
// TODO:    XmStringCharSet charset;
// TODO:    XmStringDirection direction;
// TODO:    bool separator;
// TODO: 
// TODO:    /* Malloc a buffer large enough to hold the string.  XmStringLength
// TODO:       should always be slightly longer than necessary, but won't be
// TODO:       shorter than the equivalent null-terminated string */
// TODO:    toString = malloc__(XmStringLength(fromString));
// TODO: 
// TODO:    /* loop over all of the segments in the string, copying each segment
// TODO:       into the output string and converting separators into newlines */
// TODO:    XmStringInitContext(&context, fromString);
// TODO:    toPtr = toString;
// TODO:    while (XmStringGetNextSegment(context, &text,
// TODO:                                  &charset, &direction, &separator))
// TODO:    {
// TODO:       for (fromPtr=text; *fromPtr!='\0'; fromPtr++)
// TODO:          *toPtr++ = *fromPtr;
// TODO:       if (separator)
// TODO:          *toPtr++ = '\n';
// TODO:       free__(text);
// TODO:       free__(charset);
// TODO:    }
// TODO: 
// TODO:    /* terminate the string, free the context, and return the string */
// TODO:    *toPtr++ = '\0';
// TODO:    XmStringFreeContext(context);
// TODO:    return toString;
// TODO: }

/*
** Get the XFontStruct that corresponds to the default (first) font in
** a Motif font list.  Since Motif stores this, it saves us from storing
** it or querying it from the X server.
*/
Ne_Font GetDefaultFontStruct(const Ne_Font& font)
{
   return font;
}

// TODO: /*
// TODO: ** Create a string table suitable for passing to XmList widgets
// TODO: */
// TODO: NeString* StringTable(int count, ...)
// TODO: {
// TODO:    va_list ap;
// TODO:    NeString* array;
// TODO:    int i;
// TODO:    char* str;
// TODO: 
// TODO:    va_start(ap, count);
// TODO:    array = (NeString*)malloc__((count+1) * sizeof(NeString));
// TODO:    for (i = 0;  i < count; i++)
// TODO:    {
// TODO:       str = va_arg(ap, char*);
// TODO:       array[i] = NeNewString(str);
// TODO:    }
// TODO:    array[i] = (NeString)0;
// TODO:    va_end(ap);
// TODO:    return(array);
// TODO: }
// TODO: 
// TODO: void FreeStringTable(NeString* table)
// TODO: {
// TODO:    int i;
// TODO: 
// TODO:    for (i = 0; table[i] != 0; i++)
// TODO:       NeStringFree(table[i]);
// TODO:    free__((char*)table);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Simulate a button press.  The purpose of this routine is show users what
// TODO: ** is happening when they take an action with a non-obvious side effect,
// TODO: ** such as when a user double clicks on a list item.  The argument is an
// TODO: ** XmPushButton widget to "press"
// TODO: */
// TODO: void SimulateButtonPress(Fl_Widget* widget)
// TODO: {
// TODO:    XEvent keyEvent;
// TODO: 
// TODO:    memset((char*)&keyEvent, 0, sizeof(XKeyPressedEvent));
// TODO:    keyEvent.type = KeyPress;
// TODO:    keyEvent.xkey.serial = 1;
// TODO:    keyEvent.xkey.send_event = true;
// TODO: 
// TODO:    if (XtIsSubclass(widget, xmGadgetClass))
// TODO:    {
// TODO:       /* On some Motif implementations, asking a gadget for its
// TODO:          window will crash, rather than return the window of its
// TODO:          parent. */
// TODO:       Fl_Widget* parent = XtParent(widget);
// TODO:       keyEvent.xkey.display = XtDisplay(parent);
// TODO:       keyEvent.xkey.window = XtWindow(parent);
// TODO: 
// TODO:       XtCallActionProc(parent, "ManagerGadgetSelect",
// TODO:                        &keyEvent, NULL, 0);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       keyEvent.xkey.display = XtDisplay(widget);
// TODO:       keyEvent.xkey.window = XtWindow(widget);
// TODO: 
// TODO:       XtCallActionProc(widget, "ArmAndActivate", &keyEvent, NULL, 0);
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Add an item to an already established pull-down or pop-up menu, including
// TODO: ** mnemonics, accelerators and callbacks.
// TODO: */
// TODO: Fl_Widget* AddMenuItem(Fl_Widget* parent, char* name, char* label,
// TODO:                    char mnemonic, char* acc, char* accText,
// TODO:                    XtCallbackProc callback, void* cbArg)
// TODO: {
// TODO:    Fl_Widget* button;
// TODO:    NeString st1, st2;
// TODO: 
// TODO:    button = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent,
// TODO:                                     XmNlabelString, st1=NeNewString(label),
// TODO:                                     XmNmnemonic, mnemonic,
// TODO:                                     XmNacceleratorText, st2=NeNewString(accText),
// TODO:                                     XmNaccelerator, acc, NULL);
// TODO:    XtAddCallback(button, XmNactivateCallback, callback, cbArg);
// TODO:    NeStringFree(st1);
// TODO:    NeStringFree(st2);
// TODO:    return button;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Add a toggle button item to an already established pull-down or pop-up
// TODO: ** menu, including mnemonics, accelerators and callbacks.
// TODO: */
// TODO: Fl_Widget* AddMenuToggle(Fl_Widget* parent, char* name, char* label,
// TODO:                      char mnemonic, char* acc, char* accText,
// TODO:                      XtCallbackProc callback, void* cbArg, int set)
// TODO: {
// TODO:    Fl_Widget* button;
// TODO:    NeString st1, st2;
// TODO: 
// TODO:    button = XtVaCreateManagedWidget(name, xmToggleButtonWidgetClass, parent,
// TODO:                                     XmNlabelString, st1=NeNewString(label),
// TODO:                                     XmNmnemonic, mnemonic,
// TODO:                                     XmNacceleratorText, st2=NeNewString(accText),
// TODO:                                     XmNaccelerator, acc,
// TODO:                                     XmNset, set, NULL);
// TODO:    XtAddCallback(button, XmNvalueChangedCallback, callback, cbArg);
// TODO:    NeStringFree(st1);
// TODO:    NeStringFree(st2);
// TODO:    return button;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Add a sub-menu to an established pull-down or pop-up menu, including
// TODO: ** mnemonics, accelerators and callbacks.  Returns the menu pane of the
// TODO: ** new sub menu.
// TODO: */
// TODO: Fl_Widget* AddSubMenu(Fl_Widget* parent, char* name, char* label, char mnemonic)
// TODO: {
// TODO:    Fl_Widget* menu;
// TODO:    NeString st1;
// TODO: 
// TODO:    menu = CreatePulldownMenu(parent, name, NULL, 0);
// TODO:    XtVaCreateManagedWidget(name, xmCascadeButtonWidgetClass, parent,
// TODO:                            XmNlabelString, st1=NeNewString(label),
// TODO:                            XmNmnemonic, mnemonic,
// TODO:                            XmNsubMenuId, menu, NULL);
// TODO:    NeStringFree(st1);
// TODO:    return menu;
// TODO: }

/*
** SetIntText
**
** Set the text of a motif label to show an integer
*/
void SetIntText(Fl_Input* text, int value)
{
   char labelString[20] = "";
   sprintf(labelString, "%d", value);
   text->value(labelString);
}

// TODO: /*
// TODO: ** GetIntText, GetFloatText, GetIntTextWarn, GetFloatTextWarn
// TODO: **
// TODO: ** Get the text of a motif text widget as an integer or floating point number.
// TODO: ** The functions will return TEXT_READ_OK of the value was read correctly.
// TODO: ** If not, they will return either TEXT_IS_BLANK, or TEXT_NOT_NUMBER.  The
// TODO: ** GetIntTextWarn and GetFloatTextWarn will display a dialog warning the
// TODO: ** user that the value could not be read.  The argument fieldName is used
// TODO: ** in the dialog to help the user identify where the problem is.  Set
// TODO: ** warnBlank to true if a blank field is also considered an error.
// TODO: */
// TODO: int GetFloatText(Fl_Widget* text, double* value)
// TODO: {
// TODO:    char* strValue, *endPtr;
// TODO:    int retVal;
// TODO: 
// TODO:    strValue = NeTextGetString(text);	/* Get Value */
// TODO:    removeWhiteSpace(strValue);		/* Remove blanks and tabs */
// TODO:    *value = strtod(strValue, &endPtr);	/* Convert string to double */
// TODO:    if (strlen(strValue) == 0)		/* const char* is empty */
// TODO:       retVal = TEXT_IS_BLANK;
// TODO:    else if (*endPtr != '\0')		/* Whole string not parsed */
// TODO:       retVal = TEXT_NOT_NUMBER;
// TODO:    else
// TODO:       retVal = TEXT_READ_OK;
// TODO:    free__(strValue);
// TODO:    return retVal;
// TODO: }

int GetIntText(Fl_Input* text, int* value)
{
   char*endPtr;
   int retVal = 0;

   std::string strValue = text->value();
   Trim(strValue);
   *value = strtol(strValue.c_str(), &endPtr, 10);
   if (strValue.empty())
      retVal = TEXT_IS_BLANK;
   else if (*endPtr != '\0')			/* Whole string not parsed */
      retVal = TEXT_NOT_NUMBER;
   else
      retVal = TEXT_READ_OK;

   return retVal;
}

// TODO: int GetFloatTextWarn(Fl_Widget* text, double* value, const char* fieldName,
// TODO:                      int warnBlank)
// TODO: {
// TODO:    int result;
// TODO:    char* valueStr;
// TODO: 
// TODO:    result = GetFloatText(text, value);
// TODO:    if (result == TEXT_READ_OK || (result == TEXT_IS_BLANK && !warnBlank))
// TODO:       return result;
// TODO:    valueStr = NeTextGetString(text);
// TODO: 
// TODO:    if (result == TEXT_IS_BLANK)
// TODO:    {
// TODO:       DialogF(DF_ERR, text, 1, "Warning", "Please supply %s value", "OK", fieldName);
// TODO:    }
// TODO:    else   /* TEXT_NOT_NUMBER */
// TODO:    {
// TODO:       DialogF(DF_ERR, text, 1, "Warning", "Can't read %s value: \"%s\"", "OK", fieldName, valueStr);
// TODO:    }
// TODO: 
// TODO:    free__(valueStr);
// TODO:    return result;
// TODO: }

int GetIntTextWarn(Fl_Input* text, int* value, const char* fieldName, int warnBlank)
{
   int result = GetIntText(text, value);
   if (result == TEXT_READ_OK || (result == TEXT_IS_BLANK && !warnBlank))
      return result;
   
   if (result == TEXT_IS_BLANK)
   {
      DialogF(DF_ERR, text, 1, "Warning", "Please supply a value for %s", "OK", fieldName);
   }
   else   /* TEXT_NOT_NUMBER */
   {
      DialogF(DF_ERR, text, 1, "Warning", "Can't read integer value \"%s\" in %s", "OK", text->value(), fieldName);
   }
   return result;
}

bool TextWidgetIsBlank(Fl_Input* textW)
{
   char* str = NeTextGetString(textW);
   removeWhiteSpace(str);
   bool retVal = (*str == '\0');
   delete[] str;
   return retVal;
}

// TODO: /*
// TODO: ** Turn a multi-line editing text widget into a fake single line text area
// TODO: ** by disabling the translation for Return.  This is a way to give users
// TODO: ** extra space, by allowing wrapping, but still prohibiting newlines.
// TODO: ** (SINGLE_LINE_EDIT mode can't be used, in this case, because it forces
// TODO: ** the widget to be one line high).
// TODO: */
// TODO: void MakeSingleLineTextW(Fl_Widget* textW)
// TODO: {
// TODO:    static XtTranslations noReturnTable = NULL;
// TODO:    static char* noReturnTranslations = "<Key>Return: activate()\n";
// TODO: 
// TODO:    if (noReturnTable == NULL)
// TODO:       noReturnTable = XtParseTranslationTable(noReturnTranslations);
// TODO:    XtOverrideTranslations(textW, noReturnTable);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Add up-arrow/down-arrow recall to a single line text field.  When user
// TODO: ** presses up-arrow, string is cleared and recent entries are presented,
// TODO: ** moving to older ones as each successive up-arrow is pressed.  Down-arrow
// TODO: ** moves to more recent ones, final down-arrow clears the field.  Associated
// TODO: ** routine, AddToHistoryList, makes maintaining a history list easier.
// TODO: **
// TODO: ** Arguments are the widget, and pointers to the history list and number of
// TODO: ** items, which are expected to change periodically.
// TODO: */
// TODO: void AddHistoryToTextWidget(Fl_Widget* textW, char** *historyList, int* nItems)
// TODO: {
// TODO:    histInfo* histData;
// TODO: 
// TODO:    /* create a data structure for passing history info to the callbacks */
// TODO:    histData = (histInfo*)malloc__(sizeof(histInfo));
// TODO:    histData->list = historyList;
// TODO:    histData->nItems = nItems;
// TODO:    histData->index = -1;
// TODO: 
// TODO:    /* Add an event handler for handling up/down arrow events */
// TODO:    XtAddEventHandler(textW, KeyPressMask, false,
// TODO:                      (XtEventHandler)histArrowKeyEH, histData);
// TODO: 
// TODO:    /* Add a destroy callback for freeing history data structure */
// TODO:    XtAddCallback(textW, XmNdestroyCallback, histDestroyCB, histData);
// TODO: }
// TODO: 
// TODO: static void histDestroyCB(Fl_Widget* w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    free__((char*)clientData);
// TODO: }
// TODO: 
// TODO: static void histArrowKeyEH(Fl_Widget* w, XtPointer callData, int event,
// TODO:                            bool* continueDispatch)
// TODO: {
// TODO:    histInfo* histData = (histInfo*)callData;
// TODO:    KeySym keysym = XLookupKeysym((XKeyEvent*)event, 0);
// TODO: 
// TODO:    /* only process up and down arrow keys */
// TODO:    if (keysym != XK_Up && keysym != XK_Down)
// TODO:       return;
// TODO: 
// TODO:    /* increment or decrement the index depending on which arrow was pressed */
// TODO:    histData->index += (keysym == XK_Up) ? 1 : -1;
// TODO: 
// TODO:    /* if the index is out of range, beep, fix it up, and return */
// TODO:    if (histData->index < -1)
// TODO:    {
// TODO:       histData->index = -1;
// TODO:       XBell(XtDisplay(w), 0);
// TODO:       return;
// TODO:    }
// TODO:    if (histData->index >= *histData->nItems)
// TODO:    {
// TODO:       histData->index = *histData->nItems - 1;
// TODO:       XBell(XtDisplay(w), 0);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Change the text field contents */
// TODO:    NeTextSetString(w, histData->index == -1 ? "" :
// TODO:                    (*histData->list)[histData->index]);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Copies a string on to the end of history list, which may be reallocated
// TODO: ** to make room.  If historyList grows beyond its internally set boundary
// TODO: ** for size (HISTORY_LIST_MAX), it is trimmed back to a smaller size
// TODO: ** (HISTORY_LIST_TRIM_TO).  Before adding to the list, checks if the item
// TODO: ** is a duplicate of the last item.  If so, it is not added.
// TODO: */
// TODO: void AddToHistoryList(char* newItem, char** *historyList, int* nItems)
// TODO: {
// TODO:    char** newList;
// TODO:    int i;
// TODO: 
// TODO:    if (*nItems != 0 && !strcmp(newItem, **historyList))
// TODO:       return;
// TODO:    if (*nItems == HISTORY_LIST_MAX)
// TODO:    {
// TODO:       for (i=HISTORY_LIST_TRIM_TO; i<HISTORY_LIST_MAX; i++)
// TODO:          free__((*historyList)[i]);
// TODO:       *nItems = HISTORY_LIST_TRIM_TO;
// TODO:    }
// TODO:    newList = (char**)malloc__(sizeof(char*) * (*nItems + 1));
// TODO:    for (i=0; i < *nItems; i++)
// TODO:       newList[i+1] = (*historyList)[i];
// TODO:    if (*nItems != 0 && *historyList != NULL)
// TODO:       free__((char*)*historyList);
// TODO:    (*nItems)++;
// TODO:    newList[0] = NeNewString(newItem);
// TODO:    *historyList = newList;
// TODO: }
// TODO: 
/*
** BeginWait/EndWait
**
** Display/Remove a watch cursor over topCursorWidget and its descendents
*/
void BeginWait(Fl_Window* win)
{
   win->cursor(FL_CURSOR_WAIT);
}

// TODO: void BusyWait(Fl_Widget* widget)
// TODO: {
// TODO: #ifdef __unix__
// TODO:    static const int timeout = 100000;  /* 1/10 sec = 100 ms = 100,000 us */
// TODO:    static struct timeval last = { 0, 0 };
// TODO:    struct timeval current;
// TODO:    gettimeofday(&current, NULL);
// TODO: 
// TODO:    if ((current.tv_sec != last.tv_sec) ||
// TODO:          (current.tv_usec - last.tv_usec > timeout))
// TODO:    {
// TODO:       XmUpdateDisplay(widget);
// TODO:       last = current;
// TODO:    }
// TODO: #else
// TODO:    static time_t last = 0;
// TODO:    time_t current;
// TODO:    time(&current);
// TODO: 
// TODO:    if (difftime(current, last) > 0)
// TODO:    {
// TODO:       XmUpdateDisplay(widget);
// TODO:       last = current;
// TODO:    }
// TODO: #endif
// TODO: }

void EndWait(Fl_Window* win)
{
   win->cursor(FL_CURSOR_DEFAULT);
}

// TODO: /*
// TODO: ** Create an X window geometry string from width, height, x, and y values.
// TODO: ** This is a complement to the X routine XParseGeometry, and uses the same
// TODO: ** bitmask values (XValue, YValue, WidthValue, HeightValue, XNegative, and
// TODO: ** YNegative) as defined in <X11/Xutil.h> and documented under XParseGeometry.
// TODO: ** It expects a string of at least MAX_GEOMETRY_STRING_LEN in which to write
// TODO: ** result.  Note that in a geometry string, it is not possible to supply a y
// TODO: ** position without an x position.  Also note that the X/YNegative flags
// TODO: ** mean "add a '-' and negate the value" which is kind of odd.
// TODO: */
// TODO: void CreateGeometryString(char* string, int x, int y,
// TODO:                           int width, int height, int bitmask)
// TODO: {
// TODO:    char* ptr = string;
// TODO:    int nChars;
// TODO: 
// TODO:    if (bitmask & WidthValue)
// TODO:    {
// TODO:       sprintf(ptr, "%d%n", width, &nChars);
// TODO:       ptr += nChars;
// TODO:    }
// TODO:    if (bitmask & HeightValue)
// TODO:    {
// TODO:       sprintf(ptr, "x%d%n", height, &nChars);
// TODO:       ptr += nChars;
// TODO:    }
// TODO:    if (bitmask & XValue)
// TODO:    {
// TODO:       if (bitmask & XNegative)
// TODO:          sprintf(ptr, "-%d%n", -x, &nChars);
// TODO:       else
// TODO:          sprintf(ptr, "+%d%n", x, &nChars);
// TODO:       ptr += nChars;
// TODO:    }
// TODO:    if (bitmask & YValue)
// TODO:    {
// TODO:       if (bitmask & YNegative)
// TODO:          sprintf(ptr, "-%d%n", -y, &nChars);
// TODO:       else
// TODO:          sprintf(ptr, "+%d%n", y, &nChars);
// TODO:       ptr += nChars;
// TODO:    }
// TODO:    *ptr = '\0';
// TODO: }

/*
** Remove the white space (blanks and tabs) from a string
*/
static void removeWhiteSpace(char* string)
{
   char* outPtr = string;

   while(1)
   {
      if (*string == 0)
      {
         *outPtr = 0;
         return;
      }
      else if (*string != ' ' && *string != '\t')
         *(outPtr++) = *(string++);
      else
         string++;
   }
}

// TODO: /*
// TODO: ** Compares two strings and return TRUE if the two strings
// TODO: ** are the same, ignoring whitespace and case differences.
// TODO: */
// TODO: static int stripCaseCmp(const char* str1, const char* str2)
// TODO: {
// TODO:    const char* c1, *c2;
// TODO: 
// TODO:    for (c1=str1, c2=str2; *c1!='\0' && *c2!='\0'; c1++, c2++)
// TODO:    {
// TODO:       while (*c1 == ' ' || *c1 == '\t')
// TODO:          c1++;
// TODO:       while (*c2 == ' ' || *c2 == '\t')
// TODO:          c2++;
// TODO:       if (toupper((unsigned char)*c1) != toupper((unsigned char)*c2))
// TODO:          return FALSE;
// TODO:    }
// TODO:    return *c1 == '\0' && *c2 == '\0';
// TODO: }
// TODO: 
// TODO: static void warnHandlerCB(const char* message)
// TODO: {
// TODO:    if (strstr(message, "XtRemoveGrab"))
// TODO:       return;
// TODO:    if (strstr(message, "Attempt to remove non-existant passive grab"))
// TODO:       return;
// TODO:    fputs(message, stderr);
// TODO:    fputc('\n', stderr);
// TODO: }
// TODO: 
// TODO: static XModifierKeymap* getKeyboardMapping(Display* display)
// TODO: {
// TODO:    static XModifierKeymap* keyboardMap = NULL;
// TODO: 
// TODO:    if (keyboardMap == NULL)
// TODO:    {
// TODO:       keyboardMap = XGetModifierMapping(display);
// TODO:    }
// TODO:    return(keyboardMap);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** get mask for a modifier
// TODO: **
// TODO: */
// TODO: 
// TODO: static Modifiers findModifierMapping(Display* display, KeyCode keyCode)
// TODO: {
// TODO:    int i, j;
// TODO:    KeyCode* mapentry;
// TODO:    XModifierKeymap* modMap = getKeyboardMapping(display);
// TODO: 
// TODO:    if (modMap == NULL || keyCode == 0)
// TODO:    {
// TODO:       return(0);
// TODO:    }
// TODO: 
// TODO:    mapentry = modMap->modifiermap;
// TODO:    for (i = 0; i < 8; ++i)
// TODO:    {
// TODO:       for (j = 0; j < (modMap->max_keypermod); ++j)
// TODO:       {
// TODO:          if (keyCode == *mapentry)
// TODO:          {
// TODO:             return(1 << ((mapentry - modMap->modifiermap) / modMap->max_keypermod));
// TODO:          }
// TODO:          ++mapentry;
// TODO:       }
// TODO:    }
// TODO:    return(0);
// TODO: }
// TODO: 
// TODO: Modifiers GetNumLockModMask(Display* display)
// TODO: {
// TODO:    static int numLockMask = -1;
// TODO: 
// TODO:    if (numLockMask == -1)
// TODO:    {
// TODO:       numLockMask = findModifierMapping(display, XKeysymToKeycode(display, XK_Num_Lock));
// TODO:    }
// TODO:    return(numLockMask);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Grab a key regardless of caps-lock and other silly latching keys.
// TODO: **
// TODO: */
// TODO: 
// TODO: static void reallyGrabAKey(Fl_Widget* dialog, int keyCode, Modifiers mask)
// TODO: {
// TODO:    Modifiers numLockMask = GetNumLockModMask(XtDisplay(dialog));
// TODO: 
// TODO:    if (keyCode == 0)  /* No anykey grabs, sorry */
// TODO:       return;
// TODO: 
// TODO:    XtGrabKey(dialog, keyCode, mask, true, GrabModeAsync, GrabModeAsync);
// TODO:    XtGrabKey(dialog, keyCode, mask|LockMask, true, GrabModeAsync, GrabModeAsync);
// TODO:    if (numLockMask && numLockMask != LockMask)
// TODO:    {
// TODO:       XtGrabKey(dialog, keyCode, mask|numLockMask, true, GrabModeAsync, GrabModeAsync);
// TODO:       XtGrabKey(dialog, keyCode, mask|LockMask|numLockMask, true, GrabModeAsync, GrabModeAsync);
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Part of dialog mnemonic processing.  Search the widget tree under w
// TODO: ** for widgets with mnemonics.  When found, add a passive grab to the
// TODO: ** dialog widget for the mnemonic character, thus directing mnemonic
// TODO: ** events to the dialog widget.
// TODO: */
// TODO: static void addMnemonicGrabs(Fl_Widget* dialog, Fl_Widget* w, int unmodifiedToo)
// TODO: {
// TODO:    char mneString[2];
// TODO:    WidgetList children;
// TODO:    int numChildren;
// TODO:    int i, isMenu;
// TODO:    KeySym mnemonic = '\0';
// TODO:    unsigned char rowColType;
// TODO:    unsigned int keyCode;
// TODO: 
// TODO:    if (XtIsComposite(w))
// TODO:    {
// TODO:       if (XtClass(w) == xmRowColumnWidgetClass)
// TODO:       {
// TODO:          XtVaGetValues(w, XmNrowColumnType, &rowColType, NULL);
// TODO:          isMenu = rowColType != XmWORK_AREA;
// TODO:       }
// TODO:       else
// TODO:          isMenu = false;
// TODO:       if (!isMenu)
// TODO:       {
// TODO:          XtVaGetValues(w, XmNchildren, &children, XmNnumChildren,
// TODO:                        &numChildren, NULL);
// TODO:          for (i=0; i<(int)numChildren; i++)
// TODO:             addMnemonicGrabs(dialog, children[i], unmodifiedToo);
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       XtVaGetValues(w, XmNmnemonic, &mnemonic, NULL);
// TODO:       if (mnemonic != XK_VoidSymbol && mnemonic != '\0')
// TODO:       {
// TODO:          mneString[0] = mnemonic;
// TODO:          mneString[1] = '\0';
// TODO:          keyCode = XKeysymToKeycode(XtDisplay(dialog),
// TODO:                                     NeStringToKeysym(mneString));
// TODO:          reallyGrabAKey(dialog, keyCode, Mod1Mask);
// TODO:          if (unmodifiedToo)
// TODO:             reallyGrabAKey(dialog, keyCode, 0);
// TODO:       }
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Callback routine for dialog mnemonic processing.
// TODO: */
// TODO: static void mnemonicCB(Fl_Widget* w, XtPointer callData, XKeyEvent* event)
// TODO: {
// TODO:    findAndActivateMnemonic(w, event->keycode);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Look for a widget in the widget tree w, with a mnemonic matching
// TODO: ** keycode.  When one is found, simulate a button press on that widget
// TODO: ** and give it the keyboard focus.  If the mnemonic is on a label,
// TODO: ** look in the userData field of the label to see if it points to
// TODO: ** another widget, and give that the focus.  This routine is just
// TODO: ** sufficient for NEdit, no doubt it will need to be extended for
// TODO: ** mnemonics on widgets other than just buttons and text fields.
// TODO: */
// TODO: static void findAndActivateMnemonic(Fl_Widget* w, unsigned int keycode)
// TODO: {
// TODO:    WidgetList children;
// TODO:    int numChildren;
// TODO:    int i, isMenu;
// TODO:    KeySym mnemonic = '\0';
// TODO:    char mneString[2];
// TODO:    Fl_Widget* userData;
// TODO:    unsigned char rowColType;
// TODO: 
// TODO:    if (XtIsComposite(w))
// TODO:    {
// TODO:       if (XtClass(w) == xmRowColumnWidgetClass)
// TODO:       {
// TODO:          XtVaGetValues(w, XmNrowColumnType, &rowColType, NULL);
// TODO:          isMenu = rowColType != XmWORK_AREA;
// TODO:       }
// TODO:       else
// TODO:          isMenu = false;
// TODO:       if (!isMenu)
// TODO:       {
// TODO:          XtVaGetValues(w, XmNchildren, &children, XmNnumChildren,
// TODO:                        &numChildren, NULL);
// TODO:          for (i=0; i<(int)numChildren; i++)
// TODO:             findAndActivateMnemonic(children[i], keycode);
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       XtVaGetValues(w, XmNmnemonic, &mnemonic, NULL);
// TODO:       if (mnemonic != '\0')
// TODO:       {
// TODO:          mneString[0] = mnemonic;
// TODO:          mneString[1] = '\0';
// TODO:          if (XKeysymToKeycode(XtDisplay(XtParent(w)),
// TODO:                               NeStringToKeysym(mneString)) == keycode)
// TODO:          {
// TODO:             if (XtClass(w) == xmLabelWidgetClass ||
// TODO:                   XtClass(w) == xmLabelGadgetClass)
// TODO:             {
// TODO:                XtVaGetValues(w, XmNuserData, &userData, NULL);
// TODO:                if (userData!=NULL && XtIsWidget(userData) &&
// TODO:                      XmIsTraversable(userData))
// TODO:                   XmProcessTraversal(userData, XmTRAVERSE_CURRENT);
// TODO:             }
// TODO:             else if (XmIsTraversable(w))
// TODO:             {
// TODO:                XmProcessTraversal(w, XmTRAVERSE_CURRENT);
// TODO:                SimulateButtonPress(w);
// TODO:             }
// TODO:          }
// TODO:       }
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Part of workaround for Motif Caps/Num Lock bug.  Search the widget tree
// TODO: ** under w for widgets with accelerators.  When found, add three passive
// TODO: ** grabs to topWidget, one for the accelerator keysym + modifiers + Caps
// TODO: ** Lock, one for Num Lock, and one for both, thus directing lock +
// TODO: ** accelerator events to topWidget.
// TODO: */
// TODO: static void addAccelGrabs(Fl_Widget* topWidget, Fl_Widget* w)
// TODO: {
// TODO:    WidgetList children;
// TODO:    Fl_Widget* menu;
// TODO:    int numChildren;
// TODO:    int i;
// TODO: 
// TODO:    if (XtIsComposite(w))
// TODO:    {
// TODO:       XtVaGetValues(w, XmNchildren, &children, XmNnumChildren,
// TODO:                     &numChildren, NULL);
// TODO:       for (i=0; i<(int)numChildren; i++)
// TODO:          addAccelGrabs(topWidget, children[i]);
// TODO:    }
// TODO:    else if (XtClass(w) == xmCascadeButtonWidgetClass)
// TODO:    {
// TODO:       XtVaGetValues(w, XmNsubMenuId, &menu, NULL);
// TODO:       if (menu != NULL)
// TODO:          addAccelGrabs(topWidget, menu);
// TODO:    }
// TODO:    else
// TODO:       addAccelGrab(topWidget, w);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Grabs the key + modifier defined in the widget's accelerator resource,
// TODO: ** in combination with the Caps Lock and Num Lock accelerators.
// TODO: */
// TODO: static void addAccelGrab(Fl_Widget* topWidget, Fl_Widget* w)
// TODO: {
// TODO:    char* accelString = NULL;
// TODO:    KeySym keysym;
// TODO:    unsigned int modifiers;
// TODO:    KeyCode code;
// TODO:    Modifiers numLockMask = GetNumLockModMask(XtDisplay(topWidget));
// TODO: 
// TODO:    XtVaGetValues(w, XmNaccelerator, &accelString, NULL);
// TODO:    if (accelString == NULL || *accelString == '\0')
// TODO:    {
// TODO:       free__(accelString);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    if (!parseAccelString(XtDisplay(topWidget), accelString, &keysym, &modifiers))
// TODO:    {
// TODO:       free__(accelString);
// TODO:       return;
// TODO:    }
// TODO:    free__(accelString);
// TODO: 
// TODO:    /* Check to see if this server has this key mapped.  Some cruddy PC X
// TODO:       servers (Xoftware) have terrible default keymaps. If not,
// TODO:       XKeysymToKeycode will return 0.  However, it's bad news to pass
// TODO:       that to XtGrabKey because 0 is really "AnyKey" which is definitely
// TODO:       not what we want!! */
// TODO: 
// TODO:    code = XKeysymToKeycode(XtDisplay(topWidget), keysym);
// TODO:    if (code == 0)
// TODO:       return;
// TODO: 
// TODO:    XtGrabKey(topWidget, code,
// TODO:              modifiers | LockMask, true, GrabModeAsync, GrabModeAsync);
// TODO:    if (numLockMask && numLockMask != LockMask)
// TODO:    {
// TODO:       XtGrabKey(topWidget, code,
// TODO:                 modifiers | numLockMask, true, GrabModeAsync, GrabModeAsync);
// TODO:       XtGrabKey(topWidget, code,
// TODO:                 modifiers | LockMask | numLockMask, true, GrabModeAsync, GrabModeAsync);
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Read a Motif accelerator string and translate it into a keysym + modifiers.
// TODO: ** Returns TRUE if the parse was successful, FALSE, if not.
// TODO: */
// TODO: static int parseAccelString(Display* display, const char* string, KeySym* keySym,
// TODO:                             unsigned int* modifiers)
// TODO: {
// TODO: #define N_MODIFIERS 12
// TODO:    /*... Is NumLock always Mod3? */
// TODO:    static char* modifierNames[N_MODIFIERS] = {"Ctrl", "Shift", "Alt", "Mod2",
// TODO:          "Mod3", "Mod4", "Mod5", "Button1", "Button2", "Button3", "Button4",
// TODO:          "Button5"
// TODO:                                              };
// TODO:    static unsigned int modifierMasks[N_MODIFIERS] = {ControlMask, ShiftMask,
// TODO:          Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask, Button1Mask, Button2Mask,
// TODO:          Button3Mask, Button4Mask, Button5Mask
// TODO:                                                     };
// TODO:    Modifiers numLockMask = GetNumLockModMask(display);
// TODO:    char modStr[MAX_ACCEL_LEN];
// TODO:    char evtStr[MAX_ACCEL_LEN];
// TODO:    char keyStr[MAX_ACCEL_LEN];
// TODO:    const char* c, *evtStart, *keyStart;
// TODO:    int i;
// TODO: 
// TODO:    if (strlen(string) >= MAX_ACCEL_LEN)
// TODO:       return FALSE;
// TODO: 
// TODO:    /* Get the modifier part */
// TODO:    for (c = string; *c != '<'; c++)
// TODO:       if (*c == '\0')
// TODO:          return FALSE;
// TODO:    strncpy(modStr, string, c - string);
// TODO:    modStr[c - string] = '\0';
// TODO: 
// TODO:    /* Verify the <key> or <keypress> part */
// TODO:    evtStart = c;
// TODO:    for (; *c != '>'; c++)
// TODO:       if (*c == '\0')
// TODO:          return FALSE;
// TODO:    c++;
// TODO:    strncpy(evtStr, evtStart, c - evtStart);
// TODO:    evtStr[c - evtStart] = '\0';
// TODO:    if (!stripCaseCmp(evtStr, "<key>") && !stripCaseCmp(evtStr, "<keypress>"))
// TODO:       return FALSE;
// TODO: 
// TODO:    /* Get the keysym part */
// TODO:    keyStart = c;
// TODO:    for (; *c != '\0' && !(c != keyStart && *c == ':'); c++);
// TODO:    strncpy(keyStr, keyStart, c - keyStart);
// TODO:    keyStr[c - keyStart] = '\0';
// TODO:    *keySym = NeStringToKeysym(keyStr);
// TODO: 
// TODO:    /* Parse the modifier part */
// TODO:    *modifiers = 0;
// TODO:    c = modStr;
// TODO:    while (*c != '\0')
// TODO:    {
// TODO:       while (*c == ' ' || *c == '\t')
// TODO:          c++;
// TODO:       if (*c == '\0')
// TODO:          break;
// TODO:       for (i = 0; i < N_MODIFIERS; i++)
// TODO:       {
// TODO:          if (!strncmp(c, modifierNames[i], strlen(modifierNames[i])))
// TODO:          {
// TODO:             c += strlen(modifierNames[i]);
// TODO:             if (modifierMasks[i] != numLockMask)
// TODO:             {
// TODO:                *modifiers |= modifierMasks[i];
// TODO:             }
// TODO:             break;
// TODO:          }
// TODO:       }
// TODO:       if (i == N_MODIFIERS)
// TODO:          return FALSE;
// TODO:    }
// TODO: 
// TODO:    return TRUE;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Event handler for patching around Motif's lock + accelerator problem.
// TODO: ** Looks for a menu item in the patched menu hierarchy and invokes its
// TODO: ** ArmAndActivate action.
// TODO: */
// TODO: static void lockCB(Fl_Widget* w, XtPointer callData, int event,
// TODO:                    bool* continueDispatch)
// TODO: {
// TODO:    Modifiers numLockMask = GetNumLockModMask(XtDisplay(w));
// TODO:    Fl_Widget* topMenuWidget = (Fl_Widget*)callData;
// TODO:    *continueDispatch = TRUE;
// TODO: 
// TODO:    if (!(((XKeyEvent*)event)->state & (LockMask | numLockMask)))
// TODO:       return;
// TODO: 
// TODO:    if (findAndActivateAccel(topMenuWidget, ((XKeyEvent*) event)->keycode,
// TODO:                             ((XKeyEvent*) event)->state & ~(LockMask | numLockMask), event))
// TODO:    {
// TODO:       *continueDispatch = FALSE;
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Search through menu hierarchy under w and look for a button with
// TODO: ** accelerator matching keyCode + modifiers, and do its action
// TODO: */
// TODO: static int findAndActivateAccel(Fl_Widget* w, unsigned int keyCode,
// TODO:                                 unsigned int modifiers, int event)
// TODO: {
// TODO:    WidgetList children;
// TODO:    Fl_Widget* menu;
// TODO:    int numChildren;
// TODO:    int i;
// TODO:    char* accelString = NULL;
// TODO:    KeySym keysym;
// TODO:    unsigned int mods;
// TODO: 
// TODO:    if (XtIsComposite(w))
// TODO:    {
// TODO:       XtVaGetValues(w, XmNchildren, &children, XmNnumChildren,
// TODO:                     &numChildren, NULL);
// TODO:       for (i=0; i<(int)numChildren; i++)
// TODO:          if (findAndActivateAccel(children[i], keyCode, modifiers, event))
// TODO:             return TRUE;
// TODO:    }
// TODO:    else if (XtClass(w) == xmCascadeButtonWidgetClass)
// TODO:    {
// TODO:       XtVaGetValues(w, XmNsubMenuId, &menu, NULL);
// TODO:       if (menu != NULL)
// TODO:          if (findAndActivateAccel(menu, keyCode, modifiers, event))
// TODO:             return TRUE;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       XtVaGetValues(w, XmNaccelerator, &accelString, NULL);
// TODO:       if (accelString != NULL && *accelString != '\0')
// TODO:       {
// TODO:          if (!parseAccelString(XtDisplay(w), accelString, &keysym, &mods))
// TODO:             return FALSE;
// TODO:          if (keyCode == XKeysymToKeycode(XtDisplay(w), keysym) &&
// TODO:                modifiers == mods)
// TODO:          {
// TODO:             if (XtIsSensitive(w))
// TODO:             {
// TODO:                XtCallActionProc(w, "ArmAndActivate", event, NULL, 0);
// TODO:                return TRUE;
// TODO:             }
// TODO:          }
// TODO:       }
// TODO:    }
// TODO:    return FALSE;
// TODO: }

/*
** Global installation of mouse wheel actions for scrolled windows.
*/
void InstallMouseWheelActions(Ne_AppContext& context)
{
   static XtActionsRec Actions[] =
   {
      {"scrolled-window-scroll-up",   scrollUpAP},
      {"scrolled-window-page-up",     pageUpAP},
      {"scrolled-window-scroll-down", scrollDownAP},
      {"scrolled-window-page-down",   pageDownAP}
   };

   NeAppAddActions(context, Actions, ARRAY_SIZE(Actions));
}

// TODO: /*
// TODO: ** Add mouse wheel support to a specific widget, which must be the scrollable
// TODO: ** widget of a ScrolledWindow.
// TODO: */
// TODO: void AddMouseWheelSupport(Fl_Widget* w)
// TODO: {
// TODO:    if (XmIsScrolledWindow(XtParent(w)))
// TODO:    {
// TODO:       static const char scrollTranslations[] =
// TODO:          "Shift<Btn4Down>,<Btn4Up>: scrolled-window-scroll-up(1)\n"
// TODO:          "Shift<Btn5Down>,<Btn5Up>: scrolled-window-scroll-down(1)\n"
// TODO:          "Ctrl<Btn4Down>,<Btn4Up>:  scrolled-window-page-up()\n"
// TODO:          "Ctrl<Btn5Down>,<Btn5Up>:  scrolled-window-page-down()\n"
// TODO:          "<Btn4Down>,<Btn4Up>:      scrolled-window-scroll-up(3)\n"
// TODO:          "<Btn5Down>,<Btn5Up>:      scrolled-window-scroll-down(3)\n";
// TODO:       static XtTranslations trans_table = NULL;
// TODO: 
// TODO:       if (trans_table == NULL)
// TODO:       {
// TODO:          trans_table = XtParseTranslationTable(scrollTranslations);
// TODO:       }
// TODO:       XtOverrideTranslations(w, trans_table);
// TODO:    }
// TODO: }

static void pageUpAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   // TODO:    Fl_Widget* scrolledWindow, scrollBar;
   // TODO:    const char* al[1];
   // TODO: 
   // TODO:    al[0] = "Up";
   // TODO:    scrolledWindow = XtParent(w);
   // TODO:    scrollBar = XtNameToWidget(scrolledWindow, "VertScrollBar");
   // TODO:    if (scrollBar)
   // TODO:       XtCallActionProc(scrollBar, "PageUpOrLeft", event, al, 1) ;
}

static void pageDownAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   // TODO:    Fl_Widget* scrolledWindow, scrollBar;
   // TODO:    const char* al[1];
   // TODO: 
   // TODO:    al[0] = "Down";
   // TODO:    scrolledWindow = XtParent(w);
   // TODO:    scrollBar = XtNameToWidget(scrolledWindow, "VertScrollBar");
   // TODO:    if (scrollBar)
   // TODO:       XtCallActionProc(scrollBar, "PageDownOrRight", event, al, 1) ;
   // TODO:    return;
}

static void scrollUpAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   // TODO:    Fl_Widget* scrolledWindow, scrollBar;
   // TODO:    const char* al[1];
   // TODO:    int i, nLines;
   // TODO: 
   // TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &nLines) != 1)
   // TODO:       return;
   // TODO:    al[0] = "Up";
   // TODO:    scrolledWindow = XtParent(w);
   // TODO:    scrollBar = XtNameToWidget(scrolledWindow, "VertScrollBar");
   // TODO:    if (scrollBar)
   // TODO:       for (i=0; i<nLines; i++)
   // TODO:          XtCallActionProc(scrollBar, "IncrementUpOrLeft", event, al, 1) ;
}

static void scrollDownAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   // TODO:    Fl_Widget* scrolledWindow, scrollBar;
   // TODO:    const char* al[1];
   // TODO:    int i, nLines;
   // TODO: 
   // TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &nLines) != 1)
   // TODO:       return;
   // TODO:    al[0] = "Down";
   // TODO:    scrolledWindow = XtParent(w);
   // TODO:    scrollBar = XtNameToWidget(scrolledWindow, "VertScrollBar");
   // TODO:    if (scrollBar)
   // TODO:       for (i=0; i<nLines; i++)
   // TODO:          XtCallActionProc(scrollBar, "IncrementDownOrRight", event, al, 1) ;
   // TODO:    return;
}


/*
** This is a disguisting hack to work around a bug in OpenMotif.
** OpenMotif's toggle button Select() action routine remembers the last radio
** button that was toggled (stored as global state) and refuses to take any
** action when that button is clicked again. It fails to detect that we may
** have changed the button state and that clicking that button could make
** sense. The result is that radio buttons may apparently get stuck, ie.
** it is not possible to directly select with the mouse the previously
** selected button without selection another radio button first.
** The workaround consist of faking a mouse click on the button that we
** toggled by calling the Arm, Select, and Disarm action procedures.
**
** A minor remaining issue is the fact that, if the workaround is used,
** it is not possible to change the state without notifying potential
** XmNvalueChangedCallbacks. In practice, this doesn't seem to be a problem.
**
*/
void NeRadioButtonChangeState(Fl_Button* widget, bool state, bool notify)
{
   NeToggleButtonSetState(widget, state, notify);
}

/* Workaround for bug in OpenMotif 2.1 and 2.2.  If you have an active tear-off
** menu from a TopLevelShell that is a child of an ApplicationShell, and then
** close the parent window, Motif crashes.  The problem doesn't
** happen if you close the tear-offs first, so, we programatically close them
** before destroying the shell widget.
*/
void CloseAllPopupsFor(Fl_Widget* shell)
{
   // TODO: #ifndef LESSTIF_VERSION
   // TODO:    /* Doesn't happen in LessTif.  The tear-off menus are popup-children of
   // TODO:     * of the TopLevelShell there, which just works.  Motif wants to make
   // TODO:     * them popup-children of the ApplicationShell, where it seems to get
   // TODO:     * into trouble. */
   // TODO: 
   // TODO:    Fl_Widget* app = XtParent(shell);
   // TODO:    int i;
   // TODO: 
   // TODO:    for (i = 0; i < app->core.num_popups; i++)
   // TODO:    {
   // TODO:       Fl_Widget* pop = app->core.popup_list[i];
   // TODO:       Fl_Widget* shellFor;
   // TODO: 
   // TODO:       XtVaGetValues(pop, XtNtransientFor, &shellFor, NULL);
   // TODO:    }
   // TODO: #endif
}

// TODO: static long queryDesktop(Display* display, Window window, Atom deskTopAtom)
// TODO: {
// TODO:    long deskTopNumber = 0;
// TODO:    Atom actualType;
// TODO:    int actualFormat;
// TODO:    unsigned long nItems, bytesAfter;
// TODO:    unsigned char* prop;
// TODO: 
// TODO:    if (XGetWindowProperty(display, window, deskTopAtom, 0, 1,
// TODO:                           false, AnyPropertyType, &actualType, &actualFormat, &nItems,
// TODO:                           &bytesAfter, &prop) != Success)
// TODO:    {
// TODO:       return -1; /* Property not found */
// TODO:    }
// TODO: 
// TODO:    if (actualType == None)
// TODO:    {
// TODO:       return -1; /* Property does not exist */
// TODO:    }
// TODO: 
// TODO:    if (actualFormat != 32 || nItems != 1)
// TODO:    {
// TODO:       XFree((char*)prop);
// TODO:       return -1; /* Wrong format */
// TODO:    }
// TODO: 
// TODO:    deskTopNumber = *(long*)prop;
// TODO:    XFree((char*)prop);
// TODO:    return deskTopNumber;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Returns the current desktop number, or -1 if no desktop information
// TODO: ** is available.
// TODO: */
// TODO: long QueryCurrentDesktop(Display* display, Window rootWindow)
// TODO: {
// TODO:    static Atom currentDesktopAtom = (Atom)-1;
// TODO: 
// TODO:    if (currentDesktopAtom == (Atom)-1)
// TODO:       currentDesktopAtom = XInternAtom(display, "_NET_CURRENT_DESKTOP", true);
// TODO: 
// TODO:    if (currentDesktopAtom != None)
// TODO:       return queryDesktop(display, rootWindow, currentDesktopAtom);
// TODO: 
// TODO:    return -1; /* No desktop information */
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Returns the number of the desktop the given shell window is currently on,
// TODO: ** or -1 if no desktop information is available.  Note that windows shown
// TODO: ** on all desktops (sometimes called sticky windows) should return 0xFFFFFFFF.
// TODO: */
// TODO: long QueryDesktop(Display* display, Fl_Widget* shell)
// TODO: {
// TODO:    static Atom wmDesktopAtom = (Atom)-1;
// TODO: 
// TODO:    if (wmDesktopAtom == (Atom)-1)
// TODO:       wmDesktopAtom = XInternAtom(display, "_NET_WM_DESKTOP", true);
// TODO: 
// TODO:    if (wmDesktopAtom != None)
// TODO:       return queryDesktop(display, XtWindow(shell), wmDesktopAtom);
// TODO: 
// TODO:    return -1;  /* No desktop information */
// TODO: }
// TODO: 
// TODO: 
// TODO: /*
// TODO: ** Clipboard wrapper functions that call the Motif clipboard functions
// TODO: ** a number of times before giving up. The interfaces are similar to the
// TODO: ** native Motif functions.
// TODO: */

#define SPINCOUNT  10   /* Try at most 10 times */
#define USLEEPTIME 1000 /* 1 ms between retries */

/*
** Warning reporting
*/
static void warning(const char* mesg)
{
   fprintf(stderr, "NEdit warning:\n%s\n", mesg);
}

// TODO: /*
// TODO: ** Sleep routine
// TODO: */
// TODO: static void microsleep(long usecs)
// TODO: {
// TODO:    static struct timeval timeoutVal;
// TODO:    timeoutVal.tv_sec = usecs/1000000;
// TODO:    timeoutVal.tv_usec = usecs - timeoutVal.tv_sec*1000000;
// TODO:    select(0, NULL, NULL, NULL, &timeoutVal);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** XmClipboardStartCopy spinlock wrapper.
// TODO: */
// TODO: int SpinClipboardStartCopy(Display* display, Window window,
// TODO:                            NeString clip_label, Time timestamp, Fl_Widget* widget,
// TODO:                            XmCutPasteProc callback, long* item_id)
// TODO: {
// TODO:    int i, res;
// TODO:    for (i=0; i<SPINCOUNT; ++i)
// TODO:    {
// TODO:       res = XmClipboardStartCopy(display, window, clip_label, timestamp,
// TODO:                                  widget, callback, item_id);
// TODO:       if (res == XmClipboardSuccess)
// TODO:       {
// TODO:          return res;
// TODO:       }
// TODO:       microsleep(USLEEPTIME);
// TODO:    }
// TODO:    warning("XmClipboardStartCopy() failed: clipboard locked.");
// TODO:    return res;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** XmClipboardCopy spinlock wrapper.
// TODO: */
// TODO: int SpinClipboardCopy(Display* display, Window window, long item_id,
// TODO:                       char* format_name, XtPointer buffer, unsigned long length,
// TODO:                       long private_id, long* data_id)
// TODO: {
// TODO:    int i, res;
// TODO:    for (i=0; i<SPINCOUNT; ++i)
// TODO:    {
// TODO:       res = XmClipboardCopy(display, window, item_id, format_name,
// TODO:                             buffer, length, private_id, data_id);
// TODO:       if (res == XmClipboardSuccess)
// TODO:       {
// TODO:          return res;
// TODO:       }
// TODO:       if (res == XmClipboardFail)
// TODO:       {
// TODO:          warning("XmClipboardCopy() failed: XmClipboardStartCopy not "
// TODO:                  "called or too many formats.");
// TODO:          return res;
// TODO:       }
// TODO:       microsleep(USLEEPTIME);
// TODO:    }
// TODO:    warning("XmClipboardCopy() failed: clipboard locked.");
// TODO:    return res;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** XmClipboardEndCopy spinlock wrapper.
// TODO: */
// TODO: int SpinClipboardEndCopy(Display* display, Window window, long item_id)
// TODO: {
// TODO:    int i, res;
// TODO:    for (i=0; i<SPINCOUNT; ++i)
// TODO:    {
// TODO:       res = XmClipboardEndCopy(display, window, item_id);
// TODO:       if (res == XmClipboardSuccess)
// TODO:       {
// TODO:          return res;
// TODO:       }
// TODO:       if (res == XmClipboardFail)
// TODO:       {
// TODO:          warning("XmClipboardEndCopy() failed: XmClipboardStartCopy not "
// TODO:                  "called.");
// TODO:          return res;
// TODO:       }
// TODO:       microsleep(USLEEPTIME);
// TODO:    }
// TODO:    warning("XmClipboardEndCopy() failed: clipboard locked.");
// TODO:    return res;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** XmClipboardInquireLength spinlock wrapper.
// TODO: */
// TODO: int SpinClipboardInquireLength(Display* display, Window window,
// TODO:                                char* format_name, unsigned long* length)
// TODO: {
// TODO:    int i, res;
// TODO:    for (i=0; i<SPINCOUNT; ++i)
// TODO:    {
// TODO:       res = XmClipboardInquireLength(display, window, format_name, length);
// TODO:       if (res == XmClipboardSuccess)
// TODO:       {
// TODO:          return res;
// TODO:       }
// TODO:       if (res == XmClipboardNoData)
// TODO:       {
// TODO:          return res;
// TODO:       }
// TODO:       microsleep(USLEEPTIME);
// TODO:    }
// TODO:    warning("XmClipboardInquireLength() failed: clipboard locked.");
// TODO:    return res;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** XmClipboardRetrieve spinlock wrapper.
// TODO: */
// TODO: int SpinClipboardRetrieve(Display* display, Window window, char* format_name,
// TODO:                           XtPointer buffer, unsigned long length, unsigned long* num_bytes,
// TODO:                           long* private_id)
// TODO: {
// TODO:    int i, res;
// TODO:    for (i=0; i<SPINCOUNT; ++i)
// TODO:    {
// TODO:       res = XmClipboardRetrieve(display, window, format_name, buffer,
// TODO:                                 length, num_bytes, private_id);
// TODO:       if (res == XmClipboardSuccess)
// TODO:       {
// TODO:          return res;
// TODO:       }
// TODO:       if (res == XmClipboardTruncate)
// TODO:       {
// TODO:          warning("XmClipboardRetrieve() failed: buffer too small.");
// TODO:          return res;
// TODO:       }
// TODO:       if (res == XmClipboardNoData)
// TODO:       {
// TODO:          return res;
// TODO:       }
// TODO:       microsleep(USLEEPTIME);
// TODO:    }
// TODO:    warning("XmClipboardRetrieve() failed: clipboard locked.");
// TODO:    return res;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** XmClipboardLock spinlock wrapper.
// TODO: */
// TODO: int SpinClipboardLock(Display* display, Window window)
// TODO: {
// TODO:    int i, res;
// TODO:    for (i=0; i<SPINCOUNT; ++i)
// TODO:    {
// TODO:       res = XmClipboardLock(display, window);
// TODO:       if (res == XmClipboardSuccess)
// TODO:       {
// TODO:          return res;
// TODO:       }
// TODO:       microsleep(USLEEPTIME);
// TODO:    }
// TODO:    warning("XmClipboardLock() failed: clipboard locked.");
// TODO:    return res;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** XmClipboardUnlock spinlock wrapper.
// TODO: */
// TODO: int SpinClipboardUnlock(Display* display, Window window)
// TODO: {
// TODO:    int i, res;
// TODO:    /* Spinning doesn't make much sense in this case, I think. */
// TODO:    for (i=0; i<SPINCOUNT; ++i)
// TODO:    {
// TODO:       /* Remove ALL locks (we don't use nested locking in NEdit) */
// TODO:       res = XmClipboardUnlock(display, window, true);
// TODO:       if (res == XmClipboardSuccess)
// TODO:       {
// TODO:          return res;
// TODO:       }
// TODO:       microsleep(USLEEPTIME);
// TODO:    }
// TODO:    /*
// TODO:     * This warning doesn't make much sense in practice. It's usually
// TODO:     * triggered when we try to unlock the clipboard after a failed clipboard
// TODO:     * operation, in an attempt to work around possible *tif clipboard locking
// TODO:     * bugs. In these cases, failure _is_ the expected outcome and the warning
// TODO:     * is bogus. Therefore, the warning is disabled.
// TODO:    warning("XmClipboardUnlock() failed: clipboard not locked or locked "
// TODO:            "by another application.");
// TODO:     */
// TODO:    return res;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Send a client message to a EWMH/NetWM compatible X Window Manager.
// TODO: ** Code taken from wmctrl-1.07 (GPL licensed)
// TODO: */
// TODO: void WmClientMsg(Display* disp, Window win, const char* msg,
// TODO:                  unsigned long data0, unsigned long data1,
// TODO:                  unsigned long data2, unsigned long data3,
// TODO:                  unsigned long data4)
// TODO: {
// TODO:    XEvent event;
// TODO:    long mask = SubstructureRedirectMask | SubstructureNotifyMask;
// TODO: 
// TODO:    event.xclient.type = ClientMessage;
// TODO:    event.xclient.serial = 0;
// TODO:    event.xclient.send_event = true;
// TODO:    event.xclient.message_type = XInternAtom(disp, msg, false);
// TODO:    event.xclient.window = win;
// TODO:    event.xclient.format = 32;
// TODO:    event.xclient.data.l[0] = data0;
// TODO:    event.xclient.data.l[1] = data1;
// TODO:    event.xclient.data.l[2] = data2;
// TODO:    event.xclient.data.l[3] = data3;
// TODO:    event.xclient.data.l[4] = data4;
// TODO: 
// TODO:    if (!XSendEvent(disp, DefaultRootWindow(disp), false, mask, &event))
// TODO:    {
// TODO:       fprintf(stderr, "nedit: cannot send %s EWMH event.\n", msg);
// TODO:    }
// TODO: }

void NeStringFree(NeString str)
{
   delete[] str;
}

char* GetNeStringText(NeString str)
{
   return str;
}

char* NeNewString(const char* str)
{
   if (!str)
      return 0;
   char* tmp = new char[strlen(str) + 1];
   strcpy(tmp, str);
   return tmp;
}

char* NeTextGetString(const Fl_Input* w)
{
   return NeNewString(w->value());
}

void NeTextSetString(Fl_Input* w, const char* str, bool doCallback)
{
   w->value(str);
   if (doCallback)
      w->do_callback();
}


double GetTimeOfDay()
{
#ifdef WIN32
   struct _timeb timebuffer;
   _ftime (&timebuffer);
   return timebuffer.time * 1000.0  + timebuffer.millitm;
#else
  struct timeval t;
  gettimeofday(&t, 0);
  return (t.tv_sec * 1000 + t.tv_usec/1000);
  
#endif
}



