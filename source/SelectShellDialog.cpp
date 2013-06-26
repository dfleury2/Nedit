#include "SelectShellDialog.h"

#include "preferences.h"
#include "window.h"
#include "../util/DialogF.h"

static void shellSelOKCB(Fl_Widget* widget, void* data);
static void shellSelCancelCB(Fl_Widget* widgget, void* data);

/*
**  Create and show a dialog for selecting the shell
*/
void SelectShellDialog(Fl_Widget* parent, WindowInfo* forWindow)
{
   Fl_Window dialog(0, 0, 250, 110, "Command Shell");
   Fl_Group promptGroup(0, 0, 250, 30*2);

   Fl_Input shellText(5, 30, 240, 25, "Enter shell path:");
   shellText.align(FL_ALIGN_TOP_LEFT);

   Fl_Group promptResizable(0, 59, 250, 1);
   promptResizable.end();
   promptGroup.resizable(&promptResizable);
   promptGroup.end();

   Fl_Group buttonLine(0, 60, 250, 50);
   buttonLine.box(FL_ENGRAVED_FRAME);
   
   Fl_Button btnOk(10, 70, 80, 25, "Ok");
   btnOk.shortcut(FL_Enter);
   btnOk.callback(shellSelOKCB, &shellText);

   Fl_Button btnCancel(160, 70, 80, 25, "Cancel");
   btnCancel.shortcut(FL_Escape);
   btnCancel.callback(shellSelCancelCB);

   buttonLine.end();

   dialog.focus(&shellText);
   dialog.resizable(promptGroup);
   dialog.end();

   // Set dialog's text to the current setting.
   shellText.value(GetPrefShell());

   //  Show dialog and wait until the user made her choice.
   ManageDialogCenteredOnPointer(&dialog);

   dialog.set_modal();
   dialog.show();
   while (dialog.shown()) Fl::wait();
}

static void shellSelOKCB(Fl_Widget* w, void* data)
{
   TRACE();

   //  Get the string that the user entered and make sure it's ok.
   const char* shellName = static_cast<Fl_Input*>(data)->value();

   struct stat attribute;
   if (stat(shellName, &attribute) == -1)
   {
      unsigned dlgResult = DialogF(DF_WARN, w, 2, "Command Shell",
                          "The selected shell is not available.\nDo you want to use it anyway?",
                          "OK", "Cancel");
      if (1 != dlgResult)
      {
         return;
      }
   }

   SetPrefShell(shellName);

   WidgetToMainWindow(w)->hide();
}

static void shellSelCancelCB(Fl_Widget* w, void* data)
{
   TRACE();
   WidgetToMainWindow(w)->hide();
}
