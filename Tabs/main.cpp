#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Tabs.H>

#include <iostream>

using namespace std;

int main()
{
   Fl_Window win(30, 50, 600, 500, "Tab Sample");
   
   Fl_Menu_Bar menu(0,0,600,25);
   menu.add("TEST/MENU1", 0, NULL);
   menu.add("TEST/MENU2", 0, NULL);
   menu.add("TEST2/MENU1", 0, NULL);
   menu.add("TEST2/MENU2", 0, NULL);
   menu.add("TEST2/MENU3", 0, NULL);

   Fl_Tabs tabs(0, 25, 600, 575);
   tabs.color(FL_RED);

   int x, y, w, h;
   tabs.client_area(x, y, w, h);
   cout << x << "," << y << "," << w << "," << h << endl;

      for (int i = 0; i< 50; ++i)
      {
         Fl_Group* g = new Fl_Group(x, y, w, h);
         g->box(FL_ENGRAVED_BOX);
         char buffer[32];
         sprintf(buffer, "Group %d", i+1);
         g->copy_label(buffer);
         g->color( fl_rgb_color( rand()% 255, rand()%255, rand()%255) );
         g->end();
         if (!i) tabs.resizable(g);
         g->selection_color(FL_YELLOW);
         g->labelcolor(FL_MAGENTA);
      }
   tabs.end();
   tabs.selection_color(FL_GREEN);
   tabs.labelcolor(FL_RED);
   win.resizable(&tabs);
   win.end();
   win.show();

   return Fl::run();
}
