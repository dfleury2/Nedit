/*******************************************************************************
*									       *
* nedit.c -- Nirvana Editor main program				       *
*									       *
* Copyright (C) 1999 Mark Edel						       *
*									       *
* This is free__ software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version. In addition, you may distribute versions of this program linked to  *
* Motif or Open Motif. See README for details.                                 *
*                                                                              *
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
* Modifications:							       *
*									       *
*	8/18/93 - Mark Edel & Joy Kyriakopulos - Ported to VMS		       *
*									       *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "nedit.h"
#include "file.h"
#include "preferences.h"
#include "regularExp.h"
#include "selection.h"
#include "tags.h"
#include "menu.h"
#include "macro.h"
#include "server.h"
#include "interpret.h"
#include "parse.h"
#include "help.h"
#include "../util/misc.h"
#include "../util/printUtils.h"
#include "../util/fileUtils.h"
#include "../util/getfiles.h"
#include "../util/motif.h"
#include "../util/DialogF.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <sys/param.h>
#include <unistd.h>
#include <locale.h>
#endif // WIN32

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

#include <FL/fl_ask.H>
#include <FL/Fl_Window.H>

#include <iostream>

WindowInfo* WindowList = NULL;
char* ArgV0 = NULL;
bool IsServer = false;
Ne_AppContext AppContext(APP_NAME);

static const char cmdLineHelp[] =
   "Usage:  nedit [-read] [-create] [-line n | +n] [-server] [-do command]\n\
   [-tags file] [-tabs n] [-wrap] [-nowrap] [-autowrap]\n\
   [-autoindent] [-noautoindent] [-autosave] [-noautosave]\n\
   [-lm languagemode] [-rows n] [-columns n] [-font font]\n\
   [-geometry geometry] [-iconic] [-noiconic] [-svrname name]\n\
   [-display [host]:server[.screen] [-xrm resourcestring]\n\
   [-import file] [-background color] [-foreground color]\n\
   [-tabbed] [-untabbed] [-group] [-V|-version] [-h|-help]\n\
   [--] [file...]\n";


void nextArg(int argc, char** argv, int* argIndex)
{
   if (*argIndex + 1 >= argc)
   {
      fprintf(stderr, "NEdit: %s requires an argument\n%s", argv[*argIndex], cmdLineHelp);
      exit(EXIT_FAILURE);
   }
   (*argIndex)++;
}

// --------------------------------------------------------------------------
// Return true if -do macro is valid, otherwise write an error on stderr
// --------------------------------------------------------------------------
int checkDoMacroArg(const char* macro)
{
   Program* prog;
   char* errMsg, *stoppedAt, *tMacro;
   int macroLen;

   // Add a terminating newline (which command line users are likely to omit
   // since they are typically invoking a single routine)
   macroLen = strlen(macro);
   tMacro = (char*)malloc__(strlen(macro)+2);
   strncpy(tMacro, macro, macroLen);
   tMacro[macroLen] = '\n';
   tMacro[macroLen+1] = '\0';

   // Do a test parse
   prog = ParseMacro(tMacro, &errMsg, &stoppedAt);
   free__(tMacro);
   if (prog == NULL)
   {
      ParseError(NULL, tMacro, stoppedAt, "argument to -do", errMsg);
      return false;
   }
   FreeProgram(prog);
   return true;
}

int sortAlphabetical(const void* k1, const void* k2)
{
   const char* key1 = *(const char**)k1;
   const char* key2 = *(const char**)k2;
   return strcmp(key1, key2);
}
