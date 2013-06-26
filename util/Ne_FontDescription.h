#ifndef Ne_FontDescription_h__
#define Ne_FontDescription_h__

struct Ne_Font;

#include <FL/Fl.H>

#include <vector>
#include <string>

struct Ne_FontDescription
{
   Ne_FontDescription() : font(-1), size(0) {}

   std::string name; // Font User Name : Verdana bold italic
   Fl_Font font;     // FLTK font index
   int size;         // size (0 is for True Type Fonts)

   bool isFixed() const { return (size !=0); }

   Ne_Font getFont(int size = 0) const;

   static void InitFonts();
};

typedef std::vector<Ne_FontDescription> Ne_FontDescriptions;
extern Ne_FontDescriptions Fonts;

#endif // Ne_FontDescription_h__
