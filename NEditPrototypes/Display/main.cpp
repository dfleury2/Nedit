#include <FL/Fl.H>
#include <FL/Fl_Window.H>

#include "../../util/fontsel.h"
#include "../../util/Ne_FontDescription.h"

#include <iostream>

using namespace std;

int main()
{
   Fl_Window win(30, 50, 600, 500, "Font Description");
   win.show();

   Ne_FontDescription::InitFonts();

   Fl_Color fgPixel = fl_rgb_color(0,0,0);
   Fl_Color bgPixel = fl_rgb_color(255,255,255);

   FontSel(&win, ONLY_FIXED, 0, fgPixel, bgPixel);

   return Fl::run();
}
