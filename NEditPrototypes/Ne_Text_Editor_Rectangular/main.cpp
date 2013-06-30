#include "Ne_Text_Buffer.h"

#include "Ne_Text_Display.h"
#include "Ne_Text_Editor.h"

#include "../util/Ne_Color.h"
#include "../util/fontsel.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>

#include <fstream>
#include <string>

Ne_Text_Display* text = 0;

// --------------------------------------------------------------------------
void SetFontCB(Fl_Widget* w, void* data)
{
   std::string fontName = (data?(const char*)data:"");
   if (fontName.empty())
      fontName = FontSel(w, true, "", FL_BLACK, FL_WHITE);

   if (!fontName.empty())
      TextDSetFont(text, Ne_Font(fontName));
}

// --------------------------------------------------------------------------
int main(int argc, char* argv[])
{
   Ne_Text_Buffer* buffer = BufCreate();

   if ( argc > 1 )
   {
      // Ouverture d'un fichier pour le buffer;
      std::ifstream file(argv[1]);
      if (!file)
         return 0;

      file.seekg(0, std::ios_base::end);

      std::string filebuffer;
      filebuffer.resize(file.tellg());
      file.seekg(0, std::ios_base::beg);

      file.read(&filebuffer[0], filebuffer.size());
      BufSetAll(buffer, filebuffer.c_str());
   }

   //Ne_Window win(30,50,600,700, "Nedit Text Editor Test");
   Fl_Double_Window win(30,50,600,700, "Nedit Text Editor Test");
   Fl_Menu_Bar menuBar(0,0,600,25);
   menuBar.add("Set Font/Consolas 12", 0, SetFontCB, (void*)"Consolas 12");
   menuBar.add("Set Font/Consolas 14", 0, SetFontCB, (void*)"Consolas 14");
   menuBar.add("Set Font/Consolas 18", 0, SetFontCB, (void*)"Consolas 18");
   menuBar.add("Set Font/Bitstream Vera Sans 14", 0, SetFontCB, (void*)"Bitstream Vera Sans 14");
   menuBar.add("Set Font/MS Sans Serif 12", 0, SetFontCB, (void*)"MS Sans Serif 12", FL_MENU_DIVIDER);
   menuBar.add("Set Font/Choose...", 0, SetFontCB, 0);

   //Ne_Text_Editor* text = Ne_Text_Editor::Create(0, 25, win.w(), win.h()  -25,
   text = TextCreate(0, 25, win.w(), win.h()  -25,
      60,
      buffer,
      Ne_Font("Consolas 14"),  // textfont
      //Ne_Font("Fixedsys 9"),  // textfont
      GetColor("DodgerBlue4"), GetColor("AntiqueWhite"), // gbcolor, fgcolor
      GetColor("DodgerBlue3"), GetColor("black"), // select gbcolor, fgcolor
      GetColor("white"), GetColor("red"), // hilite bgcolor, fgcolor
      GetColor("AntiqueWhite"),   // cursor fgcolor
      GetColor("AntiqueWhite"),   // lineNo fgColor
      false, 0, 0, 
      GetColor("black"), GetColor("LemonChiffon1") // Calltip fgcolor bgcolor
      );

   //   nedit.textRows: 50
   //   nedit.textCols: 80
   //   nedit.tabDistance: 2
   //   nedit.emulateTabs: 2
   //   nedit.insertTabs: False
   //   nedit.boldHighlightFont: Consolas bold 14
   //   nedit.italicHighlightFont: Consolas italic 14
   //   nedit.boldItalicHighlightFont: Consolas bold italic 14

   win.resizable(text);

   win.show();

   return Fl::run();
}
