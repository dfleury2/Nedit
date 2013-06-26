#include "Ne_FontDescription.h"

#include "Ne_Font.h"

// --------------------------------------------------------------------------
Ne_FontDescriptions Fonts;

void Ne_FontDescription::InitFonts()
{
   // Retrieve all fonts
   int numFonts = Fl::set_fonts("*");
   for( int i = 0; i < numFonts; ++i)
   {
      Ne_FontDescription font;
      font.name = Fl::get_font_name(i); // Expecting : name [bold] [italic]
      font.font = i;

      // Get font size
      int* sizeArray;
      int sizeCount = Fl::get_font_sizes(i, sizeArray);
      if (sizeCount)
      {  // Fixed Font
         for (int s = 0; s < sizeCount; ++s)
         {
            font.size = sizeArray[s];
            Fonts.push_back(font);
         }
      }
      else
         Fonts.push_back(font); // TTF

   }
}

// --------------------------------------------------------------------------
Ne_Font Ne_FontDescription::getFont(int size) const
{
   Ne_Font f;
   f.name = name;
   f.font = font;
   if (this->size == 0)
   {  // TTF
      if (size == 0)
         f.size = 12;
      else
         f.size = size;
   }
   else
      f.size = this->size;
   return f;
}
