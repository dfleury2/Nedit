#ifndef Ne_AppContext_h__
#define Ne_AppContext_h__

#include "misc.h"
#include "Ne_Database.h"

#include <FL/Fl_Widget.H>

#include <string>
#include <map>
#include <vector>

class Ne_AppContext
{
public:
   Ne_AppContext(const std::string& appName) : appDB(appName) {}

   void addAction(XtActionsRec action);
   int addHook(XtActionHookProc hook, void* data);

   void callAction(Fl_Widget* w, const std::string& action, int event, const char** args, int nArgs);

   Ne_Database appDB;

private:
   typedef std::map<std::string, XtActionProc> Actions;
   Actions actions_;

   typedef std::vector< std::pair< XtActionHookProc, void* > > Hooks;
   Hooks hooks_;
};

void NeAppAddActions(Ne_AppContext&, XtActionList, int);
int NeAppAddActionHook(Ne_AppContext&, XtActionHookProc, void*);

#endif // Ne_AppContext_h__
