/*******************************************************************************
*									       *
* calltips.c -- Calltip UI functions  (calltip *file* functions are in tags.c) *
*									       *
* Copyright (C) 2002 Nathaniel Gray					       *
*									       *
* This is free__ software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version. In addition, you may distribute version of this program linked to   *
* Motif or Open Motif. See README for details.                                 *
* 									       *
* This software is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
* for more details.							       *
* 									       *
* You should have received a copy of the GNU General Public License along with *
* software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
* Place, Suite 330, Boston, MA  02111-1307 USA		                       *
*									       *
* Nirvana Text Editor	    						       *
* April, 1997								       *
*									       *
* Written by Mark Edel  						       *
*									       *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "Ne_Text_Editor.h"
#include "calltips.h"
#include "../util/misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

static char* expandAllTabs(char* text, int tab_width);

/*
** Pop-down a calltip if one exists, else do nothing
*/
void KillCalltip(WindowInfo* window, int calltipID)
{
// TODO:    textDisp* textD = ((TextWidget)window->lastFocus)->text.textD;
// TODO:    TextDKillCalltip(textD, calltipID);
}

// TODO: void TextDKillCalltip(textDisp* textD, int calltipID)
// TODO: {
// TODO:    if (textD->calltip.ID == 0)
// TODO:       return;
// TODO:    if (calltipID == 0 || calltipID == textD->calltip.ID)
// TODO:    {
// TODO:       XtPopdown(textD->calltipShell);
// TODO:       textD->calltip.ID = 0;
// TODO:    }
// TODO: }

/*
** Is a calltip displayed?  Returns the calltip ID of the currently displayed
** calltip, or 0 if there is no calltip displayed.  If called with
** calltipID != 0, returns 0 unless there is a calltip being
** displayed with that calltipID.
*/
int GetCalltipID(WindowInfo* window, int calltipID)
{
// TODO:    textDisp* textD = ((TextWidget)window->lastFocus)->text.textD;
// TODO:    if (calltipID == 0)
// TODO:       return textD->calltip.ID;
// TODO:    else
// TODO:    {
// TODO:       if (calltipID == textD->calltip.ID)
// TODO:          return calltipID;
// TODO:       else
// TODO:          return 0;
// TODO:    }
   return 0;
}

#define CALLTIP_EDGE_GUARD 5
// TODO: static bool offscreenV(XWindowAttributes* screenAttr, int top, int height)
// TODO: {
// TODO:    return (top < CALLTIP_EDGE_GUARD ||
// TODO:            top + height >= screenAttr->height - CALLTIP_EDGE_GUARD);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Update the position of the current calltip if one exists, else do nothing
// TODO: */
// TODO: void TextDRedrawCalltip(textDisp* textD, int calltipID)
// TODO: {
// TODO:    int lineHeight = textD->ascent + textD->descent;
// TODO:    Position txtX, txtY, borderWidth, abs_x, abs_y, tipWidth, tipHeight;
// TODO:    XWindowAttributes screenAttr;
// TODO:    int rel_x, rel_y, flip_delta;
// TODO: 
// TODO:    if (textD->calltip.ID == 0)
// TODO:       return;
// TODO:    if (calltipID != 0 && calltipID != textD->calltip.ID)
// TODO:       return;
// TODO: 
// TODO:    /* Get the location/dimensions of the text area */
// TODO:    XtVaGetValues(textD->w, XmNx, &txtX, XmNy, &txtY, NULL);
// TODO: 
// TODO:    if (textD->calltip.anchored)
// TODO:    {
// TODO:       /* Put it at the anchor position */
// TODO:       if (!TextDPositionToXY(textD, textD->calltip.pos, &rel_x, &rel_y))
// TODO:       {
// TODO:          if (textD->calltip.alignMode == TIP_STRICT)
// TODO:             TextDKillCalltip(textD, textD->calltip.ID);
// TODO:          return;
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if (textD->calltip.pos < 0)
// TODO:       {
// TODO:          /* First display of tip with cursor offscreen (detected in
// TODO:              ShowCalltip) */
// TODO:          textD->calltip.pos = textD->width/2;
// TODO:          textD->calltip.hAlign = TIP_CENTER;
// TODO:          rel_y = textD->height/3;
// TODO:       }
// TODO:       else if (!TextDPositionToXY(textD, textD->cursorPos, &rel_x, &rel_y))
// TODO:       {
// TODO:          /* Window has scrolled and tip is now offscreen */
// TODO:          if (textD->calltip.alignMode == TIP_STRICT)
// TODO:             TextDKillCalltip(textD, textD->calltip.ID);
// TODO:          return;
// TODO:       }
// TODO:       rel_x = textD->calltip.pos;
// TODO:    }
// TODO: 
// TODO:    XtVaGetValues(textD->calltipShell, XmNwidth, &tipWidth, XmNheight,
// TODO:                  &tipHeight, XmNborderWidth, &borderWidth, NULL);
// TODO:    rel_x += borderWidth;
// TODO:    rel_y += lineHeight/2 + borderWidth;
// TODO: 
// TODO:    /* Adjust rel_x for horizontal alignment modes */
// TODO:    if (textD->calltip.hAlign == TIP_CENTER)
// TODO:       rel_x -= tipWidth/2;
// TODO:    else if (textD->calltip.hAlign == TIP_RIGHT)
// TODO:       rel_x -= tipWidth;
// TODO: 
// TODO:    /* Adjust rel_y for vertical alignment modes */
// TODO:    if (textD->calltip.vAlign == TIP_ABOVE)
// TODO:    {
// TODO:       flip_delta = tipHeight + lineHeight + 2*borderWidth;
// TODO:       rel_y -= flip_delta;
// TODO:    }
// TODO:    else
// TODO:       flip_delta = -(tipHeight + lineHeight + 2*borderWidth);
// TODO: 
// TODO:    XtTranslateCoords(textD->w, rel_x, rel_y, &abs_x, &abs_y);
// TODO: 
// TODO:    /* If we're not in strict mode try to keep the tip on-screen */
// TODO:    if (textD->calltip.alignMode == TIP_SLOPPY)
// TODO:    {
// TODO:       XGetWindowAttributes(XtDisplay(textD->w),
// TODO:                            RootWindowOfScreen(XtScreen(textD->w)), &screenAttr);
// TODO: 
// TODO:       /* make sure tip doesn't run off right or left side of screen */
// TODO:       if (abs_x + tipWidth >= screenAttr.width - CALLTIP_EDGE_GUARD)
// TODO:          abs_x = screenAttr.width - tipWidth - CALLTIP_EDGE_GUARD;
// TODO:       if (abs_x < CALLTIP_EDGE_GUARD)
// TODO:          abs_x = CALLTIP_EDGE_GUARD;
// TODO: 
// TODO:       /* Try to keep the tip onscreen vertically if possible */
// TODO:       if (screenAttr.height > tipHeight &&
// TODO:             offscreenV(&screenAttr, abs_y, tipHeight))
// TODO:       {
// TODO:          /* Maybe flipping from below to above (or vice-versa) will help */
// TODO:          if (!offscreenV(&screenAttr, abs_y + flip_delta, tipHeight))
// TODO:             abs_y += flip_delta;
// TODO:          /* Make sure the tip doesn't end up *totally* offscreen */
// TODO:          else if (abs_y + tipHeight < 0)
// TODO:             abs_y = CALLTIP_EDGE_GUARD;
// TODO:          else if (abs_y >= screenAttr.height)
// TODO:             abs_y = screenAttr.height - tipHeight - CALLTIP_EDGE_GUARD;
// TODO:          /* If no case applied, just go with the default placement. */
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    XtVaSetValues(textD->calltipShell, XmNx, abs_x, XmNy, abs_y, NULL);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Returns a new string with each \t replaced with tab_width spaces or
// TODO: ** a pointer to text if there were no tabs.  Returns NULL on new failure.
// TODO: ** Note that this is dumb replacement, not smart tab-like behavior!  The goal
// TODO: ** is to prevent tabs from turning into squares in calltips, not to get the
// TODO: ** formatting just right.
// TODO: */
// TODO: static char* expandAllTabs(char* text, int tab_width)
// TODO: {
// TODO:    int i, nTabs=0;
// TODO:    size_t len;
// TODO:    char* c, *cCpy, *textCpy;
// TODO: 
// TODO:    /* First count 'em */
// TODO:    for (c = text; *c; ++c)
// TODO:       if (*c == '\t')
// TODO:          ++nTabs;
// TODO:    if (nTabs == 0)
// TODO:       return text;
// TODO: 
// TODO:    /* Allocate the new string */
// TODO:    len = strlen(text) + (tab_width - 1)*nTabs;
// TODO:    textCpy = (char*)malloc__(len + 1);
// TODO:    if (!textCpy)
// TODO:    {
// TODO:       fprintf(stderr,
// TODO:               "nedit: Out of heap memory in expandAllTabs!\n");
// TODO:       return NULL;
// TODO:    }
// TODO: 
// TODO:    /* Now replace 'em */
// TODO:    for (c = text, cCpy = textCpy;  *c;  ++c, ++cCpy)
// TODO:    {
// TODO:       if (*c == '\t')
// TODO:       {
// TODO:          for (i = 0; i < tab_width; ++i, ++cCpy)
// TODO:             *cCpy = ' ';
// TODO:          --cCpy;  /* Will be incremented in outer for loop */
// TODO:       }
// TODO:       else
// TODO:          *cCpy = *c;
// TODO:    }
// TODO:    *cCpy = '\0';
// TODO:    return textCpy;
// TODO: }

/*
** Pop-up a calltip.
** If a calltip is already being displayed it is destroyed and replaced with
** the new calltip.  Returns the ID of the calltip or 0 on failure.
*/
int ShowCalltip(WindowInfo* window, char* text, bool anchored, int pos, int hAlign, int vAlign, int alignMode)
{
// TODO:    static int StaticCalltipID = 1;
// TODO:    textDisp* textD = ((TextWidget)window->lastFocus)->text.textD;
// TODO:    int rel_x, rel_y;
// TODO:    Position txtX, txtY;
// TODO:    char* textCpy;
// TODO:    NeString str;
// TODO: 
// TODO:    /* Destroy any previous calltip */
// TODO:    TextDKillCalltip(textD, 0);
// TODO: 
// TODO:    /* Make sure the text isn't NULL */
// TODO:    if (text == NULL) return 0;
// TODO: 
// TODO:    /* Expand any tabs in the calltip and make it an NeString */
// TODO:    textCpy = expandAllTabs(text, BufGetTabDistance(textD->buffer));
// TODO:    if (textCpy == NULL)
// TODO:       return 0;       /* Out of memory */
// TODO:    str = XmStringCreateLtoR(textCpy, XmFONTLIST_DEFAULT_TAG);
// TODO:    if (textCpy != text)
// TODO:       free__(textCpy);
// TODO: 
// TODO:    /* Get the location/dimensions of the text area */
// TODO:    XtVaGetValues(textD->w,
// TODO:                  XmNx, &txtX,
// TODO:                  XmNy, &txtY,
// TODO:                  NULL);
// TODO: 
// TODO:    /* Create the calltip widget on first request */
// TODO:    if (textD->calltipW == NULL)
// TODO:    {
// TODO:       Arg args[10];
// TODO:       int argcnt = 0;
// TODO:       XtSetArg(args[argcnt], XmNsaveUnder, true);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNallowShellResize, true);
// TODO:       argcnt++;
// TODO: 
// TODO:       textD->calltipShell = CreatePopupShellWithBestVis("calltipshell",
// TODO:                             overrideShellWidgetClass, textD->w, args, argcnt);
// TODO: 
// TODO:       /* Might want to make this a read-only XmText eventually so that
// TODO:           users can copy from it */
// TODO:       textD->calltipW = XtVaCreateManagedWidget(
// TODO:                            "calltip", xmLabelWidgetClass, textD->calltipShell,
// TODO:                            XmNborderWidth, 1,              /* Thin borders */
// TODO:                            XmNhighlightThickness, 0,
// TODO:                            XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                            XmNforeground, textD->calltipFGPixel,
// TODO:                            XmNbackground, textD->calltipBGPixel,
// TODO:                            NULL);
// TODO:    }
// TODO: 
// TODO:    /* Set the text on the label */
// TODO:    XtVaSetValues(textD->calltipW, XmNlabelString, str, NULL);
// TODO:    NeStringFree(str);
// TODO: 
// TODO:    /* Figure out where to put the tip */
// TODO:    if (anchored)
// TODO:    {
// TODO:       /* Put it at the specified position */
// TODO:       /* If position is not displayed, return 0 */
// TODO:       if (pos < textD->firstChar || pos > textD->lastChar)
// TODO:       {
// TODO:          XBell(TheDisplay, 0);
// TODO:          return 0;
// TODO:       }
// TODO:       textD->calltip.pos = pos;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       /* Put it next to the cursor, or in the center of the window if the
// TODO:           cursor is offscreen and mode != strict */
// TODO:       if (!TextDPositionToXY(textD, textD->cursorPos, &rel_x, &rel_y))
// TODO:       {
// TODO:          if (alignMode == TIP_STRICT)
// TODO:          {
// TODO:             XBell(TheDisplay, 0);
// TODO:             return 0;
// TODO:          }
// TODO:          textD->calltip.pos = -1;
// TODO:       }
// TODO:       else
// TODO:          /* Store the x-offset for use when redrawing */
// TODO:          textD->calltip.pos = rel_x;
// TODO:    }
// TODO: 
// TODO:    /* Should really bounds-check these enumerations... */
// TODO:    textD->calltip.ID = StaticCalltipID;
// TODO:    textD->calltip.anchored = anchored;
// TODO:    textD->calltip.hAlign = hAlign;
// TODO:    textD->calltip.vAlign = vAlign;
// TODO:    textD->calltip.alignMode = alignMode;
// TODO: 
// TODO:    /* Increment the static calltip ID.  Macro variables can only be int,
// TODO:        not unsigned, so have to work to keep it > 0 on overflow */
// TODO:    if (++StaticCalltipID <= 0)
// TODO:       StaticCalltipID = 1;
// TODO: 
// TODO:    /* Realize the calltip's shell so that its width & height are known */
// TODO:    XtRealizeWidget(textD->calltipShell);
// TODO:    /* Move the calltip and pop it up */
// TODO:    TextDRedrawCalltip(textD, 0);
// TODO:    XtPopup(textD->calltipShell, XtGrabNone);
// TODO:    return textD->calltip.ID;
   return 0;
}
