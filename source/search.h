/* $Id: search.h,v 1.28 2006/10/13 07:26:02 ajbj Exp $ */
/*******************************************************************************
*                                                                              *
* search.h -- Nirvana Editor Search Header File                                *
*                                                                              *
* Copyright 2003 The NEdit Developers                                          *
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

#ifndef NEDIT_SEARCH_H_INCLUDED
#define NEDIT_SEARCH_H_INCLUDED

#include "nedit.h"

enum SearchDirection {SEARCH_FORWARD, SEARCH_BACKWARD};

void CreateFindDlog(Fl_Widget* parent, WindowInfo* window);
void CreateReplaceDlog(Fl_Widget* parent, WindowInfo* window);
// TODO: void CreateReplaceMultiFileDlog(WindowInfo* window);
void DoFindReplaceDlog(WindowInfo* window, int direction, int keepDialogs, int searchType, double time);
void DoReplaceMultiFileDlog(WindowInfo* window);
void UpdateReplaceActionButtons(WindowInfo* window);
void DoFindDlog(WindowInfo* window, int direction, int keepDialogs, int searchType, int time);
int SearchAndSelect(WindowInfo* window, int direction, const char* searchString, int searchType, int searchWrap);
int SearchAndSelectSame(WindowInfo* window, int direction, int searchWrap);
// TODO: int SearchAndSelectIncremental(WindowInfo* window, int direction,
// TODO:                                const char* searchString, int searchType, int searchWrap, int continued);
void SearchForSelected(WindowInfo* window, int direction, int searchWrap, int searchType, int time);
// TODO: int SearchAndReplace(WindowInfo* window, int direction, const char* searchString,
// TODO:                      const char* replaceString, int searchType, int searchWrap);
// TODO: int ReplaceAndSearch(WindowInfo* window, int direction, const char* searchString,
// TODO:                      const char* replaceString, int searchType, int searchWrap);
// TODO: int ReplaceFindSame(WindowInfo* window, int direction, int searchWrap);
// TODO: int ReplaceSame(WindowInfo* window, int direction, int searchWrap);
int ReplaceAll(WindowInfo* window, const char* searchString, const char* replaceString, int searchType);
void ReplaceInSelection(const WindowInfo* window, const char* searchString, const char* replaceString, const int searchType);
int SearchWindow(WindowInfo* window, int direction, const char* searchString,
                 int searchType, int searchWrap, int beginPos, int* startPos, int* endPos,
                 int* extentBW, int* extentFW);
int SearchString(const char* string, const char* searchString, int direction,
                 int searchType, int wrap, int beginPos, int* startPos, int* endPos,
                 int* searchExtentBW, int* searchExtentFW, const char* delimiters);
char* ReplaceAllInString(const char* inString, const char* searchString,
                         const char* replaceString, int searchType, int* copyStart,
                         int* copyEnd, int* replacementLength, const char* delimiters);
void BeginISearch(WindowInfo* window, int direction);
// TODO: void EndISearch(WindowInfo* window);
// TODO: void SetISearchTextCallbacks(WindowInfo* window);
void FlashMatching(WindowInfo* window, Ne_Text_Editor* textW);
// TODO: void SelectToMatchingCharacter(WindowInfo* window);
void GotoMatchingCharacter(WindowInfo* window);
void RemoveFromMultiReplaceDialog(WindowInfo* window);
// TODO: bool WindowCanBeClosed(WindowInfo* window);

/*
** Schwarzenberg: added SEARCH_LITERAL_WORD .. SEARCH_REGEX_NOCASE
**
** The order of the integers in this enumeration must be exactly
** the same as the order of the coressponding strings of the
** array  SearchMethodStrings defined in preferences.c (!!)
**
*/
enum SearchType
{
   SEARCH_LITERAL, SEARCH_CASE_SENSE, SEARCH_REGEX,
   SEARCH_LITERAL_WORD, SEARCH_CASE_SENSE_WORD, SEARCH_REGEX_NOCASE,
   N_SEARCH_TYPES /* must be last in enum SearchType */
};

/*
** Parses a search type description string. If the string contains a valid
** search type description, returns TRUE and writes the corresponding
** SearchType in searchType. Returns FALSE and leaves searchType untouched
** otherwise.
*/
int StringToSearchType(const char* string, int* searchType);

/*
** History of search actions.
*/
extern int NHist;

#endif /* NEDIT_SEARCH_H_INCLUDED */
