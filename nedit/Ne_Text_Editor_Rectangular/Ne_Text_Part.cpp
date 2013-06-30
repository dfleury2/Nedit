#include "Ne_Text_Part.h"

#include "Ne_Text_Part.h"

#include "../util/Ne_Color.h"

#include <FL/Fl.H>

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

// --------------------------------------------------------------------------
Ne_Text_Part::Ne_Text_Part()
{
   //this->fontStruct = Ne_Font();
   this->selectBGPixel = GetColor(NEDIT_DEFAULT_SEL_FG);
   this->highlightFGPixel = GetColor(NEDIT_DEFAULT_HI_FG);
   this->highlightBGPixel = GetColor(NEDIT_DEFAULT_HI_BG);
   this->lineNumFGPixel = GetColor(NEDIT_DEFAULT_LINENO_FG);
   this->cursorFGPixel = GetColor(NEDIT_DEFAULT_CURSOR_FG);
   this->calltipFGPixel = GetColor(NEDIT_DEFAULT_CALLTIP_FG);
   this->calltipBGPixel = GetColor(NEDIT_DEFAULT_CALLTIP_BG);
   this->backlightCharTypes = "";
   this->rows = 24;
   this->columns = 80;
   this->marginWidth = 5;
   this->marginHeight = 5;
   this->pendingDelete = true;
   this->autoWrap = true;
   this->continuousWrap = true;
   this->autoIndent = true;
   this->smartIndent = false;
   this->overstrike = false;
   this->heavyCursor = false;
   this->readOnly = false;
   this->hidePointer = false;
   this->wrapMargin = 0;

   this->lineNumCols = 0;
   this->autoShowInsertPos = true;
   this->autoWrapPastedText = false;
   this->delimiters = ".,/\\`'!@#%^&*()-=+{}[]\":;<>?";
   this->cursorBlinkRate = 500;
   this->emulateTabs = 0;

   // TODO:    { textNfocusCallback, textCFocusCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.focusInCB), XtRCallback, NULL },
   // TODO:    { textNlosingFocusCallback, textCLosingFocusCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.focusOutCB), XtRCallback,NULL },
   // TODO:    { textNcursorMovementCallback, textCCursorMovementCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.cursorCB), XtRCallback, NULL },
   // TODO:    { textNdragStartCallback, textCDragStartCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.dragStartCB), XtRCallback, NULL },
   // TODO:    { textNdragEndCallback, textCDragEndCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.dragEndCB), XtRCallback, NULL },
   // TODO:    { textNsmartIndentCallback, textCSmartIndentCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.smartIndentCB), XtRCallback, NULL },

   anchor = rectAnchor = 0;
   dragState = 0;
   multiClickState = 0;
   btnDownX = btnDownY = 0;
   lastBtnDown = 0;
   mouseX = mouseY = 0;
   selectionOwner = 0;
   motifDestOwner = 0;
   emTabsBeforeCursor = 0;
   autoScrollProcID = 0;
   this->cursorVPadding = 0;

   dragOrigBuf = NULL;
   dragXOffset = dragYOffset = 0;
   dragType = 0;
   dragInsertPos = 0;
   dragRectStart = 0;
   dragInserted = 0;
   dragDeleted = 0;
   dragSourceDeletePos = 0;
   dragSourceInserted = 0;
   dragSourceDeleted = 0;
   dragNLines = 0;
   this->isColumnar = false;
   this->cursorBlinkProcID = 0;
}
