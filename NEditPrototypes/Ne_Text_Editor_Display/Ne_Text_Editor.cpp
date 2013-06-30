#include "Ne_Text_Editor.h"

#include "Ne_Text_Buffer.h"
//#include "textSel.h"
#include "Ne_Text_Drag.h"
//#include "nedit.h"
//#include "calltips.h"
//#include "../util/DialogF.h"
#include "../util/utils.h"
#include "../util/misc.h"
#include "../util/Ne_AppContext.h"
//#include "window.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>
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

inline std::string filename( const std::string& filepath)
{
   std::string::size_type pos = filepath.find_last_of('\\');
   if (pos==std::string::npos)
      return filepath;

   return std::string(filepath, pos+1);
}

#define TRACE() { std::cout << "[" << filename(__FILE__) << ":" << __LINE__ << "] " << __FUNCTION__ << std::endl; }

// TODO: #include <stdio.h>
// TODO: #include <stdlib.h>
// TODO: #include <limits.h>
// TODO: #include <string.h>
// TODO: #include <ctype.h>
// TODO: #ifndef WIN32
// TODO: #include <sys/param.h>
// TODO: #endif
// TODO: #include <limits.h>


// Number of pixels of motion from the initial (grab-focus) button press
// required to begin recognizing a mouse drag for the purpose of making a selection
#define NE_SELECT_THRESHOLD 5

/* Length of delay in milliseconds for vertical autoscrolling */
#define NE_VERTICAL_SCROLL_DELAY 50

// -------------------------------------------------------------------------------
// Callback attached to the text buffer to receive delete information before
// the modifications are actually made.
// -------------------------------------------------------------------------------
static void BufferPreDeleteCB(int pos, int nDeleted, void* cbArg)
{
   ((Ne_Text_Editor*)cbArg)->bufPreDeleteCB(pos, nDeleted);
}

// -------------------------------------------------------------------------------
// Callback attached to the text buffer to receive modification information
// -------------------------------------------------------------------------------
static void BufferModifiedCB(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg)
{
   ((Ne_Text_Editor*)cbArg)->bufModifiedCB(pos, nInserted, nDeleted, nRestyled, deletedText);
}

// --------------------------------------------------------------------------
Ne_Text_Editor::Ne_Text_Editor(int x, int y, int w, int h, const char* l)
   : Ne_Text_Display(x, y, w, h, l),
   MultiClickTime(400)
{}

static void grabFocusAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void moveDestinationAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void extendAdjustAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void extendStartAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void extendEndAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void processCancelAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void secondaryStartAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void secondaryOrDragStartAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void secondaryAdjustAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void secondaryOrDragAdjustAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void copyToAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void copyToOrEndDragAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void copyPrimaryAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void cutPrimaryAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void moveToAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void moveToOrEndDragAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void endDragAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void exchangeAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void mousePanAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void pasteClipboardAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void copyClipboardAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void cutClipboardAP(Fl_Widget* w, int event, String *args,int *nArgs);
// TODO: static void insertStringAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void selfInsertAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void newlineAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void newlineAndIndentAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void newlineNoIndentAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void processTabAP(Fl_Widget* w, int event, const char**args,int *nArgs);
static void endOfLineAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void beginningOfLineAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void deleteSelectionAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void deletePreviousCharacterAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void deleteNextCharacterAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void deletePreviousWordAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void deleteNextWordAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void deleteToStartOfLineAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void deleteToEndOfLineAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void forwardCharacterAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void backwardCharacterAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void forwardWordAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void backwardWordAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void forwardParagraphAP(Fl_Widget* w, int event, const char**args,int *nArgs);
static void backwardParagraphAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void keySelectAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void processUpAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void processShiftUpAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void processDownAP(Fl_Widget* w, int event, const char**args,int *nArgs);
static void processShiftDownAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void beginningOfFileAP(Fl_Widget* w, int event, const char**args,int *nArgs);
static void endOfFileAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void nextPageAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void previousPageAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void pageLeftAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void pageRightAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void toggleOverstrikeAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void scrollUpAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void scrollDownAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void scrollLeftAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void scrollRightAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void scrollToLineAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void selectAllAP(Fl_Widget* w, int, const char** args, int* nArgs);
static void deselectAllAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void focusInAP(Fl_Widget* w, int, const char** args, int* nArgs);
// TODO: static void focusOutAP(Fl_Widget* w, int, const char** args, int* nArgs);
static bool hasKey(const char* key, const char** args, const int* nArgs);
static int strCaseCmp(const char *str1, const char *str2);
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
      "~Shift ~Ctrl Alt<KeyPress>osfBeginLine: last_document()\n"

      /* Backspace */
      "Ctrl<KeyPress>osfBackSpace: delete_previous_word()\n"
      "<KeyPress>osfBackSpace: delete_previous_character()\n"

      /* Delete */
      "Shift Ctrl<KeyPress>osfDelete: cut_primary()\n"
      "Ctrl<KeyPress>osfDelete: delete_to_end_of_line()\n"
      "Shift<KeyPress>osfDelete: cut_clipboard()\n"
      "<KeyPress>osfDelete: delete_next_character()\n"

      /* Insert */
      "Shift Ctrl<KeyPress>osfInsert: copy_primary()\n"
      "Shift<KeyPress>osfInsert: paste_clipboard()\n"
      "Ctrl<KeyPress>osfInsert: copy_clipboard()\n"
      "~Shift ~Ctrl<KeyPress>osfInsert: set_overtype_mode()\n"

      /* BeginLine */
      "Shift Ctrl<KeyPress>osfBeginLine: beginning_of_file(\"extend\")\n"
      "Ctrl<KeyPress>osfBeginLine: beginning_of_file()\n"
      "Shift<KeyPress>osfBeginLine: beginning_of_line(\"extend\")\n"
      "~Alt~Shift~Ctrl~Meta<KeyPress>osfBeginLine: beginning_of_line()\n"

      /* EndLine */
      "Shift Ctrl<KeyPress>osfEndLine: end_of_file(\"extend\")\n"
      "Ctrl<KeyPress>osfEndLine: end_of_file()\n"
      "Shift<KeyPress>osfEndLine: end_of_line(\"extend\")\n"
      "~Alt~Shift~Ctrl~Meta<KeyPress>osfEndLine: end_of_line()\n"

      /* Left */
      "Shift Ctrl<KeyPress>osfLeft: backward_word(\"extend\")\n"
      "Ctrl<KeyPress>osfLeft: backward_word()\n"
      "Shift<KeyPress>osfLeft: key_select(\"left\")\n"
      "~Alt~Shift~Ctrl~Meta<KeyPress>osfLeft: backward_character()\n"

      /* Right */
      "Shift Ctrl<KeyPress>osfRight: forward_word(\"extend\")\n"
      "Ctrl<KeyPress>osfRight: forward_word()\n"
      "Shift<KeyPress>osfRight: key_select(\"right\")\n"
      "~Alt~Shift~Ctrl~Meta<KeyPress>osfRight: forward_character()\n"

      /* Up */
      "Shift Ctrl<KeyPress>osfUp: backward_paragraph(\"extend\")\n"
      "Ctrl<KeyPress>osfUp: backward_paragraph()\n"
      "Shift<KeyPress>osfUp: process_shift_up()\n"
      "~Alt~Shift~Ctrl~Meta<KeyPress>osfUp: process_up()\n"

      /* Down */
      "Shift Ctrl<KeyPress>osfDown: forward_paragraph(\"extend\")\n"
      "Ctrl<KeyPress>osfDown: forward_paragraph()\n"
      "Shift<KeyPress>osfDown: process_shift_down()\n"
      "~Alt~Shift~Ctrl~Meta<KeyPress>osfDown: process_down()\n"

      /* PageUp */
      "Shift Ctrl<KeyPress>osfPageUp: page_left(\"extend\")\n"
      "Ctrl<KeyPress>osfPageUp: previous_document()\n"
      "Shift<KeyPress>osfPageUp: previous_page(\"extend\")\n"
      "~Alt ~Shift ~Ctrl ~Meta<KeyPress>osfPageUp: previous_page()\n"

      /* PageDown */
      "Shift Ctrl<KeyPress>osfPageDown: page_right(\"extend\")\n"
      "Ctrl<KeyPress>osfPageDown: next_document()\n"
      "Shift<KeyPress>osfPageDown: next_page(\"extend\")\n"
      "~Alt ~Shift ~Ctrl ~Meta<KeyPress>osfPageDown: next_page()\n"

      "<KeyPress>osfCancel: process_cancel()\n"

      /* Cut/Copy/Paste */
      "Ctrl~Alt~Meta<KeyPress>v: paste_clipboard()\n"
      "Ctrl~Alt~Meta<KeyPress>c: copy_clipboard()\n"
      "Ctrl~Alt~Meta<KeyPress>x: cut_clipboard()\n"
      "Ctrl~Alt~Meta<KeyPress>u: delete_to_start_of_line()\n"

      "Ctrl<KeyPress>Return: newline_and_indent()\n"
      "Shift<KeyPress>Return: newline_no_indent()\n"
      "<KeyPress>Return: newline()\n"

      /* KP_Enter = osfActivate Note: Ctrl+KP_Enter is already bound to Execute Command Line... */
      "Shift<KeyPress>osfActivate: newline_no_indent()\n"
      "<KeyPress>osfActivate: newline()\n"
      "Ctrl<KeyPress>Tab: self_insert()\n"
      "<KeyPress>Tab: process_tab()\n"
      "Shift Ctrl~Meta~Alt<KeyPress>space: key_select()\n"
      "Ctrl~Meta~Alt<KeyPress>slash: select_all()\n"
      "Ctrl~Meta~Alt<KeyPress>backslash: deselect_all()\n"
      "<KeyPress>: self_insert()\n"
      
      // <Btn1>
      "Alt Ctrl<Btn1Down>: move_destination()\n"
      "Meta Ctrl<Btn1Down>: move_destination()\n"
      "Shift<Btn1Down>: extend_start()\n"
      "<Btn1Down>: grab_focus()\n"
      "Button1~Ctrl<MotionNotify>: extend_adjust()\n"
      "<Btn1Up>: extend_end()\n"
      
      // <Btn2>
      "<Btn2Down>: secondary_or_drag_start()\n"
      "Shift Button2<MotionNotify>: secondary_or_drag_adjust(\"copy\")\n"
      "Button2<MotionNotify>: secondary_or_drag_adjust()\n"
      "Shift Ctrl<Btn2Up>: move_to_or_end_drag(\"copy\", \"overlay\")\n"
      "Shift <Btn2Up>: move_to_or_end_drag(\"copy\")\n"
      "Alt<Btn2Up>: exchange()\n"
      "Meta<Btn2Up>: exchange()\n"
      "Ctrl<Btn2Up>: copy_to_or_end_drag(\"overlay\")\n"
      "<Btn2Up>: copy_to_or_end_drag()\n"
      
      // <Btn3>
      "Ctrl~Meta~Alt<Btn3Down>: mouse_pan()\n"
      "Ctrl~Meta~Alt Button3<MotionNotify>: mouse_pan()\n"      
      "~Shift ~Ctrl<Btn3Up>: end_drag()\n"
      "<FocusIn>: focusIn()\n"
      "<FocusOut>: focusOut()\n"
      
      // Support for mouse wheel in XFree86 Btn4/Btn5
      "Shift<Btn4Down>,<Btn4Up>: scroll_up(1)\n"
      "Shift<Btn5Down>,<Btn5Up>: scroll_down(1)\n"
      "Ctrl<Btn4Down>,<Btn4Up>: scroll_up(1, pages)\n"
      "Ctrl<Btn5Down>,<Btn5Up>: scroll_down(1, pages)\n"
      "<Btn4Down>,<Btn4Up>: scroll_up(5)\n"
      "<Btn5Down>,<Btn5Up>: scroll_down(5)\n";

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
// TODO:    {"move-to-or-end-drag", moveToOrEndDragAP},
// TODO:    {"move_to_or_end_drag", moveToOrEndDragAP},
   {"end_drag", endDragAP},
   {"copy-to", copyToAP},
   {"copy_to", copyToAP},
   {"copy-to-or-end-drag", copyToOrEndDragAP},
   {"copy_to_or_end_drag", copyToOrEndDragAP},
// TODO:    {"exchange", exchangeAP},
// TODO:    {"process-cancel", processCancelAP},
// TODO:    {"process_cancel", processCancelAP},
// TODO:    {"paste-clipboard", pasteClipboardAP},
// TODO:    {"paste_clipboard", pasteClipboardAP},
// TODO:    {"copy-clipboard", copyClipboardAP},
// TODO:    {"copy_clipboard", copyClipboardAP},
// TODO:    {"cut-clipboard", cutClipboardAP},
// TODO:    {"cut_clipboard", cutClipboardAP},
// TODO:    {"copy-primary", copyPrimaryAP},
// TODO:    {"copy_primary", copyPrimaryAP},
// TODO:    {"cut-primary", cutPrimaryAP},
// TODO:    {"cut_primary", cutPrimaryAP},
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
   {"set-overtype-mode", toggleOverstrikeAP}, // TODO: To remove... present in the menu.cpp file
   {"set_overtype_mode", toggleOverstrikeAP}, // TODO: To remove... present in the menu.cpp file
// TODO:    {"scroll-up", scrollUpAP},
// TODO:    {"scroll_up", scrollUpAP},
// TODO:    {"scroll-down", scrollDownAP},
// TODO:    {"scroll_down", scrollDownAP},
// TODO:    {"scroll_left", scrollLeftAP},
// TODO:    {"scroll_right", scrollRightAP},
   {"scroll-to-line", scrollToLineAP},
   {"scroll_to_line", scrollToLineAP},
   {"select-all", selectAllAP},
   {"select_all", selectAllAP},
   {"deselect-all", deselectAllAP},
   {"deselect_all", deselectAllAP},
// TODO:    {"focusIn", focusInAP},
// TODO:    {"focusOut", focusOutAP},
   {"process-return", selfInsertAP},
   {"process_return", selfInsertAP},
   {"process-tab", processTabAP},
   {"process_tab", processTabAP},
// TODO:    {"insert-string", insertStringAP},
// TODO:    {"insert_string", insertStringAP},
   {"mouse_pan", mousePanAP}
};

/* The motif text widget defined a bunch of actions which the nedit text
   widget as-of-yet does not support:

     Actions which were not bound to keys (for emacs emulation, some of
     them should probably be supported:

	kill-next-character()
	kill-next-word()
	kill-previous-character()
	kill-previous-word()
	kill-selection()
	kill-to-end-of-line()
	kill-to-start-of-line()
	unkill()
	next-line()
	newline-and-backup()
	beep()
	redraw-display()
	scroll-one-line-down()
	scroll-one-line-up()
	set-insertion-point()

    Actions which are not particularly useful:

	set-anchor()
	activate()
	clear-selection() -> this is a wierd one
	do-quick-action() -> don't think this ever worked
	Help()
	next-tab-group()
	select-adjust()
	select-start()
	select-end()
*/

// TODO: static XtResource resources[] =
// TODO: {
// TODO:    { XmNhighlightThickness, XmRDimension, sizeof(Dimension), XtOffset(TextWidget, primitive.highlight_thickness), XmRInt, 0 },
// TODO:    { XmNshadowThickness, XmRDimension, sizeof(Dimension), XtOffset(TextWidget, primitive.shadow_thickness), XmRInt, 0 },
// TODO:    { textNfont, XmRFontStruct, sizeof(XFontStruct *), XtOffset(TextWidget, text.fontStruct), XmRString, "fixed" },
// TODO:    { textNselectForeground, XmRPixel, sizeof(Pixel), XtOffset(TextWidget, text.selectFGPixel), XmRString, NEDIT_DEFAULT_SEL_FG },
// TODO:    { textNselectBackground, XmRPixel, sizeof(Pixel), XtOffset(TextWidget, text.selectBGPixel), XmRString, NEDIT_DEFAULT_SEL_BG },
// TODO:    { textNhighlightForeground, XmRPixel,sizeof(Pixel), XtOffset(TextWidget, text.highlightFGPixel), XmRString, NEDIT_DEFAULT_HI_FG },
// TODO:    { textNhighlightBackground, XmRPixel,sizeof(Pixel), XtOffset(TextWidget, text.highlightBGPixel), XmRString, NEDIT_DEFAULT_HI_BG },
// TODO:    { textNlineNumForeground, XmRPixel,sizeof(Pixel), XtOffset(TextWidget, text.lineNumFGPixel), XmRString, NEDIT_DEFAULT_LINENO_FG },
// TODO:    { textNcursorForeground, XmRPixel,sizeof(Pixel), XtOffset(TextWidget, text.cursorFGPixel), XmRString, NEDIT_DEFAULT_CURSOR_FG },
// TODO:    { textNcalltipForeground, XmRPixel,sizeof(Pixel), XtOffset(TextWidget, text.calltipFGPixel), XmRString, NEDIT_DEFAULT_CALLTIP_FG },
// TODO:    { textNcalltipBackground, XmRPixel,sizeof(Pixel), XtOffset(TextWidget, text.calltipBGPixel), XmRString, NEDIT_DEFAULT_CALLTIP_BG },
// TODO:    { textNbacklightCharTypes,mRString,sizeof(XmString), XtOffset(TextWidget, text.backlightCharTypes), XmRString, NULL },
// TODO:    { textNrows, XmRInt,sizeof(int), XtOffset(TextWidget, text.rows), XmRString, "24" },
// TODO:    { textNcolumns, XmRInt, sizeof(int), XtOffset(TextWidget, text.columns), XmRString, "80" },
// TODO:    { textNmarginWidth, XmRInt, sizeof(int), XtOffset(TextWidget, text.marginWidth), XmRString, "5" },
// TODO:    { textNmarginHeight, XmRInt, sizeof(int), XtOffset(TextWidget, text.marginHeight), XmRString, "5" },
// TODO:    { textNpendingDelete, XmRBoolean, sizeof(bool), XtOffset(TextWidget, text.pendingDelete), XmRString, "True" },
// TODO:    { textNautoWrap, textCAutoWrap, XmRBoolean, sizeof(bool), XtOffset(TextWidget, text.autoWrap), XmRString, "True" },
// TODO:    { textNcontinuousWrap, textCContinuousWrap, XmRBoolean, sizeof(bool), XtOffset(TextWidget, text.continuousWrap), XmRString, "True" },
// TODO:    { textNautoIndent, textCAutoIndent, XmRBoolean, sizeof(bool), XtOffset(TextWidget, text.autoIndent), XmRString, "True" },
// TODO:    { textNsmartIndent, textCSmartIndent, XmRBoolean, sizeof(bool), XtOffset(TextWidget, text.smartIndent), XmRString, "False" },
// TODO:    { textNoverstrike, XmRBoolean, sizeof(bool), XtOffset(TextWidget, text.overstrike), XmRString, "False" },
// TODO:    { textNheavyCursor, XmRBoolean, sizeof(bool), XtOffset(TextWidget, text.heavyCursor), XmRString, "False" },
// TODO:    { textNreadOnly, textCReadOnly, XmRBoolean, sizeof(bool), XtOffset(TextWidget, text.readOnly), XmRString, "False" },
// TODO:    { textNhidePointer, textCHidePointer, XmRBoolean, sizeof(bool), XtOffset(TextWidget, text.hidePointer), XmRString, "False" },
// TODO:    { textNwrapMargin, XmRInt, sizeof(int), XtOffset(TextWidget, text.wrapMargin), XmRString, "0" },
// TODO:    { textNhScrollBar, XmRWidget, sizeof(Fl_Widget*), XtOffset(TextWidget, text.hScrollBar), XmRString, "" },
// TODO:    { textNvScrollBar, XmRWidget, sizeof(Fl_Widget*), XtOffset(TextWidget, text.vScrollBar), XmRString, "" },
// TODO:    { textNlineNumCols, XmRInt, sizeof(int), XtOffset(TextWidget, text.lineNumCols), XmRString, "0" },
// TODO:    { textNautoShowInsertPos, XmRBoolean, sizeof(bool), XtOffset(TextWidget, text.autoShowInsertPos), XmRString, "True" },
// TODO:    { textNautoWrapPastedText, XmRBoolean, sizeof(bool), XtOffset(TextWidget, text.autoWrapPastedText), XmRString, "False" },
// TODO:    { textNwordDelimiters, XmRString, sizeof(char *), XtOffset(TextWidget, text.delimiters), XmRString, ".,/\\`'!@#%^&*()-=+{}[]\":;<>?" },
// TODO:    { textNblinkRate, XmRInt, sizeof(int), XtOffset(TextWidget, text.cursorBlinkRate), XmRString, "500" },
// TODO:    { textNemulateTabs, textCEmulateTabs, XmRInt, sizeof(int), XtOffset(TextWidget, text.emulateTabs), XmRString, "0" },
// TODO:    { textNfocusCallback, textCFocusCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.focusInCB), XtRCallback, NULL },
// TODO:    { textNlosingFocusCallback, textCLosingFocusCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.focusOutCB), XtRCallback,NULL },
// TODO:    { textNcursorMovementCallback, textCCursorMovementCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.cursorCB), XtRCallback, NULL },
// TODO:    { textNdragStartCallback, textCDragStartCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.dragStartCB), XtRCallback, NULL },
// TODO:    { textNdragEndCallback, textCDragEndCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.dragEndCB), XtRCallback, NULL },
// TODO:    { textNsmartIndentCallback, textCSmartIndentCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.smartIndentCB), XtRCallback, NULL },
// TODO:    { textNcursorVPadding, textCCursorVPadding, XtRCardinal, sizeof(int), XtOffset(TextWidget, text.cursorVPadding), XmRString, "0" }
// TODO: };
// TODO: 
// TODO: static TextClassRec textClassRec =
// TODO: {
// TODO:    /* CoreClassPart */
// TODO:    {
// TODO:       (WidgetClass) &xmPrimitiveClassRec,  /* superclass       */
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
// TODO: WidgetClass textWidgetClass = (WidgetClass)&textClassRec;
// TODO: #define NEDIT_HIDE_CURSOR_MASK (KeyPressMask)
// TODO: #define NEDIT_SHOW_CURSOR_MASK (FocusChangeMask | PointerMotionMask | ButtonMotionMask | ButtonPressMask | ButtonReleaseMask)
// TODO: static char empty_bits[] = {0x00, 0x00, 0x00, 0x00};
// TODO: static Cursor empty_cursor = 0;


// --------------------------------------------------------------------------
// Install actions for use in translation tables and macro recording, relating to menu item commands
// --------------------------------------------------------------------------
Ne_AppContext TextAppContext(""); // Should be in the AppContext from nedit.h to allow repeat/macro

// --------------------------------------------------------------------------
static bool InstallTextActions(Ne_AppContext& context)
{
   NeAppAddActions(context, actionsList, ARRAY_SIZE(actionsList));
   return true;
}

static bool initTextActions = InstallTextActions(TextAppContext);

// --------------------------------------------------------------------------
// Return the (statically allocated) action table for menu item actions.
// --------------------------------------------------------------------------
XtActionsRec* GetTextActions(int* nActions)
{
   *nActions = ARRAY_SIZE(actionsList);
   return actionsList;
}

// --------------------------------------------------------------------------
Ne_Text_Editor* Ne_Text_Editor::Create(int x, int y, int w, int h,
   int lineNumWidth, Ne_Text_Buffer* buffer,
   const Ne_Font& font,
   Fl_Color bgPixel, Fl_Color fgPixel,
   Fl_Color selectFGPixel, Fl_Color selectBGPixel, Fl_Color highlightFGPixel,
   Fl_Color highlightBGPixel, Fl_Color cursorFGPixel, Fl_Color lineNumFGPixel,
   bool continuousWrap, int wrapMargin,
   Fl_Color calltipFGPixel, Fl_Color calltipBGPixel)
{
   Ne_Text_Editor* textD = new Ne_Text_Editor(x, y, w, h);
   textD->color(bgPixel, selectBGPixel);

   textD->buffer = buffer;
   textD->fontStruct = font;
   fl_font(textD->fontStruct.font, textD->fontStruct.size);
   textD->descent = fl_descent();
   textD->ascent = fl_height(textD->fontStruct.font, textD->fontStruct.size) - textD->descent;
   int fontWidth = int(fl_width("MW")/2);
   textD->fixedFontWidth = (fl_width("WWMM") == fl_width("i,[l") ? fontWidth : -1);

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
   textD->lineStarts = new int[textD->nVisibleLines];
   textD->lineStarts[0] = 0;
   for (int i = 1; i < textD->nVisibleLines; i++)
      textD->lineStarts[i] = -1;
   // TODO:     textD->calltipW = NULL;
   // TODO:     textD->calltipShell = NULL;
   // TODO:     textD->calltip.ID = 0;
   // TODO:     textD->calltipFGPixel = calltipFGPixel;
   // TODO:     textD->calltipBGPixel = calltipBGPixel;
   // TODO:     textD->bgClassPixel = NULL;
   // TODO:     textD->bgClass = NULL;
   // TODO:     TextDSetupBGClasses(widget, bgClassString, &textD->bgClassPixel, &textD->bgClass, bgPixel);
   // TODO:     textD->suppressResync = 0;
   // TODO:     textD->nLinesDeleted = 0;
   // TODO:     textD->modifyingTabDist = 0;
   // TODO:     textD->pointerHidden = false;

   // Attach the callback to the text buffer for receiving modification information
   if (buffer != NULL)
   {
      buffer->addModifyCB(BufferModifiedCB, textD);
      buffer->addPreDeleteCB(BufferPreDeleteCB, textD);
   }

   // Update the display to reflect the contents of the buffer
   if (buffer != NULL)
      BufferModifiedCB(0, buffer->size(), 0, 0, NULL, textD);

   // Decide if the horizontal scroll bar needs to be visible
   textD->hideOrShowHScrollBar();

   return textD;
}

// --------------------------------------------------------------------------
// For testing only... the Ne_Text_Editor will have the handle
// --------------------------------------------------------------------------
int Ne_Text_Editor::handle(int e)
{
   printf("Ne_Text_Editor::handle [%d][%s]\n", e, fl_eventnames[e]);

   switch (e)
   {
   case FL_FOCUS:
      //show_cursor(mCursorOn); // redraws the cursor
      //if (buffer()->selected()) redraw(); // Redraw selections...
      //Fl::focus(this);
      return 1;

   case FL_UNFOCUS:
      //show_cursor(mCursorOn); // redraws the cursor
      //if (buffer()->selected()) redraw(); // Redraw selections...
   case FL_HIDE:
      //if (when() & FL_WHEN_RELEASE) maybe_do_callback();
      return 1;

   case FL_PUSH:
      if (inWindow(Fl::event_x(), Fl::event_y()))
         return handle_key_button(e);
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
      //if (!Fl::event_text()) {
      //  fl_beep();
      return 1;
      //   //}

      //buffer()->remove_selection();
      //if (insert_mode()) insert(Fl::event_text());
      //else overstrike(Fl::event_text());
      //show_insert_position();
      //set_changed();
      //if (when()&FL_WHEN_CHANGED) do_callback();
      return 1;

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

   printf("handle_key [%d] [Key:%d] [State:%d %d %d %d] [Button:%d]\n",
         del,
         currentState.key,
         currentState.shift, currentState.ctrl, currentState.alt, currentState.meta,
         currentState.button);

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
            for(int i = 0; i < args.size(); ++i)
               params[i] = args[i].c_str();

            // We must translate the string to a function name with args....
            TextAppContext.callAction(this, parsedCommand, event, params, args.size());
            
            delete[] parsedCommand;
            delete[] params;
         break;
      }
   }
   redraw();

   return 1;
}

// TODO: /*
// TODO: ** Fl_Widget* initialize method
// TODO: */
// TODO: static void initialize(TextWidget request, TextWidget new)
// TODO: {
// TODO:    XFontStruct *fs = new->text.fontStruct;
// TODO:    char *delimiters;
// TODO:    Ne_Text_Buffer *buf;
// TODO:    Pixel white, black;
// TODO:    int textLeft;
// TODO:    int charWidth = fs->max_bounds.width;
// TODO:    int marginWidth = new->text.marginWidth;
// TODO:    int lineNumCols = new->text.lineNumCols;
// TODO: 
// TODO:    /* Set the initial window size based on the rows and columns resources */
// TODO:    if (request->core.width == 0)
// TODO:       new->core.width = charWidth * new->text.columns + marginWidth*2 +
// TODO:                         (lineNumCols == 0 ? 0 : marginWidth + charWidth * lineNumCols);
// TODO:    if (request->core.height == 0)
// TODO:       new->core.height = (fs->ascent + fs->descent) * new->text.rows +
// TODO:                          new->text.marginHeight * 2;
// TODO: 
// TODO:    /* The default colors work for B&W as well as color, except for
// TODO:       selectFGPixel and selectBGPixel, where color highlighting looks
// TODO:       much better without reverse video, so if we get here, and the
// TODO:       selection is totally unreadable because of the bad default colors,
// TODO:       swap the colors and make the selection reverse video */
// TODO:    white = WhitePixelOfScreen(XtScreen((Fl_Widget*)new));
// TODO:    black = BlackPixelOfScreen(XtScreen((Fl_Widget*)new));
// TODO:    if (    new->text.selectBGPixel == white &&
// TODO:            new->core.background_pixel == white &&
// TODO:            new->text.selectFGPixel == black &&
// TODO:            new->primitive.foreground == black)
// TODO:    {
// TODO:       new->text.selectBGPixel = black;
// TODO:       new->text.selectFGPixel = white;
// TODO:    }
// TODO: 
// TODO:    /* Create the initial text buffer for the widget to display (which can
// TODO:       be replaced later with TextSetBuffer) */
// TODO:    buf = BufCreate();
// TODO: 
// TODO:    /* Create and initialize the text-display part of the widget */
// TODO:    textLeft = new->text.marginWidth +
// TODO:    (lineNumCols == 0 ? 0 : marginWidth + charWidth * lineNumCols);
// TODO:    new->text.textD = TextDCreate((Fl_Widget*)new, new->text.hScrollBar,
// TODO:                                  new->text.vScrollBar, textLeft, new->text.marginHeight,
// TODO:                                  new->core.width - marginWidth - textLeft,
// TODO:                                  new->core.height - new->text.marginHeight * 2,
// TODO:                                  lineNumCols == 0 ? 0 : marginWidth,
// TODO:                                  lineNumCols == 0 ? 0 : lineNumCols * charWidth,
// TODO:                                  buf, new->text.fontStruct, new->core.background_pixel,
// TODO:                                  new->primitive.foreground, new->text.selectFGPixel,
// TODO:                                  new->text.selectBGPixel, new->text.highlightFGPixel,
// TODO:                                  new->text.highlightBGPixel, new->text.cursorFGPixel,
// TODO:                                  new->text.lineNumFGPixel,
// TODO:                                  new->text.continuousWrap, new->text.wrapMargin,
// TODO:                                  new->text.backlightCharTypes, new->text.calltipFGPixel,
// TODO:                                  new->text.calltipBGPixel);
// TODO: 
// TODO:    /* Add mandatory delimiters blank, tab, and newline to the list of
// TODO:       delimiters.  The memory use scheme here is that new values are
// TODO:       always copied, and can therefore be safely freed on subsequent
// TODO:       set-values calls or destroy */
// TODO:    delimiters = XtMalloc(strlen(new->text.delimiters) + 4);
// TODO:    sprintf(delimiters, "%s%s", " \t\n", new->text.delimiters);
// TODO:    new->text.delimiters = delimiters;
// TODO: 
// TODO:    /* Start with the cursor blanked (widgets don't have focus on creation,
// TODO:       the initial FocusIn event will unblank it and get blinking started) */
// TODO:    new->text.textD->cursorOn = False;
// TODO: 
// TODO:    /* Initialize the widget variables */
// TODO:    new->text.autoScrollProcID = 0;
// TODO:    new->text.cursorBlinkProcID = 0;
// TODO:    new->text.dragState = NOT_CLICKED;
// TODO:    new->text.multiClickState = NO_CLICKS;
// TODO:    new->text.lastBtnDown = 0;
// TODO:    new->text.selectionOwner = False;
// TODO:    new->text.motifDestOwner = False;
// TODO:    new->text.emTabsBeforeCursor = 0;
// TODO: 
// TODO: #ifndef NO_XMIM
// TODO:    /* Register the widget to the input manager */
// TODO:    XmImRegister((Fl_Widget*)new, 0);
// TODO:    /* In case some Resources for the IC need to be set, add them below */
// TODO:    XmImVaSetValues((Fl_Widget*)new, NULL);
// TODO: #endif
// TODO: 
// TODO:    XtAddEventHandler((Fl_Widget*)new, GraphicsExpose, True, (XtEventHandler)redisplayGE, (Opaque)NULL);
// TODO: 
// TODO:    if (new->text.hidePointer)
// TODO:    {
// TODO:       Display *theDisplay;
// TODO:       Pixmap empty_pixmap;
// TODO:       XColor black_color;
// TODO:       /* Set up the empty Cursor */
// TODO:       if (empty_cursor == 0)
// TODO:       {
// TODO:          theDisplay = XtDisplay((Fl_Widget*)new);
// TODO:          empty_pixmap = XCreateBitmapFromData(theDisplay, RootWindowOfScreen(XtScreen((Fl_Widget*)new)), empty_bits, 1, 1);
// TODO:          XParseColor(theDisplay, DefaultColormapOfScreen(XtScreen((Fl_Widget*)new)),
// TODO:                      "black", &black_color);
// TODO:          empty_cursor = XCreatePixmapCursor(theDisplay, empty_pixmap, empty_pixmap, &black_color, &black_color, 0, 0);
// TODO:       }
// TODO: 
// TODO:       /* Add event handler to hide the pointer on keypresses */
// TODO:       XtAddEventHandler((Fl_Widget*)new, NEDIT_HIDE_CURSOR_MASK, False, handleHidePointer, (Opaque)NULL);
// TODO:    }
// TODO: }
// TODO: 
// TODO: /* Hide the pointer while the user is typing */
// TODO: static void handleHidePointer(Fl_Widget* w, XtPointer unused, int event, bool *continue_to_dispatch)
// TODO: {
// TODO:    TextWidget tw = (TextWidget) w;
// TODO:    ShowHidePointer(tw, True);
// TODO: }
// TODO: 
// TODO: /* Restore the pointer if the mouse moves or focus changes */
// TODO: static void handleShowPointer(Fl_Widget* w, XtPointer unused, int event, bool *continue_to_dispatch)
// TODO: {
// TODO:    TextWidget tw = (TextWidget) w;
// TODO:    ShowHidePointer(tw, False);
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
// TODO:             XtRemoveEventHandler((Fl_Widget*)w, NEDIT_HIDE_CURSOR_MASK, False, handleHidePointer, (Opaque)NULL);
// TODO:             /* Switch to empty cursor */
// TODO:             XDefineCursor(XtDisplay(w), XtWindow(w), empty_cursor);
// TODO: 
// TODO:             w->text.textD->pointerHidden = True;
// TODO: 
// TODO:             /* Listen to mouse movement, focus change, and button presses */
// TODO:             XtAddEventHandler((Fl_Widget*)w, NEDIT_SHOW_CURSOR_MASK, False, handleShowPointer, (Opaque)NULL);
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             /* Don't listen to mouse/focus events any more */
// TODO:             XtRemoveEventHandler((Fl_Widget*)w, NEDIT_SHOW_CURSOR_MASK, False, handleShowPointer, (Opaque)NULL);
// TODO:             /* Switch to regular cursor */
// TODO:             XUndefineCursor(XtDisplay(w), XtWindow(w));
// TODO: 
// TODO:             w->text.textD->pointerHidden = False;
// TODO: 
// TODO:             /* Listen for keypresses now */
// TODO:             XtAddEventHandler((Fl_Widget*)w, NEDIT_HIDE_CURSOR_MASK, False, handleHidePointer, (Opaque)NULL);
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
// TODO:    Ne_Text_Buffer *buf;
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
// TODO:    XtFree(w->text.delimiters);
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
// TODO:    XFontStruct *fs = w->text.fontStruct;
// TODO:    int height = w->core.height, width = w->core.width;
// TODO:    int marginWidth = w->text.marginWidth, marginHeight = w->text.marginHeight;
// TODO:    int lineNumAreaWidth = w->text.lineNumCols == 0 ? 0 : w->text.marginWidth +
// TODO:                           fs->max_bounds.width * w->text.lineNumCols;
// TODO: 
// TODO:    w->text.columns = (width - marginWidth*2 - lineNumAreaWidth) / fs->max_bounds.width;
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
// TODO:       w->core.width = width = fs->max_bounds.width + marginWidth*2 + lineNumAreaWidth;
// TODO:    }
// TODO:    if (w->text.rows < 1)
// TODO:    {
// TODO:       w->text.rows = 1;
// TODO:       w->core.height = height = fs->ascent + fs->descent + marginHeight*2;
// TODO:    }
// TODO: 
// TODO:    /* Resize the text display that the widget uses to render text */
// TODO:    TextDResize(w->text.textD, width - marginWidth*2 - lineNumAreaWidth, height - marginHeight*2);
// TODO: 
// TODO:    /* if the window became shorter or narrower, there may be text left
// TODO:       in the bottom or right margin area, which must be cleaned up */
// TODO:    if (XtIsRealized((Fl_Widget*)w))
// TODO:    {
// TODO:       XClearArea(XtDisplay(w), XtWindow(w), 0, height-marginHeight, width, marginHeight, False);
// TODO:       XClearArea(XtDisplay(w), XtWindow(w),width-marginWidth, 0, marginWidth, height, False);
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Fl_Widget* redisplay method
// TODO: */
// TODO: static void redisplay(TextWidget w, int event, Region region)
// TODO: {
// TODO:    XExposeEvent *e = &event->xexpose;
// TODO: 
// TODO:    TextDRedisplayRect(w->text.textD, e->x, e->y, e->width, e->height);
// TODO: }
// TODO: 
// TODO: static Bool findGraphicsExposeOrNoExposeEvent(Display *theDisplay, int event, XPointer arg)
// TODO: {
// TODO:    if ((theDisplay == event->xany.display) &&
// TODO:          (event->type == GraphicsExpose || event->type == NoExpose) &&
// TODO:          ((Fl_Widget*)arg == XtWindowToWidget(event->xany.display, event->xany.window)))
// TODO:    {
// TODO:       return(True);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       return(False);
// TODO:    }
// TODO: }
// TODO: 
// TODO: static void adjustRectForGraphicsExposeOrNoExposeEvent(TextWidget w, int event,
// TODO:       bool *first, int *left, int *top, int *width, int *height)
// TODO: {
// TODO:    bool removeQueueEntry = False;
// TODO: 
// TODO:    if (event->type == GraphicsExpose)
// TODO:    {
// TODO:       XGraphicsExposeEvent *e = &event->xgraphicsexpose;
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
// TODO:          *first = False;
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          int prev_left = *left;
// TODO:          int prev_top = *top;
// TODO: 
// TODO:          *left = std::min(*left, x);
// TODO:          *top = std::min(*top, y);
// TODO:          *width = std::max(prev_left + *width, x + e->width) - *left;
// TODO:          *height = std::max(prev_top + *height, y + e->height) - *top;
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

// TODO: /*
// TODO: ** Fl_Widget* setValues method
// TODO: */
// TODO: static bool setValues(TextWidget current, TextWidget request, TextWidget new)
// TODO: {
// TODO:    bool redraw = False, reconfigure = False;
// TODO: 
// TODO:    if (new->text.overstrike != current->text.overstrike)
// TODO:    {
// TODO:       if (current->text.textD->cursorStyle == BLOCK_CURSOR)
// TODO:          TextDSetCursorStyle(current->text.textD,
// TODO:                              current->text.heavyCursor ? HEAVY_CURSOR : NORMAL_CURSOR);
// TODO:       else if (current->text.textD->cursorStyle == NORMAL_CURSOR ||
// TODO:                current->text.textD->cursorStyle == HEAVY_CURSOR)
// TODO:          TextDSetCursorStyle(current->text.textD, BLOCK_CURSOR);
// TODO:    }
// TODO: 
// TODO:    if (new->text.fontStruct != current->text.fontStruct)
// TODO:    {
// TODO:       if (new->text.lineNumCols != 0)
// TODO:          reconfigure = True;
// TODO:       TextDSetFont(current->text.textD, new->text.fontStruct);
// TODO:    }
// TODO: 
// TODO:    if (new->text.wrapMargin != current->text.wrapMargin ||
// TODO:          new->text.continuousWrap != current->text.continuousWrap)
// TODO:       TextDSetWrapMode(current->text.textD, new->text.continuousWrap,
// TODO:                        new->text.wrapMargin);
// TODO: 
// TODO:    /* When delimiters are changed, copy the memory, so that the caller
// TODO:       doesn't have to manage it, and add mandatory delimiters blank,
// TODO:       tab, and newline to the list */
// TODO:    if (new->text.delimiters != current->text.delimiters)
// TODO:    {
// TODO:       char *delimiters = XtMalloc(strlen(new->text.delimiters) + 4);
// TODO:       XtFree(current->text.delimiters);
// TODO:       sprintf(delimiters, "%s%s", " \t\n", new->text.delimiters);
// TODO:       new->text.delimiters = delimiters;
// TODO:    }
// TODO: 
// TODO:    /* Setting the lineNumCols resource tells the text widget to hide or
// TODO:       show, or change the number of columns of the line number display,
// TODO:       which requires re-organizing the x coordinates of both the line
// TODO:       number display and the main text display */
// TODO:    if (new->text.lineNumCols != current->text.lineNumCols || reconfigure)
// TODO:    {
// TODO:       int marginWidth = new->text.marginWidth;
// TODO:       int charWidth = new->text.fontStruct->max_bounds.width;
// TODO:       int lineNumCols = new->text.lineNumCols;
// TODO:       if (lineNumCols == 0)
// TODO:       {
// TODO:          TextDSetLineNumberArea(new->text.textD, 0, 0, marginWidth);
// TODO:          new->text.columns = (new->core.width - marginWidth*2) / charWidth;
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          TextDSetLineNumberArea(new->text.textD, marginWidth,
// TODO:                                 charWidth * lineNumCols,
// TODO:                                 2*marginWidth + charWidth * lineNumCols);
// TODO:          new->text.columns = (new->core.width - marginWidth*3 - charWidth
// TODO:                               * lineNumCols) / charWidth;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    if (new->text.backlightCharTypes != current->text.backlightCharTypes)
// TODO:    {
// TODO:       TextDSetupBGClasses((Fl_Widget*)new, new->text.backlightCharTypes,
// TODO:                           &new->text.textD->bgClassPixel, &new->text.textD->bgClass,
// TODO:                           new->text.textD->bgPixel);
// TODO:       redraw = True;
// TODO:    }
// TODO: 
// TODO:    return redraw;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Fl_Widget* realize method
// TODO: */
// TODO: static void realize(Fl_Widget* w, XtValueMask *valueMask, XSetWindowAttributes *attributes)
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
// TODO: static XtGeometryResult queryGeometry(Fl_Widget* w, XtWidgetGeometry *proposed, XtWidgetGeometry *answer)
// TODO: {
// TODO:    TextWidget tw = (TextWidget)w;
// TODO: 
// TODO:    int curHeight = tw->core.height;
// TODO:    int curWidth = tw->core.width;
// TODO:    XFontStruct *fs = tw->text.textD->fontStruct;
// TODO:    int fontWidth = fs->max_bounds.width;
// TODO:    int fontHeight = fs->ascent + fs->descent;
// TODO:    int marginHeight = tw->text.marginHeight;
// TODO:    int propWidth = (proposed->request_mode & CWWidth) ? proposed->width : 0;
// TODO:    int propHeight = (proposed->request_mode & CWHeight) ? proposed->height : 0;
// TODO: 
// TODO:    answer->request_mode = CWHeight | CWWidth;
// TODO: 
// TODO:    if(proposed->request_mode & CWWidth)
// TODO:       /* Accept a width no smaller than 10 chars */
// TODO:       answer->width = std::max(fontWidth * 10, proposed->width);
// TODO:    else
// TODO:       answer->width = curWidth;
// TODO: 
// TODO:    if(proposed->request_mode & CWHeight)
// TODO:       /* Accept a height no smaller than an exact multiple of the line height
// TODO:          and at least one line high */
// TODO:       answer->height = std::max(1, ((propHeight - 2*marginHeight) / fontHeight)) *
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
** Return the cursor position
*/
int Ne_Text_Editor::getCursorPos()
{
   return getInsertPosition();
}

/*
** Set the cursor position
*/
void Ne_Text_Editor::setCursorPos(int pos)
{
   setInsertPosition(pos);
   checkAutoShowInsertPos();
   callCursorMovementCBs(0);

}

// TODO: void TextPasteClipboard(Fl_Widget* w, Time time)
// TODO: {
// TODO:    cancelDrag(w);
// TODO:    if (checkReadOnly(w))
// TODO:       return;
// TODO:    TakeMotifDestination(w, time);
// TODO:    InsertClipboard(w, False);
// TODO:    callCursorMovementCBs(w, NULL);
// TODO: }
// TODO: 
// TODO: void TextColPasteClipboard(Fl_Widget* w, Time time)
// TODO: {
// TODO:    cancelDrag(w);
// TODO:    if (checkReadOnly(w))
// TODO:       return;
// TODO:    TakeMotifDestination(w, time);
// TODO:    InsertClipboard(w, True);
// TODO:    callCursorMovementCBs(w, NULL);
// TODO: }
// TODO: 
// TODO: void TextCopyClipboard(Fl_Widget* w, Time time)
// TODO: {
// TODO:    cancelDrag(w);
// TODO:    if (!((TextWidget)w)->text.textD->buffer->primary.selected)
// TODO:    {
// TODO:       XBell(XtDisplay(w), 0);
// TODO:       return;
// TODO:    }
// TODO:    CopyToClipboard(w, time);
// TODO: }
// TODO: 
// TODO: void TextCutClipboard(Fl_Widget* w, Time time)
// TODO: {
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
// TODO: 
// TODO:    cancelDrag(w);
// TODO:    if (checkReadOnly(w))
// TODO:       return;
// TODO:    if (!textD->buffer->primary.selected)
// TODO:    {
// TODO:       XBell(XtDisplay(w), 0);
// TODO:       return;
// TODO:    }
// TODO:    TakeMotifDestination(w, time);
// TODO:    CopyToClipboard (w, time);
// TODO:    BufRemoveSelected(textD->buffer);
// TODO:    TextDSetInsertPosition(textD, textD->buffer->cursorPosHint);
// TODO:    checkAutoShowInsertPos(w);
// TODO: }

int Ne_Text_Editor::firstVisibleLine()
{
   return this->topLineNum;
}

int Ne_Text_Editor::numVisibleLines()
{
   return this->nVisibleLines;
}

int Ne_Text_Editor::visibleWidth()
{
   return this->width;
}

int Ne_Text_Editor::firstVisiblePos()
{
   return this->firstChar;
}

int Ne_Text_Editor::lastVisiblePos()
{
   return this->lastChar;
}

/*
** Insert text "chars" at the cursor position, respecting pending delete
** selections, overstrike, and handling cursor repositioning as if the text
** had been typed.  If autoWrap is on wraps the text to fit within the wrap
** margin, auto-indenting where the line was wrapped (but nowhere else).
** "allowPendingDelete" controls whether primary selections in the widget are
** treated as pending delete selections (True), or ignored (False). "event"
** is optional and is just passed on to the cursor movement callbacks.
*/
void Ne_Text_Editor::insertAtCursor(const char *chars, int event, int allowPendingDelete, int allowWrap)
{
   int wrapMargin, colNum, lineStartPos, cursorPos;
   const char* c;
   Ne_Text_Buffer* buf = this->buffer;
   fl_font(this->fontStruct.font, this->fontStruct.size);
   int fontWidth = (int)(fl_width("MW")/2);
   int replaceSel, singleLine, breakAt = 0;

   // Don't wrap if auto-wrap is off or suppressed, or it's just a newline
   if (!allowWrap || !this->autoWrap || (chars[0] == '\n' && chars[1] == '\0'))
   {
      simpleInsertAtCursor(chars, event, allowPendingDelete);
      return;
   }

   /* If this is going to be a pending delete operation, the real insert
      position is the start of the selection.  This will make rectangular
      selections wrap strangely, but this routine should rarely be used for
      them, and even more rarely when they need to be wrapped. */
   replaceSel = allowPendingDelete && pendingSelection();
   cursorPos = replaceSel ? buf->getSelection().start : getInsertPosition();

   /* If the text is only one line and doesn't need to be wrapped, just insert
      it and be done (for efficiency only, this routine is called for each
      character typed). (Of course, it may not be significantly more efficient
      than the more general code below it, so it may be a waste of time!) */
   wrapMargin = this->wrapMargin != 0 ? this->wrapMargin : this->width / fontWidth;
   lineStartPos = buf->startOfLine(cursorPos);
   colNum = buf->countDispChars(lineStartPos, cursorPos);
   for (c=chars; *c!='\0' && *c!='\n'; c++)
      colNum += buf->charWidth(*c, colNum, buf->getTabDistance(), buf->getNullSubsChar());
   singleLine = *c == '\0';
   if (colNum < wrapMargin && singleLine)
   {
      simpleInsertAtCursor(chars, event, true);
      return;
   }

   /* Wrap the text */
   std::string lineStartText = buf->getRange(lineStartPos, cursorPos);
   std::string wrappedText = wrapText(lineStartText.c_str(), chars, lineStartPos, wrapMargin, replaceSel ? NULL : &breakAt);

   /* Insert the text.  Where possible, use TextDInsert which is optimized
      for less redraw. */
   if (replaceSel)
   {
      buf->replaceSelected(wrappedText.c_str());
      setInsertPosition(buf->getCursorPosHint());
   }
   else if (this->isOverstrike)
   {
      if (breakAt == 0 && singleLine)
         this->overstrike(wrappedText.c_str());
      else
      {
         buf->replace(cursorPos-breakAt, cursorPos, wrappedText.c_str());
         this->setInsertPosition(buf->getCursorPosHint());
      }
   }
   else
   {
      if (breakAt == 0)
      {
         this->insert(wrappedText.c_str());
      }
      else
      {
         buf->replace(cursorPos-breakAt, cursorPos, wrappedText.c_str());
         this->setInsertPosition(buf->getCursorPosHint());
      }
   }
   checkAutoShowInsertPos();
   callCursorMovementCBs(event);
}

// TODO: /*
// TODO: ** Fetch text from the widget's buffer, adding wrapping newlines to emulate
// TODO: ** effect acheived by wrapping in the text display in continuous wrap mode.
// TODO: */
// TODO: char *TextGetWrapped(Fl_Widget* w, int startPos, int endPos, int *outLen)
// TODO: {
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
// TODO:    Ne_Text_Buffer *buf = textD->buffer;
// TODO:    Ne_Text_Buffer *outBuf;
// TODO:    int fromPos, toPos, outPos;
// TODO:    char c, *outString;
// TODO: 
// TODO:    if (!((TextWidget)w)->text.continuousWrap || startPos == endPos)
// TODO:    {
// TODO:       *outLen = endPos - startPos;
// TODO:       return BufGetRange(buf, startPos, endPos);
// TODO:    }
// TODO: 
// TODO:    /* Create a text buffer with a good estimate of the size that adding
// TODO:       newlines will expand it to.  Since it's a text buffer, if we guess
// TODO:       wrong, it will fail softly, and simply expand the size */
// TODO:    outBuf = BufCreatePreallocated((endPos-startPos) + (endPos-startPos)/5);
// TODO:    outPos = 0;
// TODO: 
// TODO:    /* Go (displayed) line by line through the buffer, adding newlines where
// TODO:       the text is wrapped at some character other than an existing newline */
// TODO:    fromPos = startPos;
// TODO:    toPos = TextDCountForwardNLines(textD, startPos, 1, False);
// TODO:    while (toPos < endPos)
// TODO:    {
// TODO:       BufCopyFromBuf(buf, outBuf, fromPos, toPos, outPos);
// TODO:       outPos += toPos - fromPos;
// TODO:       c = BufGetCharacter(outBuf, outPos-1);
// TODO:       if (c == ' ' || c == '\t')
// TODO:          BufReplace(outBuf, outPos-1, outPos, "\n");
// TODO:       else if (c != '\n')
// TODO:       {
// TODO:          BufInsert(outBuf, outPos, "\n");
// TODO:          outPos++;
// TODO:       }
// TODO:       fromPos = toPos;
// TODO:       toPos = TextDCountForwardNLines(textD, fromPos, 1, True);
// TODO:    }
// TODO:    BufCopyFromBuf(buf, outBuf, fromPos, endPos, outPos);
// TODO: 
// TODO:    /* return the contents of the output buffer as a string */
// TODO:    outString = BufGetAll(outBuf);
// TODO:    *outLen = outBuf->length;
// TODO:    BufFree(outBuf);
// TODO:    return outString;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Return the (statically allocated) action table for menu item actions.
// TODO: **
// TODO: ** Warning: This routine can only be used before the first text widget is
// TODO: ** created!  After that, apparently, Xt takes over the table and overwrites
// TODO: ** it with its own version.  XtGetActionList is preferable, but is not
// TODO: ** available before X11R5.
// TODO: */
// TODO: XtActionsRec *TextGetActions(int *nActions)
// TODO: {
// TODO:    *nActions = XtNumber(actionsList);
// TODO:    return actionsList;
// TODO: }

static void grabFocusAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   double lastBtnDown = textD->lastBtnDown;
   int row, column;

   // Indicate state for future events, PRIMARY_CLICKED indicates that
   // the proper initialization has been done for primary dragging and/or
   // multi-clicking.  Also record the timestamp for multi-click processing
   textD->dragState = NE_PRIMARY_CLICKED;
   textD->lastBtnDown = GetTimeOfDay();

   // Become owner of the MOTIF_DESTINATION selection, making this widget
   // the designated recipient of secondary quick actions in Motif XmText
   // widgets and in other NEdit text widgets
// TODO:    TakeMotifDestination(w, e->time);

   // Check for possible multi-click sequence in progress
   if (textD->multiClickState != NE_NO_CLICKS)
   {
      if ( !textD->mouseMoveForDrag(Fl::event_x(), Fl::event_y())
         &&  GetTimeOfDay() < lastBtnDown + textD->MultiClickTime)
      {
         if (textD->multiClickState == NE_ONE_CLICK)
         {
            textD->selectWord(Fl::event_x());
            textD->callCursorMovementCBs(event);
            return;
         }
         else if (textD->multiClickState == NE_TWO_CLICKS)
         {
            textD->selectLine();
            textD->callCursorMovementCBs(event);
            return;
         }
         else if (textD->multiClickState == NE_THREE_CLICKS)
         {
            textD->buffer->select(0, textD->buffer->size());
            return;
         }
         else if (textD->multiClickState > NE_THREE_CLICKS)
            textD->multiClickState = NE_NO_CLICKS;
      }
      else
         textD->multiClickState = NE_NO_CLICKS;
   }

   // Clear any existing selections
   textD->buffer->unselect();

   // Move the cursor to the pointer location
   moveDestinationAP(w, event, args, nArgs);

   // Record the site of the initial button press and the initial character
   // position so subsequent motion events and clicking can decide when and
   // where to begin a primary selection
   textD->btnDownX = Fl::event_x();
   textD->btnDownY = Fl::event_y();
   textD->anchor = textD->getInsertPosition();
   textD->XYToUnconstrainedPosition(Fl::event_x(), Fl::event_y(), &row, &column);
   column = textD->OffsetWrappedColumn(row, column);
}

static void moveDestinationAP(Fl_Widget* w, int event, const char**args, int *nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   // Move the cursor
   textD->setInsertPosition(textD->XYToPosition(Fl::event_x(), Fl::event_y()));
   textD->checkAutoShowInsertPos();
   textD->callCursorMovementCBs(event);
}

static void extendAdjustAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int dragState = textD->dragState;

   // Make sure the proper initialization was done on mouse down 
   if (dragState != NE_PRIMARY_DRAG && dragState != NE_PRIMARY_CLICKED)
      return;

   // If the selection hasn't begun, decide whether the mouse has moved
   // far enough from the initial mouse down to be considered a drag
   if (textD->dragState == NE_PRIMARY_CLICKED)
   {
      if (abs(Fl::event_x() - textD->btnDownX) > NE_SELECT_THRESHOLD || abs(Fl::event_y() - textD->btnDownY) > NE_SELECT_THRESHOLD)
         textD->dragState = NE_PRIMARY_DRAG;
      else
         return;
   }

   // If "rect" argument has appeared or disappeared, keep dragState up
   // to date about which type of drag this is
   textD->dragState = NE_PRIMARY_DRAG;

   // Record the new position for the autoscrolling timer routine, and
   // engage or disengage the timer if the mouse is in/out of the window
   textD->checkAutoScroll(Fl::event_x(),Fl::event_y());

   // Adjust the selection and move the cursor
   textD->adjustSelection(Fl::event_x(),Fl::event_y());
}

void extendStartAP(Fl_Widget* w, int event, const char**args, int *nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer *buf = textD->buffer;
   Ne_Text_Selection* sel = &buf->getSelection();
   int anchor, anchorLineStart, newPos, row, column;

   // Find the new anchor point for the rest of this drag operation
   newPos = textD->XYToPosition(Fl::event_x(), Fl::event_y());
   textD->XYToUnconstrainedPosition(Fl::event_x(), Fl::event_y(), &row, &column);
   column = textD->OffsetWrappedColumn(row, column);
   if (sel->selected)
   {
      if (abs(newPos - sel->start) < abs(newPos - sel->end))
         anchor = sel->end;
      else
         anchor = sel->start;
      anchorLineStart = buf->startOfLine(anchor);
   }
   else
   {
      anchor = textD->getInsertPosition();
      anchorLineStart = buf->startOfLine(anchor);
   }
   textD->anchor = anchor;

   // Make the new selection 
   buf->select(std::min(anchor, newPos), std::max(anchor, newPos));

   // Never mind the motion threshold, go right to dragging since
   // extend-start is unambiguously the start of a selection
   textD->dragState = NE_PRIMARY_DRAG;

   // Don't do by-word or by-line adjustment, just by character
   textD->multiClickState = NE_NO_CLICKS;

   // Move the cursor
   textD->setInsertPosition(newPos);
   textD->callCursorMovementCBs(event);
}

static void extendEndAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   if (textD->dragState == NE_PRIMARY_CLICKED && textD->lastBtnDown <= GetTimeOfDay() + textD->MultiClickTime)
      textD->multiClickState++;
   textD->endDrag();
}

// TODO: static void processCancelAP(Fl_Widget* w, int event, const char**args, int*nArgs)
// TODO: {
// TODO:    int dragState = ((TextWidget)w)->text.dragState;
// TODO:    Ne_Text_Buffer *buf = ((TextWidget)w)->text.textD->buffer;
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
// TODO: 
// TODO:    /* If there's a calltip displayed, kill it. */
// TODO:    TextDKillCalltip(textD, 0);
// TODO: 
// TODO:    if (dragState == PRIMARY_DRAG || dragState == PRIMARY_RECT_DRAG)
// TODO:       BufUnselect(buf);
// TODO:    cancelDrag(w);
// TODO: }

static void secondaryStartAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer *buf = textD->buffer;
   Ne_Text_Selection* sel = &buf->getSecSelection();
   int anchor, pos, row, column;

   // Find the new anchor point and make the new selection 
   pos = textD->XYToPosition(Fl::event_x(), Fl::event_y());
   if (sel->selected)
   {
      if (abs(pos - sel->start) < abs(pos - sel->end))
         anchor = sel->end;
      else
         anchor = sel->start;
      buf->secondarySelect(anchor, pos);
   }
   else
      anchor = pos;

   /* Record the site of the initial button press and the initial character
      position so subsequent motion events can decide when to begin a
      selection, (and where the selection began) */
   textD->btnDownX = Fl::event_x();
   textD->btnDownY = Fl::event_y();
   textD->anchor = pos;
   textD->XYToUnconstrainedPosition(Fl::event_x(), Fl::event_y(), &row, &column);
   column = textD->OffsetWrappedColumn(row, column);
// TODO:    textD->rectAnchor = column;
   textD->dragState = NE_SECONDARY_CLICKED;
}

static void secondaryOrDragStartAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer *buf = textD->buffer;

   // If the click was outside of the primary selection, this is not
   // a drag, start a secondary selection
   if (!buf->getSelection().selected || !textD->inSelection(Fl::event_x(), Fl::event_y()))
   {
      secondaryStartAP(w, event, args, nArgs);
      return;
   }

   if (textD->checkReadOnly())
      return;

   // Record the site of the initial button press and the initial character
   // position so subsequent motion events can decide when to begin a
   // drag, and where to drag to
   textD->btnDownX = Fl::event_x();
   textD->btnDownY = Fl::event_y();
   textD->dragState = NE_CLICKED_IN_SELECTION;
}

static void secondaryAdjustAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int dragState = textD->dragState;

   // Make sure the proper initialization was done on mouse down
   if (dragState != NE_SECONDARY_DRAG && dragState != NE_SECONDARY_CLICKED)
      return;

   // If the selection hasn't begun, decide whether the mouse has moved
   // far enough from the initial mouse down to be considered a drag
   if (textD->dragState == NE_SECONDARY_CLICKED)
   {
      if (textD->mouseMoveForDrag(Fl::event_x(), Fl::event_y()))
         textD->dragState = NE_SECONDARY_DRAG;
      else
         return;
   }

   // If "rect" argument has appeared or disappeared, keep dragState up
   // to date about which type of drag this is
   textD->dragState = NE_SECONDARY_DRAG;

   // Record the new position for the autoscrolling timer routine, and
   // engage or disengage the timer if the mouse is in/out of the window
   textD->checkAutoScroll(Fl::event_x(), Fl::event_y());

   // Adjust the selection
   textD->adjustSecondarySelection(Fl::event_x(), Fl::event_y());
}

static void secondaryOrDragAdjustAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int dragState = textD->dragState;

   // Only dragging of blocks of text is handled in this action proc. Otherwise, defer to secondaryAdjust to handle the rest
   if (dragState != NE_CLICKED_IN_SELECTION && dragState != NE_PRIMARY_BLOCK_DRAG)
   {
      secondaryAdjustAP(w, event, args, nArgs);
      return;
   }

   // Decide whether the mouse has moved far enough from the initial mouse down to be considered a drag
   if (textD->dragState == NE_CLICKED_IN_SELECTION)
   {
      if (textD->mouseMoveForDrag(Fl::event_x(), Fl::event_y()))
         BeginBlockDrag(textD);
      else
         return;
   }

   // Record the new position for the autoscrolling timer routine, and
   // engage or disengage the timer if the mouse is in/out of the window
   textD->checkAutoScroll(Fl::event_x(), Fl::event_y());

   // Adjust the selection
   BlockDragSelection(textD, Fl::event_x(), Fl::event_y(), hasKey("overlay", args, nArgs) ?
                      (hasKey("copy", args, nArgs) ? NE_DRAG_OVERLAY_COPY : NE_DRAG_OVERLAY_MOVE) :
                         (hasKey("copy", args, nArgs) ? NE_DRAG_COPY : NE_DRAG_MOVE));
}

static void copyToAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int dragState = textD->dragState;
   Ne_Text_Buffer *buf = textD->buffer;
   Ne_Text_Selection*secondary = &buf->getSecSelection(), *primary = &buf->getSelection();
   int insertPos, lineStart, column;

   textD->endDrag();
   if (!((dragState == NE_SECONDARY_DRAG && secondary->selected) || dragState == NE_SECONDARY_CLICKED || dragState == NE_NOT_CLICKED))
      return;
   if (!(secondary->selected)) // TODO: && !(textD->motifDestOwner)))
   {
      if (textD->checkReadOnly())
      {
         buf->secondaryUnselect();
         return;
      }
   }
   if (secondary->selected)
   {
// TODO:       if (textD->motifDestOwner)
// TODO:       {
         textD->blankCursor();
         std::string textToCopy = buf->getSecSelectText();
         textD->insertAtCursor(textToCopy.c_str(), event, true, textD->autoWrapPastedText);
         buf->secondaryUnselect();
         textD->unblankCursor();
// TODO:       }
// TODO:       else
// TODO:          SendSecondarySelection(w, GetTimeOfDay(), false);
   }
   else if (primary->selected)
   {
      std::string textToCopy = buf->getSelectionText();
      textD->setInsertPosition(textD->XYToPosition(Fl::event_x(), Fl::event_y()));
      textD->insertAtCursor(textToCopy.c_str(), event, false, textD->autoWrapPastedText);
   }
   else
   {
      textD->setInsertPosition(textD->XYToPosition(Fl::event_x(), Fl::event_y()));
// TODO:       InsertPrimarySelection(w, GetTimeOfDay(), false);
   }
}

static void copyToOrEndDragAP(Fl_Widget* w, int event, const char**args, int *nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int dragState = textD->dragState;

   if (dragState != NE_PRIMARY_BLOCK_DRAG)
   {
      copyToAP(w, event, args, nArgs);
      return;
   }

   FinishBlockDrag(w);
}

static void moveToAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
// TODO:    XButtonEvent *e = &event->xbutton;
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
// TODO:    int dragState = ((TextWidget)w)->text.dragState;
// TODO:    Ne_Text_Buffer *buf = textD->buffer;
// TODO:    selection *secondary = &buf->secondary, *primary = &buf->primary;
// TODO:    int insertPos, rectangular = secondary->rectangular;
// TODO:    int column, lineStart;
// TODO:    char *textToCopy;
// TODO: 
// TODO:    endDrag(w);
// TODO:    if (!((dragState == SECONDARY_DRAG && secondary->selected) ||
// TODO:          (dragState == SECONDARY_RECT_DRAG && secondary->selected) ||
// TODO:          dragState == SECONDARY_CLICKED || dragState == NOT_CLICKED))
// TODO:       return;
// TODO:    if (checkReadOnly(w))
// TODO:    {
// TODO:       BufSecondaryUnselect(buf);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    if (secondary->selected)
// TODO:    {
// TODO:       if (((TextWidget)w)->text.motifDestOwner)
// TODO:       {
// TODO:          textToCopy = BufGetSecSelectText(buf);
// TODO:          if (primary->selected && rectangular)
// TODO:          {
// TODO:             insertPos = TextDGetInsertPosition(textD);
// TODO:             BufReplaceSelected(buf, textToCopy);
// TODO:             TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO:          }
// TODO:          else if (rectangular)
// TODO:          {
// TODO:             insertPos = TextDGetInsertPosition(textD);
// TODO:             lineStart = BufStartOfLine(buf, insertPos);
// TODO:             column = BufCountDispChars(buf, lineStart, insertPos);
// TODO:             BufInsertCol(buf, column, lineStart, textToCopy, NULL, NULL);
// TODO:             TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO:          }
// TODO:          else
// TODO:             TextInsertAtCursor(w, textToCopy, event, True,
// TODO:                                ((TextWidget)w)->text.autoWrapPastedText);
// TODO:          XtFree(textToCopy);
// TODO:          BufRemoveSecSelect(buf);
// TODO:          BufSecondaryUnselect(buf);
// TODO:       }
// TODO:       else
// TODO:          SendSecondarySelection(w, e->time, True);
// TODO:    }
// TODO:    else if (primary->selected)
// TODO:    {
// TODO:       textToCopy = BufGetRange(buf, primary->start, primary->end);
// TODO:       TextDSetInsertPosition(textD, TextDXYToPosition(textD, e->x, e->y));
// TODO:       TextInsertAtCursor(w, textToCopy, event, False,
// TODO:                          ((TextWidget)w)->text.autoWrapPastedText);
// TODO:       XtFree(textToCopy);
// TODO:       BufRemoveSelected(buf);
// TODO:       BufUnselect(buf);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       TextDSetInsertPosition(textD, TextDXYToPosition(textD, e->x, e->y));
// TODO:       MovePrimarySelection(w, e->time, False);
// TODO:    }
}

// TODO: static void moveToOrEndDragAP(Fl_Widget* w, int event, String *args,int *nArgs)
// TODO: {
// TODO:    int dragState = ((TextWidget)w)->text.dragState;
// TODO: 
// TODO:    if (dragState != PRIMARY_BLOCK_DRAG)
// TODO:    {
// TODO:       moveToAP(w, event, args, nArgs);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    FinishBlockDrag((TextWidget)w);
// TODO: }

static void endDragAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   if (textD->dragState == NE_PRIMARY_BLOCK_DRAG)
      FinishBlockDrag(w);
   else
      textD->endDrag();
}

// TODO: static void exchangeAP(Fl_Widget* w, int, const char** args, int* nArgs)
// TODO: {
// TODO:    XButtonEvent *e = &event->xbutton;
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
// TODO:    Ne_Text_Buffer *buf = textD->buffer;
// TODO:    selection *sec = &buf->secondary, *primary = &buf->primary;
// TODO:    char *primaryText, *secText;
// TODO:    int newPrimaryStart, newPrimaryEnd, secWasRect;
// TODO:    int dragState = ((TextWidget)w)->text.dragState; /* save before endDrag */
// TODO:    int silent = hasKey("nobell", args, nArgs);
// TODO: 
// TODO:    endDrag(w);
// TODO:    if (checkReadOnly(w))
// TODO:       return;
// TODO: 
// TODO:    /* If there's no secondary selection here, or the primary and secondary
// TODO:       selection overlap, just beep and return */
// TODO:    if (!sec->selected || (primary->selected &&
// TODO:                           ((primary->start <= sec->start && primary->end > sec->start) ||
// TODO:                            (sec->start <= primary->start && sec->end > primary->start))))
// TODO:    {
// TODO:       BufSecondaryUnselect(buf);
// TODO:       ringIfNecessary(silent);
// TODO:       /* If there's no secondary selection, but the primary selection is
// TODO:          being dragged, we must not forget to finish the dragging.
// TODO:          Otherwise, modifications aren't recorded. */
// TODO:       if (dragState == PRIMARY_BLOCK_DRAG)
// TODO:          FinishBlockDrag((TextWidget)w);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* if the primary selection is in another widget, use selection routines */
// TODO:    if (!primary->selected)
// TODO:    {
// TODO:       ExchangeSelections(w, e->time);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Both primary and secondary are in this widget, do the exchange here */
// TODO:    primaryText = BufGetSelectionText(buf);
// TODO:    secText = BufGetSecSelectText(buf);
// TODO:    secWasRect = sec->rectangular;
// TODO:    BufReplaceSecSelect(buf, primaryText);
// TODO:    newPrimaryStart = primary->start;
// TODO:    BufReplaceSelected(buf, secText);
// TODO:    newPrimaryEnd = newPrimaryStart + strlen(secText);
// TODO:    XtFree(primaryText);
// TODO:    XtFree(secText);
// TODO:    BufSecondaryUnselect(buf);
// TODO:    if (secWasRect)
// TODO:    {
// TODO:       TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       BufSelect(buf, newPrimaryStart, newPrimaryEnd);
// TODO:       TextDSetInsertPosition(textD, newPrimaryEnd);
// TODO:    }
// TODO:    checkAutoShowInsertPos(w);
// TODO: }
// TODO: 
// TODO: static void copyPrimaryAP(Fl_Widget* w, int event, const char**args, int*nArgs)
// TODO: {
// TODO:    XKeyEvent *e = &event->xkey;
// TODO:    TextWidget tw = (TextWidget)w;
// TODO:    Ne_Text_Editor* textD = tw->text.textD;
// TODO:    Ne_Text_Buffer *buf = textD->buffer;
// TODO:    selection *primary = &buf->primary;
// TODO:    int rectangular = hasKey("rect", args, nArgs);
// TODO:    char *textToCopy;
// TODO:    int insertPos, col;
// TODO: 
// TODO:    cancelDrag(w);
// TODO:    if (checkReadOnly(w))
// TODO:       return;
// TODO:    if (primary->selected && rectangular)
// TODO:    {
// TODO:       textToCopy = BufGetSelectionText(buf);
// TODO:       insertPos = TextDGetInsertPosition(textD);
// TODO:       col = BufCountDispChars(buf, BufStartOfLine(buf, insertPos), insertPos);
// TODO:       BufInsertCol(buf, col, insertPos, textToCopy, NULL, NULL);
// TODO:       TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO:       XtFree(textToCopy);
// TODO:       checkAutoShowInsertPos(w);
// TODO:    }
// TODO:    else if (primary->selected)
// TODO:    {
// TODO:       textToCopy = BufGetSelectionText(buf);
// TODO:       insertPos = TextDGetInsertPosition(textD);
// TODO:       BufInsert(buf, insertPos, textToCopy);
// TODO:       TextDSetInsertPosition(textD, insertPos + strlen(textToCopy));
// TODO:       XtFree(textToCopy);
// TODO:       checkAutoShowInsertPos(w);
// TODO:    }
// TODO:    else if (rectangular)
// TODO:    {
// TODO:       if (!TextDPositionToXY(textD, TextDGetInsertPosition(textD),
// TODO:                              &tw->text.btnDownX, &tw->text.btnDownY))
// TODO:          return; /* shouldn't happen */
// TODO:       InsertPrimarySelection(w, e->time, True);
// TODO:    }
// TODO:    else
// TODO:       InsertPrimarySelection(w, e->time, False);
// TODO: }
// TODO: 
// TODO: static void cutPrimaryAP(Fl_Widget* w, int event, const char**args, int*nArgs)
// TODO: {
// TODO:    XKeyEvent *e = &event->xkey;
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
// TODO:    Ne_Text_Buffer *buf = textD->buffer;
// TODO:    selection *primary = &buf->primary;
// TODO:    char *textToCopy;
// TODO:    int rectangular = hasKey("rect", args, nArgs);
// TODO:    int insertPos, col;
// TODO: 
// TODO:    cancelDrag(w);
// TODO:    if (checkReadOnly(w))
// TODO:       return;
// TODO:    if (primary->selected && rectangular)
// TODO:    {
// TODO:       textToCopy = BufGetSelectionText(buf);
// TODO:       insertPos = TextDGetInsertPosition(textD);
// TODO:       col = BufCountDispChars(buf, BufStartOfLine(buf, insertPos), insertPos);
// TODO:       BufInsertCol(buf, col, insertPos, textToCopy, NULL, NULL);
// TODO:       TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO:       XtFree(textToCopy);
// TODO:       BufRemoveSelected(buf);
// TODO:       checkAutoShowInsertPos(w);
// TODO:    }
// TODO:    else if (primary->selected)
// TODO:    {
// TODO:       textToCopy = BufGetSelectionText(buf);
// TODO:       insertPos = TextDGetInsertPosition(textD);
// TODO:       BufInsert(buf, insertPos, textToCopy);
// TODO:       TextDSetInsertPosition(textD, insertPos + strlen(textToCopy));
// TODO:       XtFree(textToCopy);
// TODO:       BufRemoveSelected(buf);
// TODO:       checkAutoShowInsertPos(w);
// TODO:    }
// TODO:    else if (rectangular)
// TODO:    {
// TODO:       if (!TextDPositionToXY(textD, TextDGetInsertPosition(textD),
// TODO:                              &((TextWidget)w)->text.btnDownX,
// TODO:                              &((TextWidget)w)->text.btnDownY))
// TODO:          return; /* shouldn't happen */
// TODO:       MovePrimarySelection(w, e->time, True);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       MovePrimarySelection(w, e->time, False);
// TODO:    }
// TODO: }

static void mousePanAP(Fl_Widget* w, int, const char** args, int* nArgs)
{
   TRACE();
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   int lineHeight = textD->fontStruct.height();
   int topLineNum, horizOffset;

   if (textD->dragState == NE_MOUSE_PAN)
   {
      textD->setScroll( (textD->btnDownY - Fl::event_y() + lineHeight/2) / lineHeight, textD->btnDownX - Fl::event_x());
   }
   else if (textD->dragState == NE_NOT_CLICKED)
   {
      textD->getScroll(&topLineNum, &horizOffset);
      textD->btnDownX = Fl::event_x() + horizOffset;
      textD->btnDownY = Fl::event_y() + topLineNum * lineHeight;
      textD->dragState = NE_MOUSE_PAN;
      fl_cursor(FL_CURSOR_CROSS);
   }
   else
      textD->cancelDrag();
}

// TODO: static void pasteClipboardAP(Fl_Widget* w, int event, const char**args, int*nArgs)
// TODO: {
// TODO:    if (hasKey("rect", args, nArgs))
// TODO:       TextColPasteClipboard(w, event->xkey.time);
// TODO:    else
// TODO:       TextPasteClipboard(w, event->xkey.time);
// TODO: }
// TODO: 
// TODO: static void copyClipboardAP(Fl_Widget* w, int event, const char**args, int*nArgs)
// TODO: {
// TODO:    TextCopyClipboard(w, event->xkey.time);
// TODO: }
// TODO: 
// TODO: static void cutClipboardAP(Fl_Widget* w, int event, const char**args, int*nArgs)
// TODO: {
// TODO:    TextCutClipboard(w, event->xkey.time);
// TODO: }
// TODO: 
// TODO: static void insertStringAP(Fl_Widget* w, int event, const char**args, int*nArgs)
// TODO: {
// TODO:    SmartIndentCBStruct smartIndent;
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
// TODO: 
// TODO:    if (*nArgs == 0)
// TODO:       return;
// TODO:    cancelDrag(w);
// TODO:    if (checkReadOnly(w))
// TODO:       return;
// TODO:    if (((TextWidget)w)->text.smartIndent)
// TODO:    {
// TODO:       smartIndent.reason = CHAR_TYPED;
// TODO:       smartIndent.pos = TextDGetInsertPosition(textD);
// TODO:       smartIndent.indentRequest = 0;
// TODO:       smartIndent.charsTyped = args[0];
// TODO:       XtCallCallbacks(w, textNsmartIndentCallback, (XtPointer)&smartIndent);
// TODO:    }
// TODO:    TextInsertAtCursor(w, args[0], event, True, True);
// TODO:    BufUnselect((((TextWidget)w)->text.textD)->buffer);
// TODO: }

static void selfInsertAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   TRACE();

   NeSmartIndentCBStruct smartIndent;
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   char* chars;
   {  const char* utf8 = Fl::event_text();
      chars = new char[strlen(utf8)+1];
      fl_utf8toa(utf8, strlen(utf8)+1, chars, strlen(utf8)+1);
   }

   if (strlen(chars) == 0)
      return;
   textD->cancelDrag();
   if (textD->checkReadOnly())
      return;
// TODO:    TakeMotifDestination(w, e->time);

   if (!textD->buffer->substituteNullChars(chars, strlen(chars)))
   {
// TODO:       DialogF(DF_ERR,textD, 1, "Error", "Too much binary data", "OK");
      delete[] chars;
      return;
   }

   // If smart indent is on, call the smart indent callback to check the inserted character
   if (textD->smartIndent)
   {
      smartIndent.reason = NE_CHAR_TYPED;
      smartIndent.pos = textD->getInsertPosition();
      smartIndent.indentRequest = 0;
      smartIndent.charsTyped = (char*)chars;
// TODO:       XtCallCallbacks(w, textNsmartIndentCallback, (XtPointer)&smartIndent);
   }
   textD->insertAtCursor(chars, event, true, true);
   textD->buffer->unselect();
   delete[] chars;
}

static void newlineAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* editor = (Ne_Text_Editor*)w;
   if (editor->autoIndent || editor->smartIndent)
      newlineAndIndentAP(w, event, args, nArgs);
   else
      newlineNoIndentAP(w, event, args, nArgs);
}

static void newlineNoIndentAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* editor = (Ne_Text_Editor*)w;

   editor->cancelDrag();
   if (editor->checkReadOnly())
      return;
// TODO:    TakeMotifDestination(w, e->time);
   editor->simpleInsertAtCursor("\n", event, true);
   editor->buffer->unselect();
}

static void newlineAndIndentAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer *buf = textD->buffer;
   char *indentStr;
   int cursorPos, lineStartPos, column;

   if (textD->checkReadOnly())
      return;
   textD->cancelDrag();
// TODO:    TakeMotifDestination(w, e->time);

   // Create a string containing a newline followed by auto or smart indent string
   cursorPos = textD->getInsertPosition();
   lineStartPos = buf->startOfLine(cursorPos);
   indentStr = textD->createIndentString(buf, 0, lineStartPos, cursorPos, NULL, &column);

   // Insert it at the cursor
   textD->simpleInsertAtCursor(indentStr, event, true);
   delete[] indentStr;

   if (textD->emulateTabs > 0)
   {
      // If emulated tabs are on, make the inserted indent deletable by
      // tab. Round this up by faking the column a bit to the right to
      // let the user delete half-tabs with one keypress.
      column += textD->emulateTabs - 1;
      textD->emTabsBeforeCursor = column / textD->emulateTabs;
   }

   buf->unselect();
}

static void processTabAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer *buf = textD->buffer;
   Ne_Text_Selection* sel = &buf->getSelection();
   int emTabDist = textD->emulateTabs;
   int emTabsBeforeCursor = textD->emTabsBeforeCursor;
   int insertPos, indent, startIndent, toIndent, lineStart, tabWidth;
   char *outStr, *outPtr;

   if (textD->checkReadOnly())
      return;
   textD->cancelDrag();
// TODO:    TakeMotifDestination(w, event->xkey.time);

   // If emulated tabs are off, just insert a tab
   if (emTabDist <= 0)
   {
      textD->insertAtCursor("\t", event, true, true);
      return;
   }

   /* Find the starting and ending indentation.  If the tab is to
      replace an existing selection, use the start of the selection
      instead of the cursor position as the indent.  When replacing
      rectangular selections, tabs are automatically recalculated as
      if the inserted text began at the start of the line */
   insertPos = textD->pendingSelection() ? sel->start : textD->getInsertPosition();
   lineStart = buf->startOfLine(insertPos);
// TODO:    if (textD->pendingSelection() && sel->rectangular)
// TODO:       insertPos = BufCountForwardDispChars(buf, lineStart, sel->rectStart);
   startIndent = buf->countDispChars(lineStart, insertPos);
   toIndent = startIndent + emTabDist - (startIndent % emTabDist);
// TODO:    if (textD->pendingSelection() && sel->rectangular)
// TODO:    {
// TODO:       toIndent -= startIndent;
// TODO:       startIndent = 0;
// TODO:    }

   /* Allocate a buffer assuming all the inserted characters will be spaces */
   outStr = new char[toIndent - startIndent + 1];

   /* Add spaces and tabs to outStr until it reaches toIndent */
   outPtr = outStr;
   indent = startIndent;
   while (indent < toIndent)
   {
      tabWidth = buf->charWidth('\t', indent, buf->getTabDistance(), buf->getNullSubsChar());
// TODO:       if (buf->useTabs && tabWidth > 1 && indent + tabWidth <= toIndent)
// TODO:       {
// TODO:          *outPtr++ = '\t';
// TODO:          indent += tabWidth;
// TODO:       }
// TODO:       else
      {
         *outPtr++ = ' ';
         indent++;
      }
   }
   *outPtr = '\0';

   // Insert the emulated tab
   textD->insertAtCursor(outStr, event, true, true);
   delete[] outStr;

   // Restore and ++ emTabsBeforeCursor cleared by TextInsertAtCursor
   textD->emTabsBeforeCursor = emTabsBeforeCursor + 1;

   buf->unselect();
}

static void deleteSelectionAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   textD->cancelDrag();
   if (textD->checkReadOnly())
      return;
// TODO:    TakeMotifDestination(w, e->time);
   textD->deletePendingSelection(event);
}

static void deletePreviousCharacterAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = textD->getInsertPosition();
   char c;
   bool silent = hasKey("nobell", args, nArgs);

   textD->cancelDrag();
   if (textD->checkReadOnly())
      return;

// TODO:    TakeMotifDestination(w, e->time);
   if (textD->deletePendingSelection(event))
      return;

   if (insertPos == 0)
   {
      ringIfNecessary(silent);
      return;
   }

   if (textD->deleteEmulatedTab(event))
      return;

   if (textD->isOverstrike)
   {
      c = textD->buffer->getCharacter(insertPos - 1);
      if (c == '\n')
         textD->buffer->remove(insertPos - 1, insertPos);
      else if (c != '\t')
         textD->buffer->replace(insertPos - 1, insertPos, " ");
   }
   else
   {
      textD->buffer->remove(insertPos - 1, insertPos);
   }

   textD->setInsertPosition(insertPos - 1);
   textD->checkAutoShowInsertPos();
   textD->callCursorMovementCBs(event);
}

static void deleteNextCharacterAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = textD->getInsertPosition();
   bool silent = hasKey("nobell", args, nArgs);

   textD->cancelDrag();
   if (textD->checkReadOnly())
      return;
// TODO:    TakeMotifDestination(w, e->time);
   if (textD->deletePendingSelection(event))
      return;
   if (insertPos == textD->buffer->size())
   {
      ringIfNecessary(silent);
      return;
   }
   textD->buffer->remove(insertPos , insertPos + 1);
   textD->checkAutoShowInsertPos();
   textD->callCursorMovementCBs(event);
}

static void deletePreviousWordAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = textD->getInsertPosition();
   int pos, lineStart = textD->buffer->startOfLine(insertPos);
   const std::string& delimiters = textD->delimiters;
   bool silent = hasKey("nobell", args, nArgs);

   textD->cancelDrag();
   if (textD->checkReadOnly())
   {
      return;
   }

// TODO:    TakeMotifDestination(w, e->time);
   if (textD->deletePendingSelection(event))
   {
      return;
   }

   if (insertPos == lineStart)
   {
      ringIfNecessary(silent);
      return;
   }

   pos = std::max(insertPos - 1, 0);
   while (strchr(delimiters.c_str(), textD->buffer->getCharacter(pos)) != NULL && pos != lineStart)
   {
      pos--;
   }

   pos = textD->startOfWord(pos);
   textD->buffer->remove(pos, insertPos);
   textD->checkAutoShowInsertPos();
   textD->callCursorMovementCBs(event);
}

static void deleteNextWordAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = textD->getInsertPosition();
   int pos, lineEnd = textD->buffer->endOfLine(insertPos);
   const std::string& delimiters = textD->delimiters;
   bool silent = hasKey("nobell", args, nArgs);

   textD->cancelDrag();
   if (textD->checkReadOnly())
   {
      return;
   }

// TODO:    TakeMotifDestination(w, e->time);
   if (textD->deletePendingSelection(event))
   {
      return;
   }

   if (insertPos == lineEnd)
   {
      ringIfNecessary(silent);
      return;
   }

   pos = insertPos;
   while (strchr(delimiters.c_str(), textD->buffer->getCharacter(pos)) != NULL && pos != lineEnd)
   {
      pos++;
   }

   pos = textD->endOfWord(pos);
   textD->buffer->remove(insertPos, pos);
   textD->checkAutoShowInsertPos();
   textD->callCursorMovementCBs(event);
}

static void deleteToEndOfLineAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = textD->getInsertPosition();
   int endOfLine;
   bool silent = hasKey("nobell", args, nArgs);

   if (hasKey("absolute", args, nArgs))
      endOfLine = textD->buffer->endOfLine(insertPos);
   else
      endOfLine = textD->endOfLine(insertPos, false);
   textD->cancelDrag();
   if (textD->checkReadOnly())
      return;
// TODO:    TakeMotifDestination(w, e->time);
   if (textD->deletePendingSelection(event))
      return;
   if (insertPos == endOfLine)
   {
      ringIfNecessary(silent);
      return;
   }
   textD->buffer->remove(insertPos, endOfLine);
   textD->checkAutoShowInsertPos();
   textD->callCursorMovementCBs(event);
}

static void deleteToStartOfLineAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = textD->getInsertPosition();
   int startOfLine;
   bool silent = hasKey("nobell", args, nArgs);

   if (hasKey("wrap", args, nArgs))
      startOfLine = textD->startOfLine(insertPos);
   else
      startOfLine = textD->buffer->startOfLine(insertPos);
   textD->cancelDrag();
   if (textD->checkReadOnly())
      return;
// TODO:    TakeMotifDestination(w, e->time);
   if (textD->deletePendingSelection(event))
      return;
   if (insertPos == startOfLine)
   {
      ringIfNecessary(silent);
      return;
   }
   textD->buffer->remove(startOfLine, insertPos);
   textD->checkAutoShowInsertPos();
   textD->callCursorMovementCBs(event);
}

// --------------------------------------------------------------------------
static void forwardCharacterAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* editor = (Ne_Text_Editor*)w;
   int insertPos = editor->getInsertPosition();
   bool silent = hasKey("nobell", args, nArgs);

   editor->cancelDrag();
   if (!editor->moveRight())
      ringIfNecessary(silent);

   editor->checkMoveSelectionChange(event, insertPos, args, nArgs);
   editor->checkAutoShowInsertPos();
   editor->callCursorMovementCBs(event);
}

// --------------------------------------------------------------------------
static void backwardCharacterAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   TRACE();
   Ne_Text_Editor* editor = (Ne_Text_Editor*)w;
   int insertPos = editor->getInsertPosition();
   bool silent = hasKey("nobell", args, nArgs);

   editor->cancelDrag();
   if (!editor->moveLeft())
      ringIfNecessary(silent);

   editor->checkMoveSelectionChange(event, insertPos, args, nArgs);
   editor->checkAutoShowInsertPos();
   editor->callCursorMovementCBs(event);
}

// --------------------------------------------------------------------------
static void forwardWordAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* editor = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = editor->buffer;
   int pos, insertPos = editor->getInsertPosition();
   std::string delimiters = editor->delimiters;
   int silent = hasKey("nobell", args, nArgs);

   editor->cancelDrag();
   if (insertPos == buf->size())
   {
      ringIfNecessary(silent);
      return;
   }
   pos = insertPos;

   if (hasKey("tail", args, nArgs))
   {
      for (; pos<buf->size(); pos++)
      {
         if (NULL == strchr(delimiters.c_str(), buf->getCharacter(pos)))
         {
            break;
         }
      }
      if (NULL == strchr(delimiters.c_str(), buf->getCharacter(pos)))
      {
         pos = editor->endOfWord(pos);
      }
   }
   else
   {
      if (NULL == strchr(delimiters.c_str(), buf->getCharacter(pos)))
      {
         pos = editor->endOfWord(pos);
      }
      for (; pos<buf->size(); pos++)
      {
         if (NULL == strchr(delimiters.c_str(), buf->getCharacter(pos)))
         {
            break;
         }
      }
   }

   editor->setInsertPosition(pos);
   editor->checkMoveSelectionChange(event, insertPos, args, nArgs);
   editor->checkAutoShowInsertPos();
   editor->callCursorMovementCBs(event);
}

// --------------------------------------------------------------------------
static void backwardWordAP(Fl_Widget* w, int event, const char** args, int *nArgs)
{
   Ne_Text_Editor* editor = (Ne_Text_Editor*)w;
   Ne_Text_Buffer* buf = editor->buffer;
   const std::string& delimiters = editor->delimiters;
   int pos, insertPos = editor->getInsertPosition();
   int silent = hasKey("nobell", args, nArgs);

   editor->cancelDrag();
   if (insertPos == 0)
   {
      ringIfNecessary(silent);
      return;
   }
   pos = std::max(insertPos - 1, 0);
   while (strchr(delimiters.c_str(), buf->getCharacter(pos)) != NULL && pos > 0)
      pos--;
   pos = editor->startOfWord(pos);

   editor->setInsertPosition(pos);
   editor->checkMoveSelectionChange(event, insertPos, args, nArgs);
   editor->checkAutoShowInsertPos();
   editor->callCursorMovementCBs(event);
}

static void forwardParagraphAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int pos, insertPos = textD->getInsertPosition();
   Ne_Text_Buffer *buf = textD->buffer;
   char c;
   static char whiteChars[] = " \t";
   int silent = hasKey("nobell", args, nArgs);

   textD->cancelDrag();
   if (insertPos == buf->size())
   {
      ringIfNecessary(silent);
      return;
   }
   pos = std::min(buf->endOfLine(insertPos)+1, buf->size());
   while (pos < buf->size())
   {
      c = buf->getCharacter(pos);
      if (c == '\n')
         break;
      if (strchr(whiteChars, c) != NULL)
         pos++;
      else
         pos = std::min(buf->endOfLine(pos)+1, buf->size());
   }
   textD->setInsertPosition(std::min(pos+1, buf->size()));
   textD->checkMoveSelectionChange(event, insertPos, args, nArgs);
   textD->checkAutoShowInsertPos();
   textD->callCursorMovementCBs(event);
}

static void backwardParagraphAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int parStart, pos, insertPos = textD->getInsertPosition();
   Ne_Text_Buffer *buf = textD->buffer;
   char c;
   static char whiteChars[] = " \t";
   int silent = hasKey("nobell", args, nArgs);

   textD->cancelDrag();
   if (insertPos == 0)
   {
      ringIfNecessary(silent);
      return;
   }
   parStart = buf->startOfLine(std::max(insertPos-1, 0));
   pos = std::max(parStart - 2, 0);
   while (pos > 0)
   {
      c = buf->getCharacter(pos);
      if (c == '\n')
         break;
      if (strchr(whiteChars, c) != NULL)
         pos--;
      else
      {
         parStart = buf->startOfLine(pos);
         pos = std::max(parStart - 2, 0);
      }
   }
   textD->setInsertPosition(parStart);
   textD->checkMoveSelectionChange(event, insertPos, args, nArgs);
   textD->checkAutoShowInsertPos();
   textD->callCursorMovementCBs(event);
}

static void keySelectAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* editor = (Ne_Text_Editor*)w;
   int stat, insertPos = editor->getInsertPosition();
   int silent = hasKey("nobell", args, nArgs);

   editor->cancelDrag();
   if (hasKey("left", args, nArgs)) stat = editor->moveLeft();
   else if (hasKey("right", args, nArgs)) stat = editor->moveRight();
   else if (hasKey("up", args, nArgs)) stat = editor->moveUp(false);
   else if (hasKey("down", args, nArgs)) stat = editor->moveDown(false);
   else
   {
      editor->keyMoveExtendSelection(event, insertPos, hasKey("rect", args,nArgs));
      return;
   }
   if (!stat)
   {
      ringIfNecessary(silent);
   }
   else
   {
      editor->keyMoveExtendSelection(event, insertPos, hasKey("rect", args,nArgs));
      editor->checkAutoShowInsertPos();
      editor->callCursorMovementCBs(event);
   }
}

static void processUpAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* editor = (Ne_Text_Editor*)w;
   int insertPos = editor->getInsertPosition();
   bool silent = hasKey("nobell", args, nArgs);
   bool abs = hasKey("absolute", args, nArgs);

   editor->cancelDrag();
   if (!editor->moveUp(abs))
      ringIfNecessary(silent);
   editor->checkMoveSelectionChange(event, insertPos, args, nArgs);
   editor->checkAutoShowInsertPos();
   editor->callCursorMovementCBs(event);
}

static void processShiftUpAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* editor = (Ne_Text_Editor*)w;
   int insertPos = editor->getInsertPosition();
   bool silent = hasKey("nobell", args, nArgs);
   bool abs = hasKey("absolute", args, nArgs);

   editor->cancelDrag();
   if (!editor->moveUp(abs))
      ringIfNecessary(silent);
   editor->keyMoveExtendSelection(event, insertPos, hasKey("rect", args, nArgs));
   editor->checkAutoShowInsertPos();
   editor->callCursorMovementCBs(event);
}

static void processDownAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* editor = (Ne_Text_Editor*)w;
   int insertPos = editor->getInsertPosition();
   bool silent = hasKey("nobell", args, nArgs);
   bool abs = hasKey("absolute", args, nArgs);

   editor->cancelDrag();
   if (!editor->moveDown(abs))
      ringIfNecessary(silent);
   editor->checkMoveSelectionChange(event, insertPos, args, nArgs);
   editor->checkAutoShowInsertPos();
   editor->callCursorMovementCBs(event);
}

static void processShiftDownAP(Fl_Widget* w, int event, const char** args, int *nArgs)
{
   Ne_Text_Editor* editor = (Ne_Text_Editor*)w;
   int insertPos = editor->getInsertPosition();
   bool silent = hasKey("nobell", args, nArgs);
   bool abs = hasKey("absolute", args, nArgs);

   editor->cancelDrag();
   if (!editor->moveDown(abs))
      ringIfNecessary(silent);
   editor->keyMoveExtendSelection(event, insertPos, hasKey("rect", args, nArgs));
   editor->checkAutoShowInsertPos();
   editor->callCursorMovementCBs(event);
}

static void beginningOfLineAP(Fl_Widget* w, int event, const char**args, int *nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = textD->getInsertPosition();

   textD->cancelDrag();
   if (hasKey("absolute", args, nArgs))
      textD->setInsertPosition(textD->buffer->startOfLine(insertPos));
   else
      textD->setInsertPosition(textD->startOfLine(insertPos));
   textD->checkMoveSelectionChange(event, insertPos, args, nArgs);
   textD->checkAutoShowInsertPos();
   textD->callCursorMovementCBs(event);
   textD->cursorPreferredCol = 0;
}

static void endOfLineAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = textD->getInsertPosition();

   textD->cancelDrag();
   if (hasKey("absolute", args, nArgs))
      textD->setInsertPosition(textD->buffer->endOfLine(insertPos));
   else
      textD->setInsertPosition(textD->endOfLine(insertPos, false));
   textD->checkMoveSelectionChange(event, insertPos, args, nArgs);
   textD->checkAutoShowInsertPos();
   textD->callCursorMovementCBs(event);
   textD->cursorPreferredCol = -1;
}

static void beginningOfFileAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = textD->getInsertPosition();

   textD->cancelDrag();
   if (hasKey("scrollbar", args, nArgs))
   {
      if (textD->topLineNum != 1)
      {
         textD->setScroll(1, textD->horizOffset);
      }
   }
   else
   {
      textD->setInsertPosition(0);
      textD->checkMoveSelectionChange(event, insertPos, args, nArgs);
      textD->checkAutoShowInsertPos();
      textD->callCursorMovementCBs(event);
   }
}

static void endOfFileAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = textD->getInsertPosition();
   int lastTopLine;

   textD->cancelDrag();
   if (hasKey("scrollbar", args, nArgs))
   {
      lastTopLine = std::max(1,
                        textD->nBufferLines - (textD->nVisibleLines - 2) +
                        textD->cursorVPadding);
      if (lastTopLine != textD->topLineNum)
      {
         textD->setScroll(lastTopLine, textD->horizOffset);
      }
   }
   else
   {
      textD->setInsertPosition(textD->buffer->size());
      textD->checkMoveSelectionChange(event, insertPos, args, nArgs);
      textD->checkAutoShowInsertPos();
      textD->callCursorMovementCBs(event);
   }
}

static void nextPageAP(Fl_Widget* w, int event, const char** args, int* nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer *buf = textD->buffer;
   int lastTopLine = std::max(1,
                         textD->nBufferLines - (textD->nVisibleLines - 2) +
                         textD->cursorVPadding );
   int insertPos = textD->getInsertPosition();
   int column = 0, visLineNum, lineStartPos;
   int pos, targetLine;
   int pageForwardCount = std::max(1, textD->nVisibleLines - 1);
   int maintainColumn = 0;
   bool silent = hasKey("nobell", args, nArgs);

   maintainColumn = hasKey("column", args, nArgs);
   textD->cancelDrag();
   if (hasKey("scrollbar", args, nArgs))   /* scrollbar only */
   {
      targetLine = std::min(textD->topLineNum + pageForwardCount, lastTopLine);

      if (targetLine == textD->topLineNum)
      {
         ringIfNecessary(silent);
         return;
      }
      textD->setScroll(targetLine, textD->horizOffset);
   }
   else if (hasKey("stutter", args, nArgs))   /* Mac style */
   {
      /* move to bottom line of visible area */
      /* if already there, page down maintaining preferrred column */
      targetLine = std::max(std::min(textD->nVisibleLines - 1, textD->nBufferLines), 0);
      column = textD->preferredColumn(&visLineNum, &lineStartPos);
      if (lineStartPos == textD->lineStarts[targetLine])
      {
         if (insertPos >= buf->size() || textD->topLineNum == lastTopLine)
         {
            ringIfNecessary(silent);
            return;
         }
         targetLine = std::min(textD->topLineNum + pageForwardCount, lastTopLine);
         pos = textD->countForwardNLines(insertPos, pageForwardCount, false);
         if (maintainColumn)
         {
            pos = textD->posOfPreferredCol(column, pos);
         }
         textD->setInsertPosition(pos);
         textD->setScroll(targetLine, textD->horizOffset);
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
            pos = textD->posOfPreferredCol( column, pos);
         }
         textD->setInsertPosition(pos);
      }
      textD->checkMoveSelectionChange(event, insertPos, args, nArgs);
      textD->checkAutoShowInsertPos();
      textD->callCursorMovementCBs(event);
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
      if (insertPos >= buf->size() && textD->topLineNum == lastTopLine)
      {
         ringIfNecessary(silent);
         return;
      }
      if (maintainColumn)
      {
         column = textD->preferredColumn(&visLineNum, &lineStartPos);
      }
      targetLine = textD->topLineNum + textD->nVisibleLines - 1;
      if (targetLine < 1) targetLine = 1;
      if (targetLine > lastTopLine) targetLine = lastTopLine;
      pos = textD->countForwardNLines(insertPos, textD->nVisibleLines-1, false);
      if (maintainColumn)
      {
         pos = textD->posOfPreferredCol(column, pos);
      }
      textD->setInsertPosition(pos);
      textD->setScroll(targetLine, textD->horizOffset);
      textD->checkMoveSelectionChange(event, insertPos, args, nArgs);
      textD->checkAutoShowInsertPos();
      textD->callCursorMovementCBs(event);
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

static void previousPageAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int insertPos = textD->getInsertPosition();
   int column = 0, visLineNum, lineStartPos;
   int pos, targetLine;
   int pageBackwardCount = std::max(1, textD->nVisibleLines - 1);
   int maintainColumn = 0;
   int silent = hasKey("nobell", args, nArgs);

   maintainColumn = hasKey("column", args, nArgs);
   textD->cancelDrag();
   if (hasKey("scrollbar", args, nArgs))   /* scrollbar only */
   {
      targetLine = std::max(textD->topLineNum - pageBackwardCount, 1);

      if (targetLine == textD->topLineNum)
      {
         ringIfNecessary(silent);
         return;
      }
      textD->setScroll(targetLine, textD->horizOffset);
   }
   else if (hasKey("stutter", args, nArgs))   /* Mac style */
   {
      /* move to top line of visible area */
      /* if already there, page up maintaining preferrred column if required */
      targetLine = 0;
      column = textD->preferredColumn(&visLineNum, &lineStartPos);
      if (lineStartPos == textD->lineStarts[targetLine])
      {
         if (textD->topLineNum == 1 && (maintainColumn || column == 0))
         {
            ringIfNecessary(silent);
            return;
         }
         targetLine = std::max(textD->topLineNum - pageBackwardCount, 1);
         pos = textD->countBackwardNLines(insertPos, pageBackwardCount);
         if (maintainColumn)
         {
            pos = textD->posOfPreferredCol(column, pos);
         }
         textD->setInsertPosition(pos);
         textD->setScroll(targetLine, textD->horizOffset);
      }
      else
      {
         pos = textD->lineStarts[targetLine];
         if (maintainColumn)
         {
            pos = textD->posOfPreferredCol(column, pos);
         }
         textD->setInsertPosition(pos);
      }
      textD->checkMoveSelectionChange(event, insertPos, args, nArgs);
      textD->checkAutoShowInsertPos();
      textD->callCursorMovementCBs(event);
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
         column = textD->preferredColumn(&visLineNum, &lineStartPos);
      }
      targetLine = textD->topLineNum - (textD->nVisibleLines - 1);
      if (targetLine < 1) targetLine = 1;
      pos = textD->countBackwardNLines(insertPos, textD->nVisibleLines-1);
      if (maintainColumn)
      {
         pos = textD->posOfPreferredCol(column, pos);
      }
      textD->setInsertPosition(pos);
      textD->setScroll(targetLine, textD->horizOffset);
      textD->checkMoveSelectionChange(event, insertPos, args, nArgs);
      textD->checkAutoShowInsertPos();
      textD->callCursorMovementCBs(event);
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

// TODO: static void pageLeftAP(Fl_Widget* w, int, const char** args, int* nArgs)
// TODO: {
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
// TODO:    Ne_Text_Buffer *buf = textD->buffer;
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
// TODO:       horizOffset = std::max(0, textD->horizOffset - textD->width);
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
// TODO:       pos = BufCountForwardDispChars(buf, lineStartPos, std::max(0, indent - textD->width / maxCharWidth));
// TODO:       TextDSetInsertPosition(textD, pos);
// TODO:       TextDSetScroll(textD, textD->topLineNum, std::max(0, textD->horizOffset - textD->width));
// TODO:       checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO:       checkAutoShowInsertPos(w);
// TODO:       callCursorMovementCBs(w, event);
// TODO:    }
// TODO: }
// TODO: 
// TODO: static void pageRightAP(Fl_Widget* w, int, const char** args, int* nArgs)
// TODO: {
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
// TODO:    Ne_Text_Buffer *buf = textD->buffer;
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
// TODO:       XtVaGetValues(textD->hScrollBar, XmNmaximum, &sliderMax, XmNsliderSize, &sliderSize, NULL);
// TODO:       horizOffset = std::min(textD->horizOffset + textD->width, sliderMax - sliderSize);
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
// TODO:       pos = BufCountForwardDispChars(buf, lineStartPos, indent + textD->width / maxCharWidth);
// TODO:       TextDSetInsertPosition(textD, pos);
// TODO:       TextDSetScroll(textD, textD->topLineNum, textD->horizOffset + textD->width);
// TODO:       if (textD->horizOffset == oldHorizOffset && insertPos == pos)
// TODO:          ringIfNecessary(silent);
// TODO:       checkMoveSelectionChange(w, event, insertPos, args, nArgs);
// TODO:       checkAutoShowInsertPos(w);
// TODO:       callCursorMovementCBs(w, event);
// TODO:    }
// TODO: }

static void toggleOverstrikeAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;

   if (textD->isOverstrike)
   {
      textD->isOverstrike = false;
      textD->setCursorStyle(textD->heavyCursor ? NE_HEAVY_CURSOR : NE_NORMAL_CURSOR);
   }
   else
   {
      textD->isOverstrike = true;
      if ( textD->cursorStyle == NE_NORMAL_CURSOR || textD->cursorStyle == NE_HEAVY_CURSOR)
         textD->setCursorStyle(NE_BLOCK_CURSOR);
   }
}


// TODO: static void scrollUpAP(Fl_Widget* w, int event, const char**args, int*nArgs)
// TODO: {
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
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
// TODO: static void scrollDownAP(Fl_Widget* w, int event, const char**args, int*nArgs)
// TODO: {
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
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
// TODO: static void scrollLeftAP(Fl_Widget* w, int event, const char**args, int*nArgs)
// TODO: {
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
// TODO:    int horizOffset, nPixels;
// TODO:    int sliderMax, sliderSize;
// TODO: 
// TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &nPixels) != 1)
// TODO:       return;
// TODO:    XtVaGetValues(textD->hScrollBar, XmNmaximum, &sliderMax,
// TODO:                  XmNsliderSize, &sliderSize, NULL);
// TODO:    horizOffset = std::min(std::max(0, textD->horizOffset - nPixels), sliderMax - sliderSize);
// TODO:    if (textD->horizOffset != horizOffset)
// TODO:    {
// TODO:       TextDSetScroll(textD, textD->topLineNum, horizOffset);
// TODO:    }
// TODO: }
// TODO: 
// TODO: static void scrollRightAP(Fl_Widget* w, int event, const char**args, int*nArgs)
// TODO: {
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
// TODO:    int horizOffset, nPixels;
// TODO:    int sliderMax, sliderSize;
// TODO: 
// TODO:    if (*nArgs == 0 || sscanf(args[0], "%d", &nPixels) != 1)
// TODO:       return;
// TODO:    XtVaGetValues(textD->hScrollBar, XmNmaximum, &sliderMax,
// TODO:                  XmNsliderSize, &sliderSize, NULL);
// TODO:    horizOffset = std::min(std::max(0, textD->horizOffset + nPixels), sliderMax - sliderSize);
// TODO:    if (textD->horizOffset != horizOffset)
// TODO:    {
// TODO:       TextDSetScroll(textD, textD->topLineNum, horizOffset);
// TODO:    }
// TODO: }

static void scrollToLineAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   int topLineNum, horizOffset, lineNum;

   if (*nArgs == 0 || sscanf(args[0], "%d", &lineNum) != 1)
      return;
   textD->getScroll(&topLineNum, &horizOffset);
   textD->setScroll(lineNum, horizOffset);
}

static void selectAllAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer *buf = textD->buffer;

   textD->cancelDrag();
   buf->select(0, buf->size());
}

static void deselectAllAP(Fl_Widget* w, int event, const char**args, int*nArgs)
{
   Ne_Text_Editor* textD = (Ne_Text_Editor*)w;
   Ne_Text_Buffer *buf = textD->buffer;

   textD->cancelDrag();
   buf->unselect();
}

// TODO: /*
// TODO: **  Called on the Intrinsic FocusIn event.
// TODO: **
// TODO: **  Note that the widget has no internal state about the focus, ie. it does
// TODO: **  not know whether it has the focus or not.
// TODO: */
// TODO: static void focusInAP(Fl_Widget* widget, XEvent* event, String* unused1, int* unused2)
// TODO: {
// TODO:    TextWidget textwidget = (TextWidget) widget;
// TODO:    textDisp* textD = textwidget->text.textD;
// TODO: 
// TODO:    /* I don't entirely understand the traversal mechanism in Motif widgets,
// TODO:       particularly, what leads to this widget getting a focus-in event when
// TODO:       it does not actually have the input focus.  The temporary solution is
// TODO:       to do the comparison below, and not show the cursor when Motif says
// TODO:       we don't have focus, but keep looking for the real answer */
// TODO: #if XmVersion >= 1002
// TODO:    if (widget != XmGetFocusWidget(widget))
// TODO:       return;
// TODO: #endif
// TODO: 
// TODO:    /* If the timer is not already started, start it */
// TODO:    if (textwidget->text.cursorBlinkRate != 0
// TODO:          && textwidget->text.cursorBlinkProcID == 0)
// TODO:    {
// TODO:       textwidget->text.cursorBlinkProcID
// TODO:          = XtAppAddTimeOut(XtWidgetToApplicationContext(widget),
// TODO:                            textwidget->text.cursorBlinkRate, cursorBlinkTimerProc,
// TODO:                            widget);
// TODO:    }
// TODO: 
// TODO:    /* Change the cursor to active style */
// TODO:    if (textwidget->text.overstrike)
// TODO:       TextDSetCursorStyle(textD, BLOCK_CURSOR);
// TODO:    else
// TODO:       TextDSetCursorStyle(textD, (textwidget->text.heavyCursor
// TODO:                                   ? HEAVY_CURSOR
// TODO:                                   : NORMAL_CURSOR));
// TODO:    TextDUnblankCursor(textD);
// TODO: 
// TODO: #ifndef NO_XMIM
// TODO:    /* Notify Motif input manager that widget has focus */
// TODO:    XmImVaSetFocusValues(widget, NULL);
// TODO: #endif
// TODO: 
// TODO:    /* Call any registered focus-in callbacks */
// TODO:    XtCallCallbacks((Fl_Widget*) widget, textNfocusCallback, (XtPointer) event);
// TODO: }
// TODO: 
// TODO: static void focusOutAP(Fl_Widget* w, int, const char** args, int* nArgs)
// TODO: {
// TODO:    Ne_Text_Editor* textD = ((TextWidget)w)->text.textD;
// TODO: 
// TODO:    /* Remove the cursor blinking timer procedure */
// TODO:    if (((TextWidget)w)->text.cursorBlinkProcID != 0)
// TODO:       XtRemoveTimeOut(((TextWidget)w)->text.cursorBlinkProcID);
// TODO:    ((TextWidget)w)->text.cursorBlinkProcID = 0;
// TODO: 
// TODO:    /* Leave a dim or destination cursor */
// TODO:    TextDSetCursorStyle(textD, ((TextWidget)w)->text.motifDestOwner ? CARET_CURSOR : DIM_CURSOR);
// TODO:    TextDUnblankCursor(textD);
// TODO: 
// TODO:    /* If there's a calltip displayed, kill it. */
// TODO:    TextDKillCalltip(textD, 0);
// TODO: 
// TODO:    /* Call any registered focus-out callbacks */
// TODO:    XtCallCallbacks((Fl_Widget*)w, textNlosingFocusCallback, (XtPointer)event);
// TODO: }

// --------------------------------------------------------------------------
// For actions involving cursor movement, "extend" keyword means incorporate
// the new cursor position in the selection, and lack of an "extend" keyword
// means cancel the existing selection
// --------------------------------------------------------------------------
void Ne_Text_Editor::checkMoveSelectionChange(int event, int startPos, const char** args, int *nArgs)
{
   if (hasKey("extend", args, nArgs))
      keyMoveExtendSelection(event, startPos, hasKey("rect", args, nArgs));
   else
      buffer->unselect();
}

// --------------------------------------------------------------------------
// If a selection change was requested via a keyboard command for moving
// the insertion cursor (usually with the "extend" keyword), adjust the
// selection to include the new cursor position, or begin a new selection
// between startPos and the new cursor position with anchor at startPos.
// --------------------------------------------------------------------------
void Ne_Text_Editor::keyMoveExtendSelection(int event, int origPos, int rectangular)
{
// TODO:    XKeyEvent *e = &event->xkey;
   Ne_Text_Editor* textD = this;
   Ne_Text_Buffer* buf = textD->buffer;
   Ne_Text_Selection* sel = &buf->getSelection();
   int newPos = this->getInsertPosition();
   int startPos, endPos, startCol, endCol, newCol, origCol;
   int anchor, rectAnchor, anchorLineStart;

   // Moving the cursor does not take the Motif destination, but as soon as
   // the user selects something, grab it (I'm not sure if this distinction
   // actually makes sense, but it's what Motif was doing, back when their
   // secondary selections actually worked correctly)
// TODO:    TakeMotifDestination(w, e->time);

// TODO:    if (sel->selected && sel->rectangular && rectangular)
// TODO:    {
// TODO:       /* rect -> rect */
// TODO:       newCol = BufCountDispChars(buf, BufStartOfLine(buf, newPos), newPos);
// TODO:       startCol = std::min(tw->text.rectAnchor, newCol);
// TODO:       endCol   = std::max(tw->text.rectAnchor, newCol);
// TODO:       startPos = BufStartOfLine(buf, std::min(tw->text.anchor, newPos));
// TODO:       endPos = BufEndOfLine(buf, std::max(tw->text.anchor, newPos));
// TODO:       BufRectSelect(buf, startPos, endPos, startCol, endCol);
// TODO:    }
// TODO:    else if (sel->selected && rectangular)     /* plain -> rect */
// TODO:    {
// TODO:       newCol = BufCountDispChars(buf, BufStartOfLine(buf, newPos), newPos);
// TODO:       if (abs(newPos - sel->start) < abs(newPos - sel->end))
// TODO:          anchor = sel->end;
// TODO:       else
// TODO:          anchor = sel->start;
// TODO:       anchorLineStart = BufStartOfLine(buf, anchor);
// TODO:       rectAnchor = BufCountDispChars(buf, anchorLineStart, anchor);
// TODO:       tw->text.anchor = anchor;
// TODO:       tw->text.rectAnchor = rectAnchor;
// TODO:       BufRectSelect(buf, BufStartOfLine(buf, std::min(anchor, newPos)),
// TODO:                     BufEndOfLine(buf, std::max(anchor, newPos)),
// TODO:                     std::min(rectAnchor, newCol), std::max(rectAnchor, newCol));
// TODO:    }
// TODO:    else if (sel->selected && sel->rectangular)     /* rect -> plain */
// TODO:    {
// TODO:       startPos = BufCountForwardDispChars(buf, BufStartOfLine(buf, sel->start), sel->rectStart);
// TODO:       endPos = BufCountForwardDispChars(buf, BufStartOfLine(buf, sel->end), sel->rectEnd);
// TODO:       if (abs(origPos - startPos) < abs(origPos - endPos))
// TODO:          anchor = endPos;
// TODO:       else
// TODO:          anchor = startPos;
// TODO:       BufSelect(buf, anchor, newPos);
// TODO:    }
// TODO:    else 
   if (sel->selected)     /* plain -> plain */
   {
      if (abs(origPos - sel->start) < abs(origPos - sel->end))
         anchor = sel->end;
      else
         anchor = sel->start;
      buf->select(anchor, newPos);
   }
// TODO:    else if (rectangular)     /* no sel -> rect */
// TODO:    {
// TODO:       origCol = BufCountDispChars(buf, BufStartOfLine(buf, origPos), origPos);
// TODO:       newCol = BufCountDispChars(buf, BufStartOfLine(buf, newPos), newPos);
// TODO:       startCol = std::min(newCol, origCol);
// TODO:       endCol = std::max(newCol, origCol);
// TODO:       startPos = BufStartOfLine(buf, std::min(origPos, newPos));
// TODO:       endPos = BufEndOfLine(buf, std::max(origPos, newPos));
// TODO:       tw->text.anchor = origPos;
// TODO:       tw->text.rectAnchor = origCol;
// TODO:       BufRectSelect(buf, startPos, endPos, startCol, endCol);
// TODO:    }
   else     /* no sel -> plain */
   {
      this->anchor = origPos;
// TODO:       this->text.rectAnchor = BufCountDispChars(buf, BufStartOfLine(buf, origPos), origPos);
      buf->select(this->anchor, newPos);
   }
}

// --------------------------------------------------------------------------
void Ne_Text_Editor::checkAutoShowInsertPos()
{
   if (this->autoShowInsertPos)
      this->makeInsertPosVisible();
}

bool Ne_Text_Editor::checkReadOnly() const
{
   if (this->readOnly)
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
void Ne_Text_Editor::simpleInsertAtCursor(const char *chars, int event, int allowPendingDelete)
{
   Ne_Text_Buffer *buf = this->buffer;
   const char *c;

   if (allowPendingDelete && pendingSelection())
   {
      buf->replaceSelected(chars);
      this->setInsertPosition(buf->getCursorPosHint());
   }
   else if (this->isOverstrike)
   {
      for (c=chars; *c!='\0' && *c!='\n'; c++);
      if (*c == '\n')
         this->insert(chars);
      else
         overstrike(chars);
   }
   else
      this->insert(chars);
   checkAutoShowInsertPos();
   callCursorMovementCBs(event);
}

/*
** If there's a selection, delete it and position the cursor where the
** selection was deleted.  (Called by routines which do deletion to check
** first for and do possible selection delete)
*/
bool Ne_Text_Editor::deletePendingSelection(int event)
{
   Ne_Text_Buffer *buf = this->buffer;

   if (this->buffer->getSelection().selected)
   {
      buf->removeSelected();
      setInsertPosition(buf->getCursorPosHint());
      checkAutoShowInsertPos();
      callCursorMovementCBs(event);
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
bool Ne_Text_Editor::pendingSelection()
{
   Ne_Text_Selection* sel = &this->buffer->getSelection();
   int pos = this->getInsertPosition();

   return (this->pendingDelete && sel->selected && pos >= sel->start && pos <= sel->end);
}

/*
** Check if tab emulation is on and if there are emulated tabs before the
** cursor, and if so, delete an emulated tab as a unit.  Also finishes up
** by calling checkAutoShowInsertPos and callCursorMovementCBs, so the
** calling action proc can just return (this is necessary to preserve
** emTabsBeforeCursor which is otherwise cleared by callCursorMovementCBs).
*/
bool Ne_Text_Editor::deleteEmulatedTab(int event)
{
   Ne_Text_Buffer* buf = this->buffer;
   int emTabDist = this->emulateTabs;
   int emTabsBeforeCursor = this->emTabsBeforeCursor;
   int startIndent, toIndent, insertPos, startPos, lineStart;
   int pos, indent, startPosIndent;
   char c;

   if (emTabDist <= 0 || emTabsBeforeCursor <= 0)
      return false;

   // Find the position of the previous tab stop
   insertPos = this->getInsertPosition();
   lineStart = buf->startOfLine(insertPos);
   startIndent = buf->countDispChars(lineStart, insertPos);
   toIndent = (startIndent-1) - ((startIndent-1) % emTabDist);

   // Find the position at which to begin deleting (stop at non-whitespace characters)
   startPosIndent = indent = 0;
   startPos = lineStart;
   for (pos=lineStart; pos < insertPos; pos++)
   {
      c = buf->getCharacter(pos);
      indent += buf->charWidth(c, indent, buf->getTabDistance(), buf->getNullSubsChar());
      if (indent > toIndent)
         break;
      startPosIndent = indent;
      startPos = pos + 1;
   }

   /* Just to make sure, check that we're not deleting any non-white chars */
   for (pos=insertPos-1; pos>=startPos; pos--)
   {
      c = buf->getCharacter(pos);
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
      char* spaceString = new char[toIndent - startPosIndent + 1];
      memset(spaceString, ' ', toIndent-startPosIndent);
      spaceString[toIndent - startPosIndent] = '\0';
      buf->replace(startPos, insertPos, spaceString);
      setInsertPosition(startPos + toIndent - startPosIndent);
      delete[] spaceString;
   }
   else
   {
      buf->remove(startPos, insertPos);
      setInsertPosition(startPos);
   }

   // The normal cursor movement stuff would usually be called by the action
   // routine, but this wraps around it to restore emTabsBeforeCursor
   checkAutoShowInsertPos();
   callCursorMovementCBs(event);

   // Decrement and restore the marker for consecutive emulated tabs, which
   // would otherwise have been zeroed by callCursorMovementCBs
   this->emTabsBeforeCursor = emTabsBeforeCursor - 1;
   return true;
}

/*
** Select the word or whitespace adjacent to the cursor, and move the cursor
** to its end.  pointerX is used as a tie-breaker, when the cursor is at the
** boundary between a word and some white-space.  If the cursor is on the
** left, the word or space on the left is used.  If it's on the right, that
** is used instead.
*/
void Ne_Text_Editor::selectWord(int pointerX)
{
   Ne_Text_Buffer *buf = this->buffer;
   int x, y, insertPos = getInsertPosition();

   positionToXY(insertPos, &x, &y);
   if (pointerX < x && insertPos > 0 && buf->getCharacter(insertPos-1) != '\n')
      insertPos--;
   buf->select(startOfWord(insertPos), endOfWord(insertPos));
}

int Ne_Text_Editor::startOfWord(int pos)
{
   int startPos;
   Ne_Text_Buffer *buf = this->buffer;
   const std::string& delimiters = this->delimiters;
   char c = buf->getCharacter(pos);

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
      if (!buf->searchBackward(pos, delimiters.c_str(), &startPos))
         return 0;
   }
   return std::min(pos, startPos+1);
}

int Ne_Text_Editor::endOfWord(int pos)
{
   int endPos;
   Ne_Text_Buffer*buf = this->buffer;
   char c = buf->getCharacter(pos);

   if (c == ' ' || c== '\t')
   {
      if (!spanForward(buf, pos, " \t", false, &endPos))
         return buf->size();
   }
   else if (strchr(delimiters.c_str(), c))
   {
      if (!spanForward(buf, pos, delimiters.c_str(), true, &endPos))
         return buf->size();
   }
   else
   {
      if (!buf->searchForward(pos, delimiters.c_str(), &endPos))
         return buf->size();
   }
   return endPos;
}

/*
** Search forwards in buffer "buf" for the first character NOT in
** "searchChars",  starting with the character "startPos", and returning the
** result in "foundPos" returns True if found, False if not. If ignoreSpace
** is set, then Space, Tab, and Newlines are ignored in searchChars.
*/
bool Ne_Text_Editor::spanForward(Ne_Text_Buffer *buf, int startPos, const char *searchChars, int ignoreSpace, int *foundPos)
{
   int pos;
   const char *c;

   pos = startPos;
   while (pos < buf->size())
   {
      for (c=searchChars; *c!='\0'; c++)
         if(!(ignoreSpace && (*c==' ' || *c=='\t' || *c=='\n')))
            if (buf->getCharacter(pos) == *c)
               break;
      if(*c == 0)
      {
         *foundPos = pos;
         return true;
      }
      pos++;
   }
   *foundPos = buf->size();
   return false;
}

/*
** Search backwards in buffer "buf" for the first character NOT in
** "searchChars",  starting with the character BEFORE "startPos", returning the
** result in "foundPos" returns True if found, False if not. If ignoreSpace is
** set, then Space, Tab, and Newlines are ignored in searchChars.
*/
bool Ne_Text_Editor::spanBackward(Ne_Text_Buffer *buf, int startPos, const char *searchChars, int ignoreSpace, int *foundPos)
{
   int pos;
   const char *c;

   if (startPos == 0)
   {
      *foundPos = 0;
      return false;
   }
   pos = startPos == 0 ? 0 : startPos - 1;
   while (pos >= 0)
   {
      for (c=searchChars; *c!='\0'; c++)
         if(!(ignoreSpace && (*c==' ' || *c=='\t' || *c=='\n')))
            if (buf->getCharacter(pos) == *c)
               break;
      if(*c == 0)
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
void Ne_Text_Editor::selectLine()
{
   int insertPos = getInsertPosition();
   int endPos, startPos;

   endPos = buffer->endOfLine(insertPos);
   startPos = buffer->startOfLine(insertPos);
   buffer->select(startPos, std::min(endPos + 1, buffer->size()));
   setInsertPosition(endPos);
}

/*
** Given a new mouse pointer location, pass the position on to the
** autoscroll timer routine, and make sure the timer is on when it's
** needed and off when it's not.
*/
void Ne_Text_Editor::checkAutoScroll(int x, int y)
{
   // Is the pointer in or out of the text area?
   bool inWindow= (x >= this->left &&
              x < this->width - this->marginWidth &&
              y >= this->marginHeight &&
              y < this->height - this->marginHeight);

   // If it's in the window, cancel the timer procedure
   if (inWindow)
   {
// TODO:       if (w->text.autoScrollProcID != 0)
// TODO:          XtRemoveTimeOut(w->text.autoScrollProcID);;
// TODO:       w->text.autoScrollProcID = 0;
      return;
   }

   /* If the timer is not already started, start it */
   //if (w->text.autoScrollProcID == 0)
   //{
   //   w->text.autoScrollProcID = XtAppAddTimeOut(
   //                                 XtWidgetToApplicationContext((Fl_Widget*)w),
   //                                 0, autoScrollTimerProc, w);
   //}

   // Pass on the newest mouse location to the autoscroll routine
   this->mouseX = x;
   this->mouseY = y;
}

/*
** Reset drag state and cancel the auto-scroll timer
*/
void Ne_Text_Editor::endDrag()
{
// TODO:    if (((TextWidget)w)->text.autoScrollProcID != 0)
// TODO:       XtRemoveTimeOut(((TextWidget)w)->text.autoScrollProcID);
// TODO:    ((TextWidget)w)->text.autoScrollProcID = 0;
   if (this->dragState == NE_MOUSE_PAN)
      fl_cursor(FL_CURSOR_DEFAULT);
   this->dragState = NE_NOT_CLICKED;
}

// --------------------------------------------------------------------------
// Cancel any drag operation that might be in progress.  Should be included
// in nearly every key event to cleanly end any dragging before edits are made
// which might change the insert position or the content of the buffer during
// a drag operation)
// --------------------------------------------------------------------------
void Ne_Text_Editor::cancelDrag()
{
// TODO:    int dragState = ((TextWidget)w)->text.dragState;
// TODO: 
// TODO:    if (((TextWidget)w)->text.autoScrollProcID != 0)
// TODO:       XtRemoveTimeOut(((TextWidget)w)->text.autoScrollProcID);
// TODO:    if (dragState == SECONDARY_DRAG || dragState == SECONDARY_RECT_DRAG)
// TODO:       BufSecondaryUnselect(((TextWidget)w)->text.textD->buffer);
// TODO:    if (dragState == PRIMARY_BLOCK_DRAG)
// TODO:       CancelBlockDrag((TextWidget)w);
// TODO:    if (dragState == MOUSE_PAN)
// TODO:       XUngrabPointer(XtDisplay(w), CurrentTime);
// TODO:    if (dragState != NOT_CLICKED)
// TODO:       ((TextWidget)w)->text.dragState = DRAG_CANCELED;
}

// --------------------------------------------------------------------------
// Do operations triggered by cursor movement: Call cursor movement callback
// procedure(s), and cancel marker indicating that the cursor is after one or
// more just-entered emulated tabs (spaces to be deleted as a unit).
// --------------------------------------------------------------------------
void Ne_Text_Editor::callCursorMovementCBs(int event)
{
   this->emTabsBeforeCursor = 0;
// TODO:    XtCallCallbacks((Fl_Widget*)w, textNcursorMovementCallback, (XtPointer)event);
}

/*
** Adjust the selection as the mouse is dragged to position: (x, y).
*/
void Ne_Text_Editor::adjustSelection(int x, int y)
{
   Ne_Text_Buffer *buf = this->buffer;
   int row, col, startCol, endCol, startPos, endPos;
   int newPos = XYToPosition(x, y);

   // Adjust the selection
   if (this->multiClickState == NE_ONE_CLICK)
   {
      startPos = startOfWord(std::min(this->anchor, newPos));
      endPos = endOfWord(std::max(this->anchor, newPos));
      buf->select(startPos, endPos);
      newPos = newPos < this->anchor ? startPos : endPos;
   }
   else if (this->multiClickState == NE_TWO_CLICKS)
   {
      startPos = buf->startOfLine(std::min(this->anchor, newPos));
      endPos = buf->endOfLine(std::max(this->anchor, newPos));
      buf->select(startPos, std::min(endPos+1, buf->size()));
      newPos = newPos < this->anchor ? startPos : endPos;
   }
   else
      buf->select(this->anchor, newPos);

   // Move the cursor
   setInsertPosition(newPos);
   callCursorMovementCBs(0);
}

void Ne_Text_Editor::adjustSecondarySelection(int x, int y)
{
   int row, col, startCol, endCol, startPos, endPos;
   int newPos = XYToPosition(x, y);

   buffer->secondarySelect(this->anchor, newPos);
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
std::string Ne_Text_Editor::wrapText(const char *startLine, const char *text, int bufOffset, int wrapMargin, int *breakBefore)
{
   Ne_Text_Buffer* buf = this->buffer;
   int startLineLen = strlen(startLine);
   int colNum, pos, lineStartPos, limitPos, breakAt, charsAdded;
   int firstBreak = -1, tabDist = buf->getTabDistance();
   char c;

   // Create a temporary text buffer and load it with the strings
   Ne_Text_Buffer*wrapBuf = new Ne_Text_Buffer();
   wrapBuf->insertAt(0, startLine);
   wrapBuf->insertAt(wrapBuf->size(), text);

   // Scan the buffer for long lines and apply wrapLine when wrapMargin is
   // exceeded.  limitPos enforces no breaks in the "startLine" part of the
   // string (if requested), and prevents re-scanning of long unbreakable
   // lines for each character beyond the margin
   colNum = 0;
   pos = 0;
   lineStartPos = 0;
   limitPos = breakBefore == NULL ? startLineLen : 0;
   while (pos < wrapBuf->size())
   {
      c = wrapBuf->getCharacter(pos);
      if (c == '\n')
      {
         lineStartPos = limitPos = pos + 1;
         colNum = 0;
      }
      else
      {
         colNum += buf->charWidth(c, colNum, tabDist, buf->getNullSubsChar());
         if (colNum > wrapMargin)
         {
            if (!wrapLine(wrapBuf, bufOffset, lineStartPos, pos, limitPos, &breakAt, &charsAdded))
            {
               limitPos = std::max(pos, limitPos);
            }
            else
            {
               lineStartPos = limitPos = breakAt+1;
               pos += charsAdded;
               colNum = wrapBuf->countDispChars(lineStartPos, pos+1);
               if (firstBreak == -1)
                  firstBreak = breakAt;
            }
         }
      }
      pos++;
   }

   // Return the wrapped text, possibly including part of startLine
   std::string wrappedText;
   if (breakBefore == NULL)
      wrappedText = wrapBuf->getRange(startLineLen, wrapBuf->size());
   else
   {
      *breakBefore = firstBreak != -1 && firstBreak < startLineLen ? startLineLen - firstBreak : 0;
      wrappedText = wrapBuf->getRange(startLineLen - *breakBefore, wrapBuf->size());
   }
   delete wrapBuf;
   return wrappedText;
}

/*
** Wraps the end of a line beginning at lineStartPos and ending at lineEndPos
** in "buf", at the last white-space on the line >= limitPos.  (The implicit
** assumption is that just the last character of the line exceeds the wrap
** margin, and anywhere on the line we can wrap is correct).  Returns False if
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
bool Ne_Text_Editor::wrapLine(Ne_Text_Buffer *buf, int bufOffset,
                    int lineStartPos, int lineEndPos, int limitPos, int *breakAt,
                    int *charsAdded)
{
   int p, length, column;
   char c;
   std::string indentStr;

   /* Scan backward for whitespace or BOL.  If BOL, return False, no
      whitespace in line at which to wrap */
   for (p=lineEndPos; ; p--)
   {
      if (p < lineStartPos || p < limitPos)
         return false;
      c = buf->getCharacter(p);
      if (c == '\t' || c == ' ')
         break;
   }

   /* Create an auto-indent string to insert to do wrap.  If the auto
      indent string reaches the wrap position, slice the auto-indent
      back off and return to the left margin */
   if (this->autoIndent || this->smartIndent)
   {
      indentStr = createIndentString(buf, bufOffset, lineStartPos, lineEndPos, &length, &column);
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
   buf->replace(p, p+1, indentStr.c_str());
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
char* Ne_Text_Editor::createIndentString(Ne_Text_Buffer *buf, int bufOffset, int lineStartPos, int lineEndPos, int *length, int *column)
{
   int pos, indent = -1, tabDist = this->buffer->getTabDistance();
   int i;
   char *indentPtr, *indentStr, c;
   NeSmartIndentCBStruct smartIndent;

   /* If smart indent is on, call the smart indent callback.  It is not
      called when multi-line changes are being made (lineStartPos != 0),
      because smart indent needs to search back an indeterminate distance
      through the buffer, and reconciling that with wrapping changes made,
      but not yet committed in the buffer, would make programming smart
      indent more difficult for users and make everything more complicated */
   if (this->smartIndent && (lineStartPos == 0 || buf == this->buffer))
   {
      smartIndent.reason = NE_NEWLINE_INDENT_NEEDED;
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
         c =  buf->getCharacter(pos);
         if (c != ' ' && c != '\t')
            break;
         if (c == '\t')
            indent += tabDist - (indent % tabDist);
         else
            indent++;
      }
   }

   /* Allocate and create a string of tabs and spaces to achieve the indent */
   indentPtr = indentStr = new char[indent + 2];
   *indentPtr++ = '\n';
   for (i=0; i < indent; i++)
      *indentPtr++ = ' ';
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
// TODO: static void autoScrollTimerProc(XtPointer clientData, XtIntervalId *id)
// TODO: {
// TODO:    TextWidget w = (TextWidget)clientData;
// TODO:    Ne_Text_Editor* textD = w->text.textD;
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
// TODO:                                  (NE_VERTICAL_SCROLL_DELAY*fontWidth) / fontHeight :
// TODO:                                  NE_VERTICAL_SCROLL_DELAY, autoScrollTimerProc, w);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Xt timer procedure for cursor blinking
// TODO: */
// TODO: static void cursorBlinkTimerProc(XtPointer clientData, XtIntervalId *id)
// TODO: {
// TODO:    TextWidget w = (TextWidget)clientData;
// TODO:    Ne_Text_Editor* textD = w->text.textD;
// TODO: 
// TODO:    /* Blink the cursor */
// TODO:    if (textD->cursorOn)
// TODO:       TextDBlankCursor(textD);
// TODO:    else
// TODO:       TextDUnblankCursor(textD);
// TODO: 
// TODO:    /* re-establish the timer proc (this routine) to continue processing */
// TODO:    w->text.cursorBlinkProcID = XtAppAddTimeOut(
// TODO:                                   XtWidgetToApplicationContext((Fl_Widget*)w),
// TODO:                                   w->text.cursorBlinkRate, cursorBlinkTimerProc, w);
// TODO: }
// TODO: 
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

// --------------------------------------------------------------------------
bool Ne_Text_Editor::inWindow(int x, int y)
{
   return ((x >= this->x() && x <= this->x() + this->width + this->lineNumWidth + 2*marginWidth)
      && (y >= this->top && y <= this->top + this->height));
}

// --------------------------------------------------------------------------
bool Ne_Text_Editor::mouseMoveForDrag(int x, int y)
{
   return (abs(x - this->btnDownX) > NE_SELECT_THRESHOLD || abs(y - this->btnDownY) > NE_SELECT_THRESHOLD);
}

// --------------------------------------------------------------------------
// look at an action procedure's arguments to see if argument "key" has been
// specified in the argument list
// --------------------------------------------------------------------------
static bool hasKey(const char*key, const char** args, const int *nArgs)
{
   int i;

   for (i=0; i<(int)*nArgs; i++)
      if (!strCaseCmp(args[i], key))
         return true;
   return false;
}

// --------------------------------------------------------------------------
// strCaseCmp compares its arguments and returns 0 if the two strings
// are equal IGNORING case differences.  Otherwise returns 1.
// --------------------------------------------------------------------------
static int strCaseCmp(const char *str1, const char *str2)
{
   unsigned const char *c1 = (unsigned const char*) str1;
   unsigned const char *c2 = (unsigned const char*) str2;

   for (; *c1!='\0' && *c2!='\0'; c1++, c2++)
      if (toupper(*c1) != toupper(*c2))
         return 1;
   if (*c1 == *c2)
      return 0;

   return 1;
}

static void ringIfNecessary(bool silent)
{
   if (!silent)
      fl_beep();
}

