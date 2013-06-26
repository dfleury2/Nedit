#ifndef Ne_StatsLine_h__
#define Ne_StatsLine_h__

#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>

// --------------------------------------------------------------------------
class Ne_StatsLine : public Fl_Group
{
public:
   Ne_StatsLine(int x, int y, int w, int h, const char* l = 0);

   Fl_Box& statsLine() { return statsLine_; }
   Fl_Box& statsLineColNo() { return statsLineColNo_; }

   void resize(int x, int y, int w, int h);

private:
   Fl_Box statsLine_; // file stats information display
   Fl_Box statsLineColNo_; // Line/Column information display
};

#endif // Ne_StatsLine_h__
