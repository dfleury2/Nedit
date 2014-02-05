#include "Ne_MenuBar.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/names.h>

#include <utility>
#include <iostream>

// --------------------------------------------------------------------------
Ne_MenuBar::Ne_MenuBar(int x, int y, int w, int h, Ne_AppContext* appContext)
    : Fl_Menu_Bar(x, y, w, h), appContext_(appContext),
    lastFocus(0)
{  
    box(FL_THIN_UP_BOX);

    //this->h(fl_height(this->textfont(), this->textsize()) + 9);
}

// --------------------------------------------------------------------------
int Ne_MenuBar::handle(int e)
{
    switch(e)
    {
    case FL_PUSH:
        {
            lastFocus = Fl::focus();
            Fl::focus(this);
            return Fl_Menu_Bar::handle(e);
        }
    case FL_LEAVE:
        {
            if (lastFocus)
            {
                lastFocus->take_focus();
                lastFocus = 0;
            }
            return 1;
        }
    }

    return Fl_Menu_Bar::handle(e);
}

// --------------------------------------------------------------------------
int StringToShortcut(const std::string& str)
{
    int shortcut = 0;
    if (str.find("Shift") != std::string::npos)
        shortcut += FL_SHIFT;
    if (str.find("Ctrl") != std::string::npos)
        shortcut += FL_CTRL;
    if (str.find("Alt") != std::string::npos)
        shortcut += FL_ALT;

    std::string::size_type key = str.find("<Key>");
    if (key != std::string::npos && str[key+5] != '\0')
        shortcut += str[key+5];

    return shortcut;
}

// --------------------------------------------------------------------------
const std::string&  Ne_MenuBar::add(const std::string& name, const std::string& label, char mnemonic, Fl_Callback* cb, void* data, int flags)
{
    std::string path = ComputePath(label, mnemonic);

    bool success = nameToPath_.insert(std::make_pair(name, path)).second;
    if (!success)
        throw "duplicate name [" + name + "]";

    int shortcut = 0;
    if (appContext_)
    {
        Ne_Database& db = appContext_->appDB;
        for(Ne_Database::const_iterator value = db.begin(); value != db.end(); ++value)
        {
            std::string key = name + ".accelerator";
            int found = value->first.find(key);
            if ( found != std::string::npos && (found + key.size()) == value->first.size())
            {
                // std::cout << name << "=" << value->second << std::endl;
                shortcut = StringToShortcut(value->second);
            }
        }
    }

    Fl_Menu_Bar::add(path.c_str(), shortcut, cb, data, flags);
    return name;
}

// --------------------------------------------------------------------------
const std::string&  Ne_MenuBar::add_toggle(const std::string& name, const std::string& label, char mnemonic, Fl_Callback* cb, void* data, int flags, bool state)
{
    if (state) flags |= FL_MENU_VALUE;
    return add(name, label, mnemonic, cb, data, FL_MENU_TOGGLE | flags);
}

// --------------------------------------------------------------------------
const std::string&  Ne_MenuBar::add_radio(const std::string& name, const std::string& label, char mnemonic, Fl_Callback* cb, void* data, int flags, bool state)
{
    if (state) flags |= FL_MENU_VALUE;
    return add(name, label, mnemonic, cb, data, FL_MENU_RADIO | flags);
}

// --------------------------------------------------------------------------
const std::string& Ne_MenuBar::add_menu(const std::string& name, const std::string& label, char mnemonic, Fl_Callback* cb, void* data, int flags)
{
    return add(name, label, mnemonic, cb, data, FL_SUBMENU | flags);
}

// --------------------------------------------------------------------------
void Ne_MenuBar::replace(const std::string& name, const std::string& label)
{
    int index = find_index(name);
    Fl_Menu_Bar::replace(index, label.c_str());

    char path[512] = "";
    item_pathname(&path[0], sizeof path, &menu()[index]);
    nameToPath_[name] = path;
}

// -------------------------------------------------------------------------
void Ne_MenuBar::remove(const std::string& name)
{
    int index = find_index(name);
    Fl_Menu_Bar::remove(index);
}

// -------------------------------------------------------------------------
void Ne_MenuBar::set_sensitive(const std::string& name, bool state)
{
    Fl_Menu_Item* item = get_item(name);
    if (state) item->activate();
    else item->deactivate();
}

// -------------------------------------------------------------------------
void Ne_MenuBar::set_checked(const std::string& name, bool state)
{
    Fl_Menu_Item* item = get_item(name);
    if (state) item->set();
    else item->clear();
}

// --------------------------------------------------------------------------
Fl_Menu_Item* Ne_MenuBar::get_item(const std::string& name)
{
    std::map<std::string, std::string>::const_iterator found = nameToPath_.find(name);
    if (found == nameToPath_.end())
        throw "get_item(" + name +") failed";
    Fl_Menu_Item* item = const_cast<Fl_Menu_Item*>(find_item(found->second.c_str()));
    if (!item)
        throw "get_item(" + found->second +") failed on find_item";
    return item;
}

// --------------------------------------------------------------------------
int Ne_MenuBar::find_index(const std::string& name)
{
    Fl_Menu_Item* item = get_item(name);
    return this->find_index(item);
}

// --------------------------------------------------------------------------
int Ne_MenuBar::find_index(const Fl_Menu_Item* item)
{
    int index = Fl_Menu_Bar::find_index(item);
    if (index==-1)
        throw "find_index(" + std::string(item->label()) + ") failed";
    return index;
}

// --------------------------------------------------------------------------
std::string Ne_MenuBar::item_path(const std::string& name)
{
    int index = find_index(name);
    char path[512] = "";
    item_pathname(&path[0], sizeof path, &menu()[index]);
    return path;
}

// --------------------------------------------------------------------------
std::string Ne_MenuBar::ComputePath( const std::string& label, char mnemonic )
{
    if (mnemonic == 0) return label;

    std::string::size_type foundSep = label.find_last_of('/');
    if (foundSep == std::string::npos)
        foundSep = 0;

    std::string::size_type foundMnemonic = label.find(mnemonic, foundSep);
    if (foundMnemonic == std::string::npos)
        throw "Mnemonic [" + std::string(1,mnemonic) + "] not found in label [" + label + "]";

    std::string path = label;
    path.insert(foundMnemonic, 1, '&');
    return path;
}
