/* $Id: managedList.h,v 1.7 2004/11/09 21:58:45 yooden Exp $ */
/*******************************************************************************
*                                                                              *
* managedList.h -- Nirvana Editor Managed List Header File                     *
*                                                                              *
* Copyright 2002 The NEdit Developers                                          *
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

#ifndef NEDIT_MANAGEDLIST_H_INCLUDED
#define NEDIT_MANAGEDLIST_H_INCLUDED

#include "misc.h"

#include <FL/Fl_Button.H>
#include <FL/Fl_Browser.H>

// Common interface to display a name on the managed list
struct NamedItem
{
   virtual const char* getName() const = 0;
   virtual ~NamedItem() {}
};

Fl_Browser* CreateManagedList(const Ne_Dimension& dimension,
                         NamedItem** itemList, int* nItems, int maxItems, int nColumns,
                         void *(*getDialogDataCB)(void*, int, int*, void*), void* getDialogDataArg,
                         void (*setDialogDataCB)(void*, void*), void* setDialogDataArg,
                         void (*freeItemCB)(void*));
int UpdateManagedList(Fl_Browser* listW, int explicitRequest);
int ManagedListSelectedIndex(Fl_Browser* listW);
void ChangeManagedListData(Fl_Browser* listW);
void SelectManagedListItem(Fl_Browser* listW, int itemIndex);
void AddDeleteConfirmCB(Fl_Widget* listW, int (*deleteConfirmCB)(int, void*), void* deleteConfirmArg);

#endif /* NEDIT_MANAGEDLIST_H_INCLUDED */
