#ifndef Ne_MenuBar_h__
#define Ne_MenuBar_h__

#include "Ne_AppContext.h"

#include <FL/Fl_Menu_Bar.H>

#include <string>
#include <map>

class Ne_MenuBar : public Fl_Menu_Bar
{
public:
   typedef std::map<std::string, std::string > NameToPaths;
   typedef NameToPaths::const_iterator const_iterator;
   
   Ne_MenuBar(int x, int y, int w, int h, Ne_AppContext* appContext = 0);

   int handle(int e);

   const std::string&  add(const std::string& name, const std::string& label, char mnemonic, Fl_Callback* cb, void* data= 0, int flags= 0);
   const std::string&  add_toggle(const std::string& name, const std::string& label, char mnemonic, Fl_Callback* cb, void* data= 0, int flags= 0, bool state = false);
   const std::string&  add_radio(const std::string& name, const std::string& label, char mnemonic, Fl_Callback* cb, void* data= 0, int flags= 0, bool state = false);
   const std::string&  add_menu(const std::string& name, const std::string& label, char mnemonic, Fl_Callback* cb, void* data= 0, int flags= 0);

   void replace( const std::string& name, const std::string& label);
   void remove(const std::string& name);
   void remove(int index) { Fl_Menu_Bar::remove(index); }

   void set_sensitive(const std::string& name, bool state);
   void set_checked(const std::string& name, bool state);

   Fl_Menu_Item* get_item( const std::string& name );
   int find_index(const std::string&);
   int find_index(const Fl_Menu_Item*);
   std::string item_path(const std::string& name);

   // File Menu Items
   Fl_Menu_Item* getNewOppositeItem() { return get_item("newOpposite"); } // New Window or New Tab Item
   Fl_Menu_Item* getOpenPreviousItem() { return get_item("openPrevious"); }
   Fl_Menu_Item* getCloseItem() { return get_item("close"); }
   Fl_Menu_Item* getPrintSelectionItem() { return get_item("printSelection"); };

   // Edit Menu Items
   Fl_Menu_Item* getUndoItem() { return get_item("undo"); };
   Fl_Menu_Item* getRedoItem() { return get_item("redo"); };
   Fl_Menu_Item* getCutItem() { return get_item("cut"); };
   Fl_Menu_Item* getDelItem() { return get_item("delete"); };
   Fl_Menu_Item* getCopyItem() { return get_item("copy"); };
   Fl_Menu_Item* getLowerItem() { return get_item("lowerCase"); };
   Fl_Menu_Item* getUpperItem() { return get_item("upperCase"); };

   // Search Menu Items
   Fl_Menu_Item* getFindAgainItem() { return get_item("findAgain"); };
   Fl_Menu_Item* getFindSelectionItem() { return get_item("findSelection"); };
   Fl_Menu_Item* getReplaceFindAgainItem() { return get_item("replaceFindAgain"); };
   Fl_Menu_Item* getReplaceAgainItem() { return get_item("replaceAgain"); };
   Fl_Menu_Item* getGotoSelectedItem() { return get_item("gotoSelected"); };
   Fl_Menu_Item* getFindDefinitionItem() { return get_item("findDefinition"); };
   Fl_Menu_Item* getShowCalltipItem() { return get_item("showCalltip"); };
   
   // Preferences Menu Items
   Fl_Menu_Item* getShowLineNumbersDefItem() { return get_item("showLineNumbersDef"); };

   Fl_Menu_Item* getSortOpenPrevMenuItem() { return get_item("sortOpenPrevMenu"); };
   Fl_Menu_Item* getPathInWindowsMenuItem() { return get_item("pathInWindowsMenu"); };

   Fl_Menu_Item* getStatsLineItemItem() { return get_item("statisticsLine"); };
   Fl_Menu_Item* getISearchLineItemItem() { return get_item("incrementalSearchLine"); };
   Fl_Menu_Item* getLineNumsItemItem() { return get_item("lineNumbers"); };
   Fl_Menu_Item* getAutoIndentOffItem() { return get_item("autoIndentOff"); };
   Fl_Menu_Item* getAutoIndentItem() { return get_item("autoIndent"); };
   Fl_Menu_Item* getLanguageModesItem() { return get_item("languageMode"); };
   Fl_Menu_Item* getSmartIndentItem() { return get_item("smarIndent"); };
   Fl_Menu_Item* getNoWrapItem() { return get_item("noWrap"); };
   Fl_Menu_Item* getNewlineWrapItem() { return get_item("newlineWrap"); };
   Fl_Menu_Item* getContinuousWrapItem() { return get_item("continuousWrap"); };
   Fl_Menu_Item* getHighlightItem() { return get_item("highlightSyntax"); };
   Fl_Menu_Item* getBacklightCharsItem() { return get_item("backlightChars"); };
   Fl_Menu_Item* getSaveLastItem() { return get_item("makeBackupCopy"); };
   Fl_Menu_Item* getAutoSaveItem() { return get_item("incrementalBackup"); };

   Fl_Menu_Item* getShowMatchingOffItem() { return get_item("showMatchingOff"); };
   Fl_Menu_Item* getShowMatchingDelimitItem() { return get_item("showMatchingDelimiter"); };
   Fl_Menu_Item* getShowMatchingRangeItem() { return get_item("showMatchingRange"); };
   Fl_Menu_Item* getMatchSyntaxBasedItem() { return get_item("matchSyntax"); };

   // Shell Menu Items
   Fl_Menu_Item* getFilterItem() { return get_item("filterSelection"); };
   Fl_Menu_Item* getCancelShellItem() { return get_item("cancelShellCommand"); };

   // Macro Menu Items
   Fl_Menu_Item* getLearnItem() { return get_item("learnKeystrokes"); };
   Fl_Menu_Item* getFinishLearnItem() { return get_item("finishLearn"); };
   Fl_Menu_Item* getCancelMacroItem() { return get_item("cancelLearn"); };
   Fl_Menu_Item* getReplayItem() { return get_item("replayKeystrokes"); };
   Fl_Menu_Item* getRepeatItem() { return get_item("repeat"); };

   // Windows Menu Items
   Fl_Menu_Item* getSplitPaneItem() { return get_item("splitPane"); };
   Fl_Menu_Item* getClosePaneItem() { return get_item("closePane"); };
   Fl_Menu_Item* getDetachDocumentItem() { return get_item("detachBuffer"); };
   Fl_Menu_Item* getMoveDocumentItem() { return get_item("moveDocument"); };

   //   Fl_Menu_Item* getItem() { return get_item(""); };

   
   NameToPaths::const_iterator begin() const { return nameToPath_.begin(); }
   NameToPaths::const_iterator end() const { return nameToPath_.end(); }
   
   static std::string ComputePath( const std::string& label, char mnemonic );

private:
   NameToPaths nameToPath_;
   Ne_AppContext* appContext_;
   Fl_Widget* lastFocus;
};

#endif // Ne_MenuBar_h__
