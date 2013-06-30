#ifndef Ne_Range_Set_h__
#define Ne_Range_Set_h__

#include <FL/Fl.H>

#define N_RANGESETS 63

/**
   NeRange from [start-]end
*/
struct NeRange
{
   NeRange() : start(0), end(0) {}
   int start, end;
};

struct NeRangeset;
class Ne_Text_Buffer;

typedef NeRangeset* RangesetUpdateFn(NeRangeset* p, int pos, int inserted, int deleted);

struct NeRangeset
{
   RangesetUpdateFn* update_fn;	/* modification update function */
   char* update_name;			/* update function name */
   int maxpos;				/* text buffer maxpos */
   int last_index;			/* a place to start looking */
   int n_ranges;			/* how many ranges in ranges */
   NeRange* ranges;			/* the ranges table */
   unsigned char label;		/* a number 1-63 */

   signed char color_set;              /* 0: unset; 1: set; -1: invalid */
   char* color_name;			/* the name of an assigned color */
   Fl_Color color;			/* the value of a particular color */
   Ne_Text_Buffer* buf;			/* the text buffer of the rangeset */
   char* name;                         /* name of rangeset */
};

struct NeRangesetTable
{
   int n_set;				/* how many sets are active */
   Ne_Text_Buffer* buf;			/* the text buffer of the rangeset */
   NeRangeset set[N_RANGESETS];		/* the rangeset table */
   unsigned char order[N_RANGESETS];	/* inds of set[]s ordered by depth */
   unsigned char active[N_RANGESETS];	/* entry true if corresp. set active */
   unsigned char depth[N_RANGESETS];	/* depth[i]: pos of set[i] in order[] */
   unsigned char list[N_RANGESETS + 1];/* string of labels in depth order */
};



void RangesetRefreshRange(NeRangeset* rangeset, int start, int end);
void RangesetEmpty(NeRangeset* rangeset);
void RangesetInit(NeRangeset* rangeset, int label, Ne_Text_Buffer* buf);
int RangesetChangeModifyResponse(NeRangeset* rangeset, char* name);
int RangesetFindRangeNo(NeRangeset* rangeset, int index, int* start, int* end);
int RangesetFindRangeOfPos(NeRangeset* rangeset, int pos, int incl_end);
int RangesetCheckRangeOfPos(NeRangeset* rangeset, int pos);
int RangesetInverse(NeRangeset* p);
int RangesetAdd(NeRangeset* origSet, NeRangeset* plusSet);
int RangesetAddBetween(NeRangeset* rangeset, int start, int end);
int RangesetRemove(NeRangeset* origSet, NeRangeset* minusSet);
int RangesetRemoveBetween(NeRangeset* rangeset, int start, int end);
int RangesetGetNRanges(NeRangeset* rangeset);
void RangesetGetInfo(NeRangeset* rangeset, int* defined, int* label, int* count, char** color, char** name, char** mode);
NeRangesetTable* RangesetTableAlloc(Ne_Text_Buffer* buf);
NeRangesetTable* RangesetTableFree(NeRangesetTable* table);
NeRangesetTable* RangesetTableClone(NeRangesetTable* srcTable, Ne_Text_Buffer* destBuffer);
int RangesetFindIndex(NeRangesetTable* table, int label, int must_be_active);
int RangesetLabelOK(int label);
int RangesetCreate(NeRangesetTable* table);
int nRangesetsAvailable(NeRangesetTable* table);
NeRangeset* RangesetForget(NeRangesetTable* table, int label);
NeRangeset* RangesetFetch(NeRangesetTable* table, int label);
unsigned char* RangesetGetList(NeRangesetTable* table);
void RangesetTableUpdatePos(NeRangesetTable* table, int pos, int n_ins, int n_del);
void RangesetBufModifiedCB(int pos, int nInserted, int nDeleted, int nRestyled, const char* deletedText, void* cbArg);
int RangesetIndex1ofPos(NeRangesetTable* table, int pos, int needs_color);
int RangesetAssignColorName(NeRangeset* rangeset, char* color_name);
int RangesetAssignColorPixel(NeRangeset* rangeset, Fl_Color color, int ok);
char* RangesetGetName(NeRangeset* rangeset);
int RangesetAssignName(NeRangeset* rangeset, char* name);
int RangesetGetColorValid(NeRangeset* rangeset, Fl_Color* color);
char* RangesetTableGetColorName(NeRangesetTable* table, int index);
int RangesetTableGetColorValid(NeRangesetTable* table, int index, Fl_Color* color);
int RangesetTableAssignColorPixel(NeRangesetTable* table, int index, Fl_Color color, int ok);

#endif // Ne_Range_Set_h__
