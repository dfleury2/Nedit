#include "file.h"
#include "Ne_Text_Buffer.h"
#include "Ne_Text_Editor.h"
#include "window.h"
#include "preferences.h"
#include "undo.h"
#include "menu.h"
#include "tags.h"
#include "server.h"
#include "interpret.h"
#include "../util/misc.h"
#include "../util/DialogF.h"
#include "../util/fileUtils.h"
#include "../util/getfiles.h"
#include "../util/printUtils.h"
#include "../util/utils.h"

#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_ask.H>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <sys/param.h>
#include <unistd.h>
#else
#include <io.h>
#endif
#include <fcntl.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

/* Maximum frequency in miliseconds of checking for external modifications.
   The periodic check is only performed on buffer modification, and the check
   interval is only to prevent checking on every keystroke in case of a file
   system which is slow to process stat requests (which I'm not sure exists) */
#define MOD_CHECK_INTERVAL 3000

static int doSave(WindowInfo* window);
static void safeClose(WindowInfo* window);
static int doOpen(WindowInfo* window, const char* name, const char* path,int flags);
static void backupFileName(WindowInfo* window, char* name, size_t len);
static int writeBckVersion(WindowInfo* window);
static int bckError(WindowInfo* window, const char* errString, const char* file);
static int fileWasModifiedExternally(WindowInfo* window);
static const char* errorString();
static void addWrapNewlines(WindowInfo* window);
// TODO: static void setFormatCB(Widget w, XtPointer clientData, XtPointer callData);
// TODO: static void addWrapCB(Widget w, XtPointer clientData, XtPointer callData);
static int cmpWinAgainstFile(WindowInfo* window, const char* fileName);
// TODO: static void modifiedWindowDestroyedCB(Widget w, XtPointer clientData,XtPointer callData);
static void forceShowLineNumbers(WindowInfo* window);

WindowInfo* EditNewFile(WindowInfo* inWindow, char* geometry, int iconic, const char* languageMode, const char* defaultPath)
{
   // Find a (relatively) unique name for the new file
   char name[MAXPATHLEN] = "";
   UniqueUntitledName(name);

   // create new window/document
   WindowInfo* window = 0;
   if (inWindow)
      window = CreateDocument(inWindow, name);
   else
      window = CreateNeWindow(name, geometry, iconic);

   char* path = window->path;
   strcpy(window->filename, name);
   strcpy(path, (defaultPath && *defaultPath) ? defaultPath : GetCurrentDir());
   size_t pathlen = strlen(window->path);
   // do we have a "/" at the end? if not, add one
   if (0 < pathlen && path[pathlen - 1] != '/' && pathlen < MAXPATHLEN - 1)
   {
      strcpy(&path[pathlen], "/");
   }
   SetWindowModified(window, false);
   CLEAR_ALL_LOCKS(window->lockReasons);
   UpdateWindowReadOnly(window);
   UpdateStatsLine(window);
   UpdateWindowTitle(window);
   RefreshTabState(window);

   if (languageMode == NULL)
      DetermineLanguageMode(window, true);
   else
      SetLanguageMode(window, FindLanguageMode(languageMode), true);

   ShowTabBar(window, GetShowTabBar(window));

   if (iconic && IsIconic(window))
      RaiseDocument(window);
   else
      RaiseDocumentWindow(window);

   SortTabBar(window);
   return window;
}

/*
** Open an existing file specified by name and path.  Use the window inWindow
** unless inWindow is NULL or points to a window which is already in use
** (displays a file other than Untitled, or is Untitled but modified).  Flags
** can be any of:
**
**	CREATE: 		If file is not found, (optionally) prompt the
**				user whether to create
**	SUPPRESS_CREATE_WARN	When creating a file, don't ask the user
**	PREF_READ_ONLY		Make the file read-only regardless
**
** If languageMode is passed as NULL, it will be determined automatically
** from the file extension or file contents.
**
** If bgOpen is true, then the file will be open in background. This
** works in association with the SetLanguageMode() function that has
** the syntax highlighting deferred, in order to speed up the file-
** opening operation when multiple files are being opened in succession.
*/
WindowInfo* EditExistingFile(WindowInfo* inWindow, const char* name,
                             const char* path, int flags, char* geometry, int iconic,
                             const char* languageMode, int tabbed, bool bgOpen)
{
   char fullname[MAXPATHLEN] = "";

   /* first look to see if file is already displayed in a window */
   WindowInfo* window = FindWindowWithFile(name, path);
   if (window != NULL)
   {
      if (!bgOpen)
      {
         if (iconic)
            window->mainWindow->show();
         else
            RaiseDocumentWindow(window);
      }
      return window;
   }

   /* If an existing window isn't specified; or the window is already
      in use (not Untitled or Untitled and modified), or is currently
      busy running a macro; create the window */
   if (inWindow == NULL)
   {
      window = CreateNeWindow(name, geometry, iconic);
   }
   else if (inWindow->filenameSet || inWindow->fileChanged || inWindow->macroCmdData != NULL)
   {
      if (tabbed)
      {
         window = CreateDocument(inWindow, name);
      }
      else
      {
         window = CreateNeWindow(name, geometry, iconic);
      }
   }
   else
   {
      // open file in untitled document
      window = inWindow;
      strcpy(window->path, path);
      strcpy(window->filename, name);
      if (!iconic && !bgOpen)
      {
         window->mainWindow->show();
      }
   }

   // Open the file
   if (!doOpen(window, name, path, flags))
   {
      /* The user may have destroyed the window instead of closing the
         warning dialog; don't close it twice */
      safeClose(window);

      return NULL;
   }
   forceShowLineNumbers(window);

   /* Decide what language mode to use, trigger language specific actions */
   if (languageMode == NULL)
      DetermineLanguageMode(window, true);
   else
      SetLanguageMode(window, FindLanguageMode(languageMode), true);

   /* update tab label and tooltip */
   RefreshTabState(window);
   SortTabBar(window);
   ShowTabBar(window, GetShowTabBar(window));

   if (!bgOpen)
      RaiseDocument(window);

   // Bring the title bar and statistics line up to date, doOpen does
   // not necessarily set the window title or read-only status
   UpdateWindowTitle(window);
   UpdateWindowReadOnly(window);
   UpdateStatsLine(window);

   /* Add the name to the convenience menu of previously opened files */
   strcpy(fullname, path);
   strcat(fullname, name);
   if (GetPrefAlwaysCheckRelTagsSpecs())
      AddRelTagsFile(GetPrefTagFile(), path, TAG);
   AddToPrevOpenMenu(fullname);

   return window;
}

void RevertToSaved(WindowInfo* window)
{
   char name[MAXPATHLEN], path[MAXPATHLEN];
   int i;
   int insertPositions[MAX_PANES], topLines[MAX_PANES];
   int horizOffsets[MAX_PANES];
   int openFlags = 0;
   Ne_Text_Editor* text;

   /* Can't revert untitled windows */
   if (!window->filenameSet)
   {
      DialogF(DF_WARN, window->mainWindow, 1, "Error",
              "Window '%s' was never saved, can't re-read", "OK",
              window->filename);
      return;
   }

   /* save insert & scroll positions of all of the panes to restore later */
   for (i=0; i<=window->nPanes; i++)
   {
      text = i==0 ? window->textArea : window->textPanes[i-1];
      insertPositions[i] = TextGetCursorPos(text);
      TextGetScroll(text, &topLines[i], &horizOffsets[i]);
   }

   /* re-read the file, update the window title if new file is different */
   strcpy(name, window->filename);
   strcpy(path, window->path);
   RemoveBackupFile(window);
   ClearUndoList(window);
   openFlags |= IS_USER_LOCKED(window->lockReasons) ? NE_PREF_READ_ONLY : 0;
   if (!doOpen(window, name, path, openFlags))
   {
      /* This is a bit sketchy.  The only error in doOpen that irreperably
               damages the window is "too much binary data".  It should be
               pretty rare to be reverting something that was fine only to find
               that now it has too much binary data. */
      if (!window->fileMissing)
         safeClose(window);
      else
      {
         /* Treat it like an externally modified file */
         window->lastModTime=0;
         window->fileMissing=false;
      }
      return;
   }
   forceShowLineNumbers(window);
   UpdateWindowTitle(window);
   UpdateWindowReadOnly(window);

   /* restore the insert and scroll positions of each pane */
   for (i=0; i<=window->nPanes; i++)
   {
      text = i==0 ? window->textArea : window->textPanes[i-1];
      TextSetCursorPos(text, insertPositions[i]);
      TextSetScroll(text, topLines[i], horizOffsets[i]);
   }
}

/*
** Checks whether a window is still alive, and closes it only if so.
** Intended to be used when the file could not be opened for some reason.
** Normally the window is still alive, but the user may have closed the
** window instead of the error dialog. In that case, we shouldn't close the
** window a second time.
*/
static void safeClose(WindowInfo* window)
{
   WindowInfo* p = WindowList;
   while (p)
   {
      if (p == window)
      {
         CloseWindow(window);
         return;
      }
      p = p->next;
   }
}

static int doOpen(WindowInfo* window, const char* name, const char* path, int flags)
{
   char fullname[MAXPATHLEN] = "";
   struct stat statbuf;
   int fileLen, readLen;
   char* fileString, *c;
   FILE* fp = NULL;
   int fd;
   int resp;

   // initialize lock reasons
   CLEAR_ALL_LOCKS(window->lockReasons);

   // Update the window data structure
   strcpy(window->filename, name);
   strcpy(window->path, path);
   window->filenameSet = true;
   window->fileMissing = true;

   // Get the full name of the file
   strcpy(fullname, path);
   strcat(fullname, name);

   // Open the file
#ifndef DONT_USE_ACCESS
   /* The only advantage of this is if you use clearcase,
   which messes up the mtime of files opened with r+,
   even if they're never actually written.
   To avoid requiring special builds for clearcase users,
   this is now the default. */
   {
      if ((fp = fopen(fullname, "r")) != NULL)
      {
// TODO:          if (access(fullname, W_OK) != 0)
// TODO:             SET_PERM_LOCKED(window->lockReasons, true);
#else
   fp = fopen(fullname, "rb+");
   if (fp == NULL)
   {
      /* Error opening file or file is not writeable */
      fp = fopen(fullname, "rb");
      if (fp != NULL)
      {
         /* File is read only */
         SET_PERM_LOCKED(window->lockReasons, true);
#endif
      }
      else if (flags & NE_CREATE && errno == ENOENT)
      {
         /* Give option to create (or to exit if this is the only window) */
         if (!(flags & NE_SUPPRESS_CREATE_WARN))
         {
            /* on Solaris 2.6, and possibly other OSes, dialog won't
            show if parent window is iconized. */
            RaiseShellWindow(window->mainWindow, false);

            /* ask user for next action if file not found */
            if (WindowList == window && window->next == NULL)
            {
               resp = DialogF(DF_WARN, window->mainWindow, 3, "New File",
                              "Can't open %s:\n%s", "New File", "Cancel",
                              "Exit NEdit", fullname, errorString());
            }
            else
            {
               resp = DialogF(DF_WARN, window->mainWindow, 2, "New File",
                              "Can't open %s:\n%s", "New File", "Cancel", fullname,
                              errorString());
            }

            if (resp == 2)
            {
               return false;
            }
            else if (resp == 3)
            {
               exit(EXIT_SUCCESS);
            }
         }

         /* Test if new file can be created */
         if ((fd = creat(fullname, 0666)) == -1)
         {
            DialogF(DF_ERR, window->mainWindow, 1, "Error creating File", "Can't create %s:\n%s", "OK", fullname, errorString());
            return false;
         }
         else
         {
            close(fd);
            remove(fullname);
         }

         SetWindowModified(window, false);
         if ((flags & NE_PREF_READ_ONLY) != 0)
         {
            SET_USER_LOCKED(window->lockReasons, true);
         }
         UpdateWindowReadOnly(window);
         return true;
      }
      else
      {
         // A true error
         DialogF(DF_ERR, window->mainWindow, 1, "Error opening File", "Could not open %s%s:\n%s", "OK", path, name, errorString());
         return false;
      }
   }

   // Get the length of the file, the protection mode, and the time of the
   // last modification to the file
   if (fstat(fileno(fp), &statbuf) != 0)
   {
      fclose(fp);
      window->filenameSet = false; /* Temp. prevent check for changes. */
      DialogF(DF_ERR, window->mainWindow, 1, "Error opening File", "Error opening %s", "OK", name);
      window->filenameSet = true;
      return false;
   }

   if (S_ISDIR(statbuf.st_mode))
   {
      fclose(fp);
      window->filenameSet = false; /* Temp. prevent check for changes. */
      DialogF(DF_ERR, window->mainWindow, 1, "Error opening File", "Can't open directory %s", "OK", name);
      window->filenameSet = true;
      return false;
   }

#ifdef S_ISBLK
   if (S_ISBLK(statbuf.st_mode))
   {
      fclose(fp);
      window->filenameSet = false; /* Temp. prevent check for changes. */
      DialogF(DF_ERR, window->mainWindow, 1, "Error opening File", "Can't open block device %s", "OK", name);
      window->filenameSet = true;
      return false;
   }
#endif
   fileLen = statbuf.st_size;

   /* Allocate space for the whole contents of the file (unfortunately) */
   fileString = new char[fileLen+1];   /* +1 = space for null */
   if (fileString == NULL)
   {
      fclose(fp);
      window->filenameSet = false; /* Temp. prevent check for changes. */
      DialogF(DF_ERR, window->mainWindow, 1, "Error while opening File", "File is too large to edit", "OK");
      window->filenameSet = true;
      return false;
   }

   /* Read the file into fileString and terminate with a null */
   readLen = fread(fileString, sizeof(char), fileLen, fp);
   if (ferror(fp))
   {
      fclose(fp);
      window->filenameSet = false; /* Temp. prevent check for changes. */
      DialogF(DF_ERR, window->mainWindow, 1, "Error while opening File", "Error reading %s:\n%s", "OK", name, errorString());
      window->filenameSet = true;
      free__(fileString);
      return false;
   }
   fileString[readLen] = 0;

   /* Close the file */
   if (fclose(fp) != 0)
   {
      /* unlikely error */
      DialogF(DF_WARN, window->mainWindow, 1, "Error while opening File", "Unable to close file", "OK");
      /* we read it successfully, so continue */
   }

   // Any errors that happen after this point leave the window in a
   // "broken" state, and thus RevertToSaved will abandon the window if
   // window->fileMissing is false and doOpen fails.
   window->fileMode = statbuf.st_mode;
   window->fileUid = statbuf.st_uid;
   window->fileGid = statbuf.st_gid;
   window->lastModTime = statbuf.st_mtime;
   window->device = statbuf.st_dev;
   window->inode = statbuf.st_ino;
   window->fileMissing = false;

   /* Detect and convert DOS and Macintosh format files */
   if (GetPrefForceOSConversion())
   {
      window->fileFormat = FormatOfFile(fileString);
      if (window->fileFormat == DOS_FILE_FORMAT)
      {
         ConvertFromDosFileString(fileString, &readLen, NULL);
      }
      else if (window->fileFormat == MAC_FILE_FORMAT)
      {
         ConvertFromMacFileString(fileString, readLen);
      }
   }

   // Display the file contents in the text widget
   window->ignoreModify = true;
   BufSetAll(window->buffer, fileString);
   window->ignoreModify = false;

   /* Check that the length that the buffer thinks it has is the same
      as what we gave it.  If not, there were probably nuls in the file.
      Substitute them with another character.  If that is impossible, warn
      the user, make the file read-only, and force a substitution */
   if (window->buffer->length != readLen)
   {
      if (!BufSubstituteNullChars(fileString, readLen, window->buffer))
      {
         resp = DialogF(DF_ERR, window->mainWindow, 2, "Error while opening File", "Too much binary data in file.  You may view\nit, but not modify or re-save its contents.", "View", "Cancel");
         if (resp == 2)
         {
            return false;
         }

         SET_TMBD_LOCKED(window->lockReasons, true);
         for (c = fileString; c < &fileString[readLen]; c++)
         {
            if (*c == '\0')
            {
               *c = (char) 0xfe;
            }
         }
         window->buffer->nullSubsChar = (char) 0xfe;
      }
      window->ignoreModify = true;
      BufSetAll(window->buffer, fileString);
      window->ignoreModify = false;
   }

   // Release the memory that holds fileString
   delete[] fileString;

   /* Set window title and file changed flag */
   if ((flags & NE_PREF_READ_ONLY) != 0)
   {
      SET_USER_LOCKED(window->lockReasons, true);
   }
   if (IS_PERM_LOCKED(window->lockReasons))
   {
      window->fileChanged = false;
      UpdateWindowTitle(window);
   }
   else
   {
      SetWindowModified(window, false);
      if (IS_ANY_LOCKED(window->lockReasons))
      {
         UpdateWindowTitle(window);
      }
   }
   UpdateWindowReadOnly(window);

   return true;
}

int IncludeFile(WindowInfo* window, const char* name)
{
   struct stat statbuf;
   int fileLen, readLen;
   char* fileString;
   FILE* fp = NULL;

   /* Open the file */
   fp = fopen(name, "rb");
   if (fp == NULL)
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Error opening File", "Could not open %s:\n%s", "OK", name, errorString());
      return false;
   }

   /* Get the length of the file */
   if (fstat(fileno(fp), &statbuf) != 0)
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Error opening File", "Error opening %s", "OK", name);
      fclose(fp);
      return false;
   }

   if (S_ISDIR(statbuf.st_mode))
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Error opening File", "Can't open directory %s", "OK", name);
      fclose(fp);
      return false;
   }
   fileLen = statbuf.st_size;

   /* allocate space for the whole contents of the file */
   fileString = (char*)malloc__(fileLen+1);   /* +1 = space for null */
   if (fileString == NULL)
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Error opening File", "File is too large to include", "OK");
      fclose(fp);
      return false;
   }

   /* read the file into fileString and terminate with a null */
   readLen = fread(fileString, sizeof(char), fileLen, fp);
   if (ferror(fp))
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Error opening File", "Error reading %s:\n%s", "OK", name, errorString());
      fclose(fp);
      free__(fileString);
      return false;
   }
   fileString[readLen] = 0;

   /* Detect and convert DOS and Macintosh format files */
   switch (FormatOfFile(fileString))
   {
   case DOS_FILE_FORMAT:
      ConvertFromDosFileString(fileString, &readLen, NULL);
      break;
   case MAC_FILE_FORMAT:
      ConvertFromMacFileString(fileString, readLen);
      break;
   default:
      /*  Default is Unix, no conversion necessary.  */
      break;
   }

   /* If the file contained ascii nulls, re-map them */
   if (!BufSubstituteNullChars(fileString, readLen, window->buffer))
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Error opening File", "Too much binary data in file", "OK");
   }

   /* close the file */
   if (fclose(fp) != 0)
   {
      /* unlikely error */
      DialogF(DF_WARN, window->mainWindow, 1, "Error opening File",
              "Unable to close file", "OK");
      /* we read it successfully, so continue */
   }

   /* insert the contents of the file in the selection or at the insert
      position in the window if no selection exists */
   if (window->buffer->primary.selected)
      BufReplaceSelected(window->buffer, fileString);
   else
      BufInsert(window->buffer, TextGetCursorPos(window->lastFocus),
                fileString);

   /* release the memory that holds fileString */
   free__(fileString);

   return true;
}

/*
** Close all files and windows, leaving one untitled window
*/
int CloseAllFilesAndWindows()
{
   while (WindowList->next != NULL ||
          WindowList->filenameSet || WindowList->fileChanged)
   {
      /*
       * When we're exiting through a macro, the document running the
       * macro does not disappear from the list, so we could get stuck
       * in an endless loop if we try to close it. Therefore, we close
       * other documents first. (Note that the document running the macro
       * may get closed because it is in the same window as another
       * document that gets closed, but it won't disappear; it becomes
       * Untitled.)
       */
      if (WindowList == MacroRunWindow() && WindowList->next != NULL)
      {
         if (!CloseAllDocumentInWindow(WindowList->next))
         {
            return false;
         }
      }
      else
      {
         if (!CloseAllDocumentInWindow(WindowList))
         {
            return false;
         }
      }
   }

   return true;
}

int CloseFileAndWindow(WindowInfo* window, int preResponse)
{
   int response, stat;

   /* Make sure that the window is not in iconified state */
   if (window->fileChanged)
      RaiseDocumentWindow(window);

   /* If the window is a normal & unmodified file or an empty new file,
      or if the user wants to ignore external modifications then
      just close it.  Otherwise ask for confirmation first. */
   if (!window->fileChanged &&
         /* Normal File */
         ((!window->fileMissing && window->lastModTime > 0) ||
          /* New File*/
          (window->fileMissing && window->lastModTime == 0) ||
          /* File deleted/modified externally, ignored by user. */
          !GetPrefWarnFileMods()))
   {
      CloseWindow(window);
      /* up-to-date windows don't have outstanding backup files to close */
   }
   else
   {
      if (preResponse == PROMPT_SBC_DIALOG_RESPONSE)
      {
         response = DialogF(DF_WARN, window->mainWindow, 3, "Save File",
                            "Save %s before closing?", "Yes", "No", "Cancel", window->filename);
      }
      else
      {
         response = preResponse;
      }

      if (response == YES_SBC_DIALOG_RESPONSE)
      {
         /* Save */
         stat = SaveWindow(window);
         if (stat)
         {
            CloseWindow(window);
         }
         else
         {
            return false;
         }
      }
      else if (response == NO_SBC_DIALOG_RESPONSE)
      {
         /* Don't Save */
         RemoveBackupFile(window);
         CloseWindow(window);
      }
      else   /* 3 == Cancel */
      {
         return false;
      }
   }
   return true;
}

int SaveWindow(WindowInfo* window)
{
   int stat;

   /* Try to ensure our information is up-to-date */
   CheckForChangesToFile(window);

   /* Return success if the file is normal & unchanged or is a
       read-only file. */
   if ((!window->fileChanged && !window->fileMissing &&
         window->lastModTime > 0) ||
         IS_ANY_LOCKED_IGNORING_PERM(window->lockReasons))
      return true;
   /* Prompt for a filename if this is an Untitled window */
   if (!window->filenameSet)
      return SaveWindowAs(window, NULL, false);

   /* Check for external modifications and warn the user */
   if (GetPrefWarnFileMods() && fileWasModifiedExternally(window))
   {
      stat = DialogF(DF_WARN, window->mainWindow, 2, "Save File",
                     "%s has been modified by another program.\n\n"
                     "Continuing this operation will overwrite any external\n"
                     "modifications to the file since it was opened in NEdit,\n"
                     "and your work or someone else's may potentially be lost.\n\n"
                     "To preserve the modified file, cancel this operation and\n"
                     "use Save As... to save this file under a different name,\n"
                     "or Revert to Saved to revert to the modified version.",
                     "Continue", "Cancel", window->filename);
      if (stat == 2)
      {
         /* Cancel and mark file as externally modified */
         window->lastModTime = 0;
         window->fileMissing = false;
         return false;
      }
   }

   if (writeBckVersion(window))
      return false;
   stat = doSave(window);
   if (stat)
      RemoveBackupFile(window);

   return stat;
}

int SaveWindowAs(WindowInfo* window, const char* newName, int addWrap)
{
   int response, retVal, fileFormat;
   char fullname[MAXPATHLEN], filename[MAXPATHLEN], pathname[MAXPATHLEN];
   WindowInfo* otherWindow;

   /* Get the new name for the file */
   if (newName == NULL)
   {
      response = PromptForNewFile(window, "Save File As", fullname, &fileFormat, &addWrap);
      if (response != GFN_OK)
         return false;
      window->fileFormat = fileFormat;
   }
   else
   {
      strcpy(fullname, newName);
   }

   if (1 == NormalizePathname(fullname))
   {
      return false;
   }

   /* Add newlines if requested */
   if (addWrap)
      addWrapNewlines(window);

   if (ParseFilename(fullname, filename, pathname) != 0)
   {
      return false;
   }

   /* If the requested file is this file, just save it and return */
   if (!strcmp(window->filename, filename) &&
         !strcmp(window->path, pathname))
   {
      if (writeBckVersion(window))
         return false;
      return doSave(window);
   }

   /* If the file is open in another window, make user close it.  Note that
      it is possible for user to close the window by hand while the dialog
      is still up, because the dialog is not application modal, so after
      doing the dialog, check again whether the window still exists. */
   otherWindow = FindWindowWithFile(filename, pathname);
   if (otherWindow != NULL)
   {
      response = DialogF(DF_WARN, window->mainWindow, 2, "File open",
                         "%s is open in another NEdit window", "Cancel",
                         "Close Other Window", filename);

      if (response == 1)
      {
         return false;
      }
      if (otherWindow == FindWindowWithFile(filename, pathname))
      {
         if (!CloseFileAndWindow(otherWindow, PROMPT_SBC_DIALOG_RESPONSE))
         {
            return false;
         }
      }
   }

   /* Destroy the file closed property for the original file */
   DeleteFileClosedProperty(window);

   /* Change the name of the file and save it under the new name */
   RemoveBackupFile(window);
   strcpy(window->filename, filename);
   strcpy(window->path, pathname);
   window->fileMode = 0;
   window->fileUid = 0;
   window->fileGid = 0;
   CLEAR_ALL_LOCKS(window->lockReasons);
   retVal = doSave(window);
   UpdateWindowReadOnly(window);
   RefreshTabState(window);

   /* Add the name to the convenience menu of previously opened files */
   AddToPrevOpenMenu(fullname);

   /*  If name has changed, language mode may have changed as well, unless
       it's an Untitled window for which the user already set a language
       mode; it's probably the right one.  */
   if (PLAIN_LANGUAGE_MODE == window->languageMode || window->filenameSet)
   {
      DetermineLanguageMode(window, false);
   }
   window->filenameSet = true;

   /* Update the stats line and window title with the new filename */
   UpdateWindowTitle(window);
   UpdateStatsLine(window);

   SortTabBar(window);
   return retVal;
}

static int doSave(WindowInfo* window)
{
   char* fileString = NULL;
   char fullname[MAXPATHLEN];
   struct stat statbuf;
   FILE* fp;
   int fileLen, result;

   /* Get the full name of the file */
   strcpy(fullname, window->path);
   strcat(fullname, window->filename);

#ifndef WIN32
   /*  Check for root and warn him if he wants to write to a file with
       none of the write bits set.  */
   if ((0 == getuid())
         && (0 == stat(fullname, &statbuf))
         && !(statbuf.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH)))
   {
      result = DialogF(DF_WARN, window->mainWindow, 2, "Writing Read-only File",
                       "File '%s' is marked as read-only.\n"
                       "Do you want to save anyway?",
                       "Save", "Cancel", window->filename);
      if (1 != result)
      {
         return true;
      }
   }
#endif

   /* add a terminating newline if the file doesn't already have one for
      Unix utilities which get confused otherwise
      NOTE: this must be done _before_ we create/open the file, because the
            (potential) buffer modification can trigger a check for file
            changes. If the file is created for the first time, it has
            zero size on disk, and the check would falsely conclude that the
            file has changed on disk, and would pop up a warning dialog */
   if (BufGetCharacter(window->buffer, window->buffer->length - 1) != '\n'
         && window->buffer->length != 0
         && GetPrefAppendLF())
   {
      BufInsert(window->buffer, window->buffer->length, "\n");
   }

   /* open the file */
   fp = fopen(fullname, "wb");
   if (fp == NULL)
   {
      result = DialogF(DF_WARN, window->mainWindow, 2, "Error saving File",
                       "Unable to save %s:\n%s\n\nSave as a new file?",
                       "Save As...", "Cancel",
                       window->filename, errorString());

      if (result == 1)
      {
         return SaveWindowAs(window, NULL, 0);
      }
      return false;
   }

   /* get the text buffer contents and its length */
   fileString = BufGetAll(window->buffer);
   fileLen = window->buffer->length;

   /* If null characters are substituted for, put them back */
   BufUnsubstituteNullChars(fileString, window->buffer);

   /* If the file is to be saved in DOS or Macintosh format, reconvert */
   if (window->fileFormat == DOS_FILE_FORMAT)
   {
      if (!ConvertToDosFileString(&fileString, &fileLen))
      {
         DialogF(DF_ERR, window->mainWindow, 1, "Out of Memory", "Out of memory!  Try\nsaving in Unix format", "OK");
         return false;
      }
   }
   else if (window->fileFormat == MAC_FILE_FORMAT)
   {
      ConvertToMacFileString(fileString, fileLen);
   }

   /* write to the file */
   fwrite(fileString, sizeof(char), fileLen, fp);

   if (ferror(fp))
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Error saving File", "%s not saved:\n%s", "OK", window->filename, errorString());
      fclose(fp);
      remove(fullname);
      delete[] fileString;
      return false;
   }

   /* close the file */
   if (fclose(fp) != 0)
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Error closing File", "Error closing file:\n%s", "OK", errorString());
      delete[] fileString;
      return false;
   }

   /* free__ the text buffer copy returned from NeTextGetString */
   delete[] fileString;

   /* success, file was written */
   SetWindowModified(window, false);

   /* update the modification time */
   if (stat(fullname, &statbuf) == 0)
   {
      window->lastModTime = statbuf.st_mtime;
      window->fileMissing = false;
      window->device = statbuf.st_dev;
      window->inode = statbuf.st_ino;
   }
   else
   {
      /* This needs to produce an error message -- the file can't be
          accessed! */
      window->lastModTime = 0;
      window->fileMissing = true;
      window->device = 0;
      window->inode = 0;
   }

   return true;
}

/*
** Create a backup file for the current window.  The name for the backup file
** is generated using the name and path stored in the window and adding a
** tilde (~) on UNIX and underscore (_) on VMS to the beginning of the name.
*/
int WriteBackupFile(WindowInfo* window)
{
   char* fileString = NULL;
   char name[MAXPATHLEN];
   FILE* fp;
   int fd, fileLen;

   /* Generate a name for the autoSave file */
   backupFileName(window, name, sizeof(name));

   /* remove the old backup file.
      Well, this might fail - we'll notice later however. */
   remove(name);

   /* open the file, set more restrictive permissions (using default
       permissions was somewhat of a security hole, because permissions were
       independent of those of the original file being edited */
#ifdef WIN32
   if ((fd = open(name, O_CREAT|O_EXCL|O_WRONLY)) < 0
      || (fp = fdopen(fd, "w")) == NULL)
#else
   if ((fd = open(name, O_CREAT|O_EXCL|O_WRONLY, S_IRUSR | S_IWUSR)) < 0
         || (fp = fdopen(fd, "w")) == NULL)
#endif // WIN32
   {
      DialogF(DF_WARN, window->mainWindow, 1, "Error writing Backup",
              "Unable to save backup for %s:\n%s\n"
              "Automatic backup is now off", "OK", window->filename,
              errorString());
      window->autoSave = false;
      SetToggleButtonState(window, window->autoSaveItem, false, false);
      return false;
   }

   /* get the text buffer contents and its length */
   fileString = BufGetAll(window->buffer);
   fileLen = window->buffer->length;

   /* If null characters are substituted for, put them back */
   BufUnsubstituteNullChars(fileString, window->buffer);

   /* add a terminating newline if the file doesn't already have one */
   if (fileLen != 0 && fileString[fileLen-1] != '\n')
      fileString[fileLen++] = '\n'; 	 /* null terminator no longer needed */

   /* write out the file */
   fwrite(fileString, sizeof(char), fileLen, fp);
   if (ferror(fp))
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Error saving Backup", "Error while saving backup for %s:\n%s\nAutomatic backup is now off", "OK", window->filename,
              errorString());
      fclose(fp);
      remove(name);
      delete[] fileString;
      window->autoSave = false;
      return false;
   }

   /* close the backup file */
   if (fclose(fp) != 0)
   {
      delete[] fileString;
      return false;
   }

   /* Free the text buffer copy returned from NeTextGetString */
   delete[] fileString;

   return true;
}

/*
** Remove the backup file associated with this window
*/
void RemoveBackupFile(WindowInfo* window)
{
   char name[MAXPATHLEN];

   /* Don't delete backup files when backups aren't activated. */
   if (window->autoSave == false)
      return;

   backupFileName(window, name, sizeof(name));
   remove(name);
}

/*
** Generate the name of the backup file for this window from the filename
** and path in the window data structure & write into name
*/
static void backupFileName(WindowInfo* window, char* name, size_t len)
{
   char bckname[MAXPATHLEN];
   if (window->filenameSet)
   {
      sprintf(name, "%s~%s", window->path, window->filename);
   }
   else
   {
      strcpy(bckname, "~");
      strncat(bckname, window->filename, MAXPATHLEN - 1);
      PrependHome(bckname, name, len);
   }
}

/*
** If saveOldVersion is on, copies the existing version of the file to
** <filename>.bck in anticipation of a new version being saved.  Returns
** true if backup fails and user requests that the new file not be written.
*/
static int writeBckVersion(WindowInfo* window)
{
   char fullname[MAXPATHLEN], bckname[MAXPATHLEN];
   struct stat statbuf;
   int in_fd, out_fd;
   char* io_buffer;
#define IO_BUFFER_SIZE ((size_t)(1024*1024))

   /* Do only if version backups are turned on */
   if (!window->saveOldVersion)
   {
      return false;
   }

   /* Get the full name of the file */
   strcpy(fullname, window->path);
   strcat(fullname, window->filename);

   /* Generate name for old version */
   if ((strlen(fullname) + 5) > (size_t) MAXPATHLEN)
   {
      return bckError(window, "file name too long", window->filename);
   }
   sprintf(bckname, "%s.bck", fullname);

   /* Delete the old backup file */
   /* Errors are ignored; we'll notice them later. */
   remove(bckname);

   /* open the file being edited.  If there are problems with the
      old file, don't bother the user, just skip the backup */
   in_fd = open(fullname, O_RDONLY);
   if (in_fd<0)
   {
      return false;
   }

   /* Get permissions of the file.
      We preserve the normal permissions but not ownership, extended
      attributes, et cetera. */
   if (fstat(in_fd, &statbuf) != 0)
   {
      return false;
   }

   /* open the destination file exclusive and with restrictive permissions. */
#ifdef WIN32
   out_fd = open(bckname, O_CREAT|O_EXCL|O_TRUNC|O_WRONLY);
#else
   out_fd = open(bckname, O_CREAT|O_EXCL|O_TRUNC|O_WRONLY, S_IRUSR | S_IWUSR);
#endif
   if (out_fd < 0)
   {
      return bckError(window, "Error open backup file", bckname);
   }

   /* Set permissions on new file */
#ifndef WIN32
   if (fchmod(out_fd, statbuf.st_mode) != 0)
   {
      close(in_fd);
      close(out_fd);
      remove(bckname);
      return bckError(window, "fchmod() failed", bckname);
   }
#endif

   /* Allocate I/O buffer */
   io_buffer = new char[IO_BUFFER_SIZE];
   if (NULL == io_buffer)
   {
      close(in_fd);
      close(out_fd);
      remove(bckname);
      return bckError(window, "out of memory", bckname);
   }

   /* copy loop */
   for (;;)
   {
      ssize_t bytes_read;
      ssize_t bytes_written;
      bytes_read = read(in_fd, io_buffer, IO_BUFFER_SIZE);

      if (bytes_read < 0)
      {
         close(in_fd);
         close(out_fd);
         remove(bckname);
         delete[] io_buffer;
         return bckError(window, "read() error", window->filename);
      }

      if (0 == bytes_read)
      {
         break; /* EOF */
      }

      /* write to the file */
      bytes_written = write(out_fd, io_buffer, (size_t) bytes_read);
      if (bytes_written != bytes_read)
      {
         close(in_fd);
         close(out_fd);
         remove(bckname);
         delete[] io_buffer;
         return bckError(window, errorString(), bckname);
      }
   }

   /* close the input and output files */
   close(in_fd);
   close(out_fd);

   delete[] io_buffer;

   return false;
}

/*
** Error processing for writeBckVersion, gives the user option to cancel
** the subsequent save, or continue and optionally turn off versioning
*/
static int bckError(WindowInfo* window, const char* errString, const char* file)
{
   int resp;

   resp = DialogF(DF_ERR, window->mainWindow, 3, "Error writing Backup", "Couldn't write .bck (last version) file.\n%s: %s", "Cancel Save", "Turn off Backups", "Continue", file, errString);
   if (resp == 1)
      return true;
   if (resp == 2)
   {
      window->saveOldVersion = false;
      SetToggleButtonState(window, window->saveLastItem, false, false);
   }
   return false;
}

// TODO: void PrintWindow(WindowInfo* window, int selectedOnly)
// TODO: {
// TODO:    textBuffer* buf = window->buffer;
// TODO:    selection* sel = &buf->primary;
// TODO:    char* fileString = NULL;
// TODO:    int fileLen;
// TODO: 
// TODO:    /* get the contents of the text buffer from the text area widget.  Add
// TODO:       wrapping newlines if necessary to make it match the displayed text */
// TODO:    if (selectedOnly)
// TODO:    {
// TODO:       if (!sel->selected)
// TODO:       {
// TODO:          XBell(TheDisplay, 0);
// TODO:          return;
// TODO:       }
// TODO:       if (sel->rectangular)
// TODO:       {
// TODO:          fileString = BufGetSelectionText(buf);
// TODO:          fileLen = strlen(fileString);
// TODO:       }
// TODO:       else
// TODO:          fileString = TextGetWrapped(window->textArea, sel->start, sel->end,
// TODO:                                      &fileLen);
// TODO:    }
// TODO:    else
// TODO:       fileString = TextGetWrapped(window->textArea, 0, buf->length, &fileLen);
// TODO: 
// TODO:    /* If null characters are substituted for, put them back */
// TODO:    BufUnsubstituteNullChars(fileString, buf);
// TODO: 
// TODO:    /* add a terminating newline if the file doesn't already have one */
// TODO:    if (fileLen != 0 && fileString[fileLen-1] != '\n')
// TODO:       fileString[fileLen++] = '\n'; 	 /* null terminator no longer needed */
// TODO: 
// TODO:    /* Print the string */
// TODO:    PrintString(fileString, fileLen, window->mainWindow, window->filename);
// TODO: 
// TODO:    /* Free the text buffer copy returned from NeTextGetString */
// TODO:    free__(fileString);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Print a string (length is required).  parent is the dialog parent, for
// TODO: ** error dialogs, and jobName is the print title.
// TODO: */
// TODO: void PrintString(const char* string, int length, Widget parent, const char* jobName)
// TODO: {
// TODO:    char tmpFileName[L_tmpnam];    /* L_tmpnam defined in stdio.h */
// TODO:    FILE* fp;
// TODO:    int fd;
// TODO: 
// TODO:    /* Generate a temporary file name */
// TODO:    /*  If the glibc is used, the linker issues a warning at this point. This is
// TODO:    very thoughtful of him, but does not apply to NEdit. The recommended
// TODO:    replacement mkstemp(3) uses the same algorithm as NEdit, namely
// TODO:        1. Create a filename
// TODO:        2. Open the file with the O_CREAT|O_EXCL flags
// TODO:    So all an attacker can do is a DoS on the print function. */
// TODO:    tmpnam(tmpFileName);
// TODO: 
// TODO:    /* open the temporary file */
// TODO: #ifdef VMS
// TODO:    if ((fp = fopen(tmpFileName, "w", "rfm = stmlf")) == NULL)
// TODO: #else
// TODO:    if ((fd = open(tmpFileName, O_CREAT|O_EXCL|O_WRONLY, S_IRUSR | S_IWUSR)) < 0 || (fp = fdopen(fd, "w")) == NULL)
// TODO: #endif /* VMS */
// TODO:    {
// TODO:       DialogF(DF_WARN, parent, 1, "Error while Printing",
// TODO:               "Unable to write file for printing:\n%s", "OK",
// TODO:               errorString());
// TODO:       return;
// TODO:    }
// TODO: 
// TODO: #ifdef VMS
// TODO:    chmod(tmpFileName, S_IRUSR | S_IWUSR);
// TODO: #endif
// TODO: 
// TODO:    /* write to the file */
// TODO: #ifdef IBM_FWRITE_BUG
// TODO:    write(fileno(fp), string, length);
// TODO: #else
// TODO:    fwrite(string, sizeof(char), length, fp);
// TODO: #endif
// TODO:    if (ferror(fp))
// TODO:    {
// TODO:       DialogF(DF_ERR, parent, 1, "Error while Printing", "%s not printed:\n%s", "OK", jobName, errorString());
// TODO:       fclose(fp); /* should call close(fd) in turn! */
// TODO:       remove(tmpFileName);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* close the temporary file */
// TODO:    if (fclose(fp) != 0)
// TODO:    {
// TODO:       DialogF(DF_ERR, parent, 1, "Error while Printing", "Error closing temp. print file:\n%s", "OK", errorString());
// TODO:       remove(tmpFileName);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Print the temporary file, then delete it and return success */
// TODO: #ifdef VMS
// TODO:    strcat(tmpFileName, ".");
// TODO:    PrintFile(parent, tmpFileName, jobName, true);
// TODO: #else
// TODO:    PrintFile(parent, tmpFileName, jobName);
// TODO:    remove(tmpFileName);
// TODO: #endif /*VMS*/
// TODO:    return;
// TODO: }

// --------------------------------------------------------------------------
// Wrapper for GetExistingFilename which uses the current window's path
// (if set) as the default directory.
// --------------------------------------------------------------------------
int PromptForExistingFile(WindowInfo* window, char* prompt, char* fullname)
{
  Fl_Native_File_Chooser fileChooser;
  fileChooser.title("Open file");
  fileChooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
  if ( fileChooser.show() ) return GFN_CANCEL;
  
  const char* filename = fileChooser.filename();
  if (!filename) return GFN_CANCEL;

  strcpy(fullname, filename);
  return GFN_OK;

// TODO:    char* savedDefaultDir;
// TODO:    int retVal;
// TODO: 
// TODO:    /* Temporarily set default directory to window->path, prompt for file,
// TODO:       then, if the call was unsuccessful, restore the original default
// TODO:       directory */
// TODO:    savedDefaultDir = GetFileDialogDefaultDirectory();
// TODO:    if (*window->path != '\0')
// TODO:       SetFileDialogDefaultDirectory(window->path);
// TODO:    retVal = GetExistingFilename(window->mainWindow, prompt, fullname);
// TODO:    if (retVal != GFN_OK)
// TODO:       SetFileDialogDefaultDirectory(savedDefaultDir);
// TODO: 
// TODO:    free__(savedDefaultDir);
// TODO: 
// TODO:    return retVal;
}

/*
** Wrapper for HandleCustomNewFileSB which uses the current window's path
** (if set) as the default directory, and asks about embedding newlines
** to make wrapping permanent.
*/
int PromptForNewFile(WindowInfo* window, char* prompt, char* fullname, int* fileFormat, int* addWrap)
{
// TODO:    Arg args[20];
// TODO:    NeString s1, s2;
// TODO:    Widget fileSB, wrapToggle;
// TODO:    Widget formatForm, formatBtns, unixFormat, dosFormat, macFormat;
// TODO:    char* savedDefaultDir;

   int retVal = 0;

   Fl_Native_File_Chooser fnfc;
   fnfc.title("Save File As?");
   fnfc.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);

   // Keep syncrhonize with enum fileFormats
   fnfc.filter("Unix Format\t*\nWindows Format\t*\nMacOS Format\t*");
   
   fnfc.filter_value(window->fileFormat);

   if ( fnfc.show() ) return GFN_CANCEL;
   
   *fileFormat = fnfc.filter_value();
   strcpy(fullname, fnfc.filename());

// TODO:    /* Temporarily set default directory to window->path, prompt for file,
// TODO:       then, if the call was unsuccessful, restore the original default
// TODO:       directory */
// TODO:    savedDefaultDir = GetFileDialogDefaultDirectory();
// TODO:    if (*window->path != '\0')
// TODO:       SetFileDialogDefaultDirectory(window->path);
// TODO: 
// TODO:    /* Present a file selection dialog with an added field for requesting
// TODO:       long line wrapping to become permanent via inserted newlines */
// TODO:    n = 0;
// TODO:    XtSetArg(args[n], XmNselectionLabelString, s1 = XmStringCreateLocalized("New File Name:"));
// TODO:    n++;
// TODO:    XtSetArg(args[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
// TODO:    n++;
// TODO:    XtSetArg(args[n], XmNdialogTitle, s2 = NeNewString(prompt));
// TODO:    n++;
// TODO:    fileSB = CreateFileSelectionDialog(window->mainWindow,"FileSelect",args,n);
// TODO:    NeStringFree(s1);
// TODO:    NeStringFree(s2);
// TODO:    formatForm = XtVaCreateManagedWidget("formatForm", xmFormWidgetClass, fileSB, NULL);
// TODO:    formatBtns = XtVaCreateManagedWidget("formatBtns",
// TODO:                                         xmRowColumnWidgetClass, formatForm,
// TODO:                                         XmNradioBehavior, XmONE_OF_MANY,
// TODO:                                         XmNorientation, XmHORIZONTAL,
// TODO:                                         XmNpacking, XmPACK_TIGHT,
// TODO:                                         XmNtopAttachment, XmATTACH_FORM,
// TODO:                                         XmNleftAttachment, XmATTACH_FORM,
// TODO:                                         NULL);
// TODO:    XtVaCreateManagedWidget("formatBtns", xmLabelWidgetClass, formatBtns, XmNlabelString, s1=NeNewString("Format:"), NULL);
// TODO:    NeStringFree(s1);
// TODO:    unixFormat = XtVaCreateManagedWidget("unixFormat",
// TODO:                                         xmToggleButtonWidgetClass, formatBtns,
// TODO:                                         XmNlabelString, s1 = NeNewString("Unix"),
// TODO:                                         XmNset, *fileFormat == UNIX_FILE_FORMAT,
// TODO:                                         XmNuserData, (XtPointer)UNIX_FILE_FORMAT,
// TODO:                                         XmNmarginHeight, 0,
// TODO:                                         XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                         XmNmnemonic, 'U',
// TODO:                                         NULL);
// TODO:    NeStringFree(s1);
// TODO:    XtAddCallback(unixFormat, XmNvalueChangedCallback, setFormatCB, fileFormat);
// TODO:    dosFormat = XtVaCreateManagedWidget("dosFormat",
// TODO:                                        xmToggleButtonWidgetClass, formatBtns,
// TODO:                                        XmNlabelString, s1 = NeNewString("DOS"),
// TODO:                                        XmNset, *fileFormat == DOS_FILE_FORMAT,
// TODO:                                        XmNuserData, (XtPointer)DOS_FILE_FORMAT,
// TODO:                                        XmNmarginHeight, 0,
// TODO:                                        XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                        XmNmnemonic, 'O',
// TODO:                                        NULL);
// TODO:    NeStringFree(s1);
// TODO:    XtAddCallback(dosFormat, XmNvalueChangedCallback, setFormatCB, fileFormat);
// TODO:    macFormat = XtVaCreateManagedWidget("macFormat",
// TODO:                                        xmToggleButtonWidgetClass, formatBtns,
// TODO:                                        XmNlabelString, s1 = NeNewString("Macintosh"),
// TODO:                                        XmNset, *fileFormat == MAC_FILE_FORMAT,
// TODO:                                        XmNuserData, (XtPointer)MAC_FILE_FORMAT,
// TODO:                                        XmNmarginHeight, 0,
// TODO:                                        XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                        XmNmnemonic, 'M',
// TODO:                                        NULL);
// TODO:    NeStringFree(s1);
// TODO:    XtAddCallback(macFormat, XmNvalueChangedCallback, setFormatCB, fileFormat);
// TODO:    if (window->wrapMode == CONTINUOUS_WRAP)
// TODO:    {
// TODO:       wrapToggle = XtVaCreateManagedWidget("addWrap",
// TODO:                                            xmToggleButtonWidgetClass, formatForm,
// TODO:                                            XmNlabelString, s1 = NeNewString("Add line breaks where wrapped"),
// TODO:                                            XmNalignment, XmALIGNMENT_BEGINNING,
// TODO:                                            XmNmnemonic, 'A',
// TODO:                                            XmNtopAttachment, XmATTACH_WIDGET,
// TODO:                                            XmNtopWidget, formatBtns,
// TODO:                                            XmNleftAttachment, XmATTACH_FORM,
// TODO:                                            NULL);
// TODO:       XtAddCallback(wrapToggle, XmNvalueChangedCallback, addWrapCB, addWrap);
// TODO:       NeStringFree(s1);
// TODO:    }
// TODO:    *addWrap = false;
// TODO:    XtVaSetValues(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_FILTER_LABEL),
// TODO:                  XmNmnemonic, 'l',
// TODO:                  XmNuserData, XmFileSelectionBoxGetChild(fileSB, XmDIALOG_FILTER_TEXT),
// TODO:                  NULL);
// TODO:    XtVaSetValues(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_DIR_LIST_LABEL),
// TODO:                  XmNmnemonic, 'D',
// TODO:                  XmNuserData, XmFileSelectionBoxGetChild(fileSB, XmDIALOG_DIR_LIST),
// TODO:                  NULL);
// TODO:    XtVaSetValues(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_LIST_LABEL),
// TODO:                  XmNmnemonic, 'F',
// TODO:                  XmNuserData, XmFileSelectionBoxGetChild(fileSB, XmDIALOG_LIST),
// TODO:                  NULL);
// TODO:    XtVaSetValues(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_SELECTION_LABEL),
// TODO:                  XmNmnemonic, prompt[strspn(prompt, "lFD")],
// TODO:                  XmNuserData, XmFileSelectionBoxGetChild(fileSB, XmDIALOG_TEXT),
// TODO:                  NULL);
// TODO:    AddDialogMnemonicHandler(fileSB, false);
// TODO:    RemapDeleteKey(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_FILTER_TEXT));
// TODO:    RemapDeleteKey(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_TEXT));
// TODO:    retVal = HandleCustomNewFileSB(fileSB, fullname, window->filenameSet ? window->filename : NULL);
// TODO: 
// TODO:    if (retVal != GFN_OK)
// TODO:       SetFileDialogDefaultDirectory(savedDefaultDir);
// TODO: 
// TODO:    free__(savedDefaultDir);
// TODO: 
   return GFN_OK;

   return retVal;
}

/*
** Find a name for an untitled file, unique in the name space of in the opened
** files in this session, i.e. Untitled or Untitled_nn, and write it into
** the string "name".
*/
void UniqueUntitledName(char* name)
{
   WindowInfo* w;
   int i;

   for (i=0; i<INT_MAX; i++)
   {
      if (i == 0)
         sprintf(name, "Untitled");
      else
         sprintf(name, "Untitled_%d", i);
      for (w=WindowList; w!=NULL; w=w->next)
         if (!strcmp(w->filename, name))
            break;
      if (w == NULL)
         break;
   }
}

// TODO: /*
// TODO: ** Callback that guards us from trying to access a window after it has
// TODO: ** been destroyed while a modal dialog is up.
// TODO: */
// TODO: static void modifiedWindowDestroyedCB(Widget w, XtPointer clientData,
// TODO:                                       XtPointer callData)
// TODO: {
// TODO:    *(Bool*)clientData = true;
// TODO: }

/*
** Check if the file in the window was changed by an external source.
** and put up a warning dialog if it has.
*/
void CheckForChangesToFile(WindowInfo* window)
{
// TODO:    static WindowInfo* lastCheckWindow = NULL;
// TODO:    static Time lastCheckTime = 0;
// TODO:    char fullname[MAXPATHLEN];
// TODO:    struct stat statbuf;
// TODO:    Time timestamp;
// TODO:    FILE* fp;
// TODO:    int resp, silent = 0;
// TODO:    XWindowAttributes winAttr;
// TODO:    bool windowIsDestroyed = false;
// TODO: 
// TODO:    if (!window->filenameSet)
// TODO:       return;
// TODO: 
// TODO:    /* If last check was very recent, don't impact performance */
// TODO:    timestamp = XtLastTimestampProcessed(XtDisplay(window->mainWindow));
// TODO:    if (window == lastCheckWindow &&
// TODO:          timestamp - lastCheckTime < MOD_CHECK_INTERVAL)
// TODO:       return;
// TODO:    lastCheckWindow = window;
// TODO:    lastCheckTime = timestamp;
// TODO: 
// TODO:    /* Update the status, but don't pop up a dialog if we're called
// TODO:       from a place where the window might be iconic (e.g., from the
// TODO:       replace dialog) or on another desktop.
// TODO: 
// TODO:       This works, but I bet it costs a round-trip to the server.
// TODO:       Might be better to capture MapNotify/Unmap events instead.
// TODO: 
// TODO:       For tabs that are not on top, we don't want the dialog either,
// TODO:       and we don't even need to contact the server to find out. By
// TODO:       performing this check first, we avoid a server round-trip for
// TODO:       most files in practice. */
// TODO:    if (!IsTopDocument(window))
// TODO:       silent = 1;
// TODO:    else
// TODO:    {
// TODO:       XGetWindowAttributes(XtDisplay(window->mainWindow),
// TODO:                            XtWindow(window->mainWindow),
// TODO:                            &winAttr);
// TODO: 
// TODO:       if (winAttr.map_state != IsViewable)
// TODO:          silent = 1;
// TODO:    }
// TODO: 
// TODO:    /* Get the file mode and modification time */
// TODO:    strcpy(fullname, window->path);
// TODO:    strcat(fullname, window->filename);
// TODO:    if (stat(fullname, &statbuf) != 0)
// TODO:    {
// TODO:       /* Return if we've already warned the user or we can't warn him now */
// TODO:       if (window->fileMissing || silent)
// TODO:       {
// TODO:          return;
// TODO:       }
// TODO: 
// TODO:       /* Can't stat the file -- maybe it's been deleted.
// TODO:          The filename is now invalid */
// TODO:       window->fileMissing = true;
// TODO:       window->lastModTime = 1;
// TODO:       window->device = 0;
// TODO:       window->inode = 0;
// TODO: 
// TODO:       /* Warn the user, if they like to be warned (Maybe this should be its
// TODO:           own preference setting: GetPrefWarnFileDeleted()) */
// TODO:       if (GetPrefWarnFileMods())
// TODO:       {
// TODO:          char* title;
// TODO:          char* body;
// TODO: 
// TODO:          /* See note below about pop-up timing and XUngrabPointer */
// TODO:          XUngrabPointer(XtDisplay(window->mainWindow), timestamp);
// TODO: 
// TODO:          /* If the window (and the dialog) are destroyed while the dialog
// TODO:             is up (typically closed via the window manager), we should
// TODO:             avoid accessing the window afterwards. */
// TODO:          XtAddCallback(window->mainWindow, XmNdestroyCallback,
// TODO:                        modifiedWindowDestroyedCB, &windowIsDestroyed);
// TODO: 
// TODO:          /*  Set title, message body and button to match stat()'s error.  */
// TODO:          switch (errno)
// TODO:          {
// TODO:          case ENOENT:
// TODO:             /*  A component of the path file_name does not exist.  */
// TODO:             title = "File not Found";
// TODO:             body = "File '%s' (or directory in its path)\n"
// TODO:                    "no longer exists.\n"
// TODO:                    "Another program may have deleted or moved it.";
// TODO:             resp = DialogF(DF_ERR, window->mainWindow, 2, title, body, "Save", "Cancel", window->filename);
// TODO:             break;
// TODO:          case EACCES:
// TODO:             /*  Search permission denied for a path component. We add
// TODO:                 one to the response because Re-Save wouldn't really
// TODO:                 make sense here.  */
// TODO:             title = "Permission Denied";
// TODO:             body = "You no longer have access to file '%s'.\n"
// TODO:                    "Another program may have changed the permissions of\n"
// TODO:                    "one of its parent directories.";
// TODO:             resp = 1 + DialogF(DF_ERR, window->mainWindow, 1, title, body, "Cancel", window->filename);
// TODO:             break;
// TODO:          default:
// TODO:             /*  Everything else. This hints at an internal error (eg.
// TODO:                 ENOTDIR) or at some bad state at the host.  */
// TODO:             title = "File not Accessible";
// TODO:             body = "Error while checking the status of file '%s':\n"
// TODO:                    "    '%s'\n"
// TODO:                    "Please make sure that no data is lost before closing\n"
// TODO:                    "this window.";
// TODO:             resp = DialogF(DF_ERR, window->mainWindow, 2, title, body, "Save", "Cancel", window->filename, errorString());
// TODO:             break;
// TODO:          }
// TODO: 
// TODO:          if (!windowIsDestroyed)
// TODO:          {
// TODO:             XtRemoveCallback(window->mainWindow, XmNdestroyCallback,
// TODO:                              modifiedWindowDestroyedCB, &windowIsDestroyed);
// TODO:          }
// TODO: 
// TODO:          switch (resp)
// TODO:          {
// TODO:          case 1:
// TODO:             SaveWindow(window);
// TODO:             break;
// TODO:             /*  Good idea, but this leads to frequent crashes, see
// TODO:                 SF#1578869. Reinsert this if circumstances change by
// TODO:                 uncommenting this part and inserting a "Close" button
// TODO:                 before each Cancel button above.
// TODO:             case 2:
// TODO:                 CloseWindow(window);
// TODO:                 return;
// TODO:             */
// TODO:          }
// TODO:       }
// TODO: 
// TODO:       /* A missing or (re-)saved file can't be read-only. */
// TODO:       /*  TODO: A document without a file can be locked though.  */
// TODO:       /* Make sure that the window was not destroyed behind our back! */
// TODO:       if (!windowIsDestroyed)
// TODO:       {
// TODO:          SET_PERM_LOCKED(window->lockReasons, false);
// TODO:          UpdateWindowTitle(window);
// TODO:          UpdateWindowReadOnly(window);
// TODO:       }
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Check that the file's read-only status is still correct (but
// TODO:       only if the file can still be opened successfully in read mode) */
// TODO:    if (window->fileMode != statbuf.st_mode ||
// TODO:          window->fileUid != statbuf.st_uid ||
// TODO:          window->fileGid != statbuf.st_gid)
// TODO:    {
// TODO:       window->fileMode = statbuf.st_mode;
// TODO:       window->fileUid = statbuf.st_uid;
// TODO:       window->fileGid = statbuf.st_gid;
// TODO:       if ((fp = fopen(fullname, "r")) != NULL)
// TODO:       {
// TODO:          int readOnly;
// TODO:          fclose(fp);
// TODO: #ifndef DONT_USE_ACCESS
// TODO:          readOnly = access(fullname, W_OK) != 0;
// TODO: #else
// TODO:          if (((fp = fopen(fullname, "r+")) != NULL))
// TODO:          {
// TODO:             readOnly = false;
// TODO:             fclose(fp);
// TODO:          }
// TODO:          else
// TODO:             readOnly = true;
// TODO: #endif
// TODO:          if (IS_PERM_LOCKED(window->lockReasons) != readOnly)
// TODO:          {
// TODO:             SET_PERM_LOCKED(window->lockReasons, readOnly);
// TODO:             UpdateWindowTitle(window);
// TODO:             UpdateWindowReadOnly(window);
// TODO:          }
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* Warn the user if the file has been modified, unless checking is
// TODO:       turned off or the user has already been warned.  Popping up a dialog
// TODO:       from a focus callback (which is how this routine is usually called)
// TODO:       seems to catch Motif off guard, and if the timing is just right, the
// TODO:       dialog can be left with a still active pointer grab from a Motif menu
// TODO:       which is still in the process of popping down.  The workaround, below,
// TODO:       of calling XUngrabPointer is inelegant but seems to fix the problem. */
// TODO:    if (!silent &&
// TODO:          ((window->lastModTime != 0 &&
// TODO:            window->lastModTime != statbuf.st_mtime) ||
// TODO:           window->fileMissing))
// TODO:    {
// TODO:       window->lastModTime = 0;        /* Inhibit further warnings */
// TODO:       window->fileMissing = false;
// TODO:       if (!GetPrefWarnFileMods())
// TODO:          return;
// TODO:       if (GetPrefWarnRealFileMods() &&
// TODO:             !cmpWinAgainstFile(window, fullname))
// TODO:       {
// TODO:          /* Contents hasn't changed. Update the modification time. */
// TODO:          window->lastModTime = statbuf.st_mtime;
// TODO:          return;
// TODO:       }
// TODO:       XUngrabPointer(XtDisplay(window->mainWindow), timestamp);
// TODO:       if (window->fileChanged)
// TODO:          resp = DialogF(DF_WARN, window->mainWindow, 2,
// TODO:                         "File modified externally",
// TODO:                         "%s has been modified by another program.  Reload?\n\n"
// TODO:                         "WARNING: Reloading will discard changes made in this\n"
// TODO:                         "editing session!", "Reload", "Cancel", window->filename);
// TODO:       else
// TODO:          resp = DialogF(DF_WARN, window->mainWindow, 2,
// TODO:                         "File modified externally",
// TODO:                         "%s has been modified by another\nprogram.  Reload?",
// TODO:                         "Reload", "Cancel", window->filename);
// TODO:       if (resp == 1)
// TODO:          RevertToSaved(window);
// TODO:    }
}

/*
** Return true if the file displayed in window has been modified externally
** to nedit.  This should return false if the file has been deleted or is
** unavailable.
*/
static int fileWasModifiedExternally(WindowInfo* window)
{
   char fullname[MAXPATHLEN];
   struct stat statbuf;

   if (!window->filenameSet)
      return false;
   /* if (window->lastModTime == 0)
   return false; */
   strcpy(fullname, window->path);
   strcat(fullname, window->filename);
   if (stat(fullname, &statbuf) != 0)
      return false;
   if (window->lastModTime == statbuf.st_mtime)
      return false;
   if (GetPrefWarnRealFileMods() && !cmpWinAgainstFile(window, fullname))
   {
      return false;
   }
   return true;
}

/*
** Check the read-only or locked status of the window and beep and return
** false if the window should not be written in.
*/
int CheckReadOnly(WindowInfo* window)
{
   if (IS_ANY_LOCKED(window->lockReasons))
   {
      fl_beep();
      return true;
   }
   return false;
}

/*
** Wrapper for strerror so all the calls don't have to be ifdef'd for VMS.
*/
static const char* errorString()
{
   return strerror(errno);
}

// TODO: /*
// TODO: ** Callback procedure for File Format toggle buttons.  Format is stored
// TODO: ** in userData field of widget button
// TODO: */
// TODO: static void setFormatCB(Widget w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    if (NeToggleButtonGetState(w))
// TODO:    {
// TODO:       XtPointer userData;
// TODO:       XtVaGetValues(w, XmNuserData, &userData, NULL);
// TODO:       *(int*) clientData = (int) userData;
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Callback procedure for toggle button requesting newlines to be inserted
// TODO: ** to emulate continuous wrapping.
// TODO: */
// TODO: static void addWrapCB(Widget w, XtPointer clientData, XtPointer callData)
// TODO: {
// TODO:    int resp;
// TODO:    int* addWrap = (int*)clientData;
// TODO: 
// TODO:    if (NeToggleButtonGetState(w))
// TODO:    {
// TODO:       resp = DialogF(DF_WARN, w, 2, "Add Wrap",
// TODO:                      "This operation adds permanent line breaks to\n"
// TODO:                      "match the automatic wrapping done by the\n"
// TODO:                      "Continuous Wrap mode Preferences Option.\n\n"
// TODO:                      "*** This Option is Irreversable ***\n\n"
// TODO:                      "Once newlines are inserted, continuous wrapping\n"
// TODO:                      "will no longer work automatically on these lines", "OK",
// TODO:                      "Cancel");
// TODO:       if (resp == 2)
// TODO:       {
// TODO:          NeToggleButtonSetState(w, false, false);
// TODO:          *addWrap = false;
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          *addWrap = true;
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       *addWrap = false;
// TODO:    }
// TODO: }

/*
** Change a window created in NEdit's continuous wrap mode to the more
** conventional Unix format of embedded newlines.  Indicate to the user
** by turning off Continuous Wrap mode.
*/
static void addWrapNewlines(WindowInfo* window)
{
   int fileLen, i, insertPositions[MAX_PANES], topLines[MAX_PANES];
   int horizOffset;
   Ne_Text_Editor* text;
   char* fileString;

   /* save the insert and scroll positions of each pane */
   for (i=0; i<=window->nPanes; i++)
   {
      text = i==0 ? window->textArea : window->textPanes[i-1];
      insertPositions[i] = TextGetCursorPos(text);
      TextGetScroll(text, &topLines[i], &horizOffset);
   }

   /* Modify the buffer to add wrapping */
   fileString = TextGetWrapped(window->textArea, 0, window->buffer->length, &fileLen);
   BufSetAll(window->buffer, fileString);
   free__(fileString);

   /* restore the insert and scroll positions of each pane */
   for (i=0; i<=window->nPanes; i++)
   {
      text = i==0 ? window->textArea : window->textPanes[i-1];
      TextSetCursorPos(text, insertPositions[i]);
      TextSetScroll(text, topLines[i], 0);
   }

   /* Show the user that something has happened by turning off
      Continuous Wrap mode */
   SetToggleButtonState(window, window->continuousWrapItem, false, true);
}

/*
 * Number of bytes read at once by cmpWinAgainstFile
 */
#define PREFERRED_CMPBUF_LEN 32768

/*
 * Check if the contens of the textBuffer *buf is equal
 * the contens of the file named fileName. The format of
 * the file (UNIX/DOS/MAC) is handled properly.
 *
 * Return values
 *   0: no difference found
 *  !0: difference found or could not compare contents.
 */
static int cmpWinAgainstFile(WindowInfo* window, const char* fileName)
{
   char    fileString[PREFERRED_CMPBUF_LEN + 2];
   struct  stat statbuf;
   int     fileLen, restLen, nRead, bufPos, rv, offset, filePos;
   char    pendingCR = 0;
   int	    fileFormat = window->fileFormat;
   char    message[MAXPATHLEN+50];
   Ne_Text_Buffer* buf = window->buffer;
   FILE*   fp;

   fp = fopen(fileName, "r");
   if (!fp)
      return (1);
   if (fstat(fileno(fp), &statbuf) != 0)
   {
      fclose(fp);
      return (1);
   }

   fileLen = statbuf.st_size;
   /* For DOS files, we can't simply check the length */
   if (fileFormat != DOS_FILE_FORMAT)
   {
      if (fileLen != buf->length)
      {
         fclose(fp);
         return (1);
      }
   }
   else
   {
      /* If a DOS file is smaller on disk, it's certainly different */
      if (fileLen < buf->length)
      {
         fclose(fp);
         return (1);
      }
   }

   /* For large files, the comparison can take a while. If it takes too long,
      the user should be given a clue about what is happening. */
   sprintf(message, "Comparing externally modified %s ...", window->filename);
   restLen = std::min(PREFERRED_CMPBUF_LEN, fileLen);
   bufPos = 0;
   filePos = 0;
   while (restLen > 0)
   {
      AllWindowsBusy(message);
      if (pendingCR)
      {
         fileString[0] = pendingCR;
         offset = 1;
      }
      else
      {
         offset = 0;
      }

      nRead = fread(fileString+offset, sizeof(char), restLen, fp);
      if (nRead != restLen)
      {
         fclose(fp);
         AllWindowsUnbusy();
         return (1);
      }
      filePos += nRead;

      nRead += offset;

      /* check for on-disk file format changes, but only for the first hunk */
      if (bufPos == 0 && fileFormat != FormatOfFile(fileString))
      {
         fclose(fp);
         AllWindowsUnbusy();
         return (1);
      }

      if (fileFormat == MAC_FILE_FORMAT)
         ConvertFromMacFileString(fileString, nRead);
      else if (fileFormat == DOS_FILE_FORMAT)
         ConvertFromDosFileString(fileString, &nRead, &pendingCR);

      /* Beware of 0 chars ! */
      BufSubstituteNullChars(fileString, nRead, buf);
      rv = BufCmp(buf, bufPos, nRead, fileString);
      if (rv)
      {
         fclose(fp);
         AllWindowsUnbusy();
         return (rv);
      }
      bufPos += nRead;
      restLen = std::min(fileLen - filePos, PREFERRED_CMPBUF_LEN);
   }
   AllWindowsUnbusy();
   fclose(fp);
   if (pendingCR)
   {
      rv = BufCmp(buf, bufPos, 1, &pendingCR);
      if (rv)
      {
         return (rv);
      }
      bufPos += 1;
   }
   if (bufPos != buf->length)
   {
      return (1);
   }
   return (0);
}

/*
** Force ShowLineNumbers() to re-evaluate line counts for the window if line
** counts are required.
*/
static void forceShowLineNumbers(WindowInfo* window)
{
   bool showLineNum = window->showLineNumbers;
   if (showLineNum)
   {
      window->showLineNumbers = false;
      ShowLineNumbers(window, showLineNum);
   }
}
