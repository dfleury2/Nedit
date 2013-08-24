#include "Ne_Text_Editor.h"

#include "../util/Ne_AppContext.h"
#include "../util/Ne_Color.h"

#include "Ne_Text_Sel.h"
#include "Ne_Text_Drag.h"
#include "nedit.h"
// TODO: #include "calltips.h"
#include "../util/DialogF.h"
#include "../util/utils.h"
#include "../util/misc.h"
// TODO: #include "window.h"

#include <FL/names.h>
#include <FL/fl_ask.H>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <list>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#ifndef WIN32
#include <sys/param.h>
#endif
#include <limits.h>

/* Number of pixels of motion from the initial (grab-focus) button press
   required to begin recognizing a mouse drag for the purpose of making a
   selection */
#define NE_SELECT_THRESHOLD 5

/* Length of delay in milliseconds for vertical autoscrolling */
#define VERTICAL_SCROLL_DELAY 50

// TODO: static void initialize(TextWidget request, TextWidget newWidget);
// TODO: static void handleHidePointer(Fl_Widget* w, XtPointer unused, int event, bool* continue_to_dispatch);
// TODO: static void handleShowPointer(Fl_Widget* w, XtPointer unused, int event, bool* continue_to_dispatch);
// TODO: static void redisplay(TextWidget w, int event, Region region);
// TODO: static void redisplayGE(TextWidget w, XtPointer client_data, int event, bool* continue_to_dispatch_return);
// TODO: static void destroy(TextWidget w);
// TODO: static void resize(TextWidget w);
// TODO: static void realize(Fl_Widget* w, XtValueMask* valueMask, XSetWindowAttributes* attributes);
// TODO: static XtGeometryResult queryGeometry(Fl_Widget* w, XtWidgetGeometry* proposed, XtWidgetGeometry* answer);
static void grabFocusAP(Fl_Widget* w, int event, const char** args, int* n_args);
static void moveDestinationAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void extendAdjustAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void extendStartAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void extendEndAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void processCancelAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void secondaryStartAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void secondaryOrDragStartAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void secondaryAdjustAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void secondaryOrDragAdjustAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void copyToAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void copyToOrEndDragAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void copyPrimaryAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void cutPrimaryAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void moveToAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void moveToOrEndDragAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void endDragAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void exchangeAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void mousePanAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void pasteClipboardAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void copyClipboardAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void cutClipboardAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void insertStringAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void selfInsertAP(Fl_Widget* w, int event, const char** args, int* n_args);
static void newlineAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void newlineAndIndentAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void newlineNoIndentAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void processTabAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void endOfLineAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void beginningOfLineAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void deleteSelectionAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void deletePreviousCharacterAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void deleteNextCharacterAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void deletePreviousWordAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void deleteNextWordAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void deleteToStartOfLineAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void deleteToEndOfLineAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void forwardCharacterAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void backwardCharacterAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void forwardWordAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void backwardWordAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void forwardParagraphAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void backwardParagraphAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void keySelectAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void processUpAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void processShiftUpAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void processDownAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void processShiftDownAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void beginningOfFileAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void endOfFileAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void nextPageAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void previousPageAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: static void pageLeftAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: static void pageRightAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void toggleOverstrikeAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: static void scrollUpAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: static void scrollDownAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: static void scrollLeftAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: static void scrollRightAP(Fl_Widget* w, int event, const char** args, int* nArgs);
// TODO: static void scrollToLineAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void selectAllAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void deselectAllAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void focusInAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void focusOutAP(Fl_Widget* w, int event, const char** args, int* nArgs);
static void checkMoveSelectionChange(Fl_Widget* w, int event, int startPos, const char** args, int* nArgs);
static void keyMoveExtendSelection(Fl_Widget* w, int event, int startPos, int rectangular);
static void checkAutoShowInsertPos(Fl_Widget* w);
static int checkReadOnly(Fl_Widget* w);
static void simpleInsertAtCursor(Fl_Widget* w, char* chars, int event, int allowPendingDelete);
static int pendingSelection(Fl_Widget* w);
static int deletePendingSelection(Fl_Widget* w, int event);
static int deleteEmulatedTab(Fl_Widget* w, int event);
static void selectWord(Fl_Widget* w, int pointerX);
static int spanForward(Ne_Text_Buffer* buf, int startPos, const char* searchChars, int ignoreSpace, int* foundPos);
static int spanBackward(Ne_Text_Buffer* buf, int startPos, const char* searchChars, int ignoreSpace, int* foundPos);
static void selectLine(Fl_Widget* w);
static int startOfWord(Fl_Widget* w, int pos);
static int endOfWord(Fl_Widget* w, int pos);
static void checkAutoScroll(Fl_Widget* w, int x, int y);
static void endDrag(Fl_Widget* w);
static void cancelDrag(Fl_Widget* w);
static void callCursorMovementCBs(Fl_Widget* w, int event);
static void adjustSelection(Fl_Widget* w, int x, int y);
static void adjustSecondarySelection(Ne_Text_Editor* textD, int x, int y);
static void insertClipboard(Ne_Text_Editor* textD);
// TODO: static void autoScrollTimerProc(XtPointer clientData, XtIntervalId* id);
static char* wrapText(Fl_Widget* w, char* startLine, char* text, int bufOffset, int wrapMargin, int* breakBefore);
static int wrapLine(Fl_Widget* w, Ne_Text_Buffer* buf, int bufOffset, int lineStartPos, int lineEndPos, int limitPos, int* breakAt, int* charsAdded);
static char* createIndentString(Fl_Widget* w, Ne_Text_Buffer* buf, int bufOffset, int lineStartPos, int lineEndPos, int* length, int* column);
static void cursorBlinkTimerProc(void* data);
static bool hasKey(const char* key, const char** args, const int* nArgs);
static int max(int i1, int i2);
static int min(int i1, int i2);
static int strCaseCmp(const char* str1, const char* str2);
static void ringIfNecessary(bool silent);

// --------------------------------------------------------------------------
struct EventKeyButtonStateCommand
{
   explicit EventKeyButtonStateCommand()
   {
      event = key = button = 0;
      shift = ctrl = alt = meta = 0;
   }

   bool match( const EventKeyButtonStateCommand& cmd ) 
   {
      if (event != cmd.event) return false;
      if (key != cmd.key) return false;
      if (button != cmd.button) return false;
      if (!(shift == cmd.shift || cmd.shift == -1)) return false;
      if (!(ctrl == cmd.ctrl || cmd.ctrl == -1)) return false;
      if (!(alt == cmd.alt || cmd.alt == -1)) return false;
      if (!(meta == cmd.meta || cmd.meta== -1)) return false;

      return true;
   }

   int event;
   int key; // -1: any (printable ?), else char code
   int button;
   int shift, ctrl, alt, meta; // 0 : Inactive, 1: Active, -1: don't care

   std::string command;
};

typedef std::list<EventKeyButtonStateCommand> KeyStateTranslationTable;

static KeyStateTranslationTable KeyStateTranslations;

int AsState(const std::string& line, const char* modifierName)
{
   std::string::size_type sep = line.find(':');
   if (sep == std::string::npos) return 0; // Bad format

   std::string::size_type found = line.find(modifierName);
   if (found == std::string::npos || found > sep) return 0;

   if (found==0) return 1;
   if (line[found-1] == '~') return -1;
   return 1;
}

int AsEvent(const std::string&line)
{
   std::string::size_type sep = line.find(':');
   if (sep == std::string::npos) return 0; // bad format

   std::string::size_type keyPress = line.find("<KeyPress>");
   if (keyPress != std::string::npos && keyPress < sep) return FL_KEYBOARD;

   for (int i = 1; i < 10; ++i)
   {
      std::ostringstream oss;
      oss << "Btn" << i << "Down";
      std::string::size_type btnDown = line.find(oss.str());
      if (btnDown != std::string::npos) return FL_PUSH;
   }

   for (int i = 1; i < 10; ++i)
   {
      std::ostringstream oss;
      oss << "Btn" << i << "Up";
      std::string::size_type btnDown = line.find(oss.str());
      if (btnDown != std::string::npos) return FL_RELEASE;
   }

   std::string::size_type mouseMove = line.find("<MotionNotify>");
   if (mouseMove != std::string::npos && mouseMove < sep) return FL_MOVE;

   return 0; // ???
}

int AsKey(const std::string&line)
{
   if (line.find("osfBeginLine") != std::string::npos) return FL_Home;
   if (line.find("osfBackSpace") != std::string::npos) return FL_BackSpace;
   if (line.find("osfDelete") != std::string::npos)  return FL_Delete;
   if (line.find("osfInsert") != std::string::npos) return FL_Insert;
   if (line.find("osfInsert") != std::string::npos) return FL_Insert;
   if (line.find("osfEndLine") != std::string::npos) return FL_End;
   if (line.find("osfLeft") != std::string::npos) return FL_Left;
   if (line.find("osfRight") != std::string::npos) return FL_Right;
   if (line.find("osfUp") != std::string::npos) return FL_Up;
   if (line.find("osfDown") != std::string::npos) return FL_Down;
   if (line.find("osfPageUp") != std::string::npos) return FL_Page_Up;
   if (line.find("osfPageDown") != std::string::npos) return FL_Page_Down;
   if (line.find("osfCancel") != std::string::npos) return FL_Escape;

   if (line.find("Return") != std::string::npos) return FL_Enter;
   if (line.find("osfActivate") != std::string::npos) return FL_KP_Enter;
   if (line.find("Tab") != std::string::npos) return FL_Tab;
   if (line.find("space") != std::string::npos) return ' ';

   std::string::size_type sep = line.find(':');
   if (sep == std::string::npos) return 0; // Bad format

   std::string::size_type keyPress = line.find("<KeyPress>");
   if (keyPress == std::string::npos)
      return 0; // No key
   
   std::string key = line.substr(keyPress+10, sep - (keyPress+10));
   if (key.empty()) return -1;
   if (key.size() == 1) return key[0];

   // Another keycode...
   if (key == "slash") return '/';
   if (key == "backslash") return '\\';
   if (key == "space") return ' ';

   return 0; // Unrecognized key
}

int AsButton(const std::string&line)
{
   for (int i = 1; i < 10; ++i)
   {
      std::ostringstream oss;
      oss << "Btn" << i << "Down";
      std::string::size_type button = line.find(oss.str());
      if (button != std::string::npos) return i;

      oss.str("");
      oss << "Btn" << i << "Up";
      button = line.find(oss.str());
      if (button != std::string::npos) return i;

      oss.str("");
      oss << "Button" << i;
      button = line.find(oss.str());
      if (button != std::string::npos) return i;
   }
   
   return 0; // No button
}


std::string GetCommand(const std::string& line)
{
   std::string::size_type sep = line.find(':');
   if (sep == std::string::npos)
      return "";
   
   return Trim(line.substr(sep+1));
}

void AddKeyTranslation(KeyStateTranslationTable& KeyStateTranslations, const std::string& line)
{
   EventKeyButtonStateCommand cmd; 
   cmd.event = AsEvent(line);
   cmd.key = AsKey(line);
   cmd.shift = AsState(line, "Shift");
   cmd.ctrl = AsState(line, "Ctrl");
   cmd.alt = AsState(line, "Alt");
   cmd.meta = AsState(line, "Meta");
   cmd.button = AsButton(line);
   cmd.command = GetCommand(line);

   KeyStateTranslations.push_back(cmd);
}

bool InitKeyStateTranslations()
{
static char defaultTranslations[] =
   /* Home */
   "~Shift ~Ctrl Alt<Key>osfBeginLine: last_document()\n"

   /* Backspace */
   "Ctrl<KeyPress>osfBackSpace: delete_previous_word()\n"
   "<KeyPress>osfBackSpace: delete_previous_character()\n"

   /* Delete */
   "Alt Shift Ctrl<KeyPress>osfDelete: cut_primary(\"rect\")\n"
   "Meta Shift Ctrl<KeyPress>osfDelete: cut_primary(\"rect\")\n"
   "Shift Ctrl<KeyPress>osfDelete: cut_primary()\n"
   "Ctrl<KeyPress>osfDelete: delete_to_end_of_line()\n"
   "Shift<KeyPress>osfDelete: cut_clipboard()\n"
   "<KeyPress>osfDelete: delete_next_character()\n"

   /* Insert */
   "Alt Shift Ctrl<KeyPress>osfInsert: copy_primary(\"rect\")\n"
   "Meta Shift Ctrl<KeyPress>osfInsert: copy_primary(\"rect\")\n"
   "Shift Ctrl<KeyPress>osfInsert: copy_primary()\n"
   "Shift<KeyPress>osfInsert: paste_clipboard()\n"
   "Ctrl<KeyPress>osfInsert: copy_clipboard()\n"
   //"~Shift ~Ctrl<KeyPress>osfInsert: set_overtype_mode()\n" // .. BUG ???
   "~Shift ~Ctrl<KeyPress>osfInsert: toggle-overstrike()\n"

   /* Cut/Copy/Paste */
   "Shift Ctrl<KeyPress>osfCut: cut_primary()\n"
   "<KeyPress>osfCut: cut_clipboard()\n"
   "<KeyPress>osfCopy: copy_clipboard()\n"
   "<KeyPress>osfPaste: paste_clipboard()\n"
   "<KeyPress>osfPrimaryPaste: copy_primary()\n"

   /* BeginLine */
   "Alt Shift Ctrl<KeyPress>osfBeginLine: beginning_of_file(\"extend\", \"rect\")\n"
   "Meta Shift Ctrl<KeyPress>osfBeginLine: beginning_of_file(\"extend\" \"rect\")\n"
   "Alt Shift<KeyPress>osfBeginLine: beginning_of_line(\"extend\", \"rect\")\n"
   "Meta Shift<KeyPress>osfBeginLine: beginning_of_line(\"extend\", \"rect\")\n"
   "Shift Ctrl<KeyPress>osfBeginLine: beginning_of_file(\"extend\")\n"
   "Ctrl<KeyPress>osfBeginLine: beginning_of_file()\n"
   "Shift<KeyPress>osfBeginLine: beginning_of_line(\"extend\")\n"
   "~Alt~Shift~Ctrl~Meta<KeyPress>osfBeginLine: beginning_of_line()\n"

   /* EndLine */
   "Alt Shift Ctrl<KeyPress>osfEndLine: end_of_file(\"extend\", \"rect\")\n"
   "Meta Shift Ctrl<KeyPress>osfEndLine: end_of_file(\"extend\", \"rect\")\n"
   "Alt Shift<KeyPress>osfEndLine: end_of_line(\"extend\", \"rect\")\n"
   "Meta Shift<KeyPress>osfEndLine: end_of_line(\"extend\", \"rect\")\n"
   "Shift Ctrl<KeyPress>osfEndLine: end_of_file(\"extend\")\n"
   "Ctrl<KeyPress>osfEndLine: end_of_file()\n"
   "Shift<KeyPress>osfEndLine: end_of_line(\"extend\")\n"
   "~Alt~Shift~Ctrl~Meta<KeyPress>osfEndLine: end_of_line()\n"

   /* Left */
   "Alt Shift Ctrl<KeyPress>osfLeft: backward_word(\"extend\", \"rect\")\n"
   "Meta Shift Ctrl<KeyPress>osfLeft: backward_word(\"extend\", \"rect\")\n"
   "Alt Shift<KeyPress>osfLeft: key_select(\"left\", \"rect\")\n"
   "Meta Shift<KeyPress>osfLeft: key_select(\"left\", \"rect\")\n"
   "Shift Ctrl<KeyPress>osfLeft: backward_word(\"extend\")\n"
   "Ctrl<KeyPress>osfLeft: backward_word()\n"
   "Shift<KeyPress>osfLeft: key_select(\"left\")\n"
   "~Alt~Shift~Ctrl~Meta<KeyPress>osfLeft: backward_character()\n"

   /* Right */
   "Alt Shift Ctrl<KeyPress>osfRight: forward_word(\"extend\", \"rect\")\n"
   "Meta Shift Ctrl<KeyPress>osfRight: forward_word(\"extend\", \"rect\")\n"
   "Alt Shift<KeyPress>osfRight: key_select(\"right\", \"rect\")\n"
   "Meta Shift<KeyPress>osfRight: key_select(\"right\", \"rect\")\n"
   "Shift Ctrl<KeyPress>osfRight: forward_word(\"extend\")\n"
   "Ctrl<KeyPress>osfRight: forward_word()\n"
   "Shift<KeyPress>osfRight: key_select(\"right\")\n"
   "~Alt~Shift~Ctrl~Meta<KeyPress>osfRight: forward_character()\n"

   /* Up */
   "Alt Shift Ctrl<KeyPress>osfUp: backward_paragraph(\"extend\", \"rect\")\n"
   "Meta Shift Ctrl<KeyPress>osfUp: backward_paragraph(\"extend\", \"rect\")\n"
   "Alt Shift<KeyPress>osfUp: process_shift_up(\"rect\")\n"
   "Meta Shift<KeyPress>osfUp: process_shift_up(\"rect\")\n"
   "Shift Ctrl<KeyPress>osfUp: backward_paragraph(\"extend\")\n"
   "Ctrl<KeyPress>osfUp: backward_paragraph()\n"
   "Shift<KeyPress>osfUp: process_shift_up()\n"
   "~Alt~Shift~Ctrl~Meta<KeyPress>osfUp: process_up()\n"

   /* Down */
   "Alt Shift Ctrl<KeyPress>osfDown: forward_paragraph(\"extend\", \"rect\")\n"
   "Meta Shift Ctrl<KeyPress>osfDown: forward_paragraph(\"extend\", \"rect\")\n"
   "Alt Shift<KeyPress>osfDown: process_shift_down(\"rect\")\n"
   "Meta Shift<KeyPress>osfDown: process_shift_down(\"rect\")\n"
   "Shift Ctrl<KeyPress>osfDown: forward_paragraph(\"extend\")\n"
   "Ctrl<KeyPress>osfDown: forward_paragraph()\n"
   "Shift<KeyPress>osfDown: process_shift_down()\n"
   "~Alt~Shift~Ctrl~Meta<KeyPress>osfDown: process_down()\n"

   /* PageUp */
   "Alt Shift Ctrl<KeyPress>osfPageUp: page_left(\"extend\", \"rect\")\n"
   "Meta Shift Ctrl<KeyPress>osfPageUp: page_left(\"extend\", \"rect\")\n"
   "Alt Shift<KeyPress>osfPageUp: previous_page(\"extend\", \"rect\")\n"
   "Meta Shift<KeyPress>osfPageUp: previous_page(\"extend\", \"rect\")\n"
   "Shift Ctrl<KeyPress>osfPageUp: page_left(\"extend\")\n"
   "Ctrl<KeyPress>osfPageUp: previous_document()\n"
   "Shift<KeyPress>osfPageUp: previous_page(\"extend\")\n"
   "~Alt ~Shift ~Ctrl ~Meta<KeyPress>osfPageUp: previous_page()\n"

   /* PageDown */
   "Alt Shift Ctrl<KeyPress>osfPageDown: page_right(\"extend\", \"rect\")\n"
   "Meta Shift Ctrl<KeyPress>osfPageDown: page_right(\"extend\", \"rect\")\n"
   "Alt Shift<KeyPress>osfPageDown: next_page(\"extend\", \"rect\")\n"
   "Meta Shift<KeyPress>osfPageDown: next_page(\"extend\", \"rect\")\n"
   "Shift Ctrl<KeyPress>osfPageDown: page_right(\"extend\")\n"
   "Ctrl<KeyPress>osfPageDown: next_document()\n"
   "Shift<KeyPress>osfPageDown: next_page(\"extend\")\n"
   "~Alt ~Shift ~Ctrl ~Meta<KeyPress>osfPageDown: next_page()\n"

   /* PageLeft and PageRight are placed later than the PageUp/PageDown
      bindings.  Some systems map osfPageLeft to Ctrl-PageUp.
      Overloading this single key gives problems, and we want to give
      priority to the normal version. */

   /* PageLeft */
   "Alt Shift<KeyPress>osfPageLeft: page_left(\"extend\", \"rect\")\n"
   "Meta Shift<KeyPress>osfPageLeft: page_left(\"extend\", \"rect\")\n"
   "Shift<KeyPress>osfPageLeft: page_left(\"extend\")\n"
   "~Alt ~Shift ~Ctrl ~Meta<KeyPress>osfPageLeft: page_left()\n"

   /* PageRight */
   "Alt Shift<KeyPress>osfPageRight: page_right(\"extend\", \"rect\")\n"
   "Meta Shift<KeyPress>osfPageRight: page_right(\"extend\", \"rect\")\n"
   "Shift<KeyPress>osfPageRight: page_right(\"extend\")\n"
   "~Alt ~Shift ~Ctrl ~Meta<KeyPress>osfPageRight: page_right()\n"

   "Shift<KeyPress>osfSelect: key_select()\n"
   "<KeyPress>osfCancel: process_cancel()\n"
   "Ctrl~Alt~Meta<KeyPress>v: paste_clipboard()\n"
   "Ctrl~Alt~Meta<KeyPress>c: copy_clipboard()\n"
   "Ctrl~Alt~Meta<KeyPress>x: cut_clipboard()\n"
   "Ctrl~Alt~Meta<KeyPress>u: delete_to_start_of_line()\n"
   "Ctrl<KeyPress>Return: newline_and_indent()\n"
   "Shift<KeyPress>Return: newline_no_indent()\n"
   "<KeyPress>Return: newline()\n"
   /* KP_Enter = osfActivate
      Note: Ctrl+KP_Enter is already bound to Execute Command Line... */
   "Shift<KeyPress>osfActivate: newline_no_indent()\n"
   "<KeyPress>osfActivate: newline()\n"
   "Ctrl<KeyPress>Tab: self_insert()\n"
   "<KeyPress>Tab: process_tab()\n"
   "Alt Shift Ctrl<KeyPress>space: key_select(\"rect\")\n"
   "Meta Shift Ctrl<KeyPress>space: key_select(\"rect\")\n"
   "Shift Ctrl~Meta~Alt<KeyPress>space: key_select()\n"
   "Ctrl~Meta~Alt<KeyPress>slash: select_all()\n"
   "Ctrl~Meta~Alt<KeyPress>backslash: deselect_all()\n"
   "<KeyPress>: self_insert()\n"
   "Alt Ctrl<Btn1Down>: move_destination()\n"
   "Meta Ctrl<Btn1Down>: move_destination()\n"
   "Shift Ctrl<Btn1Down>: extend_start(\"rect\")\n"
   "Shift<Btn1Down>: extend_start()\n"
   "<Btn1Down>: grab_focus()\n"
   "Button1 Ctrl<MotionNotify>: extend_adjust(\"rect\")\n"
   "Button1~Ctrl<MotionNotify>: extend_adjust()\n"
   "<Btn1Up>: extend_end()\n"
   "<Btn2Down>: secondary_or_drag_start()\n"
   "Shift Ctrl Button2<MotionNotify>: secondary_or_drag_adjust(\"rect\", \"copy\", \"overlay\")\n"
   "Shift Button2<MotionNotify>: secondary_or_drag_adjust(\"copy\")\n"
   "Ctrl Button2<MotionNotify>: secondary_or_drag_adjust(\"rect\", \"overlay\")\n"
   "Button2<MotionNotify>: secondary_or_drag_adjust()\n"
   "Shift Ctrl<Btn2Up>: move_to_or_end_drag(\"copy\", \"overlay\")\n"
   "Shift <Btn2Up>: move_to_or_end_drag(\"copy\")\n"
   "Alt<Btn2Up>: exchange()\n"
   "Meta<Btn2Up>: exchange()\n"
   "Ctrl<Btn2Up>: copy_to_or_end_drag(\"overlay\")\n"
   "<Btn2Up>: copy_to_or_end_drag()\n"
   "Ctrl~Meta~Alt<Btn3Down>: mouse_pan()\n"
   "Ctrl~Meta~Alt Button3<MotionNotify>: mouse_pan()\n"
   "<Btn3Up>: end_drag()\n"
   "<FocusIn>: focusIn()\n"
   "<FocusOut>: focusOut()\n"
   /* Support for mouse wheel in XFree86 */
   "Shift<Btn4Down>,<Btn4Up>: scroll_up(1)\n"
   "Shift<Btn5Down>,<Btn5Up>: scroll_down(1)\n"
   "Ctrl<Btn4Down>,<Btn4Up>: scroll_up(1, pages)\n"
   "Ctrl<Btn5Down>,<Btn5Up>: scroll_down(1, pages)\n"
   "<Btn4Down>,<Btn4Up>: scroll_up(5)\n"
   "<Btn5Down>,<Btn5Up>: scroll_down(5)\n";
/* some of the translations from the Motif text widget were not picked up:
:<KeyPress>osfSelect: set-anchor()\n\
:<KeyPress>osfActivate: activate()\n\
~Shift Ctrl~Meta~Alt<KeyPress>Return: activate()\n\
~Shift Ctrl~Meta~Alt<KeyPress>space: set-anchor()\n\
 :<KeyPress>osfClear: clear-selection()\n\
~Shift~Ctrl~Meta~Alt<KeyPress>Return: process-return()\n\
Shift~Meta~Alt<KeyPress>Tab: prev-tab-group()\n\
Ctrl~Meta~Alt<KeyPress>Tab: next-tab-group()\n\
<UnmapNotify>: unmap()\n\
<EnterNotify>: enter()\n\
<LeaveNotify>: leave()\n
*/
   std::istringstream iss(defaultTranslations);

   for(std::string line; getline(iss, line);)
      AddKeyTranslation(KeyStateTranslations, line);

   return true;
}

static bool InitKeyStateTranslations_ = InitKeyStateTranslations();

static XtActionsRec actionsList[] =
{
   {"self-insert", selfInsertAP},
   {"self_insert", selfInsertAP},
   {"grab-focus", grabFocusAP},
   {"grab_focus", grabFocusAP},
   {"extend-adjust", extendAdjustAP},
   {"extend_adjust", extendAdjustAP},
   {"extend-start", extendStartAP},
   {"extend_start", extendStartAP},
   {"extend-end", extendEndAP},
   {"extend_end", extendEndAP},
   {"secondary-adjust", secondaryAdjustAP},
   {"secondary_adjust", secondaryAdjustAP},
   {"secondary-or-drag-adjust", secondaryOrDragAdjustAP},
   {"secondary_or_drag_adjust", secondaryOrDragAdjustAP},
   {"secondary-start", secondaryStartAP},
   {"secondary_start", secondaryStartAP},
   {"secondary-or-drag-start", secondaryOrDragStartAP},
   {"secondary_or_drag_start", secondaryOrDragStartAP},
   {"process-bdrag", secondaryOrDragStartAP},
   {"process_bdrag", secondaryOrDragStartAP},
   {"move-destination", moveDestinationAP},
   {"move_destination", moveDestinationAP},
   {"move-to", moveToAP},
   {"move_to", moveToAP},
   {"move-to-or-end-drag", moveToOrEndDragAP},
   {"move_to_or_end_drag", moveToOrEndDragAP},
   {"end_drag", endDragAP},
   {"copy-to", copyToAP},
   {"copy_to", copyToAP},
   {"copy-to-or-end-drag", copyToOrEndDragAP},
   {"copy_to_or_end_drag", copyToOrEndDragAP},
   {"exchange", exchangeAP},
   {"process-cancel", processCancelAP},
   {"process_cancel", processCancelAP},
   {"paste-clipboard", pasteClipboardAP},
   {"paste_clipboard", pasteClipboardAP},
   {"copy-clipboard", copyClipboardAP},
   {"copy_clipboard", copyClipboardAP},
   {"cut-clipboard", cutClipboardAP},
   {"cut_clipboard", cutClipboardAP},
   {"copy-primary", copyPrimaryAP},
   {"copy_primary", copyPrimaryAP},
   {"cut-primary", cutPrimaryAP},
   {"cut_primary", cutPrimaryAP},
   {"newline", newlineAP},
   {"newline-and-indent", newlineAndIndentAP},
   {"newline_and_indent", newlineAndIndentAP},
   {"newline-no-indent", newlineNoIndentAP},
   {"newline_no_indent", newlineNoIndentAP},
   {"delete-selection", deleteSelectionAP},
   {"delete_selection", deleteSelectionAP},
   {"delete-previous-character", deletePreviousCharacterAP},
   {"delete_previous_character", deletePreviousCharacterAP},
   {"delete-next-character", deleteNextCharacterAP},
   {"delete_next_character", deleteNextCharacterAP},
   {"delete-previous-word", deletePreviousWordAP},
   {"delete_previous_word", deletePreviousWordAP},
   {"delete-next-word", deleteNextWordAP},
   {"delete_next_word", deleteNextWordAP},
   {"delete-to-start-of-line", deleteToStartOfLineAP},
   {"delete_to_start_of_line", deleteToStartOfLineAP},
   {"delete-to-end-of-line", deleteToEndOfLineAP},
   {"delete_to_end_of_line", deleteToEndOfLineAP},
   {"forward-character", forwardCharacterAP},
   {"forward_character", forwardCharacterAP},
   {"backward-character", backwardCharacterAP},
   {"backward_character", backwardCharacterAP},
   {"key-select", keySelectAP},
   {"key_select", keySelectAP},
   {"process-up", processUpAP},
   {"process_up", processUpAP},
   {"process-down", processDownAP},
   {"process_down", processDownAP},
   {"process-shift-up", processShiftUpAP},
   {"process_shift_up", processShiftUpAP},
   {"process-shift-down", processShiftDownAP},
   {"process_shift_down", processShiftDownAP},
   {"process-home", beginningOfLineAP},
   {"process_home", beginningOfLineAP},
   {"forward-word", forwardWordAP},
   {"forward_word", forwardWordAP},
   {"backward-word", backwardWordAP},
   {"backward_word", backwardWordAP},
   {"forward-paragraph", forwardParagraphAP},
   {"forward_paragraph", forwardParagraphAP},
   {"backward-paragraph", backwardParagraphAP},
   {"backward_paragraph", backwardParagraphAP},
   {"beginning-of-line", beginningOfLineAP},
   {"beginning_of_line", beginningOfLineAP},
   {"end-of-line", endOfLineAP},
   {"end_of_line", endOfLineAP},
   {"beginning-of-file", beginningOfFileAP},
   {"beginning_of_file", beginningOfFileAP},
   {"end-of-file", endOfFileAP},
   {"end_of_file", endOfFileAP},
   {"next-page", nextPageAP},
   {"next_page", nextPageAP},
   {"previous-page", previousPageAP},
   {"previous_page", previousPageAP},
// TODO:    {"page-left", pageLeftAP},
// TODO:    {"page_left", pageLeftAP},
// TODO:    {"page-right", pageRightAP},
// TODO:    {"page_right", pageRightAP},
   {"toggle-overstrike", toggleOverstrikeAP},
   {"toggle_overstrike", toggleOverstrikeAP},
// TODO:    {"scroll-up", scrollUpAP},
// TODO:    {"scroll_up", scrollUpAP},
// TODO:    {"scroll-down", scrollDownAP},
// TODO:    {"scroll_down", scrollDownAP},
// TODO:    {"scroll_left", scrollLeftAP},
// TODO:    {"scroll_right", scrollRightAP},
// TODO:    {"scroll-to-line", scrollToLineAP},
// TODO:    {"scroll_to_line", scrollToLineAP},
   {"select-all", selectAllAP},
   {"select_all", selectAllAP},
   {"deselect-all", deselectAllAP},
   {"deselect_all", deselectAllAP},
   {"focusIn", focusInAP},
   {"focusOut", focusOutAP},
   {"process-return", selfInsertAP},
   {"process_return", selfInsertAP},
   {"process-tab", processTabAP},
   {"process_tab", processTabAP},
   {"insert-string", insertStringAP},
   {"insert_string", insertStringAP},
   {"mouse_pan", mousePanAP}
};

// TODO: /* The motif text widget defined a bunch of actions which the nedit text
// TODO:    widget as-of-yet does not support:
// TODO: 
// TODO:      Actions which were not bound to keys (for emacs emulation, some of
// TODO:      them should probably be supported:
// TODO: 
// TODO: 	kill-next-character()
// TODO: 	kill-next-word()
// TODO: 	kill-previous-character()
// TODO: 	kill-previous-word()
// TODO: 	kill-selection()
// TODO: 	kill-to-end-of-line()
// TODO: 	kill-to-start-of-line()
// TODO: 	unkill()
// TODO: 	next-line()
// TODO: 	newline-and-backup()
// TODO: 	beep()
// TODO: 	redraw-display()
// TODO: 	scroll-one-line-down()
// TODO: 	scroll-one-line-up()
// TODO: 	set-insertion-point()
// TODO: 
// TODO:     Actions which are not particularly useful:
// TODO: 
// TODO: 	set-anchor()
// TODO: 	activate()
// TODO: 	clear-selection() -> this is a wierd one
// TODO: 	do-quick-action() -> don't think this ever worked
// TODO: 	Help()
// TODO: 	next-tab-group()
// TODO: 	select-adjust()
// TODO: 	select-start()
// TODO: 	select-end()
// TODO: */
// TODO: 
// TODO: static XtResource resources[] =
// TODO: {
// TODO:    {
// TODO:       XmNhighlightThickness, XmCHighlightThickness, XmRDimension,
// TODO:       sizeof(Dimension), XtOffset(TextWidget, primitive.highlight_thickness),
// TODO:       XmRInt, 0
// TODO:    },
// TODO:    {
// TODO:       XmNshadowThickness, XmCShadowThickness, XmRDimension, sizeof(Dimension),
// TODO:       XtOffset(TextWidget, primitive.shadow_thickness), XmRInt, 0
// TODO:    },
// TODO:    {
// TODO:       textNfont, textCFont, XmRFontStruct, sizeof(XFontStruct*),
// TODO:       XtOffset(TextWidget, text.fontStruct), XmRString, "fixed"
// TODO:    },
// TODO:    {
// TODO:       textNselectForeground, textCSelectForeground, XmRPixel, sizeof(Pixel),
// TODO:       XtOffset(TextWidget, text.selectFGPixel), XmRString,
// TODO:       NEDIT_DEFAULT_SEL_FG
// TODO:    },
// TODO:    {
// TODO:       textNselectBackground, textCSelectBackground, XmRPixel, sizeof(Pixel),
// TODO:       XtOffset(TextWidget, text.selectBGPixel), XmRString,
// TODO:       NEDIT_DEFAULT_SEL_BG
// TODO:    },
// TODO:    {
// TODO:       textNhighlightForeground, textCHighlightForeground, XmRPixel,sizeof(Pixel),
// TODO:       XtOffset(TextWidget, text.highlightFGPixel), XmRString,
// TODO:       NEDIT_DEFAULT_HI_FG
// TODO:    },
// TODO:    {
// TODO:       textNhighlightBackground, textCHighlightBackground, XmRPixel,sizeof(Pixel),
// TODO:       XtOffset(TextWidget, text.highlightBGPixel), XmRString,
// TODO:       NEDIT_DEFAULT_HI_BG
// TODO:    },
// TODO:    {
// TODO:       textNlineNumForeground, textCLineNumForeground, XmRPixel,sizeof(Pixel),
// TODO:       XtOffset(TextWidget, text.lineNumFGPixel), XmRString,
// TODO:       NEDIT_DEFAULT_LINENO_FG
// TODO:    },
// TODO:    {
// TODO:       textNcursorForeground, textCCursorForeground, XmRPixel,sizeof(Pixel),
// TODO:       XtOffset(TextWidget, text.cursorFGPixel), XmRString,
// TODO:       NEDIT_DEFAULT_CURSOR_FG
// TODO:    },
// TODO:    {
// TODO:       textNcalltipForeground, textCcalltipForeground, XmRPixel,sizeof(Pixel),
// TODO:       XtOffset(TextWidget, text.calltipFGPixel), XmRString,
// TODO:       NEDIT_DEFAULT_CALLTIP_FG
// TODO:    },
// TODO:    {
// TODO:       textNcalltipBackground, textCcalltipBackground, XmRPixel,sizeof(Pixel),
// TODO:       XtOffset(TextWidget, text.calltipBGPixel), XmRString,
// TODO:       NEDIT_DEFAULT_CALLTIP_BG
// TODO:    },
// TODO:    {
// TODO:       textNbacklightCharTypes,textCBacklightCharTypes,XmRString,sizeof(XmString),
// TODO:       XtOffset(TextWidget, text.backlightCharTypes), XmRString, NULL
// TODO:    },
// TODO:    {
// TODO:       textNrows, textCRows, XmRInt,sizeof(int),
// TODO:       XtOffset(TextWidget, text.rows), XmRString, "24"
// TODO:    },
// TODO:    {
// TODO:       textNcolumns, textCColumns, XmRInt, sizeof(int),
// TODO:       XtOffset(TextWidget, text.columns), XmRString, "80"
// TODO:    },
// TODO:    {
// TODO:       textNmarginWidth, textCMarginWidth, XmRInt, sizeof(int),
// TODO:       XtOffset(TextWidget, text.marginWidth), XmRString, "5"
// TODO:    },
// TODO:    {
// TODO:       textNmarginHeight, textCMarginHeight, XmRInt, sizeof(int),
// TODO:       XtOffset(TextWidget, text.marginHeight), XmRString, "5"
// TODO:    },
// TODO:    {
// TODO:       textNpendingDelete, textCPendingDelete, XmRBoolean, sizeof(bool),
// TODO:       XtOffset(TextWidget, text.pendingDelete), XmRString, "True"
// TODO:    },
// TODO:    {
// TODO:       textNautoWrap, textCAutoWrap, XmRBoolean, sizeof(bool),
// TODO:       XtOffset(TextWidget, text.autoWrap), XmRString, "True"
// TODO:    },
// TODO:    {
// TODO:       textNcontinuousWrap, textCContinuousWrap, XmRBoolean, sizeof(bool),
// TODO:       XtOffset(TextWidget, text.continuousWrap), XmRString, "True"
// TODO:    },
// TODO:    {
// TODO:       textNautoIndent, textCAutoIndent, XmRBoolean, sizeof(bool),
// TODO:       XtOffset(TextWidget, text.autoIndent), XmRString, "True"
// TODO:    },
// TODO:    {
// TODO:       textNsmartIndent, textCSmartIndent, XmRBoolean, sizeof(bool),
// TODO:       XtOffset(TextWidget, text.smartIndent), XmRString, "false"
// TODO:    },
// TODO:    {
// TODO:       textNoverstrike, textCOverstrike, XmRBoolean, sizeof(bool),
// TODO:       XtOffset(TextWidget, text.overstrike), XmRString, "false"
// TODO:    },
// TODO:    {
// TODO:       textNheavyCursor, textCHeavyCursor, XmRBoolean, sizeof(bool),
// TODO:       XtOffset(TextWidget, text.heavyCursor), XmRString, "false"
// TODO:    },
// TODO:    {
// TODO:       textNreadOnly, textCReadOnly, XmRBoolean, sizeof(bool),
// TODO:       XtOffset(TextWidget, text.readOnly), XmRString, "false"
// TODO:    },
// TODO:    {
// TODO:       textNhidePointer, textCHidePointer, XmRBoolean, sizeof(bool),
// TODO:       XtOffset(TextWidget, text.hidePointer), XmRString, "false"
// TODO:    },
// TODO:    {
// TODO:       textNwrapMargin, textCWrapMargin, XmRInt, sizeof(int),
// TODO:       XtOffset(TextWidget, text.wrapMargin), XmRString, "0"
// TODO:    },
// TODO:    {
// TODO:       textNhScrollBar, textCHScrollBar, XmRWidget, sizeof(Fl_Widget*),
// TODO:       XtOffset(TextWidget, text.hScrollBar), XmRString, ""
// TODO:    },
// TODO:    {
// TODO:       textNvScrollBar, textCVScrollBar, XmRWidget, sizeof(Fl_Widget*),
// TODO:       XtOffset(TextWidget, text.vScrollBar), XmRString, ""
// TODO:    },
// TODO:    {
// TODO:       textNlineNumCols, textCLineNumCols, XmRInt, sizeof(int),
// TODO:       XtOffset(TextWidget, text.lineNumCols), XmRString, "0"
// TODO:    },
// TODO:    {
// TODO:       textNautoShowInsertPos, textCAutoShowInsertPos, XmRBoolean,
// TODO:       sizeof(bool), XtOffset(TextWidget, text.autoShowInsertPos),
// TODO:       XmRString, "True"
// TODO:    },
// TODO:    {
// TODO:       textNautoWrapPastedText, textCAutoWrapPastedText, XmRBoolean,
// TODO:       sizeof(bool), XtOffset(TextWidget, text.autoWrapPastedText),
// TODO:       XmRString, "false"
// TODO:    },
// TODO:    {
// TODO:       textNwordDelimiters, textCWordDelimiters, XmRString, sizeof(char*),
// TODO:       XtOffset(TextWidget, text.delimiters), XmRString,
// TODO:       ".,/\\`'!@#%^&*()-=+{}[]\":;<>?"
// TODO:    },
// TODO:    {
// TODO:       textNblinkRate, textCBlinkRate, XmRInt, sizeof(int),
// TODO:       XtOffset(TextWidget, text.cursorBlinkRate), XmRString, "500"
// TODO:    },
// TODO:    {
// TODO:       textNemulateTabs, textCEmulateTabs, XmRInt, sizeof(int),
// TODO:       XtOffset(TextWidget, text.emulateTabs), XmRString, "0"
// TODO:    },
// TODO:    {
// TODO:       textNfocusCallback, textCFocusCallback, XmRCallback, sizeof(caddr_t),
// TODO:       XtOffset(TextWidget, text.focusInCB), XtRCallback, NULL
// TODO:    },
// TODO:    {
// TODO:       textNlosingFocusCallback, textCLosingFocusCallback, XmRCallback,
// TODO:       sizeof(caddr_t), XtOffset(TextWidget, text.focusOutCB), XtRCallback,NULL
// TODO:    },
// TODO:    {
// TODO:       textNcursorMovementCallback, textCCursorMovementCallback, XmRCallback,
// TODO:       sizeof(caddr_t), XtOffset(TextWidget, text.cursorCB), XtRCallback, NULL
// TODO:    },
// TODO:    {
// TODO:       textNdragStartCallback, textCDragStartCallback, XmRCallback,
// TODO:       sizeof(caddr_t), XtOffset(TextWidget, text.dragStartCB), XtRCallback,
// TODO:       NULL
// TODO:    },
// TODO:    {
// TODO:       textNdragEndCallback, textCDragEndCallback, XmRCallback,
// TODO:       sizeof(caddr_t), XtOffset(TextWidget, text.dragEndCB), XtRCallback, NULL
// TODO:    },
// TODO:    {
// TODO:       textNsmartIndentCallback, textCSmartIndentCallback, XmRCallback,
// TODO:       sizeof(caddr_t), XtOffset(TextWidget, text.smartIndentCB), XtRCallback,
// TODO:       NULL
// TODO:    },
// TODO:    {
// TODO:       textNcursorVPadding, textCCursorVPadding, XtRCardinal, sizeof(int),
// TODO:       XtOffset(TextWidget, text.cursorVPadding), XmRString, "0"
// TODO:    }
// TODO: };
// TODO: 
// TODO: static TextClassRec textClassRec =
// TODO: {
// TODO:    /* CoreClassPart */
// TODO:    {
// TODO:       (WidgetClass)& xmPrimitiveClassRec,  /* superclass       */
// TODO:       "Text",                         /* class_name            */
// TODO:       sizeof(TextRec),                /* widget_size           */
// TODO:       NULL,                           /* class_initialize      */
// TODO:       NULL,                           /* class_part_initialize */
// TODO:       FALSE,                          /* class_inited          */
// TODO:       (XtInitProc)initialize,         /* initialize            */
// TODO:       NULL,                           /* initialize_hook       */
// TODO:       realize,   		            /* realize               */
// TODO:       actionsList,                    /* actions               */
// TODO:       XtNumber(actionsList),          /* num_actions           */
// TODO:       resources,                      /* resources             */
// TODO:       XtNumber(resources),            /* num_resources         */
// TODO:       NULLQUARK,                      /* xrm_class             */
// TODO:       TRUE,                           /* compress_motion       */
// TODO:       TRUE,                           /* compress_exposure     */
// TODO:       TRUE,                           /* compress_enterleave   */
// TODO:       FALSE,                          /* visible_interest      */
// TODO:       (XtWidgetProc)destroy,          /* destroy               */
// TODO:       (XtWidgetProc)resize,           /* resize                */
// TODO:       (XtExposeProc)redisplay,        /* expose                */
// TODO:       (XtSetValuesFunc)setValues,     /* set_values            */
// TODO:       NULL,                           /* set_values_hook       */
// TODO:       XtInheritSetValuesAlmost,       /* set_values_almost     */
// TODO:       NULL,                           /* get_values_hook       */
// TODO:       NULL,                           /* accept_focus          */
// TODO:       XtVersion,                      /* version               */
// TODO:       NULL,                           /* callback private      */
// TODO:       defaultTranslations,            /* tm_table              */
// TODO:       queryGeometry,                  /* query_geometry        */
// TODO:       NULL,                           /* display_accelerator   */
// TODO:       NULL,                           /* extension             */
// TODO:    },
// TODO:    /* Motif primitive class fields */
// TODO:    {
// TODO:       (XtWidgetProc)_XtInherit,   	/* Primitive border_highlight   */
// TODO:       (XtWidgetProc)_XtInherit,   	/* Primitive border_unhighlight */
// TODO:       NULL, /*XtInheritTranslations,*/	/* translations                 */
// TODO:       NULL,				/* arm_and_activate             */
// TODO:       NULL,				/* get resources      		*/
// TODO:       0,					/* num get_resources  		*/
// TODO:       NULL,         			/* extension                    */
// TODO:    },
// TODO:    /* Text class part */
// TODO:    {
// TODO:       0,                              	/* ignored	                */
// TODO:    }
// TODO: };
// TODO: 
// TODO: WidgetClass textWidgetClass = (WidgetClass)& textClassRec;
// TODO: #define NEDIT_HIDE_CURSOR_MASK (KeyPressMask)
// TODO: #define NEDIT_SHOW_CURSOR_MASK (FocusChangeMask | PointerMotionMask | ButtonMotionMask | ButtonPressMask | ButtonReleaseMask)
// TODO: static char empty_bits[] = {0x00, 0x00, 0x00, 0x00};
// TODO: static Cursor empty_cursor = 0;

//// --------------------------------------------------------------------------
static bool InstallTextActions(Ne_AppContext& context)
{
   NeAppAddActions(context, actionsList, ARRAY_SIZE(actionsList));
   return true;
}

static bool initTextActions = false;

// --------------------------------------------------------------------------
Ne_Text_Editor::Ne_Text_Editor(int x, int y, int w, int h, const char* l)
   : Ne_Text_Display(x, y, w, h, l),
   MultiClickTime(400)
{
   if (!initTextActions)
      InstallTextActions(AppContext); // Damned ugly...
}

// --------------------------------------------------------------------------
int Ne_Text_Editor::handle(int e)
{
   //printf("Ne_Text_Editor::handle [%d][%s]\n", e, fl_eventnames[e]);

   switch (e)
   {
   case FL_FOCUS:
      //show_cursor(mCursorOn); // redraws the cursor
      //if (buffer()->selected()) redraw(); // Redraw selections...
      focusInAP(this, e, NULL, 0);
      return 1;

   case FL_UNFOCUS:
      //show_cursor(mCursorOn); // redraws the cursor
      //if (buffer()->selected()) redraw(); // Redraw selections...
   case FL_HIDE:
      focusOutAP(this, e, NULL, 0);
      //if (when() & FL_WHEN_RELEASE) maybe_do_callback();
      return 1;

   case FL_PUSH:
      if (inWindow(Fl::event_x(), Fl::event_y()))
      {
         take_focus();
         return handle_key_button(e);
      }
      break;

   case FL_DRAG:
      return handle_key_button(FL_MOVE);

   case FL_RELEASE:
   case FL_KEYBOARD:
      //if (active_r() && window() && this == Fl::belowmouse()) 
      //  window()->cursor(FL_CURSOR_NONE);
      return handle_key_button(e);

      //printf("button [%d] # of Clicks [%d]\n", Fl::event_button(), Fl::event_clicks());
      //if (Fl::event_button() == 2) {
      //   // don't let the text_display see this event
      //   if (Fl_Group::handle(event)) return 1;
      //   dragType = DRAG_NONE;
      //   if(buffer()->selected()) {
      //      buffer()->unselect();
      //   }
      //   int pos = xy_to_position(Fl::event_x(), Fl::event_y(), CURSOR_POS);
      //   insert_position(pos);
      //   Fl::paste(*this, 0);
      //   Fl::focus(this);
      //   set_changed();
      //   if (when()&FL_WHEN_CHANGED) do_callback();
      //   return 1;
      //}
      //break;

   case FL_PASTE:
   {
      insertClipboard(this);
      return 1;

      //buffer()->remove_selection();
      //if (insert_mode()) insert(Fl::event_text());
      //else overstrike(Fl::event_text());
      //show_insert_position();
      //set_changed();
      //if (when()&FL_WHEN_CHANGED) do_callback();
      return 1;
   }
   case FL_ENTER:
      // MRS: WIN32 only?  Need to test!
      //    case FL_MOVE:
      //show_cursor(mCursorOn);
      return 1;

   case FL_SHORTCUT:
      //if (!(shortcut() ? Fl::test_shortcut(shortcut()) : test_shortcut()))
      //   return 0;
      //if (Fl::visible_focus() && handle(FL_FOCUS)) {
      //   Fl::focus(this);
      //   return 1;
      //}
      break;

      // Handle drag'n'drop attempt by the user. This is a simplified 
      // implementation which allows dnd operations onto the scroll bars.
   case FL_DND_ENTER: // save the current cursor position
      //if (Fl::visible_focus() && handle(FL_FOCUS))
      //   Fl::focus(this);
      //show_cursor(mCursorOn);
      //dndCursorPos = insert_position();
      /* fall through */
   case FL_DND_DRAG: // show a temporary insertion cursor
      //insert_position(xy_to_position(Fl::event_x(), Fl::event_y(), CURSOR_POS));
      return 1;      
   case FL_DND_LEAVE: // restore original cursor
      //insert_position(dndCursorPos);
      return 1;      
   case FL_DND_RELEASE: // keep insertion cursor and wait for the FL_PASTE event
      //buffer()->unselect(); // FL_PASTE must not destroy current selection!
      return 1;
   }

   return Ne_Text_Display::handle(e);
}

// --------------------------------------------------------------------------
int Ne_Text_Editor::handle_key_button(int event)
{
   int del = 0;
   if (event == FL_KEYBOARD && Fl::compose(del))
   {  
      selfInsertAP(this, event, NULL, 0);
      redraw();
      return 1;
   }

   EventKeyButtonStateCommand currentState;
   currentState.event = event;
   currentState.key = Fl::event_key();
   currentState.button = 0;
   if (Fl::event_button() == FL_LEFT_MOUSE || (Fl::event_state() & FL_BUTTON1)) currentState.button = 1;
   else if (Fl::event_button() == FL_MIDDLE_MOUSE || (Fl::event_state() & FL_BUTTON2)) currentState.button = 2;
   else if (Fl::event_button() == FL_RIGHT_MOUSE || (Fl::event_state() & FL_BUTTON3)) currentState.button = 3;
   if (currentState.button) currentState.key = 0;
   currentState.shift = (Fl::event_state() & FL_SHIFT?1:0);
   currentState.ctrl = (Fl::event_state() & FL_CTRL?1:0);
   currentState.alt = (Fl::event_state() & FL_ALT?1:0);
   currentState.meta = (Fl::event_state() & FL_META?1:0);

   //printf("handle_key [%d] [Key:%d] [State:%d %d %d %d] [Button:%d]\n",
   //      del,
   //      currentState.key,
   //      currentState.shift, currentState.ctrl, currentState.alt, currentState.meta,
   //      currentState.button);

   for(KeyStateTranslationTable::const_iterator cmd = KeyStateTranslations.begin(); cmd != KeyStateTranslations.end(); ++cmd)
   {
      if (currentState.match(*cmd))
      {  // Execute the command
         const std::string& command = cmd->command;
         char* parsedCommand = new char[command.size()+1];
         strcpy(parsedCommand, command.c_str());
         std::string::size_type sep = command.find('(');
         if (sep == std::string::npos)
            return 1; // bad command format : function( [param1][,param2]... )
         parsedCommand[sep] = '\0';

         std::vector<std::string> args = Split(command.substr(sep+1), ",");
         for(std::vector<std::string>::iterator arg = args.begin(); arg != args.end(); ++arg)
            *arg = Trim(*arg, "\")(");

         const char** params = new const char*[args.size()];
         for(std::vector<std::string>::size_type i = 0; i < args.size(); ++i)
            params[i] = args[i].c_str();

         // We must translate the string to a function name with args....
         AppContext.callAction(this, parsedCommand, event, params, args.size());

         delete[] parsedCommand;
         delete[] params;

         redraw();
         return 1;
      }
   }

   return 0;
}

// --------------------------------------------------------------------------
bool Ne_Text_Editor::inWindow(int x, int y) const
{
   return ((x >= this->x() && x <= this->x() + this->width + this->lineNumWidth + 2*marginWidth)
      && (y >= this->top && y <= this->top + this->height));
}

// --------------------------------------------------------------------------
bool Ne_Text_Editor::mouseMoveForDrag(int x, int y) const
{
   return (abs(x - this->text.btnDownX) > NE_SELECT_THRESHOLD || abs(y - this->text.btnDownY) > NE_SELECT_THRESHOLD);
}

// -------------------------------------------------------------------------------
void TextInitialize(Ne_Text_Editor* textD)
{
   Ne_Font* fs = &textD->primaryFont;
   Fl_Color white, black;
   int textLeft;
   int charWidth = fs->max_width();
   int marginWidth = textD->marginWidth;
   int lineNumCols = textD->text.lineNumCols;

// TODO:    /* Set the initial window size based on the rows and columns resources */
// TODO:    if (request->core.width == 0)
// TODO:       newWidget->core.width = charWidth * newWidget->text.columns + marginWidth*2 +
// TODO:       (lineNumCols == 0 ? 0 : marginWidth + charWidth * lineNumCols);
// TODO:    if (request->core.height == 0)
// TODO:       newWidget->core.height = (fs->ascent + fs->descent) * newWidget->text.rows +
// TODO:       newWidget->text.marginHeight * 2;

   /* The default colors work for B&W as well as color, except for
   selectFGPixel and selectBGPixel, where color highlighting looks
   much better without reverse video, so if we get here, and the
   selection is totally unreadable because of the bad default colors,
   swap the colors and make the selection reverse video */
   white = FL_WHITE;
   black = FL_BLACK;
// TODO:    if (textD->text.selectBGPixel == white &&
// TODO:       textD->core.background_pixel == white &&
// TODO:       textD->text.selectFGPixel == black &&
// TODO:       textD->primitive.foreground == black)
// TODO:    {
// TODO:       textD->text.selectBGPixel = black;
// TODO:       textD->text.selectFGPixel = white;
// TODO:    }

   /* Create the initial text buffer for the widget to display (which can
   be replaced later with TextSetBuffer) */
   Ne_Text_Buffer* buf = BufCreate();

   /* Create and initialize the text-display part of the widget */
   textLeft = textD->marginWidth + (lineNumCols == 0 ? 0 : marginWidth + charWidth * lineNumCols);
   
// TODO:    newWidget->text.textD = TextDCreate((Fl_Widget*)newWidget, newWidget->text.hScrollBar,
// TODO:       newWidget->text.vScrollBar, textLeft, newWidget->text.marginHeight,
// TODO:       newWidget->core.width - marginWidth - textLeft,
// TODO:       newWidget->core.height - newWidget->text.marginHeight * 2,
// TODO:       lineNumCols == 0 ? 0 : marginWidth,
// TODO:       lineNumCols == 0 ? 0 : lineNumCols * charWidth,
// TODO:       buf, newWidget->text.fontStruct, newWidget->core.background_pixel,
// TODO:       newWidget->primitive.foreground, newWidget->text.selectFGPixel,
// TODO:       newWidget->text.selectBGPixel, newWidget->text.highlightFGPixel,
// TODO:       newWidget->text.highlightBGPixel, newWidget->text.cursorFGPixel,
// TODO:       newWidget->text.lineNumFGPixel,
// TODO:       newWidget->text.continuousWrap, newWidget->text.wrapMargin,
// TODO:       newWidget->text.backlightCharTypes, newWidget->text.calltipFGPixel,
// TODO:       newWidget->text.calltipBGPixel);

   textD->cursorOn = true;
   textD->cursorPos = 0;
   textD->cursorX = -100;
   textD->cursorY = -100;
   textD->cursorToHint = NO_HINT;
   textD->cursorStyle = NE_NORMAL_CURSOR;
   textD->cursorPreferredCol = -1;
   textD->buffer = buf;
   textD->firstChar = 0;
   textD->lastChar = 0;
   textD->nBufferLines = 0;
   textD->topLineNum = 1;
   textD->absTopLineNum = 1;
   textD->needAbsTopLineNum = false;
   textD->horizOffset = 0;
   // TODO:    textD->visibility = VisibilityUnobscured;
// TODO:    textD->fontStruct = textD->text.fontStruct;
   textD->ascent = textD->primaryFont.ascent();
   textD->descent = textD->primaryFont.descent();
   textD->fixedFontWidth = textD->primaryFont.isFixed() ? textD->primaryFont.max_width() : -1;
   textD->styleBuffer = NULL;
   textD->styleTable = NULL;
   textD->nStyles = 0;
   textD->bgPixel = FL_BACKGROUND_COLOR;
   textD->fgPixel = FL_FOREGROUND_COLOR;
   textD->selectFGPixel = GetColor(NEDIT_DEFAULT_SEL_FG);
   textD->highlightFGPixel = GetColor(NEDIT_DEFAULT_HI_FG);
   textD->selectBGPixel = GetColor(NEDIT_DEFAULT_SEL_BG);
   textD->highlightBGPixel = GetColor(NEDIT_DEFAULT_HI_BG);
   textD->lineNumFGPixel = GetColor(NEDIT_DEFAULT_LINENO_FG);
   textD->cursorFGPixel = GetColor(NEDIT_DEFAULT_CURSOR_FG);
   textD->lineNumWidth = std::max(60, textLeft); // TODO: textD->width - marginWidth - textLeft;
   textD->computeTextAreaSize(textD->x(), textD->y(), textD->w(), textD->h());

   textD->nVisibleLines = (textD->height - 1) / (textD->ascent + textD->descent);
   if (textD->nVisibleLines == 0) ++textD->nVisibleLines; // Avoid 0 array allocation
   textD->lineStarts = (int*)malloc__(sizeof(int) * textD->nVisibleLines);
   textD->lineStarts[0] = 0;

   textD->calltipW = NULL;
   textD->calltipShell = NULL;
   textD->calltip.ID = 0;
   textD->calltipFGPixel = GetColor(NEDIT_DEFAULT_CALLTIP_FG);
   textD->calltipBGPixel = GetColor(NEDIT_DEFAULT_CALLTIP_BG);
   for (int i = 1; i < textD->nVisibleLines; i++)
      textD->lineStarts[i] = -1;

   textD->bgClassPixel = NULL;
   textD->bgClass = NULL;
// TODO:    TextDSetupBGClasses(textD, bgClassString, &textD->bgClassPixel, &textD->bgClass, bgPixel);
   textD->suppressResync = 0;
   textD->nLinesDeleted = 0;
   textD->modifyingTabDist = 0;
   textD->pointerHidden = false;

   /* Attach the callback to the text buffer for receiving modification information */
   if (textD->buffer != NULL)
      TextDSetBuffer(textD, textD->buffer); // TODO... to replace by CreateD...

   /* Add mandatory delimiters blank, tab, and newline to the list of
   delimiters.  The memory use scheme here is that new values are
   always copied, and can therefore be safely freed on subsequent
   set-values calls or destroy */
   textD->text.delimiters += " \t\n";

   /* Start with the cursor blanked (widgets don't have focus on creation,
   the initial FocusIn event will unblank it and get blinking started) */
   textD->cursorOn = false;

   /* Initialize the widget variables */
   textD->text.autoScrollProcID = 0;
   textD->text.cursorBlinkProcID = 0;
   textD->text.dragState = NOT_CLICKED;
   textD->text.multiClickState = NO_CLICKS;
   textD->text.lastBtnDown = 0;
   textD->text.selectionOwner = false;
   textD->text.motifDestOwner = false;
   textD->text.emTabsBeforeCursor = 0;

// TODO: #ifndef NO_XMIM
// TODO:    /* Register the widget to the input manager */
// TODO:    XmImRegister((Fl_Widget*)newWidget, 0);
// TODO:    /* In case some Resources for the IC need to be set, add them below */
// TODO:    XmImVaSetValues((Fl_Widget*)newWidget, NULL);
// TODO: #endif

// TODO:    XtAddEventHandler((Fl_Widget*)newWidget, GraphicsExpose, True, (XtEventHandler)redisplayGE, (Opaque)NULL);

// TODO:    if (newWidget->text.hidePointer)
// TODO:    {
// TODO:       Display* theDisplay;
// TODO:       Pixmap empty_pixmap;
// TODO:       XColor black_color;
// TODO:       /* Set up the empty Cursor */
// TODO:       if (empty_cursor == 0)
// TODO:       {
// TODO:          theDisplay = XtDisplay((Fl_Widget*)newWidget);
// TODO:          empty_pixmap = XCreateBitmapFromData(theDisplay,
// TODO:             RootWindowOfScreen(XtScreen((Fl_Widget*)newWidget)), empty_bits, 1, 1);
// TODO:          XParseColor(theDisplay, DefaultColormapOfScreen(XtScreen((Fl_Widget*)newWidget)),
// TODO:             "black", &black_color);
// TODO:          empty_cursor = XCreatePixmapCursor(theDisplay, empty_pixmap,
// TODO:             empty_pixmap, &black_color, &black_color, 0, 0);
// TODO:       }
// TODO: 
// TODO:       /* Add event handler to hide the pointer on keypresses */
// TODO:       XtAddEventHandler((Fl_Widget*)newWidget, NEDIT_HIDE_CURSOR_MASK, false, handleHidePointer, (Opaque)NULL);
// TODO:    }
}

// -------------------------------------------------------------------------------
Ne_Text_Editor* TextCreate(int x, int y, int w, int h,
                      int lineNumWidth, Ne_Text_Buffer* buffer,
                      const Ne_Font& fontStruct, Fl_Color bgPixel, Fl_Color fgPixel,
                      Fl_Color selectFGPixel, Fl_Color selectBGPixel, Fl_Color highlightFGPixel,
                      Fl_Color highlightBGPixel, Fl_Color cursorFGPixel, Fl_Color lineNumFGPixel,
                      int continuousWrap, int wrapMargin, const char* bgClassString,
                      Fl_Color calltipFGPixel, Fl_Color calltipBGPixel)
{
   Ne_Text_Editor* textD = new Ne_Text_Editor(x, y, w, h);
   textD->cursorOn = true;
   textD->cursorPos = 0;
   textD->cursorX = -100;
   textD->cursorY = -100;
   textD->cursorToHint = NO_HINT;
   textD->cursorStyle = NE_NORMAL_CURSOR;
   textD->cursorPreferredCol = -1;
   textD->buffer = buffer;
   textD->firstChar = 0;
   textD->lastChar = 0;
   textD->nBufferLines = 0;
   textD->topLineNum = 1;
   textD->absTopLineNum = 1;
   textD->needAbsTopLineNum = false;
   textD->horizOffset = 0;
// TODO:    textD->visibility = VisibilityUnobscured;
   textD->primaryFont = fontStruct;
   textD->ascent = fontStruct.ascent();
   textD->descent = fontStruct.descent();
   textD->fixedFontWidth = fontStruct.isFixed() ? fontStruct.max_width() : -1;
   textD->styleBuffer = NULL;
   textD->styleTable = NULL;
   textD->nStyles = 0;
   textD->bgPixel = bgPixel;
   textD->fgPixel = fgPixel;
   textD->selectFGPixel = selectFGPixel;
   textD->highlightFGPixel = highlightFGPixel;
   textD->selectBGPixel = selectBGPixel;
   textD->highlightBGPixel = highlightBGPixel;
   textD->lineNumFGPixel = lineNumFGPixel;
   textD->cursorFGPixel = cursorFGPixel;
   textD->wrapMargin = wrapMargin;
   textD->continuousWrap = continuousWrap;
   textD->lineNumWidth = lineNumWidth;
   textD->computeTextAreaSize(x, y, w, h);
   
   textD->nVisibleLines = (textD->height - 1) / (textD->ascent + textD->descent);
   if (textD->nVisibleLines == 0) ++textD->nVisibleLines; // Avoid 0 array allocation
   textD->lineStarts = (int*)malloc__(sizeof(int) * textD->nVisibleLines);
   textD->lineStarts[0] = 0;

   textD->calltipW = NULL;
   textD->calltipShell = NULL;
   textD->calltip.ID = 0;
   textD->calltipFGPixel = calltipFGPixel;
   textD->calltipBGPixel = calltipBGPixel;
   for (int i = 1; i < textD->nVisibleLines; i++)
      textD->lineStarts[i] = -1;

   textD->bgClassPixel = NULL;
   textD->bgClass = NULL;
   TextDSetupBGClasses(textD, bgClassString, &textD->bgClassPixel, &textD->bgClass, bgPixel);
   textD->suppressResync = 0;
   textD->nLinesDeleted = 0;
   textD->modifyingTabDist = 0;
   textD->pointerHidden = false;

   /* Attach the callback to the text buffer for receiving modification information */
   if (buffer != NULL)
      TextDSetBuffer(textD, textD->buffer); // TODO... to replace by CreateD...

   return textD;
}


// TODO: /* Hide the pointer while the user is typing */
// TODO: static void handleHidePointer(Fl_Widget* w, XtPointer unused,
// TODO:                               int event, bool* continue_to_dispatch)
// TODO: {
// TODO:    TextWidget tw = (TextWidget) w;
// TODO:    ShowHidePointer(tw, True);
// TODO: }
// TODO: 
// TODO: /* Restore the pointer if the mouse moves or focus changes */
// TODO: static void handleShowPointer(Fl_Widget* w, XtPointer unused,
// TODO:                               int event, bool* continue_to_dispatch)
// TODO: {
// TODO:    TextWidget tw = (TextWidget) w;
// TODO:    ShowHidePointer(tw, false);
// TODO: }
// TODO: 
// TODO: void ShowHidePointer(TextWidget w, bool hidePointer)
// TODO: {
// TODO:    if (w->text.hidePointer)
// TODO:    {
// TODO:       if (hidePointer != w->text.textD->pointerHidden)
// TODO:       {
// TODO:          if (hidePointer)
// TODO:          {
// TODO:             /* Don't listen for keypresses any more */
// TODO:             XtRemoveEventHandler((Fl_Widget*)w, NEDIT_HIDE_CURSOR_MASK, false,
// TODO:                                  handleHidePointer, (Opaque)NULL);
// TODO:             /* Switch to empty cursor */
// TODO:             XDefineCursor(XtDisplay(w), XtWindow(w), empty_cursor);
// TODO: 
// TODO:             w->text.textD->pointerHidden = True;
// TODO: 
// TODO:             /* Listen to mouse movement, focus change, and button presses */
// TODO:             XtAddEventHandler((Fl_Widget*)w, NEDIT_SHOW_CURSOR_MASK,
// TODO:                               false, handleShowPointer, (Opaque)NULL);
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             /* Don't listen to mouse/focus events any more */
// TODO:             XtRemoveEventHandler((Fl_Widget*)w, NEDIT_SHOW_CURSOR_MASK,
// TODO:                                  false, handleShowPointer, (Opaque)NULL);
// TODO:             /* Switch to regular cursor */
// TODO:             XUndefineCursor(XtDisplay(w), XtWindow(w));
// TODO: 
// TODO:             w->text.textD->pointerHidden = false;
// TODO: 
// TODO:             /* Listen for keypresses now */
// TODO:             XtAddEventHandler((Fl_Widget*)w, NEDIT_HIDE_CURSOR_MASK, false,
// TODO:                               handleHidePointer, (Opaque)NULL);
// TODO:          }
// TODO:       }
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Fl_Widget* destroy method
// TODO: */
// TODO: static void destroy(TextWidget w)
// TODO: {
// TODO:    Ne_Text_Buffer* buf;
// TODO: 
// TODO:    /* Free the text display and possibly the attached buffer.  The buffer
// TODO:       is freed only if after removing all of the modify procs (by calling
// TODO:       StopHandlingXSelections and TextDFree) there are no modify procs
// TODO:       left */
// TODO:    StopHandlingXSelections((Fl_Widget*)w);
// TODO:    buf = w->text.textD->buffer;
// TODO:    TextDFree(w->text.textD);
// TODO:    if (buf->nModifyProcs == 0)
// TODO:       BufFree(buf);
// TODO: 
// TODO:    if (w->text.cursorBlinkProcID != 0)
// TODO:       XtRemoveTimeOut(w->text.cursorBlinkProcID);
// TODO:    free__(w->text.delimiters);
// TODO:    XtRemoveAllCallbacks((Fl_Widget*)w, textNfocusCallback);
// TODO:    XtRemoveAllCallbacks((Fl_Widget*)w, textNlosingFocusCallback);
// TODO:    XtRemoveAllCallbacks((Fl_Widget*)w, textNcursorMovementCallback);
// TODO:    XtRemoveAllCallbacks((Fl_Widget*)w, textNdragStartCallback);
// TODO:    XtRemoveAllCallbacks((Fl_Widget*)w, textNdragEndCallback);
// TODO: 
// TODO: #ifndef NO_XMIM
// TODO:    /* Unregister the widget from the input manager */
// TODO:    XmImUnregister((Fl_Widget*)w);
// TODO: #endif
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Fl_Widget* resize method.  Called when the size of the widget changes
// TODO: */
// TODO: static void resize(TextWidget w)
// TODO: {
// TODO:    XFontStruct* fs = w->text.fontStruct;
// TODO:    int height = w->core.height, width = w->core.width;
// TODO:    int marginWidth = w->text.marginWidth, marginHeight = w->text.marginHeight;
// TODO:    int lineNumAreaWidth = w->text.lineNumCols == 0 ? 0 : w->text.marginWidth +
// TODO:                           fs->max_bounds.width * w->text.lineNumCols;
// TODO: 
// TODO:    w->text.columns = (width - marginWidth*2 - lineNumAreaWidth) /
// TODO:                      fs->max_bounds.width;
// TODO:    w->text.rows = (height - marginHeight*2) / (fs->ascent + fs->descent);
// TODO: 
// TODO:    /* Reject widths and heights less than a character, which the text
// TODO:       display can't tolerate.  This is not strictly legal, but I've seen
// TODO:       it done in other widgets and it seems to do no serious harm.  NEdit
// TODO:       prevents panes from getting smaller than one line, but sometimes
// TODO:       splitting windows on Linux 2.0 systems (same Motif, why the change in
// TODO:       behavior?), causes one or two resize calls with < 1 line of height.
// TODO:       Fixing it here is 100x easier than re-designing textDisp.c */
// TODO:    if (w->text.columns < 1)
// TODO:    {
// TODO:       w->text.columns = 1;
// TODO:       w->core.width = width = fs->max_bounds.width + marginWidth*2 +
// TODO:                               lineNumAreaWidth;
// TODO:    }
// TODO:    if (w->text.rows < 1)
// TODO:    {
// TODO:       w->text.rows = 1;
// TODO:       w->core.height = height = fs->ascent + fs->descent + marginHeight*2;
// TODO:    }
// TODO: 
// TODO:    /* Resize the text display that the widget uses to render text */
// TODO:    TextDResize(w->text.textD, width - marginWidth*2 - lineNumAreaWidth,
// TODO:                height - marginHeight*2);
// TODO: 
// TODO:    /* if the window became shorter or narrower, there may be text left
// TODO:       in the bottom or right margin area, which must be cleaned up */
// TODO:    if (XtIsRealized((Fl_Widget*)w))
// TODO:    {
// TODO:       XClearArea(XtDisplay(w), XtWindow(w), 0, height-marginHeight,
// TODO:                  width, marginHeight, false);
// TODO:       XClearArea(XtDisplay(w), XtWindow(w),width-marginWidth,
// TODO:                  0, marginWidth, height, false);
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Fl_Widget* redisplay method
// TODO: */
// TODO: static void redisplay(TextWidget w, int event, Region region)
// TODO: {
// TODO:    XExposeEvent* e = &event->xexpose;
// TODO: 
// TODO:    TextDRedisplayRect(w->text.textD, e->x, e->y, e->width, e->height);
// TODO: }
// TODO: 
// TODO: static Bool findGraphicsExposeOrNoExposeEvent(Display* theDisplay, int event, XPointer arg)
// TODO: {
// TODO:    if ((theDisplay == event->xany.display) &&
// TODO:          (event->type == GraphicsExpose || event->type == NoExpose) &&
// TODO:          ((Fl_Widget*)arg == XtWindowToWidget(event->xany.display, event->xany.window)))
// TODO:    {
// TODO:       return(True);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       return(false);
// TODO:    }
// TODO: }
// TODO: 
// TODO: static void adjustRectForGraphicsExposeOrNoExposeEvent(TextWidget w, int event,
// TODO:       bool* first, int* left, int* top, int* width, int* height)
// TODO: {
// TODO:    bool removeQueueEntry = false;
// TODO: 
// TODO:    if (event->type == GraphicsExpose)
// TODO:    {
// TODO:       XGraphicsExposeEvent* e = &event->xgraphicsexpose;
// TODO:       int x = e->x, y = e->y;
// TODO: 
// TODO:       TextDImposeGraphicsExposeTranslation(w->text.textD, &x, &y);
// TODO:       if (*first)
// TODO:       {
// TODO:          *left = x;
// TODO:          *top = y;
// TODO:          *width = e->width;
// TODO:          *height = e->height;
// TODO: 
// TODO:          *first = false;
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          int prev_left = *left;
// TODO:          int prev_top = *top;
// TODO: 
// TODO:          *left = min(*left, x);
// TODO:          *top = min(*top, y);
// TODO:          *width = max(prev_left + *width, x + e->width) - *left;
// TODO:          *height = max(prev_top + *height, y + e->height) - *top;
// TODO:       }
// TODO:       if (e->count == 0)
// TODO:       {
// TODO:          removeQueueEntry = True;
// TODO:       }
// TODO:    }
// TODO:    else if (event->type == NoExpose)
// TODO:    {
// TODO:       removeQueueEntry = True;
// TODO:    }
// TODO:    if (removeQueueEntry)
// TODO:    {
// TODO:       TextDPopGraphicExposeQueueEntry(w->text.textD);
// TODO:    }
// TODO: }
// TODO: 
// TODO: static void redisplayGE(TextWidget w, XtPointer client_data,
// TODO:                         int event, bool* continue_to_dispatch_return)
// TODO: {
// TODO:    if (event->type == GraphicsExpose || event->type == NoExpose)
// TODO:    {
// TODO:       HandleAllPendingGraphicsExposeNoExposeEvents(w, event);
// TODO:    }
// TODO: }
// TODO: 
// TODO: void HandleAllPendingGraphicsExposeNoExposeEvents(TextWidget w, int event)
// TODO: {
// TODO:    XEvent foundEvent;
// TODO:    int left;
// TODO:    int top;
// TODO:    int width;
// TODO:    int height;
// TODO:    bool invalidRect = True;
// TODO: 
// TODO:    if (event)
// TODO:    {
// TODO:       adjustRectForGraphicsExposeOrNoExposeEvent(w, event, &invalidRect, &left, &top, &width, &height);
// TODO:    }
// TODO:    while (XCheckIfEvent(XtDisplay(w), &foundEvent, findGraphicsExposeOrNoExposeEvent, (XPointer)w))
// TODO:    {
// TODO:       adjustRectForGraphicsExposeOrNoExposeEvent(w, &foundEvent, &invalidRect, &left, &top, &width, &height);
// TODO:    }
// TODO:    if (!invalidRect)
// TODO:    {
// TODO:       TextDRedisplayRect(w->text.textD, left, top, width, height);
// TODO:    }
// TODO: }

/*
** Fl_Widget* setValues method
*/
bool TextSetValues(Ne_Text_Editor* current) //, Ne_Text_Editor* request, Ne_Text_Editor* newWidget)
{
   bool redraw = false, reconfigure = false;

// TODO:    if (newWidget->text.overstrike != current->text.overstrike)
   {
      if (current->cursorStyle == NE_BLOCK_CURSOR)
         TextDSetCursorStyle(current, current->text.heavyCursor ? NE_HEAVY_CURSOR : NE_NORMAL_CURSOR);
      else if (current->cursorStyle == NE_NORMAL_CURSOR || current->cursorStyle == NE_HEAVY_CURSOR)
         TextDSetCursorStyle(current, NE_BLOCK_CURSOR);
   }

// TODO:    if (newWidget->text.fontStruct != current->text.fontStruct)
   {
// TODO:       if (newWidget->text.lineNumCols != 0)
// TODO:          reconfigure = True;
      TextDSetFont(current, current->primaryFont);
   }

// TODO:    if (newWidget->text.wrapMargin != current->text.wrapMargin || newWidget->text.continuousWrap != current->text.continuousWrap)
      TextDSetWrapMode(current, current->continuousWrap, current->wrapMargin);

   /* When delimiters are changed, copy the memory, so that the caller
      doesn't have to manage it, and add mandatory delimiters blank,
      tab, and newline to the list */
// TODO:    if (newWidget->text.delimiters != current->text.delimiters)
   {
      current->text.delimiters += " \t\n";
   }

   /* Setting the lineNumCols resource tells the text widget to hide or
      show, or change the number of columns of the line number display,
      which requires re-organizing the x coordinates of both the line
      number display and the main text display */
// TODO:    if (newWidget->text.lineNumCols != current->text.lineNumCols || reconfigure)
   {
      int marginWidth = current->marginWidth;
      int charWidth = current->primaryFont.max_width();
      int lineNumCols = current->text.lineNumCols;
      if (lineNumCols == 0)
      {
         TextDSetLineNumberArea(current, 0, 0, marginWidth);
         //current->text.columns = (current->width - marginWidth*2) / charWidth;
      }
      else
      {
         TextDSetLineNumberArea(current, marginWidth,
                                charWidth * lineNumCols,
                                2*marginWidth + charWidth * lineNumCols);
         //current->text.columns = (current->width - marginWidth*3 - charWidth * lineNumCols) / charWidth;
      }
   }

// TODO:    if (newWidget->text.backlightCharTypes != current->text.backlightCharTypes)
   {
      TextDSetupBGClasses(current, current->text.backlightCharTypes.c_str(),
                          &current->bgClassPixel, &current->bgClass,
                          current->bgPixel);
      redraw = true;
   }

   return redraw;
}

// TODO: /*
// TODO: ** Fl_Widget* realize method
// TODO: */
// TODO: static void realize(Fl_Widget* w, XtValueMask* valueMask,
// TODO:                     XSetWindowAttributes* attributes)
// TODO: {
// TODO:    /* Set bit gravity window attribute.  This saves a full blank and redraw
// TODO:       on window resizing */
// TODO:    *valueMask |= CWBitGravity;
// TODO:    attributes->bit_gravity = NorthWestGravity;
// TODO: 
// TODO:    /* Continue with realize method from superclass */
// TODO:    (xmPrimitiveClassRec.core_class.realize)(w, valueMask, attributes);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Fl_Widget* query geometry method ... unless asked to negotiate a different size simply return current size.
// TODO: */
// TODO: static XtGeometryResult queryGeometry(Fl_Widget* w, XtWidgetGeometry* proposed,
// TODO:                                       XtWidgetGeometry* answer)
// TODO: {
// TODO:    TextWidget tw = (TextWidget)w;
// TODO: 
// TODO:    int curHeight = textD->core.height;
// TODO:    int curWidth = textD->core.width;
// TODO:    XFontStruct* fs = textD->text.textD->fontStruct;
// TODO:    int fontWidth = fs->max_bounds.width;
// TODO:    int fontHeight = fs->ascent + fs->descent;
// TODO:    int marginHeight = textD->text.marginHeight;
// TODO:    int propWidth = (proposed->request_mode & CWWidth) ? proposed->width : 0;
// TODO:    int propHeight = (proposed->request_mode & CWHeight) ? proposed->height : 0;
// TODO: 
// TODO:    answer->request_mode = CWHeight | CWWidth;
// TODO: 
// TODO:    if (proposed->request_mode & CWWidth)
// TODO:       /* Accept a width no smaller than 10 chars */
// TODO:       answer->width = max(fontWidth * 10, proposed->width);
// TODO:    else
// TODO:       answer->width = curWidth;
// TODO: 
// TODO:    if (proposed->request_mode & CWHeight)
// TODO:       /* Accept a height no smaller than an exact multiple of the line height
// TODO:          and at least one line high */
// TODO:       answer->height = max(1, ((propHeight - 2*marginHeight) / fontHeight)) *
// TODO:                        fontHeight + 2*marginHeight;
// TODO:    else
// TODO:       answer->height = curHeight;
// TODO: 
// TODO:    /*printf("propWidth %d, propHeight %d, ansWidth %d, ansHeight %d\n",
// TODO:    	    propWidth, propHeight, answer->width, answer->height);*/
// TODO:    if (propWidth == answer->width && propHeight == answer->height)
// TODO:       return XtGeometryYes;
// TODO:    else if (answer->width == curWidth && answer->height == curHeight)
// TODO:       return XtGeometryNo;
// TODO:    else
// TODO:       return XtGeometryAlmost;
// TODO: }

/*
** Set the text buffer which this widget will display and interact with.
** The currently attached buffer is automatically freed, ONLY if it has
** no additional modify procs attached (as it would if it were being
** displayed by another text widget).
*/
void TextSetBuffer(Ne_Text_Editor* textD, Ne_Text_Buffer* buffer)
{
   Ne_Text_Buffer* oldBuf = textD->buffer;

// TODO:    StopHandlingXSelections(w);
   TextDSetBuffer(textD, buffer);
   if (oldBuf && oldBuf->modifyProcs.empty())
      BufFree(oldBuf);
}

/*
** Get the buffer associated with this text widget.  Note that attaching
** additional modify callbacks to the buffer will prevent it from being
** automatically freed when the widget is destroyed.
*/
Ne_Text_Buffer* TextGetBuffer(Ne_Text_Editor* textD)
{
   return textD->buffer;
}

/*
** Translate a line number and column into a position
*/
int TextLineAndColToPos(Ne_Text_Editor* textD, int lineNum, int column)
{
   return TextDLineAndColToPos(textD, lineNum, column);
}

/*
** Translate a position into a line number (if the position is visible,
** if it's not, return false
*/
int TextPosToLineAndCol(Ne_Text_Editor* textD, int pos, int* lineNum, int* column)
{
   return TextDPosToLineAndCol(textD, pos, lineNum, column);
}

/*
** Translate a buffer text position to the XY location where the center
** of the cursor would be positioned to point to that character.  Returns
** false if the position is not displayed because it is VERTICALLY out
** of view.  If the position is horizontally out of view, returns the
** x coordinate where the position would be if it were visible.
*/
int TextPosToXY(Ne_Text_Editor* textD, int pos, int* x, int* y)
{
   return TextDPositionToXY(textD, pos, x, y);
}

/*
** Return the cursor position
*/
int TextGetCursorPos(Ne_Text_Editor* textD)
{
   return TextDGetInsertPosition(textD);
}

/*
** Set the cursor position
*/
void TextSetCursorPos(Ne_Text_Editor* textD, int pos)
{
   TextDSetInsertPosition(textD, pos);
   checkAutoShowInsertPos(textD);
   callCursorMovementCBs(textD, NULL);

}

/*
** Return the horizontal and vertical scroll positions of the widget
*/
void TextGetScroll(Ne_Text_Editor* textD, int* topLineNum, int* horizOffset)
{
   TextDGetScroll(textD, topLineNum, horizOffset);
}

/*
** Set the horizontal and vertical scroll positions of the widget
*/
void TextSetScroll(Ne_Text_Editor* textD, int topLineNum, int horizOffset)
{
   TextDSetScroll(textD, topLineNum, horizOffset);
}

int TextGetMinFontWidth(Ne_Text_Editor* textD, bool considerStyles)
{
   return(TextDMinFontWidth(textD, considerStyles));
}

int TextGetMaxFontWidth(Ne_Text_Editor* textD, bool considerStyles)
{
   return(TextDMaxFontWidth(textD, considerStyles));
}

// TODO: /*
// TODO: ** Set this widget to be the owner of selections made in it's attached
// TODO: ** buffer (text buffers may be shared among several text widgets).
// TODO: */
// TODO: void TextHandleXSelections(Fl_Widget* w)
// TODO: {
// TODO:    HandleXSelections(w);
// TODO: }

void TextPasteClipboard(Ne_Text_Editor* textD, double time)
{
   cancelDrag(textD);
   if (checkReadOnly(textD))
      return;
   TakeMotifDestination(textD, time);
   InsertClipboard(textD, false);
   callCursorMovementCBs(textD, NULL);
}

void TextColPasteClipboard(Ne_Text_Editor* textD, double time)
{
   cancelDrag(textD);
   if (checkReadOnly(textD))
      return;
   TakeMotifDestination(textD, time);
   InsertClipboard(textD, true);
   callCursorMovementCBs(textD, NULL);
}

void TextCopyClipboard(Ne_Text_Editor* textD, double time)
{
   cancelDrag(textD);
   if (!textD->buffer->primary.selected)
   {
      fl_beep();
      return;
   }
   CopyToClipboard(textD, time);
}

void TextCutClipboard(Ne_Text_Editor* textD, double time)
{
   cancelDrag(textD);
   if (checkReadOnly(textD))
      return;
   if (!textD->buffer->primary.selected)
   {
      fl_beep();
      return;
   }
   TakeMotifDestination(textD, time);
   CopyToClipboard(textD, time);
   BufRemoveSelected(textD->buffer);
   TextDSetInsertPosition(textD, textD->buffer->cursorPosHint);
   checkAutoShowInsertPos(textD);
}

int TextFirstVisibleLine(Ne_Text_Editor* textD)
{
   return(textD->topLineNum);
}

int TextNumVisibleLines(Ne_Text_Editor* textD)
{
   return(textD->nVisibleLines);
}

int TextVisibleWidth(Ne_Text_Editor* textD)
{
   return(textD->width);
}

int TextFirstVisiblePos(Ne_Text_Editor* textD)
{
   return textD->firstChar;
}

int TextLastVisiblePos(Ne_Text_Editor* textD)
{
   return textD->lastChar;
}

/*
** Insert text "chars" at the cursor position, respecting pending delete
** selections, overstrike, and handling cursor repositioning as if the text
** had been typed.  If autoWrap is on wraps the text to fit within the wrap
** margin, auto-indenting where the line was wrapped (but nowhere else).
** "allowPendingDelete" controls whether primary selections in the widget are
** treated as pending delete selections (True), or ignored (false). "event"
** is optional and is just passed on to the cursor movement callbacks.
*/
void TextInsertAtCursor(Ne_Text_Editor* textD, char* chars, int event, int allowPendingDelete, int allowWrap)
{
   int wrapMargin, colNum, lineStartPos, cursorPos;
   char* c, *lineStartText, *wrappedText;
   Ne_Text_Buffer* buf = textD->buffer;
   int fontWidth = textD->primaryFont.max_width();
   int replaceSel, singleLine, breakAt = 0;

   /* Don't wrap if auto-wrap is off or suppressed, or it's just a newline */
   if (!allowWrap || !textD->text.autoWrap ||
         (chars[0] == '\n' && chars[1] == '\0'))
   {
      simpleInsertAtCursor(textD, chars, event, allowPendingDelete);
      return;
   }

   /* If this is going to be a pending delete operation, the real insert
      position is the start of the selection.  This will make rectangular
      selections wrap strangely, but this routine should rarely be used for
      them, and even more rarely when they need to be wrapped. */
   replaceSel = allowPendingDelete && pendingSelection(textD);
   cursorPos = replaceSel ? buf->primary.start : TextDGetInsertPosition(textD);

   /* If the text is only one line and doesn't need to be wrapped, just insert
      it and be done (for efficiency only, this routine is called for each
      character typed). (Of course, it may not be significantly more efficient
      than the more general code below it, so it may be a waste of time!) */
   wrapMargin = textD->wrapMargin != 0 ? textD->wrapMargin : textD->width / fontWidth;
   lineStartPos = BufStartOfLine(buf, cursorPos);
   colNum = BufCountDispChars(buf, lineStartPos, cursorPos);
   for (c=chars; *c!='\0' && *c!='\n'; c++)
      colNum += BufCharWidth(*c, colNum, buf->tabDist, buf->nullSubsChar);
   singleLine = *c == '\0';
   if (colNum < wrapMargin && singleLine)
   {
      simpleInsertAtCursor(textD, chars, event, true);
      return;
   }

   /* Wrap the text */
   lineStartText = BufGetRange(buf, lineStartPos, cursorPos);
   wrappedText = wrapText(textD, lineStartText, chars, lineStartPos, wrapMargin, replaceSel ? NULL : &breakAt);
   free__(lineStartText);

   /* Insert the text.  Where possible, use TextDInsert which is optimized
      for less redraw. */
   if (replaceSel)
   {
      BufReplaceSelected(buf, wrappedText);
      TextDSetInsertPosition(textD, buf->cursorPosHint);
   }
   else if (textD->text.overstrike)
   {
      if (breakAt == 0 && singleLine)
         TextDOverstrike(textD, wrappedText);
      else
      {
         BufReplace(buf, cursorPos-breakAt, cursorPos, wrappedText);
         TextDSetInsertPosition(textD, buf->cursorPosHint);
      }
   }
   else
   {
      if (breakAt == 0)
      {
         TextDInsert(textD, wrappedText);
      }
      else
      {
         BufReplace(buf, cursorPos-breakAt, cursorPos, wrappedText);
         TextDSetInsertPosition(textD, buf->cursorPosHint);
      }
   }
   free__(wrappedText);
   checkAutoShowInsertPos(textD);
   callCursorMovementCBs(textD, event);
}

/*
** Fetch text from the widget's buffer, adding wrapping newlines to emulate
** effect acheived by wrapping in the text display in continuous wrap mode.
*/
char* TextGetWrapped(Ne_Text_Editor* textD, int startPos, int endPos, int* outLen)
{
   Ne_Text_Buffer* buf = textD->buffer;
   Ne_Text_Buffer* outBuf;
   int fromPos, toPos, outPos;
   char c, *outString;

   if (!textD->continuousWrap || startPos == endPos)
   {
      *outLen = endPos - startPos;
      return BufGetRange(buf, startPos, endPos);
   }

   /* Create a text buffer with a good estimate of the size that adding
      newlines will expand it to.  Since it's a text buffer, if we guess
      wrong, it will fail softly, and simply expand the size */
   outBuf = BufCreatePreallocated((endPos-startPos) + (endPos-startPos)/5);
   outPos = 0;

   /* Go (displayed) line by line through the buffer, adding newlines where
      the text is wrapped at some character other than an existing newline */
   fromPos = startPos;
   toPos = TextDCountForwardNLines(textD, startPos, 1, false);
   while (toPos < endPos)
   {
      BufCopyFromBuf(buf, outBuf, fromPos, toPos, outPos);
      outPos += toPos - fromPos;
      c = BufGetCharacter(outBuf, outPos-1);
      if (c == ' ' || c == '\t')
         BufReplace(outBuf, outPos-1, outPos, "\n");
      else if (c != '\n')
      {
         BufInsert(outBuf, outPos, "\n");
         outPos++;
      }
      fromPos = toPos;
      toPos = TextDCountForwardNLines(textD, fromPos, 1, true);
   }
   BufCopyFromBuf(buf, outBuf, fromPos, endPos, outPos);

   /* return the contents of the output buffer as a string */
   outString = BufGetAll(outBuf);
   *outLen = outBuf->length;
   BufFree(outBuf);
   return outString;
}

/*
** Return the (statically allocated) action table for menu item actions.
**
** Warning: This routine can only be used before the first text widget is
** created!  After that, apparently, Xt takes over the table and overwrites
** it with its own version.  XtGetActionList is preferable, but is not
** available before X11R5.
*/
XtActionsRec* TextGetActions(int* nActions)
{
   *nActions = ARRAY_SIZE(actionsList);
   return actionsList;
}

static void grabFocusAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   double lastBtnDown = textD->text.lastBtnDown;
   int row, column;

   /* Indicate state for future events, PRIMARY_CLICKED indicates that
      the proper initialization has been done for primary dragging and/or
      multi-clicking.  Also record the timestamp for multi-click processing */
   textD->text.dragState = PRIMARY_CLICKED;
   textD->text.lastBtnDown = GetTimeOfDay();

   /* Become owner of the MOTIF_DESTINATION selection, making this widget
      the designated recipient of secondary quick actions in Motif XmText
      widgets and in other NEdit text widgets */
   TakeMotifDestination(textD, GetTimeOfDay());

   /* Check for possible multi-click sequence in progress */
   if (textD->text.multiClickState != NO_CLICKS)
   {
      if (GetTimeOfDay() < lastBtnDown + textD->MultiClickTime)
      {
         if (textD->text.multiClickState == ONE_CLICK)
         {
            selectWord(w, Fl::event_x());
            callCursorMovementCBs(w, event);
            return;
         }
         else if (textD->text.multiClickState == TWO_CLICKS)
         {
            selectLine(w);
            callCursorMovementCBs(w, event);
            return;
         }
         else if (textD->text.multiClickState == THREE_CLICKS)
         {
            BufSelect(textD->buffer, 0, textD->buffer->length);
            return;
         }
         else if (textD->text.multiClickState > THREE_CLICKS)
            textD->text.multiClickState = NO_CLICKS;
      }
      else
         textD->text.multiClickState = NO_CLICKS;
   }

   /* Clear any existing selections */
   BufUnselect(textD->buffer);

   /* Move the cursor to the pointer location */
   moveDestinationAP(w, event, args, nArgs);

   /* Record the site of the initial button press and the initial character
      position so subsequent motion events and clicking can decide when and
      where to begin a primary selection */
   textD->text.btnDownX = Fl::event_x();
   textD->text.btnDownY = Fl::event_y();
   textD->text.anchor = TextDGetInsertPosition(textD);
   TextDXYToUnconstrainedPosition(textD, Fl::event_x(), Fl::event_x(), &row, &column);
   column = TextDOffsetWrappedColumn(textD, row, column);
   textD->text.rectAnchor = column;
}

static void moveDestinationAP(Fl_Widget* w, int event, const char** args,
                              int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   /* Get input focus */
// TODO:    XmProcessTraversal(w, XmTRAVERSE_CURRENT);

   /* Move the cursor */
   TextDSetInsertPosition(textD, TextDXYToPosition(textD, Fl::event_x(), Fl::event_y()));
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void extendAdjustAP(Fl_Widget* w, int event, const char** args,
                           int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int dragState = textD->text.dragState;
   int rectDrag = hasKey("rect", args, nArgs);

   /* Make sure the proper initialization was done on mouse down */
   if (dragState != PRIMARY_DRAG && dragState != PRIMARY_CLICKED &&
         dragState != PRIMARY_RECT_DRAG)
      return;

   /* If the selection hasn't begun, decide whether the mouse has moved
      far enough from the initial mouse down to be considered a drag */
   if (textD->text.dragState == PRIMARY_CLICKED)
   {
      if (textD->mouseMoveForDrag(Fl::event_x(), Fl::event_y()))
         textD->text.dragState = rectDrag ? PRIMARY_RECT_DRAG : PRIMARY_DRAG;
      else
         return;
   }

   /* If "rect" argument has appeared or disappeared, keep dragState up
      to date about which type of drag this is */
   textD->text.dragState = rectDrag ? PRIMARY_RECT_DRAG : PRIMARY_DRAG;

   /* Record the new position for the autoscrolling timer routine, and
      engage or disengage the timer if the mouse is in/out of the window */
   checkAutoScroll(w, Fl::event_x(), Fl::event_y());

   /* Adjust the selection and move the cursor */
   adjustSelection(w, Fl::event_x(), Fl::event_y());
}

static void extendStartAP(Fl_Widget* w, int event, const char** args,
                          int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   selection* sel = &buf->primary;
   int anchor, rectAnchor, anchorLineStart, newPos, row, column;

   /* Find the new anchor point for the rest of this drag operation */
   newPos = TextDXYToPosition(textD, Fl::event_x(), Fl::event_y());
   TextDXYToUnconstrainedPosition(textD, Fl::event_x(), Fl::event_y(), &row, &column);
   column = TextDOffsetWrappedColumn(textD, row, column);
   if (sel->selected)
   {
      if (sel->rectangular)
      {
         rectAnchor = column < (sel->rectEnd + sel->rectStart) / 2 ?
                      sel->rectEnd : sel->rectStart;
         anchorLineStart = BufStartOfLine(buf, newPos <
                                          (sel->end + sel->start) / 2 ? sel->end : sel->start);
         anchor = BufCountForwardDispChars(buf, anchorLineStart, rectAnchor);
      }
      else
      {
         if (abs(newPos - sel->start) < abs(newPos - sel->end))
            anchor = sel->end;
         else
            anchor = sel->start;
         anchorLineStart = BufStartOfLine(buf, anchor);
         rectAnchor = BufCountDispChars(buf, anchorLineStart, anchor);
      }
   }
   else
   {
      anchor = TextDGetInsertPosition(textD);
      anchorLineStart = BufStartOfLine(buf, anchor);
      rectAnchor = BufCountDispChars(buf, anchorLineStart, anchor);
   }
   textD->text.anchor = anchor;
   textD->text.rectAnchor = rectAnchor;

   /* Make the new selection */
   if (hasKey("rect", args, nArgs))
      BufRectSelect(buf, BufStartOfLine(buf, min(anchor, newPos)),
                    BufEndOfLine(buf, max(anchor, newPos)),
                    min(rectAnchor, column), max(rectAnchor, column));
   else
      BufSelect(buf, min(anchor, newPos), max(anchor, newPos));

   /* Never mind the motion threshold, go right to dragging since
      extend-start is unambiguously the start of a selection */
   textD->text.dragState = PRIMARY_DRAG;

   /* Don't do by-word or by-line adjustment, just by character */
   textD->text.multiClickState = NO_CLICKS;

   /* Move the cursor */
   TextDSetInsertPosition(textD, newPos);
   callCursorMovementCBs(w, event);
}

static void extendEndAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   if (textD->text.dragState == PRIMARY_CLICKED && textD->text.lastBtnDown <= GetTimeOfDay() + textD->MultiClickTime)
      textD->text.multiClickState++;
   endDrag(w);
}

static void processCancelAP(Fl_Widget* w, int event, const char** args,
                            int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int dragState = textD->text.dragState;
   Ne_Text_Buffer* buf = textD->buffer;

   /* If there's a calltip displayed, kill it. */
// TODO:    TextDKillCalltip(textD, 0);

   if (dragState == PRIMARY_DRAG || dragState == PRIMARY_RECT_DRAG)
      BufUnselect(buf);
   cancelDrag(w);
}

static void secondaryStartAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   selection* sel = &buf->secondary;
   int anchor, pos, row, column;

   /* Find the new anchor point and make the new selection */
   pos = TextDXYToPosition(textD, Fl::event_x(), Fl::event_y());
   if (sel->selected)
   {
      if (abs(pos - sel->start) < abs(pos - sel->end))
         anchor = sel->end;
      else
         anchor = sel->start;
      BufSecondarySelect(buf, anchor, pos);
   }
   else
      anchor = pos;

   /* Record the site of the initial button press and the initial character
      position so subsequent motion events can decide when to begin a
      selection, (and where the selection began) */
   textD->text.btnDownX = Fl::event_x();
   textD->text.btnDownY = Fl::event_y();
   textD->text.anchor = pos;
   TextDXYToUnconstrainedPosition(textD, Fl::event_x(), Fl::event_y(), &row, &column);
   column = TextDOffsetWrappedColumn(textD, row, column);
   textD->text.rectAnchor = column;
   textD->text.dragState = SECONDARY_CLICKED;
}

static void secondaryOrDragStartAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;

   /* If the click was outside of the primary selection, this is not
      a drag, start a secondary selection */
   if (!buf->primary.selected || !TextDInSelection(textD, Fl::event_x(), Fl::event_y()))
   {
      secondaryStartAP(w, event, args, nArgs);
      return;
   }

   if (checkReadOnly(w))
      return;

   /* Record the site of the initial button press and the initial character
      position so subsequent motion events can decide when to begin a
      drag, and where to drag to */
   textD->text.btnDownX = Fl::event_x();
   textD->text.btnDownY = Fl::event_y();
   textD->text.dragState = CLICKED_IN_SELECTION;
}

static void secondaryAdjustAP(Fl_Widget* w, int event, const char** args,
                              int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int dragState = textD->text.dragState;
   bool rectDrag = hasKey("rect", args, nArgs);

   /* Make sure the proper initialization was done on mouse down */
   if (dragState != SECONDARY_DRAG && dragState != SECONDARY_RECT_DRAG &&
         dragState != SECONDARY_CLICKED)
      return;

   /* If the selection hasn't begun, decide whether the mouse has moved
      far enough from the initial mouse down to be considered a drag */
   if (textD->text.dragState == SECONDARY_CLICKED)
   {
      if (textD->mouseMoveForDrag(Fl::event_x(), Fl::event_y()))
         textD->text.dragState = rectDrag ? SECONDARY_RECT_DRAG: SECONDARY_DRAG;
      else
         return;
   }

   /* If "rect" argument has appeared or disappeared, keep dragState up
      to date about which type of drag this is */
   textD->text.dragState = rectDrag ? SECONDARY_RECT_DRAG : SECONDARY_DRAG;

   /* Record the new position for the autoscrolling timer routine, and
      engage or disengage the timer if the mouse is in/out of the window */
   checkAutoScroll(textD,Fl::event_x(), Fl::event_y());

   /* Adjust the selection */
   adjustSecondarySelection(textD,Fl::event_x(), Fl::event_y());
}

static void secondaryOrDragAdjustAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int dragState = textD->text.dragState;

   /* Only dragging of blocks of text is handled in this action proc.
      Otherwise, defer to secondaryAdjust to handle the rest */
   if (dragState != CLICKED_IN_SELECTION && dragState != PRIMARY_BLOCK_DRAG)
   {
      secondaryAdjustAP(w, event, args, nArgs);
      return;
   }

   /* Decide whether the mouse has moved far enough from the
      initial mouse down to be considered a drag */
   if (textD->text.dragState == CLICKED_IN_SELECTION)
   {
      if (textD->mouseMoveForDrag(Fl::event_x(), Fl::event_y()))
         BeginBlockDrag(textD);
      else
         return;
   }

   /* Record the new position for the autoscrolling timer routine, and
      engage or disengage the timer if the mouse is in/out of the window */
   checkAutoScroll(textD, Fl::event_x(), Fl::event_y());

   /* Adjust the selection */
   BlockDragSelection(textD, Fl::event_x(), Fl::event_y(), hasKey("overlay", args, nArgs) ?
                      (hasKey("copy", args, nArgs) ? DRAG_OVERLAY_COPY : DRAG_OVERLAY_MOVE) :
                         (hasKey("copy", args, nArgs) ? DRAG_COPY : DRAG_MOVE));
}

static void copyToAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int dragState = textD->text.dragState;
   Ne_Text_Buffer* buf = textD->buffer;
   selection* secondary = &buf->secondary, *primary = &buf->primary;
   int rectangular = secondary->rectangular;
   char* textToCopy;
   int insertPos, lineStart, column;

   endDrag(w);
   if (!((dragState == SECONDARY_DRAG && secondary->selected) ||
         (dragState == SECONDARY_RECT_DRAG && secondary->selected) ||
         dragState == SECONDARY_CLICKED || dragState == NOT_CLICKED))
      return;
   if (!(secondary->selected && !(textD->text.motifDestOwner)))
   {
      if (checkReadOnly(w))
      {
         BufSecondaryUnselect(buf);
         return;
      }
   }
   if (secondary->selected)
   {
      if (textD->text.motifDestOwner)
      {
         TextDBlankCursor(textD);
         textToCopy = BufGetSecSelectText(buf);
         if (primary->selected && rectangular)
         {
            insertPos = TextDGetInsertPosition(textD);
            BufReplaceSelected(buf, textToCopy);
            TextDSetInsertPosition(textD, buf->cursorPosHint);
         }
         else if (rectangular)
         {
            insertPos = TextDGetInsertPosition(textD);
            lineStart = BufStartOfLine(buf, insertPos);
            column = BufCountDispChars(buf, lineStart, insertPos);
            BufInsertCol(buf, column, lineStart, textToCopy, NULL, NULL);
            TextDSetInsertPosition(textD, buf->cursorPosHint);
         }
         else
            TextInsertAtCursor(textD, textToCopy, event, true,
                               textD->text.autoWrapPastedText);
         free__(textToCopy);
         BufSecondaryUnselect(buf);
         TextDUnblankCursor(textD);
      }
      else
         SendSecondarySelection(textD, GetTimeOfDay(), false);
   }
   else if (primary->selected)
   {
      textToCopy = BufGetSelectionText(buf);
      TextDSetInsertPosition(textD, TextDXYToPosition(textD, Fl::event_x(), Fl::event_y()));
      TextInsertAtCursor(textD, textToCopy, event, false,
                         textD->text.autoWrapPastedText);
      free__(textToCopy);
   }
   else
   {
      TextDSetInsertPosition(textD, TextDXYToPosition(textD, Fl::event_x(), Fl::event_y()));
      InsertPrimarySelection(textD, GetTimeOfDay(), false);
   }
}

static void copyToOrEndDragAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int dragState = textD->text.dragState;

   if (dragState != PRIMARY_BLOCK_DRAG)
   {
      copyToAP(w, event, args, nArgs);
      return;
   }

   FinishBlockDrag(textD);
}

static void moveToAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int dragState = textD->text.dragState;
   Ne_Text_Buffer* buf = textD->buffer;
   selection* secondary = &buf->secondary, *primary = &buf->primary;
   int insertPos, rectangular = secondary->rectangular;
   int column, lineStart;
   char* textToCopy;

   endDrag(w);
   if (!((dragState == SECONDARY_DRAG && secondary->selected) ||
         (dragState == SECONDARY_RECT_DRAG && secondary->selected) ||
         dragState == SECONDARY_CLICKED || dragState == NOT_CLICKED))
      return;
   if (checkReadOnly(w))
   {
      BufSecondaryUnselect(buf);
      return;
   }

   if (secondary->selected)
   {
      if (textD->text.motifDestOwner)
      {
         textToCopy = BufGetSecSelectText(buf);
         if (primary->selected && rectangular)
         {
            insertPos = TextDGetInsertPosition(textD);
            BufReplaceSelected(buf, textToCopy);
            TextDSetInsertPosition(textD, buf->cursorPosHint);
         }
         else if (rectangular)
         {
            insertPos = TextDGetInsertPosition(textD);
            lineStart = BufStartOfLine(buf, insertPos);
            column = BufCountDispChars(buf, lineStart, insertPos);
            BufInsertCol(buf, column, lineStart, textToCopy, NULL, NULL);
            TextDSetInsertPosition(textD, buf->cursorPosHint);
         }
         else
            TextInsertAtCursor(textD, textToCopy, event, true,
                               textD->text.autoWrapPastedText);
         free__(textToCopy);
         BufRemoveSecSelect(buf);
         BufSecondaryUnselect(buf);
      }
      else
         SendSecondarySelection(textD, GetTimeOfDay(), true);
   }
   else if (primary->selected)
   {
      textToCopy = BufGetRange(buf, primary->start, primary->end);
      TextDSetInsertPosition(textD, TextDXYToPosition(textD, Fl::event_x(), Fl::event_y()));
      TextInsertAtCursor(textD, textToCopy, event, false,
                         textD->text.autoWrapPastedText);
      free__(textToCopy);
      BufRemoveSelected(buf);
      BufUnselect(buf);
   }
   else
   {
      TextDSetInsertPosition(textD, TextDXYToPosition(textD, Fl::event_x(), Fl::event_y()));
      MovePrimarySelection(textD, GetTimeOfDay(), false);
   }
}

static void moveToOrEndDragAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   int dragState = textD->text.dragState;

   if (dragState != PRIMARY_BLOCK_DRAG)
   {
      moveToAP(w, event, args, nArgs);
      return;
   }

   FinishBlockDrag(textD);
}

static void endDragAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   if (textD->text.dragState == PRIMARY_BLOCK_DRAG)
      FinishBlockDrag(textD);
   else
      endDrag(w);
}

static void exchangeAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   selection* sec = &buf->secondary, *primary = &buf->primary;
   char* primaryText, *secText;
   int newPrimaryStart, newPrimaryEnd, secWasRect;
   int dragState = textD->text.dragState; /* save before endDrag */
   bool silent = hasKey("nobell", args, nArgs);

   endDrag(w);
   if (checkReadOnly(w))
      return;

   /* If there's no secondary selection here, or the primary and secondary
      selection overlap, just beep and return */
   if (!sec->selected || (primary->selected &&
                          ((primary->start <= sec->start && primary->end > sec->start) ||
                           (sec->start <= primary->start && sec->end > primary->start))))
   {
      BufSecondaryUnselect(buf);
      ringIfNecessary(silent);
      /* If there's no secondary selection, but the primary selection is
         being dragged, we must not forget to finish the dragging.
         Otherwise, modifications aren't recorded. */
      if (dragState == PRIMARY_BLOCK_DRAG)
         FinishBlockDrag(textD);
      return;
   }

   /* if the primary selection is in another widget, use selection routines */
   if (!primary->selected)
   {
      ExchangeSelections(textD, GetTimeOfDay());
      return;
   }

   /* Both primary and secondary are in this widget, do the exchange here */
   primaryText = BufGetSelectionText(buf);
   secText = BufGetSecSelectText(buf);
   secWasRect = sec->rectangular;
   BufReplaceSecSelect(buf, primaryText);
   newPrimaryStart = primary->start;
   BufReplaceSelected(buf, secText);
   newPrimaryEnd = newPrimaryStart + strlen(secText);
   free__(primaryText);
   free__(secText);
   BufSecondaryUnselect(buf);
   if (secWasRect)
   {
      TextDSetInsertPosition(textD, buf->cursorPosHint);
   }
   else
   {
      BufSelect(buf, newPrimaryStart, newPrimaryEnd);
      TextDSetInsertPosition(textD, newPrimaryEnd);
   }
   checkAutoShowInsertPos(w);
}

static void copyPrimaryAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   selection* primary = &buf->primary;
   int rectangular = hasKey("rect", args, nArgs);
   char* textToCopy;
   int insertPos, col;

   cancelDrag(w);
   if (checkReadOnly(w))
      return;
   if (primary->selected && rectangular)
   {
      textToCopy = BufGetSelectionText(buf);
      insertPos = TextDGetInsertPosition(textD);
      col = BufCountDispChars(buf, BufStartOfLine(buf, insertPos), insertPos);
      BufInsertCol(buf, col, insertPos, textToCopy, NULL, NULL);
      TextDSetInsertPosition(textD, buf->cursorPosHint);
      free__(textToCopy);
      checkAutoShowInsertPos(w);
   }
   else if (primary->selected)
   {
      textToCopy = BufGetSelectionText(buf);
      insertPos = TextDGetInsertPosition(textD);
      BufInsert(buf, insertPos, textToCopy);
      TextDSetInsertPosition(textD, insertPos + strlen(textToCopy));
      free__(textToCopy);
      checkAutoShowInsertPos(w);
   }
   else if (rectangular)
   {
      if (!TextDPositionToXY(textD, TextDGetInsertPosition(textD),
                             &textD->text.btnDownX, &textD->text.btnDownY))
         return; /* shouldn't happen */
      InsertPrimarySelection(textD, GetTimeOfDay(), true);
   }
   else
      InsertPrimarySelection(textD, GetTimeOfDay(), false);
}

static void cutPrimaryAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   selection* primary = &buf->primary;
   char* textToCopy;
   int rectangular = hasKey("rect", args, nArgs);
   int insertPos, col;

   cancelDrag(w);
   if (checkReadOnly(w))
      return;
   if (primary->selected && rectangular)
   {
      textToCopy = BufGetSelectionText(buf);
      insertPos = TextDGetInsertPosition(textD);
      col = BufCountDispChars(buf, BufStartOfLine(buf, insertPos), insertPos);
      BufInsertCol(buf, col, insertPos, textToCopy, NULL, NULL);
      TextDSetInsertPosition(textD, buf->cursorPosHint);
      free__(textToCopy);
      BufRemoveSelected(buf);
      checkAutoShowInsertPos(w);
   }
   else if (primary->selected)
   {
      textToCopy = BufGetSelectionText(buf);
      insertPos = TextDGetInsertPosition(textD);
      BufInsert(buf, insertPos, textToCopy);
      TextDSetInsertPosition(textD, insertPos + strlen(textToCopy));
      free__(textToCopy);
      BufRemoveSelected(buf);
      checkAutoShowInsertPos(w);
   }
   else if (rectangular)
   {
      if (!TextDPositionToXY(textD, TextDGetInsertPosition(textD),
                             &textD->text.btnDownX,
                             &textD->text.btnDownY))
         return; /* shouldn't happen */
      MovePrimarySelection(textD, GetTimeOfDay(), true);
   }
   else
   {
      MovePrimarySelection(textD, GetTimeOfDay(), false);
   }
}

static void mousePanAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int lineHeight = textD->ascent + textD->descent;
   int topLineNum, horizOffset;

   if (textD->text.dragState == MOUSE_PAN)
   {
      TextDSetScroll(textD,
                     (textD->text.btnDownY - Fl::event_y() + lineHeight/2) / lineHeight,
                     textD->text.btnDownX - Fl::event_x());
   }
   else if (textD->text.dragState == NOT_CLICKED)
   {
      TextDGetScroll(textD, &topLineNum, &horizOffset);
      textD->text.btnDownX = Fl::event_x() + horizOffset;
      textD->text.btnDownY = Fl::event_y() + topLineNum * lineHeight;
      textD->text.dragState = MOUSE_PAN;
      textD->window()->cursor(FL_CURSOR_CROSS);
   }
   else
      cancelDrag(w);
}

static void pasteClipboardAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   if (hasKey("rect", args, nArgs))
      TextColPasteClipboard(textD, GetTimeOfDay());
   else
      TextPasteClipboard(textD, GetTimeOfDay());
}

static void copyClipboardAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   TextCopyClipboard(textD, GetTimeOfDay());
}

static void cutClipboardAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   TextCutClipboard(textD, GetTimeOfDay());
}

static void insertStringAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   smartIndentCBStruct smartIndent;
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   if (*nArgs == 0)
      return;
   cancelDrag(w);
   if (checkReadOnly(w))
      return;
   if (textD->text.smartIndent)
   {
      smartIndent.reason = CHAR_TYPED;
      smartIndent.pos = TextDGetInsertPosition(textD);
      smartIndent.indentRequest = 0;
      smartIndent.charsTyped = (char*)args[0];
// TODO:       XtCallCallbacks(w, textNsmartIndentCallback, (void*)&smartIndent);
   }
   TextInsertAtCursor(textD, (char*)args[0], event, true, true);
   BufUnselect(textD->buffer);
}

static void selfInsertAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   TRACE();

   char* chars;
   int nChars;
   smartIndentCBStruct smartIndent;
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   {  const char* utf8 = Fl::event_text();
      chars = new char[strlen(utf8)+1];
      fl_utf8toa(utf8, strlen(utf8)+1, chars, strlen(utf8)+1);
   }

   nChars = strlen(chars);
   if (nChars == 0)
      return;

   cancelDrag(w);
   if (checkReadOnly(w))
      return;
   
   TakeMotifDestination(textD, GetTimeOfDay());

   if (!BufSubstituteNullChars(chars, nChars, textD->buffer))
   {
      DialogF(DF_ERR, textD, 1, "Error", "Too much binary data", "OK");
      return;
   }

   // If smart indent is on, call the smart indent callback to check the inserted character
   if (textD->text.smartIndent)
   {
      smartIndent.reason = CHAR_TYPED;
      smartIndent.pos = TextDGetInsertPosition(textD);
      smartIndent.indentRequest = 0;
      smartIndent.charsTyped = chars;
// TODO:       XtCallCallbacks(w, textNsmartIndentCallback, (XtPointer)&smartIndent);
   }
   TextInsertAtCursor(textD, chars, event, true, true);
   BufUnselect(textD->buffer);
}

static void newlineAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   if (textD->text.autoIndent || textD->text.smartIndent)
      newlineAndIndentAP(w, event, args, nArgs);
   else
      newlineNoIndentAP(w, event, args, nArgs);
}

static void newlineNoIndentAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   cancelDrag(w);
   if (checkReadOnly(w))
      return;
   TakeMotifDestination(textD, GetTimeOfDay());
   simpleInsertAtCursor(textD, "\n", event, true);
   BufUnselect(textD->buffer);
}

static void newlineAndIndentAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   char* indentStr;
   int cursorPos, lineStartPos, column;

   if (checkReadOnly(w))
      return;
   cancelDrag(w);
   TakeMotifDestination(textD, GetTimeOfDay());

   // Create a string containing a newline followed by auto or smart indent string
   cursorPos = TextDGetInsertPosition(textD);
   lineStartPos = BufStartOfLine(buf, cursorPos);
   indentStr = createIndentString(w, buf, 0, lineStartPos, cursorPos, NULL, &column);

   // Insert it at the cursor
   simpleInsertAtCursor(w, indentStr, event, true);
   free__(indentStr);

   if (textD->text.emulateTabs > 0)
   {
      /*  If emulated tabs are on, make the inserted indent deletable by
          tab. Round this up by faking the column a bit to the right to
          let the user delete half-tabs with one keypress.  */
      column += textD->text.emulateTabs - 1;
      textD->text.emTabsBeforeCursor = column / textD->text.emulateTabs;
   }

   BufUnselect(buf);
}

static void processTabAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   selection* sel = &buf->primary;
   int emTabDist = textD->text.emulateTabs;
   int emTabsBeforeCursor = textD->text.emTabsBeforeCursor;
   int insertPos, indent, startIndent, toIndent, lineStart, tabWidth;
   char* outStr, *outPtr;

   if (checkReadOnly(w))
      return;
   cancelDrag(w);
   TakeMotifDestination(textD, GetTimeOfDay());

   // If emulated tabs are off, just insert a tab
   if (emTabDist <= 0)
   {
      TextInsertAtCursor(textD, "\t", event, true, true);
      return;
   }

   /* Find the starting and ending indentation.  If the tab is to
      replace an existing selection, use the start of the selection
      instead of the cursor position as the indent.  When replacing
      rectangular selections, tabs are automatically recalculated as
      if the inserted text began at the start of the line */
   insertPos = pendingSelection(w) ? sel->start : TextDGetInsertPosition(textD);
   lineStart = BufStartOfLine(buf, insertPos);
   if (pendingSelection(w) && sel->rectangular)
      insertPos = BufCountForwardDispChars(buf, lineStart, sel->rectStart);
   startIndent = BufCountDispChars(buf, lineStart, insertPos);
   toIndent = startIndent + emTabDist - (startIndent % emTabDist);
   if (pendingSelection(w) && sel->rectangular)
   {
      toIndent -= startIndent;
      startIndent = 0;
   }

   /* Allocate a buffer assuming all the inserted characters will be spaces */
   outStr = (char*)malloc__(toIndent - startIndent + 1);

   /* Add spaces and tabs to outStr until it reaches toIndent */
   outPtr = outStr;
   indent = startIndent;
   while (indent < toIndent)
   {
      tabWidth = BufCharWidth('\t', indent, buf->tabDist, buf->nullSubsChar);
      if (buf->useTabs && tabWidth > 1 && indent + tabWidth <= toIndent)
      {
         *outPtr++ = '\t';
         indent += tabWidth;
      }
      else
      {
         *outPtr++ = ' ';
         indent++;
      }
   }
   *outPtr = '\0';

   /* Insert the emulated tab */
   TextInsertAtCursor(textD, outStr, event, true, true);
   free__(outStr);

   /* Restore and ++ emTabsBeforeCursor cleared by TextInsertAtCursor */
   textD->text.emTabsBeforeCursor = emTabsBeforeCursor + 1;

   BufUnselect(buf);
}

static void deleteSelectionAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   cancelDrag(w);
   if (checkReadOnly(w))
      return;
   TakeMotifDestination(textD, GetTimeOfDay());
   deletePendingSelection(textD, event);
}

static void deletePreviousCharacterAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);
   char c;
   bool silent = hasKey("nobell", args, nArgs);

   cancelDrag(w);
   if (checkReadOnly(w))
      return;

   TakeMotifDestination(textD, GetTimeOfDay());
   if (deletePendingSelection(w, event))
      return;

   if (insertPos == 0)
   {
      ringIfNecessary(silent);
      return;
   }

   if (deleteEmulatedTab(w, event))
      return;

   if (textD->text.overstrike)
   {
      c = BufGetCharacter(textD->buffer, insertPos - 1);
      if (c == '\n')
         BufRemove(textD->buffer, insertPos - 1, insertPos);
      else if (c != '\t')
         BufReplace(textD->buffer, insertPos - 1, insertPos, " ");
   }
   else
   {
      BufRemove(textD->buffer, insertPos - 1, insertPos);
   }

   TextDSetInsertPosition(textD, insertPos - 1);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void deleteNextCharacterAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);
   bool silent = hasKey("nobell", args, nArgs);

   cancelDrag(w);
   if (checkReadOnly(w))
      return;
   TakeMotifDestination(textD, GetTimeOfDay());
   if (deletePendingSelection(w, event))
      return;
   if (insertPos == textD->buffer->length)
   {
      ringIfNecessary(silent);
      return;
   }
   BufRemove(textD->buffer, insertPos , insertPos + 1);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void deletePreviousWordAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);
   int pos, lineStart = BufStartOfLine(textD->buffer, insertPos);
   const std::string& delimiters = textD->text.delimiters;
   bool silent = hasKey("nobell", args, nArgs);

   cancelDrag(w);
   if (checkReadOnly(w))
   {
      return;
   }

   TakeMotifDestination(textD, GetTimeOfDay());
   if (deletePendingSelection(w, event))
   {
      return;
   }

   if (insertPos == lineStart)
   {
      ringIfNecessary(silent);
      return;
   }

   pos = max(insertPos - 1, 0);
   while (strchr(delimiters.c_str(), BufGetCharacter(textD->buffer, pos)) != NULL &&
          pos != lineStart)
   {
      pos--;
   }

   pos = startOfWord(textD, pos);
   BufRemove(textD->buffer, pos, insertPos);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void deleteNextWordAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);
   int pos, lineEnd = BufEndOfLine(textD->buffer, insertPos);
   const std::string& delimiters = textD->text.delimiters;
   bool silent = hasKey("nobell", args, nArgs);

   cancelDrag(w);
   if (checkReadOnly(w))
   {
      return;
   }

   TakeMotifDestination(textD, GetTimeOfDay());
   if (deletePendingSelection(w, event))
   {
      return;
   }

   if (insertPos == lineEnd)
   {
      ringIfNecessary(silent);
      return;
   }

   pos = insertPos;
   while (strchr(delimiters.c_str(), BufGetCharacter(textD->buffer, pos)) != NULL &&
          pos != lineEnd)
   {
      pos++;
   }

   pos = endOfWord(textD, pos);
   BufRemove(textD->buffer, insertPos, pos);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void deleteToEndOfLineAP(Fl_Widget* w, int event, const char** args,
                                int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);
   int endOfLine;
   bool silent = false;

   silent = hasKey("nobell", args, nArgs);
   if (hasKey("absolute", args, nArgs))
      endOfLine = BufEndOfLine(textD->buffer, insertPos);
   else
      endOfLine = TextDEndOfLine(textD, insertPos, false);
   cancelDrag(w);
   if (checkReadOnly(w))
      return;
   TakeMotifDestination(textD, GetTimeOfDay());
   if (deletePendingSelection(w, event))
      return;
   if (insertPos == endOfLine)
   {
      ringIfNecessary(silent);
      return;
   }
   BufRemove(textD->buffer, insertPos, endOfLine);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void deleteToStartOfLineAP(Fl_Widget* w, int event, const char** args,
                                  int* nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);
   int startOfLine;
   bool silent = false;

   silent = hasKey("nobell", args, nArgs);
   if (hasKey("wrap", args, nArgs))
      startOfLine = TextDStartOfLine(textD, insertPos);
   else
      startOfLine = BufStartOfLine(textD->buffer, insertPos);
   cancelDrag(w);
   if (checkReadOnly(w))
      return;
   TakeMotifDestination(textD, GetTimeOfDay());
   if (deletePendingSelection(w, event))
      return;
   if (insertPos == startOfLine)
   {
      ringIfNecessary(silent);
      return;
   }
   BufRemove(textD->buffer, startOfLine, insertPos);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void forwardCharacterAP(Fl_Widget* w, int event, const char** args,int* nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   int insertPos = TextDGetInsertPosition(textD);
   bool silent = hasKey("nobell", args, nArgs);

   cancelDrag(w);
   if (!TextDMoveRight(textD))
   {
      ringIfNecessary(silent);
   }
   checkMoveSelectionChange(w, event, insertPos, args, nArgs);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void backwardCharacterAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   int insertPos = TextDGetInsertPosition(textD);
   bool silent = hasKey("nobell", args, nArgs);

   cancelDrag(w);
   if (!TextDMoveLeft(textD))
   {
      ringIfNecessary(silent);
   }
   checkMoveSelectionChange(w, event, insertPos, args, nArgs);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void forwardWordAP(Fl_Widget* w, int event, const char** args,
                          int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   int pos, insertPos = TextDGetInsertPosition(textD);
   const std::string& delimiters = textD->text.delimiters;
   bool silent = hasKey("nobell", args, nArgs);

   cancelDrag(w);
   if (insertPos == buf->length)
   {
      ringIfNecessary(silent);
      return;
   }
   pos = insertPos;

   if (hasKey("tail", args, nArgs))
   {
      for (; pos<buf->length; pos++)
      {
         if (NULL == strchr(delimiters.c_str(), BufGetCharacter(buf, pos)))
         {
            break;
         }
      }
      if (NULL == strchr(delimiters.c_str(), BufGetCharacter(buf, pos)))
      {
         pos = endOfWord(w, pos);
      }
   }
   else
   {
      if (NULL == strchr(delimiters.c_str(), BufGetCharacter(buf, pos)))
      {
         pos = endOfWord(w, pos);
      }
      for (; pos<buf->length; pos++)
      {
         if (NULL == strchr(delimiters.c_str(), BufGetCharacter(buf, pos)))
         {
            break;
         }
      }
   }

   TextDSetInsertPosition(textD, pos);
   checkMoveSelectionChange(w, event, insertPos, args, nArgs);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void backwardWordAP(Fl_Widget* w, int event, const char** args,
                           int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   int pos, insertPos = TextDGetInsertPosition(textD);
   const std::string& delimiters = textD->text.delimiters;
   bool silent = hasKey("nobell", args, nArgs);

   cancelDrag(w);
   if (insertPos == 0)
   {
      ringIfNecessary(silent);
      return;
   }
   pos = max(insertPos - 1, 0);
   while (strchr(delimiters.c_str(), BufGetCharacter(buf, pos)) != NULL && pos > 0)
      pos--;
   pos = startOfWord(w, pos);

   TextDSetInsertPosition(textD, pos);
   checkMoveSelectionChange(w, event, insertPos, args, nArgs);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void forwardParagraphAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int pos, insertPos = TextDGetInsertPosition(textD);
   Ne_Text_Buffer* buf = textD->buffer;
   char c;
   static char whiteChars[] = " \t";
   bool silent = hasKey("nobell", args, nArgs);

   cancelDrag(w);
   if (insertPos == buf->length)
   {
      ringIfNecessary(silent);
      return;
   }
   pos = min(BufEndOfLine(buf, insertPos)+1, buf->length);
   while (pos < buf->length)
   {
      c = BufGetCharacter(buf, pos);
      if (c == '\n')
         break;
      if (strchr(whiteChars, c) != NULL)
         pos++;
      else
         pos = min(BufEndOfLine(buf, pos)+1, buf->length);
   }
   TextDSetInsertPosition(textD, min(pos+1, buf->length));
   checkMoveSelectionChange(w, event, insertPos, args, nArgs);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void backwardParagraphAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int parStart, pos, insertPos = TextDGetInsertPosition(textD);
   Ne_Text_Buffer* buf = textD->buffer;
   char c;
   static char whiteChars[] = " \t";
   bool silent = hasKey("nobell", args, nArgs);

   cancelDrag(w);
   if (insertPos == 0)
   {
      ringIfNecessary(silent);
      return;
   }
   parStart = BufStartOfLine(buf, max(insertPos-1, 0));
   pos = max(parStart - 2, 0);
   while (pos > 0)
   {
      c = BufGetCharacter(buf, pos);
      if (c == '\n')
         break;
      if (strchr(whiteChars, c) != NULL)
         pos--;
      else
      {
         parStart = BufStartOfLine(buf, pos);
         pos = max(parStart - 2, 0);
      }
   }
   TextDSetInsertPosition(textD, parStart);
   checkMoveSelectionChange(w, event, insertPos, args, nArgs);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void keySelectAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int stat, insertPos = TextDGetInsertPosition(textD);
   bool silent = hasKey("nobell", args, nArgs);

   cancelDrag(w);
   if (hasKey("left", args, nArgs)) stat = TextDMoveLeft(textD);
   else if (hasKey("right", args, nArgs)) stat = TextDMoveRight(textD);
   else if (hasKey("up", args, nArgs)) stat = TextDMoveUp(textD, 0);
   else if (hasKey("down", args, nArgs)) stat = TextDMoveDown(textD, 0);
   else
   {
      keyMoveExtendSelection(w, event, insertPos, hasKey("rect", args,nArgs));
      return;
   }
   if (!stat)
   {
      ringIfNecessary(silent);
   }
   else
   {
      keyMoveExtendSelection(w, event, insertPos, hasKey("rect", args,nArgs));
      checkAutoShowInsertPos(w);
      callCursorMovementCBs(w, event);
   }
}

static void processUpAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   int insertPos = TextDGetInsertPosition(textD);
   bool silent = hasKey("nobell", args, nArgs);
   bool abs = hasKey("absolute", args, nArgs);

   cancelDrag(w);
   if (!TextDMoveUp(textD, abs))
      ringIfNecessary(silent);
   checkMoveSelectionChange(w, event, insertPos, args, nArgs);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void processShiftUpAP(Fl_Widget* w, int event, const char** args,
                             int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);
   bool silent = hasKey("nobell", args, nArgs);
   bool abs = hasKey("absolute", args, nArgs);

   cancelDrag(w);
   if (!TextDMoveUp(textD, abs))
      ringIfNecessary(silent);
   keyMoveExtendSelection(w, event, insertPos, hasKey("rect", args, nArgs));
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void processDownAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);
   bool silent = hasKey("nobell", args, nArgs);
   bool abs = hasKey("absolute", args, nArgs);

   cancelDrag(w);
   if (!TextDMoveDown(textD, abs))
      ringIfNecessary(silent);
   checkMoveSelectionChange(w, event, insertPos, args, nArgs);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void processShiftDownAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);
   bool silent = hasKey("nobell", args, nArgs);
   bool abs = hasKey("absolute", args, nArgs);

   cancelDrag(w);
   if (!TextDMoveDown(textD, abs))
      ringIfNecessary(silent);
   keyMoveExtendSelection(w, event, insertPos, hasKey("rect", args, nArgs));
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

static void beginningOfLineAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);

   cancelDrag(w);
   if (hasKey("absolute", args, nArgs))
      TextDSetInsertPosition(textD, BufStartOfLine(textD->buffer, insertPos));
   else
      TextDSetInsertPosition(textD, TextDStartOfLine(textD, insertPos));
   checkMoveSelectionChange(w, event, insertPos, args, nArgs);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
   textD->cursorPreferredCol = 0;
}

static void endOfLineAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);

   cancelDrag(w);
   if (hasKey("absolute", args, nArgs))
      TextDSetInsertPosition(textD, BufEndOfLine(textD->buffer, insertPos));
   else
      TextDSetInsertPosition(textD, TextDEndOfLine(textD, insertPos, false));
   checkMoveSelectionChange(w, event, insertPos, args, nArgs);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
   textD->cursorPreferredCol = -1;
}

static void beginningOfFileAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);

   cancelDrag(w);
   if (hasKey("scrollbar", args, nArgs))
   {
      if (textD->topLineNum != 1)
      {
         TextDSetScroll(textD, 1, textD->horizOffset);
      }
   }
   else
   {
      TextDSetInsertPosition(textD, 0);
      checkMoveSelectionChange(w, event, insertPos, args, nArgs);
      checkAutoShowInsertPos(w);
      callCursorMovementCBs(w, event);
   }
}

static void endOfFileAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);
   int lastTopLine;

   cancelDrag(w);
   if (hasKey("scrollbar", args, nArgs))
   {
      lastTopLine = max(1,
                        textD->nBufferLines - (textD->nVisibleLines - 2) +
                        textD->text.cursorVPadding);
      if (lastTopLine != textD->topLineNum)
      {
         TextDSetScroll(textD, lastTopLine, textD->horizOffset);
      }
   }
   else
   {
      TextDSetInsertPosition(textD, textD->buffer->length);
      checkMoveSelectionChange(w, event, insertPos, args, nArgs);
      checkAutoShowInsertPos(w);
      callCursorMovementCBs(w, event);
   }
}

static void nextPageAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   int lastTopLine = max(1,
                         textD->nBufferLines - (textD->nVisibleLines - 2) +
                         textD->text.cursorVPadding);
   int insertPos = TextDGetInsertPosition(textD);
   int column = 0, visLineNum, lineStartPos;
   int pos, targetLine;
   int pageForwardCount = max(1, textD->nVisibleLines - 1);
   int maintainColumn = 0;
   bool silent = hasKey("nobell", args, nArgs);

   maintainColumn = hasKey("column", args, nArgs);
   cancelDrag(w);
   if (hasKey("scrollbar", args, nArgs))   /* scrollbar only */
   {
      targetLine = min(textD->topLineNum + pageForwardCount, lastTopLine);

      if (targetLine == textD->topLineNum)
      {
         ringIfNecessary(silent);
         return;
      }
      TextDSetScroll(textD, targetLine, textD->horizOffset);
   }
   else if (hasKey("stutter", args, nArgs))   /* Mac style */
   {
      /* move to bottom line of visible area */
      /* if already there, page down maintaining preferrred column */
      targetLine = max(min(textD->nVisibleLines - 1, textD->nBufferLines), 0);
      column = TextDPreferredColumn(textD, &visLineNum, &lineStartPos);
      if (lineStartPos == textD->lineStarts[targetLine])
      {
         if (insertPos >= buf->length || textD->topLineNum == lastTopLine)
         {
            ringIfNecessary(silent);
            return;
         }
         targetLine = min(textD->topLineNum + pageForwardCount, lastTopLine);
         pos = TextDCountForwardNLines(textD, insertPos, pageForwardCount, false);
         if (maintainColumn)
         {
            pos = TextDPosOfPreferredCol(textD, column, pos);
         }
         TextDSetInsertPosition(textD, pos);
         TextDSetScroll(textD, targetLine, textD->horizOffset);
      }
      else
      {
         pos = textD->lineStarts[targetLine];
         while (targetLine > 0 && pos == -1)
         {
            --targetLine;
            pos = textD->lineStarts[targetLine];
         }
         if (lineStartPos == pos)
         {
            ringIfNecessary(silent);
            return;
         }
         if (maintainColumn)
         {
            pos = TextDPosOfPreferredCol(textD, column, pos);
         }
         TextDSetInsertPosition(textD, pos);
      }
      checkMoveSelectionChange(w, event, insertPos, args, nArgs);
      checkAutoShowInsertPos(w);
      callCursorMovementCBs(w, event);
      if (maintainColumn)
      {
         textD->cursorPreferredCol = column;
      }
      else
      {
         textD->cursorPreferredCol = -1;
      }
   }
   else   /* "standard" */
   {
      if (insertPos >= buf->length && textD->topLineNum == lastTopLine)
      {
         ringIfNecessary(silent);
         return;
      }
      if (maintainColumn)
      {
         column = TextDPreferredColumn(textD, &visLineNum, &lineStartPos);
      }
      targetLine = textD->topLineNum + textD->nVisibleLines - 1;
      if (targetLine < 1) targetLine = 1;
      if (targetLine > lastTopLine) targetLine = lastTopLine;
      pos = TextDCountForwardNLines(textD, insertPos, textD->nVisibleLines-1, false);
      if (maintainColumn)
      {
         pos = TextDPosOfPreferredCol(textD, column, pos);
      }
      TextDSetInsertPosition(textD, pos);
      TextDSetScroll(textD, targetLine, textD->horizOffset);
      checkMoveSelectionChange(w, event, insertPos, args, nArgs);
      checkAutoShowInsertPos(w);
      callCursorMovementCBs(w, event);
      if (maintainColumn)
      {
         textD->cursorPreferredCol = column;
      }
      else
      {
         textD->cursorPreferredCol = -1;
      }
   }
}

static void previousPageAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);
   int column = 0, visLineNum, lineStartPos;
   int pos, targetLine;
   int pageBackwardCount = max(1, textD->nVisibleLines - 1);
   int maintainColumn = 0;
   bool silent = hasKey("nobell", args, nArgs);

   maintainColumn = hasKey("column", args, nArgs);
   cancelDrag(w);
   if (hasKey("scrollbar", args, nArgs))   /* scrollbar only */
   {
      targetLine = max(textD->topLineNum - pageBackwardCount, 1);

      if (targetLine == textD->topLineNum)
      {
         ringIfNecessary(silent);
         return;
      }
      TextDSetScroll(textD, targetLine, textD->horizOffset);
   }
   else if (hasKey("stutter", args, nArgs))   /* Mac style */
   {
      /* move to top line of visible area */
      /* if already there, page up maintaining preferrred column if required */
      targetLine = 0;
      column = TextDPreferredColumn(textD, &visLineNum, &lineStartPos);
      if (lineStartPos == textD->lineStarts[targetLine])
      {
         if (textD->topLineNum == 1 && (maintainColumn || column == 0))
         {
            ringIfNecessary(silent);
            return;
         }
         targetLine = max(textD->topLineNum - pageBackwardCount, 1);
         pos = TextDCountBackwardNLines(textD, insertPos, pageBackwardCount);
         if (maintainColumn)
         {
            pos = TextDPosOfPreferredCol(textD, column, pos);
         }
         TextDSetInsertPosition(textD, pos);
         TextDSetScroll(textD, targetLine, textD->horizOffset);
      }
      else
      {
         pos = textD->lineStarts[targetLine];
         if (maintainColumn)
         {
            pos = TextDPosOfPreferredCol(textD, column, pos);
         }
         TextDSetInsertPosition(textD, pos);
      }
      checkMoveSelectionChange(w, event, insertPos, args, nArgs);
      checkAutoShowInsertPos(w);
      callCursorMovementCBs(w, event);
      if (maintainColumn)
      {
         textD->cursorPreferredCol = column;
      }
      else
      {
         textD->cursorPreferredCol = -1;
      }
   }
   else   /* "standard" */
   {
      if (insertPos <= 0 && textD->topLineNum == 1)
      {
         ringIfNecessary(silent);
         return;
      }
      if (maintainColumn)
      {
         column = TextDPreferredColumn(textD, &visLineNum, &lineStartPos);
      }
      targetLine = textD->topLineNum - (textD->nVisibleLines - 1);
      if (targetLine < 1) targetLine = 1;
      pos = TextDCountBackwardNLines(textD, insertPos, textD->nVisibleLines-1);
      if (maintainColumn)
      {
         pos = TextDPosOfPreferredCol(textD, column, pos);
      }
      TextDSetInsertPosition(textD, pos);
      TextDSetScroll(textD, targetLine, textD->horizOffset);
      checkMoveSelectionChange(w, event, insertPos, args, nArgs);
      checkAutoShowInsertPos(w);
      callCursorMovementCBs(w, event);
      if (maintainColumn)
      {
         textD->cursorPreferredCol = column;
      }
      else
      {
         textD->cursorPreferredCol = -1;
      }
   }
}

// TODO: static void pageLeftAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: {
// TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO:    Ne_Text_Buffer* buf = textD->buffer;
// TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO:    int maxCharWidth = textD->fontStruct->max_bounds.width;
// TODO:    int lineStartPos, indent, pos;
// TODO:    int horizOffset;
// TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: 
// TODO:    cancelDrag(w);
// TODO:    if (hasKey("scrollbar", args, nArgs))
// TODO:    {
// TODO:       if (textD->horizOffset == 0)
// TODO:       {
// TODO:          ringIfNecessary(silent);
// TODO:          return;
// TODO:       }
// TODO:       horizOffset = max(0, textD->horizOffset - textD->width);
// TODO:       TextDSetScroll(textD, textD->topLineNum, horizOffset);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       lineStartPos = BufStartOfLine(buf, insertPos);
// TODO:       if (insertPos == lineStartPos && textD->horizOffset == 0)
// TODO:       {
// TODO:          ringIfNecessary(silent);
// TODO:          return;
// TODO:       }
// TODO:       indent = BufCountDispChars(buf, lineStartPos, insertPos);
// TODO:       pos = BufCountForwardDispChars(buf, lineStartPos,
// TODO:                                      max(0, indent - textD->width / maxCharWidth));
// TODO:       TextDSetInsertPosition(textD, pos);
// TODO:       TextDSetScroll(textD, textD->topLineNum,
// TODO:                      max(0, textD->horizOffset - textD->width));
// TODO:       checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO:       checkAutoShowInsertPos(w);
// TODO:       callCursorMovementCBs(w, event);
// TODO:    }
// TODO: }
// TODO: 
// TODO: static void pageRightAP(Fl_Widget* w, int event, const char** args, int* nArgs)
// TODO: {
// TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO:    Ne_Text_Buffer* buf = textD->buffer;
// TODO:    int insertPos = TextDGetInsertPosition(textD);
// TODO:    int maxCharWidth = textD->fontStruct->max_bounds.width;
// TODO:    int oldHorizOffset = textD->horizOffset;
// TODO:    int lineStartPos, indent, pos;
// TODO:    int horizOffset, sliderSize, sliderMax;
// TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: 
// TODO:    cancelDrag(w);
// TODO:    if (hasKey("scrollbar", args, nArgs))
// TODO:    {
// TODO:       XtVaGetValues(textD->hScrollBar, XmNmaximum, &sliderMax,
// TODO:                     XmNsliderSize, &sliderSize, NULL);
// TODO:       horizOffset = min(textD->horizOffset + textD->width, sliderMax - sliderSize);
// TODO:       if (textD->horizOffset == horizOffset)
// TODO:       {
// TODO:          ringIfNecessary(silent);
// TODO:          return;
// TODO:       }
// TODO:       TextDSetScroll(textD, textD->topLineNum, horizOffset);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       lineStartPos = BufStartOfLine(buf, insertPos);
// TODO:       indent = BufCountDispChars(buf, lineStartPos, insertPos);
// TODO:       pos = BufCountForwardDispChars(buf, lineStartPos,
// TODO:                                      indent + textD->width / maxCharWidth);
// TODO:       TextDSetInsertPosition(textD, pos);
// TODO:       TextDSetScroll(textD, textD->topLineNum, textD->horizOffset + textD->width);
// TODO:       if (textD->horizOffset == oldHorizOffset && insertPos == pos)
// TODO:          ringIfNecessary(silent);
// TODO:       checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO:       checkAutoShowInsertPos(w);
// TODO:       callCursorMovementCBs(w, event);
// TODO:    }
// TODO: }

static void toggleOverstrikeAP(Fl_Widget* w, int event, const char** args,
                               int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   if (textD->text.overstrike)
   {
      textD->text.overstrike = false;
      TextDSetCursorStyle(textD, textD->text.heavyCursor ? NE_HEAVY_CURSOR : NE_NORMAL_CURSOR);
   }
   else
   {
      textD->text.overstrike = true;
      if (textD->cursorStyle == NE_NORMAL_CURSOR || textD->cursorStyle == NE_HEAVY_CURSOR)
         TextDSetCursorStyle(textD, NE_BLOCK_CURSOR);
   }
}

// TODO: static void scrollUpAP(Fl_Widget* w, int event, const char** args,
// TODO:                        int* nArgs)
// TODO: {
// TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO:    int topLineNum, horizOffset, nLines;
// TODO: 
// TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &nLines) != 1)
// TODO:       return;
// TODO:    if (*nArgs == 2)
// TODO:    {
// TODO:       /* Allow both 'page' and 'pages' */
// TODO:       if (strncmp(args[1], "page", 4) == 0)
// TODO:          nLines *= textD->nVisibleLines;
// TODO: 
// TODO:       /* 'line' or 'lines' is the only other valid possibility */
// TODO:       else if (strncmp(args[1], "line", 4) != 0)
// TODO:          return;
// TODO:    }
// TODO:    TextDGetScroll(textD, &topLineNum, &horizOffset);
// TODO:    TextDSetScroll(textD, topLineNum-nLines, horizOffset);
// TODO: }
// TODO: 
// TODO: static void scrollDownAP(Fl_Widget* w, int event, const char** args,
// TODO:                          int* nArgs)
// TODO: {
// TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO:    int topLineNum, horizOffset, nLines;
// TODO: 
// TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &nLines) != 1)
// TODO:       return;
// TODO:    if (*nArgs == 2)
// TODO:    {
// TODO:       /* Allow both 'page' and 'pages' */
// TODO:       if (strncmp(args[1], "page", 4) == 0)
// TODO:          nLines *= textD->nVisibleLines;
// TODO: 
// TODO:       /* 'line' or 'lines' is the only other valid possibility */
// TODO:       else if (strncmp(args[1], "line", 4) != 0)
// TODO:          return;
// TODO:    }
// TODO:    TextDGetScroll(textD, &topLineNum, &horizOffset);
// TODO:    TextDSetScroll(textD, topLineNum+nLines, horizOffset);
// TODO: }
// TODO: 
// TODO: static void scrollLeftAP(Fl_Widget* w, int event, const char** args,
// TODO:                          int* nArgs)
// TODO: {
// TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO:    int horizOffset, nPixels;
// TODO:    int sliderMax, sliderSize;
// TODO: 
// TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &nPixels) != 1)
// TODO:       return;
// TODO:    XtVaGetValues(textD->hScrollBar, XmNmaximum, &sliderMax,
// TODO:                  XmNsliderSize, &sliderSize, NULL);
// TODO:    horizOffset = min(max(0, textD->horizOffset - nPixels), sliderMax - sliderSize);
// TODO:    if (textD->horizOffset != horizOffset)
// TODO:    {
// TODO:       TextDSetScroll(textD, textD->topLineNum, horizOffset);
// TODO:    }
// TODO: }
// TODO: 
// TODO: static void scrollRightAP(Fl_Widget* w, int event, const char** args,
// TODO:                           int* nArgs)
// TODO: {
// TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO:    int horizOffset, nPixels;
// TODO:    int sliderMax, sliderSize;
// TODO: 
// TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &nPixels) != 1)
// TODO:       return;
// TODO:    XtVaGetValues(textD->hScrollBar, XmNmaximum, &sliderMax,
// TODO:                  XmNsliderSize, &sliderSize, NULL);
// TODO:    horizOffset = min(max(0, textD->horizOffset + nPixels), sliderMax - sliderSize);
// TODO:    if (textD->horizOffset != horizOffset)
// TODO:    {
// TODO:       TextDSetScroll(textD, textD->topLineNum, horizOffset);
// TODO:    }
// TODO: }
// TODO: 
// TODO: static void scrollToLineAP(Fl_Widget* w, int event, const char** args,
// TODO:                            int* nArgs)
// TODO: {
// TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO:    int topLineNum, horizOffset, lineNum;
// TODO: 
// TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &lineNum) != 1)
// TODO:       return;
// TODO:    TextDGetScroll(textD, &topLineNum, &horizOffset);
// TODO:    TextDSetScroll(textD, lineNum, horizOffset);
// TODO: }

static void selectAllAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;

   cancelDrag(w);
   BufSelect(buf, 0, buf->length);
}

static void deselectAllAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   cancelDrag(w);
   BufUnselect(textD->buffer);
}

/*
**  Called on the Intrinsic FocusIn event.
**
**  Note that the widget has no internal state about the focus, ie. it does
**  not know whether it has the focus or not.
*/
static void focusInAP(Fl_Widget* widget, int event, const char** unused1, int* unused2)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)widget;

   /* I don't entirely understand the traversal mechanism in Motif widgets,
      particularly, what leads to this widget getting a focus-in event when
      it does not actually have the input focus.  The temporary solution is
      to do the comparison below, and not show the cursor when Motif says
      we don't have focus, but keep looking for the real answer */

   /* If the timer is not already started, start it */
   if (textD->cursorBlinkRate != 0 && textD->text.cursorBlinkProcID == 0)
   {
      textD->text.cursorBlinkProcID = 1;
      Fl::add_timeout(textD->cursorBlinkRate/1000.0, cursorBlinkTimerProc, textD);
   }

   /* Change the cursor to active style */
   if (textD->text.overstrike)
      TextDSetCursorStyle(textD, NE_BLOCK_CURSOR);
   else
      TextDSetCursorStyle(textD, (textD->text.heavyCursor ? NE_HEAVY_CURSOR : NE_NORMAL_CURSOR));
   TextDUnblankCursor(textD);

   /* Call any registered focus-in callbacks */
// TODO:    XtCallCallbacks((Fl_Widget*) widget, textNfocusCallback, (void*) event);
}

static void focusOutAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   /* Remove the cursor blinking timer procedure */
   textD->text.cursorBlinkProcID = 0;

   /* Leave a dim or destination cursor */
   TextDSetCursorStyle(textD, textD->text.motifDestOwner ? NE_CARET_CURSOR : NE_DIM_CURSOR);

   /* If there's a calltip displayed, kill it. */
// TODO:    TextDKillCalltip(textD, 0);

   /* Call any registered focus-out callbacks */
// TODO:    XtCallCallbacks((Fl_Widget*)w, textNlosingFocusCallback, (XtPointer)event);
}

/*
** For actions involving cursor movement, "extend" keyword means incorporate
** the new cursor position in the selection, and lack of an "extend" keyword
** means cancel the existing selection
*/
static void checkMoveSelectionChange(Fl_Widget* w, int event, int startPos,
                                     const char** args, int* nArgs)
{
      Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   if (hasKey("extend", args, nArgs))
      keyMoveExtendSelection(w, event, startPos, hasKey("rect", args, nArgs));
   else
      BufUnselect(textD->buffer);
}

/*
** If a selection change was requested via a keyboard command for moving
** the insertion cursor (usually with the "extend" keyword), adjust the
** selection to include the new cursor position, or begin a new selection
** between startPos and the new cursor position with anchor at startPos.
*/
static void keyMoveExtendSelection(Fl_Widget* w, int event, int origPos, int rectangular)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   Ne_Text_Buffer* buf = textD->buffer;
   selection* sel = &buf->primary;
   int newPos = TextDGetInsertPosition(textD);
   int startPos, endPos, startCol, endCol, newCol, origCol;
   int anchor, rectAnchor, anchorLineStart;

   /* Moving the cursor does not take the Motif destination, but as soon as
      the user selects something, grab it (I'm not sure if this distinction
      actually makes sense, but it's what Motif was doing, back when their
      secondary selections actually worked correctly) */
   TakeMotifDestination(textD, GetTimeOfDay());

   if ((sel->selected || sel->zeroWidth) && sel->rectangular && rectangular)
   {
      /* rect -> rect */
      newCol = BufCountDispChars(buf, BufStartOfLine(buf, newPos), newPos);
      startCol = min(textD->text.rectAnchor, newCol);
      endCol   = max(textD->text.rectAnchor, newCol);
      startPos = BufStartOfLine(buf, min(textD->text.anchor, newPos));
      endPos = BufEndOfLine(buf, max(textD->text.anchor, newPos));
      BufRectSelect(buf, startPos, endPos, startCol, endCol);
   }
   else if (sel->selected && rectangular)     /* plain -> rect */
   {
      newCol = BufCountDispChars(buf, BufStartOfLine(buf, newPos), newPos);
      if (abs(newPos - sel->start) < abs(newPos - sel->end))
         anchor = sel->end;
      else
         anchor = sel->start;
      anchorLineStart = BufStartOfLine(buf, anchor);
      rectAnchor = BufCountDispChars(buf, anchorLineStart, anchor);
      textD->text.anchor = anchor;
      textD->text.rectAnchor = rectAnchor;
      BufRectSelect(buf, BufStartOfLine(buf, min(anchor, newPos)),
                    BufEndOfLine(buf, max(anchor, newPos)),
                    min(rectAnchor, newCol), max(rectAnchor, newCol));
   }
   else if (sel->selected && sel->rectangular)     /* rect -> plain */
   {
      startPos = BufCountForwardDispChars(buf,
                                          BufStartOfLine(buf, sel->start), sel->rectStart);
      endPos = BufCountForwardDispChars(buf,
                                        BufStartOfLine(buf, sel->end), sel->rectEnd);
      if (abs(origPos - startPos) < abs(origPos - endPos))
         anchor = endPos;
      else
         anchor = startPos;
      BufSelect(buf, anchor, newPos);
   }
   else if (sel->selected)     /* plain -> plain */
   {
      if (abs(origPos - sel->start) < abs(origPos - sel->end))
         anchor = sel->end;
      else
         anchor = sel->start;
      BufSelect(buf, anchor, newPos);
   }
   else if (rectangular)     /* no sel -> rect */
   {
      origCol = BufCountDispChars(buf, BufStartOfLine(buf, origPos), origPos);
      newCol = BufCountDispChars(buf, BufStartOfLine(buf, newPos), newPos);
      startCol = min(newCol, origCol);
      endCol = max(newCol, origCol);
      startPos = BufStartOfLine(buf, min(origPos, newPos));
      endPos = BufEndOfLine(buf, max(origPos, newPos));
      textD->text.anchor = origPos;
      textD->text.rectAnchor = origCol;
      BufRectSelect(buf, startPos, endPos, startCol, endCol);
   }
   else     /* no sel -> plain */
   {
      textD->text.anchor = origPos;
      textD->text.rectAnchor = BufCountDispChars(buf,
                                              BufStartOfLine(buf, origPos), origPos);
      BufSelect(buf, textD->text.anchor, newPos);
   }
}

static void checkAutoShowInsertPos(Fl_Widget* w)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   if (textD->text.autoShowInsertPos)
      TextDMakeInsertPosVisible(textD);
}

static int checkReadOnly(Fl_Widget* w)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   if (textD->text.readOnly)
   {
      fl_beep();
      return true;
   }
   return false;
}

/*
** Insert text "chars" at the cursor position, as if the text had been
** typed.  Same as TextInsertAtCursor, but without the complicated auto-wrap
** scanning and re-formatting.
*/
static void simpleInsertAtCursor(Fl_Widget* w, char* chars, int event, int allowPendingDelete)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   char* c;

   if (allowPendingDelete && pendingSelection(w))
   {
      BufReplaceSelected(buf, chars);
      TextDSetInsertPosition(textD, buf->cursorPosHint);
   }
   else if (textD->text.overstrike)
   {
      for (c=chars; *c!='\0' && *c!='\n'; c++);
      if (*c == '\n')
         TextDInsert(textD, chars);
      else
         TextDOverstrike(textD, chars);
   }
   else
      TextDInsert(textD, chars);
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);
}

/*
** If there's a selection, delete it and position the cursor where the
** selection was deleted.  (Called by routines which do deletion to check
** first for and do possible selection delete)
*/
static int deletePendingSelection(Fl_Widget* w, int event)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;

   if (textD->buffer->primary.selected)
   {
      BufRemoveSelected(buf);
      TextDSetInsertPosition(textD, buf->cursorPosHint);
      checkAutoShowInsertPos(w);
      callCursorMovementCBs(w, event);
      return true;
   }
   else
      return false;
}

/*
** Return true if pending delete is on and there's a selection contiguous
** with the cursor ready to be deleted.  These criteria are used to decide
** if typing a character or inserting something should delete the selection
** first.
*/
static int pendingSelection(Fl_Widget* w)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   selection* sel = &textD->buffer->primary;
   int pos = TextDGetInsertPosition(textD);

   return (textD->text.pendingDelete && sel->selected && pos >= sel->start && pos <= sel->end);
}

/*
** Check if tab emulation is on and if there are emulated tabs before the
** cursor, and if so, delete an emulated tab as a unit.  Also finishes up
** by calling checkAutoShowInsertPos and callCursorMovementCBs, so the
** calling action proc can just return (this is necessary to preserve
** emTabsBeforeCursor which is otherwise cleared by callCursorMovementCBs).
*/
static int deleteEmulatedTab(Fl_Widget* w, int event)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   int emTabDist = textD->text.emulateTabs;
   int emTabsBeforeCursor = textD->text.emTabsBeforeCursor;
   int startIndent, toIndent, insertPos, startPos, lineStart;
   int pos, indent, startPosIndent;
   char c, *spaceString;

   if (emTabDist <= 0 || emTabsBeforeCursor <= 0)
      return false;

   /* Find the position of the previous tab stop */
   insertPos = TextDGetInsertPosition(textD);
   lineStart = BufStartOfLine(buf, insertPos);
   startIndent = BufCountDispChars(buf, lineStart, insertPos);
   toIndent = (startIndent-1) - ((startIndent-1) % emTabDist);

   /* Find the position at which to begin deleting (stop at non-whitespace
      characters) */
   startPosIndent = indent = 0;
   startPos = lineStart;
   for (pos=lineStart; pos < insertPos; pos++)
   {
      c = BufGetCharacter(buf, pos);
      indent += BufCharWidth(c, indent, buf->tabDist, buf->nullSubsChar);
      if (indent > toIndent)
         break;
      startPosIndent = indent;
      startPos = pos + 1;
   }

   /* Just to make sure, check that we're not deleting any non-white chars */
   for (pos=insertPos-1; pos>=startPos; pos--)
   {
      c = BufGetCharacter(buf, pos);
      if (c != ' ' && c != '\t')
      {
         startPos = pos + 1;
         break;
      }
   }

   /* Do the text replacement and reposition the cursor.  If any spaces need
      to be inserted to make up for a deleted tab, do a BufReplace, otherwise,
      do a BufRemove. */
   if (startPosIndent < toIndent)
   {
      spaceString = (char*)malloc__(toIndent - startPosIndent + 1);
      memset(spaceString, ' ', toIndent-startPosIndent);
      spaceString[toIndent - startPosIndent] = '\0';
      BufReplace(buf, startPos, insertPos, spaceString);
      TextDSetInsertPosition(textD, startPos + toIndent - startPosIndent);
      free__(spaceString);
   }
   else
   {
      BufRemove(buf, startPos, insertPos);
      TextDSetInsertPosition(textD, startPos);
   }

   /* The normal cursor movement stuff would usually be called by the action
      routine, but this wraps around it to restore emTabsBeforeCursor */
   checkAutoShowInsertPos(w);
   callCursorMovementCBs(w, event);

   /* Decrement and restore the marker for consecutive emulated tabs, which
      would otherwise have been zeroed by callCursorMovementCBs */
   textD->text.emTabsBeforeCursor = emTabsBeforeCursor - 1;
   return true;
}

/*
** Select the word or whitespace adjacent to the cursor, and move the cursor
** to its end.  pointerX is used as a tie-breaker, when the cursor is at the
** boundary between a word and some white-space.  If the cursor is on the
** left, the word or space on the left is used.  If it's on the right, that
** is used instead.
*/
static void selectWord(Fl_Widget* w, int pointerX)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   int x, y, insertPos = TextDGetInsertPosition(textD);

   TextPosToXY(textD, insertPos, &x, &y);
   if (pointerX < x && insertPos > 0 && BufGetCharacter(buf, insertPos-1) != '\n')
      insertPos--;
   BufSelect(buf, startOfWord(w, insertPos), endOfWord(w, insertPos));
}

static int startOfWord(Fl_Widget* w, int pos)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   int startPos;
   Ne_Text_Buffer* buf = textD->buffer;
   const std::string& delimiters=textD->text.delimiters;
   char c = BufGetCharacter(buf, pos);

   if (c == ' ' || c== '\t')
   {
      if (!spanBackward(buf, pos, " \t", false, &startPos))
         return 0;
   }
   else if (strchr(delimiters.c_str(), c))
   {
      if (!spanBackward(buf, pos, delimiters.c_str(), true, &startPos))
         return 0;
   }
   else
   {
      if (!BufSearchBackward(buf, pos, delimiters.c_str(), &startPos))
         return 0;
   }
   return min(pos, startPos+1);

}

static int endOfWord(Fl_Widget* w, int pos)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int endPos;
   Ne_Text_Buffer* buf = textD->buffer;
   const std::string& delimiters = textD->text.delimiters;
   char c = BufGetCharacter(buf, pos);

   if (c == ' ' || c== '\t')
   {
      if (!spanForward(buf, pos, " \t", false, &endPos))
         return buf->length;
   }
   else if (strchr(delimiters.c_str(), c))
   {
      if (!spanForward(buf, pos, delimiters.c_str(), true, &endPos))
         return buf->length;
   }
   else
   {
      if (!BufSearchForward(buf, pos, delimiters.c_str(), &endPos))
         return buf->length;
   }
   return endPos;
}

/*
** Search forwards in buffer "buf" for the first character NOT in
** "searchChars",  starting with the character "startPos", and returning the
** result in "foundPos" returns True if found, false if not. If ignoreSpace
** is set, then Space, Tab, and Newlines are ignored in searchChars.
*/
static int spanForward(Ne_Text_Buffer* buf, int startPos, const char* searchChars, int ignoreSpace, int* foundPos)
{
   int pos;
   const char* c;

   pos = startPos;
   while (pos < buf->length)
   {
      for (c=searchChars; *c!='\0'; c++)
         if (!(ignoreSpace && (*c==' ' || *c=='\t' || *c=='\n')))
            if (BufGetCharacter(buf, pos) == *c)
               break;
      if (*c == 0)
      {
         *foundPos = pos;
         return true;
      }
      pos++;
   }
   *foundPos = buf->length;
   return false;
}

/*
** Search backwards in buffer "buf" for the first character NOT in
** "searchChars",  starting with the character BEFORE "startPos", returning the
** result in "foundPos" returns True if found, false if not. If ignoreSpace is
** set, then Space, Tab, and Newlines are ignored in searchChars.
*/
static int spanBackward(Ne_Text_Buffer* buf, int startPos, const char* searchChars, int ignoreSpace, int* foundPos)
{
   int pos;
   const char* c;

   if (startPos == 0)
   {
      *foundPos = 0;
      return false;
   }
   pos = startPos == 0 ? 0 : startPos - 1;
   while (pos >= 0)
   {
      for (c=searchChars; *c!='\0'; c++)
         if (!(ignoreSpace && (*c==' ' || *c=='\t' || *c=='\n')))
            if (BufGetCharacter(buf, pos) == *c)
               break;
      if (*c == 0)
      {
         *foundPos = pos;
         return true;
      }
      pos--;
   }
   *foundPos = 0;
   return false;
}

/*
** Select the line containing the cursor, including the terminating newline,
** and move the cursor to its end.
*/
static void selectLine(Fl_Widget* w)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = TextDGetInsertPosition(textD);
   int endPos, startPos;

   endPos = BufEndOfLine(textD->buffer, insertPos);
   startPos = BufStartOfLine(textD->buffer, insertPos);
   BufSelect(textD->buffer, startPos, min(endPos + 1, textD->buffer->length));
   TextDSetInsertPosition(textD, endPos);
}

/*
** Given a new mouse pointer location, pass the position on to the
** autoscroll timer routine, and make sure the timer is on when it's
** needed and off when it's not.
*/
static void checkAutoScroll(Fl_Widget* w, int x, int y)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   /* Is the pointer in or out of the window? */
   bool  inWindow = (x >= textD->left &&
              x < textD->width - textD->marginWidth &&
              y >= textD->marginHeight &&
              y < textD->height - textD->marginHeight);

   /* If it's in the window, cancel the timer procedure */
   if (inWindow)
   {
// TODO:       if (w->text.autoScrollProcID != 0)
// TODO:          XtRemoveTimeOut(w->text.autoScrollProcID);;
// TODO:       w->text.autoScrollProcID = 0;
      return;
   }

// TODO:    /* If the timer is not already started, start it */
// TODO:    if (w->text.autoScrollProcID == 0)
// TODO:    {
// TODO:       w->text.autoScrollProcID = XtAppAddTimeOut(
// TODO:                                     XtWidgetToApplicationContext((Fl_Widget*)w),
// TODO:                                     0, autoScrollTimerProc, w);
// TODO:    }

   /* Pass on the newest mouse location to the autoscroll routine */
   textD->text.mouseX = x;
   textD->text.mouseY = y;
}

/*
** Reset drag state and cancel the auto-scroll timer
*/
static void endDrag(Fl_Widget* w)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
// TODO:    if (((TextWidget)w)->text.autoScrollProcID != 0)
// TODO:       XtRemoveTimeOut(((TextWidget)w)->text.autoScrollProcID);
// TODO:    ((TextWidget)w)->text.autoScrollProcID = 0;
// TODO:    if (((TextWidget)w)->text.dragState == MOUSE_PAN)
// TODO:       XUngrabPointer(XtDisplay(w), CurrentTime);
   textD->text.dragState = NOT_CLICKED;
}

/*
** Cancel any drag operation that might be in progress.  Should be included
** in nearly every key event to cleanly end any dragging before edits are made
** which might change the insert position or the content of the buffer during
** a drag operation)
*/
static void cancelDrag(Fl_Widget* w)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   int dragState = textD->text.dragState;

// TODO:    if (((TextWidget)w)->text.autoScrollProcID != 0)
// TODO:       XtRemoveTimeOut(((TextWidget)w)->text.autoScrollProcID);
   if (dragState == SECONDARY_DRAG || dragState == SECONDARY_RECT_DRAG)
      BufSecondaryUnselect(textD->buffer);
   if (dragState == PRIMARY_BLOCK_DRAG)
      CancelBlockDrag(textD);
// TODO:    if (dragState == MOUSE_PAN)
// TODO:       XUngrabPointer(XtDisplay(w), CurrentTime);
   if (dragState != NOT_CLICKED)
      textD->text.dragState = DRAG_CANCELED;
}

/*
** Do operations triggered by cursor movement: Call cursor movement callback
** procedure(s), and cancel marker indicating that the cursor is after one or
** more just-entered emulated tabs (spaces to be deleted as a unit).
*/
static void callCursorMovementCBs(Fl_Widget* w, int event)
{
// TODO:    ((TextWidget)w)->text.emTabsBeforeCursor = 0;
// TODO:    XtCallCallbacks((Fl_Widget*)w, textNcursorMovementCallback, (XtPointer)event);
}

/*
** Adjust the selection as the mouse is dragged to position: (x, y).
*/
static void adjustSelection(Fl_Widget* w, int x, int y)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   int row, col, startCol, endCol, startPos, endPos;
   int newPos = TextDXYToPosition(textD, x, y);

   /* Adjust the selection */
   if (textD->text.dragState == PRIMARY_RECT_DRAG)
   {
      TextDXYToUnconstrainedPosition(textD, x, y, &row, &col);
      col = TextDOffsetWrappedColumn(textD, row, col);
      startCol = min(textD->text.rectAnchor, col);
      endCol = max(textD->text.rectAnchor, col);
      startPos = BufStartOfLine(buf, min(textD->text.anchor, newPos));
      endPos = BufEndOfLine(buf, max(textD->text.anchor, newPos));
      BufRectSelect(buf, startPos, endPos, startCol, endCol);
   }
   else if (textD->text.multiClickState == ONE_CLICK)
   {
      startPos = startOfWord(textD, min(textD->text.anchor, newPos));
      endPos = endOfWord(textD, max(textD->text.anchor, newPos));
      BufSelect(buf, startPos, endPos);
      newPos = newPos < textD->text.anchor ? startPos : endPos;
   }
   else if (textD->text.multiClickState == TWO_CLICKS)
   {
      startPos = BufStartOfLine(buf, min(textD->text.anchor, newPos));
      endPos = BufEndOfLine(buf, max(textD->text.anchor, newPos));
      BufSelect(buf, startPos, min(endPos+1, buf->length));
      newPos = newPos < textD->text.anchor ? startPos : endPos;
   }
   else
      BufSelect(buf, textD->text.anchor, newPos);

   /* Move the cursor */
   TextDSetInsertPosition(textD, newPos);
   callCursorMovementCBs(w, NULL);
}

static void adjustSecondarySelection(Ne_Text_Editor* textD, int x, int y)
{
   Ne_Text_Buffer* buf = textD->buffer;
   int row, col, startCol, endCol, startPos, endPos;
   int newPos = TextDXYToPosition(textD, x, y);

   if (textD->text.dragState == SECONDARY_RECT_DRAG)
   {
      TextDXYToUnconstrainedPosition(textD, x, y, &row, &col);
      col = TextDOffsetWrappedColumn(textD, row, col);
      startCol = min(textD->text.rectAnchor, col);
      endCol = max(textD->text.rectAnchor, col);
      startPos = BufStartOfLine(buf, min(textD->text.anchor, newPos));
      endPos = BufEndOfLine(buf, max(textD->text.anchor, newPos));
      BufSecRectSelect(buf, startPos, endPos, startCol, endCol);
   }
   else
      BufSecondarySelect(textD->buffer, textD->text.anchor, newPos);
}

static void insertClipboard(Ne_Text_Editor* textD)
{
   const char* text = Fl::event_text();
   if (!text)
   {
      fl_beep();
      return;
   }

   char* string;
   {
      const char* utf8 = Fl::event_text();
      string = new char[Fl::event_length()+1];
      fl_utf8toa(utf8, Fl::event_length()+1, string, Fl::event_length()+1);
   }

   int retLength = strlen(string);

   /* If the string contains ascii-nul characters, substitute something
      else, or give up, warn, and refuse */
   if (!BufSubstituteNullChars(string, retLength, textD->buffer))
   {
      fprintf(stderr, "Too much binary data, text not pasted\n");
      delete[] string;
      return;
   }

   /* Insert it in the text widget */
   if (textD->text.isColumnar && !textD->buffer->primary.selected)
   {
      int cursorPos = TextDGetInsertPosition(textD);
      int cursorLineStart = BufStartOfLine(textD->buffer, cursorPos);
      int column = BufCountDispChars(textD->buffer, cursorLineStart, cursorPos);
      if (textD->text.overstrike)
      {
         BufOverlayRect(textD->buffer, cursorLineStart, column, -1, string, NULL, NULL);
      }
      else
      {
         BufInsertCol(textD->buffer, column, cursorLineStart, string, NULL, NULL);
      }
      TextDSetInsertPosition(textD, BufCountForwardDispChars(textD->buffer, cursorLineStart, column));
      if (textD->text.autoShowInsertPos)
         TextDMakeInsertPosVisible(textD);
   }
   else
      TextInsertAtCursor(textD, string, NULL, true, textD->text.autoWrapPastedText);
   delete[] string;
}

/*
** Wrap multi-line text in argument "text" to be inserted at the end of the
** text on line "startLine" and return the result.  If "breakBefore" is
** non-NULL, allow wrapping to extend back into "startLine", in which case
** the returned text will include the wrapped part of "startLine", and
** "breakBefore" will return the number of characters at the end of
** "startLine" that were absorbed into the returned string.  "breakBefore"
** will return zero if no characters were absorbed into the returned string.
** The buffer offset of text in the widget's text buffer is needed so that
** smart indent (which can be triggered by wrapping) can search back farther
** in the buffer than just the text in startLine.
*/
static char* wrapText(Fl_Widget* w, char* startLine, char* text, int bufOffset,
                      int wrapMargin, int* breakBefore)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* wrapBuf, *buf = textD->buffer;
   int startLineLen = strlen(startLine);
   int colNum, pos, lineStartPos, limitPos, breakAt, charsAdded;
   int firstBreak = -1, tabDist = buf->tabDist;
   char c, *wrappedText;

   /* Create a temporary text buffer and load it with the strings */
   wrapBuf = BufCreate();
   BufInsert(wrapBuf, 0, startLine);
   BufInsert(wrapBuf, wrapBuf->length, text);

   /* Scan the buffer for long lines and apply wrapLine when wrapMargin is
      exceeded.  limitPos enforces no breaks in the "startLine" part of the
      string (if requested), and prevents re-scanning of long unbreakable
      lines for each character beyond the margin */
   colNum = 0;
   pos = 0;
   lineStartPos = 0;
   limitPos = breakBefore == NULL ? startLineLen : 0;
   while (pos < wrapBuf->length)
   {
      c = BufGetCharacter(wrapBuf, pos);
      if (c == '\n')
      {
         lineStartPos = limitPos = pos + 1;
         colNum = 0;
      }
      else
      {
         colNum += BufCharWidth(c, colNum, tabDist, buf->nullSubsChar);
         if (colNum > wrapMargin)
         {
            if (!wrapLine(w, wrapBuf, bufOffset, lineStartPos, pos,
                          limitPos, &breakAt, &charsAdded))
            {
               limitPos = max(pos, limitPos);
            }
            else
            {
               lineStartPos = limitPos = breakAt+1;
               pos += charsAdded;
               colNum = BufCountDispChars(wrapBuf, lineStartPos, pos+1);
               if (firstBreak == -1)
                  firstBreak = breakAt;
            }
         }
      }
      pos++;
   }

   /* Return the wrapped text, possibly including part of startLine */
   if (breakBefore == NULL)
      wrappedText = BufGetRange(wrapBuf, startLineLen, wrapBuf->length);
   else
   {
      *breakBefore = firstBreak != -1 && firstBreak < startLineLen ?
                     startLineLen - firstBreak : 0;
      wrappedText = BufGetRange(wrapBuf, startLineLen - *breakBefore,
                                wrapBuf->length);
   }
   BufFree(wrapBuf);
   return wrappedText;
}

/*
** Wraps the end of a line beginning at lineStartPos and ending at lineEndPos
** in "buf", at the last white-space on the line >= limitPos.  (The implicit
** assumption is that just the last character of the line exceeds the wrap
** margin, and anywhere on the line we can wrap is correct).  Returns false if
** unable to wrap the line.  "breakAt", returns the character position at
** which the line was broken,
**
** Auto-wrapping can also trigger auto-indent.  The additional parameter
** bufOffset is needed when auto-indent is set to smart indent and the smart
** indent routines need to scan far back in the buffer.  "charsAdded" returns
** the number of characters added to acheive the auto-indent.  wrapMargin is
** used to decide whether auto-indent should be skipped because the indent
** string itself would exceed the wrap margin.
*/
static int wrapLine(Fl_Widget* w, Ne_Text_Buffer* buf, int bufOffset,
                    int lineStartPos, int lineEndPos, int limitPos, int* breakAt,
                    int* charsAdded)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   int p, length, column;
   char c, *indentStr;

   /* Scan backward for whitespace or BOL.  If BOL, return false, no
      whitespace in line at which to wrap */
   for (p=lineEndPos; ; p--)
   {
      if (p < lineStartPos || p < limitPos)
         return false;
      c = BufGetCharacter(buf, p);
      if (c == '\t' || c == ' ')
         break;
   }

   /* Create an auto-indent string to insert to do wrap.  If the auto
      indent string reaches the wrap position, slice the auto-indent
      back off and return to the left margin */
   if (textD->text.autoIndent || textD->text.smartIndent)
   {
      indentStr = createIndentString(w, buf, bufOffset, lineStartPos,
                                     lineEndPos, &length, &column);
      if (column >= p-lineStartPos)
         indentStr[1] = '\0';
   }
   else
   {
      indentStr = "\n";
      length = 1;
   }

   /* Replace the whitespace character with the auto-indent string
      and return the stats */
   BufReplace(buf, p, p+1, indentStr);
   if (textD->text.autoIndent || textD->text.smartIndent)
      free__(indentStr);
   *breakAt = p;
   *charsAdded = length-1;
   return true;
}

/*
** Create and return an auto-indent string to add a newline at lineEndPos to a
** line starting at lineStartPos in buf.  "buf" may or may not be the real
** text buffer for the widget.  If it is not the widget's text buffer it's
** offset position from the real buffer must be specified in "bufOffset" to
** allow the smart-indent routines to scan back as far as necessary. The
** string length is returned in "length" (or "length" can be passed as NULL,
** and the indent column is returned in "column" (if non NULL).
*/
static char* createIndentString(Fl_Widget* w, Ne_Text_Buffer* buf, int bufOffset,
                                int lineStartPos, int lineEndPos, int* length, int* column)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int pos, indent = -1, tabDist = textD->buffer->tabDist;
   int i, useTabs = textD->buffer->useTabs;
   char* indentPtr, *indentStr, c;
   smartIndentCBStruct smartIndent;

   /* If smart indent is on, call the smart indent callback.  It is not
      called when multi-line changes are being made (lineStartPos != 0),
      because smart indent needs to search back an indeterminate distance
      through the buffer, and reconciling that with wrapping changes made,
      but not yet committed in the buffer, would make programming smart
      indent more difficult for users and make everything more complicated */
   if (textD->text.smartIndent && (lineStartPos == 0 || buf == textD->buffer))
   {
      smartIndent.reason = NEWLINE_INDENT_NEEDED;
      smartIndent.pos = lineEndPos + bufOffset;
      smartIndent.indentRequest = 0;
      smartIndent.charsTyped = NULL;
// TODO:       XtCallCallbacks((Fl_Widget*)tw, textNsmartIndentCallback, (XtPointer)&smartIndent);
      indent = smartIndent.indentRequest;
   }

   /* If smart indent wasn't used, measure the indent distance of the line */
   if (indent == -1)
   {
      indent = 0;
      for (pos=lineStartPos; pos<lineEndPos; pos++)
      {
         c =  BufGetCharacter(buf, pos);
         if (c != ' ' && c != '\t')
            break;
         if (c == '\t')
            indent += tabDist - (indent % tabDist);
         else
            indent++;
      }
   }

   /* Allocate and create a string of tabs and spaces to achieve the indent */
   indentPtr = indentStr = (char*)malloc__(indent + 2);
   *indentPtr++ = '\n';
   if (useTabs)
   {
      for (i=0; i < indent / tabDist; i++)
         *indentPtr++ = '\t';
      for (i=0; i < indent % tabDist; i++)
         *indentPtr++ = ' ';
   }
   else
   {
      for (i=0; i < indent; i++)
         *indentPtr++ = ' ';
   }
   *indentPtr = '\0';

   /* Return any requested stats */
   if (length != NULL)
      *length = indentPtr - indentStr;
   if (column != NULL)
      *column = indent;

   return indentStr;
}

// TODO: /*
// TODO: ** Xt timer procedure for autoscrolling
// TODO: */
// TODO: static void autoScrollTimerProc(XtPointer clientData, XtIntervalId* id)
// TODO: {
// TODO:    TextWidget w = (TextWidget)clientData;
// TODO:    textDisp* textD = w->text.textD;
// TODO:    int topLineNum, horizOffset, newPos, cursorX, y;
// TODO:    int fontWidth = textD->fontStruct->max_bounds.width;
// TODO:    int fontHeight = textD->fontStruct->ascent + textD->fontStruct->descent;
// TODO: 
// TODO:    /* For vertical autoscrolling just dragging the mouse outside of the top
// TODO:       or bottom of the window is sufficient, for horizontal (non-rectangular)
// TODO:       scrolling, see if the position where the CURSOR would go is outside */
// TODO:    newPos = TextDXYToPosition(textD, w->text.mouseX, w->text.mouseY);
// TODO:    if (w->text.dragState == PRIMARY_RECT_DRAG)
// TODO:       cursorX = w->text.mouseX;
// TODO:    else if (!TextDPositionToXY(textD, newPos, &cursorX, &y))
// TODO:       cursorX = w->text.mouseX;
// TODO: 
// TODO:    /* Scroll away from the pointer, 1 character (horizontal), or 1 character
// TODO:       for each fontHeight distance from the mouse to the text (vertical) */
// TODO:    TextDGetScroll(textD, &topLineNum, &horizOffset);
// TODO:    if (cursorX >= (int)w->core.width - w->text.marginWidth)
// TODO:       horizOffset += fontWidth;
// TODO:    else if (w->text.mouseX < textD->left)
// TODO:       horizOffset -= fontWidth;
// TODO:    if (w->text.mouseY >= (int)w->core.height - w->text.marginHeight)
// TODO:       topLineNum += 1 + ((w->text.mouseY - (int)w->core.height -
// TODO:                           w->text.marginHeight) / fontHeight) + 1;
// TODO:    else if (w->text.mouseY < w->text.marginHeight)
// TODO:       topLineNum -= 1 + ((w->text.marginHeight-w->text.mouseY) / fontHeight);
// TODO:    TextDSetScroll(textD, topLineNum, horizOffset);
// TODO: 
// TODO:    /* Continue the drag operation in progress.  If none is in progress
// TODO:       (safety check) don't continue to re-establish the timer proc */
// TODO:    if (w->text.dragState == PRIMARY_DRAG)
// TODO:    {
// TODO:       adjustSelection(w, w->text.mouseX, w->text.mouseY);
// TODO:    }
// TODO:    else if (w->text.dragState == PRIMARY_RECT_DRAG)
// TODO:    {
// TODO:       adjustSelection(w, w->text.mouseX, w->text.mouseY);
// TODO:    }
// TODO:    else if (w->text.dragState == SECONDARY_DRAG)
// TODO:    {
// TODO:       adjustSecondarySelection(w, w->text.mouseX, w->text.mouseY);
// TODO:    }
// TODO:    else if (w->text.dragState == SECONDARY_RECT_DRAG)
// TODO:    {
// TODO:       adjustSecondarySelection(w, w->text.mouseX, w->text.mouseY);
// TODO:    }
// TODO:    else if (w->text.dragState == PRIMARY_BLOCK_DRAG)
// TODO:    {
// TODO:       BlockDragSelection(w, w->text.mouseX, w->text.mouseY, USE_LAST);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       w->text.autoScrollProcID = 0;
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* re-establish the timer proc (this routine) to continue processing */
// TODO:    w->text.autoScrollProcID = XtAppAddTimeOut(
// TODO:                                  XtWidgetToApplicationContext((Fl_Widget*)w),
// TODO:                                  w->text.mouseY >= w->text.marginHeight &&
// TODO:                                  w->text.mouseY < w->core.height - w->text.marginHeight ?
// TODO:                                  (VERTICAL_SCROLL_DELAY*fontWidth) / fontHeight :
// TODO:                                  VERTICAL_SCROLL_DELAY, autoScrollTimerProc, w);
// TODO: }

/*
** Xt timer procedure for cursor blinking
*/
static void cursorBlinkTimerProc(void* data)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)data;

   /* Blink the cursor */
   if (textD->cursorOn)
      TextDBlankCursor(textD);
   else
      TextDUnblankCursor(textD);

   /* re-establish or not the timer proc (this routine) to continue processing */
   if (textD->text.cursorBlinkProcID)
      Fl::add_timeout(textD->cursorBlinkRate/1000.0, cursorBlinkTimerProc, textD);
   else
      TextDUnblankCursor(textD);
}

// TODO: /*
// TODO: **  Sets the caret to on or off and restart the caret blink timer.
// TODO: **  This could be used by other modules to modify the caret's blinking.
// TODO: */
// TODO: void ResetCursorBlink(TextWidget textWidget, bool startsBlanked)
// TODO: {
// TODO:    if (0 != textWidget->text.cursorBlinkRate)
// TODO:    {
// TODO:       if (0 != textWidget->text.cursorBlinkProcID)
// TODO:       {
// TODO:          XtRemoveTimeOut(textWidget->text.cursorBlinkProcID);
// TODO:       }
// TODO: 
// TODO:       if (startsBlanked)
// TODO:       {
// TODO:          TextDBlankCursor(textWidget->text.textD);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          TextDUnblankCursor(textWidget->text.textD);
// TODO:       }
// TODO: 
// TODO:       textWidget->text.cursorBlinkProcID
// TODO:          = XtAppAddTimeOut(XtWidgetToApplicationContext((Fl_Widget*) textWidget),
// TODO:                            textWidget->text.cursorBlinkRate, cursorBlinkTimerProc,
// TODO:                            textWidget);
// TODO:    }
// TODO: }

/*
** look at an action procedure's arguments to see if argument "key" has been
** specified in the argument list
*/
static bool hasKey(const char* key, const char** args, const int* nArgs)
{
   int i;

   for (i=0; i<(int)*nArgs; i++)
      if (!strCaseCmp(args[i], key))
         return true;
   return false;
}

static int max(int i1, int i2)
{
   return i1 >= i2 ? i1 : i2;
}

static int min(int i1, int i2)
{
   return i1 <= i2 ? i1 : i2;
}

/*
** strCaseCmp compares its arguments and returns 0 if the two strings
** are equal IGNORING case differences.  Otherwise returns 1.
*/

static int strCaseCmp(const char* str1, const char* str2)
{
   unsigned const char* c1 = (unsigned const char*) str1;
   unsigned const char* c2 = (unsigned const char*) str2;

   for (; *c1!='\0' && *c2!='\0'; c1++, c2++)
      if (toupper(*c1) != toupper(*c2))
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

static void ringIfNecessary(bool silent)
{
   if (!silent)
      fl_beep();
}
