static const char CVSID[] = "$Id: server.c,v 1.35 2010/07/05 06:23:59 lebert Exp $";
/*******************************************************************************
*									       *
* server.c -- Nirvana Editor edit-server component			       *
*									       *
* Copyright (C) 1999 Mark Edel						       *
*									       *
* This is free__ software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version. In addition, you may distribute version of this program linked to   *
* Motif or Open Motif. See README for details.                                 *
* 									       *
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
* November, 1995							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "server.h"
#include "Ne_Text_Buffer.h"
#include "nedit.h"
#include "window.h"
#include "file.h"
#include "selection.h"
#include "macro.h"
#include "menu.h"
#include "preferences.h"
#include "server_common.h"
#include "../util/fileUtils.h"
#include "../util/utils.h"
#include "../util/misc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/param.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <pwd.h>
#endif

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif


// TODO: static void processServerCommand();
static void cleanUpServerCommunication();
// TODO: static void processServerCommandString(char* string);
// TODO: static void getFileClosedProperty(WindowInfo* window);
// TODO: static int isLocatedOnDesktop(WindowInfo* window, long currentDesktop);
// TODO: static WindowInfo* findWindowOnDesktop(int tabbed, long currentDesktop);
// TODO: 
// TODO: static Atom ServerRequestAtom = 0;
// TODO: static Atom ServerExistsAtom = 0;

/*
** Set up inter-client communication for NEdit server end, expected to be
** called only once at startup time
*/
void InitServerCommunication()
{
// TODO:    Window rootWindow = RootWindow(TheDisplay, DefaultScreen(TheDisplay));
// TODO: 
// TODO:    /* Create the server property atoms on the current DISPLAY. */
// TODO:    CreateServerPropertyAtoms(GetPrefServerName(),
// TODO:                              &ServerExistsAtom,
// TODO:                              &ServerRequestAtom);
// TODO: 
// TODO:    /* Pay attention to PropertyChangeNotify events on the root window.
// TODO:       Do this before putting up the server atoms, to avoid a race
// TODO:       condition (when nc sees that the server exists, it sends a command,
// TODO:       so we must make sure that we already monitor properties). */
// TODO:    XSelectInput(TheDisplay, rootWindow, PropertyChangeMask);
// TODO: 
// TODO:    /* Create the server-exists property on the root window to tell clients
// TODO:       whether to try a request (otherwise clients would always have to
// TODO:       try and wait for their timeouts to expire) */
// TODO:    XChangeProperty(TheDisplay, rootWindow, ServerExistsAtom, XA_STRING, 8,
// TODO:                    PropModeReplace, (unsigned char*)"true", 4);

   /* Set up exit handler for cleaning up server-exists property */
   atexit(cleanUpServerCommunication);
}

static void deleteProperty(Atom* atom)
{
// TODO:    if (!IsServer)
// TODO:    {
// TODO:       *atom = None;
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    if (*atom != None)
// TODO:    {
// TODO:       XDeleteProperty(TheDisplay, RootWindow(TheDisplay, DefaultScreen(TheDisplay)), *atom);
// TODO:       *atom = None;
// TODO:    }
}

/*
** Exit handler.  Removes server-exists property on root window
*/
static void cleanUpServerCommunication()
{
   WindowInfo* w;

   /* Delete any per-file properties that still exist
    * (and that server knows about)
    */
   for (w = WindowList; w; w = w->next)
   {
      DeleteFileClosedProperty(w);
   }

// TODO:    /* Delete any per-file properties that still exist
// TODO:     * (but that that server doesn't know about)
// TODO:     */
// TODO:    DeleteServerFileAtoms(GetPrefServerName(), RootWindow(TheDisplay, DefaultScreen(TheDisplay)));
// TODO: 
// TODO:    /* Delete the server-exists property from the root window (if it was
// TODO:       assigned) and don't let the process exit until the X server has
// TODO:       processed the delete request (otherwise it won't be done) */
// TODO:    deleteProperty(&ServerExistsAtom);
// TODO:    XSync(TheDisplay, false);
}

/*
** Special event loop for NEdit servers.  Processes PropertyNotify events on
** the root window (this would not be necessary if it were possible to
** register an Xt event-handler for a window, rather than only a widget).
** Invokes server routines when a server-request property appears,
** re-establishes server-exists property when another server exits and
** this server is still alive to take over.
*/
void ServerMainLoop(Ne_AppContext& context)
{
   while (true)
   {
// TODO:       XEvent event;
// TODO:       XtAppNextEvent(context, &event);
// TODO:       ServerDispatchEvent(&event);
   }
}

// TODO: static void processServerCommand()
// TODO: {
// TODO:    Atom dummyAtom;
// TODO:    unsigned long nItems, dummyULong;
// TODO:    unsigned char* propValue;
// TODO:    int getFmt;
// TODO: 
// TODO:    /* Get the value of the property, and delete it from the root window */
// TODO:    if (XGetWindowProperty(TheDisplay, RootWindow(TheDisplay,
// TODO:                           DefaultScreen(TheDisplay)), ServerRequestAtom, 0, INT_MAX, true,
// TODO:                           XA_STRING, &dummyAtom, &getFmt, &nItems, &dummyULong, &propValue)
// TODO:          != Success || getFmt != 8)
// TODO:       return;
// TODO: 
// TODO:    /* Invoke the command line processor on the string to process the request */
// TODO:    processServerCommandString((char*)propValue);
// TODO:    XFree(propValue);
// TODO: }
// TODO: 
// TODO: bool ServerDispatchEvent(XEvent* event)
// TODO: {
// TODO:    if (IsServer)
// TODO:    {
// TODO:       Window rootWindow = RootWindow(TheDisplay, DefaultScreen(TheDisplay));
// TODO:       if (event->xany.window == rootWindow && event->xany.type == PropertyNotify)
// TODO:       {
// TODO:          const XPropertyEvent* e = &event->xproperty;
// TODO: 
// TODO:          if (e->type == PropertyNotify && e->window == rootWindow)
// TODO:          {
// TODO:             if (e->atom == ServerRequestAtom && e->state == PropertyNewValue)
// TODO:                processServerCommand();
// TODO:             else if (e->atom == ServerExistsAtom && e->state == PropertyDelete)
// TODO:                XChangeProperty(TheDisplay,
// TODO:                                rootWindow,
// TODO:                                ServerExistsAtom, XA_STRING,
// TODO:                                8, PropModeReplace,
// TODO:                                (unsigned char*)"true", 4);
// TODO:          }
// TODO:       }
// TODO:    }
// TODO:    return XtDispatchEvent(event);
// TODO: }
// TODO: 
// TODO: /* Try to find existing 'FileOpen' property atom for path. */
// TODO: static Atom findFileOpenProperty(const char* filename,
// TODO:                                  const char* pathname)
// TODO: {
// TODO:    char path[MAXPATHLEN];
// TODO:    Atom atom;
// TODO: 
// TODO:    if (!IsServer) return(None);
// TODO: 
// TODO:    strcpy(path, pathname);
// TODO:    strcat(path, filename);
// TODO:    atom = CreateServerFileOpenAtom(GetPrefServerName(), path);
// TODO:    return(atom);
// TODO: }
// TODO: 
// TODO: /* Destroy the 'FileOpen' atom to inform nc that this file has
// TODO: ** been opened.
// TODO: */
// TODO: static void deleteFileOpenProperty(WindowInfo* window)
// TODO: {
// TODO:    if (window->filenameSet)
// TODO:    {
// TODO:       Atom atom = findFileOpenProperty(window->filename, window->path);
// TODO:       deleteProperty(&atom);
// TODO:    }
// TODO: }
// TODO: 
// TODO: static void deleteFileOpenProperty2(const char* filename,
// TODO:                                     const char* pathname)
// TODO: {
// TODO:    Atom atom = findFileOpenProperty(filename, pathname);
// TODO:    deleteProperty(&atom);
// TODO: }
// TODO: 
// TODO: 
// TODO: 
// TODO: /* Try to find existing 'FileClosed' property atom for path. */
// TODO: static Atom findFileClosedProperty(const char* filename,
// TODO:                                    const char* pathname)
// TODO: {
// TODO:    char path[MAXPATHLEN];
// TODO:    Atom atom;
// TODO: 
// TODO:    if (!IsServer) return(None);
// TODO: 
// TODO:    strcpy(path, pathname);
// TODO:    strcat(path, filename);
// TODO:    atom = CreateServerFileClosedAtom(GetPrefServerName(),
// TODO:                                      path,
// TODO:                                      true); /* don't create */
// TODO:    return(atom);
// TODO: }
// TODO: 
// TODO: /* Get hold of the property to use when closing the file. */
// TODO: static void getFileClosedProperty(WindowInfo* window)
// TODO: {
// TODO:    if (window->filenameSet)
// TODO:    {
// TODO:       window->fileClosedAtom = findFileClosedProperty(window->filename,
// TODO:                                window->path);
// TODO:    }
// TODO: }

/* Delete the 'FileClosed' atom to inform nc that this file has
** been closed.
*/
void DeleteFileClosedProperty(WindowInfo* window)
{
   if (window->filenameSet)
   {
      deleteProperty(&window->fileClosedAtom);
   }
}

// TODO: static void deleteFileClosedProperty2(const char* filename,
// TODO:                                       const char* pathname)
// TODO: {
// TODO:    Atom atom = findFileClosedProperty(filename, pathname);
// TODO:    deleteProperty(&atom);
// TODO: }
// TODO: 
// TODO: static int isLocatedOnDesktop(WindowInfo* window, long currentDesktop)
// TODO: {
// TODO:    long windowDesktop;
// TODO:    if (currentDesktop == -1)
// TODO:       return true; /* No desktop information available */
// TODO: 
// TODO:    windowDesktop = QueryDesktop(TheDisplay, window->mainWindow);
// TODO:    /* Sticky windows have desktop 0xFFFFFFFF by convention */
// TODO:    if (windowDesktop == currentDesktop || windowDesktop == 0xFFFFFFFFL)
// TODO:       return true; /* Desktop matches, or window is sticky */
// TODO: 
// TODO:    return false;
// TODO: }
// TODO: 
// TODO: static WindowInfo* findWindowOnDesktop(int tabbed, long currentDesktop)
// TODO: {
// TODO:    WindowInfo* window;
// TODO: 
// TODO:    if (tabbed == 0 || (tabbed == -1 && GetPrefOpenInTab() == 0))
// TODO:    {
// TODO:       /* A new window is requested, unless we find an untitled unmodified
// TODO:           document on the current desktop */
// TODO:       for (window=WindowList; window!=NULL; window=window->next)
// TODO:       {
// TODO:          if (window->filenameSet || window->fileChanged ||
// TODO:                window->macroCmdData != NULL)
// TODO:          {
// TODO:             continue;
// TODO:          }
// TODO:          /* No check for top document here! */
// TODO:          if (isLocatedOnDesktop(window, currentDesktop))
// TODO:          {
// TODO:             return window;
// TODO:          }
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       /* Find a window on the current desktop to hold the new document */
// TODO:       for (window=WindowList; window!=NULL; window=window->next)
// TODO:       {
// TODO:          /* Avoid unnecessary property access (server round-trip) */
// TODO:          if (!IsTopDocument(window))
// TODO:          {
// TODO:             continue;
// TODO:          }
// TODO:          if (isLocatedOnDesktop(window, currentDesktop))
// TODO:          {
// TODO:             return window;
// TODO:          }
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    return NULL; /* No window found on current desktop -> create new window */
// TODO: }
// TODO: 
// TODO: static void processServerCommandString(char* string)
// TODO: {
// TODO:    char* fullname, filename[MAXPATHLEN], pathname[MAXPATHLEN];
// TODO:    char* doCommand, *geometry, *langMode, *inPtr;
// TODO:    int editFlags, stringLen = strlen(string);
// TODO:    int lineNum, createFlag, readFlag, iconicFlag, lastIconic = 0, tabbed = -1;
// TODO:    int fileLen, doLen, lmLen, geomLen, charsRead, itemsRead;
// TODO:    WindowInfo* window, *lastFile = NULL;
// TODO:    long currentDesktop = QueryCurrentDesktop(TheDisplay,
// TODO:                          RootWindow(TheDisplay, DefaultScreen(TheDisplay)));
// TODO: 
// TODO:    /* If the command string is empty, put up an empty, Untitled window
// TODO:       (or just pop one up if it already exists) */
// TODO:    if (string[0] == '\0')
// TODO:    {
// TODO:       for (window=WindowList; window!=NULL; window=window->next)
// TODO:          if (!window->filenameSet && !window->fileChanged &&
// TODO:                isLocatedOnDesktop(window, currentDesktop))
// TODO:             break;
// TODO:       if (window == NULL)
// TODO:       {
// TODO:          EditNewFile(findWindowOnDesktop(tabbed, currentDesktop), NULL,
// TODO:                      false, NULL, NULL);
// TODO:          CheckCloseDim();
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          RaiseDocument(window);
// TODO:          WmClientMsg(TheDisplay, XtWindow(window->mainWindow),
// TODO:                      "_NET_ACTIVE_WINDOW", 0, 0, 0, 0, 0);
// TODO:          XMapRaised(TheDisplay, XtWindow(window->mainWindow));
// TODO:       }
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /*
// TODO:    ** Loop over all of the files in the command list
// TODO:    */
// TODO:    inPtr = string;
// TODO:    while (TRUE)
// TODO:    {
// TODO: 
// TODO:       if (*inPtr == '\0')
// TODO:          break;
// TODO: 
// TODO:       /* Read a server command from the input string.  Header contains:
// TODO:          linenum createFlag fileLen doLen\n, followed by a filename and -do
// TODO:          command both followed by newlines.  This bit of code reads the
// TODO:          header, and converts the newlines following the filename and do
// TODO:          command to nulls to terminate the filename and doCommand strings */
// TODO:       itemsRead = sscanf(inPtr, "%d %d %d %d %d %d %d %d %d%n", &lineNum,
// TODO:                          &readFlag, &createFlag, &iconicFlag, &tabbed, &fileLen,
// TODO:                          &doLen, &lmLen, &geomLen, &charsRead);
// TODO:       if (itemsRead != 9)
// TODO:          goto readError;
// TODO:       inPtr += charsRead + 1;
// TODO:       if (inPtr - string + fileLen > stringLen)
// TODO:          goto readError;
// TODO:       fullname = inPtr;
// TODO:       inPtr += fileLen;
// TODO:       *inPtr++ = '\0';
// TODO:       if (inPtr - string + doLen > stringLen)
// TODO:          goto readError;
// TODO:       doCommand = inPtr;
// TODO:       inPtr += doLen;
// TODO:       *inPtr++ = '\0';
// TODO:       if (inPtr - string + lmLen > stringLen)
// TODO:          goto readError;
// TODO:       langMode = inPtr;
// TODO:       inPtr += lmLen;
// TODO:       *inPtr++ = '\0';
// TODO:       if (inPtr - string + geomLen > stringLen)
// TODO:          goto readError;
// TODO:       geometry = inPtr;
// TODO:       inPtr += geomLen;
// TODO:       *inPtr++ = '\0';
// TODO: 
// TODO:       /* An empty file name means:
// TODO:        *   put up an empty, Untitled window, or use an existing one
// TODO:        *   choose a random window for executing the -do macro upon
// TODO:        */
// TODO:       if (fileLen <= 0)
// TODO:       {
// TODO:          for (window=WindowList; window!=NULL; window=window->next)
// TODO:             if (!window->filenameSet && !window->fileChanged &&
// TODO:                   isLocatedOnDesktop(window, currentDesktop))
// TODO:                break;
// TODO: 
// TODO:          if (*doCommand == '\0')
// TODO:          {
// TODO:             if (window == NULL)
// TODO:             {
// TODO:                EditNewFile(findWindowOnDesktop(tabbed, currentDesktop),
// TODO:                            NULL, iconicFlag, lmLen==0?NULL:langMode, NULL);
// TODO:             }
// TODO:             else
// TODO:             {
// TODO:                if (iconicFlag)
// TODO:                   RaiseDocument(window);
// TODO:                else
// TODO:                   RaiseDocumentWindow(window);
// TODO:             }
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             WindowInfo* win = WindowList;
// TODO:             /* Starting a new command while another one is still running
// TODO:                in the same window is not possible (crashes). */
// TODO:             while (win != NULL && win->macroCmdData != NULL)
// TODO:             {
// TODO:                win = win->next;
// TODO:             }
// TODO: 
// TODO:             if (!win)
// TODO:             {
// TODO:                XBell(TheDisplay, 0);
// TODO:             }
// TODO:             else
// TODO:             {
// TODO:                /* Raise before -do (macro could close window). */
// TODO:                if (iconicFlag)
// TODO:                   RaiseDocument(win);
// TODO:                else
// TODO:                   RaiseDocumentWindow(win);
// TODO:                DoMacro(win, doCommand, "-do macro");
// TODO:             }
// TODO:          }
// TODO:          CheckCloseDim();
// TODO:          return;
// TODO:       }
// TODO: 
// TODO:       /* Process the filename by looking for the files in an
// TODO:          existing window, or opening if they don't exist */
// TODO:       editFlags = (readFlag ? PREF_READ_ONLY : 0) | CREATE |
// TODO:                   (createFlag ? SUPPRESS_CREATE_WARN : 0);
// TODO:       if (ParseFilename(fullname, filename, pathname) != 0)
// TODO:       {
// TODO:          fprintf(stderr, "NEdit: invalid file name\n");
// TODO:          deleteFileClosedProperty2(filename, pathname);
// TODO:          break;
// TODO:       }
// TODO: 
// TODO:       window = FindWindowWithFile(filename, pathname);
// TODO:       if (window == NULL)
// TODO:       {
// TODO:          /* Files are opened in background to improve opening speed
// TODO:             by defering certain time  consuiming task such as syntax
// TODO:             highlighting. At the end of the file-opening loop, the
// TODO:             last file opened will be raised to restore those deferred
// TODO:             items. The current file may also be raised if there're
// TODO:             macros to execute on. */
// TODO:          window = EditExistingFile(findWindowOnDesktop(tabbed, currentDesktop),
// TODO:                                    filename, pathname, editFlags, geometry, iconicFlag,
// TODO:                                    lmLen == 0 ? NULL : langMode,
// TODO:                                    tabbed == -1? GetPrefOpenInTab() : tabbed, true);
// TODO: 
// TODO:          if (window)
// TODO:          {
// TODO:             if (lastFile && window->mainWindow != lastFile->mainWindow)
// TODO:             {
// TODO:                RaiseDocument(lastFile);
// TODO:             }
// TODO:          }
// TODO: 
// TODO:       }
// TODO: 
// TODO:       /* Do the actions requested (note DoMacro is last, since the do
// TODO:          command can do anything, including closing the window!) */
// TODO:       if (window != NULL)
// TODO:       {
// TODO:          deleteFileOpenProperty(window);
// TODO:          getFileClosedProperty(window);
// TODO: 
// TODO:          if (lineNum > 0)
// TODO:             SelectNumberedLine(window, lineNum);
// TODO: 
// TODO:          if (*doCommand != '\0')
// TODO:          {
// TODO:             RaiseDocument(window);
// TODO: 
// TODO:             if (!iconicFlag)
// TODO:             {
// TODO:                WmClientMsg(TheDisplay, XtWindow(window->mainWindow),
// TODO:                            "_NET_ACTIVE_WINDOW", 0, 0, 0, 0, 0);
// TODO:                XMapRaised(TheDisplay, XtWindow(window->mainWindow));
// TODO:             }
// TODO: 
// TODO:             /* Starting a new command while another one is still running
// TODO:                in the same window is not possible (crashes). */
// TODO:             if (window->macroCmdData != NULL)
// TODO:             {
// TODO:                XBell(TheDisplay, 0);
// TODO:             }
// TODO:             else
// TODO:             {
// TODO:                DoMacro(window, doCommand, "-do macro");
// TODO:                /* in case window is closed by macro functions
// TODO:                   such as close() or detach_document() */
// TODO:                if (!IsValidWindow(window))
// TODO:                   window = NULL;
// TODO:                if (lastFile && !IsValidWindow(lastFile))
// TODO:                   lastFile = NULL;
// TODO:             }
// TODO:          }
// TODO: 
// TODO:          /* register the last file opened for later use */
// TODO:          if (window)
// TODO:          {
// TODO:             lastFile = window;
// TODO:             lastIconic = iconicFlag;
// TODO:          }
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          deleteFileOpenProperty2(filename, pathname);
// TODO:          deleteFileClosedProperty2(filename, pathname);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* Raise the last file opened */
// TODO:    if (lastFile)
// TODO:    {
// TODO:       if (lastIconic)
// TODO:          RaiseDocument(lastFile);
// TODO:       else
// TODO:          RaiseDocumentWindow(lastFile);
// TODO:       CheckCloseDim();
// TODO:    }
// TODO:    return;
// TODO: 
// TODO: readError:
// TODO:    fprintf(stderr, "NEdit: error processing server request\n");
// TODO:    return;
// TODO: }
