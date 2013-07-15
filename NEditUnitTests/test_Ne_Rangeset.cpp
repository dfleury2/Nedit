#include <gtest/gtest.h>

#include "../source/Ne_Rangeset.h"
#include "../source/Ne_Text_Buffer.h"

// --------------------------------------------------------------------------
TEST(Ne_Rangeset_Test, CreateCloneAndFree)
{
   Ne_Text_Buffer* buffer = BufCreate();
   Ne_Text_Buffer* destBuffer = BufCreate();

   buffer->rangesetTable = RangesetTableAlloc(buffer);
   ASSERT_EQ(buffer->rangesetTable->n_set, 0);
   ASSERT_EQ(buffer->rangesetTable->buf, buffer);

   destBuffer->rangesetTable = RangesetTableClone(buffer->rangesetTable, destBuffer); 
   ASSERT_EQ(destBuffer->rangesetTable->n_set, 0);
   ASSERT_EQ(destBuffer->rangesetTable->buf, destBuffer);

   ASSERT_EQ(nRangesetsAvailable(buffer->rangesetTable), 63);
   ASSERT_EQ(RangesetCreate(buffer->rangesetTable), 58);
   ASSERT_EQ(nRangesetsAvailable(buffer->rangesetTable), 62);
   ASSERT_EQ(RangesetCreate(buffer->rangesetTable), 10);
   ASSERT_EQ(nRangesetsAvailable(buffer->rangesetTable), 61);

   RangesetForget(buffer->rangesetTable, 58);
   ASSERT_EQ(nRangesetsAvailable(buffer->rangesetTable), 62);
   
   ASSERT_EQ(RangesetCreate(buffer->rangesetTable), 58);
   ASSERT_EQ(nRangesetsAvailable(buffer->rangesetTable), 61);

   Rangeset* rangeset58 = RangesetFetch(buffer->rangesetTable, 58);
   RangesetAddBetween(rangeset58, 0, 10);

   Rangeset* rangeset27 = RangesetFetch(buffer->rangesetTable, 27);
   ASSERT_EQ(nRangesetsAvailable(buffer->rangesetTable), 61);

   ASSERT_EQ(RangesetAssignColorName(rangeset58, "red"), 1);
   ASSERT_EQ(RangesetAssignName(rangeset58, "rangeset58"), 1);
   ASSERT_STREQ(RangesetGetName(rangeset58), "rangeset58");

   Fl_Color color = fl_rgb_color(128,0,0);
   ASSERT_EQ(RangesetAssignColorPixel(rangeset58, color, 1), 1);
   Fl_Color ret;
   ASSERT_EQ(RangesetGetColorValid(rangeset58, &ret), 1);
   ASSERT_EQ(color, ret);

   ASSERT_STREQ(RangesetTableGetColorName(buffer->rangesetTable, 0), "red");

   ASSERT_EQ(RangesetTableGetColorValid(buffer->rangesetTable, 0, &ret), 1);
   ASSERT_EQ(color, ret);

   Fl_Color colorGreen = fl_rgb_color(0,128,0);
   RangesetTableAssignColorPixel(buffer->rangesetTable, 1, colorGreen, 1);

   ASSERT_EQ(RangesetInverse(0), -1);
   ASSERT_EQ(RangesetInverse(rangeset58), 1);

   BufFree(buffer);
   BufFree(destBuffer);
}

// --------------------------------------------------------------------------
TEST(Ne_Rangeset_Test, Range)
{
   Ne_Text_Buffer* buffer = BufCreate();
   BufSetAll(buffer, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

   buffer->rangesetTable = RangesetTableAlloc(buffer);
   RangesetCreate(buffer->rangesetTable);
   Rangeset* rangeset58 = RangesetFetch(buffer->rangesetTable, 58);
   ASSERT_NE(rangeset58, (void*)0);

   ASSERT_EQ(RangesetCheckRangeOfPos(rangeset58, 0), -1);

   ASSERT_EQ(RangesetGetNRanges(rangeset58), 0);
   ASSERT_EQ(RangesetAddBetween(rangeset58, 0, 0), 0);
   ASSERT_EQ(RangesetAddBetween(rangeset58, 0, 10), 1);
   ASSERT_EQ(RangesetAddBetween(rangeset58, 12, 15), 2);

   // Range getter
   int start = 0, end= 0;
   ASSERT_EQ(RangesetFindRangeNo(rangeset58, 0, &start, &end), 1);
   ASSERT_EQ(start, 0);
   ASSERT_EQ(end, 10);

   ASSERT_EQ(RangesetFindRangeNo(rangeset58, 1, &start, &end), 1);
   ASSERT_EQ(start, 12);
   ASSERT_EQ(end, 15);

   ASSERT_EQ(RangesetFindRangeNo(rangeset58, 2, &start, &end), 0);

   // Position find in a range
   ASSERT_EQ(RangesetFindRangeOfPos(0, 0, 0), -1);
   ASSERT_EQ(RangesetFindRangeOfPos(rangeset58, 18, 0), -1);
   ASSERT_EQ(RangesetFindRangeOfPos(rangeset58, 15, 0), -1);
   ASSERT_EQ(RangesetFindRangeOfPos(rangeset58, 15, 1), 1);
   ASSERT_EQ(RangesetFindRangeOfPos(rangeset58, 9, 0), 0);

   ASSERT_EQ(RangesetCheckRangeOfPos(rangeset58, 0), 0);


   // Add overlapped range
   ASSERT_EQ(RangesetAddBetween(rangeset58, 20, 5), 1);

   ASSERT_EQ(RangesetRemoveBetween(rangeset58, 12, 8), 2);
   ASSERT_EQ(RangesetRemoveBetween(rangeset58, 0, 20), 0);
   ASSERT_EQ(RangesetRemoveBetween(rangeset58, 2,2), 0);

   BufFree(buffer);
}

// --------------------------------------------------------------------------
TEST(Ne_Rangeset_Test, RangeAddRemove)
{
   Ne_Text_Buffer* buffer = BufCreate();
   BufSetAll(buffer, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

   buffer->rangesetTable = RangesetTableAlloc(buffer);
   RangesetCreate(buffer->rangesetTable);
   RangesetCreate(buffer->rangesetTable);
   RangesetCreate(buffer->rangesetTable);

   Rangeset* rangeset58 = RangesetFetch(buffer->rangesetTable, 58);
   ASSERT_NE(rangeset58, (void*)0);
   Rangeset* rangeset10 = RangesetFetch(buffer->rangesetTable, 10);
   ASSERT_NE(rangeset10, (void*)0);
   Rangeset* rangeset15 = RangesetFetch(buffer->rangesetTable, 15);
   ASSERT_NE(rangeset15, (void*)0);

   RangesetAddBetween(rangeset58, 0, 0);
   RangesetAddBetween(rangeset58, 0, 10);
   RangesetAddBetween(rangeset58, 12, 15);

   ASSERT_EQ(RangesetAdd(rangeset58, rangeset10), 2);

   RangesetAddBetween(rangeset10, 10, 12);

   ASSERT_EQ(RangesetAdd(rangeset58, rangeset10), 1);
   
   ASSERT_EQ(RangesetGetNRanges(rangeset58), 1);
   ASSERT_EQ(RangesetAdd(rangeset15, rangeset58), 1);

   BufFree(buffer);
}

// --------------------------------------------------------------------------
TEST(Ne_Rangeset_Test, RangeRemove)
{
   Ne_Text_Buffer* buffer = BufCreate();
   BufSetAll(buffer, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

   buffer->rangesetTable = RangesetTableAlloc(buffer);
   RangesetCreate(buffer->rangesetTable);
   RangesetCreate(buffer->rangesetTable);
   RangesetCreate(buffer->rangesetTable);

   Rangeset* rangeset58 = RangesetFetch(buffer->rangesetTable, 58);
   ASSERT_NE(rangeset58, (void*)0);
   Rangeset* rangeset10 = RangesetFetch(buffer->rangesetTable, 10);
   ASSERT_NE(rangeset10, (void*)0);

   ASSERT_EQ(RangesetRemove(rangeset58, rangeset10), 0);

   RangesetAddBetween(rangeset58, 5, 15);
   RangesetAddBetween(rangeset10, 8, 10);

   ASSERT_EQ(RangesetRemove(rangeset58, rangeset10), 2);

   BufFree(buffer);
}
