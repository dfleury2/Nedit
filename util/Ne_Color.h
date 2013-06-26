#ifndef Ne_Color_h__
#define Ne_Color_h__

#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>

#include <string>

Fl_Color GetColor(const char* colorName);
bool CheckColor(const char* name);
std::string ColorPicker();

#endif // Ne_Color_h__
