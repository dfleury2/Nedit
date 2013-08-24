/*******************************************************************************
*                                                                              *
* window.c -- Nirvana Editor window creation/deletion                          *
*                                                                              *
* Copyright (C) 1999 Mark Edel                                                 *
*                                                                              *
* This is free__ software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version. In addition, you may distribute version of this program linked to   *
* Motif or Open Motif. See README for details.                                 *
*                                                                              *
* This software is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
* for more details.                                                            *
*                                                                              *
* You should have received a copy of the GNU General Public License along with *
* software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
* Place, Suite 330, Boston, MA  02111-1307 USA                                 *
*                                                                              *
* Nirvana Text Editor                                                          *
* May 10, 1991                                                                 *
*                                                                              *
* Written by Mark Edel                                                         *
*                                                                              *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "window.h"
#include "resource.h"
#include "Ne_Text_Buffer.h"
#include "Ne_Text_Sel.h"
#include "Ne_Text_Editor.h"
#include "Ne_Text_Display.h"
#include "Ne_Text_Part.h"
#include "nedit.h"
#include "menu.h"
#include "file.h"
#include "search.h"
#include "undo.h"
#include "preferences.h"
#include "selection.h"
#include "server.h"
#include "shell.h"
#include "macro.h"
#include "highlight.h"
#include "smartIndent.h"
#include "userCmds.h"
//#include "nedit.bm"
//#include "n.bm"
#include "windowTitle.h"
#include "interpret.h"
#include "Ne_Rangeset.h"
#include "../util/misc.h"
#include "../util/fileUtils.h"
#include "../util/utils.h"
#include "../util/fileUtils.h"
#include "../util/DialogF.h"
#include "../util/Ne_Color.h"
#include "../util/Ne_Font.h"

#include <FL/x.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Tabs.H>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifndef WIN32
#include <sys/param.h>

#endif
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#ifdef __unix__
#include <sys/time.h>
#endif

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

/* Initial minimum height of a pane.  Just a fallback in case setPaneMinHeight
   (which may break in a future release) is not available */
#define PANE_MIN_HEIGHT 39

/* Thickness of 3D border around statistics and/or incremental search areas
   below the main menu bar */
#define STAT_SHADOW_THICKNESS 1

/* bitmap data for the close-tab button */
#define close_width 11
#define close_height 11
static unsigned char close_bits[] =
{
   0x00, 0x00, 0x00, 0x00, 0x8c, 0x01, 0xdc, 0x01, 0xf8, 0x00, 0x70, 0x00,
   0xf8, 0x00, 0xdc, 0x01, 0x8c, 0x01, 0x00, 0x00, 0x00, 0x00
};

/* bitmap data for the isearch-find button */
#define isrcFind_width 11
#define isrcFind_height 11
static unsigned char isrcFind_bits[] =
{
   0xe0, 0x01, 0x10, 0x02, 0xc8, 0x04, 0x08, 0x04, 0x08, 0x04, 0x00, 0x04,
   0x18, 0x02, 0xdc, 0x01, 0x0e, 0x00, 0x07, 0x00, 0x03, 0x00
};

/* bitmap data for the isearch-clear button */
#define isrcClear_width 11
#define isrcClear_height 11
static unsigned char isrcClear_bits[] =
{
   0x00, 0x00, 0x00, 0x00, 0x04, 0x01, 0x84, 0x01, 0xc4, 0x00, 0x64, 0x00,
   0xc4, 0x00, 0x84, 0x01, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00
};

// TODO: static void hideTooltip(Fl_Widget* tab);
// TODO: static Pixmap createBitmapWithDepth(Fl_Widget* w, char* data, unsigned int width,
// TODO:                                     unsigned int height);
static WindowInfo* getNextTabWindow(WindowInfo* window, int direction, int crossWin, int wrap);
// TODO: static Fl_Widget* addTab(Fl_Widget* folder, const char* string);
static int compareWindowNames(const void* windowA, const void* windowB);
// TODO: static int getTabPosition(Fl_Widget* tab);
// TODO: static Fl_Widget* manageToolBars(Fl_Widget* toolBarsForm);
// TODO: static void hideTearOffs(Fl_Widget* menuPane);
// TODO: static void CloseDocumentWindow(Fl_Widget* w, WindowInfo* window, XtPointer callData);
// TODO: static void closeTabCB(Fl_Widget* w, Fl_Widget* mainWin, caddr_t callData);
// TODO: static void raiseTabCB(Fl_Widget* w, XtPointer clientData, XtPointer callData);
static Ne_Text_Editor* createTextArea(WindowInfo* window, int rows, int cols, int emTabDist, char* delimiters, int wrapMargin, int lineNumCols);
static void showStats(WindowInfo* window, int state);
static void showISearch(WindowInfo* window, int state);
static void showStatsForm(WindowInfo* window);
static void addToWindowList(WindowInfo* window);
static void removeFromWindowList(WindowInfo* window);
// TODO: static void focusCB(Fl_Widget* w, WindowInfo* window, XtPointer callData);
static void AllWindowEventCB(Fl_Widget* w, void* data);
static void modifiedCB(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg);
static void movedCB(Fl_Widget* w, void* data);
// TODO: static void dragStartCB(Fl_Widget* w, WindowInfo* window, XtPointer callData);
// TODO: static void dragEndCB(Fl_Widget* w, WindowInfo* window, dragEndCBStruct* callData);
// TODO: static void closeCB(Fl_Widget* w, WindowInfo* window, XtPointer callData);
// TODO: static void saveYourselfCB(Fl_Widget* w, Fl_Widget* appShell, XtPointer callData);
// TODO: static void setPaneDesiredHeight(Fl_Widget* w, int height);
// TODO: static void setPaneMinHeight(Fl_Widget* w, int min);
// TODO: static void addWindowIcon(Fl_Widget* shell);
// TODO: static void wmSizeUpdateProc(XtPointer clientData, int* id);
// TODO: static void getGeometryString(WindowInfo* window, char* geomString);
// TODO: #ifdef ROWCOLPATCH
// TODO: static void patchRowCol(Fl_Widget* w);
// TODO: static void patchedRemoveChild(Fl_Widget* child);
// TODO: #endif
static void refreshMenuBar(WindowInfo* window);
// TODO: static void cloneDocument(WindowInfo* window, WindowInfo* orgWin);
// TODO: static void cloneTextPanes(WindowInfo* window, WindowInfo* orgWin);
// TODO: static UndoInfo* cloneUndoItems(UndoInfo* orgList);
// TODO: static Fl_Widget* containingPane(Fl_Widget* w);

static WindowInfo* inFocusDocument = NULL;  	/* where we are now */
static WindowInfo* lastFocusDocument = NULL;	    	/* where we came from */
static int DoneWithMoveDocumentDialog;
static int updateLineNumDisp(WindowInfo* window);
static int updateGutterWidth(WindowInfo* window);
static void deleteDocument(WindowInfo* window);
static void cancelTimeOut(int* timer);

// From Xt, Shell.c, "BIGSIZE"
static const int XT_IGNORE_PPOSITION = 32767;

/*
** Create a new editor window
*/
WindowInfo* CreateNeWindow(const char* name, char* geometry, int iconic)
{
// TODO:    Fl_Widget* winShell, *mainWin, *menuBar, *pane, *text, *stats, *statsAreaForm;
// TODO:    Fl_Widget* closeTabBtn, *tabForm, *form;
// TODO:    Fl_Color bgpix, fgpix;
// TODO:    Arg al[20];
// TODO:    int ac;
// TODO:    NeString s1;
// TODO:    XmFontList statsFontList;
// TODO:    WindowInfo* win;
// TODO:    char newGeometry[MAX_GEOM_STRING_LEN];
// TODO:    unsigned int rows, cols;
// TODO:    int x = 0, y = 0, bitmask, showTabBar, state;
// TODO: 
// TODO:    static Pixmap isrcFind = 0;
// TODO:    static Pixmap isrcClear = 0;
// TODO:    static Pixmap closeTabPixmap = 0;

   // Allocate some memory for the new window data structure
   WindowInfo* window = new WindowInfo();
   window->type = WindowInfo::Window;

   window->replaceDlog = NULL;
   window->replaceText = NULL;
   window->replaceWithText = NULL;
   window->replaceWordToggle = NULL;
   window->replaceCaseToggle = NULL;
   window->replaceRegexToggle = NULL;
   window->findDlog = NULL;
   window->findText = NULL;
   window->findWordToggle = NULL;
   window->findCaseToggle = NULL;
   window->findRegexToggle = NULL;
   window->replaceMultiFileDlog = NULL;
   window->replaceMultiFilePathBtn = NULL;
   window->replaceMultiFileList = NULL;
   window->multiFileReplSelected = false;
   window->multiFileBusy = false;
   window->writableWindows = NULL;
   window->nWritableWindows = 0;
   window->fileChanged = false;
   window->fileMode = 0;
   window->fileUid = 0;
   window->fileGid = 0;
   window->filenameSet = false;
   window->fileFormat = UNIX_FILE_FORMAT;
   window->lastModTime = 0;
   window->fileMissing = true;
   strcpy(window->filename, name);
   window->undo = NULL;
   window->redo = NULL;
   window->nPanes = 0;
   window->autoSaveCharCount = 0;
   window->autoSaveOpCount = 0;
   window->undoOpCount = 0;
   window->undoMemUsed = 0;
   CLEAR_ALL_LOCKS(window->lockReasons);
   window->indentStyle = GetPrefAutoIndent(PLAIN_LANGUAGE_MODE);
   window->autoSave = GetPrefAutoSave();
   window->saveOldVersion = GetPrefSaveOldVersion();
   window->wrapMode = GetPrefWrap(PLAIN_LANGUAGE_MODE);
   window->overstrike = false;
   window->showMatchingStyle = GetPrefShowMatching();
   window->matchSyntaxBased = GetPrefMatchSyntaxBased();
   window->showStats = GetPrefStatsLine();
   window->showISearchLine = GetPrefISearchLine();
   window->showLineNumbers = GetPrefLineNums();
   window->highlightSyntax = GetPrefHighlightSyntax();
   window->backlightCharTypes = NULL;
   window->backlightChars = GetPrefBacklightChars();
   if (window->backlightChars)
   {
      char* cTypes = GetPrefBacklightCharTypes();
      if (cTypes && window->backlightChars)
      {
         window->backlightCharTypes = new char[strlen(cTypes) + 1];
         strcpy(window->backlightCharTypes, cTypes);
      }
   }
   window->modeMessageDisplayed = false;
   window->modeMessage = NULL;
   window->ignoreModify = false;
   window->windowMenuValid = false;
   window->flashTimeoutID = 0;
   window->fileClosedAtom = 0; //None;
   window->wasSelected = false;

   strcpy(window->fontName, GetPrefFontName());
   strcpy(window->italicFontName, GetPrefItalicFontName());
   strcpy(window->boldFontName, GetPrefBoldFontName());
   strcpy(window->boldItalicFontName, GetPrefBoldItalicFontName());
   window->colorDialog = NULL;
   window->fontList = GetPrefFontList();
   window->italicFontStruct = GetPrefItalicFont();
   window->boldFontStruct = GetPrefBoldFont();
   window->boldItalicFontStruct = GetPrefBoldItalicFont();
   window->fontDialog = NULL;
   window->nMarks = 0;
   window->markTimeoutID = 0;
   window->highlightData = NULL;
   window->shellCmdData = NULL;
   window->macroCmdData = NULL;
   window->smartIndentData = NULL;
   window->languageMode = PLAIN_LANGUAGE_MODE;
   window->iSearchHistIndex = 0;
   window->iSearchStartPos = -1;
   window->replaceLastRegexCase   = true;
   window->replaceLastLiteralCase = false;
   window->iSearchLastRegexCase   = true;
   window->iSearchLastLiteralCase = false;
   window->findLastRegexCase      = true;
   window->findLastLiteralCase    = false;
   window->tab = NULL;
   window->device = 0;
   window->inode = 0;

   window->bgMenuUndoItem = 0;
   window->bgMenuRedoItem = 0;

   int rows = GetPrefRows();
   int cols = GetPrefCols();

// TODO:    /* If window geometry was specified, split it apart into a window position
// TODO:       component and a window size component.  Create a new geometry string
// TODO:       containing the position component only.  Rows and cols are stripped off
// TODO:       because we can't easily calculate the size in pixels from them until the
// TODO:       whole window is put together.  Note that the preference resource is only
// TODO:       for clueless users who decide to specify the standard X geometry
// TODO:       application resource, which is pretty useless because width and height
// TODO:       are the same as the rows and cols preferences, and specifying a window
// TODO:       location will force all the windows to pile on top of one another */
// TODO:    if (geometry == NULL || geometry[0] == '\0')
// TODO:       geometry = GetPrefGeometry();
// TODO:    if (geometry == NULL || geometry[0] == '\0')
// TODO:    {
// TODO:       rows = GetPrefRows();
// TODO:       cols = GetPrefCols();
// TODO:       newGeometry[0] = '\0';
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       bitmask = XParseGeometry(geometry, &x, &y, &cols, &rows);
// TODO:       if (bitmask == 0)
// TODO:          fprintf(stderr, "Bad window geometry specified: %s\n", geometry);
// TODO:       else
// TODO:       {
// TODO:          if (!(bitmask & WidthValue))
// TODO:             cols = GetPrefCols();
// TODO:          if (!(bitmask & HeightValue))
// TODO:             rows = GetPrefRows();
// TODO:       }
// TODO:       CreateGeometryString(newGeometry, x, y, 0, 0, bitmask & ~(WidthValue | HeightValue));
// TODO:    }
// TODO: 
// TODO:    /* Create a new toplevel shell to hold the window */
// TODO:    ac = 0;
// TODO:    XtSetArg(al[ac], XmNdeleteResponse, XmDO_NOTHING);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNgeometry, newGeometry[0]=='\0'?NULL:newGeometry);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNinitialState, iconic ? IconicState : NormalState);
// TODO:    ac++;
// TODO: 
// TODO:    if (newGeometry[0] == '\0')
// TODO:    {
// TODO:       /* Workaround to make Xt ignore Motif's bad PPosition size changes. Even
// TODO:          though we try to remove the PPosition in RealizeWithoutForcingPosition,
// TODO:          it is not sufficient.  Motif will recompute the size hints some point
// TODO:          later and put PPosition back! If the window is mapped after that time,
// TODO:          then the window will again wind up at 0, 0.  So, XEmacs does this, and
// TODO:          now we do.
// TODO: 
// TODO:          Alternate approach, relying on ShellP.h:
// TODO: 
// TODO:          ((WMShellWidget)winShell)->mainWindow.client_specified &= ~_XtShellPPositionOK;
// TODO:        */
// TODO: 
// TODO:       XtSetArg(al[ac], XtNx, XT_IGNORE_PPOSITION);
// TODO:       ac++;
// TODO:       XtSetArg(al[ac], XtNy, XT_IGNORE_PPOSITION);
// TODO:       ac++;
// TODO:    }
// TODO: 

   // Create a MainWindow to manage the menubar and text area, set the
   // userData resource to be used by WidgetToWindow to recover the
   // window pointer from the widget id of any of the window's widgets
   window->mainWindow = new Fl_Double_Window(50, 50, 700, 600, name);

   // Set icon for window (MacOS uses app bundle for icon...)
#ifdef WIN32
   window->mainWindow->icon((char *)LoadIcon(fl_display, MAKEINTRESOURCE(IDI_ICON)));
#elif !defined(__APPLE__)
//    fl_open_display();
//    window->mainWindow->icon((char *)XCreateBitmapFromData(fl_display, DefaultRootWindow(fl_display),
//       (char *)sudoku_bits, sudoku_width,
//       sudoku_height));
#endif

   // Create the menu bar
   window->menuBar = CreateMenuBar(window->mainWindow, window);

   // Create the tabs
   window->tab = new Fl_Tabs(0, 25, window->mainWindow->w(), window->mainWindow->h() - 25);
   window->tab->end();
   window->tab->selection_color(fl_color_average(FL_BACKGROUND_COLOR, FL_WHITE, 0.2));

   // The statsAreaForm holds the I-Search line.
// TODO:    window->iSearchForm = new Fl_Window(0, 25, window->mainWindow->w(), 24);
// TODO:    window->iSearchForm->box(FL_FLAT_BOX);
// TODO: 
// TODO:    window->iSearchFindButton = new Fl_Button(2,4,16,16, "@>");
// TODO: 
// TODO:    window->iSearchCaseToggle = new Fl_Check_Button( window->mainWindow->w() - 60, 4, 55, 16, "Case");
// TODO:    window->iSearchCaseToggle->value( GetPrefSearch() == SEARCH_CASE_SENSE 
// TODO:                                     || GetPrefSearch() == SEARCH_REGEX
// TODO:                                     || GetPrefSearch() == SEARCH_CASE_SENSE_WORD );
// TODO: 
// TODO:    window->iSearchRegexToggle = new Fl_Check_Button(window->iSearchCaseToggle->x() - 70, 4, 70, 16, "RegExp");
// TODO:    window->iSearchRegexToggle->value( GetPrefSearch() == SEARCH_REGEX_NOCASE
// TODO:                                 || GetPrefSearch() == SEARCH_REGEX);
// TODO: 
// TODO:    window->iSearchRevToggle = new Fl_Check_Button(window->iSearchRegexToggle->x()-50, 4, 45, 16, "Rev");
// TODO: 
// TODO:    window->iSearchClearButton = new Fl_Button(window->iSearchRevToggle->x()-20, 4, 16, 16, "@|<") ;
// TODO: 
// TODO:    window->iSearchText = new Fl_Input(20, 1, 400, 22);
// TODO:    window->iSearchForm->focus(window->iSearchText);
// TODO: 
// TODO: // TODO:    SetISearchTextCallbacks(window);
// TODO: 
// TODO: // TODO:    if (window->showISearchLine)
// TODO: // TODO:       XtManageChild(window->iSearchForm);
// TODO:    window->iSearchForm->show();
// TODO:    window->iSearchForm->end();

// TODO:    /* create the a form to house the tab bar and close-tab button */
// TODO:    tabForm = XtVaCreateWidget("tabForm",
// TODO:                               xmFormWidgetClass, statsAreaForm,
// TODO:                               XmNmarginHeight, 0,
// TODO:                               XmNmarginWidth, 0,
// TODO:                               XmNspacing, 0,
// TODO:                               XmNresizable, false,
// TODO:                               XmNleftAttachment, XmATTACH_FORM,
// TODO:                               XmNrightAttachment, XmATTACH_FORM,
// TODO:                               XmNshadowThickness, 0, NULL);
// TODO: 
// TODO:    /* button to close top document */
// TODO:    if (closeTabPixmap == 0)
// TODO:    {
// TODO:       closeTabPixmap = createBitmapWithDepth(tabForm,
// TODO:                                              (char*)close_bits, close_width, close_height);
// TODO:    }
// TODO:    closeTabBtn = XtVaCreateManagedWidget("closeTabBtn",
// TODO:                                          xmPushButtonWidgetClass, tabForm,
// TODO:                                          XmNmarginHeight, 0,
// TODO:                                          XmNmarginWidth, 0,
// TODO:                                          XmNhighlightThickness, 0,
// TODO:                                          XmNlabelType, XmPIXMAP,
// TODO:                                          XmNlabelPixmap, closeTabPixmap,
// TODO:                                          XmNshadowThickness, 1,
// TODO:                                          XmNtraversalOn, false,
// TODO:                                          XmNrightAttachment, XmATTACH_FORM,
// TODO:                                          XmNrightOffset, 3,
// TODO:                                          XmNbottomAttachment, XmATTACH_FORM,
// TODO:                                          XmNbottomOffset, 3,
// TODO:                                          NULL);
// TODO:    XtAddCallback(closeTabBtn, XmNactivateCallback, (XtCallbackProc)closeTabCB, mainWin);
// TODO: 
   // create the tab bar
   //window->tabBar = new Fl_Tabs(0, 50, window->mainWindow->w(),  window->mainWindow->h() - 50);
   //window->tabBar->color(FL_RED);
   //window->tabBar->end();

// TODO:    window->tabBar = XtVaCreateManagedWidget("tabBar",
// TODO:                     xmlFolderWidgetClass, tabForm,
// TODO:                     XmNresizePolicy, XmRESIZE_PACK,
// TODO:                     XmNleftAttachment, XmATTACH_FORM,
// TODO:                     XmNleftOffset, 0,
// TODO:                     XmNrightAttachment, XmATTACH_WIDGET,
// TODO:                     XmNrightWidget, closeTabBtn,
// TODO:                     XmNrightOffset, 5,
// TODO:                     XmNbottomAttachment, XmATTACH_FORM,
// TODO:                     XmNbottomOffset, 0,
// TODO:                     XmNtopAttachment, XmATTACH_FORM,
// TODO:                     NULL);
// TODO: 
// TODO:    window->tabMenuPane = CreateTabContextMenu(window->tabBar, window);
// TODO:    AddTabContextMenuAction(window->tabBar);
// TODO: 
// TODO:    /* create an unmanaged composite widget to get the folder
// TODO:       widget to hide the 3D shadow for the manager area.
// TODO:       Note: this works only on the patched XmLFolder widget */
// TODO:    form = XtVaCreateWidget("form",
// TODO:                            xmFormWidgetClass, window->tabBar,
// TODO:                            XmNheight, 1,
// TODO:                            XmNresizable, false,
// TODO:                            NULL);
// TODO: 
// TODO:    XtAddCallback(window->tabBar, XmNactivateCallback, raiseTabCB, NULL);

// TODO:    window->tab = addTab(window->tabBar, name);
// TODO: 
   // A form to hold the stats line text and line/col widgets
   window->statsLineForm = 0;
// TODO:    window->statsLineForm = new Ne_StatsLine(0,49,window->mainWindow->w(), 20);
// TODO:    //window->statsLineForm->box(FL_THIN_DOWN_FRAME);
// TODO: 
// TODO:    // Manage the statsLineForm
// TODO:    if (window->showStats)
// TODO:       window->statsLineForm->show();
// TODO:    else
// TODO:       window->statsLineForm->hide();
// TODO: 
// TODO:    window->statsLineForm->end(); // after show/hide else menu bar is hidden

// TODO:    /* Create paned window to manage split pane behavior */
// TODO:    pane = XtVaCreateManagedWidget("pane", xmPanedWindowWidgetClass,  mainWin,
// TODO:                                   XmNseparatorOn, false,
// TODO:                                   XmNspacing, 3, XmNsashIndent, -2, NULL);
// TODO:    window->splitPane = pane;
// TODO:    XmMainWindowSetAreas(mainWin, menuBar, statsAreaForm, NULL, NULL, pane);
// TODO: 
// TODO:    /* Store a copy of document/window pointer in text pane to support
// TODO:       action procedures. See also WidgetToWindow() for info. */
// TODO:    XtVaSetValues(pane, XmNuserData, window, NULL);
// TODO: 

   // Create the first, and most permanent text area (other panes may
   //  be added & removed, but this one will never be removed */
   window->textArea = createTextArea(window, rows,cols,
                         GetPrefEmTabDist(PLAIN_LANGUAGE_MODE), GetPrefDelimiters(),
                         GetPrefWrapMargin(), window->showLineNumbers?MIN_LINE_NUM_COLS:0);
   window->lastFocus = window->textArea;
   window->textArea->label(window->filename);
   window->tab->add(window->textArea);
   window->tab->resizable(window->textArea);

   // Set the initial colors from the globals.
   SetColors(window,
             GetPrefColorName(TEXT_FG_COLOR),
             GetPrefColorName(TEXT_BG_COLOR),
             GetPrefColorName(SELECT_FG_COLOR),
             GetPrefColorName(SELECT_BG_COLOR),
             GetPrefColorName(HILITE_FG_COLOR),
             GetPrefColorName(HILITE_BG_COLOR),
             GetPrefColorName(LINENO_FG_COLOR),
             GetPrefColorName(CURSOR_FG_COLOR));

// TODO:    /* Create the right button popup menu (note: order is important here,
// TODO:       since the translation for popping up this menu was probably already
// TODO:       added in createTextArea, but CreateBGMenu requires window->textArea
// TODO:       to be set so it can attach the menu to it (because menu shells are
// TODO:       finicky about the kinds of widgets they are attached to)) */
// TODO:    window->bgMenuPane = CreateBGMenu(window);

   /* cache user menus: init. user background menu cache */
   InitUserBGMenuCache(&window->userBGMenuCache);

   // Create the text buffer rather than using the one created automatically
   //   with the text area widget.  This is done so the syntax highlighting
   //   modify callback can be called to synchronize the style buffer BEFORE
   //   the text display's callback is called upon to display a modification
   window->buffer = BufCreate();
   BufAddModifyCB(window->buffer, SyntaxHighlightModifyCB, window);

   /* Attach the buffer to the text widget, and add callbacks for modify */
   TextSetBuffer(window->textArea, window->buffer);
   BufAddModifyCB(window->buffer, modifiedCB, window);

   // Set the requested hardware tab distance and useTabs in the text buffer
   BufSetTabDistance(window->buffer, GetPrefTabDist(PLAIN_LANGUAGE_MODE));
   window->buffer->useTabs = GetPrefInsertTabs();

   // add the window to the global window list, update the Windows menus
   addToWindowList(window);
   InvalidateWindowMenus();
   InvalidatePrevOpenMenus();

// TODO:    showTabBar = GetShowTabBar(window);
// TODO:    if (showTabBar)
// TODO:       XtManageChild(tabForm);
// TODO: 
// TODO:    manageToolBars(statsAreaForm);
// TODO: 
// TODO:    if (showTabBar || window->showISearchLine || window->showStats)
// TODO:       XtManageChild(statsAreaForm);
// TODO: 

   // realize all of the widgets in the new window
   window->mainWindow->focus(window->textArea);

// TODO:    /* Make close command in window menu gracefully prompt for close */
// TODO:    AddMotifCloseCallback(winShell, (XtCallbackProc)closeCB, window);

   /* Make window resizing work in nice character heights */
   UpdateWMSizeHints(window);

// TODO:    /* Set the minimum pane height for the initial text pane */
// TODO:    UpdateMinPaneHeights(window);
// TODO: 
// TODO:    /* create dialogs shared by all documents in a window */
// TODO:    CreateFindDlog(window->mainWindow, window);
// TODO:    CreateReplaceDlog(window->mainWindow, window);
// TODO:    CreateReplaceMultiFileDlog(window);
// TODO: 
// TODO:    /* dim/undim Attach_Tab menu items */
// TODO:    state = NDocuments(window) < NWindows();
// TODO:    for (win=WindowList; win; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:       {
// TODO:          NeSetSensitive(win->moveDocumentItem, state);
// TODO:          NeSetSensitive(win->contextMoveDocumentItem, state);
// TODO:       }
// TODO:    }

   //window->mainWindow->resizable(window->tabBar);
   window->mainWindow->resizable(window->tab);
   return window;
}

// TODO: /*
// TODO: ** ButtonPress event handler for tabs.
// TODO: */
// TODO: static void tabClickEH(Fl_Widget* w, XtPointer clientData, XEvent* event)
// TODO: {
// TODO:    /* hide the tooltip when user clicks with any button. */
// TODO:    if (BubbleButton_Timer(w))
// TODO:    {
// TODO:       XtRemoveTimeOut(BubbleButton_Timer(w));
// TODO:       BubbleButton_Timer(w) = (int)NULL;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       hideTooltip(w);
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** add a tab to the tab bar for the new document.
// TODO: */
// TODO: static Fl_Widget* addTab(Fl_Widget* folder, const char* string)
// TODO: {
// TODO:    Fl_Widget* tooltipLabel, tab;
// TODO:    NeString s1;
// TODO: 
// TODO:    s1 = NeNewString((char*)string);
// TODO:    tab = XtVaCreateManagedWidget("tab",
// TODO:                                  xrwsBubbleButtonWidgetClass, folder,
// TODO:                                  /* XmNmarginWidth, <default@nedit.c>, */
// TODO:                                  /* XmNmarginHeight, <default@nedit.c>, */
// TODO:                                  /* XmNalignment, <default@nedit.c>, */
// TODO:                                  XmNlabelString, s1,
// TODO:                                  XltNbubbleString, s1,
// TODO:                                  XltNshowBubble, GetPrefToolTips(),
// TODO:                                  XltNautoParkBubble, true,
// TODO:                                  XltNslidingBubble, false,
// TODO:                                  /* XltNdelay, 800,*/
// TODO:                                  /* XltNbubbleDuration, 8000,*/
// TODO:                                  NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    /* there's things to do as user click on the tab */
// TODO:    XtAddEventHandler(tab, ButtonPressMask, false,
// TODO:                      (XtEventHandler)tabClickEH, (XtPointer)0);
// TODO: 
// TODO:    /* BubbleButton simply use reversed video for tooltips,
// TODO:       we try to use the 'standard' color */
// TODO:    tooltipLabel = XtNameToWidget(tab, "*BubbleLabel");
// TODO:    XtVaSetValues(tooltipLabel,
// TODO:                  XmNbackground, AllocateColor(tab, GetPrefTooltipBgColor()),
// TODO:                  XmNforeground, AllocateColor(tab, NEDIT_DEFAULT_FG),
// TODO:                  NULL);
// TODO: 
// TODO:    /* put borders around tooltip. BubbleButton use
// TODO:       transientShellWidgetClass as tooltip shell, which
// TODO:       came without borders */
// TODO:    XtVaSetValues(XtParent(tooltipLabel), XmNborderWidth, 1, NULL);
// TODO: 
// TODO: #ifdef LESSTIF_VERSION
// TODO:    /* If we don't do this, no popup when right-click on tabs */
// TODO:    AddTabContextMenuAction(tab);
// TODO: #endif /* LESSTIF_VERSION */
// TODO: 
// TODO:    return tab;
// TODO: }

/*
** Comparison function for sorting windows by title.
** Windows are sorted by alphabetically by filename and then
** alphabetically by path.
*/
static int compareWindowNames(const void* windowA, const void* windowB)
{
   int rc;
   const WindowInfo* a = *((WindowInfo**)windowA);
   const WindowInfo* b = *((WindowInfo**)windowB);

   rc = strcmp(a->filename, b->filename);
   if (rc != 0)
      return rc;
   rc = strcmp(a->path, b->path);
   return rc;
}

/*
** Sort tabs in the tab bar alphabetically, if demanded so.
*/
void SortTabBar(WindowInfo* window)
{
   WindowInfo* w;
   WindowInfo** windows;
//    WidgetList tabList;
   int i, j, nDoc, tabCount;

   if (!GetPrefSortTabs())
      return;

   /* need more than one tab to sort */
   nDoc = NDocuments(window);
   if (nDoc < 2)
      return;

   /* first sort the documents */
   windows = (WindowInfo**)malloc__(sizeof(WindowInfo*) * nDoc);
   for (w=WindowList, i=0; w!=NULL; w=w->next)
   {
      if (window->mainWindow == w->mainWindow)
         windows[i++] = w;
   }
   qsort(windows, nDoc, sizeof(WindowInfo*), compareWindowNames);

// TODO:    /* assign tabs to documents in sorted order */
// TODO:    XtVaGetValues(window->tabBar, XmNtabWidgetList, &tabList,
// TODO:                  XmNtabCount, &tabCount, NULL);
// TODO: 
// TODO:    for (i=0, j=0; i<tabCount && j<nDoc; i++)
// TODO:    {
// TODO:       if (tabList[i]->core.being_destroyed)
// TODO:          continue;
// TODO: 
// TODO:       /* set tab as active */
// TODO:       if (IsTopDocument(windows[j]))
// TODO:          XmLFolderSetActiveTab(window->tabBar, i, false);
// TODO: 
// TODO:       windows[j]->tab = tabList[i];
// TODO:       RefreshTabState(windows[j]);
// TODO:       j++;
// TODO:    }
// TODO: 
   free__((char*)windows);
}

/*
** find which document a tab belongs to
*/
WindowInfo* TabToWindow(Fl_Widget* tab)
{
   WindowInfo* win;
   for (win=WindowList; win; win=win->next)
   {
      if (win->textArea == tab)
         return win;
   }

   return NULL;
}

/*
** Close a document, or an editor window
*/
void CloseWindow(WindowInfo* window)
{
   int keepWindow, state;
   char name[MAXPATHLEN];
   WindowInfo* win, *topBuf = NULL, *nextBuf = NULL;

   /* Free smart indent macro programs */
   EndSmartIndent(window);

   /* Clean up macro references to the doomed window.  If a macro is
      executing, stop it.  If macro is calling this (closing its own
      window), leave the window alive until the macro completes */
   keepWindow = !MacroWindowCloseActions(window);

#ifndef VMS
   /* Kill shell sub-process and free__ related memory */
   AbortShellCommand(window);
#endif /*VMS*/

   /* Unload the default tips files for this language mode if necessary */
   UnloadLanguageModeTipsFile(window);

   /* If a window is closed while it is on the multi-file replace dialog
      list of any other window (or even the same one), we must update those
      lists or we end up with dangling references. Normally, there can
      be only one of those dialogs at the same time (application modal),
      but LessTif doesn't even (always) honor application modalness, so
      there can be more than one dialog. */
   RemoveFromMultiReplaceDialog(window);

   /* Destroy the file closed property for this file */
   DeleteFileClosedProperty(window);

   /* Remove any possibly pending callback which might fire after the
      widget is gone. */
   cancelTimeOut(&window->flashTimeoutID);
   cancelTimeOut(&window->markTimeoutID);

   /* if this is the last window, or must be kept alive temporarily because
      it's running the macro calling us, don't close it, make it Untitled */
   if (keepWindow || (WindowList == window && window->next == NULL))
   {
      window->filename[0] = '\0';
      UniqueUntitledName(name);
      CLEAR_ALL_LOCKS(window->lockReasons);
      window->fileMode = 0;
      window->fileUid = 0;
      window->fileGid = 0;
      strcpy(window->filename, name);
      strcpy(window->path, "");
      window->ignoreModify = true;
      BufSetAll(window->buffer, "");
      window->ignoreModify = false;
      window->nMarks = 0;
      window->filenameSet = false;
      window->fileMissing = true;
      window->fileChanged = false;
      window->fileFormat = UNIX_FILE_FORMAT;
      window->lastModTime = 0;
      window->device = 0;
      window->inode = 0;

      StopHighlighting(window);
      EndSmartIndent(window);
      UpdateWindowTitle(window);
      UpdateWindowReadOnly(window);
// TODO:       NeSetSensitive(window->closeItem, false);
// TODO:       NeSetSensitive(window->readOnlyItem, true);
// TODO:       NeToggleButtonSetState(window->readOnlyItem, false, false);
      ClearUndoList(window);
      ClearRedoList(window);
      if (window->statsLineForm)
         window->statsLineForm->statsLine().copy_label(""); /* resets scroll pos of stats line from long file names */
      UpdateStatsLine(window);
      DetermineLanguageMode(window, true);
      RefreshTabState(window);
      updateLineNumDisp(window);
      return;
   }

   /* Free syntax highlighting patterns, if any. w/o redisplaying */
   FreeHighlightingData(window);

   /* remove the buffer modification callbacks so the buffer will be
      deallocated when the last text widget is destroyed */
   BufRemoveModifyCB(window->buffer, modifiedCB, window);
   BufRemoveModifyCB(window->buffer, SyntaxHighlightModifyCB, window);

#ifdef ROWCOLPATCH
   patchRowCol(window->menuBar);
#endif

   /* free__ the undo and redo lists */
   ClearUndoList(window);
   ClearRedoList(window);

   /* close the document/window */
   if (NDocuments(window) > 1)
   {
      if (MacroRunWindow() && MacroRunWindow() != window
            && MacroRunWindow()->mainWindow == window->mainWindow)
      {
         nextBuf = MacroRunWindow();
         RaiseDocument(nextBuf);
      }
      else if (IsTopDocument(window))
      {
         /* need to find a successor before closing a top document */
         nextBuf = getNextTabWindow(window, 1, 0, 0);
         RaiseDocument(nextBuf);
      }
      else
      {
         topBuf = GetTopDocument(window->mainWindow);
      }
   }

   /* remove the window from the global window list, update window menus */
   removeFromWindowList(window);
   InvalidateWindowMenus();
   CheckCloseDim(); /* Close of window running a macro may have been disabled. */

   /* remove the tab of the closing document from tab bar */
// TODO:    XtDestroyWidget(window->tab);
   window->tab->remove(window->textArea);

   /* refresh tab bar after closing a document */
   if (nextBuf)
   {
      ShowWindowTabBar(nextBuf);
      updateLineNumDisp(nextBuf);
   }
   else if (topBuf)
   {
      ShowWindowTabBar(topBuf);
      updateLineNumDisp(topBuf);
   }

   /* dim/undim Detach_Tab menu items */
   win = nextBuf? nextBuf : topBuf;
   if (win)
   {
      state = NDocuments(win) > 1;
// TODO:       NeSetSensitive(win->detachDocumentItem, state);
// TODO:       NeSetSensitive(win->contextDetachDocumentItem, state);
   }

   /* dim/undim Attach_Tab menu items */
   state = NDocuments(WindowList) < NWindows();
   for (win=WindowList; win; win=win->next)
   {
      if (IsTopDocument(win))
      {
// TODO:          NeSetSensitive(win->moveDocumentItem, state);
// TODO:          NeSetSensitive(win->contextMoveDocumentItem, state);
      }
   }

   /* free__ background menu cache for document */
   FreeUserBGMenuCache(&window->userBGMenuCache);

   /* destroy the document's pane, or the window */
   if (nextBuf || topBuf)
   {
      deleteDocument(window);
   }
   else
   {
      /* free__ user menu cache for window */
      FreeUserMenuCache(window->userMenuCache);

      /* remove and deallocate all of the widgets associated with window */
      free__(window->backlightCharTypes); /* we made a copy earlier on */
      CloseAllPopupsFor(window->mainWindow);
// TODO:       XtDestroyWidget(window->mainWindow);
   }

   /* deallocate the window data structure */
   free__((char*)window);
}

/*
** check if tab bar is to be shown on this window
*/
int GetShowTabBar(WindowInfo* window)
{
   if (!GetPrefTabBar())
      return false;
   else if (NDocuments(window) == 1)
      return !GetPrefTabBarHideOne();
   else
      return true;
}

void ShowWindowTabBar(WindowInfo* window)
{
   if (GetPrefTabBar())
   {
      if (GetPrefTabBarHideOne())
         ShowTabBar(window, NDocuments(window)>1);
      else
         ShowTabBar(window, true);
   }
   else
      ShowTabBar(window, false);
}

/*
** Check if there is already a window open for a given file
*/
WindowInfo* FindWindowWithFile(const char* name, const char* path)
{
   WindowInfo* window;

   /* I don't think this algorithm will work on vms so I am
      disabling it for now */
   if (!GetPrefHonorSymlinks())
   {
      char fullname[MAXPATHLEN + 1];
      struct stat attribute;

      strncpy(fullname, path, MAXPATHLEN);
      strncat(fullname, name, MAXPATHLEN);
      fullname[MAXPATHLEN] = '\0';

      if (0 == stat(fullname, &attribute))
      {
         for (window = WindowList; window != NULL; window = window->next)
         {
            if (attribute.st_dev == window->device
                  && attribute.st_ino == window->inode)
            {
               return window;
            }
         }
      }   /*  else:  Not an error condition, just a new file. Continue to check
                whether the filename is already in use for an unsaved document.  */
   }

   for (window = WindowList; window != NULL; window = window->next)
   {
      if (!strcmp(window->filename, name) && !strcmp(window->path, path))
      {
         return window;
      }
   }
   return NULL;
}

// TODO: /*
// TODO: ** Add another independently scrollable pane to the current document,
// TODO: ** splitting the pane which currently has keyboard focus.
// TODO: */
// TODO: void SplitPane(WindowInfo* window)
// TODO: {
// TODO:    short paneHeights[MAX_PANES+1];
// TODO:    int insertPositions[MAX_PANES+1], topLines[MAX_PANES+1];
// TODO:    int horizOffsets[MAX_PANES+1];
// TODO:    int i, focusPane, emTabDist, wrapMargin, lineNumCols, totalHeight=0;
// TODO:    char* delimiters;
// TODO:    Fl_Widget* text = NULL;
// TODO:    textDisp* textD, *newTextD;
// TODO: 
// TODO:    /* Don't create new panes if we're already at the limit */
// TODO:    if (window->nPanes >= MAX_PANES)
// TODO:       return;
// TODO: 
// TODO:    /* Record the current heights, scroll positions, and insert positions
// TODO:       of the existing panes, keyboard focus */
// TODO:    focusPane = 0;
// TODO:    for (i=0; i<=window->nPanes; i++)
// TODO:    {
// TODO:       text = i==0 ? window->textArea : window->textPanes[i-1];
// TODO:       insertPositions[i] = TextGetCursorPos(text);
// TODO:       XtVaGetValues(containingPane(text),XmNheight,&paneHeights[i],NULL);
// TODO:       totalHeight += paneHeights[i];
// TODO:       TextGetScroll(text, &topLines[i], &horizOffsets[i]);
// TODO:       if (text == window->lastFocus)
// TODO:          focusPane = i;
// TODO:    }
// TODO: 
// TODO:    /* Unmanage & remanage the panedWindow so it recalculates pane heights */
// TODO:    XtUnmanageChild(window->splitPane);
// TODO: 
// TODO:    /* Create a text widget to add to the pane and set its buffer and
// TODO:       highlight data to be the same as the other panes in the document */
// TODO:    XtVaGetValues(window->textArea, textNemulateTabs, &emTabDist,
// TODO:                  textNwordDelimiters, &delimiters, textNwrapMargin, &wrapMargin,
// TODO:                  textNlineNumCols, &lineNumCols, NULL);
// TODO:    text = createTextArea(window->splitPane, window, 1, 1, emTabDist,
// TODO:                          delimiters, wrapMargin, lineNumCols);
// TODO: 
// TODO:    TextSetBuffer(text, window->buffer);
// TODO:    if (window->highlightData != NULL)
// TODO:       AttachHighlightToWidget(text, window);
// TODO:    if (window->backlightChars)
// TODO:    {
// TODO:       XtVaSetValues(text, textNbacklightCharTypes,
// TODO:                     window->backlightCharTypes, NULL);
// TODO:    }
// TODO:    XtManageChild(text);
// TODO:    window->textPanes[window->nPanes++] = text;
// TODO: 
// TODO:    /* Fix up the colors */
// TODO:    textD = ((TextWidget)window->textArea)->text.textD;
// TODO:    newTextD = ((TextWidget)text)->text.textD;
// TODO:    XtVaSetValues(text,
// TODO:                  XmNforeground, textD->fgPixel,
// TODO:                  XmNbackground, textD->bgPixel,
// TODO:                  NULL);
// TODO:    TextDSetColors(newTextD, textD->fgPixel, textD->bgPixel,
// TODO:                   textD->selectFGPixel, textD->selectBGPixel, textD->highlightFGPixel,
// TODO:                   textD->highlightBGPixel, textD->lineNumFGPixel,
// TODO:                   textD->cursorFGPixel);
// TODO: 
// TODO:    /* Set the minimum pane height in the new pane */
// TODO:    UpdateMinPaneHeights(window);
// TODO: 
// TODO:    /* adjust the heights, scroll positions, etc., to split the focus pane */
// TODO:    for (i=window->nPanes; i>focusPane; i--)
// TODO:    {
// TODO:       insertPositions[i] = insertPositions[i-1];
// TODO:       paneHeights[i] = paneHeights[i-1];
// TODO:       topLines[i] = topLines[i-1];
// TODO:       horizOffsets[i] = horizOffsets[i-1];
// TODO:    }
// TODO:    paneHeights[focusPane] = paneHeights[focusPane]/2;
// TODO:    paneHeights[focusPane+1] = paneHeights[focusPane];
// TODO: 
// TODO:    for (i=0; i<=window->nPanes; i++)
// TODO:    {
// TODO:       text = i==0 ? window->textArea : window->textPanes[i-1];
// TODO:       setPaneDesiredHeight(containingPane(text), paneHeights[i]);
// TODO:    }
// TODO: 
// TODO:    /* Re-manage panedWindow to recalculate pane heights & reset selection */
// TODO:    if (IsTopDocument(window))
// TODO:       XtManageChild(window->splitPane);
// TODO: 
// TODO:    /* Reset all of the heights, scroll positions, etc. */
// TODO:    for (i=0; i<=window->nPanes; i++)
// TODO:    {
// TODO:       text = i==0 ? window->textArea : window->textPanes[i-1];
// TODO:       TextSetCursorPos(text, insertPositions[i]);
// TODO:       TextSetScroll(text, topLines[i], horizOffsets[i]);
// TODO:       setPaneDesiredHeight(containingPane(text),
// TODO:                            totalHeight/(window->nPanes+1));
// TODO:    }
// TODO:    XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);
// TODO: 
// TODO:    /* Update the window manager size hints after the sizes of the panes have
// TODO:       been set (the widget heights are not yet readable here, but they will
// TODO:       be by the time the event loop gets around to running this timer proc) */
// TODO:    XtAppAddTimeOut(XtWidgetToApplicationContext(window->mainWindow), 0,
// TODO:                    wmSizeUpdateProc, window);
// TODO: }

Ne_Text_Editor* GetPaneByIndex(WindowInfo* window, int paneIndex)
{
   Ne_Text_Editor* text = NULL;
   if (paneIndex >= 0 && paneIndex <= window->nPanes)
   {
      text = (paneIndex == 0) ? window->textArea : window->textPanes[paneIndex - 1];
   }
   return(text);
}

int WidgetToPaneIndex(WindowInfo* window, Fl_Widget* w)
{
   int paneIndex = 0;

// TODO:    for (int i = 0; i <= window->nPanes; ++i)
// TODO:    {
// TODO:       Fl_Widget* text = (i == 0) ? window->textArea : window->textPanes[i - 1];
// TODO:       if (text == w)
// TODO:       {
// TODO:          paneIndex = i;
// TODO:       }
// TODO:    }
   return(paneIndex);
}

// TODO: /*
// TODO: ** Close the window pane that last had the keyboard focus.  (Actually, close
// TODO: ** the bottom pane and make it look like pane which had focus was closed)
// TODO: */
// TODO: void ClosePane(WindowInfo* window)
// TODO: {
// TODO:    short paneHeights[MAX_PANES+1];
// TODO:    int insertPositions[MAX_PANES+1], topLines[MAX_PANES+1];
// TODO:    int horizOffsets[MAX_PANES+1];
// TODO:    int i, focusPane;
// TODO:    Fl_Widget* text;
// TODO: 
// TODO:    /* Don't delete the last pane */
// TODO:    if (window->nPanes <= 0)
// TODO:       return;
// TODO: 
// TODO:    /* Record the current heights, scroll positions, and insert positions
// TODO:       of the existing panes, and the keyboard focus */
// TODO:    focusPane = 0;
// TODO:    for (i=0; i<=window->nPanes; i++)
// TODO:    {
// TODO:       text = i==0 ? window->textArea : window->textPanes[i-1];
// TODO:       insertPositions[i] = TextGetCursorPos(text);
// TODO:       XtVaGetValues(containingPane(text),
// TODO:                     XmNheight, &paneHeights[i], NULL);
// TODO:       TextGetScroll(text, &topLines[i], &horizOffsets[i]);
// TODO:       if (text == window->lastFocus)
// TODO:          focusPane = i;
// TODO:    }
// TODO: 
// TODO:    /* Unmanage & remanage the panedWindow so it recalculates pane heights */
// TODO:    XtUnmanageChild(window->splitPane);
// TODO: 
// TODO:    /* Destroy last pane, and make sure lastFocus points to an existing pane.
// TODO:       Workaround for OM 2.1.30: text widget must be unmanaged for
// TODO:       xmPanedWindowWidget to calculate the correct pane heights for
// TODO:       the remaining panes, simply detroying it didn't seem enough */
// TODO:    window->nPanes--;
// TODO:    XtUnmanageChild(containingPane(window->textPanes[window->nPanes]));
// TODO:    XtDestroyWidget(containingPane(window->textPanes[window->nPanes]));
// TODO: 
// TODO:    if (window->nPanes == 0)
// TODO:       window->lastFocus = window->textArea;
// TODO:    else if (focusPane > window->nPanes)
// TODO:       window->lastFocus = window->textPanes[window->nPanes-1];
// TODO: 
// TODO:    /* adjust the heights, scroll positions, etc., to make it look
// TODO:       like the pane with the input focus was closed */
// TODO:    for (i=focusPane; i<=window->nPanes; i++)
// TODO:    {
// TODO:       insertPositions[i] = insertPositions[i+1];
// TODO:       paneHeights[i] = paneHeights[i+1];
// TODO:       topLines[i] = topLines[i+1];
// TODO:       horizOffsets[i] = horizOffsets[i+1];
// TODO:    }
// TODO: 
// TODO:    /* set the desired heights and re-manage the paned window so it will
// TODO:       recalculate pane heights */
// TODO:    for (i=0; i<=window->nPanes; i++)
// TODO:    {
// TODO:       text = i==0 ? window->textArea : window->textPanes[i-1];
// TODO:       setPaneDesiredHeight(containingPane(text), paneHeights[i]);
// TODO:    }
// TODO: 
// TODO:    if (IsTopDocument(window))
// TODO:       XtManageChild(window->splitPane);
// TODO: 
// TODO:    /* Reset all of the scroll positions, insert positions, etc. */
// TODO:    for (i=0; i<=window->nPanes; i++)
// TODO:    {
// TODO:       text = i==0 ? window->textArea : window->textPanes[i-1];
// TODO:       TextSetCursorPos(text, insertPositions[i]);
// TODO:       TextSetScroll(text, topLines[i], horizOffsets[i]);
// TODO:    }
// TODO:    XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);
// TODO: 
// TODO:    /* Update the window manager size hints after the sizes of the panes have
// TODO:       been set (the widget heights are not yet readable here, but they will
// TODO:       be by the time the event loop gets around to running this timer proc) */
// TODO:    XtAppAddTimeOut(XtWidgetToApplicationContext(window->mainWindow), 0,
// TODO:                    wmSizeUpdateProc, window);
// TODO: }

/*
** Turn on and off the display of line numbers
*/
void ShowLineNumbers(WindowInfo* window, bool state)
{
   int i, marginWidth;
   unsigned reqCols = 0;
   int windowWidth;
   WindowInfo* win;
   Ne_Text_Editor* textD = window->textArea;

   if (window->showLineNumbers == state)
      return;
   window->showLineNumbers = state;

   /* Just setting window->showLineNumbers is sufficient to tell
      updateLineNumDisp() to expand the line number areas and the window
      size for the number of lines required.  To hide the line number
      display, set the width to zero, and contract the window width. */
   if (state)
   {
      reqCols = updateLineNumDisp(window);
   }
   else
   {
      windowWidth = window->mainWindow->w();
      marginWidth = window->textArea->marginWidth;
      window->mainWindow->size( windowWidth - textD->left + marginWidth, window->mainWindow->h());

      for (i=0; i<=window->nPanes; i++)
      {
         Ne_Text_Editor* textD = i==0 ? window->textArea : window->textPanes[i-1];
         textD->lineNumCols = 0;
      }
   }

   /* line numbers panel is shell-level, hence other
      tabbed documents in the window should synch */
   for (win=WindowList; win; win=win->next)
   {
      if (win->mainWindow != window->mainWindow || win == window)
         continue;

      win->showLineNumbers = state;

      for (i=0; i<=win->nPanes; i++)
      {
         Ne_Text_Editor* text = i==0 ? win->textArea : win->textPanes[i-1];
         /*  reqCols should really be cast here, but into what? XmRInt?  */
         textD->lineNumCols = reqCols;
      }
   }

   /* Tell WM that the non-expandable part of the window has changed size */
   UpdateWMSizeHints(window);
}

void SetTabDist(WindowInfo* window, int tabDist)
{
   if (window->buffer->tabDist != tabDist)
   {
      int saveCursorPositions[MAX_PANES + 1];
      int saveVScrollPositions[MAX_PANES + 1];
      int saveHScrollPositions[MAX_PANES + 1];
      int paneIndex;

      window->ignoreModify = true;

      for (paneIndex = 0; paneIndex <= window->nPanes; ++paneIndex)
      {
         Ne_Text_Editor* textD = GetPaneByIndex(window, paneIndex);

         TextGetScroll(textD, &saveVScrollPositions[paneIndex], &saveHScrollPositions[paneIndex]);
         saveCursorPositions[paneIndex] = TextGetCursorPos(textD);
         textD->modifyingTabDist = 1;
      }

      BufSetTabDistance(window->buffer, tabDist);

      for (paneIndex = 0; paneIndex <= window->nPanes; ++paneIndex)
      {
         Ne_Text_Editor* textD = GetPaneByIndex(window, paneIndex);

         textD->modifyingTabDist = 0;
         TextSetCursorPos(textD, saveCursorPositions[paneIndex]);
         TextSetScroll(textD, saveVScrollPositions[paneIndex], saveHScrollPositions[paneIndex]);
      }

      window->ignoreModify = false;
   }
}

void SetEmTabDist(WindowInfo* window, int emTabDist)
{
   int i;

   window->textArea->text.emulateTabs = emTabDist;
   for (i = 0; i < window->nPanes; ++i)
   {
      window->textPanes[i]->text.emulateTabs = emTabDist;
   }
}

/*
** Turn on and off the display of the statistics line
*/
void ShowStatsLine(WindowInfo* window, int state)
{
   TRACE();

   // In continuous wrap mode, text widgets must be told to keep track of
   // the top line number in absolute (non-wrapped) lines, because it can
   // be a costly calculation, and is only needed for displaying line
   // numbers, either in the stats line, or along the left margin
   for (int i=0; i<=window->nPanes; i++)
   {
       Fl_Widget* text = i==0 ? window->textArea : window->textPanes[i-1];
// TODO:       TextDMaintainAbsLineNum(((TextWidget)text)->text.textD, state);
   }
   window->showStats = state;
   showStats(window, state);

   // i-search line is shell-level, hence other tabbed documents in the window should synch
   for (WindowInfo* win=WindowList; win; win=win->next)
   {
      if (win->mainWindow != window->mainWindow || win == window)
         continue;
      win->showStats = state;
   }
}

// Displays and undisplays the statistics line (regardless of settings of
// window->showStats or window->modeMessageDisplayed)
static void showStats(WindowInfo* window, int state)
{
   if (state)
   {
      window->statsLineForm->show();
      showStatsForm(window);
   }
   else
   {
      window->statsLineForm->hide();
      showStatsForm(window);
   }
}

/*
*/
static void showTabBar(WindowInfo* window, int state)
{
   if (state)
   {
      window->tabBar->show();
      window->tab->add(window->textArea);
      showStatsForm(window);
   }
   else
   {
      window->tabBar->hide();
      window->tab->remove(window->textArea);
      showStatsForm(window);
   }
}

/*
*/
void ShowTabBar(WindowInfo* window, int state)
{
// TODO:    if (window->tabBar->visible() && state || !window->tabBar->visible() && !state)
// TODO:       return;
// TODO:    showTabBar(window, state);
// TODO:    window->mainWindow->redraw();
}

/*
** Turn on and off the continuing display of the incremental search line
** (when off, it is popped up and down as needed via TempShowISearch)
*/
void ShowISearchLine(WindowInfo* window, int state)
{
   WindowInfo* win;

   if (window->showISearchLine == state)
      return;
   window->showISearchLine = state;
   showISearch(window, state);

   /* i-search line is shell-level, hence other tabbed documents in the window should synch */
   for (win=WindowList; win; win=win->next)
   {
      if (win->mainWindow != window->mainWindow || win == window)
         continue;
      win->showISearchLine = state;
   }
}

/*
** Temporarily show and hide the incremental search line if the line is not
** already up.
*/
void TempShowISearch(WindowInfo* window, int state)
{
   if (window->showISearchLine)
      return;
   if (window->iSearchForm->visible() != state)
      showISearch(window, state);
}

/*
** Put up or pop-down the incremental search line regardless of settings
** of showISearchLine or TempShowISearch
*/
static void showISearch(WindowInfo* window, int state)
{
   if (state)
   {
      window->iSearchForm->show();
      showStatsForm(window);
   }
   else
   {
      window->iSearchForm->hide();
      showStatsForm(window);
   }

   /* Tell WM that the non-expandable part of the window has changed size */
   /* This is already done in showStatsForm */
   /* UpdateWMSizeHints(window); */
}

/*
** Show or hide the extra display area under the main menu bar which
** optionally contains the status line and the incremental search bar
*/
static void showStatsForm(WindowInfo* window)
{
   Fl_Widget* statsAreaForm = window->statsLineForm;
   Fl_Widget* mainW = window->mainWindow;

// TODO:    /* The very silly use of XmNcommandWindowLocation and XmNshowSeparator
// TODO:       below are to kick the main window widget to position and remove the
// TODO:       status line when it is managed and unmanaged.  At some Motif version
// TODO:       level, the showSeparator trick backfires and leaves the separator
// TODO:       shown, but fortunately the dynamic behavior is fixed, too so the
// TODO:       workaround is no longer necessary, either.  (... the version where
// TODO:       this occurs may be earlier than 2.1.  If the stats line shows
// TODO:       double thickness shadows in earlier Motif versions, the #if XmVersion
// TODO:       directive should be moved back to that earlier version) */
// TODO:    if (manageToolBars(statsAreaForm))
// TODO:    {
// TODO:       XtUnmanageChild(statsAreaForm);    /*... will this fix Solaris 7??? */
// TODO:       XtVaSetValues(mainW, XmNcommandWindowLocation, XmCOMMAND_ABOVE_WORKSPACE, NULL);
// TODO:       XtManageChild(statsAreaForm);
// TODO:       XtVaSetValues(mainW, XmNshowSeparator, false, NULL);
// TODO:       UpdateStatsLine(window);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       XtUnmanageChild(statsAreaForm);
// TODO:       XtVaSetValues(mainW, XmNcommandWindowLocation, XmCOMMAND_BELOW_WORKSPACE, NULL);
// TODO:    }
// TODO: 
// TODO:    /* Tell WM that the non-expandable part of the window has changed size */
// TODO:    UpdateWMSizeHints(window);
}

// Display a special message in the stats line (show the stats line if it
// is not currently shown).
void SetModeMessage(WindowInfo* window, const char* message)
{
   // this document may be hidden (not on top) or later made hidden,
   // so we save a copy of the mode message, so we can restore the
   // statsline when the document is raised to top again */
   window->modeMessageDisplayed = true;
   delete[] window->modeMessage;
   window->modeMessage = NeNewString(message);

   if (!IsTopDocument(window))
      return;

  window->statsLineForm->statsLine().copy_label(message);
   // Don't invoke the stats line again, if stats line is already displayed.
   if (!window->showStats)
      showStats(window, true);
}

// TODO: /*
// TODO: ** Clear special statistics line message set in SetModeMessage, returns
// TODO: ** the statistics line to its original state as set in window->showStats
// TODO: */
// TODO: void ClearModeMessage(WindowInfo* window)
// TODO: {
// TODO:    if (!window->modeMessageDisplayed)
// TODO:       return;
// TODO: 
// TODO:    window->modeMessageDisplayed = false;
// TODO:    free__(window->modeMessage);
// TODO:    window->modeMessage = NULL;
// TODO: 
// TODO:    if (!IsTopDocument(window))
// TODO:       return;
// TODO: 
// TODO:    /*
// TODO:     * Remove the stats line only if indicated by it's window state.
// TODO:     */
// TODO:    if (!window->showStats)
// TODO:       showStats(window, false);
// TODO:    UpdateStatsLine(window);
// TODO: }
 
/*
** Count the windows
*/
int NWindows()
{
   WindowInfo* win;
   int n;

   for (win=WindowList, n=0; win!=NULL; win=win->next, n++);
   return n;
}

/*
** Set autoindent state to one of  NO_AUTO_INDENT, AUTO_INDENT, or SMART_INDENT.
*/
void SetAutoIndent(WindowInfo* window, int state)
{
   int autoIndent = state == AUTO_INDENT, smartIndent = state == SMART_INDENT;
   int i;

   if (window->indentStyle == SMART_INDENT && !smartIndent)
      EndSmartIndent(window);
   else if (smartIndent && window->indentStyle != SMART_INDENT)
      BeginSmartIndent(window, true);
   window->indentStyle = state;
   window->textArea->text.autoIndent = autoIndent;
   window->textArea->text.smartIndent = smartIndent;
   for (i=0; i<window->nPanes; i++)
   {
      window->textPanes[i]->text.autoIndent = autoIndent;
      window->textPanes[i]->text.smartIndent = smartIndent;
   }
   if (IsTopDocument(window))
   {
// TODO:       NeToggleButtonSetState(window->smartIndentItem, smartIndent, false);
// TODO:       NeToggleButtonSetState(window->autoIndentItem, autoIndent, false);
// TODO:       NeToggleButtonSetState(window->autoIndentOffItem, state == NO_AUTO_INDENT, false);
   }
}

// TODO: /*
// TODO: ** Set showMatching state to one of NO_FLASH, FLASH_DELIMIT or FLASH_RANGE.
// TODO: ** Update the menu to reflect the change of state.
// TODO: */
// TODO: void SetShowMatching(WindowInfo* window, int state)
// TODO: {
// TODO:    window->showMatchingStyle = state;
// TODO:    if (IsTopDocument(window))
// TODO:    {
// TODO:       NeToggleButtonSetState(window->showMatchingOffItem, state == NO_FLASH, false);
// TODO:       NeToggleButtonSetState(window->showMatchingDelimitItem, state == FLASH_DELIMIT, false);
// TODO:       NeToggleButtonSetState(window->showMatchingRangeItem, state == FLASH_RANGE, false);
// TODO:    }
// TODO: }

/*
** Update the "New (in X)" menu item to reflect the preferences
*/
void UpdateNewOppositeMenu(WindowInfo* window, int openInTab)
{
// TODO:    if (openInTab)
// TODO:       XtVaSetValues(window->menuBar->getNewOppositeItem(), XmNlabelString, NeNewString("New &Window"));
// TODO:    else
// TODO:       XtVaSetValues(window->menuBar->getNewOppositeItem(), XmNlabelString, NeNewString("New &Tab"));
}

// --------------------------------------------------------------------------
// Set the fonts for "window" from a font name, and updates the display.
// Also updates window->fontList which is used for statistics line.
//
// Note that this leaks memory and server resources.  In previous NEdit
// versions, fontLists were carefully tracked and freed, but X and Motif
// have some kind of timing problem when widgets are distroyed, such that
// fonts may not be freed immediately after widget destruction with 100%
// safety.  Rather than kludge around this with timerProcs, I have chosen
// to create new fontLists only when the user explicitly changes the font
// (which shouldn't happen much in normal NEdit operation), and skip the
// futile effort of freeing them.
// --------------------------------------------------------------------------
void SetFonts(WindowInfo* window, const char* fontName, const char* italicName, const char* boldName, const char* boldItalicName)
{
   TRACE();

   // Check which fonts have changed
   bool highlightChanged = false;
   bool primaryChanged = strcmp(fontName, window->fontName);
   if (strcmp(italicName, window->italicFontName)) highlightChanged = true;
   if (strcmp(boldName, window->boldFontName)) highlightChanged = true;
   if (strcmp(boldItalicName, window->boldItalicFontName)) highlightChanged = true;
   if (!primaryChanged && !highlightChanged)
      return;

// TODO:    /* Get information about the current window sizing, to be used to determine the correct window size after the font is changed */
// TODO:    XtVaGetValues(window->mainWindow, XmNwidth, &oldWindowWidth, XmNheight, &oldWindowHeight, NULL);
// TODO:    XtVaGetValues(window->textArea, XmNheight, &textHeight, , &marginHeight, textNmarginWidth, &marginWidth, textNfont, &oldFont, NULL);
// TODO:    oldTextWidth = textD->width + textD->lineNumWidth;
// TODO:    oldTextHeight = textHeight - 2*marginHeight;
// TODO:    for (i=0; i<window->nPanes; i++)
// TODO:    {
// TODO:       XtVaGetValues(window->textPanes[i], XmNheight, &textHeight, NULL);
// TODO:       oldTextHeight += textHeight - 2*marginHeight;
// TODO:    }
// TODO:    borderWidth = oldWindowWidth - oldTextWidth;
// TODO:    borderHeight = oldWindowHeight - oldTextHeight;
// TODO:    oldFontWidth = oldFont->max_bounds.width;
// TODO:    oldFontHeight = textD->ascent + textD->descent;
// TODO: 

   // Change the fonts in the window data structure.  If the primary font
   // didn't work, use Motif's fallback mechanism by stealing it from the
   // statistics line.  Highlight fonts are allowed to be NULL, which
   // is interpreted as "use the primary font"
   if (primaryChanged)
   {
      Ne_Font font; 
      if (font.init(fontName))
      {
         strcpy(window->fontName, fontName);
         window->fontList = font;
         window->textArea->primaryFont = font;
         window->textArea->redraw();
      }
   }
   if (highlightChanged)
   {
      strcpy(window->italicFontName, italicName);
      window->italicFontStruct = Ne_Font(italicName);
      strcpy(window->boldFontName, boldName);
      window->boldFontStruct = Ne_Font(boldName);
      strcpy(window->boldItalicFontName, boldItalicName);
      window->boldItalicFontStruct = Ne_Font(boldItalicName);
   }

   /* Change the primary font in all the widgets */
   if (primaryChanged)
   {
      const Ne_Font& font = GetDefaultFontStruct(window->fontList);
      window->textArea->primaryFont = font;
      // TODO: XtVaSetValues(window->textArea, textNfont, font, NULL);
      for (int i=0; i<window->nPanes; i++)
         window->textPanes[i]->primaryFont = font; // TODO: XtVaSetValues(window->textPanes[i], textNfont, font, NULL);
   }

   /* Change the highlight fonts, even if they didn't change, because
      primary font is read through the style table for syntax highlighting */
   if (window->highlightData != NULL)
      UpdateHighlightStyles(window);

// TODO:    /* Change the window manager size hints.
// TODO:       Note: this has to be done _before_ we set the new sizes. ICCCM2
// TODO:       compliant window managers (such as fvwm2) would otherwise resize
// TODO:       the window twice: once because of the new sizes requested, and once
// TODO:       because of the new size increments, resulting in an overshoot. */
// TODO:    UpdateWMSizeHints(window);
// TODO: 
// TODO:    /* Use the information from the old window to re-size the window to a
// TODO:       size appropriate for the new font, but only do so if there's only
// TODO:       _one_ document in the window, in order to avoid growing-window bug */
// TODO:    if (NDocuments(window) == 1)
// TODO:    {
// TODO:       fontWidth = GetDefaultFontStruct(window->fontList)->max_bounds.width;
// TODO:       fontHeight = textD->ascent + textD->descent;
// TODO:       newWindowWidth = (oldTextWidth*fontWidth) / oldFontWidth + borderWidth;
// TODO:       newWindowHeight = (oldTextHeight*fontHeight) / oldFontHeight +
// TODO:                         borderHeight;
// TODO:       XtVaSetValues(window->mainWindow, XmNwidth, newWindowWidth, XmNheight,
// TODO:                     newWindowHeight, NULL);
// TODO:    }
// TODO: 
// TODO:    /* Change the minimum pane height */
// TODO:    UpdateMinPaneHeights(window);
}

void SetColors(WindowInfo* window, const char* textFg, const char* textBg,
               const char* selectFg, const char* selectBg, const char* hiliteFg,
               const char* hiliteBg, const char* lineNoFg, const char* cursorFg)
{
   Fl_Color   textFgPix   = GetColor(textFg);
   Fl_Color   textBgPix   = GetColor(textBg);
   Fl_Color   selectFgPix = GetColor(selectFg);
   Fl_Color   selectBgPix = GetColor(selectBg);
   Fl_Color   hiliteFgPix = GetColor(hiliteFg);
   Fl_Color   hiliteBgPix = GetColor(hiliteBg);
   Fl_Color   lineNoFgPix = GetColor(lineNoFg);
   Fl_Color   cursorFgPix = GetColor(cursorFg);

   // Update the main pane
   window->textArea->bgPixel = textBgPix;
   window->textArea->fgPixel = textFgPix;
   window->textArea->cursorFGPixel = cursorFgPix;

   TextDSetColors(window->textArea, textFgPix, textBgPix, selectFgPix, selectBgPix, hiliteFgPix, hiliteBgPix, lineNoFgPix, cursorFgPix);
   
   // Update any additional panes
   for (int i=0; i<window->nPanes; i++)
   {
      Ne_Text_Editor* textD = window->textPanes[i];
      TextDSetColors(textD, textFgPix, textBgPix, selectFgPix, selectBgPix, hiliteFgPix, hiliteBgPix, lineNoFgPix, cursorFgPix);
   }

   // Redo any syntax highlighting
   if (window->highlightData != NULL)
      UpdateHighlightStyles(window);
}

// TODO: /*
// TODO: ** Set insert/overstrike mode
// TODO: */
// TODO: void SetOverstrike(WindowInfo* window, int overstrike)
// TODO: {
// TODO:    int i;
// TODO: 
// TODO:    XtVaSetValues(window->textArea, textNoverstrike, overstrike, NULL);
// TODO:    for (i=0; i<window->nPanes; i++)
// TODO:       XtVaSetValues(window->textPanes[i], textNoverstrike, overstrike, NULL);
// TODO:    window->overstrike = overstrike;
// TODO: }

/*
** Select auto-wrap mode, one of NO_WRAP, NEWLINE_WRAP, or CONTINUOUS_WRAP
*/
void SetAutoWrap(WindowInfo* window, int state)
{
   int i;
   int autoWrap = state == NEWLINE_WRAP, contWrap = state == CONTINUOUS_WRAP;

   window->textArea->text.autoWrap = autoWrap;
   window->textArea->continuousWrap = contWrap;
   for (i=0; i<window->nPanes; i++)
   {
      window->textPanes[i]->text.autoWrap = autoWrap;
      window->textPanes[i]->continuousWrap = contWrap;
   }
   window->wrapMode = state;

   if (IsTopDocument(window))
   {
// TODO:       NeToggleButtonSetState(window->newlineWrapItem, autoWrap, false);
// TODO:       NeToggleButtonSetState(window->continuousWrapItem, contWrap, false);
// TODO:       NeToggleButtonSetState(window->noWrapItem, state == NO_WRAP, false);
   }
}

/*
** Set the auto-scroll margin
*/
void SetAutoScroll(WindowInfo* window, int margin)
{
// TODO:    XtVaSetValues(window->textArea, textNcursorVPadding, margin, NULL);
// TODO:    for (int i=0; i<window->nPanes; i++)
// TODO:       XtVaSetValues(window->textPanes[i], textNcursorVPadding, margin, NULL);
}

/*
** Recover the window pointer from any widget in the window, by searching
** up the widget hierarcy for the top level container widget where the
** window pointer is stored in the userData field. In a tabbed window,
** this is the window pointer of the top (active) document, which is
** returned if w is 'shell-level' widget - menus, find/replace dialogs, etc.
**
** To support action routine in tabbed windows, a copy of the window
** pointer is also store in the splitPane widget.
*/
WindowInfo* WidgetToWindow(Fl_Widget* w)
{
   Fl_Widget* parent = WidgetToMainWindow(w);

// TODO:       /* make sure it is not a dialog shell */

   WindowInfo* window = NULL;
   for (window=WindowList; window!=NULL; window=window->next)
     if (window->mainWindow == parent)
       break;

   if (window == NULL)
      throw "No WindowInfo found";

   Fl_Widget* selectedTabs = window->tab->value();
   for (window=WindowList; window!=NULL; window=window->next)
      if (window->textArea == selectedTabs)
         return window;

   throw "No WindowInfo found for the tabs";
   return 0;
}

/*
** Change the window appearance and the window data structure to show
** that the file it contains has been modified
*/
void SetWindowModified(WindowInfo* window, int modified)
{
   if (window->fileChanged == false && modified == true)
   {
      NeSetSensitive(window->menuBar->getCloseItem(), true);
      window->fileChanged = true;
      UpdateWindowTitle(window);
      RefreshTabState(window);
   }
   else if (window->fileChanged == true && modified == false)
   {
      window->fileChanged = false;
      UpdateWindowTitle(window);
      RefreshTabState(window);
   }
}

/*
** Update the window title to reflect the filename, read-only, and modified
** status of the window data structure
*/
void UpdateWindowTitle(const WindowInfo* window)
{
   char* iconTitle, *title;

   if (!IsTopDocument(window))
      return;

   title = FormatWindowTitle(window->filename,
                             window->path,
                             GetPrefServerName(),
                             IsServer,
                             window->filenameSet,
                             window->lockReasons,
                             window->fileChanged,
                             GetPrefTitleFormat());

   iconTitle = new char[strlen(window->filename) + 2];

   strcpy(iconTitle, window->filename);
   if (window->fileChanged)
      strcat(iconTitle, "*");

   window->mainWindow->label(title, iconTitle);

// TODO:    /* If there's a find or replace dialog up in "Keep Up" mode, with a
// TODO:       file name in the title, update it too */
// TODO:    if (window->findDlog && NeToggleButtonGetState(window->findKeepBtn))
// TODO:    {
// TODO:       sprintf(title, "Find (in %s)", window->filename);
// TODO:       XtVaSetValues(XtParent(window->findDlog), XmNtitle, title, NULL);
// TODO:    }
// TODO:    if (window->replaceDlog && NeToggleButtonGetState(window->replaceKeepBtn))
// TODO:    {
// TODO:       sprintf(title, "Replace (in %s)", window->filename);
// TODO:       XtVaSetValues(XtParent(window->replaceDlog), XmNtitle, title, NULL);
// TODO:    }
   delete[] iconTitle;

   // Update the Windows menus with the new name
   InvalidateWindowMenus();
}

/*
** Update the read-only state of the text area(s) in the window, and
** the ReadOnly toggle button in the File menu to agree with the state in
** the window data structure.
*/
void UpdateWindowReadOnly(WindowInfo* window)
{
   int i, state;

   if (!IsTopDocument(window))
      return;

// TODO:    state = IS_ANY_LOCKED(window->lockReasons);
// TODO:    XtVaSetValues(window->textArea, textNreadOnly, state, NULL);
// TODO:    for (i=0; i<window->nPanes; i++)
// TODO:       XtVaSetValues(window->textPanes[i], textNreadOnly, state, NULL);
// TODO:    NeToggleButtonSetState(window->readOnlyItem, state, false);
// TODO:    NeSetSensitive(window->readOnlyItem, !IS_ANY_LOCKED_IGNORING_USER(window->lockReasons));
}


// Find the start and end of a single line selection.  Hides rectangular
// selection issues for older routines which use selections that won't span lines.
int GetSimpleSelection(Ne_Text_Buffer* buf, int* left, int* right)
{
   int selStart, selEnd, isRect, rectStart, rectEnd, lineStart;

   // get the character to match and its position from the selection, or
   // the character before the insert point if nothing is selected.
   // Give up if too many characters are selected
   if (!BufGetSelectionPos(buf, &selStart, &selEnd, &isRect, &rectStart, &rectEnd))
      return false;

   if (isRect)
   {
      lineStart = BufStartOfLine(buf, selStart);
      selStart = BufCountForwardDispChars(buf, lineStart, rectStart);
      selEnd = BufCountForwardDispChars(buf, lineStart, rectEnd);
   }
   
   *left = selStart;
   *right = selEnd;
   
   return true;
}

/*
** If the selection (or cursor position if there's no selection) is not
** fully shown, scroll to bring it in to view.  Note that as written,
** this won't work well with multi-line selections.  Modest re-write
** of the horizontal scrolling part would be quite easy to make it work
** well with rectangular selections.
*/
void MakeSelectionVisible(WindowInfo* window, Fl_Widget* textPane)
{
   int left, right, isRect, rectStart, rectEnd, horizOffset;
   int scrollOffset, leftX, rightX, y, rows, margin;
   int topLineNum, lastLineNum, rightLineNum, leftLineNum, linesToScroll;
// TODO:    textDisp* textD = ((TextWidget)textPane)->text.textD;
// TODO:    int topChar = TextFirstVisiblePos(textPane);
// TODO:    int lastChar = TextLastVisiblePos(textPane);
// TODO:    int targetLineNum;
// TODO:    int width;
// TODO: 
// TODO:    /* find out where the selection is */
// TODO:    if (!BufGetSelectionPos(window->buffer, &left, &right, &isRect,
// TODO:                            &rectStart, &rectEnd))
// TODO:    {
// TODO:       left = right = TextGetCursorPos(textPane);
// TODO:       isRect = false;
// TODO:    }
// TODO: 
// TODO:    /* Check vertical positioning unless the selection is already shown or
// TODO:       already covers the display.  If the end of the selection is below
// TODO:       bottom, scroll it in to view until the end selection is scrollOffset
// TODO:       lines from the bottom of the display or the start of the selection
// TODO:       scrollOffset lines from the top.  Calculate a pleasing distance from the
// TODO:       top or bottom of the window, to scroll the selection to (if scrolling is
// TODO:       necessary), around 1/3 of the height of the window */
// TODO:    if (!((left >= topChar && right <= lastChar) ||
// TODO:          (left <= topChar && right >= lastChar)))
// TODO:    {
// TODO:       XtVaGetValues(textPane, textNrows, &rows, NULL);
// TODO:       scrollOffset = rows/3;
// TODO:       TextGetScroll(textPane, &topLineNum, &horizOffset);
// TODO:       if (right > lastChar)
// TODO:       {
// TODO:          /* End of sel. is below bottom of screen */
// TODO:          leftLineNum = topLineNum +
// TODO:                        TextDCountLines(textD, topChar, left, false);
// TODO:          targetLineNum = topLineNum + scrollOffset;
// TODO:          if (leftLineNum >= targetLineNum)
// TODO:          {
// TODO:             /* Start of sel. is not between top & target */
// TODO:             linesToScroll = TextDCountLines(textD, lastChar, right, false) +
// TODO:                             scrollOffset;
// TODO:             if (leftLineNum - linesToScroll < targetLineNum)
// TODO:                linesToScroll = leftLineNum - targetLineNum;
// TODO:             /* Scroll start of selection to the target line */
// TODO:             TextSetScroll(textPane, topLineNum+linesToScroll, horizOffset);
// TODO:          }
// TODO:       }
// TODO:       else if (left < topChar)
// TODO:       {
// TODO:          /* Start of sel. is above top of screen */
// TODO:          lastLineNum = topLineNum + rows;
// TODO:          rightLineNum = lastLineNum -
// TODO:                         TextDCountLines(textD, right, lastChar, false);
// TODO:          targetLineNum = lastLineNum - scrollOffset;
// TODO:          if (rightLineNum <= targetLineNum)
// TODO:          {
// TODO:             /* End of sel. is not between bottom & target */
// TODO:             linesToScroll = TextDCountLines(textD, left, topChar, false) +
// TODO:                             scrollOffset;
// TODO:             if (rightLineNum + linesToScroll > targetLineNum)
// TODO:                linesToScroll = targetLineNum - rightLineNum;
// TODO:             /* Scroll end of selection to the target line */
// TODO:             TextSetScroll(textPane, topLineNum-linesToScroll, horizOffset);
// TODO:          }
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* If either end of the selection off screen horizontally, try to bring it
// TODO:       in view, by making sure both end-points are visible.  Using only end
// TODO:       points of a multi-line selection is not a great idea, and disaster for
// TODO:       rectangular selections, so this part of the routine should be re-written
// TODO:       if it is to be used much with either.  Note also that this is a second
// TODO:       scrolling operation, causing the display to jump twice.  It's done after
// TODO:       vertical scrolling to take advantage of TextPosToXY which requires it's
// TODO:       reqested position to be vertically on screen) */
// TODO:    if (TextPosToXY(textPane, left, &leftX, &y) &&
// TODO:          TextPosToXY(textPane, right, &rightX, &y) && leftX <= rightX)
// TODO:    {
// TODO:       TextGetScroll(textPane, &topLineNum, &horizOffset);
// TODO:       XtVaGetValues(textPane, XmNwidth, &width, textNmarginWidth, &margin,
// TODO:                     NULL);
// TODO:       if (leftX < margin + textD->lineNumLeft + textD->lineNumWidth)
// TODO:          horizOffset -=
// TODO:             margin + textD->lineNumLeft + textD->lineNumWidth - leftX;
// TODO:       else if (rightX > width - margin)
// TODO:          horizOffset += rightX - (width - margin);
// TODO:       TextSetScroll(textPane, topLineNum, horizOffset);
// TODO:    }

   /* make sure that the statistics line is up to date */
   UpdateStatsLine(window);
}

static Ne_Text_Editor* createTextArea(WindowInfo* window, int rows, int cols, int emTabDist, char* delimiters, int wrapMargin, int lineNumCols)
{
   // Create a text widget inside of a scrolled window widget
   int x, y, w, h;
   window->tab->client_area(x, y, w, h);

   Ne_Text_Editor* textD = new Ne_Text_Editor(x, y, w, h);
   textD->text.backlightCharTypes = window->backlightCharTypes?window->backlightCharTypes:"";
   textD->lineNumCols = lineNumCols;
   textD->text.emulateTabs = emTabDist;
   textD->primaryFont = window->fontList;
   textD->text.readOnly = IS_ANY_LOCKED(window->lockReasons);
   textD->text.delimiters = delimiters;
   textD->wrapMargin = wrapMargin;
   textD->text.autoIndent = window->indentStyle == AUTO_INDENT;
   textD->text.smartIndent = window->indentStyle == SMART_INDENT;
   textD->text.autoWrap = window->wrapMode == NEWLINE_WRAP;
   textD->continuousWrap = window->wrapMode == CONTINUOUS_WRAP;
   textD->text.overstrike = window->overstrike;
   textD->text.hidePointer = GetPrefTypingHidesPointer();
   textD->text.cursorVPadding = GetVerticalAutoScroll();

   TextInitialize(textD);

   ///* add focus, drag, cursor tracking, and smart indent callbacks */
   //XtAddCallback(text, textNfocusCallback, (XtCallbackProc)focusCB, window);
   //XtAddCallback(text, textNcursorMovementCallback, (XtCallbackProc)movedCB, window);
   //XtAddCallback(text, textNdragStartCallback, (XtCallbackProc)dragStartCB, window);
   //XtAddCallback(text, textNdragEndCallback, (XtCallbackProc)dragEndCB, window);
   //XtAddCallback(text, textNsmartIndentCallback, SmartIndentCB, window);

   ///* This makes sure the text area initially has a the insert point shown
   //   ... (check if still true with the nedit text widget, probably not) */
   //XmAddTabGroup(containingPane(text));

   ///* Augment translation table for right button popup menu */
   //AddBGMenuAction(text);

   /* If absolute line numbers will be needed for display in the statistics
      line, tell the widget to maintain them (otherwise, it's a costly
      operation and performance will be better without it) */
   TextDMaintainAbsLineNum(textD, window->showStats);

   return textD;
}

static void movedCB(Fl_Widget* w, void* data)
{
   TRACE();
   Ne_Text_Editor* text= (Ne_Text_Editor*) w;
   WindowInfo* window = (WindowInfo*)data;

   if (window->ignoreModify)
      return;

   /* update line and column nubers in statistics line */
   UpdateStatsLine(window);

   /* Check the character before the cursor for matchable characters */
   FlashMatching(window, text);

   /* Check for changes to read-only status and/or file modifications */
   CheckForChangesToFile(window);

// TODO:    /*  This callback is not only called for focussed panes, but for newly
// TODO:        created panes as well. So make sure that the cursor is left alone
// TODO:        for unfocussed panes.
// TODO:        TextWidget have no state per se about focus, so we use the related
// TODO:        ID for the blink procedure.  */
// TODO:    if (0 != textWidget->text.cursorBlinkProcID)
// TODO:    {
// TODO:       /*  Start blinking the caret again.  */
// TODO:       ResetCursorBlink(textWidget, false);
// TODO:    }
}

static void modifiedCB(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg)
{
   WindowInfo* window = (WindowInfo*)cbArg;
   bool selected = window->buffer->primary.selected;

   /* update the table of bookmarks */
   if (!window->ignoreModify)
   {
      UpdateMarkTable(window, pos, nInserted, nDeleted);
   }

   /* Check and dim/undim selection related menu items */
   if ((window->wasSelected && !selected) || (!window->wasSelected && selected))
   {
      window->wasSelected = selected;

      /* do not refresh shell-level items (window, menu-bar etc) when motifying non-top document */
      if (IsTopDocument(window))
      {
// TODO:          NeSetSensitive(window->menuBar->getPrintSelectionItem(), selected);
// TODO:          NeSetSensitive(window->cutItem, selected);
// TODO:          NeSetSensitive(window->copyItem, selected);
// TODO:          NeSetSensitive(window->delItem, selected);
         /* Note we don't change the selection for items like
            "Open Selected" and "Find Selected".  That's because
            it works on selections in external applications.
            Desensitizing it if there's no NEdit selection
            disables this feature. */
// TODO:          NeSetSensitive(window->filterItem, selected);

// TODO:          DimSelectionDepUserMenuItems(window, selected);
// TODO:          if (window->replaceDlog != NULL && XtIsManaged(window->replaceDlog))
// TODO:          {
// TODO:             UpdateReplaceActionButtons(window);
// TODO:          }
      }
   }

   /* When the program needs to make a change to a text area without without
      recording it for undo or marking file as changed it sets ignoreModify */
   if (window->ignoreModify || (nDeleted == 0 && nInserted == 0))
      return;

   /* Make sure line number display is sufficient for new data */
   updateLineNumDisp(window);

   // Save information for undoing this operation (this call also counts
   // characters and editing operations for triggering autosave
   SaveUndoInformation(window, pos, nInserted, nDeleted, deletedText);

   /* Trigger automatic backup if operation or character limits reached */
   if (window->autoSave &&
         (window->autoSaveCharCount > AUTOSAVE_CHAR_LIMIT ||
          window->autoSaveOpCount > AUTOSAVE_OP_LIMIT))
   {
      WriteBackupFile(window);
      window->autoSaveCharCount = 0;
      window->autoSaveOpCount = 0;
   }

   /* Indicate that the window has now been modified */
   SetWindowModified(window, true);

   /* Update # of bytes, and line and col statistics */
   UpdateStatsLine(window);

   /* Check if external changes have been made to file and warn user */
   CheckForChangesToFile(window);
}

void AllWindowEventCB(Fl_Widget* w, void* data)
{
   TRACE();
   WindowInfo* window = (WindowInfo*)data;
   Ne_Text_Editor* text = (Ne_Text_Editor*)w;
   
   movedCB(w, data);
}

// TODO: static void focusCB(Fl_Widget* w, WindowInfo* window, XtPointer callData)
// TODO: {
// TODO:    /* record which window pane last had the keyboard focus */
// TODO:    window->lastFocus = w;
// TODO: 
// TODO:    /* update line number statistic to reflect current focus pane */
// TODO:    UpdateStatsLine(window);
// TODO: 
// TODO:    /* finish off the current incremental search */
// TODO:    EndISearch(window);
// TODO: 
// TODO:    /* Check for changes to read-only status and/or file modifications */
// TODO:    CheckForChangesToFile(window);
// TODO: }
// TODO: 
// TODO: static void dragStartCB(Fl_Widget* w, WindowInfo* window, XtPointer callData)
// TODO: {
// TODO:    /* don't record all of the intermediate drag steps for undo */
// TODO:    window->ignoreModify = true;
// TODO: }
// TODO: 
// TODO: static void dragEndCB(Fl_Widget* w, WindowInfo* window, dragEndCBStruct* callData)
// TODO: {
// TODO:    /* restore recording of undo information */
// TODO:    window->ignoreModify = false;
// TODO: 
// TODO:    /* Do nothing if drag operation was canceled */
// TODO:    if (callData->nCharsInserted == 0)
// TODO:       return;
// TODO: 
// TODO:    /* Save information for undoing this operation not saved while
// TODO:       undo recording was off */
// TODO:    modifiedCB(callData->startPos, callData->nCharsInserted,
// TODO:               callData->nCharsDeleted, 0, callData->deletedText, window);
// TODO: }
// TODO: 
// TODO: static void closeCB(Fl_Widget* w, WindowInfo* window, XtPointer callData)
// TODO: {
// TODO:    window = WidgetToWindow(w);
// TODO:    if (!WindowCanBeClosed(window))
// TODO:    {
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    CloseDocumentWindow(w, window, callData);
// TODO: }
// TODO: 
// TODO: #ifndef NO_SESSION_RESTART
// TODO: static void saveYourselfCB(Fl_Widget* w, Fl_Widget* appShell, XtPointer callData)
// TODO: {
// TODO:    WindowInfo* win, *topWin, **revWindowList;
// TODO:    char geometry[MAX_GEOM_STRING_LEN];
// TODO:    int argc = 0, maxArgc, nWindows, i;
// TODO:    char** argv;
// TODO:    int wasIconic = false;
// TODO:    int n, nItems;
// TODO:    WidgetList children;
// TODO: 
// TODO:    /* Allocate memory for an argument list and for a reversed list of
// TODO:       windows.  The window list is reversed for IRIX 4DWM and any other
// TODO:       window/session manager combination which uses window creation
// TODO:       order for re-associating stored geometry information with
// TODO:       new windows created by a restored application */
// TODO:    maxArgc = 4;  /* nedit -server -svrname name */
// TODO:    nWindows = 0;
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       maxArgc += 5;  /* -iconic -group -geometry WxH+x+y filename */
// TODO:       nWindows++;
// TODO:    }
// TODO:    argv = (char**)malloc__(maxArgc*sizeof(char*));
// TODO:    revWindowList = (WindowInfo**)malloc__(sizeof(WindowInfo*)*nWindows);
// TODO:    for (win=WindowList, i=nWindows-1; win!=NULL; win=win->next, i--)
// TODO:       revWindowList[i] = win;
// TODO: 
// TODO:    /* Create command line arguments for restoring each window in the list */
// TODO:    argv[argc++] = NeNewString(ArgV0);
// TODO:    if (IsServer)
// TODO:    {
// TODO:       argv[argc++] = NeNewString("-server");
// TODO:       if (GetPrefServerName()[0] != '\0')
// TODO:       {
// TODO:          argv[argc++] = NeNewString("-svrname");
// TODO:          argv[argc++] = NeNewString(GetPrefServerName());
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* editor windows are popup-shell children of top-level appShell */
// TODO:    XtVaGetValues(appShell, XmNchildren, &children,
// TODO:                  XmNnumChildren, &nItems, NULL);
// TODO: 
// TODO:    for (n=nItems-1; n>=0; n--)
// TODO:    {
// TODO:       WidgetList tabs;
// TODO:       int tabCount;
// TODO: 
// TODO:       if (strcmp(XtName(children[n]), "textShell") ||
// TODO:             ((topWin = WidgetToWindow(children[n])) == NULL))
// TODO:          continue;   /* skip non-editor windows */
// TODO: 
// TODO:       /* create a group for each window */
// TODO:       getGeometryString(topWin, geometry);
// TODO:       argv[argc++] = NeNewString("-group");
// TODO:       argv[argc++] = NeNewString("-geometry");
// TODO:       argv[argc++] = NeNewString(geometry);
// TODO:       if (IsIconic(topWin))
// TODO:       {
// TODO:          argv[argc++] = NeNewString("-iconic");
// TODO:          wasIconic = true;
// TODO:       }
// TODO:       else if (wasIconic)
// TODO:       {
// TODO:          argv[argc++] = NeNewString("-noiconic");
// TODO:          wasIconic = false;
// TODO:       }
// TODO: 
// TODO:       /* add filename of each tab in window... */
// TODO:       XtVaGetValues(topWin->tabBar, XmNtabWidgetList, &tabs,
// TODO:                     XmNtabCount, &tabCount, NULL);
// TODO: 
// TODO:       for (i=0; i< tabCount; i++)
// TODO:       {
// TODO:          win = TabToWindow(tabs[i]);
// TODO:          if (win->filenameSet)
// TODO:          {
// TODO:             /* add filename */
// TODO:             argv[argc] = malloc__(strlen(win->path) +
// TODO:                                   strlen(win->filename) + 1);
// TODO:             sprintf(argv[argc++], "%s%s", win->path, win->filename);
// TODO:          }
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    free__((char*)revWindowList);
// TODO: 
// TODO:    /* Set the window's WM_COMMAND property to the created command line */
// TODO:    XSetCommand(TheDisplay, XtWindow(appShell), argv, argc);
// TODO:    for (i=0; i<argc; i++)
// TODO:       free__(argv[i]);
// TODO:    free__((char*)argv);
// TODO: }
// TODO: 
// TODO: void AttachSessionMgrHandler(Fl_Widget* appShell)
// TODO: {
// TODO:    static Atom wmpAtom, syAtom = 0;
// TODO: 
// TODO:    /* Add wm protocol callback for making nedit restartable by session
// TODO:       managers.  Doesn't yet handle multiple-desktops or iconifying right. */
// TODO:    if (syAtom == 0)
// TODO:    {
// TODO:       wmpAtom = XmInternAtom(TheDisplay, "WM_PROTOCOLS", false);
// TODO:       syAtom = XmInternAtom(TheDisplay, "WM_SAVE_YOURSELF", false);
// TODO:    }
// TODO:    XmAddProtocolCallback(appShell, wmpAtom, syAtom,
// TODO:                          (XtCallbackProc)saveYourselfCB, (XtPointer)appShell);
// TODO: }
// TODO: #endif /* NO_SESSION_RESTART */

/*
** Returns true if window is iconic (as determined by the WM_STATE property
** on the shell window.  I think this is the most reliable way to tell,
** but if someone has a better idea please send me a note).
*/
int IsIconic(WindowInfo* window)
{
   int result = 0;
// TODO:    unsigned long* property = NULL;
// TODO:    unsigned long nItems;
// TODO:    unsigned long leftover;
// TODO:    static Atom wmStateAtom = 0;
// TODO:    Atom actualType;
// TODO:    int actualFormat;
// TODO: 
// TODO:    if (wmStateAtom == 0)
// TODO:       wmStateAtom = XInternAtom(XtDisplay(window->mainWindow), "WM_STATE", false);
// TODO:    if (XGetWindowProperty(XtDisplay(window->mainWindow), XtWindow(window->mainWindow),
// TODO:                           wmStateAtom, 0L, 1L, false, wmStateAtom, &actualType, &actualFormat,
// TODO:                           &nItems, &leftover, (unsigned char**)&property) != Success ||
// TODO:          nItems != 1 || property == NULL)
// TODO:       return false;
// TODO:    result = *property == IconicState;
// TODO:    free__((char*)property);
   return result;
}

/*
** Add a window to the the window list.
*/
static void addToWindowList(WindowInfo* window)
{
   WindowInfo* temp = WindowList;
   WindowList = window;
   window->next = temp;
}

/*
** Remove a window from the list of windows
*/
static void removeFromWindowList(WindowInfo* window)
{
   WindowInfo* temp;

   if (WindowList == window)
      WindowList = window->next;
   else
   {
      for (temp = WindowList; temp != NULL; temp = temp->next)
      {
         if (temp->next == window)
         {
            temp->next = window->next;
            break;
         }
      }
   }
}

/*
**  Set the new gutter width in the window. Sadly, the only way to do this is
**  to set it on every single document, so we have to iterate over them.
**
**  (Iteration taken from NDocuments(); is there a better way to do it?)
*/
static int updateGutterWidth(WindowInfo* window)
{
   WindowInfo* document;
   int reqCols = MIN_LINE_NUM_COLS;
   int newColsDiff = 0;
   int maxCols = 0;

   for (document = WindowList; NULL != document; document = document->next)
   {
      if (document->mainWindow == window->mainWindow)
      {
         /*  We found ourselves a document from this window.  */
         int lineNumCols, tmpReqCols;
         Ne_Text_Editor* textD = document->textArea;

         lineNumCols = document->textArea->lineNumCols;

         /* Is the width of the line number area sufficient to display all the
            line numbers in the file?  If not, expand line number field, and the
            window width. */

         if (lineNumCols > maxCols)
         {
            maxCols = lineNumCols;
         }

         tmpReqCols = textD->nBufferLines < 1
                      ? 1
                      : (int) log10((double) textD->nBufferLines + 1) + 1;

         if (tmpReqCols > reqCols)
         {
            reqCols = tmpReqCols;
         }
      }
   }

   if (reqCols != maxCols)
   {
      Ne_Font* fs;
      short fontWidth;

      newColsDiff = reqCols - maxCols;

      fs = &window->textArea->primaryFont;
      fontWidth = fs->max_width();

      int windowWidth = window->textArea->width;
      window->mainWindow->size( windowWidth + (newColsDiff * fontWidth) , window->mainWindow->h());

      UpdateWMSizeHints(window);
   }

   for (document = WindowList; NULL != document; document = document->next)
   {
      if (document->mainWindow == window->mainWindow)
      {
         int i;
         int lineNumCols;

         lineNumCols = document->textArea->lineNumCols;

         if (lineNumCols == reqCols)
         {
            continue;
         }

         /*  Update all panes of this document.  */
         for (i = 0; i <= document->nPanes; i++)
         {
            Ne_Text_Editor* text = 0==i ? document->textArea : document->textPanes[i-1];
            text->lineNumCols = reqCols;
         }
      }
   }

   return reqCols;
// TODO: 
// TODO:    WindowInfo* document;
// TODO:    int reqCols = MIN_LINE_NUM_COLS;
// TODO:    int newColsDiff = 0;
// TODO:    int maxCols = 0;
// TODO: 
// TODO:    for (document = WindowList; NULL != document; document = document->next)
// TODO:    {
// TODO:       if (document->mainWindow == window->mainWindow)
// TODO:       {
// TODO:          /*  We found ourselves a document from this window.  */
// TODO:          int lineNumCols, tmpReqCols;
// TODO: // TODO:          document->textArea->position_to_linecol(document->textArea->insert_position(), &lineNumCols, &tmpReqCols);
// TODO: 
// TODO:          // Is the width of the line number area sufficient to display all the
// TODO:          // line numbers in the file?  If not, expand line number field, and the
// TODO:          // window width.
// TODO:          if (lineNumCols > maxCols)
// TODO:          {
// TODO:             maxCols = lineNumCols;
// TODO:          }
// TODO: 
// TODO: // TODO:          tmpReqCols = textD->nBufferLines < 1
// TODO: // TODO:                       ? 1
// TODO: // TODO:                       : (int) log10((double) textD->nBufferLines + 1) + 1;
// TODO: // TODO: 
// TODO: // TODO:          if (tmpReqCols > reqCols)
// TODO: // TODO:          {
// TODO: // TODO:             reqCols = tmpReqCols;
// TODO: // TODO:          }
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    if (reqCols != maxCols)
// TODO:    {
// TODO: // TODO:       XFontStruct* fs;
// TODO: // TODO:       int windowWidth;
// TODO: // TODO:       short fontWidth;
// TODO: // TODO: 
// TODO: // TODO:       newColsDiff = reqCols - maxCols;
// TODO: // TODO: 
// TODO: // TODO:       XtVaGetValues(window->textArea, textNfont, &fs, NULL);
// TODO: // TODO:       fontWidth = fs->max_bounds.width;
// TODO: // TODO: 
// TODO: // TODO:       XtVaGetValues(window->mainWindow, XmNwidth, &windowWidth, NULL);
// TODO: // TODO:       XtVaSetValues(window->mainWindow,
// TODO: // TODO:                     XmNwidth, (int) windowWidth + (newColsDiff * fontWidth),
// TODO: // TODO:                     NULL);
// TODO: // TODO: 
// TODO: // TODO:       UpdateWMSizeHints(window);
// TODO:    }
// TODO: 
// TODO:    for (document = WindowList; NULL != document; document = document->next)
// TODO:    {
// TODO:       if (document->mainWindow == window->mainWindow)
// TODO:       {
// TODO:          Fl_Widget* text;
// TODO:          int i;
// TODO:          int lineNumCols;
// TODO: 
// TODO: // TODO:          XtVaGetValues(document->textArea,
// TODO: // TODO:                        textNlineNumCols, &lineNumCols, NULL);
// TODO: // TODO: 
// TODO: // TODO:          if (lineNumCols == reqCols)
// TODO: // TODO:          {
// TODO: // TODO:             continue;
// TODO: // TODO:          }
// TODO: // TODO: 
// TODO: // TODO:          /*  Update all panes of this document.  */
// TODO: // TODO:          for (i = 0; i <= document->nPanes; i++)
// TODO: // TODO:          {
// TODO: // TODO:             text = 0==i ? document->textArea : document->textPanes[i-1];
// TODO: // TODO:             XtVaSetValues(text, textNlineNumCols, reqCols, NULL);
// TODO: // TODO:          }
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    return reqCols;
}

/*
**  If necessary, enlarges the window and line number display area to make
**  room for numbers.
*/
static int updateLineNumDisp(WindowInfo* window)
{
   if (!window->showLineNumbers)
   {
      return 0;
   }

   /* Decide how wide the line number field has to be to display all
      possible line numbers */
   return updateGutterWidth(window);
}

/*
** Update the optional statistics line.
*/
void UpdateStatsLine(WindowInfo* window)
{
   int line, pos, colNum;
   char* str, slinecol[32];
   NeString xmslinecol;

   if (!IsTopDocument(window))
      return;

   /* This routine is called for each character typed, so its performance
      affects overall editor perfomance.  Only update if the line is on. */
   if (!window->showStats || !window->statsLineForm)
      return;

   /* Compose the string to display. If line # isn't available, leave it off */
   pos = TextGetCursorPos(window->lastFocus);
   str = new char[strlen(window->filename) + strlen(window->path) + 45];
   const char* format = window->fileFormat == DOS_FILE_FORMAT ? " DOS" :
            (window->fileFormat == MAC_FILE_FORMAT ? " Mac" : "");

   if (!TextPosToLineAndCol((Ne_Text_Editor*)(window->lastFocus), pos, &line, &colNum))
   {
      sprintf(str, "%s%s%s %d bytes", window->path, window->filename, format, window->buffer->length);
      sprintf(slinecol, "L: ---  C: ---");
   }
   else
   {
      sprintf(slinecol, "L: %d  C: %d", line, colNum);
      if (window->showLineNumbers)
         sprintf(str, "%s%s%s byte %d of %d", window->path, window->filename, format, pos, window->buffer->length);
      else
         sprintf(str, "%s%s%s %d bytes", window->path, window->filename, format, window->buffer->length);
   }

   /* Don't clobber the line if there's a special message being displayed */
   if (!window->modeMessageDisplayed)
   {
      /* Change the text in the stats line */
      window->statsLineForm->statsLine().copy_label(str);
      window->statsLineForm->statsLine().redraw();
   }
   delete[] str;

   /* Update the line/col display */
   xmslinecol = NeNewString(slinecol);
   window->statsLineForm->statsLineColNo().copy_label(xmslinecol);
   window->statsLineForm->statsLineColNo().redraw();
   NeStringFree(xmslinecol);
}

static bool currentlyBusy = false;
static long busyStartTime = 0;
static bool modeMessageSet = false;

// TODO: /*
// TODO:  * Auxiliary function for measuring elapsed time during busy waits.
// TODO:  */
// TODO: static long getRelTimeInTenthsOfSeconds()
// TODO: {
// TODO: #ifdef __unix__
// TODO:    struct timeval current;
// TODO:    gettimeofday(&current, NULL);
// TODO:    return (current.tv_sec*10 + current.tv_usec/100000) & 0xFFFFFFFL;
// TODO: #else
// TODO:    time_t current;
// TODO:    time(&current);
// TODO:    return (current*10) & 0xFFFFFFFL;
// TODO: #endif
// TODO: }

void AllWindowsBusy(const char* message)
{
// TODO:    WindowInfo* w;
// TODO: 
// TODO:    if (!currentlyBusy)
// TODO:    {
// TODO:       busyStartTime = getRelTimeInTenthsOfSeconds();
// TODO:       modeMessageSet = false;
// TODO: 
// TODO:       for (w=WindowList; w!=NULL; w=w->next)
// TODO:       {
// TODO:          /* We don't the display message here yet, but defer it for
// TODO:             a while. If the wait is short, we don't want
// TODO:             to have it flash on and off the screen.  However,
// TODO:             we can't use a time since in generally we are in
// TODO:             a tight loop and only processing exposure events, so it's
// TODO:             up to the caller to make sure that this routine is called
// TODO:             at regular intervals.
// TODO:          */
// TODO:          BeginWait(w->mainWindow);
// TODO:       }
// TODO:    }
// TODO:    else if (!modeMessageSet && message &&
// TODO:             getRelTimeInTenthsOfSeconds() - busyStartTime > 10)
// TODO:    {
// TODO:       /* Show the mode message when we've been busy for more than a second */
// TODO:       for (w=WindowList; w!=NULL; w=w->next)
// TODO:       {
// TODO:          SetModeMessage(w, message);
// TODO:       }
// TODO:       modeMessageSet = true;
// TODO:    }
// TODO:    BusyWait(WindowList->mainWindow);
// TODO: 
// TODO:    currentlyBusy = true;
}

void AllWindowsUnbusy()
{
// TODO:    WindowInfo* w;
// TODO: 
// TODO:    for (w=WindowList; w!=NULL; w=w->next)
// TODO:    {
// TODO:       ClearModeMessage(w);
// TODO:       EndWait(w->mainWindow);
// TODO:    }
// TODO: 
// TODO:    currentlyBusy = false;
// TODO:    modeMessageSet = false;
// TODO:    busyStartTime = 0;
}

// TODO: /*
// TODO: ** Paned windows are impossible to adjust after they are created, which makes
// TODO: ** them nearly useless for NEdit (or any application which needs to dynamically
// TODO: ** adjust the panes) unless you tweek some private data to overwrite the
// TODO: ** desired and minimum pane heights which were set at creation time.  These
// TODO: ** will probably break in a future release of Motif because of dependence on
// TODO: ** private data.
// TODO: */
// TODO: static void setPaneDesiredHeight(Fl_Widget* w, int height)
// TODO: {
// TODO:    ((XmPanedWindowConstraintPtr)w->core.constraints)->panedw.dheight = height;
// TODO: }
// TODO: static void setPaneMinHeight(Fl_Widget* w, int min)
// TODO: {
// TODO:    ((XmPanedWindowConstraintPtr)w->core.constraints)->panedw.min = min;
// TODO: }

/*
** Update the window manager's size hints.  These tell it the increments in
** which it is allowed to resize the window.  While this isn't particularly
** important for NEdit (since it can tolerate any window size), setting these
** hints also makes the resize indicator show the window size in characters
** rather than pixels, which is very helpful to users.
*/
void UpdateWMSizeHints(WindowInfo* window)
{
   int shellWidth, shellHeight, textHeight, hScrollBarHeight;
// TODO:    int marginHeight, marginWidth, totalHeight, nCols, nRows;
// TODO:    XFontStruct* fs;
// TODO:    int i, baseWidth, baseHeight, fontHeight, fontWidth;
// TODO:    Fl_Widget* hScrollBar;
// TODO:    textDisp* textD = ((TextWidget)window->textArea)->text.textD;
// TODO: 
// TODO:    /* Find the dimensions of a single character of the text font */
// TODO:    XtVaGetValues(window->textArea, textNfont, &fs, NULL);
// TODO:    fontHeight = textD->ascent + textD->descent;
// TODO:    fontWidth = fs->max_bounds.width;
// TODO: 
// TODO:    /* Find the base (non-expandable) width and height of the editor window.
// TODO: 
// TODO:       FIXME:
// TODO:       To workaround the shrinking-window bug on some WM such as Metacity,
// TODO:       which caused the window to shrink as we switch between documents
// TODO:       using different font sizes on the documents in the same window, the
// TODO:       base width, and similarly the base height, is ajusted such that:
// TODO:            shellWidth = baseWidth + cols * textWidth
// TODO:       There are two issues with this workaround:
// TODO:       1. the right most characters may appear partially obsure
// TODO:       2. the Col x Row info reported by the WM will be based on the fully
// TODO:          display text.
// TODO:    */
// TODO:    XtVaGetValues(window->textArea, XmNheight, &textHeight,
// TODO:                  textNmarginHeight, &marginHeight, textNmarginWidth, &marginWidth,
// TODO:                  NULL);
// TODO:    totalHeight = textHeight - 2*marginHeight;
// TODO:    for (i=0; i<window->nPanes; i++)
// TODO:    {
// TODO:       XtVaGetValues(window->textPanes[i], XmNheight, &textHeight,
// TODO:                     textNhScrollBar, &hScrollBar, NULL);
// TODO:       totalHeight += textHeight - 2*marginHeight;
// TODO:       if (!XtIsManaged(hScrollBar))
// TODO:       {
// TODO:          XtVaGetValues(hScrollBar, XmNheight, &hScrollBarHeight, NULL);
// TODO:          totalHeight -= hScrollBarHeight;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    XtVaGetValues(window->mainWindow, XmNwidth, &shellWidth,
// TODO:                  XmNheight, &shellHeight, NULL);
// TODO:    nCols = textD->width / fontWidth;
// TODO:    nRows = totalHeight / fontHeight;
// TODO:    baseWidth = shellWidth - nCols * fontWidth;
// TODO:    baseHeight = shellHeight - nRows * fontHeight;
// TODO: 
// TODO:    /* Set the size hints in the shell widget */
// TODO:    XtVaSetValues(window->mainWindow, XmNwidthInc, fs->max_bounds.width,
// TODO:                  XmNheightInc, fontHeight,
// TODO:                  XmNbaseWidth, baseWidth, XmNbaseHeight, baseHeight,
// TODO:                  XmNminWidth, baseWidth + fontWidth,
// TODO:                  XmNminHeight, baseHeight + (1+window->nPanes) * fontHeight, NULL);
// TODO: 
// TODO:    /* Motif will keep placing this on the shell every time we change it,
// TODO:       so it needs to be undone every single time.  This only seems to
// TODO:       happen on mult-head dispalys on screens 1 and higher. */

   RemovePPositionHint(window->mainWindow);
}

/*
** Update the minimum allowable height for a split pane after a change
** to font or margin height.
*/
void UpdateMinPaneHeights(WindowInfo* window)
{
// TODO:    textDisp* textD = ((TextWidget)window->textArea)->text.textD;
// TODO:    int hsbHeight, swMarginHeight,frameShadowHeight;
// TODO:    int i, marginHeight, minPaneHeight;
// TODO:    Fl_Widget* hScrollBar;
// TODO: 
// TODO:    /* find the minimum allowable size for a pane */
// TODO:    XtVaGetValues(window->textArea, textNhScrollBar, &hScrollBar, NULL);
// TODO:    XtVaGetValues(containingPane(window->textArea),
// TODO:                  XmNscrolledWindowMarginHeight, &swMarginHeight, NULL);
// TODO:    XtVaGetValues(XtParent(window->textArea),
// TODO:                  XmNshadowThickness, &frameShadowHeight, NULL);
// TODO:    XtVaGetValues(window->textArea, textNmarginHeight, &marginHeight, NULL);
// TODO:    XtVaGetValues(hScrollBar, XmNheight, &hsbHeight, NULL);
// TODO:    minPaneHeight = textD->ascent + textD->descent + marginHeight*2 +
// TODO:                    swMarginHeight*2 + hsbHeight + 2*frameShadowHeight;
// TODO: 
// TODO:    /* Set it in all of the widgets in the window */
// TODO:    setPaneMinHeight(containingPane(window->textArea), minPaneHeight);
// TODO:    for (i=0; i<window->nPanes; i++)
// TODO:       setPaneMinHeight(containingPane(window->textPanes[i]),
// TODO:                        minPaneHeight);
}

// TODO: /* Add an icon to an applicaction shell widget.  addWindowIcon adds a large
// TODO: ** (primary window) icon, AddSmallIcon adds a small (secondary window) icon.
// TODO: **
// TODO: ** Note: I would prefer that these were not hardwired, but yhere is something
// TODO: ** weird about the  XmNiconPixmap resource that prevents it from being set
// TODO: ** from the defaults in the application resource database.
// TODO: */
// TODO: static void addWindowIcon(Fl_Widget* shell)
// TODO: {
// TODO:    static Pixmap iconPixmap = 0, maskPixmap = 0;
// TODO: 
// TODO:    if (iconPixmap == 0)
// TODO:    {
// TODO:       iconPixmap = XCreateBitmapFromData(TheDisplay,
// TODO:                                          RootWindowOfScreen(XtScreen(shell)), (char*)iconBits,
// TODO:                                          iconBitmapWidth, iconBitmapHeight);
// TODO:       maskPixmap = XCreateBitmapFromData(TheDisplay,
// TODO:                                          RootWindowOfScreen(XtScreen(shell)), (char*)maskBits,
// TODO:                                          iconBitmapWidth, iconBitmapHeight);
// TODO:    }
// TODO:    XtVaSetValues(shell, XmNiconPixmap, iconPixmap, XmNiconMask, maskPixmap,
// TODO:                  NULL);
// TODO: }
// TODO: void AddSmallIcon(Fl_Widget* shell)
// TODO: {
// TODO:    static Pixmap iconPixmap = 0, maskPixmap = 0;
// TODO: 
// TODO:    if (iconPixmap == 0)
// TODO:    {
// TODO:       iconPixmap = XCreateBitmapFromData(TheDisplay,
// TODO:                                          RootWindowOfScreen(XtScreen(shell)), (char*)n_bits,
// TODO:                                          n_width, n_height);
// TODO:       maskPixmap = XCreateBitmapFromData(TheDisplay,
// TODO:                                          RootWindowOfScreen(XtScreen(shell)), (char*)n_mask,
// TODO:                                          n_width, n_height);
// TODO:    }
// TODO:    XtVaSetValues(shell, XmNiconPixmap, iconPixmap,
// TODO:                  XmNiconMask, maskPixmap, NULL);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Create pixmap per the widget's color depth setting.
// TODO: **
// TODO: ** This fixes a BadMatch (X_CopyArea) error due to mismatching of
// TODO: ** color depth between the bitmap (depth of 1) and the screen,
// TODO: ** specifically on when linked to LessTif v1.2 (release 0.93.18
// TODO: ** & 0.93.94 tested).  LessTif v2.x showed no such problem.
// TODO: */
// TODO: static Pixmap createBitmapWithDepth(Fl_Widget* w, char* data, unsigned int width,
// TODO:                                     unsigned int height)
// TODO: {
// TODO:    Pixmap pixmap;
// TODO:    Fl_Color fg, bg;
// TODO:    int depth;
// TODO: 
// TODO:    XtVaGetValues(w, XmNforeground, &fg, XmNbackground, &bg,
// TODO:                  XmNdepth, &depth, NULL);
// TODO:    pixmap = XCreatePixmapFromBitmapData(XtDisplay(w),
// TODO:                                         RootWindowOfScreen(XtScreen(w)), (char*)data,
// TODO:                                         width, height, fg, bg, depth);
// TODO: 
// TODO:    return pixmap;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Save the position and size of a window as an X standard geometry string.
// TODO: ** A string of at least MAX_GEOMETRY_STRING_LEN characters should be
// TODO: ** provided in the argument "geomString" to receive the result.
// TODO: */
// TODO: static void getGeometryString(WindowInfo* window, char* geomString)
// TODO: {
// TODO:    int x, y, fontWidth, fontHeight, baseWidth, baseHeight;
// TODO:    unsigned int width, height, dummyW, dummyH, bw, depth, nChild;
// TODO:    Window parent, root, *child, w = XtWindow(window->mainWindow);
// TODO:    Display* dpy = XtDisplay(window->mainWindow);
// TODO: 
// TODO:    /* Find the width and height from the window of the shell */
// TODO:    XGetGeometry(dpy, w, &root, &x, &y, &width, &height, &bw, &depth);
// TODO: 
// TODO:    /* Find the top left corner (x and y) of the window decorations.  (This
// TODO:       is what's required in the geometry string to restore the window to it's
// TODO:       original position, since the window manager re-parents the window to
// TODO:       add it's title bar and menus, and moves the requested window down and
// TODO:       to the left.)  The position is found by traversing the window hier-
// TODO:       archy back to the window to the last parent before the root window */
// TODO:    for (;;)
// TODO:    {
// TODO:       XQueryTree(dpy, w, &root, &parent,  &child, &nChild);
// TODO:       XFree((char*)child);
// TODO:       if (parent == root)
// TODO:          break;
// TODO:       w = parent;
// TODO:    }
// TODO:    XGetGeometry(dpy, w, &root, &x, &y, &dummyW, &dummyH, &bw, &depth);
// TODO: 
// TODO:    /* Use window manager size hints (set by UpdateWMSizeHints) to
// TODO:       translate the width and height into characters, as opposed to pixels */
// TODO:    XtVaGetValues(window->mainWindow, XmNwidthInc, &fontWidth,
// TODO:                  XmNheightInc, &fontHeight, XmNbaseWidth, &baseWidth,
// TODO:                  XmNbaseHeight, &baseHeight, NULL);
// TODO:    width = (width-baseWidth) / fontWidth;
// TODO:    height = (height-baseHeight) / fontHeight;
// TODO: 
// TODO:    /* Write the string */
// TODO:    CreateGeometryString(geomString, x, y, width, height,
// TODO:                         XValue | YValue | WidthValue | HeightValue);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Xt timer procedure for updating size hints.  The new sizes of objects in
// TODO: ** the window are not ready immediately after adding or removing panes.  This
// TODO: ** is a timer routine to be invoked with a timeout of 0 to give the event
// TODO: ** loop a chance to finish processing the size changes before reading them
// TODO: ** out for setting the window manager size hints.
// TODO: */
// TODO: static void wmSizeUpdateProc(XtPointer clientData, int* id)
// TODO: {
// TODO:    UpdateWMSizeHints((WindowInfo*)clientData);
// TODO: }
// TODO: 
// TODO: #ifdef ROWCOLPATCH
// TODO: /*
// TODO: ** There is a bad memory reference in the delete_child method of the
// TODO: ** RowColumn widget in some Motif versions (so far, just Solaris with Motif
// TODO: ** 1.2.3) which appears durring the phase 2 destroy of the widget. This
// TODO: ** patch replaces the method with a call to the Composite widget's
// TODO: ** delete_child method.  The composite delete_child method handles part,
// TODO: ** but not all of what would have been done by the original method, meaning
// TODO: ** that this is dangerous and should be used sparingly.  Note that
// TODO: ** patchRowCol is called only in CloseWindow, before the widget is about to
// TODO: ** be destroyed, and only on systems where the bug has been observed
// TODO: */
// TODO: static void patchRowCol(Fl_Widget* w)
// TODO: {
// TODO:    ((XmRowColumnClassRec*)XtClass(w))->composite_class.delete_child =
// TODO:       patchedRemoveChild;
// TODO: }
// TODO: static void patchedRemoveChild(Fl_Widget* child)
// TODO: {
// TODO:    /* Call composite class method instead of broken row col delete_child
// TODO:       method */
// TODO:    (*((CompositeWidgetClass)compositeWidgetClass)->composite_class.
// TODO:     delete_child)(child);
// TODO: }
// TODO: #endif /* ROWCOLPATCH */
// TODO: 
// TODO: /*
// TODO: ** Set the backlight character class string
// TODO: */
// TODO: void SetBacklightChars(WindowInfo* window, char* applyBacklightTypes)
// TODO: {
// TODO:    int i;
// TODO:    int is_applied = NeToggleButtonGetState(window->backlightCharsItem) ? 1 : 0;
// TODO:    int do_apply = applyBacklightTypes ? 1 : 0;
// TODO: 
// TODO:    window->backlightChars = do_apply;
// TODO: 
// TODO:    free__(window->backlightCharTypes);
// TODO:    if (window->backlightChars &&
// TODO:          (window->backlightCharTypes = malloc__(strlen(applyBacklightTypes)+1)))
// TODO:       strcpy(window->backlightCharTypes, applyBacklightTypes);
// TODO:    else
// TODO:       window->backlightCharTypes = NULL;
// TODO: 
// TODO:    XtVaSetValues(window->textArea,
// TODO:                  textNbacklightCharTypes, window->backlightCharTypes, NULL);
// TODO:    for (i=0; i<window->nPanes; i++)
// TODO:       XtVaSetValues(window->textPanes[i],
// TODO:                     textNbacklightCharTypes, window->backlightCharTypes, NULL);
// TODO:    if (is_applied != do_apply)
// TODO:       SetToggleButtonState(window, window->backlightCharsItem, do_apply, false);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** perform generic management on the children (toolbars) of toolBarsForm,
// TODO: ** a.k.a. statsForm, by setting the form attachment of the managed child
// TODO: ** widgets per their position/order.
// TODO: **
// TODO: ** You can optionally create separator after a toolbar widget with it's
// TODO: ** widget name set to "TOOLBAR_SEP", which will appear below the toolbar
// TODO: ** widget. These seperators will then be managed automatically by this
// TODO: ** routine along with the toolbars they 'attached' to.
// TODO: **
// TODO: ** It also takes care of the attachment offset settings of the child
// TODO: ** widgets to keep the border lines of the parent form displayed, so
// TODO: ** you don't have set them before hand.
// TODO: **
// TODO: ** Note: XtManage/XtUnmange the target child (toolbar) before calling this
// TODO: **       function.
// TODO: **
// TODO: ** Returns the last toolbar widget managed.
// TODO: **
// TODO: */
// TODO: static Fl_Widget* manageToolBars(Fl_Widget* toolBarsForm)
// TODO: {
// TODO:    Fl_Widget* topWidget = NULL;
// TODO:    WidgetList children;
// TODO:    int n, nItems=0;
// TODO: 
// TODO:    XtVaGetValues(toolBarsForm, XmNchildren, &children,
// TODO:                  XmNnumChildren, &nItems, NULL);
// TODO: 
// TODO:    for (n=0; n<nItems; n++)
// TODO:    {
// TODO:       Fl_Widget* tbar = children[n];
// TODO: 
// TODO:       if (XtIsManaged(tbar))
// TODO:       {
// TODO:          if (topWidget)
// TODO:          {
// TODO:             XtVaSetValues(tbar, XmNtopAttachment, XmATTACH_WIDGET,
// TODO:                           XmNtopWidget, topWidget,
// TODO:                           XmNbottomAttachment, XmATTACH_NONE,
// TODO:                           XmNleftOffset, STAT_SHADOW_THICKNESS,
// TODO:                           XmNrightOffset, STAT_SHADOW_THICKNESS,
// TODO:                           NULL);
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             /* the very first toolbar on top */
// TODO:             XtVaSetValues(tbar, XmNtopAttachment, XmATTACH_FORM,
// TODO:                           XmNbottomAttachment, XmATTACH_NONE,
// TODO:                           XmNleftOffset, STAT_SHADOW_THICKNESS,
// TODO:                           XmNtopOffset, STAT_SHADOW_THICKNESS,
// TODO:                           XmNrightOffset, STAT_SHADOW_THICKNESS,
// TODO:                           NULL);
// TODO:          }
// TODO: 
// TODO:          topWidget = tbar;
// TODO: 
// TODO:          /* if the next widget is a separator, turn it on */
// TODO:          if (n+1<nItems && !strcmp(XtName(children[n+1]), "TOOLBAR_SEP"))
// TODO:          {
// TODO:             XtManageChild(children[n+1]);
// TODO:          }
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          /* Remove top attachment to widget to avoid circular dependency.
// TODO:             Attach bottom to form so that when the widget is redisplayed
// TODO:             later, it will trigger the parent form to resize properly as
// TODO:             if the widget is being inserted */
// TODO:          XtVaSetValues(tbar, XmNtopAttachment, XmATTACH_NONE,
// TODO:                        XmNbottomAttachment, XmATTACH_FORM, NULL);
// TODO: 
// TODO:          /* if the next widget is a separator, turn it off */
// TODO:          if (n+1<nItems && !strcmp(XtName(children[n+1]), "TOOLBAR_SEP"))
// TODO:          {
// TODO:             XtUnmanageChild(children[n+1]);
// TODO:          }
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    if (topWidget)
// TODO:    {
// TODO:       if (strcmp(XtName(topWidget), "TOOLBAR_SEP"))
// TODO:       {
// TODO:          XtVaSetValues(topWidget,
// TODO:                        XmNbottomAttachment, XmATTACH_FORM,
// TODO:                        XmNbottomOffset, STAT_SHADOW_THICKNESS,
// TODO:                        NULL);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          /* is a separator */
// TODO:          Fl_Widget* wgt;
// TODO:          XtVaGetValues(topWidget, XmNtopWidget, &wgt, NULL);
// TODO: 
// TODO:          /* don't need sep below bottom-most toolbar */
// TODO:          XtUnmanageChild(topWidget);
// TODO:          XtVaSetValues(wgt,
// TODO:                        XmNbottomAttachment, XmATTACH_FORM,
// TODO:                        XmNbottomOffset, STAT_SHADOW_THICKNESS,
// TODO:                        NULL);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    return topWidget;
// TODO: }

/*
** Calculate the dimension of the text area, in terms of rows & cols,
** as if there's only one single text pane in the window.
*/
static void getTextPaneDimension(WindowInfo* window, int* nRows, int* nCols)
{
   Fl_Widget* hScrollBar;
   int hScrollBarHeight, paneHeight;
   int marginHeight, marginWidth, totalHeight, fontHeight;
   Ne_Text_Display* textD = window->textArea;

   // width is the same for panes
   *nCols = window->textArea->columns();

   *nRows = window->textArea->rows(); // simple...
   /* we have to work out the height, as the text area may have been split */
// TODO:    XtVaGetValues(window->textArea, textNhScrollBar, &hScrollBar,
// TODO:                  textNmarginHeight, &marginHeight, textNmarginWidth, &marginWidth,
// TODO:                  NULL);
// TODO:    XtVaGetValues(hScrollBar, XmNheight, &hScrollBarHeight, NULL);
// TODO:    XtVaGetValues(window->splitPane, XmNheight, &paneHeight, NULL);
// TODO:    totalHeight = paneHeight - 2*marginHeight -hScrollBarHeight;
// TODO:    fontHeight = textD->ascent + textD->descent;
// TODO:    *nRows = totalHeight/fontHeight;
}

/*
** Create a new document in the shell window.
** Document are created in 'background' so that the user
** menus, ie. the Macro/Shell/BG menus, will not be updated
** unnecessarily; hence speeding up the process of opening
** multiple files.
*/
WindowInfo* CreateDocument(WindowInfo* shellWindow, const char* name)
{
   WindowInfo* window;

   // Allocate some memory for the new window data structure
   // and inherit settings and later reset those required
   window = new WindowInfo(*shellWindow);
// TODO:    memcpy(window, shellWindow, sizeof(WindowInfo));

   window->type = WindowInfo::Document;

#if 0
   // share these dialog items with parent shell
   window->replaceDlog = NULL;
   window->replaceText = NULL;
   window->replaceWithText = NULL;
   window->replaceWordToggle = NULL;
   window->replaceCaseToggle = NULL;
   window->replaceRegexToggle = NULL;
   window->findDlog = NULL;
   window->findText = NULL;
   window->findWordToggle = NULL;
   window->findCaseToggle = NULL;
   window->findRegexToggle = NULL;
   window->replaceMultiFileDlog = NULL;
   window->replaceMultiFilePathBtn = NULL;
   window->replaceMultiFileList = NULL;
   window->showLineNumbers = GetPrefLineNums();
   window->showStats = GetPrefStatsLine();
   window->showISearchLine = GetPrefISearchLine();
#endif

   window->multiFileReplSelected = false;
   window->multiFileBusy = false;
   window->writableWindows = NULL;
   window->nWritableWindows = 0;
   window->fileChanged = false;
   window->fileMissing = true;
   window->fileMode = 0;
   window->fileUid = 0;
   window->fileGid = 0;
   window->filenameSet = false;
   window->fileFormat = UNIX_FILE_FORMAT;
   window->lastModTime = 0;
   strcpy(window->filename, name);
   window->undo = NULL;
   window->redo = NULL;
   window->nPanes = 0;
   window->autoSaveCharCount = 0;
   window->autoSaveOpCount = 0;
   window->undoOpCount = 0;
   window->undoMemUsed = 0;
   CLEAR_ALL_LOCKS(window->lockReasons);
   window->indentStyle = GetPrefAutoIndent(PLAIN_LANGUAGE_MODE);
   window->autoSave = GetPrefAutoSave();
   window->saveOldVersion = GetPrefSaveOldVersion();
   window->wrapMode = GetPrefWrap(PLAIN_LANGUAGE_MODE);
   window->overstrike = false;
   window->showMatchingStyle = GetPrefShowMatching();
   window->matchSyntaxBased = GetPrefMatchSyntaxBased();
   window->highlightSyntax = GetPrefHighlightSyntax();
   window->backlightCharTypes = NULL;
   window->backlightChars = GetPrefBacklightChars();
   if (window->backlightChars)
   {
      char* cTypes = GetPrefBacklightCharTypes();
      if (cTypes && window->backlightChars)
      {
         if ((window->backlightCharTypes = (char*)malloc__(strlen(cTypes) + 1)))
            strcpy(window->backlightCharTypes, cTypes);
      }
   }
   window->modeMessageDisplayed = false;
   window->modeMessage = NULL;
   window->ignoreModify = false;
   window->windowMenuValid = false;
   window->flashTimeoutID = 0;
   window->fileClosedAtom = 0; //None;
   window->wasSelected = false;
   strcpy(window->fontName, GetPrefFontName());
   strcpy(window->italicFontName, GetPrefItalicFontName());
   strcpy(window->boldFontName, GetPrefBoldFontName());
   strcpy(window->boldItalicFontName, GetPrefBoldItalicFontName());
   window->colorDialog = NULL;
   window->fontList = GetPrefFontList();
   window->italicFontStruct = GetPrefItalicFont();
   window->boldFontStruct = GetPrefBoldFont();
   window->boldItalicFontStruct = GetPrefBoldItalicFont();
   window->fontDialog = NULL;
   window->nMarks = 0;
   window->markTimeoutID = 0;
   window->highlightData = NULL;
   window->shellCmdData = NULL;
   window->macroCmdData = NULL;
   window->smartIndentData = NULL;
   window->languageMode = PLAIN_LANGUAGE_MODE;
   window->iSearchHistIndex = 0;
   window->iSearchStartPos = -1;
   window->replaceLastRegexCase   = true;
   window->replaceLastLiteralCase = false;
   window->iSearchLastRegexCase   = true;
   window->iSearchLastLiteralCase = false;
   window->findLastRegexCase      = true;
   window->findLastLiteralCase    = false;
// TODO:    window->tab = NULL;
   window->bgMenuUndoItem = NULL;
   window->bgMenuRedoItem = NULL;
   window->device = 0;
   window->inode = 0;

   window->bgMenuUndoItem = 0;
   window->bgMenuRedoItem = 0;


// TODO:    if (window->fontList == NULL)
// TODO:       XtVaGetValues(shellWindow->statsLine, XmNfontList,
// TODO:                     &window->fontList,NULL);
// TODO: 

   int nRows, nCols;
   getTextPaneDimension(shellWindow, &nRows, &nCols);

// TODO:    /* Create pane that actaully holds the new document. As
// TODO:       document is created in 'background', we need to hide
// TODO:       it. If we leave it unmanaged without setting it to
// TODO:       the XmNworkWindow of the mainWin, due to a unknown
// TODO:       bug in Motif where splitpane's scrollWindow child
// TODO:       somehow came up with a height taller than the splitpane,
// TODO:       the bottom part of the text editing widget is obstructed
// TODO:       when later brought up by  RaiseDocument(). So we first
// TODO:       manage it hidden, then unmanage it and reset XmNworkWindow,
// TODO:       then let RaiseDocument() show it later. */
// TODO:    pane = XtVaCreateWidget("pane",
// TODO:                            xmPanedWindowWidgetClass, window->mainWin,
// TODO:                            XmNmarginWidth, 0, XmNmarginHeight, 0, XmNseparatorOn, false,
// TODO:                            XmNspacing, 3, XmNsashIndent, -2,
// TODO:                            XmNmappedWhenManaged, false,
// TODO:                            NULL);
// TODO:    XtVaSetValues(window->mainWin, XmNworkWindow, pane, NULL);
// TODO:    XtManageChild(pane);
// TODO:    window->splitPane = pane;
// TODO: 
// TODO:    /* Store a copy of document/window pointer in text pane to support
// TODO:       action procedures. See also WidgetToWindow() for info. */
// TODO:    XtVaSetValues(pane, XmNuserData, window, NULL);

   // Create a new text area
   window->textArea = createTextArea(window, nRows, nCols,
      GetPrefEmTabDist(PLAIN_LANGUAGE_MODE), GetPrefDelimiters(),
      GetPrefWrapMargin(), window->showLineNumbers?MIN_LINE_NUM_COLS:0);
   window->lastFocus = window->textArea;

   /* Set the initial colors from the globals. */
   SetColors(window,
             GetPrefColorName(TEXT_FG_COLOR),
             GetPrefColorName(TEXT_BG_COLOR),
             GetPrefColorName(SELECT_FG_COLOR),
             GetPrefColorName(SELECT_BG_COLOR),
             GetPrefColorName(HILITE_FG_COLOR),
             GetPrefColorName(HILITE_BG_COLOR),
             GetPrefColorName(LINENO_FG_COLOR),
             GetPrefColorName(CURSOR_FG_COLOR));

// TODO:    /* Create the right button popup menu (note: order is important here,
// TODO:       since the translation for popping up this menu was probably already
// TODO:       added in createTextArea, but CreateBGMenu requires window->textArea
// TODO:       to be set so it can attach the menu to it (because menu shells are
// TODO:       finicky about the kinds of widgets they are attached to)) */
// TODO:    window->bgMenuPane = CreateBGMenu(window);

   /* cache user menus: init. user background menu cache */
   InitUserBGMenuCache(&window->userBGMenuCache);

   /* Create the text buffer rather than using the one created automatically
      with the text area widget.  This is done so the syntax highlighting
      modify callback can be called to synchronize the style buffer BEFORE
      the text display's callback is called upon to display a modification */
   window->buffer = BufCreate();
   BufAddModifyCB(window->buffer, SyntaxHighlightModifyCB, window);

   /* Attach the buffer to the text widget, and add callbacks for modify */
   TextSetBuffer(window->textArea, window->buffer);
   BufAddModifyCB(window->buffer, modifiedCB, window);

// TODO:    /* Designate the permanent text area as the owner for selections */
// TODO:    HandleXSelections(text);

   /* Set the requested hardware tab distance and useTabs in the text buffer */
   BufSetTabDistance(window->buffer, GetPrefTabDist(PLAIN_LANGUAGE_MODE));
   window->buffer->useTabs = GetPrefInsertTabs();
   window->textArea->label(window->filename);
   window->tab->add(window->textArea);
   window->tab->value(window->textArea);

   // add the window to the global window list, update the Windows menus
   addToWindowList(window);
   InvalidateWindowMenus();

// TODO:    /* return the shell ownership to previous tabbed doc */
// TODO:    XtVaSetValues(window->mainWin, XmNworkWindow, shellWindow->splitPane, NULL);
// TODO:    XLowerWindow(TheDisplay, XtWindow(window->splitPane));
// TODO:    XtUnmanageChild(window->splitPane);
// TODO:    XtVaSetValues(window->splitPane, XmNmappedWhenManaged, true, NULL);

   return window;
}

/*
** return the next/previous docment on the tab list.
**
** If <wrap> is true then the next tab of the rightmost tab will be the
** second tab from the right, and the the previous tab of the leftmost
** tab will be the second from the left.  This is useful for getting
** the next tab after a tab detaches/closes and you don't want to wrap around.
*/
static WindowInfo* getNextTabWindow(WindowInfo* window, int direction, int crossWin, int wrap)
{
//    WidgetList tabList, tabs;
   WindowInfo* win;

   int tabCount, tabTotalCount;
   int tabPos, nextPos;
   int i, n;
   int nBuf = crossWin? NWindows() : NDocuments(window);

   if (nBuf <= 1)
      return NULL;

   // get the list of tabs
   Fl_Widget** tabs = new Fl_Widget*[nBuf];
   tabTotalCount = 0;
   if (crossWin)
   {
// TODO:       int n, nItems;
// TODO:       /* get list of tabs in all windows */
// TODO:       for (n=0; n<nItems; n++)
// TODO:       {
// TODO:          if (strcmp(XtName(children[n]), "textShell") || ((win = WidgetToWindow(children[n])) == NULL))
// TODO:             continue;   /* skip non-text-editor windows */
// TODO: 
// TODO:          XtVaGetValues(win->tabBar, XmNtabWidgetList, &tabList, XmNtabCount, &tabCount, NULL);
// TODO: 
// TODO:          for (i=0; i< tabCount; i++)
// TODO:          {
// TODO:             tabs[tabTotalCount++] = tabList[i];
// TODO:          }
// TODO:       }
      throw "Not yet...";
   }
   else
   {
      /* get list of tabs in this window */
      tabCount = window->tab->children();

      for (int i=0; i< tabCount; i++)
      {
// TODO:          if (TabToWindow(window->tab->child(i)))    /* make sure tab is valid */
            tabs[tabTotalCount++] = window->tab->child(i);
      }
   }

   /* find the position of the tab in the tablist */
   tabPos = 0;
   for (int n=0; n<tabTotalCount; ++n)
   {
      if (tabs[n] == window->tab->child(n))
      {
         tabPos = n;
         break;
      }
   }

   // calculate index position of next tab
   nextPos = tabPos + direction;
   if (nextPos >= nBuf)
   {
      if (wrap)
         nextPos = 0;
      else
         nextPos = nBuf - 2;
   }
   else if (nextPos < 0)
   {
      if (wrap)
         nextPos = nBuf - 1;
      else
         nextPos = 1;
   }

   // return the document where the next tab belongs to
   win = TabToWindow(tabs[nextPos]);
   delete[] tabs;
   return win;
}

// TODO: /*
// TODO: ** return the integer position of a tab in the tabbar it
// TODO: ** belongs to, or -1 if there's an error, somehow.
// TODO: */
// TODO: static int getTabPosition(Fl_Widget* tab)
// TODO: {
// TODO:    WidgetList tabList;
// TODO:    int i, tabCount;
// TODO:    Fl_Widget* tabBar = XtParent(tab);
// TODO: 
// TODO:    XtVaGetValues(tabBar, XmNtabWidgetList, &tabList,
// TODO:                  XmNtabCount, &tabCount, NULL);
// TODO: 
// TODO:    for (i=0; i< tabCount; i++)
// TODO:    {
// TODO:       if (tab == tabList[i])
// TODO:          return i;
// TODO:    }
// TODO: 
// TODO:    return -1; /* something is wrong! */
// TODO: }

/*
** update the tab label, etc. of a tab, per the states of it's document.
*/
void RefreshTabState(WindowInfo* win)
{
   char labelString[MAXPATHLEN];
// TODO:    unsigned char alignment;

   /* Set tab label to document's filename. Position of "*" (modified) will change per label alignment setting */
// TODO:    XtVaGetValues(win->tab, XmNalignment, &alignment, NULL);
// TODO:    if (alignment != XmALIGNMENT_END)
// TODO:    {
// TODO:       sprintf(labelString, "%s%s", win->fileChanged? "*" : "", win->filename);
// TODO:    }
// TODO:    else
   {
      sprintf(labelString, "%s%s", win->filename, win->fileChanged? "*" : "");
   }

   // Make the top document stand out a little more
   if (IsTopDocument(win))
      ; // TODO: tag = "BOLD";

   //if (GetPrefShowPathInWindowsMenu() && win->filenameSet)
   //{
   //   strcat(labelString, " - ");
   //   strcat(labelString, win->path);
   //}

   win->tab->value()->copy_label(labelString);
   win->tab->redraw();
}

/*
** close all the documents in a window
*/
int CloseAllDocumentInWindow(WindowInfo* window)
{
   WindowInfo* win;

   if (NDocuments(window) == 1)
   {
      /* only one document in the window */
      return CloseFileAndWindow(window, PROMPT_SBC_DIALOG_RESPONSE);
   }
   else
   {
      Fl_Widget* winShell = window->mainWindow;
      WindowInfo* topDocument;

      /* close all _modified_ documents belong to this window */
      for (win = WindowList; win;)
      {
         if (win->mainWindow == winShell && win->fileChanged)
         {
            WindowInfo* next = win->next;
            if (!CloseFileAndWindow(win, PROMPT_SBC_DIALOG_RESPONSE))
               return false;
            win = next;
         }
         else
            win = win->next;
      }

      /* see there's still documents left in the window */
      for (win = WindowList; win; win=win->next)
         if (win->mainWindow == winShell)
            break;

      if (win)
      {
         topDocument = GetTopDocument(winShell);

         /* close all non-top documents belong to this window */
         for (win = WindowList; win;)
         {
            if (win->mainWindow == winShell && win != topDocument)
            {
               WindowInfo* next = win->next;
               if (!CloseFileAndWindow(win, PROMPT_SBC_DIALOG_RESPONSE))
                  return false;
               win = next;
            }
            else
               win = win->next;
         }

         /* close the last document and its window */
         if (!CloseFileAndWindow(topDocument, PROMPT_SBC_DIALOG_RESPONSE))
            return false;
      }
   }

   return true;
}

// TODO: static void CloseDocumentWindow(Fl_Widget* w, WindowInfo* window, XtPointer callData)
// TODO: {
// TODO:    int nDocuments = NDocuments(window);
// TODO: 
// TODO:    if (nDocuments == NWindows())
// TODO:    {
// TODO:       /* this is only window, then exit */
// TODO:       XtCallActionProc(WindowList->lastFocus, "exit",
// TODO:                        ((XmAnyCallbackStruct*)callData)->event, NULL, 0);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if (nDocuments == 1)
// TODO:       {
// TODO:          CloseFileAndWindow(window, PROMPT_SBC_DIALOG_RESPONSE);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          int resp = 1;
// TODO:          if (GetPrefWarnExit())
// TODO:             resp = DialogF(DF_QUES, window->mainWindow, 2, "Close Window",
// TODO:                            "Close ALL documents in this window?", "Close", "Cancel");
// TODO: 
// TODO:          if (resp == 1)
// TODO:             CloseAllDocumentInWindow(window);
// TODO:       }
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Refresh the menu entries per the settings of the
// TODO: ** top document.
// TODO: */
// TODO: void RefreshMenuToggleStates(WindowInfo* window)
// TODO: {
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    if (!IsTopDocument(window))
// TODO:       return;
// TODO: 
// TODO:    /* File menu */
// TODO:    NeSetSensitive(window->menuBar->getPrintSelectionItem(), window->wasSelected);
// TODO: 
// TODO:    /* Edit menu */
// TODO:    NeSetSensitive(window->undoItem, window->undo != NULL);
// TODO:    NeSetSensitive(window->redoItem, window->redo != NULL);
// TODO:    NeSetSensitive(window->menuBar->getPrintSelectionItem(), window->wasSelected);
// TODO:    NeSetSensitive(window->cutItem, window->wasSelected);
// TODO:    NeSetSensitive(window->copyItem, window->wasSelected);
// TODO:    NeSetSensitive(window->delItem, window->wasSelected);
// TODO: 
// TODO:    /* Preferences menu */
// TODO:    NeToggleButtonSetState(window->statsLineItem, window->showStats, false);
// TODO:    NeToggleButtonSetState(window->iSearchLineItem, window->showISearchLine, false);
// TODO:    NeToggleButtonSetState(window->lineNumsItem, window->showLineNumbers, false);
// TODO:    NeToggleButtonSetState(window->highlightItem, window->highlightSyntax, false);
// TODO:    NeSetSensitive(window->highlightItem, window->languageMode != PLAIN_LANGUAGE_MODE);
// TODO:    NeToggleButtonSetState(window->backlightCharsItem, window->backlightChars, false);
// TODO: #ifndef VMS
// TODO:    NeToggleButtonSetState(window->saveLastItem, window->saveOldVersion, false);
// TODO: #endif
// TODO:    NeToggleButtonSetState(window->autoSaveItem, window->autoSave, false);
// TODO:    NeToggleButtonSetState(window->overtypeModeItem, window->overstrike, false);
// TODO:    NeToggleButtonSetState(window->matchSyntaxBasedItem, window->matchSyntaxBased, false);
// TODO:    NeToggleButtonSetState(window->readOnlyItem, IS_USER_LOCKED(window->lockReasons), false);
// TODO: 
// TODO:    NeSetSensitive(window->smartIndentItem,
// TODO:                   SmartIndentMacrosAvailable(LanguageModeName(window->languageMode)));
// TODO: 
// TODO:    SetAutoIndent(window, window->indentStyle);
// TODO:    SetAutoWrap(window, window->wrapMode);
// TODO:    SetShowMatching(window, window->showMatchingStyle);
// TODO:    SetLanguageMode(window, window->languageMode, false);
// TODO: 
// TODO:    /* Windows Menu */
// TODO:    NeSetSensitive(window->splitPaneItem, window->nPanes < MAX_PANES);
// TODO:    NeSetSensitive(window->closePaneItem, window->nPanes > 0);
// TODO:    NeSetSensitive(window->detachDocumentItem, NDocuments(window)>1);
// TODO:    NeSetSensitive(window->contextDetachDocumentItem, NDocuments(window)>1);
// TODO: 
// TODO:    for (win=WindowList; win; win=win->next)
// TODO:       if (win->mainWindow != window->mainWindow)
// TODO:          break;
// TODO:    NeSetSensitive(window->moveDocumentItem, win != NULL);
// TODO: }

/*
** Refresh the various settings/state of the shell window per the
** settings of the top document.
*/
static void refreshMenuBar(WindowInfo* window)
{
// TODO:    RefreshMenuToggleStates(window);
// TODO: 
// TODO:    /* Add/remove language specific menu items */
// TODO:    UpdateUserMenus(window);
// TODO: 
// TODO:    /* refresh selection-sensitive menus */
// TODO:    DimSelectionDepUserMenuItems(window, window->wasSelected);
}

/*
** remember the last document.
*/
WindowInfo* MarkLastDocument(WindowInfo* window)
{
   WindowInfo* prev = lastFocusDocument;

   if (window)
      lastFocusDocument = window;

   return prev;
}

/*
** remember the active (top) document.
*/
WindowInfo* MarkActiveDocument(WindowInfo* window)
{
   WindowInfo* prev = inFocusDocument;

   if (window)
      inFocusDocument = window;

   return prev;
}

// TODO: /*
// TODO: ** Bring up the next window by tab order
// TODO: */
// TODO: void NextDocument(WindowInfo* window)
// TODO: {
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    if (WindowList->next == NULL)
// TODO:       return;
// TODO: 
// TODO:    win = getNextTabWindow(window, 1, GetPrefGlobalTabNavigate(), 1);
// TODO:    if (win == NULL)
// TODO:       return;
// TODO: 
// TODO:    if (window->mainWindow == win->mainWindow)
// TODO:       RaiseDocument(win);
// TODO:    else
// TODO:       RaiseFocusDocumentWindow(win, true);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Bring up the previous window by tab order
// TODO: */
// TODO: void PreviousDocument(WindowInfo* window)
// TODO: {
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    if (WindowList->next == NULL)
// TODO:       return;
// TODO: 
// TODO:    win = getNextTabWindow(window, -1, GetPrefGlobalTabNavigate(), 1);
// TODO:    if (win == NULL)
// TODO:       return;
// TODO: 
// TODO:    if (window->mainWindow == win->mainWindow)
// TODO:       RaiseDocument(win);
// TODO:    else
// TODO:       RaiseFocusDocumentWindow(win, true);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Bring up the last active window
// TODO: */
// TODO: void LastDocument(WindowInfo* window)
// TODO: {
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    for (win = WindowList; win; win=win->next)
// TODO:       if (lastFocusDocument == win)
// TODO:          break;
// TODO: 
// TODO:    if (!win)
// TODO:       return;
// TODO: 
// TODO:    if (window->mainWindow == win->mainWindow)
// TODO:       RaiseDocument(win);
// TODO:    else
// TODO:       RaiseFocusDocumentWindow(win, true);
// TODO: 
// TODO: }

// make sure window is alive is kicking
int IsValidWindow(WindowInfo* window)
{
   WindowInfo* win;

   for (win = WindowList; win; win=win->next)
      if (window == win)
         return true;

   return false;
}

/*
** raise the document and its shell window and focus depending on pref.
*/
void RaiseDocumentWindow(WindowInfo* window)
{
   if (!window)
      return;

   RaiseDocument(window);
   RaiseShellWindow(window->mainWindow, GetPrefFocusOnRaise());
}

/*
** raise the document and its shell window and optionally focus.
*/
void RaiseFocusDocumentWindow(WindowInfo* window, bool focus)
{
   if (!window)
      return;

   RaiseDocument(window);
   RaiseShellWindow(window->mainWindow, focus);
}

// TODO: /*
// TODO: ** Redisplay menu tearoffs previously hid by hideTearOffs()
// TODO: */
// TODO: static void redisplayTearOffs(Fl_Widget* menuPane)
// TODO: {
// TODO:    WidgetList itemList;
// TODO:    Fl_Widget* subMenuID;
// TODO:    Cardinal nItems;
// TODO:    int n;
// TODO: 
// TODO:    /* redisplay all submenu tearoffs */
// TODO:    XtVaGetValues(menuPane, XmNchildren, &itemList,
// TODO:                  XmNnumChildren, &nItems, NULL);
// TODO:    for (n=0; n<(int)nItems; n++)
// TODO:    {
// TODO:       if (XtClass(itemList[n]) == xmCascadeButtonWidgetClass)
// TODO:       {
// TODO:          XtVaGetValues(itemList[n], XmNsubMenuId, &subMenuID, NULL);
// TODO:          redisplayTearOffs(subMenuID);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* redisplay tearoff for this menu */
// TODO:    if (!XmIsMenuShell(XtParent(menuPane)))
// TODO:       ShowHiddenTearOff(menuPane);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** hide all the tearoffs spawned from this menu.
// TODO: ** It works recursively to close the tearoffs of the submenus
// TODO: */
// TODO: static void hideTearOffs(Fl_Widget* menuPane)
// TODO: {
// TODO:    WidgetList itemList;
// TODO:    Fl_Widget* subMenuID;
// TODO:    Cardinal nItems;
// TODO:    int n;
// TODO: 
// TODO:    /* hide all submenu tearoffs */
// TODO:    XtVaGetValues(menuPane, XmNchildren, &itemList,
// TODO:                  XmNnumChildren, &nItems, NULL);
// TODO:    for (n=0; n<(int)nItems; n++)
// TODO:    {
// TODO:       if (XtClass(itemList[n]) == xmCascadeButtonWidgetClass)
// TODO:       {
// TODO:          XtVaGetValues(itemList[n], XmNsubMenuId, &subMenuID, NULL);
// TODO:          hideTearOffs(subMenuID);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* hide tearoff for this menu */
// TODO:    if (!XmIsMenuShell(XtParent(menuPane)))
// TODO:       XtUnmapWidget(XtParent(menuPane));
// TODO: }

/*
** Raise a tabbed document within its shell window.
**
** NB: use RaiseDocumentWindow() to raise the doc and
**     its shell window.
*/
void RaiseDocument(WindowInfo* window)
{
   if (!window || !WindowList)
      return;

   WindowInfo* lastwin = MarkActiveDocument(window);
   if (lastwin != window && IsValidWindow(lastwin))
      MarkLastDocument(lastwin);

   /* document already on top? */
// TODO:    XtVaGetValues(window->mainWin, XmNuserData, &win, NULL);
// TODO:    if (win == window)
// TODO:       return;

   /* set the document as top document */
   window->type = WindowInfo::Window;

// TODO:    /* show the new top document */
// TODO:    XtVaSetValues(window->mainWin, XmNworkWindow, window->splitPane, NULL);
// TODO:    XtManageChild(window->splitPane);
// TODO:    XRaiseWindow(TheDisplay, XtWindow(window->splitPane));
   window->tab->value(window->textArea);
// TODO:    if (window->mainWindow->shown())
      window->mainWindow->show();


   // Turn on syntax highlight that might have been deferred.
   // NB: this must be done after setting the document as
   //     XmNworkWindow and managed, else the parent shell
   //  window may shrink on some window-managers such as
   //  metacity, due to changes made in UpdateWMSizeHints().
   if (window->highlightSyntax && window->highlightData==NULL)
      StartHighlighting(window, false);

// TODO:    /* put away the bg menu tearoffs of last active document */
// TODO:    hideTearOffs(win->bgMenuPane);
// TODO: 
// TODO:    /* restore the bg menu tearoffs of active document */
// TODO:    redisplayTearOffs(window->bgMenuPane);
// TODO: 
// TODO:    /* set tab as active */
// TODO:    XmLFolderSetActiveTab(window->tabBar, getTabPosition(window->tab), false);

   // set keyboard focus. Must be done before unmanaging previous
   // top document, else lastFocus will be reset to textArea
   window->mainWindow->focus(window->lastFocus);

// TODO:    /* we only manage the top document, else the next time a document
// TODO:       is raised again, it's textpane might not resize properly.
// TODO:       Also, somehow (bug?) XtUnmanageChild() doesn't hide the
// TODO:       splitPane, which obscure lower part of the statsform when
// TODO:       we toggle its components, so we need to put the document at
// TODO:       the back */
// TODO:    XLowerWindow(TheDisplay, XtWindow(win->splitPane));
// TODO:    XtUnmanageChild(win->splitPane);
   RefreshTabState(window);

// TODO:    /* now refresh window state/info. RefreshWindowStates()
// TODO:       has a lot of work to do, so we update the screen first so
// TODO:       the document appears to switch swiftly. */
// TODO:    XmUpdateDisplay(window->splitPane);
   RefreshWindowStates(window);
   RefreshTabState(window);

// TODO:    /* put away the bg menu tearoffs of last active document */
// TODO:    hideTearOffs(win->bgMenuPane);
// TODO: 
// TODO:    /* restore the bg menu tearoffs of active document */
// TODO:    redisplayTearOffs(window->bgMenuPane);
// TODO: 
// TODO:    /* Make sure that the "In Selection" button tracks the presence of a
// TODO:       selection and that the window inherits the proper search scope. */
// TODO:    if (window->replaceDlog != NULL && XtIsManaged(window->replaceDlog))
// TODO:    {
// TODO:       UpdateReplaceActionButtons(window);
// TODO:    }

   UpdateWMSizeHints(window);
}

WindowInfo* GetTopDocument(Fl_Widget* w)
{
   WindowInfo* window = WidgetToWindow(w);

   return WidgetToWindow(window->mainWindow);
}

bool IsTopDocument(const WindowInfo* window)
{
// TODO:    return (window->type == WindowInfo::Window); // TODO: window == GetTopDocument(window->mainWindow)? true : false;
   if (!window || !window->tab || ! window->tab->value() )
      return false;
   return window->tab->value() == window->textArea;
}

static void deleteDocument(WindowInfo* window)
{
   if (NULL == window)
   {
      return;
   }

// TODO:    XtDestroyWidget(window->splitPane);
}

/*
** return the number of documents owned by this shell window
*/
int NDocuments(WindowInfo* window)
{
   WindowInfo* win;
   int nDocument = 0;

   for (win = WindowList; win; win = win->next)
   {
      if (win->mainWindow == window->mainWindow)
         nDocument++;
   }

   return nDocument;
}

/*
** refresh window state for this document
*/
void RefreshWindowStates(WindowInfo* window)
{
   if (!IsTopDocument(window))
      return;

   if (window->modeMessageDisplayed)
      ; // TODO: NeTextSetString(window->statsLine, window->modeMessage);
   else
      UpdateStatsLine(window);
   UpdateWindowReadOnly(window);
   UpdateWindowTitle(window);

   /* show/hide statsline as needed */
// TODO:    if (window->modeMessageDisplayed && !XtIsManaged(window->statsLineForm))
// TODO:    {
// TODO:       /* turn on statline to display mode message */
// TODO:       showStats(window, true);
// TODO:    }
// TODO:    else if (window->showStats && !XtIsManaged(window->statsLineForm))
// TODO:    {
// TODO:       /* turn on statsline since it is enabled */
// TODO:       showStats(window, true);
// TODO:    }
// TODO:    else if (!window->showStats && !window->modeMessageDisplayed &&
// TODO:             XtIsManaged(window->statsLineForm))
// TODO:    {
// TODO:       /* turn off statsline since there's nothing to show */
// TODO:       showStats(window, false);
// TODO:    }

   /* signal if macro/shell is running */
   if (window->shellCmdData || window->macroCmdData)
      BeginWait(window->mainWindow);
   else
      EndWait(window->mainWindow);

// TODO:    /* we need to force the statsline to reveal itself */
// TODO:    if (XtIsManaged(window->statsLineForm))
// TODO:    {
// TODO:       XmTextSetCursorPosition(window->statsLine, 0);     /* start of line */
// TODO:       XmTextSetCursorPosition(window->statsLine, 9000);  /* end of line */
// TODO:    }
// TODO: 
// TODO:    XmUpdateDisplay(window->statsLine);
   refreshMenuBar(window);

   updateLineNumDisp(window);
}

// TODO: static void cloneTextPanes(WindowInfo* window, WindowInfo* orgWin)
// TODO: {
// TODO:    short paneHeights[MAX_PANES+1];
// TODO:    int insertPositions[MAX_PANES+1], topLines[MAX_PANES+1];
// TODO:    int horizOffsets[MAX_PANES+1];
// TODO:    int i, focusPane, emTabDist, wrapMargin, lineNumCols, totalHeight=0;
// TODO:    char* delimiters;
// TODO:    Fl_Widget* text;
// TODO:    selection sel;
// TODO:    textDisp* textD, *newTextD;
// TODO: 
// TODO:    /* transfer the primary selection */
// TODO:    memcpy(&sel, &orgWin->buffer->primary, sizeof(selection));
// TODO: 
// TODO:    if (sel.selected)
// TODO:    {
// TODO:       if (sel.rectangular)
// TODO:          BufRectSelect(window->buffer, sel.start, sel.end,
// TODO:                        sel.rectStart, sel.rectEnd);
// TODO:       else
// TODO:          BufSelect(window->buffer, sel.start, sel.end);
// TODO:    }
// TODO:    else
// TODO:       BufUnselect(window->buffer);
// TODO: 
// TODO:    /* Record the current heights, scroll positions, and insert positions
// TODO:       of the existing panes, keyboard focus */
// TODO:    focusPane = 0;
// TODO:    for (i=0; i<=orgWin->nPanes; i++)
// TODO:    {
// TODO:       text = i==0 ? orgWin->textArea : orgWin->textPanes[i-1];
// TODO:       insertPositions[i] = TextGetCursorPos(text);
// TODO:       XtVaGetValues(containingPane(text), XmNheight, &paneHeights[i], NULL);
// TODO:       totalHeight += paneHeights[i];
// TODO:       TextGetScroll(text, &topLines[i], &horizOffsets[i]);
// TODO:       if (text == orgWin->lastFocus)
// TODO:          focusPane = i;
// TODO:    }
// TODO: 
// TODO:    window->nPanes = orgWin->nPanes;
// TODO: 
// TODO:    /* Copy some parameters */
// TODO:    XtVaGetValues(orgWin->textArea, textNemulateTabs, &emTabDist,
// TODO:                  textNwordDelimiters, &delimiters, textNwrapMargin, &wrapMargin,
// TODO:                  NULL);
// TODO:    lineNumCols = orgWin->showLineNumbers ? MIN_LINE_NUM_COLS : 0;
// TODO:    XtVaSetValues(window->textArea, textNemulateTabs, emTabDist,
// TODO:                  textNwordDelimiters, delimiters, textNwrapMargin, wrapMargin,
// TODO:                  textNlineNumCols, lineNumCols, NULL);
// TODO: 
// TODO: 
// TODO:    /* clone split panes, if any */
// TODO:    textD = ((TextWidget)window->textArea)->text.textD;
// TODO:    if (window->nPanes)
// TODO:    {
// TODO:       /* Unmanage & remanage the panedWindow so it recalculates pane
// TODO:               heights */
// TODO:       XtUnmanageChild(window->splitPane);
// TODO: 
// TODO:       /* Create a text widget to add to the pane and set its buffer and
// TODO:          highlight data to be the same as the other panes in the orgWin */
// TODO: 
// TODO:       for (i=0; i<orgWin->nPanes; i++)
// TODO:       {
// TODO:          text = createTextArea(window->splitPane, window, 1, 1, emTabDist,
// TODO:                                delimiters, wrapMargin, lineNumCols);
// TODO:          TextSetBuffer(text, window->buffer);
// TODO: 
// TODO:          if (window->highlightData != NULL)
// TODO:             AttachHighlightToWidget(text, window);
// TODO:          XtManageChild(text);
// TODO:          window->textPanes[i] = text;
// TODO: 
// TODO:          /* Fix up the colors */
// TODO:          newTextD = ((TextWidget)text)->text.textD;
// TODO:          XtVaSetValues(text, XmNforeground, textD->fgPixel,
// TODO:                        XmNbackground, textD->bgPixel, NULL);
// TODO:          TextDSetColors(newTextD, textD->fgPixel, textD->bgPixel,
// TODO:                         textD->selectFGPixel, textD->selectBGPixel,
// TODO:                         textD->highlightFGPixel,textD->highlightBGPixel,
// TODO:                         textD->lineNumFGPixel, textD->cursorFGPixel);
// TODO:       }
// TODO: 
// TODO:       /* Set the minimum pane height in the new pane */
// TODO:       UpdateMinPaneHeights(window);
// TODO: 
// TODO:       for (i=0; i<=window->nPanes; i++)
// TODO:       {
// TODO:          text = i==0 ? window->textArea : window->textPanes[i-1];
// TODO:          setPaneDesiredHeight(containingPane(text), paneHeights[i]);
// TODO:       }
// TODO: 
// TODO:       /* Re-manage panedWindow to recalculate pane heights & reset selection */
// TODO:       XtManageChild(window->splitPane);
// TODO:    }
// TODO: 
// TODO:    /* Reset all of the heights, scroll positions, etc. */
// TODO:    for (i=0; i<=window->nPanes; i++)
// TODO:    {
// TODO:       textDisp* textD;
// TODO: 
// TODO:       text = i==0 ? window->textArea : window->textPanes[i-1];
// TODO:       TextSetCursorPos(text, insertPositions[i]);
// TODO:       TextSetScroll(text, topLines[i], horizOffsets[i]);
// TODO: 
// TODO:       /* dim the cursor */
// TODO:       textD = ((TextWidget)text)->text.textD;
// TODO:       TextDSetCursorStyle(textD, DIM_CURSOR);
// TODO:       TextDUnblankCursor(textD);
// TODO:    }
// TODO: 
// TODO:    /* set the focus pane */
// TODO:    for (i=0; i<=window->nPanes; i++)
// TODO:    {
// TODO:       text = i==0 ? window->textArea : window->textPanes[i-1];
// TODO:       if (i == focusPane)
// TODO:       {
// TODO:          window->lastFocus = text;
// TODO:          XmProcessTraversal(text, XmTRAVERSE_CURRENT);
// TODO:          break;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* Update the window manager size hints after the sizes of the panes have
// TODO:       been set (the widget heights are not yet readable here, but they will
// TODO:       be by the time the event loop gets around to running this timer proc) */
// TODO:    XtAppAddTimeOut(XtWidgetToApplicationContext(window->mainWindow), 0,
// TODO:                    wmSizeUpdateProc, window);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** clone a document's states and settings into the other.
// TODO: */
// TODO: static void cloneDocument(WindowInfo* window, WindowInfo* orgWin)
// TODO: {
// TODO:    const char* orgDocument;
// TODO:    char* params[4];
// TODO:    int emTabDist;
// TODO: 
// TODO:    strcpy(window->path, orgWin->path);
// TODO:    strcpy(window->filename, orgWin->filename);
// TODO: 
// TODO:    ShowLineNumbers(window, orgWin->showLineNumbers);
// TODO: 
// TODO:    window->ignoreModify = true;
// TODO: 
// TODO:    /* copy the text buffer */
// TODO:    orgDocument = BufAsString(orgWin->buffer);
// TODO:    BufSetAll(window->buffer, orgDocument);
// TODO: 
// TODO:    /* copy the tab preferences (here!) */
// TODO:    BufSetTabDistance(window->buffer, orgWin->buffer->tabDist);
// TODO:    window->buffer->useTabs = orgWin->buffer->useTabs;
// TODO:    XtVaGetValues(orgWin->textArea, textNemulateTabs, &emTabDist, NULL);
// TODO:    SetEmTabDist(window, emTabDist);
// TODO: 
// TODO:    window->ignoreModify = false;
// TODO: 
// TODO:    /* transfer text fonts */
// TODO:    params[0] = orgWin->fontName;
// TODO:    params[1] = orgWin->italicFontName;
// TODO:    params[2] = orgWin->boldFontName;
// TODO:    params[3] = orgWin->boldItalicFontName;
// TODO:    XtCallActionProc(window->textArea, "set_fonts", NULL, params, 4);
// TODO: 
// TODO:    SetBacklightChars(window, orgWin->backlightCharTypes);
// TODO: 
// TODO:    /* Clone rangeset info.
// TODO: 
// TODO:       FIXME:
// TODO:       Cloning of rangesets must be done before syntax highlighting,
// TODO:       else the rangesets do not be highlighted (colored) properly
// TODO:       if syntax highlighting is on.
// TODO:    */
// TODO:    window->buffer->rangesetTable =
// TODO:       RangesetTableClone(orgWin->buffer->rangesetTable, window->buffer);
// TODO: 
// TODO:    /* Syntax highlighting */
// TODO:    window->languageMode = orgWin->languageMode;
// TODO:    window->highlightSyntax = orgWin->highlightSyntax;
// TODO:    if (window->highlightSyntax)
// TODO:       StartHighlighting(window, false);
// TODO: 
// TODO:    /* copy states of original document */
// TODO:    window->filenameSet = orgWin->filenameSet;
// TODO:    window->fileFormat = orgWin->fileFormat;
// TODO:    window->lastModTime = orgWin->lastModTime;
// TODO:    window->fileChanged = orgWin->fileChanged;
// TODO:    window->fileMissing = orgWin->fileMissing;
// TODO:    window->lockReasons = orgWin->lockReasons;
// TODO:    window->autoSaveCharCount = orgWin->autoSaveCharCount;
// TODO:    window->autoSaveOpCount = orgWin->autoSaveOpCount;
// TODO:    window->undoOpCount = orgWin->undoOpCount;
// TODO:    window->undoMemUsed = orgWin->undoMemUsed;
// TODO:    window->lockReasons = orgWin->lockReasons;
// TODO:    window->autoSave = orgWin->autoSave;
// TODO:    window->saveOldVersion = orgWin->saveOldVersion;
// TODO:    window->wrapMode = orgWin->wrapMode;
// TODO:    SetOverstrike(window, orgWin->overstrike);
// TODO:    window->showMatchingStyle = orgWin->showMatchingStyle;
// TODO:    window->matchSyntaxBased = orgWin->matchSyntaxBased;
// TODO: #if 0
// TODO:    window->showStats = orgWin->showStats;
// TODO:    window->showISearchLine = orgWin->showISearchLine;
// TODO:    window->showLineNumbers = orgWin->showLineNumbers;
// TODO:    window->modeMessageDisplayed = orgWin->modeMessageDisplayed;
// TODO:    window->ignoreModify = orgWin->ignoreModify;
// TODO:    window->windowMenuValid = orgWin->windowMenuValid;
// TODO:    window->flashTimeoutID = orgWin->flashTimeoutID;
// TODO:    window->wasSelected = orgWin->wasSelected;
// TODO:    strcpy(window->fontName, orgWin->fontName);
// TODO:    strcpy(window->italicFontName, orgWin->italicFontName);
// TODO:    strcpy(window->boldFontName, orgWin->boldFontName);
// TODO:    strcpy(window->boldItalicFontName, orgWin->boldItalicFontName);
// TODO:    window->fontList = orgWin->fontList;
// TODO:    window->italicFontStruct = orgWin->italicFontStruct;
// TODO:    window->boldFontStruct = orgWin->boldFontStruct;
// TODO:    window->boldItalicFontStruct = orgWin->boldItalicFontStruct;
// TODO:    window->markTimeoutID = orgWin->markTimeoutID;
// TODO:    window->highlightData = orgWin->highlightData;
// TODO:    window->shellCmdData = orgWin->shellCmdData;
// TODO:    window->macroCmdData = orgWin->macroCmdData;
// TODO:    window->smartIndentData = orgWin->smartIndentData;
// TODO: #endif
// TODO:    window->iSearchHistIndex = orgWin->iSearchHistIndex;
// TODO:    window->iSearchStartPos = orgWin->iSearchStartPos;
// TODO:    window->replaceLastRegexCase = orgWin->replaceLastRegexCase;
// TODO:    window->replaceLastLiteralCase = orgWin->replaceLastLiteralCase;
// TODO:    window->iSearchLastRegexCase = orgWin->iSearchLastRegexCase;
// TODO:    window->iSearchLastLiteralCase = orgWin->iSearchLastLiteralCase;
// TODO:    window->findLastRegexCase = orgWin->findLastRegexCase;
// TODO:    window->findLastLiteralCase = orgWin->findLastLiteralCase;
// TODO:    window->device = orgWin->device;
// TODO:    window->inode = orgWin->inode;
// TODO:    window->fileClosedAtom = orgWin->fileClosedAtom;
// TODO:    orgWin->fileClosedAtom = None;
// TODO: 
// TODO:    /* copy the text/split panes settings, cursor pos & selection */
// TODO:    cloneTextPanes(window, orgWin);
// TODO: 
// TODO:    /* copy undo & redo list */
// TODO:    window->undo = cloneUndoItems(orgWin->undo);
// TODO:    window->redo = cloneUndoItems(orgWin->redo);
// TODO: 
// TODO:    /* copy bookmarks */
// TODO:    window->nMarks = orgWin->nMarks;
// TODO:    memcpy(&window->markTable, &orgWin->markTable,
// TODO:           sizeof(Bookmark)*window->nMarks);
// TODO: 
// TODO:    /* kick start the auto-indent engine */
// TODO:    window->indentStyle = NO_AUTO_INDENT;
// TODO:    SetAutoIndent(window, orgWin->indentStyle);
// TODO: 
// TODO:    /* synchronize window state to this document */
// TODO:    RefreshWindowStates(window);
// TODO: }
// TODO: 
// TODO: static UndoInfo* cloneUndoItems(UndoInfo* orgList)
// TODO: {
// TODO:    UndoInfo* head = NULL, *undo, *clone, *last = NULL;
// TODO: 
// TODO:    for (undo = orgList; undo; undo = undo->next)
// TODO:    {
// TODO:       clone = (UndoInfo*)malloc__(sizeof(UndoInfo));
// TODO:       memcpy(clone, undo, sizeof(UndoInfo));
// TODO: 
// TODO:       if (undo->oldText)
// TODO:       {
// TODO:          clone->oldText = malloc__(strlen(undo->oldText)+1);
// TODO:          strcpy(clone->oldText, undo->oldText);
// TODO:       }
// TODO:       clone->next = NULL;
// TODO: 
// TODO:       if (last)
// TODO:          last->next = clone;
// TODO:       else
// TODO:          head = clone;
// TODO: 
// TODO:       last = clone;
// TODO:    }
// TODO: 
// TODO:    return head;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** spin off the document to a new window
// TODO: */
// TODO: WindowInfo* DetachDocument(WindowInfo* window)
// TODO: {
// TODO:    WindowInfo* win = NULL, *cloneWin;
// TODO: 
// TODO:    if (NDocuments(window) < 2)
// TODO:       return NULL;
// TODO: 
// TODO:    /* raise another document in the same shell window if the window
// TODO:       being detached is the top document */
// TODO:    if (IsTopDocument(window))
// TODO:    {
// TODO:       win = getNextTabWindow(window, 1, 0, 0);
// TODO:       RaiseDocument(win);
// TODO:    }
// TODO: 
// TODO:    /* Create a new window */
// TODO:    cloneWin = CreateWindow(window->filename, NULL, false);
// TODO: 
// TODO:    /* CreateWindow() simply adds the new window's pointer to the
// TODO:       head of WindowList. We need to adjust the detached window's
// TODO:       pointer, so that macro functions such as focus_window("last")
// TODO:       will travel across the documents per the sequence they're
// TODO:       opened. The new doc will appear to replace it's former self
// TODO:       as the old doc is closed. */
// TODO:    WindowList = cloneWin->next;
// TODO:    cloneWin->next = window->next;
// TODO:    window->next = cloneWin;
// TODO: 
// TODO:    /* these settings should follow the detached document.
// TODO:       must be done before cloning window, else the height
// TODO:       of split panes may not come out correctly */
// TODO:    ShowISearchLine(cloneWin, window->showISearchLine);
// TODO:    ShowStatsLine(cloneWin, window->showStats);
// TODO: 
// TODO:    /* clone the document & its pref settings */
// TODO:    cloneDocument(cloneWin, window);
// TODO: 
// TODO:    /* remove the document from the old window */
// TODO:    window->fileChanged = false;
// TODO:    CloseFileAndWindow(window, NO_SBC_DIALOG_RESPONSE);
// TODO: 
// TODO:    /* refresh former host window */
// TODO:    if (win)
// TODO:    {
// TODO:       RefreshWindowStates(win);
// TODO:    }
// TODO: 
// TODO:    /* this should keep the new document window fresh */
// TODO:    RefreshWindowStates(cloneWin);
// TODO:    RefreshTabState(cloneWin);
// TODO:    SortTabBar(cloneWin);
// TODO: 
// TODO:    return cloneWin;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Move document to an other window.
// TODO: **
// TODO: ** the moving document will receive certain window settings from
// TODO: ** its new host, i.e. the window size, stats and isearch lines.
// TODO: */
// TODO: WindowInfo* MoveDocument(WindowInfo* toWindow, WindowInfo* window)
// TODO: {
// TODO:    WindowInfo* win = NULL, *cloneWin;
// TODO: 
// TODO:    /* prepare to move document */
// TODO:    if (NDocuments(window) < 2)
// TODO:    {
// TODO:       /* hide the window to make it look like we are moving */
// TODO:       XtUnmapWidget(window->mainWindow);
// TODO:    }
// TODO:    else if (IsTopDocument(window))
// TODO:    {
// TODO:       /* raise another document to replace the document being moved */
// TODO:       win = getNextTabWindow(window, 1, 0, 0);
// TODO:       RaiseDocument(win);
// TODO:    }
// TODO: 
// TODO:    /* relocate the document to target window */
// TODO:    cloneWin = CreateDocument(toWindow, window->filename);
// TODO:    ShowTabBar(cloneWin, GetShowTabBar(cloneWin));
// TODO:    cloneDocument(cloneWin, window);
// TODO: 
// TODO:    /* CreateDocument() simply adds the new window's pointer to the
// TODO:       head of WindowList. We need to adjust the detached window's
// TODO:       pointer, so that macro functions such as focus_window("last")
// TODO:       will travel across the documents per the sequence they're
// TODO:       opened. The new doc will appear to replace it's former self
// TODO:       as the old doc is closed. */
// TODO:    WindowList = cloneWin->next;
// TODO:    cloneWin->next = window->next;
// TODO:    window->next = cloneWin;
// TODO: 
// TODO:    /* remove the document from the old window */
// TODO:    window->fileChanged = false;
// TODO:    CloseFileAndWindow(window, NO_SBC_DIALOG_RESPONSE);
// TODO: 
// TODO:    /* some menu states might have changed when deleting document */
// TODO:    if (win)
// TODO:       RefreshWindowStates(win);
// TODO: 
// TODO:    /* this should keep the new document window fresh */
// TODO:    RaiseDocumentWindow(cloneWin);
// TODO:    RefreshTabState(cloneWin);
// TODO:    SortTabBar(cloneWin);
// TODO: 
// TODO:    return cloneWin;
// TODO: }
// TODO: 
// TODO: static void moveDocumentCB(Fl_Widget* dialog, WindowInfo* window,
// TODO:                            XtPointer call_data)
// TODO: {
// TODO:    XmSelectionBoxCallbackStruct* cbs = (XmSelectionBoxCallbackStruct*) call_data;
// TODO:    DoneWithMoveDocumentDialog = cbs->reason;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** present dialog for selecting a target window to move this document
// TODO: ** into. Do nothing if there is only one shell window opened.
// TODO: */
// TODO: void MoveDocumentDialog(WindowInfo* window)
// TODO: {
// TODO:    WindowInfo* win, *targetWin, **shellWinList;
// TODO:    int i, nList=0, nWindows=0, ac;
// TODO:    char tmpStr[MAXPATHLEN+50];
// TODO:    Fl_Widget* parent, dialog, listBox, moveAllOption;
// TODO:    NeString* list = NULL;
// TODO:    NeString popupTitle, s1;
// TODO:    Arg csdargs[20];
// TODO:    int* position_list, position_count;
// TODO: 
// TODO:    /* get the list of available shell windows, not counting
// TODO:       the document to be moved */
// TODO:    nWindows = NWindows();
// TODO:    list = (XmStringTable) malloc__(nWindows * sizeof(NeString*));
// TODO:    shellWinList = (WindowInfo**) malloc__(nWindows * sizeof(WindowInfo*));
// TODO: 
// TODO:    for (win=WindowList; win; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win) || win->mainWindow == window->mainWindow)
// TODO:          continue;
// TODO: 
// TODO:       sprintf(tmpStr, "%s%s",
// TODO:               win->filenameSet? win->path : "", win->filename);
// TODO: 
// TODO:       list[nList] = NeNewString(tmpStr);
// TODO:       shellWinList[nList] = win;
// TODO:       nList++;
// TODO:    }
// TODO: 
// TODO:    /* stop here if there's no other window to move to */
// TODO:    if (!nList)
// TODO:    {
// TODO:       free__((char*)list);
// TODO:       free__((char*)shellWinList);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* create the dialog */
// TODO:    parent = window->mainWindow;
// TODO:    popupTitle = NeNewString("Move Document");
// TODO:    sprintf(tmpStr, "Move %s into window of", window->filename);
// TODO:    s1 = NeNewString(tmpStr);
// TODO:    ac = 0;
// TODO:    XtSetArg(csdargs[ac], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
// TODO:    ac++;
// TODO:    XtSetArg(csdargs[ac], XmNdialogTitle, popupTitle);
// TODO:    ac++;
// TODO:    XtSetArg(csdargs[ac], XmNlistLabelString, s1);
// TODO:    ac++;
// TODO:    XtSetArg(csdargs[ac], XmNlistItems, list);
// TODO:    ac++;
// TODO:    XtSetArg(csdargs[ac], XmNlistItemCount, nList);
// TODO:    ac++;
// TODO:    XtSetArg(csdargs[ac], XmNvisibleItemCount, 12);
// TODO:    ac++;
// TODO:    XtSetArg(csdargs[ac], XmNautoUnmanage, false);
// TODO:    ac++;
// TODO:    dialog = CreateSelectionDialog(parent,"moveDocument",csdargs,ac);
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_TEXT));
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_SELECTION_LABEL));
// TODO:    XtAddCallback(dialog, XmNokCallback, (XtCallbackProc)moveDocumentCB, window);
// TODO:    XtAddCallback(dialog, XmNapplyCallback, (XtCallbackProc)moveDocumentCB, window);
// TODO:    XtAddCallback(dialog, XmNcancelCallback, (XtCallbackProc)moveDocumentCB, window);
// TODO:    NeStringFree(s1);
// TODO:    NeStringFree(popupTitle);
// TODO: 
// TODO:    /* free__ the window list */
// TODO:    for (i=0; i<nList; i++)
// TODO:       NeStringFree(list[i]);
// TODO:    free__((char*)list);
// TODO: 
// TODO:    /* create the option box for moving all documents */
// TODO:    s1 = MKSTRING("Move all documents in this window");
// TODO:    moveAllOption =  XtVaCreateWidget("moveAll",
// TODO:                                      xmToggleButtonWidgetClass, dialog,
// TODO:                                      XmNlabelString, s1,
// TODO:                                      XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                      NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    if (NDocuments(window) >1)
// TODO:       XtManageChild(moveAllOption);
// TODO: 
// TODO:    /* disable option if only one document in the window */
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_APPLY_BUTTON));
// TODO: 
// TODO:    s1 = MKSTRING("Move");
// TODO:    XtVaSetValues(dialog, XmNokLabelString, s1, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    /* default to the first window on the list */
// TODO:    listBox = XmSelectionBoxGetChild(dialog, XmDIALOG_LIST);
// TODO:    XmListSelectPos(listBox, 1, true);
// TODO: 
// TODO:    /* show the dialog */
// TODO:    DoneWithMoveDocumentDialog = 0;
// TODO:    ManageDialogCenteredOnPointer(dialog);
// TODO:    while (!DoneWithMoveDocumentDialog)
// TODO:       XtAppProcessEvent(XtWidgetToApplicationContext(parent), XtIMAll);
// TODO: 
// TODO:    /* get the window to move document into */
// TODO:    XmListGetSelectedPos(listBox, &position_list, &position_count);
// TODO:    targetWin = shellWinList[position_list[0]-1];
// TODO:    free__((char*)position_list);
// TODO: 
// TODO:    /* now move document(s) */
// TODO:    if (DoneWithMoveDocumentDialog == XmCR_OK)
// TODO:    {
// TODO:       /* move top document */
// TODO:       if (NeToggleButtonGetState(moveAllOption))
// TODO:       {
// TODO:          /* move all documents */
// TODO:          for (win = WindowList; win;)
// TODO:          {
// TODO:             if (win != window && win->mainWindow == window->mainWindow)
// TODO:             {
// TODO:                WindowInfo* next = win->next;
// TODO:                MoveDocument(targetWin, win);
// TODO:                win = next;
// TODO:             }
// TODO:             else
// TODO:                win = win->next;
// TODO:          }
// TODO: 
// TODO:          /* invoking document is the last to move */
// TODO:          MoveDocument(targetWin, window);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          MoveDocument(targetWin, window);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    free__((char*)shellWinList);
// TODO:    XtDestroyWidget(dialog);
// TODO: }
// TODO: 
// TODO: static void hideTooltip(Fl_Widget* tab)
// TODO: {
// TODO:    Fl_Widget* tooltip = XtNameToWidget(tab, "*BubbleShell");
// TODO: 
// TODO:    if (tooltip)
// TODO:       XtPopdown(tooltip);
// TODO: }
// TODO: 
// TODO: static void closeTabProc(XtPointer clientData, int* id)
// TODO: {
// TODO:    CloseFileAndWindow((WindowInfo*)clientData, PROMPT_SBC_DIALOG_RESPONSE);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** callback to close-tab button.
// TODO: */
// TODO: static void closeTabCB(Fl_Widget* w, Fl_Widget* mainWin, caddr_t callData)
// TODO: {
// TODO:    /* FIXME: XtRemoveActionHook() related coredump
// TODO: 
// TODO:       An unknown bug seems to be associated with the XtRemoveActionHook()
// TODO:       call in FinishLearn(), which resulted in coredump if a tab was
// TODO:       closed, in the middle of keystrokes learning, by clicking on the
// TODO:       close-tab button.
// TODO: 
// TODO:       As evident to our accusation, the coredump may be surpressed by
// TODO:       simply commenting out the XtRemoveActionHook() call. The bug was
// TODO:       consistent on both Motif and Lesstif on various platforms.
// TODO: 
// TODO:       Closing the tab through either the "Close" menu or its accel key,
// TODO:       however, was without any trouble.
// TODO: 
// TODO:       While its actual mechanism is not well understood, we somehow
// TODO:       managed to workaround the bug by delaying the action of closing
// TODO:       the tab. For now. */
// TODO:    XtAppAddTimeOut(XtWidgetToApplicationContext(w), 0,
// TODO:                    closeTabProc, GetTopDocument(mainWin));
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** callback to clicks on a tab to raise it's document.
// TODO: */
// TODO: static void raiseTabCB(Fl_Widget* w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    XmLFolderCallbackStruct* cbs = (XmLFolderCallbackStruct*)callData;
// TODO:    WidgetList tabList;
// TODO:    Fl_Widget* tab;
// TODO: 
// TODO:    XtVaGetValues(w, XmNtabWidgetList, &tabList, NULL);
// TODO:    tab = tabList[cbs->pos];
// TODO:    RaiseDocument(TabToWindow(tab));
// TODO: }
// TODO: 
// TODO: static Fl_Widget* containingPane(Fl_Widget* w)
// TODO: {
// TODO:    /* The containing pane used to simply be the first parent, but with
// TODO:       the introduction of an XmFrame, it's the grandparent. */
// TODO:    return XtParent(XtParent(w));
// TODO: }

static void cancelTimeOut(int* timer)
{
   if (*timer != 0)
   {
// TODO:       XtRemoveTimeOut(*timer);
      *timer = 0;
   }
}

/*
** set/clear toggle menu state if the calling document is on top.
*/
void SetToggleButtonState(WindowInfo* window, Fl_Widget* w, bool state, bool notify)
{
   if (IsTopDocument(window))
   {
// TODO:       NeToggleButtonSetState(w, state, notify);
   }
}

// set/clear menu sensitivity if the calling document is on top.
void SetSensitive(WindowInfo* window, Fl_Widget* w, bool sensitive)
{
   if (IsTopDocument(window))
   {
      NeSetSensitive(w, sensitive);
   }
}

void SetSensitive(WindowInfo* window, Fl_Menu_Item* item, bool sensitive)
{
   if (IsTopDocument(window))
   {
      NeSetSensitive(item, sensitive);
   }
}
