#ifndef Ne_Font_h__
#define Ne_Font_h__

#include <FL/Fl.H>

#include <string>

struct Ne_Font
{
   Ne_Font();

   // Create a font based on a string : Verdana bold italic 12
   Ne_Font(const char* str) { if (str) init(str); }
   Ne_Font(const std::string& str) { init(str); }
   Ne_Font(const std::string& str, int size);

   std::string name;
   Fl_Font font;
   int size;
   int max_width() const;
   int min_width() const;
   bool isFixed() const { return (max_width() == min_width());}
   int fixedFontWidth() const { return (isFixed() ? max_width() : -1); }

   int ascent() const;
   int descent() const;
   int height() const { return ascent() + descent(); }

   bool init(const std::string& str);

private:
   void computeMaxMinWidth() const;
   mutable int max_width_, min_width_;
   mutable int ascent_, descent_;
};


#endif // Ne_Font_h__
