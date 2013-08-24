#ifndef NEDIT_TEXTP_H_INCLUDED
#define NEDIT_TEXTP_H_INCLUDED

#include "../util/Ne_Font.h"

#include <FL/Fl.H>
#include <FL/Fl_Scrollbar.H>

#include <string>

struct Ne_Text_Buffer;
class Ne_Text_Display;

enum NeDragStates
{
   NOT_CLICKED,
   PRIMARY_CLICKED,
   SECONDARY_CLICKED,
   CLICKED_IN_SELECTION,
   PRIMARY_DRAG,
   PRIMARY_RECT_DRAG,
   SECONDARY_DRAG,
   SECONDARY_RECT_DRAG,
   PRIMARY_BLOCK_DRAG,
   DRAG_CANCELED,
   MOUSE_PAN
};

enum NeMultiClickStates
{
   NO_CLICKS,
   ONE_CLICK,
   TWO_CLICKS,
   THREE_CLICKS
};

struct Ne_Text_Part
{
   Ne_Text_Part(Ne_Text_Display* textDisplay);

   // resources
   bool pendingDelete;
   bool autoShowInsertPos;
   bool autoWrap;
   bool autoWrapPastedText;
   bool continuousWrap;
   bool autoIndent;
   bool smartIndent;
   bool overstrike;
   bool heavyCursor;
   bool readOnly;
   bool hidePointer;
   int lineNumCols;
   int wrapMargin;
   int emulateTabs;
   std::string delimiters;
   int cursorVPadding;

   // TODO:    XtCallbackList focusInCB;
   // TODO:    XtCallbackList focusOutCB;
   // TODO:    XtCallbackList cursorCB;
   // TODO:    XtCallbackList dragStartCB;
   // TODO:    XtCallbackList dragEndCB;
   // TODO:    XtCallbackList smartIndentCB;

   // private state
   Ne_Text_Display* textD;          /* Pointer to display information */
   int anchor, rectAnchor;          /* Anchors for drag operations and rectangular drag operations */
   int dragState;                   /* Why is the mouse being dragged and what is being acquired */
   int multiClickState;             /* How long is this multi-click sequence so far */
   int btnDownX, btnDownY;          /* Mark the position of last btn down action for deciding when to begin
                                       paying attention to motion actions, and where to paste columns */
   double lastBtnDown;              /* Timestamp of last button down event for multi-click recognition */
   int mouseX, mouseY;              /* Last known mouse position in drag operation (for autoscroll) */
   int selectionOwner;              /* True if widget owns the selection */
   int motifDestOwner;              /* " " owns the motif destination */
   int emTabsBeforeCursor;          /* If non-zero, number of consecutive emulated tabs just entered.  Saved so chars can be deleted as a unit */
   int autoScrollProcID;            /* id of Xt timer proc for autoscroll */
   int cursorBlinkProcID;           /* id of timer proc for cursor blink */
   Ne_Text_Buffer* dragOrigBuf;     /* backup buffer copy used during block dragging of selections */
   int dragXOffset, dragYOffset;    /* offsets between cursor location and actual insertion point in drag */
   int dragType;                    /* style of block drag operation */
   int dragInsertPos;               /* location where text being block dragged was last inserted */
   int dragRectStart;               /* rect. offset "" */
   int dragInserted;                /* # of characters inserted at drag destination in last drag position */
   int dragDeleted;                 /* # of characters deleted "" */
   int dragSourceDeletePos;         /* location from which move source text was removed at start of drag */
   int dragSourceInserted;          /* # of chars. inserted when move source text was deleted */
   int dragSourceDeleted;           /* # of chars. deleted "" */
   int dragNLines;                  /* # of newlines in text being drag'd */
   std::string backlightCharTypes;  /* background class string to parse */
   bool isColumnar;                 /* isColumnar paste */
};

#endif /* NEDIT_TEXTP_H_INCLUDED */
