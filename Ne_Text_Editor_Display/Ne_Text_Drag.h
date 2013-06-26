#ifndef NEDIT_TEXTDRAG_H_INCLUDED
#define NEDIT_TEXTDRAG_H_INCLUDED

#include <FL/Fl_Widget.H>

enum NeBlockDragTypes
{
   NE_USE_LAST,
   NE_DRAG_COPY,
   NE_DRAG_MOVE,
   NE_DRAG_OVERLAY_MOVE,
   NE_DRAG_OVERLAY_COPY
};

void BeginBlockDrag(Fl_Widget*);
void BlockDragSelection(Fl_Widget*, int x, int y, int dragType);
void FinishBlockDrag(Fl_Widget*);
void CancelBlockDrag(Fl_Widget*);

#endif // NEDIT_TEXTDRAG_H_INCLUDED
