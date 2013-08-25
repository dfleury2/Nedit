#ifndef NEDIT_WINDOWTITLE_H_INCLUDED
#define NEDIT_WINDOWTITLE_H_INCLUDED

#include "nedit.h"

const char* FormatWindowTitle(const std::string& filename,
                        const std::string& path,
                        const std::string& serverName,
                        int isServer,
                        int filenameSet,
                        int lockReasons,
                        int fileChanged,
                        const char* titleFormat);

void EditCustomTitleFormat(WindowInfo* window);

// Unit test purpose only
std::string CompressWindowTitle(const std::string& t);


#endif
