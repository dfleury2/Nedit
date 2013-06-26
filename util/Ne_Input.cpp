#include "Ne_Input.h"

Ne_Input::Ne_Input(int X, int Y, int W, int H, const char* L) : Fl_Input(X, Y, W, H, L)
{}

int Ne_Input::handle(int e)
{
   switch (e)
   {
   //case FL_KEYUP:                      // key pressed/released?
   case FL_KEYDOWN:
      {
         switch (Fl::event_key())        // up/down keys?
         {
         case FL_Up:
         case FL_Down:
            this->do_callback();
            return 1;              // prevent Fl_Input from seeing them
         }
         break;
      }
   }
   return(Fl_Input::handle(e));          // pass all other events down to Fl_Input
}
