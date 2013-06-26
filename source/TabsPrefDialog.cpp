#include "TabsPrefDialog.h"

#include "preferences.h"
#include "window.h"
#include "help.h"
#include "../util/DialogF.h"

#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Int_Input.H>

// Module-global variables for Tabs dialog
static WindowInfo* TabsDialogForWindow;
static Fl_Input* TabDistText, *EmTabText;
static Fl_Button* EmTabToggle, *UseTabsToggle;

static void tabsOKCB(Fl_Widget* w, void* data);
static void tabsCancelCB(Fl_Widget* w, void* data);
static void tabsHelpCB(Fl_Widget* w, void* data);
static void emTabsCB(Fl_Widget* w, void* data);

/*
** Present the user a dialog for setting tab related preferences, either as
** defaults, or for a specific window (pass "forWindow" as NULL to set default
** preference, or a window to set preferences for the specific window.
*/
void TabsPrefDialog(Fl_Widget* parent, WindowInfo* forWindow)
{
   const int BorderSpace = 5;
   const int HeightByLine = 30;
   const int ButtonLineHeight = 50;
   const int ButtonWidth = 80;
   const int ButtonHeight = 25;

   Fl_Window dialog(0, 0, 350, BorderSpace + 3 * HeightByLine + ButtonLineHeight, "Tabs");
   Fl_Group promptGroup(0, 0, dialog.w(), dialog.h() - ButtonLineHeight);

   TabDistText = new Fl_Int_Input(290, BorderSpace, 50, 25, "&Tab spacing (for hardware tab characters)");
   EmTabText = new Fl_Int_Input(290, BorderSpace * 2 + 25, 50, 25, "Emulated tab &spacing");
   EmTabToggle = new Fl_Check_Button(BorderSpace, BorderSpace * 2 + 25, 100, 25, "&Emulate tabs");
   EmTabToggle->callback(emTabsCB);
   UseTabsToggle = new Fl_Check_Button(BorderSpace, BorderSpace * 3 + 25 * 2, 350, 25, "&Use tab characters in padding and emulated tabs");

   Fl_Group promptResizable(0, promptGroup.h() - 1, dialog.w(), 1);
   promptResizable.end();
   promptGroup.resizable(&promptResizable);
   promptGroup.end();

   Fl_Group buttonLine(0, promptGroup.h(), dialog.w(), ButtonLineHeight);
   buttonLine.box(FL_ENGRAVED_FRAME);

   Fl_Button btnOk(BorderSpace, buttonLine.y() + BorderSpace * 2, ButtonWidth, ButtonHeight, "Ok");
   btnOk.shortcut(FL_Enter);
   btnOk.callback(tabsOKCB);

   Fl_Button btnCancel(btnOk.x() + btnOk.w() + BorderSpace, btnOk.y(), ButtonWidth, ButtonHeight, "Cancel");
   btnCancel.shortcut(FL_Escape);
   btnCancel.callback(tabsCancelCB);

   Fl_Button btnHelp(btnCancel.x() + btnCancel.w() + BorderSpace, btnOk.y(), ButtonWidth, ButtonHeight, "Help");
   btnHelp.callback(tabsHelpCB);

   buttonLine.end();

   dialog.focus(&btnOk);
   dialog.resizable(promptGroup);
   dialog.end();


   // Set default values
   int emTabDist = GetPrefEmTabDist(PLAIN_LANGUAGE_MODE);
   int useTabs = GetPrefInsertTabs();
   int tabDist = GetPrefTabDist(PLAIN_LANGUAGE_MODE);

   if (forWindow != NULL)
   {  // Set value for a specific window
      // TODO:       XtVaGetValues(forWindow->textArea, textNemulateTabs, &emTabDist, NULL);
// TODO:       useTabs = forWindow->buffer->useTabs;
// TODO:       tabDist = BufGetTabDistance(forWindow->buffer);
   }
   bool emulate = (emTabDist != 0);
   SetIntText(TabDistText, tabDist);
   NeToggleButtonSetState(EmTabToggle, emulate);
   if (emulate)
      SetIntText(EmTabText, emTabDist);
   NeToggleButtonSetState(UseTabsToggle, useTabs != 0);
   NeSetSensitive(EmTabText, emulate);

   // put up dialog and wait for user to press ok or cancel
   TabsDialogForWindow = forWindow;
   ManageDialogCenteredOnPointer(&dialog);
   
   dialog.set_modal();
   dialog.show();
   while (dialog.shown()) Fl::wait();
}

static void tabsOKCB(Fl_Widget* w, void* data)
{
   TRACE();
   int tabDist, emTabDist;
   WindowInfo* window = TabsDialogForWindow;

   // get the values that the user entered and make sure they're ok
   bool emulate = NeToggleButtonGetState(EmTabToggle);
   bool useTabs = NeToggleButtonGetState(UseTabsToggle);
   int stat = GetIntTextWarn(TabDistText, &tabDist, "tab spacing", true);
   if (stat != TEXT_READ_OK)
      return;

   if (tabDist <= 0 || tabDist > MAX_EXP_CHAR_LEN)
   {
      DialogF(DF_WARN, TabDistText, 1, "Tab Spacing", "Tab spacing out of range", "OK");
      return;
   }

   if (emulate)
   {
      stat = GetIntTextWarn(EmTabText, &emTabDist, "emulated tab spacing",true);
      if (stat != TEXT_READ_OK)
         return;

      if (emTabDist <= 0 || tabDist >= 1000)
      {
         DialogF(DF_WARN, EmTabText, 1, "Tab Spacing", "Emulated tab spacing out of range", "OK");
         return;
      }
   }
   else
      emTabDist = 0;

   // Set the value in either the requested window or default preferences
   if (TabsDialogForWindow == NULL)
   {
      SetPrefTabDist(tabDist);
      SetPrefEmTabDist(emTabDist);
      SetPrefInsertTabs(useTabs);
   }
   else
   {
      char* params[1];
      char numStr[25];
   // TODO:
      params[0] = numStr;
      sprintf(numStr, "%d", tabDist);
// TODO:       XtCallActionProc(window->textArea, "set_tab_dist", NULL, params, 1);
      params[0] = numStr;
      sprintf(numStr, "%d", emTabDist);
// TODO:       XtCallActionProc(window->textArea, "set_em_tab_dist", NULL, params, 1);
      params[0] = numStr;
      sprintf(numStr, "%d", useTabs);
// TODO:       XtCallActionProc(window->textArea, "set_use_tabs", NULL, params, 1);
   }
   WidgetToMainWindow(w)->hide();
}

static void tabsCancelCB(Fl_Widget* w, void* data)
{
   TRACE();
   WidgetToMainWindow(w)->hide();
}

static void tabsHelpCB(Fl_Widget* w, void* data)
{
   TRACE();
   Help(HELP_TABS_DIALOG);
}

static void emTabsCB(Fl_Widget* w, void* data)
{
   int state = NeToggleButtonGetState(dynamic_cast<Fl_Button*>(w));  
   NeSetSensitive(EmTabText, state != 0);
}

