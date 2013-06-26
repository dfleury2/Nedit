static const char CVSID[] = "$Id: selection.c,v 1.34 2008/02/26 22:21:47 ajbj Exp $";
/*******************************************************************************
*									       *
* Copyright (C) 1999 Mark Edel						       *
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
* May 10, 1991								       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "selection.h"
#include "Ne_Text_Buffer.h"
#include "Ne_Text_Editor.h"
#include "nedit.h"
#include "file.h"
#include "window.h"
#include "menu.h"
#include "preferences.h"
#include "server.h"
#include "../util/DialogF.h"
#include "../util/fileUtils.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#ifndef WIN32
#include <sys/param.h>
#endif

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

#include <FL/fl_ask.H>

static void gotoCB(Ne_Text_Editor* w, WindowInfo* window, Atom* sel, Atom* type, char* value, int* length, int* format);
static void fileCB(Ne_Text_Editor* w, WindowInfo* window, Atom* sel, Atom* type, char* value, int* length, int* format);
// TODO: static void getAnySelectionCB(Widget widget, char** result, Atom* sel, Atom* type, char* value, int* length, int* format);
// TODO: static void processMarkEvent(Widget w, XtPointer clientData, XEvent* event, bool* continueDispatch, char* action, int extend);
// TODO: static void markTimeoutProc(XtPointer clientData, XtIntervalId* id);
// TODO: static void markKeyCB(Widget w, XtPointer clientData, XEvent* event, bool* continueDispatch);
// TODO: static void gotoMarkKeyCB(Widget w, XtPointer clientData, XEvent* event, bool* continueDispatch);
// TODO: static void gotoMarkExtendKeyCB(Widget w, XtPointer clientData, XEvent* event, bool* continueDispatch);
static void maintainSelection(selection* sel, int pos, int nInserted, int nDeleted);
static void maintainPosition(int* position, int modPos, int nInserted, int nDeleted);

/*
** Extract the line and column number from the text string.
** Set the line and/or column number to -1 if not specified, and return -1 if
** both line and column numbers are not specified.
*/
int StringToLineAndCol(const char* text, int* lineNum, int* column)
{
   char* endptr;
   long  tempNum;
   int   textLen;

   /* Get line number */
   tempNum = strtol(text, &endptr, 10);

   /* If user didn't specify a line number, set lineNum to -1 */
   if (endptr  == text)
   {
      *lineNum = -1;
   }
   else if (tempNum >= INT_MAX)
   {
      *lineNum = INT_MAX;
   }
   else if (tempNum <  0)
   {
      *lineNum = 0;
   }
   else
   {
      *lineNum = tempNum;
   }

   /* Find the next digit */
   for (textLen = strlen(endptr); textLen > 0; endptr++, textLen--)
   {
      if (isdigit((unsigned char)*endptr) ||
            *endptr == '-' || *endptr == '+')
      {
         break;
      }
   }

   /* Get column */
   if (*endptr != '\0')
   {
      tempNum = strtol(endptr, NULL, 10);
      if (tempNum >= INT_MAX)
      {
         *column = INT_MAX;
      }
      else if (tempNum <  0)
      {
         *column = 0;
      }
      else
      {
         *column = tempNum;
      }
   }
   else
   {
      *column = -1;
   }

   return *lineNum == -1 && *column == -1 ? -1 : 0;
}

void GotoLineNumber(WindowInfo* window)
{
   char lineNumText[DF_MAX_PROMPT_LENGTH];
   const char* params[1];
   int lineNum, column, response;

   response = DialogF(DF_PROMPT, window->mainWindow, 2, "Goto Line Number", "Goto Line (and/or Column)  Number:", lineNumText, "OK", "Cancel");
   if (response == 0)
      return;

   if (StringToLineAndCol(lineNumText, &lineNum, &column) == -1)
   {
      fl_beep();
      return;
   }
   params[0] = lineNumText;
   AppContext.callAction(window->lastFocus, "goto_line_number", NULL, params, 1);
}

void GotoSelectedLineNumber(WindowInfo* window, double time)
{
   char* selectedValue = BufGetSelectionText(window->buffer);
   int length = strlen(selectedValue);
   gotoCB(window->textArea, window, 0, 0, selectedValue, &length, 0);
   delete[] selectedValue;
}

void OpenSelectedFile(WindowInfo* window, double time)
{
   char* selectedValue = BufGetSelectionText(window->buffer);
   int length = strlen(selectedValue);
   fileCB(window->textArea, window, 0, 0, selectedValue, &length, 0);
   delete[] selectedValue;
}

/*
** Getting the current selection by making the request, and then blocking
** (processing events) while waiting for a reply.  On failure (timeout or
** bad format) returns NULL, otherwise returns the contents of the selection.
*/
char* GetAnySelection(WindowInfo* window)
{
   static char waitingMarker[1] = "";
   char* selText = waitingMarker;
// TODO:    XEvent nextEvent;
// TODO: 
// TODO:    /* If the selection is in the window's own buffer get it from there,
// TODO:       but substitute null characters as if it were an external selection */
// TODO:    if (window->buffer->primary.selected)
// TODO:    {
// TODO:       selText = BufGetSelectionText(window->buffer);
// TODO:       BufUnsubstituteNullChars(selText, window->buffer);
// TODO:       return selText;
// TODO:    }
// TODO: 
// TODO:    /* Request the selection value to be delivered to getAnySelectionCB */
// TODO:    XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
// TODO:                        (XtSelectionCallbackProc)getAnySelectionCB, &selText,
// TODO:                        XtLastTimestampProcessed(XtDisplay(window->textArea)));
// TODO: 
// TODO:    /* Wait for the value to appear */
// TODO:    while (selText == waitingMarker)
// TODO:    {
// TODO:       XtAppNextEvent(XtWidgetToApplicationContext(window->textArea),
// TODO:                      &nextEvent);
// TODO:       ServerDispatchEvent(&nextEvent);
// TODO:    }
   return selText;
}

static void gotoCB(Ne_Text_Editor* w, WindowInfo* window, Atom* sel, Atom* type, char* value, int* length, int* format)
{
   /* two integers and some space in between */
   char lineText[(TYPE_INT_STR_SIZE(int) * 2) + 5];
   int rc, lineNum, column, position, curCol;

   /* skip if we can't get the selection data, or it's obviously not a number */
   if (value == NULL)
   {
      fl_beep();
      return;
   }
   if (((size_t) *length) > sizeof(lineText) - 1)
   {
      fl_beep();
      return;
   }
   /* should be of type text??? */
   if (format && *format != 8)
   {
      fprintf(stderr, "NEdit: Can't handle non 8-bit text\n");
      fl_beep();
      return;
   }
   strncpy(lineText, value, sizeof(lineText));
   lineText[sizeof(lineText) - 1] = '\0';

   rc = StringToLineAndCol(lineText, &lineNum, &column);
   if (rc == -1)
   {
      fl_beep();
      return;
   }

   /* User specified column, but not line number */
   if (lineNum == -1)
   {
      position = TextGetCursorPos(w);
      if (TextPosToLineAndCol(w, position, &lineNum, &curCol) == false)
      {
         fl_beep();
         return;
      }
   }
   /* User didn't specify a column */
   else if (column == -1)
   {
      SelectNumberedLine(window, lineNum);
      return;
   }

   position = TextLineAndColToPos(w, lineNum, column);
   if (position == -1)
   {
      fl_beep();
      return;
   }
   TextSetCursorPos(w, position);
}

static void fileCB(Ne_Text_Editor* w, WindowInfo* window, Atom* sel, Atom* type, char* value, int* length, int* format)
{
   char nameText[MAXPATHLEN], includeName[MAXPATHLEN];
   char filename[MAXPATHLEN], pathname[MAXPATHLEN];
   char* inPtr, *outPtr;
   static char includeDir[] = "/usr/include/";

   /* get the string, or skip if we can't get the selection data, or it's
      obviously not a file name */
   if (value == NULL)
   {
      fl_beep();
      return;
   }
   if (*length > MAXPATHLEN || *length == 0)
   {
      fl_beep();
      return;
   }
   /* should be of type text??? */
   if (format && *format != 8)
   {
      fprintf(stderr, "NEdit: Can't handle non 8-bit text\n");
      fl_beep();
      return;
   }
   strncpy(nameText, value, *length);
   nameText[*length] = '\0';

   /* extract name from #include syntax */
   if (sscanf(nameText, "#include \"%[^\"]\"", includeName) == 1)
      strcpy(nameText, includeName);
   else if (sscanf(nameText, "#include <%[^<>]>", includeName) == 1)
      sprintf(nameText, "%s%s", includeDir, includeName);

   /* strip whitespace from name */
   for (inPtr=nameText, outPtr=nameText; *inPtr!='\0'; inPtr++)
      if (*inPtr != ' ' && *inPtr != '\t' && *inPtr != '\n')
         *outPtr++ = *inPtr;
   *outPtr = '\0';

   /* Process ~ characters in name */
   ExpandTilde(nameText);

   /* If path name is relative, make it refer to current window's directory */
   if (nameText[0] != '/')
   {
      strcpy(filename, window->path);
      strcat(filename, nameText);
      strcpy(nameText, filename);
   }

   /* Expand wildcards in file name.
      Some older systems don't have the glob subroutine for expanding file
      names, in these cases, either don't expand names, or try to use the
      Motif internal parsing routine _XmOSGetDirEntries, which is not
      guaranteed to be available, but in practice is there and does work. */
   /* Open the file */
   if (ParseFilename(nameText, filename, pathname) != 0)
   {
      fl_beep();
      return;
   }
   EditExistingFile(window, filename, pathname, 0, NULL, false, NULL, GetPrefOpenInTab(), false);

   CheckCloseDim();
}

// TODO: static void getAnySelectionCB(Widget widget, char** result, Atom* sel,
// TODO:                               Atom* type, char* value, int* length, int* format)
// TODO: {
// TODO:    /* Confirm that the returned value is of the correct type */
// TODO:    if (*type != XA_STRING || *format != 8)
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:       XtFree((char*) value);
// TODO:       *result = NULL;
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Append a null, and return the string */
// TODO:    *result = malloc__(*length + 1);
// TODO:    strncpy(*result, value, *length);
// TODO:    XtFree(value);
// TODO:    (*result)[*length] = '\0';
// TODO: }
// TODO: 
void SelectNumberedLine(WindowInfo* window, int lineNum)
{
   int i, lineStart = 0, lineEnd;

   /* count lines to find the start and end positions for the selection */
   if (lineNum < 1)
      lineNum = 1;
   lineEnd = -1;
   for (i=1; i<=lineNum && lineEnd<window->buffer->length; i++)
   {
      lineStart = lineEnd + 1;
      lineEnd = BufEndOfLine(window->buffer, lineStart);
   }

   /* highlight the line */
   if (i>lineNum)
   {
      /* Line was found */
      if (lineEnd < window->buffer->length)
      {
         BufSelect(window->buffer, lineStart, lineEnd+1);
      }
      else
      {
         /* Don't select past the end of the buffer ! */
         BufSelect(window->buffer, lineStart, window->buffer->length);
      }
   }
   else
   {
      /* Line was not found -> position the selection & cursor at the end
         without making a real selection and beep */
      lineStart = window->buffer->length;
      BufSelect(window->buffer, lineStart, lineStart);
      fl_beep();
   }
   MakeSelectionVisible(window, window->lastFocus);
   TextSetCursorPos(window->lastFocus, lineStart);
}

// TODO: void MarkDialog(WindowInfo* window)
// TODO: {
// TODO:    char letterText[DF_MAX_PROMPT_LENGTH], *params[1];
// TODO:    int response;
// TODO: 
// TODO:    response = DialogF(DF_PROMPT, window->mainWindow, 2, "Mark",
// TODO:                       "Enter a single letter label to use for recalling\n"
// TODO:                       "the current selection and cursor position.\n\n"
// TODO:                       "(To skip this dialog, use the accelerator key,\n"
// TODO:                       "followed immediately by a letter key (a-z))", letterText, "OK",
// TODO:                       "Cancel");
// TODO:    if (response == 2)
// TODO:       return;
// TODO:    if (strlen(letterText) != 1 || !isalpha((unsigned char)letterText[0]))
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:       return;
// TODO:    }
// TODO:    params[0] = letterText;
// TODO:    XtCallActionProc(window->lastFocus, "mark", NULL, params, 1);
// TODO: }
// TODO: 
// TODO: void GotoMarkDialog(WindowInfo* window, int extend)
// TODO: {
// TODO:    char letterText[DF_MAX_PROMPT_LENGTH], *params[2];
// TODO:    int response;
// TODO: 
// TODO:    response = DialogF(DF_PROMPT, window->mainWindow, 2, "Goto Mark",
// TODO:                       "Enter the single letter label used to mark\n"
// TODO:                       "the selection and/or cursor position.\n\n"
// TODO:                       "(To skip this dialog, use the accelerator\n"
// TODO:                       "key, followed immediately by the letter)", letterText, "OK",
// TODO:                       "Cancel");
// TODO:    if (response == 2)
// TODO:       return;
// TODO:    if (strlen(letterText) != 1 || !isalpha((unsigned char)letterText[0]))
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:       return;
// TODO:    }
// TODO:    params[0] = letterText;
// TODO:    params[1] = "extend";
// TODO:    XtCallActionProc(window->lastFocus, "goto_mark", NULL, params,
// TODO:                     extend ? 2 : 1);
// TODO: }

/*
** Process a command to mark a selection.  Expects the user to continue
** the command by typing a label character.  Handles both correct user
** behavior (type a character a-z) or bad behavior (do nothing or type
** something else).
*/
void BeginMarkCommand(WindowInfo* window)
{
// TODO:    XtInsertEventHandler(window->lastFocus, KeyPressMask, false, markKeyCB, window, XtListHead);
// TODO:    window->markTimeoutID = XtAppAddTimeOut(
// TODO:                               XtWidgetToApplicationContext(window->mainWindow), 4000,
// TODO:                               markTimeoutProc, window->lastFocus);
}

// TODO: /*
// TODO: ** Process a command to go to a marked selection.  Expects the user to
// TODO: ** continue the command by typing a label character.  Handles both correct
// TODO: ** user behavior (type a character a-z) or bad behavior (do nothing or type
// TODO: ** something else).
// TODO: */
// TODO: void BeginGotoMarkCommand(WindowInfo* window, int extend)
// TODO: {
// TODO:    XtInsertEventHandler(window->lastFocus, KeyPressMask, false,
// TODO:                         extend ? gotoMarkExtendKeyCB : gotoMarkKeyCB, window, XtListHead);
// TODO:    window->markTimeoutID = XtAppAddTimeOut(
// TODO:                               XtWidgetToApplicationContext(window->mainWindow), 4000,
// TODO:                               markTimeoutProc, window->lastFocus);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Xt timer procedure for removing event handler if user failed to type a
// TODO: ** mark character withing the allowed time
// TODO: */
// TODO: static void markTimeoutProc(XtPointer clientData, XtIntervalId* id)
// TODO: {
// TODO:    Widget w = (Widget)clientData;
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO: 
// TODO:    XtRemoveEventHandler(w, KeyPressMask, false, markKeyCB, window);
// TODO:    XtRemoveEventHandler(w, KeyPressMask, false, gotoMarkKeyCB, window);
// TODO:    XtRemoveEventHandler(w, KeyPressMask, false, gotoMarkExtendKeyCB, window);
// TODO:    window->markTimeoutID = 0;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Temporary event handlers for keys pressed after the mark or goto-mark
// TODO: ** commands, If the key is valid, grab the key event and call the action
// TODO: ** procedure to mark (or go to) the selection, otherwise, remove the handler
// TODO: ** and give up.
// TODO: */
// TODO: static void processMarkEvent(Widget w, XtPointer clientData, XEvent* event,
// TODO:                              bool* continueDispatch, char* action, int extend)
// TODO: {
// TODO:    XKeyEvent* e = (XKeyEvent*)event;
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    Modifiers modifiers;
// TODO:    KeySym keysym;
// TODO:    char* params[2], string[2];
// TODO: 
// TODO:    XtTranslateKeycode(TheDisplay, e->keycode, e->state, &modifiers,
// TODO:                       &keysym);
// TODO:    if ((keysym >= 'A' && keysym <= 'Z') || (keysym >= 'a' && keysym <= 'z'))
// TODO:    {
// TODO:       string[0] = toupper(keysym);
// TODO:       string[1] = '\0';
// TODO:       params[0] = string;
// TODO:       params[1] = "extend";
// TODO:       XtCallActionProc(window->lastFocus, action, event, params,
// TODO:                        extend ? 2 : 1);
// TODO:       *continueDispatch = false;
// TODO:    }
// TODO:    XtRemoveEventHandler(w, KeyPressMask, false, markKeyCB, window);
// TODO:    XtRemoveEventHandler(w, KeyPressMask, false, gotoMarkKeyCB, window);
// TODO:    XtRemoveEventHandler(w, KeyPressMask, false, gotoMarkExtendKeyCB, window);
// TODO:    XtRemoveTimeOut(window->markTimeoutID);
// TODO: }
// TODO: static void markKeyCB(Widget w, XtPointer clientData, XEvent* event,
// TODO:                       bool* continueDispatch)
// TODO: {
// TODO:    processMarkEvent(w, clientData, event, continueDispatch, "mark", false);
// TODO: }
// TODO: static void gotoMarkKeyCB(Widget w, XtPointer clientData, XEvent* event,
// TODO:                           bool* continueDispatch)
// TODO: {
// TODO:    processMarkEvent(w, clientData, event, continueDispatch, "goto_mark",false);
// TODO: }
// TODO: static void gotoMarkExtendKeyCB(Widget w, XtPointer clientData, XEvent* event,
// TODO:                                 bool* continueDispatch)
// TODO: {
// TODO:    processMarkEvent(w, clientData, event, continueDispatch, "goto_mark", true);
// TODO: }
// TODO: 
// TODO: void AddMark(WindowInfo* window, Widget widget, char label)
// TODO: {
// TODO:    int index;
// TODO: 
// TODO:    /* look for a matching mark to re-use, or advance
// TODO:       nMarks to create a new one */
// TODO:    label = toupper(label);
// TODO:    for (index=0; index<window->nMarks; index++)
// TODO:    {
// TODO:       if (window->markTable[index].label == label)
// TODO:          break;
// TODO:    }
// TODO:    if (index >= MAX_MARKS)
// TODO:    {
// TODO:       fprintf(stderr, "no more marks allowed\n"); /* shouldn't happen */
// TODO:       return;
// TODO:    }
// TODO:    if (index == window->nMarks)
// TODO:       window->nMarks++;
// TODO: 
// TODO:    /* store the cursor location and selection position in the table */
// TODO:    window->markTable[index].label = label;
// TODO:    memcpy(&window->markTable[index].sel, &window->buffer->primary,
// TODO:           sizeof(selection));
// TODO:    window->markTable[index].cursorPos = TextGetCursorPos(widget);
// TODO: }
// TODO: 
// TODO: void GotoMark(WindowInfo* window, Widget w, char label, int extendSel)
// TODO: {
// TODO:    int index, oldStart, newStart, oldEnd, newEnd, cursorPos;
// TODO:    selection* sel, *oldSel;
// TODO: 
// TODO:    /* look up the mark in the mark table */
// TODO:    label = toupper(label);
// TODO:    for (index=0; index<window->nMarks; index++)
// TODO:    {
// TODO:       if (window->markTable[index].label == label)
// TODO:          break;
// TODO:    }
// TODO:    if (index == window->nMarks)
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* reselect marked the selection, and move the cursor to the marked pos */
// TODO:    sel = &window->markTable[index].sel;
// TODO:    oldSel = &window->buffer->primary;
// TODO:    cursorPos = window->markTable[index].cursorPos;
// TODO:    if (extendSel)
// TODO:    {
// TODO:       oldStart = oldSel->selected ? oldSel->start : TextGetCursorPos(w);
// TODO:       oldEnd = oldSel->selected ? oldSel->end : TextGetCursorPos(w);
// TODO:       newStart = sel->selected ? sel->start : cursorPos;
// TODO:       newEnd = sel->selected ? sel->end : cursorPos;
// TODO:       BufSelect(window->buffer, oldStart < newStart ? oldStart : newStart,
// TODO:                 oldEnd > newEnd ? oldEnd : newEnd);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if (sel->selected)
// TODO:       {
// TODO:          if (sel->rectangular)
// TODO:             BufRectSelect(window->buffer, sel->start, sel->end,
// TODO:                           sel->rectStart, sel->rectEnd);
// TODO:          else
// TODO:             BufSelect(window->buffer, sel->start, sel->end);
// TODO:       }
// TODO:       else
// TODO:          BufUnselect(window->buffer);
// TODO:    }
// TODO: 
// TODO:    /* Move the window into a pleasing position relative to the selection
// TODO:       or cursor.   MakeSelectionVisible is not great with multi-line
// TODO:       selections, and here we will sometimes give it one.  And to set the
// TODO:       cursor position without first using the less pleasing capability
// TODO:       of the widget itself for bringing the cursor in to view, you have to
// TODO:       first turn it off, set the position, then turn it back on. */
// TODO:    XtVaSetValues(w, textNautoShowInsertPos, false, NULL);
// TODO:    TextSetCursorPos(w, cursorPos);
// TODO:    MakeSelectionVisible(window, window->lastFocus);
// TODO:    XtVaSetValues(w, textNautoShowInsertPos, true, NULL);
// TODO: }

/*
** Keep the marks in the windows book-mark table up to date across
** changes to the underlying buffer
*/
void UpdateMarkTable(WindowInfo* window, int pos, int nInserted,
                     int nDeleted)
{
   int i;

   for (i=0; i<window->nMarks; i++)
   {
      maintainSelection(&window->markTable[i].sel, pos, nInserted,
                        nDeleted);
      maintainPosition(&window->markTable[i].cursorPos, pos, nInserted,
                       nDeleted);
   }
}


// Update a selection across buffer modifications specified by
// "pos", "nDeleted", and "nInserted".
static void maintainSelection(selection* sel, int pos, int nInserted, int nDeleted)
{
   if (!sel->selected || pos > sel->end)
      return;
   maintainPosition(&sel->start, pos, nInserted, nDeleted);
   maintainPosition(&sel->end, pos, nInserted, nDeleted);
   if (sel->end <= sel->start)
      sel->selected = false;
}


// Update a position across buffer modifications specified by
// "modPos", "nDeleted", and "nInserted".
static void maintainPosition(int* position, int modPos, int nInserted, int nDeleted)
{
   if (modPos > *position)
      return;
   if (modPos+nDeleted <= *position)
      *position += nInserted - nDeleted;
   else
      *position = modPos;
}
