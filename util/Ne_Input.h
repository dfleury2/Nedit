#ifndef Ne_Input_h__
#define Ne_Input_h__

#include <FL/Fl.H>
#include <FL/Fl_Input.H>

// --------------------------------------------------------------------------
class Ne_Input : public Fl_Input
{
public:
   Ne_Input(int X, int Y, int W, int H, const char* L = 0);

   int handle(int e);
};

#endif
