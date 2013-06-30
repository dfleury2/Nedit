#ifndef Ne_Text_Editor_h__
#define Ne_Text_Editor_h__

#include "Ne_Text_Display.h"

#include <string>

// Resource strings
#define textNfont "font"
#define textNrows "rows"
#define textNcolumns "columns"
#define textNmarginWidth "marginWidth"
#define textNmarginHeight "marginHeight"
#define textNselectForeground "selectForeground"
#define textNselectBackground "selectBackground"
#define textNhighlightForeground "highlightForeground"
#define textNhighlightBackground "highlightBackground"
#define textNcursorForeground "cursorForeground"
#define textNlineNumForeground "lineNumForeground"
#define textNcalltipForeground "calltipForeground"
#define textNcalltipBackground "calltipBackground"
#define textNpendingDelete "pendingDelete"
#define textNhScrollBar "hScrollBar"
#define textNvScrollBar "vScrollBar"
#define textNlineNumCols "lineNumCols"
#define textNautoShowInsertPos "autoShowInsertPos"
#define textNautoWrapPastedText "autoWrapPastedText"
#define textNwordDelimiters "wordDelimiters"
#define textNblinkRate "blinkRate"
#define textNfocusCallback "focusCallback"
#define textNlosingFocusCallback "losingFocusCallback"
#define textNcursorMovementCallback "cursorMovementCallback"
#define textNdragStartCallback "dragStartCallback"
#define textNdragEndCallback "dragEndCallback"
#define textNsmartIndentCallback "smartIndentCallback"
#define textNautoWrap "autoWrap"
#define textNcontinuousWrap "continuousWrap"
#define textNwrapMargin "wrapMargin"
#define textNautoIndent "autoIndent"
#define textNsmartIndent "smartIndent"
#define textNoverstrike "overstrike"
#define textNheavyCursor "heavyCursor"
#define textNreadOnly "readOnly"
#define textNhidePointer "hidePointer"
#define textNemulateTabs "emulateTabs"
#define textNcursorVPadding "cursorVPadding"
#define textNbacklightCharTypes "backlightCharTypes"

// --------------------------------------------------------------------------
struct NeDragEndCBStruct
{
   int startPos;
   int nCharsDeleted;
   int nCharsInserted;
   std::string deletedText;
} ;

// --------------------------------------------------------------------------
enum NeSmartIndentCallbackReasons {NE_NEWLINE_INDENT_NEEDED, NE_CHAR_TYPED};

// --------------------------------------------------------------------------
struct NeSmartIndentCBStruct
{
   NeSmartIndentCallbackReasons reason;
   int pos;
   int indentRequest;
   char *charsTyped;
} ;

// --------------------------------------------------------------------------
class Ne_Text_Editor : public Ne_Text_Display
{
public:
   // Complete constructor...
   static Ne_Text_Editor* Create(int x, int y, int w, int h,
      int lineNumWidth, Ne_Text_Buffer* buffer,
      const Ne_Font& font,
      Fl_Color bgPixel, Fl_Color fgPixel,
      Fl_Color selectFGPixel, Fl_Color selectBGPixel, Fl_Color highlightFGPixel,
      Fl_Color highlightBGPixel, Fl_Color cursorFGPixel, Fl_Color lineNumFGPixel,
      bool continuousWrap, int wrapMargin,
      Fl_Color calltipFGPixel, Fl_Color calltipBGPixel);

   Ne_Text_Editor(int x, int y, int w, int h, const char* l = 0 );

   int getCursorPos();
   void setCursorPos(int pos);
// TODO: void TextPasteClipboard(Fl_Widget* w, Time time);
// TODO: void TextColPasteClipboard(Fl_Widget* w, Time time);
// TODO: void TextCopyClipboard(Fl_Widget* w, Time time);
// TODO: void TextCutClipboard(Fl_Widget* w, Time time);
   int firstVisibleLine();
   int numVisibleLines();
   int visibleWidth();
   void insertAtCursor(const char *chars, int event, int allowPendingDelete, int allowWrap);
   int firstVisiblePos();
   int lastVisiblePos();
// TODO: char *TextGetWrapped(Fl_Widget* w, int startPos, int endPos, int *length);
// TODO: XtActionsRec *TextGetActions(int *nActions);
// TODO: void ShowHidePointer(TextWidget w, bool hidePointer);
// TODO: void ResetCursorBlink(TextWidget textWidget, bool startsBlanked);

   int handle(int e);

// Everything should be protected, but too lazy to plug callback into the class...
// so CB has access to every method of this class
   
   int handle_key_button(int event);
// TODO: static void initialize(TextWidget request, TextWidget new);
// TODO: static void handleHidePointer(Fl_Widget* w, XtPointer unused, int event, bool *continue_to_dispatch);
// TODO: static void handleShowPointer(Fl_Widget* w, XtPointer unused, int event, bool *continue_to_dispatch);
// TODO: static void redisplay(TextWidget w, int event, Region region);
// TODO: static void redisplayGE(TextWidget w, XtPointer client_data, int event, bool *continue_to_dispatch_return);
// TODO: static void destroy(TextWidget w);
// TODO: static void resize(TextWidget w);
// TODO: static bool setValues(TextWidget current, TextWidget request, TextWidget new);
// TODO: static void realize(Fl_Widget* w, XtValueMask *valueMask, XSetWindowAttributes *attributes);
// TODO: static XtGeometryResult queryGeometry(Fl_Widget* w, XtWidgetGeometry *proposed, XtWidgetGeometry *answer);
   void checkMoveSelectionChange(int event, int startPos, const char**args, int *nArgs);
   void keyMoveExtendSelection(int event, int startPos, int rectangular);
   void checkAutoShowInsertPos();
   bool checkReadOnly() const;
   void simpleInsertAtCursor(const char *chars, int event, int allowPendingDelete);
   bool pendingSelection();
   bool deletePendingSelection(int event);
   bool deleteEmulatedTab(int event);
   void selectWord(int pointerX);
   bool spanForward(Ne_Text_Buffer *buf, int startPos, const char *searchChars,int ignoreSpace, int *foundPos);
   bool spanBackward(Ne_Text_Buffer *buf, int startPos, const char *searchChars, int ignoreSpace, int *foundPos);
   void selectLine();
   int startOfWord(int pos);
   int endOfWord(int pos);
   void checkAutoScroll(int x, int y);
   void endDrag();
   void cancelDrag();
   void callCursorMovementCBs(int event);
   void adjustSelection(int x, int y);
   void adjustSecondarySelection(int x, int y);
// TODO: static void autoScrollTimerProc(XtPointer clientData, XtIntervalId *id);
   std::string wrapText(const char *startLine, const char *text, int bufOffset, int wrapMargin, int *breakBefore);
   bool wrapLine(Ne_Text_Buffer *buf, int bufOffset, int lineStartPos, int lineEndPos, int limitPos, int *breakAt, int *charsAdded);
   char *createIndentString(Ne_Text_Buffer *buf, int bufOffset, int lineStartPos, int lineEndPos, int *length, int *column);
// TODO: static void cursorBlinkTimerProc(XtPointer clientData, XtIntervalId *id);
   bool inWindow(int x, int y);

   bool mouseMoveForDrag(int x, int y);

   double MultiClickTime;
private:

};

#endif // Ne_Text_Editor_h__
