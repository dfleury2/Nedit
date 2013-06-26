static const char CVSID[] = "$Id: windowTitle.c,v 1.16 2007/12/31 11:12:44 yooden Exp $";
/*******************************************************************************
*                                                                              *
* windowTitle.c -- Nirvana Editor window title customization                   *
*                                                                              *
* Copyright (C) 2001, Arne Forlie                                              *
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
* Written by Arne Forlie, http://arne.forlie.com                               *
*                                                                              *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "windowTitle.h"
#include "Ne_Text_Buffer.h"
#include "nedit.h"
#include "preferences.h"
#include "help.h"
#include "window.h"
#include "../util/prefFile.h"
#include "../util/misc.h"
#include "../util/DialogF.h"
#include "../util/utils.h"
#include "../util/fileUtils.h"

#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Int_Input.H>
#include <FL/fl_ask.H>

#include <string>

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifndef WIN32
#include <sys/param.h>
#endif

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif


#define WINDOWTITLE_MAX_LEN 500

// Customize window title dialog information
static struct CustomizeWindowTitleDialog
{
   WindowInfo*    window;
   Fl_Window*     form;
   Fl_Input*      previewW;
   Fl_Input*      formatW;

   Fl_Check_Button* filenameW;
   Fl_Check_Button* fileStatusW;
   Fl_Check_Button* fileShortStatusW;

   Fl_Check_Button* hostnameW;
   Fl_Check_Button* userNameW;
   Fl_Check_Button* neditServerW;

   Fl_Check_Button* dirnameW;
   Fl_Int_Input* ndirW;


   Fl_Check_Button* oFileChangedW;
   Fl_Check_Button* oFileLockedW;
   Fl_Check_Button* oFileReadOnlyW;
   Fl_Check_Button* oServerNameW;
   Fl_Check_Button* oDirW;

   std::string	filename;
   std::string path;
   std::string serverName;
   bool        isServer;
   bool        filenameSet;
   int         lockReasons;
   bool        fileChanged;

   bool suppressFormatUpdate;
} etDialog = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
             NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
             "","","",false,false,0,false,false
            };


static char* removeSequence(char* sourcePtr, char c)
{
   while (*sourcePtr == c)
   {
      sourcePtr++;
   }
   return(sourcePtr);
}


/*
** Two functions for performing safe insertions into a finite
** size buffer so that we don't get any memory overruns.
*/
static char* safeStrCpy(char* dest, char* destEnd, const char* source)
{
   int len = (int)strlen(source);
   if (len <= (destEnd - dest))
   {
      strcpy(dest, source);
      return(dest + len);
   }
   else
   {
      strncpy(dest, source, destEnd - dest);
      *destEnd = '\0';
      return(destEnd);
   }
}

static char* safeCharAdd(char* dest, char* destEnd, char c)
{
   if (destEnd - dest > 0)
   {
      *dest++ = c;
      *dest = '\0';
   }
   return(dest);
}

/*
** Remove empty paranthesis pairs and multiple spaces in a row
** with one space.
** Also remove leading and trailing spaces and dashes.
*/
static void compressWindowTitle(char* title)
{
   /* Compress the title */
   bool modified;
   do
   {
      char* sourcePtr = title;
      char* destPtr   = sourcePtr;
      char c = *sourcePtr++;

      modified = false;

      /* Remove leading spaces and dashes */
      while (c == ' ' || c == '-')
      {
         c = *sourcePtr++;
      }

      /* Remove empty constructs */
      while (c != '\0')
      {
         switch (c)
         {
            /* remove sequences */
         case ' ':
         case '-':
            sourcePtr = removeSequence(sourcePtr, c);
            *destPtr++ = c; /* leave one */
            break;

            /* remove empty paranthesis pairs */
         case '(':
            if (*sourcePtr == ')')
            {
               modified = true;
               sourcePtr++;
            }
            else *destPtr++ = c;
            sourcePtr = removeSequence(sourcePtr, ' ');
            break;

         case '[':
            if (*sourcePtr == ']')
            {
               modified = true;
               sourcePtr++;
            }
            else *destPtr++ = c;
            sourcePtr = removeSequence(sourcePtr, ' ');
            break;

         case '{':
            if (*sourcePtr == '}')
            {
               modified = true;
               sourcePtr++;
            }
            else *destPtr++ = c;
            sourcePtr = removeSequence(sourcePtr, ' ');
            break;

         default:
            *destPtr++ = c;
            break;
         }
         c = *sourcePtr++;
         *destPtr = '\0';
      }

      /* Remove trailing spaces and dashes */
      while (destPtr-- > title)
      {
         if (*destPtr != ' ' && *destPtr != '-')
            break;
         *destPtr = '\0';
      }
   }
   while (modified == true);
}


/*
** Format the windows title using a printf like formatting string.
** The following flags are recognised:
**  %s    : server name
**  %[n]d : directory, with one optional digit specifying the max number
**          of trailing directory components to display. Skipped components are
**          replaced by an ellipsis (...).
**  %f    : file name
**  %h    : host name
**  %S    : file status
**  %u    : user name
**
**  if the ClearCase view tag and server name are identical, only the first one
**  specified in the formatting string will be displayed.
*/
char* FormatWindowTitle(const char* filename,
                        const char* path,
                        const char* serverName,
                        int isServer,
                        int filenameSet,
                        int lockReasons,
                        int fileChanged,
                        const char* titleFormat)
{
   static char title[WINDOWTITLE_MAX_LEN];
   char* titlePtr = title;
   char* titleEnd = title + WINDOWTITLE_MAX_LEN - 1;


   /* Flags to supress one of these if both are specified and they are identical */
   int serverNameSeen = false;
   int clearCaseViewTagSeen = false;

   bool fileNamePresent = false;
   bool hostNamePresent = false;
   bool userNamePresent = false;
   bool serverNamePresent = false;
   bool fileStatusPresent = false;
   bool dirNamePresent = false;
   int noOfComponents = -1;
   bool shortStatus = false;

   *titlePtr = '\0';  /* always start with an empty string */

   while (*titleFormat != '\0' && titlePtr < titleEnd)
   {
      char c = *titleFormat++;
      if (c == '%')
      {
         c = *titleFormat++;
         if (c == '\0')
         {
            titlePtr = safeCharAdd(titlePtr, titleEnd, '%');
            break;
         }
         switch (c)
         {
         case 's': /* server name */
            serverNamePresent = true;
            if (isServer && serverName[0] != '\0')   /* only applicable for servers */
            {
               titlePtr = safeStrCpy(titlePtr, titleEnd, serverName);
               serverNameSeen = true;
            }
            break;

         case 'd': /* directory without any limit to no. of components */
            dirNamePresent = true;
            if (filenameSet)
            {
               titlePtr = safeStrCpy(titlePtr, titleEnd, path);
            }
            break;

         case '0': /* directory with limited no. of components */
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
            if (*titleFormat == 'd')
            {
               dirNamePresent = true;
               noOfComponents = c - '0';
               titleFormat++; /* delete the argument */

               if (filenameSet)
               {
                  const char* trailingPath = GetTrailingPathComponents(path,
                                             noOfComponents);

                  /* prefix with ellipsis if components were skipped */
                  if (trailingPath > path)
                  {
                     titlePtr = safeStrCpy(titlePtr, titleEnd, "...");
                  }
                  titlePtr = safeStrCpy(titlePtr, titleEnd, trailingPath);
               }
            }
            break;

         case 'f': /* file name */
            fileNamePresent = true;
            titlePtr = safeStrCpy(titlePtr, titleEnd, filename);
            break;

         case 'h': /* host name */
            hostNamePresent = true;
            titlePtr = safeStrCpy(titlePtr, titleEnd, GetNameOfHost());
            break;

         case 'S': /* file status */
            fileStatusPresent = true;
            if (IS_ANY_LOCKED_IGNORING_USER(lockReasons) && fileChanged)
               titlePtr = safeStrCpy(titlePtr, titleEnd, "read only, modified");
            else if (IS_ANY_LOCKED_IGNORING_USER(lockReasons))
               titlePtr = safeStrCpy(titlePtr, titleEnd, "read only");
            else if (IS_USER_LOCKED(lockReasons) && fileChanged)
               titlePtr = safeStrCpy(titlePtr, titleEnd, "locked, modified");
            else if (IS_USER_LOCKED(lockReasons))
               titlePtr = safeStrCpy(titlePtr, titleEnd, "locked");
            else if (fileChanged)
               titlePtr = safeStrCpy(titlePtr, titleEnd, "modified");
            break;

         case 'c': // Clearcase... not supported
            titlePtr = safeStrCpy(titlePtr, titleEnd, "");
            break;

         case 'u': /* user name */
            userNamePresent = true;
            titlePtr = safeStrCpy(titlePtr, titleEnd, NeGetUserName());
            break;

         case '%': /* escaped % */
            titlePtr = safeCharAdd(titlePtr, titleEnd, '%');
            break;

         case '*': /* short file status ? */
            fileStatusPresent = true;
            if (*titleFormat && *titleFormat == 'S')
            {
               ++titleFormat;
               shortStatus = true;
               if (IS_ANY_LOCKED_IGNORING_USER(lockReasons) && fileChanged)
                  titlePtr = safeStrCpy(titlePtr, titleEnd, "RO*");
               else if (IS_ANY_LOCKED_IGNORING_USER(lockReasons))
                  titlePtr = safeStrCpy(titlePtr, titleEnd, "RO");
               else if (IS_USER_LOCKED(lockReasons) && fileChanged)
                  titlePtr = safeStrCpy(titlePtr, titleEnd, "LO*");
               else if (IS_USER_LOCKED(lockReasons))
                  titlePtr = safeStrCpy(titlePtr, titleEnd, "LO");
               else if (fileChanged)
                  titlePtr = safeStrCpy(titlePtr, titleEnd, "*");
               break;
            }
            /* fall-through */
         default:
            titlePtr = safeCharAdd(titlePtr, titleEnd, c);
            break;
         }
      }
      else
      {
         titlePtr = safeCharAdd(titlePtr, titleEnd, c);
      }
   }

   compressWindowTitle(title);

   if (title[0] == 0)
   {
      sprintf(&title[0], "<empty>"); /* For preview purposes only */
   }

   if (etDialog.form)
   {
      // Prevent recursive callback loop
      etDialog.suppressFormatUpdate = true;

      // Sync radio buttons with format string (in case the user entered the format manually)
      NeToggleButtonSetState(etDialog.filenameW,   fileNamePresent,   false);
      NeToggleButtonSetState(etDialog.fileStatusW, fileStatusPresent, false);
      NeToggleButtonSetState(etDialog.neditServerW, serverNamePresent, false);
      NeToggleButtonSetState(etDialog.dirnameW,    dirNamePresent,    false);
      NeToggleButtonSetState(etDialog.hostnameW,   hostNamePresent,   false);
      NeToggleButtonSetState(etDialog.userNameW,   userNamePresent,   false);

      NeSetSensitive(etDialog.fileShortStatusW,    fileStatusPresent);
      if (fileStatusPresent)
      {
         NeToggleButtonSetState(etDialog.fileShortStatusW, shortStatus, false);
      }

      // Directory components are also sensitive to presence of dir
      NeSetSensitive(etDialog.ndirW,    dirNamePresent);

      if (dirNamePresent) // Avoid erasing number when not active
      {
         if (noOfComponents >= 0)
         {
            char* value = NeTextGetString(etDialog.ndirW);
            char buf[2];
            sprintf(&buf[0], "%d", noOfComponents);
            if (strcmp(&buf[0], value)) // Don't overwrite unless diff.
               SetIntText(etDialog.ndirW, noOfComponents);
            delete[] value;
         }
         else
         {
            NeTextSetString(etDialog.ndirW, "");
         }
      }

      // Enable/disable test buttons, depending on presence of codes
      NeSetSensitive(etDialog.oFileChangedW,  fileStatusPresent);
      NeSetSensitive(etDialog.oFileReadOnlyW, fileStatusPresent);
      NeSetSensitive(etDialog.oFileLockedW,   fileStatusPresent && !IS_PERM_LOCKED(etDialog.lockReasons));

      NeSetSensitive(etDialog.oServerNameW, serverNamePresent);

      NeSetSensitive(etDialog.oDirW,    dirNamePresent);

      etDialog.suppressFormatUpdate = false;
   }

   return(title);
}

// a utility that sets the values of all toggle buttons
static void setToggleButtons()
{
   NeToggleButtonSetState(etDialog.oDirW, etDialog.filenameSet == true, false);
   NeToggleButtonSetState(etDialog.oFileChangedW, etDialog.fileChanged == true, false);
   NeToggleButtonSetState(etDialog.oFileReadOnlyW, IS_PERM_LOCKED(etDialog.lockReasons), false);
   NeToggleButtonSetState(etDialog.oFileLockedW, IS_USER_LOCKED(etDialog.lockReasons), false);
   // Read-only takes precedence on locked
   NeSetSensitive(etDialog.oFileLockedW, !IS_PERM_LOCKED(etDialog.lockReasons));

   NeToggleButtonSetState(etDialog.oServerNameW, etDialog.isServer, false);
}

static void formatChangedCB(Fl_Widget* w, void* data)
{
   TRACE();
   int  filenameSet = NeToggleButtonGetState(etDialog.oDirW);

   if (etDialog.suppressFormatUpdate)
      return; /* Prevent recursive feedback */

   char* format = NeTextGetString(etDialog.formatW);

   std::string serverName = NeToggleButtonGetState(etDialog.oServerNameW) ? etDialog.serverName : "";

   char* title = FormatWindowTitle(
      etDialog.filename.c_str(),
      etDialog.filenameSet == true ? etDialog.path.c_str() : "/a/very/long/path/used/as/example/",
      serverName.c_str(),
      etDialog.isServer,
      filenameSet,
      etDialog.lockReasons,
      NeToggleButtonGetState(etDialog.oFileChangedW),
      format);
   delete[] format;

   etDialog.previewW->value(title);
   etDialog.previewW->redraw_label();
}

static void serverNameCB(Fl_Widget* w, void* data)
{
   TRACE();
   etDialog.isServer = NeToggleButtonGetState(dynamic_cast<Fl_Button*>(w));
   formatChangedCB(w, data);
}

static void fileChangedCB(Fl_Widget* w, void* data)
{
   TRACE();
   etDialog.fileChanged = NeToggleButtonGetState(dynamic_cast<Fl_Button*>(w));
   formatChangedCB(w, data);
}

static void fileLockedCB(Fl_Widget* w, void* data)
{
   TRACE();
   SET_USER_LOCKED(etDialog.lockReasons, NeToggleButtonGetState(dynamic_cast<Fl_Button*>(w)));
   formatChangedCB(w, data);
}

static void fileReadOnlyCB(Fl_Widget* w, void* data)
{
   TRACE();
   SET_PERM_LOCKED(etDialog.lockReasons, NeToggleButtonGetState(dynamic_cast<Fl_Button*>(w)));
   formatChangedCB(w, data);
}

static void applyCB(Fl_Widget* w, void* data)
{
   TRACE();
   char* format = NeTextGetString(etDialog.formatW);

   WidgetToMainWindow(w)->hide();

   if (strcmp(format, GetPrefTitleFormat()) != 0)
   {
      SetPrefTitleFormat(format);
   }
   delete[] format;
}

static void closeCB(Fl_Widget* w, void* data)
{
   TRACE();
   WidgetToMainWindow(w)->hide();
}

static void restoreCB(Fl_Widget* w, void* data)
{
   TRACE();
   NeTextSetString(etDialog.formatW, "[%s] %f (%S) - %d");
}

static void helpCB(Fl_Widget* w, void* data)
{
   TRACE();
   Help(HELP_CUSTOM_TITLE_DIALOG);
}

static void wtDestroyCB(Fl_Widget* w, void* data)
{
   TRACE();

   delete etDialog.form;
   etDialog.form = NULL;
}

static void appendToFormat(const char* string)
{
   char* format = NeTextGetString(etDialog.formatW);
   char* buf = new char[strlen(string) + strlen(format) + 1];
   strcpy(buf, format);
   strcat(buf, string);
   NeTextSetString(etDialog.formatW, buf);
   delete[] format;
   delete[] buf;
}

static void removeFromFormat(const char* string)
{
   char* format = NeTextGetString(etDialog.formatW);
   char* pos;

   /* There can be multiple occurences */
   while ((pos = strstr(format, string)))
   {
      /* If the string is preceded or followed by a brace, include
         the brace(s) for removal */
      char* start = pos;
      char* end = pos + strlen(string);
      char post = *end;

      if (post == '}' || post == ')' || post == ']' || post == '>')
      {
         end += 1;
         post = *end;
      }

      if (start > format)
      {
         char pre = *(start-1);
         if (pre == '{' || pre == '(' || pre == '[' || pre == '<')
            start -= 1;
      }
      if (start > format)
      {
         char pre = *(start-1);
         /* If there is a space in front and behind, remove one space
            (there can be more spaces, but in that case it is likely
            that the user entered them manually); also remove trailing
            space */
         if (pre == ' ' && post == ' ')
         {
            end += 1;
         }
         else if (pre == ' ' && post == (char)0)
         {
            /* Remove (1) trailing space */
            start -= 1;
         }
      }

      /* Contract the string: move end to start */
      strcpy(start, end);
   }

   /* Remove leading and trailing space */
   pos = format;
   while (*pos == ' ') ++pos;
   strcpy(format, pos);

   pos = format + strlen(format) - 1;
   while (pos >= format && *pos == ' ')
   {
      --pos;
   }
   *(pos+1) = (char)0;

   NeTextSetString(etDialog.formatW, format);
   delete[] format;
}


static void toggleFileCB(Fl_Widget* w, void* data)
{
   TRACE();
   if (NeToggleButtonGetState(etDialog.filenameW))
      appendToFormat(" %f");
   else
      removeFromFormat("%f");
}

static void toggleServerCB(Fl_Widget* w, void* data)
{
   TRACE();
   if (NeToggleButtonGetState(etDialog.neditServerW))
      appendToFormat(" [%s]");
   else
      removeFromFormat("%s");
}

static void toggleHostCB(Fl_Widget* w, void* data)
{
   TRACE();
   if (NeToggleButtonGetState(etDialog.hostnameW))
      appendToFormat(" [%h]");
   else
      removeFromFormat("%h");
}

static void toggleStatusCB(Fl_Widget* w, void* date)
{
   TRACE();
   if (NeToggleButtonGetState(etDialog.fileStatusW))
   {
      if (NeToggleButtonGetState(etDialog.fileShortStatusW))
         appendToFormat(" (%*S)");
      else
         appendToFormat(" (%S)");
   }
   else
   {
      removeFromFormat("%S");
      removeFromFormat("%*S");
   }
}

static void toggleShortStatusCB(Fl_Widget* w, void* data)
{
   TRACE();
   char* format, *pos;

   if (etDialog.suppressFormatUpdate)
   {
      return;
   }

   format = NeTextGetString(etDialog.formatW);

   if (NeToggleButtonGetState(etDialog.fileShortStatusW))
   {
      /* Find all %S occurrences and replace them by %*S */
      do
      {
         pos = strstr(format, "%S");
         if (pos)
         {
            char* tmp = new char[(strlen(format)+2)*sizeof(char)];
            strncpy(tmp, format, (size_t)(pos-format+1));
            tmp[pos-format+1] = 0;
            strcat(tmp, "*");
            strcat(tmp, pos+1);
            delete[] format;
            format = tmp;
         }
      }
      while (pos);
   }
   else
   {
      /* Replace all %*S occurences by %S */
      do
      {
         pos = strstr(format, "%*S");
         if (pos)
         {
            strcpy(pos+1, pos+2);
         }
      }
      while (pos);
   }

   NeTextSetString(etDialog.formatW, format);
   delete[] format;
}

static void toggleUserCB(Fl_Widget* w, void* data)
{
   TRACE();
   if (NeToggleButtonGetState(etDialog.userNameW))
      appendToFormat(" %u");
   else
      removeFromFormat("%u");
}

static void toggleDirectoryCB(Fl_Widget* w, void* data)
{
   TRACE();
   if (NeToggleButtonGetState(etDialog.dirnameW))
   {
      char buf[20];
      int maxComp;
      char* value = NeTextGetString(etDialog.ndirW);
      if (*value)
      {
         if (sscanf(value, "%d", &maxComp) > 0)
         {
            sprintf(&buf[0], " %%%dd ", maxComp);
         }
         else
         {
            sprintf(&buf[0], " %%d "); /* Should not be necessary */
         }
      }
      else
      {
         sprintf(&buf[0], " %%d ");
      }
      delete[] value;
      appendToFormat(buf);
   }
   else
   {
      int i;
      removeFromFormat("%d");
      for (i=0; i<=9; ++i)
      {
         char buf[20];
         sprintf(&buf[0], "%%%dd", i);
         removeFromFormat(buf);
      }
   }
}

static void enterMaxDirCB(Fl_Widget* w, void* data)
{
   TRACE();
   int maxComp = -1;
   char* format;
   char* value;

   if (etDialog.suppressFormatUpdate)
      return;

   format = NeTextGetString(etDialog.formatW);
   value = NeTextGetString(etDialog.ndirW);

   if (*value)
   {
      if (sscanf(value, "%d", &maxComp) <= 0)
      {
         /* Don't allow non-digits to be entered */
         fl_beep();
         NeTextSetString(etDialog.ndirW, "");
      }
   }

   if (maxComp >= 0)
   {
      char* pos;
      int found = false;
      char insert[2];
      insert[0] = (char)('0' + maxComp);
      insert[1] = (char)0; /* '0' digit and 0 char ! */

      /* Find all %d and %nd occurrences and replace them by the new value */
      do
      {
         int i;
         found = false;
         pos = strstr(format, "%d");
         if (pos)
         {
            char* tmp = new char[(strlen(format)+2)*sizeof(char)];
            strncpy(tmp, format, (size_t)(pos-format+1));
            tmp[pos-format+1] = 0;
            strcat(tmp, &insert[0]);
            strcat(tmp, pos+1);
            delete[] format;
            format = tmp;
            found = true;
         }

         for (i=0; i<=9; ++i)
         {
            char buf[20];
            sprintf(&buf[0], "%%%dd", i);
            if (i != maxComp)
            {
               pos = strstr(format, &buf[0]);
               if (pos)
               {
                  *(pos+1) = insert[0];
                  found = true;
               }
            }
         }
      }
      while (found);
   }
   else
   {
      int found = true;

      /* Replace all %nd occurences by %d */
      do
      {
         int i;
         found = false;
         for (i=0; i<=9; ++i)
         {
            char buf[20];
            char* pos;
            sprintf(&buf[0], "%%%dd", i);
            pos = strstr(format, &buf[0]);
            if (pos)
            {
               strcpy(pos+1, pos+2);
               found = true;
            }
         }
      }
      while (found);
   }

   NeTextSetString(etDialog.formatW, format);
   delete[] format;
   delete[] value;
}

static void createEditTitleDialog()
{
   etDialog.form = new Fl_Window(30, 50, 400, 450, "Customize Window Title");
   etDialog.form->callback(wtDestroyCB);
  
   // Definition form 
   Fl_Group* selectFrame = new Fl_Group(5,30, 390, 190, "Title Definition");
   selectFrame->align(FL_ALIGN_TOP_LEFT);
   selectFrame->box(FL_ENGRAVED_FRAME);

      Fl_Box* selectLbl = new Fl_Box(10, 35, 380, 25, "Select title components to include:  ");
      selectLbl->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

      etDialog.filenameW = new Fl_Check_Button(10, 65, 190, 25, "&File name (%f)");
      etDialog.filenameW->callback(toggleFileCB);

      etDialog.fileStatusW = new Fl_Check_Button(10, 95, 120, 25, "File s&tatus (%S)");
      etDialog.fileStatusW->callback(toggleStatusCB);

      etDialog.fileShortStatusW = new Fl_Check_Button(130, 95, 70, 25, "&brief");
      etDialog.fileShortStatusW->callback(toggleShortStatusCB);

      etDialog.dirnameW = new Fl_Check_Button(10, 155, 120, 25, "&Directory (%d),");
      etDialog.dirnameW->callback(toggleDirectoryCB);
      etDialog.ndirW = new Fl_Int_Input(250, 155, 50, 25, "ma&x. components: ");
      etDialog.ndirW->callback(enterMaxDirCB);


      etDialog.hostnameW = new Fl_Check_Button(200, 65, 190, 25,"&Host name (%h)");
      etDialog.hostnameW->callback(toggleHostCB);

      etDialog.userNameW = new Fl_Check_Button(200, 95, 190, 25, "&User name (%u)");
      etDialog.userNameW->callback(toggleUserCB);

      etDialog.neditServerW = new Fl_Check_Button(200, 125, 190, 25, "NEdit &server name (%s)");
      etDialog.neditServerW->callback(toggleServerCB);

      etDialog.formatW = new Fl_Input(60, 185, 330, 25, "Fo&rmat:");
      etDialog.formatW->callback(formatChangedCB);
      etDialog.formatW->when(FL_WHEN_CHANGED);
   selectFrame->end();

   Fl_Group* previewFrame = new Fl_Group(5, 240, 390, 160, "Preview");
   previewFrame->align(FL_ALIGN_TOP_LEFT);
   previewFrame->box(FL_ENGRAVED_FRAME);

      etDialog.previewW = new Fl_Input(10, 250, 380, 25);
      etDialog.previewW->deactivate();
      etDialog.previewW->box(FL_ENGRAVED_FRAME);

      Fl_Box* testLbl = new Fl_Box(10, 280, 385, 25, "Test settings:");
      testLbl->align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

      etDialog.oFileChangedW = new Fl_Check_Button(10, 310, 100, 25, "File m&odified");
      etDialog.oFileChangedW->callback(fileChangedCB);

      etDialog.oFileReadOnlyW = new Fl_Check_Button(110, 310, 120, 25, "File read o&nly");
      etDialog.oFileReadOnlyW->callback(fileReadOnlyCB);

      etDialog.oFileLockedW = new Fl_Check_Button(230, 310, 100, 25, "Fi&le locked");
      etDialog.oFileLockedW->callback(fileLockedCB);

      etDialog.oServerNameW = new Fl_Check_Button(10, 340, 385, 25, "Ser&ver name present");
      etDialog.oServerNameW->callback(serverNameCB);

      etDialog.oDirW = new Fl_Check_Button(10, 370, 385, 25, "D&irectory present");
      etDialog.oDirW->callback(formatChangedCB);
   previewFrame->end();

   Fl_Group* resizableFrame = new Fl_Group(0, 401, 400, 1);
   resizableFrame->end();

   Fl_Group* buttonLine = new Fl_Group(0, 405, 400, 45);
   buttonLine->box(FL_ENGRAVED_FRAME);
   
   Fl_Button* btnApply = new Fl_Button(20, 415, 80, 25, "Apply");
   btnApply->shortcut(FL_Enter);
   btnApply->callback(applyCB);

   Fl_Button* btnRestore = new Fl_Button(110, 415, 80, 25, "D&efault");
   btnRestore->callback(restoreCB);

   Fl_Button* btnClose = new Fl_Button(200, 415, 80, 25, "Cancel");
   btnClose->shortcut(FL_Escape);
   btnClose->callback(closeCB);

   Fl_Button* btnHelp = new Fl_Button(290, 415, 80, 25, "Hel&p");
   btnHelp->callback(helpCB);

   buttonLine->end();

   etDialog.form->resizable(resizableFrame);

   etDialog.suppressFormatUpdate = false;
}

void EditCustomTitleFormat(WindowInfo* window)
{
   // copy attributes from current window so that we can use as many
   // 'real world' defaults as possible when testing the effect
   // of different formatting strings.
   etDialog.path = window->path;
   etDialog.filename = window->filename;
   etDialog.serverName = (IsServer ? GetPrefServerName() : "servername");
   etDialog.isServer    = IsServer;
   etDialog.filenameSet = window->filenameSet;
   etDialog.lockReasons = window->lockReasons;
   etDialog.fileChanged = window->fileChanged;

   if (etDialog.window != window && etDialog.form)
   {
      // Destroy the dialog owned by the other window.
      // Note: don't rely on the destroy event handler to reset the
      //       form. Events are handled asynchronously, so the old dialog
      //        may continue to live for a while.
      delete etDialog.form;
      etDialog.form = NULL;
   }

   etDialog.window      = window;

   // Create the dialog if it doesn't already exist
   if (etDialog.form == NULL)
   {
      createEditTitleDialog();
   }

   // set initial value of format field
   NeTextSetString(etDialog.formatW, GetPrefTitleFormat());

   // force update of the dialog
   setToggleButtons();
   formatChangedCB(0, 0);

   // put up dialog and wait for user to press ok or cancel
   ManageDialogCenteredOnPointer(etDialog.form);

   etDialog.form->show();
}
