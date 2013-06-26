#ifndef NEDIT_FONTSEL_H_INCLUDED
#define NEDIT_FONTSEL_H_INCLUDED

#include <FL/Fl_Widget.H>

#include <string>

// constant values for controlling the proportional font toggle
#define ONLY_FIXED   0
#define PREF_FIXED   1
#define PREF_PROP    2

// function prototype
std::string FontSel(Fl_Widget* parent, int showPropFont, const char* currFont, Fl_Color sampleFG, Fl_Color sampleBG);

#endif // NEDIT_FONTSEL_H_INCLUDED
