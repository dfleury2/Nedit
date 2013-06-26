/* $Id: menu.h,v 1.14 2008/01/04 22:11:03 yooden Exp $ */
/*******************************************************************************
*                                                                              *
* menu.h -- Nirvana Editor Menu Header File                                    *
*                                                                              *
* Copyright 2004 The NEdit Developers                                          *
*                                                                              *
* This is free__ software; you can redistribute it and/or modify it under the    *
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

#ifndef NEDIT_MENU_H_INCLUDED
#define NEDIT_MENU_H_INCLUDED

#include "nedit.h"

#include "../util/misc.h"
#include "../util/Ne_AppContext.h"

#define PERMANENT_MENU_ITEM (XtPointer)1
#define TEMPORARY_MENU_ITEM (XtPointer)2

Ne_MenuBar* CreateMenuBar(Fl_Widget* parent, WindowInfo* window);
void InstallMenuActions(Ne_AppContext& context);
XtActionsRec* GetMenuActions(int* nActions);
void InvalidateWindowMenus();
void InvalidatePrevOpenMenus();
void CheckCloseDim();
void AddToPrevOpenMenu(const char* filename);
void WriteNEditDB();
void ReadNEditDB();
// TODO: Fl_Widget* CreateBGMenu(WindowInfo* window);
// TODO: void AddBGMenuAction(Fl_Widget* widget);
// TODO: void HidePointerOnKeyedEvent(Fl_Widget* w, XEvent* event);
// TODO: Fl_Widget* CreateTabContextMenu(Fl_Widget* parent, WindowInfo* window);
// TODO: void AddTabContextMenuAction(Fl_Widget* widget);
// TODO: void ShowHiddenTearOff(Fl_Widget* menuPane);

#endif /* NEDIT_MENU_H_INCLUDED */
