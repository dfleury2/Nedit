#ifndef NEDIT_TEXTDRAG_H_INCLUDED
#define NEDIT_TEXTDRAG_H_INCLUDED

#include "Ne_Text_Editor.h"

enum NeBlockDragTypes {USE_LAST, DRAG_COPY, DRAG_MOVE, DRAG_OVERLAY_MOVE,
                     DRAG_OVERLAY_COPY
                    };

void BeginBlockDrag(Ne_Text_Editor* textD);
void BlockDragSelection(Ne_Text_Editor* textD, int x, int y, int dragType);
void FinishBlockDrag(Ne_Text_Editor* textD);
void CancelBlockDrag(Ne_Text_Editor* textD);

#endif /* NEDIT_TEXTDRAG_H_INCLUDED */
