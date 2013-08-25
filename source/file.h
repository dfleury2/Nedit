#ifndef NEDIT_FILE_H_INCLUDED
#define NEDIT_FILE_H_INCLUDED

#include "nedit.h"

#include <FL/Fl_Widget.H>

/* flags for EditExistingFile */
#define NE_CREATE 1
#define NE_SUPPRESS_CREATE_WARN 2
#define NE_PREF_READ_ONLY 4

#define PROMPT_SBC_DIALOG_RESPONSE 0
#define YES_SBC_DIALOG_RESPONSE 1
#define NO_SBC_DIALOG_RESPONSE 2

WindowInfo* EditNewFile(WindowInfo* inWindow, char* geometry, int iconic, const char* languageMode, const char* defaultPath);
WindowInfo* EditExistingFile(WindowInfo* inWindow, const char* name, const char* path, int flags, char* geometry, int iconic, const char* languageMode, int tabbed, bool bgOpen);
void RevertToSaved(WindowInfo* window);
int SaveWindow(WindowInfo* window);
int SaveWindowAs(WindowInfo* window, const char* newName, int addWrap);
int CloseAllFilesAndWindows();
int CloseFileAndWindow(WindowInfo* window, int preResponse);
void PrintWindow(WindowInfo* window, int selectedOnly);
void PrintString(const char* str, int length, Fl_Widget* parent, const char* jobName);
int WriteBackupFile(WindowInfo* window);
int IncludeFile(WindowInfo* window, const char* name);
int PromptForExistingFile(WindowInfo* window, char* prompt, char* fullname);
int PromptForNewFile(WindowInfo* window, char* prompt, char* fullname, int* fileFormat, int* addWrap);
int CheckReadOnly(WindowInfo* window);
void RemoveBackupFile(WindowInfo* window);
void UniqueUntitledName(char* name);
void CheckForChangesToFile(WindowInfo* window);

#endif /* NEDIT_FILE_H_INCLUDED */
