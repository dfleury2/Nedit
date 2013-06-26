static const char CVSID[] = "$Id: printUtils.c,v 1.25 2004/08/01 10:06:12 yooden Exp $";
/*******************************************************************************
*									       *
* printUtils.c -- Nirvana library Printer Menu	& Printing Routines   	       *
*									       *
* Copyright (C) 1999 Mark Edel						       *
*									       *
* This is free software; you can redistribute it and/or modify it under the    *
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
*                                        				       *
* April 20, 1992							       *
*									       *
* Written by Arnulfo Zepeda-Navratil				               *
*            Centro de Investigacion y Estudio Avanzados ( CINVESTAV )         *
*            Dept. Fisica - Mexico                                             *
*            BITNET: ZEPEDA@CINVESMX                                           *
*									       *
* Modified by Donna Reid and Joy Kyriakopulos 4/8/93 - VMS port		       *
*									       *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "printUtils.h"
#include "DialogF.h"
#include "misc.h"
#include "prefFile.h"
#include "utils.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#ifdef USE_DIRENT
#include <dirent.h>
#else
#endif /* USE_DIRENT */
#ifndef WIN32
#include <sys/dir.h>
#include <sys/param.h>
#else
#include <io.h>
#endif
#include <sys/stat.h>

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

// Separator between directory references in PATH environmental variable
#ifdef WIN32
#define SEPARATOR ';'
#else
#define SEPARATOR ':'
#endif

/* Number of extra pixels down to place a label even with a text widget */
#define LABEL_TEXT_DIFF 6

/* Maximum text string lengths */
#define MAX_OPT_STR 20
#define MAX_QUEUE_STR 60
#define MAX_INT_STR 13
#define MAX_HOST_STR 100
#define MAX_PCMD_STR 100
#define MAX_NAME_STR 100
#define MAX_CMD_STR 256
#define VMS_MAX_JOB_NAME_STR 39

#define N_PRINT_PREFS 7 /* must agree with number of preferences below */
struct printPrefDescrip
{
   PrefDescripRec printCommand;
   PrefDescripRec copiesOption;
   PrefDescripRec queueOption;
   PrefDescripRec nameOption;
   PrefDescripRec hostOption;
   PrefDescripRec defaultQueue;
   PrefDescripRec defaultHost;
};

// TODO: /* Function Prototypes */
// TODO: static Fl_Widget* createForm(Fl_Widget* parent);
// TODO: static void allowOnlyNumInput(Fl_Widget* widget, caddr_t client_data,
// TODO:                               XmTextVerifyCallbackStruct* call_data);
// TODO: static void noSpaceOrPunct(Fl_Widget* widget, caddr_t client_data,
// TODO:                            XmTextVerifyCallbackStruct* call_data);
// TODO: static void updatePrintCmd(Fl_Widget* w, caddr_t client_data, caddr_t call_data);
// TODO: static void printCmdModified(Fl_Widget* w, caddr_t client_data, caddr_t call_data);
// TODO: static void printButtonCB(Fl_Widget* widget, caddr_t client_data, caddr_t call_data);
// TODO: static void cancelButtonCB(Fl_Widget* widget, caddr_t client_data, caddr_t call_data);
// TODO: static void setQueueLabelText();
static int fileInDir(const char* filename, const char* dirpath, unsigned short mode_flags);
static bool fileInPath(const char* filename, unsigned short mode_flags);
static bool flprPresent();
// TODO: #ifdef USE_LPR_PRINT_CMD
// TODO: static void getLprQueueDefault(char* defqueue);
// TODO: #endif
#ifndef USE_LPR_PRINT_CMD
static void getLpQueueDefault(char* defqueue);
#endif
// TODO: static void setHostLabelText();
// TODO: #ifdef VMS
// TODO: static void getVmsQueueDefault(char* defqueue);
// TODO: #else
static void getFlprHostDefault(char* defhost);
static void getFlprQueueDefault(char* defqueue);
// TODO: #endif

/* Module Global Variables */
static bool  DoneWithDialog;
static bool	PreferencesLoaded = false;
static Fl_Widget*	Form;
static Fl_Widget*	Label2;
static Fl_Widget*	Label3;
static Fl_Widget*	Text1;
static Fl_Widget*	Text2;
static Fl_Widget*	Text3;
static Fl_Widget*	Text4;
static const char* PrintFileName;
static const char* PrintJobName;
static char PrintCommand[MAX_PCMD_STR];	/* print command string */
static char CopiesOption[MAX_OPT_STR];	/* # of copies argument string */
static char QueueOption[MAX_OPT_STR];	/* queue name argument string */
static char NameOption[MAX_OPT_STR];	/* print job name argument string */
static char HostOption[MAX_OPT_STR];	/* host name argument string */
static char DefaultQueue[MAX_QUEUE_STR];/* default print queue */
static char DefaultHost[MAX_HOST_STR];	/* default host name */
static char Copies[MAX_INT_STR] = "";	/* # of copies last entered by user */
static char Queue[MAX_QUEUE_STR] = "";	/* queue name last entered by user */
static char Host[MAX_HOST_STR] = "";	/* host name last entered by user */
static char CmdText[MAX_CMD_STR] = "";	/* print command last entered by user */
static int  CmdFieldModified = false;	/* user last changed the print command
					   field, so don't trust the rest */


static struct printPrefDescrip PrintPrefDescrip =
{
   { "printCommand", PREF_STRING, NULL, PrintCommand, (void*)MAX_PCMD_STR, false },
   { "printCopiesOption", PREF_STRING, NULL, CopiesOption, (void*)MAX_OPT_STR, false },
   { "printQueueOption", PREF_STRING, NULL, QueueOption, (void*)MAX_OPT_STR, false },
   { "printNameOption", PREF_STRING, NULL, NameOption, (void*)MAX_OPT_STR, false },
   { "printHostOption", PREF_STRING, NULL, HostOption, (void*)MAX_OPT_STR, false },
   { "printDefaultQueue", PREF_STRING, NULL, DefaultQueue, (void*)MAX_QUEUE_STR, false },
   { "printDefaultHost", PREF_STRING, NULL, DefaultHost, (void*)MAX_HOST_STR, false }
};

// TODO: /*
// TODO: ** PrintFile(Fl_Widget* parent, char *printFile, char *jobName);
// TODO: **
// TODO: ** function to put up an application-modal style Print Panel dialog
// TODO: ** box.
// TODO: **
// TODO: **	parent		Parent widget for displaying dialog
// TODO: **	printFile	File to print (assumed to be a temporary file
// TODO: **			and not revealed to the user)
// TODO: **	jobName		Title for the print banner page
// TODO: */
// TODO: #ifdef VMS
// TODO: void PrintFile(Fl_Widget* parent, const char* printFile, const char* jobName, int delete)
// TODO: #else
// TODO: void PrintFile(Fl_Widget* parent, const char* printFile, const char* jobName)
// TODO: #endif /*VMS*/
// TODO: {
// TODO:    /* In case the program hasn't called LoadPrintPreferences, set up the
// TODO:       default values for the print preferences */
// TODO:    if (!PreferencesLoaded)
// TODO:       LoadPrintPreferences(NULL, "", "", true);
// TODO: 
// TODO:    /* Make the PrintFile information available to the callback routines */
// TODO:    PrintFileName = printFile;
// TODO:    PrintJobName = jobName;
// TODO: #ifdef VMS
// TODO:    DeleteFile = delete;
// TODO: #endif /*VMS*/
// TODO: 
// TODO:    /* Create and display the print dialog */
// TODO:    DoneWithDialog = false;
// TODO:    Form = createForm(parent);
// TODO:    ManageDialogCenteredOnPointer(Form);
// TODO: 
// TODO:    /* Process events until the user is done with the print dialog */
// TODO:    while (!DoneWithDialog)
// TODO:       XtAppProcessEvent(XtWidgetToApplicationContext(Form), XtIMAll);
// TODO: 
// TODO:    /* Destroy the dialog.  Print dialogs are not preserved across calls
// TODO:       to PrintFile so that it may be called with different parents and
// TODO:       to generally simplify the call (this, of course, makes it slower) */
// TODO:    XtDestroyWidget(Form);
// TODO: }
// TODO: 
/*
** LoadPrintPreferences
**
** Read an X database to obtain print dialog preferences.
**
**	prefDB		X database potentially containing print preferences
**	appName		Application name which can be used to qualify
**			resource names for database lookup.
**	appClass	Application class which can be used to qualify
**			resource names for database lookup.
**	lookForFlpr	Check if the flpr print command is installed
**			and use that for the default if it's found.
**			(flpr is a Fermilab utility for printing on
**			arbitrary systems that support the lpr protocol)
*/
void LoadPrintPreferences(Ne_Database* prefDB, const char* appName, int lookForFlpr)
{
   static char defaultQueue[MAX_QUEUE_STR], defaultHost[MAX_HOST_STR];

   /* check if flpr is installed, and otherwise choose appropriate
      printer per system type */
   if (lookForFlpr && flprPresent())
   {
      getFlprQueueDefault(defaultQueue);
      getFlprHostDefault(defaultHost);
      PrintPrefDescrip.printCommand.defaultString = "flpr";
      PrintPrefDescrip.copiesOption.defaultString = "";
      PrintPrefDescrip.queueOption.defaultString = "-q";
      PrintPrefDescrip.nameOption.defaultString = "-j ";
      PrintPrefDescrip.hostOption.defaultString = "-h";
      PrintPrefDescrip.defaultQueue.defaultString = defaultQueue;
      PrintPrefDescrip.defaultHost.defaultString = defaultHost;
   }
   else
   {
#ifdef USE_LPR_PRINT_CMD
      getLprQueueDefault(defaultQueue);
      PrintPrefDescrip.printCommand.defaultString = "lpr";
      PrintPrefDescrip.copiesOption.defaultString = "-# ";
      PrintPrefDescrip.queueOption.defaultString = "-P ";
      PrintPrefDescrip.nameOption.defaultString = "-J ";
      PrintPrefDescrip.hostOption.defaultString = "";
      PrintPrefDescrip.defaultQueue.defaultString = defaultQueue;
      PrintPrefDescrip.defaultHost.defaultString = "";
#else
      getLpQueueDefault(defaultQueue);
      PrintPrefDescrip.printCommand.defaultString = "lp"; /* was lp -c */
      PrintPrefDescrip.copiesOption.defaultString = "-n";
      PrintPrefDescrip.queueOption.defaultString = "-d";
      PrintPrefDescrip.nameOption.defaultString = "-t";
      PrintPrefDescrip.hostOption.defaultString = "";
      PrintPrefDescrip.defaultQueue.defaultString = defaultQueue;
      PrintPrefDescrip.defaultHost.defaultString = "";
#endif
   }

   /* Read in the preferences from the X database using the mechanism from
      prefFile.c (this allows LoadPrintPreferences to work before any
      widgets are created, which is more convenient than XtGetApplication-
      Resources for applications which have no main window) */
   RestorePreferences(NULL, prefDB, appName, (PrefDescripRec*)&PrintPrefDescrip, N_PRINT_PREFS);

   PreferencesLoaded = true;
}

// TODO: static Fl_Widget* createForm(Fl_Widget* parent)
// TODO: {
// TODO:    Fl_Widget* form, printOk, printCancel, label1, separator;
// TODO:    Fl_Widget* topWidget = NULL;
// TODO:    NeString st0;
// TODO:    Arg args[65];
// TODO:    int argcnt;
// TODO:    Fl_Widget* bwidgetarray [30];
// TODO:    int bwidgetcnt = 0;
// TODO: 
// TODO:    /************************ FORM ***************************/
// TODO:    argcnt = 0;
// TODO:    XtSetArg(args[argcnt], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNdialogTitle, (st0=XmStringCreateLtoR(
// TODO:          "Print", XmSTRING_DEFAULT_CHARSET)));
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNautoUnmanage, false);
// TODO:    argcnt++;
// TODO:    form = CreateFormDialog(parent, "printForm", args, argcnt);
// TODO:    XtVaSetValues(form, XmNshadowThickness, 0, NULL);
// TODO: 
// TODO:    NeStringFree(st0);
// TODO: 
// TODO:    /*********************** LABEL 1 and TEXT BOX 1 *********************/
// TODO:    if (CopiesOption[0] != '\0')
// TODO:    {
// TODO:       argcnt = 0;
// TODO:       XtSetArg(args[argcnt], XmNlabelString, (st0=XmStringCreateLtoR(
// TODO:             "Number of copies (1)", XmSTRING_DEFAULT_CHARSET)));
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNmnemonic, 'N');
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopOffset, LABEL_TEXT_DIFF+5);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNleftOffset, 8);
// TODO:       argcnt++;
// TODO:       label1 = XmCreateLabelGadget(form, "label1", args, argcnt);
// TODO:       NeStringFree(st0);
// TODO:       bwidgetarray[bwidgetcnt] = label1;
// TODO:       bwidgetcnt++;
// TODO: 
// TODO:       argcnt = 0;
// TODO:       XtSetArg(args[argcnt], XmNshadowThickness, (short)2);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNcolumns, 3);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNrows, 1);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNvalue , Copies);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNmaxLength, 3);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopOffset, 5);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNleftWidget, label1);
// TODO:       argcnt++;
// TODO:       Text1 = XmCreateText(form, "text1", args, argcnt);
// TODO:       bwidgetarray[bwidgetcnt] = Text1;
// TODO:       bwidgetcnt++;
// TODO:       XtAddCallback(Text1, XmNmodifyVerifyCallback,
// TODO:                     (XtCallbackProc)allowOnlyNumInput, NULL);
// TODO:       XtAddCallback(Text1, XmNvalueChangedCallback,
// TODO:                     (XtCallbackProc)updatePrintCmd, NULL);
// TODO:       RemapDeleteKey(Text1);
// TODO:       topWidget = Text1;
// TODO:       XtVaSetValues(label1, XmNuserData, Text1, NULL); /* mnemonic procesing */
// TODO:    }
// TODO: 
// TODO:    /************************ LABEL 2 and TEXT 2 ************************/
// TODO:    if (QueueOption[0] != '\0')
// TODO:    {
// TODO:       argcnt = 0;
// TODO:       XtSetArg(args[argcnt], XmNlabelString, (st0=XmStringCreateLtoR(
// TODO:             "  ", XmSTRING_DEFAULT_CHARSET)));
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNmnemonic, 'Q');
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNrecomputeSize, true);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopAttachment,
// TODO:                topWidget==NULL?XmATTACH_FORM:XmATTACH_WIDGET);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopWidget, topWidget);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopOffset, LABEL_TEXT_DIFF+4);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNleftOffset, 8);
// TODO:       argcnt++;
// TODO:       Label2 = XmCreateLabelGadget(form, "label2", args, argcnt);
// TODO:       NeStringFree(st0);
// TODO:       bwidgetarray[bwidgetcnt] = Label2;
// TODO:       bwidgetcnt++;
// TODO:       setQueueLabelText();
// TODO: 
// TODO:       argcnt = 0;
// TODO:       XtSetArg(args[argcnt], XmNshadowThickness, (short)2);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNcolumns, (short)17);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNmaxLength, MAX_QUEUE_STR);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNvalue, Queue);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNleftWidget, Label2);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopAttachment,
// TODO:                topWidget==NULL?XmATTACH_FORM:XmATTACH_WIDGET);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopWidget, topWidget);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNrightOffset, 8);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopOffset, 4);
// TODO:       argcnt++;
// TODO:       Text2 = XmCreateText(form, "text2", args, argcnt);
// TODO:       XtAddCallback(Text2, XmNmodifyVerifyCallback,
// TODO:                     (XtCallbackProc)noSpaceOrPunct, NULL);
// TODO:       XtAddCallback(Text2, XmNvalueChangedCallback,
// TODO:                     (XtCallbackProc)updatePrintCmd, NULL);
// TODO:       bwidgetarray[bwidgetcnt] = Text2;
// TODO:       bwidgetcnt++;
// TODO:       RemapDeleteKey(Text2);
// TODO:       XtVaSetValues(Label2, XmNuserData, Text2, NULL); /* mnemonic procesing */
// TODO:       topWidget = Text2;
// TODO:    }
// TODO: 
// TODO:    /****************** LABEL 3 and TEXT 3 *********************/
// TODO:    if (HostOption[0] != '\0')
// TODO:    {
// TODO:       argcnt = 0;
// TODO:       XtSetArg(args[argcnt], XmNlabelString, (st0=XmStringCreateLtoR(
// TODO:             "  ", XmSTRING_DEFAULT_CHARSET)));
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNmnemonic, 'H');
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNrecomputeSize, true);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNvalue , "");
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopAttachment,
// TODO:                topWidget==NULL?XmATTACH_FORM:XmATTACH_WIDGET);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopWidget, topWidget);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNleftOffset, 8);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopOffset, LABEL_TEXT_DIFF+4);
// TODO:       argcnt++;
// TODO:       Label3 = XmCreateLabelGadget(form, "label3", args, argcnt);
// TODO:       NeStringFree(st0);
// TODO:       bwidgetarray[bwidgetcnt] = Label3;
// TODO:       bwidgetcnt++;
// TODO:       setHostLabelText();
// TODO: 
// TODO:       argcnt = 0;
// TODO:       XtSetArg(args[argcnt], XmNcolumns, 17);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNrows, 1);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNvalue, Host);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNmaxLength, MAX_HOST_STR);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNleftWidget, Label3);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopAttachment,
// TODO:                topWidget==NULL?XmATTACH_FORM:XmATTACH_WIDGET);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopWidget, topWidget);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNrightOffset, 8);
// TODO:       argcnt++;
// TODO:       XtSetArg(args[argcnt], XmNtopOffset, 4);
// TODO:       argcnt++;
// TODO:       Text3 = XmCreateText(form, "Text3", args, argcnt);
// TODO:       XtAddCallback(Text3, XmNmodifyVerifyCallback,
// TODO:                     (XtCallbackProc)noSpaceOrPunct, NULL);
// TODO:       XtAddCallback(Text3, XmNvalueChangedCallback,
// TODO:                     (XtCallbackProc)updatePrintCmd, NULL);
// TODO:       bwidgetarray[bwidgetcnt] = Text3;
// TODO:       bwidgetcnt++;
// TODO:       RemapDeleteKey(Text3);
// TODO:       XtVaSetValues(Label3, XmNuserData, Text3, NULL); /* mnemonic procesing */
// TODO:       topWidget = Text3;
// TODO:    }
// TODO: 
// TODO:    /************************** TEXT 4 ***************************/
// TODO:    argcnt = 0;
// TODO:    XtSetArg(args[argcnt], XmNvalue, CmdText);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNcolumns, 50);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNleftOffset, 8);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNtopOffset, 8);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNtopWidget, topWidget);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNrightOffset, 8);
// TODO:    argcnt++;
// TODO:    Text4 = XmCreateText(form, "Text4", args, argcnt);
// TODO:    XtAddCallback(Text4, XmNmodifyVerifyCallback,
// TODO:                  (XtCallbackProc)printCmdModified, NULL);
// TODO:    bwidgetarray[bwidgetcnt] = Text4;
// TODO:    bwidgetcnt++;
// TODO:    RemapDeleteKey(Text4);
// TODO:    topWidget = Text4;
// TODO:    if (!CmdFieldModified)
// TODO:       updatePrintCmd(NULL, NULL, NULL);
// TODO: 
// TODO:    /*********************** SEPARATOR **************************/
// TODO:    argcnt = 0;
// TODO:    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNtopOffset, 8);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNtopWidget, topWidget);
// TODO:    argcnt++;
// TODO:    separator = XmCreateSeparatorGadget(form, "separator", args, argcnt);
// TODO:    bwidgetarray[bwidgetcnt] = separator;
// TODO:    bwidgetcnt++;
// TODO:    topWidget = separator;
// TODO: 
// TODO:    /********************** CANCEL BUTTON *************************/
// TODO:    argcnt = 0;
// TODO:    XtSetArg(args[argcnt], XmNlabelString, (st0=XmStringCreateLtoR(
// TODO:          "Cancel", XmSTRING_DEFAULT_CHARSET)));
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_POSITION);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNleftPosition, 60);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNtopWidget, topWidget);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNtopOffset, 7);
// TODO:    argcnt++;
// TODO:    printCancel = XmCreatePushButton(form, "printCancel", args, argcnt);
// TODO:    NeStringFree(st0);
// TODO:    bwidgetarray[bwidgetcnt] = printCancel;
// TODO:    bwidgetcnt++;
// TODO:    XtAddCallback(printCancel, XmNactivateCallback,
// TODO:                  (XtCallbackProc)cancelButtonCB, NULL);
// TODO: 
// TODO:    /*********************** PRINT BUTTON **************************/
// TODO:    argcnt = 0;
// TODO:    XtSetArg(args[argcnt], XmNlabelString, (st0=XmStringCreateLtoR(
// TODO:          "Print", XmSTRING_DEFAULT_CHARSET)));
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNshowAsDefault, true);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_POSITION);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNrightPosition, 40);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNtopWidget, topWidget);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNtopOffset, 7);
// TODO:    argcnt++;
// TODO:    printOk = XmCreatePushButton(form, "printOk", args, argcnt);
// TODO:    NeStringFree(st0);
// TODO:    bwidgetarray[bwidgetcnt] = printOk;
// TODO:    bwidgetcnt++;
// TODO:    XtAddCallback(printOk, XmNactivateCallback,
// TODO:                  (XtCallbackProc)printButtonCB, NULL);
// TODO: 
// TODO:    argcnt = 0;
// TODO:    XtSetArg(args[argcnt], XmNcancelButton, printCancel);
// TODO:    argcnt++;
// TODO:    XtSetArg(args[argcnt], XmNdefaultButton, printOk);
// TODO:    argcnt++;
// TODO:    XtSetValues(form, args, argcnt);
// TODO: 
// TODO:    XtManageChildren(bwidgetarray, bwidgetcnt);
// TODO:    AddDialogMnemonicHandler(form, FALSE);
// TODO:    return form;
// TODO: }
// TODO: 
// TODO: static void setQueueLabelText()
// TODO: {
// TODO:    Arg args[15];
// TODO:    int	argcnt;
// TODO:    NeString	st0;
// TODO:    char        tmp_buf[MAX_QUEUE_STR+8];
// TODO: 
// TODO:    if (DefaultQueue[0] != '\0')
// TODO:       sprintf(tmp_buf, "Queue (%s)", DefaultQueue);
// TODO:    else
// TODO:       sprintf(tmp_buf, "Queue");
// TODO:    argcnt = 0;
// TODO:    XtSetArg(args[argcnt], XmNlabelString, (st0=XmStringCreateLtoR(
// TODO:          tmp_buf, XmSTRING_DEFAULT_CHARSET)));
// TODO:    argcnt++;
// TODO:    XtSetValues(Label2, args, argcnt);
// TODO:    NeStringFree(st0);
// TODO: }
// TODO: 
// TODO: static void setHostLabelText()
// TODO: {
// TODO:    Arg args[15];
// TODO:    int	argcnt;
// TODO:    NeString st0;
// TODO:    char tmp_buf[MAX_HOST_STR+7];
// TODO: 
// TODO:    if (strcmp(DefaultHost, ""))
// TODO:       sprintf(tmp_buf, "Host (%s)", DefaultHost);
// TODO:    else
// TODO:       sprintf(tmp_buf, "Host");
// TODO:    argcnt = 0;
// TODO:    XtSetArg(args[argcnt], XmNlabelString, (st0=XmStringCreateLtoR(
// TODO:          tmp_buf, XmSTRING_DEFAULT_CHARSET)));
// TODO:    argcnt++;
// TODO: 
// TODO:    XtSetValues(Label3, args, argcnt);
// TODO:    NeStringFree(st0);
// TODO: }
// TODO: 
// TODO: static void allowOnlyNumInput(Fl_Widget* widget, caddr_t client_data,
// TODO:                               XmTextVerifyCallbackStruct* call_data)
// TODO: {
// TODO:    int i, textInserted, nInserted;
// TODO: 
// TODO:    nInserted = call_data->text->length;
// TODO:    textInserted = (nInserted > 0);
// TODO:    if ((call_data->reason == XmCR_MODIFYING_TEXT_VALUE) && textInserted)
// TODO:    {
// TODO:       for (i=0; i<nInserted; i++)
// TODO:       {
// TODO:          if (!isdigit((unsigned char)call_data->text->ptr[i]))
// TODO:          {
// TODO:             call_data->doit = false;
// TODO:             return;
// TODO:          }
// TODO:       }
// TODO:    }
// TODO:    call_data->doit = true;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Prohibit a relatively random sampling of characters that will cause
// TODO: ** problems on command lines
// TODO: */
// TODO: static void noSpaceOrPunct(Fl_Widget* widget, caddr_t client_data,
// TODO:                            XmTextVerifyCallbackStruct* call_data)
// TODO: {
// TODO:    int i, j, textInserted, nInserted;
// TODO: #ifndef VMS
// TODO:    static char prohibited[] = " \t,;|<>()[]{}!@?";
// TODO: #else
// TODO:    static char prohibited[] = " \t,;|@+";
// TODO: #endif
// TODO: 
// TODO:    nInserted = call_data->text->length;
// TODO:    textInserted = (nInserted > 0);
// TODO:    if ((call_data->reason == XmCR_MODIFYING_TEXT_VALUE) && textInserted)
// TODO:    {
// TODO:       for (i=0; i<nInserted; i++)
// TODO:       {
// TODO:          for (j=0; j<(int)XtNumber(prohibited); j++)
// TODO:          {
// TODO:             if (call_data->text->ptr[i] == prohibited[j])
// TODO:             {
// TODO:                call_data->doit = false;
// TODO:                return;
// TODO:             }
// TODO:          }
// TODO:       }
// TODO:    }
// TODO:    call_data->doit = true;
// TODO: }
// TODO: 
// TODO: static void updatePrintCmd(Fl_Widget* w, caddr_t client_data, caddr_t call_data)
// TODO: {
// TODO:    char command[MAX_CMD_STR], copiesArg[MAX_OPT_STR+MAX_INT_STR];
// TODO:    char jobArg[MAX_NAME_STR], hostArg[MAX_OPT_STR+MAX_HOST_STR];
// TODO:    char queueArg[MAX_OPT_STR+MAX_QUEUE_STR];
// TODO:    char* str;
// TODO:    int nCopies;
// TODO: #ifdef VMS
// TODO:    char printJobName[VMS_MAX_JOB_NAME_STR+1];
// TODO: #endif /*VMS*/
// TODO: 
// TODO:    /* read each text field in the dialog and generate the corresponding
// TODO:       command argument */
// TODO:    if (CopiesOption[0] == '\0')
// TODO:    {
// TODO:       copiesArg[0] = '\0';
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       str = NeTextGetString(Text1);
// TODO:       if (str[0] == '\0')
// TODO:       {
// TODO:          copiesArg[0] = '\0';
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          if (sscanf(str, "%d", &nCopies) != 1)
// TODO:          {
// TODO:             copiesArg[0] = '\0';
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             sprintf(copiesArg, " %s%s", CopiesOption, str);
// TODO:          }
// TODO:       }
// TODO:       XtFree(str);
// TODO:    }
// TODO:    if (QueueOption[0] == '\0')
// TODO:    {
// TODO:       queueArg[0] = '\0';
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       str = NeTextGetString(Text2);
// TODO:       if (str[0] == '\0')
// TODO:          queueArg[0] = '\0';
// TODO:       else
// TODO:          sprintf(queueArg, " %s%s", QueueOption, str);
// TODO:       XtFree(str);
// TODO:    }
// TODO:    if (HostOption[0] == '\0')
// TODO:    {
// TODO:       hostArg[0] = '\0';
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       str = NeTextGetString(Text3);
// TODO:       if (str[0] == '\0')
// TODO:          hostArg[0] = '\0';
// TODO:       else
// TODO:          sprintf(hostArg, " %s%s", HostOption, str);
// TODO:       XtFree(str);
// TODO:    }
// TODO:    if (NameOption[0] == '\0')
// TODO:       jobArg[0] = '\0';
// TODO:    else
// TODO:    {
// TODO: #ifdef VMS
// TODO:       /* truncate job name on VMS systems or it will cause problems */
// TODO:       strncpy(printJobName,PrintJobName,VMS_MAX_JOB_NAME_STR);
// TODO:       printJobName[VMS_MAX_JOB_NAME_STR] = '\0';
// TODO:       sprintf(jobArg, " %s\"%s\"", NameOption, printJobName);
// TODO: #else
// TODO:       sprintf(jobArg, " %s\"%s\"", NameOption, PrintJobName);
// TODO: #endif
// TODO:    }
// TODO: 
// TODO:    /* Compose the command from the options determined above */
// TODO:    sprintf(command, "%s%s%s%s%s", PrintCommand, copiesArg,
// TODO:            queueArg, hostArg, jobArg);
// TODO: 
// TODO:    /* display it in the command text area */
// TODO:    NeTextSetString(Text4, command);
// TODO: 
// TODO:    /* Indicate that the command field was synthesized from the other fields,
// TODO:       so future dialog invocations can safely re-generate the command without
// TODO:       overwriting commands specifically entered by the user */
// TODO:    CmdFieldModified = false;
// TODO: }
// TODO: 
// TODO: static void printCmdModified(Fl_Widget* w, caddr_t client_data, caddr_t call_data)
// TODO: {
// TODO:    /* Indicate that the user has specifically modified the print command
// TODO:       and that this field should be left as is in subsequent dialogs */
// TODO:    CmdFieldModified = true;
// TODO: }
// TODO: 
// TODO: static void printButtonCB(Fl_Widget* widget, caddr_t client_data, caddr_t call_data)
// TODO: {
// TODO:    char* str, command[MAX_CMD_STR];
// TODO: #ifdef VMS
// TODO:    int spawn_sts;
// TODO:    int spawnFlags=CLI$M_NOCLISYM;
// TODO:    struct dsc$descriptor cmdDesc;
// TODO: 
// TODO:    /* get the print command from the command text area */
// TODO:    str = NeTextGetString(Text4);
// TODO: 
// TODO:    /* add the file name to the print command */
// TODO:    sprintf(command, "%s %s", str, PrintFileName);
// TODO: 
// TODO:    XtFree(str);
// TODO: 
// TODO:    /* append /DELETE to print command if requested */
// TODO:    if (DeleteFile)
// TODO:       strcat(command, "/DELETE");
// TODO: 
// TODO:    /* spawn the print command */
// TODO:    cmdDesc.dsc$w_length  = strlen(command);
// TODO:    cmdDesc.dsc$b_dtype   = DSC$K_DTYPE_T;
// TODO:    cmdDesc.dsc$b_class   = DSC$K_CLASS_S;
// TODO:    cmdDesc.dsc$a_pointer = command;
// TODO:    spawn_sts = lib$spawn(&cmdDesc,0,0,&spawnFlags,0,0,0,0,0,0,0,0);
// TODO: 
// TODO:    if (spawn_sts != SS$_NORMAL)
// TODO:    {
// TODO:       DialogF(DF_WARN, widget, 1, "Print Error",
// TODO:               "Unable to Print:\n%d - %s\n  spawnFlags = %d\n", "OK",
// TODO:               spawn_sts, strerror(EVMSERR, spawn_sts), spawnFlags);
// TODO:       return;
// TODO:    }
// TODO: #else
// TODO:    int nRead;
// TODO:    FILE* pipe;
// TODO:    char errorString[MAX_PRINT_ERROR_LENGTH], discarded[1024];
// TODO: 
// TODO:    /* get the print command from the command text area */
// TODO:    str = NeTextGetString(Text4);
// TODO: 
// TODO:    /* add the file name and output redirection to the print command */
// TODO:    sprintf(command, "cat %s | %s 2>&1", PrintFileName, str);
// TODO:    XtFree(str);
// TODO: 
// TODO:    /* Issue the print command using a popen call and recover error messages
// TODO:       from the output stream of the command. */
// TODO:    pipe = popen(command,"r");
// TODO:    if (pipe == NULL)
// TODO:    {
// TODO:       DialogF(DF_WARN, widget, 1, "Print Error", "Unable to Print:\n%s",
// TODO:               "OK", strerror(errno));
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    errorString[0] = 0;
// TODO:    nRead = fread(errorString, sizeof(char), MAX_PRINT_ERROR_LENGTH-1, pipe);
// TODO:    /* Make sure that the print command doesn't get stuck when trying to
// TODO:       write a lot of output on stderr (pipe may fill up). We discard
// TODO:       the additional output, though. */
// TODO:    while (fread(discarded, sizeof(char), 1024, pipe) > 0);
// TODO: 
// TODO:    if (!ferror(pipe))
// TODO:    {
// TODO:       errorString[nRead] = '\0';
// TODO:    }
// TODO: 
// TODO:    if (pclose(pipe))
// TODO:    {
// TODO:       DialogF(DF_WARN, widget, 1, "Print Error", "Unable to Print:\n%s",
// TODO:               "OK", errorString);
// TODO:       return;
// TODO:    }
// TODO: #endif /*(VMS)*/
// TODO: 
// TODO:    /* Print command succeeded, so retain the current print parameters */
// TODO:    if (CopiesOption[0] != '\0')
// TODO:    {
// TODO:       str = NeTextGetString(Text1);
// TODO:       strcpy(Copies, str);
// TODO:       XtFree(str);
// TODO:    }
// TODO:    if (QueueOption[0] != '\0')
// TODO:    {
// TODO:       str = NeTextGetString(Text2);
// TODO:       strcpy(Queue, str);
// TODO:       XtFree(str);
// TODO:    }
// TODO:    if (HostOption[0] != '\0')
// TODO:    {
// TODO:       str = NeTextGetString(Text3);
// TODO:       strcpy(Host, str);
// TODO:       XtFree(str);
// TODO:    }
// TODO:    str = NeTextGetString(Text4);
// TODO:    strcpy(CmdText, str);
// TODO:    XtFree(str);
// TODO: 
// TODO: 
// TODO:    /* Pop down the dialog */
// TODO:    DoneWithDialog = true;
// TODO: }
// TODO: 
// TODO: static void cancelButtonCB(Fl_Widget* widget, caddr_t client_data, caddr_t call_data)
// TODO: {
// TODO:    DoneWithDialog = true;
// TODO:    CmdFieldModified = false;
// TODO: }

/*
** Is the filename file in the directory dirpath
** and does it have at least some of the mode_flags enabled ?
*/
static int fileInDir(const char* filename, const char* dirpath, unsigned short mode_flags)
{
// TODO:    DIR*           dfile;
// TODO: #ifdef USE_DIRENT
// TODO:    struct dirent* DirEntryPtr;
// TODO: #else
// TODO:    struct direct* DirEntryPtr;
// TODO: #endif
// TODO:    struct stat   statbuf;
// TODO:    char          fullname[MAXPATHLEN];
// TODO: 
// TODO:    dfile = opendir(dirpath);
// TODO:    if (dfile != NULL)
// TODO:    {
// TODO:       while ((DirEntryPtr=readdir(dfile)) != NULL)
// TODO:       {
// TODO:          if (!strcmp(DirEntryPtr->d_name, filename))
// TODO:          {
// TODO:             strcpy(fullname,dirpath);
// TODO:             strcat(fullname,"/");
// TODO:             strcat(fullname,filename);
// TODO:             stat(fullname,&statbuf);
// TODO:             closedir(dfile);
// TODO:             return statbuf.st_mode & mode_flags;
// TODO:          }
// TODO:       }
// TODO:       closedir(dfile);
// TODO:    }
   return false;
}

/*
** Is the filename file in the environment path directories
** and does it have at least some of the mode_flags enabled ?
*/
static bool fileInPath(const char* filename, unsigned short mode_flags)
{
   char path[MAXPATHLEN];
   char* pathstring,*lastchar;

   /* Get environmental value of PATH */
   pathstring = getenv("PATH");
   if (pathstring == NULL)
      return false;

   /* parse the pathstring and search on each directory found */
   do
   {
      /* if final path in list is empty, don't search it */
      if (!strcmp(pathstring, ""))
         return false;
      /* locate address of next : character */
      lastchar = strchr(pathstring, SEPARATOR);
      if (lastchar != NULL)
      {
         /* if more directories remain in pathstring, copy up to : */
         strncpy(path, pathstring, lastchar-pathstring);
         path[lastchar-pathstring] = '\0';
      }
      else
      {
         /* if it's the last directory, just copy it */
         strcpy(path, pathstring);
      }
      /* search for the file in this path */
      if (fileInDir(filename, path, mode_flags))
         return true; /* found it !! */
      /* point pathstring to start of new dir string */
      pathstring = lastchar + 1;
   }
   while (lastchar != NULL);
   return false;
}

/*
** Is flpr present in the search path and is it executable ?
*/
static bool flprPresent()
{
   /* Is flpr present in the search path and is it executable ? */
   return fileInPath("flpr",0111);
}

static int foundTag(const char* tagfilename, const char* tagname, char* result)
{
   FILE* tfile;
   char tagformat[512],line[512];

   strcpy(tagformat, tagname);
   strcat(tagformat, " %s");

   tfile = fopen(tagfilename,"r");
   if (tfile != NULL)
   {
      while (!feof(tfile))
      {
         fgets(line,sizeof(line),tfile);
         if (sscanf(line,tagformat,result) != 0)
         {
            fclose(tfile);
            return true;
         }
      }
      fclose(tfile);
   }
   return false;
}

static int foundEnv(const char* EnvVarName, char* result)
{
   char* dqstr;

   dqstr = getenv(EnvVarName);
   if (dqstr != NULL)
   {
      strcpy(result,dqstr);
      return true;
   }
   return false;
}

static void getFlprHostDefault(char* defhost)
{
   if (!foundEnv("FLPHOST",defhost))
      if (!foundTag("/usr/local/etc/flp.defaults", "host", defhost))
         strcpy(defhost,"");
}

static void getFlprQueueDefault(char* defqueue)
{
   if (!foundEnv("FLPQUE",defqueue))
      if (!foundTag("/usr/local/etc/flp.defaults", "queue", defqueue))
         strcpy(defqueue,"");
}

#ifdef USE_LPR_PRINT_CMD
static void getLprQueueDefault(char* defqueue)
{
   if (!foundEnv("PRINTER",defqueue))
      strcpy(defqueue,"");
}
#endif

#ifndef USE_LPR_PRINT_CMD
static void getLpQueueDefault(char* defqueue)
{
   if (!foundEnv("LPDEST",defqueue))
      defqueue[0] = '\0';
}
#endif
