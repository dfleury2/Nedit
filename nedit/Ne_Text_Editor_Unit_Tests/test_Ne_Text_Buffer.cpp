#include "../../../libCppUT/Test.h"

#include "../Ne_Text_Editor/Ne_Text_Buffer.h"

// --------------------------------------------------------------------------
TEST(Get, Ne_Text_Buffer)
{
   const char* buffer = "AAABBBCCC\nEEEFFFGGG";
   Ne_Text_Buffer text;
   text.setAll(buffer);

   ASSERT_EQUAL(text.getAll(), buffer);
   ASSERT_EQUAL(text.getCharacter(0), 'A');
   ASSERT_EQUAL(text.getCharacter(9), '\n');
   ASSERT_EQUAL(text.getCharacter(10), 'E');
}

// --------------------------------------------------------------------------
TEST(Insert, Ne_Text_Buffer)
{
   Ne_Text_Buffer text;
   text.insertAt(0, "BBB");
   ASSERT_EQUAL(text.getAll(), "BBB");

   text.insertAt(0, "AAA");
   ASSERT_EQUAL(text.getAll(), "AAABBB");

   text.insertAt(100, "\nEEEFFFGGG");
   ASSERT_EQUAL(text.getAll(), "AAABBB\nEEEFFFGGG");
}

// --------------------------------------------------------------------------
TEST(Remove, Ne_Text_Buffer)
{
   const char* buffer = "AAABBBCCC\nEEEFFFGGG";
   Ne_Text_Buffer text;
   text.setAll(buffer);

   text.remove(5, 7);
   ASSERT_EQUAL(text.getAll(), "AAABBCC\nEEEFFFGGG");

   text.remove(0, 5);
   ASSERT_EQUAL(text.getAll(), "CC\nEEEFFFGGG");
}

// --------------------------------------------------------------------------
TEST(Replace, Ne_Text_Buffer)
{
   const char* buffer = "AAABBBCCC\nEEEFFFGGG";
   Ne_Text_Buffer text;
   text.setAll(buffer);

   text.replace(5, 7, "TEXT");
   ASSERT_EQUAL(text.getAll(), "AAABBTEXTCC\nEEEFFFGGG");
}

// --------------------------------------------------------------------------
TEST(Selection, Ne_Text_Buffer)
{
   const char* buffer = "AAABBBCCC\nEEEFFFGGG";
   Ne_Text_Buffer text;
   text.setAll(buffer);

   std::string selection = text.getSelectionText();
   ASSERT_EQUAL(selection, "");
   text.select(3, 7);

   selection = text.getSelectionText();
   ASSERT_EQUAL(selection, "BBBC");

   text.unselect();
   selection = text.getSelectionText();
   ASSERT_EQUAL(selection, "");
}
