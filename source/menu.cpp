/*******************************************************************************
*                                                                              *
* menu.c -- Nirvana Editor menus                                               *
*                                                                              *
* Copyright (C) 1999 Mark Edel                                                 *
*                                                                              *
* This is free__ software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version. In addition, you may distribute versions of this program linked to  *
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

#include "menu.h"
#include "Ne_Text_Buffer.h"
#include "Ne_Text_Editor.h"
#include "nedit.h"
#include "file.h"
#include "window.h"
#include "search.h"
#include "selection.h"
#include "undo.h"
#include "shift.h"
#include "help.h"
#include "preferences.h"
#include "WrapMarginDialog.h"
#include "TabsPrefDialog.h"
#include "SelectShellDialog.h"

#include "tags.h"
#include "userCmds.h"
#include "shell.h"
#include "macro.h"
#include "highlight.h"
#include "highlightData.h"
#include "interpret.h"
#include "smartIndent.h"
#include "windowTitle.h"
#include "../util/getfiles.h"
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "../util/fileUtils.h"
#include "../util/utils.h"
#include "../util/Ne_AppContext.h"
#include "../util/Ne_MenuLevel.h"

#include <FL/fl_ask.H>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef WIN32
#include <sys/param.h>
#include <unistd.h>
#endif // WIN32

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

typedef void (*menuCallbackProc)();

static void doActionCB(Fl_Widget* w, void* data);
// TODO: static void doTabActionCB(Fl_Widget* w, void* data);
static void pasteColCB(Fl_Widget* w, void* data);
static void shiftLeftCB(Fl_Widget* w, void* data);
static void shiftRightCB(Fl_Widget* w, void* data);
static void findCB(Fl_Widget* w, void* data);
static void findSameCB(Fl_Widget* w, void* data);
static void findSelCB(Fl_Widget* w, void* data);
static void findIncrCB(Fl_Widget* w, void* data);
static void replaceCB(Fl_Widget* w, void* data);
static void replaceSameCB(Fl_Widget* w, void* data);
static void replaceFindSameCB(Fl_Widget* w, void* data);
static void markCB(Fl_Widget* w, void* data);
static void gotoMarkCB(Fl_Widget* w, void* data);
static void gotoMatchingCB(Fl_Widget* w, void* data);
static void autoIndentOffCB(Fl_Widget* w, void* data);
static void autoIndentCB(Fl_Widget* w, void* data);
static void smartIndentCB(Fl_Widget* w, void* data);
static void preserveCB(Fl_Widget* w, void* data);
static void autoSaveCB(Fl_Widget* w, void* data);
static void newlineWrapCB(Fl_Widget* w, void* data);
static void noWrapCB(Fl_Widget* w, void* data);
static void continuousWrapCB(Fl_Widget* w, void* data);
static void wrapMarginCB(Fl_Widget* w, void* data);
static void fontCB(Fl_Widget* w, void* data);
static void tabsCB(Fl_Widget* w, void* data);
static void backlightCharsCB(Fl_Widget* w, void* data);
static void showMatchingOffCB(Fl_Widget* w, void* data);
static void showMatchingDelimitCB(Fl_Widget* w, void* data);
static void showMatchingRangeCB(Fl_Widget* w, void* data);
static void matchSyntaxBasedCB(Fl_Widget* w, void* data);
static void statsCB(Fl_Widget* w, void* data);
static void autoIndentOffDefCB(Fl_Widget* w, void* data);
static void autoIndentDefCB(Fl_Widget* w, void* data);
static void smartIndentDefCB(Fl_Widget* w, void* data);
static void autoSaveDefCB(Fl_Widget* w, void* data);
static void preserveDefCB(Fl_Widget* w, void* data);
static void noWrapDefCB(Fl_Widget* w, void* data);
static void newlineWrapDefCB(Fl_Widget* w, void* data);
static void contWrapDefCB(Fl_Widget* w, void* data);
static void wrapMarginDefCB(Fl_Widget* w, void* data);
static void shellSelDefCB(Fl_Widget* widget, void* data);
static void openInTabDefCB(Fl_Widget* w, void* data);
static void tabBarDefCB(Fl_Widget* w, void* data);
static void tabBarHideDefCB(Fl_Widget* w, void* data);
static void tabSortDefCB(Fl_Widget* w, void* data);
static void toolTipsDefCB(Fl_Widget* w, void* data);
static void tabNavigateDefCB(Fl_Widget* w, void* data);
static void statsLineDefCB(Fl_Widget* w, void* data);
static void iSearchLineDefCB(Fl_Widget* w, void* data);
static void lineNumsDefCB(Fl_Widget* w, void* data);
static void pathInWindowsMenuDefCB(Fl_Widget* w, void* data);
static void customizeTitleDefCB(Fl_Widget* w, void* data);
static void tabsDefCB(Fl_Widget* w, void* data);
static void showMatchingOffDefCB(Fl_Widget* w, void* data);
static void showMatchingDelimitDefCB(Fl_Widget* w, void* data);
static void showMatchingRangeDefCB(Fl_Widget* w, void* data);
static void matchSyntaxBasedDefCB(Fl_Widget* w, void* data);
static void highlightOffDefCB(Fl_Widget* w, void* data);
static void highlightDefCB(Fl_Widget* w, void* data);
static void backlightCharsDefCB(Fl_Widget* w, void* data);
static void fontDefCB(Fl_Widget* w, void* data);
static void colorDefCB(Fl_Widget* w, void* data);
static void smartTagsDefCB(Fl_Widget* parent, void* data);
static void showAllTagsDefCB(Fl_Widget* parent, void* data);
static void languageDefCB(Fl_Widget* w, void* data);
static void highlightingDefCB(Fl_Widget* w, void* data);
static void smartMacrosDefCB(Fl_Widget* w, void* data);
static void stylesDefCB(Fl_Widget* w, void* data);
static void shellDefCB(Fl_Widget* w, void* data);
static void macroDefCB(Fl_Widget* w, void* data);
static void bgMenuDefCB(Fl_Widget* w, void* data);
static void searchDlogsDefCB(Fl_Widget* w, void* data);
static void beepOnSearchWrapDefCB(Fl_Widget* w, void* data);
static void keepSearchDlogsDefCB(Fl_Widget* w, void* data);
static void searchWrapsDefCB(Fl_Widget* w, void* data);
static void appendLFCB(Fl_Widget* w, void* data);
static void sortOpenPrevDefCB(Fl_Widget* w, void* data);
static void reposDlogsDefCB(Fl_Widget* w, void* data);
static void autoScrollDefCB(Fl_Widget* w, void* data);
static void modWarnDefCB(Fl_Widget* w, void* data);
static void modWarnRealDefCB(Fl_Widget* w, void* data);
static void exitWarnDefCB(Fl_Widget* w, void* data);
static void searchLiteralCB(Fl_Widget* w, void* data);
static void searchCaseSenseCB(Fl_Widget* w, void* data);
static void searchLiteralWordCB(Fl_Widget* w, void* data);
static void searchCaseSenseWordCB(Fl_Widget* w, void* data);
static void searchRegexNoCaseCB(Fl_Widget* w, void* data);
static void searchRegexCB(Fl_Widget* w, void* data);
static void size24x80CB(Fl_Widget* w, void* data);
static void size40x80CB(Fl_Widget* w, void* data);
static void size60x80CB(Fl_Widget* w, void* data);
static void size80x80CB(Fl_Widget* w, void* data);
static void sizeCustomCB(Fl_Widget* w, void* data);
static void savePrefCB(Fl_Widget* w, void* data);
static void formFeedCB(Fl_Widget* w, void* data);
static void cancelShellCB(Fl_Widget* w, void* data);
static void learnCB(Fl_Widget* w, void* data);
static void finishLearnCB(Fl_Widget* w, void* data);
static void cancelLearnCB(Fl_Widget* w, void* data);
static void replayCB(Fl_Widget* w, void* data);
// TODO: static void windowMenuCB(Fl_Widget* w, void* data);
static void prevOpenMenuCB(Fl_Widget* w, void* data);
// TODO: static void unloadTagsFileMenuCB(Fl_Widget* w, void* data);
// TODO: static void unloadTipsFileMenuCB(Fl_Widget* w, void* data);
static void newAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void newOppositeAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void newTabAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void openDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void openAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void openSelectedAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void closeAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void saveAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void saveAsDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void saveAsAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void revertDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void revertAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void includeDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void includeAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void loadMacroDialogAP(Fl_Widget* w, int, const char** args, int* nArgs) ;
static void loadMacroAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void loadTagsDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void loadTagsAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void unloadTagsAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void loadTipsDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void loadTipsAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void unloadTipsAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void printAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void printSelAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void exitAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void undoAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void redoAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void clearAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void selAllAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void shiftLeftAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void shiftLeftTabAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void shiftRightAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void shiftRightTabAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void findDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void findAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void findSameAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void findSelAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void findIncrAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void startIncrFindAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void replaceDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void replaceAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void replaceAllAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void replaceInSelAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void replaceSameAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void replaceFindAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void replaceFindSameAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void gotoAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void gotoDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void gotoSelectedAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void repeatDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void repeatMacroAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void markAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void markDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void gotoMarkAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void gotoMarkDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void selectToMatchingAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void gotoMatchingAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void findDefAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void showTipAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void splitPaneAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void detachDocumentDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void detachDocumentAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void moveDocumentDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void nextDocumentAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void prevDocumentAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void lastDocumentAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void closePaneAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void capitalizeAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void lowercaseAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void fillAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void controlDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void filterDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void shellFilterAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void execDialogAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void execAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void execLineAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void shellMenuAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void macroMenuAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void bgMenuAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void beginningOfSelectionAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void endOfSelectionAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void updateWindowMenu(const WindowInfo* window);
static void updatePrevOpenMenu(WindowInfo* window);
// TODO: static void updateTagsFileMenu(WindowInfo* window);
// TODO: static void updateTipsFileMenu(WindowInfo* window);
static int searchDirection(int ignoreArgs, const char** args, int* nArgs);
static int searchWrap(int ignoreArgs, const char** args, int* nArgs);
static int searchKeepDialogs(int ignoreArgs, const char** args, int* nArgs);
static int searchType(int ignoreArgs, const char** args, int* nArgs);
static const char** shiftKeyToDir(bool isShiftPressed);
static void raiseCB(Fl_Widget* w, void* data);
static void openPrevCB(Fl_Widget* w, void* data);
// TODO: static void unloadTagsFileCB(Fl_Widget* w, char* name, caddr_t callData);
// TODO: static void unloadTipsFileCB(Fl_Widget* w, char* name, caddr_t callData);
static int cmpStrPtr(const void* strA, const void* strB);
// TODO: static void setWindowSizeDefault(int rows, int cols);
static void updateWindowSizeMenus();
static void updateWindowSizeMenu(WindowInfo* win);
static int strCaseCmp(const char* str1, const char* str2);
static int compareWindowNames(const void* windowA, const void* windowB);
static void bgMenuPostAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void tabMenuPostAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void raiseWindowAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void focusPaneAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setStatisticsLineAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setIncrementalSearchLineAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setShowLineNumbersAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setAutoIndentAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setWrapTextAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setWrapMarginAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setHighlightSyntaxAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setMakeBackupCopyAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setIncrementalBackupAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setShowMatchingAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setMatchSyntaxBasedAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setOvertypeModeAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setLockedAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setUseTabsAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setEmTabDistAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setTabDistAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setFontsAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void setLanguageModeAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static HelpMenu* buildHelpMenu(Fl_Widget* pane, HelpMenu* menu, WindowInfo* window);

// Application action table
static XtActionsRec Actions[] =
{
   {"new", newAP},
   {"new_opposite", newOppositeAP},
   {"new_tab", newTabAP},
   {"open", openAP},
   {"open-dialog", openDialogAP},
   {"open_dialog", openDialogAP},
   {"open-selected", openSelectedAP},
   {"open_selected", openSelectedAP},
   {"close", closeAP},
   {"save", saveAP},
   {"save-as", saveAsAP},
   {"save_as", saveAsAP},
   {"save-as-dialog", saveAsDialogAP},
   {"save_as_dialog", saveAsDialogAP},
   {"revert-to-saved", revertAP},
   {"revert_to_saved", revertAP},
   {"revert_to_saved_dialog", revertDialogAP},
   {"include-file", includeAP},
   {"include_file", includeAP},
   {"include-file-dialog", includeDialogAP},
   {"include_file_dialog", includeDialogAP},
   {"load-macro-file", loadMacroAP},
   {"load_macro_file", loadMacroAP},
   {"load-macro-file-dialog", loadMacroDialogAP},
   {"load_macro_file_dialog", loadMacroDialogAP},
   {"load-tags-file", loadTagsAP},
   {"load_tags_file", loadTagsAP},
   {"load-tags-file-dialog", loadTagsDialogAP},
   {"load_tags_file_dialog", loadTagsDialogAP},
   {"unload_tags_file", unloadTagsAP},
   {"load_tips_file", loadTipsAP},
   {"load_tips_file_dialog", loadTipsDialogAP},
   {"unload_tips_file", unloadTipsAP},
   {"print", printAP},
   {"print-selection", printSelAP},
   {"print_selection", printSelAP},
   {"exit", exitAP},
   {"undo", undoAP},
   {"redo", redoAP},
   {"delete", clearAP},
   {"select-all", selAllAP},
   {"select_all", selAllAP},
   {"shift-left", shiftLeftAP},
   {"shift_left", shiftLeftAP},
   {"shift-left-by-tab", shiftLeftTabAP},
   {"shift_left_by_tab", shiftLeftTabAP},
   {"shift-right", shiftRightAP},
   {"shift_right", shiftRightAP},
   {"shift-right-by-tab", shiftRightTabAP},
   {"shift_right_by_tab", shiftRightTabAP},
   {"find", findAP},
   {"find-dialog", findDialogAP},
   {"find_dialog", findDialogAP},
   {"find-again", findSameAP},
   {"find_again", findSameAP},
   {"find-selection", findSelAP},
   {"find_selection", findSelAP},
   {"find_incremental", findIncrAP},
   {"start_incremental_find", startIncrFindAP},
   {"replace", replaceAP},
   {"replace-dialog", replaceDialogAP},
   {"replace_dialog", replaceDialogAP},
   {"replace-all", replaceAllAP},
   {"replace_all", replaceAllAP},
   {"replace-in-selection", replaceInSelAP},
   {"replace_in_selection", replaceInSelAP},
   {"replace-again", replaceSameAP},
   {"replace_again", replaceSameAP},
   {"replace_find", replaceFindAP},
   {"replace_find_same", replaceFindSameAP},
   {"replace_find_again", replaceFindSameAP},
   {"goto-line-number", gotoAP},
   {"goto_line_number", gotoAP},
   {"goto-line-number-dialog", gotoDialogAP},
   {"goto_line_number_dialog", gotoDialogAP},
   {"goto-selected", gotoSelectedAP},
   {"goto_selected", gotoSelectedAP},
   {"mark", markAP},
   {"mark-dialog", markDialogAP},
   {"mark_dialog", markDialogAP},
   {"goto-mark", gotoMarkAP},
   {"goto_mark", gotoMarkAP},
   {"goto-mark-dialog", gotoMarkDialogAP},
   {"goto_mark_dialog", gotoMarkDialogAP},
   {"match", selectToMatchingAP},
   {"select_to_matching", selectToMatchingAP},
   {"goto_matching", gotoMatchingAP},
   {"find-definition", findDefAP},
   {"find_definition", findDefAP},
   {"show_tip", showTipAP},
   {"split-pane", splitPaneAP},
   {"split_pane", splitPaneAP},
   {"close-pane", closePaneAP},
   {"close_pane", closePaneAP},
   {"detach_document", detachDocumentAP},
   {"detach_document_dialog", detachDocumentDialogAP},
   {"move_document_dialog", moveDocumentDialogAP},
   {"next_document", nextDocumentAP},
   {"previous_document", prevDocumentAP},
   {"last_document", lastDocumentAP},
   {"uppercase", capitalizeAP},
   {"lowercase", lowercaseAP},
   {"fill-paragraph", fillAP},
   {"fill_paragraph", fillAP},
   {"control-code-dialog", controlDialogAP},
   {"control_code_dialog", controlDialogAP},
   {"filter-selection-dialog", filterDialogAP},
   {"filter_selection_dialog", filterDialogAP},
   {"filter-selection", shellFilterAP},
   {"filter_selection", shellFilterAP},
   {"execute-command", execAP},
   {"execute_command", execAP},
   {"execute-command-dialog", execDialogAP},
   {"execute_command_dialog", execDialogAP},
   {"execute-command-line", execLineAP},
   {"execute_command_line", execLineAP},
   {"shell-menu-command", shellMenuAP},
   {"shell_menu_command", shellMenuAP},
   {"macro-menu-command", macroMenuAP},
   {"macro_menu_command", macroMenuAP},
   {"bg_menu_command", bgMenuAP},
   {"post_window_bg_menu", bgMenuPostAP},
   {"post_tab_context_menu", tabMenuPostAP},
   {"beginning-of-selection", beginningOfSelectionAP},
   {"beginning_of_selection", beginningOfSelectionAP},
   {"end-of-selection", endOfSelectionAP},
   {"end_of_selection", endOfSelectionAP},
   {"repeat_macro", repeatMacroAP},
   {"repeat_dialog", repeatDialogAP},
   {"raise_window", raiseWindowAP},
   {"focus_pane", focusPaneAP},
   {"set_statistics_line", setStatisticsLineAP},
   {"set_incremental_search_line", setIncrementalSearchLineAP},
   {"set_show_line_numbers", setShowLineNumbersAP},
   {"set_auto_indent", setAutoIndentAP},
   {"set_wrap_text", setWrapTextAP},
   {"set_wrap_margin", setWrapMarginAP},
   {"set_highlight_syntax", setHighlightSyntaxAP},
   {"set_make_backup_copy", setMakeBackupCopyAP},
   {"set_incremental_backup", setIncrementalBackupAP},
   {"set_show_matching", setShowMatchingAP},
   {"set_match_syntax_based", setMatchSyntaxBasedAP},
   {"set_overtype_mode", setOvertypeModeAP},
   {"set_locked", setLockedAP},
   {"set_tab_dist", setTabDistAP},
   {"set_em_tab_dist", setEmTabDistAP},
   {"set_use_tabs", setUseTabsAP},
   {"set_fonts", setFontsAP},
   {"set_language_mode", setLanguageModeAP}
};

/* List of previously opened files for File menu */
static int NPrevOpen = 0;
static char** PrevOpen = NULL;

// Install actions for use in translation tables and macro recording, relating to menu item commands
void InstallMenuActions(Ne_AppContext& context)
{
   NeAppAddActions(context, Actions, ARRAY_SIZE(Actions));
}

// Return the (statically allocated) action table for menu item actions.
XtActionsRec* GetMenuActions(int* nActions)
{
   *nActions = ARRAY_SIZE(Actions);
   return Actions;
}

// --------------------------------------------------------------------------
// Create the menu bar
// --------------------------------------------------------------------------
Ne_MenuBar* CreateMenuBar(Fl_Widget* parent, WindowInfo* window)
{
   // cache user menus: allocate user menu cache
   window->userMenuCache = CreateUserMenuCache();

   // Create the menu bar (row column) widget
   Ne_MenuBar* menuBar = new Ne_MenuBar(0, 0, parent->w(), 22, &AppContext);

   // "File" pull down menu.
   Ne_MenuLevel level;
   menuBar->add_menu("fileMenu", "File", 0, NULL);
   level.push(menuBar->item_path("fileMenu"));

   menuBar->add("new", level + "New", 'N', doActionCB, "new");
   if (GetPrefOpenInTab())
      menuBar->add("newOpposite", level + "New Window", 'W', doActionCB, "new_opposite");
   else
      menuBar->add("newOpposite", level + "New Tab", 'T', doActionCB, "new_opposite");

   menuBar->add("open", level + "Open...", 'O', doActionCB, "open_dialog");
   menuBar->get_item("open")->shortcut(FL_CTRL + 'o');

   menuBar->add("openSelected", level + "Open Selected", 'd', doActionCB, "open_selected");
   if (GetPrefMaxPrevOpenFiles() > 0)
   {
      menuBar->add_menu("openPrevious", level + "Open Previous", 'v', NULL, 0, FL_MENU_DIVIDER);
      menuBar->set_sensitive("openPrevious", NPrevOpen != 0);
      menuBar->getOpenPreviousItem()->callback(prevOpenMenuCB, window);
   // TODO:       XtAddCallback(window->prevOpenMenuItem, XmNcascadingCallback, (XtCallbackProc)prevOpenMenuCB, window);
   }

   menuBar->add("close", level + "Close", 'C', doActionCB, "close");
   menuBar->add("save", level + "Save", 'S', doActionCB, "save");
   menuBar->add("saveAs", level + "Save As...", 'A', doActionCB, "save_as_dialog");
   menuBar->add("revertToSaved", level + "Revert to Saved", 'R', doActionCB, "revert_to_saved_dialog", FL_MENU_DIVIDER);
   menuBar->add("includeFile", level + "Include File...", 'I', doActionCB, "include_file_dialog");
   menuBar->add("loadMacroFile", level + "Load Macro File...", 'M', doActionCB, "load_macro_file_dialog");
   menuBar->add("loadTagsFile", level + "Load Tags File...", 'g', doActionCB, "load_tags_file_dialog");
   menuBar->add_menu("unloadTagsFiles", level + "Unload Tags File", 'U', NULL, 0);
   menuBar->set_sensitive("unloadTagsFiles", TagsFileList != NULL);
// TODO:    XtAddCallback(window->unloadTagsMenuItem, XmNcascadingCallback, (XtCallbackProc)unloadTagsFileMenuCB, window);
   menuBar->add("loadTipsFile", level + "Load Calltips File...", 'F', doActionCB, "load_tips_file_dialog");
   menuBar->add_menu("unloadTipsFiles", level + "Unload Calltips File", 'e', NULL, 0, FL_MENU_DIVIDER);
   menuBar->set_sensitive("unloadTipsFiles", TipsFileList != NULL);
// TODO:    XtAddCallback(window->unloadTipsMenuItem, XmNcascadingCallback, (XtCallbackProc)unloadTipsFileMenuCB, window);
   menuBar->add("print", level + "Print...", 'P', doActionCB, "print");
   menuBar->add("printSelection", level + "Print Selection...", 'l', doActionCB, "print_selection", FL_MENU_DIVIDER);
   menuBar->set_sensitive("printSelection", window->wasSelected);
   menuBar->add("exit", level + "Exit", 'x', doActionCB, "exit");
   level.pop();

   CheckCloseDim();

   // "Edit" pull down menu.
   menuBar->add_menu("editMenu", "Edit", 0, NULL);
   level.push(menuBar->item_path("editMenu"));
   menuBar->add("undo", level + "Undo", 'U', doActionCB, "undo");
   menuBar->set_sensitive("undo", false);
   menuBar->add("redo", level + "Redo", 'R', doActionCB, "redo", FL_MENU_DIVIDER);
   menuBar->set_sensitive("redo", false);
   menuBar->add("cut", level + "Cut", 't', doActionCB, "cut_clipboard");
   menuBar->set_sensitive("cut", window->wasSelected);
   menuBar->add("copy", level + "Copy", 'C', doActionCB, "copy_clipboard");
   menuBar->set_sensitive("copy", window->wasSelected);
   menuBar->add("paste", level + "Paste", 'P', doActionCB, "paste_clipboard");
   menuBar->add("pasteColumn", level + "Paste Column", 's', pasteColCB, window);
   menuBar->add("delete", level + "Delete", 'D', doActionCB, "delete_selection");
   menuBar->set_sensitive("delete", window->wasSelected);
   menuBar->add("selectAll", level + "Select All", 'A', doActionCB, "select_all", FL_MENU_DIVIDER);
   menuBar->add("shiftLeft", level + "Shift Left", 'L', shiftLeftCB, window);
   //createFakeMenuItem(menuPane, "shiftLeftShift", shiftLeftCB, window);
   menuBar->add("shiftRight", level + "Shift Right", 'g', shiftRightCB, window);
   //createFakeMenuItem(menuPane, "shiftRightShift", shiftRightCB, window);
   menuBar->add("lowerCase", level + "Lower-case", 'w', doActionCB, "lowercase");
   menuBar->add("upperCase", level + "Upper-case", 'e', doActionCB, "uppercase");
   menuBar->add("fillParagraph", level + "Fill Paragraph", 'F', doActionCB, "fill_paragraph", FL_MENU_DIVIDER);
   menuBar->add("insertFormFeed", level + "Insert Form Feed", 'I', formFeedCB, window);
   menuBar->add("insertCtrlCode", level + "Insert Ctrl Code...", 'n',doActionCB, "control_code_dialog");
   level.pop();

   // "Search" pull down menu.
   menuBar->add_menu("searchMenu", "Search", 0, NULL);
   level.push(menuBar->item_path("searchMenu"));

   menuBar->add("find", level + "Find...", 'F', findCB, window);
   menuBar->add("findShift", level + "Find Shift...", 0, findCB, window);
   menuBar->add("findAgain", level + "Find Again", 'i', findSameCB, window);
   menuBar->set_sensitive("findAgain", NHist);
   menuBar->add("findAgainShift", level + "Find Again Shift", 0, findSameCB, window);
   menuBar->add("findSelection", level + "Find Selection", 'S',findSelCB, window);
   menuBar->add("findSelectionShift", level + "Find Selection Shift", 0, findSelCB, window);
   menuBar->add("findIncremental", level + "Find Incremental", 'n', findIncrCB, window);
   menuBar->add("findIncrementalShift", level + "Find Incremental Shift", 0, findIncrCB, window);
   menuBar->add("replace", level + "Replace...", 'R', replaceCB, window);
// TODO:    createFakeMenuItem(menuPane, "replaceShift", replaceCB, window);
   menuBar->add("replaceFindAgain", level + "Replace Find Again", 'A', replaceFindSameCB, window);
   menuBar->set_sensitive("replaceFindAgain", NHist);
// TODO:    createFakeMenuItem(menuPane, "replaceFindAgainShift", replaceFindSameCB, window);
   menuBar->add("replaceAgain", level + "Replace Again", 'p', replaceSameCB, window, FL_MENU_DIVIDER);
   menuBar->set_sensitive("replaceAgain", NHist);
// TODO:    createFakeMenuItem(menuPane, "replaceAgainShift", replaceSameCB, window);
   menuBar->add("gotoLineNumber", level + "Goto Line Number...", 'L', doActionCB, "goto_line_number_dialog");
   menuBar->add("gotoSelected", level + "Goto Selected", 'G', doActionCB, "goto_selected", FL_MENU_DIVIDER);
   menuBar->add("mark", level + "Mark", 'k', markCB, window);
   menuBar->add("gotoMark", level + "Goto Mark", 'o', gotoMarkCB, window, FL_MENU_DIVIDER);
// TODO:    createFakeMenuItem(menuPane, "gotoMarkShift", gotoMarkCB, window);
   menuBar->add("gotoMatching", level + "Goto Matching (..)", 'M', gotoMatchingCB, window);
// TODO:    createFakeMenuItem(menuPane, "gotoMatchingShift", gotoMatchingCB, window);
   menuBar->add("findDefinition", level + "Find Definition", 'D', doActionCB, "find_definition");
   menuBar->set_sensitive("findDefinition", TagsFileList != NULL);
   menuBar->add("showCalltip", level + "Show Calltip", 'C', doActionCB, "show_tip");
   menuBar->set_sensitive("showCalltip", (TagsFileList != NULL || TipsFileList != NULL));
   level.pop();

   // "Preferences" menu, Default Settings sub menu
   menuBar->add_menu("preferencesMenu", "Preferences", 0, NULL); // Name needed for changing the label
   level.push(menuBar->item_path("preferencesMenu"));
   level.push("&Default Settings"); // No name needed for this submenu

   menuBar->add("languageModesDef", level + "Language Modes...", 'L', languageDefCB, window);

   // Auto Indent sub menu
   level.push("&Auto Indent");
   menuBar->add_radio("autoIndentOffDef", level + "Off", 'O', autoIndentOffDefCB, window, 0, GetPrefAutoIndent(PLAIN_LANGUAGE_MODE) == NO_AUTO_INDENT);
   menuBar->add_radio("autoIndentOnDef", level + "On", 'n', autoIndentDefCB, window, 0, GetPrefAutoIndent(PLAIN_LANGUAGE_MODE) == AUTO_INDENT);
   menuBar->add_radio("smartIndentDef", level + "Smart", 'S', smartIndentDefCB, window, FL_MENU_DIVIDER, GetPrefAutoIndent(PLAIN_LANGUAGE_MODE) == SMART_INDENT);
   menuBar->add("ProgramSmartIndent", level + "Program Smart Indent...", 'P', smartMacrosDefCB, window);
   level.pop();

   // Wrap sub menu
   level.push(Ne_MenuBar::ComputePath("Wrap", 'W'));
   menuBar->add_radio("noWrapDef", level + "None", 'N', noWrapDefCB, window, 0, GetPrefWrap(PLAIN_LANGUAGE_MODE) == NO_WRAP);
   menuBar->add_radio("newLineWrapDef", level + "Auto Newline", 'A', newlineWrapDefCB, window, 0, GetPrefWrap(PLAIN_LANGUAGE_MODE) == NEWLINE_WRAP);
   menuBar->add_radio("continousWrapDef", level + "Continuous", 'C', contWrapDefCB, window, FL_MENU_DIVIDER, GetPrefWrap(PLAIN_LANGUAGE_MODE) == CONTINUOUS_WRAP);
   menuBar->add("wrapMarginDef", level + "Wrap Margin...", 'W', wrapMarginDefCB, window);
   level.pop();

   // Smart Tags sub menu
   level.push(Ne_MenuBar::ComputePath("Tag Collisions", 'l'));
   menuBar->add_radio("showallDef", level + "Show All", 'A', showAllTagsDefCB, window, 0, !GetPrefSmartTags());
   menuBar->add_radio("smartDef", level + "Smart", 'S', smartTagsDefCB, window, 0, GetPrefSmartTags());
   level.pop();

   menuBar->add("shellSel", level + "Command Shell...", 'o', shellSelDefCB, window);
   menuBar->add("tabDistance", level + "Tab Stops...", 'T', tabsDefCB, window);
   menuBar->add("textFontDef", level + "Text Fonts...", 'F', fontDefCB, window);
   menuBar->add("colors", level + "Colors...", 'C', colorDefCB, window);

   // Customize Menus sub menu
   level.push(Ne_MenuBar::ComputePath("Customize Menus", 'u'));
   menuBar->add("shellMenuDef", level + "Shell Menu...", 'S', shellDefCB, window);
   menuBar->add("macroMenuDef", level + "Macro Menu...", 'M', macroDefCB, window);
   menuBar->add("windowBackgroundMenu", level + "Window Background Menu...", 'W', bgMenuDefCB, window, FL_MENU_DIVIDER);

   menuBar->add_toggle("sortOpenPrevMenu", level + "Sort Open Prev. Menu", 'o', sortOpenPrevDefCB, window, 0, GetPrefSortOpenPrevMenu());
   menuBar->add_toggle("pathInWindowsMenu", level + "Show Path In Windows Menu", 'P', pathInWindowsMenuDefCB, window, 0, GetPrefShowPathInWindowsMenu());
   level.pop();

   menuBar->add("custimizeTitle", level + "Customize Window Title...", 'd', customizeTitleDefCB, window);

   // Search sub menu
   level.push(Ne_MenuBar::ComputePath("Searching", 'g'));
   menuBar->add_toggle("verbose", level + "Verbose", 'V', searchDlogsDefCB, window, 0, GetPrefSearchDlogs());
   menuBar->add_toggle("wrapAround", level + "Wrap Around", 'W', searchWrapsDefCB, window, 0, GetPrefSearchWraps());
   menuBar->add_toggle("beepOnSearchWrap", level + "Beep On Search Wrap", 'B', beepOnSearchWrapDefCB, window, 0, GetPrefBeepOnSearchWrap());
   menuBar->add_toggle("keepDialogsUp", level + "Keep Dialogs Up", 'K', keepSearchDlogsDefCB, window, 0, GetPrefKeepSearchDlogs());

   level.push(Ne_MenuBar::ComputePath("Default Search Style", 'D'));
   menuBar->add_radio("literal", level + "Literal", 'L', searchLiteralCB, window, 0, GetPrefSearch() == SEARCH_LITERAL);
   menuBar->add_radio("caseSensitive", level + "Literal, Case Sensitive", 'C', searchCaseSenseCB, window, 0, GetPrefSearch() == SEARCH_CASE_SENSE);
   menuBar->add_radio("literalWord", level + "Literal, Whole Word", 'W', searchLiteralWordCB, window, 0, GetPrefSearch() == SEARCH_LITERAL_WORD);
   menuBar->add_radio("caseSensitiveWord", level + "Literal, Case Sensitive, Whole Word", 't', searchCaseSenseWordCB, window, 0, GetPrefSearch() == SEARCH_CASE_SENSE_WORD);
   menuBar->add_radio("regularExpression", level + "Regular Expression", 'R', searchRegexCB, window, 0, GetPrefSearch() == SEARCH_REGEX);
   menuBar->add_radio("regularExpressionNoCase", level + "Regular Expression, Case Insensitive", 'I', searchRegexNoCaseCB, window, 0, GetPrefSearch() == SEARCH_REGEX_NOCASE);
   level.pop();
   level.pop();

   // Syntax Highlighting sub menu
   level.push(Ne_MenuBar::ComputePath("Syntax Highlighting", 'H'));
   menuBar->add_radio("highlightOffDef", level + "Off", 'O', highlightOffDefCB, window, 0, !GetPrefHighlightSyntax());
   menuBar->add_radio("highlightDef", level + "On", 'n', highlightDefCB, window, FL_MENU_DIVIDER, GetPrefHighlightSyntax());
   menuBar->add("recognitionPatterns", level + "Recognition Patterns...", 'R', highlightingDefCB, window);
   menuBar->add("textDrawingStyles", level + "Text Drawing Styles...", 'T', stylesDefCB, window);
   level.pop();

   menuBar->add_toggle("backlightCharsDef", level + "Apply Backlighting", 'g', backlightCharsDefCB, window, 0, GetPrefBacklightChars());

   // tabbed editing sub menu
   level.push(Ne_MenuBar::ComputePath("Tabbed Editing", 0));
   menuBar->add_toggle("openAsTab", level + "Open File In New Tab", 'T', openInTabDefCB, window, 0, GetPrefOpenInTab());
   menuBar->add_toggle("showTabBar", level + "Show Tab Bar", 'B', tabBarDefCB, window, 0, GetPrefTabBar());
   menuBar->add_toggle("hideTabBar", level + "Hide Tab Bar When Only One Document is Open", 'H', tabBarHideDefCB, window, 0, GetPrefTabBarHideOne());
   menuBar->add_toggle("tabNavigateDef", level + "Next\\/Prev Tabs Across Windows", 'W', tabNavigateDefCB, window, 0, GetPrefGlobalTabNavigate());
   menuBar->add_toggle("tabSortDef", level + "Sort Tabs Alphabetically", 'S', tabSortDefCB, window, 0, GetPrefSortTabs());
   level.pop();

   menuBar->add_toggle("showTooltipsDef", level + "Show Tooltips", 0, toolTipsDefCB, window, 0, GetPrefToolTips());
   menuBar->add_toggle("statisticsLineDef", level + "Statistics Line", 'S', statsLineDefCB, window, 0, GetPrefStatsLine());
   menuBar->add_toggle("incrementalSearchLineDef", level + "Incremental Search Line", 'i', iSearchLineDefCB, window, 0, GetPrefISearchLine());
   menuBar->add_toggle("showLineNumbersDef", level + "Show Line Numbers", 'N', lineNumsDefCB, window, 0, GetPrefLineNums());
   menuBar->add_toggle("preserveLastVersionDef", level + "Make Backup Copy (*.bck)", 'e', preserveDefCB, window, 0, GetPrefSaveOldVersion());
   menuBar->add_toggle("incrementalBackupDef", level + "Incremental Backup", 'B', autoSaveDefCB, window, 0, GetPrefAutoSave());

   // Show Matching sub menu
   level.push(Ne_MenuBar::ComputePath("Show Matching (..)", 'M'));
   menuBar->add_radio("showMatchingOffDef", level + "Off", 'O', showMatchingOffDefCB, window, 0, GetPrefShowMatching() == NO_FLASH);
   menuBar->add_radio("showMatchingDelimitDef", level + "Delimiter", 'D', showMatchingDelimitDefCB, window, 0, GetPrefShowMatching() == FLASH_DELIMIT);
   menuBar->add_radio("showMatchingRangeDef", level + "Range", 'R', showMatchingRangeDefCB, window, FL_MENU_DIVIDER, GetPrefShowMatching() == FLASH_RANGE);

   menuBar->add_toggle("matchSyntaxBasedDef", level + "Syntax Based", 'S', matchSyntaxBasedDefCB, window, 0, GetPrefMatchSyntaxBased());
   level.pop();

   // Append LF at end of files on save
   menuBar->add_toggle("appendLFItem", level + "Terminate with Line Break on Save", 'v', appendLFCB, NULL, 0, GetPrefAppendLF());

   menuBar->add_toggle("popupsUnderPointer", level + "Popups Under Pointer", 'P', reposDlogsDefCB, window, 0, GetPrefRepositionDialogs());
   menuBar->add_toggle("autoScroll", level + "Auto Scroll Near Window Top/Bottom", 0, autoScrollDefCB, window, 0, GetPrefAutoScroll());

   level.push(Ne_MenuBar::ComputePath("Warnings", 'r'));
   menuBar->add_toggle("filesModifiedExternally", level + "Files Modified Externally", 'F', modWarnDefCB, window, 0, GetPrefWarnFileMods());
   menuBar->add_toggle("checkModifiedFileContents", level + "Check Modified File Contents", 'C', modWarnRealDefCB, window, 0, GetPrefWarnRealFileMods());
   menuBar->set_sensitive("checkModifiedFileContents", GetPrefWarnFileMods());
   menuBar->add_toggle("onExit", level + "On Exit", 'O', exitWarnDefCB, window, 0, GetPrefWarnExit());
   level.pop();

   // Initial Window Size sub menu (simulates radioBehavior)
   level.push(Ne_MenuBar::ComputePath("Initial Window Size", 'z'));
   menuBar->add_radio("24X80", level + "24 x 80", '2', size24x80CB, window, false);
   menuBar->add_radio("40X80", level + "40 x 80", '4', size40x80CB, window, false);
   menuBar->add_radio("60X80", level + "60 x 80", '6', size60x80CB, window, false);
   menuBar->add_radio("80X80", level + "80 x 80", '8', size80x80CB, window, false);
   menuBar->add_radio("customSize", level + "Custom...", 'C', sizeCustomCB, window, false);
   window->menuBar = menuBar;
   updateWindowSizeMenu(window);
   level.pop();
   level.pop();

   // Remainder of Preferences menu
   menuBar->add("saveDefaults", level + "Save Defaults...", 'v', savePrefCB, window, FL_MENU_DIVIDER);

   menuBar->add_toggle("statisticsLine", level + "Statistics Line", 'S', statsCB, window, 0, GetPrefStatsLine());
   menuBar->add_toggle("incrementalSearchLine", level + "Incremental Search Line", 'I', doActionCB, "set_incremental_search_line", 0, GetPrefISearchLine());
   menuBar->add_toggle("lineNumbers", level + "Show Line Numbers", 'N', doActionCB, "set_show_line_numbers", 0, GetPrefLineNums());
   
   CreateLanguageModeSubMenu(window, menuBar, level, "languageMode", "Language Mode", 'L');

   level.push(Ne_MenuBar::ComputePath("Auto Indent", 'A'));
   menuBar->add_radio("autoIndentOff", level + "Off", 'O', autoIndentOffCB, window, 0, window->indentStyle == NO_AUTO_INDENT);
   menuBar->add_radio("autoIndent", level + "On", 'n', autoIndentCB, window, 0, window->indentStyle == AUTO_INDENT);
   menuBar->add_radio("smarIndent", level + "Smart", 'S', smartIndentCB, window, 0, window->indentStyle == SMART_INDENT);
   level.pop();
   level.push(Ne_MenuBar::ComputePath("Wrap", 'W'));
   menuBar->add_radio("noWrap", level + "None", 'N', noWrapCB, window, 0, window->wrapMode==NO_WRAP);
   menuBar->add_radio("newlineWrap", level + "Auto Newline", 'A', newlineWrapCB, window, 0, window->wrapMode==NEWLINE_WRAP);
   menuBar->add_radio("continuousWrap", level + "Continuous", 'C', continuousWrapCB, window, FL_MENU_DIVIDER, window->wrapMode==CONTINUOUS_WRAP);
   menuBar->add("wrapMargin", level + "Wrap Margin...", 'W', wrapMarginCB, window);
   level.pop();
   menuBar->add("tabs", level + "Tab Stops...", 'T', tabsCB, window);
   menuBar->add("textFont", level + "Text Fonts...", 'F', fontCB, window);
   menuBar->add_toggle("highlightSyntax", level + "Highlight Syntax", 'H', doActionCB, "set_highlight_syntax", 0, GetPrefHighlightSyntax());
   menuBar->add_toggle("backlightChars", level + "Apply Backlighting", 'g', backlightCharsCB, window, 0, window->backlightChars);
   menuBar->add_toggle("makeBackupCopy", level + "Make Backup Copy (*.bck)", 'e', preserveCB, window, 0, window->saveOldVersion);
   menuBar->add_toggle("incrementalBackup", level + "Incremental Backup", 'B', autoSaveCB, window, 0, window->autoSave);

   level.push(Ne_MenuBar::ComputePath("Show Matching (..)", 'M'));
   menuBar->add_radio("showMatchingOff", level + "Off", 'O', showMatchingOffCB, window, 0, window->showMatchingStyle == NO_FLASH);
   menuBar->add_radio("showMatchingDelimiter", level + "Delimiter", 'D', showMatchingDelimitCB, window, 0, window->showMatchingStyle == FLASH_DELIMIT);
   menuBar->add_radio("showMatchingRange", level + "Range", 'R', showMatchingRangeCB, window, FL_MENU_DIVIDER, window->showMatchingStyle == FLASH_RANGE);
   menuBar->add_toggle("matchSyntax", level + "Syntax Based", 'S', matchSyntaxBasedCB, window, 0, window->matchSyntaxBased);
   level.pop();
   level.pop();

   // Create the Shell menu
   menuBar->add_menu("shellMenu", "Shell", 0, NULL); // Name needed for changing the label
   level.push(menuBar->item_path("shellMenu"));

   menuBar->add("executeCommand", level + "Execute Command...", 'E', doActionCB, "execute_command_dialog");
   menuBar->add("executeCommandLine", level + "Execute Command Line", 'x', doActionCB, "execute_command_line");
   menuBar->add("filterSelection", level + "Filter Selection...", 'F', doActionCB, "filter_selection_dialog");
   menuBar->add("cancelShellCommand", level + "Cancel Shell Command", 'C', cancelShellCB, window, FL_MENU_DIVIDER);
   level.pop();

   // Create the Macro menu
   menuBar->add_menu("macroMenu", "Macro", 0, NULL); // Name needed for changing the label
   level.push(menuBar->item_path("macroMenu"));
   menuBar->add("learnKeystrokes", level + "Learn Keystrokes", 'L', learnCB, window);
   menuBar->add("finishLearn", level + "Finish Learn", 'F', finishLearnCB, window);
   menuBar->add("cancelLearn", level + "Cancel Learn", 'C', cancelLearnCB, window);
   menuBar->add("replayKeystrokes", level + "Replay Keystrokes", 'K', replayCB, window);
   menuBar->add("repeat", level + "Repeat...", 'R', doActionCB, "repeat_dialog", FL_MENU_DIVIDER);
   level.pop();

   // Create the Windows menu
   menuBar->add_menu("windowsMenu", "Windows", 0, NULL); // Name needed for changing the label
   level.push(menuBar->item_path("windowsMenu"));
// TODO:    XtAddCallback(cascade, XmNcascadingCallback, (XtCallbackProc)windowMenuCB, window);
   menuBar->add("splitPane", level + "Split Pane", 'S', doActionCB, "split_pane");
   menuBar->add("closePane", level + "Close Pane", 'C', doActionCB, "close_pane", FL_MENU_DIVIDER);
   menuBar->set_sensitive("closePane", false);

   menuBar->add("detachBuffer", level + "Detach Tab", 'D', doActionCB, "detach_document");
   menuBar->set_sensitive("detachBuffer", false);
   menuBar->add("moveDocument", level + "Move Tab To...", 'M', doActionCB, "move_document_dialog", FL_MENU_DIVIDER);
   menuBar->set_sensitive("moveDocument", false);
   menuBar->add("windowNameStart", level + "WindowNameStart", 0, NULL, 0, FL_MENU_INVISIBLE);
   menuBar->add("windowNameEnd", level + "WindowNameEnd", 0, NULL, 0, FL_MENU_INVISIBLE);
   level.pop();

   // Create "Help" pull down menu.
   menuBar->add_menu("helpMenu", "Help", 0, NULL); // Name needed for changing the label
   level.push(menuBar->item_path("helpMenu"));
// TODO:    buildHelpMenu(menuPane, &H_M[0], window);
   level.pop();

// TODO:    // Looking for mnemonic - if root item change, all sub items had to change in the Ne_MenuBar name to path map
// TODO:    for(Ne_MenuBar::const_iterator item = menuBar->begin(); item != menuBar->end(); ++item)
// TODO:    {
// TODO:       Ne_Database::const_iterator found = AppContext.appDB.find( "*" + item->first + ".mnemonic");
// TODO:       if (found != AppContext.appDB.end())
// TODO:       {
// TODO:          std::cout << item->first << "=" << found->second << std::endl;
// TODO:          if (found->second.size()==1)
// TODO:             menuBar->replace(item->first, Ne_MenuBar::ComputePath(item->second, found->second[0]));
// TODO:       }
// TODO:    }

// TODO:    // Looking for shortcuts
// TODO:    for(Ne_MenuBar::const_iterator item = menuBar->begin(); item != menuBar->end(); ++item)
// TODO:    {
// TODO:       Ne_Database::const_iterator found = AppContext.appDB.find( "*" + item->first + ".accelerator"); // incorect... no hierarchy use
// TODO:       if (found != AppContext.appDB.end())
// TODO:       {
// TODO:          std::cout << item->first << "=" << found->second << std::endl;
// TODO:          if (found->second.size()==1)
// TODO:             menuBar->get_item(item->first)->shortcut(FL_CTRL + 'a'); // hardest part
// TODO:       }
// TODO:    }
   return menuBar;
}

// TODO: /*----------------------------------------------------------------------------*/
// TODO: 
// TODO: static Fl_Widget* makeHelpMenuItem(
// TODO: 
// TODO:    Fl_Widget*           parent,
// TODO:    char*            name,      /* to be assigned to the child widget */
// TODO:    char*            label,     /* text to be displayed in menu       */
// TODO:    char             mnemonic,  /* letter in label to be underlined   */
// TODO:    menuCallbackProc callback,  /* activated when menu item selected  */
// TODO:    void*            cbArg,     /* passed to activated call back      */
// TODO:    int              mode,      /* SGI_CUSTOM menu option             */
// TODO:    enum HelpTopic   topic      /* associated with this menu item     */
// TODO: )
// TODO: {
// TODO:    Fl_Widget* menuItem =
// TODO:       createMenuItem(parent, name, label, mnemonic, callback, cbArg, mode);
// TODO: 
// TODO:    XtVaSetValues(menuItem, XmNuserData, (XtPointer)topic, NULL);
// TODO:    return menuItem;
// TODO: }
// TODO: 
// TODO: /*----------------------------------------------------------------------------*/
// TODO: 
// TODO: static void helpCB(Fl_Widget* menuItem, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    enum HelpTopic topic;
// TODO:    XtPointer userData;
// TODO: 
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(MENU_WIDGET(menuItem))->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    XtVaGetValues(menuItem, XmNuserData, &userData, NULL);
// TODO:    topic = (enum HelpTopic)(int)userData;
// TODO: 
// TODO:    Help(topic);
// TODO: }
// TODO: 
// TODO: /*----------------------------------------------------------------------------*/
// TODO: 
// TODO: #define NON_MENU_HELP 9
// TODO: 
// TODO: static HelpMenu* buildHelpMenu(
// TODO: 
// TODO:    Fl_Widget*       pane,  /* Menu pane on which to place new menu items */
// TODO:    HelpMenu*    menu,  /* Data to drive building the help menu       */
// TODO:    WindowInfo* window  /* Main NEdit window information              */
// TODO: )
// TODO: {
// TODO: #ifdef VMS
// TODO:    int hideIt = 1;  /* All menu items matching this will be inaccessible */
// TODO: #else
// TODO:    int hideIt = -1; /* This value should make all menu items accessible  */
// TODO: #endif
// TODO: 
// TODO:    if (menu != NULL)
// TODO:    {
// TODO:       int crntLevel = menu->level;
// TODO: 
// TODO:       /*-------------------------
// TODO:       * For each menu element ...
// TODO:       *-------------------------*/
// TODO:       while (menu != NULL && menu->level == crntLevel)
// TODO:       {
// TODO:          /*----------------------------------------------
// TODO:          * ... see if dealing with a separator or submenu
// TODO:          *----------------------------------------------*/
// TODO:          if (menu->topic == HELP_none)
// TODO:          {
// TODO:             if (menu->mnemonic == '-')
// TODO:             {
// TODO:                createMenuSeparator(pane, menu->wgtName);
// TODO:                menu = menu->next;
// TODO:             }
// TODO:             else
// TODO:             {
// TODO:                /*-------------------------------------------------------
// TODO:                * Do not show any of the submenu when it is to be hidden.
// TODO:                *-------------------------------------------------------*/
// TODO:                if (menu->hideIt == hideIt || menu->hideIt == NON_MENU_HELP)
// TODO:                {
// TODO:                   do
// TODO:                   {
// TODO:                      menu = menu->next;
// TODO:                   }
// TODO:                   while (menu != NULL && menu->level > crntLevel);
// TODO: 
// TODO:                }
// TODO:                else
// TODO:                {
// TODO:                   Fl_Widget* subPane =
// TODO:                      createMenu(pane, menu->wgtName, menu->subTitle,
// TODO:                                 menu->mnemonic, NULL);
// TODO: 
// TODO:                   menu = buildHelpMenu(subPane, menu->next, window);
// TODO:                }
// TODO:             }
// TODO:          }
// TODO: 
// TODO:          else
// TODO:          {
// TODO:             /*---------------------------------------
// TODO:             * Show menu item if not going to hide it.
// TODO:             * This is the easy way out of hiding
// TODO:             * menu items. When entire submenus want
// TODO:             * to be hidden, either the entire branch
// TODO:             * will have to be marked, or this algorithm
// TODO:             * will have to become a lot smarter.
// TODO:             *---------------------------------------*/
// TODO:             if (menu->hideIt != hideIt && menu->hideIt != NON_MENU_HELP)
// TODO:                makeHelpMenuItem(
// TODO:                   pane, menu->wgtName, HelpTitles[menu->topic],
// TODO:                   menu->mnemonic, helpCB, window, SHORT, menu->topic);
// TODO: 
// TODO:             menu = menu->next;
// TODO:          }
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    return menu;
// TODO: }
// TODO: 
// TODO: /*----------------------------------------------------------------------------*/
// TODO: 
// TODO: /*
// TODO: ** handle actions called from the context menus of tabs.
// TODO: */
// TODO: static void doTabActionCB(Fl_Widget* w, void* data)
// TODO: {
// TODO:    Fl_Widget* menu = w;
// TODO:    WindowInfo* win, *window = WidgetToWindow(menu);
// TODO: 
// TODO:    /* extract the window to be acted upon, see comment in
// TODO:       tabMenuPostAP() for detail */
// TODO:    XtVaGetValues(window->tabMenuPane, XmNuserData, &win, NULL);
// TODO: 
// TODO:    HidePointerOnKeyedEvent(win->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    XtCallActionProc(win->lastFocus, (char*)clientData,
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, NULL, 0);
// TODO: }

static void doActionCB(Fl_Widget* w, void* data)
{
   printf("DoActionCB [%s]\n", (const char*)data);

   const char* action = (const char*)data;

   AppContext.callAction(w, action, Fl::event(), NULL, 0);
}

static void pasteColCB(Fl_Widget* w, void* data)
{
   TRACE();

   static char* params[1] = {"rect"};

// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    XtCallActionProc(WidgetToWindow(w)->lastFocus, "paste_clipboard",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, params, 1);
}

static void shiftLeftCB(Fl_Widget* w, void* data)
{
   TRACE();

   AppContext.callAction(WidgetToWindow(w)->lastFocus,
                    Fl::event_shift() ? "shift_left_by_tab" : "shift_left",
                    Fl::event(), NULL, 0);
   WidgetToMainWindow(w)->redraw();
}

static void shiftRightCB(Fl_Widget* w, void* data)
{
   TRACE();

   AppContext.callAction(WidgetToWindow(w)->lastFocus,
                    Fl::event_shift() ? "shift_right_by_tab" : "shift_right",
                    Fl::event(), NULL, 0);
   WidgetToMainWindow(w)->redraw();
}

static void findCB(Fl_Widget* w, void* data)
{
   TRACE();

   AppContext.callAction(WidgetToWindow(w)->lastFocus, "find_dialog",
      Fl::event(), shiftKeyToDir(Fl::event_shift()), 1);
}

static void findSameCB(Fl_Widget* w, void* data)
{
   TRACE();

   AppContext.callAction(WidgetToWindow(w)->lastFocus, "find_again",
      Fl::event(), shiftKeyToDir(Fl::event_shift()), 1);
}

static void findSelCB(Fl_Widget* w, void* data)
{
   TRACE();

   AppContext.callAction(WidgetToWindow(w)->lastFocus, "find_selection",
      Fl::event(), shiftKeyToDir(Fl::event_shift()), 1);
}

static void findIncrCB(Fl_Widget* w, void* data)
{
   TRACE();

   AppContext.callAction(WidgetToWindow(w)->lastFocus, "start_incremental_find", 
      Fl::event(), shiftKeyToDir(Fl::event_shift()), 1);
}

static void replaceCB(Fl_Widget* w, void* data)
{
   TRACE();

   AppContext.callAction(WidgetToWindow(w)->lastFocus, "replace_dialog", 
      Fl::event(), shiftKeyToDir(Fl::event_shift()), 1);
}

static void replaceSameCB(Fl_Widget* w, void* data)
{
   TRACE();

// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    XtCallActionProc(WidgetToWindow(w)->lastFocus, "replace_again",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event,
// TODO:                     shiftKeyToDir(callData), 1);
}

static void replaceFindSameCB(Fl_Widget* w, void* data)
{
   TRACE();

   AppContext.callAction(WidgetToWindow(w)->lastFocus, "replace_find_same", 
      Fl::event(), shiftKeyToDir(Fl::event_shift()), 1);
}

static void markCB(Fl_Widget* w, void* data)
{
   TRACE();
   int event = Fl::event();
   WindowInfo* window = WidgetToWindow(w);

   if (event == FL_KEYDOWN) // || event == KeyRelease)
      BeginMarkCommand(window);
   else
      AppContext.callAction(window->lastFocus, "mark_dialog", event, NULL, 0);
}

static void gotoMarkCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    XEvent* event = ((XmAnyCallbackStruct*)callData)->event;
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    int extend = event->xbutton.state & ShiftMask;
// TODO:    static char* params[1] = {"extend"};
// TODO: 
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    if (event->type == KeyPress || event->type == KeyRelease)
// TODO:       BeginGotoMarkCommand(window, extend);
// TODO:    else
// TODO:       XtCallActionProc(window->lastFocus, "goto_mark_dialog", event, params,
// TODO:                        extend ? 1 : 0);
}

static void gotoMatchingCB(Fl_Widget* w, void* data)
{
   TRACE();
   bool isShift = false; // TODO: ((XmAnyCallbackStruct*)callData)->event->xbutton.state & ShiftMask;

   AppContext.callAction(WidgetToWindow(w)->lastFocus,
      isShift ? "select_to_matching" : "goto_matching",
      Fl::event(), NULL, 0);
}

static void autoIndentOffCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    static char* params[1] = {"off"};
// TODO:    Fl_Widget* menu = w;
// TODO: 
// TODO:    window = WidgetToWindow(menu);
// TODO: 
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(menu)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    XtCallActionProc(WidgetToWindow(menu)->lastFocus, "set_auto_indent",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, params, 1);
}

static void autoIndentCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    static char* params[1] = {"on"};
// TODO:    Fl_Widget* menu = w;
// TODO: 
// TODO:    window = WidgetToWindow(menu);
// TODO: 
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(menu)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    XtCallActionProc(WidgetToWindow(menu)->lastFocus, "set_auto_indent",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, params, 1);
}

static void smartIndentCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    static char* params[1] = {"smart"};
// TODO:    Fl_Widget* menu = w;
// TODO: 
// TODO:    window = WidgetToWindow(menu);
// TODO: 
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(menu)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    XtCallActionProc(WidgetToWindow(menu)->lastFocus, "set_auto_indent",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, params, 1);
}

static void autoSaveCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    Fl_Widget* menu = w;
// TODO: 
// TODO:    window = WidgetToWindow(menu);
// TODO: 
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(menu)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    XtCallActionProc(WidgetToWindow(menu)->lastFocus, "set_incremental_backup",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, NULL, 0);
}

static void preserveCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    Fl_Widget* menu = w;
// TODO: 
// TODO:    window = WidgetToWindow(menu);
// TODO: 
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(menu)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    XtCallActionProc(WidgetToWindow(menu)->lastFocus, "set_make_backup_copy",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, NULL, 0);
}

static void showMatchingOffCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    static char* params[1] = {NO_FLASH_STRING};
// TODO:    Fl_Widget* menu = w;
// TODO: 
// TODO:    window = WidgetToWindow(menu);
// TODO: 
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(menu)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    XtCallActionProc(WidgetToWindow(menu)->lastFocus, "set_show_matching",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, params, 1);
}

static void showMatchingDelimitCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    static char* params[1] = {FLASH_DELIMIT_STRING};
// TODO:    Fl_Widget* menu = w;
// TODO: 
// TODO:    window = WidgetToWindow(menu);
// TODO: 
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(menu)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    XtCallActionProc(WidgetToWindow(menu)->lastFocus, "set_show_matching",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, params, 1);
}

static void showMatchingRangeCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    static char* params[1] = {FLASH_RANGE_STRING};
// TODO:    Fl_Widget* menu = w;
// TODO: 
// TODO:    window = WidgetToWindow(menu);
// TODO: 
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(menu)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    XtCallActionProc(WidgetToWindow(menu)->lastFocus, "set_show_matching",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, params, 1);
}

static void matchSyntaxBasedCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    Fl_Widget* menu = w;
// TODO: 
// TODO:    window = WidgetToWindow(menu);
// TODO: 
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(menu)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    XtCallActionProc(WidgetToWindow(menu)->lastFocus, "set_match_syntax_based",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, NULL, 0);
}

static void fontCB(Fl_Widget* w, void* data)
{
   TRACE();
   ChooseFonts(WidgetToWindow(w), true);
}

static void noWrapCB(Fl_Widget* w, void* data)
{
   TRACE();
   static const char* params[1] = {"none"};

   WindowInfo* window = WidgetToWindow(w);
   AppContext.callAction(window->lastFocus, "set_wrap_text", Fl::event(), params, 1);
}

static void newlineWrapCB(Fl_Widget* w, void* data)
{
   TRACE();
   static const char* params[1] = {"auto"};

   WindowInfo* window = WidgetToWindow(w);
   AppContext.callAction(window->lastFocus, "set_wrap_text", Fl::event(), params, 1);
}

static void continuousWrapCB(Fl_Widget* w, void* data)
{
   TRACE();
   static const char* params[1] = {"continuous"};

   WindowInfo* window = WidgetToWindow(w);
   AppContext.callAction(window->lastFocus, "set_wrap_text", Fl::event(), params, 1);
}

static void wrapMarginCB(Fl_Widget* w, void* data)
{
   TRACE();
   WindowInfo* window = WidgetToWindow(w);

   WrapMarginDialog(window->mainWindow, window);
}

static void backlightCharsCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    int applyBacklight = NeToggleButtonGetState(w);
// TODO:    window = WidgetToWindow(w);
// TODO:    SetBacklightChars(window, applyBacklight?GetPrefBacklightCharTypes():NULL);
}

static void tabsCB(Fl_Widget* w, void* data)
{
   TRACE();
   WindowInfo* window = WidgetToWindow(w);
   TabsPrefDialog(window->mainWindow, window);
}

static void statsCB(Fl_Widget* w, void* data)
{
   TRACE();

   AppContext.callAction(w, "set_statistics_line", Fl::event(), NULL, 0);
}

static void autoIndentOffDefCB(Fl_Widget* w, void* data)
{
   /* Set the preference and make the other windows' menus agree */
   SetPrefAutoIndent(NO_AUTO_INDENT);
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->autoIndentOffDefItem, true, false);
// TODO:       NeToggleButtonSetState(win->autoIndentDefItem, false, false);
// TODO:       NeToggleButtonSetState(win->smartIndentDefItem, false, false);
// TODO:    }
}

static void autoIndentDefCB(Fl_Widget* w, void* data)
{
   /* Set the preference and make the other windows' menus agree */
   SetPrefAutoIndent(AUTO_INDENT);
// TODO:    WindowInfo* win;
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->autoIndentDefItem, true, false);
// TODO:       NeToggleButtonSetState(win->autoIndentOffDefItem, false, false);
// TODO:       NeToggleButtonSetState(win->smartIndentDefItem, false, false);
// TODO:    }
}

static void smartIndentDefCB(Fl_Widget* w, void* data)
{
   /* Set the preference and make the other windows' menus agree */
   SetPrefAutoIndent(SMART_INDENT);
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->smartIndentDefItem, true, false);
// TODO:       NeToggleButtonSetState(win->autoIndentOffDefItem, false, false);
// TODO:       NeToggleButtonSetState(win->autoIndentDefItem, false, false);
// TODO:    }
}

static void autoSaveDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefAutoSave(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->autoSaveDefItem, state, false);
// TODO:    }
}

static void preserveDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefSaveOldVersion(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->saveLastDefItem, state, false);
// TODO:    }
}

static void fontDefCB(Fl_Widget* w, void* data)
{
   TRACE();
   ChooseFonts(WidgetToWindow(w), false);
}

static void colorDefCB(Fl_Widget* w, void* data)
{
   TRACE();
   ChooseColors(WidgetToWindow(w));
}

static void noWrapDefCB(Fl_Widget* w, void* data)
{
   TRACE();

   /* Set the preference and make the other windows' menus agree */
   SetPrefWrap(NO_WRAP);

// TODO:    for (WindowInfo* win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->noWrapDefItem, true, false);
// TODO:       NeToggleButtonSetState(win->newlineWrapDefItem, false, false);
// TODO:       NeToggleButtonSetState(win->contWrapDefItem, false, false);
// TODO:    }
}

static void newlineWrapDefCB(Fl_Widget* w, void* data)
{
   TRACE();

   /* Set the preference and make the other windows' menus agree */
   SetPrefWrap(NEWLINE_WRAP);
// TODO:    for ( WindowInfo* win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->newlineWrapDefItem, true, false);
// TODO:       NeToggleButtonSetState(win->contWrapDefItem, false, false);
// TODO:       NeToggleButtonSetState(win->noWrapDefItem, false, false);
// TODO:    }
}

static void contWrapDefCB(Fl_Widget* w, void* data)
{
   TRACE();

   /* Set the preference and make the other windows' menus agree */
   SetPrefWrap(CONTINUOUS_WRAP);
// TODO:    for (WindowInfo* win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->contWrapDefItem, true, false);
// TODO:       NeToggleButtonSetState(win->newlineWrapDefItem, false, false);
// TODO:       NeToggleButtonSetState(win->noWrapDefItem, false, false);
// TODO:    }
}

static void wrapMarginDefCB(Fl_Widget* w, void* data)
{
   TRACE();
   WindowInfo* window = WidgetToWindow(w);
   WrapMarginDialog(window->mainWindow, window);
}

static void smartTagsDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    SetPrefSmartTags(true);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->smartTagsDefItem, true, false);
// TODO:       NeToggleButtonSetState(win->allTagsDefItem, false, false);
// TODO:    }
}

static void showAllTagsDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    SetPrefSmartTags(false);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->smartTagsDefItem, false, false);
// TODO:       NeToggleButtonSetState(win->allTagsDefItem, true, false);
// TODO:    }
}

static void shellSelDefCB(Fl_Widget* w, void* data)
{
   TRACE();
   SelectShellDialog(WidgetToWindow(w)->mainWindow, NULL);
}

static void tabsDefCB(Fl_Widget* w, void* data)
{
   TRACE();
   TabsPrefDialog(WidgetToWindow(w)->mainWindow, NULL);
}

static void showMatchingOffDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefShowMatching(NO_FLASH);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->showMatchingOffDefItem, true, false);
// TODO:       NeToggleButtonSetState(win->showMatchingDelimitDefItem, false, false);
// TODO:       NeToggleButtonSetState(win->showMatchingRangeDefItem, false, false);
// TODO:    }
}

static void showMatchingDelimitDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefShowMatching(FLASH_DELIMIT);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->showMatchingOffDefItem, false, false);
// TODO:       NeToggleButtonSetState(win->showMatchingDelimitDefItem, true, false);
// TODO:       NeToggleButtonSetState(win->showMatchingRangeDefItem, false, false);
// TODO:    }
}

static void showMatchingRangeDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefShowMatching(FLASH_RANGE);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->showMatchingOffDefItem, false, false);
// TODO:       NeToggleButtonSetState(win->showMatchingDelimitDefItem, false, false);
// TODO:       NeToggleButtonSetState(win->showMatchingRangeDefItem, true, false);
// TODO:    }
}

static void matchSyntaxBasedDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefMatchSyntaxBased(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->matchSyntaxBasedDefItem, state, false);
// TODO:    }
}

static void backlightCharsDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefBacklightChars(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->backlightCharsDefItem, state, false);
// TODO:    }
}

static void highlightOffDefCB(Fl_Widget* w, void* data)
{
   TRACE();
   WindowInfo* win;
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefHighlightSyntax(false);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->highlightOffDefItem, true, false);
// TODO:       NeToggleButtonSetState(win->highlightDefItem, false, false);
// TODO:    }
}

static void highlightDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefHighlightSyntax(true);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->highlightOffDefItem, false, false);
// TODO:       NeToggleButtonSetState(win->highlightDefItem, true, false);
// TODO:    }
}

static void highlightingDefCB(Fl_Widget* w, void* data)
{
   TRACE();
   EditHighlightPatterns(WidgetToWindow(w));
}

static void smartMacrosDefCB(Fl_Widget* w, void* data)
{
   TRACE();
   EditSmartIndentMacros(WidgetToWindow(w));
}

static void stylesDefCB(Fl_Widget* w, void* data)
{
   TRACE();
   EditHighlightStyles(NULL);
}

static void languageDefCB(Fl_Widget* w, void* data)
{
   TRACE();
   EditLanguageModes();
}

static void shellDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    EditShellMenu(WidgetToWindow(w));
}

static void macroDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    EditMacroMenu(WidgetToWindow(w));
}

static void bgMenuDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    EditBGMenu(WidgetToWindow(w));
}

static void customizeTitleDefCB(Fl_Widget* w, void* data)
{
   TRACE();
   WindowInfo* window = WidgetToWindow(w);
   EditCustomTitleFormat(window);
}

static void searchDlogsDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefSearchDlogs(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->searchDlogsDefItem, state, false);
// TODO:    }
}

static void beepOnSearchWrapDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefBeepOnSearchWrap(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->beepOnSearchWrapDefItem, state, false);
// TODO:    }
}

static void keepSearchDlogsDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefKeepSearchDlogs(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->keepSearchDlogsDefItem, state, false);
// TODO:    }
}

static void searchWrapsDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefSearchWraps(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->searchWrapsDefItem, state, false);
// TODO:    }
}

static void appendLFCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    SetPrefAppendLF(state);
// TODO:    for (win = WindowList; win != NULL; win = win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->appendLFItem, state, false);
// TODO:    }
// TODO: }
}

static void sortOpenPrevDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference, make the other windows' menus agree, and invalidate their Open Previous menus */
// TODO:    SetPrefSortOpenPrevMenu(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->sortOpenPrevDefItem, state, false);
// TODO:    }
}

static void reposDlogsDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefRepositionDialogs(state);
// TODO:    SetPointerCenteredDialogs(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->reposDlogsDefItem, state, false);
// TODO:    }
}

static void autoScrollDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefAutoScroll(state);
// TODO:    /* XXX: Should we ensure auto-scroll now if needed? */
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->autoScrollDefItem, state, false);
// TODO:    }
}

static void modWarnDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefWarnFileMods(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->modWarnDefItem, state, false);
// TODO:       NeSetSensitive(win->modWarnRealDefItem, state);
// TODO:    }
}

static void modWarnRealDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefWarnRealFileMods(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->modWarnRealDefItem, state, false);
// TODO:    }
}

static void exitWarnDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefWarnExit(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->exitWarnDefItem, state, false);
// TODO:    }
}

static void openInTabDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefOpenInTab(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:       NeToggleButtonSetState(win->openInTabDefItem, state, false);
}

static void tabBarDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefTabBar(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->tabBarDefItem, state, false);
// TODO:       ShowWindowTabBar(win);
// TODO:    }
}

static void tabBarHideDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefTabBarHideOne(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeToggleButtonSetState(win->tabBarHideDefItem, state, false);
// TODO:       ShowWindowTabBar(win);
// TODO:    }
}

static void toolTipsDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefToolTips(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       XtVaSetValues(win->tab, XltNshowBubble, GetPrefToolTips(), NULL);
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->toolTipsDefItem, state, false);
// TODO:    }
}

static void tabNavigateDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefGlobalTabNavigate(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->tabNavigateDefItem, state, false);
// TODO:    }
}

static void tabSortDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefSortTabs(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->tabSortDefItem, state, false);
// TODO:    }
// TODO: 
// TODO:    /* If we just enabled sorting, sort all tabs.  Note that this reorders
// TODO:       the next pointers underneath us, which is scary, but SortTabBar never
// TODO:       touches windows that are earlier in the WindowList so it's ok. */
// TODO:    if (state)
// TODO:    {
// TODO:       Fl_Widget* shell=(Fl_Widget*)0;
// TODO:       for (win=WindowList; win!=NULL; win=win->next)
// TODO:       {
// TODO:          if (win->mainWindow != shell)
// TODO:          {
// TODO:             SortTabBar(win);
// TODO:             shell = win->mainWindow;
// TODO:          }
// TODO:       }
// TODO:    }
}

static void statsLineDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefStatsLine(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->statsLineDefItem, state, false);
// TODO:    }
}

static void iSearchLineDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefISearchLine(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->iSearchLineDefItem, state, false);
// TODO:    }
}

static void lineNumsDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefLineNums(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->lineNumsDefItem, state, false);
// TODO:    }
}

static void pathInWindowsMenuDefCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO:    int state = NeToggleButtonGetState(w);
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    SetPrefShowPathInWindowsMenu(state);
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (IsTopDocument(win))
// TODO:          NeToggleButtonSetState(win->pathInWindowsMenuDefItem, state, false);
// TODO:    }
// TODO:    InvalidateWindowMenus();
}

static void searchLiteralCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    if (NeToggleButtonGetState(w))
// TODO:    {
// TODO:       SetPrefSearch(SEARCH_LITERAL);
// TODO:       for (win=WindowList; win!=NULL; win=win->next)
// TODO:       {
// TODO:          if (!IsTopDocument(win))
// TODO:             continue;
// TODO:          NeToggleButtonSetState(win->searchLiteralDefItem, true, false);
// TODO:          NeToggleButtonSetState(win->searchCaseSenseDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchLiteralWordDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchCaseSenseWordDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchRegexDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchRegexNoCaseDefItem, false, false);
// TODO:       }
// TODO:    }
}

static void searchCaseSenseCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    if (NeToggleButtonGetState(w))
// TODO:    {
// TODO:       SetPrefSearch(SEARCH_CASE_SENSE);
// TODO:       for (win=WindowList; win!=NULL; win=win->next)
// TODO:       {
// TODO:          if (!IsTopDocument(win))
// TODO:             continue;
// TODO:          NeToggleButtonSetState(win->searchLiteralDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchCaseSenseDefItem, true, false);
// TODO:          NeToggleButtonSetState(win->searchLiteralWordDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchCaseSenseWordDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchRegexDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchRegexNoCaseDefItem, false, false);
// TODO:       }
// TODO:    }
}

static void searchLiteralWordCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    if (NeToggleButtonGetState(w))
// TODO:    {
// TODO:       SetPrefSearch(SEARCH_LITERAL_WORD);
// TODO:       for (win=WindowList; win!=NULL; win=win->next)
// TODO:       {
// TODO:          if (!IsTopDocument(win))
// TODO:             continue;
// TODO:          NeToggleButtonSetState(win->searchLiteralDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchCaseSenseDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchLiteralWordDefItem, true, false);
// TODO:          NeToggleButtonSetState(win->searchCaseSenseWordDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchRegexDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchRegexNoCaseDefItem, false, false);
// TODO:       }
// TODO:    }
}

static void searchCaseSenseWordCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    if (NeToggleButtonGetState(w))
// TODO:    {
// TODO:       SetPrefSearch(SEARCH_CASE_SENSE_WORD);
// TODO:       for (win=WindowList; win!=NULL; win=win->next)
// TODO:       {
// TODO:          if (!IsTopDocument(win))
// TODO:             continue;
// TODO:          NeToggleButtonSetState(win->searchLiteralDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchCaseSenseDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchLiteralWordDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchCaseSenseWordDefItem, true, false);
// TODO:          NeToggleButtonSetState(win->searchRegexDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchRegexNoCaseDefItem, false, false);
// TODO:       }
// TODO:    }
}

static void searchRegexCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    if (NeToggleButtonGetState(w))
// TODO:    {
// TODO:       SetPrefSearch(SEARCH_REGEX);
// TODO:       for (win=WindowList; win!=NULL; win=win->next)
// TODO:       {
// TODO:          if (!IsTopDocument(win))
// TODO:             continue;
// TODO:          NeToggleButtonSetState(win->searchLiteralDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchCaseSenseDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchLiteralWordDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchCaseSenseWordDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchRegexDefItem, true, false);
// TODO:          NeToggleButtonSetState(win->searchRegexNoCaseDefItem, false, false);
// TODO:       }
// TODO:    }
}

static void searchRegexNoCaseCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    /* Set the preference and make the other windows' menus agree */
// TODO:    if (NeToggleButtonGetState(w))
// TODO:    {
// TODO:       SetPrefSearch(SEARCH_REGEX_NOCASE);
// TODO:       for (win=WindowList; win!=NULL; win=win->next)
// TODO:       {
// TODO:          if (!IsTopDocument(win))
// TODO:             continue;
// TODO:          NeToggleButtonSetState(win->searchLiteralDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchCaseSenseDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchLiteralWordDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchCaseSenseWordDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchRegexDefItem, false, false);
// TODO:          NeToggleButtonSetState(win->searchRegexNoCaseDefItem, true, false);
// TODO:       }
// TODO:    }
}

static void size24x80CB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    setWindowSizeDefault(24, 80);
}

static void size40x80CB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    setWindowSizeDefault(40, 80);
}

static void size60x80CB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    setWindowSizeDefault(60, 80);
}

static void size80x80CB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    setWindowSizeDefault(80, 80);
}

static void sizeCustomCB(Fl_Widget* w, void* data)
{
   TRACE();
   RowColumnPrefDialog(WidgetToWindow(w)->mainWindow);
   updateWindowSizeMenus();
}

static void savePrefCB(Fl_Widget* w, void* data)
{
   TRACE();
   SaveNEditPrefs(WidgetToWindow(w)->mainWindow, false);
}

static void formFeedCB(Fl_Widget* w, void* data)
{
   TRACE();
   static const char* params[1] = {"\f"};

   AppContext.callAction(WidgetToWindow(w)->lastFocus, "insert_string",
      Fl::event(), params, 1);
}

static void cancelShellCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    AbortShellCommand(WidgetToWindow(w));
}

static void learnCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    BeginLearn(WidgetToWindow(w));
}

static void finishLearnCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    FinishLearn();
}

static void cancelLearnCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    CancelMacroOrLearn(WidgetToWindow(w));
}

static void replayCB(Fl_Widget* w, void* data)
{
   TRACE();
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    Replay(WidgetToWindow(w));
}

// TODO: static void windowMenuCB(Fl_Widget* w, void* data)
// TODO: {
// TODO:    window = WidgetToWindow(w);
// TODO: 
// TODO:    if (!window->windowMenuValid)
// TODO:    {
// TODO:       updateWindowMenu(window);
// TODO:       window->windowMenuValid = true;
// TODO:    }
// TODO: }

static void prevOpenMenuCB(Fl_Widget* w, void* data)
{
   TRACE();
   WindowInfo* window = WidgetToWindow(w);

   updatePrevOpenMenu(window);
}

// TODO: static void unloadTagsFileMenuCB(Fl_Widget* w, void* data)
// TODO: {
// TODO:    updateTagsFileMenu(WidgetToWindow(w));
// TODO: }
// TODO: 
// TODO: static void unloadTipsFileMenuCB(Fl_Widget* w, void* data)
// TODO: {
// TODO:    updateTipsFileMenu(WidgetToWindow(w));
// TODO: }
// TODO: 
/*
** open a new tab or window.
*/
static void newAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE()
   WindowInfo* window = WidgetToWindow(w);

   int openInTab = GetPrefOpenInTab();

   if (*nArgs > 0)
   {
      if (strcmp(args[0], "prefs") == 0)
      {
         /* accept default */;
      }
      else if (strcmp(args[0], "tab") == 0)
      {
         openInTab = 1;
      }
      else if (strcmp(args[0], "window") == 0)
      {
         openInTab = 0;
      }
      else if (strcmp(args[0], "opposite") == 0)
      {
         openInTab = !openInTab;
      }
      else
      {
         fprintf(stderr, "nedit: Unknown argument to action procedure \"new\": %s\n", args[0]);
      }
   }

   EditNewFile(openInTab? window : NULL, NULL, false, NULL, window->path);
   CheckCloseDim();
}

/*
** These are just here because our techniques make it hard to bind a menu item
** to an action procedure that takes arguments.  The user doesn't need to know
** about them -- they can use new( "opposite" ) or new( "tab" ).
*/
static void newOppositeAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   EditNewFile(GetPrefOpenInTab()? NULL : window, NULL, false, NULL,window->path);
   CheckCloseDim();
}
static void newTabAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   EditNewFile(window, NULL, false, NULL, window->path);
   CheckCloseDim();
}

static void openDialogAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE();
   WindowInfo* window = WidgetToWindow(w);
   char fullname[MAXPATHLEN];
   const char* params[2];
   int n=1;

   int response = PromptForExistingFile(window, "Open File", fullname);
   if (response != GFN_OK)
      return;
   params[0] = fullname;

   if (*nArgs>0 && !strcmp(args[0], "1"))
      params[n++] = "1";

   AppContext.callAction(window->lastFocus, "open", Fl::event(), params, n);
   CheckCloseDim();
}

static void openAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE();
   WindowInfo* window = WidgetToWindow(w);
   char filename[MAXPATHLEN] = "", pathname[MAXPATHLEN] = "";

   if (*nArgs == 0)
   {
      fprintf(stderr, "nedit: open action requires file argument\n");
      return;
   }
   if (0 != ParseFilename(args[0], filename, pathname) || strlen(filename) + strlen(pathname) > MAXPATHLEN - 1)
   {
      fprintf(stderr, "nedit: invalid file name for open action: %s\n", args[0]);
      return;
   }
   EditExistingFile(window, filename, pathname, 0, NULL, false, NULL, GetPrefOpenInTab(), false);
   CheckCloseDim();
}

static void openSelectedAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   OpenSelectedFile(WidgetToWindow(w), GetTimeOfDay());
   CheckCloseDim();
}

static void closeAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   int preResponse = PROMPT_SBC_DIALOG_RESPONSE;

   if (*nArgs > 0)
   {
      if (strcmp(args[0], "prompt") == 0)
      {
         preResponse = PROMPT_SBC_DIALOG_RESPONSE;
      }
      else if (strcmp(args[0], "save") == 0)
      {
         preResponse = YES_SBC_DIALOG_RESPONSE;
      }
      else if (strcmp(args[0], "nosave") == 0)
      {
         preResponse = NO_SBC_DIALOG_RESPONSE;
      }
   }
   CloseFileAndWindow(WidgetToWindow(w), preResponse);
   CheckCloseDim();
   WidgetToMainWindow(w)->redraw();
}

static void saveAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   SaveWindow(window);
}

static void saveAsDialogAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);
   int response, addWrap, fileFormat;
   char fullname[MAXPATHLEN];
   const char*params[2];

   response = PromptForNewFile(window, "Save File As", fullname, &fileFormat, &addWrap);
   if (response != GFN_OK)
      return;
   window->fileFormat = fileFormat;
   params[0] = fullname;
   params[1] = "wrapped";
   AppContext.callAction(window->lastFocus, "save_as", event, params, addWrap?2:1);
}

static void saveAsAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   if (*nArgs == 0)
   {
      fprintf(stderr, "nedit: save_as action requires file argument\n");
      return;
   }
   SaveWindowAs(WidgetToWindow(w), args[0], *nArgs == 2 && !strCaseCmp(args[1], "wrapped"));
}

static void revertDialogAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);
   int b;

   /* re-reading file is irreversible, prompt the user first */
   if (window->fileChanged)
   {
      b = DialogF(DF_QUES, window->mainWindow, 2, "Discard Changes",
                  "Discard changes to\n%s%s?", "OK", "Cancel", window->path,
                  window->filename);
   }
   else
   {
      b = DialogF(DF_QUES, window->mainWindow, 2, "Reload File",
                  "Re-load file\n%s%s?", "Re-read", "Cancel", window->path,
                  window->filename);
   }

   if (b != 1)
   {
      return;
   }
   AppContext.callAction(window->lastFocus, "revert_to_saved", event, NULL, 0);
}

static void revertAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   RevertToSaved(WidgetToWindow(w));
}

static void includeDialogAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);
   char filename[MAXPATHLEN];
   const char*params[1];
   int response;

   if (CheckReadOnly(window))
      return;
   response = PromptForExistingFile(window, "Include File", filename);
   if (response != GFN_OK)
      return;
   params[0] = filename;
   AppContext.callAction(window->lastFocus, "include_file", event, params, 1);
}

static void includeAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   if (*nArgs == 0)
   {
      fprintf(stderr, "nedit: include action requires file argument\n");
      return;
   }
   IncludeFile(WidgetToWindow(w), args[0]);
}

static void loadMacroDialogAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);
   char filename[MAXPATHLEN];
   const char *params[1];
   int response;

   response = PromptForExistingFile(window, "Load Macro File", filename);
   if (response != GFN_OK)
      return;
   params[0] = filename;
   AppContext.callAction(window->lastFocus, "load_macro_file", event, params, 1);
}

static void loadMacroAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   if (*nArgs == 0)
   {
      fprintf(stderr,"nedit: load_macro_file action requires file argument\n");
      return;
   }
   ReadMacroFile(WidgetToWindow(w), args[0], true);
}

static void loadTagsDialogAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);
   char filename[MAXPATHLEN];
   const char* params[1];
   int response;

   response = PromptForExistingFile(window, "Load Tags File", filename);
   if (response != GFN_OK)
      return;
   params[0] = filename;
   AppContext.callAction(window->lastFocus, "load_tags_file", event, params, 1);
}

static void loadTagsAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   if (*nArgs == 0)
   {
      fprintf(stderr,"nedit: load_tags_file action requires file argument\n");
      return;
   }

   if (!AddTagsFile(args[0], TAG))
   {
      DialogF(DF_WARN, WidgetToWindow(w)->mainWindow, 1, "Error Reading File",
              "Error reading ctags file:\n'%s'\ntags not loaded", "OK",
              args[0]);
   }
}

static void unloadTagsAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   if (*nArgs == 0)
   {
      fprintf(stderr,
              "nedit: unload_tags_file action requires file argument\n");
      return;
   }

   if (DeleteTagsFile(args[0], TAG, true))
   {
      WindowInfo* win;

// TODO:       /* refresh the "Unload Tags File" tear-offs after unloading, or
// TODO:          close the tear-offs if all tags files have been unloaded */
// TODO:       for (win=WindowList; win!=NULL; win=win->next)
// TODO:       {
// TODO:          if (IsTopDocument(win) && !XmIsMenuShell(XtParent(win->unloadTagsMenuPane)))
// TODO:          {
// TODO:             if (XtIsSensitive(win->unloadTagsMenuItem))
// TODO:                updateTagsFileMenu(win);
// TODO:          }

   }
}

static void loadTipsDialogAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    char filename[MAXPATHLEN], *params[1];
// TODO:    int response;
// TODO: 
// TODO:    response = PromptForExistingFile(window, "Load Calltips File", filename);
// TODO:    if (response != GFN_OK)
// TODO:       return;
// TODO:    params[0] = filename;
// TODO:    XtCallActionProc(window->lastFocus, "load_tips_file", event, params, 1);
}

static void loadTipsAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    if (*nArgs == 0)
// TODO:    {
// TODO:       fprintf(stderr,"nedit: load_tips_file action requires file argument\n");
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    if (!AddTagsFile(args[0], TIP))
// TODO:    {
// TODO:       DialogF(DF_WARN, WidgetToWindow(w)->mainWindow, 1, "Error Reading File",
// TODO:               "Error reading tips file:\n'%s'\ntips not loaded", "OK",
// TODO:               args[0]);
// TODO:    }
}

static void unloadTipsAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    if (*nArgs == 0)
// TODO:    {
// TODO:       fprintf(stderr,
// TODO:               "nedit: unload_tips_file action requires file argument\n");
// TODO:       return;
// TODO:    }
// TODO:    /* refresh the "Unload Calltips File" tear-offs after unloading, or
// TODO:       close the tear-offs if all tips files have been unloaded */
// TODO:    if (DeleteTagsFile(args[0], TIP, true))
// TODO:    {
// TODO:       WindowInfo* win;
// TODO: 
// TODO:       for (win=WindowList; win!=NULL; win=win->next)
// TODO:       {
// TODO:          if (IsTopDocument(win) &&
// TODO:                !XmIsMenuShell(XtParent(win->unloadTipsMenuPane)))
// TODO:          {
// TODO:             if (XtIsSensitive(win->unloadTipsMenuItem))
// TODO:                updateTipsFileMenu(win);
// TODO:          }
// TODO:       }
// TODO:    }
}

static void printAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    PrintWindow(WidgetToWindow(w), false);
}

static void printSelAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    PrintWindow(WidgetToWindow(w), true);
}

static void exitAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (!CheckPrefsChangesSaved(window->mainWindow))
      return;

   // If this is not the last window (more than one window is open),
   // confirm with the user before exiting.
   if (GetPrefWarnExit() && !(window == WindowList && window->next == NULL))
   {
      int resp, titleLen, lineLen;
      char exitMsg[DF_MAX_MSG_LENGTH], *ptr, *title;
      char filename[MAXPATHLEN];
      WindowInfo* win;

      // List the windows being edited and make sure the user really wants to exit
      ptr = exitMsg;
      lineLen = 0;
      strcpy(ptr, "Editing: ");
      ptr += 9;
      lineLen += 9;
      for (win=WindowList; win!=NULL; win=win->next)
      {
         sprintf(filename, "%s%s", win->filename, win->fileChanged? "*": "");
         title = filename;
         titleLen = strlen(title);
         if (ptr - exitMsg + titleLen + 30 >= DF_MAX_MSG_LENGTH)
         {
            strcpy(ptr, "...");
            ptr += 3;
            break;
         }
         if (lineLen + titleLen + (win->next==NULL?5:2) > 50)
         {
            *ptr++ = '\n';
            lineLen = 0;
         }
         if (win->next == NULL)
         {
            sprintf(ptr, "and %s.", title);
            ptr += 5 + titleLen;
            lineLen += 5 + titleLen;
         }
         else
         {
            sprintf(ptr, "%s, ", title);
            ptr += 2 + titleLen;
            lineLen += 2 + titleLen;
         }
      }
      sprintf(ptr, "\n\nExit NEdit?");
      resp = DialogF(DF_QUES, window->mainWindow, 2, "Exit", "%s", "Exit", "Cancel", exitMsg);
      if (resp == 2)
         return;
   }

   // Close all files and exit when the last one is closed
   if (CloseAllFilesAndWindows())
      exit(EXIT_SUCCESS);
}

static void undoAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   Undo(window);
   WidgetToMainWindow(w)->redraw();
}

static void redoAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   Redo(window);
   WidgetToMainWindow(w)->redraw();
}

static void clearAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   BufRemoveSelected(window->buffer);
}

static void selAllAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   BufSelect(window->buffer, 0, window->buffer->length);
}

static void shiftLeftAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   ShiftSelection(window, SHIFT_LEFT, false);
}

static void shiftLeftTabAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   ShiftSelection(window, SHIFT_LEFT, true);
}

static void shiftRightAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   ShiftSelection(window, SHIFT_RIGHT, false);
}

static void shiftRightTabAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   ShiftSelection(window, SHIFT_RIGHT, true);
}

static void findDialogAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE();
   DoFindDlog(WidgetToWindow(w), searchDirection(0, args, nArgs),
      searchKeepDialogs(0, args, nArgs), searchType(0, args, nArgs),
      0 /*event->xbutton.time*/);
}

static void findAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE();
   if (*nArgs == 0)
   {
      fprintf(stderr, "nedit: find action requires search string argument\n");
      return;
   }
   SearchAndSelect(WidgetToWindow(w), searchDirection(1, args, nArgs), args[0],
                   searchType(1, args, nArgs), searchWrap(1, args, nArgs));
}

static void findSameAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE();
   SearchAndSelectSame(WidgetToWindow(w), searchDirection(0, args, nArgs),
                       searchWrap(0, args, nArgs));
}

static void findSelAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE();
   SearchForSelected(WidgetToWindow(w), searchDirection(0, args, nArgs),
                     searchType(0, args, nArgs), searchWrap(0, args, nArgs),
                     0 /*event->xbutton.time*/ );
}

static void startIncrFindAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE();
   BeginISearch(WidgetToWindow(w), searchDirection(0, args, nArgs));
}

static void findIncrAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    int i, continued = FALSE;
// TODO:    if (*nArgs == 0)
// TODO:    {
// TODO:       fprintf(stderr, "nedit: find action requires search string argument\n");
// TODO:       return;
// TODO:    }
// TODO:    for (i=1; i<(int)*nArgs; i++)
// TODO:       if (!strCaseCmp(args[i], "continued"))
// TODO:          continued = TRUE;
// TODO:    SearchAndSelectIncremental(WidgetToWindow(w),
// TODO:                               searchDirection(1, args, nArgs), args[0],
// TODO:                               searchType(1, args, nArgs), searchWrap(1, args, nArgs), continued);
}

static void replaceDialogAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   DoFindReplaceDlog(window, searchDirection(0, args, nArgs),
                     searchKeepDialogs(0, args, nArgs), searchType(0, args, nArgs),
                     0/*event->xbutton.time*/);
}

static void replaceAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO: 
// TODO:    if (CheckReadOnly(window))
// TODO:       return;
// TODO:    if (*nArgs < 2)
// TODO:    {
// TODO:       fprintf(stderr,
// TODO:               "nedit: replace action requires search and replace string arguments\n");
// TODO:       return;
// TODO:    }
// TODO:    SearchAndReplace(window, searchDirection(2, args, nArgs),
// TODO:                     args[0], args[1], searchType(2, args, nArgs), searchWrap(2, args, nArgs));
}

static void replaceAllAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   if (*nArgs < 2)
   {
      fprintf(stderr, "nedit: replace_all action requires search and replace string arguments\n");
      return;
   }
   ReplaceAll(window, args[0], args[1], searchType(2, args, nArgs));
}

static void replaceInSelAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   if (*nArgs < 2)
   {
      fprintf(stderr, "nedit: replace_in_selection requires search and replace string arguments\n");
      return;
   }
   ReplaceInSelection(window, args[0], args[1], searchType(2, args, nArgs));
}

static void replaceSameAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   ReplaceSame(window, searchDirection(0, args, nArgs), searchWrap(0, args, nArgs));
}

static void replaceFindAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
   {
      return;
   }

   if (*nArgs < 2)
   {
      DialogF(DF_WARN, window->mainWindow, 1, "Error in replace_find",
              "replace_find action requires search and replace string arguments",
              "OK");
      return;
   }

   ReplaceAndSearch(window, searchDirection(2, args, nArgs), args[0], args[1],
                    searchType(2, args, nArgs), searchWrap(0, args, nArgs));
}

static void replaceFindSameAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   ReplaceFindSame(window, searchDirection(0, args, nArgs), searchWrap(0, args, nArgs));
}

static void gotoAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   int lineNum, column, position, curCol;

   /* Accept various formats:
         [line]:[column]   (menu action)
         line              (macro call)
         line, column      (macro call) */
   if (*nArgs == 0 || *nArgs > 2
         || (*nArgs == 1 && StringToLineAndCol(args[0], &lineNum, &column) == -1)
         || (*nArgs == 2 && (!StringToNum(args[0], &lineNum) || !StringToNum(args[1], &column))))
   {
      fprintf(stderr,"nedit: goto_line_number action requires line and/or column number\n");
      return;
   }

   Ne_Text_Editor* text = static_cast<Ne_Text_Editor*>(w);

   /* User specified column, but not line number */
   if (lineNum == -1)
   {
      position = TextGetCursorPos(text);
      if (TextPosToLineAndCol(text, position, &lineNum, &curCol) == false)
         return;
   }
   else if (column == -1)
   {
      /* User didn't specify a column */
      SelectNumberedLine(WidgetToWindow(w), lineNum);
      return;
   }

   position = TextLineAndColToPos(text, lineNum, column);
   if (position == -1)
      return;

   TextSetCursorPos(text, position);
}

static void gotoDialogAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   GotoLineNumber(WidgetToWindow(w));
}

static void gotoSelectedAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   GotoSelectedLineNumber(WidgetToWindow(w), 0 /*event->xbutton.time*/);
}

static void repeatDialogAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    RepeatDialog(WidgetToWindow(w));
}

static void repeatMacroAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    int how;
// TODO: 
// TODO:    if (*nArgs != 2)
// TODO:    {
// TODO:       fprintf(stderr, "nedit: repeat_macro requires two arguments\n");
// TODO:       return;
// TODO:    }
// TODO:    if (!strcmp(args[0], "in_selection"))
// TODO:       how = REPEAT_IN_SEL;
// TODO:    else if (!strcmp(args[0], "to_end"))
// TODO:       how = REPEAT_TO_END;
// TODO:    else if (sscanf(args[0], "%d", &how) != 1)
// TODO:    {
// TODO:       fprintf(stderr, "nedit: repeat_macro requires method/count\n");
// TODO:       return;
// TODO:    }
// TODO:    RepeatMacro(WidgetToWindow(w), args[1], how);
}

static void markAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    if (*nArgs == 0 || strlen(args[0]) != 1 ||
// TODO:          !isalnum((unsigned char)args[0][0]))
// TODO:    {
// TODO:       fprintf(stderr,"nedit: mark action requires a single-letter label\n");
// TODO:       return;
// TODO:    }
// TODO:    AddMark(WidgetToWindow(w), w, args[0][0]);
}

static void markDialogAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    MarkDialog(WidgetToWindow(w));
}

static void gotoMarkAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    if (*nArgs == 0 || strlen(args[0]) != 1 ||
// TODO:          !isalnum((unsigned char)args[0][0]))
// TODO:    {
// TODO:       fprintf(stderr,
// TODO:               "nedit: goto_mark action requires a single-letter label\n");
// TODO:       return;
// TODO:    }
// TODO:    GotoMark(WidgetToWindow(w), w, args[0][0], *nArgs > 1 &&
// TODO:             !strcmp(args[1], "extend"));
}

static void gotoMarkDialogAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    GotoMarkDialog(WidgetToWindow(w), *nArgs!=0 && !strcmp(args[0], "extend"));
}

static void selectToMatchingAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    SelectToMatchingCharacter(WidgetToWindow(w));
}

static void gotoMatchingAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   GotoMatchingCharacter(WidgetToWindow(w));
}

static void findDefAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    FindDefinition(WidgetToWindow(w), event->xbutton.time,
// TODO:                   *nArgs == 0 ? NULL : args[0]);
}

static void showTipAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    FindDefCalltip(WidgetToWindow(w), event->xbutton.time,
// TODO:                   *nArgs == 0 ? NULL : args[0]);
}

static void splitPaneAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO: 
// TODO:    SplitPane(window);
// TODO:    if (IsTopDocument(window))
// TODO:    {
// TODO:       NeSetSensitive(window->splitPaneItem, window->nPanes < MAX_PANES);
// TODO:       NeSetSensitive(window->closePaneItem, window->nPanes > 0);
// TODO:    }
}

static void closePaneAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO: 
// TODO:    ClosePane(window);
// TODO:    if (IsTopDocument(window))
// TODO:    {
// TODO:       NeSetSensitive(window->splitPaneItem, window->nPanes < MAX_PANES);
// TODO:       NeSetSensitive(window->closePaneItem, window->nPanes > 0);
// TODO:    }
}

static void detachDocumentDialogAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    int resp;
// TODO: 
// TODO:    if (NDocuments(window) < 2)
// TODO:       return;
// TODO: 
// TODO:    resp = DialogF(DF_QUES, window->mainWindow, 2, "Detach %s?",
// TODO:                   "Detach", "Cancel", window->filename);
// TODO: 
// TODO:    if (resp == 1)
// TODO:       DetachDocument(window);
}

static void detachDocumentAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO: 
// TODO:    if (NDocuments(window) < 2)
// TODO:       return;
// TODO: 
// TODO:    DetachDocument(window);
}

static void moveDocumentDialogAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    MoveDocumentDialog(WidgetToWindow(w));
}

static void nextDocumentAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    NextDocument(WidgetToWindow(w));
}

static void prevDocumentAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    PreviousDocument(WidgetToWindow(w));
}

static void lastDocumentAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    LastDocument(WidgetToWindow(w));
}

static void capitalizeAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   UpcaseSelection(window);
}

static void lowercaseAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   DowncaseSelection(window);
}

static void fillAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);

   if (CheckReadOnly(window))
      return;
   FillSelection(window);
}

static void controlDialogAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);
   unsigned char charCodeString[2];
   char charCodeText[DF_MAX_PROMPT_LENGTH], dummy[DF_MAX_PROMPT_LENGTH];
   const char* params[1];
   int charCode, nRead;

   if (CheckReadOnly(window))
      return;

   int response = DialogF(DF_PROMPT, window->mainWindow, 2, "Insert Ctrl Code", "ASCII Character Code:", charCodeText, "OK", "Cancel");
   if (response == 0)
      return;
   /* If we don't scan for a trailing string invalid input would be accepted sometimes. */
   nRead = sscanf(charCodeText, "%i%s", &charCode, dummy);
   if (nRead != 1 || charCode < 0 || charCode > 255)
   {
      fl_beep();
      return;
   }
   charCodeString[0] = (unsigned char)charCode;
   charCodeString[1] = '\0';
   params[0] = (char*)charCodeString;

   if (!BufSubstituteNullChars((char*)charCodeString, 1, window->buffer))
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Error", "Too much binary data", "OK");
      return;
   }

   AppContext.callAction(window->lastFocus, "insert_string", event, params, 1);
}

static void filterDialogAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    char* params[1], cmdText[DF_MAX_PROMPT_LENGTH];
// TODO:    int resp;
// TODO:    static char** cmdHistory = NULL;
// TODO:    static int nHistoryCmds = 0;
// TODO: 
// TODO:    if (CheckReadOnly(window))
// TODO:       return;
// TODO:    if (!window->buffer->primary.selected)
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    SetDialogFPromptHistory(cmdHistory, nHistoryCmds);
// TODO: 
// TODO:    resp = DialogF(DF_PROMPT, window->mainWindow, 2, "Filter Selection", "Shell command:   (use up arrow key to recall previous)", cmdText, "OK", "Cancel");
// TODO: 
// TODO:    if (resp == 2)
// TODO:       return;
// TODO:    AddToHistoryList(cmdText, &cmdHistory, &nHistoryCmds);
// TODO:    params[0] = cmdText;
// TODO:    XtCallActionProc(w, "filter_selection", event, params, 1);
}

static void shellFilterAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO: 
// TODO:    if (CheckReadOnly(window))
// TODO:       return;
// TODO:    if (*nArgs == 0)
// TODO:    {
// TODO:       fprintf(stderr,
// TODO:               "nedit: filter_selection requires shell command argument\n");
// TODO:       return;
// TODO:    }
// TODO:    FilterSelection(window, args[0],
// TODO:                    event->xany.send_event == MACRO_EVENT_MARKER);
}

static void execDialogAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    char* params[1], cmdText[DF_MAX_PROMPT_LENGTH];
// TODO:    int resp;
// TODO:    static char** cmdHistory = NULL;
// TODO:    static int nHistoryCmds = 0;
// TODO: 
// TODO:    if (CheckReadOnly(window))
// TODO:       return;
// TODO:    SetDialogFPromptHistory(cmdHistory, nHistoryCmds);
// TODO: 
// TODO:    resp = DialogF(DF_PROMPT, window->mainWindow, 2, "Execute Command",
// TODO:                   "Shell command:   (use up arrow key to recall previous;\n"
// TODO:                   "%% expands to current filename, # to line number)", cmdText, "OK",
// TODO:                   "Cancel");
// TODO: 
// TODO:    if (resp == 2)
// TODO:       return;
// TODO:    AddToHistoryList(cmdText, &cmdHistory, &nHistoryCmds);
// TODO:    params[0] = cmdText;
// TODO:    XtCallActionProc(w, "execute_command", event, params, 1);;
}

static void execAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO: 
// TODO:    if (CheckReadOnly(window))
// TODO:       return;
// TODO:    if (*nArgs == 0)
// TODO:    {
// TODO:       fprintf(stderr,
// TODO:               "nedit: execute_command requires shell command argument\n");
// TODO:       return;
// TODO:    }
// TODO:    ExecShellCommand(window, args[0],
// TODO:                     event->xany.send_event == MACRO_EVENT_MARKER);
}

static void execLineAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO: 
// TODO:    if (CheckReadOnly(window))
// TODO:       return;
// TODO:    ExecCursorLine(window, event->xany.send_event == MACRO_EVENT_MARKER);
}

static void shellMenuAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    if (*nArgs == 0)
// TODO:    {
// TODO:       fprintf(stderr,
// TODO:               "nedit: shell_menu_command requires item-name argument\n");
// TODO:       return;
// TODO:    }
// TODO:    HidePointerOnKeyedEvent(w, event);
// TODO:    DoNamedShellMenuCmd(WidgetToWindow(w), args[0],
// TODO:                        event->xany.send_event == MACRO_EVENT_MARKER);
}

static void macroMenuAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    if (*nArgs == 0)
// TODO:    {
// TODO:       fprintf(stderr,
// TODO:               "nedit: macro_menu_command requires item-name argument\n");
// TODO:       return;
// TODO:    }
// TODO:    /* Don't allow users to execute a macro command from the menu (or accel)
// TODO:       if there's already a macro command executing, UNLESS the macro is
// TODO:       directly called from another one.  NEdit can't handle
// TODO:       running multiple, independent uncoordinated, macros in the same
// TODO:       window.  Macros may invoke macro menu commands recursively via the
// TODO:       macro_menu_command action proc, which is important for being able to
// TODO:       repeat any operation, and to embed macros within eachother at any
// TODO:       level, however, a call here with a macro running means that THE USER
// TODO:       is explicitly invoking another macro via the menu or an accelerator,
// TODO:       UNLESS the macro event marker is set */
// TODO:    if (event->xany.send_event != MACRO_EVENT_MARKER)
// TODO:    {
// TODO:       if (WidgetToWindow(w)->macroCmdData != NULL)
// TODO:       {
// TODO:          XBell(TheDisplay, 0);
// TODO:          return;
// TODO:       }
// TODO:    }
// TODO:    HidePointerOnKeyedEvent(w, event);
// TODO:    DoNamedMacroMenuCmd(WidgetToWindow(w), args[0]);
}

static void bgMenuAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    if (*nArgs == 0)
// TODO:    {
// TODO:       fprintf(stderr,
// TODO:               "nedit: bg_menu_command requires item-name argument\n");
// TODO:       return;
// TODO:    }
// TODO:    /* Same remark as for macro menu commands (see above). */
// TODO:    if (event->xany.send_event != MACRO_EVENT_MARKER)
// TODO:    {
// TODO:       if (WidgetToWindow(w)->macroCmdData != NULL)
// TODO:       {
// TODO:          XBell(TheDisplay, 0);
// TODO:          return;
// TODO:       }
// TODO:    }
// TODO:    HidePointerOnKeyedEvent(w, event);
// TODO:    DoNamedBGMenuCmd(WidgetToWindow(w), args[0]);
}

static void beginningOfSelectionAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    textBuffer* buf = TextGetBuffer(w);
// TODO:    int start, end, isRect, rectStart, rectEnd;
// TODO: 
// TODO:    if (!BufGetSelectionPos(buf, &start, &end, &isRect, &rectStart, &rectEnd))
// TODO:       return;
// TODO:    if (!isRect)
// TODO:       TextSetCursorPos(w, start);
// TODO:    else
// TODO:       TextSetCursorPos(w, BufCountForwardDispChars(buf,
// TODO:                        BufStartOfLine(buf, start), rectStart));
}

static void endOfSelectionAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    textBuffer* buf = TextGetBuffer(w);
// TODO:    int start, end, isRect, rectStart, rectEnd;
// TODO: 
// TODO:    if (!BufGetSelectionPos(buf, &start, &end, &isRect, &rectStart, &rectEnd))
// TODO:       return;
// TODO:    if (!isRect)
// TODO:       TextSetCursorPos(w, end);
// TODO:    else
// TODO:       TextSetCursorPos(w, BufCountForwardDispChars(buf,
// TODO:                        BufStartOfLine(buf, end), rectEnd));
}

static void raiseWindowAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    WindowInfo* nextWindow;
// TODO:    WindowInfo* tmpWindow;
// TODO:    int windowIndex;
// TODO:    bool focus = GetPrefFocusOnRaise();
// TODO: 
// TODO:    if (*nArgs > 0)
// TODO:    {
// TODO:       if (strcmp(args[0], "last") == 0)
// TODO:       {
// TODO:          window = WindowList;
// TODO:       }
// TODO:       else if (strcmp(args[0], "first") == 0)
// TODO:       {
// TODO:          window = WindowList;
// TODO:          if (window != NULL)
// TODO:          {
// TODO:             nextWindow = window->next;
// TODO:             while (nextWindow != NULL)
// TODO:             {
// TODO:                window = nextWindow;
// TODO:                nextWindow = nextWindow->next;
// TODO:             }
// TODO:          }
// TODO:       }
// TODO:       else if (strcmp(args[0], "previous") == 0)
// TODO:       {
// TODO:          tmpWindow = window;
// TODO:          window = WindowList;
// TODO:          if (window != NULL)
// TODO:          {
// TODO:             nextWindow = window->next;
// TODO:             while (nextWindow != NULL && nextWindow != tmpWindow)
// TODO:             {
// TODO:                window = nextWindow;
// TODO:                nextWindow = nextWindow->next;
// TODO:             }
// TODO:             if (nextWindow == NULL && tmpWindow != WindowList)
// TODO:             {
// TODO:                window = NULL;
// TODO:             }
// TODO:          }
// TODO:       }
// TODO:       else if (strcmp(args[0], "next") == 0)
// TODO:       {
// TODO:          if (window != NULL)
// TODO:          {
// TODO:             window = window->next;
// TODO:             if (window == NULL)
// TODO:             {
// TODO:                window = WindowList;
// TODO:             }
// TODO:          }
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          if (sscanf(args[0], "%d", &windowIndex) == 1)
// TODO:          {
// TODO:             if (windowIndex > 0)
// TODO:             {
// TODO:                for (window = WindowList; window != NULL && windowIndex > 1;
// TODO:                      --windowIndex)
// TODO:                {
// TODO:                   window = window->next;
// TODO:                }
// TODO:             }
// TODO:             else if (windowIndex < 0)
// TODO:             {
// TODO:                for (window = WindowList; window != NULL;
// TODO:                      window = window->next)
// TODO:                {
// TODO:                   ++windowIndex;
// TODO:                }
// TODO:                if (windowIndex >= 0)
// TODO:                {
// TODO:                   for (window = WindowList; window != NULL &&
// TODO:                         windowIndex > 0; window = window->next)
// TODO:                   {
// TODO:                      --windowIndex;
// TODO:                   }
// TODO:                }
// TODO:                else
// TODO:                {
// TODO:                   window = NULL;
// TODO:                }
// TODO:             }
// TODO:             else
// TODO:             {
// TODO:                window = NULL;
// TODO:             }
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             window = NULL;
// TODO:          }
// TODO:       }
// TODO: 
// TODO:       if (*nArgs > 1)
// TODO:       {
// TODO:          if (strcmp(args[1], "focus") == 0)
// TODO:          {
// TODO:             focus = true;
// TODO:          }
// TODO:          else if (strcmp(args[1], "nofocus") == 0)
// TODO:          {
// TODO:             focus = false;
// TODO:          }
// TODO:       }
// TODO:    }
// TODO:    if (window != NULL)
// TODO:    {
// TODO:       RaiseFocusDocumentWindow(window, focus);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:    }
}

static void focusPaneAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    Fl_Widget* newFocusPane = NULL;
// TODO:    int paneIndex;
// TODO: 
// TODO:    if (*nArgs > 0)
// TODO:    {
// TODO:       if (strcmp(args[0], "first") == 0)
// TODO:       {
// TODO:          paneIndex = 0;
// TODO:       }
// TODO:       else if (strcmp(args[0], "last") == 0)
// TODO:       {
// TODO:          paneIndex = window->nPanes;
// TODO:       }
// TODO:       else if (strcmp(args[0], "next") == 0)
// TODO:       {
// TODO:          paneIndex = WidgetToPaneIndex(window, window->lastFocus) + 1;
// TODO:          if (paneIndex > window->nPanes)
// TODO:          {
// TODO:             paneIndex = 0;
// TODO:          }
// TODO:       }
// TODO:       else if (strcmp(args[0], "previous") == 0)
// TODO:       {
// TODO:          paneIndex = WidgetToPaneIndex(window, window->lastFocus) - 1;
// TODO:          if (paneIndex < 0)
// TODO:          {
// TODO:             paneIndex = window->nPanes;
// TODO:          }
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          if (sscanf(args[0], "%d", &paneIndex) == 1)
// TODO:          {
// TODO:             if (paneIndex > 0)
// TODO:             {
// TODO:                paneIndex = paneIndex - 1;
// TODO:             }
// TODO:             else if (paneIndex < 0)
// TODO:             {
// TODO:                paneIndex = window->nPanes + (paneIndex + 1);
// TODO:             }
// TODO:             else
// TODO:             {
// TODO:                paneIndex = -1;
// TODO:             }
// TODO:          }
// TODO:       }
// TODO:       if (paneIndex >= 0 && paneIndex <= window->nPanes)
// TODO:       {
// TODO:          newFocusPane = GetPaneByIndex(window, paneIndex);
// TODO:       }
// TODO:       if (newFocusPane != NULL)
// TODO:       {
// TODO:          window->lastFocus = newFocusPane;
// TODO:          XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          XBell(TheDisplay, 0);
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       fprintf(stderr, "nedit: focus_pane requires argument\n");
// TODO:    }
// TODO: }
}

void ACTION_BOOL_PARAM_OR_TOGGLE(bool& newState, int numArgs, const char** argvVal, bool oValue, const char* actionName)
{
    if ((numArgs) > 0) {
        int intState;

        if (sscanf(argvVal[0], "%d", &intState) == 1) {
            (newState) = (intState != 0);
        }
        else {
            fprintf(stderr, "nedit: %s requires 0 or 1 argument\n", actionName); \
            return;
        }
    }
    else {
        (newState) = !(oValue);
    }
}

static void setStatisticsLineAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE();
   WindowInfo* window = WidgetToWindow(w);
   bool newState = false;

   // stats line is a shell-level item, so we toggle the button state regardless of it's 'topness'
   ACTION_BOOL_PARAM_OR_TOGGLE(newState, *nArgs, args, window->showStats, "set_statistics_line");
   // TODO: NeToggleButtonSetState(window->statsLineItem, newState, false);
   ShowStatsLine(window, newState);
}

static void setIncrementalSearchLineAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    bool newState;
// TODO: 
// TODO:    /* i-search line is a shell-level item, so we toggle the button
// TODO:       state regardless of it's 'topness' */
// TODO:    ACTION_BOOL_PARAM_OR_TOGGLE(newState, *nArgs, args, window->showISearchLine, "set_incremental_search_line");
// TODO:    NeToggleButtonSetState(window->iSearchLineItem, newState, false);
// TODO:    ShowISearchLine(window, newState);
}

static void setShowLineNumbersAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   WindowInfo* window = WidgetToWindow(w);
   bool newState = false;

   /* line numbers panel is a shell-level item, so we toggle the button
      state regardless of it's 'topness' */
   ACTION_BOOL_PARAM_OR_TOGGLE(newState, *nArgs, args, window->showLineNumbers, "set_show_line_numbers");
   //NeToggleButtonSetState(window->lineNumsItem, newState, false);
   ShowLineNumbers(window, newState);
}

static void setAutoIndentAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    if (*nArgs > 0)
// TODO:    {
// TODO:       if (strcmp(args[0], "off") == 0)
// TODO:       {
// TODO:          SetAutoIndent(window, NO_AUTO_INDENT);
// TODO:       }
// TODO:       else if (strcmp(args[0], "on") == 0)
// TODO:       {
// TODO:          SetAutoIndent(window, AUTO_INDENT);
// TODO:       }
// TODO:       else if (strcmp(args[0], "smart") == 0)
// TODO:       {
// TODO:          SetAutoIndent(window, SMART_INDENT);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          fprintf(stderr, "nedit: set_auto_indent invalid argument\n");
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       fprintf(stderr, "nedit: set_auto_indent requires argument\n");
// TODO:    }
}

static void setWrapTextAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE();
   WindowInfo* window = WidgetToWindow(w);
   if (*nArgs > 0)
   {
      if (strcmp(args[0], "none") == 0)
      {
         SetAutoWrap(window, NO_WRAP);
      }
      else if (strcmp(args[0], "auto") == 0)
      {
         SetAutoWrap(window, NEWLINE_WRAP);
      }
      else if (strcmp(args[0], "continuous") == 0)
      {
         SetAutoWrap(window, CONTINUOUS_WRAP);
      }
      else
      {
         fprintf(stderr, "nedit: set_wrap_text invalid argument\n");
      }
   }
   else
   {
      fprintf(stderr, "nedit: set_wrap_text requires argument\n");
   }
}

static void setWrapMarginAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO: 
// TODO:    if (*nArgs > 0)
// TODO:    {
// TODO:       int newMargin = 0;
// TODO:       if (sscanf(args[0], "%d", &newMargin) == 1 && newMargin >= 0 && newMargin < 1000)
// TODO:       {
// TODO:          int i;
// TODO: 
// TODO:          XtVaSetValues(window->textArea, textNwrapMargin, newMargin, NULL);
// TODO:          for (i = 0; i < window->nPanes; ++i)
// TODO:          {
// TODO:             XtVaSetValues(window->textPanes[i], textNwrapMargin, newMargin, NULL);
// TODO:          }
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          fprintf(stderr, "nedit: set_wrap_margin requires integer argument >= 0 and < 1000\n");
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       fprintf(stderr, "nedit: set_wrap_margin requires argument\n");
// TODO:    }
}

static void setHighlightSyntaxAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    bool newState;
// TODO: 
// TODO:    ACTION_BOOL_PARAM_OR_TOGGLE(newState, *nArgs, args, window->highlightSyntax, "set_highlight_syntax");
// TODO: 
// TODO:    if (IsTopDocument(window))
// TODO:       NeToggleButtonSetState(window->highlightItem, newState, false);
// TODO:    window->highlightSyntax = newState;
// TODO:    if (window->highlightSyntax)
// TODO:    {
// TODO:       StartHighlighting(window, true);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       StopHighlighting(window);
// TODO:    }
}

static void setMakeBackupCopyAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    bool newState;
// TODO: 
// TODO:    ACTION_BOOL_PARAM_OR_TOGGLE(newState, *nArgs, args, window->saveOldVersion, "set_make_backup_copy");
// TODO: 
// TODO: #ifndef VMS
// TODO:    if (IsTopDocument(window))
// TODO:       NeToggleButtonSetState(window->saveLastItem, newState, false);
// TODO: #endif
// TODO:    window->saveOldVersion = newState;
}

static void setIncrementalBackupAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    bool newState;
// TODO: 
// TODO:    ACTION_BOOL_PARAM_OR_TOGGLE(newState, *nArgs, args, window->autoSave, "set_incremental_backup");
// TODO: 
// TODO:    if (IsTopDocument(window))
// TODO:       NeToggleButtonSetState(window->autoSaveItem, newState, false);
// TODO:    window->autoSave = newState;
}

static void setShowMatchingAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    if (*nArgs > 0)
// TODO:    {
// TODO:       if (strcmp(args[0], NO_FLASH_STRING) == 0)
// TODO:       {
// TODO:          SetShowMatching(window, NO_FLASH);
// TODO:       }
// TODO:       else if (strcmp(args[0], FLASH_DELIMIT_STRING) == 0)
// TODO:       {
// TODO:          SetShowMatching(window, FLASH_DELIMIT);
// TODO:       }
// TODO:       else if (strcmp(args[0], FLASH_RANGE_STRING) == 0)
// TODO:       {
// TODO:          SetShowMatching(window, FLASH_RANGE);
// TODO:       }
// TODO:       /* For backward compatibility with pre-5.2 versions, we also
// TODO:          accept 0 and 1 as aliases for NO_FLASH and FLASH_DELIMIT.
// TODO:          It is quite unlikely, though, that anyone ever used this
// TODO:          action procedure via the macro language or a key binding,
// TODO:          so this can probably be left out safely. */
// TODO:       else if (strcmp(args[0], "0") == 0)
// TODO:       {
// TODO:          SetShowMatching(window, NO_FLASH);
// TODO:       }
// TODO:       else if (strcmp(args[0], "1") == 0)
// TODO:       {
// TODO:          SetShowMatching(window, FLASH_DELIMIT);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          fprintf(stderr, "nedit: Invalid argument for set_show_matching\n");
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       fprintf(stderr, "nedit: set_show_matching requires argument\n");
// TODO:    }
}

static void setMatchSyntaxBasedAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    bool newState;
// TODO: 
// TODO:    ACTION_BOOL_PARAM_OR_TOGGLE(newState, *nArgs, args, window->matchSyntaxBased, "set_match_syntax_based");
// TODO: 
// TODO:    if (IsTopDocument(window))
// TODO:       NeToggleButtonSetState(window->matchSyntaxBasedItem, newState, false);
// TODO:    window->matchSyntaxBased = newState;
}

static void setOvertypeModeAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    bool newState;
// TODO: 
// TODO:    if (window == NULL)
// TODO:       return;
// TODO: 
// TODO:    ACTION_BOOL_PARAM_OR_TOGGLE(newState, *nArgs, args, window->overstrike, "set_overtype_mode");
// TODO: 
// TODO:    if (IsTopDocument(window))
// TODO:       NeToggleButtonSetState(window->overtypeModeItem, newState, false);
// TODO:    SetOverstrike(window, newState);
}

static void setLockedAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    bool newState;
// TODO: 
// TODO:    ACTION_BOOL_PARAM_OR_TOGGLE(newState, *nArgs, args, IS_USER_LOCKED(window->lockReasons), "set_locked");
// TODO: 
// TODO:    SET_USER_LOCKED(window->lockReasons, newState);
// TODO:    if (IsTopDocument(window))
// TODO:       NeToggleButtonSetState(window->readOnlyItem, IS_ANY_LOCKED(window->lockReasons), false);
// TODO:    UpdateWindowTitle(window);
// TODO:    UpdateWindowReadOnly(window);
}

static void setTabDistAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO: 
// TODO:    if (*nArgs > 0)
// TODO:    {
// TODO:       int newTabDist = 0;
// TODO:       if (sscanf(args[0], "%d", &newTabDist) == 1 &&
// TODO:             newTabDist > 0 &&
// TODO:             newTabDist <= MAX_EXP_CHAR_LEN)
// TODO:       {
// TODO:          SetTabDist(window, newTabDist);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          fprintf(stderr,
// TODO:                  "nedit: set_tab_dist requires integer argument > 0 and <= %d\n",
// TODO:                  MAX_EXP_CHAR_LEN);
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       fprintf(stderr, "nedit: set_tab_dist requires argument\n");
// TODO:    }
}

static void setEmTabDistAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO: 
// TODO:    if (*nArgs > 0)
// TODO:    {
// TODO:       int newEmTabDist = 0;
// TODO:       if (sscanf(args[0], "%d", &newEmTabDist) == 1 &&
// TODO:             newEmTabDist < 1000)
// TODO:       {
// TODO:          if (newEmTabDist < 0)
// TODO:          {
// TODO:             newEmTabDist = 0;
// TODO:          }
// TODO:          SetEmTabDist(window, newEmTabDist);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          fprintf(stderr,
// TODO:                  "nedit: set_em_tab_dist requires integer argument >= -1 and < 1000\n");
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       fprintf(stderr, "nedit: set_em_tab_dist requires integer argument\n");
// TODO:    }
}

static void setUseTabsAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO:    bool newState;
// TODO: 
// TODO:    ACTION_BOOL_PARAM_OR_TOGGLE(newState, *nArgs, args, window->buffer->useTabs, "set_use_tabs");
// TODO: 
// TODO:    window->buffer->useTabs = newState;
}

static void setFontsAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE();
   WindowInfo* window = WidgetToWindow(w);
   if (*nArgs >= 4)
   {
      SetFonts(window, args[0], args[1], args[2], args[3]);
   }
   else
   {
      fprintf(stderr, "nedit: set_fonts requires 4 arguments\n");
   }
}

static void setLanguageModeAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE();
   WindowInfo* window = WidgetToWindow(w);

   if (*nArgs > 0)
   {
      SetLanguageMode(window, FindLanguageMode(args[0]), false);
   }
   else
   {
      fprintf(stderr, "nedit: set_language_mode requires argument\n");
   }
}

/*
** Make sure the close menu item is dimmed appropriately for the current
** set of windows.  It should be dim only for the last Untitled, unmodified,
** editor window, and sensitive otherwise.
*/
void CheckCloseDim()
{
   WindowInfo* window;

   if (WindowList == NULL)
      return;
   if (WindowList->next==NULL && !WindowList->filenameSet && !WindowList->fileChanged)
   {
      NeSetSensitive(WindowList->menuBar->getCloseItem(), false);
      return;
   }

   for (window=WindowList; window!=NULL; window=window->next)
   {
      if (!IsTopDocument(window))
         continue;
      NeSetSensitive(window->menuBar->getCloseItem(), true);
   }
}

/*
** Invalidate the Window menus of all NEdit windows to but don't change
** the menus until they're needed (Originally, this was "UpdateWindowMenus",
** but creating and destroying manu items for every window every time a
** new window was created or something changed, made things move very
** slowly with more than 10 or so windows).
*/
void InvalidateWindowMenus()
{
   // Mark the window menus invalid (to be updated when the user pulls one
   // down), unless the menu is torn off, meaning it is visible to the user
   // and should be updated immediately
   for (WindowInfo* w=WindowList; w!=NULL; w=w->next)
   {
      if (w->type == WindowInfo::Window)
         updateWindowMenu(w);
   }
}

/*
** Mark the Previously Opened Files menus of all NEdit windows as invalid.
** Since actually changing the menus is slow, they're just marked and updated
** when the user pulls one down.
*/
void InvalidatePrevOpenMenus()
{
   // Mark the menus invalid (to be updated when the user pulls one
   // down), unless the menu is torn off, meaning it is visible to the user
   // and should be updated immediately */
   for (WindowInfo* w=WindowList; w!=NULL; w=w->next)
   {
// TODO:       if (!XmIsMenuShell(XtParent(w->prevOpenMenuPane)))
         updatePrevOpenMenu(w);
   }
}

/*
** Add a file to the list of previously opened files for display in the
** File menu.
*/
void AddToPrevOpenMenu(const char* filename)
{
   int i;
   char* nameCopy;
   WindowInfo* w;

   /* If the Open Previous command is disabled, just return */
   if (GetPrefMaxPrevOpenFiles() < 1)
   {
      return;
   }

   /*  Refresh list of previously opened files to avoid Big Race Condition,
   where two sessions overwrite each other's changes in NEdit's
   history file.
   Of course there is still Little Race Condition, which occurs if a
   Session A reads the list, then Session B reads the list and writes
   it before Session A gets a chance to write.  */
   ReadNEditDB();

   /* If the name is already in the list, move it to the start */
   for (i=0; i<NPrevOpen; i++)
   {
      if (!strcmp(filename, PrevOpen[i]))
      {
         nameCopy = PrevOpen[i];
         memmove(&PrevOpen[1], &PrevOpen[0], sizeof(char*) * i);
         PrevOpen[0] = nameCopy;
         InvalidatePrevOpenMenus();
         WriteNEditDB();
         return;
      }
   }

   /* If the list is already full, make room */
   if (NPrevOpen >= GetPrefMaxPrevOpenFiles())
   {
      /*  This is only safe if GetPrefMaxPrevOpenFiles() > 0.  */
      free__(PrevOpen[--NPrevOpen]);
   }

   /* Add it to the list */
   nameCopy = (char*)malloc__(strlen(filename) + 1);
   strcpy(nameCopy, filename);
   memmove(&PrevOpen[1], &PrevOpen[0], sizeof(char*) * NPrevOpen);
   PrevOpen[0] = nameCopy;
   NPrevOpen++;

   /* Mark the Previously Opened Files menu as invalid in all windows */
   InvalidatePrevOpenMenus();

   /* Undim the menu in all windows if it was previously empty */
   if (NPrevOpen > 0)
   {
      for (w=WindowList; w!=NULL; w=w->next)
      {
         if (!IsTopDocument(w))
            continue;
         NeSetSensitive(w->menuBar->getOpenPreviousItem(), true);
      }
   }

   /* Write the menu contents to disk to restore in later sessions */
   WriteNEditDB();
}

static char* getWindowsMenuEntry(const WindowInfo* window)
{
   static char fullTitle[MAXPATHLEN * 2 + 3+ 1];

   sprintf(fullTitle, "%s%s", window->filename, window->fileChanged? "*" : "");

   if (GetPrefShowPathInWindowsMenu() && window->filenameSet)
   {
      strcat(fullTitle, " - ");
      strcat(fullTitle, window->path);
   }

   return(fullTitle);
}

// --------------------------------------------------------------------------
int GetWindowNameMarker(const Ne_MenuBar* menuBar, const char* name)
{
   for(int i = 0; i < menuBar->size(); ++i)
   {
      const Fl_Menu_Item& item = menuBar->menu()[i];
      const char* label  = item.label();
      if (label && strcmp(label, name) ==0)
         return i;
   }
   return -1;
}

int GetWindowNameStartMarker(const Ne_MenuBar* menuBar) { return GetWindowNameMarker(menuBar, "WindowNameStart"); }
int GetWindowNameEndMarker(const Ne_MenuBar* menuBar) { return GetWindowNameMarker(menuBar, "WindowNameEnd"); }

// --------------------------------------------------------------------------
void clearAllWindowNames(Ne_MenuBar* menuBar)
{
   int startIndex = GetWindowNameStartMarker(menuBar);
   int endIndex = GetWindowNameEndMarker(menuBar);
   
   // BUG ? if the end marker item is delete, then, it's remove with the previous item on the list
   ((Fl_Menu_Item*)menuBar->menu())[endIndex].flags = menuBar->menu()[endIndex].flags & !FL_MENU_INVISIBLE;
   while( endIndex != startIndex + 1)
   {
      menuBar->remove(startIndex+1);
      //DisplayMenuInfo();
      endIndex = GetWindowNameEndMarker(menuBar);
   }
   // Back to invisibility
   ((Fl_Menu_Item*)menuBar->menu())[endIndex].flags = menuBar->menu()[endIndex].flags | FL_MENU_INVISIBLE;
}

/*
** Update the Window menu of a single window to reflect the current state of
** all NEdit windows as determined by the global WindowList.
*/
static void updateWindowMenu(const WindowInfo* window)
{
   WindowInfo* w;
   int i, n, nWindows, windowIndex;
   WindowInfo** windows;

   if (!IsTopDocument(window))
      return;

   // Make a sorted list of windows
   for (w=WindowList, nWindows=0; w!=NULL; w=w->next, nWindows++);
   windows = (WindowInfo**)malloc__(sizeof(WindowInfo*) * nWindows);
   for (w=WindowList, i=0; w!=NULL; w=w->next, i++)
      windows[i] = w;
   qsort(windows, nWindows, sizeof(WindowInfo*), compareWindowNames);

   clearAllWindowNames(window->menuBar);

   // Add new items for the titles of the remaining windows to the menu
   for (int windowIndex=0; windowIndex<nWindows;++windowIndex)
   {
      std::string title = getWindowsMenuEntry(windows[windowIndex]);
      for(std::string::size_type i = 0; i < title.size(); ++i)
      {
         if (title[i] == '/')
         {
            title.insert(i, 1, '\\'); ++i;
         }
      }

      int index = GetWindowNameEndMarker(window->menuBar);
      window->menuBar->insert(index, title.c_str(), 0, raiseCB, windows[windowIndex]);
   }
   free__((char*)windows);
}

// Update the Previously Opened Files menu of a single window to reflect the
// current state of the list as retrieved from FIXME.
// Thanks to Markus Schwarzenberg for the sorting part.
static void updatePrevOpenMenu(WindowInfo* window)
{
   // Read history file to get entries written by other sessions.
   ReadNEditDB();

   // Sort the previously opened file list if requested
   char** prevOpenSorted = new char*[NPrevOpen];
   memcpy(prevOpenSorted, PrevOpen, NPrevOpen * sizeof(char*));
   if (GetPrefSortOpenPrevMenu())
      qsort(prevOpenSorted, NPrevOpen, sizeof(char*), cmpStrPtr);

   // Remove the previous submenu
   Ne_MenuBar* menuBar = window->menuBar;
   int openPreviousItemIndex = menuBar->find_index(menuBar->getOpenPreviousItem());
   window->menuBar->clear_submenu(openPreviousItemIndex);

   // Add new items for the remaining file names to the menu
   for (int index = 0; index<NPrevOpen; ++index)
   {
      std::string itemName = prevOpenSorted[index];
      for(std::string::size_type found = 0; (found = itemName.find('/', found)) != std::string::npos; found += 2)
         itemName.insert(found, 1, '\\'); 
      menuBar->insert(openPreviousItemIndex+1, itemName.c_str(), 0, openPrevCB, window);
   }

   delete[] prevOpenSorted;
}

// TODO: /*
// TODO: ** This function manages the display of the Tags File Menu, which is displayed
// TODO: ** when the user selects Un-load Tags File.
// TODO: */
// TODO: static void updateTagsFileMenu(WindowInfo* window)
// TODO: {
// TODO:    tagFile* tf;
// TODO:    Fl_Widget* btn;
// TODO:    WidgetList items;
// TODO:    Cardinal nItems;
// TODO:    int n;
// TODO:    NeString st1;
// TODO: 
// TODO:    /* Go thru all of the items in the menu and rename them to match the file
// TODO:       list.  In older Motifs (particularly ibm), it was dangerous to replace
// TODO:       a whole menu pane, which would be much simpler.  However, since the
// TODO:       code was already written for the Windows menu and is well tested, I'll
// TODO:       stick with this weird method of re-naming the items */
// TODO:    XtVaGetValues(window->unloadTagsMenuPane, XmNchildren, &items,
// TODO:                  XmNnumChildren, &nItems, NULL);
// TODO:    tf = TagsFileList;
// TODO:    for (n=0; n<(int)nItems; n++)
// TODO:    {
// TODO:       if (!tf)
// TODO:       {
// TODO:          /* unmanaging before destroying stops parent from displaying */
// TODO:          XtUnmanageChild(items[n]);
// TODO:          XtDestroyWidget(items[n]);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          XtVaSetValues(items[n], XmNlabelString,
// TODO:                        st1=NeNewString(tf->filename), NULL);
// TODO:          XtRemoveAllCallbacks(items[n], XmNactivateCallback);
// TODO:          XtAddCallback(items[n], XmNactivateCallback,
// TODO:                        (XtCallbackProc)unloadTagsFileCB, tf->filename);
// TODO:          NeStringFree(st1);
// TODO:          tf = tf->next;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* Add new items for the remaining file names to the menu */
// TODO:    while (tf)
// TODO:    {
// TODO:       btn = XtVaCreateManagedWidget("win", xmPushButtonWidgetClass,
// TODO:                                     window->unloadTagsMenuPane, XmNlabelString,
// TODO:                                     st1=NeNewString(tf->filename),XmNmarginHeight, 0,
// TODO:                                     XmNuserData, TEMPORARY_MENU_ITEM, NULL);
// TODO:       XtAddCallback(btn, XmNactivateCallback,
// TODO:                     (XtCallbackProc)unloadTagsFileCB, tf->filename);
// TODO:       NeStringFree(st1);
// TODO:       tf = tf->next;
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** This function manages the display of the Tips File Menu, which is displayed
// TODO: ** when the user selects Un-load Calltips File.
// TODO: */
// TODO: static void updateTipsFileMenu(WindowInfo* window)
// TODO: {
// TODO:    tagFile* tf;
// TODO:    Fl_Widget* btn;
// TODO:    WidgetList items;
// TODO:    Cardinal nItems;
// TODO:    int n;
// TODO:    NeString st1;
// TODO: 
// TODO:    /* Go thru all of the items in the menu and rename them to match the file
// TODO:       list.  In older Motifs (particularly ibm), it was dangerous to replace
// TODO:       a whole menu pane, which would be much simpler.  However, since the
// TODO:       code was already written for the Windows menu and is well tested, I'll
// TODO:       stick with this weird method of re-naming the items */
// TODO:    XtVaGetValues(window->unloadTipsMenuPane, XmNchildren, &items,
// TODO:                  XmNnumChildren, &nItems, NULL);
// TODO:    tf = TipsFileList;
// TODO:    for (n=0; n<(int)nItems; n++)
// TODO:    {
// TODO:       if (!tf)
// TODO:       {
// TODO:          /* unmanaging before destroying stops parent from displaying */
// TODO:          XtUnmanageChild(items[n]);
// TODO:          XtDestroyWidget(items[n]);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          XtVaSetValues(items[n], XmNlabelString,
// TODO:                        st1=NeNewString(tf->filename), NULL);
// TODO:          XtRemoveAllCallbacks(items[n], XmNactivateCallback);
// TODO:          XtAddCallback(items[n], XmNactivateCallback,
// TODO:                        (XtCallbackProc)unloadTipsFileCB, tf->filename);
// TODO:          NeStringFree(st1);
// TODO:          tf = tf->next;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* Add new items for the remaining file names to the menu */
// TODO:    while (tf)
// TODO:    {
// TODO:       btn = XtVaCreateManagedWidget("win", xmPushButtonWidgetClass,
// TODO:                                     window->unloadTipsMenuPane, XmNlabelString,
// TODO:                                     st1=NeNewString(tf->filename),XmNmarginHeight, 0,
// TODO:                                     XmNuserData, TEMPORARY_MENU_ITEM, NULL);
// TODO:       XtAddCallback(btn, XmNactivateCallback,
// TODO:                     (XtCallbackProc)unloadTipsFileCB, tf->filename);
// TODO:       NeStringFree(st1);
// TODO:       tf = tf->next;
// TODO:    }
// TODO: }

/*
** Comparison function for sorting file names for the Open Previous submenu
*/
static int cmpStrPtr(const void* strA, const void* strB)
{
   return strcmp(*((char**)strA), *((char**)strB));
}

static char neditDBBadFilenameChars[] = "\n";

/*
** Write dynamic database of file names for "Open Previous".  Eventually,
** this may hold window positions, and possibly file marks, in which case,
** it should be moved to a different module, but for now it's just a list
** of previously opened files.
*/
void WriteNEditDB()
{
   const char* fullName = GetRCFileName(NEDIT_HISTORY);
   FILE* fp;
   int i;
   static char fileHeader[] =
      "# File name database for NEdit Open Previous command\n";

   if (fullName == NULL)
   {
      /*  GetRCFileName() might return NULL if an error occurs during
      creation of the preference file directory. */
      return;
   }

   /* If the Open Previous command is disabled, just return */
   if (GetPrefMaxPrevOpenFiles() < 1)
   {
      return;
   }

   /* open the file */
   if ((fp = fopen(fullName, "w")) == NULL)
   {
#ifdef VMS
      /* When the version number, ";1" is specified as part of the file
      name, fopen(fullName, "w"), will only open for writing if the
      file does not exist. Using, fopen(fullName, "r+"), opens an
      existing file for "update" - read/write pointer is placed at the
      beginning of file.
      By calling ftruncate(), we discard the old contents and avoid
      trailing garbage in the file if the new contents is shorter. */
      if ((fp = fopen(fullName, "r+")) == NULL)
      {
         return;
      }
      if (ftruncate(fileno(fp), 0) != 0)
      {
         fclose(fp);
         return;
      }
#else
      return;
#endif
   }

   /* write the file header text to the file */
   fprintf(fp, "%s", fileHeader);

   /* Write the list of file names */
   for (i = 0; i < NPrevOpen; ++i)
   {
      size_t lineLen = strlen(PrevOpen[i]);

      if (lineLen > 0 && PrevOpen[i][0] != '#' &&
         strcspn(PrevOpen[i], neditDBBadFilenameChars) == lineLen)
      {
         fprintf(fp, "%s\n", PrevOpen[i]);
      }
   }

   fclose(fp);
}

/*
**  Read database of file names for 'Open Previous' submenu.
**
**  Eventually, this may hold window positions, and possibly file marks (in
**  which case it should be moved to a different module) but for now it's
**  just a list of previously opened files.
**
**  This list is read once at startup and potentially refreshed before a
**  new entry is about to be written to the file or before the menu is
**  displayed. If the file is modified since the last read (or not read
**  before), it is read in, otherwise nothing is done.
*/
void ReadNEditDB()
{
   const char* fullName = GetRCFileName(NEDIT_HISTORY);
   char line[MAXPATHLEN + 2];
   char* nameCopy;
   struct stat attribute;
   FILE* fp;
   size_t lineLen;
   static time_t lastNeditdbModTime = 0;

   /*  If the Open Previous command is disabled or the user set the
   resource to an (invalid) negative value, just return.  */
   if (GetPrefMaxPrevOpenFiles() < 1)
   {
      return;
   }

   /* Initialize the files list and allocate a (permanent) block memory
   of the size prescribed by the maxPrevOpenFiles resource */
   if (!PrevOpen)
   {
      PrevOpen = new char*[GetPrefMaxPrevOpenFiles()];
      NPrevOpen = 0;
   }

   /* Don't move this check ahead of the previous statements. PrevOpen
   must be initialized at all times. */
   if (fullName == NULL)
   {
      /*  GetRCFileName() might return NULL if an error occurs during
      creation of the preference file directory. */
      return;
   }

   /*  Stat history file to see whether someone touched it after this
   session last changed it.  */
   if (0 == stat(fullName, &attribute))
   {
      if (lastNeditdbModTime >= attribute.st_mtime)
      {
         /*  Do nothing, history file is unchanged.  */
         return;
      }
      else
      {
         /*  Memorize modtime to compare to next time.  */
         lastNeditdbModTime = attribute.st_mtime;
      }
   }
   else
   {
      /*  stat() failed, probably for non-exiting history database.  */
      if (ENOENT != errno)
      {
         perror("nedit: Error reading history database");
      }
      return;
   }

   /* open the file */
   if ((fp = fopen(fullName, "r")) == NULL)
   {
      return;
   }

   /*  Clear previous list.  */
   while (0 != NPrevOpen)
   {
      free__(PrevOpen[--NPrevOpen]);
   }

   /* read lines of the file, lines beginning with # are considered to be
   comments and are thrown away.  Lines are subject to cursory checking,
   then just copied to the Open Previous file menu list */
   while (true)
   {
      if (fgets(line, sizeof(line), fp) == NULL)
      {
         /* end of file */
         fclose(fp);
         return;
      }
      if (line[0] == '#')
      {
         /* comment */
         continue;
      }
      lineLen = strlen(line);
      if (lineLen == 0)
      {
         /* blank line */
         continue;
      }
      if (line[lineLen - 1] != '\n')
      {
         /* no newline, probably truncated */
         fprintf(stderr, "nedit: Line too long in history file\n");
         while (fgets(line, sizeof(line), fp) != NULL)
         {
            lineLen = strlen(line);
            if (lineLen > 0 && line[lineLen - 1] == '\n')
            {
               break;
            }
         }
         continue;
      }
      line[--lineLen] = '\0';
      if (strcspn(line, neditDBBadFilenameChars) != lineLen)
      {
         /* non-filename characters */
         fprintf(stderr, "nedit: History file may be corrupted\n");
         continue;
      }
      nameCopy = new char[lineLen + 1];
      strcpy(nameCopy, line);
      PrevOpen[NPrevOpen++] = nameCopy;
      if (NPrevOpen >= GetPrefMaxPrevOpenFiles())
      {
         /* too many entries */
         fclose(fp);
         return;
      }
   }
}

// TODO: static void setWindowSizeDefault(int rows, int cols)
// TODO: {
// TODO:    SetPrefRows(rows);
// TODO:    SetPrefCols(cols);
// TODO:    updateWindowSizeMenus();
// TODO: }

static void updateWindowSizeMenus()
{
   WindowInfo* win;

   for (win=WindowList; win!=NULL; win=win->next)
      updateWindowSizeMenu(win);
}

static void updateWindowSizeMenu(WindowInfo* win)
{
   int rows = GetPrefRows(), cols = GetPrefCols();

   if (!IsTopDocument(win))
      return;

   win->menuBar->set_checked("24X80", rows==24&&cols==80);
   win->menuBar->set_checked("40X80", rows==40&&cols==80);
   win->menuBar->set_checked("60X80", rows==60&&cols==80);
   win->menuBar->set_checked("80X80", rows==24&&cols==80);
   if ((rows!=24 && rows!=40 && rows!=60 && rows!=80) || cols!=80)
   {
      win->menuBar->set_checked("customSize", true);
      char title[50] = "";
      sprintf(title, "Custom... (%d x %d)", rows, cols);
      win->menuBar->replace("customSize", title);
   }
   else
   {
      win->menuBar->set_checked("customSize", false);
      win->menuBar->replace("customSize", "Custom...");
   }
}

/*
** Scans action argument list for arguments "forward" or "backward" to
** determine search direction for search and replace actions.  "ignoreArgs"
** tells the routine how many required arguments there are to ignore before
** looking for keywords
*/
static int searchDirection(int ignoreArgs, const char** args, int* nArgs)
{
   int i;

   for (i=ignoreArgs; i<(int)*nArgs; i++)
   {
      if (!strCaseCmp(args[i], "forward"))
         return SEARCH_FORWARD;
      if (!strCaseCmp(args[i], "backward"))
         return SEARCH_BACKWARD;
   }
   return SEARCH_FORWARD;
}

/*
** Scans action argument list for arguments "keep" or "nokeep" to
** determine whether to keep dialogs up for search and replace.  "ignoreArgs"
** tells the routine how many required arguments there are to ignore before
** looking for keywords
*/
static int searchKeepDialogs(int ignoreArgs, const char** args, int* nArgs)
{
   int i;

   for (i=ignoreArgs; i<(int)*nArgs; i++)
   {
      if (!strCaseCmp(args[i], "keep"))
         return true;
      if (!strCaseCmp(args[i], "nokeep"))
         return false;
   }
   return GetPrefKeepSearchDlogs();
}

/*
** Scans action argument list for arguments "wrap" or "nowrap" to
** determine search direction for search and replace actions.  "ignoreArgs"
** tells the routine how many required arguments there are to ignore before
** looking for keywords
*/
static int searchWrap(int ignoreArgs, const char** args, int* nArgs)
{
   int i;

   for (i=ignoreArgs; i<(int)*nArgs; i++)
   {
      if (!strCaseCmp(args[i], "wrap"))
         return(true);
      if (!strCaseCmp(args[i], "nowrap"))
         return(false);
   }
   return GetPrefSearchWraps();
}

/*
** Scans action argument list for arguments "literal", "case" or "regex" to
** determine search type for search and replace actions.  "ignoreArgs"
** tells the routine how many required arguments there are to ignore before
** looking for keywords
*/
static int searchType(int ignoreArgs, const char** args, int* nArgs)
{
   int i, tmpSearchType;

   for (i=ignoreArgs; i<(int)*nArgs; i++)
   {
      if (StringToSearchType(args[i], &tmpSearchType))
         return tmpSearchType;
   }
   return GetPrefSearch();
}

/*
** Return a pointer to the string describing search direction for search action
** routine parameters given a callback XmAnyCallbackStruct pointed to by
** "callData", by checking if the shift key is pressed (for search callbacks).
*/
static const char** shiftKeyToDir(bool isShiftPressed)
{
   static const char* backwardParam[1] = {"backward"};
   static const char* forwardParam[1] = {"forward"};
   if (isShiftPressed)
      return backwardParam;
   return forwardParam;
}

static void raiseCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = (WindowInfo*)data;
   RaiseFocusDocumentWindow(window, true /* always focus */);
}

static void openPrevCB(Fl_Widget* w, void* data)
{
   TRACE();
   const char* params[1];
   Fl_Widget* menu = w;

   params[0] = ((Ne_MenuBar*)w)->mvalue()->label();
   AppContext.callAction(WidgetToWindow(menu)->lastFocus, "open", Fl::event(), params, 1);
   CheckCloseDim();
}

// TODO: static void unloadTagsFileCB(Fl_Widget* w, char* name, caddr_t callData)
// TODO: {
// TODO:    char* params[1];
// TODO:    Fl_Widget* menu = w;
// TODO: 
// TODO:    HidePointerOnKeyedEvent(WidgetToWindow(w)->lastFocus,
// TODO:                            ((XmAnyCallbackStruct*)callData)->event);
// TODO:    params[0] = name;
// TODO:    XtCallActionProc(WidgetToWindow(menu)->lastFocus, "unload_tags_file",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, params, 1);
// TODO: }
// TODO: 
// TODO: static void unloadTipsFileCB(Fl_Widget* w, char* name, caddr_t callData)
// TODO: {
// TODO:    char* params[1];
// TODO: #if XmVersion >= 1002
// TODO:    Fl_Widget* menu = XmGetPostedFromWidget(XtParent(w)); /* If menu is torn off */
// TODO: #else
// TODO:    Fl_Widget* menu = w;
// TODO: #endif
// TODO: 
// TODO:    params[0] = name;
// TODO:    XtCallActionProc(WidgetToWindow(menu)->lastFocus, "unload_tips_file",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, params, 1);
// TODO: }

/*
** strCaseCmp compares its arguments and returns 0 if the two strings
** are equal IGNORING case differences.  Otherwise returns 1.
*/
static int strCaseCmp(const char* str1, const char* str2)
{
   const char* c1, *c2;

   for (c1=str1, c2=str2; *c1!='\0' && *c2!='\0'; c1++, c2++)
      if (toupper((unsigned char)*c1) != toupper((unsigned char)*c2))
         return 1;
   if (*c1 == *c2)
   {
      return(0);
   }
   else
   {
      return(1);
   }
}

/*
** Comparison function for sorting windows by title for the window menu.
** Windows are sorted by Untitled and then alphabetically by filename and
** then alphabetically by path.
*/
static int compareWindowNames(const void* windowA, const void* windowB)
{
   int rc;
   const WindowInfo* a = *((WindowInfo**)windowA);
   const WindowInfo* b = *((WindowInfo**)windowB);
   /* Untitled first */
   rc = a->filenameSet ==  b->filenameSet ? 0 :
      a->filenameSet && !b->filenameSet ? 1 : -1;
   if (rc != 0)
      return rc;
   rc = strcmp(a->filename, b->filename);
   if (rc != 0)
      return rc;
   rc = strcmp(a->path, b->path);
   return rc;
}

// TODO: /*
// TODO: ** Create popup for right button programmable menu
// TODO: */
// TODO: Fl_Widget* CreateBGMenu(WindowInfo* window)
// TODO: {
// TODO:    Arg args[1];
// TODO: 
// TODO:    /* There is still some mystery here.  It's important to get the XmNmenuPost
// TODO:       resource set to the correct menu button, or the menu will not post
// TODO:       properly, but there's also some danger that it will take over the entire
// TODO:       button and interfere with text widget translations which use the button
// TODO:       with modifiers.  I don't entirely understand why it works properly now
// TODO:       when it failed often in development, and certainly ignores the ~ syntax
// TODO:       in translation event specifications. */
// TODO:    XtSetArg(args[0], XmNmenuPost, GetPrefBGMenuBtn());
// TODO:    return CreatePopupMenu(window->textArea, "bgMenu", args, 1);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Create context popup menu for tabs & tab bar
// TODO: */
// TODO: Fl_Widget* CreateTabContextMenu(Fl_Widget* parent, WindowInfo* window)
// TODO: {
// TODO:    Fl_Widget*   menu;
// TODO:    Arg      args[8];
// TODO:    int      n;
// TODO: 
// TODO:    n = 0;
// TODO:    XtSetArg(args[n], XmNtearOffModel, XmTEAR_OFF_DISABLED);
// TODO:    n++;
// TODO:    menu = CreatePopupMenu(parent, "tabContext", args, n);
// TODO: 
// TODO:    createMenuItem(menu, "new", "New Tab", 0, doTabActionCB, "new_tab");
// TODO:    createMenuItem(menu, "close", "Close Tab", 0, doTabActionCB, "close");
// TODO:    createMenuSeparator(menu, "sep1");
// TODO:    window->contextDetachDocumentItem = createMenuItem(menu, "detach",
// TODO:                                        "Detach Tab", 0, doTabActionCB, "detach_document");
// TODO:    NeSetSensitive(window->contextDetachDocumentItem, false);
// TODO:    window->contextMoveDocumentItem = createMenuItem(menu, "attach",
// TODO:                                      "Move Tab To...", 0, doTabActionCB, "move_document_dialog");
// TODO: 
// TODO:    return menu;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Add a translation to the text widget to trigger the background menu using
// TODO: ** the mouse-button + modifier combination specified in the resource:
// TODO: ** nedit.bgMenuBtn.
// TODO: */
// TODO: void AddBGMenuAction(Fl_Widget* widget)
// TODO: {
// TODO:    static XtTranslations table = NULL;
// TODO: 
// TODO:    if (table == NULL)
// TODO:    {
// TODO:       char translations[MAX_ACCEL_LEN + 25];
// TODO:       sprintf(translations, "%s: post_window_bg_menu()\n",GetPrefBGMenuBtn());
// TODO:       table = XtParseTranslationTable(translations);
// TODO:    }
// TODO:    XtOverrideTranslations(widget, table);
// TODO: }

static void bgMenuPostAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO: 
// TODO:    /* The Motif popup handling code BLOCKS events while the menu is posted,
// TODO:       including the matching btn-up events which complete various dragging
// TODO:       operations which it may interrupt.  Cancel to head off problems */
// TODO:    XtCallActionProc(window->lastFocus, "process_cancel", event, NULL, 0);
// TODO: 
// TODO:    /* Pop up the menu */
// TODO:    XmMenuPosition(window->bgMenuPane, (XButtonPressedEvent*)event);
// TODO:    XtManageChild(window->bgMenuPane);
// TODO: 
// TODO:    /*
// TODO:       These statements have been here for a very long time, but seem
// TODO:       unnecessary and are even dangerous: when any of the lock keys are on,
// TODO:       Motif thinks it shouldn't display the background menu, but this
// TODO:       callback is called anyway. When we then grab the focus and force the
// TODO:       menu to be drawn, bad things can happen (like a total lockup of the X
// TODO:       server).
// TODO: 
// TODO:       XtPopup(XtParent(window->bgMenuPane), XtGrabNonexclusive);
// TODO:       XtMapWidget(XtParent(window->bgMenuPane));
// TODO:       XtMapWidget(window->bgMenuPane);
// TODO:    */
// TODO: }
// TODO: 
// TODO: void AddTabContextMenuAction(Fl_Widget* widget)
// TODO: {
// TODO:    static XtTranslations table = NULL;
// TODO: 
// TODO:    if (table == NULL)
// TODO:    {
// TODO:       char* translations = "<Btn3Down>: post_tab_context_menu()\n";
// TODO:       table = XtParseTranslationTable(translations);
// TODO:    }
// TODO:    XtOverrideTranslations(widget, table);
}

/*
** action procedure for posting context menu of tabs
*/
static void tabMenuPostAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    WindowInfo* window;
// TODO:    XButtonPressedEvent* xbutton = (XButtonPressedEvent*)event;
// TODO:    Fl_Widget* wgt;
// TODO: 
// TODO:    /* Determine if the context menu was called from tabs or gutter,
// TODO:       then stored the corresponding window info as userData of
// TODO:       the popup menu pane, which will later be extracted by
// TODO:       doTabActionCB() to act upon. When the context menu was called
// TODO:       from the gutter, the active doc is assumed.
// TODO: 
// TODO:       Lesstif requires the action [to pupop the menu] to also be
// TODO:       to the tabs, else nothing happed when right-click on tabs.
// TODO:       Even so, the action procedure sometime appear to be called
// TODO:       from the gutter even if users did right-click on the tabs.
// TODO:       Here we try to cater for the uncertainty. */
// TODO:    if (XtClass(w) == xrwsBubbleButtonWidgetClass)
// TODO:       window = TabToWindow(w);
// TODO:    else if (xbutton->subwindow)
// TODO:    {
// TODO:       wgt = XtWindowToWidget(XtDisplay(w), xbutton->subwindow);
// TODO:       window = TabToWindow(wgt);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       window = WidgetToWindow(w);
// TODO:    }
// TODO:    XtVaSetValues(window->tabMenuPane, XmNuserData, (XtPointer)window, NULL);
// TODO: 
// TODO:    /* The Motif popup handling code BLOCKS events while the menu is posted,
// TODO:       including the matching btn-up events which complete various dragging
// TODO:       operations which it may interrupt.  Cancel to head off problems */
// TODO:    XtCallActionProc(window->lastFocus, "process_cancel", event, NULL, 0);
// TODO: 
// TODO:    /* Pop up the menu */
// TODO:    XmMenuPosition(window->tabMenuPane, (XButtonPressedEvent*)event);
// TODO:    XtManageChild(window->tabMenuPane);
}

// TODO: /*
// TODO: ** Event handler for restoring the input hint of menu tearoffs
// TODO: ** previously disabled in ShowHiddenTearOff()
// TODO: */
// TODO: static void tearoffMappedCB(Fl_Widget* w, XtPointer clientData, XUnmapEvent* event)
// TODO: {
// TODO:    Fl_Widget* shell = (Fl_Widget*)clientData;
// TODO:    XWMHints* wmHints;
// TODO: 
// TODO:    if (event->type != MapNotify)
// TODO:       return;
// TODO: 
// TODO:    /* restore the input hint previously disabled in ShowHiddenTearOff() */
// TODO:    wmHints = XGetWMHints(TheDisplay, XtWindow(shell));
// TODO:    wmHints->input = true;
// TODO:    wmHints->flags |= InputHint;
// TODO:    XSetWMHints(TheDisplay, XtWindow(shell), wmHints);
// TODO:    XFree(wmHints);
// TODO: 
// TODO:    /* we only need to do this only */
// TODO:    XtRemoveEventHandler(shell, StructureNotifyMask, false,
// TODO:                         (XtEventHandler)tearoffMappedCB, shell);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Redisplay (map) a hidden tearoff
// TODO: */
// TODO: void ShowHiddenTearOff(Fl_Widget* menuPane)
// TODO: {
// TODO:    Fl_Widget* shell;
// TODO: 
// TODO:    if (!menuPane)
// TODO:       return;
// TODO: 
// TODO:    shell = XtParent(menuPane);
// TODO:    if (!XmIsMenuShell(shell))
// TODO:    {
// TODO:       XWindowAttributes winAttr;
// TODO: 
// TODO:       XGetWindowAttributes(XtDisplay(shell), XtWindow(shell), &winAttr);
// TODO:       if (winAttr.map_state == IsUnmapped)
// TODO:       {
// TODO:          XWMHints* wmHints;
// TODO: 
// TODO:          /* to workaround a problem where the remapped tearoffs
// TODO:             always receive the input focus insteads of the text
// TODO:             editing window, we disable the input hint of the
// TODO:             tearoff shell temporarily. */
// TODO:          wmHints = XGetWMHints(XtDisplay(shell), XtWindow(shell));
// TODO:          wmHints->input = false;
// TODO:          wmHints->flags |= InputHint;
// TODO:          XSetWMHints(XtDisplay(shell), XtWindow(shell), wmHints);
// TODO:          XFree(wmHints);
// TODO: 
// TODO:          /* show the tearoff */
// TODO:          XtMapWidget(shell);
// TODO: 
// TODO:          /* the input hint will be restored when the tearoff
// TODO:          is mapped */
// TODO:          XtAddEventHandler(shell, StructureNotifyMask, false,
// TODO:                            (XtEventHandler)tearoffMappedCB, shell);
// TODO:       }
// TODO:    }
// TODO: }
// TODO: 
