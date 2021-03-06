/* $Id: nedit.h,v 1.69 2008/01/04 22:11:03 yooden Exp $ */
/*******************************************************************************
*                                                                              *
* nedit.h -- Nirvana Editor Common Header File                                 *
*                                                                              *
* Copyright 2004 The NEdit Developers                                          *
*                                                                              *
* This is free__ software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version. In addition, you may distribute versions of this program linked to  *
* Motif or Open Motif. See README for details.                                 *
*                                                                              *
* This software is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for    *
* more details.                                                                *
*                                                                              *
* You should have received a copy of the GNU General Public License along with *
* software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
* Place, Suite 330, Boston, MA  02111-1307 USA                                 *
*                                                                              *
* Nirvana Text Editor                                                          *
* July 31, 2001                                                                *
*                                                                              *
*******************************************************************************/

#ifndef NEDIT_NEDIT_H_INCLUDED
#define NEDIT_NEDIT_H_INCLUDED

#include "Ne_Text_Buffer.h"
#include "Ne_Text_Editor.h"
#include "../util/utils.h"
#include "../util/misc.h"
#include "../util/Ne_AppContext.h"
#include "../util/Ne_MenuBar.h"
#include "../util/Ne_Font.h"
#include "../util/Ne_StatsLine.h"
#include <sys/types.h>

#ifndef WIN32
#include <sys/param.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Toggle_Button.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Browser.H>

#include <iostream>
#include <list>

#define NEDIT_VERSION           5
#define NEDIT_REVISION          5

/* Some default colors */
#define NEDIT_DEFAULT_FG        "black"
#define NEDIT_DEFAULT_TEXT_BG   "grey90"
#define NEDIT_DEFAULT_SEL_FG    "black"
#define NEDIT_DEFAULT_SEL_BG    "gray80"
#define NEDIT_DEFAULT_HI_FG     "white" // These are colors for flashing
#define NEDIT_DEFAULT_HI_BG     "red"   // matching parens
#define NEDIT_DEFAULT_LINENO_FG "black"
#define NEDIT_DEFAULT_CURSOR_FG "black"
#define NEDIT_DEFAULT_HELP_FG   "black"
#define NEDIT_DEFAULT_HELP_BG   "gray80"
#define NEDIT_DEFAULT_CALLTIP_FG "black"
#define NEDIT_DEFAULT_CALLTIP_BG "LemonChiffon1"

/* Tuning parameters */
#define SEARCHMAX 5119          /* Maximum length of search/replace strings */
#define MAX_SEARCH_HISTORY 100   /* Maximum length of search string history */
#define MAX_PANES 6      /* Max # of ADDITIONAL text editing panes
that can be added to a window */
#ifndef VMS
#define AUTOSAVE_CHAR_LIMIT 30   /* number of characters user can type before
NEdit generates a new backup file */
#else
#define AUTOSAVE_CHAR_LIMIT 80   /* set higher on VMS becaus saving is slower */
#endif /*VMS*/
#define AUTOSAVE_OP_LIMIT 8   /* number of distinct editing operations user
can do before NEdit gens. new backup file */
#define MAX_FONT_LEN 100   /* maximum length for a font name */
#define MAX_COLOR_LEN 30   /* maximum length for a color name */
#define MAX_MARKS 36          /* max. # of bookmarks (one per letter & #) */
#define MIN_LINE_NUM_COLS 4    /* Min. # of columns in line number display */
#define APP_NAME "nedit"   /* application name for loading resources */
#define APP_CLASS "NEdit"   /* application class for loading resources */

   /* The accumulated list of undo operations can potentially consume huge
      amounts of memory.  These tuning parameters determine how much undo infor-
      mation is retained.  Normally, the list is kept between UNDO_OP_LIMIT and
      UNDO_OP_TRIMTO in length (when the list reaches UNDO_OP_LIMIT, it is
      trimmed to UNDO_OP_TRIMTO then allowed to grow back to UNDO_OP_LIMIT).
      When there are very large amounts of saved text held in the list,
      UNDO_WORRY_LIMIT and UNDO_PURGE_LIMIT take over and cause the list to
      be trimmed back further to keep its size down. */
#define UNDO_PURGE_LIMIT 15000000 /* If undo list gets this large (in bytes),
   trim it to length of UNDO_PURGE_TRIMTO */
#define UNDO_PURGE_TRIMTO 1     /* Amount to trim the undo list in a purge */
#define UNDO_WORRY_LIMIT 2000000  /* If undo list gets this large (in bytes),
   trim it to length of UNDO_WORRY_TRIMTO */
#define UNDO_WORRY_TRIMTO 5     /* Amount to trim the undo list when memory
   use begins to get serious */
#define UNDO_OP_LIMIT 400     /* normal limit for length of undo list */
#define UNDO_OP_TRIMTO 200     /* size undo list is normally trimmed to
   when it exceeds UNDO_OP_TRIMTO in length */

   enum indentStyle {NO_AUTO_INDENT, AUTO_INDENT, SMART_INDENT};
enum wrapStyle {NO_WRAP, NEWLINE_WRAP, CONTINUOUS_WRAP};
enum showMatchingStyle {NO_FLASH, FLASH_DELIMIT, FLASH_RANGE};
enum virtKeyOverride { VIRT_KEY_OVERRIDE_NEVER, VIRT_KEY_OVERRIDE_AUTO,
                       VIRT_KEY_OVERRIDE_ALWAYS
                     };

// This enum must be kept in parallel to the array TruncSubstitutionModes[] in preferences.c 
enum truncSubstitution {TRUNCSUBST_SILENT, TRUNCSUBST_FAIL, TRUNCSUBST_WARN, TRUNCSUBST_IGNORE};

#define NO_FLASH_STRING      "off"
#define FLASH_DELIMIT_STRING   "delimiter"
#define FLASH_RANGE_STRING   "range"

// TODO: #define MKSTRING(string) \
// TODO:    XmStringCreateLtoR(string, XmSTRING_DEFAULT_CHARSET)
// TODO:
// TODO: #define SET_ONE_RSRC(widget, name, newValue) \
// TODO: { \
// TODO:     static Arg args[1] = {{name, (XtArgVal)0}}; \
// TODO:     args[0].value = (XtArgVal)newValue; \
// TODO:     XtSetValues(widget, args, 1); \
// TODO: }

// This handles all the different reasons files can be locked
#define USER_LOCKED_BIT     0
#define PERM_LOCKED_BIT     1
#define TOO_MUCH_BINARY_DATA_LOCKED_BIT 2

#define LOCKED_BIT_TO_MASK(bitNum) (1 << (bitNum))
#define SET_LOCKED_BY_REASON(reasons, onOrOff, reasonBit) ((onOrOff) ? \
                    ((reasons) |= LOCKED_BIT_TO_MASK(reasonBit)) : \
                    ((reasons) &= ~LOCKED_BIT_TO_MASK(reasonBit)))

#define IS_USER_LOCKED(reasons) (((reasons) & LOCKED_BIT_TO_MASK(USER_LOCKED_BIT)) != 0)
#define SET_USER_LOCKED(reasons, onOrOff) SET_LOCKED_BY_REASON(reasons, onOrOff, USER_LOCKED_BIT)
#define IS_PERM_LOCKED(reasons) (((reasons) & LOCKED_BIT_TO_MASK(PERM_LOCKED_BIT)) != 0)
#define SET_PERM_LOCKED(reasons, onOrOff) SET_LOCKED_BY_REASON(reasons, onOrOff, PERM_LOCKED_BIT)
#define IS_TMBD_LOCKED(reasons) (((reasons) & LOCKED_BIT_TO_MASK(TOO_MUCH_BINARY_DATA_LOCKED_BIT)) != 0)
#define SET_TMBD_LOCKED(reasons, onOrOff) SET_LOCKED_BY_REASON(reasons, onOrOff, TOO_MUCH_BINARY_DATA_LOCKED_BIT)

#define IS_ANY_LOCKED_IGNORING_USER(reasons) (((reasons) & ~LOCKED_BIT_TO_MASK(USER_LOCKED_BIT)) != 0)
#define IS_ANY_LOCKED_IGNORING_PERM(reasons) (((reasons) & ~LOCKED_BIT_TO_MASK(PERM_LOCKED_BIT)) != 0)
#define IS_ANY_LOCKED(reasons) ((reasons) != 0)
#define CLEAR_ALL_LOCKS(reasons) ((reasons) = 0)

// determine a safe size for a string to hold an integer-like number contained in xType
#define TYPE_INT_STR_SIZE(xType) ((sizeof(xType) * 3) + 2)

// Record on undo list
struct UndoInfo
{
   int type;
   int startPos;
   int endPos;
   int oldLen;
   char* oldText;
   char inUndo;   // flag to indicate undo command on
                  // this record in progress.  Redirects
                  // SaveUndoInfo to save the next modifications
                  // on the redo list instead of the undo list.
   char restoresToSaved;
                  // flag to indicate undoing this
                  // operation will restore file to
                  // last saved (unmodified) state
   UndoInfo* next;   // pointer to the next undo record
};

// Element in bookmark table
struct Bookmark
{
   char label;
   int cursorPos;
   selection sel;
};

// Identifiers for the different colors that can be adjusted.
enum colorTypes
{
   TEXT_FG_COLOR,
   TEXT_BG_COLOR,
   SELECT_FG_COLOR,
   SELECT_BG_COLOR,
   HILITE_FG_COLOR,
   HILITE_BG_COLOR,
   LINENO_FG_COLOR,
   CURSOR_FG_COLOR,
   NUM_COLORS
};

// cache user menus: manage mode of user menu list element
enum UserMenuManageMode
{
   UMMM_UNMANAGE,     /* user menu item is unmanaged */
   UMMM_UNMANAGE_ALL, /* user menu item is a sub menu and is completely unmanaged (including nested sub menus) */
   UMMM_MANAGE,       /* user menu item is managed; menu items of potential sub menu are (un)managed individually */
   UMMM_MANAGE_ALL    /* user menu item is a sub menu and is completely managed */
};

struct UserMenuList;

// structure representing one user menu item
struct UserMenuListElement
{
   UserMenuManageMode    umleManageMode;          /* current manage mode */
   UserMenuManageMode    umlePrevManageMode;      /* previous manage mode */
   char*                 umleAccKeys;             /* accelerator keys of item */
   bool                  umleAccLockPatchApplied; /* indicates, if accelerator lock patch is applied */
   Fl_Widget*            umleMenuItem;            /* menu item represented by this element */
   Fl_Widget*            umleSubMenuPane;         /* holds menu pane, if item represents a sub menu */
   UserMenuList* umleSubMenuList;                 /* elements of sub menu, if item represents a sub menu */
};

/* structure holding a list of user menu items */
struct UserMenuList
{
   int umlNbrItems;
   UserMenuListElement** umlItems;
};

/* structure holding cache info about Shell and Macro menus, which are
   shared over all "tabbed" documents (needed to manage/unmanage this
   user definable menus when language mode changes) */
struct UserMenuCache
{
   int          umcLanguageMode;     /* language mode applied for shared user menus */
   bool         umcShellMenuCreated; /* indicating, if all shell menu items were created */
   bool         umcMacroMenuCreated; /* indicating, if all macro menu items were created */
   UserMenuList umcShellMenuList;    /* list of all shell menu items */
   UserMenuList umcMacroMenuList;    /* list of all macro menu items */
} ;

/* structure holding cache info about Background menu, which is
   owned by each document individually (needed to manage/unmanage this
   user definable menu when language mode changes) */
struct UserBGMenuCache
{
   int          ubmcLanguageMode;    /* language mode applied for background user menu */
   bool         ubmcMenuCreated;     /* indicating, if all background menu items were created */
   UserMenuList ubmcMenuList;        /* list of all background menu items */
};

// --------------------------------------------------------------------------
// The WindowInfo structure holds the information on a Document. A number
// of 'tabbed' documents may reside within a shell window, hence some of
// its members are of 'shell-level'; namely the find/replace dialogs, the
// menu bar & its associated members, the components on the stats area
// (i-search line, statsline and tab bar), plus probably a few others.
// See CreateWindow() and CreateDocument() for more info.
//
// Each document actually 'lives' within its splitPane widget member,
// which can be raised to become the 'top' (visible) document by function
// RaiseDocument(). The non-top documents may still be accessed through
// macros, or the context menu on the tab bar.
//
// Prior to the introduction of tabbed mode, each window may house only
// one document, making it effectively an 'editor window', hence the name
// WindowInfo. This struct name has been preserved to ease the transition
// when tabbed mode was introduced after NEdit 5.4.
// --------------------------------------------------------------------------
struct WindowInfo
{
   enum WindowInfoType { Window, Document };
   // Window or Document ?
   WindowInfoType type; 
   WindowInfo* next;
   // main window
   Fl_Double_Window* mainWindow;

   Ne_MenuBar*   menuBar;         /* the main menu bar */

   Ne_StatsLine*  statsLineForm;

   Fl_Widget*   splitPane;      /* paned win. for splitting text area */
   Ne_Text_Editor*   textArea;   /* the first text editing area Fl_Widget* */
   Ne_Text_Buffer*  buffer;      /* holds the text being edited */

   Ne_Text_Editor*   textPanes[MAX_PANES];   /* split area additional ones created on demand */
   Ne_Text_Editor*   lastFocus;     /* the last pane to have kbd. focus */

   Fl_Window*  iSearchForm;         /* incremental search line Fl_Widget*s */
   Fl_Button*     iSearchFindButton;
   Fl_Input*      iSearchText;
   Fl_Button*     iSearchClearButton;
   Fl_Check_Button*     iSearchRegexToggle;
   Fl_Check_Button*     iSearchCaseToggle;
   Fl_Check_Button*     iSearchRevToggle;

   Fl_Widget*   tabBar;           /* tab bar for tabbed window */
   Fl_Tabs*   tab;                  /* tab for this document */

   // Replace Dialog
   Fl_Window*   replaceDlog;      /* replace dialog */
   Fl_Input*   replaceText;      /* replace dialog settable Fl_Widget*s... */
   Fl_Input*   replaceWithText;
   Fl_Check_Button*       replaceCaseToggle;
   Fl_Check_Button*   replaceWordToggle;
   Fl_Check_Button*   replaceRegexToggle;
   Fl_Check_Button*   replaceRevToggle;
   Fl_Check_Button*   replaceKeepBtn;
   Fl_Button*   replaceBtn;
   Fl_Button*   replaceAllBtn;
   Fl_Widget*      replaceInWinBtn;
   Fl_Widget*   replaceInSelBtn;
   Fl_Widget*   replaceFindBtn;
   Fl_Widget*   replaceAndFindBtn;

   // Find dialog
   Fl_Window*   findDlog;      /* find dialog */
   Fl_Input*   findText;      /* find dialog settable Fl_Widget*s... */
   Fl_Check_Button*      findCaseToggle;
   Fl_Check_Button*      findWordToggle;
   Fl_Check_Button*      findRegexToggle;
   Fl_Check_Button*   findRevToggle;
   Fl_Check_Button*   findKeepBtn;
   Fl_Button*   findBtn;

   // Replace in multiple files dialog
   Fl_Window*   replaceMultiFileDlog;   /* Replace in multiple files */
   Fl_Browser*   replaceMultiFileList;
   Fl_Check_Button*   replaceMultiFilePathBtn;
   void* fontDialog;                   /* NULL, unless font dialog is up */
   void* colorDialog;                  /* NULL, unless color dialog is up */
   Fl_Widget*   readOnlyItem;           /* menu bar settable Fl_Widget*s... */
   Fl_Widget*   autoSaveItem;
   Fl_Widget*   saveLastItem;
   Fl_Widget*   langModeCascade;
   Fl_Widget*   autoIndentOffItem;
   Fl_Widget*   autoIndentItem;
   Fl_Widget*   smartIndentItem;
   Fl_Widget*     noWrapItem;
   Fl_Widget*     newlineWrapItem;
   Fl_Widget*     continuousWrapItem;
   Fl_Widget*   statsLineItem;
   Fl_Widget*   iSearchLineItem;
   Fl_Widget*   lineNumsItem;
   Fl_Widget*   showMatchingOffItem;
   Fl_Widget*   showMatchingDelimitItem;
   Fl_Widget*   showMatchingRangeItem;
   Fl_Widget*   matchSyntaxBasedItem;
   Fl_Widget*   overtypeModeItem;
   Fl_Widget*   highlightItem;
   Fl_Widget*   windowMenuPane;
   Fl_Widget*   shellMenuPane;
   Fl_Widget*   macroMenuPane;
   Fl_Widget*     bgMenuPane;
   Fl_Widget*     tabMenuPane;
   Fl_Widget*     unloadTagsMenuItem;
   Fl_Widget*     unloadTipsMenuItem;
   Fl_Widget*   filterItem;
   Fl_Widget*   autoSaveDefItem;
   Fl_Widget*   saveLastDefItem;
   Fl_Widget*   noWrapDefItem;
   Fl_Widget*   newlineWrapDefItem;
   Fl_Widget*   contWrapDefItem;
   Fl_Widget*   showMatchingOffDefItem;
   Fl_Widget*   showMatchingDelimitDefItem;
   Fl_Widget*   showMatchingRangeDefItem;
   Fl_Widget*   matchSyntaxBasedDefItem;
   Fl_Widget*   highlightOffDefItem;
   Fl_Widget*   highlightDefItem;
   Fl_Widget*   backlightCharsItem;
   Fl_Widget*   backlightCharsDefItem;
   Fl_Widget*   searchDlogsDefItem;
   Fl_Widget*      beepOnSearchWrapDefItem;
   Fl_Widget*   keepSearchDlogsDefItem;
   Fl_Widget*   searchWrapsDefItem;
   Fl_Widget*      appendLFItem;
   Fl_Widget*   sortOpenPrevDefItem;
   Fl_Widget*   allTagsDefItem;
   Fl_Widget*   smartTagsDefItem;
   Fl_Widget*   reposDlogsDefItem;
   Fl_Widget*      autoScrollDefItem;
   Fl_Widget*   openInTabDefItem;
   Fl_Widget*   tabBarDefItem;
   Fl_Widget*   tabBarHideDefItem;
   Fl_Widget*   toolTipsDefItem;
   Fl_Widget*   tabNavigateDefItem;
   Fl_Widget*      tabSortDefItem;
   Fl_Widget*   statsLineDefItem;
   Fl_Widget*   iSearchLineDefItem;
   Fl_Widget*   lineNumsDefItem;
   Fl_Widget*   pathInWindowsMenuDefItem;
   Fl_Widget*     modWarnDefItem;
   Fl_Widget*     modWarnRealDefItem;
   Fl_Widget*     exitWarnDefItem;
   Fl_Widget*   searchLiteralDefItem;
   Fl_Widget*   searchCaseSenseDefItem;
   Fl_Widget*   searchLiteralWordDefItem;
   Fl_Widget*   searchCaseSenseWordDefItem;
   Fl_Widget*   searchRegexNoCaseDefItem;
   Fl_Widget*   searchRegexDefItem;
   Fl_Widget*   size24x80DefItem;
   Fl_Widget*   size40x80DefItem;
   Fl_Widget*   size60x80DefItem;
   Fl_Widget*   size80x80DefItem;
   Fl_Widget*   sizeCustomDefItem;
   Fl_Widget*   cancelShellItem;
   Fl_Widget*   learnItem;
   Fl_Widget*   finishLearnItem;
   Fl_Widget*   cancelMacroItem;
   Fl_Widget*   replayItem;
   Fl_Widget*   repeatItem;
   Fl_Widget*   splitPaneItem;
   Fl_Widget*   closePaneItem;
   Fl_Widget*   detachDocumentItem;
   Fl_Widget*   moveDocumentItem;
   Fl_Widget*   contextMoveDocumentItem;
   Fl_Widget*   contextDetachDocumentItem;
   Fl_Widget*     bgMenuUndoItem;
   Fl_Widget*     bgMenuRedoItem;
   char   filename[MAXPATHLEN];   /* name component of file being edited*/
   char   path[MAXPATHLEN];   /* path component of file being edited*/
   unsigned   fileMode;      /* permissions of file being edited */
   uid_t   fileUid;       /* last recorded user id of the file */
   gid_t   fileGid;      /* last recorded group id of the file */
   int        fileFormat;           /* whether to save the file straight (Unix format), or convert it to MS DOS style with \r\n line breaks */
   time_t       lastModTime;           /* time of last modification to file */
   dev_t       device;                 /*  device where the file resides */
   ino_t       inode;                  /*  file's inode  */
   UndoInfo*   undo;         /* info for undoing last operation */
   UndoInfo*   redo;         /* info for redoing last undone op */
   int      nPanes;         /* number of additional text editing areas, created by splitWindow */
   int      autoSaveCharCount;   /* count of single characters typed since last backup file generated */
   int      autoSaveOpCount;   /* count of editing operations "" */
   int      undoOpCount;      /* count of stored undo operations */
   int      undoMemUsed;      /* amount of memory (in bytes) dedicated to the undo list */
   char   fontName[MAX_FONT_LEN];   /* names of the text fonts in use */
   char   italicFontName[MAX_FONT_LEN];
   char   boldFontName[MAX_FONT_LEN];
   char   boldItalicFontName[MAX_FONT_LEN];
   Ne_Font fontList;      /* fontList for the primary font */
   Ne_Font italicFontStruct;   /* fontStructs for highlighting fonts */
   Ne_Font boldFontStruct;
   Ne_Font boldItalicFontStruct;
   int flashTimeoutID;   /* timer procedure id for getting rid of highlighted matching paren.  Non- zero val. means highlight is drawn */
   int      flashPos;      /* position saved for erasing matching paren highlight (if one is drawn) */
   int    wasSelected;      /* last selection state (for dim/undim of selection related menu items */
   bool   filenameSet;      /* is the window still "Untitled"? */
   bool   fileChanged;      /* has window been modified? */
   bool     fileMissing;            /* is the window's file gone? */
   int         lockReasons;            /* all ways a file can be locked */
   bool   autoSave;      /* is autosave turned on? */
   bool   saveOldVersion;      /* keep old version in filename.bck */
   char   indentStyle;      /* whether/how to auto indent */
   char   wrapMode;      /* line wrap style: NO_WRAP, NEWLINE_WRAP or CONTINUOUS_WRAP */
   bool   overstrike;      /* is overstrike mode turned on ? */
   char    showMatchingStyle;    /* How to show matching parens: NO_FLASH, FLASH_DELIMIT, or FLASH_RANGE */
   char   matchSyntaxBased;   /* Use syntax info to show matching */
   bool   showStats;      /* is stats line supposed to be shown */
   bool    showISearchLine;       /* is incr. search line to be shown */
   bool    showLineNumbers;       /* is the line number display shown */
   bool   highlightSyntax;   /* is syntax highlighting turned on? */
   bool   backlightChars;      /* is char backlighting turned on? */
   char*   backlightCharTypes;   /* what backlighting to use */
   bool   modeMessageDisplayed;   /* special stats line banner for learn and shell command executing modes */
   char*   modeMessage;      /* stats line banner content for learn and shell command executing modes */
   bool   ignoreModify;      /* ignore modifications to text area */
   bool   windowMenuValid;   /* is window menu up to date? */
   int      rHistIndex, fHistIndex;   /* history placeholders for */
   int        iSearchHistIndex;   /*   find and replace dialogs */
   int        iSearchStartPos;       /* start pos. of current incr. search */
   int          iSearchLastBeginPos;    /* beg. pos. last match of current i.s.*/
   int        nMarks;               /* number of active bookmarks */
   int markTimeoutID;          /* backup timer for mark event handler*/
   Bookmark   markTable[MAX_MARKS];   /* marked locations in window */
   void*       highlightData;    /* info for syntax highlighting */
   void*       shellCmdData;     /* when a shell command is executing, info. about it, otherwise, NULL */
   void*       macroCmdData;     /* same for macro commands */
   void*       smartIndentData;      /* compiled macros for smart indent */
   Atom   fileClosedAtom;         /* Atom used to tell nc that the file is closed */
   int       languageMode;          /* identifies language mode currently
                                      selected in the window */
   bool   multiFileReplSelected;   /* selected during last multi-window replacement operation (history) */
   WindowInfo**   writableWindows;/* temporary list of writable windows used during multi-file replacements */
   int      nWritableWindows;   /* number of elements in the list */
   bool    multiFileBusy;      /* suppresses multiple beeps/dialogs during multi-file replacements */
   bool    replaceFailed;      /* flags replacements failures during multi-file replacements */
   bool   replaceLastRegexCase;   /* last state of the case sense button in regex mode for replace dialog */
   bool   replaceLastLiteralCase; /* idem, for literal mode */
   bool   iSearchLastRegexCase;   /* idem, for regex mode in incremental search bar */
   bool   iSearchLastLiteralCase; /* idem, for literal mode */
   bool   findLastRegexCase;    /* idem, for regex mode in find dialog */
   bool   findLastLiteralCase;    /* idem, for literal mode */

   UserMenuCache*   userMenuCache;     /* cache user menus: */
   UserBGMenuCache  userBGMenuCache;   /* shell & macro menu are shared over all "tabbed" documents, while each document has its own background menu. */
};

class WindowManager;

extern WindowInfo* WindowList;
extern WindowManager windowManager;
extern char* ArgV0;
extern bool IsServer;
extern Ne_AppContext AppContext;

void nextArg(int argc, char** argv, int* argIndex);
int checkDoMacroArg(const char* macro);
int sortAlphabetical(const void* k1, const void* k2);


#endif /* NEDIT_NEDIT_H_INCLUDED */
