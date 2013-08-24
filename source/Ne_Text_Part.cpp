#include "Ne_Text_Part.h"

// --------------------------------------------------------------------------
Ne_Text_Part::Ne_Text_Part(Ne_Text_Display* textDisplay)
{
   this->backlightCharTypes = "";
   this->pendingDelete = true;
   this->autoWrap = true;
   this->autoIndent = true;
   this->smartIndent = false;
   this->overstrike = false;
   this->heavyCursor = false;
   this->readOnly = false;
   this->hidePointer = false;

   this->autoShowInsertPos = true;
   this->autoWrapPastedText = false;
   this->delimiters = ".,/\\`'!@#%^&*()-=+{}[]\":;<>?";
   this->emulateTabs = 0;

   // TODO:    { textNfocusCallback, textCFocusCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.focusInCB), XtRCallback, NULL },
   // TODO:    { textNlosingFocusCallback, textCLosingFocusCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.focusOutCB), XtRCallback,NULL },
   // TODO:    { textNcursorMovementCallback, textCCursorMovementCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.cursorCB), XtRCallback, NULL },
   // TODO:    { textNdragStartCallback, textCDragStartCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.dragStartCB), XtRCallback, NULL },
   // TODO:    { textNdragEndCallback, textCDragEndCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.dragEndCB), XtRCallback, NULL },
   // TODO:    { textNsmartIndentCallback, textCSmartIndentCallback, XmRCallback, sizeof(caddr_t), XtOffset(TextWidget, text.smartIndentCB), XtRCallback, NULL },

   this->textD = textDisplay;
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
