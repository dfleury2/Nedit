#ifndef NEDIT_TEXTDISP_H_INCLUDED
#define NEDIT_TEXTDISP_H_INCLUDED

#include "Ne_Text_Buffer.h"
#include "Ne_Text_Part.h"

#include "../util/Ne_Font.h"
#include "../util/misc.h"

#include <FL/Fl_Group.H>
#include <FL/Fl_Scrollbar.H>

enum NeCursorStyles
{
   NE_NORMAL_CURSOR,
   NE_CARET_CURSOR,
   NE_DIM_CURSOR,
   NE_BLOCK_CURSOR,
   NE_HEAVY_CURSOR
};

#define NO_HINT -1

struct NeStyleTableEntry
{
   char* highlightName;
   char* styleName;
   char* colorName;
   char isBold;
   char isItalic;
   unsigned short red;
   unsigned short green;
   unsigned short blue;
   Fl_Color color;
   bool underline;
   Ne_Font font;
   char* bgColorName;      /* background style coloring (name may be NULL) */
   unsigned short bgRed;
   unsigned short bgGreen;
   unsigned short bgBlue;
   Fl_Color bgColor;
};

class Ne_Text_Display;

typedef void (*unfinishedStyleCBProc)(Ne_Text_Display*, int, void*);

struct calltipStruct
{
   int ID;                 /* ID of displayed calltip.  Equals zero if none is displayed. */
   bool anchored;          /* Is it anchored to a position */
   int pos;                /* int tip is anchored to */
   int hAlign;             /* horizontal alignment */
   int vAlign;             /* vertical alignment */
   int alignMode;          /* Strict or sloppy alignment */
} ;

class Ne_Text_Display : public Fl_Group
{
public:
   Ne_Text_Display(int x, int y, int w, int h, const char* l = 0);
   ~Ne_Text_Display();

   void draw();
   void resize(int x, int y, int w, int h);

   bool canRedraw;
   void computeTextAreaSize(int x, int y, int w, int h);

   int rows() const;    // # of rows in the text area
   int columns() const; // # of columns in the text area
   bool isLineNumberAreaRequired() const { return (lineNumWidth != 0); }

   Ne_Text_Part text;
   
   // Text Drawing Area Only
   int top, left, width, height;    
   // Line Number Area
   int lineNumLeft, lineNumWidth;
   int lineNumCols;
   // Margins between borders, and line number area and text drawing area
   int marginWidth, marginHeight;

   int cursorPos;
   bool cursorOn;
   int cursorX, cursorY;   /* X, Y pos. of last drawn cursor
                           Note: these are used for *drawing*
                           and are not generally reliable
                           for finding the insert position's
                           x/y coordinates! */
   int cursorToHint;       /* Tells the buffer modified callback where to move the cursor, to reduce the number of redraw calls */
   int cursorStyle;        /* One of enum cursorStyles above */
   int cursorPreferredCol; /* Column for vert. cursor movement */
   int cursorBlinkRate;
   
   int nVisibleLines;      /* # of visible (displayed) lines */
   int nBufferLines;			/* # of newlines in the buffer */
   Ne_Text_Buffer* buffer; /* Contains text to be displayed */
   Ne_Text_Buffer* styleBuffer;  /* Optional parallel buffer containing color and font information */
   int firstChar, lastChar;		/* Buffer positions of first and last displayed character (lastChar points
    					                  either to a newline or one character beyond the end of the buffer) */
   bool continuousWrap;    /* Wrap long lines when displaying */
   int wrapMargin;         /* Margin in # of char positions for wrapping in continuousWrap mode */
   int* lineStarts;
   int topLineNum;			/* Line number of top displayed line of file (first line of file is 1) */
   int absTopLineNum;      /* In continuous wrap mode, the line number of the top line if the text
                              were not wrapped (note that this is only maintained as needed). */
   int needAbsTopLineNum;		/* Externally settable flag to continue
    					   maintaining absTopLineNum even if
					   it isn't needed for line # display */
   int horizOffset;			   /* Horizontal scroll pos. in pixels */
   int visibility;            /* Window visibility (see XVisibility event) */
   int nStyles;			      /* Number of entries in styleTable */
   NeStyleTableEntry* styleTable;    	/* Table of fonts and colors for coloring/syntax-highlighting */
   char unfinishedStyle;      /* Style buffer entry which triggers on-the-fly reparsing of region */
   unfinishedStyleCBProc unfinishedHighlightCB;		/* Callback to parse "unfinished" regions */
   void* highlightCBArg;      /* Arg to unfinishedHighlightCB */
   Ne_Font primaryFont;		   /* Font structure for primary font */
   int ascent, descent;		   /* Composite ascent and descent for primary font + all-highlight fonts */
   int fixedFontWidth;			/* Font width if all current fonts are fixed and match in width, else -1 */
   Fl_Scrollbar* hScrollBar;
   Fl_Scrollbar* vScrollBar;
   
   // Foreground/Background colors
   Fl_Color fgPixel, bgPixel; 
   // Foreground/Background select color
   Fl_Color selectFGPixel, selectBGPixel;
   // Highlight colors are used when flashing matching parens
   Fl_Color highlightFGPixel, highlightBGPixel;
   // Color for drawing line numbers
   Fl_Color lineNumFGPixel;
   // Foreground cursor color
   Fl_Color cursorFGPixel;
   Fl_Color* bgClassPixel;		/* table of colors for each BG class */
   unsigned char* bgClass;		/* obtains index into bgClassPixel[] */

   Fl_Widget* calltipW;       /* The Label widget for the calltip */
   Fl_Widget* calltipShell;   /* The Shell that holds the calltip */
   calltipStruct calltip;     /* The info for the calltip itself */
   // Foreground/Background calltip color
   Fl_Color calltipFGPixel, calltipBGPixel;
   int suppressResync;			/* Suppress resynchronization of line starts during buffer updates */
   int nLinesDeleted;			/* Number of lines deleted during buffer modification (only used when resynchronization is suppressed) */
   int modifyingTabDist;		/* Whether tab distance is being modified */
   bool pointerHidden;        /* true if the mouse pointer is hidden */
   std::string delimiters;
   int emulateTabs;
};

Ne_Text_Display* TextDCreate( int x, int y, int w, int h,
                      int lineNumWidth, Ne_Text_Buffer* buffer,
                      const Ne_Font& fontStruct, Fl_Color bgPixel, Fl_Color fgPixel,
                      Fl_Color selectFGPixel, Fl_Color selectBGPixel, Fl_Color highlightFGPixel,
                      Fl_Color highlightBGPixel, Fl_Color cursorFGPixel, Fl_Color lineNumFGPixel,
                      int continuousWrap, int wrapMargin, const char* bgClassString,
                      Fl_Color calltipFGPixel, Fl_Color calltipBGPixel);
void TextDFree(Ne_Text_Display* textD);
void TextDSetBuffer(Ne_Text_Display* textD, Ne_Text_Buffer* buffer);
void TextDAttachHighlightData(Ne_Text_Display* textD, Ne_Text_Buffer* styleBuffer,
                              NeStyleTableEntry* styleTable, int nStyles, char unfinishedStyle,
                              unfinishedStyleCBProc unfinishedHighlightCB, void* cbArg);
void TextDSetColors(Ne_Text_Display* textD, Fl_Color textFgP, Fl_Color textBgP,
                    Fl_Color selectFgP, Fl_Color selectBgP, Fl_Color hiliteFgP, Fl_Color hiliteBgP,
                    Fl_Color lineNoFgP, Fl_Color cursorFgP);
void TextDSetFont(Ne_Text_Display* textD, const Ne_Font& fontStruct);
int TextDMinFontWidth(Ne_Text_Display* textD, bool considerStyles);
int TextDMaxFontWidth(Ne_Text_Display* textD, bool considerStyles);
void TextDResize(Ne_Text_Display* textD, int width, int height);
void TextDRedisplayRect(Ne_Text_Display* textD, int left, int top, int width, int height);
void TextDSetScroll(Ne_Text_Display* textD, int topLineNum, int horizOffset);
void TextDGetScroll(Ne_Text_Display* textD, int* topLineNum, int* horizOffset);
void TextDInsert(Ne_Text_Display* textD, char* text);
void TextDOverstrike(Ne_Text_Display* textD, char* text);
void TextDSetInsertPosition(Ne_Text_Display* textD, int newPos);
int TextDGetInsertPosition(Ne_Text_Display* textD);
int TextDXYToPosition(Ne_Text_Display* textD, int x, int y);
int TextDXYToCharPos(Ne_Text_Display* textD, int x, int y);
void TextDXYToUnconstrainedPosition(Ne_Text_Display* textD, int x, int y, int* row, int* column);
int TextDLineAndColToPos(Ne_Text_Display* textD, int lineNum, int column);
int TextDOffsetWrappedColumn(Ne_Text_Display* textD, int row, int column);
int TextDOffsetWrappedRow(Ne_Text_Display* textD, int row);
int TextDPositionToXY(Ne_Text_Display* textD, int pos, int* x, int* y);
int TextDPosToLineAndCol(Ne_Text_Display* textD, int pos, int* lineNum, int* column);
int TextDInSelection(Ne_Text_Display* textD, int x, int y);
void TextDMakeInsertPosVisible(Ne_Text_Display* textD);
int TextDMoveRight(Ne_Text_Display* textD);
int TextDMoveLeft(Ne_Text_Display* textD);
int TextDMoveUp(Ne_Text_Display* textD, int absolute);
int TextDMoveDown(Ne_Text_Display* textD, int absolute);
void TextDBlankCursor(Ne_Text_Display* textD);
void TextDUnblankCursor(Ne_Text_Display* textD);
void TextDSetCursorStyle(Ne_Text_Display* textD, int style);
void TextDSetWrapMode(Ne_Text_Display* textD, int wrap, int wrapMargin);
int TextDEndOfLine(const Ne_Text_Display* textD, const int pos, const bool startPosIsLineStart);
int TextDStartOfLine(const Ne_Text_Display* textD, const int pos);
int TextDCountForwardNLines(const Ne_Text_Display* textD, const int startPos, const unsigned nLines, const bool startPosIsLineStart);
int TextDCountBackwardNLines(Ne_Text_Display* textD, int startPos, int nLines);
int TextDCountLines(Ne_Text_Display* textD, int startPos, int endPos, int startPosIsLineStart);
void TextDSetupBGClasses(Fl_Widget* w, const char* str, Fl_Color** pp_bgClassPixel, unsigned char** pp_bgClass, Fl_Color bgPixelDefault);
void TextDSetLineNumberArea(Ne_Text_Display* textD, int lineNumLeft, int lineNumWidth, int textLeft);
void TextDMaintainAbsLineNum(Ne_Text_Display* textD, int state);
int TextDPosOfPreferredCol(Ne_Text_Display* textD, int column, int lineStartPos);
int TextDPreferredColumn(Ne_Text_Display* textD, int* visLineNum, int* lineStartPos);

#endif /* NEDIT_TEXTDISP_H_INCLUDED */
