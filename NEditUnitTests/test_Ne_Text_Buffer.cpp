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
      BufRemoveModifyCB(buffer, bufModifiedCB, 0);
      BufRemovePreDeleteCB(buffer, bufPreDeleteCB, 0);
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

   BufCheckDisplay(buffer, 0, 5);
   EXPECT_EQ(bufModifiedCBCallCount, 2);
   EXPECT_EQ(modifiedPosition, 0);
   EXPECT_EQ(modifiedInserted, 0);
   EXPECT_EQ(modifiedDeleted, 0);
   EXPECT_EQ(modifiedDeletedText, "");
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

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, ExpandCharWidth)
{
   EXPECT_EQ(BufCharWidth('\0', 4, 3, '\0'), 5);
   EXPECT_EQ(BufCharWidth('\1', 4, 3, '\0'), 5);

   // Tabs and indent
   // Example:
   // tabs = 8
   // 0------70------7
   // AB\t    C\t
   // First \t -> 6 chars width
   // Second \t ->7 chars width
   EXPECT_EQ(BufCharWidth('\t', 0, 3, '\0'), 3);
   EXPECT_EQ(BufCharWidth('\t', 1, 3, '\0'), 2);
   EXPECT_EQ(BufCharWidth('\t', 2, 3, '\0'), 1);
   EXPECT_EQ(BufCharWidth('\t', 3, 3, '\0'), 3);
   EXPECT_EQ(BufCharWidth('\t', 4, 3, '\0'), 2);
   EXPECT_EQ(BufCharWidth('\t', 5, 3, '\0'), 1);
   EXPECT_EQ(BufCharWidth('\t', 6, 3, '\0'), 3);

   EXPECT_EQ(BufCharWidth(char(8), 5, 3, '\0'), 4);
   EXPECT_EQ(BufCharWidth(char(127), 5, 3, '\0'), 5);

   EXPECT_EQ(BufCharWidth('A', 5, 3, '\0'), 1);

   BufSetAll(buffer, "AB_DE_GH_JK_");
   ((char*)(BufAsString(buffer)))[2] = '\t';
   ((char*)(BufAsString(buffer)))[5] = '\0';
   ((char*)(BufAsString(buffer)))[7] = char(127);
   ((char*)(BufAsString(buffer)))[11] = char(2);

   char str[MAX_EXP_CHAR_LEN];
   memset(&str[0], '\0', sizeof(str));
   EXPECT_EQ(BufGetExpandedChar(buffer, 0, 8, str), 1);
   EXPECT_STREQ(str, "A");

   memset(&str[0], '\0', sizeof(str));
   EXPECT_EQ(BufGetExpandedChar(buffer, 1, 8, str), 1);
   EXPECT_STREQ(str, "B");

   memset(&str[0], '\0', sizeof(str));
   EXPECT_EQ(BufGetExpandedChar(buffer, 2, 8, str), 8);
   EXPECT_STREQ(str, "        ");

   memset(&str[0], '\0', sizeof(str));
   EXPECT_EQ(BufGetExpandedChar(buffer, 3, 8, str), 1);
   EXPECT_STREQ(str, "D");

   memset(&str[0], '\0', sizeof(str));
   EXPECT_EQ(BufGetExpandedChar(buffer, 5, 8, str), 5);
   EXPECT_STREQ(str, "<nul>");

   memset(&str[0], '\0', sizeof(str));
   EXPECT_EQ(BufGetExpandedChar(buffer, 7, 8, str), 5);
   EXPECT_STREQ(str, "<del>");

   memset(&str[0], '\0', sizeof(str));
   EXPECT_EQ(BufGetExpandedChar(buffer, 11, 8, str), 5);
   EXPECT_STREQ(str, "<stx>");

   // Only tabs
   BufSetAll(buffer, "\t\t\t");
   memset(&str[0], '\0', sizeof(str));
   EXPECT_EQ(BufGetExpandedChar(buffer, 0, 8, str), 8);
   EXPECT_STREQ(str, "        ");

   memset(&str[0], '\0', sizeof(str));
   EXPECT_EQ(BufGetExpandedChar(buffer, 1, 8, str), 8);
   EXPECT_STREQ(str, "        ");
}

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, CountDisplChars)
{
   BufSetAll(buffer, "AB\tC\tD\nEFG\tH");
   EXPECT_EQ(BufGetTabDistance(buffer), 8);
   BufSetTabDistance(buffer, 12);
   EXPECT_EQ(BufGetTabDistance(buffer), 12);

   EXPECT_EQ(BufCountDispChars(buffer, 0, 0), 0);
   EXPECT_EQ(BufCountDispChars(buffer, 0, 1), 1);
   EXPECT_EQ(BufCountDispChars(buffer, 0, 2), 2);
   EXPECT_EQ(BufCountDispChars(buffer, 0, 3), 12);
   EXPECT_EQ(BufCountDispChars(buffer, 0, 4), 13);
   EXPECT_EQ(BufCountDispChars(buffer, 0, 5), 24);
   EXPECT_EQ(BufCountDispChars(buffer, 0, 6), 25);

   EXPECT_EQ(BufCountDispChars(buffer, 2, 5), 24);
   EXPECT_EQ(BufCountDispChars(buffer, 2, 6), 25);

   EXPECT_EQ(BufCountDispChars(buffer, 3, 5), 12);
   EXPECT_EQ(BufCountDispChars(buffer, 4, 6), 13);

   EXPECT_EQ(BufCountDispChars(buffer, 0, 100), 37);

   EXPECT_EQ(BufCountForwardDispChars(buffer, 100, 10), 100);
   EXPECT_EQ(BufCountForwardDispChars(buffer, 37, 10), 37);

   EXPECT_EQ(BufCountForwardDispChars(buffer, 25, 10), 25);

   EXPECT_EQ(BufCountForwardDispChars(buffer, 11, 10), 12);
   EXPECT_EQ(BufCountForwardDispChars(buffer, 10, 10), 11);
   EXPECT_EQ(BufCountForwardDispChars(buffer, 10, 1), 11);

   EXPECT_EQ(BufCountForwardDispChars(buffer, 8, 10), 11);
   EXPECT_EQ(BufCountForwardDispChars(buffer, 8, 1), 9);

   EXPECT_EQ(BufCountForwardDispChars(buffer, 0, 10), 3);
   EXPECT_EQ(BufCountForwardDispChars(buffer, 0, 25), 6);
   EXPECT_EQ(BufCountForwardDispChars(buffer, 0, 100), 6);
}

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, InsertCol)
{
   BufSetAll(buffer,
      "AB\n"
      "C\n"
      "\n"
      "D\n"
      "EFG\n"
      "H\n");

   int charsInserted, charsDeleted;
   BufInsertCol(buffer, 2, 5, "XYZ", &charsInserted, &charsDeleted);
   EXPECT_STREQ(BufAsString(buffer),
      "AB\n"
      "C\n"
      "  XYZ\n"
      "D\n"
      "EFG\n"
      "H\n");
   EXPECT_EQ(charsInserted, 5);
   EXPECT_EQ(charsDeleted, 0);

   BufInsertCol(buffer, 2, 16, "xyz", &charsInserted, &charsDeleted);
   EXPECT_STREQ(BufAsString(buffer),
      "AB\n"
      "C\n"
      "  XYZ\n"
      "D\n"
      "EFxyzG\n"
      "H\n");
   EXPECT_EQ(charsInserted, 6);
   EXPECT_EQ(charsDeleted, 3);
}

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, Rectangular)
{
   BufSetAll(buffer,
      "AB\n"
      "C\n"
      "\n"
      "D\n"
      "EFG\n"
      "H\n");

   int charsInserted, charsDeleted;
   BufOverlayRect(buffer, 5, 3, -1, "XY\nZ", &charsInserted, &charsDeleted);
   EXPECT_STREQ(BufAsString(buffer),
      "AB\n"
      "C\n"
      "   XY\n"
      "D  Z\n"
      "EFG\n"
      "H\n");
   EXPECT_EQ(charsInserted, 10);
   EXPECT_EQ(charsDeleted, 2);

   BufOverlayRect(buffer, 2, 2, 5, "xxx\ny\nz\nuuuuu\nv", &charsInserted, &charsDeleted);
   EXPECT_STREQ(BufAsString(buffer),
      "ABxxx\n"
      "C y\n"
      "  z\n"
      "D uuuuu\n"
      "EFv\n"
      "H\n");
   EXPECT_EQ(charsInserted, 25);
   EXPECT_EQ(charsDeleted, 19);

   BufReplaceRect(buffer, 0, 2, 5, 3, "a\nbb\nccc\n\nd");
   EXPECT_STREQ(BufAsString(buffer),
      "ABxxxa\n"
      "     bb\n"
      "     ccc\n"
      "\n"
      "     d\n"
      "C y\n"
      "  z\n"
      "D uuuuu\n"
      "EFv\n"
      "H\n");

   BufReplaceRect(buffer, 25, 32, 1, 2, "XXX\nYY\nZZZ");
   EXPECT_STREQ(BufAsString(buffer),
      "ABxxxa\n"
      "     bb\n"
      "     ccc\n"
      "\n"
      " XXX   d\n"
      "CYY y\n"
      " ZZZ\n"
      "  z\n"
      "D uuuuu\n"
      "EFv\n"
      "H\n");

   BufRemoveRect(buffer, 3, 32, 1, 3);
   EXPECT_STREQ(BufAsString(buffer),
      "Axxa\n"
      "   bb\n"
      "   ccc\n"
      "\n"
      " X   d\n"
      "CYY y\n"
      " ZZZ\n"
      "  z\n"
      "D uuuuu\n"
      "EFv\n"
      "H\n");

   BufClearRect(buffer, 0, 64, 2, 5);
   EXPECT_STREQ(BufAsString(buffer),
      "Ax\n"
      " \n"
      "     c\n"
      "\n"
      " X   d\n"
      "CY\n"
      " Z\n"
      " \n"
      "D    uu\n"
      "EF\n"
      "H\n");

   char* text = BufGetTextInRect(buffer, 0, 64, 1, 3);
   EXPECT_STREQ(text,
      "x\n"
      "\n"
      "  \n"
      "\n"
      "X \n"
      "Y\n"
      "Z\n"
      "\n"
      "  \n"
      "F\n"
      "\n"
      );
   free__(text);

}

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, CountNLines)
{
   BufSetAll(buffer,
      "AB\n"
      "C\n"
      "\n"
      "D\n"
      "EFG\n"
      "H\n");
   EXPECT_EQ(BufCountForwardNLines(buffer, 0, 0), 0);
   EXPECT_EQ(BufCountForwardNLines(buffer, 0, 1), 3);
   EXPECT_EQ(BufCountForwardNLines(buffer, 0, 2), 5);
   EXPECT_EQ(BufCountForwardNLines(buffer, 0, 3), 6);
   EXPECT_EQ(BufCountForwardNLines(buffer, 0, 4), 8);
   EXPECT_EQ(BufCountForwardNLines(buffer, 0, 5), 12);
   EXPECT_EQ(BufCountForwardNLines(buffer, 0, 6), 14);
   EXPECT_EQ(BufCountForwardNLines(buffer, 0, 7), 14);
   EXPECT_EQ(BufCountForwardNLines(buffer, 0, 8), 14);

   EXPECT_EQ(BufCountForwardNLines(buffer, 5, 0), 5);
   EXPECT_EQ(BufCountForwardNLines(buffer, 5, 1), 6);
   EXPECT_EQ(BufCountForwardNLines(buffer, 5, 2), 8);
   EXPECT_EQ(BufCountForwardNLines(buffer, 5, 3), 12);
   EXPECT_EQ(BufCountForwardNLines(buffer, 5, 4), 14);
   EXPECT_EQ(BufCountForwardNLines(buffer, 5, 5), 14);
   EXPECT_EQ(BufCountForwardNLines(buffer, 5, 6), 14);
   EXPECT_EQ(BufCountForwardNLines(buffer, 5, 7), 14);
   EXPECT_EQ(BufCountForwardNLines(buffer, 5, 8), 14);

   EXPECT_EQ(BufCountBackwardNLines(buffer, 25, 0), 14);
   EXPECT_EQ(BufCountBackwardNLines(buffer, 15, 0), 14);
   EXPECT_EQ(BufCountBackwardNLines(buffer, 10, 0), 8);
   EXPECT_EQ(BufCountBackwardNLines(buffer, 10, 1), 6);
   EXPECT_EQ(BufCountBackwardNLines(buffer, 10, 2), 5);
   EXPECT_EQ(BufCountBackwardNLines(buffer, 10, 3), 3);

   EXPECT_EQ(BufCountBackwardNLines(buffer, 5, 0), 5);
   EXPECT_EQ(BufCountBackwardNLines(buffer, 5, 1), 3);
   EXPECT_EQ(BufCountBackwardNLines(buffer, 5, 2), 0);
   EXPECT_EQ(BufCountBackwardNLines(buffer, 5, 3), 0);
   EXPECT_EQ(BufCountBackwardNLines(buffer, 5, 4), 0);
   EXPECT_EQ(BufCountBackwardNLines(buffer, 5, 5), 0);

   EXPECT_EQ(BufCountBackwardNLines(buffer, -5, 3), 0);
}

// --------------------------------------------------------------------------
TEST_F(Ne_Text_Buffer_Test, GetSetSelection)
{
   BufSetAll(buffer,
      "0123456789\n"
      "1234567890\n"
      "2345678901\n"
      "3456789012\n"
      "4567890123\n"
      "5678901234\n");

   char* text = BufGetSelectionText(buffer);
   EXPECT_STREQ(text, "");
   free__(text);

   BufSelect(buffer, 5, 13);
   text = BufGetSelectionText(buffer);
   EXPECT_STREQ(text, "56789\n12");
   free__(text);

   BufRemoveSelected(buffer);
   EXPECT_STREQ(BufAsString(buffer),
      "0123434567890\n"
      "2345678901\n"
      "3456789012\n"
      "4567890123\n"
      "5678901234\n");
   
   text = BufGetSelectionText(buffer);
   EXPECT_STREQ(text, "");
   free__(text);

   BufSelect(buffer, 5, 8);
   text = BufGetSelectionText(buffer);
   EXPECT_STREQ(text, "345");
   free__(text);

   BufUnselect(buffer);
   text = BufGetSelectionText(buffer);
   EXPECT_STREQ(text, "");
   free__(text);
}
