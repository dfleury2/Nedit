static const char CVSID[] = "$Id: tags.c,v 1.71 2009/06/23 21:30:09 lebert Exp $";
/*******************************************************************************
*                                                                              *
* tags.c -- Nirvana editor tag file handling                                   *
*                                                                              *
* Copyright (C) 1999 Mark Edel                                                 *
*                                                                              *
* This is free__ software; you can redistribute it and/or modify it under the    *
* terms of the GNU General Public License as published by the Free Software    *
* Foundation; either version 2 of the License, or (at your option) any later   *
* version. In addition, you may distribute versions of this program linked to  *
* Motif or Open Motif. See README for details.                                 *
*                                                                              *
* This software is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License        *
* for more details.                                                            *
*                                                                              *
* You should have received a copy of the GNU General Public License along with *
* software; if not, write to the Free Software Foundation, Inc., 59 Temple     *
* Place, Suite 330, Boston, MA  02111-1307 USA                                 *
*                                                                              *
* Nirvana Text Editor                                                          *
* July, 1993                                                                   *
*                                                                              *
* Written by Mark Edel                                                         *
*                                                                              *
*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "tags.h"
#include "Ne_Text_Buffer.h"
#include "Ne_Text_Editor.h"
#include "nedit.h"
#include "window.h"
#include "file.h"
#include "preferences.h"
#include "search.h"
#include "selection.h"
#include "calltips.h"
#include "../util/DialogF.h"
#include "../util/fileUtils.h"
#include "../util/misc.h"
#include "../util/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <sys/param.h>
#include <unistd.h>
#endif

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

#include <FL/fl_ask.H>

#define MAXLINE 2048
#define MAX_TAG_LEN 256
#define MAXDUPTAGS 100
#define MAX_TAG_INCLUDE_RECURSION_LEVEL 5

/* Take this many lines when making a tip from a tag. (should probably be a language-dependent option, but...) */
#define TIP_DEFAULT_LINES 4

#define STRSAVE(a)  ((a!=NULL)?strcpy((char*)malloc__(strlen(a)+1),(a)):strcpy((char*)malloc__(1),""))

typedef struct _tag
{
   struct _tag* next;
   const char* path;
   const char* name;
   const char* file;
   int language;
   const char* searchString; /* see comment below */
   int posInf;               /* see comment below */
   short index;
} tag;

/*
**  contents of                   tag->searchString   | tag->posInf
**    ctags, line num specified:  ""                  | line num
**    ctags, search expr specfd:  ctags search expr   | -1
**    etags  (emacs tags)         etags search string | search start pos
*/
enum searchDirection {FORWARD, BACKWARD};

static int loadTagsFile(const char* tagSpec, int index, int recLevel);
// TODO: static void findDefCB(Widget widget, WindowInfo* window, Atom* sel,Atom* type, char* value, int* length, int* format);
static void setTag(tag* t, const char* name, const char* file,
                   int language, const char* searchString, int posInf,
                   const char* tag);
// TODO: static int fakeRegExSearch(WindowInfo* window, char* buffer,const char* searchString, int* startPos, int* endPos);
static unsigned hashAddr(const char* key);
static void updateMenuItems();
static int addTag(const char* name, const char* file, int lang,
                  const char* search, int posInf,  const  char* path,
                  int index);
static int delTag(const char* name, const char* file, int lang, const char* search, int posInf,  int index);
static tag* getTag(const char* name, int search_type);
static int findDef(WindowInfo* window, const char* value, int search_type);
static int findAllMatches(WindowInfo* window, const char* string);
// TODO: static void findAllCB(Widget parent, XtPointer client_data, XtPointer call_data);
// TODO: static Widget createSelectMenu(Widget parent, char* label, int nArgs, char* args[]);
// TODO: static void editTaggedLocation(Widget parent, int i);
// TODO: static void showMatchingCalltip(Widget parent, int i);

static const char* rcs_strdup(const char* str);
static void rcs_free(const char* str);
static int searchLine(char* line, const char* regex);
static void rstrip(char* dst, const char* src);
static int nextTFBlock(FILE* fp, char* header, char** tiptext, int* lineAt, int* lineNo);
static int loadTipsFile(const char* tipsFile, int index, int recLevel);

/* Hash table of tags, implemented as an array.  Each bin contains a NULL-terminated linked list of parsed tags */
static tag** Tags = NULL;
static int DefTagHashSize = 10000;
/* list of loaded tags files */
tagFile* TagsFileList = NULL;

/* Hash table of calltip tags */
static tag** Tips = NULL;
tagFile* TipsFileList = NULL;


/* These are all transient global variables -- they don't hold any state between tag/tip lookups */
static int searchMode = TAG;
static const char* tagName;
static char tagFiles[MAXDUPTAGS][MAXPATHLEN];
static char tagSearch[MAXDUPTAGS][MAXPATHLEN];
static int  tagPosInf[MAXDUPTAGS];
static bool globAnchored;
static int globPos;
static int globHAlign;
static int globVAlign;
static int globAlignMode;

/* A wrapper for calling TextDShowCalltip */
static int tagsShowCalltip(WindowInfo* window, char* text)
{
   if (text)
      return ShowCalltip(window, text, globAnchored, globPos, globHAlign, globVAlign, globAlignMode);
   else
      return 0;
}

/* Set the head of the proper file list (Tags or Tips) to t */
static tagFile* setFileListHead(tagFile* t, int file_type)
{
   if (file_type == TAG)
      TagsFileList = t;
   else
      TipsFileList = t;
   return t;
}

/*      Compute hash address from a string key */
static unsigned hashAddr(const char* key)
{
   unsigned s=strlen(key);
   unsigned a=0,x=0,i;

   for (i=0; (i+3)<s; i += 4)
   {
      strncpy((char*)&a,&key[i],4);
      x += a;
   }

   for (a=1; i<(s+1); i++, a *= 256)
      x += key[i] * a;

   return x;
}

/*      Retrieve a tag structure from the hash table */
static tag* getTag(const char* name, int search_type)
{
   static char lastName[MAXLINE];
   static tag* t;
   static int addr;
   tag** table;

   if (search_type == TIP)
      table = Tips;
   else
      table = Tags;

   if (table == NULL) return NULL;

   if (name)
   {
      addr = hashAddr(name) % DefTagHashSize;
      t = table[addr];
      strcpy(lastName,name);
   }
   else if (t)
   {
      name = lastName;
      t = t->next;
   }
   else return NULL;

   for (; t; t = t->next)
      if (!strcmp(name,t->name)) return t;
   return NULL;
}

/* Add a tag specification to the hash table
**   Return Value:  0 ... tag already existing, spec not added
**                  1 ... tag spec is new, added.
**   (We don't return boolean as the return value is used as counter increment!)
**
*/
static int addTag(const char* name, const char* file, int lang,
                  const char* search, int posInf, const char* path, int index)
{
   int addr = hashAddr(name) % DefTagHashSize;
   tag* t;
   char newfile[MAXPATHLEN];
   tag** table;

   if (searchMode == TIP)
   {
      if (Tips == NULL)
         Tips = (tag**)calloc(DefTagHashSize, sizeof(tag*));
      table = Tips;
   }
   else
   {
      if (Tags == NULL)
         Tags = (tag**)calloc(DefTagHashSize, sizeof(tag*));
      table = Tags;
   }

   if (*file == '/')
      strcpy(newfile,file);
   else
      sprintf(newfile,"%s%s", path, file);

   NormalizePathname(newfile);

   for (t = table[addr]; t; t = t->next)
   {
      if (strcmp(name,t->name)) continue;
      if (lang != t->language) continue;
      if (strcmp(search,t->searchString)) continue;
      if (posInf != t->posInf) continue;
      if (*t->file == '/' && strcmp(newfile,t->file)) continue;
      if (*t->file != '/')
      {
         char tmpfile[MAXPATHLEN];
         sprintf(tmpfile, "%s%s", t->path, t->file);
         NormalizePathname(tmpfile);
         if (strcmp(newfile, tmpfile)) continue;
      }
      return 0;
   }

   t = (tag*) malloc__(sizeof(tag));
   setTag(t, name, file, lang, search, posInf, path);
   t->index = index;
   t->next = table[addr];
   table[addr] = t;
   return 1;
}

/*  Delete a tag from the cache.
 *  Search is limited to valid matches of 'name','file', 'search', posInf, and 'index'.
 *  EX: delete all tags matching index 2 ==>
 *                      delTag(tagname,NULL,-2,NULL,-2,2);
 *  (posInf = -2 is an invalid match, posInf range: -1 .. +MAXINT,
     lang = -2 is also an invalid match)
 */
static int delTag(const char* name, const char* file, int lang, const char* search, int posInf, int index)
{
   tag* t, *last;
   int start,finish,i,del=0;
   tag** table;

   if (searchMode == TIP)
      table = Tips;
   else
      table = Tags;

   if (table == NULL) return false;
   if (name)
      start = finish = hashAddr(name) % DefTagHashSize;
   else
   {
      start = 0;
      finish = DefTagHashSize;
   }
   for (i = start; i<finish; i++)
   {
      for (last = NULL, t = table[i]; t; last = t, t = t?t->next:table[i])
      {
         if (name && strcmp(name,t->name)) continue;
         if (index && index != t->index) continue;
         if (file && strcmp(file,t->file)) continue;
         if (lang >= PLAIN_LANGUAGE_MODE && lang != t->language) continue;
         if (search && strcmp(search,t->searchString)) continue;
         if (posInf == t->posInf) continue;
         if (last)
            last->next = t->next;
         else
            table[i] = t->next;
         rcs_free(t->name);
         rcs_free(t->file);
         rcs_free(t->searchString);
         rcs_free(t->path);
         free__(t);
         t = NULL;
         del++;
      }
   }
   return del>0;
}

/* used  in AddRelTagsFile and AddTagsFile */
static int tagFileIndex = 0;

/*
** AddRelTagsFile():  Rescan tagSpec for relative tag file specs
** (not starting with [/~]) and extend the tag files list if in
** windowPath a tags file matching the relative spec has been found.
*/
int AddRelTagsFile(const char* tagSpec, const char* windowPath, int file_type)
{
   tagFile* t;
   int added=0;
   struct stat statbuf;
   char* filename;
   char pathName[MAXPATHLEN];
   char* tmptagSpec;
   tagFile* FileList;

   searchMode = file_type;
   if (searchMode == TAG)
      FileList = TagsFileList;
   else
      FileList = TipsFileList;

   tmptagSpec = (char*) malloc__(strlen(tagSpec)+1);
   strcpy(tmptagSpec, tagSpec);
   for (filename = strtok(tmptagSpec, ":"); filename; filename = strtok(NULL, ":"))
   {
      if (*filename == '/' || *filename == '~')
         continue;
      if (windowPath && *windowPath)
      {
         strcpy(pathName, windowPath);
      }
      else
      {
         strcpy(pathName, GetCurrentDir());
      }
      strcat(pathName, "/");
      strcat(pathName, filename);
      NormalizePathname(pathName);

      for (t = FileList; t && strcmp(t->filename, pathName); t = t->next);
      if (t)
      {
         added=1;
         continue;
      }
      if (stat(pathName, &statbuf) != 0)
         continue;
      t = (tagFile*) malloc__(sizeof(tagFile));
      t->filename = STRSAVE(pathName);
      t->loaded = 0;
      t->date = statbuf.st_mtime;
      t->index = ++tagFileIndex;
      t->next = FileList;
      FileList = setFileListHead(t, file_type);
      added=1;
   }
   free__(tmptagSpec);
   updateMenuItems();
   if (added)
      return true;
   else
      return false;
}

/*
**  AddTagsFile():  Set up the the list of tag files to manage from a file spec.
**  The file spec comes from the X-Resource Nedit.tags: It can list multiple
**  tags files, specified by separating them with colons. The .Xdefaults would
**  look like this:
**    Nedit.tags: <tagfile1>:<tagfile2>
**  Returns true if all files were found in the FileList or loaded successfully,
**  false otherwise.
*/
int AddTagsFile(const char* tagSpec, int file_type)
{
   tagFile* t;
   int added=1;
   struct stat statbuf;
   char* filename;
   char pathName[MAXPATHLEN];
   char* tmptagSpec;
   tagFile* FileList;

   /* To prevent any possible segfault */
   if (tagSpec == NULL)
   {
      fprintf(stderr, "nedit: Internal Error!\n" "  Passed NULL pointer to AddTagsFile!\n");
      return false;
   }

   searchMode = file_type;
   if (searchMode == TAG)
      FileList = TagsFileList;
   else
      FileList = TipsFileList;

   tmptagSpec = new char[strlen(tagSpec)+1];
   strcpy(tmptagSpec, tagSpec);
   for (filename = strtok(tmptagSpec,":"); filename; filename = strtok(NULL,":"))
   {
      if (*filename != '/')
      {
         strcpy(pathName, GetCurrentDir());
         strcat(pathName,"/");
         strcat(pathName,filename);
      }
      else
      {
         strcpy(pathName,filename);
      }
      NormalizePathname(pathName);

      for (t = FileList; t && strcmp(t->filename,pathName); t = t->next);
      if (t)
      {
         /* This file is already in the list.  It's easiest to just
             refcount all tag/tip files even though we only actually care
             about tip files. */
         ++(t->refcount);
         added=1;
         continue;
      }
      if (stat(pathName,&statbuf) != 0)
      {
         /* Problem reading this tags file.  Return false */
         added = 0;
         continue;
      }
      t = (tagFile*) malloc__(sizeof(tagFile));
      t->filename = STRSAVE(pathName);
      t->loaded = 0;
      t->date = statbuf.st_mtime;
      t->index = ++tagFileIndex;
      t->next = FileList;
      t->refcount = 1;
      FileList = setFileListHead(t, file_type);
   }
   delete[] tmptagSpec;
   updateMenuItems();
   if (added)
      return true;
   else
      return false;
}

/* Un-manage a colon-delimited set of tags files
 * Return true if all files were found in the FileList and unloaded, false
 * if any file was not found in the FileList.
 * "file_type" is either TAG or TIP
 * If "force_unload" is true, a calltips file will be deleted even if its
 * refcount is nonzero.
 */
int DeleteTagsFile(const char* tagSpec, int file_type, bool force_unload)
{
   tagFile* t, *last;
   tagFile* FileList;
   char pathName[MAXPATHLEN], *tmptagSpec, *filename;
   int removed;

   /* To prevent any possible segfault */
   if (tagSpec == NULL)
   {
      fprintf(stderr, "nedit: Internal Error: Passed NULL pointer to DeleteTagsFile!\n");
      return false;
   }

   searchMode = file_type;
   if (searchMode == TAG)
      FileList = TagsFileList;
   else
      FileList = TipsFileList;

   tmptagSpec = (char*) malloc__(strlen(tagSpec)+1);
   strcpy(tmptagSpec, tagSpec);
   removed=1;
   for (filename = strtok(tmptagSpec,":"); filename;
         filename = strtok(NULL,":"))
   {
      if (*filename != '/')
      {
         strcpy(pathName, GetCurrentDir());
         strcat(pathName,"/");
         strcat(pathName,filename);
      }
      else
      {
         strcpy(pathName,filename);
      }
      NormalizePathname(pathName);

      for (last=NULL,t = FileList; t; last = t,t = t->next)
      {
         if (strcmp(t->filename, pathName))
            continue;
         /* Don't unload tips files with nonzero refcounts unless forced */
         if (searchMode == TIP && !force_unload && --t->refcount > 0)
         {
            break;
         }
         if (t->loaded)
            delTag(NULL,NULL,-2,NULL,-2,t->index);
         if (last) last->next = t->next;
         else FileList = setFileListHead(t->next, file_type);
         free__(t->filename);
         free__(t);
         updateMenuItems();
         break;
      }
      /* If any file can't be removed, return false */
      if (!t)
         removed = 0;
   }
   if (removed)
      return true;
   else
      return false;
}

/*
** Update the "Find Definition", "Unload Tags File", "Show Calltip",
** and "Unload Calltips File" menu items in the existing windows.
*/
static void updateMenuItems()
{
   WindowInfo* w;
   bool tipStat=false, tagStat=false;

   if (TipsFileList) tipStat=true;
   if (TagsFileList) tagStat=true;

   for (w=WindowList; w!=NULL; w=w->next)
   {
      if (!IsTopDocument(w))
         continue;
// TODO:       NeSetSensitive(w->showTipItem, tipStat || tagStat);
// TODO:       NeSetSensitive(w->unloadTipsMenuItem, tipStat);
// TODO:       NeSetSensitive(w->findDefItem, tagStat);
// TODO:       NeSetSensitive(w->unloadTagsMenuItem, tagStat);
   }
}

/*
** Scans one <line> from a ctags tags file (<index>) in tagPath.
** Return value: Number of tag specs added.
*/
static int scanCTagsLine(const char* line, const char* tagPath, int index)
{
   char name[MAXLINE], searchString[MAXLINE];
   char file[MAXPATHLEN];
   char* posTagREEnd, *posTagRENull;
   int  nRead, pos;

   nRead = sscanf(line, "%s\t%s\t%[^\n]", name, file, searchString);
   if (nRead != 3)
      return 0;
   if (*name == '!')
      return 0;

   /*
   ** Guess the end of searchString:
   ** Try to handle original ctags and exuberant ctags format:
   */
   if (searchString[0] == '/' || searchString[0] == '?')
   {

      pos=-1; /* "search expr without pos info" */

      /* Situations: /<ANY expr>/\0
      **             ?<ANY expr>?\0          --> original ctags
      **             /<ANY expr>/;"  <flags>
      **             ?<ANY expr>?;"  <flags> --> exuberant ctags
      */
      posTagREEnd = strrchr(searchString, ';');
      posTagRENull = strchr(searchString, 0);
      if (!posTagREEnd || (posTagREEnd[1] != '"') ||
            (posTagRENull[-1] == searchString[0]))
      {
         /*  -> original ctags format = exuberant ctags format 1 */
         posTagREEnd = posTagRENull;
      }
      else
      {
         /* looks like exuberant ctags format 2 */
         *posTagREEnd = 0;
      }

      /*
      ** Hide the last delimiter:
      **   /<expression>/    becomes   /<expression>
      **   ?<expression>?    becomes   ?<expression>
      ** This will save a little work in fakeRegExSearch.
      */
      if (posTagREEnd > (searchString+2))
      {
         posTagREEnd--;
         if (searchString[0] == *posTagREEnd)
            *posTagREEnd=0;
      }
   }
   else
   {
      pos=atoi(searchString);
      *searchString=0;
   }
   /* No ability to read language mode right now */
   return addTag(name, file, PLAIN_LANGUAGE_MODE, searchString, pos, tagPath, index);
}

/*
 * Scans one <line> from an etags (emacs) tags file (<index>) in tagPath.
 * recLevel = current recursion level for tags file including
 * file = destination definition file. possibly modified. len=MAXPATHLEN!
 * Return value: Number of tag specs added.
 */
static int scanETagsLine(const char* line, const char* tagPath, int index,
                         char* file, int recLevel)
{
   char name[MAXLINE], searchString[MAXLINE];
   char incPath[MAXPATHLEN];
   int pos, len;
   const char* posDEL, *posSOH, *posCOM;

   /* check for destination file separator  */
   if (line[0]==12)  /* <np> */
   {
      *file=0;
      return 0;
   }

   /* check for standard definition line */
   posDEL=strchr(line, '\177');
   posSOH=strchr(line, '\001');
   posCOM=strrchr(line, ',');
   if (*file && posDEL && (posSOH > posDEL) && (posCOM > posSOH))
   {
      /* exuberant ctags -e style  */
      len=Min(MAXLINE-1, posDEL - line);
      strncpy(searchString, line, len);
      searchString[len]=0;
      len=Min(MAXLINE-1, (posSOH - posDEL) - 1);
      strncpy(name, posDEL + 1, len);
      name[len]=0;
      pos=atoi(posCOM+1);
      /* No ability to set language mode for the moment */
      return addTag(name, file, PLAIN_LANGUAGE_MODE, searchString, pos,
                    tagPath, index);
   }
   if (*file && posDEL && (posCOM > posDEL))
   {
      /* old etags style, part  name<soh>  is missing here! */
      len=Min(MAXLINE-1, posDEL - line);
      strncpy(searchString, line, len);
      searchString[len]=0;
      /* guess name: take the last alnum (plus _) part of searchString */
      while (--len >= 0)
      {
         if (isalnum((unsigned char)searchString[len]) ||
               (searchString[len] == '_'))
            break;
      }
      if (len<0)
         return 0;
      pos=len;
      while (pos >= 0 && (isalnum((unsigned char)searchString[pos]) ||
                          (searchString[pos] == '_')))
         pos--;
      strncpy(name, searchString + pos + 1, len - pos);
      name[len - pos] = 0; /* name ready */
      pos=atoi(posCOM+1);
      return addTag(name, file, PLAIN_LANGUAGE_MODE, searchString, pos,
                    tagPath, index);
   }
   /* check for destination file spec */
   if (*line && posCOM)
   {
      len=Min(MAXPATHLEN-1, posCOM - line);
      strncpy(file, line, len);
      file[len]=0;
      /* check if that's an include file ... */
      if (!(strncmp(posCOM+1, "include", 7)))
      {
         if (*file != '/')
         {
            if ((strlen(tagPath) + strlen(file)) >= MAXPATHLEN)
            {
               fprintf(stderr, "tags.c: MAXPATHLEN overflow\n");
               *file=0; /* invalidate */
               return 0;
            }
            strcpy(incPath, tagPath);
            strcat(incPath, file);
            CompressPathname(incPath);
            return(loadTagsFile(incPath, index, recLevel+1));
         }
         else
         {
            return(loadTagsFile(file, index, recLevel+1));
         }
      }
   }
   return 0;
}

/* Tag File Type */
typedef enum
{
   TFT_CHECK, TFT_ETAGS, TFT_CTAGS
} TFT;

/*
** Loads tagsFile into the hash table.
** Returns the number of added tag specifications.
*/
static int loadTagsFile(const char* tagsFile, int index, int recLevel)
{
   FILE* fp = NULL;
   char line[MAXLINE];
   char file[MAXPATHLEN], tagPath[MAXPATHLEN];
   char resolvedTagsFile[MAXPATHLEN+1];
   int nTagsAdded=0;
   int tagFileType = TFT_CHECK;

   if (recLevel > MAX_TAG_INCLUDE_RECURSION_LEVEL)
   {
      return 0;
   }
   /* the path of the tags file must be resolved to find the right files:
    * definition source files are (in most cases) specified relatively inside
    * the tags file to the tags files directory.
    */
   if (!ResolvePath(tagsFile, resolvedTagsFile))
   {
      return 0;
   }

   /* Open the file */
   if ((fp = fopen(resolvedTagsFile, "r")) == NULL)
   {
      return 0;
   }

   ParseFilename(resolvedTagsFile, NULL, tagPath);

   /* Read the file and store its contents */
   while (fgets(line, MAXLINE, fp))
   {

      /* This might take a while if you have a huge tags file (like I do)..
         keep the windows up to date and post a busy cursor so the user
         doesn't think we died. */

      AllWindowsBusy("Loading tags file...");

      /* the first character in the file decides if the file is treat as
         etags or ctags file.
       */
      if (tagFileType==TFT_CHECK)
      {
         if (line[0]==12) /* <np> */
            tagFileType=TFT_ETAGS;
         else
            tagFileType=TFT_CTAGS;
      }
      if (tagFileType==TFT_CTAGS)
      {
         nTagsAdded += scanCTagsLine(line, tagPath, index);
      }
      else
      {
         nTagsAdded += scanETagsLine(line, tagPath, index, file, recLevel);
      }
   }
   fclose(fp);

   AllWindowsUnbusy();
   return nTagsAdded;
}

/*
** Given a tag name, lookup the file and path of the definition
** and the proper search string. Returned strings are pointers
** to internal storage which are valid until the next loadTagsFile call.
**
** Invocation with name != NULL (containing the searched definition)
**    --> returns first definition of  name
** Successive invocation with name == NULL
**    --> returns further definitions (resulting from multiple tags files)
**
** Return Value: true:  tag spec found
**               false: no (more) definitions found.
*/
#define TAG_STS_ERR_FMT "NEdit: Error getting status for tag file %s\n"
int LookupTag(const char* name, const char** file, int* language,
              const char** searchString, int* pos, const char** path,
              int search_type)
{
   tag* t;
   tagFile* tf;
   struct stat statbuf;
   tagFile* FileList;
   int load_status;

   searchMode = search_type;
   if (searchMode == TIP)
      FileList = TipsFileList;
   else
      FileList = TagsFileList;

   /*
   ** Go through the list of all tags Files:
   **   - load them (if not already loaded)
   **   - check for update of the tags file and reload it in that case
   **   - save the modification date of the tags file
   **
   ** Do this only as long as name != NULL, not for sucessive calls
   ** to find multiple tags specs.
   **
   */
   for (tf = FileList; tf && name; tf = tf->next)
   {
      if (tf->loaded)
      {
         if (stat(tf->filename,&statbuf) != 0)   /*  */
         {
            fprintf(stderr, TAG_STS_ERR_FMT, tf->filename);
         }
         else
         {
            if (tf->date == statbuf.st_mtime)
            {
               /* current tags file tf is already loaded and up to date */
               continue;
            }
         }
         /* tags file has been modified, delete it's entries and reload it */
         delTag(NULL,NULL,-2,NULL,-2,tf->index);
      }
      /* If we get here we have to try to (re-) load the tags file */
      if (FileList == TipsFileList)
         load_status = loadTipsFile(tf->filename, tf->index, 0);
      else
         load_status = loadTagsFile(tf->filename, tf->index, 0);
      if (load_status)
      {
         if (stat(tf->filename,&statbuf) != 0)
         {
            if (!tf->loaded)
            {
               /* if tf->loaded == 1 we already have seen the error msg */
               fprintf(stderr, TAG_STS_ERR_FMT, tf->filename);
            }
         }
         else
         {
            tf->date = statbuf.st_mtime;
         }
         tf->loaded = 1;
      }
      else
      {
         tf->loaded = 0;
      }
   }

   t = getTag(name, search_type);

   if (!t)
   {
      return false;
   }
   else
   {
      *file = t->file;
      *language = t->language;
      *searchString = t->searchString;
      *pos = t->posInf;
      *path = t->path;
      return true;
   }
}

/*
** This code path is followed if the request came from either
** FindDefinition or FindDefCalltip.  This should probably be refactored.
*/
static int findDef(WindowInfo* window, const char* value, int search_type)
{
   static char tagText[MAX_TAG_LEN + 1];
   const char* p;
   char message[MAX_TAG_LEN+40];
   int l, ml, status = 0;

   searchMode = search_type;
   l = strlen(value);
   if (l <= MAX_TAG_LEN)
   {
      /* should be of type text??? */
      for (p = value; *p && isascii(*p); p++)
      {
      }
      if (!(*p))
      {
         ml = ((l < MAX_TAG_LEN) ? (l) : (MAX_TAG_LEN));
         strncpy(tagText, value, ml);
         tagText[ml] = '\0';
         /* See if we can find the tip/tag */
         status = findAllMatches(window, tagText);

         /* If we didn't find a requested calltip, see if we can use a tag */
         if (status == 0 && search_type == TIP && TagsFileList != NULL)
         {
            searchMode = TIP_FROM_TAG;
            status = findAllMatches(window, tagText);
         }

         if (status == 0)
         {
            /* Didn't find any matches */
            if (searchMode == TIP_FROM_TAG || searchMode == TIP)
            {
               sprintf(message, "No match for \"%s\" in calltips or tags.", tagName);
               tagsShowCalltip(window, message);
            }
            else
            {
               DialogF(DF_WARN, window->textArea, 1, "Tags", "\"%s\" not found in tags file%s", "OK", tagName, (TagsFileList && TagsFileList->next) ? "s" : "");
            }
         }
      }
      else
      {
         fprintf(stderr, "NEdit: Can't handle non 8-bit text\n");
         fl_beep();
      }
   }
   else
   {
      fprintf(stderr, "NEdit: Tag Length too long.\n");
      fl_beep();
   }
   return status;
}

// TODO: /*
// TODO: ** Lookup the definition for the current primary selection the currently
// TODO: ** loaded tags file and bring up the file and line that the tags file
// TODO: ** indicates.
// TODO: */
// TODO: static void findDefinitionHelper(WindowInfo* window, Time time, const char* arg,
// TODO:                                  int search_type)
// TODO: {
// TODO:    if (arg)
// TODO:    {
// TODO:       findDef(window, arg, search_type);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       searchMode = search_type;
// TODO:       XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
// TODO:                           (XtSelectionCallbackProc)findDefCB, window, time);
// TODO:    }
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** See findDefHelper
// TODO: */
// TODO: void FindDefinition(WindowInfo* window, Time time, const char* arg)
// TODO: {
// TODO:    findDefinitionHelper(window, time, arg, TAG);
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** See findDefHelper
// TODO: */
// TODO: void FindDefCalltip(WindowInfo* window, Time time, const char* arg)
// TODO: {
// TODO:    /* Reset calltip parameters to reasonable defaults */
// TODO:    globAnchored = false;
// TODO:    globPos = -1;
// TODO:    globHAlign = TIP_LEFT;
// TODO:    globVAlign = TIP_BELOW;
// TODO:    globAlignMode = TIP_SLOPPY;
// TODO: 
// TODO:    findDefinitionHelper(window, time, arg, TIP);
// TODO: }
// TODO: 
// TODO: /* Callback function for FindDefinition */
// TODO: static void findDefCB(Widget widget, WindowInfo* window, Atom* sel,
// TODO:                       Atom* type, char* value, int* length, int* format)
// TODO: {
// TODO:    /* skip if we can't get the selection data, or it's obviously too long */
// TODO:    if (*type == XT_CONVERT_FAIL || value == NULL)
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       findDef(window, value, searchMode);
// TODO:    }
// TODO:    XtFree(value);
// TODO: }
// TODO: 
/*
** Try to display a calltip
**  anchored:       If true, tip appears at position pos
**  lookup:         If true, text is considered a key to be searched for in the
**                  tip and/or tag database depending on search_type
**  search_type:    Either TIP or TIP_FROM_TAG
*/
int ShowTipString(WindowInfo* window, char* text, bool anchored,
                  int pos, bool lookup, int search_type, int hAlign, int vAlign,
                  int alignMode)
{

   if (search_type == TAG) return 0;

   /* So we don't have to carry all of the calltip alignment info around */
   globAnchored = anchored;
   globPos = pos;
   globHAlign = hAlign;
   globVAlign = vAlign;
   globAlignMode = alignMode;

   /* If this isn't a lookup request, just display it. */
   if (!lookup)
      return tagsShowCalltip(window, text);
   else
      return findDef(window, text, search_type);
}

// store all of the info into a pre-allocated tags struct
static void setTag(tag* t, const char* name, const char* file,
                   int language, const char* searchString, int posInf,
                   const char* path)
{
   t->name         = rcs_strdup(name);
   t->file         = rcs_strdup(file);
   t->language     = language;
   t->searchString = rcs_strdup(searchString);
   t->posInf       = posInf;
   t->path         = rcs_strdup(path);
}

// TODO: /*
// TODO: ** ctags search expressions are literal strings with a search direction flag,
// TODO: ** line starting "^" and ending "$" delimiters. This routine translates them
// TODO: ** into NEdit compatible regular expressions and does the search.
// TODO: ** Etags search expressions are plain literals strings, which
// TODO: **
// TODO: ** If in_buffer is not NULL then it is searched instead of the window buffer.
// TODO: ** In this case in_buffer should be an malloc__ allocated buffer and the
// TODO: ** caller is responsible for freeing it.
// TODO: */
// TODO: static int fakeRegExSearch(WindowInfo* window, char* in_buffer,
// TODO:                            const char* searchString, int* startPos, int* endPos)
// TODO: {
// TODO:    int found, searchStartPos, dir, ctagsMode;
// TODO:    char searchSubs[3*MAXLINE+3], *outPtr;
// TODO:    const char* fileString, *inPtr;
// TODO: 
// TODO:    if (in_buffer == NULL)
// TODO:    {
// TODO:       /* get the entire (sigh) text buffer from the text area widget */
// TODO:       fileString = BufAsString(window->buffer);
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       fileString = in_buffer;
// TODO:    }
// TODO: 
// TODO:    /* determine search direction and start position */
// TODO:    if (*startPos != -1)   /* etags mode! */
// TODO:    {
// TODO:       dir = SEARCH_FORWARD;
// TODO:       searchStartPos = *startPos;
// TODO:       ctagsMode=0;
// TODO:    }
// TODO:    else if (searchString[0] == '/')
// TODO:    {
// TODO:       dir = SEARCH_FORWARD;
// TODO:       searchStartPos = 0;
// TODO:       ctagsMode=1;
// TODO:    }
// TODO:    else if (searchString[0] == '?')
// TODO:    {
// TODO:       dir = SEARCH_BACKWARD;
// TODO:       /* searchStartPos = window->buffer->length; */
// TODO:       searchStartPos = strlen(fileString);
// TODO:       ctagsMode=1;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       fprintf(stderr, "NEdit: Error parsing tag file search string");
// TODO:       return false;
// TODO:    }
// TODO: 
// TODO:    /* Build the search regex. */
// TODO:    outPtr=searchSubs;
// TODO:    if (ctagsMode)
// TODO:    {
// TODO:       inPtr=searchString+1; /* searchString[0] is / or ? --> search dir */
// TODO:       if (*inPtr == '^')
// TODO:       {
// TODO:          /* If the first char is a caret then it's a RE line start delim */
// TODO:          *outPtr++ = *inPtr++;
// TODO:       }
// TODO:    }
// TODO:    else      /* etags mode, no search dir spec, no leading caret */
// TODO:    {
// TODO:       inPtr=searchString;
// TODO:    }
// TODO:    while (*inPtr)
// TODO:    {
// TODO:       if ((*inPtr=='\\' && inPtr[1]=='/') ||
// TODO:             (*inPtr=='\r' && inPtr[1]=='$' && !inPtr[2])
// TODO:          )
// TODO:       {
// TODO:          /* Remove:
// TODO:             - escapes (added by standard and exuberant ctags) from slashes
// TODO:             - literal CRs generated by standard ctags for DOSified sources
// TODO:          */
// TODO:          inPtr++;
// TODO:       }
// TODO:       else if (strchr("()-[]<>{}.|^*+?&\\", *inPtr)
// TODO:                || (*inPtr == '$' && (inPtr[1]||(!ctagsMode))))
// TODO:       {
// TODO:          /* Escape RE Meta Characters to match them literally.
// TODO:             Don't escape $ if it's the last charcter of the search expr
// TODO:             in ctags mode; always escape $ in etags mode.
// TODO:           */
// TODO:          *outPtr++ = '\\';
// TODO:          *outPtr++ = *inPtr++;
// TODO:       }
// TODO:       else if (isspace((unsigned char)*inPtr))     /* col. multiple spaces */
// TODO:       {
// TODO:          *outPtr++ = '\\';
// TODO:          *outPtr++ = 's';
// TODO:          *outPtr++ = '+';
// TODO:          do
// TODO:          {
// TODO:             inPtr++ ;
// TODO:          }
// TODO:          while (isspace((unsigned char)*inPtr));
// TODO:       }
// TODO:       else                  /* simply copy all other characters */
// TODO:       {
// TODO:          *outPtr++ = *inPtr++;
// TODO:       }
// TODO:    }
// TODO:    *outPtr=0; /* Terminate searchSubs */
// TODO: 
// TODO:    found = SearchString(fileString, searchSubs, dir, SEARCH_REGEX,
// TODO:                         false, searchStartPos, startPos, endPos, NULL, NULL, NULL);
// TODO: 
// TODO:    if (!found && !ctagsMode)
// TODO:    {
// TODO:       /* position of the target definition could have been drifted before
// TODO:          startPos, if nothing has been found by now try searching backward
// TODO:          again from startPos.
// TODO:       */
// TODO:       found = SearchString(fileString, searchSubs, SEARCH_BACKWARD,
// TODO:                            SEARCH_REGEX, false, searchStartPos, startPos, endPos, NULL,
// TODO:                            NULL, NULL);
// TODO:    }
// TODO: 
// TODO:    /* return the result */
// TODO:    if (found)
// TODO:    {
// TODO:       /* *startPos and *endPos are set in SearchString*/
// TODO:       return true;
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       /* startPos, endPos left untouched by SearchString if search failed. */
// TODO:       XBell(TheDisplay, 0);
// TODO:       return false;
// TODO:    }
// TODO: }

// Finds all matches and handles tag "collisions". Prompts user with a
// list of collided tags in the hash table and allows the user to select
// the correct one.
static int findAllMatches(WindowInfo* window, const char* string)
{
// TODO:    Widget dialogParent = window->textArea;
   char filename[MAXPATHLEN], pathname[MAXPATHLEN];
   char temp[32+2*MAXPATHLEN+MAXLINE];
   const char* fileToSearch, *searchString, *tagPath;
   char** dupTagsList;
   int startPos, i, pathMatch=0, samePath=0, langMode, nMatches=0;

   /* verify that the string is reasonable as a tag */
   if (*string == '\0' || strlen(string) > MAX_TAG_LEN)
   {
      fl_beep();
      return -1;
   }
   tagName=string;

   /* First look up all of the matching tags */
   while (LookupTag(string, &fileToSearch, &langMode, &searchString, &startPos,
                    &tagPath, searchMode))
   {
      /* Skip this tag if it has a language mode that doesn't match the
          current language mode, but don't skip anything if the window is in
          PLAIN_LANGUAGE_MODE. */
      if (window->languageMode != PLAIN_LANGUAGE_MODE &&
            GetPrefSmartTags() && langMode != PLAIN_LANGUAGE_MODE &&
            langMode != window->languageMode)
      {
         string=NULL;
         continue;
      }
      if (*fileToSearch == '/')
         strcpy(tagFiles[nMatches], fileToSearch);
      else
         sprintf(tagFiles[nMatches],"%s%s",tagPath,fileToSearch);
      strcpy(tagSearch[nMatches],searchString);
      tagPosInf[nMatches]=startPos;
      ParseFilename(tagFiles[nMatches], filename, pathname);
      /* Is this match in the current file?  If so, use it! */
      if (GetPrefSmartTags() && !strcmp(window->filename,filename)
            && !strcmp(window->path,pathname))
      {
         if (nMatches)
         {
            strcpy(tagFiles[0],tagFiles[nMatches]);
            strcpy(tagSearch[0],tagSearch[nMatches]);
            tagPosInf[0]=tagPosInf[nMatches];
         }
         nMatches = 1;
         break;
      }
      /* Is this match in the same dir. as the current file? */
      if (!strcmp(window->path,pathname))
      {
         samePath++;
         pathMatch=nMatches;
      }
      if (++nMatches >= MAXDUPTAGS)
      {
         DialogF(DF_WARN, NULL, 1, "Tags", "Too many duplicate tags, first %d shown", "OK", MAXDUPTAGS);
         break;
      }
      /* Tell LookupTag to look for more definitions of the same tag: */
      string = NULL;
   }

   /* Did we find any matches? */
   if (!nMatches)
   {
      return 0;
   }

   /* Only one of the matches is in the same dir. as this file.  Use it. */
   if (GetPrefSmartTags() && samePath == 1 && nMatches > 1)
   {
      strcpy(tagFiles[0],tagFiles[pathMatch]);
      strcpy(tagSearch[0],tagSearch[pathMatch]);
      tagPosInf[0]=tagPosInf[pathMatch];
      nMatches = 1;
   }

   /*  If all of the tag entries are the same file, just use the first.
    */
   if (GetPrefSmartTags())
   {
      for (i=1; i<nMatches; i++)
         if (strcmp(tagFiles[i],tagFiles[i-1]))
            break;
      if (i==nMatches)
         nMatches = 1;
   }

   if (nMatches>1)
   {
      if (!(dupTagsList = (char**) malloc__(sizeof(char*) * nMatches)))
      {
         fprintf(stderr, "nedit: findAllMatches(): out of heap space!\n");
         fl_beep();
         return -1;
      }

      for (i=0; i<nMatches; i++)
      {
         ParseFilename(tagFiles[i], filename, pathname);
         if ((i<nMatches-1 && !strcmp(tagFiles[i],tagFiles[i+1])) ||
               (i>0 && !strcmp(tagFiles[i],tagFiles[i-1])))
         {
            if (*(tagSearch[i]) && (tagPosInf[i] != -1))  /* etags */
            {
               sprintf(temp,"%2d. %s%s %8i %s", i+1, pathname,
                       filename, tagPosInf[i], tagSearch[i]);
            }
            else if (*(tagSearch[i]))     /* ctags search expr */
            {
               sprintf(temp,"%2d. %s%s          %s", i+1, pathname, filename, tagSearch[i]);
            }
            else     /* line number only */
            {
               sprintf(temp,"%2d. %s%s %8i", i+1, pathname, filename, tagPosInf[i]);
            }
         }
         else
         {
            sprintf(temp,"%2d. %s%s",i+1,pathname,filename);
         }

         if (NULL == (dupTagsList[i] = (char*) malloc__(strlen(temp) + 1)))
         {
            int j;
            fprintf(stderr, "nedit: findAllMatches(): out of heap space!\n");

            /*  dupTagsList[i] is unallocated, let's free__ [i - 1] to [0]  */
            for (j = i - 1; j > -1; j--)
            {
               free__(dupTagsList[j]);
            }
            free__(dupTagsList);

            fl_beep();
            return -1;
         }

         strcpy(dupTagsList[i],temp);
      }
// TODO:       createSelectMenu(dialogParent, "Duplicate Tags", nMatches, dupTagsList);
      for (i=0; i<nMatches; i++)
         free__(dupTagsList[i]);
      free__(dupTagsList);
      return 1;
   }

   /*
   **  No need for a dialog list, there is only one tag matching --
   **  Go directly to the tag
   */
// TODO:    if (searchMode == TAG)
// TODO:       editTaggedLocation(dialogParent, 0);
// TODO:    else
// TODO:       showMatchingCalltip(dialogParent, 0);
   return 1;
}

// TODO: /*      Callback function for the FindAll widget. Process the users response. */
// TODO: static void findAllCB(Widget parent, XtPointer client_data, XtPointer call_data)
// TODO: {
// TODO:    int i;
// TODO:    char* eptr;
// TODO: 
// TODO:    XmSelectionBoxCallbackStruct* cbs =
// TODO:       (XmSelectionBoxCallbackStruct*) call_data;
// TODO:    if (cbs->reason == XmCR_NO_MATCH)
// TODO:       return;
// TODO:    if (cbs->reason == XmCR_CANCEL)
// TODO:    {
// TODO:       XtDestroyWidget(XtParent(parent));
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    XmStringGetLtoR(cbs->value,XmFONTLIST_DEFAULT_TAG,&eptr);
// TODO:    if ((i = atoi(eptr)-1) < 0)
// TODO:    {
// TODO:       XBell(TheDisplay, 0);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    if (searchMode == TAG)
// TODO:       editTaggedLocation(parent, i);   /* Open the file with the definition */
// TODO:    else
// TODO:       showMatchingCalltip(parent, i);
// TODO: 
// TODO:    if (cbs->reason == XmCR_OK)
// TODO:       XtDestroyWidget(XtParent(parent));
// TODO: }
// TODO: 
// TODO: /*      Window manager close-box callback for tag-collision dialog */
// TODO: static void findAllCloseCB(Widget parent, XtPointer client_data,
// TODO:                            XtPointer call_data)
// TODO: {
// TODO:    XtDestroyWidget(parent);
// TODO: }
// TODO: 
// TODO: /*
// TODO:  * Given a \0 terminated string and a position, advance the position
// TODO:  * by n lines, where line separators (for now) are \n.  If the end of
// TODO:  * string is reached before n lines, return the number of lines advanced,
// TODO:  * else normally return -1.
// TODO:  */
// TODO: static int moveAheadNLines(char* str, int* pos, int n)
// TODO: {
// TODO:    int i=n;
// TODO:    while (str[*pos] != '\0' && n>0)
// TODO:    {
// TODO:       if (str[*pos] == '\n')
// TODO:          --n;
// TODO:       ++(*pos);
// TODO:    }
// TODO:    if (n==0)
// TODO:       return -1;
// TODO:    else
// TODO:       return i-n;
// TODO: }
// TODO: 
// TODO: /*
// TODO: ** Show the calltip specified by tagFiles[i], tagSearch[i], tagPosInf[i]
// TODO: ** This reads from either a source code file (if searchMode == TIP_FROM_TAG)
// TODO: ** or a calltips file (if searchMode == TIP).
// TODO: */
// TODO: static void showMatchingCalltip(Widget parent, int i)
// TODO: {
// TODO:    int startPos=0, fileLen, readLen, tipLen;
// TODO:    int endPos=0;
// TODO:    char* fileString;
// TODO:    FILE* fp;
// TODO:    struct stat statbuf;
// TODO:    char* message;
// TODO: 
// TODO:    /* 1. Open the target file */
// TODO:    NormalizePathname(tagFiles[i]);
// TODO:    fp = fopen(tagFiles[i], "r");
// TODO:    if (fp == NULL)
// TODO:    {
// TODO:       DialogF(DF_ERR, parent, 1, "Error opening File", "Error opening %s", "OK", tagFiles[i]);
// TODO:       return;
// TODO:    }
// TODO:    if (fstat(fileno(fp), &statbuf) != 0)
// TODO:    {
// TODO:       fclose(fp);
// TODO:       DialogF(DF_ERR, parent, 1, "Error opening File", "Error opening %s", "OK", tagFiles[i]);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* 2. Read the target file */
// TODO:    /* Allocate space for the whole contents of the file (unfortunately) */
// TODO:    fileLen = statbuf.st_size;
// TODO:    fileString = malloc__(fileLen+1);  /* +1 = space for null */
// TODO:    if (fileString == NULL)
// TODO:    {
// TODO:       fclose(fp);
// TODO:       DialogF(DF_ERR, parent, 1, "File too large", "File is too large to load", "OK");
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* Read the file into fileString and terminate with a null */
// TODO:    readLen = fread(fileString, sizeof(char), fileLen, fp);
// TODO:    if (ferror(fp))
// TODO:    {
// TODO:       fclose(fp);
// TODO:       DialogF(DF_ERR, parent, 1, "Error reading File", "Error reading %s", "OK", tagFiles[i]);
// TODO:       XtFree(fileString);
// TODO:       return;
// TODO:    }
// TODO:    fileString[readLen] = 0;
// TODO: 
// TODO:    /* Close the file */
// TODO:    if (fclose(fp) != 0)
// TODO:    {
// TODO:       /* unlikely error */
// TODO:       DialogF(DF_WARN, parent, 1, "Error closing File",
// TODO:               "Unable to close file", "OK");
// TODO:       /* we read it successfully, so continue */
// TODO:    }
// TODO: 
// TODO:    /* 3. Search for the tagged location (set startPos) */
// TODO:    if (!*(tagSearch[i]))
// TODO:    {
// TODO:       /* It's a line number, just go for it */
// TODO:       if ((moveAheadNLines(fileString, &startPos, tagPosInf[i]-1)) >= 0)
// TODO:       {
// TODO:          DialogF(DF_ERR, parent, 1, "Tags Error", "%s\n not long enough for definition to be on line %d", "OK", tagFiles[i], tagPosInf[i]);
// TODO:          XtFree(fileString);
// TODO:          return;
// TODO:       }
// TODO:    }
// TODO:    else
// TODO:    {
// TODO:       startPos = tagPosInf[i];
// TODO:       if (!fakeRegExSearch(WidgetToWindow(parent), fileString, tagSearch[i],
// TODO:                            &startPos, &endPos))
// TODO:       {
// TODO:          DialogF(DF_WARN, parent, 1, "Tag not found",
// TODO:                  "Definition for %s\nnot found in %s", "OK", tagName,
// TODO:                  tagFiles[i]);
// TODO:          XtFree(fileString);
// TODO:          return;
// TODO:       }
// TODO:    }
// TODO: 
// TODO:    if (searchMode == TIP)
// TODO:    {
// TODO:       int dummy, found;
// TODO: 
// TODO:       /* 4. Find the end of the calltip (delimited by an empty line) */
// TODO:       endPos = startPos;
// TODO:       found = SearchString(fileString, "\\n\\s*\\n", SEARCH_FORWARD,
// TODO:                            SEARCH_REGEX, false, startPos, &endPos, &dummy, NULL,
// TODO:                            NULL, NULL);
// TODO:       if (!found)
// TODO:       {
// TODO:          /* Just take 4 lines */
// TODO:          moveAheadNLines(fileString, &endPos, TIP_DEFAULT_LINES);
// TODO:          --endPos;  /* Lose the last \n */
// TODO:       }
// TODO:    }
// TODO:    else      /* Mode = TIP_FROM_TAG */
// TODO:    {
// TODO:       /* 4. Copy TIP_DEFAULT_LINES lines of text to the calltip string */
// TODO:       endPos = startPos;
// TODO:       moveAheadNLines(fileString, &endPos, TIP_DEFAULT_LINES);
// TODO:       /* Make sure not to overrun the fileString with ". . ." */
// TODO:       if (((size_t) endPos) <= (strlen(fileString)-5))
// TODO:       {
// TODO:          sprintf(&fileString[endPos], ". . .");
// TODO:          endPos += 5;
// TODO:       }
// TODO:    }
// TODO:    /* 5. Copy the calltip to a string */
// TODO:    tipLen = endPos - startPos;
// TODO:    message = malloc__(tipLen+1);  /* +1 = space for null */
// TODO:    if (message == NULL)
// TODO:    {
// TODO:       DialogF(DF_ERR, parent, 1, "Out of Memory", "Can't allocate memory for calltip message", "OK");
// TODO:       XtFree(fileString);
// TODO:       return;
// TODO:    }
// TODO:    strncpy(message, &fileString[startPos], tipLen);
// TODO:    message[tipLen] = 0;
// TODO: 
// TODO:    /* 6. Display it */
// TODO:    tagsShowCalltip(WidgetToWindow(parent), message);
// TODO:    XtFree(message);
// TODO:    XtFree(fileString);
// TODO: }
// TODO: 
// TODO: /*  Open a new (or existing) editor window to the location specified in
// TODO:     tagFiles[i], tagSearch[i], tagPosInf[i] */
// TODO: static void editTaggedLocation(Widget parent, int i)
// TODO: {
// TODO:    /* Globals: tagSearch, tagPosInf, tagFiles, tagName, textNrows,
// TODO:            WindowList */
// TODO:    int startPos, endPos, lineNum, rows;
// TODO:    char filename[MAXPATHLEN], pathname[MAXPATHLEN];
// TODO:    WindowInfo* windowToSearch;
// TODO:    WindowInfo* parentWindow = WidgetToWindow(parent);
// TODO: 
// TODO:    ParseFilename(tagFiles[i],filename,pathname);
// TODO:    /* open the file containing the definition */
// TODO:    EditExistingFile(parentWindow, filename, pathname, 0, NULL, false,
// TODO:                     NULL, GetPrefOpenInTab(), false);
// TODO:    windowToSearch = FindWindowWithFile(filename, pathname);
// TODO:    if (windowToSearch == NULL)
// TODO:    {
// TODO:       DialogF(DF_WARN, parent, 1, "File not found", "File %s not found", "OK",
// TODO:               tagFiles[i]);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    startPos=tagPosInf[i];
// TODO: 
// TODO:    if (!*(tagSearch[i]))
// TODO:    {
// TODO:       /* if the search string is empty, select the numbered line */
// TODO:       SelectNumberedLine(windowToSearch, startPos);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* search for the tags file search string in the newly opened file */
// TODO:    if (!fakeRegExSearch(windowToSearch, NULL, tagSearch[i], &startPos,
// TODO:                         &endPos))
// TODO:    {
// TODO:       DialogF(DF_WARN, windowToSearch->mainWindow, 1, "Tag Error",
// TODO:               "Definition for %s\nnot found in %s", "OK", tagName,
// TODO:               tagFiles[i]);
// TODO:       return;
// TODO:    }
// TODO: 
// TODO:    /* select the matched string */
// TODO:    BufSelect(windowToSearch->buffer, startPos, endPos);
// TODO:    RaiseFocusDocumentWindow(windowToSearch, true);
// TODO: 
// TODO:    /* Position it nicely in the window,
// TODO:       about 1/4 of the way down from the top */
// TODO:    lineNum = BufCountLines(windowToSearch->buffer, 0, startPos);
// TODO:    XtVaGetValues(windowToSearch->lastFocus, textNrows, &rows, NULL);
// TODO:    TextSetScroll(windowToSearch->lastFocus, lineNum - rows/4, 0);
// TODO:    TextSetCursorPos(windowToSearch->lastFocus, endPos);
// TODO: }
// TODO: 
// TODO: /*      Create a Menu for user to select from the collided tags */
// TODO: static Widget createSelectMenu(Widget parent, char* label, int nArgs,
// TODO:                                char* args[])
// TODO: {
// TODO:    int i;
// TODO:    char tmpStr[100];
// TODO:    Widget menu;
// TODO:    XmStringTable list;
// TODO:    NeString popupTitle;
// TODO:    int ac;
// TODO:    Arg csdargs[20];
// TODO: 
// TODO:    list = (XmStringTable) malloc__(nArgs * sizeof(NeString*));
// TODO:    for (i=0; i<nArgs; i++)
// TODO:       list[i] = NeNewString(args[i]);
// TODO:    sprintf(tmpStr,"Select File With TAG: %s",tagName);
// TODO:    popupTitle = NeNewString(tmpStr);
// TODO:    ac = 0;
// TODO:    XtSetArg(csdargs[ac], XmNlistLabelString, popupTitle);
// TODO:    ac++;
// TODO:    XtSetArg(csdargs[ac], XmNlistItems, list);
// TODO:    ac++;
// TODO:    XtSetArg(csdargs[ac], XmNlistItemCount, nArgs);
// TODO:    ac++;
// TODO:    XtSetArg(csdargs[ac], XmNvisibleItemCount, 12);
// TODO:    ac++;
// TODO:    XtSetArg(csdargs[ac], XmNautoUnmanage, false);
// TODO:    ac++;
// TODO:    menu = CreateSelectionDialog(parent,label,csdargs,ac);
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(menu, XmDIALOG_TEXT));
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(menu, XmDIALOG_HELP_BUTTON));
// TODO:    XtUnmanageChild(XmSelectionBoxGetChild(menu, XmDIALOG_SELECTION_LABEL));
// TODO:    XtAddCallback(menu, XmNokCallback, (XtCallbackProc)findAllCB, menu);
// TODO:    XtAddCallback(menu, XmNapplyCallback, (XtCallbackProc)findAllCB, menu);
// TODO:    XtAddCallback(menu, XmNcancelCallback, (XtCallbackProc)findAllCB, menu);
// TODO:    AddMotifCloseCallback(XtParent(menu), findAllCloseCB, NULL);
// TODO:    for (i=0; i<nArgs; i++)
// TODO:       NeStringFree(list[i]);
// TODO:    XtFree((char*)list);
// TODO:    NeStringFree(popupTitle);
// TODO:    ManageDialogCenteredOnPointer(menu);
// TODO:    return menu;
// TODO: }



/*--------------------------------------------------------------------------

   Reference-counted string hack; SJT 4/2000

   This stuff isn't specific to tags, so it should be in it's own file.
   However, I'm leaving it in here for now to reduce the diffs.

   This could really benefit from using a real hash table.
*/

#define RCS_SIZE 10000

struct rcs;

struct rcs_stats
{
   int talloc, tshar, tgiveup, tbytes, tbyteshared;
};

struct rcs
{
   struct rcs* next;
   char*       string;
   int         usage;
};

static struct rcs*       Rcs[RCS_SIZE];
static struct rcs_stats  RcsStats;

/*
** Take a normal string, create a shared string from it if need be,
** and return pointer to that shared string.
**
** Returned strings are const because they are shared.  Do not modify them!
*/

static const char* rcs_strdup(const char* str)
{
   int bucket;
   size_t len;
   struct rcs* rp;
   struct rcs* prev = NULL;

   char* newstr = NULL;

   if (str == NULL)
      return NULL;

   bucket = hashAddr(str) % RCS_SIZE;
   len = strlen(str);

   RcsStats.talloc++;

#if 0
   /* Don't share if it won't save space.

      Doesn't save anything - if we have lots of small-size objects,
      it's beneifical to share them.  We don't know until we make a full
      count.  My tests show that it's better to leave this out.  */
   if (len <= sizeof(struct rcs))
   {
      new_str = strdup(str); /* GET RID OF strdup() IF EVER ENABLED (not ANSI) */
      RcsStats.tgiveup++;
      return;
   }
#endif

   /* Find it in hash */
   for (rp = Rcs[bucket]; rp; rp = rp->next)
   {
      if (!strcmp(str, rp->string))
         break;
      prev = rp;
   }

   if (rp)  /* It exists, return it and bump ref ct */
   {
      rp->usage++;
      newstr = rp->string;

      RcsStats.tshar++;
      RcsStats.tbyteshared += len;
   }
   else     /* Doesn't exist, conjure up a new one. */
   {
      struct rcs* newrcs;
      if (NULL == (newrcs = (struct rcs*) malloc__(sizeof(struct rcs))))
      {
         /*  Not much to fall back to here.  */
         fprintf(stderr, "nedit: rcs_strdup(): out of heap space!\n");
         fl_beep();
         exit(1);
      }

      if (NULL == (newrcs->string = (char*) malloc__(len + 1)))
      {
         /*  Not much to fall back to here.  */
         fprintf(stderr, "nedit: rcs_strdup(): out of heap space!\n");
         fl_beep();
         exit(1);
      }
      strcpy(newrcs->string, str);
      newrcs->usage = 1;
      newrcs->next = NULL;

      if (Rcs[bucket])
         prev->next = newrcs;
      else
         Rcs[bucket] = newrcs;

      newstr = newrcs->string;
   }

   RcsStats.tbytes += len;
   return newstr;
}

/*
** Decrease the reference count on a shared string.  When the reference
** count reaches zero, free__ the master string.
*/
static void rcs_free(const char* rcs_str)
{
   int bucket;
   struct rcs* rp;
   struct rcs* prev = NULL;

   if (rcs_str == NULL)
      return;

   bucket = hashAddr(rcs_str) % RCS_SIZE;

   /* find it in hash */
   for (rp = Rcs[bucket]; rp; rp = rp->next)
   {
      if (rcs_str == rp->string)
         break;
      prev = rp;
   }

   if (rp)  /* It's a shared string, decrease ref count */
   {
      rp->usage--;

      if (rp->usage < 0) /* D'OH! */
      {
         fprintf(stderr, "NEdit: internal error deallocating shared string.");
         return;
      }

      if (rp->usage == 0)  /* Last one- free__ the storage */
      {
         free__(rp->string);
         if (prev)
            prev->next = rp->next;
         else
            Rcs[bucket] = rp->next;
         free__(rp);
      }
   }
   else    /* Doesn't appear to be a shared string */
   {
      fprintf(stderr, "NEdit: attempt to free__ a non-shared string.");
      return;
   }
}

/********************************************************************
 *           Functions for loading Calltips files                   *
 ********************************************************************/

enum tftoken_types { TF_EOF, TF_BLOCK, TF_VERSION, TF_INCLUDE, TF_LANGUAGE, TF_ALIAS, TF_ERROR, TF_ERROR_EOF };

/* A wrapper for SearchString */
static int searchLine(char* line, const char* regex)
{
   int dummy1, dummy2;
   return SearchString(line, regex, SEARCH_FORWARD, SEARCH_REGEX, false, 0, &dummy1, &dummy2, NULL, NULL, NULL);
}

/* Check if a line has non-ws characters */
static bool lineEmpty(const char* line)
{
   while (*line && *line != '\n')
   {
      if (*line != ' ' && *line != '\t')
         return false;
      ++line;
   }
   return true;
}

/* Remove trailing whitespace from a line */
static void rstrip(char* dst, const char* src)
{
   int wsStart, dummy2;
   /* Strip trailing whitespace */
   if (SearchString(src, "\\s*\\n", SEARCH_FORWARD, SEARCH_REGEX, false, 0, &wsStart, &dummy2, NULL, NULL, NULL))
   {
      if (dst != src)
         memcpy(dst, src, wsStart);
      dst[wsStart] = 0;
   }
   else if (dst != src)
      strcpy(dst, src);
}

/*
** Get the next block from a tips file.  A block is a \n\n+ delimited set of
** lines in a calltips file.  All of the parameters except <fp> are return
** values, and most have different roles depending on the type of block
** that is found.
**      header:     Depends on the block type
**      body:       Depends on the block type.  Used to return a new
**                  dynamically allocated string.
**      blkLine:    Returns the line number of the first line of the block
**                  after the "* xxxx *" line.
**      currLine:   Used to keep track of the current line in the file.
*/
static int nextTFBlock(FILE* fp, char* header, char** body, int* blkLine,
                       int* currLine)
{
   /* These are the different kinds of tokens */
   const char* commenTF_regex = "^\\s*\\* comment \\*\\s*$";
   const char* version_regex  = "^\\s*\\* version \\*\\s*$";
   const char* include_regex  = "^\\s*\\* include \\*\\s*$";
   const char* language_regex = "^\\s*\\* language \\*\\s*$";
   const char* alias_regex    = "^\\s*\\* alias \\*\\s*$";
   char line[MAXLINE], *status;
   int dummy1;
   int code;

   /* Skip blank lines and comments */
   while (1)
   {
      /* Skip blank lines */
      while ((status=fgets(line, MAXLINE, fp)))
      {
         ++(*currLine);
         if (!lineEmpty(line))
            break;
      }

      /* Check for error or EOF */
      if (!status)
         return TF_EOF;

      /* We've got a non-blank line -- is it a comment block? */
      if (!searchLine(line, commenTF_regex))
         break;

      /* Skip the comment (non-blank lines) */
      while ((status=fgets(line, MAXLINE, fp)))
      {
         ++(*currLine);
         if (lineEmpty(line))
            break;
      }

      if (!status)
         return TF_EOF;
   }

   /* Now we know it's a meaningful block */
   dummy1 = searchLine(line, include_regex);
   if (dummy1 || searchLine(line, alias_regex))
   {
      /* INCLUDE or ALIAS block */
      int incLen, incPos, i, incLines;

      /* fprintf(stderr, "Starting include/alias at line %i\n", *currLine); */
      if (dummy1)
         code = TF_INCLUDE;
      else
      {
         code = TF_ALIAS;
         /* Need to read the header line for an alias */
         status=fgets(line, MAXLINE, fp);
         ++(*currLine);
         if (!status)
            return TF_ERROR_EOF;
         if (lineEmpty(line))
         {
            fprintf(stderr, "nedit: Warning: empty '* alias *' "
                    "block in calltips file.\n");
            return TF_ERROR;
         }
         rstrip(header, line);
      }
      incPos = ftell(fp);
      *blkLine = *currLine + 1; /* Line of first actual filename/alias */
      if (incPos < 0)
         return TF_ERROR;
      /* Figure out how long the block is */
      while ((status=fgets(line, MAXLINE, fp)) || feof(fp))
      {
         ++(*currLine);
         if (feof(fp) || lineEmpty(line))
            break;
      }
      incLen = ftell(fp) - incPos;
      incLines = *currLine - *blkLine;
      /* Correct currLine for the empty line it read at the end */
      --(*currLine);
      if (incLines == 0)
      {
         fprintf(stderr, "nedit: Warning: empty '* include *' or"
                 " '* alias *' block in calltips file.\n");
         return TF_ERROR;
      }
      /* Make space for the filenames/alias sources */
      *body = (char*)malloc__(incLen+1);
      if (!*body)
         return TF_ERROR;
      *body[0]=0;
      if (fseek(fp, incPos, SEEK_SET) != 0)
      {
         free__(*body);
         return TF_ERROR;
      }
      /* Read all the lines in the block */
      /* fprintf(stderr, "Copying lines\n"); */
      for (i=0; i<incLines; i++)
      {
         status = fgets(line, MAXLINE, fp);
         if (!status)
         {
            free__(*body);
            return TF_ERROR_EOF;
         }
         rstrip(line,line);
         if (i)
            strcat(*body, ":");
         strcat(*body, line);
      }
      /* fprintf(stderr, "Finished include/alias at line %i\n", *currLine); */
   }

   else if (searchLine(line, language_regex))
   {
      /* LANGUAGE block */
      status=fgets(line, MAXLINE, fp);
      ++(*currLine);
      if (!status)
         return TF_ERROR_EOF;
      if (lineEmpty(line))
      {
         fprintf(stderr, "nedit: Warning: empty '* language *' block in calltips file.\n");
         return TF_ERROR;
      }
      *blkLine = *currLine;
      rstrip(header, line);
      code = TF_LANGUAGE;
   }

   else if (searchLine(line, version_regex))
   {
      /* VERSION block */
      status=fgets(line, MAXLINE, fp);
      ++(*currLine);
      if (!status)
         return TF_ERROR_EOF;
      if (lineEmpty(line))
      {
         fprintf(stderr, "nedit: Warning: empty '* version *' block in calltips file.\n");
         return TF_ERROR;
      }
      *blkLine = *currLine;
      rstrip(header, line);
      code = TF_VERSION;
   }

   else
   {
      /* Calltip block */
      /*  The first line is the key, the rest is the tip.
          Strip trailing whitespace. */
      rstrip(header, line);

      status=fgets(line, MAXLINE, fp);
      ++(*currLine);
      if (!status)
         return TF_ERROR_EOF;
      if (lineEmpty(line))
      {
         fprintf(stderr, "nedit: Warning: empty calltip block:\n"
                 "   \"%s\"\n", header);
         return TF_ERROR;
      }
      *blkLine = *currLine;
      *body = strdup(line);
      code = TF_BLOCK;
   }

   /* Skip the rest of the block */
   dummy1 = *currLine;
   while (fgets(line, MAXLINE, fp))
   {
      ++(*currLine);
      if (lineEmpty(line))
         break;
   }

   /* Warn about any unneeded extra lines (which are ignored). */
   if (dummy1+1 < *currLine && code != TF_BLOCK)
   {
      fprintf(stderr, "nedit: Warning: extra lines in language or version block ignored.\n");
   }

   return code;
}

/* A struct for describing a calltip alias */
typedef struct _alias
{
   char* dest;
   char* sources;
   struct _alias* next;
} tf_alias;

/*
** Allocate a new alias, copying dest and stealing sources.  This may
** seem strange but that's the way it's called
*/
static tf_alias* new_alias(const char* dest, char* sources)
{
   tf_alias* alias;

   /* fprintf(stderr, "new_alias: %s <- %s\n", dest, sources); */
   /* Allocate the alias */
   alias = (tf_alias*)malloc__(sizeof(tf_alias));
   if (!alias)
      return NULL;

   /* Fill it in */
   alias->dest = (char*)malloc__(strlen(dest)+1);
   if (!(alias->dest))
      return NULL;
   strcpy(alias->dest, dest);
   alias->sources = sources;
   return alias;
}

/* Deallocate a linked-list of aliases */
static void free_alias_list(tf_alias* alias)
{
   tf_alias* tmp_alias;
   while (alias)
   {
      tmp_alias = alias->next;
      free__(alias->dest);
      free__(alias->sources);
      free__(alias);
      alias = tmp_alias;
   }
}

/*
** Load a calltips file and insert all of the entries into the global tips
** database.  Each tip is essentially stored as its filename and the line
** at which it appears--the exact same way ctags indexes source-code.  That's
** why calltips and tags share so much code.
*/
static int loadTipsFile(const char* tipsFile, int index, int recLevel)
{
   FILE* fp = NULL;
   char header[MAXLINE];
   char* body, *tipIncFile;
   char tipPath[MAXPATHLEN];
   char resolvedTipsFile[MAXPATHLEN+1];
   int nTipsAdded=0, langMode = PLAIN_LANGUAGE_MODE, oldLangMode;
   int currLine=0, code, blkLine;
   tf_alias* aliases=NULL, *tmp_alias;

   if (recLevel > MAX_TAG_INCLUDE_RECURSION_LEVEL)
   {
      fprintf(stderr, "nedit: Warning: Reached recursion limit before loading calltips file:\n\t%s\n", tipsFile);
      return 0;
   }

   /* find the tips file */
   /* Allow ~ in Unix filenames */
   strncpy(tipPath, tipsFile, MAXPATHLEN);    /* ExpandTilde is destructive */
   ExpandTilde(tipPath);
   if (!ResolvePath(tipPath, resolvedTipsFile))
      return 0;

   /* Get the path to the tips file */
   ParseFilename(resolvedTipsFile, NULL, tipPath);

   /* Open the file */
   if ((fp = fopen(resolvedTipsFile, "r")) == NULL)
      return 0;

   while (1)
   {
      code = nextTFBlock(fp, header, &body, &blkLine, &currLine);
      if (code == TF_ERROR_EOF)
      {
         fprintf(stderr,"nedit: Warning: unexpected EOF in calltips file.\n");
         break;
      }
      if (code == TF_EOF)
         break;

      switch (code)
      {
      case TF_BLOCK:
         /* Add the calltip to the global hash table.
             For the moment I'm just using line numbers because I don't
             want to have to deal with adding escape characters for
             regex metacharacters that might appear in the string */
         nTipsAdded += addTag(header, resolvedTipsFile, langMode, "", blkLine, tipPath, index);
         free__(body);
         break;
      case TF_INCLUDE:
         /* nextTFBlock returns a colon-separated list of tips files in body */
         for (tipIncFile=strtok(body,":"); tipIncFile; tipIncFile=strtok(NULL,":"))
         {
            /* fprintf(stderr, "nedit: DEBUG: including tips file '%s'\n", tipIncFile); */
            nTipsAdded += loadTipsFile(tipIncFile, index, recLevel+1);
         }
         free__(body);
         break;
      case TF_LANGUAGE:
         /* Switch to the new language mode if it's valid, else ignore it. */
         oldLangMode = langMode;
         langMode = FindLanguageMode(header);
         if (langMode == PLAIN_LANGUAGE_MODE &&
               strcmp(header, "Plain"))
         {
            fprintf(stderr, "nedit: Error reading calltips file:\n\t%s\n" "Unknown language mode: \"%s\"\n", tipsFile, header);
            langMode = oldLangMode;
         }
         break;
      case TF_ERROR:
         fprintf(stderr,"nedit: Warning: Recoverable error while " "reading calltips file:\n   \"%s\"\n", resolvedTipsFile);
         break;
      case TF_ALIAS:
         /* Allocate a new alias struct */
         tmp_alias = aliases;
         aliases = new_alias(header, body);
         if (!aliases)
         {
            fprintf(stderr,"nedit: Can't allocate memory for tipfile " "alias in calltips file:\n   \"%s\"\n", resolvedTipsFile);
            /* Deallocate any allocated aliases */
            free_alias_list(tmp_alias);
            return 0;
         }
         /* Add it to the list */
         aliases->next = tmp_alias;
         break;
      default:
         ;/* Ignore TF_VERSION for now */
      }
   }

   /* Now resolve any aliases */
   tmp_alias = aliases;
   while (tmp_alias)
   {
      tag* t;
      char* src;
      t = getTag(tmp_alias->dest, TIP);
      if (!t)
      {
         fprintf(stderr, "nedit: Can't find destination of alias \"%s\"\n"
                 "  in calltips file:\n   \"%s\"\n",
                 tmp_alias->dest, resolvedTipsFile);
      }
      else
      {
         for (src=strtok(tmp_alias->sources,":"); src; src=strtok(NULL,":"))
            addTag(src, resolvedTipsFile, t->language, "", t->posInf,
                   tipPath, index);
      }
      tmp_alias = tmp_alias->next;
   }
   free_alias_list(aliases);
   return nTipsAdded;
}
