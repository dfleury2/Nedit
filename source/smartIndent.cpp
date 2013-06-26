/*******************************************************************************
*                                                                              *
* smartIndent.c -- Maintain, and allow user to edit, macros for smart indent   *
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
*                                                                              *
* July, 1997                                                                   *
*                                                                              *
* Written by Mark Edel                                                         *
*                                                                              *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "smartIndent.h"
#include "Ne_Text_Buffer.h"
#include "nedit.h"
#include "Ne_Text_Editor.h"
#include "preferences.h"
#include "interpret.h"
#include "macro.h"
#include "window.h"
#include "parse.h"
#include "shift.h"
#include "help.h"
#include "../util/DialogF.h"
#include "../util/misc.h"

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Box.H>

#include <stdio.h>
#include <string.h>
#include <limits.h>

#ifndef WIN32
#include <sys/param.h>
#endif

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

static char MacroEndBoundary[] = "--End-of-Macro--";

struct smartIndentRec
{
  char* lmName;
  char* initMacro;
  char* newlineMacro;
  char* modMacro;
};

struct windowSmartIndentData
{
  Program* newlineMacro;
  int inNewLineMacro;
  Program* modMacro;
  int inModMacro;
};

// Smart indent macros dialog information
static struct
{
  Fl_Window* shell;
  Fl_Choice* lmOptMenu;
  Fl_Text_Editor* initMacro;
  Fl_Text_Buffer* initMacroBuffer;
  Fl_Text_Editor* newlineMacro;
  Fl_Text_Buffer* newlineMacroBuffer;
  Fl_Text_Editor* modMacro;
  Fl_Text_Buffer* modMacroBuffer;
  char* langModeName;
} SmartIndentDialog = {NULL,NULL,NULL,NULL,NULL,NULL};

// Common smart indent macros dialog information
static struct
{
  Fl_Window* shell;
  Fl_Text_Display* text;
  Fl_Text_Buffer* buffer;
} CommonDialog = {NULL,NULL,NULL};

// --------------------------------------------------------------------------
static int NSmartIndentSpecs = 0;
static smartIndentRec* SmartIndentSpecs[MAX_LANGUAGE_MODES];
static char* CommonMacros = NULL;

static void executeNewlineMacro(WindowInfo* window,smartIndentCBStruct* cbInfo);
static void executeModMacro(WindowInfo* window,smartIndentCBStruct* cbInfo);
static void insertShiftedMacro(Ne_Text_Buffer* buf, char* macro);
static int isDefaultIndentSpec(smartIndentRec* indentSpec);
static smartIndentRec* findIndentSpec(const char* modeName);
static char* ensureNewline(char* string);
static int loadDefaultIndentSpec(char* lmName);
static int siParseError(char* stringStart, char* stoppedAt, char* message);
static void destroyCB(Fl_Widget* w, void* data);
static void langModeCB(Fl_Widget* w, void* data);
static void commonDialogCB(Fl_Widget* w, void* data);
static void lmDialogCB(Fl_Widget* w, void* data);
static void okCB(Fl_Widget* w, void* data);
static void applyCB(Fl_Widget* w, void* data);
static void checkCB(Fl_Widget* w, void* data);
static void restoreCB(Fl_Widget* w, void* data);
static void deleteCB(Fl_Widget* w, void* data);
static void closeCB(Fl_Widget* w, void* data);
static void helpCB(Fl_Widget* w, void* data);
static int checkSmartIndentDialogData();
static smartIndentRec* getSmartIndentDialogData();
static void setSmartIndentDialogData(smartIndentRec* is);
static void comDestroyCB(Fl_Widget* w, void* data);
static void comOKCB(Fl_Widget* w, void* data);
static void comApplyCB(Fl_Widget* w, void* data);
static void comCheckCB(Fl_Widget* w, void* data);
static void comRestoreCB(Fl_Widget* w, void* data);
static void comCloseCB(Fl_Widget* w, void* data);
static int updateSmartIndentCommonData();
static int checkSmartIndentCommonDialogData();
static int updateSmartIndentData();
static char* readSIMacro(char** inPtr);
static smartIndentRec* copyIndentSpec(smartIndentRec* is);
static void freeIndentSpec(smartIndentRec* is);
static int indentSpecsDiffer(smartIndentRec* is1, smartIndentRec* is2);

#define N_DEFAULT_INDENT_SPECS 4
static smartIndentRec DefaultIndentSpecs[N_DEFAULT_INDENT_SPECS] =
{
   {
      "C",
      "# C Macros and tuning parameters are shared with C++, and are declared\n\
# in the common section.  Press Common / Shared Initialization above.\n",
      "return cFindSmartIndentDist($1)\n",
      "if ($2 == \"}\" || $2 == \"{\" || $2 == \"#\")\n\
    cBraceOrPound($1, $2)\n"
   },
{
   "C++",
   "# C++ Macros and tuning parameters are shared with C, and are declared\n\
# in the common section.  Press Common / Shared Initialization above.\n",
   "return cFindSmartIndentDist($1)\n",
      "if ($2 == \"}\" || $2 == \"{\" || $2 == \"#\")\n\
    cBraceOrPound($1, $2)\n"
   },
{
   "Python",
   "# Number of characters in a normal indent level.  May be a number, or the\n\
# string \"default\", meaning, guess the value from the current tab settings.\n\
$pyIndentDist = \"default\"\n",
   "if (get_range($1-1, $1) != \":\")\n\
    return -1\n\
return measureIndent($1) + defaultIndent($pyIndentDist)\n", NULL
   },
{
   "Matlab",
   "# Number of spaces to indent \"case\" statements\n\
$caseDepth = 2\n\
define matlabNewlineMacro\n\
{\n\
   if (!$em_tab_dist)\n\
        tabsize = $tab_dist\n\
   else\n\
        tabsize = $em_tab_dist\n\
   startLine = startOfLine($1)\n\
   indentLevel = measureIndent($1)\n\
\n\
   # If this line is continued on next, return default:\n\
   lastLine = get_range(startLine, $1)\n\
   if (search_string(lastLine, \"...\", 0) != -1) {\n\
      if ($n_args == 2)\n\
         return matlabNewlineMacro(startLine - 1, 1)\n\
      else {\n\
         return -1\n\
      }\n\
   }\n\
\n\
   # Correct the indentLevel if this was a continued line.\n\
   while (startLine > 1)\n\
   {\n\
      endLine = startLine - 1\n\
      startLine = startOfLine(endLine)\n\
      lastLine = get_range(startLine, endLine)\n\
      # No \"...\" means we've found the root\n\
      if (search_string(lastLine, \"...\", 0) == -1) {\n\
         startLine = endLine + 1\n\
         break\n\
      }\n\
   }\n\
   indentLevel = measureIndent(startLine)\n\
\n\
   # Get the first word of the indentLevel line\n\
   FWend = search(\">|\\n\", startLine + indentLevel, \"regex\")\n\
   # This search fails on EOF\n\
   if (FWend == -1)\n\
      FWend = $1\n\
\n\
   firstWord = get_range(startLine + indentLevel, FWend)\n\
\n\
   # How shall we change the indent level based on the first word?\n\
   if (search_string(firstWord, \\\n\
         \"<for>|<function>|<if>|<switch>|<try>|<while>\", 0, \"regex\") == 0) {\n\
      return indentLevel + tabsize\n\
   }\n\
   else if ((firstWord == \"end\") || (search_string(firstWord, \\\n\
            \"<case>|<catch>|<else>|<elseif>|<otherwise>\", 0, \"regex\") == 0)) {\n\
      # Get the last indent level \n\
      if (startLine > 0) # avoid infinite loop\n\
	 last_indent = matlabNewlineMacro(startLine - 1, 1)\n\
      else\n\
         last_indent = indentLevel\n\
\n\
      # Re-indent this line\n\
      if ($n_args == 1) {\n\
         if (firstWord == \"case\" || firstWord == \"otherwise\")\n\
            replace_range(startLine, startLine + indentLevel, \\\n\
                          makeIndentString(last_indent - tabsize + $caseDepth))\n\
         else\n\
            replace_range(startLine, startLine + indentLevel, \\\n\
                          makeIndentString(last_indent - tabsize))\n\
      }\n\
\n\
      if (firstWord == \"end\") {\n\
         return max(last_indent - tabsize, 0)\n\
      }\n\
      else {\n\
         return last_indent\n\
      }\n\
   } \n\
   else {\n\
      return indentLevel\n\
   }\n\
}\n\
", "return matlabNewlineMacro($1)\n", NULL
}
};

static char DefaultCommonMacros[] = "#\n\
# C/C++ Style/tuning parameters\n\
#\n\
\n\
# Number of characters in a normal indent level.  May be a number, or the\n\
# string \"default\", meaning, guess the value from the current tab settings.\n\
$cIndentDist = \"default\"\n\
\n\
# Number of characters in a line continuation.  May be a number or the\n\
# string \"default\", meaning, guess the value from the current tab settings.\n\
$cContinuationIndent = \"default\"\n\
\n\
# How far back from the current position to search for an anchoring position\n\
# on which to base indent.  When no reliable indicators of proper indent level\n\
# can be found within the requested distance, reverts to plain auto indent.\n\
$cMaxSearchBackLines = 10\n\
\n\
#\n\
# Find the start of the line containing position $1\n\
#\n\
define startOfLine {\n\
\n\
    for (i=$1-1; ; i--) {\n\
	if (i <= 0)\n\
	    return 0\n\
	if (get_character(i) == \"\\n\")\n\
	    return i + 1\n\
    }\n\
}\n\
\n\
#\n\
# Find the indent level of the line containing character position $1\n\
#\n\
define measureIndent {\n\
    \n\
    # measure the indentation to the first non-white character on the line\n\
    indent = 0\n\
    for (i=startOfLine($1); i < $text_length; i++) {\n\
	c = get_character(i)\n\
	if (c != \" \" && c != \"\\t\")\n\
	    break\n\
	if (c == \"\\t\")\n\
	    indent += $tab_dist - (indent % $tab_dist)\n\
	else\n\
	    indent++\n\
    }\n\
    return indent\n\
}\n\
\n\
#\n\
# Make a string to produce an indent of $1 characters\n\
#\n\
define makeIndentString {\n\
\n\
    if ($use_tabs) {\n\
	nTabs = $1 / $tab_dist\n\
	nSpaces = $1 % $tab_dist\n\
    } else {\n\
	nTabs = 0\n\
	nSpaces = $1\n\
    }\n\
    indentString = \"\"\n\
    for (i=0; i<nTabs; i++)\n\
	indentString = indentString \"\\t\"\n\
    for (i=0; i<nSpaces; i++)\n\
	indentString = indentString \" \"\n\
    return indentString\n\
}\n\
\n\
#\n\
# If $1 is a number, just pass it on.  If it is the string \"default\",\n\
# figure out a reasonable indent distance for a structured languages\n\
# like C, based on how tabs are set.\n\
#\n\
define defaultIndent {\n\
\n\
    if ($1 != \"default\")\n\
    	return $1\n\
    if ($em_tab_dist)\n\
    	return $em_tab_dist\n\
    if ($tab_dist <= 8)\n\
    	return $tab_dist\n\
    return 4\n\
}\n\
   \n\
#\n\
# If $1 is a number, just pass it on.  If it is the string \"default\",\n\
# figure out a reasonable amount of indentation for continued lines\n\
# based on how tabs are set.\n\
#\n\
define defaultContIndent {\n\
\n\
    if ($1 != \"default\")\n\
    	return $1\n\
    if ($em_tab_dist)\n\
    	return $em_tab_dist * 2\n\
    if ($tab_dist <= 8)\n\
    	return $tab_dist * 2\n\
    return 8\n\
}\n\
\n\
#\n\
# Find the end of the conditional part of if/while/for, by looking for balanced\n\
# parenthesis between $1 and $2.  returns -1 if parens don't balance before\n\
# $2, or if no parens are found\n\
#\n\
define findBalancingParen {\n\
\n\
    openParens = 0\n\
    parensFound = 0\n\
    for (i=$1; i<$2; i++) {\n\
	c = get_character(i)\n\
	if (c == \"(\") {\n\
	    openParens++\n\
	    parensFound = 1\n\
	} else if (c == \")\")\n\
	    openParens--\n\
	else if (!parensFound && c != \" \" && c != \"\\t\")\n\
	    return -1\n\
	if (parensFound && openParens <=0)\n\
	    return i+1\n\
    }\n\
    return -1\n\
}\n\
\n\
#\n\
# Skip over blank space and comments and preprocessor directives from position\n\
# $1 to a maximum of $2.\n\
# if $3 is non-zero, newlines are considered blank space as well.  Return -1\n\
# if the maximum position ($2) is hit mid-comment or mid-directive\n\
#\n\
define cSkipBlankSpace {\n\
    \n\
    for (i=$1; i<$2; i++) {\n\
	c = get_character(i)\n\
	if (c == \"/\") {\n\
	    if (i+1 >= $2)\n\
		return i\n\
	    if (get_character(i+1) == \"*\") {\n\
		for (i=i+1; ; i++) {\n\
		    if (i+1 >= $2)\n\
			return -1\n\
		    if (get_character(i) == \"*\" && get_character(i+1) == \"/\") {\n\
			i++\n\
			break\n\
		    }\n\
		}\n\
	    } else if (get_character(i+1) == \"/\") {\n\
		for (i=i+1; i<$2; i++) {\n\
		    if (get_character(i) == \"\\n\") {\n\
			if (!$3)\n\
			    return i\n\
			break\n\
		    }\n\
		}\n\
	    }\n\
	} else if (c == \"#\" && $3) {\n\
	    for (i=i+1; ; i++) {\n\
		if (i >= $2) {\n\
		    if (get_character(i-1) == \"\\\\\")\n\
			return -1\n\
		    else\n\
			break\n\
		}\n\
		if (get_character(i) == \"\\n\" && get_character(i-1) != \"\\\\\")\n\
		    break\n\
	    }\n\
	} else if (!(c == \" \" || c == \"\\t\" || ($3 && c==\"\\n\")))\n\
	    return i\n\
    }\n\
    return $2\n\
}\n\
\n\
#\n\
# Search backward for an anchor point: a line ending brace, or semicolon\n\
# or case statement, followed (ignoring blank lines and comments) by what we\n\
# assume is a properly indented line, a brace on a line by itself, or a case\n\
# statement.  Returns the position of the first non-white, non comment\n\
# character on the line.  returns -1 if an anchor position can't be found\n\
# before $cMaxSearchBackLines.\n\
#\n\
define cFindIndentAnchorPoint {\n\
\n\
    nLines = 0\n\
    anchorPos = $1\n\
    for (i=$1-1; i>0; i--) {\n\
	c = get_character(i)\n\
	if (c == \";\" || c == \"{\" || c == \"}\" || c == \":\") {\n\
\n\
	    # Verify that it's line ending\n\
	    lineEnd = cSkipBlankSpace(i+1, $1, 0)\n\
	    if (lineEnd == -1 || \\\n\
	    	    (lineEnd != $text_length && get_character(lineEnd) != \"\\n\"))\n\
   		continue\n\
\n\
	    # if it's a colon, it's only meaningful if \"case\" begins the line\n\
	    if (c == \":\") {\n\
	    	lineStart = startOfLine(i)\n\
		caseStart = cSkipBlankSpace(lineStart, lineEnd, 0)\n\
		if (get_range(caseStart, caseStart+4) != \"case\")\n\
		    continue\n\
		delim = get_character(caseStart+4)\n\
		if (delim!=\" \" && delim!=\"\\t\" && delim!=\"(\" && delim!=\":\")\n\
		    continue\n\
		isCase = 1\n\
	    } else\n\
	    	isCase = 0\n\
\n\
	    # Move forward past blank lines and comment lines to find\n\
	    #    non-blank, non-comment line-start\n\
	    anchorPos = cSkipBlankSpace(lineEnd, $1, 1)\n\
\n\
	    # Accept if it's before the requested position, otherwise\n\
	    #    continue further back in the file and try again\n\
	    if (anchorPos != -1 && anchorPos < $1)\n\
		break\n\
\n\
	    # A case statement by itself is an acceptable anchor\n\
	    if (isCase)\n\
	    	return caseStart\n\
\n\
	    # A brace on a line by itself is an acceptable anchor, even\n\
	    #    if it doesn't follow a semicolon or another brace\n\
	    if (c == \"{\" || c == \"}\") {\n\
		for (j = i-1; ; j--) {\n\
		    if (j == 0)\n\
			return i\n\
		    ch = get_character(j)\n\
		    if (ch == \"\\n\")\n\
		       return i\n\
		    if (ch != \"\\t\" && ch != \" \")\n\
		       break\n\
		}\n\
	    }\n\
\n\
	} else if (c == \"\\n\")\n\
	    if (++nLines > $cMaxSearchBackLines)\n\
		return -1\n\
    }\n\
    if (i <= 0)\n\
	return -1\n\
    return anchorPos\n\
}\n\
\n\
#\n\
# adjust the indent on a line about to recive either a right or left brace\n\
# or pound (#) character ($2) following position $1\n\
#\n\
define cBraceOrPound {\n\
\n\
    # Find start of the line, and make sure there's nothing but white-space\n\
    #   before the character.  If there's anything before it, do nothing\n\
    for (i=$1-1; ; i--) {\n\
	if (i < 0) {\n\
	    lineStart = 0\n\
	    break\n\
	}\n\
	c = get_character(i)\n\
	if (c == \"\\n\") {\n\
	    lineStart = i + 1\n\
	    break\n\
	}\n\
	if (c != \" \" && c != \"\\t\")\n\
	    return\n\
    }\n\
\n\
    # If the character was a pound, drag it all the way to the left margin\n\
    if ($2 == \"#\") {\n\
	replace_range(lineStart, $1, \"\")\n\
	return\n\
    }\n\
\n\
    # Find the position on which to base the indent\n\
    indent = cFindSmartIndentDist($1 - 1, \"noContinue\")\n\
    if (indent == -1)\n\
	return\n\
    \n\
    # Adjust the indent if it's a right brace (left needs no adjustment)\n\
    if ($2 == \"}\") {\n\
	indent -= defaultIndent($cIndentDist)\n\
        if (indent < 0)\n\
	    indent = 0\n\
    }\n\
\n\
    # Replace the current indent with the new indent string\n\
    insertStr = makeIndentString(indent)\n\
    replace_range(lineStart, $1, insertStr)\n\
}\n\
\n\
#\n\
# Find Smart Indent Distance for a newline character inserted at $1,\n\
# or return -1 to give up.  Adding the optional argument \"noContinue\"\n\
# will stop the routine from inserting line continuation indents\n\
#\n\
define cFindSmartIndentDist {\n\
\n\
    # Find a known good indent to base the new indent upon\n\
    anchorPos = cFindIndentAnchorPoint($1)\n\
    if (anchorPos == -1)\n\
	return -1\n\
\n\
    # Find the indentation of that line\n\
    anchorIndent = measureIndent(anchorPos)\n\
\n\
    # Look for special keywords which affect indent (for, if, else while, do)\n\
    #    and modify the continuation indent distance to the normal indent\n\
    #    distance when a completed statement of this type occupies the line.\n\
    if ($n_args >= 2 && $2 == \"noContinue\") {\n\
	continueIndent = 0\n\
	$allowSemi = 0\n\
    } else\n\
	continueIndent = cCalcContinueIndent(anchorPos, $1)\n\
\n\
    # Move forward from anchor point, ignoring comments and blank lines,\n\
    #   remembering the last non-white, non-comment character.  If $1 is\n\
    #   in the middle of a comment, give up\n\
    lastChar = get_character(anchorPos)\n\
    if (anchorPos < $1) {\n\
	for (i=anchorPos;;) {\n\
   	    i = cSkipBlankSpace(i, $1, 1)\n\
	    if (i == -1)\n\
		return -1\n\
 	    if (i >= $1)\n\
 		break\n\
 	    lastChar = get_character(i++)\n\
	}\n\
    }\n\
\n\
    # Return the new indent based on the type of the last character.\n\
    #   In a for stmt, however, last character may be a semicolon and not\n\
    #   signal the end of the statement\n\
    if (lastChar == \"{\")\n\
	return anchorIndent + defaultIndent($cIndentDist)\n\
    else if (lastChar == \"}\")\n\
	return anchorIndent\n\
    else if (lastChar == \";\") {\n\
	if ($allowSemi)\n\
	    return anchorIndent + continueIndent\n\
	else\n\
	    return anchorIndent\n\
    } else if (lastChar == \":\" && get_range(anchorPos, anchorPos+4) == \"case\")\n\
    	return anchorIndent + defaultIndent($cIndentDist)\n\
    return anchorIndent + continueIndent\n\
}\n\
\n\
#\n\
# Calculate the continuation indent distance for statements not ending in\n\
# semicolons or braces.  This is not necessarily $continueIndent.  It may\n\
# be adjusted if the statement contains if, while, for, or else.\n\
#\n\
# As a side effect, also return $allowSemi to help distinguish statements\n\
# which might contain an embedded semicolon, which should not be interpreted\n\
# as an end of statement character.\n\
#\n\
define cCalcContinueIndent {\n\
\n\
    anchorPos = $1\n\
    maxPos = $2\n\
\n\
    # Figure out if the anchor is on a keyword which changes indent.  A special\n\
    #   case is made for elses nested in after braces\n\
    anchorIsFor = 0\n\
    $allowSemi = 0\n\
    if (get_character(anchorPos) == \"}\") {\n\
	for (i=anchorPos+1; i<maxPos; i++) {\n\
	    c = get_character(i)\n\
	    if (c != \" \" && c != \"\\t\")\n\
		break\n\
	}\n\
	if (get_range(i, i+4) == \"else\") {\n\
	    keywordEnd = i + 4\n\
	    needsBalancedParens = 0\n\
	} else\n\
	    return defaultContIndent($cContinuationIndent)\n\
    } else if (get_range(anchorPos, anchorPos + 4) == \"else\") {\n\
	keywordEnd = anchorPos + 4\n\
	needsBalancedParens = 0\n\
    } else if (get_range(anchorPos, anchorPos + 2) == \"do\") {\n\
	keywordEnd = anchorPos + 2\n\
	needsBalancedParens = 0\n\
    } else if (get_range(anchorPos, anchorPos + 3) == \"for\") {\n\
	keywordEnd = anchorPos + 3\n\
	anchorIsFor = 1\n\
	needsBalancedParens = 1\n\
    } else if (get_range(anchorPos, anchorPos + 2) == \"if\") {\n\
	keywordEnd = anchorPos + 2\n\
	needsBalancedParens = 1\n\
    } else if (get_range(anchorPos, anchorPos + 5) == \"while\") {\n\
	keywordEnd = anchorPos + 5\n\
	needsBalancedParens = 1\n\
    } else\n\
	return defaultContIndent($cContinuationIndent)\n\
\n\
    # If the keyword must be followed balanced parenthesis, find the end of\n\
    # the statement by following balanced parens.  If the parens aren't\n\
    # balanced by maxPos, continue the condition.  In the special case of\n\
    # the for keyword, a semicolon can end the line and the caller should be\n\
    # signaled to allow that\n\
    if (needsBalancedParens) {\n\
	stmtEnd = findBalancingParen(keywordEnd, maxPos)\n\
	if (stmtEnd == -1) {\n\
	    $allowSemi = anchorIsFor\n\
	    return defaultContIndent($cContinuationIndent)\n\
	}\n\
    } else\n\
	stmtEnd = keywordEnd\n\
\n\
    # check if the statement ends the line\n\
    lineEnd = cSkipBlankSpace(stmtEnd, maxPos, 0)\n\
    if (lineEnd == -1)		    	    # ends in comment or preproc\n\
	return -1\n\
    if (lineEnd == maxPos)  	    	    # maxPos happens at stmt end\n\
	return defaultIndent($cIndentDist)\n\
    c = get_character(lineEnd)\n\
    if (c != \"\\n\")   		    	    # something past last paren on line,\n\
	return defaultIndent($cIndentDist)  #   probably quoted or extra braces\n\
\n\
    # stmt contintinues beyond matching paren && newline, we're in\n\
    #   the conditional part, calculate the continue indent distance\n\
    #   recursively, based on the anchor point of the new line\n\
    newAnchor = cSkipBlankSpace(lineEnd+1, maxPos, 1)\n\
    if (newAnchor == -1)\n\
	return -1\n\
    if (newAnchor == maxPos)\n\
	return defaultIndent($cIndentDist)\n\
    return cCalcContinueIndent(newAnchor, maxPos) + defaultIndent($cIndentDist)\n\
}\n\
";

// --------------------------------------------------------------------------
// Turn on smart-indent (well almost).  Unfortunately, this doesn't do
// everything.  It requires that the smart indent callback (SmartIndentCB)
// is already attached to all of the text widgets in the window, and that the
// smartIndent resource must be turned on in the widget.  These are done
// separately, because they are required per-text widget, and therefore must
// be repeated whenever a new text widget is created within this window
// (a split-window command).
// --------------------------------------------------------------------------
void BeginSmartIndent(WindowInfo* window, int warn)
{
   windowSmartIndentData* winData;
   smartIndentRec* indentMacros;
   char *stoppedAt, *errMsg;
   static int initialized;

   // Find the window's language mode.  If none is set, warn the user
   char* modeName = LanguageModeName(window->languageMode);
   if (modeName == NULL)
   {
      if (warn)
      {
         DialogF(DF_WARN, window->mainWindow, 1, "Smart Indent",
            "No language-specific mode has been set for this file.\n\n"
            "To use smart indent in this window, please select a\n"
            "language from the Preferences -> Language Modes menu.",
            "OK");
      }
      return;
   }

   // Look up the appropriate smart-indent macros for the language
   indentMacros = findIndentSpec(modeName);
   if (indentMacros == NULL)
   {
      if (warn)
      {
         DialogF(DF_WARN, window->mainWindow, 1, "Smart Indent",
            "Smart indent is not available in languagemode\n%s.\n\n"
            "You can create new smart indent macros in the\n"
            "Preferences -> Default Settings -> Smart Indent\n"
            "dialog, or choose a different language mode from:\n"
            "Preferences -> Language Mode.", "OK", modeName);
      }
      return;
   }

   // Make sure that the initial macro file is loaded before we execute
   // any of the smart-indent macros. Smart-indent macros may reference
   // routines defined in that file.
   ReadMacroInitFile(window);

   // Compile and run the common and language-specific initialization macros
   // (Note that when these return, the immediate commands in the file have not
   // necessarily been executed yet.  They are only SCHEDULED for execution)
   if (!initialized)
   {
      if (!ReadMacroString(window, CommonMacros,
         "smart indent common initialization macros"))
         return;
      initialized = true;
   }
   if (indentMacros->initMacro != NULL)
   {
      if (!ReadMacroString(window, indentMacros->initMacro,
         "smart indent initialization macro"))
         return;
   }

   // Compile the newline and modify macros and attach them to the window
   winData = new windowSmartIndentData();
   winData->inNewLineMacro = 0;
   winData->inModMacro = 0;
   winData->newlineMacro = ParseMacro(indentMacros->newlineMacro, &errMsg, &stoppedAt);
   if (winData->newlineMacro == NULL)
   {
      delete winData;
      ParseError(window->mainWindow, indentMacros->newlineMacro, stoppedAt, "newline macro", errMsg);
      return;
   }
   if (indentMacros->modMacro == NULL)
      winData->modMacro = NULL;
   else
   {
      winData->modMacro = ParseMacro(indentMacros->modMacro, &errMsg, &stoppedAt);
      if (winData->modMacro == NULL)
      {
         FreeProgram(winData->newlineMacro);
         free__((char*)winData);
         ParseError(window->mainWindow, indentMacros->modMacro, stoppedAt, "smart indent modify macro", errMsg);
         return;
      }
   }
   window->smartIndentData = (void*)winData;
}

// --------------------------------------------------------------------------
void EndSmartIndent(WindowInfo* window)
{
   windowSmartIndentData* winData =
      (windowSmartIndentData*)window->smartIndentData;

   if (winData == NULL)
      return;

   /* Free programs and allocated data */
   if (winData->modMacro != NULL)
      FreeProgram(winData->modMacro);
   FreeProgram(winData->newlineMacro);
   free__((char*)winData);
   window->smartIndentData = NULL;
}

// --------------------------------------------------------------------------
// Returns true if there are smart indent macros for a named language
// --------------------------------------------------------------------------
int SmartIndentMacrosAvailable(const char* languageModeName)
{
   return findIndentSpec(languageModeName) != NULL;
}

// --------------------------------------------------------------------------
// Attaches to the text widget's smart-indent callback to invoke a user
// defined macro when the text widget requires an indent (not just when the
// user types a newline, but also when the widget does an auto-wrap with
// auto-indent on), or the user types some other character.
// --------------------------------------------------------------------------
void SmartIndentCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = WidgetToWindow(w);
   smartIndentCBStruct* cbInfo = (smartIndentCBStruct*)data;

   if (window->smartIndentData == NULL)
      return;
   if (cbInfo->reason == CHAR_TYPED)
      executeModMacro(window, cbInfo);
   else if (cbInfo->reason == NEWLINE_INDENT_NEEDED)
      executeNewlineMacro(window, cbInfo);
}

// --------------------------------------------------------------------------
// Run the newline macro with information from the smart-indent callback
// structure passed by the widget
// --------------------------------------------------------------------------
static void executeNewlineMacro(WindowInfo* window, smartIndentCBStruct* cbInfo)
{
   windowSmartIndentData* winData = (windowSmartIndentData*)window->smartIndentData;
   /* posValue probably shouldn't be static due to re-entrance issues <slobasso> */
   static DataValue posValue = {INT_TAG, {0}};
   DataValue result;
   RestartData* continuation;
   char* errMsg;
   int stat;

   /* Beware of recursion: the newline macro may insert a string which
   triggers the newline macro to be called again and so on. Newline
   macros shouldn't insert strings, but nedit must not crash either if
   they do. */
   if (winData->inNewLineMacro)
      return;

   /* Call newline macro with the position at which to add newline/indent */
   posValue.val.n = cbInfo->pos;
   ++(winData->inNewLineMacro);
   stat = ExecuteMacro(window, winData->newlineMacro, 1, &posValue, &result,
      &continuation, &errMsg);

   /* Don't allow preemption or time limit.  Must get return value */
   while (stat == MACRO_TIME_LIMIT)
      stat = ContinueMacro(continuation, &result, &errMsg);

   --(winData->inNewLineMacro);
   /* Collect Garbage.  Note that the mod macro does not collect garbage,
   (because collecting per-line is more efficient than per-character)
   but GC now depends on the newline macro being mandatory */
   SafeGC();

   /* Process errors in macro execution */
   if (stat == MACRO_PREEMPT || stat == MACRO_ERROR)
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Smart Indent", "Error in smart indent macro:\n%s", "OK", stat == MACRO_ERROR ? errMsg : "dialogs and shell commands not permitted");
      EndSmartIndent(window);
      return;
   }

   /* Validate and return the result */
   if (result.tag != INT_TAG || result.val.n < -1 || result.val.n > 1000)
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Smart Indent", "Smart indent macros must return\ninteger indent distance", "OK");
      EndSmartIndent(window);
      return;
   }

   cbInfo->indentRequest = result.val.n;
}

// --------------------------------------------------------------------------
bool InSmartIndentMacros(WindowInfo* window)
{
   windowSmartIndentData* winData = (windowSmartIndentData*)window->smartIndentData;

   return((winData && (winData->inModMacro || winData->inNewLineMacro)));
}

// --------------------------------------------------------------------------
// Run the modification macro with information from the smart-indent callback
// structure passed by the widget
// --------------------------------------------------------------------------
static void executeModMacro(WindowInfo* window,smartIndentCBStruct* cbInfo)
{
   windowSmartIndentData* winData =
      (windowSmartIndentData*)window->smartIndentData;
   /* args probably shouldn't be static due to future re-entrance issues <slobasso> */
   static DataValue args[2] = {{INT_TAG, {0}}, {STRING_TAG, {0}}};
   /* after 5.2 release remove inModCB and use new winData->inModMacro value */
   static int inModCB = false;
   DataValue result;
   RestartData* continuation;
   char* errMsg;
   int stat;

   /* Check for inappropriate calls and prevent re-entering if the macro
   makes a buffer modification */
   if (winData == NULL || winData->modMacro == NULL || inModCB)
      return;

   /* Call modification macro with the position of the modification,
   and the character(s) inserted.  Don't allow
   preemption or time limit.  Execution must not overlap or re-enter */
   args[0].val.n = cbInfo->pos;
   AllocNStringCpy(&args[1].val.str, cbInfo->charsTyped);

   inModCB = true;
   ++(winData->inModMacro);

   stat = ExecuteMacro(window, winData->modMacro, 2, args, &result,
      &continuation, &errMsg);
   while (stat == MACRO_TIME_LIMIT)
      stat = ContinueMacro(continuation, &result, &errMsg);

   --(winData->inModMacro);
   inModCB = false;

   /* Process errors in macro execution */
   if (stat == MACRO_PREEMPT || stat == MACRO_ERROR)
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Smart Indent", "Error in smart indent modification macro:\n%s", "OK", stat == MACRO_ERROR ? errMsg : "dialogs and shell commands not permitted");
      EndSmartIndent(window);
      return;
   }
}

// --------------------------------------------------------------------------
void EditSmartIndentMacros(WindowInfo* window)
{
   // if the dialog is already displayed, just pop it to the top and return
   if (SmartIndentDialog.shell != NULL)
   {
      SmartIndentDialog.shell->show();
      return;
   }

   if (LanguageModeName(0) == NULL)
   {
      DialogF(DF_WARN, window->mainWindow, 1, "Language Mode", "No Language Modes defined", "OK");
      return;
   }

   // Decide on an initial language mode
   char* lmName = LanguageModeName(window->languageMode == PLAIN_LANGUAGE_MODE ? 0 : window->languageMode);
   SmartIndentDialog.langModeName = NeNewString(lmName);

   // Create a form widget in an application shell
   SmartIndentDialog.shell = new Fl_Double_Window(30, 50, 650, 650, "Program Smart Indent Macros");
   SmartIndentDialog.shell->callback(closeCB, NULL);

   Fl_Button* commonBtn = new Fl_Button(5,5, 150, 40, "&Common / Shared\nInitialization...");
   commonBtn->callback(commonDialogCB);

   SmartIndentDialog.lmOptMenu = new Fl_Choice(300, 15, 140, 25, "&Language Mode:");
   SmartIndentDialog.lmOptMenu->callback(langModeCB);
   CreateLanguageModeChoice(SmartIndentDialog.lmOptMenu);

   Fl_Button* lmBtn = new Fl_Button(500, 5, 145, 40, "&Add / Modify\nLanguage Mode...");
   lmBtn->callback(lmDialogCB);

   Fl_Group* buttonLine = new Fl_Group(0, 605, 650, 45);
   buttonLine->box(FL_ENGRAVED_FRAME);

   Fl_Button* btnOk = new Fl_Button(10, 615, 70, 25, "Ok");
   btnOk->shortcut(FL_Enter);
   btnOk->callback(okCB);

   Fl_Button* btnApply = new Fl_Button(90, 615, 70, 25, "Appl&y");
   btnApply->callback(applyCB);

   Fl_Button* checkBtn = new Fl_Button(170, 615, 70, 25, "Chec&k");
   checkBtn->callback(checkCB);

   Fl_Button* deleteBtn = new Fl_Button(250, 615, 70, 25, "&Delete");
   deleteBtn->callback(deleteCB);

   Fl_Button* restoreBtn = new Fl_Button(330, 615, 140, 25, "Restore De&faults");
   restoreBtn->callback(restoreCB);

   Fl_Button* closeBtn = new Fl_Button(480, 615, 70, 25, "Close");
   closeBtn->callback(closeCB);
   closeBtn->shortcut(FL_Escape);

   Fl_Button* helpBtn = new Fl_Button(560, 615, 70, 25, "&Help");
   helpBtn->callback(helpCB);

   buttonLine->end();

   Fl_Tile* tile = new Fl_Tile(5, 50, 640, 550);
   {
      Fl_Group* initMacroGroup = new Fl_Group(5,50, 640, 150);
      initMacroGroup->box(FL_FLAT_BOX);
      {
         Fl_Box* initLbl = new Fl_Box(10, 50, 400, 25, "Language Specific Initialization Macro Commands and Definitions");
         initLbl->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
         SmartIndentDialog.initMacro = new Fl_Text_Editor(5,75, 640, 125);
         SmartIndentDialog.initMacroBuffer = new Fl_Text_Buffer();
         SmartIndentDialog.initMacro->buffer(SmartIndentDialog.initMacroBuffer);
      }
      initMacroGroup->resizable(SmartIndentDialog.initMacro);
      initMacroGroup->end();

      Fl_Group* newLineMacroGroup = new Fl_Group(5,200, 640, 150);
      newLineMacroGroup->box(FL_FLAT_BOX);
      {
         Fl_Box* newLineLbl = new Fl_Box(10, 200, 100, 25, "Newline Macro");
         newLineLbl->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
         Fl_Box* newLineArgLbl = new Fl_Box(320, 200, 220, 25, "($1 is insert position, return indent request or -1)");
         newLineArgLbl->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
         SmartIndentDialog.newlineMacro = new Fl_Text_Editor(5, 225, 640, 125);
         SmartIndentDialog.newlineMacroBuffer = new Fl_Text_Buffer();
         SmartIndentDialog.newlineMacro->buffer(SmartIndentDialog.newlineMacroBuffer);
      }
      newLineMacroGroup->resizable(SmartIndentDialog.newlineMacro);
      newLineMacroGroup->end();

      Fl_Group* modMacroGroup = new Fl_Group(5,350, 640, 250);
      modMacroGroup->box(FL_FLAT_BOX);
      {
         Fl_Box* modLbl = new Fl_Box(10, 350, 100, 25, "Type-in Macro");
         modLbl->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
         Fl_Box* modAgrLbl = new Fl_Box(340, 350, 240, 25, "($1 is position, $2 is character to be inserted)");
         modAgrLbl->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
         SmartIndentDialog.modMacro = new Fl_Text_Editor(5, 380, 640, 220, "");
         SmartIndentDialog.modMacroBuffer = new Fl_Text_Buffer();
         SmartIndentDialog.modMacro->buffer(SmartIndentDialog.modMacroBuffer);
      }
      modMacroGroup->resizable(SmartIndentDialog.modMacro);
      modMacroGroup->end();
   }
   tile->end();

   SmartIndentDialog.shell->resizable(tile);

   // Fill in the dialog information for the selected language mode
   setSmartIndentDialogData(findIndentSpec(lmName));
   SetLangModeMenu(SmartIndentDialog.lmOptMenu,SmartIndentDialog.langModeName);

   // Realize all of the widgets in the new dialog
   SmartIndentDialog.shell->show();
}

// --------------------------------------------------------------------------
static void destroyCB(Fl_Widget* w, void* data)
{
   delete[] SmartIndentDialog.langModeName;
   SmartIndentDialog.langModeName = NULL;
   
   delete SmartIndentDialog.shell;
   SmartIndentDialog.shell = NULL;

   delete SmartIndentDialog.initMacroBuffer;
   SmartIndentDialog.initMacroBuffer = NULL;

   delete SmartIndentDialog.modMacroBuffer;
   SmartIndentDialog.modMacroBuffer = NULL;

   delete SmartIndentDialog.newlineMacroBuffer;
   SmartIndentDialog.newlineMacroBuffer = NULL;
}

// --------------------------------------------------------------------------
static void langModeCB(Fl_Widget* w, void* data)
{
   TRACE();
   static smartIndentRec emptyIndentSpec = {NULL, NULL, NULL, NULL};

   // Get the newly selected mode name.  If it's the same, do nothing
   const char* modeName = SmartIndentDialog.lmOptMenu->text();
   if (modeName == NULL || !strcmp(modeName, SmartIndentDialog.langModeName))
      return;

   // Find the original macros
   int i = 0;
   for (i=0; i<NSmartIndentSpecs; i++)
      if (!strcmp(SmartIndentDialog.langModeName,SmartIndentSpecs[i]->lmName))
         break;
   smartIndentRec* oldMacros = (i == NSmartIndentSpecs ? &emptyIndentSpec : SmartIndentSpecs[i]);

   // Check if the macros have changed, if so allow user to apply, discard, or cancel
   smartIndentRec* newMacros = getSmartIndentDialogData();
   if (indentSpecsDiffer(oldMacros, newMacros))
   {
      int resp = DialogF(DF_QUES, SmartIndentDialog.shell, 3, "Smart Indent",
         "Smart indent macros for language mode\n"
         "%s were changed.  Apply changes?",
         "Apply", "Discard", "Cancel",
         SmartIndentDialog.langModeName);

      if (resp == 2 /*discard */ || resp == 0 /* cancel */)
      {
         SetLangModeMenu(SmartIndentDialog.lmOptMenu, SmartIndentDialog.langModeName);
         return;
      }
      else if (resp == 1 /* apply */)
      {
         if (checkSmartIndentDialogData())
         {
            if (oldMacros == &emptyIndentSpec)
            {
               SmartIndentSpecs[NSmartIndentSpecs++] = copyIndentSpec(newMacros);
            }
            else
            {
               freeIndentSpec(oldMacros);
               SmartIndentSpecs[i] = copyIndentSpec(newMacros);
            }
         }
         else
         {
            SetLangModeMenu(SmartIndentDialog.lmOptMenu, SmartIndentDialog.langModeName);
            return;
         }
      }
   }
   freeIndentSpec(newMacros);

   // Fill the dialog with the new language mode information
   SmartIndentDialog.langModeName = NeNewString(modeName);
   setSmartIndentDialogData(findIndentSpec(modeName));
}

// --------------------------------------------------------------------------
static void lmDialogCB(Fl_Widget* w, void* data)
{
   TRACE();
   EditLanguageModes();
}

// --------------------------------------------------------------------------
static void commonDialogCB(Fl_Widget* w, void* data)
{
   TRACE();
   EditCommonSmartIndentMacro();
}

// --------------------------------------------------------------------------
static void okCB(Fl_Widget* w, void* data)
{
   TRACE();
   // change the macro
   if (!updateSmartIndentData())
      return;

   // pop down and destroy the dialog
   destroyCB(SmartIndentDialog.shell, &SmartIndentDialog);
}

// --------------------------------------------------------------------------
static void applyCB(Fl_Widget* w, void* data)
{
   TRACE();
   // change the patterns
   updateSmartIndentData();
}

// --------------------------------------------------------------------------
static void checkCB(Fl_Widget* w, void* data)
{
   TRACE();
   if (checkSmartIndentDialogData())
      DialogF(DF_INF, SmartIndentDialog.shell, 1, "Macro compiled", "Macros compiled without error", "OK");
}

// --------------------------------------------------------------------------
static void restoreCB(Fl_Widget* w, void* data)
{
   TRACE();
   int i;
   smartIndentRec* defaultIS;

   /* Find the default indent spec */
   for (i=0; i<N_DEFAULT_INDENT_SPECS; i++)
   {
      if (!strcmp(SmartIndentDialog.langModeName,
         DefaultIndentSpecs[i].lmName))
      {
         break;
      }
   }

   if (i == N_DEFAULT_INDENT_SPECS)
   {
      DialogF(DF_WARN, SmartIndentDialog.shell, 1, "Smart Indent",
         "There are no default indent macros\nfor language mode %s",
         "OK", SmartIndentDialog.langModeName);
      return;
   }
   defaultIS = &DefaultIndentSpecs[i];

   if (DialogF(DF_WARN, SmartIndentDialog.shell, 2, "Discard Changes",
      "Are you sure you want to discard\n"
      "all changes to smart indent macros\n"
      "for language mode %s?", "Discard", "Cancel",
      SmartIndentDialog.langModeName) == 2)
   {
      return;
   }

   /* if a stored version of the indent macros exist, replace them, if not,
   add a new one */
   for (i=0; i<NSmartIndentSpecs; i++)
      if (!strcmp(SmartIndentDialog.langModeName,SmartIndentSpecs[i]->lmName))
         break;
   if (i < NSmartIndentSpecs)
   {
      freeIndentSpec(SmartIndentSpecs[i]);
      SmartIndentSpecs[i] = copyIndentSpec(defaultIS);
   }
   else
      SmartIndentSpecs[NSmartIndentSpecs++] = copyIndentSpec(defaultIS);

   /* Update the dialog */
   setSmartIndentDialogData(defaultIS);
}

// --------------------------------------------------------------------------
static void deleteCB(Fl_Widget* w, void* data)
{
   TRACE();

   int resp = DialogF(DF_WARN, SmartIndentDialog.shell, 2, "Delete Macros",
      "Are you sure you want to delete smart indent\n"
      "macros for language mode %s?", "Yes, Delete", "Cancel",
      SmartIndentDialog.langModeName);
   if (resp == 0)
   {
      return;
   }

   /* if a stored version of the pattern set exists, delete it from the list */
   int i = 0;
   for (i=0; i<NSmartIndentSpecs; i++)
      if (!strcmp(SmartIndentDialog.langModeName,SmartIndentSpecs[i]->lmName))
         break;
   if (i < NSmartIndentSpecs)
   {
      freeIndentSpec(SmartIndentSpecs[i]);
      memmove(&SmartIndentSpecs[i], &SmartIndentSpecs[i+1], (NSmartIndentSpecs-1 - i) * sizeof(smartIndentRec*));
      NSmartIndentSpecs--;
   }

   /* Clear out the dialog */
   setSmartIndentDialogData(NULL);
}

// --------------------------------------------------------------------------
static void closeCB(Fl_Widget* w, void* data)
{
   TRACE();
   // pop down and destroy the dialog 
   CloseAllPopupsFor(SmartIndentDialog.shell);

   // pop down and destroy the dialog
   destroyCB(SmartIndentDialog.shell, &SmartIndentDialog);
}

// --------------------------------------------------------------------------
static void helpCB(Fl_Widget* w, void* data)
{
   TRACE();
   Help(HELP_SMART_INDENT);
}

// --------------------------------------------------------------------------
static int checkSmartIndentDialogData()
{
   char* errMsg, *stoppedAt;

   // Check the initialization macro
   if (SmartIndentDialog.initMacro->buffer()->length()!=0)
   {
      char* widgetText = ensureNewline(NeNewString(SmartIndentDialog.initMacro->buffer()->text()));
      if (!CheckMacroString(SmartIndentDialog.shell, widgetText, "initialization Macro", &stoppedAt))
      {
         SmartIndentDialog.initMacro->insert_position(stoppedAt - widgetText);
         SmartIndentDialog.shell->focus(SmartIndentDialog.initMacro);
         free__(widgetText);
         return false;
      }
      free__(widgetText);
   }

   // Test compile the newline macro
   if (SmartIndentDialog.newlineMacro->buffer()->length()==0)
   {
      DialogF(DF_WARN, SmartIndentDialog.shell, 1, "Smart Indent", "Newline macro required", "OK");
      return false;
   }

   char* widgetText = ensureNewline(NeNewString(SmartIndentDialog.newlineMacro->buffer()->text()));
   Program* prog = ParseMacro(widgetText, &errMsg, &stoppedAt);
   if (prog == NULL)
   {
      ParseError(SmartIndentDialog.shell, widgetText, stoppedAt, "Newline Macro", errMsg);
      SmartIndentDialog.newlineMacro->insert_position(stoppedAt - widgetText);
      SmartIndentDialog.shell->focus(SmartIndentDialog.newlineMacro);
      free__(widgetText);
      return false;
   }
   free__(widgetText);
   FreeProgram(prog);

   // Test compile the modify macro
   if (SmartIndentDialog.modMacro->buffer()->length()!=0)
   {
      char* widgetText = ensureNewline(NeNewString(SmartIndentDialog.modMacro->buffer()->text()));
      prog = ParseMacro(widgetText, &errMsg, &stoppedAt);
      if (prog == NULL)
      {
         ParseError(SmartIndentDialog.shell, widgetText, stoppedAt, "Type-in Macro", errMsg);
         SmartIndentDialog.modMacro->insert_position(stoppedAt - widgetText);
         SmartIndentDialog.shell->focus(SmartIndentDialog.modMacro);
         free__(widgetText);
         return false;
      }
      free__(widgetText);
      FreeProgram(prog);
   }
   return true;
}

// --------------------------------------------------------------------------
static smartIndentRec* getSmartIndentDialogData()
{
   smartIndentRec* is = new smartIndentRec();
   is->lmName = NeNewString(SmartIndentDialog.langModeName);
   is->initMacro = (SmartIndentDialog.initMacro->buffer()->length() == 0 ? NULL : ensureNewline(NeNewString(SmartIndentDialog.initMacro->buffer()->text())));
   is->newlineMacro = (SmartIndentDialog.newlineMacro->buffer()->length() == 0 ? NULL: ensureNewline(NeNewString(SmartIndentDialog.newlineMacro->buffer()->text())));
   is->modMacro = (SmartIndentDialog.modMacro->buffer()->length() == 0 ? NULL : ensureNewline(NeNewString(SmartIndentDialog.modMacro->buffer()->text())));
   return is;
}

// --------------------------------------------------------------------------
static void setSmartIndentDialogData(smartIndentRec* is)
{
   if (is == NULL)
   {
      SmartIndentDialog.initMacro->buffer()->text("");
      SmartIndentDialog.newlineMacro->buffer()->text("");
      SmartIndentDialog.modMacro->buffer()->text("");
   }
   else
   {
      if (is->initMacro == NULL)
         SmartIndentDialog.initMacro->buffer()->text("");
      else
         SmartIndentDialog.initMacro->buffer()->text(is->initMacro);
      
      SmartIndentDialog.newlineMacro->buffer()->text(is->newlineMacro);
      if (is->modMacro == NULL)
         SmartIndentDialog.modMacro->buffer()->text("");
      else
         SmartIndentDialog.modMacro->buffer()->text(is->modMacro);
   }
}

// --------------------------------------------------------------------------
void EditCommonSmartIndentMacro()
{
   // if the dialog is already displayed, just pop it to the top and return
   if (CommonDialog.shell != NULL)
   {
      CommonDialog.shell->show();
      return;
   }

   // Create a form widget in an application shell
   CommonDialog.shell = new Fl_Double_Window(30, 50, 650, 600, "Common Smart Indent Macros");
   CommonDialog.shell->callback(comDestroyCB, &CommonDialog);

   Fl_Box* topLbl = new Fl_Box(5, 5, 300, 25, "Common Definitions for Smart Indent Macros");
   topLbl->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

   CommonDialog.text = new Fl_Text_Editor(5, 30, 640, 520);
   CommonDialog.buffer = new Fl_Text_Buffer();
   CommonDialog.buffer->text(CommonMacros);
   CommonDialog.text->buffer(CommonDialog.buffer);

   Fl_Group* buttonLine = new Fl_Group(0, 555, 650, 45);
   buttonLine->box(FL_ENGRAVED_FRAME);

   Fl_Button* btnOk = new Fl_Button(30, 565, 100, 25, "Ok");
   btnOk->shortcut(FL_Enter);
   btnOk->callback(comOKCB);

   Fl_Button* btnApply = new Fl_Button(140, 565, 100, 25, "Appl&y");
   btnApply->callback(comApplyCB);

   Fl_Button* checkBtn = new Fl_Button(250, 565, 100, 25, "Chec&k");
   checkBtn->callback(comCheckCB);

   Fl_Button* restoreBtn = new Fl_Button(360, 565, 150, 25, "Restore De&faults");
   restoreBtn->callback(comRestoreCB);

   Fl_Button* closeBtn = new Fl_Button(520, 565, 100, 25, "Close");
   closeBtn->callback(comCloseCB);
   closeBtn->shortcut(FL_Escape);

   buttonLine->end();

   CommonDialog.shell->resizable(CommonDialog.text);

   // Realize all of the widgets in the new dialog
   CommonDialog.shell->show();
}

// --------------------------------------------------------------------------
static void comDestroyCB(Fl_Widget* w, void* data)
{
   delete CommonDialog.shell;
   CommonDialog.shell = NULL;

   delete CommonDialog.buffer;
   CommonDialog.buffer = NULL;
}

// --------------------------------------------------------------------------
static void comOKCB(Fl_Widget* w, void* data)
{
   TRACE();
   // change the macro
   if (!updateSmartIndentCommonData())
      return;

   // pop down and destroy the dialog
   comDestroyCB(CommonDialog.shell, &CommonDialog);
}

// --------------------------------------------------------------------------
static void comApplyCB(Fl_Widget* w, void* data)
{
   TRACE();
   // change the macro
   updateSmartIndentCommonData();
}

// --------------------------------------------------------------------------
static void comCheckCB(Fl_Widget* w, void* data)
{
   TRACE();
   if (checkSmartIndentCommonDialogData())
   {
      DialogF(DF_INF, CommonDialog.shell, 1, "Macro compiled", "Macros compiled without error", "OK");
   }
}

// --------------------------------------------------------------------------
static void comRestoreCB(Fl_Widget* w, void* data)
{
   TRACE();
   int resp = DialogF(DF_WARN, CommonDialog.shell, 2, "Discard Changes",
      "Are you sure you want to discard all\n"
      "changes to common smart indent macros", "Discard", "Cancel");
   if (resp == 0 /* cancel */)
      return;

   // replace common macros with default
   free__(CommonMacros);
   CommonMacros = NeNewString(DefaultCommonMacros);

   // Update the dialog
   CommonDialog.text->buffer()->text(CommonMacros);
}

// --------------------------------------------------------------------------
static void comCloseCB(Fl_Widget* w, void* data)
{
   TRACE();
   // pop down and destroy the dialog
   comDestroyCB(CommonDialog.shell, &CommonDialog);
}

// --------------------------------------------------------------------------
// Update the smart indent macros being edited in the dialog
// with the information that the dialog is currently displaying, and
// apply changes to any window which is currently using the macros.
// --------------------------------------------------------------------------
static int updateSmartIndentCommonData()
{
   TRACE();

   // Make sure the patterns are valid and compile
   if (!checkSmartIndentCommonDialogData())
      return false;

   // Get the current data
   CommonMacros = ensureNewline(NeNewString(CommonDialog.buffer->text()));

   // Re-execute initialization macros (macros require a window to function,
   // since user could theoretically execute an action routine, but it
   // probably won't be referenced in a smart indent initialization)
   if (!ReadMacroString(WindowList, CommonMacros, "common macros"))
      return false;

   // Find windows that are currently using smart indent and
   // re-initialize the smart indent macros (in case they have initialization
   // data which depends on common data)
   for (WindowInfo* window=WindowList; window!=NULL; window=window->next)
   {
      if (window->indentStyle == SMART_INDENT &&
         window->languageMode != PLAIN_LANGUAGE_MODE)
      {
         EndSmartIndent(window);
         BeginSmartIndent(window, false);
      }
   }

   // Note that preferences have been changed
   MarkPrefsChanged();

   return true;
}

// --------------------------------------------------------------------------
static int checkSmartIndentCommonDialogData()
{
   char* stoppedAt;

   if (CommonDialog.buffer->length()!=0)
   {
      char* widgetText = ensureNewline(NeNewString(CommonDialog.buffer->text()));
      if (!CheckMacroString(CommonDialog.shell, widgetText, "macros", &stoppedAt))
      {
         CommonDialog.text->insert_position(stoppedAt-widgetText);
         CommonDialog.shell->focus(CommonDialog.text);
         free__(widgetText);
         return false;
      }
      free__(widgetText);
   }
   return true;
}

// --------------------------------------------------------------------------
// Update the smart indent macros being edited in the dialog
// with the information that the dialog is currently displaying, and
// apply changes to any window which is currently using the macros.
// --------------------------------------------------------------------------
static int updateSmartIndentData()
{

   // Make sure the patterns are valid and compile
   if (!checkSmartIndentDialogData())
      return false;

   // Get the current data
   smartIndentRec* newMacros = getSmartIndentDialogData();

   // Find the original macros
   int i = 0;
   for (i=0; i<NSmartIndentSpecs; i++)
      if (!strcmp(SmartIndentDialog.langModeName,SmartIndentSpecs[i]->lmName))
         break;

   // If it's a new language, add it at the end, otherwise free__ the
   // existing macros and replace it
   if (i == NSmartIndentSpecs)
   {
      SmartIndentSpecs[NSmartIndentSpecs++] = newMacros;
   }
   else
   {
      freeIndentSpec(SmartIndentSpecs[i]);
      SmartIndentSpecs[i] = newMacros;
   }

   // Find windows that are currently using this indent specification and
   // re-do the smart indent macros
   for (WindowInfo* window =WindowList; window!=NULL; window=window->next)
   {
      char* lmName = LanguageModeName(window->languageMode);
      if (lmName != NULL && !strcmp(lmName, newMacros->lmName))
      {
         SetSensitive(window, window->smartIndentItem, true);
         if (window->indentStyle == SMART_INDENT &&
            window->languageMode != PLAIN_LANGUAGE_MODE)
         {
            EndSmartIndent(window);
            BeginSmartIndent(window, false);
         }
      }
   }

   // Note that preferences have been changed
   MarkPrefsChanged();

   return true;
}

// --------------------------------------------------------------------------
static int loadDefaultIndentSpec(char* lmName)
{
   for (int i=0; i<N_DEFAULT_INDENT_SPECS; i++)
   {
      if (!strcmp(lmName, DefaultIndentSpecs[i].lmName))
      {
         SmartIndentSpecs[NSmartIndentSpecs++] = copyIndentSpec(&DefaultIndentSpecs[i]);
         return true;
      }
   }
   return false;
}

// --------------------------------------------------------------------------
int LoadSmartIndentString(char* inString)
{
   char* errMsg, *inPtr = inString;
   smartIndentRec is, *isCopy;
   int i;

   for (;;)
   {
      /* skip over blank space */
      inPtr += strspn(inPtr, " \t\n");

      /* finished */
      if (*inPtr == '\0')
         return true;

      /* read language mode name */
      is.lmName = ReadSymbolicField(&inPtr);
      if (is.lmName == NULL)
         return siParseError(inString, inPtr, "language mode name required");
      if (!SkipDelimiter(&inPtr, &errMsg))
      {
         delete[] is.lmName;
         return siParseError(inString, inPtr, errMsg);
      }

      /* look for "Default" keyword, and if it's there, return the default smart indent macros */
      if (!strncmp(inPtr, "Default", 7))
      {
         inPtr += 7;
         if (!loadDefaultIndentSpec(is.lmName))
         {
            delete[] is.lmName;
            return siParseError(inString, inPtr,
               "no default smart indent macros");
         }
         delete[] is.lmName;
         continue;
      }

      /* read the initialization macro (arbitrary text terminated by the macro end boundary string) */
      is.initMacro = readSIMacro(&inPtr);
      if (is.initMacro == NULL)
      {
         delete[] is.lmName;
         return siParseError(inString, inPtr, "no end boundary to initialization macro");
      }

      /* read the newline macro */
      is.newlineMacro = readSIMacro(&inPtr);
      if (is.newlineMacro == NULL)
      {
         delete[] is.lmName;
         delete[] is.initMacro;
         return siParseError(inString, inPtr, "no end boundary to newline macro");
      }

      /* read the modify macro */
      is.modMacro = readSIMacro(&inPtr);
      if (is.modMacro == NULL)
      {
         delete[] is.lmName;
         delete[] is.initMacro;
         delete[] is.newlineMacro;
         return siParseError(inString, inPtr, "no end boundary to modify macro");
      }

      /* if there's no mod macro, make it null so it won't be executed */
      if (is.modMacro[0] == '\0')
      {
         delete[] is.modMacro;
         is.modMacro = NULL;
      }

      /* create a new data structure and add/change it in the list */
      isCopy = new smartIndentRec();
      *isCopy = is;
      for (i=0; i<NSmartIndentSpecs; i++)
      {
         if (!strcmp(SmartIndentSpecs[i]->lmName, is.lmName))
         {
            freeIndentSpec(SmartIndentSpecs[i]);
            SmartIndentSpecs[i] = isCopy;
            break;
         }
      }
      if (i == NSmartIndentSpecs)
         SmartIndentSpecs[NSmartIndentSpecs++] = isCopy;
   }
}

// --------------------------------------------------------------------------
int LoadSmartIndentCommonString(char* inString)
{
   int shiftedLen;
   char* inPtr = inString;

   /* If called from -import, can replace existing ones */
   delete[] CommonMacros;

   /* skip over blank space */
   inPtr += strspn(inPtr, " \t\n");

   /* look for "Default" keyword, and if it's there, return the default smart common macro */
   if (!strncmp(inPtr, "Default", 7))
   {
      CommonMacros = NeNewString(DefaultCommonMacros);
      return true;
   }

   /* Remove leading tabs added by writer routine */
   CommonMacros = ShiftText(inPtr, SHIFT_LEFT, true, 8, 8, &shiftedLen);
   return true;
}

// --------------------------------------------------------------------------
// Read a macro (arbitrary text terminated by the macro end boundary string)
// from the position pointed to by *inPtr, trim off added tabs and return an
// allocated copy of the string, and advance *inPtr to the end of the macro.
// Returns NULL if the macro end boundary string is not found.
// --------------------------------------------------------------------------
static char* readSIMacro(char** inPtr)
{
   char* retStr, *macroStr, *macroEnd;
   int shiftedLen;

   /* Strip leading newline */
   if (**inPtr == '\n')
      (*inPtr)++;

   /* Find the end of the macro */
   macroEnd = strstr(*inPtr, MacroEndBoundary);
   if (macroEnd == NULL)
      return NULL;

   /* Copy the macro */
   macroStr = new char[macroEnd - *inPtr + 1];
   strncpy(macroStr, *inPtr, macroEnd - *inPtr);
   macroStr[macroEnd - *inPtr] = '\0';

   /* Remove leading tabs added by writer routine */
   *inPtr = macroEnd + strlen(MacroEndBoundary);
   retStr = ShiftText(macroStr, SHIFT_LEFT, true, 8, 8, &shiftedLen);
   delete[]macroStr;
   return retStr;
}

// --------------------------------------------------------------------------
static smartIndentRec* copyIndentSpec(smartIndentRec* is)
{
   smartIndentRec* ris = new smartIndentRec();
   ris->lmName = NeNewString(is->lmName);
   ris->initMacro = NeNewString(is->initMacro);
   ris->newlineMacro = NeNewString(is->newlineMacro);
   ris->modMacro = NeNewString(is->modMacro);
   return ris;
}

// --------------------------------------------------------------------------
static void freeIndentSpec(smartIndentRec* is)
{
   delete[] is->lmName;
   if (is->initMacro != NULL) delete[] is->initMacro;
   delete[] is->newlineMacro;
   if (is->modMacro != NULL) delete[] is->modMacro;
}

// --------------------------------------------------------------------------
static int indentSpecsDiffer(smartIndentRec* is1, smartIndentRec* is2)
{
   return AllocatedStringsDiffer(is1->initMacro, is2->initMacro) ||
      AllocatedStringsDiffer(is1->newlineMacro, is2->newlineMacro) ||
      AllocatedStringsDiffer(is1->modMacro, is2->modMacro);
}

// --------------------------------------------------------------------------
static int siParseError(char* stringStart, char* stoppedAt, char* message)
{
   return ParseError(NULL, stringStart, stoppedAt, "smart indent specification", message);
}

// --------------------------------------------------------------------------
char* WriteSmartIndentString()
{
   int i;
   smartIndentRec* sis;
   Ne_Text_Buffer* outBuf;
   char* outStr, *escapedStr;

   outBuf = BufCreate();
   for (i=0; i<NSmartIndentSpecs; i++)
   {
      sis = SmartIndentSpecs[i];
      BufInsert(outBuf, outBuf->length, "\t");
      BufInsert(outBuf, outBuf->length, sis->lmName);
      BufInsert(outBuf, outBuf->length, ":");
      if (isDefaultIndentSpec(sis))
         BufInsert(outBuf, outBuf->length, "Default\n");
      else
      {
         insertShiftedMacro(outBuf, sis->initMacro);
         insertShiftedMacro(outBuf, sis->newlineMacro);
         insertShiftedMacro(outBuf, sis->modMacro);
      }
   }

   /* Get the output string, and lop off the trailing newline */
   outStr = BufGetRange(outBuf, 0, outBuf->length > 0 ? outBuf->length-1 : 0);
   BufFree(outBuf);

   /* Protect newlines and backslashes from translation by the resource reader */
   escapedStr = EscapeSensitiveChars(outStr);
   delete[] outStr;
   return escapedStr;
}

// --------------------------------------------------------------------------
char* WriteSmartIndentCommonString()
{
   int len;
   char* outStr, *escapedStr;

   if (!strcmp(CommonMacros, DefaultCommonMacros))
      return NeNewString("Default");
   if (CommonMacros == NULL)
      return NeNewString("");

   /* Shift the macro over by a tab to keep .nedit file bright and clean */
   outStr = ShiftText(CommonMacros, SHIFT_RIGHT, true, 8, 8, &len);

   /* Protect newlines and backslashes from translation by the resource reader */
   escapedStr = EscapeSensitiveChars(outStr);
   free__(outStr);

   /* If there's a trailing escaped newline, remove it */
   len = strlen(escapedStr);
   if (len > 1 && escapedStr[len-1] == '\n' && escapedStr[len-2] == '\\')
      escapedStr[len-2] = '\0';
   return escapedStr;
}

// --------------------------------------------------------------------------
// Insert macro text "macro" into buffer "buf" shifted right by 8 characters
// (so it looks nice in the .nedit file), and terminated with a macro-end-
// boundary string.
// --------------------------------------------------------------------------
static void insertShiftedMacro(Ne_Text_Buffer* buf, char*  macro)
{
   char* shiftedMacro;
   int shiftedLen;

   if (macro != NULL)
   {
      shiftedMacro = ShiftText(macro, SHIFT_RIGHT, true, 8, 8, &shiftedLen);
      BufInsert(buf, buf->length, shiftedMacro);
      free__(shiftedMacro);
   }
   BufInsert(buf, buf->length, "\t");
   BufInsert(buf, buf->length, MacroEndBoundary);
   BufInsert(buf, buf->length, "\n");
}

// --------------------------------------------------------------------------
static int isDefaultIndentSpec(smartIndentRec* indentSpec)
{
   int i;

   for (i=0; i<N_DEFAULT_INDENT_SPECS; i++)
      if (!strcmp(indentSpec->lmName, DefaultIndentSpecs[i].lmName))
         return !indentSpecsDiffer(indentSpec, &DefaultIndentSpecs[i]);
   return false;
}

// --------------------------------------------------------------------------
static smartIndentRec* findIndentSpec(const char* modeName)
{
   if (modeName == NULL)
      return NULL;

   for (int i=0; i<NSmartIndentSpecs; i++)
      if (!strcmp(modeName, SmartIndentSpecs[i]->lmName))
         return SmartIndentSpecs[i];
   return NULL;
}

// --------------------------------------------------------------------------
// If "string" is not terminated with a newline character,  return a
// reallocated string which does end in a newline (otherwise, just pass on
// string as function value).  (The macro language requires newline terminators
// for statements, but the text widget doesn't force it like the NEdit text
// buffer does, so this might avoid some confusion.)
// --------------------------------------------------------------------------
static char* ensureNewline(char* string)
{
   char* newString;
   int length;

   if (string == NULL)
      return NULL;
   length = strlen(string);
   if (length == 0 || string[length-1] == '\n')
      return string;
   newString = new char[length + 2];
   strcpy(newString, string);
   newString[length] = '\n';
   newString[length+1] = '\0';
   delete[] string;
   return newString;
}

// --------------------------------------------------------------------------
// Returns true if there are smart indent macros, or potential macros
// not yet committed in the smart indent dialog for a language mode,
// --------------------------------------------------------------------------
int LMHasSmartIndentMacros(const char* languageMode)
{
   if (findIndentSpec(languageMode) != NULL)
      return true;
   return SmartIndentDialog.shell!=NULL && !strcmp(SmartIndentDialog.langModeName,
      languageMode);
}

// --------------------------------------------------------------------------
// Change the language mode name of smart indent macro sets for language
// "oldName" to "newName" in both the stored macro sets, and the pattern set
// currently being edited in the dialog.
// --------------------------------------------------------------------------
void RenameSmartIndentMacros(const char* oldName, const char* newName)
{
   int i;

   for (i=0; i<NSmartIndentSpecs; i++)
   {
      if (!strcmp(oldName, SmartIndentSpecs[i]->lmName))
      {
         free__(SmartIndentSpecs[i]->lmName);
         SmartIndentSpecs[i]->lmName = NeNewString(newName);
      }
   }
   if (SmartIndentDialog.shell != NULL)
   {
      if (!strcmp(SmartIndentDialog.langModeName, oldName))
      {
         free__(SmartIndentDialog.langModeName);
         SmartIndentDialog.langModeName = NeNewString(newName);
      }
   }
}

// --------------------------------------------------------------------------
// If a smart indent dialog is up, ask to have the option menu for
// chosing language mode updated (via a call to CreateLanguageModeMenu)
// --------------------------------------------------------------------------
void UpdateLangModeMenuSmartIndent()
{

   if (SmartIndentDialog.shell == NULL)
      return;

   CreateLanguageModeChoice(SmartIndentDialog.lmOptMenu);
   SetLangModeMenu(SmartIndentDialog.lmOptMenu, SmartIndentDialog.langModeName);
}
