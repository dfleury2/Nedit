#ifndef NEDIT_WINDOWTITLE_H_INCLUDED
#define NEDIT_WINDOWTITLE_H_INCLUDED

#include "nedit.h"

const char* FormatWindowTitle(const char* filename,
                        const char* path,
                        const char* serverName,
                        int isServer,
                        int filenameSet,
                        int lockReasons,
                        int fileChanged,
                        const char* titleFormat);

void EditCustomTitleFormat(WindowInfo* window);

#endif
