// TODO: /*******************************************************************************
// TODO: *									       *
// TODO: * server_common.c -- Nirvana Editor common server stuff			       *
// TODO: *									       *
// TODO: * Copyright (C) 1999 Mark Edel						       *
// TODO: *									       *
// TODO: * This is free__ software; you can redistribute it and/or modify it under the    *
// TODO: * terms of the GNU General Public License as published by the Free Software    *
// TODO: * Foundation; either version 2 of the License, or (at your option) any later   *
// TODO: * version. In addition, you may distribute versions of this program linked to  *
// TODO: * Motif or Open Motif. See README for details.                                 *
// TODO: * 									       *
// TODO: * This software is distributed in the hope that it will be useful, but WITHOUT *
// TODO: * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
// TODO: * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for    *
// TODO: * more details.                                                                *
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
// TODO: #include <stdio.h>
// TODO: #include <Xm/Xm.h>
// TODO: #include <sys/types.h>
// TODO: #ifdef VMS
// TODO: #include "../util/VMSparam.h"
// TODO: #else
// TODO: #ifndef WIN32
// TODO: #include <sys/param.h>
// TODO: #endif
// TODO: #endif /*VMS*/
// TODO: #include "nedit.h"
// TODO: #include "server_common.h"
// TODO: #include "../util/utils.h"
// TODO: 
// TODO: /*
// TODO:  * Create the server property atoms for the server with serverName.
// TODO:  * Atom names are generated as follows:
// TODO:  *
// TODO:  * NEDIT_SERVER_EXISTS_<host_name>_<user>_<server_name>
// TODO:  * NEDIT_SERVER_REQUEST_<host_name>_<user>_<server_name>
// TODO:  *
// TODO:  * <server_name> is the name that can be set by the user to allow
// TODO:  * for multiple servers to run on the same display. <server_name>
// TODO:  * defaults to "" if not supplied by the user.
// TODO:  *
// TODO:  * <user> is the user name of the current user.
// TODO:  */
// TODO: void CreateServerPropertyAtoms(const char* serverName,
// TODO:                                Atom* serverExistsAtomReturn,
// TODO:                                Atom* serverRequestAtomReturn)
// TODO: {
// TODO:    char propName[20+1+MAXNODENAMELEN+1+MAXUSERNAMELEN+1+MAXSERVERNAMELEN];
// TODO:    const char* userName = NeGetUserName();
// TODO:    const char* hostName = GetNameOfHost();
// TODO: 
// TODO:    sprintf(propName, "NEDIT_SERVER_EXISTS_%s_%s_%s", hostName, userName, serverName);
// TODO:    *serverExistsAtomReturn = XInternAtom(TheDisplay, propName, false);
// TODO:    sprintf(propName, "NEDIT_SERVER_REQUEST_%s_%s_%s", hostName, userName, serverName);
// TODO:    *serverRequestAtomReturn = XInternAtom(TheDisplay, propName, false);
// TODO: }
// TODO: 
// TODO: /*
// TODO:  * Create the individual property atoms for each file being
// TODO:  * opened by the server with serverName. This atom is used
// TODO:  * by nc to monitor if the file has been closed.
// TODO:  *
// TODO:  * Atom names are generated as follows:
// TODO:  *
// TODO:  * NEDIT_FILE_<host_name>_<user>_<server_name>_<path>
// TODO:  *
// TODO:  * <server_name> is the name that can be set by the user to allow
// TODO:  * for multiple servers to run on the same display. <server_name>
// TODO:  * defaults to "" if not supplied by the user.
// TODO:  *
// TODO:  * <user> is the user name of the current user.
// TODO:  *
// TODO:  * <path> is the path of the file being edited.
// TODO:  */
// TODO: Atom CreateServerFileOpenAtom(const char* serverName,
// TODO:                               const char* path)
// TODO: {
// TODO:    char propName[10+1+MAXNODENAMELEN+1+MAXUSERNAMELEN+1+MAXSERVERNAMELEN+1+MAXPATHLEN+1+7];
// TODO:    const char* userName = NeGetUserName();
// TODO:    const char* hostName = GetNameOfHost();
// TODO:    Atom        atom;
// TODO: 
// TODO:    sprintf(propName, "NEDIT_FILE_%s_%s_%s_%s_WF_OPEN", hostName, userName, serverName, path);
// TODO:    atom = XInternAtom(TheDisplay, propName, false);
// TODO:    return(atom);
// TODO: }
// TODO: 
// TODO: Atom CreateServerFileClosedAtom(const char* serverName,
// TODO:                                 const char* path,
// TODO:                                 Bool only_if_exist)
// TODO: {
// TODO:    char propName[10+1+MAXNODENAMELEN+1+MAXUSERNAMELEN+1+MAXSERVERNAMELEN+1+MAXPATHLEN+1+9];
// TODO:    const char* userName = NeGetUserName();
// TODO:    const char* hostName = GetNameOfHost();
// TODO:    Atom        atom;
// TODO: 
// TODO:    sprintf(propName, "NEDIT_FILE_%s_%s_%s_%s_WF_CLOSED", hostName, userName, serverName, path);
// TODO:    atom = XInternAtom(TheDisplay, propName, only_if_exist);
// TODO:    return(atom);
// TODO: }
// TODO: 
// TODO: /*
// TODO:  * Delete all File atoms that belong to this server (as specified by
// TODO:  * <host_name>_<user>_<server_name>).
// TODO:  */
// TODO: void DeleteServerFileAtoms(const char* serverName, Window rootWindow)
// TODO: {
// TODO:    char propNamePrefix[10+1+MAXNODENAMELEN+1+MAXUSERNAMELEN+1+MAXSERVERNAMELEN+1];
// TODO:    const char* userName = NeGetUserName();
// TODO:    const char* hostName = GetNameOfHost();
// TODO:    int length = sprintf(propNamePrefix, "NEDIT_FILE_%s_%s_%s_", hostName, userName, serverName);
// TODO: 
// TODO:    int nProperties;
// TODO:    Atom* atoms = XListProperties(TheDisplay, rootWindow, &nProperties);
// TODO:    if (atoms != NULL)
// TODO:    {
// TODO:       int i;
// TODO:       for (i = 0; i < nProperties; i++)
// TODO:       {
// TODO:          /* XGetAtomNames() is more efficient, but doesn't exist in X11R5. */
// TODO:          char* name = XGetAtomName(TheDisplay, atoms[i]);
// TODO:          if (name != NULL && strncmp(propNamePrefix, name, length) == 0)
// TODO:          {
// TODO:             XDeleteProperty(TheDisplay, rootWindow, atoms[i]);
// TODO:          }
// TODO:          XFree(name);
// TODO:       }
// TODO:       XFree((char*)atoms);
// TODO:    }
// TODO: }
