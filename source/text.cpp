// TODO: static const char CVSID[] = "$Id: text.c,v 1.57 2008/01/04 22:11:04 yooden Exp $";
// TODO: /*******************************************************************************
// TODO: *									       *
// TODO: * text.c - Display text from a text buffer				       *
// TODO: *									       *
// TODO: * Copyright (C) 1999 Mark Edel						       *
// TODO: *									       *
// TODO: * This is free__ software; you can redistribute it and/or modify it under the    *
// TODO: * terms of the GNU General Public License as published by the Free Software    *
// TODO: * Foundation; either version 2 of the License, or (at your option) any later   *
// TODO: * version. In addition, you may distribute version of this program linked to   *
// TODO: * Motif or Open Motif. See README for details.                                 *
// TODO: * 									       *
// TODO: * This software is distributed in the hope that it will be useful, but WITHOUT *
// TODO: * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
// TODO: * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
// TODO: * for more details.							       *
// TODO: * 									       *
// TODO: * You should have received a copy of the GNU General Public License along with *
// TODO: * software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
// TODO: * Place, Suite 330, Boston, MA  02111-1307 USA		                       *
// TODO: *									       *
// TODO: * Nirvana Text Editor	    						       *
// TODO: * June 15, 1995								       *
// TODO: *									       *
// TODO: *******************************************************************************/
// TODO: 
// TODO: #ifdef HAVE_CONFIG_H
// TODO: #include "../config.h"
// TODO: #endif
// TODO: 
// TODO: #include "Ne_Text_Editor.h"
// TODO: #include "Ne_Text_Part.h"
// TODO: #include "Ne_Text_Buffer.h"
// TODO: #include "Ne_Text_Display.h"
// TODO: #include "Ne_Text_Sel.h"
// TODO: #include "Ne_Text_Drag.h"
// TODO: #include "nedit.h"
// TODO: #include "calltips.h"
// TODO: #include "../util/DialogF.h"
// TODO: #include "window.h"
// TODO: 
// TODO: #include <stdio.h>
// TODO: #include <stdlib.h>
// TODO: #include <limits.h>
// TODO: #include <string.h>
// TODO: #include <ctype.h>
// TODO: #ifdef VMS
// TODO: #include "../util/VMSparam.h"
// TODO: #else
// TODO: #ifndef WIN32
// TODO: #include <sys/param.h>
// TODO: #endif
// TODO: #endif /*VMS*/
// TODO: #include <limits.h>
// TODO: 
// TODO: #ifdef HAVE_DEBUG_H
// TODO: #include "../debug.h"
// TODO: #endif
// TODO: 
// TODO: 
// TODO: #ifdef UNICOS
// TODO: #define XtOffset(p_type,field) ((size_t)__INTADDR__(&(((p_type)0)->field)))
// TODO: #endif
// TODO: 
// TODO: /* Number of pixels of motion from the initial (grab-focus) button press
// TODO:    required to begin recognizing a mouse drag for the purpose of making a
// TODO:    selection */
// TODO: #define SELECT_THRESHOLD 5
// TODO: 
// TODO: /* Length of delay in milliseconds for vertical autoscrolling */
// TODO: #define VERTICAL_SCROLL_DELAY 50
// TODO: 
// TODO: // TODO: static void initialize(TextWidget request, TextWidget newWidget);
// TODO: // TODO: static void handleHidePointer(Fl_Widget* w, XtPointer unused,
// TODO: // TODO:                               int event, bool* continue_to_dispatch);
// TODO: // TODO: static void handleShowPointer(Fl_Widget* w, XtPointer unused,
// TODO: // TODO:                               int event, bool* continue_to_dispatch);
// TODO: // TODO: static void redisplay(TextWidget w, int event, Region region);
// TODO: // TODO: static void redisplayGE(TextWidget w, XtPointer client_data,
// TODO: // TODO:                         int event, bool* continue_to_dispatch_return);
// TODO: // TODO: static void destroy(TextWidget w);
// TODO: // TODO: static void resize(TextWidget w);
// TODO: // TODO: static bool setValues(TextWidget current, TextWidget request,
// TODO: // TODO:                          TextWidget newWidget);
// TODO: // TODO: static void realize(Fl_Widget* w, XtValueMask* valueMask,
// TODO: // TODO:                     XSetWindowAttributes* attributes);
// TODO: // TODO: static XtGeometryResult queryGeometry(Fl_Widget* w, XtWidgetGeometry* proposed,
// TODO: // TODO:                                       XtWidgetGeometry* answer);
// TODO: // TODO: static void grabFocusAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                         int* n_args);
// TODO: // TODO: static void moveDestinationAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs);
// TODO: // TODO: static void extendAdjustAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                            int* nArgs);
// TODO: // TODO: static void extendStartAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                           int* nArgs);
// TODO: // TODO: static void extendEndAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                         int* nArgs);
// TODO: // TODO: static void processCancelAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                             int* nArgs);
// TODO: // TODO: static void secondaryStartAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                              int* nArgs);
// TODO: // TODO: static void secondaryOrDragStartAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                    int* nArgs);
// TODO: // TODO: static void secondaryAdjustAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs);
// TODO: // TODO: static void secondaryOrDragAdjustAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                     int* nArgs);
// TODO: // TODO: static void copyToAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void copyToOrEndDragAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs);
// TODO: // TODO: static void copyPrimaryAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                           int* nArgs);
// TODO: // TODO: static void cutPrimaryAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                          int* nArgs);
// TODO: // TODO: static void moveToAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void moveToOrEndDragAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs);
// TODO: // TODO: static void endDragAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void exchangeAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void mousePanAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void pasteClipboardAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                              int* nArgs);
// TODO: // TODO: static void copyClipboardAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                             int* nArgs);
// TODO: // TODO: static void cutClipboardAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                            int* nArgs);
// TODO: // TODO: static void insertStringAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                            int* nArgs);
// TODO: // TODO: static void selfInsertAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                          int* n_args);
// TODO: // TODO: static void newlineAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void newlineAndIndentAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                int* nArgs);
// TODO: // TODO: static void newlineNoIndentAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs);
// TODO: // TODO: static void processTabAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                          int* nArgs);
// TODO: // TODO: static void endOfLineAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void beginningOfLineAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs);
// TODO: // TODO: static void deleteSelectionAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs);
// TODO: // TODO: static void deletePreviousCharacterAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                       int* nArgs);
// TODO: // TODO: static void deleteNextCharacterAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                   int* nArgs);
// TODO: // TODO: static void deletePreviousWordAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                  int* nArgs);
// TODO: // TODO: static void deleteNextWordAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                              int* nArgs);
// TODO: // TODO: static void deleteToStartOfLineAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                   int* nArgs);
// TODO: // TODO: static void deleteToEndOfLineAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                 int* nArgs);
// TODO: // TODO: static void forwardCharacterAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                int* nArgs);
// TODO: // TODO: static void backwardCharacterAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                 int* nArgs);
// TODO: // TODO: static void forwardWordAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                           int* nArgs);
// TODO: // TODO: static void backwardWordAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                            int* nArgs);
// TODO: // TODO: static void forwardParagraphAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                int* nArgs);
// TODO: // TODO: static void backwardParagraphAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                 int* nArgs);
// TODO: // TODO: static void keySelectAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void processUpAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void processShiftUpAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                              int* nArgs);
// TODO: // TODO: static void processDownAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                           int* nArgs);
// TODO: // TODO: static void processShiftDownAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                int* nArgs);
// TODO: // TODO: static void beginningOfFileAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs);
// TODO: // TODO: static void endOfFileAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void nextPageAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void previousPageAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                            int* nArgs);
// TODO: // TODO: static void pageLeftAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void pageRightAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void toggleOverstrikeAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                int* nArgs);
// TODO: // TODO: static void scrollUpAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                        int* nArgs);
// TODO: // TODO: static void scrollDownAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: // TODO: static void scrollLeftAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                          int* nArgs);
// TODO: // TODO: static void scrollRightAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                           int* nArgs);
// TODO: // TODO: static void scrollToLineAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                            int* nArgs);
// TODO: // TODO: static void selectAllAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                         int* nArgs);
// TODO: // TODO: static void deselectAllAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                           int* nArgs);
// TODO: // TODO: static void focusInAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                       int* nArgs);
// TODO: // TODO: static void focusOutAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                        int* nArgs);
// TODO: // TODO: static void checkMoveSelectionChange(Fl_Widget* w, int event, int startPos,
// TODO: // TODO:                                      const char** args, int* nArgs);
// TODO: // TODO: static void keyMoveExtendSelection(Fl_Widget* w, int event, int startPos,
// TODO: // TODO:                                    int rectangular);
// TODO: // TODO: static void checkAutoShowInsertPos(Fl_Widget* w);
// TODO: // TODO: static int checkReadOnly(Fl_Widget* w);
// TODO: // TODO: static void simpleInsertAtCursor(Fl_Widget* w, char* chars, int event,
// TODO: // TODO:                                  int allowPendingDelete);
// TODO: // TODO: static int pendingSelection(Fl_Widget* w);
// TODO: // TODO: static int deletePendingSelection(Fl_Widget* w, int event);
// TODO: // TODO: static int deleteEmulatedTab(Fl_Widget* w, int event);
// TODO: // TODO: static void selectWord(Fl_Widget* w, int pointerX);
// TODO: // TODO: static int spanForward(textBuffer* buf, int startPos, char* searchChars,
// TODO: // TODO:                        int ignoreSpace, int* foundPos);
// TODO: // TODO: static int spanBackward(textBuffer* buf, int startPos, char* searchChars, int
// TODO: // TODO:                         ignoreSpace, int* foundPos);
// TODO: // TODO: static void selectLine(Fl_Widget* w);
// TODO: // TODO: static int startOfWord(TextWidget w, int pos);
// TODO: // TODO: static int endOfWord(TextWidget w, int pos);
// TODO: // TODO: static void checkAutoScroll(TextWidget w, int x, int y);
// TODO: // TODO: static void endDrag(Fl_Widget* w);
// TODO: // TODO: static void cancelDrag(Fl_Widget* w);
// TODO: // TODO: static void callCursorMovementCBs(Fl_Widget* w, int event);
// TODO: // TODO: static void adjustSelection(TextWidget tw, int x, int y);
// TODO: // TODO: static void adjustSecondarySelection(TextWidget tw, int x, int y);
// TODO: // TODO: static void autoScrollTimerProc(XtPointer clientData, XtIntervalId* id);
// TODO: // TODO: static char* wrapText(TextWidget tw, char* startLine, char* text, int bufOffset,
// TODO: // TODO:                       int wrapMargin, int* breakBefore);
// TODO: // TODO: static int wrapLine(TextWidget tw, textBuffer* buf, int bufOffset,
// TODO: // TODO:                     int lineStartPos, int lineEndPos, int limitPos, int* breakAt,
// TODO: // TODO:                     int* charsAdded);
// TODO: // TODO: static char* createIndentString(TextWidget tw, textBuffer* buf, int bufOffset,
// TODO: // TODO:                                 int lineStartPos, int lineEndPos, int* length, int* column);
// TODO: // TODO: static void cursorBlinkTimerProc(XtPointer clientData, XtIntervalId* id);
// TODO: // TODO: static int hasKey(const char* key, const const char** args, const int* nArgs);
// TODO: // TODO: static int max(int i1, int i2);
// TODO: // TODO: static int min(int i1, int i2);
// TODO: // TODO: static int strCaseCmp(const char* str1, const char* str2);
// TODO: // TODO: static void ringIfNecessary(bool silent, Fl_Widget* w);
// TODO: // TODO: 
// TODO: // TODO: static char defaultTranslations[] =
// TODO: // TODO:    /* Home */
// TODO: // TODO:    "~Shift ~Ctrl Alt<Key>osfBeginLine: last_document()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* Backspace */
// TODO: // TODO:    "Ctrl<KeyPress>osfBackSpace: delete_previous_word()\n"
// TODO: // TODO:    "<KeyPress>osfBackSpace: delete_previous_character()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* Delete */
// TODO: // TODO:    "Alt Shift Ctrl<KeyPress>osfDelete: cut_primary(\"rect\")\n"
// TODO: // TODO:    "Meta Shift Ctrl<KeyPress>osfDelete: cut_primary(\"rect\")\n"
// TODO: // TODO:    "Shift Ctrl<KeyPress>osfDelete: cut_primary()\n"
// TODO: // TODO:    "Ctrl<KeyPress>osfDelete: delete_to_end_of_line()\n"
// TODO: // TODO:    "Shift<KeyPress>osfDelete: cut_clipboard()\n"
// TODO: // TODO:    "<KeyPress>osfDelete: delete_next_character()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* Insert */
// TODO: // TODO:    "Alt Shift Ctrl<KeyPress>osfInsert: copy_primary(\"rect\")\n"
// TODO: // TODO:    "Meta Shift Ctrl<KeyPress>osfInsert: copy_primary(\"rect\")\n"
// TODO: // TODO:    "Shift Ctrl<KeyPress>osfInsert: copy_primary()\n"
// TODO: // TODO:    "Shift<KeyPress>osfInsert: paste_clipboard()\n"
// TODO: // TODO:    "Ctrl<KeyPress>osfInsert: copy_clipboard()\n"
// TODO: // TODO:    "~Shift ~Ctrl<KeyPress>osfInsert: set_overtype_mode()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* Cut/Copy/Paste */
// TODO: // TODO:    "Shift Ctrl<KeyPress>osfCut: cut_primary()\n"
// TODO: // TODO:    "<KeyPress>osfCut: cut_clipboard()\n"
// TODO: // TODO:    "<KeyPress>osfCopy: copy_clipboard()\n"
// TODO: // TODO:    "<KeyPress>osfPaste: paste_clipboard()\n"
// TODO: // TODO:    "<KeyPress>osfPrimaryPaste: copy_primary()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* BeginLine */
// TODO: // TODO:    "Alt Shift Ctrl<KeyPress>osfBeginLine: beginning_of_file(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift Ctrl<KeyPress>osfBeginLine: beginning_of_file(\"extend\" \"rect\")\n"
// TODO: // TODO:    "Alt Shift<KeyPress>osfBeginLine: beginning_of_line(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift<KeyPress>osfBeginLine: beginning_of_line(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Shift Ctrl<KeyPress>osfBeginLine: beginning_of_file(\"extend\")\n"
// TODO: // TODO:    "Ctrl<KeyPress>osfBeginLine: beginning_of_file()\n"
// TODO: // TODO:    "Shift<KeyPress>osfBeginLine: beginning_of_line(\"extend\")\n"
// TODO: // TODO:    "~Alt~Shift~Ctrl~Meta<KeyPress>osfBeginLine: beginning_of_line()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* EndLine */
// TODO: // TODO:    "Alt Shift Ctrl<KeyPress>osfEndLine: end_of_file(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift Ctrl<KeyPress>osfEndLine: end_of_file(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Alt Shift<KeyPress>osfEndLine: end_of_line(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift<KeyPress>osfEndLine: end_of_line(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Shift Ctrl<KeyPress>osfEndLine: end_of_file(\"extend\")\n"
// TODO: // TODO:    "Ctrl<KeyPress>osfEndLine: end_of_file()\n"
// TODO: // TODO:    "Shift<KeyPress>osfEndLine: end_of_line(\"extend\")\n"
// TODO: // TODO:    "~Alt~Shift~Ctrl~Meta<KeyPress>osfEndLine: end_of_line()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* Left */
// TODO: // TODO:    "Alt Shift Ctrl<KeyPress>osfLeft: backward_word(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift Ctrl<KeyPress>osfLeft: backward_word(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Alt Shift<KeyPress>osfLeft: key_select(\"left\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift<KeyPress>osfLeft: key_select(\"left\", \"rect\")\n"
// TODO: // TODO:    "Shift Ctrl<KeyPress>osfLeft: backward_word(\"extend\")\n"
// TODO: // TODO:    "Ctrl<KeyPress>osfLeft: backward_word()\n"
// TODO: // TODO:    "Shift<KeyPress>osfLeft: key_select(\"left\")\n"
// TODO: // TODO:    "~Alt~Shift~Ctrl~Meta<KeyPress>osfLeft: backward_character()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* Right */
// TODO: // TODO:    "Alt Shift Ctrl<KeyPress>osfRight: forward_word(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift Ctrl<KeyPress>osfRight: forward_word(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Alt Shift<KeyPress>osfRight: key_select(\"right\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift<KeyPress>osfRight: key_select(\"right\", \"rect\")\n"
// TODO: // TODO:    "Shift Ctrl<KeyPress>osfRight: forward_word(\"extend\")\n"
// TODO: // TODO:    "Ctrl<KeyPress>osfRight: forward_word()\n"
// TODO: // TODO:    "Shift<KeyPress>osfRight: key_select(\"right\")\n"
// TODO: // TODO:    "~Alt~Shift~Ctrl~Meta<KeyPress>osfRight: forward_character()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* Up */
// TODO: // TODO:    "Alt Shift Ctrl<KeyPress>osfUp: backward_paragraph(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift Ctrl<KeyPress>osfUp: backward_paragraph(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Alt Shift<KeyPress>osfUp: process_shift_up(\"rect\")\n"
// TODO: // TODO:    "Meta Shift<KeyPress>osfUp: process_shift_up(\"rect\")\n"
// TODO: // TODO:    "Shift Ctrl<KeyPress>osfUp: backward_paragraph(\"extend\")\n"
// TODO: // TODO:    "Ctrl<KeyPress>osfUp: backward_paragraph()\n"
// TODO: // TODO:    "Shift<KeyPress>osfUp: process_shift_up()\n"
// TODO: // TODO:    "~Alt~Shift~Ctrl~Meta<KeyPress>osfUp: process_up()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* Down */
// TODO: // TODO:    "Alt Shift Ctrl<KeyPress>osfDown: forward_paragraph(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift Ctrl<KeyPress>osfDown: forward_paragraph(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Alt Shift<KeyPress>osfDown: process_shift_down(\"rect\")\n"
// TODO: // TODO:    "Meta Shift<KeyPress>osfDown: process_shift_down(\"rect\")\n"
// TODO: // TODO:    "Shift Ctrl<KeyPress>osfDown: forward_paragraph(\"extend\")\n"
// TODO: // TODO:    "Ctrl<KeyPress>osfDown: forward_paragraph()\n"
// TODO: // TODO:    "Shift<KeyPress>osfDown: process_shift_down()\n"
// TODO: // TODO:    "~Alt~Shift~Ctrl~Meta<KeyPress>osfDown: process_down()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* PageUp */
// TODO: // TODO:    "Alt Shift Ctrl<KeyPress>osfPageUp: page_left(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift Ctrl<KeyPress>osfPageUp: page_left(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Alt Shift<KeyPress>osfPageUp: previous_page(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift<KeyPress>osfPageUp: previous_page(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Shift Ctrl<KeyPress>osfPageUp: page_left(\"extend\")\n"
// TODO: // TODO:    "Ctrl<KeyPress>osfPageUp: previous_document()\n"
// TODO: // TODO:    "Shift<KeyPress>osfPageUp: previous_page(\"extend\")\n"
// TODO: // TODO:    "~Alt ~Shift ~Ctrl ~Meta<KeyPress>osfPageUp: previous_page()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* PageDown */
// TODO: // TODO:    "Alt Shift Ctrl<KeyPress>osfPageDown: page_right(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift Ctrl<KeyPress>osfPageDown: page_right(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Alt Shift<KeyPress>osfPageDown: next_page(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift<KeyPress>osfPageDown: next_page(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Shift Ctrl<KeyPress>osfPageDown: page_right(\"extend\")\n"
// TODO: // TODO:    "Ctrl<KeyPress>osfPageDown: next_document()\n"
// TODO: // TODO:    "Shift<KeyPress>osfPageDown: next_page(\"extend\")\n"
// TODO: // TODO:    "~Alt ~Shift ~Ctrl ~Meta<KeyPress>osfPageDown: next_page()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* PageLeft and PageRight are placed later than the PageUp/PageDown
// TODO: // TODO:       bindings.  Some systems map osfPageLeft to Ctrl-PageUp.
// TODO: // TODO:       Overloading this single key gives problems, and we want to give
// TODO: // TODO:       priority to the normal version. */
// TODO: // TODO: 
// TODO: // TODO:    /* PageLeft */
// TODO: // TODO:    "Alt Shift<KeyPress>osfPageLeft: page_left(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift<KeyPress>osfPageLeft: page_left(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Shift<KeyPress>osfPageLeft: page_left(\"extend\")\n"
// TODO: // TODO:    "~Alt ~Shift ~Ctrl ~Meta<KeyPress>osfPageLeft: page_left()\n"
// TODO: // TODO: 
// TODO: // TODO:    /* PageRight */
// TODO: // TODO:    "Alt Shift<KeyPress>osfPageRight: page_right(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Meta Shift<KeyPress>osfPageRight: page_right(\"extend\", \"rect\")\n"
// TODO: // TODO:    "Shift<KeyPress>osfPageRight: page_right(\"extend\")\n"
// TODO: // TODO:    "~Alt ~Shift ~Ctrl ~Meta<KeyPress>osfPageRight: page_right()\n"
// TODO: // TODO: 
// TODO: // TODO:    "Shift<KeyPress>osfSelect: key_select()\n"
// TODO: // TODO:    "<KeyPress>osfCancel: process_cancel()\n"
// TODO: // TODO:    "Ctrl~Alt~Meta<KeyPress>v: paste_clipboard()\n"
// TODO: // TODO:    "Ctrl~Alt~Meta<KeyPress>c: copy_clipboard()\n"
// TODO: // TODO:    "Ctrl~Alt~Meta<KeyPress>x: cut_clipboard()\n"
// TODO: // TODO:    "Ctrl~Alt~Meta<KeyPress>u: delete_to_start_of_line()\n"
// TODO: // TODO:    "Ctrl<KeyPress>Return: newline_and_indent()\n"
// TODO: // TODO:    "Shift<KeyPress>Return: newline_no_indent()\n"
// TODO: // TODO:    "<KeyPress>Return: newline()\n"
// TODO: // TODO:    /* KP_Enter = osfActivate
// TODO: // TODO:       Note: Ctrl+KP_Enter is already bound to Execute Command Line... */
// TODO: // TODO:    "Shift<KeyPress>osfActivate: newline_no_indent()\n"
// TODO: // TODO:    "<KeyPress>osfActivate: newline()\n"
// TODO: // TODO:    "Ctrl<KeyPress>Tab: self_insert()\n"
// TODO: // TODO:    "<KeyPress>Tab: process_tab()\n"
// TODO: // TODO:    "Alt Shift Ctrl<KeyPress>space: key_select(\"rect\")\n"
// TODO: // TODO:    "Meta Shift Ctrl<KeyPress>space: key_select(\"rect\")\n"
// TODO: // TODO:    "Shift Ctrl~Meta~Alt<KeyPress>space: key_select()\n"
// TODO: // TODO:    "Ctrl~Meta~Alt<KeyPress>slash: select_all()\n"
// TODO: // TODO:    "Ctrl~Meta~Alt<KeyPress>backslash: deselect_all()\n"
// TODO: // TODO:    "<KeyPress>: self_insert()\n"
// TODO: // TODO:    "Alt Ctrl<Btn1Down>: move_destination()\n"
// TODO: // TODO:    "Meta Ctrl<Btn1Down>: move_destination()\n"
// TODO: // TODO:    "Shift Ctrl<Btn1Down>: extend_start(\"rect\")\n"
// TODO: // TODO:    "Shift<Btn1Down>: extend_start()\n"
// TODO: // TODO:    "<Btn1Down>: grab_focus()\n"
// TODO: // TODO:    "Button1 Ctrl<MotionNotify>: extend_adjust(\"rect\")\n"
// TODO: // TODO:    "Button1~Ctrl<MotionNotify>: extend_adjust()\n"
// TODO: // TODO:    "<Btn1Up>: extend_end()\n"
// TODO: // TODO:    "<Btn2Down>: secondary_or_drag_start()\n"
// TODO: // TODO:    "Shift Ctrl Button2<MotionNotify>: secondary_or_drag_adjust(\"rect\", \"copy\", \"overlay\")\n"
// TODO: // TODO:    "Shift Button2<MotionNotify>: secondary_or_drag_adjust(\"copy\")\n"
// TODO: // TODO:    "Ctrl Button2<MotionNotify>: secondary_or_drag_adjust(\"rect\", \"overlay\")\n"
// TODO: // TODO:    "Button2<MotionNotify>: secondary_or_drag_adjust()\n"
// TODO: // TODO:    "Shift Ctrl<Btn2Up>: move_to_or_end_drag(\"copy\", \"overlay\")\n"
// TODO: // TODO:    "Shift <Btn2Up>: move_to_or_end_drag(\"copy\")\n"
// TODO: // TODO:    "Alt<Btn2Up>: exchange()\n"
// TODO: // TODO:    "Meta<Btn2Up>: exchange()\n"
// TODO: // TODO:    "Ctrl<Btn2Up>: copy_to_or_end_drag(\"overlay\")\n"
// TODO: // TODO:    "<Btn2Up>: copy_to_or_end_drag()\n"
// TODO: // TODO:    "Ctrl~Meta~Alt<Btn3Down>: mouse_pan()\n"
// TODO: // TODO:    "Ctrl~Meta~Alt Button3<MotionNotify>: mouse_pan()\n"
// TODO: // TODO:    "<Btn3Up>: end_drag()\n"
// TODO: // TODO:    "<FocusIn>: focusIn()\n"
// TODO: // TODO:    "<FocusOut>: focusOut()\n"
// TODO: // TODO:    /* Support for mouse wheel in XFree86 */
// TODO: // TODO:    "Shift<Btn4Down>,<Btn4Up>: scroll_up(1)\n"
// TODO: // TODO:    "Shift<Btn5Down>,<Btn5Up>: scroll_down(1)\n"
// TODO: // TODO:    "Ctrl<Btn4Down>,<Btn4Up>: scroll_up(1, pages)\n"
// TODO: // TODO:    "Ctrl<Btn5Down>,<Btn5Up>: scroll_down(1, pages)\n"
// TODO: // TODO:    "<Btn4Down>,<Btn4Up>: scroll_up(5)\n"
// TODO: // TODO:    "<Btn5Down>,<Btn5Up>: scroll_down(5)\n";
// TODO: // TODO: /* some of the translations from the Motif text widget were not picked up:
// TODO: // TODO: :<KeyPress>osfSelect: set-anchor()\n\
// TODO: // TODO: :<KeyPress>osfActivate: activate()\n\
// TODO: // TODO: ~Shift Ctrl~Meta~Alt<KeyPress>Return: activate()\n\
// TODO: // TODO: ~Shift Ctrl~Meta~Alt<KeyPress>space: set-anchor()\n\
// TODO: // TODO:  :<KeyPress>osfClear: clear-selection()\n\
// TODO: // TODO: ~Shift~Ctrl~Meta~Alt<KeyPress>Return: process-return()\n\
// TODO: // TODO: Shift~Meta~Alt<KeyPress>Tab: prev-tab-group()\n\
// TODO: // TODO: Ctrl~Meta~Alt<KeyPress>Tab: next-tab-group()\n\
// TODO: // TODO: <UnmapNotify>: unmap()\n\
// TODO: // TODO: <EnterNotify>: enter()\n\
// TODO: // TODO: <LeaveNotify>: leave()\n
// TODO: // TODO: */
// TODO: // TODO: 
// TODO: // TODO: 
// TODO: // TODO: static XtActionsRec actionsList[] =
// TODO: // TODO: {
// TODO: // TODO:    {"self-insert", selfInsertAP},
// TODO: // TODO:    {"self_insert", selfInsertAP},
// TODO: // TODO:    {"grab-focus", grabFocusAP},
// TODO: // TODO:    {"grab_focus", grabFocusAP},
// TODO: // TODO:    {"extend-adjust", extendAdjustAP},
// TODO: // TODO:    {"extend_adjust", extendAdjustAP},
// TODO: // TODO:    {"extend-start", extendStartAP},
// TODO: // TODO:    {"extend_start", extendStartAP},
// TODO: // TODO:    {"extend-end", extendEndAP},
// TODO: // TODO:    {"extend_end", extendEndAP},
// TODO: // TODO:    {"secondary-adjust", secondaryAdjustAP},
// TODO: // TODO:    {"secondary_adjust", secondaryAdjustAP},
// TODO: // TODO:    {"secondary-or-drag-adjust", secondaryOrDragAdjustAP},
// TODO: // TODO:    {"secondary_or_drag_adjust", secondaryOrDragAdjustAP},
// TODO: // TODO:    {"secondary-start", secondaryStartAP},
// TODO: // TODO:    {"secondary_start", secondaryStartAP},
// TODO: // TODO:    {"secondary-or-drag-start", secondaryOrDragStartAP},
// TODO: // TODO:    {"secondary_or_drag_start", secondaryOrDragStartAP},
// TODO: // TODO:    {"process-bdrag", secondaryOrDragStartAP},
// TODO: // TODO:    {"process_bdrag", secondaryOrDragStartAP},
// TODO: // TODO:    {"move-destination", moveDestinationAP},
// TODO: // TODO:    {"move_destination", moveDestinationAP},
// TODO: // TODO:    {"move-to", moveToAP},
// TODO: // TODO:    {"move_to", moveToAP},
// TODO: // TODO:    {"move-to-or-end-drag", moveToOrEndDragAP},
// TODO: // TODO:    {"move_to_or_end_drag", moveToOrEndDragAP},
// TODO: // TODO:    {"end_drag", endDragAP},
// TODO: // TODO:    {"copy-to", copyToAP},
// TODO: // TODO:    {"copy_to", copyToAP},
// TODO: // TODO:    {"copy-to-or-end-drag", copyToOrEndDragAP},
// TODO: // TODO:    {"copy_to_or_end_drag", copyToOrEndDragAP},
// TODO: // TODO:    {"exchange", exchangeAP},
// TODO: // TODO:    {"process-cancel", processCancelAP},
// TODO: // TODO:    {"process_cancel", processCancelAP},
// TODO: // TODO:    {"paste-clipboard", pasteClipboardAP},
// TODO: // TODO:    {"paste_clipboard", pasteClipboardAP},
// TODO: // TODO:    {"copy-clipboard", copyClipboardAP},
// TODO: // TODO:    {"copy_clipboard", copyClipboardAP},
// TODO: // TODO:    {"cut-clipboard", cutClipboardAP},
// TODO: // TODO:    {"cut_clipboard", cutClipboardAP},
// TODO: // TODO:    {"copy-primary", copyPrimaryAP},
// TODO: // TODO:    {"copy_primary", copyPrimaryAP},
// TODO: // TODO:    {"cut-primary", cutPrimaryAP},
// TODO: // TODO:    {"cut_primary", cutPrimaryAP},
// TODO: // TODO:    {"newline", newlineAP},
// TODO: // TODO:    {"newline-and-indent", newlineAndIndentAP},
// TODO: // TODO:    {"newline_and_indent", newlineAndIndentAP},
// TODO: // TODO:    {"newline-no-indent", newlineNoIndentAP},
// TODO: // TODO:    {"newline_no_indent", newlineNoIndentAP},
// TODO: // TODO:    {"delete-selection", deleteSelectionAP},
// TODO: // TODO:    {"delete_selection", deleteSelectionAP},
// TODO: // TODO:    {"delete-previous-character", deletePreviousCharacterAP},
// TODO: // TODO:    {"delete_previous_character", deletePreviousCharacterAP},
// TODO: // TODO:    {"delete-next-character", deleteNextCharacterAP},
// TODO: // TODO:    {"delete_next_character", deleteNextCharacterAP},
// TODO: // TODO:    {"delete-previous-word", deletePreviousWordAP},
// TODO: // TODO:    {"delete_previous_word", deletePreviousWordAP},
// TODO: // TODO:    {"delete-next-word", deleteNextWordAP},
// TODO: // TODO:    {"delete_next_word", deleteNextWordAP},
// TODO: // TODO:    {"delete-to-start-of-line", deleteToStartOfLineAP},
// TODO: // TODO:    {"delete_to_start_of_line", deleteToStartOfLineAP},
// TODO: // TODO:    {"delete-to-end-of-line", deleteToEndOfLineAP},
// TODO: // TODO:    {"delete_to_end_of_line", deleteToEndOfLineAP},
// TODO: // TODO:    {"forward-character", forwardCharacterAP},
// TODO: // TODO:    {"forward_character", forwardCharacterAP},
// TODO: // TODO:    {"backward-character", backwardCharacterAP},
// TODO: // TODO:    {"backward_character", backwardCharacterAP},
// TODO: // TODO:    {"key-select", keySelectAP},
// TODO: // TODO:    {"key_select", keySelectAP},
// TODO: // TODO:    {"process-up", processUpAP},
// TODO: // TODO:    {"process_up", processUpAP},
// TODO: // TODO:    {"process-down", processDownAP},
// TODO: // TODO:    {"process_down", processDownAP},
// TODO: // TODO:    {"process-shift-up", processShiftUpAP},
// TODO: // TODO:    {"process_shift_up", processShiftUpAP},
// TODO: // TODO:    {"process-shift-down", processShiftDownAP},
// TODO: // TODO:    {"process_shift_down", processShiftDownAP},
// TODO: // TODO:    {"process-home", beginningOfLineAP},
// TODO: // TODO:    {"process_home", beginningOfLineAP},
// TODO: // TODO:    {"forward-word", forwardWordAP},
// TODO: // TODO:    {"forward_word", forwardWordAP},
// TODO: // TODO:    {"backward-word", backwardWordAP},
// TODO: // TODO:    {"backward_word", backwardWordAP},
// TODO: // TODO:    {"forward-paragraph", forwardParagraphAP},
// TODO: // TODO:    {"forward_paragraph", forwardParagraphAP},
// TODO: // TODO:    {"backward-paragraph", backwardParagraphAP},
// TODO: // TODO:    {"backward_paragraph", backwardParagraphAP},
// TODO: // TODO:    {"beginning-of-line", beginningOfLineAP},
// TODO: // TODO:    {"beginning_of_line", beginningOfLineAP},
// TODO: // TODO:    {"end-of-line", endOfLineAP},
// TODO: // TODO:    {"end_of_line", endOfLineAP},
// TODO: // TODO:    {"beginning-of-file", beginningOfFileAP},
// TODO: // TODO:    {"beginning_of_file", beginningOfFileAP},
// TODO: // TODO:    {"end-of-file", endOfFileAP},
// TODO: // TODO:    {"end_of_file", endOfFileAP},
// TODO: // TODO:    {"next-page", nextPageAP},
// TODO: // TODO:    {"next_page", nextPageAP},
// TODO: // TODO:    {"previous-page", previousPageAP},
// TODO: // TODO:    {"previous_page", previousPageAP},
// TODO: // TODO:    {"page-left", pageLeftAP},
// TODO: // TODO:    {"page_left", pageLeftAP},
// TODO: // TODO:    {"page-right", pageRightAP},
// TODO: // TODO:    {"page_right", pageRightAP},
// TODO: // TODO:    {"toggle-overstrike", toggleOverstrikeAP},
// TODO: // TODO:    {"toggle_overstrike", toggleOverstrikeAP},
// TODO: // TODO:    {"scroll-up", scrollUpAP},
// TODO: // TODO:    {"scroll_up", scrollUpAP},
// TODO: // TODO:    {"scroll-down", scrollDownAP},
// TODO: // TODO:    {"scroll_down", scrollDownAP},
// TODO: // TODO:    {"scroll_left", scrollLeftAP},
// TODO: // TODO:    {"scroll_right", scrollRightAP},
// TODO: // TODO:    {"scroll-to-line", scrollToLineAP},
// TODO: // TODO:    {"scroll_to_line", scrollToLineAP},
// TODO: // TODO:    {"select-all", selectAllAP},
// TODO: // TODO:    {"select_all", selectAllAP},
// TODO: // TODO:    {"deselect-all", deselectAllAP},
// TODO: // TODO:    {"deselect_all", deselectAllAP},
// TODO: // TODO:    {"focusIn", focusInAP},
// TODO: // TODO:    {"focusOut", focusOutAP},
// TODO: // TODO:    {"process-return", selfInsertAP},
// TODO: // TODO:    {"process_return", selfInsertAP},
// TODO: // TODO:    {"process-tab", processTabAP},
// TODO: // TODO:    {"process_tab", processTabAP},
// TODO: // TODO:    {"insert-string", insertStringAP},
// TODO: // TODO:    {"insert_string", insertStringAP},
// TODO: // TODO:    {"mouse_pan", mousePanAP},
// TODO: // TODO: };
// TODO: // TODO: 
// TODO: // TODO: /* The motif text widget defined a bunch of actions which the nedit text
// TODO: // TODO:    widget as-of-yet does not support:
// TODO: // TODO: 
// TODO: // TODO:      Actions which were not bound to keys (for emacs emulation, some of
// TODO: // TODO:      them should probably be supported:
// TODO: // TODO: 
// TODO: // TODO: 	kill-next-character()
// TODO: // TODO: 	kill-next-word()
// TODO: // TODO: 	kill-previous-character()
// TODO: // TODO: 	kill-previous-word()
// TODO: // TODO: 	kill-selection()
// TODO: // TODO: 	kill-to-end-of-line()
// TODO: // TODO: 	kill-to-start-of-line()
// TODO: // TODO: 	unkill()
// TODO: // TODO: 	next-line()
// TODO: // TODO: 	newline-and-backup()
// TODO: // TODO: 	beep()
// TODO: // TODO: 	redraw-display()
// TODO: // TODO: 	scroll-one-line-down()
// TODO: // TODO: 	scroll-one-line-up()
// TODO: // TODO: 	set-insertion-point()
// TODO: // TODO: 
// TODO: // TODO:     Actions which are not particularly useful:
// TODO: // TODO: 
// TODO: // TODO: 	set-anchor()
// TODO: // TODO: 	activate()
// TODO: // TODO: 	clear-selection() -> this is a wierd one
// TODO: // TODO: 	do-quick-action() -> don't think this ever worked
// TODO: // TODO: 	Help()
// TODO: // TODO: 	next-tab-group()
// TODO: // TODO: 	select-adjust()
// TODO: // TODO: 	select-start()
// TODO: // TODO: 	select-end()
// TODO: // TODO: */
// TODO: // TODO: 
// TODO: // TODO: static XtResource resources[] =
// TODO: // TODO: {
// TODO: // TODO:    {
// TODO: // TODO:       XmNhighlightThickness, XmCHighlightThickness, XmRDimension,
// TODO: // TODO:       sizeof(Dimension), XtOffset(TextWidget, primitive.highlight_thickness),
// TODO: // TODO:       XmRInt, 0
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       XmNshadowThickness, XmCShadowThickness, XmRDimension, sizeof(Dimension),
// TODO: // TODO:       XtOffset(TextWidget, primitive.shadow_thickness), XmRInt, 0
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNfont, textCFont, XmRFontStruct, sizeof(XFontStruct*),
// TODO: // TODO:       XtOffset(TextWidget, text.fontStruct), XmRString, "fixed"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNselectForeground, textCSelectForeground, XmRPixel, sizeof(Pixel),
// TODO: // TODO:       XtOffset(TextWidget, text.selectFGPixel), XmRString,
// TODO: // TODO:       NEDIT_DEFAULT_SEL_FG
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNselectBackground, textCSelectBackground, XmRPixel, sizeof(Pixel),
// TODO: // TODO:       XtOffset(TextWidget, text.selectBGPixel), XmRString,
// TODO: // TODO:       NEDIT_DEFAULT_SEL_BG
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNhighlightForeground, textCHighlightForeground, XmRPixel,sizeof(Pixel),
// TODO: // TODO:       XtOffset(TextWidget, text.highlightFGPixel), XmRString,
// TODO: // TODO:       NEDIT_DEFAULT_HI_FG
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNhighlightBackground, textCHighlightBackground, XmRPixel,sizeof(Pixel),
// TODO: // TODO:       XtOffset(TextWidget, text.highlightBGPixel), XmRString,
// TODO: // TODO:       NEDIT_DEFAULT_HI_BG
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNlineNumForeground, textCLineNumForeground, XmRPixel,sizeof(Pixel),
// TODO: // TODO:       XtOffset(TextWidget, text.lineNumFGPixel), XmRString,
// TODO: // TODO:       NEDIT_DEFAULT_LINENO_FG
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNcursorForeground, textCCursorForeground, XmRPixel,sizeof(Pixel),
// TODO: // TODO:       XtOffset(TextWidget, text.cursorFGPixel), XmRString,
// TODO: // TODO:       NEDIT_DEFAULT_CURSOR_FG
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNcalltipForeground, textCcalltipForeground, XmRPixel,sizeof(Pixel),
// TODO: // TODO:       XtOffset(TextWidget, text.calltipFGPixel), XmRString,
// TODO: // TODO:       NEDIT_DEFAULT_CALLTIP_FG
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNcalltipBackground, textCcalltipBackground, XmRPixel,sizeof(Pixel),
// TODO: // TODO:       XtOffset(TextWidget, text.calltipBGPixel), XmRString,
// TODO: // TODO:       NEDIT_DEFAULT_CALLTIP_BG
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNbacklightCharTypes,textCBacklightCharTypes,XmRString,sizeof(NeString),
// TODO: // TODO:       XtOffset(TextWidget, text.backlightCharTypes), XmRString, NULL
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNrows, textCRows, XmRInt,sizeof(int),
// TODO: // TODO:       XtOffset(TextWidget, text.rows), XmRString, "24"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNcolumns, textCColumns, XmRInt, sizeof(int),
// TODO: // TODO:       XtOffset(TextWidget, text.columns), XmRString, "80"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNmarginWidth, textCMarginWidth, XmRInt, sizeof(int),
// TODO: // TODO:       XtOffset(TextWidget, text.marginWidth), XmRString, "5"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNmarginHeight, textCMarginHeight, XmRInt, sizeof(int),
// TODO: // TODO:       XtOffset(TextWidget, text.marginHeight), XmRString, "5"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNpendingDelete, textCPendingDelete, XmRBoolean, sizeof(bool),
// TODO: // TODO:       XtOffset(TextWidget, text.pendingDelete), XmRString, "true"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNautoWrap, textCAutoWrap, XmRBoolean, sizeof(bool),
// TODO: // TODO:       XtOffset(TextWidget, text.autoWrap), XmRString, "true"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNcontinuousWrap, textCContinuousWrap, XmRBoolean, sizeof(bool),
// TODO: // TODO:       XtOffset(TextWidget, text.continuousWrap), XmRString, "true"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNautoIndent, textCAutoIndent, XmRBoolean, sizeof(bool),
// TODO: // TODO:       XtOffset(TextWidget, text.autoIndent), XmRString, "true"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNsmartIndent, textCSmartIndent, XmRBoolean, sizeof(bool),
// TODO: // TODO:       XtOffset(TextWidget, text.smartIndent), XmRString, "false"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNoverstrike, textCOverstrike, XmRBoolean, sizeof(bool),
// TODO: // TODO:       XtOffset(TextWidget, text.overstrike), XmRString, "false"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNheavyCursor, textCHeavyCursor, XmRBoolean, sizeof(bool),
// TODO: // TODO:       XtOffset(TextWidget, text.heavyCursor), XmRString, "false"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNreadOnly, textCReadOnly, XmRBoolean, sizeof(bool),
// TODO: // TODO:       XtOffset(TextWidget, text.readOnly), XmRString, "false"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNhidePointer, textCHidePointer, XmRBoolean, sizeof(bool),
// TODO: // TODO:       XtOffset(TextWidget, text.hidePointer), XmRString, "false"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNwrapMargin, textCWrapMargin, XmRInt, sizeof(int),
// TODO: // TODO:       XtOffset(TextWidget, text.wrapMargin), XmRString, "0"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNhScrollBar, textCHScrollBar, XmRWidget, sizeof(Fl_Widget*),
// TODO: // TODO:       XtOffset(TextWidget, text.hScrollBar), XmRString, ""
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNvScrollBar, textCVScrollBar, XmRWidget, sizeof(Fl_Widget*),
// TODO: // TODO:       XtOffset(TextWidget, text.vScrollBar), XmRString, ""
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNlineNumCols, textCLineNumCols, XmRInt, sizeof(int),
// TODO: // TODO:       XtOffset(TextWidget, text.lineNumCols), XmRString, "0"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNautoShowInsertPos, textCAutoShowInsertPos, XmRBoolean,
// TODO: // TODO:       sizeof(bool), XtOffset(TextWidget, text.autoShowInsertPos),
// TODO: // TODO:       XmRString, "true"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNautoWrapPastedText, textCAutoWrapPastedText, XmRBoolean,
// TODO: // TODO:       sizeof(bool), XtOffset(TextWidget, text.autoWrapPastedText),
// TODO: // TODO:       XmRString, "false"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNwordDelimiters, textCWordDelimiters, XmRString, sizeof(char*),
// TODO: // TODO:       XtOffset(TextWidget, text.delimiters), XmRString,
// TODO: // TODO:       ".,/\\`'!@#%^&*()-=+{}[]\":;<>?"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNblinkRate, textCBlinkRate, XmRInt, sizeof(int),
// TODO: // TODO:       XtOffset(TextWidget, text.cursorBlinkRate), XmRString, "500"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNemulateTabs, textCEmulateTabs, XmRInt, sizeof(int),
// TODO: // TODO:       XtOffset(TextWidget, text.emulateTabs), XmRString, "0"
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNfocusCallback, textCFocusCallback, XmRCallback, sizeof(caddr_t),
// TODO: // TODO:       XtOffset(TextWidget, text.focusInCB), XtRCallback, NULL
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNlosingFocusCallback, textCLosingFocusCallback, XmRCallback,
// TODO: // TODO:       sizeof(caddr_t), XtOffset(TextWidget, text.focusOutCB), XtRCallback,NULL
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNcursorMovementCallback, textCCursorMovementCallback, XmRCallback,
// TODO: // TODO:       sizeof(caddr_t), XtOffset(TextWidget, text.cursorCB), XtRCallback, NULL
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNdragStartCallback, textCDragStartCallback, XmRCallback,
// TODO: // TODO:       sizeof(caddr_t), XtOffset(TextWidget, text.dragStartCB), XtRCallback,
// TODO: // TODO:       NULL
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNdragEndCallback, textCDragEndCallback, XmRCallback,
// TODO: // TODO:       sizeof(caddr_t), XtOffset(TextWidget, text.dragEndCB), XtRCallback, NULL
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNsmartIndentCallback, textCSmartIndentCallback, XmRCallback,
// TODO: // TODO:       sizeof(caddr_t), XtOffset(TextWidget, text.smartIndentCB), XtRCallback,
// TODO: // TODO:       NULL
// TODO: // TODO:    },
// TODO: // TODO:    {
// TODO: // TODO:       textNcursorVPadding, textCCursorVPadding, XtRCardinal, sizeof(int),
// TODO: // TODO:       XtOffset(TextWidget, text.cursorVPadding), XmRString, "0"
// TODO: // TODO:    }
// TODO: // TODO: };
// TODO: // TODO: 
// TODO: // TODO: static TextClassRec textClassRec =
// TODO: // TODO: {
// TODO: // TODO:    /* CoreClassPart */
// TODO: // TODO:    {
// TODO: // TODO:       (WidgetClass)& xmPrimitiveClassRec,  /* superclass       */
// TODO: // TODO:       "Text",                         /* class_name            */
// TODO: // TODO:       sizeof(TextRec),                /* widget_size           */
// TODO: // TODO:       NULL,                           /* class_initialize      */
// TODO: // TODO:       NULL,                           /* class_part_initialize */
// TODO: // TODO:       FALSE,                          /* class_inited          */
// TODO: // TODO:       (XtInitProc)initialize,         /* initialize            */
// TODO: // TODO:       NULL,                           /* initialize_hook       */
// TODO: // TODO:       realize,   		            /* realize               */
// TODO: // TODO:       actionsList,                    /* actions               */
// TODO: // TODO:       XtNumber(actionsList),          /* num_actions           */
// TODO: // TODO:       resources,                      /* resources             */
// TODO: // TODO:       XtNumber(resources),            /* num_resources         */
// TODO: // TODO:       NULLQUARK,                      /* xrm_class             */
// TODO: // TODO:       TRUE,                           /* compress_motion       */
// TODO: // TODO:       TRUE,                           /* compress_exposure     */
// TODO: // TODO:       TRUE,                           /* compress_enterleave   */
// TODO: // TODO:       FALSE,                          /* visible_interest      */
// TODO: // TODO:       (XtWidgetProc)destroy,          /* destroy               */
// TODO: // TODO:       (XtWidgetProc)resize,           /* resize                */
// TODO: // TODO:       (XtExposeProc)redisplay,        /* expose                */
// TODO: // TODO:       (XtSetValuesFunc)setValues,     /* set_values            */
// TODO: // TODO:       NULL,                           /* set_values_hook       */
// TODO: // TODO:       XtInheritSetValuesAlmost,       /* set_values_almost     */
// TODO: // TODO:       NULL,                           /* get_values_hook       */
// TODO: // TODO:       NULL,                           /* accept_focus          */
// TODO: // TODO:       XtVersion,                      /* version               */
// TODO: // TODO:       NULL,                           /* callback private      */
// TODO: // TODO:       defaultTranslations,            /* tm_table              */
// TODO: // TODO:       queryGeometry,                  /* query_geometry        */
// TODO: // TODO:       NULL,                           /* display_accelerator   */
// TODO: // TODO:       NULL,                           /* extension             */
// TODO: // TODO:    },
// TODO: // TODO:    /* Motif primitive class fields */
// TODO: // TODO:    {
// TODO: // TODO:       (XtWidgetProc)_XtInherit,   	/* Primitive border_highlight   */
// TODO: // TODO:       (XtWidgetProc)_XtInherit,   	/* Primitive border_unhighlight */
// TODO: // TODO:       NULL, /*XtInheritTranslations,*/	/* translations                 */
// TODO: // TODO:       NULL,				/* arm_and_activate             */
// TODO: // TODO:       NULL,				/* get resources      		*/
// TODO: // TODO:       0,					/* num get_resources  		*/
// TODO: // TODO:       NULL,         			/* extension                    */
// TODO: // TODO:    },
// TODO: // TODO:    /* Text class part */
// TODO: // TODO:    {
// TODO: // TODO:       0,                              	/* ignored	                */
// TODO: // TODO:    }
// TODO: // TODO: };
// TODO: // TODO: 
// TODO: // TODO: WidgetClass textWidgetClass = (WidgetClass)& textClassRec;
// TODO: // TODO: #define NEDIT_HIDE_CURSOR_MASK (KeyPressMask)
// TODO: // TODO: #define NEDIT_SHOW_CURSOR_MASK (FocusChangeMask | PointerMotionMask | ButtonMotionMask | ButtonPressMask | ButtonReleaseMask)
// TODO: // TODO: static char empty_bits[] = {0x00, 0x00, 0x00, 0x00};
// TODO: // TODO: static Cursor empty_cursor = 0;
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Fl_Widget* initialize method
// TODO: // TODO: */
// TODO: // TODO: static void initialize(TextWidget request, TextWidget newWidget)
// TODO: // TODO: {
// TODO: // TODO:    XFontStruct* fs = newWidget->text.fontStruct;
// TODO: // TODO:    char* delimiters;
// TODO: // TODO:    textBuffer* buf;
// TODO: // TODO:    Pixel white, black;
// TODO: // TODO:    int textLeft;
// TODO: // TODO:    int charWidth = fs->max_bounds.width;
// TODO: // TODO:    int marginWidth = newWidget->text.marginWidth;
// TODO: // TODO:    int lineNumCols = newWidget->text.lineNumCols;
// TODO: // TODO: 
// TODO: // TODO:    /* Set the initial window size based on the rows and columns resources */
// TODO: // TODO:    if (request->core.width == 0)
// TODO: // TODO:       newWidget->core.width = charWidth * newWidget->text.columns + marginWidth*2 +
// TODO: // TODO:                               (lineNumCols == 0 ? 0 : marginWidth + charWidth * lineNumCols);
// TODO: // TODO:    if (request->core.height == 0)
// TODO: // TODO:       newWidget->core.height = (fs->ascent + fs->descent) * newWidget->text.rows +
// TODO: // TODO:                                newWidget->text.marginHeight * 2;
// TODO: // TODO: 
// TODO: // TODO:    /* The default colors work for B&W as well as color, except for
// TODO: // TODO:       selectFGPixel and selectBGPixel, where color highlighting looks
// TODO: // TODO:       much better without reverse video, so if we get here, and the
// TODO: // TODO:       selection is totally unreadable because of the bad default colors,
// TODO: // TODO:       swap the colors and make the selection reverse video */
// TODO: // TODO:    white = WhitePixelOfScreen(XtScreen((Fl_Widget*)newWidget));
// TODO: // TODO:    black = BlackPixelOfScreen(XtScreen((Fl_Widget*)newWidget));
// TODO: // TODO:    if (newWidget->text.selectBGPixel == white &&
// TODO: // TODO:          newWidget->core.background_pixel == white &&
// TODO: // TODO:          newWidget->text.selectFGPixel == black &&
// TODO: // TODO:          newWidget->primitive.foreground == black)
// TODO: // TODO:    {
// TODO: // TODO:       newWidget->text.selectBGPixel = black;
// TODO: // TODO:       newWidget->text.selectFGPixel = white;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Create the initial text buffer for the widget to display (which can
// TODO: // TODO:       be replaced later with TextSetBuffer) */
// TODO: // TODO:    buf = BufCreate();
// TODO: // TODO: 
// TODO: // TODO:    /* Create and initialize the text-display part of the widget */
// TODO: // TODO:    textLeft = newWidget->text.marginWidth +
// TODO: // TODO:               (lineNumCols == 0 ? 0 : marginWidth + charWidth * lineNumCols);
// TODO: // TODO:    newWidget->text.textD = TextDCreate((Fl_Widget*)newWidget, newWidget->text.hScrollBar,
// TODO: // TODO:                                        newWidget->text.vScrollBar, textLeft, newWidget->text.marginHeight,
// TODO: // TODO:                                        newWidget->core.width - marginWidth - textLeft,
// TODO: // TODO:                                        newWidget->core.height - newWidget->text.marginHeight * 2,
// TODO: // TODO:                                        lineNumCols == 0 ? 0 : marginWidth,
// TODO: // TODO:                                        lineNumCols == 0 ? 0 : lineNumCols * charWidth,
// TODO: // TODO:                                        buf, newWidget->text.fontStruct, newWidget->core.background_pixel,
// TODO: // TODO:                                        newWidget->primitive.foreground, newWidget->text.selectFGPixel,
// TODO: // TODO:                                        newWidget->text.selectBGPixel, newWidget->text.highlightFGPixel,
// TODO: // TODO:                                        newWidget->text.highlightBGPixel, newWidget->text.cursorFGPixel,
// TODO: // TODO:                                        newWidget->text.lineNumFGPixel,
// TODO: // TODO:                                        newWidget->text.continuousWrap, newWidget->text.wrapMargin,
// TODO: // TODO:                                        newWidget->text.backlightCharTypes, newWidget->text.calltipFGPixel,
// TODO: // TODO:                                        newWidget->text.calltipBGPixel);
// TODO: // TODO: 
// TODO: // TODO:    /* Add mandatory delimiters blank, tab, and newline to the list of
// TODO: // TODO:       delimiters.  The memory use scheme here is that new values are
// TODO: // TODO:       always copied, and can therefore be safely freed on subsequent
// TODO: // TODO:       set-values calls or destroy */
// TODO: // TODO:    delimiters = malloc__(strlen(newWidget->text.delimiters) + 4);
// TODO: // TODO:    sprintf(delimiters, "%s%s", " \t\n", newWidget->text.delimiters);
// TODO: // TODO:    newWidget->text.delimiters = delimiters;
// TODO: // TODO: 
// TODO: // TODO:    /* Start with the cursor blanked (widgets don't have focus on creation,
// TODO: // TODO:       the initial FocusIn event will unblank it and get blinking started) */
// TODO: // TODO:    newWidget->text.textD->cursorOn = false;
// TODO: // TODO: 
// TODO: // TODO:    /* Initialize the widget variables */
// TODO: // TODO:    newWidget->text.autoScrollProcID = 0;
// TODO: // TODO:    newWidget->text.cursorBlinkProcID = 0;
// TODO: // TODO:    newWidget->text.dragState = NOT_CLICKED;
// TODO: // TODO:    newWidget->text.multiClickState = NO_CLICKS;
// TODO: // TODO:    newWidget->text.lastBtnDown = 0;
// TODO: // TODO:    newWidget->text.selectionOwner = false;
// TODO: // TODO:    newWidget->text.motifDestOwner = false;
// TODO: // TODO:    newWidget->text.emTabsBeforeCursor = 0;
// TODO: // TODO: 
// TODO: // TODO: #ifndef NO_XMIM
// TODO: // TODO:    /* Register the widget to the input manager */
// TODO: // TODO:    XmImRegister((Fl_Widget*)newWidget, 0);
// TODO: // TODO:    /* In case some Resources for the IC need to be set, add them below */
// TODO: // TODO:    XmImVaSetValues((Fl_Widget*)newWidget, NULL);
// TODO: // TODO: #endif
// TODO: // TODO: 
// TODO: // TODO:    XtAddEventHandler((Fl_Widget*)newWidget, GraphicsExpose, true,
// TODO: // TODO:                      (XtEventHandler)redisplayGE, (Opaque)NULL);
// TODO: // TODO: 
// TODO: // TODO:    if (newWidget->text.hidePointer)
// TODO: // TODO:    {
// TODO: // TODO:       Display* theDisplay;
// TODO: // TODO:       Pixmap empty_pixmap;
// TODO: // TODO:       XColor black_color;
// TODO: // TODO:       /* Set up the empty Cursor */
// TODO: // TODO:       if (empty_cursor == 0)
// TODO: // TODO:       {
// TODO: // TODO:          theDisplay = XtDisplay((Fl_Widget*)newWidget);
// TODO: // TODO:          empty_pixmap = XCreateBitmapFromData(theDisplay,
// TODO: // TODO:                                               RootWindowOfScreen(XtScreen((Fl_Widget*)newWidget)), empty_bits, 1, 1);
// TODO: // TODO:          XParseColor(theDisplay, DefaultColormapOfScreen(XtScreen((Fl_Widget*)newWidget)),
// TODO: // TODO:                      "black", &black_color);
// TODO: // TODO:          empty_cursor = XCreatePixmapCursor(theDisplay, empty_pixmap,
// TODO: // TODO:                                             empty_pixmap, &black_color, &black_color, 0, 0);
// TODO: // TODO:       }
// TODO: // TODO: 
// TODO: // TODO:       /* Add event handler to hide the pointer on keypresses */
// TODO: // TODO:       XtAddEventHandler((Fl_Widget*)newWidget, NEDIT_HIDE_CURSOR_MASK, false,
// TODO: // TODO:                         handleHidePointer, (Opaque)NULL);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /* Hide the pointer while the user is typing */
// TODO: // TODO: static void handleHidePointer(Fl_Widget* w, XtPointer unused,
// TODO: // TODO:                               int event, bool* continue_to_dispatch)
// TODO: // TODO: {
// TODO: // TODO:    TextWidget tw = (TextWidget) w;
// TODO: // TODO:    ShowHidePointer(tw, true);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /* Restore the pointer if the mouse moves or focus changes */
// TODO: // TODO: static void handleShowPointer(Fl_Widget* w, XtPointer unused,
// TODO: // TODO:                               int event, bool* continue_to_dispatch)
// TODO: // TODO: {
// TODO: // TODO:    TextWidget tw = (TextWidget) w;
// TODO: // TODO:    ShowHidePointer(tw, false);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: void ShowHidePointer(TextWidget w, bool hidePointer)
// TODO: // TODO: {
// TODO: // TODO:    if (w->text.hidePointer)
// TODO: // TODO:    {
// TODO: // TODO:       if (hidePointer != w->text.textD->pointerHidden)
// TODO: // TODO:       {
// TODO: // TODO:          if (hidePointer)
// TODO: // TODO:          {
// TODO: // TODO:             /* Don't listen for keypresses any more */
// TODO: // TODO:             XtRemoveEventHandler((Fl_Widget*)w, NEDIT_HIDE_CURSOR_MASK, false,
// TODO: // TODO:                                  handleHidePointer, (Opaque)NULL);
// TODO: // TODO:             /* Switch to empty cursor */
// TODO: // TODO:             XDefineCursor(XtDisplay(w), XtWindow(w), empty_cursor);
// TODO: // TODO: 
// TODO: // TODO:             w->text.textD->pointerHidden = true;
// TODO: // TODO: 
// TODO: // TODO:             /* Listen to mouse movement, focus change, and button presses */
// TODO: // TODO:             XtAddEventHandler((Fl_Widget*)w, NEDIT_SHOW_CURSOR_MASK,
// TODO: // TODO:                               false, handleShowPointer, (Opaque)NULL);
// TODO: // TODO:          }
// TODO: // TODO:          else
// TODO: // TODO:          {
// TODO: // TODO:             /* Don't listen to mouse/focus events any more */
// TODO: // TODO:             XtRemoveEventHandler((Fl_Widget*)w, NEDIT_SHOW_CURSOR_MASK,
// TODO: // TODO:                                  false, handleShowPointer, (Opaque)NULL);
// TODO: // TODO:             /* Switch to regular cursor */
// TODO: // TODO:             XUndefineCursor(XtDisplay(w), XtWindow(w));
// TODO: // TODO: 
// TODO: // TODO:             w->text.textD->pointerHidden = false;
// TODO: // TODO: 
// TODO: // TODO:             /* Listen for keypresses now */
// TODO: // TODO:             XtAddEventHandler((Fl_Widget*)w, NEDIT_HIDE_CURSOR_MASK, false,
// TODO: // TODO:                               handleHidePointer, (Opaque)NULL);
// TODO: // TODO:          }
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Fl_Widget* destroy method
// TODO: // TODO: */
// TODO: // TODO: static void destroy(TextWidget w)
// TODO: // TODO: {
// TODO: // TODO:    textBuffer* buf;
// TODO: // TODO: 
// TODO: // TODO:    /* Free the text display and possibly the attached buffer.  The buffer
// TODO: // TODO:       is freed only if after removing all of the modify procs (by calling
// TODO: // TODO:       StopHandlingXSelections and TextDFree) there are no modify procs
// TODO: // TODO:       left */
// TODO: // TODO:    StopHandlingXSelections((Fl_Widget*)w);
// TODO: // TODO:    buf = w->text.textD->buffer;
// TODO: // TODO:    TextDFree(w->text.textD);
// TODO: // TODO:    if (buf->nModifyProcs == 0)
// TODO: // TODO:       BufFree(buf);
// TODO: // TODO: 
// TODO: // TODO:    if (w->text.cursorBlinkProcID != 0)
// TODO: // TODO:       XtRemoveTimeOut(w->text.cursorBlinkProcID);
// TODO: // TODO:    XtFree(w->text.delimiters);
// TODO: // TODO:    XtRemoveAllCallbacks((Fl_Widget*)w, textNfocusCallback);
// TODO: // TODO:    XtRemoveAllCallbacks((Fl_Widget*)w, textNlosingFocusCallback);
// TODO: // TODO:    XtRemoveAllCallbacks((Fl_Widget*)w, textNcursorMovementCallback);
// TODO: // TODO:    XtRemoveAllCallbacks((Fl_Widget*)w, textNdragStartCallback);
// TODO: // TODO:    XtRemoveAllCallbacks((Fl_Widget*)w, textNdragEndCallback);
// TODO: // TODO: 
// TODO: // TODO: #ifndef NO_XMIM
// TODO: // TODO:    /* Unregister the widget from the input manager */
// TODO: // TODO:    XmImUnregister((Fl_Widget*)w);
// TODO: // TODO: #endif
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Fl_Widget* resize method.  Called when the size of the widget changes
// TODO: // TODO: */
// TODO: // TODO: static void resize(TextWidget w)
// TODO: // TODO: {
// TODO: // TODO:    XFontStruct* fs = w->text.fontStruct;
// TODO: // TODO:    int height = w->core.height, width = w->core.width;
// TODO: // TODO:    int marginWidth = w->text.marginWidth, marginHeight = w->text.marginHeight;
// TODO: // TODO:    int lineNumAreaWidth = w->text.lineNumCols == 0 ? 0 : w->text.marginWidth +
// TODO: // TODO:                           fs->max_bounds.width * w->text.lineNumCols;
// TODO: // TODO: 
// TODO: // TODO:    w->text.columns = (width - marginWidth*2 - lineNumAreaWidth) /
// TODO: // TODO:                      fs->max_bounds.width;
// TODO: // TODO:    w->text.rows = (height - marginHeight*2) / (fs->ascent + fs->descent);
// TODO: // TODO: 
// TODO: // TODO:    /* Reject widths and heights less than a character, which the text
// TODO: // TODO:       display can't tolerate.  This is not strictly legal, but I've seen
// TODO: // TODO:       it done in other widgets and it seems to do no serious harm.  NEdit
// TODO: // TODO:       prevents panes from getting smaller than one line, but sometimes
// TODO: // TODO:       splitting windows on Linux 2.0 systems (same Motif, why the change in
// TODO: // TODO:       behavior?), causes one or two resize calls with < 1 line of height.
// TODO: // TODO:       Fixing it here is 100x easier than re-designing textDisp.c */
// TODO: // TODO:    if (w->text.columns < 1)
// TODO: // TODO:    {
// TODO: // TODO:       w->text.columns = 1;
// TODO: // TODO:       w->core.width = width = fs->max_bounds.width + marginWidth*2 +
// TODO: // TODO:                               lineNumAreaWidth;
// TODO: // TODO:    }
// TODO: // TODO:    if (w->text.rows < 1)
// TODO: // TODO:    {
// TODO: // TODO:       w->text.rows = 1;
// TODO: // TODO:       w->core.height = height = fs->ascent + fs->descent + marginHeight*2;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Resize the text display that the widget uses to render text */
// TODO: // TODO:    TextDResize(w->text.textD, width - marginWidth*2 - lineNumAreaWidth,
// TODO: // TODO:                height - marginHeight*2);
// TODO: // TODO: 
// TODO: // TODO:    /* if the window became shorter or narrower, there may be text left
// TODO: // TODO:       in the bottom or right margin area, which must be cleaned up */
// TODO: // TODO:    if (XtIsRealized((Fl_Widget*)w))
// TODO: // TODO:    {
// TODO: // TODO:       XClearArea(XtDisplay(w), XtWindow(w), 0, height-marginHeight,
// TODO: // TODO:                  width, marginHeight, false);
// TODO: // TODO:       XClearArea(XtDisplay(w), XtWindow(w),width-marginWidth,
// TODO: // TODO:                  0, marginWidth, height, false);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Fl_Widget* redisplay method
// TODO: // TODO: */
// TODO: // TODO: static void redisplay(TextWidget w, int event, Region region)
// TODO: // TODO: {
// TODO: // TODO:    XExposeEvent* e = &event->xexpose;
// TODO: // TODO: 
// TODO: // TODO:    TextDRedisplayRect(w->text.textD, e->x, e->y, e->width, e->height);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static Bool findGraphicsExposeOrNoExposeEvent(Display* theDisplay, int event, XPointer arg)
// TODO: // TODO: {
// TODO: // TODO:    if ((theDisplay == event->xany.display) &&
// TODO: // TODO:          (event->type == GraphicsExpose || event->type == NoExpose) &&
// TODO: // TODO:          ((Fl_Widget*)arg == XtWindowToWidget(event->xany.display, event->xany.window)))
// TODO: // TODO:    {
// TODO: // TODO:       return(true);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       return(false);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void adjustRectForGraphicsExposeOrNoExposeEvent(TextWidget w, int event,
// TODO: // TODO:       bool* first, int* left, int* top, int* width, int* height)
// TODO: // TODO: {
// TODO: // TODO:    bool removeQueueEntry = false;
// TODO: // TODO: 
// TODO: // TODO:    if (event->type == GraphicsExpose)
// TODO: // TODO:    {
// TODO: // TODO:       XGraphicsExposeEvent* e = &event->xgraphicsexpose;
// TODO: // TODO:       int x = e->x, y = e->y;
// TODO: // TODO: 
// TODO: // TODO:       TextDImposeGraphicsExposeTranslation(w->text.textD, &x, &y);
// TODO: // TODO:       if (*first)
// TODO: // TODO:       {
// TODO: // TODO:          *left = x;
// TODO: // TODO:          *top = y;
// TODO: // TODO:          *width = e->width;
// TODO: // TODO:          *height = e->height;
// TODO: // TODO: 
// TODO: // TODO:          *first = false;
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          int prev_left = *left;
// TODO: // TODO:          int prev_top = *top;
// TODO: // TODO: 
// TODO: // TODO:          *left = min(*left, x);
// TODO: // TODO:          *top = min(*top, y);
// TODO: // TODO:          *width = max(prev_left + *width, x + e->width) - *left;
// TODO: // TODO:          *height = max(prev_top + *height, y + e->height) - *top;
// TODO: // TODO:       }
// TODO: // TODO:       if (e->count == 0)
// TODO: // TODO:       {
// TODO: // TODO:          removeQueueEntry = true;
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO:    else if (event->type == NoExpose)
// TODO: // TODO:    {
// TODO: // TODO:       removeQueueEntry = true;
// TODO: // TODO:    }
// TODO: // TODO:    if (removeQueueEntry)
// TODO: // TODO:    {
// TODO: // TODO:       TextDPopGraphicExposeQueueEntry(w->text.textD);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void redisplayGE(TextWidget w, XtPointer client_data,
// TODO: // TODO:                         int event, bool* continue_to_dispatch_return)
// TODO: // TODO: {
// TODO: // TODO:    if (event->type == GraphicsExpose || event->type == NoExpose)
// TODO: // TODO:    {
// TODO: // TODO:       HandleAllPendingGraphicsExposeNoExposeEvents(w, event);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: void HandleAllPendingGraphicsExposeNoExposeEvents(TextWidget w, int event)
// TODO: // TODO: {
// TODO: // TODO:    XEvent foundEvent;
// TODO: // TODO:    int left;
// TODO: // TODO:    int top;
// TODO: // TODO:    int width;
// TODO: // TODO:    int height;
// TODO: // TODO:    bool invalidRect = true;
// TODO: // TODO: 
// TODO: // TODO:    if (event)
// TODO: // TODO:    {
// TODO: // TODO:       adjustRectForGraphicsExposeOrNoExposeEvent(w, event, &invalidRect, &left, &top, &width, &height);
// TODO: // TODO:    }
// TODO: // TODO:    while (XCheckIfEvent(XtDisplay(w), &foundEvent, findGraphicsExposeOrNoExposeEvent, (XPointer)w))
// TODO: // TODO:    {
// TODO: // TODO:       adjustRectForGraphicsExposeOrNoExposeEvent(w, &foundEvent, &invalidRect, &left, &top, &width, &height);
// TODO: // TODO:    }
// TODO: // TODO:    if (!invalidRect)
// TODO: // TODO:    {
// TODO: // TODO:       TextDRedisplayRect(w->text.textD, left, top, width, height);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Fl_Widget* setValues method
// TODO: // TODO: */
// TODO: // TODO: static bool setValues(TextWidget current, TextWidget request,
// TODO: // TODO:                          TextWidget newWidget)
// TODO: // TODO: {
// TODO: // TODO:    bool redraw = false, reconfigure = false;
// TODO: // TODO: 
// TODO: // TODO:    if (newWidget->text.overstrike != current->text.overstrike)
// TODO: // TODO:    {
// TODO: // TODO:       if (current->text.textD->cursorStyle == BLOCK_CURSOR)
// TODO: // TODO:          TextDSetCursorStyle(current->text.textD,
// TODO: // TODO:                              current->text.heavyCursor ? HEAVY_CURSOR : NORMAL_CURSOR);
// TODO: // TODO:       else if (current->text.textD->cursorStyle == NORMAL_CURSOR ||
// TODO: // TODO:                current->text.textD->cursorStyle == HEAVY_CURSOR)
// TODO: // TODO:          TextDSetCursorStyle(current->text.textD, BLOCK_CURSOR);
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    if (newWidget->text.fontStruct != current->text.fontStruct)
// TODO: // TODO:    {
// TODO: // TODO:       if (newWidget->text.lineNumCols != 0)
// TODO: // TODO:          reconfigure = true;
// TODO: // TODO:       TextDSetFont(current->text.textD, newWidget->text.fontStruct);
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    if (newWidget->text.wrapMargin != current->text.wrapMargin ||
// TODO: // TODO:          newWidget->text.continuousWrap != current->text.continuousWrap)
// TODO: // TODO:       TextDSetWrapMode(current->text.textD, newWidget->text.continuousWrap,
// TODO: // TODO:                        newWidget->text.wrapMargin);
// TODO: // TODO: 
// TODO: // TODO:    /* When delimiters are changed, copy the memory, so that the caller
// TODO: // TODO:       doesn't have to manage it, and add mandatory delimiters blank,
// TODO: // TODO:       tab, and newline to the list */
// TODO: // TODO:    if (newWidget->text.delimiters != current->text.delimiters)
// TODO: // TODO:    {
// TODO: // TODO:       char* delimiters = malloc__(strlen(newWidget->text.delimiters) + 4);
// TODO: // TODO:       XtFree(current->text.delimiters);
// TODO: // TODO:       sprintf(delimiters, "%s%s", " \t\n", newWidget->text.delimiters);
// TODO: // TODO:       newWidget->text.delimiters = delimiters;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Setting the lineNumCols resource tells the text widget to hide or
// TODO: // TODO:       show, or change the number of columns of the line number display,
// TODO: // TODO:       which requires re-organizing the x coordinates of both the line
// TODO: // TODO:       number display and the main text display */
// TODO: // TODO:    if (newWidget->text.lineNumCols != current->text.lineNumCols || reconfigure)
// TODO: // TODO:    {
// TODO: // TODO:       int marginWidth = newWidget->text.marginWidth;
// TODO: // TODO:       int charWidth = newWidget->text.fontStruct->max_bounds.width;
// TODO: // TODO:       int lineNumCols = newWidget->text.lineNumCols;
// TODO: // TODO:       if (lineNumCols == 0)
// TODO: // TODO:       {
// TODO: // TODO:          TextDSetLineNumberArea(newWidget->text.textD, 0, 0, marginWidth);
// TODO: // TODO:          newWidget->text.columns = (newWidget->core.width - marginWidth*2) / charWidth;
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          TextDSetLineNumberArea(newWidget->text.textD, marginWidth,
// TODO: // TODO:                                 charWidth * lineNumCols,
// TODO: // TODO:                                 2*marginWidth + charWidth * lineNumCols);
// TODO: // TODO:          newWidget->text.columns = (newWidget->core.width - marginWidth*3 - charWidth
// TODO: // TODO:                                     * lineNumCols) / charWidth;
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    if (newWidget->text.backlightCharTypes != current->text.backlightCharTypes)
// TODO: // TODO:    {
// TODO: // TODO:       TextDSetupBGClasses((Fl_Widget*)newWidget, newWidget->text.backlightCharTypes,
// TODO: // TODO:                           &newWidget->text.textD->bgClassPixel, &newWidget->text.textD->bgClass,
// TODO: // TODO:                           newWidget->text.textD->bgPixel);
// TODO: // TODO:       redraw = true;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    return redraw;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Fl_Widget* realize method
// TODO: // TODO: */
// TODO: // TODO: static void realize(Fl_Widget* w, XtValueMask* valueMask,
// TODO: // TODO:                     XSetWindowAttributes* attributes)
// TODO: // TODO: {
// TODO: // TODO:    /* Set bit gravity window attribute.  This saves a full blank and redraw
// TODO: // TODO:       on window resizing */
// TODO: // TODO:    *valueMask |= CWBitGravity;
// TODO: // TODO:    attributes->bit_gravity = NorthWestGravity;
// TODO: // TODO: 
// TODO: // TODO:    /* Continue with realize method from superclass */
// TODO: // TODO:    (xmPrimitiveClassRec.core_class.realize)(w, valueMask, attributes);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Fl_Widget* query geometry method ... unless asked to negotiate a different size simply return current size.
// TODO: // TODO: */
// TODO: // TODO: static XtGeometryResult queryGeometry(Fl_Widget* w, XtWidgetGeometry* proposed,
// TODO: // TODO:                                       XtWidgetGeometry* answer)
// TODO: // TODO: {
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO: 
// TODO: // TODO:    int curHeight = tw->core.height;
// TODO: // TODO:    int curWidth = tw->core.width;
// TODO: // TODO:    XFontStruct* fs = tw->text.textD->fontStruct;
// TODO: // TODO:    int fontWidth = fs->max_bounds.width;
// TODO: // TODO:    int fontHeight = fs->ascent + fs->descent;
// TODO: // TODO:    int marginHeight = tw->text.marginHeight;
// TODO: // TODO:    int propWidth = (proposed->request_mode & CWWidth) ? proposed->width : 0;
// TODO: // TODO:    int propHeight = (proposed->request_mode & CWHeight) ? proposed->height : 0;
// TODO: // TODO: 
// TODO: // TODO:    answer->request_mode = CWHeight | CWWidth;
// TODO: // TODO: 
// TODO: // TODO:    if (proposed->request_mode & CWWidth)
// TODO: // TODO:       /* Accept a width no smaller than 10 chars */
// TODO: // TODO:       answer->width = max(fontWidth * 10, proposed->width);
// TODO: // TODO:    else
// TODO: // TODO:       answer->width = curWidth;
// TODO: // TODO: 
// TODO: // TODO:    if (proposed->request_mode & CWHeight)
// TODO: // TODO:       /* Accept a height no smaller than an exact multiple of the line height
// TODO: // TODO:          and at least one line high */
// TODO: // TODO:       answer->height = max(1, ((propHeight - 2*marginHeight) / fontHeight)) *
// TODO: // TODO:                        fontHeight + 2*marginHeight;
// TODO: // TODO:    else
// TODO: // TODO:       answer->height = curHeight;
// TODO: // TODO: 
// TODO: // TODO:    /*printf("propWidth %d, propHeight %d, ansWidth %d, ansHeight %d\n",
// TODO: // TODO:    	    propWidth, propHeight, answer->width, answer->height);*/
// TODO: // TODO:    if (propWidth == answer->width && propHeight == answer->height)
// TODO: // TODO:       return XtGeometryYes;
// TODO: // TODO:    else if (answer->width == curWidth && answer->height == curHeight)
// TODO: // TODO:       return XtGeometryNo;
// TODO: // TODO:    else
// TODO: // TODO:       return XtGeometryAlmost;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Set the text buffer which this widget will display and interact with.
// TODO: // TODO: ** The currently attached buffer is automatically freed, ONLY if it has
// TODO: // TODO: ** no additional modify procs attached (as it would if it were being
// TODO: // TODO: ** displayed by another text widget).
// TODO: // TODO: */
// TODO: // TODO: void TextSetBuffer(Fl_Widget* w, textBuffer* buffer)
// TODO: // TODO: {
// TODO: // TODO:    textBuffer* oldBuf = ((TextWidget)w)->text.textD->buffer;
// TODO: // TODO: 
// TODO: // TODO:    StopHandlingXSelections(w);
// TODO: // TODO:    TextDSetBuffer(((TextWidget)w)->text.textD, buffer);
// TODO: // TODO:    if (oldBuf->nModifyProcs == 0)
// TODO: // TODO:       BufFree(oldBuf);
// TODO: // TODO: }
// TODO: 
// TODO: // Get the buffer associated with this text widget.  Note that attaching
// TODO: // additional modify callbacks to the buffer will prevent it from being
// TODO: // automatically freed when the widget is destroyed.
// TODO: textBuffer* TextGetBuffer(Fl_Widget* w)
// TODO: {
// TODO:    return 0; // TODO: ((TextWidget)w)->text.textD->buffer;
// TODO: }
// TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Translate a line number and column into a position
// TODO: // TODO: */
// TODO: // TODO: int TextLineAndColToPos(Fl_Widget* w, int lineNum, int column)
// TODO: // TODO: {
// TODO: // TODO:    return TextDLineAndColToPos(((TextWidget)w)->text.textD, lineNum, column);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: 
// TODO: // Translate a position into a line number (if the position is visible,
// TODO: // if it's not, return false
// TODO: int TextPosToLineAndCol(Fl_Text_Editor* text, int pos, int* lineNum, int* column)
// TODO: {
// TODO:    int x, y;
// TODO:    text->position_to_xy(0, &x, &y);
// TODO: // TODO:    text->position_to_linecol(text->insert_position(), lineNum, column);
// TODO:    return true;
// TODO: }
// TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Translate a buffer text position to the XY location where the center
// TODO: // TODO: ** of the cursor would be positioned to point to that character.  Returns
// TODO: // TODO: ** false if the position is not displayed because it is VERTICALLY out
// TODO: // TODO: ** of view.  If the position is horizontally out of view, returns the
// TODO: // TODO: ** x coordinate where the position would be if it were visible.
// TODO: // TODO: */
// TODO: // TODO: int TextPosToXY(Fl_Widget* w, int pos, int* x, int* y)
// TODO: // TODO: {
// TODO: // TODO:    return TextDPositionToXY(((TextWidget)w)->text.textD, pos, x, y);
// TODO: // TODO: }
// TODO: 
// TODO: // Return the cursor position
// TODO: int TextGetCursorPos(Fl_Widget* w)
// TODO: {
// TODO:    int pos = 0;
// TODO:    Fl_Text_Editor* text = dynamic_cast<Fl_Text_Editor*>(w);
// TODO:    if (text)
// TODO:       pos = text->insert_position();
// TODO: 
// TODO:    return pos;
// TODO: }
// TODO: 
// TODO: // Set the cursor position
// TODO: void TextSetCursorPos(Fl_Widget* w, int pos)
// TODO: {
// TODO: // TODO:    TextDSetInsertPosition(((TextWidget)w)->text.textD, pos);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, NULL);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Return the horizontal and vertical scroll positions of the widget
// TODO: */
// TODO: void TextGetScroll(Fl_Widget* w, int* topLineNum, int* horizOffset)
// TODO: {
// TODO: // TODO:    TextDGetScroll(((TextWidget)w)->text.textD, topLineNum, horizOffset);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Set the horizontal and vertical scroll positions of the widget
// TODO: */
// TODO: void TextSetScroll(Fl_Widget* w, int topLineNum, int horizOffset)
// TODO: {
// TODO: // TODO:    TextDSetScroll(((TextWidget)w)->text.textD, topLineNum, horizOffset);
// TODO: }
// TODO: 
// TODO: int TextGetMinFontWidth(Fl_Widget* w, bool considerStyles)
// TODO: {
// TODO: // TODO:    return(TextDMinFontWidth(((TextWidget)w)->text.textD, considerStyles));
// TODO:    return 0;
// TODO: }
// TODO: 
// TODO: int TextGetMaxFontWidth(Fl_Widget* w, bool considerStyles)
// TODO: {
// TODO: // TODO:    return(TextDMaxFontWidth(((TextWidget)w)->text.textD, considerStyles));
// TODO:    return 0;
// TODO: }
// TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Set this widget to be the owner of selections made in it's attached
// TODO: // TODO: ** buffer (text buffers may be shared among several text widgets).
// TODO: // TODO: */
// TODO: // TODO: void TextHandleXSelections(Fl_Widget* w)
// TODO: // TODO: {
// TODO: // TODO:    HandleXSelections(w);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: void TextPasteClipboard(Fl_Widget* w, Time time)
// TODO: // TODO: {
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    TakeMotifDestination(w, time);
// TODO: // TODO:    InsertClipboard(w, false);
// TODO: // TODO:    callCursorMovementCBs(w, NULL);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: void TextColPasteClipboard(Fl_Widget* w, Time time)
// TODO: // TODO: {
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    TakeMotifDestination(w, time);
// TODO: // TODO:    InsertClipboard(w, true);
// TODO: // TODO:    callCursorMovementCBs(w, NULL);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: void TextCopyClipboard(Fl_Widget* w, Time time)
// TODO: // TODO: {
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (!((TextWidget)w)->text.textD->buffer->primary.selected)
// TODO: // TODO:    {
// TODO: // TODO:       XBell(XtDisplay(w), 0);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO:    CopyToClipboard(w, time);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: void TextCutClipboard(Fl_Widget* w, Time time)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    if (!textD->buffer->primary.selected)
// TODO: // TODO:    {
// TODO: // TODO:       XBell(XtDisplay(w), 0);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO:    TakeMotifDestination(w, time);
// TODO: // TODO:    CopyToClipboard(w, time);
// TODO: // TODO:    BufRemoveSelected(textD->buffer);
// TODO: // TODO:    TextDSetInsertPosition(textD, textD->buffer->cursorPosHint);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO: }
// TODO: 
// TODO: int TextFirstVisibleLine(Fl_Widget* w)
// TODO: {
// TODO: // TODO:    return(((TextWidget)w)->text.textD->topLineNum);
// TODO:    return 0;
// TODO: }
// TODO: 
// TODO: int TextNumVisibleLines(Fl_Widget* w)
// TODO: {
// TODO: // TODO:    return(((TextWidget)w)->text.textD->nVisibleLines);
// TODO:    return 0;
// TODO: }
// TODO: 
// TODO: int TextVisibleWidth(Fl_Widget* w)
// TODO: {
// TODO: // TODO:    return(((TextWidget)w)->text.textD->width);
// TODO:    return 0;
// TODO: }
// TODO: 
// TODO: // TODO: int TextFirstVisiblePos(Fl_Widget* w)
// TODO: // TODO: {
// TODO: // TODO:    return ((TextWidget)w)->text.textD->firstChar;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: int TextLastVisiblePos(Fl_Widget* w)
// TODO: // TODO: {
// TODO: // TODO:    return ((TextWidget)w)->text.textD->lastChar;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Insert text "chars" at the cursor position, respecting pending delete
// TODO: // TODO: ** selections, overstrike, and handling cursor repositioning as if the text
// TODO: // TODO: ** had been typed.  If autoWrap is on wraps the text to fit within the wrap
// TODO: // TODO: ** margin, auto-indenting where the line was wrapped (but nowhere else).
// TODO: // TODO: ** "allowPendingDelete" controls whether primary selections in the widget are
// TODO: // TODO: ** treated as pending delete selections (true), or ignored (false). "event"
// TODO: // TODO: ** is optional and is just passed on to the cursor movement callbacks.
// TODO: // TODO: */
// TODO: // TODO: void TextInsertAtCursor(Fl_Widget* w, char* chars, int event,
// TODO: // TODO:                         int allowPendingDelete, int allowWrap)
// TODO: // TODO: {
// TODO: // TODO:    int wrapMargin, colNum, lineStartPos, cursorPos;
// TODO: // TODO:    char* c, *lineStartText, *wrappedText;
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO:    textDisp* textD = tw->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    int fontWidth = textD->fontStruct->max_bounds.width;
// TODO: // TODO:    int replaceSel, singleLine, breakAt = 0;
// TODO: // TODO: 
// TODO: // TODO:    /* Don't wrap if auto-wrap is off or suppressed, or it's just a newline */
// TODO: // TODO:    if (!allowWrap || !tw->text.autoWrap ||
// TODO: // TODO:          (chars[0] == '\n' && chars[1] == '\0'))
// TODO: // TODO:    {
// TODO: // TODO:       simpleInsertAtCursor(w, chars, event, allowPendingDelete);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* If this is going to be a pending delete operation, the real insert
// TODO: // TODO:       position is the start of the selection.  This will make rectangular
// TODO: // TODO:       selections wrap strangely, but this routine should rarely be used for
// TODO: // TODO:       them, and even more rarely when they need to be wrapped. */
// TODO: // TODO:    replaceSel = allowPendingDelete && pendingSelection(w);
// TODO: // TODO:    cursorPos = replaceSel ? buf->primary.start : TextDGetInsertPosition(textD);
// TODO: // TODO: 
// TODO: // TODO:    /* If the text is only one line and doesn't need to be wrapped, just insert
// TODO: // TODO:       it and be done (for efficiency only, this routine is called for each
// TODO: // TODO:       character typed). (Of course, it may not be significantly more efficient
// TODO: // TODO:       than the more general code below it, so it may be a waste of time!) */
// TODO: // TODO:    wrapMargin = tw->text.wrapMargin != 0 ? tw->text.wrapMargin :
// TODO: // TODO:                 textD->width / fontWidth;
// TODO: // TODO:    lineStartPos = BufStartOfLine(buf, cursorPos);
// TODO: // TODO:    colNum = BufCountDispChars(buf, lineStartPos, cursorPos);
// TODO: // TODO:    for (c=chars; *c!='\0' && *c!='\n'; c++)
// TODO: // TODO:       colNum += BufCharWidth(*c, colNum, buf->tabDist, buf->nullSubsChar);
// TODO: // TODO:    singleLine = *c == '\0';
// TODO: // TODO:    if (colNum < wrapMargin && singleLine)
// TODO: // TODO:    {
// TODO: // TODO:       simpleInsertAtCursor(w, chars, event, true);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Wrap the text */
// TODO: // TODO:    lineStartText = BufGetRange(buf, lineStartPos, cursorPos);
// TODO: // TODO:    wrappedText = wrapText(tw, lineStartText, chars, lineStartPos, wrapMargin,
// TODO: // TODO:                           replaceSel ? NULL : &breakAt);
// TODO: // TODO:    XtFree(lineStartText);
// TODO: // TODO: 
// TODO: // TODO:    /* Insert the text.  Where possible, use TextDInsert which is optimized
// TODO: // TODO:       for less redraw. */
// TODO: // TODO:    if (replaceSel)
// TODO: // TODO:    {
// TODO: // TODO:       BufReplaceSelected(buf, wrappedText);
// TODO: // TODO:       TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO: // TODO:    }
// TODO: // TODO:    else if (tw->text.overstrike)
// TODO: // TODO:    {
// TODO: // TODO:       if (breakAt == 0 && singleLine)
// TODO: // TODO:          TextDOverstrike(textD, wrappedText);
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          BufReplace(buf, cursorPos-breakAt, cursorPos, wrappedText);
// TODO: // TODO:          TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       if (breakAt == 0)
// TODO: // TODO:       {
// TODO: // TODO:          TextDInsert(textD, wrappedText);
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          BufReplace(buf, cursorPos-breakAt, cursorPos, wrappedText);
// TODO: // TODO:          TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO:    XtFree(wrappedText);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: 
// TODO: /*
// TODO: ** Fetch text from the widget's buffer, adding wrapping newlines to emulate
// TODO: ** effect acheived by wrapping in the text display in continuous wrap mode.
// TODO: */
// TODO: char* TextGetWrapped(Fl_Widget* w, int startPos, int endPos, int* outLen)
// TODO: {
// TODO:    char *outString = 0;
// TODO: // TODO:    char c;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    textBuffer* outBuf;
// TODO: // TODO:    int fromPos, toPos, outPos;
// TODO: // TODO: 
// TODO: // TODO:    if (!((TextWidget)w)->text.continuousWrap || startPos == endPos)
// TODO: // TODO:    {
// TODO: // TODO:       *outLen = endPos - startPos;
// TODO: // TODO:       return BufGetRange(buf, startPos, endPos);
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Create a text buffer with a good estimate of the size that adding
// TODO: // TODO:       newlines will expand it to.  Since it's a text buffer, if we guess
// TODO: // TODO:       wrong, it will fail softly, and simply expand the size */
// TODO: // TODO:    outBuf = BufCreatePreallocated((endPos-startPos) + (endPos-startPos)/5);
// TODO: // TODO:    outPos = 0;
// TODO: // TODO: 
// TODO: // TODO:    /* Go (displayed) line by line through the buffer, adding newlines where
// TODO: // TODO:       the text is wrapped at some character other than an existing newline */
// TODO: // TODO:    fromPos = startPos;
// TODO: // TODO:    toPos = TextDCountForwardNLines(textD, startPos, 1, false);
// TODO: // TODO:    while (toPos < endPos)
// TODO: // TODO:    {
// TODO: // TODO:       BufCopyFromBuf(buf, outBuf, fromPos, toPos, outPos);
// TODO: // TODO:       outPos += toPos - fromPos;
// TODO: // TODO:       c = BufGetCharacter(outBuf, outPos-1);
// TODO: // TODO:       if (c == ' ' || c == '\t')
// TODO: // TODO:          BufReplace(outBuf, outPos-1, outPos, "\n");
// TODO: // TODO:       else if (c != '\n')
// TODO: // TODO:       {
// TODO: // TODO:          BufInsert(outBuf, outPos, "\n");
// TODO: // TODO:          outPos++;
// TODO: // TODO:       }
// TODO: // TODO:       fromPos = toPos;
// TODO: // TODO:       toPos = TextDCountForwardNLines(textD, fromPos, 1, true);
// TODO: // TODO:    }
// TODO: // TODO:    BufCopyFromBuf(buf, outBuf, fromPos, endPos, outPos);
// TODO: // TODO: 
// TODO: // TODO:    /* return the contents of the output buffer as a string */
// TODO: // TODO:    outString = BufGetAll(outBuf);
// TODO: // TODO:    *outLen = outBuf->length;
// TODO: // TODO:    BufFree(outBuf);
// TODO:    return outString;
// TODO: }
// TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Return the (statically allocated) action table for menu item actions.
// TODO: // TODO: **
// TODO: // TODO: ** Warning: This routine can only be used before the first text widget is
// TODO: // TODO: ** created!  After that, apparently, Xt takes over the table and overwrites
// TODO: // TODO: ** it with its own version.  XtGetActionList is preferable, but is not
// TODO: // TODO: ** available before X11R5.
// TODO: // TODO: */
// TODO: // TODO: XtActionsRec* TextGetActions(int* nActions)
// TODO: // TODO: {
// TODO: // TODO:    *nActions = XtNumber(actionsList);
// TODO: // TODO:    return actionsList;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void grabFocusAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XButtonEvent* e = &event->xbutton;
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO:    textDisp* textD = tw->text.textD;
// TODO: // TODO:    Time lastBtnDown = tw->text.lastBtnDown;
// TODO: // TODO:    int row, column;
// TODO: // TODO: 
// TODO: // TODO:    /* Indicate state for future events, PRIMARY_CLICKED indicates that
// TODO: // TODO:       the proper initialization has been done for primary dragging and/or
// TODO: // TODO:       multi-clicking.  Also record the timestamp for multi-click processing */
// TODO: // TODO:    tw->text.dragState = PRIMARY_CLICKED;
// TODO: // TODO:    tw->text.lastBtnDown = e->time;
// TODO: // TODO: 
// TODO: // TODO:    /* Become owner of the MOTIF_DESTINATION selection, making this widget
// TODO: // TODO:       the designated recipient of secondary quick actions in Motif XmText
// TODO: // TODO:       widgets and in other NEdit text widgets */
// TODO: // TODO:    TakeMotifDestination(w, e->time);
// TODO: // TODO: 
// TODO: // TODO:    /* Check for possible multi-click sequence in progress */
// TODO: // TODO:    if (tw->text.multiClickState != NO_CLICKS)
// TODO: // TODO:    {
// TODO: // TODO:       if (e->time < lastBtnDown + XtGetMultiClickTime(XtDisplay(w)))
// TODO: // TODO:       {
// TODO: // TODO:          if (tw->text.multiClickState == ONE_CLICK)
// TODO: // TODO:          {
// TODO: // TODO:             selectWord(w, e->x);
// TODO: // TODO:             callCursorMovementCBs(w, event);
// TODO: // TODO:             return;
// TODO: // TODO:          }
// TODO: // TODO:          else if (tw->text.multiClickState == TWO_CLICKS)
// TODO: // TODO:          {
// TODO: // TODO:             selectLine(w);
// TODO: // TODO:             callCursorMovementCBs(w, event);
// TODO: // TODO:             return;
// TODO: // TODO:          }
// TODO: // TODO:          else if (tw->text.multiClickState == THREE_CLICKS)
// TODO: // TODO:          {
// TODO: // TODO:             BufSelect(textD->buffer, 0, textD->buffer->length);
// TODO: // TODO:             return;
// TODO: // TODO:          }
// TODO: // TODO:          else if (tw->text.multiClickState > THREE_CLICKS)
// TODO: // TODO:             tw->text.multiClickState = NO_CLICKS;
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:          tw->text.multiClickState = NO_CLICKS;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Clear any existing selections */
// TODO: // TODO:    BufUnselect(textD->buffer);
// TODO: // TODO: 
// TODO: // TODO:    /* Move the cursor to the pointer location */
// TODO: // TODO:    moveDestinationAP(w, event, args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    /* Record the site of the initial button press and the initial character
// TODO: // TODO:       position so subsequent motion events and clicking can decide when and
// TODO: // TODO:       where to begin a primary selection */
// TODO: // TODO:    tw->text.btnDownX = e->x;
// TODO: // TODO:    tw->text.btnDownY = e->y;
// TODO: // TODO:    tw->text.anchor = TextDGetInsertPosition(textD);
// TODO: // TODO:    TextDXYToUnconstrainedPosition(textD, e->x, e->y, &row, &column);
// TODO: // TODO:    column = TextDOffsetWrappedColumn(textD, row, column);
// TODO: // TODO:    tw->text.rectAnchor = column;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void moveDestinationAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XButtonEvent* e = &event->xbutton;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO: 
// TODO: // TODO:    /* Get input focus */
// TODO: // TODO:    XmProcessTraversal(w, XmTRAVERSE_CURRENT);
// TODO: // TODO: 
// TODO: // TODO:    /* Move the cursor */
// TODO: // TODO:    TextDSetInsertPosition(textD, TextDXYToPosition(textD, e->x, e->y));
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void extendAdjustAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                            int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO:    XMotionEvent* e = &event->xmotion;
// TODO: // TODO:    int dragState = tw->text.dragState;
// TODO: // TODO:    int rectDrag = hasKey("rect", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    /* Make sure the proper initialization was done on mouse down */
// TODO: // TODO:    if (dragState != PRIMARY_DRAG && dragState != PRIMARY_CLICKED &&
// TODO: // TODO:          dragState != PRIMARY_RECT_DRAG)
// TODO: // TODO:       return;
// TODO: // TODO: 
// TODO: // TODO:    /* If the selection hasn't begun, decide whether the mouse has moved
// TODO: // TODO:       far enough from the initial mouse down to be considered a drag */
// TODO: // TODO:    if (tw->text.dragState == PRIMARY_CLICKED)
// TODO: // TODO:    {
// TODO: // TODO:       if (abs(e->x - tw->text.btnDownX) > SELECT_THRESHOLD ||
// TODO: // TODO:             abs(e->y - tw->text.btnDownY) > SELECT_THRESHOLD)
// TODO: // TODO:          tw->text.dragState = rectDrag ? PRIMARY_RECT_DRAG : PRIMARY_DRAG;
// TODO: // TODO:       else
// TODO: // TODO:          return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* If "rect" argument has appeared or disappeared, keep dragState up
// TODO: // TODO:       to date about which type of drag this is */
// TODO: // TODO:    tw->text.dragState = rectDrag ? PRIMARY_RECT_DRAG : PRIMARY_DRAG;
// TODO: // TODO: 
// TODO: // TODO:    /* Record the new position for the autoscrolling timer routine, and
// TODO: // TODO:       engage or disengage the timer if the mouse is in/out of the window */
// TODO: // TODO:    checkAutoScroll(tw, e->x, e->y);
// TODO: // TODO: 
// TODO: // TODO:    /* Adjust the selection and move the cursor */
// TODO: // TODO:    adjustSelection(tw, e->x, e->y);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void extendStartAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                           int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XMotionEvent* e = &event->xmotion;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    selection* sel = &buf->primary;
// TODO: // TODO:    int anchor, rectAnchor, anchorLineStart, newPos, row, column;
// TODO: // TODO: 
// TODO: // TODO:    /* Find the new anchor point for the rest of this drag operation */
// TODO: // TODO:    newPos = TextDXYToPosition(textD, e->x, e->y);
// TODO: // TODO:    TextDXYToUnconstrainedPosition(textD, e->x, e->y, &row, &column);
// TODO: // TODO:    column = TextDOffsetWrappedColumn(textD, row, column);
// TODO: // TODO:    if (sel->selected)
// TODO: // TODO:    {
// TODO: // TODO:       if (sel->rectangular)
// TODO: // TODO:       {
// TODO: // TODO:          rectAnchor = column < (sel->rectEnd + sel->rectStart) / 2 ?
// TODO: // TODO:                       sel->rectEnd : sel->rectStart;
// TODO: // TODO:          anchorLineStart = BufStartOfLine(buf, newPos <
// TODO: // TODO:                                           (sel->end + sel->start) / 2 ? sel->end : sel->start);
// TODO: // TODO:          anchor = BufCountForwardDispChars(buf, anchorLineStart, rectAnchor);
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          if (abs(newPos - sel->start) < abs(newPos - sel->end))
// TODO: // TODO:             anchor = sel->end;
// TODO: // TODO:          else
// TODO: // TODO:             anchor = sel->start;
// TODO: // TODO:          anchorLineStart = BufStartOfLine(buf, anchor);
// TODO: // TODO:          rectAnchor = BufCountDispChars(buf, anchorLineStart, anchor);
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       anchor = TextDGetInsertPosition(textD);
// TODO: // TODO:       anchorLineStart = BufStartOfLine(buf, anchor);
// TODO: // TODO:       rectAnchor = BufCountDispChars(buf, anchorLineStart, anchor);
// TODO: // TODO:    }
// TODO: // TODO:    ((TextWidget)w)->text.anchor = anchor;
// TODO: // TODO:    ((TextWidget)w)->text.rectAnchor = rectAnchor;
// TODO: // TODO: 
// TODO: // TODO:    /* Make the new selection */
// TODO: // TODO:    if (hasKey("rect", args, nArgs))
// TODO: // TODO:       BufRectSelect(buf, BufStartOfLine(buf, min(anchor, newPos)),
// TODO: // TODO:                     BufEndOfLine(buf, max(anchor, newPos)),
// TODO: // TODO:                     min(rectAnchor, column), max(rectAnchor, column));
// TODO: // TODO:    else
// TODO: // TODO:       BufSelect(buf, min(anchor, newPos), max(anchor, newPos));
// TODO: // TODO: 
// TODO: // TODO:    /* Never mind the motion threshold, go right to dragging since
// TODO: // TODO:       extend-start is unambiguously the start of a selection */
// TODO: // TODO:    ((TextWidget)w)->text.dragState = PRIMARY_DRAG;
// TODO: // TODO: 
// TODO: // TODO:    /* Don't do by-word or by-line adjustment, just by character */
// TODO: // TODO:    ((TextWidget)w)->text.multiClickState = NO_CLICKS;
// TODO: // TODO: 
// TODO: // TODO:    /* Move the cursor */
// TODO: // TODO:    TextDSetInsertPosition(textD, newPos);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void extendEndAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                         int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XButtonEvent* e = &event->xbutton;
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO: 
// TODO: // TODO:    if (tw->text.dragState == PRIMARY_CLICKED &&
// TODO: // TODO:          tw->text.lastBtnDown <= e->time + XtGetMultiClickTime(XtDisplay(w)))
// TODO: // TODO:       tw->text.multiClickState++;
// TODO: // TODO:    endDrag(w);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void processCancelAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                             int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    int dragState = ((TextWidget)w)->text.dragState;
// TODO: // TODO:    textBuffer* buf = ((TextWidget)w)->text.textD->buffer;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO: 
// TODO: // TODO:    /* If there's a calltip displayed, kill it. */
// TODO: // TODO:    TextDKillCalltip(textD, 0);
// TODO: // TODO: 
// TODO: // TODO:    if (dragState == PRIMARY_DRAG || dragState == PRIMARY_RECT_DRAG)
// TODO: // TODO:       BufUnselect(buf);
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void secondaryStartAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                              int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XMotionEvent* e = &event->xmotion;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    selection* sel = &buf->secondary;
// TODO: // TODO:    int anchor, pos, row, column;
// TODO: // TODO: 
// TODO: // TODO:    /* Find the new anchor point and make the new selection */
// TODO: // TODO:    pos = TextDXYToPosition(textD, e->x, e->y);
// TODO: // TODO:    if (sel->selected)
// TODO: // TODO:    {
// TODO: // TODO:       if (abs(pos - sel->start) < abs(pos - sel->end))
// TODO: // TODO:          anchor = sel->end;
// TODO: // TODO:       else
// TODO: // TODO:          anchor = sel->start;
// TODO: // TODO:       BufSecondarySelect(buf, anchor, pos);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:       anchor = pos;
// TODO: // TODO: 
// TODO: // TODO:    /* Record the site of the initial button press and the initial character
// TODO: // TODO:       position so subsequent motion events can decide when to begin a
// TODO: // TODO:       selection, (and where the selection began) */
// TODO: // TODO:    ((TextWidget)w)->text.btnDownX = e->x;
// TODO: // TODO:    ((TextWidget)w)->text.btnDownY = e->y;
// TODO: // TODO:    ((TextWidget)w)->text.anchor = pos;
// TODO: // TODO:    TextDXYToUnconstrainedPosition(textD, e->x, e->y, &row, &column);
// TODO: // TODO:    column = TextDOffsetWrappedColumn(textD, row, column);
// TODO: // TODO:    ((TextWidget)w)->text.rectAnchor = column;
// TODO: // TODO:    ((TextWidget)w)->text.dragState = SECONDARY_CLICKED;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void secondaryOrDragStartAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                    int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XMotionEvent* e = &event->xmotion;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO: 
// TODO: // TODO:    /* If the click was outside of the primary selection, this is not
// TODO: // TODO:       a drag, start a secondary selection */
// TODO: // TODO:    if (!buf->primary.selected || !TextDInSelection(textD, e->x, e->y))
// TODO: // TODO:    {
// TODO: // TODO:       secondaryStartAP(w, event, args, nArgs);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO: 
// TODO: // TODO:    /* Record the site of the initial button press and the initial character
// TODO: // TODO:       position so subsequent motion events can decide when to begin a
// TODO: // TODO:       drag, and where to drag to */
// TODO: // TODO:    ((TextWidget)w)->text.btnDownX = e->x;
// TODO: // TODO:    ((TextWidget)w)->text.btnDownY = e->y;
// TODO: // TODO:    ((TextWidget)w)->text.dragState = CLICKED_IN_SELECTION;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void secondaryAdjustAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO:    XMotionEvent* e = &event->xmotion;
// TODO: // TODO:    int dragState = tw->text.dragState;
// TODO: // TODO:    int rectDrag = hasKey("rect", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    /* Make sure the proper initialization was done on mouse down */
// TODO: // TODO:    if (dragState != SECONDARY_DRAG && dragState != SECONDARY_RECT_DRAG &&
// TODO: // TODO:          dragState != SECONDARY_CLICKED)
// TODO: // TODO:       return;
// TODO: // TODO: 
// TODO: // TODO:    /* If the selection hasn't begun, decide whether the mouse has moved
// TODO: // TODO:       far enough from the initial mouse down to be considered a drag */
// TODO: // TODO:    if (tw->text.dragState == SECONDARY_CLICKED)
// TODO: // TODO:    {
// TODO: // TODO:       if (abs(e->x - tw->text.btnDownX) > SELECT_THRESHOLD ||
// TODO: // TODO:             abs(e->y - tw->text.btnDownY) > SELECT_THRESHOLD)
// TODO: // TODO:          tw->text.dragState = rectDrag ? SECONDARY_RECT_DRAG: SECONDARY_DRAG;
// TODO: // TODO:       else
// TODO: // TODO:          return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* If "rect" argument has appeared or disappeared, keep dragState up
// TODO: // TODO:       to date about which type of drag this is */
// TODO: // TODO:    tw->text.dragState = rectDrag ? SECONDARY_RECT_DRAG : SECONDARY_DRAG;
// TODO: // TODO: 
// TODO: // TODO:    /* Record the new position for the autoscrolling timer routine, and
// TODO: // TODO:       engage or disengage the timer if the mouse is in/out of the window */
// TODO: // TODO:    checkAutoScroll(tw, e->x, e->y);
// TODO: // TODO: 
// TODO: // TODO:    /* Adjust the selection */
// TODO: // TODO:    adjustSecondarySelection(tw, e->x, e->y);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void secondaryOrDragAdjustAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                     int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO:    XMotionEvent* e = &event->xmotion;
// TODO: // TODO:    int dragState = tw->text.dragState;
// TODO: // TODO: 
// TODO: // TODO:    /* Only dragging of blocks of text is handled in this action proc.
// TODO: // TODO:       Otherwise, defer to secondaryAdjust to handle the rest */
// TODO: // TODO:    if (dragState != CLICKED_IN_SELECTION && dragState != PRIMARY_BLOCK_DRAG)
// TODO: // TODO:    {
// TODO: // TODO:       secondaryAdjustAP(w, event, args, nArgs);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Decide whether the mouse has moved far enough from the
// TODO: // TODO:       initial mouse down to be considered a drag */
// TODO: // TODO:    if (tw->text.dragState == CLICKED_IN_SELECTION)
// TODO: // TODO:    {
// TODO: // TODO:       if (abs(e->x - tw->text.btnDownX) > SELECT_THRESHOLD ||
// TODO: // TODO:             abs(e->y - tw->text.btnDownY) > SELECT_THRESHOLD)
// TODO: // TODO:          BeginBlockDrag(tw);
// TODO: // TODO:       else
// TODO: // TODO:          return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Record the new position for the autoscrolling timer routine, and
// TODO: // TODO:       engage or disengage the timer if the mouse is in/out of the window */
// TODO: // TODO:    checkAutoScroll(tw, e->x, e->y);
// TODO: // TODO: 
// TODO: // TODO:    /* Adjust the selection */
// TODO: // TODO:    BlockDragSelection(tw, e->x, e->y, hasKey("overlay", args, nArgs) ?
// TODO: // TODO:                       (hasKey("copy", args, nArgs) ? DRAG_OVERLAY_COPY : DRAG_OVERLAY_MOVE) :
// TODO: // TODO:                          (hasKey("copy", args, nArgs) ? DRAG_COPY : DRAG_MOVE));
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void copyToAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XButtonEvent* e = &event->xbutton;
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO:    textDisp* textD = tw->text.textD;
// TODO: // TODO:    int dragState = tw->text.dragState;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    selection* secondary = &buf->secondary, *primary = &buf->primary;
// TODO: // TODO:    int rectangular = secondary->rectangular;
// TODO: // TODO:    char* textToCopy;
// TODO: // TODO:    int insertPos, lineStart, column;
// TODO: // TODO: 
// TODO: // TODO:    endDrag(w);
// TODO: // TODO:    if (!((dragState == SECONDARY_DRAG && secondary->selected) ||
// TODO: // TODO:          (dragState == SECONDARY_RECT_DRAG && secondary->selected) ||
// TODO: // TODO:          dragState == SECONDARY_CLICKED || dragState == NOT_CLICKED))
// TODO: // TODO:       return;
// TODO: // TODO:    if (!(secondary->selected && !((TextWidget)w)->text.motifDestOwner))
// TODO: // TODO:    {
// TODO: // TODO:       if (checkReadOnly(w))
// TODO: // TODO:       {
// TODO: // TODO:          BufSecondaryUnselect(buf);
// TODO: // TODO:          return;
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO:    if (secondary->selected)
// TODO: // TODO:    {
// TODO: // TODO:       if (tw->text.motifDestOwner)
// TODO: // TODO:       {
// TODO: // TODO:          TextDBlankCursor(textD);
// TODO: // TODO:          textToCopy = BufGetSecSelectText(buf);
// TODO: // TODO:          if (primary->selected && rectangular)
// TODO: // TODO:          {
// TODO: // TODO:             insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:             BufReplaceSelected(buf, textToCopy);
// TODO: // TODO:             TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO: // TODO:          }
// TODO: // TODO:          else if (rectangular)
// TODO: // TODO:          {
// TODO: // TODO:             insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:             lineStart = BufStartOfLine(buf, insertPos);
// TODO: // TODO:             column = BufCountDispChars(buf, lineStart, insertPos);
// TODO: // TODO:             BufInsertCol(buf, column, lineStart, textToCopy, NULL, NULL);
// TODO: // TODO:             TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO: // TODO:          }
// TODO: // TODO:          else
// TODO: // TODO:             TextInsertAtCursor(w, textToCopy, event, true,
// TODO: // TODO:                                tw->text.autoWrapPastedText);
// TODO: // TODO:          XtFree(textToCopy);
// TODO: // TODO:          BufSecondaryUnselect(buf);
// TODO: // TODO:          TextDUnblankCursor(textD);
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:          SendSecondarySelection(w, e->time, false);
// TODO: // TODO:    }
// TODO: // TODO:    else if (primary->selected)
// TODO: // TODO:    {
// TODO: // TODO:       textToCopy = BufGetSelectionText(buf);
// TODO: // TODO:       TextDSetInsertPosition(textD, TextDXYToPosition(textD, e->x, e->y));
// TODO: // TODO:       TextInsertAtCursor(w, textToCopy, event, false,
// TODO: // TODO:                          tw->text.autoWrapPastedText);
// TODO: // TODO:       XtFree(textToCopy);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       TextDSetInsertPosition(textD, TextDXYToPosition(textD, e->x, e->y));
// TODO: // TODO:       InsertPrimarySelection(w, e->time, false);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void copyToOrEndDragAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    int dragState = ((TextWidget)w)->text.dragState;
// TODO: // TODO: 
// TODO: // TODO:    if (dragState != PRIMARY_BLOCK_DRAG)
// TODO: // TODO:    {
// TODO: // TODO:       copyToAP(w, event, args, nArgs);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    FinishBlockDrag((TextWidget)w);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void moveToAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XButtonEvent* e = &event->xbutton;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int dragState = ((TextWidget)w)->text.dragState;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    selection* secondary = &buf->secondary, *primary = &buf->primary;
// TODO: // TODO:    int insertPos, rectangular = secondary->rectangular;
// TODO: // TODO:    int column, lineStart;
// TODO: // TODO:    char* textToCopy;
// TODO: // TODO: 
// TODO: // TODO:    endDrag(w);
// TODO: // TODO:    if (!((dragState == SECONDARY_DRAG && secondary->selected) ||
// TODO: // TODO:          (dragState == SECONDARY_RECT_DRAG && secondary->selected) ||
// TODO: // TODO:          dragState == SECONDARY_CLICKED || dragState == NOT_CLICKED))
// TODO: // TODO:       return;
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:    {
// TODO: // TODO:       BufSecondaryUnselect(buf);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    if (secondary->selected)
// TODO: // TODO:    {
// TODO: // TODO:       if (((TextWidget)w)->text.motifDestOwner)
// TODO: // TODO:       {
// TODO: // TODO:          textToCopy = BufGetSecSelectText(buf);
// TODO: // TODO:          if (primary->selected && rectangular)
// TODO: // TODO:          {
// TODO: // TODO:             insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:             BufReplaceSelected(buf, textToCopy);
// TODO: // TODO:             TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO: // TODO:          }
// TODO: // TODO:          else if (rectangular)
// TODO: // TODO:          {
// TODO: // TODO:             insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:             lineStart = BufStartOfLine(buf, insertPos);
// TODO: // TODO:             column = BufCountDispChars(buf, lineStart, insertPos);
// TODO: // TODO:             BufInsertCol(buf, column, lineStart, textToCopy, NULL, NULL);
// TODO: // TODO:             TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO: // TODO:          }
// TODO: // TODO:          else
// TODO: // TODO:             TextInsertAtCursor(w, textToCopy, event, true,
// TODO: // TODO:                                ((TextWidget)w)->text.autoWrapPastedText);
// TODO: // TODO:          XtFree(textToCopy);
// TODO: // TODO:          BufRemoveSecSelect(buf);
// TODO: // TODO:          BufSecondaryUnselect(buf);
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:          SendSecondarySelection(w, e->time, true);
// TODO: // TODO:    }
// TODO: // TODO:    else if (primary->selected)
// TODO: // TODO:    {
// TODO: // TODO:       textToCopy = BufGetRange(buf, primary->start, primary->end);
// TODO: // TODO:       TextDSetInsertPosition(textD, TextDXYToPosition(textD, e->x, e->y));
// TODO: // TODO:       TextInsertAtCursor(w, textToCopy, event, false,
// TODO: // TODO:                          ((TextWidget)w)->text.autoWrapPastedText);
// TODO: // TODO:       XtFree(textToCopy);
// TODO: // TODO:       BufRemoveSelected(buf);
// TODO: // TODO:       BufUnselect(buf);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       TextDSetInsertPosition(textD, TextDXYToPosition(textD, e->x, e->y));
// TODO: // TODO:       MovePrimarySelection(w, e->time, false);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void moveToOrEndDragAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    int dragState = ((TextWidget)w)->text.dragState;
// TODO: // TODO: 
// TODO: // TODO:    if (dragState != PRIMARY_BLOCK_DRAG)
// TODO: // TODO:    {
// TODO: // TODO:       moveToAP(w, event, args, nArgs);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    FinishBlockDrag((TextWidget)w);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void endDragAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    if (((TextWidget)w)->text.dragState == PRIMARY_BLOCK_DRAG)
// TODO: // TODO:       FinishBlockDrag((TextWidget)w);
// TODO: // TODO:    else
// TODO: // TODO:       endDrag(w);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void exchangeAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XButtonEvent* e = &event->xbutton;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    selection* sec = &buf->secondary, *primary = &buf->primary;
// TODO: // TODO:    char* primaryText, *secText;
// TODO: // TODO:    int newPrimaryStart, newPrimaryEnd, secWasRect;
// TODO: // TODO:    int dragState = ((TextWidget)w)->text.dragState; /* save before endDrag */
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    endDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO: 
// TODO: // TODO:    /* If there's no secondary selection here, or the primary and secondary
// TODO: // TODO:       selection overlap, just beep and return */
// TODO: // TODO:    if (!sec->selected || (primary->selected &&
// TODO: // TODO:                           ((primary->start <= sec->start && primary->end > sec->start) ||
// TODO: // TODO:                            (sec->start <= primary->start && sec->end > primary->start))))
// TODO: // TODO:    {
// TODO: // TODO:       BufSecondaryUnselect(buf);
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:       /* If there's no secondary selection, but the primary selection is
// TODO: // TODO:          being dragged, we must not forget to finish the dragging.
// TODO: // TODO:          Otherwise, modifications aren't recorded. */
// TODO: // TODO:       if (dragState == PRIMARY_BLOCK_DRAG)
// TODO: // TODO:          FinishBlockDrag((TextWidget)w);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* if the primary selection is in another widget, use selection routines */
// TODO: // TODO:    if (!primary->selected)
// TODO: // TODO:    {
// TODO: // TODO:       ExchangeSelections(w, e->time);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Both primary and secondary are in this widget, do the exchange here */
// TODO: // TODO:    primaryText = BufGetSelectionText(buf);
// TODO: // TODO:    secText = BufGetSecSelectText(buf);
// TODO: // TODO:    secWasRect = sec->rectangular;
// TODO: // TODO:    BufReplaceSecSelect(buf, primaryText);
// TODO: // TODO:    newPrimaryStart = primary->start;
// TODO: // TODO:    BufReplaceSelected(buf, secText);
// TODO: // TODO:    newPrimaryEnd = newPrimaryStart + strlen(secText);
// TODO: // TODO:    XtFree(primaryText);
// TODO: // TODO:    XtFree(secText);
// TODO: // TODO:    BufSecondaryUnselect(buf);
// TODO: // TODO:    if (secWasRect)
// TODO: // TODO:    {
// TODO: // TODO:       TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       BufSelect(buf, newPrimaryStart, newPrimaryEnd);
// TODO: // TODO:       TextDSetInsertPosition(textD, newPrimaryEnd);
// TODO: // TODO:    }
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void copyPrimaryAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                           int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XKeyEvent* e = &event->xkey;
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO:    textDisp* textD = tw->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    selection* primary = &buf->primary;
// TODO: // TODO:    int rectangular = hasKey("rect", args, nArgs);
// TODO: // TODO:    char* textToCopy;
// TODO: // TODO:    int insertPos, col;
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    if (primary->selected && rectangular)
// TODO: // TODO:    {
// TODO: // TODO:       textToCopy = BufGetSelectionText(buf);
// TODO: // TODO:       insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:       col = BufCountDispChars(buf, BufStartOfLine(buf, insertPos), insertPos);
// TODO: // TODO:       BufInsertCol(buf, col, insertPos, textToCopy, NULL, NULL);
// TODO: // TODO:       TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO: // TODO:       XtFree(textToCopy);
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:    }
// TODO: // TODO:    else if (primary->selected)
// TODO: // TODO:    {
// TODO: // TODO:       textToCopy = BufGetSelectionText(buf);
// TODO: // TODO:       insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:       BufInsert(buf, insertPos, textToCopy);
// TODO: // TODO:       TextDSetInsertPosition(textD, insertPos + strlen(textToCopy));
// TODO: // TODO:       XtFree(textToCopy);
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:    }
// TODO: // TODO:    else if (rectangular)
// TODO: // TODO:    {
// TODO: // TODO:       if (!TextDPositionToXY(textD, TextDGetInsertPosition(textD),
// TODO: // TODO:                              &tw->text.btnDownX, &tw->text.btnDownY))
// TODO: // TODO:          return; /* shouldn't happen */
// TODO: // TODO:       InsertPrimarySelection(w, e->time, true);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:       InsertPrimarySelection(w, e->time, false);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void cutPrimaryAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                          int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XKeyEvent* e = &event->xkey;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    selection* primary = &buf->primary;
// TODO: // TODO:    char* textToCopy;
// TODO: // TODO:    int rectangular = hasKey("rect", args, nArgs);
// TODO: // TODO:    int insertPos, col;
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    if (primary->selected && rectangular)
// TODO: // TODO:    {
// TODO: // TODO:       textToCopy = BufGetSelectionText(buf);
// TODO: // TODO:       insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:       col = BufCountDispChars(buf, BufStartOfLine(buf, insertPos), insertPos);
// TODO: // TODO:       BufInsertCol(buf, col, insertPos, textToCopy, NULL, NULL);
// TODO: // TODO:       TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO: // TODO:       XtFree(textToCopy);
// TODO: // TODO:       BufRemoveSelected(buf);
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:    }
// TODO: // TODO:    else if (primary->selected)
// TODO: // TODO:    {
// TODO: // TODO:       textToCopy = BufGetSelectionText(buf);
// TODO: // TODO:       insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:       BufInsert(buf, insertPos, textToCopy);
// TODO: // TODO:       TextDSetInsertPosition(textD, insertPos + strlen(textToCopy));
// TODO: // TODO:       XtFree(textToCopy);
// TODO: // TODO:       BufRemoveSelected(buf);
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:    }
// TODO: // TODO:    else if (rectangular)
// TODO: // TODO:    {
// TODO: // TODO:       if (!TextDPositionToXY(textD, TextDGetInsertPosition(textD),
// TODO: // TODO:                              &((TextWidget)w)->text.btnDownX,
// TODO: // TODO:                              &((TextWidget)w)->text.btnDownY))
// TODO: // TODO:          return; /* shouldn't happen */
// TODO: // TODO:       MovePrimarySelection(w, e->time, true);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       MovePrimarySelection(w, e->time, false);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void mousePanAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XButtonEvent* e = &event->xbutton;
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO:    textDisp* textD = tw->text.textD;
// TODO: // TODO:    int lineHeight = textD->ascent + textD->descent;
// TODO: // TODO:    int topLineNum, horizOffset;
// TODO: // TODO:    static Cursor panCursor = 0;
// TODO: // TODO: 
// TODO: // TODO:    if (tw->text.dragState == MOUSE_PAN)
// TODO: // TODO:    {
// TODO: // TODO:       TextDSetScroll(textD,
// TODO: // TODO:                      (tw->text.btnDownY - e->y + lineHeight/2) / lineHeight,
// TODO: // TODO:                      tw->text.btnDownX - e->x);
// TODO: // TODO:    }
// TODO: // TODO:    else if (tw->text.dragState == NOT_CLICKED)
// TODO: // TODO:    {
// TODO: // TODO:       TextDGetScroll(textD, &topLineNum, &horizOffset);
// TODO: // TODO:       tw->text.btnDownX = e->x + horizOffset;
// TODO: // TODO:       tw->text.btnDownY = e->y + topLineNum * lineHeight;
// TODO: // TODO:       tw->text.dragState = MOUSE_PAN;
// TODO: // TODO:       if (!panCursor)
// TODO: // TODO:          panCursor = XCreateFontCursor(XtDisplay(w), XC_fleur);
// TODO: // TODO:       XGrabPointer(XtDisplay(w), XtWindow(w), false,
// TODO: // TODO:                    ButtonMotionMask | ButtonReleaseMask, GrabModeAsync,
// TODO: // TODO:                    GrabModeAsync, None, panCursor, CurrentTime);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:       cancelDrag(w);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void pasteClipboardAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                              int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    if (hasKey("rect", args, nArgs))
// TODO: // TODO:       TextColPasteClipboard(w, event->xkey.time);
// TODO: // TODO:    else
// TODO: // TODO:       TextPasteClipboard(w, event->xkey.time);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void copyClipboardAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                             int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    TextCopyClipboard(w, event->xkey.time);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void cutClipboardAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                            int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    TextCutClipboard(w, event->xkey.time);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void insertStringAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                            int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    smartIndentCBStruct smartIndent;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO: 
// TODO: // TODO:    if (*nArgs == 0)
// TODO: // TODO:       return;
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    if (((TextWidget)w)->text.smartIndent)
// TODO: // TODO:    {
// TODO: // TODO:       smartIndent.reason = CHAR_TYPED;
// TODO: // TODO:       smartIndent.pos = TextDGetInsertPosition(textD);
// TODO: // TODO:       smartIndent.indentRequest = 0;
// TODO: // TODO:       smartIndent.charsTyped = args[0];
// TODO: // TODO:       XtCallCallbacks(w, textNsmartIndentCallback, (XtPointer)&smartIndent);
// TODO: // TODO:    }
// TODO: // TODO:    TextInsertAtCursor(w, args[0], event, true, true);
// TODO: // TODO:    BufUnselect((((TextWidget)w)->text.textD)->buffer);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void selfInsertAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    WindowInfo* window = WidgetToWindow(w);
// TODO: // TODO: 
// TODO: // TODO: #ifdef NO_XMIM
// TODO: // TODO:    static XComposeStatus compose = {NULL, 0};
// TODO: // TODO: #else
// TODO: // TODO:    int status;
// TODO: // TODO: #endif
// TODO: // TODO:    XKeyEvent* e = &event->xkey;
// TODO: // TODO:    char chars[20];
// TODO: // TODO:    KeySym keysym;
// TODO: // TODO:    int nChars;
// TODO: // TODO:    smartIndentCBStruct smartIndent;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO: 
// TODO: // TODO: #ifdef NO_XMIM
// TODO: // TODO:    nChars = XLookupString(&event->xkey, chars, 19, &keysym, &compose);
// TODO: // TODO:    if (nChars == 0)
// TODO: // TODO:       return;
// TODO: // TODO: #else
// TODO: // TODO:    nChars = XmImMbLookupString(w, &event->xkey, chars, 19, &keysym,
// TODO: // TODO:                                &status);
// TODO: // TODO:    if (nChars == 0 || status == XLookupNone ||
// TODO: // TODO:          status == XLookupKeySym || status == XBufferOverflow)
// TODO: // TODO:       return;
// TODO: // TODO: #endif
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    TakeMotifDestination(w, e->time);
// TODO: // TODO:    chars[nChars] = '\0';
// TODO: // TODO: 
// TODO: // TODO:    if (!BufSubstituteNullChars(chars, nChars, window->buffer))
// TODO: // TODO:    {
// TODO: // TODO:       DialogF(DF_ERR, window->mainWindow, 1, "Error", "Too much binary data", "OK");
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* If smart indent is on, call the smart indent callback to check the
// TODO: // TODO:       inserted character */
// TODO: // TODO:    if (((TextWidget)w)->text.smartIndent)
// TODO: // TODO:    {
// TODO: // TODO:       smartIndent.reason = CHAR_TYPED;
// TODO: // TODO:       smartIndent.pos = TextDGetInsertPosition(textD);
// TODO: // TODO:       smartIndent.indentRequest = 0;
// TODO: // TODO:       smartIndent.charsTyped = chars;
// TODO: // TODO:       XtCallCallbacks(w, textNsmartIndentCallback, (XtPointer)&smartIndent);
// TODO: // TODO:    }
// TODO: // TODO:    TextInsertAtCursor(w, chars, event, true, true);
// TODO: // TODO:    BufUnselect(textD->buffer);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void newlineAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    if (((TextWidget)w)->text.autoIndent || ((TextWidget)w)->text.smartIndent)
// TODO: // TODO:       newlineAndIndentAP(w, event, args, nArgs);
// TODO: // TODO:    else
// TODO: // TODO:       newlineNoIndentAP(w, event, args, nArgs);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void newlineNoIndentAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XKeyEvent* e = &event->xkey;
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    TakeMotifDestination(w, e->time);
// TODO: // TODO:    simpleInsertAtCursor(w, "\n", event, true);
// TODO: // TODO:    BufUnselect((((TextWidget)w)->text.textD)->buffer);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void newlineAndIndentAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XKeyEvent* e = &event->xkey;
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO:    textDisp* textD = tw->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    char* indentStr;
// TODO: // TODO:    int cursorPos, lineStartPos, column;
// TODO: // TODO: 
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    TakeMotifDestination(w, e->time);
// TODO: // TODO: 
// TODO: // TODO:    /* Create a string containing a newline followed by auto or smart
// TODO: // TODO:       indent string */
// TODO: // TODO:    cursorPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    lineStartPos = BufStartOfLine(buf, cursorPos);
// TODO: // TODO:    indentStr = createIndentString(tw, buf, 0, lineStartPos,
// TODO: // TODO:                                   cursorPos, NULL, &column);
// TODO: // TODO: 
// TODO: // TODO:    /* Insert it at the cursor */
// TODO: // TODO:    simpleInsertAtCursor(w, indentStr, event, true);
// TODO: // TODO:    XtFree(indentStr);
// TODO: // TODO: 
// TODO: // TODO:    if (tw->text.emulateTabs > 0)
// TODO: // TODO:    {
// TODO: // TODO:       /*  If emulated tabs are on, make the inserted indent deletable by
// TODO: // TODO:           tab. Round this up by faking the column a bit to the right to
// TODO: // TODO:           let the user delete half-tabs with one keypress.  */
// TODO: // TODO:       column += tw->text.emulateTabs - 1;
// TODO: // TODO:       tw->text.emTabsBeforeCursor = column / tw->text.emulateTabs;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    BufUnselect(buf);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void processTabAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                          int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    selection* sel = &buf->primary;
// TODO: // TODO:    int emTabDist = ((TextWidget)w)->text.emulateTabs;
// TODO: // TODO:    int emTabsBeforeCursor = ((TextWidget)w)->text.emTabsBeforeCursor;
// TODO: // TODO:    int insertPos, indent, startIndent, toIndent, lineStart, tabWidth;
// TODO: // TODO:    char* outStr, *outPtr;
// TODO: // TODO: 
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    TakeMotifDestination(w, event->xkey.time);
// TODO: // TODO: 
// TODO: // TODO:    /* If emulated tabs are off, just insert a tab */
// TODO: // TODO:    if (emTabDist <= 0)
// TODO: // TODO:    {
// TODO: // TODO:       TextInsertAtCursor(w, "\t", event, true, true);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Find the starting and ending indentation.  If the tab is to
// TODO: // TODO:       replace an existing selection, use the start of the selection
// TODO: // TODO:       instead of the cursor position as the indent.  When replacing
// TODO: // TODO:       rectangular selections, tabs are automatically recalculated as
// TODO: // TODO:       if the inserted text began at the start of the line */
// TODO: // TODO:    insertPos = pendingSelection(w) ?
// TODO: // TODO:                sel->start : TextDGetInsertPosition(textD);
// TODO: // TODO:    lineStart = BufStartOfLine(buf, insertPos);
// TODO: // TODO:    if (pendingSelection(w) && sel->rectangular)
// TODO: // TODO:       insertPos = BufCountForwardDispChars(buf, lineStart, sel->rectStart);
// TODO: // TODO:    startIndent = BufCountDispChars(buf, lineStart, insertPos);
// TODO: // TODO:    toIndent = startIndent + emTabDist - (startIndent % emTabDist);
// TODO: // TODO:    if (pendingSelection(w) && sel->rectangular)
// TODO: // TODO:    {
// TODO: // TODO:       toIndent -= startIndent;
// TODO: // TODO:       startIndent = 0;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Allocate a buffer assuming all the inserted characters will be spaces */
// TODO: // TODO:    outStr = malloc__(toIndent - startIndent + 1);
// TODO: // TODO: 
// TODO: // TODO:    /* Add spaces and tabs to outStr until it reaches toIndent */
// TODO: // TODO:    outPtr = outStr;
// TODO: // TODO:    indent = startIndent;
// TODO: // TODO:    while (indent < toIndent)
// TODO: // TODO:    {
// TODO: // TODO:       tabWidth = BufCharWidth('\t', indent, buf->tabDist, buf->nullSubsChar);
// TODO: // TODO:       if (buf->useTabs && tabWidth > 1 && indent + tabWidth <= toIndent)
// TODO: // TODO:       {
// TODO: // TODO:          *outPtr++ = '\t';
// TODO: // TODO:          indent += tabWidth;
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          *outPtr++ = ' ';
// TODO: // TODO:          indent++;
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO:    *outPtr = '\0';
// TODO: // TODO: 
// TODO: // TODO:    /* Insert the emulated tab */
// TODO: // TODO:    TextInsertAtCursor(w, outStr, event, true, true);
// TODO: // TODO:    XtFree(outStr);
// TODO: // TODO: 
// TODO: // TODO:    /* Restore and ++ emTabsBeforeCursor cleared by TextInsertAtCursor */
// TODO: // TODO:    ((TextWidget)w)->text.emTabsBeforeCursor = emTabsBeforeCursor + 1;
// TODO: // TODO: 
// TODO: // TODO:    BufUnselect(buf);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void deleteSelectionAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XKeyEvent* e = &event->xkey;
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    TakeMotifDestination(w, e->time);
// TODO: // TODO:    deletePendingSelection(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void deletePreviousCharacterAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                       int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XKeyEvent* e = &event->xkey;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    char c;
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO: 
// TODO: // TODO:    TakeMotifDestination(w, e->time);
// TODO: // TODO:    if (deletePendingSelection(w, event))
// TODO: // TODO:       return;
// TODO: // TODO: 
// TODO: // TODO:    if (insertPos == 0)
// TODO: // TODO:    {
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    if (deleteEmulatedTab(w, event))
// TODO: // TODO:       return;
// TODO: // TODO: 
// TODO: // TODO:    if (((TextWidget)w)->text.overstrike)
// TODO: // TODO:    {
// TODO: // TODO:       c = BufGetCharacter(textD->buffer, insertPos - 1);
// TODO: // TODO:       if (c == '\n')
// TODO: // TODO:          BufRemove(textD->buffer, insertPos - 1, insertPos);
// TODO: // TODO:       else if (c != '\t')
// TODO: // TODO:          BufReplace(textD->buffer, insertPos - 1, insertPos, " ");
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       BufRemove(textD->buffer, insertPos - 1, insertPos);
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    TextDSetInsertPosition(textD, insertPos - 1);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void deleteNextCharacterAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                   int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XKeyEvent* e = &event->xkey;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    TakeMotifDestination(w, e->time);
// TODO: // TODO:    if (deletePendingSelection(w, event))
// TODO: // TODO:       return;
// TODO: // TODO:    if (insertPos == textD->buffer->length)
// TODO: // TODO:    {
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO:    BufRemove(textD->buffer, insertPos , insertPos + 1);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void deletePreviousWordAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                  int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XKeyEvent* e = &event->xkey;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    int pos, lineStart = BufStartOfLine(textD->buffer, insertPos);
// TODO: // TODO:    char* delimiters = ((TextWidget)w)->text.delimiters;
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:    {
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    TakeMotifDestination(w, e->time);
// TODO: // TODO:    if (deletePendingSelection(w, event))
// TODO: // TODO:    {
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    if (insertPos == lineStart)
// TODO: // TODO:    {
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    pos = max(insertPos - 1, 0);
// TODO: // TODO:    while (strchr(delimiters, BufGetCharacter(textD->buffer, pos)) != NULL &&
// TODO: // TODO:           pos != lineStart)
// TODO: // TODO:    {
// TODO: // TODO:       pos--;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    pos = startOfWord((TextWidget)w, pos);
// TODO: // TODO:    BufRemove(textD->buffer, pos, insertPos);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void deleteNextWordAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                              int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XKeyEvent* e = &event->xkey;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    int pos, lineEnd = BufEndOfLine(textD->buffer, insertPos);
// TODO: // TODO:    char* delimiters = ((TextWidget)w)->text.delimiters;
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:    {
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    TakeMotifDestination(w, e->time);
// TODO: // TODO:    if (deletePendingSelection(w, event))
// TODO: // TODO:    {
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    if (insertPos == lineEnd)
// TODO: // TODO:    {
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    pos = insertPos;
// TODO: // TODO:    while (strchr(delimiters, BufGetCharacter(textD->buffer, pos)) != NULL &&
// TODO: // TODO:           pos != lineEnd)
// TODO: // TODO:    {
// TODO: // TODO:       pos++;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    pos = endOfWord((TextWidget)w, pos);
// TODO: // TODO:    BufRemove(textD->buffer, insertPos, pos);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void deleteToEndOfLineAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                 int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XKeyEvent* e = &event->xkey;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    int endOfLine;
// TODO: // TODO:    int silent = 0;
// TODO: // TODO: 
// TODO: // TODO:    silent = hasKey("nobell", args, nArgs);
// TODO: // TODO:    if (hasKey("absolute", args, nArgs))
// TODO: // TODO:       endOfLine = BufEndOfLine(textD->buffer, insertPos);
// TODO: // TODO:    else
// TODO: // TODO:       endOfLine = TextDEndOfLine(textD, insertPos, false);
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    TakeMotifDestination(w, e->time);
// TODO: // TODO:    if (deletePendingSelection(w, event))
// TODO: // TODO:       return;
// TODO: // TODO:    if (insertPos == endOfLine)
// TODO: // TODO:    {
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO:    BufRemove(textD->buffer, insertPos, endOfLine);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void deleteToStartOfLineAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                   int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    XKeyEvent* e = &event->xkey;
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    int startOfLine;
// TODO: // TODO:    int silent = 0;
// TODO: // TODO: 
// TODO: // TODO:    silent = hasKey("nobell", args, nArgs);
// TODO: // TODO:    if (hasKey("wrap", args, nArgs))
// TODO: // TODO:       startOfLine = TextDStartOfLine(textD, insertPos);
// TODO: // TODO:    else
// TODO: // TODO:       startOfLine = BufStartOfLine(textD->buffer, insertPos);
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (checkReadOnly(w))
// TODO: // TODO:       return;
// TODO: // TODO:    TakeMotifDestination(w, e->time);
// TODO: // TODO:    if (deletePendingSelection(w, event))
// TODO: // TODO:       return;
// TODO: // TODO:    if (insertPos == startOfLine)
// TODO: // TODO:    {
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO:    BufRemove(textD->buffer, startOfLine, insertPos);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void forwardCharacterAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (!TextDMoveRight(((TextWidget)w)->text.textD))
// TODO: // TODO:    {
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:    }
// TODO: // TODO:    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void backwardCharacterAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                 int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (!TextDMoveLeft(((TextWidget)w)->text.textD))
// TODO: // TODO:    {
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:    }
// TODO: // TODO:    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void forwardWordAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                           int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    int pos, insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    char* delimiters = ((TextWidget)w)->text.delimiters;
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (insertPos == buf->length)
// TODO: // TODO:    {
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO:    pos = insertPos;
// TODO: // TODO: 
// TODO: // TODO:    if (hasKey("tail", args, nArgs))
// TODO: // TODO:    {
// TODO: // TODO:       for (; pos<buf->length; pos++)
// TODO: // TODO:       {
// TODO: // TODO:          if (NULL == strchr(delimiters, BufGetCharacter(buf, pos)))
// TODO: // TODO:          {
// TODO: // TODO:             break;
// TODO: // TODO:          }
// TODO: // TODO:       }
// TODO: // TODO:       if (NULL == strchr(delimiters, BufGetCharacter(buf, pos)))
// TODO: // TODO:       {
// TODO: // TODO:          pos = endOfWord((TextWidget)w, pos);
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       if (NULL == strchr(delimiters, BufGetCharacter(buf, pos)))
// TODO: // TODO:       {
// TODO: // TODO:          pos = endOfWord((TextWidget)w, pos);
// TODO: // TODO:       }
// TODO: // TODO:       for (; pos<buf->length; pos++)
// TODO: // TODO:       {
// TODO: // TODO:          if (NULL == strchr(delimiters, BufGetCharacter(buf, pos)))
// TODO: // TODO:          {
// TODO: // TODO:             break;
// TODO: // TODO:          }
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    TextDSetInsertPosition(textD, pos);
// TODO: // TODO:    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void backwardWordAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                            int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    int pos, insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    char* delimiters = ((TextWidget)w)->text.delimiters;
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (insertPos == 0)
// TODO: // TODO:    {
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO:    pos = max(insertPos - 1, 0);
// TODO: // TODO:    while (strchr(delimiters, BufGetCharacter(buf, pos)) != NULL && pos > 0)
// TODO: // TODO:       pos--;
// TODO: // TODO:    pos = startOfWord((TextWidget)w, pos);
// TODO: // TODO: 
// TODO: // TODO:    TextDSetInsertPosition(textD, pos);
// TODO: // TODO:    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void forwardParagraphAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int pos, insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    char c;
// TODO: // TODO:    static char whiteChars[] = " \t";
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (insertPos == buf->length)
// TODO: // TODO:    {
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO:    pos = min(BufEndOfLine(buf, insertPos)+1, buf->length);
// TODO: // TODO:    while (pos < buf->length)
// TODO: // TODO:    {
// TODO: // TODO:       c = BufGetCharacter(buf, pos);
// TODO: // TODO:       if (c == '\n')
// TODO: // TODO:          break;
// TODO: // TODO:       if (strchr(whiteChars, c) != NULL)
// TODO: // TODO:          pos++;
// TODO: // TODO:       else
// TODO: // TODO:          pos = min(BufEndOfLine(buf, pos)+1, buf->length);
// TODO: // TODO:    }
// TODO: // TODO:    TextDSetInsertPosition(textD, min(pos+1, buf->length));
// TODO: // TODO:    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void backwardParagraphAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                 int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int parStart, pos, insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    char c;
// TODO: // TODO:    static char whiteChars[] = " \t";
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (insertPos == 0)
// TODO: // TODO:    {
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO:    parStart = BufStartOfLine(buf, max(insertPos-1, 0));
// TODO: // TODO:    pos = max(parStart - 2, 0);
// TODO: // TODO:    while (pos > 0)
// TODO: // TODO:    {
// TODO: // TODO:       c = BufGetCharacter(buf, pos);
// TODO: // TODO:       if (c == '\n')
// TODO: // TODO:          break;
// TODO: // TODO:       if (strchr(whiteChars, c) != NULL)
// TODO: // TODO:          pos--;
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          parStart = BufStartOfLine(buf, pos);
// TODO: // TODO:          pos = max(parStart - 2, 0);
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO:    TextDSetInsertPosition(textD, parStart);
// TODO: // TODO:    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void keySelectAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int stat, insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (hasKey("left", args, nArgs)) stat = TextDMoveLeft(textD);
// TODO: // TODO:    else if (hasKey("right", args, nArgs)) stat = TextDMoveRight(textD);
// TODO: // TODO:    else if (hasKey("up", args, nArgs)) stat = TextDMoveUp(textD, 0);
// TODO: // TODO:    else if (hasKey("down", args, nArgs)) stat = TextDMoveDown(textD, 0);
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       keyMoveExtendSelection(w, event, insertPos, hasKey("rect", args,nArgs));
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO:    if (!stat)
// TODO: // TODO:    {
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       keyMoveExtendSelection(w, event, insertPos, hasKey("rect", args,nArgs));
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:       callCursorMovementCBs(w, event);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void processUpAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO:    int abs = hasKey("absolute", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (!TextDMoveUp(((TextWidget)w)->text.textD, abs))
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void processShiftUpAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                              int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO:    int abs = hasKey("absolute", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (!TextDMoveUp(((TextWidget)w)->text.textD, abs))
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:    keyMoveExtendSelection(w, event, insertPos, hasKey("rect", args, nArgs));
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void processDownAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                           int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO:    int abs = hasKey("absolute", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (!TextDMoveDown(((TextWidget)w)->text.textD, abs))
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void processShiftDownAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO:    int abs = hasKey("absolute", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (!TextDMoveDown(((TextWidget)w)->text.textD, abs))
// TODO: // TODO:       ringIfNecessary(silent, w);
// TODO: // TODO:    keyMoveExtendSelection(w, event, insertPos, hasKey("rect", args, nArgs));
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void beginningOfLineAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (hasKey("absolute", args, nArgs))
// TODO: // TODO:       TextDSetInsertPosition(textD, BufStartOfLine(textD->buffer, insertPos));
// TODO: // TODO:    else
// TODO: // TODO:       TextDSetInsertPosition(textD, TextDStartOfLine(textD, insertPos));
// TODO: // TODO:    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO:    textD->cursorPreferredCol = 0;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void endOfLineAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (hasKey("absolute", args, nArgs))
// TODO: // TODO:       TextDSetInsertPosition(textD, BufEndOfLine(textD->buffer, insertPos));
// TODO: // TODO:    else
// TODO: // TODO:       TextDSetInsertPosition(textD, TextDEndOfLine(textD, insertPos, false));
// TODO: // TODO:    checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO:    textD->cursorPreferredCol = -1;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void beginningOfFileAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                               int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (hasKey("scrollbar", args, nArgs))
// TODO: // TODO:    {
// TODO: // TODO:       if (textD->topLineNum != 1)
// TODO: // TODO:       {
// TODO: // TODO:          TextDSetScroll(textD, 1, textD->horizOffset);
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       TextDSetInsertPosition(((TextWidget)w)->text.textD, 0);
// TODO: // TODO:       checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:       callCursorMovementCBs(w, event);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void endOfFileAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    int lastTopLine;
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (hasKey("scrollbar", args, nArgs))
// TODO: // TODO:    {
// TODO: // TODO:       lastTopLine = max(1,
// TODO: // TODO:                         textD->nBufferLines - (textD->nVisibleLines - 2) +
// TODO: // TODO:                         ((TextWidget)w)->text.cursorVPadding);
// TODO: // TODO:       if (lastTopLine != textD->topLineNum)
// TODO: // TODO:       {
// TODO: // TODO:          TextDSetScroll(textD, lastTopLine, textD->horizOffset);
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       TextDSetInsertPosition(textD, textD->buffer->length);
// TODO: // TODO:       checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:       callCursorMovementCBs(w, event);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void nextPageAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    int lastTopLine = max(1,
// TODO: // TODO:                          textD->nBufferLines - (textD->nVisibleLines - 2) +
// TODO: // TODO:                          ((TextWidget)w)->text.cursorVPadding);
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    int column = 0, visLineNum, lineStartPos;
// TODO: // TODO:    int pos, targetLine;
// TODO: // TODO:    int pageForwardCount = max(1, textD->nVisibleLines - 1);
// TODO: // TODO:    int maintainColumn = 0;
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    maintainColumn = hasKey("column", args, nArgs);
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (hasKey("scrollbar", args, nArgs))   /* scrollbar only */
// TODO: // TODO:    {
// TODO: // TODO:       targetLine = min(textD->topLineNum + pageForwardCount, lastTopLine);
// TODO: // TODO: 
// TODO: // TODO:       if (targetLine == textD->topLineNum)
// TODO: // TODO:       {
// TODO: // TODO:          ringIfNecessary(silent, w);
// TODO: // TODO:          return;
// TODO: // TODO:       }
// TODO: // TODO:       TextDSetScroll(textD, targetLine, textD->horizOffset);
// TODO: // TODO:    }
// TODO: // TODO:    else if (hasKey("stutter", args, nArgs))   /* Mac style */
// TODO: // TODO:    {
// TODO: // TODO:       /* move to bottom line of visible area */
// TODO: // TODO:       /* if already there, page down maintaining preferrred column */
// TODO: // TODO:       targetLine = max(min(textD->nVisibleLines - 1, textD->nBufferLines), 0);
// TODO: // TODO:       column = TextDPreferredColumn(textD, &visLineNum, &lineStartPos);
// TODO: // TODO:       if (lineStartPos == textD->lineStarts[targetLine])
// TODO: // TODO:       {
// TODO: // TODO:          if (insertPos >= buf->length || textD->topLineNum == lastTopLine)
// TODO: // TODO:          {
// TODO: // TODO:             ringIfNecessary(silent, w);
// TODO: // TODO:             return;
// TODO: // TODO:          }
// TODO: // TODO:          targetLine = min(textD->topLineNum + pageForwardCount, lastTopLine);
// TODO: // TODO:          pos = TextDCountForwardNLines(textD, insertPos, pageForwardCount, false);
// TODO: // TODO:          if (maintainColumn)
// TODO: // TODO:          {
// TODO: // TODO:             pos = TextDPosOfPreferredCol(textD, column, pos);
// TODO: // TODO:          }
// TODO: // TODO:          TextDSetInsertPosition(textD, pos);
// TODO: // TODO:          TextDSetScroll(textD, targetLine, textD->horizOffset);
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          pos = textD->lineStarts[targetLine];
// TODO: // TODO:          while (targetLine > 0 && pos == -1)
// TODO: // TODO:          {
// TODO: // TODO:             --targetLine;
// TODO: // TODO:             pos = textD->lineStarts[targetLine];
// TODO: // TODO:          }
// TODO: // TODO:          if (lineStartPos == pos)
// TODO: // TODO:          {
// TODO: // TODO:             ringIfNecessary(silent, w);
// TODO: // TODO:             return;
// TODO: // TODO:          }
// TODO: // TODO:          if (maintainColumn)
// TODO: // TODO:          {
// TODO: // TODO:             pos = TextDPosOfPreferredCol(textD, column, pos);
// TODO: // TODO:          }
// TODO: // TODO:          TextDSetInsertPosition(textD, pos);
// TODO: // TODO:       }
// TODO: // TODO:       checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:       callCursorMovementCBs(w, event);
// TODO: // TODO:       if (maintainColumn)
// TODO: // TODO:       {
// TODO: // TODO:          textD->cursorPreferredCol = column;
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          textD->cursorPreferredCol = -1;
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO:    else   /* "standard" */
// TODO: // TODO:    {
// TODO: // TODO:       if (insertPos >= buf->length && textD->topLineNum == lastTopLine)
// TODO: // TODO:       {
// TODO: // TODO:          ringIfNecessary(silent, w);
// TODO: // TODO:          return;
// TODO: // TODO:       }
// TODO: // TODO:       if (maintainColumn)
// TODO: // TODO:       {
// TODO: // TODO:          column = TextDPreferredColumn(textD, &visLineNum, &lineStartPos);
// TODO: // TODO:       }
// TODO: // TODO:       targetLine = textD->topLineNum + textD->nVisibleLines - 1;
// TODO: // TODO:       if (targetLine < 1) targetLine = 1;
// TODO: // TODO:       if (targetLine > lastTopLine) targetLine = lastTopLine;
// TODO: // TODO:       pos = TextDCountForwardNLines(textD, insertPos, textD->nVisibleLines-1, false);
// TODO: // TODO:       if (maintainColumn)
// TODO: // TODO:       {
// TODO: // TODO:          pos = TextDPosOfPreferredCol(textD, column, pos);
// TODO: // TODO:       }
// TODO: // TODO:       TextDSetInsertPosition(textD, pos);
// TODO: // TODO:       TextDSetScroll(textD, targetLine, textD->horizOffset);
// TODO: // TODO:       checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:       callCursorMovementCBs(w, event);
// TODO: // TODO:       if (maintainColumn)
// TODO: // TODO:       {
// TODO: // TODO:          textD->cursorPreferredCol = column;
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          textD->cursorPreferredCol = -1;
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void previousPageAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                            int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    int column = 0, visLineNum, lineStartPos;
// TODO: // TODO:    int pos, targetLine;
// TODO: // TODO:    int pageBackwardCount = max(1, textD->nVisibleLines - 1);
// TODO: // TODO:    int maintainColumn = 0;
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    maintainColumn = hasKey("column", args, nArgs);
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (hasKey("scrollbar", args, nArgs))   /* scrollbar only */
// TODO: // TODO:    {
// TODO: // TODO:       targetLine = max(textD->topLineNum - pageBackwardCount, 1);
// TODO: // TODO: 
// TODO: // TODO:       if (targetLine == textD->topLineNum)
// TODO: // TODO:       {
// TODO: // TODO:          ringIfNecessary(silent, w);
// TODO: // TODO:          return;
// TODO: // TODO:       }
// TODO: // TODO:       TextDSetScroll(textD, targetLine, textD->horizOffset);
// TODO: // TODO:    }
// TODO: // TODO:    else if (hasKey("stutter", args, nArgs))   /* Mac style */
// TODO: // TODO:    {
// TODO: // TODO:       /* move to top line of visible area */
// TODO: // TODO:       /* if already there, page up maintaining preferrred column if required */
// TODO: // TODO:       targetLine = 0;
// TODO: // TODO:       column = TextDPreferredColumn(textD, &visLineNum, &lineStartPos);
// TODO: // TODO:       if (lineStartPos == textD->lineStarts[targetLine])
// TODO: // TODO:       {
// TODO: // TODO:          if (textD->topLineNum == 1 && (maintainColumn || column == 0))
// TODO: // TODO:          {
// TODO: // TODO:             ringIfNecessary(silent, w);
// TODO: // TODO:             return;
// TODO: // TODO:          }
// TODO: // TODO:          targetLine = max(textD->topLineNum - pageBackwardCount, 1);
// TODO: // TODO:          pos = TextDCountBackwardNLines(textD, insertPos, pageBackwardCount);
// TODO: // TODO:          if (maintainColumn)
// TODO: // TODO:          {
// TODO: // TODO:             pos = TextDPosOfPreferredCol(textD, column, pos);
// TODO: // TODO:          }
// TODO: // TODO:          TextDSetInsertPosition(textD, pos);
// TODO: // TODO:          TextDSetScroll(textD, targetLine, textD->horizOffset);
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          pos = textD->lineStarts[targetLine];
// TODO: // TODO:          if (maintainColumn)
// TODO: // TODO:          {
// TODO: // TODO:             pos = TextDPosOfPreferredCol(textD, column, pos);
// TODO: // TODO:          }
// TODO: // TODO:          TextDSetInsertPosition(textD, pos);
// TODO: // TODO:       }
// TODO: // TODO:       checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:       callCursorMovementCBs(w, event);
// TODO: // TODO:       if (maintainColumn)
// TODO: // TODO:       {
// TODO: // TODO:          textD->cursorPreferredCol = column;
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          textD->cursorPreferredCol = -1;
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO:    else   /* "standard" */
// TODO: // TODO:    {
// TODO: // TODO:       if (insertPos <= 0 && textD->topLineNum == 1)
// TODO: // TODO:       {
// TODO: // TODO:          ringIfNecessary(silent, w);
// TODO: // TODO:          return;
// TODO: // TODO:       }
// TODO: // TODO:       if (maintainColumn)
// TODO: // TODO:       {
// TODO: // TODO:          column = TextDPreferredColumn(textD, &visLineNum, &lineStartPos);
// TODO: // TODO:       }
// TODO: // TODO:       targetLine = textD->topLineNum - (textD->nVisibleLines - 1);
// TODO: // TODO:       if (targetLine < 1) targetLine = 1;
// TODO: // TODO:       pos = TextDCountBackwardNLines(textD, insertPos, textD->nVisibleLines-1);
// TODO: // TODO:       if (maintainColumn)
// TODO: // TODO:       {
// TODO: // TODO:          pos = TextDPosOfPreferredCol(textD, column, pos);
// TODO: // TODO:       }
// TODO: // TODO:       TextDSetInsertPosition(textD, pos);
// TODO: // TODO:       TextDSetScroll(textD, targetLine, textD->horizOffset);
// TODO: // TODO:       checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:       callCursorMovementCBs(w, event);
// TODO: // TODO:       if (maintainColumn)
// TODO: // TODO:       {
// TODO: // TODO:          textD->cursorPreferredCol = column;
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          textD->cursorPreferredCol = -1;
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void pageLeftAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    int maxCharWidth = textD->fontStruct->max_bounds.width;
// TODO: // TODO:    int lineStartPos, indent, pos;
// TODO: // TODO:    int horizOffset;
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (hasKey("scrollbar", args, nArgs))
// TODO: // TODO:    {
// TODO: // TODO:       if (textD->horizOffset == 0)
// TODO: // TODO:       {
// TODO: // TODO:          ringIfNecessary(silent, w);
// TODO: // TODO:          return;
// TODO: // TODO:       }
// TODO: // TODO:       horizOffset = max(0, textD->horizOffset - textD->width);
// TODO: // TODO:       TextDSetScroll(textD, textD->topLineNum, horizOffset);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       lineStartPos = BufStartOfLine(buf, insertPos);
// TODO: // TODO:       if (insertPos == lineStartPos && textD->horizOffset == 0)
// TODO: // TODO:       {
// TODO: // TODO:          ringIfNecessary(silent, w);
// TODO: // TODO:          return;
// TODO: // TODO:       }
// TODO: // TODO:       indent = BufCountDispChars(buf, lineStartPos, insertPos);
// TODO: // TODO:       pos = BufCountForwardDispChars(buf, lineStartPos,
// TODO: // TODO:                                      max(0, indent - textD->width / maxCharWidth));
// TODO: // TODO:       TextDSetInsertPosition(textD, pos);
// TODO: // TODO:       TextDSetScroll(textD, textD->topLineNum,
// TODO: // TODO:                      max(0, textD->horizOffset - textD->width));
// TODO: // TODO:       checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:       callCursorMovementCBs(w, event);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void pageRightAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    int maxCharWidth = textD->fontStruct->max_bounds.width;
// TODO: // TODO:    int oldHorizOffset = textD->horizOffset;
// TODO: // TODO:    int lineStartPos, indent, pos;
// TODO: // TODO:    int horizOffset, sliderSize, sliderMax;
// TODO: // TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    if (hasKey("scrollbar", args, nArgs))
// TODO: // TODO:    {
// TODO: // TODO:       XtVaGetValues(textD->hScrollBar, XmNmaximum, &sliderMax,
// TODO: // TODO:                     XmNsliderSize, &sliderSize, NULL);
// TODO: // TODO:       horizOffset = min(textD->horizOffset + textD->width, sliderMax - sliderSize);
// TODO: // TODO:       if (textD->horizOffset == horizOffset)
// TODO: // TODO:       {
// TODO: // TODO:          ringIfNecessary(silent, w);
// TODO: // TODO:          return;
// TODO: // TODO:       }
// TODO: // TODO:       TextDSetScroll(textD, textD->topLineNum, horizOffset);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       lineStartPos = BufStartOfLine(buf, insertPos);
// TODO: // TODO:       indent = BufCountDispChars(buf, lineStartPos, insertPos);
// TODO: // TODO:       pos = BufCountForwardDispChars(buf, lineStartPos,
// TODO: // TODO:                                      indent + textD->width / maxCharWidth);
// TODO: // TODO:       TextDSetInsertPosition(textD, pos);
// TODO: // TODO:       TextDSetScroll(textD, textD->topLineNum, textD->horizOffset + textD->width);
// TODO: // TODO:       if (textD->horizOffset == oldHorizOffset && insertPos == pos)
// TODO: // TODO:          ringIfNecessary(silent, w);
// TODO: // TODO:       checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:       callCursorMovementCBs(w, event);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void toggleOverstrikeAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                                int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO: 
// TODO: // TODO:    if (tw->text.overstrike)
// TODO: // TODO:    {
// TODO: // TODO:       tw->text.overstrike = false;
// TODO: // TODO:       TextDSetCursorStyle(tw->text.textD,
// TODO: // TODO:                           tw->text.heavyCursor ? HEAVY_CURSOR : NORMAL_CURSOR);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       tw->text.overstrike = true;
// TODO: // TODO:       if (tw->text.textD->cursorStyle == NORMAL_CURSOR ||
// TODO: // TODO:             tw->text.textD->cursorStyle == HEAVY_CURSOR)
// TODO: // TODO:          TextDSetCursorStyle(tw->text.textD, BLOCK_CURSOR);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void scrollUpAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                        int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int topLineNum, horizOffset, nLines;
// TODO: // TODO: 
// TODO: // TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &nLines) != 1)
// TODO: // TODO:       return;
// TODO: // TODO:    if (*nArgs == 2)
// TODO: // TODO:    {
// TODO: // TODO:       /* Allow both 'page' and 'pages' */
// TODO: // TODO:       if (strncmp(args[1], "page", 4) == 0)
// TODO: // TODO:          nLines *= textD->nVisibleLines;
// TODO: // TODO: 
// TODO: // TODO:       /* 'line' or 'lines' is the only other valid possibility */
// TODO: // TODO:       else if (strncmp(args[1], "line", 4) != 0)
// TODO: // TODO:          return;
// TODO: // TODO:    }
// TODO: // TODO:    TextDGetScroll(textD, &topLineNum, &horizOffset);
// TODO: // TODO:    TextDSetScroll(textD, topLineNum-nLines, horizOffset);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void scrollDownAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                          int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int topLineNum, horizOffset, nLines;
// TODO: // TODO: 
// TODO: // TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &nLines) != 1)
// TODO: // TODO:       return;
// TODO: // TODO:    if (*nArgs == 2)
// TODO: // TODO:    {
// TODO: // TODO:       /* Allow both 'page' and 'pages' */
// TODO: // TODO:       if (strncmp(args[1], "page", 4) == 0)
// TODO: // TODO:          nLines *= textD->nVisibleLines;
// TODO: // TODO: 
// TODO: // TODO:       /* 'line' or 'lines' is the only other valid possibility */
// TODO: // TODO:       else if (strncmp(args[1], "line", 4) != 0)
// TODO: // TODO:          return;
// TODO: // TODO:    }
// TODO: // TODO:    TextDGetScroll(textD, &topLineNum, &horizOffset);
// TODO: // TODO:    TextDSetScroll(textD, topLineNum+nLines, horizOffset);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void scrollLeftAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                          int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int horizOffset, nPixels;
// TODO: // TODO:    int sliderMax, sliderSize;
// TODO: // TODO: 
// TODO: // TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &nPixels) != 1)
// TODO: // TODO:       return;
// TODO: // TODO:    XtVaGetValues(textD->hScrollBar, XmNmaximum, &sliderMax,
// TODO: // TODO:                  XmNsliderSize, &sliderSize, NULL);
// TODO: // TODO:    horizOffset = min(max(0, textD->horizOffset - nPixels), sliderMax - sliderSize);
// TODO: // TODO:    if (textD->horizOffset != horizOffset)
// TODO: // TODO:    {
// TODO: // TODO:       TextDSetScroll(textD, textD->topLineNum, horizOffset);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void scrollRightAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                           int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int horizOffset, nPixels;
// TODO: // TODO:    int sliderMax, sliderSize;
// TODO: // TODO: 
// TODO: // TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &nPixels) != 1)
// TODO: // TODO:       return;
// TODO: // TODO:    XtVaGetValues(textD->hScrollBar, XmNmaximum, &sliderMax,
// TODO: // TODO:                  XmNsliderSize, &sliderSize, NULL);
// TODO: // TODO:    horizOffset = min(max(0, textD->horizOffset + nPixels), sliderMax - sliderSize);
// TODO: // TODO:    if (textD->horizOffset != horizOffset)
// TODO: // TODO:    {
// TODO: // TODO:       TextDSetScroll(textD, textD->topLineNum, horizOffset);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void scrollToLineAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                            int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int topLineNum, horizOffset, lineNum;
// TODO: // TODO: 
// TODO: // TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &lineNum) != 1)
// TODO: // TODO:       return;
// TODO: // TODO:    TextDGetScroll(textD, &topLineNum, &horizOffset);
// TODO: // TODO:    TextDSetScroll(textD, lineNum, horizOffset);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void selectAllAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                         int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textBuffer* buf = ((TextWidget)w)->text.textD->buffer;
// TODO: // TODO: 
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    BufSelect(buf, 0, buf->length);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void deselectAllAP(Fl_Widget* w, int event, const char** args,
// TODO: // TODO:                           int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    cancelDrag(w);
// TODO: // TODO:    BufUnselect(((TextWidget)w)->text.textD->buffer);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: **  Called on the Intrinsic FocusIn event.
// TODO: // TODO: **
// TODO: // TODO: **  Note that the widget has no internal state about the focus, ie. it does
// TODO: // TODO: **  not know whether it has the focus or not.
// TODO: // TODO: */
// TODO: // TODO: static void focusInAP(Fl_Widget* widget, int event, const char** unused1,
// TODO: // TODO:                       int* unused2)
// TODO: // TODO: {
// TODO: // TODO:    TextWidget textwidget = (TextWidget) widget;
// TODO: // TODO:    textDisp* textD = textwidget->text.textD;
// TODO: // TODO: 
// TODO: // TODO:    /* I don't entirely understand the traversal mechanism in Motif widgets,
// TODO: // TODO:       particularly, what leads to this widget getting a focus-in event when
// TODO: // TODO:       it does not actually have the input focus.  The temporary solution is
// TODO: // TODO:       to do the comparison below, and not show the cursor when Motif says
// TODO: // TODO:       we don't have focus, but keep looking for the real answer */
// TODO: // TODO: #if XmVersion >= 1002
// TODO: // TODO:    if (widget != XmGetFocusWidget(widget))
// TODO: // TODO:       return;
// TODO: // TODO: #endif
// TODO: // TODO: 
// TODO: // TODO:    /* If the timer is not already started, start it */
// TODO: // TODO:    if (textwidget->text.cursorBlinkRate != 0
// TODO: // TODO:          && textwidget->text.cursorBlinkProcID == 0)
// TODO: // TODO:    {
// TODO: // TODO:       textwidget->text.cursorBlinkProcID
// TODO: // TODO:          = XtAppAddTimeOut(XtWidgetToApplicationContext(widget),
// TODO: // TODO:                            textwidget->text.cursorBlinkRate, cursorBlinkTimerProc,
// TODO: // TODO:                            widget);
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Change the cursor to active style */
// TODO: // TODO:    if (textwidget->text.overstrike)
// TODO: // TODO:       TextDSetCursorStyle(textD, BLOCK_CURSOR);
// TODO: // TODO:    else
// TODO: // TODO:       TextDSetCursorStyle(textD, (textwidget->text.heavyCursor
// TODO: // TODO:                                   ? HEAVY_CURSOR
// TODO: // TODO:                                   : NORMAL_CURSOR));
// TODO: // TODO:    TextDUnblankCursor(textD);
// TODO: // TODO: 
// TODO: // TODO: #ifndef NO_XMIM
// TODO: // TODO:    /* Notify Motif input manager that widget has focus */
// TODO: // TODO:    XmImVaSetFocusValues(widget, NULL);
// TODO: // TODO: #endif
// TODO: // TODO: 
// TODO: // TODO:    /* Call any registered focus-in callbacks */
// TODO: // TODO:    XtCallCallbacks((Fl_Widget*) widget, textNfocusCallback, (XtPointer) event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void focusOutAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO: 
// TODO: // TODO:    /* Remove the cursor blinking timer procedure */
// TODO: // TODO:    if (((TextWidget)w)->text.cursorBlinkProcID != 0)
// TODO: // TODO:       XtRemoveTimeOut(((TextWidget)w)->text.cursorBlinkProcID);
// TODO: // TODO:    ((TextWidget)w)->text.cursorBlinkProcID = 0;
// TODO: // TODO: 
// TODO: // TODO:    /* Leave a dim or destination cursor */
// TODO: // TODO:    TextDSetCursorStyle(textD, ((TextWidget)w)->text.motifDestOwner ?
// TODO: // TODO:                        CARET_CURSOR : DIM_CURSOR);
// TODO: // TODO:    TextDUnblankCursor(textD);
// TODO: // TODO: 
// TODO: // TODO:    /* If there's a calltip displayed, kill it. */
// TODO: // TODO:    TextDKillCalltip(textD, 0);
// TODO: // TODO: 
// TODO: // TODO:    /* Call any registered focus-out callbacks */
// TODO: // TODO:    XtCallCallbacks((Fl_Widget*)w, textNlosingFocusCallback, (XtPointer)event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** For actions involving cursor movement, "extend" keyword means incorporate
// TODO: // TODO: ** the new cursor position in the selection, and lack of an "extend" keyword
// TODO: // TODO: ** means cancel the existing selection
// TODO: // TODO: */
// TODO: // TODO: static void checkMoveSelectionChange(Fl_Widget* w, int event, int startPos,
// TODO: // TODO:                                      const char** args, int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    if (hasKey("extend", args, nArgs))
// TODO: // TODO:       keyMoveExtendSelection(w, event, startPos, hasKey("rect", args, nArgs));
// TODO: // TODO:    else
// TODO: // TODO:       BufUnselect((((TextWidget)w)->text.textD)->buffer);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** If a selection change was requested via a keyboard command for moving
// TODO: // TODO: ** the insertion cursor (usually with the "extend" keyword), adjust the
// TODO: // TODO: ** selection to include the new cursor position, or begin a new selection
// TODO: // TODO: ** between startPos and the new cursor position with anchor at startPos.
// TODO: // TODO: */
// TODO: // TODO: static void keyMoveExtendSelection(Fl_Widget* w, int event, int origPos,
// TODO: // TODO:                                    int rectangular)
// TODO: // TODO: {
// TODO: // TODO:    XKeyEvent* e = &event->xkey;
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO:    textDisp* textD = tw->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    selection* sel = &buf->primary;
// TODO: // TODO:    int newPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    int startPos, endPos, startCol, endCol, newCol, origCol;
// TODO: // TODO:    int anchor, rectAnchor, anchorLineStart;
// TODO: // TODO: 
// TODO: // TODO:    /* Moving the cursor does not take the Motif destination, but as soon as
// TODO: // TODO:       the user selects something, grab it (I'm not sure if this distinction
// TODO: // TODO:       actually makes sense, but it's what Motif was doing, back when their
// TODO: // TODO:       secondary selections actually worked correctly) */
// TODO: // TODO:    TakeMotifDestination(w, e->time);
// TODO: // TODO: 
// TODO: // TODO:    if ((sel->selected || sel->zeroWidth) && sel->rectangular && rectangular)
// TODO: // TODO:    {
// TODO: // TODO:       /* rect -> rect */
// TODO: // TODO:       newCol = BufCountDispChars(buf, BufStartOfLine(buf, newPos), newPos);
// TODO: // TODO:       startCol = min(tw->text.rectAnchor, newCol);
// TODO: // TODO:       endCol   = max(tw->text.rectAnchor, newCol);
// TODO: // TODO:       startPos = BufStartOfLine(buf, min(tw->text.anchor, newPos));
// TODO: // TODO:       endPos = BufEndOfLine(buf, max(tw->text.anchor, newPos));
// TODO: // TODO:       BufRectSelect(buf, startPos, endPos, startCol, endCol);
// TODO: // TODO:    }
// TODO: // TODO:    else if (sel->selected && rectangular)     /* plain -> rect */
// TODO: // TODO:    {
// TODO: // TODO:       newCol = BufCountDispChars(buf, BufStartOfLine(buf, newPos), newPos);
// TODO: // TODO:       if (abs(newPos - sel->start) < abs(newPos - sel->end))
// TODO: // TODO:          anchor = sel->end;
// TODO: // TODO:       else
// TODO: // TODO:          anchor = sel->start;
// TODO: // TODO:       anchorLineStart = BufStartOfLine(buf, anchor);
// TODO: // TODO:       rectAnchor = BufCountDispChars(buf, anchorLineStart, anchor);
// TODO: // TODO:       tw->text.anchor = anchor;
// TODO: // TODO:       tw->text.rectAnchor = rectAnchor;
// TODO: // TODO:       BufRectSelect(buf, BufStartOfLine(buf, min(anchor, newPos)),
// TODO: // TODO:                     BufEndOfLine(buf, max(anchor, newPos)),
// TODO: // TODO:                     min(rectAnchor, newCol), max(rectAnchor, newCol));
// TODO: // TODO:    }
// TODO: // TODO:    else if (sel->selected && sel->rectangular)     /* rect -> plain */
// TODO: // TODO:    {
// TODO: // TODO:       startPos = BufCountForwardDispChars(buf,
// TODO: // TODO:                                           BufStartOfLine(buf, sel->start), sel->rectStart);
// TODO: // TODO:       endPos = BufCountForwardDispChars(buf,
// TODO: // TODO:                                         BufStartOfLine(buf, sel->end), sel->rectEnd);
// TODO: // TODO:       if (abs(origPos - startPos) < abs(origPos - endPos))
// TODO: // TODO:          anchor = endPos;
// TODO: // TODO:       else
// TODO: // TODO:          anchor = startPos;
// TODO: // TODO:       BufSelect(buf, anchor, newPos);
// TODO: // TODO:    }
// TODO: // TODO:    else if (sel->selected)     /* plain -> plain */
// TODO: // TODO:    {
// TODO: // TODO:       if (abs(origPos - sel->start) < abs(origPos - sel->end))
// TODO: // TODO:          anchor = sel->end;
// TODO: // TODO:       else
// TODO: // TODO:          anchor = sel->start;
// TODO: // TODO:       BufSelect(buf, anchor, newPos);
// TODO: // TODO:    }
// TODO: // TODO:    else if (rectangular)     /* no sel -> rect */
// TODO: // TODO:    {
// TODO: // TODO:       origCol = BufCountDispChars(buf, BufStartOfLine(buf, origPos), origPos);
// TODO: // TODO:       newCol = BufCountDispChars(buf, BufStartOfLine(buf, newPos), newPos);
// TODO: // TODO:       startCol = min(newCol, origCol);
// TODO: // TODO:       endCol = max(newCol, origCol);
// TODO: // TODO:       startPos = BufStartOfLine(buf, min(origPos, newPos));
// TODO: // TODO:       endPos = BufEndOfLine(buf, max(origPos, newPos));
// TODO: // TODO:       tw->text.anchor = origPos;
// TODO: // TODO:       tw->text.rectAnchor = origCol;
// TODO: // TODO:       BufRectSelect(buf, startPos, endPos, startCol, endCol);
// TODO: // TODO:    }
// TODO: // TODO:    else     /* no sel -> plain */
// TODO: // TODO:    {
// TODO: // TODO:       tw->text.anchor = origPos;
// TODO: // TODO:       tw->text.rectAnchor = BufCountDispChars(buf,
// TODO: // TODO:                                               BufStartOfLine(buf, origPos), origPos);
// TODO: // TODO:       BufSelect(buf, tw->text.anchor, newPos);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void checkAutoShowInsertPos(Fl_Widget* w)
// TODO: // TODO: {
// TODO: // TODO:    if (((TextWidget)w)->text.autoShowInsertPos)
// TODO: // TODO:       TextDMakeInsertPosVisible(((TextWidget)w)->text.textD);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static int checkReadOnly(Fl_Widget* w)
// TODO: // TODO: {
// TODO: // TODO:    if (((TextWidget)w)->text.readOnly)
// TODO: // TODO:    {
// TODO: // TODO:       XBell(XtDisplay(w), 0);
// TODO: // TODO:       return true;
// TODO: // TODO:    }
// TODO: // TODO:    return false;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Insert text "chars" at the cursor position, as if the text had been
// TODO: // TODO: ** typed.  Same as TextInsertAtCursor, but without the complicated auto-wrap
// TODO: // TODO: ** scanning and re-formatting.
// TODO: // TODO: */
// TODO: // TODO: static void simpleInsertAtCursor(Fl_Widget* w, char* chars, int event,
// TODO: // TODO:                                  int allowPendingDelete)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    char* c;
// TODO: // TODO: 
// TODO: // TODO:    if (allowPendingDelete && pendingSelection(w))
// TODO: // TODO:    {
// TODO: // TODO:       BufReplaceSelected(buf, chars);
// TODO: // TODO:       TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO: // TODO:    }
// TODO: // TODO:    else if (((TextWidget)w)->text.overstrike)
// TODO: // TODO:    {
// TODO: // TODO:       for (c=chars; *c!='\0' && *c!='\n'; c++);
// TODO: // TODO:       if (*c == '\n')
// TODO: // TODO:          TextDInsert(textD, chars);
// TODO: // TODO:       else
// TODO: // TODO:          TextDOverstrike(textD, chars);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:       TextDInsert(textD, chars);
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** If there's a selection, delete it and position the cursor where the
// TODO: // TODO: ** selection was deleted.  (Called by routines which do deletion to check
// TODO: // TODO: ** first for and do possible selection delete)
// TODO: // TODO: */
// TODO: // TODO: static int deletePendingSelection(Fl_Widget* w, int event)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO: 
// TODO: // TODO:    if (((TextWidget)w)->text.textD->buffer->primary.selected)
// TODO: // TODO:    {
// TODO: // TODO:       BufRemoveSelected(buf);
// TODO: // TODO:       TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO: // TODO:       checkAutoShowInsertPos(w);
// TODO: // TODO:       callCursorMovementCBs(w, event);
// TODO: // TODO:       return true;
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:       return false;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Return true if pending delete is on and there's a selection contiguous
// TODO: // TODO: ** with the cursor ready to be deleted.  These criteria are used to decide
// TODO: // TODO: ** if typing a character or inserting something should delete the selection
// TODO: // TODO: ** first.
// TODO: // TODO: */
// TODO: // TODO: static int pendingSelection(Fl_Widget* w)
// TODO: // TODO: {
// TODO: // TODO:    selection* sel = &((TextWidget)w)->text.textD->buffer->primary;
// TODO: // TODO:    int pos = TextDGetInsertPosition(((TextWidget)w)->text.textD);
// TODO: // TODO: 
// TODO: // TODO:    return ((TextWidget)w)->text.pendingDelete && sel->selected &&
// TODO: // TODO:           pos >= sel->start && pos <= sel->end;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Check if tab emulation is on and if there are emulated tabs before the
// TODO: // TODO: ** cursor, and if so, delete an emulated tab as a unit.  Also finishes up
// TODO: // TODO: ** by calling checkAutoShowInsertPos and callCursorMovementCBs, so the
// TODO: // TODO: ** calling action proc can just return (this is necessary to preserve
// TODO: // TODO: ** emTabsBeforeCursor which is otherwise cleared by callCursorMovementCBs).
// TODO: // TODO: */
// TODO: // TODO: static int deleteEmulatedTab(Fl_Widget* w, int event)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    textBuffer* buf = ((TextWidget)w)->text.textD->buffer;
// TODO: // TODO:    int emTabDist = ((TextWidget)w)->text.emulateTabs;
// TODO: // TODO:    int emTabsBeforeCursor = ((TextWidget)w)->text.emTabsBeforeCursor;
// TODO: // TODO:    int startIndent, toIndent, insertPos, startPos, lineStart;
// TODO: // TODO:    int pos, indent, startPosIndent;
// TODO: // TODO:    char c, *spaceString;
// TODO: // TODO: 
// TODO: // TODO:    if (emTabDist <= 0 || emTabsBeforeCursor <= 0)
// TODO: // TODO:       return false;
// TODO: // TODO: 
// TODO: // TODO:    /* Find the position of the previous tab stop */
// TODO: // TODO:    insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    lineStart = BufStartOfLine(buf, insertPos);
// TODO: // TODO:    startIndent = BufCountDispChars(buf, lineStart, insertPos);
// TODO: // TODO:    toIndent = (startIndent-1) - ((startIndent-1) % emTabDist);
// TODO: // TODO: 
// TODO: // TODO:    /* Find the position at which to begin deleting (stop at non-whitespace
// TODO: // TODO:       characters) */
// TODO: // TODO:    startPosIndent = indent = 0;
// TODO: // TODO:    startPos = lineStart;
// TODO: // TODO:    for (pos=lineStart; pos < insertPos; pos++)
// TODO: // TODO:    {
// TODO: // TODO:       c = BufGetCharacter(buf, pos);
// TODO: // TODO:       indent += BufCharWidth(c, indent, buf->tabDist, buf->nullSubsChar);
// TODO: // TODO:       if (indent > toIndent)
// TODO: // TODO:          break;
// TODO: // TODO:       startPosIndent = indent;
// TODO: // TODO:       startPos = pos + 1;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Just to make sure, check that we're not deleting any non-white chars */
// TODO: // TODO:    for (pos=insertPos-1; pos>=startPos; pos--)
// TODO: // TODO:    {
// TODO: // TODO:       c = BufGetCharacter(buf, pos);
// TODO: // TODO:       if (c != ' ' && c != '\t')
// TODO: // TODO:       {
// TODO: // TODO:          startPos = pos + 1;
// TODO: // TODO:          break;
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Do the text replacement and reposition the cursor.  If any spaces need
// TODO: // TODO:       to be inserted to make up for a deleted tab, do a BufReplace, otherwise,
// TODO: // TODO:       do a BufRemove. */
// TODO: // TODO:    if (startPosIndent < toIndent)
// TODO: // TODO:    {
// TODO: // TODO:       spaceString = malloc__(toIndent - startPosIndent + 1);
// TODO: // TODO:       memset(spaceString, ' ', toIndent-startPosIndent);
// TODO: // TODO:       spaceString[toIndent - startPosIndent] = '\0';
// TODO: // TODO:       BufReplace(buf, startPos, insertPos, spaceString);
// TODO: // TODO:       TextDSetInsertPosition(textD, startPos + toIndent - startPosIndent);
// TODO: // TODO:       XtFree(spaceString);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       BufRemove(buf, startPos, insertPos);
// TODO: // TODO:       TextDSetInsertPosition(textD, startPos);
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* The normal cursor movement stuff would usually be called by the action
// TODO: // TODO:       routine, but this wraps around it to restore emTabsBeforeCursor */
// TODO: // TODO:    checkAutoShowInsertPos(w);
// TODO: // TODO:    callCursorMovementCBs(w, event);
// TODO: // TODO: 
// TODO: // TODO:    /* Decrement and restore the marker for consecutive emulated tabs, which
// TODO: // TODO:       would otherwise have been zeroed by callCursorMovementCBs */
// TODO: // TODO:    ((TextWidget)w)->text.emTabsBeforeCursor = emTabsBeforeCursor - 1;
// TODO: // TODO:    return true;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Select the word or whitespace adjacent to the cursor, and move the cursor
// TODO: // TODO: ** to its end.  pointerX is used as a tie-breaker, when the cursor is at the
// TODO: // TODO: ** boundary between a word and some white-space.  If the cursor is on the
// TODO: // TODO: ** left, the word or space on the left is used.  If it's on the right, that
// TODO: // TODO: ** is used instead.
// TODO: // TODO: */
// TODO: // TODO: static void selectWord(Fl_Widget* w, int pointerX)
// TODO: // TODO: {
// TODO: // TODO:    TextWidget tw = (TextWidget)w;
// TODO: // TODO:    textBuffer* buf = tw->text.textD->buffer;
// TODO: // TODO:    int x, y, insertPos = TextDGetInsertPosition(tw->text.textD);
// TODO: // TODO: 
// TODO: // TODO:    TextPosToXY(w, insertPos, &x, &y);
// TODO: // TODO:    if (pointerX < x && insertPos > 0 && BufGetCharacter(buf, insertPos-1) != '\n')
// TODO: // TODO:       insertPos--;
// TODO: // TODO:    BufSelect(buf, startOfWord(tw, insertPos), endOfWord(tw, insertPos));
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static int startOfWord(TextWidget w, int pos)
// TODO: // TODO: {
// TODO: // TODO:    int startPos;
// TODO: // TODO:    textBuffer* buf = w->text.textD->buffer;
// TODO: // TODO:    char* delimiters=w->text.delimiters;
// TODO: // TODO:    char c = BufGetCharacter(buf, pos);
// TODO: // TODO: 
// TODO: // TODO:    if (c == ' ' || c== '\t')
// TODO: // TODO:    {
// TODO: // TODO:       if (!spanBackward(buf, pos, " \t", false, &startPos))
// TODO: // TODO:          return 0;
// TODO: // TODO:    }
// TODO: // TODO:    else if (strchr(delimiters, c))
// TODO: // TODO:    {
// TODO: // TODO:       if (!spanBackward(buf, pos, delimiters, true, &startPos))
// TODO: // TODO:          return 0;
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       if (!BufSearchBackward(buf, pos, delimiters, &startPos))
// TODO: // TODO:          return 0;
// TODO: // TODO:    }
// TODO: // TODO:    return min(pos, startPos+1);
// TODO: // TODO: 
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static int endOfWord(TextWidget w, int pos)
// TODO: // TODO: {
// TODO: // TODO:    int endPos;
// TODO: // TODO:    textBuffer* buf = w->text.textD->buffer;
// TODO: // TODO:    char* delimiters=w->text.delimiters;
// TODO: // TODO:    char c = BufGetCharacter(buf, pos);
// TODO: // TODO: 
// TODO: // TODO:    if (c == ' ' || c== '\t')
// TODO: // TODO:    {
// TODO: // TODO:       if (!spanForward(buf, pos, " \t", false, &endPos))
// TODO: // TODO:          return buf->length;
// TODO: // TODO:    }
// TODO: // TODO:    else if (strchr(delimiters, c))
// TODO: // TODO:    {
// TODO: // TODO:       if (!spanForward(buf, pos, delimiters, true, &endPos))
// TODO: // TODO:          return buf->length;
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       if (!BufSearchForward(buf, pos, delimiters, &endPos))
// TODO: // TODO:          return buf->length;
// TODO: // TODO:    }
// TODO: // TODO:    return endPos;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Search forwards in buffer "buf" for the first character NOT in
// TODO: // TODO: ** "searchChars",  starting with the character "startPos", and returning the
// TODO: // TODO: ** result in "foundPos" returns true if found, false if not. If ignoreSpace
// TODO: // TODO: ** is set, then Space, Tab, and Newlines are ignored in searchChars.
// TODO: // TODO: */
// TODO: // TODO: static int spanForward(textBuffer* buf, int startPos, char* searchChars,
// TODO: // TODO:                        int ignoreSpace, int* foundPos)
// TODO: // TODO: {
// TODO: // TODO:    int pos;
// TODO: // TODO:    char* c;
// TODO: // TODO: 
// TODO: // TODO:    pos = startPos;
// TODO: // TODO:    while (pos < buf->length)
// TODO: // TODO:    {
// TODO: // TODO:       for (c=searchChars; *c!='\0'; c++)
// TODO: // TODO:          if (!(ignoreSpace && (*c==' ' || *c=='\t' || *c=='\n')))
// TODO: // TODO:             if (BufGetCharacter(buf, pos) == *c)
// TODO: // TODO:                break;
// TODO: // TODO:       if (*c == 0)
// TODO: // TODO:       {
// TODO: // TODO:          *foundPos = pos;
// TODO: // TODO:          return true;
// TODO: // TODO:       }
// TODO: // TODO:       pos++;
// TODO: // TODO:    }
// TODO: // TODO:    *foundPos = buf->length;
// TODO: // TODO:    return false;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Search backwards in buffer "buf" for the first character NOT in
// TODO: // TODO: ** "searchChars",  starting with the character BEFORE "startPos", returning the
// TODO: // TODO: ** result in "foundPos" returns true if found, false if not. If ignoreSpace is
// TODO: // TODO: ** set, then Space, Tab, and Newlines are ignored in searchChars.
// TODO: // TODO: */
// TODO: // TODO: static int spanBackward(textBuffer* buf, int startPos, char* searchChars, int
// TODO: // TODO:                         ignoreSpace, int* foundPos)
// TODO: // TODO: {
// TODO: // TODO:    int pos;
// TODO: // TODO:    char* c;
// TODO: // TODO: 
// TODO: // TODO:    if (startPos == 0)
// TODO: // TODO:    {
// TODO: // TODO:       *foundPos = 0;
// TODO: // TODO:       return false;
// TODO: // TODO:    }
// TODO: // TODO:    pos = startPos == 0 ? 0 : startPos - 1;
// TODO: // TODO:    while (pos >= 0)
// TODO: // TODO:    {
// TODO: // TODO:       for (c=searchChars; *c!='\0'; c++)
// TODO: // TODO:          if (!(ignoreSpace && (*c==' ' || *c=='\t' || *c=='\n')))
// TODO: // TODO:             if (BufGetCharacter(buf, pos) == *c)
// TODO: // TODO:                break;
// TODO: // TODO:       if (*c == 0)
// TODO: // TODO:       {
// TODO: // TODO:          *foundPos = pos;
// TODO: // TODO:          return true;
// TODO: // TODO:       }
// TODO: // TODO:       pos--;
// TODO: // TODO:    }
// TODO: // TODO:    *foundPos = 0;
// TODO: // TODO:    return false;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Select the line containing the cursor, including the terminating newline,
// TODO: // TODO: ** and move the cursor to its end.
// TODO: // TODO: */
// TODO: // TODO: static void selectLine(Fl_Widget* w)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO: // TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO: // TODO:    int endPos, startPos;
// TODO: // TODO: 
// TODO: // TODO:    endPos = BufEndOfLine(textD->buffer, insertPos);
// TODO: // TODO:    startPos = BufStartOfLine(textD->buffer, insertPos);
// TODO: // TODO:    BufSelect(textD->buffer, startPos, min(endPos + 1, textD->buffer->length));
// TODO: // TODO:    TextDSetInsertPosition(textD, endPos);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Given a new mouse pointer location, pass the position on to the
// TODO: // TODO: ** autoscroll timer routine, and make sure the timer is on when it's
// TODO: // TODO: ** needed and off when it's not.
// TODO: // TODO: */
// TODO: // TODO: static void checkAutoScroll(TextWidget w, int x, int y)
// TODO: // TODO: {
// TODO: // TODO:    int inWindow;
// TODO: // TODO: 
// TODO: // TODO:    /* Is the pointer in or out of the window? */
// TODO: // TODO:    inWindow = x >= w->text.textD->left &&
// TODO: // TODO:               x < w->core.width - w->text.marginWidth &&
// TODO: // TODO:               y >= w->text.marginHeight &&
// TODO: // TODO:               y < w->core.height - w->text.marginHeight;
// TODO: // TODO: 
// TODO: // TODO:    /* If it's in the window, cancel the timer procedure */
// TODO: // TODO:    if (inWindow)
// TODO: // TODO:    {
// TODO: // TODO:       if (w->text.autoScrollProcID != 0)
// TODO: // TODO:          XtRemoveTimeOut(w->text.autoScrollProcID);;
// TODO: // TODO:       w->text.autoScrollProcID = 0;
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* If the timer is not already started, start it */
// TODO: // TODO:    if (w->text.autoScrollProcID == 0)
// TODO: // TODO:    {
// TODO: // TODO:       w->text.autoScrollProcID = XtAppAddTimeOut(
// TODO: // TODO:                                     XtWidgetToApplicationContext((Fl_Widget*)w),
// TODO: // TODO:                                     0, autoScrollTimerProc, w);
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Pass on the newest mouse location to the autoscroll routine */
// TODO: // TODO:    w->text.mouseX = x;
// TODO: // TODO:    w->text.mouseY = y;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Reset drag state and cancel the auto-scroll timer
// TODO: // TODO: */
// TODO: // TODO: static void endDrag(Fl_Widget* w)
// TODO: // TODO: {
// TODO: // TODO:    if (((TextWidget)w)->text.autoScrollProcID != 0)
// TODO: // TODO:       XtRemoveTimeOut(((TextWidget)w)->text.autoScrollProcID);
// TODO: // TODO:    ((TextWidget)w)->text.autoScrollProcID = 0;
// TODO: // TODO:    if (((TextWidget)w)->text.dragState == MOUSE_PAN)
// TODO: // TODO:       XUngrabPointer(XtDisplay(w), CurrentTime);
// TODO: // TODO:    ((TextWidget)w)->text.dragState = NOT_CLICKED;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Cancel any drag operation that might be in progress.  Should be included
// TODO: // TODO: ** in nearly every key event to cleanly end any dragging before edits are made
// TODO: // TODO: ** which might change the insert position or the content of the buffer during
// TODO: // TODO: ** a drag operation)
// TODO: // TODO: */
// TODO: // TODO: static void cancelDrag(Fl_Widget* w)
// TODO: // TODO: {
// TODO: // TODO:    int dragState = ((TextWidget)w)->text.dragState;
// TODO: // TODO: 
// TODO: // TODO:    if (((TextWidget)w)->text.autoScrollProcID != 0)
// TODO: // TODO:       XtRemoveTimeOut(((TextWidget)w)->text.autoScrollProcID);
// TODO: // TODO:    if (dragState == SECONDARY_DRAG || dragState == SECONDARY_RECT_DRAG)
// TODO: // TODO:       BufSecondaryUnselect(((TextWidget)w)->text.textD->buffer);
// TODO: // TODO:    if (dragState == PRIMARY_BLOCK_DRAG)
// TODO: // TODO:       CancelBlockDrag((TextWidget)w);
// TODO: // TODO:    if (dragState == MOUSE_PAN)
// TODO: // TODO:       XUngrabPointer(XtDisplay(w), CurrentTime);
// TODO: // TODO:    if (dragState != NOT_CLICKED)
// TODO: // TODO:       ((TextWidget)w)->text.dragState = DRAG_CANCELED;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Do operations triggered by cursor movement: Call cursor movement callback
// TODO: // TODO: ** procedure(s), and cancel marker indicating that the cursor is after one or
// TODO: // TODO: ** more just-entered emulated tabs (spaces to be deleted as a unit).
// TODO: // TODO: */
// TODO: // TODO: static void callCursorMovementCBs(Fl_Widget* w, int event)
// TODO: // TODO: {
// TODO: // TODO:    ((TextWidget)w)->text.emTabsBeforeCursor = 0;
// TODO: // TODO:    XtCallCallbacks((Fl_Widget*)w, textNcursorMovementCallback, (XtPointer)event);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Adjust the selection as the mouse is dragged to position: (x, y).
// TODO: // TODO: */
// TODO: // TODO: static void adjustSelection(TextWidget tw, int x, int y)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = tw->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    int row, col, startCol, endCol, startPos, endPos;
// TODO: // TODO:    int newPos = TextDXYToPosition(textD, x, y);
// TODO: // TODO: 
// TODO: // TODO:    /* Adjust the selection */
// TODO: // TODO:    if (tw->text.dragState == PRIMARY_RECT_DRAG)
// TODO: // TODO:    {
// TODO: // TODO:       TextDXYToUnconstrainedPosition(textD, x, y, &row, &col);
// TODO: // TODO:       col = TextDOffsetWrappedColumn(textD, row, col);
// TODO: // TODO:       startCol = min(tw->text.rectAnchor, col);
// TODO: // TODO:       endCol = max(tw->text.rectAnchor, col);
// TODO: // TODO:       startPos = BufStartOfLine(buf, min(tw->text.anchor, newPos));
// TODO: // TODO:       endPos = BufEndOfLine(buf, max(tw->text.anchor, newPos));
// TODO: // TODO:       BufRectSelect(buf, startPos, endPos, startCol, endCol);
// TODO: // TODO:    }
// TODO: // TODO:    else if (tw->text.multiClickState == ONE_CLICK)
// TODO: // TODO:    {
// TODO: // TODO:       startPos = startOfWord(tw, min(tw->text.anchor, newPos));
// TODO: // TODO:       endPos = endOfWord(tw, max(tw->text.anchor, newPos));
// TODO: // TODO:       BufSelect(buf, startPos, endPos);
// TODO: // TODO:       newPos = newPos < tw->text.anchor ? startPos : endPos;
// TODO: // TODO:    }
// TODO: // TODO:    else if (tw->text.multiClickState == TWO_CLICKS)
// TODO: // TODO:    {
// TODO: // TODO:       startPos = BufStartOfLine(buf, min(tw->text.anchor, newPos));
// TODO: // TODO:       endPos = BufEndOfLine(buf, max(tw->text.anchor, newPos));
// TODO: // TODO:       BufSelect(buf, startPos, min(endPos+1, buf->length));
// TODO: // TODO:       newPos = newPos < tw->text.anchor ? startPos : endPos;
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:       BufSelect(buf, tw->text.anchor, newPos);
// TODO: // TODO: 
// TODO: // TODO:    /* Move the cursor */
// TODO: // TODO:    TextDSetInsertPosition(textD, newPos);
// TODO: // TODO:    callCursorMovementCBs((Fl_Widget*)tw, NULL);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void adjustSecondarySelection(TextWidget tw, int x, int y)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = tw->text.textD;
// TODO: // TODO:    textBuffer* buf = textD->buffer;
// TODO: // TODO:    int row, col, startCol, endCol, startPos, endPos;
// TODO: // TODO:    int newPos = TextDXYToPosition(textD, x, y);
// TODO: // TODO: 
// TODO: // TODO:    if (tw->text.dragState == SECONDARY_RECT_DRAG)
// TODO: // TODO:    {
// TODO: // TODO:       TextDXYToUnconstrainedPosition(textD, x, y, &row, &col);
// TODO: // TODO:       col = TextDOffsetWrappedColumn(textD, row, col);
// TODO: // TODO:       startCol = min(tw->text.rectAnchor, col);
// TODO: // TODO:       endCol = max(tw->text.rectAnchor, col);
// TODO: // TODO:       startPos = BufStartOfLine(buf, min(tw->text.anchor, newPos));
// TODO: // TODO:       endPos = BufEndOfLine(buf, max(tw->text.anchor, newPos));
// TODO: // TODO:       BufSecRectSelect(buf, startPos, endPos, startCol, endCol);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:       BufSecondarySelect(textD->buffer, tw->text.anchor, newPos);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Wrap multi-line text in argument "text" to be inserted at the end of the
// TODO: // TODO: ** text on line "startLine" and return the result.  If "breakBefore" is
// TODO: // TODO: ** non-NULL, allow wrapping to extend back into "startLine", in which case
// TODO: // TODO: ** the returned text will include the wrapped part of "startLine", and
// TODO: // TODO: ** "breakBefore" will return the number of characters at the end of
// TODO: // TODO: ** "startLine" that were absorbed into the returned string.  "breakBefore"
// TODO: // TODO: ** will return zero if no characters were absorbed into the returned string.
// TODO: // TODO: ** The buffer offset of text in the widget's text buffer is needed so that
// TODO: // TODO: ** smart indent (which can be triggered by wrapping) can search back farther
// TODO: // TODO: ** in the buffer than just the text in startLine.
// TODO: // TODO: */
// TODO: // TODO: static char* wrapText(TextWidget tw, char* startLine, char* text, int bufOffset,
// TODO: // TODO:                       int wrapMargin, int* breakBefore)
// TODO: // TODO: {
// TODO: // TODO:    textBuffer* wrapBuf, *buf = tw->text.textD->buffer;
// TODO: // TODO:    int startLineLen = strlen(startLine);
// TODO: // TODO:    int colNum, pos, lineStartPos, limitPos, breakAt, charsAdded;
// TODO: // TODO:    int firstBreak = -1, tabDist = buf->tabDist;
// TODO: // TODO:    char c, *wrappedText;
// TODO: // TODO: 
// TODO: // TODO:    /* Create a temporary text buffer and load it with the strings */
// TODO: // TODO:    wrapBuf = BufCreate();
// TODO: // TODO:    BufInsert(wrapBuf, 0, startLine);
// TODO: // TODO:    BufInsert(wrapBuf, wrapBuf->length, text);
// TODO: // TODO: 
// TODO: // TODO:    /* Scan the buffer for long lines and apply wrapLine when wrapMargin is
// TODO: // TODO:       exceeded.  limitPos enforces no breaks in the "startLine" part of the
// TODO: // TODO:       string (if requested), and prevents re-scanning of long unbreakable
// TODO: // TODO:       lines for each character beyond the margin */
// TODO: // TODO:    colNum = 0;
// TODO: // TODO:    pos = 0;
// TODO: // TODO:    lineStartPos = 0;
// TODO: // TODO:    limitPos = breakBefore == NULL ? startLineLen : 0;
// TODO: // TODO:    while (pos < wrapBuf->length)
// TODO: // TODO:    {
// TODO: // TODO:       c = BufGetCharacter(wrapBuf, pos);
// TODO: // TODO:       if (c == '\n')
// TODO: // TODO:       {
// TODO: // TODO:          lineStartPos = limitPos = pos + 1;
// TODO: // TODO:          colNum = 0;
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          colNum += BufCharWidth(c, colNum, tabDist, buf->nullSubsChar);
// TODO: // TODO:          if (colNum > wrapMargin)
// TODO: // TODO:          {
// TODO: // TODO:             if (!wrapLine(tw, wrapBuf, bufOffset, lineStartPos, pos,
// TODO: // TODO:                           limitPos, &breakAt, &charsAdded))
// TODO: // TODO:             {
// TODO: // TODO:                limitPos = max(pos, limitPos);
// TODO: // TODO:             }
// TODO: // TODO:             else
// TODO: // TODO:             {
// TODO: // TODO:                lineStartPos = limitPos = breakAt+1;
// TODO: // TODO:                pos += charsAdded;
// TODO: // TODO:                colNum = BufCountDispChars(wrapBuf, lineStartPos, pos+1);
// TODO: // TODO:                if (firstBreak == -1)
// TODO: // TODO:                   firstBreak = breakAt;
// TODO: // TODO:             }
// TODO: // TODO:          }
// TODO: // TODO:       }
// TODO: // TODO:       pos++;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Return the wrapped text, possibly including part of startLine */
// TODO: // TODO:    if (breakBefore == NULL)
// TODO: // TODO:       wrappedText = BufGetRange(wrapBuf, startLineLen, wrapBuf->length);
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       *breakBefore = firstBreak != -1 && firstBreak < startLineLen ?
// TODO: // TODO:                      startLineLen - firstBreak : 0;
// TODO: // TODO:       wrappedText = BufGetRange(wrapBuf, startLineLen - *breakBefore,
// TODO: // TODO:                                 wrapBuf->length);
// TODO: // TODO:    }
// TODO: // TODO:    BufFree(wrapBuf);
// TODO: // TODO:    return wrappedText;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Wraps the end of a line beginning at lineStartPos and ending at lineEndPos
// TODO: // TODO: ** in "buf", at the last white-space on the line >= limitPos.  (The implicit
// TODO: // TODO: ** assumption is that just the last character of the line exceeds the wrap
// TODO: // TODO: ** margin, and anywhere on the line we can wrap is correct).  Returns false if
// TODO: // TODO: ** unable to wrap the line.  "breakAt", returns the character position at
// TODO: // TODO: ** which the line was broken,
// TODO: // TODO: **
// TODO: // TODO: ** Auto-wrapping can also trigger auto-indent.  The additional parameter
// TODO: // TODO: ** bufOffset is needed when auto-indent is set to smart indent and the smart
// TODO: // TODO: ** indent routines need to scan far back in the buffer.  "charsAdded" returns
// TODO: // TODO: ** the number of characters added to acheive the auto-indent.  wrapMargin is
// TODO: // TODO: ** used to decide whether auto-indent should be skipped because the indent
// TODO: // TODO: ** string itself would exceed the wrap margin.
// TODO: // TODO: */
// TODO: // TODO: static int wrapLine(TextWidget tw, textBuffer* buf, int bufOffset,
// TODO: // TODO:                     int lineStartPos, int lineEndPos, int limitPos, int* breakAt,
// TODO: // TODO:                     int* charsAdded)
// TODO: // TODO: {
// TODO: // TODO:    int p, length, column;
// TODO: // TODO:    char c, *indentStr;
// TODO: // TODO: 
// TODO: // TODO:    /* Scan backward for whitespace or BOL.  If BOL, return false, no
// TODO: // TODO:       whitespace in line at which to wrap */
// TODO: // TODO:    for (p=lineEndPos; ; p--)
// TODO: // TODO:    {
// TODO: // TODO:       if (p < lineStartPos || p < limitPos)
// TODO: // TODO:          return false;
// TODO: // TODO:       c = BufGetCharacter(buf, p);
// TODO: // TODO:       if (c == '\t' || c == ' ')
// TODO: // TODO:          break;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Create an auto-indent string to insert to do wrap.  If the auto
// TODO: // TODO:       indent string reaches the wrap position, slice the auto-indent
// TODO: // TODO:       back off and return to the left margin */
// TODO: // TODO:    if (tw->text.autoIndent || tw->text.smartIndent)
// TODO: // TODO:    {
// TODO: // TODO:       indentStr = createIndentString(tw, buf, bufOffset, lineStartPos,
// TODO: // TODO:                                      lineEndPos, &length, &column);
// TODO: // TODO:       if (column >= p-lineStartPos)
// TODO: // TODO:          indentStr[1] = '\0';
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       indentStr = "\n";
// TODO: // TODO:       length = 1;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Replace the whitespace character with the auto-indent string
// TODO: // TODO:       and return the stats */
// TODO: // TODO:    BufReplace(buf, p, p+1, indentStr);
// TODO: // TODO:    if (tw->text.autoIndent || tw->text.smartIndent)
// TODO: // TODO:       XtFree(indentStr);
// TODO: // TODO:    *breakAt = p;
// TODO: // TODO:    *charsAdded = length-1;
// TODO: // TODO:    return true;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Create and return an auto-indent string to add a newline at lineEndPos to a
// TODO: // TODO: ** line starting at lineStartPos in buf.  "buf" may or may not be the real
// TODO: // TODO: ** text buffer for the widget.  If it is not the widget's text buffer it's
// TODO: // TODO: ** offset position from the real buffer must be specified in "bufOffset" to
// TODO: // TODO: ** allow the smart-indent routines to scan back as far as necessary. The
// TODO: // TODO: ** string length is returned in "length" (or "length" can be passed as NULL,
// TODO: // TODO: ** and the indent column is returned in "column" (if non NULL).
// TODO: // TODO: */
// TODO: // TODO: static char* createIndentString(TextWidget tw, textBuffer* buf, int bufOffset,
// TODO: // TODO:                                 int lineStartPos, int lineEndPos, int* length, int* column)
// TODO: // TODO: {
// TODO: // TODO:    textDisp* textD = tw->text.textD;
// TODO: // TODO:    int pos, indent = -1, tabDist = textD->buffer->tabDist;
// TODO: // TODO:    int i, useTabs = textD->buffer->useTabs;
// TODO: // TODO:    char* indentPtr, *indentStr, c;
// TODO: // TODO:    smartIndentCBStruct smartIndent;
// TODO: // TODO: 
// TODO: // TODO:    /* If smart indent is on, call the smart indent callback.  It is not
// TODO: // TODO:       called when multi-line changes are being made (lineStartPos != 0),
// TODO: // TODO:       because smart indent needs to search back an indeterminate distance
// TODO: // TODO:       through the buffer, and reconciling that with wrapping changes made,
// TODO: // TODO:       but not yet committed in the buffer, would make programming smart
// TODO: // TODO:       indent more difficult for users and make everything more complicated */
// TODO: // TODO:    if (tw->text.smartIndent && (lineStartPos == 0 || buf == textD->buffer))
// TODO: // TODO:    {
// TODO: // TODO:       smartIndent.reason = NEWLINE_INDENT_NEEDED;
// TODO: // TODO:       smartIndent.pos = lineEndPos + bufOffset;
// TODO: // TODO:       smartIndent.indentRequest = 0;
// TODO: // TODO:       smartIndent.charsTyped = NULL;
// TODO: // TODO:       XtCallCallbacks((Fl_Widget*)tw, textNsmartIndentCallback,
// TODO: // TODO:                       (XtPointer)&smartIndent);
// TODO: // TODO:       indent = smartIndent.indentRequest;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* If smart indent wasn't used, measure the indent distance of the line */
// TODO: // TODO:    if (indent == -1)
// TODO: // TODO:    {
// TODO: // TODO:       indent = 0;
// TODO: // TODO:       for (pos=lineStartPos; pos<lineEndPos; pos++)
// TODO: // TODO:       {
// TODO: // TODO:          c =  BufGetCharacter(buf, pos);
// TODO: // TODO:          if (c != ' ' && c != '\t')
// TODO: // TODO:             break;
// TODO: // TODO:          if (c == '\t')
// TODO: // TODO:             indent += tabDist - (indent % tabDist);
// TODO: // TODO:          else
// TODO: // TODO:             indent++;
// TODO: // TODO:       }
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* Allocate and create a string of tabs and spaces to achieve the indent */
// TODO: // TODO:    indentPtr = indentStr = malloc__(indent + 2);
// TODO: // TODO:    *indentPtr++ = '\n';
// TODO: // TODO:    if (useTabs)
// TODO: // TODO:    {
// TODO: // TODO:       for (i=0; i < indent / tabDist; i++)
// TODO: // TODO:          *indentPtr++ = '\t';
// TODO: // TODO:       for (i=0; i < indent % tabDist; i++)
// TODO: // TODO:          *indentPtr++ = ' ';
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       for (i=0; i < indent; i++)
// TODO: // TODO:          *indentPtr++ = ' ';
// TODO: // TODO:    }
// TODO: // TODO:    *indentPtr = '\0';
// TODO: // TODO: 
// TODO: // TODO:    /* Return any requested stats */
// TODO: // TODO:    if (length != NULL)
// TODO: // TODO:       *length = indentPtr - indentStr;
// TODO: // TODO:    if (column != NULL)
// TODO: // TODO:       *column = indent;
// TODO: // TODO: 
// TODO: // TODO:    return indentStr;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Xt timer procedure for autoscrolling
// TODO: // TODO: */
// TODO: // TODO: static void autoScrollTimerProc(XtPointer clientData, XtIntervalId* id)
// TODO: // TODO: {
// TODO: // TODO:    TextWidget w = (TextWidget)clientData;
// TODO: // TODO:    textDisp* textD = w->text.textD;
// TODO: // TODO:    int topLineNum, horizOffset, newPos, cursorX, y;
// TODO: // TODO:    int fontWidth = textD->fontStruct->max_bounds.width;
// TODO: // TODO:    int fontHeight = textD->fontStruct->ascent + textD->fontStruct->descent;
// TODO: // TODO: 
// TODO: // TODO:    /* For vertical autoscrolling just dragging the mouse outside of the top
// TODO: // TODO:       or bottom of the window is sufficient, for horizontal (non-rectangular)
// TODO: // TODO:       scrolling, see if the position where the CURSOR would go is outside */
// TODO: // TODO:    newPos = TextDXYToPosition(textD, w->text.mouseX, w->text.mouseY);
// TODO: // TODO:    if (w->text.dragState == PRIMARY_RECT_DRAG)
// TODO: // TODO:       cursorX = w->text.mouseX;
// TODO: // TODO:    else if (!TextDPositionToXY(textD, newPos, &cursorX, &y))
// TODO: // TODO:       cursorX = w->text.mouseX;
// TODO: // TODO: 
// TODO: // TODO:    /* Scroll away from the pointer, 1 character (horizontal), or 1 character
// TODO: // TODO:       for each fontHeight distance from the mouse to the text (vertical) */
// TODO: // TODO:    TextDGetScroll(textD, &topLineNum, &horizOffset);
// TODO: // TODO:    if (cursorX >= (int)w->core.width - w->text.marginWidth)
// TODO: // TODO:       horizOffset += fontWidth;
// TODO: // TODO:    else if (w->text.mouseX < textD->left)
// TODO: // TODO:       horizOffset -= fontWidth;
// TODO: // TODO:    if (w->text.mouseY >= (int)w->core.height - w->text.marginHeight)
// TODO: // TODO:       topLineNum += 1 + ((w->text.mouseY - (int)w->core.height -
// TODO: // TODO:                           w->text.marginHeight) / fontHeight) + 1;
// TODO: // TODO:    else if (w->text.mouseY < w->text.marginHeight)
// TODO: // TODO:       topLineNum -= 1 + ((w->text.marginHeight-w->text.mouseY) / fontHeight);
// TODO: // TODO:    TextDSetScroll(textD, topLineNum, horizOffset);
// TODO: // TODO: 
// TODO: // TODO:    /* Continue the drag operation in progress.  If none is in progress
// TODO: // TODO:       (safety check) don't continue to re-establish the timer proc */
// TODO: // TODO:    if (w->text.dragState == PRIMARY_DRAG)
// TODO: // TODO:    {
// TODO: // TODO:       adjustSelection(w, w->text.mouseX, w->text.mouseY);
// TODO: // TODO:    }
// TODO: // TODO:    else if (w->text.dragState == PRIMARY_RECT_DRAG)
// TODO: // TODO:    {
// TODO: // TODO:       adjustSelection(w, w->text.mouseX, w->text.mouseY);
// TODO: // TODO:    }
// TODO: // TODO:    else if (w->text.dragState == SECONDARY_DRAG)
// TODO: // TODO:    {
// TODO: // TODO:       adjustSecondarySelection(w, w->text.mouseX, w->text.mouseY);
// TODO: // TODO:    }
// TODO: // TODO:    else if (w->text.dragState == SECONDARY_RECT_DRAG)
// TODO: // TODO:    {
// TODO: // TODO:       adjustSecondarySelection(w, w->text.mouseX, w->text.mouseY);
// TODO: // TODO:    }
// TODO: // TODO:    else if (w->text.dragState == PRIMARY_BLOCK_DRAG)
// TODO: // TODO:    {
// TODO: // TODO:       BlockDragSelection(w, w->text.mouseX, w->text.mouseY, USE_LAST);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       w->text.autoScrollProcID = 0;
// TODO: // TODO:       return;
// TODO: // TODO:    }
// TODO: // TODO: 
// TODO: // TODO:    /* re-establish the timer proc (this routine) to continue processing */
// TODO: // TODO:    w->text.autoScrollProcID = XtAppAddTimeOut(
// TODO: // TODO:                                  XtWidgetToApplicationContext((Fl_Widget*)w),
// TODO: // TODO:                                  w->text.mouseY >= w->text.marginHeight &&
// TODO: // TODO:                                  w->text.mouseY < w->core.height - w->text.marginHeight ?
// TODO: // TODO:                                  (VERTICAL_SCROLL_DELAY*fontWidth) / fontHeight :
// TODO: // TODO:                                  VERTICAL_SCROLL_DELAY, autoScrollTimerProc, w);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** Xt timer procedure for cursor blinking
// TODO: // TODO: */
// TODO: // TODO: static void cursorBlinkTimerProc(XtPointer clientData, XtIntervalId* id)
// TODO: // TODO: {
// TODO: // TODO:    TextWidget w = (TextWidget)clientData;
// TODO: // TODO:    textDisp* textD = w->text.textD;
// TODO: // TODO: 
// TODO: // TODO:    /* Blink the cursor */
// TODO: // TODO:    if (textD->cursorOn)
// TODO: // TODO:       TextDBlankCursor(textD);
// TODO: // TODO:    else
// TODO: // TODO:       TextDUnblankCursor(textD);
// TODO: // TODO: 
// TODO: // TODO:    /* re-establish the timer proc (this routine) to continue processing */
// TODO: // TODO:    w->text.cursorBlinkProcID = XtAppAddTimeOut(
// TODO: // TODO:                                   XtWidgetToApplicationContext((Fl_Widget*)w),
// TODO: // TODO:                                   w->text.cursorBlinkRate, cursorBlinkTimerProc, w);
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: **  Sets the caret to on or off and restart the caret blink timer.
// TODO: // TODO: **  This could be used by other modules to modify the caret's blinking.
// TODO: // TODO: */
// TODO: // TODO: void ResetCursorBlink(TextWidget textWidget, bool startsBlanked)
// TODO: // TODO: {
// TODO: // TODO:    if (0 != textWidget->text.cursorBlinkRate)
// TODO: // TODO:    {
// TODO: // TODO:       if (0 != textWidget->text.cursorBlinkProcID)
// TODO: // TODO:       {
// TODO: // TODO:          XtRemoveTimeOut(textWidget->text.cursorBlinkProcID);
// TODO: // TODO:       }
// TODO: // TODO: 
// TODO: // TODO:       if (startsBlanked)
// TODO: // TODO:       {
// TODO: // TODO:          TextDBlankCursor(textWidget->text.textD);
// TODO: // TODO:       }
// TODO: // TODO:       else
// TODO: // TODO:       {
// TODO: // TODO:          TextDUnblankCursor(textWidget->text.textD);
// TODO: // TODO:       }
// TODO: // TODO: 
// TODO: // TODO:       textWidget->text.cursorBlinkProcID
// TODO: // TODO:          = XtAppAddTimeOut(XtWidgetToApplicationContext((Fl_Widget*) textWidget),
// TODO: // TODO:                            textWidget->text.cursorBlinkRate, cursorBlinkTimerProc,
// TODO: // TODO:                            textWidget);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** look at an action procedure's arguments to see if argument "key" has been
// TODO: // TODO: ** specified in the argument list
// TODO: // TODO: */
// TODO: // TODO: static int hasKey(const char* key, const const char** args, const int* nArgs)
// TODO: // TODO: {
// TODO: // TODO:    int i;
// TODO: // TODO: 
// TODO: // TODO:    for (i=0; i<(int)*nArgs; i++)
// TODO: // TODO:       if (!strCaseCmp(args[i], key))
// TODO: // TODO:          return true;
// TODO: // TODO:    return false;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static int max(int i1, int i2)
// TODO: // TODO: {
// TODO: // TODO:    return i1 >= i2 ? i1 : i2;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static int min(int i1, int i2)
// TODO: // TODO: {
// TODO: // TODO:    return i1 <= i2 ? i1 : i2;
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: /*
// TODO: // TODO: ** strCaseCmp compares its arguments and returns 0 if the two strings
// TODO: // TODO: ** are equal IGNORING case differences.  Otherwise returns 1.
// TODO: // TODO: */
// TODO: // TODO: 
// TODO: // TODO: static int strCaseCmp(const char* str1, const char* str2)
// TODO: // TODO: {
// TODO: // TODO:    unsigned const char* c1 = (unsigned const char*) str1;
// TODO: // TODO:    unsigned const char* c2 = (unsigned const char*) str2;
// TODO: // TODO: 
// TODO: // TODO:    for (; *c1!='\0' && *c2!='\0'; c1++, c2++)
// TODO: // TODO:       if (toupper(*c1) != toupper(*c2))
// TODO: // TODO:          return 1;
// TODO: // TODO:    if (*c1 == *c2)
// TODO: // TODO:    {
// TODO: // TODO:       return(0);
// TODO: // TODO:    }
// TODO: // TODO:    else
// TODO: // TODO:    {
// TODO: // TODO:       return(1);
// TODO: // TODO:    }
// TODO: // TODO: }
// TODO: // TODO: 
// TODO: // TODO: static void ringIfNecessary(bool silent, Fl_Widget* w)
// TODO: // TODO: {
// TODO: // TODO:    if (!silent)
// TODO: // TODO:       XBell(XtDisplay(w), 0);
// TODO: // TODO: }
// TODO: 