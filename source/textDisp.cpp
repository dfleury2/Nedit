// TODO: static const char CVSID[] = "$Id: textDisp.c,v 1.71 2008/01/04 22:31:48 yooden Exp $";
// TODO: /*******************************************************************************
// TODO: *									       *
// TODO: * textDisp.c - Display text from a text buffer				       *
// TODO: *									       *
// TODO: * Copyright (C) 1999 Mark Edel						       *
// TODO: *									       *
// TODO: * This is free__ software; you can redistribute it and/or modify it under the    *
// TODO: * terms of the GNU General Public License as published by the Free Software    *
// TODO: * Foundation; either version 2 of the License, or (at your option) any later   *
// TODO: * version. In addition, you may distribute version of this program linked to   *
// TODO: * Motif or Open Motif. See README for details.                                 *
// TODO: * 									       *
// TODO: * This software is distributed in the hope that it will be useful, but WITHOUT *
// TODO: * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
// TODO: * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
// TODO: * for more details.							       *
// TODO: * 									       *
// TODO: * You should have received a copy of the GNU General Public License along with *
// TODO: * software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
// TODO: * Place, Suite 330, Boston, MA  02111-1307 USA		                       *
// TODO: *									       *
// TODO: * Nirvana Text Editor	    						       *
// TODO: * June 15, 1995								       *
// TODO: *									       *
// TODO: * Written by Mark Edel							       *
// TODO: *									       *
// TODO: *******************************************************************************/
// TODO: 
// TODO: #ifdef HAVE_CONFIG_H
// TODO: #include "../config.h"
// TODO: #endif
// TODO: 
// TODO: #include "Ne_Text_Display.h"
// TODO: #include "Ne_Text_Buffer.h"
// TODO: #include "Ne_Text_Editor.h"
// TODO: #include "Ne_Text_Part.h"
// TODO: #include "nedit.h"
// TODO: #include "calltips.h"
// TODO: #include "highlight.h"
// TODO: #include "Ne_Rangeset.h"
// TODO: 
// TODO: #include <stdio.h>
// TODO: #include <stdlib.h>
// TODO: #include <string.h>
// TODO: #include <limits.h>
// TODO: #ifdef VMS
// TODO: #include "../util/VMSparam.h"
// TODO: #else
// TODO: #ifndef WIN32
// TODO: #include <sys/param.h>
// TODO: #endif
// TODO: #endif /*VMS*/
// TODO: 
// TODO: #include <Xm/Xm.h>
// TODO: #include <Xm/ScrolledW.h>
// TODO: #include <Xm/ScrollBar.h>
// TODO: #include <Xm/Label.h>
// TODO: #include <X11/Shell.h>
// TODO: 
// TODO: #ifdef HAVE_DEBUG_H
// TODO: #include "../debug.h"
// TODO: #endif
// TODO: 
// TODO: /* Masks for text drawing methods.  These are or'd together to form an
// TODO:    integer which describes what drawing calls to use to draw a string */
// TODO: #define FILL_SHIFT 8
// TODO: #define SECONDARY_SHIFT 9
// TODO: #define PRIMARY_SHIFT 10
// TODO: #define HIGHLIGHT_SHIFT 11
// TODO: #define STYLE_LOOKUP_SHIFT 0
// TODO: #define BACKLIGHT_SHIFT 12
// TODO: 
// TODO: #define FILL_MASK (1 << FILL_SHIFT)
// TODO: #define SECONDARY_MASK (1 << SECONDARY_SHIFT)
// TODO: #define PRIMARY_MASK (1 << PRIMARY_SHIFT)
// TODO: #define HIGHLIGHT_MASK (1 << HIGHLIGHT_SHIFT)
// TODO: #define STYLE_LOOKUP_MASK (0xff << STYLE_LOOKUP_SHIFT)
// TODO: #define BACKLIGHT_MASK  (0xff << BACKLIGHT_SHIFT)
// TODO: 
// TODO: #define RANGESET_SHIFT (20)
// TODO: #define RANGESET_MASK (0x3F << RANGESET_SHIFT)
// TODO: 
// TODO: /* If you use both 32-Bit Style mask layout:
// TODO:    Bits +----------------+----------------+----------------+----------------+
// TODO:     hex |1F1E1D1C1B1A1918|1716151413121110| F E D C B A 9 8| 7 6 5 4 3 2 1 0|
// TODO:     dec |3130292827262524|2322212019181716|151413121110 9 8| 7 6 5 4 3 2 1 0|
// TODO:         +----------------+----------------+----------------+----------------+
// TODO:    Type |             r r| r r r r b b b b| b b b b H 1 2 F| s s s s s s s s|
// TODO:         +----------------+----------------+----------------+----------------+
// TODO:    where: s - style lookup value (8 bits)
// TODO:         F - fill (1 bit)
// TODO:         2 - secondary selection  (1 bit)
// TODO:         1 - primary selection (1 bit)
// TODO:         H - highlight (1 bit)
// TODO:         b - backlighting index (8 bits)
// TODO:         r - rangeset index (6 bits)
// TODO:    This leaves 6 "unused" bits */
// TODO: 
// TODO: /* Maximum displayable line length (how many characters will fit across the
// TODO:    widest window).  This amount of memory is temporarily allocated from the
// TODO:    stack in the redisplayLine routine for drawing strings */
// TODO: #define MAX_DISP_LINE_LEN 1000
// TODO: 
// TODO: /* Macro for getting the TextPart from a textD */
// TODO: #define TEXT_OF_TEXTD(t)    (((TextWidget)((t)->w))->text)
// TODO: 
// TODO: enum positionTypes {CURSOR_POS, CHARACTER_POS};
// TODO: 
// TODO: static void updateLineStarts(textDisp* textD, int pos, int charsInserted,
// TODO:                              int charsDeleted, int linesInserted, int linesDeleted, int* scrolled);
// TODO: static void offsetLineStarts(textDisp* textD, int newTopLineNum);
// TODO: static void calcLineStarts(textDisp* textD, int startLine, int endLine);
// TODO: static void calcLastChar(textDisp* textD);
// TODO: static int posToVisibleLineNum(textDisp* textD, int pos, int* lineNum);
// TODO: static void redisplayLine(textDisp* textD, int visLineNum, int leftClip,
// TODO:                           int rightClip, int leftCharIndex, int rightCharIndex);
// TODO: static void drawString(textDisp* textD, int style, int x, int y, int toX,
// TODO:                        char* string, int nChars);
// TODO: static void clearRect(textDisp* textD, GC gc, int x, int y,
// TODO:                       int width, int height);
// TODO: static void drawCursor(textDisp* textD, int x, int y);
// TODO: static int styleOfPos(textDisp* textD, int lineStartPos,
// TODO:                       int lineLen, int lineIndex, int dispIndex, int thisChar);
// TODO: static int stringWidth(const textDisp* textD, const char* string,
// TODO:                        const int length, const int style);
// TODO: static int inSelection(selection* sel, int pos, int lineStartPos,
// TODO:                        int dispIndex);
// TODO: static int xyToPos(textDisp* textD, int x, int y, int posType);
// TODO: static void xyToUnconstrainedPos(textDisp* textD, int x, int y, int* row,
// TODO:                                  int* column, int posType);
// TODO: static void bufPreDeleteCB(int pos, int nDeleted, void* cbArg);
// TODO: static void bufModifiedCB(int pos, int nInserted, int nDeleted,
// TODO:                           int nRestyled, const char* deletedText, void* cbArg);
// TODO: static void setScroll(textDisp* textD, int topLineNum, int horizOffset,
// TODO:                       int updateVScrollBar, int updateHScrollBar);
// TODO: static void hScrollCB(Widget w, XtPointer clientData, XtPointer callData);
// TODO: static void vScrollCB(Widget w, XtPointer clientData, XtPointer callData);
// TODO: static void visibilityEH(Widget w, XtPointer data, XEvent* event,
// TODO:                          bool* continueDispatch);
// TODO: static void redrawLineNumbers(textDisp* textD, int clearAll);
// TODO: static void updateVScrollBarRange(textDisp* textD);
// TODO: static int updateHScrollBarRange(textDisp* textD);
// TODO: static int max(int i1, int i2);
// TODO: static int min(int i1, int i2);
// TODO: static int countLines(const char* string);
// TODO: static int measureVisLine(textDisp* textD, int visLineNum);
// TODO: static int emptyLinesVisible(textDisp* textD);
// TODO: static void blankCursorProtrusions(textDisp* textD);
// TODO: static void allocateFixedFontGCs(textDisp* textD, XFontStruct* fontStruct,
// TODO:                                  Pixel bgPixel, Pixel fgPixel, Pixel selectFGPixel, Pixel selectBGPixel,
// TODO:                                  Pixel highlightFGPixel, Pixel highlightBGPixel, Pixel lineNumFGPixel);
// TODO: static GC allocateGC(Widget w, unsigned long valueMask,
// TODO:                      unsigned long foreground, unsigned long background, Font font,
// TODO:                      unsigned long dynamicMask, unsigned long dontCareMask);
// TODO: static void releaseGC(Widget w, GC gc);
// TODO: static void resetClipRectangles(textDisp* textD);
// TODO: static int visLineLength(textDisp* textD, int visLineNum);
// TODO: static void measureDeletedLines(textDisp* textD, int pos, int nDeleted);
// TODO: static void findWrapRange(textDisp* textD, const char* deletedText, int pos,
// TODO:                           int nInserted, int nDeleted, int* modRangeStart, int* modRangeEnd,
// TODO:                           int* linesInserted, int* linesDeleted);
// TODO: static void wrappedLineCounter(const textDisp* textD, const textBuffer* buf,
// TODO:                                const int startPos, const int maxPos, const int maxLines,
// TODO:                                const bool startPosIsLineStart, const int styleBufOffset,
// TODO:                                int* retPos, int* retLines, int* retLineStart, int* retLineEnd);
// TODO: static void findLineEnd(textDisp* textD, int startPos, int startPosIsLineStart,
// TODO:                         int* lineEnd, int* nextLineStart);
// TODO: static int wrapUsesCharacter(textDisp* textD, int lineEndPos);
// TODO: static void hideOrShowHScrollBar(textDisp* textD);
// TODO: static int rangeTouchesRectSel(selection* sel, int rangeStart, int rangeEnd);
// TODO: static void extendRangeForStyleMods(textDisp* textD, int* start, int* end);
// TODO: static int getAbsTopLineNum(textDisp* textD);
// TODO: static void offsetAbsLineNum(textDisp* textD, int oldFirstChar);
// TODO: static int maintainingAbsTopLineNum(textDisp* textD);
// TODO: static void resetAbsLineNum(textDisp* textD);
// TODO: static int measurePropChar(const textDisp* textD, const char c,
// TODO:                            const int colNum, const int pos);
// TODO: static Pixel allocBGColor(Widget w, char* colorName, int* ok);
// TODO: static Pixel getRangesetColor(textDisp* textD, int ind, Pixel bground);
// TODO: static void textDRedisplayRange(textDisp* textD, int start, int end);
// TODO: 
// TODO: textDisp* TextDCreate(Widget widget, Widget hScrollBar, Widget vScrollBar,
// TODO:                       Position left, Position top, Position width, Position height,
// TODO:                       Position lineNumLeft, Position lineNumWidth, textBuffer* buffer,
// TODO:                       XFontStruct* fontStruct, Pixel bgPixel, Pixel fgPixel,
// TODO:                       Pixel selectFGPixel, Pixel selectBGPixel, Pixel highlightFGPixel,
// TODO:                       Pixel highlightBGPixel, Pixel cursorFGPixel, Pixel lineNumFGPixel,
// TODO:                       int continuousWrap, int wrapMargin, NeString bgClassString,
// TODO:                       Pixel calltipFGPixel, Pixel calltipBGPixel)
// TODO: {
// TODO:    textDisp* textD;
// TODO:    XGCValues gcValues;
// TODO:    int i;
// TODO: 
// TODO:    textD = (textDisp*)malloc__(sizeof(textDisp));
// TODO:    textD->w = widget;
// TODO:    textD->top = top;
// TODO:    textD->left = left;
// TODO:    textD->width = width;
// TODO:    textD->height = height;
// TODO:    textD->cursorOn = true;
// TODO:    textD->cursorPos = 0;
// TODO:    textD->cursorX = -100;
// TODO:    textD->cursorY = -100;
// TODO:    textD->cursorToHint = NO_HINT;
// TODO:    textD->cursorStyle = NORMAL_CURSOR;
// TODO:    textD->cursorPreferredCol = -1;
// TODO:    textD->buffer = buffer;
// TODO:    textD->firstChar = 0;
// TODO:    textD->lastChar = 0;
// TODO:    textD->nBufferLines = 0;
// TODO:    textD->topLineNum = 1;
// TODO:    textD->absTopLineNum = 1;
// TODO:    textD->needAbsTopLineNum = false;
// TODO:    textD->horizOffset = 0;
// TODO:    textD->visibility = VisibilityUnobscured;
// TODO:    textD->hScrollBar = hScrollBar;
// TODO:    textD->vScrollBar = vScrollBar;
// TODO:    textD->fontStruct = fontStruct;
// TODO:    textD->ascent = fontStruct->ascent;
// TODO:    textD->descent = fontStruct->descent;
// TODO:    textD->fixedFontWidth = fontStruct->min_bounds.width ==
// TODO:                            fontStruct->max_bounds.width ? fontStruct->min_bounds.width : -1;
// TODO:    textD->styleBuffer = NULL;
// TODO:    textD->styleTable = NULL;
// TODO:    textD->nStyles = 0;
// TODO:    textD->bgPixel = bgPixel;
// TODO:    textD->fgPixel = fgPixel;
// TODO:    textD->selectFGPixel = selectFGPixel;
// TODO:    textD->highlightFGPixel = highlightFGPixel;
// TODO:    textD->selectBGPixel = selectBGPixel;
// TODO:    textD->highlightBGPixel = highlightBGPixel;
// TODO:    textD->lineNumFGPixel = lineNumFGPixel;
// TODO:    textD->cursorFGPixel = cursorFGPixel;
// TODO:    textD->wrapMargin = wrapMargin;
// TODO:    textD->continuousWrap = continuousWrap;
// TODO:    allocateFixedFontGCs(textD, fontStruct, bgPixel, fgPixel, selectFGPixel,
// TODO:                         selectBGPixel, highlightFGPixel, highlightBGPixel, lineNumFGPixel);
// TODO:    textD->styleGC = allocateGC(textD->w, 0, 0, 0, fontStruct->fid,
// TODO:                                GCClipMask|GCForeground|GCBackground, GCArcMode);
// TODO:    textD->lineNumLeft = lineNumLeft;
// TODO:    textD->lineNumWidth = lineNumWidth;
// TODO:    textD->nVisibleLines = (height - 1) / (textD->ascent + textD->descent) + 1;
// TODO:    gcValues.foreground = cursorFGPixel;
// TODO:    textD->cursorFGGC = XtGetGC(widget, GCForeground, &gcValues);
// TODO:    textD->lineStarts = (int*)malloc__(sizeof(int) * textD->nVisibleLines);
// TODO:    textD->lineStarts[0] = 0;
// TODO:    textD->calltipW = NULL;
// TODO:    textD->calltipShell = NULL;
// TODO:    textD->calltip.ID = 0;
// TODO:    textD->calltipFGPixel = calltipFGPixel;
// TODO:    textD->calltipBGPixel = calltipBGPixel;
// TODO:    for (i=1; i<textD->nVisibleLines; i++)
// TODO:       textD->lineStarts[i] = -1;
// TODO:    textD->bgClassPixel = NULL;
// TODO:    textD->bgClass = NULL;
// TODO:    TextDSetupBGClasses(widget, bgClassString, &textD->bgClassPixel,
// TODO:                        &textD->bgClass, bgPixel);
// TODO:    textD->suppressResync = 0;
// TODO:    textD->nLinesDeleted = 0;
// TODO:    textD->modifyingTabDist = 0;
// TODO:    textD->pointerHidden = false;
// TODO:    textD->graphicsExposeQueue = NULL;
// TODO: 
// TODO:    /* Attach an event handler to the widget so we can know the visibility
// TODO:       (used for choosing the fastest drawing method) */
// TODO:    XtAddEventHandler(widget, VisibilityChangeMask, false,
// TODO:                      visibilityEH, textD);
// TODO: 
// TODO:    /* Attach the callback to the text buffer for receiving modification
// TODO:       information */
// TODO:    if (buffer != NULL)
// TODO:    {
// TODO:       BufAddModifyCB(buffer, bufModifiedCB, textD);
// TODO:       BufAddPreDeleteCB(buffer, bufPreDeleteCB, textD);
// TODO:    }
// TODO: 
// TODO:    /* Initialize the scroll bars and attach movement callbacks */
// TODO:    if (vScrollBar != NULL)
// TODO:    {
// TODO:       XtVaSetValues(vScrollBar, XmNminimum, 1, XmNmaximum, 2,
// TODO:                     XmNsliderSize, 1, XmNrepeatDelay, 10, XmNvalue, 1, NULL);
// TODO:       XtAddCallback(vScrollBar, XmNdragCallback, vScrollCB, (XtPointer)textD);
// TODO:       XtAddCallback(vScrollBar, XmNvalueChangedCallback, vScrollCB,
// TODO:                     (XtPointer)textD);
// TODO:    }
// TODO:    if (hScrollBar != NULL)
// TODO:    {
// TODO:       XtVaSetValues(hScrollBar, XmNminimum, 0, XmNmaximum, 1,
// TODO:                     XmNsliderSize, 1, XmNrepeatDelay, 10, XmNvalue, 0,
// TODO:                     XmNincrement, fontStruct->max_bounds.width, NULL);
// TODO:       XtAddCallback(hScrollBar, XmNdragCallback, hScrollCB, (XtPointer)textD);
// TODO:       XtAddCallback(hScrollBar, XmNvalueChangedCallback, hScrollCB,
// TODO:                     (XtPointer)textD);
// TODO:    }
// TODO: 
// TODO:    /* Update the display to reflect the contents of the buffer */
// TODO:    if (buffer != NULL)
// TODO:       bufModifiedCB(0, buffer->length, 0, 0, NULL, textD);
// TODO: 
// TODO:    /* Decide if the horizontal scroll bar needs to be visible */
// TODO:    hideOrShowHScrollBar(textD);
// TODO: 
// TODO:    return textD;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Free a text display and release its associated memory.  Note, the text
// TODO: ** BUFFER that the text display displays is a separate entity and is not
// TODO: ** freed, nor are the style buffer or style table.
// TODO: */
// TODO: void TextDFree(textDisp* textD)
// TODO: {
// TODO:    BufRemoveModifyCB(textD->buffer, bufModifiedCB, textD);
// TODO:    BufRemovePreDeleteCB(textD->buffer, bufPreDeleteCB, textD);
// TODO:    releaseGC(textD->w, textD->gc);
// TODO:    releaseGC(textD->w, textD->selectGC);
// TODO:    releaseGC(textD->w, textD->highlightGC);
// TODO:    releaseGC(textD->w, textD->selectBGGC);
// TODO:    releaseGC(textD->w, textD->highlightBGGC);
// TODO:    releaseGC(textD->w, textD->styleGC);
// TODO:    releaseGC(textD->w, textD->lineNumGC);
// TODO:    XtFree((char*)textD->lineStarts);
// TODO:    while (TextDPopGraphicExposeQueueEntry(textD))
// TODO:    {
// TODO:    }
// TODO:    XtFree((char*)textD->bgClassPixel);
// TODO:    XtFree((char*)textD->bgClass);
// TODO:    XtFree((char*)textD);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Attach a text buffer to display, replacing the current buffer (if any)
// TODO: */
// TODO: void TextDSetBuffer(textDisp* textD, textBuffer* buffer)
// TODO: {
// TODO:    /* If the text display is already displaying a buffer, clear it off
// TODO:       of the display and remove our callback from it */
// TODO:    if (textD->buffer != NULL)
// TODO:    {
// TODO:       bufModifiedCB(0, 0, textD->buffer->length, 0, NULL, textD);
// TODO:       BufRemoveModifyCB(textD->buffer, bufModifiedCB, textD);
// TODO:       BufRemovePreDeleteCB(textD->buffer, bufPreDeleteCB, textD);
// TODO:    }
// TODO: 
// TODO:    /* Add the buffer to the display, and attach a callback to the buffer for
// TODO:       receiving modification information when the buffer contents change */
// TODO:    textD->buffer = buffer;
// TODO:    BufAddModifyCB(buffer, bufModifiedCB, textD);
// TODO:    BufAddPreDeleteCB(buffer, bufPreDeleteCB, textD);
// TODO: 
// TODO:    /* Update the display */
// TODO:    bufModifiedCB(0, buffer->length, 0, 0, NULL, textD);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Attach (or remove) highlight information in text display and redisplay.
// TODO: ** Highlighting information consists of a style buffer which parallels the
// TODO: ** normal text buffer, but codes font and color information for the display;
// TODO: ** a style table which translates style buffer codes (indexed by buffer
// TODO: ** character - 65 (ASCII code for 'A')) into fonts and colors; and a callback
// TODO: ** mechanism for as-needed highlighting, triggered by a style buffer entry of
// TODO: ** "unfinishedStyle".  Style buffer can trigger additional redisplay during
// TODO: ** a normal buffer modification if the buffer contains a primary selection
// TODO: ** (see extendRangeForStyleMods for more information on this protocol).
// TODO: **
// TODO: ** Style buffers, tables and their associated memory are managed by the caller.
// TODO: */
// TODO: void TextDAttachHighlightData(textDisp* textD, textBuffer* styleBuffer,
// TODO:                               styleTableEntry* styleTable, int nStyles, char unfinishedStyle,
// TODO:                               unfinishedStyleCBProc unfinishedHighlightCB, void* cbArg)
// TODO: {
// TODO:    textD->styleBuffer = styleBuffer;
// TODO:    textD->styleTable = styleTable;
// TODO:    textD->nStyles = nStyles;
// TODO:    textD->unfinishedStyle = unfinishedStyle;
// TODO:    textD->unfinishedHighlightCB = unfinishedHighlightCB;
// TODO:    textD->highlightCBArg = cbArg;
// TODO: 
// TODO:    /* Call TextDSetFont to combine font information from style table and
// TODO:       primary font, adjust font-related parameters, and then redisplay */
// TODO:    TextDSetFont(textD, textD->fontStruct);
// TODO: }
// TODO: 
// TODO: 
// TODO: /* Change the (non syntax-highlit) colors */
// TODO: void TextDSetColors(textDisp* textD, Pixel textFgP, Pixel textBgP,
// TODO:                     Pixel selectFgP, Pixel selectBgP, Pixel hiliteFgP, Pixel hiliteBgP,
// TODO:                     Pixel lineNoFgP, Pixel cursorFgP)
// TODO: {
// TODO:    XGCValues values;
// TODO:    Display* d = XtDisplay(textD->w);
// TODO: 
// TODO:    /* Update the stored pixels */
// TODO:    textD->fgPixel = textFgP;
// TODO:    textD->bgPixel = textBgP;
// TODO:    textD->selectFGPixel = selectFgP;
// TODO:    textD->selectBGPixel = selectBgP;
// TODO:    textD->highlightFGPixel = hiliteFgP;
// TODO:    textD->highlightBGPixel = hiliteBgP;
// TODO:    textD->lineNumFGPixel = lineNoFgP;
// TODO:    textD->cursorFGPixel = cursorFgP;
// TODO: 
// TODO:    releaseGC(textD->w, textD->gc);
// TODO:    releaseGC(textD->w, textD->selectGC);
// TODO:    releaseGC(textD->w, textD->selectBGGC);
// TODO:    releaseGC(textD->w, textD->highlightGC);
// TODO:    releaseGC(textD->w, textD->highlightBGGC);
// TODO:    releaseGC(textD->w, textD->lineNumGC);
// TODO:    allocateFixedFontGCs(textD, textD->fontStruct, textBgP, textFgP, selectFgP,
// TODO:                         selectBgP, hiliteFgP, hiliteBgP, lineNoFgP);
// TODO: 
// TODO:    /* Change the cursor GC (the cursor GC is not shared). */
// TODO:    values.foreground = cursorFgP;
// TODO:    XChangeGC(d, textD->cursorFGGC, GCForeground, &values);
// TODO: 
// TODO:    /* Redisplay */
// TODO:    TextDRedisplayRect(textD, textD->left, textD->top, textD->width,
// TODO:                       textD->height);
// TODO:    redrawLineNumbers(textD, true);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Change the (non highlight) font
// TODO: */
// TODO: void TextDSetFont(textDisp* textD, XFontStruct* fontStruct)
// TODO: {
// TODO:    Display* display = XtDisplay(textD->w);
// TODO:    int i, maxAscent = fontStruct->ascent, maxDescent = fontStruct->descent;
// TODO:    int width, height, fontWidth;
// TODO:    Pixel bgPixel, fgPixel, selectFGPixel, selectBGPixel;
// TODO:    Pixel highlightFGPixel, highlightBGPixel, lineNumFGPixel;
// TODO:    XGCValues values;
// TODO:    XFontStruct* styleFont;
// TODO: 
// TODO:    /* If font size changes, cursor will be redrawn in a new position */
// TODO:    blankCursorProtrusions(textD);
// TODO: 
// TODO:    /* If there is a (syntax highlighting) style table in use, find the new
// TODO:       maximum font height for this text display */
// TODO:    for (i=0; i<textD->nStyles; i++)
// TODO:    {
// TODO:       styleFont = textD->styleTable[i].font;
// TODO:       if (styleFont != NULL && styleFont->ascent > maxAscent)
// TODO:          maxAscent = styleFont->ascent;
// TODO:       if (styleFont != NULL && styleFont->descent > maxDescent)
// TODO:          maxDescent = styleFont->descent;
// TODO:    }
// TODO:    textD->ascent = maxAscent;
// TODO:    textD->descent = maxDescent;
// TODO: 
// TODO:    /* If all of the current fonts are fixed and match in width, compute */
// TODO:    fontWidth = fontStruct->max_bounds.width;
// TODO:    if (fontWidth != fontStruct->min_bounds.width)
// TODO:       fontWidth = -1;
// TODO:    else
// TODO:    {
// TODO:       for (i=0; i<textD->nStyles; i++)
// TODO:       {
// TODO:          styleFont = textD->styleTable[i].font;
// TODO:          if (styleFont != NULL &&
// TODO:                (styleFont->max_bounds.width != fontWidth ||
// TODO:                 styleFont->max_bounds.width != styleFont->min_bounds.width))
// TODO:             fontWidth = -1;
// TODO:       }
// TODO:    }
// TODO:    textD->fixedFontWidth = fontWidth;
// TODO: 
// TODO:    /* Don't let the height dip below one line, or bad things can happen */
// TODO:    if (textD->height < maxAscent + maxDescent)
// TODO:       textD->height = maxAscent + maxDescent;
// TODO: 
// TODO:    /* Change the font.  In most cases, this means re-allocating the
// TODO:       affected GCs (they are shared with other widgets, and if the primary
// TODO:       font changes, must be re-allocated to change it). Unfortunately,
// TODO:       this requres recovering all of the colors from the existing GCs */
// TODO:    textD->fontStruct = fontStruct;
// TODO:    XGetGCValues(display, textD->gc, GCForeground|GCBackground, &values);
// TODO:    fgPixel = values.foreground;
// TODO:    bgPixel = values.background;
// TODO:    XGetGCValues(display, textD->selectGC, GCForeground|GCBackground, &values);
// TODO:    selectFGPixel = values.foreground;
// TODO:    selectBGPixel = values.background;
// TODO:    XGetGCValues(display, textD->highlightGC,GCForeground|GCBackground,&values);
// TODO:    highlightFGPixel = values.foreground;
// TODO:    highlightBGPixel = values.background;
// TODO:    XGetGCValues(display, textD->lineNumGC, GCForeground, &values);
// TODO:    lineNumFGPixel = values.foreground;
// TODO:    releaseGC(textD->w, textD->gc);
// TODO:    releaseGC(textD->w, textD->selectGC);
// TODO:    releaseGC(textD->w, textD->highlightGC);
// TODO:    releaseGC(textD->w, textD->selectBGGC);
// TODO:    releaseGC(textD->w, textD->highlightBGGC);
// TODO:    releaseGC(textD->w, textD->lineNumGC);
// TODO:    allocateFixedFontGCs(textD, fontStruct, bgPixel, fgPixel, selectFGPixel,
// TODO:                         selectBGPixel, highlightFGPixel, highlightBGPixel, lineNumFGPixel);
// TODO:    XSetFont(display, textD->styleGC, fontStruct->fid);
// TODO: 
// TODO:    /* Do a full resize to force recalculation of font related parameters */
// TODO:    width = textD->width;
// TODO:    height = textD->height;
// TODO:    textD->width = textD->height = 0;
// TODO:    TextDResize(textD, width, height);
// TODO: 
// TODO:    /* if the shell window doesn't get resized, and the new fonts are
// TODO:       of smaller sizes, sometime we get some residual text on the
// TODO:       blank space at the bottom part of text area. Clear it here. */
// TODO:    clearRect(textD, textD->gc, textD->left,
// TODO:              textD->top + textD->height - maxAscent - maxDescent,
// TODO:              textD->width, maxAscent + maxDescent);
// TODO: 
// TODO:    /* Redisplay */
// TODO:    TextDRedisplayRect(textD, textD->left, textD->top, textD->width,
// TODO:                       textD->height);
// TODO: 
// TODO:    /* Clean up line number area in case spacing has changed */
// TODO:    redrawLineNumbers(textD, true);
// TODO: }
// TODO: 
// TODO: int TextDMinFontWidth(textDisp* textD, bool considerStyles)
// TODO: {
// TODO:    int fontWidth = textD->fontStruct->max_bounds.width;
// TODO:    int i;
// TODO: 
// TODO:    if (considerStyles)
// TODO:    {
// TODO:       for (i = 0; i < textD->nStyles; ++i)
// TODO:       {
// TODO:          int thisWidth = (textD->styleTable[i].font)->min_bounds.width;
// TODO:          if (thisWidth < fontWidth)
// TODO:          {
// TODO:             fontWidth = thisWidth;
// TODO:          }
// TODO:       }
// TODO:    }
// TODO:    return(fontWidth);
// TODO: }
// TODO: 
// TODO: int TextDMaxFontWidth(textDisp* textD, bool considerStyles)
// TODO: {
// TODO:    int fontWidth = textD->fontStruct->max_bounds.width;
// TODO:    int i;
// TODO: 
// TODO:    if (considerStyles)
// TODO:    {
// TODO:       for (i = 0; i < textD->nStyles; ++i)
// TODO:       {
// TODO:          int thisWidth = (textD->styleTable[i].font)->max_bounds.width;
// TODO:          if (thisWidth > fontWidth)
// TODO:          {
// TODO:             fontWidth = thisWidth;
// TODO:          }
// TODO:       }
// TODO:    }
// TODO:    return(fontWidth);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Change the size of the displayed text area
// TODO: */
// TODO: void TextDResize(textDisp* textD, int width, int height)
// TODO: {
// TODO:    int oldVisibleLines = textD->nVisibleLines;
// TODO:    int canRedraw = XtWindow(textD->w) != 0;
// TODO:    int newVisibleLines = height / (textD->ascent + textD->descent);
// TODO:    int redrawAll = false;
// TODO:    int oldWidth = textD->width;
// TODO:    int exactHeight = height - height % (textD->ascent + textD->descent);
// TODO: 
// TODO:    textD->width = width;
// TODO:    textD->height = height;
// TODO: 
// TODO:    /* In continuous wrap mode, a change in width affects the total number of
// TODO:       lines in the buffer, and can leave the top line number incorrect, and
// TODO:       the top character no longer pointing at a valid line start */
// TODO:    if (textD->continuousWrap && textD->wrapMargin==0 && width!=oldWidth)
// TODO:    {
// TODO:       int oldFirstChar = textD->firstChar;
// TODO:       textD->nBufferLines = TextDCountLines(textD, 0, textD->buffer->length,
// TODO:                                             true);
// TODO:       textD->firstChar = TextDStartOfLine(textD, textD->firstChar);
// TODO:       textD->topLineNum = TextDCountLines(textD, 0, textD->firstChar, true)+1;
// TODO:       redrawAll = true;
// TODO:       offsetAbsLineNum(textD, oldFirstChar);
// TODO:    }
// TODO: 
// TODO:    /* reallocate and update the line starts array, which may have changed
// TODO:       size and/or contents. (contents can change in continuous wrap mode
// TODO:       when the width changes, even without a change in height) */
// TODO:    if (oldVisibleLines < newVisibleLines)
// TODO:    {
// TODO:       XtFree((char*)textD->lineStarts);
// TODO:       textD->lineStarts = (int*)malloc__(sizeof(int) * newVisibleLines);
// TODO:    }
// TODO:    textD->nVisibleLines = newVisibleLines;
// TODO:    calcLineStarts(textD, 0, newVisibleLines);
// TODO:    calcLastChar(textD);
// TODO: 
// TODO:    /* if the window became shorter, there may be partially drawn
// TODO:       text left at the bottom edge, which must be cleaned up */
// TODO:    if (canRedraw && oldVisibleLines>newVisibleLines && exactHeight!=height)
// TODO:       XClearArea(XtDisplay(textD->w), XtWindow(textD->w), textD->left,
// TODO:                  textD->top + exactHeight,  textD->width,
// TODO:                  height - exactHeight, false);
// TODO: 
// TODO:    /* if the window became taller, there may be an opportunity to display
// TODO:       more text by scrolling down */
// TODO:    if (canRedraw && oldVisibleLines < newVisibleLines && textD->topLineNum +
// TODO:          textD->nVisibleLines > textD->nBufferLines)
// TODO:       setScroll(textD, max(1, textD->nBufferLines - textD->nVisibleLines +
// TODO:                            2 + TEXT_OF_TEXTD(textD).cursorVPadding),
// TODO:                 textD->horizOffset, false, false);
// TODO: 
// TODO:    /* Update the scroll bar page increment size (as well as other scroll
// TODO:       bar parameters.  If updating the horizontal range caused scrolling,
// TODO:       redraw */
// TODO:    updateVScrollBarRange(textD);
// TODO:    if (updateHScrollBarRange(textD))
// TODO:       redrawAll = true;
// TODO: 
// TODO:    /* If a full redraw is needed */
// TODO:    if (redrawAll && canRedraw)
// TODO:       TextDRedisplayRect(textD, textD->left, textD->top, textD->width,
// TODO:                          textD->height);
// TODO: 
// TODO:    /* Decide if the horizontal scroll bar needs to be visible */
// TODO:    hideOrShowHScrollBar(textD);
// TODO: 
// TODO:    /* Refresh the line number display to draw more line numbers, or
// TODO:       erase extras */
// TODO:    redrawLineNumbers(textD, true);
// TODO: 
// TODO:    /* Redraw the calltip */
// TODO:    TextDRedrawCalltip(textD, 0);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Refresh a rectangle of the text display.  left and top are in coordinates of
// TODO: ** the text drawing window
// TODO: */
// TODO: void TextDRedisplayRect(textDisp* textD, int left, int top, int width,
// TODO:                         int height)
// TODO: {
// TODO:    int fontHeight, firstLine, lastLine, line;
// TODO: 
// TODO:    /* find the line number range of the display */
// TODO:    fontHeight = textD->ascent + textD->descent;
// TODO:    firstLine = (top - textD->top - fontHeight + 1) / fontHeight;
// TODO:    lastLine = (top + height - textD->top) / fontHeight;
// TODO: 
// TODO:    /* If the graphics contexts are shared using XtAllocateGC, their
// TODO:       clipping rectangles may have changed since the last use */
// TODO:    resetClipRectangles(textD);
// TODO: 
// TODO:    /* draw the lines of text */
// TODO:    for (line=firstLine; line<=lastLine; line++)
// TODO:       redisplayLine(textD, line, left, left+width, 0, INT_MAX);
// TODO: 
// TODO:    /* draw the line numbers if exposed area includes them */
// TODO:    if (textD->lineNumWidth != 0 && left <= textD->lineNumLeft + textD->lineNumWidth)
// TODO:       redrawLineNumbers(textD, false);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Refresh all of the text between buffer positions "start" and "end"
// TODO: ** not including the character at the position "end".
// TODO: ** If end points beyond the end of the buffer, refresh the whole display
// TODO: ** after pos, including blank lines which are not technically part of
// TODO: ** any range of characters.
// TODO: */
// TODO: static void textDRedisplayRange(textDisp* textD, int start, int end)
// TODO: {
// TODO:    int i, startLine, lastLine, startIndex, endIndex;
// TODO: 
// TODO:    /* If the range is outside of the displayed text, just return */
// TODO:    if (end < textD->firstChar || (start > textD->lastChar &&
// TODO:                                   !emptyLinesVisible(textD)))
// TODO:       return;
// TODO: 
// TODO:    /* Clean up the starting and ending values */
// TODO:    if (start < 0) start = 0;
// TODO:    if (start > textD->buffer->length) start = textD->buffer->length;
// TODO:    if (end < 0) end = 0;
// TODO:    if (end > textD->buffer->length) end = textD->buffer->length;
// TODO: 
// TODO:    /* Get the starting and ending lines */
// TODO:    if (start < textD->firstChar)
// TODO:    {
// TODO:       start = textD->firstChar;
// TODO:    }
// TODO: 
// TODO:    if (!posToVisibleLineNum(textD, start, &startLine))
// TODO:    {
// TODO:       startLine = textD->nVisibleLines - 1;
// TODO:    }
// TODO: 
// TODO:    if (end >= textD->lastChar)
// TODO:    {
// TODO:       lastLine = textD->nVisibleLines - 1;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if (!posToVisibleLineNum(textD, end, &lastLine))
// TODO:       {
// TODO:          /* shouldn't happen */
// TODO:          lastLine = textD->nVisibleLines - 1;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* Get the starting and ending positions within the lines */
// TODO:    startIndex = (textD->lineStarts[startLine] == -1)
// TODO:                 ? 0
// TODO:                 : start - textD->lineStarts[startLine];
// TODO:    if (end >= textD->lastChar)
// TODO:    {
// TODO:       /*  Request to redisplay beyond textD->lastChar, so tell
// TODO:           redisplayLine() to display everything to infy.  */
// TODO:       endIndex = INT_MAX;
// TODO:    }
// TODO:    else if (textD->lineStarts[lastLine] == -1)
// TODO:    {
// TODO:       /*  Here, lastLine is determined by posToVisibleLineNum() (see
// TODO:           if/else above) but deemed to be out of display according to
// TODO:           textD->lineStarts. */
// TODO:       endIndex = 0;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       endIndex = end - textD->lineStarts[lastLine];
// TODO:    }
// TODO: 
// TODO:    /* Reset the clipping rectangles for the drawing GCs which are shared
// TODO:       using XtAllocateGC, and may have changed since the last use */
// TODO:    resetClipRectangles(textD);
// TODO: 
// TODO:    /* If the starting and ending lines are the same, redisplay the single
// TODO:       line between "start" and "end" */
// TODO:    if (startLine == lastLine)
// TODO:    {
// TODO:       redisplayLine(textD, startLine, 0, INT_MAX, startIndex, endIndex);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Redisplay the first line from "start" */
// TODO:    redisplayLine(textD, startLine, 0, INT_MAX, startIndex, INT_MAX);
// TODO: 
// TODO:    /* Redisplay the lines in between at their full width */
// TODO:    for (i=startLine+1; i<lastLine; i++)
// TODO:       redisplayLine(textD, i, 0, INT_MAX, 0, INT_MAX);
// TODO: 
// TODO:    /* Redisplay the last line to "end" */
// TODO:    redisplayLine(textD, lastLine, 0, INT_MAX, 0, endIndex);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Set the scroll position of the text display vertically by line number and
// TODO: ** horizontally by pixel offset from the left margin
// TODO: */
// TODO: void TextDSetScroll(textDisp* textD, int topLineNum, int horizOffset)
// TODO: {
// TODO:    int sliderSize, sliderMax;
// TODO:    int vPadding = (int)(TEXT_OF_TEXTD(textD).cursorVPadding);
// TODO: 
// TODO:    /* Limit the requested scroll position to allowable values */
// TODO:    if (topLineNum < 1)
// TODO:       topLineNum = 1;
// TODO:    else if ((topLineNum > textD->topLineNum) &&
// TODO:             (topLineNum > (textD->nBufferLines + 2 - textD->nVisibleLines +
// TODO:                            vPadding)))
// TODO:       topLineNum = max(textD->topLineNum,
// TODO:                        textD->nBufferLines + 2 - textD->nVisibleLines + vPadding);
// TODO:    XtVaGetValues(textD->hScrollBar, XmNmaximum, &sliderMax,
// TODO:                  XmNsliderSize, &sliderSize, NULL);
// TODO:    if (horizOffset < 0)
// TODO:       horizOffset = 0;
// TODO:    if (horizOffset > sliderMax - sliderSize)
// TODO:       horizOffset = sliderMax - sliderSize;
// TODO: 
// TODO:    setScroll(textD, topLineNum, horizOffset, true, true);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Get the current scroll position for the text display, in terms of line
// TODO: ** number of the top line and horizontal pixel offset from the left margin
// TODO: */
// TODO: void TextDGetScroll(textDisp* textD, int* topLineNum, int* horizOffset)
// TODO: {
// TODO:    *topLineNum = textD->topLineNum;
// TODO:    *horizOffset = textD->horizOffset;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Set the position of the text insertion cursor for text display "textD"
// TODO: */
// TODO: void TextDSetInsertPosition(textDisp* textD, int newPos)
// TODO: {
// TODO:    /* make sure new position is ok, do nothing if it hasn't changed */
// TODO:    if (newPos == textD->cursorPos)
// TODO:       return;
// TODO:    if (newPos < 0) newPos = 0;
// TODO:    if (newPos > textD->buffer->length) newPos = textD->buffer->length;
// TODO: 
// TODO:    /* cursor movement cancels vertical cursor motion column */
// TODO:    textD->cursorPreferredCol = -1;
// TODO: 
// TODO:    /* erase the cursor at it's previous position */
// TODO:    TextDBlankCursor(textD);
// TODO: 
// TODO:    /* draw it at its new position */
// TODO:    textD->cursorPos = newPos;
// TODO:    textD->cursorOn = true;
// TODO:    textDRedisplayRange(textD, textD->cursorPos-1, textD->cursorPos + 1);
// TODO: }
// TODO: 
// TODO: void TextDBlankCursor(textDisp* textD)
// TODO: {
// TODO:    if (!textD->cursorOn)
// TODO:       return;
// TODO: 
// TODO:    blankCursorProtrusions(textD);
// TODO:    textD->cursorOn = false;
// TODO:    textDRedisplayRange(textD, textD->cursorPos-1, textD->cursorPos+1);
// TODO: }
// TODO: 
// TODO: void TextDUnblankCursor(textDisp* textD)
// TODO: {
// TODO:    if (!textD->cursorOn)
// TODO:    {
// TODO:       textD->cursorOn = true;
// TODO:       textDRedisplayRange(textD, textD->cursorPos-1, textD->cursorPos+1);
// TODO:    }
// TODO: }
// TODO: 
// TODO: void TextDSetCursorStyle(textDisp* textD, int style)
// TODO: {
// TODO:    textD->cursorStyle = style;
// TODO:    blankCursorProtrusions(textD);
// TODO:    if (textD->cursorOn)
// TODO:    {
// TODO:       textDRedisplayRange(textD, textD->cursorPos-1, textD->cursorPos + 1);
// TODO:    }
// TODO: }
// TODO: 
// TODO: void TextDSetWrapMode(textDisp* textD, int wrap, int wrapMargin)
// TODO: {
// TODO:    textD->wrapMargin = wrapMargin;
// TODO:    textD->continuousWrap = wrap;
// TODO: 
// TODO:    /* wrapping can change change the total number of lines, re-count */
// TODO:    textD->nBufferLines = TextDCountLines(textD, 0, textD->buffer->length,true);
// TODO: 
// TODO:    /* changing wrap margins wrap or changing from wrapped mode to non-wrapped
// TODO:       can leave the character at the top no longer at a line start, and/or
// TODO:       change the line number */
// TODO:    textD->firstChar = TextDStartOfLine(textD, textD->firstChar);
// TODO:    textD->topLineNum = TextDCountLines(textD, 0, textD->firstChar, true) + 1;
// TODO:    resetAbsLineNum(textD);
// TODO: 
// TODO:    /* update the line starts array */
// TODO:    calcLineStarts(textD, 0, textD->nVisibleLines);
// TODO:    calcLastChar(textD);
// TODO: 
// TODO:    /* Update the scroll bar page increment size (as well as other scroll
// TODO:       bar parameters) */
// TODO:    updateVScrollBarRange(textD);
// TODO:    updateHScrollBarRange(textD);
// TODO: 
// TODO:    /* Decide if the horizontal scroll bar needs to be visible */
// TODO:    hideOrShowHScrollBar(textD);
// TODO: 
// TODO:    /* Do a full redraw */
// TODO:    TextDRedisplayRect(textD, 0, textD->top, textD->width + textD->left,
// TODO:                       textD->height);
// TODO: }
// TODO: 
// TODO: int TextDGetInsertPosition(textDisp* textD)
// TODO: {
// TODO:    return textD->cursorPos;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Insert "text" at the current cursor location.  This has the same
// TODO: ** effect as inserting the text into the buffer using BufInsert and
// TODO: ** then moving the insert position after the newly inserted text, except
// TODO: ** that it's optimized to do less redrawing.
// TODO: */
// TODO: void TextDInsert(textDisp* textD, char* text)
// TODO: {
// TODO:    int pos = textD->cursorPos;
// TODO: 
// TODO:    textD->cursorToHint = pos + strlen(text);
// TODO:    BufInsert(textD->buffer, pos, text);
// TODO:    textD->cursorToHint = NO_HINT;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Insert "text" (which must not contain newlines), overstriking the current
// TODO: ** cursor location.
// TODO: */
// TODO: void TextDOverstrike(textDisp* textD, char* text)
// TODO: {
// TODO:    int startPos = textD->cursorPos;
// TODO:    textBuffer* buf = textD->buffer;
// TODO:    int lineStart = BufStartOfLine(buf, startPos);
// TODO:    int textLen = strlen(text);
// TODO:    int i, p, endPos, indent, startIndent, endIndent;
// TODO:    char* c, ch, *paddedText = NULL;
// TODO: 
// TODO:    /* determine how many displayed character positions are covered */
// TODO:    startIndent = BufCountDispChars(textD->buffer, lineStart, startPos);
// TODO:    indent = startIndent;
// TODO:    for (c=text; *c!='\0'; c++)
// TODO:       indent += BufCharWidth(*c, indent, buf->tabDist, buf->nullSubsChar);
// TODO:    endIndent = indent;
// TODO: 
// TODO:    /* find which characters to remove, and if necessary generate additional
// TODO:       padding to make up for removed control characters at the end */
// TODO:    indent=startIndent;
// TODO:    for (p=startPos; ; p++)
// TODO:    {
// TODO:       if (p == buf->length)
// TODO:          break;
// TODO:       ch = BufGetCharacter(buf, p);
// TODO:       if (ch == '\n')
// TODO:          break;
// TODO:       indent += BufCharWidth(ch, indent, buf->tabDist, buf->nullSubsChar);
// TODO:       if (indent == endIndent)
// TODO:       {
// TODO:          p++;
// TODO:          break;
// TODO:       }
// TODO:       else if (indent > endIndent)
// TODO:       {
// TODO:          if (ch != '\t')
// TODO:          {
// TODO:             p++;
// TODO:             paddedText = malloc__(textLen + MAX_EXP_CHAR_LEN + 1);
// TODO:             strcpy(paddedText, text);
// TODO:             for (i=0; i<indent-endIndent; i++)
// TODO:                paddedText[textLen+i] = ' ';
// TODO:             paddedText[textLen+i] = '\0';
// TODO:          }
// TODO:          break;
// TODO:       }
// TODO:    }
// TODO:    endPos = p;
// TODO: 
// TODO:    textD->cursorToHint = startPos + textLen;
// TODO:    BufReplace(buf, startPos, endPos, paddedText == NULL ? text : paddedText);
// TODO:    textD->cursorToHint = NO_HINT;
// TODO:    XtFree(paddedText);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Translate window coordinates to the nearest text cursor position.
// TODO: */
// TODO: int TextDXYToPosition(textDisp* textD, int x, int y)
// TODO: {
// TODO:    return xyToPos(textD, x, y, CURSOR_POS);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Translate window coordinates to the nearest character cell.
// TODO: */
// TODO: int TextDXYToCharPos(textDisp* textD, int x, int y)
// TODO: {
// TODO:    return xyToPos(textD, x, y, CHARACTER_POS);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Translate window coordinates to the nearest row and column number for
// TODO: ** positioning the cursor.  This, of course, makes no sense when the font
// TODO: ** is proportional, since there are no absolute columns.
// TODO: */
// TODO: void TextDXYToUnconstrainedPosition(textDisp* textD, int x, int y, int* row,
// TODO:                                     int* column)
// TODO: {
// TODO:    xyToUnconstrainedPos(textD, x, y, row, column, CURSOR_POS);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Translate line and column to the nearest row and column number for
// TODO: ** positioning the cursor.  This, of course, makes no sense when the font
// TODO: ** is proportional, since there are no absolute columns.
// TODO: */
// TODO: int TextDLineAndColToPos(textDisp* textD, int lineNum, int column)
// TODO: {
// TODO:    int i, lineEnd, charIndex, outIndex;
// TODO:    int lineStart=0, charLen=0;
// TODO:    char* lineStr, expandedChar[MAX_EXP_CHAR_LEN];
// TODO: 
// TODO:    /* Count lines */
// TODO:    if (lineNum < 1)
// TODO:       lineNum = 1;
// TODO:    lineEnd = -1;
// TODO:    for (i=1; i<=lineNum && lineEnd<textD->buffer->length; i++)
// TODO:    {
// TODO:       lineStart = lineEnd + 1;
// TODO:       lineEnd = BufEndOfLine(textD->buffer, lineStart);
// TODO:    }
// TODO: 
// TODO:    /* If line is beyond end of buffer, position at last character in buffer */
// TODO:    if (lineNum >= i)
// TODO:    {
// TODO:       return lineEnd;
// TODO:    }
// TODO: 
// TODO:    /* Start character index at zero */
// TODO:    charIndex=0;
// TODO: 
// TODO:    /* Only have to count columns if column isn't zero (or negative) */
// TODO:    if (column > 0)
// TODO:    {
// TODO:       /* Count columns, expanding each character */
// TODO:       lineStr = BufGetRange(textD->buffer, lineStart, lineEnd);
// TODO:       outIndex = 0;
// TODO:       for (i=lineStart; i<lineEnd; i++, charIndex++)
// TODO:       {
// TODO:          charLen = BufExpandCharacter(lineStr[charIndex], outIndex,
// TODO:                                       expandedChar, textD->buffer->tabDist,
// TODO:                                       textD->buffer->nullSubsChar);
// TODO:          if (outIndex+charLen >= column) break;
// TODO:          outIndex+=charLen;
// TODO:       }
// TODO: 
// TODO:       /* If the column is in the middle of an expanded character, put cursor
// TODO:        * in front of character if in first half of character, and behind
// TODO:        * character if in last half of character
// TODO:        */
// TODO:       if (column >= outIndex + (charLen / 2))
// TODO:          charIndex++;
// TODO: 
// TODO:       /* If we are beyond the end of the line, back up one space */
// TODO:       if ((i>=lineEnd)&&(charIndex>0)) charIndex--;
// TODO:    }
// TODO: 
// TODO:    /* Position is the start of the line plus the index into line buffer */
// TODO:    return lineStart + charIndex;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Translate a buffer text position to the XY location where the center
// TODO: ** of the cursor would be positioned to point to that character.  Returns
// TODO: ** false if the position is not displayed because it is VERTICALLY out
// TODO: ** of view.  If the position is horizontally out of view, returns the
// TODO: ** x coordinate where the position would be if it were visible.
// TODO: */
// TODO: int TextDPositionToXY(textDisp* textD, int pos, int* x, int* y)
// TODO: {
// TODO:    int charIndex, lineStartPos, fontHeight, lineLen;
// TODO:    int visLineNum, charLen, outIndex, xStep, charStyle;
// TODO:    char* lineStr, expandedChar[MAX_EXP_CHAR_LEN];
// TODO: 
// TODO:    /* If position is not displayed, return false */
// TODO:    if (pos < textD->firstChar ||
// TODO:          (pos > textD->lastChar && !emptyLinesVisible(textD)))
// TODO:       return false;
// TODO: 
// TODO:    /* Calculate y coordinate */
// TODO:    if (!posToVisibleLineNum(textD, pos, &visLineNum))
// TODO:       return false;
// TODO:    fontHeight = textD->ascent + textD->descent;
// TODO:    *y = textD->top + visLineNum*fontHeight + fontHeight/2;
// TODO: 
// TODO:    /* Get the text, length, and  buffer position of the line. If the position
// TODO:       is beyond the end of the buffer and should be at the first position on
// TODO:       the first empty line, don't try to get or scan the text  */
// TODO:    lineStartPos = textD->lineStarts[visLineNum];
// TODO:    if (lineStartPos == -1)
// TODO:    {
// TODO:       *x = textD->left - textD->horizOffset;
// TODO:       return true;
// TODO:    }
// TODO:    lineLen = visLineLength(textD, visLineNum);
// TODO:    lineStr = BufGetRange(textD->buffer, lineStartPos, lineStartPos + lineLen);
// TODO: 
// TODO:    /* Step through character positions from the beginning of the line
// TODO:       to "pos" to calculate the x coordinate */
// TODO:    xStep = textD->left - textD->horizOffset;
// TODO:    outIndex = 0;
// TODO:    for (charIndex=0; charIndex<pos-lineStartPos; charIndex++)
// TODO:    {
// TODO:       charLen = BufExpandCharacter(lineStr[charIndex], outIndex, expandedChar,
// TODO:                                    textD->buffer->tabDist, textD->buffer->nullSubsChar);
// TODO:       charStyle = styleOfPos(textD, lineStartPos, lineLen, charIndex,
// TODO:                              outIndex, lineStr[charIndex]);
// TODO:       xStep += stringWidth(textD, expandedChar, charLen, charStyle);
// TODO:       outIndex += charLen;
// TODO:    }
// TODO:    *x = xStep;
// TODO:    XtFree(lineStr);
// TODO:    return true;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** If the text widget is maintaining a line number count appropriate to "pos"
// TODO: ** return the line and column numbers of pos, otherwise return false.  If
// TODO: ** continuous wrap mode is on, returns the absolute line number (as opposed to
// TODO: ** the wrapped line number which is used for scrolling).  THIS ROUTINE ONLY
// TODO: ** WORKS FOR DISPLAYED LINES AND, IN CONTINUOUS WRAP MODE, ONLY WHEN THE
// TODO: ** ABSOLUTE LINE NUMBER IS BEING MAINTAINED.  Otherwise, it returns false.
// TODO: */
// TODO: int TextDPosToLineAndCol(textDisp* textD, int pos, int* lineNum, int* column)
// TODO: {
// TODO:    textBuffer* buf = textD->buffer;
// TODO: 
// TODO:    /* In continuous wrap mode, the absolute (non-wrapped) line count is
// TODO:       maintained separately, as needed.  Only return it if we're actually
// TODO:       keeping track of it and pos is in the displayed text */
// TODO:    if (textD->continuousWrap)
// TODO:    {
// TODO:       if (!maintainingAbsTopLineNum(textD) || pos < textD->firstChar ||
// TODO:             pos > textD->lastChar)
// TODO:          return false;
// TODO:       *lineNum = textD->absTopLineNum + BufCountLines(buf,
// TODO:                  textD->firstChar, pos);
// TODO:       *column = BufCountDispChars(buf, BufStartOfLine(buf, pos), pos);
// TODO:       return true;
// TODO:    }
// TODO: 
// TODO:    /* Only return the data if pos is within the displayed text */
// TODO:    if (!posToVisibleLineNum(textD, pos, lineNum))
// TODO:       return false;
// TODO:    *column = BufCountDispChars(buf, textD->lineStarts[*lineNum], pos);
// TODO:    *lineNum += textD->topLineNum;
// TODO:    return true;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Return true if position (x, y) is inside of the primary selection
// TODO: */
// TODO: int TextDInSelection(textDisp* textD, int x, int y)
// TODO: {
// TODO:    int row, column, pos = xyToPos(textD, x, y, CHARACTER_POS);
// TODO:    textBuffer* buf = textD->buffer;
// TODO: 
// TODO:    xyToUnconstrainedPos(textD, x, y, &row, &column, CHARACTER_POS);
// TODO:    if (rangeTouchesRectSel(&buf->primary, textD->firstChar, textD->lastChar))
// TODO:       column = TextDOffsetWrappedColumn(textD, row, column);
// TODO:    return inSelection(&buf->primary, pos, BufStartOfLine(buf, pos), column);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Correct a column number based on an unconstrained position (as returned by
// TODO: ** TextDXYToUnconstrainedPosition) to be relative to the last actual newline
// TODO: ** in the buffer before the row and column position given, rather than the
// TODO: ** last line start created by line wrapping.  This is an adapter
// TODO: ** for rectangular selections and code written before continuous wrap mode,
// TODO: ** which thinks that the unconstrained column is the number of characters
// TODO: ** from the last newline.  Obviously this is time consuming, because it
// TODO: ** invloves character re-counting.
// TODO: */
// TODO: int TextDOffsetWrappedColumn(textDisp* textD, int row, int column)
// TODO: {
// TODO:    int lineStart, dispLineStart;
// TODO: 
// TODO:    if (!textD->continuousWrap || row < 0 || row > textD->nVisibleLines)
// TODO:       return column;
// TODO:    dispLineStart = textD->lineStarts[row];
// TODO:    if (dispLineStart == -1)
// TODO:       return column;
// TODO:    lineStart = BufStartOfLine(textD->buffer, dispLineStart);
// TODO:    return column + BufCountDispChars(textD->buffer, lineStart, dispLineStart);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Correct a row number from an unconstrained position (as returned by
// TODO: ** TextDXYToUnconstrainedPosition) to a straight number of newlines from the
// TODO: ** top line of the display.  Because rectangular selections are based on
// TODO: ** newlines, rather than display wrapping, and anywhere a rectangular selection
// TODO: ** needs a row, it needs it in terms of un-wrapped lines.
// TODO: */
// TODO: int TextDOffsetWrappedRow(textDisp* textD, int row)
// TODO: {
// TODO:    if (!textD->continuousWrap || row < 0 || row > textD->nVisibleLines)
// TODO:       return row;
// TODO:    return BufCountLines(textD->buffer, textD->firstChar,
// TODO:                         textD->lineStarts[row]);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Scroll the display to bring insertion cursor into view.
// TODO: **
// TODO: ** Note: it would be nice to be able to do this without counting lines twice
// TODO: ** (setScroll counts them too) and/or to count from the most efficient
// TODO: ** starting point, but the efficiency of this routine is not as important to
// TODO: ** the overall performance of the text display.
// TODO: */
// TODO: void TextDMakeInsertPosVisible(textDisp* textD)
// TODO: {
// TODO:    int hOffset, topLine, x, y;
// TODO:    int cursorPos = textD->cursorPos;
// TODO:    int linesFromTop = 0, do_padding = 1;
// TODO:    int cursorVPadding = (int)TEXT_OF_TEXTD(textD).cursorVPadding;
// TODO: 
// TODO:    hOffset = textD->horizOffset;
// TODO:    topLine = textD->topLineNum;
// TODO: 
// TODO:    /* Don't do padding if this is a mouse operation */
// TODO:    do_padding = ((TEXT_OF_TEXTD(textD).dragState == NOT_CLICKED) &&
// TODO:                  (cursorVPadding > 0));
// TODO: 
// TODO:    /* Find the new top line number */
// TODO:    if (cursorPos < textD->firstChar)
// TODO:    {
// TODO:       topLine -= TextDCountLines(textD, cursorPos, textD->firstChar, false);
// TODO:       /* linesFromTop = 0; */
// TODO:    }
// TODO:    else if (cursorPos > textD->lastChar && !emptyLinesVisible(textD))
// TODO:    {
// TODO:       topLine += TextDCountLines(textD, textD->lastChar -
// TODO:                                  (wrapUsesCharacter(textD, textD->lastChar) ? 0 : 1),
// TODO:                                  cursorPos, false);
// TODO:       linesFromTop = textD->nVisibleLines-1;
// TODO:    }
// TODO:    else if (cursorPos == textD->lastChar && !emptyLinesVisible(textD) &&
// TODO:             !wrapUsesCharacter(textD, textD->lastChar))
// TODO:    {
// TODO:       topLine++;
// TODO:       linesFromTop = textD->nVisibleLines-1;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       /* Avoid extra counting if cursorVPadding is disabled */
// TODO:       if (do_padding)
// TODO:          linesFromTop = TextDCountLines(textD, textD->firstChar,
// TODO:                                         cursorPos, true);
// TODO:    }
// TODO:    if (topLine < 1)
// TODO:    {
// TODO:       fprintf(stderr, "nedit: internal consistency check tl1 failed\n");
// TODO:       topLine = 1;
// TODO:    }
// TODO: 
// TODO:    if (do_padding)
// TODO:    {
// TODO:       /* Keep the cursor away from the top or bottom of screen. */
// TODO:       if (textD->nVisibleLines <= 2*(int)cursorVPadding)
// TODO:       {
// TODO:          topLine += (linesFromTop - textD->nVisibleLines/2);
// TODO:          topLine = max(topLine, 1);
// TODO:       }
// TODO:       else if (linesFromTop < (int)cursorVPadding)
// TODO:       {
// TODO:          topLine -= (cursorVPadding - linesFromTop);
// TODO:          topLine = max(topLine, 1);
// TODO:       }
// TODO:       else if (linesFromTop > textD->nVisibleLines-(int)cursorVPadding-1)
// TODO:       {
// TODO:          topLine += (linesFromTop - (textD->nVisibleLines-cursorVPadding-1));
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* Find the new setting for horizontal offset (this is a bit ungraceful).
// TODO:       If the line is visible, just use TextDPositionToXY to get the position
// TODO:       to scroll to, otherwise, do the vertical scrolling first, then the
// TODO:       horizontal */
// TODO:    if (!TextDPositionToXY(textD, cursorPos, &x, &y))
// TODO:    {
// TODO:       setScroll(textD, topLine, hOffset, true, true);
// TODO:       if (!TextDPositionToXY(textD, cursorPos, &x, &y))
// TODO:          return; /* Give up, it's not worth it (but why does it fail?) */
// TODO:    }
// TODO:    if (x > textD->left + textD->width)
// TODO:       hOffset += x - (textD->left + textD->width);
// TODO:    else if (x < textD->left)
// TODO:       hOffset += x - textD->left;
// TODO: 
// TODO:    /* Do the scroll */
// TODO:    setScroll(textD, topLine, hOffset, true, true);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Return the current preferred column along with the current
// TODO: ** visible line index (-1 if not visible) and the lineStartPos
// TODO: ** of the current insert position.
// TODO: */
// TODO: int TextDPreferredColumn(textDisp* textD, int* visLineNum, int* lineStartPos)
// TODO: {
// TODO:    int column;
// TODO: 
// TODO:    /* Find the position of the start of the line.  Use the line starts array
// TODO:    if possible, to avoid unbounded line-counting in continuous wrap mode */
// TODO:    if (posToVisibleLineNum(textD, textD->cursorPos, visLineNum))
// TODO:    {
// TODO:       *lineStartPos = textD->lineStarts[*visLineNum];
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       *lineStartPos = TextDStartOfLine(textD, textD->cursorPos);
// TODO:       *visLineNum = -1;
// TODO:    }
// TODO: 
// TODO:    /* Decide what column to move to, if there's a preferred column use that */
// TODO:    column = (textD->cursorPreferredCol >= 0)
// TODO:             ? textD->cursorPreferredCol
// TODO:             : BufCountDispChars(textD->buffer, *lineStartPos, textD->cursorPos);
// TODO:    return(column);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Return the insert position of the requested column given
// TODO: ** the lineStartPos.
// TODO: */
// TODO: int TextDPosOfPreferredCol(textDisp* textD, int column, int lineStartPos)
// TODO: {
// TODO:    int newPos;
// TODO: 
// TODO:    newPos = BufCountForwardDispChars(textD->buffer, lineStartPos, column);
// TODO:    if (textD->continuousWrap)
// TODO:    {
// TODO:       newPos = min(newPos, TextDEndOfLine(textD, lineStartPos, true));
// TODO:    }
// TODO:    return(newPos);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Cursor movement functions
// TODO: */
// TODO: int TextDMoveRight(textDisp* textD)
// TODO: {
// TODO:    if (textD->cursorPos >= textD->buffer->length)
// TODO:       return false;
// TODO:    TextDSetInsertPosition(textD, textD->cursorPos + 1);
// TODO:    return true;
// TODO: }
// TODO: 
// TODO: int TextDMoveLeft(textDisp* textD)
// TODO: {
// TODO:    if (textD->cursorPos <= 0)
// TODO:       return false;
// TODO:    TextDSetInsertPosition(textD, textD->cursorPos - 1);
// TODO:    return true;
// TODO: }
// TODO: 
// TODO: int TextDMoveUp(textDisp* textD, int absolute)
// TODO: {
// TODO:    int lineStartPos, column, prevLineStartPos, newPos, visLineNum;
// TODO: 
// TODO:    /* Find the position of the start of the line.  Use the line starts array
// TODO:       if possible, to avoid unbounded line-counting in continuous wrap mode */
// TODO:    if (absolute)
// TODO:    {
// TODO:       lineStartPos = BufStartOfLine(textD->buffer, textD->cursorPos);
// TODO:       visLineNum = -1;
// TODO:    }
// TODO:    else if (posToVisibleLineNum(textD, textD->cursorPos, &visLineNum))
// TODO:       lineStartPos = textD->lineStarts[visLineNum];
// TODO:    else
// TODO:    {
// TODO:       lineStartPos = TextDStartOfLine(textD, textD->cursorPos);
// TODO:       visLineNum = -1;
// TODO:    }
// TODO:    if (lineStartPos == 0)
// TODO:       return false;
// TODO: 
// TODO:    /* Decide what column to move to, if there's a preferred column use that */
// TODO:    column = textD->cursorPreferredCol >= 0
// TODO:             ? textD->cursorPreferredCol
// TODO:             : BufCountDispChars(textD->buffer, lineStartPos, textD->cursorPos);
// TODO: 
// TODO:    /* count forward from the start of the previous line to reach the column */
// TODO:    if (absolute)
// TODO:    {
// TODO:       prevLineStartPos = BufCountBackwardNLines(textD->buffer, lineStartPos, 1);
// TODO:    }
// TODO:    else if (visLineNum != -1 && visLineNum != 0)
// TODO:    {
// TODO:       prevLineStartPos = textD->lineStarts[visLineNum-1];
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       prevLineStartPos = TextDCountBackwardNLines(textD, lineStartPos, 1);
// TODO:    }
// TODO: 
// TODO:    newPos = BufCountForwardDispChars(textD->buffer, prevLineStartPos, column);
// TODO:    if (textD->continuousWrap && !absolute)
// TODO:       newPos = min(newPos, TextDEndOfLine(textD, prevLineStartPos, true));
// TODO: 
// TODO:    /* move the cursor */
// TODO:    TextDSetInsertPosition(textD, newPos);
// TODO: 
// TODO:    /* if a preferred column wasn't aleady established, establish it */
// TODO:    textD->cursorPreferredCol = column;
// TODO: 
// TODO:    return true;
// TODO: }
// TODO: 
// TODO: int TextDMoveDown(textDisp* textD, int absolute)
// TODO: {
// TODO:    int lineStartPos, column, nextLineStartPos, newPos, visLineNum;
// TODO: 
// TODO:    if (textD->cursorPos == textD->buffer->length)
// TODO:    {
// TODO:       return false;
// TODO:    }
// TODO: 
// TODO:    if (absolute)
// TODO:    {
// TODO:       lineStartPos = BufStartOfLine(textD->buffer, textD->cursorPos);
// TODO:       visLineNum = -1;
// TODO:    }
// TODO:    else if (posToVisibleLineNum(textD, textD->cursorPos, &visLineNum))
// TODO:    {
// TODO:       lineStartPos = textD->lineStarts[visLineNum];
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       lineStartPos = TextDStartOfLine(textD, textD->cursorPos);
// TODO:       visLineNum = -1;
// TODO:    }
// TODO: 
// TODO:    column = textD->cursorPreferredCol >= 0
// TODO:             ? textD->cursorPreferredCol
// TODO:             : BufCountDispChars(textD->buffer, lineStartPos, textD->cursorPos);
// TODO: 
// TODO:    if (absolute)
// TODO:       nextLineStartPos = BufCountForwardNLines(textD->buffer, lineStartPos, 1);
// TODO:    else
// TODO:       nextLineStartPos = TextDCountForwardNLines(textD, lineStartPos, 1, true);
// TODO: 
// TODO:    newPos = BufCountForwardDispChars(textD->buffer, nextLineStartPos, column);
// TODO: 
// TODO:    if (textD->continuousWrap && !absolute)
// TODO:    {
// TODO:       newPos = min(newPos, TextDEndOfLine(textD, nextLineStartPos, true));
// TODO:    }
// TODO: 
// TODO:    TextDSetInsertPosition(textD, newPos);
// TODO:    textD->cursorPreferredCol = column;
// TODO: 
// TODO:    return true;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Same as BufCountLines, but takes in to account wrapping if wrapping is
// TODO: ** turned on.  If the caller knows that startPos is at a line start, it
// TODO: ** can pass "startPosIsLineStart" as true to make the call more efficient
// TODO: ** by avoiding the additional step of scanning back to the last newline.
// TODO: */
// TODO: int TextDCountLines(textDisp* textD, int startPos, int endPos,
// TODO:                     int startPosIsLineStart)
// TODO: {
// TODO:    int retLines, retPos, retLineStart, retLineEnd;
// TODO: 
// TODO:    /* If we're not wrapping use simple (and more efficient) BufCountLines */
// TODO:    if (!textD->continuousWrap)
// TODO:       return BufCountLines(textD->buffer, startPos, endPos);
// TODO: 
// TODO:    wrappedLineCounter(textD, textD->buffer, startPos, endPos, INT_MAX,
// TODO:                       startPosIsLineStart, 0, &retPos, &retLines, &retLineStart,
// TODO:                       &retLineEnd);
// TODO:    return retLines;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Same as BufCountForwardNLines, but takes in to account line breaks when
// TODO: ** wrapping is turned on. If the caller knows that startPos is at a line start,
// TODO: ** it can pass "startPosIsLineStart" as true to make the call more efficient
// TODO: ** by avoiding the additional step of scanning back to the last newline.
// TODO: */
// TODO: int TextDCountForwardNLines(const textDisp* textD, const int startPos,
// TODO:                             const unsigned nLines, const bool startPosIsLineStart)
// TODO: {
// TODO:    int retLines, retPos, retLineStart, retLineEnd;
// TODO: 
// TODO:    /* if we're not wrapping use more efficient BufCountForwardNLines */
// TODO:    if (!textD->continuousWrap)
// TODO:       return BufCountForwardNLines(textD->buffer, startPos, nLines);
// TODO: 
// TODO:    /* wrappedLineCounter can't handle the 0 lines case */
// TODO:    if (nLines == 0)
// TODO:       return startPos;
// TODO: 
// TODO:    /* use the common line counting routine to count forward */
// TODO:    wrappedLineCounter(textD, textD->buffer, startPos, textD->buffer->length,
// TODO:                       nLines, startPosIsLineStart, 0, &retPos, &retLines, &retLineStart,
// TODO:                       &retLineEnd);
// TODO:    return retPos;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Same as BufEndOfLine, but takes in to account line breaks when wrapping
// TODO: ** is turned on.  If the caller knows that startPos is at a line start, it
// TODO: ** can pass "startPosIsLineStart" as true to make the call more efficient
// TODO: ** by avoiding the additional step of scanning back to the last newline.
// TODO: **
// TODO: ** Note that the definition of the end of a line is less clear when continuous
// TODO: ** wrap is on.  With continuous wrap off, it's just a pointer to the newline
// TODO: ** that ends the line.  When it's on, it's the character beyond the last
// TODO: ** DISPLAYABLE character on the line, where a whitespace character which has
// TODO: ** been "converted" to a newline for wrapping is not considered displayable.
// TODO: ** Also note that, a line can be wrapped at a non-whitespace character if the
// TODO: ** line had no whitespace.  In this case, this routine returns a pointer to
// TODO: ** the start of the next line.  This is also consistent with the model used by
// TODO: ** visLineLength.
// TODO: */
// TODO: int TextDEndOfLine(const textDisp* textD, const int pos,
// TODO:                    const bool startPosIsLineStart)
// TODO: {
// TODO:    int retLines, retPos, retLineStart, retLineEnd;
// TODO: 
// TODO:    /* If we're not wrapping use more efficient BufEndOfLine */
// TODO:    if (!textD->continuousWrap)
// TODO:       return BufEndOfLine(textD->buffer, pos);
// TODO: 
// TODO:    if (pos == textD->buffer->length)
// TODO:       return pos;
// TODO:    wrappedLineCounter(textD, textD->buffer, pos, textD->buffer->length, 1,
// TODO:                       startPosIsLineStart, 0, &retPos, &retLines, &retLineStart,
// TODO:                       &retLineEnd);
// TODO:    return retLineEnd;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Same as BufStartOfLine, but returns the character after last wrap point
// TODO: ** rather than the last newline.
// TODO: */
// TODO: int TextDStartOfLine(const textDisp* textD, const int pos)
// TODO: {
// TODO:    int retLines, retPos, retLineStart, retLineEnd;
// TODO: 
// TODO:    /* If we're not wrapping, use the more efficient BufStartOfLine */
// TODO:    if (!textD->continuousWrap)
// TODO:       return BufStartOfLine(textD->buffer, pos);
// TODO: 
// TODO:    wrappedLineCounter(textD, textD->buffer, BufStartOfLine(textD->buffer, pos),
// TODO:                       pos, INT_MAX, true, 0, &retPos, &retLines, &retLineStart,
// TODO:                       &retLineEnd);
// TODO:    return retLineStart;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Same as BufCountBackwardNLines, but takes in to account line breaks when
// TODO: ** wrapping is turned on.
// TODO: */
// TODO: int TextDCountBackwardNLines(textDisp* textD, int startPos, int nLines)
// TODO: {
// TODO:    textBuffer* buf = textD->buffer;
// TODO:    int pos, lineStart, retLines, retPos, retLineStart, retLineEnd;
// TODO: 
// TODO:    /* If we're not wrapping, use the more efficient BufCountBackwardNLines */
// TODO:    if (!textD->continuousWrap)
// TODO:       return BufCountBackwardNLines(textD->buffer, startPos, nLines);
// TODO: 
// TODO:    pos = startPos;
// TODO:    while (true)
// TODO:    {
// TODO:       lineStart = BufStartOfLine(buf, pos);
// TODO:       wrappedLineCounter(textD, textD->buffer, lineStart, pos, INT_MAX,
// TODO:                          true, 0, &retPos, &retLines, &retLineStart, &retLineEnd);
// TODO:       if (retLines > nLines)
// TODO:          return TextDCountForwardNLines(textD, lineStart, retLines-nLines,
// TODO:                                         true);
// TODO:       nLines -= retLines;
// TODO:       pos = lineStart - 1;
// TODO:       if (pos < 0)
// TODO:          return 0;
// TODO:       nLines -= 1;
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Callback attached to the text buffer to receive delete information before
// TODO: ** the modifications are actually made.
// TODO: */
// TODO: static void bufPreDeleteCB(int pos, int nDeleted, void* cbArg)
// TODO: {
// TODO:    textDisp* textD = (textDisp*)cbArg;
// TODO:    if (textD->continuousWrap &&
// TODO:          (textD->fixedFontWidth == -1 || textD->modifyingTabDist))
// TODO:       /* Note: we must perform this measurement, even if there is not a
// TODO:          single character deleted; the number of "deleted" lines is the
// TODO:          number of visual lines spanned by the real line in which the
// TODO:          modification takes place.
// TODO:          Also, a modification of the tab distance requires the same
// TODO:          kind of calculations in advance, even if the font width is "fixed",
// TODO:          because when the width of the tab characters changes, the layout
// TODO:          of the text may be completely different. */
// TODO:       measureDeletedLines(textD, pos, nDeleted);
// TODO:    else
// TODO:       textD->suppressResync = 0; /* Probably not needed, but just in case */
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Callback attached to the text buffer to receive modification information
// TODO: */
// TODO: static void bufModifiedCB(int pos, int nInserted, int nDeleted,
// TODO:                           int nRestyled, const char* deletedText, void* cbArg)
// TODO: {
// TODO:    int linesInserted, linesDeleted, startDispPos, endDispPos;
// TODO:    textDisp* textD = (textDisp*)cbArg;
// TODO:    textBuffer* buf = textD->buffer;
// TODO:    int oldFirstChar = textD->firstChar;
// TODO:    int scrolled, origCursorPos = textD->cursorPos;
// TODO:    int wrapModStart, wrapModEnd;
// TODO: 
// TODO:    /* buffer modification cancels vertical cursor motion column */
// TODO:    if (nInserted != 0 || nDeleted != 0)
// TODO:       textD->cursorPreferredCol = -1;
// TODO: 
// TODO:    /* Count the number of lines inserted and deleted, and in the case
// TODO:       of continuous wrap mode, how much has changed */
// TODO:    if (textD->continuousWrap)
// TODO:    {
// TODO:       findWrapRange(textD, deletedText, pos, nInserted, nDeleted,
// TODO:                     &wrapModStart, &wrapModEnd, &linesInserted, &linesDeleted);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       linesInserted = nInserted == 0 ? 0 :
// TODO:                       BufCountLines(buf, pos, pos + nInserted);
// TODO:       linesDeleted = nDeleted == 0 ? 0 : countLines(deletedText);
// TODO:    }
// TODO: 
// TODO:    /* Update the line starts and topLineNum */
// TODO:    if (nInserted != 0 || nDeleted != 0)
// TODO:    {
// TODO:       if (textD->continuousWrap)
// TODO:       {
// TODO:          updateLineStarts(textD, wrapModStart, wrapModEnd-wrapModStart,
// TODO:                           nDeleted + pos-wrapModStart + (wrapModEnd-(pos+nInserted)),
// TODO:                           linesInserted, linesDeleted, &scrolled);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          updateLineStarts(textD, pos, nInserted, nDeleted, linesInserted,
// TODO:                           linesDeleted, &scrolled);
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:       scrolled = false;
// TODO: 
// TODO:    /* If we're counting non-wrapped lines as well, maintain the absolute
// TODO:       (non-wrapped) line number of the text displayed */
// TODO:    if (maintainingAbsTopLineNum(textD) && (nInserted != 0 || nDeleted != 0))
// TODO:    {
// TODO:       if (pos + nDeleted < oldFirstChar)
// TODO:          textD->absTopLineNum += BufCountLines(buf, pos, pos + nInserted) -
// TODO:                                  countLines(deletedText);
// TODO:       else if (pos < oldFirstChar)
// TODO:          resetAbsLineNum(textD);
// TODO:    }
// TODO: 
// TODO:    /* Update the line count for the whole buffer */
// TODO:    textD->nBufferLines += linesInserted - linesDeleted;
// TODO: 
// TODO:    /* Update the scroll bar ranges (and value if the value changed).  Note
// TODO:       that updating the horizontal scroll bar range requires scanning the
// TODO:       entire displayed text, however, it doesn't seem to hurt performance
// TODO:       much.  Note also, that the horizontal scroll bar update routine is
// TODO:       allowed to re-adjust horizOffset if there is blank space to the right
// TODO:       of all lines of text. */
// TODO:    updateVScrollBarRange(textD);
// TODO:    scrolled |= updateHScrollBarRange(textD);
// TODO: 
// TODO:    /* Update the cursor position */
// TODO:    if (textD->cursorToHint != NO_HINT)
// TODO:    {
// TODO:       textD->cursorPos = textD->cursorToHint;
// TODO:       textD->cursorToHint = NO_HINT;
// TODO:    }
// TODO:    else if (textD->cursorPos > pos)
// TODO:    {
// TODO:       if (textD->cursorPos < pos + nDeleted)
// TODO:          textD->cursorPos = pos;
// TODO:       else
// TODO:          textD->cursorPos += nInserted - nDeleted;
// TODO:    }
// TODO: 
// TODO:    /* If the changes caused scrolling, re-paint everything and we're done. */
// TODO:    if (scrolled)
// TODO:    {
// TODO:       blankCursorProtrusions(textD);
// TODO:       TextDRedisplayRect(textD, 0, textD->top, textD->width + textD->left,
// TODO:                          textD->height);
// TODO:       if (textD->styleBuffer)  /* See comments in extendRangeForStyleMods */
// TODO:       {
// TODO:          textD->styleBuffer->primary.selected = false;
// TODO:          textD->styleBuffer->primary.zeroWidth = false;
// TODO:       }
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* If the changes didn't cause scrolling, decide the range of characters
// TODO:       that need to be re-painted.  Also if the cursor position moved, be
// TODO:       sure that the redisplay range covers the old cursor position so the
// TODO:       old cursor gets erased, and erase the bits of the cursor which extend
// TODO:       beyond the left and right edges of the text. */
// TODO:    startDispPos = textD->continuousWrap ? wrapModStart : pos;
// TODO:    if (origCursorPos == startDispPos && textD->cursorPos != startDispPos)
// TODO:       startDispPos = min(startDispPos, origCursorPos-1);
// TODO:    if (linesInserted == linesDeleted)
// TODO:    {
// TODO:       if (nInserted == 0 && nDeleted == 0)
// TODO:          endDispPos = pos + nRestyled;
// TODO:       else
// TODO:       {
// TODO:          endDispPos = textD->continuousWrap ? wrapModEnd :
// TODO:                       BufEndOfLine(buf, pos + nInserted) + 1;
// TODO:          if (origCursorPos >= startDispPos &&
// TODO:                (origCursorPos <= endDispPos || endDispPos == buf->length))
// TODO:             blankCursorProtrusions(textD);
// TODO:       }
// TODO:       /* If more than one line is inserted/deleted, a line break may have
// TODO:          been inserted or removed in between, and the line numbers may
// TODO:          have changed. If only one line is altered, line numbers cannot
// TODO:          be affected (the insertion or removal of a line break always
// TODO:          results in at least two lines being redrawn). */
// TODO:       if (linesInserted > 1) redrawLineNumbers(textD, false);
// TODO:    }
// TODO:    else     /* linesInserted != linesDeleted */
// TODO:    {
// TODO:       endDispPos = textD->lastChar + 1;
// TODO:       if (origCursorPos >= pos)
// TODO:          blankCursorProtrusions(textD);
// TODO:       redrawLineNumbers(textD, false);
// TODO:    }
// TODO: 
// TODO:    /* If there is a style buffer, check if the modification caused additional
// TODO:       changes that need to be redisplayed.  (Redisplaying separately would
// TODO:       cause double-redraw on almost every modification involving styled
// TODO:       text).  Extend the redraw range to incorporate style changes */
// TODO:    if (textD->styleBuffer)
// TODO:       extendRangeForStyleMods(textD, &startDispPos, &endDispPos);
// TODO: 
// TODO:    /* Redisplay computed range */
// TODO:    textDRedisplayRange(textD, startDispPos, endDispPos);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** In continuous wrap mode, internal line numbers are calculated after
// TODO: ** wrapping.  A separate non-wrapped line count is maintained when line
// TODO: ** numbering is turned on.  There is some performance cost to maintaining this
// TODO: ** line count, so normally absolute line numbers are not tracked if line
// TODO: ** numbering is off.  This routine allows callers to specify that they still
// TODO: ** want this line count maintained (for use via TextDPosToLineAndCol).
// TODO: ** More specifically, this allows the line number reported in the statistics
// TODO: ** line to be calibrated in absolute lines, rather than post-wrapped lines.
// TODO: */
// TODO: void TextDMaintainAbsLineNum(textDisp* textD, int state)
// TODO: {
// TODO:    textD->needAbsTopLineNum = state;
// TODO:    resetAbsLineNum(textD);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Returns the absolute (non-wrapped) line number of the first line displayed.
// TODO: ** Returns 0 if the absolute top line number is not being maintained.
// TODO: */
// TODO: static int getAbsTopLineNum(textDisp* textD)
// TODO: {
// TODO:    if (!textD->continuousWrap)
// TODO:       return textD->topLineNum;
// TODO:    if (maintainingAbsTopLineNum(textD))
// TODO:       return textD->absTopLineNum;
// TODO:    return 0;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Re-calculate absolute top line number for a change in scroll position.
// TODO: */
// TODO: static void offsetAbsLineNum(textDisp* textD, int oldFirstChar)
// TODO: {
// TODO:    if (maintainingAbsTopLineNum(textD))
// TODO:    {
// TODO:       if (textD->firstChar < oldFirstChar)
// TODO:          textD->absTopLineNum -= BufCountLines(textD->buffer,
// TODO:                                                textD->firstChar, oldFirstChar);
// TODO:       else
// TODO:          textD->absTopLineNum += BufCountLines(textD->buffer,
// TODO:                                                oldFirstChar, textD->firstChar);
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Return true if a separate absolute top line number is being maintained
// TODO: ** (for displaying line numbers or showing in the statistics line).
// TODO: */
// TODO: static int maintainingAbsTopLineNum(textDisp* textD)
// TODO: {
// TODO:    return textD->continuousWrap &&
// TODO:           (textD->lineNumWidth != 0 || textD->needAbsTopLineNum);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Count lines from the beginning of the buffer to reestablish the
// TODO: ** absolute (non-wrapped) top line number.  If mode is not continuous wrap,
// TODO: ** or the number is not being maintained, does nothing.
// TODO: */
// TODO: static void resetAbsLineNum(textDisp* textD)
// TODO: {
// TODO:    textD->absTopLineNum = 1;
// TODO:    offsetAbsLineNum(textD, 0);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Find the line number of position "pos" relative to the first line of
// TODO: ** displayed text. Returns false if the line is not displayed.
// TODO: */
// TODO: static int posToVisibleLineNum(textDisp* textD, int pos, int* lineNum)
// TODO: {
// TODO:    int i;
// TODO: 
// TODO:    if (pos < textD->firstChar)
// TODO:       return false;
// TODO:    if (pos > textD->lastChar)
// TODO:    {
// TODO:       if (emptyLinesVisible(textD))
// TODO:       {
// TODO:          if (textD->lastChar < textD->buffer->length)
// TODO:          {
// TODO:             if (!posToVisibleLineNum(textD, textD->lastChar, lineNum))
// TODO:             {
// TODO:                fprintf(stderr, "nedit: Consistency check ptvl failed\n");
// TODO:                return false;
// TODO:             }
// TODO:             return ++(*lineNum) <= textD->nVisibleLines-1;
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             posToVisibleLineNum(textD, max(textD->lastChar-1, 0), lineNum);
// TODO:             return true;
// TODO:          }
// TODO:       }
// TODO:       return false;
// TODO:    }
// TODO: 
// TODO:    for (i=textD->nVisibleLines-1; i>=0; i--)
// TODO:    {
// TODO:       if (textD->lineStarts[i] != -1 && pos >= textD->lineStarts[i])
// TODO:       {
// TODO:          *lineNum = i;
// TODO:          return true;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    return false;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Redisplay the text on a single line represented by "visLineNum" (the
// TODO: ** number of lines down from the top of the display), limited by
// TODO: ** "leftClip" and "rightClip" window coordinates and "leftCharIndex" and
// TODO: ** "rightCharIndex" character positions (not including the character at
// TODO: ** position "rightCharIndex").
// TODO: **
// TODO: ** The cursor is also drawn if it appears on the line.
// TODO: */
// TODO: static void redisplayLine(textDisp* textD, int visLineNum, int leftClip,
// TODO:                           int rightClip, int leftCharIndex, int rightCharIndex)
// TODO: {
// TODO:    textBuffer* buf = textD->buffer;
// TODO:    int i, x, y, startX, charIndex, lineStartPos, lineLen, fontHeight;
// TODO:    int stdCharWidth, charWidth, startIndex, charStyle, style;
// TODO:    int charLen, outStartIndex, outIndex, cursorX = 0, hasCursor = false;
// TODO:    int dispIndexOffset, cursorPos = textD->cursorPos, y_orig;
// TODO:    char expandedChar[MAX_EXP_CHAR_LEN], outStr[MAX_DISP_LINE_LEN];
// TODO:    char* lineStr, *outPtr;
// TODO:    char baseChar;
// TODO: 
// TODO:    /* If line is not displayed, skip it */
// TODO:    if (visLineNum < 0 || visLineNum >= textD->nVisibleLines)
// TODO:       return;
// TODO: 
// TODO:    /* Shrink the clipping range to the active display area */
// TODO:    leftClip = max(textD->left, leftClip);
// TODO:    rightClip = min(rightClip, textD->left + textD->width);
// TODO: 
// TODO:    if (leftClip > rightClip)
// TODO:    {
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Calculate y coordinate of the string to draw */
// TODO:    fontHeight = textD->ascent + textD->descent;
// TODO:    y = textD->top + visLineNum * fontHeight;
// TODO: 
// TODO:    /* Get the text, length, and  buffer position of the line to display */
// TODO:    lineStartPos = textD->lineStarts[visLineNum];
// TODO:    if (lineStartPos == -1)
// TODO:    {
// TODO:       lineLen = 0;
// TODO:       lineStr = NULL;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       lineLen = visLineLength(textD, visLineNum);
// TODO:       lineStr = BufGetRange(buf, lineStartPos, lineStartPos + lineLen);
// TODO:    }
// TODO: 
// TODO:    /* Space beyond the end of the line is still counted in units of characters
// TODO:       of a standardized character width (this is done mostly because style
// TODO:       changes based on character position can still occur in this region due
// TODO:       to rectangular selections).  stdCharWidth must be non-zero to prevent a
// TODO:       potential infinite loop if x does not advance */
// TODO:    stdCharWidth = textD->fontStruct->max_bounds.width;
// TODO:    if (stdCharWidth <= 0)
// TODO:    {
// TODO:       fprintf(stderr, "nedit: Internal Error, bad font measurement\n");
// TODO:       XtFree(lineStr);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Rectangular selections are based on "real" line starts (after a newline
// TODO:       or start of buffer).  Calculate the difference between the last newline
// TODO:       position and the line start we're using.  Since scanning back to find a
// TODO:       newline is expensive, only do so if there's actually a rectangular
// TODO:       selection which needs it */
// TODO:    if (textD->continuousWrap && (rangeTouchesRectSel(&buf->primary,
// TODO:                                  lineStartPos, lineStartPos + lineLen) || rangeTouchesRectSel(
// TODO:                                     &buf->secondary, lineStartPos, lineStartPos + lineLen) ||
// TODO:                                  rangeTouchesRectSel(&buf->highlight, lineStartPos,
// TODO:                                        lineStartPos + lineLen)))
// TODO:    {
// TODO:       dispIndexOffset = BufCountDispChars(buf,
// TODO:                                           BufStartOfLine(buf, lineStartPos), lineStartPos);
// TODO:    }
// TODO:    else
// TODO:       dispIndexOffset = 0;
// TODO: 
// TODO:    /* Step through character positions from the beginning of the line (even if
// TODO:       that's off the left edge of the displayed area) to find the first
// TODO:       character position that's not clipped, and the x coordinate for drawing
// TODO:       that character */
// TODO:    x = textD->left - textD->horizOffset;
// TODO:    outIndex = 0;
// TODO: 
// TODO:    for (charIndex = 0; ; charIndex++)
// TODO:    {
// TODO:       baseChar = '\0';
// TODO:       charLen = charIndex >= lineLen
// TODO:                 ? 1
// TODO:                 : BufExpandCharacter(baseChar = lineStr[charIndex], outIndex,
// TODO:                                      expandedChar, buf->tabDist, buf->nullSubsChar);
// TODO:       style = styleOfPos(textD, lineStartPos, lineLen, charIndex,
// TODO:                          outIndex + dispIndexOffset, baseChar);
// TODO:       charWidth = charIndex >= lineLen
// TODO:                   ? stdCharWidth
// TODO:                   : stringWidth(textD, expandedChar, charLen, style);
// TODO: 
// TODO:       if (x + charWidth >= leftClip && charIndex >= leftCharIndex)
// TODO:       {
// TODO:          startIndex = charIndex;
// TODO:          outStartIndex = outIndex;
// TODO:          startX = x;
// TODO:          break;
// TODO:       }
// TODO:       x += charWidth;
// TODO:       outIndex += charLen;
// TODO:    }
// TODO: 
// TODO:    /* Scan character positions from the beginning of the clipping range, and
// TODO:       draw parts whenever the style changes (also note if the cursor is on
// TODO:       this line, and where it should be drawn to take advantage of the x
// TODO:       position which we've gone to so much trouble to calculate) */
// TODO:    outPtr = outStr;
// TODO:    outIndex = outStartIndex;
// TODO:    x = startX;
// TODO:    for (charIndex = startIndex; charIndex < rightCharIndex; charIndex++)
// TODO:    {
// TODO:       if (lineStartPos+charIndex == cursorPos)
// TODO:       {
// TODO:          if (charIndex < lineLen
// TODO:                || (charIndex == lineLen && cursorPos >= buf->length))
// TODO:          {
// TODO:             hasCursor = true;
// TODO:             cursorX = x - 1;
// TODO:          }
// TODO:          else if (charIndex == lineLen)
// TODO:          {
// TODO:             if (wrapUsesCharacter(textD, cursorPos))
// TODO:             {
// TODO:                hasCursor = true;
// TODO:                cursorX = x - 1;
// TODO:             }
// TODO:          }
// TODO:       }
// TODO: 
// TODO:       baseChar = '\0';
// TODO:       charLen = charIndex >= lineLen
// TODO:                 ? 1
// TODO:                 : BufExpandCharacter(baseChar = lineStr[charIndex], outIndex,
// TODO:                                      expandedChar, buf->tabDist, buf->nullSubsChar);
// TODO:       charStyle = styleOfPos(textD, lineStartPos, lineLen, charIndex,
// TODO:                              outIndex + dispIndexOffset, baseChar);
// TODO:       for (i = 0; i < charLen; i++)
// TODO:       {
// TODO:          if (i != 0 && charIndex < lineLen && lineStr[charIndex] == '\t')
// TODO:          {
// TODO:             charStyle = styleOfPos(textD, lineStartPos, lineLen, charIndex,
// TODO:                                    outIndex + dispIndexOffset, '\t');
// TODO:          }
// TODO: 
// TODO:          if (charStyle != style)
// TODO:          {
// TODO:             drawString(textD, style, startX, y, x, outStr, outPtr - outStr);
// TODO:             outPtr = outStr;
// TODO:             startX = x;
// TODO:             style = charStyle;
// TODO:          }
// TODO: 
// TODO:          if (charIndex < lineLen)
// TODO:          {
// TODO:             *outPtr = expandedChar[i];
// TODO:             charWidth = stringWidth(textD, &expandedChar[i], 1, charStyle);
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             charWidth = stdCharWidth;
// TODO:          }
// TODO: 
// TODO:          outPtr++;
// TODO:          x += charWidth;
// TODO:          outIndex++;
// TODO:       }
// TODO: 
// TODO:       if (outPtr - outStr + MAX_EXP_CHAR_LEN >= MAX_DISP_LINE_LEN
// TODO:             || x >= rightClip)
// TODO:       {
// TODO:          break;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* Draw the remaining style segment */
// TODO:    drawString(textD, style, startX, y, x, outStr, outPtr - outStr);
// TODO: 
// TODO:    /* Draw the cursor if part of it appeared on the redisplayed part of
// TODO:       this line.  Also check for the cases which are not caught as the
// TODO:       line is scanned above: when the cursor appears at the very end
// TODO:       of the redisplayed section. */
// TODO:    y_orig = textD->cursorY;
// TODO:    if (textD->cursorOn)
// TODO:    {
// TODO:       if (hasCursor)
// TODO:       {
// TODO:          drawCursor(textD, cursorX, y);
// TODO:       }
// TODO:       else if (charIndex < lineLen
// TODO:                && (lineStartPos+charIndex+1 == cursorPos)
// TODO:                && x == rightClip)
// TODO:       {
// TODO:          if (cursorPos >= buf->length)
// TODO:          {
// TODO:             drawCursor(textD, x - 1, y);
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             if (wrapUsesCharacter(textD, cursorPos))
// TODO:             {
// TODO:                drawCursor(textD, x - 1, y);
// TODO:             }
// TODO:          }
// TODO:       }
// TODO:       else if ((lineStartPos + rightCharIndex) == cursorPos)
// TODO:       {
// TODO:          drawCursor(textD, x - 1, y);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* If the y position of the cursor has changed, redraw the calltip */
// TODO:    if (hasCursor && (y_orig != textD->cursorY || y_orig != y))
// TODO:       TextDRedrawCalltip(textD, 0);
// TODO: 
// TODO:    XtFree(lineStr);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Draw a string or blank area according to parameter "style", using the
// TODO: ** appropriate colors and drawing method for that style, with top left
// TODO: ** corner at x, y.  If style says to draw text, use "string" as source of
// TODO: ** characters, and draw "nChars", if style is FILL, erase
// TODO: ** rectangle where text would have drawn from x to toX and from y to
// TODO: ** the maximum y extent of the current font(s).
// TODO: */
// TODO: static void drawString(textDisp* textD, int style, int x, int y, int toX,
// TODO:                        char* string, int nChars)
// TODO: {
// TODO:    GC gc, bgGC;
// TODO:    XGCValues gcValues;
// TODO:    XFontStruct* fs = textD->fontStruct;
// TODO:    Pixel bground = textD->bgPixel;
// TODO:    Pixel fground = textD->fgPixel;
// TODO:    int underlineStyle = FALSE;
// TODO: 
// TODO:    /* Don't draw if widget isn't realized */
// TODO:    if (XtWindow(textD->w) == 0)
// TODO:       return;
// TODO: 
// TODO:    /* select a GC */
// TODO:    if (style & (STYLE_LOOKUP_MASK | BACKLIGHT_MASK | RANGESET_MASK))
// TODO:    {
// TODO:       gc = bgGC = textD->styleGC;
// TODO:    }
// TODO:    else if (style & HIGHLIGHT_MASK)
// TODO:    {
// TODO:       gc = textD->highlightGC;
// TODO:       bgGC = textD->highlightBGGC;
// TODO:    }
// TODO:    else if (style & PRIMARY_MASK)
// TODO:    {
// TODO:       gc = textD->selectGC;
// TODO:       bgGC = textD->selectBGGC;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       gc = bgGC = textD->gc;
// TODO:    }
// TODO: 
// TODO:    if (gc == textD->styleGC)
// TODO:    {
// TODO:       /* we have work to do */
// TODO:       styleTableEntry* styleRec;
// TODO:       /* Set font, color, and gc depending on style.  For normal text, GCs
// TODO:          for normal drawing, or drawing within a selection or highlight are
// TODO:          pre-allocated and pre-configured.  For syntax highlighting, GCs are
// TODO:          configured here, on the fly. */
// TODO:       if (style & STYLE_LOOKUP_MASK)
// TODO:       {
// TODO:          styleRec = &textD->styleTable[(style & STYLE_LOOKUP_MASK) - ASCII_A];
// TODO:          underlineStyle = styleRec->underline;
// TODO:          fs = styleRec->font;
// TODO:          gcValues.font = fs->fid;
// TODO:          fground = styleRec->color;
// TODO:          /* here you could pick up specific select and highlight fground */
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          styleRec = NULL;
// TODO:          gcValues.font = fs->fid;
// TODO:          fground = textD->fgPixel;
// TODO:       }
// TODO:       /* Background color priority order is:
// TODO:          1 Primary(Selection), 2 Highlight(Parens),
// TODO:          3 Rangeset, 4 SyntaxHighlightStyle,
// TODO:          5 Backlight (if NOT fill), 6 DefaultBackground */
// TODO:       bground =
// TODO:          style & PRIMARY_MASK   ? textD->selectBGPixel :
// TODO:          style & HIGHLIGHT_MASK ? textD->highlightBGPixel :
// TODO:          style & RANGESET_MASK  ?
// TODO:          getRangesetColor(textD,
// TODO:                           (style&RANGESET_MASK)>>RANGESET_SHIFT,
// TODO:                           bground) :
// TODO:          styleRec && styleRec->bgColorName ? styleRec->bgColor :
// TODO:          (style & BACKLIGHT_MASK) && !(style & FILL_MASK) ?
// TODO:          textD->bgClassPixel[(style>>BACKLIGHT_SHIFT) & 0xff] :
// TODO:          textD->bgPixel;
// TODO:       if (fground == bground) /* B&W kludge */
// TODO:          fground = textD->bgPixel;
// TODO:       /* set up gc for clearing using the foreground color entry */
// TODO:       gcValues.foreground = gcValues.background = bground;
// TODO:       XChangeGC(XtDisplay(textD->w), gc,
// TODO:                 GCFont | GCForeground | GCBackground, &gcValues);
// TODO:    }
// TODO: 
// TODO:    /* Draw blank area rather than text, if that was the request */
// TODO:    if (style & FILL_MASK)
// TODO:    {
// TODO:       /* wipes out to right hand edge of widget */
// TODO:       if (toX >= textD->left)
// TODO:          clearRect(textD, bgGC, max(x, textD->left), y,
// TODO:                    toX - max(x, textD->left), textD->ascent + textD->descent);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* If any space around the character remains unfilled (due to use of
// TODO:       different sized fonts for highlighting), fill in above or below
// TODO:       to erase previously drawn characters */
// TODO:    if (fs->ascent < textD->ascent)
// TODO:       clearRect(textD, bgGC, x, y, toX - x, textD->ascent - fs->ascent);
// TODO:    if (fs->descent < textD->descent)
// TODO:       clearRect(textD, bgGC, x, y + textD->ascent + fs->descent, toX - x,
// TODO:                 textD->descent - fs->descent);
// TODO: 
// TODO:    /* set up gc for writing text (set foreground properly) */
// TODO:    if (bgGC == textD->styleGC)
// TODO:    {
// TODO:       gcValues.foreground = fground;
// TODO:       XChangeGC(XtDisplay(textD->w), gc, GCForeground, &gcValues);
// TODO:    }
// TODO: 
// TODO:    /* Draw the string using gc and font set above */
// TODO:    XDrawImageString(XtDisplay(textD->w), XtWindow(textD->w), gc, x,
// TODO:                     y + textD->ascent, string, nChars);
// TODO: 
// TODO:    /* Underline if style is secondary selection */
// TODO:    if (style & SECONDARY_MASK || underlineStyle)
// TODO:    {
// TODO:       /* restore foreground in GC (was set to background by clearRect()) */
// TODO:       gcValues.foreground = fground;
// TODO:       XChangeGC(XtDisplay(textD->w), gc,
// TODO:                 GCForeground, &gcValues);
// TODO:       /* draw underline */
// TODO:       XDrawLine(XtDisplay(textD->w), XtWindow(textD->w), gc, x,
// TODO:                 y + textD->ascent, toX - 1, y + textD->ascent);
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Clear a rectangle with the appropriate background color for "style"
// TODO: */
// TODO: static void clearRect(textDisp* textD, GC gc, int x, int y,
// TODO:                       int width, int height)
// TODO: {
// TODO:    /* A width of zero means "clear to end of window" to XClearArea */
// TODO:    if (width == 0 || XtWindow(textD->w) == 0)
// TODO:       return;
// TODO: 
// TODO:    if (gc == textD->gc)
// TODO:    {
// TODO:       XClearArea(XtDisplay(textD->w), XtWindow(textD->w), x, y,
// TODO:                  width, height, false);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       XFillRectangle(XtDisplay(textD->w), XtWindow(textD->w),
// TODO:                      gc, x, y, width, height);
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Draw a cursor with top center at x, y.
// TODO: */
// TODO: static void drawCursor(textDisp* textD, int x, int y)
// TODO: {
// TODO:    XSegment segs[5];
// TODO:    int left, right, cursorWidth, midY;
// TODO:    int fontWidth = textD->fontStruct->min_bounds.width, nSegs = 0;
// TODO:    int fontHeight = textD->ascent + textD->descent;
// TODO:    int bot = y + fontHeight - 1;
// TODO: 
// TODO:    if (XtWindow(textD->w) == 0 || x < textD->left-1 ||
// TODO:          x > textD->left + textD->width)
// TODO:       return;
// TODO: 
// TODO:    /* For cursors other than the block, make them around 2/3 of a character
// TODO:       width, rounded to an even number of pixels so that X will draw an
// TODO:       odd number centered on the stem at x. */
// TODO:    cursorWidth = (fontWidth/3) * 2;
// TODO:    left = x - cursorWidth/2;
// TODO:    right = left + cursorWidth;
// TODO: 
// TODO:    /* Create segments and draw cursor */
// TODO:    if (textD->cursorStyle == CARET_CURSOR)
// TODO:    {
// TODO:       midY = bot - fontHeight/5;
// TODO:       segs[0].x1 = left;
// TODO:       segs[0].y1 = bot;
// TODO:       segs[0].x2 = x;
// TODO:       segs[0].y2 = midY;
// TODO:       segs[1].x1 = x;
// TODO:       segs[1].y1 = midY;
// TODO:       segs[1].x2 = right;
// TODO:       segs[1].y2 = bot;
// TODO:       segs[2].x1 = left;
// TODO:       segs[2].y1 = bot;
// TODO:       segs[2].x2 = x;
// TODO:       segs[2].y2=midY-1;
// TODO:       segs[3].x1 = x;
// TODO:       segs[3].y1=midY-1;
// TODO:       segs[3].x2 = right;
// TODO:       segs[3].y2 = bot;
// TODO:       nSegs = 4;
// TODO:    }
// TODO:    else if (textD->cursorStyle == NORMAL_CURSOR)
// TODO:    {
// TODO:       segs[0].x1 = left;
// TODO:       segs[0].y1 = y;
// TODO:       segs[0].x2 = right;
// TODO:       segs[0].y2 = y;
// TODO:       segs[1].x1 = x;
// TODO:       segs[1].y1 = y;
// TODO:       segs[1].x2 = x;
// TODO:       segs[1].y2 = bot;
// TODO:       segs[2].x1 = left;
// TODO:       segs[2].y1 = bot;
// TODO:       segs[2].x2 = right;
// TODO:       segs[2].y2=bot;
// TODO:       nSegs = 3;
// TODO:    }
// TODO:    else if (textD->cursorStyle == HEAVY_CURSOR)
// TODO:    {
// TODO:       segs[0].x1 = x-1;
// TODO:       segs[0].y1 = y;
// TODO:       segs[0].x2 = x-1;
// TODO:       segs[0].y2 = bot;
// TODO:       segs[1].x1 = x;
// TODO:       segs[1].y1 = y;
// TODO:       segs[1].x2 = x;
// TODO:       segs[1].y2 = bot;
// TODO:       segs[2].x1 = x+1;
// TODO:       segs[2].y1 = y;
// TODO:       segs[2].x2 = x+1;
// TODO:       segs[2].y2 = bot;
// TODO:       segs[3].x1 = left;
// TODO:       segs[3].y1 = y;
// TODO:       segs[3].x2 = right;
// TODO:       segs[3].y2 = y;
// TODO:       segs[4].x1 = left;
// TODO:       segs[4].y1 = bot;
// TODO:       segs[4].x2 = right;
// TODO:       segs[4].y2=bot;
// TODO:       nSegs = 5;
// TODO:    }
// TODO:    else if (textD->cursorStyle == DIM_CURSOR)
// TODO:    {
// TODO:       midY = y + fontHeight/2;
// TODO:       segs[0].x1 = x;
// TODO:       segs[0].y1 = y;
// TODO:       segs[0].x2 = x;
// TODO:       segs[0].y2 = y;
// TODO:       segs[1].x1 = x;
// TODO:       segs[1].y1 = midY;
// TODO:       segs[1].x2 = x;
// TODO:       segs[1].y2 = midY;
// TODO:       segs[2].x1 = x;
// TODO:       segs[2].y1 = bot;
// TODO:       segs[2].x2 = x;
// TODO:       segs[2].y2 = bot;
// TODO:       nSegs = 3;
// TODO:    }
// TODO:    else if (textD->cursorStyle == BLOCK_CURSOR)
// TODO:    {
// TODO:       right = x + fontWidth;
// TODO:       segs[0].x1 = x;
// TODO:       segs[0].y1 = y;
// TODO:       segs[0].x2 = right;
// TODO:       segs[0].y2 = y;
// TODO:       segs[1].x1 = right;
// TODO:       segs[1].y1 = y;
// TODO:       segs[1].x2 = right;
// TODO:       segs[1].y2=bot;
// TODO:       segs[2].x1 = right;
// TODO:       segs[2].y1 = bot;
// TODO:       segs[2].x2 = x;
// TODO:       segs[2].y2 = bot;
// TODO:       segs[3].x1 = x;
// TODO:       segs[3].y1 = bot;
// TODO:       segs[3].x2 = x;
// TODO:       segs[3].y2 = y;
// TODO:       nSegs = 4;
// TODO:    }
// TODO:    XDrawSegments(XtDisplay(textD->w), XtWindow(textD->w),
// TODO:                  textD->cursorFGGC, segs, nSegs);
// TODO: 
// TODO:    /* Save the last position drawn */
// TODO:    textD->cursorX = x;
// TODO:    textD->cursorY = y;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Determine the drawing method to use to draw a specific character from "buf".
// TODO: ** "lineStartPos" gives the character index where the line begins, "lineIndex",
// TODO: ** the number of characters past the beginning of the line, and "dispIndex",
// TODO: ** the number of displayed characters past the beginning of the line.  Passing
// TODO: ** lineStartPos of -1 returns the drawing style for "no text".
// TODO: **
// TODO: ** Why not just: styleOfPos(textD, pos)?  Because style applies to blank areas
// TODO: ** of the window beyond the text boundaries, and because this routine must also
// TODO: ** decide whether a position is inside of a rectangular selection, and do so
// TODO: ** efficiently, without re-counting character positions from the start of the
// TODO: ** line.
// TODO: **
// TODO: ** Note that style is a somewhat incorrect name, drawing method would
// TODO: ** be more appropriate.
// TODO: */
// TODO: static int styleOfPos(textDisp* textD, int lineStartPos,
// TODO:                       int lineLen, int lineIndex, int dispIndex, int thisChar)
// TODO: {
// TODO:    textBuffer* buf = textD->buffer;
// TODO:    textBuffer* styleBuf = textD->styleBuffer;
// TODO:    int pos, style = 0;
// TODO: 
// TODO:    if (lineStartPos == -1 || buf == NULL)
// TODO:       return FILL_MASK;
// TODO: 
// TODO:    pos = lineStartPos + min(lineIndex, lineLen);
// TODO: 
// TODO:    if (lineIndex >= lineLen)
// TODO:       style = FILL_MASK;
// TODO:    else if (styleBuf != NULL)
// TODO:    {
// TODO:       style = (unsigned char)BufGetCharacter(styleBuf, pos);
// TODO:       if (style == textD->unfinishedStyle)
// TODO:       {
// TODO:          /* encountered "unfinished" style, trigger parsing */
// TODO:          (textD->unfinishedHighlightCB)(textD, pos, textD->highlightCBArg);
// TODO:          style = (unsigned char)BufGetCharacter(styleBuf, pos);
// TODO:       }
// TODO:    }
// TODO:    if (inSelection(&buf->primary, pos, lineStartPos, dispIndex))
// TODO:       style |= PRIMARY_MASK;
// TODO:    if (inSelection(&buf->highlight, pos, lineStartPos, dispIndex))
// TODO:       style |= HIGHLIGHT_MASK;
// TODO:    if (inSelection(&buf->secondary, pos, lineStartPos, dispIndex))
// TODO:       style |= SECONDARY_MASK;
// TODO:    /* store in the RANGESET_MASK portion of style the rangeset index for pos */
// TODO:    if (buf->rangesetTable)
// TODO:    {
// TODO:       int rangesetIndex = RangesetIndex1ofPos(buf->rangesetTable, pos, true);
// TODO:       style |= ((rangesetIndex << RANGESET_SHIFT) & RANGESET_MASK);
// TODO:    }
// TODO:    /* store in the BACKLIGHT_MASK portion of style the background color class
// TODO:       of the character thisChar */
// TODO:    if (textD->bgClass)
// TODO:    {
// TODO:       style |= (textD->bgClass[(unsigned char)thisChar]<<BACKLIGHT_SHIFT);
// TODO:    }
// TODO:    return style;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Find the width of a string in the font of a particular style
// TODO: */
// TODO: static int stringWidth(const textDisp* textD, const char* string,
// TODO:                        const int length, const int style)
// TODO: {
// TODO:    XFontStruct* fs;
// TODO: 
// TODO:    if (style & STYLE_LOOKUP_MASK)
// TODO:       fs = textD->styleTable[(style & STYLE_LOOKUP_MASK) - ASCII_A].font;
// TODO:    else
// TODO:       fs = textD->fontStruct;
// TODO:    return XTextWidth(fs, (char*) string, (int) length);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Return true if position "pos" with indentation "dispIndex" is in
// TODO: ** selection "sel"
// TODO: */
// TODO: static int inSelection(selection* sel, int pos, int lineStartPos, int dispIndex)
// TODO: {
// TODO:    return sel->selected &&
// TODO:           ((!sel->rectangular &&
// TODO:             pos >= sel->start && pos < sel->end) ||
// TODO:            (sel->rectangular &&
// TODO:             pos >= sel->start && lineStartPos <= sel->end &&
// TODO:             dispIndex >= sel->rectStart && dispIndex < sel->rectEnd));
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Translate window coordinates to the nearest (insert cursor or character
// TODO: ** cell) text position.  The parameter posType specifies how to interpret the
// TODO: ** position: CURSOR_POS means translate the coordinates to the nearest cursor
// TODO: ** position, and CHARACTER_POS means return the position of the character
// TODO: ** closest to (x, y).
// TODO: */
// TODO: static int xyToPos(textDisp* textD, int x, int y, int posType)
// TODO: {
// TODO:    int charIndex, lineStart, lineLen, fontHeight;
// TODO:    int charWidth, charLen, charStyle, visLineNum, xStep, outIndex;
// TODO:    char* lineStr, expandedChar[MAX_EXP_CHAR_LEN];
// TODO: 
// TODO:    /* Find the visible line number corresponding to the y coordinate */
// TODO:    fontHeight = textD->ascent + textD->descent;
// TODO:    visLineNum = (y - textD->top) / fontHeight;
// TODO:    if (visLineNum < 0)
// TODO:       return textD->firstChar;
// TODO:    if (visLineNum >= textD->nVisibleLines)
// TODO:       visLineNum = textD->nVisibleLines - 1;
// TODO: 
// TODO:    /* Find the position at the start of the line */
// TODO:    lineStart = textD->lineStarts[visLineNum];
// TODO: 
// TODO:    /* If the line start was empty, return the last position in the buffer */
// TODO:    if (lineStart == -1)
// TODO:       return textD->buffer->length;
// TODO: 
// TODO:    /* Get the line text and its length */
// TODO:    lineLen = visLineLength(textD, visLineNum);
// TODO:    lineStr = BufGetRange(textD->buffer, lineStart, lineStart + lineLen);
// TODO: 
// TODO:    /* Step through character positions from the beginning of the line
// TODO:       to find the character position corresponding to the x coordinate */
// TODO:    xStep = textD->left - textD->horizOffset;
// TODO:    outIndex = 0;
// TODO:    for (charIndex=0; charIndex<lineLen; charIndex++)
// TODO:    {
// TODO:       charLen = BufExpandCharacter(lineStr[charIndex], outIndex, expandedChar,
// TODO:                                    textD->buffer->tabDist, textD->buffer->nullSubsChar);
// TODO:       charStyle = styleOfPos(textD, lineStart, lineLen, charIndex, outIndex,
// TODO:                              lineStr[charIndex]);
// TODO:       charWidth = stringWidth(textD, expandedChar, charLen, charStyle);
// TODO:       if (x < xStep + (posType == CURSOR_POS ? charWidth/2 : charWidth))
// TODO:       {
// TODO:          XtFree(lineStr);
// TODO:          return lineStart + charIndex;
// TODO:       }
// TODO:       xStep += charWidth;
// TODO:       outIndex += charLen;
// TODO:    }
// TODO: 
// TODO:    /* If the x position was beyond the end of the line, return the position
// TODO:       of the newline at the end of the line */
// TODO:    XtFree(lineStr);
// TODO:    return lineStart + lineLen;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Translate window coordinates to the nearest row and column number for
// TODO: ** positioning the cursor.  This, of course, makes no sense when the font is
// TODO: ** proportional, since there are no absolute columns.  The parameter posType
// TODO: ** specifies how to interpret the position: CURSOR_POS means translate the
// TODO: ** coordinates to the nearest position between characters, and CHARACTER_POS
// TODO: ** means translate the position to the nearest character cell.
// TODO: */
// TODO: static void xyToUnconstrainedPos(textDisp* textD, int x, int y, int* row,
// TODO:                                  int* column, int posType)
// TODO: {
// TODO:    int fontHeight = textD->ascent + textD->descent;
// TODO:    int fontWidth = textD->fontStruct->max_bounds.width;
// TODO: 
// TODO:    /* Find the visible line number corresponding to the y coordinate */
// TODO:    *row = (y - textD->top) / fontHeight;
// TODO:    if (*row < 0) *row = 0;
// TODO:    if (*row >= textD->nVisibleLines) *row = textD->nVisibleLines - 1;
// TODO:    *column = ((x-textD->left) + textD->horizOffset +
// TODO:               (posType == CURSOR_POS ? fontWidth/2 : 0)) / fontWidth;
// TODO:    if (*column < 0) *column = 0;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Offset the line starts array, topLineNum, firstChar and lastChar, for a new
// TODO: ** vertical scroll position given by newTopLineNum.  If any currently displayed
// TODO: ** lines will still be visible, salvage the line starts values, otherwise,
// TODO: ** count lines from the nearest known line start (start or end of buffer, or
// TODO: ** the closest value in the lineStarts array)
// TODO: */
// TODO: static void offsetLineStarts(textDisp* textD, int newTopLineNum)
// TODO: {
// TODO:    int oldTopLineNum = textD->topLineNum;
// TODO:    int oldFirstChar = textD->firstChar;
// TODO:    int lineDelta = newTopLineNum - oldTopLineNum;
// TODO:    int nVisLines = textD->nVisibleLines;
// TODO:    int* lineStarts = textD->lineStarts;
// TODO:    int i, lastLineNum;
// TODO:    textBuffer* buf = textD->buffer;
// TODO: 
// TODO:    /* If there was no offset, nothing needs to be changed */
// TODO:    if (lineDelta == 0)
// TODO:       return;
// TODO: 
// TODO:    /* {   int i;
// TODO:    	printf("Scroll, lineDelta %d\n", lineDelta);
// TODO:    	printf("lineStarts Before: ");
// TODO:    	for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
// TODO:    	printf("\n");
// TODO:    } */
// TODO: 
// TODO:    /* Find the new value for firstChar by counting lines from the nearest
// TODO:       known line start (start or end of buffer, or the closest value in the
// TODO:       lineStarts array) */
// TODO:    lastLineNum = oldTopLineNum + nVisLines - 1;
// TODO:    if (newTopLineNum < oldTopLineNum && newTopLineNum < -lineDelta)
// TODO:    {
// TODO:       textD->firstChar = TextDCountForwardNLines(textD, 0, newTopLineNum-1,
// TODO:                          true);
// TODO:       /* printf("counting forward %d lines from start\n", newTopLineNum-1);*/
// TODO:    }
// TODO:    else if (newTopLineNum < oldTopLineNum)
// TODO:    {
// TODO:       textD->firstChar = TextDCountBackwardNLines(textD, textD->firstChar,
// TODO:                          -lineDelta);
// TODO:       /* printf("counting backward %d lines from firstChar\n", -lineDelta);*/
// TODO:    }
// TODO:    else if (newTopLineNum < lastLineNum)
// TODO:    {
// TODO:       textD->firstChar = lineStarts[newTopLineNum - oldTopLineNum];
// TODO:       /* printf("taking new start from lineStarts[%d]\n",
// TODO:       	newTopLineNum - oldTopLineNum); */
// TODO:    }
// TODO:    else if (newTopLineNum-lastLineNum < textD->nBufferLines-newTopLineNum)
// TODO:    {
// TODO:       textD->firstChar = TextDCountForwardNLines(textD,
// TODO:                          lineStarts[nVisLines-1], newTopLineNum - lastLineNum, true);
// TODO:       /* printf("counting forward %d lines from start of last line\n",
// TODO:       	newTopLineNum - lastLineNum); */
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       textD->firstChar = TextDCountBackwardNLines(textD, buf->length,
// TODO:                          textD->nBufferLines - newTopLineNum + 1);
// TODO:       /* printf("counting backward %d lines from end\n",
// TODO:        		textD->nBufferLines - newTopLineNum + 1); */
// TODO:    }
// TODO: 
// TODO:    /* Fill in the line starts array */
// TODO:    if (lineDelta < 0 && -lineDelta < nVisLines)
// TODO:    {
// TODO:       for (i=nVisLines-1; i >= -lineDelta; i--)
// TODO:          lineStarts[i] = lineStarts[i+lineDelta];
// TODO:       calcLineStarts(textD, 0, -lineDelta);
// TODO:    }
// TODO:    else if (lineDelta > 0 && lineDelta < nVisLines)
// TODO:    {
// TODO:       for (i=0; i<nVisLines-lineDelta; i++)
// TODO:          lineStarts[i] = lineStarts[i+lineDelta];
// TODO:       calcLineStarts(textD, nVisLines-lineDelta, nVisLines-1);
// TODO:    }
// TODO:    else
// TODO:       calcLineStarts(textD, 0, nVisLines);
// TODO: 
// TODO:    /* Set lastChar and topLineNum */
// TODO:    calcLastChar(textD);
// TODO:    textD->topLineNum = newTopLineNum;
// TODO: 
// TODO:    /* If we're numbering lines or being asked to maintain an absolute line
// TODO:       number, re-calculate the absolute line number */
// TODO:    offsetAbsLineNum(textD, oldFirstChar);
// TODO: 
// TODO:    /* {   int i;
// TODO:    	printf("lineStarts After: ");
// TODO:    	for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
// TODO:    	printf("\n");
// TODO:    } */
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Update the line starts array, topLineNum, firstChar and lastChar for text
// TODO: ** display "textD" after a modification to the text buffer, given by the
// TODO: ** position where the change began "pos", and the nmubers of characters
// TODO: ** and lines inserted and deleted.
// TODO: */
// TODO: static void updateLineStarts(textDisp* textD, int pos, int charsInserted,
// TODO:                              int charsDeleted, int linesInserted, int linesDeleted, int* scrolled)
// TODO: {
// TODO:    int* lineStarts = textD->lineStarts;
// TODO:    int i, lineOfPos, lineOfEnd, nVisLines = textD->nVisibleLines;
// TODO:    int charDelta = charsInserted - charsDeleted;
// TODO:    int lineDelta = linesInserted - linesDeleted;
// TODO: 
// TODO:    /* {   int i;
// TODO:    	printf("linesDeleted %d, linesInserted %d, charsInserted %d, charsDeleted %d\n",
// TODO:    	    	linesDeleted, linesInserted, charsInserted, charsDeleted);
// TODO:    	printf("lineStarts Before: ");
// TODO:    	for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
// TODO:    	printf("\n");
// TODO:    } */
// TODO:    /* If all of the changes were before the displayed text, the display
// TODO:       doesn't change, just update the top line num and offset the line
// TODO:       start entries and first and last characters */
// TODO:    if (pos + charsDeleted < textD->firstChar)
// TODO:    {
// TODO:       textD->topLineNum += lineDelta;
// TODO:       for (i=0; i<nVisLines && lineStarts[i] != -1; i++)
// TODO:          lineStarts[i] += charDelta;
// TODO:       /* {   int i;
// TODO:           printf("lineStarts after delete doesn't touch: ");
// TODO:           for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
// TODO:           printf("\n");
// TODO:       } */
// TODO:       textD->firstChar += charDelta;
// TODO:       textD->lastChar += charDelta;
// TODO:       *scrolled = false;
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* The change began before the beginning of the displayed text, but
// TODO:       part or all of the displayed text was deleted */
// TODO:    if (pos < textD->firstChar)
// TODO:    {
// TODO:       /* If some text remains in the window, anchor on that  */
// TODO:       if (posToVisibleLineNum(textD, pos + charsDeleted, &lineOfEnd) &&
// TODO:             ++lineOfEnd < nVisLines && lineStarts[lineOfEnd] != -1)
// TODO:       {
// TODO:          textD->topLineNum = max(1, textD->topLineNum + lineDelta);
// TODO:          textD->firstChar = TextDCountBackwardNLines(textD,
// TODO:                             lineStarts[lineOfEnd] + charDelta, lineOfEnd);
// TODO:          /* Otherwise anchor on original line number and recount everything */
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          if (textD->topLineNum > textD->nBufferLines + lineDelta)
// TODO:          {
// TODO:             textD->topLineNum = 1;
// TODO:             textD->firstChar = 0;
// TODO:          }
// TODO:          else
// TODO:             textD->firstChar = TextDCountForwardNLines(textD, 0,
// TODO:                                textD->topLineNum - 1, true);
// TODO:       }
// TODO:       calcLineStarts(textD, 0, nVisLines-1);
// TODO:       /* {   int i;
// TODO:           printf("lineStarts after delete encroaches: ");
// TODO:           for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
// TODO:           printf("\n");
// TODO:       } */
// TODO:       /* calculate lastChar by finding the end of the last displayed line */
// TODO:       calcLastChar(textD);
// TODO:       *scrolled = true;
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* If the change was in the middle of the displayed text (it usually is),
// TODO:       salvage as much of the line starts array as possible by moving and
// TODO:       offsetting the entries after the changed area, and re-counting the
// TODO:       added lines or the lines beyond the salvaged part of the line starts
// TODO:       array */
// TODO:    if (pos <= textD->lastChar)
// TODO:    {
// TODO:       /* find line on which the change began */
// TODO:       posToVisibleLineNum(textD, pos, &lineOfPos);
// TODO:       /* salvage line starts after the changed area */
// TODO:       if (lineDelta == 0)
// TODO:       {
// TODO:          for (i=lineOfPos+1; i<nVisLines && lineStarts[i]!= -1; i++)
// TODO:             lineStarts[i] += charDelta;
// TODO:       }
// TODO:       else if (lineDelta > 0)
// TODO:       {
// TODO:          for (i=nVisLines-1; i>=lineOfPos+lineDelta+1; i--)
// TODO:             lineStarts[i] = lineStarts[i-lineDelta] +
// TODO:                             (lineStarts[i-lineDelta] == -1 ? 0 : charDelta);
// TODO:       }
// TODO:       else /* (lineDelta < 0) */
// TODO:       {
// TODO:          for (i=max(0,lineOfPos+1); i<nVisLines+lineDelta; i++)
// TODO:             lineStarts[i] = lineStarts[i-lineDelta] +
// TODO:                             (lineStarts[i-lineDelta] == -1 ? 0 : charDelta);
// TODO:       }
// TODO:       /* {   int i;
// TODO:           printf("lineStarts after salvage: ");
// TODO:           for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
// TODO:           printf("\n");
// TODO:       } */
// TODO:       /* fill in the missing line starts */
// TODO:       if (linesInserted >= 0)
// TODO:          calcLineStarts(textD, lineOfPos + 1, lineOfPos + linesInserted);
// TODO:       if (lineDelta < 0)
// TODO:          calcLineStarts(textD, nVisLines+lineDelta, nVisLines);
// TODO:       /* {   int i;
// TODO:           printf("lineStarts after recalculation: ");
// TODO:           for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
// TODO:           printf("\n");
// TODO:       } */
// TODO:       /* calculate lastChar by finding the end of the last displayed line */
// TODO:       calcLastChar(textD);
// TODO:       *scrolled = false;
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Change was past the end of the displayed text, but displayable by virtue
// TODO:       of being an insert at the end of the buffer into visible blank lines */
// TODO:    if (emptyLinesVisible(textD))
// TODO:    {
// TODO:       posToVisibleLineNum(textD, pos, &lineOfPos);
// TODO:       calcLineStarts(textD, lineOfPos, lineOfPos+linesInserted);
// TODO:       calcLastChar(textD);
// TODO:       /* {   int i;
// TODO:           printf("lineStarts after insert at end: ");
// TODO:           for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
// TODO:           printf("\n");
// TODO:       } */
// TODO:       *scrolled = false;
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Change was beyond the end of the buffer and not visible, do nothing */
// TODO:    *scrolled = false;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Scan through the text in the "textD"'s buffer and recalculate the line
// TODO: ** starts array values beginning at index "startLine" and continuing through
// TODO: ** (including) "endLine".  It assumes that the line starts entry preceding
// TODO: ** "startLine" (or textD->firstChar if startLine is 0) is good, and re-counts
// TODO: ** newlines to fill in the requested entries.  Out of range values for
// TODO: ** "startLine" and "endLine" are acceptable.
// TODO: */
// TODO: static void calcLineStarts(textDisp* textD, int startLine, int endLine)
// TODO: {
// TODO:    int startPos, bufLen = textD->buffer->length;
// TODO:    int line, lineEnd, nextLineStart, nVis = textD->nVisibleLines;
// TODO:    int* lineStarts = textD->lineStarts;
// TODO: 
// TODO:    /* Clean up (possibly) messy input parameters */
// TODO:    if (nVis == 0) return;
// TODO:    if (endLine < 0) endLine = 0;
// TODO:    if (endLine >= nVis) endLine = nVis - 1;
// TODO:    if (startLine < 0) startLine = 0;
// TODO:    if (startLine >=nVis) startLine = nVis - 1;
// TODO:    if (startLine > endLine)
// TODO:       return;
// TODO: 
// TODO:    /* Find the last known good line number -> position mapping */
// TODO:    if (startLine == 0)
// TODO:    {
// TODO:       lineStarts[0] = textD->firstChar;
// TODO:       startLine = 1;
// TODO:    }
// TODO:    startPos = lineStarts[startLine-1];
// TODO: 
// TODO:    /* If the starting position is already past the end of the text,
// TODO:       fill in -1's (means no text on line) and return */
// TODO:    if (startPos == -1)
// TODO:    {
// TODO:       for (line=startLine; line<=endLine; line++)
// TODO:          lineStarts[line] = -1;
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Loop searching for ends of lines and storing the positions of the
// TODO:       start of the next line in lineStarts */
// TODO:    for (line=startLine; line<=endLine; line++)
// TODO:    {
// TODO:       findLineEnd(textD, startPos, true, &lineEnd, &nextLineStart);
// TODO:       startPos = nextLineStart;
// TODO:       if (startPos >= bufLen)
// TODO:       {
// TODO:          /* If the buffer ends with a newline or line break, put
// TODO:             buf->length in the next line start position (instead of
// TODO:             a -1 which is the normal marker for an empty line) to
// TODO:             indicate that the cursor may safely be displayed there */
// TODO:          if (line == 0 || (lineStarts[line-1] != bufLen &&
// TODO:                            lineEnd != nextLineStart))
// TODO:          {
// TODO:             lineStarts[line] = bufLen;
// TODO:             line++;
// TODO:          }
// TODO:          break;
// TODO:       }
// TODO:       lineStarts[line] = startPos;
// TODO:    }
// TODO: 
// TODO:    /* Set any entries beyond the end of the text to -1 */
// TODO:    for (; line<=endLine; line++)
// TODO:       lineStarts[line] = -1;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Given a textDisp with a complete, up-to-date lineStarts array, update
// TODO: ** the lastChar entry to point to the last buffer position displayed.
// TODO: */
// TODO: static void calcLastChar(textDisp* textD)
// TODO: {
// TODO:    int i;
// TODO: 
// TODO:    for (i=textD->nVisibleLines-1; i>0 && textD->lineStarts[i]== -1; i--);
// TODO:    textD->lastChar = i < 0 ? 0 :
// TODO:                      TextDEndOfLine(textD, textD->lineStarts[i], true);
// TODO: }
// TODO: 
// TODO: void TextDImposeGraphicsExposeTranslation(textDisp* textD, int* xOffset, int* yOffset)
// TODO: {
// TODO:    if (textD->graphicsExposeQueue)
// TODO:    {
// TODO:       graphicExposeTranslationEntry* thisGEQEntry = textD->graphicsExposeQueue->next;
// TODO:       if (thisGEQEntry)
// TODO:       {
// TODO:          *xOffset += thisGEQEntry->horizontal;
// TODO:          *yOffset += thisGEQEntry->vertical;
// TODO:       }
// TODO:    }
// TODO: }
// TODO: 
// TODO: bool TextDPopGraphicExposeQueueEntry(textDisp* textD)
// TODO: {
// TODO:    graphicExposeTranslationEntry* removedGEQEntry = textD->graphicsExposeQueue;
// TODO: 
// TODO:    if (removedGEQEntry)
// TODO:    {
// TODO:       textD->graphicsExposeQueue = removedGEQEntry->next;
// TODO:       XtFree((char*)removedGEQEntry);
// TODO:    }
// TODO:    return(removedGEQEntry?true:false);
// TODO: }
// TODO: 
// TODO: void TextDTranlateGraphicExposeQueue(textDisp* textD, int xOffset, int yOffset, bool appendEntry)
// TODO: {
// TODO:    graphicExposeTranslationEntry* newGEQEntry = NULL;
// TODO:    if (appendEntry)
// TODO:    {
// TODO:       newGEQEntry = (graphicExposeTranslationEntry*)malloc__(sizeof(graphicExposeTranslationEntry));
// TODO:       newGEQEntry->next = NULL;
// TODO:       newGEQEntry->horizontal = xOffset;
// TODO:       newGEQEntry->vertical = yOffset;
// TODO:    }
// TODO:    if (textD->graphicsExposeQueue)
// TODO:    {
// TODO:       graphicExposeTranslationEntry* iter = textD->graphicsExposeQueue;
// TODO:       while (iter->next)
// TODO:       {
// TODO:          iter->next->horizontal += xOffset;
// TODO:          iter->next->vertical += yOffset;
// TODO:          iter = iter->next;
// TODO:       }
// TODO:       if (appendEntry)
// TODO:       {
// TODO:          iter->next = (struct graphicExposeTranslationEntry*)newGEQEntry;
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if (appendEntry)
// TODO:       {
// TODO:          textD->graphicsExposeQueue = newGEQEntry;
// TODO:       }
// TODO:    }
// TODO: }
// TODO: 
// TODO: static void setScroll(textDisp* textD, int topLineNum, int horizOffset,
// TODO:                       int updateVScrollBar, int updateHScrollBar)
// TODO: {
// TODO:    int fontHeight = textD->ascent + textD->descent;
// TODO:    int origHOffset = textD->horizOffset;
// TODO:    int lineDelta = textD->topLineNum - topLineNum;
// TODO:    int xOffset, yOffset, srcX, srcY, dstX, dstY, width, height;
// TODO:    int exactHeight = textD->height - textD->height %
// TODO:                      (textD->ascent + textD->descent);
// TODO: 
// TODO:    /* Do nothing if scroll position hasn't actually changed or there's no
// TODO:       window to draw in yet */
// TODO:    if (XtWindow(textD->w) == 0 || (textD->horizOffset == horizOffset &&
// TODO:                                    textD->topLineNum == topLineNum))
// TODO:       return;
// TODO: 
// TODO:    /* If part of the cursor is protruding beyond the text clipping region,
// TODO:       clear it off */
// TODO:    blankCursorProtrusions(textD);
// TODO: 
// TODO:    /* If the vertical scroll position has changed, update the line
// TODO:       starts array and related counters in the text display */
// TODO:    offsetLineStarts(textD, topLineNum);
// TODO: 
// TODO:    /* Just setting textD->horizOffset is enough information for redisplay */
// TODO:    textD->horizOffset = horizOffset;
// TODO: 
// TODO:    /* Update the scroll bar positions if requested, note: updating the
// TODO:       horizontal scroll bars can have the further side-effect of changing
// TODO:       the horizontal scroll position, textD->horizOffset */
// TODO:    if (updateVScrollBar && textD->vScrollBar != NULL)
// TODO:    {
// TODO:       updateVScrollBarRange(textD);
// TODO:    }
// TODO:    if (updateHScrollBar && textD->hScrollBar != NULL)
// TODO:    {
// TODO:       updateHScrollBarRange(textD);
// TODO:    }
// TODO: 
// TODO:    /* Redisplay everything if the window is partially obscured (since
// TODO:       it's too hard to tell what displayed areas are salvageable) or
// TODO:       if there's nothing to recover because the scroll distance is large */
// TODO:    xOffset = origHOffset - textD->horizOffset;
// TODO:    yOffset = lineDelta * fontHeight;
// TODO:    if (textD->visibility != VisibilityUnobscured ||
// TODO:          abs(xOffset) > textD->width || abs(yOffset) > exactHeight)
// TODO:    {
// TODO:       TextDTranlateGraphicExposeQueue(textD, xOffset, yOffset, false);
// TODO:       TextDRedisplayRect(textD, textD->left, textD->top, textD->width,
// TODO:                          textD->height);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       /* If the window is not obscured, paint most of the window using XCopyArea
// TODO:          from existing displayed text, and redraw only what's necessary */
// TODO:       /* Recover the useable window areas by moving to the proper location */
// TODO:       srcX = textD->left + (xOffset >= 0 ? 0 : -xOffset);
// TODO:       dstX = textD->left + (xOffset >= 0 ? xOffset : 0);
// TODO:       width = textD->width - abs(xOffset);
// TODO:       srcY = textD->top + (yOffset >= 0 ? 0 : -yOffset);
// TODO:       dstY = textD->top + (yOffset >= 0 ? yOffset : 0);
// TODO:       height = exactHeight - abs(yOffset);
// TODO:       resetClipRectangles(textD);
// TODO:       TextDTranlateGraphicExposeQueue(textD, xOffset, yOffset, true);
// TODO:       XCopyArea(XtDisplay(textD->w), XtWindow(textD->w), XtWindow(textD->w),
// TODO:                 textD->gc, srcX, srcY, width, height, dstX, dstY);
// TODO:       /* redraw the un-recoverable parts */
// TODO:       if (yOffset > 0)
// TODO:       {
// TODO:          TextDRedisplayRect(textD, textD->left, textD->top,
// TODO:                             textD->width, yOffset);
// TODO:       }
// TODO:       else if (yOffset < 0)
// TODO:       {
// TODO:          TextDRedisplayRect(textD, textD->left, textD->top +
// TODO:                             textD->height + yOffset, textD->width, -yOffset);
// TODO:       }
// TODO:       if (xOffset > 0)
// TODO:       {
// TODO:          TextDRedisplayRect(textD, textD->left, textD->top,
// TODO:                             xOffset, textD->height);
// TODO:       }
// TODO:       else if (xOffset < 0)
// TODO:       {
// TODO:          TextDRedisplayRect(textD, textD->left + textD->width + xOffset,
// TODO:                             textD->top, -xOffset, textD->height);
// TODO:       }
// TODO:       /* Restore protruding parts of the cursor */
// TODO:       textDRedisplayRange(textD, textD->cursorPos-1, textD->cursorPos+1);
// TODO:    }
// TODO: 
// TODO:    /* Refresh line number/calltip display if its up and we've scrolled
// TODO:        vertically */
// TODO:    if (lineDelta != 0)
// TODO:    {
// TODO:       redrawLineNumbers(textD, false);
// TODO:       TextDRedrawCalltip(textD, 0);
// TODO:    }
// TODO: 
// TODO:    HandleAllPendingGraphicsExposeNoExposeEvents((TextWidget)textD->w, NULL);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Update the minimum, maximum, slider size, page increment, and value
// TODO: ** for vertical scroll bar.
// TODO: */
// TODO: static void updateVScrollBarRange(textDisp* textD)
// TODO: {
// TODO:    int sliderSize, sliderMax, sliderValue;
// TODO: 
// TODO:    if (textD->vScrollBar == NULL)
// TODO:       return;
// TODO: 
// TODO:    /* The Vert. scroll bar value and slider size directly represent the top
// TODO:       line number, and the number of visible lines respectively.  The scroll
// TODO:       bar maximum value is chosen to generally represent the size of the whole
// TODO:       buffer, with minor adjustments to keep the scroll bar widget happy */
// TODO:    sliderSize = max(textD->nVisibleLines, 1); /* Avoid X warning (size < 1) */
// TODO:    sliderValue = textD->topLineNum;
// TODO:    sliderMax = max(textD->nBufferLines + 2 +
// TODO:                    TEXT_OF_TEXTD(textD).cursorVPadding,
// TODO:                    sliderSize + sliderValue);
// TODO:    XtVaSetValues(textD->vScrollBar,
// TODO:                  XmNmaximum, sliderMax,
// TODO:                  XmNsliderSize, sliderSize,
// TODO:                  XmNpageIncrement, max(1, textD->nVisibleLines - 1),
// TODO:                  XmNvalue, sliderValue, NULL);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Update the minimum, maximum, slider size, page increment, and value
// TODO: ** for the horizontal scroll bar.  If scroll position is such that there
// TODO: ** is blank space to the right of all lines of text, scroll back (adjust
// TODO: ** horizOffset but don't redraw) to take up the slack and position the
// TODO: ** right edge of the text at the right edge of the display.
// TODO: **
// TODO: ** Note, there is some cost to this routine, since it scans the whole range
// TODO: ** of displayed text, particularly since it's usually called for each typed
// TODO: ** character!
// TODO: */
// TODO: static int updateHScrollBarRange(textDisp* textD)
// TODO: {
// TODO:    int i, maxWidth = 0, sliderMax, sliderWidth;
// TODO:    int origHOffset = textD->horizOffset;
// TODO: 
// TODO:    if (textD->hScrollBar == NULL || !XtIsManaged(textD->hScrollBar))
// TODO:       return false;
// TODO: 
// TODO:    /* Scan all the displayed lines to find the width of the longest line */
// TODO:    for (i=0; i<textD->nVisibleLines && textD->lineStarts[i]!= -1; i++)
// TODO:       maxWidth = max(measureVisLine(textD, i), maxWidth);
// TODO: 
// TODO:    /* If the scroll position is beyond what's necessary to keep all lines
// TODO:       in view, scroll to the left to bring the end of the longest line to
// TODO:       the right margin */
// TODO:    if (maxWidth < textD->width + textD->horizOffset && textD->horizOffset > 0)
// TODO:       textD->horizOffset = max(0, maxWidth - textD->width);
// TODO: 
// TODO:    /* Readjust the scroll bar */
// TODO:    sliderWidth = textD->width;
// TODO:    sliderMax = max(maxWidth, sliderWidth + textD->horizOffset);
// TODO:    XtVaSetValues(textD->hScrollBar,
// TODO:                  XmNmaximum, sliderMax,
// TODO:                  XmNsliderSize, sliderWidth,
// TODO:                  XmNpageIncrement, max(textD->width - 100, 10),
// TODO:                  XmNvalue, textD->horizOffset, NULL);
// TODO: 
// TODO:    /* Return true if scroll position was changed */
// TODO:    return origHOffset != textD->horizOffset;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Define area for drawing line numbers.  A width of 0 disables line
// TODO: ** number drawing.
// TODO: */
// TODO: void TextDSetLineNumberArea(textDisp* textD, int lineNumLeft, int lineNumWidth,
// TODO:                             int textLeft)
// TODO: {
// TODO:    int newWidth = textD->width + textD->left - textLeft;
// TODO:    textD->lineNumLeft = lineNumLeft;
// TODO:    textD->lineNumWidth = lineNumWidth;
// TODO:    textD->left = textLeft;
// TODO:    XClearWindow(XtDisplay(textD->w), XtWindow(textD->w));
// TODO:    resetAbsLineNum(textD);
// TODO:    TextDResize(textD, newWidth, textD->height);
// TODO:    TextDRedisplayRect(textD, 0, textD->top, INT_MAX, textD->height);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Refresh the line number area.  If clearAll is false, writes only over
// TODO: ** the character cell areas.  Setting clearAll to true will clear out any
// TODO: ** stray marks outside of the character cell area, which might have been
// TODO: ** left from before a resize or font change.
// TODO: */
// TODO: static void redrawLineNumbers(textDisp* textD, int clearAll)
// TODO: {
// TODO:    int y, line, visLine, nCols, lineStart;
// TODO:    char lineNumString[12];
// TODO:    int lineHeight = textD->ascent + textD->descent;
// TODO:    int charWidth = textD->fontStruct->max_bounds.width;
// TODO:    XRectangle clipRect;
// TODO:    Display* display = XtDisplay(textD->w);
// TODO: 
// TODO:    /* Don't draw if lineNumWidth == 0 (line numbers are hidden), or widget is
// TODO:       not yet realized */
// TODO:    if (textD->lineNumWidth == 0 || XtWindow(textD->w) == 0)
// TODO:       return;
// TODO: 
// TODO:    /* Make sure we reset the clipping range for the line numbers GC, because
// TODO:       the GC may be shared (eg, if the line numbers and text have the same
// TODO:       color) and therefore the clipping ranges may be invalid. */
// TODO:    clipRect.x = textD->lineNumLeft;
// TODO:    clipRect.y = textD->top;
// TODO:    clipRect.width = textD->lineNumWidth;
// TODO:    clipRect.height = textD->height;
// TODO:    XSetClipRectangles(display, textD->lineNumGC, 0, 0,
// TODO:                       &clipRect, 1, Unsorted);
// TODO: 
// TODO:    /* Erase the previous contents of the line number area, if requested */
// TODO:    if (clearAll)
// TODO:       XClearArea(XtDisplay(textD->w), XtWindow(textD->w), textD->lineNumLeft,
// TODO:                  textD->top, textD->lineNumWidth, textD->height, false);
// TODO: 
// TODO:    /* Draw the line numbers, aligned to the text */
// TODO:    nCols = min(11, textD->lineNumWidth / charWidth);
// TODO:    y = textD->top;
// TODO:    line = getAbsTopLineNum(textD);
// TODO:    for (visLine=0; visLine < textD->nVisibleLines; visLine++)
// TODO:    {
// TODO:       lineStart = textD->lineStarts[visLine];
// TODO:       if (lineStart != -1 && (lineStart==0 ||
// TODO:                               BufGetCharacter(textD->buffer, lineStart-1)=='\n'))
// TODO:       {
// TODO:          sprintf(lineNumString, "%*d", nCols, line);
// TODO:          XDrawImageString(XtDisplay(textD->w), XtWindow(textD->w),
// TODO:                           textD->lineNumGC, textD->lineNumLeft, y + textD->ascent,
// TODO:                           lineNumString, strlen(lineNumString));
// TODO:          line++;
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          XClearArea(XtDisplay(textD->w), XtWindow(textD->w),
// TODO:                     textD->lineNumLeft, y, textD->lineNumWidth,
// TODO:                     textD->ascent + textD->descent, false);
// TODO:          if (visLine == 0)
// TODO:             line++;
// TODO:       }
// TODO:       y += lineHeight;
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Callbacks for drag or valueChanged on scroll bars
// TODO: */
// TODO: static void vScrollCB(Widget w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    textDisp* textD = (textDisp*)clientData;
// TODO:    int newValue = ((XmScrollBarCallbackStruct*)callData)->value;
// TODO:    int lineDelta = newValue - textD->topLineNum;
// TODO: 
// TODO:    if (lineDelta == 0)
// TODO:       return;
// TODO:    setScroll(textD, newValue, textD->horizOffset, false, true);
// TODO: }
// TODO: static void hScrollCB(Widget w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    textDisp* textD = (textDisp*)clientData;
// TODO:    int newValue = ((XmScrollBarCallbackStruct*)callData)->value;
// TODO: 
// TODO:    if (newValue == textD->horizOffset)
// TODO:       return;
// TODO:    setScroll(textD, textD->topLineNum, newValue, false, false);
// TODO: }
// TODO: 
// TODO: static void visibilityEH(Widget w, XtPointer data, XEvent* event,
// TODO:                          bool* continueDispatch)
// TODO: {
// TODO:    /* Record whether the window is fully visible or not.  This information
// TODO:       is used for choosing the scrolling methodology for optimal performance,
// TODO:       if the window is partially obscured, XCopyArea may not work */
// TODO:    ((textDisp*)data)->visibility = ((XVisibilityEvent*)event)->state;
// TODO: }
// TODO: 
// TODO: static int max(int i1, int i2)
// TODO: {
// TODO:    return i1 >= i2 ? i1 : i2;
// TODO: }
// TODO: 
// TODO: static int min(int i1, int i2)
// TODO: {
// TODO:    return i1 <= i2 ? i1 : i2;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Count the number of newlines in a null-terminated text string;
// TODO: */
// TODO: static int countLines(const char* string)
// TODO: {
// TODO:    const char* c;
// TODO:    int lineCount = 0;
// TODO: 
// TODO:    if (string == NULL)
// TODO:       return 0;
// TODO:    for (c=string; *c!='\0'; c++)
// TODO:       if (*c == '\n') lineCount++;
// TODO:    return lineCount;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Return the width in pixels of the displayed line pointed to by "visLineNum"
// TODO: */
// TODO: static int measureVisLine(textDisp* textD, int visLineNum)
// TODO: {
// TODO:    int i, width = 0, len, style, lineLen = visLineLength(textD, visLineNum);
// TODO:    int charCount = 0, lineStartPos = textD->lineStarts[visLineNum];
// TODO:    char expandedChar[MAX_EXP_CHAR_LEN];
// TODO: 
// TODO:    if (textD->styleBuffer == NULL)
// TODO:    {
// TODO:       for (i=0; i<lineLen; i++)
// TODO:       {
// TODO:          len = BufGetExpandedChar(textD->buffer, lineStartPos + i,
// TODO:                                   charCount, expandedChar);
// TODO:          width += XTextWidth(textD->fontStruct, expandedChar, len);
// TODO:          charCount += len;
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       for (i=0; i<lineLen; i++)
// TODO:       {
// TODO:          len = BufGetExpandedChar(textD->buffer, lineStartPos+i,
// TODO:                                   charCount, expandedChar);
// TODO:          style = (unsigned char)BufGetCharacter(textD->styleBuffer,
// TODO:                                                 lineStartPos+i) - ASCII_A;
// TODO:          width += XTextWidth(textD->styleTable[style].font, expandedChar,
// TODO:                              len);
// TODO:          charCount += len;
// TODO:       }
// TODO:    }
// TODO:    return width;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Return true if there are lines visible with no corresponding buffer text
// TODO: */
// TODO: static int emptyLinesVisible(textDisp* textD)
// TODO: {
// TODO:    return textD->nVisibleLines > 0 &&
// TODO:           textD->lineStarts[textD->nVisibleLines-1] == -1;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** When the cursor is at the left or right edge of the text, part of it
// TODO: ** sticks off into the clipped region beyond the text.  Normal redrawing
// TODO: ** can not overwrite this protruding part of the cursor, so it must be
// TODO: ** erased independently by calling this routine.
// TODO: */
// TODO: static void blankCursorProtrusions(textDisp* textD)
// TODO: {
// TODO:    int x, width, cursorX = textD->cursorX, cursorY = textD->cursorY;
// TODO:    int fontWidth = textD->fontStruct->max_bounds.width;
// TODO:    int fontHeight = textD->ascent + textD->descent;
// TODO:    int cursorWidth, left = textD->left, right = left + textD->width;
// TODO: 
// TODO:    cursorWidth = (fontWidth/3) * 2;
// TODO:    if (cursorX >= left-1 && cursorX <= left + cursorWidth/2 - 1)
// TODO:    {
// TODO:       x = cursorX - cursorWidth/2;
// TODO:       width = left - x;
// TODO:    }
// TODO:    else if (cursorX >= right - cursorWidth/2 && cursorX <= right)
// TODO:    {
// TODO:       x = right;
// TODO:       width = cursorX + cursorWidth/2 + 2 - right;
// TODO:    }
// TODO:    else
// TODO:       return;
// TODO: 
// TODO:    XClearArea(XtDisplay(textD->w), XtWindow(textD->w), x, cursorY,
// TODO:               width, fontHeight, false);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Allocate shared graphics contexts used by the widget, which must be
// TODO: ** re-allocated on a font change.
// TODO: */
// TODO: static void allocateFixedFontGCs(textDisp* textD, XFontStruct* fontStruct,
// TODO:                                  Pixel bgPixel, Pixel fgPixel, Pixel selectFGPixel, Pixel selectBGPixel,
// TODO:                                  Pixel highlightFGPixel, Pixel highlightBGPixel, Pixel lineNumFGPixel)
// TODO: {
// TODO:    textD->gc = allocateGC(textD->w, GCFont | GCForeground | GCBackground,
// TODO:                           fgPixel, bgPixel, fontStruct->fid, GCClipMask, GCArcMode);
// TODO:    textD->selectGC = allocateGC(textD->w, GCFont | GCForeground | GCBackground,
// TODO:                                 selectFGPixel, selectBGPixel, fontStruct->fid, GCClipMask,
// TODO:                                 GCArcMode);
// TODO:    textD->selectBGGC = allocateGC(textD->w, GCForeground, selectBGPixel, 0,
// TODO:                                   fontStruct->fid, GCClipMask, GCArcMode);
// TODO:    textD->highlightGC = allocateGC(textD->w, GCFont|GCForeground|GCBackground,
// TODO:                                    highlightFGPixel, highlightBGPixel, fontStruct->fid, GCClipMask,
// TODO:                                    GCArcMode);
// TODO:    textD->highlightBGGC = allocateGC(textD->w, GCForeground, highlightBGPixel,
// TODO:                                      0, fontStruct->fid, GCClipMask, GCArcMode);
// TODO:    textD->lineNumGC = allocateGC(textD->w, GCFont | GCForeground |
// TODO:                                  GCBackground, lineNumFGPixel, bgPixel, fontStruct->fid,
// TODO:                                  GCClipMask, GCArcMode);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** X11R4 does not have the XtAllocateGC function for sharing graphics contexts
// TODO: ** with changeable fields.  Unfortunately the R4 call for creating shared
// TODO: ** graphics contexts (XtGetGC) is rarely useful because most widgets need
// TODO: ** to be able to set and change clipping, and that makes the GC unshareable.
// TODO: **
// TODO: ** This function allocates and returns a gc, using XtAllocateGC if possible,
// TODO: ** or XCreateGC on X11R4 systems where XtAllocateGC is not available.
// TODO: */
// TODO: static GC allocateGC(Widget w, unsigned long valueMask,
// TODO:                      unsigned long foreground, unsigned long background, Font font,
// TODO:                      unsigned long dynamicMask, unsigned long dontCareMask)
// TODO: {
// TODO:    XGCValues gcValues;
// TODO: 
// TODO:    gcValues.font = font;
// TODO:    gcValues.background = background;
// TODO:    gcValues.foreground = foreground;
// TODO: #if defined(XlibSpecificationRelease) && XlibSpecificationRelease > 4
// TODO:    return XtAllocateGC(w, 0, valueMask, &gcValues, dynamicMask,
// TODO:                        dontCareMask);
// TODO: #else
// TODO:    return XCreateGC(XtDisplay(w), RootWindowOfScreen(XtScreen(w)),
// TODO:                     valueMask, &gcValues);
// TODO: #endif
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Release a gc allocated with allocateGC above
// TODO: */
// TODO: static void releaseGC(Widget w, GC gc)
// TODO: {
// TODO: #if defined(XlibSpecificationRelease) && XlibSpecificationRelease > 4
// TODO:    XtReleaseGC(w, gc);
// TODO: #else
// TODO:    XFreeGC(XtDisplay(w), gc);
// TODO: #endif
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** resetClipRectangles sets the clipping rectangles for GCs which clip
// TODO: ** at the text boundary (as opposed to the window boundary).  These GCs
// TODO: ** are shared such that the drawing styles are constant, but the clipping
// TODO: ** rectangles are allowed to change among different users of the GCs (the
// TODO: ** GCs were created with XtAllocGC).  This routine resets them so the clipping
// TODO: ** rectangles are correct for this text display.
// TODO: */
// TODO: static void resetClipRectangles(textDisp* textD)
// TODO: {
// TODO:    XRectangle clipRect;
// TODO:    Display* display = XtDisplay(textD->w);
// TODO: 
// TODO:    clipRect.x = textD->left;
// TODO:    clipRect.y = textD->top;
// TODO:    clipRect.width = textD->width;
// TODO:    clipRect.height = textD->height - textD->height %
// TODO:                      (textD->ascent + textD->descent);
// TODO: 
// TODO:    XSetClipRectangles(display, textD->gc, 0, 0,
// TODO:                       &clipRect, 1, Unsorted);
// TODO:    XSetClipRectangles(display, textD->selectGC, 0, 0,
// TODO:                       &clipRect, 1, Unsorted);
// TODO:    XSetClipRectangles(display, textD->highlightGC, 0, 0,
// TODO:                       &clipRect, 1, Unsorted);
// TODO:    XSetClipRectangles(display, textD->selectBGGC, 0, 0,
// TODO:                       &clipRect, 1, Unsorted);
// TODO:    XSetClipRectangles(display, textD->highlightBGGC, 0, 0,
// TODO:                       &clipRect, 1, Unsorted);
// TODO:    XSetClipRectangles(display, textD->styleGC, 0, 0,
// TODO:                       &clipRect, 1, Unsorted);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Return the length of a line (number of displayable characters) by examining
// TODO: ** entries in the line starts array rather than by scanning for newlines
// TODO: */
// TODO: static int visLineLength(textDisp* textD, int visLineNum)
// TODO: {
// TODO:    int nextLineStart, lineStartPos = textD->lineStarts[visLineNum];
// TODO: 
// TODO:    if (lineStartPos == -1)
// TODO:       return 0;
// TODO:    if (visLineNum+1 >= textD->nVisibleLines)
// TODO:       return textD->lastChar - lineStartPos;
// TODO:    nextLineStart = textD->lineStarts[visLineNum+1];
// TODO:    if (nextLineStart == -1)
// TODO:       return textD->lastChar - lineStartPos;
// TODO:    if (wrapUsesCharacter(textD, nextLineStart-1))
// TODO:       return nextLineStart-1 - lineStartPos;
// TODO:    return nextLineStart - lineStartPos;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** When continuous wrap is on, and the user inserts or deletes characters,
// TODO: ** wrapping can happen before and beyond the changed position.  This routine
// TODO: ** finds the extent of the changes, and counts the deleted and inserted lines
// TODO: ** over that range.  It also attempts to minimize the size of the range to
// TODO: ** what has to be counted and re-displayed, so the results can be useful
// TODO: ** both for delimiting where the line starts need to be recalculated, and
// TODO: ** for deciding what part of the text to redisplay.
// TODO: */
// TODO: static void findWrapRange(textDisp* textD, const char* deletedText, int pos,
// TODO:                           int nInserted, int nDeleted, int* modRangeStart, int* modRangeEnd,
// TODO:                           int* linesInserted, int* linesDeleted)
// TODO: {
// TODO:    int length, retPos, retLines, retLineStart, retLineEnd;
// TODO:    textBuffer* deletedTextBuf, *buf = textD->buffer;
// TODO:    int nVisLines = textD->nVisibleLines;
// TODO:    int* lineStarts = textD->lineStarts;
// TODO:    int countFrom, countTo, lineStart, adjLineStart, i;
// TODO:    int visLineNum = 0, nLines = 0;
// TODO: 
// TODO:    /*
// TODO:    ** Determine where to begin searching: either the previous newline, or
// TODO:    ** if possible, limit to the start of the (original) previous displayed
// TODO:    ** line, using information from the existing line starts array
// TODO:    */
// TODO:    if (pos >= textD->firstChar && pos <= textD->lastChar)
// TODO:    {
// TODO:       for (i=nVisLines-1; i>0; i--)
// TODO:          if (lineStarts[i] != -1 && pos >= lineStarts[i])
// TODO:             break;
// TODO:       if (i > 0)
// TODO:       {
// TODO:          countFrom = lineStarts[i-1];
// TODO:          visLineNum = i-1;
// TODO:       }
// TODO:       else
// TODO:          countFrom = BufStartOfLine(buf, pos);
// TODO:    }
// TODO:    else
// TODO:       countFrom = BufStartOfLine(buf, pos);
// TODO: 
// TODO: 
// TODO:    /*
// TODO:    ** Move forward through the (new) text one line at a time, counting
// TODO:    ** displayed lines, and looking for either a real newline, or for the
// TODO:    ** line starts to re-sync with the original line starts array
// TODO:    */
// TODO:    lineStart = countFrom;
// TODO:    *modRangeStart = countFrom;
// TODO:    while (true)
// TODO:    {
// TODO: 
// TODO:       /* advance to the next line.  If the line ended in a real newline
// TODO:          or the end of the buffer, that's far enough */
// TODO:       wrappedLineCounter(textD, buf, lineStart, buf->length, 1, true, 0,
// TODO:                          &retPos, &retLines, &retLineStart, &retLineEnd);
// TODO:       if (retPos >= buf->length)
// TODO:       {
// TODO:          countTo = buf->length;
// TODO:          *modRangeEnd = countTo;
// TODO:          if (retPos != retLineEnd)
// TODO:             nLines++;
// TODO:          break;
// TODO:       }
// TODO:       else
// TODO:          lineStart = retPos;
// TODO:       nLines++;
// TODO:       if (lineStart > pos + nInserted &&
// TODO:             BufGetCharacter(buf, lineStart-1) == '\n')
// TODO:       {
// TODO:          countTo = lineStart;
// TODO:          *modRangeEnd = lineStart;
// TODO:          break;
// TODO:       }
// TODO: 
// TODO:       /* Don't try to resync in continuous wrap mode with non-fixed font
// TODO:          sizes; it would result in a chicken-and-egg dependency between
// TODO:          the calculations for the inserted and the deleted lines.
// TODO:               If we're in that mode, the number of deleted lines is calculated in
// TODO:               advance, without resynchronization, so we shouldn't resynchronize
// TODO:               for the inserted lines either. */
// TODO:       if (textD->suppressResync)
// TODO:          continue;
// TODO: 
// TODO:       /* check for synchronization with the original line starts array
// TODO:          before pos, if so, the modified range can begin later */
// TODO:       if (lineStart <= pos)
// TODO:       {
// TODO:          while (visLineNum<nVisLines && lineStarts[visLineNum] < lineStart)
// TODO:             visLineNum++;
// TODO:          if (visLineNum < nVisLines && lineStarts[visLineNum] == lineStart)
// TODO:          {
// TODO:             countFrom = lineStart;
// TODO:             nLines = 0;
// TODO:             if (visLineNum+1 < nVisLines && lineStarts[visLineNum+1] != -1)
// TODO:                *modRangeStart = min(pos, lineStarts[visLineNum+1]-1);
// TODO:             else
// TODO:                *modRangeStart = countFrom;
// TODO:          }
// TODO:          else
// TODO:             *modRangeStart = min(*modRangeStart, lineStart-1);
// TODO:       }
// TODO: 
// TODO:       /* check for synchronization with the original line starts array
// TODO:          after pos, if so, the modified range can end early */
// TODO:       else if (lineStart > pos + nInserted)
// TODO:       {
// TODO:          adjLineStart = lineStart - nInserted + nDeleted;
// TODO:          while (visLineNum<nVisLines && lineStarts[visLineNum]<adjLineStart)
// TODO:             visLineNum++;
// TODO:          if (visLineNum < nVisLines && lineStarts[visLineNum] != -1 &&
// TODO:                lineStarts[visLineNum] == adjLineStart)
// TODO:          {
// TODO:             countTo = TextDEndOfLine(textD, lineStart, true);
// TODO:             *modRangeEnd = lineStart;
// TODO:             break;
// TODO:          }
// TODO:       }
// TODO:    }
// TODO:    *linesInserted = nLines;
// TODO: 
// TODO: 
// TODO:    /* Count deleted lines between countFrom and countTo as the text existed
// TODO:       before the modification (that is, as if the text between pos and
// TODO:       pos+nInserted were replaced by "deletedText").  This extra context is
// TODO:       necessary because wrapping can occur outside of the modified region
// TODO:       as a result of adding or deleting text in the region. This is done by
// TODO:       creating a textBuffer containing the deleted text and the necessary
// TODO:       additional context, and calling the wrappedLineCounter on it.
// TODO: 
// TODO:       NOTE: This must not be done in continuous wrap mode when the font
// TODO:        width is not fixed. In that case, the calculation would try
// TODO:        to access style information that is no longer available (deleted
// TODO:        text), or out of date (updated highlighting), possibly leading
// TODO:        to completely wrong calculations and/or even crashes eventually.
// TODO:        (This is not theoretical; it really happened.)
// TODO: 
// TODO:        In that case, the calculation of the number of deleted lines
// TODO:        has happened before the buffer was modified (only in that case,
// TODO:        because resynchronization of the line starts is impossible
// TODO:        in that case, which makes the whole calculation less efficient).
// TODO:    */
// TODO:    if (textD->suppressResync)
// TODO:    {
// TODO:       *linesDeleted = textD->nLinesDeleted;
// TODO:       textD->suppressResync = 0;
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    length = (pos-countFrom) + nDeleted +(countTo-(pos+nInserted));
// TODO:    deletedTextBuf = BufCreatePreallocated(length);
// TODO:    if (pos > countFrom)
// TODO:       BufCopyFromBuf(textD->buffer, deletedTextBuf, countFrom, pos, 0);
// TODO:    if (nDeleted != 0)
// TODO:       BufInsert(deletedTextBuf, pos-countFrom, deletedText);
// TODO:    if (countTo > pos+nInserted)
// TODO:       BufCopyFromBuf(textD->buffer, deletedTextBuf,
// TODO:                      pos+nInserted, countTo, pos-countFrom+nDeleted);
// TODO:    /* Note that we need to take into account an offset for the style buffer:
// TODO:       the deletedTextBuf can be out of sync with the style buffer. */
// TODO:    wrappedLineCounter(textD, deletedTextBuf, 0, length, INT_MAX, true,
// TODO:                       countFrom, &retPos, &retLines, &retLineStart, &retLineEnd);
// TODO:    BufFree(deletedTextBuf);
// TODO:    *linesDeleted = retLines;
// TODO:    textD->suppressResync = 0;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** This is a stripped-down version of the findWrapRange() function above,
// TODO: ** intended to be used to calculate the number of "deleted" lines during
// TODO: ** a buffer modification. It is called _before_ the modification takes place.
// TODO: **
// TODO: ** This function should only be called in continuous wrap mode with a
// TODO: ** non-fixed font width. In that case, it is impossible to calculate
// TODO: ** the number of deleted lines, because the necessary style information
// TODO: ** is no longer available _after_ the modification. In other cases, we
// TODO: ** can still perform the calculation afterwards (possibly even more
// TODO: ** efficiently).
// TODO: */
// TODO: static void measureDeletedLines(textDisp* textD, int pos, int nDeleted)
// TODO: {
// TODO:    int retPos, retLines, retLineStart, retLineEnd;
// TODO:    textBuffer* buf = textD->buffer;
// TODO:    int nVisLines = textD->nVisibleLines;
// TODO:    int* lineStarts = textD->lineStarts;
// TODO:    int countFrom, lineStart;
// TODO:    int nLines = 0, i;
// TODO:    /*
// TODO:    ** Determine where to begin searching: either the previous newline, or
// TODO:    ** if possible, limit to the start of the (original) previous displayed
// TODO:    ** line, using information from the existing line starts array
// TODO:    */
// TODO:    if (pos >= textD->firstChar && pos <= textD->lastChar)
// TODO:    {
// TODO:       for (i=nVisLines-1; i>0; i--)
// TODO:          if (lineStarts[i] != -1 && pos >= lineStarts[i])
// TODO:             break;
// TODO:       if (i > 0)
// TODO:       {
// TODO:          countFrom = lineStarts[i-1];
// TODO:       }
// TODO:       else
// TODO:          countFrom = BufStartOfLine(buf, pos);
// TODO:    }
// TODO:    else
// TODO:       countFrom = BufStartOfLine(buf, pos);
// TODO: 
// TODO:    /*
// TODO:    ** Move forward through the (new) text one line at a time, counting
// TODO:    ** displayed lines, and looking for either a real newline, or for the
// TODO:    ** line starts to re-sync with the original line starts array
// TODO:    */
// TODO:    lineStart = countFrom;
// TODO:    while (true)
// TODO:    {
// TODO:       /* advance to the next line.  If the line ended in a real newline
// TODO:          or the end of the buffer, that's far enough */
// TODO:       wrappedLineCounter(textD, buf, lineStart, buf->length, 1, true, 0,
// TODO:                          &retPos, &retLines, &retLineStart, &retLineEnd);
// TODO:       if (retPos >= buf->length)
// TODO:       {
// TODO:          if (retPos != retLineEnd)
// TODO:             nLines++;
// TODO:          break;
// TODO:       }
// TODO:       else
// TODO:          lineStart = retPos;
// TODO:       nLines++;
// TODO:       if (lineStart > pos + nDeleted &&
// TODO:             BufGetCharacter(buf, lineStart-1) == '\n')
// TODO:       {
// TODO:          break;
// TODO:       }
// TODO: 
// TODO:       /* Unlike in the findWrapRange() function above, we don't try to
// TODO:          resync with the line starts, because we don't know the length
// TODO:          of the inserted text yet, nor the updated style information.
// TODO: 
// TODO:          Because of that, we also shouldn't resync with the line starts
// TODO:          after the modification either, because we must perform the
// TODO:          calculations for the deleted and inserted lines in the same way.
// TODO: 
// TODO:          This can result in some unnecessary recalculation and redrawing
// TODO:          overhead, and therefore we should only use this two-phase mode
// TODO:          of calculation when it's really needed (continuous wrap + variable
// TODO:          font width). */
// TODO:    }
// TODO:    textD->nLinesDeleted = nLines;
// TODO:    textD->suppressResync = 1;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Count forward from startPos to either maxPos or maxLines (whichever is
// TODO: ** reached first), and return all relevant positions and line count.
// TODO: ** The provided textBuffer may differ from the actual text buffer of the
// TODO: ** widget. In that case it must be a (partial) copy of the actual text buffer
// TODO: ** and the styleBufOffset argument must indicate the starting position of the
// TODO: ** copy, to take into account the correct style information.
// TODO: **
// TODO: ** Returned values:
// TODO: **
// TODO: **   retPos:	    Position where counting ended.  When counting lines, the
// TODO: **  	    	    position returned is the start of the line "maxLines"
// TODO: **  	    	    lines beyond "startPos".
// TODO: **   retLines:	    Number of line breaks counted
// TODO: **   retLineStart:  Start of the line where counting ended
// TODO: **   retLineEnd:    End position of the last line traversed
// TODO: */
// TODO: static void wrappedLineCounter(const textDisp* textD, const textBuffer* buf,
// TODO:                                const int startPos, const int maxPos, const int maxLines,
// TODO:                                const bool startPosIsLineStart, const int styleBufOffset,
// TODO:                                int* retPos, int* retLines, int* retLineStart, int* retLineEnd)
// TODO: {
// TODO:    int lineStart, newLineStart = 0, b, p, colNum, wrapMargin;
// TODO:    int maxWidth, width, countPixels, i, foundBreak;
// TODO:    int nLines = 0, tabDist = textD->buffer->tabDist;
// TODO:    unsigned char c;
// TODO:    char nullSubsChar = textD->buffer->nullSubsChar;
// TODO: 
// TODO:    /* If the font is fixed, or there's a wrap margin set, it's more efficient
// TODO:       to measure in columns, than to count pixels.  Determine if we can count
// TODO:       in columns (countPixels == false) or must count pixels (countPixels ==
// TODO:       true), and set the wrap target for either pixels or columns */
// TODO:    if (textD->fixedFontWidth != -1 || textD->wrapMargin != 0)
// TODO:    {
// TODO:       countPixels = false;
// TODO:       wrapMargin = textD->wrapMargin != 0 ? textD->wrapMargin :
// TODO:                    textD->width / textD->fixedFontWidth;
// TODO:       maxWidth = INT_MAX;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       countPixels = true;
// TODO:       wrapMargin = INT_MAX;
// TODO:       maxWidth = textD->width;
// TODO:    }
// TODO: 
// TODO:    /* Find the start of the line if the start pos is not marked as a
// TODO:       line start. */
// TODO:    if (startPosIsLineStart)
// TODO:       lineStart = startPos;
// TODO:    else
// TODO:       lineStart = TextDStartOfLine(textD, startPos);
// TODO: 
// TODO:    /*
// TODO:    ** Loop until position exceeds maxPos or line count exceeds maxLines.
// TODO:    ** (actually, contines beyond maxPos to end of line containing maxPos,
// TODO:    ** in case later characters cause a word wrap back before maxPos)
// TODO:    */
// TODO:    colNum = 0;
// TODO:    width = 0;
// TODO:    for (p=lineStart; p<buf->length; p++)
// TODO:    {
// TODO:       c = BufGetCharacter(buf, p);
// TODO: 
// TODO:       /* If the character was a newline, count the line and start over,
// TODO:          otherwise, add it to the width and column counts */
// TODO:       if (c == '\n')
// TODO:       {
// TODO:          if (p >= maxPos)
// TODO:          {
// TODO:             *retPos = maxPos;
// TODO:             *retLines = nLines;
// TODO:             *retLineStart = lineStart;
// TODO:             *retLineEnd = maxPos;
// TODO:             return;
// TODO:          }
// TODO:          nLines++;
// TODO:          if (nLines >= maxLines)
// TODO:          {
// TODO:             *retPos = p + 1;
// TODO:             *retLines = nLines;
// TODO:             *retLineStart = p + 1;
// TODO:             *retLineEnd = p;
// TODO:             return;
// TODO:          }
// TODO:          lineStart = p + 1;
// TODO:          colNum = 0;
// TODO:          width = 0;
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          colNum += BufCharWidth(c, colNum, tabDist, nullSubsChar);
// TODO:          if (countPixels)
// TODO:             width += measurePropChar(textD, c, colNum, p+styleBufOffset);
// TODO:       }
// TODO: 
// TODO:       /* If character exceeded wrap margin, find the break point
// TODO:          and wrap there */
// TODO:       if (colNum > wrapMargin || width > maxWidth)
// TODO:       {
// TODO:          foundBreak = false;
// TODO:          for (b=p; b>=lineStart; b--)
// TODO:          {
// TODO:             c = BufGetCharacter(buf, b);
// TODO:             if (c == '\t' || c == ' ')
// TODO:             {
// TODO:                newLineStart = b + 1;
// TODO:                if (countPixels)
// TODO:                {
// TODO:                   colNum = 0;
// TODO:                   width = 0;
// TODO:                   for (i=b+1; i<p+1; i++)
// TODO:                   {
// TODO:                      width += measurePropChar(textD,
// TODO:                                               BufGetCharacter(buf, i), colNum,
// TODO:                                               i+styleBufOffset);
// TODO:                      colNum++;
// TODO:                   }
// TODO:                }
// TODO:                else
// TODO:                   colNum = BufCountDispChars(buf, b+1, p+1);
// TODO:                foundBreak = true;
// TODO:                break;
// TODO:             }
// TODO:          }
// TODO:          if (!foundBreak)   /* no whitespace, just break at margin */
// TODO:          {
// TODO:             newLineStart = max(p, lineStart+1);
// TODO:             colNum = BufCharWidth(c, colNum, tabDist, nullSubsChar);
// TODO:             if (countPixels)
// TODO:                width = measurePropChar(textD, c, colNum, p+styleBufOffset);
// TODO:          }
// TODO:          if (p >= maxPos)
// TODO:          {
// TODO:             *retPos = maxPos;
// TODO:             *retLines = maxPos < newLineStart ? nLines : nLines + 1;
// TODO:             *retLineStart = maxPos < newLineStart ? lineStart :
// TODO:                             newLineStart;
// TODO:             *retLineEnd = maxPos;
// TODO:             return;
// TODO:          }
// TODO:          nLines++;
// TODO:          if (nLines >= maxLines)
// TODO:          {
// TODO:             *retPos = foundBreak ? b + 1 : max(p, lineStart+1);
// TODO:             *retLines = nLines;
// TODO:             *retLineStart = lineStart;
// TODO:             *retLineEnd = foundBreak ? b : p;
// TODO:             return;
// TODO:          }
// TODO:          lineStart = newLineStart;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* reached end of buffer before reaching pos or line target */
// TODO:    *retPos = buf->length;
// TODO:    *retLines = nLines;
// TODO:    *retLineStart = lineStart;
// TODO:    *retLineEnd = buf->length;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Measure the width in pixels of a character "c" at a particular column
// TODO: ** "colNum" and buffer position "pos".  This is for measuring characters in
// TODO: ** proportional or mixed-width highlighting fonts.
// TODO: **
// TODO: ** A note about proportional and mixed-width fonts: the mixed width and
// TODO: ** proportional font code in nedit does not get much use in general editing,
// TODO: ** because nedit doesn't allow per-language-mode fonts, and editing programs
// TODO: ** in a proportional font is usually a bad idea, so very few users would
// TODO: ** choose a proportional font as a default.  There are still probably mixed-
// TODO: ** width syntax highlighting cases where things don't redraw properly for
// TODO: ** insertion/deletion, though static display and wrapping and resizing
// TODO: ** should now be solid because they are now used for online help display.
// TODO: */
// TODO: static int measurePropChar(const textDisp* textD, const char c,
// TODO:                            const int colNum, const int pos)
// TODO: {
// TODO:    int charLen, style;
// TODO:    char expChar[MAX_EXP_CHAR_LEN];
// TODO:    textBuffer* styleBuf = textD->styleBuffer;
// TODO: 
// TODO:    charLen = BufExpandCharacter(c, colNum, expChar,
// TODO:                                 textD->buffer->tabDist, textD->buffer->nullSubsChar);
// TODO:    if (styleBuf == NULL)
// TODO:    {
// TODO:       style = 0;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       style = (unsigned char)BufGetCharacter(styleBuf, pos);
// TODO:       if (style == textD->unfinishedStyle)
// TODO:       {
// TODO:          /* encountered "unfinished" style, trigger parsing */
// TODO:          (textD->unfinishedHighlightCB)(textD, pos, textD->highlightCBArg);
// TODO:          style = (unsigned char)BufGetCharacter(styleBuf, pos);
// TODO:       }
// TODO:    }
// TODO:    return stringWidth(textD, expChar, charLen, style);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Finds both the end of the current line and the start of the next line.  Why?
// TODO: ** In continuous wrap mode, if you need to know both, figuring out one from the
// TODO: ** other can be expensive or error prone.  The problem comes when there's a
// TODO: ** trailing space or tab just before the end of the buffer.  To translate an
// TODO: ** end of line value to or from the next lines start value, you need to know
// TODO: ** whether the trailing space or tab is being used as a line break or just a
// TODO: ** normal character, and to find that out would otherwise require counting all
// TODO: ** the way back to the beginning of the line.
// TODO: */
// TODO: static void findLineEnd(textDisp* textD, int startPos, int startPosIsLineStart,
// TODO:                         int* lineEnd, int* nextLineStart)
// TODO: {
// TODO:    int retLines, retLineStart;
// TODO: 
// TODO:    /* if we're not wrapping use more efficient BufEndOfLine */
// TODO:    if (!textD->continuousWrap)
// TODO:    {
// TODO:       *lineEnd = BufEndOfLine(textD->buffer, startPos);
// TODO:       *nextLineStart = min(textD->buffer->length, *lineEnd + 1);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* use the wrapped line counter routine to count forward one line */
// TODO:    wrappedLineCounter(textD, textD->buffer, startPos, textD->buffer->length,
// TODO:                       1, startPosIsLineStart, 0, nextLineStart, &retLines,
// TODO:                       &retLineStart, lineEnd);
// TODO:    return;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Line breaks in continuous wrap mode usually happen at newlines or
// TODO: ** whitespace.  This line-terminating character is not included in line
// TODO: ** width measurements and has a special status as a non-visible character.
// TODO: ** However, lines with no whitespace are wrapped without the benefit of a
// TODO: ** line terminating character, and this distinction causes endless trouble
// TODO: ** with all of the text display code which was originally written without
// TODO: ** continuous wrap mode and always expects to wrap at a newline character.
// TODO: **
// TODO: ** Given the position of the end of the line, as returned by TextDEndOfLine
// TODO: ** or BufEndOfLine, this returns true if there is a line terminating
// TODO: ** character, and false if there's not.  On the last character in the
// TODO: ** buffer, this function can't tell for certain whether a trailing space was
// TODO: ** used as a wrap point, and just guesses that it wasn't.  So if an exact
// TODO: ** accounting is necessary, don't use this function.
// TODO: */
// TODO: static int wrapUsesCharacter(textDisp* textD, int lineEndPos)
// TODO: {
// TODO:    char c;
// TODO: 
// TODO:    if (!textD->continuousWrap || lineEndPos == textD->buffer->length)
// TODO:       return true;
// TODO: 
// TODO:    c = BufGetCharacter(textD->buffer, lineEndPos);
// TODO:    return c == '\n' || ((c == '\t' || c == ' ') &&
// TODO:                         lineEndPos + 1 != textD->buffer->length);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Decide whether the user needs (or may need) a horizontal scroll bar,
// TODO: ** and manage or unmanage the scroll bar widget accordingly.  The H.
// TODO: ** scroll bar is only hidden in continuous wrap mode when it's absolutely
// TODO: ** certain that the user will not need it: when wrapping is set
// TODO: ** to the window edge, or when the wrap margin is strictly less than
// TODO: ** the longest possible line.
// TODO: */
// TODO: static void hideOrShowHScrollBar(textDisp* textD)
// TODO: {
// TODO:    if (textD->continuousWrap && (textD->wrapMargin == 0 || textD->wrapMargin *
// TODO:                                  textD->fontStruct->max_bounds.width < textD->width))
// TODO:       XtUnmanageChild(textD->hScrollBar);
// TODO:    else
// TODO:       XtManageChild(textD->hScrollBar);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Return true if the selection "sel" is rectangular, and touches a
// TODO: ** buffer position withing "rangeStart" to "rangeEnd"
// TODO: */
// TODO: static int rangeTouchesRectSel(selection* sel, int rangeStart, int rangeEnd)
// TODO: {
// TODO:    return sel->selected && sel->rectangular && sel->end >= rangeStart &&
// TODO:           sel->start <= rangeEnd;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Extend the range of a redraw request (from *start to *end) with additional
// TODO: ** redraw requests resulting from changes to the attached style buffer (which
// TODO: ** contains auxiliary information for coloring or styling text).
// TODO: */
// TODO: static void extendRangeForStyleMods(textDisp* textD, int* start, int* end)
// TODO: {
// TODO:    selection* sel = &textD->styleBuffer->primary;
// TODO:    int extended = false;
// TODO: 
// TODO:    /* The peculiar protocol used here is that modifications to the style
// TODO:       buffer are marked by selecting them with the buffer's primary selection.
// TODO:       The style buffer is usually modified in response to a modify callback on
// TODO:       the text buffer BEFORE textDisp.c's modify callback, so that it can keep
// TODO:       the style buffer in step with the text buffer.  The style-update
// TODO:       callback can't just call for a redraw, because textDisp hasn't processed
// TODO:       the original text changes yet.  Anyhow, to minimize redrawing and to
// TODO:       avoid the complexity of scheduling redraws later, this simple protocol
// TODO:       tells the text display's buffer modify callback to extend it's redraw
// TODO:       range to show the text color/and font changes as well. */
// TODO:    if (sel->selected)
// TODO:    {
// TODO:       if (sel->start < *start)
// TODO:       {
// TODO:          *start = sel->start;
// TODO:          extended = true;
// TODO:       }
// TODO:       if (sel->end > *end)
// TODO:       {
// TODO:          *end = sel->end;
// TODO:          extended = true;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* If the selection was extended due to a style change, and some of the
// TODO:       fonts don't match in spacing, extend redraw area to end of line to
// TODO:       redraw characters exposed by possible font size changes */
// TODO:    if (textD->fixedFontWidth == -1 && extended)
// TODO:       *end = BufEndOfLine(textD->buffer, *end) + 1;
// TODO: }
// TODO: 
// TODO: /**********************  Backlight Functions ******************************/
// TODO: /*
// TODO: ** Allocate a read-only (shareable) colormap cell for a named color, from the
// TODO: ** the default colormap of the screen on which the widget (w) is displayed. If
// TODO: ** the colormap is full and there's no suitable substitute, print an error on
// TODO: ** stderr, and return the widget's background color as a backup.
// TODO: */
// TODO: static Pixel allocBGColor(Widget w, char* colorName, int* ok)
// TODO: {
// TODO:    int r,g,b;
// TODO:    *ok = 1;
// TODO:    return AllocateColor(w, colorName, &r, &g, &b);
// TODO: }
// TODO: 
// TODO: static Pixel getRangesetColor(textDisp* textD, int ind, Pixel bground)
// TODO: {
// TODO:    textBuffer* buf;
// TODO:    RangesetTable* tab;
// TODO:    Pixel color;
// TODO:    char* color_name;
// TODO:    int valid;
// TODO: 
// TODO:    if (ind > 0)
// TODO:    {
// TODO:       ind--;
// TODO:       buf = textD->buffer;
// TODO:       tab = buf->rangesetTable;
// TODO: 
// TODO:       valid = RangesetTableGetColorValid(tab, ind, &color);
// TODO:       if (valid == 0)
// TODO:       {
// TODO:          color_name = RangesetTableGetColorName(tab, ind);
// TODO:          if (color_name)
// TODO:             color = allocBGColor(textD->w, color_name, &valid);
// TODO:          RangesetTableAssignColorPixel(tab, ind, color, valid);
// TODO:       }
// TODO:       if (valid > 0)
// TODO:       {
// TODO:          return color;
// TODO:       }
// TODO:    }
// TODO:    return bground;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Read the background color class specification string in str, allocating the
// TODO: ** necessary colors, and allocating and setting up the character->class_no and
// TODO: ** class_no->pixel map arrays, returned via *pp_bgClass and *pp_bgClassPixel
// TODO: ** respectively.
// TODO: ** Note: the allocation of class numbers could be more intelligent: there can
// TODO: ** never be more than 256 of these (one per character); but I don't think
// TODO: ** there'll be a pressing need. I suppose the scanning of the specification
// TODO: ** could be better too, but then, who cares!
// TODO: */
// TODO: void TextDSetupBGClasses(Widget w, NeString str, Pixel** pp_bgClassPixel,
// TODO:                          unsigned char** pp_bgClass, Pixel bgPixelDefault)
// TODO: {
// TODO:    unsigned char bgClass[256];
// TODO:    Pixel bgClassPixel[256];
// TODO:    int class_no = 0;
// TODO:    char* semicol;
// TODO:    char* s = (char*)str;
// TODO:    size_t was_semicol;
// TODO:    int lo, hi, dummy;
// TODO:    char* pos;
// TODO:    bool is_good = true;
// TODO: 
// TODO:    XtFree((char*)*pp_bgClass);
// TODO:    XtFree((char*)*pp_bgClassPixel);
// TODO: 
// TODO:    *pp_bgClassPixel = NULL;
// TODO:    *pp_bgClass = NULL;
// TODO: 
// TODO:    if (!s)
// TODO:       return;
// TODO: 
// TODO:    /* default for all chars is class number zero, for standard background */
// TODO:    memset(bgClassPixel, 0, sizeof bgClassPixel);
// TODO:    memset(bgClass, 0, sizeof bgClass);
// TODO:    bgClassPixel[0] = bgPixelDefault;
// TODO:    /* since class no == 0 in a "style" has no set bits in BACKLIGHT_MASK
// TODO:       (see styleOfPos()), when drawString() is called for text with a
// TODO:       backlight class no of zero, bgClassPixel[0] is never consulted, and
// TODO:       the default background color is chosen. */
// TODO: 
// TODO:    /* The format of the class string s is:
// TODO:              low[-high]{,low[-high]}:color{;low-high{,low[-high]}:color}
// TODO:          eg
// TODO:              32-255:#f0f0f0;1-31,127:red;128-159:orange;9-13:#e5e5e5
// TODO:       where low and high represent a character range between ordinal
// TODO:       ASCII values. Using strtol() allows automatic octal, dec and hex
// TODO:       reading of low and high. The example format sets backgrounds as follows:
// TODO:              char   1 - 8    colored red     (control characters)
// TODO:              char   9 - 13   colored #e5e5e5 (isspace() control characters)
// TODO:              char  14 - 31   colored red     (control characters)
// TODO:              char  32 - 126  colored #f0f0f0
// TODO:              char 127        colored red     (delete character)
// TODO:              char 128 - 159  colored orange  ("shifted" control characters)
// TODO:              char 160 - 255  colored #f0f0f0
// TODO:       Notice that some of the later ranges overwrite the class values defined
// TODO:       for earlier ones (eg the first clause, 32-255:#f0f0f0 sets the DEL
// TODO:       character background color to #f0f0f0; it is then set to red by the
// TODO:       clause 1-31,127:red). */
// TODO: 
// TODO:    while (s && class_no < 255)
// TODO:    {
// TODO:       class_no++;                   /* simple class alloc scheme */
// TODO:       was_semicol = 0;
// TODO:       is_good = true;
// TODO:       if ((semicol = (char*)strchr(s, ';')))
// TODO:       {
// TODO:          *semicol = '\0';    /* null-terminate low[-high]:color clause */
// TODO:          was_semicol = 1;
// TODO:       }
// TODO: 
// TODO:       /* loop over ranges before the color spec, assigning the characters
// TODO:          in the ranges to the current class number */
// TODO:       for (lo = hi = strtol(s, &pos, 0);
// TODO:             is_good;
// TODO:             lo = hi = strtol(pos + 1, &pos, 0))
// TODO:       {
// TODO:          if (pos && *pos == '-')
// TODO:             hi = strtol(pos + 1, &pos, 0);  /* get end of range */
// TODO:          is_good = (pos && 0 <= lo && lo <= hi && hi <= 255);
// TODO:          if (is_good)
// TODO:             while (lo <= hi)
// TODO:                bgClass[lo++] = (unsigned char)class_no;
// TODO:          if (*pos != ',')
// TODO:             break;
// TODO:       }
// TODO:       if ((is_good = (is_good && *pos == ':')))
// TODO:       {
// TODO:          is_good = (*pos++ != '\0');         /* pos now points to color */
// TODO:          bgClassPixel[class_no] = allocBGColor(w, pos, &dummy);
// TODO:       }
// TODO:       if (!is_good)
// TODO:       {
// TODO:          /* complain? this class spec clause (in string s) was faulty */
// TODO:       }
// TODO: 
// TODO:       /* end of loop iterator clauses */
// TODO:       if (was_semicol)
// TODO:          *semicol = ';';       /* un-null-terminate low[-high]:color clause */
// TODO:       s = semicol + was_semicol;
// TODO:    }
// TODO: 
// TODO:    /* when we get here, we've set up our class table and class-to-pixel table
// TODO:       in local variables: now put them into the "real thing" */
// TODO:    class_no++;                     /* bigger than all valid class_nos */
// TODO:    *pp_bgClass = (unsigned char*)malloc__(256);
// TODO:    *pp_bgClassPixel = (Pixel*)malloc__(class_no * sizeof(Pixel));
// TODO:    if (!*pp_bgClass || !*pp_bgClassPixel)
// TODO:    {
// TODO:       XtFree((char*)*pp_bgClass);
// TODO:       XtFree((char*)*pp_bgClassPixel);
// TODO:       return;
// TODO:    }
// TODO:    memcpy(*pp_bgClass, bgClass, 256);
// TODO:    memcpy(*pp_bgClassPixel, bgClassPixel, class_no * sizeof(Pixel));
// TODO: }
