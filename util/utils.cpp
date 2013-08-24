#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>
#include <pwd.h>
#endif

#define DEFAULT_NEDIT_HOME ".nedit"

static const char* hiddenFileNames[N_FILE_TYPES] = {".nedit", ".neditmacro", ".neditdb"};
static const char* plainFileNames[N_FILE_TYPES] = {"nedit.rc", "autoload.nm", "nedit.history"};

static std::string buildFilePath(const char* dir, const char* file);
static bool isDir(const std::string& file);
static bool isRegFile(const std::string& file);

// -------------------------------------------------------------------------
void* Malloc(int size, int line, const char* filename)
{
   //printf("Malloc [%d] [%s] Size [%d]\n", line, filename, size);
   return malloc(size);
}

// -------------------------------------------------------------------------
void Free(void* arg, int line, const char* filename)
{
   //printf("Free [%d] [%s]\n", line, filename);
   free(arg);
}

// -------------------------------------------------------------------------
// return non-NULL value for the current working directory.
// If system call fails, provide a fallback value
// -------------------------------------------------------------------------
const char* GetCurrentDir()
{
   static char curdir[MAXPATHLEN] = "";

   if (!getcwd(curdir, (size_t) MAXPATHLEN))
   {
      perror("nedit: getcwd() fails");
      strcpy(curdir, ".");
   }
   return curdir;
}

// -------------------------------------------------------------------------
// return a non-NULL value for the user's home directory,
// without trailing slash.
// We try the  environment var and the system user database.
const char* GetHomeDir()
{
   static char homedir[MAXPATHLEN] = "";
#ifdef WIN32
   const char* ptr = getenv("USERPROFILE");
   if (ptr)
      strncpy(homedir, ptr, sizeof(homedir)-1);
#else
   const char* ptr;
   struct passwd* passwdEntry;
   size_t len;

   if (*homedir)
   {
      return homedir;
   }
   ptr=getenv("HOME");
   if (!ptr)
   {
      passwdEntry = getpwuid(getuid());
      if (passwdEntry && *(passwdEntry->pw_dir))
      {
         ptr= passwdEntry->pw_dir;
      }
      else
      {
         /* This is really serious, so just exit. */
         perror("nedit: getpwuid() failed ");
         exit(EXIT_FAILURE);
      }
   }
   strncpy(homedir, ptr, sizeof(homedir)-1);
   homedir[sizeof(homedir)-1]='\0';
   /* Fix trailing slash */
   len=strlen(homedir);
   if (len>1 && homedir[len-1]=='/')
   {
      homedir[len-1]='\0';
   }
#endif
   return homedir;
}

// -------------------------------------------------------------------------
// Return a pointer to the username of the current user in a statically allocated string.
// -------------------------------------------------------------------------
const char* NeGetUserName()
{
   static char* userName = NULL;
#ifdef WIN32
   if (userName) return userName;

   userName = new char[MAXPATHLEN];
   userName[0] = '\0';
   const char* ptr = getenv("USERNAME");
    if (ptr)
      strncpy(userName, ptr, MAXPATHLEN-1);

    return userName;
#else
   // cuserid has apparently been dropped from the ansi C standard, and if
   // strict ansi compliance is turned on (on Sun anyhow, maybe others), calls
   // to cuserid fail to compile.  Older versions of nedit try to use the
   // getlogin call first, then if that fails, use getpwuid and getuid.  This
   // results in the user-name of the original terminal being used, which is
   // not correct when the user uses the su command.  Now, getpwuid only:

   const struct passwd* passwdEntry;

   if (userName)
      return userName;

   passwdEntry = getpwuid(getuid());
   if (!passwdEntry)
   {
      // This is really serious, but sometimes username service
      // is misconfigured through no fault of the user.  Be nice
      // and let the user start nc anyway.
      perror("nedit: getpwuid() failed - reverting to $USER");
      return getenv("USER");
   }
   else
   {
      userName=(char*)malloc__(strlen(passwdEntry->pw_name)+1);
      strcpy(userName, passwdEntry->pw_name);
      return userName;
   }
#endif
}

// -------------------------------------------------------------------------
// Writes the hostname of the current system in string "hostname".
// -------------------------------------------------------------------------
const char* GetNameOfHost()
{
   static char hostname[MAXNODENAMELEN+1] = "";
   static bool hostnameFound = false;

   if (!hostnameFound)
   {
#ifdef WIN32
      hostnameFound = true;
      const char* ptr = getenv("COMPUTERNAME");
      if (ptr)
         strncpy(hostname, ptr, sizeof(hostname)-1);
#else
      struct utsname nameStruct;
      int rc = uname(&nameStruct);
      if (rc<0)
      {
         // Shouldn't ever happen, so we better exit() here
         perror("nedit: uname() failed ");
         exit(EXIT_FAILURE);
      }
      strcpy(hostname, nameStruct.nodename);
#endif
      hostnameFound = true;
   }
   return hostname;
}

// -------------------------------------------------------------------------
// Create a path: $HOME/filename
// Return "" if it doesn't fit into the buffer
// -------------------------------------------------------------------------
char* PrependHome(const char* filename, char* buf, size_t buflen)
{
   const char* homedir;
   size_t home_len, file_len;

   homedir=GetHomeDir();
   home_len=strlen(homedir);
   file_len=strlen(filename);
   if ((home_len+1+file_len)>=buflen)
   {
      buf[0]='\0';
   }
   else
   {
      strcpy(buf, homedir);
      strcat(buf, "/");
      strcat(buf, filename);
   }
   return buf;
}

// -------------------------------------------------------------------------
int Min(int i1, int i2)
{
   return i1 <= i2 ? i1 : i2;
}

// -------------------------------------------------------------------------
//  Returns a pointer to the name of an rc file of the requested type.
//
//  Preconditions:
//      - MAXPATHLEN is set to the max. allowed path length
//      - fullPath points to a buffer of at least MAXPATHLEN
//
//  Returns:
//      - NULL if an error occurs while creating a directory
//      - Pointer to a static array containing the file name
// -------------------------------------------------------------------------
const char* GetRCFileName(FileTypes type)
{
   static std::string rcFiles[N_FILE_TYPES];
   static bool namesDetermined = false;

   if (!namesDetermined)
   {
      std::string nedit_home = GetEnv("NEDIT_HOME");
      if (nedit_home.empty())
      {  //  No NEDIT_HOME
         // Let's try if ~/.nedit is a regular file or not.
         std::string  legacyFile = buildFilePath(GetHomeDir(), hiddenFileNames[NEDIT_RC]);
         if (isRegFile(legacyFile))
         {
            // This is a legacy setup with rc files in $HOME
            for (int i = 0; i < N_FILE_TYPES; i++)
            {
               rcFiles[i] = buildFilePath(GetHomeDir(), hiddenFileNames[i]);
            }
         }
         else
         {
            // ${HOME}/.nedit does not exist as a regular file.
            // FIXME: Devices, sockets and fifos are ignored for now.
            std::string defaultNEditHome = buildFilePath(GetHomeDir(), DEFAULT_NEDIT_HOME);
            if (!isDir(defaultNEditHome))
            {
               /* Create DEFAULT_NEDIT_HOME */
#ifdef WIN32
               if (mkdir(defaultNEditHome.c_str()) != 0)
#else
               if (mkdir(defaultNEditHome.c_str(), 0777) != 0)
#endif
               {
                  perror("nedit: Error while creating rc file directory"
                         " $HOME/" DEFAULT_NEDIT_HOME "\n"
                         " (Make sure all parent directories exist.)");
                  return NULL;
               }
            }

            /* All set for DEFAULT_NEDIT_HOME, let's copy the names */
            for (int i = 0; i < N_FILE_TYPES; i++)
            {
               rcFiles[i] = buildFilePath(defaultNEditHome.c_str(), plainFileNames[i]);
            }
         }
      }
      else
      {  // $NEDIT_HOME is set.
         // FIXME: Is this required? Does VMS know stat(), mkdir()?
         if (!isDir(nedit_home))
         {
            /* Create $NEDIT_HOME */
#ifdef WIN32
            if (mkdir(nedit_home.c_str()) != 0)
#else
            if (mkdir(nedit_home.c_str(), 0777) != 0)
#endif
            {
               perror("nedit: Error while creating rc file directory $NEDIT_HOME\n"
                      "nedit: (Make sure all parent directories exist.)");
               return NULL;
            }
         }

         // All set for NEDIT_HOME, let's copy the names
         for (int i = 0; i < N_FILE_TYPES; i++)
         {
            rcFiles[i] = buildFilePath(nedit_home.c_str(), plainFileNames[i]);
         }
      }

      namesDetermined = true;
   }

   return rcFiles[type].c_str();
}

// -------------------------------------------------------------------------
//  Builds a file path from 'dir' and 'file', watching for buffer overruns.
//
//  Preconditions:
//      - MAXPATHLEN is set to the max. allowed path length
//      - 'fullPath' points to a buffer of at least MAXPATHLEN
//      - 'dir' and 'file' are valid strings
//
//  Postcondition:
//      - 'fullpath' will contain 'dir/file'
//      - Exits when the result would be greater than MAXPATHLEN
// -------------------------------------------------------------------------
static std::string buildFilePath(const char* dir, const char* file)
{
   std::string fullpath = dir;
   fullpath += "/";
   fullpath += file;
   return fullpath;
}

// -------------------------------------------------------------------------
//  Returns true if 'file' is a directory, false otherwise.
//  Links are followed.
//
//  Preconditions:
//      - None
//
//  Returns:
//      - true for directories, false otherwise
// -------------------------------------------------------------------------
static bool isDir(const std::string& file)
{
   struct stat attribute;

   return ((stat(file.c_str(), &attribute) == 0) && S_ISDIR(attribute.st_mode));
}

// -------------------------------------------------------------------------
//  Returns true if 'file' is a regular file, false otherwise.
//  Links are followed.
//
//  Preconditions:
//      - None
//
//  Returns:
//      - true for regular files, false otherwise
// -------------------------------------------------------------------------
static bool isRegFile(const std::string& file)
{
   struct stat attribute;

   return ((stat(file.c_str(), &attribute) == 0) && S_ISREG(attribute.st_mode));
}

// -------------------------------------------------------------------------
//  Part of the simple stack. Accepts a stack and the pointer you want to
//  store. NULL is not allowed, as it is used in Pop() to signal an empty
//  stack.
// -------------------------------------------------------------------------
void Push(Stack* stack, const void* value)
{
   stackObject* pushee;

   // Throw away invalid parameters.
   if (NULL == value)
   {
      fprintf(stderr, "nedit: Internal error: NULL was pushed.\n");
      return;
   }
   if (NULL == stack)
   {
      fprintf(stderr, "nedit: Internal error: push() called with NULL stack.\n");
      return;
   }

   //  Allocate memory for new value.
   pushee = (stackObject*) malloc__(sizeof(stackObject));

   //  Put pushee on top of stack.
   pushee->value = (void*) value;
   pushee->next = stack->top;
   stack->top = pushee;
   (stack->size)++;

   return;
}

// -------------------------------------------------------------------------
//  Part of the simple stack, returns the topmost item from the stack or
//  NULL if the stack is empty. It also returns NULL if the stack itself
//  is NULL.
//
//  Precondition: The stack's top element is either NULL or a properly
//  initialised stackObject.
// -------------------------------------------------------------------------
void* Pop(Stack* stack)
{
   stackObject* popee;
   void* value;

   //  Throw away invalid parameter.
   if (NULL == stack)
   {
      fprintf(stderr, "nedit: Internal error: pop() called with NULL stack.\n");
      return NULL;
   }

   //  Return NULL if Stack is empty.
   if (NULL == stack->top)
   {
      return NULL;
   }

   //  Remove top entry in the stack.
   popee = stack->top;
   stack->top = popee->next;
   (stack->size)--;

   value = popee->value;
   free__((char*) popee);

   return value;
}

//// -------------------------------------------------------------------------
////  We currently don't need this function: In the only situation where we use
////  the stack, we empty it completely. This might come in handy if the stack
////  is ever used anywhere else.
////
////  Beware: Utterly untested.
//// -------------------------------------------------------------------------
//void FreeStack(Stack* stack)
//{
//    void* dummy;
//
//    while (NULL != (dummy = Pop(progStack))) {}
//
//    free__((char*) stack);
//}

// --------------------------------------------------------------------------
std::string Trim(const std::string& str, const std::string& trim)
{
   if ( str.empty() )
      return "";

   std::string::size_type begin = str.find_first_not_of( trim );
   if ( begin == std::string::npos )
      return "";

   std::string::size_type end = str.find_last_not_of( trim );

   return std::string( str, begin, end - begin + 1 );
}

// ------------------------------------------------------------------------
std::string GetStringIndex(const std::string& chaine, int index, const char sep)
{
   if ( index < 0 )
      return ""; // Rien à faire pour un index négatif

   const char* strPtr = chaine.c_str();

   while(index)
   {
      while(*strPtr)
      {
         if(*strPtr++ == sep) break;
      }

      if(*strPtr == '\0')
      {
         return "";
      }

      --index;
   }

   size_t start = strPtr - chaine.c_str();
   size_t end   = chaine.find(sep, start);

   if(end == std::string::npos) end = chaine.size();

   return std::string(chaine, start, end-start);
}

// --------------------------------------------------------------------------
std::vector<std::string::size_type> SplitPosition(const std::string& str, const char sep)
{
   std::vector<std::string::size_type> tmp;
   for(std::string::size_type i = 0; i < str.size(); ++i)
      if (str[i] == sep)
         tmp.push_back(i);
   return tmp;
}

// --------------------------------------------------------------------------
std::vector<std::string> Split(const std::string& str, const std::string& sep)
{
   std::vector<std::string> tmp;
   std::string::size_type pos = 0;
   std::string::size_type begin = 0;
   while ((pos = str.find(sep, begin)) != std::string::npos)
   {
      std::string value=str.substr(begin, pos - begin);
      tmp.push_back(value);
      begin = pos + sep.size();
   }
   std::string value = str.substr(begin);
   if (!value.empty())
      tmp.push_back(value);
   return tmp;
}

// --------------------------------------------------------------------------
std::string GetEnv(const char* varName)
{
   const char* envValue = getenv("NEDIT_HOME");
   if (envValue == NULL)
      return std::string();
   return envValue;
}