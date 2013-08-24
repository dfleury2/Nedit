#ifndef NEDIT_FILEUTILS_H_INCLUDED
#define NEDIT_FILEUTILS_H_INCLUDED

enum FileFormats {UNIX_FILE_FORMAT, DOS_FILE_FORMAT, MAC_FILE_FORMAT};

int ParseFilename(const char* fullname, char* filename, char* pathname);
int ExpandTilde(char* pathname);
const char* GetTrailingPathComponents(const char* path, int noOfComponents);
int NormalizePathname(char* pathname);
int CompressPathname(char* pathname);
int ResolvePath(const char* pathIn, char* pathResolved);

int FormatOfFile(const char* fileString);
void ConvertFromDosFileString(char* inString, int* length, char* pendingCR);
void ConvertFromMacFileString(char* fileString, int length);
int ConvertToDosFileString(char** fileString, int* length);
void ConvertToMacFileString(char* fileString, int length);
char* ReadAnyTextFile(const char* fileName, int forceNL);

#endif /* NEDIT_FILEUTILS_H_INCLUDED */
