#ifndef Ne_Text_Buffer_h__
#define Ne_Text_Buffer_h__

#include <vector>
#include <string>

// Maximum length in characters of a tab or control character expansion of a single buffer character
#define MAX_EXP_CHAR_LEN 20

struct NeRangesetTable;

// --------------------------------------------------------------------------
struct Ne_Text_Selection
{
   Ne_Text_Selection();

   bool getSelectionPos(int* start, int* end) const;

   bool selected;       /* true if the selection is active */
   int start;           /* Pos. of start of selection. */
   int end;             /* Pos. of end of selection. */
} ;

typedef void (NeBufModifyCallbackProc)(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg);
typedef void (NeBufPreDeleteCallbackProc)(int pos, int nDeleted, void* cbArg);

// --------------------------------------------------------------------------
struct Ne_Text_Modify_Callback
{
   Ne_Text_Modify_Callback( NeBufModifyCallbackProc* proc, void* argument)
      : modifyProc(proc), arg(argument)
   {}

   NeBufModifyCallbackProc* modifyProc;
   void* arg;
};

inline bool operator == (const Ne_Text_Modify_Callback& lhs, const Ne_Text_Modify_Callback& rhs)
{
   return ((lhs.modifyProc == rhs.modifyProc) && (lhs.arg == rhs.arg));
}

// --------------------------------------------------------------------------
struct Ne_Text_PreDelete_Callback
{
   Ne_Text_PreDelete_Callback( NeBufPreDeleteCallbackProc* proc, void* argument)
      : preDeleteProc(proc), arg(argument)
   {}

   NeBufPreDeleteCallbackProc* preDeleteProc;
   void* arg;
};

inline bool operator == (const Ne_Text_PreDelete_Callback& lhs, const Ne_Text_PreDelete_Callback& rhs)
{
   return ((lhs.preDeleteProc == rhs.preDeleteProc) && (lhs.arg == rhs.arg));
}

// --------------------------------------------------------------------------
class Ne_Text_Buffer
{
public:
   Ne_Text_Buffer(int requestedSize = 0);
   const std::string& getAll() const;
   void setAll(const char* text);

   int size() const { return buffer.size(); }
   int length() const { return size(); }
   int getCursorPosHint() const { return cursorPosHint; }
   Ne_Text_Selection& getSelection() { return primary; }
   Ne_Text_Selection& getSecSelection() { return secondary; }
   Ne_Text_Selection& getHighlightSelection() { return highlight; }

   char getNullSubsChar() const { return nullSubsChar; }

   std::string getRange(int start, int end) const;
   char getCharacter(int pos) const;
   void insertAt(int pos, const char* text);
   void remove(int start, int end);
   void replace(int start, int end, const char* text);
   void copyFromBuf(const Ne_Text_Buffer& fromBuf, int fromStart, int fromEnd, int toPos);
   
   int getTabDistance() const;
   void setTabDistance(int tabDist);
   void checkDisplay(int start, int end);
   void select(int start, int end);
   void unselect();
   bool getSelectionPos(int* start, int* end);
   std::string getSelectionText() const;
   void removeSelected();
   void replaceSelected(const char* text);
   void secondarySelect(int start, int end);
   void secondaryUnselect();
   bool getSecSelectPos(int* start, int* end);
   std::string getSecSelectText() const;
   void removeSecSelect();
   void replaceSecSelect(const char* text);
   void setHighlight(int start, int end);
   void unsetHighlight();
   bool getHighlightPos(int* start, int* end) const;
   void addModifyCB(NeBufModifyCallbackProc* bufModifiedCB, void* cbArg);
   void addHighPriorityModifyCB(NeBufModifyCallbackProc* bufModifiedCB, void* cbArg);
   void removeModifyCB(NeBufModifyCallbackProc* bufModifiedCB, void* cbArg);
   void addPreDeleteCB(NeBufPreDeleteCallbackProc* bufPreDeleteCB, void* cbArg);
   void removePreDeleteCB(NeBufPreDeleteCallbackProc* bufPreDeleteCB, void* cbArg);
   int startOfLine(int pos) const;
   int endOfLine(int pos) const;
   int getExpandedChar(const int pos, const int indent, char* outStr) const;
   int expandCharacter(char c, int indent, char* outStr, int tabDist, char nullSubsChar) const;
   int countLines(int startPos, int endPos) const;
   int countForwardNLines(const int startPos, const unsigned nLines) const;
   int countBackwardNLines(int startPos, int nLines) const;
   bool searchForward(int startPos, const char* searchChars, int* foundPos) const;
   bool searchBackward(int startPos, const char* searchChars, int* foundPos) const;
   int cmp(int pos, int len, const char* cmpText) const;

   int charWidth(char c, int indent, int tabDist, char nullSubsChar) const;
   int countDispChars(const int lineStartPos, const int targetPos) const;
   int countForwardDispChars(int lineStartPos, int nChars) const;

   bool substituteNullChars(char* string, int length);
   void unsubstituteNullChars(char* string);

   NeRangesetTable* rangesetTable;

protected:
   int insert(int pos, const char* text);
   void deleteBuf(int start, int end);

   void callModifyCBs(int pos, int nDeleted, int nInserted, int nRestyled, const char* deletedText);
   void callPreDeleteCBs(int pos, int nDeleted);
   
   void setSelection(Ne_Text_Selection& sel, int start, int end);
   std::string getSelectionText(const Ne_Text_Selection& sel) const;

   void updateSelections(int pos, int nDeleted, int nInserted);
   void updateSelection(Ne_Text_Selection& sel, int pos, int nDeleted, int nInserted);

   void replaceSelected(Ne_Text_Selection& sel, const char* text);
   void removeSelected(Ne_Text_Selection& sel);
   
   void redisplaySelection(const Ne_Text_Selection& oldSelection, const Ne_Text_Selection& newSelection);


   bool searchForward(int startPos, char searchChar, int* foundPos) const;
   bool searchBackward(int startPos, char searchChar, int* foundPos) const;

private:
   std::string buffer;           /* memory where the text is stored */

   Ne_Text_Selection primary;		/* highlighted areas */
   Ne_Text_Selection secondary;
   Ne_Text_Selection highlight;
   int tabDist;		            /* equiv. number of characters in a tab */

   std::vector<Ne_Text_Modify_Callback> modifyProcs;        /* procedures with args to call when buffer is modified to redisplay contents */  
   std::vector<Ne_Text_PreDelete_Callback> preDeleteProcs;  /* procedures with args to call before text is deleted from the buffer */

   int cursorPosHint;	/* hint for reasonable cursor position after a buffer modification operation */
   char nullSubsChar;   /* NEdit is based on C null-terminated strings,
                        so ascii-nul characters must be substituted
                        with something else.  This is the else, but of course, things get quite messy when you use it */
};

#endif // Ne_Text_Buffer_h__
