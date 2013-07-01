#ifndef NEDIT_TEXTBUF_H_INCLUDED
#define NEDIT_TEXTBUF_H_INCLUDED

#include <list>

/* Maximum length in characters of a tab or control character expansion  of a single buffer character */
#define MAX_EXP_CHAR_LEN 20

struct RangesetTable;

struct selection
{
   selection();

   bool selected;          /* True if the selection is active */
   bool rectangular;       /* True if the selection is rectangular */
   bool zeroWidth;         /* Width 0 selections aren't "real" selections, but they can be useful when creating rectangular selections from the keyboard. */
   int start;              /* Pos. of start of selection, or if rectangular start of line containing it. */
   int end;                /* Pos. of end of selection, or if rectangular of line containing it. */
   int rectStart;          /* Indent of left edge of rect. selection */
   int rectEnd;            /* Indent of right edge of rect. selection */
};

typedef void (*bufModifyCallbackProc)(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg);
typedef void (*bufPreDeleteCallbackProc)(int pos, int nDeleted, void* cbArg);

struct Ne_Text_Buffer
{
   int length; 	        /* length of the text in the buffer (the length of the buffer itself must be calculated: gapEnd - gapStart + length) */
   char* buf;                  /* allocated memory where the text is stored */
   int gapStart;  	        /* points to the first character of the gap */
   int gapEnd;                 /* points to the first char after the gap */
   selection primary;		/* highlighted areas */
   selection secondary;
   selection highlight;
   int tabDist;		/* equiv. number of characters in a tab */
   int useTabs;		/* True if buffer routines are allowed to use tabs for padding in rectangular operations */
   
   /* procedures and args to call when buffer is modified to redisplay contents */
   typedef std::list<std::pair<bufModifyCallbackProc, void*> > ModifyProcs;
   ModifyProcs modifyProcs;

   int nPreDeleteProcs;	/* number of pre-delete procs attached */
   bufPreDeleteCallbackProc	/* procedure to call before text is deleted */
   *preDeleteProcs;	/* from the buffer; at most one is supported. */
   void** preDeleteCbArgs;	/* caller argument for pre-delete proc above */
   
   int cursorPosHint;		/* hint for reasonable cursor position after a buffer modification operation */
   char nullSubsChar;	    	/* NEdit is based on C null-terminated strings, so ascii-nul characters must be substituted
				   with something else.  This is the else, but of course, things get quite messy when you use it */
   RangesetTable* rangesetTable;   /* current range sets */
};

Ne_Text_Buffer* BufCreate();
Ne_Text_Buffer* BufCreatePreallocated(int requestedSize);
void BufFree(Ne_Text_Buffer* buf);
char* BufGetAll(Ne_Text_Buffer* buf);
const char* BufAsString(Ne_Text_Buffer* buf);
void BufSetAll(Ne_Text_Buffer* buf, const char* text);
char* BufGetRange(const Ne_Text_Buffer* buf, int start, int end);
char BufGetCharacter(const Ne_Text_Buffer* buf, const int pos);
char* BufGetTextInRect(Ne_Text_Buffer* buf, int start, int end, int rectStart, int rectEnd);
void BufInsert(Ne_Text_Buffer* buf, int pos, const char* text);
void BufRemove(Ne_Text_Buffer* buf, int start, int end);
void BufReplace(Ne_Text_Buffer* buf, int start, int end, const char* text);
void BufCopyFromBuf(Ne_Text_Buffer* fromBuf, Ne_Text_Buffer* toBuf, int fromStart, int fromEnd, int toPos);
void BufInsertCol(Ne_Text_Buffer* buf, int column, int startPos, const char* text, int* charsInserted, int* charsDeleted);
void BufReplaceRect(Ne_Text_Buffer* buf, int start, int end, int rectStart, int rectEnd, const char* text);
void BufRemoveRect(Ne_Text_Buffer* buf, int start, int end, int rectStart, int rectEnd);
void BufOverlayRect(Ne_Text_Buffer* buf, int startPos, int rectStart, int rectEnd, const char* text, int* charsInserted, int* charsDeleted);
void BufClearRect(Ne_Text_Buffer* buf, int start, int end, int rectStart, int rectEnd);
int BufGetTabDistance(Ne_Text_Buffer* buf);
void BufSetTabDistance(Ne_Text_Buffer* buf, int tabDist);
void BufCheckDisplay(Ne_Text_Buffer* buf, int start, int end);
void BufSelect(Ne_Text_Buffer* buf, int start, int end);
void BufUnselect(Ne_Text_Buffer* buf);
void BufRectSelect(Ne_Text_Buffer* buf, int start, int end, int rectStart, int rectEnd);
int BufGetSelectionPos(Ne_Text_Buffer* buf, int* start, int* end, int* isRect, int* rectStart, int* rectEnd);
int BufGetEmptySelectionPos(Ne_Text_Buffer* buf, int* start, int* end, int* isRect, int* rectStart, int* rectEnd);
char* BufGetSelectionText(Ne_Text_Buffer* buf);
void BufRemoveSelected(Ne_Text_Buffer* buf);
void BufReplaceSelected(Ne_Text_Buffer* buf, const char* text);
void BufSecondarySelect(Ne_Text_Buffer* buf, int start, int end);
void BufSecondaryUnselect(Ne_Text_Buffer* buf);
void BufSecRectSelect(Ne_Text_Buffer* buf, int start, int end, int rectStart, int rectEnd);
int BufGetSecSelectPos(Ne_Text_Buffer* buf, int* start, int* end, int* isRect, int* rectStart, int* rectEnd);
char* BufGetSecSelectText(Ne_Text_Buffer* buf);
void BufRemoveSecSelect(Ne_Text_Buffer* buf);
void BufReplaceSecSelect(Ne_Text_Buffer* buf, const char* text);
void BufHighlight(Ne_Text_Buffer* buf, int start, int end);
void BufUnhighlight(Ne_Text_Buffer* buf);
void BufRectHighlight(Ne_Text_Buffer* buf, int start, int end, int rectStart, int rectEnd);
int BufGetHighlightPos(Ne_Text_Buffer* buf, int* start, int* end, int* isRect, int* rectStart, int* rectEnd);
void BufAddModifyCB(Ne_Text_Buffer* buf, bufModifyCallbackProc bufModifiedCB, void* cbArg);
void BufAddHighPriorityModifyCB(Ne_Text_Buffer* buf, bufModifyCallbackProc bufModifiedCB, void* cbArg);
void BufRemoveModifyCB(Ne_Text_Buffer* buf, bufModifyCallbackProc bufModifiedCB, void* cbArg);
void BufAddPreDeleteCB(Ne_Text_Buffer* buf, bufPreDeleteCallbackProc bufPreDeleteCB, void* cbArg);
void BufRemovePreDeleteCB(Ne_Text_Buffer* buf, bufPreDeleteCallbackProc bufPreDeleteCB,	void* cbArg);
int BufStartOfLine(Ne_Text_Buffer* buf, int pos);
int BufEndOfLine(Ne_Text_Buffer* buf, int pos);
int BufGetExpandedChar(const Ne_Text_Buffer* buf, const int pos, const int indent, char* outStr);
int BufExpandCharacter(char c, int indent, char* outStr, int tabDist, char nullSubsChar);
int BufCharWidth(char c, int indent, int tabDist, char nullSubsChar);
int BufCountDispChars(const Ne_Text_Buffer* buf, const int lineStartPos, const int targetPos);
int BufCountForwardDispChars(Ne_Text_Buffer* buf, int lineStartPos, int nChars);
int BufCountLines(Ne_Text_Buffer* buf, int startPos, int endPos);
int BufCountForwardNLines(const Ne_Text_Buffer* buf, const int startPos, const unsigned nLines);
int BufCountBackwardNLines(Ne_Text_Buffer* buf, int startPos, int nLines);
int BufSearchForward(Ne_Text_Buffer* buf, int startPos, const char* searchChars, int* foundPos);
int BufSearchBackward(Ne_Text_Buffer* buf, int startPos, const char* searchChars, int* foundPos);
int BufSubstituteNullChars(char* string, int length, Ne_Text_Buffer* buf);
void BufUnsubstituteNullChars(char* string, Ne_Text_Buffer* buf);
int BufCmp(Ne_Text_Buffer* buf, int pos, int len, const char* cmpText);

#endif /* NEDIT_TEXTBUF_H_INCLUDED */
