// TODO: static const char CVSID[] = "$Id: nc.c,v 1.48 2007/11/29 22:18:48 tringali Exp $";
// TODO: /*******************************************************************************
// TODO: *									       *
// TODO: * nc.c -- Nirvana Editor client program for nedit server processes	       *
// TODO: *									       *
// TODO: * Copyright (C) 1999 Mark Edel						       *
// TODO: *									       *
// TODO: * This is free__ software; you can redistribute it and/or modify it under the    *
// TODO: * terms of the GNU General Public License as published by the Free Software    *
// TODO: * Foundation; either version 2 of the License, or (at your option) any later   *
// TODO: * version. In addition, you may distribute version of this program linked to   *
// TODO: * Motif or Open Motif. See README for details.                                 *
// TODO: * 									       *
// TODO: * This software is distributed in the hope that it will be useful, but WITHOUT *
// TODO: * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
// TODO: * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
// TODO: * for more details.							       *
// TODO: * 									       *
// TODO: * You should have received a copy of the GNU General Public License along with *
// TODO: * software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
// TODO: * Place, Suite 330, Boston, MA  02111-1307 USA		                       *
// TODO: *									       *
// TODO: * Nirvana Text Editor	    						       *
// TODO: * November, 1995							       *
// TODO: *									       *
// TODO: * Written by Mark Edel							       *
// TODO: *									       *
// TODO: *******************************************************************************/
// TODO: 
// TODO: #ifdef HAVE_CONFIG_H
// TODO: #include "../config.h"
// TODO: #endif
// TODO: 
// TODO: #include "server_common.h"
// TODO: #include "../util/fileUtils.h"
// TODO: #include "../util/utils.h"
// TODO: #include "../util/prefFile.h"
// TODO: #include "../util/system.h"
// TODO: 
// TODO: #include <stdio.h>
// TODO: #include <stdlib.h>
// TODO: #include <limits.h>
// TODO: #include <string.h>
// TODO: #ifdef VMS
// TODO: #include <lib$routines.h>
// TODO: #include descrip
// TODO: #include ssdef
// TODO: #include syidef
// TODO: #include "../util/VMSparam.h"
// TODO: #include "../util/VMSutils.h"
// TODO: #else
// TODO: #ifndef WIN32
// TODO: #include <sys/param.h>
// TODO: #endif
// TODO: #include <sys/types.h>
// TODO: #include <sys/utsname.h>
// TODO: #include <unistd.h>
// TODO: #include <pwd.h>
// TODO: #include "../util/clearcase.h"
// TODO: #endif /* VMS */
// TODO: #ifdef __EMX__
// TODO: #include <process.h>
// TODO: #endif
// TODO: 
// TODO: #include <X11/Intrinsic.h>
// TODO: #include <X11/Xatom.h>
// TODO: 
// TODO: #ifdef HAVE_DEBUG_H
// TODO: #include "../debug.h"
// TODO: #endif
// TODO: 
// TODO: #define APP_NAME "nc"
// TODO: #define APP_CLASS "NEditClient"
// TODO: 
// TODO: #define PROPERTY_CHANGE_TIMEOUT (Preferences.timeOut * 1000) /* milliseconds */
// TODO: #define SERVER_START_TIMEOUT    (Preferences.timeOut * 3000) /* milliseconds */
// TODO: #define REQUEST_TIMEOUT         (Preferences.timeOut * 1000) /* milliseconds */
// TODO: #define FILE_OPEN_TIMEOUT       (Preferences.timeOut * 3000) /* milliseconds */
// TODO: 
// TODO: typedef struct
// TODO: {
// TODO:    char* shell;
// TODO:    char* serverRequest;
// TODO: } CommandLine;
// TODO: 
// TODO: static void timeOutProc(bool* timeOutReturn, XtIntervalId* id);
// TODO: static int startServer(const char* message, const char* commandLine);
// TODO: static CommandLine processCommandLine(int argc, char** argv);
// TODO: static void parseCommandLine(int argc, char** arg, CommandLine* cmdLine);
// TODO: static void nextArg(int argc, char** argv, int* argIndex);
// TODO: static void copyCommandLineArg(CommandLine* cmdLine, const char* arg);
// TODO: static void printNcVersion();
// TODO: static bool findExistingServer(XtAppContext context,
// TODO:                                   Window rootWindow,
// TODO:                                   Atom serverExistsAtom);
// TODO: static void startNewServer(XtAppContext context,
// TODO:                            Window rootWindow,
// TODO:                            char* commandLine,
// TODO:                            Atom serverExistsAtom);
// TODO: static void waitUntilRequestProcessed(XtAppContext context,
// TODO:                                       Window rootWindow,
// TODO:                                       char* commandString,
// TODO:                                       Atom serverRequestAtom);
// TODO: static void waitUntilFilesOpenedOrClosed(XtAppContext context,
// TODO:       Window rootWindow);
// TODO: 
// TODO: Display* TheDisplay;
// TODO: XtAppContext AppContext;
// TODO: static Atom currentWaitForAtom;
// TODO: static Atom noAtom = (Atom)(-1);
// TODO: 
// TODO: static const char cmdLineHelp[] =
// TODO: #ifdef VMS
// TODO:    "[Sorry, no on-line help available.]\n"; /* Why is that ? */
// TODO: #else
// TODO:    "Usage:  nc [-read] [-create]\n"
// TODO:    "           [-line n | +n] [-do command] [-lm languagemode]\n"
// TODO:    "           [-svrname name] [-svrcmd command]\n"
// TODO:    "           [-ask] [-noask] [-timeout seconds]\n"
// TODO:    "           [-geometry geometry | -g geometry] [-icon | -iconic]\n"
// TODO:    "           [-tabbed] [-untabbed] [-group] [-wait]\n"
// TODO:    "           [-V | -version] [-h|-help]\n"
// TODO:    "           [-xrm resourcestring] [-display [host]:server[.screen]]\n"
// TODO:    "           [--] [file...]\n";
// TODO: #endif /*VMS*/
// TODO: 
// TODO: /* Structure to hold X Resource values */
// TODO: static struct
// TODO: {
// TODO:    int autoStart;
// TODO:    char serverCmd[2*MAXPATHLEN]; /* holds executable name + flags */
// TODO:    char serverName[MAXPATHLEN];
// TODO:    int waitForClose;
// TODO:    int timeOut;
// TODO: } Preferences;
// TODO: 
// TODO: /* Application resources */
// TODO: static PrefDescripRec PrefDescrip[] =
// TODO: {
// TODO:    {
// TODO:       "autoStart", "AutoStart", PREF_BOOLEAN, "true",
// TODO:       &Preferences.autoStart, NULL, true
// TODO:    },
// TODO:    {
// TODO:       "serverCommand", "ServerCommand", PREF_STRING, "nedit -server",
// TODO:       Preferences.serverCmd, (void*)sizeof(Preferences.serverCmd), false
// TODO:    },
// TODO:    {
// TODO:       "serverName", "serverName", PREF_STRING, "", Preferences.serverName,
// TODO:       (void*)sizeof(Preferences.serverName), false
// TODO:    },
// TODO:    {
// TODO:       "waitForClose", "WaitForClose", PREF_BOOLEAN, "false",
// TODO:       &Preferences.waitForClose, NULL, false
// TODO:    },
// TODO:    {
// TODO:       "timeOut", "TimeOut", PREF_INT, "10",
// TODO:       &Preferences.timeOut, NULL, false
// TODO:    }
// TODO: };
// TODO: 
// TODO: /* Resource related command line options */
// TODO: static XrmOptionDescRec OpTable[] =
// TODO: {
// TODO:    {"-ask", ".autoStart", XrmoptionNoArg, (caddr_t)"false"},
// TODO:    {"-noask", ".autoStart", XrmoptionNoArg, (caddr_t)"true"},
// TODO:    {"-svrname", ".serverName", XrmoptionSepArg, (caddr_t)NULL},
// TODO:    {"-svrcmd", ".serverCommand", XrmoptionSepArg, (caddr_t)NULL},
// TODO:    {"-wait", ".waitForClose", XrmoptionNoArg, (caddr_t)"true"},
// TODO:    {"-timeout", ".timeOut", XrmoptionSepArg, (caddr_t)NULL}
// TODO: };
// TODO: 
// TODO: /* Struct to hold info about files being opened and edited. */
// TODO: typedef struct _FileListEntry
// TODO: {
// TODO:    Atom                  waitForFileOpenAtom;
// TODO:    Atom                  waitForFileClosedAtom;
// TODO:    char*                 path;
// TODO:    struct _FileListEntry* next;
// TODO: } FileListEntry;
// TODO: 
// TODO: typedef struct
// TODO: {
// TODO:    int            waitForOpenCount;
// TODO:    int            waitForCloseCount;
// TODO:    FileListEntry* fileList;
// TODO: } FileListHead;
// TODO: static FileListHead fileListHead;
// TODO: 
// TODO: static void setPropertyValue(Atom atom)
// TODO: {
// TODO:    XChangeProperty(TheDisplay,
// TODO:                    RootWindow(TheDisplay, DefaultScreen(TheDisplay)),
// TODO:                    atom, XA_STRING,
// TODO:                    8, PropModeReplace,
// TODO:                    (unsigned char*)"true", 4);
// TODO: }
// TODO: 
// TODO: /* Add another entry to the file entry list, if it doesn't exist yet. */
// TODO: static void addToFileList(const char* path)
// TODO: {
// TODO:    FileListEntry* item;
// TODO: 
// TODO:    /* see if the file already exists in the list */
// TODO:    for (item = fileListHead.fileList; item; item = item->next)
// TODO:    {
// TODO:       if (!strcmp(item->path, path))
// TODO:          break;
// TODO:    }
// TODO: 
// TODO:    /* Add the atom to the head of the file list if it wasn't found. */
// TODO:    if (item == 0)
// TODO:    {
// TODO:       item = malloc__(sizeof(item[0]));
// TODO:       item->waitForFileOpenAtom = None;
// TODO:       item->waitForFileClosedAtom = None;
// TODO:       item->path = (char*)malloc__(strlen(path)+1);
// TODO:       strcpy(item->path, path);
// TODO:       item->next = fileListHead.fileList;
// TODO:       fileListHead.fileList = item;
// TODO:    }
// TODO: }
// TODO: 
// TODO: /* Creates the properties for the various paths */
// TODO: static void createWaitProperties()
// TODO: {
// TODO:    FileListEntry* item;
// TODO: 
// TODO:    for (item = fileListHead.fileList; item; item = item->next)
// TODO:    {
// TODO:       fileListHead.waitForOpenCount++;
// TODO:       item->waitForFileOpenAtom =
// TODO:          CreateServerFileOpenAtom(Preferences.serverName, item->path);
// TODO:       setPropertyValue(item->waitForFileOpenAtom);
// TODO: 
// TODO:       if (Preferences.waitForClose == true)
// TODO:       {
// TODO:          fileListHead.waitForCloseCount++;
// TODO:          item->waitForFileClosedAtom =
// TODO:             CreateServerFileClosedAtom(Preferences.serverName,
// TODO:                                        item->path,
// TODO:                                        false);
// TODO:          setPropertyValue(item->waitForFileClosedAtom);
// TODO:       }
// TODO:    }
// TODO: }
// TODO: 
// TODO: int main(int argc, char** argv)
// TODO: {
// TODO:    XtAppContext context;
// TODO:    Window rootWindow;
// TODO:    CommandLine commandLine;
// TODO:    Atom serverExistsAtom, serverRequestAtom;
// TODO:    XrmDatabase prefDB;
// TODO:    bool serverExists;
// TODO: 
// TODO:    /* Initialize toolkit and get an application context */
// TODO:    XtToolkitInitialize();
// TODO:    AppContext = context = XtCreateApplicationContext();
// TODO: 
// TODO: #ifdef VMS
// TODO:    /* Convert the command line to Unix style */
// TODO:    ConvertVMSCommandLine(&argc, &argv);
// TODO: #endif /*VMS*/
// TODO: #ifdef __EMX__
// TODO:    /* expand wildcards if necessary */
// TODO:    _wildcard(&argc, &argv);
// TODO: #endif
// TODO: 
// TODO:    /* Read the preferences command line into a database (note that we
// TODO:       don't support the .nc file anymore) */
// TODO:    prefDB = CreatePreferencesDatabase(NULL, APP_CLASS,
// TODO:                                       OpTable, XtNumber(OpTable), (unsigned*)&argc, argv);
// TODO: 
// TODO:    /* Process the command line before calling XtOpenDisplay, because the
// TODO:       latter consumes certain command line arguments that we still need
// TODO:       (-icon, -geometry ...) */
// TODO:    commandLine = processCommandLine(argc, argv);
// TODO: 
// TODO:    /* Open the display and find the root window */
// TODO:    TheDisplay = XtOpenDisplay(context, NULL, APP_NAME, APP_CLASS, NULL,
// TODO:                               0, &argc, argv);
// TODO:    if (!TheDisplay)
// TODO:    {
// TODO:       XtWarning("nc: Can't open display\n");
// TODO:       exit(EXIT_FAILURE);
// TODO:    }
// TODO:    rootWindow = RootWindow(TheDisplay, DefaultScreen(TheDisplay));
// TODO: 
// TODO:    /* Read the application resources into the Preferences data structure */
// TODO:    RestorePreferences(prefDB, XtDatabase(TheDisplay), APP_NAME,
// TODO:                       APP_CLASS, PrefDescrip, XtNumber(PrefDescrip));
// TODO: 
// TODO:    /* Make sure that the time out unit is at least 1 second and not too
// TODO:       large either (overflow!). */
// TODO:    if (Preferences.timeOut < 1)
// TODO:    {
// TODO:       Preferences.timeOut = 1;
// TODO:    }
// TODO:    else if (Preferences.timeOut > 1000)
// TODO:    {
// TODO:       Preferences.timeOut = 1000;
// TODO:    }
// TODO: 
// TODO: #ifndef VMS
// TODO:    /* For Clearcase users who have not set a server name, use the clearcase
// TODO:       view name.  Clearcase views make files with the same absolute path names
// TODO:       but different contents (and therefore can't be edited in the same nedit
// TODO:       session). This should have no bad side-effects for non-clearcase users */
// TODO:    if (Preferences.serverName[0] == '\0')
// TODO:    {
// TODO:       const char* viewTag = GetClearCaseViewTag();
// TODO:       if (viewTag != NULL && strlen(viewTag) < MAXPATHLEN)
// TODO:       {
// TODO:          strcpy(Preferences.serverName, viewTag);
// TODO:       }
// TODO:    }
// TODO: #endif /* VMS */
// TODO: 
// TODO:    /* Create the wait properties for the various files. */
// TODO:    createWaitProperties();
// TODO: 
// TODO:    /* Monitor the properties on the root window */
// TODO:    XSelectInput(TheDisplay, rootWindow, PropertyChangeMask);
// TODO: 
// TODO:    /* Create the server property atoms on the current DISPLAY. */
// TODO:    CreateServerPropertyAtoms(Preferences.serverName,
// TODO:                              &serverExistsAtom,
// TODO:                              &serverRequestAtom);
// TODO: 
// TODO:    serverExists = findExistingServer(context,
// TODO:                                      rootWindow,
// TODO:                                      serverExistsAtom);
// TODO: 
// TODO:    if (serverExists == false)
// TODO:       startNewServer(context, rootWindow, commandLine.shell, serverExistsAtom);
// TODO: 
// TODO:    waitUntilRequestProcessed(context,
// TODO:                              rootWindow,
// TODO:                              commandLine.serverRequest,
// TODO:                              serverRequestAtom);
// TODO: 
// TODO:    waitUntilFilesOpenedOrClosed(context, rootWindow);
// TODO: 
// TODO:    XtCloseDisplay(TheDisplay);
// TODO:    XtFree(commandLine.shell);
// TODO:    XtFree(commandLine.serverRequest);
// TODO:    return 0;
// TODO: }
// TODO: 
// TODO: 
// TODO: /*
// TODO: ** Xt timer procedure for timeouts on NEdit server requests
// TODO: */
// TODO: static void timeOutProc(bool* timeOutReturn, XtIntervalId* id)
// TODO: {
// TODO:    /* NOTE: XtAppNextEvent() does call this routine but
// TODO:    ** doesn't return unless there are more events.
// TODO:    ** Hence, we generate this (synthetic) event to break the deadlock
// TODO:    */
// TODO:    Window rootWindow = RootWindow(TheDisplay, DefaultScreen(TheDisplay));
// TODO:    if (currentWaitForAtom != noAtom)
// TODO:    {
// TODO:       XChangeProperty(TheDisplay, rootWindow, currentWaitForAtom, XA_STRING,
// TODO:                       8, PropModeReplace, (unsigned char*)"", strlen(""));
// TODO:    }
// TODO: 
// TODO:    /* Flag that the timeout has occurred. */
// TODO:    *timeOutReturn = true;
// TODO: }
// TODO: 
// TODO: 
// TODO: 
// TODO: static bool findExistingServer(XtAppContext context,
// TODO:                                   Window rootWindow,
// TODO:                                   Atom serverExistsAtom)
// TODO: {
// TODO:    bool serverExists = true;
// TODO:    unsigned char* propValue;
// TODO:    int getFmt;
// TODO:    Atom dummyAtom;
// TODO:    unsigned long dummyULong, nItems;
// TODO: 
// TODO:    /* See if there might be a server (not a guaranty), by translating the
// TODO:       root window property NEDIT_SERVER_EXISTS_<user>_<host> */
// TODO:    if (XGetWindowProperty(TheDisplay, rootWindow, serverExistsAtom, 0,
// TODO:                           INT_MAX, false, XA_STRING, &dummyAtom, &getFmt, &nItems,
// TODO:                           &dummyULong, &propValue) != Success || nItems == 0)
// TODO:    {
// TODO:       serverExists = false;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       bool timeOut = false;
// TODO:       XtIntervalId timerId;
// TODO: 
// TODO:       XFree(propValue);
// TODO: 
// TODO:       /* Remove the server exists property to make sure the server is
// TODO:       ** running. If it is running it will get recreated.
// TODO:       */
// TODO:       XDeleteProperty(TheDisplay, rootWindow, serverExistsAtom);
// TODO:       XSync(TheDisplay, false);
// TODO:       timerId = XtAppAddTimeOut(context,
// TODO:                                 PROPERTY_CHANGE_TIMEOUT,
// TODO:                                 (XtTimerCallbackProc)timeOutProc,
// TODO:                                 &timeOut);
// TODO:       currentWaitForAtom = serverExistsAtom;
// TODO: 
// TODO:       while (!timeOut)
// TODO:       {
// TODO:          /* NOTE: XtAppNextEvent() does call the timeout routine but
// TODO:          ** doesn't return unless there are more events. */
// TODO:          XEvent event;
// TODO:          const XPropertyEvent* e = (const XPropertyEvent*)&event;
// TODO:          XtAppNextEvent(context, &event);
// TODO: 
// TODO:          /* We will get a PropertyNewValue when the server recreates
// TODO:          ** the server exists atom. */
// TODO:          if (e->type == PropertyNotify &&
// TODO:                e->window == rootWindow &&
// TODO:                e->atom == serverExistsAtom)
// TODO:          {
// TODO:             if (e->state == PropertyNewValue)
// TODO:             {
// TODO:                break;
// TODO:             }
// TODO:          }
// TODO:          XtDispatchEvent(&event);
// TODO:       }
// TODO: 
// TODO:       /* Start a new server if the timeout expired. The server exists
// TODO:       ** property was not recreated. */
// TODO:       if (timeOut)
// TODO:       {
// TODO:          serverExists = false;
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          XtRemoveTimeOut(timerId);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    return(serverExists);
// TODO: }
// TODO: 
// TODO: 
// TODO: 
// TODO: 
// TODO: static void startNewServer(XtAppContext context,
// TODO:                            Window rootWindow,
// TODO:                            char* commandLine,
// TODO:                            Atom serverExistsAtom)
// TODO: {
// TODO:    bool timeOut = false;
// TODO:    XtIntervalId timerId;
// TODO: 
// TODO:    /* Add back the server name resource from the command line or resource
// TODO:       database to the command line for starting the server.  If -svrcmd
// TODO:       appeared on the original command line, it was removed by
// TODO:       CreatePreferencesDatabase before the command line was recorded
// TODO:       in commandLine.shell. Moreover, if no server name was specified, it
// TODO:       may have defaulted to the ClearCase view tag. */
// TODO:    if (Preferences.serverName[0] != '\0')
// TODO:    {
// TODO:       strcat(commandLine, " -svrname ");
// TODO:       strcat(commandLine, Preferences.serverName);
// TODO:    }
// TODO:    switch (startServer("No servers available, start one? (y|n) [y]: ",
// TODO:                        commandLine))
// TODO:    {
// TODO:    case -1: /* Start failed */
// TODO:       XtCloseDisplay(TheDisplay);
// TODO:       exit(EXIT_FAILURE);
// TODO:       break;
// TODO:    case -2: /* Start canceled by user */
// TODO:       XtCloseDisplay(TheDisplay);
// TODO:       exit(EXIT_SUCCESS);
// TODO:       break;
// TODO:    }
// TODO: 
// TODO:    /* Set up a timeout proc in case the server is dead.  The standard
// TODO:       selection timeout is probably a good guess at how long to wait
// TODO:       for this style of inter-client communication as well */
// TODO:    timerId = XtAppAddTimeOut(context,
// TODO:                              SERVER_START_TIMEOUT,
// TODO:                              (XtTimerCallbackProc)timeOutProc,
// TODO:                              &timeOut);
// TODO:    currentWaitForAtom = serverExistsAtom;
// TODO: 
// TODO:    /* Wait for the server to start */
// TODO:    while (!timeOut)
// TODO:    {
// TODO:       XEvent event;
// TODO:       const XPropertyEvent* e = (const XPropertyEvent*)&event;
// TODO:       /* NOTE: XtAppNextEvent() does call the timeout routine but
// TODO:       ** doesn't return unless there are more events. */
// TODO:       XtAppNextEvent(context, &event);
// TODO: 
// TODO:       /* We will get a PropertyNewValue when the server updates
// TODO:       ** the server exists atom. If the property is deleted the
// TODO:       ** the server must have died. */
// TODO:       if (e->type == PropertyNotify &&
// TODO:             e->window == rootWindow &&
// TODO:             e->atom == serverExistsAtom)
// TODO:       {
// TODO:          if (e->state == PropertyNewValue)
// TODO:          {
// TODO:             break;
// TODO:          }
// TODO:          else if (e->state == PropertyDelete)
// TODO:          {
// TODO:             fprintf(stderr, "%s: The server failed to start.\n", APP_NAME);
// TODO:             XtCloseDisplay(TheDisplay);
// TODO:             exit(EXIT_FAILURE);
// TODO:          }
// TODO:       }
// TODO:       XtDispatchEvent(&event);
// TODO:    }
// TODO:    /* Exit if the timeout expired. */
// TODO:    if (timeOut)
// TODO:    {
// TODO:       fprintf(stderr, "%s: The server failed to start (time-out).\n", APP_NAME);
// TODO:       XtCloseDisplay(TheDisplay);
// TODO:       exit(EXIT_FAILURE);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       XtRemoveTimeOut(timerId);
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Prompt the user about starting a server, with "message", then start server
// TODO: */
// TODO: static int startServer(const char* message, const char* commandLineArgs)
// TODO: {
// TODO:    char c, *commandLine;
// TODO: #ifdef VMS
// TODO:    int spawnFlags = 1 /* + 1<<3 */;			/* NOWAIT, NOKEYPAD */
// TODO:    int spawn_sts;
// TODO:    struct dsc$descriptor_s* cmdDesc;
// TODO:    char* nulDev = "NL:";
// TODO:    struct dsc$descriptor_s* nulDevDesc;
// TODO: #else
// TODO:    int sysrc;
// TODO: #endif /* !VMS */
// TODO: 
// TODO:    /* prompt user whether to start server */
// TODO:    if (!Preferences.autoStart)
// TODO:    {
// TODO:       printf(message);
// TODO:       do
// TODO:       {
// TODO:          c = getc(stdin);
// TODO:       }
// TODO:       while (c == ' ' || c == '\t');
// TODO:       if (c != 'Y' && c != 'y' && c != '\n')
// TODO:          return (-2);
// TODO:    }
// TODO: 
// TODO:    /* start the server */
// TODO: #ifdef VMS
// TODO:    commandLine = malloc__(strlen(Preferences.serverCmd) +
// TODO:                           strlen(commandLineArgs) + 3);
// TODO:    sprintf(commandLine, "%s %s", Preferences.serverCmd, commandLineArgs);
// TODO:    cmdDesc = NulStrToDesc(commandLine);	/* build command descriptor */
// TODO:    nulDevDesc = NulStrToDesc(nulDev);		/* build "NL:" descriptor */
// TODO:    spawn_sts = lib$spawn(cmdDesc, nulDevDesc, 0, &spawnFlags, 0,0,0,0,0,0,0,0);
// TODO:    XtFree(commandLine);
// TODO:    if (spawn_sts != SS$_NORMAL)
// TODO:    {
// TODO:       fprintf(stderr, "Error return from lib$spawn: %d\n", spawn_sts);
// TODO:       fprintf(stderr, "Nedit server not started.\n");
// TODO:       return (-1);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       FreeStrDesc(cmdDesc);
// TODO:       return 0;
// TODO:    }
// TODO: #else
// TODO: #if defined(__EMX__)  /* OS/2 */
// TODO:    /* Unfortunately system() calls a shell determined by the environment
// TODO:       variables COMSPEC and EMXSHELL. We have to figure out which one. */
// TODO:    {
// TODO:       char* sh_spec, *sh, *base;
// TODO:       char* CMD_EXE="cmd.exe";
// TODO: 
// TODO:       commandLine = malloc__(strlen(Preferences.serverCmd) +
// TODO:                              strlen(commandLineArgs) + 15);
// TODO:       sh = getenv("EMXSHELL");
// TODO:       if (sh == NULL)
// TODO:          sh = getenv("COMSPEC");
// TODO:       if (sh == NULL)
// TODO:          sh = CMD_EXE;
// TODO:       sh_spec=NeNewString(sh);
// TODO:       base=_getname(sh_spec);
// TODO:       _remext(base);
// TODO:       if (stricmp(base, "cmd") == 0 || stricmp(base, "4os2") == 0)
// TODO:       {
// TODO:          sprintf(commandLine, "start /C /MIN %s %s",
// TODO:                  Preferences.serverCmd, commandLineArgs);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          sprintf(commandLine, "%s %s &",
// TODO:                  Preferences.serverCmd, commandLineArgs);
// TODO:       }
// TODO:       XtFree(sh_spec);
// TODO:    }
// TODO: #else /* Unix */
// TODO:    commandLine = malloc__(strlen(Preferences.serverCmd) +
// TODO:                           strlen(commandLineArgs) + 3);
// TODO:    sprintf(commandLine, "%s %s&", Preferences.serverCmd, commandLineArgs);
// TODO: #endif
// TODO: 
// TODO:    sysrc=system(commandLine);
// TODO:    XtFree(commandLine);
// TODO: 
// TODO:    if (sysrc==0)
// TODO:       return 0;
// TODO:    else
// TODO:       return (-1);
// TODO: #endif /* !VMS */
// TODO: }
// TODO: 
// TODO: /* Reconstruct the command line in string commandLine in case we have to
// TODO:  * start a server (nc command line args parallel nedit's).  Include
// TODO:  * -svrname if nc wants a named server, so nedit will match. Special
// TODO:  * characters are protected from the shell by escaping EVERYTHING with \
// TODO:  */
// TODO: static CommandLine processCommandLine(int argc, char** argv)
// TODO: {
// TODO:    CommandLine commandLine;
// TODO:    int i;
// TODO:    int length = 0;
// TODO: 
// TODO:    for (i=1; i<argc; i++)
// TODO:    {
// TODO:       length += 1 + strlen(argv[i])*4 + 2;
// TODO:    }
// TODO:    commandLine.shell = malloc__(length+1 + 9 + MAXPATHLEN);
// TODO:    *commandLine.shell = '\0';
// TODO: 
// TODO:    /* Convert command line arguments into a command string for the server */
// TODO:    parseCommandLine(argc, argv, &commandLine);
// TODO:    if (commandLine.serverRequest == NULL)
// TODO:    {
// TODO:       fprintf(stderr, "nc: Invalid commandline argument\n");
// TODO:       exit(EXIT_FAILURE);
// TODO:    }
// TODO: 
// TODO:    return(commandLine);
// TODO: }
// TODO: 
// TODO: 
// TODO: /*
// TODO: ** Converts command line into a command string suitable for passing to
// TODO: ** the server
// TODO: */
// TODO: static void parseCommandLine(int argc, char** argv, CommandLine* commandLine)
// TODO: {
// TODO: #define MAX_RECORD_HEADER_LENGTH 38
// TODO:    char name[MAXPATHLEN], path[MAXPATHLEN];
// TODO:    const char* toDoCommand = "", *langMode = "", *geometry = "";
// TODO:    char* commandString, *outPtr;
// TODO:    int lineNum = 0, read = 0, create = 0, iconic = 0, tabbed = -1, length = 0;
// TODO:    int i, lineArg, nRead, charsWritten, opts = true;
// TODO:    int fileCount = 0, group = 0, isTabbed;
// TODO: 
// TODO:    /* Allocate a string for output, for the maximum possible length.  The
// TODO:       maximum length is calculated by assuming every argument is a file,
// TODO:       and a complete record of maximum length is created for it */
// TODO:    for (i=1; i<argc; i++)
// TODO:    {
// TODO:       length += MAX_RECORD_HEADER_LENGTH + strlen(argv[i]) + MAXPATHLEN;
// TODO:    }
// TODO:    /* In case of no arguments, must still allocate space for one record header */
// TODO:    if (length < MAX_RECORD_HEADER_LENGTH)
// TODO:    {
// TODO:       length = MAX_RECORD_HEADER_LENGTH;
// TODO:    }
// TODO:    commandString = malloc__(length+1);
// TODO: 
// TODO:    /* Parse the arguments and write the output string */
// TODO:    outPtr = commandString;
// TODO:    for (i=1; i<argc; i++)
// TODO:    {
// TODO:       if (opts && !strcmp(argv[i], "--"))
// TODO:       {
// TODO:          opts = false; /* treat all remaining arguments as filenames */
// TODO:          continue;
// TODO:       }
// TODO:       else if (opts && !strcmp(argv[i], "-do"))
// TODO:       {
// TODO:          nextArg(argc, argv, &i);
// TODO:          toDoCommand = argv[i];
// TODO:       }
// TODO:       else if (opts && !strcmp(argv[i], "-lm"))
// TODO:       {
// TODO:          copyCommandLineArg(commandLine, argv[i]);
// TODO:          nextArg(argc, argv, &i);
// TODO:          langMode = argv[i];
// TODO:          copyCommandLineArg(commandLine, argv[i]);
// TODO:       }
// TODO:       else if (opts && (!strcmp(argv[i], "-g")  ||
// TODO:                         !strcmp(argv[i], "-geometry")))
// TODO:       {
// TODO:          copyCommandLineArg(commandLine, argv[i]);
// TODO:          nextArg(argc, argv, &i);
// TODO:          geometry = argv[i];
// TODO:          copyCommandLineArg(commandLine, argv[i]);
// TODO:       }
// TODO:       else if (opts && !strcmp(argv[i], "-read"))
// TODO:       {
// TODO:          read = 1;
// TODO:       }
// TODO:       else if (opts && !strcmp(argv[i], "-create"))
// TODO:       {
// TODO:          create = 1;
// TODO:       }
// TODO:       else if (opts && !strcmp(argv[i], "-tabbed"))
// TODO:       {
// TODO:          tabbed = 1;
// TODO:          group = 0;	/* override -group option */
// TODO:       }
// TODO:       else if (opts && !strcmp(argv[i], "-untabbed"))
// TODO:       {
// TODO:          tabbed = 0;
// TODO:          group = 0;  /* override -group option */
// TODO:       }
// TODO:       else if (opts && !strcmp(argv[i], "-group"))
// TODO:       {
// TODO:          group = 2; /* 2: start new group, 1: in group */
// TODO:       }
// TODO:       else if (opts && (!strcmp(argv[i], "-iconic") ||
// TODO:                         !strcmp(argv[i], "-icon")))
// TODO:       {
// TODO:          iconic = 1;
// TODO:          copyCommandLineArg(commandLine, argv[i]);
// TODO:       }
// TODO:       else if (opts && !strcmp(argv[i], "-line"))
// TODO:       {
// TODO:          nextArg(argc, argv, &i);
// TODO:          nRead = sscanf(argv[i], "%d", &lineArg);
// TODO:          if (nRead != 1)
// TODO:             fprintf(stderr, "nc: argument to line should be a number\n");
// TODO:          else
// TODO:             lineNum = lineArg;
// TODO:       }
// TODO:       else if (opts && (*argv[i] == '+'))
// TODO:       {
// TODO:          nRead = sscanf((argv[i]+1), "%d", &lineArg);
// TODO:          if (nRead != 1)
// TODO:             fprintf(stderr, "nc: argument to + should be a number\n");
// TODO:          else
// TODO:             lineNum = lineArg;
// TODO:       }
// TODO:       else if (opts && (!strcmp(argv[i], "-ask") || !strcmp(argv[i], "-noask")))
// TODO:       {
// TODO:          ; /* Ignore resource-based arguments which are processed later */
// TODO:       }
// TODO:       else if (opts && (!strcmp(argv[i], "-svrname") ||
// TODO:                         !strcmp(argv[i], "-svrcmd")))
// TODO:       {
// TODO:          nextArg(argc, argv, &i); /* Ignore rsrc args with data */
// TODO:       }
// TODO:       else if (opts && (!strcmp(argv[i], "-xrm") ||
// TODO:                         !strcmp(argv[i], "-display")))
// TODO:       {
// TODO:          copyCommandLineArg(commandLine, argv[i]);
// TODO:          nextArg(argc, argv, &i); /* Ignore rsrc args with data */
// TODO:          copyCommandLineArg(commandLine, argv[i]);
// TODO:       }
// TODO:       else if (opts && (!strcmp(argv[i], "-version") || !strcmp(argv[i], "-V")))
// TODO:       {
// TODO:          printNcVersion();
// TODO:          exit(EXIT_SUCCESS);
// TODO:       }
// TODO:       else if (opts && (!strcmp(argv[i], "-h") ||
// TODO:                         !strcmp(argv[i], "-help")))
// TODO:       {
// TODO:          fprintf(stderr, "%s", cmdLineHelp);
// TODO:          exit(EXIT_SUCCESS);
// TODO:       }
// TODO:       else if (opts && (*argv[i] == '-'))
// TODO:       {
// TODO: #ifdef VMS
// TODO:          *argv[i] = '/';
// TODO: #endif /*VMS*/
// TODO:          fprintf(stderr, "nc: Unrecognized option %s\n%s", argv[i],
// TODO:                  cmdLineHelp);
// TODO:          exit(EXIT_FAILURE);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO: #ifdef VMS
// TODO:          int numFiles, j, oldLength;
// TODO:          char** nameList = NULL, *newCommandString;
// TODO:          /* Use VMS's LIB$FILESCAN for filename in argv[i] to process */
// TODO:          /* wildcards and to obtain a full VMS file specification     */
// TODO:          numFiles = VMSFileScan(argv[i], &nameList, NULL, INCLUDE_FNF);
// TODO:          /* for each expanded file name do: */
// TODO:          for (j = 0; j < numFiles; ++j)
// TODO:          {
// TODO:             oldLength = outPtr-commandString;
// TODO:             newCommandString = malloc__(oldLength+length+1);
// TODO:             strncpy(newCommandString, commandString, oldLength);
// TODO:             XtFree(commandString);
// TODO:             commandString = newCommandString;
// TODO:             outPtr = newCommandString + oldLength;
// TODO:             if (ParseFilename(nameList[j], name, path) != 0)
// TODO:             {
// TODO:                /* An Error, most likely too long paths/strings given */
// TODO:                commandLine->serverRequest = NULL;
// TODO:                return;
// TODO:             }
// TODO:             strcat(path, name);
// TODO: 
// TODO:             /* determine if file is to be openned in new tab, by
// TODO:                factoring the options -group, -tabbed & -untabbed */
// TODO:             if (group == 2)
// TODO:             {
// TODO:                isTabbed = 0;  /* start a new window for new group */
// TODO:                group = 1;     /* next file will be within group */
// TODO:             }
// TODO:             else if (group == 1)
// TODO:             {
// TODO:                isTabbed = 1;  /* new tab for file in group */
// TODO:             }
// TODO:             else
// TODO:             {
// TODO:                isTabbed = tabbed; /* not in group */
// TODO:             }
// TODO: 
// TODO:             /* See below for casts */
// TODO:             sprintf(outPtr, "%d %d %d %d %d %ld %ld %ld %ld\n%s\n%s\n%s\n%s\n%n",
// TODO:                     lineNum, read, create, iconic, tabbed, (long) strlen(path),
// TODO:                     (long) strlen(toDoCommand), (long) strlen(langMode),
// TODO:                     (long) strlen(geometry),
// TODO:                     path, toDoCommand, langMode, geometry, &charsWritten);
// TODO:             outPtr += charsWritten;
// TODO:             free__(nameList[j]);
// TODO: 
// TODO:             /* Create the file open atoms for the paths supplied */
// TODO:             addToFileList(path);
// TODO:             fileCount++;
// TODO:          }
// TODO:          if (nameList != NULL)
// TODO:             free__(nameList);
// TODO: #else
// TODO:          if (ParseFilename(argv[i], name, path) != 0)
// TODO:          {
// TODO:             /* An Error, most likely too long paths/strings given */
// TODO:             commandLine->serverRequest = NULL;
// TODO:             return;
// TODO:          }
// TODO:          strcat(path, name);
// TODO: 
// TODO:          /* determine if file is to be openned in new tab, by
// TODO:             factoring the options -group, -tabbed & -untabbed */
// TODO:          if (group == 2)
// TODO:          {
// TODO:             isTabbed = 0;  /* start a new window for new group */
// TODO:             group = 1;     /* next file will be within group */
// TODO:          }
// TODO:          else if (group == 1)
// TODO:          {
// TODO:             isTabbed = 1;  /* new tab for file in group */
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             isTabbed = tabbed; /* not in group */
// TODO:          }
// TODO: 
// TODO:          /* SunOS 4 acc or acc and/or its runtime library has a bug
// TODO:             such that %n fails (segv) if it follows a string in a
// TODO:             printf or sprintf.  The silly code below avoids this.
// TODO: 
// TODO:               The "long" cast on strlen() is necessary because size_t
// TODO:               is 64 bit on Alphas, and 32-bit on most others.  There is
// TODO:               no printf format specifier for "size_t", thanx, ANSI. */
// TODO:          sprintf(outPtr, "%d %d %d %d %d %ld %ld %ld %ld\n%n", lineNum,
// TODO:                  read, create, iconic, isTabbed, (long) strlen(path),
// TODO:                  (long) strlen(toDoCommand), (long) strlen(langMode),
// TODO:                  (long) strlen(geometry), &charsWritten);
// TODO:          outPtr += charsWritten;
// TODO:          strcpy(outPtr, path);
// TODO:          outPtr += strlen(path);
// TODO:          *outPtr++ = '\n';
// TODO:          strcpy(outPtr, toDoCommand);
// TODO:          outPtr += strlen(toDoCommand);
// TODO:          *outPtr++ = '\n';
// TODO:          strcpy(outPtr, langMode);
// TODO:          outPtr += strlen(langMode);
// TODO:          *outPtr++ = '\n';
// TODO:          strcpy(outPtr, geometry);
// TODO:          outPtr += strlen(geometry);
// TODO:          *outPtr++ = '\n';
// TODO:          toDoCommand = "";
// TODO: 
// TODO:          /* Create the file open atoms for the paths supplied */
// TODO:          addToFileList(path);
// TODO:          fileCount++;
// TODO: #endif /* VMS */
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* If there's an un-written -do command, or we are to open a new window,
// TODO:     * or user has requested iconic state, but not provided a file name,
// TODO:     * create a server request with an empty file name and requested
// TODO:     * iconic state (and optional language mode and geometry).
// TODO:     */
// TODO:    if (toDoCommand[0] != '\0' || fileCount == 0)
// TODO:    {
// TODO:       sprintf(outPtr, "0 0 0 %d %d 0 %ld %ld %ld\n\n%n", iconic, tabbed,
// TODO:               (long) strlen(toDoCommand),
// TODO:               (long) strlen(langMode), (long) strlen(geometry), &charsWritten);
// TODO:       outPtr += charsWritten;
// TODO:       strcpy(outPtr, toDoCommand);
// TODO:       outPtr += strlen(toDoCommand);
// TODO:       *outPtr++ = '\n';
// TODO:       strcpy(outPtr, langMode);
// TODO:       outPtr += strlen(langMode);
// TODO:       *outPtr++ = '\n';
// TODO:       strcpy(outPtr, geometry);
// TODO:       outPtr += strlen(geometry);
// TODO:       *outPtr++ = '\n';
// TODO:    }
// TODO: 
// TODO:    *outPtr = '\0';
// TODO:    commandLine->serverRequest = commandString;
// TODO: }
// TODO: 
// TODO: 
// TODO: static void waitUntilRequestProcessed(XtAppContext context,
// TODO:                                       Window rootWindow,
// TODO:                                       char* commandString,
// TODO:                                       Atom serverRequestAtom)
// TODO: {
// TODO:    XtIntervalId timerId;
// TODO:    bool timeOut = false;
// TODO: 
// TODO:    /* Set the NEDIT_SERVER_REQUEST_<user>_<host> property on the root
// TODO:       window to activate the server */
// TODO:    XChangeProperty(TheDisplay, rootWindow, serverRequestAtom, XA_STRING, 8,
// TODO:                    PropModeReplace, (unsigned char*)commandString,
// TODO:                    strlen(commandString));
// TODO: 
// TODO:    /* Set up a timeout proc in case the server is dead.  The standard
// TODO:       selection timeout is probably a good guess at how long to wait
// TODO:       for this style of inter-client communication as well */
// TODO:    timerId = XtAppAddTimeOut(context,
// TODO:                              REQUEST_TIMEOUT,
// TODO:                              (XtTimerCallbackProc)timeOutProc,
// TODO:                              &timeOut);
// TODO:    currentWaitForAtom = serverRequestAtom;
// TODO: 
// TODO:    /* Wait for the property to be deleted to know the request was processed */
// TODO:    while (!timeOut)
// TODO:    {
// TODO:       XEvent event;
// TODO:       const XPropertyEvent* e = (const XPropertyEvent*)&event;
// TODO: 
// TODO:       XtAppNextEvent(context, &event);
// TODO:       if (e->window == rootWindow &&
// TODO:             e->atom == serverRequestAtom &&
// TODO:             e->state == PropertyDelete)
// TODO:          break;
// TODO:       XtDispatchEvent(&event);
// TODO:    }
// TODO: 
// TODO:    /* Exit if the timeout expired. */
// TODO:    if (timeOut)
// TODO:    {
// TODO:       fprintf(stderr, "%s: The server did not respond to the request.\n", APP_NAME);
// TODO:       XtCloseDisplay(TheDisplay);
// TODO:       exit(EXIT_FAILURE);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       XtRemoveTimeOut(timerId);
// TODO:    }
// TODO: }
// TODO: 
// TODO: static void waitUntilFilesOpenedOrClosed(XtAppContext context,
// TODO:       Window rootWindow)
// TODO: {
// TODO:    XtIntervalId timerId;
// TODO:    bool timeOut = false;
// TODO: 
// TODO:    /* Set up a timeout proc so we don't wait forever if the server is dead.
// TODO:       The standard selection timeout is probably a good guess at how
// TODO:       long to wait for this style of inter-client communication as
// TODO:       well */
// TODO:    timerId = XtAppAddTimeOut(context, FILE_OPEN_TIMEOUT,
// TODO:                              (XtTimerCallbackProc)timeOutProc, &timeOut);
// TODO:    currentWaitForAtom = noAtom;
// TODO: 
// TODO:    /* Wait for all of the windows to be opened by server,
// TODO:     * and closed if -wait was supplied */
// TODO:    while (fileListHead.fileList)
// TODO:    {
// TODO:       XEvent event;
// TODO:       const XPropertyEvent* e = (const XPropertyEvent*)&event;
// TODO: 
// TODO:       XtAppNextEvent(context, &event);
// TODO: 
// TODO:       /* Update the fileList and check if all files have been closed. */
// TODO:       if (e->type == PropertyNotify && e->window == rootWindow)
// TODO:       {
// TODO:          FileListEntry* item;
// TODO: 
// TODO:          if (e->state == PropertyDelete)
// TODO:          {
// TODO:             for (item = fileListHead.fileList; item; item = item->next)
// TODO:             {
// TODO:                if (e->atom == item->waitForFileOpenAtom)
// TODO:                {
// TODO:                   /* The 'waitForFileOpen' property is deleted when the file is opened */
// TODO:                   fileListHead.waitForOpenCount--;
// TODO:                   item->waitForFileOpenAtom = None;
// TODO: 
// TODO:                   /* Reset the timer while we wait for all files to be opened. */
// TODO:                   XtRemoveTimeOut(timerId);
// TODO:                   timerId = XtAppAddTimeOut(context, FILE_OPEN_TIMEOUT,
// TODO:                                             (XtTimerCallbackProc)timeOutProc, &timeOut);
// TODO:                }
// TODO:                else if (e->atom == item->waitForFileClosedAtom)
// TODO:                {
// TODO:                   /* When file is opened in -wait mode the property
// TODO:                    * is deleted when the file is closed.
// TODO:                    */
// TODO:                   fileListHead.waitForCloseCount--;
// TODO:                   item->waitForFileClosedAtom = None;
// TODO:                }
// TODO:             }
// TODO: 
// TODO:             if (fileListHead.waitForOpenCount == 0 && !timeOut)
// TODO:             {
// TODO:                XtRemoveTimeOut(timerId);
// TODO:             }
// TODO: 
// TODO:             if (fileListHead.waitForOpenCount == 0 &&
// TODO:                   fileListHead.waitForCloseCount == 0)
// TODO:             {
// TODO:                break;
// TODO:             }
// TODO:          }
// TODO:       }
// TODO: 
// TODO:       /* We are finished if we are only waiting for files to open and
// TODO:       ** the file open timeout has expired. */
// TODO:       if (!Preferences.waitForClose && timeOut)
// TODO:       {
// TODO:          break;
// TODO:       }
// TODO: 
// TODO:       XtDispatchEvent(&event);
// TODO:    }
// TODO: }
// TODO: 
// TODO: 
// TODO: static void nextArg(int argc, char** argv, int* argIndex)
// TODO: {
// TODO:    if (*argIndex + 1 >= argc)
// TODO:    {
// TODO: #ifdef VMS
// TODO:       *argv[*argIndex] = '/';
// TODO: #endif /*VMS*/
// TODO:       fprintf(stderr, "nc: %s requires an argument\n%s",
// TODO:               argv[*argIndex], cmdLineHelp);
// TODO:       exit(EXIT_FAILURE);
// TODO:    }
// TODO:    (*argIndex)++;
// TODO: }
// TODO: 
// TODO: /* Copies a given nc command line argument to the server startup command
// TODO: ** line (-icon, -geometry, -xrm, ...) Special characters are protected from
// TODO: ** the shell by escaping EVERYTHING with \
// TODO: ** Note that the .shell string in the command line structure is large enough
// TODO: ** to hold the escaped characters.
// TODO: */
// TODO: static void copyCommandLineArg(CommandLine* commandLine, const char* arg)
// TODO: {
// TODO:    const char* c;
// TODO:    char* outPtr = commandLine->mainWindow + strlen(commandLine->mainWindow);
// TODO: #if defined(VMS) || defined(__EMX__)
// TODO:    /* Non-Unix shells don't want/need esc */
// TODO:    for (c=arg; *c!='\0'; c++)
// TODO:    {
// TODO:       *outPtr++ = *c;
// TODO:    }
// TODO:    *outPtr++ = ' ';
// TODO:    *outPtr = '\0';
// TODO: #else
// TODO:    *outPtr++ = '\'';
// TODO:    for (c=arg; *c!='\0'; c++)
// TODO:    {
// TODO:       if (*c == '\'')
// TODO:       {
// TODO:          *outPtr++ = '\'';
// TODO:          *outPtr++ = '\\';
// TODO:       }
// TODO:       *outPtr++ = *c;
// TODO:       if (*c == '\'')
// TODO:       {
// TODO:          *outPtr++ = '\'';
// TODO:       }
// TODO:    }
// TODO:    *outPtr++ = '\'';
// TODO:    *outPtr++ = ' ';
// TODO:    *outPtr = '\0';
// TODO: #endif /* VMS */
// TODO: }
// TODO: 
// TODO: /* Print version of 'nc' */
// TODO: static void printNcVersion()
// TODO: {
// TODO:    static const char* const ncHelpText = \
// TODO:                                          "nc (NEdit) Version 5.5 (October 2004)\n\n\
// TODO:      Built on: %s, %s, %s\n\
// TODO:      Built at: %s, %s\n";
// TODO: 
// TODO:    fprintf(stdout, ncHelpText,
// TODO:            COMPILE_OS, COMPILE_MACHINE, COMPILE_COMPILER,
// TODO:            __DATE__, __TIME__);
// TODO: }
