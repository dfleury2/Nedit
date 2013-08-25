#include <gtest/gtest.h>

#include "../source/windowTitle.h"

// --------------------------------------------------------------------------
TEST(WindowTitleTest, Test)
{
   EXPECT_STREQ(FormatWindowTitle("F", "P", "S", 0, 0, 0, 0, ""), "<empty>");
   EXPECT_STREQ(FormatWindowTitle("F", "P", "S", 0, 0, 0, 0, "   "), "<empty>");
   EXPECT_STREQ(FormatWindowTitle("F", "P", "S", 0, 0, 0, 0, "%s"), "<empty>");
   EXPECT_STREQ(FormatWindowTitle("F", "P", "S", 1, 0, 0, 0, "%s"), "S");
   EXPECT_STREQ(FormatWindowTitle("F", "P", "S", 0, 0, 0, 0, "%f"), "F");
   EXPECT_STREQ(FormatWindowTitle("F", "P", "S", 0, 0, 0, 0, "%"), "%");
   EXPECT_STREQ(FormatWindowTitle("F", "P", "S", 0, 0, 0, 0, "%% default %"), "% default %");
   EXPECT_STREQ(FormatWindowTitle("F", "P", "S", 0, 1, 0, 0, "%d"), "P");

   std::string hostname = GetNameOfHost();
   EXPECT_EQ(FormatWindowTitle("F", "P", "S", 0, 1, 0, 0, "%h"), hostname);

   std::string username = NeGetUserName();
   EXPECT_EQ(FormatWindowTitle("F", "P", "S", 0, 1, 0, 0, "%u"), username);
}

// --------------------------------------------------------------------------
TEST(WindowTitleTest, CompressWindowTitle)
{
   EXPECT_EQ(CompressWindowTitle("  -  - "), "");
   EXPECT_EQ(CompressWindowTitle("  ()- "), "");
   EXPECT_EQ(CompressWindowTitle("  (   )- "), "(   )");
   EXPECT_EQ(CompressWindowTitle("  []- "), "");
   EXPECT_EQ(CompressWindowTitle("{}"), "");
   EXPECT_EQ(CompressWindowTitle("{A}"), "{A}");
   EXPECT_EQ(CompressWindowTitle("[]{A}()"), "{A}");
}
