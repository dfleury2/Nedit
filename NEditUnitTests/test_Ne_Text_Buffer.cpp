#include <gtest/gtest.h>

#include "../source/Ne_Text_Buffer.h"
#include "../util/utils.h"

// --------------------------------------------------------------------------
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

   EXPECT_EQ(BufGetCharacter(buffer, -1), '\0');
   EXPECT_EQ(BufGetCharacter(buffer, 10), '\0');
   EXPECT_EQ(BufGetCharacter(buffer, 0), 'A');
   EXPECT_EQ(BufGetCharacter(buffer, 1), 'B');
   EXPECT_EQ(BufGetCharacter(buffer, 5), 'F');
   EXPECT_EQ(BufGetCharacter(buffer, 6), '\0');
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

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, Insert)
{
   BufInsert(buffer, 0, "");
   EXPECT_STREQ(BufAsString(buffer), "");
   EXPECT_EQ(preDeletePosition, 0);

   BufInsert(buffer, 0, "B");
   EXPECT_STREQ(BufAsString(buffer), "B");
   EXPECT_EQ(preDeletePosition, 0);
   EXPECT_EQ(buffer->cursorPosHint, 1);

   BufInsert(buffer, -1, "A");
   EXPECT_STREQ(BufAsString(buffer), "AB");
   EXPECT_EQ(preDeletePosition, 0);
   EXPECT_EQ(buffer->cursorPosHint, 1);

   BufInsert(buffer, 5, "E");
   EXPECT_STREQ(BufAsString(buffer), "ABE");
   EXPECT_EQ(preDeletePosition, 2);
   EXPECT_EQ(buffer->cursorPosHint, 3);

   BufInsert(buffer, 2, "CD");
   EXPECT_STREQ(BufAsString(buffer), "ABCDE");
   EXPECT_EQ(preDeletePosition, 2);
   EXPECT_EQ(buffer->cursorPosHint, 4);
}

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, Replace)
{
   BufReplace(buffer, 0, 0, "ABCDEF");
   EXPECT_STREQ(BufAsString(buffer), "ABCDEF");

   BufReplace(buffer, 2, 2, "--");
   EXPECT_STREQ(BufAsString(buffer), "AB--CDEF");

   BufReplace(buffer, 0, 4, "x");
   EXPECT_STREQ(BufAsString(buffer), "xCDEF");
}

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, Remove)
{
   BufInsert(buffer, 0, "ABCDEF");

   BufRemove(buffer, 2, 0);
   EXPECT_STREQ(BufAsString(buffer), "CDEF");

   BufRemove(buffer, 3, 10);
   EXPECT_STREQ(BufAsString(buffer), "CDE");
}

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, StartEndCountOfLine)
{
   // A
   // BC
   // DEFG
   // H
   //
   // I
   BufSetAll(buffer, "A\nBC\nDEFG\nH\n\nI");
   EXPECT_EQ(BufStartOfLine(buffer, 0), 0);
   EXPECT_EQ(BufStartOfLine(buffer, 1), 0);
   EXPECT_EQ(BufStartOfLine(buffer, 2), 2);
   EXPECT_EQ(BufStartOfLine(buffer, 3), 2);
   EXPECT_EQ(BufStartOfLine(buffer, 4), 2);
   EXPECT_EQ(BufStartOfLine(buffer, 5), 5);
   EXPECT_EQ(BufStartOfLine(buffer, 8), 5);
   EXPECT_EQ(BufStartOfLine(buffer, 9), 5);
   EXPECT_EQ(BufStartOfLine(buffer, 10), 10);
   EXPECT_EQ(BufStartOfLine(buffer, 11), 10);
   EXPECT_EQ(BufStartOfLine(buffer, 12), 12);
   EXPECT_EQ(BufStartOfLine(buffer, 13), 13);
   EXPECT_EQ(BufStartOfLine(buffer, 100), 13);

   EXPECT_EQ(BufEndOfLine(buffer, 0), 1);
   EXPECT_EQ(BufEndOfLine(buffer, 1), 1);
   EXPECT_EQ(BufEndOfLine(buffer, 2), 4);
   EXPECT_EQ(BufEndOfLine(buffer, 3), 4);
   EXPECT_EQ(BufEndOfLine(buffer, 4), 4);
   EXPECT_EQ(BufEndOfLine(buffer, 5), 9);
   EXPECT_EQ(BufEndOfLine(buffer, 8), 9);
   EXPECT_EQ(BufEndOfLine(buffer, 9), 9);
   EXPECT_EQ(BufEndOfLine(buffer, 10), 11);
   EXPECT_EQ(BufEndOfLine(buffer, 11), 11);
   EXPECT_EQ(BufEndOfLine(buffer, 12), 12);
   EXPECT_EQ(BufEndOfLine(buffer, 13), 14);
   EXPECT_EQ(BufEndOfLine(buffer, 100), 14);

   EXPECT_EQ(BufCountLines(buffer, 0, 0), 0);
   EXPECT_EQ(BufCountLines(buffer, 0, 1), 0);
   EXPECT_EQ(BufCountLines(buffer, 0, 2), 1);
   EXPECT_EQ(BufCountLines(buffer, 0, 5), 2);
   EXPECT_EQ(BufCountLines(buffer, 1, 3), 1);
   EXPECT_EQ(BufCountLines(buffer, -1, 100), 5);
}

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, Search)
{
   BufSetAll(buffer, "A\nBC\nDEFG\nH\n\nI");
   int foundPosition = -1;
   EXPECT_EQ(BufSearchForward(buffer, 0, "DH", &foundPosition), 1);
   EXPECT_EQ(foundPosition, 5);
   EXPECT_EQ(BufSearchBackward(buffer, 4, "DH", &foundPosition), 0);
   EXPECT_EQ(foundPosition, 0);

   EXPECT_EQ(BufSearchForward(buffer, 6, "DH", &foundPosition), 1);
   EXPECT_EQ(foundPosition, 10);
   EXPECT_EQ(BufSearchBackward(buffer, 6, "DH", &foundPosition), 1);
   EXPECT_EQ(foundPosition, 5);

   EXPECT_EQ(BufSearchForward(buffer, 11, "DH", &foundPosition), 0);
   EXPECT_EQ(foundPosition, 14);
   EXPECT_EQ(BufSearchBackward(buffer, 11, "DH", &foundPosition), 1);
   EXPECT_EQ(foundPosition, 10);

   EXPECT_EQ(BufSearchForward(buffer, 0, "I", &foundPosition), 1);
   EXPECT_EQ(foundPosition, 13);

   EXPECT_EQ(BufSearchBackward(buffer, 100, "I", &foundPosition), 1);
   EXPECT_EQ(foundPosition, 13);

   EXPECT_EQ(BufSearchForward(buffer, 0, "X", &foundPosition), 0);
   EXPECT_EQ(foundPosition, 14);
   EXPECT_EQ(BufSearchBackward(buffer, 10, "X", &foundPosition), 0);
   EXPECT_EQ(foundPosition, 0);

   EXPECT_EQ(BufSearchBackward(buffer, 0, "A", &foundPosition), 0);
   EXPECT_EQ(foundPosition, 0);
}

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, Cmp)
{
   BufSetAll(buffer, "A\nBC\nDEFG\nH\n\nI");

   EXPECT_EQ(BufCmp(buffer, 0, 14, "A\nBC\nDEFG\nH\n\nI"), 0);
   EXPECT_EQ(BufCmp(buffer, 0, 15, "A\nBC\nDEFG\nH\n\nI"), 1);
   EXPECT_EQ(BufCmp(buffer, -1, 14, "A\nBC\nDEFG\nH\n\nI"), -1);
   EXPECT_EQ(BufCmp(buffer, 0, 0, "A\nBC\nDEFG\nH\n\nI"), 0);
   EXPECT_EQ(BufCmp(buffer, 8, 10, "A\nBC\nDEFG\nH\n\nI"), 1);
   EXPECT_EQ(BufCmp(buffer, 8, 6, "G\nH\n\nI"), 0);
}

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, SubstituteNullChar)
{
   BufSetAll(buffer, "ABCDEFGH");
   ((char*)(BufAsString(buffer)))[5] = '\0';
   
   char str[] = "ABCDEFGH";
   str[4] = '\0';

   BufUnsubstituteNullChars(str, buffer);
   EXPECT_EQ(str[4], '\0');

   BufSubstituteNullChars(str, 8, buffer);
   EXPECT_EQ(BufGetCharacter(buffer, 5), 1);
   EXPECT_EQ(str[4], 1);
   EXPECT_EQ(buffer->nullSubsChar, 1);

   BufUnsubstituteNullChars(str, buffer);
   EXPECT_EQ(str[4], '\0');
}
