#include "Ne_Text_Drag.h"

#include "Ne_Text_Editor.h"
#include "Ne_Text_Buffer.h"

#include <limits.h>

static void trackModifyRange(int* rangeStart, int* modRangeEnd, int* unmodRangeEnd, int modPos, int nInserted, int nDeleted);
// TODO: static void findTextMargins(textBuffer* buf, int start, int end, int* leftMargin, int* rightMargin);
static int findRelativeLineStart(Ne_Text_Buffer* buf, int referencePos, int referenceLineNum, int newLineNum);
static int min3(int i1, int i2, int i3);
static int max3(int i1, int i2, int i3);
static int max(int i1, int i2);

/*
** Start the process of dragging the current primary-selected text across
** the window (move by dragging, as opposed to dragging to create the
** selection)
*/
void BeginBlockDrag(Fl_Widget* w)
{
   Ne_Text_Display* textD = (Ne_Text_Display*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   int fontHeight = textD->fontStruct.height();
   int fontWidth = textD->fontStruct.max_width();
   Ne_Text_Selection* sel = &buf->getSelection();
   int nLines, mousePos, lineStart;
   int x, y, lineEnd;

   char* text;

   // Save a copy of the whole text buffer as a backup, and for deriving changes
   textD->dragOrigBuf = new Ne_Text_Buffer();
   textD->dragOrigBuf->setTabDistance(buf->getTabDistance());
   textD->dragOrigBuf->setAll(buf->getAll().c_str());
   textD->dragOrigBuf->select(sel->start, sel->end);

   // Record the mouse pointer offsets from the top left corner of the
   // selection (the position where text will actually be inserted In dragging non-rectangular selections)
   if (!textD->positionToXY(sel->start, &x, &y))
      x = buf->countDispChars(textD->startOfLine(sel->start), sel->start) * fontWidth + textD->left - textD->horizOffset;
   textD->dragXOffset = textD->btnDownX - x;

   mousePos = textD->XYToPosition(textD->btnDownX, textD->btnDownY);
   nLines = buf->countLines(sel->start, mousePos);
   textD->dragYOffset = nLines * fontHeight + (((textD->btnDownY - textD->marginHeight) % fontHeight) - fontHeight/2);
   textD->dragNLines = buf->countLines(sel->start, sel->end);

   // Record the current drag insert position and the information for
   // undoing the fictional insert of the selection in its new position
   textD->dragInsertPos = sel->start;
   textD->dragInserted = sel->end - sel->start;
   textD->dragDeleted = 0;
   textD->dragRectStart = 0;
   textD->dragType = NE_DRAG_MOVE;
   textD->dragSourceDeletePos = sel->start;
   textD->dragSourceInserted = textD->dragDeleted;
   textD->dragSourceDeleted = textD->dragInserted;

   // For non-rectangular selections, fill in the rectangular information in
   // the selection for overlay mode drags which are done rectangularly
   lineStart = buf->startOfLine(sel->start);
// TODO:    if (textD->dragNLines == 0)
// TODO:    {
// TODO:       textD->dragOrigBuf->primary.rectStart = buf->countDispChars(lineStart, sel->start);
// TODO:       textD->dragOrigBuf->primary.rectEnd = buf->countDispChars(lineStart, sel->end);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       lineEnd = buf->getCharacter(sel->end - 1) == '\n' ? sel->end - 1 : sel->end;
// TODO:       findTextMargins(buf, lineStart, lineEnd, &textD->dragOrigBuf->primary.rectStart, &textD->dragOrigBuf->primary.rectEnd);
// TODO:    }

   // Set the drag state to announce an ongoing block-drag
   textD->dragState = NE_PRIMARY_BLOCK_DRAG;

   // Call the callback announcing the start of a block drag
// TODO:    XtCallCallbacks((Widget)tw, textNdragStartCallback, (XtPointer)NULL);
}

/*
** Reposition the primary-selected text that is being dragged as a block
** for a new mouse position of (x, y)
*/
void BlockDragSelection(Fl_Widget* w, int x, int y, int dragType)
{
   Ne_Text_Display* textD = (Ne_Text_Display*)w;
   Ne_Text_Buffer* buf = textD->buffer;
   int fontHeight = textD->fontStruct.height();
   int fontWidth = textD->fontStruct.max_width();
   Ne_Text_Buffer* origBuf = textD->dragOrigBuf;
   int dragXOffset = textD->dragXOffset;
   Ne_Text_Buffer* tempBuf;
   Ne_Text_Selection* origSel = &origBuf->getSelection();
   int overlay, oldDragType = textD->dragType;
   int nLines = textD->dragNLines;
   int insLineNum, insLineStart, insRectStart, insRectEnd, insStart;
   char *text;
   int modRangeStart = -1, tempModRangeEnd = -1, bufModRangeEnd = -1;
   int referenceLine, referencePos, tempStart, tempEnd, origSelLen;
   int insertInserted, insertDeleted, row, column;
   int origSelLineStart, origSelLineEnd;
   int sourceInserted, sourceDeleted, sourceDeletePos;

   if (textD->dragState != NE_PRIMARY_BLOCK_DRAG)
      return;

   /* The operation of block dragging is simple in theory, but not so simple
      in practice.  There is a backup buffer (tw->text.dragOrigBuf) which
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
   tempBuf = new Ne_Text_Buffer();
   tempBuf->setTabDistance(buf->getTabDistance());
   tempStart = min3(textD->dragInsertPos, origSel->start, buf->countBackwardNLines(textD->firstChar, nLines+2));
   tempEnd = buf->countForwardNLines(max3(textD->dragInsertPos, origSel->start, textD->lastChar), nLines+2) + origSel->end - origSel->start;
   tempBuf->setAll(origBuf->getRange(tempStart, tempEnd).c_str());

   /* If the drag type is USE_LAST, use the last dragType applied */
   if (dragType == NE_USE_LAST)
      dragType = textD->dragType;
   overlay = (dragType == NE_DRAG_OVERLAY_MOVE || dragType == NE_DRAG_OVERLAY_COPY);

   /* Overlay mode uses rectangular selections whether or not the original
      was rectangular.  To use a plain selection as if it were rectangular,
      the start and end positions need to be moved to the line boundaries
      and trailing newlines must be excluded */
   origSelLineStart = origBuf->startOfLine(origSel->start);
   if (origBuf->getCharacter(origSel->end - 1) == '\n')
      origSelLineEnd = origSel->end - 1;
   else
      origSelLineEnd = origBuf->endOfLine(origSel->end);
// TODO:    if (overlay && nLines != 0)
// TODO:       dragXOffset -= fontWidth * (origSel->rectStart - (origSel->start - origSelLineStart));

   /* If the drag operation is of a different type than the last one, and the
      operation is a move, expand the modified-range to include undoing the
      text-removal at the site from which the text was dragged. */
   if (dragType != oldDragType && textD->dragSourceDeleted != 0)
      trackModifyRange(&modRangeStart, &bufModRangeEnd, &tempModRangeEnd,
                       textD->dragSourceDeletePos, textD->dragSourceInserted,
                       textD->dragSourceDeleted);

   /* Do, or re-do the original text removal at the site where a move began.
      If this part has not changed from the last call, do it silently to
      bring the temporary buffer in sync with the real (displayed)
      buffer.  If it's being re-done, track the changes to complete the
      redo operation begun above */
   if (dragType == NE_DRAG_MOVE || dragType == NE_DRAG_OVERLAY_MOVE)
   {
// TODO:       if (overlay)
// TODO:       {
// TODO:          int prevLen = tempBuf->length();
// TODO:          origSelLen = origSelLineEnd - origSelLineStart;
// TODO:          if (overlay)
// TODO:             BufClearRect(tempBuf, origSelLineStart-tempStart, origSelLineEnd-tempStart, origSel->rectStart, origSel->rectEnd);
// TODO:          else
// TODO:             BufRemoveRect(tempBuf, origSelLineStart-tempStart, origSelLineEnd-tempStart, origSel->rectStart, origSel->rectEnd);
// TODO:          sourceDeletePos = origSelLineStart;
// TODO:          sourceInserted = origSelLen - prevLen + tempBuf->length();
// TODO:          sourceDeleted = origSelLen;
// TODO:       }
// TODO:       else
      {
         tempBuf->remove(origSel->start - tempStart, origSel->end - tempStart);
         sourceDeletePos = origSel->start;
         sourceInserted = 0;
         sourceDeleted = origSel->end - origSel->start;
      }
      if (dragType != oldDragType)
         trackModifyRange(&modRangeStart, &tempModRangeEnd, &bufModRangeEnd, sourceDeletePos, sourceInserted, sourceDeleted);
   }
   else
   {
      sourceDeletePos = 0;
      sourceInserted = 0;
      sourceDeleted = 0;
   }

   /* Expand the modified-range to include undoing the insert from the last call. */
   trackModifyRange(&modRangeStart, &bufModRangeEnd, &tempModRangeEnd, textD->dragInsertPos, textD->dragInserted, textD->dragDeleted);

   /* Find the line number and column of the insert position.  Note that in
      continuous wrap mode, these must be calculated as if the text were
      not wrapped */
   textD->XYToUnconstrainedPosition(max(0, x - dragXOffset), max(0, y - (textD->dragYOffset % fontHeight)), &row, &column);
   column = textD->OffsetWrappedColumn(row, column);
   row = textD->OffsetWrappedRow(row);
   insLineNum = row + textD->topLineNum - textD->dragYOffset / fontHeight;

   /* find a common point of reference between the two buffers, from which
      the insert position line number can be translated to a position */
   if (textD->firstChar > modRangeStart)
   {
      referenceLine = textD->topLineNum - buf->countLines(modRangeStart, textD->firstChar);
      referencePos = modRangeStart;
   }
   else
   {
      referencePos = textD->firstChar;
      referenceLine = textD->topLineNum;
   }

   /* find the position associated with the start of the new line in the
      temporary buffer */
   insLineStart = findRelativeLineStart(tempBuf, referencePos - tempStart, referenceLine, insLineNum) + tempStart;
   if (insLineStart - tempStart == tempBuf->length())
      insLineStart = tempBuf->startOfLine(insLineStart - tempStart) + tempStart;

   /* Find the actual insert position */
   if (overlay)
   {
      insStart = insLineStart;
      insRectStart = column;
   }
   else     /* note, this will fail with proportional fonts */
   {
      insStart = tempBuf->countForwardDispChars(insLineStart - tempStart, column) + tempStart;
      insRectStart = 0;
   }

   /* If the position is the same as last time, don't bother drawing (it
      would be nice if this decision could be made earlier) */
   if (insStart == textD->dragInsertPos && insRectStart == textD->dragRectStart && dragType == oldDragType)
   {
      delete tempBuf;
      return;
   }

   // Do the insert in the temporary buffer
// TODO:    if (overlay)
// TODO:    {
// TODO:       insText = BufGetTextInRect(origBuf, origSelLineStart, origSelLineEnd, origSel->rectStart, origSel->rectEnd);
// TODO:       if (overlay)
// TODO:          BufOverlayRect(tempBuf, insStart - tempStart, insRectStart, insRectStart + origSel->rectEnd - origSel->rectStart, insText, &insertInserted, &insertDeleted);
// TODO:       else
// TODO:          BufInsertCol(tempBuf, insRectStart, insStart - tempStart, insText, &insertInserted, &insertDeleted);
// TODO:       trackModifyRange(&modRangeStart, &tempModRangeEnd, &bufModRangeEnd, insStart, insertInserted, insertDeleted);
// TODO:       XtFree(insText);
// TODO:    }
// TODO:    else
   {
      std::string insText = origBuf->getSelectionText();
      tempBuf->insertAt(insStart - tempStart, insText.c_str());
      trackModifyRange(&modRangeStart, &tempModRangeEnd, &bufModRangeEnd, insStart, origSel->end - origSel->start, 0);
      insertInserted = origSel->end - origSel->start;
      insertDeleted = 0;
   }

   // Make the changes in the real buffer
   std::string repText = tempBuf->getRange(modRangeStart - tempStart, tempModRangeEnd - tempStart);
   delete tempBuf;
   textD->blankCursor();
   buf->replace(modRangeStart, bufModRangeEnd, repText.c_str());

   /* Store the necessary information for undoing this step */
   textD->dragInsertPos = insStart;
   textD->dragRectStart = insRectStart;
   textD->dragInserted = insertInserted;
   textD->dragDeleted = insertDeleted;
   textD->dragSourceDeletePos = sourceDeletePos;
   textD->dragSourceInserted = sourceInserted;
   textD->dragSourceDeleted = sourceDeleted;
   textD->dragType = dragType;

   /* Reset the selection and cursor position */
// TODO:    if (overlay)
// TODO:    {
// TODO:       insRectEnd = insRectStart + origSel->rectEnd - origSel->rectStart;
// TODO:       BufRectSelect(buf, insStart, insStart + insertInserted, insRectStart, insRectEnd);
// TODO:       TextDSetInsertPosition(textD, BufCountForwardDispChars(buf, BufCountForwardNLines(buf, insStart, tw->text.dragNLines), insRectEnd));
// TODO:    }
// TODO:    else
   {
      buf->select(insStart, insStart + origSel->end - origSel->start);
      textD->setInsertPosition(insStart + origSel->end - origSel->start);
   }
   textD->unblankCursor();
// TODO:    XtCallCallbacks((Widget)tw, textNcursorMovementCallback, (XtPointer)NULL);
   textD->emTabsBeforeCursor = 0;
}

/*
** Complete a block text drag operation
*/
void FinishBlockDrag(Fl_Widget* w)
{
   Ne_Text_Display* textD = (Ne_Text_Display*)w;
   NeDragEndCBStruct endStruct;
   int modRangeStart = -1, origModRangeEnd, bufModRangeEnd;
   
   /* Find the changed region of the buffer, covering both the deletion
      of the selected text at the drag start position, and insertion at
      the drag destination */
   trackModifyRange(&modRangeStart, &bufModRangeEnd, &origModRangeEnd, textD->dragSourceDeletePos, textD->dragSourceInserted, textD->dragSourceDeleted);
   trackModifyRange(&modRangeStart, &bufModRangeEnd, &origModRangeEnd, textD->dragInsertPos, textD->dragInserted, textD->dragDeleted);

   /* Get the original (pre-modified) range of text from saved backup buffer */
   std::string deletedText = textD->dragOrigBuf->getRange(modRangeStart, origModRangeEnd);

   /* Free the backup buffer */
   delete textD->dragOrigBuf;

   /* Return to normal drag state */
   textD->dragState = NE_NOT_CLICKED;

   /* Call finish-drag calback */
   endStruct.startPos = modRangeStart;
   endStruct.nCharsDeleted = origModRangeEnd - modRangeStart;
   endStruct.nCharsInserted = bufModRangeEnd - modRangeStart;
   endStruct.deletedText = deletedText;
// TODO:    XtCallCallbacks((Widget)tw, textNdragEndCallback, (XtPointer)&endStruct);
}

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
static void trackModifyRange(int* rangeStart, int* modRangeEnd, int* unmodRangeEnd, int modPos, int nInserted, int nDeleted)
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

/*
** Find a text position in buffer "buf" by counting forward or backward
** from a reference position with known line number
*/
static int findRelativeLineStart(Ne_Text_Buffer* buf, int referencePos, int referenceLineNum, int newLineNum)
{
   if (newLineNum < referenceLineNum)
      return buf->countBackwardNLines(referencePos, referenceLineNum - newLineNum);
   else if (newLineNum > referenceLineNum)
      return buf->countForwardNLines(referencePos, newLineNum - referenceLineNum);
   return buf->startOfLine(referencePos);
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
