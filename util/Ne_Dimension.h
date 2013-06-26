#ifndef __Ne_Dimension_h__
#define __Ne_Dimension_h__

struct Ne_Dimension
{
   Ne_Dimension(int X, int Y, int W, int H) : x(X), y(Y), w(W), h(H) {}
   int x,y,w,h;
};

#endif
