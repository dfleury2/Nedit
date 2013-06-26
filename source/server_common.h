// TODO: /* $Id: server_common.h,v 1.3 2004/11/09 21:58:44 yooden Exp $ */
// TODO: /*******************************************************************************
// TODO: *									       *
// TODO: * server_common.h -- Nirvana Editor common server stuff			       *
// TODO: *									       *
// TODO: * Copyright (C) 1999 Mark Edel						       *
// TODO: *									       *
// TODO: * This is free__ software; you can redistribute it and/or modify it under the    *
// TODO: * terms of the GNU General Public License as published by the Free Software    *
// TODO: * Foundation; either version 2 of the License, or (at your option) any later   *
// TODO: * version. In addition, you may distribute versions of this program linked to  *
// TODO: * Motif or Open Motif. See README for details.                                 *
// TODO: *                                                                              *
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
// TODO: 
// TODO: #ifndef NEDIT_SERVER_COMMON_H_INCLUDED
// TODO: #define NEDIT_SERVER_COMMON_H_INCLUDED
// TODO: 
// TODO: #include <X11/Intrinsic.h>
// TODO: 
// TODO: /* Lets limit the unique server name to MAXPATHLEN */
// TODO: #define MAXSERVERNAMELEN MAXPATHLEN
// TODO: 
// TODO: #define DEFAULTSERVERNAME ""
// TODO: 
// TODO: void CreateServerPropertyAtoms(const char* serverName,
// TODO:                                Atom* serverExistsAtomReturn,
// TODO:                                Atom* serverRequestAtomReturn);
// TODO: Atom CreateServerFileOpenAtom(const char* serverName,
// TODO:                               const char* path);
// TODO: Atom CreateServerFileClosedAtom(const char* serverName,
// TODO:                                 const char* path,
// TODO:                                 Bool only_if_exists);
// TODO: void DeleteServerFileAtoms(const char* serverName, Window rootWindow);
// TODO: 
// TODO: #endif /* NEDIT_SERVER_COMMON_H_INCLUDED */
