#include "Ne_Text_Display.h"

#include "Ne_Text_Buffer.h"
#include "Ne_Range_Set.h"

#include "../util/Ne_Color.h"

// TODO:  #include "calltips.h"
// TODO:  #include "highlight.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Box.H>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <algorithm>

// Some default colors from nedit.h
#define NEDIT_DEFAULT_FG        "black"
#define NEDIT_DEFAULT_TEXT_BG   "grey90"
#define NEDIT_DEFAULT_SEL_FG    "black"
#define NEDIT_DEFAULT_SEL_BG    "gray80"
#define NEDIT_DEFAULT_HI_FG     "white" // These are colors for flashing
#define NEDIT_DEFAULT_HI_BG     "red"   // matching parens
#define NEDIT_DEFAULT_LINENO_FG "black"
#define NEDIT_DEFAULT_CURSOR_FG "black"
#define NEDIT_DEFAULT_HELP_FG   "black"
#define NEDIT_DEFAULT_HELP_BG   "gray80"
#define NEDIT_DEFAULT_CALLTIP_FG "black"
#define NEDIT_DEFAULT_CALLTIP_BG "LemonChiffon1"

// Masks for text drawing methods.  These are or'd together to form an integer which describes what drawing calls to use to draw a string
#define FILL_SHIFT 8
#define SECONDARY_SHIFT 9
#define PRIMARY_SHIFT 10
#define HIGHLIGHT_SHIFT 11
#define STYLE_LOOKUP_SHIFT 0
#define BACKLIGHT_SHIFT 12

#define FILL_MASK (1 << FILL_SHIFT)
#define SECONDARY_MASK (1 << SECONDARY_SHIFT)
#define PRIMARY_MASK (1 << PRIMARY_SHIFT)
#define HIGHLIGHT_MASK (1 << HIGHLIGHT_SHIFT)
#define STYLE_LOOKUP_MASK (0xff << STYLE_LOOKUP_SHIFT)
#define BACKLIGHT_MASK  (0xff << BACKLIGHT_SHIFT)

#define RANGESET_SHIFT (20)
#define RANGESET_MASK (0x3F << RANGESET_SHIFT)

/* If you use both 32-Bit Style mask layout:
Bits +----------------+----------------+----------------+----------------+
 hex |1F1E1D1C1B1A1918|1716151413121110| F E D C B A 9 8| 7 6 5 4 3 2 1 0|
 dec |3130292827262524|2322212019181716|151413121110 9 8| 7 6 5 4 3 2 1 0|
     +----------------+----------------+----------------+----------------+
Type |             r r| r r r r b b b b| b b b b H 1 2 F| s s s s s s s s|
     +----------------+----------------+----------------+----------------+
where: s - style lookup value (8 bits)
F - fill (1 bit)
2 - secondary selection  (1 bit)
1 - primary selection (1 bit)
H - highlight (1 bit)
b - backlighting index (8 bits)
r - rangeset index (6 bits)
This leaves 6 "unused" bits */

/* Maximum displayable line length (how many characters will fit across the
widest window).  This amount of memory is temporarily allocated from the
stack in the redisplayLine routine for drawing strings */
#define MAX_DISP_LINE_LEN 1000

// -------------------------------------------------------------------------------
// Callback attached to the text buffer to receive delete information before
// the modifications are actually made.
// -------------------------------------------------------------------------------
static void NeBufPreDeleteCB(int pos, int nDeleted, void* cbArg)
{
   ((Ne_Text_Display*)cbArg)->bufPreDeleteCB(pos, nDeleted);
}

// -------------------------------------------------------------------------------
// Callback attached to the text buffer to receive modification information
// -------------------------------------------------------------------------------
static void NeBufModifiedCB(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg)
{
   ((Ne_Text_Display*)cbArg)->bufModifiedCB(pos, nInserted, nDeleted, nRestyled, deletedText);
}

// -------------------------------------------------------------------------------
static void HScrollCB(Fl_Widget* w, void* data)
{
   ((Ne_Text_Display*)data)->hScrollCB();
}

// -------------------------------------------------------------------------------
static void VScrollCB(Fl_Widget* w, void* data)
{
   ((Ne_Text_Display*)data)->vScrollCB();
}

// -------------------------------------------------------------------------------
Ne_Text_Display::Ne_Text_Display(int x, int y, int w, int h, const char* l)
   : Fl_Group(x, y, w, h, l)
{
   box(FL_FLAT_BOX);

   Fl_Box* boxResizable = new Fl_Box(x, y, 1, 1);
   resizable(boxResizable);

   int scrollBarSize = Fl::scrollbar_size();
   vScrollBar = new Fl_Scrollbar(x + w - scrollBarSize, y, scrollBarSize, h - scrollBarSize);
   vScrollBar->callback(VScrollCB, this);

   hScrollBar = new Fl_Scrollbar(x, y + h - scrollBarSize, w - scrollBarSize, scrollBarSize);
   hScrollBar->callback(HScrollCB, this);
   hScrollBar->type(FL_HORIZONTAL);

   end();

   canRedraw = false;

   cursorOn = true;
   cursorPos = 0;
   cursorX = -100;
   cursorY = -100;
   cursorToHint = NO_HINT;
   cursorStyle = NE_NORMAL_CURSOR;
   cursorPreferredCol = -1;
   firstChar = 0;
   lastChar = 0;
   nBufferLines = 0;
   topLineNum = 1;
   absTopLineNum = 1;
   needAbsTopLineNum = false;
   horizOffset = 0;

   fontStruct = Ne_Font();
   descent = fontStruct.descent();
   ascent = fontStruct.ascent();
   fixedFontWidth = fontStruct.fixedFontWidth();

   this->fgPixel = GetColor(NEDIT_DEFAULT_FG);
   this->bgPixel = GetColor(NEDIT_DEFAULT_TEXT_BG);
   this->selectBGPixel = GetColor(NEDIT_DEFAULT_SEL_FG);
   this->highlightFGPixel = GetColor(NEDIT_DEFAULT_HI_FG);
   this->highlightBGPixel = GetColor(NEDIT_DEFAULT_HI_BG);
   this->lineNumFGPixel = GetColor(NEDIT_DEFAULT_LINENO_FG);
   this->cursorFGPixel = GetColor(NEDIT_DEFAULT_CURSOR_FG);
   this->calltipFGPixel = GetColor(NEDIT_DEFAULT_CALLTIP_FG);
   this->calltipBGPixel = GetColor(NEDIT_DEFAULT_CALLTIP_BG);
   
   this->bgClassPixel = NULL;
   this->bgClass = NULL;

   this->suppressResync = 0;
   this->nLinesDeleted = 0;
   this->modifyingTabDist = 0;
   this->pointerHidden = false;

   this->buffer = NULL;
   this->styleBuffer = NULL;
   this->lineStarts = NULL;
   this->nVisibleLines = 0;
   this->nStyles = 0;

   this->lineNumLeft = this->lineNumWidth = 0; // No line numbers
   this->marginWidth = this->marginHeight = 5;
   this->rows = 24;
   this->columns = 80;
   this->lineNumCols = 0;
   this->cursorVPadding = 0;
   this->autoShowInsertPos = true;
   this->emTabsBeforeCursor = 0;
   this->anchor = 0;
   this->delimiters = " \n.,/\\`'!@#%^&*()-=+{}[]\":;<>?";
   this->isOverstrike = false;
   this->heavyCursor = false;
   this->autoIndent = true;
   this->smartIndent = false;
   this->readOnly = false;
   this->pendingDelete = true;
   this->emulateTabs = 0;
   this->autoWrap = false;
   this->autoWrapPastedText = false;
   this->btnDownX = this-> btnDownY = -1;
   this->mouseX = this->mouseY = -1;
   this->multiClickState = NE_NO_CLICKS;
   this->lastBtnDown = 0;
   this->dragState = NE_NOT_CLICKED;
   this->dragOrigBuf = NULL;
   this->dragXOffset = 0; 
   this->dragYOffset = 0;
   this->dragType = 0;
   this->dragInsertPos = 0;
   this->dragInserted = 0;
   this->dragDeleted = 0;
   this->dragSourceDeletePos = 0;
   this->dragSourceInserted = 0;
   this->dragSourceDeleted = 0;
   this->dragNLines = 0;

   this->continuousWrap = true;
   this->wrapMargin = 0;

   unfinishedStyle = '\0';
   unfinishedHighlightCB = NULL;
   highlightCBArg = NULL;

   computeTextAreaSize(x, y, w, h);
}

// -------------------------------------------------------------------------------
void Ne_Text_Display::draw()
{
   canRedraw = true;

   // Drawn the box
   color(bgPixel);
   draw_box();

   // Drawing Text Area Widget
   //fl_rect(x(), y(), w() - vScrollBar->w(), h() - hScrollBar->h(), FL_RED);

   // Drawing Text Area
   //fl_rect(left, top, width, height, FL_GREEN);

   // Drawing line number area
   //fl_rect(lineNumLeft, top, lineNumWidth, height, FL_YELLOW);

   // Drawing Text Area
   //fl_rect(lineNumLeft, top, this->width + this->left - lineNumLeft, this->height, FL_WHITE);
   redisplayRect(lineNumLeft, top, this->width + this->left - lineNumLeft, this->height);

   draw_child(*vScrollBar);
   draw_child(*hScrollBar);

   // Drawing lower right grey box
   fl_color(FL_GRAY);
   fl_rectf(x() + w() - vScrollBar->w(), y() + h() - hScrollBar->h(), vScrollBar->w(), hScrollBar->h());

   // Drawing separation between line and text
   fl_line(this->lineNumLeft + this->lineNumWidth - this->marginWidth / 2, y(),
           this->lineNumLeft + this->lineNumWidth - this->marginWidth / 2, y() + h() - hScrollBar->h() - 1);

   canRedraw = false;
}

// -------------------------------------------------------------------------------
void Ne_Text_Display::resize(int x, int y, int w, int h)
{
   int oldWidth = this->width;
   computeTextAreaSize(x, y, w, h);

   int charWidth = fontStruct.max_width();

   this->lineNumCols = lineNumWidth / charWidth;
   this->columns = (width - lineNumWidth) / charWidth;
   this->rows = (height) / (this->ascent + this->descent);

   // Resize the text display that the widget uses to render text
   resizeTextArea(width, height, oldWidth);

   Fl_Group::resize(x, y, w, h);
}

// -------------------------------------------------------------------------------
void Ne_Text_Display::computeTextAreaSize(int x, int y, int w, int h)
{
   this->left = x + marginWidth;
   this->top = y + marginHeight;
   this->width = w - vScrollBar->w() - 2 * marginWidth;
   this->height = h - hScrollBar->h() - 2 * marginHeight;
   if (lineNumWidth != 0)
   {
      lineNumLeft = x + marginWidth;
      this->left += lineNumLeft + lineNumWidth;
      this->width -= lineNumWidth + marginWidth;
   }
}

// -------------------------------------------------------------------------------
// x, y, w, h are the dimension of the widget not the text area (contrario to the nedit TextDCreate.
// lineNumleft disapear here and computed with lineNumWidth
// A 0 value for lineNumWidth desactivate the line number display
// -------------------------------------------------------------------------------
Ne_Text_Display* Ne_Text_Display::Create(int x, int y, int w, int h,
   int lineNumWidth,
   Ne_Text_Buffer* buffer,
   const Ne_Font& font,
   Fl_Color bgPixel, Fl_Color fgPixel,
   Fl_Color selectFGPixel, Fl_Color selectBGPixel,
   Fl_Color highlightFGPixel, Fl_Color highlightBGPixel,
   Fl_Color cursorFGPixel, Fl_Color lineNumFGPixel,
   bool continuousWrap, int wrapMargin,
   const char* bgClassString,
   Fl_Color calltipFGPixel, Fl_Color calltipBGPixel)
{
   Ne_Text_Display* textD = new Ne_Text_Display(x, y, w, h);
   textD->buffer = buffer;

   textD->fontStruct = font;
   textD->descent = textD->fontStruct.descent();
   textD->ascent = textD->fontStruct.ascent();
   textD->fixedFontWidth = textD->fontStruct.fixedFontWidth();

   textD->styleBuffer = NULL;
   textD->styleTable = NULL;
   textD->nStyles = 0;
   textD->bgPixel = bgPixel;
   textD->fgPixel = fgPixel;
   textD->selectFGPixel = selectFGPixel;
   textD->highlightFGPixel = highlightFGPixel;
   textD->selectBGPixel = selectBGPixel;
   textD->highlightBGPixel = highlightBGPixel;
   textD->lineNumFGPixel = lineNumFGPixel;
   textD->cursorFGPixel = cursorFGPixel;
   textD->wrapMargin = wrapMargin;
   textD->continuousWrap = continuousWrap;
   textD->lineNumWidth = lineNumWidth;
   textD->computeTextAreaSize(x, y, w, h);

   textD->nVisibleLines = (textD->height - 1) / (textD->ascent + textD->descent);
   if (textD->nVisibleLines == 0) ++textD->nVisibleLines; // Avoid 0 array allocation
   textD->lineStarts = new int[textD->nVisibleLines];
   textD->lineStarts[0] = 0;
   for (int i = 1; i < textD->nVisibleLines; i++)
      textD->lineStarts[i] = -1;
   // TODO:     textD->calltipW = NULL;
   // TODO:     textD->calltipShell = NULL;
   // TODO:     textD->calltip.ID = 0;
    textD->calltipFGPixel = calltipFGPixel;
    textD->calltipBGPixel = calltipBGPixel;
    textD->bgClassPixel = NULL;
    textD->bgClass = NULL;
    textD->setupBGClasses(bgClassString, &textD->bgClassPixel, &textD->bgClass, bgPixel);
    textD->suppressResync = 0;
    textD->nLinesDeleted = 0;
    textD->modifyingTabDist = 0;
    textD->pointerHidden = false;

   // Attach the callback to the text buffer for receiving modification information
   if (buffer != NULL)
   {
      buffer->addModifyCB(NeBufModifiedCB, textD);
      buffer->addPreDeleteCB(NeBufPreDeleteCB, textD);
   }

   // Update the display to reflect the contents of the buffer
   if (buffer != NULL)
      NeBufModifiedCB(0, buffer->size(), 0, 0, NULL, textD);

   // Decide if the horizontal scroll bar needs to be visible
   textD->hideOrShowHScrollBar();

   return textD;
}

/*
** Free a text display and release its associated memory.  Note, the text
** BUFFER that the text display displays is a separate entity and is not
** freed, nor are the style buffer or style table.
*/
Ne_Text_Display::~Ne_Text_Display()
{
   this->buffer->removeModifyCB(NeBufModifiedCB, this);
   this->buffer->removePreDeleteCB(NeBufPreDeleteCB, this);
   delete[] this->lineStarts;
    free((char*)this->bgClassPixel);
    free((char*)this->bgClass);
}

/*
** Attach a text buffer to display, replacing the current buffer (if any)
*/
void Ne_Text_Display::setBuffer(Ne_Text_Buffer* buffer)
{
   /* If the text display is already displaying a buffer, clear it off
   of the display and remove our callback from it */
   if (this->buffer != NULL)
   {
      bufModifiedCB(0, 0, this->buffer->size(), 0, NULL);
      this->buffer->removeModifyCB(NeBufModifiedCB, this);
      this->buffer->removePreDeleteCB(NeBufPreDeleteCB, this);
   }

   // Add the buffer to the display, and attach a callback to the buffer for
   // receiving modification information when the buffer contents change
   this->buffer = buffer;
   buffer->addModifyCB(NeBufModifiedCB, this);
   buffer->addPreDeleteCB(NeBufPreDeleteCB, this);

   // Update the display
   bufModifiedCB(0, buffer->size(), 0, 0, NULL);
}

/*
** Get the buffer associated with this text widget.  Note that attaching
** additional modify callbacks to the buffer will prevent it from being
** automatically freed when the widget is destroyed.
*/
Ne_Text_Buffer* Ne_Text_Display::getBuffer()
{
   return this->buffer;
}

/*
** Attach (or remove) highlight information in text display and redisplay.
** Highlighting information consists of a style buffer which parallels the
** normal text buffer, but codes font and color information for the display;
** a style table which translates style buffer codes (indexed by buffer
** character - 65 (ASCII code for 'A')) into fonts and colors; and a callback
** mechanism for as-needed highlighting, triggered by a style buffer entry of
** "unfinishedStyle".  Style buffer can trigger additional redisplay during
** a normal buffer modification if the buffer contains a primary selection
** (see extendRangeForStyleMods for more information on this protocol).
**
** Style buffers, tables and their associated memory are managed by the caller.
*/
void Ne_Text_Display::attachHighlightData(Ne_Text_Buffer* styleBuffer,
   NeStyleTableEntry* styleTable, int nStyles, char unfinishedStyle,
   NeUnfinishedStyleCBProc* unfinishedHighlightCB, void* cbArg)
{
   this->styleBuffer = styleBuffer;
   this->styleTable = styleTable;
   this->nStyles = nStyles;
   this->unfinishedStyle = unfinishedStyle;
   this->unfinishedHighlightCB = unfinishedHighlightCB;
   this->highlightCBArg = cbArg;

   // Call TextDSetFont to combine font information from style table and
   // primary font, adjust font-related parameters, and then redisplay
   setFont(this->fontStruct);
}

// --------------------------------------------------------------------------
// Change the (non syntax-highlit) colors
// --------------------------------------------------------------------------
void Ne_Text_Display::setColors(Fl_Color textFgP, Fl_Color textBgP,
   Fl_Color selectFgP, Fl_Color selectBgP, Fl_Color hiliteFgP, Fl_Color hiliteBgP,
   Fl_Color lineNoFgP, Fl_Color cursorFgP)
{
   // Update the stored pixels
   this->fgPixel = textFgP;
   this->bgPixel = textBgP;
   this->selectFGPixel = selectFgP;
   this->selectBGPixel = selectBgP;
   this->highlightFGPixel = hiliteFgP;
   this->highlightBGPixel = hiliteBgP;
   this->lineNumFGPixel = lineNoFgP;
   this->cursorFGPixel = cursorFgP;

   // Redisplay
   redraw();
}

// --------------------------------------------------------------------------
// Change the (non highlight) font
// --------------------------------------------------------------------------
void Ne_Text_Display::setFont(const Ne_Font& fontStruct)
{
   this->descent = fontStruct.descent();
   this->ascent = fontStruct.ascent();

   int maxAscent = this->ascent;
   int maxDescent = this->descent;
   Ne_Font* styleFont = 0;

    // If font size changes, cursor will be redrawn in a new position
    blankCursorProtrusions();

    // If there is a (syntax highlighting) style table in use, find the new maximum font height for this text display
    for (int i = 0; i < this->nStyles; i++)
    {
       styleFont = &this->styleTable[i].font;
       if (styleFont != NULL && styleFont->ascent() > maxAscent)
          maxAscent = styleFont->ascent();
       if (styleFont != NULL && styleFont->descent() > maxDescent)
          maxDescent = styleFont->descent();
    }
   this->ascent = maxAscent;
   this->descent = maxDescent;

   // If all of the current fonts are fixed and match in width, compute
   int fontWidth = fontStruct.max_width();
   fixedFontWidth = fontStruct.fixedFontWidth();

   if (fontWidth != -1)
   {
       for (int i = 0; i < this->nStyles; i++)
       {
          styleFont = &this->styleTable[i].font;
          if (styleFont != NULL &&
                (styleFont->max_width() != fontWidth ||
                 styleFont->max_width() != styleFont->min_width()))
             fontWidth = -1;
       }
   }
   this->fixedFontWidth = fontWidth;

   // Change the font.  In most cases, this means re-allocating the
   // affected GCs (they are shared with other widgets, and if the primary
   // font changes, must be re-allocated to change it). Unfortunately,
   // this requres recovering all of the colors from the existing GCs
   this->fontStruct = fontStruct;

   // Do a full resize to force recalculation of font related parameters
   resizeTextArea(this->width, this->height, 0);

   // Redisplay
   redraw();
}

// -------------------------------------------------------------------------
int Ne_Text_Display::minFontWidth(bool considerStyles)
{
   int fontWidth = this->fontStruct.min_width();

   if (considerStyles)
   {
      for (int i = 0; i < this->nStyles; ++i)
         fontWidth = std::min(fontWidth, this->styleTable[i].font.min_width());
   }
   return fontWidth;
}

// -------------------------------------------------------------------------
int Ne_Text_Display::maxFontWidth(bool considerStyles)
{
   int fontWidth = this->fontStruct.max_width();

   if (considerStyles)
   {
      for (int i = 0; i < this->nStyles; ++i)
         fontWidth = std::max(fontWidth, this->styleTable[i].font.max_width());
   }
   return fontWidth;
}

/*
** Change the size of the displayed text area
*/
void Ne_Text_Display::resizeTextArea(int width, int height, int oldWidth)
{
   int oldVisibleLines = this->nVisibleLines;
   int newVisibleLines = height / (this->ascent + this->descent);
   int redrawAll = false;
   int exactHeight = height - height % (this->ascent + this->descent);


    // In continuous wrap mode, a change in width affects the total number of
    // lines in the buffer, and can leave the top line number incorrect, and
    // the top character no longer pointing at a valid line start
    if (this->continuousWrap && this->wrapMargin == 0 && width != oldWidth)
    {
       int oldFirstChar = this->firstChar;
       this->nBufferLines = countLines(0, this->buffer->size(), true);
       this->firstChar = startOfLine(this->firstChar);
       this->topLineNum = countLines(0, this->firstChar, true) + 1;
       redrawAll = true;
       offsetAbsLineNum(oldFirstChar);
    }

   // reallocate and update the line starts array, which may have changed
   // size and/or contents. (contents can change in continuous wrap mode
   // when the width changes, even without a change in height)
   if (oldVisibleLines < newVisibleLines)
   {
      delete[] this->lineStarts;
      this->lineStarts = new int[newVisibleLines];
   }
   this->nVisibleLines = newVisibleLines;
   calcLineStarts(0, newVisibleLines);
   calcLastChar();

   // if the window became taller, there may be an opportunity to display more text by scrolling down
   if (canRedraw && oldVisibleLines < newVisibleLines && this->topLineNum + this->nVisibleLines > this->nBufferLines)
      setScroll(std::max(1, this->nBufferLines - this->nVisibleLines + 2 + this->cursorVPadding), this->horizOffset, false, false);

   // Update the scroll bar page increment size (as well as other scroll
   // bar parameters.  If updating the horizontal range caused scrolling, redraw
   updateVScrollBarRange();
   if (updateHScrollBarRange())
      redrawAll = true;

    // If a full redraw is needed
    if (redrawAll && canRedraw)
       redisplayRect(this->left, this->top, this->width, this->height);

    // Decide if the horizontal scroll bar needs to be visible
    hideOrShowHScrollBar();

    // Refresh the line number display to draw more line numbers, or erase extras
    redrawLineNumbers(true);

    // Redraw the calltip
// TODO:     TextDRedrawCalltip(textD, 0);
}

/*
** Refresh a rectangle of the text display.  left and top are in coordinates of
** the text drawing window (including line nmuber area)
*/
void Ne_Text_Display::redisplayRect(int left, int top, int width, int height)
{
   // Find the line number range of the display
   int fontHeight = this->ascent + this->descent;
   int firstLine = (top - this->top - fontHeight + 1) / fontHeight;
   int lastLine = (top + height - this->top) / fontHeight;

   // If the graphics contexts are shared using XtAllocateGC, their clipping rectangles may have changed since the last use
   fl_push_clip(std::max(left, this->left), std::max(top, this->top), std::min(width, this->width), std::min(height, this->height));

   // Draw the lines of text
   for (int line = firstLine; line <= lastLine; line++)
      redisplayLine(line, left, left + width, 0, INT_MAX);
   
   fl_pop_clip();

   // Draw the line numbers if exposed area includes them
   if (this->lineNumWidth != 0 && left <= this->lineNumLeft + this->lineNumWidth)
      redrawLineNumbers(false);
}

 /*
 ** Refresh all of the text between buffer positions "start" and "end"
 ** not including the character at the position "end".
 ** If end points beyond the end of the buffer, refresh the whole display
 ** after pos, including blank lines which are not technically part of
 ** any range of characters.
 */
 void Ne_Text_Display::redisplayRange(int start, int end)
 {
    int i, startLine, lastLine, startIndex, endIndex;

    // If the range is outside of the displayed text, just return
    if (end < this->firstChar || (start > this->lastChar && !emptyLinesVisible()))
       return;

    // Clean up the starting and ending values
    if (start < 0) start = 0;
    if (start > this->buffer->size()) start = this->buffer->size();
    if (end < 0) end = 0;
    if (end > this->buffer->size()) end = this->buffer->size();

    // Get the starting and ending lines
    if (start < this->firstChar)
    {
       start = this->firstChar;
    }

    if (!posToVisibleLineNum(start, &startLine))
    {
       startLine = this->nVisibleLines - 1;
    }

    if (end >= this->lastChar)
    {
       lastLine = this->nVisibleLines - 1;
    }
    else
    {
       if (!posToVisibleLineNum(end, &lastLine))
       {
          // shouldn't happen
          lastLine = this->nVisibleLines - 1;
       }
    }

    // Get the starting and ending positions within the lines
    startIndex = (this->lineStarts[startLine] == -1)
                 ? 0
                 : start - this->lineStarts[startLine];
    if (end >= this->lastChar)
    {
       // Request to redisplay beyond this->lastChar, so tell
       // redisplayLine() to display everything to infy.
       endIndex = INT_MAX;
    }
    else if (this->lineStarts[lastLine] == -1)
    {
       // Here, lastLine is determined by posToVisibleLineNum() (see
       // if/else above) but deemed to be out of display according to
       // this->lineStarts.
       endIndex = 0;
    }
    else
    {
       endIndex = end - this->lineStarts[lastLine];
    }

    // If the starting and ending lines are the same, redisplay the single
    // line between "start" and "end"
    if (startLine == lastLine)
    {
       redisplayLine(startLine, 0, INT_MAX, startIndex, endIndex);
       return;
    }

    // Redisplay the first line from "start"
    redisplayLine(startLine, 0, INT_MAX, startIndex, INT_MAX);

    // Redisplay the lines in between at their full width
    for (i = startLine + 1; i < lastLine; i++)
       redisplayLine(i, 0, INT_MAX, 0, INT_MAX);

    // Redisplay the last line to "end"
    redisplayLine(lastLine, 0, INT_MAX, 0, endIndex);
 }

/*
** Set the scroll position of the text display vertically by line number and
** horizontally by pixel offset from the left margin
*/
void Ne_Text_Display::setScroll(int topLineNum, int horizOffset)
{
   int sliderSize = 0, sliderMax = 0;
   int vPadding = this->cursorVPadding;

   // Limit the requested scroll position to allowable values
   if (topLineNum < 1)
      topLineNum = 1;
   else if ((topLineNum > this->topLineNum) &&
      (topLineNum > (this->nBufferLines + 2 - this->nVisibleLines + vPadding)))
      topLineNum = std::max(this->topLineNum, this->nBufferLines + 2 - this->nVisibleLines + vPadding);

   if (horizOffset < 0)
      horizOffset = 0;

   setScroll(topLineNum, horizOffset, true, true);
}

/*
** Get the current scroll position for the text display, in terms of line
** number of the top line and horizontal pixel offset from the left margin
*/
void Ne_Text_Display::getScroll(int* topLineNum, int* horizOffset)
{
   *topLineNum = this->topLineNum;
   *horizOffset = this->horizOffset;
}

/*
** Set the position of the text insertion cursor for text display "textD"
*/
void Ne_Text_Display::setInsertPosition(int newPos)
{
   /* make sure new position is ok, do nothing if it hasn't changed */
   if (newPos == this->cursorPos)
      return;
   if (newPos < 0) newPos = 0;
   if (newPos > this->buffer->size()) newPos = this->buffer->size();

   /* cursor movement cancels vertical cursor motion column */
   this->cursorPreferredCol = -1;

   /* erase the cursor at it's previous position */
   blankCursor();

   /* draw it at its new position */
   this->cursorPos = newPos;
   this->cursorOn = true;
   redisplayRange(this->cursorPos - 1, this->cursorPos + 1);
}

 void Ne_Text_Display::blankCursor()
 {
    if (!this->cursorOn)
       return;

    blankCursorProtrusions();
    this->cursorOn = false;
    redisplayRange(this->cursorPos - 1, this->cursorPos + 1);
 }

 void Ne_Text_Display::unblankCursor()
 {
    if (!this->cursorOn)
    {
       this->cursorOn = true;
       redisplayRange(this->cursorPos - 1, this->cursorPos + 1);
    }
 }

void Ne_Text_Display::setCursorStyle(NeCursorStyles style)
{
   this->cursorStyle = style;
   blankCursorProtrusions();
   if (this->cursorOn)
      redisplayRange(this->cursorPos - 1, this->cursorPos + 1);
}

void Ne_Text_Display::setWrapMode(bool wrap, int wrapMargin)
{
   this->wrapMargin = wrapMargin;
   this->continuousWrap = wrap;

   /* wrapping can change change the total number of lines, re-count */
   this->nBufferLines = countLines(0, this->buffer->size(), true);

   /* changing wrap margins wrap or changing from wrapped mode to non-wrapped
   can leave the character at the top no longer at a line start, and/or
   change the line number */
   this->firstChar = startOfLine(this->firstChar);
   this->topLineNum = countLines(0, this->firstChar, true) + 1;
   resetAbsLineNum();

   // update the line starts array
   calcLineStarts(0, this->nVisibleLines);
   calcLastChar();

   // Update the scroll bar page increment size (as well as other scroll bar parameters)
   updateVScrollBarRange();
   updateHScrollBarRange();

   // Decide if the horizontal scroll bar needs to be visible
   hideOrShowHScrollBar();

   // Do a full redraw
   redraw();
}

// --------------------------------------------------------------------------
int Ne_Text_Display::getInsertPosition()
{
   return this->cursorPos;
}

/*
** Insert "text" at the current cursor location.  This has the same
** effect as inserting the text into the buffer using BufInsert and
** then moving the insert position after the newly inserted text, except
** that it's optimized to do less redrawing.
*/
void Ne_Text_Display::insert(const char* text)
{
   int pos = this->cursorPos;

   this->cursorToHint = pos + strlen(text);
   this->buffer->insertAt(pos, text);
   this->cursorToHint = NO_HINT;
}

/*
** Insert "text" (which must not contain newlines), overstriking the current
** cursor location.
*/
void Ne_Text_Display::overstrike(const char* text)
{
   int startPos = this->cursorPos;
   Ne_Text_Buffer* buf = this->buffer;
   int lineStart = this->buffer->startOfLine(startPos);
   int textLen = strlen(text);
   int i, p, endPos, indent, startIndent, endIndent;
   char ch, *paddedText = NULL;
   const char* c;

   // determine how many displayed character positions are covered
   startIndent = this->buffer->countDispChars(lineStart, startPos);
   indent = startIndent;
   for (c = text; *c != '\0'; c++)
      indent += this->buffer->charWidth(*c, indent, buf->getTabDistance(), buf->getNullSubsChar());
   endIndent = indent;

   // find which characters to remove, and if necessary generate additional
   // padding to make up for removed control characters at the end
   indent = startIndent;
   for (p = startPos; ; p++)
   {
      if (p == buf->size())
         break;
      ch = buffer->getCharacter(p);
      if (ch == '\n')
         break;
      indent += this->buffer->charWidth(ch, indent, buf->getTabDistance(), buf->getNullSubsChar());
      if (indent == endIndent)
      {
         p++;
         break;
      }
      else if (indent > endIndent)
      {
         if (ch != '\t')
         {
            p++;
            paddedText = (char*)malloc(textLen + MAX_EXP_CHAR_LEN + 1);
            strcpy(paddedText, text);
            for (i = 0; i < indent - endIndent; i++)
               paddedText[textLen + i] = ' ';
            paddedText[textLen + i] = '\0';
         }
         break;
      }
   }
   endPos = p;

   this->cursorToHint = startPos + textLen;
   this->buffer->replace(startPos, endPos, paddedText == NULL ? text : paddedText);
   this->cursorToHint = NO_HINT;
   free(paddedText);
}

/*
** Translate window coordinates to the nearest text cursor position.
*/
int Ne_Text_Display::XYToPosition(int x, int y)
{
   return xyToPos(x, y, NE_CURSOR_POS);
}

 /*
 ** Translate window coordinates to the nearest character cell.
 */
 int Ne_Text_Display::XYToCharPos(int x, int y)
 {
    return xyToPos(x, y, NE_CHARACTER_POS);
 }

/*
** Translate window coordinates to the nearest row and column number for
** positioning the cursor.  This, of course, makes no sense when the font
** is proportional, since there are no absolute columns.
*/
void Ne_Text_Display::XYToUnconstrainedPosition(int x, int y, int* row, int* column)
{
   xyToUnconstrainedPos(x, y, row, column, NE_CURSOR_POS);
}

 /*
 ** Translate line and column to the nearest row and column number for
 ** positioning the cursor.  This, of course, makes no sense when the font
 ** is proportional, since there are no absolute columns.
 */
 int Ne_Text_Display::lineAndColToPos(int lineNum, int column)
 {
    int i, lineEnd, charIndex, outIndex;
    int lineStart = 0, charLen = 0;
    char* lineStr, expandedChar[MAX_EXP_CHAR_LEN];

    /* Count lines */
    if (lineNum < 1)
       lineNum = 1;
    lineEnd = -1;
    for (i = 1; i <= lineNum && lineEnd < this->buffer->size(); i++)
    {
       lineStart = lineEnd + 1;
       lineEnd = buffer->endOfLine(lineStart);
    }

    /* If line is beyond end of buffer, position at last character in buffer */
    if (lineNum >= i)
    {
       return lineEnd;
    }

    /* Start character index at zero */
    charIndex = 0;

    /* Only have to count columns if column isn't zero (or negative) */
    if (column > 0)
    {
       /* Count columns, expanding each character */
       lineStr = strdup(this->buffer->getRange(lineStart, lineEnd).c_str());
       outIndex = 0;
       for (i = lineStart; i < lineEnd; i++, charIndex++)
       {
          charLen = buffer->expandCharacter(lineStr[charIndex], outIndex,
                                       expandedChar, this->buffer->getTabDistance(),
                                       this->buffer->getNullSubsChar());
          if (outIndex + charLen >= column) break;
          outIndex += charLen;
       }

       /* If the column is in the middle of an expanded character, put cursor
        * in front of character if in first half of character, and behind
        * character if in last half of character
        */
       if (column >= outIndex + (charLen / 2))
          charIndex++;

       /* If we are beyond the end of the line, back up one space */
       if ((i >= lineEnd) && (charIndex > 0)) charIndex--;
    }

    /* Position is the start of the line plus the index into line buffer */
    free(lineStr);
    return lineStart + charIndex;
 }

/*
** Translate a buffer text position to the XY location where the center
** of the cursor would be positioned to point to that character.  Returns
** false if the position is not displayed because it is VERTICALLY out
** of view.  If the position is horizontally out of view, returns the
** x coordinate where the position would be if it were visible.
*/
bool Ne_Text_Display::positionToXY(int pos, int* x, int* y)
{
   int charIndex, lineStartPos, fontHeight, lineLen;
   int visLineNum, charLen, outIndex, xStep, charStyle;
   char expandedChar[MAX_EXP_CHAR_LEN];
   std::string lineStr;

   // If position is not displayed, return false
   if (pos < this->firstChar ||
      (pos > this->lastChar && !emptyLinesVisible()))
      return false;

   // Calculate y coordinate
   if (!posToVisibleLineNum(pos, &visLineNum))
      return false;
   fontHeight = this->ascent + this->descent;
   *y = this->top + visLineNum * fontHeight + fontHeight / 2;

   // Get the text, length, and  buffer position of the line. If the position
   // is beyond the end of the buffer and should be at the first position on
   // the first empty line, don't try to get or scan the text
   lineStartPos = this->lineStarts[visLineNum];
   if (lineStartPos == -1)
   {
      *x = this->left - this->horizOffset;
      return true;
   }
   lineLen = visLineLength(visLineNum);
   lineStr = this->buffer->getRange(lineStartPos, lineStartPos + lineLen);

   // Step through character positions from the beginning of the line to "pos" to calculate the x coordinate
   xStep = this->left - this->horizOffset;
   outIndex = 0;
   for (charIndex = 0; charIndex < pos - lineStartPos; charIndex++)
   {
      charLen = buffer->expandCharacter(lineStr[charIndex], outIndex, expandedChar, this->buffer->getTabDistance(), this->buffer->getNullSubsChar());
      charStyle = styleOfPos(lineStartPos, lineLen, charIndex, outIndex, lineStr[charIndex]);
      xStep += stringWidth(expandedChar, charLen, charStyle);
      outIndex += charLen;
   }
   *x = xStep;
   return true;
}

/*
** If the text widget is maintaining a line number count appropriate to "pos"
** return the line and column numbers of pos, otherwise return false.  If
** continuous wrap mode is on, returns the absolute line number (as opposed to
** the wrapped line number which is used for scrolling).  THIS ROUTINE ONLY
** WORKS FOR DISPLAYED LINES AND, IN CONTINUOUS WRAP MODE, ONLY WHEN THE
** ABSOLUTE LINE NUMBER IS BEING MAINTAINED.  Otherwise, it returns false.
*/
int Ne_Text_Display::posToLineAndCol(int pos, int* lineNum, int* column)
{
   Ne_Text_Buffer* buf = this->buffer;

   /* In continuous wrap mode, the absolute (non-wrapped) line count is
   maintained separately, as needed.  Only return it if we're actually
   keeping track of it and pos is in the displayed text */
   if (this->continuousWrap)
   {
      if (!maintainingAbsTopLineNum() || pos < this->firstChar ||
         pos > this->lastChar)
         return false;
      *lineNum = this->absTopLineNum + buf->countLines( this->firstChar, pos);
      *column = buf->countDispChars(buf->startOfLine(pos), pos);
      return true;
   }

   /* Only return the data if pos is within the displayed text */
   if (!posToVisibleLineNum(pos, lineNum))
      return false;
   *column = buf->countDispChars(this->lineStarts[*lineNum], pos);
   *lineNum += this->topLineNum;
   return true;
}

/*
** Return true if position (x, y) is inside of the primary selection
*/
bool Ne_Text_Display::inSelection(int x, int y)
{
   int row, column, pos = xyToPos(x, y, NE_CHARACTER_POS);
   Ne_Text_Buffer* buf = this->buffer;

   xyToUnconstrainedPos(x, y, &row, &column, NE_CHARACTER_POS);
   return inSelection(&buf->getSelection(), pos, buf->startOfLine(pos), column);
}

/*
** Correct a column number based on an unconstrained position (as returned by
** TextDXYToUnconstrainedPosition) to be relative to the last actual newline
** in the buffer before the row and column position given, rather than the
** last line start created by line wrapping.  This is an adapter
** for rectangular selections and code written before continuous wrap mode,
** which thinks that the unconstrained column is the number of characters
** from the last newline.  Obviously this is time consuming, because it
** invloves character re-counting.
*/
int Ne_Text_Display::OffsetWrappedColumn(int row, int column)
{
   int lineStart, dispLineStart;

   if (!this->continuousWrap || row < 0 || row > this->nVisibleLines)
      return column;
   dispLineStart = this->lineStarts[row];
   if (dispLineStart == -1)
      return column;
   lineStart = this->buffer->startOfLine(dispLineStart);
   return column + this->buffer->countDispChars(lineStart, dispLineStart);
}

 /*
 ** Correct a row number from an unconstrained position (as returned by
 ** TextDXYToUnconstrainedPosition) to a straight number of newlines from the
 ** top line of the display.  Because rectangular selections are based on
 ** newlines, rather than display wrapping, and anywhere a rectangular selection
 ** needs a row, it needs it in terms of un-wrapped lines.
 */
 int Ne_Text_Display::OffsetWrappedRow(int row)
 {
    if (!this->continuousWrap || row < 0 || row > this->nVisibleLines)
       return row;
    return this->buffer->countLines(this->firstChar, this->lineStarts[row]);
 }

/*
** Scroll the display to bring insertion cursor into view.
**
** Note: it would be nice to be able to do this without counting lines twice
** (setScroll counts them too) and/or to count from the most efficient
** starting point, but the efficiency of this routine is not as important to
** the overall performance of the text display.
*/
void Ne_Text_Display::makeInsertPosVisible()
{
   int hOffset, topLine, x, y;
   int cursorPos = this->cursorPos;
   int linesFromTop = 0, do_padding = 1;
   int cursorVPadding = this->cursorVPadding;

   hOffset = this->horizOffset;
   topLine = this->topLineNum;

   // Don't do padding if this is a mouse operation
   do_padding = (this->dragState == NE_NOT_CLICKED) && (cursorVPadding > 0);

   // Find the new top line number
   if (cursorPos < this->firstChar)
   {
      topLine -= countLines(cursorPos, this->firstChar, false);
      /* linesFromTop = 0; */
   }
   else if (cursorPos > this->lastChar && !emptyLinesVisible())
   {
      topLine += countLines(this->lastChar - (wrapUsesCharacter(this->lastChar) ? 0 : 1), cursorPos, false);
      linesFromTop = this->nVisibleLines - 1;
   }
   else if (cursorPos == this->lastChar && !emptyLinesVisible() && !wrapUsesCharacter(this->lastChar))
   {
      topLine++;
      linesFromTop = this->nVisibleLines - 1;
   }
   else
   {
      /* Avoid extra counting if cursorVPadding is disabled */
      if (do_padding)
         linesFromTop = countLines(this->firstChar, cursorPos, true);
   }
   if (topLine < 1)
   {
      fprintf(stderr, "nedit: internal consistency check tl1 failed\n");
      topLine = 1;
   }

   if (do_padding)
   {
      /* Keep the cursor away from the top or bottom of screen. */
      if (this->nVisibleLines <= 2 * (int)cursorVPadding)
      {
         topLine += (linesFromTop - this->nVisibleLines / 2);
         topLine = std::max(topLine, 1);
      }
      else if (linesFromTop < (int)cursorVPadding)
      {
         topLine -= (cursorVPadding - linesFromTop);
         topLine = std::max(topLine, 1);
      }
      else if (linesFromTop > this->nVisibleLines - (int)cursorVPadding - 1)
      {
         topLine += (linesFromTop - (this->nVisibleLines - cursorVPadding - 1));
      }
   }

   // Find the new setting for horizontal offset (this is a bit ungraceful).
   // If the line is visible, just use TextDPositionToXY to get the position
   // to scroll to, otherwise, do the vertical scrolling first, then the horizontal
   if (!positionToXY(cursorPos, &x, &y))
   {
      setScroll(topLine, hOffset, true, true);
      if (!positionToXY(cursorPos, &x, &y))
         return; // Give up, it's not worth it (but why does it fail?)
   }
   if (x > this->left + this->width)
      hOffset += x - (this->left + this->width);
   else if (x < this->left)
      hOffset += x - this->left;

   // Do the scroll
   setScroll( topLine, hOffset, true, true);
}

/*
** Return the current preferred column along with the current
** visible line index (-1 if not visible) and the lineStartPos
** of the current insert position.
*/
int Ne_Text_Display::preferredColumn(int* visLineNum, int* lineStartPos)
{
   int column;

   /* Find the position of the start of the line.  Use the line starts array
   if possible, to avoid unbounded line-counting in continuous wrap mode */
   if (posToVisibleLineNum(this->cursorPos, visLineNum))
   {
      *lineStartPos = this->lineStarts[*visLineNum];
   }
   else
   {
      *lineStartPos = startOfLine(this->cursorPos);
      *visLineNum = -1;
   }

   /* Decide what column to move to, if there's a preferred column use that */
   column = (this->cursorPreferredCol >= 0)
      ? this->cursorPreferredCol
      : this->buffer->countDispChars(*lineStartPos, this->cursorPos);
   return(column);
}

/*
** Return the insert position of the requested column given
** the lineStartPos.
*/
int Ne_Text_Display::posOfPreferredCol(int column, int lineStartPos)
{
   int newPos = this->buffer->countForwardDispChars(lineStartPos, column);
   if (this->continuousWrap)
   {
      newPos = std::min(newPos, endOfLine( lineStartPos, true));
   }
   return(newPos);
}

// --------------------------------------------------------------------------
// Cursor movement functions
// --------------------------------------------------------------------------
bool Ne_Text_Display::moveRight()
{
   if (this->cursorPos >= this->buffer->size())
      return false;
   setInsertPosition(this->cursorPos + 1);
   return true;
}

// --------------------------------------------------------------------------
bool Ne_Text_Display::moveLeft()
{
   if (this->cursorPos <= 0)
      return false;
   setInsertPosition(this->cursorPos - 1);
   return true;
}

// --------------------------------------------------------------------------
bool Ne_Text_Display::moveUp(bool absolute)
{
   int lineStartPos, column, prevLineStartPos, newPos, visLineNum;

   // Find the position of the start of the line.  Use the line starts array
   // if possible, to avoid unbounded line-counting in continuous wrap mode
   if (absolute)
   {
      lineStartPos = this->buffer->startOfLine(this->cursorPos);
      visLineNum = -1;
   }
   else if (posToVisibleLineNum(this->cursorPos, &visLineNum))
      lineStartPos = this->lineStarts[visLineNum];
   else
   {
      lineStartPos = startOfLine(this->cursorPos);
      visLineNum = -1;
   }
   if (lineStartPos == 0)
      return false;

   // Decide what column to move to, if there's a preferred column use that
   column = this->cursorPreferredCol >= 0
      ? this->cursorPreferredCol
      : this->buffer->countDispChars(lineStartPos, this->cursorPos);

   // count forward from the start of the previous line to reach the column
   if (absolute)
   {
      prevLineStartPos = this->buffer->countBackwardNLines(lineStartPos, 1);
   }
   else if (visLineNum != -1 && visLineNum != 0)
   {
      prevLineStartPos = this->lineStarts[visLineNum - 1];
   }
   else
   {
      prevLineStartPos = countBackwardNLines(lineStartPos, 1);
   }

   newPos = this->buffer->countForwardDispChars(prevLineStartPos, column);
   if (this->continuousWrap && !absolute)
      newPos = std::min(newPos, endOfLine(prevLineStartPos, true));

   // move the cursor
   setInsertPosition(newPos);

   // if a preferred column wasn't aleady established, establish it
   this->cursorPreferredCol = column;

   return true;
}

// --------------------------------------------------------------------------
bool Ne_Text_Display::moveDown(bool absolute)
{
   int lineStartPos, column, nextLineStartPos, newPos, visLineNum;

   if (this->cursorPos == this->buffer->size())
   {
      return false;
   }

   if (absolute)
   {
      lineStartPos = this->buffer->startOfLine(this->cursorPos);
      visLineNum = -1;
   }
   else if (posToVisibleLineNum(this->cursorPos, &visLineNum))
   {
      lineStartPos = this->lineStarts[visLineNum];
   }
   else
   {
      lineStartPos = startOfLine(this->cursorPos);
      visLineNum = -1;
   }

   column = this->cursorPreferredCol >= 0
      ? this->cursorPreferredCol
      : this->buffer->countDispChars(lineStartPos, this->cursorPos);

   if (absolute)
      nextLineStartPos = this->buffer->countForwardNLines(lineStartPos, 1);
   else
      nextLineStartPos = countForwardNLines(lineStartPos, 1, true);

   newPos = this->buffer->countForwardDispChars(nextLineStartPos, column);

   if (this->continuousWrap && !absolute)
   {
      newPos = std::min(newPos, endOfLine(nextLineStartPos, true));
   }

   setInsertPosition(newPos);
   this->cursorPreferredCol = column;

   return true;
}

/*
** Same as BufCountLines, but takes in to account wrapping if wrapping is
** turned on.  If the caller knows that startPos is at a line start, it
** can pass "startPosIsLineStart" as true to make the call more efficient
** by avoiding the additional step of scanning back to the last newline.
*/
int Ne_Text_Display::countLines(int startPos, int endPos, bool startPosIsLineStart)
{
   int retLines, retPos, retLineStart, retLineEnd;

   // If we're not wrapping use simple (and more efficient) BufCountLines
   if (!this->continuousWrap)
      return this->buffer->countLines(startPos, endPos);

   wrappedLineCounter(this->buffer, startPos, endPos, INT_MAX,
      startPosIsLineStart, 0, &retPos, &retLines, &retLineStart,
      &retLineEnd);
   return retLines;
}

/*
** Same as BufCountForwardNLines, but takes in to account line breaks when
** wrapping is turned on. If the caller knows that startPos is at a line start,
** it can pass "startPosIsLineStart" as true to make the call more efficient
** by avoiding the additional step of scanning back to the last newline.
*/
int Ne_Text_Display::countForwardNLines(const int startPos, const unsigned nLines, const bool startPosIsLineStart)
{
   int retLines, retPos, retLineStart, retLineEnd;

   /* if we're not wrapping use more efficient BufCountForwardNLines */
   if (!this->continuousWrap)
      return this->buffer->countForwardNLines(startPos, nLines);

   /* wrappedLineCounter can't handle the 0 lines case */
   if (nLines == 0)
      return startPos;

   /* use the common line counting routine to count forward */
   wrappedLineCounter(this->buffer, startPos, this->buffer->size(),
      nLines, startPosIsLineStart, 0, &retPos, &retLines, &retLineStart,
      &retLineEnd);
   return retPos;
}

/*
** Same as BufEndOfLine, but takes in to account line breaks when wrapping
** is turned on.  If the caller knows that startPos is at a line start, it
** can pass "startPosIsLineStart" as true to make the call more efficient
** by avoiding the additional step of scanning back to the last newline.
**
** Note that the definition of the end of a line is less clear when continuous
** wrap is on.  With continuous wrap off, it's just a pointer to the newline
** that ends the line.  When it's on, it's the character beyond the last
** DISPLAYABLE character on the line, where a whitespace character which has
** been "converted" to a newline for wrapping is not considered displayable.
** Also note that, a line can be wrapped at a non-whitespace character if the
** line had no whitespace.  In this case, this routine returns a pointer to
** the start of the next line.  This is also consistent with the model used by
** visLineLength.
*/
int Ne_Text_Display::endOfLine(const int pos, const bool startPosIsLineStart)
{
   int retLines, retPos, retLineStart, retLineEnd;

   /* If we're not wrapping use more efficient BufEndOfLine */
   if (!this->continuousWrap)
      return this->buffer->endOfLine(pos);

   if (pos == this->buffer->size())
      return pos;
   wrappedLineCounter(this->buffer, pos, this->buffer->size(), 1,
      startPosIsLineStart, 0, &retPos, &retLines, &retLineStart,
      &retLineEnd);
   return retLineEnd;
}

/*
** Same as BufStartOfLine, but returns the character after last wrap point
** rather than the last newline.
*/
int Ne_Text_Display::startOfLine(const int pos)
{
   int retLines, retPos, retLineStart, retLineEnd;

   /* If we're not wrapping, use the more efficient BufStartOfLine */
   if (!this->continuousWrap)
      return this->buffer->startOfLine(pos);

   wrappedLineCounter(this->buffer, this->buffer->startOfLine(pos),
      pos, INT_MAX, true, 0, &retPos, &retLines, &retLineStart,
      &retLineEnd);
   return retLineStart;
}

/*
** Same as BufCountBackwardNLines, but takes in to account line breaks when
** wrapping is turned on.
*/
int Ne_Text_Display::countBackwardNLines(int startPos, int nLines)
{
   Ne_Text_Buffer* buf = this->buffer;
   int pos, lineStart, retLines, retPos, retLineStart, retLineEnd;

   // If we're not wrapping, use the more efficient BufCountBackwardNLines
   if (!this->continuousWrap)
      return this->buffer->countBackwardNLines(startPos, nLines);

   pos = startPos;
   while (true)
   {
      lineStart = buf->startOfLine(pos);
      wrappedLineCounter(this->buffer, lineStart, pos, INT_MAX, true, 0, &retPos, &retLines, &retLineStart, &retLineEnd);
      if (retLines > nLines)
         return countForwardNLines(lineStart, retLines - nLines, true);
      nLines -= retLines;
      pos = lineStart - 1;
      if (pos < 0)
         return 0;
      nLines -= 1;
   }
}

/*
** In continuous wrap mode, internal line numbers are calculated after
** wrapping.  A separate non-wrapped line count is maintained when line
** numbering is turned on.  There is some performance cost to maintaining this
** line count, so normally absolute line numbers are not tracked if line
** numbering is off.  This routine allows callers to specify that they still
** want this line count maintained (for use via TextDPosToLineAndCol).
** More specifically, this allows the line number reported in the statistics
** line to be calibrated in absolute lines, rather than post-wrapped lines.
*/
void Ne_Text_Display::maintainAbsLineNum(int state)
{
   this->needAbsTopLineNum = state;
   resetAbsLineNum();
}

/*
** Returns the absolute (non-wrapped) line number of the first line displayed.
** Returns 0 if the absolute top line number is not being maintained.
*/
int Ne_Text_Display::getAbsTopLineNum()
{
   if (!this->continuousWrap)
      return this->topLineNum;
   if (maintainingAbsTopLineNum())
      return this->absTopLineNum;
   return 0;
}

/*
** Re-calculate absolute top line number for a change in scroll position.
*/
void Ne_Text_Display::offsetAbsLineNum(int oldFirstChar)
{
   if (maintainingAbsTopLineNum())
   {
      if (this->firstChar < oldFirstChar)
         this->absTopLineNum -= this->buffer->countLines(this->firstChar, oldFirstChar);
      else
         this->absTopLineNum += this->buffer->countLines(oldFirstChar, this->firstChar);
   }
}

/*
** Return true if a separate absolute top line number is being maintained
** (for displaying line numbers or showing in the statistics line).
*/
int Ne_Text_Display::maintainingAbsTopLineNum()
{
   return this->continuousWrap && (this->lineNumWidth != 0 || this->needAbsTopLineNum);
}

/*
** Count lines from the beginning of the buffer to reestablish the
** absolute (non-wrapped) top line number.  If mode is not continuous wrap,
** or the number is not being maintained, does nothing.
*/
void Ne_Text_Display::resetAbsLineNum()
{
   this->absTopLineNum = 1;
   offsetAbsLineNum(0);
}

/*
** Find the line number of position "pos" relative to the first line of
** displayed text. Returns false if the line is not displayed.
*/
bool Ne_Text_Display::posToVisibleLineNum(int pos, int* lineNum) const
{
   if (pos < this->firstChar)
      return false;
   if (pos > this->lastChar)
   {
      if (emptyLinesVisible())
      {
         if (this->lastChar < this->buffer->size())
         {
            if (!posToVisibleLineNum(this->lastChar, lineNum))
            {
               fprintf(stderr, "nedit: Consistency check ptvl failed\n");
               return false;
            }
            return ++(*lineNum) <= this->nVisibleLines - 1;
         }
         else
         {
            posToVisibleLineNum(std::max(this->lastChar - 1, 0), lineNum);
            return true;
         }
      }
      return false;
   }

   for (int i = this->nVisibleLines - 1; i >= 0; i--)
   {
      if (this->lineStarts[i] != -1 && pos >= this->lineStarts[i])
      {
         *lineNum = i;
         return true;
      }
   }

   return false;
}

/*
** Redisplay the text on a single line represented by "visLineNum" (the
** number of lines down from the top of the display), limited by
** "leftClip" and "rightClip" window coordinates and "leftCharIndex" and
** "rightCharIndex" character positions (not including the character at
** position "rightCharIndex").
**
** The cursor is also drawn if it appears on the line.
*/
void Ne_Text_Display::redisplayLine(int visLineNum, int leftClip, int rightClip, int leftCharIndex, int rightCharIndex)
{
   Ne_Text_Buffer* buf = this->buffer;
   int i, x, startX, charIndex, lineStartPos;
   int stdCharWidth, charWidth, startIndex, charStyle, style = 0;
   int charLen, outStartIndex, outIndex, cursorX = 0, hasCursor = false;
   int dispIndexOffset, cursorPos = this->cursorPos, y_orig;
   char expandedChar[MAX_EXP_CHAR_LEN];
   char outStr[MAX_DISP_LINE_LEN];
   char *outPtr;
   char baseChar;

   std::string lineStr;
   int lineLen = 0;

   // If line is not displayed, skip it
   if (visLineNum < 0 || visLineNum >= this->nVisibleLines)
      return;

   // Shrink the clipping range to the active display area
   leftClip = std::max(this->left, leftClip);
   rightClip = std::min(rightClip, this->left + this->width);

   if (leftClip > rightClip)
      return;

   // Calculate y coordinate of the string to draw
   int fontHeight = this->ascent + this->descent;
   int y = this->top + visLineNum * fontHeight;

   // Get the text, length, and  buffer position of the line to display
   lineStartPos = this->lineStarts[visLineNum];
   if (lineStartPos == -1)
   {
      lineLen = 0;
      lineStr = "";
   }
   else
   {
      lineLen = visLineLength(visLineNum);
      lineStr = buf->getRange(lineStartPos, lineStartPos + lineLen);
   }

   // Space beyond the end of the line is still counted in units of characters
   // of a standardized character width (this is done mostly because style
   // changes based on character position can still occur in this region due
   // to rectangular selections).  stdCharWidth must be non-zero to prevent a
   // potential infinite loop if x does not advance
   stdCharWidth = this->fontStruct.max_width();
   if (stdCharWidth <= 0)
   {
      fprintf(stderr, "nedit: Internal Error, bad font measurement\n");
      return;
   }

   dispIndexOffset = 0;

   // Step through character positions from the beginning of the line (even if
   // that's off the left edge of the displayed area) to find the first
   // character position that's not clipped, and the x coordinate for drawing
   // that character
   x = this->left - this->horizOffset;
   outIndex = 0;

   for (charIndex = 0; ; charIndex++)
   {
      baseChar = '\0';
      charLen = charIndex >= lineLen
         ? 1
         : buffer->expandCharacter(baseChar = lineStr[charIndex], outIndex, expandedChar, buf->getTabDistance(), buf->getNullSubsChar());
      style = styleOfPos(lineStartPos, lineLen, charIndex, outIndex + dispIndexOffset, baseChar);
      charWidth = charIndex >= lineLen
         ? stdCharWidth
         : stringWidth(expandedChar, charLen, style);

      if (x + charWidth >= leftClip && charIndex >= leftCharIndex)
      {
         startIndex = charIndex;
         outStartIndex = outIndex;
         startX = x;
         break;
      }
      x += charWidth;
      outIndex += charLen;
   }

   // Scan character positions from the beginning of the clipping range, and
   // draw parts whenever the style changes (also note if the cursor is on
   // this line, and where it should be drawn to take advantage of the x
   // position which we've gone to so much trouble to calculate)
   outPtr = outStr;
   outIndex = outStartIndex;
   x = startX;
   for (charIndex = startIndex; charIndex < rightCharIndex; charIndex++)
   {
      if (lineStartPos + charIndex == cursorPos)
      {
         if (charIndex < lineLen || (charIndex == lineLen && cursorPos >= buf->size()))
         {
            hasCursor = true;
            cursorX = x - 1;
         }
         else if (charIndex == lineLen)
         {
            if (wrapUsesCharacter(cursorPos))
            {
               hasCursor = true;
               cursorX = x - 1;
            }
         }
      }

      baseChar = '\0';
      charLen = charIndex >= lineLen
         ? 1
         : buffer->expandCharacter(baseChar = lineStr[charIndex], outIndex,
         expandedChar, buf->getTabDistance(), buf->getNullSubsChar());
      charStyle = styleOfPos(lineStartPos, lineLen, charIndex, outIndex + dispIndexOffset, baseChar);
      for (i = 0; i < charLen; i++)
      {
         if (i != 0 && charIndex < lineLen && lineStr[charIndex] == '\t')
         {
            charStyle = styleOfPos(lineStartPos, lineLen, charIndex, outIndex + dispIndexOffset, '\t');
         }

         if (charStyle != style)
         {
            drawString(style, startX, y, x, outStr, outPtr - outStr);
            outPtr = outStr;
            startX = x;
            style = charStyle;
         }

         if (charIndex < lineLen)
         {
            *outPtr = expandedChar[i];
            charWidth = stringWidth(&expandedChar[i], 1, charStyle);
         }
         else
         {
            charWidth = stdCharWidth;
         }

         outPtr++;
         x += charWidth;
         outIndex++;
      }

      if (outPtr - outStr + MAX_EXP_CHAR_LEN >= MAX_DISP_LINE_LEN  || x >= rightClip)
      {
         break;
      }
   }

   // Draw the remaining style segment
   drawString(style, startX, y, x, outStr, outPtr - outStr);

   // Draw the cursor if part of it appeared on the redisplayed part of
   // this line.  Also check for the cases which are not caught as the
   // line is scanned above: when the cursor appears at the very end
   // of the redisplayed section.
   y_orig = this->cursorY;

   if (this->cursorOn)
   {
      if (hasCursor)
      {
         drawCursor(cursorX, y);
      }
      else if (charIndex < lineLen
         && (lineStartPos + charIndex + 1 == cursorPos)
         && x == rightClip)
      {
         if (cursorPos >= buf->size())
         {
            drawCursor(x - 1, y);
         }
         else
         {
            if (wrapUsesCharacter(cursorPos))
            {
               drawCursor(x - 1, y);
            }
         }
      }
      else if ((lineStartPos + rightCharIndex) == cursorPos)
      {
         drawCursor(x - 1, y);
      }
   }

   // TODO:     /* If the y position of the cursor has changed, redraw the calltip */
   // TODO:     if (hasCursor && (y_orig != this->cursorY || y_orig != y))
   // TODO:        TextDRedrawCalltip(textD, 0);
}

/*
** Draw a string or blank area according to parameter "style", using the
** appropriate colors and drawing method for that style, with top left
** corner at x, y.  If style says to draw text, use "string" as source of
** characters, and draw "nChars", if style is FILL, erase
** rectangle where text would have drawn from x to toX and from y to
** the maximum y extent of the current font(s).
*/
void Ne_Text_Display::drawString(int style, int x, int y, int toX, const char* string, int nChars)
{
   Ne_Font& fs = this->fontStruct;
   Fl_Color bground = this->bgPixel;
   Fl_Color fground = this->fgPixel;
   bool underlineStyle = false;

   // Don't draw if widget isn't realized
   if (!canRedraw)
      return;

   // TODO:     /* select a GC */
   // TODO:     if (style & (STYLE_LOOKUP_MASK | BACKLIGHT_MASK | RANGESET_MASK))
   // TODO:     {
   // TODO:        gc = bgGC = this->styleGC;
   // TODO:     }
   // TODO:     else if (style & HIGHLIGHT_MASK)
   // TODO:     {
   // TODO:        gc = this->highlightGC;
   // TODO:        bgGC = this->highlightBGGC;
   // TODO:     }
   // TODO:     else if (style & PRIMARY_MASK)
   // TODO:     {
   // TODO:        gc = this->selectGC;
   // TODO:        bgGC = this->selectBGGC;
   // TODO:     }
   // TODO:     else
   // TODO:     {
   // TODO:        gc = bgGC = this->gc;
   // TODO:     }
   // TODO: 
   // TODO:     if (gc == this->styleGC)
   // TODO:     {
   // TODO:        /* we have work to do */
   // TODO:        styleTableEntry* styleRec;
   // TODO:        /* Set font, color, and gc depending on style.  For normal text, GCs
   // TODO:           for normal drawing, or drawing within a selection or highlight are
   // TODO:           pre-allocated and pre-configured.  For syntax highlighting, GCs are
   // TODO:           configured here, on the fly. */
   // TODO:        if (style & STYLE_LOOKUP_MASK)
   // TODO:        {
   // TODO:           styleRec = &this->styleTable[(style & STYLE_LOOKUP_MASK) - ASCII_A];
   // TODO:           underlineStyle = styleRec->underline;
   // TODO:           fs = styleRec->font;
   // TODO:           gcValues.font = fs->fid;
   // TODO:           fground = styleRec->color;
   // TODO:           /* here you could pick up specific select and highlight fground */
   // TODO:        }
   // TODO:        else
   // TODO:        {
   // TODO:           styleRec = NULL;
   // TODO:           gcValues.font = fs->fid;
   // TODO:           fground = this->fgPixel;
   // TODO:        }
   /* Background color priority order is:
   1 Primary(Selection), 2 Highlight(Parens),
   3 NeRangeset, 4 SyntaxHighlightStyle,
   5 Backlight (if NOT fill), 6 DefaultBackground */
   bground =
      style & PRIMARY_MASK   ? this->selectBGPixel :
      style & HIGHLIGHT_MASK ? this->highlightBGPixel :
      style & RANGESET_MASK  ? getRangesetColor((style & RANGESET_MASK) >> RANGESET_SHIFT, bground) :
       this->bgPixel; // to remove
   // TODO:           styleRec && styleRec->bgColorName ? styleRec->bgColor :
   // TODO:           (style & BACKLIGHT_MASK) && !(style & FILL_MASK) ? this->bgClassPixel[(style >> BACKLIGHT_SHIFT) & 0xff] :
   // TODO:           this->bgPixel;
   if (fground == bground) /* B&W kludge */
      fground = fl_contrast(bground, bground);
   // TODO:        /* set up gc for clearing using the foreground color entry */
   // TODO:        gcValues.foreground = gcValues.background = bground;
   // TODO:        XChangeGC(XtDisplay(this->w), gc, GCFont | GCForeground | GCBackground, &gcValues);
   // TODO:     }

   // Draw blank area rather than text, if that was the request
   if (style & FILL_MASK)
   {
      // wipes out to right hand edge of widget
      if (toX >= this->left)
         clearRect(bground, std::max(x, this->left), y, toX - std::max(x, this->left), this->ascent + this->descent);

      return;
   }

   // TODO:     /* If any space around the character remains unfilled (due to use of
   // TODO:        different sized fonts for highlighting), fill in above or below
   // TODO:        to erase previously drawn characters */
   // TODO:     if (fs->ascent < this->ascent)
   // TODO:        clearRect(textD, bgGC, x, y, toX - x, this->ascent - fs->ascent);
   // TODO:     if (fs->descent < this->descent)
   // TODO:        clearRect(textD, bgGC, x, y + this->ascent + fs->descent, toX - x, this->descent - fs->descent);
   // TODO: 
   // TODO:     /* set up gc for writing text (set foreground properly) */
   // TODO:     if (bgGC == this->styleGC)
   // TODO:     {
   // TODO:        gcValues.foreground = fground;
   // TODO:        XChangeGC(XtDisplay(this->w), gc, GCForeground, &gcValues);
   // TODO:     }

   // Draw the string using gc and font set above
   fl_rectf(x, y, (int)fl_width(string, nChars), this->ascent + this->descent, bground);
   fl_color(fground);
   fl_draw(string, nChars, x, y + this->ascent);

   // Underline if style is secondary selection
   if (style & SECONDARY_MASK || underlineStyle)
   {
      // restore foreground in GC (was set to background by clearRect())
      fl_color(fground);

      // draw underline
      fl_line(x, y + this->ascent, toX - 1, y + this->ascent);
   }
}

/*
** Clear a rectangle with the appropriate background color for "style"
*/
void Ne_Text_Display::clearRect(Fl_Color color, int x, int y, int width, int height)
{
   // A width of zero means "clear to end of window" to XClearArea ???
   if (width == 0 || !canRedraw)
      return;

   fl_rectf(x, y, width, height, color);
}

/*
** Draw a cursor with top center at x, y.
*/
void Ne_Text_Display::drawCursor(int x, int y)
{
   int left, right, cursorWidth;
   int fontWidth = (this->fontStruct.max_width()+this->fontStruct.min_width())/2;
   int fontHeight = this->ascent + this->descent;
   int bot = y + fontHeight - 1;

   if (!canRedraw || x < this->left - 1 || x > this->left + this->width)
      return;

   // For cursors other than the block, make them around 2/3 of a character
   // width, rounded to an even number of pixels so that X will draw an
   // odd number centered on the stem at x.
   cursorWidth = (fontWidth / 3) * 2;
   left = x - cursorWidth / 2;
   right = left + cursorWidth;

   fl_color(this->cursorFGPixel);

   fl_push_no_clip();
   // Create segments and draw cursor
   if (this->cursorStyle == NE_CARET_CURSOR)
   {
      // TODO:        int midY = bot - fontHeight / 5;
      // TODO:        segs[0].x1 = left;
      // TODO:        segs[0].y1 = bot;
      // TODO:        segs[0].x2 = x;
      // TODO:        segs[0].y2 = midY;
      // TODO:        segs[1].x1 = x;
      // TODO:        segs[1].y1 = midY;
      // TODO:        segs[1].x2 = right;
      // TODO:        segs[1].y2 = bot;
      // TODO:        segs[2].x1 = left;
      // TODO:        segs[2].y1 = bot;
      // TODO:        segs[2].x2 = x;
      // TODO:        segs[2].y2 = midY - 1;
      // TODO:        segs[3].x1 = x;
      // TODO:        segs[3].y1 = midY - 1;
      // TODO:        segs[3].x2 = right;
      // TODO:        segs[3].y2 = bot;
      // TODO:        nSegs = 4;
   }
   else if (this->cursorStyle == NE_NORMAL_CURSOR)
   {
      fl_color(this->cursorFGPixel);
      fl_line(left, y, right, y);
      fl_line(x, y, x, bot);
      fl_line(left, bot, right, bot);
   }
   else if (this->cursorStyle == NE_HEAVY_CURSOR)
   {
      fl_color(this->cursorFGPixel);
      fl_rect(x-1, y, 2, fontHeight);
      fl_rect(left, y, cursorWidth, 2);
      fl_rect(left, bot-1, cursorWidth, 2);
   }
   // TODO:     else if (this->cursorStyle == DIM_CURSOR)
   // TODO:     {
   // TODO:        int midY = y + fontHeight / 2;
   // TODO:        segs[0].x1 = x;
   // TODO:        segs[0].y1 = y;
   // TODO:        segs[0].x2 = x;
   // TODO:        segs[0].y2 = y;
   // TODO:        segs[1].x1 = x;
   // TODO:        segs[1].y1 = midY;
   // TODO:        segs[1].x2 = x;
   // TODO:        segs[1].y2 = midY;
   // TODO:        segs[2].x1 = x;
   // TODO:        segs[2].y1 = bot;
   // TODO:        segs[2].x2 = x;
   // TODO:        segs[2].y2 = bot;
   // TODO:        nSegs = 3;
   // TODO:     }
   else if (this->cursorStyle == NE_BLOCK_CURSOR)
   {
      right = x + fontWidth;
      fl_rect(x, y, right - left, fontHeight);
   }
   fl_pop_clip();

   // Save the last position drawn
   this->cursorX = x;
   this->cursorY = y;
}

/*
** Determine the drawing method to use to draw a specific character from "buf".
** "lineStartPos" gives the character index where the line begins, "lineIndex",
** the number of characters past the beginning of the line, and "dispIndex",
** the number of displayed characters past the beginning of the line.  Passing
** lineStartPos of -1 returns the drawing style for "no text".
**
** Why not just: styleOfPos(textD, pos)?  Because style applies to blank areas
** of the window beyond the text boundaries, and because this routine must also
** decide whether a position is inside of a rectangular selection, and do so
** efficiently, without re-counting character positions from the start of the
** line.
**
** Note that style is a somewhat incorrect name, drawing method would
** be more appropriate.
*/
int Ne_Text_Display::styleOfPos(int lineStartPos, int lineLen, int lineIndex, int dispIndex, int thisChar)
{
   Ne_Text_Buffer* buf = this->buffer;
   Ne_Text_Buffer* styleBuf = this->styleBuffer;
   int style = 0;

   if (lineStartPos == -1 || buf == NULL)
      return FILL_MASK;

   int pos = lineStartPos + std::min(lineIndex, lineLen);

   if (lineIndex >= lineLen)
      style = FILL_MASK;
   else if (styleBuf != NULL)
   {
      style = (unsigned char)styleBuf->getCharacter(pos);
      if (style == this->unfinishedStyle)
      {
         // encountered "unfinished" style, trigger parsing
         (*this->unfinishedHighlightCB)(this, pos, this->highlightCBArg);
         style = (unsigned char)styleBuf->getCharacter(pos);
      }
   }
   if (inSelection(&buf->getSelection(), pos, lineStartPos, dispIndex))
      style |= PRIMARY_MASK;
   if (inSelection(&buf->getHighlightSelection(), pos, lineStartPos, dispIndex))
      style |= HIGHLIGHT_MASK;
   if (inSelection(&buf->getSecSelection(), pos, lineStartPos, dispIndex))
      style |= SECONDARY_MASK;
    // store in the RANGESET_MASK portion of style the rangeset index for pos
    if (buf->rangesetTable)
    {
       int rangesetIndex = RangesetIndex1ofPos(buf->rangesetTable, pos, true);
       style |= ((rangesetIndex << RANGESET_SHIFT) & RANGESET_MASK);
    }
    // store in the BACKLIGHT_MASK portion of style the background color class of the character thisChar
    if (this->bgClass)
    {
       style |= (this->bgClass[(unsigned char)thisChar] << BACKLIGHT_SHIFT);
    }
   return style;
}

/*
** Find the width of a string in the font of a particular style
*/
int Ne_Text_Display::stringWidth(const char* string, const int length, const int style) const
{
   const Ne_Font* fs;

   if (style & STYLE_LOOKUP_MASK)
      fs = &this->styleTable[(style & STYLE_LOOKUP_MASK) - 'A'].font;
   else
      fs = &this->fontStruct;
   fl_font(fs->font, fs->size);
   return (int)fl_width(string, length);
}

/*
** Return true if position "pos" with indentation "dispIndex" is in selection "sel"
*/
bool Ne_Text_Display::inSelection(Ne_Text_Selection* sel, int pos, int lineStartPos, int dispIndex) const
{
   return sel->selected && (pos >= sel->start && pos < sel->end);
}

/*
** Translate window coordinates to the nearest (insert cursor or character
** cell) text position.  The parameter posType specifies how to interpret the
** position: CURSOR_POS means translate the coordinates to the nearest cursor
** position, and CHARACTER_POS means return the position of the character
** closest to (x, y).
*/
int Ne_Text_Display::xyToPos(int x, int y, NePositionTypes posType)
{
   int charIndex, lineStart, fontHeight;
   int charWidth, charLen, charStyle, visLineNum, xStep, outIndex;
   char expandedChar[MAX_EXP_CHAR_LEN];

   // Find the visible line number corresponding to the y coordinate
   fontHeight = this->ascent + this->descent;
   visLineNum = (y - this->top) / fontHeight;
   if (visLineNum < 0)
      return this->firstChar;
   if (visLineNum >= this->nVisibleLines)
      visLineNum = this->nVisibleLines - 1;

   // Find the position at the start of the line
   lineStart = this->lineStarts[visLineNum];

   // If the line start was empty, return the last position in the buffer
   if (lineStart == -1)
      return this->buffer->size();

   // Get the line text and its length
   int lineLen = visLineLength(visLineNum);
   std::string lineStr = this->buffer->getRange(lineStart, lineStart + lineLen);

   // Step through character positions from the beginning of the line
   // to find the character position corresponding to the x coordinate
   xStep = this->left - this->horizOffset;
   outIndex = 0;
   for (charIndex = 0; charIndex < lineLen; charIndex++)
   {
      charLen = buffer->expandCharacter(lineStr[charIndex], outIndex, expandedChar, this->buffer->getTabDistance(), this->buffer->getNullSubsChar());
      charStyle = styleOfPos(lineStart, lineLen, charIndex, outIndex, lineStr[charIndex]);
      charWidth = stringWidth(expandedChar, charLen, charStyle);
      if (x < xStep + (posType == NE_CURSOR_POS ? charWidth / 2 : charWidth))
         return lineStart + charIndex;

      xStep += charWidth;
      outIndex += charLen;
   }

   // If the x position was beyond the end of the line, return the position
   // of the newline at the end of the line
   return lineStart + lineLen;
}

/*
** Translate window coordinates to the nearest row and column number for
** positioning the cursor.  This, of course, makes no sense when the font is
** proportional, since there are no absolute columns.  The parameter posType
** specifies how to interpret the position: CURSOR_POS means translate the
** coordinates to the nearest position between characters, and CHARACTER_POS
** means translate the position to the nearest character cell.
*/
void Ne_Text_Display::xyToUnconstrainedPos(int x, int y, int* row, int* column, NePositionTypes posType)
{
   int fontHeight = this->ascent + this->descent;
   int fontWidth = this->fontStruct.max_width();

   /* Find the visible line number corresponding to the y coordinate */
   *row = (y - this->top) / fontHeight;
   if (*row < 0) *row = 0;
   if (*row >= this->nVisibleLines) *row = this->nVisibleLines - 1;
   *column = ((x - this->left) + this->horizOffset + (posType == NE_CURSOR_POS ? fontWidth / 2 : 0)) / fontWidth;
   if (*column < 0) *column = 0;
}

/*
** Offset the line starts array, topLineNum, firstChar and lastChar, for a new
** vertical scroll position given by newTopLineNum.  If any currently displayed
** lines will still be visible, salvage the line starts values, otherwise,
** count lines from the nearest known line start (start or end of buffer, or
** the closest value in the lineStarts array)
*/
void Ne_Text_Display::offsetLineStarts(int newTopLineNum)
{
   int oldTopLineNum = this->topLineNum;
   int oldFirstChar = this->firstChar;
   int lineDelta = newTopLineNum - oldTopLineNum;
   int nVisLines = this->nVisibleLines;
   int* lineStarts = this->lineStarts;
   int i, lastLineNum;
   Ne_Text_Buffer* buf = this->buffer;

   /* If there was no offset, nothing needs to be changed */
   if (lineDelta == 0)
      return;

   /* {   int i;
   printf("Scroll, lineDelta %d\n", lineDelta);
   printf("lineStarts Before: ");
   for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
   printf("\n");
   } */

   /* Find the new value for firstChar by counting lines from the nearest
   known line start (start or end of buffer, or the closest value in the
   lineStarts array) */
   lastLineNum = oldTopLineNum + nVisLines - 1;
   if (newTopLineNum < oldTopLineNum && newTopLineNum < -lineDelta)
   {
      this->firstChar = countForwardNLines(0, newTopLineNum - 1, true);
      /* printf("counting forward %d lines from start\n", newTopLineNum-1);*/
   }
   else if (newTopLineNum < oldTopLineNum)
   {
      this->firstChar = countBackwardNLines(this->firstChar, -lineDelta);
      /* printf("counting backward %d lines from firstChar\n", -lineDelta);*/
   }
   else if (newTopLineNum < lastLineNum)
   {
      this->firstChar = lineStarts[newTopLineNum - oldTopLineNum];
      /* printf("taking new start from lineStarts[%d]\n", newTopLineNum - oldTopLineNum); */
   }
   else if (newTopLineNum - lastLineNum < this->nBufferLines - newTopLineNum)
   {
      this->firstChar = countForwardNLines( lineStarts[nVisLines - 1], newTopLineNum - lastLineNum, true);
      /* printf("counting forward %d lines from start of last line\n", newTopLineNum - lastLineNum); */
   }
   else
   {
      this->firstChar = countBackwardNLines(buf->size(), this->nBufferLines - newTopLineNum + 1);
      /* printf("counting backward %d lines from end\n", this->nBufferLines - newTopLineNum + 1); */
   }

   /* Fill in the line starts array */
   if (lineDelta < 0 && -lineDelta < nVisLines)
   {
      for (i = nVisLines - 1; i >= -lineDelta; i--)
         lineStarts[i] = lineStarts[i + lineDelta];
      calcLineStarts(0, -lineDelta);
   }
   else if (lineDelta > 0 && lineDelta < nVisLines)
   {
      for (i = 0; i < nVisLines - lineDelta; i++)
         lineStarts[i] = lineStarts[i + lineDelta];
      calcLineStarts(nVisLines - lineDelta, nVisLines - 1);
   }
   else
      calcLineStarts(0, nVisLines);

   // Set lastChar and topLineNum
   calcLastChar();
   this->topLineNum = newTopLineNum;

   // If we're numbering lines or being asked to maintain an absolute line
   // number, re-calculate the absolute line number
   offsetAbsLineNum(oldFirstChar);

   /* {   int i;
   printf("lineStarts After: ");
   for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
   printf("\n");
   } */
}

/*
** Update the line starts array, topLineNum, firstChar and lastChar for text
** display "textD" after a modification to the text buffer, given by the
** position where the change began "pos", and the nmubers of characters
** and lines inserted and deleted.
*/
void Ne_Text_Display::updateLineStarts(int pos, int charsInserted, int charsDeleted, int linesInserted, int linesDeleted, bool* scrolled)
{
   int* lineStarts = this->lineStarts;
   int i, lineOfPos, lineOfEnd, nVisLines = this->nVisibleLines;
   int charDelta = charsInserted - charsDeleted;
   int lineDelta = linesInserted - linesDeleted;

   /* {   int i;
   printf("linesDeleted %d, linesInserted %d, charsInserted %d, charsDeleted %d\n",
   linesDeleted, linesInserted, charsInserted, charsDeleted);
   printf("lineStarts Before: ");
   for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
   printf("\n");
   } */
   /* If all of the changes were before the displayed text, the display
   doesn't change, just update the top line num and offset the line
   start entries and first and last characters */
   if (pos + charsDeleted < this->firstChar)
   {
      this->topLineNum += lineDelta;
      for (i = 0; i < nVisLines && lineStarts[i] != -1; i++)
         lineStarts[i] += charDelta;
      /* {   int i;
      printf("lineStarts after delete doesn't touch: ");
      for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
      printf("\n");
      } */
      this->firstChar += charDelta;
      this->lastChar += charDelta;
      *scrolled = false;
      return;
   }

   /* The change began before the beginning of the displayed text, but
   part or all of the displayed text was deleted */
   if (pos < this->firstChar)
   {
      /* If some text remains in the window, anchor on that  */
      if (posToVisibleLineNum(pos + charsDeleted, &lineOfEnd)
         && ++lineOfEnd < nVisLines && lineStarts[lineOfEnd] != -1)
      {
         this->topLineNum = std::max(1, this->topLineNum + lineDelta);
         this->firstChar = countBackwardNLines(lineStarts[lineOfEnd] + charDelta, lineOfEnd);
         /* Otherwise anchor on original line number and recount everything */
      }
      else
      {
         if (this->topLineNum > this->nBufferLines + lineDelta)
         {
            this->topLineNum = 1;
            this->firstChar = 0;
         }
         else
            this->firstChar = countForwardNLines(0, this->topLineNum - 1, true);
      }
      calcLineStarts(0, nVisLines - 1);
      /* {   int i;
      printf("lineStarts after delete encroaches: ");
      for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
      printf("\n");
      } */
      /* calculate lastChar by finding the end of the last displayed line */
      calcLastChar();
      *scrolled = true;
      return;
   }

   /* If the change was in the middle of the displayed text (it usually is),
   salvage as much of the line starts array as possible by moving and
   offsetting the entries after the changed area, and re-counting the
   added lines or the lines beyond the salvaged part of the line starts
   array */
   if (pos <= this->lastChar)
   {
      /* find line on which the change began */
      posToVisibleLineNum(pos, &lineOfPos);
      /* salvage line starts after the changed area */
      if (lineDelta == 0)
      {
         for (i = lineOfPos + 1; i < nVisLines && lineStarts[i] != -1; i++)
            lineStarts[i] += charDelta;
      }
      else if (lineDelta > 0)
      {
         for (i = nVisLines - 1; i >= lineOfPos + lineDelta + 1; i--)
            lineStarts[i] = lineStarts[i - lineDelta] +
            (lineStarts[i - lineDelta] == -1 ? 0 : charDelta);
      }
      else /* (lineDelta < 0) */
      {
         for (i = std::max(0, lineOfPos + 1); i < nVisLines + lineDelta; i++)
            lineStarts[i] = lineStarts[i - lineDelta] +
            (lineStarts[i - lineDelta] == -1 ? 0 : charDelta);
      }
      /* {   int i;
      printf("lineStarts after salvage: ");
      for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
      printf("\n");
      } */
      /* fill in the missing line starts */
      if (linesInserted >= 0)
         calcLineStarts(lineOfPos + 1, lineOfPos + linesInserted);
      if (lineDelta < 0)
         calcLineStarts(nVisLines + lineDelta, nVisLines);
      /* {   int i;
      printf("lineStarts after recalculation: ");
      for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
      printf("\n");
      } */
      /* calculate lastChar by finding the end of the last displayed line */
      calcLastChar();
      *scrolled = false;
      return;
   }

   /* Change was past the end of the displayed text, but displayable by virtue
   of being an insert at the end of the buffer into visible blank lines */
   if (emptyLinesVisible())
   {
      posToVisibleLineNum(pos, &lineOfPos);
      calcLineStarts(lineOfPos, lineOfPos + linesInserted);
      calcLastChar();
      /* {   int i;
      printf("lineStarts after insert at end: ");
      for(i=0; i<nVisLines; i++) printf("%d ", lineStarts[i]);
      printf("\n");
      } */
      *scrolled = false;
      return;
   }

   /* Change was beyond the end of the buffer and not visible, do nothing */
   *scrolled = false;
}

/*
** Scan through the text in the "textD"'s buffer and recalculate the line
** starts array values beginning at index "startLine" and continuing through
** (including) "endLine".  It assumes that the line starts entry preceding
** "startLine" (or this->firstChar if startLine is 0) is good, and re-counts
** newlines to fill in the requested entries.  Out of range values for
** "startLine" and "endLine" are acceptable.
*/
void Ne_Text_Display::calcLineStarts(int startLine, int endLine)
{
   int startPos, bufLen = this->buffer->size();
   int line, lineEnd, nextLineStart, nVis = this->nVisibleLines;
   int* lineStarts = this->lineStarts;

   /* Clean up (possibly) messy input parameters */
   if (nVis == 0) return;
   if (endLine < 0) endLine = 0;
   if (endLine >= nVis) endLine = nVis - 1;
   if (startLine < 0) startLine = 0;
   if (startLine >= nVis) startLine = nVis - 1;
   if (startLine > endLine)
      return;

   /* Find the last known good line number -> position mapping */
   if (startLine == 0)
   {
      lineStarts[0] = this->firstChar;
      startLine = 1;
   }
   startPos = lineStarts[startLine - 1];

   /* If the starting position is already past the end of the text,
   fill in -1's (means no text on line) and return */
   if (startPos == -1)
   {
      for (line = startLine; line <= endLine; line++)
         lineStarts[line] = -1;
      return;
   }

   /* Loop searching for ends of lines and storing the positions of the
   start of the next line in lineStarts */
   for (line = startLine; line <= endLine; line++)
   {
      findLineEnd(startPos, true, &lineEnd, &nextLineStart);
      startPos = nextLineStart;
      if (startPos >= bufLen)
      {
         /* If the buffer ends with a newline or line break, put
         buf->length in the next line start position (instead of
         a -1 which is the normal marker for an empty line) to
         indicate that the cursor may safely be displayed there */
         if (line == 0 || (lineStarts[line - 1] != bufLen &&
            lineEnd != nextLineStart))
         {
            lineStarts[line] = bufLen;
            line++;
         }
         break;
      }
      lineStarts[line] = startPos;
   }

   /* Set any entries beyond the end of the text to -1 */
   for (; line <= endLine; line++)
      lineStarts[line] = -1;
}

/*
** Given a Ne_Text_Display with a complete, up-to-date lineStarts array, update
** the lastChar entry to point to the last buffer position displayed.
*/
void Ne_Text_Display::calcLastChar()
{
   int i;
   for (i = this->nVisibleLines - 1; i > 0 && this->lineStarts[i] == -1; i--);
   this->lastChar = i < 0 ? 0 : endOfLine(this->lineStarts[i], true);
}

void Ne_Text_Display::setScroll(int topLineNum, int horizOffset, bool updateVScrollBar, bool updateHScrollBar)
{
   int fontHeight = this->ascent + this->descent;
   int origHOffset = this->horizOffset;
   int lineDelta = this->topLineNum - topLineNum;
   int exactHeight = this->height - this->height % (this->ascent + this->descent);

   // Do nothing if scroll position hasn't actually changed or there's no window to draw in yet
   if (this->horizOffset == horizOffset && this->topLineNum == topLineNum)
      return;

   // If part of the cursor is protruding beyond the text clipping region, clear it off
   blankCursorProtrusions();

   // If the vertical scroll position has changed, update the line starts array and related counters in the text display
   offsetLineStarts(topLineNum);

   // Just setting this->horizOffset is enough information for redisplay
   this->horizOffset = horizOffset;

   // Update the scroll bar positions if requested, note: updating the
   // horizontal scroll bars can have the further side-effect of changing
   // the horizontal scroll position, this->horizOffset
   if (updateVScrollBar && this->vScrollBar != NULL)
   {
      updateVScrollBarRange();
   }
   if (updateHScrollBar && this->hScrollBar != NULL)
   {
      updateHScrollBarRange();
   }

   redraw();
}

/*
** Update the minimum, maximum, slider size, page increment, and value
** for vertical scroll bar.
*/
void Ne_Text_Display::updateVScrollBarRange()
{
   if (this->vScrollBar == NULL)
      return;

   // The Vert. scroll bar value and slider size directly represent the top
   // line number, and the number of visible lines respectively.  The scroll
   // bar maximum value is chosen to generally represent the size of the whole
   // buffer, with minor adjustments to keep the scroll bar widget happy
   int sliderSize = std::max(this->nVisibleLines, 1); /* Avoid X warning (size < 1) */
   int sliderValue = this->topLineNum;
   int sliderMax = std::max(this->nBufferLines + 2 + this->cursorVPadding, sliderSize); // ??? + sliderValue);
   this->vScrollBar->value(sliderValue, sliderSize, 1, sliderMax);
}

/*
** Update the minimum, maximum, slider size, page increment, and value
** for the horizontal scroll bar.  If scroll position is such that there
** is blank space to the right of all lines of text, scroll back (adjust
** horizOffset but don't redraw) to take up the slack and position the
** right edge of the text at the right edge of the display.
**
** Note, there is some cost to this routine, since it scans the whole range
** of displayed text, particularly since it's usually called for each typed
** character!
*/
bool Ne_Text_Display::updateHScrollBarRange()
{
   int origHOffset = this->horizOffset;

   if (this->hScrollBar == NULL || !this->hScrollBar->visible())
      return false;

   // Scan all the displayed lines to find the width of the longest line
   int maxWidth = 0;
   for (int i = 0; i < this->nVisibleLines && this->lineStarts[i] != -1; i++)
      maxWidth = std::max(measureVisLine(i), maxWidth);

   // If the scroll position is beyond what's necessary to keep all lines
   // in view, scroll to the left to bring the end of the longest line to
   // the right margin
   if (maxWidth < this->width + this->horizOffset && this->horizOffset > 0)
      this->horizOffset = std::max(0, maxWidth - this->width);

   // Readjust the scroll bar
   int sliderWidth = this->width;
   int sliderMax = std::max(maxWidth, sliderWidth + this->horizOffset);

   this->hScrollBar->value(this->horizOffset, sliderWidth, 1, sliderMax);

   // Return true if scroll position was changed
   return origHOffset != this->horizOffset;
}

/*
** Define area for drawing line numbers.  A width of 0 disables line
** number drawing.
*/
void Ne_Text_Display::setLineNumberArea(int lineNumLeft, int lineNumWidth, int textLeft)
{
   int newWidth = this->width + this->left - textLeft;
   this->lineNumLeft = lineNumLeft;
   this->lineNumWidth = lineNumWidth;
   this->left = textLeft;

   resetAbsLineNum();
   resize(x(), y(), newWidth, this->height);
   redraw();
}

/*
** Refresh the line number area.  If clearAll is false, writes only over
** the character cell areas.  Setting clearAll to true will clear out any
** stray marks outside of the character cell area, which might have been
** left from before a resize or font change.
*/
void Ne_Text_Display::redrawLineNumbers(int clearAll)
{
   // Don't draw if lineNumWidth == 0 (line numbers are hidden), or widget is not yet realized
   if (this->lineNumWidth == 0 || !canRedraw)
      return;

   fl_push_clip(lineNumLeft, top, lineNumWidth, height);

   // Erase the previous contents of the line number area, if requested
   if (clearAll)
      fl_rectf(this->lineNumLeft, this->top, this->lineNumWidth, this->height, color());

   int lineHeight = fontStruct.height();
   int charWidth = fontStruct.max_width();

   // Draw the line numbers, aligned to the text
   int nCols = std::min(11, this->lineNumWidth / charWidth);
   int y = this->top;
   int line = getAbsTopLineNum();
   for (int visLine = 0; visLine < this->nVisibleLines; visLine++)
   {
      int lineStart = this->lineStarts[visLine];
      if (lineStart != -1 && (lineStart == 0 || this->buffer->getCharacter(lineStart - 1) == '\n'))
      {
         char lineNumString[12];
         sprintf(lineNumString, "%d", line);
         int stringWidth = (int)fl_width(lineNumString);
         fl_color(this->lineNumFGPixel);
         //fl_draw(lineNumString, this->lineNumLeft, y + this->ascent);
         fl_draw(lineNumString, this->lineNumLeft + this->lineNumWidth - this->marginWidth - stringWidth, y + this->ascent);
         line++;
      }
      else
      {
         fl_rectf(this->lineNumLeft, y, this->lineNumWidth, this->ascent + this->descent, this->bgPixel);
         if (visLine == 0)
            line++;
      }
      y += lineHeight;
   }

   fl_pop_clip();
}

/*
** Callbacks for drag or valueChanged on scroll bars
*/
void Ne_Text_Display::vScrollCB()
{
   int newValue = vScrollBar->value();
   int lineDelta = newValue - this->topLineNum;

   if (lineDelta == 0)
      return;
   setScroll(newValue, this->horizOffset, false, true);
}
void Ne_Text_Display::hScrollCB()
{
   int newValue = hScrollBar->value();
   if (newValue == this->horizOffset)
      return;
   setScroll(this->topLineNum, newValue, false, false);
}

/*
** Count the number of newlines in a null-terminated text string;
*/
int Ne_Text_Display::countLines(const char* string)
{
   const char* c;
   int lineCount = 0;

   if (string == NULL)
      return 0;
   for (c = string; *c != '\0'; c++)
      if (*c == '\n') lineCount++;
   return lineCount;
}

/*
** Return the width in pixels of the displayed line pointed to by "visLineNum"
*/
int Ne_Text_Display::measureVisLine(int visLineNum)
{
   int i, width = 0, len, style, lineLen = visLineLength(visLineNum);
   int charCount = 0, lineStartPos = this->lineStarts[visLineNum];
   char expandedChar[MAX_EXP_CHAR_LEN];

   if (this->styleBuffer == NULL)
   {
      for (i = 0; i < lineLen; i++)
      {
         len = this->buffer->getExpandedChar(lineStartPos + i, charCount, expandedChar);
         fl_font(this->fontStruct.font, this->fontStruct.size);
         width += (int)fl_width(expandedChar, len);
         charCount += len;
      }
   }
   else
   {
      for (i = 0; i < lineLen; i++)
      {
         len = this->buffer->getExpandedChar(lineStartPos + i, charCount, expandedChar);
         style = (unsigned char)this->styleBuffer->getCharacter(lineStartPos + i) - 'A';
         fl_font(this->styleTable[style].font.font, this->styleTable[style].font.size);
         width += (int)fl_width(expandedChar, len);
         charCount += len;
      }
   }
   return width;
}

/*
** Return true if there are lines visible with no corresponding buffer text
*/
bool Ne_Text_Display::emptyLinesVisible() const
{
   return this->nVisibleLines > 0 && this->lineStarts[this->nVisibleLines - 1] == -1;
}

 /*
 ** When the cursor is at the left or right edge of the text, part of it
 ** sticks off into the clipped region beyond the text.  Normal redrawing
 ** can not overwrite this protruding part of the cursor, so it must be
 ** erased independently by calling this routine.
 */
 void Ne_Text_Display::blankCursorProtrusions()
 {
    int x, width, cursorX = this->cursorX, cursorY = this->cursorY;
    int fontWidth = this->fontStruct.max_width();
    int fontHeight = this->ascent + this->descent;
    int cursorWidth, left = this->left, right = left + this->width;

    cursorWidth = (fontWidth / 3) * 2;
    if (cursorX >= left - 1 && cursorX <= left + cursorWidth / 2 - 1)
    {
       x = cursorX - cursorWidth / 2;
       width = left - x;
    }
    else if (cursorX >= right - cursorWidth / 2 && cursorX <= right)
    {
       x = right;
       width = cursorX + cursorWidth / 2 + 2 - right;
    }
    else
       return;

    fl_color(this->bgPixel);
    fl_rectf(x, cursorY, width, fontHeight);
 }

/*
** Return the length of a line (number of displayable characters) by examining
** entries in the line starts array rather than by scanning for newlines
*/
int Ne_Text_Display::visLineLength(int visLineNum)
{
   int nextLineStart, lineStartPos = this->lineStarts[visLineNum];

   if (lineStartPos == -1)
      return 0;
   if (visLineNum + 1 >= this->nVisibleLines)
      return this->lastChar - lineStartPos;
   nextLineStart = this->lineStarts[visLineNum + 1];
   if (nextLineStart == -1)
      return this->lastChar - lineStartPos;
   if (wrapUsesCharacter(nextLineStart - 1))
      return nextLineStart - 1 - lineStartPos;
   return nextLineStart - lineStartPos;
}

 /*
 ** When continuous wrap is on, and the user inserts or deletes characters,
 ** wrapping can happen before and beyond the changed position.  This routine
 ** finds the extent of the changes, and counts the deleted and inserted lines
 ** over that range.  It also attempts to minimize the size of the range to
 ** what has to be counted and re-displayed, so the results can be useful
 ** both for delimiting where the line starts need to be recalculated, and
 ** for deciding what part of the text to redisplay.
 */
 void Ne_Text_Display::findWrapRange(const char* deletedText, int pos,
                           int nInserted, int nDeleted, int* modRangeStart, int* modRangeEnd,
                           int* linesInserted, int* linesDeleted)
 {
    int length, retPos, retLines, retLineStart, retLineEnd;
    Ne_Text_Buffer* deletedTextBuf, *buf = this->buffer;
    int nVisLines = this->nVisibleLines;
    int* lineStarts = this->lineStarts;
    int countFrom, countTo, lineStart, adjLineStart, i;
    int visLineNum = 0, nLines = 0;

    /*
    ** Determine where to begin searching: either the previous newline, or
    ** if possible, limit to the start of the (original) previous displayed
    ** line, using information from the existing line starts array
    */
    if (pos >= this->firstChar && pos <= this->lastChar)
    {
       for (i = nVisLines - 1; i > 0; i--)
          if (lineStarts[i] != -1 && pos >= lineStarts[i])
             break;
       if (i > 0)
       {
          countFrom = lineStarts[i - 1];
          visLineNum = i - 1;
       }
       else
          countFrom = buf->startOfLine(pos);
    }
    else
       countFrom = buf->startOfLine(pos);


    /*
    ** Move forward through the (new) text one line at a time, counting
    ** displayed lines, and looking for either a real newline, or for the
    ** line starts to re-sync with the original line starts array
    */
    lineStart = countFrom;
    *modRangeStart = countFrom;
    while (true)
    {

       /* advance to the next line.  If the line ended in a real newline
          or the end of the buffer, that's far enough */
       wrappedLineCounter(buf, lineStart, buf->length(), 1, true, 0,
                          &retPos, &retLines, &retLineStart, &retLineEnd);
       if (retPos >= buf->length())
       {
          countTo = buf->length();
          *modRangeEnd = countTo;
          if (retPos != retLineEnd)
             nLines++;
          break;
       }
       else
          lineStart = retPos;
       nLines++;
       if (lineStart > pos + nInserted &&
             buf->getCharacter(lineStart - 1) == '\n')
       {
          countTo = lineStart;
          *modRangeEnd = lineStart;
          break;
       }

       /* Don't try to resync in continuous wrap mode with non-fixed font
          sizes; it would result in a chicken-and-egg dependency between
          the calculations for the inserted and the deleted lines.
               If we're in that mode, the number of deleted lines is calculated in
               advance, without resynchronization, so we shouldn't resynchronize
               for the inserted lines either. */
       if (this->suppressResync)
          continue;

       /* check for synchronization with the original line starts array
          before pos, if so, the modified range can begin later */
       if (lineStart <= pos)
       {
          while (visLineNum < nVisLines && lineStarts[visLineNum] < lineStart)
             visLineNum++;
          if (visLineNum < nVisLines && lineStarts[visLineNum] == lineStart)
          {
             countFrom = lineStart;
             nLines = 0;
             if (visLineNum + 1 < nVisLines && lineStarts[visLineNum + 1] != -1)
                *modRangeStart = std::min(pos, lineStarts[visLineNum + 1] - 1);
             else
                *modRangeStart = countFrom;
          }
          else
             *modRangeStart = std::min(*modRangeStart, lineStart - 1);
       }

       /* check for synchronization with the original line starts array
          after pos, if so, the modified range can end early */
       else if (lineStart > pos + nInserted)
       {
          adjLineStart = lineStart - nInserted + nDeleted;
          while (visLineNum < nVisLines && lineStarts[visLineNum] < adjLineStart)
             visLineNum++;
          if (visLineNum < nVisLines && lineStarts[visLineNum] != -1 &&
                lineStarts[visLineNum] == adjLineStart)
          {
             countTo = endOfLine(lineStart, true);
             *modRangeEnd = lineStart;
             break;
          }
       }
    }
    *linesInserted = nLines;


    /* Count deleted lines between countFrom and countTo as the text existed
       before the modification (that is, as if the text between pos and
       pos+nInserted were replaced by "deletedText").  This extra context is
       necessary because wrapping can occur outside of the modified region
       as a result of adding or deleting text in the region. This is done by
       creating a Ne_Text_Buffer containing the deleted text and the necessary
       additional context, and calling the wrappedLineCounter on it.

       NOTE: This must not be done in continuous wrap mode when the font
        width is not fixed. In that case, the calculation would try
        to access style information that is no longer available (deleted
        text), or out of date (updated highlighting), possibly leading
        to completely wrong calculations and/or even crashes eventually.
        (This is not theoretical; it really happened.)

        In that case, the calculation of the number of deleted lines
        has happened before the buffer was modified (only in that case,
        because resynchronization of the line starts is impossible
        in that case, which makes the whole calculation less efficient).
    */
    if (this->suppressResync)
    {
       *linesDeleted = this->nLinesDeleted;
       this->suppressResync = 0;
       return;
    }

    length = (pos - countFrom) + nDeleted + (countTo - (pos + nInserted));
    deletedTextBuf = new Ne_Text_Buffer(length);
    if (pos > countFrom)
       deletedTextBuf->copyFromBuf(*this->buffer, countFrom, pos, 0);
    if (nDeleted != 0)
       deletedTextBuf->insertAt(pos - countFrom, deletedText);
    if (countTo > pos + nInserted)
       deletedTextBuf->copyFromBuf(*this->buffer, pos + nInserted, countTo, pos - countFrom + nDeleted);
    /* Note that we need to take into account an offset for the style buffer:
       the deletedTextBuf can be out of sync with the style buffer. */
    wrappedLineCounter(deletedTextBuf, 0, length, INT_MAX, true,
                       countFrom, &retPos, &retLines, &retLineStart, &retLineEnd);
    delete deletedTextBuf;
    *linesDeleted = retLines;
    this->suppressResync = 0;
 }

 /*
 ** This is a stripped-down version of the findWrapRange() function above,
 ** intended to be used to calculate the number of "deleted" lines during
 ** a buffer modification. It is called _before_ the modification takes place.
 **
 ** This function should only be called in continuous wrap mode with a
 ** non-fixed font width. In that case, it is impossible to calculate
 ** the number of deleted lines, because the necessary style information
 ** is no longer available _after_ the modification. In other cases, we
 ** can still perform the calculation afterwards (possibly even more
 ** efficiently).
 */
 void Ne_Text_Display::measureDeletedLines(int pos, int nDeleted)
 {
    int retPos, retLines, retLineStart, retLineEnd;
    Ne_Text_Buffer* buf = this->buffer;
    int nVisLines = this->nVisibleLines;
    int* lineStarts = this->lineStarts;
    int countFrom, lineStart;
    int nLines = 0, i;
    /*
    ** Determine where to begin searching: either the previous newline, or
    ** if possible, limit to the start of the (original) previous displayed
    ** line, using information from the existing line starts array
    */
    if (pos >= this->firstChar && pos <= this->lastChar)
    {
       for (i = nVisLines - 1; i > 0; i--)
          if (lineStarts[i] != -1 && pos >= lineStarts[i])
             break;
       if (i > 0)
       {
          countFrom = lineStarts[i - 1];
       }
       else
          countFrom = buf->startOfLine(pos);
    }
    else
       countFrom = buf->startOfLine(pos);

    /*
    ** Move forward through the (new) text one line at a time, counting
    ** displayed lines, and looking for either a real newline, or for the
    ** line starts to re-sync with the original line starts array
    */
    lineStart = countFrom;
    while (true)
    {
       /* advance to the next line.  If the line ended in a real newline
          or the end of the buffer, that's far enough */
       wrappedLineCounter(buf, lineStart, buf->length(), 1, true, 0,
                          &retPos, &retLines, &retLineStart, &retLineEnd);
       if (retPos >= buf->length())
       {
          if (retPos != retLineEnd)
             nLines++;
          break;
       }
       else
          lineStart = retPos;
       nLines++;
       if (lineStart > pos + nDeleted &&
             buf->getCharacter(lineStart - 1) == '\n')
       {
          break;
       }

       /* Unlike in the findWrapRange() function above, we don't try to
          resync with the line starts, because we don't know the length
          of the inserted text yet, nor the updated style information.

          Because of that, we also shouldn't resync with the line starts
          after the modification either, because we must perform the
          calculations for the deleted and inserted lines in the same way.

          This can result in some unnecessary recalculation and redrawing
          overhead, and therefore we should only use this two-phase mode
          of calculation when it's really needed (continuous wrap + variable
          font width). */
    }
    this->nLinesDeleted = nLines;
    this->suppressResync = 1;
 }

/*
** Count forward from startPos to either maxPos or maxLines (whichever is
** reached first), and return all relevant positions and line count.
** The provided Ne_Text_Buffer may differ from the actual text buffer of the
** widget. In that case it must be a (partial) copy of the actual text buffer
** and the styleBufOffset argument must indicate the starting position of the
** copy, to take into account the correct style information.
**
** Returned values:
**
**   retPos:	    Position where counting ended.  When counting lines, the
**  	    	    position returned is the start of the line "maxLines"
**  	    	    lines beyond "startPos".
**   retLines:	    Number of line breaks counted
**   retLineStart:  Start of the line where counting ended
**   retLineEnd:    End position of the last line traversed
*/
void Ne_Text_Display::wrappedLineCounter(const Ne_Text_Buffer* buf,
   const int startPos, const int maxPos, const int maxLines,
   const bool startPosIsLineStart, const int styleBufOffset,
   int* retPos, int* retLines, int* retLineStart, int* retLineEnd)
{
   int lineStart, newLineStart = 0, b, p, colNum, wrapMargin;
   int maxWidth, width, countPixels, i, foundBreak;
   int nLines = 0, tabDist = this->buffer->getTabDistance();
   unsigned char c;
   char nullSubsChar = this->buffer->getNullSubsChar();

   /* If the font is fixed, or there's a wrap margin set, it's more efficient
   to measure in columns, than to count pixels.  Determine if we can count
   in columns (countPixels == false) or must count pixels (countPixels ==
   true), and set the wrap target for either pixels or columns */
   if (this->fixedFontWidth != -1 || this->wrapMargin != 0)
   {
      countPixels = false;
      wrapMargin = this->wrapMargin != 0 ? this->wrapMargin : this->width / this->fixedFontWidth;
      maxWidth = INT_MAX;
   }
   else
   {
      countPixels = true;
      wrapMargin = INT_MAX;
      maxWidth = this->width;
   }

   /* Find the start of the line if the start pos is not marked as a
   line start. */
   if (startPosIsLineStart)
      lineStart = startPos;
   else
      lineStart = startOfLine(startPos);

   /*
   ** Loop until position exceeds maxPos or line count exceeds maxLines.
   ** (actually, contines beyond maxPos to end of line containing maxPos,
   ** in case later characters cause a word wrap back before maxPos)
   */
   colNum = 0;
   width = 0;
   for (p = lineStart; p < buf->size(); p++)
   {
      c = buf->getCharacter(p);

      /* If the character was a newline, count the line and start over,
      otherwise, add it to the width and column counts */
      if (c == '\n')
      {
         if (p >= maxPos)
         {
            *retPos = maxPos;
            *retLines = nLines;
            *retLineStart = lineStart;
            *retLineEnd = maxPos;
            return;
         }
         nLines++;
         if (nLines >= maxLines)
         {
            *retPos = p + 1;
            *retLines = nLines;
            *retLineStart = p + 1;
            *retLineEnd = p;
            return;
         }
         lineStart = p + 1;
         colNum = 0;
         width = 0;
      }
      else
      {
         colNum += buf->charWidth(c, colNum, tabDist, nullSubsChar);
         if (countPixels)
            width += measurePropChar(c, colNum, p + styleBufOffset);
      }

      // If character exceeded wrap margin, find the break point and wrap there
      if (colNum > wrapMargin || width > maxWidth)
      {
         foundBreak = false;
         for (b = p; b >= lineStart; b--)
         {
            c = buf->getCharacter(b);
            if (c == '\t' || c == ' ')
            {
               newLineStart = b + 1;
               if (countPixels)
               {
                  colNum = 0;
                  width = 0;
                  for (i = b + 1; i < p + 1; i++)
                  {
                     width += measurePropChar(buf->getCharacter(i), colNum, i + styleBufOffset);
                     colNum++;
                  }
               }
               else
                  colNum = buf->countDispChars(b + 1, p + 1);
               foundBreak = true;
               break;
            }
         }
         if (!foundBreak)   // no whitespace, just break at margin
         {
            newLineStart = std::max(p, lineStart + 1);
            colNum = buf->charWidth(c, colNum, tabDist, nullSubsChar);
            if (countPixels)
               width = measurePropChar(c, colNum, p + styleBufOffset);
         }
         if (p >= maxPos)
         {
            *retPos = maxPos;
            *retLines = maxPos < newLineStart ? nLines : nLines + 1;
            *retLineStart = maxPos < newLineStart ? lineStart : newLineStart;
            *retLineEnd = maxPos;
            return;
         }
         nLines++;
         if (nLines >= maxLines)
         {
            *retPos = foundBreak ? b + 1 : std::max(p, lineStart + 1);
            *retLines = nLines;
            *retLineStart = lineStart;
            *retLineEnd = foundBreak ? b : p;
            return;
         }
         lineStart = newLineStart;
      }
   }

   // reached end of buffer before reaching pos or line target
   *retPos = buf->size();
   *retLines = nLines;
   *retLineStart = lineStart;
   *retLineEnd = buf->size();
}

/*
** Measure the width in pixels of a character "c" at a particular column
** "colNum" and buffer position "pos".  This is for measuring characters in
** proportional or mixed-width highlighting fonts.
**
** A note about proportional and mixed-width fonts: the mixed width and
** proportional font code in nedit does not get much use in general editing,
** because nedit doesn't allow per-language-mode fonts, and editing programs
** in a proportional font is usually a bad idea, so very few users would
** choose a proportional font as a default.  There are still probably mixed-
** width syntax highlighting cases where things don't redraw properly for
** insertion/deletion, though static display and wrapping and resizing
** should now be solid because they are now used for online help display.
*/
int Ne_Text_Display::measurePropChar(const char c, const int colNum, const int pos)
{
   int charLen, style;
   char expChar[MAX_EXP_CHAR_LEN];
   Ne_Text_Buffer* styleBuf = this->styleBuffer;

   charLen = buffer->expandCharacter(c, colNum, expChar, this->buffer->getTabDistance(), this->buffer->getNullSubsChar());
   if (styleBuf == NULL)
   {
      style = 0;
   }
   else
   {
      style = (unsigned char)styleBuf->getCharacter(pos);
      if (style == this->unfinishedStyle)
      {
         // encountered "unfinished" style, trigger parsing
         (*this->unfinishedHighlightCB)(this, pos, this->highlightCBArg);
         style = (unsigned char)styleBuf->getCharacter(pos);
      }
   }
   return stringWidth(expChar, charLen, style);
}

/*
** Finds both the end of the current line and the start of the next line.  Why?
** In continuous wrap mode, if you need to know both, figuring out one from the
** other can be expensive or error prone.  The problem comes when there's a
** trailing space or tab just before the end of the buffer.  To translate an
** end of line value to or from the next lines start value, you need to know
** whether the trailing space or tab is being used as a line break or just a
** normal character, and to find that out would otherwise require counting all
** the way back to the beginning of the line.
*/
void Ne_Text_Display::findLineEnd(int startPos, bool startPosIsLineStart, int* lineEnd, int* nextLineStart)
{
   int retLines, retLineStart;

   // if we're not wrapping use more efficient BufEndOfLine
   if (!this->continuousWrap)
   {
      *lineEnd = this->buffer->endOfLine(startPos);
      *nextLineStart = std::min(this->buffer->size(), *lineEnd + 1);
      return;
   }

   // use the wrapped line counter routine to count forward one line
   wrappedLineCounter(this->buffer, startPos, this->buffer->size(),
      1, startPosIsLineStart, 0, nextLineStart, &retLines,
      &retLineStart, lineEnd);
   return;
}

/*
** Line breaks in continuous wrap mode usually happen at newlines or
** whitespace.  This line-terminating character is not included in line
** width measurements and has a special status as a non-visible character.
** However, lines with no whitespace are wrapped without the benefit of a
** line terminating character, and this distinction causes endless trouble
** with all of the text display code which was originally written without
** continuous wrap mode and always expects to wrap at a newline character.
**
** Given the position of the end of the line, as returned by TextDEndOfLine
** or BufEndOfLine, this returns true if there is a line terminating
** character, and false if there's not.  On the last character in the
** buffer, this function can't tell for certain whether a trailing space was
** used as a wrap point, and just guesses that it wasn't.  So if an exact
** accounting is necessary, don't use this function.
*/
int Ne_Text_Display::wrapUsesCharacter(int lineEndPos)
{

   if (!this->continuousWrap || lineEndPos == this->buffer->size())
      return true;
   char c = this->buffer->getCharacter(lineEndPos);
   return c == '\n' || ((c == '\t' || c == ' ') && lineEndPos + 1 != this->buffer->size());
}

/*
** Decide whether the user needs (or may need) a horizontal scroll bar,
** and manage or unmanage the scroll bar widget accordingly.  The H.
** scroll bar is only hidden in continuous wrap mode when it's absolutely
** certain that the user will not need it: when wrapping is set
** to the window edge, or when the wrap margin is strictly less than
** the longest possible line.
*/
void Ne_Text_Display::hideOrShowHScrollBar()
{
   int charWidth = this->fontStruct.max_width();
   if (this->continuousWrap && (this->wrapMargin == 0 || this->wrapMargin * charWidth < this->width))
      this->hScrollBar->hide();
   else
      this->hScrollBar->show();
}

 /*
 ** Extend the range of a redraw request (from *start to *end) with additional
 ** redraw requests resulting from changes to the attached style buffer (which
 ** contains auxiliary information for coloring or styling text).
 */
 void Ne_Text_Display::extendRangeForStyleMods(int* start, int* end)
 {
    Ne_Text_Selection* sel = &this->styleBuffer->getSelection();
    int extended = false;

    /* The peculiar protocol used here is that modifications to the style
       buffer are marked by selecting them with the buffer's primary selection.
       The style buffer is usually modified in response to a modify callback on
       the text buffer BEFORE Ne_Text_Display.c's modify callback, so that it can keep
       the style buffer in step with the text buffer.  The style-update
       callback can't just call for a redraw, because Ne_Text_Display hasn't processed
       the original text changes yet.  Anyhow, to minimize redrawing and to
       avoid the complexity of scheduling redraws later, this simple protocol
       tells the text display's buffer modify callback to extend it's redraw
       range to show the text color/and font changes as well. */
    if (sel->selected)
    {
       if (sel->start < *start)
       {
          *start = sel->start;
          extended = true;
       }
       if (sel->end > *end)
       {
          *end = sel->end;
          extended = true;
       }
    }

    /* If the selection was extended due to a style change, and some of the
       fonts don't match in spacing, extend redraw area to end of line to
       redraw characters exposed by possible font size changes */
    if (this->fixedFontWidth == -1 && extended)
       *end = this->buffer->endOfLine(*end) + 1;
 }

 /**********************  Backlight Functions ******************************/
 /*
 ** Allocate a read-only (shareable) colormap cell for a named color, from the
 ** the default colormap of the screen on which the widget (w) is displayed. If
 ** the colormap is full and there's no suitable substitute, print an error on
 ** stderr, and return the widget's background color as a backup.
 */
Fl_Color Ne_Text_Display::allocBGColor(const char* colorName, int* ok)
 {
    *ok = 1;
    return GetColor(colorName);
 }

 Fl_Color Ne_Text_Display::getRangesetColor(int ind, Fl_Color bground)
 {
    Ne_Text_Buffer* buf;
    NeRangesetTable* tab;
    Fl_Color color;
    char* color_name;
    int valid;

    if (ind > 0)
    {
       ind--;
       buf = this->buffer;
       tab = buf->rangesetTable;

       valid = RangesetTableGetColorValid(tab, ind, &color);
       if (valid == 0)
       {
          color_name = RangesetTableGetColorName(tab, ind);
          if (color_name)
             color = allocBGColor(color_name, &valid);
          RangesetTableAssignColorPixel(tab, ind, color, valid);
       }
       if (valid > 0)
       {
          return color;
       }
    }
    return bground;
 }

 /*
 ** Read the background color class specification string in str, allocating the
 ** necessary colors, and allocating and setting up the character->class_no and
 ** class_no->pixel map arrays, returned via *pp_bgClass and *pp_bgClassPixel
 ** respectively.
 ** Note: the allocation of class numbers could be more intelligent: there can
 ** never be more than 256 of these (one per character); but I don't think
 ** there'll be a pressing need. I suppose the scanning of the specification
 ** could be better too, but then, who cares!
 */
 void Ne_Text_Display::setupBGClasses(const char* str, Fl_Color** pp_bgClassPixel, unsigned char** pp_bgClass, Fl_Color bgPixelDefault)
 {
    unsigned char bgClass[256];
    Fl_Color bgClassPixel[256];
    int class_no = 0;
    char* semicol;
    char* s = (char*)str;
    size_t was_semicol;
    int lo, hi, dummy;
    char* pos;
    bool is_good = true;

    free((char*)*pp_bgClass);
    free((char*)*pp_bgClassPixel);

    *pp_bgClassPixel = NULL;
    *pp_bgClass = NULL;

    if (!s)
       return;

    /* default for all chars is class number zero, for standard background */
    memset(bgClassPixel, 0, sizeof bgClassPixel);
    memset(bgClass, 0, sizeof bgClass);
    bgClassPixel[0] = bgPixelDefault;
    /* since class no == 0 in a "style" has no set bits in BACKLIGHT_MASK
       (see styleOfPos()), when drawString() is called for text with a
       backlight class no of zero, bgClassPixel[0] is never consulted, and
       the default background color is chosen. */

    /* The format of the class string s is:
              low[-high]{,low[-high]}:color{;low-high{,low[-high]}:color}
          eg
              32-255:#f0f0f0;1-31,127:red;128-159:orange;9-13:#e5e5e5
       where low and high represent a character range between ordinal
       ASCII values. Using strtol() allows automatic octal, dec and hex
       reading of low and high. The example format sets backgrounds as follows:
              char   1 - 8    colored red     (control characters)
              char   9 - 13   colored #e5e5e5 (isspace() control characters)
              char  14 - 31   colored red     (control characters)
              char  32 - 126  colored #f0f0f0
              char 127        colored red     (delete character)
              char 128 - 159  colored orange  ("shifted" control characters)
              char 160 - 255  colored #f0f0f0
       Notice that some of the later ranges overwrite the class values defined
       for earlier ones (eg the first clause, 32-255:#f0f0f0 sets the DEL
       character background color to #f0f0f0; it is then set to red by the
       clause 1-31,127:red). */

    while (s && class_no < 255)
    {
       class_no++;                   /* simple class alloc scheme */
       was_semicol = 0;
       is_good = true;
       if ((semicol = (char*)strchr(s, ';')))
       {
          *semicol = '\0';    /* null-terminate low[-high]:color clause */
          was_semicol = 1;
       }

       /* loop over ranges before the color spec, assigning the characters
          in the ranges to the current class number */
       for (lo = hi = strtol(s, &pos, 0);
             is_good;
             lo = hi = strtol(pos + 1, &pos, 0))
       {
          if (pos && *pos == '-')
             hi = strtol(pos + 1, &pos, 0);  /* get end of range */
          is_good = (pos && 0 <= lo && lo <= hi && hi <= 255);
          if (is_good)
             while (lo <= hi)
                bgClass[lo++] = (unsigned char)class_no;
          if (*pos != ',')
             break;
       }
       if ((is_good = (is_good && *pos == ':')))
       {
          is_good = (*pos++ != '\0');         /* pos now points to color */
          bgClassPixel[class_no] = allocBGColor(pos, &dummy);
       }
       if (!is_good)
       {
          /* complain? this class spec clause (in string s) was faulty */
       }

       /* end of loop iterator clauses */
       if (was_semicol)
          *semicol = ';';       /* un-null-terminate low[-high]:color clause */
       s = semicol + was_semicol;
    }

    /* when we get here, we've set up our class table and class-to-pixel table
       in local variables: now put them into the "real thing" */
    class_no++;                     /* bigger than all valid class_nos */
    *pp_bgClass = (unsigned char*)malloc(256);
    *pp_bgClassPixel = (Fl_Color*)malloc(class_no * sizeof(Fl_Color));
    if (!*pp_bgClass || !*pp_bgClassPixel)
    {
       free((char*)*pp_bgClass);
       free((char*)*pp_bgClassPixel);
       return;
    }
    memcpy(*pp_bgClass, bgClass, 256);
    memcpy(*pp_bgClassPixel, bgClassPixel, class_no * sizeof(Fl_Color));
 }

// --------------------------------------------------------------------------
void Ne_Text_Display::bufPreDeleteCB(int pos, int nDeleted)
{
    if (this->continuousWrap &&
          (this->fixedFontWidth == -1 || this->modifyingTabDist))
       /* Note: we must perform this measurement, even if there is not a
          single character deleted; the number of "deleted" lines is the
          number of visual lines spanned by the real line in which the
          modification takes place.
          Also, a modification of the tab distance requires the same
          kind of calculations in advance, even if the font width is "fixed",
          because when the width of the tab characters changes, the layout
          of the text may be completely different. */
       measureDeletedLines(pos, nDeleted);
    else
       this->suppressResync = 0; // Probably not needed, but just in case
}

// --------------------------------------------------------------------------
void Ne_Text_Display::bufModifiedCB(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText)
{
   int linesInserted, linesDeleted; //, startDispPos, endDispPos;
   Ne_Text_Buffer* buf = this->buffer;
   int oldFirstChar = this->firstChar;
   bool scrolled;
   int origCursorPos = this->cursorPos;
    int wrapModStart, wrapModEnd;

   // buffer modification cancels vertical cursor motion column
   if (nInserted != 0 || nDeleted != 0)
      this->cursorPreferredCol = -1;

    // Count the number of lines inserted and deleted, and in the case
    // of continuous wrap mode, how much has changed
    if (this->continuousWrap)
    {
       findWrapRange(deletedText, pos, nInserted, nDeleted, &wrapModStart, &wrapModEnd, &linesInserted, &linesDeleted);
    }
    else
    {
      linesInserted = nInserted == 0 ? 0 : buf->countLines(pos, pos + nInserted);
      linesDeleted = nDeleted == 0 ? 0 : countLines(deletedText);
    }

   // Update the line starts and topLineNum
   if (nInserted != 0 || nDeleted != 0)
   {
      if (this->continuousWrap)
      {
          updateLineStarts(wrapModStart, wrapModEnd - wrapModStart,
                           nDeleted + pos - wrapModStart + (wrapModEnd - (pos + nInserted)),
                           linesInserted, linesDeleted, &scrolled);
      }
      else
      {
         updateLineStarts(pos, nInserted, nDeleted, linesInserted, linesDeleted, &scrolled);
      }
   }
   else
      scrolled = false;

    // If we're counting non-wrapped lines as well, maintain the absolute
    // (non-wrapped) line number of the text displayed
    if (maintainingAbsTopLineNum() && (nInserted != 0 || nDeleted != 0))
    {
       if (pos + nDeleted < oldFirstChar)
          this->absTopLineNum += buf->countLines(pos, pos + nInserted) - countLines(deletedText);
       else if (pos < oldFirstChar)
          resetAbsLineNum();
    }

   // Update the line count for the whole buffer
   this->nBufferLines += linesInserted - linesDeleted;

   // Update the scroll bar ranges (and value if the value changed).  Note
   // that updating the horizontal scroll bar range requires scanning the
   // entire displayed text, however, it doesn't seem to hurt performance
   // much.  Note also, that the horizontal scroll bar update routine is
   // allowed to re-adjust horizOffset if there is blank space to the right
   // of all lines of text.
   updateVScrollBarRange();
   scrolled = scrolled || updateHScrollBarRange();

   // Update the cursor position
   if (this->cursorToHint != NO_HINT)
   {
      this->cursorPos = this->cursorToHint;
      this->cursorToHint = NO_HINT;
   }
   else if (this->cursorPos > pos)
   {
      if (this->cursorPos < pos + nDeleted)
         this->cursorPos = pos;
      else
         this->cursorPos += nInserted - nDeleted;
   }

   redraw(); // To remove... when below is done
   // TODO:     /* If the changes caused scrolling, re-paint everything and we're done. */
   // TODO:     if (scrolled)
   // TODO:     {
   // TODO:        blankCursorProtrusions(textD);
   // TODO:        redisplayRect(textD, 0, this->top, this->width + this->left, this->height);
   // TODO:        if (this->styleBuffer)  /* See comments in extendRangeForStyleMods */
   // TODO:        {
   // TODO:           this->styleBuffer->primary.selected = false;
   // TODO:           this->styleBuffer->primary.zeroWidth = false;
   // TODO:        }
   // TODO:        return;
   // TODO:     }
   // TODO: 
   // TODO:     /* If the changes didn't cause scrolling, decide the range of characters
   // TODO:        that need to be re-painted.  Also if the cursor position moved, be
   // TODO:        sure that the redisplay range covers the old cursor position so the
   // TODO:        old cursor gets erased, and erase the bits of the cursor which extend
   // TODO:        beyond the left and right edges of the text. */
   // TODO:     startDispPos = this->continuousWrap ? wrapModStart : pos;
   // TODO:     if (origCursorPos == startDispPos && this->cursorPos != startDispPos)
   // TODO:        startDispPos = min(startDispPos, origCursorPos - 1);
   // TODO:     if (linesInserted == linesDeleted)
   // TODO:     {
   // TODO:        if (nInserted == 0 && nDeleted == 0)
   // TODO:           endDispPos = pos + nRestyled;
   // TODO:        else
   // TODO:        {
   // TODO:           endDispPos = this->continuousWrap ? wrapModEnd :
   // TODO:                        BufEndOfLine(buf, pos + nInserted) + 1;
   // TODO:           if (origCursorPos >= startDispPos &&
   // TODO:                 (origCursorPos <= endDispPos || endDispPos == buf->length))
   // TODO:              blankCursorProtrusions(textD);
   // TODO:        }
   // TODO:        /* If more than one line is inserted/deleted, a line break may have
   // TODO:           been inserted or removed in between, and the line numbers may
   // TODO:           have changed. If only one line is altered, line numbers cannot
   // TODO:           be affected (the insertion or removal of a line break always
   // TODO:           results in at least two lines being redrawn). */
   // TODO:        if (linesInserted > 1) redrawLineNumbers(textD, false);
   // TODO:     }
   // TODO:     else     /* linesInserted != linesDeleted */
   // TODO:     {
   // TODO:        endDispPos = this->lastChar + 1;
   // TODO:        if (origCursorPos >= pos)
   // TODO:           blankCursorProtrusions(textD);
   // TODO:        redrawLineNumbers(textD, false);
   // TODO:     }
   // TODO: 
   // TODO:     /* If there is a style buffer, check if the modification caused additional
   // TODO:        changes that need to be redisplayed.  (Redisplaying separately would
   // TODO:        cause double-redraw on almost every modification involving styled
   // TODO:        text).  Extend the redraw range to incorporate style changes */
   // TODO:     if (this->styleBuffer)
   // TODO:        extendRangeForStyleMods(textD, &startDispPos, &endDispPos);
   // TODO: 
   // TODO:     /* Redisplay computed range */
   // TODO:     textDRedisplayRange(textD, startDispPos, endDispPos);
}
