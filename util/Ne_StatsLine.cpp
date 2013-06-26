#include "Ne_StatsLine.h"

// --------------------------------------------------------------------------
Ne_StatsLine::Ne_StatsLine(int x, int y, int w, int h, const char* l)
   : Fl_Group(x, y, w, h, l),
   statsLine_(x, y, w - 140, h),
   statsLineColNo_(x + w - 140, y, 130, h, "L: ---  C: ---")
{
   //window->statsLineForm->box(FL_THIN_DOWN_FRAME);

   // Create file statistics display area.
   //window->statsLine = new Fl_Box(0,0,window->mainWindow->w()-window->statsLineColNo->w(),20);
   statsLine_.align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);

   //= new Fl_Box(window->mainWindow->w()-140, 0, 140, 20, );
   statsLineColNo_.align(FL_ALIGN_RIGHT|FL_ALIGN_INSIDE);
   //window->statsLineColNo->labelfont(window->fontList.font);
   //window->statsLineColNo->labelsize(FL_NORMAL_SIZE - 1);

   end();
}

// --------------------------------------------------------------------------
void Ne_StatsLine::resize(int x, int y, int w, int h)
{
   statsLine_.resize(x, y, w - 140, h);
   statsLineColNo_.resize(x + w - 140, y, 130, h);

   Fl_Group::resize(x, y, w, h);
}
