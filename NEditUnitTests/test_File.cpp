#include <gtest/gtest.h>

#include "../source/file.h"

// --------------------------------------------------------------------------
TEST(FileTest, UniqueUntitledName)
{
   char name[MAXPATHLEN] = "";
   UniqueUntitledName(name);
   ASSERT_STREQ(name, "Untitled");
}
