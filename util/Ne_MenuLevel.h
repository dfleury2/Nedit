#ifndef Ne_MenuLevel_h__
#define Ne_MenuLevel_h__

#include <vector>
#include <string>

// --------------------------------------------------------------------------
struct Ne_MenuLevel
{
   std::vector<std::string> levels;
   void push(const std::string& level)
   {
      levels.push_back(level);
   }
   void pop()
   {
      levels.pop_back();
   }

   operator const std::string& ()
   {
      static std::string path;
      path.clear();

      for (std::vector<std::string>::const_iterator s = levels.begin(); s != levels.end(); ++s)
         path += (s != levels.begin() ? "/" : "") + (*s);
      return path;
   }
};

inline Ne_MenuLevel operator + (const Ne_MenuLevel& ml, const char* l)
{
   Ne_MenuLevel tmp = ml;
   tmp.push(l);
   return tmp;
}

#endif // Ne_MenuLevel_h__
