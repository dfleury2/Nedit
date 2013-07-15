#ifndef rangeset_h_DEFINED
#define rangeset_h_DEFINED

#include <FL/Fl.H>

#define N_RANGESETS 63

// --------------------------------------------------------------------------
struct Range
{
   int start, end;               /* range from [start-]end */
};

// --------------------------------------------------------------------------
struct Rangeset;
struct Ne_Text_Buffer;

typedef Rangeset* RangesetUpdateFn(Rangeset* p, int pos, int ins, int del);

// --------------------------------------------------------------------------
struct Rangeset
{
   RangesetUpdateFn* update_fn;  /* modification update function */
   char* update_name;            /* update function name */
   int maxpos;                   /* text buffer maxpos */
   int last_index;               /* a place to start looking */
   int n_ranges;                 /* how many ranges in ranges */
   Range* ranges;                /* the ranges table */
   unsigned char label;          /* a number 1-63 */

   signed char color_set;        /* 0: unset; 1: set; -1: invalid */
   char* color_name;             /* the name of an assigned color */
   Fl_Color color;               /* the value of a particular color */
   Ne_Text_Buffer* buf;          /* the text buffer of the rangeset */
   char* name;                   /* name of rangeset */
};

// --------------------------------------------------------------------------
struct RangesetTable
{
   int n_set;                          /* how many sets are active */
   Ne_Text_Buffer* buf;                /* the text buffer of the rangeset */
   Rangeset set[N_RANGESETS];          /* the rangeset table */
   unsigned char order[N_RANGESETS];   /* inds of set[]s ordered by depth */
   unsigned char active[N_RANGESETS];  /* entry true if corresp. set active */
   unsigned char depth[N_RANGESETS];   /* depth[i]: pos of set[i] in order[] */
   unsigned char list[N_RANGESETS + 1];/* string of labels in depth order */
};

// --------------------------------------------------------------------------
RangesetTable* RangesetTableAlloc(Ne_Text_Buffer* buf);
RangesetTable* RangesetTableFree(RangesetTable* table);
RangesetTable* RangesetTableClone(RangesetTable* srcTable, Ne_Text_Buffer* destBuffer);
int RangesetFindIndex(RangesetTable* table, int label, int must_be_active);
int RangesetCreate(RangesetTable* table);
int nRangesetsAvailable(RangesetTable* table);
Rangeset* RangesetForget(RangesetTable* table, int label);
Rangeset* RangesetFetch(RangesetTable* table, int label);
unsigned char* RangesetGetList(RangesetTable* table);
void RangesetTableUpdatePos(RangesetTable* table, int pos, int n_ins, int n_del);
void RangesetBufModifiedCB(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg);
int RangesetIndex1ofPos(RangesetTable* table, int pos, int needs_color);
char* RangesetTableGetColorName(RangesetTable* table, int index);
int RangesetTableGetColorValid(RangesetTable* table, int index, Fl_Color* color);
int RangesetTableAssignColorPixel(RangesetTable* table, int index, Fl_Color color, int ok);

// --------------------------------------------------------------------------
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
int RangesetLabelOK(int label);
int RangesetAssignColorName(Rangeset* rangeset, char* color_name);
int RangesetAssignColorPixel(Rangeset* rangeset, Fl_Color color, int ok);
char* RangesetGetName(Rangeset* rangeset);
int RangesetAssignName(Rangeset* rangeset, char* name);
int RangesetGetColorValid(Rangeset* rangeset, Fl_Color* color);

#endif /* rangeset_h_DEFINED */
