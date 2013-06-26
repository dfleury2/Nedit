static const char CVSID[] = "$Id: userCmds.c,v 1.57 2009/06/23 21:03:13 lebert Exp $";
/*******************************************************************************
*									       *
* userCmds.c -- Nirvana Editor shell and macro command dialogs 		       *
*									       *
* Copyright (C) 1999 Mark Edel						       *
*									       *
* This is free__ software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version. In addition, you may distribute version of this program linked to   *
* Motif or Open Motif. See README for details.                                 *
*                                                                              *
* This software is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
* for more details.							       *
* 									       *
* You should have received a copy of the GNU General Public License along with *
* software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
* Place, Suite 330, Boston, MA  02111-1307 USA		                       *
*									       *
* Nirvana Text Editor	    						       *
* April, 1997								       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "userCmds.h"
#include "Ne_Text_Buffer.h"
#include "Ne_Text_Editor.h"
#include "nedit.h"
#include "preferences.h"
#include "window.h"
#include "menu.h"
#include "shell.h"
#include "macro.h"
#include "file.h"
#include "interpret.h"
#include "parse.h"
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "../util/managedList.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef WIN32
#include <sys/param.h>
#endif // WIN32

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

#if XmVersion >= 1002
#define MENU_WIDGET(w) (XmGetPostedFromWidget(XtParent(w)))
#else
#define MENU_WIDGET(w) (w)
#endif

/* max. number of user programmable menu commands allowed per each of the
   macro, shell, and background menus */
#define MAX_ITEMS_PER_MENU 400

/* indicates, that an unknown (i.e. not existing) language mode
   is bound to an user menu item */
#define UNKNOWN_LANGUAGE_MODE -2

/* major divisions (in position units) in User Commands dialogs */
#define LEFT_MARGIN_POS 1
#define RIGHT_MARGIN_POS 99
#define LIST_RIGHT 45
#define SHELL_CMD_TOP 70
#define MACRO_CMD_TOP 40

/* types of current dialog and/or menu */
enum dialogTypes {SHELL_CMDS, MACRO_CMDS, BG_MENU_CMDS};

/* Structure representing a menu item for shell, macro and BG menus*/
typedef struct
{
   char* name;
   unsigned int modifiers;
   int keysym;
   char mnemonic;
   char input;
   char output;
   char repInput;
   char saveFirst;
   char loadAfter;
   char* cmd;
} menuItemRec;

/* Structure for widgets and flags associated with shell command,
   macro command and BG command editing dialogs */
typedef struct
{
   int dialogType;
   WindowInfo* window;
   Fl_Widget* nameTextW, *accTextW, *mneTextW, *cmdTextW, *saveFirstBtn;
   Fl_Widget* loadAfterBtn, *selInpBtn, *winInpBtn, *eitherInpBtn, *noInpBtn;
   Fl_Widget* repInpBtn, *sameOutBtn, *dlogOutBtn, *winOutBtn, *dlogShell;
   Fl_Widget* managedList;
   menuItemRec** menuItemsList;
   int nMenuItems;
} userCmdDialog;

/* Structure for keeping track of hierarchical sub-menus during user-menu
   creation */
typedef struct
{
   char* name;
   Fl_Widget* menuPane;
} menuTreeItem;

/* Structure holding hierarchical info about one sub-menu.

   Suppose following user menu items:
   a.) "menuItem1"
   b.) "subMenuA>menuItemA1"
   c.) "subMenuA>menuItemA2"
   d.) "subMenuA>subMenuB>menuItemB1"
   e.) "subMenuA>subMenuB>menuItemB2"

   Structure of this user menu is:

   Main Menu    Name       Sub-Menu A   Name       Sub-Menu B   Name
   element nbr.            element nbr.            element nbr.
        0       menuItem1
        1       subMenuA --+->    0     menuItemA1
                           +->    1     menuItemA2
                           +->    2     subMenuB --+->    0     menuItemB1
                                                   +->    1     menuItemB2

   Above example holds 2 sub-menus:
   1.) "subMenuA" (hierarchical ID = {1} means: element nbr. "1" of main menu)
   2.) "subMenuA>subMenuB" (hierarchical ID = {1, 2} means: el. nbr. "2" of
       "subMenuA", which itself is el. nbr. "0" of main menu) */
struct userSubMenuInfo
{
   char* usmiName;  /* hierarchical name of sub-menu */
   int*  usmiId;    /* hierarchical ID of sub-menu   */
   int   usmiIdLen; /* length of hierarchical ID     */
};

/* Holds info about sub-menu structure of an user menu */
typedef struct
{
   int              usmcNbrOfMainMenuItems; /* number of main menu items */
   int              usmcNbrOfSubMenus;      /* number of sub-menus */
   userSubMenuInfo* usmcInfo;               /* list of sub-menu info */
} userSubMenuCache;

/* Structure holding info about a single menu item.
   According to above example there exist 5 user menu items:
   a.) "menuItem1"  (hierarchical ID = {0} means: element nbr. "0" of main menu)
   b.) "menuItemA1" (hierarchical ID = {1, 0} means: el. nbr. "0" of
                     "subMenuA", which itself is el. nbr. "1" of main menu)
   c.) "menuItemA2" (hierarchical ID = {1, 1})
   d.) "menuItemB1" (hierarchical ID = {1, 2, 0})
   e.) "menuItemB2" (hierarchical ID = {1, 2, 1})
 */
typedef struct
{
   char*    umiName;               /* hierarchical name of menu item
                                       (w.o. language mode info) */
   int*     umiId;                 /* hierarchical ID of menu item */
   int      umiIdLen;              /* length of hierarchical ID */
   bool  umiIsDefault;          /* menu item is default one ("@*") */
   int      umiNbrOfLanguageModes; /* number of language modes
                                       applicable for this menu item */
   int*     umiLanguageMode;       /* list of applicable lang. modes */
   int      umiDefaultIndex;       /* array index of menu item to be
                                       used as default, if no lang. mode
                                       matches */
   bool  umiToBeManaged;        /* indicates, that menu item needs
                                       to be managed */
} userMenuInfo;

/* Structure holding info about a selected user menu (shell, macro or
   background) */
typedef struct
{
   int                sumType;            /* type of menu (shell, macro or
                                              background */
   Fl_Widget*             sumMenuPane;        /* pane of main menu */
   int                sumNbrOfListItems;  /* number of menu items */
   menuItemRec**      sumItemList;        /* list of menu items */
   userMenuInfo**     sumInfoList;        /* list of infos about menu items */
   userSubMenuCache*  sumSubMenus;        /* info about sub-menu structure */
   UserMenuList*      sumMainMenuList;    /* cached info about main menu */
   bool*           sumMenuCreated;     /* pointer to "menu created"
                                              indicator */
} selectedUserMenu;

/* Descriptions of the current user programmed menu items for re-generating
   menus and processing shell, macro, and background menu selections */
static menuItemRec* ShellMenuItems[MAX_ITEMS_PER_MENU];
static userMenuInfo*     ShellMenuInfo[MAX_ITEMS_PER_MENU];
static userSubMenuCache  ShellSubMenus;
static int NShellMenuItems = 0;
static menuItemRec* MacroMenuItems[MAX_ITEMS_PER_MENU];
static userMenuInfo*     MacroMenuInfo[MAX_ITEMS_PER_MENU];
static userSubMenuCache  MacroSubMenus;
static int NMacroMenuItems = 0;
static menuItemRec* BGMenuItems[MAX_ITEMS_PER_MENU];
static userMenuInfo*     BGMenuInfo[MAX_ITEMS_PER_MENU];
static userSubMenuCache  BGSubMenus;
static int NBGMenuItems = 0;

/* Top level shells of the user-defined menu editing dialogs */
static Fl_Widget* ShellCmdDialog = NULL;
static Fl_Widget* MacroCmdDialog = NULL;
static Fl_Widget* BGMenuCmdDialog = NULL;

/* Paste learn/replay sequence buttons in user defined menu editing dialogs
   (for dimming and undimming externally when replay macro is available) */
static Fl_Widget* MacroPasteReplayBtn = NULL;
static Fl_Widget* BGMenuPasteReplayBtn = NULL;

// TODO: static void editMacroOrBGMenu(WindowInfo* window, int dialogType);
static void dimSelDepItemsInMenu(Fl_Widget* menuPane, menuItemRec** menuList, int nMenuItems, int sensitive);
// TODO: static void rebuildMenuOfAllWindows(int menuType);
static void rebuildMenu(WindowInfo* window, int menuType);
// TODO: static Fl_Widget* findInMenuTree(menuTreeItem* menuTree, int nTreeEntries, const char* hierName);
static char* copySubstring(const char* string, int length);
// TODO: static Fl_Widget* createUserMenuItem(Fl_Widget* menuPane, char* name, menuItemRec* f, int index, XtCallbackProc cbRtn, XtPointer cbArg);
// TODO: static Fl_Widget* createUserSubMenu(Fl_Widget* parent, char* label, Fl_Widget** menuItem);
static void deleteMenuItems(Fl_Widget* menuPane);
static void selectUserMenu(WindowInfo* window, int menuType, selectedUserMenu* menu);
static void updateMenu(WindowInfo* window, int menuType);
// TODO: static void manageTearOffMenu(Fl_Widget* menuPane);
static void resetManageMode(UserMenuList* list);
// TODO: static void manageAllSubMenuWidgets(UserMenuListElement* subMenu);
// TODO: static void unmanageAllSubMenuWidgets(UserMenuListElement* subMenu);
// TODO: static void manageMenuWidgets(UserMenuList* list);
// TODO: static void removeAccelFromMenuWidgets(UserMenuList* menuList);
// TODO: static void assignAccelToMenuWidgets(UserMenuList* menuList, WindowInfo* window);
static void manageUserMenu(selectedUserMenu* menu, WindowInfo* window);
static void createMenuItems(WindowInfo* window, selectedUserMenu* menu);
// TODO: static void okCB(Fl_Widget* w, XtPointer clientData, XtPointer callData);
// TODO: static void applyCB(Fl_Widget* w, XtPointer clientData, XtPointer callData);
// TODO: static void checkCB(Fl_Widget* w, XtPointer clientData, XtPointer callData);
// TODO: static int checkMacro(userCmdDialog* ucd);
// TODO: static int checkMacroText(char* macro, Fl_Widget* errorParent, Fl_Widget* errFocus);
// TODO: static int applyDialogChanges(userCmdDialog* ucd);
// TODO: static void closeCB(Fl_Widget* w, XtPointer clientData, XtPointer callData);
// TODO: static void pasteReplayCB(Fl_Widget* w, XtPointer clientData, XtPointer callData);
// TODO: static void destroyCB(Fl_Widget* w, XtPointer clientData, XtPointer callData);
// TODO: static void accKeyCB(Fl_Widget* w, XtPointer clientData, XKeyEvent* event);
// TODO: static void sameOutCB(Fl_Widget* w, XtPointer clientData, XtPointer callData);
// TODO: static void shellMenuCB(Fl_Widget* w, WindowInfo* window, XtPointer callData);
// TODO: static void macroMenuCB(Fl_Widget* w, WindowInfo* window, XtPointer callData);
// TODO: static void bgMenuCB(Fl_Widget* w, WindowInfo* window, XtPointer callData) ;
// TODO: static void accFocusCB(Fl_Widget* w, XtPointer clientData, XtPointer callData);
// TODO: static void accLoseFocusCB(Fl_Widget* w, XtPointer clientData, XtPointer callData);
// TODO: static void updateDialogFields(menuItemRec* f, userCmdDialog* ucd);
// TODO: static menuItemRec* readDialogFields(userCmdDialog* ucd, int silent);
// TODO: static menuItemRec* copyMenuItemRec(menuItemRec* item);
static void freeMenuItemRec(menuItemRec* item);
// TODO: static void* getDialogDataCB(void* oldItem, int explicitRequest, int* abort, void* cbArg);
// TODO: static void setDialogDataCB(void* item, void* cbArg);
// TODO: static void freeItemCB(void* item);
// TODO: static int dialogFieldsAreEmpty(userCmdDialog* ucd);
// TODO: static void disableTextW(Fl_Widget* textW);
static char* writeMenuItemString(menuItemRec** menuItems, int nItems, int listType);
static int loadMenuItemString(char* inString, menuItemRec** menuItems, int* nItems, int listType);
static void generateAcceleratorString(char* text, unsigned int modifiers, int keysym);
// TODO: static void genAccelEventName(char* text, unsigned int modifiers, int keysym);
static int parseAcceleratorString(const char* string, unsigned int* modifiers, int* keysym);
static int parseError(const char* message);
static char* copyMacroToEnd(char** inPtr);
static void addTerminatingNewline(char** string);
static void parseMenuItemList(menuItemRec** itemList, int nbrOfItems, userMenuInfo** infoList, userSubMenuCache* subMenus);
static int getSubMenuDepth(const char* menuName);
static userMenuInfo* parseMenuItemRec(menuItemRec* item);
static void parseMenuItemName(char* menuItemName, userMenuInfo* info);
static void generateUserMenuId(userMenuInfo* info, userSubMenuCache* subMenus);
static userSubMenuInfo* findSubMenuInfo(userSubMenuCache* subMenus, const char* hierName);
static char* stripLanguageMode(const char* menuItemName);
static void setDefaultIndex(userMenuInfo** infoList, int nbrOfItems, int defaultIdx);
static void applyLangModeToUserMenuInfo(userMenuInfo** infoList, int nbrOfItems, int languageMode);
static int doesLanguageModeMatch(userMenuInfo* info, int languageMode);
static void freeUserMenuInfoList(userMenuInfo** infoList, int nbrOfItems);
static void freeUserMenuInfo(userMenuInfo* info);
static void allocSubMenuCache(userSubMenuCache* subMenus, int nbrOfItems);
static void freeSubMenuCache(userSubMenuCache* subMenus);
static void allocUserMenuList(UserMenuList* list, int nbrOfItems);
static void freeUserMenuList(UserMenuList* list);
static UserMenuListElement* allocUserMenuListElement(Fl_Widget* menuItem, char* accKeys);
static void freeUserMenuListElement(UserMenuListElement* element);
static UserMenuList* allocUserSubMenuList(int nbrOfItems);
static void freeUserSubMenuList(UserMenuList* list);

// TODO: /*
// TODO: ** Present a dialog for editing the user specified commands in the shell menu
// TODO: */
// TODO: void EditShellMenu(WindowInfo* window)
// TODO: {
// TODO:    Fl_Widget* form, accLabel, inpLabel, inpBox, outBox, outLabel;
// TODO:    Fl_Widget* nameLabel, cmdLabel, okBtn, applyBtn, closeBtn;
// TODO:    userCmdDialog* ucd;
// TODO:    NeString s1;
// TODO:    int ac, i;
// TODO:    Arg args[20];
// TODO: 
// TODO:    /* if the dialog is already displayed, just pop it to the top and return */
// TODO:    if (ShellCmdDialog != NULL)
// TODO:    {
// TODO:       RaiseDialogWindow(ShellCmdDialog);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Create a structure for keeping track of dialog state */
// TODO:    ucd = (userCmdDialog*)malloc__(sizeof(userCmdDialog));
// TODO:    ucd->window = window;
// TODO: 
// TODO:    /* Set the dialog to operate on the Shell menu */
// TODO:    ucd->menuItemsList = (menuItemRec**)malloc__(sizeof(menuItemRec*) *
// TODO:                         MAX_ITEMS_PER_MENU);
// TODO:    for (i=0; i<NShellMenuItems; i++)
// TODO:       ucd->menuItemsList[i] = copyMenuItemRec(ShellMenuItems[i]);
// TODO:    ucd->nMenuItems = NShellMenuItems;
// TODO:    ucd->dialogType = SHELL_CMDS;
// TODO: 
// TODO:    ac = 0;
// TODO:    XtSetArg(args[ac], XmNdeleteResponse, XmDO_NOTHING);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNiconName, "NEdit Shell Menu");
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNtitle, "Shell Menu");
// TODO:    ac++;
// TODO:    ucd->dlogShell = CreateWidget(TheAppShell, "shellCommands",
// TODO:                                  topLevelShellWidgetClass, args, ac);
// TODO:    AddSmallIcon(ucd->dlogShell);
// TODO:    form = XtVaCreateManagedWidget("editShellCommands", xmFormWidgetClass,
// TODO:                                   ucd->dlogShell, XmNautoUnmanage, false,
// TODO:                                   XmNresizePolicy, XmRESIZE_NONE, NULL);
// TODO:    ShellCmdDialog = ucd->dlogShell;
// TODO:    XtAddCallback(form, XmNdestroyCallback, destroyCB, ucd);
// TODO:    AddMotifCloseCallback(ucd->dlogShell, closeCB, ucd);
// TODO: 
// TODO:    ac = 0;
// TODO:    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_POSITION);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNtopPosition, 2);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNleftPosition, LEFT_MARGIN_POS);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNrightPosition, LIST_RIGHT-1);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_POSITION);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNbottomPosition, SHELL_CMD_TOP);
// TODO:    ac++;
// TODO:    ucd->managedList = CreateManagedList(form, "list", args, ac,
// TODO:                                         (void**)ucd->menuItemsList, &ucd->nMenuItems, MAX_ITEMS_PER_MENU,
// TODO:                                         20, getDialogDataCB, ucd, setDialogDataCB, ucd, freeItemCB);
// TODO: 
// TODO:    ucd->loadAfterBtn = XtVaCreateManagedWidget("loadAfterBtn",
// TODO:                        xmToggleButtonWidgetClass, form,
// TODO:                        XmNlabelString, s1=MKSTRING("Re-load file after executing command"),
// TODO:                        XmNmnemonic, 'R',
// TODO:                        XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                        XmNset, false,
// TODO:                        XmNleftAttachment, XmATTACH_POSITION,
// TODO:                        XmNleftPosition, LIST_RIGHT,
// TODO:                        XmNrightAttachment, XmATTACH_POSITION,
// TODO:                        XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                        XmNbottomAttachment, XmATTACH_POSITION,
// TODO:                        XmNbottomPosition, SHELL_CMD_TOP, NULL);
// TODO:    NeStringFree(s1);
// TODO:    ucd->saveFirstBtn = XtVaCreateManagedWidget("saveFirstBtn",
// TODO:                        xmToggleButtonWidgetClass, form,
// TODO:                        XmNlabelString, s1=MKSTRING("Save file before executing command"),
// TODO:                        XmNmnemonic, 'f',
// TODO:                        XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                        XmNset, false,
// TODO:                        XmNleftAttachment, XmATTACH_POSITION,
// TODO:                        XmNleftPosition, LIST_RIGHT,
// TODO:                        XmNrightAttachment, XmATTACH_POSITION,
// TODO:                        XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                        XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                        XmNbottomWidget, ucd->loadAfterBtn, NULL);
// TODO:    NeStringFree(s1);
// TODO:    ucd->repInpBtn = XtVaCreateManagedWidget("repInpBtn",
// TODO:                     xmToggleButtonWidgetClass, form,
// TODO:                     XmNlabelString, s1=MKSTRING("Output replaces input"),
// TODO:                     XmNmnemonic, 'f',
// TODO:                     XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                     XmNset, false,
// TODO:                     XmNleftAttachment, XmATTACH_POSITION,
// TODO:                     XmNleftPosition, LIST_RIGHT,
// TODO:                     XmNrightAttachment, XmATTACH_POSITION,
// TODO:                     XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                     XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                     XmNbottomWidget, ucd->saveFirstBtn, NULL);
// TODO:    NeStringFree(s1);
// TODO:    outBox = XtVaCreateManagedWidget("outBox", xmRowColumnWidgetClass, form,
// TODO:                                     XmNpacking, XmPACK_TIGHT,
// TODO:                                     XmNorientation, XmHORIZONTAL,
// TODO:                                     XmNradioBehavior, true,
// TODO:                                     XmNradioAlwaysOne, true,
// TODO:                                     XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                     XmNleftPosition, LIST_RIGHT + 2,
// TODO:                                     XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                     XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                                     XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                                     XmNbottomWidget, ucd->repInpBtn,
// TODO:                                     XmNbottomOffset, 4, NULL);
// TODO:    ucd->sameOutBtn = XtVaCreateManagedWidget("sameOutBtn",
// TODO:                      xmToggleButtonWidgetClass, outBox,
// TODO:                      XmNlabelString, s1=MKSTRING("same document"),
// TODO:                      XmNmnemonic, 'm',
// TODO:                      XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                      XmNmarginHeight, 0,
// TODO:                      XmNset, true, NULL);
// TODO:    NeStringFree(s1);
// TODO:    XtAddCallback(ucd->sameOutBtn, XmNvalueChangedCallback, sameOutCB, ucd);
// TODO:    ucd->dlogOutBtn = XtVaCreateManagedWidget("dlogOutBtn",
// TODO:                      xmToggleButtonWidgetClass, outBox,
// TODO:                      XmNlabelString, s1=MKSTRING("dialog"),
// TODO:                      XmNmnemonic, 'g',
// TODO:                      XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                      XmNmarginHeight, 0,
// TODO:                      XmNset, false, NULL);
// TODO:    NeStringFree(s1);
// TODO:    ucd->winOutBtn = XtVaCreateManagedWidget("winOutBtn", xmToggleButtonWidgetClass,
// TODO:                     outBox,
// TODO:                     XmNlabelString, s1=MKSTRING("new document"),
// TODO:                     XmNmnemonic, 'n',
// TODO:                     XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                     XmNmarginHeight, 0,
// TODO:                     XmNset, false, NULL);
// TODO:    NeStringFree(s1);
// TODO:    outLabel = XtVaCreateManagedWidget("outLabel", xmLabelGadgetClass, form,
// TODO:                                       XmNlabelString, s1=MKSTRING("Command Output (stdout/stderr):"),
// TODO:                                       XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                       XmNmarginTop, 5,
// TODO:                                       XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                       XmNleftPosition, LIST_RIGHT,
// TODO:                                       XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                       XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                                       XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                                       XmNbottomWidget, outBox, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    inpBox = XtVaCreateManagedWidget("inpBox", xmRowColumnWidgetClass, form,
// TODO:                                     XmNpacking, XmPACK_TIGHT,
// TODO:                                     XmNorientation, XmHORIZONTAL,
// TODO:                                     XmNradioBehavior, true,
// TODO:                                     XmNradioAlwaysOne, true,
// TODO:                                     XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                     XmNleftPosition, LIST_RIGHT + 2,
// TODO:                                     XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                     XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                                     XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                                     XmNbottomWidget, outLabel, NULL);
// TODO:    ucd->selInpBtn = XtVaCreateManagedWidget("selInpBtn", xmToggleButtonWidgetClass,
// TODO:                     inpBox,
// TODO:                     XmNlabelString, s1=MKSTRING("selection"),
// TODO:                     XmNmnemonic, 's',
// TODO:                     XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                     XmNmarginHeight, 0,
// TODO:                     XmNset, true, NULL);
// TODO:    NeStringFree(s1);
// TODO:    ucd->winInpBtn = XtVaCreateManagedWidget("winInpBtn",
// TODO:                     xmToggleButtonWidgetClass, inpBox,
// TODO:                     XmNlabelString, s1=MKSTRING("document"),
// TODO:                     XmNmnemonic, 'w',
// TODO:                     XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                     XmNmarginHeight, 0,
// TODO:                     XmNset, false, NULL);
// TODO:    NeStringFree(s1);
// TODO:    ucd->eitherInpBtn = XtVaCreateManagedWidget("eitherInpBtn",
// TODO:                        xmToggleButtonWidgetClass, inpBox,
// TODO:                        XmNlabelString, s1=MKSTRING("either"),
// TODO:                        XmNmnemonic, 't',
// TODO:                        XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                        XmNmarginHeight, 0,
// TODO:                        XmNset, false, NULL);
// TODO:    NeStringFree(s1);
// TODO:    ucd->noInpBtn = XtVaCreateManagedWidget("noInpBtn",
// TODO:                                            xmToggleButtonWidgetClass, inpBox,
// TODO:                                            XmNlabelString, s1=MKSTRING("none"),
// TODO:                                            XmNmnemonic, 'o',
// TODO:                                            XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                            XmNmarginHeight, 0,
// TODO:                                            XmNset, false, NULL);
// TODO:    NeStringFree(s1);
// TODO:    inpLabel = XtVaCreateManagedWidget("inpLabel", xmLabelGadgetClass, form,
// TODO:                                       XmNlabelString, s1=MKSTRING("Command Input (stdin):"),
// TODO:                                       XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                       XmNmarginTop, 5,
// TODO:                                       XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                       XmNleftPosition, LIST_RIGHT,
// TODO:                                       XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                       XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                                       XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                                       XmNbottomWidget, inpBox, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    ucd->mneTextW = XtVaCreateManagedWidget("mne", xmTextWidgetClass, form,
// TODO:                                            XmNcolumns, 1,
// TODO:                                            XmNmaxLength, 1,
// TODO:                                            XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                            XmNleftPosition, RIGHT_MARGIN_POS-10,
// TODO:                                            XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                            XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                                            XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                                            XmNbottomWidget, inpLabel, NULL);
// TODO:    RemapDeleteKey(ucd->mneTextW);
// TODO: 
// TODO:    ucd->accTextW = XtVaCreateManagedWidget("acc", xmTextWidgetClass, form,
// TODO:                                            XmNcolumns, 12,
// TODO:                                            XmNmaxLength, MAX_ACCEL_LEN-1,
// TODO:                                            XmNcursorPositionVisible, false,
// TODO:                                            XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                            XmNleftPosition, LIST_RIGHT,
// TODO:                                            XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                            XmNrightPosition, RIGHT_MARGIN_POS-15,
// TODO:                                            XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                                            XmNbottomWidget, inpLabel, NULL);
// TODO:    XtAddEventHandler(ucd->accTextW, KeyPressMask, false,
// TODO:                      (XtEventHandler)accKeyCB, ucd);
// TODO:    XtAddCallback(ucd->accTextW, XmNfocusCallback, accFocusCB, ucd);
// TODO:    XtAddCallback(ucd->accTextW, XmNlosingFocusCallback, accLoseFocusCB, ucd);
// TODO:    accLabel = XtVaCreateManagedWidget("accLabel", xmLabelGadgetClass, form,
// TODO:                                       XmNlabelString, s1=MKSTRING("Accelerator"),
// TODO:                                       XmNmnemonic, 'l',
// TODO:                                       XmNuserData, ucd->accTextW,
// TODO:                                       XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                       XmNmarginTop, 5,
// TODO:                                       XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                       XmNleftPosition, LIST_RIGHT,
// TODO:                                       XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                       XmNrightPosition, LIST_RIGHT + 24,
// TODO:                                       XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                                       XmNbottomWidget, ucd->mneTextW, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    XtVaCreateManagedWidget("mneLabel", xmLabelGadgetClass, form,
// TODO:                            XmNlabelString, s1=MKSTRING("Mnemonic"),
// TODO:                            XmNmnemonic, 'i',
// TODO:                            XmNuserData, ucd->mneTextW,
// TODO:                            XmNalignment, XmALIGNMENT_END,
// TODO:                            XmNmarginTop, 5,
// TODO:                            XmNleftAttachment, XmATTACH_POSITION,
// TODO:                            XmNleftPosition, LIST_RIGHT + 24,
// TODO:                            XmNrightAttachment, XmATTACH_POSITION,
// TODO:                            XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                            XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                            XmNbottomWidget, ucd->mneTextW, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    ucd->nameTextW = XtVaCreateManagedWidget("name", xmTextWidgetClass, form,
// TODO:                     XmNleftAttachment, XmATTACH_POSITION,
// TODO:                     XmNleftPosition, LIST_RIGHT,
// TODO:                     XmNrightAttachment, XmATTACH_POSITION,
// TODO:                     XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                     XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                     XmNbottomWidget, accLabel, NULL);
// TODO:    RemapDeleteKey(ucd->nameTextW);
// TODO: 
// TODO:    nameLabel = XtVaCreateManagedWidget("nameLabel", xmLabelGadgetClass, form,
// TODO:                                        XmNlabelString, s1=MKSTRING("Menu Entry"),
// TODO:                                        XmNmnemonic, 'y',
// TODO:                                        XmNuserData, ucd->nameTextW,
// TODO:                                        XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                        XmNmarginTop, 5,
// TODO:                                        XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                        XmNleftPosition, LIST_RIGHT,
// TODO:                                        XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                                        XmNbottomWidget, ucd->nameTextW, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    XtVaCreateManagedWidget("nameNotes", xmLabelGadgetClass, form,
// TODO:                            XmNlabelString, s1=MKSTRING("(> for sub-menu, @ language mode)"),
// TODO:                            XmNalignment, XmALIGNMENT_END,
// TODO:                            XmNmarginTop, 5,
// TODO:                            XmNleftAttachment, XmATTACH_WIDGET,
// TODO:                            XmNleftWidget, nameLabel,
// TODO:                            XmNrightAttachment, XmATTACH_POSITION,
// TODO:                            XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                            XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                            XmNbottomWidget, ucd->nameTextW, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    XtVaCreateManagedWidget("topLabel", xmLabelGadgetClass, form,
// TODO:                            XmNlabelString, s1=MKSTRING(
// TODO:                                     "Select a shell menu item from the list at left.\n\
// TODO: Select \"New\" to add a new command to the menu."),
// TODO:                            XmNtopAttachment, XmATTACH_POSITION,
// TODO:                            XmNtopPosition, 2,
// TODO:                            XmNleftAttachment, XmATTACH_POSITION,
// TODO:                            XmNleftPosition, LIST_RIGHT,
// TODO:                            XmNrightAttachment, XmATTACH_POSITION,
// TODO:                            XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                            XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                            XmNbottomWidget, nameLabel, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    cmdLabel = XtVaCreateManagedWidget("cmdLabel", xmLabelGadgetClass, form,
// TODO:                                       XmNlabelString, s1=MKSTRING("Shell Command to Execute"),
// TODO:                                       XmNmnemonic, 'x',
// TODO:                                       XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                       XmNmarginTop, 5,
// TODO:                                       XmNtopAttachment, XmATTACH_POSITION,
// TODO:                                       XmNtopPosition, SHELL_CMD_TOP,
// TODO:                                       XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                       XmNleftPosition, LEFT_MARGIN_POS, NULL);
// TODO:    NeStringFree(s1);
// TODO:    XtVaCreateManagedWidget("cmdLabel", xmLabelGadgetClass, form,
// TODO:                            XmNlabelString, s1=MKSTRING("(% expands to current filename, # to line number)"),
// TODO:                            XmNalignment, XmALIGNMENT_END,
// TODO:                            XmNmarginTop, 5,
// TODO:                            XmNtopAttachment, XmATTACH_POSITION,
// TODO:                            XmNtopPosition, SHELL_CMD_TOP,
// TODO:                            XmNleftAttachment, XmATTACH_WIDGET,
// TODO:                            XmNleftWidget, cmdLabel,
// TODO:                            XmNrightAttachment, XmATTACH_POSITION,
// TODO:                            XmNrightPosition, RIGHT_MARGIN_POS, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    okBtn = XtVaCreateManagedWidget("ok",xmPushButtonWidgetClass,form,
// TODO:                                    XmNlabelString, s1=MKSTRING("OK"),
// TODO:                                    XmNmarginWidth, BUTTON_WIDTH_MARGIN,
// TODO:                                    XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                    XmNleftPosition, 13,
// TODO:                                    XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                    XmNrightPosition, 29,
// TODO:                                    XmNbottomAttachment, XmATTACH_POSITION,
// TODO:                                    XmNbottomPosition, 99, NULL);
// TODO:    XtAddCallback(okBtn, XmNactivateCallback, okCB, ucd);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    applyBtn = XtVaCreateManagedWidget("apply",xmPushButtonWidgetClass,form,
// TODO:                                       XmNlabelString, s1=MKSTRING("Apply"),
// TODO:                                       XmNmnemonic, 'A',
// TODO:                                       XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                       XmNleftPosition, 42,
// TODO:                                       XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                       XmNrightPosition, 58,
// TODO:                                       XmNbottomAttachment, XmATTACH_POSITION,
// TODO:                                       XmNbottomPosition, 99, NULL);
// TODO:    XtAddCallback(applyBtn, XmNactivateCallback, applyCB, ucd);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    closeBtn = XtVaCreateManagedWidget("close",
// TODO:                                       xmPushButtonWidgetClass, form,
// TODO:                                       XmNlabelString, s1=MKSTRING("Close"),
// TODO:                                       XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                       XmNleftPosition, 71,
// TODO:                                       XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                       XmNrightPosition, 87,
// TODO:                                       XmNbottomAttachment, XmATTACH_POSITION,
// TODO:                                       XmNbottomPosition, 99,
// TODO:                                       NULL);
// TODO:    XtAddCallback(closeBtn, XmNactivateCallback, closeCB, ucd);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    ac = 0;
// TODO:    XtSetArg(args[ac], XmNeditMode, XmMULTI_LINE_EDIT);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNscrollHorizontal, false);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNwordWrap, true);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNtopWidget, cmdLabel);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNleftPosition, LEFT_MARGIN_POS);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNrightPosition, RIGHT_MARGIN_POS);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_WIDGET);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNbottomWidget, okBtn);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNbottomOffset, 5);
// TODO:    ac++;
// TODO:    ucd->cmdTextW = XmCreateScrolledText(form, "name", args, ac);
// TODO:    AddMouseWheelSupport(ucd->cmdTextW);
// TODO:    XtManageChild(ucd->cmdTextW);
// TODO:    MakeSingleLineTextW(ucd->cmdTextW);
// TODO:    RemapDeleteKey(ucd->cmdTextW);
// TODO:    XtVaSetValues(cmdLabel, XmNuserData, ucd->cmdTextW, NULL); /* for mnemonic */
// TODO: 
// TODO:    /* Disable text input for the accelerator key widget, let the
// TODO:       event handler manage it instead */
// TODO:    disableTextW(ucd->accTextW);
// TODO: 
// TODO:    /* initializs the dialog fields to match "New" list item */
// TODO:    updateDialogFields(NULL, ucd);
// TODO: 
// TODO:    /* Set initial default button */
// TODO:    XtVaSetValues(form, XmNdefaultButton, okBtn, NULL);
// TODO:    XtVaSetValues(form, XmNcancelButton, closeBtn, NULL);
// TODO: 
// TODO:    /* Handle mnemonic selection of buttons and focus to dialog */
// TODO:    AddDialogMnemonicHandler(form, FALSE);
// TODO: 
// TODO:    /* realize all of the widgets in the new window */
// TODO:    RealizeWithoutForcingPosition(ucd->dlogShell);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Present a dialogs for editing the user specified commands in the Macro
// TODO: ** and background menus
// TODO: */
// TODO: void EditMacroMenu(WindowInfo* window)
// TODO: {
// TODO:    editMacroOrBGMenu(window, MACRO_CMDS);
// TODO: }
// TODO: void EditBGMenu(WindowInfo* window)
// TODO: {
// TODO:    editMacroOrBGMenu(window, BG_MENU_CMDS);
// TODO: }
// TODO: 
// TODO: static void editMacroOrBGMenu(WindowInfo* window, int dialogType)
// TODO: {
// TODO:    Fl_Widget* form, accLabel, pasteReplayBtn;
// TODO:    Fl_Widget* nameLabel, cmdLabel, okBtn, applyBtn, closeBtn;
// TODO:    userCmdDialog* ucd;
// TODO:    char* title;
// TODO:    NeString s1;
// TODO:    int ac, i;
// TODO:    Arg args[20];
// TODO: 
// TODO:    /* if the dialog is already displayed, just pop it to the top and return */
// TODO:    if (dialogType == MACRO_CMDS && MacroCmdDialog != NULL)
// TODO:    {
// TODO:       RaiseDialogWindow(MacroCmdDialog);
// TODO:       return;
// TODO:    }
// TODO:    if (dialogType == BG_MENU_CMDS && BGMenuCmdDialog != NULL)
// TODO:    {
// TODO:       RaiseDialogWindow(BGMenuCmdDialog);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Create a structure for keeping track of dialog state */
// TODO:    ucd = (userCmdDialog*)malloc__(sizeof(userCmdDialog));
// TODO:    ucd->window = window;
// TODO: 
// TODO:    /* Set the dialog to operate on the Macro menu */
// TODO:    ucd->menuItemsList = (menuItemRec**)malloc__(sizeof(menuItemRec**) *
// TODO:                         MAX_ITEMS_PER_MENU);
// TODO:    if (dialogType == MACRO_CMDS)
// TODO:    {
// TODO:       for (i=0; i<NMacroMenuItems; i++)
// TODO:          ucd->menuItemsList[i] = copyMenuItemRec(MacroMenuItems[i]);
// TODO:       ucd->nMenuItems = NMacroMenuItems;
// TODO:    }
// TODO:    else     /* BG_MENU_CMDS */
// TODO:    {
// TODO:       for (i=0; i<NBGMenuItems; i++)
// TODO:          ucd->menuItemsList[i] = copyMenuItemRec(BGMenuItems[i]);
// TODO:       ucd->nMenuItems = NBGMenuItems;
// TODO:    }
// TODO:    ucd->dialogType = dialogType;
// TODO: 
// TODO:    title = dialogType == MACRO_CMDS ? "Macro Commands" :
// TODO:            "Window Background Menu";
// TODO:    ac = 0;
// TODO:    XtSetArg(args[ac], XmNdeleteResponse, XmDO_NOTHING);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNiconName, title);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNtitle, title);
// TODO:    ac++;
// TODO:    ucd->dlogShell = CreateWidget(TheAppShell, "macros",
// TODO:                                  topLevelShellWidgetClass, args, ac);
// TODO:    AddSmallIcon(ucd->dlogShell);
// TODO:    form = XtVaCreateManagedWidget("editMacroCommands", xmFormWidgetClass,
// TODO:                                   ucd->dlogShell, XmNautoUnmanage, false,
// TODO:                                   XmNresizePolicy, XmRESIZE_NONE, NULL);
// TODO:    XtAddCallback(form, XmNdestroyCallback, destroyCB, ucd);
// TODO:    AddMotifCloseCallback(ucd->dlogShell, closeCB, ucd);
// TODO: 
// TODO:    ac = 0;
// TODO:    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_POSITION);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNtopPosition, 2);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNleftPosition, LEFT_MARGIN_POS);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNrightPosition, LIST_RIGHT-1);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_POSITION);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNbottomPosition, MACRO_CMD_TOP);
// TODO:    ac++;
// TODO:    ucd->managedList = CreateManagedList(form, "list", args, ac,
// TODO:                                         (void**)ucd->menuItemsList, &ucd->nMenuItems, MAX_ITEMS_PER_MENU, 20,
// TODO:                                         getDialogDataCB, ucd, setDialogDataCB, ucd, freeItemCB);
// TODO: 
// TODO:    ucd->selInpBtn = XtVaCreateManagedWidget("selInpBtn",
// TODO:                     xmToggleButtonWidgetClass, form,
// TODO:                     XmNlabelString, s1=MKSTRING("Requires Selection"),
// TODO:                     XmNmnemonic, 'R',
// TODO:                     XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                     XmNmarginHeight, 0,
// TODO:                     XmNset, false,
// TODO:                     XmNleftAttachment, XmATTACH_POSITION,
// TODO:                     XmNleftPosition, LIST_RIGHT,
// TODO:                     XmNbottomAttachment, XmATTACH_POSITION,
// TODO:                     XmNbottomPosition, MACRO_CMD_TOP, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    ucd->mneTextW = XtVaCreateManagedWidget("mne", xmTextWidgetClass, form,
// TODO:                                            XmNcolumns, 1,
// TODO:                                            XmNmaxLength, 1,
// TODO:                                            XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                            XmNleftPosition, RIGHT_MARGIN_POS-21-5,
// TODO:                                            XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                            XmNrightPosition, RIGHT_MARGIN_POS-21,
// TODO:                                            XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                                            XmNbottomWidget, ucd->selInpBtn,
// TODO:                                            XmNbottomOffset, 5, NULL);
// TODO:    RemapDeleteKey(ucd->mneTextW);
// TODO: 
// TODO:    ucd->accTextW = XtVaCreateManagedWidget("acc", xmTextWidgetClass, form,
// TODO:                                            XmNcolumns, 12,
// TODO:                                            XmNmaxLength, MAX_ACCEL_LEN-1,
// TODO:                                            XmNcursorPositionVisible, false,
// TODO:                                            XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                            XmNleftPosition, LIST_RIGHT,
// TODO:                                            XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                            XmNrightPosition, RIGHT_MARGIN_POS-20-10,
// TODO:                                            XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                                            XmNbottomWidget, ucd->selInpBtn,
// TODO:                                            XmNbottomOffset, 5, NULL);
// TODO:    XtAddEventHandler(ucd->accTextW, KeyPressMask, false,
// TODO:                      (XtEventHandler)accKeyCB, ucd);
// TODO:    XtAddCallback(ucd->accTextW, XmNfocusCallback, accFocusCB, ucd);
// TODO:    XtAddCallback(ucd->accTextW, XmNlosingFocusCallback, accLoseFocusCB, ucd);
// TODO: 
// TODO:    accLabel = XtVaCreateManagedWidget("accLabel", xmLabelGadgetClass, form,
// TODO:                                       XmNlabelString, s1=MKSTRING("Accelerator"),
// TODO:                                       XmNmnemonic, 'l',
// TODO:                                       XmNuserData, ucd->accTextW,
// TODO:                                       XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                       XmNmarginTop, 5,
// TODO:                                       XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                       XmNleftPosition, LIST_RIGHT,
// TODO:                                       XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                       XmNrightPosition, LIST_RIGHT + 22,
// TODO:                                       XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                                       XmNbottomWidget, ucd->mneTextW, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    XtVaCreateManagedWidget("mneLabel", xmLabelGadgetClass, form,
// TODO:                            XmNlabelString, s1=MKSTRING("Mnemonic"),
// TODO:                            XmNmnemonic, 'i',
// TODO:                            XmNuserData, ucd->mneTextW,
// TODO:                            XmNalignment, XmALIGNMENT_END,
// TODO:                            XmNmarginTop, 5,
// TODO:                            XmNleftAttachment, XmATTACH_POSITION,
// TODO:                            XmNleftPosition, LIST_RIGHT + 22,
// TODO:                            XmNrightAttachment, XmATTACH_POSITION,
// TODO:                            XmNrightPosition, RIGHT_MARGIN_POS-21,
// TODO:                            XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                            XmNbottomWidget, ucd->mneTextW, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    pasteReplayBtn = XtVaCreateManagedWidget("pasteReplay",
// TODO:                     xmPushButtonWidgetClass, form,
// TODO:                     XmNlabelString, s1=MKSTRING("Paste Learn/\nReplay Macro"),
// TODO:                     XmNmnemonic, 'P',
// TODO:                     XmNsensitive, GetReplayMacro() != NULL,
// TODO:                     XmNleftAttachment, XmATTACH_POSITION,
// TODO:                     XmNleftPosition, RIGHT_MARGIN_POS-20,
// TODO:                     XmNrightAttachment, XmATTACH_POSITION,
// TODO:                     XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                     XmNbottomAttachment, XmATTACH_POSITION,
// TODO:                     XmNbottomPosition, MACRO_CMD_TOP, NULL);
// TODO:    XtAddCallback(pasteReplayBtn, XmNactivateCallback,
// TODO:                  pasteReplayCB, ucd);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    ucd->nameTextW = XtVaCreateManagedWidget("name", xmTextWidgetClass, form,
// TODO:                     XmNleftAttachment, XmATTACH_POSITION,
// TODO:                     XmNleftPosition, LIST_RIGHT,
// TODO:                     XmNrightAttachment, XmATTACH_POSITION,
// TODO:                     XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                     XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                     XmNbottomWidget, accLabel, NULL);
// TODO:    RemapDeleteKey(ucd->nameTextW);
// TODO: 
// TODO:    nameLabel = XtVaCreateManagedWidget("nameLabel", xmLabelGadgetClass, form,
// TODO:                                        XmNlabelString, s1=MKSTRING("Menu Entry"),
// TODO:                                        XmNmnemonic, 'y',
// TODO:                                        XmNuserData, ucd->nameTextW,
// TODO:                                        XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                        XmNmarginTop, 5,
// TODO:                                        XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                        XmNleftPosition, LIST_RIGHT,
// TODO:                                        XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                                        XmNbottomWidget, ucd->nameTextW, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    XtVaCreateManagedWidget("nameNotes", xmLabelGadgetClass, form,
// TODO:                            XmNlabelString, s1=MKSTRING("(> for sub-menu, @ language mode)"),
// TODO:                            XmNalignment, XmALIGNMENT_END,
// TODO:                            XmNmarginTop, 5,
// TODO:                            XmNleftAttachment, XmATTACH_WIDGET,
// TODO:                            XmNleftWidget, nameLabel,
// TODO:                            XmNrightAttachment, XmATTACH_POSITION,
// TODO:                            XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                            XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                            XmNbottomWidget, ucd->nameTextW, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    XtVaCreateManagedWidget("topLabel", xmLabelGadgetClass, form,
// TODO:                            XmNlabelString, s1=MKSTRING(
// TODO:                                     "Select a macro menu item from the list at left.\n\
// TODO: Select \"New\" to add a new command to the menu."),
// TODO:                            XmNtopAttachment, XmATTACH_POSITION,
// TODO:                            XmNtopPosition, 2,
// TODO:                            XmNleftAttachment, XmATTACH_POSITION,
// TODO:                            XmNleftPosition, LIST_RIGHT,
// TODO:                            XmNrightAttachment, XmATTACH_POSITION,
// TODO:                            XmNrightPosition, RIGHT_MARGIN_POS,
// TODO:                            XmNbottomAttachment, XmATTACH_WIDGET,
// TODO:                            XmNbottomWidget, nameLabel, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    cmdLabel = XtVaCreateManagedWidget("cmdLabel", xmLabelGadgetClass, form,
// TODO:                                       XmNlabelString, s1=MKSTRING("Macro Command to Execute"),
// TODO:                                       XmNmnemonic, 'x',
// TODO:                                       XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                       XmNmarginTop, 5,
// TODO:                                       XmNtopAttachment, XmATTACH_POSITION,
// TODO:                                       XmNtopPosition, MACRO_CMD_TOP,
// TODO:                                       XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                       XmNleftPosition, LEFT_MARGIN_POS, NULL);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    okBtn = XtVaCreateManagedWidget("ok",xmPushButtonWidgetClass,form,
// TODO:                                    XmNlabelString, s1=MKSTRING("OK"),
// TODO:                                    XmNmarginWidth, BUTTON_WIDTH_MARGIN,
// TODO:                                    XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                    XmNleftPosition, 8,
// TODO:                                    XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                    XmNrightPosition, 23,
// TODO:                                    XmNbottomAttachment, XmATTACH_POSITION,
// TODO:                                    XmNbottomPosition, 99, NULL);
// TODO:    XtAddCallback(okBtn, XmNactivateCallback, okCB, ucd);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    applyBtn = XtVaCreateManagedWidget("apply",xmPushButtonWidgetClass,form,
// TODO:                                       XmNlabelString, s1=MKSTRING("Apply"),
// TODO:                                       XmNmnemonic, 'A',
// TODO:                                       XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                       XmNleftPosition, 31,
// TODO:                                       XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                       XmNrightPosition, 46,
// TODO:                                       XmNbottomAttachment, XmATTACH_POSITION,
// TODO:                                       XmNbottomPosition, 99, NULL);
// TODO:    XtAddCallback(applyBtn, XmNactivateCallback, applyCB, ucd);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    applyBtn = XtVaCreateManagedWidget("check",xmPushButtonWidgetClass,form,
// TODO:                                       XmNlabelString, s1=MKSTRING("Check"),
// TODO:                                       XmNmnemonic, 'C',
// TODO:                                       XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                       XmNleftPosition, 54,
// TODO:                                       XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                       XmNrightPosition, 69,
// TODO:                                       XmNbottomAttachment, XmATTACH_POSITION,
// TODO:                                       XmNbottomPosition, 99, NULL);
// TODO:    XtAddCallback(applyBtn, XmNactivateCallback, checkCB, ucd);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    closeBtn = XtVaCreateManagedWidget("close",
// TODO:                                       xmPushButtonWidgetClass, form,
// TODO:                                       XmNlabelString, s1=MKSTRING("Close"),
// TODO:                                       XmNleftAttachment, XmATTACH_POSITION,
// TODO:                                       XmNleftPosition, 77,
// TODO:                                       XmNrightAttachment, XmATTACH_POSITION,
// TODO:                                       XmNrightPosition, 92,
// TODO:                                       XmNbottomAttachment, XmATTACH_POSITION,
// TODO:                                       XmNbottomPosition, 99,
// TODO:                                       NULL);
// TODO:    XtAddCallback(closeBtn, XmNactivateCallback, closeCB, ucd);
// TODO:    NeStringFree(s1);
// TODO: 
// TODO:    ac = 0;
// TODO:    XtSetArg(args[ac], XmNeditMode, XmMULTI_LINE_EDIT);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNtopWidget, cmdLabel);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNleftPosition, LEFT_MARGIN_POS);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNrightPosition, RIGHT_MARGIN_POS);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_WIDGET);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNbottomWidget, okBtn);
// TODO:    ac++;
// TODO:    XtSetArg(args[ac], XmNbottomOffset, 5);
// TODO:    ac++;
// TODO:    ucd->cmdTextW = XmCreateScrolledText(form, "name", args, ac);
// TODO:    AddMouseWheelSupport(ucd->cmdTextW);
// TODO:    XtManageChild(ucd->cmdTextW);
// TODO:    RemapDeleteKey(ucd->cmdTextW);
// TODO:    XtVaSetValues(cmdLabel, XmNuserData, ucd->cmdTextW, NULL); /* for mnemonic */
// TODO: 
// TODO:    /* Disable text input for the accelerator key widget, let the
// TODO:       event handler manage it instead */
// TODO:    disableTextW(ucd->accTextW);
// TODO: 
// TODO:    /* initializs the dialog fields to match "New" list item */
// TODO:    updateDialogFields(NULL, ucd);
// TODO: 
// TODO:    /* Set initial default button */
// TODO:    XtVaSetValues(form, XmNdefaultButton, okBtn, NULL);
// TODO:    XtVaSetValues(form, XmNcancelButton, closeBtn, NULL);
// TODO: 
// TODO:    /* Handle mnemonic selection of buttons and focus to dialog */
// TODO:    AddDialogMnemonicHandler(form, FALSE);
// TODO: 
// TODO:    /* Make widgets for top level shell and paste-replay buttons available
// TODO:       to other functions */
// TODO:    if (dialogType == MACRO_CMDS)
// TODO:    {
// TODO:       MacroCmdDialog = ucd->dlogShell;
// TODO:       MacroPasteReplayBtn = pasteReplayBtn;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       BGMenuCmdDialog = ucd->dlogShell;
// TODO:       BGMenuPasteReplayBtn = pasteReplayBtn;
// TODO:    }
// TODO: 
// TODO:    /* Realize all of the widgets in the new dialog */
// TODO:    RealizeWithoutForcingPosition(ucd->dlogShell);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Update the Shell, Macro, and Window Background menus of window
// TODO: ** "window" from the currently loaded command descriptions.
// TODO: */
// TODO: void UpdateUserMenus(WindowInfo* window)
// TODO: {
// TODO:    if (!IsTopDocument(window))
// TODO:       return;
// TODO: 
// TODO:    /* update user menus, which are shared over all documents, only
// TODO:       if language mode was changed */
// TODO:    if (window->userMenuCache->umcLanguageMode != window->languageMode)
// TODO:    {
// TODO: #ifndef VMS
// TODO:       updateMenu(window, SHELL_CMDS);
// TODO: #endif
// TODO:       updateMenu(window, MACRO_CMDS);
// TODO: 
// TODO:       /* remember language mode assigned to shared user menus */
// TODO:       window->userMenuCache->umcLanguageMode = window->languageMode;
// TODO:    }
// TODO: 
// TODO:    /* update background menu, which is owned by a single document, only
// TODO:       if language mode was changed */
// TODO:    if (window->userBGMenuCache.ubmcLanguageMode != window->languageMode)
// TODO:    {
// TODO:       updateMenu(window, BG_MENU_CMDS);
// TODO: 
// TODO:       /* remember language mode assigned to background menu */
// TODO:       window->userBGMenuCache.ubmcLanguageMode = window->languageMode;
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Dim/undim buttons for pasting replay macros into macro and bg menu dialogs
// TODO: */
// TODO: void DimPasteReplayBtns(int sensitive)
// TODO: {
// TODO:    if (MacroCmdDialog != NULL)
// TODO:       NeSetSensitive(MacroPasteReplayBtn, sensitive);
// TODO:    if (BGMenuCmdDialog != NULL)
// TODO:       NeSetSensitive(BGMenuPasteReplayBtn, sensitive);
// TODO: }

/*
** Dim/undim user programmable menu items which depend on there being
** a selection in their associated window.
*/
void DimSelectionDepUserMenuItems(WindowInfo* window, int sensitive)
{
   if (!IsTopDocument(window))
      return;

   dimSelDepItemsInMenu(window->shellMenuPane, ShellMenuItems, NShellMenuItems, sensitive);
   dimSelDepItemsInMenu(window->macroMenuPane, MacroMenuItems, NMacroMenuItems, sensitive);
   dimSelDepItemsInMenu(window->bgMenuPane, BGMenuItems, NBGMenuItems, sensitive);
}

static void dimSelDepItemsInMenu(Fl_Widget* menuPane, menuItemRec** menuList, int nMenuItems, int sensitive)
{
// TODO:    WidgetList items;
// TODO:    Fl_Widget* subMenu;
// TODO:    XtPointer userData;
// TODO:    int n, index;
// TODO:    Cardinal nItems;
// TODO: 
// TODO:    XtVaGetValues(menuPane, XmNchildren, &items, XmNnumChildren, &nItems, NULL);
// TODO:    for (n=0; n<(int)nItems; n++)
// TODO:    {
// TODO:       XtVaGetValues(items[n], XmNuserData, &userData, NULL);
// TODO:       if (userData != (XtPointer)PERMANENT_MENU_ITEM)
// TODO:       {
// TODO:          if (XtClass(items[n]) == xmCascadeButtonWidgetClass)
// TODO:          {
// TODO:             XtVaGetValues(items[n], XmNsubMenuId, &subMenu, NULL);
// TODO:             dimSelDepItemsInMenu(subMenu, menuList, nMenuItems, sensitive);
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             index = (int)userData - 10;
// TODO:             if (index <0 || index >= nMenuItems)
// TODO:                return;
// TODO:             if (menuList[index]->input == FROM_SELECTION)
// TODO:                NeSetSensitive(items[n], sensitive);
// TODO:          }
// TODO:       }
// TODO:    }
}

/*
** Harmless kludge for making undo/redo menu items in background menu properly
** sensitive (even though they're programmable) along with real undo item
** in the Edit menu
*/
void SetBGMenuUndoSensitivity(WindowInfo* window, bool sensitive)
{
   if (window->bgMenuUndoItem != NULL)
      SetSensitive(window, window->bgMenuUndoItem, sensitive);
}
void SetBGMenuRedoSensitivity(WindowInfo* window, bool sensitive)
{
   if (window->bgMenuRedoItem != NULL)
      SetSensitive(window, window->bgMenuRedoItem, sensitive);
}

/*
** Generate a text string for the preferences file describing the contents
** of the shell cmd list.  This string is not exactly of the form that it
** can be read by LoadShellCmdsString, rather, it is what needs to be written
** to a resource file such that it will read back in that form.
*/
char* WriteShellCmdsString()
{
   return writeMenuItemString(ShellMenuItems, NShellMenuItems, SHELL_CMDS);
}

/*
** Generate a text string for the preferences file describing the contents of
** the macro menu and background menu commands lists.  These strings are not
** exactly of the form that it can be read by LoadMacroCmdsString, rather, it
** is what needs to be written to a resource file such that it will read back
** in that form.
*/
char* WriteMacroCmdsString()
{
   return writeMenuItemString(MacroMenuItems, NMacroMenuItems, MACRO_CMDS);
}

char* WriteBGMenuCmdsString()
{
   return writeMenuItemString(BGMenuItems, NBGMenuItems, BG_MENU_CMDS);
}

/*
** Read a string representing shell command menu items and add them to the
** internal list used for constructing shell menus
*/
int LoadShellCmdsString(char* inString)
{
   return loadMenuItemString(inString, ShellMenuItems, &NShellMenuItems, SHELL_CMDS);
}

/*
** Read strings representing macro menu or background menu command menu items
** and add them to the internal lists used for constructing menus
*/
int LoadMacroCmdsString(char* inString)
{
   return loadMenuItemString(inString, MacroMenuItems, &NMacroMenuItems, MACRO_CMDS);
}

int LoadBGMenuCmdsString(char* inString)
{
   return loadMenuItemString(inString, BGMenuItems, &NBGMenuItems, BG_MENU_CMDS);
}

/*
** Cache user menus:
** Setup user menu info after read of macro, shell and background menu
** string (reason: language mode info from preference string is read *after*
** user menu preference string was read).
*/
void SetupUserMenuInfo()
{
   parseMenuItemList(ShellMenuItems, NShellMenuItems, ShellMenuInfo, &ShellSubMenus);
   parseMenuItemList(MacroMenuItems, NMacroMenuItems, MacroMenuInfo, &MacroSubMenus);
   parseMenuItemList(BGMenuItems   , NBGMenuItems   , BGMenuInfo   , &BGSubMenus);
}

/*
** Cache user menus:
** Update user menu info to take into account e.g. change of language modes
** (i.e. add / move / delete of language modes etc).
*/
void UpdateUserMenuInfo()
{
   freeUserMenuInfoList(ShellMenuInfo, NShellMenuItems);
   freeSubMenuCache(&ShellSubMenus);
   parseMenuItemList(ShellMenuItems, NShellMenuItems, ShellMenuInfo, &ShellSubMenus);

   freeUserMenuInfoList(MacroMenuInfo, NMacroMenuItems);
   freeSubMenuCache(&MacroSubMenus);
   parseMenuItemList(MacroMenuItems, NMacroMenuItems, MacroMenuInfo, &MacroSubMenus);

   freeUserMenuInfoList(BGMenuInfo, NBGMenuItems);
   freeSubMenuCache(&BGSubMenus);
   parseMenuItemList(BGMenuItems, NBGMenuItems, BGMenuInfo, &BGSubMenus);
}

// TODO: /*
// TODO: ** Search through the shell menu and execute the first command with menu item
// TODO: ** name "itemName".  Returns true on successs and false on failure.
// TODO: */
// TODO: #ifndef VMS
// TODO: int DoNamedShellMenuCmd(WindowInfo* window, const char* itemName, int fromMacro)
// TODO: {
// TODO:    int i;
// TODO: 
// TODO:    for (i=0; i<NShellMenuItems; i++)
// TODO:    {
// TODO:       if (!strcmp(ShellMenuItems[i]->name, itemName))
// TODO:       {
// TODO:          if (ShellMenuItems[i]->output == TO_SAME_WINDOW &&
// TODO:                CheckReadOnly(window))
// TODO:             return false;
// TODO:          DoShellMenuCmd(window, ShellMenuItems[i]->cmd,
// TODO:                         ShellMenuItems[i]->input, ShellMenuItems[i]->output,
// TODO:                         ShellMenuItems[i]->repInput, ShellMenuItems[i]->saveFirst,
// TODO:                         ShellMenuItems[i]->loadAfter, fromMacro);
// TODO:          return true;
// TODO:       }
// TODO:    }
// TODO:    return false;
// TODO: }
// TODO: #endif /*VMS*/
// TODO: 
// TODO: /*
// TODO: ** Search through the Macro or background menu and execute the first command
// TODO: ** with menu item name "itemName".  Returns true on successs and false on
// TODO: ** failure.
// TODO: */
// TODO: int DoNamedMacroMenuCmd(WindowInfo* window, const char* itemName)
// TODO: {
// TODO:    int i;
// TODO: 
// TODO:    for (i=0; i<NMacroMenuItems; i++)
// TODO:    {
// TODO:       if (!strcmp(MacroMenuItems[i]->name, itemName))
// TODO:       {
// TODO:          DoMacro(window, MacroMenuItems[i]->cmd, "macro menu command");
// TODO:          return true;
// TODO:       }
// TODO:    }
// TODO:    return false;
// TODO: }
// TODO: 
// TODO: int DoNamedBGMenuCmd(WindowInfo* window, const char* itemName)
// TODO: {
// TODO:    int i;
// TODO: 
// TODO:    for (i=0; i<NBGMenuItems; i++)
// TODO:    {
// TODO:       if (!strcmp(BGMenuItems[i]->name, itemName))
// TODO:       {
// TODO:          DoMacro(window, BGMenuItems[i]->cmd, "background menu macro");
// TODO:          return true;
// TODO:       }
// TODO:    }
// TODO:    return false;
// TODO: }

/*
** Cache user menus:
** Rebuild all of the Shell, Macro, Background menus of given editor window.
*/
void RebuildAllMenus(WindowInfo* window)
{
   rebuildMenu(window, SHELL_CMDS);
   rebuildMenu(window, MACRO_CMDS);
   rebuildMenu(window, BG_MENU_CMDS);
}

// TODO: /*
// TODO: ** Cache user menus:
// TODO: ** Rebuild either Shell, Macro or Background menus of all editor windows.
// TODO: */
// TODO: static void rebuildMenuOfAllWindows(int menuType)
// TODO: {
// TODO:    WindowInfo* w;
// TODO: 
// TODO:    for (w=WindowList; w!=NULL; w=w->next)
// TODO:       rebuildMenu(w, menuType);
// TODO: }

/*
** Rebuild either the Shell, Macro or Background menu of "window", depending
** on value of "menuType". Rebuild is realized by following main steps:
** - dismiss user (sub) menu tearoff.
** - delete all user defined menu widgets.
** - update user menu including (re)creation of menu widgets.
*/
static void rebuildMenu(WindowInfo* window, int menuType)
{
   selectedUserMenu  menu;

   /* Background menu is always rebuild (exists once per document).
      Shell, macro (user) menu cache is rebuild only, if given window is
      currently displayed on top. */
   if (menuType != BG_MENU_CMDS && !IsTopDocument(window))
      return;

   /* Fetch the appropriate menu data */
   selectUserMenu(window, menuType, &menu);

   /* destroy all widgets related to menu pane */
   deleteMenuItems(menu.sumMenuPane);

   /* remove cached user menu info */
   freeUserMenuList(menu.sumMainMenuList);
   *menu.sumMenuCreated = false;

   /* re-create & cache user menu items */
   updateMenu(window, menuType);
}

/*
** Fetch the appropriate menu info for given menu type
*/
static void selectUserMenu(WindowInfo* window, int menuType, selectedUserMenu* menu)
{
   if (menuType == SHELL_CMDS)
   {
// TODO:       menu->sumMenuPane       = window->mainWindowMenuPane;
      menu->sumNbrOfListItems = NShellMenuItems;
      menu->sumItemList       = ShellMenuItems;
      menu->sumInfoList       = ShellMenuInfo;
      menu->sumSubMenus       = &ShellSubMenus;
      menu->sumMainMenuList   = &window->userMenuCache->umcShellMenuList;
      menu->sumMenuCreated    = &window->userMenuCache->umcShellMenuCreated;
   }
   else if (menuType == MACRO_CMDS)
   {
      menu->sumMenuPane       = window->macroMenuPane;
      menu->sumNbrOfListItems = NMacroMenuItems;
      menu->sumItemList       = MacroMenuItems;
      menu->sumInfoList       = MacroMenuInfo;
      menu->sumSubMenus       = &MacroSubMenus;
      menu->sumMainMenuList   = &window->userMenuCache->umcMacroMenuList;
      menu->sumMenuCreated    = &window->userMenuCache->umcMacroMenuCreated;
   }
   else     /* BG_MENU_CMDS */
   {
      menu->sumMenuPane       = window->bgMenuPane;
      menu->sumNbrOfListItems = NBGMenuItems;
      menu->sumItemList       = BGMenuItems;
      menu->sumInfoList       = BGMenuInfo;
      menu->sumSubMenus       = &BGSubMenus;
      menu->sumMainMenuList   = &window->userBGMenuCache.ubmcMenuList;
      menu->sumMenuCreated    = &window->userBGMenuCache.ubmcMenuCreated;
   }
   menu->sumType = menuType;
}

/*
** Updates either the Shell, Macro or Background menu of "window", depending
** on value of "menuType". Update is realized by following main steps:
** - set / reset "to be managed" flag of user menu info list items
**   according to current selected language mode.
** - create *all* user menu items (widgets etc). related to given
**   window & menu type, if not done before.
** - manage / unmanage user menu widgets according to "to be managed"
**   indication of user menu info list items.
*/
static void updateMenu(WindowInfo* window, int menuType)
{
   selectedUserMenu menu;

   /* Fetch the appropriate menu data */
   selectUserMenu(window, menuType, &menu);

   /* Set / reset "to be managed" flag of all info list items */
   applyLangModeToUserMenuInfo(menu.sumInfoList, menu.sumNbrOfListItems, window->languageMode);

   /* create user menu items, if not done before */
   if (!*menu.sumMenuCreated)
      createMenuItems(window, &menu);

   /* manage user menu items depending on current language mode */
   manageUserMenu(&menu, window);

   if (menuType == BG_MENU_CMDS)
   {
// TODO:       /* Set the proper sensitivity of items which may be dimmed */
// TODO:       SetBGMenuUndoSensitivity(window, XtIsSensitive(window->undoItem));
// TODO:       SetBGMenuRedoSensitivity(window, XtIsSensitive(window->redoItem));
   }

   DimSelectionDepUserMenuItems(window, window->buffer->primary.selected);
}

// TODO: /*
// TODO: ** Manually adjust the dimension of the menuShell _before_
// TODO: ** re-managing the menu pane, to either expose hidden menu
// TODO: ** entries or remove empty space.
// TODO: */
// TODO: static void manageTearOffMenu(Fl_Widget* menuPane)
// TODO: {
// TODO:    Dimension width, height, border;
// TODO: 
// TODO:    /* somehow OM went into a long CPU cycling when we
// TODO:       attempt to change the shell window dimension by
// TODO:       setting the XmNwidth & XmNheight directly. Using
// TODO:       XtResizeWidget() seem to fix it */
// TODO:    XtVaGetValues(XtParent(menuPane), XmNborderWidth, &border, NULL);
// TODO:    XtVaGetValues(menuPane, XmNwidth, &width, XmNheight, &height, NULL);
// TODO:    XtResizeWidget(XtParent(menuPane), width, height, border);
// TODO: 
// TODO:    XtManageChild(menuPane);
// TODO: }

/*
** Cache user menus:
** Reset manage mode of user menu items in window cache.
*/
static void resetManageMode(UserMenuList* list)
{
   int i;
   UserMenuListElement* element;

   for (i=0; i<list->umlNbrItems; i ++)
   {
      element = list->umlItems[i];

      /* remember current manage mode before reset it to "unmanaged" */
      element->umlePrevManageMode = element->umleManageMode;
      element->umleManageMode     = UMMM_UNMANAGE;

      /* recursively reset manage mode of sub-menus */
      if (element->umleSubMenuList != NULL)
         resetManageMode(element->umleSubMenuList);
   }
}

// TODO: /*
// TODO: ** Cache user menus:
// TODO: ** Manage all menu widgets of given user sub-menu list.
// TODO: */
// TODO: static void manageAllSubMenuWidgets(UserMenuListElement* subMenu)
// TODO: {
// TODO:    int i;
// TODO:    UserMenuList* subMenuList;
// TODO:    UserMenuListElement* element;
// TODO:    WidgetList widgetList;
// TODO:    Cardinal nWidgetListItems;
// TODO: 
// TODO:    /* if the sub-menu is torn off, unmanage the menu pane
// TODO:       before updating it to prevent the tear-off menu
// TODO:       from shrinking and expanding as the menu entries
// TODO:       are (un)managed */
// TODO:    if (!XmIsMenuShell(XtParent(subMenu->umleSubMenuPane)))
// TODO:    {
// TODO:       XtUnmanageChild(subMenu->umleSubMenuPane);
// TODO:    }
// TODO: 
// TODO:    /* manage all children of sub-menu pane */
// TODO:    XtVaGetValues(subMenu->umleSubMenuPane,
// TODO:                  XmNchildren, &widgetList,
// TODO:                  XmNnumChildren, &nWidgetListItems,
// TODO:                  NULL);
// TODO:    XtManageChildren(widgetList, nWidgetListItems);
// TODO: 
// TODO:    /* scan, if an menu item of given sub-menu holds a nested
// TODO:       sub-menu */
// TODO:    subMenuList = subMenu->umleSubMenuList;
// TODO: 
// TODO:    for (i=0; i<subMenuList->umlNbrItems; i ++)
// TODO:    {
// TODO:       element = subMenuList->umlItems[i];
// TODO: 
// TODO:       if (element->umleSubMenuList != NULL)
// TODO:       {
// TODO:          /* if element is a sub-menu, then continue managing
// TODO:             all items of that sub-menu recursively */
// TODO:          manageAllSubMenuWidgets(element);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* manage sub-menu pane widget itself */
// TODO:    XtManageChild(subMenu->umleMenuItem);
// TODO: 
// TODO:    /* if the sub-menu is torn off, then adjust & manage the menu */
// TODO:    if (!XmIsMenuShell(XtParent(subMenu->umleSubMenuPane)))
// TODO:    {
// TODO:       manageTearOffMenu(subMenu->umleSubMenuPane);
// TODO:    }
// TODO: 
// TODO:    /* redisplay sub-menu tearoff window, if the sub-menu
// TODO:       was torn off before */
// TODO:    ShowHiddenTearOff(subMenu->umleSubMenuPane);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Cache user menus:
// TODO: ** Unmanage all menu widgets of given user sub-menu list.
// TODO: */
// TODO: static void unmanageAllSubMenuWidgets(UserMenuListElement* subMenu)
// TODO: {
// TODO:    int i;
// TODO:    Fl_Widget* shell;
// TODO:    UserMenuList* subMenuList;
// TODO:    UserMenuListElement* element;
// TODO:    WidgetList widgetList;
// TODO:    Cardinal nWidgetListItems;
// TODO: 
// TODO:    /* if sub-menu is torn-off, then unmap its shell
// TODO:       (so tearoff window isn't displayed anymore) */
// TODO:    shell = XtParent(subMenu->umleSubMenuPane);
// TODO:    if (!XmIsMenuShell(shell))
// TODO:    {
// TODO:       XtUnmapWidget(shell);
// TODO:    }
// TODO: 
// TODO:    /* unmanage all children of sub-menu pane */
// TODO:    XtVaGetValues(subMenu->umleSubMenuPane,
// TODO:                  XmNchildren, &widgetList,
// TODO:                  XmNnumChildren, &nWidgetListItems,
// TODO:                  NULL);
// TODO:    XtUnmanageChildren(widgetList, nWidgetListItems);
// TODO: 
// TODO:    /* scan, if an menu item of given sub-menu holds a nested
// TODO:       sub-menu */
// TODO:    subMenuList = subMenu->umleSubMenuList;
// TODO: 
// TODO:    for (i=0; i<subMenuList->umlNbrItems; i ++)
// TODO:    {
// TODO:       element = subMenuList->umlItems[i];
// TODO: 
// TODO:       if (element->umleSubMenuList != NULL)
// TODO:       {
// TODO:          /* if element is a sub-menu, then continue unmanaging
// TODO:             all items of that sub-menu recursively */
// TODO:          unmanageAllSubMenuWidgets(element);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* unmanage sub-menu pane widget itself */
// TODO:    XtUnmanageChild(subMenu->umleMenuItem);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Cache user menus:
// TODO: ** Manage / unmanage menu widgets according to given user menu list.
// TODO: */
// TODO: static void manageMenuWidgets(UserMenuList* list)
// TODO: {
// TODO:    int i;
// TODO:    UserMenuListElement* element;
// TODO: 
// TODO:    /* (un)manage all elements of given user menu list */
// TODO:    for (i=0; i<list->umlNbrItems; i ++)
// TODO:    {
// TODO:       element = list->umlItems[i];
// TODO: 
// TODO:       if (element->umlePrevManageMode != element->umleManageMode ||
// TODO:             element->umleManageMode == UMMM_MANAGE)
// TODO:       {
// TODO:          /* previous and current manage mode differ OR
// TODO:             current manage mode indicates: element needs to be
// TODO:             (un)managed individually */
// TODO:          if (element->umleManageMode == UMMM_MANAGE_ALL)
// TODO:          {
// TODO:             /* menu item represented by "element" is a sub-menu and
// TODO:                needs to be completely managed */
// TODO:             manageAllSubMenuWidgets(element);
// TODO:          }
// TODO:          else if (element->umleManageMode == UMMM_MANAGE)
// TODO:          {
// TODO:             if (element->umlePrevManageMode == UMMM_UNMANAGE ||
// TODO:                   element->umlePrevManageMode == UMMM_UNMANAGE_ALL)
// TODO:             {
// TODO:                /* menu item represented by "element" was unmanaged
// TODO:                   before and needs to be managed now */
// TODO:                XtManageChild(element->umleMenuItem);
// TODO:             }
// TODO: 
// TODO:             /* if element is a sub-menu, then continue (un)managing
// TODO:                single elements of that sub-menu one by one */
// TODO:             if (element->umleSubMenuList != NULL)
// TODO:             {
// TODO:                /* if the sub-menu is torn off, unmanage the menu pane
// TODO:                   before updating it to prevent the tear-off menu
// TODO:                   from shrinking and expanding as the menu entries
// TODO:                   are (un)managed */
// TODO:                if (!XmIsMenuShell(XtParent(element->umleSubMenuPane)))
// TODO:                {
// TODO:                   XtUnmanageChild(element->umleSubMenuPane);
// TODO:                }
// TODO: 
// TODO:                /* (un)manage menu entries of sub-menu */
// TODO:                manageMenuWidgets(element->umleSubMenuList);
// TODO: 
// TODO:                /* if the sub-menu is torn off, then adjust & manage the menu */
// TODO:                if (!XmIsMenuShell(XtParent(element->umleSubMenuPane)))
// TODO:                {
// TODO:                   manageTearOffMenu(element->umleSubMenuPane);
// TODO:                }
// TODO: 
// TODO:                /* if the sub-menu was torn off then redisplay it */
// TODO:                ShowHiddenTearOff(element->umleSubMenuPane);
// TODO:             }
// TODO:          }
// TODO:          else if (element->umleManageMode == UMMM_UNMANAGE_ALL)
// TODO:          {
// TODO:             /* menu item represented by "element" is a sub-menu and
// TODO:                needs to be completely unmanaged */
// TODO:             unmanageAllSubMenuWidgets(element);
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             /* current mode is UMMM_UNMANAGE -> menu item represented
// TODO:                by "element" is a single menu item and needs to be
// TODO:                unmanaged */
// TODO:             XtUnmanageChild(element->umleMenuItem);
// TODO:          }
// TODO:       }
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Cache user menus:
// TODO: ** Remove accelerators from all items of given user (sub-)menu list.
// TODO: */
// TODO: static void removeAccelFromMenuWidgets(UserMenuList* menuList)
// TODO: {
// TODO:    int i;
// TODO:    UserMenuListElement* element;
// TODO: 
// TODO:    /* scan all elements of this (sub-)menu */
// TODO:    for (i=0; i<menuList->umlNbrItems; i ++)
// TODO:    {
// TODO:       element = menuList->umlItems[i];
// TODO: 
// TODO:       if (element->umleSubMenuList != NULL)
// TODO:       {
// TODO:          /* if element is a sub-menu, then continue removing accelerators
// TODO:             from all items of that sub-menu recursively */
// TODO:          removeAccelFromMenuWidgets(element->umleSubMenuList);
// TODO:       }
// TODO:       else if (element->umleAccKeys != NULL &&
// TODO:                element->umleManageMode == UMMM_UNMANAGE &&
// TODO:                element->umlePrevManageMode == UMMM_MANAGE)
// TODO:       {
// TODO:          /* remove accelerator if one was bound */
// TODO:          XtVaSetValues(element->umleMenuItem, XmNaccelerator, NULL, NULL);
// TODO:       }
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Cache user menus:
// TODO: ** Assign accelerators to all managed items of given user (sub-)menu list.
// TODO: */
// TODO: static void assignAccelToMenuWidgets(UserMenuList* menuList, WindowInfo* window)
// TODO: {
// TODO:    int i;
// TODO:    UserMenuListElement* element;
// TODO: 
// TODO:    /* scan all elements of this (sub-)menu */
// TODO:    for (i=0; i<menuList->umlNbrItems; i ++)
// TODO:    {
// TODO:       element = menuList->umlItems[i];
// TODO: 
// TODO:       if (element->umleSubMenuList != NULL)
// TODO:       {
// TODO:          /* if element is a sub-menu, then continue assigning accelerators
// TODO:             to all managed items of that sub-menu recursively */
// TODO:          assignAccelToMenuWidgets(element->umleSubMenuList, window);
// TODO:       }
// TODO:       else if (element->umleAccKeys != NULL &&
// TODO:                element->umleManageMode == UMMM_MANAGE &&
// TODO:                element->umlePrevManageMode == UMMM_UNMANAGE)
// TODO:       {
// TODO:          /* assign accelerator if applicable */
// TODO:          XtVaSetValues(element->umleMenuItem, XmNaccelerator,
// TODO:                        element->umleAccKeys, NULL);
// TODO:          if (!element->umleAccLockPatchApplied)
// TODO:          {
// TODO:             UpdateAccelLockPatch(window->splitPane, element->umleMenuItem);
// TODO:             element->umleAccLockPatchApplied = true;
// TODO:          }
// TODO:       }
// TODO:    }
// TODO: }

/*
** Cache user menus:
** (Un)Manage all items of selected user menu.
*/
static void manageUserMenu(selectedUserMenu* menu, WindowInfo* window)
{
   int n, i;
   int* id;
   bool currentLEisSubMenu;
   userMenuInfo* info;
   UserMenuList* menuList;
   UserMenuListElement* currentLE;
   UserMenuManageMode* mode;

   /* reset manage mode of all items of selected user menu in window cache */
   resetManageMode(menu->sumMainMenuList);

   /* set manage mode of all items of selected user menu in window cache
      according to the "to be managed" indication of the info list */
   for (n=0; n<menu->sumNbrOfListItems; n++)
   {
      info = menu->sumInfoList[n];

      menuList = menu->sumMainMenuList;
      id = info->umiId;

      /* select all menu list items belonging to menu record "info" using
         hierarchical ID of current menu info (e.g. id = {3} means:
         4th element of main menu; {0} = 1st element etc.)*/
      for (i=0; i<info->umiIdLen; i ++)
      {
         currentLE = menuList->umlItems[*id];
         mode = &currentLE->umleManageMode;
         currentLEisSubMenu = (currentLE->umleSubMenuList != NULL);

         if (info->umiToBeManaged)
         {
            /* menu record needs to be managed: */
            if (*mode == UMMM_UNMANAGE)
            {
               /* "mode" was not touched after reset ("init. state"):
                  if current list element represents a sub-menu, then
                  probably the complete sub-menu needs to be managed
                  too. If current list element indicates single menu
                  item, then just this item needs to be managed */
               if (currentLEisSubMenu)
               {
                  *mode = UMMM_MANAGE_ALL;
               }
               else
               {
                  *mode = UMMM_MANAGE;
               }
            }
            else if (*mode == UMMM_UNMANAGE_ALL)
            {
               /* "mode" was touched after reset:
                  current list element represents a sub-menu and min.
                  one element of the sub-menu needs to be unmanaged ->
                  the sub-menu needs to be (un)managed element by
                  element */
               *mode = UMMM_MANAGE;
            }
         }
         else
         {
            /* menu record needs to be unmanaged: */
            if (*mode == UMMM_UNMANAGE)
            {
               /* "mode" was not touched after reset ("init. state"):
                  if current list element represents a sub-menu, then
                  probably the complete sub-menu needs to be unmanaged
                  too. */
               if (currentLEisSubMenu)
               {
                  *mode = UMMM_UNMANAGE_ALL;
               }
            }
            else if (*mode == UMMM_MANAGE_ALL)
            {
               /* "mode" was touched after reset:
                  current list element represents a sub-menu and min.
                  one element of the sub-menu needs to be managed ->
                  the sub-menu needs to be (un)managed element by
                  element */
               *mode = UMMM_MANAGE;
            }
         }

         menuList = currentLE->umleSubMenuList;

         id ++;
      }
   }

// TODO:    /* if the menu is torn off, unmanage the menu pane
// TODO:       before updating it to prevent the tear-off menu
// TODO:       from shrinking and expanding as the menu entries
// TODO:       are managed */
// TODO:    if (!XmIsMenuShell(XtParent(menu->sumMenuPane)))
// TODO:       XtUnmanageChild(menu->sumMenuPane);
// TODO: 
// TODO:    /* manage menu widgets according to current / previous manage mode of
// TODO:       user menu window cache */
// TODO:    manageMenuWidgets(menu->sumMainMenuList);
// TODO: 
// TODO:    /* Note: before new accelerator is assigned it seems to be necessary
// TODO:       to remove old accelerator from user menu widgets. Removing same
// TODO:       accelerator *after* it was assigned to another user menu widget
// TODO:       doesn't work */
// TODO:    removeAccelFromMenuWidgets(menu->sumMainMenuList);
// TODO: 
// TODO:    assignAccelToMenuWidgets(menu->sumMainMenuList, window);
// TODO: 
// TODO:    /* if the menu is torn off, then adjust & manage the menu */
// TODO:    if (!XmIsMenuShell(XtParent(menu->sumMenuPane)))
// TODO:       manageTearOffMenu(menu->sumMenuPane);
}

/*
** Create either the variable Shell menu, Macro menu or Background menu
** items of "window" (driven by value of "menuType")
*/
static void createMenuItems(WindowInfo* window, selectedUserMenu* menu)
{
// TODO:    Fl_Widget* btn, subPane, newSubPane;
// TODO:    int n;
// TODO:    menuItemRec* item;
// TODO:    menuTreeItem* menuTree;
// TODO:    int i, nTreeEntries, size;
// TODO:    char* hierName, *namePtr, *subMenuName, *subSep, *fullName;
// TODO:    int menuType = menu->sumType;
// TODO:    userMenuInfo* info;
// TODO:    userSubMenuCache* subMenus = menu->sumSubMenus;
// TODO:    userSubMenuInfo* subMenuInfo;
// TODO:    UserMenuList* menuList;
// TODO:    UserMenuListElement* currentLE;
// TODO:    int subMenuDepth;
// TODO:    char accKeysBuf[MAX_ACCEL_LEN+5];
// TODO:    char* accKeys;
// TODO: 
// TODO:    /* Allocate storage for structures to help find panes of sub-menus */
// TODO:    size = sizeof(menuTreeItem) * menu->sumNbrOfListItems;
// TODO:    menuTree = (menuTreeItem*)malloc__(size);
// TODO:    nTreeEntries = 0;
// TODO: 
// TODO:    /* Harmless kludge: undo and redo items are marked specially if found
// TODO:       in the background menu, and used to dim/undim with edit menu */
// TODO:    window->bgMenuUndoItem = NULL;
// TODO:    window->bgMenuRedoItem = NULL;
// TODO: 
// TODO:    /*
// TODO:    ** Add items to the menu pane, creating hierarchical sub-menus as
// TODO:    ** necessary
// TODO:    */
// TODO:    allocUserMenuList(menu->sumMainMenuList, subMenus->usmcNbrOfMainMenuItems);
// TODO:    for (n=0; n<menu->sumNbrOfListItems; n++)
// TODO:    {
// TODO:       item = menu->sumItemList[n];
// TODO:       info = menu->sumInfoList[n];
// TODO:       menuList = menu->sumMainMenuList;
// TODO:       subMenuDepth = 0;
// TODO: 
// TODO:       fullName = info->umiName;
// TODO: 
// TODO:       /* create/find sub-menus, stripping off '>' until item name is
// TODO:          reached, then create the menu item */
// TODO:       namePtr = fullName;
// TODO:       subPane = menu->sumMenuPane;
// TODO:       for (;;)
// TODO:       {
// TODO:          subSep = strchr(namePtr, '>');
// TODO:          if (subSep == NULL)
// TODO:          {
// TODO:             btn = createUserMenuItem(subPane, namePtr, item, n,
// TODO:                                      (XtCallbackProc)(menuType == SHELL_CMDS ? shellMenuCB :
// TODO:                                            (menuType == MACRO_CMDS ? macroMenuCB : bgMenuCB)),
// TODO:                                      (XtPointer)window);
// TODO:             if (menuType == BG_MENU_CMDS && !strcmp(item->cmd, "undo()\n"))
// TODO:                window->bgMenuUndoItem = btn;
// TODO:             else if (menuType == BG_MENU_CMDS && !strcmp(item->cmd,"redo()\n"))
// TODO:                window->bgMenuRedoItem = btn;
// TODO:             /* generate accelerator keys */
// TODO:             genAccelEventName(accKeysBuf, item->modifiers, item->keysym);
// TODO:             accKeys = item->keysym == NoSymbol ? NULL : NeNewString(accKeysBuf);
// TODO:             /* create corresponding menu list item */
// TODO:             menuList->umlItems[menuList->umlNbrItems ++] =
// TODO:                allocUserMenuListElement(btn, accKeys);
// TODO:             break;
// TODO:          }
// TODO:          hierName = copySubstring(fullName, subSep - fullName);
// TODO:          subMenuInfo = findSubMenuInfo(subMenus, hierName);
// TODO:          newSubPane = findInMenuTree(menuTree, nTreeEntries, hierName);
// TODO:          if (newSubPane == NULL)
// TODO:          {
// TODO:             subMenuName = copySubstring(namePtr, subSep - namePtr);
// TODO:             newSubPane = createUserSubMenu(subPane, subMenuName, &btn);
// TODO:             free__(subMenuName);
// TODO:             menuTree[nTreeEntries].name = hierName;
// TODO:             menuTree[nTreeEntries++].menuPane = newSubPane;
// TODO: 
// TODO:             currentLE = allocUserMenuListElement(btn, NULL);
// TODO:             menuList->umlItems[menuList->umlNbrItems ++] = currentLE;
// TODO:             currentLE->umleSubMenuPane = newSubPane;
// TODO:             currentLE->umleSubMenuList =
// TODO:                allocUserSubMenuList(subMenuInfo->usmiId[subMenuInfo->usmiIdLen]);
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             currentLE = menuList->umlItems[subMenuInfo->usmiId[subMenuDepth]];
// TODO:             free__(hierName);
// TODO:          }
// TODO:          subPane = newSubPane;
// TODO:          menuList = currentLE->umleSubMenuList;
// TODO:          subMenuDepth ++;
// TODO:          namePtr = subSep + 1;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    *menu->sumMenuCreated = true;
// TODO: 
// TODO:    /* Free the structure used to keep track of sub-menus durring creation */
// TODO:    for (i=0; i<nTreeEntries; i++)
// TODO:       free__(menuTree[i].name);
// TODO:    free__((char*)menuTree);
}

// TODO: /*
// TODO: ** Find the widget corresponding to a hierarchical menu name (a>b>c...)
// TODO: */
// TODO: static Fl_Widget* findInMenuTree(menuTreeItem* menuTree, int nTreeEntries,
// TODO:                              const char* hierName)
// TODO: {
// TODO:    int i;
// TODO: 
// TODO:    for (i=0; i<nTreeEntries; i++)
// TODO:       if (!strcmp(hierName, menuTree[i].name))
// TODO:          return menuTree[i].menuPane;
// TODO:    return NULL;
// TODO: }

// --------------------------------------------------------------------------
static char* copySubstring(const char* string, int length)
{
   char* retStr = new char[length + 1];

   strncpy(retStr, string, length);
   retStr[length] = '\0';
   return retStr;
}

// TODO: static Fl_Widget* createUserMenuItem(Fl_Widget* menuPane, char* name, menuItemRec* f,
// TODO:                                  int index, XtCallbackProc cbRtn, XtPointer cbArg)
// TODO: {
// TODO:    NeString st1, st2;
// TODO:    char accText[MAX_ACCEL_LEN];
// TODO:    Fl_Widget* btn;
// TODO: 
// TODO:    generateAcceleratorString(accText, f->modifiers, f->keysym);
// TODO:    st1=NeNewString(name);
// TODO:    st2=NeNewString(accText);
// TODO:    btn = XtVaCreateWidget("cmd", xmPushButtonWidgetClass, menuPane,
// TODO:                           XmNlabelString, st1,
// TODO:                           XmNacceleratorText, st2,
// TODO:                           XmNmnemonic, f->mnemonic,
// TODO:                           XmNuserData, (XtPointer)(index+10), NULL);
// TODO:    XtAddCallback(btn, XmNactivateCallback, cbRtn, cbArg);
// TODO:    NeStringFree(st1);
// TODO:    NeStringFree(st2);
// TODO:    return btn;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Add a user-defined sub-menu to an established pull-down menu, marking
// TODO: ** it's userData field with TEMPORARY_MENU_ITEM so it can be found and
// TODO: ** removed later if the menu is redefined.  Returns the menu pane of the
// TODO: ** new sub-menu.
// TODO: */
// TODO: static Fl_Widget* createUserSubMenu(Fl_Widget* parent, char* label, Fl_Widget** menuItem)
// TODO: {
// TODO:    Fl_Widget* menuPane;
// TODO:    NeString st1;
// TODO:    static Arg args[1] = {{XmNuserData, (XtArgVal)TEMPORARY_MENU_ITEM}};
// TODO: 
// TODO:    menuPane  = CreatePulldownMenu(parent, "userPulldown", args, 1);
// TODO:    *menuItem = XtVaCreateWidget("userCascade", xmCascadeButtonWidgetClass, parent,
// TODO:                                 XmNlabelString, st1=NeNewString(label),
// TODO:                                 XmNsubMenuId, menuPane, XmNuserData, TEMPORARY_MENU_ITEM,
// TODO:                                 NULL);
// TODO:    NeStringFree(st1);
// TODO:    return menuPane;
// TODO: }

/*
** Cache user menus:
** Delete all variable menu items of given menu pane
*/
static void deleteMenuItems(Fl_Widget* menuPane)
{
// TODO:    WidgetList itemList, items;
// TODO:    Cardinal nItems;
// TODO:    Fl_Widget* subMenuID;
// TODO:    XtPointer userData;
// TODO:    int n;
// TODO: 
// TODO:    /* Fetch the list of children from the menu pane to delete */
// TODO:    XtVaGetValues(menuPane, XmNchildren, &itemList, XmNnumChildren, &nItems, NULL);
// TODO: 
// TODO:    /* make a copy because the widget alters the list as you delete widgets */
// TODO:    items = (WidgetList)malloc__(sizeof(Fl_Widget*) * nItems);
// TODO:    memcpy(items, itemList, sizeof(Fl_Widget*) * nItems);
// TODO: 
// TODO:    /* delete all of the widgets not marked as PERMANENT_MENU_ITEM */
// TODO:    for (n=0; n<(int)nItems; n++)
// TODO:    {
// TODO:       XtVaGetValues(items[n], XmNuserData, &userData, NULL);
// TODO:       if (userData != (XtPointer)PERMANENT_MENU_ITEM)
// TODO:       {
// TODO:          if (XtClass(items[n]) == xmCascadeButtonWidgetClass)
// TODO:          {
// TODO:             XtVaGetValues(items[n], XmNsubMenuId, &subMenuID, NULL);
// TODO: 
// TODO:             deleteMenuItems(subMenuID);
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             /* remove accel. before destroy or lose it forever */
// TODO:             XtVaSetValues(items[n], XmNaccelerator, NULL, NULL);
// TODO:          }
// TODO:          XtDestroyWidget(items[n]);
// TODO:       }
// TODO:    }
// TODO:    free__((char*)items);
}

// TODO: static void closeCB(Fl_Widget* w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    userCmdDialog* ucd = (userCmdDialog*)clientData;
// TODO: 
// TODO:    /* Mark that there's no longer a (macro, bg, or shell) dialog up */
// TODO:    if (ucd->dialogType == SHELL_CMDS)
// TODO:       ShellCmdDialog = NULL;
// TODO:    else if (ucd->dialogType == MACRO_CMDS)
// TODO:       MacroCmdDialog = NULL;
// TODO:    else
// TODO:       BGMenuCmdDialog = NULL;
// TODO: 
// TODO:    /* pop down and destroy the dialog (memory for ucd is freed in the
// TODO:       destroy callback) */
// TODO:    XtDestroyWidget(ucd->dlogShell);
// TODO: }
// TODO: 
// TODO: static void okCB(Fl_Widget* w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    userCmdDialog* ucd = (userCmdDialog*)clientData;
// TODO: 
// TODO:    /* Read the dialog fields, and update the menus */
// TODO:    if (!applyDialogChanges(ucd))
// TODO:       return;
// TODO: 
// TODO:    /* Mark that there's no longer a (macro, bg, or shell) dialog up */
// TODO:    if (ucd->dialogType == SHELL_CMDS)
// TODO:       ShellCmdDialog = NULL;
// TODO:    else if (ucd->dialogType == MACRO_CMDS)
// TODO:       MacroCmdDialog = NULL;
// TODO:    else
// TODO:       BGMenuCmdDialog = NULL;
// TODO: 
// TODO:    /* pop down and destroy the dialog (memory for ucd is freed in the
// TODO:       destroy callback) */
// TODO:    XtDestroyWidget(ucd->dlogShell);
// TODO: }
// TODO: 
// TODO: static void applyCB(Fl_Widget* w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    applyDialogChanges((userCmdDialog*)clientData);
// TODO: }
// TODO: 
// TODO: static void checkCB(Fl_Widget* w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    userCmdDialog* ucd = (userCmdDialog*)clientData;
// TODO: 
// TODO:    if (checkMacro(ucd))
// TODO:    {
// TODO:       DialogF(DF_INF, ucd->dlogShell, 1, "Macro",
// TODO:               "Macro compiled without error", "OK");
// TODO:    }
// TODO: }
// TODO: 
// TODO: static int checkMacro(userCmdDialog* ucd)
// TODO: {
// TODO:    menuItemRec* f;
// TODO: 
// TODO:    f = readDialogFields(ucd, false);
// TODO:    if (f == NULL)
// TODO:       return false;
// TODO:    if (!checkMacroText(f->cmd, ucd->dlogShell, ucd->cmdTextW))
// TODO:    {
// TODO:       freeMenuItemRec(f);
// TODO:       return false;
// TODO:    }
// TODO:    return true;
// TODO: }
// TODO: 
// TODO: static int checkMacroText(char* macro, Fl_Widget* errorParent, Fl_Widget* errFocus)
// TODO: {
// TODO:    Program* prog;
// TODO:    char* errMsg, *stoppedAt;
// TODO: 
// TODO:    prog = ParseMacro(macro, &errMsg, &stoppedAt);
// TODO:    if (prog == NULL)
// TODO:    {
// TODO:       if (errorParent != NULL)
// TODO:       {
// TODO:          ParseError(errorParent, macro, stoppedAt, "macro", errMsg);
// TODO:          XmTextSetInsertionPosition(errFocus, stoppedAt - macro);
// TODO:          XmProcessTraversal(errFocus, XmTRAVERSE_CURRENT);
// TODO:       }
// TODO:       return false;
// TODO:    }
// TODO:    FreeProgram(prog);
// TODO:    if (*stoppedAt != '\0')
// TODO:    {
// TODO:       if (errorParent != NULL)
// TODO:       {
// TODO:          ParseError(errorParent, macro, stoppedAt,"macro","syntax error");
// TODO:          XmTextSetInsertionPosition(errFocus, stoppedAt - macro);
// TODO:          XmProcessTraversal(errFocus, XmTRAVERSE_CURRENT);
// TODO:       }
// TODO:       return false;
// TODO:    }
// TODO:    return true;
// TODO: }
// TODO: 
// TODO: static int applyDialogChanges(userCmdDialog* ucd)
// TODO: {
// TODO:    int i;
// TODO: 
// TODO:    /* Get the current contents of the dialog fields */
// TODO:    if (!UpdateManagedList(ucd->managedList, true))
// TODO:       return false;
// TODO: 
// TODO:    /* Test compile the macro */
// TODO:    if (ucd->dialogType == MACRO_CMDS)
// TODO:       if (!checkMacro(ucd))
// TODO:          return false;
// TODO: 
// TODO:    /* Update the menu information */
// TODO:    if (ucd->dialogType == SHELL_CMDS)
// TODO:    {
// TODO:       for (i=0; i<NShellMenuItems; i++)
// TODO:          freeMenuItemRec(ShellMenuItems[i]);
// TODO:       freeUserMenuInfoList(ShellMenuInfo, NShellMenuItems);
// TODO:       freeSubMenuCache(&ShellSubMenus);
// TODO:       for (i=0; i<ucd->nMenuItems; i++)
// TODO:          ShellMenuItems[i] = copyMenuItemRec(ucd->menuItemsList[i]);
// TODO:       NShellMenuItems = ucd->nMenuItems;
// TODO:       parseMenuItemList(ShellMenuItems, NShellMenuItems, ShellMenuInfo, &ShellSubMenus);
// TODO:    }
// TODO:    else if (ucd->dialogType == MACRO_CMDS)
// TODO:    {
// TODO:       for (i=0; i<NMacroMenuItems; i++)
// TODO:          freeMenuItemRec(MacroMenuItems[i]);
// TODO:       freeUserMenuInfoList(MacroMenuInfo, NMacroMenuItems);
// TODO:       freeSubMenuCache(&MacroSubMenus);
// TODO:       for (i=0; i<ucd->nMenuItems; i++)
// TODO:          MacroMenuItems[i] = copyMenuItemRec(ucd->menuItemsList[i]);
// TODO:       NMacroMenuItems = ucd->nMenuItems;
// TODO:       parseMenuItemList(MacroMenuItems, NMacroMenuItems, MacroMenuInfo, &MacroSubMenus);
// TODO:    }
// TODO:    else     /* BG_MENU_CMDS */
// TODO:    {
// TODO:       for (i=0; i<NBGMenuItems; i++)
// TODO:          freeMenuItemRec(BGMenuItems[i]);
// TODO:       freeUserMenuInfoList(BGMenuInfo, NBGMenuItems);
// TODO:       freeSubMenuCache(&BGSubMenus);
// TODO:       for (i=0; i<ucd->nMenuItems; i++)
// TODO:          BGMenuItems[i] = copyMenuItemRec(ucd->menuItemsList[i]);
// TODO:       NBGMenuItems = ucd->nMenuItems;
// TODO:       parseMenuItemList(BGMenuItems, NBGMenuItems, BGMenuInfo, &BGSubMenus);
// TODO:    }
// TODO: 
// TODO:    /* Update the menus themselves in all of the NEdit windows */
// TODO:    rebuildMenuOfAllWindows(ucd->dialogType);
// TODO: 
// TODO:    /* Note that preferences have been changed */
// TODO:    MarkPrefsChanged();
// TODO:    return true;
// TODO: }
// TODO: 
// TODO: static void pasteReplayCB(Fl_Widget* w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    userCmdDialog* ucd = (userCmdDialog*)clientData;
// TODO: 
// TODO:    if (GetReplayMacro() == NULL)
// TODO:       return;
// TODO: 
// TODO:    XmTextInsert(ucd->cmdTextW, XmTextGetInsertionPosition(ucd->cmdTextW),
// TODO:                 GetReplayMacro());
// TODO: }
// TODO: 
// TODO: static void destroyCB(Fl_Widget* w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    userCmdDialog* ucd = (userCmdDialog*)clientData;
// TODO:    int i;
// TODO: 
// TODO:    for (i=0; i<ucd->nMenuItems; i++)
// TODO:       freeMenuItemRec(ucd->menuItemsList[i]);
// TODO:    free__((char*)ucd->menuItemsList);
// TODO:    free__((char*)ucd);
// TODO: }
// TODO: 
// TODO: static void accFocusCB(Fl_Widget* w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    userCmdDialog* ucd = (userCmdDialog*)clientData;
// TODO: 
// TODO:    RemoveDialogMnemonicHandler(XtParent(ucd->accTextW));
// TODO: }
// TODO: 
// TODO: static void accLoseFocusCB(Fl_Widget* w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    userCmdDialog* ucd = (userCmdDialog*)clientData;
// TODO: 
// TODO:    AddDialogMnemonicHandler(XtParent(ucd->accTextW), FALSE);
// TODO: }
// TODO: 
// TODO: static void accKeyCB(Fl_Widget* w, XtPointer clientData, XKeyEvent* event)
// TODO: {
// TODO:    userCmdDialog* ucd = (userCmdDialog*)clientData;
// TODO:    int keysym = XLookupKeysym(event, 0);
// TODO:    char outStr[MAX_ACCEL_LEN];
// TODO: 
// TODO:    /* Accept only real keys, not modifiers alone */
// TODO:    if (IsModifierKey(keysym))
// TODO:       return;
// TODO: 
// TODO:    /* Tab key means go to the next field, don't enter */
// TODO:    if (keysym == XK_Tab)
// TODO:       return;
// TODO: 
// TODO:    /* Beep and return if the modifiers are buttons or ones we don't support */
// TODO:    if (event->state & ~(ShiftMask | LockMask | ControlMask | Mod1Mask |
// TODO:                         Mod2Mask | Mod3Mask | Mod4Mask | Mod5Mask))
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Delete or backspace clears field */
// TODO:    if (keysym == XK_Delete || keysym == XK_BackSpace)
// TODO:    {
// TODO:       NeTextSetString(ucd->accTextW, "");
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* generate the string to use in the dialog field */
// TODO:    generateAcceleratorString(outStr, event->state, keysym);
// TODO: 
// TODO:    /* Reject single character accelerators (a very simple way to eliminate
// TODO:       un-modified letters and numbers)  The goal is give users a clue that
// TODO:       they're supposed to type the actual keys, not the name.  This scheme
// TODO:       is not rigorous and still allows accelerators like Comma. */
// TODO:    if (strlen(outStr) == 1)
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* fill in the accelerator field in the dialog */
// TODO:    NeTextSetString(ucd->accTextW, outStr);
// TODO: }
// TODO: 
// TODO: static void sameOutCB(Fl_Widget* w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    NeSetSensitive(((userCmdDialog*)clientData)->repInpBtn,
// TODO:                   NeToggleButtonGetState(w));
// TODO: }
// TODO: 
// TODO: static void shellMenuCB(Fl_Widget* w, WindowInfo* window, XtPointer callData)
// TODO: {
// TODO:    XtArgVal userData;
// TODO:    int index;
// TODO:    char* params[1];
// TODO: 
// TODO:    window = WidgetToWindow(MENU_WIDGET(w));
// TODO: 
// TODO:    /* get the index of the shell command and verify that it's in range */
// TODO:    XtVaGetValues(w, XmNuserData, &userData, NULL);
// TODO:    index = (int)userData - 10;
// TODO:    if (index <0 || index >= NShellMenuItems)
// TODO:       return;
// TODO: 
// TODO:    params[0] = ShellMenuItems[index]->name;
// TODO:    XtCallActionProc(window->lastFocus, "shell_menu_command",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, params, 1);
// TODO: }
// TODO: 
// TODO: static void macroMenuCB(Fl_Widget* w, WindowInfo* window, XtPointer callData)
// TODO: {
// TODO:    XtArgVal userData;
// TODO:    int index;
// TODO:    char* params[1];
// TODO: 
// TODO:    window = WidgetToWindow(MENU_WIDGET(w));
// TODO: 
// TODO:    /* Don't allow users to execute a macro command from the menu (or accel)
// TODO:       if there's already a macro command executing.  NEdit can't handle
// TODO:       running multiple, independent uncoordinated, macros in the same
// TODO:       window.  Macros may invoke macro menu commands recursively via the
// TODO:       macro_menu_command action proc, which is important for being able to
// TODO:       repeat any operation, and to embed macros within eachother at any
// TODO:       level, however, a call here with a macro running means that THE USER
// TODO:       is explicitly invoking another macro via the menu or an accelerator. */
// TODO:    if (window->macroCmdData != NULL)
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* get the index of the macro command and verify that it's in range */
// TODO:    XtVaGetValues(w, XmNuserData, &userData, NULL);
// TODO:    index = (int)userData - 10;
// TODO:    if (index <0 || index >= NMacroMenuItems)
// TODO:       return;
// TODO: 
// TODO:    params[0] = MacroMenuItems[index]->name;
// TODO:    XtCallActionProc(window->lastFocus, "macro_menu_command",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, params, 1);
// TODO: }
// TODO: 
// TODO: static void bgMenuCB(Fl_Widget* w, WindowInfo* window, XtPointer callData)
// TODO: {
// TODO:    XtArgVal userData;
// TODO:    int index;
// TODO:    char* params[1];
// TODO: 
// TODO:    /* Same remark as for macro menu commands (see above). */
// TODO:    if (window->macroCmdData != NULL)
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* get the index of the macro command and verify that it's in range */
// TODO:    XtVaGetValues(w, XmNuserData, &userData, NULL);
// TODO:    index = (int)userData - 10;
// TODO:    if (index <0 || index >= NBGMenuItems)
// TODO:       return;
// TODO: 
// TODO:    params[0] = BGMenuItems[index]->name;
// TODO:    XtCallActionProc(window->lastFocus, "bg_menu_command",
// TODO:                     ((XmAnyCallbackStruct*)callData)->event, params, 1);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Update the name, accelerator, mnemonic, and command fields in the shell
// TODO: ** command or macro dialog to agree with the currently selected item in the
// TODO: ** menu item list.
// TODO: */
// TODO: static void updateDialogFields(menuItemRec* f, userCmdDialog* ucd)
// TODO: {
// TODO:    char mneString[2], accString[MAX_ACCEL_LEN];
// TODO: 
// TODO:    /* fill in the name, accelerator, mnemonic, and command fields of the
// TODO:       dialog for the newly selected item, or blank them if "New" is selected */
// TODO:    if (f == NULL)
// TODO:    {
// TODO:       NeTextSetString(ucd->nameTextW, "");
// TODO:       NeTextSetString(ucd->cmdTextW, "");
// TODO:       NeTextSetString(ucd->accTextW, "");
// TODO:       NeTextSetString(ucd->mneTextW, "");
// TODO:       if (ucd->dialogType == SHELL_CMDS)
// TODO:       {
// TODO:          NeRadioButtonChangeState(ucd->selInpBtn, true, true);
// TODO:          NeRadioButtonChangeState(ucd->sameOutBtn, true, true);
// TODO:          NeRadioButtonChangeState(ucd->repInpBtn, false, false);
// TODO:          NeSetSensitive(ucd->repInpBtn, true);
// TODO:          NeRadioButtonChangeState(ucd->saveFirstBtn, false, false);
// TODO:          NeRadioButtonChangeState(ucd->loadAfterBtn, false, false);
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       mneString[0] = f->mnemonic;
// TODO:       mneString[1] = '\0';
// TODO:       generateAcceleratorString(accString, f->modifiers, f->keysym);
// TODO:       NeTextSetString(ucd->nameTextW, f->name);
// TODO:       NeTextSetString(ucd->cmdTextW, f->cmd);
// TODO:       NeTextSetString(ucd->accTextW, accString);
// TODO:       NeTextSetString(ucd->mneTextW, mneString);
// TODO:       NeRadioButtonChangeState(ucd->selInpBtn, f->input==FROM_SELECTION, false);
// TODO:       if (ucd->dialogType == SHELL_CMDS)
// TODO:       {
// TODO:          NeRadioButtonChangeState(ucd->winInpBtn, f->input == FROM_WINDOW,
// TODO:                                 false);
// TODO:          NeRadioButtonChangeState(ucd->eitherInpBtn, f->input == FROM_EITHER,
// TODO:                                 false);
// TODO:          NeRadioButtonChangeState(ucd->noInpBtn, f->input == FROM_NONE,
// TODO:                                 false);
// TODO:          NeRadioButtonChangeState(ucd->sameOutBtn, f->output==TO_SAME_WINDOW,
// TODO:                                 false);
// TODO:          NeRadioButtonChangeState(ucd->winOutBtn, f->output==TO_NEW_WINDOW,
// TODO:                                 false);
// TODO:          NeRadioButtonChangeState(ucd->dlogOutBtn, f->output==TO_DIALOG,
// TODO:                                 false);
// TODO:          NeRadioButtonChangeState(ucd->repInpBtn, f->repInput, false);
// TODO:          NeSetSensitive(ucd->repInpBtn, f->output==TO_SAME_WINDOW);
// TODO:          NeRadioButtonChangeState(ucd->saveFirstBtn, f->saveFirst, false);
// TODO:          NeRadioButtonChangeState(ucd->loadAfterBtn, f->loadAfter, false);
// TODO:       }
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Read the name, accelerator, mnemonic, and command fields from the shell or
// TODO: ** macro commands dialog into a newly allocated menuItemRec.  Returns a
// TODO: ** pointer to the new menuItemRec structure as the function value, or NULL on
// TODO: ** failure.
// TODO: */
// TODO: static menuItemRec* readDialogFields(userCmdDialog* ucd, int silent)
// TODO: {
// TODO:    char* nameText, *cmdText, *mneText, *accText;
// TODO:    menuItemRec* f;
// TODO: 
// TODO:    nameText = NeTextGetString(ucd->nameTextW);
// TODO:    if (*nameText == '\0')
// TODO:    {
// TODO:       if (!silent)
// TODO:       {
// TODO:          DialogF(DF_WARN, ucd->dlogShell, 1, "Menu Entry",
// TODO:                  "Please specify a name\nfor the menu item", "OK");
// TODO:          XmProcessTraversal(ucd->nameTextW, XmTRAVERSE_CURRENT);
// TODO:       }
// TODO:       free__(nameText);
// TODO:       return NULL;
// TODO:    }
// TODO: 
// TODO:    if (strchr(nameText, ':'))
// TODO:    {
// TODO:       if (!silent)
// TODO:       {
// TODO:          DialogF(DF_WARN, ucd->dlogShell, 1, "Menu Entry",
// TODO:                  "Menu item names may not\ncontain colon (:) characters",
// TODO:                  "OK");
// TODO:          XmProcessTraversal(ucd->nameTextW, XmTRAVERSE_CURRENT);
// TODO:       }
// TODO:       free__(nameText);
// TODO:       return NULL;
// TODO:    }
// TODO: 
// TODO:    cmdText = NeTextGetString(ucd->cmdTextW);
// TODO:    if (cmdText == NULL || *cmdText == '\0')
// TODO:    {
// TODO:       if (!silent)
// TODO:       {
// TODO:          DialogF(DF_WARN, ucd->dlogShell, 1, "Command to Execute",
// TODO:                  "Please specify %s to execute", "OK",
// TODO:                  ucd->dialogType == SHELL_CMDS
// TODO:                  ? "shell command"
// TODO:                  : "macro command(s)");
// TODO:          XmProcessTraversal(ucd->cmdTextW, XmTRAVERSE_CURRENT);
// TODO:       }
// TODO:       free__(nameText);
// TODO:       free__(cmdText);
// TODO: 
// TODO:       return NULL;
// TODO:    }
// TODO: 
// TODO:    if (ucd->dialogType == MACRO_CMDS || ucd->dialogType == BG_MENU_CMDS)
// TODO:    {
// TODO:       addTerminatingNewline(&cmdText);
// TODO:       if (!checkMacroText(cmdText, silent ? NULL : ucd->dlogShell,
// TODO:                           ucd->cmdTextW))
// TODO:       {
// TODO:          free__(nameText);
// TODO:          free__(cmdText);
// TODO:          return NULL;
// TODO:       }
// TODO:    }
// TODO:    f = (menuItemRec*)malloc__(sizeof(menuItemRec));
// TODO:    f->name = nameText;
// TODO:    f->cmd = cmdText;
// TODO:    if ((mneText = NeTextGetString(ucd->mneTextW)) != NULL)
// TODO:    {
// TODO:       f->mnemonic = mneText==NULL ? '\0' : mneText[0];
// TODO:       free__(mneText);
// TODO:       if (f->mnemonic == ':')		/* colons mess up string parsing */
// TODO:          f->mnemonic = '\0';
// TODO:    }
// TODO:    if ((accText = NeTextGetString(ucd->accTextW)) != NULL)
// TODO:    {
// TODO:       parseAcceleratorString(accText, &f->modifiers, &f->keysym);
// TODO:       free__(accText);
// TODO:    }
// TODO:    if (ucd->dialogType == SHELL_CMDS)
// TODO:    {
// TODO:       if (NeToggleButtonGetState(ucd->selInpBtn))
// TODO:          f->input = FROM_SELECTION;
// TODO:       else if (NeToggleButtonGetState(ucd->winInpBtn))
// TODO:          f->input = FROM_WINDOW;
// TODO:       else if (NeToggleButtonGetState(ucd->eitherInpBtn))
// TODO:          f->input = FROM_EITHER;
// TODO:       else
// TODO:          f->input = FROM_NONE;
// TODO:       if (NeToggleButtonGetState(ucd->winOutBtn))
// TODO:          f->output = TO_NEW_WINDOW;
// TODO:       else if (NeToggleButtonGetState(ucd->dlogOutBtn))
// TODO:          f->output = TO_DIALOG;
// TODO:       else
// TODO:          f->output = TO_SAME_WINDOW;
// TODO:       f->repInput = NeToggleButtonGetState(ucd->repInpBtn);
// TODO:       f->saveFirst = NeToggleButtonGetState(ucd->saveFirstBtn);
// TODO:       f->loadAfter = NeToggleButtonGetState(ucd->loadAfterBtn);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       f->input = NeToggleButtonGetState(ucd->selInpBtn) ? FROM_SELECTION :
// TODO:                  FROM_NONE;
// TODO:       f->output = TO_SAME_WINDOW;
// TODO:       f->repInput = false;
// TODO:       f->saveFirst = false;
// TODO:       f->loadAfter = false;
// TODO:    }
// TODO:    return f;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Copy a menu item record, and its associated memory
// TODO: */
// TODO: static menuItemRec* copyMenuItemRec(menuItemRec* item)
// TODO: {
// TODO:    menuItemRec* newItem;
// TODO: 
// TODO:    newItem = (menuItemRec*)malloc__(sizeof(menuItemRec));
// TODO:    *newItem = *item;
// TODO:    newItem->name = malloc__(strlen(item->name)+1);
// TODO:    strcpy(newItem->name, item->name);
// TODO:    newItem->cmd = malloc__(strlen(item->cmd)+1);
// TODO:    strcpy(newItem->cmd, item->cmd);
// TODO:    return newItem;
// TODO: }

/*
** Free a menu item record, and its associated memory
*/
static void freeMenuItemRec(menuItemRec* item)
{
   delete[] item->name;
   delete[] item->cmd;
   delete item;
}

// TODO: /*
// TODO: ** Callbacks for managed-list operations
// TODO: */
// TODO: static void* getDialogDataCB(void* oldItem, int explicitRequest, int* abort,
// TODO:                              void* cbArg)
// TODO: {
// TODO:    userCmdDialog* ucd = (userCmdDialog*)cbArg;
// TODO:    menuItemRec* currentFields;
// TODO: 
// TODO:    /* If the dialog is currently displaying the "new" entry and the
// TODO:       fields are empty, that's just fine */
// TODO:    if (oldItem == NULL && dialogFieldsAreEmpty(ucd))
// TODO:       return NULL;
// TODO: 
// TODO:    /* If there are no problems reading the data, just return it */
// TODO:    currentFields = readDialogFields(ucd, true);
// TODO:    if (currentFields != NULL)
// TODO:       return (void*)currentFields;
// TODO: 
// TODO:    /* If user might not be expecting fields to be read, give more warning */
// TODO:    if (!explicitRequest)
// TODO:    {
// TODO:       if (DialogF(DF_WARN, ucd->dlogShell, 2, "Discard Entry",
// TODO:                   "Discard incomplete entry\nfor current menu item?", "Keep",
// TODO:                   "Discard") == 2)
// TODO:       {
// TODO:          return oldItem == NULL
// TODO:                 ? NULL
// TODO:                 : (void*)copyMenuItemRec((menuItemRec*)oldItem);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* Do readDialogFields again without "silent" mode to display warning(s) */
// TODO:    readDialogFields(ucd, false);
// TODO:    *abort = true;
// TODO:    return NULL;
// TODO: }
// TODO: 
// TODO: 
// TODO: static void setDialogDataCB(void* item, void* cbArg)
// TODO: {
// TODO:    updateDialogFields((menuItemRec*)item, (userCmdDialog*)cbArg);
// TODO: }
// TODO: 
// TODO: static int dialogFieldsAreEmpty(userCmdDialog* ucd)
// TODO: {
// TODO:    return TextWidgetIsBlank(ucd->nameTextW) &&
// TODO:           TextWidgetIsBlank(ucd->cmdTextW) &&
// TODO:           TextWidgetIsBlank(ucd->accTextW) &&
// TODO:           TextWidgetIsBlank(ucd->mneTextW) &&
// TODO:           (ucd->dialogType != SHELL_CMDS || (
// TODO:               NeToggleButtonGetState(ucd->selInpBtn) &&
// TODO:               NeToggleButtonGetState(ucd->sameOutBtn) &&
// TODO:               !NeToggleButtonGetState(ucd->repInpBtn) &&
// TODO:               !NeToggleButtonGetState(ucd->saveFirstBtn) &&
// TODO:               !NeToggleButtonGetState(ucd->loadAfterBtn)));
// TODO: }
// TODO: 
// TODO: static void freeItemCB(void* item)
// TODO: {
// TODO:    freeMenuItemRec((menuItemRec*)item);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Gut a text widget of it's ability to process input
// TODO: */
// TODO: static void disableTextW(Fl_Widget* textW)
// TODO: {
// TODO:    static XtTranslations emptyTable = NULL;
// TODO:    static char* emptyTranslations = "\
// TODO:     	<EnterWindow>:	enter()\n\
// TODO: 	<Btn1Down>:	grab-focus()\n\
// TODO: 	<Btn1Motion>:	extend-adjust()\n\
// TODO: 	<Btn1Up>:	extend-end()\n\
// TODO: 	Shift<Key>Tab:	prev-tab-group()\n\
// TODO: 	Ctrl<Key>Tab:	next-tab-group()\n\
// TODO: 	<Key>Tab:	next-tab-group()\n\
// TODO: 	<LeaveWindow>:	leave()\n\
// TODO: 	<FocusIn>:	focusIn()\n\
// TODO: 	<FocusOut>:	focusOut()\n\
// TODO: 	<Unmap>:	unmap()\n";
// TODO: 
// TODO:    /* replace the translation table with the slimmed down one above */
// TODO:    if (emptyTable == NULL)
// TODO:       emptyTable = XtParseTranslationTable(emptyTranslations);
// TODO:    XtVaSetValues(textW, XmNtranslations, emptyTable, NULL);
// TODO: }

// --------------------------------------------------------------------------
static char* writeMenuItemString(menuItemRec** menuItems, int nItems, int listType)
{
   char* outStr, *outPtr, *c, accStr[MAX_ACCEL_LEN];
   menuItemRec* f;
   int i, length;

   /* determine the max. amount of memory needed for the returned string
      and allocate a buffer for composing the string */
   length = 0;
   for (i=0; i<nItems; i++)
   {
      f = menuItems[i];
      generateAcceleratorString(accStr, f->modifiers, f->keysym);
      length += strlen(f->name) * 2; /* allow for \n & \\ expansions */
      length += strlen(accStr);
      length += strlen(f->cmd) * 6;	/* allow for \n & \\ expansions */
      length += 21;			/* number of characters added below */
   }
   length++;				/* terminating null */
   outStr = new char[length];

   /* write the string */
   outPtr = outStr;
   *outPtr++ = '\\';
   *outPtr++ = '\n';
   for (i=0; i<nItems; i++)
   {
      f = menuItems[i];
      generateAcceleratorString(accStr, f->modifiers, f->keysym);
      *outPtr++ = '\t';
      for (c=f->name; *c!='\0'; ++c)   /* Copy the command name */
      {
         if (*c == '\\')              /* changing backslashes to \\ */
         {
            *outPtr++ = '\\';
            *outPtr++ = '\\';
         }
         else if (*c == '\n')     /* changing newlines to \n */
         {
            *outPtr++ = '\\';
            *outPtr++ = 'n';
         }
         else
         {
            *outPtr++ = *c;
         }
      }
      *outPtr++ = ':';
      strcpy(outPtr, accStr);
      outPtr += strlen(accStr);
      *outPtr++ = ':';
      if (f->mnemonic != '\0')
         *outPtr++ = f->mnemonic;
      *outPtr++ = ':';
      if (listType == SHELL_CMDS)
      {
         if (f->input == FROM_SELECTION)
            *outPtr++ = 'I';
         else if (f->input == FROM_WINDOW)
            *outPtr++ = 'A';
         else if (f->input == FROM_EITHER)
            *outPtr++ = 'E';
         if (f->output == TO_DIALOG)
            *outPtr++ = 'D';
         else if (f->output == TO_NEW_WINDOW)
            *outPtr++ = 'W';
         if (f->repInput)
            *outPtr++ = 'X';
         if (f->saveFirst)
            *outPtr++ = 'S';
         if (f->loadAfter)
            *outPtr++ = 'L';
         *outPtr++ = ':';
      }
      else
      {
         if (f->input == FROM_SELECTION)
            *outPtr++ = 'R';
         *outPtr++ = ':';
         *outPtr++ = ' ';
         *outPtr++ = '{';
      }
      *outPtr++ = '\\';
      *outPtr++ = 'n';
      *outPtr++ = '\\';
      *outPtr++ = '\n';
      *outPtr++ = '\t';
      *outPtr++ = '\t';
      for (c=f->cmd; *c!='\0'; c++)   /* Copy the command string, changing */
      {
         if (*c == '\\')  	    	/* backslashes to double backslashes */
         {
            *outPtr++ = '\\';	/* and newlines to backslash-n's,    */
            *outPtr++ = '\\';	/* followed by real newlines and tab */
         }
         else if (*c == '\n')
         {
            *outPtr++ = '\\';
            *outPtr++ = 'n';
            *outPtr++ = '\\';
            *outPtr++ = '\n';
            *outPtr++ = '\t';
            *outPtr++ = '\t';
         }
         else
            *outPtr++ = *c;
      }
      if (listType == MACRO_CMDS || listType == BG_MENU_CMDS)
      {
         if (*(outPtr-1) == '\t') outPtr--;
         *outPtr++ = '}';
      }
      *outPtr++ = '\\';
      *outPtr++ = 'n';
      *outPtr++ = '\\';
      *outPtr++ = '\n';
   }
   --outPtr;
   *--outPtr = '\0';
   return outStr;
}

// --------------------------------------------------------------------------
static int loadMenuItemString(char* inString, menuItemRec** menuItems, int* nItems, int listType)
{
   menuItemRec* f;
   char* cmdStr;
   char* inPtr = inString;
   char* nameStr, accStr[MAX_ACCEL_LEN], mneChar;
   int keysym;
   unsigned int modifiers;
   int i, input, output, saveFirst, loadAfter, repInput;
   int nameLen, accLen, mneLen, cmdLen;

   for (;;)
   {

      /* remove leading whitespace */
      while (*inPtr == ' ' || *inPtr == '\t')
         inPtr++;

      /* end of string in proper place */
      if (*inPtr == '\0')
      {
         return true;
      }

      /* read name field */
      nameLen = strcspn(inPtr, ":");
      if (nameLen == 0)
         return parseError("no name field");
      nameStr = new char[nameLen+1];
      strncpy(nameStr, inPtr, nameLen);
      nameStr[nameLen] = '\0';
      inPtr += nameLen;
      if (*inPtr == '\0')
         return parseError("end not expected");
      inPtr++;

      /* read accelerator field */
      accLen = strcspn(inPtr, ":");
      if (accLen >= MAX_ACCEL_LEN)
         return parseError("accelerator field too long");
      strncpy(accStr, inPtr, accLen);
      accStr[accLen] = '\0';
      inPtr += accLen;
      if (*inPtr == '\0')
         return parseError("end not expected");
      inPtr++;

      /* read menemonic field */
      mneLen = strcspn(inPtr, ":");
      if (mneLen > 1)
         return parseError("mnemonic field too long");
      if (mneLen == 1)
         mneChar = *inPtr++;
      else
         mneChar = '\0';
      inPtr++;
      if (*inPtr == '\0')
         return parseError("end not expected");

      /* read flags field */
      input = FROM_NONE;
      output = TO_SAME_WINDOW;
      repInput = false;
      saveFirst = false;
      loadAfter = false;
      for (; *inPtr != ':'; inPtr++)
      {
         if (listType == SHELL_CMDS)
         {
            if (*inPtr == 'I')
               input = FROM_SELECTION;
            else if (*inPtr == 'A')
               input = FROM_WINDOW;
            else if (*inPtr == 'E')
               input = FROM_EITHER;
            else if (*inPtr == 'W')
               output = TO_NEW_WINDOW;
            else if (*inPtr == 'D')
               output = TO_DIALOG;
            else if (*inPtr == 'X')
               repInput = true;
            else if (*inPtr == 'S')
               saveFirst = true;
            else if (*inPtr == 'L')
               loadAfter = true;
            else
               return parseError("unreadable flag field");
         }
         else
         {
            if (*inPtr == 'R')
               input = FROM_SELECTION;
            else
               return parseError("unreadable flag field");
         }
      }
      inPtr++;

      /* read command field */
      if (listType == SHELL_CMDS)
      {
         if (*inPtr++ != '\n')
            return parseError("command must begin with newline");
         while (*inPtr == ' ' || *inPtr == '\t') /* leading whitespace */
            inPtr++;
         cmdLen = strcspn(inPtr, "\n");
         if (cmdLen == 0)
            return parseError("shell command field is empty");
         cmdStr = new char[cmdLen+1];
         strncpy(cmdStr, inPtr, cmdLen);
         cmdStr[cmdLen] = '\0';
         inPtr += cmdLen;
      }
      else
      {
         cmdStr = copyMacroToEnd(&inPtr);
         if (cmdStr == NULL)
            return false;
      }
      while (*inPtr == ' ' || *inPtr == '\t' || *inPtr == '\n')
         inPtr++; /* skip trailing whitespace & newline */

      /* parse the accelerator field */
      if (!parseAcceleratorString(accStr, &modifiers, &keysym))
         return parseError("couldn't read accelerator field");

      /* create a menu item record */
      f = new menuItemRec();
      f->name = nameStr;
      f->cmd = cmdStr;
      f->mnemonic = mneChar;
      f->modifiers = modifiers;
      f->input = input;
      f->output = output;
      f->repInput = repInput;
      f->saveFirst = saveFirst;
      f->loadAfter = loadAfter;
      f->keysym = keysym;

      /* add/replace menu record in the list */
      for (i=0; i < *nItems; i++)
      {
         if (!strcmp(menuItems[i]->name, f->name))
         {
            freeMenuItemRec(menuItems[i]);
            menuItems[i] = f;
            break;
         }
      }
      if (i == *nItems)
         menuItems[(*nItems)++] = f;

   }
}

// --------------------------------------------------------------------------
static int parseError(const char* message)
{
   fprintf(stderr, "NEdit: Parse error in user defined menu item, %s\n",message);
   return false;
}

/*
** Create a text string representing an accelerator for the dialog,
** the shellCommands or macroCommands resource, and for the menu item.
*/
static void generateAcceleratorString(char* text, unsigned int modifiers, int keysym)
{
   char* shiftStr = "", *ctrlStr = "", *altStr = "";
   char* mod2Str  = "", *mod3Str = "", *mod4Str = "", *mod5Str = "";
   char keyName[20];
// TODO:    Modifiers numLockMask = GetNumLockModMask(TheDisplay);

   /* if there's no accelerator, generate an empty string */
   if (keysym == 0) // NoSymbol
   {
      *text = '\0';
      return;
   }

   /* Translate the modifiers into strings.
      Lock and NumLock are always ignored (see util/misc.c),
      so we don't display them either. */
   if (modifiers & FL_SHIFT)
      shiftStr = "Shift+";
   if (modifiers & FL_CTRL)
      ctrlStr = "Ctrl+";
   if (modifiers & FL_ALT)
      altStr = "Alt+";
// TODO:    if ((modifiers & Mod2Mask) && (Mod2Mask != numLockMask))
// TODO:       mod2Str = "Mod2+";
// TODO:    if ((modifiers & Mod3Mask) && (Mod3Mask != numLockMask))
// TODO:       mod3Str = "Mod3+";
// TODO:    if ((modifiers & Mod4Mask) && (Mod4Mask != numLockMask))
// TODO:       mod4Str = "Mod4+";
// TODO:    if ((modifiers & Mod5Mask) && (Mod5Mask != numLockMask))
// TODO:       mod5Str = "Mod5+";

   // for a consistent look to the accelerator names in the menus,
   // capitalize the first letter of the keysym */
   strcpy(keyName, NeKeysymToString(keysym));
   *keyName = toupper(*keyName);

   /* concatenate the strings together */
   sprintf(text, "%s%s%s%s%s%s%s%s", shiftStr, ctrlStr, altStr, mod2Str, mod3Str, mod4Str, mod5Str, keyName);
}

// TODO: /*
// TODO: ** Create a translation table event description string for the menu
// TODO: ** XmNaccelerator resource.
// TODO: */
// TODO: static void genAccelEventName(char* text, unsigned int modifiers,
// TODO:                               int keysym)
// TODO: {
// TODO:    char* shiftStr = "", *lockStr = "", *ctrlStr = "", *altStr  = "";
// TODO:    char* mod2Str  = "", *mod3Str = "", *mod4Str = "", *mod5Str = "";
// TODO: 
// TODO:    /* if there's no accelerator, generate an empty string */
// TODO:    if (keysym == NoSymbol)
// TODO:    {
// TODO:       *text = '\0';
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* translate the modifiers into strings */
// TODO:    if (modifiers & ShiftMask)
// TODO:       shiftStr = "Shift ";
// TODO:    if (modifiers & LockMask)
// TODO:       lockStr = "Lock ";
// TODO:    if (modifiers & ControlMask)
// TODO:       ctrlStr = "Ctrl ";
// TODO:    if (modifiers & Mod1Mask)
// TODO:       altStr = "Alt ";
// TODO:    if (modifiers & Mod2Mask)
// TODO:       mod2Str = "Mod2 ";
// TODO:    if (modifiers & Mod3Mask)
// TODO:       mod3Str = "Mod3 ";
// TODO:    if (modifiers & Mod4Mask)
// TODO:       mod4Str = "Mod4 ";
// TODO:    if (modifiers & Mod5Mask)
// TODO:       mod5Str = "Mod5 ";
// TODO: 
// TODO:    /* put the modifiers together with the key name */
// TODO:    sprintf(text, "%s%s%s%s%s%s%s%s<Key>%s",
// TODO:            shiftStr, lockStr, ctrlStr, altStr,
// TODO:            mod2Str,  mod3Str, mod4Str, mod5Str,
// TODO:            NeKeysymToString(keysym));
// TODO: }
// TODO: 
/*
** Read an accelerator name and put it into the form of a modifier mask
** and a int code.  Returns false if string can't be read
** ... does not handle whitespace in string (look at scanf)
*/
static int parseAcceleratorString(const char* string, unsigned int* modifiers, int* keysym)
{
   int i, nFields, inputLength = strlen(string);
   char fields[10][MAX_ACCEL_LEN];

   // a blank field means no accelerator
   if (inputLength == 0)
   {
      *modifiers = 0;
      *keysym = 0; // TODO: NoSymbol;
      return true;
   }

   // limit the string length so no field strings will overflow
   if (inputLength > MAX_ACCEL_LEN)
      return false;

   // divide the input into '+' separated fields
   nFields = sscanf(string, "%[^+]+%[^+]+%[^+]+%[^+]+%[^+]+%[^+]+%[^+]+%[^+]+%[^+]+%[^+]",
                    fields[0], fields[1], fields[2], fields[3], fields[4], fields[5],
                    fields[6], fields[7], fields[8], fields[9]);
   if (nFields == 0)
      return false;

   // get the key name from the last field and translate it to a keysym.
   // If the name is capitalized, try it lowercase as well, since some
   // of the keysyms are "prettied up" by generateAcceleratorString
   *keysym = NeStringToKeysym(fields[nFields-1]);
   if (*keysym == 0) // NoSymbol
     return false;

   // parse the modifier names from the rest of the fields
   *modifiers = 0;
   for (i=0; i<nFields-1; i++)
   {
      if (!strcmp(fields[i], "Shift"))
         *modifiers |= FL_SHIFT;
      else if (!strcmp(fields[i], "Lock"))
         *modifiers |= FL_CAPS_LOCK;
      else if (!strcmp(fields[i], "Ctrl"))
         *modifiers |= FL_CTRL;
      // comparision with "Alt" for compatibility with old .nedit files
      else if (!strcmp(fields[i], "Alt"))
         *modifiers |= FL_ALT;
// TODO:       else if (!strcmp(fields[i], "Mod2"))
// TODO:          *modifiers |= Mod2Mask;
// TODO:       else if (!strcmp(fields[i], "Mod3"))
// TODO:          *modifiers |= Mod3Mask;
// TODO:       else if (!strcmp(fields[i], "Mod4"))
// TODO:          *modifiers |= Mod4Mask;
// TODO:       else if (!strcmp(fields[i], "Mod5"))
// TODO:          *modifiers |= Mod5Mask;
      else
         return false;
   }

   // all fields successfully parsed
   return true;
}

/*
** Scan text from "*inPtr" to the end of macro input (matching brace),
** advancing inPtr, and return macro text as function return value.
**
** This is kind of wastefull in that it throws away the compiled macro,
** to be re-generated from the text as needed, but compile time is
** negligible for most macros.
*/
static char* copyMacroToEnd(char** inPtr)
{
   char* retStr, *errMsg, *stoppedAt, *p, *retPtr;
   Program* prog;

   /* Skip over whitespace to find make sure there's a beginning brace
      to anchor the parse (if not, it will take the whole file) */
   *inPtr += strspn(*inPtr, " \t\n");
   if (**inPtr != '{')
   {
      ParseError(NULL, *inPtr, *inPtr-1, "macro menu item", "expecting '{'");
      return NULL;
   }

   /* Parse the input */
   prog = ParseMacro(*inPtr, &errMsg, &stoppedAt);
   if (prog == NULL)
   {
      ParseError(NULL, *inPtr, stoppedAt, "macro menu item", errMsg);
      return NULL;
   }
   FreeProgram(prog);

   /* Copy and return the body of the macro, stripping outer braces and
      extra leading tabs added by the writer routine */
   (*inPtr)++;
   *inPtr += strspn(*inPtr, " \t");
   if (**inPtr == '\n')(*inPtr)++;
   if (**inPtr == '\t')(*inPtr)++;
   if (**inPtr == '\t')(*inPtr)++;
   retPtr = retStr = new char[stoppedAt - *inPtr + 1];
   for (p = *inPtr; p < stoppedAt - 1; p++)
   {
      if (!strncmp(p, "\n\t\t", 3))
      {
         *retPtr++ = '\n';
         p += 2;
      }
      else
         *retPtr++ = *p;
   }
   if (*(retPtr-1) == '\t') retPtr--;
   *retPtr = '\0';
   *inPtr = stoppedAt;
   return retStr;
}

/*
** If "*string" is not terminated with a newline character, reallocate the
** string and add one.  (The macro language requires newline terminators for
** statements, but the text widget doesn't force it like the NEdit text buffer
** does, so this might avoid some confusion.)
*/
static void addTerminatingNewline(char** string)
{
   char* newString;
   int length;

   length = strlen(*string);
   if ((*string)[length-1] != '\n')
   {
      newString = new char[length + 2];
      strcpy(newString, *string);
      newString[length] = '\n';
      newString[length+1] = '\0';
      delete[] *string;
      *string = newString;
   }
}

/*
** Cache user menus:
** allocate an empty user (shell, macro) menu cache structure
*/
UserMenuCache* CreateUserMenuCache()
{
   /* allocate some memory for the new data structure */
   UserMenuCache* cache = new UserMenuCache();

   cache->umcLanguageMode              = -2;
   cache->umcShellMenuCreated          =  false;
   cache->umcMacroMenuCreated          =  false;
   cache->umcShellMenuList.umlNbrItems =  0;
   cache->umcShellMenuList.umlItems    =  NULL;
   cache->umcMacroMenuList.umlNbrItems =  0;
   cache->umcMacroMenuList.umlItems    =  NULL;

   return cache;
}

// --------------------------------------------------------------------------
void FreeUserMenuCache(UserMenuCache* cache)
{
   freeUserMenuList(&cache->umcShellMenuList);
   freeUserMenuList(&cache->umcMacroMenuList);

   delete cache;
}

/*
** Cache user menus:
** init. a user background menu cache structure
*/
void InitUserBGMenuCache(UserBGMenuCache* cache)
{
   cache->ubmcLanguageMode         = -2;
   cache->ubmcMenuCreated          =  false;
   cache->ubmcMenuList.umlNbrItems =  0;
   cache->ubmcMenuList.umlItems    =  NULL;
}

void FreeUserBGMenuCache(UserBGMenuCache* cache)
{
   freeUserMenuList(&cache->ubmcMenuList);
}

/*
** Cache user menus:
** Parse given menu item list and setup a user menu info list for
** management of user menu.
*/
static void parseMenuItemList(menuItemRec** itemList, int nbrOfItems, userMenuInfo** infoList, userSubMenuCache* subMenus)
{
   int i;
   userMenuInfo* info;

   /* Allocate storage for structures to keep track of sub-menus */
   allocSubMenuCache(subMenus, nbrOfItems);

   /* 1st pass: setup user menu info: extract language modes, menu name &
      default indication; build user menu ID */
   for (i=0; i<nbrOfItems; i++)
   {
      infoList[i] = parseMenuItemRec(itemList[i]);
      generateUserMenuId(infoList[i], subMenus);
   }

   /* 2nd pass: solve "default" dependencies */
   for (i=0; i<nbrOfItems; i++)
   {
      info = infoList[i];

      /* If the user menu item is a default one, then scan the list for
         items with the same name and a language mode specified.
         If one is found, then set the default index to the index of the
         current default item. */
      if (info->umiIsDefault)
      {
         setDefaultIndex(infoList, nbrOfItems, i);
      }
   }
}

/*
** Returns the sub-menu depth (i.e. nesting level) of given
** menu name.
*/
static int getSubMenuDepth(const char* menuName)
{
   const char* subSep;
   int depth = 0;

   /* determine sub-menu depth by counting '>' of given "menuName" */
   subSep = menuName;
   while ((subSep = strchr(subSep, '>')) != NULL)
   {
      depth ++;
      subSep ++;
   }

   return depth;
}

/*
** Cache user menus:
** Parse a singe menu item. Allocate & setup a user menu info element
** holding extracted info.
*/
static userMenuInfo* parseMenuItemRec(menuItemRec* item)
{
   userMenuInfo* newInfo;
   int subMenuDepth;
   int idSize;

   /* allocate a new user menu info element */
   newInfo = new userMenuInfo();

   /* determine sub-menu depth and allocate some memory
      for hierarchical ID; init. ID with {0,.., 0} */
   newInfo->umiName = stripLanguageMode(item->name);

   subMenuDepth = getSubMenuDepth(newInfo->umiName);
   idSize       = sizeof(int)*(subMenuDepth+1);

   newInfo->umiId = new int[idSize];
   memset(newInfo->umiId,0,idSize);

   /* init. remaining parts of user menu info element */
   newInfo->umiIdLen              = 0;
   newInfo->umiIsDefault          = false;
   newInfo->umiNbrOfLanguageModes = 0;
   newInfo->umiLanguageMode       = NULL;
   newInfo->umiDefaultIndex       = -1;
   newInfo->umiToBeManaged        = false;

   /* assign language mode info to new user menu info element */
   parseMenuItemName(item->name, newInfo);

   return newInfo;
}

/*
** Cache user menus:
** Extract language mode related info out of given menu item name string.
** Store this info in given user menu info structure.
*/
static void parseMenuItemName(char* menuItemName, userMenuInfo* info)
{
   char* atPtr, *firstAtPtr, *endPtr;
   char c;
   int languageMode;
   int langModes[MAX_LANGUAGE_MODES];
   int nbrLM = 0;
   int size;

   atPtr = firstAtPtr = strchr(menuItemName, '@');
   if (atPtr != NULL)
   {
      if (!strcmp(atPtr+1, "*"))
      {
         // only language is "*": this is for all but language specific macros
         info->umiIsDefault = true;
         return;
      }

      /* setup a list of all language modes related to given menu item */
      while (atPtr != NULL)
      {
         /* extract language mode name after "@" sign */
         for (endPtr=atPtr+1; isalnum((unsigned char)*endPtr) || *endPtr=='_' ||
               *endPtr=='-' || *endPtr==' ' || *endPtr=='+' || *endPtr=='$' ||
               *endPtr=='#'; endPtr++);

         /* lookup corresponding language mode index; if PLAIN is
            returned then this means, that language mode name after
            "@" is unknown (i.e. not defined) */
         c = *endPtr;
         *endPtr = '\0';
         languageMode = FindLanguageMode(atPtr+1);
         if (languageMode == PLAIN_LANGUAGE_MODE)
         {
            langModes[nbrLM] = UNKNOWN_LANGUAGE_MODE;
         }
         else
         {
            langModes[nbrLM] = languageMode;
         }
         nbrLM ++;
         *endPtr = c;

         /* look for next "@" */
         atPtr = strchr(endPtr, '@');
      }

      if (nbrLM != 0)
      {
         info->umiNbrOfLanguageModes = nbrLM;
         size = sizeof(int)*nbrLM;
         info->umiLanguageMode = new int[size];
         memcpy(info->umiLanguageMode, langModes, size);
      }
   }
}

/*
** Cache user menus:
** generates an ID (= array of integers) of given user menu info, which
** allows to find the user menu  item within the menu tree later on: 1st
** integer of ID indicates position within main menu; 2nd integer indicates
** position within 1st sub-menu etc.
*/
static void generateUserMenuId(userMenuInfo* info, userSubMenuCache* subMenus)
{
   int idSize;
   char* hierName, *subSep;
   int subMenuDepth = 0;
   int* menuIdx = &subMenus->usmcNbrOfMainMenuItems;
   userSubMenuInfo* curSubMenu;

   /* find sub-menus, stripping off '>' until item name is reached */
   subSep = info->umiName;
   while ((subSep = strchr(subSep, '>')) != NULL) // TODO: Using '/' ?
   {
      hierName = copySubstring(info->umiName, subSep - info->umiName);
      curSubMenu = findSubMenuInfo(subMenus, hierName);
      if (curSubMenu == NULL)
      {
         /* sub-menu info not stored before: new sub-menu;
            remember its hierarchical position */
         info->umiId[subMenuDepth] = *menuIdx;
         (*menuIdx) ++;

         /* store sub-menu info in list of subMenus; allocate
            some memory for hierarchical ID of sub-menu & take over
            current hierarchical ID of current user menu info */
         curSubMenu = &subMenus->usmcInfo[subMenus->usmcNbrOfSubMenus];
         subMenus->usmcNbrOfSubMenus ++;
         curSubMenu->usmiName  = hierName;
         idSize = sizeof(int)*(subMenuDepth+2);
         curSubMenu->usmiId = new int[idSize];
         memcpy(curSubMenu->usmiId, info->umiId, idSize);
         curSubMenu->usmiIdLen = subMenuDepth+1;
      }
      else
      {
         /* sub-menu info already stored before: takeover its
            hierarchical position */
         delete[]  hierName;
         info->umiId[subMenuDepth] = curSubMenu->usmiId[subMenuDepth];
      }

      subMenuDepth ++;
      menuIdx = &curSubMenu->usmiId[subMenuDepth];

      subSep ++;
   }

   /* remember position of menu item within final (sub) menu */
   info->umiId[subMenuDepth] = *menuIdx;
   info->umiIdLen = subMenuDepth + 1;
   (*menuIdx) ++;
}

/*
** Cache user menus:
** Find info corresponding to a hierarchical menu name (a>b>c...)
*/
static userSubMenuInfo* findSubMenuInfo(userSubMenuCache* subMenus,
                                        const char* hierName)
{
   int i;

   for (i=0; i<subMenus->usmcNbrOfSubMenus; i++)
      if (!strcmp(hierName, subMenus->usmcInfo[i].usmiName))
         return &subMenus->usmcInfo[i];
   return NULL;
}

/*
** Cache user menus:
** Returns an allocated copy of menuItemName stripped of language mode
** parts (i.e. parts starting with "@").
*/
static char* stripLanguageMode(const char* menuItemName)
{
   const char* firstAtPtr = strchr(menuItemName, '@');
   if (firstAtPtr == NULL)
      return NeNewString(menuItemName);
   else
      return copySubstring(menuItemName, firstAtPtr-menuItemName);
}

// --------------------------------------------------------------------------
static void setDefaultIndex(userMenuInfo** infoList, int nbrOfItems, int defaultIdx)
{
   char* defaultMenuName = infoList[defaultIdx]->umiName;
   userMenuInfo* info;

   /* Scan the list for items with the same name and a language mode
      specified. If one is found, then set the default index to the
      index of the current default item. */
   for (int i=0; i<nbrOfItems; i++)
   {
      info = infoList[i];

      if (!info->umiIsDefault && strcmp(info->umiName, defaultMenuName)==0)
      {
         info->umiDefaultIndex = defaultIdx;
      }
   }
}

// --------------------------------------------------------------------------
// Determine the info list menu items, which need to be managed
// for given language mode. Set / reset "to be managed" indication
// of info list items accordingly.
// --------------------------------------------------------------------------
static void applyLangModeToUserMenuInfo(userMenuInfo** infoList, int nbrOfItems, int languageMode)
{
   userMenuInfo* info;

   /* 1st pass: mark all items as "to be managed", which are applicable
      for all language modes or which are indicated as "default" items */
   for (int i=0; i<nbrOfItems; i++)
   {
      info = infoList[i];

      info->umiToBeManaged =
         (info->umiNbrOfLanguageModes == 0 || info->umiIsDefault);
   }

   /* 2nd pass: mark language mode specific items matching given language
      mode as "to be managed". Reset "to be managed" indications of
      "default" items, if applicable */
   for (int i=0; i<nbrOfItems; i++)
   {
      info = infoList[i];

      if (info->umiNbrOfLanguageModes != 0)
      {
         if (doesLanguageModeMatch(info, languageMode))
         {
            info->umiToBeManaged = true;

            if (info->umiDefaultIndex != -1)
               infoList[info->umiDefaultIndex]->umiToBeManaged = false;
         }
      }
   }
}

// --------------------------------------------------------------------------
// Returns true, if given user menu info is applicable for given language mode
// --------------------------------------------------------------------------
static int doesLanguageModeMatch(userMenuInfo* info, int languageMode)
{
   for (int i=0; i<info->umiNbrOfLanguageModes; i++)
   {
      if (info->umiLanguageMode[i] == languageMode)
         return true;
   }

   return false;
}

// --------------------------------------------------------------------------
static void freeUserMenuInfoList(userMenuInfo** infoList, int nbrOfItems)
{
   for (int i=0; i<nbrOfItems; i++)
   {
      freeUserMenuInfo(infoList[i]);
   }
}

// --------------------------------------------------------------------------
static void freeUserMenuInfo(userMenuInfo* info)
{
   delete[] info->umiName;

   delete[] info->umiId;

   if (info->umiNbrOfLanguageModes != 0)
      delete[] info->umiLanguageMode;

   delete info;
}

// --------------------------------------------------------------------------
// Cache user menus:
// Allocate & init. storage for structures to manage sub-menus
// --------------------------------------------------------------------------
static void allocSubMenuCache(userSubMenuCache* subMenus, int nbrOfItems)
{
   subMenus->usmcNbrOfMainMenuItems = 0;
   subMenus->usmcNbrOfSubMenus = 0;
   subMenus->usmcInfo = new userSubMenuInfo[nbrOfItems];
}

// --------------------------------------------------------------------------
static void freeSubMenuCache(userSubMenuCache* subMenus)
{
   int i;

   for (i=0; i<subMenus->usmcNbrOfSubMenus; i++)
   {
      delete[] subMenus->usmcInfo[i].usmiName;
      delete[] subMenus->usmcInfo[i].usmiId;
   }

   delete[] subMenus->usmcInfo;
}

// --------------------------------------------------------------------------
static void allocUserMenuList(UserMenuList* list, int nbrOfItems)
{
   int size = sizeof(UserMenuListElement*) * nbrOfItems;

   list->umlNbrItems = 0;
   list->umlItems = new UserMenuListElement*[size];
}

// --------------------------------------------------------------------------
static void freeUserMenuList(UserMenuList* list)
{
   int i;

   for (i=0; i<list->umlNbrItems; i++)
      freeUserMenuListElement(list->umlItems[i]);

   list->umlNbrItems = 0;

   delete[] list->umlItems;
   list->umlItems = NULL;
}

// --------------------------------------------------------------------------
static UserMenuListElement* allocUserMenuListElement(Fl_Widget* menuItem, char* accKeys)
{
   UserMenuListElement* element = new UserMenuListElement();

   element->umleManageMode          = UMMM_UNMANAGE;
   element->umlePrevManageMode      = UMMM_UNMANAGE;
   element->umleAccKeys             = accKeys;
   element->umleAccLockPatchApplied = false;
   element->umleMenuItem            = menuItem;
   element->umleSubMenuPane         = NULL;
   element->umleSubMenuList         = NULL;

   return element;
}

// --------------------------------------------------------------------------
static void freeUserMenuListElement(UserMenuListElement* element)
{
   if (element->umleSubMenuList != NULL)
      freeUserSubMenuList(element->umleSubMenuList);

   delete[] element->umleAccKeys;
   delete[] element;
}

// --------------------------------------------------------------------------
static UserMenuList* allocUserSubMenuList(int nbrOfItems)
{
   UserMenuList* list = new UserMenuList();

   allocUserMenuList(list, nbrOfItems);

   return list;
}

// --------------------------------------------------------------------------
static void freeUserSubMenuList(UserMenuList* list)
{
   freeUserMenuList(list);

   delete list;
}
