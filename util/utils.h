#ifndef NEDIT_UTILS_H_INCLUDED
#define NEDIT_UTILS_H_INCLUDED

#include <string>
#include <vector>

#include <sys/types.h>

#define malloc__(size) Malloc(size, __LINE__, __FILE__)
#define free__(arg) Free(arg, __LINE__, __FILE__)

void* Malloc(int size, int line, const char* filename);
void Free(void* arg, int line, const char* filename);

#ifndef WIN32
#include <sys/utsname.h>
#include <sys/param.h>
#endif

#ifdef WIN32
#define MAXPATHLEN 1024

#include <direct.h>

#ifndef S_ISDIR
#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#endif
#endif // WIN32

/* N_FILE_TYPES must be the last entry!! This saves us from counting. */
enum {NEDIT_RC, AUTOLOAD_NM, NEDIT_HISTORY, N_FILE_TYPES};

/* If anyone knows where to get this from system include files (in a machine
   independent way), please change this (L_cuserid is apparently not ANSI) */
#define MAXUSERNAMELEN 32

/* Ditto for the maximum length for a node name.  SYS_NMLN is not available
   on most systems, and I don't know what the portable alternative is. */
#ifdef SYS_NMLN
#define MAXNODENAMELEN SYS_NMLN
#else
#define MAXNODENAMELEN (MAXPATHLEN+2)
#endif

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))

const char* GetCurrentDir();
const char* GetHomeDir();
char* PrependHome(const char* filename, char* buf, size_t buflen);
const char* NeGetUserName();
const char* GetNameOfHost();
int Min(int i1, int i2);
const char* GetRCFileName(int type);

/*
**  Simple stack implementation which only keeps void pointers.
**  The stack must already be allocated and initialised:
**
**  Stack* stack = (Stack*) malloc__(sizeof(Stack));
**  stack->top = NULL;
**  stack->size = 0;
**
**  NULL is not allowed to pass in, as it is used to signal an empty stack.
**
**  The user should only ever care about Stack, stackObject is an internal
**  object. (So it should really go in utils.c. A forward reference was
**  refused by my compiler for some reason though.)
*/
typedef struct _stackObject
{
   void* value;
   struct _stackObject* next;
} stackObject;

typedef struct
{
   unsigned size;
   stackObject* top;
} Stack;

void Push(Stack* stack, const void* value);
void* Pop(Stack* stack);


std::string Trim(const std::string& str, const std::string& sep = " \t");
std::vector<std::string> Split(const std::string& str, const std::string& sep);
std::string GetStringIndex(const std::string& chaine,  int index, const char sep);

std::vector<std::string::size_type> SplitPosition(const std::string& str, const char sep);

#endif /* NEDIT_UTILS_H_INCLUDED */
