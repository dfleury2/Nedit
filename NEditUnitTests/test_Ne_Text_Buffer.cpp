#include <gtest/gtest.h>

#include "../source/Ne_Text_Buffer.h"

// --------------------------------------------------------------------------
TEST(Ne_Text_Buffer_Test, Create)
{
   Ne_Text_Buffer* buffer = BufCreate();
   EXPECT_EQ(buffer->length, 0);
}