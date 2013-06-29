static const char CVSID[] = "$Id: search.c,v 1.90 2008/10/06 04:40:42 ajbj Exp $";
/*******************************************************************************
*									       *
* search.c -- Nirvana Editor search and replace functions		       *
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

#include "../util/Ne_AppContext.h"

#include "search.h"
#include "regularExp.h"
#include "Ne_Text_Buffer.h"
#include "Ne_Text_Editor.h"
#include "nedit.h"
#include "server.h"
#include "window.h"
#include "userCmds.h"
#include "preferences.h"
#include "file.h"
#include "highlight.h"
#include "selection.h"
#include "../util/DialogF.h"
#include "../util/misc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifndef WIN32
#include <sys/param.h>
#endif // WIN32

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

#include <FL/fl_ask.H>
#include <FL/Fl_Multi_Browser.H>
#include <FL/Fl_Select_Browser.H>
#include "../util/Ne_Input.h"


int NHist = 0;

struct SelectionInfo
{
   int done;
   WindowInfo* window;
   char* selection;
};

struct SearchSelectedCallData
{
   int direction;
   int searchType;
   int searchWrap;
} ;

/* History mechanism for search and replace strings */
static char* SearchHistory[MAX_SEARCH_HISTORY];
static char* ReplaceHistory[MAX_SEARCH_HISTORY];
static int SearchTypeHistory[MAX_SEARCH_HISTORY];
static int HistStart = 0;

static bool textFieldNonEmpty(Fl_Input* w);
// TODO: static void setTextField(WindowInfo* window, Time time, Widget textField);
// TODO: static void getSelectionCB(Widget w, SelectionInfo* selectionInfo, Atom* selection, Atom* type, char* value, int* length, int* format);
static void rKeepCB(Fl_Widget* w, void* data);
static void fKeepCB(Fl_Widget* w, void* data);
static void replaceCB(Fl_Widget* w, void* data);
static void replaceAllCB(Fl_Widget* w, void* data);
static void rInSelCB(Fl_Widget* w, void* data);
static void rCancelCB(Fl_Widget* w, void* data);
static void fCancelCB(Fl_Widget* w, void* data);
static void rFindCB(Fl_Widget* w, void* data);
static void rFindTextValueChangedCB(Fl_Widget* w, void* data);
static void rFindArrowKeyCB(Fl_Widget* w, void* data);

static void rSetActionButtons(WindowInfo* window, int replaceBtn, int replaceFindBtn, int replaceAndFindBtn,
                              int replaceInWinBtn,
                              int replaceInSelBtn,
                              int replaceAllBtn);

static void replaceArrowKeyCB(Fl_Widget* w, void* data);
static void fUpdateActionButtons(WindowInfo* window);
static void findTextValueChangedCB(Fl_Widget* w, void* data);
static void findArrowKeyCB(Fl_Widget* w, void* data);
static void replaceFindCB(Fl_Widget* w, void* data);
static void findCB(Fl_Widget* w, void* data);
static void replaceMultiFileCB(Fl_Widget* w, void* data);
static void rMultiFileReplaceCB(Fl_Widget* w, void* data);
static void rMultiFileCancelCB(Fl_Widget* w, void* data);
static void rMultiFileSelectAllCB(Fl_Widget* w, void* data);
static void rMultiFileDeselectAllCB(Fl_Widget* w, void* data);
static void rMultiFilePathCB(Fl_Widget* w, void* data);
static void uploadFileListItems(WindowInfo* window, bool replace);
static int countWindows();
static int countWritableWindows();
static void collectWritableWindows(WindowInfo* window);
static void freeWritableWindowsCB(Fl_Widget* w, void* data);
static void checkMultiReplaceListForDoomedW(WindowInfo* window, WindowInfo* doomedWindow);
static void removeDoomedWindowFromList(WindowInfo* window, int index);
static void unmanageReplaceDialogs(const WindowInfo* window);
// TODO: static void flashTimeoutProc(XtPointer clientData, XtIntervalId* id);
static void eraseFlash(WindowInfo* window);
static int getReplaceDlogInfo(WindowInfo* window, int* direction, char* searchString, char* replaceString, int* searchType);
static int getFindDlogInfo(WindowInfo* window, int* direction, char* searchString, int* searchType);
static void selectedSearchCB(WindowInfo* window, const SearchSelectedCallData& callDataItems);
// TODO: static void iSearchTextClearAndPasteAP(Widget w, XEvent* event, String* args,
// TODO:                                        Cardinal* nArg);
// TODO: static void iSearchTextClearCB(Widget w, WindowInfo* window,
// TODO:                                XmAnyCallbackStruct* callData);
// TODO: static void iSearchTextActivateCB(Widget w, WindowInfo* window,
// TODO:                                   XmAnyCallbackStruct* callData);
// TODO: static void iSearchTextValueChangedCB(Widget w, WindowInfo* window,
// TODO:                                       XmAnyCallbackStruct* callData);
// TODO: static void iSearchTextKeyEH(Widget w, WindowInfo* window,
// TODO:                              XKeyEvent* event, bool* continueDispatch);
static int searchLiteral(const char* string, const char* searchString, int caseSense,
                         int direction, int wrap, int beginPos, int* startPos, int* endPos,
                         int* searchExtentBW, int* searchExtentFW);
static int searchLiteralWord(const char* string, const char* searchString, int caseSense,
                             int direction, int wrap, int beginPos, int* startPos, int* endPos,
                             const char* delimiters);
static int searchRegex(const char* string, const char* searchString, int direction,
                       int wrap, int beginPos, int* startPos, int* endPos, int* searchExtentBW,
                       int* searchExtentFW, const char* delimiters, int defaultFlags);
static int forwardRegexSearch(const char* string, const char* searchString, int wrap,
                              int beginPos, int* startPos, int* endPos, int* searchExtentBW,
                              int* searchExtentFW, const char* delimiters, int defaultFlags);
static int backwardRegexSearch(const char* string, const char* searchString, int wrap,
                               int beginPos, int* startPos, int* endPos, int* searchExtentBW,
                               int* searchExtentFW, const char* delimiters, int defaultFlags);
static void upCaseString(char* outString, const char* inString);
static void downCaseString(char* outString, const char* inString);
static void resetFindTabGroup(WindowInfo* window);
static void resetReplaceTabGroup(WindowInfo* window);
static int searchMatchesSelection(WindowInfo* window, const char* searchString, int searchType, 
                                  int* left, int* right, int* searchExtentBW, int* searchExtentFW);
static int findMatchingChar(WindowInfo* window, char toMatch,
                            void* toMatchStyle, int charPos, int startLimit, int endLimit,
                            int* matchPos);
static bool replaceUsingRE(const char* searchStr, const char* replaceStr,
                              const char* sourceStr, int beginPos, char* destStr, int maxDestLen,
                              int prevChar, const char* delimiters, int defaultFlags);
static void saveSearchHistory(const char* searchString, const char* replaceString, int searchType, int isIncremental);
static int historyIndex(int nCycles);
static const char* searchTypeArg(int searchType);
static const char* searchWrapArg(int searchWrap);
static const char* directionArg(int direction);
static int isRegexType(int searchType);
static int defaultRegexFlags(int searchType);
static void findRegExpToggleCB(Fl_Widget* w, void* data);
static void replaceRegExpToggleCB(Fl_Widget* w, void* data);
// TODO: static void iSearchRegExpToggleCB(Widget w, XtPointer clientData,
// TODO:                                   XtPointer callData);
static void findCaseToggleCB(Fl_Widget* w, void* data);
static void replaceCaseToggleCB(Fl_Widget* w, void* data);
// TODO: static void iSearchCaseToggleCB(Widget w, XtPointer clientData,
// TODO:                                 XtPointer callData);
static void iSearchTryBeepOnWrap(WindowInfo* window, int direction, int beginPos, int startPos);
static void iSearchRecordLastBeginPos(WindowInfo* window, int direction, int initPos);
static bool prefOrUserCancelsSubst(Fl_Widget* parent);

typedef struct _charMatchTable
{
   char c;
   char match;
   char direction;
} charMatchTable;

#define N_MATCH_CHARS 13
#define N_FLASH_CHARS 6
static charMatchTable MatchingChars[N_MATCH_CHARS] =
{
   {'{', '}', SEARCH_FORWARD},
   {'}', '{', SEARCH_BACKWARD},
   {'(', ')', SEARCH_FORWARD},
   {')', '(', SEARCH_BACKWARD},
   {'[', ']', SEARCH_FORWARD},
   {']', '[', SEARCH_BACKWARD},
   {'<', '>', SEARCH_FORWARD},
   {'>', '<', SEARCH_BACKWARD},
   {'/', '/', SEARCH_FORWARD},
   {'"', '"', SEARCH_FORWARD},
   {'\'', '\'', SEARCH_FORWARD},
   {'`', '`', SEARCH_FORWARD},
   {'\\', '\\', SEARCH_FORWARD},
};

/*
** Definitions for the search method strings, used as arguments for
** macro search subroutines and search action routines
*/
static const char* searchTypeStrings[] =
{
   "literal",          /* SEARCH_LITERAL         */
   "case",             /* SEARCH_CASE_SENSE      */
   "regex",            /* SEARCH_REGEX           */
   "word",             /* SEARCH_LITERAL_WORD    */
   "caseWord",         /* SEARCH_CASE_SENSE_WORD */
   "regexNoCase",      /* SEARCH_REGEX_NOCASE    */
   NULL
};

/*
** Window for which a search dialog callback is currently active. That window
** cannot be safely closed because the callback would access the armed button
** after it got destroyed.
** Note that an attempt to close such a window can only happen if the search
** action triggers a modal dialog and the user tries to close the window via
** the window manager without dismissing the dialog first. It is up to the
** close callback of the window to intercept and reject the request by calling
** the WindowCanBeClosed() function.
*/
static WindowInfo* windowNotToClose = NULL;

// TODO: bool WindowCanBeClosed(WindowInfo* window)
// TODO: {
// TODO:    if (windowNotToClose &&
// TODO:          GetTopDocument(window->mainWindow) ==
// TODO:          GetTopDocument(windowNotToClose->mainWindow))
// TODO:    {
// TODO:       return false;
// TODO:    }
// TODO:    return true; /* It's safe */
// TODO: }

/*
** Shared routine for replace and find dialogs and i-search bar to initialize
** the state of the regex/case/word toggle buttons, and the sticky case
** sensitivity states.
*/
static void initToggleButtons(int searchType, Fl_Button* regexToggle,
                              Fl_Button* caseToggle, Fl_Button* wordToggle,
                              bool* lastLiteralCase,
                              bool* lastRegexCase)
{
   /* Set the initial search type and remember the corresponding case
      sensitivity states in case sticky case sensitivity is required. */
   switch (searchType)
   {
   case SEARCH_LITERAL:
      *lastLiteralCase = false;
      *lastRegexCase   = true;
      NeToggleButtonSetState(regexToggle, false, false);
      NeToggleButtonSetState(caseToggle,  false, false);
      if (wordToggle)
      {
         NeToggleButtonSetState(wordToggle, false, false);
         NeSetSensitive(wordToggle, true);
      }
      break;
   case SEARCH_CASE_SENSE:
      *lastLiteralCase = true;
      *lastRegexCase   = true;
      NeToggleButtonSetState(regexToggle, false, false);
      NeToggleButtonSetState(caseToggle,  true,  false);
      if (wordToggle)
      {
         NeToggleButtonSetState(wordToggle, false, false);
         NeSetSensitive(wordToggle, true);
      }
      break;
   case SEARCH_LITERAL_WORD:
      *lastLiteralCase = false;
      *lastRegexCase   = true;
      NeToggleButtonSetState(regexToggle, false, false);
      NeToggleButtonSetState(caseToggle, false, false);
      if (wordToggle)
      {
         NeToggleButtonSetState(wordToggle,  true,  false);
         NeSetSensitive(wordToggle, true);
      }
      break;
   case SEARCH_CASE_SENSE_WORD:
      *lastLiteralCase = true;
      *lastRegexCase   = true;
      NeToggleButtonSetState(regexToggle, false, false);
      NeToggleButtonSetState(caseToggle,  true,  false);
      if (wordToggle)
      {
         NeToggleButtonSetState(wordToggle,  true,  false);
         NeSetSensitive(wordToggle, true);
      }
      break;
   case SEARCH_REGEX:
      *lastLiteralCase = false;
      *lastRegexCase   = true;
      NeToggleButtonSetState(regexToggle, true,  false);
      NeToggleButtonSetState(caseToggle,  true,  false);
      if (wordToggle)
      {
         NeToggleButtonSetState(wordToggle,  false, false);
         NeSetSensitive(wordToggle, false);
      }
      break;
   case SEARCH_REGEX_NOCASE:
      *lastLiteralCase = false;
      *lastRegexCase   = false;
      NeToggleButtonSetState(regexToggle, true,  false);
      NeToggleButtonSetState(caseToggle,  false, false);
      if (wordToggle)
      {
         NeToggleButtonSetState(wordToggle,  false, false);
         NeSetSensitive(wordToggle, false);
      }
      break;
   }
}

void DoFindReplaceDlog(WindowInfo* window, int direction, int keepDialogs, int searchType, double time)
{

   /* Create the dialog if it doesn't already exist */
   if (window->replaceDlog == NULL)
      CreateReplaceDlog(window->mainWindow, window);

// TODO:    setTextField(window, time, window->replaceText);
// TODO: 
// TODO:    /* If the window is already up, just pop it to the top */
// TODO:    if (XtIsManaged(window->replaceDlog))
// TODO:    {
// TODO:       RaiseDialogWindow(XtParent(window->replaceDlog));
// TODO:       return;
// TODO:    }

   window->replaceDlog->show();

// TODO:    /* Blank the Replace with field */
// TODO:    NeTextSetString(window->replaceWithText, "");
// TODO: 
// TODO:    /* Set the initial search type */
// TODO:    initToggleButtons(searchType, window->replaceRegexToggle,
// TODO:                      window->replaceCaseToggle, &window->replaceWordToggle,
// TODO:                      &window->replaceLastLiteralCase,
// TODO:                      &window->replaceLastRegexCase);
// TODO: 
// TODO:    /* Set the initial direction based on the direction argument */
// TODO:    NeToggleButtonSetState(window->replaceRevToggle,
// TODO:                           direction == SEARCH_FORWARD ? false: true, true);
// TODO: 
// TODO:    /* Set the state of the Keep Dialog Up button */
// TODO:    NeToggleButtonSetState(window->replaceKeepBtn, keepDialogs, true);
// TODO: 
// TODO:    UpdateReplaceActionButtons(window);
// TODO: 
// TODO:    /* Start the search history mechanism at the current history item */
// TODO:    window->rHistIndex = 0;
// TODO: 
// TODO:    /* Display the dialog */
// TODO:    ManageDialogCenteredOnPointer(window->replaceDlog);
// TODO: 
// TODO:    /* Workaround: LessTif (as of version 0.89) needs reminding of who had
// TODO:       the focus when the dialog was unmanaged.  When re-managed, focus is
// TODO:       lost and events fall through to the window below. */
// TODO:    XmProcessTraversal(window->replaceText, XmTRAVERSE_CURRENT);
}

// TODO: static void setTextField(WindowInfo* window, Time time, Widget textField)
// TODO: {
// TODO:    XEvent nextEvent;
// TODO:    char* primary_selection = 0;
// TODO:    SelectionInfo* selectionInfo = XtNew(SelectionInfo);
// TODO: 
// TODO:    if (GetPrefFindReplaceUsesSelection())
// TODO:    {
// TODO:       selectionInfo->done = 0;
// TODO:       selectionInfo->window = window;
// TODO:       selectionInfo->selection = 0;
// TODO:       XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
// TODO:                           (XtSelectionCallbackProc)getSelectionCB, selectionInfo, time);
// TODO:       while (selectionInfo->done == 0)
// TODO:       {
// TODO:          XtAppNextEvent(XtWidgetToApplicationContext(window->textArea), &nextEvent);
// TODO:          ServerDispatchEvent(&nextEvent);
// TODO:       }
// TODO:       primary_selection = selectionInfo->selection;
// TODO:    }
// TODO:    if (primary_selection == 0)
// TODO:    {
// TODO:       primary_selection = NeNewString("");
// TODO:    }
// TODO: 
// TODO:    /* Update the field */
// TODO:    NeTextSetString(textField, primary_selection);
// TODO: 
// TODO:    XtFree(primary_selection);
// TODO:    XtFree((char*)selectionInfo);
// TODO: }
// TODO: 
// TODO: static void getSelectionCB(Widget w, SelectionInfo* selectionInfo, Atom* selection,
// TODO:                            Atom* type, char* value, int* length, int* format)
// TODO: {
// TODO:    WindowInfo* window = selectionInfo->window;
// TODO: 
// TODO:    /* return an empty string if we can't get the selection data */
// TODO:    if (*type == XT_CONVERT_FAIL || *type != XA_STRING || value == NULL || *length == 0)
// TODO:    {
// TODO:       XtFree(value);
// TODO:       selectionInfo->selection = 0;
// TODO:       selectionInfo->done = 1;
// TODO:       return;
// TODO:    }
// TODO:    /* return an empty string if the data is not of the correct format. */
// TODO:    if (*format != 8)
// TODO:    {
// TODO:       DialogF(DF_WARN, window->mainWindow, 1, "Invalid Format",
// TODO:               "NEdit can't handle non 8-bit text", "OK");
// TODO:       XtFree(value);
// TODO:       selectionInfo->selection = 0;
// TODO:       selectionInfo->done = 1;
// TODO:       return;
// TODO:    }
// TODO:    selectionInfo->selection = malloc__(*length+1);
// TODO:    memcpy(selectionInfo->selection, value, *length);
// TODO:    selectionInfo->selection[*length] = 0;
// TODO:    XtFree(value);
// TODO:    selectionInfo->done = 1;
// TODO: }

void DoFindDlog(WindowInfo* window, int direction, int keepDialogs, int searchType, int time)
{
   TRACE();

   /* Create the dialog if it doesn't already exist */
   if (window->findDlog == NULL)
      CreateFindDlog(window->mainWindow, window);
   
// TODO:    setTextField(window, time, window->findText);
// TODO: 
// TODO:    /* If the window is already up, just pop it to the top */
// TODO:    if (XtIsManaged(window->findDlog))
// TODO:    {
// TODO:       RaiseDialogWindow(XtParent(window->findDlog));
// TODO:       return;
// TODO:    }

   window->findDlog->show();

   /* Set the initial search type */
   initToggleButtons(searchType, window->findRegexToggle,
                     window->findCaseToggle, window->findWordToggle,
                     &window->findLastLiteralCase,
                     &window->findLastRegexCase);

   /* Set the initial direction based on the direction argument */
   NeToggleButtonSetState(window->findRevToggle,
                          direction == SEARCH_FORWARD ? false : true, true);

   /* Set the state of the Keep Dialog Up button */
   NeToggleButtonSetState(window->findKeepBtn, keepDialogs, true);

   /* Set the state of the Find button */
   fUpdateActionButtons(window);

   /* start the search history mechanism at the current history item */
   window->fHistIndex = 0;

   /* Display the dialog */
   ManageDialogCenteredOnPointer(window->findDlog);

// TODO:    /* Workaround: LessTif (as of version 0.89) needs reminding of who had
// TODO:       the focus when the dialog was unmanaged.  When re-managed, focus is
// TODO:       lost and events fall through to the window below. */
// TODO:    XmProcessTraversal(window->findText, XmTRAVERSE_CURRENT);
}

void DoReplaceMultiFileDlog(WindowInfo* window)
{
   char	searchString[SEARCHMAX], replaceString[SEARCHMAX];
   int		direction, searchType;

   /* Validate and fetch the find and replace strings from the dialog */
   if (!getReplaceDlogInfo(window, &direction, searchString, replaceString, &searchType))
      return;

   /* Don't let the user select files when no replacement can be made */
   if (*searchString == '\0')
   {
      /* Set the initial focus of the dialog back to the search string */
      resetReplaceTabGroup(window);
      /* pop down the replace dialog */
      if (!NeToggleButtonGetState(window->replaceKeepBtn))
         unmanageReplaceDialogs(window);
      return;
   }

   /* Create the dialog if it doesn't already exist */
   if (window->replaceMultiFileDlog == NULL)
      CreateReplaceMultiFileDlog(window);

   /* Raising the window doesn't make sense. It is modal, so we
      can't get here unless it is unmanaged */
   /* Prepare a list of writable windows */
   collectWritableWindows(window);

   /* Initialize/update the list of files. */
   uploadFileListItems(window, false);

   /* Display the dialog */
   ManageDialogCenteredOnPointer(window->replaceMultiFileDlog);

   window->replaceMultiFileDlog->show();
   while (window->replaceMultiFileDlog->shown()) Fl::wait();

   freeWritableWindowsCB(window->replaceMultiFileDlog, window);
}

/*
** If a window is closed (possibly via the window manager) while it is on the
** multi-file replace dialog list of any other window (or even the same one),
** we must update those lists or we end up with dangling references.
** Normally, there can be only one of those dialogs at the same time
** (application modal), but Lesstif doesn't (always) honor application
** modalness, so there can be more than one dialog.
*/
void RemoveFromMultiReplaceDialog(WindowInfo* doomedWindow)
{
   WindowInfo* w;

   for (w=WindowList; w!=NULL; w=w->next)
      if (w->writableWindows)
         /* A multi-file replacement dialog is up for this window */
         checkMultiReplaceListForDoomedW(w, doomedWindow);
}

void CreateReplaceDlog(Fl_Widget* parent, WindowInfo* window)
{
   window->replaceDlog = new Fl_Double_Window(500, 255, "Replace/Find");
   if (GetPrefKeepSearchDlogs())
   {
      char 	title[MAXPATHLEN + 19];
      sprintf(title, "Replace/Find (in %s)", window->filename);
      window->replaceDlog->copy_label(title);
   }

   window->replaceText = new Ne_Input(5,25, 490, 25, "S&tring to Find");
   window->replaceText->align(FL_ALIGN_TOP_LEFT);
   window->replaceText->when(FL_WHEN_CHANGED | FL_WHEN_RELEASE_ALWAYS);
   window->replaceText->callback(rFindTextValueChangedCB, window); // include rFindArrowKeyCB

   Fl_Box* helpLabel = new Fl_Box(380, 5, 105, 25, "(use up arrow key to recall previous)");
   helpLabel->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

   window->replaceWithText = new Ne_Input(5,80, 490, 25, "Replace &With:");
   window->replaceWithText->align(FL_ALIGN_TOP_LEFT);
   window->replaceWithText->when(FL_WHEN_CHANGED | FL_WHEN_RELEASE_ALWAYS);
   window->replaceWithText->callback(replaceArrowKeyCB, window);

   Fl_Group* searchTypeBox = new Fl_Group(5, 110, 500, 30);
   window->replaceRegexToggle = new Fl_Check_Button(5, 115, 150, 25, "&Regular Expression");
   window->replaceRegexToggle->callback(replaceRegExpToggleCB, window);

   window->replaceCaseToggle = new Fl_Check_Button(160, 115, 120, 25, "&Case Sensitive");
   window->replaceCaseToggle->callback(replaceCaseToggleCB, window);

   window->replaceWordToggle = new Fl_Check_Button(280, 115, 150, 25, "W&hole Word");
   searchTypeBox->end();

   window->replaceRevToggle = new Fl_Check_Button(5, 145, 180, 25, "Search &Backward");
   window->replaceKeepBtn = new Fl_Check_Button(400, 145, 100, 25, "&Keep Dialog");
   window->replaceKeepBtn->callback(rKeepCB, window);

   Fl_Box* allLabel = new Fl_Box(5, 175, 115, 25, "Replace all in:");
   allLabel->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

   window->replaceInWinBtn = new Fl_Button(120, 175, 70, 25, "W&indow");
   window->replaceInWinBtn->callback(replaceAllCB, window);
   
   window->replaceInSelBtn = new Fl_Button(195, 175, 80, 25, "&Selection");
   window->replaceInSelBtn->callback(rInSelCB, window);

   window->replaceAllBtn = new Fl_Button(285, 175, 150, 25, "&Multiple Documents...");
   window->replaceAllBtn->callback(replaceMultiFileCB, window);

   Fl_Group* buttonLine = new Fl_Group(0, 205, 500, 50);
   buttonLine->box(FL_ENGRAVED_FRAME);

   window->replaceBtn = new Fl_Button(25, 215, 100, 25, "Replace");
   window->replaceBtn->callback(replaceCB, window);

   window->replaceFindBtn = new Fl_Button(140, 215, 100, 25, "&Find");
   window->replaceFindBtn->callback(rFindCB, window);

   window->replaceAndFindBtn = new Fl_Button(255, 215, 100, 25, "Replace & Fi&nd");
   window->replaceAndFindBtn->callback(replaceFindCB, window);

   Fl_Button* cancelButton = new Fl_Button(370, 215, 100, 25, "Cancel");
   cancelButton->callback(rCancelCB, window);

   buttonLine->end();
}

void CreateFindDlog(Fl_Widget* parent, WindowInfo* window)
{
   char 	title[MAXPATHLEN + 11];

   Fl_Double_Window* form = new Fl_Double_Window(500, 170, "Find");
   if (GetPrefKeepSearchDlogs())
   {
      sprintf(title, "Find (in %s)", window->filename);
      form->copy_label(title);
   }

   Fl_Input* findText = new Ne_Input(5,25, 490, 25, "&String to Find");
   findText->align(FL_ALIGN_TOP_LEFT);
   findText->when(FL_WHEN_CHANGED | FL_WHEN_RELEASE_ALWAYS);
   findText->callback(findTextValueChangedCB, window); // include findArrowKeyCB

   Fl_Box* helpLabel = new Fl_Box(380, 5, 105, 25, "(use up arrow key to recall previous)");
   helpLabel->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

   Fl_Group* searchTypeBox = new Fl_Group(5, 50, 500, 30);
   window->findRegexToggle = new Fl_Check_Button(5, 55, 150, 25, "&Regular Expression");
   window->findRegexToggle->callback(findRegExpToggleCB, window);

   window->findCaseToggle = new Fl_Check_Button(160, 55, 120, 25, "&Case Sensitive");
   window->findCaseToggle->callback(findCaseToggleCB, window);

   window->findWordToggle = new Fl_Check_Button(280, 55, 150, 25, "W&hole Word");
   searchTypeBox->end();

   Fl_Check_Button* reverseBtn = new Fl_Check_Button(5, 85, 180, 25, "Search &Backward");
   Fl_Check_Button* keepBtn = new Fl_Check_Button(400, 85, 100, 25, "&Keep Dialog");
   keepBtn->callback(fKeepCB, window);

   Fl_Group* buttonLine = new Fl_Group(0, 120, 500, 50);
   buttonLine->box(FL_ENGRAVED_FRAME);

   Fl_Button* findBtn = new Fl_Button(100, 135, 80, 25, "Find");
   findBtn->shortcut(FL_Enter);
   findBtn->callback(findCB, window);

   Fl_Button* btnCancel = new Fl_Button(300, 135, 80, 25, "Cancel");
   btnCancel->shortcut(FL_Escape);
   btnCancel->callback(fCancelCB, window);

   buttonLine->end();

   form->resizable(searchTypeBox);
   form->size_range(form->w(), form->h());

   window->findDlog = form;
   window->findText = findText;
   window->findRevToggle = reverseBtn;
   window->findKeepBtn = keepBtn;
   window->findBtn = findBtn;
}

void CreateReplaceMultiFileDlog(WindowInfo* window)
{
   /* Ideally, we should create the multi-file dialog as a child widget
      of the replace dialog. However, if we do this, the main window
      can hide the multi-file dialog when raised (I'm not sure why, but
      it's something that I observed with fvwm). By using the main window
      as the parent, it is possible that the replace dialog _partially_
      covers the multi-file dialog, but this much better than the multi-file
      dialog being covered completely by the main window */
   Fl_Double_Window* form = new Fl_Double_Window(30,50, 400, 400, "Replace All in Multiple Documents");
   form->set_modal();

   Fl_Browser* list = new Fl_Multi_Browser(5, 25, 390, 325, "Files in which to Replace All:");
   list->align(FL_ALIGN_TOP_LEFT);
   form->resizable(list);

   /* Pathname toggle button at top right (always unset by default) */
   Fl_Check_Button* pathBtn = new Fl_Check_Button(260, 5, 140, 25, "Show &Path Names");
   pathBtn->callback(rMultiFilePathCB, window);

   Fl_Button* replaceBtn = new Fl_Button(5, 365, 90, 25, "&Replace");
   replaceBtn->callback(rMultiFileReplaceCB, window);

   Fl_Button* selectAllBtn = new Fl_Button(105, 365, 90, 25, "&Select All");
   selectAllBtn->callback(rMultiFileSelectAllCB, window);

   Fl_Button* deselectAllBtn = new Fl_Button(205, 365, 90, 25, "&Deselect All");
   deselectAllBtn->callback(rMultiFileDeselectAllCB, window);

   Fl_Button* cancelBtn = new Fl_Button(305, 365, 90, 25, "&Cancel");
   cancelBtn->shortcut(FL_Escape);
   cancelBtn->callback(rMultiFileCancelCB, window);

   window->replaceMultiFileDlog = form;
   window->replaceMultiFileList = list;
   window->replaceMultiFilePathBtn = pathBtn;
}

/*
** Iterates through the list of writable windows of a window, and removes
** the doomed window if necessary.
*/
static void checkMultiReplaceListForDoomedW(WindowInfo* window, WindowInfo* doomedWindow)
{
   /* If the window owning the list and the doomed window are one and the
      same, we just close the multi-file replacement dialog. */
   if (window == doomedWindow)
   {
      window->replaceMultiFileDlog->hide();
      return;
   }

   /* Check whether the doomed window is currently listed */
   for (int i = 0; i < window->nWritableWindows; ++i)
   {
      WindowInfo* w = window->writableWindows[i];
      if (w == doomedWindow)
      {
         removeDoomedWindowFromList(window, i);
         break;
      }
   }
}

/*
** Removes a window that is about to be closed from the list of files in
** which to replace. If the list becomes empty, the dialog is popped down.
*/
static void removeDoomedWindowFromList(WindowInfo* window, int index)
{
   int       entriesToMove;

   /* If the list would become empty, we remove the dialog */
   if (window->nWritableWindows <= 1)
   {
      window->replaceMultiFileDlog->hide();
      return;
   }

   entriesToMove = window->nWritableWindows - index - 1;
   memmove(&(window->writableWindows[index]),
           &(window->writableWindows[index+1]),
           (size_t)(entriesToMove*sizeof(WindowInfo*)));
   window->nWritableWindows -= 1;

// TODO:    XmListDeletePos(window->replaceMultiFileList, index + 1);
}

/* when keeping a window up, clue the user what window it's associated with */
static void rKeepCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);
   if (NeToggleButtonGetState(window->replaceKeepBtn))
   {
      char title[MAXPATHLEN + 19];
      sprintf(title, "Replace/Find (in %s)", window->filename);
      window->replaceDlog->copy_label(title);
   }
   else
      window->replaceDlog->copy_label("Replace/Find");
}

static void fKeepCB(Fl_Widget* w, void* data)
{
   TRACE();
   WindowInfo* window = static_cast<WindowInfo*>(data);
   if (NeToggleButtonGetState(window->findKeepBtn))
   {
      char title[MAXPATHLEN + 11];
      sprintf(title, "Find (in %s)", window->filename);
      window->findDlog->copy_label(title);
   }
   else
      window->findDlog->copy_label("Find");
}

static void replaceCB(Fl_Widget* w, void* data)
{
   char searchString[SEARCHMAX], replaceString[SEARCHMAX];
   int direction, searchType;
   const char* params[5];

   WindowInfo* window = static_cast<WindowInfo*>(data);

   /* Validate and fetch the find and replace strings from the dialog */
   if (!getReplaceDlogInfo(window, &direction, searchString, replaceString, &searchType))
      return;

   /* Set the initial focus of the dialog back to the search string */
   resetReplaceTabGroup(window);

   /* Find the text and replace it */
   params[0] = searchString;
   params[1] = replaceString;
   params[2] = directionArg(direction);
   params[3] = searchTypeArg(searchType);
   params[4] = searchWrapArg(GetPrefSearchWraps());
   windowNotToClose = window;
   AppContext.callAction(window->lastFocus, "replace", Fl::event(), params, 5);
   windowNotToClose = NULL;

   /* Pop down the dialog */
   if (!NeToggleButtonGetState(window->replaceKeepBtn))
      unmanageReplaceDialogs(window);
}

static void replaceAllCB(Fl_Widget* w, void* data)
{
   char searchString[SEARCHMAX], replaceString[SEARCHMAX];
   int direction, searchType;
   const char* params[3];

   WindowInfo* window = static_cast<WindowInfo*>(data);

   /* Validate and fetch the find and replace strings from the dialog */
   if (!getReplaceDlogInfo(window, &direction, searchString, replaceString, &searchType))
      return;

   /* Set the initial focus of the dialog back to the search string	*/
   resetReplaceTabGroup(window);

   /* do replacement */
   params[0] = searchString;
   params[1] = replaceString;
   params[2] = searchTypeArg(searchType);
   windowNotToClose = window;
   AppContext.callAction(window->lastFocus, "replace_all",
      Fl::event(), params, 3);
   windowNotToClose = NULL;

   /* pop down the dialog */
   if (!NeToggleButtonGetState(window->replaceKeepBtn))
      unmanageReplaceDialogs(window);
}

static void replaceMultiFileCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);
   DoReplaceMultiFileDlog(window);
}

/*
** Callback that frees the list of windows the multi-file replace
** dialog is unmapped.
**/
static void freeWritableWindowsCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);
   delete[] window->writableWindows;
   window->writableWindows = NULL;
   window->nWritableWindows = 0;
}

/*
** Comparison function for sorting windows by title for the window menu
*/
static int compareWindowNames(const void* windowA, const void* windowB)
{
   const WindowInfo* a = *((WindowInfo**)windowA);
   const WindowInfo* b = *((WindowInfo**)windowB);

   return strcmp(a->filename, b->filename);
}

/*
** Count no. of windows
*/
static int countWindows()
{
   int nWindows;
   const WindowInfo* w;

   for (w=WindowList, nWindows=0; w!=NULL; w=w->next, ++nWindows);

   return nWindows;
}

/*
** Count no. of writable windows, but first update the status of all files.
*/
static int countWritableWindows()
{
   int nWritable, nBefore, nAfter;
   WindowInfo* w;

   nBefore = countWindows();
   for (w=WindowList, nWritable=0; w!=NULL; w=w->next)
   {
      /* We must be very careful! The status check may trigger a pop-up
         dialog when the file has changed on disk, and the user may destroy
         arbitrary windows in response. */
      CheckForChangesToFile(w);
      nAfter = countWindows();
      if (nAfter != nBefore)
      {
         /* The user has destroyed a file; start counting all over again */
         nBefore = nAfter;
         w = WindowList;
         nWritable = 0;
         continue;
      }
      if (!IS_ANY_LOCKED(w->lockReasons)) ++nWritable;
   }
   return nWritable;
}

/*
** Collects a list of writable windows (sorted by file name).
** The previous list, if any is freed first.
**/
static void collectWritableWindows(WindowInfo* window)
{
   int nWritable = countWritableWindows();
   int i;
   WindowInfo* w;

   delete[] window->writableWindows;

   /* Make a sorted list of writable windows */
   WindowInfo** windows = new WindowInfo*[nWritable];
   for (w=WindowList, i=0; w!=NULL; w=w->next)
      if (!IS_ANY_LOCKED(w->lockReasons)) windows[i++] = w;
   qsort(windows, nWritable, sizeof(WindowInfo*), compareWindowNames);

   window->writableWindows = windows;
   window->nWritableWindows = nWritable;
}

static void rMultiFileReplaceCB(Fl_Widget* w, void* data)
{
   char 	searchString[SEARCHMAX], replaceString[SEARCHMAX];
   int 	direction, searchType;
   const char*	 params[4];
   int 	nSelected, i;
   WindowInfo*	 writableWin;
   bool 	replaceFailed, noWritableLeft;

   WindowInfo* window = static_cast<WindowInfo*>(data);
   nSelected = 0;
   for (i=0;i<window->nWritableWindows; ++i)
      if (window->replaceMultiFileList->selected(i))
         ++nSelected;

   if (!nSelected)
   {
      DialogF(DF_INF, window->replaceMultiFileDlog, 1, "No Files", "No files selected!", "OK");
      return; /* Give the user another chance */
   }

   /* Set the initial focus of the dialog back to the search string */
   resetReplaceTabGroup(window);

   /*
    * Protect the user against him/herself; Maybe this is a bit too much?
    */
   if (DialogF(DF_QUES, window->mainWindow, 2, "Multi-File Replacement",
               "Multi-file replacements are difficult to undo.\n"
               "Proceed with the replacement ?", "Yes", "Cancel") != 1)
   {
      /* pop down the multi-file dialog only */
      window->replaceMultiFileDlog->hide();
      return;
   }

   /* Fetch the find and replace strings from the dialog;
      they should have been validated already, but since Lesstif may not
      honor modal dialogs, it is possible that the user modified the
      strings again, so we should verify them again too. */
   if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
                           &searchType))
      return;

   /* Set the initial focus of the dialog back to the search string */
   resetReplaceTabGroup(window);

   params[0] = searchString;
   params[1] = replaceString;
   params[2] = searchTypeArg(searchType);

   replaceFailed = true;
   noWritableLeft = true;
   /* Perform the replacements and mark the selected files (history) */
   for (i=0; i<window->nWritableWindows; ++i)
   {
      writableWin = window->writableWindows[i];
      if (window->replaceMultiFileList->selected(i))
      {
         /* First check again whether the file is still writable. If the
            file status has changed or the file was locked in the mean time
            (possible due to Lesstif modal dialog bug), we just skip the
            window. */
         if (!IS_ANY_LOCKED(writableWin->lockReasons))
         {
            noWritableLeft = false;
            writableWin->multiFileReplSelected = true;
            writableWin->multiFileBusy = true; /* Avoid multi-beep/dialog */
            writableWin->replaceFailed = false;
            AppContext.callAction(writableWin->lastFocus, "replace_all", Fl::event(), params, 3);
            writableWin->multiFileBusy = false;
            if (!writableWin->replaceFailed)
               replaceFailed = false;
         }
      }
      else
      {
         writableWin->multiFileReplSelected = false;
      }
   }

   if (!NeToggleButtonGetState(window->replaceKeepBtn))
   {
      /* Pop down both replace dialogs. */
      unmanageReplaceDialogs(window);
   }
   else
   {
      /* pow down only the file selection dialog */
      window->replaceMultiFileDlog->hide();
   }

   /* We suppressed multiple beeps/dialogs. If there wasn't any file in
      which the replacement succeeded, we should still warn the user */
   if (replaceFailed)
   {
      if (GetPrefSearchDlogs())
      {
         if (noWritableLeft)
         {
            DialogF(DF_INF, window->mainWindow, 1, "Read-only Files", "All selected files have become read-only.", "OK");
         }
         else
         {
            DialogF(DF_INF, window->mainWindow, 1, "String not found", "String was not found", "OK");
         }
      }
      else
      {
         fl_beep();
      }
   }
}

static void rMultiFileCancelCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);

   /* Set the initial focus of the dialog back to the search string	*/
   resetReplaceTabGroup(window);

   /* pop down the multi-window replace dialog */
   window->replaceMultiFileDlog->hide();
}

static void rMultiFileSelectAllCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);

   for (int i=0; i<window->nWritableWindows; ++i)
      window->replaceMultiFileList->select(i+1);
}

static void rMultiFileDeselectAllCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);
   for (int i=0; i<window->nWritableWindows; ++i)
      window->replaceMultiFileList->select(i+1, 0);
}

static void rMultiFilePathCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);
   uploadFileListItems(window, true);  /* Replace */
}

/*
 * Uploads the file items to the multi-file replacement dialog list.
 * A boolean argument indicates whether the elements currently in the
 * list have to be replaced or not.
 * Depending on the state of the "Show path names" toggle button, either
 * the file names or the path names are listed.
 */
static void uploadFileListItems(WindowInfo* window, bool replace)
{
   int           nWritable, i, *selected, selectedCount;
   char          buf[MAXPATHLEN+1], policy;
   WindowInfo*    w;
   Fl_Browser*        list;
   
   std::vector<std::string> names;

   nWritable = window->nWritableWindows;
   list = window->replaceMultiFileList;

   bool usePathNames = NeToggleButtonGetState(window->replaceMultiFilePathBtn);

   /* Note: the windows are sorted alphabetically by _file_ name. This
            order is _not_ changed when we switch to path names. That
            would be confusing for the user */

   for (i = 0; i < nWritable; ++i)
   {
      w = window->writableWindows[i];
      if (usePathNames && window->filenameSet)
      {
         sprintf(buf, "%s%s", w->path, w->filename);
      }
      else
      {
         sprintf(buf, "%s", w->filename);
      }
      names.push_back(buf);
   }

   /*
    * If the list is in extended selection mode, we can't pre-select
    * more than one item in (probably because XmListSelectPos is
    * equivalent to a button1 click; I don't think that there is an
    * equivalent for CTRL-button1). Therefore, we temporarily put the
    * list into multiple selection mode.
    */
   if (replace)
   {
      for(int i = 0; i < list->size() && i < names.size(); ++i)
         list->text(i+1, names[i].c_str());
   }
   else
   {
      int nVisible;
      int firstSelected = 0;

      /* Remove the old list, if any */
      list->clear();

      /* Initial settings */
      for(unsigned i = 0; i < names.size(); ++i)
         list->add(names[i].c_str());

// TODO:       /* Pre-select the files from the last run. */
// TODO:       selectedCount = 0;
// TODO:       for (i = 0; i < nWritable; ++i)
// TODO:       {
// TODO:          if (window->writableWindows[i]->multiFileReplSelected)
// TODO:          {
// TODO:             XmListSelectPos(list, i+1, false);
// TODO:             ++selectedCount;
// TODO:             /* Remember the first selected item */
// TODO:             if (firstSelected == 0) firstSelected = i+1;
// TODO:          }
// TODO:       }
// TODO:       /* If no files are selected, we select them all. Normally this only
// TODO:          happens the first time the dialog is used, but it looks "silly"
// TODO:          if the dialog pops up with nothing selected. */
// TODO:       if (selectedCount == 0)
// TODO:       {
// TODO:          for (i = 0; i < nWritable; ++i)
// TODO:          {
// TODO:             XmListSelectPos(list, i+1, false);
// TODO:          }
// TODO:          firstSelected = 1;
// TODO:       }

// TODO:       /* Make sure that the first selected item is visible; otherwise, the
// TODO:          user could get the impression that nothing is selected. By
// TODO:          visualizing at least the first selected item, the user will more
// TODO:          easily be confident that the previous selection is still active. */
// TODO:       XtSetArg(args[0], XmNvisibleItemCount, &nVisible);
// TODO:       XtGetValues(list, args, 1);
// TODO:       /* Make sure that we don't create blank lines at the bottom by
// TODO:          positioning too far. */
// TODO:       if (nWritable <= nVisible)
// TODO:       {
// TODO:          /* No need to shift the visible position */
// TODO:          firstSelected = 1;
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          int maxFirst = nWritable - nVisible + 1;
// TODO:          if (firstSelected > maxFirst)
// TODO:             firstSelected = maxFirst;
// TODO:       }
// TODO:       XmListSetPos(list, firstSelected);
   }
}

/*
** Unconditionally pops down the replace dialog and the
** replace-in-multiple-files dialog, if it exists.
*/
static void unmanageReplaceDialogs(const WindowInfo* window)
{
   /* If the replace dialog goes down, the multi-file replace dialog must go down too */
   if (window->replaceMultiFileDlog && window->replaceMultiFileDlog->visible())
   {
      window->replaceMultiFileDlog->hide();
   }

   if (window->replaceDlog && window->replaceDlog->visible())
   {
      window->replaceDlog->hide();
   }
}

static void rInSelCB(Fl_Widget* w, void* data)
{
   char searchString[SEARCHMAX], replaceString[SEARCHMAX];
   int direction, searchType;
   const char* params[3];

   WindowInfo* window = static_cast<WindowInfo*>(data);

   /* Validate and fetch the find and replace strings from the dialog */
   if (!getReplaceDlogInfo(window, &direction, searchString, replaceString, &searchType))
      return;

   /* Set the initial focus of the dialog back to the search string */
   resetReplaceTabGroup(window);

   /* do replacement */
   params[0] = searchString;
   params[1] = replaceString;
   params[2] = searchTypeArg(searchType);
   windowNotToClose = window;
   AppContext.callAction(window->lastFocus, "replace_in_selection", Fl::event(), params, 3);
   windowNotToClose = NULL;

   /* pop down the dialog */
   if (!NeToggleButtonGetState(window->replaceKeepBtn))
      unmanageReplaceDialogs(window);
}

static void rCancelCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);

   /* Set the initial focus of the dialog back to the search string	*/
   resetReplaceTabGroup(window);

   /* pop down the dialog */
   unmanageReplaceDialogs(window);
}

static void fCancelCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);

   /* pop down the dialog */
   window->findDlog->hide();

   /* Set the initial focus of the dialog back to the search string	*/
   resetFindTabGroup(window);
}

static void rFindCB(Fl_Widget* w, void* data)
{
   char searchString[SEARCHMAX], replaceString[SEARCHMAX];
   int direction, searchType;
   const char* params[4];

   WindowInfo* window = static_cast<WindowInfo*>(data);

   /* Validate and fetch the find and replace strings from the dialog */
   if (!getReplaceDlogInfo(window, &direction, searchString, replaceString, &searchType))
      return;

   /* Set the initial focus of the dialog back to the search string	*/
   resetReplaceTabGroup(window);

   /* Find the text and mark it */
   params[0] = searchString;
   params[1] = directionArg(direction);
   params[2] = searchTypeArg(searchType);
   params[3] = searchWrapArg(GetPrefSearchWraps());
   windowNotToClose = window;
   AppContext.callAction(window->lastFocus, "find", Fl::event(), params, 4);
   windowNotToClose = NULL;

   /* Doctor the search history generated by the action to include the
      replace string (if any), so the replace string can be used on
      subsequent replaces, even though no actual replacement was done. */
   if (historyIndex(1) != -1 &&
         !strcmp(SearchHistory[historyIndex(1)], searchString))
   {
      delete[] (ReplaceHistory[historyIndex(1)]);
      ReplaceHistory[historyIndex(1)] = NeNewString(replaceString);
   }

   /* Pop down the dialog */
   if (!NeToggleButtonGetState(window->replaceKeepBtn))
      unmanageReplaceDialogs(window);
}

static void replaceFindCB(Fl_Widget* w, void* data)
{
   char searchString[SEARCHMAX+1], replaceString[SEARCHMAX+1];
   int direction, searchType;
   const char* params[4];

   WindowInfo* window = static_cast<WindowInfo*>(data);

   /* Validate and fetch the find and replace strings from the dialog */
   if (!getReplaceDlogInfo(window, &direction, searchString, replaceString, &searchType))
      return;

   /* Set the initial focus of the dialog back to the search string */
   resetReplaceTabGroup(window);

   /* Find the text and replace it */
   params[0] = searchString;
   params[1] = replaceString;
   params[2] = directionArg(direction);
   params[3] = searchTypeArg(searchType);
   windowNotToClose = window;
   AppContext.callAction(window->lastFocus, "replace_find", Fl::event(), params, 4);
   windowNotToClose = NULL;

   /* Pop down the dialog */
   if (!NeToggleButtonGetState(window->replaceKeepBtn))
      unmanageReplaceDialogs(window);
}

static void rSetActionButtons(WindowInfo* window,
                              int replaceBtn,
                              int replaceFindBtn,
                              int replaceAndFindBtn,
                              int replaceInWinBtn,
                              int replaceInSelBtn,
                              int replaceAllBtn)
{
   NeSetSensitive(window->replaceBtn,        replaceBtn);
   NeSetSensitive(window->replaceFindBtn,    replaceFindBtn);
   NeSetSensitive(window->replaceAndFindBtn, replaceAndFindBtn);
   NeSetSensitive(window->replaceInWinBtn, replaceInWinBtn);
   NeSetSensitive(window->replaceInSelBtn, replaceInSelBtn);
   NeSetSensitive(window->replaceAllBtn,     replaceAllBtn);
}

void UpdateReplaceActionButtons(WindowInfo* window)
{
   /* Is there any text in the search for field */
   bool searchText = textFieldNonEmpty(window->replaceText);
   rSetActionButtons(window, searchText, searchText, searchText,
                     searchText, searchText && window->wasSelected,
                     searchText && (countWritableWindows() > 1));
}

static bool textFieldNonEmpty(Fl_Input* w)
{
   const char* str = w->value();
   bool nonEmpty = str && (str[0] != '\0');
   return(nonEmpty);
}

static void rFindTextValueChangedCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);
   TRACE();
   if (Fl::event_key() == FL_Up || Fl::event_key() == FL_Down)        // up/down keys?
      rFindArrowKeyCB(w, window);
   else
      UpdateReplaceActionButtons(window);
}

static void rFindArrowKeyCB(Fl_Widget* w, void* data)
{
   int keysym = Fl::event();
   int index;
   char* searchStr, *replaceStr;
   int searchType;

   WindowInfo* window = static_cast<WindowInfo*>(data);
   index = window->rHistIndex;

   /* only process up and down arrow keys */
   if (keysym != FL_Up && keysym != FL_Down)
      return;

   /* increment or decrement the index depending on which arrow was pressed */
   index += (keysym == FL_Up) ? 1 : -1;

   /* if the index is out of range, beep and return */
   if (index != 0 && historyIndex(index) == -1)
   {
      fl_beep();
      return;
   }

   /* determine the strings and button settings to use */
   if (index == 0)
   {
      searchStr = "";
      replaceStr = "";
      searchType = GetPrefSearch();
   }
   else
   {
      searchStr = SearchHistory[historyIndex(index)];
      replaceStr = ReplaceHistory[historyIndex(index)];
      searchType = SearchTypeHistory[historyIndex(index)];
   }

   /* Set the buttons and fields with the selected search type */
   initToggleButtons(searchType, window->replaceRegexToggle,
                     window->replaceCaseToggle, window->replaceWordToggle,
                     &window->replaceLastLiteralCase,
                     &window->replaceLastRegexCase);

   NeTextSetString(window->replaceText, searchStr);
   NeTextSetString(window->replaceWithText, replaceStr);

   /* Set the state of the Replace, Find ... buttons */
   UpdateReplaceActionButtons(window);

   window->rHistIndex = index;
}

static void replaceArrowKeyCB(Fl_Widget* w, void* data)
{
   int keysym = Fl::event();
   int index;

   WindowInfo* window = static_cast<WindowInfo*>(data);
   index = window->rHistIndex;

   /* only process up and down arrow keys */
   if (keysym != FL_Up && keysym != FL_Down)
      return;

   /* increment or decrement the index depending on which arrow was pressed */
   index += (keysym == FL_Up) ? 1 : -1;

   /* if the index is out of range, beep and return */
   if (index != 0 && historyIndex(index) == -1)
   {
      fl_beep();
      return;
   }

   /* change only the replace field information */
   if (index == 0)
      NeTextSetString(window->replaceWithText, "");
   else
      NeTextSetString(window->replaceWithText, ReplaceHistory[historyIndex(index)]);
   window->rHistIndex = index;
}

static void fUpdateActionButtons(WindowInfo* window)
{
   bool buttonState = textFieldNonEmpty(window->findText);
   NeSetSensitive(window->findBtn, buttonState);
}

static void findTextValueChangedCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);
   
   TRACE();
   if (Fl::event_key() == FL_Up || Fl::event_key() == FL_Down)        // up/down keys?
      findArrowKeyCB(w, window);
   else
      fUpdateActionButtons(window);
}

static void findArrowKeyCB(Fl_Widget* w, void* data)
{
   int keysym = Fl::event_key();
   int index;
   char* searchStr;
   int searchType;

   WindowInfo* window = static_cast<WindowInfo*>(data);
   index = window->fHistIndex;

   /* only process up and down arrow keys */
   if (keysym != FL_Up && keysym != FL_Down)
      return;

   /* increment or decrement the index depending on which arrow was pressed */
   index += (keysym == FL_Up) ? 1 : -1;

   /* if the index is out of range, beep and return */
   if (index != 0 && historyIndex(index) == -1)
   {
      fl_beep();
      return;
   }


   /* determine the strings and button settings to use */
   if (index == 0)
   {
      searchStr = "";
      searchType = GetPrefSearch();
   }
   else
   {
      searchStr = SearchHistory[historyIndex(index)];
      searchType = SearchTypeHistory[historyIndex(index)];
   }

   /* Set the buttons and fields with the selected search type */
   initToggleButtons(searchType, window->findRegexToggle,
                     window->findCaseToggle, window->findWordToggle,
                     &window->findLastLiteralCase,
                     &window->findLastRegexCase);
   NeTextSetString(window->findText, searchStr);

   /* Set the state of the Find ... button */
   fUpdateActionButtons(window);

   window->fHistIndex = index;
}

static void findCB(Fl_Widget* w, void* data)
{
   char searchString[SEARCHMAX];
   int direction, searchType;
   const char* params[4];

   WindowInfo* window = static_cast<WindowInfo*>(data);

   /* fetch find string, direction and type from the dialog */
   if (!getFindDlogInfo(window, &direction, searchString, &searchType))
      return;

   /* find the text and mark it */
   params[0] = searchString;
   params[1] = directionArg(direction);
   params[2] = searchTypeArg(searchType);
   params[3] = searchWrapArg(GetPrefSearchWraps());
   windowNotToClose = window;
   AppContext.callAction(window->lastFocus, "find",Fl::event(), params, 4);
   windowNotToClose = NULL;

   /* pop down the dialog */
   if (!NeToggleButtonGetState(window->findKeepBtn))
      window->findDlog->hide();

   /* Set the initial focus of the dialog back to the search string	*/
   resetFindTabGroup(window);
}

/*
** Fetch and verify (particularly regular expression) search and replace
** strings and search type from the Replace dialog.  If the strings are ok,
** save a copy in the search history, copy them in to "searchString",
** "replaceString', which are assumed to be at least SEARCHMAX in length,
** return search type in "searchType", and return true as the function
** value.  Otherwise, return false.
*/
static int getReplaceDlogInfo(WindowInfo* window, int* direction, char* searchString, char* replaceString, int* searchType)
{
   char* replaceText, *replaceWithText;
   regexp* compiledRE = NULL;
   char* compileMsg;

   /* Get the search and replace strings, search type, and direction from the dialog */
   replaceText = NeTextGetString(window->replaceText);
   replaceWithText = NeTextGetString(window->replaceWithText);

   if (NeToggleButtonGetState(window->replaceRegexToggle))
   {
      int regexDefault;
      if (NeToggleButtonGetState(window->replaceCaseToggle))
      {
         *searchType = SEARCH_REGEX;
         regexDefault = REDFLT_STANDARD;
      }
      else
      {
         *searchType = SEARCH_REGEX_NOCASE;
         regexDefault = REDFLT_CASE_INSENSITIVE;
      }
      /* If the search type is a regular expression, test compile it immediately and present error messages */
      compiledRE = CompileRE(replaceText, &compileMsg, regexDefault);
      if (compiledRE == NULL)
      {
         DialogF(DF_WARN, window->replaceDlog, 1, "Search String",
                 "Please respecify the search string:\n%s", "OK", compileMsg);
         delete[] replaceText;
         delete[] replaceWithText;
         return false;
      }
      free__((char*)compiledRE);
   }
   else
   {
      if (NeToggleButtonGetState(window->replaceCaseToggle))
      {
         if (NeToggleButtonGetState(window->replaceWordToggle))
            *searchType = SEARCH_CASE_SENSE_WORD;
         else
            *searchType = SEARCH_CASE_SENSE;
      }
      else
      {
         if (NeToggleButtonGetState(window->replaceWordToggle))
            *searchType = SEARCH_LITERAL_WORD;
         else
            *searchType = SEARCH_LITERAL;
      }
   }

   *direction = NeToggleButtonGetState(window->replaceRevToggle) ? SEARCH_BACKWARD : SEARCH_FORWARD;

   /* Return strings */
   if (strlen(replaceText) >= SEARCHMAX)
   {
      DialogF(DF_WARN, window->replaceDlog, 1, "String too long", "Search string too long.", "OK");
      delete[] replaceText;
      delete[] replaceWithText;
      return false;
   }
   if (strlen(replaceWithText) >= SEARCHMAX)
   {
      DialogF(DF_WARN, window->replaceDlog, 1, "String too long", "Replace string too long.", "OK");
      delete[] replaceText;
      delete[] replaceWithText;
      return false;
   }
   strcpy(searchString, replaceText);
   strcpy(replaceString, replaceWithText);
   delete[] replaceText;
   delete[] replaceWithText;
   return true;
}

/*
** Fetch and verify (particularly regular expression) search string,
** direction, and search type from the Find dialog.  If the search string
** is ok, save a copy in the search history, copy it to "searchString",
** which is assumed to be at least SEARCHMAX in length, return search type
** in "searchType", and return true as the function value.  Otherwise,
** return false.
*/
static int getFindDlogInfo(WindowInfo* window, int* direction, char* searchString, int* searchType)
{
   char* findText;
   regexp* compiledRE = NULL;
   char* compileMsg;

   /* Get the search string, search type, and direction from the dialog */
   findText = NeTextGetString(window->findText);

   if (NeToggleButtonGetState(window->findRegexToggle))
   {
      int regexDefault;
      if (NeToggleButtonGetState(window->findCaseToggle))
      {
         *searchType = SEARCH_REGEX;
         regexDefault = REDFLT_STANDARD;
      }
      else
      {
         *searchType = SEARCH_REGEX_NOCASE;
         regexDefault = REDFLT_CASE_INSENSITIVE;
      }
      /* If the search type is a regular expression, test compile it
         immediately and present error messages */
      compiledRE = CompileRE(findText, &compileMsg, regexDefault);
      if (compiledRE == NULL)
      {
         DialogF(DF_WARN, window->findDlog, 1, "Regex Error",
                 "Please respecify the search string:\n%s", "OK", compileMsg);
         return false;
      }
      free__((char*)compiledRE);
   }
   else
   {
      if (NeToggleButtonGetState(window->findCaseToggle))
      {
         if (NeToggleButtonGetState(window->findWordToggle))
            *searchType = SEARCH_CASE_SENSE_WORD;
         else
            *searchType = SEARCH_CASE_SENSE;
      }
      else
      {
         if (NeToggleButtonGetState(window->findWordToggle))
            *searchType = SEARCH_LITERAL_WORD;
         else
            *searchType = SEARCH_LITERAL;
      }
   }

   *direction = NeToggleButtonGetState(window->findRevToggle) ? SEARCH_BACKWARD : SEARCH_FORWARD;

   if (isRegexType(*searchType))
   {
   }

   /* Return the search string */
   if (strlen(findText) >= SEARCHMAX)
   {
      DialogF(DF_WARN, window->findDlog, 1, "String too long", "Search string too long.", "OK");
      NeStringFree(findText);
      return false;
   }
   strcpy(searchString, findText);
   NeStringFree(findText);
   return true;
}

int SearchAndSelectSame(WindowInfo* window, int direction, int searchWrap)
{
   if (NHist < 1)
   {
      fl_beep();
      return 0;
   }

   return SearchAndSelect(window, direction, SearchHistory[historyIndex(1)],
                          SearchTypeHistory[historyIndex(1)], searchWrap);
}

/*
** Search for "searchString" in "window", and select the matching text in
** the window when found (or beep or put up a dialog if not found).  Also
** adds the search string to the global search history.
*/
int SearchAndSelect(WindowInfo* window, int direction, const char* searchString,
                    int searchType, int searchWrap)
{
   int startPos, endPos;
   int beginPos, cursorPos, selStart, selEnd;
   int movedFwd = 0;

   /* Save a copy of searchString in the search history */
   saveSearchHistory(searchString, NULL, searchType, false);

   /* set the position to start the search so we don't find the same
      string that was found on the last search	*/
   if (searchMatchesSelection(window, searchString, searchType, &selStart, &selEnd, NULL, NULL))
   {
      /* selection matches search string, start before or after sel.	*/
      if (direction == SEARCH_BACKWARD)
      {
         beginPos = selStart - 1;
      }
      else
      {
         beginPos = selStart + 1;
         movedFwd = 1;
      }
   }
   else
   {
      selStart = -1;
      selEnd = -1;
      /* no selection, or no match, search relative cursor */
      cursorPos = TextGetCursorPos(window->lastFocus);
      if (direction == SEARCH_BACKWARD)
      {
         /* use the insert position - 1 for backward searches */
         beginPos = cursorPos-1;
      }
      else
      {
         /* use the insert position for forward searches */
         beginPos = cursorPos;
      }
   }

   /* when the i-search bar is active and search is repeated there
      (Return), the action "find" is called (not: "find_incremental").
      "find" calls this function SearchAndSelect.
      To keep track of the iSearchLastBeginPos correctly in the
      repeated i-search case it is necessary to call the following
      function here, otherwise there are no beeps on the repeated
      incremental search wraps.  */
   iSearchRecordLastBeginPos(window, direction, beginPos);

   /* do the search.  SearchWindow does appropriate dialogs and beeps */
   if (!SearchWindow(window, direction, searchString, searchType, searchWrap,
                     beginPos, &startPos, &endPos, NULL, NULL))
      return false;

   /* if the search matched an empty string (possible with regular exps)
      beginning at the start of the search, go to the next occurrence,
      otherwise repeated finds will get "stuck" at zero-length matches */
   if (direction==SEARCH_FORWARD && beginPos==startPos && beginPos==endPos)
   {
      if (!movedFwd &&
            !SearchWindow(window, direction, searchString, searchType,
                          searchWrap, beginPos+1, &startPos, &endPos, NULL, NULL))
         return false;
   }

   /* if matched text is already selected, just beep */
   if (selStart==startPos && selEnd==endPos)
   {
      fl_beep();
      return false;
   }

   /* select the text found string */
   BufSelect(window->buffer, startPos, endPos);
   MakeSelectionVisible(window, window->lastFocus);
   TextSetCursorPos(window->lastFocus, endPos);

   return true;
}

void SearchForSelected(WindowInfo* window, int direction, int searchType, int searchWrap, int time)
{
   SearchSelectedCallData callData;
   callData.direction = direction;
   callData.searchType = searchType;
   callData.searchWrap = searchWrap;

   selectedSearchCB(window, callData);
}

static void selectedSearchCB(WindowInfo* window, const SearchSelectedCallData& callDataItems)
{
   int searchType;
   char searchString[SEARCHMAX+1];

   char* value= BufGetSelectionText(window->textArea->buffer);
   size_t length = strlen(value);

   /* skip if we can't get the selection data or it's too long */
   if (value == NULL)
   {
      if (GetPrefSearchDlogs())
         DialogF(DF_WARN, window->mainWindow, 1, "Wrong Selection", "Selection not appropriate for searching", "OK");
      else
         fl_beep();
      return;
   }
   if (length > SEARCHMAX)
   {
      if (GetPrefSearchDlogs())
         DialogF(DF_WARN, window->mainWindow, 1, "Selection too long", "Selection too long", "OK");
      else
         fl_beep();
      delete[] value;
      return;
   }
   if (length == 0)
   {
      fl_beep();
      delete[] value;
      return;
   }
   
   /* make the selection the current search string */
   strncpy(searchString, value, length);
   searchString[length] = '\0';
   delete[] value;

   /* Use the passed method for searching, unless it is regex, since this
      kind of search is by definition a literal search */
   searchType = callDataItems.searchType;
   if (searchType == SEARCH_REGEX)
      searchType = SEARCH_CASE_SENSE;
   else if (searchType == SEARCH_REGEX_NOCASE)
      searchType = SEARCH_LITERAL;

   /* search for it in the window */
   SearchAndSelect(window, callDataItems.direction, searchString, searchType, callDataItems.searchWrap);
}

/*
** Pop up and clear the incremental search line and prepare to search.
*/
void BeginISearch(WindowInfo* window, int direction)
{
   window->iSearchStartPos = -1;
   NeTextSetString(window->iSearchText, "");
   NeToggleButtonSetState(window->iSearchRevToggle,
                          direction == SEARCH_BACKWARD, false);
   /* Note: in contrast to the replace and find dialogs, the regex and
      case toggles are not reset to their default state when the incremental
      search bar is redisplayed. I'm not sure whether this is the best
      choice. If not, an initToggleButtons() call should be inserted
      here. But in that case, it might be appropriate to have different
      default search modes for i-search and replace/find. */
   TempShowISearch(window, true);
   window->mainWindow->focus(window->iSearchText);
}

// TODO: /*
// TODO: ** Incremental searching is anchored at the position where the cursor
// TODO: ** was when the user began typing the search string.  Call this routine
// TODO: ** to forget about this original anchor, and if the search bar is not
// TODO: ** permanently up, pop it down.
// TODO: */
// TODO: void EndISearch(WindowInfo* window)
// TODO: {
// TODO:    /* Note: Please maintain this such that it can be freely peppered in
// TODO:       mainline code, without callers having to worry about performance
// TODO:       or visual glitches.  */
// TODO: 
// TODO:    /* Forget the starting position used for the current run of searches */
// TODO:    window->iSearchStartPos = -1;
// TODO: 
// TODO:    /* Mark the end of incremental search history overwriting */
// TODO:    saveSearchHistory("", NULL, 0, false);
// TODO: 
// TODO:    /* Pop down the search line (if it's not pegged up in Preferences) */
// TODO:    TempShowISearch(window, false);
// TODO: }

/*
** Reset window->iSearchLastBeginPos to the resulting initial
** search begin position for incremental searches.
*/
static void iSearchRecordLastBeginPos(WindowInfo* window, int direction, int initPos)
{
   window->iSearchLastBeginPos = initPos;
   if (direction == SEARCH_BACKWARD)
      window->iSearchLastBeginPos--;
}

// TODO: /*
// TODO: ** Search for "searchString" in "window", and select the matching text in
// TODO: ** the window when found (or beep or put up a dialog if not found).  If
// TODO: ** "continued" is true and a prior incremental search starting position is
// TODO: ** recorded, search from that original position, otherwise, search from the
// TODO: ** current cursor position.
// TODO: */
// TODO: int SearchAndSelectIncremental(WindowInfo* window, int direction,
// TODO:                                const char* searchString, int searchType, int searchWrap, int continued)
// TODO: {
// TODO:    int beginPos, startPos, endPos;
// TODO: 
// TODO:    /* If there's a search in progress, start the search from the original
// TODO:       starting position, otherwise search from the cursor position. */
// TODO:    if (!continued || window->iSearchStartPos == -1)
// TODO:    {
// TODO:       window->iSearchStartPos = TextGetCursorPos(window->lastFocus);
// TODO:       iSearchRecordLastBeginPos(window, direction, window->iSearchStartPos);
// TODO:    }
// TODO:    beginPos = window->iSearchStartPos;
// TODO: 
// TODO:    /* If the search string is empty, beep eventually if text wrapped
// TODO:       back to the initial position, re-init iSearchLastBeginPos,
// TODO:       clear the selection, set the cursor back to what would be the
// TODO:       beginning of the search, and return. */
// TODO:    if (searchString[0] == 0)
// TODO:    {
// TODO:       int beepBeginPos = (direction == SEARCH_BACKWARD) ? beginPos-1:beginPos;
// TODO:       iSearchTryBeepOnWrap(window, direction, beepBeginPos, beepBeginPos);
// TODO:       iSearchRecordLastBeginPos(window, direction, window->iSearchStartPos);
// TODO:       BufUnselect(window->buffer);
// TODO:       TextSetCursorPos(window->lastFocus, beginPos);
// TODO:       return true;
// TODO:    }
// TODO: 
// TODO:    /* Save the string in the search history, unless we're cycling thru
// TODO:       the search history itself, which can be detected by matching the
// TODO:       search string with the search string of the current history index. */
// TODO:    if (!(window->iSearchHistIndex > 1 && !strcmp(searchString,
// TODO:          SearchHistory[historyIndex(window->iSearchHistIndex)])))
// TODO:    {
// TODO:       saveSearchHistory(searchString, NULL, searchType, true);
// TODO:       /* Reset the incremental search history pointer to the beginning */
// TODO:       window->iSearchHistIndex = 1;
// TODO:    }
// TODO: 
// TODO:    /* begin at insert position - 1 for backward searches */
// TODO:    if (direction == SEARCH_BACKWARD)
// TODO:       beginPos--;
// TODO: 
// TODO:    /* do the search.  SearchWindow does appropriate dialogs and beeps */
// TODO:    if (!SearchWindow(window, direction, searchString, searchType, searchWrap,
// TODO:                      beginPos, &startPos, &endPos, NULL, NULL))
// TODO:       return false;
// TODO: 
// TODO:    window->iSearchLastBeginPos = startPos;
// TODO: 
// TODO:    /* if the search matched an empty string (possible with regular exps)
// TODO:       beginning at the start of the search, go to the next occurrence,
// TODO:       otherwise repeated finds will get "stuck" at zero-length matches */
// TODO:    if (direction==SEARCH_FORWARD && beginPos==startPos && beginPos==endPos)
// TODO:       if (!SearchWindow(window, direction, searchString, searchType, searchWrap,
// TODO:                         beginPos+1, &startPos, &endPos, NULL, NULL))
// TODO:          return false;
// TODO: 
// TODO:    window->iSearchLastBeginPos = startPos;
// TODO: 
// TODO:    /* select the text found string */
// TODO:    BufSelect(window->buffer, startPos, endPos);
// TODO:    MakeSelectionVisible(window, window->lastFocus);
// TODO:    TextSetCursorPos(window->lastFocus, endPos);
// TODO: 
// TODO:    return true;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Attach callbacks to the incremental search bar widgets.  This also fudges
// TODO: ** up the translations on the text widget so Shift+Return will call the
// TODO: ** activate callback (along with Return and Ctrl+Return).  It does this
// TODO: ** because incremental search uses the activate callback from the text
// TODO: ** widget to detect when the user has pressed Return to search for the next
// TODO: ** occurrence of the search string, and Shift+Return, which is the natural
// TODO: ** command for a reverse search does not naturally trigger this callback.
// TODO: */
// TODO: void SetISearchTextCallbacks(WindowInfo* window)
// TODO: {
// TODO:    static XtTranslations tableText = NULL;
// TODO:    static char* translationsText = "Shift<KeyPress>Return: activate()\n";
// TODO: 
// TODO:    static XtTranslations tableClear = NULL;
// TODO:    static char* translationsClear =
// TODO:       "<Btn2Down>:Arm()\n<Btn2Up>: isearch_clear_and_paste() Disarm()\n";
// TODO: 
// TODO:    static XtActionsRec actions[] =
// TODO:    {
// TODO:       { "isearch_clear_and_paste", iSearchTextClearAndPasteAP }
// TODO:    };
// TODO: 
// TODO:    if (tableText == NULL)
// TODO:       tableText = XtParseTranslationTable(translationsText);
// TODO:    XtOverrideTranslations(window->iSearchText, tableText);
// TODO: 
// TODO:    if (tableClear == NULL)
// TODO:    {
// TODO:       /* make sure actions are loaded */
// TODO:       XtAppAddActions(XtWidgetToApplicationContext(window->iSearchText),
// TODO:                       actions, XtNumber(actions));
// TODO:       tableClear = XtParseTranslationTable(translationsClear);
// TODO:    }
// TODO:    XtOverrideTranslations(window->iSearchClearButton, tableClear);
// TODO: 
// TODO:    XtAddCallback(window->iSearchText, XmNactivateCallback,
// TODO:                  (XtCallbackProc)iSearchTextActivateCB, window);
// TODO:    XtAddCallback(window->iSearchText, XmNvalueChangedCallback,
// TODO:                  (XtCallbackProc)iSearchTextValueChangedCB, window);
// TODO:    XtAddEventHandler(window->iSearchText, KeyPressMask, false,
// TODO:                      (XtEventHandler)iSearchTextKeyEH, window);
// TODO: 
// TODO:    /* Attach callbacks to deal with the optional sticky case sensitivity
// TODO:       behaviour. Do this before installing the search callbacks to make
// TODO:       sure that the proper search parameters are taken into account. */
// TODO:    XtAddCallback(window->iSearchCaseToggle, XmNvalueChangedCallback,
// TODO:                  (XtCallbackProc)iSearchCaseToggleCB, window);
// TODO:    XtAddCallback(window->iSearchRegexToggle, XmNvalueChangedCallback,
// TODO:                  (XtCallbackProc)iSearchRegExpToggleCB, window);
// TODO: 
// TODO:    /* When search parameters (direction or search type), redo the search */
// TODO:    XtAddCallback(window->iSearchCaseToggle, XmNvalueChangedCallback,
// TODO:                  (XtCallbackProc)iSearchTextValueChangedCB, window);
// TODO:    XtAddCallback(window->iSearchRegexToggle, XmNvalueChangedCallback,
// TODO:                  (XtCallbackProc)iSearchTextValueChangedCB, window);
// TODO:    XtAddCallback(window->iSearchRevToggle, XmNvalueChangedCallback,
// TODO:                  (XtCallbackProc)iSearchTextValueChangedCB, window);
// TODO: 
// TODO:    /* find button: just like pressing return */
// TODO:    XtAddCallback(window->iSearchFindButton, XmNactivateCallback,
// TODO:                  (XtCallbackProc)iSearchTextActivateCB, window);
// TODO:    /* clear button: empty the search text widget */
// TODO:    XtAddCallback(window->iSearchClearButton, XmNactivateCallback,
// TODO:                  (XtCallbackProc)iSearchTextClearCB, window);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Remove callbacks before resetting the incremental search text to avoid any
// TODO: ** cursor movement and/or clearing of selections.
// TODO: */
// TODO: static void iSearchTextSetString(Widget w, WindowInfo* window,
// TODO:                                  char* str)
// TODO: {
// TODO:    /* remove callbacks which would be activated by emptying the text */
// TODO:    XtRemoveAllCallbacks(window->iSearchText, XmNvalueChangedCallback);
// TODO:    XtRemoveAllCallbacks(window->iSearchText, XmNactivateCallback);
// TODO:    /* empty the text */
// TODO:    NeTextSetString(window->iSearchText, str ? str : "");
// TODO:    /* put back the callbacks */
// TODO:    XtAddCallback(window->iSearchText, XmNactivateCallback,
// TODO:                  (XtCallbackProc)iSearchTextActivateCB, window);
// TODO:    XtAddCallback(window->iSearchText, XmNvalueChangedCallback,
// TODO:                  (XtCallbackProc)iSearchTextValueChangedCB, window);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Action routine for Mouse Button 2 on the iSearchClearButton: resets the
// TODO: ** string then calls the activate callback for the text directly.
// TODO: */
// TODO: static void iSearchTextClearAndPasteAP(Widget w, XEvent* event, String* args,
// TODO:                                        Cardinal* nArg)
// TODO: {
// TODO:    WindowInfo* window;
// TODO:    char* selText;
// TODO:    XmAnyCallbackStruct cbdata;
// TODO: 
// TODO:    memset(&cbdata, 0, sizeof(cbdata));
// TODO:    cbdata.event = event;
// TODO: 
// TODO:    window = WidgetToWindow(w);
// TODO: 
// TODO:    selText = GetAnySelection(window);
// TODO:    iSearchTextSetString(w, window, selText);
// TODO:    if (selText)
// TODO:    {
// TODO:       XmTextSetInsertionPosition(window->iSearchText, strlen(selText));
// TODO:       XtFree(selText);
// TODO:    }
// TODO:    iSearchTextActivateCB(w, window, &cbdata);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** User pressed the clear incremental search bar button. Remove callbacks
// TODO: ** before resetting the text to avoid any cursor movement and/or clearing
// TODO: ** of selections.
// TODO: */
// TODO: static void iSearchTextClearCB(Widget w, WindowInfo* window,
// TODO:                                XmAnyCallbackStruct* callData)
// TODO: {
// TODO:    window = WidgetToWindow(w);
// TODO: 
// TODO:    iSearchTextSetString(w, window, NULL);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** User pressed return in the incremental search bar.  Do a new search with
// TODO: ** the search string displayed.  The direction of the search is toggled if
// TODO: ** the Ctrl key or the Shift key is pressed when the text field is activated.
// TODO: */
// TODO: static void iSearchTextActivateCB(Widget w, WindowInfo* window,
// TODO:                                   XmAnyCallbackStruct* callData)
// TODO: {
// TODO:    char* params[4];
// TODO:    char* searchString;
// TODO:    int searchType, direction;
// TODO: 
// TODO:    window = WidgetToWindow(w);
// TODO: 
// TODO:    /* Fetch the string, search type and direction from the incremental
// TODO:       search bar widgets at the top of the window */
// TODO:    searchString = NeTextGetString(window->iSearchText);
// TODO:    if (NeToggleButtonGetState(window->iSearchCaseToggle))
// TODO:    {
// TODO:       if (NeToggleButtonGetState(window->iSearchRegexToggle))
// TODO:          searchType = SEARCH_REGEX;
// TODO:       else
// TODO:          searchType = SEARCH_CASE_SENSE;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if (NeToggleButtonGetState(window->iSearchRegexToggle))
// TODO:          searchType = SEARCH_REGEX_NOCASE;
// TODO:       else
// TODO:          searchType = SEARCH_LITERAL;
// TODO:    }
// TODO:    direction = NeToggleButtonGetState(window->iSearchRevToggle) ?
// TODO:                SEARCH_BACKWARD : SEARCH_FORWARD;
// TODO: 
// TODO:    /* Reverse the search direction if the Ctrl or Shift key was pressed */
// TODO:    if (callData->event->xbutton.state & (ShiftMask | ControlMask))
// TODO:       direction = direction == SEARCH_FORWARD ?
// TODO:                   SEARCH_BACKWARD : SEARCH_FORWARD;
// TODO: 
// TODO:    /* find the text and mark it */
// TODO:    params[0] = searchString;
// TODO:    params[1] = directionArg(direction);
// TODO:    params[2] = searchTypeArg(searchType);
// TODO:    params[3] = searchWrapArg(GetPrefSearchWraps());
// TODO:    XtCallActionProc(window->lastFocus, "find", callData->event, params, 4);
// TODO:    XtFree(searchString);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Called when user types in the incremental search line.  Redoes the
// TODO: ** search for the new search string.
// TODO: */
// TODO: static void iSearchTextValueChangedCB(Widget w, WindowInfo* window,
// TODO:                                       XmAnyCallbackStruct* callData)
// TODO: {
// TODO:    char* params[5];
// TODO:    char* searchString;
// TODO:    int searchType, direction, nParams;
// TODO: 
// TODO:    window = WidgetToWindow(w);
// TODO: 
// TODO:    /* Fetch the string, search type and direction from the incremental
// TODO:       search bar widgets at the top of the window */
// TODO:    searchString = NeTextGetString(window->iSearchText);
// TODO:    if (NeToggleButtonGetState(window->iSearchCaseToggle))
// TODO:    {
// TODO:       if (NeToggleButtonGetState(window->iSearchRegexToggle))
// TODO:          searchType = SEARCH_REGEX;
// TODO:       else
// TODO:          searchType = SEARCH_CASE_SENSE;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if (NeToggleButtonGetState(window->iSearchRegexToggle))
// TODO:          searchType = SEARCH_REGEX_NOCASE;
// TODO:       else
// TODO:          searchType = SEARCH_LITERAL;
// TODO:    }
// TODO:    direction = NeToggleButtonGetState(window->iSearchRevToggle) ?
// TODO:                SEARCH_BACKWARD : SEARCH_FORWARD;
// TODO: 
// TODO:    /* If the search type is a regular expression, test compile it.  If it
// TODO:       fails, silently skip it.  (This allows users to compose the expression
// TODO:       in peace when they have unfinished syntax, but still get beeps when
// TODO:       correct syntax doesn't match) */
// TODO:    if (isRegexType(searchType))
// TODO:    {
// TODO:       regexp* compiledRE = NULL;
// TODO:       char* compileMsg;
// TODO:       compiledRE = CompileRE(searchString, &compileMsg,
// TODO:                              defaultRegexFlags(searchType));
// TODO:       if (compiledRE == NULL)
// TODO:       {
// TODO:          XtFree(searchString);
// TODO:          return;
// TODO:       }
// TODO:       free__((char*)compiledRE);
// TODO:    }
// TODO: 
// TODO:    /* Call the incremental search action proc to do the searching and
// TODO:       selecting (this allows it to be recorded for learn/replay).  If
// TODO:       there's an incremental search already in progress, mark the operation
// TODO:       as "continued" so the search routine knows to re-start the search
// TODO:       from the original starting position */
// TODO:    nParams = 0;
// TODO:    params[nParams++] = searchString;
// TODO:    params[nParams++] = directionArg(direction);
// TODO:    params[nParams++] = searchTypeArg(searchType);
// TODO:    params[nParams++] = searchWrapArg(GetPrefSearchWraps());
// TODO:    if (window->iSearchStartPos != -1)
// TODO:       params[nParams++] = "continued";
// TODO:    XtCallActionProc(window->lastFocus, "find_incremental",
// TODO:                     callData->event, params, nParams);
// TODO:    XtFree(searchString);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Process arrow keys for history recall, and escape key for leaving
// TODO: ** incremental search bar.
// TODO: */
// TODO: static void iSearchTextKeyEH(Widget w, WindowInfo* window,
// TODO:                              XKeyEvent* event, bool* continueDispatch)
// TODO: {
// TODO:    KeySym keysym = XLookupKeysym(event, 0);
// TODO:    int index;
// TODO:    char* searchStr;
// TODO:    int searchType;
// TODO: 
// TODO:    /* only process up and down arrow keys */
// TODO:    if (keysym != XK_Up && keysym != XK_Down && keysym != XK_Escape)
// TODO:    {
// TODO:       *continueDispatch = true;
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    window = WidgetToWindow(w);
// TODO:    index = window->iSearchHistIndex;
// TODO:    *continueDispatch = false;
// TODO: 
// TODO:    /* allow escape key to cancel search */
// TODO:    if (keysym == XK_Escape)
// TODO:    {
// TODO:       XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);
// TODO:       EndISearch(window);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* increment or decrement the index depending on which arrow was pressed */
// TODO:    index += (keysym == XK_Up) ? 1 : -1;
// TODO: 
// TODO:    /* if the index is out of range, beep and return */
// TODO:    if (index != 0 && historyIndex(index) == -1)
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* determine the strings and button settings to use */
// TODO:    if (index == 0)
// TODO:    {
// TODO:       searchStr = "";
// TODO:       searchType = GetPrefSearch();
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       searchStr = SearchHistory[historyIndex(index)];
// TODO:       searchType = SearchTypeHistory[historyIndex(index)];
// TODO:    }
// TODO: 
// TODO:    /* Set the info used in the value changed callback before calling
// TODO:      NeTextSetString(). */
// TODO:    window->iSearchHistIndex = index;
// TODO:    initToggleButtons(searchType, window->iSearchRegexToggle,
// TODO:                      window->iSearchCaseToggle, NULL,
// TODO:                      &window->iSearchLastLiteralCase,
// TODO:                      &window->iSearchLastRegexCase);
// TODO: 
// TODO:    /* Beware the value changed callback is processed as part of this call */
// TODO:    NeTextSetString(window->iSearchText, searchStr);
// TODO:    XmTextSetInsertionPosition(window->iSearchText,
// TODO:                               XmTextGetLastPosition(window->iSearchText));
// TODO: }
// TODO: 
/*
** Check the character before the insertion cursor of textW and flash
** matching parenthesis, brackets, or braces, by temporarily highlighting
** the matching character (a timer procedure is scheduled for removing the
** highlights)
*/
void FlashMatching(WindowInfo* window, Ne_Text_Editor* textW)
{
   char c;
   void* style;
   int pos, matchIndex;
   int startPos, endPos, searchPos, matchPos;
   int constrain;

   /* if a marker is already drawn, erase it and cancel the timeout */
   if (window->flashTimeoutID != 0)
   {
      eraseFlash(window);
// TODO:       XtRemoveTimeOut(window->flashTimeoutID);
      window->flashTimeoutID = 0;
   }

   /* no flashing required */
   if (window->showMatchingStyle == NO_FLASH)
   {
      return;
   }

   /* don't flash matching characters if there's a selection */
   if (window->buffer->primary.selected)
      return;

   /* get the character to match and the position to start from */
   pos = TextGetCursorPos(textW) - 1;
   if (pos < 0)
      return;
   c = BufGetCharacter(window->buffer, pos);
   style = GetHighlightInfo(window, pos);

   /* is the character one we want to flash? */
   for (matchIndex = 0; matchIndex<N_FLASH_CHARS; matchIndex++)
   {
      if (MatchingChars[matchIndex].c == c)
         break;
   }
   if (matchIndex == N_FLASH_CHARS)
      return;

   /* constrain the search to visible text only when in single-pane mode
      AND using delimiter flashing (otherwise search the whole buffer) */
   constrain = ((window->nPanes == 0) &&
                (window->showMatchingStyle == FLASH_DELIMIT));

   if (MatchingChars[matchIndex].direction == SEARCH_BACKWARD)
   {
      startPos = constrain ? TextFirstVisiblePos(textW) : 0;
      endPos = pos;
      searchPos = endPos;
   }
   else
   {
      startPos = pos;
      endPos = constrain ? TextLastVisiblePos(textW) :
               window->buffer->length;
      searchPos = startPos;
   }

   /* do the search */
   if (!findMatchingChar(window, c, style, searchPos, startPos, endPos,
                         &matchPos))
      return;

   if (window->showMatchingStyle == FLASH_DELIMIT)
   {
      /* Highlight either the matching character ... */
      BufHighlight(window->buffer, matchPos, matchPos+1);
   }
   else
   {
      /* ... or the whole range. */
      if (MatchingChars[matchIndex].direction == SEARCH_BACKWARD)
      {
         BufHighlight(window->buffer, matchPos, pos+1);
      }
      else
      {
         BufHighlight(window->buffer, matchPos+1, pos);
      }
   }

// TODO:    /* Set up a timer to erase the box after 1.5 seconds */
// TODO:    window->flashTimeoutID = XtAppAddTimeOut(
// TODO:                                XtWidgetToApplicationContext(window->mainWindow), 1500,
// TODO:                                flashTimeoutProc, window);
   window->flashPos = matchPos;
}

// TODO: void SelectToMatchingCharacter(WindowInfo* window)
// TODO: {
// TODO:    int selStart, selEnd;
// TODO:    int startPos, endPos, matchPos;
// TODO:    textBuffer* buf = window->buffer;
// TODO: 
// TODO:    /* get the character to match and its position from the selection, or
// TODO:       the character before the insert point if nothing is selected.
// TODO:       Give up if too many characters are selected */
// TODO:    if (!GetSimpleSelection(buf, &selStart, &selEnd))
// TODO:    {
// TODO:       selEnd = TextGetCursorPos(window->lastFocus);
// TODO:       if (window->overstrike)
// TODO:          selEnd += 1;
// TODO:       selStart = selEnd - 1;
// TODO:       if (selStart < 0)
// TODO:       {
// TODO:          XBell(TheDisplay, 0);
// TODO:          return;
// TODO:       }
// TODO:    }
// TODO:    if ((selEnd - selStart) != 1)
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Search for it in the buffer */
// TODO:    if (!findMatchingChar(window, BufGetCharacter(buf, selStart),
// TODO:                          GetHighlightInfo(window, selStart), selStart, 0, buf->length, &matchPos))
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:       return;
// TODO:    }
// TODO:    startPos = (matchPos > selStart) ? selStart : matchPos;
// TODO:    endPos = (matchPos > selStart) ? matchPos : selStart;
// TODO: 
// TODO:    /* temporarily shut off autoShowInsertPos before setting the cursor
// TODO:       position so MakeSelectionVisible gets a chance to place the cursor
// TODO:       string at a pleasing position on the screen (otherwise, the cursor would
// TODO:       be automatically scrolled on screen and MakeSelectionVisible would do
// TODO:       nothing) */
// TODO:    XtVaSetValues(window->lastFocus, textNautoShowInsertPos, false, NULL);
// TODO:    /* select the text between the matching characters */
// TODO:    BufSelect(buf, startPos, endPos+1);
// TODO:    MakeSelectionVisible(window, window->lastFocus);
// TODO:    XtVaSetValues(window->lastFocus, textNautoShowInsertPos, true, NULL);
// TODO: }

void GotoMatchingCharacter(WindowInfo* window)
{
   int selStart, selEnd;
   int matchPos;
   Ne_Text_Buffer* buf = window->buffer;

   /* get the character to match and its position from the selection, or
      the character before the insert point if nothing is selected.
      Give up if too many characters are selected */
   if (!GetSimpleSelection(buf, &selStart, &selEnd))
   {
      selEnd = TextGetCursorPos(window->lastFocus);
      if (window->overstrike)
         selEnd += 1;
      selStart = selEnd - 1;
      if (selStart < 0)
      {
         fl_beep();
         return;
      }
   }
   if ((selEnd - selStart) != 1)
   {
      fl_beep();
      return;
   }

   /* Search for it in the buffer */
   if (!findMatchingChar(window, BufGetCharacter(buf, selStart),
                         GetHighlightInfo(window, selStart), selStart, 0,
                         buf->length, &matchPos))
   {
      fl_beep();
      return;
   }

   /* temporarily shut off autoShowInsertPos before setting the cursor
      position so MakeSelectionVisible gets a chance to place the cursor
      string at a pleasing position on the screen (otherwise, the cursor would
      be automatically scrolled on screen and MakeSelectionVisible would do
      nothing) */
// TODO:    XtVaSetValues(window->lastFocus, textNautoShowInsertPos, false, NULL);
   TextSetCursorPos(window->lastFocus, matchPos+1);
   MakeSelectionVisible(window, window->lastFocus);
// TODO:    XtVaSetValues(window->lastFocus, textNautoShowInsertPos, true, NULL);
}

static int findMatchingChar(WindowInfo* window, char toMatch,
                            void* styleToMatch, int charPos, int startLimit, int endLimit,
                            int* matchPos)
{
   int nestDepth, matchIndex, direction, beginPos, pos;
   char matchChar, c;
   void* style = NULL;
   Ne_Text_Buffer* buf = window->buffer;
   int matchSyntaxBased = window->matchSyntaxBased;

   /* If we don't match syntax based, fake a matching style. */
   if (!matchSyntaxBased) style = styleToMatch;

   /* Look up the matching character and match direction */
   for (matchIndex = 0; matchIndex<N_MATCH_CHARS; matchIndex++)
   {
      if (MatchingChars[matchIndex].c == toMatch)
         break;
   }
   if (matchIndex == N_MATCH_CHARS)
      return false;
   matchChar = MatchingChars[matchIndex].match;
   direction = MatchingChars[matchIndex].direction;

   /* find it in the buffer */
   beginPos = (direction==SEARCH_FORWARD) ? charPos+1 : charPos-1;
   nestDepth = 1;
   if (direction == SEARCH_FORWARD)
   {
      for (pos=beginPos; pos<endLimit; pos++)
      {
         c=BufGetCharacter(buf, pos);
         if (c == matchChar)
         {
            if (matchSyntaxBased) style = GetHighlightInfo(window, pos);
            if (style == styleToMatch)
            {
               nestDepth--;
               if (nestDepth == 0)
               {
                  *matchPos = pos;
                  return true;
               }
            }
         }
         else if (c == toMatch)
         {
            if (matchSyntaxBased) style = GetHighlightInfo(window, pos);
            if (style == styleToMatch)
               nestDepth++;
         }
      }
   }
   else     /* SEARCH_BACKWARD */
   {
      for (pos=beginPos; pos>=startLimit; pos--)
      {
         c=BufGetCharacter(buf, pos);
         if (c == matchChar)
         {
            if (matchSyntaxBased) style = GetHighlightInfo(window, pos);
            if (style == styleToMatch)
            {
               nestDepth--;
               if (nestDepth == 0)
               {
                  *matchPos = pos;
                  return true;
               }
            }
         }
         else if (c == toMatch)
         {
            if (matchSyntaxBased) style = GetHighlightInfo(window, pos);
            if (style == styleToMatch)
               nestDepth++;
         }
      }
   }
   return false;
}

// TODO: /*
// TODO: ** Xt timer procedure for erasing the matching parenthesis marker.
// TODO: */
// TODO: static void flashTimeoutProc(XtPointer clientData, XtIntervalId* id)
// TODO: {
// TODO:    eraseFlash((WindowInfo*)clientData);
// TODO:    ((WindowInfo*)clientData)->flashTimeoutID = 0;
// TODO: }

/*
** Erase the marker drawn on a matching parenthesis bracket or brace
** character.
*/
static void eraseFlash(WindowInfo* window)
{
   BufUnhighlight(window->buffer);
}

/*
** Search and replace using previously entered search strings (from dialog
** or selection).
*/
int ReplaceSame(WindowInfo* window, int direction, int searchWrap)
{
   if (NHist < 1)
   {
      fl_beep();
      return false;
   }

   return SearchAndReplace(window, direction, SearchHistory[historyIndex(1)],
                           ReplaceHistory[historyIndex(1)],
                           SearchTypeHistory[historyIndex(1)], searchWrap);
}

/*
** Search and replace using previously entered search strings (from dialog
** or selection).
*/
int ReplaceFindSame(WindowInfo* window, int direction, int searchWrap)
{
   if (NHist < 1)
   {
      fl_beep();
      return false;
   }

   return ReplaceAndSearch(window, direction, SearchHistory[historyIndex(1)],
                           ReplaceHistory[historyIndex(1)],
                           SearchTypeHistory[historyIndex(1)], searchWrap);
}

/*
** Replace selection with "replaceString" and search for string "searchString" in window "window",
** using algorithm "searchType" and direction "direction"
*/
int ReplaceAndSearch(WindowInfo* window, int direction, const char* searchString,
                     const char* replaceString, int searchType, int searchWrap)
{
   int startPos = 0, endPos = 0, replaceLen = 0;
   int searchExtentBW, searchExtentFW;
   int replaced;

   /* Save a copy of search and replace strings in the search history */
   saveSearchHistory(searchString, replaceString, searchType, false);

   replaced = 0;

   /* Replace the selected text only if it matches the search string */
   if (searchMatchesSelection(window, searchString, searchType,
                              &startPos, &endPos, &searchExtentBW,
                              &searchExtentFW))
   {
      /* replace the text */
      if (isRegexType(searchType))
      {
         char replaceResult[SEARCHMAX+1], *foundString;
         foundString = BufGetRange(window->buffer, searchExtentBW,
                                   searchExtentFW+1);
         replaceUsingRE(searchString, replaceString, foundString,
                        startPos-searchExtentBW,
                        replaceResult, SEARCHMAX, startPos == 0 ? '\0' :
                        BufGetCharacter(window->buffer, startPos-1),
                        GetWindowDelimiters(window), defaultRegexFlags(searchType));
         delete[] foundString;
         BufReplace(window->buffer, startPos, endPos, replaceResult);
         replaceLen = strlen(replaceResult);
      }
      else
      {
         BufReplace(window->buffer, startPos, endPos, replaceString);
         replaceLen = strlen(replaceString);
      }

      /* Position the cursor so the next search will work correctly based */
      /* on the direction of the search */
      TextSetCursorPos(window->lastFocus, startPos + ((direction == SEARCH_FORWARD) ? replaceLen : 0));
      replaced = 1;
   }

   /* do the search; beeps/dialogs are taken care of */
   SearchAndSelect(window, direction, searchString, searchType, searchWrap);

   return replaced;
}

/*
** Search for string "searchString" in window "window", using algorithm
** "searchType" and direction "direction", and replace it with "replaceString"
** Also adds the search and replace strings to the global search history.
*/
int SearchAndReplace(WindowInfo* window, int direction, const char* searchString,
                     const char* replaceString, int searchType, int searchWrap)
{
   int startPos, endPos, replaceLen, searchExtentBW, searchExtentFW;
   int found;
   int beginPos, cursorPos;

   /* Save a copy of search and replace strings in the search history */
   saveSearchHistory(searchString, replaceString, searchType, false);

   /* If the text selected in the window matches the search string, 	*/
   /* the user is probably using search then replace method, so	*/
   /* replace the selected text regardless of where the cursor is.	*/
   /* Otherwise, search for the string.				*/
   if (!searchMatchesSelection(window, searchString, searchType,
                               &startPos, &endPos, &searchExtentBW, &searchExtentFW))
   {
      /* get the position to start the search */
      cursorPos = TextGetCursorPos(window->lastFocus);
      if (direction == SEARCH_BACKWARD)
      {
         /* use the insert position - 1 for backward searches */
         beginPos = cursorPos-1;
      }
      else
      {
         /* use the insert position for forward searches */
         beginPos = cursorPos;
      }
      /* do the search */
      found = SearchWindow(window, direction, searchString, searchType, searchWrap,
                           beginPos, &startPos, &endPos, &searchExtentBW, &searchExtentFW);
      if (!found)
         return false;
   }

   /* replace the text */
   if (isRegexType(searchType))
   {
      char replaceResult[SEARCHMAX], *foundString;
      foundString = BufGetRange(window->buffer, searchExtentBW, searchExtentFW+1);
      replaceUsingRE(searchString, replaceString, foundString,
                     startPos - searchExtentBW,
                     replaceResult, SEARCHMAX, startPos == 0 ? '\0' :
                     BufGetCharacter(window->buffer, startPos-1),
                     GetWindowDelimiters(window), defaultRegexFlags(searchType));
      delete[] foundString;
      BufReplace(window->buffer, startPos, endPos, replaceResult);
      replaceLen = strlen(replaceResult);
   }
   else
   {
      BufReplace(window->buffer, startPos, endPos, replaceString);
      replaceLen = strlen(replaceString);
   }

   /* after successfully completing a replace, selected text attracts
      attention away from the area of the replacement, particularly
      when the selection represents a previous search. so deselect */
   BufUnselect(window->buffer);

   /* temporarily shut off autoShowInsertPos before setting the cursor
      position so MakeSelectionVisible gets a chance to place the replaced
      string at a pleasing position on the screen (otherwise, the cursor would
      be automatically scrolled on screen and MakeSelectionVisible would do
      nothing) */
// TODO:    XtVaSetValues(window->lastFocus, textNautoShowInsertPos, false, NULL);
   TextSetCursorPos(window->lastFocus, startPos + ((direction == SEARCH_FORWARD) ? replaceLen : 0));
   MakeSelectionVisible(window, window->lastFocus);
// TODO:    XtVaSetValues(window->lastFocus, textNautoShowInsertPos, true, NULL);

   return true;
}

/*
**  Uses the resource nedit.truncSubstitution to determine how to deal with
**  regex failures. This function only knows about the resource (via the usual
**  setting getter) and asks or warns the user depending on that.
**
**  One could argue that the dialoging should be determined by the setting
**  'searchDlogs'. However, the incomplete substitution is not just a question
**  of verbosity, but of data loss. The search is successful, only the
**  replacement fails due to an internal limitation of NEdit.
**
**  The parameters 'parent' and 'display' are only used to put dialogs and
**  beeps at the right place.
**
**  The result is either predetermined by the resource or the user's choice.
*/
static bool prefOrUserCancelsSubst(Fl_Widget* parent)
{
   bool cancel = true;
   unsigned confirmResult = 0;

   switch (GetPrefTruncSubstitution())
   {
   case TRUNCSUBST_SILENT:
      /*  silently fail the operation  */
      cancel = true;
      break;

   case TRUNCSUBST_FAIL:
      /*  fail the operation and pop up a dialog informing the user  */
      fl_beep();
      DialogF(DF_INF, parent, 1, "Substitution Failed",
              "The result length of the substitution exceeded an internal limit.\n"
              "The substitution is canceled.",
              "OK");
      cancel = true;
      break;

   case TRUNCSUBST_WARN:
      /*  pop up dialog and ask for confirmation  */
      fl_beep();
      confirmResult = DialogF(DF_WARN, parent, 2,
                              "Substitution Failed",
                              "The result length of the substitution exceeded an internal limit.\n"
                              "Executing the substitution will result in loss of data.",
                              "Lose Data", "Cancel");
      cancel = (1 != confirmResult);
      break;

   case TRUNCSUBST_IGNORE:
      /*  silently conclude the operation; THIS WILL DESTROY DATA.  */
      cancel = false;
      break;
   }

   return cancel;
}

/*
** Replace all occurrences of "searchString" in "window" with "replaceString"
** within the current primary selection in "window". Also adds the search and
** replace strings to the global search history.
*/
void ReplaceInSelection(const WindowInfo* window, const char* searchString,
                        const char* replaceString, const int searchType)
{
   int selStart, selEnd, beginPos, startPos, endPos, realOffset, replaceLen;
   int found, isRect, rectStart, rectEnd, lineStart, cursorPos;
   int extentBW, extentFW;
   char* fileString;
   Ne_Text_Buffer* tempBuf;
   bool substSuccess = false;
   bool anyFound = false;
   bool cancelSubst = true;

   /* save a copy of search and replace strings in the search history */
   saveSearchHistory(searchString, replaceString, searchType, false);

   /* find out where the selection is */
   if (!BufGetSelectionPos(window->buffer, &selStart, &selEnd, &isRect, &rectStart, &rectEnd))
      return;

   /* get the selected text */
   if (isRect)
   {
      selStart = BufStartOfLine(window->buffer, selStart);
      selEnd = BufEndOfLine(window->buffer, selEnd);
      fileString = BufGetRange(window->buffer, selStart, selEnd);
   }
   else
      fileString = BufGetSelectionText(window->buffer);

   /* create a temporary buffer in which to do the replacements to hide the
      intermediate steps from the display routines, and so everything can
      be undone in a single operation */
   tempBuf = BufCreate();
   BufSetAll(tempBuf, fileString);

   /* search the string and do the replacements in the temporary buffer */
   replaceLen = strlen(replaceString);
   found = true;
   beginPos = 0;
   cursorPos = 0;
   realOffset = 0;
   while (found)
   {
      found = SearchString(fileString, searchString, SEARCH_FORWARD,
                           searchType, false, beginPos, &startPos, &endPos, &extentBW,
                           &extentFW, GetWindowDelimiters(window));
      if (!found)
         break;

      anyFound = true;
      /* if the selection is rectangular, verify that the found
         string is in the rectangle */
      if (isRect)
      {
         lineStart = BufStartOfLine(window->buffer, selStart+startPos);
         if (BufCountDispChars(window->buffer, lineStart, selStart+startPos) <
               rectStart || BufCountDispChars(window->buffer, lineStart,
                                              selStart+endPos) > rectEnd)
         {
            if (fileString[endPos] == '\0')
               break;
            /* If the match starts before the left boundary of the
               selection, and extends past it, we should not continue
               search after the end of the (false) match, because we
               could miss a valid match starting between the left boundary
               and the end of the false match. */
            if (BufCountDispChars(window->buffer, lineStart,
                                  selStart+startPos) < rectStart &&
                  BufCountDispChars(window->buffer, lineStart,
                                    selStart+endPos) > rectStart)
               beginPos += 1;
            else
               beginPos = (startPos == endPos) ? endPos+1 : endPos;
            continue;
         }
      }

      /* Make sure the match did not start past the end (regular expressions
         can consider the artificial end of the range as the end of a line,
         and match a fictional whole line beginning there) */
      if (startPos == (selEnd - selStart))
      {
         found = false;
         break;
      }

      /* replace the string and compensate for length change */
      if (isRegexType(searchType))
      {
         char replaceResult[SEARCHMAX], *foundString;
         foundString = BufGetRange(tempBuf, extentBW+realOffset,
                                   extentFW+realOffset+1);
         substSuccess = replaceUsingRE(searchString, replaceString,
                                       foundString, startPos - extentBW, replaceResult, SEARCHMAX,
                                       0 == (startPos + realOffset)
                                       ? '\0'
                                       : BufGetCharacter(tempBuf, startPos + realOffset - 1),
                                       GetWindowDelimiters(window), defaultRegexFlags(searchType));
         delete[] foundString;

         if (!substSuccess)
         {
            /*  The substitution failed. Primary reason for this would be
                a result string exceeding SEARCHMAX. */

            cancelSubst = prefOrUserCancelsSubst(window->mainWindow);

            if (cancelSubst)
            {
               /*  No point in trying other substitutions.  */
               break;
            }
         }

         BufReplace(tempBuf, startPos+realOffset, endPos+realOffset, replaceResult);
         replaceLen = strlen(replaceResult);
      }
      else
      {
         /* at this point plain substitutions (should) always work */
         BufReplace(tempBuf, startPos+realOffset, endPos+realOffset,
                    replaceString);
         substSuccess = true;
      }

      realOffset += replaceLen - (endPos - startPos);
      /* start again after match unless match was empty, then endPos+1 */
      beginPos = (startPos == endPos) ? endPos+1 : endPos;
      cursorPos = endPos;
      if (fileString[endPos] == '\0')
         break;
   }
   delete[] fileString;

   if (anyFound)
   {
      if (substSuccess || !cancelSubst)
      {
         /*  Either the substitution was successful (the common case) or the
             user does not care and wants to have a faulty replacement.  */

         /* replace the selected range in the real buffer */
         BufReplace(window->buffer, selStart, selEnd, BufAsString(tempBuf));

         /* set the insert point at the end of the last replacement */
         TextSetCursorPos(window->lastFocus, selStart + cursorPos + realOffset);

         /* leave non-rectangular selections selected (rect. ones after replacement
            are less useful since left/right positions are randomly adjusted) */
         if (!isRect)
         {
            BufSelect(window->buffer, selStart, selEnd + realOffset);
         }
      }
   }
   else
   {
      /*  Nothing found, tell the user about it  */
      if (GetPrefSearchDlogs())
      {
         /* Avoid bug in Motif 1.1 by putting away search dialog
            before DialogF */
         if (window->findDlog && window->findDlog->visible() &&
               !NeToggleButtonGetState(window->findKeepBtn))
            window->findDlog->hide();
         if (window->replaceDlog && window->replaceDlog->visible() &&
               !NeToggleButtonGetState(window->replaceKeepBtn))
            unmanageReplaceDialogs(window);
         DialogF(DF_INF, window->mainWindow, 1, "String not found", "String was not found", "OK");
      }
      else
         fl_beep();
   }

   BufFree(tempBuf);
   return;
}

/*
** Replace all occurrences of "searchString" in "window" with "replaceString".
** Also adds the search and replace strings to the global search history.
*/
int ReplaceAll(WindowInfo* window, const char* searchString, const char* replaceString, int searchType)
{
   const char* fileString;
   char* newFileString;
   int copyStart, copyEnd, replacementLen;

   /* reject empty string */
   if (*searchString == '\0')
      return false;

   /* save a copy of search and replace strings in the search history */
   saveSearchHistory(searchString, replaceString, searchType, false);

   /* view the entire text buffer from the text area widget as a string */
   fileString = BufAsString(window->buffer);

   newFileString = ReplaceAllInString(fileString, searchString, replaceString,
                                      searchType, &copyStart, &copyEnd, &replacementLen,
                                      GetWindowDelimiters(window));

   if (newFileString == NULL)
   {
      if (window->multiFileBusy)
      {
         window->replaceFailed = true; /* only needed during multi-file replacements */
      }
      else if (GetPrefSearchDlogs())
      {
         if (window->findDlog && window->findDlog->visible() && !NeToggleButtonGetState(window->findKeepBtn))
            window->findDlog->hide();
         if (window->replaceDlog && window->replaceDlog->visible() && !NeToggleButtonGetState(window->replaceKeepBtn))
            unmanageReplaceDialogs(window);
         DialogF(DF_INF, window->mainWindow, 1, "String not found", "String was not found", "OK");
      }
      else
         fl_beep();
      return false;
   }

   /* replace the contents of the text widget with the substituted text */
   BufReplace(window->buffer, copyStart, copyEnd, newFileString);

   /* Move the cursor to the end of the last replacement */
   TextSetCursorPos(window->lastFocus, copyStart + replacementLen);

   delete[] newFileString;
   return true;
}

/*
** Replace all occurrences of "searchString" in "inString" with "replaceString"
** and return an allocated string covering the range between the start of the
** first replacement (returned in "copyStart", and the end of the last
** replacement (returned in "copyEnd")
*/
char* ReplaceAllInString(const char* inString, const char* searchString,
                         const char* replaceString, int searchType, int* copyStart,
                         int* copyEnd, int* replacementLength, const char* delimiters)
{
   int beginPos, startPos, endPos, lastEndPos;
   int found, nFound, removeLen, replaceLen, copyLen, addLen;
   char* outString, *fillPtr;
   int searchExtentBW, searchExtentFW;

   /* reject empty string */
   if (*searchString == '\0')
      return NULL;

   /* rehearse the search first to determine the size of the buffer needed
      to hold the substituted text.  No substitution done here yet */
   replaceLen = strlen(replaceString);
   found = true;
   nFound = 0;
   removeLen = 0;
   addLen = 0;
   beginPos = 0;
   *copyStart = -1;
   while (found)
   {
      found = SearchString(inString, searchString, SEARCH_FORWARD, searchType,
                           false, beginPos, &startPos, &endPos, &searchExtentBW,
                           &searchExtentFW, delimiters);
      if (found)
      {
         if (*copyStart < 0)
            *copyStart = startPos;
         *copyEnd = endPos;
         /* start next after match unless match was empty, then endPos+1 */
         beginPos = (startPos == endPos) ? endPos+1 : endPos;
         nFound++;
         removeLen += endPos - startPos;
         if (isRegexType(searchType))
         {
            char replaceResult[SEARCHMAX];
            replaceUsingRE(searchString, replaceString, &inString[searchExtentBW],
                           startPos-searchExtentBW,
                           replaceResult, SEARCHMAX, startPos == 0 ? '\0' :
                           inString[startPos-1], delimiters,
                           defaultRegexFlags(searchType));
            addLen += strlen(replaceResult);
         }
         else
            addLen += replaceLen;
         if (inString[endPos] == '\0')
            break;
      }
   }
   if (nFound == 0)
      return NULL;

   /* Allocate a new buffer to hold all of the new text between the first
      and last substitutions */
   copyLen = *copyEnd - *copyStart;
   outString = (char*)malloc__(copyLen - removeLen + addLen + 1);

   /* Scan through the text buffer again, substituting the replace string
      and copying the part between replaced text to the new buffer  */
   found = true;
   beginPos = 0;
   lastEndPos = 0;
   fillPtr = outString;
   while (found)
   {
      found = SearchString(inString, searchString, SEARCH_FORWARD, searchType,
                           false, beginPos, &startPos, &endPos, &searchExtentBW,
                           &searchExtentFW, delimiters);
      if (found)
      {
         if (beginPos != 0)
         {
            memcpy(fillPtr, &inString[lastEndPos], startPos - lastEndPos);
            fillPtr += startPos - lastEndPos;
         }
         if (isRegexType(searchType))
         {
            char replaceResult[SEARCHMAX];
            replaceUsingRE(searchString, replaceString, &inString[searchExtentBW],
                           startPos-searchExtentBW,
                           replaceResult, SEARCHMAX, startPos == 0 ? '\0' :
                           inString[startPos-1], delimiters,
                           defaultRegexFlags(searchType));
            replaceLen = strlen(replaceResult);
            memcpy(fillPtr, replaceResult, replaceLen);
         }
         else
         {
            memcpy(fillPtr, replaceString, replaceLen);
         }
         fillPtr += replaceLen;
         lastEndPos = endPos;
         /* start next after match unless match was empty, then endPos+1 */
         beginPos = (startPos == endPos) ? endPos+1 : endPos;
         if (inString[endPos] == '\0')
            break;
      }
   }
   *fillPtr = '\0';
   *replacementLength = fillPtr - outString;
   return outString;
}

/*
** If this is an incremental search and BeepOnSearchWrap is on:
** Emit a beep if the search wrapped over BOF/EOF compared to
** the last startPos of the current incremental search.
*/
static void iSearchTryBeepOnWrap(WindowInfo* window, int direction,
                                 int beginPos, int startPos)
{
   if (GetPrefBeepOnSearchWrap())
   {
      if (direction == SEARCH_FORWARD)
      {
         if ((startPos >= beginPos
               && window->iSearchLastBeginPos < beginPos)
               ||(startPos < beginPos
                  && window->iSearchLastBeginPos >= beginPos))
         {
            fl_beep();
         }
      }
      else
      {
         if ((startPos <= beginPos
               && window->iSearchLastBeginPos > beginPos)
               ||(startPos > beginPos
                  && window->iSearchLastBeginPos <= beginPos))
         {
            fl_beep();
         }
      }
   }
}

/*
** Search the text in "window", attempting to match "searchString"
*/
int SearchWindow(WindowInfo* window, int direction, const char* searchString,
                 int searchType, int searchWrap, int beginPos, int* startPos,
                 int* endPos, int* extentBW, int* extentFW)
{
   const char* fileString;
   int found, resp, fileEnd = window->buffer->length - 1, outsideBounds;

   /* reject empty string */
   if (*searchString == '\0')
      return false;

   /* get the entire text buffer from the text area widget */
   fileString = BufAsString(window->buffer);

   /* If we're already outside the boundaries, we must consider wrapping
      immediately (Note: fileEnd+1 is a valid starting position. Consider
      searching for $ at the end of a file ending with \n.) */
   if ((direction == SEARCH_FORWARD && beginPos > fileEnd + 1)
         || (direction == SEARCH_BACKWARD && beginPos < 0))
   {
      outsideBounds = true;
   }
   else
   {
      outsideBounds = false;
   }

   /* search the string copied from the text area widget, and present
      dialogs, or just beep.  iSearchStartPos is not a perfect indicator that
      an incremental search is in progress.  A parameter would be better. */
   if (window->iSearchStartPos == -1)   /* normal search */
   {
      found = !outsideBounds && SearchString(fileString, searchString, direction, searchType,
                                             false, beginPos, startPos, endPos, extentBW, extentFW,
                                             GetWindowDelimiters(window));
      /* Avoid Motif 1.1 bug by putting away search dialog before DialogF */
// TODO:       if (window->findDlog && XtIsManaged(window->findDlog) && !NeToggleButtonGetState(window->findKeepBtn))
// TODO:          XtUnmanageChild(window->findDlog);
// TODO:       if (window->replaceDlog && XtIsManaged(window->replaceDlog) && !NeToggleButtonGetState(window->replaceKeepBtn))
// TODO:          unmanageReplaceDialogs(window);
      if (!found)
      {
         if (searchWrap)
         {
            if (direction == SEARCH_FORWARD && beginPos != 0)
            {
               if (GetPrefBeepOnSearchWrap())
               {
                  fl_beep();
               }
               else if (GetPrefSearchDlogs())
               {
                  resp = DialogF(DF_QUES, window->mainWindow, 2, "Wrap Search",
                                 "Continue search from\nbeginning of file?",
                                 "Continue", "Cancel");
                  if (resp == 2)
                  {
                     return false;
                  }
               }
               found = SearchString(fileString, searchString, direction,
                                    searchType, false, 0, startPos, endPos, extentBW,
                                    extentFW, GetWindowDelimiters(window));
            }
            else if (direction == SEARCH_BACKWARD && beginPos != fileEnd)
            {
               if (GetPrefBeepOnSearchWrap())
               {
                  fl_beep();
               }
               else if (GetPrefSearchDlogs())
               {
                  resp = DialogF(DF_QUES, window->mainWindow, 2, "Wrap Search",
                                 "Continue search\nfrom end of file?", "Continue",
                                 "Cancel");
                  if (resp == 2)
                  {
                     return false;
                  }
               }
               found = SearchString(fileString, searchString, direction,
                                    searchType, false, fileEnd + 1, startPos, endPos, extentBW,
                                    extentFW, GetWindowDelimiters(window));
            }
         }
         if (!found)
         {
            if (GetPrefSearchDlogs())
            {
               DialogF(DF_INF, window->mainWindow, 1, "String not found",
                       "String was not found","OK");
            }
            else
            {
               fl_beep();
            }
         }
      }
   }
   else     /* incremental search */
   {
      if (outsideBounds && searchWrap)
      {
         if (direction == SEARCH_FORWARD) beginPos = 0;
         else beginPos = fileEnd+1;
         outsideBounds = false;
      }
      found = !outsideBounds &&
              SearchString(fileString, searchString, direction,
                           searchType, searchWrap, beginPos, startPos, endPos,
                           extentBW, extentFW, GetWindowDelimiters(window));
      if (found)
      {
         iSearchTryBeepOnWrap(window, direction, beginPos, *startPos);
      }
      else
         fl_beep();
   }

   return found;
}

/*
** Search the null terminated string "string" for "searchString", beginning at
** "beginPos".  Returns the boundaries of the match in "startPos" and "endPos".
** searchExtentBW and searchExtentFW return the backward most and forward most
** positions used to make the match, which are usually startPos and endPos,
** but may extend further if positive look ahead or look behind was used in
** a regular expression match.  "delimiters" may be used to provide an
** alternative set of word delimiters for regular expression "<" and ">"
** characters, or simply passed as null for the default delimiter set.
*/
int SearchString(const char* string, const char* searchString, int direction,
                 int searchType, int wrap, int beginPos, int* startPos, int* endPos,
                 int* searchExtentBW, int* searchExtentFW, const char* delimiters)
{
   switch (searchType)
   {
   case SEARCH_CASE_SENSE_WORD:
      return searchLiteralWord(string, searchString, true,  direction, wrap, beginPos, startPos, endPos, delimiters);
   case SEARCH_LITERAL_WORD:
      return  searchLiteralWord(string, searchString, false, direction, wrap, beginPos, startPos, endPos, delimiters);
   case SEARCH_CASE_SENSE:
      return searchLiteral(string, searchString, true, direction, wrap, beginPos, startPos, endPos, searchExtentBW, searchExtentFW);
   case SEARCH_LITERAL:
      return  searchLiteral(string, searchString, false, direction, wrap, beginPos, startPos, endPos, searchExtentBW, searchExtentFW);
   case SEARCH_REGEX:
      return  searchRegex(string, searchString, direction, wrap, beginPos, startPos, endPos, searchExtentBW, searchExtentFW, delimiters, REDFLT_STANDARD);
   case SEARCH_REGEX_NOCASE:
      return  searchRegex(string, searchString, direction, wrap, beginPos, startPos, endPos, searchExtentBW, searchExtentFW, delimiters, REDFLT_CASE_INSENSITIVE);
   }
   return false; /* never reached, just makes compilers happy */
}

/*
** Parses a search type description string. If the string contains a valid
** search type description, returns true and writes the corresponding
** SearchType in searchType. Returns false and leaves searchType untouched
** otherwise. (Originally written by Markus Schwarzenberg; slightly adapted).
*/
int StringToSearchType(const char* string, int* searchType)
{
   int i;
   for (i = 0; searchTypeStrings[i]; i++)
   {
      if (!strcmp(string, searchTypeStrings[i]))
      {
         break;
      }
   }
   if (!searchTypeStrings[i])
   {
      return false;
   }
   *searchType = i;
   return true;
}

/*
**  Searches for whole words (Markus Schwarzenberg).
**
**  If the first/last character of `searchString' is a "normal
**  word character" (not contained in `delimiters', not a whitespace)
**  then limit search to strings, who's next left/next right character
**  is contained in `delimiters' or is a whitespace or text begin or end.
**
**  If the first/last character of `searchString' itself is contained
**  in delimiters or is a white space, then the neighbour character of the
**  first/last character will not be checked, just a simple match
**  will suffice in that case.
**
*/
static int searchLiteralWord(const char* string, const char* searchString, int caseSense,
                             int direction, int wrap, int beginPos, int* startPos, int* endPos,
                             const char* delimiters)
{
   /* This is critical code for the speed of searches.			    */
   /* For efficiency, we define the macro DOSEARCH with the guts of the search */
   /* routine and repeat it, changing the parameters of the outer loop for the */
   /* searching, forwards, backwards, and before and after the begin point	    */
#define DOSEARCHWORD() \
    if (*filePtr == *ucString || *filePtr == *lcString) { \
	/* matched first character */ \
	ucPtr = ucString; \
	lcPtr = lcString; \
	tempPtr = filePtr; \
	while (*tempPtr == *ucPtr || *tempPtr == *lcPtr) { \
	    tempPtr++; ucPtr++; lcPtr++; \
	    if (   *ucPtr == 0 /* matched whole string */ \
		&& (cignore_R ||\
		    isspace((unsigned char)*tempPtr) ||\
		    strchr(delimiters, *tempPtr) ) \
		    /* next char right delimits word ? */ \
		&& (cignore_L ||\
                    filePtr==string || /* border case */ \
                    isspace((unsigned char)filePtr[-1]) ||\
                    strchr(delimiters,filePtr[-1]) ))\
                    /* next char left delimits word ? */ { \
		*startPos = filePtr - string; \
		*endPos = tempPtr - string; \
		return true; \
	    } \
	} \
    }

   register const char* filePtr, *tempPtr, *ucPtr, *lcPtr;
   char lcString[SEARCHMAX], ucString[SEARCHMAX];

   int cignore_L=0, cignore_R=0;

   /* SEARCHMAX was fine in the original NEdit, but it should be done away
      with now that searching can be done from macros without limits.
      Returning search failure here is cheating users.  This limit is not
      documented. */
   if (strlen(searchString) >= SEARCHMAX)
      return false;

   /* If there is no language mode, we use the default list of delimiters */
   if (delimiters==NULL) delimiters = GetPrefDelimiters();

   if (isspace((unsigned char)*searchString)
         || strchr(delimiters, *searchString))
      cignore_L=1;

   if (isspace((unsigned char)searchString[strlen(searchString)-1])
         || strchr(delimiters, searchString[strlen(searchString)-1]))
      cignore_R=1;

   if (caseSense)
   {
      strcpy(ucString, searchString);
      strcpy(lcString, searchString);
   }
   else
   {
      upCaseString(ucString, searchString);
      downCaseString(lcString, searchString);
   }

   if (direction == SEARCH_FORWARD)
   {
      /* search from beginPos to end of string */
      for (filePtr=string+beginPos; *filePtr!=0; filePtr++)
      {
         DOSEARCHWORD()
      }
      if (!wrap)
         return false;

      /* search from start of file to beginPos */
      for (filePtr=string; filePtr<=string+beginPos; filePtr++)
      {
         DOSEARCHWORD()
      }
      return false;
   }
   else
   {
      /* SEARCH_BACKWARD */
      /* search from beginPos to start of file. A negative begin pos */
      /* says begin searching from the far end of the file */
      if (beginPos >= 0)
      {
         for (filePtr=string+beginPos; filePtr>=string; filePtr--)
         {
            DOSEARCHWORD()
         }
      }
      if (!wrap)
         return false;
      /* search from end of file to beginPos */
      /*... this strlen call is extreme inefficiency, but it's not obvious */
      /* how to get the text string length from the text widget (under 1.1)*/
      for (filePtr=string+strlen(string); filePtr>=string+beginPos; filePtr--)
      {
         DOSEARCHWORD()
      }
      return false;
   }
}


static int searchLiteral(const char* string, const char* searchString, int caseSense,
                         int direction, int wrap, int beginPos, int* startPos, int* endPos,
                         int* searchExtentBW, int* searchExtentFW)
{
   /* This is critical code for the speed of searches.			    */
   /* For efficiency, we define the macro DOSEARCH with the guts of the search */
   /* routine and repeat it, changing the parameters of the outer loop for the */
   /* searching, forwards, backwards, and before and after the begin point	    */
#define DOSEARCH() \
    if (*filePtr == *ucString || *filePtr == *lcString) { \
	/* matched first character */ \
	ucPtr = ucString; \
	lcPtr = lcString; \
	tempPtr = filePtr; \
	while (*tempPtr == *ucPtr || *tempPtr == *lcPtr) { \
	    tempPtr++; ucPtr++; lcPtr++; \
	    if (*ucPtr == 0) { \
		/* matched whole string */ \
		*startPos = filePtr - string; \
		*endPos = tempPtr - string; \
		if (searchExtentBW != NULL) \
		    *searchExtentBW = *startPos; \
		if (searchExtentFW != NULL) \
		    *searchExtentFW = *endPos; \
		return true; \
	    } \
	} \
    } \
 
   register const char* filePtr, *tempPtr, *ucPtr, *lcPtr;
   char lcString[SEARCHMAX], ucString[SEARCHMAX];

   /* SEARCHMAX was fine in the original NEdit, but it should be done away with
      now that searching can be done from macros without limits.  Returning
      search failure here is cheating users.  This limit is not documented. */
   if (strlen(searchString) >= SEARCHMAX)
      return false;

   if (caseSense)
   {
      strcpy(ucString, searchString);
      strcpy(lcString, searchString);
   }
   else
   {
      upCaseString(ucString, searchString);
      downCaseString(lcString, searchString);
   }

   if (direction == SEARCH_FORWARD)
   {
      /* search from beginPos to end of string */
      for (filePtr=string+beginPos; *filePtr!=0; filePtr++)
      {
         DOSEARCH()
      }
      if (!wrap)
         return false;
      /* search from start of file to beginPos	*/
      for (filePtr=string; filePtr<=string+beginPos; filePtr++)
      {
         DOSEARCH()
      }
      return false;
   }
   else
   {
      /* SEARCH_BACKWARD */
      /* search from beginPos to start of file.  A negative begin pos	*/
      /* says begin searching from the far end of the file		*/
      if (beginPos >= 0)
      {
         for (filePtr=string+beginPos; filePtr>=string; filePtr--)
         {
            DOSEARCH()
         }
      }
      if (!wrap)
         return false;
      /* search from end of file to beginPos */
      /*... this strlen call is extreme inefficiency, but it's not obvious */
      /* how to get the text string length from the text widget (under 1.1)*/
      for (filePtr=string+strlen(string);
            filePtr>=string+beginPos; filePtr--)
      {
         DOSEARCH()
      }
      return false;
   }
}

static int searchRegex(const char* string, const char* searchString, int direction,
                       int wrap, int beginPos, int* startPos, int* endPos, int* searchExtentBW,
                       int* searchExtentFW, const char* delimiters, int defaultFlags)
{
   if (direction == SEARCH_FORWARD)
      return forwardRegexSearch(string, searchString, wrap,
                                beginPos, startPos, endPos, searchExtentBW, searchExtentFW,
                                delimiters, defaultFlags);
   else
      return backwardRegexSearch(string, searchString, wrap,
                                 beginPos, startPos, endPos, searchExtentBW, searchExtentFW,
                                 delimiters, defaultFlags);
}

static int forwardRegexSearch(const char* string, const char* searchString, int wrap,
                              int beginPos, int* startPos, int* endPos, int* searchExtentBW,
                              int* searchExtentFW, const char* delimiters, int defaultFlags)
{
   regexp* compiledRE = NULL;
   char* compileMsg;

   /* compile the search string for searching with ExecRE.  Note that
      this does not process errors from compiling the expression.  It
      assumes that the expression was checked earlier. */
   compiledRE = CompileRE(searchString, &compileMsg, defaultFlags);
   if (compiledRE == NULL)
      return false;

   /* search from beginPos to end of string */
   if (ExecRE(compiledRE, string + beginPos, NULL, false,
              (beginPos == 0) ? '\0' : string[beginPos-1], '\0', delimiters,
              string, NULL))
   {
      *startPos = compiledRE->startp[0] - string;
      *endPos = compiledRE->endp[0] - string;
      if (searchExtentFW != NULL)
         *searchExtentFW = compiledRE->extentpFW - string;
      if (searchExtentBW != NULL)
         *searchExtentBW = compiledRE->extentpBW - string;
      free__((char*)compiledRE);
      return true;
   }

   /* if wrap turned off, we're done */
   if (!wrap)
   {
      free__((char*)compiledRE);
      return false;
   }

   /* search from the beginning of the string to beginPos */
   if (ExecRE(compiledRE, string, string + beginPos, false, '\0',
              string[beginPos], delimiters, string, NULL))
   {
      *startPos = compiledRE->startp[0] - string;
      *endPos = compiledRE->endp[0] - string;
      if (searchExtentFW != NULL)
         *searchExtentFW = compiledRE->extentpFW - string;
      if (searchExtentBW != NULL)
         *searchExtentBW = compiledRE->extentpBW - string;
      free__((char*)compiledRE);
      return true;
   }

   free__((char*)compiledRE);
   return false;
}

static int backwardRegexSearch(const char* string, const char* searchString, int wrap,
                               int beginPos, int* startPos, int* endPos, int* searchExtentBW,
                               int* searchExtentFW, const char* delimiters, int defaultFlags)
{
   regexp* compiledRE = NULL;
   char* compileMsg;
   int length;

   /* compile the search string for searching with ExecRE */
   compiledRE = CompileRE(searchString, &compileMsg, defaultFlags);
   if (compiledRE == NULL)
      return false;

   /* search from beginPos to start of file.  A negative begin pos	*/
   /* says begin searching from the far end of the file.		*/
   if (beginPos >= 0)
   {
      if (ExecRE(compiledRE, string, string + beginPos, true, '\0', '\0',
                 delimiters, string, NULL))
      {
         *startPos = compiledRE->startp[0] - string;
         *endPos = compiledRE->endp[0] - string;
         if (searchExtentFW != NULL)
            *searchExtentFW = compiledRE->extentpFW - string;
         if (searchExtentBW != NULL)
            *searchExtentBW = compiledRE->extentpBW - string;
         free__((char*)compiledRE);
         return true;
      }
   }

   /* if wrap turned off, we're done */
   if (!wrap)
   {
      free__((char*)compiledRE);
      return false;
   }

   /* search from the end of the string to beginPos */
   if (beginPos < 0)
      beginPos = 0;
   length = strlen(string); /* sadly, this means scanning entire string */
   if (ExecRE(compiledRE, string + beginPos, string + length, true,
              (beginPos == 0) ? '\0' : string[beginPos-1], '\0', delimiters,
              string, NULL))
   {
      *startPos = compiledRE->startp[0] - string;
      *endPos = compiledRE->endp[0] - string;
      if (searchExtentFW != NULL)
         *searchExtentFW = compiledRE->extentpFW - string;
      if (searchExtentBW != NULL)
         *searchExtentBW = compiledRE->extentpBW - string;
      free__((char*)compiledRE);
      return true;
   }
   free__((char*)compiledRE);
   return false;
}

static void upCaseString(char* outString, const char* inString)
{
   char* outPtr;
   const char* inPtr;

   for (outPtr=outString, inPtr=inString; *inPtr!=0; inPtr++, outPtr++)
   {
      *outPtr = toupper((unsigned char)*inPtr);
   }
   *outPtr = 0;
}

static void downCaseString(char* outString, const char* inString)
{
   char* outPtr;
   const char* inPtr;

   for (outPtr=outString, inPtr=inString; *inPtr!=0; inPtr++, outPtr++)
   {
      *outPtr = tolower((unsigned char)*inPtr);
   }
   *outPtr = 0;
}

/*
** resetFindTabGroup & resetReplaceTabGroup are really gruesome kludges to
** set the keyboard traversal.  XmProcessTraversal does not work at
** all on these dialogs.  ...It seems to have started working around
** Motif 1.1.2
*/
static void resetFindTabGroup(WindowInfo* window)
{
   window->findText->take_focus();
}
static void resetReplaceTabGroup(WindowInfo* window)
{
   window->replaceDlog->focus(window->replaceText);
}

/*
** Return true if "searchString" exactly matches the text in the window's
** current primary selection using search algorithm "searchType".  If true,
** also return the position of the selection in "left" and "right".
*/
static int searchMatchesSelection(WindowInfo* window, const char* searchString,
                                  int searchType, int* left, int* right, int* searchExtentBW,
                                  int* searchExtentFW)
{
   int selLen, selStart, selEnd, startPos, endPos, extentBW, extentFW, beginPos;
   int regexLookContext = isRegexType(searchType) ? 1000 : 0;
   char* string;
   int found, isRect, rectStart, rectEnd, lineStart = 0;

   /* find length of selection, give up on no selection or too long */
   if (!BufGetEmptySelectionPos(window->buffer, &selStart, &selEnd, &isRect,
                                &rectStart, &rectEnd))
      return false;
   if (selEnd - selStart > SEARCHMAX)
      return false;

   /* if the selection is rectangular, don't match if it spans lines */
   if (isRect)
   {
      lineStart = BufStartOfLine(window->buffer, selStart);
      if (lineStart != BufStartOfLine(window->buffer, selEnd))
         return false;
   }

   /* get the selected text plus some additional context for regular
      expression lookahead */
   if (isRect)
   {
      int stringStart = lineStart + rectStart - regexLookContext;
      if (stringStart < 0) stringStart = 0;
      string = BufGetRange(window->buffer, stringStart, lineStart + rectEnd + regexLookContext);
      selLen = rectEnd - rectStart;
      beginPos = lineStart + rectStart - stringStart;
   }
   else
   {
      int stringStart = selStart - regexLookContext;
      if (stringStart < 0) stringStart = 0;
      string = BufGetRange(window->buffer, stringStart, selEnd + regexLookContext);
      selLen = selEnd - selStart;
      beginPos = selStart - stringStart;
   }
   if (*string == '\0')
   {
      delete[] string;
      return false;
   }

   /* search for the string in the selection (we are only interested 	*/
   /* in an exact match, but the procedure SearchString does important */
   /* stuff like applying the correct matching algorithm)		*/
   found = SearchString(string, searchString, SEARCH_FORWARD, searchType,
                        false, beginPos, &startPos, &endPos, &extentBW, &extentFW,
                        GetWindowDelimiters(window));
   delete[] string;

   /* decide if it is an exact match */
   if (!found)
      return false;
   if (startPos != beginPos || endPos - beginPos != selLen)
      return false;

   /* return the start and end of the selection */
   if (isRect)
      GetSimpleSelection(window->buffer, left, right);
   else
   {
      *left = selStart;
      *right = selEnd;
   }
   if (searchExtentBW != NULL)
      *searchExtentBW = *left - (startPos - extentBW);

   if (searchExtentFW != NULL)
      *searchExtentFW = *right + extentFW - endPos;
   return true;
}

/*
** Substitutes a replace string for a string that was matched using a
** regular expression.  This was added later and is rather inefficient
** because instead of using the compiled regular expression that was used
** to make the match in the first place, it re-compiles the expression
** and redoes the search on the already-matched string.  This allows the
** code to continue using strings to represent the search and replace
** items.
*/
static bool replaceUsingRE(const char* searchStr, const char* replaceStr,
                              const char* sourceStr, const int beginPos, char* destStr,
                              const int maxDestLen, const int prevChar, const char* delimiters,
                              const int defaultFlags)
{
   regexp* compiledRE;
   char* compileMsg;
   bool substResult = false;

   compiledRE = CompileRE(searchStr, &compileMsg, defaultFlags);
   ExecRE(compiledRE, sourceStr+beginPos, NULL, false, prevChar, '\0', delimiters, sourceStr, NULL);
   substResult = SubstituteRE(compiledRE, replaceStr, destStr, maxDestLen);
   free__((char*)compiledRE);

   return substResult;
}

/*
** Store the search and replace strings, and search type for later recall.
** If replaceString is NULL, duplicate the last replaceString used.
** Contiguous incremental searches share the same history entry (each new
** search modifies the current search string, until a non-incremental search
** is made.  To mark the end of an incremental search, call saveSearchHistory
** again with an empty search string and isIncremental==false.
*/
static void saveSearchHistory(const char* searchString, const char* replaceString, int searchType, int isIncremental)
{
   char* sStr, *rStr;
   static int currentItemIsIncremental = false;
   WindowInfo* w;

   /* Cancel accumulation of contiguous incremental searches (even if the
      information is not worthy of saving) if search is not incremental */
   if (!isIncremental)
      currentItemIsIncremental = false;

   /* Don't save empty search strings */
   if (searchString[0] == '\0')
      return;

   /* If replaceString is NULL, duplicate the last one (if any) */
   if (replaceString == NULL)
      replaceString = NHist >= 1 ? ReplaceHistory[historyIndex(1)] : "";

   /* Compare the current search and replace strings against the saved ones.
      If they are identical, don't bother saving */
   if (NHist >= 1 && searchType == SearchTypeHistory[historyIndex(1)] &&
         !strcmp(SearchHistory[historyIndex(1)], searchString) &&
         !strcmp(ReplaceHistory[historyIndex(1)], replaceString))
   {
      return;
   }

   /* If the current history item came from an incremental search, and the
      new one is also incremental, just update the entry */
   if (currentItemIsIncremental && isIncremental)
   {
      delete[] (SearchHistory[historyIndex(1)]);
      SearchHistory[historyIndex(1)] = NeNewString(searchString);
      SearchTypeHistory[historyIndex(1)] = searchType;
      return;
   }
   currentItemIsIncremental = isIncremental;

   if (NHist==0)
   {
      for (w=WindowList; w!=NULL; w=w->next)
      {
         if (!IsTopDocument(w))
            continue;
         NeSetSensitive(w->menuBar->getFindAgainItem(), true);
         NeSetSensitive(w->menuBar->getReplaceFindAgainItem(), true);
         NeSetSensitive(w->menuBar->getReplaceAgainItem(), true);
      }
   }

   /* If there are more than MAX_SEARCH_HISTORY strings saved, recycle
      some space, free__ the entry that's about to be overwritten */
   if (NHist == MAX_SEARCH_HISTORY)
   {
      delete[] (SearchHistory[HistStart]);
      delete[] (ReplaceHistory[HistStart]);
   }
   else
      NHist++;

   /* Allocate and copy the search and replace strings and add them to the
      circular buffers at HistStart, bump the buffer pointer to next pos. */
   sStr = (char*)malloc__(strlen(searchString) + 1);
   rStr = (char*)malloc__(strlen(replaceString) + 1);
   strcpy(sStr, searchString);
   strcpy(rStr, replaceString);
   SearchHistory[HistStart] = sStr;
   ReplaceHistory[HistStart] = rStr;
   SearchTypeHistory[HistStart] = searchType;
   HistStart++;
   if (HistStart >= MAX_SEARCH_HISTORY)
      HistStart = 0;
}

/*
** return an index into the circular buffer arrays of history information
** for search strings, given the number of saveSearchHistory cycles back from
** the current time.
*/
static int historyIndex(int nCycles)
{
   int index;

   if (nCycles > NHist || nCycles <= 0)
      return -1;
   index = HistStart - nCycles;
   if (index < 0)
      index = MAX_SEARCH_HISTORY + index;
   return index;
}

/*
** Return a pointer to the string describing search type for search action
** routine parameters (see menu.c for processing of action routines)
*/
static const char* searchTypeArg(int searchType)
{
   if (0 <= searchType && searchType < N_SEARCH_TYPES)
   {
      return searchTypeStrings[searchType];
   }
   return searchTypeStrings[SEARCH_LITERAL];
}

/*
** Return a pointer to the string describing search wrap for search action
** routine parameters (see menu.c for processing of action routines)
*/
static const char* searchWrapArg(int searchWrap)
{
   if (searchWrap)
   {
      return "wrap";
   }
   return "nowrap";
}

/*
** Return a pointer to the string describing search direction for search action
** routine parameters (see menu.c for processing of action routines)
*/
static const char* directionArg(int direction)
{
   if (direction == SEARCH_BACKWARD)
      return "backward";
   return "forward";
}

// Checks whether a search mode in one of the regular expression modes.
static int isRegexType(int searchType)
{
   return searchType == SEARCH_REGEX || searchType == SEARCH_REGEX_NOCASE;
}

/*
** Returns the default flags for regular expression matching, given a
** regular expression search mode.
*/
static int defaultRegexFlags(int searchType)
{
   switch (searchType)
   {
   case SEARCH_REGEX:
      return REDFLT_STANDARD;
   case SEARCH_REGEX_NOCASE:
      return REDFLT_CASE_INSENSITIVE;
   default:
      /* We should never get here, but just in case ... */
      return REDFLT_STANDARD;
   }
}

/*
** The next 4 callbacks handle the states of find/replace toggle
** buttons, which depend on the state of the "Regex" button, and the
** sensitivity of the Whole Word buttons.
** Callbacks are necessary for both "Regex" and "Case Sensitive"
** buttons to make sure the states are saved even after a cancel operation.
**
** If sticky case sensitivity is requested, the behavior is as follows:
**   The first time "Regular expression" is checked, "Match case" gets
**   checked too. Thereafter, checking or unchecking "Regular expression"
**   restores the "Match case" button to the setting it had the last
**   time when literals or REs where used.
** Without sticky behavior, the state of the Regex button doesn't influence
** the state of the Case Sensitive button.
**
** Independently, the state of the buttons is always restored to the
** default state when a dialog is popped up, and when the user returns
** from stepping through the search history.
**
** NOTE: similar call-backs exist for the incremental search bar; see window.c.
*/
static void findRegExpToggleCB(Fl_Widget* w, void* data)
{
   TRACE();
   WindowInfo* window = static_cast<WindowInfo*>(data);
   bool searchRegex = NeToggleButtonGetState(window->findRegexToggle);
   bool searchCaseSense = NeToggleButtonGetState(window->findCaseToggle);

   /* In sticky mode, restore the state of the Case Sensitive button */
   if (GetPrefStickyCaseSenseBtn())
   {
      if (searchRegex)
      {
         window->findLastLiteralCase = searchCaseSense;
         NeToggleButtonSetState(window->findCaseToggle, window->findLastRegexCase, false);
      }
      else
      {
         window->findLastRegexCase = searchCaseSense;
         NeToggleButtonSetState(window->findCaseToggle, window->findLastLiteralCase, false);
      }
   }
   /* make the Whole Word button insensitive for regex searches */
   NeSetSensitive(window->findWordToggle, !searchRegex);
}

static void replaceRegExpToggleCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);
   int searchRegex = NeToggleButtonGetState(window->replaceRegexToggle);
   int searchCaseSense = NeToggleButtonGetState(window->replaceCaseToggle);

   /* In sticky mode, restore the state of the Case Sensitive button */
   if (GetPrefStickyCaseSenseBtn())
   {
      if (searchRegex)
      {
         window->replaceLastLiteralCase = searchCaseSense;
         NeToggleButtonSetState(window->replaceCaseToggle, window->replaceLastRegexCase, false);
      }
      else
      {
         window->replaceLastRegexCase = searchCaseSense;
         NeToggleButtonSetState(window->replaceCaseToggle, window->replaceLastLiteralCase, false);
      }
   }
   /* make the Whole Word button insensitive for regex searches */
   NeSetSensitive(window->replaceWordToggle, !searchRegex);
}

// TODO: static void iSearchRegExpToggleCB(Widget w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    int searchRegex = NeToggleButtonGetState(w);
// TODO:    int searchCaseSense = NeToggleButtonGetState(window->iSearchCaseToggle);
// TODO: 
// TODO:    /* In sticky mode, restore the state of the Case Sensitive button */
// TODO:    if (GetPrefStickyCaseSenseBtn())
// TODO:    {
// TODO:       if (searchRegex)
// TODO:       {
// TODO:          window->iSearchLastLiteralCase = searchCaseSense;
// TODO:          NeToggleButtonSetState(window->iSearchCaseToggle,
// TODO:                                 window->iSearchLastRegexCase, false);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          window->iSearchLastRegexCase = searchCaseSense;
// TODO:          NeToggleButtonSetState(window->iSearchCaseToggle,
// TODO:                                 window->iSearchLastLiteralCase, false);
// TODO:       }
// TODO:    }
// TODO:    /* The iSearch bar has no Whole Word button to enable/disable. */
// TODO: }

static void findCaseToggleCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);
   bool searchCaseSense = NeToggleButtonGetState(window->findCaseToggle);

   /* Save the state of the Case Sensitive button
      depending on the state of the Regex button*/
   if (NeToggleButtonGetState(window->findRegexToggle))
      window->findLastRegexCase = searchCaseSense;
   else
      window->findLastLiteralCase = searchCaseSense;
}

static void replaceCaseToggleCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = static_cast<WindowInfo*>(data);
   int searchCaseSense = NeToggleButtonGetState(window->replaceCaseToggle);

   /* Save the state of the Case Sensitive button
      depending on the state of the Regex button*/
   if (NeToggleButtonGetState(window->replaceRegexToggle))
      window->replaceLastRegexCase = searchCaseSense;
   else
      window->replaceLastLiteralCase = searchCaseSense;
}

// TODO: static void iSearchCaseToggleCB(Widget w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    int searchCaseSense = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Save the state of the Case Sensitive button
// TODO:       depending on the state of the Regex button*/
// TODO:    if (NeToggleButtonGetState(window->iSearchRegexToggle))
// TODO:       window->iSearchLastRegexCase = searchCaseSense;
// TODO:    else
// TODO:       window->iSearchLastLiteralCase = searchCaseSense;
// TODO: }
