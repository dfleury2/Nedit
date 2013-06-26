/*******************************************************************************
*                                                                              *
* managedList.c -- User interface for reorderable list of records              *
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
* November, 1995                                                               *
*                                                                              *
* Written by Mark Edel                                                         *
*                                                                              *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "managedList.h"
#include "misc.h"
#include "DialogF.h"

#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Button.H>

#include <string>
#include <iostream>

#include <stdio.h>
#include <string.h>

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

// Common data between the managed list callback functions
struct managedListData
{
   Fl_Browser* listW;
   Fl_Button* deleteBtn;
   Fl_Button* copyBtn;
   Fl_Button* moveUpBtn;
   Fl_Button* moveDownBtn;

   void* (*getDialogDataCB)(void*, int, int*, void*);
   void* getDialogDataArg;
   void (*setDialogDataCB)(void*, void*);
   void* setDialogDataArg;
   void* (*copyItemCB)(void*);
   void (*freeItemCB)(void*);
   int (*deleteConfirmCB)(int, void*);
   void* deleteConfirmArg;
   int maxItems;
   int* nItems;
   NamedItem** itemList;
   int lastSelection;
};

static Fl_Browser* ManageListAndButtons(Fl_Browser* listW,
                            Fl_Button* deleteBtn, Fl_Button* copyBtn, Fl_Button* moveUpBtn, Fl_Button* moveDownBtn,
                            NamedItem** itemList, int* nItems, int maxItems,
                            void *(*getDialogDataCB)(void*, int, int*, void*), void* getDialogDataArg,
                            void (*setDialogDataCB)(void*, void*), void* setDialogDataArg,
                            void (*freeItemCB)(void*));
static void destroyCB(Fl_Widget* w, void* data);
static void deleteCB(Fl_Widget* w, void* data);
static void copyCB(Fl_Widget* w, void* data);
static void moveUpCB(Fl_Widget* w, void* data);
static void moveDownCB(Fl_Widget* w, void* data);
static int incorporateDialogData(managedListData* ml, int listPos, int explicitRequest);
static void updateDialogFromList(managedListData* ml, int selection);
static void updateListWidgetItem(managedListData* ml, int listPos);
static void listSelectionCB(Fl_Widget* w, void* data);
static int selectedListPosition(managedListData* ml);
static void selectItem(Fl_Browser* listW, int itemIndex, int updateDialog);

// --------------------------------------------------------------------------
// Create a user interface to help manage a list of arbitrary data records
// which users can edit, add, delete, and reorder.
//
// The caller creates the overall dialog for presenting the data to the user,
// but embeds the list and button widgets created here (and list management
// code they activate) to handle the organization of the overall list.
//
// This routine creates a form widget containing the buttons and list widget
// with which the user interacts with the list data.  ManageListAndButtons
// can be used alternatively to take advantage of the management code with a
// different arrangement of the widgets (this routine puts buttons in a
// column on the left, list on the right) imposed here.
//
// "args" and "argc" are passed to the form widget creation routine, so that
// attachments can be specified for embedding the form in a dialog.
//
// See ManageListAndButtons for a description of the remaining arguments.
// --------------------------------------------------------------------------
Fl_Browser* CreateManagedList(const Ne_Dimension& dimension,
                         NamedItem** itemList, int* nItems, int maxItems, int nColumns,
                         void *(*getDialogDataCB)(void*, int, int*, void*), void* getDialogDataArg,
                         void (*setDialogDataCB)(void*, void*), void* setDialogDataArg,
                         void (*freeItemCB)(void*))
{
   Fl_Group* form = new Fl_Group(dimension.x, dimension.y, dimension.w, dimension.h);
      Fl_Group* btnColumn = new Fl_Group(dimension.x, dimension.y, 80, dimension.h);
         Fl_Button* deleteBtn = new Fl_Button(dimension.x, dimension.y, 80, 25, "Delete");
         Fl_Button* copyBtn = new Fl_Button(dimension.x, dimension.y + 30, 80, 25, "Copy");
         Fl_Button* moveUpBtn = new Fl_Button(dimension.x, dimension.y + 2 * 30, 80, 25, "Move @8>");
         Fl_Button* moveDownBtn = new Fl_Button(dimension.x, dimension.y + 3 * 30, 80, 25, "Move @8<");
         Fl_Group* resizableBox = new Fl_Group(dimension.x, dimension.y + 4 * 30, 80, 1);
      btnColumn->resizable(resizableBox);
      btnColumn->end();

      Fl_Hold_Browser* listW
         = new Fl_Hold_Browser(btnColumn->x() + btnColumn->w() + 5, dimension.y,
                               dimension.w - (btnColumn->x() + btnColumn->w()), dimension.h);

   form->resizable(listW);
   form->end();

   return ManageListAndButtons(listW,
      deleteBtn, copyBtn, moveUpBtn, moveDownBtn,
      itemList, nItems, maxItems,
      getDialogDataCB, getDialogDataArg,
      setDialogDataCB, setDialogDataArg,
      freeItemCB);
}

// --------------------------------------------------------------------------
// Manage a list widget and a set of buttons which represent a list of
// records.  The caller provides facilities for editing the records
// individually, and this code handles the organization of the overall list,
// such that the user can modify, add, and delete records, and re-order the
// list.
//
// The format for the list of records managed by this code should be an
// array of size "maxItems" of pointers to record structures.  The records
// themselves can be of any format, but the first field of the structure
// must be a pointer to a character string which will be displayed as the
// item name in the list.  The list "itemList", and the number of items
// "nItems" are automatically updated by the list management routines as the
// user makes changes.
//
// The caller must provide routines for transferring data to and from the
// dialog fields dedicated to displaying and editing records in the list.
// The callback "setDialogDataCB" must take the contents of the item pointer
// passed to it, and display the data it contains, erasing any previously
// displayed data.  The format of the setDialogData callback is:
//
//    void setDialogDataCB(void *item, void *cbArg)
//
//	 item:	a pointer to the record to be displayed
//
//  	 cbArg:	an arbitrary argument passed on to the callback routine
//
// The callback "setDialogDataCB" must allocate (with malloc) and return a
// new record reflecting the current contents of the dialog fields.  It may
// do error checking on the data that the user has entered, and can abort
// whatever operation triggered the request by setting "abort" to true.
// This routine is called in a variety of contexts, such as the user
// clicking on a list item, or requesting that a copy be made of the current
// list item.  To aide in communicating errors to the user, the boolean value
// "explicitRequest" distinguishes between the case where the user has
// specifically requested that the fields be read, and the case where he
// may be surprised that errors are being reported, and require further
// explanation.  The format of the getDialogData callback is:
//
//    void *getDialogDataCB(void *oldItem, int explicitRequest, int *abort,
//  	      void *cbArg)
//
//	 oldItem: a pointer to the existing record being modified in the
//	    dialog, or NULL, if the user is modifying the "New" item.
//
//	 explicitRequest: true if the user directly asked for the records
//	    to be changed (as with an OK or Apply button).  If a less direct
//	    process resulted in the request, the user might need extra
//	    explanation and possibly a chance to proceed using the existing
//	    stored data (to use the data from oldItem, the routine should
//	    make a new copy).
//
//	 abort: Can be set to true if the dialog fields contain errors.
//	    Setting abort to true, stops whetever process made the request
//	    for updating the data in the list from the displayed data, and
//	    forces the user to remain focused on the currently displayed
//	    item until he either gives up or gets it right.
//
//	 cbArg: arbitrary data, passed on from where the callback was
//	    established in the list creation routines
//
//    The return value should be an allocated
//
// The callback "freeItemCB" should free the item passed to it:
//
//    void freeItemCB(void *item, void *cbArg)
//
//	 item:	a pointer to the record to be freed
//
// The difference between ManageListAndButtons and CreateManagedList, is that
// in this routine, the caller creates the list and button widgets and passes
// them here so that they be arranged explicitly, rather than relying on the
// default style imposed by CreateManagedList.  ManageListAndButtons simply
// attaches the appropriate callbacks to process mouse and keyboard input from
// the widgets.
// --------------------------------------------------------------------------
Fl_Browser* ManageListAndButtons(Fl_Browser* listW,
                            Fl_Button* deleteBtn, Fl_Button* copyBtn, Fl_Button* moveUpBtn, Fl_Button* moveDownBtn,
                            NamedItem** itemList, int* nItems,
                            int maxItems, void *(*getDialogDataCB)(void*, int, int*, void*), void* getDialogDataArg,
                            void (*setDialogDataCB)(void*, void*), void* setDialogDataArg,
                            void (*freeItemCB)(void*))
{
   managedListData* ml = new managedListData();

   // Create a managedList data structure to hold information about the
   // widgets, callbacks, and current state of the list
   ml->listW = listW;
   ml->deleteBtn = deleteBtn;
   ml->copyBtn = copyBtn;
   ml->moveUpBtn = moveUpBtn;
   ml->moveDownBtn = moveDownBtn;
   ml->getDialogDataCB = NULL;
   ml->getDialogDataArg = getDialogDataArg;
   ml->setDialogDataCB = NULL;
   ml->setDialogDataArg = setDialogDataArg;
   ml->freeItemCB = freeItemCB;
   ml->deleteConfirmCB = NULL;
   ml->deleteConfirmArg = NULL;
   ml->nItems = nItems;
   ml->maxItems = maxItems;
   ml->itemList = itemList;
   ml->lastSelection = 1;

   // Make the managed list data structure accessible from the list widget
   // pointer, and make sure it gets freed when the list is destroyed
   ml->listW->user_data(ml);

// TODO:    XtAddCallback(ml->listW, XmNdestroyCallback, destroyCB, ml);

   // Add callbacks for button and list actions
   ml->deleteBtn->callback(deleteCB, ml);
   ml->copyBtn->callback(copyCB, ml);
   ml->moveUpBtn->callback(moveUpCB, ml);
   ml->moveDownBtn->callback(moveDownCB, ml);
   ml->listW->callback(listSelectionCB, ml);

   // Initialize the list and buttons (don't set up the callbacks until
   // this is done, so they won't get called on creation)
   updateDialogFromList(ml, -1);
   ml->getDialogDataCB = getDialogDataCB;
   ml->setDialogDataCB = setDialogDataCB;

   return ml->listW;
}

// --------------------------------------------------------------------------
// Update the currently selected list item from the dialog fields, using
// the getDialogDataCB callback.  "explicitRequest" is a boolean value
// passed to on to the getDialogDataCB callback to help set the tone for
// how error messages are presented (see ManageListAndButtons for more
// information).
// --------------------------------------------------------------------------
int UpdateManagedList(Fl_Browser* listW, int explicitRequest)
{
   // Recover the pointer to the managed list structure from the widget's userData pointer
   managedListData* ml = (managedListData*)listW->user_data();

   // Make the update
   return incorporateDialogData(ml, selectedListPosition(ml), explicitRequest);
}

// --------------------------------------------------------------------------
// Update the displayed list and data to agree with a data list which has
// been changed externally (not by the ManagedList list manager).
// --------------------------------------------------------------------------
void ChangeManagedListData(Fl_Browser* listW)
{
   // Recover the pointer to the managed list structure from the widget's userData pointer
   managedListData* ml = (managedListData*)listW->user_data();

   updateDialogFromList(ml, -1);
}

// --------------------------------------------------------------------------
// Change the selected item in the managed list given the index into the
// list being managed.
// --------------------------------------------------------------------------
void SelectManagedListItem(Fl_Browser* listW, int itemIndex)
{
   selectItem(listW, itemIndex, true);
}

// --------------------------------------------------------------------------
// Return the index of the item currently selected in the list
// --------------------------------------------------------------------------
int ManagedListSelectedIndex(Fl_Browser* listW)
{
   managedListData* ml = (managedListData*)listW->user_data();
   return selectedListPosition(ml)-2;
}

// --------------------------------------------------------------------------
// Add a delete-confirmation callback to a managed list.  This will be called
// when the user presses the Delete button on the managed list.  The callback
// can put up a dialog, and optionally abort the operation by returning false.
// --------------------------------------------------------------------------
void AddDeleteConfirmCB(Fl_Widget* listW, int (*deleteConfirmCB)(int, void*),void* deleteConfirmArg)
{
   managedListData* ml = static_cast<managedListData*>(listW->user_data());
   if (ml)
   {
      ml->deleteConfirmCB = deleteConfirmCB;
      ml->deleteConfirmArg = deleteConfirmArg;
   }
}

// --------------------------------------------------------------------------
// Called on destruction of the list widget
// --------------------------------------------------------------------------
static void destroyCB(Fl_Widget* w, void* data)
{
   delete static_cast<managedListData*>(data);
}

// --------------------------------------------------------------------------
// Button callbacks: deleteCB, copyCB, moveUpCB, moveDownCB
// --------------------------------------------------------------------------
static void deleteCB(Fl_Widget* w, void* data)
{
   TRACE();
   managedListData* ml = (managedListData*)data;

   // get the selected list position and the item to be deleted
   int listPos = selectedListPosition(ml);
   int ind = listPos-2;

   // if there's a delete confirmation callback, call it first, and allow
   // it to request that the operation be aborted
   if (ml->deleteConfirmCB != NULL)
      if (!(*ml->deleteConfirmCB)(ind, ml->deleteConfirmArg))
         return;

   // free the item and remove it from the list
   (*ml->freeItemCB)(ml->itemList[ind]);
   for (int i=ind; i<*ml->nItems-1; i++)
      ml->itemList[i] = ml->itemList[i+1];
   (*ml->nItems)--;

   // update the list widget and move the selection to the previous item
   // in the list and display the fields appropriate  for that entry
   updateDialogFromList(ml, ind-1);
}

// --------------------------------------------------------------------------
static void copyCB(Fl_Widget* w, void* data)
{
   TRACE();
   managedListData* ml = (managedListData*)data;
   int abort = false;

   // get the selected list position and the item to be copied
   int listPos = selectedListPosition(ml);
   if (listPos == 1)
      return; // can't copy "new"

   if ((*ml->nItems) == ml->maxItems)
   {
      DialogF(DF_ERR, ml->listW, 1, "Limits exceeded", "Cannot copy item.\nToo many items in list.", "OK");
      return;
   }

   // Bring the entry up to date (could result in operation being canceled)
   NamedItem* item = (NamedItem*)(*ml->getDialogDataCB)(ml->itemList[listPos-2], false, &abort, ml->getDialogDataArg);
   if (abort)
      return;
   if (item != NULL)
   {
      (*ml->freeItemCB)(ml->itemList[listPos-2]);
      ml->itemList[listPos-2] = item;
   }

   // Make a copy by requesting the data again.
   // In case getDialogDataCB() returned a fallback value, the dialog may
   // not be in sync with the internal list. If we _explicitly_ request the
   // data again, we could get an invalid answer. Therefore, we first update
   // the dialog to make sure that we can copy the right data.
   updateDialogFromList(ml, listPos-2);
   item = (NamedItem*)(*ml->getDialogDataCB)(ml->itemList[listPos-2], true, &abort, ml->getDialogDataArg);
   if (abort)
      return;

   // add the item to the item list
   for (int i= *ml->nItems; i>=listPos; i--)
      ml->itemList[i] = ml->itemList[i-1];
   ml->itemList[listPos-1] = item;
   (*ml->nItems)++;

   // redisplay the list widget and select the new item
   updateDialogFromList(ml, listPos-1);
}

// --------------------------------------------------------------------------
static void moveUpCB(Fl_Widget* w, void* data)
{
   TRACE();
   managedListData* ml = (managedListData*)data;

   // get the item index currently selected in the menu item list
   int listPos = selectedListPosition(ml);
   int ind = listPos-2;

   // Bring the item up to date with the dialog fields (It would be better
   // if this could be avoided, because user errors will be flagged here,
   // but it's not worth re-writing everything for such a trivial point)
   if (!incorporateDialogData(ml, ml->lastSelection, false))
      return;

   // shuffle the item up in the menu item list
   NamedItem* temp = ml->itemList[ind];
   ml->itemList[ind] = ml->itemList[ind-1];
   ml->itemList[ind-1] = temp;

   // update the list widget and keep the selection on moved item
   updateDialogFromList(ml, ind-1);
}

// --------------------------------------------------------------------------
static void moveDownCB(Fl_Widget* w, void* data)
{
   TRACE();
   managedListData* ml = (managedListData*)data;

   // get the item index currently selected in the menu item list
   int listPos = selectedListPosition(ml);
   int ind = listPos-2;

   // Bring the item up to date with the dialog fields (I wish this could be avoided)
   if (!incorporateDialogData(ml, ml->lastSelection, false))
      return;

   // shuffle the item down in the menu item list
   NamedItem* temp = ml->itemList[ind];
   ml->itemList[ind] = ml->itemList[ind+1];
   ml->itemList[ind+1] = temp;

   // update the list widget and keep the selection on moved item
   updateDialogFromList(ml, ind+1);
}

// --------------------------------------------------------------------------
// Called when the user clicks on an item in the list widget
// --------------------------------------------------------------------------
static void listSelectionCB(Fl_Widget* w, void* data)
{
   TRACE();
   Fl_Hold_Browser* browser = dynamic_cast<Fl_Hold_Browser*>(w);
   managedListData* ml = static_cast<managedListData*>(data);
   int listPos = browser->value();

   // Save the current dialog fields before overwriting them.  If there's an
   // error, force the user to go back to the old selection and fix it
   //   before proceeding
   if (ml->getDialogDataCB != NULL && ml->lastSelection != 0)
   {
      if (!incorporateDialogData(ml, ml->lastSelection, false))
      {
         ml->listW->select(ml->lastSelection);
         return;
      }
      // reselect item because incorporateDialogData can alter selection
      selectItem(ml->listW, listPos - 2, false);
   }
   ml->lastSelection = listPos;

   // Dim or un-dim buttons at bottom of dialog based on whether the
   // selected item is a menu entry, or "New"
   if (listPos == 1)
   {
      NeSetSensitive(ml->copyBtn, false);
      NeSetSensitive(ml->deleteBtn, false);
      NeSetSensitive(ml->moveUpBtn, false);
      NeSetSensitive(ml->moveDownBtn, false);
   }
   else
   {
      NeSetSensitive(ml->copyBtn, true);
      NeSetSensitive(ml->deleteBtn, true);
      NeSetSensitive(ml->moveUpBtn, listPos != 2);
      NeSetSensitive(ml->moveDownBtn, listPos != *ml->nItems+1);
   }

   // get the index of the item currently selected in the item list
   int ind = listPos - 2;

   // tell the caller to show the new item
   if (ml->setDialogDataCB != NULL)
      (*ml->setDialogDataCB)(listPos==1 ? NULL : ml->itemList[ind], ml->setDialogDataArg);
}

// --------------------------------------------------------------------------
// Incorporate the current contents of the dialog fields into the list
// being managed, and if necessary change the display in the list widget.
// The data is obtained by calling the getDialogDataCB callback, which
// is allowed to reject whatever request triggered the update.  If the
// request is rejected, the return value from this function will be false.
// --------------------------------------------------------------------------
static int incorporateDialogData(managedListData* ml, int listPos, int explicitRequest)
{
   int abort = false;

   // Get the current contents of the dialog fields.  Callback will set
   // abort to true if canceled
   NamedItem* item = (NamedItem*)(*ml->getDialogDataCB)(listPos == 1 ? NULL : ml->itemList[listPos-2], explicitRequest, &abort, ml->getDialogDataArg);
   if (abort)
      return false;
   if (item == NULL) // don't modify if fields are empty
      return true;

   // If the item is "new" add a new entry to the list, otherwise,
   // modify the entry with the text fields from the dialog
   if (listPos == 1)
   {
      if ((*ml->nItems) == ml->maxItems)
      {
         DialogF(DF_ERR, ml->listW, 1, "Limits exceeded", "Cannot add new item.\nToo many items in list.", "OK");
         return false;
      }
      ml->itemList[(*ml->nItems)++] = item;
      updateDialogFromList(ml, *ml->nItems - 1);
   }
   else
   {
      (*ml->freeItemCB)(ml->itemList[listPos-2]);
      ml->itemList[listPos-2] = item;
      updateListWidgetItem(ml, listPos);
   }
   return true;
}

// --------------------------------------------------------------------------
// Update the list widget to reflect the current contents of the managed item
// list, set the item that should now be highlighted, and call getDisplayed
// on the newly selected item to fill in the dialog fields.
// --------------------------------------------------------------------------
static void updateDialogFromList(managedListData* ml, int selection)
{
   ml->listW->clear();

   ml->listW->add("New");
   // Fill in the list widget with the names from the item list
   for (int i=0; i < *ml->nItems; i++)
      ml->listW->add(ml->itemList[i]->getName());
   
   // Select the requested item (indirectly filling in the dialog fields),
   // but don't trigger an update of the last selected item from the current
   // dialog fields
   ml->lastSelection = 0;
   selectItem(ml->listW, selection, true);
}

// --------------------------------------------------------------------------
// Update one item of the managed list widget to reflect the current contents
// of the managed item list.
// --------------------------------------------------------------------------
static void updateListWidgetItem(managedListData* ml, int listPos)
{
   // save the current selected position (Motif sometimes does stupid things
   // to the selection when a change is made, like selecting the new item
   // if it matches the name of currently selected one) */
   int savedPos = selectedListPosition(ml);

   // update the list
   ml->listW->replace(listPos, ml->itemList[listPos-2]->getName());

   // restore the selected position
   ml->listW->select(savedPos);
}

// --------------------------------------------------------------------------
// Get the position of the selection in the menu item list widget
// --------------------------------------------------------------------------
static int selectedListPosition(managedListData* ml)
{
   return ml->listW->value();
}

// --------------------------------------------------------------------------
// Select an item in the list given the list (array) index value.
// If updateDialog is true, trigger a complete dialog update, which
// could potentially reject the change.
// --------------------------------------------------------------------------
static void selectItem(Fl_Browser* listW, int itemIndex, int updateDialog)
{
   int selection = itemIndex + 2;
   
   // Select the item
   listW->select(selection);
   if (updateDialog)
      listSelectionCB(listW, listW->user_data());

   // If the selected item is not visible, scroll the list
   listW->make_visible(selection);
}
