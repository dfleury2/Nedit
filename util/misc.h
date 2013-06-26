/* $Id: misc.h,v 1.31 2010/07/05 06:23:59 lebert Exp $ */
/*******************************************************************************
*                                                                              *
* misc.h -- Nirvana Editor Miscellaneous Header File                           *
*                                                                              *
* Copyright 2004 The NEdit Developers                                          *
*                                                                              *
* This is free software; you can redistribute it and/or modify it under the    *
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

#ifndef NEDIT_MISC_H_INCLUDED
#define NEDIT_MISC_H_INCLUDED

#include "../util/Ne_Font.h"
#include "../util/Ne_Dimension.h"

class Ne_AppContext;

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>

#include <string.h>

#include <string>
#include <iostream>

#define TEXT_READ_OK 0
#define TEXT_IS_BLANK 1
#define TEXT_NOT_NUMBER 2

/* maximum length for a window geometry string */
#define MAX_GEOM_STRING_LEN 24

/* Maximum length for a menu accelerator string.
   Which e.g. can be parsed by misc.c:parseAccelString()
   (how many modifier keys can you hold down at once?) */
#define MAX_ACCEL_LEN 100

/*  button margin width to avoid cramped buttons  */
#define BUTTON_WIDTH_MARGIN 12


/* Motif compatibility mode for FLTK */
typedef char* NeString;

void NeStringFree(NeString str);
char* GetNeStringText( NeString str );
char* NeNewString(const char* str);

char* NeTextGetString(const Fl_Input*);
void NeTextSetString(Fl_Input*, const char* str, bool doCallback = false);

typedef void (*XtActionProc)(Fl_Widget* w, int e, const char** args, int* nArgs);
typedef void (*XtActionHookProc)(Fl_Widget* w, void* clientData, const char* actionName, int event, const char** params, int* numParams);

struct XtActionsRec {
  const char* name;
  XtActionProc proc;
}; 

typedef XtActionsRec* XtActionList;

void NeToggleButtonSetState(Fl_Button* w, bool state, bool doCallback = false);
bool NeToggleButtonGetState(Fl_Button* w);
void NeSetSensitive(Fl_Widget* w, bool state);
void NeSetSensitive(Fl_Menu_Item* item, bool state);
int NeStringToKeysym(const char*);
const char* NeKeysymToString(int);

#ifdef WIN32
typedef unsigned long ssize_t;
typedef int Atom;
typedef size_t uid_t;
typedef size_t gid_t;
typedef Fl_Widget** WidgetList;
#endif

double GetTimeOfDay(); // return the # of millisec since EPOC

inline std::string filename( const std::string& filepath)
{
   std::string::size_type pos = filepath.find_last_of('\\');
   if (pos==std::string::npos)
      return filepath;

   return std::string(filepath, pos+1);
}

#define TRACE() { std::cout << "[" << filename(__FILE__) << ":" << __LINE__ << "] " << __FUNCTION__ << std::endl; }


Fl_Group* WidgetToMainWindow(Fl_Widget* w);
// TODO: void AddMotifCloseCallback(Fl_Widget* shell, XtCallbackProc closeCB, void* arg);
// TODO: void PopDownBugPatch(Fl_Widget* w);
void SetDeleteRemap(int state);
// TODO: void RealizeWithoutForcingPosition(Fl_Widget* shell);
void RemovePPositionHint(Fl_Widget* shell);
void ManageDialogCenteredOnPointer(Fl_Widget* dialogChild);
void SetPointerCenteredDialogs(int state);
// TODO: void RaiseDialogWindow(Fl_Widget* shell);
void RaiseShellWindow(Fl_Widget* shell, bool focus);
// TODO: void RaiseWindow(Display* display, Window w, bool focus);
// TODO: void AddDialogMnemonicHandler(Fl_Widget* dialog, int unmodifiedToo);
// TODO: void RemoveDialogMnemonicHandler(Fl_Widget* dialog);
// TODO: void UpdateAccelLockPatch(Fl_Widget* topWidget, Fl_Widget* newButton);
// TODO: char* GetNeStringText(NeString fromString);
Ne_Font GetDefaultFontStruct(const Ne_Font& font);
// TODO: NeString* StringTable(int count, ...);
// TODO: void FreeStringTable(NeString* table);
// TODO: void SimulateButtonPress(Fl_Widget* widget);
// TODO: Fl_Widget* AddMenuItem(Fl_Widget* parent, char* name, char* label, char mnemonic,
// TODO:                    char* acc, char* accText, XtCallbackProc callback, void* cbArg);
// TODO: Fl_Widget* AddMenuToggle(Fl_Widget* parent, char* name, char* label, char mnemonic,
// TODO:                      char* acc, char* accText, XtCallbackProc callback, void* cbArg,int set);
// TODO: Fl_Widget* AddSubMenu(Fl_Widget* parent, char* name, char* label, char mnemonic);
void SetIntText(Fl_Input* text, int value);
// TODO: int GetFloatText(Fl_Widget* text, double* value);
int GetIntText(Fl_Input* text, int* value);
// TODO: int GetFloatTextWarn(Fl_Widget* text, double* value, const char* fieldName, int warnBlank);
int GetIntTextWarn(Fl_Input* text, int* value, const char* fieldName, int warnBlank);
bool TextWidgetIsBlank(Fl_Input* textW);
// TODO: void MakeSingleLineTextW(Fl_Widget* textW);
void BeginWait(Fl_Window* win);
// TODO: void BusyWait(Fl_Widget* anyWidget);
void EndWait(Fl_Window* win);
// TODO: void PasswordText(Fl_Widget* w, char* passTxt);
// TODO: void AddHistoryToTextWidget(Fl_Widget* textW, char** *historyList, int* nItems);
// TODO: void AddToHistoryList(char* newItem, char** *historyList, int* nItems);
// TODO: void CreateGeometryString(char* string, int x, int y,
// TODO:                           int width, int height, int bitmask);
// TODO: bool FindBestVisual(Display* display, const char* appName, const char* appClass,
// TODO:                        Visual** visual, int* depth, Colormap* colormap);
// TODO: Fl_Widget* CreateDialogShell(Fl_Widget* parent, char* name, ArgList arglist,
// TODO:                          Cardinal  argcount);
// TODO: Fl_Widget* CreatePopupMenu(Fl_Widget* parent, char* name, ArgList arglist,
// TODO:                        Cardinal argcount);
// TODO: Fl_Widget* CreatePulldownMenu(Fl_Widget* parent, char* name, ArgList arglist,
// TODO:                           Cardinal  argcount);
// TODO: Fl_Widget* CreatePromptDialog(Fl_Widget* parent, char* name); // TODO:, ArgList arglist, Cardinal  argcount);
// TODO: Fl_Widget* CreateSelectionDialog(Fl_Widget* parent, char* name, ArgList arglist,
// TODO:                              Cardinal  argcount);
// TODO: Fl_Widget* CreateFormDialog(Fl_Widget* parent, char* name, ArgList arglist,
// TODO:                         Cardinal  argcount);
// TODO: Fl_Widget* CreateFileSelectionDialog(Fl_Widget* parent, char* name, ArgList arglist,
// TODO:                                  Cardinal  argcount);
// TODO: Fl_Widget* CreateQuestionDialog(Fl_Widget* parent, char* name, ArgList arglist,
// TODO:                             Cardinal  argcount);
// TODO: Fl_Widget* CreateMessageDialog(Fl_Widget* parent, char* name, ArgList arglist,
// TODO:                            Cardinal  argcount);
// TODO: Fl_Widget* CreateErrorDialog(Fl_Widget* parent, char* name, ArgList arglist,
// TODO:                          Cardinal  argcount);
// TODO: Fl_Widget* CreateShellWithBestVis(String appName, String appClass,
// TODO:                               WidgetClass wClass, Display* display, ArgList args, Cardinal nArgs);
// TODO: Fl_Widget* CreatePopupShellWithBestVis(String shellName, WidgetClass wClass,
// TODO:                                    Fl_Widget* parent, ArgList arglist, Cardinal argcount);
// TODO: Fl_Widget* CreateWidget(Fl_Widget* parent, const char* name, WidgetClass wClass,
// TODO:                     ArgList arglist, Cardinal  argcount);
// TODO: Modifiers GetNumLockModMask(Display* display);
void InstallMouseWheelActions(Ne_AppContext& context);
// TODO: void AddMouseWheelSupport(Fl_Widget* w);
void NeRadioButtonChangeState(Fl_Button* widget, bool state, bool notify);
void CloseAllPopupsFor(Fl_Widget* shell);
// TODO: long QueryCurrentDesktop(Display* display, Window rootWindow);
// TODO: long QueryDesktop(Display* display, Fl_Widget* shell);
// TODO: int SpinClipboardStartCopy(Display* display, Window window,
// TODO:                            NeString clip_label, Time timestamp, Fl_Widget* widget,
// TODO:                            XmCutPasteProc callback, long* item_id);
// TODO: int SpinClipboardCopy(Display* display, Window window, long item_id,
// TODO:                       char* format_name, XtPointer buffer, unsigned long length,
// TODO:                       long private_id, long* data_id);
// TODO: int SpinClipboardEndCopy(Display* display, Window window, long item_id);
// TODO: int SpinClipboardInquireLength(Display* display, Window window,
// TODO:                                char* format_name, unsigned long* length);
// TODO: int SpinClipboardRetrieve(Display* display, Window window, char* format_name,
// TODO:                           XtPointer buffer, unsigned long length, unsigned long* num_bytes,
// TODO:                           long* private_id);
// TODO: int SpinClipboardLock(Display* display, Window window);
// TODO: int SpinClipboardUnlock(Display* display, Window window);
// TODO: void WmClientMsg(Display* disp, Window win, const char* msg,
// TODO:                  unsigned long data0, unsigned long data1,
// TODO:                  unsigned long data2, unsigned long data3,
// TODO:                  unsigned long data4);

#endif /* NEDIT_MISC_H_INCLUDED */
