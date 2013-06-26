static const char CVSID[] = "$Id: DialogF.c,v 1.32 2007/12/28 19:48:07 yooden Exp $";
/*******************************************************************************
*                                                                              *
* DialogF -- modal dialog printf routine                                       *
*                                                                              *
* Copyright (C) 1999 Mark Edel                                                 *
*                                                                              *
* This is free software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version. In addition, you may distribute version of this program linked to   *
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
* April 26, 1991                                                               *
*                                                                              *
* Written by Joy Kyriakopulos                                                  *
*                                                                              *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "DialogF.h"
#include "misc.h"
#include "Ne_Input.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

#include <string>
#include <cstdarg>
#include <iostream>
#include <vector>

using namespace std;

#define NUM_BUTTONS_SUPPORTED 3		/* except prompt dialog */
#define NUM_BUTTONS_MAXPROMPT 4
#define MAX_TITLE_LEN 256

// --------------------------------------------------------------------------
struct dfcallbackstruct
{
   dfcallbackstruct() : button(0) {}

   unsigned button; // button pressed by user
};

static char** PromptHistory = 0;
static int NPromptHistoryItems = -1;

// --------------------------------------------------------------------------
class Ne_DialogF : public Fl_Window
{
public:
   Ne_DialogF(int x, int y, int w, int h, const char* label, dfcallbackstruct& df)
      : Fl_Window(x, y, w, h, label), df_(df)
   {
      labelBox = new Fl_Box(10, 10, w-10, h - 50 );
      labelBox->align(FL_ALIGN_CENTER | FL_ALIGN_WRAP);

      buttonLine = new Fl_Group(0, labelBox->h(), w, 50);
      buttonLine->box(FL_ENGRAVED_FRAME);

      Fl_Group* leftResizeable = new Fl_Group(buttonLine->x(),buttonLine->y(), 1, buttonLine->h());
      
      buttonLine->resizable(leftResizeable);
      buttonLine->end();

      resizable(labelBox);
   }

   void add_button(const char* label, long value)
   {
      buttonLine->begin();
      // From right to Left
      Fl_Button* bt = new Fl_Button(w() - 10 - (buttons.size()+1) * 70, buttonLine->y() + 10, 60, 25);
      bt->copy_label(label);
      if (!strcmp(label, "OK"))
         bt->shortcut(FL_Enter);
      else if (!strcmp(label, "Cancel") || !strcmp(label, "Dismiss"))
         bt->shortcut(FL_Escape);
      bt->callback(ButtonCB, value);
      buttonLine->end();
      buttons.push_back(bt);
   }

   static void ButtonCB(Fl_Widget* w, long v)
   {
      Fl_Button* bt = dynamic_cast<Fl_Button*>(w);
      Ne_DialogF* dialog = WidgetToWindow(bt);
      dialog->df_.button = unsigned(v);
      dialog->hide();
   }

   static Ne_DialogF* WidgetToWindow(Fl_Widget* w)
   {
      do
      {
         if (!w->parent())
            return dynamic_cast<Ne_DialogF*>(w);
         w = w->parent();
      }
      while(w);
      return 0; // no window ?
   }

   Fl_Box* labelBox;
   Fl_Group* buttonLine;
   std::vector<Fl_Button*> buttons;

   dfcallbackstruct& df_;
};


/*******************************************************************************
* DialogF()                                                                    *
*                                                                              *
* function to put up modal versions of the XmCreate<type>Dialog functions      *
* (where <type> is Error, Information, Prompt, Question, Message, or Warning). *
* The purpose of this routine is to allow a printf-style dialog box to be      *
* invoked in-line without giving control back to the main loop. The message    *
* string can contain vsprintf specifications.                                  *
* DialogF displays the dialog in application-modal style, blocking the         *
* application and keeping the modal dialog as the top window until the user    *
* responds. DialogF accepts a variable number of arguments, so the calling     *
* routine needs to #include <stdarg.h>.                                        *
*                                                                              *
* The first button is automatically  marked as the default button              *
* (activated when the user types Return, surrounded by a special outline), and *
* any button named either Cancel, or Dismiss is marked as the cancel button    *
* (activated by the ESC key).                                                  *
* Buttons marked Dismiss or Cancel are also triggered by close of dialog via   *
* the window close box. If there's no Cancel or Dismiss button, button 1 is    *
* invoked when the close box is pressed.                                       *
*                                                                              *
* Arguments:                                                                   *
*                                                                              *
* unsigned dialog_type  dialog type (e.g. DF_ERR for error dialog, refer to    *
*                       DialogF.h for dialog type values)                      *
* Fl_Widget* parent         parent widget ID                                   *
* unsigned n            # of buttons to display; if set to 0, use defaults in  *
*                       XmCreate<type>Dialog; value in range 0 to              *
*                       NUM_BUTTONS_SUPPORTED (for prompt dialogs:             *
*                       NUM_BUTTONS_MAXPROMPT)                                 *
* char* title           dialog title                                           *
* char* msgstr          message string (may contain conversion specifications  *
*                       for vsprintf)                                          *
* char* input_string    if dialog type = DF_PROMPT, then: a character string   *
*                       array in which to put the string input by the user. Do *
*                       NOT include an input_string argument for other dialog  *
*                       types.                                                 *
* char* but_lbl         button label(s) for buttons requested (if n > 0, one   *
*                       but_lbl argument per button)                           *
* <anytype> <args>      arguments for vsprintf (if any)                        *
*                                                                              *
*                                                                              *
* Returns:                                                                     *
*                                                                              *
* button selected by user (i.e. 1, 2, or 3. or 4 for prompt)                   *
*                                                                              *
*                                                                              *
* Examples:                                                                    *
*                                                                              
* but_pressed = DialogF (DF_QUES, toplevel, 3, "Direction?", "up", "down", "other");
* but_pressed = DialogF (DF_ERR, toplevel, 1, "You can't do that!", "Acknowledged");
* but_pressed = DialogF (DF_PROMPT, toplevel, 0, "New %s", new_sub_category, categories[i]); 
*/
unsigned DialogF(DialogType dialog_type, Fl_Widget* parent, unsigned n, const char* title, const char* msgstr, ...) /* variable # arguments */
{
   bool isPromptDialog = (dialog_type == DF_PROMPT);
   if ((!isPromptDialog && (n > NUM_BUTTONS_SUPPORTED)) || (isPromptDialog && (n > NUM_BUTTONS_MAXPROMPT)))
   {
      printf("\nError calling DialogF - Too many buttons specified\n");
      return 0;
   }

   dfcallbackstruct df;

   va_list var;
   va_start(var, msgstr);

   // Get where to put dialog input string
   char* input_string = (isPromptDialog?va_arg(var, char*):0);

   Ne_DialogF dialog(50, 50, 300, 150, title, df);

   // Set up button labels and find a "Cancel/Dismiss" button
   std::vector<std::string> buttonLabels;
   for (int argcount = 0; argcount < (int)n; ++argcount)
   {
      std::string label = va_arg(var, char*);
      // Cancel/Dismiss is always the 0 index
      if (label == "Cancel" || label == "Dismiss")
         buttonLabels.insert(buttonLabels.begin(), label);
      else
         buttonLabels.push_back(label);
   }
   
   for( size_t i = 0; i < buttonLabels.size(); ++i)
      dialog.add_button(buttonLabels[i].c_str(), i);

   char msgstr_vsp[1024] = "";
   vsnprintf(msgstr_vsp, sizeof(msgstr_vsp), msgstr, var);
   va_end(var);

   Fl_Box* labelBox  = new Fl_Box(5, 5, 290, 50, msgstr_vsp);

   Ne_Input* input = 0;
   if (isPromptDialog)
   {
       input = new Ne_Input(5, 60, 290, 25);

// TODO:       XtSetArg(args[argcount], XmNselectionLabelString, msgstr_xms); argcount++;
// TODO:       XtSetArg(args[argcount], XmNdialogTitle, titstr_xms); argcount++;
// TODO:       XtSetArg(args[argcount], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); argcount ++;
// TODO: 
// TODO:       dialog = CreatePromptDialog(parent, dialog_name[dialog_num], args, argcount);
// TODO:       XtAddCallback(dialog, XmNokCallback, (XtCallbackProc)ok_callback, (char*)&df);
// TODO:       XtAddCallback(dialog, XmNcancelCallback, (XtCallbackProc)cancel_callback, (char*)&df);
// TODO:       XtAddCallback(dialog, XmNhelpCallback, (XtCallbackProc)help_callback, (char*)&df);
// TODO:       XtAddCallback(dialog, XmNapplyCallback, (XtCallbackProc)apply_callback, (char*)&df);
// TODO:       RemapDeleteKey(XmSelectionBoxGetChild(dialog, XmDIALOG_TEXT));
// TODO: 
// TODO:       /* Text area in prompt dialog should get focus, not ok button
// TODO:          since user enters text first.  To fix this, we need to turn
// TODO:          off the default button for the dialog, until after keyboard
// TODO:          focus has been established */
// TODO:       XtVaSetValues(dialog, XmNdefaultButton, NULL, NULL);
// TODO:       XtAddCallback(XmSelectionBoxGetChild(dialog, XmDIALOG_TEXT), XmNfocusCallback, (XtCallbackProc)focusCB, (char*)dialog);
// TODO: 
// TODO:       /* Limit the length of the text that can be entered in text field */
// TODO:       XtVaSetValues(XmSelectionBoxGetChild(dialog, XmDIALOG_TEXT), XmNmaxLength, DF_MAX_PROMPT_LENGTH-1, NULL);
// TODO: 
// TODO:       /* Turn on the requested number of buttons in the dialog by managing/unmanaging the button widgets */
// TODO:       switch (n)  		/* number of buttons requested */
// TODO:       {
// TODO:       case 0:
// TODO:       case 3:
// TODO:          break;		/* or default of 3 buttons */
// TODO:       case 1:
// TODO:          XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
// TODO:       case 2:
// TODO:          XtUnmanageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
// TODO:          break;
// TODO:       case 4:
// TODO:          XtManageChild(XmSelectionBoxGetChild(dialog, XmDIALOG_APPLY_BUTTON));
// TODO:          df.apply_up = 1;		/* apply button managed */
// TODO:       default:
// TODO:          break;
// TODO:       }				/* end switch */
// TODO: 
// TODO:       /*  Set margin width to managed buttons.  */
// TODO:       for (i = 0; (unsigned) i < N_SELECTION_BUTTONS; i++)
// TODO:       {
// TODO:          Fl_Widget* button = XmSelectionBoxGetChild(dialog, selectionButton_id[i]);
// TODO: 
// TODO:          if (XtIsManaged(button))
// TODO:          {
// TODO:             XtVaSetValues(button, XmNmarginWidth, BUTTON_WIDTH_MARGIN, NULL);
// TODO:          }
// TODO:       }
// TODO: 
// TODO:       /* If the button labeled cancel or dismiss is not the cancel button, or
// TODO:          if there is no button labeled cancel or dismiss, redirect escape key
// TODO:          events (this is necessary because the XmNcancelButton resource in
// TODO:          the bulletin board widget class is blocked from being reset) */
// TODO:       if (cancel_index == -1)
// TODO:          addEscapeHandler(dialog, NULL, 0);
// TODO:       else if (cancel_index != CANCEL_BTN)
// TODO:          addEscapeHandler(dialog, &df, cancel_index);
// TODO: 
// TODO:       /* Add a callback to the window manager close callback for the dialog */
// TODO:       AddMotifCloseCallback(XtParent(dialog),
// TODO:                             (XtCallbackProc)(cancel_index == APPLY_BTN ? apply_callback :
// TODO:                                              (cancel_index == CANCEL_BTN ? cancel_callback :
// TODO:                                               (cancel_index == HELP_BTN ? help_callback : ok_callback))), &df);
// TODO: 
// TODO:       /* A previous call to SetDialogFPromptHistory can request that an
// TODO:          up-arrow history-recall mechanism be attached.  If so, do it here */
// TODO:       if (NPromptHistoryItems != -1)
// TODO:          AddHistoryToTextWidget(XmSelectionBoxGetChild(dialog,XmDIALOG_TEXT), &PromptHistory, &NPromptHistoryItems);

      /* Get the focus to the input string.  */
       dialog.focus(input);

// TODO:       PromptHistory = NULL;
// TODO:       NPromptHistoryItems = -1;
   } // End prompt dialog path
   else // MessageBox dialogs
   {
// TODO:       XtAddCallback(dialog, XmNokCallback, (XtCallbackProc)ok_callback, (char*)&df);
// TODO:       XtAddCallback(dialog, XmNcancelCallback, (XtCallbackProc)cancel_callback, (char*)&df);
// TODO:       XtAddCallback(dialog, XmNhelpCallback, (XtCallbackProc)help_callback, (char*)&df);
// TODO: 
// TODO:       /* If the button labeled cancel or dismiss is not the cancel button, or
// TODO:          if there is no button labeled cancel or dismiss, redirect escape key
// TODO:          events (this is necessary because the XmNcancelButton resource in
// TODO:          the bulletin board widget class is blocked from being reset) */
// TODO:       if (cancel_index == -1)
// TODO:          addEscapeHandler(dialog, NULL, 0);
// TODO:       else if (cancel_index != CANCEL_BTN)
// TODO:          addEscapeHandler(dialog, &df, cancel_index);
// TODO: 
   }

   /* Pop up the dialog */
   ManageDialogCenteredOnPointer(&dialog);

   dialog.set_modal();
   dialog.show();
   while (dialog.shown()) Fl::wait();
   
   if (isPromptDialog)
      strcpy(input_string, input->value());   /* This step is necessary */

   return (df.button);
}

/*
** Add up-arrow history recall to the next DialogF(DF_PROMPT... call (see
** AddHistoryToTextWidget in misc.c).  This must be re-set before each call.
** calling DialogF with a dialog type of DF_PROMPT automatically resets
** this mode back to no-history-recall.
*/
void SetDialogFPromptHistory(char** historyList, int nItems)
{
   PromptHistory = historyList;
   NPromptHistoryItems = nItems;
}
