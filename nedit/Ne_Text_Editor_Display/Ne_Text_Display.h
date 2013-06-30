#ifndef Ne_Text_Display_h__
#define Ne_Text_Display_h__

#include "../util/Ne_Font.h"
#include "../util/Ne_Dimension.h"

#include <FL/Fl_Group.H>
#include <FL/Fl_Scrollbar.H>

class Ne_Text_Buffer;
struct Ne_Text_Selection;

// --------------------------------------------------------------------------
enum NePositionTypes
{
   NE_CURSOR_POS,
   NE_CHARACTER_POS
};

// --------------------------------------------------------------------------
enum NeCursorStyles
{
   NE_NORMAL_CURSOR,
   NE_CARET_CURSOR,
   NE_DIM_CURSOR,
   NE_BLOCK_CURSOR,
   NE_HEAVY_CURSOR
};

// --------------------------------------------------------------------------
enum NeDragStates
{
   NE_NOT_CLICKED,
   NE_PRIMARY_CLICKED,
   NE_SECONDARY_CLICKED,
   NE_CLICKED_IN_SELECTION,
   NE_PRIMARY_DRAG,
   NE_SECONDARY_DRAG,
   NE_PRIMARY_BLOCK_DRAG,
   NE_DRAG_CANCELED,
   NE_MOUSE_PAN
};

// --------------------------------------------------------------------------
enum NeMultiClickStates
{
   NE_NO_CLICKS,
   NE_ONE_CLICK,
   NE_TWO_CLICKS,
   NE_THREE_CLICKS
};

#define NO_HINT -1

// --------------------------------------------------------------------------
struct NeStyleTableEntry
{
    char *highlightName;
    char *styleName;
    char *colorName;
    char isBold;
    char isItalic;
    unsigned short red;
    unsigned short green;
    unsigned short blue;
    Fl_Color color;
    bool underline;
    Ne_Font font;
    char *bgColorName;      /* background style coloring (name may be NULL) */
    unsigned short bgRed;
    unsigned short bgGreen;
    unsigned short bgBlue;
    Fl_Color bgColor;
} ;

class Ne_Text_Display;

typedef void (*NeUnfinishedStyleCBProc)(Ne_Text_Display*, int pos, void*);

// --------------------------------------------------------------------------
struct NeCalltipStruct 
{
    int ID;          /* ID of displayed calltip.  Equals zero if none is displayed. */
    bool anchored;   /* Is it anchored to a position */
    int pos;         /* Position tip is anchored to */
    int hAlign;      /* horizontal alignment */
    int vAlign;      /* vertical alignment */
    int alignMode;   /* Strict or sloppy alignment */
};

// --------------------------------------------------------------------------
class Ne_Text_Display : public Fl_Group 
{
public:
   Ne_Text_Display(int x, int y, int w, int h, const char* l = 0);
   ~Ne_Text_Display();

   int top, left, width, height;    // Text Drawing Area Only
   int lineNumLeft, lineNumWidth;   // Line Number Area
   int cursorPos;             // Insert cursor position
   bool cursorOn;             // Do we have to show the cursor ?
   int cursorX, cursorY;      // X, Y pos. of last drawn cursor
   int cursorToHint;          // Tells the buffer modified callback where to move the cursor, to reduce the number of redraw calls
   NeCursorStyles cursorStyle;  // One of enum cursorStyles above
   int cursorPreferredCol;    // Column for vert. cursor movement
   int nVisibleLines;         // # of visible (displayed) lines
   int nBufferLines;          // # of newlines in the buffer
   Ne_Text_Buffer* buffer;    // Contains text to be displayed
   Ne_Text_Buffer* styleBuffer;  // Optional parallel buffer containing color and font information
   int firstChar, lastChar;   // Buffer positions of first and last displayed character (lastChar points
                              // either to a newline or one character beyond the end of the buffer)
   bool continuousWrap;       // Wrap long lines when displaying
   int wrapMargin;            // Margin in # of char positions for wrapping in continuousWrap mode
   int *lineStarts;
   int topLineNum;            // Line number of top displayed line of file (first line of file is 1)
   int absTopLineNum;         // In continuous wrap mode, the line number of the top line if the text were not wrapped (note that this is only maintained as needed).
   int needAbsTopLineNum;     // Externally settable flag to continue maintaining absTopLineNum even if it isn't needed for line # display
   int horizOffset;           // Horizontal scroll pos. in pixels

   int nStyles;               // Number of entries in styleTable
   NeStyleTableEntry* styleTable;  // Table of fonts and colors for coloring/syntax-highlighting
   char unfinishedStyle;      // Style buffer entry which triggers on-the-fly reparsing of region
   NeUnfinishedStyleCBProc* unfinishedHighlightCB; // Callback to parse "unfinished" regions
   void* highlightCBArg;      // Arg to unfinishedHighlightCB
   Ne_Font fontStruct;        // Font structure for primary font
   int ascent, descent;       // Composite ascent and descent for primary font + all-highlight fonts
   int fixedFontWidth;        // Font width if all current fonts are fixed and match in width, else -1
   Fl_Scrollbar* hScrollBar;
   Fl_Scrollbar* vScrollBar;

   Fl_Color fgPixel, bgPixel;             // Foreground/Background colors
   Fl_Color selectFGPixel, selectBGPixel; // Foreground,Background  select color
   Fl_Color highlightFGPixel, highlightBGPixel; // Highlight colors are used when flashing matching parens
   Fl_Color lineNumFGPixel;   // Color for drawing line numbers
   Fl_Color cursorFGPixel;
   Fl_Color* bgClassPixel;      /* table of colors for each BG class */
   unsigned char *bgClass;      /* obtains index into bgClassPixel[] */

// TODO:    Fl_Widget* calltipW;       /* The Label widget for the calltip */
// TODO:    Fl_Widget* calltipShell;   /* The Shell that holds the calltip */
// TODO:    CalltipStruct calltip;     /* The info for the calltip itself */
   Fl_Color calltipFGPixel, calltipBGPixel;
   int suppressResync;         /* Suppress resynchronization of line starts during buffer updates */
   int nLinesDeleted;         /* Number of lines deleted during buffer modification (only used when resynchronization is suppressed) */
   int modifyingTabDist;      /* Whether tab distance is being modified */
   bool pointerHidden;        /* true if the mouse pointer is hidden */

   // This part is a merge from textP.h file
   int marginWidth, marginHeight;      // Marging between internal widgets
   int rows, columns;                  // # of Rows and Cols visible in the Text Area
   int lineNumCols;
   int cursorVPadding;
   bool autoShowInsertPos;
   NeDragStates dragState;    // Why is the mouse being dragged and what is being acquired
   int emTabsBeforeCursor;		// If non-zero, number of consecutive emulated tabs just entered.  Saved so chars can be deleted as a unit
   int anchor;                // Anchors for drag operations and rectangular drag operations
   std::string delimiters;
   bool isOverstrike;
   bool heavyCursor;
   bool autoIndent;
   bool smartIndent;
   bool readOnly;
   bool pendingDelete;
   int emulateTabs;
   bool autoWrap;
   bool autoWrapPastedText;
   int btnDownX, btnDownY;		// Mark the position of last btn down action for deciding when to begin
                              // paying attention to motion actions, and where to paste columns
   int mouseX, mouseY;			// Last known mouse position in drag operation (for autoscroll)
   int multiClickState;       // How long is this multi-click sequence so far
   double lastBtnDown;        // Timestamp of last button down event for multi-click recognition

   Ne_Text_Buffer* dragOrigBuf;  /* backup buffer copy used during block dragging of selections */
   int dragXOffset, dragYOffset; /* offsets between cursor location and actual insertion point in drag */
   int dragType;   	    	    	/* style of block drag operation */
   int dragInsertPos;	         /* location where text being block dragged was last inserted */
   int dragRectStart;	    	   /* rect. offset "" */
   int dragInserted;	    	    	/* # of characters inserted at drag destination in last drag position */
   int dragDeleted;	    	    	/* # of characters deleted "" */
   int dragSourceDeletePos;    	/* location from which move source text was removed at start of drag */
   int dragSourceInserted;       /* # of chars. inserted when move source text was deleted */
   int dragSourceDeleted;  	   /* # of chars. deleted "" */
   int dragNLines; 	    	    	/* # of newlines in text being drag'd */

   // Factory for Creating a full ocnfigurated Ne_Text_Display
   static Ne_Text_Display* Create(int x, int y, int w, int h,
      int lineNumWidth,
      Ne_Text_Buffer* buffer,
      const Ne_Font& font,
      Fl_Color bgPixel, Fl_Color fgPixel,
      Fl_Color selectFGPixel, Fl_Color selectBGPixel,
      Fl_Color highlightFGPixel, Fl_Color highlightBGPixel,
      Fl_Color cursorFGPixel, Fl_Color lineNumFGPixel,
      bool continuousWrap, int wrapMargin,
      const char* bgClassString,
      Fl_Color calltipFGPixel, Fl_Color calltipBGPixel);

   void setBuffer(Ne_Text_Buffer* buffer);
   Ne_Text_Buffer* getBuffer();
   void attachHighlightData(Ne_Text_Buffer *styleBuffer, NeStyleTableEntry *styleTable, int nStyles, char unfinishedStyle, NeUnfinishedStyleCBProc* unfinishedHighlightCB, void *cbArg);
   void setColors(Fl_Color textFgP, Fl_Color textBgP, Fl_Color selectFgP, Fl_Color selectBgP, Fl_Color hiliteFgP, Fl_Color hiliteBgP,  Fl_Color lineNoFgP, Fl_Color cursorFgP);
   void setFont(const Ne_Font& fontStruct);
   int minFontWidth(bool considerStyles);
   int maxFontWidth(bool considerStyles);
   void resizeTextArea(int width, int height, int oldWidth);
   void redisplayRect(int left, int top, int width, int height);
   void setScroll(int topLineNum, int horizOffset);
   void getScroll(int *topLineNum, int *horizOffset);
   void insert(const char *text);
   void overstrike(const char *text);
   void setInsertPosition(int newPos);
   int getInsertPosition();
   int XYToPosition(int x, int y);
   int XYToCharPos(int x, int y);
   void XYToUnconstrainedPosition(int x, int y, int *row, int *column);
   int lineAndColToPos(int lineNum, int column);
   int OffsetWrappedColumn(int row, int column);
   int OffsetWrappedRow(int row);
   bool positionToXY(int pos, int *x, int *y);
   int posToLineAndCol(int pos, int *lineNum, int *column);
   bool inSelection(int x, int y);
   void makeInsertPosVisible();
   bool moveRight();
   bool moveLeft();
   bool moveUp(bool absolute);
   bool moveDown(bool absolute);
   void blankCursor();
   void unblankCursor();
   void setCursorStyle(NeCursorStyles style);
   void setWrapMode(bool wrap, int wrapMargin);
   int endOfLine(const int pos, const bool startPosIsLineStart);
   int startOfLine(const int pos);
   int countForwardNLines(const int startPos, const unsigned nLines, const bool startPosIsLineStart);
   int countBackwardNLines(int startPos, int nLines);
   int countLines(int startPos, int endPos, bool startPosIsLineStart);
   void setupBGClasses(const char* str, Fl_Color **pp_bgClassPixel, unsigned char **pp_bgClass, Fl_Color bgPixelDefault);
   void setLineNumberArea(int lineNumLeft, int lineNumWidth, int textLeft);
   void maintainAbsLineNum(int state);
   int posOfPreferredCol(int column, int lineStartPos);
   int preferredColumn(int *visLineNum, int *lineStartPos);

   void bufPreDeleteCB(int pos, int nDeleted);
   void bufModifiedCB(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText);
   void hScrollCB();
   void vScrollCB();

protected:
   bool canRedraw; // Flag allowing draw only in draw method, using something in FLTK ?

   void draw();
   void resize(int x, int y, int w, int h);

   void computeTextAreaSize(int x, int y, int w, int h);

   void updateLineStarts(int pos, int charsInserted, int charsDeleted, int linesInserted, int linesDeleted, bool* scrolled);
   void offsetLineStarts(int newTopLineNum);
   void calcLineStarts(int startLine, int endLine);
   void calcLastChar();
   bool posToVisibleLineNum(int pos, int* lineNum) const;
   void redisplayLine(int visLineNum, int leftClip, int rightClip, int leftCharIndex, int rightCharIndex);
   void drawString(int style, int x, int y, int toX, const char* string, int nChars);
   void clearRect(Fl_Color color, int x, int y,  int width, int height);
   void drawCursor(int x, int y);
   int styleOfPos(int lineStartPos, int lineLen, int lineIndex, int dispIndex, int thisChar);
   int stringWidth(const char* string, const int length, const int style) const;
   bool inSelection(Ne_Text_Selection* sel, int pos, int lineStartPos, int dispIndex) const;
   int xyToPos(int x, int y, NePositionTypes posType);
   void xyToUnconstrainedPos(int x, int y, int* row, int* column, NePositionTypes posType);
   void setScroll(int topLineNum, int horizOffset, bool updateVScrollBar, bool updateHScrollBar);
   void redrawLineNumbers(int clearAll);
   void updateVScrollBarRange();
   bool updateHScrollBarRange();
   int countLines(const char* string);
   int measureVisLine(int visLineNum);
   bool emptyLinesVisible() const;
   void blankCursorProtrusions();
   int visLineLength(int visLineNum);
   void measureDeletedLines(int pos, int nDeleted);
   void findWrapRange(const char* deletedText, int pos, int nInserted, int nDeleted, int* modRangeStart, int* modRangeEnd, int* linesInserted, int* linesDeleted);
   void wrappedLineCounter(const Ne_Text_Buffer* buf, const int startPos, const int maxPos, const int maxLines, const bool startPosIsLineStart, const int styleBufOffset, int* retPos, int* retLines, int* retLineStart, int* retLineEnd);
   void findLineEnd(int startPos, bool startPosIsLineStart, int* lineEnd, int* nextLineStart);
   int wrapUsesCharacter(int lineEndPos);
   void hideOrShowHScrollBar();
   void extendRangeForStyleMods(int* start, int* end);
   int getAbsTopLineNum();
   void offsetAbsLineNum(int oldFirstChar);
   int maintainingAbsTopLineNum();
   void resetAbsLineNum();
   int measurePropChar(const char c, const int colNum, const int pos);
   Fl_Color allocBGColor(const char* colorName, int* ok);
   Fl_Color getRangesetColor(int ind, Fl_Color bground);
   void redisplayRange(int start, int end);

};

#endif // Ne_Text_Display_h__
