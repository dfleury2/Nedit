#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>

#include "../../util/fontsel.h"
#include "../../util/Ne_FontDescription.h"

#include "../../source/Ne_Text_Display.h"
#include "../../source/Ne_Text_Buffer.h"

#include <iostream>
#include <fstream>
#include <iterator>

using namespace std;

// --------------------------------------------------------------------------
int main()
{
   Ne_FontDescription::InitFonts();
   Ne_Font font;
   for(Ne_FontDescriptions::const_iterator f = Fonts.begin(); f != Fonts.end(); ++f)
   {
      if (f->name.find("Consolas") != std::string::npos
         || f->name.find("lucida") != std::string::npos)
      {
         font = f->getFont(12);
         break;
      }
   }

   ifstream file("main.cpp");
   std::string fileStr((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
   Ne_Text_Buffer* buffer = BufCreate();
   BufSetAll(buffer, fileStr.c_str());

   Fl_Double_Window win(30, 50, 600, 500, "Display Text");
 
   Ne_Text_Display* display = TextDCreate(0,0,600,500,
      60,
      buffer,
      font /* NE_Font */,
      FL_WHITE,
      FL_BLACK,
      FL_BLUE,
      FL_YELLOW,
      FL_GREEN,
      FL_RED, 
      FL_RED,
      FL_BLACK,
      0 /* continous Wrap */,
      0 /* wrap marging */,
      "",
      FL_BLACK,
      FL_YELLOW);

   display->buffer = buffer;
   //TextDResize(&display, 600,500);
   
   win.resizable(display);
   win.show();

   return Fl::run();
}
