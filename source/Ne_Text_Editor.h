#ifndef NEDIT_TEXT_H_INCLUDED
#define NEDIT_TEXT_H_INCLUDED

#include "Ne_Text_Buffer.h"
#include "Ne_Text_Display.h"

// Resource strings
#define textNfont "font"
#define textCFont "Font"
#define textNrows "rows"
#define textCRows "Rows"
#define textNcolumns "columns"
#define textCColumns "Columns"
#define textNmarginWidth "marginWidth"
#define textCMarginWidth "MarginWidth"
#define textNmarginHeight "marginHeight"
#define textCMarginHeight "MarginHeight"
#define textNselectForeground "selectForeground"
#define textCSelectForeground "SelectForeground"
#define textNselectBackground "selectBackground"
#define textCSelectBackground "SelectBackground"
#define textNhighlightForeground "highlightForeground"
#define textCHighlightForeground "HighlightForeground"
#define textNhighlightBackground "highlightBackground"
#define textCHighlightBackground "HighlightBackground"
#define textNcursorForeground "cursorForeground"
#define textCCursorForeground "CursorForeground"
#define textNlineNumForeground "lineNumForeground"
#define textCLineNumForeground "LineNumForeground"
#define textNcalltipForeground "calltipForeground"
#define textCcalltipForeground "CalltipForeground"
#define textNcalltipBackground "calltipBackground"
#define textCcalltipBackground "CalltipBackground"
#define textNpendingDelete "pendingDelete"
#define textCPendingDelete "PendingDelete"
#define textNhScrollBar "hScrollBar"
#define textCHScrollBar "HScrollBar"
#define textNvScrollBar "vScrollBar"
#define textCVScrollBar "VScrollBar"
#define textNlineNumCols "lineNumCols"
#define textCLineNumCols "LineNumCols"
#define textNautoShowInsertPos "autoShowInsertPos"
#define textCAutoShowInsertPos "AutoShowInsertPos"
#define textNautoWrapPastedText "autoWrapPastedText"
#define textCAutoWrapPastedText "AutoWrapPastedText"
#define textNwordDelimiters "wordDelimiters"
#define textCWordDelimiters "WordDelimiters"
#define textNblinkRate "blinkRate"
#define textCBlinkRate "BlinkRate"
#define textNfocusCallback "focusCallback"
#define textCFocusCallback "FocusCallback"
#define textNlosingFocusCallback "losingFocusCallback"
#define textCLosingFocusCallback "LosingFocusCallback"
#define textNcursorMovementCallback "cursorMovementCallback"
#define textCCursorMovementCallback "CursorMovementCallback"
#define textNdragStartCallback "dragStartCallback"
#define textCDragStartCallback "DragStartCallback"
#define textNdragEndCallback "dragEndCallback"
#define textCDragEndCallback "DragEndCallback"
#define textNsmartIndentCallback "smartIndentCallback"
#define textCSmartIndentCallback "SmartIndentCallback"
#define textNautoWrap "autoWrap"
#define textCAutoWrap "AutoWrap"
#define textNcontinuousWrap "continuousWrap"
#define textCContinuousWrap "ContinuousWrap"
#define textNwrapMargin "wrapMargin"
#define textCWrapMargin "WrapMargin"
#define textNautoIndent "autoIndent"
#define textCAutoIndent "AutoIndent"
#define textNsmartIndent "smartIndent"
#define textCSmartIndent "SmartIndent"
#define textNoverstrike "overstrike"
#define textCOverstrike "Overstrike"
#define textNheavyCursor "heavyCursor"
#define textCHeavyCursor "HeavyCursor"
#define textNreadOnly "readOnly"
#define textCReadOnly "ReadOnly"
#define textNhidePointer "hidePointer"
#define textCHidePointer "HidePointer"
#define textNemulateTabs "emulateTabs"
#define textCEmulateTabs "EmulateTabs"
#define textNcursorVPadding "cursorVPadding"
#define textCCursorVPadding "CursorVPadding"
#define textNbacklightCharTypes "backlightCharTypes"
#define textCBacklightCharTypes "BacklightCharTypes"


// TODO: extern WidgetClass textWidgetClass;
// TODO: struct _TextClassRec;
// TODO: struct _TextRec;
// TODO: 
// TODO: typedef struct _TextRec* TextWidget;

struct dragEndCBStruct
{
   int startPos;
   int nCharsDeleted;
   int nCharsInserted;
   char* deletedText;
};

enum smartIndentCallbackReasons {NEWLINE_INDENT_NEEDED, CHAR_TYPED};
struct smartIndentCBStruct
{
   int reason;
   int pos;
   int indentRequest;
   char* charsTyped;
};

// --------------------------------------------------------------------------
class Ne_Text_Editor : public Ne_Text_Display
{
public:
   Ne_Text_Editor(int x, int y, int w, int h, const char* l = 0 );
   
   int handle(int e);
   int handle_key_button(int event);
   bool inWindow(int x, int y) const;
   bool mouseMoveForDrag(int x, int y) const;
   double MultiClickTime;
};


// -------------------------------------------------------------------------------
void TextInitialize(Ne_Text_Editor* textD);

// -------------------------------------------------------------------------------
Ne_Text_Editor* TextCreate(int x, int y, int w, int h,
                      int lineNumWidth, Ne_Text_Buffer* buffer,
                      const Ne_Font& fontStruct, Fl_Color bgPixel, Fl_Color fgPixel,
                      Fl_Color selectFGPixel, Fl_Color selectBGPixel, Fl_Color highlightFGPixel,
                      Fl_Color highlightBGPixel, Fl_Color cursorFGPixel, Fl_Color lineNumFGPixel,
                      int continuousWrap, int wrapMargin, const char* bgClassString,
                      Fl_Color calltipFGPixel, Fl_Color calltipBGPixel);


/* User callable routines */
void TextSetBuffer(Ne_Text_Editor* textD, Ne_Text_Buffer* buffer);
Ne_Text_Buffer* TextGetBuffer(Ne_Text_Editor* textD);
int TextLineAndColToPos(Ne_Text_Editor* textD, int lineNum, int column);
int TextPosToLineAndCol(Ne_Text_Editor* textD, int pos, int* lineNum, int* column);
int TextPosToXY(Ne_Text_Editor* textD, int pos, int* x, int* y);
int TextGetCursorPos(Ne_Text_Editor* textD);
void TextSetCursorPos(Ne_Text_Editor* textD, int pos);
void TextGetScroll(Ne_Text_Editor* textD, int* topLineNum, int* horizOffset);
void TextSetScroll(Ne_Text_Editor* textD, int topLineNum, int horizOffset);
int TextGetMinFontWidth(Ne_Text_Editor* textD, bool considerStyles);
int TextGetMaxFontWidth(Ne_Text_Editor* textD, bool considerStyles);
// TODO: void TextHandleXSelections(Ne_Text_Editor* textD);
void TextPasteClipboard(Ne_Text_Editor* textD, double time);
void TextColPasteClipboard(Ne_Text_Editor* textD, double time);
void TextCopyClipboard(Ne_Text_Editor* textD, double time);
void TextCutClipboard(Ne_Text_Editor* textD, double time);
int TextFirstVisibleLine(Ne_Text_Editor* textD);
int TextNumVisibleLines(Ne_Text_Editor* textD);
int TextVisibleWidth(Ne_Text_Editor* textD);
void TextInsertAtCursor(Ne_Text_Editor* textD, char* chars, int event, int allowPendingDelete, int allowWrap);
int TextFirstVisiblePos(Ne_Text_Editor* textD);
int TextLastVisiblePos(Ne_Text_Editor* textD);
char* TextGetWrapped(Ne_Text_Editor* textD, int startPos, int endPos, int* length);
XtActionsRec* TextGetActions(int* nActions);
// TODO: void ShowHidePointer(TextWidget w, bool hidePointer);
// TODO: void ResetCursorBlink(TextWidget textWidget, bool startsBlanked);
bool TextSetValues(Ne_Text_Editor* current); // No delta, Ne_Text_Editor* request, Ne_Text_Editor* newWidget);

// TODO: void HandleAllPendingGraphicsExposeNoExposeEvents(TextWidget w, int event);

#endif /* NEDIT_TEXT_H_INCLUDED */
