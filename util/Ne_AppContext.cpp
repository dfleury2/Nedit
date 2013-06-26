#include "Ne_AppContext.h"

#include <utility>
#include <iostream>

void Ne_AppContext::addAction(XtActionsRec action)
{
   actions_.insert(std::make_pair(action.name, action.proc));
}

int Ne_AppContext::addHook(XtActionHookProc hook, void* data)
{
   hooks_.push_back( std::make_pair(hook, data));
   return hooks_.size()-1;
}

void Ne_AppContext::callAction(Fl_Widget* w, const std::string& action, int event, const char** args, int nArgs)
{
   Actions::const_iterator foundAction = actions_.find(action);
   if (foundAction == actions_.end())
   {
      std::cout << "Action [" << action << "] not found" << std::endl;
      return;
   }
   
   for(Hooks::const_iterator hook = hooks_.begin(); hook != hooks_.end(); ++hook)
      hook->first(w, hook->second, action.c_str(), event, args, &nArgs);

   foundAction->second(w, event, args, &nArgs);
}

void NeAppAddActions(Ne_AppContext& context, XtActionList actionList, int n)
{
   for(int i = 0; i < n; ++i)
      context.addAction(actionList[i]);
}

int NeAppAddActionHook(Ne_AppContext& context, XtActionHookProc proc, void* data)
{
   return context.addHook(proc, data);
}
