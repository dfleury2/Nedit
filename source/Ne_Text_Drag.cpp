#include "Ne_Text_Drag.h"

#include "Ne_Text_Buffer.h"

#include "../util/utils.h"

#include <limits.h>


static void trackModifyRange(int* rangeStart, int* modRangeEnd, int* unmodRangeEnd, int modPos, int nInserted, int nDeleted);
static void findTextMargins(Ne_Text_Buffer* buf, int start, int end, int* leftMargin, int* rightMargin);
static int findRelativeLineStart(Ne_Text_Buffer* buf, int referencePos, int referenceLineNum, int newLineNum);
static int min3(int i1, int i2, int i3);
static int max3(int i1, int i2, int i3);
static int max(int i1, int i2);

/*
** Start the process of dragging the current primary-selected text across
** the window (move by dragging, as opposed to dragging to create the
** selection)
*/
void BeginBlockDrag(Ne_Text_Editor* textD)
{
   Ne_Text_Buffer* buf = textD->buffer;
   int fontHeight = textD->fontStruct.ascent() + textD->fontStruct.descent();
   int fontWidth = textD->fontStruct.max_width();
   selection* sel = &buf->primary;
   int nLines, mousePos, lineStart;
   int x, y, lineEnd;

   char* text;

   /* Save a copy of the whole text buffer as a backup, and for deriving changes */
   textD->text.dragOrigBuf = BufCreate();
   BufSetTabDistance(textD->text.dragOrigBuf, buf->tabDist);
   textD->text.dragOrigBuf->useTabs = buf->useTabs;
   text = BufGetAll(buf);
   BufSetAll(textD->text.dragOrigBuf, text);
   free__(text);
   if (sel->rectangular)
      BufRectSelect(textD->text.dragOrigBuf, sel->start, sel->end, sel->rectStart, sel->rectEnd);
   else
      BufSelect(textD->text.dragOrigBuf, sel->start, sel->end);

   /* Record the mouse pointer offsets from the top left corner of the
      selection (the position where text will actually be inserted In dragging
      non-rectangular selections)  */
   if (sel->rectangular)
   {
      textD->text.dragXOffset = textD->text.btnDownX + textD->horizOffset - textD->left - sel->rectStart * fontWidth;
   }
   else
   {
      if (!TextDPositionToXY(textD, sel->start, &x, &y))
         x = BufCountDispChars(buf, TextDStartOfLine(textD, sel->start), sel->start) * fontWidth + textD->left - textD->horizOffset;
      textD->text.dragXOffset = textD->text.btnDownX - x;
   }
   mousePos = TextDXYToPosition(textD, textD->text.btnDownX, textD->text.btnDownY);
   nLines = BufCountLines(buf, sel->start, mousePos);
   textD->text.dragYOffset = nLines * fontHeight + (((textD->text.btnDownY - textD->marginHeight) % fontHeight) - fontHeight/2);
   textD->text.dragNLines = BufCountLines(buf, sel->start, sel->end);

   /* Record the current drag insert position and the information for
      undoing the fictional insert of the selection in its new position */
   textD->text.dragInsertPos = sel->start;
   textD->text.dragInserted = sel->end - sel->start;
   if (sel->rectangular)
   {
      Ne_Text_Buffer* testBuf = BufCreate();
      char* testText = BufGetRange(buf, sel->start, sel->end);
      BufSetTabDistance(testBuf, buf->tabDist);
      testBuf->useTabs = buf->useTabs;
      BufSetAll(testBuf, testText);
      free__(testText);
      BufRemoveRect(testBuf, 0, sel->end - sel->start, sel->rectStart, sel->rectEnd);
      textD->text.dragDeleted = testBuf->length;
      BufFree(testBuf);
      textD->text.dragRectStart = sel->rectStart;
   }
   else
   {
      textD->text.dragDeleted = 0;
      textD->text.dragRectStart = 0;
   }
   textD->text.dragType = DRAG_MOVE;
   textD->text.dragSourceDeletePos = sel->start;
   textD->text.dragSourceInserted = textD->text.dragDeleted;
   textD->text.dragSourceDeleted = textD->text.dragInserted;

   /* For non-rectangular selections, fill in the rectangular information in
      the selection for overlay mode drags which are done rectangularly */
   if (!sel->rectangular)
   {
      lineStart = BufStartOfLine(buf, sel->start);
      if (textD->text.dragNLines == 0)
      {
         textD->text.dragOrigBuf->primary.rectStart = BufCountDispChars(buf, lineStart, sel->start);
         textD->text.dragOrigBuf->primary.rectEnd = BufCountDispChars(buf, lineStart, sel->end);
      }
      else
      {
         lineEnd = BufGetCharacter(buf, sel->end - 1) == '\n' ? sel->end - 1 : sel->end;
         findTextMargins(buf, lineStart, lineEnd,
                         &textD->text.dragOrigBuf->primary.rectStart,
                         &textD->text.dragOrigBuf->primary.rectEnd);
      }
   }

   /* Set the drag state to announce an ongoing block-drag */
   textD->text.dragState = PRIMARY_BLOCK_DRAG;

   /* Call the callback announcing the start of a block drag */
// TODO:    XtCallCallbacks((Widget)tw, textNdragStartCallback, (XtPointer)NULL);
}

/*
** Reposition the primary-selected text that is being dragged as a block
** for a new mouse position of (x, y)
*/
void BlockDragSelection(Ne_Text_Editor* textD, int x, int y, int dragType)
{
   Ne_Text_Buffer* buf = textD->buffer;
   int fontHeight = textD->fontStruct.ascent() + textD->fontStruct.descent();
   int fontWidth = textD->fontStruct.max_width();
   Ne_Text_Buffer* origBuf = textD->text.dragOrigBuf;
   int dragXOffset = textD->text.dragXOffset;
   Ne_Text_Buffer* tempBuf;
   selection* origSel = &origBuf->primary;
   int rectangular = origSel->rectangular;
   int overlay, oldDragType = textD->text.dragType;
   int nLines = textD->text.dragNLines;
   int insLineNum, insLineStart, insRectStart, insRectEnd, insStart;
   char* repText, *text, *insText;
   int modRangeStart = -1, tempModRangeEnd = -1, bufModRangeEnd = -1;
   int referenceLine, referencePos, tempStart, tempEnd, origSelLen;
   int insertInserted, insertDeleted, row, column;
   int origSelLineStart, origSelLineEnd;
   int sourceInserted, sourceDeleted, sourceDeletePos;

   if (textD->text.dragState != PRIMARY_BLOCK_DRAG)
      return;

   /* The operation of block dragging is simple in theory, but not so simple
      in practice.  There is a backup buffer (textD->text.dragOrigBuf) which
      holds a copy of the buffer as it existed before the drag.  When the
      user drags the mouse to a new location, this routine is called, and
      a temporary buffer is created and loaded with the local part of the
      buffer (from the backup) which might be changed by the drag.  The
      changes are all made to this temporary buffer, and the parts of this
      buffer which then differ from the real (displayed) buffer are used to
      replace those parts, thus one replace operation serves as both undo
      and modify.  This double-buffering of the operation prevents excessive
      redrawing (though there is still plenty of needless redrawing due to
      re-selection and rectangular operations).

      The hard part is keeping track of the changes such that a single replace
      operation will do everyting.  This is done using a routine called
      trackModifyRange which tracks expanding ranges of changes in the two
      buffers in modRangeStart, tempModRangeEnd, and bufModRangeEnd. */

   /* Create a temporary buffer for accumulating changes which will
      eventually be replaced in the real buffer.  Load the buffer with the
      range of characters which might be modified in this drag step
      (this could be tighter, but hopefully it's not too slow) */
   tempBuf = BufCreate();
   tempBuf->tabDist = buf->tabDist;
   tempBuf->useTabs = buf->useTabs;
   tempStart = min3(textD->text.dragInsertPos, origSel->start,
                    BufCountBackwardNLines(buf, textD->firstChar, nLines+2));
   tempEnd = BufCountForwardNLines(buf, max3(textD->text.dragInsertPos,
                                   origSel->start, textD->lastChar), nLines+2) +
             origSel->end - origSel->start;
   text = BufGetRange(origBuf, tempStart, tempEnd);
   BufSetAll(tempBuf, text);
   free__(text);

   /* If the drag type is USE_LAST, use the last dragType applied */
   if (dragType == USE_LAST)
      dragType = textD->text.dragType;
   overlay = dragType == DRAG_OVERLAY_MOVE || dragType == DRAG_OVERLAY_COPY;

   /* Overlay mode uses rectangular selections whether or not the original
      was rectangular.  To use a plain selection as if it were rectangular,
      the start and end positions need to be moved to the line boundaries
      and trailing newlines must be excluded */
   origSelLineStart = BufStartOfLine(origBuf, origSel->start);
   if (!rectangular && BufGetCharacter(origBuf, origSel->end - 1) == '\n')
      origSelLineEnd = origSel->end - 1;
   else
      origSelLineEnd = BufEndOfLine(origBuf, origSel->end);
   if (!rectangular && overlay && nLines != 0)
      dragXOffset -= fontWidth * (origSel->rectStart -
                                  (origSel->start - origSelLineStart));

   /* If the drag operation is of a different type than the last one, and the
      operation is a move, expand the modified-range to include undoing the
      text-removal at the site from which the text was dragged. */
   if (dragType != oldDragType && textD->text.dragSourceDeleted != 0)
      trackModifyRange(&modRangeStart, &bufModRangeEnd, &tempModRangeEnd,
                       textD->text.dragSourceDeletePos, textD->text.dragSourceInserted,
                       textD->text.dragSourceDeleted);

   /* Do, or re-do the original text removal at the site where a move began.
      If this part has not changed from the last call, do it silently to
      bring the temporary buffer in sync with the real (displayed)
      buffer.  If it's being re-done, track the changes to complete the
      redo operation begun above */
   if (dragType == DRAG_MOVE || dragType == DRAG_OVERLAY_MOVE)
   {
      if (rectangular || overlay)
      {
         int prevLen = tempBuf->length;
         origSelLen = origSelLineEnd - origSelLineStart;
         if (overlay)
            BufClearRect(tempBuf, origSelLineStart-tempStart,
                         origSelLineEnd-tempStart, origSel->rectStart,
                         origSel->rectEnd);
         else
            BufRemoveRect(tempBuf, origSelLineStart-tempStart,
                          origSelLineEnd-tempStart, origSel->rectStart,
                          origSel->rectEnd);
         sourceDeletePos = origSelLineStart;
         sourceInserted = origSelLen - prevLen + tempBuf->length;
         sourceDeleted = origSelLen;
      }
      else
      {
         BufRemove(tempBuf, origSel->start - tempStart,
                   origSel->end - tempStart);
         sourceDeletePos = origSel->start;
         sourceInserted = 0;
         sourceDeleted = origSel->end - origSel->start;
      }
      if (dragType != oldDragType)
         trackModifyRange(&modRangeStart, &tempModRangeEnd, &bufModRangeEnd,
                          sourceDeletePos, sourceInserted, sourceDeleted);
   }
   else
   {
      sourceDeletePos = 0;
      sourceInserted = 0;
      sourceDeleted = 0;
   }

   /* Expand the modified-range to include undoing the insert from the last
      call. */
   trackModifyRange(&modRangeStart, &bufModRangeEnd, &tempModRangeEnd,
                    textD->text.dragInsertPos, textD->text.dragInserted, textD->text.dragDeleted);

   /* Find the line number and column of the insert position.  Note that in
      continuous wrap mode, these must be calculated as if the text were
      not wrapped */
   TextDXYToUnconstrainedPosition(textD, max(0, x - dragXOffset),
                                  max(0, y - (textD->text.dragYOffset % fontHeight)), &row, &column);
   column = TextDOffsetWrappedColumn(textD, row, column);
   row = TextDOffsetWrappedRow(textD, row);
   insLineNum = row + textD->topLineNum - textD->text.dragYOffset / fontHeight;

   /* find a common point of reference between the two buffers, from which
      the insert position line number can be translated to a position */
   if (textD->firstChar > modRangeStart)
   {
      referenceLine = textD->topLineNum -
                      BufCountLines(buf, modRangeStart, textD->firstChar);
      referencePos = modRangeStart;
   }
   else
   {
      referencePos = textD->firstChar;
      referenceLine = textD->topLineNum;
   }

   /* find the position associated with the start of the new line in the
      temporary buffer */
   insLineStart = findRelativeLineStart(tempBuf, referencePos - tempStart,
                                        referenceLine, insLineNum) + tempStart;
   if (insLineStart - tempStart == tempBuf->length)
      insLineStart = BufStartOfLine(tempBuf, insLineStart - tempStart) +
                     tempStart;

   /* Find the actual insert position */
   if (rectangular || overlay)
   {
      insStart = insLineStart;
      insRectStart = column;
   }
   else     /* note, this will fail with proportional fonts */
   {
      insStart = BufCountForwardDispChars(tempBuf, insLineStart - tempStart,
                                          column) + tempStart;
      insRectStart = 0;
   }

   /* If the position is the same as last time, don't bother drawing (it
      would be nice if this decision could be made earlier) */
   if (insStart == textD->text.dragInsertPos &&
         insRectStart == textD->text.dragRectStart && dragType == oldDragType)
   {
      BufFree(tempBuf);
      return;
   }

   /* Do the insert in the temporary buffer */
   if (rectangular || overlay)
   {
      insText = BufGetTextInRect(origBuf, origSelLineStart, origSelLineEnd,
                                 origSel->rectStart, origSel->rectEnd);
      if (overlay)
         BufOverlayRect(tempBuf, insStart - tempStart, insRectStart,
                        insRectStart + origSel->rectEnd - origSel->rectStart,
                        insText, &insertInserted, &insertDeleted);
      else
         BufInsertCol(tempBuf, insRectStart, insStart - tempStart, insText,
                      &insertInserted, &insertDeleted);
      trackModifyRange(&modRangeStart, &tempModRangeEnd, &bufModRangeEnd,
                       insStart, insertInserted, insertDeleted);
      free__(insText);
   }
   else
   {
      insText = BufGetSelectionText(origBuf);
      BufInsert(tempBuf, insStart - tempStart, insText);
      trackModifyRange(&modRangeStart, &tempModRangeEnd, &bufModRangeEnd,
                       insStart, origSel->end - origSel->start, 0);
      insertInserted = origSel->end - origSel->start;
      insertDeleted = 0;
      free__(insText);
   }

   /* Make the changes in the real buffer */
   repText = BufGetRange(tempBuf, modRangeStart - tempStart,
                         tempModRangeEnd - tempStart);
   BufFree(tempBuf);
   TextDBlankCursor(textD);
   BufReplace(buf, modRangeStart, bufModRangeEnd, repText);
   free__(repText);

   /* Store the necessary information for undoing this step */
   textD->text.dragInsertPos = insStart;
   textD->text.dragRectStart = insRectStart;
   textD->text.dragInserted = insertInserted;
   textD->text.dragDeleted = insertDeleted;
   textD->text.dragSourceDeletePos = sourceDeletePos;
   textD->text.dragSourceInserted = sourceInserted;
   textD->text.dragSourceDeleted = sourceDeleted;
   textD->text.dragType = dragType;

   /* Reset the selection and cursor position */
   if (rectangular || overlay)
   {
      insRectEnd = insRectStart + origSel->rectEnd - origSel->rectStart;
      BufRectSelect(buf, insStart, insStart + insertInserted, insRectStart,
                    insRectEnd);
      TextDSetInsertPosition(textD, BufCountForwardDispChars(buf,
                             BufCountForwardNLines(buf, insStart, textD->text.dragNLines),
                             insRectEnd));
   }
   else
   {
      BufSelect(buf, insStart, insStart + origSel->end - origSel->start);
      TextDSetInsertPosition(textD, insStart + origSel->end - origSel->start);
   }
   TextDUnblankCursor(textD);
// TODO:    XtCallCallbacks((Widget)tw, textNcursorMovementCallback, (XtPointer)NULL);
   textD->text.emTabsBeforeCursor = 0;
}

/*
** Complete a block text drag operation
*/
void FinishBlockDrag(Ne_Text_Editor* textD)
{
   dragEndCBStruct endStruct;
   int modRangeStart = -1, origModRangeEnd, bufModRangeEnd;
   char* deletedText;

   /* Find the changed region of the buffer, covering both the deletion
      of the selected text at the drag start position, and insertion at
      the drag destination */
   trackModifyRange(&modRangeStart, &bufModRangeEnd, &origModRangeEnd,
                    textD->text.dragSourceDeletePos, textD->text.dragSourceInserted,
                    textD->text.dragSourceDeleted);
   trackModifyRange(&modRangeStart, &bufModRangeEnd, &origModRangeEnd,
                    textD->text.dragInsertPos, textD->text.dragInserted,
                    textD->text.dragDeleted);

   /* Get the original (pre-modified) range of text from saved backup buffer */
   deletedText = BufGetRange(textD->text.dragOrigBuf, modRangeStart,
                             origModRangeEnd);

   /* Free the backup buffer */
   BufFree(textD->text.dragOrigBuf);

   /* Return to normal drag state */
   textD->text.dragState = NOT_CLICKED;

   /* Call finish-drag calback */
   endStruct.startPos = modRangeStart;
   endStruct.nCharsDeleted = origModRangeEnd - modRangeStart;
   endStruct.nCharsInserted = bufModRangeEnd - modRangeStart;
   endStruct.deletedText = deletedText;
// TODO:    XtCallCallbacks((Widget)tw, textNdragEndCallback, (XtPointer)&endStruct);
   free__(deletedText);
}

/*
** Cancel a block drag operation
*/
void CancelBlockDrag(Ne_Text_Editor* textD)
{
   Ne_Text_Buffer* buf = textD->buffer;
   Ne_Text_Buffer* origBuf = textD->text.dragOrigBuf;
   selection* origSel = &origBuf->primary;
   int modRangeStart = -1, origModRangeEnd, bufModRangeEnd;
   char* repText;
   dragEndCBStruct endStruct;

   /* If the operation was a move, make the modify range reflect the
      removal of the text from the starting position */
   if (textD->text.dragSourceDeleted != 0)
      trackModifyRange(&modRangeStart, &bufModRangeEnd, &origModRangeEnd,
                       textD->text.dragSourceDeletePos, textD->text.dragSourceInserted,
                       textD->text.dragSourceDeleted);

   /* Include the insert being undone from the last step in the modified
      range. */
   trackModifyRange(&modRangeStart, &bufModRangeEnd, &origModRangeEnd,
                    textD->text.dragInsertPos, textD->text.dragInserted, textD->text.dragDeleted);

   /* Make the changes in the buffer */
   repText = BufGetRange(origBuf, modRangeStart, origModRangeEnd);
   BufReplace(buf, modRangeStart, bufModRangeEnd, repText);
   free__(repText);

   /* Reset the selection and cursor position */
   if (origSel->rectangular)
      BufRectSelect(buf, origSel->start, origSel->end, origSel->rectStart,
                    origSel->rectEnd);
   else
      BufSelect(buf, origSel->start, origSel->end);
   TextDSetInsertPosition(textD, buf->cursorPosHint);
// TODO:    XtCallCallbacks((Widget)tw, textNcursorMovementCallback, NULL);
   textD->text.emTabsBeforeCursor = 0;

   /* Free the backup buffer */
   BufFree(origBuf);

   /* Indicate end of drag */
   textD->text.dragState = DRAG_CANCELED;

   /* Call finish-drag calback */
   endStruct.startPos = 0;
   endStruct.nCharsDeleted = 0;
   endStruct.nCharsInserted = 0;
   endStruct.deletedText = NULL;
// TODO:    XtCallCallbacks((Widget)tw, textNdragEndCallback, (XtPointer)&endStruct);
}

/*
** Maintain boundaries of changed region between two buffers which
** start out with identical contents, but diverge through insertion,
** deletion, and replacement, such that the buffers can be reconciled
** by replacing the changed region of either buffer with the changed
** region of the other.
**
** rangeStart is the beginning of the modification region in the shared
** coordinates of both buffers (which are identical up to rangeStart).
** modRangeEnd is the end of the changed region for the buffer being
** modified, unmodRangeEnd is the end of the region for the buffer NOT
** being modified.  A value of -1 in rangeStart indicates that there
** have been no modifications so far.
*/
static void trackModifyRange(int* rangeStart, int* modRangeEnd,
                             int* unmodRangeEnd, int modPos, int nInserted, int nDeleted)
{
   if (*rangeStart == -1)
   {
      *rangeStart = modPos;
      *modRangeEnd = modPos + nInserted;
      *unmodRangeEnd = modPos + nDeleted;
   }
   else
   {
      if (modPos < *rangeStart)
         *rangeStart = modPos;
      if (modPos + nDeleted > *modRangeEnd)
      {
         *unmodRangeEnd += modPos + nDeleted - *modRangeEnd;
         *modRangeEnd = modPos + nInserted;
      }
      else
         *modRangeEnd += nInserted - nDeleted;
   }
}

/*
** Find the left and right margins of text between "start" and "end" in
** buffer "buf".  Note that "start is assumed to be at the start of a line.
*/
static void findTextMargins(Ne_Text_Buffer* buf, int start, int end, int* leftMargin,
                            int* rightMargin)
{
   char c;
   int pos, width = 0, maxWidth = 0, minWhite = INT_MAX, inWhite = true;

   for (pos=start; pos<end; pos++)
   {
      c = BufGetCharacter(buf, pos);
      if (inWhite && c != ' ' && c != '\t')
      {
         inWhite = false;
         if (width < minWhite)
            minWhite = width;
      }
      if (c == '\n')
      {
         if (width > maxWidth)
            maxWidth = width;
         width = 0;
         inWhite = true;
      }
      else
         width += BufCharWidth(c, width, buf->tabDist, buf->nullSubsChar);
   }
   if (width > maxWidth)
      maxWidth = width;
   *leftMargin = minWhite == INT_MAX ? 0 : minWhite;
   *rightMargin = maxWidth;
}

/*
** Find a text position in buffer "buf" by counting forward or backward
** from a reference position with known line number
*/
static int findRelativeLineStart(Ne_Text_Buffer* buf, int referencePos,
                                 int referenceLineNum, int newLineNum)
{
   if (newLineNum < referenceLineNum)
      return BufCountBackwardNLines(buf, referencePos,
                                    referenceLineNum - newLineNum);
   else if (newLineNum > referenceLineNum)
      return BufCountForwardNLines(buf, referencePos,
                                   newLineNum - referenceLineNum);
   return BufStartOfLine(buf, referencePos);
}

static int min3(int i1, int i2, int i3)
{
   if (i1 <= i2 && i1 <= i3)
      return i1;
   return i2 <= i3 ? i2 : i3;
}

static int max3(int i1, int i2, int i3)
{
   if (i1 >= i2 && i1 >= i3)
      return i1;
   return i2 >= i3 ? i2 : i3;
}

static int max(int i1, int i2)
{
   return i1 >= i2 ? i1 : i2;
}
