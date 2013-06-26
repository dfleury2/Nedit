#ifndef Ne_Text_Part_h__
#define Ne_Text_Part_h__

class Ne_Text_Buffer;
class Ne_Text_Display;

#include "../util/Ne_Font.h"

#include <FL/Fl_Scrollbar.H>

#include <string>

struct Ne_Text_Part
{
   Ne_Text_Part();

   // resources
   //Fl_Color selectFGPixel, selectBGPixel, highlightFGPixel, highlightBGPixel;
   //Fl_Color cursorFGPixel, lineNumFGPixel, calltipFGPixel, calltipBGPixel;
   //Ne_Font fontStruct;
   //bool pendingDelete;
   //bool autoShowInsertPos;
   //bool autoWrap;
   //bool autoWrapPastedText;
   //bool continuousWrap;
   //bool autoIndent;
   //bool smartIndent;
   //bool overstrike;
   //bool heavyCursor;
   //bool readOnly;
   bool hidePointer;
   //int rows, columns;
   //int marginWidth, marginHeight;
   double cursorBlinkRate;
   //int wrapMargin;
   //int emulateTabs;
   //int lineNumCols;
   //std::string delimiters;
   //int cursorVPadding;
   //Fl_Widget* hScrollBar;
   //Fl_Widget* vScrollBar;
// TODO:    XtCallbackList focusInCB;
// TODO:    XtCallbackList focusOutCB;
// TODO:    XtCallbackList cursorCB;
// TODO:    XtCallbackList dragStartCB;
// TODO:    XtCallbackList dragEndCB;
// TODO:    XtCallbackList smartIndentCB;
   
   // private state
   //Ne_Text_Display* textD;    /* Pointer to display information */
   //int anchor, rectAnchor;	   /* Anchors for drag operations and rectangular drag operations */
   //int dragState;			      /* Why is the mouse being dragged and what is being acquired */
   //int multiClickState;       /* How long is this multi-click sequence so far */
   //int btnDownX, btnDownY;		/* Mark the position of last btn down action for deciding when to begin
   //                              paying attention to motion actions, and where to paste columns */
// TODO:    Time lastBtnDown;			   /* Timestamp of last button down event for multi-click recognition */
   //int mouseX, mouseY;			/* Last known mouse position in drag operation (for autoscroll) */
   int selectionOwner;			/* true if widget owns the selection */
   int motifDestOwner;			/* " " owns the motif destination */
   //int emTabsBeforeCursor;		/* If non-zero, number of consecutive emulated tabs just entered.  Saved so chars can be deleted as a unit */
// TODO:    XtIntervalId autoScrollProcID;	/* id of Xt timer proc for autoscroll */
// TODO:    XtIntervalId cursorBlinkProcID;	/* id of timer proc for cursor blink */
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
   std::string backlightCharTypes;  /* background class string to parse */
};

#endif
