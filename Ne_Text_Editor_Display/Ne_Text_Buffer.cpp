#include "Ne_Text_Buffer.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <algorithm>

using namespace std;


static void histogramCharacters(const char* string, int length, bool hist[256], bool init);

static void subsChars(char* str, int length, char fromChar, char toChar);
static char chooseNullSubsChar(bool hist[256]);

static const char* ControlCodeTable[32] =
{
   "nul", "soh", "stx", "etx", "eot", "enq", "ack", "bel",
   "bs", "ht", "nl", "vt", "np", "cr", "so", "si",
   "dle", "dc1", "dc2", "dc3", "dc4", "nak", "syn", "etb",
   "can", "em", "sub", "esc", "fs", "gs", "rs", "us"
};

// --------------------------------------------------------------------------
Ne_Text_Selection::Ne_Text_Selection()
{
   selected = false;
   start = end = 0;
}

// --------------------------------------------------------------------------
// Create an empty text buffer of a pre-determined size
// --------------------------------------------------------------------------
Ne_Text_Buffer::Ne_Text_Buffer(int requestedSize)
{
   buffer.reserve(requestedSize);
   nullSubsChar = '\0';
   cursorPosHint = 0;
   tabDist = 8;
   rangesetTable = NULL;
}

// --------------------------------------------------------------------------
const std::string& Ne_Text_Buffer::getAll() const
{
   return buffer;
}

// --------------------------------------------------------------------------
// Replace the entire contents of the text buffer
// --------------------------------------------------------------------------
void Ne_Text_Buffer::setAll(const char* text)
{
   callPreDeleteCBs(0, buffer.size());

   // Save information for redisplay, and get rid of the old buffer
   std::string deletedText = getAll();

   // Start a new buffer
   buffer = text;

   // Zero all of the existing selections
   updateSelections(0, deletedText.size(), 0);

   // Call the saved display routine(s) to update the screen
   callModifyCBs(0, deletedText.size(), buffer.size(), 0, deletedText.c_str());
}

// --------------------------------------------------------------------------
// Return a copy of the text between "start" and "end" character positions
// from text buffer "buf".  Positions start at 0, and the range does not
// include the character pointed to by "end"
// --------------------------------------------------------------------------
std::string Ne_Text_Buffer::getRange(int start, int end) const
{
   if (start < 0 || start > buffer.size() || start == end) return "";

   if (end < start) std::swap(end, start);
   if (end > buffer.size()) end = buffer.size();

   return buffer.substr(start, end - start);
}

// --------------------------------------------------------------------------
// Return the character at buffer position "pos".  Positions start at 0.
// --------------------------------------------------------------------------
char Ne_Text_Buffer::getCharacter(int pos) const
{
   if (pos < 0 || pos >= buffer.size()) return '\0';
   return buffer[pos];
}

// --------------------------------------------------------------------------
// Insert null-terminated string "text" at position "pos" in "buf"
// --------------------------------------------------------------------------
void Ne_Text_Buffer::insertAt(int pos, const char* text)
{
   // if pos is not contiguous to existing text, make it
   if (pos > buffer.size()) pos = buffer.size();
   if (pos < 0) pos = 0;

   // Even if nothing is deleted, we must call these callbacks
   callPreDeleteCBs(pos, 0);

   // insert and redisplay
   int nInserted = insert(pos, text);
   cursorPosHint = pos + nInserted;
   callModifyCBs(pos, 0, nInserted, 0, NULL);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::remove(int start, int end)
{
   // Make sure the arguments make sense
   if (start > end) std::swap(start, end);

   if (start > buffer.size()) start = buffer.size();
   if (start < 0) start = 0;
   if (end > buffer.size()) end = buffer.size();
   if (end < 0) end = 0;

   callPreDeleteCBs(start, end - start);

   // Remove and redisplay
   std::string deletedText = getRange(start, end);
   deleteBuf(start, end);

   cursorPosHint = start;
   callModifyCBs(start, end - start, 0, 0, deletedText.c_str());
}

// --------------------------------------------------------------------------
// Delete the characters between "start" and "end", and insert the
// null-terminated string "text" in their place in in "buf"
// --------------------------------------------------------------------------
void Ne_Text_Buffer::replace(int start, int end, const char* text)
{
   int nInserted = strlen(text);

   callPreDeleteCBs(start, end - start);
   std::string deletedText = getRange(start, end);
   deleteBuf(start, end);
   insert(start, text);

   cursorPosHint = start + nInserted;
   callModifyCBs(start, end - start, nInserted, 0, deletedText.c_str());
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::copyFromBuf(const Ne_Text_Buffer& fromBuf, int fromStart, int fromEnd, int toPos)
{
   int length = fromEnd - fromStart;

   // Insert the new text
   buffer.insert(toPos, fromBuf.getAll(), fromStart, length );

   updateSelections(toPos, 0, length);
}

// --------------------------------------------------------------------------
// Get the hardware tab distance used by all displays for this buffer,
// and used in computing offsets for rectangular selection operations.
// --------------------------------------------------------------------------
int Ne_Text_Buffer::getTabDistance() const
{
   return tabDist;
}

// --------------------------------------------------------------------------
// Set the hardware tab distance used by all displays for this buffer,
// and used in computing offsets for rectangular selection operations.
// --------------------------------------------------------------------------
void Ne_Text_Buffer::setTabDistance(int tabDist)
{
   // First call the pre-delete callbacks with the previous tab setting still active.
   callPreDeleteCBs(0, buffer.size());

   // Change the tab setting
   this->tabDist = tabDist;

   // Force any display routines to redisplay everything
   const std::string& deletedText = getAll();
   callModifyCBs(0, buffer.size(), buffer.size(), 0, deletedText.c_str());
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::checkDisplay(int start, int end)
{
   // just to make sure colors in the selected region are up to date
   callModifyCBs(start, 0, 0, end - start, NULL);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::select(int start, int end)
{
   Ne_Text_Selection oldSelection = primary;

   setSelection(primary, start, end);
   redisplaySelection(oldSelection, primary);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::unselect()
{
   Ne_Text_Selection oldSelection = primary;

   primary.selected = false;
   redisplaySelection(oldSelection, primary);
}

// --------------------------------------------------------------------------
bool Ne_Text_Buffer::getSelectionPos(int* start, int* end)
{
   return primary.getSelectionPos(start, end);
}

// --------------------------------------------------------------------------
std::string Ne_Text_Buffer::getSelectionText() const
{
   return getSelectionText(primary);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::removeSelected()
{
   removeSelected(primary);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::replaceSelected(const char* text)
{
   replaceSelected(primary, text);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::secondarySelect(int start, int end)
{
   Ne_Text_Selection oldSelection = secondary;

   setSelection(secondary, start, end);
   redisplaySelection(oldSelection, secondary);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::secondaryUnselect()
{
   Ne_Text_Selection oldSelection = secondary;

   secondary.selected = false;
   redisplaySelection(oldSelection, secondary);
}

// --------------------------------------------------------------------------
bool Ne_Text_Buffer::getSecSelectPos(int* start, int* end)
{
   return secondary.getSelectionPos(start, end);
}

// --------------------------------------------------------------------------
std::string Ne_Text_Buffer::getSecSelectText() const
{
   return getSelectionText(secondary);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::removeSecSelect()
{
   removeSelected(secondary);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::replaceSecSelect(const char* text)
{
   replaceSelected(secondary, text);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::setHighlight(int start, int end)
{
   Ne_Text_Selection oldSelection = highlight;

   setSelection(this->highlight, start, end);
   redisplaySelection(oldSelection, highlight);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::unsetHighlight()
{
   Ne_Text_Selection oldSelection = highlight;

   highlight.selected = false;
   redisplaySelection(oldSelection, highlight);
}

// --------------------------------------------------------------------------
bool Ne_Text_Buffer::getHighlightPos(int* start, int* end) const
{
   return highlight.getSelectionPos(start, end);
}

// --------------------------------------------------------------------------
// Add a callback routine to be called when the buffer is modified
// --------------------------------------------------------------------------
void Ne_Text_Buffer::addModifyCB(NeBufModifyCallbackProc* bufModifiedCB, void* cbArg)
{
   modifyProcs.push_back(Ne_Text_Modify_Callback(bufModifiedCB, cbArg));
}

// --------------------------------------------------------------------------
// Similar to the above, but makes sure that the callback is called before
// normal priority callbacks.
// --------------------------------------------------------------------------
void Ne_Text_Buffer::addHighPriorityModifyCB(NeBufModifyCallbackProc* bufModifiedCB, void* cbArg)
{
   modifyProcs.insert(modifyProcs.begin(), Ne_Text_Modify_Callback(bufModifiedCB, cbArg));
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::removeModifyCB(NeBufModifyCallbackProc* bufModifiedCB, void* cbArg)
{
   std::vector<Ne_Text_Modify_Callback>::iterator found
      = std::find(modifyProcs.begin(), modifyProcs.end(),
                  Ne_Text_Modify_Callback(bufModifiedCB, cbArg));
   
   if (found != modifyProcs.end())
      modifyProcs.erase(found);
}

// --------------------------------------------------------------------------
// Add a callback routine to be called before text is deleted from the buffer.
// --------------------------------------------------------------------------
void Ne_Text_Buffer::addPreDeleteCB(NeBufPreDeleteCallbackProc* bufPreDeleteCB, void* cbArg)
{
   preDeleteProcs.push_back(Ne_Text_PreDelete_Callback(bufPreDeleteCB, cbArg));
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::removePreDeleteCB(NeBufPreDeleteCallbackProc* bufPreDeleteCB, void* cbArg)
{
   std::vector<Ne_Text_PreDelete_Callback>::iterator found
      = std::find(preDeleteProcs.begin(), preDeleteProcs.end(),
                  Ne_Text_PreDelete_Callback(bufPreDeleteCB, cbArg));

   if (found != preDeleteProcs.end())
      preDeleteProcs.erase(found);
}

// --------------------------------------------------------------------------
// Find the position of the start of the line containing position "pos"
// --------------------------------------------------------------------------
int Ne_Text_Buffer::startOfLine(int pos) const
{
   int startPos;

   if (!searchBackward(pos, '\n', &startPos))
      return 0;
   return startPos + 1;
}

// --------------------------------------------------------------------------
// Find the position of the end of the line containing position "pos"
// (which is either a pointer to the newline character ending the line,
// or a pointer to one character beyond the end of the buffer)
// --------------------------------------------------------------------------
int Ne_Text_Buffer::endOfLine(int pos) const
{
   int endPos;

   if (!searchForward(pos, '\n', &endPos))
      endPos = buffer.size();
   return endPos;
}

// --------------------------------------------------------------------------
// Get a character from the text buffer expanded into it's screen
// representation (which may be several characters for a tab or a
// control code).  Returns the number of characters written to "outStr".
// "indent" is the number of characters from the start of the line
// for figuring tabs.  Output string is guranteed to be shorter or
// equal in length to MAX_EXP_CHAR_LEN
// --------------------------------------------------------------------------
int Ne_Text_Buffer::getExpandedChar(const int pos, const int indent, char* outStr) const
{
   return expandCharacter(getCharacter(pos), indent, outStr, tabDist, nullSubsChar);
}

// --------------------------------------------------------------------------
// Expand a single character from the text buffer into it's screen
// representation (which may be several characters for a tab or a
// control code).  Returns the number of characters added to "outStr".
// "indent" is the number of characters from the start of the line
// for figuring tabs.  Output string is guranteed to be shorter or
// equal in length to MAX_EXP_CHAR_LEN
// --------------------------------------------------------------------------
int Ne_Text_Buffer::expandCharacter(const char c, const int indent, char* outStr, const int tabDist, const char nullSubsChar) const
{
   int i, nSpaces;

   // Convert tabs to spaces
   if (c == '\t')
   {
      nSpaces = tabDist - (indent % tabDist);
      for (i = 0; i < nSpaces; i++)
         outStr[i] = ' ';
      return nSpaces;
   }

   // Convert ASCII control codes to readable character sequences
   if (c == nullSubsChar)
   {
      sprintf(outStr, "<nul>");
      return 5;
   }
   if (((unsigned char)c) <= 31)
   {
      sprintf(outStr, "<%s>", ControlCodeTable[(unsigned char)c]);
      return strlen(outStr);
   }
   else if (c == 127)
   {
      sprintf(outStr, "<del>");
      return 5;
   }

   // Otherwise, just return the character
   *outStr = c;
   return 1;
}

// --------------------------------------------------------------------------
// Return the length in displayed characters of character "c" expanded
// for display (as discussed above in BufGetExpandedChar).  If the
// buffer for which the character width is being measured is doing null
// substitution, nullSubsChar should be passed as that character (or nul
// to ignore).
// --------------------------------------------------------------------------
int Ne_Text_Buffer::charWidth(char c, int indent, int tabDist, char nullSubsChar) const
{
   // Note, this code must parallel that in expandCharacter
   if (c == nullSubsChar)
      return 5; // <nul>
   else if (c == '\t')
      return tabDist - (indent % tabDist);
   else if (((unsigned char)c) <= 31)
      return strlen(ControlCodeTable[(unsigned char)c]) + 2;
   else if (c == 127)
      return 5; // <del>
   return 1;
}

// --------------------------------------------------------------------------
// Count the number of displayed characters between buffer position
// "lineStartPos" and "targetPos". (displayed characters are the characters
// shown on the screen to represent characters in the buffer, where tabs and
// control characters are expanded)
// --------------------------------------------------------------------------
int Ne_Text_Buffer::countDispChars(const int lineStartPos, const int targetPos) const
{
   int charCount = 0;
   char expandedChar[MAX_EXP_CHAR_LEN];

   int pos = lineStartPos;
   while (pos < targetPos && pos < buffer.size())
      charCount += getExpandedChar(pos++, charCount, expandedChar);
   return charCount;
}

// --------------------------------------------------------------------------
// Count forward from buffer position "startPos" in displayed characters
// (displayed characters are the characters shown on the screen to represent
// characters in the buffer, where tabs and control characters are expanded)
// --------------------------------------------------------------------------
int Ne_Text_Buffer::countForwardDispChars(int lineStartPos, int nChars) const
{
   int pos, charCount = 0;
   char c;

   pos = lineStartPos;
   while (charCount < nChars && pos < buffer.size())
   {
      c = getCharacter(pos);
      if (c == '\n')
         return pos;
      charCount += charWidth(c, charCount, tabDist, nullSubsChar);
      pos++;
   }
   return pos;
}

// --------------------------------------------------------------------------
// Count the number of newlines between startPos and endPos in buffer "buf".
// The character at position "endPos" is not counted.
// --------------------------------------------------------------------------
int Ne_Text_Buffer::countLines(int startPos, int endPos) const
{
   return std::count(&buffer[startPos], &buffer[endPos], '\n');
}

// --------------------------------------------------------------------------
// Find the first character of the line "nLines" forward from "startPos"
// in "buf" and return its position
// --------------------------------------------------------------------------
int Ne_Text_Buffer::countForwardNLines(const int startPos, const unsigned nLines) const
{
   if (nLines == 0)
      return startPos;

   int lineCount = 0;
   int pos = startPos;
   while (pos < buffer.size())
   {
      if (buffer[pos++] == '\n')
      {
         lineCount++;
         if (lineCount == nLines)
            return pos;
      }
   }
   return pos;
}

// --------------------------------------------------------------------------
// Find the position of the first character of the line "nLines" backwards
// from "startPos" (not counting the character pointed to by "startpos" if
// that is a newline) in "buf".  nLines == 0 means find the beginning of the line
// --------------------------------------------------------------------------
int Ne_Text_Buffer::countBackwardNLines(int startPos, int nLines) const
{
   int pos = startPos - 1;
   if (pos <= 0)
      return 0;

   int lineCount = -1;
   while (pos >= 0)
   {
      if (buffer[pos] == '\n')
      {
         if (++lineCount >= nLines)
            return pos + 1;
      }
      pos--;
   }
   return 0;
}

// --------------------------------------------------------------------------
// Search forwards in buffer "buf" for characters in "searchChars", starting
// with the character "startPos", and returning the result in "foundPos"
// returns true if found, false if not.
// --------------------------------------------------------------------------
bool Ne_Text_Buffer::searchForward(int startPos, const char* searchChars, int* foundPos) const
{
   std::string::size_type found = buffer.find_first_of(searchChars, startPos);
   if (found ==  std::string::npos)
   {
      *foundPos = buffer.size();
      return false;
   }

   *foundPos = found;
   return true;
}

// --------------------------------------------------------------------------
// Search backwards in buffer "buf" for characters in "searchChars", starting
// with the character BEFORE "startPos", returning the result in "foundPos"
// true if found, false if not.
// --------------------------------------------------------------------------
bool Ne_Text_Buffer::searchBackward(int startPos, const char* searchChars, int* foundPos) const
{
   std::string::size_type found = buffer.find_last_of(searchChars, startPos);
   if (found ==  std::string::npos)
   {
      *foundPos = buffer.size();
      return false;
   }

   *foundPos = found;
   return true;
}

// --------------------------------------------------------------------------
// The primary routine for integrating new text into a text buffer with
// substitution of another character for ascii nuls.  This substitutes null
// characters in the string in preparation for being copied or replaced
// into the buffer, and if neccessary, adjusts the buffer as well, in the
// event that the string contains the character it is currently using for
// substitution.  Returns false, if substitution is no longer possible
// because all non-printable characters are already in use.
// --------------------------------------------------------------------------
bool Ne_Text_Buffer::substituteNullChars(char* str, int length)
{
   bool histogram[256];

   // Find out what characters the string contains
   histogramCharacters(str, length, histogram, true);

   // Does the string contain the null-substitute character?  If so, re-
   // histogram the buffer text to find a character which is ok in both the
   // string and the buffer, and change the buffer's null-substitution
   // character.  If none can be found, give up and return false
   if (histogram[(unsigned char)nullSubsChar] != 0)
   {
      // here we know we can modify the file buffer directly, so we cast away constness
      const std::string& bufString = getAll();
      histogramCharacters(bufString.c_str(), bufString.size(), histogram, false);
      char newSubsChar = chooseNullSubsChar(histogram);
      if (newSubsChar == '\0')
         return false;

      // bufString points to the buffer's data, so we substitute in situ
      subsChars((char*)bufString.c_str(), bufString.size(), nullSubsChar, newSubsChar);
      nullSubsChar = newSubsChar;
   }

   // If the string contains null characters, substitute them with the
   // buffer's null substitution character */
   if (histogram[0])
      subsChars(str, length, '\0', nullSubsChar);
   return true;
}

// --------------------------------------------------------------------------
// Convert strings obtained from buffers which contain null characters, which
// have been substituted for by a special substitution character, back to
// a null-containing string.  There is no time penalty for calling this
// routine if no substitution has been done.
// --------------------------------------------------------------------------
void Ne_Text_Buffer::unsubstituteNullChars(char* string)
{
   char* c, subsChar = nullSubsChar;

   if (subsChar == '\0')
      return;
   for (c = string; *c != '\0'; c++)
      if (*c == subsChar)
         *c = '\0';
}

// --------------------------------------------------------------------------
// Compares len Bytes contained in buf starting at Position pos with
// the contens of cmpText. Returns 0 if there are no differences, != 0 otherwise.
// --------------------------------------------------------------------------
int Ne_Text_Buffer::cmp(int pos, int len, const char* cmpText) const
{
   int posEnd = pos + len;

   if (posEnd > buffer.size())
      return 1;

   if (pos < 0)
      return -1;

   return strncmp(&buffer[pos], cmpText, len);
}

// --------------------------------------------------------------------------
// Create a pseudo-histogram of the characters in a string (don't actually
// count, because we don't want overflow, just mark the character's presence
// with a 1).  If init is true, initialize the histogram before acumulating.
// if not, add the new data to an existing histogram.
// --------------------------------------------------------------------------
static void histogramCharacters(const char* str, int length, bool hist[256], bool init)
{
   if (init)
      for (int i = 0; i < 256; i++)
         hist[i] = false;
   for (const char* c = str; c < &str[length]; ++c)
      hist[*((unsigned char*)c)] = true;
}

// --------------------------------------------------------------------------
// Substitute fromChar with toChar in string.
// --------------------------------------------------------------------------
static void subsChars(char* str, int length, char fromChar, char toChar)
{
   for (char* c = str; c < &str[length]; c++)
      if (*c == fromChar) *c = toChar;
}

// --------------------------------------------------------------------------
// Search through ascii control characters in histogram in order of least
// likelihood of use, find an unused character to use as a stand-in for a
// null.  If the character set is full (no available characters outside of
// the printable set, return the null character.
// --------------------------------------------------------------------------
static char chooseNullSubsChar(bool hist[256])
{
#define N_REPLACEMENTS 25
   static char replacements[N_REPLACEMENTS] = {1, 2, 3, 4, 5, 6, 14, 15, 16, 17, 18, 19,
      20, 21, 22, 23, 24, 25, 26, 28, 29, 30, 31, 11, 7
   };

   for (int i = 0; i < N_REPLACEMENTS; i++)
      if (!hist[(unsigned char)replacements[i]])
         return replacements[i];
   return '\0';
}

// --------------------------------------------------------------------------
// Internal (non-redisplaying) version of insertAt.  Returns the length of
// text inserted (this is just strlen(text), however this calculation can be
// expensive and the length will be required by any caller who will continue
// on to call redisplay).  pos must be contiguous with the existing text in
// the buffer (i.e. not past the end).
// --------------------------------------------------------------------------
int Ne_Text_Buffer::insert(int pos, const char* text)
{
   int length = strlen(text);

   // Insert the new text
   buffer.insert(pos, text);

   updateSelections(pos, 0, length);

   return length;
}

// --------------------------------------------------------------------------
// Internal (non-redisplaying) version of remove.  Removes the contents
// of the buffer between start and end (and moves the gap to the site of
// the delete).
// --------------------------------------------------------------------------
void Ne_Text_Buffer::deleteBuf(int start, int end)
{
   buffer.erase(start, end - start);

   // fix up any selections which might be affected by the change 
   updateSelections(start, end - start, 0);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::setSelection(Ne_Text_Selection& sel, int start, int end)
{
   sel.selected = (start != end);
   sel.start = std::min(start, end);
   sel.end = std::max(start, end);
}

// --------------------------------------------------------------------------
bool Ne_Text_Selection::getSelectionPos(int* start, int* end) const
{
   *start = this->start;
   *end = this->end;
   return selected;
}

// --------------------------------------------------------------------------
std::string Ne_Text_Buffer::getSelectionText(const Ne_Text_Selection& sel) const
{
   int start, end;

   // If there's no selection, return an allocated empty string
   if (!sel.getSelectionPos(&start, &end))
      return "";

   return getRange(start, end);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::removeSelected(Ne_Text_Selection& sel)
{
   int start, end;
   
   if (!sel.getSelectionPos(&start, &end))
      return;

   remove(start, end);
}

// --------------------------------------------------------------------------
void Ne_Text_Buffer::replaceSelected(Ne_Text_Selection& sel, const char* text)
{
   int start, end;
   Ne_Text_Selection oldSelection = sel;

   /* If there's no selection, return */
   if (!sel.getSelectionPos(&start, &end))
      return;

   // Do the appropriate type of replace
   replace(start, end, text);

   // Unselect (happens automatically in BufReplace
   sel.selected = false;
   redisplaySelection(oldSelection, sel);
}


// --------------------------------------------------------------------------
// Call the stored modify callback procedure(s) for this buffer to update the
// changed area(s) on the screen and any other listeners.
// --------------------------------------------------------------------------
void Ne_Text_Buffer::callModifyCBs(int pos, int nDeleted, int nInserted, int nRestyled, const char* deletedText)
{
   for (size_t i = 0; i < modifyProcs.size(); i++)
      (*modifyProcs[i].modifyProc)(pos, nInserted, nDeleted, nRestyled, deletedText, modifyProcs[i].arg);
}

// --------------------------------------------------------------------------
// Call the stored pre-delete callback procedure(s) for this buffer to update
// the changed area(s) on the screen and any other listeners.
// --------------------------------------------------------------------------
void Ne_Text_Buffer::callPreDeleteCBs(int pos, int nDeleted)
{
   for (size_t i = 0; i < preDeleteProcs.size(); ++i)
      (*preDeleteProcs[i].preDeleteProc)(pos, nDeleted, preDeleteProcs[i].arg);
}

// --------------------------------------------------------------------------
// Call the stored redisplay procedure(s) for this buffer to update the
// screen for a change in a selection.
// --------------------------------------------------------------------------
void Ne_Text_Buffer::redisplaySelection(const Ne_Text_Selection& oldSelection, const Ne_Text_Selection& newSelection)
{
   // If either selection is rectangular, add an additional character to
   // the end of the selection to request the redraw routines to wipe out
   // the parts of the selection beyond the end of the line
   int oldStart = oldSelection.start;
   int newStart = newSelection.start;
   int oldEnd = oldSelection.end;
   int newEnd = newSelection.end;

   // If the old or new selection is unselected, just redisplay the
   // single area that is (was) selected and return
   if (!oldSelection.selected && !newSelection.selected)
      return;
   if (!oldSelection.selected)
   {
      callModifyCBs(newStart, 0, 0, newEnd - newStart, NULL);
      return;
   }
   if (!newSelection.selected)
   {
      callModifyCBs(oldStart, 0, 0, oldEnd - oldStart, NULL);
      return;
   }

   // If the selections are non-contiguous, do two separate updates and return
   if (oldEnd < newStart || newEnd < oldStart)
   {
      callModifyCBs(oldStart, 0, 0, oldEnd - oldStart, NULL);
      callModifyCBs(newStart, 0, 0, newEnd - newStart, NULL);
      return;
   }

   /* Otherwise, separate into 3 separate regions: ch1, and ch2 (the two
   changed areas), and the unchanged area of their intersection,
   and update only the changed area(s) */
   int ch1Start = std::min(oldStart, newStart);
   int ch2End = std::max(oldEnd, newEnd);
   int ch1End = std::max(oldStart, newStart);
   int ch2Start = std::min(oldEnd, newEnd);
   if (ch1Start != ch1End)
      callModifyCBs(ch1Start, 0, 0, ch1End - ch1Start, NULL);
   if (ch2Start != ch2End)
      callModifyCBs(ch2Start, 0, 0, ch2End - ch2Start, NULL);
}

// --------------------------------------------------------------------------
// Update all of the selections in "buf" for changes in the buffer's text
// --------------------------------------------------------------------------
void Ne_Text_Buffer::updateSelections(int pos, int nDeleted, int nInserted)
{
   updateSelection(primary, pos, nDeleted, nInserted);
   updateSelection(secondary, pos, nDeleted, nInserted);
   updateSelection(highlight, pos, nDeleted, nInserted);
}

// --------------------------------------------------------------------------
// Update an individual selection for changes in the corresponding text
// --------------------------------------------------------------------------
void Ne_Text_Buffer::updateSelection(Ne_Text_Selection& sel, int pos, int nDeleted, int nInserted)
{
   if (!sel.selected || pos > sel.end)
      return;
   if (pos + nDeleted <= sel.start)
   {
      sel.start += nInserted - nDeleted;
      sel.end += nInserted - nDeleted;
   }
   else if (pos <= sel.start && pos + nDeleted >= sel.end)
   {
      sel.start = pos;
      sel.end = pos;
      sel.selected = false;
   }
   else if (pos <= sel.start && pos + nDeleted < sel.end)
   {
      sel.start = pos;
      sel.end = nInserted + sel.end - nDeleted;
   }
   else if (pos < sel.end)
   {
      sel.end += nInserted - nDeleted;
      if (sel.end <= sel.start)
         sel.selected = false;
   }
}

// --------------------------------------------------------------------------
// Search forwards in buffer "buf" for character "searchChar", starting
// with the character "startPos", and returning the result in "foundPos"
// returns true if found, false if not.  (The difference between this and
// BufSearchForward is that it's optimized for single characters.  The
// overall performance of the text widget is dependent on its ability to
// count lines quickly, hence searching for a single character: newline)
// --------------------------------------------------------------------------
bool Ne_Text_Buffer::searchForward(int startPos, char searchChar, int* foundPos) const
{
   for(int pos = startPos; pos < buffer.size(); ++pos)
   {
      if (buffer[pos] == searchChar)
      {
         *foundPos = pos;
         return true;
      }
   }

   *foundPos = buffer.size();
   return false;
}

// --------------------------------------------------------------------------
// Search backwards in buffer "buf" for character "searchChar", starting
// with the character BEFORE "startPos", returning the result in "foundPos"
// returns true if found, false if not.  (The difference between this and
// BufSearchBackward is that it's optimized for single characters.  The
// overall performance of the text widget is dependent on its ability to
// count lines quickly, hence searching for a single character: newline)
// --------------------------------------------------------------------------
bool Ne_Text_Buffer::searchBackward(int startPos, char searchChar, int* foundPos) const
{
   if (startPos <= 0)
   {
      *foundPos = 0;
      return false;
   }

   for(int pos = (startPos == 0 ? 0 : startPos - 1); pos >= 0; --pos)
   {
      if (buffer[pos] == searchChar)
      {
         *foundPos = pos;
         return true;
      }
   }
   *foundPos = 0;
   return false;
}
