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

   BufFree(buffer);
   BufFree(destBuffer);
}
