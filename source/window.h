#ifndef NEDIT_WINDOW_H_INCLUDED
#define NEDIT_WINDOW_H_INCLUDED

#include "nedit.h"

#include <list>

typedef std::list<WindowInfo*> Windows;

// --------------------------------------------------------------------------
// Manage Windows
// --------------------------------------------------------------------------
class WindowManager
{
public:
   Windows windows;
};

// Returns the number of open windows
Windows::size_type NWindows();
// Returns the number of documents (ie. tabs) for a window (or a doc belonging to the window)
Windows::size_type NDocuments(WindowInfo* window);
// Returns the WindowInfo of the widget
WindowInfo* WidgetToWindow(Fl_Widget* w);

WindowInfo* CreateNeWindow(const char* title, char* geometry, int iconic);
void CloseWindow(WindowInfo* window);
void UpdateWindowTitle(const WindowInfo* window);
void UpdateWindowReadOnly(WindowInfo* window);
void UpdateStatsLine(WindowInfo* window);
void UpdateWMSizeHints(WindowInfo* window);
void UpdateMinPaneHeights(WindowInfo* window);
void UpdateNewOppositeMenu(WindowInfo* window, int openInTab);
void SetWindowModified(WindowInfo* window, int modified);
void MakeSelectionVisible(WindowInfo* window, Fl_Widget* textPane);
int GetSimpleSelection(Ne_Text_Buffer* buf, int* left, int* right);
WindowInfo* FindWindowWithFile(const char* name, const char* path);
void SetAutoIndent(WindowInfo* window, int state);
// TODO: void SetShowMatching(WindowInfo* window, int state);
void SetFonts(WindowInfo* window, const char* fontName, const char* italicName, const char* boldName, const char* boldItalicName);
void SetColors(WindowInfo* window, const char* textFg, const char* textBg,
               const char* selectFg, const char* selectBg, const char* hiliteFg,
               const char* hiliteBg, const char* lineNoFg, const char* cursorFg);
// TODO: void SetOverstrike(WindowInfo* window, int overstrike);
void SetAutoWrap(WindowInfo* window, int state);
void SetAutoScroll(WindowInfo* window, int margin);
// TODO: void SplitPane(WindowInfo* window);
Ne_Text_Editor* GetPaneByIndex(WindowInfo* window, int paneIndex);
int WidgetToPaneIndex(WindowInfo* window, Fl_Widget* w);
// TODO: void ClosePane(WindowInfo* window);
int GetShowTabBar(WindowInfo* window);
void ShowTabBar(WindowInfo* window, int state);
void ShowStatsLine(WindowInfo* window, int state);
void ShowISearchLine(WindowInfo* window, int state);
void TempShowISearch(WindowInfo* window, int state);
void ShowLineNumbers(WindowInfo* window, bool state);
void SetModeMessage(WindowInfo* window, const char* message);
// TODO: void ClearModeMessage(WindowInfo* window);
// TODO: void AddSmallIcon(Fl_Widget* shell);
void SetTabDist(WindowInfo* window, int tabDist);
void SetEmTabDist(WindowInfo* window, int emTabDist);
int CloseAllDocumentInWindow(WindowInfo* window);
WindowInfo* CreateDocument(WindowInfo* shellWindow, const char* name);
WindowInfo* TabToWindow(Fl_Widget* tab);
void RaiseDocument(WindowInfo* window);
void RaiseDocumentWindow(WindowInfo* window);
void RaiseFocusDocumentWindow(WindowInfo* window, bool focus);
WindowInfo* MarkLastDocument(WindowInfo* window);
WindowInfo* MarkActiveDocument(WindowInfo* window);
// TODO: void NextDocument(WindowInfo* window);
// TODO: void PreviousDocument(WindowInfo* window);
// TODO: void LastDocument(WindowInfo* window);
// TODO: WindowInfo* MoveDocument(WindowInfo* toWindow, WindowInfo* window);
// TODO: WindowInfo* DetachDocument(WindowInfo* window);
// TODO: void MoveDocumentDialog(WindowInfo* window);
WindowInfo* GetTopDocument(Fl_Widget* w);
bool IsTopDocument(const WindowInfo* window);
int IsIconic(WindowInfo* window);
int IsValidWindow(WindowInfo* window);
void RefreshTabState(WindowInfo* window);
void ShowWindowTabBar(WindowInfo* window);
// TODO: void RefreshMenuToggleStates(WindowInfo* window);
void RefreshWindowStates(WindowInfo* window);
void AllWindowsBusy(const char* message);
void AllWindowsUnbusy();
void SortTabBar(WindowInfo* window);
// TODO: void SetBacklightChars(WindowInfo* window, char* applyBacklightTypes);
void SetToggleButtonState(WindowInfo* window, Fl_Widget* w, bool state, bool notify);
void SetSensitive(WindowInfo* window, Fl_Widget* w, bool sensitive);
void SetSensitive(WindowInfo* window, Fl_Menu_Item* item, bool sensitive);

#endif /* NEDIT_WINDOW_H_INCLUDED */
