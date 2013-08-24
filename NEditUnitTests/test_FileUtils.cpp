#include <gtest/gtest.h>

#include "../util/fileUtils.h"

// --------------------------------------------------------------------------
TEST(FileUtilsTest, ParseFilename)
{
   char filename[256] = "";
   char pathname[256] = "";
#ifdef WIN32
   ASSERT_EQ(ParseFilename("z:/usr/local/file.txt", filename, pathname), 0);
   ASSERT_STREQ(filename, "file.txt");
   ASSERT_STREQ(pathname, "z:/usr/local/");
#else
   ASSERT_EQ(ParseFilename("/usr/local/file.txt", filename, pathname), 0);
   ASSERT_STREQ(filename, "file.txt");
   ASSERT_STREQ(pathname, "/usr/local/");
#endif
}
