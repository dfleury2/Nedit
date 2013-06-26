// TODO: static const char CVSID[] = "$Id: textSel.c,v 1.19 2008/01/04 22:11:05 yooden Exp $";
// TODO: /*******************************************************************************
// TODO: *									       *
// TODO: * textSel.c - Selection and clipboard routines for NEdit text widget		       *
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
// TODO: * Dec. 15, 1995								       *
// TODO: *									       *
// TODO: * Written by Mark Edel							       *
// TODO: *									       *
// TODO: *******************************************************************************/
// TODO: 
// TODO: #ifdef HAVE_CONFIG_H
// TODO: #include "../config.h"
// TODO: #endif
// TODO: 
// TODO: #include "Ne_Text_Sel.h"
// TODO: #include "Ne_Text_Part.h"
// TODO: #include "Ne_Text_Editor.h"
// TODO: #include "Ne_Text_Display.h"
// TODO: #include "Ne_Text_Buffer.h"
// TODO: #include "../util/misc.h"
// TODO: 
// TODO: #include <stdio.h>
// TODO: #include <string.h>
// TODO: #include <limits.h>
// TODO: 
// TODO: #include <Xm/Xm.h>
// TODO: #include <Xm/CutPaste.h>
// TODO: #include <Xm/Text.h>
// TODO: #include <X11/Xatom.h>
// TODO: #if XmVersion >= 1002
// TODO: #include <Xm/PrimitiveP.h>
// TODO: #endif
// TODO: 
// TODO: #ifdef HAVE_DEBUG_H
// TODO: #include "../debug.h"
// TODO: #endif
// TODO: 
// TODO: 
// TODO: #define N_SELECT_TARGETS 7
// TODO: #define N_ATOMS 11
// TODO: enum atomIndex {A_TEXT, A_TARGETS, A_MULTIPLE, A_TIMESTAMP,
// TODO:                 A_INSERT_SELECTION, A_DELETE, A_CLIPBOARD, A_INSERT_INFO,
// TODO:                 A_ATOM_PAIR, A_MOTIF_DESTINATION, A_COMPOUND_TEXT
// TODO:                };
// TODO: 
// TODO: /* Results passed back to the convert proc processing an INSERT_SELECTION
// TODO:    request, by getInsertSelection when the selection to insert has been
// TODO:    received and processed */
// TODO: enum insertResultFlags {INSERT_WAITING, UNSUCCESSFUL_INSERT, SUCCESSFUL_INSERT};
// TODO: 
// TODO: /* Actions for selection notify event handler upon receiving confermation
// TODO:    of a successful convert selection request */
// TODO: enum selectNotifyActions {UNSELECT_SECONDARY, REMOVE_SECONDARY,
// TODO:                           EXCHANGE_SECONDARY
// TODO:                          };
// TODO: 
// TODO: /* temporary structure for passing data to the event handler for completing
// TODO:    selection requests (the hard way, via xlib calls) */
// TODO: typedef struct
// TODO: {
// TODO:    int action;
// TODO:    XtIntervalId timeoutProcID;
// TODO:    Time timeStamp;
// TODO:    Widget widget;
// TODO:    char* actionText;
// TODO:    int length;
// TODO: } selectNotifyInfo;
// TODO: 
// TODO: static void modifiedCB(int pos, int nInserted, int nDeleted,
// TODO:                        int nRestyled, const char* deletedText, void* cbArg);
// TODO: static void sendSecondary(Widget w, Time time, Atom sel, int action,
// TODO:                           char* actionText, int actionTextLen);
// TODO: static void getSelectionCB(Widget w, XtPointer clientData, Atom* selType,
// TODO:                            Atom* type, XtPointer value, unsigned long* length, int* format);
// TODO: static void getInsertSelectionCB(Widget w, XtPointer clientData,Atom* selType,
// TODO:                                  Atom* type, XtPointer value, unsigned long* length, int* format);
// TODO: static void getExchSelCB(Widget w, XtPointer clientData, Atom* selType,
// TODO:                          Atom* type, XtPointer value, unsigned long* length, int* format);
// TODO: static bool convertSelectionCB(Widget w, Atom* selType, Atom* target,
// TODO:                                   Atom* type, XtPointer* value, unsigned long* length, int* format);
// TODO: static void loseSelectionCB(Widget w, Atom* selType);
// TODO: static bool convertSecondaryCB(Widget w, Atom* selType, Atom* target,
// TODO:                                   Atom* type, XtPointer* value, unsigned long* length, int* format);
// TODO: static void loseSecondaryCB(Widget w, Atom* selType);
// TODO: static bool convertMotifDestCB(Widget w, Atom* selType, Atom* target,
// TODO:                                   Atom* type, XtPointer* value, unsigned long* length, int* format);
// TODO: static void loseMotifDestCB(Widget w, Atom* selType);
// TODO: static void selectNotifyEH(Widget w, XtPointer data, XEvent* event,
// TODO:                            bool* continueDispatch);
// TODO: static void selectNotifyTimerProc(XtPointer clientData, XtIntervalId* id);
// TODO: static Atom getAtom(Display* display, int atomNum);
// TODO: 
// TODO: /*
// TODO: ** Designate text widget "w" to be the selection owner for primary selections
// TODO: ** in its attached buffer (a buffer can be attached to multiple text widgets).
// TODO: */
// TODO: void HandleXSelections(Widget w)
// TODO: {
// TODO:    int i;
// TODO:    textBuffer* buf = ((TextWidget)w)->text.textD->buffer;
// TODO: 
// TODO:    /* Remove any existing selection handlers for other widgets */
// TODO:    for (i=0; i<buf->nModifyProcs; i++)
// TODO:    {
// TODO:       if (buf->modifyProcs[i] == modifiedCB)
// TODO:       {
// TODO:          BufRemoveModifyCB(buf, modifiedCB, buf->cbArgs[i]);
// TODO:          break;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* Add a handler with this widget as the CB arg (and thus the sel. owner) */
// TODO:    BufAddModifyCB(((TextWidget)w)->text.textD->buffer, modifiedCB, w);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Discontinue ownership of selections for widget "w"'s attached buffer
// TODO: ** (if "w" was the designated selection owner)
// TODO: */
// TODO: void StopHandlingXSelections(Widget w)
// TODO: {
// TODO:    int i;
// TODO:    textBuffer* buf = ((TextWidget)w)->text.textD->buffer;
// TODO: 
// TODO:    for (i=0; i<buf->nModifyProcs; i++)
// TODO:    {
// TODO:       if (buf->modifyProcs[i] == modifiedCB && buf->cbArgs[i] == w)
// TODO:       {
// TODO:          BufRemoveModifyCB(buf, modifiedCB, buf->cbArgs[i]);
// TODO:          return;
// TODO:       }
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Copy the primary selection to the clipboard
// TODO: */
// TODO: void CopyToClipboard(Widget w, Time time)
// TODO: {
// TODO:    char* text;
// TODO:    long itemID = 0;
// TODO:    NeString s;
// TODO:    int stat, length;
// TODO: 
// TODO:    /* Get the selected text, if there's no selection, do nothing */
// TODO:    text = BufGetSelectionText(((TextWidget)w)->text.textD->buffer);
// TODO:    if (*text == '\0')
// TODO:    {
// TODO:       XtFree(text);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* If the string contained ascii-nul characters, something else was
// TODO:       substituted in the buffer.  Put the nulls back */
// TODO:    length = strlen(text);
// TODO:    BufUnsubstituteNullChars(text, ((TextWidget)w)->text.textD->buffer);
// TODO: 
// TODO:    /* Shut up LessTif */
// TODO:    if (SpinClipboardLock(XtDisplay(w), XtWindow(w)) != ClipboardSuccess)
// TODO:    {
// TODO:       XtFree(text);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Use the XmClipboard routines to copy the text to the clipboard.
// TODO:       If errors occur, just give up.  */
// TODO:    s = NeNewString("NEdit");
// TODO:    stat = SpinClipboardStartCopy(XtDisplay(w), XtWindow(w), s,
// TODO:                                  time, w, NULL, &itemID);
// TODO:    NeStringFree(s);
// TODO:    if (stat != ClipboardSuccess)
// TODO:    {
// TODO:       SpinClipboardUnlock(XtDisplay(w), XtWindow(w));
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Note that we were previously passing length + 1 here, but I suspect
// TODO:       that this was inconsistent with the somewhat ambiguous policy of
// TODO:       including a terminating null but not mentioning it in the length */
// TODO: 
// TODO:    if (SpinClipboardCopy(XtDisplay(w), XtWindow(w), itemID, "STRING",
// TODO:                          text, length, 0, NULL) != ClipboardSuccess)
// TODO:    {
// TODO:       XtFree(text);
// TODO:       SpinClipboardEndCopy(XtDisplay(w), XtWindow(w), itemID);
// TODO:       SpinClipboardUnlock(XtDisplay(w), XtWindow(w));
// TODO:       return;
// TODO:    }
// TODO:    XtFree(text);
// TODO:    SpinClipboardEndCopy(XtDisplay(w), XtWindow(w), itemID);
// TODO:    SpinClipboardUnlock(XtDisplay(w), XtWindow(w));
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Insert the X PRIMARY selection (from whatever window currently owns it)
// TODO: ** at the cursor position.
// TODO: */
// TODO: void InsertPrimarySelection(Widget w, Time time, int isColumnar)
// TODO: {
// TODO:    static int isColFlag;
// TODO: 
// TODO:    /* Theoretically, strange things could happen if the user managed to get
// TODO:       in any events between requesting receiving the selection data, however,
// TODO:       getSelectionCB simply inserts the selection at the cursor.  Don't
// TODO:       bother with further measures until real problems are observed. */
// TODO:    isColFlag = isColumnar;
// TODO:    XtGetSelectionValue(w, XA_PRIMARY, XA_STRING, getSelectionCB, &isColFlag,
// TODO:                        time);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Insert the secondary selection at the motif destination by initiating
// TODO: ** an INSERT_SELECTION request to the current owner of the MOTIF_DESTINATION
// TODO: ** selection.  Upon completion, unselect the secondary selection.  If
// TODO: ** "removeAfter" is true, also delete the secondary selection from the
// TODO: ** widget's buffer upon completion.
// TODO: */
// TODO: void SendSecondarySelection(Widget w, Time time, int removeAfter)
// TODO: {
// TODO:    sendSecondary(w, time, getAtom(XtDisplay(w), A_MOTIF_DESTINATION),
// TODO:                  removeAfter ? REMOVE_SECONDARY : UNSELECT_SECONDARY, NULL, 0);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Exchange Primary and secondary selections (to be called by the widget
// TODO: ** with the secondary selection)
// TODO: */
// TODO: void ExchangeSelections(Widget w, Time time)
// TODO: {
// TODO:    if (!((TextWidget)w)->text.textD->buffer->secondary.selected)
// TODO:       return;
// TODO: 
// TODO:    /* Initiate an long series of events: 1) get the primary selection,
// TODO:       2) replace the primary selection with this widget's secondary, 3) replace
// TODO:       this widget's secondary with the text returned from getting the primary
// TODO:       selection.  This could be done with a much more efficient MULTIPLE
// TODO:       request following ICCCM conventions, but the X toolkit MULTIPLE handling
// TODO:       routines can't handle INSERT_SELECTION requests inside of MULTIPLE
// TODO:       requests, because they don't allow access to the requested property atom
// TODO:       in  inside of an XtConvertSelectionProc.  It's simply not worth
// TODO:       duplicating all of Xt's selection handling routines for a little
// TODO:       performance, and this would make the code incompatible with Motif text
// TODO:       widgets */
// TODO:    XtGetSelectionValue(w, XA_PRIMARY, XA_STRING, getExchSelCB, NULL, time);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Insert the contents of the PRIMARY selection at the cursor position in
// TODO: ** widget "w" and delete the contents of the selection in its current owner
// TODO: ** (if the selection owner supports DELETE targets).
// TODO: */
// TODO: void MovePrimarySelection(Widget w, Time time, int isColumnar)
// TODO: {
// TODO:    static Atom targets[2] = {XA_STRING};
// TODO:    static int isColFlag;
// TODO:    static XtPointer clientData[2] =
// TODO:    {(XtPointer)& isColFlag, (XtPointer)& isColFlag};
// TODO: 
// TODO:    targets[1] = getAtom(XtDisplay(w), A_DELETE);
// TODO:    isColFlag = isColumnar;
// TODO:    /* some strangeness here: the selection callback appears to be getting
// TODO:       clientData[1] for targets[0] */
// TODO:    XtGetSelectionValues(w, XA_PRIMARY, targets, 2, getSelectionCB,
// TODO:                         clientData, time);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Insert the X CLIPBOARD selection at the cursor position.  If isColumnar,
// TODO: ** do an BufInsertCol for a columnar paste instead of BufInsert.
// TODO: */
// TODO: void InsertClipboard(Widget w, int isColumnar)
// TODO: {
// TODO:    unsigned long length, retLength;
// TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO:    textBuffer* buf = ((TextWidget)w)->text.textD->buffer;
// TODO:    int cursorLineStart, column, cursorPos;
// TODO:    char* string;
// TODO:    long id = 0;
// TODO: 
// TODO:    /* Get the clipboard contents.  Note: this code originally used the
// TODO:       CLIPBOARD selection, rather than the Motif clipboard interface.  It
// TODO:       was changed because Motif widgets in the same application would hang
// TODO:       when users pasted data from nedit text widgets.  This happened because
// TODO:       the XmClipboard routines used by the widgets do blocking event reads,
// TODO:       preventing a response by a selection owner in the same application.
// TODO:       While the Motif clipboard routines as they are used below, limit the
// TODO:       size of the data that be transferred via the clipboard, and are
// TODO:       generally slower and buggier, they do preserve the clipboard across
// TODO:       widget destruction and even program termination. */
// TODO:    if (SpinClipboardInquireLength(XtDisplay(w), XtWindow(w), "STRING", &length)
// TODO:          != ClipboardSuccess || length == 0)
// TODO:    {
// TODO:       /*
// TODO:        * Possibly, the clipboard can remain in a locked state after
// TODO:        * a failure, so we try to remove the lock, just to be sure.
// TODO:        */
// TODO:       SpinClipboardUnlock(XtDisplay(w), XtWindow(w));
// TODO:       return;
// TODO:    }
// TODO:    string = malloc__(length+1);
// TODO:    if (SpinClipboardRetrieve(XtDisplay(w), XtWindow(w), "STRING", string,
// TODO:                              length, &retLength, &id) != ClipboardSuccess || retLength == 0)
// TODO:    {
// TODO:       XtFree(string);
// TODO:       /*
// TODO:        * Possibly, the clipboard can remain in a locked state after
// TODO:        * a failure, so we try to remove the lock, just to be sure.
// TODO:        */
// TODO:       SpinClipboardUnlock(XtDisplay(w), XtWindow(w));
// TODO:       return;
// TODO:    }
// TODO:    string[retLength] = '\0';
// TODO: 
// TODO:    /* If the string contains ascii-nul characters, substitute something
// TODO:       else, or give up, warn, and refuse */
// TODO:    if (!BufSubstituteNullChars(string, retLength, buf))
// TODO:    {
// TODO:       fprintf(stderr, "Too much binary data, text not pasted\n");
// TODO:       XtFree(string);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Insert it in the text widget */
// TODO:    if (isColumnar && !buf->primary.selected)
// TODO:    {
// TODO:       cursorPos = TextDGetInsertPosition(textD);
// TODO:       cursorLineStart = BufStartOfLine(buf, cursorPos);
// TODO:       column = BufCountDispChars(buf, cursorLineStart, cursorPos);
// TODO:       if (((TextWidget)w)->text.overstrike)
// TODO:       {
// TODO:          BufOverlayRect(buf, cursorLineStart, column, -1, string, NULL,
// TODO:                         NULL);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          BufInsertCol(buf, column, cursorLineStart, string, NULL, NULL);
// TODO:       }
// TODO:       TextDSetInsertPosition(textD,
// TODO:                              BufCountForwardDispChars(buf, cursorLineStart, column));
// TODO:       if (((TextWidget)w)->text.autoShowInsertPos)
// TODO:          TextDMakeInsertPosVisible(textD);
// TODO:    }
// TODO:    else
// TODO:       TextInsertAtCursor(w, string, NULL, true,
// TODO:                          ((TextWidget)w)->text.autoWrapPastedText);
// TODO:    XtFree(string);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Take ownership of the MOTIF_DESTINATION selection.  This is Motif's private
// TODO: ** selection type for designating a widget to receive the result of
// TODO: ** secondary quick action requests.  The NEdit text widget uses this also
// TODO: ** for compatibility with Motif text widgets.
// TODO: */
// TODO: void TakeMotifDestination(Widget w, Time time)
// TODO: {
// TODO:    if (((TextWidget)w)->text.motifDestOwner || ((TextWidget)w)->text.readOnly)
// TODO:       return;
// TODO: 
// TODO:    /* Take ownership of the MOTIF_DESTINATION selection */
// TODO:    if (!XtOwnSelection(w, getAtom(XtDisplay(w), A_MOTIF_DESTINATION), time,
// TODO:                        convertMotifDestCB, loseMotifDestCB, NULL))
// TODO:    {
// TODO:       return;
// TODO:    }
// TODO:    ((TextWidget)w)->text.motifDestOwner = true;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** This routine is called every time there is a modification made to the
// TODO: ** buffer to which this callback is attached, with an argument of the text
// TODO: ** widget that has been designated (by HandleXSelections) to handle its
// TODO: ** selections.  It checks if the status of the selection in the buffer
// TODO: ** has changed since last time, and owns or disowns the X selection depending
// TODO: ** on the status of the primary selection in the buffer.  If it is not allowed
// TODO: ** to take ownership of the selection, it unhighlights the text in the buffer
// TODO: ** (Being in the middle of a modify callback, this has a somewhat complicated
// TODO: ** result, since later callbacks will see the second modifications first).
// TODO: */
// TODO: static void modifiedCB(int pos, int nInserted, int nDeleted,
// TODO:                        int nRestyled, const char* deletedText, void* cbArg)
// TODO: {
// TODO:    TextWidget w = (TextWidget)cbArg;
// TODO:    Time time = XtLastTimestampProcessed(XtDisplay((Widget)w));
// TODO:    int selected = w->text.textD->buffer->primary.selected;
// TODO:    int isOwner = w->text.selectionOwner;
// TODO: 
// TODO:    /* If the widget owns the selection and the buffer text is still selected,
// TODO:       or if the widget doesn't own it and there's no selection, do nothing */
// TODO:    if ((isOwner && selected) || (!isOwner && !selected))
// TODO:       return;
// TODO: 
// TODO:    /* Don't disown the selection here.  Another application (namely: klipper)
// TODO:       may try to take it when it thinks nobody has the selection.  We then
// TODO:       lose it, making selection-based macro operations fail.  Disowning
// TODO:       is really only for when the widget is destroyed to avoid a convert
// TODO:       callback from firing at a bad time. */
// TODO: 
// TODO:    /* Take ownership of the selection */
// TODO:    if (!XtOwnSelection((Widget)w, XA_PRIMARY, time, convertSelectionCB,
// TODO:                        loseSelectionCB, NULL))
// TODO:       BufUnselect(w->text.textD->buffer);
// TODO:    else
// TODO:       w->text.selectionOwner = true;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Send an INSERT_SELECTION request to "sel".
// TODO: ** Upon completion, do the action specified by "action" (one of enum
// TODO: ** selectNotifyActions) using "actionText" and freeing actionText (if
// TODO: ** not NULL) when done.
// TODO: */
// TODO: static void sendSecondary(Widget w, Time time, Atom sel, int action,
// TODO:                           char* actionText, int actionTextLen)
// TODO: {
// TODO:    static Atom selInfoProp[2] = {XA_SECONDARY, XA_STRING};
// TODO:    Display* disp = XtDisplay(w);
// TODO:    selectNotifyInfo* cbInfo;
// TODO:    XtAppContext context = XtWidgetToApplicationContext((Widget)w);
// TODO: 
// TODO:    /* Take ownership of the secondary selection, give up if we can't */
// TODO:    if (!XtOwnSelection(w, XA_SECONDARY, time, convertSecondaryCB,
// TODO:                        loseSecondaryCB, NULL))
// TODO:    {
// TODO:       BufSecondaryUnselect(((TextWidget)w)->text.textD->buffer);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Set up a property on this window to pass along with the
// TODO:       INSERT_SELECTION request to tell the MOTIF_DESTINATION owner what
// TODO:       selection and what target from that selection to insert */
// TODO:    XChangeProperty(disp, XtWindow(w), getAtom(disp, A_INSERT_INFO),
// TODO:                    getAtom(disp, A_ATOM_PAIR), 32, PropModeReplace,
// TODO:                    (unsigned char*)selInfoProp, 2 /* 1? */);
// TODO: 
// TODO:    /* Make INSERT_SELECTION request to the owner of selection "sel"
// TODO:       to do the insert.  This must be done using XLib calls to specify
// TODO:       the property with the information about what to insert.  This
// TODO:       means it also requires an event handler to see if the request
// TODO:       succeeded or not, and a backup timer to clean up if the select
// TODO:       notify event is never returned */
// TODO:    XConvertSelection(XtDisplay(w), sel, getAtom(disp, A_INSERT_SELECTION),
// TODO:                      getAtom(disp, A_INSERT_INFO), XtWindow(w), time);
// TODO:    cbInfo = (selectNotifyInfo*)malloc__(sizeof(selectNotifyInfo));
// TODO:    cbInfo->action = action;
// TODO:    cbInfo->timeStamp = time;
// TODO:    cbInfo->widget = (Widget)w;
// TODO:    cbInfo->actionText = actionText;
// TODO:    cbInfo->length = actionTextLen;
// TODO:    XtAddEventHandler(w, 0, true, selectNotifyEH, (XtPointer)cbInfo);
// TODO:    cbInfo->timeoutProcID = XtAppAddTimeOut(context,
// TODO:                                            XtAppGetSelectionTimeout(context),
// TODO:                                            selectNotifyTimerProc, (XtPointer)cbInfo);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Called when data arrives from a request for the PRIMARY selection.  If
// TODO: ** everything is in order, it inserts it at the cursor in the requesting
// TODO: ** widget.
// TODO: */
// TODO: static void getSelectionCB(Widget w, XtPointer clientData, Atom* selType,
// TODO:                            Atom* type, XtPointer value, unsigned long* length, int* format)
// TODO: {
// TODO:    textDisp* textD = ((TextWidget)w)->text.textD;
// TODO:    int isColumnar = *(int*)clientData;
// TODO:    int cursorLineStart, cursorPos, column, row;
// TODO:    char* string;
// TODO: 
// TODO:    /* Confirm that the returned value is of the correct type */
// TODO:    if (*type != XA_STRING || *format != 8)
// TODO:    {
// TODO:       XtFree((char*) value);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Copy the string just to make space for the null character (this may
// TODO:       not be necessary, XLib documentation claims a NULL is already added,
// TODO:       but the Xt documentation for this routine makes no such claim) */
// TODO:    string = malloc__(*length + 1);
// TODO:    memcpy(string, (char*)value, *length);
// TODO:    string[*length] = '\0';
// TODO: 
// TODO:    /* If the string contains ascii-nul characters, substitute something
// TODO:       else, or give up, warn, and refuse */
// TODO:    if (!BufSubstituteNullChars(string, *length, textD->buffer))
// TODO:    {
// TODO:       fprintf(stderr, "Too much binary data, giving up\n");
// TODO:       XtFree(string);
// TODO:       XtFree((char*)value);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Insert it in the text widget */
// TODO:    if (isColumnar)
// TODO:    {
// TODO:       cursorPos = TextDGetInsertPosition(textD);
// TODO:       cursorLineStart = BufStartOfLine(textD->buffer, cursorPos);
// TODO:       TextDXYToUnconstrainedPosition(textD, ((TextWidget)w)->text.btnDownX,
// TODO:                                      ((TextWidget)w)->text.btnDownY, &row, &column);
// TODO:       BufInsertCol(textD->buffer, column, cursorLineStart, string, NULL,NULL);
// TODO:       TextDSetInsertPosition(textD, textD->buffer->cursorPosHint);
// TODO:    }
// TODO:    else
// TODO:       TextInsertAtCursor(w, string, NULL, false,
// TODO:                          ((TextWidget)w)->text.autoWrapPastedText);
// TODO:    XtFree(string);
// TODO: 
// TODO:    /* The selection requstor is required to free__ the memory passed
// TODO:       to it via value */
// TODO:    XtFree((char*)value);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Called when data arrives from request resulting from processing an
// TODO: ** INSERT_SELECTION request.  If everything is in order, inserts it at
// TODO: ** the cursor or replaces pending delete selection in widget "w", and sets
// TODO: ** the flag passed in clientData to SUCCESSFUL_INSERT or UNSUCCESSFUL_INSERT
// TODO: ** depending on the success of the operation.
// TODO: */
// TODO: static void getInsertSelectionCB(Widget w, XtPointer clientData,Atom* selType,
// TODO:                                  Atom* type, XtPointer value, unsigned long* length, int* format)
// TODO: {
// TODO:    textBuffer* buf = ((TextWidget)w)->text.textD->buffer;
// TODO:    char* string;
// TODO:    int* resultFlag = (int*)clientData;
// TODO: 
// TODO:    /* Confirm that the returned value is of the correct type */
// TODO:    if (*type != XA_STRING || *format != 8 || value == NULL)
// TODO:    {
// TODO:       XtFree((char*) value);
// TODO:       *resultFlag = UNSUCCESSFUL_INSERT;
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Copy the string just to make space for the null character */
// TODO:    string = malloc__(*length + 1);
// TODO:    memcpy(string, (char*)value, *length);
// TODO:    string[*length] = '\0';
// TODO: 
// TODO:    /* If the string contains ascii-nul characters, substitute something
// TODO:       else, or give up, warn, and refuse */
// TODO:    if (!BufSubstituteNullChars(string, *length, buf))
// TODO:    {
// TODO:       fprintf(stderr, "Too much binary data, giving up\n");
// TODO:       XtFree(string);
// TODO:       XtFree((char*)value);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Insert it in the text widget */
// TODO:    TextInsertAtCursor(w, string, NULL, true,
// TODO:                       ((TextWidget)w)->text.autoWrapPastedText);
// TODO:    XtFree(string);
// TODO:    *resultFlag = SUCCESSFUL_INSERT;
// TODO: 
// TODO:    /* This callback is required to free__ the memory passed to it thru value */
// TODO:    XtFree((char*)value);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Called when data arrives from an X primary selection request for the
// TODO: ** purpose of exchanging the primary and secondary selections.
// TODO: ** If everything is in order, stores the retrieved text temporarily and
// TODO: ** initiates a request to replace the primary selection with this widget's
// TODO: ** secondary selection.
// TODO: */
// TODO: static void getExchSelCB(Widget w, XtPointer clientData, Atom* selType,
// TODO:                          Atom* type, XtPointer value, unsigned long* length, int* format)
// TODO: {
// TODO:    /* Confirm that there is a value and it is of the correct type */
// TODO:    if (*length == 0 || value == NULL || *type != XA_STRING || *format != 8)
// TODO:    {
// TODO:       XtFree((char*) value);
// TODO:       XBell(XtDisplay(w), 0);
// TODO:       BufSecondaryUnselect(((TextWidget)w)->text.textD->buffer);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Request the selection owner to replace the primary selection with
// TODO:       this widget's secondary selection.  When complete, replace this
// TODO:       widget's secondary selection with text "value" and free__ it. */
// TODO:    sendSecondary(w, XtLastTimestampProcessed(XtDisplay(w)), XA_PRIMARY,
// TODO:                  EXCHANGE_SECONDARY, (char*)value, *length);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Selection converter procedure used by the widget when it is the selection
// TODO: ** owner to provide data in the format requested by the selection requestor.
// TODO: **
// TODO: ** Note: Memory left in the *value field is freed by Xt as long as there is no
// TODO: ** done_proc procedure registered in the XtOwnSelection call where this
// TODO: ** procdeure is registered
// TODO: */
// TODO: static bool convertSelectionCB(Widget w, Atom* selType, Atom* target,
// TODO:                                   Atom* type, XtPointer* value, unsigned long* length, int* format)
// TODO: {
// TODO:    XSelectionRequestEvent* event = XtGetSelectionRequest(w, *selType, 0);
// TODO:    textBuffer* buf = ((TextWidget)w)->text.textD->buffer;
// TODO:    Display* display = XtDisplay(w);
// TODO:    Atom* targets, dummyAtom;
// TODO:    unsigned long nItems, dummyULong;
// TODO:    Atom* reqAtoms;
// TODO:    int getFmt, result = INSERT_WAITING;
// TODO:    XEvent nextEvent;
// TODO: 
// TODO:    /* target is text, string, or compound text */
// TODO:    if (*target == XA_STRING || *target == getAtom(display, A_TEXT) ||
// TODO:          *target == getAtom(display, A_COMPOUND_TEXT))
// TODO:    {
// TODO:       /* We really don't directly support COMPOUND_TEXT, but recent
// TODO:          versions gnome-terminal incorrectly ask for it, even though
// TODO:          don't declare that we do.  Just reply in string format. */
// TODO:       *type = XA_STRING;
// TODO:       *value = (XtPointer)BufGetSelectionText(buf);
// TODO:       *length = strlen((char*)*value);
// TODO:       *format = 8;
// TODO:       BufUnsubstituteNullChars(*value, buf);
// TODO:       return true;
// TODO:    }
// TODO: 
// TODO:    /* target is "TARGETS", return a list of targets we can handle */
// TODO:    if (*target == getAtom(display, A_TARGETS))
// TODO:    {
// TODO:       targets = (Atom*)malloc__(sizeof(Atom) * N_SELECT_TARGETS);
// TODO:       targets[0] = XA_STRING;
// TODO:       targets[1] = getAtom(display, A_TEXT);
// TODO:       targets[2] = getAtom(display, A_TARGETS);
// TODO:       targets[3] = getAtom(display, A_MULTIPLE);
// TODO:       targets[4] = getAtom(display, A_TIMESTAMP);
// TODO:       targets[5] = getAtom(display, A_INSERT_SELECTION);
// TODO:       targets[6] = getAtom(display, A_DELETE);
// TODO:       *type = XA_ATOM;
// TODO:       *value = (XtPointer)targets;
// TODO:       *length = N_SELECT_TARGETS;
// TODO:       *format = 32;
// TODO:       return true;
// TODO:    }
// TODO: 
// TODO:    /* target is "INSERT_SELECTION":  1) get the information about what
// TODO:       selection and target to use to get the text to insert, from the
// TODO:       property named in the property field of the selection request event.
// TODO:       2) initiate a get value request for the selection and target named
// TODO:       in the property, and WAIT until it completes */
// TODO:    if (*target == getAtom(display, A_INSERT_SELECTION))
// TODO:    {
// TODO:       if (((TextWidget)w)->text.readOnly)
// TODO:          return false;
// TODO:       if (XGetWindowProperty(event->display, event->requestor,
// TODO:                              event->property, 0, 2, false, AnyPropertyType, &dummyAtom,
// TODO:                              &getFmt, &nItems, &dummyULong,
// TODO:                              (unsigned char**)&reqAtoms) != Success ||
// TODO:             getFmt != 32 || nItems != 2)
// TODO:          return false;
// TODO:       if (reqAtoms[1] != XA_STRING)
// TODO:          return false;
// TODO:       XtGetSelectionValue(w, reqAtoms[0], reqAtoms[1],
// TODO:                           getInsertSelectionCB, &result, event->time);
// TODO:       XFree((char*)reqAtoms);
// TODO:       while (result == INSERT_WAITING)
// TODO:       {
// TODO:          XtAppNextEvent(XtWidgetToApplicationContext(w), &nextEvent);
// TODO:          XtDispatchEvent(&nextEvent);
// TODO:       }
// TODO:       *type = getAtom(display, A_INSERT_SELECTION);
// TODO:       *format = 8;
// TODO:       *value = NULL;
// TODO:       *length = 0;
// TODO:       return result == SUCCESSFUL_INSERT;
// TODO:    }
// TODO: 
// TODO:    /* target is "DELETE": delete primary selection */
// TODO:    if (*target == getAtom(display, A_DELETE))
// TODO:    {
// TODO:       BufRemoveSelected(buf);
// TODO:       *length = 0;
// TODO:       *format = 8;
// TODO:       *type = getAtom(display, A_DELETE);
// TODO:       *value = NULL;
// TODO:       return true;
// TODO:    }
// TODO: 
// TODO:    /* targets TIMESTAMP and MULTIPLE are handled by the toolkit, any
// TODO:       others are unrecognized, return false */
// TODO:    return false;
// TODO: }
// TODO: 
// TODO: static void loseSelectionCB(Widget w, Atom* selType)
// TODO: {
// TODO:    TextWidget tw = (TextWidget)w;
// TODO:    selection* sel = &tw->text.textD->buffer->primary;
// TODO:    char zeroWidth = sel->rectangular ? sel->zeroWidth : 0;
// TODO: 
// TODO:    /* For zero width rect. sel. we give up the selection but keep the
// TODO:        zero width tag. */
// TODO:    tw->text.selectionOwner = false;
// TODO:    BufUnselect(tw->text.textD->buffer);
// TODO:    sel->zeroWidth = zeroWidth;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Selection converter procedure used by the widget to (temporarily) provide
// TODO: ** the secondary selection data to a single requestor who has been asked
// TODO: ** to insert it.
// TODO: */
// TODO: static bool convertSecondaryCB(Widget w, Atom* selType, Atom* target,
// TODO:                                   Atom* type, XtPointer* value, unsigned long* length, int* format)
// TODO: {
// TODO:    textBuffer* buf = ((TextWidget)w)->text.textD->buffer;
// TODO: 
// TODO:    /* target must be string */
// TODO:    if (*target != XA_STRING && *target != getAtom(XtDisplay(w), A_TEXT))
// TODO:       return false;
// TODO: 
// TODO:    /* Return the contents of the secondary selection.  The memory allocated
// TODO:       here is freed by the X toolkit */
// TODO:    *type = XA_STRING;
// TODO:    *value = (XtPointer)BufGetSecSelectText(buf);
// TODO:    *length = strlen((char*)*value);
// TODO:    *format = 8;
// TODO:    BufUnsubstituteNullChars(*value, buf);
// TODO:    return true;
// TODO: }
// TODO: 
// TODO: static void loseSecondaryCB(Widget w, Atom* selType)
// TODO: {
// TODO:    /* do nothing, secondary selections are transient anyhow, and it
// TODO:       will go away on its own */
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Selection converter procedure used by the widget when it owns the Motif
// TODO: ** destination, to handle INSERT_SELECTION requests.
// TODO: */
// TODO: static bool convertMotifDestCB(Widget w, Atom* selType, Atom* target,
// TODO:                                   Atom* type, XtPointer* value, unsigned long* length, int* format)
// TODO: {
// TODO:    XSelectionRequestEvent* event = XtGetSelectionRequest(w, *selType, 0);
// TODO:    Display* display = XtDisplay(w);
// TODO:    Atom* targets, dummyAtom;
// TODO:    unsigned long nItems, dummyULong;
// TODO:    Atom* reqAtoms;
// TODO:    int getFmt, result = INSERT_WAITING;
// TODO:    XEvent nextEvent;
// TODO: 
// TODO:    /* target is "TARGETS", return a list of targets it can handle */
// TODO:    if (*target == getAtom(display, A_TARGETS))
// TODO:    {
// TODO:       targets = (Atom*)malloc__(sizeof(Atom) * 3);
// TODO:       targets[0] = getAtom(display, A_TARGETS);
// TODO:       targets[1] = getAtom(display, A_TIMESTAMP);
// TODO:       targets[2] = getAtom(display, A_INSERT_SELECTION);
// TODO:       *type = XA_ATOM;
// TODO:       *value = (XtPointer)targets;
// TODO:       *length = 3;
// TODO:       *format = 32;
// TODO:       return true;
// TODO:    }
// TODO: 
// TODO:    /* target is "INSERT_SELECTION":  1) get the information about what
// TODO:       selection and target to use to get the text to insert, from the
// TODO:       property named in the property field of the selection request event.
// TODO:       2) initiate a get value request for the selection and target named
// TODO:       in the property, and WAIT until it completes */
// TODO:    if (*target == getAtom(display, A_INSERT_SELECTION))
// TODO:    {
// TODO:       if (((TextWidget)w)->text.readOnly)
// TODO:          return false;
// TODO:       if (XGetWindowProperty(event->display, event->requestor,
// TODO:                              event->property, 0, 2, false, AnyPropertyType, &dummyAtom,
// TODO:                              &getFmt, &nItems, &dummyULong,
// TODO:                              (unsigned char**)&reqAtoms) != Success ||
// TODO:             getFmt != 32 || nItems != 2)
// TODO:          return false;
// TODO:       if (reqAtoms[1] != XA_STRING)
// TODO:          return false;
// TODO:       XtGetSelectionValue(w, reqAtoms[0], reqAtoms[1],
// TODO:                           getInsertSelectionCB, &result, event->time);
// TODO:       XFree((char*)reqAtoms);
// TODO:       while (result == INSERT_WAITING)
// TODO:       {
// TODO:          XtAppNextEvent(XtWidgetToApplicationContext(w), &nextEvent);
// TODO:          XtDispatchEvent(&nextEvent);
// TODO:       }
// TODO:       *type = getAtom(display, A_INSERT_SELECTION);
// TODO:       *format = 8;
// TODO:       *value = NULL;
// TODO:       *length = 0;
// TODO:       return result == SUCCESSFUL_INSERT;
// TODO:    }
// TODO: 
// TODO:    /* target TIMESTAMP is handled by the toolkit and not passed here, any
// TODO:       others are unrecognized */
// TODO:    return false;
// TODO: }
// TODO: 
// TODO: static void loseMotifDestCB(Widget w, Atom* selType)
// TODO: {
// TODO:    ((TextWidget)w)->text.motifDestOwner = false;
// TODO:    if (((TextWidget)w)->text.textD->cursorStyle == CARET_CURSOR)
// TODO:       TextDSetCursorStyle(((TextWidget)w)->text.textD, DIM_CURSOR);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Event handler for SelectionNotify events, to finish off INSERT_SELECTION
// TODO: ** requests which must be done through the lower
// TODO: ** level (and more complicated) XLib selection mechanism.  Matches the
// TODO: ** time stamp in the request against the time stamp stored when the selection
// TODO: ** request was made to find the selectionNotify event that it was installed
// TODO: ** to catch.  When it finds the correct event, it does the action it was
// TODO: ** installed to do, and removes itself and its backup timer (which would do
// TODO: ** the clean up if the selectionNotify event never arrived.)
// TODO: */
// TODO: static void selectNotifyEH(Widget w, XtPointer data, XEvent* event,
// TODO:                            bool* continueDispatch)
// TODO: {
// TODO:    textBuffer* buf = ((TextWidget)w)->text.textD->buffer;
// TODO:    XSelectionEvent* e = (XSelectionEvent*)event;
// TODO:    selectNotifyInfo* cbInfo = (selectNotifyInfo*)data;
// TODO:    int selStart, selEnd;
// TODO:    char* string;
// TODO: 
// TODO:    /* Check if this was the selection request for which this handler was
// TODO:       set up, if not, do nothing */
// TODO:    if (event->type != SelectionNotify || e->time != cbInfo->timeStamp)
// TODO:       return;
// TODO: 
// TODO:    /* The time stamp matched, remove this event handler and its
// TODO:       backup timer procedure */
// TODO:    XtRemoveEventHandler(w, 0, true, selectNotifyEH, data);
// TODO:    XtRemoveTimeOut(cbInfo->timeoutProcID);
// TODO: 
// TODO:    /* Check if the request succeeded, if not, beep, remove any existing
// TODO:       secondary selection, and return */
// TODO:    if (e->property == None)
// TODO:    {
// TODO:       XBell(XtDisplay(w), 0);
// TODO:       BufSecondaryUnselect(buf);
// TODO:       XtDisownSelection(w, XA_SECONDARY, e->time);
// TODO:       XtFree((char*) cbInfo->actionText);
// TODO:       XtFree((char*)cbInfo);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Do the requested action, if the action is exchange, also clean up
// TODO:       the properties created for returning the primary selection and making
// TODO:       the MULTIPLE target request */
// TODO:    if (cbInfo->action == REMOVE_SECONDARY)
// TODO:    {
// TODO:       BufRemoveSecSelect(buf);
// TODO:    }
// TODO:    else if (cbInfo->action == EXCHANGE_SECONDARY)
// TODO:    {
// TODO:       string = malloc__(cbInfo->length + 1);
// TODO:       memcpy(string, cbInfo->actionText, cbInfo->length);
// TODO:       string[cbInfo->length] = '\0';
// TODO:       selStart = buf->secondary.start;
// TODO:       if (BufSubstituteNullChars(string, cbInfo->length, buf))
// TODO:       {
// TODO:          BufReplaceSecSelect(buf, string);
// TODO:          if (buf->secondary.rectangular)
// TODO:          {
// TODO:             /*... it would be nice to re-select, but probably impossible */
// TODO:             TextDSetInsertPosition(((TextWidget)w)->text.textD,
// TODO:                                    buf->cursorPosHint);
// TODO:          }
// TODO:          else
// TODO:          {
// TODO:             selEnd = selStart + cbInfo->length;
// TODO:             BufSelect(buf, selStart, selEnd);
// TODO:             TextDSetInsertPosition(((TextWidget)w)->text.textD, selEnd);
// TODO:          }
// TODO:       }
// TODO:       else
// TODO:          fprintf(stderr, "Too much binary data\n");
// TODO:       XtFree(string);
// TODO:    }
// TODO:    BufSecondaryUnselect(buf);
// TODO:    XtDisownSelection(w, XA_SECONDARY, e->time);
// TODO:    XtFree((char*)cbInfo->actionText);
// TODO:    XtFree((char*)cbInfo);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Xt timer procedure for timeouts on XConvertSelection requests, cleans up
// TODO: ** after a complete failure of the selection mechanism to return a selection
// TODO: ** notify event for a convert selection request
// TODO: */
// TODO: static void selectNotifyTimerProc(XtPointer clientData, XtIntervalId* id)
// TODO: {
// TODO:    selectNotifyInfo* cbInfo = (selectNotifyInfo*)clientData;
// TODO:    textBuffer* buf = ((TextWidget)cbInfo->widget)->text.textD->buffer;
// TODO: 
// TODO:    fprintf(stderr, "NEdit: timeout on selection request\n");
// TODO:    XtRemoveEventHandler(cbInfo->widget, 0, true, selectNotifyEH, cbInfo);
// TODO:    BufSecondaryUnselect(buf);
// TODO:    XtDisownSelection(cbInfo->widget, XA_SECONDARY, cbInfo->timeStamp);
// TODO:    XtFree((char*) cbInfo->actionText);
// TODO:    XtFree((char*)cbInfo);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Maintain a cache of interned atoms.  To reference one, use the constant
// TODO: ** from the enum, atomIndex, above.
// TODO: */
// TODO: static Atom getAtom(Display* display, int atomNum)
// TODO: {
// TODO:    static Atom atomList[N_ATOMS] = {0};
// TODO:    static char* atomNames[N_ATOMS] = {"TEXT", "TARGETS", "MULTIPLE",
// TODO:                                       "TIMESTAMP", "INSERT_SELECTION", "DELETE", "CLIPBOARD",
// TODO:                                       "INSERT_INFO", "ATOM_PAIR", "MOTIF_DESTINATION", "COMPOUND_TEXT"
// TODO:                                      };
// TODO: 
// TODO:    if (atomList[atomNum] == 0)
// TODO:       atomList[atomNum] = XInternAtom(display, atomNames[atomNum], false);
// TODO:    return atomList[atomNum];
// TODO: }
// TODO: 