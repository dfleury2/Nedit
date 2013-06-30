#include <gtest/gtest.h>

#include "../source/Ne_Text_Buffer.h"
#include "../util/utils.h"

namespace
{
   int bufModifiedCBCallCount = 0;
   int modifiedPosition = -1;
   int modifiedInserted = -1;
   int modifiedDeleted = -1;
   int modifiedRestyled = -1;
   std::string modifiedDeletedText;

   int bufPreDeleteCBCallCount = 0;
   int preDeletePosition = -1;
   int preDeleteDeleted = -1;

   void bufModifiedCB(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg)
   {
      ++bufModifiedCBCallCount;
      modifiedPosition = pos;
      modifiedInserted = nInserted;
      modifiedDeleted = nDeleted;
      modifiedRestyled = nRestyled;
      if (deletedText)
         modifiedDeletedText = deletedText;
   }

   void bufPreDeleteCB(int pos, int nDeleted, void* cbArg)
   {
      ++bufPreDeleteCBCallCount;
      preDeletePosition = pos;
      preDeleteDeleted = nDeleted;
   }
}


// --------------------------------------------------------------------------
class Ne_Text_Buffer_Test : public ::testing::Test
{
protected:
   virtual void SetUp()
   {
      buffer = BufCreate();
      BufAddModifyCB(buffer, bufModifiedCB, 0);
      BufAddPreDeleteCB(buffer, bufPreDeleteCB, 0);
      bufModifiedCBCallCount = 0;
      bufPreDeleteCBCallCount = 0;
      preDeletePosition = -1;
      preDeleteDeleted = -1;
   }

   virtual void TearDown()
   {
      BufFree(buffer);
   }

   Ne_Text_Buffer* buffer;
};

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, CreateFree)
{
   EXPECT_EQ(buffer->length, 0);

   char* text = BufGetAll(buffer);
   EXPECT_EQ(strlen(text), 0);
   free__(text);

   const char* str = BufAsString(buffer);
   EXPECT_EQ(strlen(str), 0);
}

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, SetGet)
{
   BufSetAll(buffer, "ABCDEF");
   EXPECT_EQ(bufPreDeleteCBCallCount, 1);
   EXPECT_EQ(preDeletePosition, 0);
   EXPECT_EQ(preDeleteDeleted, 0);

   EXPECT_EQ(bufModifiedCBCallCount, 1);
   EXPECT_EQ(modifiedPosition, 0);
   EXPECT_EQ(modifiedInserted, 6);
   EXPECT_EQ(modifiedDeleted, 0);
   EXPECT_EQ(modifiedDeletedText, "");
   
   EXPECT_STREQ(BufAsString(buffer), "ABCDEF");
}

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, GetRange)
{
   BufSetAll(buffer, "ABCDEF");

   char* text = BufGetRange(buffer, -1, 0);
   EXPECT_STREQ(text, "");
   free__(text);

   text = BufGetRange(buffer, 10, 15);
   EXPECT_STREQ(text, "");
   free__(text);

   text = BufGetRange(buffer, 2, 5);
   EXPECT_STREQ(text, "CDE");
   free__(text);

   text = BufGetRange(buffer, 5, 2);
   EXPECT_STREQ(text, "CDE");
   free__(text);
}
