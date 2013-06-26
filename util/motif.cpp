// TODO: /*******************************************************************************
// TODO: *                                                                              *
// TODO: * motif.c:  Determine stability of Motif                                       *
// TODO: *                                                                              *
// TODO: * Copyright (C) 2003 Nathaniel Gray                                            *
// TODO: *                                                                              *
// TODO: * This is free software; you can redistribute it and/or modify it under the    *
// TODO: * terms of the GNU General Public License as published by the Free Software    *
// TODO: * Foundation; either version 2 of the License, or (at your option) any later   *
// TODO: * version. In addition, you may distribute versions of this program linked to  *
// TODO: * Motif or Open Motif. See README for details.                                 *
// TODO: *                                                                              *
// TODO: * This software is distributed in the hope that it will be useful, but WITHOUT *
// TODO: * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
// TODO: * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
// TODO: * for more details.                                                            *
// TODO: *                                                                              *
// TODO: * You should have received a copy of the GNU General Public License along with *
// TODO: * software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
// TODO: * Place, Suite 330, Boston, MA  02111-1307 USA                                 *
// TODO: *                                                                              *
// TODO: * Nirvana Text Editor                                                          *
// TODO: * July 28, 1992                                                                *
// TODO: *                                                                              *
// TODO: * Written by Nathaniel Gray                                                    *
// TODO: *                                                                              *
// TODO: * Modifications by:                                                            *
// TODO: * Scott Tringali                                                               *
// TODO: *                                                                              *
// TODO: *******************************************************************************/
// TODO: 
// TODO: /*
// TODO:  * About the different #defines that Motif gives us:
// TODO:  * All Motifs #define several values.  These are the values in
// TODO:  * Open Motif 2.1.30, for example:
// TODO:  *     #define XmVERSION       2
// TODO:  *     #define XmREVISION      1
// TODO:  *     #define XmUPDATE_LEVEL  30
// TODO:  *     #define XmVersion       (XmVERSION * 1000 + XmREVISION)
// TODO:  *     #define XmVERSION_STRING "@(#)Motif Version 2.1.30"
// TODO:  *
// TODO:  * In addition, LessTif #defines several values as shown here for
// TODO:  * version 0.93.0:
// TODO:  *     #define LESSTIF_VERSION  0
// TODO:  *     #define LESSTIF_REVISION 93
// TODO:  *     #define LesstifVersion   (LESSTIF_VERSION * 1000 + LESSTIF_REVISION)
// TODO:  *     #define LesstifVERSION_STRING \
// TODO:  *             "@(#)GNU/LessTif Version 2.1 Release 0.93.0"
// TODO:  *
// TODO:  * Also, in LessTif the XmVERSION_STRING is identical to the
// TODO:  * LesstifVERSION_STRING.  Unfortunately, the only way to find out the
// TODO:  * "update level" of a LessTif release is to parse the LesstifVERSION_STRING.
// TODO:  */
// TODO: 
// TODO: #include "motif.h"
// TODO: #include <Xm/Xm.h>
// TODO: #include <string.h>
// TODO: 
// TODO: #ifdef LESSTIF_VERSION
// TODO: static enum MotifStability GetLessTifStability();
// TODO: #else
// TODO: static enum MotifStability GetOpenMotifStability();
// TODO: #endif
// TODO: 
// TODO: /*
// TODO:  * These are versions of LessTif that are known to be stable with NEdit in
// TODO:  * Motif 2.1 mode.
// TODO:  */
// TODO: static const char* const knownGoodLesstif[] =
// TODO: {
// TODO:    "0.92.32",
// TODO:    "0.93.0",
// TODO:    "0.93.12",
// TODO:    "0.93.18",
// TODO: #ifndef __x86_64
// TODO:    "0.93.94",    /* 64-bit build .93.94 is broken */
// TODO: #endif
// TODO:    NULL
// TODO: };
// TODO: 
// TODO: /*
// TODO:  * These are versions of LessTif that are known NOT to be stable with NEdit in
// TODO:  * Motif 2.1 mode.
// TODO:  */
// TODO: const char* const knownBadLessTif[] =
// TODO: {
// TODO:    "0.93.25",
// TODO:    "0.93.29",
// TODO:    "0.93.34"
// TODO:    "0.93.36",
// TODO:    "0.93.39",
// TODO:    "0.93.40",
// TODO:    "0.93.41",
// TODO:    "0.93.44",
// TODO: #ifdef __x86_64
// TODO:    "0.93.94",    /* 64-bit build .93.94 is broken */
// TODO: #endif
// TODO:    "0.93.95b",   /* SF bug 1087192 */
// TODO:    "0.94.4",     /* Alt-H, ESC => crash */
// TODO:    "0.95.0",     /* same as above */
// TODO:    NULL
// TODO: };
// TODO: 
// TODO: 
// TODO: #ifdef LESSTIF_VERSION
// TODO: 
// TODO: static enum MotifStability GetLessTifStability()
// TODO: {
// TODO:    int i;
// TODO:    const char* rev = NULL;
// TODO: 
// TODO:    /* We assume that the lesstif version is the string after the last
// TODO:        space. */
// TODO: 
// TODO:    rev = strrchr(LesstifVERSION_STRING, ' ');
// TODO: 
// TODO:    if (rev == NULL)
// TODO:       return MotifUnknown;
// TODO: 
// TODO:    rev += 1;
// TODO: 
// TODO:    /* Check for known good LessTif versions */
// TODO:    for (i = 0; knownGoodLesstif[i]; i++)
// TODO:       if (!strcmp(rev, knownGoodLesstif[i]))
// TODO:          return MotifKnownGood;
// TODO: 
// TODO:    /* Check for known bad LessTif versions */
// TODO:    for (i = 0; knownBadLessTif[i]; i++)
// TODO:       if (!strcmp(rev, knownBadLessTif[i]))
// TODO:          return MotifKnownBad;
// TODO: 
// TODO:    return MotifUnknown;
// TODO: }
// TODO: 
// TODO: #else
// TODO: 
// TODO: /* The stability depends on the patch level, so fold it into the
// TODO:    usual XmVersion for easy comparison. */
// TODO: static const int XmFullVersion = (XmVersion * 100 + XmUPDATE_LEVEL);
// TODO: 
// TODO: static enum MotifStability GetOpenMotifStability()
// TODO: {
// TODO:    enum MotifStability result = MotifUnknown;
// TODO: 
// TODO:    const bool really222 =
// TODO:       (strcmp("@(#)Motif Version 2.2.2", XmVERSION_STRING) == 0);
// TODO: 
// TODO:    if (XmFullVersion <= 200200)         /* 1.0 - 2.1 are fine */
// TODO:    {
// TODO:       result = MotifKnownGood;
// TODO:    }
// TODO:    else if ((XmFullVersion < 200202) || really222)  /* 2.2.0 - 2.2.2 are bad */
// TODO:    {
// TODO:       result = MotifKnownBad;
// TODO:    }
// TODO:    else if (XmFullVersion >= 200203 && XmFullVersion <= 200300) /* 2.2.3 - 2.3 is good */
// TODO:    {
// TODO:       result = MotifKnownGood;
// TODO:    }
// TODO:    else                            /* Anything else unknown */
// TODO:    {
// TODO:       result = MotifUnknown;
// TODO:    }
// TODO: 
// TODO:    return result;
// TODO: }
// TODO: 
// TODO: #endif
// TODO: 
// TODO: 
// TODO: enum MotifStability GetMotifStability()
// TODO: {
// TODO: #ifdef LESSTIF_VERSION
// TODO:    return GetLessTifStability();
// TODO: #else
// TODO:    return GetOpenMotifStability();
// TODO: #endif
// TODO: }
// TODO: 
// TODO: 
// TODO: const char* GetMotifStableVersions()
// TODO: {
// TODO:    int i;
// TODO:    static char msg[sizeof knownGoodLesstif * 80];
// TODO: 
// TODO:    for (i = 0; knownGoodLesstif[i] != NULL; i++)
// TODO:    {
// TODO:       strcat(msg, knownGoodLesstif[i]);
// TODO:       strcat(msg, "\n");
// TODO:    }
// TODO: 
// TODO:    strcat(msg, "OpenMotif 2.1.30\n");
// TODO:    strcat(msg, "OpenMotif 2.2.3\n");
// TODO:    strcat(msg, "OpenMotif 2.3\n");
// TODO: 
// TODO:    return msg;
// TODO: }
