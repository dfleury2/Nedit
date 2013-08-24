static const char CVSID[] = "$Id: macro.c,v 1.116 2008/08/20 14:57:35 lebert Exp $";
/*******************************************************************************
*                                                                              *
* macro.c -- Macro file processing, learn/replay, and built-in macro           *
*            subroutines                                                       *
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
* April, 1997                                                                  *
*                                                                              *
* Written by Mark Edel                                                         *
*                                                                              *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "macro.h"
#include "Ne_Text_Buffer.h"
#include "Ne_Text_Editor.h"
#include "nedit.h"
#include "window.h"
#include "preferences.h"
#include "interpret.h"
#include "parse.h"
#include "search.h"
#include "server.h"
#include "shell.h"
#include "smartIndent.h"
#include "userCmds.h"
#include "selection.h"
#include "rbTree.h"
#include "tags.h"
#include "calltips.h"
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "../util/fileUtils.h"
#include "../util/utils.h"
#include "../util/getfiles.h"
#include "highlight.h"
#include "highlightData.h"
#include "Ne_Rangeset.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <sys/param.h>
#endif // WIN32
#include <fcntl.h>

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

#include <FL/fl_ask.H>

/* Maximum number of actions in a macro and args in
   an action (to simplify the reader) */
#define MAX_MACRO_ACTIONS 1024
#define MAX_ACTION_ARGS 40

// How long to wait (msec) before putting up Macro Command banner 
#define BANNER_WAIT_TIME 6000

// The following definitions cause an exit from the macro with a message 
// added if (1) to remove compiler warnings on solaris 
#define M_FAILURE(s)  do { *errMsg = s; if (1) return false; } while (0)
#define M_STR_ALLOC_ASSERT(xDV) do { if (xDV.tag == STRING_TAG && !xDV.val.str.rep) { *errMsg = "Failed to allocate value: %s"; return(false); } } while (0)
#define M_ARRAY_INSERT_FAILURE() M_FAILURE("array element failed to insert: %s")

/* Data attached to window during shell command execution with
   information for controling and communicating with the process */
typedef struct
{
   int bannerTimeoutID;
   int continueWorkProcID;
   char bannerIsUp;
   char closeOnCompletion;
   Program* program;
   RestartData* context;
   Fl_Widget* dialog;
} macroCmdInfo;

// Widgets and global data for Repeat dialog 
struct repeatDialog
{
   WindowInfo* forWindow;
   char* lastCommand;
   Fl_Widget* shell, *repeatText, *lastCmdToggle;
   Fl_Widget* inSelToggle, *toEndToggle;
} ;

static void cancelLearn();
static void runMacro(WindowInfo* window, Program* prog);
static void finishMacroCmdExecution(WindowInfo* window);
static void repeatOKCB(Fl_Widget* w, void* data);
static void repeatApplyCB(Fl_Widget* w, void* data);
static int doRepeatDialogAction(repeatDialog* rd, int event);
static void repeatCancelCB(Fl_Widget* w, void* data);
static void repeatDestroyCB(Fl_Widget* w, void* data);
static void learnActionHook(Fl_Widget* w, void* clientData, const char* actionName, int event, const char** params, int* numParams);
static void lastActionHook(Fl_Widget* w, void* clientData, const char* actionName, int event, const char** params, int* numParams);
static char* actionToString(Fl_Widget* w, const char* actionName, int event, const char** params, int numParams);
static int isMouseAction(const char* action);
static int isRedundantAction(const char* action);
static int isIgnoredAction(const char* action);
static int readCheckMacroString(Fl_Widget* dialogParent, char* string, WindowInfo* runWindow, const char* errIn, char** errPos);
static void bannerTimeoutProc(void* clientData, int* id);
static bool continueWorkProc(void* clientData);
static int escapeStringChars(char* fromString, char* toString);
static int escapedStringLength(char* string);
static int lengthMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int minMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int maxMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int focusWindowMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int getRangeMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int getCharacterMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int replaceRangeMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int replaceSelectionMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int getSelectionMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int validNumberMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int replaceInStringMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int replaceSubstringMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int readFileMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int writeFileMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int appendFileMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int writeOrAppendFile(int append, WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int substringMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int toupperMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int tolowerMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int stringToClipboardMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int clipboardToStringMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int searchMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int searchStringMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int setCursorPosMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int beepMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int selectMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int selectRectangleMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int tPrintMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int getenvMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int shellCmdMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int dialogMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static void dialogBtnCB(Fl_Widget* w, void* data);
static void dialogCloseCB(Fl_Widget* w, void* data);
#ifdef LESSTIF_VERSION
static void dialogEscCB(Fl_Widget* w, void* clientData, int event,
                        bool* cont);
#endif // LESSTIF_VERSION 
static int stringDialogMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static void stringDialogBtnCB(Fl_Widget* w, void* data);
static void stringDialogCloseCB(Fl_Widget* w, void* data);
#ifdef LESSTIF_VERSION
static void stringDialogEscCB(Fl_Widget* w, void* clientData, int event,
                              bool* cont);
#endif // LESSTIF_VERSION 
static int calltipMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int killCalltipMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
// T Balinski 
static int listDialogMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static void listDialogBtnCB(Fl_Widget* w, void* data);
static void listDialogCloseCB(Fl_Widget* w, void* data);
// T Balinski End 
#ifdef LESSTIF_VERSION
static void listDialogEscCB(Fl_Widget* w, void* clientData, int event,
                            bool* cont);
#endif // LESSTIF_VERSION 
static int stringCompareMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int splitMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
/* DISASBLED for 5.4
static int setBacklightStringMS(WindowInfo *window, DataValue *argList,
	int nArgs, DataValue *result, char **errMsg);
*/
static int cursorMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int lineMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int columnMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int fileNameMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int filePathMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int lengthMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int selectionStartMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int selectionEndMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int selectionLeftMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int selectionRightMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int statisticsLineMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int incSearchLineMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int showLineNumbersMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int autoIndentMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int wrapTextMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int highlightSyntaxMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int makeBackupCopyMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int incBackupMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int showMatchingMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int matchSyntaxBasedMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int overTypeModeMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int readOnlyMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int lockedMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int fileFormatMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int fontNameMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int fontNameItalicMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int fontNameBoldMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int fontNameBoldItalicMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int subscriptSepMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int minFontWidthMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int maxFontWidthMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int wrapMarginMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int topLineMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int numDisplayLinesMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int displayWidthMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int activePaneMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int nPanesMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int emptyArrayMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int serverNameMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int tabDistMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int emTabDistMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int useTabsMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int modifiedMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int languageModeMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int calltipIDMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int readSearchArgs(DataValue* argList, int nArgs, int* searchDirection, int* searchType, int* wrap, char** errMsg);
static int wrongNArgsErr(char** errMsg);
static int tooFewArgsErr(char** errMsg);
static int strCaseCmp(char* str1, char* str2);
static int readIntArg(DataValue dv, int* result, char** errMsg);
static int readStringArg(DataValue dv, char** result, char* stringStorage, char** errMsg);
/* DISABLED FOR 5.4
static int backlightStringMV(WindowInfo *window, DataValue *argList,
	int nArgs, DataValue *result, char **errMsg);
*/
static int rangesetListMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int versionMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int rangesetCreateMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int rangesetDestroyMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int rangesetGetByNameMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int rangesetAddMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int rangesetSubtractMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int rangesetInvertMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int rangesetInfoMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int rangesetRangeMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int rangesetIncludesPosMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int rangesetSetColorMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int rangesetSetNameMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int rangesetSetModeMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);

static int fillPatternResult(DataValue* result, char** errMsg, WindowInfo* window, char* patternName, bool preallocatedPatternName, bool includeName, char* styleName, int bufferPos);
static int getPatternByNameMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int getPatternAtPosMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);

static int fillStyleResult(DataValue* result, char** errMsg, WindowInfo* window, char* styleName, bool preallocatedStyleName, bool includeName, int patCode, int bufferPos);
static int getStyleByNameMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int getStyleAtPosMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);
static int filenameDialogMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg);

// Built-in subroutines and variables for the macro language 
static BuiltInSubr MacroSubrs[] = {lengthMS, getRangeMS, tPrintMS,
                                   dialogMS, stringDialogMS, replaceRangeMS, replaceSelectionMS,
                                   setCursorPosMS, getCharacterMS, minMS, maxMS, searchMS,
                                   searchStringMS, substringMS, replaceSubstringMS, readFileMS,
                                   writeFileMS, appendFileMS, beepMS, getSelectionMS, validNumberMS,
                                   replaceInStringMS, selectMS, selectRectangleMS, focusWindowMS,
                                   shellCmdMS, stringToClipboardMS, clipboardToStringMS, toupperMS,
                                   tolowerMS, listDialogMS, getenvMS,
                                   stringCompareMS, splitMS, calltipMS, killCalltipMS,
                                   // DISABLED for 5.4        setBacklightStringMS,
                                   rangesetCreateMS, rangesetDestroyMS,
                                   rangesetAddMS, rangesetSubtractMS, rangesetInvertMS,
                                   rangesetInfoMS, rangesetRangeMS, rangesetIncludesPosMS,
                                   rangesetSetColorMS, rangesetSetNameMS, rangesetSetModeMS,
                                   rangesetGetByNameMS,
                                   getPatternByNameMS, getPatternAtPosMS,
                                   getStyleByNameMS, getStyleAtPosMS, filenameDialogMS
                                  };
#define N_MACRO_SUBRS (sizeof MacroSubrs/sizeof *MacroSubrs)
static const char* MacroSubrNames[N_MACRO_SUBRS] = {"length", "get_range", "t_print",
      "dialog", "string_dialog", "replace_range", "replace_selection",
      "set_cursor_pos", "get_character", "min", "max", "search",
      "search_string", "substring", "replace_substring", "read_file",
      "write_file", "append_file", "beep", "get_selection", "valid_number",
      "replace_in_string", "select", "select_rectangle", "focus_window",
      "shell_command", "string_to_clipboard", "clipboard_to_string",
      "toupper", "tolower", "list_dialog", "getenv",
      "string_compare", "split", "calltip", "kill_calltip",
      // DISABLED for 5.4        "set_backlight_string", 
      "rangeset_create", "rangeset_destroy",
      "rangeset_add", "rangeset_subtract", "rangeset_invert",
      "rangeset_info", "rangeset_range", "rangeset_includes",
      "rangeset_set_color", "rangeset_set_name", "rangeset_set_mode",
      "rangeset_get_by_name",
      "get_pattern_by_name", "get_pattern_at_pos",
      "get_style_by_name", "get_style_at_pos", "filename_dialog"
                                                   };
static BuiltInSubr SpecialVars[] = {cursorMV, lineMV, columnMV,
                                    fileNameMV, filePathMV, lengthMV, selectionStartMV, selectionEndMV,
                                    selectionLeftMV, selectionRightMV, wrapMarginMV, tabDistMV,
                                    emTabDistMV, useTabsMV, languageModeMV, modifiedMV,
                                    statisticsLineMV, incSearchLineMV, showLineNumbersMV,
                                    autoIndentMV, wrapTextMV, highlightSyntaxMV,
                                    makeBackupCopyMV, incBackupMV, showMatchingMV, matchSyntaxBasedMV,
                                    overTypeModeMV, readOnlyMV, lockedMV, fileFormatMV,
                                    fontNameMV, fontNameItalicMV,
                                    fontNameBoldMV, fontNameBoldItalicMV, subscriptSepMV,
                                    minFontWidthMV, maxFontWidthMV, topLineMV, numDisplayLinesMV,
                                    displayWidthMV, activePaneMV, nPanesMV, emptyArrayMV,
                                    serverNameMV, calltipIDMV,
                                    // DISABLED for 5.4        backlightStringMV, 
                                    rangesetListMV, versionMV
                                   };
#define N_SPECIAL_VARS (sizeof SpecialVars/sizeof *SpecialVars)
static const char* SpecialVarNames[N_SPECIAL_VARS] = {"$cursor", "$line", "$column",
      "$file_name", "$file_path", "$text_length", "$selection_start",
      "$selection_end", "$selection_left", "$selection_right",
      "$wrap_margin", "$tab_dist", "$em_tab_dist", "$use_tabs",
      "$language_mode", "$modified",
      "$statistics_line", "$incremental_search_line", "$show_line_numbers",
      "$auto_indent", "$wrap_text", "$highlight_syntax",
      "$make_backup_copy", "$incremental_backup", "$show_matching", "$match_syntax_based",
      "$overtype_mode", "$read_only", "$locked", "$file_format",
      "$font_name", "$font_name_italic",
      "$font_name_bold", "$font_name_bold_italic", "$sub_sep",
      "$min_font_width", "$max_font_width", "$top_line", "$n_display_lines",
      "$display_width", "$active_pane", "$n_panes", "$empty_array",
      "$server_name", "$calltip_ID",
      // DISABLED for 5.4       "$backlight_string", 
      "$rangeset_list", "$VERSION"
                                                     };

// Global symbols for returning values from built-in functions 
#define N_RETURN_GLOBALS 5
enum retGlobalSyms {STRING_DIALOG_BUTTON, SEARCH_END, READ_STATUS,
                    SHELL_CMD_STATUS, LIST_DIALOG_BUTTON
                   };
static const char* ReturnGlobalNames[N_RETURN_GLOBALS] = {"$string_dialog_button",
      "$search_end", "$read_status", "$shell_cmd_status",
      "$list_dialog_button"
                                                         };
static Symbol* ReturnGlobals[N_RETURN_GLOBALS];

// List of actions not useful when learning a macro sequence (also see below) 
static char* IgnoredActions[] = {"focusIn", "focusOut"};

// List of actions intended to be attached to mouse buttons, which the user
// must be warned can't be recorded in a learn/replay sequence
static const char* MouseActions[] = {"grab_focus", "extend_adjust", "extend_start",
                                     "extend_end", "secondary_or_drag_adjust", "secondary_adjust",
                                     "secondary_or_drag_start", "secondary_start", "move_destination",
                                     "move_to", "move_to_or_end_drag", "copy_to", "copy_to_or_end_drag",
                                     "exchange", "process_bdrag", "mouse_pan"
                                    };

// List of actions to not record because they
// generate further actions, more suitable for recording
static const char* RedundantActions[] = {"open_dialog", "save_as_dialog",
                                        "revert_to_saved_dialog", "include_file_dialog", "load_macro_file_dialog",
                                        "load_tags_file_dialog", "find_dialog", "replace_dialog",
                                        "goto_line_number_dialog", "mark_dialog", "goto_mark_dialog",
                                        "control_code_dialog", "filter_selection_dialog", "execute_command_dialog",
                                        "repeat_dialog", "start_incremental_find"
                                        };

// The last command executed (used by the Repeat command) 
static char* LastCommand = NULL;

// The current macro to execute on Replay command 
static char* ReplayMacro = NULL;

// Buffer where macro commands are recorded in Learn mode 
static Ne_Text_Buffer* MacroRecordBuf = NULL;

// Action Hook id for recording actions for Learn mode 
static int MacroRecordActionHook = 0;

// Window where macro recording is taking place 
static WindowInfo* MacroRecordWindow = NULL;

// Arrays for translating escape characters in escapeStringChars 
static char ReplaceChars[] = "\\\"ntbrfav";
static char EscapeChars[] = "\\\"\n\t\b\r\f\a\v";

/*
** Install built-in macro subroutines and special variables for accessing
** editor information
*/
void RegisterMacroSubroutines()
{
   static DataValue subrPtr = {NO_TAG, {0}}, noValue = {NO_TAG, {0}};

   // Install symbols for built-in routines and variables, with pointers
   // to the appropriate c routines to do the work
   for (unsigned i=0; i<N_MACRO_SUBRS; i++)
   {
      subrPtr.val.subr = MacroSubrs[i];
      InstallSymbol(MacroSubrNames[i], C_FUNCTION_SYM, subrPtr);
   }
   for (unsigned i=0; i<N_SPECIAL_VARS; i++)
   {
      subrPtr.val.subr = SpecialVars[i];
      InstallSymbol(SpecialVarNames[i], PROC_VALUE_SYM, subrPtr);
   }

   // Define global variables used for return values, remember their
   // locations so they can be set without a LookupSymbol call
   for (unsigned i=0; i<N_RETURN_GLOBALS; i++)
      ReturnGlobals[i] = InstallSymbol(ReturnGlobalNames[i], GLOBAL_SYM, noValue);
}

#define MAX_LEARN_MSG_LEN ((2 * MAX_ACCEL_LEN) + 60)

void BeginLearn(WindowInfo* window)
{
// TODO:    WindowInfo* win;
// TODO:    NeString s;
// TODO:    NeString xmFinish;
// TODO:    NeString xmCancel;
// TODO:    char* cFinish;
// TODO:    char* cCancel;
// TODO:    char message[MAX_LEARN_MSG_LEN];
// TODO: 
   // If we're already in learn mode, return 
   if (MacroRecordActionHook != 0)
      return;

   // dim the inappropriate menus and items, and undim finish and cancel 
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeSetSensitive(win->learnItem, false);
// TODO:    }
// TODO:    SetSensitive(window, window->finishLearnItem, false);
// TODO:    XtVaSetValues(window->cancelMacroItem, XmNlabelString, s=NeNewString("Cancel Learn"), NULL);
// TODO:    NeStringFree(s);
// TODO:    SetSensitive(window, window->cancelMacroItem, true);
// TODO: 
// TODO:    // Mark the window where learn mode is happening 
// TODO:    MacroRecordWindow = window;
// TODO: 
// TODO:    // Allocate a text buffer for accumulating the macro strings 
// TODO:    MacroRecordBuf = BufCreate();
// TODO: 
// TODO:    // Add the action hook for recording the actions 
// TODO:    MacroRecordActionHook = XtAppAddActionHook(XtWidgetToApplicationContext(window->mainWindow), learnActionHook, window);
// TODO: 
// TODO:    // Extract accelerator texts from menu PushButtons 
// TODO:    XtVaGetValues(window->finishLearnItem, XmNacceleratorText, &xmFinish, NULL);
// TODO:    XtVaGetValues(window->cancelMacroItem, XmNacceleratorText, &xmCancel, NULL);
// TODO: 
// TODO:    // Translate Motif strings to char* 
// TODO:    cFinish = GetNeStringText(xmFinish);
// TODO:    cCancel = GetNeStringText(xmCancel);
// TODO: 
// TODO:    // Free Motif Strings 
// TODO:    NeStringFree(xmFinish);
// TODO:    NeStringFree(xmCancel);
// TODO: 
// TODO:    // Create message 
// TODO:    if (cFinish[0] == '\0')
// TODO:    {
// TODO:       if (cCancel[0] == '\0')
// TODO:       {
// TODO:          strncpy(message, "Learn Mode -- Use menu to finish or cancel", MAX_LEARN_MSG_LEN);
// TODO:          message[MAX_LEARN_MSG_LEN - 1] = '\0';
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          sprintf(message, "Learn Mode -- Use menu to finish, press %s to cancel", cCancel);
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if (cCancel[0] == '\0')
// TODO:       {
// TODO:          sprintf(message, "Learn Mode -- Press %s to finish, use menu to cancel", cFinish);
// TODO: 
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          sprintf(message,"Learn Mode -- Press %s to finish, %s to cancel", cFinish, cCancel);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    // Free C-strings 
// TODO:    free__(cFinish);
// TODO:    free__(cCancel);
// TODO: 
// TODO:    // Put up the learn-mode banner 
// TODO:    SetModeMessage(window, message);
}

void AddLastCommandActionHook(Ne_AppContext& context)
{
   NeAppAddActionHook(context, lastActionHook, NULL);
}

void FinishLearn()
{
// TODO:    WindowInfo* win;
// TODO: 
   // If we're not in learn mode, return 
   if (MacroRecordActionHook == 0)
      return;

// TODO:    // Remove the action hook 
// TODO:    XtRemoveActionHook(MacroRecordActionHook);
// TODO:    MacroRecordActionHook = 0;
// TODO: 
// TODO:    // Free the old learn/replay sequence 
// TODO:    free__(ReplayMacro);
// TODO: 
// TODO:    // Store the finished action for the replay menu item 
// TODO:    ReplayMacro = BufGetAll(MacroRecordBuf);
// TODO: 
// TODO:    // Free the buffer used to accumulate the macro sequence 
// TODO:    BufFree(MacroRecordBuf);
// TODO: 
// TODO:    // Undim the menu items dimmed during learn 
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeSetSensitive(win->learnItem, true);
// TODO:    }
// TODO:    if (IsTopDocument(MacroRecordWindow))
// TODO:    {
// TODO:       NeSetSensitive(MacroRecordWindow->finishLearnItem, false);
// TODO:       NeSetSensitive(MacroRecordWindow->cancelMacroItem, false);
// TODO:    }
// TODO: 
// TODO:    // Undim the replay and paste-macro buttons 
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeSetSensitive(win->replayItem, true);
// TODO:    }
// TODO:    DimPasteReplayBtns(true);
// TODO: 
// TODO:    // Clear learn-mode banner 
// TODO:    ClearModeMessage(MacroRecordWindow);
}

/*
** Cancel Learn mode, or macro execution (they're bound to the same menu item)
*/
void CancelMacroOrLearn(WindowInfo* window)
{
   if (MacroRecordActionHook != 0)
      cancelLearn();
   else if (window->macroCmdData != NULL)
      AbortMacroCommand(window);
}

static void cancelLearn()
{
// TODO:    WindowInfo* win;
// TODO: 
   // If we're not in learn mode, return 
   if (MacroRecordActionHook == 0)
      return;

// TODO:    // Remove the action hook 
// TODO:    XtRemoveActionHook(MacroRecordActionHook);
// TODO:    MacroRecordActionHook = 0;
// TODO: 
// TODO:    // Free the macro under construction 
// TODO:    BufFree(MacroRecordBuf);
// TODO: 
// TODO:    // Undim the menu items dimmed during learn 
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:    {
// TODO:       if (!IsTopDocument(win))
// TODO:          continue;
// TODO:       NeSetSensitive(win->learnItem, true);
// TODO:    }
// TODO:    if (IsTopDocument(MacroRecordWindow))
// TODO:    {
// TODO:       NeSetSensitive(MacroRecordWindow->finishLearnItem, false);
// TODO:       NeSetSensitive(MacroRecordWindow->cancelMacroItem, false);
// TODO:    }
// TODO: 
// TODO:    // Clear learn-mode banner 
// TODO:    ClearModeMessage(MacroRecordWindow);
}

/*
** Execute the learn/replay sequence stored in "window"
*/
void Replay(WindowInfo* window)
{
   Program* prog;
   char* errMsg, *stoppedAt;

   // Verify that a replay macro exists and it's not empty and that 
   // we're not already running a macro 
   if (ReplayMacro != NULL &&
         ReplayMacro[0] != 0 &&
         window->macroCmdData == NULL)
   {
      /* Parse the replay macro (it's stored in text form) and compile it into
      an executable program "prog" */
      prog = ParseMacro(ReplayMacro, &errMsg, &stoppedAt);
      if (prog == NULL)
      {
         fprintf(stderr, "NEdit internal error, learn/replay macro syntax error: %s\n", errMsg);
         return;
      }

      // run the executable program 
      runMacro(window, prog);
   }
}

/*
**  Read the initial NEdit macro file if one exists.
*/
void ReadMacroInitFile(WindowInfo* window)
{
   const char* autoloadName = GetRCFileName(AUTOLOAD_NM);
   static int initFileLoaded = false;

   // GetRCFileName() might return NULL if an error occurs during
   // creation of the preference file directory.
   if (autoloadName != NULL && !initFileLoaded)
   {
      ReadMacroFile(window, autoloadName, false);
      initFileLoaded = true;
   }
}

/*
** Read an NEdit macro file.  Extends the syntax of the macro parser with
** define keyword, and allows intermixing of defines with immediate actions.
*/
int ReadMacroFile(WindowInfo* window, const char* fileName, int warnNotExist)
{
   /* read-in macro file and force a terminating \n, to prevent syntax
   ** errors with statements on the last line
   */
   char* fileString = ReadAnyTextFile(fileName, true);
   if (fileString == NULL)
   {
      if (errno != ENOENT || warnNotExist)
      {
         DialogF(DF_ERR, window->mainWindow, 1, "Read Macro", "Error reading macro file %s: %s", "OK", fileName, strerror(errno));
      }
      return false;
   }

   // Parse fileString 
   int result = readCheckMacroString(window->mainWindow, fileString, window, fileName, NULL);
   delete[] fileString;
   return result;
}

/*
** Parse and execute a macro string including macro definitions.  Report
** parsing errors in a dialog posted over window->mainWindow.
*/
int ReadMacroString(WindowInfo* window, char* string, const char* errIn)
{
   return readCheckMacroString(window->mainWindow, string, window, errIn, NULL);
}

/*
** Check a macro string containing definitions for errors.  Returns true
** if macro compiled successfully.  Returns false and puts up
** a dialog explaining if macro did not compile successfully.
*/
int CheckMacroString(Fl_Widget* dialogParent, char* string, const char* errIn,
                     char** errPos)
{
   return readCheckMacroString(dialogParent, string, NULL, errIn, errPos);
}

/*
** Parse and optionally execute a macro string including macro definitions.
** Report parsing errors in a dialog posted over dialogParent, using the
** string errIn to identify the entity being parsed (filename, macro string,
** etc.).  If runWindow is specified, runs the macro against the window.  If
** runWindow is passed as NULL, does parse only.  If errPos is non-null,
** returns a pointer to the error location in the string.
*/
static int readCheckMacroString(Fl_Widget* dialogParent, char* string,
                                WindowInfo* runWindow, const char* errIn, char** errPos)
{
   char* stoppedAt, *inPtr, *namePtr, *errMsg;
   char subrName[MAX_SYM_LEN];
   Program* prog;
   Symbol* sym;
   DataValue subrPtr;
   Stack* progStack = new Stack();
   progStack->top = NULL;
   progStack->size = 0;

   inPtr = string;
   while (*inPtr != '\0')
   {

      // skip over white space and comments 
      while (*inPtr==' ' || *inPtr=='\t' || *inPtr=='\n'|| *inPtr=='#')
      {
         if (*inPtr == '#')
            while (*inPtr != '\n' && *inPtr != '\0') inPtr++;
         else
            inPtr++;
      }
      if (*inPtr == '\0')
         break;

      // look for define keyword, and compile and store defined routines 
      if (!strncmp(inPtr, "define", 6) && (inPtr[6]==' ' || inPtr[6]=='\t'))
      {
         inPtr += 6;
         inPtr += strspn(inPtr, " \t\n");
         namePtr = subrName;
         while ((namePtr < &subrName[MAX_SYM_LEN - 1])
                && (isalnum((unsigned char)*inPtr) || *inPtr == '_'))
         {
            *namePtr++ = *inPtr++;
         }
         *namePtr = '\0';
         if (isalnum((unsigned char)*inPtr) || *inPtr == '_')
         {
            return ParseError(dialogParent, string, inPtr, errIn,
                              "subroutine name too long");
         }
         inPtr += strspn(inPtr, " \t\n");
         if (*inPtr != '{')
         {
            if (errPos != NULL) *errPos = stoppedAt;
            return ParseError(dialogParent, string, inPtr,
                              errIn, "expected '{'");
         }
         prog = ParseMacro(inPtr, &errMsg, &stoppedAt);
         if (prog == NULL)
         {
            if (errPos != NULL) *errPos = stoppedAt;
            return ParseError(dialogParent, string, stoppedAt,
                              errIn, errMsg);
         }
         if (runWindow != NULL)
         {
            sym = LookupSymbol(subrName);
            if (sym == NULL)
            {
               subrPtr.val.prog = prog;
               subrPtr.tag = NO_TAG;
               sym = InstallSymbol(subrName, MACRO_FUNCTION_SYM, subrPtr);
            }
            else
            {
               if (sym->type == MACRO_FUNCTION_SYM)
                  FreeProgram(sym->value.val.prog);
               else
                  sym->type = MACRO_FUNCTION_SYM;
               sym->value.val.prog = prog;
            }
         }
         inPtr = stoppedAt;

         /* Parse and execute immediate (outside of any define) macro commands
            and WAIT for them to finish executing before proceeding.  Note that
            the code below is not perfect.  If you interleave code blocks with
            definitions in a file which is loaded from another macro file, it
            will probably run the code blocks in reverse order! */
      }
      else
      {
         prog = ParseMacro(inPtr, &errMsg, &stoppedAt);
         if (prog == NULL)
         {
            if (errPos != NULL)
            {
               *errPos = stoppedAt;
            }

            return ParseError(dialogParent, string, stoppedAt,
                              errIn, errMsg);
         }

         if (runWindow != NULL)
         {
            int nextEvent;
            if (runWindow->macroCmdData == NULL)
            {
               runMacro(runWindow, prog);
// TODO:                while (runWindow->macroCmdData != NULL)
// TODO:                {
// TODO:                   XtAppNextEvent(XtWidgetToApplicationContext(runWindow->mainWindow),  &nextEvent);
// TODO:                   ServerDispatchEvent(&nextEvent);
// TODO:                }
            }
            else
            {
               /*  If we come here this means that the string was parsed
                   from within another macro via load_macro_file(). In
                   this case, plain code segments outside of define
                   blocks are rolled into one Program each and put on
                   the stack. At the end, the stack is unrolled, so the
                   plain Programs would be executed in the wrong order.

                   So we don't hand the Programs over to the interpreter
                   just yet (via RunMacroAsSubrCall()), but put it on a
                   stack of our own, reversing order once again.   */
               Push(progStack, (void*) prog);
            }
         }
         inPtr = stoppedAt;
      }
   }

   //  Unroll reversal stack for macros loaded from macros.  
   while (NULL != (prog = (Program*) Pop(progStack)))
   {
      RunMacroAsSubrCall(prog);
   }

   //  This stack is empty, so just free__ it without checking the members.  
   delete progStack;

   return true;
}

/*
** Run a pre-compiled macro, changing the interface state to reflect that
** a macro is running, and handling preemption, resumption, and cancellation.
** frees prog when macro execution is complete;
*/
static void runMacro(WindowInfo* window, Program* prog)
{
// TODO:    DataValue result;
// TODO:    char* errMsg;
// TODO:    int stat;
// TODO:    macroCmdInfo* cmdData;
// TODO:    NeString s;
// TODO: 
   /* If a macro is already running, just call the program as a subroutine,
      instead of starting a new one, so we don't have to keep a separate
      context, and the macros will serialize themselves automatically */
   if (window->macroCmdData != NULL)
   {
      RunMacroAsSubrCall(prog);
      return;
   }

// TODO:    // put up a watch cursor over the waiting window 
// TODO:    BeginWait(window->mainWindow);
// TODO: 
// TODO:    // enable the cancel menu item 
// TODO:    XtVaSetValues(window->cancelMacroItem, XmNlabelString,
// TODO:                  s=NeNewString("Cancel Macro"), NULL);
// TODO:    NeStringFree(s);
// TODO:    SetSensitive(window, window->cancelMacroItem, true);
// TODO: 
// TODO:    /* Create a data structure for passing macro execution information around
// TODO:       amongst the callback routines which will process i/o and completion */
// TODO:    cmdData = (macroCmdInfo*)malloc__(sizeof(macroCmdInfo));
// TODO:    window->macroCmdData = cmdData;
// TODO:    cmdData->bannerIsUp = false;
// TODO:    cmdData->closeOnCompletion = false;
// TODO:    cmdData->program = prog;
// TODO:    cmdData->context = NULL;
// TODO:    cmdData->continueWorkProcID = 0;
// TODO:    cmdData->dialog = NULL;
// TODO: 
// TODO:    // Set up timer proc for putting up banner when macro takes too long 
// TODO:    cmdData->bannerTimeoutID = XtAppAddTimeOut(
// TODO:                                  XtWidgetToApplicationContext(window->mainWindow), BANNER_WAIT_TIME,
// TODO:                                  bannerTimeoutProc, window);
// TODO: 
// TODO:    // Begin macro execution 
// TODO:    stat = ExecuteMacro(window, prog, 0, NULL, &result, &cmdData->context,
// TODO:                        &errMsg);
// TODO: 
// TODO:    if (stat == MACRO_ERROR)
// TODO:    {
// TODO:       finishMacroCmdExecution(window);
// TODO:       DialogF(DF_ERR, window->mainWindow, 1, "Macro Error", "Error executing macro: %s", "OK", errMsg);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    if (stat == MACRO_DONE)
// TODO:    {
// TODO:       finishMacroCmdExecution(window);
// TODO:       return;
// TODO:    }
// TODO:    if (stat == MACRO_TIME_LIMIT)
// TODO:    {
// TODO:       ResumeMacroExecution(window);
// TODO:       return;
// TODO:    }
   // (stat == MACRO_PREEMPT) Macro was preempted 
}

/*
** Continue with macro execution after preemption.  Called by the routines
** whose actions cause preemption when they have completed their lengthy tasks.
** Re-establishes macro execution work proc.  Window must be the window in
** which the macro is executing (the window to which macroCmdData is attached),
** and not the window to which operations are focused.
*/
void ResumeMacroExecution(WindowInfo* window)
{
   macroCmdInfo* cmdData = (macroCmdInfo*)window->macroCmdData;

// TODO:    if (cmdData != NULL)
// TODO:       cmdData->continueWorkProcID = XtAppAddWorkProc(
// TODO:                                        XtWidgetToApplicationContext(window->mainWindow),
// TODO:                                        continueWorkProc, window);
}

/*
** Cancel the macro command in progress (user cancellation via GUI)
*/
void AbortMacroCommand(WindowInfo* window)
{
   if (window->macroCmdData == NULL)
      return;

   /* If there's both a macro and a shell command executing, the shell command
      must have been called from the macro.  When called from a macro, shell
      commands don't put up cancellation controls of their own, but rely
      instead on the macro cancellation mechanism (here) */
   if (window->shellCmdData != NULL)
      AbortShellCommand(window);

   // Free the continuation 
   FreeRestartData(((macroCmdInfo*)window->macroCmdData)->context);

   // Kill the macro command 
   finishMacroCmdExecution(window);
}

/*
** Call this before closing a window, to clean up macro references to the
** window, stop any macro which might be running from it, free__ associated
** memory, and check that a macro is not attempting to close the window from
** which it is run.  If this is being called from a macro, and the window
** this routine is examining is the window from which the macro was run, this
** routine will return false, and the caller must NOT CLOSE THE WINDOW.
** Instead, empty it and make it Untitled, and let the macro completion
** process close the window when the macro is finished executing.
*/
int MacroWindowCloseActions(WindowInfo* window)
{
   macroCmdInfo* mcd, *cmdData = (macroCmdInfo*)window->macroCmdData;
   WindowInfo* w;

   if (MacroRecordActionHook != 0 && MacroRecordWindow == window)
   {
      FinishLearn();
   }

   /* If no macro is executing in the window, allow the close, but check
      if macros executing in other windows have it as focus.  If so, set
      their focus back to the window from which they were originally run */
   if (cmdData == NULL)
   {
      for (w=WindowList; w!=NULL; w=w->next)
      {
         mcd = (macroCmdInfo*)w->macroCmdData;
         if (w == MacroRunWindow() && MacroFocusWindow() == window)
            SetMacroFocusWindow(MacroRunWindow());
         else if (mcd != NULL && mcd->context->focusWindow == window)
            mcd->context->focusWindow = mcd->context->runWindow;
      }
      return true;
   }

   /* If the macro currently running (and therefore calling us, because
      execution must otherwise return to the main loop to execute any
      commands), is running in this window, tell the caller not to close,
      and schedule window close on completion of macro */
   if (window == MacroRunWindow())
   {
      cmdData->closeOnCompletion = true;
      return false;
   }

   // Free the continuation 
   FreeRestartData(cmdData->context);

   // Kill the macro command 
   finishMacroCmdExecution(window);
   return true;
}

/*
** Clean up after the execution of a macro command: free__ memory, and restore
** the user interface state.
*/
static void finishMacroCmdExecution(WindowInfo* window)
{
   macroCmdInfo* cmdData = (macroCmdInfo*)window->macroCmdData;
   int closeOnCompletion = cmdData->closeOnCompletion;
   NeString s;
// TODO:    XClientMessageEvent event;
// TODO: 
// TODO:    // Cancel pending timeout and work proc 
// TODO:    if (cmdData->bannerTimeoutID != 0)
// TODO:       XtRemoveTimeOut(cmdData->bannerTimeoutID);
// TODO:    if (cmdData->continueWorkProcID != 0)
// TODO:       XtRemoveWorkProc(cmdData->continueWorkProcID);
// TODO: 
// TODO:    // Clean up waiting-for-macro-command-to-complete mode 
// TODO:    EndWait(window->mainWindow);
// TODO:    XtVaSetValues(window->cancelMacroItem, XmNlabelString, s=NeNewString("Cancel Learn"), NULL);
// TODO:    NeStringFree(s);
// TODO:    SetSensitive(window, window->cancelMacroItem, false);
// TODO:    if (cmdData->bannerIsUp)
// TODO:       ClearModeMessage(window);
// TODO: 
// TODO:    // If a dialog was up, get rid of it 
// TODO:    if (cmdData->dialog != NULL)
// TODO:       XtDestroyWidget(XtParent(cmdData->dialog));
// TODO: 
// TODO:    // Free execution information 
// TODO:    FreeProgram(cmdData->program);
// TODO:    free__((char*)cmdData);
// TODO:    window->macroCmdData = NULL;
// TODO: 
// TODO:    /* If macro closed its own window, window was made empty and untitled,
// TODO:       but close was deferred until completion.  This is completion, so if
// TODO:       the window is still empty, do the close */
// TODO:    if (closeOnCompletion && !window->filenameSet && !window->fileChanged)
// TODO:    {
// TODO:       CloseWindow(window);
// TODO:       window = NULL;
// TODO:    }
// TODO: 
// TODO:    // If no other macros are executing, do garbage collection 
// TODO:    SafeGC();
// TODO: 
// TODO:    /* In processing the .neditmacro file (and possibly elsewhere), there
// TODO:       is an event loop which waits for macro completion.  Send an event
// TODO:       to wake up that loop, otherwise execution will stall until the user
// TODO:       does something to the window. */
// TODO:    if (!closeOnCompletion)
// TODO:    {
// TODO:       event.format = 8;
// TODO:       event.type = ClientMessage;
// TODO:       XSendEvent(XtDisplay(window->mainWindow), XtWindow(window->mainWindow), false,
// TODO:                  NoEventMask, (int*)&event);
// TODO:    }
}

/*
** Do garbage collection of strings if there are no macros currently
** executing.  NEdit's macro language GC strategy is to call this routine
** whenever a macro completes.  If other macros are still running (preempted
** or waiting for a shell command or dialog), this does nothing and therefore
** defers GC to the completion of the last macro out.
*/
void SafeGC()
{
// TODO:    WindowInfo* win;
// TODO: 
// TODO:    for (win=WindowList; win!=NULL; win=win->next)
// TODO:       if (win->macroCmdData != NULL || InSmartIndentMacros(win))
// TODO:          return;
   GarbageCollectStrings();
}

/*
** Executes macro string "macro" using the lastFocus pane in "window".
** Reports errors via a dialog posted over "window", integrating the name
** "errInName" into the message to help identify the source of the error.
*/
void DoMacro(WindowInfo* window, const char* macro, const char* errInName)
{
   Program* prog;
   char* errMsg, *stoppedAt, *tMacro;
   int macroLen;

   /* Add a terminating newline (which command line users are likely to omit
      since they are typically invoking a single routine) */
   macroLen = strlen(macro);
   tMacro = (char*)malloc__(strlen(macro)+2);
   strncpy(tMacro, macro, macroLen);
   tMacro[macroLen] = '\n';
   tMacro[macroLen+1] = '\0';

   // Parse the macro and report errors if it fails 
   prog = ParseMacro(tMacro, &errMsg, &stoppedAt);
   if (prog == NULL)
   {
      ParseError(window->mainWindow, tMacro, stoppedAt, errInName, errMsg);
      free__(tMacro);
      return;
   }
   free__(tMacro);

   // run the executable program (prog is freed upon completion) 
   runMacro(window, prog);
}

/*
** Get the current Learn/Replay macro in text form.  Returned string is a
** pointer to the stored macro and should not be freed by the caller (and
** will cease to exist when the next replay macro is installed)
*/
char* GetReplayMacro()
{
   return ReplayMacro;
}

/*
** Present the user a dialog for "Repeat" command
*/
void RepeatDialog(WindowInfo* window)
{
// TODO:    Fl_Widget* form, *selBox, *radioBox, *timesForm;
// TODO:    repeatDialog* rd;
// TODO:    Arg selBoxArgs[1];
// TODO:    char* lastCmdLabel, *parenChar;
// TODO:    NeString s1;
// TODO:    int cmdNameLen;
// TODO: 
// TODO:    if (LastCommand == NULL)
// TODO:    {
// TODO:       DialogF(DF_WARN, window->mainWindow, 1, "Repeat Macro",
// TODO:               "No previous commands or learn/\nreplay sequences to repeat",
// TODO:               "OK");
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Remeber the last command, since the user is allowed to work in the
// TODO:       window while the dialog is up */
// TODO:    rd = (repeatDialog*)malloc__(sizeof(repeatDialog));
// TODO:    rd->lastCommand = NeNewString(LastCommand);
// TODO: 
// TODO:    /* make a label for the Last command item of the dialog, which includes
// TODO:       the last executed action name */
// TODO:    parenChar = strchr(LastCommand, '(');
// TODO:    if (parenChar == NULL)
// TODO:       return;
// TODO:    cmdNameLen = parenChar-LastCommand;
// TODO:    lastCmdLabel = (char*)malloc__(16 + cmdNameLen);
// TODO:    strcpy(lastCmdLabel, "Last Command (");
// TODO:    strncpy(&lastCmdLabel[14], LastCommand, cmdNameLen);
// TODO:    strcpy(&lastCmdLabel[14 + cmdNameLen], ")");
// TODO: 
// TODO:    XtSetArg(selBoxArgs[0], XmNautoUnmanage, false);
// TODO:    selBox = CreatePromptDialog(window->mainWindow, "repeat", selBoxArgs, 1);
// TODO:    rd->mainWindow = XtParent(selBox);
// TODO:    XtAddCallback(rd->mainWindow, XmNdestroyCallback, repeatDestroyCB, rd);
// TODO:    XtAddCallback(selBox, XmNokCallback, repeatOKCB, rd);
// TODO:    XtAddCallback(selBox, XmNapplyCallback, repeatApplyCB, rd);
// TODO:    XtAddCallback(selBox, XmNcancelCallback, repeatCancelCB, rd);
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_TEXT));
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_SELECTION_LABEL));
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_HELP_BUTTON));
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_APPLY_BUTTON));
// TODO:    XtVaSetValues(XtParent(selBox), XmNtitle, "Repeat Macro", NULL);
// TODO:    AddMotifCloseCallback(XtParent(selBox), repeatCancelCB, rd);
// TODO: 
// TODO:    form = XtVaCreateManagedWidget("form", xmFormWidgetClass, selBox, NULL);
// TODO: 
// TODO:    radioBox = XtVaCreateManagedWidget("cmdSrc", xmRowColumnWidgetClass, form,
// TODO:                                       XmNradioBehavior, true,
// TODO:                                       XmNorientation, XmHORIZONTAL,
// TODO:                                       XmNpacking, XmPACK_TIGHT,
// TODO:                                       XmNtopAttachment, XmATTACH_FORM,
// TODO:                                       XmNleftAttachment, XmATTACH_FORM, NULL);
// TODO:    rd->lastCmdToggle = XtVaCreateManagedWidget("lastCmdToggle",
// TODO:                        xmToggleButtonWidgetClass, radioBox, XmNset, true,
// TODO:                        XmNlabelString, s1=NeNewString(lastCmdLabel),
// TODO:                        XmNmnemonic, 'C', NULL);
// TODO:    NeStringFree(s1);
// TODO:    free__(lastCmdLabel);
// TODO:    XtVaCreateManagedWidget("learnReplayToggle",
// TODO:                            xmToggleButtonWidgetClass, radioBox, XmNset, false,
// TODO:                            XmNlabelString,
// TODO:                            s1=NeNewString("Learn/Replay"),
// TODO:                            XmNmnemonic, 'L',
// TODO:                            XmNsensitive, ReplayMacro != NULL, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    timesForm = XtVaCreateManagedWidget("form", xmFormWidgetClass, form,
// TODO:                                        XmNtopAttachment, XmATTACH_WIDGET,
// TODO:                                        XmNtopWidget, radioBox,
// TODO:                                        XmNtopOffset, 10,
// TODO:                                        XmNleftAttachment, XmATTACH_FORM, NULL);
// TODO:    radioBox = XtVaCreateManagedWidget("method", xmRowColumnWidgetClass,
// TODO:                                       timesForm,
// TODO:                                       XmNradioBehavior, true,
// TODO:                                       XmNorientation, XmHORIZONTAL,
// TODO:                                       XmNpacking, XmPACK_TIGHT,
// TODO:                                       XmNtopAttachment, XmATTACH_FORM,
// TODO:                                       XmNbottomAttachment, XmATTACH_FORM,
// TODO:                                       XmNleftAttachment, XmATTACH_FORM, NULL);
// TODO:    rd->inSelToggle = XtVaCreateManagedWidget("inSelToggle",
// TODO:                      xmToggleButtonWidgetClass, radioBox, XmNset, false,
// TODO:                      XmNlabelString, s1=NeNewString("In Selection"),
// TODO:                      XmNmnemonic, 'I', NULL);
// TODO:    NeStringFree(s1);
// TODO:    rd->toEndToggle = XtVaCreateManagedWidget("toEndToggle",
// TODO:                      xmToggleButtonWidgetClass, radioBox, XmNset, false,
// TODO:                      XmNlabelString, s1=NeNewString("To End"),
// TODO:                      XmNmnemonic, 'T', NULL);
// TODO:    NeStringFree(s1);
// TODO:    XtVaCreateManagedWidget("nTimesToggle",
// TODO:                            xmToggleButtonWidgetClass, radioBox, XmNset, true,
// TODO:                            XmNlabelString, s1=NeNewString("N Times"),
// TODO:                            XmNmnemonic, 'N',
// TODO:                            XmNset, true, NULL);
// TODO:    NeStringFree(s1);
// TODO:    rd->repeatText = XtVaCreateManagedWidget("repeatText", xmTextWidgetClass,
// TODO:                     timesForm,
// TODO:                     XmNcolumns, 5,
// TODO:                     XmNtopAttachment, XmATTACH_FORM,
// TODO:                     XmNbottomAttachment, XmATTACH_FORM,
// TODO:                     XmNleftAttachment, XmATTACH_WIDGET,
// TODO:                     XmNleftWidget, radioBox, NULL);
// TODO:    RemapDeleteKey(rd->repeatText);
// TODO: 
// TODO:    // Handle mnemonic selection of buttons and focus to dialog 
// TODO:    AddDialogMnemonicHandler(form, FALSE);
// TODO: 
// TODO:    // Set initial focus 
// TODO: #if XmVersion >= 1002
// TODO:    XtVaSetValues(form, XmNinitialFocus, timesForm, NULL);
// TODO:    XtVaSetValues(timesForm, XmNinitialFocus, rd->repeatText, NULL);
// TODO: #endif
// TODO: 
// TODO:    // put up dialog 
// TODO:    rd->forWindow = window;
// TODO:    ManageDialogCenteredOnPointer(selBox);
}

static void repeatOKCB(Fl_Widget* w, void* data)
{
// TODO:    repeatDialog* rd = (repeatDialog*)clientData;
// TODO: 
// TODO:    if (doRepeatDialogAction(rd, ((XmAnyCallbackStruct*)callData)->event))
// TODO:       XtDestroyWidget(rd->mainWindow);
}

/* Note that the apply button is not managed in the repeat dialog.  The dialog
   itself is capable of non-modal operation, but to be complete, it needs
   to dynamically update last command, dimming of learn/replay, possibly a
   stop button for the macro, and possibly in-selection with selection */
static void repeatApplyCB(Fl_Widget* w, void* data)
{
// TODO:    doRepeatDialogAction((repeatDialog*)clientData, ((XmAnyCallbackStruct*)callData)->event);
}

static int doRepeatDialogAction(repeatDialog* rd, int event)
{
// TODO:    int nTimes;
// TODO:    char nTimesStr[TYPE_INT_STR_SIZE(int)];
// TODO:    char* params[2];
// TODO: 
// TODO:    // Find out from the dialog how to repeat the command 
// TODO:    if (NeToggleButtonGetState(rd->inSelToggle))
// TODO:    {
// TODO:       if (!rd->forWindow->buffer->primary.selected)
// TODO:       {
// TODO:          DialogF(DF_WARN, rd->mainWindow, 1, "Repeat Macro",
// TODO:                  "No selection in window to repeat within", "OK");
// TODO:          XmProcessTraversal(rd->inSelToggle, XmTRAVERSE_CURRENT);
// TODO:          return false;
// TODO:       }
// TODO:       params[0] = "in_selection";
// TODO:    }
// TODO:    else if (NeToggleButtonGetState(rd->toEndToggle))
// TODO:    {
// TODO:       params[0] = "to_end";
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if (GetIntTextWarn(rd->repeatText, &nTimes, "number of times", true)
// TODO:             != TEXT_READ_OK)
// TODO:       {
// TODO:          XmProcessTraversal(rd->repeatText, XmTRAVERSE_CURRENT);
// TODO:          return false;
// TODO:       }
// TODO:       sprintf(nTimesStr, "%d", nTimes);
// TODO:       params[0] = nTimesStr;
// TODO:    }
// TODO: 
// TODO:    // Figure out which command user wants to repeat 
// TODO:    if (NeToggleButtonGetState(rd->lastCmdToggle))
// TODO:       params[1] = NeNewString(rd->lastCommand);
// TODO:    else
// TODO:    {
// TODO:       if (ReplayMacro == NULL)
// TODO:          return false;
// TODO:       params[1] = NeNewString(ReplayMacro);
// TODO:    }
// TODO: 
// TODO:    // call the action routine repeat_macro to do the work 
// TODO:    XtCallActionProc(rd->forWindow->lastFocus, "repeat_macro", event, params,2);
// TODO:    free__(params[1]);
   return true;
}

static void repeatCancelCB(Fl_Widget* w, void* data)
{
// TODO:    repeatDialog* rd = (repeatDialog*)clientData;
// TODO: 
// TODO:    XtDestroyWidget(rd->mainWindow);
}

static void repeatDestroyCB(Fl_Widget* w, void* data)
{
// TODO:    repeatDialog* rd = (repeatDialog*)clientData;
// TODO: 
// TODO:    free__(rd->lastCommand);
// TODO:    free__((char*)rd);
}

/*
** Dispatches a macro to which repeats macro command in "command", either
** an integer number of times ("how" == positive integer), or within a
** selected range ("how" == REPEAT_IN_SEL), or to the end of the window
** ("how == REPEAT_TO_END).
**
** Note that as with most macro routines, this returns BEFORE the macro is
** finished executing
*/
void RepeatMacro(WindowInfo* window, const char* command, int how)
{
   Program* prog;
   char* errMsg, *stoppedAt, *loopMacro, *loopedCmd;

   if (command == NULL)
      return;

   // Wrap a for loop and counter/tests around the command 
   if (how == REPEAT_TO_END)
      loopMacro = "lastCursor=-1\nstartPos=$cursor\n\
while($cursor>=startPos&&$cursor!=lastCursor){\nlastCursor=$cursor\n%s\n}\n";
   else if (how == REPEAT_IN_SEL)
      loopMacro = "selStart = $selection_start\nif (selStart == -1)\nreturn\n\
selEnd = $selection_end\nset_cursor_pos(selStart)\nselect(0,0)\n\
boundText = get_range(selEnd, selEnd+10)\n\
while($cursor >= selStart && $cursor < selEnd && \\\n\
get_range(selEnd, selEnd+10) == boundText) {\n\
startLength = $text_length\n%s\n\
selEnd += $text_length - startLength\n}\n";
   else
      loopMacro = "for(i=0;i<%d;i++){\n%s\n}\n";
   loopedCmd = (char*)malloc__(strlen(command) + strlen(loopMacro) + 25);
   if (how == REPEAT_TO_END || how == REPEAT_IN_SEL)
      sprintf(loopedCmd, loopMacro, command);
   else
      sprintf(loopedCmd, loopMacro, how, command);

   // Parse the resulting macro into an executable program "prog" 
   prog = ParseMacro(loopedCmd, &errMsg, &stoppedAt);
   if (prog == NULL)
   {
      fprintf(stderr, "NEdit internal error, repeat macro syntax wrong: %s\n",
              errMsg);
      return;
   }
   free__(loopedCmd);

   // run the executable program 
   runMacro(window, prog);
}

/*
** Macro recording action hook for Learn/Replay, added temporarily during
** learn.
*/
static void learnActionHook(Fl_Widget* w, void* clientData, const char* actionName, int event, const char** params, int* numParams)
{
   WindowInfo* window;
   int i;
   char* actionString;

   /* Select only actions in text panes in the window for which this
      action hook is recording macros (from clientData). */
   for (window=WindowList; window!=NULL; window=window->next)
   {
      if (window->textArea == w)
         break;
      for (i=0; i<window->nPanes; i++)
      {
         if (window->textPanes[i] == w)
            break;
      }
      if (i < window->nPanes)
         break;
   }
   if (window == NULL || window != (WindowInfo*)clientData)
      return;

   /* beep on un-recordable operations which require a mouse position, to
      remind the user that the action was not recorded */
   if (isMouseAction(actionName))
   {
      fl_beep();
      return;
   }

   // Record the action and its parameters 
   actionString = actionToString(w, actionName, event, params, *numParams);
   if (actionString != NULL)
   {
      BufInsert(MacroRecordBuf, MacroRecordBuf->length, actionString);
      free__(actionString);
   }
}

/*
** Permanent action hook for remembering last action for possible replay
*/
static void lastActionHook(Fl_Widget* w, void* clientData, const char* actionName, int event, const char** params, int* numParams)
{
   TRACE();

   //WindowInfo* window;
   //int i;
   //char* actionString;

   //// Find the window to which this action belongs 
   //for (window=WindowList; window!=NULL; window=window->next)
   //{
   //   if (window->textArea == w)
   //      break;
   //   for (i=0; i<window->nPanes; i++)
   //   {
   //      if (window->textPanes[i] == w)
   //         break;
   //   }
   //   if (i < window->nPanes)
   //      break;
   //}
   //if (window == NULL)
   //   return;

   ///* The last action is recorded for the benefit of repeating the last
   //   action.  Don't record repeat_macro and wipe out the real action */
   //if (!strcmp(actionName, "repeat_macro"))
   //   return;

   //// Record the action and its parameters 
   //actionString = actionToString(w, actionName, event, params, *numParams);
   //if (actionString != NULL)
   //{
   //   free__(LastCommand);
   //   LastCommand = actionString;
   //}
}

/*
** Create a macro string to represent an invocation of an action routine.
** Returns NULL for non-operational or un-recordable actions.
*/
static char* actionToString(Fl_Widget* w, const char* actionName, int event, const char** params, int numParams)
{
   char* outStr;
// TODO:    char chars[20], *charList[1], *outStr, *outPtr;
// TODO:    KeySym keysym;
// TODO:    int i, nChars, nParams, length, nameLength;
// TODO: #ifndef NO_XMIM
// TODO:    int status;
// TODO: #endif
// TODO: 
// TODO:    if (isIgnoredAction(actionName) || isRedundantAction(actionName) ||
// TODO:          isMouseAction(actionName))
// TODO:       return NULL;
// TODO: 
// TODO:    // Convert self_insert actions, to insert_string 
// TODO:    if (!strcmp(actionName, "self_insert") || !strcmp(actionName, "self-insert"))
// TODO:    {
// TODO:       actionName = "insert_string";
// TODO: #ifdef NO_XMIM
// TODO:       nChars = XLookupString((XKeyEvent*)event, chars, 19, &keysym, NULL);
// TODO:       if (nChars == 0)
// TODO:          return NULL;
// TODO: #else
// TODO: 
// TODO:       nChars = XmImMbLookupString(w, (XKeyEvent*)event, chars, 19, &keysym, &status);
// TODO:       if (nChars == 0 || status == XLookupNone ||
// TODO:             status == XLookupKeySym || status == XBufferOverflow)
// TODO:          return NULL;
// TODO: #endif
// TODO:       chars[nChars] = '\0';
// TODO:       charList[0] = chars;
// TODO:       params = charList;
// TODO:       nParams = 1;
// TODO:    }
// TODO:    else
// TODO:       nParams = numParams;
// TODO: 
// TODO:    // Figure out the length of string required 
// TODO:    nameLength = strlen(actionName);
// TODO:    length = nameLength + 3;
// TODO:    for (i=0; i<nParams; i++)
// TODO:       length += escapedStringLength(params[i]) + 4;
// TODO: 
// TODO:    // Allocate the string and copy the information to it 
// TODO:    outPtr = outStr = malloc__(length + 1);
// TODO:    strcpy(outPtr, actionName);
// TODO:    outPtr += nameLength;
// TODO:    *outPtr++ = '(';
// TODO:    for (i=0; i<nParams; i++)
// TODO:    {
// TODO:       *outPtr++ = '\"';
// TODO:       outPtr += escapeStringChars(params[i], outPtr);
// TODO:       *outPtr++ = '\"';
// TODO:       *outPtr++ = ',';
// TODO:       *outPtr++ = ' ';
// TODO:    }
// TODO:    if (nParams != 0)
// TODO:       outPtr -= 2;
// TODO:    *outPtr++ = ')';
// TODO:    *outPtr++ = '\n';
// TODO:    *outPtr++ = '\0';
   return outStr;
}

static int isMouseAction(const char* action)
{
   int i;

   for (i=0; i<(int)ARRAY_SIZE(MouseActions); i++)
      if (!strcmp(action, MouseActions[i]))
         return true;
   return false;
}

static int isRedundantAction(const char* action)
{
   int i;

   for (i=0; i<(int)ARRAY_SIZE(RedundantActions); i++)
      if (!strcmp(action, RedundantActions[i]))
         return true;
   return false;
}

static int isIgnoredAction(const char* action)
{
   int i;

   for (i=0; i<(int)ARRAY_SIZE(IgnoredActions); i++)
      if (!strcmp(action, IgnoredActions[i]))
         return true;
   return false;
}

/*
** Timer proc for putting up the "Macro Command in Progress" banner if
** the process is taking too long.
*/
#define MAX_TIMEOUT_MSG_LEN (MAX_ACCEL_LEN + 60)
static void bannerTimeoutProc(void* clientData, int* id)
{
   WindowInfo* window = (WindowInfo*)clientData;
   macroCmdInfo* cmdData = (macroCmdInfo*)window->macroCmdData;
// TODO:    NeString xmCancel;
   char* cCancel = "\0";
   char message[MAX_TIMEOUT_MSG_LEN];

   cmdData->bannerIsUp = true;

// TODO:    // Extract accelerator text from menu PushButtons 
// TODO:    XtVaGetValues(window->cancelMacroItem, XmNacceleratorText, &xmCancel, NULL);
// TODO: 
// TODO:    if (!XmStringEmpty(xmCancel))
// TODO:    {
// TODO:       // Translate Motif string to char* 
// TODO:       cCancel = GetNeStringText(xmCancel);
// TODO: 
// TODO:       // Free Motif String 
// TODO:       NeStringFree(xmCancel);
// TODO:    }

   // Create message 
   if (cCancel[0] == '\0')
   {
      strncpy(message, "Macro Command in Progress", MAX_TIMEOUT_MSG_LEN);
      message[MAX_TIMEOUT_MSG_LEN - 1] = '\0';
   }
   else
   {
      sprintf(message,
              "Macro Command in Progress -- Press %s to Cancel",
              cCancel);
   }

   // Free C-string 
   free__(cCancel);

   SetModeMessage(window, message);
   cmdData->bannerTimeoutID = 0;
}

/*
** Work proc for continuing execution of a preempted macro.
**
** Xt WorkProcs are designed to run first-in first-out, which makes them
** very bad at sharing time between competing tasks.  For this reason, it's
** usually bad to use work procs anywhere where their execution is likely to
** overlap.  Using a work proc instead of a timer proc (which I usually
** prefer) here means macros will probably share time badly, but we're more
** interested in making the macros cancelable, and in continuing other work
** than having users run a bunch of them at once together.
*/
static bool continueWorkProc(void* clientData)
{
   WindowInfo* window = (WindowInfo*)clientData;
   macroCmdInfo* cmdData = (macroCmdInfo*)window->macroCmdData;
   char* errMsg;
   int stat;
   DataValue result;

   stat = ContinueMacro(cmdData->context, &result, &errMsg);
   if (stat == MACRO_ERROR)
   {
      finishMacroCmdExecution(window);
      DialogF(DF_ERR, window->mainWindow, 1, "Macro Error", "Error executing macro: %s", "OK", errMsg);
      return true;
   }
   else if (stat == MACRO_DONE)
   {
      finishMacroCmdExecution(window);
      return true;
   }
   else if (stat == MACRO_PREEMPT)
   {
      cmdData->continueWorkProcID = 0;
      return true;
   }

   // Macro exceeded time slice, re-schedule it 
   if (stat != MACRO_TIME_LIMIT)
      return true; // shouldn't happen 
   return false;
}

/*
** Copy fromString to toString replacing special characters in strings, such
** that they can be read back by the macro parser's string reader.  i.e. double
** quotes are replaced by \", backslashes are replaced with \\, C-std control
** characters like \n are replaced with their backslash counterparts.  This
** routine should be kept reasonably in sync with yylex in parse.y.  Companion
** routine escapedStringLength predicts the length needed to write the string
** when it is expanded with the additional characters.  Returns the number
** of characters to which the string expanded.
*/
static int escapeStringChars(char* fromString, char* toString)
{
   char* e, *c, *outPtr = toString;

   // substitute escape sequences 
   for (c=fromString; *c!='\0'; c++)
   {
      for (e=EscapeChars; *e!='\0'; e++)
      {
         if (*c == *e)
         {
            *outPtr++ = '\\';
            *outPtr++ = ReplaceChars[e-EscapeChars];
            break;
         }
      }
      if (*e == '\0')
         *outPtr++ = *c;
   }
   *outPtr = '\0';
   return outPtr - toString;
}

/*
** Predict the length of a string needed to hold a copy of "string" with
** special characters replaced with escape sequences by escapeStringChars.
*/
static int escapedStringLength(char* string)
{
   char* c, *e;
   int length = 0;

   // calculate length and allocate returned string 
   for (c=string; *c!='\0'; c++)
   {
      for (e=EscapeChars; *e!='\0'; e++)
      {
         if (*c == *e)
         {
            length++;
            break;
         }
      }
      length++;
   }
   return length;
}

/*
** Built-in macro subroutine for getting the length of a string
*/
static int lengthMS(WindowInfo* window, DataValue* argList, int nArgs,
                    DataValue* result, char** errMsg)
{
   char* string, stringStorage[TYPE_INT_STR_SIZE(int)];

   if (nArgs != 1)
      return wrongNArgsErr(errMsg);
   if (!readStringArg(argList[0], &string, stringStorage, errMsg))
      return false;
   result->tag = INT_TAG;
   result->val.n = strlen(string);
   return true;
}

/*
** Built-in macro subroutines for min and max
*/
static int minMS(WindowInfo* window, DataValue* argList, int nArgs,
                 DataValue* result, char** errMsg)
{
   int minVal, value, i;

   if (nArgs == 1)
      return tooFewArgsErr(errMsg);
   if (!readIntArg(argList[0], &minVal, errMsg))
      return false;
   for (i=0; i<nArgs; i++)
   {
      if (!readIntArg(argList[i], &value, errMsg))
         return false;
      minVal = value < minVal ? value : minVal;
   }
   result->tag = INT_TAG;
   result->val.n = minVal;
   return true;
}
static int maxMS(WindowInfo* window, DataValue* argList, int nArgs,
                 DataValue* result, char** errMsg)
{
   int maxVal, value, i;

   if (nArgs == 1)
      return tooFewArgsErr(errMsg);
   if (!readIntArg(argList[0], &maxVal, errMsg))
      return false;
   for (i=0; i<nArgs; i++)
   {
      if (!readIntArg(argList[i], &value, errMsg))
         return false;
      maxVal = value > maxVal ? value : maxVal;
   }
   result->tag = INT_TAG;
   result->val.n = maxVal;
   return true;
}

static int focusWindowMS(WindowInfo* window, DataValue* argList, int nArgs,
                         DataValue* result, char** errMsg)
{
   char stringStorage[TYPE_INT_STR_SIZE(int)], *string;
   WindowInfo* w;
   char fullname[MAXPATHLEN];
   char normalizedString[MAXPATHLEN];

   /* Read the argument representing the window to focus to, and translate
      it into a pointer to a real WindowInfo */
   if (nArgs != 1)
      return wrongNArgsErr(errMsg);

   if (!readStringArg(argList[0], &string, stringStorage, errMsg))
   {
      return false;
   }
   else if (!strcmp(string, "last"))
   {
      w = WindowList;
   }
   else if (!strcmp(string, "next"))
   {
      w = window->next;
   }
   else if (strlen(string) >= MAXPATHLEN)
   {
      *errMsg = "Pathname too long in focus_window()";
      return false;
   }
   else
   {
      // just use the plain name as supplied 
      for (w=WindowList; w != NULL; w = w->next)
      {
         sprintf(fullname, "%s%s", w->path, w->filename);
         if (!strcmp(string, fullname))
         {
            break;
         }
      }
      // didn't work? try normalizing the string passed in 
      if (w == NULL)
      {
         strncpy(normalizedString, string, MAXPATHLEN);
         normalizedString[MAXPATHLEN-1] = '\0';
         if (1 == NormalizePathname(normalizedString))
         {
            //  Something is broken with the input pathname. 
            *errMsg = "Pathname too long in focus_window()";
            return false;
         }
         for (w=WindowList; w != NULL; w = w->next)
         {
            sprintf(fullname, "%s%s", w->path, w->filename);
            if (!strcmp(normalizedString, fullname))
               break;
         }
      }
   }

   // If no matching window was found, return empty string and do nothing 
   if (w == NULL)
   {
      result->tag = STRING_TAG;
      result->val.str.rep = PERM_ALLOC_STR("");
      result->val.str.len = 0;
      return true;
   }

   // Change the focused window to the requested one 
   SetMacroFocusWindow(w);

// TODO:    // turn on syntax highlight that might have been deferred 
// TODO:    if (w->highlightSyntax && w->highlightData==NULL)
// TODO:       StartHighlighting(w, false);

   // Return the name of the window 
   result->tag = STRING_TAG;
   AllocNString(&result->val.str, strlen(w->path)+strlen(w->filename)+1);
   sprintf(result->val.str.rep, "%s%s", w->path, w->filename);
   return true;
}

/*
** Built-in macro subroutine for getting text from the current window's text
** buffer
*/
static int getRangeMS(WindowInfo* window, DataValue* argList, int nArgs,
                      DataValue* result, char** errMsg)
{
   int from, to;
   Ne_Text_Buffer* buf = window->buffer;
   char* rangeText;

   // Validate arguments and convert to int 
   if (nArgs != 2)
      return wrongNArgsErr(errMsg);
   if (!readIntArg(argList[0], &from, errMsg))
      return false;
   if (!readIntArg(argList[1], &to, errMsg))
      return false;
   if (from < 0) from = 0;
   if (from > buf->length) from = buf->length;
   if (to < 0) to = 0;
   if (to > buf->length) to = buf->length;
   if (from > to)
   {
      int temp = from;
      from = to;
      to = temp;
   }

   /* Copy text from buffer (this extra copy could be avoided if textBuf.c
      provided a routine for writing into a pre-allocated string) */
   result->tag = STRING_TAG;
   AllocNString(&result->val.str, to - from + 1);
   rangeText = BufGetRange(buf, from, to);
   BufUnsubstituteNullChars(rangeText, buf);
   strcpy(result->val.str.rep, rangeText);
   /* Note: after the un-substitution, it is possible that strlen() != len,
      but that's because strlen() can't deal with 0-characters. */
   free__(rangeText);
   return true;
}

/*
** Built-in macro subroutine for getting a single character at the position
** given, from the current window
*/
static int getCharacterMS(WindowInfo* window, DataValue* argList, int nArgs,
                          DataValue* result, char** errMsg)
{
   int pos;
   Ne_Text_Buffer* buf = window->buffer;

   // Validate argument and convert it to int 
   if (nArgs != 1)
      return wrongNArgsErr(errMsg);
   if (!readIntArg(argList[0], &pos, errMsg))
      return false;
   if (pos < 0) pos = 0;
   if (pos > buf->length) pos = buf->length;

   // Return the character in a pre-allocated string) 
   result->tag = STRING_TAG;
   AllocNString(&result->val.str, 2);
   result->val.str.rep[0] = BufGetCharacter(buf, pos);
   BufUnsubstituteNullChars(result->val.str.rep, buf);
   /* Note: after the un-substitution, it is possible that strlen() != len,
      but that's because strlen() can't deal with 0-characters. */
   return true;
}

/*
** Built-in macro subroutine for replacing text in the current window's text
** buffer
*/
static int replaceRangeMS(WindowInfo* window, DataValue* argList, int nArgs,
                          DataValue* result, char** errMsg)
{
   int from, to;
   char stringStorage[TYPE_INT_STR_SIZE(int)], *string;
   Ne_Text_Buffer* buf = window->buffer;

   // Validate arguments and convert to int 
   if (nArgs != 3)
      return wrongNArgsErr(errMsg);
   if (!readIntArg(argList[0], &from, errMsg))
      return false;
   if (!readIntArg(argList[1], &to, errMsg))
      return false;
   if (!readStringArg(argList[2], &string, stringStorage, errMsg))
      return false;
   if (from < 0) from = 0;
   if (from > buf->length) from = buf->length;
   if (to < 0) to = 0;
   if (to > buf->length) to = buf->length;
   if (from > to)
   {
      int temp = from;
      from = to;
      to = temp;
   }

   // Don't allow modifications if the window is read-only 
   if (IS_ANY_LOCKED(window->lockReasons))
   {
      fl_beep();
      result->tag = NO_TAG;
      return true;
   }

   /* There are no null characters in the string (because macro strings
      still have null termination), but if the string contains the
      character used by the buffer for null substitution, it could
      theoretically become a null.  In the highly unlikely event that
      all of the possible substitution characters in the buffer are used
      up, stop the macro and tell the user of the failure */
   if (!BufSubstituteNullChars(string, strlen(string), window->buffer))
   {
      *errMsg = "Too much binary data in file";
      return false;
   }

   // Do the replace 
   BufReplace(buf, from, to, string);
   result->tag = NO_TAG;
   return true;
}

/*
** Built-in macro subroutine for replacing the primary-selection selected
** text in the current window's text buffer
*/
static int replaceSelectionMS(WindowInfo* window, DataValue* argList, int nArgs,
                              DataValue* result, char** errMsg)
{
   char stringStorage[TYPE_INT_STR_SIZE(int)], *string;

   // Validate argument and convert to string 
   if (nArgs != 1)
      return wrongNArgsErr(errMsg);
   if (!readStringArg(argList[0], &string, stringStorage, errMsg))
      return false;

   // Don't allow modifications if the window is read-only 
   if (IS_ANY_LOCKED(window->lockReasons))
   {
      fl_beep();
      result->tag = NO_TAG;
      return true;
   }

   /* There are no null characters in the string (because macro strings
      still have null termination), but if the string contains the
      character used by the buffer for null substitution, it could
      theoretically become a null.  In the highly unlikely event that
      all of the possible substitution characters in the buffer are used
      up, stop the macro and tell the user of the failure */
   if (!BufSubstituteNullChars(string, strlen(string), window->buffer))
   {
      *errMsg = "Too much binary data in file";
      return false;
   }

   // Do the replace 
   BufReplaceSelected(window->buffer, string);
   result->tag = NO_TAG;
   return true;
}

/*
** Built-in macro subroutine for getting the text currently selected by
** the primary selection in the current window's text buffer, or in any
** part of screen if "any" argument is given
*/
static int getSelectionMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   char* selText;

   /* Read argument list to check for "any" keyword, and get the appropriate
      selection */
   if (nArgs != 0 && nArgs != 1)
      return wrongNArgsErr(errMsg);
   if (nArgs == 1)
   {
      if (argList[0].tag != STRING_TAG || strcmp(argList[0].val.str.rep, "any"))
      {
         *errMsg = "Unrecognized argument to %s";
         return false;
      }
      selText = GetAnySelection(window);
      if (selText == NULL)
         selText = NeNewString("");
   }
   else
   {
      selText = BufGetSelectionText(window->buffer);
      BufUnsubstituteNullChars(selText, window->buffer);
   }

   // Return the text as an allocated string 
   result->tag = STRING_TAG;
   AllocNStringCpy(&result->val.str, selText);
   free__(selText);
   return true;
}

/*
** Built-in macro subroutine for determining if implicit conversion of
** a string to number will succeed or fail
*/
static int validNumberMS(WindowInfo* window, DataValue* argList, int nArgs,
                         DataValue* result, char** errMsg)
{
   char* string, stringStorage[TYPE_INT_STR_SIZE(int)];

   if (nArgs != 1)
   {
      return wrongNArgsErr(errMsg);
   }
   if (!readStringArg(argList[0], &string, stringStorage, errMsg))
   {
      return false;
   }

   result->tag = INT_TAG;
   result->val.n = StringToNum(string, NULL);

   return true;
}

/*
** Built-in macro subroutine for replacing a substring within another string
*/
static int replaceSubstringMS(WindowInfo* window, DataValue* argList, int nArgs,
                              DataValue* result, char** errMsg)
{
   int from, to, length, replaceLen, outLen;
   char stringStorage[2][TYPE_INT_STR_SIZE(int)], *string, *replStr;

   // Validate arguments and convert to int 
   if (nArgs != 4)
      return wrongNArgsErr(errMsg);
   if (!readStringArg(argList[0], &string, stringStorage[1], errMsg))
      return false;
   if (!readIntArg(argList[1], &from, errMsg))
      return false;
   if (!readIntArg(argList[2], &to, errMsg))
      return false;
   if (!readStringArg(argList[3], &replStr, stringStorage[1], errMsg))
      return false;
   length = strlen(string);
   if (from < 0) from = 0;
   if (from > length) from = length;
   if (to < 0) to = 0;
   if (to > length) to = length;
   if (from > to)
   {
      int temp = from;
      from = to;
      to = temp;
   }

   // Allocate a new string and do the replacement 
   replaceLen = strlen(replStr);
   outLen = length - (to - from) + replaceLen;
   result->tag = STRING_TAG;
   AllocNString(&result->val.str, outLen+1);
   strncpy(result->val.str.rep, string, from);
   strncpy(&result->val.str.rep[from], replStr, replaceLen);
   strncpy(&result->val.str.rep[from + replaceLen], &string[to], length - to);
   return true;
}

/*
** Built-in macro subroutine for getting a substring of a string.
** Called as substring(string, from [, to])
*/
static int substringMS(WindowInfo* window, DataValue* argList, int nArgs,
                       DataValue* result, char** errMsg)
{
   int from, to, length;
   char stringStorage[TYPE_INT_STR_SIZE(int)], *string;

   // Validate arguments and convert to int 
   if (nArgs != 2 && nArgs != 3)
      return wrongNArgsErr(errMsg);
   if (!readStringArg(argList[0], &string, stringStorage, errMsg))
      return false;
   if (!readIntArg(argList[1], &from, errMsg))
      return false;
   length = to = strlen(string);
   if (nArgs == 3)
      if (!readIntArg(argList[2], &to, errMsg))
         return false;
   if (from < 0) from += length;
   if (from < 0) from = 0;
   if (from > length) from = length;
   if (to < 0) to += length;
   if (to < 0) to = 0;
   if (to > length) to = length;
   if (from > to) to = from;

   // Allocate a new string and copy the sub-string into it 
   result->tag = STRING_TAG;
   AllocNStringNCpy(&result->val.str, &string[from], to - from);
   return true;
}

static int toupperMS(WindowInfo* window, DataValue* argList, int nArgs,
                     DataValue* result, char** errMsg)
{
   int i, length;
   char stringStorage[TYPE_INT_STR_SIZE(int)], *string;

   // Validate arguments and convert to int 
   if (nArgs != 1)
      return wrongNArgsErr(errMsg);
   if (!readStringArg(argList[0], &string, stringStorage, errMsg))
      return false;
   length = strlen(string);

   // Allocate a new string and copy an uppercased version of the string it 
   result->tag = STRING_TAG;
   AllocNString(&result->val.str, length + 1);
   for (i=0; i<length; i++)
      result->val.str.rep[i] = toupper((unsigned char)string[i]);
   return true;
}

static int tolowerMS(WindowInfo* window, DataValue* argList, int nArgs,
                     DataValue* result, char** errMsg)
{
   int i, length;
   char stringStorage[TYPE_INT_STR_SIZE(int)], *string;

   // Validate arguments and convert to int 
   if (nArgs != 1)
      return wrongNArgsErr(errMsg);
   if (!readStringArg(argList[0], &string, stringStorage, errMsg))
      return false;
   length = strlen(string);

   // Allocate a new string and copy an lowercased version of the string it 
   result->tag = STRING_TAG;
   AllocNString(&result->val.str, length + 1);
   for (i=0; i<length; i++)
      result->val.str.rep[i] = tolower((unsigned char)string[i]);
   return true;
}

static int stringToClipboardMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
// TODO:    long itemID = 0;
// TODO:    NeString s;
// TODO:    int stat;
// TODO:    char stringStorage[TYPE_INT_STR_SIZE(int)], *string;
// TODO: 
// TODO:    // Get the string argument 
// TODO:    if (nArgs != 1)
// TODO:       return wrongNArgsErr(errMsg);
// TODO:    if (!readStringArg(argList[0], &string, stringStorage, errMsg))
// TODO:       return false;
// TODO: 
// TODO:    /* Use the XmClipboard routines to copy the text to the clipboard.
// TODO:       If errors occur, just give up.  */
// TODO:    result->tag = NO_TAG;
// TODO:    stat = SpinClipboardStartCopy(TheDisplay, XtWindow(window->textArea),
// TODO:                                  s=NeNewString("NEdit"), XtLastTimestampProcessed(TheDisplay),
// TODO:                                  window->textArea, NULL, &itemID);
// TODO:    NeStringFree(s);
// TODO:    if (stat != ClipboardSuccess)
// TODO:       return true;
// TODO:    if (SpinClipboardCopy(TheDisplay, XtWindow(window->textArea), itemID, "STRING",
// TODO:                          string, strlen(string), 0, NULL) != ClipboardSuccess)
// TODO:    {
// TODO:       SpinClipboardEndCopy(TheDisplay, XtWindow(window->textArea), itemID);
// TODO:       return true;
// TODO:    }
// TODO:    SpinClipboardEndCopy(TheDisplay, XtWindow(window->textArea), itemID);
   return true;
}

static int clipboardToStringMS(WindowInfo* window, DataValue* argList, int nArgs,
                               DataValue* result, char** errMsg)
{
   unsigned long length, retLength;
   long id = 0;

   // Should have no arguments 
   if (nArgs != 0)
      return wrongNArgsErr(errMsg);

   // Ask if there's a string in the clipboard, and get its length 
// TODO:    if (SpinClipboardInquireLength(TheDisplay, XtWindow(window->mainWindow), "STRING", &length) != ClipboardSuccess)
// TODO:    {
// TODO:       result->tag = STRING_TAG;
// TODO:       result->val.str.rep = PERM_ALLOC_STR("");
// TODO:       result->val.str.len = 0;
// TODO:       /*
// TODO:        * Possibly, the clipboard can remain in a locked state after
// TODO:        * a failure, so we try to remove the lock, just to be sure.
// TODO:        */
// TODO:       SpinClipboardUnlock(TheDisplay, XtWindow(window->mainWindow));
// TODO:       return true;
// TODO:    }

   // Allocate a new string to hold the data 
   result->tag = STRING_TAG;
   AllocNString(&result->val.str, (int)length + 1);

// TODO:    // Copy the clipboard contents to the string 
// TODO:    if (SpinClipboardRetrieve(TheDisplay, XtWindow(window->mainWindow), "STRING",
// TODO:                              result->val.str.rep, length, &retLength, &id) != ClipboardSuccess)
// TODO:    {
// TODO:       retLength = 0;
// TODO:       /*
// TODO:        * Possibly, the clipboard can remain in a locked state after
// TODO:        * a failure, so we try to remove the lock, just to be sure.
// TODO:        */
// TODO:       SpinClipboardUnlock(TheDisplay, XtWindow(window->mainWindow));
// TODO:    }
// TODO:    result->val.str.rep[retLength] = '\0';
// TODO:    result->val.str.len = retLength;
// TODO: 
   return true;
}


/*
** Built-in macro subroutine for reading the contents of a text file into
** a string.  On success, returns 1 in $readStatus, and the contents of the
** file as a string in the subroutine return value.  On failure, returns
** the empty string "" and an 0 $readStatus.
*/
static int readFileMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   char stringStorage[TYPE_INT_STR_SIZE(int)], *name;
   struct stat statbuf;
   FILE* fp;
   int readLen;

   // Validate arguments and convert to int 
   if (nArgs != 1)
      return wrongNArgsErr(errMsg);
   if (!readStringArg(argList[0], &name, stringStorage, errMsg))
      return false;

   // Read the whole file into an allocated string 
   if ((fp = fopen(name, "r")) == NULL)
      goto errorNoClose;
   if (fstat(fileno(fp), &statbuf) != 0)
      goto error;
   result->tag = STRING_TAG;
   AllocNString(&result->val.str, statbuf.st_size+1);
   readLen = fread(result->val.str.rep, sizeof(char), statbuf.st_size+1, fp);
   if (ferror(fp))
      goto error;
   if (!feof(fp))
   {
      // Couldn't trust file size. Use slower but more general method 
      int chunkSize = 1024;
      char* buffer;

      buffer = (char*)malloc__(readLen * sizeof(char));
      memcpy(buffer, result->val.str.rep, readLen * sizeof(char));
// TODO:       while (!feof(fp))
// TODO:       {
// TODO:          buffer = XtRealloc(buffer, (readLen+chunkSize)*sizeof(char));
// TODO:          readLen += fread(&buffer[readLen], sizeof(char), chunkSize, fp);
// TODO:          if (ferror(fp))
// TODO:          {
// TODO:             free__(buffer);
// TODO:             goto error;
// TODO:          }
// TODO:       }
      AllocNString(&result->val.str, readLen + 1);
      memcpy(result->val.str.rep, buffer, readLen * sizeof(char));
      free__(buffer);
   }
   fclose(fp);

   // Return the results 
   ReturnGlobals[READ_STATUS]->value.tag = INT_TAG;
   ReturnGlobals[READ_STATUS]->value.val.n = true;
   return true;

error:
   fclose(fp);

errorNoClose:
   ReturnGlobals[READ_STATUS]->value.tag = INT_TAG;
   ReturnGlobals[READ_STATUS]->value.val.n = false;
   result->tag = STRING_TAG;
   result->val.str.rep = PERM_ALLOC_STR("");
   result->val.str.len = 0;
   return true;
}

/*
** Built-in macro subroutines for writing or appending a string (parameter $1)
** to a file named in parameter $2. Returns 1 on successful write, or 0 if
** unsuccessful.
*/
static int writeFileMS(WindowInfo* window, DataValue* argList, int nArgs,
                       DataValue* result, char** errMsg)
{
   return writeOrAppendFile(false, window, argList, nArgs, result, errMsg);
}

static int appendFileMS(WindowInfo* window, DataValue* argList, int nArgs,
                        DataValue* result, char** errMsg)
{
   return writeOrAppendFile(true, window, argList, nArgs, result, errMsg);
}

static int writeOrAppendFile(int append, WindowInfo* window,
                             DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   char stringStorage[2][TYPE_INT_STR_SIZE(int)], *name, *string;
   FILE* fp;

   // Validate argument 
   if (nArgs != 2)
      return wrongNArgsErr(errMsg);
   if (!readStringArg(argList[0], &string, stringStorage[1], errMsg))
      return false;
   if (!readStringArg(argList[1], &name, stringStorage[0], errMsg))
      return false;

   // open the file 
   if ((fp = fopen(name, append ? "a" : "w")) == NULL)
   {
      result->tag = INT_TAG;
      result->val.n = false;
      return true;
   }

   // write the string to the file 
   fwrite(string, sizeof(char), strlen(string), fp);
   if (ferror(fp))
   {
      fclose(fp);
      result->tag = INT_TAG;
      result->val.n = false;
      return true;
   }
   fclose(fp);

   // return the status 
   result->tag = INT_TAG;
   result->val.n = true;
   return true;
}

/*
** Built-in macro subroutine for searching silently in a window without
** dialogs, beeps, or changes to the selection.  Arguments are: $1: string to
** search for, $2: starting position. Optional arguments may include the
** strings: "wrap" to make the search wrap around the beginning or end of the
** string, "backward" or "forward" to change the search direction ("forward" is
** the default), "literal", "case" or "regex" to change the search type
** (default is "literal").
**
** Returns the starting position of the match, or -1 if nothing matched.
** also returns the ending position of the match in $searchEndPos
*/
static int searchMS(WindowInfo* window, DataValue* argList, int nArgs,
                    DataValue* result, char** errMsg)
{
   DataValue newArgList[9];

   /* Use the search string routine, by adding the buffer contents as
      the string argument */
   if (nArgs > 8)
      return wrongNArgsErr(errMsg);

   /* we remove constness from BufAsString() result since we know
      searchStringMS will not modify the result */
   newArgList[0].tag = STRING_TAG;
   newArgList[0].val.str.rep = (char*)BufAsString(window->buffer);
   newArgList[0].val.str.len = window->buffer->length;

   // copy other arguments to the new argument list 
   memcpy(&newArgList[1], argList, nArgs * sizeof(DataValue));

   return searchStringMS(window, newArgList, nArgs+1, result, errMsg);
}

/*
** Built-in macro subroutine for searching a string.  Arguments are $1:
** string to search in, $2: string to search for, $3: starting position.
** Optional arguments may include the strings: "wrap" to make the search
** wrap around the beginning or end of the string, "backward" or "forward"
** to change the search direction ("forward" is the default), "literal",
** "case" or "regex" to change the search type (default is "literal").
**
** Returns the starting position of the match, or -1 if nothing matched.
** also returns the ending position of the match in $searchEndPos
*/
static int searchStringMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   int beginPos, wrap, direction, found = false, foundStart, foundEnd, type;
   int skipSearch = false, len;
   char stringStorage[2][TYPE_INT_STR_SIZE(int)], *string, *searchStr;

   // Validate arguments and convert to proper types 
   if (nArgs < 3)
      return tooFewArgsErr(errMsg);
   if (!readStringArg(argList[0], &string, stringStorage[0], errMsg))
      return false;
   if (!readStringArg(argList[1], &searchStr, stringStorage[1], errMsg))
      return false;
   if (!readIntArg(argList[2], &beginPos, errMsg))
      return false;
   if (!readSearchArgs(&argList[3], nArgs-3, &direction, &type, &wrap, errMsg))
      return false;

   len = argList[0].val.str.len;
   if (beginPos > len)
   {
      if (direction == SEARCH_FORWARD)
      {
         if (wrap)
         {
            beginPos = 0; // Wrap immediately 
         }
         else
         {
            found = false;
            skipSearch = true;
         }
      }
      else
      {
         beginPos = len;
      }
   }
   else if (beginPos < 0)
   {
      if (direction == SEARCH_BACKWARD)
      {
         if (wrap)
         {
            beginPos = len; // Wrap immediately 
         }
         else
         {
            found = false;
            skipSearch = true;
         }
      }
      else
      {
         beginPos = 0;
      }
   }

   if (!skipSearch)
      found = SearchString(string, searchStr, direction, type, wrap, beginPos, &foundStart, &foundEnd, NULL, NULL, GetWindowDelimiters(window));

   // Return the results 
   ReturnGlobals[SEARCH_END]->value.tag = INT_TAG;
   ReturnGlobals[SEARCH_END]->value.val.n = found ? foundEnd : 0;
   result->tag = INT_TAG;
   result->val.n = found ? foundStart : -1;
   return true;
}

/*
** Built-in macro subroutine for replacing all occurences of a search string in
** a string with a replacement string.  Arguments are $1: string to search in,
** $2: string to search for, $3: replacement string. Also takes an optional
** search type: one of "literal", "case" or "regex" (default is "literal"), and
** an optional "copy" argument.
**
** Returns a new string with all of the replacements done.  If no replacements
** were performed and "copy" was specified, returns a copy of the original
** string.  Otherwise returns an empty string ("").
*/
static int replaceInStringMS(WindowInfo* window, DataValue* argList, int nArgs,
                             DataValue* result, char** errMsg)
{
   char stringStorage[3][TYPE_INT_STR_SIZE(int)], *string, *searchStr, *replaceStr;
   char* argStr, *replacedStr;
   int searchType = SEARCH_LITERAL, copyStart, copyEnd;
   int replacedLen, replaceEnd, force=false, i;

   // Validate arguments and convert to proper types 
   if (nArgs < 3 || nArgs > 5)
      return wrongNArgsErr(errMsg);
   if (!readStringArg(argList[0], &string, stringStorage[0], errMsg))
      return false;
   if (!readStringArg(argList[1], &searchStr, stringStorage[1], errMsg))
      return false;
   if (!readStringArg(argList[2], &replaceStr, stringStorage[2], errMsg))
      return false;
   for (i = 3; i < nArgs; i++)
   {
      // Read the optional search type and force arguments 
      if (!readStringArg(argList[i], &argStr, stringStorage[2], errMsg))
         return false;
      if (!StringToSearchType(argStr, &searchType))
      {
         // It's not a search type.  is it "copy"? 
         if (!strcmp(argStr, "copy"))
         {
            force = true;
         }
         else
         {
            *errMsg = "unrecognized argument to %s";
            return false;
         }
      }
   }

   // Do the replace 
   replacedStr = ReplaceAllInString(string, searchStr, replaceStr, searchType, &copyStart, &copyEnd, &replacedLen, GetWindowDelimiters(window));

   // Return the results 
   result->tag = STRING_TAG;
   if (replacedStr == NULL)
   {
      if (force)
      {
         // Just copy the original DataValue 
         if (argList[0].tag == STRING_TAG)
         {
            result->val.str.rep = argList[0].val.str.rep;
            result->val.str.len = argList[0].val.str.len;
         }
         else
         {
            AllocNStringCpy(&result->val.str, string);
         }
      }
      else
      {
         result->val.str.rep = PERM_ALLOC_STR("");
         result->val.str.len = 0;
      }
   }
   else
   {
      size_t remainder = strlen(&string[copyEnd]);
      replaceEnd = copyStart + replacedLen;
      AllocNString(&result->val.str, replaceEnd + remainder + 1);
      strncpy(result->val.str.rep, string, copyStart);
      strcpy(&result->val.str.rep[copyStart], replacedStr);
      strcpy(&result->val.str.rep[replaceEnd], &string[copyEnd]);
      free__(replacedStr);
   }
   return true;
}

static int readSearchArgs(DataValue* argList, int nArgs, int* searchDirection,
                          int* searchType, int* wrap, char** errMsg)
{
   int i;
   char* argStr, stringStorage[TYPE_INT_STR_SIZE(int)];

   *wrap = false;
   *searchDirection = SEARCH_FORWARD;
   *searchType = SEARCH_LITERAL;
   for (i=0; i<nArgs; i++)
   {
      if (!readStringArg(argList[i], &argStr, stringStorage, errMsg))
         return false;
      else if (!strcmp(argStr, "wrap"))
         *wrap = true;
      else if (!strcmp(argStr, "nowrap"))
         *wrap = false;
      else if (!strcmp(argStr, "backward"))
         *searchDirection = SEARCH_BACKWARD;
      else if (!strcmp(argStr, "forward"))
         *searchDirection = SEARCH_FORWARD;
      else if (!StringToSearchType(argStr, searchType))
      {
         *errMsg = "Unrecognized argument to %s";
         return false;
      }
   }
   return true;
}

static int setCursorPosMS(WindowInfo* window, DataValue* argList, int nArgs,
                          DataValue* result, char** errMsg)
{
   int pos;

   // Get argument and convert to int 
   if (nArgs != 1)
      return wrongNArgsErr(errMsg);
   if (!readIntArg(argList[0], &pos, errMsg))
      return false;

   // Set the position 
   TextSetCursorPos(window->lastFocus, pos);
   result->tag = NO_TAG;
   return true;
}

static int selectMS(WindowInfo* window, DataValue* argList, int nArgs,
                    DataValue* result, char** errMsg)
{
   int start, end, startTmp;

   // Get arguments and convert to int 
   if (nArgs != 2)
      return wrongNArgsErr(errMsg);
   if (!readIntArg(argList[0], &start, errMsg))
      return false;
   if (!readIntArg(argList[1], &end, errMsg))
      return false;

   // Verify integrity of arguments 
   if (start > end)
   {
      startTmp = start;
      start = end;
      end = startTmp;
   }
   if (start < 0) start = 0;
   if (start > window->buffer->length) start = window->buffer->length;
   if (end < 0) end = 0;
   if (end > window->buffer->length) end = window->buffer->length;

   // Make the selection 
   BufSelect(window->buffer, start, end);
   result->tag = NO_TAG;
   return true;
}

static int selectRectangleMS(WindowInfo* window, DataValue* argList, int nArgs,
                             DataValue* result, char** errMsg)
{
   int start, end, left, right;

   // Get arguments and convert to int 
   if (nArgs != 4)
      return wrongNArgsErr(errMsg);
   if (!readIntArg(argList[0], &start, errMsg))
      return false;
   if (!readIntArg(argList[1], &end, errMsg))
      return false;
   if (!readIntArg(argList[2], &left, errMsg))
      return false;
   if (!readIntArg(argList[3], &right, errMsg))
      return false;

   // Make the selection 
   BufRectSelect(window->buffer, start, end, left, right);
   result->tag = NO_TAG;
   return true;
}

/*
** Macro subroutine to ring the bell
*/
static int beepMS(WindowInfo* window, DataValue* argList, int nArgs,
                  DataValue* result, char** errMsg)
{
   if (nArgs != 0)
      return wrongNArgsErr(errMsg);
   fl_beep();
   result->tag = NO_TAG;
   return true;
}

static int tPrintMS(WindowInfo* window, DataValue* argList, int nArgs,
                    DataValue* result, char** errMsg)
{
   char stringStorage[TYPE_INT_STR_SIZE(int)], *string;
   int i;

   if (nArgs == 0)
      return tooFewArgsErr(errMsg);
   for (i=0; i<nArgs; i++)
   {
      if (!readStringArg(argList[i], &string, stringStorage, errMsg))
         return false;
      printf("%s%s", string, i==nArgs-1 ? "" : " ");
   }
   fflush(stdout);
   result->tag = NO_TAG;
   return true;
}

/*
** Built-in macro subroutine for getting the value of an environment variable
*/
static int getenvMS(WindowInfo* window, DataValue* argList, int nArgs,
                    DataValue* result, char** errMsg)
{
   char stringStorage[1][TYPE_INT_STR_SIZE(int)];
   char* name;
   char* value;

   // Get name of variable to get 
   if (nArgs != 1)
      return wrongNArgsErr(errMsg);
   if (!readStringArg(argList[0], &name, stringStorage[0], errMsg))
   {
      *errMsg = "argument to %s must be a string";
      return false;
   }
   value = getenv(name);
   if (value == NULL)
      value = "";

   // Return the text as an allocated string 
   result->tag = STRING_TAG;
   AllocNStringCpy(&result->val.str, value);
   return true;
}

static int shellCmdMS(WindowInfo* window, DataValue* argList, int nArgs,
                      DataValue* result, char** errMsg)
{
   char stringStorage[2][TYPE_INT_STR_SIZE(int)], *cmdString, *inputString;

   if (nArgs != 2)
      return wrongNArgsErr(errMsg);
   if (!readStringArg(argList[0], &cmdString, stringStorage[0], errMsg))
      return false;
   if (!readStringArg(argList[1], &inputString, stringStorage[1], errMsg))
      return false;

   /* Shell command execution requires that the macro be suspended, so
      this subroutine can't be run if macro execution can't be interrupted */
   if (MacroRunWindow()->macroCmdData == NULL)
   {
      *errMsg = "%s can't be called from non-suspendable context";
      return false;
   }

#ifdef VMS
   *errMsg = "Shell commands not supported under VMS";
   return false;
#else
   ShellCmdToMacroString(window, cmdString, inputString);
   result->tag = INT_TAG;
   result->val.n = 0;
   return true;
#endif //VMS
}

/*
** Method used by ShellCmdToMacroString (called by shellCmdMS), for returning
** macro string and exit status after the execution of a shell command is
** complete.  (Sorry about the poor modularity here, it's just not worth
** teaching other modules about macro return globals, since other than this,
** they're not used outside of macro.c)
*/
void ReturnShellCommandOutput(WindowInfo* window, const char* outText, int status)
{
   DataValue retVal;
   macroCmdInfo* cmdData = (macroCmdInfo*)window->macroCmdData;

   if (cmdData == NULL)
      return;
   retVal.tag = STRING_TAG;
   AllocNStringCpy(&retVal.val.str, outText);
   ModifyReturnedValue(cmdData->context, retVal);
   ReturnGlobals[SHELL_CMD_STATUS]->value.tag = INT_TAG;
   ReturnGlobals[SHELL_CMD_STATUS]->value.val.n = status;
}

static int dialogMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
// TODO:    macroCmdInfo* cmdData;
// TODO:    char stringStorage[TYPE_INT_STR_SIZE(int)];
// TODO:    char btnStorage[TYPE_INT_STR_SIZE(int)];
// TODO:    char* btnLabel;
// TODO:    char* message;
// TODO:    Arg al[20];
// TODO:    int ac;
// TODO:    Fl_Widget* dialog, *btn;
// TODO:    int i, nBtns;
// TODO:    NeString s1, s2;
// TODO: 
// TODO:    /* Ignore the focused window passed as the function argument and put
// TODO:       the dialog up over the window which is executing the macro */
// TODO:    window = MacroRunWindow();
// TODO:    cmdData = (macroCmdInfo*)window->macroCmdData;
// TODO: 
// TODO:    /* Dialogs require macro to be suspended and interleaved with other macros.
// TODO:       This subroutine can't be run if macro execution can't be interrupted */
// TODO:    if (!cmdData)
// TODO:    {
// TODO:       *errMsg = "%s can't be called from non-suspendable context";
// TODO:       return false;
// TODO:    }
// TODO: 
// TODO:    /* Read and check the arguments.  The first being the dialog message,
// TODO:       and the rest being the button labels */
// TODO:    if (nArgs == 0)
// TODO:    {
// TODO:       *errMsg = "%s subroutine called with no arguments";
// TODO:       return false;
// TODO:    }
// TODO:    if (!readStringArg(argList[0], &message, stringStorage, errMsg))
// TODO:    {
// TODO:       return false;
// TODO:    }
// TODO: 
// TODO:    // check that all button labels can be read 
// TODO:    for (i=1; i<nArgs; i++)
// TODO:    {
// TODO:       if (!readStringArg(argList[i], &btnLabel, btnStorage, errMsg))
// TODO:       {
// TODO:          return false;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    // pick up the first button 
// TODO:    if (nArgs == 1)
// TODO:    {
// TODO:       btnLabel = "OK";
// TODO:       nBtns = 1;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       nBtns = nArgs - 1;
// TODO:       argList++;
// TODO:       readStringArg(argList[0], &btnLabel, btnStorage, errMsg);
// TODO:    }
// TODO: 
// TODO:    // Create the message box dialog widget and its dialog shell parent 
// TODO:    ac = 0;
// TODO:    XtSetArg(al[ac], XmNtitle, " ");
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNmessageString, s1=MKSTRING(message));
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNokLabelString, s2=NeNewString(btnLabel));
// TODO:    ac++;
// TODO:    dialog = CreateMessageDialog(window->mainWindow, "macroDialog", al, ac);
// TODO:    if (1 == nArgs)
// TODO:    {
// TODO:       //  Only set margin width for the default OK button  
// TODO:       XtVaSetValues(XmMessageBoxGetChild(dialog, XmDIALOG_OK_BUTTON),
// TODO:                     XmNmarginWidth, BUTTON_WIDTH_MARGIN,
// TODO:                     NULL);
// TODO:    }
// TODO: 
// TODO:    NeStringFree(s1);
// TODO:    NeStringFree(s2);
// TODO:    AddMotifCloseCallback(XtParent(dialog), dialogCloseCB, window);
// TODO:    XtAddCallback(dialog, XmNokCallback, dialogBtnCB, window);
// TODO:    XtVaSetValues(XmMessageBoxGetChild(dialog, XmDIALOG_OK_BUTTON),
// TODO:                  XmNuserData, (void*)1, NULL);
// TODO:    cmdData->dialog = dialog;
// TODO: 
// TODO:    // Unmanage default buttons, except for "OK" 
// TODO:    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
// TODO:    XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
// TODO: 
// TODO:    /* Make callback for the unmanaged cancel button (which can
// TODO:       still get executed via the esc key) activate close box action */
// TODO:    XtAddCallback(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON),
// TODO:                  XmNactivateCallback, dialogCloseCB, window);
// TODO: 
// TODO:    // Add user specified buttons (1st is already done) 
// TODO:    for (i=1; i<nBtns; i++)
// TODO:    {
// TODO:       readStringArg(argList[i], &btnLabel, btnStorage, errMsg);
// TODO:       btn = XtVaCreateManagedWidget("mdBtn", xmPushButtonWidgetClass, dialog,
// TODO:                                     XmNlabelString, s1=NeNewString(btnLabel),
// TODO:                                     XmNuserData, (void*)(i+1), NULL);
// TODO:       XtAddCallback(btn, XmNactivateCallback, dialogBtnCB, window);
// TODO:       NeStringFree(s1);
// TODO:    }
// TODO: 
// TODO: #ifdef LESSTIF_VERSION
// TODO:    /* Workaround for Lesstif (e.g. v2.1 r0.93.18) that doesn't handle
// TODO:       the escape key for closing the dialog (probably because the
// TODO:       cancel button is not managed). */
// TODO:    XtAddEventHandler(dialog, KeyPressMask, false, dialogEscCB,
// TODO:                      (void*)window);
// TODO:    XtGrabKey(dialog, XKeysymToKeycode(XtDisplay(dialog), XK_Escape), 0,
// TODO:              true, GrabModeAsync, GrabModeAsync);
// TODO: #endif // LESSTIF_VERSION 
// TODO: 
// TODO:    // Put up the dialog 
// TODO:    ManageDialogCenteredOnPointer(dialog);
// TODO: 
// TODO:    // Stop macro execution until the dialog is complete 
// TODO:    PreemptMacro();
// TODO: 
// TODO:    // Return placeholder result.  Value will be changed by button callback 
// TODO:    result->tag = INT_TAG;
// TODO:    result->val.n = 0;
   return true;
}

// TODO: static void dialogBtnCB(Fl_Widget* w, void* data)
// TODO: {
// TODO:    WindowInfo* window = (WindowInfo*)clientData;
// TODO:    macroCmdInfo* cmdData = window->macroCmdData;
// TODO:    void* userData;
// TODO:    DataValue retVal;
// TODO: 
// TODO:    /* Return the index of the button which was pressed (stored in the userData
// TODO:       field of the button widget).  The 1st button, being a gadget, is not
// TODO:       returned in w. */
// TODO:    if (cmdData == NULL)
// TODO:       return; // shouldn't happen 
// TODO:    if (XtClass(w) == xmPushButtonWidgetClass)
// TODO:    {
// TODO:       XtVaGetValues(w, XmNuserData, &userData, NULL);
// TODO:       retVal.val.n = (int)userData;
// TODO:    }
// TODO:    else
// TODO:       retVal.val.n = 1;
// TODO:    retVal.tag = INT_TAG;
// TODO:    ModifyReturnedValue(cmdData->context, retVal);
// TODO: 
// TODO:    // Pop down the dialog 
// TODO:    XtDestroyWidget(XtParent(cmdData->dialog));
// TODO:    cmdData->dialog = NULL;
// TODO: 
// TODO:    // Continue preempted macro execution 
// TODO:    ResumeMacroExecution(window);
// TODO: }

static void dialogCloseCB(Fl_Widget* w, void* data)
{
   WindowInfo* window = (WindowInfo*)data;
   macroCmdInfo* cmdData = (macroCmdInfo*)window->macroCmdData;
   DataValue retVal;

// TODO:    // Return 0 to show that the dialog was closed via the window close box 
// TODO:    retVal.val.n = 0;
// TODO:    retVal.tag = INT_TAG;
// TODO:    ModifyReturnedValue(cmdData->context, retVal);
// TODO: 
// TODO:    // Pop down the dialog 
// TODO:    XtDestroyWidget(XtParent(cmdData->dialog));
// TODO:    cmdData->dialog = NULL;

   // Continue preempted macro execution 
   ResumeMacroExecution(window);
}

#ifdef LESSTIF_VERSION
static void dialogEscCB(Fl_Widget* w, void* clientData, int event,
                        bool* cont)
{
   if (event->xkey.keycode != XKeysymToKeycode(XtDisplay(w), XK_Escape))
      return;
   if (clientData != NULL)
   {
      dialogCloseCB(w, (WindowInfo*)clientData, NULL);
   }
   *cont = false;
}
#endif // LESSTIF_VERSION 

static int stringDialogMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
// TODO:    macroCmdInfo* cmdData;
// TODO:    char stringStorage[TYPE_INT_STR_SIZE(int)];
// TODO:    char btnStorage[TYPE_INT_STR_SIZE(int)];
// TODO:    char* btnLabel;
// TODO:    char* message;
// TODO:    Fl_Widget* dialog, *btn;
// TODO:    int i, nBtns;
// TODO:    NeString s1, s2;
// TODO:    Arg al[20];
// TODO:    int ac;
// TODO: 
// TODO:    /* Ignore the focused window passed as the function argument and put
// TODO:       the dialog up over the window which is executing the macro */
// TODO:    window = MacroRunWindow();
// TODO:    cmdData = (macroCmdInfo*)window->macroCmdData;
// TODO: 
// TODO:    /* Dialogs require macro to be suspended and interleaved with other macros.
// TODO:       This subroutine can't be run if macro execution can't be interrupted */
// TODO:    if (!cmdData)
// TODO:    {
// TODO:       *errMsg = "%s can't be called from non-suspendable context";
// TODO:       return false;
// TODO:    }
// TODO: 
// TODO:    /* Read and check the arguments.  The first being the dialog message,
// TODO:       and the rest being the button labels */
// TODO:    if (nArgs == 0)
// TODO:    {
// TODO:       *errMsg = "%s subroutine called with no arguments";
// TODO:       return false;
// TODO:    }
// TODO:    if (!readStringArg(argList[0], &message, stringStorage, errMsg))
// TODO:    {
// TODO:       return false;
// TODO:    }
// TODO:    // check that all button labels can be read 
// TODO:    for (i=1; i<nArgs; i++)
// TODO:    {
// TODO:       if (!readStringArg(argList[i], &btnLabel, stringStorage, errMsg))
// TODO:       {
// TODO:          return false;
// TODO:       }
// TODO:    }
// TODO:    if (nArgs == 1)
// TODO:    {
// TODO:       btnLabel = "OK";
// TODO:       nBtns = 1;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       nBtns = nArgs - 1;
// TODO:       argList++;
// TODO:       readStringArg(argList[0], &btnLabel, btnStorage, errMsg);
// TODO:    }
// TODO: 
// TODO:    // Create the selection box dialog widget and its dialog shell parent 
// TODO:    ac = 0;
// TODO:    XtSetArg(al[ac], XmNtitle, " ");
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNselectionLabelString, s1=MKSTRING(message));
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNokLabelString, s2=NeNewString(btnLabel));
// TODO:    ac++;
// TODO:    dialog = CreatePromptDialog(window->mainWindow, "macroStringDialog", al, ac);
// TODO:    if (1 == nArgs)
// TODO:    {
// TODO:       //  Only set margin width for the default OK button  
// TODO:       XtVaSetValues(XmSelectionBoxGetChild(dialog, XmDIALOG_OK_BUTTON),
// TODO:                     XmNmarginWidth, BUTTON_WIDTH_MARGIN,
// TODO:                     NULL);
// TODO:    }
// TODO: 
// TODO:    NeStringFree(s1);
// TODO:    NeStringFree(s2);
// TODO:    AddMotifCloseCallback(XtParent(dialog), stringDialogCloseCB, window);
// TODO:    XtAddCallback(dialog, XmNokCallback, stringDialogBtnCB, window);
// TODO:    XtVaSetValues(XmSelectionBoxGetChild(dialog, XmDIALOG_OK_BUTTON),
// TODO:                  XmNuserData, (void*)1, NULL);
// TODO:    cmdData->dialog = dialog;
// TODO: 
// TODO:    // Unmanage unneded widgets 
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
// TODO: 
// TODO:    /* Make callback for the unmanaged cancel button (which can
// TODO:       still get executed via the esc key) activate close box action */
// TODO:    XtAddCallback(XmSelectionBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON),
// TODO:                  XmNactivateCallback, stringDialogCloseCB, window);
// TODO: 
// TODO:    /* Add user specified buttons (1st is already done).  Selection box
// TODO:       requires a place-holder widget to be added before buttons can be
// TODO:       added, that's what the separator below is for */
// TODO:    XtVaCreateWidget("x", xmSeparatorWidgetClass, dialog, NULL);
// TODO:    for (i=1; i<nBtns; i++)
// TODO:    {
// TODO:       readStringArg(argList[i], &btnLabel, btnStorage, errMsg);
// TODO:       btn = XtVaCreateManagedWidget("mdBtn", xmPushButtonWidgetClass, dialog,
// TODO:                                     XmNlabelString, s1=NeNewString(btnLabel),
// TODO:                                     XmNuserData, (void*)(i+1), NULL);
// TODO:       XtAddCallback(btn, XmNactivateCallback, stringDialogBtnCB, window);
// TODO:       NeStringFree(s1);
// TODO:    }
// TODO: 
// TODO: #ifdef LESSTIF_VERSION
// TODO:    /* Workaround for Lesstif (e.g. v2.1 r0.93.18) that doesn't handle
// TODO:       the escape key for closing the dialog (probably because the
// TODO:       cancel button is not managed). */
// TODO:    XtAddEventHandler(dialog, KeyPressMask, false, stringDialogEscCB,
// TODO:                      (void*)window);
// TODO:    XtGrabKey(dialog, XKeysymToKeycode(XtDisplay(dialog), XK_Escape), 0,
// TODO:              true, GrabModeAsync, GrabModeAsync);
// TODO: #endif // LESSTIF_VERSION 
// TODO: 
// TODO:    // Put up the dialog 
// TODO:    ManageDialogCenteredOnPointer(dialog);
// TODO: 
// TODO:    // Stop macro execution until the dialog is complete 
// TODO:    PreemptMacro();
// TODO: 
// TODO:    // Return placeholder result.  Value will be changed by button callback 
// TODO:    result->tag = INT_TAG;
// TODO:    result->val.n = 0;
   return true;
}

static void stringDialogBtnCB(Fl_Widget* w, void* clientData, void* callData)
{
   WindowInfo* window = (WindowInfo*)clientData;
// TODO:    macroCmdInfo* cmdData = window->macroCmdData;
// TODO:    void* userData;
// TODO:    DataValue retVal;
// TODO:    char* text;
// TODO:    int btnNum;
// TODO: 
// TODO:    // shouldn't happen, but would crash if it did 
// TODO:    if (cmdData == NULL)
// TODO:       return;
// TODO: 
// TODO:    // Return the string entered in the selection text area 
// TODO:    text = NeTextGetString(XmSelectionBoxGetChild(cmdData->dialog,
// TODO:                           XmDIALOG_TEXT));
// TODO:    retVal.tag = STRING_TAG;
// TODO:    AllocNStringCpy(&retVal.val.str, text);
// TODO:    free__(text);
// TODO:    ModifyReturnedValue(cmdData->context, retVal);
// TODO: 
// TODO:    /* Find the index of the button which was pressed (stored in the userData
// TODO:       field of the button widget).  The 1st button, being a gadget, is not
// TODO:       returned in w. */
// TODO:    if (XtClass(w) == xmPushButtonWidgetClass)
// TODO:    {
// TODO:       XtVaGetValues(w, XmNuserData, &userData, NULL);
// TODO:       btnNum = (int)userData;
// TODO:    }
// TODO:    else
// TODO:       btnNum = 1;
// TODO: 
// TODO:    // Return the button number in the global variable $string_dialog_button 
// TODO:    ReturnGlobals[STRING_DIALOG_BUTTON]->value.tag = INT_TAG;
// TODO:    ReturnGlobals[STRING_DIALOG_BUTTON]->value.val.n = btnNum;
// TODO: 
// TODO:    // Pop down the dialog 
// TODO:    XtDestroyWidget(XtParent(cmdData->dialog));
// TODO:    cmdData->dialog = NULL;
// TODO: 
// TODO:    // Continue preempted macro execution 
   ResumeMacroExecution(window);
}

static void stringDialogCloseCB(Fl_Widget* w, void* clientData,
                                void* callData)
{
   WindowInfo* window = (WindowInfo*)clientData;
   macroCmdInfo* cmdData = (macroCmdInfo*)window->macroCmdData;
   DataValue retVal;

   // shouldn't happen, but would crash if it did 
   if (cmdData == NULL)
      return;

   // Return an empty string 
   retVal.tag = STRING_TAG;
   retVal.val.str.rep = PERM_ALLOC_STR("");
   retVal.val.str.len = 0;
   ModifyReturnedValue(cmdData->context, retVal);

   // Return button number 0 in the global variable $string_dialog_button 
   ReturnGlobals[STRING_DIALOG_BUTTON]->value.tag = INT_TAG;
   ReturnGlobals[STRING_DIALOG_BUTTON]->value.val.n = 0;

   // Pop down the dialog 
// TODO:    XtDestroyWidget(XtParent(cmdData->dialog));
   cmdData->dialog = NULL;

   // Continue preempted macro execution 
   ResumeMacroExecution(window);
}

#ifdef LESSTIF_VERSION
static void stringDialogEscCB(Fl_Widget* w, void* clientData, int event,
                              bool* cont)
{
   if (event->xkey.keycode != XKeysymToKeycode(XtDisplay(w), XK_Escape))
      return;
   if (clientData != NULL)
   {
      stringDialogCloseCB(w, (WindowInfo*)clientData, NULL);
   }
   *cont = false;
}
#endif // LESSTIF_VERSION 

/*
** A subroutine to put up a calltip
** First arg is either text to be displayed or a key for tip/tag lookup.
** Optional second arg is the buffer position beneath which to display the
**      upper-left corner of the tip.  Default (or -1) puts it under the cursor.
** Additional optional arguments:
**      "tipText": (default) Indicates first arg is text to be displayed in tip.
**      "tipKey":   Indicates first arg is key in calltips database.  If key
**                  is not found in tip database then the tags database is also
**                  searched.
**      "tagKey":   Indicates first arg is key in tags database.  (Skips
**                  search in calltips database.)
**      "center":   Horizontally center the calltip at the position
**      "right":    Put the right edge of the calltip at the position
**                  "center" and "right" cannot both be specified.
**      "above":    Place the calltip above the position
**      "strict":   Don't move the calltip to keep it on-screen and away
**                  from the cursor's line.
**
** Returns the new calltip's ID on success, 0 on failure.
**
** Does this need to go on IgnoredActions?  I don't think so, since
** showing a calltip may be part of the action you want to learn.
*/
static int calltipMS(WindowInfo* window, DataValue* argList, int nArgs,
                     DataValue* result, char** errMsg)
{
   char stringStorage[TYPE_INT_STR_SIZE(int)], *tipText, *txtArg;
   bool anchored = false, lookup = true;
   int mode = -1, i;
   int anchorPos, hAlign = TIP_LEFT, vAlign = TIP_BELOW, alignMode = TIP_SLOPPY;

   // Read and check the string 
   if (nArgs < 1)
   {
      *errMsg = "%s subroutine called with too few arguments";
      return false;
   }
   if (nArgs > 6)
   {
      *errMsg = "%s subroutine called with too many arguments";
      return false;
   }

   // Read the tip text or key 
   if (!readStringArg(argList[0], &tipText, stringStorage, errMsg))
      return false;

   // Read the anchor position (-1 for unanchored) 
   if (nArgs > 1)
   {
      if (!readIntArg(argList[1], &anchorPos, errMsg))
         return false;
   }
   else
   {
      anchorPos = -1;
   }
   if (anchorPos >= 0) anchored = true;

   // Any further args are directives for relative positioning 
   for (i = 2; i < nArgs; ++i)
   {
      if (!readStringArg(argList[i], &txtArg, stringStorage, errMsg))
      {
         return false;
      }
      switch (txtArg[0])
      {
      case 'c':
         if (strcmp(txtArg, "center"))
            goto bad_arg;
         hAlign = TIP_CENTER;
         break;
      case 'r':
         if (strcmp(txtArg, "right"))
            goto bad_arg;
         hAlign = TIP_RIGHT;
         break;
      case 'a':
         if (strcmp(txtArg, "above"))
            goto bad_arg;
         vAlign = TIP_ABOVE;
         break;
      case 's':
         if (strcmp(txtArg, "strict"))
            goto bad_arg;
         alignMode = TIP_STRICT;
         break;
      case 't':
         if (!strcmp(txtArg, "tipText"))
            mode = -1;
         else if (!strcmp(txtArg, "tipKey"))
            mode = TIP;
         else if (!strcmp(txtArg, "tagKey"))
            mode = TIP_FROM_TAG;
         else
            goto bad_arg;
         break;
      default:
         goto bad_arg;
      }
   }

   result->tag = INT_TAG;
   if (mode < 0) lookup = false;
   // Look up (maybe) a calltip and display it 
   result->val.n = ShowTipString(window, tipText, anchored, anchorPos, lookup, mode, hAlign, vAlign, alignMode);

   return true;

bad_arg:
   /* This is how the (more informative) global var. version would work,
       assuming there was a global buffer called msg.  */
   /* sprintf(msg, "unrecognized argument to %%s: \"%s\"", txtArg);
   *errMsg = msg; */
   *errMsg = "unrecognized argument to %s";
   return false;
}

/*
** A subroutine to kill the current calltip
*/
static int killCalltipMS(WindowInfo* window, DataValue* argList, int nArgs,
                         DataValue* result, char** errMsg)
{
   int calltipID = 0;

   if (nArgs > 1)
   {
      *errMsg = "%s subroutine called with too many arguments";
      return false;
   }
   if (nArgs > 0)
   {
      if (!readIntArg(argList[0], &calltipID, errMsg))
         return false;
   }

   KillCalltip(window, calltipID);

   result->tag = NO_TAG;
   return true;
}

/*
 * A subroutine to get the ID of the current calltip, or 0 if there is none.
 */
static int calltipIDMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = GetCalltipID(window, 0);
   return true;
}

/*
**  filename_dialog([title[, mode[, defaultPath[, filter[, defaultName]]]]])
**
**  Presents a FileSelectionDialog to the user prompting for a new file.
**
**  Options are:
**  title       - will be the title of the dialog, defaults to "Choose file".
**  mode        - if set to "exist" (default), the "New File Name" TextField
**                of the FSB will be unmanaged. If "new", the TextField will
**                be managed.
**  defaultPath - is the default path to use. Default (or "") will use the
**                active document's directory.
**  filter      - the file glob which determines which files to display.
**                Is set to "*" if filter is "" and by default.
**  defaultName - is the default filename that is filled in automatically.
**
** Returns "" if the user cancelled the dialog, otherwise returns the path to
** the file that was selected
**
** Note that defaultName doesn't work on all *tifs.  :-(
*/
static int filenameDialogMS(WindowInfo* window, DataValue* argList, int nArgs,
                            DataValue* result, char** errMsg)
{
   char stringStorage[5][TYPE_INT_STR_SIZE(int)];
   char filename[MAXPATHLEN + 1];
   char* title = "Choose Filename";
   char* mode = "exist";
   char* defaultPath = "";
   char* filter = "";
   char* defaultName = "";
   char* orgDefaultPath;
   char* orgFilter;
   int gfnResult;

   /* Ignore the focused window passed as the function argument and put
      the dialog up over the window which is executing the macro */
   window = MacroRunWindow();

   /* Dialogs require macro to be suspended and interleaved with other macros.
      This subroutine can't be run if macro execution can't be interrupted */
   if (NULL == window->macroCmdData)
   {
      M_FAILURE("%s can't be called from non-suspendable context");
   }

   //  Get the argument list.  
   if (nArgs > 0 && !readStringArg(argList[0], &title, stringStorage[0],
                                   errMsg))
   {
      return false;
   }

   if (nArgs > 1 && !readStringArg(argList[1], &mode, stringStorage[1],
                                   errMsg))
   {
      return false;
   }
   if (0 != strcmp(mode, "exist") && 0 != strcmp(mode, "new"))
   {
      M_FAILURE("Invalid value for mode in %s");
   }

   if (nArgs > 2 && !readStringArg(argList[2], &defaultPath, stringStorage[2],
                                   errMsg))
   {
      return false;
   }

   if (nArgs > 3 && !readStringArg(argList[3], &filter, stringStorage[3],
                                   errMsg))
   {
      return false;
   }

   if (nArgs > 4 && !readStringArg(argList[4], &defaultName, stringStorage[4],
                                   errMsg))
   {
      return false;
   }

   if (nArgs > 5)
   {
      M_FAILURE("%s called with too many arguments. Expects at most 5 arguments.");
   }

   //  Set default directory (saving original for later)  
   orgDefaultPath = GetFileDialogDefaultDirectory();
   if ('\0' != defaultPath[0])
   {
      SetFileDialogDefaultDirectory(defaultPath);
   }
   else
   {
      SetFileDialogDefaultDirectory(window->path);
   }

   //  Set filter (saving original for later)  
   orgFilter = GetFileDialogDefaultPattern();
   if ('\0' != filter[0])
   {
      SetFileDialogDefaultPattern(filter);
   }

   /*  Fork to one of the worker methods from util/getfiles.c.
       (This should obviously be refactored.)  */
   if (0 == strcmp(mode, "exist"))
   {
      gfnResult = GetExistingFilename(window->mainWindow, title, filename);
   }
   else
   {
      gfnResult = GetNewFilename(window->mainWindow, title, filename, defaultName);
   }   //  Invalid values are weeded out above.  

   //  Reset original values and free__ temps  
   SetFileDialogDefaultDirectory(orgDefaultPath);
   SetFileDialogDefaultPattern(orgFilter);
   free__(orgDefaultPath);
   free__(orgFilter);

   result->tag = STRING_TAG;
   if (GFN_OK == gfnResult)
   {
      //  Got a string, copy it to the result  
      if (!AllocNStringNCpy(&result->val.str, filename, MAXPATHLEN))
      {
         M_FAILURE("failed to allocate return value: %s");
      }
   }
   else
   {
      // User cancelled.  Return "" 
      result->val.str.rep = PERM_ALLOC_STR("");
      result->val.str.len = 0;
   }

   return true;
}

// T Balinski 
static int listDialogMS(WindowInfo* window, DataValue* argList, int nArgs,
                        DataValue* result, char** errMsg)
{
   macroCmdInfo* cmdData;
// TODO:    char stringStorage[TYPE_INT_STR_SIZE(int)];
// TODO:    char textStorage[TYPE_INT_STR_SIZE(int)];
// TODO:    char btnStorage[TYPE_INT_STR_SIZE(int)];
// TODO:    char* btnLabel;
// TODO:    char* message, *text;
// TODO:    Fl_Widget* dialog, btn;
// TODO:    int i, nBtns;
// TODO:    NeString s1, s2;
// TODO:    long nlines = 0;
// TODO:    char* p, *old_p, **text_lines, *tmp;
// TODO:    int tmp_len;
// TODO:    int n, is_last;
// TODO:    NeString* test_strings;
// TODO:    int tabDist;
// TODO:    Arg al[20];
// TODO:    int ac;
// TODO: 
// TODO: 
// TODO:    /* Ignore the focused window passed as the function argument and put
// TODO:       the dialog up over the window which is executing the macro */
// TODO:    window = MacroRunWindow();
// TODO:    cmdData = window->macroCmdData;
// TODO: 
// TODO:    /* Dialogs require macro to be suspended and interleaved with other macros.
// TODO:       This subroutine can't be run if macro execution can't be interrupted */
// TODO:    if (!cmdData)
// TODO:    {
// TODO:       *errMsg = "%s can't be called from non-suspendable context";
// TODO:       return false;
// TODO:    }
// TODO: 
// TODO:    /* Read and check the arguments.  The first being the dialog message,
// TODO:       and the rest being the button labels */
// TODO:    if (nArgs < 2)
// TODO:    {
// TODO:       *errMsg = "%s subroutine called with no message, string or arguments";
// TODO:       return false;
// TODO:    }
// TODO: 
// TODO:    if (!readStringArg(argList[0], &message, stringStorage, errMsg))
// TODO:       return false;
// TODO: 
// TODO:    if (!readStringArg(argList[1], &text, textStorage, errMsg))
// TODO:       return false;
// TODO: 
// TODO:    if (!text || text[0] == '\0')
// TODO:    {
// TODO:       *errMsg = "%s subroutine called with empty list data";
// TODO:       return false;
// TODO:    }
// TODO: 
// TODO:    // check that all button labels can be read 
// TODO:    for (i=2; i<nArgs; i++)
// TODO:       if (!readStringArg(argList[i], &btnLabel, btnStorage, errMsg))
// TODO:          return false;
// TODO: 
// TODO:    // pick up the first button 
// TODO:    if (nArgs == 2)
// TODO:    {
// TODO:       btnLabel = "OK";
// TODO:       nBtns = 1;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       nBtns = nArgs - 2;
// TODO:       argList += 2;
// TODO:       readStringArg(argList[0], &btnLabel, btnStorage, errMsg);
// TODO:    }
// TODO: 
// TODO:    // count the lines in the text - add one for unterminated last line 
// TODO:    nlines = 1;
// TODO:    for (p = text; *p; p++)
// TODO:       if (*p == '\n')
// TODO:          nlines++;
// TODO: 
// TODO:    // now set up arrays of pointers to lines 
// TODO:    //   test_strings to hold the display strings (tab expanded) 
// TODO:    //   text_lines to hold the original text lines (without the '\n's) 
// TODO:    test_strings = (NeString*) malloc__(sizeof(NeString) * nlines);
// TODO:    text_lines = (char**)malloc__(sizeof(char*) * (nlines + 1));
// TODO:    for (n = 0; n < nlines; n++)
// TODO:    {
// TODO:       test_strings[n] = (NeString)0;
// TODO:       text_lines[n] = (char*)0;
// TODO:    }
// TODO:    text_lines[n] = (char*)0;         // make sure this is a null-terminated table 
// TODO: 
// TODO:    // pick up the tabDist value 
// TODO:    tabDist = window->buffer->tabDist;
// TODO: 
// TODO:    // load the table 
// TODO:    n = 0;
// TODO:    is_last = 0;
// TODO:    p = old_p = text;
// TODO:    tmp_len = 0;      // current allocated size of temporary buffer tmp 
// TODO:    tmp = malloc__(1);  // temporary buffer into which to expand tabs 
// TODO:    do
// TODO:    {
// TODO:       is_last = (*p == '\0');
// TODO:       if (*p == '\n' || is_last)
// TODO:       {
// TODO:          *p = '\0';
// TODO:          if (strlen(old_p) > 0)      // only include non-empty lines 
// TODO:          {
// TODO:             char* s, *t;
// TODO:             int l;
// TODO: 
// TODO:             // save the actual text line in text_lines[n] 
// TODO:             text_lines[n] = (char*)malloc__(strlen(old_p) + 1);
// TODO:             strcpy(text_lines[n], old_p);
// TODO: 
// TODO:             // work out the tabs expanded length 
// TODO:             for (s = old_p, l = 0; *s; s++)
// TODO:                l += (*s == '\t') ? tabDist - (l % tabDist) : 1;
// TODO: 
// TODO:             // verify tmp is big enough then tab-expand old_p into tmp 
// TODO:             if (l > tmp_len)
// TODO:                tmp = realloc(tmp, (tmp_len = l) + 1);
// TODO:             for (s = old_p, t = tmp, l = 0; *s; s++)
// TODO:             {
// TODO:                if (*s == '\t')
// TODO:                {
// TODO:                   for (i = tabDist - (l % tabDist); i--; l++)
// TODO:                      *t++ = ' ';
// TODO:                }
// TODO:                else
// TODO:                {
// TODO:                   *t++ = *s;
// TODO:                   l++;
// TODO:                }
// TODO:             }
// TODO:             *t = '\0';
// TODO:             // that's it: tmp is the tab-expanded version of old_p 
// TODO:             test_strings[n] = MKSTRING(tmp);
// TODO:             n++;
// TODO:          }
// TODO:          old_p = p + 1;
// TODO:          if (!is_last)
// TODO:             *p = '\n';              // put back our newline 
// TODO:       }
// TODO:       p++;
// TODO:    }
// TODO:    while (!is_last);
// TODO: 
// TODO:    free__(tmp);                // don't need this anymore 
// TODO:    nlines = n;
// TODO:    if (nlines == 0)
// TODO:    {
// TODO:       test_strings[0] = MKSTRING("");
// TODO:       nlines = 1;
// TODO:    }
// TODO: 
// TODO:    // Create the selection box dialog widget and its dialog shell parent 
// TODO:    ac = 0;
// TODO:    XtSetArg(al[ac], XmNtitle, " ");
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNlistLabelString, s1=MKSTRING(message));
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNlistItems, test_strings);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNlistItemCount, nlines);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNlistVisibleItemCount, (nlines > 10) ? 10 : nlines);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNokLabelString, s2=NeNewString(btnLabel));
// TODO:    ac++;
// TODO:    dialog = CreateSelectionDialog(window->mainWindow, "macroListDialog", al, ac);
// TODO:    if (2 == nArgs)
// TODO:    {
// TODO:       //  Only set margin width for the default OK button  
// TODO:       XtVaSetValues(XmSelectionBoxGetChild(dialog, XmDIALOG_OK_BUTTON),
// TODO:                     XmNmarginWidth, BUTTON_WIDTH_MARGIN,
// TODO:                     NULL);
// TODO:    }
// TODO: 
// TODO:    AddMotifCloseCallback(XtParent(dialog), listDialogCloseCB, window);
// TODO:    XtAddCallback(dialog, XmNokCallback, listDialogBtnCB, window);
// TODO:    XtVaSetValues(XmSelectionBoxGetChild(dialog, XmDIALOG_OK_BUTTON),
// TODO:                  XmNuserData, (void*)1, NULL);
// TODO:    NeStringFree(s1);
// TODO:    NeStringFree(s2);
// TODO:    cmdData->dialog = dialog;
// TODO: 
// TODO:    // forget lines stored in list 
// TODO:    while (n--)
// TODO:       NeStringFree(test_strings[n]);
// TODO:    free__((char*)test_strings);
// TODO: 
// TODO:    // modify the list 
// TODO:    XtVaSetValues(XmSelectionBoxGetChild(dialog, XmDIALOG_LIST),
// TODO:                  XmNselectionPolicy, XmSINGLE_SELECT,
// TODO:                  XmNuserData, (void*)text_lines, NULL);
// TODO: 
// TODO:    // Unmanage unneeded widgets 
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_APPLY_BUTTON));
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_TEXT));
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_SELECTION_LABEL));
// TODO: 
// TODO:    /* Make callback for the unmanaged cancel button (which can
// TODO:       still get executed via the esc key) activate close box action */
// TODO:    XtAddCallback(XmSelectionBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON),
// TODO:                  XmNactivateCallback, listDialogCloseCB, window);
// TODO: 
// TODO:    /* Add user specified buttons (1st is already done).  Selection box
// TODO:       requires a place-holder widget to be added before buttons can be
// TODO:       added, that's what the separator below is for */
// TODO:    XtVaCreateWidget("x", xmSeparatorWidgetClass, dialog, NULL);
// TODO:    for (i=1; i<nBtns; i++)
// TODO:    {
// TODO:       readStringArg(argList[i], &btnLabel, btnStorage, errMsg);
// TODO:       btn = XtVaCreateManagedWidget("mdBtn", xmPushButtonWidgetClass, dialog,
// TODO:                                     XmNlabelString, s1=NeNewString(btnLabel),
// TODO:                                     XmNuserData, (void*)(i+1), NULL);
// TODO:       XtAddCallback(btn, XmNactivateCallback, listDialogBtnCB, window);
// TODO:       NeStringFree(s1);
// TODO:    }
// TODO: 
// TODO: #ifdef LESSTIF_VERSION
// TODO:    /* Workaround for Lesstif (e.g. v2.1 r0.93.18) that doesn't handle
// TODO:       the escape key for closing the dialog. */
// TODO:    XtAddEventHandler(dialog, KeyPressMask, false, listDialogEscCB,
// TODO:                      (void*)window);
// TODO:    XtGrabKey(dialog, XKeysymToKeycode(XtDisplay(dialog), XK_Escape), 0,
// TODO:              true, GrabModeAsync, GrabModeAsync);
// TODO: #endif // LESSTIF_VERSION 
// TODO: 
// TODO:    // Put up the dialog 
// TODO:    ManageDialogCenteredOnPointer(dialog);
// TODO: 
// TODO:    // Stop macro execution until the dialog is complete 
// TODO:    PreemptMacro();
// TODO: 
// TODO:    // Return placeholder result.  Value will be changed by button callback 
// TODO:    result->tag = INT_TAG;
// TODO:    result->val.n = 0;
   return true;
}

static void listDialogBtnCB(Fl_Widget* w, void* clientData, void* callData)
{
   WindowInfo* window = (WindowInfo*)clientData;
// TODO:    macroCmdInfo* cmdData = window->macroCmdData;
// TODO:    void* userData;
// TODO:    DataValue retVal;
// TODO:    char* text;
// TODO:    char** text_lines;
// TODO:    int btnNum;
// TODO:    int n_sel, *seltable, sel_index = 0;
// TODO:    Fl_Widget* theList;
// TODO:    size_t length;
// TODO: 
// TODO:    // shouldn't happen, but would crash if it did 
// TODO:    if (cmdData == NULL)
// TODO:       return;
// TODO: 
// TODO:    theList = XmSelectionBoxGetChild(cmdData->dialog, XmDIALOG_LIST);
// TODO:    // Return the string selected in the selection list area 
// TODO:    XtVaGetValues(theList, XmNuserData, &text_lines, NULL);
// TODO:    if (!XmListGetSelectedPos(theList, &seltable, &n_sel))
// TODO:    {
// TODO:       n_sel = 0;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       sel_index = seltable[0] - 1;
// TODO:       free__((void*)seltable);
// TODO:    }
// TODO: 
// TODO:    if (!n_sel)
// TODO:    {
// TODO:       text = PERM_ALLOC_STR("");
// TODO:       length = 0;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       length = strlen((char*)text_lines[sel_index]);
// TODO:       text = AllocString(length + 1);
// TODO:       strcpy(text, text_lines[sel_index]);
// TODO:    }
// TODO: 
// TODO:    // don't need text_lines anymore: free__ it 
// TODO:    for (sel_index = 0; text_lines[sel_index]; sel_index++)
// TODO:       free__((void*)text_lines[sel_index]);
// TODO:    free__((void*)text_lines);
// TODO: 
// TODO:    retVal.tag = STRING_TAG;
// TODO:    retVal.val.str.rep = text;
// TODO:    retVal.val.str.len = length;
// TODO:    ModifyReturnedValue(cmdData->context, retVal);
// TODO: 
// TODO:    /* Find the index of the button which was pressed (stored in the userData
// TODO:       field of the button widget).  The 1st button, being a gadget, is not
// TODO:       returned in w. */
// TODO:    if (XtClass(w) == xmPushButtonWidgetClass)
// TODO:    {
// TODO:       XtVaGetValues(w, XmNuserData, &userData, NULL);
// TODO:       btnNum = (int)userData;
// TODO:    }
// TODO:    else
// TODO:       btnNum = 1;
// TODO: 
// TODO:    // Return the button number in the global variable $list_dialog_button 
// TODO:    ReturnGlobals[LIST_DIALOG_BUTTON]->value.tag = INT_TAG;
// TODO:    ReturnGlobals[LIST_DIALOG_BUTTON]->value.val.n = btnNum;
// TODO: 
// TODO:    // Pop down the dialog 
// TODO:    XtDestroyWidget(XtParent(cmdData->dialog));
// TODO:    cmdData->dialog = NULL;
// TODO: 
// TODO:    // Continue preempted macro execution 
// TODO:    ResumeMacroExecution(window);
// TODO: }
// TODO: 
// TODO: static void listDialogCloseCB(Fl_Widget* w, void* clientData,
// TODO:                               void* callData)
// TODO: {
// TODO:    WindowInfo* window = (WindowInfo*)clientData;
// TODO:    macroCmdInfo* cmdData = window->macroCmdData;
// TODO:    DataValue retVal;
// TODO:    char** text_lines;
// TODO:    int sel_index;
// TODO:    Fl_Widget* theList;
// TODO: 
// TODO:    // shouldn't happen, but would crash if it did 
// TODO:    if (cmdData == NULL)
// TODO:       return;
// TODO: 
// TODO:    // don't need text_lines anymore: retrieve it then free__ it 
// TODO:    theList = XmSelectionBoxGetChild(cmdData->dialog, XmDIALOG_LIST);
// TODO:    XtVaGetValues(theList, XmNuserData, &text_lines, NULL);
// TODO:    for (sel_index = 0; text_lines[sel_index]; sel_index++)
// TODO:       free__((void*)text_lines[sel_index]);
// TODO:    free__((void*)text_lines);
// TODO: 
// TODO:    // Return an empty string 
// TODO:    retVal.tag = STRING_TAG;
// TODO:    retVal.val.str.rep = PERM_ALLOC_STR("");
// TODO:    retVal.val.str.len = 0;
// TODO:    ModifyReturnedValue(cmdData->context, retVal);
// TODO: 
// TODO:    // Return button number 0 in the global variable $list_dialog_button 
// TODO:    ReturnGlobals[LIST_DIALOG_BUTTON]->value.tag = INT_TAG;
// TODO:    ReturnGlobals[LIST_DIALOG_BUTTON]->value.val.n = 0;
// TODO: 
// TODO:    // Pop down the dialog 
// TODO:    XtDestroyWidget(XtParent(cmdData->dialog));
// TODO:    cmdData->dialog = NULL;

   // Continue preempted macro execution 
   ResumeMacroExecution(window);
}
// T Balinski End 

static int stringCompareMS(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   char stringStorage[3][TYPE_INT_STR_SIZE(int)];
   char* leftStr, *rightStr, *argStr;
   int considerCase = true;
   int i;
   int compareResult;

   if (nArgs < 2)
   {
      return(wrongNArgsErr(errMsg));
   }
   if (!readStringArg(argList[0], &leftStr, stringStorage[0], errMsg))
      return false;
   if (!readStringArg(argList[1], &rightStr, stringStorage[1], errMsg))
      return false;
   for (i = 2; i < nArgs; ++i)
   {
      if (!readStringArg(argList[i], &argStr, stringStorage[2], errMsg))
         return false;
      else if (!strcmp(argStr, "case"))
         considerCase = true;
      else if (!strcmp(argStr, "nocase"))
         considerCase = false;
      else
      {
         *errMsg = "Unrecognized argument to %s";
         return false;
      }
   }
   if (considerCase)
   {
      compareResult = strcmp(leftStr, rightStr);
      compareResult = (compareResult > 0) ? 1 : ((compareResult < 0) ? -1 : 0);
   }
   else
   {
      compareResult = strCaseCmp(leftStr, rightStr);
   }
   result->tag = INT_TAG;
   result->val.n = compareResult;
   return true;
}

/*
** This function is intended to split strings into an array of substrings
** Importatnt note: It should always return at least one entry with key 0
** split("", ",") result[0] = ""
** split("1,2", ",") result[0] = "1" result[1] = "2"
** split("1,2,", ",") result[0] = "1" result[1] = "2" result[2] = ""
**
** This behavior is specifically important when used to break up
** array sub-scripts
*/

static int splitMS(WindowInfo* window, DataValue* argList, int nArgs,
                   DataValue* result, char** errMsg)
{
   char stringStorage[3][TYPE_INT_STR_SIZE(int)];
   char* sourceStr, *splitStr, *typeSplitStr;
   int searchType, beginPos, foundStart, foundEnd, strLength, lastEnd;
   int found, elementEnd, indexNum;
   char indexStr[TYPE_INT_STR_SIZE(int)], *allocIndexStr;
   DataValue element;
   int elementLen;

   if (nArgs < 2)
   {
      return(wrongNArgsErr(errMsg));
   }
   if (!readStringArg(argList[0], &sourceStr, stringStorage[0], errMsg))
   {
      *errMsg = "first argument must be a string: %s";
      return(false);
   }
   if (!readStringArg(argList[1], &splitStr, stringStorage[1], errMsg))
   {
      splitStr = NULL;
   }
   else
   {
      if (splitStr[0] == 0)
      {
         splitStr = NULL;
      }
   }
   if (splitStr == NULL)
   {
      *errMsg = "second argument must be a non-empty string: %s";
      return(false);
   }
   if (nArgs > 2 && readStringArg(argList[2], &typeSplitStr, stringStorage[2], errMsg))
   {
      if (!StringToSearchType(typeSplitStr, &searchType))
      {
         *errMsg = "unrecognized argument to %s";
         return(false);
      }
   }
   else
   {
      searchType = SEARCH_LITERAL;
   }

   result->tag = ARRAY_TAG;
   result->val.arrayPtr = ArrayNew();

   beginPos = 0;
   lastEnd = 0;
   indexNum = 0;
   strLength = strlen(sourceStr);
   found = 1;
   while (found && beginPos < strLength)
   {
      sprintf(indexStr, "%d", indexNum);
      allocIndexStr = AllocString(strlen(indexStr) + 1);
      if (!allocIndexStr)
      {
         *errMsg = "array element failed to allocate key: %s";
         return(false);
      }
      strcpy(allocIndexStr, indexStr);
      found = SearchString(sourceStr, splitStr, SEARCH_FORWARD, searchType,
                           false, beginPos, &foundStart, &foundEnd,
                           NULL, NULL, GetWindowDelimiters(window));
      elementEnd = found ? foundStart : strLength;
      elementLen = elementEnd - lastEnd;
      element.tag = STRING_TAG;
      if (!AllocNStringNCpy(&element.val.str, &sourceStr[lastEnd], elementLen))
      {
         *errMsg = "failed to allocate element value: %s";
         return(false);
      }

      if (!ArrayInsert(result, allocIndexStr, &element))
      {
         M_ARRAY_INSERT_FAILURE();
      }

      if (found)
      {
         if (foundStart == foundEnd)
         {
            beginPos = foundEnd + 1; // Avoid endless loop for 0-width match 
         }
         else
         {
            beginPos = foundEnd;
         }
      }
      else
      {
         beginPos = strLength; // Break the loop 
      }
      lastEnd = foundEnd;
      ++indexNum;
   }
   if (found)
   {
      sprintf(indexStr, "%d", indexNum);
      allocIndexStr = AllocString(strlen(indexStr) + 1);
      if (!allocIndexStr)
      {
         *errMsg = "array element failed to allocate key: %s";
         return(false);
      }
      strcpy(allocIndexStr, indexStr);
      element.tag = STRING_TAG;
      if (lastEnd == strLength)
      {
         // The pattern mathed the end of the string. Add an empty chunk. 
         element.val.str.rep = PERM_ALLOC_STR("");
         element.val.str.len = 0;

         if (!ArrayInsert(result, allocIndexStr, &element))
         {
            M_ARRAY_INSERT_FAILURE();
         }
      }
      else
      {
         /* We skipped the last character to prevent an endless loop.
            Add it to the list. */
         elementLen = strLength - lastEnd;
         if (!AllocNStringNCpy(&element.val.str, &sourceStr[lastEnd], elementLen))
         {
            *errMsg = "failed to allocate element value: %s";
            return(false);
         }

         if (!ArrayInsert(result, allocIndexStr, &element))
         {
            M_ARRAY_INSERT_FAILURE();
         }

         /* If the pattern can match zero-length strings, we may have to
            add a final empty chunk.
            For instance:  split("abc\n", "$", "regex")
              -> matches before \n and at end of string
              -> expected output: "abc", "\n", ""
            The '\n' gets added in the lines above, but we still have to
            verify whether the pattern also matches the end of the string,
            and add an empty chunk in case it does. */
         found = SearchString(sourceStr, splitStr, SEARCH_FORWARD,
                              searchType, false, strLength, &foundStart, &foundEnd,
                              NULL, NULL, GetWindowDelimiters(window));
         if (found)
         {
            ++indexNum;
            sprintf(indexStr, "%d", indexNum);
            allocIndexStr = AllocString(strlen(indexStr) + 1);
            if (!allocIndexStr)
            {
               *errMsg = "array element failed to allocate key: %s";
               return(false);
            }
            strcpy(allocIndexStr, indexStr);
            element.tag = STRING_TAG;
            element.val.str.rep = PERM_ALLOC_STR("");
            element.val.str.len = 0;

            if (!ArrayInsert(result, allocIndexStr, &element))
            {
               M_ARRAY_INSERT_FAILURE();
            }
         }
      }
   }
   return(true);
}

/*
** Set the backlighting string resource for the current window. If no parameter
** is passed or the value "default" is passed, it attempts to set the preference
** value of the resource. If the empty string is passed, the backlighting string
** will be cleared, turning off backlighting.
*/
/* DISABLED for 5.4
static int setBacklightStringMS(WindowInfo *window, DataValue *argList,
      int nArgs, DataValue *result, char **errMsg)
{
    char *backlightString;

    if (nArgs == 0) {
      backlightString = GetPrefBacklightCharTypes();
    }
    else if (nArgs == 1) {
      if (argList[0].tag != STRING_TAG) {
          *errMsg = "%s not called with a string parameter";
          return false;
      }
      backlightString = argList[0].val.str.rep;
    }
    else
      return wrongNArgsErr(errMsg);

    if (strcmp(backlightString, "default") == 0)
      backlightString = GetPrefBacklightCharTypes();
    if (backlightString && *backlightString == '\0')  / * empty string param * /
      backlightString = NULL;                 / * turns of backlighting * /

    SetBacklightChars(window, backlightString);
    return true;
} */

static int cursorMV(WindowInfo* window, DataValue* argList, int nArgs,
                    DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = TextGetCursorPos(window->lastFocus);
   return true;
}

static int lineMV(WindowInfo* window, DataValue* argList, int nArgs,
                  DataValue* result, char** errMsg)
{
   int line, cursorPos, colNum;

   result->tag = INT_TAG;
   cursorPos = TextGetCursorPos(window->lastFocus);
   if (!TextPosToLineAndCol(window->lastFocus, cursorPos, &line, &colNum))
      line = BufCountLines(window->buffer, 0, cursorPos) + 1;
   result->val.n = line;
   return true;
}

static int columnMV(WindowInfo* window, DataValue* argList, int nArgs,
                    DataValue* result, char** errMsg)
{
   Ne_Text_Buffer* buf = window->buffer;
   int cursorPos;

   result->tag = INT_TAG;
   cursorPos = TextGetCursorPos(window->lastFocus);
   result->val.n = BufCountDispChars(buf, BufStartOfLine(buf, cursorPos),
                                     cursorPos);
   return true;
}

static int fileNameMV(WindowInfo* window, DataValue* argList, int nArgs,
                      DataValue* result, char** errMsg)
{
   result->tag = STRING_TAG;
   AllocNStringCpy(&result->val.str, window->filename);
   return true;
}

static int filePathMV(WindowInfo* window, DataValue* argList, int nArgs,
                      DataValue* result, char** errMsg)
{
   result->tag = STRING_TAG;
   AllocNStringCpy(&result->val.str, window->path);
   return true;
}

static int lengthMV(WindowInfo* window, DataValue* argList, int nArgs,
                    DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->buffer->length;
   return true;
}

static int selectionStartMV(WindowInfo* window, DataValue* argList, int nArgs,
                            DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->buffer->primary.selected ?
      window->buffer->primary.start : -1;
   return true;
}

static int selectionEndMV(WindowInfo* window, DataValue* argList, int nArgs,
                          DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->buffer->primary.selected ?
      window->buffer->primary.end : -1;
   return true;
}

static int selectionLeftMV(WindowInfo* window, DataValue* argList, int nArgs,
                           DataValue* result, char** errMsg)
{
   selection* sel = &window->buffer->primary;

   result->tag = INT_TAG;
   result->val.n = sel->selected && sel->rectangular ? sel->rectStart : -1;
   return true;
}

static int selectionRightMV(WindowInfo* window, DataValue* argList, int nArgs,
                            DataValue* result, char** errMsg)
{
   selection* sel = &window->buffer->primary;

   result->tag = INT_TAG;
   result->val.n = sel->selected && sel->rectangular ? sel->rectEnd : -1;
   return true;
}

static int wrapMarginMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   int margin = window->textArea->columns();
   int nCols = window->textArea->wrapMargin;

   result->tag = INT_TAG;
   result->val.n = margin == 0 ? nCols : margin;
   return true;
}

static int statisticsLineMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->showStats ? 1 : 0;
   return true;
}

static int incSearchLineMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->showISearchLine ? 1 : 0;
   return true;
}

static int showLineNumbersMV(WindowInfo* window, DataValue* argList, int nArgs,
                             DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->showLineNumbers ? 1 : 0;
   return true;
}

static int autoIndentMV(WindowInfo* window, DataValue* argList, int nArgs,
                        DataValue* result, char** errMsg)
{
   char* res = NULL;

   switch (window->indentStyle)
   {
   case NO_AUTO_INDENT:
      res = PERM_ALLOC_STR("off");
      break;
   case AUTO_INDENT:
      res = PERM_ALLOC_STR("on");
      break;
   case SMART_INDENT:
      res = PERM_ALLOC_STR("smart");
      break;
   default:
      *errMsg = "Invalid indent style value encountered in %s";
      return false;
      break;
   }
   result->tag = STRING_TAG;
   result->val.str.rep = res;
   result->val.str.len = strlen(res);
   return true;
}

static int wrapTextMV(WindowInfo* window, DataValue* argList, int nArgs,
                      DataValue* result, char** errMsg)
{
   char* res = NULL;

   switch (window->wrapMode)
   {
   case NO_WRAP:
      res = PERM_ALLOC_STR("none");
      break;
   case NEWLINE_WRAP:
      res = PERM_ALLOC_STR("auto");
      break;
   case CONTINUOUS_WRAP:
      res = PERM_ALLOC_STR("continuous");
      break;
   default:
      *errMsg = "Invalid wrap style value encountered in %s";
      return false;
      break;
   }
   result->tag = STRING_TAG;
   result->val.str.rep = res;
   result->val.str.len = strlen(res);
   return true;
}

static int highlightSyntaxMV(WindowInfo* window, DataValue* argList, int nArgs,
                             DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->highlightSyntax ? 1 : 0;
   return true;
}

static int makeBackupCopyMV(WindowInfo* window, DataValue* argList, int nArgs,
                            DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->saveOldVersion ? 1 : 0;
   return true;
}

static int incBackupMV(WindowInfo* window, DataValue* argList, int nArgs,
                       DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->autoSave ? 1 : 0;
   return true;
}

static int showMatchingMV(WindowInfo* window, DataValue* argList, int nArgs,
                          DataValue* result, char** errMsg)
{
   char* res = NULL;

   switch (window->showMatchingStyle)
   {
   case NO_FLASH:
      res = PERM_ALLOC_STR(NO_FLASH_STRING);
      break;
   case FLASH_DELIMIT:
      res = PERM_ALLOC_STR(FLASH_DELIMIT_STRING);
      break;
   case FLASH_RANGE:
      res = PERM_ALLOC_STR(FLASH_RANGE_STRING);
      break;
   default:
      *errMsg = "Invalid match flashing style value encountered in %s";
      return false;
      break;
   }
   result->tag = STRING_TAG;
   result->val.str.rep = res;
   result->val.str.len = strlen(res);
   return true;
}

static int matchSyntaxBasedMV(WindowInfo* window, DataValue* argList, int nArgs,
                              DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->matchSyntaxBased ? 1 : 0;
   return true;
}



static int overTypeModeMV(WindowInfo* window, DataValue* argList, int nArgs,
                          DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->overstrike ? 1 : 0;
   return true;
}

static int readOnlyMV(WindowInfo* window, DataValue* argList, int nArgs,
                      DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = (IS_ANY_LOCKED(window->lockReasons)) ? 1 : 0;
   return true;
}

static int lockedMV(WindowInfo* window, DataValue* argList, int nArgs,
                    DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = (IS_USER_LOCKED(window->lockReasons)) ? 1 : 0;
   return true;
}

static int fileFormatMV(WindowInfo* window, DataValue* argList, int nArgs,
                        DataValue* result, char** errMsg)
{
   char* res = NULL;

   switch (window->fileFormat)
   {
   case UNIX_FILE_FORMAT:
      res = PERM_ALLOC_STR("unix");
      break;
   case DOS_FILE_FORMAT:
      res = PERM_ALLOC_STR("dos");
      break;
   case MAC_FILE_FORMAT:
      res = PERM_ALLOC_STR("macintosh");
      break;
   default:
      *errMsg = "Invalid linefeed style value encountered in %s";
      return false;
   }
   result->tag = STRING_TAG;
   result->val.str.rep = res;
   result->val.str.len = strlen(res);
   return true;
}

static int fontNameMV(WindowInfo* window, DataValue* argList, int nArgs,
                      DataValue* result, char** errMsg)
{
   result->tag = STRING_TAG;
   AllocNStringCpy(&result->val.str, window->fontName);
   return true;
}

static int fontNameItalicMV(WindowInfo* window, DataValue* argList, int nArgs,
                            DataValue* result, char** errMsg)
{
   result->tag = STRING_TAG;
   AllocNStringCpy(&result->val.str, window->italicFontName);
   return true;
}

static int fontNameBoldMV(WindowInfo* window, DataValue* argList, int nArgs,
                          DataValue* result, char** errMsg)
{
   result->tag = STRING_TAG;
   AllocNStringCpy(&result->val.str, window->boldFontName);
   return true;
}

static int fontNameBoldItalicMV(WindowInfo* window, DataValue* argList, int nArgs,
                                DataValue* result, char** errMsg)
{
   result->tag = STRING_TAG;
   AllocNStringCpy(&result->val.str, window->boldItalicFontName);
   return true;
}

static int subscriptSepMV(WindowInfo* window, DataValue* argList, int nArgs,
                          DataValue* result, char** errMsg)
{
   result->tag = STRING_TAG;
   result->val.str.rep = PERM_ALLOC_STR(ARRAY_DIM_SEP);
   result->val.str.len = strlen(result->val.str.rep);
   return true;
}

static int minFontWidthMV(WindowInfo* window, DataValue* argList, int nArgs,
                          DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = TextGetMinFontWidth(window->textArea, window->highlightSyntax);
   return true;
}

static int maxFontWidthMV(WindowInfo* window, DataValue* argList, int nArgs,
                          DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = TextGetMaxFontWidth(window->textArea, window->highlightSyntax);
   return true;
}

static int topLineMV(WindowInfo* window, DataValue* argList, int nArgs,
                     DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = TextFirstVisibleLine(window->lastFocus);
   return true;
}

static int numDisplayLinesMV(WindowInfo* window, DataValue* argList, int nArgs,
                             DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = TextNumVisibleLines(window->lastFocus);
   return true;
}

static int displayWidthMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = TextVisibleWidth(window->lastFocus);
   return true;
}

static int activePaneMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = WidgetToPaneIndex(window, window->lastFocus) + 1;
   return true;
}

static int nPanesMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->nPanes + 1;
   return true;
}

static int emptyArrayMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   result->tag = ARRAY_TAG;
   result->val.arrayPtr = NULL;
   return true;
}

static int serverNameMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   result->tag = STRING_TAG;
   AllocNStringCpy(&result->val.str, GetPrefServerName());
   return true;
}

static int tabDistMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->buffer->tabDist;
   return true;
}

static int emTabDistMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   int dist = window->textArea->emulateTabs;
   result->tag = INT_TAG;
   result->val.n = dist;
   return true;
}

static int useTabsMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->buffer->useTabs;
   return true;
}

static int modifiedMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   result->tag = INT_TAG;
   result->val.n = window->fileChanged;
   return true;
}

static int languageModeMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
   char* lmName = LanguageModeName(window->languageMode);

   if (lmName == NULL)
      lmName = "Plain";
   result->tag = STRING_TAG;
   AllocNStringCpy(&result->val.str, lmName);
   return true;
}

/* DISABLED for 5.4
static int backlightStringMV(WindowInfo *window, DataValue *argList,
      int nArgs, DataValue *result, char **errMsg)
{
    char *backlightString = window->backlightCharTypes;

    result->tag = STRING_TAG;
    if (!backlightString || !window->backlightChars)
      backlightString = "";
    AllocNStringCpy(&result->val.str, backlightString);
    return true;
} */

// -------------------------------------------------------------------------- 

/*
** Range set macro variables and functions
*/
static int rangesetListMV(WindowInfo* window, DataValue* argList, int nArgs, DataValue* result, char** errMsg)
{
// TODO:    RangesetTable* rangesetTable = window->buffer->rangesetTable;
// TODO:    unsigned char* rangesetList;
// TODO:    char* allocIndexStr;
// TODO:    char indexStr[TYPE_INT_STR_SIZE(int)] ;
// TODO:    int nRangesets, i;
// TODO:    DataValue element;
// TODO: 
// TODO:    result->tag = ARRAY_TAG;
// TODO:    result->val.arrayPtr = ArrayNew();
// TODO: 
// TODO:    if (rangesetTable == NULL)
// TODO:    {
// TODO:       return true;
// TODO:    }
// TODO: 
// TODO:    rangesetList = RangesetGetList(rangesetTable);
// TODO:    nRangesets = strlen((char*)rangesetList);
// TODO:    for (i = 0; i < nRangesets; i++)
// TODO:    {
// TODO:       element.tag = INT_TAG;
// TODO:       element.val.n = rangesetList[i];
// TODO: 
// TODO:       sprintf(indexStr, "%d", nRangesets - i - 1);
// TODO:       allocIndexStr = AllocString(strlen(indexStr) + 1);
// TODO:       if (allocIndexStr == NULL)
// TODO:          M_FAILURE("Failed to allocate array key in %s");
// TODO:       strcpy(allocIndexStr, indexStr);
// TODO: 
// TODO:       if (!ArrayInsert(result, allocIndexStr, &element))
// TODO:          M_FAILURE("Failed to insert array element in %s");
// TODO:    }

   return true;
}

/*
**  Returns the version number of the current macro language implementation.
**  For releases, this is the same number as NEdit's major.minor version
**  number to keep things simple. For developer versions this could really
**  be anything.
**
**  Note that the current way to build $VERSION builds the same value for
**  different point revisions. This is done because the macro interface
**  does not change for the same version.
*/
static int versionMV(WindowInfo* window, DataValue* argList, int nArgs,
                     DataValue* result, char** errMsg)
{
   static unsigned version = NEDIT_VERSION * 1000 + NEDIT_REVISION;

   result->tag = INT_TAG;
   result->val.n = version;
   return true;
}

/*
** Built-in macro subroutine to create a new rangeset or rangesets.
** If called with one argument: $1 is the number of rangesets required and
** return value is an array indexed 0 to n, with the rangeset labels as values;
** (or an empty array if the requested number of rangesets are not available).
** If called with no arguments, returns a single rangeset label (not an array),
** or an empty string if there are no rangesets available.
*/
static int rangesetCreateMS(WindowInfo* window, DataValue* argList, int nArgs,
                            DataValue* result, char** errMsg)
{
   int label;
   int i, nRangesetsRequired;
   DataValue element;
   char indexStr[TYPE_INT_STR_SIZE(int)], *allocIndexStr;

// TODO:    RangesetTable* rangesetTable = window->buffer->rangesetTable;
// TODO: 
// TODO:    if (nArgs > 1)
// TODO:       return wrongNArgsErr(errMsg);
// TODO: 
// TODO:    if (rangesetTable == NULL)
// TODO:    {
// TODO:       window->buffer->rangesetTable = rangesetTable =
// TODO:                                          RangesetTableAlloc(window->buffer);
// TODO:    }
// TODO: 
// TODO:    if (nArgs == 0)
// TODO:    {
// TODO:       label = RangesetCreate(rangesetTable);
// TODO: 
// TODO:       result->tag = INT_TAG;
// TODO:       result->val.n = label;
// TODO:       return true;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if (!readIntArg(argList[0], &nRangesetsRequired, errMsg))
// TODO:          return false;
// TODO: 
// TODO:       result->tag = ARRAY_TAG;
// TODO:       result->val.arrayPtr = ArrayNew();
// TODO: 
// TODO:       if (nRangesetsRequired > nRangesetsAvailable(rangesetTable))
// TODO:          return true;
// TODO: 
// TODO:       for (i = 0; i < nRangesetsRequired; i++)
// TODO:       {
// TODO:          element.tag = INT_TAG;
// TODO:          element.val.n = RangesetCreate(rangesetTable);
// TODO: 
// TODO:          sprintf(indexStr, "%d", i);
// TODO:          allocIndexStr = AllocString(strlen(indexStr) + 1);
// TODO:          if (!allocIndexStr)
// TODO:          {
// TODO:             *errMsg = "Array element failed to allocate key: %s";
// TODO:             return(false);
// TODO:          }
// TODO:          strcpy(allocIndexStr, indexStr);
// TODO:          ArrayInsert(result, allocIndexStr, &element);
// TODO:       }
// TODO: 
// TODO:       return true;
// TODO:    }
   return true;
}

/*
** Built-in macro subroutine for forgetting a range set.
*/
static int rangesetDestroyMS(WindowInfo* window, DataValue* argList, int nArgs,
                             DataValue* result, char** errMsg)
{
// TODO:    RangesetTable* rangesetTable = window->buffer->rangesetTable;
// TODO:    DataValue* array;
// TODO:    DataValue element;
// TODO:    char keyString[TYPE_INT_STR_SIZE(int)];
// TODO:    int deleteLabels[N_RANGESETS];
// TODO:    int i, arraySize;
// TODO:    int label = 0;
// TODO: 
// TODO:    if (nArgs != 1)
// TODO:    {
// TODO:       return wrongNArgsErr(errMsg);
// TODO:    }
// TODO: 
// TODO:    if (argList[0].tag == ARRAY_TAG)
// TODO:    {
// TODO:       array = &argList[0];
// TODO:       arraySize = ArraySize(array);
// TODO: 
// TODO:       if (arraySize > N_RANGESETS)
// TODO:       {
// TODO:          M_FAILURE("Too many elements in array in %s");
// TODO:       }
// TODO: 
// TODO:       for (i = 0; i < arraySize; i++)
// TODO:       {
// TODO:          sprintf(keyString, "%d", i);
// TODO: 
// TODO:          if (!ArrayGet(array, keyString, &element))
// TODO:          {
// TODO:             M_FAILURE("Invalid key in array in %s");
// TODO:          }
// TODO: 
// TODO:          if (!readIntArg(element, &label, errMsg)
// TODO:                || !RangesetLabelOK(label))
// TODO:          {
// TODO:             M_FAILURE("Invalid rangeset label in array in %s");
// TODO:          }
// TODO: 
// TODO:          deleteLabels[i] = label;
// TODO:       }
// TODO: 
// TODO:       for (i = 0; i < arraySize; i++)
// TODO:       {
// TODO:          RangesetForget(rangesetTable, deleteLabels[i]);
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if (!readIntArg(argList[0], &label, errMsg)
// TODO:             || !RangesetLabelOK(label))
// TODO:       {
// TODO:          M_FAILURE("Invalid rangeset label in %s");
// TODO:       }
// TODO: 
// TODO:       if (rangesetTable != NULL)
// TODO:       {
// TODO:          RangesetForget(rangesetTable, label);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    // set up result 
// TODO:    result->tag = NO_TAG;
   return true;
}


/*
** Built-in macro subroutine for getting all range sets with a specfic name.
** Arguments are $1: range set name.
** return value is an array indexed 0 to n, with the rangeset labels as values;
*/
static int rangesetGetByNameMS(WindowInfo* window, DataValue* argList, int nArgs,
                               DataValue* result, char** errMsg)
{
// TODO:    char stringStorage[1][TYPE_INT_STR_SIZE(int)];
// TODO:    Rangeset* rangeset;
// TODO:    int label;
// TODO:    char* name, *rangeset_name;
// TODO:    RangesetTable* rangesetTable = window->buffer->rangesetTable;
// TODO:    unsigned char* rangesetList;
// TODO:    char* allocIndexStr;
// TODO:    char indexStr[TYPE_INT_STR_SIZE(int)] ;
// TODO:    int nRangesets, i, insertIndex = 0;
// TODO:    DataValue element;
// TODO: 
// TODO:    if (nArgs != 1)
// TODO:    {
// TODO:       return wrongNArgsErr(errMsg);
// TODO:    }
// TODO: 
// TODO:    if (!readStringArg(argList[0], &name, stringStorage[0], errMsg))
// TODO:    {
// TODO:       M_FAILURE("First parameter is not a name string in %s");
// TODO:    }
// TODO: 
// TODO:    result->tag = ARRAY_TAG;
// TODO:    result->val.arrayPtr = ArrayNew();
// TODO: 
// TODO:    if (rangesetTable == NULL)
// TODO:    {
// TODO:       return true;
// TODO:    }
// TODO: 
// TODO:    rangesetList = RangesetGetList(rangesetTable);
// TODO:    nRangesets = strlen((char*)rangesetList);
// TODO:    for (i = 0; i < nRangesets; ++i)
// TODO:    {
// TODO:       label = rangesetList[i];
// TODO:       rangeset = RangesetFetch(rangesetTable, label);
// TODO:       if (rangeset)
// TODO:       {
// TODO:          rangeset_name = RangesetGetName(rangeset);
// TODO:          if (strcmp(name, rangeset_name ? rangeset_name : "") == 0)
// TODO:          {
// TODO:             element.tag = INT_TAG;
// TODO:             element.val.n = label;
// TODO: 
// TODO:             sprintf(indexStr, "%d", insertIndex);
// TODO:             allocIndexStr = AllocString(strlen(indexStr) + 1);
// TODO:             if (allocIndexStr == NULL)
// TODO:                M_FAILURE("Failed to allocate array key in %s");
// TODO: 
// TODO:             strcpy(allocIndexStr, indexStr);
// TODO: 
// TODO:             if (!ArrayInsert(result, allocIndexStr, &element))
// TODO:                M_FAILURE("Failed to insert array element in %s");
// TODO: 
// TODO:             ++insertIndex;
// TODO:          }
// TODO:       }
// TODO:    }

   return true;
}

/*
** Built-in macro subroutine for adding to a range set. Arguments are $1: range
** set label (one integer), then either (a) $2: source range set label,
** (b) $2: int start-range, $3: int end-range, (c) nothing (use selection
** if any to specify range to add - must not be rectangular). Returns the
** index of the newly added range (cases b and c), or 0 (case a).
*/
static int rangesetAddMS(WindowInfo* window, DataValue* argList, int nArgs,
                         DataValue* result, char** errMsg)
{
   Ne_Text_Buffer* buffer = window->buffer;
// TODO:    RangesetTable* rangesetTable = buffer->rangesetTable;
// TODO:    Rangeset* targetRangeset, *sourceRangeset;
// TODO:    int start, end, isRect, rectStart, rectEnd, maxpos, index;
// TODO:    int label = 0;
// TODO: 
// TODO:    if (nArgs < 1 || nArgs > 3)
// TODO:       return wrongNArgsErr(errMsg);
// TODO: 
// TODO:    if (!readIntArg(argList[0], &label, errMsg)
// TODO:          || !RangesetLabelOK(label))
// TODO:    {
// TODO:       M_FAILURE("First parameter is an invalid rangeset label in %s");
// TODO:    }
// TODO: 
// TODO:    if (rangesetTable == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    targetRangeset = RangesetFetch(rangesetTable, label);
// TODO: 
// TODO:    if (targetRangeset == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    start = end = -1;
// TODO: 
// TODO:    if (nArgs == 1)
// TODO:    {
// TODO:       // pick up current selection in this window 
// TODO:       if (!BufGetSelectionPos(buffer, &start, &end,
// TODO:                               &isRect, &rectStart, &rectEnd) || isRect)
// TODO:       {
// TODO:          M_FAILURE("Selection missing or rectangular in call to %s");
// TODO:       }
// TODO:       if (!RangesetAddBetween(targetRangeset, start, end))
// TODO:       {
// TODO:          M_FAILURE("Failure to add selection in %s");
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    if (nArgs == 2)
// TODO:    {
// TODO:       // add ranges taken from a second set 
// TODO:       if (!readIntArg(argList[1], &label, errMsg)
// TODO:             || !RangesetLabelOK(label))
// TODO:       {
// TODO:          M_FAILURE("Second parameter is an invalid rangeset label in %s");
// TODO:       }
// TODO: 
// TODO:       sourceRangeset = RangesetFetch(rangesetTable, label);
// TODO:       if (sourceRangeset == NULL)
// TODO:       {
// TODO:          M_FAILURE("Second rangeset does not exist in %s");
// TODO:       }
// TODO: 
// TODO:       RangesetAdd(targetRangeset, sourceRangeset);
// TODO:    }
// TODO: 
// TODO:    if (nArgs == 3)
// TODO:    {
// TODO:       // add a range bounded by the start and end positions in $2, $3 
// TODO:       if (!readIntArg(argList[1], &start, errMsg))
// TODO:       {
// TODO:          return false;
// TODO:       }
// TODO:       if (!readIntArg(argList[2], &end, errMsg))
// TODO:       {
// TODO:          return false;
// TODO:       }
// TODO: 
// TODO:       // make sure range is in order and fits buffer size 
// TODO:       maxpos = buffer->length;
// TODO:       if (start < 0) start = 0;
// TODO:       if (start > maxpos) start = maxpos;
// TODO:       if (end < 0) end = 0;
// TODO:       if (end > maxpos) end = maxpos;
// TODO:       if (start > end)
// TODO:       {
// TODO:          int temp = start;
// TODO:          start = end;
// TODO:          end = temp;
// TODO:       }
// TODO: 
// TODO:       if ((start != end) && !RangesetAddBetween(targetRangeset, start, end))
// TODO:       {
// TODO:          M_FAILURE("Failed to add range in %s");
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    // (to) which range did we just add? 
// TODO:    if (nArgs != 2 && start >= 0)
// TODO:    {
// TODO:       start = (start + end) / 2;      // "middle" of added range 
// TODO:       index = 1 + RangesetFindRangeOfPos(targetRangeset, start, false);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       index = 0;
// TODO:    }
// TODO: 
// TODO:    // set up result 
// TODO:    result->tag = INT_TAG;
// TODO:    result->val.n = index;
   return true;
}


/*
** Built-in macro subroutine for removing from a range set. Almost identical to
** rangesetAddMS() - only changes are from RangesetAdd()/RangesetAddBetween()
** to RangesetSubtract()/RangesetSubtractBetween(), the handling of an
** undefined destination range, and that it returns no value.
*/
static int rangesetSubtractMS(WindowInfo* window, DataValue* argList, int nArgs,
                              DataValue* result, char** errMsg)
{
// TODO:    Ne_Text_Buffer* buffer = window->buffer;
// TODO:    RangesetTable* rangesetTable = buffer->rangesetTable;
// TODO:    Rangeset* targetRangeset, *sourceRangeset;
// TODO:    int start, end, isRect, rectStart, rectEnd, maxpos;
// TODO:    int label = 0;
// TODO: 
// TODO:    if (nArgs < 1 || nArgs > 3)
// TODO:    {
// TODO:       return wrongNArgsErr(errMsg);
// TODO:    }
// TODO: 
// TODO:    if (!readIntArg(argList[0], &label, errMsg)
// TODO:          || !RangesetLabelOK(label))
// TODO:    {
// TODO:       M_FAILURE("First parameter is an invalid rangeset label in %s");
// TODO:    }
// TODO: 
// TODO:    if (rangesetTable == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    targetRangeset = RangesetFetch(rangesetTable, label);
// TODO:    if (targetRangeset == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    if (nArgs == 1)
// TODO:    {
// TODO:       // remove current selection in this window 
// TODO:       if (!BufGetSelectionPos(buffer, &start, &end, &isRect, &rectStart, &rectEnd)
// TODO:             || isRect)
// TODO:       {
// TODO:          M_FAILURE("Selection missing or rectangular in call to %s");
// TODO:       }
// TODO:       RangesetRemoveBetween(targetRangeset, start, end);
// TODO:    }
// TODO: 
// TODO:    if (nArgs == 2)
// TODO:    {
// TODO:       // remove ranges taken from a second set 
// TODO:       if (!readIntArg(argList[1], &label, errMsg)
// TODO:             || !RangesetLabelOK(label))
// TODO:       {
// TODO:          M_FAILURE("Second parameter is an invalid rangeset label in %s");
// TODO:       }
// TODO: 
// TODO:       sourceRangeset = RangesetFetch(rangesetTable, label);
// TODO:       if (sourceRangeset == NULL)
// TODO:       {
// TODO:          M_FAILURE("Second rangeset does not exist in %s");
// TODO:       }
// TODO:       RangesetRemove(targetRangeset, sourceRangeset);
// TODO:    }
// TODO: 
// TODO:    if (nArgs == 3)
// TODO:    {
// TODO:       // remove a range bounded by the start and end positions in $2, $3 
// TODO:       if (!readIntArg(argList[1], &start, errMsg))
// TODO:          return false;
// TODO:       if (!readIntArg(argList[2], &end, errMsg))
// TODO:          return false;
// TODO: 
// TODO:       // make sure range is in order and fits buffer size 
// TODO:       maxpos = buffer->length;
// TODO:       if (start < 0) start = 0;
// TODO:       if (start > maxpos) start = maxpos;
// TODO:       if (end < 0) end = 0;
// TODO:       if (end > maxpos) end = maxpos;
// TODO:       if (start > end)
// TODO:       {
// TODO:          int temp = start;
// TODO:          start = end;
// TODO:          end = temp;
// TODO:       }
// TODO: 
// TODO:       RangesetRemoveBetween(targetRangeset, start, end);
// TODO:    }
// TODO: 
// TODO:    // set up result 
// TODO:    result->tag = NO_TAG;
   return true;
}


/*
** Built-in macro subroutine to invert a range set. Argument is $1: range set
** label (one alphabetic character). Returns nothing. Fails if range set
** undefined.
*/
static int rangesetInvertMS(WindowInfo* window, DataValue* argList, int nArgs,
                            DataValue* result, char** errMsg)
{

// TODO:    RangesetTable* rangesetTable = window->buffer->rangesetTable;
// TODO:    Rangeset* rangeset;
// TODO:    int label = 0;
// TODO: 
// TODO:    if (nArgs != 1)
// TODO:       return wrongNArgsErr(errMsg);
// TODO: 
// TODO:    if (!readIntArg(argList[0], &label, errMsg)
// TODO:          || !RangesetLabelOK(label))
// TODO:    {
// TODO:       M_FAILURE("First parameter is an invalid rangeset label in %s");
// TODO:    }
// TODO: 
// TODO:    if (rangesetTable == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    rangeset = RangesetFetch(rangesetTable, label);
// TODO:    if (rangeset == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    if (RangesetInverse(rangeset) < 0)
// TODO:    {
// TODO:       M_FAILURE("Problem inverting rangeset in %s");
// TODO:    }
// TODO: 
// TODO:    // set up result 
// TODO:    result->tag = NO_TAG;
   return true;
}


/*
** Built-in macro subroutine for finding out info about a rangeset.  Takes one
** argument of a rangeset label.  Returns an array with the following keys:
**    defined, count, color, mode.
*/
static int rangesetInfoMS(WindowInfo* window, DataValue* argList, int nArgs,
                          DataValue* result, char** errMsg)
{
// TODO:    RangesetTable* rangesetTable = window->buffer->rangesetTable;
// TODO:    Rangeset* rangeset = NULL;
// TODO:    int count, defined;
// TODO:    char* color, *name, *mode;
// TODO:    DataValue element;
// TODO:    int label = 0;
// TODO: 
// TODO:    if (nArgs != 1)
// TODO:       return wrongNArgsErr(errMsg);
// TODO: 
// TODO:    if (!readIntArg(argList[0], &label, errMsg)
// TODO:          || !RangesetLabelOK(label))
// TODO:    {
// TODO:       M_FAILURE("First parameter is an invalid rangeset label in %s");
// TODO:    }
// TODO: 
// TODO:    if (rangesetTable != NULL)
// TODO:    {
// TODO:       rangeset = RangesetFetch(rangesetTable, label);
// TODO:    }
// TODO: 
// TODO:    RangesetGetInfo(rangeset, &defined, &label, &count, &color, &name, &mode);
// TODO: 
// TODO:    // set up result 
// TODO:    result->tag = ARRAY_TAG;
// TODO:    result->val.arrayPtr = ArrayNew();
// TODO: 
// TODO:    element.tag = INT_TAG;
// TODO:    element.val.n = defined;
// TODO:    if (!ArrayInsert(result, PERM_ALLOC_STR("defined"), &element))
// TODO:       M_FAILURE("Failed to insert array element \"defined\" in %s");
// TODO: 
// TODO:    element.tag = INT_TAG;
// TODO:    element.val.n = count;
// TODO:    if (!ArrayInsert(result, PERM_ALLOC_STR("count"), &element))
// TODO:       M_FAILURE("Failed to insert array element \"count\" in %s");
// TODO: 
// TODO:    element.tag = STRING_TAG;
// TODO:    if (!AllocNStringCpy(&element.val.str, color))
// TODO:       M_FAILURE("Failed to allocate array value \"color\" in %s");
// TODO:    if (!ArrayInsert(result, PERM_ALLOC_STR("color"), &element))
// TODO:       M_FAILURE("Failed to insert array element \"color\" in %s");
// TODO: 
// TODO:    element.tag = STRING_TAG;
// TODO:    if (!AllocNStringCpy(&element.val.str, name))
// TODO:       M_FAILURE("Failed to allocate array value \"name\" in %s");
// TODO:    if (!ArrayInsert(result, PERM_ALLOC_STR("name"), &element))
// TODO:    {
// TODO:       M_FAILURE("Failed to insert array element \"name\" in %s");
// TODO:    }
// TODO: 
// TODO:    element.tag = STRING_TAG;
// TODO:    if (!AllocNStringCpy(&element.val.str, mode))
// TODO:       M_FAILURE("Failed to allocate array value \"mode\" in %s");
// TODO:    if (!ArrayInsert(result, PERM_ALLOC_STR("mode"), &element))
// TODO:       M_FAILURE("Failed to insert array element \"mode\" in %s");

   return true;
}

/*
** Built-in macro subroutine for finding the extent of a range in a set.
** If only one parameter is supplied, use the spanning range of all
** ranges, otherwise select the individual range specified.  Returns
** an array with the keys "start" and "end" and values
*/
static int rangesetRangeMS(WindowInfo* window, DataValue* argList, int nArgs,
                           DataValue* result, char** errMsg)
{
// TODO:    Ne_Text_Buffer* buffer = window->buffer;
// TODO:    RangesetTable* rangesetTable = buffer->rangesetTable;
// TODO:    Rangeset* rangeset;
// TODO:    int start, end, dummy, rangeIndex, ok;
// TODO:    DataValue element;
// TODO:    int label = 0;
// TODO: 
// TODO:    if (nArgs < 1 || nArgs > 2)
// TODO:    {
// TODO:       return wrongNArgsErr(errMsg);
// TODO:    }
// TODO: 
// TODO:    if (!readIntArg(argList[0], &label, errMsg)
// TODO:          || !RangesetLabelOK(label))
// TODO:    {
// TODO:       M_FAILURE("First parameter is an invalid rangeset label in %s");
// TODO:    }
// TODO: 
// TODO:    if (rangesetTable == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    ok = false;
// TODO:    rangeset = RangesetFetch(rangesetTable, label);
// TODO:    if (rangeset != NULL)
// TODO:    {
// TODO:       if (nArgs == 1)
// TODO:       {
// TODO:          rangeIndex = RangesetGetNRanges(rangeset) - 1;
// TODO:          ok = RangesetFindRangeNo(rangeset, 0, &start, &dummy);
// TODO:          ok &= RangesetFindRangeNo(rangeset, rangeIndex, &dummy, &end);
// TODO:          rangeIndex = -1;
// TODO:       }
// TODO:       else if (nArgs == 2)
// TODO:       {
// TODO:          if (!readIntArg(argList[1], &rangeIndex, errMsg))
// TODO:          {
// TODO:             return false;
// TODO:          }
// TODO:          ok = RangesetFindRangeNo(rangeset, rangeIndex-1, &start, &end);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    // set up result 
// TODO:    result->tag = ARRAY_TAG;
// TODO:    result->val.arrayPtr = ArrayNew();
// TODO: 
// TODO:    if (!ok)
// TODO:       return true;
// TODO: 
// TODO:    element.tag = INT_TAG;
// TODO:    element.val.n = start;
// TODO:    if (!ArrayInsert(result, PERM_ALLOC_STR("start"), &element))
// TODO:       M_FAILURE("Failed to insert array element \"start\" in %s");
// TODO: 
// TODO:    element.tag = INT_TAG;
// TODO:    element.val.n = end;
// TODO:    if (!ArrayInsert(result, PERM_ALLOC_STR("end"), &element))
// TODO:       M_FAILURE("Failed to insert array element \"end\" in %s");

   return true;
}

/*
** Built-in macro subroutine for checking a position against a range. If only
** one parameter is supplied, the current cursor position is used. Returns
** false (zero) if not in a range, range index (1-based) if in a range;
** fails if parameters were bad.
*/
static int rangesetIncludesPosMS(WindowInfo* window, DataValue* argList,
                                 int nArgs, DataValue* result, char** errMsg)
{
   Ne_Text_Buffer* buffer = window->buffer;
// TODO:    RangesetTable* rangesetTable = buffer->rangesetTable;
// TODO:    Rangeset* rangeset;
// TODO:    int pos, rangeIndex, maxpos;
// TODO:    int label = 0;
// TODO: 
// TODO:    if (nArgs < 1 || nArgs > 2)
// TODO:    {
// TODO:       return wrongNArgsErr(errMsg);
// TODO:    }
// TODO: 
// TODO:    if (!readIntArg(argList[0], &label, errMsg)
// TODO:          || !RangesetLabelOK(label))
// TODO:    {
// TODO:       M_FAILURE("First parameter is an invalid rangeset label in %s");
// TODO:    }
// TODO: 
// TODO:    if (rangesetTable == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    rangeset = RangesetFetch(rangesetTable, label);
// TODO:    if (rangeset == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    if (nArgs == 1)
// TODO:    {
// TODO:       pos = TextGetCursorPos(window->lastFocus);
// TODO:    }
// TODO:    else if (nArgs == 2)
// TODO:    {
// TODO:       if (!readIntArg(argList[1], &pos, errMsg))
// TODO:          return false;
// TODO:    }
// TODO: 
// TODO:    maxpos = buffer->length;
// TODO:    if (pos < 0 || pos > maxpos)
// TODO:    {
// TODO:       rangeIndex = 0;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       rangeIndex = RangesetFindRangeOfPos(rangeset, pos, false) + 1;
// TODO:    }
// TODO: 
// TODO:    // set up result 
// TODO:    result->tag = INT_TAG;
// TODO:    result->val.n = rangeIndex;
   return true;
}

/*
** Set the color of a range set's ranges. it is ignored if the color cannot be
** found/applied. If no color is applied, any current color is removed. Returns
** true if the rangeset is valid.
*/
static int rangesetSetColorMS(WindowInfo* window, DataValue* argList,
                              int nArgs, DataValue* result, char** errMsg)
{
   char stringStorage[1][TYPE_INT_STR_SIZE(int)];
   Ne_Text_Buffer* buffer = window->buffer;
// TODO:    RangesetTable* rangesetTable = buffer->rangesetTable;
// TODO:    Rangeset* rangeset;
// TODO:    char* color_name;
// TODO:    int label = 0;
// TODO: 
// TODO:    if (nArgs != 2)
// TODO:    {
// TODO:       return wrongNArgsErr(errMsg);
// TODO:    }
// TODO: 
// TODO:    if (!readIntArg(argList[0], &label, errMsg)
// TODO:          || !RangesetLabelOK(label))
// TODO:    {
// TODO:       M_FAILURE("First parameter is an invalid rangeset label in %s");
// TODO:    }
// TODO: 
// TODO:    if (rangesetTable == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    rangeset = RangesetFetch(rangesetTable, label);
// TODO:    if (rangeset == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    color_name = "";
// TODO:    if (rangeset != NULL)
// TODO:    {
// TODO:       if (!readStringArg(argList[1], &color_name, stringStorage[0], errMsg))
// TODO:       {
// TODO:          M_FAILURE("Second parameter is not a color name string in %s");
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    RangesetAssignColorName(rangeset, color_name);
// TODO: 
// TODO:    // set up result 
// TODO:    result->tag = NO_TAG;
   return true;
}

/*
** Set the name of a range set's ranges. Returns
** true if the rangeset is valid.
*/
static int rangesetSetNameMS(WindowInfo* window, DataValue* argList,
                             int nArgs, DataValue* result, char** errMsg)
{
   char stringStorage[1][TYPE_INT_STR_SIZE(int)];
   Ne_Text_Buffer* buffer = window->buffer;
// TODO:    RangesetTable* rangesetTable = buffer->rangesetTable;
// TODO:    Rangeset* rangeset;
// TODO:    char* name;
// TODO:    int label = 0;
// TODO: 
// TODO:    if (nArgs != 2)
// TODO:    {
// TODO:       return wrongNArgsErr(errMsg);
// TODO:    }
// TODO: 
// TODO:    if (!readIntArg(argList[0], &label, errMsg)
// TODO:          || !RangesetLabelOK(label))
// TODO:    {
// TODO:       M_FAILURE("First parameter is an invalid rangeset label in %s");
// TODO:    }
// TODO: 
// TODO:    if (rangesetTable == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    rangeset = RangesetFetch(rangesetTable, label);
// TODO:    if (rangeset == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    name = "";
// TODO:    if (rangeset != NULL)
// TODO:    {
// TODO:       if (!readStringArg(argList[1], &name, stringStorage[0], errMsg))
// TODO:       {
// TODO:          M_FAILURE("Second parameter is not a valid name string in %s");
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    RangesetAssignName(rangeset, name);
// TODO: 
// TODO:    // set up result 
// TODO:    result->tag = NO_TAG;
   return true;
}

/*
** Change a range's modification response. Returns true if the rangeset is
** valid and the response type name is valid.
*/
static int rangesetSetModeMS(WindowInfo* window, DataValue* argList,
                             int nArgs, DataValue* result, char** errMsg)
{
   char stringStorage[1][TYPE_INT_STR_SIZE(int)];
   Ne_Text_Buffer* buffer = window->buffer;
// TODO:    RangesetTable* rangesetTable = buffer->rangesetTable;
// TODO:    Rangeset* rangeset;
// TODO:    char* update_fn_name;
// TODO:    int ok;
// TODO:    int label = 0;
// TODO: 
// TODO:    if (nArgs < 1 || nArgs > 2)
// TODO:    {
// TODO:       return wrongNArgsErr(errMsg);
// TODO:    }
// TODO: 
// TODO:    if (!readIntArg(argList[0], &label, errMsg)
// TODO:          || !RangesetLabelOK(label))
// TODO:    {
// TODO:       M_FAILURE("First parameter is an invalid rangeset label in %s");
// TODO:    }
// TODO: 
// TODO:    if (rangesetTable == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    rangeset = RangesetFetch(rangesetTable, label);
// TODO:    if (rangeset == NULL)
// TODO:    {
// TODO:       M_FAILURE("Rangeset does not exist in %s");
// TODO:    }
// TODO: 
// TODO:    update_fn_name = "";
// TODO:    if (rangeset != NULL)
// TODO:    {
// TODO:       if (nArgs == 2)
// TODO:       {
// TODO:          if (!readStringArg(argList[1], &update_fn_name, stringStorage[0], errMsg))
// TODO:          {
// TODO:             M_FAILURE("Second parameter is not a string in %s");
// TODO:          }
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    ok = RangesetChangeModifyResponse(rangeset, update_fn_name);
// TODO: 
// TODO:    if (!ok)
// TODO:    {
// TODO:       M_FAILURE("Second parameter is not a valid mode in %s");
// TODO:    }
// TODO: 
// TODO:    // set up result 
// TODO:    result->tag = NO_TAG;
   return true;
}

// -------------------------------------------------------------------------- 


/*
** Routines to get details directly from the window.
*/

/*
** Sets up an array containing information about a style given its name or
** a buffer position (bufferPos >= 0) and its highlighting pattern code
** (patCode >= 0).
** From the name we obtain:
**      ["color"]       Foreground color name of style
**      ["background"]  Background color name of style if specified
**      ["bold"]        '1' if style is bold, '0' otherwise
**      ["italic"]      '1' if style is italic, '0' otherwise
** Given position and pattern code we obtain:
**      ["rgb"]         RGB representation of foreground color of style
**      ["back_rgb"]    RGB representation of background color of style
**      ["extent"]      Forward distance from position over which style applies
** We only supply the style name if the includeName parameter is set:
**      ["style"]       Name of style
**
*/
static int fillStyleResult(DataValue* result, char** errMsg,
                           WindowInfo* window, char* styleName, bool preallocatedStyleName,
                           bool includeName, int patCode, int bufferPos)
{
   DataValue DV;
   char colorValue[20];
   int r, g, b;

   // initialize array 
   result->tag = ARRAY_TAG;
   result->val.arrayPtr = ArrayNew();

   // the following array entries will be strings 
   DV.tag = STRING_TAG;

   if (includeName)
   {
      // insert style name 
      if (preallocatedStyleName)
      {
         DV.val.str.rep = styleName;
         DV.val.str.len = strlen(styleName);
      }
      else
      {
         AllocNStringCpy(&DV.val.str, styleName);
      }
      M_STR_ALLOC_ASSERT(DV);
      if (!ArrayInsert(result, PERM_ALLOC_STR("style"), &DV))
      {
         M_ARRAY_INSERT_FAILURE();
      }
   }

   // insert color name 
   AllocNStringCpy(&DV.val.str, ColorOfNamedStyle(styleName));
   M_STR_ALLOC_ASSERT(DV);
   if (!ArrayInsert(result, PERM_ALLOC_STR("color"), &DV))
   {
      M_ARRAY_INSERT_FAILURE();
   }

   /* Prepare array element for color value
      (only possible if we pass through the dynamic highlight pattern tables
      in other words, only if we have a pattern code) */
   if (patCode)
   {
      HighlightColorValueOfCode(window, patCode, &r, &g, &b);
      sprintf(colorValue, "#%02x%02x%02x", r/256, g/256, b/256);
      AllocNStringCpy(&DV.val.str, colorValue);
      M_STR_ALLOC_ASSERT(DV);
      if (!ArrayInsert(result, PERM_ALLOC_STR("rgb"), &DV))
      {
         M_ARRAY_INSERT_FAILURE();
      }
   }

   // Prepare array element for background color name 
   AllocNStringCpy(&DV.val.str, BgColorOfNamedStyle(styleName));
   M_STR_ALLOC_ASSERT(DV);
   if (!ArrayInsert(result, PERM_ALLOC_STR("background"), &DV))
   {
      M_ARRAY_INSERT_FAILURE();
   }

   /* Prepare array element for background color value
      (only possible if we pass through the dynamic highlight pattern tables
      in other words, only if we have a pattern code) */
   if (patCode)
   {
      GetHighlightBGColorOfCode(window, patCode, &r, &g, &b);
      sprintf(colorValue, "#%02x%02x%02x", r/256, g/256, b/256);
      AllocNStringCpy(&DV.val.str, colorValue);
      M_STR_ALLOC_ASSERT(DV);
      if (!ArrayInsert(result, PERM_ALLOC_STR("back_rgb"), &DV))
      {
         M_ARRAY_INSERT_FAILURE();
      }
   }

   // the following array entries will be integers 
   DV.tag = INT_TAG;

   // Put boldness value in array 
   DV.val.n = FontOfNamedStyleIsBold(styleName);
   if (!ArrayInsert(result, PERM_ALLOC_STR("bold"), &DV))
   {
      M_ARRAY_INSERT_FAILURE();
   }

   // Put italicity value in array 
   DV.val.n = FontOfNamedStyleIsItalic(styleName);
   if (!ArrayInsert(result, PERM_ALLOC_STR("italic"), &DV))
   {
      M_ARRAY_INSERT_FAILURE();
   }

   if (bufferPos >= 0)
   {
      // insert extent 
      const char* styleNameNotUsed = NULL;
      DV.val.n = StyleLengthOfCodeFromPos(window, bufferPos, &styleNameNotUsed);
      if (!ArrayInsert(result, PERM_ALLOC_STR("extent"), &DV))
      {
         M_ARRAY_INSERT_FAILURE();
      }
   }
   return true;
}

/*
** Returns an array containing information about the style of name $1
**      ["color"]       Foreground color name of style
**      ["background"]  Background color name of style if specified
**      ["bold"]        '1' if style is bold, '0' otherwise
**      ["italic"]      '1' if style is italic, '0' otherwise
**
*/
static int getStyleByNameMS(WindowInfo* window, DataValue* argList, int nArgs,
                            DataValue* result, char** errMsg)
{
   char stringStorage[1][TYPE_INT_STR_SIZE(int)];
   char* styleName;

   // Validate number of arguments 
   if (nArgs != 1)
   {
      return wrongNArgsErr(errMsg);
   }

   // Prepare result 
   result->tag = ARRAY_TAG;
   result->val.arrayPtr = NULL;

   if (!readStringArg(argList[0], &styleName, stringStorage[0], errMsg))
   {
      M_FAILURE("First parameter is not a string in %s");
   }

   if (!NamedStyleExists(styleName))
   {
      // if the given name is invalid we just return an empty array. 
      return true;
   }

   return fillStyleResult(result, errMsg, window,
                          styleName, (argList[0].tag == STRING_TAG), false, 0, -1);
}

/*
** Returns an array containing information about the style of position $1
**      ["style"]       Name of style
**      ["color"]       Foreground color name of style
**      ["background"]  Background color name of style if specified
**      ["bold"]        '1' if style is bold, '0' otherwise
**      ["italic"]      '1' if style is italic, '0' otherwise
**      ["rgb"]         RGB representation of foreground color of style
**      ["back_rgb"]    RGB representation of background color of style
**      ["extent"]      Forward distance from position over which style applies
**
*/
static int getStyleAtPosMS(WindowInfo* window, DataValue* argList, int nArgs,
                           DataValue* result, char** errMsg)
{
   int patCode;
   int bufferPos;
   Ne_Text_Buffer* buf = window->buffer;

   // Validate number of arguments 
   if (nArgs != 1)
   {
      return wrongNArgsErr(errMsg);
   }

   // Prepare result 
   result->tag = ARRAY_TAG;
   result->val.arrayPtr = NULL;

   if (!readIntArg(argList[0], &bufferPos, errMsg))
   {
      return false;
   }

   //  Verify sane buffer position 
   if ((bufferPos < 0) || (bufferPos >= buf->length))
   {
      /*  If the position is not legal, we cannot guess anything about
          the style, so we return an empty array. */
      return true;
   }

   // Determine pattern code 
   patCode = HighlightCodeOfPos(window, bufferPos);
   if (patCode == 0)
   {
      // if there is no pattern we just return an empty array. 
      return true;
   }

   return fillStyleResult(result, errMsg, window,
                          HighlightStyleOfCode(window, patCode), false, true, patCode, bufferPos);
}

/*
** Sets up an array containing information about a pattern given its name or
** a buffer position (bufferPos >= 0).
** From the name we obtain:
**      ["style"]       Name of style
**      ["extent"]      Forward distance from position over which style applies
** We only supply the pattern name if the includeName parameter is set:
**      ["pattern"]     Name of pattern
**
*/
static int fillPatternResult(DataValue* result, char** errMsg,
                             WindowInfo* window, char* patternName, bool preallocatedPatternName,
                             bool includeName, char* styleName, int bufferPos)
{
   DataValue DV;

   // initialize array 
   result->tag = ARRAY_TAG;
   result->val.arrayPtr = ArrayNew();

   // the following array entries will be strings 
   DV.tag = STRING_TAG;

   if (includeName)
   {
      // insert pattern name 
      if (preallocatedPatternName)
      {
         DV.val.str.rep = patternName;
         DV.val.str.len = strlen(patternName);
      }
      else
      {
         AllocNStringCpy(&DV.val.str, patternName);
      }
      M_STR_ALLOC_ASSERT(DV);
      if (!ArrayInsert(result, PERM_ALLOC_STR("pattern"), &DV))
      {
         M_ARRAY_INSERT_FAILURE();
      }
   }

   // insert style name 
   AllocNStringCpy(&DV.val.str, styleName);
   M_STR_ALLOC_ASSERT(DV);
   if (!ArrayInsert(result, PERM_ALLOC_STR("style"), &DV))
   {
      M_ARRAY_INSERT_FAILURE();
   }

   // the following array entries will be integers 
   DV.tag = INT_TAG;

   if (bufferPos >= 0)
   {
      // insert extent 
      int checkCode = 0;
      DV.val.n = HighlightLengthOfCodeFromPos(window, bufferPos, &checkCode);
      if (!ArrayInsert(result, PERM_ALLOC_STR("extent"), &DV))
      {
         M_ARRAY_INSERT_FAILURE();
      }
   }

   return true;
}

/*
** Returns an array containing information about a highlighting pattern. The
** single parameter contains the pattern name for which this information is
** requested.
** The returned array looks like this:
**      ["style"]       Name of style
*/
static int getPatternByNameMS(WindowInfo* window, DataValue* argList, int nArgs,
                              DataValue* result, char** errMsg)
{
   char stringStorage[1][TYPE_INT_STR_SIZE(int)];
   char* patternName = NULL;
   highlightPattern* pattern;

   // Begin of building the result. 
   result->tag = ARRAY_TAG;
   result->val.arrayPtr = NULL;

   // Validate number of arguments 
   if (nArgs != 1)
   {
      return wrongNArgsErr(errMsg);
   }

   if (!readStringArg(argList[0], &patternName, stringStorage[0], errMsg))
   {
      M_FAILURE("First parameter is not a string in %s");
   }

   pattern = FindPatternOfWindow(window, patternName);
   if (pattern == NULL)
   {
      // The pattern's name is unknown. 
      return true;
   }

   return fillPatternResult(result, errMsg, window, patternName,
                            (argList[0].tag == STRING_TAG), false, pattern->style, -1);
}

/*
** Returns an array containing information about the highlighting pattern
** applied at a given position, passed as the only parameter.
** The returned array looks like this:
**      ["pattern"]     Name of pattern
**      ["style"]       Name of style
**      ["extent"]      Distance from position over which this pattern applies
*/
static int getPatternAtPosMS(WindowInfo* window, DataValue* argList, int nArgs,
                             DataValue* result, char** errMsg)
{
   int bufferPos = -1;
   Ne_Text_Buffer* buffer = window->buffer;
   int patCode = 0;

   // Begin of building the result. 
   result->tag = ARRAY_TAG;
   result->val.arrayPtr = NULL;

   // Validate number of arguments 
   if (nArgs != 1)
   {
      return wrongNArgsErr(errMsg);
   }

   /* The most straightforward case: Get a pattern, style and extent
      for a buffer position. */
   if (!readIntArg(argList[0], &bufferPos, errMsg))
   {
      return false;
   }

   /*  Verify sane buffer position
    *  You would expect that buffer->length would be among the sane
    *  positions, but we have n characters and n+1 buffer positions. */
   if ((bufferPos < 0) || (bufferPos >= buffer->length))
   {
      /*  If the position is not legal, we cannot guess anything about
          the highlighting pattern, so we return an empty array. */
      return true;
   }

   // Determine the highlighting pattern used 
   patCode = HighlightCodeOfPos(window, bufferPos);
   if (patCode == 0)
   {
      // if there is no highlighting pattern we just return an empty array. 
      return true;
   }

   return fillPatternResult(result, errMsg, window,
                            HighlightNameOfCode(window, patCode), false, true,
                            HighlightStyleOfCode(window, patCode), bufferPos);
}

static int wrongNArgsErr(char** errMsg)
{
   *errMsg = "Wrong number of arguments to function %s";
   return false;
}

static int tooFewArgsErr(char** errMsg)
{
   *errMsg = "Too few arguments to function %s";
   return false;
}

/*
** strCaseCmp compares its arguments and returns 0 if the two strings
** are equal IGNORING case differences.  Otherwise returns 1 or -1
** depending on relative comparison.
*/
static int strCaseCmp(char* str1, char* str2)
{
   char* c1, *c2;

   for (c1 = str1, c2 = str2;
         (*c1 != '\0' && *c2 != '\0')
         && toupper((unsigned char)*c1) == toupper((unsigned char)*c2);
         ++c1, ++c2)
   {
   }

   if (((unsigned char)toupper((unsigned char)*c1))
         > ((unsigned char)toupper((unsigned char)*c2)))
   {
      return(1);
   }
   else if (((unsigned char)toupper((unsigned char)*c1))
            < ((unsigned char)toupper((unsigned char)*c2)))
   {
      return(-1);
   }
   else
   {
      return(0);
   }
}

/*
** Get an integer value from a tagged DataValue structure.  Return true
** if conversion succeeded, and store result in *result, otherwise
** return false with an error message in *errMsg.
*/
static int readIntArg(DataValue dv, int* result, char** errMsg)
{
   char* c;

   if (dv.tag == INT_TAG)
   {
      *result = dv.val.n;
      return true;
   }
   else if (dv.tag == STRING_TAG)
   {
      for (c=dv.val.str.rep; *c != '\0'; c++)
      {
         if (!(isdigit((unsigned char)*c) || *c == ' ' || *c == '\t'))
         {
            goto typeError;
         }
      }
      sscanf(dv.val.str.rep, "%d", result);
      return true;
   }

typeError:
   *errMsg = "%s called with non-integer argument";
   return false;
}

/*
** Get an string value from a tagged DataValue structure.  Return true
** if conversion succeeded, and store result in *result, otherwise
** return false with an error message in *errMsg.  If an integer value
** is converted, write the string in the space provided by "stringStorage",
** which must be large enough to handle ints of the maximum size.
*/
static int readStringArg(DataValue dv, char** result, char* stringStorage,
                         char** errMsg)
{
   if (dv.tag == STRING_TAG)
   {
      *result = dv.val.str.rep;
      return true;
   }
   else if (dv.tag == INT_TAG)
   {
      sprintf(stringStorage, "%d", dv.val.n);
      *result = stringStorage;
      return true;
   }
   *errMsg = "%s called with unknown object";
   return false;
}
