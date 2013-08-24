#include <gtest/gtest.h>

#include "../util/utils.h"

// --------------------------------------------------------------------------
TEST(UtilsTest, Directory)
{
   const char* currentDir = GetCurrentDir();
   std::string homeDir = GetHomeDir();
   const char* userName = NeGetUserName();
   const char* hostname = GetNameOfHost();
   
   char fullname[256] = "";
   PrependHome("file.txt", fullname, sizeof(fullname));
   ASSERT_EQ(homeDir + "/file.txt", fullname);

   PrependHome("file.txt", fullname, 10);
   ASSERT_STREQ(fullname, "");
}

// --------------------------------------------------------------------------
TEST(UtilsTest, Tools)
{
   ASSERT_EQ(Min(1, 2), 1);
   ASSERT_EQ(Trim(" hello "), "hello");
   ASSERT_EQ(GetStringIndex("A|B", -1, '|'), "");
   ASSERT_EQ(GetStringIndex("A|B", 0, '|'), "A");
   ASSERT_EQ(GetStringIndex("A|B", 1, '|'), "B");
   ASSERT_EQ(GetStringIndex("A|B", 2, '|'), "");
}

// --------------------------------------------------------------------------
TEST(UtilsTest, GetRCFileName)
{
}
