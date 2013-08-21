#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Menu_Bar.H>

#include <iostream>
#include <iomanip>

using namespace std;

// --------------------------------------------------------------------------
class Ne_Menu_Bar : public Fl_Menu_Bar
{
public:
   Ne_Menu_Bar(int x, int y, int w, int h) : Fl_Menu_Bar(x, y, w, h)
   {
      lightColor = fl_lighter(fl_lighter(FL_BACKGROUND_COLOR));
      darkColor = FL_BACKGROUND_COLOR;
      box(FL_NO_BOX);
   }

   void draw()
   {
      for (int i = 0; i < h(); ++i)
      {
         fl_color(fl_color_average(darkColor, lightColor, ((float)i)/h()));
         fl_line(x(), y() + i , w(), y() + i);
      }
      Fl_Menu_Bar::draw();
   }

   Fl_Color lightColor;
   Fl_Color darkColor;
};

// --------------------------------------------------------------------------
Ne_Menu_Bar* menuBar = 0;

void DisplayMenuInfo()
{
   std::cout << "---------------------------------------\n";
   std::cout << "Size : " << menuBar->size() << std::endl;
   for(int i = 0; i < menuBar->size(); ++i)
   {
      const Fl_Menu_Item& item = menuBar->menu()[i];
      const char* label  = item.label();
      std::cout << setw(5) << i << " : ";
      if (label) std::cout << label;
      char buffer[256] = "";
      int res = menuBar->item_pathname(buffer, sizeof(buffer)-1, &item);
      switch(res)
      {
         case 0: std::cout << " : " << buffer; break;
         case -1: std::cout << "NO PATH"; break;
         case -2: std::cout << "***Buffer Too Short***"; break;
      }
      if (item.flags & FL_MENU_INACTIVE) std::cout << ":INACTIVE";
      if (item.flags & FL_MENU_TOGGLE) std::cout << ":TOGGLE";
      if (item.flags & FL_MENU_VALUE) std::cout << ":VALUE";
      if (item.flags & FL_MENU_RADIO) std::cout << ":RADIO";
      if (item.flags & FL_MENU_INVISIBLE) std::cout << ":INVISIBLE";
      if (item.flags & FL_SUBMENU) std::cout << ":SUBMENU";
      if (item.flags & FL_MENU_DIVIDER) std::cout << ":DIVIDER";
      if (item.flags & FL_MENU_HORIZONTAL) std::cout << ":HORIZONTAL";
      
      std::cout << std::endl;
   }
}

// --------------------------------------------------------------------------
int GetWindowNameMarker(const char* name)
{
   for(int i = 0; i < menuBar->size(); ++i)
   {
      const Fl_Menu_Item& item = menuBar->menu()[i];
      const char* label  = item.label();
      if (label && strcmp(label, name) ==0)
         return i;
   }
   return -1;
}

int GetWindowNameStartMarker() { return GetWindowNameMarker("WindowNameStart"); }
int GetWindowNameEndMarker() { return GetWindowNameMarker("WindowNameEnd"); }

// --------------------------------------------------------------------------
void AddNameCB(Fl_Widget* w, void*)
{
   std::cout << "AddNameCB" << std::endl;
   DisplayMenuInfo();

   int index = GetWindowNameEndMarker();
   if (index==-1) return; // damned....

   static int i = 0;
   char buffer[32];
   sprintf(buffer, "Untitled_%d", ++i);

   menuBar->insert(index, buffer, 0, NULL);
}

// --------------------------------------------------------------------------
void RemoveLastNameCB(Fl_Widget* w, void*)
{
   std::cout << "RemoveLastNameCB" << std::endl;
   DisplayMenuInfo();
}

// --------------------------------------------------------------------------
void ClearAllCB(Fl_Widget* w, void*)
{
   std::cout << "ClearAllCB" << std::endl;
   DisplayMenuInfo();

   int startIndex = GetWindowNameStartMarker();
   int endIndex = GetWindowNameEndMarker();
   
   // BUG ? if the end marker item is delete, then, it's remove with the previous item on the list
   ((Fl_Menu_Item*)menuBar->menu())[endIndex].flags = menuBar->menu()[endIndex].flags & !FL_MENU_INVISIBLE;
   while( endIndex != startIndex + 1)
   {
      menuBar->remove(startIndex+1);
      DisplayMenuInfo();
      endIndex = GetWindowNameEndMarker();
   }
   // Back to invisibility
   ((Fl_Menu_Item*)menuBar->menu())[endIndex].flags = menuBar->menu()[endIndex].flags | FL_MENU_INVISIBLE;
}

// --------------------------------------------------------------------------
int main(int argc, char* argv[])
{
   Fl_Window win(30, 50, 600, 500, "Menu Bar Sample");

   menuBar = new Ne_Menu_Bar(0,0, win.w(), 23);

   menuBar->add("Menu/Add Name", FL_CTRL + 'a', AddNameCB);
   menuBar->add("Menu/Remove Last Name", FL_CTRL + 'r', RemoveLastNameCB);
   menuBar->add("Menu/Clear All", FL_Delete, ClearAllCB, 0, FL_MENU_DIVIDER);
   menuBar->add("Menu/A SubMenu", 0, NULL, 0, FL_SUBMENU | FL_MENU_DIVIDER);
   menuBar->add("Menu/A SubMenu/Item1", 0, NULL, 0);
   menuBar->add("Menu/A SubMenu/Item2", 0, NULL, 0);
   menuBar->add("Menu/A SubMenu/Item3", 0, NULL, 0);
   menuBar->add("Menu/Close");

   menuBar->add("Window/Split");
   menuBar->add("Window/Close", 0, NULL, NULL, FL_MENU_DIVIDER);
   menuBar->add("Window/Detach");
   menuBar->add("Window/Move to...", 0, NULL, NULL, FL_MENU_DIVIDER);
   menuBar->add("Window/WindowNameStart", 0, NULL, 0, FL_MENU_INVISIBLE);
   menuBar->add("Window/WindowNameEnd", 0, NULL, 0, FL_MENU_INVISIBLE);
   menuBar->add("Help/Help1");
   menuBar->add("Help/Help2");

   Fl_Box box(0,23,win.w(), win.h()-menuBar->h());
   box.box(FL_THIN_DOWN_FRAME);

   win.resizable(&box);
   //Fl::get_system_colors();
   win.show();

   // Retrieve Font
   Fl_Font f = 0;
   int nFonts = Fl::set_fonts("*");
   for(int i = 0; f == 0 && i < nFonts; ++i)
      if (!strcmp("Tahoma", Fl::get_font_name(Fl_Font(i))))
         f = Fl_Font(i);

   if (f)
      menuBar->textfont(f);
   menuBar->textsize(12);


   return Fl::run();
}
