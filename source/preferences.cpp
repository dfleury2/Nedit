/*******************************************************************************
*									                                                    *
* preferences.c -- Nirvana Editor preferences processing		                   *
*									                                                    *
* Copyright (C) 1999 Mark Edel						                               *
*									                                                    *
* This is free__ software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version. In addition, you may distribute version of this program linked to   *
* Motif or Open Motif. See README for details.                                 *
* 									                                                    *
* This software is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
* for more details.							                                        *
* 									                                                    *
* You should have received a copy of the GNU General Public License along with *
* software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
* Place, Suite 330, Boston, MA  02111-1307 USA		                            *
*									                                                    *
* Nirvana Text Editor	    						                                  *
* April 20, 1993							                                           *
*									                                                    *
* Written by Mark Edel							                                     *
*									                                                    *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "preferences.h"
#include "Ne_Text_Buffer.h"
#include "nedit.h"
#include "menu.h"
#include "Ne_Text_Editor.h"
#include "search.h"
#include "window.h"
#include "userCmds.h"
#include "highlight.h"
#include "highlightData.h"
#include "help.h"
#include "regularExp.h"
#include "smartIndent.h"
#include "windowTitle.h"
#include "server.h"
#include "tags.h"
#include "../util/prefFile.h"
#include "../util/misc.h"
#include "../util/DialogF.h"
#include "../util/managedList.h"
#include "../util/fontsel.h"
#include "../util/fileUtils.h"
#include "../util/utils.h"
#include "../util/Ne_Color.h"

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Radio_Button.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#ifndef WIN32
#include <sys/param.h>
#include <pwd.h>
#include <unistd.h>
#endif

#define PREF_FILE_VERSION "5.6"

/* New styles added in 5.2 for auto-upgrade */
#define ADD_5_2_STYLES " Pointer:#660000:Bold\nRegex:#009944:Bold\nWarning:brown2:Italic"

/* maximum number of word delimiters allowed (256 allows whole character set) */
#define MAX_WORD_DELIMITERS 256

/* maximum number of file extensions allowed in a language mode */
#define MAX_FILE_EXTENSIONS 20

/* Return values for checkFontStatus */
enum fontStatus {GOOD_FONT, BAD_PRIMARY, BAD_FONT, BAD_SIZE, BAD_SPACING};

/* enumerated type preference strings
** The order of the elements in this array must be exactly the same
** as the order of the corresponding integers of the enum SearchType
** defined in search.h (!!)
*/
static char* SearchMethodStrings[] = { "Literal", "CaseSense", "RegExp", "LiteralWord", "CaseSenseWord", "RegExpNoCase", NULL };

#define N_WRAP_STYLES 3
static char* AutoWrapTypes[N_WRAP_STYLES + 3] = {"None", "Newline", "Continuous", "True", "False", NULL };

#define N_INDENT_STYLES 3
static char* AutoIndentTypes[N_INDENT_STYLES + 3] = {"None", "Auto", "Smart", "True", "False", NULL };

#define N_VIRTKEY_OVERRIDE_MODES 3
static char* VirtKeyOverrideModes[N_VIRTKEY_OVERRIDE_MODES + 1] = { "Never", "Auto", "Always", NULL };

#define N_SHOW_MATCHING_STYLES 3
/* For backward compatibility, "false" and "true" are still accepted.
   They are internally converted to "Off" and "Delimiter" respectively.
   NOTE: N_SHOW_MATCHING_STYLES must correspond to the number of
         _real_ matching styles, not counting false & true.
         false and true should also be the last ones in the list. */
static char* ShowMatchingTypes[] = {"Off", "Delimiter", "Range", "False", "True", NULL };

/*  This array must be kept in parallel to the enum truncSubstitution
    in nedit.h  */
static char* TruncSubstitutionModes[] = {"Silent", "Fail", "Warn", "Ignore", NULL};

/* suplement wrap and indent styles w/ a value meaning "use default" for
   the override fields in the language modes dialog */
#define DEFAULT_WRAP -1
#define DEFAULT_INDENT -1
#define DEFAULT_TAB_DIST -1
#define DEFAULT_EM_TAB_DIST -1

/* list of available language modes and language specific preferences */
static int NLanguageModes = 0;
struct languageModeRec : NamedItem
{
   const char* getName() const
   {
      return name;
   }
   char* name;
   int nExtensions;
   char** extensions;
   char* recognitionExpr;
   char* defTipsFile;
   char* delimiters;
   int wrapStyle;
   int indentStyle;
   int tabDist;
   int emTabDist;
};
static languageModeRec* LanguageModes[MAX_LANGUAGE_MODES];

/* Language mode dialog information */
static struct
{
   Fl_Window* shell;
   Fl_Input* nameW;
   Fl_Input* extW;
   Fl_Input* recogW;
   Fl_Input* defTipsW;
   Fl_Input* delimitW;
   Fl_Browser* managedListW;
   Fl_Int_Input* tabW;
   Fl_Int_Input* emTabW;
   Fl_Radio_Round_Button* defaultIndentW;
   Fl_Radio_Round_Button* noIndentW;
   Fl_Radio_Round_Button* autoIndentW;
   Fl_Radio_Round_Button* smartIndentW;
   Fl_Radio_Round_Button* defaultWrapW;
   Fl_Radio_Round_Button* noWrapW;
   Fl_Radio_Round_Button* newlineWrapW;
   Fl_Radio_Round_Button* contWrapW;
   languageModeRec** languageModeList;
   int nLanguageModes;
} LMDialog = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
              NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0
             };

// Font dialog information
struct fontDialog
{
   Fl_Widget* mainWindow;
   WindowInfo* window;
   Fl_Input* primaryW;
   Fl_Button* fillW;
   Fl_Input* italicW;
   Fl_Box* italicErrW;
   Fl_Input* boldW;
   Fl_Box* boldErrW;
   Fl_Input* boldItalicW;
   Fl_Box* boldItalicErrW;
   Fl_Button* btnApply;
   bool forWindow;
} ;

// Color dialog information
struct colorDialog
{
   Fl_Widget* mainWindow;
   WindowInfo* window;
   Fl_Input* textFgW;
   Fl_Box* textFgErrW;
   Fl_Input* textBgW;
   Fl_Box* textBgErrW;
   Fl_Input* selectFgW;
   Fl_Box* selectFgErrW;
   Fl_Input* selectBgW;
   Fl_Box* selectBgErrW;
   Fl_Input* hiliteFgW;
   Fl_Box* hiliteFgErrW;
   Fl_Input* hiliteBgW;
   Fl_Box* hiliteBgErrW;
   Fl_Input* lineNoFgW;
   Fl_Box* lineNoFgErrW;
   Fl_Input* cursorFgW;
   Fl_Box* cursorFgErrW;
};

/* Repository for simple preferences settings */
static struct prefData
{
   int openInTab;		/* open files in new tabs  */
   int wrapStyle;		/* what kind of wrapping to do */
   int wrapMargin;		/* 0=wrap at window width, other=wrap margin */
   int autoIndent;		/* style for auto-indent */
   int autoSave;		/* whether automatic backup feature is on */
   int saveOldVersion;		/* whether to preserve a copy of last version */
   int searchDlogs;		/* whether to show explanatory search dialogs */
   int searchWrapBeep;     	/* 1=beep when search restarts at begin/end */
   int keepSearchDlogs;	/* whether to retain find and replace dialogs */
   int searchWraps;	/* whether to attempt search again if reach bof or eof */
   int statsLine;		/* whether to show the statistics line */
   int iSearchLine;	    	/* whether to show the incremental search line*/
   int tabBar;			/* whether to show the tab bar */
   int tabBarHideOne;		/* hide tab bar if only one document in window */
   int globalTabNavigate;  	/* prev/next document across windows */
   int toolTips;	    	/* whether to show the tooltips */
   int lineNums;   	    	/* whether to show line numbers */
   int pathInWindowsMenu;   	/* whether to show path in windows menu */
   int warnFileMods;	    	/* warn user if files externally modified */
   int warnRealFileMods;	/* only warn if file contents modified */
   int warnExit;	    	/* whether to warn on exit */
   int searchMethod;		/* initial search method as a text string */
   int textRows;		/* initial window height in characters */
   int textCols;		/* initial window width in characters */
   int tabDist;		/* number of characters between tab stops */
   int emTabDist;		/* non-zero tab dist. if emulated tabs are on */
   int insertTabs;		/* whether to use tabs for padding */
   int showMatchingStyle;	/* how to flash matching parenthesis */
   int matchSyntaxBased;	/* use syntax info to match parenthesis */
   int highlightSyntax;    	/* whether to highlight syntax by default */
   int smartTags;  	    	/* look for tag in current window first */
   int alwaysCheckRelativeTagsSpecs; /* for every new opened file of session */
   int stickyCaseSenseBtn;     /* whether Case Word Btn is sticky to Regex Btn */
   int prefFileRead;	    	/* detects whether a .nedit existed */
   int backlightChars;		/* whether to apply character "backlighting" */
   char* backlightCharTypes;	/* the backlighting color definitions */
   char fontString[MAX_FONT_LEN]; /* names of fonts for text widget */
   char boldFontString[MAX_FONT_LEN];
   char italicFontString[MAX_FONT_LEN];
   char boldItalicFontString[MAX_FONT_LEN];
   Ne_Font fontList; /* NormalFontStruct // ??? XmFontLists corresp. to above named fonts */
   Ne_Font boldFontStruct;
   Ne_Font italicFontStruct;
   Ne_Font boldItalicFontStruct;
   int sortTabs;		/* sort tabs alphabetically */
   int repositionDialogs;	/* w. to reposition dialogs under the pointer */
   int autoScroll;             /* w. to autoscroll near top/bottom of screen */
   int autoScrollVPadding;     /* how close to get before autoscrolling */
   int sortOpenPrevMenu;   	/* whether to sort the "Open Previous" menu */
   int appendLF;       /* Whether to append LF at the end of each file */
   int mapDelete;		/* whether to map delete to backspace */
   int stdOpenDialog;		/* w. to retain redundant text field in Open */
   char tagFile[MAXPATHLEN];	/* name of tags file to look for at startup */
   int maxPrevOpenFiles;   	/* limit to size of Open Previous menu */
   int typingHidesPointer;     /* hide mouse pointer when typing */
   char delimiters[MAX_WORD_DELIMITERS]; /* punctuation characters */
   char shell[MAXPATHLEN + 1]; /* shell to use for executing commands */
   char geometry[MAX_GEOM_STRING_LEN];	/* per-application geometry string, only for the clueless */
   char serverName[MAXPATHLEN];/* server name for multiple servers per disp. */
   char bgMenuBtn[MAX_ACCEL_LEN]; /* X event description for triggering posting of background menu */
   char fileVersion[6]; 	/* Version of nedit which wrote the .nedit file we're reading */
   int findReplaceUsesSelection; /* whether the find replace dialog is automatically loaded with the primary selection */
   int virtKeyOverride;	/* Override Motif default virtual key bindings never, if invalid, or always */
   char titleFormat[MAX_TITLE_FORMAT_LEN];
   char helpFontNames[NUM_HELP_FONTS][MAX_FONT_LEN];/* fonts for help system */
   char helpLinkColor[MAX_COLOR_LEN]; 	/* Color for hyperlinks in the help system */
   char colorNames[NUM_COLORS][MAX_COLOR_LEN];
   char tooltipBgColor[MAX_COLOR_LEN];
   int  undoModifiesSelection;
   int  focusOnRaise;
   bool honorSymlinks;
   int truncSubstitution;
   bool forceOSConversion;
} PrefData;

/* Temporary storage for preferences strings which are discarded after being read */
static struct
{
   char* shellCmds;
   char* macroCmds;
   char* bgMenuCmds;
   char* highlight;
   char* language;
   char* styles;
   char* smartIndent;
   char* smartIndentCommon;
   char* shell;
} TempStringPrefs;

/* preference descriptions for SavePreferences and RestorePreferences. */
static PrefDescripRec PrefDescrip[] =
{
   {
      "fileVersion", PREF_STRING, "", PrefData.fileVersion,
      (void*)sizeof(PrefData.fileVersion), true
   },
#ifndef VMS
#ifdef linux
   {
      "shellCommands", PREF_ALLOC_STRING, "spell:Alt+B:s:EX:\n\
    cat>spellTmp; xterm -e ispell -x spellTmp; cat spellTmp; rm spellTmp\n\
    wc::w:ED:\nwc | awk '{print $1 \" lines, \" $2 \" words, \" $3 \" characters\"}'\n\
    sort::o:EX:\nsort\nnumber lines::n:AW:\nnl -ba\nmake:Alt+Z:m:W:\nmake\n\
    expand::p:EX:\nexpand\nunexpand::u:EX:\nunexpand\n",
      &TempStringPrefs.shellCmds, NULL, true
   },
#elif __FreeBSD__
   {
      "shellCommands", PREF_ALLOC_STRING, "spell:Alt+B:s:EX:\n\
    cat>spellTmp; xterm -e ispell -x spellTmp; cat spellTmp; rm spellTmp\n\
    wc::w:ED:\nwc | awk '{print $2 \" lines, \" $1 \" words, \" $3 \" characters\"}'\n\
    sort::o:EX:\nsort\nnumber lines::n:AW:\npr -tn\nmake:Alt+Z:m:W:\nmake\n\
    expand::p:EX:\nexpand\nunexpand::u:EX:\nunexpand\n",
      &TempStringPrefs.shellCmds, NULL, true
   },
#else
   {
      "shellCommands", PREF_ALLOC_STRING, "spell:Alt+B:s:ED:\n\
    (cat;echo \"\") | spell\nwc::w:ED:\nwc | awk '{print $1 \" lines, \" $2 \" words, \" $3 \" characters\"}'\n\
    \nsort::o:EX:\nsort\nnumber lines::n:AW:\nnl -ba\nmake:Alt+Z:m:W:\nmake\n\
    expand::p:EX:\nexpand\nunexpand::u:EX:\nunexpand\n",
      &TempStringPrefs.shellCmds, NULL, true
   },
#endif /* linux, __FreeBSD__ */
#endif /* VMS */
   {
      "macroCommands", PREF_ALLOC_STRING,
      "Complete Word:Alt+D::: {\n\
		# This macro attempts to complete the current word by\n\
		# finding another word in the same document that has\n\
		# the same prefix; repeated invocations of the macro\n\
		# (by repeated typing of its accelerator, say) cycles\n\
		# through the alternatives found.\n\
		# \n\
		# Make sure $compWord contains something (a dummy index)\n\
		$compWord[\"\"] = \"\"\n\
		\n\
		# Test whether the rest of $compWord has been initialized:\n\
		# this avoids having to initialize the global variable\n\
		# $compWord in an external macro file\n\
		if (!(\"wordEnd\" in $compWord)) {\n\
		    # we need to initialize it\n\
		    $compWord[\"wordEnd\"] = 0\n\
		    $compWord[\"repeat\"] = 0\n\
		    $compWord[\"init\"] = 0\n\
		    $compWord[\"wordStart\"] = 0\n\
		}\n\
		\n\
		if ($compWord[\"wordEnd\"] == $cursor) {\n\
		        $compWord[\"repeat\"] += 1\n\
		}\n\
		else {\n\
		   $compWord[\"repeat\"] = 1\n\
		   $compWord[\"init\"] = $cursor\n\
		\n\
		   # search back to a word boundary to find the word to complete\n\
		   # (we use \\w here to allow for programming \"words\" that can include\n\
		   # digits and underscores; use \\l for letters only)\n\
		   $compWord[\"wordStart\"] = search(\"<\\\\w+\", $cursor, \"backward\", \"regex\", \"wrap\")\n\
		\n\
		   if ($compWord[\"wordStart\"] == -1)\n\
		      return\n\
		\n\
		    if ($search_end == $cursor)\n\
		       $compWord[\"word\"] = get_range($compWord[\"wordStart\"], $cursor)\n\
		    else\n\
		        return\n\
		}\n\
		s = $cursor\n\
		for (i=0; i <= $compWord[\"repeat\"]; i++)\n\
		    s = search($compWord[\"word\"], s - 1, \"backward\", \"regex\", \"wrap\")\n\
		\n\
		if (s == $compWord[\"wordStart\"]) {\n\
		   beep()\n\
		   $compWord[\"repeat\"] = 0\n\
		   s = $compWord[\"wordStart\"]\n\
		   se = $compWord[\"init\"]\n\
		}\n\
		else\n\
		   se = search(\">\", s, \"regex\")\n\
		\n\
		replace_range($compWord[\"wordStart\"], $cursor, get_range(s, se))\n\
		\n\
		$compWord[\"wordEnd\"] = $cursor\n\
	}\n\
	Fill Sel. w/Char:::R: {\n\
		# This macro replaces each character position in\n\
		# the selection with the string typed into the dialog\n\
		# it displays.\n\
		if ($selection_start == -1) {\n\
		    beep()\n\
		    return\n\
		}\n\
		\n\
		# Ask the user what character to fill with\n\
		fillChar = string_dialog(\"Fill selection with what character?\", \\\n\
		                         \"OK\", \"Cancel\")\n\
		if ($string_dialog_button == 2 || $string_dialog_button == 0)\n\
		    return\n\
		\n\
		# Count the number of lines (NL characters) in the selection\n\
		# (by removing all non-NLs in selection and counting the remainder)\n\
		nLines = length(replace_in_string(get_selection(), \\\n\
		                                  \"^.*$\", \"\", \"regex\"))\n\
		\n\
		rectangular = $selection_left != -1\n\
		\n\
		# work out the pieces of required of the replacement text\n\
		# this will be top mid bot where top is empty or ends in NL,\n\
		# mid is 0 or more lines of repeats ending with NL, and\n\
		# bot is 0 or more repeats of the fillChar\n\
		\n\
		toplen = -1 # top piece by default empty (no NL)\n\
		midlen = 0\n\
		botlen = 0\n\
		\n\
		if (rectangular) {\n\
		    # just fill the rectangle:  mid\\n \\ nLines\n\
		    #                           mid\\n /\n\
		    #                           bot   - last line with no nl\n\
		    midlen = $selection_right -  $selection_left\n\
		    botlen = $selection_right -  $selection_left\n\
		} else {\n\
		    #                  |col[0]\n\
		    #         .........toptoptop\\n                      |col[0]\n\
		    # either  midmidmidmidmidmid\\n \\ nLines - 1   or ...botbot...\n\
		    #         midmidmidmidmidmid\\n /                          |col[1]\n\
		    #         botbot...         |\n\
		    #                 |col[1]   |wrap margin\n\
		    # we need column positions col[0], col[1] of selection start and\n\
		    # end (use a loop and arrays to do the two positions)\n\
		    sel[0] = $selection_start\n\
		    sel[1] = $selection_end\n\
		\n\
		    # col[0] = pos_to_column($selection_start)\n\
		    # col[1] = pos_to_column($selection_end)\n\
		\n\
		    for (i = 0; i < 2; ++i) {\n\
		        end = sel[i]\n\
		        pos = search(\"^\", end, \"regex\", \"backward\")\n\
		        thisCol = 0\n\
		        while (pos < end) {\n\
		            nexttab = search(\"\\t\", pos)\n\
		            if (nexttab < 0 || nexttab >= end) {\n\
		                thisCol += end - pos # count remaining non-tabs\n\
		                nexttab = end\n\
		            } else {\n\
		                thisCol += nexttab - pos + $tab_dist\n\
		                thisCol -= (thisCol % $tab_dist)\n\
		            }\n\
		            pos = nexttab + 1 # skip past the tab or end\n\
		        }\n\
		        col[i] = thisCol\n\
		    }\n\
		    toplen = max($wrap_margin - col[0], 0)\n\
		    botlen = min(col[1], $wrap_margin)\n\
		\n\
		    if (nLines == 0) {\n\
		        toplen = -1\n\
		        botlen = max(botlen - col[0], 0)\n\
		    } else {\n\
		        midlen = $wrap_margin\n\
		        if (toplen < 0)\n\
		            toplen = 0\n\
		        nLines-- # top piece will end in a NL\n\
		    }\n\
		}\n\
		\n\
		# Create the fill text\n\
		# which is the longest piece? make a line of that length\n\
		# (use string doubling - this allows the piece to be\n\
		# appended to double in size at each iteration)\n\
		\n\
		len = max(toplen, midlen, botlen)\n\
		charlen = length(fillChar) # maybe more than one char given!\n\
		\n\
		line = \"\"\n\
		while (len > 0) {\n\
		    if (len % 2)\n\
		        line = line fillChar\n\
		    len /= 2\n\
		    if (len > 0)\n\
		        fillChar = fillChar fillChar\n\
		}\n\
		# assemble our pieces\n\
		toppiece = \"\"\n\
		midpiece = \"\"\n\
		botpiece = \"\"\n\
		if (toplen >= 0)\n\
		    toppiece = substring(line, 0, toplen * charlen) \"\\n\"\n\
		if (botlen > 0)\n\
		    botpiece = substring(line, 0, botlen * charlen)\n\
		\n\
		# assemble midpiece (use doubling again)\n\
		line = substring(line, 0, midlen * charlen) \"\\n\"\n\
		while (nLines > 0) {\n\
		    if (nLines % 2)\n\
		        midpiece = midpiece line\n\
		    nLines /= 2\n\
		    if (nLines > 0)\n\
		        line = line line\n\
		}\n\
		# Replace the selection with the complete fill text\n\
		replace_selection(toppiece midpiece botpiece)\n\
	}\n\
	Quote Mail Reply:::: {\n\
		if ($selection_start == -1)\n\
		    replace_all(\"^.*$\", \"\\\\> &\", \"regex\")\n\
		else\n\
		    replace_in_selection(\"^.*$\", \"\\\\> &\", \"regex\")\n\
	}\n\
	Unquote Mail Reply:::: {\n\
		if ($selection_start == -1)\n\
		    replace_all(\"(^\\\\> )(.*)$\", \"\\\\2\", \"regex\")\n\
		else\n\
		    replace_in_selection(\"(^\\\\> )(.*)$\", \"\\\\2\", \"regex\")\n\
	}\n\
	Comments>/* Comment */@C@C++@Java@CSS@JavaScript@Lex:::R: {\n\
		selStart = $selection_start\n\
		selEnd = $selection_end\n\
		replace_range(selStart, selEnd, \"/* \" get_selection() \" */\")\n\
		select(selStart, selEnd + 6)\n\
	}\n\
	Comments>/* Uncomment */@C@C++@Java@CSS@JavaScript@Lex:::R: {\n\
		pos = search(\"(?n\\\\s*/\\\\*\\\\s*)\", $selection_start, \"regex\")\n\
		start = $search_end\n\
		end = search(\"(?n\\\\*/\\\\s*)\", $selection_end, \"regex\", \"backward\")\n\
		if (pos != $selection_start || end == -1 )\n\
		    return\n\
		replace_selection(get_range(start, end))\n\
		select(pos, $cursor)\n\
	}\n\
	Comments>// Comment@C@C++@Java@JavaScript:::R: {\n\
		replace_in_selection(\"^.*$\", \"// &\", \"regex\")\n\
	}\n\
	Comments>// Uncomment@C@C++@Java@JavaScript:::R: {\n\
		replace_in_selection(\"(^[ \\\\t]*// ?)(.*)$\", \"\\\\2\", \"regex\")\n\
	}\n\
	Comments># Comment@Perl@Sh Ksh Bash@NEdit Macro@Makefile@Awk@Csh@Python@Tcl:::R: {\n\
		replace_in_selection(\"^.*$\", \"#&\", \"regex\")\n\
	}\n\
	Comments># Uncomment@Perl@Sh Ksh Bash@NEdit Macro@Makefile@Awk@Csh@Python@Tcl:::R: {\n\
		replace_in_selection(\"(^[ \\\\t]*#)(.*)$\", \"\\\\2\", \"regex\")\n\
	}\n\
	Comments>-- Comment@SQL:::R: {\n\
		replace_in_selection(\"^.*$\", \"--&\", \"regex\")\n\
	}\n\
	Comments>-- Uncomment@SQL:::R: {\n\
		replace_in_selection(\"(^[ \\\\t]*--)(.*)$\", \"\\\\2\", \"regex\")\n\
	}\n\
	Comments>! Comment@X Resources:::R: {\n\
		replace_in_selection(\"^.*$\", \"!&\", \"regex\")\n\
	}\n\
	Comments>! Uncomment@X Resources:::R: {\n\
		replace_in_selection(\"(^[ \\\\t]*!)(.*)$\", \"\\\\2\", \"regex\")\n\
	}\n\
	Comments>% Comment@LaTeX:::R: {\n\
		replace_in_selection(\"^.*$\", \"%&\", \"regex\")\n\
		}\n\
	Comments>% Uncomment@LaTeX:::R: {\n\
		replace_in_selection(\"(^[ \\\\t]*%)(.*)$\", \"\\\\2\", \"regex\")\n\
		}\n\
	Comments>Bar Comment@C:::R: {\n\
		if ($selection_left != -1) {\n\
		    dialog(\"Selection must not be rectangular\")\n\
		    return\n\
		}\n\
		start = $selection_start\n\
		end = $selection_end-1\n\
		origText = get_range($selection_start, $selection_end-1)\n\
		newText = \"/*\\n\" replace_in_string(get_range(start, end), \\\n\
		    \"^\", \" * \", \"regex\") \"\\n */\\n\"\n\
		replace_selection(newText)\n\
		select(start, start + length(newText))\n\
	}\n\
	Comments>Bar Uncomment@C:::R: {\n\
		selStart = $selection_start\n\
		selEnd = $selection_end\n\
		pos = search(\"/\\\\*\\\\s*\\\\n\", selStart, \"regex\")\n\
		if (pos != selStart) return\n\
		start = $search_end\n\
		end = search(\"\\\\n\\\\s*\\\\*/\\\\s*\\\\n?\", selEnd, \"regex\", \"backward\")\n\
		if (end == -1 || $search_end < selEnd) return\n\
		newText = get_range(start, end)\n\
		newText = replace_in_string(newText,\"^ *\\\\* ?\", \"\", \"regex\", \"copy\")\n\
		if (get_range(selEnd, selEnd - 1) == \"\\n\") selEnd -= 1\n\
		replace_range(selStart, selEnd, newText)\n\
		select(selStart, selStart + length(newText))\n\
	}\n\
	Make C Prototypes@C@C++:::: {\n\
		# simplistic extraction of C function prototypes, usually good enough\n\
		if ($selection_start == -1) {\n\
		    start = 0\n\
		    end = $text_length\n\
		} else {\n\
		    start = $selection_start\n\
		    end = $selection_end\n\
		}\n\
		string = get_range(start, end)\n\
		# remove all C++ and C comments, then all blank lines in the extracted range\n\
		string = replace_in_string(string, \"//.*$\", \"\", \"regex\", \"copy\")\n\
		string = replace_in_string(string, \"(?n/\\\\*.*?\\\\*/)\", \"\", \"regex\", \"copy\")\n\
		string = replace_in_string(string, \"^\\\\s*\\n\", \"\", \"regex\", \"copy\")\n\
		nDefs = 0\n\
		searchPos = 0\n\
		prototypes = \"\"\n\
		staticPrototypes = \"\"\n\
		for (;;) {\n\
		    headerStart = search_string(string, \\\n\
		        \"^[a-zA-Z]([^;#\\\"'{}=><!/]|\\n)*\\\\)[ \\t]*\\n?[ \\t]*\\\\{\", \\\n\
		        searchPos, \"regex\")\n\
		    if (headerStart == -1)\n\
		        break\n\
		    headerEnd = search_string(string, \")\", $search_end,\"backward\") + 1\n\
		    prototype = substring(string, headerStart, headerEnd) \";\\n\"\n\
		    if (substring(string, headerStart, headerStart+6) == \"static\")\n\
		        staticPrototypes = staticPrototypes prototype\n\
		    else\n\
		        prototypes = prototypes prototype\n\
		    searchPos = headerEnd\n\
		    nDefs++\n\
		}\n\
		if (nDefs == 0) {\n\
		    dialog(\"No function declarations found\")\n\
		    return\n\
		}\n\
		new()\n\
		focus_window(\"last\")\n\
		replace_range(0, 0, prototypes staticPrototypes)\n\
	}", &TempStringPrefs.macroCmds, NULL, true
   },
   {
      "bgMenuCommands", PREF_ALLOC_STRING,
      "Undo:::: {\nundo()\n}\n\
	Redo:::: {\nredo()\n}\n\
	Cut:::R: {\ncut_clipboard()\n}\n\
	Copy:::R: {\ncopy_clipboard()\n}\n\
	Paste:::: {\npaste_clipboard()\n}", &TempStringPrefs.bgMenuCmds,
      NULL, true
   },
   {
      "highlightPatterns", PREF_ALLOC_STRING,
      "Ada:Default\n\
        Awk:Default\n\
        C++:Default\n\
        C:Default\n\
        CSS:Default\n\
        Csh:Default\n\
        Fortran:Default\n\
        Java:Default\n\
        JavaScript:Default\n\
        LaTeX:Default\n\
        Lex:Default\n\
        Makefile:Default\n\
        Matlab:Default\n\
        NEdit Macro:Default\n\
        Pascal:Default\n\
        Perl:Default\n\
        PostScript:Default\n\
        Python:Default\n\
        Regex:Default\n\
        SGML HTML:Default\n\
        SQL:Default\n\
        Sh Ksh Bash:Default\n\
        Tcl:Default\n\
        VHDL:Default\n\
        Verilog:Default\n\
        XML:Default\n\
        X Resources:Default\n\
        Yacc:Default",
      &TempStringPrefs.highlight, NULL, true
   },
   {
      "languageModes", PREF_ALLOC_STRING,
       "Ada:.ada .ad .ads .adb .a:::::::\n\
        Awk:.awk:::::::\n\
        C++:.cc .hh .C .H .i .cxx .hxx .cpp .c++::::::\".,/\\`'!|@#%^&*()-=+{}[]\"\":;<>?~\":\n\
        C:.c .h::::::\".,/\\`'!|@#%^&*()-=+{}[]\"\":;<>?~\":\n\
        CSS:css::Auto:None:::\".,/\\`'!|@#%^&*()=+{}[]\"\":;<>?~\":\n\
        Csh:.csh .cshrc .tcshrc .login .logout:\"^[ \\t]*#[ \\t]*![ \\t]*/bin/t?csh\"::::::\n\
        Fortran:.f .f77 .for:::::::\n\
        Java:.java:::::::\n\
        JavaScript:.js:::::::\n\
        LaTeX:.tex .sty .cls .ltx .ins .clo .fd:::::::\n\
        Lex:.lex:::::::\n\
        Makefile:Makefile makefile .gmk:::None:8:8::\n\
        Matlab:.m .oct .sci:::::::\n\
        NEdit Macro:.nm .neditmacro:::::::\n\
        Pascal:.pas .p .int:::::::\n\
        Perl:.pl .pm .p5 .PL:\"^[ \\t]*#[ \\t]*!.*perl\":Auto:None:::\".,/\\\\`'!$@#%^&*()-=+{}[]\"\":;<>?~|\":\n\
        PostScript:.ps .eps .epsf .epsi:\"^%!\":::::\"/%(){}[]<>\":\n\
        Python:.py:\"^#!.*python\":Auto:None:::\"!\"\"#$%&'()*+,-./:;<=>?@[\\\\]^`{|}~\":\n\
        Regex:.reg .regex:\"\\(\\?[:#=!iInN].+\\)\":None:Continuous::::\n\
        SGML HTML:.sgml .sgm .html .htm:\"\\<[Hh][Tt][Mm][Ll]\\>\"::::::\n\
        SQL:.sql:::::::\n\
        Sh Ksh Bash:.sh .bash .ksh .profile .bashrc .bash_logout .bash_login .bash_profile:\"^[ \\t]*#[ \\t]*![ \\t]*/.*bin/(bash|ksh|sh|zsh)\"::::::\n\
        Tcl:.tcl .tk .itcl .itk::Smart:None::::\n\
        VHDL:.vhd .vhdl .vdl:::::::\n\
        Verilog:.v:::::::\n\
        XML:.xml .xsl .dtd:\"\\<(?i\\?xml|!doctype)\"::None:::\"<>/=\"\"'()+*?|\":\n\
        X Resources:.Xresources .Xdefaults .nedit .pats nedit.rc:\"^[!#].*([Aa]pp|[Xx]).*[Dd]efaults\"::::::\n\
        Yacc:.y::::::\".,/\\`'!|@#%^&*()-=+{}[]\"\":;<>?~\":",
      &TempStringPrefs.language, NULL, true
   },
   {
      "styles", PREF_ALLOC_STRING, "Plain:black:Plain\n\
    	Comment:gray20:Italic\n\
    	Keyword:black:Bold\n\
        Operator:dark blue:Bold\n\
        Bracket:dark blue:Bold\n\
    	Storage Type:brown:Bold\n\
    	Storage Type1:saddle brown:Bold\n\
    	String:darkGreen:Plain\n\
    	String1:SeaGreen:Plain\n\
    	String2:darkGreen:Bold\n\
    	Preprocessor:RoyalBlue4:Plain\n\
    	Preprocessor1:blue:Plain\n\
    	Character Const:darkGreen:Plain\n\
    	Numeric Const:darkGreen:Plain\n\
    	Identifier:brown:Plain\n\
    	Identifier1:RoyalBlue4:Plain\n\
        Identifier2:SteelBlue:Plain\n\
 	Subroutine:brown:Plain\n\
	Subroutine1:chocolate:Plain\n\
   	Ada Attributes:plum:Bold\n\
	Label:red:Italic\n\
	Flag:red:Bold\n\
    	Text Comment:SteelBlue4:Italic\n\
    	Text Key:VioletRed4:Bold\n\
	Text Key1:VioletRed4:Plain\n\
    	Text Arg:RoyalBlue4:Bold\n\
    	Text Arg1:SteelBlue4:Bold\n\
	Text Arg2:RoyalBlue4:Plain\n\
    	Text Escape:gray30:Bold\n\
	LaTeX Math:darkGreen:Plain\n"
      ADD_5_2_STYLES,
      &TempStringPrefs.styles, NULL, true
   },
   {
      "smartIndentInit", PREF_ALLOC_STRING,
      "C:Default\n\
	C++:Default\n\
	Python:Default\n\
	Matlab:Default", &TempStringPrefs.smartIndent, NULL, true
   },
   {
      "smartIndentInitCommon", PREF_ALLOC_STRING,
      "Default", &TempStringPrefs.smartIndentCommon, NULL, true
   },
   {
      "autoWrap", PREF_ENUM, "Continuous",
      &PrefData.wrapStyle, AutoWrapTypes, true
   },
   {
      "wrapMargin", PREF_INT, "0",
      &PrefData.wrapMargin, NULL, true
   },
   {
      "autoIndent", PREF_ENUM, "Auto",
      &PrefData.autoIndent, AutoIndentTypes, true
   },
   {
      "autoSave", PREF_BOOLEAN, "True",
      &PrefData.autoSave, NULL, true
   },
   {
      "openInTab", PREF_BOOLEAN, "True",
      &PrefData.openInTab, NULL, true
   },
   {
      "saveOldVersion", PREF_BOOLEAN, "False",
      &PrefData.saveOldVersion, NULL, true
   },
   {
      "showMatching", PREF_ENUM, "Delimiter",
      &PrefData.showMatchingStyle, ShowMatchingTypes, true
   },
   {
      "matchSyntaxBased", PREF_BOOLEAN, "True",
      &PrefData.matchSyntaxBased, NULL, true
   },
   {
      "highlightSyntax", PREF_BOOLEAN, "True",
      &PrefData.highlightSyntax, NULL, true
   },
   {
      "backlightChars", PREF_BOOLEAN, "False",
      &PrefData.backlightChars, NULL, true
   },
   {
      "backlightCharTypes", PREF_ALLOC_STRING,
      "0-8,10-31,127:red;9:grey87;32,160-255:grey94;128-159:orange",
      &PrefData.backlightCharTypes, NULL, false
   },
   {
      "searchDialogs", PREF_BOOLEAN, "False",
      &PrefData.searchDlogs, NULL, true
   },
   {
      "beepOnSearchWrap", PREF_BOOLEAN, "False",
      &PrefData.searchWrapBeep, NULL, true
   },
   {
      "retainSearchDialogs", PREF_BOOLEAN, "False",
      &PrefData.keepSearchDlogs, NULL, true
   },
   {
      "searchWraps", PREF_BOOLEAN, "True",
      &PrefData.searchWraps, NULL, true
   },
   {
      "stickyCaseSenseButton", PREF_BOOLEAN, "True",
      &PrefData.stickyCaseSenseBtn, NULL, true
   },
   {
      "repositionDialogs", PREF_BOOLEAN, "True",
      &PrefData.repositionDialogs, NULL, true
   },
   {
      "autoScroll", PREF_BOOLEAN, "False",
      &PrefData.autoScroll, NULL, true
   },
   {
      "autoScrollVPadding", PREF_INT, "4",
      &PrefData.autoScrollVPadding, NULL, false
   },
   {
      "appendLF", PREF_BOOLEAN, "True",
      &PrefData.appendLF, NULL, true
   },
   {
      "sortOpenPrevMenu", PREF_BOOLEAN, "True",
      &PrefData.sortOpenPrevMenu, NULL, true
   },
   {
      "statisticsLine", PREF_BOOLEAN, "False",
      &PrefData.statsLine, NULL, true
   },
   {
      "iSearchLine", PREF_BOOLEAN, "False",
      &PrefData.iSearchLine, NULL, true
   },
   {
      "sortTabs", PREF_BOOLEAN, "False",
      &PrefData.sortTabs, NULL, true
   },
   {
      "tabBar", PREF_BOOLEAN, "True",
      &PrefData.tabBar, NULL, true
   },
   {
      "tabBarHideOne", PREF_BOOLEAN, "True",
      &PrefData.tabBarHideOne, NULL, true
   },
   {
      "toolTips", PREF_BOOLEAN, "True",
      &PrefData.toolTips, NULL, true
   },
   {
      "globalTabNavigate", PREF_BOOLEAN, "False",
      &PrefData.globalTabNavigate, NULL, true
   },
   {
      "lineNumbers", PREF_BOOLEAN, "False",
      &PrefData.lineNums, NULL, true
   },
   {
      "pathInWindowsMenu", PREF_BOOLEAN, "True",
      &PrefData.pathInWindowsMenu, NULL, true
   },
   {
      "warnFileMods", PREF_BOOLEAN, "True",
      &PrefData.warnFileMods, NULL, true
   },
   {
      "warnRealFileMods", PREF_BOOLEAN, "True",
      &PrefData.warnRealFileMods, NULL, true
   },
   {
      "warnExit", PREF_BOOLEAN, "True",
      &PrefData.warnExit, NULL, true
   },
   {
      "searchMethod", PREF_ENUM, "Literal",
      &PrefData.searchMethod, SearchMethodStrings, true
   },
   {
      "textRows", PREF_INT, "24",
      &PrefData.textRows, NULL, true
   },
   {
      "textCols", PREF_INT, "80",
      &PrefData.textCols, NULL, true
   },
   {
      "tabDistance", PREF_INT, "8",
      &PrefData.tabDist, NULL, true
   },
   {
      "emulateTabs", PREF_INT, "0",
      &PrefData.emTabDist, NULL, true
   },
   {
      "insertTabs", PREF_BOOLEAN, "True",
      &PrefData.insertTabs, NULL, true
   },
#ifdef WIN32
      { "textFont", PREF_STRING, "Courier New 16", PrefData.fontString, (void*)sizeof(PrefData.fontString), true },
      { "boldHighlightFont", PREF_STRING, "Courier New bold 16", PrefData.boldFontString, (void*)sizeof(PrefData.boldFontString), true },
      { "italicHighlightFont", PREF_STRING, "Courier New italic 16", PrefData.italicFontString, (void*)sizeof(PrefData.italicFontString), true },
      { "boldItalicHighlightFont", PREF_STRING, "Courier New bold italic 16", PrefData.boldItalicFontString, (void*)sizeof(PrefData.boldItalicFontString), true },
#else
      { "textFont", PREF_STRING, "courier 18", PrefData.fontString, (void*)sizeof(PrefData.fontString), true },
      { "boldHighlightFont", PREF_STRING, "courier bold 18", PrefData.boldFontString, (void*)sizeof(PrefData.boldFontString), true },
      { "italicHighlightFont", PREF_STRING, "courier italic 18", PrefData.italicFontString, (void*)sizeof(PrefData.italicFontString), true },
      { "boldItalicHighlightFont", PREF_STRING, "courier bold italic 18", PrefData.boldItalicFontString, (void*)sizeof(PrefData.boldItalicFontString), true },
   //{ "textFont", PREF_STRING, "-*-courier-medium-r-normal--*-120-*-*-*-iso8859-1", PrefData.fontString, (void*)sizeof(PrefData.fontString), true },
   //{ "boldHighlightFont", PREF_STRING, "-*-courier-bold-r-normal--*-120-*-*-*-iso8859-1", PrefData.boldFontString, (void*)sizeof(PrefData.boldFontString), true },
   //{ "italicHighlightFont", PREF_STRING, "-*-courier-medium-o-normal--*-120-*-*-*-iso8859-1", PrefData.italicFontString, (void*)sizeof(PrefData.italicFontString), true },
   //{ "boldItalicHighlightFont", PREF_STRING, "-*-courier-bold-o-normal--*-120-*-*-*-iso8859-1", PrefData.boldItalicFontString, (void*)sizeof(PrefData.boldItalicFontString), true },
#endif

   {
      "helpFont", PREF_STRING,
      "-*-helvetica-medium-r-normal--*-120-*-*-*-iso8859-1",
      PrefData.helpFontNames[HELP_FONT],
      (void*)sizeof(PrefData.helpFontNames[HELP_FONT]), false
   },
   {
      "boldHelpFont", PREF_STRING,
      "-*-helvetica-bold-r-normal--*-120-*-*-*-iso8859-1",
      PrefData.helpFontNames[BOLD_HELP_FONT],
      (void*)sizeof(PrefData.helpFontNames[BOLD_HELP_FONT]), false
   },
   {
      "italicHelpFont", PREF_STRING,
      "-*-helvetica-medium-o-normal--*-120-*-*-*-iso8859-1",
      PrefData.helpFontNames[ITALIC_HELP_FONT],
      (void*)sizeof(PrefData.helpFontNames[ITALIC_HELP_FONT]), false
   },
   {
      "boldItalicHelpFont", PREF_STRING,
      "-*-helvetica-bold-o-normal--*-120-*-*-*-iso8859-1",
      PrefData.helpFontNames[BOLD_ITALIC_HELP_FONT],
      (void*)sizeof(PrefData.helpFontNames[BOLD_ITALIC_HELP_FONT]), false
   },
   {
      "fixedHelpFont", PREF_STRING,
      "-*-courier-medium-r-normal--*-120-*-*-*-iso8859-1",
      PrefData.helpFontNames[FIXED_HELP_FONT],
      (void*)sizeof(PrefData.helpFontNames[FIXED_HELP_FONT]), false
   },
   {
      "boldFixedHelpFont", PREF_STRING,
      "-*-courier-bold-r-normal--*-120-*-*-*-iso8859-1",
      PrefData.helpFontNames[BOLD_FIXED_HELP_FONT],
      (void*)sizeof(PrefData.helpFontNames[BOLD_FIXED_HELP_FONT]), false
   },
   {
      "italicFixedHelpFont", PREF_STRING,
      "-*-courier-medium-o-normal--*-120-*-*-*-iso8859-1",
      PrefData.helpFontNames[ITALIC_FIXED_HELP_FONT],
      (void*)sizeof(PrefData.helpFontNames[ITALIC_FIXED_HELP_FONT]), false
   },
   {
      "boldItalicFixedHelpFont", PREF_STRING,
      "-*-courier-bold-o-normal--*-120-*-*-*-iso8859-1",
      PrefData.helpFontNames[BOLD_ITALIC_FIXED_HELP_FONT],
      (void*)sizeof(PrefData.helpFontNames[BOLD_ITALIC_FIXED_HELP_FONT]), false
   },
   {
      "helpLinkFont", PREF_STRING,
      "-*-helvetica-medium-r-normal--*-120-*-*-*-iso8859-1",
      PrefData.helpFontNames[HELP_LINK_FONT],
      (void*)sizeof(PrefData.helpFontNames[HELP_LINK_FONT]), false
   },
   {
      "h1HelpFont", PREF_STRING,
      "-*-helvetica-bold-r-normal--*-140-*-*-*-iso8859-1",
      PrefData.helpFontNames[H1_HELP_FONT],
      (void*)sizeof(PrefData.helpFontNames[H1_HELP_FONT]), false
   },
   {
      "h2HelpFont", PREF_STRING,
      "-*-helvetica-bold-o-normal--*-120-*-*-*-iso8859-1",
      PrefData.helpFontNames[H2_HELP_FONT],
      (void*)sizeof(PrefData.helpFontNames[H2_HELP_FONT]), false
   },
   {
      "h3HelpFont", PREF_STRING,
      "-*-courier-bold-r-normal--*-120-*-*-*-iso8859-1",
      PrefData.helpFontNames[H3_HELP_FONT],
      (void*)sizeof(PrefData.helpFontNames[H3_HELP_FONT]), false
   },
   {
      "helpLinkColor", PREF_STRING, "#009900",
      PrefData.helpLinkColor,
      (void*)sizeof(PrefData.helpLinkColor), false
   },

   {
      "textFgColor", PREF_STRING, NEDIT_DEFAULT_FG,
      PrefData.colorNames[TEXT_FG_COLOR],
      (void*)sizeof(PrefData.colorNames[TEXT_FG_COLOR]), true
   },
   {
      "textBgColor", PREF_STRING, NEDIT_DEFAULT_TEXT_BG,
      PrefData.colorNames[TEXT_BG_COLOR],
      (void*)sizeof(PrefData.colorNames[TEXT_BG_COLOR]), true
   },
   {
      "selectFgColor", PREF_STRING, NEDIT_DEFAULT_SEL_FG,
      PrefData.colorNames[SELECT_FG_COLOR],
      (void*)sizeof(PrefData.colorNames[SELECT_FG_COLOR]), true
   },
   {
      "selectBgColor", PREF_STRING, NEDIT_DEFAULT_SEL_BG,
      PrefData.colorNames[SELECT_BG_COLOR],
      (void*)sizeof(PrefData.colorNames[SELECT_BG_COLOR]), true
   },
   {
      "hiliteFgColor", PREF_STRING, NEDIT_DEFAULT_HI_FG,
      PrefData.colorNames[HILITE_FG_COLOR],
      (void*)sizeof(PrefData.colorNames[HILITE_FG_COLOR]), true
   },
   {
      "hiliteBgColor", PREF_STRING, NEDIT_DEFAULT_HI_BG,
      PrefData.colorNames[HILITE_BG_COLOR],
      (void*)sizeof(PrefData.colorNames[HILITE_BG_COLOR]), true
   },
   {
      "lineNoFgColor", PREF_STRING, NEDIT_DEFAULT_LINENO_FG,
      PrefData.colorNames[LINENO_FG_COLOR],
      (void*)sizeof(PrefData.colorNames[LINENO_FG_COLOR]), true
   },
   {
      "cursorFgColor", PREF_STRING, NEDIT_DEFAULT_CURSOR_FG,
      PrefData.colorNames[CURSOR_FG_COLOR],
      (void*)sizeof(PrefData.colorNames[CURSOR_FG_COLOR]), true
   },
   {
      "tooltipBgColor", PREF_STRING, "LemonChiffon1",
      PrefData.tooltipBgColor,
      (void*)sizeof(PrefData.tooltipBgColor), false
   },
   {
      "shell", PREF_STRING, "DEFAULT", PrefData.shell,
      (void*) sizeof(PrefData.shell), true
   },
   {
      "geometry", PREF_STRING, "",
      PrefData.geometry, (void*)sizeof(PrefData.geometry), false
   },
   {
      "remapDeleteKey", PREF_BOOLEAN, "False",
      &PrefData.mapDelete, NULL, false
   },
   {
      "stdOpenDialog", PREF_BOOLEAN, "False",
      &PrefData.stdOpenDialog, NULL, false
   },
   {
      "tagFile", PREF_STRING,
      "", PrefData.tagFile, (void*)sizeof(PrefData.tagFile), false
   },
   {
      "wordDelimiters", PREF_STRING,
      ".,/\\`'!|@#%^&*()-=+{}[]\":;<>?",
      PrefData.delimiters, (void*)sizeof(PrefData.delimiters), false
   },
   {
      "serverName", PREF_STRING, "", PrefData.serverName,
      (void*)sizeof(PrefData.serverName), false
   },
   {
      "maxPrevOpenFiles", PREF_INT, "30",
      &PrefData.maxPrevOpenFiles, NULL, false
   },
   {
      "bgMenuButton", PREF_STRING,
      "~Shift~Ctrl~Meta~Alt<Btn3Down>", PrefData.bgMenuBtn,
      (void*)sizeof(PrefData.bgMenuBtn), false
   },
   {
      "smartTags", PREF_BOOLEAN, "True",
      &PrefData.smartTags, NULL, true
   },
   {
      "typingHidesPointer", PREF_BOOLEAN, "False",
      &PrefData.typingHidesPointer, NULL, false
   },
   {
      "alwaysCheckRelativeTagsSpecs",
      PREF_BOOLEAN, "True", &PrefData.alwaysCheckRelativeTagsSpecs, NULL, false
   },
   {
      "prefFileRead", PREF_BOOLEAN, "False",
      &PrefData.prefFileRead, NULL, true
   },
   {
      "findReplaceUsesSelection", PREF_BOOLEAN, "False",
      &PrefData.findReplaceUsesSelection, NULL, false
   },
   {
      "overrideDefaultVirtualKeyBindings",
      PREF_ENUM, "Auto", &PrefData.virtKeyOverride, VirtKeyOverrideModes, false
   },
   {
      "titleFormat", PREF_STRING, "[%s] %f (%S) - %d",
      PrefData.titleFormat, (void*)sizeof(PrefData.titleFormat), true
   },
   {
      "undoModifiesSelection", PREF_BOOLEAN,
      "true", &PrefData.undoModifiesSelection, NULL, false
   },
   {
      "focusOnRaise", PREF_BOOLEAN,
      "false", &PrefData.focusOnRaise, NULL, false
   },
   {
      "forceOSConversion", PREF_BOOLEAN, "True",
      &PrefData.forceOSConversion, NULL, false
   },
   {
      "truncSubstitution", PREF_ENUM, "Fail",
      &PrefData.truncSubstitution, TruncSubstitutionModes, false
   },
   {
      "honorSymlinks", PREF_BOOLEAN, "True",
      &PrefData.honorSymlinks, NULL, false
   }
};

static const char HeaderText[] = "\
! Preferences file for NEdit\n\
! (User settings in X \"application defaults\" format)\n\
!\n\
! This file is overwritten by the \"Save Defaults...\" command in NEdit\n\
! and serves only the interactively settable options presented in the NEdit\n\
! \"Preferences\" menu.  To modify other options, such as key bindings, use\n\
! the .Xdefaults file in your home directory (or the X resource\n\
! specification method appropriate to your system).  The contents of this\n\
! file can be moved into an X resource file, but since resources in this file\n\
! override their corresponding X resources, either this file should be \n\
! deleted or individual resource lines in the file should be deleted for the\n\
! moved lines to take effect.\n";

/* Module-global variable set when any preference changes (for asking the user about re-saving on exit) */
static int PrefsHaveChanged = false;

/* Module-global variable set when user uses -import to load additional preferences on top of the defaults.  Contains name of file loaded */
static char* ImportedFile = NULL;

/* Module-global variables to support Initial Window Size... dialog */
static Fl_Int_Input* RowText, *ColText;

static void translatePrefFormats(int convertOld, int fileVer);
static void setIntPref(int* prefDataField, int newValue);
static void setStringPref(char* prefDataField, const char* newValue);
static void sizeOKCB(Fl_Widget* w, void* data);
static void setStringAllocPref(char** pprefDataField, char* newValue);
static void sizeCancelCB(Fl_Widget* w, void* data);
static void reapplyLanguageMode(WindowInfo* window, int mode, int forceDefaults);

static void fillFromPrimaryCB(Fl_Widget* w, void* data);
static int checkFontStatus(fontDialog* fd, Fl_Input* fontTextFieldW);
static int showFontStatus(fontDialog* fd, Fl_Input* fontTextFieldW, Fl_Box* errorLabelW);
static void primaryModifiedCB(Fl_Widget* w, void* data);
static void italicModifiedCB(Fl_Widget* w, void* data);
static void boldModifiedCB(Fl_Widget* w, void* data);
static void boldItalicModifiedCB(Fl_Widget* w, void* data);
static void primaryBrowseCB(Fl_Widget* w, void* data);
static void italicBrowseCB(Fl_Widget* w, void* data);
static void boldBrowseCB(Fl_Widget* w, void* data);
static void boldItalicBrowseCB(Fl_Widget* w, void* data);
static void browseFont(Fl_Widget* parent, Fl_Input* fontTextW);
static void fontDestroyCB(Fl_Widget* w, void* data);
static void fontOkCB(Fl_Widget* w, void* data);
static void fontApplyCB(Fl_Widget* w, void* data);
static void fontCancelCB(Fl_Widget* w, void* data);
static void updateFonts(fontDialog* fd);

static bool checkColorStatus(colorDialog* cd, Fl_Input* colorFieldW);
static int verifyAllColors(colorDialog* cd);
static void showColorStatus(colorDialog* cd, Fl_Input* colorFieldW, Fl_Widget* errorLabelW);
static void updateColors(colorDialog* cd);
static void colorDestroyCB(Fl_Widget* w, void* data);
static void colorOkCB(Fl_Widget* w, void* data);
static void colorApplyCB(Fl_Widget* w, void* data);
static void colorCloseCB(Fl_Widget* w, void* data);
static void textFgModifiedCB(Fl_Widget* w, void* data);
static void textBgModifiedCB(Fl_Widget* w, void* data);
static void selectFgModifiedCB(Fl_Widget* w, void* data);
static void selectBgModifiedCB(Fl_Widget* w, void* data);
static void hiliteFgModifiedCB(Fl_Widget* w, void* data);
static void hiliteBgModifiedCB(Fl_Widget* w, void* data);
static void lineNoFgModifiedCB(Fl_Widget* w, void* data);
static void cursorFgModifiedCB(Fl_Widget* w, void* data);

static int matchLanguageMode(WindowInfo* window);
static int loadLanguageModesString(char* inString, int fileVer);
static char* writeLanguageModesString();
static char* createExtString(char** extensions, int nExtensions);
static char** readExtensionList(char** inPtr, int* nExtensions);
static void updateLanguageModeSubmenu(WindowInfo* window);
static void setLangModeCB(Fl_Widget* w, void* data);
static int modeError(languageModeRec* lm, const char* stringStart, const char* stoppedAt, const char* message);
static void lmDestroyCB(Fl_Widget* w, void* data);
static void lmOkCB(Fl_Widget* w, void* data);
static void lmApplyCB(Fl_Widget* w, void* data);
static void lmCloseCB(Fl_Widget* w, void* data);
static int lmDeleteConfirmCB(int itemIndex, void* cbArg);
static int updateLMList();
static languageModeRec* copyLanguageModeRec(languageModeRec* lm);
static void* lmGetDisplayedCB(void* oldItem, int explicitRequest, int* abort, void* cbArg);
static void lmSetDisplayedCB(void* item, void* cbArg);
static languageModeRec* readLMDialogFields(int silent);
static void lmFreeItemCB(void* item);
static void freeLanguageModeRec(languageModeRec* lm);
static int lmDialogEmpty();
static void updatePatternsTo5dot1();
static void updatePatternsTo5dot2();
static void updatePatternsTo5dot3();
static void updatePatternsTo5dot4();
static void updateShellCmdsTo5dot3();
static void updateShellCmdsTo5dot4();
static void updateMacroCmdsTo5dot5();
static void updatePatternsTo5dot6();
static void updateMacroCmdsTo5dot6();
static void migrateColorResources(Ne_Database* prefDB, Ne_Database* appDB);
static void spliceString(char** intoString, const char* insertString, const char* atExpr);
static int regexFind(const char* inString, const char* expr);
static int regexReplace(char** inString, const char* expr,  const char* replaceWith);
static int caseFind(const char* inString, const char* expr);
static int caseReplace(char** inString, const char* expr, const char* replaceWith, int replaceLen);
static int stringReplace(char** inString, const char* expr, const char* replaceWith, int searchType, int replaceLen);
static int replaceMacroIfUnchanged(const char* oldText, const char* newStart, const char* newEnd);
static const char* getDefaultShell();

Ne_Database CreateNEditPrefDB(int* argcInOut, char** argvInOut)
{
   return CreatePreferencesDatabase(GetRCFileName(NEDIT_RC), APP_NAME);
}

void RestoreNEditPrefs(Ne_Database* prefDB, Ne_Database* appDB)
{
   int requiresConversion;
   int major;              /* The integral part of version number */
   int minor;              /* fractional part of version number */
   int fileVer = 0;        /* Both combined into an integer */
   int nparsed;

   // Load preferences
   RestorePreferences(prefDB, appDB, APP_NAME, PrefDescrip, ARRAY_SIZE(PrefDescrip));

   // If the preferences file was written by an older version of NEdit,
   // warn the user that it will be converted.
   requiresConversion = PrefData.prefFileRead && PrefData.fileVersion[0] == '\0';
   if (requiresConversion)
   {
      updatePatternsTo5dot1();
   }

   if (PrefData.prefFileRead)
   {
      if (PrefData.fileVersion[0] == '\0')
      {
         fileVer = 0;    /* Pre-5.1 */
      }
      else
      {
         // Note: do not change the format of this.  Older executables
         // need to read this field for forward compatability.
         nparsed = sscanf(PrefData.fileVersion, "%d.%d", &major, &minor);
         if (nparsed >= 2)
         {
            // Use OSF-style numbering scheme
            fileVer = major * 1000 + minor;
         }
      }
   }

   if (PrefData.prefFileRead && fileVer < 5002)
   {
      updatePatternsTo5dot2();
   }

   if (PrefData.prefFileRead && fileVer < 5003)
   {
      updateShellCmdsTo5dot3();
      updatePatternsTo5dot3();
   }

   /* Note that we don't care about unreleased file versions.  Anyone
      who is running a CVS or alpha version of NEdit is resposnbile
      for managing the preferences file themselves.  Otherwise, it
      gets impossible to track the number of "in-between" file formats.
      We only do auto-upgrading for a real release. */

   if (PrefData.prefFileRead && (fileVer < 5004))
   {
      migrateColorResources(prefDB, appDB);
      updateShellCmdsTo5dot4();
      updatePatternsTo5dot4();
   }
   if (PrefData.prefFileRead && (fileVer < 5005))
   {
      updateMacroCmdsTo5dot5();
   }
   if (PrefData.prefFileRead && (fileVer < 5006))
   {
      fprintf(stderr, "NEdit: Converting .nedit file to 5.6 version.\n"
              "    To keep, use Preferences -> Save Defaults\n");
      updateMacroCmdsTo5dot6();
      updatePatternsTo5dot6();
   }
   // Migrate colors if there's no config file yet
   if (!PrefData.prefFileRead)
   {
      migrateColorResources(prefDB, appDB);
   }

   /* Do further parsing on resource types which RestorePreferences does
      not understand and reads as strings, to put them in the final form
      in which nedit stores and uses.  If the preferences file was
      written by an older version of NEdit, update regular expressions in
      highlight patterns to quote braces and use & instead of \0 */
   translatePrefFormats(requiresConversion, fileVer);
}

// --------------------------------------------------------------------------
// Many of of NEdit's preferences are much more complicated than just simple
// integers or strings.  These are read as strings, but must be parsed and
// translated into something meaningful.  This routine does the translation,
// and, in most cases, frees the original string, which is no longer useful.
//
// In addition this function covers settings that, while simple, require
// additional steps before they can be published.
//
// The argument convertOld attempts a conversion from pre 5.1 format .nedit
// files (which means patterns and macros may contain regular expressions
// which are of the older syntax where braces were not quoted, and \0 was a
// legal substitution character).  Macros, so far can not be automatically
// converted, unfortunately.
// --------------------------------------------------------------------------
static void translatePrefFormats(int convertOld, int fileVer)
{
   // Parse the strings which represent types which are not decoded by
   // the standard resource manager routines
   if (TempStringPrefs.shellCmds != NULL)
   {
      LoadShellCmdsString(TempStringPrefs.shellCmds);
      delete[] TempStringPrefs.shellCmds;
      TempStringPrefs.shellCmds = NULL;
   }
   if (TempStringPrefs.macroCmds != NULL)
   {
      LoadMacroCmdsString(TempStringPrefs.macroCmds);
      delete[] TempStringPrefs.macroCmds;
      TempStringPrefs.macroCmds = NULL;
   }
   if (TempStringPrefs.bgMenuCmds != NULL)
   {
      LoadBGMenuCmdsString(TempStringPrefs.bgMenuCmds);
      delete[] TempStringPrefs.bgMenuCmds;
      TempStringPrefs.bgMenuCmds = NULL;
   }
   if (TempStringPrefs.highlight != NULL)
   {
      LoadHighlightString(TempStringPrefs.highlight, convertOld);
      delete[] TempStringPrefs.highlight;
      TempStringPrefs.highlight = NULL;
   }
   if (TempStringPrefs.styles != NULL)
   {
      LoadStylesString(TempStringPrefs.styles);
      delete[] TempStringPrefs.styles;
      TempStringPrefs.styles = NULL;
   }
   if (TempStringPrefs.language != NULL)
   {
      loadLanguageModesString(TempStringPrefs.language, fileVer);
      delete[] TempStringPrefs.language;
      TempStringPrefs.language = NULL;
   }
   if (TempStringPrefs.smartIndent != NULL)
   {
      LoadSmartIndentString(TempStringPrefs.smartIndent);
      delete[] TempStringPrefs.smartIndent;
      TempStringPrefs.smartIndent = NULL;
   }
   if (TempStringPrefs.smartIndentCommon != NULL)
   {
      LoadSmartIndentCommonString(TempStringPrefs.smartIndentCommon);
      delete[] TempStringPrefs.smartIndentCommon;
      TempStringPrefs.smartIndentCommon = NULL;
   }

   // translate the font names into fontLists suitable for the text widget
   PrefData.fontList = Ne_Font(PrefData.fontString);
   PrefData.boldFontStruct = Ne_Font(PrefData.boldFontString);
   PrefData.italicFontStruct = Ne_Font(PrefData.italicFontString);
   PrefData.boldItalicFontStruct = Ne_Font(PrefData.boldItalicFontString);

   // The default set for the comand shell in PrefDescrip ("DEFAULT") is
   // only a place-holder, the actual default is the user's login shell
   // (or whatever is implemented in getDefaultShell()). We put the login
   // shell's name in PrefData here.
   if (0 == strcmp(PrefData.shell, "DEFAULT"))
   {
      strncpy(PrefData.shell, getDefaultShell(), MAXPATHLEN);
      PrefData.shell[MAXPATHLEN] = '\0';
   }

   // For compatability with older (4.0.3 and before) versions, the autoWrap
   // and autoIndent resources can accept values of true and false.  Translate
   // them into acceptable wrap and indent styles
   if (PrefData.wrapStyle == 3) PrefData.wrapStyle = CONTINUOUS_WRAP;
   if (PrefData.wrapStyle == 4) PrefData.wrapStyle = NO_WRAP;
   if (PrefData.autoIndent == 3) PrefData.autoIndent = AUTO_INDENT;
   if (PrefData.autoIndent == 4) PrefData.autoIndent = NO_AUTO_INDENT;

   // setup language mode dependent info of user menus (to increase
   // performance when switching between documents of different
   // language modes)
   SetupUserMenuInfo();
}

void SaveNEditPrefs(Fl_Widget* parent, int quietly)
{
   const char* prefFileName = GetRCFileName(NEDIT_RC);
   if (prefFileName == NULL)
   {
      /*  GetRCFileName() might return NULL if an error occurs during
          creation of the preference file directory. */
      DialogF(DF_WARN, parent, 1, "Error saving Preferences",
              "Unable to save preferences: Cannot determine filename.",
              "OK");
      return;
   }

   if (!quietly)
   {
      if (DialogF(DF_INF, parent, 2, "Save Preferences",
                  ImportedFile == NULL ?
                  "Default preferences will be saved in the file:\n"
                  "%s\n"
                  "NEdit automatically loads this file\n"
                  "each time it is started." :
                  "Default preferences will be saved in the file:\n"
                  "%s\n"
                  "SAVING WILL INCORPORATE SETTINGS\n"
                  "FROM FILE: %s", "OK", "Cancel",
                  prefFileName, ImportedFile) == 2)
      {
         return;
      }
   }

   // Write the more dynamic settings into TempStringPrefs.
   // These locations are set in PrefDescrip, so this is where
   // SavePreferences() will look for them.
   TempStringPrefs.shellCmds = WriteShellCmdsString();
   TempStringPrefs.macroCmds = WriteMacroCmdsString();
   TempStringPrefs.bgMenuCmds = WriteBGMenuCmdsString();
   TempStringPrefs.highlight = WriteHighlightString();
   TempStringPrefs.language = writeLanguageModesString();
   TempStringPrefs.styles = WriteStylesString();
   TempStringPrefs.smartIndent = WriteSmartIndentString();
   TempStringPrefs.smartIndentCommon = WriteSmartIndentCommonString();
   strcpy(PrefData.fileVersion, PREF_FILE_VERSION);

   if (!SavePreferences(prefFileName, HeaderText, PrefDescrip, ARRAY_SIZE(PrefDescrip)))
   {
      DialogF(DF_WARN, parent, 1, "Save Preferences", "Unable to save preferences in %s", "OK", prefFileName);
   }

   delete[] TempStringPrefs.shellCmds;
   delete[] TempStringPrefs.macroCmds;
   delete[] TempStringPrefs.bgMenuCmds;
   delete[] TempStringPrefs.highlight;
   delete[] TempStringPrefs.language;
   delete[] TempStringPrefs.styles;
   delete[] TempStringPrefs.smartIndent;
   delete[] TempStringPrefs.smartIndentCommon;

   PrefsHaveChanged = false;
}

/*
** Load an additional preferences file on top of the existing preferences
** derived from defaults, the .nedit file, and X resources.
*/
void ImportPrefFile(const char* filename, int convertOld)
{
   Ne_Database* db;
   char* fileString;

   fileString = ReadAnyTextFile(filename, false);
   if (fileString != NULL)
   {
      db = new Ne_Database(fileString);
      free__(fileString);
      OverlayPreferences(db, APP_NAME, PrefDescrip, ARRAY_SIZE(PrefDescrip));
      translatePrefFormats(convertOld, -1);
      ImportedFile = new char[strlen(filename) + 1];
      strcpy(ImportedFile, filename);
   }
   else
   {
      fprintf(stderr, "Could not read additional preferences file: %s\n",
              filename);
   }
}

// --------------------------------------------------------------------------
void SetPrefOpenInTab(int state)
{
   WindowInfo* w = WindowList;
   setIntPref(&PrefData.openInTab, state);
   for (; w != NULL; w = w->next)
      UpdateNewOppositeMenu(w, state);
}

// --------------------------------------------------------------------------
int GetPrefOpenInTab()
{
   return PrefData.openInTab;
}

// --------------------------------------------------------------------------
void SetPrefWrap(int state)
{
   setIntPref(&PrefData.wrapStyle, state);
}

// --------------------------------------------------------------------------
int GetPrefWrap(int langMode)
{
   if (langMode == PLAIN_LANGUAGE_MODE ||
         LanguageModes[langMode]->wrapStyle == DEFAULT_WRAP)
      return PrefData.wrapStyle;
   return LanguageModes[langMode]->wrapStyle;
}

// --------------------------------------------------------------------------
void SetPrefWrapMargin(int margin)
{
   setIntPref(&PrefData.wrapMargin, margin);
}

// --------------------------------------------------------------------------
int GetPrefWrapMargin()
{
   return PrefData.wrapMargin;
}

// --------------------------------------------------------------------------
void SetPrefSearch(int searchType)
{
   setIntPref(&PrefData.searchMethod, searchType);
}

// --------------------------------------------------------------------------
int GetPrefSearch()
{
   return PrefData.searchMethod;
}

// --------------------------------------------------------------------------
void SetPrefAutoIndent(int state)
{
   setIntPref(&PrefData.autoIndent, state);
}

// --------------------------------------------------------------------------
int GetPrefAutoIndent(int langMode)
{
   if (langMode == PLAIN_LANGUAGE_MODE ||
         LanguageModes[langMode]->indentStyle == DEFAULT_INDENT)
      return PrefData.autoIndent;
   return LanguageModes[langMode]->indentStyle;
}

// --------------------------------------------------------------------------
void SetPrefAutoSave(int state)
{
   setIntPref(&PrefData.autoSave, state);
}

// --------------------------------------------------------------------------
int GetPrefAutoSave()
{
   return PrefData.autoSave;
}

// --------------------------------------------------------------------------
void SetPrefSaveOldVersion(int state)
{
   setIntPref(&PrefData.saveOldVersion, state);
}

// --------------------------------------------------------------------------
int GetPrefSaveOldVersion()
{
   return PrefData.saveOldVersion;
}

// --------------------------------------------------------------------------
void SetPrefSearchDlogs(int state)
{
   setIntPref(&PrefData.searchDlogs, state);
}

// --------------------------------------------------------------------------
int GetPrefSearchDlogs()
{
   return PrefData.searchDlogs;
}

// --------------------------------------------------------------------------
void SetPrefBeepOnSearchWrap(int state)
{
   setIntPref(&PrefData.searchWrapBeep, state);
}

// --------------------------------------------------------------------------
int GetPrefBeepOnSearchWrap()
{
   return PrefData.searchWrapBeep;
}

// --------------------------------------------------------------------------
void SetPrefKeepSearchDlogs(int state)
{
   setIntPref(&PrefData.keepSearchDlogs, state);
}

// --------------------------------------------------------------------------
int GetPrefKeepSearchDlogs()
{
   return PrefData.keepSearchDlogs;
}

// --------------------------------------------------------------------------
void SetPrefSearchWraps(int state)
{
   setIntPref(&PrefData.searchWraps, state);
}

int GetPrefStickyCaseSenseBtn()
{
   return PrefData.stickyCaseSenseBtn;
}

int GetPrefSearchWraps()
{
   return PrefData.searchWraps;
}

void SetPrefStatsLine(int state)
{
   setIntPref(&PrefData.statsLine, state);
}

int GetPrefStatsLine()
{
   return PrefData.statsLine;
}

void SetPrefISearchLine(int state)
{
   setIntPref(&PrefData.iSearchLine, state);
}

int GetPrefISearchLine()
{
   return PrefData.iSearchLine;
}

void SetPrefSortTabs(int state)
{
   setIntPref(&PrefData.sortTabs, state);
}

int GetPrefSortTabs()
{
   return PrefData.sortTabs;
}

void SetPrefTabBar(int state)
{
   setIntPref(&PrefData.tabBar, state);
}

int GetPrefTabBar()
{
   return PrefData.tabBar;
}

void SetPrefTabBarHideOne(int state)
{
   setIntPref(&PrefData.tabBarHideOne, state);
}

int GetPrefTabBarHideOne()
{
   return 0; // TODO: PrefData.tabBarHideOne;
}

void SetPrefGlobalTabNavigate(int state)
{
   setIntPref(&PrefData.globalTabNavigate, state);
}

int GetPrefGlobalTabNavigate()
{
   return PrefData.globalTabNavigate;
}

void SetPrefToolTips(int state)
{
   setIntPref(&PrefData.toolTips, state);
}

int GetPrefToolTips()
{
   return PrefData.toolTips;
}

void SetPrefLineNums(int state)
{
   setIntPref(&PrefData.lineNums, state);
}

int GetPrefLineNums()
{
   return PrefData.lineNums;
}

void SetPrefShowPathInWindowsMenu(int state)
{
   setIntPref(&PrefData.pathInWindowsMenu, state);
}

int GetPrefShowPathInWindowsMenu()
{
   return PrefData.pathInWindowsMenu;
}

void SetPrefWarnFileMods(int state)
{
   setIntPref(&PrefData.warnFileMods, state);
}

int GetPrefWarnFileMods()
{
   return PrefData.warnFileMods;
}

void SetPrefWarnRealFileMods(int state)
{
   setIntPref(&PrefData.warnRealFileMods, state);
}

int GetPrefWarnRealFileMods()
{
   return PrefData.warnRealFileMods;
}

void SetPrefWarnExit(int state)
{
   setIntPref(&PrefData.warnExit, state);
}

int GetPrefWarnExit()
{
   return PrefData.warnExit;
}

void SetPrefv(int state)
{
   setIntPref(&PrefData.findReplaceUsesSelection, state);
}

int GetPrefFindReplaceUsesSelection()
{
   return PrefData.findReplaceUsesSelection;
}

int GetPrefMapDelete()
{
   return PrefData.mapDelete;
}

int GetPrefStdOpenDialog()
{
   return PrefData.stdOpenDialog;
}

void SetPrefRows(int nRows)
{
   setIntPref(&PrefData.textRows, nRows);
}

int GetPrefRows()
{
   return PrefData.textRows;
}

void SetPrefCols(int nCols)
{
   setIntPref(&PrefData.textCols, nCols);
}

int GetPrefCols()
{
   return PrefData.textCols;
}

void SetPrefTabDist(int tabDist)
{
   setIntPref(&PrefData.tabDist, tabDist);
}

int GetPrefTabDist(int langMode)
{
   int tabDist;
   if (langMode == PLAIN_LANGUAGE_MODE ||
         LanguageModes[langMode]->tabDist == DEFAULT_TAB_DIST)
   {
      tabDist = PrefData.tabDist;
   }
   else
   {
      tabDist = LanguageModes[langMode]->tabDist;
   }
   /* Make sure that the tab distance is in range (garbage may have
      been entered via the command line or the X resources, causing
      errors later on, like division by zero). */
   if (tabDist <= 0) return 1;
   if (tabDist > MAX_EXP_CHAR_LEN) return MAX_EXP_CHAR_LEN;
   return tabDist;
}

void SetPrefEmTabDist(int tabDist)
{
   setIntPref(&PrefData.emTabDist, tabDist);
}

int GetPrefEmTabDist(int langMode)
{
   if (langMode == PLAIN_LANGUAGE_MODE ||
         LanguageModes[langMode]->emTabDist == DEFAULT_EM_TAB_DIST)
      return PrefData.emTabDist;
   return LanguageModes[langMode]->emTabDist;
}

void SetPrefInsertTabs(int state)
{
   setIntPref(&PrefData.insertTabs, state);
}

int GetPrefInsertTabs()
{
   return PrefData.insertTabs;
}

void SetPrefShowMatching(int state)
{
   setIntPref(&PrefData.showMatchingStyle, state);
}

int GetPrefShowMatching()
{
   /*
    * For backwards compatibility with pre-5.2 versions, the boolean
    * false/true matching behavior is converted to NO_FLASH/FLASH_DELIMIT.
    */
   if (PrefData.showMatchingStyle >= N_SHOW_MATCHING_STYLES)
      PrefData.showMatchingStyle -= N_SHOW_MATCHING_STYLES;
   return PrefData.showMatchingStyle;
}

void SetPrefMatchSyntaxBased(int state)
{
   setIntPref(&PrefData.matchSyntaxBased, state);
}

int GetPrefMatchSyntaxBased()
{
   return PrefData.matchSyntaxBased;
}

void SetPrefHighlightSyntax(bool state)
{
   setIntPref(&PrefData.highlightSyntax, state);
}

bool GetPrefHighlightSyntax()
{
   return PrefData.highlightSyntax;
}

void SetPrefBacklightChars(int state)
{
   setIntPref(&PrefData.backlightChars, state);
}

int GetPrefBacklightChars()
{
   return PrefData.backlightChars;
}

void SetPrefBacklightCharTypes(char* types)
{
   setStringAllocPref(&PrefData.backlightCharTypes, types);
}

char* GetPrefBacklightCharTypes()
{
   return PrefData.backlightCharTypes;
}

void SetPrefRepositionDialogs(int state)
{
   setIntPref(&PrefData.repositionDialogs, state);
}

int GetPrefRepositionDialogs()
{
   return PrefData.repositionDialogs;
}

void SetPrefAutoScroll(int state)
{
   int margin = state ? PrefData.autoScrollVPadding : 0;

   setIntPref(&PrefData.autoScroll, state);
   for (WindowInfo* w = WindowList; w != NULL; w = w->next)
      SetAutoScroll(w, margin);
}

int GetPrefAutoScroll()
{
   return PrefData.autoScroll;
}

int GetVerticalAutoScroll()
{
   return PrefData.autoScroll ? PrefData.autoScrollVPadding : 0;
}

void SetPrefAppendLF(int state)
{
   setIntPref(&PrefData.appendLF, state);
}

int GetPrefAppendLF()
{
   return PrefData.appendLF;
}

void SetPrefSortOpenPrevMenu(int state)
{
   setIntPref(&PrefData.sortOpenPrevMenu, state);
}

int GetPrefSortOpenPrevMenu()
{
   return PrefData.sortOpenPrevMenu;
}

char* GetPrefTagFile()
{
   return PrefData.tagFile;
}

void SetPrefSmartTags(int state)
{
   setIntPref(&PrefData.smartTags, state);
}

int GetPrefSmartTags()
{
   return PrefData.smartTags;
}

int GetPrefAlwaysCheckRelTagsSpecs()
{
   return PrefData.alwaysCheckRelativeTagsSpecs;
}

char* GetPrefDelimiters()
{
   return PrefData.delimiters;
}

char* GetPrefColorName(int index)
{
   return PrefData.colorNames[index];
}

void SetPrefColorName(int index, const char* name)
{
   setStringPref(PrefData.colorNames[index], name);
}

// --------------------------------------------------------------------------
// Set the font preferences using the font name (the fontList is generated
// in this call).  Note that this leaks memory and server resources each
// time the default font is re-set.  See note on SetFontByName in window.c
// for more information.
// --------------------------------------------------------------------------
void SetPrefFont(char* fontName)
{
   setStringPref(PrefData.fontString, fontName);
   PrefData.fontList = Ne_Font(fontName);
}

void SetPrefBoldFont(char* fontName)
{
   setStringPref(PrefData.boldFontString, fontName);
   PrefData.boldFontStruct = Ne_Font(fontName);
}

void SetPrefItalicFont(char* fontName)
{
   setStringPref(PrefData.italicFontString, fontName);
   PrefData.italicFontStruct = Ne_Font(fontName);
}

void SetPrefBoldItalicFont(char* fontName)
{
   setStringPref(PrefData.boldItalicFontString, fontName);
   PrefData.boldItalicFontStruct = Ne_Font(fontName);
}

char* GetPrefFontName()
{
   return PrefData.fontString;
}

char* GetPrefBoldFontName()
{
   return PrefData.boldFontString;
}

char* GetPrefItalicFontName()
{
   return PrefData.italicFontString;
}

char* GetPrefBoldItalicFontName()
{
   return PrefData.boldItalicFontString;
}

const Ne_Font& GetPrefFontList()
{
   return PrefData.fontList;
}

const Ne_Font& GetPrefBoldFont()
{
   return PrefData.boldFontStruct;
}

const Ne_Font& GetPrefItalicFont()
{
   return PrefData.italicFontStruct;
}

const Ne_Font& GetPrefBoldItalicFont()
{
   return PrefData.boldItalicFontStruct;
}

char* GetPrefHelpFontName(int index)
{
   return PrefData.helpFontNames[index];
}

char* GetPrefHelpLinkColor()
{
   return PrefData.helpLinkColor;
}

char* GetPrefTooltipBgColor()
{
   return PrefData.tooltipBgColor;
}

void SetPrefShell(const char* shell)
{
   setStringPref(PrefData.shell, shell);
}

const char* GetPrefShell()
{
   return PrefData.shell;
}

char* GetPrefGeometry()
{
   return PrefData.geometry;
}

char* GetPrefServerName()
{
   return PrefData.serverName;
}

char* GetPrefBGMenuBtn()
{
   return PrefData.bgMenuBtn;
}

int GetPrefMaxPrevOpenFiles()
{
   return PrefData.maxPrevOpenFiles;
}

int GetPrefTypingHidesPointer()
{
   return(PrefData.typingHidesPointer);
}

void SetPrefTitleFormat(const char* format)
{
   const WindowInfo* window;

   setStringPref(PrefData.titleFormat, format);

   // update all windows
   for (window = WindowList; window != NULL; window = window->next)
   {
      UpdateWindowTitle(window);
   }
}
const char* GetPrefTitleFormat()
{
   return PrefData.titleFormat;
}

bool GetPrefUndoModifiesSelection()
{
   return (bool)PrefData.undoModifiesSelection;
}

bool GetPrefFocusOnRaise()
{
   return (bool)PrefData.focusOnRaise;
}

bool GetPrefForceOSConversion()
{
   return (bool) PrefData.forceOSConversion;
}

bool GetPrefHonorSymlinks()
{
   return PrefData.honorSymlinks;
}

int GetPrefOverrideVirtKeyBindings()
{
   return PrefData.virtKeyOverride;
}

int GetPrefTruncSubstitution()
{
   return PrefData.truncSubstitution;
}

/*
** If preferences don't get saved, ask the user on exit whether to save
*/
void MarkPrefsChanged()
{
   PrefsHaveChanged = true;
}

/*
** Check if preferences have changed, and if so, ask the user if he wants
** to re-save.  Returns false if user requests cancelation of Exit (or whatever
** operation triggered this call to be made).
*/
int CheckPrefsChangesSaved(Fl_Widget* dialogParent)
{
   int resp = 0;

   if (!PrefsHaveChanged)
      return true;

   resp = DialogF(DF_WARN, dialogParent, 3, "Default Preferences",
                  ImportedFile == NULL ?
                  "Default Preferences have changed.\n"
                  "Save changes to NEdit preference file?" :
                  "Default Preferences have changed.  SAVING \n"
                  "CHANGES WILL INCORPORATE ADDITIONAL\nSETTINGS FROM FILE: %s",
                  "Save", "Don't Save", "Cancel", ImportedFile);
   if (resp == 2)
      return true;
   if (resp == 3)
      return false;

   SaveNEditPrefs(dialogParent, true);
   return true;
}

/*
** set *prefDataField to newValue, but first check if they're different
** and update PrefsHaveChanged if a preference setting has now changed.
*/
static void setIntPref(int* prefDataField, int newValue)
{
   if (newValue != *prefDataField)
      PrefsHaveChanged = true;
   *prefDataField = newValue;
}

static void setStringPref(char* prefDataField, const char* newValue)
{
   if (strcmp(prefDataField, newValue))
      PrefsHaveChanged = true;
   strcpy(prefDataField, newValue);
}

static void setStringAllocPref(char** pprefDataField, char* newValue)
{
   char* p_newField;

   /* treat empty strings as nulls */
   if (newValue && *newValue == '\0')
      newValue = NULL;
   if (*pprefDataField &&** pprefDataField == '\0')
      *pprefDataField = NULL;         /* assume statically alloc'ed "" */

   /* check changes */
   if (!*pprefDataField && !newValue)
      return;
   else if (!*pprefDataField && newValue)
      PrefsHaveChanged = true;
   else if (*pprefDataField && !newValue)
      PrefsHaveChanged = true;
   else if (strcmp(*pprefDataField, newValue))
      PrefsHaveChanged = true;

   /* get rid of old preference */
   free__(*pprefDataField);

   /* store new preference */
   if (newValue)
   {
      p_newField = (char*)malloc__(strlen(newValue) + 1);
      strcpy(p_newField, newValue);
   }
   *pprefDataField = newValue;
}

/*
** Set the language mode for the window, update the menu and trigger language
** mode specific actions (turn on/off highlighting).  If forceNewDefaults is
** true, re-establish default settings for language-specific preferences
** regardless of whether they were previously set by the user.
*/
void SetLanguageMode(WindowInfo* window, int mode, int forceNewDefaults)
{
   TRACE();

   // Do mode-specific actions
   reapplyLanguageMode(window, mode, forceNewDefaults);

   // Select the correct language mode in the sub-menu
   if (IsTopDocument(window))
   {
      // First item of the submenu
      int index = window->menuBar->find_index(window->menuBar->getLanguageModesItem()) + 1;
      Fl_Menu_Item* start = (Fl_Menu_Item*)&(window->menuBar->menu())[index];

      for (int n = 0; start && start->label(); start = start->next(), ++n)
      {
         if (n == mode + 1)
            start->set();
         else
            start->clear();
      }
   }
}

/*
** Lookup a language mode by name, returning the index of the language
** mode or PLAIN_LANGUAGE_MODE if the name is not found
*/
int FindLanguageMode(const char* languageName)
{
   // Compare each language mode to the one we were presented
   for (int i = 0; i < NLanguageModes; i++)
      if (!strcmp(languageName, LanguageModes[i]->name))
         return i;

   return PLAIN_LANGUAGE_MODE;
}


/*
** Apply language mode matching criteria and set window->languageMode to
** the appropriate mode for the current file, trigger language mode
** specific actions (turn on/off highlighting), and update the language
** mode menu item.  If forceNewDefaults is true, re-establish default
** settings for language-specific preferences regardless of whether
** they were previously set by the user.
*/
void DetermineLanguageMode(WindowInfo* window, int forceNewDefaults)
{
   SetLanguageMode(window, matchLanguageMode(window), forceNewDefaults);
}

/*
** Return the name of the current language mode set in "window", or NULL
** if the current mode is "Plain".
*/
char* LanguageModeName(int mode)
{
   if (mode == PLAIN_LANGUAGE_MODE)
      return NULL;
   else
      return LanguageModes[mode]->name;
}

/*
** Get the set of word delimiters for the language mode set in the current
** window.  Returns NULL when no language mode is set (it would be easy to
** return the default delimiter set when the current language mode is "Plain",
** or the mode doesn't have its own delimiters, but this is usually used
** to supply delimiters for RE searching, and ExecRE can skip compiling a
** delimiter table when delimiters is NULL).
*/
char* GetWindowDelimiters(const WindowInfo* window)
{
   if (window->languageMode == PLAIN_LANGUAGE_MODE)
      return NULL;
   else
      return LanguageModes[window->languageMode]->delimiters;
}

/*
** Put up a dialog for selecting a custom initial window size
*/
void RowColumnPrefDialog(Fl_Widget* parent)
{
   Fl_Double_Window dialog(30,50, 250, 140, "Initial Window Size");
   Fl_Box topLabel(5, 5, 240, 50, "Enter desired size in rows\nand columns of characters:");

   RowText = new Fl_Int_Input(55, 60, 50, 25);
   Fl_Box xLabel(112, 60, 25, 25, "x");
   ColText = new Fl_Int_Input(140, 60, 50, 25);

   Fl_Group promptResizable(0, 86, 250, 1);
   promptResizable.end();

   Fl_Group buttonLine(0, 90, 250, 50);
   buttonLine.box(FL_ENGRAVED_FRAME);

   Fl_Button btnOk(10, 105, 80, 25, "Ok");
   btnOk.shortcut(FL_Enter);
   btnOk.callback(sizeOKCB);

   Fl_Button btnCancel(160, 105, 80, 25, "Cancel");
   btnCancel.shortcut(FL_Escape);
   btnCancel.callback(sizeCancelCB);

   buttonLine.end();

   dialog.resizable(&promptResizable);

   // put up dialog and wait for user to press ok or cancel
   ManageDialogCenteredOnPointer(&dialog);
   
   dialog.set_modal();
   dialog.show();
   while (dialog.shown()) Fl::wait();
}

static void sizeOKCB(Fl_Widget* w, void* data)
{
   int rowValue, colValue;

   // get the values that the user entered and make sure they're ok
   int stat = GetIntTextWarn(RowText, &rowValue, "number of rows", true);
   if (stat != TEXT_READ_OK)
      return;
   stat = GetIntTextWarn(ColText, &colValue, "number of columns", true);
   if (stat != TEXT_READ_OK)
      return;

   // set the corresponding preferences and dismiss the dialog
   SetPrefRows(rowValue);
   SetPrefCols(colValue);

   WidgetToMainWindow(w)->hide();
}

static void sizeCancelCB(Fl_Widget* w, void* data)
{
   WidgetToMainWindow(w)->hide();
}

// --------------------------------------------------------------------------
// Present a dialog for editing language mode information
// --------------------------------------------------------------------------
void EditLanguageModes()
{
   // if the dialog is already displayed, just pop it to the top and return
   if (LMDialog.shell != NULL)
   {
      LMDialog.shell->show();
      return;
   }

   LMDialog.languageModeList = new languageModeRec*[MAX_LANGUAGE_MODES];
   for (int i = 0; i < NLanguageModes; i++)
      LMDialog.languageModeList[i] = copyLanguageModeRec(LanguageModes[i]);
   LMDialog.nLanguageModes = NLanguageModes;

   // Create the form
   LMDialog.shell = new Fl_Double_Window(30, 50, 600, 510, "Language Modes");
   LMDialog.shell->callback(lmCloseCB);

   Fl_Box* topLbl = new Fl_Box(5, 5, 590, 50, "To modify the properties of an existing language mode, select the name from\nthe list on the left.  To add a new language, select \"&New\" from the list.");

   Fl_Group* inputBox = new Fl_Group(0, 55, 600, 255);
   // Managed List
   LMDialog.managedListW = CreateManagedList(Ne_Dimension(5, 50, 240, 250),
                           (NamedItem**)LMDialog.languageModeList, &LMDialog.nLanguageModes, MAX_LANGUAGE_MODES, 15,
                           &lmGetDisplayedCB, NULL,
                           &lmSetDisplayedCB, NULL,
                           &lmFreeItemCB);
   AddDeleteConfirmCB(LMDialog.managedListW, &lmDeleteConfirmCB, NULL);

   // Input Field
   LMDialog.nameW = new Fl_Input(250, 65, 200, 25, "Na&me");
   LMDialog.nameW->align(FL_ALIGN_TOP_LEFT);

   LMDialog.extW = new Fl_Input(250, 125, 340, 25, "&File extensions (separate w/ space)");
   LMDialog.extW->align(FL_ALIGN_TOP_LEFT);

   LMDialog.recogW = new Fl_Input(250, 195, 340, 25, "&Recognition regular expression (applied to first 200\ncharacters of file to determine type from content)");
   LMDialog.recogW->align(FL_ALIGN_TOP_LEFT);

   LMDialog.defTipsW = new Fl_Input(250, 245, 340, 25, "Default &calltips file(s) (separate w/colons)");
   LMDialog.defTipsW->align(FL_ALIGN_TOP_LEFT);
   inputBox->end();

   // Override Defaults
   Fl_Group* overrideFrame = new Fl_Group(5, 320, 590, 135, "Override Defaults");
   overrideFrame->align(FL_ALIGN_TOP | FL_ALIGN_CENTER);
   overrideFrame->box(FL_ENGRAVED_FRAME);

   LMDialog.delimitW = new Fl_Input(110, 330, 480, 25, "&Word delimiters");
   LMDialog.tabW = new Fl_Int_Input(220, 360, 30, 25, "Al&ternative hardware tab spacing");
   LMDialog.emTabW = new Fl_Int_Input(560, 360, 30, 25, "Alt&ernative emulated tab spacing");

   Fl_Group* indentBox = new Fl_Group(10, 390, 580, 30);
   LMDialog.defaultIndentW = new Fl_Radio_Round_Button(15, 392, 150, 25, "&Default indent style");
   LMDialog.defaultIndentW->value(1);
   LMDialog.noIndentW = new Fl_Radio_Round_Button(165, 392, 155, 25, "&No automatic indent");
   LMDialog.autoIndentW = new Fl_Radio_Round_Button(325, 392, 100, 25, "&Auto-indent");
   LMDialog.smartIndentW = new Fl_Radio_Round_Button(425, 392, 110, 25, "&Smart-indent");
   indentBox->end();

   Fl_Group* wrapBox = new Fl_Group(10, 420, 580, 30);
   LMDialog.defaultWrapW = new Fl_Radio_Round_Button(15, 422, 130, 25, "&Default wrap style");
   LMDialog.defaultWrapW->value(1);
   LMDialog.noWrapW = new Fl_Radio_Round_Button(165, 422, 100, 25, "&No wrapping");
   LMDialog.newlineWrapW = new Fl_Radio_Round_Button(265, 422, 140, 25, "&Auto newline wrap");
   LMDialog.contWrapW = new Fl_Radio_Round_Button(405, 422, 120, 25, "&Continuous wrap");
   wrapBox->end();
   overrideFrame->end();

   Fl_Group* buttonLine = new Fl_Group(0, 460, 600, 50);
   buttonLine->box(FL_ENGRAVED_FRAME);

   Fl_Button* btnOk = new Fl_Button(100, 475, 100, 25, "Ok");
   btnOk->shortcut(FL_Enter);
   btnOk->callback(lmOkCB);

   Fl_Button* btnApply = new Fl_Button(250, 475, 100, 25, "&Apply");
   btnApply->callback(lmApplyCB);

   Fl_Button* btnClose = new Fl_Button(400, 475, 100, 25, "Close");
   btnClose->shortcut(FL_Escape);
   btnClose->callback(lmCloseCB);

   buttonLine->end();

   LMDialog.shell->resizable(inputBox);

   // Display the form
   LMDialog.shell->show();
}

// --------------------------------------------------------------------------
static void lmDestroyCB(Fl_Widget* w, void* data)
{
   for (int i=0; i<LMDialog.nLanguageModes; i++)
      freeLanguageModeRec(LMDialog.languageModeList[i]);
   delete LMDialog.languageModeList;

   delete LMDialog.shell;
   LMDialog.shell = NULL;
}

// --------------------------------------------------------------------------
static void lmOkCB(Fl_Widget* w, void* data)
{
   TRACE();

   if (!updateLMList())
      return;

   // pop down and destroy the dialog
   lmDestroyCB(LMDialog.shell, &LMDialog);
}

// --------------------------------------------------------------------------
static void lmApplyCB(Fl_Widget* w, void* data)
{
   TRACE();

   updateLMList();
}

// --------------------------------------------------------------------------
static void lmCloseCB(Fl_Widget* w, void* data)
{
   TRACE();

   // pop down and destroy the dialog
   lmDestroyCB(LMDialog.shell, &LMDialog);
}

// --------------------------------------------------------------------------
static int lmDeleteConfirmCB(int itemIndex, void* cbArg)
{
   TRACE();

   // Allow duplicate names to be deleted regardless of dependencies
   for (int i = 0; i < LMDialog.nLanguageModes; i++)
      if (i != itemIndex && !strcmp(LMDialog.languageModeList[i]->name,
                                    LMDialog.languageModeList[itemIndex]->name))
         return true;

   // don't allow deletion if data will be lost
   if (LMHasHighlightPatterns(LMDialog.languageModeList[itemIndex]->name))
   {
      DialogF(DF_WARN, LMDialog.shell, 1, "Patterns exist",
              "This language mode has syntax highlighting\n"
              "patterns defined.  Please delete the patterns\n"
              "first, in Preferences -> Default Settings ->\n"
              "Syntax Highlighting, before proceeding here.", "OK");
      return false;
   }

   // don't allow deletion if data will be lost
   if (LMHasSmartIndentMacros(LMDialog.languageModeList[itemIndex]->name))
   {
      DialogF(DF_WARN, LMDialog.shell, 1, "Smart Indent Macros exist",
              "This language mode has smart indent macros\n"
              "defined.  Please delete the macros first,\n"
              "in Preferences -> Default Settings ->\n"
              "Auto Indent -> Program Smart Indent,\n"
              "before proceeding here.", "OK");
      return false;
   }

   return true;
}

// --------------------------------------------------------------------------
// Apply the changes that the user has made in the language modes dialog to the
// stored language mode information for this NEdit session (the data array
// LanguageModes)
// --------------------------------------------------------------------------
static int updateLMList()
{
   TRACE();

   WindowInfo* window;
   int oldLanguageMode;
   char* oldModeName, *newDelimiters;
   int i, j;

   // Get the current contents of the dialog fields
   if (!UpdateManagedList(LMDialog.managedListW, true))
      return false;

   // Fix up language mode indices in all open windows (which may change
   // if the currently selected mode is deleted or has changed position),
   // and update word delimiters
   for (window = WindowList; window != NULL; window = window->next)
   {
      if (window->languageMode != PLAIN_LANGUAGE_MODE)
      {
         oldLanguageMode = window->languageMode;
         oldModeName = LanguageModes[window->languageMode]->name;
         window->languageMode = PLAIN_LANGUAGE_MODE;
         for (i = 0; i < LMDialog.nLanguageModes; i++)
         {
            if (!strcmp(oldModeName, LMDialog.languageModeList[i]->name))
            {
               newDelimiters = LMDialog.languageModeList[i]->delimiters;
               if (newDelimiters == NULL)
                  newDelimiters = GetPrefDelimiters();
               // TODO:                XtVaSetValues(window->textArea, textNwordDelimiters, newDelimiters, NULL);
               // TODO:                for (j=0; j<window->nPanes; j++)
               // TODO:                   XtVaSetValues(window->textPanes[j], textNwordDelimiters, newDelimiters, NULL);
               // don't forget to adapt the LM stored within the user menu cache
               if (window->userMenuCache->umcLanguageMode == oldLanguageMode)
                  window->userMenuCache->umcLanguageMode = i;
               if (window->userBGMenuCache.ubmcLanguageMode == oldLanguageMode)
                  window->userBGMenuCache.ubmcLanguageMode = i;
               // update the language mode of this window (document)
               window->languageMode = i;
               break;
            }
         }
      }
   }

   // If there were any name changes, re-name dependent highlight patterns
   // and smart-indent macros and fix up the weird rename-format names
   for (i = 0; i < LMDialog.nLanguageModes; i++)
   {
      if (strchr(LMDialog.languageModeList[i]->name, ':') != NULL)
      {
         char* newName = strrchr(LMDialog.languageModeList[i]->name, ':') + 1;
         *strchr(LMDialog.languageModeList[i]->name, ':') = '\0';
         RenameHighlightPattern(LMDialog.languageModeList[i]->name, newName);
         RenameSmartIndentMacros(LMDialog.languageModeList[i]->name, newName);
         memmove(LMDialog.languageModeList[i]->name, newName, strlen(newName) + 1);
         ChangeManagedListData(LMDialog.managedListW);
      }
   }

   // Replace the old language mode list with the new one from the dialog
   for (i = 0; i < NLanguageModes; i++)
      freeLanguageModeRec(LanguageModes[i]);
   for (i = 0; i < LMDialog.nLanguageModes; i++)
      LanguageModes[i] = copyLanguageModeRec(LMDialog.languageModeList[i]);
   NLanguageModes = LMDialog.nLanguageModes;

   // Update user menu info to update language mode dependencies of user menu items
   UpdateUserMenuInfo();

   // Update the menus in the window menu bars and load any needed
   // calltips files
   for (window = WindowList; window != NULL; window = window->next)
   {
      updateLanguageModeSubmenu(window);
      if (window->languageMode != PLAIN_LANGUAGE_MODE &&
            LanguageModes[window->languageMode]->defTipsFile != NULL)
         AddTagsFile(LanguageModes[window->languageMode]->defTipsFile, TIP);
      // cache user menus: Rebuild all user menus of this window
      RebuildAllMenus(window);
   }

   // If a syntax highlighting dialog is up, update its menu
   UpdateLanguageModeMenu();
   // The same for the smart indent macro dialog
   UpdateLangModeMenuSmartIndent();
   // Note that preferences have been changed
   MarkPrefsChanged();

   return true;
}

// --------------------------------------------------------------------------
static void* lmGetDisplayedCB(void* oldItem, int explicitRequest, int* abort, void* cbArg)
{
   TRACE();

   languageModeRec* oldLM = (languageModeRec*)oldItem;
   char* tempName;
   int nCopies, oldLen;

   // If the dialog is currently displaying the "new" entry and the
   // fields are empty, that's just fine */
   if (oldItem == NULL && lmDialogEmpty())
      return NULL;

   // Read the data the user has entered in the dialog fields
   languageModeRec* lm = readLMDialogFields(true);

   // If there was a name change of a non-duplicate language mode, modify the
   // name to the weird format of: ":old name:new name".  This signals that a
   // name change is necessary in lm dependent data such as highlight
   // patterns.  Duplicate language modes may be re-named at will, since no
   // data will be lost due to the name change.
   if (lm != NULL && oldLM != NULL && strcmp(oldLM->name, lm->name))
   {
      nCopies = 0;
      for (int i = 0; i < LMDialog.nLanguageModes; i++)
         if (!strcmp(oldLM->name, LMDialog.languageModeList[i]->name))
            nCopies++;
      if (nCopies <= 1)
      {
         oldLen = strchr(oldLM->name, ':') == NULL ? strlen(oldLM->name) : strchr(oldLM->name, ':') - oldLM->name;
         tempName = (char*)malloc__(oldLen + strlen(lm->name) + 2);
         strncpy(tempName, oldLM->name, oldLen);
         sprintf(&tempName[oldLen], ":%s", lm->name);
         free__(lm->name);
         lm->name = tempName;
      }
   }

   // If there are no problems reading the data, just return it
   if (lm != NULL)
      return (void*)lm;

   // If there are problems, and the user didn't ask for the fields to be
   // read, give more warning
   if (!explicitRequest)
   {
      if (DialogF(DF_WARN, LMDialog.shell, 2, "Discard Language Mode",
                  "Discard incomplete entry\nfor current language mode?", "Keep",
                  "Discard") == 1)  // TODO:2)
      {
         return oldItem == NULL
                ? NULL
                : (void*)copyLanguageModeRec((languageModeRec*)oldItem);
      }
   }

   // Do readLMDialogFields again without "silent" mode to display warning
   lm = readLMDialogFields(false);
   *abort = true;
   return NULL;
}

// --------------------------------------------------------------------------
static void lmSetDisplayedCB(void* item, void* cbArg)
{
   TRACE();

   languageModeRec* lm = (languageModeRec*)item;
   char* extStr;

   if (item == NULL)
   {
      NeTextSetString(LMDialog.nameW, "");
      NeTextSetString(LMDialog.extW, "");
      NeTextSetString(LMDialog.recogW, "");
      NeTextSetString(LMDialog.defTipsW, "");
      NeTextSetString(LMDialog.delimitW, "");
      NeTextSetString(LMDialog.tabW, "");
      NeTextSetString(LMDialog.emTabW, "");
      NeRadioButtonChangeState(LMDialog.defaultIndentW, true, true);
      NeRadioButtonChangeState(LMDialog.defaultWrapW, true, true);
   }
   else
   {
      NeTextSetString(LMDialog.nameW, strchr(lm->name, ':') == NULL ? lm->name : strchr(lm->name, ':') + 1);
      extStr = createExtString(lm->extensions, lm->nExtensions);
      NeTextSetString(LMDialog.extW, extStr);
      delete[] extStr;
      NeTextSetString(LMDialog.recogW, lm->recognitionExpr);
      NeTextSetString(LMDialog.defTipsW, lm->defTipsFile);
      NeTextSetString(LMDialog.delimitW, lm->delimiters);
      if (lm->tabDist == DEFAULT_TAB_DIST)
         NeTextSetString(LMDialog.tabW, "");
      else
         SetIntText(LMDialog.tabW, lm->tabDist);
      if (lm->emTabDist == DEFAULT_EM_TAB_DIST)
         NeTextSetString(LMDialog.emTabW, "");
      else
         SetIntText(LMDialog.emTabW, lm->emTabDist);
      NeRadioButtonChangeState(LMDialog.defaultIndentW, lm->indentStyle == DEFAULT_INDENT, false);
      NeRadioButtonChangeState(LMDialog.noIndentW, lm->indentStyle == NO_AUTO_INDENT, false);
      NeRadioButtonChangeState(LMDialog.autoIndentW, lm->indentStyle == AUTO_INDENT, false);
      NeRadioButtonChangeState(LMDialog.smartIndentW, lm->indentStyle == SMART_INDENT, false);
      NeRadioButtonChangeState(LMDialog.defaultWrapW, lm->wrapStyle == DEFAULT_WRAP, false);
      NeRadioButtonChangeState(LMDialog.noWrapW, lm->wrapStyle == NO_WRAP, false);
      NeRadioButtonChangeState(LMDialog.newlineWrapW, lm->wrapStyle == NEWLINE_WRAP, false);
      NeRadioButtonChangeState(LMDialog.contWrapW, lm->wrapStyle == CONTINUOUS_WRAP, false);
   }
}

// --------------------------------------------------------------------------
static void lmFreeItemCB(void* item)
{
   freeLanguageModeRec((languageModeRec*)item);
}

// --------------------------------------------------------------------------
static void freeLanguageModeRec(languageModeRec* lm)
{
   delete[] lm->name;
   delete[] lm->recognitionExpr;
   delete[] lm->defTipsFile;
   delete[] lm->delimiters;
   for (int i = 0; i < lm->nExtensions; i++)
      delete[] lm->extensions[i];
   delete[] lm->extensions;
   delete lm;
}

// --------------------------------------------------------------------------
// Copy a languageModeRec data structure and all of the allocated data it contains
// --------------------------------------------------------------------------
static languageModeRec* copyLanguageModeRec(languageModeRec* lm)
{
   languageModeRec* newLM = new languageModeRec();

   newLM->name = new char[strlen(lm->name) + 1];
   strcpy(newLM->name, lm->name);
   newLM->nExtensions = lm->nExtensions;
   newLM->extensions = new char*[lm->nExtensions];
   for (int i = 0; i < lm->nExtensions; i++)
   {
      newLM->extensions[i] = new char[strlen(lm->extensions[i]) + 1];
      strcpy(newLM->extensions[i], lm->extensions[i]);
   }
   if (lm->recognitionExpr == NULL)
      newLM->recognitionExpr = NULL;
   else
   {
      newLM->recognitionExpr = new char[strlen(lm->recognitionExpr) + 1];
      strcpy(newLM->recognitionExpr, lm->recognitionExpr);
   }
   if (lm->defTipsFile == NULL)
      newLM->defTipsFile = NULL;
   else
   {
      newLM->defTipsFile = new char[strlen(lm->defTipsFile) + 1];
      strcpy(newLM->defTipsFile, lm->defTipsFile);
   }
   if (lm->delimiters == NULL)
      newLM->delimiters = NULL;
   else
   {
      newLM->delimiters = new char[strlen(lm->delimiters) + 1];
      strcpy(newLM->delimiters, lm->delimiters);
   }
   newLM->wrapStyle = lm->wrapStyle;
   newLM->indentStyle = lm->indentStyle;
   newLM->tabDist = lm->tabDist;
   newLM->emTabDist = lm->emTabDist;
   return newLM;
}

// --------------------------------------------------------------------------
// Read the fields in the language modes dialog and create a languageModeRec data
// structure reflecting the current state of the selected language mode in the dialog.
// If any of the information is incorrect or missing, display a warning dialog and
// return NULL.  Passing "silent" as true, suppresses the warning dialogs.
// --------------------------------------------------------------------------
static languageModeRec* readLMDialogFields(int silent)
{
   regexp* compiledRE;
   char* compileMsg, *extStr, *extPtr;

   // Allocate a language mode structure to return, set unread fields to
   // empty so everything can be freed on errors by freeLanguageModeRec
   languageModeRec* lm = new languageModeRec();
   lm->recognitionExpr = NULL;
   lm->defTipsFile = NULL;
   lm->delimiters = NULL;
   lm->extensions = NULL;
   lm->nExtensions = 0;

   // read the name field
   lm->name = ReadSymbolicFieldTextWidget(LMDialog.nameW, "language mode name", silent);
   if (lm->name == NULL)
   {
      free__((char*)lm);
      return NULL;
   }

   if (*lm->name == '\0')
   {
      if (!silent)
      {
         DialogF(DF_WARN, LMDialog.shell, 1, "Language Mode Name", "Please specify a name\nfor the language mode", "OK");
         LMDialog.shell->focus(LMDialog.nameW);
      }
      freeLanguageModeRec(lm);
      return NULL;
   }

   // read the extension list field
   extStr = extPtr = NeTextGetString(LMDialog.extW);
   lm->extensions = readExtensionList(&extPtr, &lm->nExtensions);
   delete[] extStr;

   // read recognition expression
   lm->recognitionExpr = NeTextGetString(LMDialog.recogW);
   if (*lm->recognitionExpr == '\0')
   {
      delete[] lm->recognitionExpr;
      lm->recognitionExpr = NULL;
   }
   else
   {
      compiledRE = CompileRE(lm->recognitionExpr, &compileMsg, REDFLT_STANDARD);

      if (compiledRE == NULL)
      {
         if (!silent)
         {
            DialogF(DF_WARN, LMDialog.shell, 1, "Regex", "Recognition expression:\n%s", "OK", compileMsg);
            LMDialog.shell->focus(LMDialog.recogW);
         }
         delete compiledRE;
         freeLanguageModeRec(lm);
         return NULL;
      }

      delete compiledRE;
   }

   // Read the default calltips file for the language mode
   lm->defTipsFile = NeTextGetString(LMDialog.defTipsW);
   if (*lm->defTipsFile == '\0')
   {
      // Empty string
      delete[] lm->defTipsFile;
      lm->defTipsFile = NULL;
   }
   else
   {
      // Ensure that AddTagsFile will work
      if (AddTagsFile(lm->defTipsFile, TIP) == false)
      {
         if (!silent)
         {
            DialogF(DF_WARN, LMDialog.shell, 1, "Error reading Calltips",
                    "Can't read default calltips file(s):\n  \"%s\"\n",
                    "OK", lm->defTipsFile);
            LMDialog.shell->focus(LMDialog.defTipsW);
         }
         freeLanguageModeRec(lm);
         return NULL;
      }
      else if (DeleteTagsFile(lm->defTipsFile, TIP, false) == false)
         fprintf(stderr, "nedit: Internal error: Trouble deleting calltips file(s):\n  \"%s\"\n", lm->defTipsFile);
   }

   // read tab spacing field
   if (TextWidgetIsBlank(LMDialog.tabW))
      lm->tabDist = DEFAULT_TAB_DIST;
   else
   {
      if (GetIntTextWarn(LMDialog.tabW, &lm->tabDist, "tab spacing", false)
            != TEXT_READ_OK)
      {
         freeLanguageModeRec(lm);
         return NULL;
      }

      if (lm->tabDist <= 0 || lm->tabDist > 100)
      {
         if (!silent)
         {
            DialogF(DF_WARN, LMDialog.shell, 1, "Invalid Tab Spacing", "Invalid tab spacing: %d", "OK", lm->tabDist);
            LMDialog.shell->focus(LMDialog.tabW);
         }
         freeLanguageModeRec(lm);
         return NULL;
      }
   }

   // read emulated tab field
   if (TextWidgetIsBlank(LMDialog.emTabW))
   {
      lm->emTabDist = DEFAULT_EM_TAB_DIST;
   }
   else
   {
      if (GetIntTextWarn(LMDialog.emTabW, &lm->emTabDist, "emulated tab spacing", false) != TEXT_READ_OK)
      {
         freeLanguageModeRec(lm);
         return NULL;
      }

      if (lm->emTabDist < 0 || lm->emTabDist > 100)
      {
         if (!silent)
         {
            DialogF(DF_WARN, LMDialog.shell, 1, "Invalid Tab Spacing", "Invalid emulated tab spacing: %d", "OK", lm->emTabDist);
            LMDialog.shell->focus(LMDialog.emTabW);
         }
         freeLanguageModeRec(lm);
         return NULL;
      }
   }

   // read delimiters string
   lm->delimiters = NeTextGetString(LMDialog.delimitW);
   if (*lm->delimiters == '\0')
   {
      delete[] lm->delimiters;
      lm->delimiters = NULL;
   }

   // read indent style
   if (NeToggleButtonGetState(LMDialog.noIndentW))
      lm->indentStyle = NO_AUTO_INDENT;
   else if (NeToggleButtonGetState(LMDialog.autoIndentW))
      lm->indentStyle = AUTO_INDENT;
   else if (NeToggleButtonGetState(LMDialog.smartIndentW))
      lm->indentStyle = SMART_INDENT;
   else
      lm->indentStyle = DEFAULT_INDENT;

   // read wrap style
   if (NeToggleButtonGetState(LMDialog.noWrapW))
      lm->wrapStyle = NO_WRAP;
   else if (NeToggleButtonGetState(LMDialog.newlineWrapW))
      lm->wrapStyle = NEWLINE_WRAP;
   else if (NeToggleButtonGetState(LMDialog.contWrapW))
      lm->wrapStyle = CONTINUOUS_WRAP;
   else
      lm->wrapStyle = DEFAULT_WRAP;

   return lm;
}

// --------------------------------------------------------------------------
// Return true if the language mode dialog fields are blank (unchanged from the "New"
// language mode state).
// --------------------------------------------------------------------------
static int lmDialogEmpty()
{
   return TextWidgetIsBlank(LMDialog.nameW) &&
          TextWidgetIsBlank(LMDialog.extW) &&
          TextWidgetIsBlank(LMDialog.recogW) &&
          TextWidgetIsBlank(LMDialog.delimitW) &&
          TextWidgetIsBlank(LMDialog.tabW) &&
          TextWidgetIsBlank(LMDialog.emTabW) &&
          NeToggleButtonGetState(LMDialog.defaultIndentW) &&
          NeToggleButtonGetState(LMDialog.defaultWrapW);
}

// --------------------------------------------------------------------------
// Present a dialog for changing fonts (primary, and for highlighting).
// --------------------------------------------------------------------------
void ChooseFonts(WindowInfo* window, int forWindow)
{
   // if the dialog is already displayed, just pop it to the top and return
   if (window->fontDialog != NULL)
   {
      if (forWindow)((fontDialog*)window->fontDialog)->btnApply->show();
      else ((fontDialog*)window->fontDialog)->btnApply->hide();

      ((fontDialog*)window->fontDialog)->mainWindow->show();
      return;
   }

   // Create a structure for keeping track of dialog state
   fontDialog* fd = new fontDialog();
   window->fontDialog = (void*)fd;

   // Create a form widget in a dialog shell
   Fl_Window* form = new Fl_Window(30, 50, 500, 350, "Text Fonts");
   form->callback(fontCancelCB, fd);
   fd->mainWindow = form;
   fd->window = window;
   fd->forWindow = forWindow;

   Fl_Group* primaryFrame = new Fl_Group(5, 25, 490, 35, "Primary Font");
   primaryFrame->align(FL_ALIGN_TOP | FL_ALIGN_CENTER);
   primaryFrame->box(FL_ENGRAVED_FRAME);

   Fl_Button* primaryBtn = new Fl_Button(10, 30, 70, 25, "B&rowse...");
   primaryBtn->callback(primaryBrowseCB, fd);

   fd->primaryW = new Fl_Input(85, 30, 400, 25);
   fd->primaryW->callback(primaryModifiedCB, fd);
   fd->primaryW->when(FL_WHEN_CHANGED);

   primaryFrame->end();

   Fl_Group* highlightFrame = new Fl_Group(5, 80, 490, 210, "Fonts for Syntax Highlighting");
   highlightFrame->align(FL_ALIGN_TOP | FL_ALIGN_CENTER);
   highlightFrame->box(FL_ENGRAVED_FRAME);

   fd->fillW = new Fl_Button(10, 85, 250, 25, "&Fill Highlight Fonts from Primary");
   fd->fillW->callback(fillFromPrimaryCB, fd);

   // Italic Font
   Fl_Box* italicLbl = new Fl_Box(10, 115, 80, 25, "Italic Font");
   italicLbl->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

   fd->italicErrW = new Fl_Box(100, 115, 390, 25, "(vvv  spacing is inconsistent with primary font  vvv)");
   fd->italicErrW->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

   Fl_Button* italicBtn = new Fl_Button(10, 140, 70, 25, "Br&owse...");
   italicBtn->callback(italicBrowseCB, fd);

   fd->italicW = new Fl_Input(85, 140, 400, 25);
   fd->italicW->callback(italicModifiedCB, fd);
   fd->italicW->when(FL_WHEN_CHANGED);

   // Bold Font
   Fl_Box* boldLbl = new Fl_Box(10, 175, 80, 25, "Bold Font");
   boldLbl->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

   fd->boldErrW = new Fl_Box(100, 175, 390, 25, "(vvv  spacing is inconsistent with primary font  vvv)");
   fd->boldErrW->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

   Fl_Button* boldBtn = new Fl_Button(10, 200, 70, 25, "Bro&wse...");
   boldBtn->callback(boldBrowseCB, fd);

   fd->boldW = new Fl_Input(85, 200, 400, 25);
   fd->boldW->callback(boldModifiedCB, fd);
   fd->boldW->when(FL_WHEN_CHANGED);

   // Bold Italic Font
   Fl_Box* boldItalicLbl = new Fl_Box(10, 235, 80, 25, "Bold Italic Font");
   boldItalicLbl->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

   fd->boldItalicErrW = new Fl_Box(100, 235, 390, 25, "(vvv  spacing is inconsistent with primary font  vvv)");
   fd->boldItalicErrW->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

   Fl_Button* boldItalicBtn = new Fl_Button(10, 260, 70, 25, "Brow&se...");
   boldItalicBtn->callback(boldItalicBrowseCB, fd);

   fd->boldItalicW = new Fl_Input(85, 260, 400, 25);
   fd->boldItalicW->callback(boldItalicModifiedCB, fd);
   fd->boldItalicW->when(FL_WHEN_CHANGED);

   highlightFrame->end();

   Fl_Box* resizableBox = new Fl_Box(0, 290, 600, 1);

   // The OK, Apply, and Cancel buttons
   Fl_Group* buttonLine = new Fl_Group(0, 300, 500, 50);
   buttonLine->box(FL_ENGRAVED_FRAME);

   Fl_Button* btnOk = new Fl_Button(50, 310, 100, 25, "Ok");
   btnOk->shortcut(FL_Enter);
   btnOk->callback(fontOkCB, fd);

   if (forWindow)
   {
      fd->btnApply = new Fl_Button(200, 310, 100, 25, "&Apply");
      fd->btnApply->callback(fontApplyCB, fd);
   }

   Fl_Button* btnClose = new Fl_Button(360, 310, 100, 25, "Close");
   btnClose->shortcut(FL_Escape);
   btnClose->callback(fontCancelCB, fd);

   buttonLine->end();

   form->resizable(resizableBox);

   // Set initial values
   if (forWindow)
   {
      NeTextSetString(fd->primaryW, window->fontName);
      fd->primaryW->do_callback();
      NeTextSetString(fd->boldW, window->boldFontName);
      fd->boldW->do_callback();
      NeTextSetString(fd->italicW, window->italicFontName);
      fd->italicW->do_callback();
      NeTextSetString(fd->boldItalicW, window->boldItalicFontName);
      fd->boldItalicW->do_callback();
   }
   else
   {
      NeTextSetString(fd->primaryW, GetPrefFontName());
      fd->primaryW->do_callback();
      NeTextSetString(fd->boldW, GetPrefBoldFontName());
      fd->boldW->do_callback();
      NeTextSetString(fd->italicW, GetPrefItalicFontName());
      fd->italicW->do_callback();
      NeTextSetString(fd->boldItalicW, GetPrefBoldItalicFontName());
      fd->boldItalicW->do_callback();
   }

   ManageDialogCenteredOnPointer(form);

   form->show();
}

// --------------------------------------------------------------------------
static void fillFromPrimaryCB(Fl_Widget* w, void* data)
{
   TRACE();
   fontDialog* fd = (fontDialog*)data;

   std::string primaryName = fd->primaryW->value();

   Ne_Font font;
   if (!font.init(primaryName))
   {
      fl_beep();
      return;
   }

   if (primaryName.find("italic") == std::string::npos)
   {
      std::string::size_type found = primaryName.rfind(' ');
      std::string boldName = primaryName.substr(0,found) + " italic " + primaryName.substr(found+1);
      NeTextSetString(fd->italicW, boldName.c_str(), true);
   }

   if (primaryName.find("bold") == std::string::npos)
   {
      std::string::size_type found = primaryName.rfind(' ');
      std::string boldName = primaryName.substr(0,found) + " bold " + primaryName.substr(found+1);
      NeTextSetString(fd->boldW, boldName.c_str(), true);
   }

   if (primaryName.find("bold italic") == std::string::npos)
   {
      std::string::size_type found = primaryName.rfind(' ');
      std::string boldName = primaryName.substr(0,found) + " bold italic " + primaryName.substr(found+1);
      NeTextSetString(fd->boldItalicW, boldName.c_str(), true);
   }
}

// --------------------------------------------------------------------------
static void primaryModifiedCB(Fl_Widget* w, void* data)
{
   TRACE();
   fontDialog* fd = (fontDialog*)data;

   showFontStatus(fd, fd->italicW, fd->italicErrW);
   showFontStatus(fd, fd->boldW, fd->boldErrW);
   showFontStatus(fd, fd->boldItalicW, fd->boldItalicErrW);
}

// --------------------------------------------------------------------------
static void italicModifiedCB(Fl_Widget* w, void* data)
{
   TRACE();
   fontDialog* fd = (fontDialog*)data;

   showFontStatus(fd, fd->italicW, fd->italicErrW);
}

// --------------------------------------------------------------------------
static void boldModifiedCB(Fl_Widget* w, void* data)
{
   TRACE();
   fontDialog* fd = (fontDialog*)data;

   showFontStatus(fd, fd->boldW, fd->boldErrW);
}

// --------------------------------------------------------------------------
static void boldItalicModifiedCB(Fl_Widget* w, void* data)
{
   TRACE();
   fontDialog* fd = (fontDialog*)data;

   showFontStatus(fd, fd->boldItalicW, fd->boldItalicErrW);
}

// --------------------------------------------------------------------------
static void primaryBrowseCB(Fl_Widget* w, void* data)
{
   TRACE();
   fontDialog* fd = (fontDialog*)data;

   browseFont(fd->mainWindow, fd->primaryW);
}

// --------------------------------------------------------------------------
static void italicBrowseCB(Fl_Widget* w, void* data)
{
   TRACE();
   fontDialog* fd = (fontDialog*)data;

   browseFont(fd->mainWindow, fd->italicW);
}

// --------------------------------------------------------------------------
static void boldBrowseCB(Fl_Widget* w, void* data)
{
   TRACE();
   fontDialog* fd = (fontDialog*)data;

   browseFont(fd->mainWindow, fd->boldW);
}

// --------------------------------------------------------------------------
static void boldItalicBrowseCB(Fl_Widget* w, void* data)
{
   TRACE();
   fontDialog* fd = (fontDialog*)data;

   browseFont(fd->mainWindow, fd->boldItalicW);
}

// --------------------------------------------------------------------------
static void fontDestroyCB(Fl_Widget* w, void* data)
{
   fontDialog* fd = (fontDialog*)data;

// TODO:    delete fd->window->fontDialog;
 TODO:    fd->window->fontDialog = NULL;
// TODO:    delete fd;
   WidgetToMainWindow(w)->hide();
}

// --------------------------------------------------------------------------
static void fontOkCB(Fl_Widget* w, void* data)
{
   TRACE();
   fontDialog* fd = (fontDialog*)data;

   updateFonts(fd);
   
   // pop down and destroy the dialog
   fontDestroyCB(fd->mainWindow, fd);
}

// --------------------------------------------------------------------------
static void fontApplyCB(Fl_Widget* w, void* data)
{
   TRACE();
   fontDialog* fd = (fontDialog*)data;

   updateFonts(fd);
}

// --------------------------------------------------------------------------
static void fontCancelCB(Fl_Widget* w, void* data)
{
   TRACE();
   fontDialog* fd = (fontDialog*)data;

   // pop down and destroy the dialog
   fontDestroyCB(fd->mainWindow, fd);
}

// --------------------------------------------------------------------------
// Check over a font name in a text field to make sure it agrees with the
// primary font in height and spacing.
// --------------------------------------------------------------------------
static int checkFontStatus(fontDialog* fd, Fl_Input* fontTextFieldW)
{
   std::string testName = fontTextFieldW->value();

   if (testName.empty())
      return BAD_FONT;
   
   Ne_Font testFont;
   if (!testFont.init(testName))
      return BAD_FONT;

   // Get width and height of the primary font
   std::string primaryName = fd->primaryW->value();
   if (primaryName.empty())
      return BAD_PRIMARY;

   Ne_Font primaryFont;
   if (!primaryFont.init(primaryName))
      return BAD_PRIMARY;

   // TODO:    primaryWidth = primaryFont->min_bounds.width;
   // TODO:    primaryHeight = primaryFont->ascent + primaryFont->descent;
   // TODO:    XFreeFont(display, primaryFont);
   // TODO:
   // TODO:    /* Compare font information */
   // TODO:    if (testWidth != primaryWidth)
   // TODO:       return BAD_SPACING;
   // TODO:    if (testHeight != primaryHeight)
   // TODO:       return BAD_SIZE;
   return GOOD_FONT;
}

// --------------------------------------------------------------------------
// Update the error label for a font text field to reflect its validity and degree
// of agreement with the currently selected primary font
// --------------------------------------------------------------------------
static int showFontStatus(fontDialog* fd, Fl_Input* fontTextFieldW, Fl_Box* errorLabelW)
{
   const char* msg = "";

   int status = checkFontStatus(fd, fontTextFieldW);
   if (status == BAD_PRIMARY)
      msg = "(font below may not match primary font)";
   else if (status == BAD_FONT)
      msg = "(xxx font below is invalid xxx)";
   else if (status == BAD_SIZE)
      msg = "(height of font below does not match primary)";
   else if (status == BAD_SPACING)
      msg = "(spacing of font below does not match primary)";

   errorLabelW->copy_label(msg);
   errorLabelW->redraw_label();

   return status;
}

// --------------------------------------------------------------------------
// Put up a font selector panel to set the font name in the text widget "fontTextW"
// --------------------------------------------------------------------------
static void browseFont(Fl_Widget* parent, Fl_Input* fontTextW)
{
   char* origFontName = NeTextGetString(fontTextW);

   // Get the values from the defaults
   int dummy;
   Fl_Color fgPixel = GetColor(GetPrefColorName(TEXT_FG_COLOR));
   Fl_Color bgPixel = GetColor(GetPrefColorName(TEXT_BG_COLOR));

   std::string newFontName = FontSel(parent, PREF_FIXED, origFontName, fgPixel, bgPixel);
   free__(origFontName);

   if (newFontName.empty())
      return;

   NeTextSetString(fontTextW, newFontName.c_str());
   fontTextW->do_callback();
}

// --------------------------------------------------------------------------
// Accept the changes in the dialog and set the fonts regardless of errors
// --------------------------------------------------------------------------
static void updateFonts(fontDialog* fd)
{
   char* fontName = NeTextGetString(fd->primaryW);
   char* italicName = NeTextGetString(fd->italicW);
   char* boldName = NeTextGetString(fd->boldW);
   char* boldItalicName = NeTextGetString(fd->boldItalicW);

   if (fd->forWindow)
   {
      const char* params[4] = { fontName, italicName, boldName, boldItalicName };
      AppContext.callAction(fd->window->textArea, "set_fonts", NULL, params, 4);
   }
   else
   {
      SetPrefFont(fontName);
      SetPrefItalicFont(italicName);
      SetPrefBoldFont(boldName);
      SetPrefBoldItalicFont(boldItalicName);
   }
   delete[] fontName;
   delete[] italicName;
   delete[] boldName;
   delete[] boldItalicName;
}

// --------------------------------------------------------------------------
// Change the language mode to the one indexed by "mode", reseting word
// delimiters, syntax highlighting and other mode specific parameters
// --------------------------------------------------------------------------
static void reapplyLanguageMode(WindowInfo* window, int mode, int forceDefaults)
{
   char* delimiters;
   int i, wrapMode, indentStyle, tabDist, emTabDist, highlight, oldEmTabDist;
   int wrapModeIsDef, tabDistIsDef, emTabDistIsDef, indentStyleIsDef;
   int highlightIsDef, haveHighlightPatterns, haveSmartIndentMacros;
   int oldMode = window->languageMode;

   /* If the mode is the same, and changes aren't being forced (as might
      happen with Save As...), don't mess with already correct settings */
   if (window->languageMode == mode && !forceDefaults)
      return;

   /* Change the mode name stored in the window */
   window->languageMode = mode;

   /* Decref oldMode's default calltips file if needed */
   if (oldMode != PLAIN_LANGUAGE_MODE && LanguageModes[oldMode]->defTipsFile)
   {
      DeleteTagsFile(LanguageModes[oldMode]->defTipsFile, TIP, false);
   }

   /* Set delimiters for all text widgets */
   if (mode == PLAIN_LANGUAGE_MODE || LanguageModes[mode]->delimiters == NULL)
      delimiters = GetPrefDelimiters();
   else
      delimiters = LanguageModes[mode]->delimiters;
   window->textArea->text.delimiters = delimiters;
   for (i=0; i<window->nPanes; i++)
      window->textPanes[i]->text.delimiters = delimiters;

   /* Decide on desired values for language-specific parameters.  If a
      parameter was set to its default value, set it to the new default,
      otherwise, leave it alone */
   wrapModeIsDef = window->wrapMode == GetPrefWrap(oldMode);
   tabDistIsDef = BufGetTabDistance(window->buffer) == GetPrefTabDist(oldMode);
   oldEmTabDist = window->textArea->text.emulateTabs;
   emTabDistIsDef = oldEmTabDist == GetPrefEmTabDist(oldMode);
   indentStyleIsDef = window->indentStyle == GetPrefAutoIndent(oldMode) ||
                      (GetPrefAutoIndent(oldMode) == SMART_INDENT &&
                       window->indentStyle == AUTO_INDENT &&
                       !SmartIndentMacrosAvailable(LanguageModeName(oldMode)));
   highlightIsDef = window->highlightSyntax == GetPrefHighlightSyntax()
                    || (GetPrefHighlightSyntax() &&
                        FindPatternSet(LanguageModeName(oldMode)) == NULL);
   wrapMode = wrapModeIsDef || forceDefaults ?
              GetPrefWrap(mode) : window->wrapMode;
   tabDist = tabDistIsDef || forceDefaults ?
             GetPrefTabDist(mode) : BufGetTabDistance(window->buffer);
   emTabDist = emTabDistIsDef || forceDefaults ?
               GetPrefEmTabDist(mode) : oldEmTabDist;
   indentStyle = indentStyleIsDef || forceDefaults ?
                 GetPrefAutoIndent(mode) : window->indentStyle;
   highlight = highlightIsDef || forceDefaults ?
               GetPrefHighlightSyntax() : window->highlightSyntax;

   /* Dim/undim smart-indent and highlighting menu items depending on
      whether patterns/macros are available */
   haveHighlightPatterns = FindPatternSet(LanguageModeName(mode)) != NULL;
   haveSmartIndentMacros = SmartIndentMacrosAvailable(LanguageModeName(mode));
   if (IsTopDocument(window))
   {
// TODO:       NeSetSensitive(window->highlightItem, haveHighlightPatterns);
// TODO:       NeSetSensitive(window->smartIndentItem, haveSmartIndentMacros);
   }

   /* Turn off requested options which are not available */
   highlight = haveHighlightPatterns && highlight;
   if (indentStyle == SMART_INDENT && !haveSmartIndentMacros)
      indentStyle = AUTO_INDENT;

   /* Change highlighting */
   window->highlightSyntax = highlight;
   SetToggleButtonState(window, window->highlightItem, highlight, false);
   StopHighlighting(window);

   /* we defer highlighting to RaiseDocument() if doc is hidden */
   if (IsTopDocument(window) && highlight)
      StartHighlighting(window, false);

   /* Force a change of smart indent macros (SetAutoIndent will re-start) */
   if (window->indentStyle == SMART_INDENT)
   {
      EndSmartIndent(window);
      window->indentStyle = AUTO_INDENT;
   }

   /* set requested wrap, indent, and tabs */
   SetAutoWrap(window, wrapMode);
   SetAutoIndent(window, indentStyle);
   SetTabDist(window, tabDist);
   SetEmTabDist(window, emTabDist);

   /* Load calltips files for new mode */
   if (mode != PLAIN_LANGUAGE_MODE && LanguageModes[mode]->defTipsFile)
   {
      AddTagsFile(LanguageModes[mode]->defTipsFile, TIP);
   }

   /* Add/remove language specific menu items */
// TODO:    UpdateUserMenus(window);
}

/*
** Find and return the name of the appropriate languange mode for
** the file in "window".  Returns a pointer to a string, which will
** remain valid until a change is made to the language modes list.
*/
static int matchLanguageMode(WindowInfo* window)
{
   char* ext, *first200;
   int i, j, fileNameLen, extLen, beginPos, endPos, start;
   const char* versionExtendedPath;

   /*... look for an explicit mode statement first */
   /* Do a regular expression search on for recognition pattern */
   first200 = BufGetRange(window->buffer, 0, 200);
   for (i=0; i<NLanguageModes; i++)
   {
      if (LanguageModes[i]->recognitionExpr != NULL)
      {
         if (SearchString(first200, LanguageModes[i]->recognitionExpr,
                          SEARCH_FORWARD, SEARCH_REGEX, false, 0, &beginPos,
                          &endPos, NULL, NULL, NULL))
         {
            free__(first200);
            return i;
         }
      }
   }
   free__(first200);

   /* Look at file extension ("@@/" starts a ClearCase version extended path,
      which gets appended after the file extension, and therefore must be
      stripped off to recognize the extension to make ClearCase users happy) */
   fileNameLen = strlen(window->filename);
   if (strchr(window->filename, ';') != NULL)
      fileNameLen = strchr(window->filename, ';') - window->filename;
   for (i=0; i<NLanguageModes; i++)
   {
      for (j=0; j<LanguageModes[i]->nExtensions; j++)
      {
         ext = LanguageModes[i]->extensions[j];
         extLen = strlen(ext);
         start = fileNameLen - extLen;
         if (start >= 0 && !strncmp(&window->filename[start], ext, extLen))
            return i;
      }
   }

   /* no appropriate mode was found */
   return PLAIN_LANGUAGE_MODE;
}

static int loadLanguageModesString(char* inString, int fileVer)
{
   char* errMsg, *styleName, *inPtr = inString;
   ;
   int i;

   for (;;)
   {

      /* skip over blank space */
      inPtr += strspn(inPtr, " \t\n");

      /* Allocate a language mode structure to return, set unread fields to
         empty so everything can be freed on errors by freeLanguageModeRec */
      languageModeRec* lm = new languageModeRec();
      lm->nExtensions = 0;
      lm->recognitionExpr = NULL;
      lm->defTipsFile = NULL;
      lm->delimiters = NULL;

      /* read language mode name */
      lm->name = ReadSymbolicField(&inPtr);
      if (lm->name == NULL)
      {
         free__((char*)lm);
         return modeError(NULL, inString, inPtr, "language mode name required");
      }
      if (!SkipDelimiter(&inPtr, &errMsg))
         return modeError(lm, inString, inPtr, errMsg);

      /* read list of extensions */
      lm->extensions = readExtensionList(&inPtr,
                                         &lm->nExtensions);
      if (!SkipDelimiter(&inPtr, &errMsg))
         return modeError(lm, inString, inPtr, errMsg);

      /* read the recognition regular expression */
      if (*inPtr == '\n' || *inPtr == '\0' || *inPtr == ':')
         lm->recognitionExpr = NULL;
      else if (!ReadQuotedString(&inPtr, &errMsg, &lm->recognitionExpr))
         return modeError(lm, inString, inPtr, errMsg);
      if (!SkipDelimiter(&inPtr, &errMsg))
         return modeError(lm, inString, inPtr, errMsg);

      /* read the indent style */
      styleName = ReadSymbolicField(&inPtr);
      if (styleName == NULL)
         lm->indentStyle = DEFAULT_INDENT;
      else
      {
         for (i = 0; i < N_INDENT_STYLES; i++)
         {
            if (!strcmp(styleName, AutoIndentTypes[i]))
            {
               lm->indentStyle = i;
               break;
            }
         }
         delete[] styleName;
         if (i == N_INDENT_STYLES)
            return modeError(lm, inString, inPtr, "unrecognized indent style");
      }
      if (!SkipDelimiter(&inPtr, &errMsg))
         return modeError(lm, inString, inPtr, errMsg);

      /* read the wrap style */
      styleName = ReadSymbolicField(&inPtr);
      if (styleName == NULL)
         lm->wrapStyle = DEFAULT_WRAP;
      else
      {
         for (i = 0; i < N_WRAP_STYLES; i++)
         {
            if (!strcmp(styleName, AutoWrapTypes[i]))
            {
               lm->wrapStyle = i;
               break;
            }
         }
         delete[] styleName;
         if (i == N_WRAP_STYLES)
            return modeError(lm, inString, inPtr, "unrecognized wrap style");
      }
      if (!SkipDelimiter(&inPtr, &errMsg))
         return modeError(lm, inString, inPtr, errMsg);

      /* read the tab distance */
      if (*inPtr == '\n' || *inPtr == '\0' || *inPtr == ':')
         lm->tabDist = DEFAULT_TAB_DIST;
      else if (!ReadNumericField(&inPtr, &lm->tabDist))
         return modeError(lm, inString, inPtr, "bad tab spacing");
      if (!SkipDelimiter(&inPtr, &errMsg))
         return modeError(lm, inString, inPtr, errMsg);

      /* read emulated tab distance */
      if (*inPtr == '\n' || *inPtr == '\0' || *inPtr == ':')
         lm->emTabDist = DEFAULT_EM_TAB_DIST;
      else if (!ReadNumericField(&inPtr, &lm->emTabDist))
         return modeError(lm, inString, inPtr, "bad emulated tab spacing");
      if (!SkipDelimiter(&inPtr, &errMsg))
         return modeError(lm, inString, inPtr, errMsg);

      /* read the delimiters string */
      if (*inPtr == '\n' || *inPtr == '\0' || *inPtr == ':')
         lm->delimiters = NULL;
      else if (!ReadQuotedString(&inPtr, &errMsg, &lm->delimiters))
         return modeError(lm, inString, inPtr, errMsg);

      /* After 5.3 all language modes need a default tips file field */
      if (!SkipDelimiter(&inPtr, &errMsg))
         if (fileVer > 5003)
            return modeError(lm, inString, inPtr, errMsg);

      /* read the default tips file */
      if (*inPtr == '\n' || *inPtr == '\0')
         lm->defTipsFile = NULL;
      else if (!ReadQuotedString(&inPtr, &errMsg, &lm->defTipsFile))
         return modeError(lm, inString, inPtr, errMsg);

      /* pattern set was read correctly, add/replace it in the list */
      for (i = 0; i < NLanguageModes; i++)
      {
         if (!strcmp(LanguageModes[i]->name, lm->name))
         {
            freeLanguageModeRec(LanguageModes[i]);
            LanguageModes[i] = lm;
            break;
         }
      }
      if (i == NLanguageModes)
      {
         LanguageModes[NLanguageModes++] = lm;
         if (NLanguageModes > MAX_LANGUAGE_MODES)
            return modeError(NULL, inString, inPtr,
                             "maximum allowable number of language modes exceeded");
      }

      /* if the string ends here, we're done */
      inPtr += strspn(inPtr, " \t\n");
      if (*inPtr == '\0')
         return true;
   } /* End for(;;) */
}

static char* writeLanguageModesString()
{
   int i;
   char* outStr, *escapedStr, *str, numBuf[25];
   Ne_Text_Buffer* outBuf;

   outBuf = BufCreate();
   for (i = 0; i < NLanguageModes; i++)
   {
      BufInsert(outBuf, outBuf->length, "\t");
      BufInsert(outBuf, outBuf->length, LanguageModes[i]->name);
      BufInsert(outBuf, outBuf->length, ":");
      BufInsert(outBuf, outBuf->length, str = createExtString(
            LanguageModes[i]->extensions, LanguageModes[i]->nExtensions));
      delete[] str;
      BufInsert(outBuf, outBuf->length, ":");
      if (LanguageModes[i]->recognitionExpr != NULL)
      {
         BufInsert(outBuf, outBuf->length, str = MakeQuotedString(LanguageModes[i]->recognitionExpr));
         delete[] str;
      }
      BufInsert(outBuf, outBuf->length, ":");
      if (LanguageModes[i]->indentStyle != DEFAULT_INDENT)
         BufInsert(outBuf, outBuf->length,
                   AutoIndentTypes[LanguageModes[i]->indentStyle]);
      BufInsert(outBuf, outBuf->length, ":");
      if (LanguageModes[i]->wrapStyle != DEFAULT_WRAP)
         BufInsert(outBuf, outBuf->length,
                   AutoWrapTypes[LanguageModes[i]->wrapStyle]);
      BufInsert(outBuf, outBuf->length, ":");
      if (LanguageModes[i]->tabDist != DEFAULT_TAB_DIST)
      {
         sprintf(numBuf, "%d", LanguageModes[i]->tabDist);
         BufInsert(outBuf, outBuf->length, numBuf);
      }
      BufInsert(outBuf, outBuf->length, ":");
      if (LanguageModes[i]->emTabDist != DEFAULT_EM_TAB_DIST)
      {
         sprintf(numBuf, "%d", LanguageModes[i]->emTabDist);
         BufInsert(outBuf, outBuf->length, numBuf);
      }
      BufInsert(outBuf, outBuf->length, ":");
      if (LanguageModes[i]->delimiters != NULL)
      {
         BufInsert(outBuf, outBuf->length,
                   str = MakeQuotedString(LanguageModes[i]->delimiters));
         delete[] str;
      }
      BufInsert(outBuf, outBuf->length, ":");
      if (LanguageModes[i]->defTipsFile != NULL)
      {
         BufInsert(outBuf, outBuf->length,
                   str = MakeQuotedString(LanguageModes[i]->defTipsFile));
         delete[] str;
      }

      BufInsert(outBuf, outBuf->length, "\n");
   }

   /* Get the output, and lop off the trailing newline */
   outStr = BufGetRange(outBuf, 0, outBuf->length - 1);
   BufFree(outBuf);
   escapedStr = EscapeSensitiveChars(outStr);
   delete[] outStr;
   return escapedStr;
}

static char* createExtString(char** extensions, int nExtensions)
{
   int e, length = 1;
   char* outStr, *outPtr;

   for (e = 0; e < nExtensions; e++)
      length += strlen(extensions[e]) + 1;
   outStr = outPtr = new char[length];
   for (e = 0; e < nExtensions; e++)
   {
      strcpy(outPtr, extensions[e]);
      outPtr += strlen(extensions[e]);
      *outPtr++ = ' ';
   }
   if (nExtensions == 0)
      *outPtr = '\0';
   else
      *(outPtr - 1) = '\0';
   return outStr;
}

static char** readExtensionList(char** inPtr, int* nExtensions)
{
   char* extensionList[MAX_FILE_EXTENSIONS];
   char** retList, *strStart;
   int i, len;

   /* skip over blank space */
   *inPtr += strspn(*inPtr, " \t");

   for (i = 0; i < MAX_FILE_EXTENSIONS &&** inPtr != ':' &&** inPtr != '\0'; i++)
   {
      *inPtr += strspn(*inPtr, " \t");
      strStart = *inPtr;
      while (**inPtr != ' ' &&** inPtr != '\t' &&** inPtr != ':' &&** inPtr != '\0')
         (*inPtr)++;
      len = *inPtr - strStart;
      extensionList[i] = new char[len + 1];
      strncpy(extensionList[i], strStart, len);
      extensionList[i][len] = '\0';
   }
   *nExtensions = i;
   if (i == 0)
      return NULL;
   retList = new char*[sizeof(char*) * i];
   memcpy(retList, extensionList, sizeof(char*) * i);
   return retList;
}

int ReadNumericField(char** inPtr, int* value)
{
   int charsRead;

   /* skip over blank space */
   *inPtr += strspn(*inPtr, " \t");

   if (sscanf(*inPtr, "%d%n", value, &charsRead) != 1)
      return false;
   *inPtr += charsRead;
   return true;
}

/*
** Parse a symbolic field, skipping initial and trailing whitespace,
** stops on first invalid character or end of string.  Valid characters
** are letters, numbers, _, -, +, $, #, and internal whitespace.  Internal
** whitespace is compressed to single space characters.
*/
char* ReadSymbolicField(char** inPtr)
{
   char* outStr, *outPtr, *strStart, *strPtr;
   int len;

   /* skip over initial blank space */
   *inPtr += strspn(*inPtr, " \t");

   /* Find the first invalid character or end of string to know how
      much memory to allocate for the returned string */
   strStart = *inPtr;
   while (isalnum((unsigned char)** inPtr) ||** inPtr == '_' ||** inPtr == '-' ||
          ** inPtr == '+' ||** inPtr == '$' ||** inPtr == '#' ||** inPtr == ' ' ||
          ** inPtr == '\t')
      (*inPtr)++;
   len = *inPtr - strStart;
   if (len == 0)
      return NULL;
   outStr = outPtr = new char[len + 1];

   /* Copy the string, compressing internal whitespace to a single space */
   strPtr = strStart;
   while (strPtr - strStart < len)
   {
      if (*strPtr == ' ' || *strPtr == '\t')
      {
         strPtr += strspn(strPtr, " \t");
         *outPtr++ = ' ';
      }
      else
         *outPtr++ = *strPtr++;
   }

   /* If there's space on the end, take it back off */
   if (outPtr > outStr && *(outPtr - 1) == ' ')
      outPtr--;
   if (outPtr == outStr)
   {
      free__(outStr);
      return NULL;
   }
   *outPtr = '\0';
   return outStr;
}

/*
** parse an individual quoted string.  Anything between
** double quotes is acceptable, quote characters can be escaped by "".
** Returns allocated string "string" containing
** argument minus quotes.  If not successful, returns false with
** (statically allocated) message in "errMsg".
*/
int ReadQuotedString(char** inPtr, char** errMsg, char** string)
{
   char* outPtr, *c;

   /* skip over blank space */
   *inPtr += strspn(*inPtr, " \t");

   /* look for initial quote */
   if (**inPtr != '\"')
   {
      *errMsg = "expecting quoted string";
      return false;
   }
   (*inPtr)++;

   /* calculate max length and allocate returned string */
   for (c = *inPtr; ; c++)
   {
      if (*c == '\0')
      {
         *errMsg = "string not terminated";
         return false;
      }
      else if (*c == '\"')
      {
         if (*(c + 1) == '\"')
            c++;
         else
            break;
      }
   }

   /* copy string up to end quote, transforming escaped quotes into quotes */
   *string = new char[c - *inPtr + 1];
   outPtr = *string;
   while (true)
   {
      if (**inPtr == '\"')
      {
         if (*(*inPtr + 1) == '\"')
            (*inPtr)++;
         else
            break;
      }
      *outPtr++ = *(*inPtr)++;
   }
   *outPtr = '\0';

   /* skip end quote */
   (*inPtr)++;
   return true;
}

/*
** Replace characters which the X resource file reader considers control
** characters, such that a string will read back as it appears in "string".
** (So far, newline characters are replaced with with \n\<newline> and
** backslashes with \\.  This has not been tested exhaustively, and
** probably should be.  It would certainly be more asthetic if other
** control characters were replaced as well).
**
** Returns an allocated string which must be freed by the caller with free__.
*/
char* EscapeSensitiveChars(const char* string)
{
   const char* c;
   char* outStr, *outPtr;
   int length = 0;

   /* calculate length and allocate returned string */
   for (c = string; *c != '\0'; c++)
   {
      if (*c == '\\')
         length++;
      else if (*c == '\n')
         length += 3;
      length++;
   }
   outStr = new char[length + 1];
   outPtr = outStr;

   /* add backslashes */
   for (c = string; *c != '\0'; c++)
   {
      if (*c == '\\')
         *outPtr++ = '\\';
      else if (*c == '\n')
      {
         *outPtr++ = '\\';
         *outPtr++ = 'n';
         *outPtr++ = '\\';
      }
      *outPtr++ = *c;
   }
   *outPtr = '\0';
   return outStr;
}

/*
** Adds double quotes around a string and escape existing double quote
** characters with two double quotes.  Enables the string to be read back
** by ReadQuotedString.
*/
char* MakeQuotedString(const char* string)
{
   const char* c;
   char* outStr, *outPtr;
   int length = 0;

   /* calculate length and allocate returned string */
   for (c = string; *c != '\0'; c++)
   {
      if (*c == '\"')
         length++;
      length++;
   }
   outStr = new char[length + 3];
   outPtr = outStr;

   /* add starting quote */
   *outPtr++ = '\"';

   /* copy string, escaping quotes with "" */
   for (c = string; *c != '\0'; c++)
   {
      if (*c == '\"')
         *outPtr++ = '\"';
      *outPtr++ = *c;
   }

   /* add ending quote */
   *outPtr++ = '\"';

   /* terminate string and return */
   *outPtr = '\0';
   return outStr;
}

/*
** Read a dialog text field containing a symbolic name (language mode names,
** style names, highlight pattern names, colors, and fonts), clean the
** entered text of leading and trailing whitespace, compress all
** internal whitespace to one space character, and check it over for
** colons, which interfere with the preferences file reader/writer syntax.
** Returns NULL on error, and puts up a dialog if silent is false.  Returns
** an empty string if the text field is blank.
*/
char* ReadSymbolicFieldTextWidget(Fl_Input* textW, const char* fieldName, int silent)
{
   char* str, *strPtr, *parsedString = NULL;

   // read from the text widget
   str = strPtr = NeTextGetString(textW);

   // parse it with the same routine used to read symbolic fields from
   // files.  If the string is not read entirely, there are invalid
   // characters, so warn the user if not in silent mode.
   parsedString = ReadSymbolicField(&strPtr);
   if (*strPtr != '\0')
   {
      if (!silent)
      {
         *(strPtr + 1) = '\0';
         DialogF(DF_WARN, textW, 1, "Invalid Character", "Invalid character \"%s\" in %s", "OK", strPtr, fieldName);
         // TODO:          XmProcessTraversal(textW, XmTRAVERSE_CURRENT);
      }
      delete[] str;
      delete[] parsedString;
      return NULL;
   }
   delete[] str;
   if (parsedString == NULL)
   {
      parsedString = new char[1];
      *parsedString = '\0';
   }
   return parsedString;
}

/*
** Create a pulldown menu pane with the names of the current language modes.
** XmNuserData for each item contains the language mode name.
*/
Fl_Widget* CreateLanguageModeMenu(Fl_Widget* parent, void* data)
{
   Fl_Widget* menu = NULL, *btn;
   int i;
   // TODO:    NeString s1;
   // TODO:
   // TODO:    menu = CreatePulldownMenu(parent, "languageModes", NULL, 0);
   // TODO:    for (i=0; i<NLanguageModes; i++)
   // TODO:    {
   // TODO:       btn = XtVaCreateManagedWidget("languageMode", xmPushButtonGadgetClass,
   // TODO:                                     menu,
   // TODO:                                     XmNlabelString, s1=NeNewString(LanguageModes[i]->name),
   // TODO:                                     XmNmarginHeight, 0,
   // TODO:                                     XmNuserData, (void*)LanguageModes[i]->name, NULL);
   // TODO:       NeStringFree(s1);
   // TODO:       XtAddCallback(btn, XmNactivateCallback, cbProc, cbArg);
   // TODO:    }
   return menu;
}

// --------------------------------------------------------------------------
// Add Choice entries into a Fl_Choice
// --------------------------------------------------------------------------
void CreateLanguageModeChoice(Fl_Choice* choice)
{
   choice->clear();
   for (int i=0; i<NLanguageModes; i++)
      choice->add(LanguageModes[i]->name);
}

// --------------------------------------------------------------------------
// Set the language mode menu in option menu "optMenu" to
// show a particular language mode
// --------------------------------------------------------------------------
void SetLangModeMenu(Fl_Choice* optMenu, const char* modeName)
{
   int index = optMenu->find_index(modeName);
   if (index != -1)
      optMenu->value(index);
}

/*
** Create a submenu for chosing language mode for the current window.
*/
void CreateLanguageModeSubMenu(WindowInfo* window, Ne_MenuBar* menuBar, const Ne_MenuLevel& level, 
                               const char* name, const char* label, const char mnemonic)
{
   // Adding Language sub menu
   menuBar->add_menu(name, level + label, mnemonic, NULL);

   updateLanguageModeSubmenu(window);
}

// --------------------------------------------------------------------------
// Re-build the language mode sub-menu using the current data stored
// in the master list: LanguageModes.
// --------------------------------------------------------------------------
static void updateLanguageModeSubmenu(WindowInfo* window)
{
   int languagesModeItemIndex = window->menuBar->find_index(window->menuBar->getLanguageModesItem());
   window->menuBar->clear_submenu(languagesModeItemIndex);

   window->menuBar->insert(languagesModeItemIndex+1, "Plain", 0, setLangModeCB, window, FL_MENU_RADIO | (FL_MENU_VALUE *(window->languageMode==PLAIN_LANGUAGE_MODE)));
   for (int i=0; i<NLanguageModes; ++i)
   {
      window->menuBar->insert(languagesModeItemIndex+2+i, LanguageModes[i]->name, 0, setLangModeCB, window, FL_MENU_RADIO | (FL_MENU_VALUE*(window->languageMode==i)));
   }
}

static void setLangModeCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = WidgetToWindow(w);
   Ne_MenuBar* menuBar = (Ne_MenuBar*)w;
   Fl_Menu_Item* item = const_cast<Fl_Menu_Item*>(menuBar->mvalue());

   if (!item->value())
      return;

   // get name of language mode stored in userData field of menu item
   int mode = FindLanguageMode(item->label());

   // If the mode didn't change, do nothing
   if (window->languageMode == (int)mode)
      return;

   // redo syntax highlighting word delimiters, etc.
   /*
       reapplyLanguageMode(window, (int)mode, false);
   */
   const char* params[] = { (((int)mode) == PLAIN_LANGUAGE_MODE) ? "" : LanguageModes[(int)mode]->name };
   AppContext.callAction(window->textArea, "set_language_mode", NULL, params, 1);
}

/*
** Skip a delimiter and it's surrounding whitespace
*/
int SkipDelimiter(char** inPtr, char** errMsg)
{
   *inPtr += strspn(*inPtr, " \t");
   if (**inPtr != ':')
   {
      *errMsg = (char*)"syntax error";
      return false;
   }
   (*inPtr)++;
   *inPtr += strspn(*inPtr, " \t");
   return true;
}

/*
** Skip an optional separator and its surrounding whitespace
** return true if delimiter found
*/
int SkipOptSeparator(char separator, char** inPtr)
{
   *inPtr += strspn(*inPtr, " \t");
   if (**inPtr != separator)
   {
      return false;
   }
   (*inPtr)++;
   *inPtr += strspn(*inPtr, " \t");
   return true;
}

/*
** Short-hand error processing for language mode parsing errors, frees
** lm (if non-null), prints a formatted message explaining where the
** error is, and returns false;
*/
static int modeError(languageModeRec* lm, const char* stringStart,
                     const char* stoppedAt, const char* message)
{
   if (lm != NULL)
      freeLanguageModeRec(lm);
   return ParseError(NULL, stringStart, stoppedAt,
                     "language mode specification", message);
}

/*
** Report parsing errors in resource strings or macros, formatted nicely so
** the user can tell where things became botched.  Errors can be sent either
** to stderr, or displayed in a dialog.  For stderr, pass toDialog as NULL.
** For a dialog, pass the dialog parent in toDialog.
*/
int ParseError(Fl_Widget* toDialog, const char* stringStart, const char* stoppedAt,
               const char* errorIn, const char* message)
{
   int len, nNonWhite = 0;
   const char* c;
   char* errorLine;

   for (c = stoppedAt; c >= stringStart; c--)
   {
      if (c == stringStart)
         break;
      else if (*c == '\n' && nNonWhite >= 5)
         break;
      else if (*c != ' ' && *c != '\t')
         nNonWhite++;
   }
   len = stoppedAt - c + (*stoppedAt == '\0' ? 0 : 1);
   errorLine = (char*)malloc__(len + 4);
   strncpy(errorLine, c, len);
   errorLine[len++] = '<';
   errorLine[len++] = '=';
   errorLine[len++] = '=';
   errorLine[len] = '\0';
   if (toDialog == NULL)
   {
      fprintf(stderr, "NEdit: %s in %s:\n%s\n", message, errorIn, errorLine);
   }
   else
   {
      DialogF(DF_WARN, toDialog, 1, "Parse Error", "%s in %s:\n%s", "OK", message, errorIn, errorLine);
   }
   free__(errorLine);
   return false;
}

/*
** Compare two strings which may be NULL
*/
int AllocatedStringsDiffer(const char* s1, const char* s2)
{
   if (s1 == NULL && s2 == NULL)
      return false;
   if (s1 == NULL || s2 == NULL)
      return true;
   return strcmp(s1, s2);
}

static void updatePatternsTo5dot1()
{
   const char* htmlDefaultExpr = "^[ \t]*HTML[ \t]*:[ \t]*Default[ \t]*$";
   const char* vhdlAnchorExpr = "^[ \t]*VHDL:";

   /* Add new patterns if there aren't already existing patterns with
      the same name.  If possible, insert before VHDL in language mode
      list.  If not, just add to end */
   if (!regexFind(TempStringPrefs.highlight, "^[ \t]*PostScript:"))
      spliceString(&TempStringPrefs.highlight, "PostScript:Default",
                   vhdlAnchorExpr);
   if (!regexFind(TempStringPrefs.language, "^[ \t]*PostScript:"))
      spliceString(&TempStringPrefs.language,
                   "PostScript:.ps .PS .eps .EPS .epsf .epsi::::::",
                   vhdlAnchorExpr);
   if (!regexFind(TempStringPrefs.highlight, "^[ \t]*Lex:"))
      spliceString(&TempStringPrefs.highlight, "Lex:Default",
                   vhdlAnchorExpr);
   if (!regexFind(TempStringPrefs.language, "^[ \t]*Lex:"))
      spliceString(&TempStringPrefs.language, "Lex:.lex::::::",
                   vhdlAnchorExpr);
   if (!regexFind(TempStringPrefs.highlight, "^[ \t]*SQL:"))
      spliceString(&TempStringPrefs.highlight, "SQL:Default",
                   vhdlAnchorExpr);
   if (!regexFind(TempStringPrefs.language, "^[ \t]*SQL:"))
      spliceString(&TempStringPrefs.language, "SQL:.sql::::::",
                   vhdlAnchorExpr);
   if (!regexFind(TempStringPrefs.highlight, "^[ \t]*Matlab:"))
      spliceString(&TempStringPrefs.highlight, "Matlab:Default",
                   vhdlAnchorExpr);
   if (!regexFind(TempStringPrefs.language, "^[ \t]*Matlab:"))
      spliceString(&TempStringPrefs.language, "Matlab:..m .oct .sci::::::",
                   vhdlAnchorExpr);
   if (!regexFind(TempStringPrefs.smartIndent, "^[ \t]*Matlab:"))
      spliceString(&TempStringPrefs.smartIndent, "Matlab:Default", NULL);
   if (!regexFind(TempStringPrefs.styles, "^[ \t]*Label:"))
      spliceString(&TempStringPrefs.styles, "Label:red:Italic",
                   "^[ \t]*Flag:");
   if (!regexFind(TempStringPrefs.styles, "^[ \t]*Storage Type1:"))
      spliceString(&TempStringPrefs.styles, "Storage Type1:saddle brown:Bold",
                   "^[ \t]*String:");

   /* Replace html pattern with sgml html pattern, as long as there
      isn't an existing html pattern which will be overwritten */
   if (regexFind(TempStringPrefs.highlight, htmlDefaultExpr))
   {
      regexReplace(&TempStringPrefs.highlight, htmlDefaultExpr,
                   "SGML HTML:Default");
      if (!regexReplace(&TempStringPrefs.language, "^[ \t]*HTML:.*$",
                        "SGML HTML:.sgml .sgm .html .htm:\"\\<(?ihtml)\\>\":::::\n"))
      {
         spliceString(&TempStringPrefs.language,
                      "SGML HTML:.sgml .sgm .html .htm:\"\\<(?ihtml)\\>\":::::\n",
                      vhdlAnchorExpr);
      }
   }
}

static void updatePatternsTo5dot2()
{
   const char* cppLm5dot1 =
      "^[ \t]*C\\+\\+:\\.cc \\.hh \\.C \\.H \\.i \\.cxx \\.hxx::::::\"\\.,/\\\\`'!\\|@#%\\^&\\*\\(\\)-=\\+\\{\\}\\[\\]\"\":;\\<\\>\\?~\"";
   const char* perlLm5dot1 =
      "^[ \t]*Perl:\\.pl \\.pm \\.p5:\"\\^\\[ \\\\t\\]\\*#\\[ \\\\t\\]\\*!\\.\\*perl\":::::";
   const char* psLm5dot1 =
      "^[ \t]*PostScript:\\.ps \\.PS \\.eps \\.EPS \\.epsf \\.epsi:\"\\^%!\":::::\"/%\\(\\)\\{\\}\\[\\]\\<\\>\"";
   const char* shLm5dot1 =
      "^[ \t]*Sh Ksh Bash:\\.sh \\.bash \\.ksh \\.profile:\"\\^\\[ \\\\t\\]\\*#\\[ \\\\t\\]\\*!\\[ \\\\t\\]\\*/bin/\\(sh\\|ksh\\|bash\\)\":::::";
   const char* tclLm5dot1 = "^[ \t]*Tcl:\\.tcl::::::";

   const char* cppLm5dot2 =
      "C++:.cc .hh .C .H .i .cxx .hxx .cpp::::::\".,/\\`'!|@#%^&*()-=+{}[]\"\":;<>?~\"";
   const char* perlLm5dot2 =
      "Perl:.pl .pm .p5 .PL:\"^[ \\t]*#[ \\t]*!.*perl\":Auto:None:::\".,/\\\\`'!$@#%^&*()-=+{}[]\"\":;<>?~|\"";
   const char* psLm5dot2 =
      "PostScript:.ps .eps .epsf .epsi:\"^%!\":::::\"/%(){}[]<>\"";
   const char* shLm5dot2 =
      "Sh Ksh Bash:.sh .bash .ksh .profile .bashrc .bash_logout .bash_login .bash_profile:\"^[ \\t]*#[ \\t]*![ \\t]*/.*bin/(sh|ksh|bash)\":::::";
   const char* tclLm5dot2 =
      "Tcl:.tcl .tk .itcl .itk::Smart:None:::";

   const char* cssLm5dot2 =
      "CSS:css::Auto:None:::\".,/\\`'!|@#%^&*()=+{}[]\"\":;<>?~\"";
   const char* reLm5dot2 =
      "Regex:.reg .regex:\"\\(\\?[:#=!iInN].+\\)\":None:Continuous:::";
   const char* xmlLm5dot2 =
      "XML:.xml .xsl .dtd:\"\\<(?i\\?xml|!doctype)\"::None:::\"<>/=\"\"'()+*?|\"";

   const char* cssHl5dot2 = "CSS:Default";
   const char* reHl5dot2 =  "Regex:Default";
   const char* xmlHl5dot2 = "XML:Default";

   const char* ptrStyle = "Pointer:#660000:Bold";
   const char* reStyle = "Regex:#009944:Bold";
   const char* wrnStyle = "Warning:brown2:Italic";

   // First upgrade modified language modes, only if the user hasn't
   // altered the default 5.1 definitions.
   if (regexFind(TempStringPrefs.language, cppLm5dot1))
      regexReplace(&TempStringPrefs.language, cppLm5dot1, cppLm5dot2);
   if (regexFind(TempStringPrefs.language, perlLm5dot1))
      regexReplace(&TempStringPrefs.language, perlLm5dot1, perlLm5dot2);
   if (regexFind(TempStringPrefs.language, psLm5dot1))
      regexReplace(&TempStringPrefs.language, psLm5dot1, psLm5dot2);
   if (regexFind(TempStringPrefs.language, shLm5dot1))
      regexReplace(&TempStringPrefs.language, shLm5dot1, shLm5dot2);
   if (regexFind(TempStringPrefs.language, tclLm5dot1))
      regexReplace(&TempStringPrefs.language, tclLm5dot1, tclLm5dot2);

   // Then append the new modes (trying to keep them in alphabetical order
   // makes no sense, since 5.1 didn't use alphabetical order).
   if (!regexFind(TempStringPrefs.language, "^[ \t]*CSS:"))
      spliceString(&TempStringPrefs.language, cssLm5dot2, NULL);
   if (!regexFind(TempStringPrefs.language, "^[ \t]*Regex:"))
      spliceString(&TempStringPrefs.language, reLm5dot2, NULL);
   if (!regexFind(TempStringPrefs.language, "^[ \t]*XML:"))
      spliceString(&TempStringPrefs.language, xmlLm5dot2, NULL);

   // Enable default highlighting patterns for these modes, unless already present
   if (!regexFind(TempStringPrefs.highlight, "^[ \t]*CSS:"))
      spliceString(&TempStringPrefs.highlight, cssHl5dot2, NULL);
   if (!regexFind(TempStringPrefs.highlight, "^[ \t]*Regex:"))
      spliceString(&TempStringPrefs.highlight, reHl5dot2, NULL);
   if (!regexFind(TempStringPrefs.highlight, "^[ \t]*XML:"))
      spliceString(&TempStringPrefs.highlight, xmlHl5dot2, NULL);

   // Finally, append the new highlight styles
   if (!regexFind(TempStringPrefs.styles, "^[ \t]*Warning:"))
      spliceString(&TempStringPrefs.styles, wrnStyle, NULL);
   if (!regexFind(TempStringPrefs.styles, "^[ \t]*Regex:"))
      spliceString(&TempStringPrefs.styles, reStyle, "^[ \t]*Warning:");
   if (!regexFind(TempStringPrefs.styles, "^[ \t]*Pointer:"))
      spliceString(&TempStringPrefs.styles, ptrStyle, "^[ \t]*Regex:");
}

static void updatePatternsTo5dot3()
{
}

static void updatePatternsTo5dot4()
{
   const char* pyLm5dot3 =
      "Python:\\.py:\"\\^#!\\.\\*python\":Auto:None::::?\n";
   const char* xrLm5dot3 =
      "X Resources:\\.Xresources \\.Xdefaults \\.nedit:\"\\^\\[!#\\]\\.\\*\\(\\[Aa\\]pp\\|\\[Xx\\]\\)\\.\\*\\[Dd\\]efaults\"::::::?\n";

   const char* pyLm5dot4 =
      "Python:.py:\"^#!.*python\":Auto:None:::\"!\"\"#$%&'()*+,-./:;<=>?@[\\\\]^`{|}~\":\n";
   const char* xrLm5dot4 =
      "X Resources:.Xresources .Xdefaults .nedit nedit.rc:\"^[!#].*([Aa]pp|[Xx]).*[Dd]efaults\"::::::\n";

   /* Upgrade modified language modes, only if the user hasn't
      altered the default 5.3 definitions. */
   if (regexFind(TempStringPrefs.language, pyLm5dot3))
      regexReplace(&TempStringPrefs.language, pyLm5dot3, pyLm5dot4);
   if (regexFind(TempStringPrefs.language, xrLm5dot3))
      regexReplace(&TempStringPrefs.language, xrLm5dot3, xrLm5dot4);

   /* Add new styles */
   if (!regexFind(TempStringPrefs.styles, "^[ \t]*Identifier2:"))
      spliceString(&TempStringPrefs.styles, "Identifier2:SteelBlue:Plain",
                   "^[ \t]*Subroutine:");
}

static void updatePatternsTo5dot6()
{
   const char* pats[] =
   {
      "Csh:\\.csh \\.cshrc \\.login \\.logout:\"\\^\\[ \\\\t\\]\\*#\\[ \\\\t\\]\\*!\\[ \\\\t\\]\\*/bin/csh\"::::::\\n",
      "Csh:.csh .cshrc .tcshrc .login .logout:\"^[ \\t]*#[ \\t]*![ \\t]*/bin/t?csh\"::::::\n",
      "LaTeX:\\.tex \\.sty \\.cls \\.ltx \\.ins:::::::\\n",
      "LaTeX:.tex .sty .cls .ltx .ins .clo .fd:::::::\n",
      "X Resources:\\.Xresources \\.Xdefaults \\.nedit:\"\\^\\[!#\\]\\.\\*\\(\\[Aa\\]pp\\|\\[Xx\\]\\)\\.\\*\\[Dd\\]efaults\"::::::\\n",
      "X Resources:.Xresources .Xdefaults .nedit .pats nedit.rc:\"^[!#].*([Aa]pp|[Xx]).*[Dd]efaults\"::::::\n",
      NULL
   };

   /* Upgrade modified language modes, only if the user hasn't
      altered the default 5.5 definitions. */
   int i;
   for (i = 0; pats[i]; i += 2)
   {
      if (regexFind(TempStringPrefs.language, pats[i]))
         regexReplace(&TempStringPrefs.language, pats[i], pats[i + 1]);
   }

   /* Add new styles */
   if (!regexFind(TempStringPrefs.styles, "^[ \t]*Bracket:"))
      spliceString(&TempStringPrefs.styles, "Bracket:dark blue:Bold",
                   "^[ \t]*Storage Type:");
   if (!regexFind(TempStringPrefs.styles, "^[ \t]*Operator:"))
      spliceString(&TempStringPrefs.styles, "Operator:dark blue:Bold",
                   "^[ \t]*Bracket:");
}


/*
 * We migrate a color from the X resources to the prefs if:
 *      1.  The prefs entry is equal to the default entry
 *      2.  The X resource is not equal to the default entry
 */
static void migrateColor(Ne_Database* prefDB, Ne_Database* appDB,
                         char* wClass, char* name, int color_index, char* default_val)
{
   char* type, *valueString;
   // TODO:    XrmValue rsrcValue;
   // TODO:
   // TODO:    /* If this color has been customized in the color dialog then use
   // TODO:        that value */
   // TODO:    if (strcmp(default_val, PrefData.colorNames[color_index]))
   // TODO:       return;
   // TODO:
   // TODO:    /* Retrieve the value of the resource from the DB */
   // TODO:    if (XrmGetResource(prefDB, name, wClass, &type, &rsrcValue))
   // TODO:    {
   // TODO:       if (strcmp(type, XmRString))
   // TODO:       {
   // TODO:          fprintf(stderr,"Internal Error: Unexpected resource type, %s\n",
   // TODO:                  type);
   // TODO:          return;
   // TODO:       }
   // TODO:       valueString = rsrcValue.addr;
   // TODO:    }
   // TODO:    else if (XrmGetResource(appDB, name, wClass, &type, &rsrcValue))
   // TODO:    {
   // TODO:       if (strcmp(type, XmRString))
   // TODO:       {
   // TODO:          fprintf(stderr,"Internal Error: Unexpected resource type, %s\n",
   // TODO:                  type);
   // TODO:          return;
   // TODO:       }
   // TODO:       valueString = rsrcValue.addr;
   // TODO:    }
   // TODO:    else
   // TODO:       /* No resources set */
   // TODO:       return;
   // TODO:
   // TODO:    /* An X resource is set.  If it's non-default, update the prefs. */
   // TODO:    if (strcmp(valueString, default_val))
   // TODO:    {
   // TODO:       strncpy(PrefData.colorNames[color_index], valueString,
   // TODO:               MAX_COLOR_LEN);
   // TODO:    }
}

/*
 * In 5.4 we moved color preferences from X resources to a color dialog,
 * meaning they're in the normal prefs system.  Users who have customized
 * their colors with X resources would probably prefer not to have to redo
 * the customization in the dialog, so we migrate them to the prefs for them.
 */
static void migrateColorResources(Ne_Database* prefDB, Ne_Database* appDB)
{
   // TODO:    migrateColor(prefDB, appDB, APP_CLASS ".Text.Foreground",
   // TODO:                 APP_NAME ".text.foreground", TEXT_FG_COLOR,
   // TODO:                 NEDIT_DEFAULT_FG);
   // TODO:    migrateColor(prefDB, appDB, APP_CLASS ".Text.Background",
   // TODO:                 APP_NAME ".text.background", TEXT_BG_COLOR,
   // TODO:                 NEDIT_DEFAULT_TEXT_BG);
   // TODO:    migrateColor(prefDB, appDB, APP_CLASS ".Text.SelectForeground",
   // TODO:                 APP_NAME ".text.selectForeground", SELECT_FG_COLOR,
   // TODO:                 NEDIT_DEFAULT_SEL_FG);
   // TODO:    migrateColor(prefDB, appDB, APP_CLASS ".Text.SelectBackground",
   // TODO:                 APP_NAME ".text.selectBackground", SELECT_BG_COLOR,
   // TODO:                 NEDIT_DEFAULT_SEL_BG);
   // TODO:    migrateColor(prefDB, appDB, APP_CLASS ".Text.HighlightForeground",
   // TODO:                 APP_NAME ".text.highlightForeground", HILITE_FG_COLOR,
   // TODO:                 NEDIT_DEFAULT_HI_FG);
   // TODO:    migrateColor(prefDB, appDB, APP_CLASS ".Text.HighlightBackground",
   // TODO:                 APP_NAME ".text.highlightBackground", HILITE_BG_COLOR,
   // TODO:                 NEDIT_DEFAULT_HI_BG);
   // TODO:    migrateColor(prefDB, appDB, APP_CLASS ".Text.LineNumForeground",
   // TODO:                 APP_NAME ".text.lineNumForeground", LINENO_FG_COLOR,
   // TODO:                 NEDIT_DEFAULT_LINENO_FG);
   // TODO:    migrateColor(prefDB, appDB, APP_CLASS ".Text.CursorForeground",
   // TODO:                 APP_NAME ".text.cursorForeground", CURSOR_FG_COLOR,
   // TODO:                 NEDIT_DEFAULT_CURSOR_FG);
}

/*
** Inserts a string into intoString, reallocating it with malloc__.  If
** regular expression atExpr is found, inserts the string before atExpr
** followed by a newline.  If atExpr is not found, inserts insertString
** at the end, PRECEDED by a newline.
*/
static void spliceString(char** intoString, const char* insertString, const char* atExpr)
{
   int beginPos, endPos;
   int intoLen = strlen(*intoString);
   int insertLen = strlen(insertString);
   char* newString = (char*)malloc__(intoLen + insertLen + 2);

   if (atExpr != NULL && SearchString(*intoString, atExpr,
                                      SEARCH_FORWARD, SEARCH_REGEX, false, 0, &beginPos, &endPos,
                                      NULL, NULL, NULL))
   {
      strncpy(newString, *intoString, beginPos);
      strncpy(&newString[beginPos], insertString, insertLen);
      newString[beginPos+insertLen] = '\n';
      strncpy(&newString[beginPos+insertLen+1],
              &((*intoString)[beginPos]), intoLen - beginPos);
   }
   else
   {
      strncpy(newString, *intoString, intoLen);
      newString[intoLen] = '\n';
      strncpy(&newString[intoLen+1], insertString, insertLen);
   }
   newString[intoLen + insertLen + 1] = '\0';
   free__(*intoString);
   *intoString = newString;
}

/*
** Simplified regular expression search routine which just returns true
** or false depending on whether inString matches expr
*/
static int regexFind(const char* inString, const char* expr)
{
   int beginPos, endPos;
   return SearchString(inString, expr, SEARCH_FORWARD, SEARCH_REGEX, false, 0, &beginPos, &endPos, NULL, NULL, NULL);
}

/*
** Simplified case-sensisitive string search routine which just
** returns true or false depending on whether inString matches expr
*/
static int caseFind(const char* inString, const char* expr)
{
   int beginPos, endPos;
   return SearchString(inString, expr, SEARCH_FORWARD, SEARCH_CASE_SENSE, false, 0, &beginPos, &endPos, NULL, NULL, NULL);
}

/*
** Common implementation for simplified string replacement routines.
*/
static int stringReplace(char** inString, const char* expr,
                         const char* replaceWith, int searchType,
                         int replaceLen)
{
   int beginPos, endPos, newLen;
   char* newString;
   int inLen = strlen(*inString);
   if (0 >= replaceLen) replaceLen = strlen(replaceWith);
   if (!SearchString(*inString, expr, SEARCH_FORWARD, searchType, false,
                     0, &beginPos, &endPos, NULL, NULL, NULL))
      return false;
   newLen = inLen + replaceLen - (endPos-beginPos);
   newString = (char*)malloc__(newLen + 1);
   strncpy(newString, *inString, beginPos);
   strncpy(&newString[beginPos], replaceWith, replaceLen);
   strncpy(&newString[beginPos+replaceLen], &((*inString)[endPos]), inLen - endPos);
   newString[newLen] = '\0';
   free__(*inString);
   *inString = newString;
   return true;
}

/*
** Simplified regular expression replacement routine which replaces the
** first occurence of expr in inString with replaceWith, reallocating
** inString with malloc__.  If expr is not found, does nothing and
** returns false.
*/
static int regexReplace(char** inString, const char* expr,
                        const char* replaceWith)
{
   return stringReplace(inString, expr, replaceWith, SEARCH_REGEX, -1);
}

/*
** Simplified case-sensisitive string replacement routine which
** replaces the first occurence of expr in inString with replaceWith,
** reallocating inString with malloc__.  If expr is not found, does nothing
** and returns false.
*/
static int caseReplace(char** inString, const char* expr,
                       const char* replaceWith, int replaceLen)
{
   return stringReplace(inString, expr, replaceWith, SEARCH_CASE_SENSE, replaceLen);
}

/*
** Looks for a (case-sensitive literal) match of an old macro text in a
** temporary macro commands buffer. If the text is found, it is replaced by
** a substring of the default macros, bounded by a given start and end pattern
** (inclusive). Returns the length of the replacement.
*/
static int replaceMacroIfUnchanged(const char* oldText, const char* newStart,
                                   const char* newEnd)
{
   if (caseFind(TempStringPrefs.macroCmds, oldText))
   {
      const char* start = strstr(PrefDescrip[2].defaultString, newStart);
      if (start)
      {
         const char* end = strstr(start, newEnd);
         if (end)
         {
            int length = (int)(end - start) + strlen(newEnd);
            caseReplace(&TempStringPrefs.macroCmds, oldText, start, length);
            return length;
         }
      }
   }
   return 0;
}

/*
** Replace all '#' characters in shell commands by '##' to keep commands
** containing those working. '#' is a line number placeholder in 5.3 and
** had no special meaning before.
*/
static void updateShellCmdsTo5dot3()
{
   char* cOld, *cNew, *pCol, *pNL;
   int  nHash, isCmd;
   char* newString;

   if (!TempStringPrefs.shellCmds)
      return;

   /* Count number of '#'. If there are '#' characters in the non-command
   ** part of the definition we count too much and later allocate too much
   ** memory for the new string, but this doesn't hurt.
   */
   for (cOld = TempStringPrefs.shellCmds, nHash = 0; *cOld; cOld++)
      if (*cOld == '#')
         nHash++;

   /* No '#' -> no conversion necessary. */
   if (!nHash)
      return;

   newString = (char*)malloc__(strlen(TempStringPrefs.shellCmds) + 1 + nHash);

   cOld  = TempStringPrefs.shellCmds;
   cNew  = newString;
   isCmd = 0;
   pCol  = NULL;
   pNL   = NULL;

   /* Copy all characters from TempStringPrefs.shellCmds into newString
   ** and duplicate '#' in command parts. A simple check for really beeing
   ** inside a command part (starting with '\n', between the the two last
   ** '\n' a colon ':' must have been found) is preformed.
   */
   while (*cOld)
   {
      /* actually every 2nd line is a command. We additionally
      ** check if there is a colon ':' in the previous line.
      */
      if (*cOld == '\n')
      {
         if ((pCol > pNL) && !isCmd)
            isCmd = 1;
         else
            isCmd = 0;
         pNL = cOld;
      }

      if (!isCmd && *cOld == ':')
         pCol = cOld;

      /* Duplicate hashes if we're in a command part */
      if (isCmd && *cOld == '#')
         *cNew++ = '#';

      /* Copy every character */
      *cNew++ = *cOld++;

   }

   /* Terminate new preferences string */
   *cNew = 0;

   /* free__ the old memory */
   free__(TempStringPrefs.shellCmds);

   /* exchange the string */
   TempStringPrefs.shellCmds = newString;

}

static void updateShellCmdsTo5dot4()
{
#ifdef __FreeBSD__
   const char* wc5dot3 =
      "^(\\s*)set wc=`wc`; echo \\$wc\\[1\\] \"words,\" \\$wc\\[2\\] \"lines,\" \\$wc\\[3\\] \"characters\"\\n";
   const char* wc5dot4 =
      "wc | awk '{print $2 \" lines, \" $1 \" words, \" $3 \" characters\"}'\n";
#else
   const char* wc5dot3 =
      "^(\\s*)set wc=`wc`; echo \\$wc\\[1\\] \"lines,\" \\$wc\\[2\\] \"words,\" \\$wc\\[3\\] \"characters\"\\n";
   const char* wc5dot4 =
      "wc | awk '{print $1 \" lines, \" $2 \" words, \" $3 \" characters\"}'\n";
#endif /* __FreeBSD__ */

   if (regexFind(TempStringPrefs.shellCmds, wc5dot3))
      regexReplace(&TempStringPrefs.shellCmds, wc5dot3, wc5dot4);

   return;
}

static void updateMacroCmdsTo5dot5()
{
   const char* uc5dot4 =
      "^(\\s*)if \\(substring\\(sel, keepEnd - 1, keepEnd == \" \"\\)\\)\\n";
   const char* uc5dot5 =
      "		if (substring(sel, keepEnd - 1, keepEnd) == \" \")\n";
   if (regexFind(TempStringPrefs.macroCmds, uc5dot4))
      regexReplace(&TempStringPrefs.macroCmds, uc5dot4, uc5dot5);

   return;
}

static void updateMacroCmdsTo5dot6()
{
   /*
      This is ridiculous. Macros don't belong in the default preferences
      string.
      This code is also likely to break when the macro commands are upgraded
      again in a next release, because it looks for patterns in the default
      macro string (which may change).
      Using a "Default" mechanism, like we do for highlighting patterns
      would simplify upgrading A LOT in the future, but changing the way
      default macros are stored, is a lot of work too, unfortunately.
   */
   const char* pats[] =
   {
      "Complete Word:Alt+D::: {\n\
		# Tuning parameters\n\
		ScanDistance = 200\n\
		\n\
		# Search back to a word boundary to find the word to complete\n\
		startScan = max(0, $cursor - ScanDistance)\n\
		endScan = min($text_length, $cursor + ScanDistance)\n\
		scanString = get_range(startScan, endScan)\n\
		keyEnd = $cursor-startScan\n\
		keyStart = search_string(scanString, \"<\", keyEnd, \"backward\", \"regex\")\n\
		if (keyStart == -1)\n\
		    return\n\
		keyString = \"<\" substring(scanString, keyStart, keyEnd)\n\
		\n\
		# search both forward and backward from the cursor position.  Note that\n\
		# using a regex search can lead to incorrect results if any of the special\n\
		# regex characters is encountered, which is not considered a delimiter\n\
		backwardSearchResult = search_string(scanString, keyString, keyStart-1, \\\n\
		    	\"backward\", \"regex\")\n\
		forwardSearchResult = search_string(scanString, keyString, keyEnd, \"regex\")\n\
		if (backwardSearchResult == -1 && forwardSearchResult == -1) {\n\
		    beep()\n\
		    return\n\
		}\n\
		\n\
		# if only one direction matched, use that, otherwise use the nearest\n\
		if (backwardSearchResult == -1)\n\
		    matchStart = forwardSearchResult\n\
		else if (forwardSearchResult == -1)\n\
		    matchStart = backwardSearchResult\n\
		else {\n\
		    if (keyStart - backwardSearchResult <= forwardSearchResult - keyEnd)\n\
		    	matchStart = backwardSearchResult\n\
		    else\n\
		    	matchStart = forwardSearchResult\n\
		}\n\
		\n\
		# find the complete word\n\
		matchEnd = search_string(scanString, \">\", matchStart, \"regex\")\n\
		completedWord = substring(scanString, matchStart, matchEnd)\n\
		\n\
		# replace it in the window\n\
		replace_range(startScan + keyStart, $cursor, completedWord)\n\
	}", "Complete Word:", "\n\t}",
      "Fill Sel. w/Char:::R: {\n\
		if ($selection_start == -1) {\n\
		    beep()\n\
		    return\n\
		}\n\
		\n\
		# Ask the user what character to fill with\n\
		fillChar = string_dialog(\"Fill selection with what character?\", \"OK\", \"Cancel\")\n\
		if ($string_dialog_button == 2 || $string_dialog_button == 0)\n\
		    return\n\
		\n\
		# Count the number of lines in the selection\n\
		nLines = 0\n\
		for (i=$selection_start; i<$selection_end; i++)\n\
		    if (get_character(i) == \"\\n\")\n\
		    	nLines++\n\
		\n\
		# Create the fill text\n\
		rectangular = $selection_left != -1\n\
		line = \"\"\n\
		fillText = \"\"\n\
		if (rectangular) {\n\
		    for (i=0; i<$selection_right-$selection_left; i++)\n\
			line = line fillChar\n\
		    for (i=0; i<nLines; i++)\n\
			fillText = fillText line \"\\n\"\n\
		    fillText = fillText line\n\
		} else {\n\
		    if (nLines == 0) {\n\
		    	for (i=$selection_start; i<$selection_end; i++)\n\
		    	    fillText = fillText fillChar\n\
		    } else {\n\
		    	startIndent = 0\n\
		    	for (i=$selection_start-1; i>=0 && get_character(i)!=\"\\n\"; i--)\n\
		    	    startIndent++\n\
		    	for (i=0; i<$wrap_margin-startIndent; i++)\n\
		    	    fillText = fillText fillChar\n\
		    	fillText = fillText \"\\n\"\n\
			for (i=0; i<$wrap_margin; i++)\n\
			    line = line fillChar\n\
			for (i=0; i<nLines-1; i++)\n\
			    fillText = fillText line \"\\n\"\n\
			for (i=$selection_end-1; i>=$selection_start && get_character(i)!=\"\\n\"; \\\n\
			    	i--)\n\
			    fillText = fillText fillChar\n\
		    }\n\
		}\n\
		\n\
		# Replace the selection with the fill text\n\
		replace_selection(fillText)\n\
	}", "Fill Sel. w/Char:", "\n\t}",
      "Comments>/* Uncomment */@C@C++@Java@CSS@JavaScript@Lex:::R: {\n\
		sel = get_selection()\n\
		selStart = $selection_start\n\
		selEnd = $selection_end\n\
		commentStart = search_string(sel, \"/*\", 0)\n\
		if (substring(sel, commentStart + 2, commentStart + 3) == \" \")\n\
		    keepStart = commentStart + 3\n\
		else\n\
		    keepStart = commentStart + 2\n\
		keepEnd = search_string(sel, \"*/\", length(sel), \"backward\")\n\
		commentEnd = keepEnd + 2\n\
		if (substring(sel, keepEnd - 1, keepEnd) == \" \")\n\
		    keepEnd = keepEnd - 1\n\
		replace_range(selStart + commentStart, selStart + commentEnd, \\\n\
			substring(sel, keepStart, keepEnd))\n\
		select(selStart, selEnd - (keepStart-commentStart) - \\\n\
			(commentEnd - keepEnd))\n\
	}", "Comments>/* Uncomment */", "\n\t}",
      "Comments>Bar Uncomment@C:::R: {\n\
		selStart = $selection_start\n\
		selEnd = $selection_end\n\
		newText = get_range(selStart+3, selEnd-4)\n\
		newText = replace_in_string(newText, \"^ \\\\* \", \"\", \"regex\")\n\
		replace_range(selStart, selEnd, newText)\n\
		select(selStart, selStart + length(newText))\n\
	}", "Comments>Bar Uncomment@C:", "\n\t}",
      "Make C Prototypes@C@C++:::: {\n\
		if ($selection_start == -1) {\n\
		    start = 0\n\
		    end = $text_length\n\
		} else {\n\
		    start = $selection_start\n\
		    end = $selection_end\n\
		}\n\
		string = get_range(start, end)\n\
		nDefs = 0", "Make C Prototypes@C@C++:", "\t\tnDefs = 0",
      NULL
   };
   int i;
   for (i = 0; pats[i]; i += 3)
      replaceMacroIfUnchanged(pats[i], pats[i + 1], pats[i + 2]);
   return;
}

// --------------------------------------------------------------------------
// Decref the default calltips file(s) for this window
// --------------------------------------------------------------------------
void UnloadLanguageModeTipsFile(WindowInfo* window)
{
   int mode = window->languageMode;
   if (mode != PLAIN_LANGUAGE_MODE && LanguageModes[mode]->defTipsFile)
   {
      DeleteTagsFile(LanguageModes[mode]->defTipsFile, TIP, false);
   }
}

/******************************************************************************
 * The Color selection dialog
 ******************************************************************************/

/*
There are 8 colors:   And 8 indices:
textFg                TEXT_FG_COLOR
textBg                TEXT_BG_COLOR
selectFg              SELECT_FG_COLOR
selectBg              SELECT_BG_COLOR
hiliteFg              HILITE_FG_COLOR
hiliteBg              HILITE_BG_COLOR
lineNoFg              LINENO_FG_COLOR
cursorFg              CURSOR_FG_COLOR
*/

#define MARGIN_SPACING 10

/*
 * Callbacks for field modifications
 */
static void textFgModifiedCB(Fl_Widget* w, void* data)
{
   TRACE();
   colorDialog* cd = (colorDialog*)data;
   showColorStatus(cd, cd->textFgW, cd->textFgErrW);
}

static void textBgModifiedCB(Fl_Widget* w, void* data)
{
   TRACE();
   colorDialog* cd = (colorDialog*)data;
   showColorStatus(cd, cd->textBgW, cd->textBgErrW);
}

static void selectFgModifiedCB(Fl_Widget* w, void* data)
{
   TRACE();
   colorDialog* cd = (colorDialog*)data;
   showColorStatus(cd, cd->selectFgW, cd->selectFgErrW);
}

static void selectBgModifiedCB(Fl_Widget* w, void* data)
{
   TRACE();
   colorDialog* cd = (colorDialog*)data;
   showColorStatus(cd, cd->selectBgW, cd->selectBgErrW);
}

static void hiliteFgModifiedCB(Fl_Widget* w, void* data)
{
   TRACE();
   colorDialog* cd = (colorDialog*)data;
   showColorStatus(cd, cd->hiliteFgW, cd->hiliteFgErrW);
}

static void hiliteBgModifiedCB(Fl_Widget* w, void* data)
{
   TRACE();
   colorDialog* cd = (colorDialog*)data;
   showColorStatus(cd, cd->hiliteBgW, cd->hiliteBgErrW);
}

static void lineNoFgModifiedCB(Fl_Widget* w, void* data)
{
   TRACE();
   colorDialog* cd = (colorDialog*)data;
   showColorStatus(cd, cd->lineNoFgW, cd->lineNoFgErrW);
}

static void cursorFgModifiedCB(Fl_Widget* w, void* data)
{
   TRACE();
   colorDialog* cd = (colorDialog*)data;
   showColorStatus(cd, cd->cursorFgW, cd->cursorFgErrW);
}

// --------------------------------------------------------------------------
// Helper functions for validating colors
// --------------------------------------------------------------------------
static int verifyAllColors(colorDialog* cd)
{
   // Maybe just check for empty strings in error widgets instead?
   return (checkColorStatus(cd, cd->textFgW) &&
           checkColorStatus(cd, cd->textBgW) &&
           checkColorStatus(cd, cd->selectFgW) &&
           checkColorStatus(cd, cd->selectBgW) &&
           checkColorStatus(cd, cd->hiliteFgW) &&
           checkColorStatus(cd, cd->hiliteBgW) &&
           checkColorStatus(cd, cd->lineNoFgW) &&
           checkColorStatus(cd, cd->cursorFgW));
}

// --------------------------------------------------------------------------
// Returns true if the color is valid, false if it's not
// #rrggbb (0-f) or Color name from rgb_txt
// --------------------------------------------------------------------------
static bool checkColorStatus(colorDialog* cd, Fl_Input* colorFieldW)
{
   return CheckColor(colorFieldW->value());
}

// --------------------------------------------------------------------------
// Show or hide errorLabelW depending on whether or not colorFieldW
//  contains a valid color name.
// --------------------------------------------------------------------------
static void showColorStatus(colorDialog* cd, Fl_Input* colorFieldW, Fl_Widget* errorLabelW)
{
   // Should set the OK/Apply button sensitivity here, instead
   // of leaving is sensitive and then complaining if an error.
   if (checkColorStatus(cd, colorFieldW))
   {
      errorLabelW->hide();
      Fl_Color color = GetColor(colorFieldW->value());
      colorFieldW->color(color);
      colorFieldW->textcolor(fl_contrast(color, color));
   }
   else
   {
      errorLabelW->show();
      colorFieldW->color(FL_WHITE);
      colorFieldW->textcolor(FL_BLACK);
   }
}

// Update the colors in the window or in the preferences
static void updateColors(colorDialog* cd)
{
   char* textFg = NeTextGetString(cd->textFgW),
      *textBg = NeTextGetString(cd->textBgW),
      *selectFg = NeTextGetString(cd->selectFgW),
      *selectBg = NeTextGetString(cd->selectBgW),
      *hiliteFg = NeTextGetString(cd->hiliteFgW),
      *hiliteBg = NeTextGetString(cd->hiliteBgW),
      *lineNoFg = NeTextGetString(cd->lineNoFgW),
      *cursorFg = NeTextGetString(cd->cursorFgW);

   for ( WindowInfo* window = WindowList; window != NULL; window = window->next)
   {
      SetColors(window, textFg, textBg, selectFg, selectBg, hiliteFg, hiliteBg, lineNoFg, cursorFg);
   }

   SetPrefColorName(TEXT_FG_COLOR  , textFg);
   SetPrefColorName(TEXT_BG_COLOR  , textBg);
   SetPrefColorName(SELECT_FG_COLOR, selectFg);
   SetPrefColorName(SELECT_BG_COLOR, selectBg);
   SetPrefColorName(HILITE_FG_COLOR, hiliteFg);
   SetPrefColorName(HILITE_BG_COLOR, hiliteBg);
   SetPrefColorName(LINENO_FG_COLOR, lineNoFg);
   SetPrefColorName(CURSOR_FG_COLOR, cursorFg);

   free__(textFg);
   free__(textBg);
   free__(selectFg);
   free__(selectBg);
   free__(hiliteFg);
   free__(hiliteBg);
   free__(lineNoFg);
   free__(cursorFg);
}


/*
 * Dialog button callbacks
 */

// --------------------------------------------------------------------------
static void colorDestroyCB(Fl_Widget* w, void* data)
{
   TRACE();
   // TODO:    colorDialog* cd = (colorDialog*)data;
   // TODO:
   // TODO:    cd->window->colorDialog = NULL;
   // TODO:    delete cd->window->colorDialog;
}

// --------------------------------------------------------------------------
static void colorOkCB(Fl_Widget* w, void* data)
{
   TRACE();
   colorDialog* cd = (colorDialog*)data;

   if (!verifyAllColors(cd))
   {
      DialogF(DF_ERR, w, 1, "Invalid Colors", "All colors must be valid to proceed.", "OK");
      return;
   }
   updateColors(cd);

   // pop down and destroy the dialog
   WidgetToMainWindow(w)->hide();
}

// --------------------------------------------------------------------------
static void colorApplyCB(Fl_Widget* w, void* data)
{
   TRACE();
   colorDialog* cd = (colorDialog*)data;

   if (!verifyAllColors(cd))
   {
      DialogF(DF_ERR, w, 1, "Invalid Colors", "All colors must be valid to be applied.", "OK");
      return;
   }
   updateColors(cd);
}

// --------------------------------------------------------------------------
static void colorCloseCB(Fl_Widget* w, void* data)
{
   TRACE();
   colorDialog* cd = (colorDialog*)data;

   // pop down and destroy the dialog
   WidgetToMainWindow(w)->hide();
}

// --------------------------------------------------------------------------
// Add a label, error label, and text entry label with a validation callback
// --------------------------------------------------------------------------
static void addColorGroup(const char* label, Fl_Input** fieldW, Fl_Box** errW,
                          const Ne_Dimension& dimension, Fl_Callback* modCallback,
                          colorDialog* cd)
{
   *fieldW = new Fl_Input(dimension.x, dimension.y, dimension.w, dimension.h, label);
   (*fieldW)->align(FL_ALIGN_TOP_LEFT);
   (*fieldW)->callback(modCallback, cd);
   (*fieldW)->when(FL_WHEN_CHANGED);

   *errW = new Fl_Box(dimension.x + dimension.w - 60, dimension.y - 25, 60, 25, "(invalid!)");
   (*errW)->hide();
}


// --------------------------------------------------------------------------
// Code for the dialog itself
// --------------------------------------------------------------------------
void ChooseColors(WindowInfo* window)
{
   // if the dialog is already displayed, just pop it to the top and return
   if (window->colorDialog != NULL)
   {
      ((colorDialog*)window->colorDialog)->mainWindow->show();
      return;
   }

   // Create a structure for keeping track of dialog state
   colorDialog* cd = new colorDialog();
   window->colorDialog = (void*)cd;

   // Create a form widget in a dialog shell
   Fl_Window* form = new Fl_Window(30, 50, 600, 380, "Colors");
   cd->mainWindow = form;
   cd->window = window;

   // Information label
   new Fl_Box(5, 5, 590, 50,
              "Colors can be entered as names (e.g. red, blue) or "
              "as RGB triples\nin the format #RRGGBB, where each digit "
              "is in the range 0-f.");

   // The left column (foregrounds) of color entry groups
   addColorGroup("&Plain Text Foreground", &(cd->textFgW), &(cd->textFgErrW), Ne_Dimension(5, 80, 290, 25), textFgModifiedCB, cd);
   addColorGroup("&Selection Foreground", &(cd->selectFgW), &(cd->selectFgErrW), Ne_Dimension(5, 140, 290, 25), selectFgModifiedCB, cd);
   addColorGroup("&Matching (..) Foreground", &(cd->hiliteFgW), &(cd->hiliteFgErrW), Ne_Dimension(5, 200, 290, 25), hiliteFgModifiedCB, cd);
   addColorGroup("&Line Numbers", &(cd->lineNoFgW), &(cd->lineNoFgErrW), Ne_Dimension(5, 260, 290, 25), lineNoFgModifiedCB, cd);

   // The right column (backgrounds)
   addColorGroup("&Text Area Background", &(cd->textBgW), &(cd->textBgErrW), Ne_Dimension(300, 80, 290, 25), textBgModifiedCB, cd);
   addColorGroup("Selection &Background", &(cd->selectBgW), &(cd->selectBgErrW), Ne_Dimension(300, 140, 290, 25), selectBgModifiedCB, cd);
   addColorGroup("Matc&hing (..) Background", &(cd->hiliteBgW), &(cd->hiliteBgErrW), Ne_Dimension(300, 200, 290, 25), hiliteBgModifiedCB, cd);
   addColorGroup("&Cursor Color", &(cd->cursorFgW), &(cd->cursorFgErrW), Ne_Dimension(300, 260, 290, 25), cursorFgModifiedCB, cd);

   Fl_Box* noteLabel = new Fl_Box(5, 290, 590, 30, "NOTE: Foreground colors only apply when syntax highlighting is DISABLED.");

   // The OK, Apply, and Cancel buttons
   Fl_Group* buttonLine = new Fl_Group(0, 330, 600, 50);
   buttonLine->box(FL_ENGRAVED_FRAME);

   Fl_Button* btnOk = new Fl_Button(100, 345, 100, 25, "Ok");
   btnOk->shortcut(FL_Enter);
   btnOk->callback(colorOkCB, cd);

   Fl_Button* btnApply = new Fl_Button(250, 345, 100, 25, "&Apply");
   btnApply->callback(colorApplyCB, cd);

   Fl_Button* btnClose = new Fl_Button(400, 345, 100, 25, "Close");
   btnClose->shortcut(FL_Escape);
   btnClose->callback(colorCloseCB, cd);

   buttonLine->end();

   form->resizable(noteLabel);

   // Set initial values
   NeTextSetString(cd->textFgW,   GetPrefColorName(TEXT_FG_COLOR));
   showColorStatus(cd, cd->textFgW, cd->textFgErrW);
   NeTextSetString(cd->textBgW,   GetPrefColorName(TEXT_BG_COLOR));
   showColorStatus(cd, cd->textBgW, cd->textBgErrW);
   NeTextSetString(cd->selectFgW, GetPrefColorName(SELECT_FG_COLOR));
   showColorStatus(cd, cd->selectFgW, cd->selectFgErrW);
   NeTextSetString(cd->selectBgW, GetPrefColorName(SELECT_BG_COLOR));
   showColorStatus(cd, cd->selectBgW, cd->selectBgErrW);
   NeTextSetString(cd->hiliteFgW, GetPrefColorName(HILITE_FG_COLOR));
   showColorStatus(cd, cd->hiliteFgW, cd->hiliteFgErrW);
   NeTextSetString(cd->hiliteBgW, GetPrefColorName(HILITE_BG_COLOR));
   showColorStatus(cd, cd->hiliteBgW, cd->hiliteFgErrW);
   NeTextSetString(cd->lineNoFgW, GetPrefColorName(LINENO_FG_COLOR));
   showColorStatus(cd, cd->lineNoFgW, cd->lineNoFgErrW);
   NeTextSetString(cd->cursorFgW, GetPrefColorName(CURSOR_FG_COLOR));
   showColorStatus(cd, cd->cursorFgW, cd->cursorFgErrW);

   // put up dialog
   ManageDialogCenteredOnPointer(form);

   form->show();
}

/*
**  This function passes up a pointer to the static name of the default
**  shell, currently defined as the user's login shell.
**  In case of errors, the fallback of "sh" will be returned.
*/
static const char* getDefaultShell()
{
   static char shellBuffer[MAXPATHLEN + 1] = "sh";
#ifdef WIN32
   strcpy(shellBuffer, "");
#else
   struct passwd* passwdEntry = NULL;

   passwdEntry = getpwuid(getuid());   /*  getuid() never fails.  */

   if (NULL == passwdEntry)
   {
      /*  Something bad happened! Do something, quick!  */
      perror("nedit: Failed to get passwd entry (falling back to 'sh')");
      return "sh";
   }

   /*  *passwdEntry may be overwritten  */
   /*  TODO: To make this and other function calling getpwuid() more robust,
       passwdEntry should be kept in a central position (Core->sysinfo?).
       That way, local code would always get a current copy of passwdEntry,
       but could still be kept lean.  The obvious alternative of a central
       facility within NEdit to access passwdEntry would increase coupling
       and would have to cover a lot of assumptions.  */
   strncpy(shellBuffer, passwdEntry->pw_shell, MAXPATHLEN);
   shellBuffer[MAXPATHLEN] = '\0';
#endif
   return shellBuffer;
}
