static const char CVSID[] = "$Id: shell.c,v 1.44 2008/01/04 22:11:04 yooden Exp $";
/*******************************************************************************
*									       *
* shell.c -- Nirvana Editor shell command execution			       *
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
* December, 1993							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "shell.h"
#include "Ne_Text_Buffer.h"
#include "Ne_Text_Editor.h"
#include "nedit.h"
#include "window.h"
#include "preferences.h"
#include "file.h"
#include "macro.h"
#include "interpret.h"
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "menu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/param.h>
#include <sys/wait.h>
#include <unistd.h>
#else
typedef int pid_t;
#include <process.h>
#endif
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

#ifdef notdef
#ifdef IBM
#define NBBY 8
#include <sys/select.h>
#endif
#include <time.h>
#endif
#ifdef __EMX__
#include <process.h>
#endif

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

#include <FL/fl_ask.H>

/* Tuning parameters */
#define IO_BUF_SIZE 4096	/* size of buffers for collecting cmd output */
#define MAX_OUT_DIALOG_ROWS 30	/* max height of dialog for command output */
#define MAX_OUT_DIALOG_COLS 80	/* max width of dialog for command output */
#define OUTPUT_FLUSH_FREQ 1000	/* how often (msec) to flush output buffers
when process is taking too long */
#define BANNER_WAIT_TIME 6000	/* how long to wait (msec) before putting up
Shell Command Executing... banner */

/* flags for issueCommand */
#define ACCUMULATE 1
#define ERROR_DIALOGS 2
#define REPLACE_SELECTION 4
#define RELOAD_FILE_AFTER 8
#define OUTPUT_TO_DIALOG 16
#define OUTPUT_TO_STRING 32

/* element of a buffer list for collecting output from shell processes */
typedef struct bufElem
{
   struct bufElem* next;
   int length;
   char contents[IO_BUF_SIZE];
} buffer;

/* data attached to window during shell command execution with
   information for controling and communicating with the process */
typedef struct
{
   int flags;
   int stdinFD, stdoutFD, stderrFD;
   pid_t childPid;
   int stdinInputID, stdoutInputID, stderrInputID;
   buffer* outBufs, *errBufs;
   char* input;
   char* inPtr;
   Fl_Widget* textW;
   int leftPos, rightPos;
   int inLength;
   int bannerTimeoutID, flushTimeoutID;
   char bannerIsUp;
   char fromMacro;
} shellCmdInfo;

static void issueCommand(WindowInfo* window, const char* command, char* input, int inputLen, int flags, Fl_Widget* textW, int replaceLeft, int replaceRight, int fromMacro);
static void stdoutReadProc(void* clientData, int* source, int* id);
static void stderrReadProc(void* clientData, int* source, int* id);
static void stdinWriteProc(void* clientData, int* source, int* id);
static void finishCmdExecution(WindowInfo* window, int terminatedOnError);
static pid_t forkCommand(Fl_Widget* parent, const char* command, const char* cmdDir, int* stdinFD, int* stdoutFD, int* stderrFD);
static void addOutput(buffer** bufList, buffer* buf);
static char* coalesceOutput(buffer** bufList, int* length);
static void freeBufList(buffer** bufList);
static void removeTrailingNewlines(char* string);
static void createOutputDialog(Fl_Widget* parent, char* text);
static void measureText(char* text, int wrapWidth, int* rows, int* cols, int* wrapped);
static void truncateString(char* string, int length);
static void bannerTimeoutProc(void* clientData, int* id);
static void flushTimeoutProc(void* clientData, int* id);
static void safeBufReplace(Ne_Text_Buffer* buf, int* start, int* end, const char* text);
static char* shellCommandSubstitutes(const char* inStr, const char* fileStr, const char* lineStr);
static int shellSubstituter(char* outStr, const char* inStr, const char* fileStr, const char* lineStr, int outLen, int predictOnly);

/*
** Filter the current selection through shell command "command".  The selection
** is removed, and replaced by the output from the command execution.  Failed
** command status and output to stderr are presented in dialog form.
*/
void FilterSelection(WindowInfo* window, const char* command, int fromMacro)
{
   int left, right, textLen;
   char* text;

   /* Can't do two shell commands at once in the same window */
   if (window->shellCmdData != NULL)
   {
      fl_beep();
      return;
   }

   /* Get the selection and the range in character positions that it
      occupies.  Beep and return if no selection */
   text = BufGetSelectionText(window->buffer);
   if (*text == '\0')
   {
      free__(text);
      fl_beep();
      return;
   }
   textLen = strlen(text);
   BufUnsubstituteNullChars(text, window->buffer);
   left = window->buffer->primary.start;
   right = window->buffer->primary.end;

   /* Issue the command and collect its output */
   issueCommand(window, command, text, textLen, ACCUMULATE | ERROR_DIALOGS |
                REPLACE_SELECTION, window->lastFocus, left, right, fromMacro);
}

/*
** Execute shell command "command", depositing the result at the current
** insert position or in the current selection if the window has a
** selection.
*/
void ExecShellCommand(WindowInfo* window, const char* command, int fromMacro)
{
   int left, right, flags = 0;
   char* subsCommand, fullName[MAXPATHLEN];
   int pos, line, column;
   char lineNumber[11];

   /* Can't do two shell commands at once in the same window */
   if (window->shellCmdData != NULL)
   {
      fl_beep();
      return;
   }

   /* get the selection or the insert position */
   pos = TextGetCursorPos(window->lastFocus);
   if (GetSimpleSelection(window->buffer, &left, &right))
      flags = ACCUMULATE | REPLACE_SELECTION;
   else
      left = right = pos;

   /* Substitute the current file name for % and the current line number
      for # in the shell command */
   strcpy(fullName, window->path);
   strcat(fullName, window->filename);
   TextPosToLineAndCol(window->lastFocus, pos, &line, &column);
   sprintf(lineNumber, "%d", line);

   subsCommand = shellCommandSubstitutes(command, fullName, lineNumber);
   if (subsCommand == NULL)
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Shell Command", "Shell command is too long due to\nfilename substitutions with '%%' or\nline number substitutions with '#'", "OK");
      return;
   }

   /* issue the command */
   issueCommand(window, subsCommand, NULL, 0, flags, window->lastFocus, left,
                right, fromMacro);
   free__(subsCommand);
}

/*
** Execute shell command "command", on input string "input", depositing the
** in a macro string (via a call back to ReturnShellCommandOutput).
*/
void ShellCmdToMacroString(WindowInfo* window, const char* command,
                           const char* input)
{
   char* inputCopy;

   /* Make a copy of the input string for issueCommand to hold and free__
      upon completion */
   inputCopy = *input == '\0' ? NULL : NeNewString(input);

   /* fork the command and begin processing input/output */
   issueCommand(window, command, inputCopy, strlen(input), ACCUMULATE | OUTPUT_TO_STRING, NULL, 0, 0, true);
}

/*
** Execute the line of text where the the insertion cursor is positioned
** as a shell command.
*/
void ExecCursorLine(WindowInfo* window, int fromMacro)
{
   char* cmdText;
   int left, right, insertPos;
   char* subsCommand, fullName[MAXPATHLEN];
   int pos, line, column;
   char lineNumber[11];

   /* Can't do two shell commands at once in the same window */
   if (window->shellCmdData != NULL)
   {
      fl_beep();
      return;
   }

   /* get all of the text on the line with the insert position */
   pos = TextGetCursorPos(window->lastFocus);
   if (!GetSimpleSelection(window->buffer, &left, &right))
   {
      left = right = pos;
      left = BufStartOfLine(window->buffer, left);
      right = BufEndOfLine(window->buffer, right);
      insertPos = right;
   }
   else
      insertPos = BufEndOfLine(window->buffer, right);
   cmdText = BufGetRange(window->buffer, left, right);
   BufUnsubstituteNullChars(cmdText, window->buffer);

   /* insert a newline after the entire line */
   BufInsert(window->buffer, insertPos, "\n");

   /* Substitute the current file name for % and the current line number
      for # in the shell command */
   strcpy(fullName, window->path);
   strcat(fullName, window->filename);
   TextPosToLineAndCol(window->lastFocus, pos, &line, &column);
   sprintf(lineNumber, "%d", line);

   subsCommand = shellCommandSubstitutes(cmdText, fullName, lineNumber);
   if (subsCommand == NULL)
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Shell Command", "Shell command is too long due to\nfilename substitutions with '%%' or\nline number substitutions with '#'", "OK");
      return;
   }

   /* issue the command */
   issueCommand(window, subsCommand, NULL, 0, 0, window->lastFocus, insertPos+1,
                insertPos+1, fromMacro);
   free__(subsCommand);
   free__(cmdText);
}

/*
** Do a shell command, with the options allowed to users (input source,
** output destination, save first and load after) in the shell commands
** menu.
*/
void DoShellMenuCmd(WindowInfo* window, const char* command,
                    int input, int output,
                    int outputReplacesInput, int saveFirst, int loadAfter, int fromMacro)
{
   int flags = 0;
   char* text;
   char* subsCommand, fullName[MAXPATHLEN];
   int left = 0, right = 0, textLen;
   int pos, line, column;
   char lineNumber[11];
   WindowInfo* inWindow = window;
   Fl_Widget* outWidget;

   /* Can't do two shell commands at once in the same window */
   if (window->shellCmdData != NULL)
   {
      fl_beep();
      return;
   }

   /* Substitute the current file name for % and the current line number
      for # in the shell command */
   strcpy(fullName, window->path);
   strcat(fullName, window->filename);
   pos = TextGetCursorPos(window->lastFocus);
   TextPosToLineAndCol(window->lastFocus, pos, &line, &column);
   sprintf(lineNumber, "%d", line);

   subsCommand = shellCommandSubstitutes(command, fullName, lineNumber);
   if (subsCommand == NULL)
   {
      DialogF(DF_ERR, window->mainWindow, 1, "Shell Command", "Shell command is too long due to\nfilename substitutions with '%%' or\nline number substitutions with '#'", "OK");
      return;
   }

   /* Get the command input as a text string.  If there is input, errors
     shouldn't be mixed in with output, so set flags to ERROR_DIALOGS */
   if (input == FROM_SELECTION)
   {
      text = BufGetSelectionText(window->buffer);
      if (*text == '\0')
      {
         free__(text);
         free__(subsCommand);
         fl_beep();
         return;
      }
      flags |= ACCUMULATE | ERROR_DIALOGS;
   }
   else if (input == FROM_WINDOW)
   {
      text = BufGetAll(window->buffer);
      flags |= ACCUMULATE | ERROR_DIALOGS;
   }
   else if (input == FROM_EITHER)
   {
      text = BufGetSelectionText(window->buffer);
      if (*text == '\0')
      {
         free__(text);
         text = BufGetAll(window->buffer);
      }
      flags |= ACCUMULATE | ERROR_DIALOGS;
   }
   else   /* FROM_NONE */
      text = NULL;

   /* If the buffer was substituting another character for ascii-nuls,
      put the nuls back in before exporting the text */
   if (text != NULL)
   {
      textLen = strlen(text);
      BufUnsubstituteNullChars(text, window->buffer);
   }
   else
      textLen = 0;

   /* Assign the output destination.  If output is to a new window,
      create it, and run the command from it instead of the current
      one, to free__ the current one from waiting for lengthy execution */
   if (output == TO_DIALOG)
   {
      outWidget = NULL;
      flags |= OUTPUT_TO_DIALOG;
      left = right = 0;
   }
   else if (output == TO_NEW_WINDOW)
   {
      EditNewFile(GetPrefOpenInTab()?inWindow:NULL, NULL, false, NULL, window->path);
      outWidget = WindowList->textArea;
      inWindow = WindowList;
      left = right = 0;
      CheckCloseDim();
   }
   else     /* TO_SAME_WINDOW */
   {
      outWidget = window->lastFocus;
      if (outputReplacesInput && input != FROM_NONE)
      {
         if (input == FROM_WINDOW)
         {
            left = 0;
            right = window->buffer->length;
         }
         else if (input == FROM_SELECTION)
         {
            GetSimpleSelection(window->buffer, &left, &right);
            flags |= ACCUMULATE | REPLACE_SELECTION;
         }
         else if (input == FROM_EITHER)
         {
            if (GetSimpleSelection(window->buffer, &left, &right))
               flags |= ACCUMULATE | REPLACE_SELECTION;
            else
            {
               left = 0;
               right = window->buffer->length;
            }
         }
      }
      else
      {
         if (GetSimpleSelection(window->buffer, &left, &right))
            flags |= ACCUMULATE | REPLACE_SELECTION;
         else
            left = right = TextGetCursorPos(window->lastFocus);
      }
   }

   /* If the command requires the file be saved first, save it */
   if (saveFirst)
   {
      if (!SaveWindow(window))
      {
         if (input != FROM_NONE)
            free__(text);
         free__(subsCommand);
         return;
      }
   }

   /* If the command requires the file to be reloaded after execution, set
      a flag for issueCommand to deal with it when execution is complete */
   if (loadAfter)
      flags |= RELOAD_FILE_AFTER;

   /* issue the command */
   issueCommand(inWindow, subsCommand, text, textLen, flags, outWidget, left,
                right, fromMacro);
   free__(subsCommand);
}

/*
** Cancel the shell command in progress
*/
void AbortShellCommand(WindowInfo* window)
{
   shellCmdInfo* cmdData = (shellCmdInfo*)window->shellCmdData;

   if (cmdData == NULL)
      return;
// TODO:    kill(cmdData->childPid, SIGTERM);
   finishCmdExecution(window, true);
}

/*
** Issue a shell command and feed it the string "input".  Output can be
** directed either to text widget "textW" where it replaces the text between
** the positions "replaceLeft" and "replaceRight", to a separate pop-up dialog
** (OUTPUT_TO_DIALOG), or to a macro-language string (OUTPUT_TO_STRING).  If
** "input" is NULL, no input is fed to the process.  If an input string is
** provided, it is freed when the command completes.  Flags:
**
**   ACCUMULATE     	Causes output from the command to be saved up until
**  	    	    	the command completes.
**   ERROR_DIALOGS  	Presents stderr output separately in popup a dialog,
**  	    	    	and also reports failed exit status as a popup dialog
**  	    	    	including the command output.
**   REPLACE_SELECTION  Causes output to replace the selection in textW.
**   RELOAD_FILE_AFTER  Causes the file to be completely reloaded after the
**  	    	    	command completes.
**   OUTPUT_TO_DIALOG   Send output to a pop-up dialog instead of textW
**   OUTPUT_TO_STRING   Output to a macro-language string instead of a text
**  	    	    	widget or dialog.
**
** REPLACE_SELECTION, ERROR_DIALOGS, and OUTPUT_TO_STRING can only be used
** along with ACCUMULATE (these operations can't be done incrementally).
*/
static void issueCommand(WindowInfo* window, const char* command, char* input,
                         int inputLen, int flags, Fl_Widget* textW, int replaceLeft,
                         int replaceRight, int fromMacro)
{
   int stdinFD, stdoutFD, stderrFD = 0;
// TODO:    XtAppContext context = XtWidgetToApplicationContext(window->mainWindow);
   shellCmdInfo* cmdData;
   pid_t childPid;

   /* verify consistency of input parameters */
   if ((flags & ERROR_DIALOGS || flags & REPLACE_SELECTION ||
         flags & OUTPUT_TO_STRING) && !(flags & ACCUMULATE))
      return;

   /* a shell command called from a macro must be executed in the same
      window as the macro, regardless of where the output is directed,
      so the user can cancel them as a unit */
   if (fromMacro)
      window = MacroRunWindow();

   /* put up a watch cursor over the waiting window */
   if (!fromMacro)
      BeginWait(window->mainWindow);

   /* enable the cancel menu item */
   if (!fromMacro)
      SetSensitive(window, window->cancelShellItem, true);

   /* fork the subprocess and issue the command */
   childPid = forkCommand(window->mainWindow, command, window->path, &stdinFD, &stdoutFD, (flags & ERROR_DIALOGS) ? &stderrFD : NULL);

// TODO:    /* set the pipes connected to the process for non-blocking i/o */
// TODO:    if (fcntl(stdinFD, F_SETFL, O_NONBLOCK) < 0)
// TODO:       perror("nedit: Internal error (fcntl)");
// TODO:    if (fcntl(stdoutFD, F_SETFL, O_NONBLOCK) < 0)
// TODO:       perror("nedit: Internal error (fcntl1)");
// TODO:    if (flags & ERROR_DIALOGS)
// TODO:    {
// TODO:       if (fcntl(stderrFD, F_SETFL, O_NONBLOCK) < 0)
// TODO:          perror("nedit: Internal error (fcntl2)");
// TODO:    }
// TODO: 
// TODO:    /* if there's nothing to write to the process' stdin, close it now */
// TODO:    if (input == NULL)
// TODO:       close(stdinFD);

   /* Create a data structure for passing process information around
      amongst the callback routines which will process i/o and completion */
   cmdData = (shellCmdInfo*)malloc__(sizeof(shellCmdInfo));
   window->shellCmdData = cmdData;
   cmdData->flags = flags;
   cmdData->stdinFD = stdinFD;
   cmdData->stdoutFD = stdoutFD;
   cmdData->stderrFD = stderrFD;
   cmdData->childPid = childPid;
   cmdData->outBufs = NULL;
   cmdData->errBufs = NULL;
   cmdData->input = input;
   cmdData->inPtr = input;
   cmdData->textW = textW;
   cmdData->bannerIsUp = false;
   cmdData->fromMacro = fromMacro;
   cmdData->leftPos = replaceLeft;
   cmdData->rightPos = replaceRight;
   cmdData->inLength = inputLen;

// TODO:    /* Set up timer proc for putting up banner when process takes too long */
// TODO:    if (fromMacro)
// TODO:       cmdData->bannerTimeoutID = 0;
// TODO:    else
// TODO:       cmdData->bannerTimeoutID = XtAppAddTimeOut(context, BANNER_WAIT_TIME, bannerTimeoutProc, window);

// TODO:    /* Set up timer proc for flushing output buffers periodically */
// TODO:    if ((flags & ACCUMULATE) || textW == NULL)
// TODO:       cmdData->flushTimeoutID = 0;
// TODO:    else
// TODO:       cmdData->flushTimeoutID = XtAppAddTimeOut(context, OUTPUT_FLUSH_FREQ, flushTimeoutProc, window);
// TODO: 
// TODO:    /* set up callbacks for activity on the file descriptors */
// TODO:    cmdData->stdoutInputID = XtAppAddInput(context, stdoutFD, (void*)XtInputReadMask, stdoutReadProc, window);
// TODO:    if (input != NULL)
// TODO:       cmdData->stdinInputID = XtAppAddInput(context, stdinFD, (void*)XtInputWriteMask, stdinWriteProc, window);
// TODO:    else
// TODO:       cmdData->stdinInputID = 0;
// TODO:    if (flags & ERROR_DIALOGS)
// TODO:       cmdData->stderrInputID = XtAppAddInput(context, stderrFD, (void*)XtInputReadMask, stderrReadProc, window);
// TODO:    else
// TODO:       cmdData->stderrInputID = 0;

   /* If this was called from a macro, preempt the macro untill shell
      command completes */
   if (fromMacro)
      PreemptMacro();
}

/*
** Called when the shell sub-process stdout stream has data.  Reads data into
** the "outBufs" buffer chain in the window->mainWindowCommandData data structure.
*/
static void stdoutReadProc(void* clientData, int* source, int* id)
{
// TODO:    WindowInfo* window = (WindowInfo*)clientData;
// TODO:    shellCmdInfo* cmdData = (shellCmdInfo*)window->shellCmdData;
// TODO:    buffer* buf;
// TODO:    int nRead;
// TODO: 
// TODO:    /* read from the process' stdout stream */
// TODO:    buf = (buffer*)malloc__(sizeof(buffer));
// TODO:    nRead = read(cmdData->stdoutFD, buf->contents, IO_BUF_SIZE);
// TODO: 
// TODO:    /* error in read */
// TODO:    if (nRead == -1)   /* error */
// TODO:    {
// TODO:       if (errno != EWOULDBLOCK && errno != EAGAIN)
// TODO:       {
// TODO:          perror("nedit: Error reading shell command output");
// TODO:          free__((char*)buf);
// TODO:          finishCmdExecution(window, true);
// TODO:       }
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* end of data.  If the stderr stream is done too, execution of the
// TODO:       shell process is complete, and we can display the results */
// TODO:    if (nRead == 0)
// TODO:    {
// TODO:       free__((char*)buf);
// TODO:       XtRemoveInput(cmdData->stdoutInputID);
// TODO:       cmdData->stdoutInputID = 0;
// TODO:       if (cmdData->stderrInputID == 0)
// TODO:          finishCmdExecution(window, false);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* characters were read successfully, add buf to linked list of buffers */
// TODO:    buf->length = nRead;
// TODO:    addOutput(&cmdData->outBufs, buf);
}

/*
** Called when the shell sub-process stderr stream has data.  Reads data into
** the "errBufs" buffer chain in the window->mainWindowCommandData data structure.
*/
static void stderrReadProc(void* clientData, int* source, int* id)
{
// TODO:    WindowInfo* window = (WindowInfo*)clientData;
// TODO:    shellCmdInfo* cmdData = (shellCmdInfo*)window->shellCmdData;
// TODO:    buffer* buf;
// TODO:    int nRead;
// TODO: 
// TODO:    /* read from the process' stderr stream */
// TODO:    buf = (buffer*)malloc__(sizeof(buffer));
// TODO:    nRead = read(cmdData->stderrFD, buf->contents, IO_BUF_SIZE);
// TODO: 
// TODO:    /* error in read */
// TODO:    if (nRead == -1)
// TODO:    {
// TODO:       if (errno != EWOULDBLOCK && errno != EAGAIN)
// TODO:       {
// TODO:          perror("nedit: Error reading shell command error stream");
// TODO:          free__((char*)buf);
// TODO:          finishCmdExecution(window, true);
// TODO:       }
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* end of data.  If the stdout stream is done too, execution of the
// TODO:       shell process is complete, and we can display the results */
// TODO:    if (nRead == 0)
// TODO:    {
// TODO:       free__((char*)buf);
// TODO:       XtRemoveInput(cmdData->stderrInputID);
// TODO:       cmdData->stderrInputID = 0;
// TODO:       if (cmdData->stdoutInputID == 0)
// TODO:          finishCmdExecution(window, false);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* characters were read successfully, add buf to linked list of buffers */
// TODO:    buf->length = nRead;
// TODO:    addOutput(&cmdData->errBufs, buf);
}

/*
** Called when the shell sub-process stdin stream is ready for input.  Writes
** data from the "input" text string passed to issueCommand.
*/
static void stdinWriteProc(void* clientData, int* source, int* id)
{
   WindowInfo* window = (WindowInfo*)clientData;
   shellCmdInfo* cmdData = (shellCmdInfo*)window->shellCmdData;
// TODO:    int nWritten;
// TODO: 
// TODO:    nWritten = write(cmdData->stdinFD, cmdData->inPtr, cmdData->inLength);
// TODO:    if (nWritten == -1)
// TODO:    {
// TODO:       if (errno == EPIPE)
// TODO:       {
// TODO:          /* Just shut off input to broken pipes.  User is likely feeding
// TODO:             it to a command which does not take input */
// TODO:          XtRemoveInput(cmdData->stdinInputID);
// TODO:          cmdData->stdinInputID = 0;
// TODO:          close(cmdData->stdinFD);
// TODO:          cmdData->inPtr = NULL;
// TODO:       }
// TODO:       else if (errno != EWOULDBLOCK && errno != EAGAIN)
// TODO:       {
// TODO:          perror("nedit: Write to shell command failed");
// TODO:          finishCmdExecution(window, true);
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       cmdData->inPtr += nWritten;
// TODO:       cmdData->inLength -= nWritten;
// TODO:       if (cmdData->inLength <= 0)
// TODO:       {
// TODO:          XtRemoveInput(cmdData->stdinInputID);
// TODO:          cmdData->stdinInputID = 0;
// TODO:          close(cmdData->stdinFD);
// TODO:          cmdData->inPtr = NULL;
// TODO:       }
// TODO:    }
}

/*
** Timer proc for putting up the "Shell Command in Progress" banner if
** the process is taking too long.
*/
#define MAX_TIMEOUT_MSG_LEN (MAX_ACCEL_LEN + 60)
static void bannerTimeoutProc(void* clientData, int* id)
{
// TODO:    WindowInfo* window = (WindowInfo*)clientData;
// TODO:    shellCmdInfo* cmdData = (shellCmdInfo*)window->shellCmdData;
// TODO:    NeString xmCancel;
// TODO:    char* cCancel;
// TODO:    char message[MAX_TIMEOUT_MSG_LEN];
// TODO: 
// TODO:    cmdData->bannerIsUp = true;
// TODO: 
// TODO:    /* Extract accelerator text from menu PushButtons */
// TODO:    XtVaGetValues(window->cancelShellItem, XmNacceleratorText, &xmCancel, NULL);
// TODO: 
// TODO:    /* Translate Motif string to char* */
// TODO:    cCancel = GetNeStringText(xmCancel);
// TODO: 
// TODO:    /* Free Motif String */
// TODO:    NeStringFree(xmCancel);
// TODO: 
// TODO:    /* Create message */
// TODO:    if ('\0' == cCancel[0])
// TODO:    {
// TODO:       strncpy(message, "Shell Command in Progress", MAX_TIMEOUT_MSG_LEN);
// TODO:       message[MAX_TIMEOUT_MSG_LEN - 1] = '\0';
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       sprintf(message,
// TODO:               "Shell Command in Progress -- Press %s to Cancel",
// TODO:               cCancel);
// TODO:    }
// TODO: 
// TODO:    /* Free C-string */
// TODO:    free__(cCancel);
// TODO: 
// TODO:    SetModeMessage(window, message);
// TODO:    cmdData->bannerTimeoutID = 0;
}

/*
** Buffer replacement wrapper routine to be used for inserting output from
** a command into the buffer, which takes into account that the buffer may
** have been shrunken by the user (eg, by Undo). If necessary, the starting
** and ending positions (part of the state of the command) are corrected.
*/
static void safeBufReplace(Ne_Text_Buffer* buf, int* start, int* end, const char* text)
{
   if (*start > buf->length)
      *start = buf->length;
   if (*end > buf->length)
      *end = buf->length;
   BufReplace(buf, *start, *end, text);
}

/*
** Timer proc for flushing output buffers periodically when the process
** takes too long.
*/
static void flushTimeoutProc(void* clientData, int* id)
{
// TODO:    WindowInfo* window = (WindowInfo*)clientData;
// TODO:    shellCmdInfo* cmdData = (shellCmdInfo*)window->shellCmdData;
// TODO:    Ne_Text_Buffer* buf = TextGetBuffer(cmdData->textW);
// TODO:    int len;
// TODO:    char* outText;
// TODO: 
// TODO:    /* shouldn't happen, but it would be bad if it did */
// TODO:    if (cmdData->textW == NULL)
// TODO:       return;
// TODO: 
// TODO:    outText = coalesceOutput(&cmdData->outBufs, &len);
// TODO:    if (len != 0)
// TODO:    {
// TODO:       if (BufSubstituteNullChars(outText, len, buf))
// TODO:       {
// TODO:          safeBufReplace(buf, &cmdData->leftPos, &cmdData->rightPos, outText);
// TODO:          TextSetCursorPos(cmdData->textW, cmdData->leftPos+strlen(outText));
// TODO:          cmdData->leftPos += len;
// TODO:          cmdData->rightPos = cmdData->leftPos;
// TODO:       }
// TODO:       else
// TODO:          fprintf(stderr, "nedit: Too much binary data\n");
// TODO:    }
// TODO:    free__(outText);

// TODO:    /* re-establish the timer proc (this routine) to continue processing */
// TODO:    cmdData->flushTimeoutID = XtAppAddTimeOut(
// TODO:                                 XtWidgetToApplicationContext(window->mainWindow),
// TODO:                                 OUTPUT_FLUSH_FREQ, flushTimeoutProc, clientData);
}

/*
** Clean up after the execution of a shell command sub-process and present
** the output/errors to the user as requested in the initial issueCommand
** call.  If "terminatedOnError" is true, don't bother trying to read the
** output, just close the i/o descriptors, free__ the memory, and restore the
** user interface state.
*/
static void finishCmdExecution(WindowInfo* window, int terminatedOnError)
{
// TODO:    shellCmdInfo* cmdData = (shellCmdInfo*)window->shellCmdData;
// TODO:    textBuffer* buf;
// TODO:    int status, failure, errorReport, reselectStart, outTextLen, errTextLen;
// TODO:    int resp, cancel = false, fromMacro = cmdData->fromMacro;
// TODO:    char* outText, *errText = NULL;
// TODO: 
// TODO:    /* Cancel any pending i/o on the file descriptors */
// TODO:    if (cmdData->stdoutInputID != 0)
// TODO:       XtRemoveInput(cmdData->stdoutInputID);
// TODO:    if (cmdData->stdinInputID != 0)
// TODO:       XtRemoveInput(cmdData->stdinInputID);
// TODO:    if (cmdData->stderrInputID != 0)
// TODO:       XtRemoveInput(cmdData->stderrInputID);
// TODO: 
// TODO:    /* Close any file descriptors remaining open */
// TODO:    close(cmdData->stdoutFD);
// TODO:    if (cmdData->flags & ERROR_DIALOGS)
// TODO:       close(cmdData->stderrFD);
// TODO:    if (cmdData->inPtr != NULL)
// TODO:       close(cmdData->stdinFD);
// TODO: 
// TODO:    /* Free the provided input text */
// TODO:    free__(cmdData->input);
// TODO: 
// TODO:    /* Cancel pending timeouts */
// TODO:    if (cmdData->flushTimeoutID != 0)
// TODO:       XtRemoveTimeOut(cmdData->flushTimeoutID);
// TODO:    if (cmdData->bannerTimeoutID != 0)
// TODO:       XtRemoveTimeOut(cmdData->bannerTimeoutID);
// TODO: 
// TODO:    /* Clean up waiting-for-shell-command-to-complete mode */
// TODO:    if (!cmdData->fromMacro)
// TODO:    {
// TODO:       EndWait(window->mainWindow);
// TODO:       SetSensitive(window, window->cancelShellItem, false);
// TODO:       if (cmdData->bannerIsUp)
// TODO:          ClearModeMessage(window);
// TODO:    }
// TODO: 
// TODO:    /* If the process was killed or became inaccessable, give up */
// TODO:    if (terminatedOnError)
// TODO:    {
// TODO:       freeBufList(&cmdData->outBufs);
// TODO:       freeBufList(&cmdData->errBufs);
// TODO:       waitpid(cmdData->childPid, &status, 0);
// TODO:       goto cmdDone;
// TODO:    }
// TODO: 
// TODO:    /* Assemble the output from the process' stderr and stdout streams into
// TODO:       null terminated strings, and free__ the buffer lists used to collect it */
// TODO:    outText = coalesceOutput(&cmdData->outBufs, &outTextLen);
// TODO:    if (cmdData->flags & ERROR_DIALOGS)
// TODO:       errText = coalesceOutput(&cmdData->errBufs, &errTextLen);
// TODO: 
// TODO:    /* Wait for the child process to complete and get its return status */
// TODO:    waitpid(cmdData->childPid, &status, 0);
// TODO: 
// TODO:    /* Present error and stderr-information dialogs.  If a command returned
// TODO:       error output, or if the process' exit status indicated failure,
// TODO:       present the information to the user. */
// TODO:    if (cmdData->flags & ERROR_DIALOGS)
// TODO:    {
// TODO:       failure = WIFEXITED(status) && WEXITSTATUS(status) != 0;
// TODO:       errorReport = *errText != '\0';
// TODO: 
// TODO:       if (failure && errorReport)
// TODO:       {
// TODO:          removeTrailingNewlines(errText);
// TODO:          truncateString(errText, DF_MAX_MSG_LENGTH);
// TODO:          resp = DialogF(DF_WARN, window->mainWindow, 2, "Warning", "%s", "Cancel",
// TODO:                         "Proceed", errText);
// TODO:          cancel = resp == 1;
// TODO:       }
// TODO:       else if (failure)
// TODO:       {
// TODO:          truncateString(outText, DF_MAX_MSG_LENGTH-70);
// TODO:          resp = DialogF(DF_WARN, window->mainWindow, 2, "Command Failure",
// TODO:                         "Command reported failed exit status.\n"
// TODO:                         "Output from command:\n%s", "Cancel", "Proceed", outText);
// TODO:          cancel = resp == 1;
// TODO:       }
// TODO:       else if (errorReport)
// TODO:       {
// TODO:          removeTrailingNewlines(errText);
// TODO:          truncateString(errText, DF_MAX_MSG_LENGTH);
// TODO:          resp = DialogF(DF_INF, window->mainWindow, 2, "Information", "%s",
// TODO:                         "Proceed", "Cancel", errText);
// TODO:          cancel = resp == 2;
// TODO:       }
// TODO: 
// TODO:       free__(errText);
// TODO:       if (cancel)
// TODO:       {
// TODO:          free__(outText);
// TODO:          goto cmdDone;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* If output is to a dialog, present the dialog.  Otherwise insert the
// TODO:       (remaining) output in the text widget as requested, and move the
// TODO:       insert point to the end */
// TODO:    if (cmdData->flags & OUTPUT_TO_DIALOG)
// TODO:    {
// TODO:       removeTrailingNewlines(outText);
// TODO:       if (*outText != '\0')
// TODO:          createOutputDialog(window->mainWindow, outText);
// TODO:    }
// TODO:    else if (cmdData->flags & OUTPUT_TO_STRING)
// TODO:    {
// TODO:       ReturnShellCommandOutput(window,outText, WEXITSTATUS(status));
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       buf = TextGetBuffer(cmdData->textW);
// TODO:       if (!BufSubstituteNullChars(outText, outTextLen, buf))
// TODO:       {
// TODO:          fprintf(stderr,"nedit: Too much binary data in shell cmd output\n");
// TODO:          outText[0] = '\0';
// TODO:       }
// TODO:       if (cmdData->flags & REPLACE_SELECTION)
// TODO:       {
// TODO:          reselectStart = buf->primary.rectangular ? -1 : buf->primary.start;
// TODO:          BufReplaceSelected(buf, outText);
// TODO:          TextSetCursorPos(cmdData->textW, buf->cursorPosHint);
// TODO:          if (reselectStart != -1)
// TODO:             BufSelect(buf, reselectStart, reselectStart + strlen(outText));
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          safeBufReplace(buf, &cmdData->leftPos, &cmdData->rightPos, outText);
// TODO:          TextSetCursorPos(cmdData->textW, cmdData->leftPos+strlen(outText));
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* If the command requires the file to be reloaded afterward, reload it */
// TODO:    if (cmdData->flags & RELOAD_FILE_AFTER)
// TODO:       RevertToSaved(window);
// TODO: 
// TODO:    /* Command is complete, free__ data structure and continue macro execution */
// TODO:    free__(outText);
// TODO: cmdDone:
// TODO:    free__((char*)cmdData);
// TODO:    window->shellCmdData = NULL;
// TODO:    if (fromMacro)
// TODO:       ResumeMacroExecution(window);
}

/*
** Fork a subprocess to execute a command, return file descriptors for pipes
** connected to the subprocess' stdin, stdout, and stderr streams.  cmdDir
** sets the default directory for the subprocess.  If stderrFD is passed as
** NULL, the pipe represented by stdoutFD is connected to both stdin and
** stderr.  The function value returns the pid of the new subprocess, or -1
** if an error occured.
*/
static pid_t forkCommand(Fl_Widget* parent, const char* command, const char* cmdDir, int* stdinFD, int* stdoutFD, int* stderrFD)
{
   pid_t childPid = 0;
// TODO:    int childStdoutFD, childStdinFD, childStderrFD, pipeFDs[2];
// TODO:    int dupFD;
// TODO: 
// TODO:    /* Ignore SIGPIPE signals generated when user attempts to provide
// TODO:       input for commands which don't take input */
// TODO:    signal(SIGPIPE, SIG_IGN);
// TODO: 
// TODO:    /* Create pipes to communicate with the sub process.  One end of each is
// TODO:       returned to the caller, the other half is spliced to stdin, stdout
// TODO:       and stderr in the child process */
// TODO:    if (pipe(pipeFDs) != 0)
// TODO:    {
// TODO:       perror("nedit: Internal error (opening stdout pipe)");
// TODO:       return -1;
// TODO:    }
// TODO:    *stdoutFD = pipeFDs[0];
// TODO:    childStdoutFD = pipeFDs[1];
// TODO:    if (pipe(pipeFDs) != 0)
// TODO:    {
// TODO:       perror("nedit: Internal error (opening stdin pipe)");
// TODO:       return -1;
// TODO:    }
// TODO:    *stdinFD = pipeFDs[1];
// TODO:    childStdinFD = pipeFDs[0];
// TODO:    if (stderrFD == NULL)
// TODO:       childStderrFD = childStdoutFD;
// TODO:    else
// TODO:    {
// TODO:       if (pipe(pipeFDs) != 0)
// TODO:       {
// TODO:          perror("nedit: Internal error (opening stdin pipe)");
// TODO:          return -1;
// TODO:       }
// TODO:       *stderrFD = pipeFDs[0];
// TODO:       childStderrFD = pipeFDs[1];
// TODO:    }
// TODO: 
// TODO:    /* Fork the process */
// TODO:    childPid = fork();
// TODO: 
// TODO:    /*
// TODO:    ** Child process context (fork returned 0), clean up the
// TODO:    ** child ends of the pipes and execute the command
// TODO:    */
// TODO:    if (0 == childPid)
// TODO:    {
// TODO:       /* close the parent end of the pipes in the child process   */
// TODO:       close(*stdinFD);
// TODO:       close(*stdoutFD);
// TODO:       if (stderrFD != NULL)
// TODO:          close(*stderrFD);
// TODO: 
// TODO:       /* close current stdin, stdout, and stderr file descriptors before
// TODO:          substituting pipes */
// TODO:       close(fileno(stdin));
// TODO:       close(fileno(stdout));
// TODO:       close(fileno(stderr));
// TODO: 
// TODO:       /* duplicate the child ends of the pipes to have the same numbers
// TODO:          as stdout & stderr, so it can substitute for stdout & stderr */
// TODO:       dupFD = dup2(childStdinFD, fileno(stdin));
// TODO:       if (dupFD == -1)
// TODO:          perror("dup of stdin failed");
// TODO:       dupFD = dup2(childStdoutFD, fileno(stdout));
// TODO:       if (dupFD == -1)
// TODO:          perror("dup of stdout failed");
// TODO:       dupFD = dup2(childStderrFD, fileno(stderr));
// TODO:       if (dupFD == -1)
// TODO:          perror("dup of stderr failed");
// TODO: 
// TODO:       /* now close the original child end of the pipes
// TODO:          (we now have the 0, 1 and 2 descriptors in their place) */
// TODO:       close(childStdinFD);
// TODO:       close(childStdoutFD);
// TODO:       close(childStderrFD);
// TODO: 
// TODO:       /* make this process the leader of a new process group, so the sub
// TODO:          processes can be killed, if necessary, with a killpg call */
// TODO: #ifndef __EMX__  /* OS/2 doesn't have this */
// TODO: #ifndef VMS  /* VMS doesn't have this */
// TODO:       setsid();
// TODO: #endif
// TODO: #endif
// TODO: 
// TODO:       /* change the current working directory to the directory of the
// TODO:           current file. */
// TODO:       if (cmdDir[0] != 0)
// TODO:       {
// TODO:          if (chdir(cmdDir) == -1)
// TODO:          {
// TODO:             perror("chdir to directory of current file failed");
// TODO:          }
// TODO:       }
// TODO: 
// TODO:       /* execute the command using the shell specified by preferences */
// TODO:       execlp(GetPrefShell(), GetPrefShell(), "-c", command, NULL);
// TODO: 
// TODO:       /* if we reach here, execlp failed */
// TODO:       fprintf(stderr, "Error starting shell: %s\n", GetPrefShell());
// TODO:       exit(EXIT_FAILURE);
// TODO:    }
// TODO: 
// TODO:    /* Parent process context, check if fork succeeded */
// TODO:    if (childPid == -1)
// TODO:    {
// TODO:       DialogF(DF_ERR, parent, 1, "Shell Command", "Error starting shell command process\n(fork failed)", "OK");
// TODO:    }
// TODO: 
// TODO:    /* close the child ends of the pipes */
// TODO:    close(childStdinFD);
// TODO:    close(childStdoutFD);
// TODO:    if (stderrFD != NULL)
// TODO:       close(childStderrFD);

   return childPid;
}

/*
** Add a buffer full of output to a buffer list
*/
static void addOutput(buffer** bufList, buffer* buf)
{
   buf->next = *bufList;
   *bufList = buf;
}

/*
** coalesce the contents of a list of buffers into a contiguous memory block,
** freeing the memory occupied by the buffer list.  Returns the memory block
** as the function result, and its length as parameter "length".
*/
static char* coalesceOutput(buffer** bufList, int* outLength)
{
   buffer* buf, *rBufList = NULL;
   char* outBuf, *outPtr, *p;
   int i, length = 0;

   /* find the total length of data read */
   for (buf=*bufList; buf!=NULL; buf=buf->next)
      length += buf->length;

   /* allocate contiguous memory for returning data */
   outBuf = (char*)malloc__(length+1);

   /* reverse the buffer list */
   while (*bufList != NULL)
   {
      buf = *bufList;
      *bufList = buf->next;
      buf->next = rBufList;
      rBufList = buf;
   }

   /* copy the buffers into the output buffer */
   outPtr = outBuf;
   for (buf=rBufList; buf!=NULL; buf=buf->next)
   {
      p = buf->contents;
      for (i=0; i<buf->length; i++)
         *outPtr++ = *p++;
   }

   /* terminate with a null */
   *outPtr = '\0';

   /* free__ the buffer list */
   freeBufList(&rBufList);

   *outLength = outPtr - outBuf;
   return outBuf;
}

static void freeBufList(buffer** bufList)
{
   buffer* buf;

   while (*bufList != NULL)
   {
      buf = *bufList;
      *bufList = buf->next;
      free__((char*)buf);
   }
}

/*
** Remove trailing newlines from a string by substituting nulls
*/
static void removeTrailingNewlines(char* string)
{
   char* endPtr = &string[strlen(string)-1];

   while (endPtr >= string && *endPtr == '\n')
      *endPtr-- = '\0';
}

/*
** Create a dialog for the output of a shell command.  The dialog lives until
** the user presses the Dismiss button, and is then destroyed
*/
static void createOutputDialog(Fl_Widget* parent, char* text)
{
// TODO:    Arg al[50];
// TODO:    int ac, rows, cols, hasScrollBar, wrapped;
// TODO:    Fl_Widget* form, *textW, *button;
// TODO:    NeString st1;
// TODO: 
// TODO:    /* measure the width and height of the text to determine size for dialog */
// TODO:    measureText(text, MAX_OUT_DIALOG_COLS, &rows, &cols, &wrapped);
// TODO:    if (rows > MAX_OUT_DIALOG_ROWS)
// TODO:    {
// TODO:       rows = MAX_OUT_DIALOG_ROWS;
// TODO:       hasScrollBar = true;
// TODO:    }
// TODO:    else
// TODO:       hasScrollBar = false;
// TODO:    if (cols > MAX_OUT_DIALOG_COLS)
// TODO:       cols = MAX_OUT_DIALOG_COLS;
// TODO:    if (cols == 0)
// TODO:       cols = 1;
// TODO:    /* Without completely emulating Motif's wrapping algorithm, we can't
// TODO:       be sure that we haven't underestimated the number of lines in case
// TODO:       a line has wrapped, so let's assume that some lines could be obscured
// TODO:       */
// TODO:    if (wrapped)
// TODO:       hasScrollBar = true;
// TODO:    ac = 0;
// TODO:    form = CreateFormDialog(parent, "shellOutForm", al, ac);
// TODO: 
// TODO:    ac = 0;
// TODO:    XtSetArg(al[ac], XmNlabelString, st1=MKSTRING("OK"));
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNmarginWidth, BUTTON_WIDTH_MARGIN);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNhighlightThickness, 2);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_NONE);
// TODO:    ac++;
// TODO:    button = XmCreatePushButtonGadget(form, "ok", al, ac);
// TODO:    XtManageChild(button);
// TODO:    XtVaSetValues(form, XmNdefaultButton, button, NULL);
// TODO:    XtVaSetValues(form, XmNcancelButton, button, NULL);
// TODO:    NeStringFree(st1);
// TODO:    XtAddCallback(button, XmNactivateCallback, destroyOutDialogCB,
// TODO:                  XtParent(form));
// TODO: 
// TODO:    ac = 0;
// TODO:    XtSetArg(al[ac], XmNrows, rows);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNcolumns, cols);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNresizeHeight, false);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNtraversalOn, true);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNwordWrap, true);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNscrollHorizontal, false);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNscrollVertical, hasScrollBar);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNhighlightThickness, 2);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNspacing, 0);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNeditMode, XmMULTI_LINE_EDIT);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNeditable, false);
// TODO:    ac++;
// TODO:    XtSetArg(al[ac], XmNvalue, text);
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
// TODO:    textW = XmCreateScrolledText(form, "outText", al, ac);
// TODO:    AddMouseWheelSupport(textW);
// TODO:    MakeSingleLineTextW(textW); /* Binds <Return> to activate() */
// TODO:    XtManageChild(textW);
// TODO: 
// TODO:    XtVaSetValues(XtParent(form), XmNtitle, "Output from Command", NULL);
// TODO:    ManageDialogCenteredOnPointer(form);
}

/*
** Measure the width and height of a string of text.  Assumes 8 character
** tabs.  wrapWidth specifies a number of columns at which text wraps.
*/
static void measureText(char* text, int wrapWidth, int* rows, int* cols, int* wrapped)
{
   int maxCols = 0, line = 1, col = 0, wrapCol;
   char* c;

   *wrapped = 0;
   for (c=text; *c!='\0'; c++)
   {
      if (*c=='\n')
      {
         line++;
         col = 0;
         continue;
      }

      if (*c == '\t')
      {
         col += 8 - (col % 8);
         wrapCol = 0; /* Tabs at end of line are not drawn when wrapped */
      }
      else if (*c == ' ')
      {
         col++;
         wrapCol = 0; /* Spaces at end of line are not drawn when wrapped */
      }
      else
      {
         col++;
         wrapCol = 1;
      }

      /* Note: there is a small chance that the number of lines is
         over-estimated when a line ends with a space or a tab (ie, followed
              by a newline) and that whitespace crosses the boundary, because
              whitespace at the end of a line does not cause wrapping. Taking
              this into account is very hard, but an over-estimation is harmless.
              The worst that can happen is that some extra blank lines are shown
              at the end of the dialog (in contrast to an under-estimation, which
              could make the last lines invisible).
              On the other hand, without emulating Motif's wrapping algorithm
              completely, we can't be sure that we don't underestimate the number
              of lines (Motif uses word wrap, and this counting algorithm uses
              character wrap). Therefore, we remember whether there is a line
              that has wrapped. In that case we allways install a scroll bar.
         */
      if (col > wrapWidth)
      {
         line++;
         *wrapped = 1;
         col = wrapCol;
      }
      else if (col > maxCols)
      {
         maxCols = col;
      }
   }
   *rows = line;
   *cols = maxCols;
}

/*
** Truncate a string to a maximum of length characters.  If it shortens the
** string, it appends "..." to show that it has been shortened. It assumes
** that the string that it is passed is writeable.
*/
static void truncateString(char* string, int length)
{
   if ((int)strlen(string) > length)
      memcpy(&string[length-3], "...", 4);
}

/*
** Substitute the string fileStr in inStr wherever % appears and
** lineStr in inStr wherever # appears, storing the
** result in outStr. If predictOnly is non-zero, the result string length
** is predicted without creating the string. Returns the length of the result
** string or -1 in case of an error.
**
*/
static int shellSubstituter(char* outStr, const char* inStr, const char* fileStr,
                            const char* lineStr, int outLen, int predictOnly)
{
   const char* inChar;
   char* outChar = NULL;
   int outWritten = 0;
   int fileLen, lineLen;

   inChar = inStr;
   if (!predictOnly)
   {
      outChar = outStr;
   }
   fileLen = strlen(fileStr);
   lineLen = strlen(lineStr);

   while (*inChar != '\0')
   {

      if (!predictOnly && outWritten >= outLen)
      {
         return(-1);
      }

      if (*inChar == '%')
      {
         if (*(inChar + 1) == '%')
         {
            inChar += 2;
            if (!predictOnly)
            {
               *outChar++ = '%';
            }
            outWritten++;
         }
         else
         {
            if (!predictOnly)
            {
               if (outWritten + fileLen >= outLen)
               {
                  return(-1);
               }
               strncpy(outChar, fileStr, fileLen);
               outChar += fileLen;
            }
            outWritten += fileLen;
            inChar++;
         }
      }
      else if (*inChar == '#')
      {
         if (*(inChar + 1) == '#')
         {
            inChar += 2;
            if (!predictOnly)
            {
               *outChar++ = '#';
            }
            outWritten++;
         }
         else
         {
            if (!predictOnly)
            {
               if (outWritten + lineLen >= outLen)
               {
                  return(-1);
               }
               strncpy(outChar, lineStr, lineLen);
               outChar += lineLen;
            }
            outWritten += lineLen;
            inChar++;
         }
      }
      else
      {
         if (!predictOnly)
         {
            *outChar++ = *inChar;
         }
         inChar++;
         outWritten++;
      }
   }

   if (!predictOnly)
   {
      if (outWritten >= outLen)
      {
         return(-1);
      }
      *outChar = '\0';
   }
   ++outWritten;
   return(outWritten);
}

static char* shellCommandSubstitutes(const char* inStr, const char* fileStr,
                                     const char* lineStr)
{
   int cmdLen;
   char* subsCmdStr = NULL;

   cmdLen = shellSubstituter(NULL, inStr, fileStr, lineStr, 0, 1);
   if (cmdLen >= 0)
   {
      subsCmdStr = (char*)malloc__(cmdLen);
      if (subsCmdStr)
      {
         cmdLen = shellSubstituter(subsCmdStr, inStr, fileStr, lineStr, cmdLen, 0);
         if (cmdLen < 0)
         {
            free__(subsCmdStr);
            subsCmdStr = NULL;
         }
      }
   }
   return(subsCmdStr);
}
