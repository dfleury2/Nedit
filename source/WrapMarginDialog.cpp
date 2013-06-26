#include "WrapMarginDialog.h"

#include "preferences.h"
#include "window.h"
#include "../util/DialogF.h"

#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Int_Input.H>

// Module-global variables for Wrap Margin dialog
static WindowInfo* WrapDialogForWindow;
static Fl_Input* WrapText;
static Fl_Button* WrapWindowToggle;

static void wrapOKCB(Fl_Widget* w, void* data);
static void wrapCancelCB(Fl_Widget* w, void* data);
static void wrapWindowCB(Fl_Widget* w, void* data);

/*
** Present the user a dialog for setting wrap margin.
*/
void WrapMarginDialog(Fl_Widget* parent, WindowInfo* forWindow)
{
   Fl_Window dialog(0, 0, 250, 150, "Wrap Margin");
   Fl_Group promptGroup(0, 0, 250, 100);

   WrapWindowToggle = new Fl_Check_Button(10, 10, 240, 25, "&Wrap and Fill at width of window");
   WrapWindowToggle->callback(wrapWindowCB, 0);

   WrapText = new Fl_Int_Input(190, 40, 50, 25, "&Margin for Wrap and Fill");

   Fl_Group promptResizable(0, 99, 250, 1);
   promptResizable.end();
   promptGroup.resizable(&promptResizable);
   promptGroup.end();

   Fl_Group buttonLine(0, 100, 250, 50);
   buttonLine.box(FL_ENGRAVED_FRAME);
   
   Fl_Button btnOk(10, 110, 80, 25, "Ok");
   btnOk.shortcut(FL_Enter);
   btnOk.callback(wrapOKCB);

   Fl_Button btnCancel(160, 110, 80, 25, "Cancel");
   btnCancel.shortcut(FL_Escape);
   btnCancel.callback(wrapCancelCB);

   buttonLine.end();

   dialog.focus(&btnOk);
   dialog.resizable(promptGroup);
   dialog.end();

   // Set default value
   int margin = 0;
   if (forWindow == NULL)
      margin = GetPrefWrapMargin();
   else
      ; // TODO: XtVaGetValues(forWindow->textArea, textNwrapMargin, &margin, NULL);
   NeToggleButtonSetState(WrapWindowToggle, margin==0);

   if (margin != 0)
      SetIntText(WrapText, margin);
   NeSetSensitive(WrapText, margin!=0);

   // put up dialog and wait for user to press ok or cancel
   WrapDialogForWindow = forWindow;
   ManageDialogCenteredOnPointer(&dialog);

   dialog.set_modal();
   dialog.show();
   while (dialog.shown()) Fl::wait();
}

static void wrapOKCB(Fl_Widget* w, void* data)
{
   TRACE();
   WindowInfo* window = WrapDialogForWindow;

   // get the values that the user entered and make sure they're ok
   int margin;
   int wrapAtWindow = NeToggleButtonGetState(WrapWindowToggle);
   if (wrapAtWindow)
      margin = 0;
   else
   {
      int stat = GetIntTextWarn(WrapText, &margin, "wrap Margin", true);
      if (stat != TEXT_READ_OK)
         return;

      if (margin <= 0 || margin >= 1000)
      {
         DialogF(DF_WARN, WrapText, 1, "Wrap Margin", "Wrap margin out of range", "OK");
         return;
      }

   }

   // Set the value in either the requested window or default preferences
   if (WrapDialogForWindow == NULL)
      SetPrefWrapMargin(margin);
   else
   {
      char* params[1];
      char marginStr[25];
      sprintf(marginStr, "%d", margin);
      params[0] = marginStr;
      // TODO: XtCallActionProc(window->textArea, "set_wrap_margin", NULL, params, 1);
   }
   WidgetToMainWindow(w)->hide();
}

static void wrapCancelCB(Fl_Widget* w, void* data)
{
   TRACE();
   WidgetToMainWindow(w)->hide();
}

static void wrapWindowCB(Fl_Widget* w, void* data)
{
   TRACE();
   int wrapAtWindow = NeToggleButtonGetState(dynamic_cast<Fl_Button*>(w));
   NeSetSensitive(WrapText, !wrapAtWindow);
}
