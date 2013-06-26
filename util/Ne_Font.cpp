#include "Ne_Font.h"

#include "Ne_FontDescription.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <iostream>
#include <algorithm>

// --------------------------------------------------------------------------
Ne_Font::Ne_Font() : font(FL_HELVETICA), size(FL_NORMAL_SIZE),
   max_width_(-1), min_width_(-1), ascent_(-1), descent_(-1)
{}

// --------------------------------------------------------------------------
Ne_Font::Ne_Font(const std::string& str, int size)
{
   char buf[32];
   sprintf(buf, " %d", size);
   init( str + buf);
}

// --------------------------------------------------------------------------
// Create a font based on a string : Verdana bold italic 12
// --------------------------------------------------------------------------
bool Ne_Font::init(const std::string& str)
{
   if (Fonts.empty()) Ne_FontDescription::InitFonts();

   font = FL_HELVETICA;
   size = 12;
   max_width_ = min_width_ = -1;
   ascent_ = descent_ = -1;

   // Looking for size
   std::string::size_type found = str.rfind(' ');
   if (found == std::string::npos)
   {
      std::cout << "Bad font string format [" << str << "]. Using default";
      return false;
   }

   name = str.substr(0, found);
   size = atoi(str.substr(found+1).c_str());

   Ne_FontDescriptions::const_iterator f = Fonts.begin();
   for(; f != Fonts.end(); ++f)
      if (f->name == name && (f->size == 0 || f->size == size))
         break;
   if (f == Fonts.end())
   {
      std::cout << "Font Name [" << name << "] or size [" << size << "] not found. Using Default.";
      size = 12;
      return false;
   }
   else
      font = f->font;

   return true;
}

// --------------------------------------------------------------------------
int Ne_Font::max_width() const
{
   if (max_width_ ==  -1) computeMaxMinWidth();
   return max_width_;
}

// --------------------------------------------------------------------------
int Ne_Font::min_width() const
{
   if (min_width_ ==  -1) computeMaxMinWidth();
   return min_width_;
}

// --------------------------------------------------------------------------
int Ne_Font::ascent() const
{
   if (ascent_ ==  -1) computeMaxMinWidth();
   return ascent_;
}

// --------------------------------------------------------------------------
int Ne_Font::descent() const
{
   if (descent_ ==  -1) computeMaxMinWidth();
   return descent_;
}

// --------------------------------------------------------------------------
void Ne_Font::computeMaxMinWidth() const
{
   fl_font(font, size);
   max_width_ = (int)fl_width('M');
   min_width_ = (int)fl_width('.');
   const char* widths = "|,i1[WQ";
   for( int i = 0; widths[i]; ++i) 
   {
      int width = (int)fl_width(widths[i]);
      max_width_ = std::max(max_width_, width);
      min_width_ = std::min(min_width_, width);
   }

   descent_ = fl_descent();
   ascent_ = fl_height(font, size) - descent_;
}
