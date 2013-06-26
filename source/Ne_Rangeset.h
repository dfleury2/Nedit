#ifndef rangeset_h_DEFINED
#define rangeset_h_DEFINED

#include <FL/Fl.H>

#define N_RANGESETS 63

typedef struct _Range Range;
typedef struct _Rangeset Rangeset;
typedef struct _RangesetTable RangesetTable;
typedef struct Ne_Text_Buffer Ne_Text_Buffer;

void RangesetRefreshRange(Rangeset* rangeset, int start, int end);
void RangesetEmpty(Rangeset* rangeset);
void RangesetInit(Rangeset* rangeset, int label, Ne_Text_Buffer* buf);
int RangesetChangeModifyResponse(Rangeset* rangeset, char* name);
int RangesetFindRangeNo(Rangeset* rangeset, int index, int* start, int* end);
int RangesetFindRangeOfPos(Rangeset* rangeset, int pos, int incl_end);
int RangesetCheckRangeOfPos(Rangeset* rangeset, int pos);
int RangesetInverse(Rangeset* p);
int RangesetAdd(Rangeset* origSet, Rangeset* plusSet);
int RangesetAddBetween(Rangeset* rangeset, int start, int end);
int RangesetRemove(Rangeset* origSet, Rangeset* minusSet);
int RangesetRemoveBetween(Rangeset* rangeset, int start, int end);
int RangesetGetNRanges(Rangeset* rangeset);
void RangesetGetInfo(Rangeset* rangeset, int* defined, int* label, int* count, char** color, char** name, char** mode);
RangesetTable* RangesetTableAlloc(Ne_Text_Buffer* buf);
RangesetTable* RangesetTableFree(RangesetTable* table);
RangesetTable* RangesetTableClone(RangesetTable* srcTable, Ne_Text_Buffer* destBuffer);
int RangesetFindIndex(RangesetTable* table, int label, int must_be_active);
int RangesetLabelOK(int label);
int RangesetCreate(RangesetTable* table);
int nRangesetsAvailable(RangesetTable* table);
Rangeset* RangesetForget(RangesetTable* table, int label);
Rangeset* RangesetFetch(RangesetTable* table, int label);
unsigned char* RangesetGetList(RangesetTable* table);
void RangesetTableUpdatePos(RangesetTable* table, int pos, int n_ins, int n_del);
void RangesetBufModifiedCB(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg);
int RangesetIndex1ofPos(RangesetTable* table, int pos, int needs_color);
int RangesetAssignColorName(Rangeset* rangeset, char* color_name);
int RangesetAssignColorPixel(Rangeset* rangeset, Fl_Color color, int ok);
char* RangesetGetName(Rangeset* rangeset);
int RangesetAssignName(Rangeset* rangeset, char* name);
int RangesetGetColorValid(Rangeset* rangeset, Fl_Color* color);
char* RangesetTableGetColorName(RangesetTable* table, int index);
int RangesetTableGetColorValid(RangesetTable* table, int index, Fl_Color* color);
int RangesetTableAssignColorPixel(RangesetTable* table, int index, Fl_Color color, int ok);

#endif /* rangeset_h_DEFINED */
