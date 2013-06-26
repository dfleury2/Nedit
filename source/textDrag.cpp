// TODO: static const char CVSID[] = "$Id: textDrag.c,v 1.11 2005/02/02 09:15:31 edg Exp $";
// TODO: /*******************************************************************************
// TODO: *									       *
// TODO: * textDrag.c - Text Dragging routines for NEdit text widget		       *
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
// TODO: #include "textDrag.h"
// TODO: #include "Ne_Text_Buffer.h"
// TODO: #include "Ne_Text_Display.h"
// TODO: #include "Ne_Text_Part.h"
// TODO: 
// TODO: #include <limits.h>
// TODO: 
// TODO: #include <X11/Intrinsic.h>
// TODO: #include <X11/IntrinsicP.h>
// TODO: #include <Xm/Xm.h>
// TODO: #include <Xm/XmP.h>
// TODO: #if XmVersion >= 1002
// TODO: #include <Xm/PrimitiveP.h>
// TODO: #endif
// TODO: 
// TODO: #ifdef HAVE_DEBUG_H
// TODO: #include "../debug.h"
// TODO: #endif
// TODO: 
// TODO: static void trackModifyRange(int* rangeStart, int* modRangeEnd,
// TODO:                              int* unmodRangeEnd, int modPos, int nInserted, int nDeleted);
// TODO: static void findTextMargins(textBuffer* buf, int start, int end, int* leftMargin,
// TODO:                             int* rightMargin);
// TODO: static int findRelativeLineStart(textBuffer* buf, int referencePos,
// TODO:                                  int referenceLineNum, int newLineNum);
// TODO: static int min3(int i1, int i2, int i3);
// TODO: static int max3(int i1, int i2, int i3);
// TODO: static int max(int i1, int i2);
// TODO: 
// TODO: /*
// TODO: ** Start the process of dragging the current primary-selected text across
// TODO: ** the window (move by dragging, as opposed to dragging to create the
// TODO: ** selection)
// TODO: */
// TODO: void BeginBlockDrag(TextWidget tw)
// TODO: {
// TODO:    textDisp* textD = tw->text.textD;
// TODO:    textBuffer* buf = textD->buffer;
// TODO:    int fontHeight = textD->fontStruct->ascent + textD->fontStruct->descent;
// TODO:    int fontWidth = textD->fontStruct->max_bounds.width;
// TODO:    selection* sel = &buf->primary;
// TODO:    int nLines, mousePos, lineStart;
// TODO:    int x, y, lineEnd;
// TODO: 
// TODO:    char* text;
// TODO: 
// TODO:    /* Save a copy of the whole text buffer as a backup, and for
// TODO:       deriving changes */
// TODO:    tw->text.dragOrigBuf = BufCreate();
// TODO:    BufSetTabDistance(tw->text.dragOrigBuf, buf->tabDist);
// TODO:    tw->text.dragOrigBuf->useTabs = buf->useTabs;
// TODO:    text = BufGetAll(buf);
// TODO:    BufSetAll(tw->text.dragOrigBuf, text);
// TODO:    XtFree(text);
// TODO:    if (sel->rectangular)
// TODO:       BufRectSelect(tw->text.dragOrigBuf, sel->start, sel->end, sel->rectStart,
// TODO:                     sel->rectEnd);
// TODO:    else
// TODO:       BufSelect(tw->text.dragOrigBuf, sel->start, sel->end);
// TODO: 
// TODO:    /* Record the mouse pointer offsets from the top left corner of the
// TODO:       selection (the position where text will actually be inserted In dragging
// TODO:       non-rectangular selections)  */
// TODO:    if (sel->rectangular)
// TODO:    {
// TODO:       tw->text.dragXOffset = tw->text.btnDownX + textD->horizOffset -
// TODO:                              textD->left - sel->rectStart * fontWidth;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if (!TextDPositionToXY(textD, sel->start, &x, &y))
// TODO:          x = BufCountDispChars(buf, TextDStartOfLine(textD, sel->start),
// TODO:                                sel->start) * fontWidth + textD->left -
// TODO:              textD->horizOffset;
// TODO:       tw->text.dragXOffset = tw->text.btnDownX - x;
// TODO:    }
// TODO:    mousePos = TextDXYToPosition(textD, tw->text.btnDownX, tw->text.btnDownY);
// TODO:    nLines = BufCountLines(buf, sel->start, mousePos);
// TODO:    tw->text.dragYOffset = nLines * fontHeight + (((tw->text.btnDownY -
// TODO:                           tw->text.marginHeight) % fontHeight) - fontHeight/2);
// TODO:    tw->text.dragNLines = BufCountLines(buf, sel->start, sel->end);
// TODO: 
// TODO:    /* Record the current drag insert position and the information for
// TODO:       undoing the fictional insert of the selection in its new position */
// TODO:    tw->text.dragInsertPos = sel->start;
// TODO:    tw->text.dragInserted = sel->end - sel->start;
// TODO:    if (sel->rectangular)
// TODO:    {
// TODO:       textBuffer* testBuf = BufCreate();
// TODO:       char* testText = BufGetRange(buf, sel->start, sel->end);
// TODO:       BufSetTabDistance(testBuf, buf->tabDist);
// TODO:       testBuf->useTabs = buf->useTabs;
// TODO:       BufSetAll(testBuf, testText);
// TODO:       XtFree(testText);
// TODO:       BufRemoveRect(testBuf, 0, sel->end - sel->start, sel->rectStart,
// TODO:                     sel->rectEnd);
// TODO:       tw->text.dragDeleted = testBuf->length;
// TODO:       BufFree(testBuf);
// TODO:       tw->text.dragRectStart = sel->rectStart;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       tw->text.dragDeleted = 0;
// TODO:       tw->text.dragRectStart = 0;
// TODO:    }
// TODO:    tw->text.dragType = DRAG_MOVE;
// TODO:    tw->text.dragSourceDeletePos = sel->start;
// TODO:    tw->text.dragSourceInserted = tw->text.dragDeleted;
// TODO:    tw->text.dragSourceDeleted = tw->text.dragInserted;
// TODO: 
// TODO:    /* For non-rectangular selections, fill in the rectangular information in
// TODO:       the selection for overlay mode drags which are done rectangularly */
// TODO:    if (!sel->rectangular)
// TODO:    {
// TODO:       lineStart = BufStartOfLine(buf, sel->start);
// TODO:       if (tw->text.dragNLines == 0)
// TODO:       {
// TODO:          tw->text.dragOrigBuf->primary.rectStart =
// TODO:             BufCountDispChars(buf, lineStart, sel->start);
// TODO:          tw->text.dragOrigBuf->primary.rectEnd =
// TODO:             BufCountDispChars(buf, lineStart, sel->end);
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          lineEnd = BufGetCharacter(buf, sel->end - 1) == '\n' ?
// TODO:                    sel->end - 1 : sel->end;
// TODO:          findTextMargins(buf, lineStart, lineEnd,
// TODO:                          &tw->text.dragOrigBuf->primary.rectStart,
// TODO:                          &tw->text.dragOrigBuf->primary.rectEnd);
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    /* Set the drag state to announce an ongoing block-drag */
// TODO:    tw->text.dragState = PRIMARY_BLOCK_DRAG;
// TODO: 
// TODO:    /* Call the callback announcing the start of a block drag */
// TODO:    XtCallCallbacks((Widget)tw, textNdragStartCallback, (XtPointer)NULL);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Reposition the primary-selected text that is being dragged as a block
// TODO: ** for a new mouse position of (x, y)
// TODO: */
// TODO: void BlockDragSelection(TextWidget tw, int x, int y, int dragType)
// TODO: {
// TODO:    textDisp* textD = tw->text.textD;
// TODO:    textBuffer* buf = textD->buffer;
// TODO:    int fontHeight = textD->fontStruct->ascent + textD->fontStruct->descent;
// TODO:    int fontWidth = textD->fontStruct->max_bounds.width;
// TODO:    textBuffer* origBuf = tw->text.dragOrigBuf;
// TODO:    int dragXOffset = tw->text.dragXOffset;
// TODO:    textBuffer* tempBuf;
// TODO:    selection* origSel = &origBuf->primary;
// TODO:    int rectangular = origSel->rectangular;
// TODO:    int overlay, oldDragType = tw->text.dragType;
// TODO:    int nLines = tw->text.dragNLines;
// TODO:    int insLineNum, insLineStart, insRectStart, insRectEnd, insStart;
// TODO:    char* repText, *text, *insText;
// TODO:    int modRangeStart = -1, tempModRangeEnd = -1, bufModRangeEnd = -1;
// TODO:    int referenceLine, referencePos, tempStart, tempEnd, origSelLen;
// TODO:    int insertInserted, insertDeleted, row, column;
// TODO:    int origSelLineStart, origSelLineEnd;
// TODO:    int sourceInserted, sourceDeleted, sourceDeletePos;
// TODO: 
// TODO:    if (tw->text.dragState != PRIMARY_BLOCK_DRAG)
// TODO:       return;
// TODO: 
// TODO:    /* The operation of block dragging is simple in theory, but not so simple
// TODO:       in practice.  There is a backup buffer (tw->text.dragOrigBuf) which
// TODO:       holds a copy of the buffer as it existed before the drag.  When the
// TODO:       user drags the mouse to a new location, this routine is called, and
// TODO:       a temporary buffer is created and loaded with the local part of the
// TODO:       buffer (from the backup) which might be changed by the drag.  The
// TODO:       changes are all made to this temporary buffer, and the parts of this
// TODO:       buffer which then differ from the real (displayed) buffer are used to
// TODO:       replace those parts, thus one replace operation serves as both undo
// TODO:       and modify.  This double-buffering of the operation prevents excessive
// TODO:       redrawing (though there is still plenty of needless redrawing due to
// TODO:       re-selection and rectangular operations).
// TODO: 
// TODO:       The hard part is keeping track of the changes such that a single replace
// TODO:       operation will do everyting.  This is done using a routine called
// TODO:       trackModifyRange which tracks expanding ranges of changes in the two
// TODO:       buffers in modRangeStart, tempModRangeEnd, and bufModRangeEnd. */
// TODO: 
// TODO:    /* Create a temporary buffer for accumulating changes which will
// TODO:       eventually be replaced in the real buffer.  Load the buffer with the
// TODO:       range of characters which might be modified in this drag step
// TODO:       (this could be tighter, but hopefully it's not too slow) */
// TODO:    tempBuf = BufCreate();
// TODO:    tempBuf->tabDist = buf->tabDist;
// TODO:    tempBuf->useTabs = buf->useTabs;
// TODO:    tempStart = min3(tw->text.dragInsertPos, origSel->start,
// TODO:                     BufCountBackwardNLines(buf, textD->firstChar, nLines+2));
// TODO:    tempEnd = BufCountForwardNLines(buf, max3(tw->text.dragInsertPos,
// TODO:                                    origSel->start, textD->lastChar), nLines+2) +
// TODO:              origSel->end - origSel->start;
// TODO:    text = BufGetRange(origBuf, tempStart, tempEnd);
// TODO:    BufSetAll(tempBuf, text);
// TODO:    XtFree(text);
// TODO: 
// TODO:    /* If the drag type is USE_LAST, use the last dragType applied */
// TODO:    if (dragType == USE_LAST)
// TODO:       dragType = tw->text.dragType;
// TODO:    overlay = dragType == DRAG_OVERLAY_MOVE || dragType == DRAG_OVERLAY_COPY;
// TODO: 
// TODO:    /* Overlay mode uses rectangular selections whether or not the original
// TODO:       was rectangular.  To use a plain selection as if it were rectangular,
// TODO:       the start and end positions need to be moved to the line boundaries
// TODO:       and trailing newlines must be excluded */
// TODO:    origSelLineStart = BufStartOfLine(origBuf, origSel->start);
// TODO:    if (!rectangular && BufGetCharacter(origBuf, origSel->end - 1) == '\n')
// TODO:       origSelLineEnd = origSel->end - 1;
// TODO:    else
// TODO:       origSelLineEnd = BufEndOfLine(origBuf, origSel->end);
// TODO:    if (!rectangular && overlay && nLines != 0)
// TODO:       dragXOffset -= fontWidth * (origSel->rectStart -
// TODO:                                   (origSel->start - origSelLineStart));
// TODO: 
// TODO:    /* If the drag operation is of a different type than the last one, and the
// TODO:       operation is a move, expand the modified-range to include undoing the
// TODO:       text-removal at the site from which the text was dragged. */
// TODO:    if (dragType != oldDragType && tw->text.dragSourceDeleted != 0)
// TODO:       trackModifyRange(&modRangeStart, &bufModRangeEnd, &tempModRangeEnd,
// TODO:                        tw->text.dragSourceDeletePos, tw->text.dragSourceInserted,
// TODO:                        tw->text.dragSourceDeleted);
// TODO: 
// TODO:    /* Do, or re-do the original text removal at the site where a move began.
// TODO:       If this part has not changed from the last call, do it silently to
// TODO:       bring the temporary buffer in sync with the real (displayed)
// TODO:       buffer.  If it's being re-done, track the changes to complete the
// TODO:       redo operation begun above */
// TODO:    if (dragType == DRAG_MOVE || dragType == DRAG_OVERLAY_MOVE)
// TODO:    {
// TODO:       if (rectangular || overlay)
// TODO:       {
// TODO:          int prevLen = tempBuf->length;
// TODO:          origSelLen = origSelLineEnd - origSelLineStart;
// TODO:          if (overlay)
// TODO:             BufClearRect(tempBuf, origSelLineStart-tempStart,
// TODO:                          origSelLineEnd-tempStart, origSel->rectStart,
// TODO:                          origSel->rectEnd);
// TODO:          else
// TODO:             BufRemoveRect(tempBuf, origSelLineStart-tempStart,
// TODO:                           origSelLineEnd-tempStart, origSel->rectStart,
// TODO:                           origSel->rectEnd);
// TODO:          sourceDeletePos = origSelLineStart;
// TODO:          sourceInserted = origSelLen - prevLen + tempBuf->length;
// TODO:          sourceDeleted = origSelLen;
// TODO:       }
// TODO:       else
// TODO:       {
// TODO:          BufRemove(tempBuf, origSel->start - tempStart,
// TODO:                    origSel->end - tempStart);
// TODO:          sourceDeletePos = origSel->start;
// TODO:          sourceInserted = 0;
// TODO:          sourceDeleted = origSel->end - origSel->start;
// TODO:       }
// TODO:       if (dragType != oldDragType)
// TODO:          trackModifyRange(&modRangeStart, &tempModRangeEnd, &bufModRangeEnd,
// TODO:                           sourceDeletePos, sourceInserted, sourceDeleted);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       sourceDeletePos = 0;
// TODO:       sourceInserted = 0;
// TODO:       sourceDeleted = 0;
// TODO:    }
// TODO: 
// TODO:    /* Expand the modified-range to include undoing the insert from the last
// TODO:       call. */
// TODO:    trackModifyRange(&modRangeStart, &bufModRangeEnd, &tempModRangeEnd,
// TODO:                     tw->text.dragInsertPos, tw->text.dragInserted, tw->text.dragDeleted);
// TODO: 
// TODO:    /* Find the line number and column of the insert position.  Note that in
// TODO:       continuous wrap mode, these must be calculated as if the text were
// TODO:       not wrapped */
// TODO:    TextDXYToUnconstrainedPosition(textD, max(0, x - dragXOffset),
// TODO:                                   max(0, y - (tw->text.dragYOffset % fontHeight)), &row, &column);
// TODO:    column = TextDOffsetWrappedColumn(textD, row, column);
// TODO:    row = TextDOffsetWrappedRow(textD, row);
// TODO:    insLineNum = row + textD->topLineNum - tw->text.dragYOffset / fontHeight;
// TODO: 
// TODO:    /* find a common point of reference between the two buffers, from which
// TODO:       the insert position line number can be translated to a position */
// TODO:    if (textD->firstChar > modRangeStart)
// TODO:    {
// TODO:       referenceLine = textD->topLineNum -
// TODO:                       BufCountLines(buf, modRangeStart, textD->firstChar);
// TODO:       referencePos = modRangeStart;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       referencePos = textD->firstChar;
// TODO:       referenceLine = textD->topLineNum;
// TODO:    }
// TODO: 
// TODO:    /* find the position associated with the start of the new line in the
// TODO:       temporary buffer */
// TODO:    insLineStart = findRelativeLineStart(tempBuf, referencePos - tempStart,
// TODO:                                         referenceLine, insLineNum) + tempStart;
// TODO:    if (insLineStart - tempStart == tempBuf->length)
// TODO:       insLineStart = BufStartOfLine(tempBuf, insLineStart - tempStart) +
// TODO:                      tempStart;
// TODO: 
// TODO:    /* Find the actual insert position */
// TODO:    if (rectangular || overlay)
// TODO:    {
// TODO:       insStart = insLineStart;
// TODO:       insRectStart = column;
// TODO:    }
// TODO:    else     /* note, this will fail with proportional fonts */
// TODO:    {
// TODO:       insStart = BufCountForwardDispChars(tempBuf, insLineStart - tempStart,
// TODO:                                           column) + tempStart;
// TODO:       insRectStart = 0;
// TODO:    }
// TODO: 
// TODO:    /* If the position is the same as last time, don't bother drawing (it
// TODO:       would be nice if this decision could be made earlier) */
// TODO:    if (insStart == tw->text.dragInsertPos &&
// TODO:          insRectStart == tw->text.dragRectStart && dragType == oldDragType)
// TODO:    {
// TODO:       BufFree(tempBuf);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Do the insert in the temporary buffer */
// TODO:    if (rectangular || overlay)
// TODO:    {
// TODO:       insText = BufGetTextInRect(origBuf, origSelLineStart, origSelLineEnd,
// TODO:                                  origSel->rectStart, origSel->rectEnd);
// TODO:       if (overlay)
// TODO:          BufOverlayRect(tempBuf, insStart - tempStart, insRectStart,
// TODO:                         insRectStart + origSel->rectEnd - origSel->rectStart,
// TODO:                         insText, &insertInserted, &insertDeleted);
// TODO:       else
// TODO:          BufInsertCol(tempBuf, insRectStart, insStart - tempStart, insText,
// TODO:                       &insertInserted, &insertDeleted);
// TODO:       trackModifyRange(&modRangeStart, &tempModRangeEnd, &bufModRangeEnd,
// TODO:                        insStart, insertInserted, insertDeleted);
// TODO:       XtFree(insText);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       insText = BufGetSelectionText(origBuf);
// TODO:       BufInsert(tempBuf, insStart - tempStart, insText);
// TODO:       trackModifyRange(&modRangeStart, &tempModRangeEnd, &bufModRangeEnd,
// TODO:                        insStart, origSel->end - origSel->start, 0);
// TODO:       insertInserted = origSel->end - origSel->start;
// TODO:       insertDeleted = 0;
// TODO:       XtFree(insText);
// TODO:    }
// TODO: 
// TODO:    /* Make the changes in the real buffer */
// TODO:    repText = BufGetRange(tempBuf, modRangeStart - tempStart,
// TODO:                          tempModRangeEnd - tempStart);
// TODO:    BufFree(tempBuf);
// TODO:    TextDBlankCursor(textD);
// TODO:    BufReplace(buf, modRangeStart, bufModRangeEnd, repText);
// TODO:    XtFree(repText);
// TODO: 
// TODO:    /* Store the necessary information for undoing this step */
// TODO:    tw->text.dragInsertPos = insStart;
// TODO:    tw->text.dragRectStart = insRectStart;
// TODO:    tw->text.dragInserted = insertInserted;
// TODO:    tw->text.dragDeleted = insertDeleted;
// TODO:    tw->text.dragSourceDeletePos = sourceDeletePos;
// TODO:    tw->text.dragSourceInserted = sourceInserted;
// TODO:    tw->text.dragSourceDeleted = sourceDeleted;
// TODO:    tw->text.dragType = dragType;
// TODO: 
// TODO:    /* Reset the selection and cursor position */
// TODO:    if (rectangular || overlay)
// TODO:    {
// TODO:       insRectEnd = insRectStart + origSel->rectEnd - origSel->rectStart;
// TODO:       BufRectSelect(buf, insStart, insStart + insertInserted, insRectStart,
// TODO:                     insRectEnd);
// TODO:       TextDSetInsertPosition(textD, BufCountForwardDispChars(buf,
// TODO:                              BufCountForwardNLines(buf, insStart, tw->text.dragNLines),
// TODO:                              insRectEnd));
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       BufSelect(buf, insStart, insStart + origSel->end - origSel->start);
// TODO:       TextDSetInsertPosition(textD, insStart + origSel->end - origSel->start);
// TODO:    }
// TODO:    TextDUnblankCursor(textD);
// TODO:    XtCallCallbacks((Widget)tw, textNcursorMovementCallback, (XtPointer)NULL);
// TODO:    tw->text.emTabsBeforeCursor = 0;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Complete a block text drag operation
// TODO: */
// TODO: void FinishBlockDrag(TextWidget tw)
// TODO: {
// TODO:    dragEndCBStruct endStruct;
// TODO:    int modRangeStart = -1, origModRangeEnd, bufModRangeEnd;
// TODO:    char* deletedText;
// TODO: 
// TODO:    /* Find the changed region of the buffer, covering both the deletion
// TODO:       of the selected text at the drag start position, and insertion at
// TODO:       the drag destination */
// TODO:    trackModifyRange(&modRangeStart, &bufModRangeEnd, &origModRangeEnd,
// TODO:                     tw->text.dragSourceDeletePos, tw->text.dragSourceInserted,
// TODO:                     tw->text.dragSourceDeleted);
// TODO:    trackModifyRange(&modRangeStart, &bufModRangeEnd, &origModRangeEnd,
// TODO:                     tw->text.dragInsertPos, tw->text.dragInserted,
// TODO:                     tw->text.dragDeleted);
// TODO: 
// TODO:    /* Get the original (pre-modified) range of text from saved backup buffer */
// TODO:    deletedText = BufGetRange(tw->text.dragOrigBuf, modRangeStart,
// TODO:                              origModRangeEnd);
// TODO: 
// TODO:    /* Free the backup buffer */
// TODO:    BufFree(tw->text.dragOrigBuf);
// TODO: 
// TODO:    /* Return to normal drag state */
// TODO:    tw->text.dragState = NOT_CLICKED;
// TODO: 
// TODO:    /* Call finish-drag calback */
// TODO:    endStruct.startPos = modRangeStart;
// TODO:    endStruct.nCharsDeleted = origModRangeEnd - modRangeStart;
// TODO:    endStruct.nCharsInserted = bufModRangeEnd - modRangeStart;
// TODO:    endStruct.deletedText = deletedText;
// TODO:    XtCallCallbacks((Widget)tw, textNdragEndCallback, (XtPointer)&endStruct);
// TODO:    XtFree(deletedText);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Cancel a block drag operation
// TODO: */
// TODO: void CancelBlockDrag(TextWidget tw)
// TODO: {
// TODO:    textBuffer* buf = tw->text.textD->buffer;
// TODO:    textBuffer* origBuf = tw->text.dragOrigBuf;
// TODO:    selection* origSel = &origBuf->primary;
// TODO:    int modRangeStart = -1, origModRangeEnd, bufModRangeEnd;
// TODO:    char* repText;
// TODO:    dragEndCBStruct endStruct;
// TODO: 
// TODO:    /* If the operation was a move, make the modify range reflect the
// TODO:       removal of the text from the starting position */
// TODO:    if (tw->text.dragSourceDeleted != 0)
// TODO:       trackModifyRange(&modRangeStart, &bufModRangeEnd, &origModRangeEnd,
// TODO:                        tw->text.dragSourceDeletePos, tw->text.dragSourceInserted,
// TODO:                        tw->text.dragSourceDeleted);
// TODO: 
// TODO:    /* Include the insert being undone from the last step in the modified
// TODO:       range. */
// TODO:    trackModifyRange(&modRangeStart, &bufModRangeEnd, &origModRangeEnd,
// TODO:                     tw->text.dragInsertPos, tw->text.dragInserted, tw->text.dragDeleted);
// TODO: 
// TODO:    /* Make the changes in the buffer */
// TODO:    repText = BufGetRange(origBuf, modRangeStart, origModRangeEnd);
// TODO:    BufReplace(buf, modRangeStart, bufModRangeEnd, repText);
// TODO:    XtFree(repText);
// TODO: 
// TODO:    /* Reset the selection and cursor position */
// TODO:    if (origSel->rectangular)
// TODO:       BufRectSelect(buf, origSel->start, origSel->end, origSel->rectStart,
// TODO:                     origSel->rectEnd);
// TODO:    else
// TODO:       BufSelect(buf, origSel->start, origSel->end);
// TODO:    TextDSetInsertPosition(tw->text.textD, buf->cursorPosHint);
// TODO:    XtCallCallbacks((Widget)tw, textNcursorMovementCallback, NULL);
// TODO:    tw->text.emTabsBeforeCursor = 0;
// TODO: 
// TODO:    /* Free the backup buffer */
// TODO:    BufFree(origBuf);
// TODO: 
// TODO:    /* Indicate end of drag */
// TODO:    tw->text.dragState = DRAG_CANCELED;
// TODO: 
// TODO:    /* Call finish-drag calback */
// TODO:    endStruct.startPos = 0;
// TODO:    endStruct.nCharsDeleted = 0;
// TODO:    endStruct.nCharsInserted = 0;
// TODO:    endStruct.deletedText = NULL;
// TODO:    XtCallCallbacks((Widget)tw, textNdragEndCallback, (XtPointer)&endStruct);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Maintain boundaries of changed region between two buffers which
// TODO: ** start out with identical contents, but diverge through insertion,
// TODO: ** deletion, and replacement, such that the buffers can be reconciled
// TODO: ** by replacing the changed region of either buffer with the changed
// TODO: ** region of the other.
// TODO: **
// TODO: ** rangeStart is the beginning of the modification region in the shared
// TODO: ** coordinates of both buffers (which are identical up to rangeStart).
// TODO: ** modRangeEnd is the end of the changed region for the buffer being
// TODO: ** modified, unmodRangeEnd is the end of the region for the buffer NOT
// TODO: ** being modified.  A value of -1 in rangeStart indicates that there
// TODO: ** have been no modifications so far.
// TODO: */
// TODO: static void trackModifyRange(int* rangeStart, int* modRangeEnd,
// TODO:                              int* unmodRangeEnd, int modPos, int nInserted, int nDeleted)
// TODO: {
// TODO:    if (*rangeStart == -1)
// TODO:    {
// TODO:       *rangeStart = modPos;
// TODO:       *modRangeEnd = modPos + nInserted;
// TODO:       *unmodRangeEnd = modPos + nDeleted;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       if (modPos < *rangeStart)
// TODO:          *rangeStart = modPos;
// TODO:       if (modPos + nDeleted > *modRangeEnd)
// TODO:       {
// TODO:          *unmodRangeEnd += modPos + nDeleted - *modRangeEnd;
// TODO:          *modRangeEnd = modPos + nInserted;
// TODO:       }
// TODO:       else
// TODO:          *modRangeEnd += nInserted - nDeleted;
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Find the left and right margins of text between "start" and "end" in
// TODO: ** buffer "buf".  Note that "start is assumed to be at the start of a line.
// TODO: */
// TODO: static void findTextMargins(textBuffer* buf, int start, int end, int* leftMargin,
// TODO:                             int* rightMargin)
// TODO: {
// TODO:    char c;
// TODO:    int pos, width = 0, maxWidth = 0, minWhite = INT_MAX, inWhite = true;
// TODO: 
// TODO:    for (pos=start; pos<end; pos++)
// TODO:    {
// TODO:       c = BufGetCharacter(buf, pos);
// TODO:       if (inWhite && c != ' ' && c != '\t')
// TODO:       {
// TODO:          inWhite = false;
// TODO:          if (width < minWhite)
// TODO:             minWhite = width;
// TODO:       }
// TODO:       if (c == '\n')
// TODO:       {
// TODO:          if (width > maxWidth)
// TODO:             maxWidth = width;
// TODO:          width = 0;
// TODO:          inWhite = true;
// TODO:       }
// TODO:       else
// TODO:          width += BufCharWidth(c, width, buf->tabDist, buf->nullSubsChar);
// TODO:    }
// TODO:    if (width > maxWidth)
// TODO:       maxWidth = width;
// TODO:    *leftMargin = minWhite == INT_MAX ? 0 : minWhite;
// TODO:    *rightMargin = maxWidth;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Find a text position in buffer "buf" by counting forward or backward
// TODO: ** from a reference position with known line number
// TODO: */
// TODO: static int findRelativeLineStart(textBuffer* buf, int referencePos,
// TODO:                                  int referenceLineNum, int newLineNum)
// TODO: {
// TODO:    if (newLineNum < referenceLineNum)
// TODO:       return BufCountBackwardNLines(buf, referencePos,
// TODO:                                     referenceLineNum - newLineNum);
// TODO:    else if (newLineNum > referenceLineNum)
// TODO:       return BufCountForwardNLines(buf, referencePos,
// TODO:                                    newLineNum - referenceLineNum);
// TODO:    return BufStartOfLine(buf, referencePos);
// TODO: }
// TODO: 
// TODO: static int min3(int i1, int i2, int i3)
// TODO: {
// TODO:    if (i1 <= i2 && i1 <= i3)
// TODO:       return i1;
// TODO:    return i2 <= i3 ? i2 : i3;
// TODO: }
// TODO: 
// TODO: static int max3(int i1, int i2, int i3)
// TODO: {
// TODO:    if (i1 >= i2 && i1 >= i3)
// TODO:       return i1;
// TODO:    return i2 >= i3 ? i2 : i3;
// TODO: }
// TODO: 
// TODO: static int max(int i1, int i2)
// TODO: {
// TODO:    return i1 >= i2 ? i1 : i2;
// TODO: }
// TODO: 