static const char CVSID[] = "$Id: getfiles.c,v 1.37 2008/02/29 16:06:05 tringali Exp $";
/*******************************************************************************
*                                                                              *
* Getfiles.c -- File Interface Routines                                        *
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
* May 23, 1991                                                                 *
*                                                                              *
* Written by Donna Reid                                                        *
*                                                                              *
* modified 11/5/91 by JMK: integrated changes made by M. Edel; updated for     *
*                          destroy widget problem (took out ManageModalDialog  *
*                          call; added comments.                               *
*          10/1/92 by MWE: Added help dialog and fixed a few bugs              *
*           4/7/93 by DR:  Port to VMS                                         *
*           6/1/93 by JMK: Integrate Port and changes by MWE to make           *
*                          directories "sticky" and a fix to prevent opening   *
*                          a directory when no filename was specified          *
*          6/24/92 by MWE: Made filename list and directory list typeable,     *
*                          set initial focus to filename list                  *
*          6/25/93 by JMK: Fix memory leaks found by Purify.                   *
*                                                                              *
* Included are two routines written using Motif for accessing files:           *
*                                                                              *
* GetExistingFilename  presents a FileSelectionBox dialog where users can      *
*                      choose an existing file to open.                        *
*                                                                              *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "getfiles.h"
#include "fileUtils.h"
#include "misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>
#ifndef WIN32
#include <sys/param.h>
#include <unistd.h>
#include <dirent.h>
#endif // WIN32
#include <sys/stat.h>

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

#define MAX_ARGS 20			/* Maximum number of X arguments */
#define PERMS 0666     			/* UNIX file permission, RW for owner,
group, world */
#define MAX_LIST_KEYSTROKES 100		/* Max # of keys user can type to 
a file list */
#define MAX_LIST_KESTROKE_WAIT 2000	/* Allowable delay in milliseconds
between characters typed to a list
before starting over (throwing
out the accumulated characters */

#define SET_ONE_RSRC(widget, name, newValue) \
{ \
   static Arg tmpargs[1] = {{name, (XtArgVal)0}}; \
   tmpargs[0].value = (XtArgVal)newValue; \
   XtSetValues(widget, tmpargs, 1); \
}

enum yesNoValues {ynNone, ynYes, ynNo};

/* Saved default directory and pattern from last successful call */
static NeString DefaultDirectory = NULL;
static NeString DefaultPattern = NULL;

/* User settable option for leaving the file name text field in
GetExistingFilename dialogs.  Off by default so new users will get
used to typing in the list rather than in the text field */
static int RemoveRedundantTextField = true;

/* Text for help button help display */
/* ... needs variant for VMS */
static const char* HelpExist =
   "The file open dialog shows a list of directories on the left, and a list \
   of files on the right.  Double clicking on a file name in the list on the \
   right, or selecting it and pressing the OK button, will open that file.  \
   Double clicking on a directory name, or selecting \
   it and pressing \"Filter\", will move into that directory.  To move upwards in \
   the directory tree, double click on the directory entry ending in \"..\".  \
   You can also begin typing a file name to select from the file list, or \
   directly type in directory and file specifications in the \
   field labeled \"Filter\".\n\
   \n\
   If you use the filter field, remember to include \
   either a file name, \"*\" is acceptable, or a trailing \"/\".  If \
   you don't, the name after the last \"/\" is interpreted as the file name to \
   match.  When you leave off the file name or trailing \"/\", you won't see \
   any files to open in the list \
   because the filter specification matched the directory file itself, rather \
   than the files in the directory.";

static const char* HelpNew =
   "This dialog allows you to create a new file, or to save the current file \
   under a new name.  To specify a file \
   name in the current directory, complete the name displayed in the \"Save File \
   As:\" field near the bottom of the dialog.  If you delete or change \
   the path shown in the field, the file will be saved using whatever path \
   you type, provided that it is a valid Unix file specification.\n\
   \n\
   To replace an existing file, select it from the Files list \
   and press \"OK\", or simply double click on the name.\n\
   \n\
   To save a file in another directory, use the Directories list \
   to move around in the file system hierarchy.  Double clicking on \
   directory names in the list, or selecting them and pressing the \
   \"Filter\" button will select that directory.  To move upwards \
   in the directory tree, double \
   click on the directory entry ending in \"..\".  You can also move directly \
   to a directory by typing the file specification of the path in the \"Filter\" \
   field and pressing the \"Filter\" button.";


// TODO:                       /*                    Local Callback Routines and variables                */
// TODO: 
// TODO:                       static void newFileOKCB(Widget w, bool* client_data,
// TODO:                             XmFileSelectionBoxCallbackStruct* call_data);
// TODO:                       static void newFileCancelCB(Widget w, bool* client_data, caddr_t
// TODO:                             call_data);
// TODO:                       static void newHelpCB(Widget w, Widget helpPanel, caddr_t call_data);
// TODO:                       static void createYesNoDialog(Widget parent);
// TODO:                       static void createErrorDialog(Widget parent);
// TODO:                       static int  doYesNoDialog(const char* msg);
// TODO:                       static void doErrorDialog(const char* errorString, const char* filename);
// TODO:                       static void existOkCB(Widget w, bool* client_data,
// TODO:                             XmFileSelectionBoxCallbackStruct* call_data);
// TODO:                       static void existCancelCB(Widget w, bool* client_data, caddr_t call_data);
// TODO:                       static void existHelpCB(Widget w, Widget helpPanel, caddr_t call_data);
// TODO:                       static void errorOKCB(Widget w, caddr_t client_data, caddr_t call_data);
// TODO:                       static void yesNoOKCB(Widget w, caddr_t client_data, caddr_t call_data);
// TODO:                       static void yesNoCancelCB(Widget w, caddr_t client_data, caddr_t call_data);
// TODO:                       static Widget createPanelHelp(Widget parent, const char* text, const char* title);
// TODO:                       static void helpDismissCB(Widget w, Widget helpPanel, caddr_t call_data);
// TODO:                       static void makeListTypeable(Widget listW);
// TODO:                       static void listCharEH(Widget w, XtPointer callData, XEvent* event,
// TODO:                             bool* continueDispatch);
// TODO:                       static void replacementDirSearchProc(Widget w, XtPointer searchData);
// TODO:                       static void replacementFileSearchProc(Widget w, XtPointer searchData);
// TODO:                       static void sortWidgetList(Widget listWidget);
// TODO:                       static int compareXmStrings(const void* string1, const void* string2);
// TODO: 
// TODO:                       static int  SelectResult = GFN_CANCEL;  /*  Initialize results as cancel   */
// TODO:                       static Widget YesNoDialog;		/* "Overwrite?" dialog widget	   */
// TODO:                       static int YesNoResult;			/* Result of overwrite dialog	   */
// TODO:                       static Widget ErrorDialog;		/* Dialog widget for error msgs	   */
// TODO:                       static int ErrorDone;			/* Flag to mark dialog completed   */
// TODO:                       static void (*OrigDirSearchProc)(Widget, XtPointer);	/* Built in Motif directory search */
// TODO:                       static void (*OrigFileSearchProc)(Widget, XtPointer);	/* Built in Motif file search proc */
// TODO: 
// TODO:                       /*
// TODO:                        * Do the hard work of setting up a file selection dialog
// TODO:                        */
// TODO:                       Widget getFilenameHelper(Widget parent, char* promptString, char* filename,
// TODO:                             int existing)
// TODO: {
// TODO:    int       n;                      /* number of arguments               */
// TODO:    Arg	      args[MAX_ARGS];	      /* arg list	                   */
// TODO:    Widget    fileSB;	              /* widget file select box 	   */
// TODO:    NeString  titleString;	      /* compound string for dialog title  */
// TODO: 
// TODO:    n = 0;
// TODO:    titleString = NeNewString(promptString);
// TODO:    XtSetArg(args[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
// TODO:    n++;
// TODO:    XtSetArg(args[n], XmNdialogTitle, titleString);
// TODO:    n++;
// TODO:    fileSB = CreateFileSelectionDialog(parent,"FileSelect",args,n);
// TODO:    NeStringFree(titleString);
// TODO:    if (existing && RemoveRedundantTextField)
// TODO:       XtUnmanageChild(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_TEXT));
// TODO:    XtUnmanageChild(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_SELECTION_LABEL));
// TODO: 
// TODO:    XtVaSetValues(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_FILTER_LABEL),
// TODO:                  XmNmnemonic, 'l',
// TODO:                  XmNuserData, XmFileSelectionBoxGetChild(fileSB, XmDIALOG_FILTER_TEXT),
// TODO:                  NULL);
// TODO:    XtVaSetValues(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_DIR_LIST_LABEL),
// TODO:                  XmNmnemonic, 'D',
// TODO:                  XmNuserData, XmFileSelectionBoxGetChild(fileSB, XmDIALOG_DIR_LIST),
// TODO:                  NULL);
// TODO:    XtVaSetValues(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_LIST_LABEL),
// TODO:                  XmNmnemonic, promptString[strspn(promptString, "lD")],
// TODO:                  XmNuserData, XmFileSelectionBoxGetChild(fileSB, XmDIALOG_LIST),
// TODO:                  NULL);
// TODO:    AddDialogMnemonicHandler(fileSB, FALSE);
// TODO:    RemapDeleteKey(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_FILTER_TEXT));
// TODO:    RemapDeleteKey(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_TEXT));
// TODO:    return fileSB;
// TODO: }

/*  GetExistingFilename				  	                   */
/*									   */
/*  This routine will popup a file selection box so that the user can      */
/*  select an existing file from the scrollable list.  The user is         */
/*  prevented from entering a new filename because the edittable text      */
/*  area of the file selection box widget is unmanaged.  After the user    */
/*  selects a file, GetExistingFilename returns the selected filename and  */
/*  GFN_OK, indicating that the OK button was pressed.  If the user        */
/*  pressed the cancel button, the return value is GFN_CANCEL, and the     */
/*  filename character string supplied in the call is not altered.	   */
/*									   */
/*  Arguments:								   */
/*									   */
/*	Widget  parent	      - parent widget id			   */
/*	char *  promptString  - prompt string				   */
/*	char *  filename      - a string to receive the selected filename  */
/*				(this string will not be altered if the    */
/*				user pressed the cancel button)		   */
/*									   */
/*  Returns:	GFN_OK	      - file was selected and OK button pressed	   */
/*		GFN_CANCEL    - Cancel button pressed and no returned file */
/*									   */
int GetExistingFilename(Fl_Widget* parent, char* promptString, char* filename)
{
// TODO:    Widget existFileSB = getFilenameHelper(parent, promptString, filename, true);
// TODO:    return HandleCustomExistFileSB(existFileSB, filename);
   return 0;
}

/* GetNewFilename
 *
 * Same as GetExistingFilename but pick a new file instead of an existing one.
 * In this case the text area of the FSB is *not* unmanaged, so the user can
 * enter a new filename.
 */
int GetNewFilename(Fl_Widget* parent, char* promptString, char* filename, char* defaultName)
{
// TODO:    Widget fileSB = getFilenameHelper(parent, promptString, filename, false);
// TODO:    return HandleCustomNewFileSB(fileSB, filename, defaultName);
   return 0;
}

// TODO: /*
// TODO: ** HandleCustomExistFileSB
// TODO: **
// TODO: ** Manage a customized file selection box for opening existing files.
// TODO: ** Use this if you want to change the standard file selection dialog
// TODO: ** from the defaults provided in GetExistingFilename, but still
// TODO: ** want take advantage of the button processing, help messages, and
// TODO: ** file checking of GetExistingFilename.
// TODO: **
// TODO: **  Arguments:
// TODO: **
// TODO: **	Widget  existFileSB   - your custom file selection box widget id
// TODO: **	char *  filename      - a string to receive the selected filename
// TODO: **				(this string will not be altered if the
// TODO: **				user pressed the cancel button)
// TODO: **
// TODO: **  Returns:	GFN_OK	      - file was selected and OK button pressed
// TODO: **		GFN_CANCEL    - Cancel button pressed and no returned file
// TODO: **
// TODO: */
// TODO: int HandleCustomExistFileSB(Widget existFileSB, char* filename)
// TODO: {
// TODO:    bool   done_with_dialog=false; /* ok to destroy dialog flag	   */
// TODO:    char*      fileString;            /* C string for file selected        */
// TODO:    char*      dirString;             /* C string for dir of file selected */
// TODO:    NeString  cFileString;            /* compound string for file selected */
// TODO:    NeString  cDir;	              /* compound directory selected	   */
// TODO:    NeString  cPattern;               /* compound filter pattern	   */
// TODO:    Widget    help;		      /* help window form dialog	   */
// TODO: #if XmVersion < 1002
// TODO:    int       i;
// TODO: #endif
// TODO: 
// TODO:    XtAddCallback(existFileSB, XmNokCallback, (XtCallbackProc)existOkCB,
// TODO:                  &done_with_dialog);
// TODO:    XtAddCallback(existFileSB, XmNcancelCallback, (XtCallbackProc)existCancelCB,
// TODO:                  &done_with_dialog);
// TODO:    AddMotifCloseCallback(XtParent(existFileSB), (XtCallbackProc)existCancelCB,
// TODO:                          &done_with_dialog);
// TODO:    help = createPanelHelp(existFileSB, HelpExist, "Selecting Files to Open");
// TODO:    createErrorDialog(existFileSB);
// TODO:    XtAddCallback(existFileSB, XmNhelpCallback, (XtCallbackProc)existHelpCB,
// TODO:                  (char*)help);
// TODO:    if (DefaultDirectory != NULL || DefaultPattern != NULL)
// TODO:       XtVaSetValues(existFileSB, XmNdirectory, DefaultDirectory,
// TODO:                     XmNpattern, DefaultPattern, NULL);
// TODO:    makeListTypeable(XmFileSelectionBoxGetChild(existFileSB,XmDIALOG_LIST));
// TODO:    makeListTypeable(XmFileSelectionBoxGetChild(existFileSB,XmDIALOG_DIR_LIST));
// TODO: #if XmVersion >= 1002
// TODO:    XtVaSetValues(existFileSB, XmNinitialFocus, XtParent(
// TODO:                     XmFileSelectionBoxGetChild(existFileSB, XmDIALOG_LIST)), NULL);
// TODO: #endif
// TODO:    ManageDialogCenteredOnPointer(existFileSB);
// TODO: 
// TODO:    /* Typing in the directory list is dependent on the list being in the
// TODO:       same form of alphabetical order expected by the character processing
// TODO:       routines.  As of about 1.2.3, some Motif libraries seem to have a
// TODO:       different idea of ordering than is usual for Unix directories.
// TODO:       To sort them properly, we have to patch the directory and file
// TODO:       searching routines to re-sort the lists when they change */
// TODO:    XtVaGetValues(existFileSB, XmNdirSearchProc, &OrigDirSearchProc,
// TODO:                  XmNfileSearchProc, &OrigFileSearchProc, NULL);
// TODO:    XtVaSetValues(existFileSB, XmNdirSearchProc, replacementDirSearchProc,
// TODO:                  XmNfileSearchProc, replacementFileSearchProc, NULL);
// TODO:    sortWidgetList(XmFileSelectionBoxGetChild(existFileSB, XmDIALOG_DIR_LIST));
// TODO:    sortWidgetList(XmFileSelectionBoxGetChild(existFileSB, XmDIALOG_LIST));
// TODO: #if XmVersion < 1002
// TODO:    /* To give file list initial focus, revoke default button status for
// TODO:       the "OK" button.  Dynamic defaulting will restore it as the default
// TODO:       button after the keyboard focus is established.  Note the voodoo
// TODO:       below: calling XmProcess traversal extra times (a recommendation from
// TODO:       OSF technical support) somehow succeedes in giving the file list focus */
// TODO:    XtVaSetValues(existFileSB, XmNdefaultButton, NULL, NULL);
// TODO:    for (i=1; i<30; i++)
// TODO:       XmProcessTraversal(XmFileSelectionBoxGetChild(existFileSB,
// TODO:                          XmDIALOG_LIST), XmTRAVERSE_CURRENT);
// TODO: #endif
// TODO: 
// TODO:    while (!done_with_dialog)
// TODO:       XtAppProcessEvent(XtWidgetToApplicationContext(existFileSB), XtIMAll);
// TODO: 
// TODO:    if (SelectResult == GFN_OK)
// TODO:    {
// TODO:       XtVaGetValues(existFileSB, XmNdirSpec, &cFileString, XmNdirectory,
// TODO:                     &cDir, XmNpattern, &cPattern, NULL);
// TODO:       /* Undocumented: file selection box widget allocates copies of these
// TODO:          strings on getValues calls.  I have risked freeing them to avoid
// TODO:          memory leaks, since I assume other developers have made this same
// TODO:          realization, therefore OSF can't easily go back and change it */
// TODO:       if (DefaultDirectory != NULL) NeStringFree(DefaultDirectory);
// TODO:       if (DefaultPattern != NULL) NeStringFree(DefaultPattern);
// TODO:       DefaultDirectory = cDir;
// TODO:       DefaultPattern = cPattern;
// TODO:       XmStringGetLtoR(cFileString, XmSTRING_DEFAULT_CHARSET, &fileString);
// TODO:       /* Motif 2.x seem to contain a bug that causes it to return only
// TODO:          the relative name of the file in XmNdirSpec when XmNpathMode is set
// TODO:          to XmPATH_MODE_RELATIVE (through X resources), although the man
// TODO:          page states that it always returns the full path name. We can
// TODO:          easily work around this by checking that the first character of the
// TODO:          file name is a `/'. */
// TODO: #ifdef VMS
// TODO:       /* VMS  won't return `/' as the 1st character of the full file spec.
// TODO:         `:' terminates the device name and is not allowed elsewhere */
// TODO:       if (strchr(fileString, ':') != NULL)
// TODO:       {
// TODO: #else
// TODO:       if (fileString[0] == '/')
// TODO:       {
// TODO: #endif        /* VMS */
// TODO:          /* The directory name is already present in the file name or
// TODO:             the user entered a full path name. */
// TODO:          strcpy(filename, fileString);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          /* Concatenate the directory name and the file name */
// TODO:          XmStringGetLtoR(cDir, XmSTRING_DEFAULT_CHARSET, &dirString);
// TODO:          strcpy(filename, dirString);
// TODO:          strcat(filename, fileString);
// TODO:          XtFree(dirString);
// TODO:       }
// TODO:       NeStringFree(cFileString);
// TODO:       XtFree(fileString);
// TODO:    }
// TODO:    /* Destroy the dialog _shell_ iso. the dialog. Normally, this shouldn't
// TODO:       be necessary as the shell is destroyed automatically when the dialog
// TODO:       is. However, due to a bug in various Lesstif versions, the latter
// TODO:       messes up the grab cascades and leaves new windows without grabs, such
// TODO:       that they appear to be frozen. */
// TODO:    XtDestroyWidget(XtParent(existFileSB));
// TODO:    return SelectResult;
// TODO: }
// TODO: 
// TODO: 
// TODO: /*
// TODO: ** HandleCustomNewFileSB
// TODO: **
// TODO: ** Manage a customized file selection box for opening new files.
// TODO: **
// TODO: **  Arguments:
// TODO: **
// TODO: **	Widget  newFileSB     - your custom file selection box widget id
// TODO: **	char *  filename      - a string to receive the selected filename
// TODO: **				(this string will not be altered if the
// TODO: **				user pressed the cancel button)
// TODO: **  	char*	defaultName   - default name to be pre-entered in filename
// TODO: **  	    	    	    	text field.
// TODO: **
// TODO: **  Returns:	GFN_OK	      - file was selected and OK button pressed
// TODO: **		GFN_CANCEL    - Cancel button pressed and no returned file
// TODO: **
// TODO: */
// TODO: int HandleCustomNewFileSB(Widget newFileSB, char* filename, char* defaultName)
// TODO: {
// TODO:    bool   done_with_dialog=false; /* ok to destroy dialog flag	   */
// TODO:    Widget    help;		      /* help window form dialog	   */
// TODO:    NeString  cFileString;            /* compound string for file selected */
// TODO:    NeString  cDir;	              /* compound directory selected	   */
// TODO:    NeString  cPattern;               /* compound filter pattern	   */
// TODO:    char*      fileString;            /* C string for file selected        */
// TODO:    char*      dirString;             /* C string for dir of file selected */
// TODO: #if XmVersion < 1002
// TODO:    int       i;
// TODO: #endif
// TODO: 
// TODO:    XtAddCallback(newFileSB, XmNokCallback, (XtCallbackProc)newFileOKCB,
// TODO:                  &done_with_dialog);
// TODO:    XtAddCallback(newFileSB, XmNcancelCallback, (XtCallbackProc)newFileCancelCB,
// TODO:                  &done_with_dialog);
// TODO: 
// TODO:    makeListTypeable(XmFileSelectionBoxGetChild(newFileSB,XmDIALOG_LIST));
// TODO:    makeListTypeable(XmFileSelectionBoxGetChild(newFileSB,XmDIALOG_DIR_LIST));
// TODO:    if (DefaultDirectory != NULL || DefaultPattern != NULL)
// TODO:       XtVaSetValues(newFileSB, XmNdirectory, DefaultDirectory,
// TODO:                     XmNpattern, DefaultPattern, NULL);
// TODO:    help = createPanelHelp(newFileSB, HelpNew, "Saving a File");
// TODO:    createYesNoDialog(newFileSB);
// TODO:    createErrorDialog(newFileSB);
// TODO:    XtAddCallback(newFileSB, XmNhelpCallback, (XtCallbackProc)newHelpCB,
// TODO:                  (char*)help);
// TODO: #if XmVersion >= 1002
// TODO:    XtVaSetValues(newFileSB, XmNinitialFocus,
// TODO:                  XmFileSelectionBoxGetChild(newFileSB, XmDIALOG_TEXT), NULL);
// TODO: #endif
// TODO:    ManageDialogCenteredOnPointer(newFileSB);
// TODO: 
// TODO: #if XmVersion < 1002
// TODO:    /* To give filename text initial focus, revoke default button status for
// TODO:       the "OK" button.  Dynamic defaulting will restore it as the default
// TODO:       button after the keyboard focus is established.  Note the voodoo
// TODO:       below: calling XmProcess traversal FOUR times (a recommendation from
// TODO:       OSF technical support) somehow succeedes in changing the focus */
// TODO:    XtVaSetValues(newFileSB, XmNdefaultButton, NULL, NULL);
// TODO:    for (i=1; i<30; i++)
// TODO:       XmProcessTraversal(XmFileSelectionBoxGetChild(newFileSB, XmDIALOG_TEXT),
// TODO:                          XmTRAVERSE_CURRENT);
// TODO: #endif
// TODO: 
// TODO:    /* Typing in the directory list is dependent on the list being in the
// TODO:       same form of alphabetical order expected by the character processing
// TODO:       routines.  As of about 1.2.3, some Motif libraries seem to have a
// TODO:       different idea of ordering than is usual for Unix directories.
// TODO:       To sort them properly, we have to patch the directory and file
// TODO:       searching routines to re-sort the lists when they change */
// TODO:    XtVaGetValues(newFileSB, XmNdirSearchProc, &OrigDirSearchProc,
// TODO:                  XmNfileSearchProc, &OrigFileSearchProc, NULL);
// TODO:    XtVaSetValues(newFileSB, XmNdirSearchProc, replacementDirSearchProc,
// TODO:                  XmNfileSearchProc, replacementFileSearchProc, NULL);
// TODO:    sortWidgetList(XmFileSelectionBoxGetChild(newFileSB, XmDIALOG_DIR_LIST));
// TODO:    sortWidgetList(XmFileSelectionBoxGetChild(newFileSB, XmDIALOG_LIST));
// TODO: 
// TODO:    /* Delay the setting of the default name till after the replacement of
// TODO:       the search procedures. Otherwise the field is cleared again by certain
// TODO:       *tif implementations */
// TODO:    if (defaultName != NULL)
// TODO:    {
// TODO:       Widget nameField = XmFileSelectionBoxGetChild(newFileSB, XmDIALOG_TEXT);
// TODO: #ifdef LESSTIF_VERSION
// TODO:       /* Workaround for Lesstif bug (0.93.94 and possibly other versions):
// TODO:          if a proportional font is used for the text field and text is
// TODO:          inserted while the dialog is managed, Lesstif crashes because it
// TODO:          tries to access a non-existing selection. By creating a temporary
// TODO:          dummy selection, the crash is avoided. */
// TODO:       XmTextFieldSetSelection(nameField, 0, 1, CurrentTime);
// TODO:       XmTextInsert(nameField, XmTextGetLastPosition(nameField), defaultName);
// TODO:       XmTextFieldSetSelection(nameField, 0, 0, CurrentTime);
// TODO: #else
// TODO:       XmTextInsert(nameField, XmTextGetLastPosition(nameField), defaultName);
// TODO: #endif
// TODO:    }
// TODO: 
// TODO:    while (!done_with_dialog)
// TODO:       XtAppProcessEvent(XtWidgetToApplicationContext(newFileSB), XtIMAll);
// TODO: 
// TODO:    if (SelectResult == GFN_OK)
// TODO:    {
// TODO:       /* See note in existing file routines about freeing the values
// TODO:          obtained in the following call */
// TODO:       XtVaGetValues(newFileSB, XmNdirSpec, &cFileString, XmNdirectory,
// TODO:                     &cDir, XmNpattern, &cPattern, NULL);
// TODO:       if (DefaultDirectory != NULL) NeStringFree(DefaultDirectory);
// TODO:       if (DefaultPattern != NULL) NeStringFree(DefaultPattern);
// TODO:       DefaultDirectory = cDir;
// TODO:       DefaultPattern = cPattern;
// TODO:       XmStringGetLtoR(cFileString, XmSTRING_DEFAULT_CHARSET, &fileString);
// TODO:       /* See note in existing file routines about Motif 2.x bug. */
// TODO: #ifdef VMS
// TODO:       /* VMS  won't return `/' as the 1st character of the full file spec.
// TODO:        `:' terminates the device name and is not allowed elsewhere */
// TODO:       if (strchr(fileString, ':') != NULL)
// TODO:       {
// TODO: #else
// TODO:       if (fileString[0] == '/')
// TODO:       {
// TODO: #endif /* VMS */
// TODO:          /* The directory name is already present in the file name or
// TODO:             the user entered a full path name. */
// TODO:          strcpy(filename, fileString);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          /* Concatenate the directory name and the file name */
// TODO:          XmStringGetLtoR(cDir, XmSTRING_DEFAULT_CHARSET, &dirString);
// TODO:          strcpy(filename, dirString);
// TODO:          strcat(filename, fileString);
// TODO:          XtFree(dirString);
// TODO:       }
// TODO:       NeStringFree(cFileString);
// TODO:       XtFree(fileString);
// TODO:    }
// TODO:    XtDestroyWidget(newFileSB);
// TODO:    return SelectResult;
// TODO: }

/*
** Return current default directory used by GetExistingFilename.
** Can return NULL if no default directory has been set (meaning
** use the application's current working directory) String must
** be freed by the caller using free.
*/
char* GetFileDialogDefaultDirectory()
{
   if (DefaultDirectory == NULL)
      return NULL;
   return NeNewString(DefaultDirectory);
}

/*
** Return current default match pattern used by GetExistingFilename.
** Can return NULL if no default pattern has been set (meaning use
** a pattern matching all files in the directory) String must be
** freed by the caller using XtFree.
*/
char* GetFileDialogDefaultPattern()
{
   if (DefaultPattern == NULL)
      return NULL;

   return NeNewString(DefaultPattern);
}

/*
** Set the current default directory to be used by GetExistingFilename.
** "dir" can be passed as NULL to clear the current default directory
** and use the application's working directory instead.
*/
void SetFileDialogDefaultDirectory(char* dir)
{
   if (DefaultDirectory != NULL)
      NeStringFree(DefaultDirectory);
   DefaultDirectory = dir==NULL ? NULL : NeNewString(dir);
}

/*
** Set the current default match pattern to be used by GetExistingFilename.
** "pattern" can be passed as NULL as the equivalent a pattern matching
** all files in the directory.
*/
void SetFileDialogDefaultPattern(char* pattern)
{
   if (DefaultPattern != NULL)
      NeStringFree(DefaultPattern);
   DefaultPattern = pattern==NULL ? NULL : NeNewString(pattern);
}

/*
** Turn on or off the text fiend in the GetExistingFilename file selection
** box, where users can enter the filename by typing.  This is redundant
** with typing in the list, and leads users who are new to nedit to miss
** the more powerful feature in favor of changing the focus and typing
** in the text field.
*/
void SetGetEFTextFieldRemoval(int state)
{
   RemoveRedundantTextField = state;
}

// TODO: /*
// TODO: ** createYesNoDialog, createErrorDialog, doYesNoDialog, doErrorDialog
// TODO: **
// TODO: ** Error Messages and question dialogs to be used with the file selection
// TODO: ** box.  Due to a crash bug in Motif 1.1.1 thru (at least) 1.1.5
// TODO: ** getfiles can not use DialogF.  According to OSF, there is an error
// TODO: ** in the creation of pushButtonGadgets involving the creation and
// TODO: ** destruction of some sort of temporary object.  These routines create
// TODO: ** the dialogs along with the file selection dialog and manage them
// TODO: ** to display messages.  This somehow avoids the problem
// TODO: */
// TODO: static void createYesNoDialog(Widget parent)
// TODO: {
// TODO:    NeString  buttonString;	      /* compound string for dialog buttons */
// TODO:    int       n;                      /* number of arguments               */
// TODO:    Arg       args[MAX_ARGS];	      /* arg list                          */
// TODO: 
// TODO:    n = 0;
// TODO:    XtSetArg(args[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
// TODO:    n++;
// TODO:    XtSetArg(args[n], XmNtitle, " ");
// TODO:    n++;
// TODO:    YesNoDialog = CreateQuestionDialog(parent, "yesNo", args, n);
// TODO:    XtAddCallback(YesNoDialog, XmNokCallback, (XtCallbackProc)yesNoOKCB, NULL);
// TODO:    XtAddCallback(YesNoDialog, XmNcancelCallback,
// TODO:                  (XtCallbackProc)yesNoCancelCB, NULL);
// TODO:    XtUnmanageChild(XmMessageBoxGetChild(YesNoDialog, XmDIALOG_HELP_BUTTON));
// TODO:    buttonString = NeNewString("Yes");
// TODO:    SET_ONE_RSRC(YesNoDialog, XmNokLabelString, buttonString);
// TODO:    NeStringFree(buttonString);
// TODO:    buttonString = NeNewString("No");
// TODO:    SET_ONE_RSRC(YesNoDialog, XmNcancelLabelString, buttonString);
// TODO:    NeStringFree(buttonString);
// TODO: }
// TODO: 
// TODO: static void createErrorDialog(Widget parent)
// TODO: {
// TODO:    NeString  buttonString;	      /* compound string for dialog button */
// TODO:    int       n;                      /* number of arguments               */
// TODO:    Arg       args[MAX_ARGS];	      /* arg list                          */
// TODO: 
// TODO:    n = 0;
// TODO:    XtSetArg(args[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
// TODO:    n++;
// TODO:    XtSetArg(args[n], XmNtitle, " ");
// TODO:    n++;
// TODO:    ErrorDialog = CreateErrorDialog(parent, "error", args, n);
// TODO:    XtAddCallback(ErrorDialog, XmNcancelCallback, (XtCallbackProc)errorOKCB,
// TODO:                  NULL);
// TODO:    XtUnmanageChild(XmMessageBoxGetChild(ErrorDialog, XmDIALOG_OK_BUTTON));
// TODO:    XtUnmanageChild(XmMessageBoxGetChild(ErrorDialog, XmDIALOG_HELP_BUTTON));
// TODO:    buttonString = XmStringCreateLtoR("OK", XmSTRING_DEFAULT_CHARSET);
// TODO:    XtVaSetValues(ErrorDialog, XmNcancelLabelString, buttonString, NULL);
// TODO:    XtVaSetValues(XmMessageBoxGetChild(ErrorDialog, XmDIALOG_CANCEL_BUTTON),
// TODO:                  XmNmarginWidth, BUTTON_WIDTH_MARGIN,
// TODO:                  NULL);
// TODO:    NeStringFree(buttonString);
// TODO: }
// TODO: 
// TODO: static int doYesNoDialog(const char* filename)
// TODO: {
// TODO:    char string[255];
// TODO:    NeString mString;
// TODO: 
// TODO:    YesNoResult = ynNone;
// TODO: 
// TODO:    sprintf(string, "File %s already exists,\nOk to overwrite?", filename);
// TODO:    mString = XmStringCreateLtoR(string, XmSTRING_DEFAULT_CHARSET);
// TODO: 
// TODO:    SET_ONE_RSRC(YesNoDialog, XmNmessageString, mString);
// TODO:    NeStringFree(mString);
// TODO:    ManageDialogCenteredOnPointer(YesNoDialog);
// TODO: 
// TODO:    while (YesNoResult == ynNone)
// TODO:       XtAppProcessEvent(XtWidgetToApplicationContext(YesNoDialog), XtIMAll);
// TODO: 
// TODO:    XtUnmanageChild(YesNoDialog);
// TODO: 
// TODO:    /* Nasty motif bug here, patched around by waiting for a ReparentNotify
// TODO:       event (with timeout) before allowing file selection dialog to pop
// TODO:       down.  If this routine returns too quickly, and the file selection
// TODO:       dialog (and thereby, this dialog as well) are destroyed while X
// TODO:       is still sorting through the events generated by the pop-down,
// TODO:       something bad happens and we get a crash */
// TODO:    if (YesNoResult == ynYes)
// TODO:       PopDownBugPatch(YesNoDialog);
// TODO: 
// TODO:    return YesNoResult == ynYes;
// TODO: }
// TODO: 
// TODO: static void doErrorDialog(const char* errorString, const char* filename)
// TODO: {
// TODO:    char string[255];
// TODO:    NeString mString;
// TODO: 
// TODO:    ErrorDone = false;
// TODO: 
// TODO:    sprintf(string, errorString, filename);
// TODO:    mString = XmStringCreateLtoR(string, XmSTRING_DEFAULT_CHARSET);
// TODO: 
// TODO:    SET_ONE_RSRC(ErrorDialog, XmNmessageString, mString);
// TODO:    NeStringFree(mString);
// TODO:    ManageDialogCenteredOnPointer(ErrorDialog);
// TODO: 
// TODO:    while (!ErrorDone)
// TODO:       XtAppProcessEvent(XtWidgetToApplicationContext(ErrorDialog), XtIMAll);
// TODO: 
// TODO:    XtUnmanageChild(ErrorDialog);
// TODO: }
// TODO: 
// TODO: static void newFileOKCB(Widget	w, bool* client_data,
// TODO:                         XmFileSelectionBoxCallbackStruct* call_data)
// TODO: 
// TODO: {
// TODO:    char* filename;                   /* name of chosen file             */
// TODO:    int  fd;                          /* file descriptor                 */
// TODO:    int  length;		      /* length of file name		 */
// TODO:    int  response;		      /* response to dialog		 */
// TODO:    struct stat buf;		      /* status from fstat		 */
// TODO: 
// TODO:    XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET, &filename);
// TODO:    SelectResult = GFN_OK;
// TODO:    length = strlen(filename);
// TODO:    if (length == 0 || filename[length-1] == '/')
// TODO:    {
// TODO:       doErrorDialog("Please supply a name for the file", NULL);
// TODO:       XtFree(filename);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO: #ifdef VMS
// TODO:    if (strchr(filename,';') && (fd = open(filename, O_RDONLY, 0)) != -1)
// TODO:    {
// TODO: #else  /* not VMS*/
// TODO:    if ((fd = open(filename, O_RDONLY, 0)) != -1)       /* exists */
// TODO:    {
// TODO: #endif /*VMS*/
// TODO:       fstat(fd, &buf);
// TODO:       close(fd);
// TODO:       if (buf.st_mode & S_IFDIR)
// TODO:       {
// TODO:          doErrorDialog("Error: %s is a directory", filename);
// TODO:          XtFree(filename);
// TODO:          return;
// TODO:       }
// TODO:       response = doYesNoDialog(filename);
// TODO: #ifdef VMS
// TODO:       if (response)
// TODO:       {
// TODO:          if (access(filename, 2) != 0)   /* have write/delete access? */
// TODO:          {
// TODO:             doErrorDialog("Error: can't overwrite %s ", filename);
// TODO:             XtFree(filename);
// TODO:             return;
// TODO:          }
// TODO:       }
// TODO:       else
// TODO:       {
// TODO: #else
// TODO:       if (!response)
// TODO:       {
// TODO: #endif /*VMS*/
// TODO:          return;
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if ((fd = creat(filename, PERMS)) == -1)
// TODO:       {
// TODO:          doErrorDialog("Error: can't create %s ", filename);
// TODO:          XtFree(filename);
// TODO:          return;
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          close(fd);
// TODO:          remove(filename);
// TODO:       }
// TODO:    }
// TODO:    XtFree(filename);
// TODO:    *client_data = true;			    /* done with dialog */
// TODO: }
// TODO: 
// TODO: 
// TODO: static void newFileCancelCB(Widget w, bool* client_data, caddr_t call_data)
// TODO: {
// TODO:    SelectResult = GFN_CANCEL;
// TODO:    *client_data = true;
// TODO: }
// TODO: 
// TODO: static void newHelpCB(Widget w, Widget helpPanel, caddr_t call_data)
// TODO: {
// TODO:    ManageDialogCenteredOnPointer(helpPanel);
// TODO: }
// TODO: 
// TODO: static void existOkCB(Widget w, bool* client_data,
// TODO:                       XmFileSelectionBoxCallbackStruct* call_data)
// TODO: {
// TODO:    char* filename;                   /* name of chosen file             */
// TODO:    int  fd;                          /* file descriptor                 */
// TODO:    int  length;		      /* length of file name		 */
// TODO: 
// TODO:    XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET, &filename);
// TODO:    SelectResult = GFN_OK;
// TODO:    length = strlen(filename);
// TODO:    if (length == 0 || filename[length-1] == '/')
// TODO:    {
// TODO:       doErrorDialog("Please select a file to open", NULL);
// TODO:       XtFree(filename);
// TODO:       return;
// TODO:    }
// TODO:    else    if ((fd = open(filename, O_RDONLY,0))  == -1)
// TODO:    {
// TODO:       doErrorDialog("Error: can't open %s ", filename);
// TODO:       XtFree(filename);
// TODO:       return;
// TODO:    }
// TODO:    else
// TODO:       close(fd);
// TODO:    XtFree(filename);
// TODO: 
// TODO:    *client_data = true;		/* done with dialog		*/
// TODO: }
// TODO: 
// TODO: 
// TODO: static void existCancelCB(Widget w, bool* client_data, caddr_t call_data)
// TODO: {
// TODO:    SelectResult = GFN_CANCEL;
// TODO:    *client_data = true;		/* done with dialog		*/
// TODO: }
// TODO: 
// TODO: static void yesNoOKCB(Widget w, caddr_t client_data, caddr_t call_data)
// TODO: {
// TODO:    YesNoResult = ynYes;
// TODO: }
// TODO: 
// TODO: static void existHelpCB(Widget w, Widget helpPanel, caddr_t call_data)
// TODO: {
// TODO:    ManageDialogCenteredOnPointer(helpPanel);
// TODO: }
// TODO: 
// TODO: static void errorOKCB(Widget w, caddr_t client_data, caddr_t call_data)
// TODO: {
// TODO:    ErrorDone = true;
// TODO: }
// TODO: 
// TODO: static void yesNoCancelCB(Widget w, caddr_t client_data, caddr_t call_data)
// TODO: {
// TODO:    YesNoResult = ynNo;
// TODO: }
// TODO: 
// TODO: static Widget createPanelHelp(Widget parent, const char* helpText, const char* title)
// TODO: {
// TODO:    Arg al[20];
// TODO:    int ac;
// TODO:    Widget form, text, button;
// TODO:    NeString st1;
// TODO: 
// TODO:    ac = 0;
// TODO:    form = CreateFormDialog(parent, "helpForm", al, ac);
// TODO: 
// TODO:    ac = 0;
// TODO:    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_NONE);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNlabelString, st1=XmStringCreateLtoR("OK",
// TODO:                                         XmSTRING_DEFAULT_CHARSET));
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNmarginWidth, BUTTON_WIDTH_MARGIN);
// TODO:    ac++;
// TODO:    button = XmCreatePushButtonGadget(form, "ok", al, ac);
// TODO:    XtAddCallback(button, XmNactivateCallback, (XtCallbackProc)helpDismissCB,
// TODO:                  (char*)form);
// TODO:    NeStringFree(st1);
// TODO:    XtManageChild(button);
// TODO:    SET_ONE_RSRC(form, XmNdefaultButton, button);
// TODO: 
// TODO:    ac = 0;
// TODO:    XtSetArg(al[ac], XmNrows, 15);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNcolumns, 60);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNresizeHeight, false);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNtraversalOn, false);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNwordWrap, true);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNscrollHorizontal, false);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNeditMode, XmMULTI_LINE_EDIT);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNeditable, false);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNvalue, helpText);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_FORM);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNleftAttachment, XmATTACH_FORM);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_WIDGET);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNrightAttachment, XmATTACH_FORM);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNbottomWidget, button);
// TODO:    ac++;
// TODO:    text = XmCreateScrolledText(form, "helpText", al, ac);
// TODO:    AddMouseWheelSupport(text);
// TODO:    XtManageChild(text);
// TODO: 
// TODO:    SET_ONE_RSRC(XtParent(form), XmNtitle, title);
// TODO: 
// TODO:    return form;
// TODO: }
// TODO: 
// TODO: static void helpDismissCB(Widget w, Widget helpPanel, caddr_t call_data)
// TODO: {
// TODO:    XtUnmanageChild(helpPanel);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Add ability for user to type filenames to a list widget
// TODO: */
// TODO: static void makeListTypeable(Widget listW)
// TODO: {
// TODO:    XtAddEventHandler(listW, KeyPressMask, false, listCharEH, NULL);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Action procedure for processing characters typed in a list, finds the
// TODO: ** first item matching the characters typed so far.
// TODO: */
// TODO: static int nKeystrokes = 0; /* Global key stroke history counter */
// TODO: static void listCharEH(Widget w, XtPointer callData, XEvent* event,
// TODO:                        bool* continueDispatch)
// TODO: {
// TODO:    char charString[5], c, *itemString;
// TODO:    int nChars, nItems, i, cmp, selectPos, topPos, nVisible;
// TODO:    NeString* items;
// TODO:    KeySym kSym;
// TODO:    char name[MAXPATHLEN], path[MAXPATHLEN];
// TODO:    static char keystrokes[MAX_LIST_KEYSTROKES];
// TODO:    static Time lastKeyTime = 0;
// TODO: 
// TODO:    /* Get the ascii character code represented by the event */
// TODO:    nChars = XLookupString((XKeyEvent*)event, charString, sizeof(charString),
// TODO:                           &kSym, NULL);
// TODO:    c = charString[0];
// TODO: 
// TODO:    /* Process selected control keys, but otherwise ignore the keystroke
// TODO:       if it isn't a single printable ascii character */
// TODO:    *continueDispatch = false;
// TODO:    if (kSym==XK_BackSpace || kSym==XK_Delete)
// TODO:    {
// TODO:       nKeystrokes = nKeystrokes > 0 ? nKeystrokes-1 : 0;
// TODO:       return;
// TODO:    }
// TODO:    else if (kSym==XK_Clear || kSym==XK_Cancel || kSym==XK_Break)
// TODO:    {
// TODO:       nKeystrokes = 0;
// TODO:       return;
// TODO:    }
// TODO:    else if (nChars!=1 || c<0x021 || c>0x07e)
// TODO:    {
// TODO:       *continueDispatch = true;
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Throw out keystrokes and start keystroke accumulation over from
// TODO:       scratch if user waits more than MAX_LIST_KESTROKE_WAIT milliseconds */
// TODO:    if (((XKeyEvent*)event)->time - lastKeyTime > MAX_LIST_KESTROKE_WAIT)
// TODO:       nKeystrokes = 0;
// TODO:    lastKeyTime = ((XKeyEvent*)event)->time;
// TODO: 
// TODO:    /* Accumulate the current keystroke, just beep if there are too many */
// TODO:    if (nKeystrokes >= MAX_LIST_KEYSTROKES)
// TODO:       XBell(XtDisplay(w), 0);
// TODO:    else
// TODO: #ifdef VMS
// TODO:       keystrokes[nKeystrokes++] = toupper(c);
// TODO: #else
// TODO:       keystrokes[nKeystrokes++] = c;
// TODO: #endif
// TODO: 
// TODO:    /* Get the items (filenames) in the list widget */
// TODO:    XtVaGetValues(w, XmNitems, &items, XmNitemCount, &nItems, NULL);
// TODO: 
// TODO:    /* compare them with the accumulated user keystrokes & decide the
// TODO:       appropriate line in the list widget to select */
// TODO:    selectPos = 0;
// TODO:    for (i=0; i<nItems; i++)
// TODO:    {
// TODO:       XmStringGetLtoR(items[i], XmSTRING_DEFAULT_CHARSET, &itemString);
// TODO:       if (ParseFilename(itemString, name, path) != 0)
// TODO:       {
// TODO:          XtFree(itemString);
// TODO:          return;
// TODO:       }
// TODO:       XtFree(itemString);
// TODO:       cmp = strncmp(name, keystrokes, nKeystrokes);
// TODO:       if (cmp == 0)
// TODO:       {
// TODO:          selectPos = i+1;
// TODO:          break;
// TODO:       }
// TODO:       else if (cmp > 0)
// TODO:       {
// TODO:          selectPos = i;
// TODO:          break;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* Make the selection, and make sure it will be visible */
// TODO:    XmListSelectPos(w, selectPos, true);
// TODO:    if (selectPos == 0) /* XmListSelectPos curiously returns 0 for last item */
// TODO:       selectPos = nItems + 1;
// TODO:    XtVaGetValues(w, XmNtopItemPosition, &topPos,
// TODO:                  XmNvisibleItemCount, &nVisible, NULL);
// TODO:    if (selectPos < topPos)
// TODO:       XmListSetPos(w, selectPos-2 > 1 ? selectPos-2 : 1);
// TODO:    else if (selectPos > topPos+nVisible-1)
// TODO:       XmListSetBottomPos(w, selectPos+2 <= nItems ? selectPos+2 : 0);
// TODO:    /* For LessTif 0.89.9. Obsolete now? */
// TODO:    XmListSelectPos(w, selectPos, true);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Replacement directory and file search procedures for the file selection
// TODO: ** box to re-sort the items in a standard order.  This is a patch, and not
// TODO: ** a very good one, for the problem that in some Motif versions, the directory
// TODO: ** list is sorted differently, such that typing of filenames fails because
// TODO: ** it expects strcmp alphabetical order, as opposed to strcasecmp.  Most
// TODO: ** users prefer the old ordering, which is what this enforces, but if
// TODO: ** ifdefs can be found that will correctly predict the ordering and adjust
// TODO: ** listCharEH above, instead of resorting to re-sorting, it should be done.
// TODO: ** This obviously wastes valuable time as the selection box is popping up
// TODO: ** and should be removed.  These routines also leak memory like a seive,
// TODO: ** because Motif's inconsistent treatment of memory in list widgets does
// TODO: ** not allow us to free lists that we pass in, and most Motif versions
// TODO: ** don't clean it up properly.
// TODO: */
// TODO: static void replacementDirSearchProc(Widget w, XtPointer searchData)
// TODO: {
// TODO:    bool updated;
// TODO: 
// TODO:    /* Call the original search procedure to do the actual search */
// TODO:    (*OrigDirSearchProc)(w, searchData);
// TODO:    /* Refreshing a list clears the keystroke history, even if no update. */
// TODO:    nKeystrokes = 0;
// TODO:    XtVaGetValues(w, XmNlistUpdated, &updated, NULL);
// TODO:    if (!updated)
// TODO:       return;
// TODO: 
// TODO:    /* Sort the items in the list */
// TODO:    sortWidgetList(XmFileSelectionBoxGetChild(w, XmDIALOG_DIR_LIST));
// TODO: }
// TODO: 
// TODO: static void replacementFileSearchProc(Widget w, XtPointer searchData)
// TODO: {
// TODO:    bool updated;
// TODO: 
// TODO:    /* Call the original search procedure to do the actual search */
// TODO:    (*OrigFileSearchProc)(w, searchData);
// TODO:    /* Refreshing a list clears the keystroke history, even if no update. */
// TODO:    nKeystrokes = 0;
// TODO:    XtVaGetValues(w, XmNlistUpdated, &updated, NULL);
// TODO:    if (!updated)
// TODO:       return;
// TODO: 
// TODO:    /* Sort the items in the list */
// TODO:    sortWidgetList(XmFileSelectionBoxGetChild(w, XmDIALOG_LIST));
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Sort the items in a list widget "listWidget"
// TODO: */
// TODO: static void sortWidgetList(Widget listWidget)
// TODO: {
// TODO:    NeString* items, *sortedItems;
// TODO:    int nItems, i;
// TODO: 
// TODO:    /* OpenMotif 2.3 will crash if we try to replace the items, when they
// TODO:       are selected. This function is only called when we refresh the
// TODO:       contents anyway. */
// TODO:    XmListDeselectAllItems(listWidget);
// TODO:    XtVaGetValues(listWidget, XmNitems, &items, XmNitemCount, &nItems, NULL);
// TODO:    sortedItems = (NeString*)malloc__(sizeof(NeString) * nItems);
// TODO:    for (i=0; i<nItems; i++)
// TODO:       sortedItems[i] = XmStringCopy(items[i]);
// TODO:    qsort(sortedItems, nItems, sizeof(NeString), compareXmStrings);
// TODO:    XmListReplaceItemsPos(listWidget, sortedItems, nItems, 1);
// TODO:    for (i=0; i<nItems; i++)
// TODO:       NeStringFree(sortedItems[i]);
// TODO:    XtFree((char*)sortedItems);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Compare procedure for qsort for sorting a list of XmStrings
// TODO: */
// TODO: static int compareXmStrings(const void* string1, const void* string2)
// TODO: {
// TODO:    char* s1, *s2;
// TODO:    int result;
// TODO: 
// TODO:    XmStringGetLtoR(*(NeString*)string1, XmSTRING_DEFAULT_CHARSET, &s1);
// TODO:    XmStringGetLtoR(*(NeString*)string2, XmSTRING_DEFAULT_CHARSET, &s2);
// TODO:    result = strcmp(s1, s2);
// TODO:    XtFree(s1);
// TODO:    XtFree(s2);
// TODO:    return result;
// TODO: }
