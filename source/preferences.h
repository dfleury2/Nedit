/* $Id: preferences.h,v 1.57 2008/01/04 22:11:03 yooden Exp $ */
/*******************************************************************************
*                                                                              *
* preference.h -- Nirvana Editor Preferences Header File                       *
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

#ifndef NEDIT_PREFERENCES_H_INCLUDED
#define NEDIT_PREFERENCES_H_INCLUDED

#include "nedit.h"
#include "../util/Ne_Database.h"
#include "../util/Ne_Font.h"
#include "../util/Ne_MenuLevel.h"
#include "../util/Ne_MenuBar.h"

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Choice.H>

#define PLAIN_LANGUAGE_MODE -1

/* maximum number of language modes allowed */
#define MAX_LANGUAGE_MODES 127

#define MAX_TITLE_FORMAT_LEN 50

/* Identifiers for individual fonts in the help fonts list */
enum helpFonts {HELP_FONT, BOLD_HELP_FONT, ITALIC_HELP_FONT,
                BOLD_ITALIC_HELP_FONT, FIXED_HELP_FONT, BOLD_FIXED_HELP_FONT,
                ITALIC_FIXED_HELP_FONT, BOLD_ITALIC_FIXED_HELP_FONT, HELP_LINK_FONT,
                H1_HELP_FONT, H2_HELP_FONT, H3_HELP_FONT, NUM_HELP_FONTS
               };

Ne_Database CreateNEditPrefDB(int* argcInOut, char** argvInOut);
void RestoreNEditPrefs(Ne_Database* prefDB, Ne_Database* appDB);
void SaveNEditPrefs(Fl_Widget* parent, int quietly);
void ImportPrefFile(const char* filename, int convertOld);
void MarkPrefsChanged();
int CheckPrefsChangesSaved(Fl_Widget* dialogParent);
void SetPrefWrap(int state);
int GetPrefWrap(int langMode);
void SetPrefWrapMargin(int margin);
int GetPrefWrapMargin();
void SetPrefSearchDlogs(int state);
int GetPrefSearchDlogs();
void SetPrefKeepSearchDlogs(int state);
int GetPrefKeepSearchDlogs();
void SetPrefSearchWraps(int state);
int GetPrefSearchWraps();
void SetPrefStatsLine(int state);
int GetPrefStatsLine();
void SetPrefISearchLine(int state);
int GetPrefISearchLine();
void SetPrefTabBar(int state);
int GetPrefTabBar();
void SetPrefSortTabs(int state);
int GetPrefSortTabs();
void SetPrefTabBarHideOne(int state);
int GetPrefTabBarHideOne();
void SetPrefGlobalTabNavigate(int state);
int GetPrefGlobalTabNavigate();
void SetPrefToolTips(int state);
int GetPrefToolTips();
void SetPrefLineNums(int state);
int GetPrefLineNums();
void SetPrefShowPathInWindowsMenu(int state);
int GetPrefShowPathInWindowsMenu();
void SetPrefWarnFileMods(int state);
int GetPrefWarnFileMods();
void SetPrefWarnRealFileMods(int state);
int GetPrefWarnRealFileMods();
void SetPrefWarnExit(int state);
int GetPrefWarnExit();
void SetPrefSearch(int searchType);
int GetPrefSearch();
void SetPrefAutoIndent(int state);
int GetPrefAutoIndent(int langMode);
void SetPrefAutoSave(int state);
int GetPrefAutoSave();
void SetPrefSaveOldVersion(int state);
int GetPrefSaveOldVersion();
void SetPrefRows(int nRows);
int GetPrefRows();
void SetPrefCols(int nCols);
int GetPrefCols();
void SetPrefTabDist(int tabDist);
int GetPrefTabDist(int langMode);
void SetPrefEmTabDist(int tabDist);
int GetPrefEmTabDist(int langMode);
void SetPrefInsertTabs(int state);
int GetPrefInsertTabs();
void SetPrefShowMatching(int state);
int GetPrefShowMatching();
void SetPrefMatchSyntaxBased(int state);
int GetPrefMatchSyntaxBased();
void SetPrefHighlightSyntax(bool state);
bool GetPrefHighlightSyntax();
void SetPrefBacklightChars(int state);
int GetPrefBacklightChars();
void SetPrefBacklightCharTypes(char* types);
char* GetPrefBacklightCharTypes();
void SetPrefRepositionDialogs(int state);
int GetPrefRepositionDialogs();
void SetPrefAutoScroll(int state);
int GetPrefAutoScroll();
int GetVerticalAutoScroll();
void SetPrefAppendLF(int state);
int GetPrefAppendLF();
void SetPrefSortOpenPrevMenu(int state);
int GetPrefSortOpenPrevMenu();
char* GetPrefTagFile();
int GetPrefSmartTags();
void SetPrefSmartTags(int state);
int GetPrefAlwaysCheckRelTagsSpecs();
void SetPrefFont(char* fontName);
void SetPrefBoldFont(char* fontName);
void SetPrefItalicFont(char* fontName);
void SetPrefBoldItalicFont(char* fontName);
char* GetPrefFontName();
char* GetPrefBoldFontName();
char* GetPrefItalicFontName();
char* GetPrefBoldItalicFontName();
const Ne_Font& GetPrefFontList();
const Ne_Font& GetPrefBoldFont();
const Ne_Font& GetPrefItalicFont();
const Ne_Font& GetPrefBoldItalicFont();
char* GetPrefTooltipBgColor();
char* GetPrefHelpFontName(int index);
char* GetPrefHelpLinkColor();
char* GetPrefColorName(int colorIndex);
void SetPrefColorName(int colorIndex, const char* color);
void SetPrefShell(const char* shell);
const char* GetPrefShell();
char* GetPrefGeometry();
char* GetPrefServerName();
char* GetPrefBGMenuBtn();
void RowColumnPrefDialog(Fl_Widget* parent);
int GetPrefMapDelete();
int GetPrefStdOpenDialog();
char* GetPrefDelimiters();
int GetPrefMaxPrevOpenFiles();
int GetPrefTypingHidesPointer();
void EditLanguageModes();
void ChooseFonts(WindowInfo* window, int forWindow);
void ChooseColors(WindowInfo* window);
char* LanguageModeName(int mode);
char* GetWindowDelimiters(const WindowInfo* window);
int ReadNumericField(char** inPtr, int* value);
char* ReadSymbolicField(char** inPtr);
char* ReadSymbolicFieldTextWidget(Fl_Input* textW, const char* fieldName, int silent);
int ReadQuotedString(char** inPtr, char** errMsg, char** string);
char* MakeQuotedString(const char* string);
char* EscapeSensitiveChars(const char* string);
int SkipDelimiter(char** inPtr, char** errMsg);
int SkipOptSeparator(char separator, char** inPtr);
int ParseError(Fl_Widget* toDialog, const char* stringStart, const char* stoppedAt, const char* errorIn, const char* message);
int AllocatedStringsDiffer(const char* s1, const char* s2);
void SetLanguageMode(WindowInfo* window, int mode, int forceNewDefaults);
int FindLanguageMode(const char* languageName);
void UnloadLanguageModeTipsFile(WindowInfo* window);
void DetermineLanguageMode(WindowInfo* window, int forceNewDefaults);
Fl_Widget* CreateLanguageModeMenu(Fl_Widget* parent, Fl_Callback* cbProc); /*, void* cbArg);*/
void CreateLanguageModeChoice(Fl_Choice* parent);
void SetLangModeMenu(Fl_Choice* optMenu, const char* modeName);
void CreateLanguageModeSubMenu(WindowInfo* window, Ne_MenuBar* menuBar, const Ne_MenuLevel& level, const char* name, const char* label, const char mnemonic);
void SetPrefFindReplaceUsesSelection(int state);
int GetPrefFindReplaceUsesSelection();
int GetPrefStickyCaseSenseBtn();
void SetPrefBeepOnSearchWrap(int state);
int GetPrefBeepOnSearchWrap();
void SetPrefTitleFormat(const char* format);
const char* GetPrefTitleFormat();
int GetPrefOverrideVirtKeyBindings();
int GetPrefTruncSubstitution();
int GetPrefOpenInTab();
void SetPrefUndoModifiesSelection(bool);
void SetPrefOpenInTab(int state);
bool GetPrefUndoModifiesSelection();
bool GetPrefFocusOnRaise();
bool GetPrefHonorSymlinks();
bool GetPrefForceOSConversion();
void SetPrefFocusOnRaise(bool);

#endif /* NEDIT_PREFERENCES_H_INCLUDED */
