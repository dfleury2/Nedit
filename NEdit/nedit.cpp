#include "../source/nedit.h"
#include "../source/file.h"
#include "../source/preferences.h"
#include "../source/regularExp.h"
#include "../source/selection.h"
#include "../source/tags.h"
#include "../source/menu.h"
#include "../source/macro.h"
#include "../source/server.h"
#include "../source/interpret.h"
#include "../source/parse.h"
#include "../source/help.h"
#include "../util/misc.h"
#include "../util/printUtils.h"
#include "../util/fileUtils.h"
#include "../util/getfiles.h"
#include "../util/motif.h"
#include "../util/DialogF.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <sys/param.h>
#include <unistd.h>
#include <locale.h>
#endif // WIN32

#ifdef HAVE_DEBUG_H
#include "../debug.h"
#endif

#include <FL/fl_ask.H>
#include <FL/Fl_Window.H>

#include <iostream>

//WindowInfo* WindowList = NULL;
//char* ArgV0 = NULL;
//bool IsServer = false;
//Ne_AppContext AppContext(APP_NAME);

/* Reasons for choice of default font qualifications:

iso8859 appears to be necessary for newer versions of XFree86 that
default to Unicode encoding, which doesn't quite work with Motif.
Otherwise Motif puts up garbage (square blocks).

(This of course, is a stupid default because there are far more iso8859
apps than Unicode apps.  But the X folks insist it's a client bug.  Hah.)

RedHat 7.3 won't default to '-1' for an encoding, if left with a *,
and so reverts to "fixed".  Yech. */
#ifdef linux
#define NEDIT_DEFAULT_FONT      "-*-helvetica-medium-r-normal-*-*-120-*-*-*-*-iso8859-1," \
   "-*-helvetica-bold-r-normal-*-*-120-*-*-*-*-iso8859-1=BOLD," \
   "-*-helvetica-medium-o-normal-*-*-120-*-*-*-*-iso8859-1=ITALIC"

#define NEDIT_FIXED_FONT        "-*-courier-medium-r-normal-*-*-120-*-*-*-*-iso8859-1," \
   "-*-courier-bold-r-normal-*-*-120-*-*-*-*-iso8859-1=BOLD," \
   "-*-courier-medium-o-normal-*-*-120-*-*-*-*-iso8859-1=ITALIC"

#define NEDIT_DEFAULT_BG        "grey70"

#define NEDIT_TEXT_TRANSLATIONS "#override\\n" \
   "Ctrl~Alt~Meta<KeyPress>v: paste-clipboard()\\n" \
   "Ctrl~Alt~Meta<KeyPress>c: copy-clipboard()\\n" \
   "Ctrl~Alt~Meta<KeyPress>x: cut-clipboard()\\n" \
   "Ctrl~Alt~Meta<KeyPress>u: delete-to-start-of-line()\\n"
#elif WIN32
#define NEDIT_DEFAULT_FONT      "Consolas 14"

#define NEDIT_FIXED_FONT        "Fixesys 14"

#define NEDIT_DEFAULT_BG        "grey70"

#define NEDIT_TEXT_TRANSLATIONS "#override\\n" \
   "Ctrl~Alt~Meta<KeyPress>v: paste-clipboard()\\n" \
   "Ctrl~Alt~Meta<KeyPress>c: copy-clipboard()\\n" \
   "Ctrl~Alt~Meta<KeyPress>x: cut-clipboard()\\n" \
   "Ctrl~Alt~Meta<KeyPress>u: delete-to-start-of-line()\\n"

#endif

static const char* fallbackResources[] =
{
   "*buttonFontList: "         NEDIT_DEFAULT_FONT,
   "*labelFontList: "          NEDIT_DEFAULT_FONT,
   "*textFontList: "           NEDIT_FIXED_FONT,
   "*background: "             NEDIT_DEFAULT_BG,
   "*foreground: "             NEDIT_DEFAULT_FG,
   "*XmText.foreground: "      NEDIT_DEFAULT_FG,
   "*XmText.background: "      NEDIT_DEFAULT_TEXT_BG,
   "*XmList.foreground: "      NEDIT_DEFAULT_FG,
   "*XmList.background: "      NEDIT_DEFAULT_TEXT_BG,
   "*XmTextField.foreground: " NEDIT_DEFAULT_FG,
   "*XmTextField.background: " NEDIT_DEFAULT_TEXT_BG,

   /* Use baseTranslations as per Xt Programmer's Manual, 10.2.12 */
   "*XmText.baseTranslations: " NEDIT_TEXT_TRANSLATIONS,
   "*XmTextField.baseTranslations: " NEDIT_TEXT_TRANSLATIONS,

   "*XmLFolder.highlightThickness: 0",
   "*XmLFolder.shadowThickness:    1",
   "*XmLFolder.maxTabWidth:        150",
   "*XmLFolder.traversalOn:        false",
   "*XmLFolder.inactiveForeground: #666" ,
   "*tab.alignment: XmALIGNMENT_BEGINNING",
   "*tab.marginWidth: 0",
   "*tab.marginHeight: 1",

   /* Prevent the file selection box from acting stupid. */
   "*XmFileSelectionBox.resizePolicy: XmRESIZE_NONE",
   "*XmFileSelectionBox.textAccelerators:",
   "*XmFileSelectionBox.pathMode: XmPATH_MODE_RELATIVE",
   "*XmFileSelectionBox.width: 500",
   "*XmFileSelectionBox.height: 400",

   /* NEdit-specific widgets.  Theses things should probably be manually
   jammed into the database, rather than fallbacks.  We really want
   the accelerators to be there even if someone creates an app-defaults
   file against our wishes. */

   "*text.lineNumForeground: " NEDIT_DEFAULT_LINENO_FG,
   "*text.background: " NEDIT_DEFAULT_TEXT_BG,
   "*text.foreground: " NEDIT_DEFAULT_FG,
   "*text.highlightForeground: " NEDIT_DEFAULT_HI_FG,
   "*text.highlightBackground: " NEDIT_DEFAULT_HI_BG,
   "*textFrame.shadowThickness: 1",
   "*menuBar.marginHeight: 0",
   "*menuBar.shadowThickness: 1",
   "*pane.sashHeight: 11",
   "*pane.sashWidth: 11",
   "*pane.marginWidth: 0",
   "*pane.marginHeight: 0",
   "*scrolledW*spacing: 0",
   "*text.selectionArrayCount: 3",
   "*helpText.background: " NEDIT_DEFAULT_HELP_BG,
   "*helpText.foreground: " NEDIT_DEFAULT_HELP_FG,
   "*helpText.selectBackground: " NEDIT_DEFAULT_BG,
   "*statsLine.background: " NEDIT_DEFAULT_BG,
   "*statsLine.FontList: " NEDIT_DEFAULT_FONT,
   "*calltip.background: LemonChiffon1",
   "*calltip.foreground: black",
   "*iSearchForm*highlightThickness: 1",
   "*fileMenu.tearOffModel: XmTEAR_OFF_ENABLED",
   "*editMenu.tearOffModel: XmTEAR_OFF_ENABLED",
   "*searchMenu.tearOffModel: XmTEAR_OFF_ENABLED",
   "*preferencesMenu.tearOffModel: XmTEAR_OFF_ENABLED",
   "*windowsMenu.tearOffModel: XmTEAR_OFF_ENABLED",
   "*shellMenu.tearOffModel: XmTEAR_OFF_ENABLED",
   "*macroMenu.tearOffModel: XmTEAR_OFF_ENABLED",
   "*helpMenu.tearOffModel: XmTEAR_OFF_ENABLED",
   "*fileMenu.mnemonic: F",
   "*fileMenu.new.accelerator: Ctrl<Key>n",
   "*fileMenu.new.acceleratorText: Ctrl+N",
   "*fileMenu.newOpposite.accelerator: Shift Ctrl<Key>n",
   "*fileMenu.newOpposite.acceleratorText: Shift+Ctrl+N",
   "*fileMenu.open.accelerator: Ctrl<Key>o",
   "*fileMenu.open.acceleratorText: Ctrl+O",
   "*fileMenu.openSelected.accelerator: Ctrl<Key>y",
   "*fileMenu.openSelected.acceleratorText: Ctrl+Y",
   "*fileMenu.close.accelerator: Ctrl<Key>w",
   "*fileMenu.close.acceleratorText: Ctrl+W",
   "*fileMenu.save.accelerator: Ctrl<Key>s",
   "*fileMenu.save.acceleratorText: Ctrl+S",
   "*fileMenu.includeFile.accelerator: Alt<Key>i",
   "*fileMenu.includeFile.acceleratorText: Alt+I",
   "*fileMenu.print.accelerator: Ctrl<Key>p",
   "*fileMenu.print.acceleratorText: Ctrl+P",
   "*fileMenu.exit.accelerator: Ctrl<Key>q",
   "*fileMenu.exit.acceleratorText: Ctrl+Q",
   "*editMenu.mnemonic: E",
   "*editMenu.undo.accelerator: Ctrl<Key>z",
   "*editMenu.undo.acceleratorText: Ctrl+Z",
   "*editMenu.redo.accelerator: Shift Ctrl<Key>z",
   "*editMenu.redo.acceleratorText: Shift+Ctrl+Z",
   "*editMenu.cut.accelerator: Ctrl<Key>x",
   "*editMenu.cut.acceleratorText: Ctrl+X",
   "*editMenu.copy.accelerator: Ctrl<Key>c",
   "*editMenu.copy.acceleratorText: Ctrl+C",
   "*editMenu.paste.accelerator: Ctrl<Key>v",
   "*editMenu.paste.acceleratorText: Ctrl+V",
   "*editMenu.pasteColumn.accelerator: Shift Ctrl<Key>v",
   "*editMenu.pasteColumn.acceleratorText: Ctrl+Shift+V",
   "*editMenu.delete.acceleratorText: Del",
   "*editMenu.selectAll.accelerator: Ctrl<Key>a",
   "*editMenu.selectAll.acceleratorText: Ctrl+A",
   "*editMenu.shiftLeft.accelerator: Ctrl<Key>9",
   "*editMenu.shiftLeft.acceleratorText: [Shift]Ctrl+9",
   "*editMenu.shiftLeftShift.accelerator: Shift Ctrl<Key>9",
   "*editMenu.shiftRight.accelerator: Ctrl<Key>0",
   "*editMenu.shiftRight.acceleratorText: [Shift]Ctrl+0",
   "*editMenu.shiftRightShift.accelerator: Shift Ctrl<Key>0",
   "*editMenu.upperCase.accelerator: Ctrl<Key>6",
   "*editMenu.upperCase.acceleratorText: Ctrl+6",
   "*editMenu.lowerCase.accelerator: Shift Ctrl<Key>6",
   "*editMenu.lowerCase.acceleratorText: Shift+Ctrl+6",
   "*editMenu.fillParagraph.accelerator: Ctrl<Key>j",
   "*editMenu.fillParagraph.acceleratorText: Ctrl+J",
   "*editMenu.insertFormFeed.accelerator: Alt Ctrl<Key>l",
   "*editMenu.insertFormFeed.acceleratorText: Alt+Ctrl+L",
   "*editMenu.insertCtrlCode.accelerator: Alt Ctrl<Key>i",
   "*editMenu.insertCtrlCode.acceleratorText: Alt+Ctrl+I",
   "*searchMenu.mnemonic: S",
   "*searchMenu.find.accelerator: Ctrl<Key>f",
   "*searchMenu.find.acceleratorText: [Shift]Ctrl+F",
   "*searchMenu.findShift.accelerator: Shift Ctrl<Key>f",
   "*searchMenu.findAgain.accelerator: Ctrl<Key>g",
   "*searchMenu.findAgain.acceleratorText: [Shift]Ctrl+G",
   "*searchMenu.findAgainShift.accelerator: Shift Ctrl<Key>g",
   "*searchMenu.findSelection.accelerator: Ctrl<Key>h",
   "*searchMenu.findSelection.acceleratorText: [Shift]Ctrl+H",
   "*searchMenu.findSelectionShift.accelerator: Shift Ctrl<Key>h",
   "*searchMenu.findIncremental.accelerator: Ctrl<Key>i",
   "*searchMenu.findIncrementalShift.accelerator: Shift Ctrl<Key>i",
   "*searchMenu.findIncremental.acceleratorText: [Shift]Ctrl+I",
   "*searchMenu.replace.accelerator: Ctrl<Key>r",
   "*searchMenu.replace.acceleratorText: [Shift]Ctrl+R",
   "*searchMenu.replaceShift.accelerator: Shift Ctrl<Key>r",
   "*searchMenu.findReplace.accelerator: Ctrl<Key>r",
   "*searchMenu.findReplace.acceleratorText: [Shift]Ctrl+R",
   "*searchMenu.findReplaceShift.accelerator: Shift Ctrl<Key>r",
   "*searchMenu.replaceFindAgain.accelerator: Ctrl<Key>t",
   "*searchMenu.replaceFindAgain.acceleratorText: [Shift]Ctrl+T",
   "*searchMenu.replaceFindAgainShift.accelerator: Shift Ctrl<Key>t",
   "*searchMenu.replaceAgain.accelerator: Alt<Key>t",
   "*searchMenu.replaceAgain.acceleratorText: [Shift]Alt+T",
   "*searchMenu.replaceAgainShift.accelerator: Shift Alt<Key>t",
   "*searchMenu.gotoLineNumber.accelerator: Ctrl<Key>l",
   "*searchMenu.gotoLineNumber.acceleratorText: Ctrl+L",
   "*searchMenu.gotoSelected.accelerator: Ctrl<Key>e",
   "*searchMenu.gotoSelected.acceleratorText: Ctrl+E",
   "*searchMenu.mark.accelerator: Alt<Key>m",
   "*searchMenu.mark.acceleratorText: Alt+M a-z",
   "*searchMenu.gotoMark.accelerator: Alt<Key>g",
   "*searchMenu.gotoMark.acceleratorText: [Shift]Alt+G a-z",
   "*searchMenu.gotoMarkShift.accelerator: Shift Alt<Key>g",
   "*searchMenu.gotoMatching.accelerator: Ctrl<Key>m",
   "*searchMenu.gotoMatching.acceleratorText: [Shift]Ctrl+M",
   "*searchMenu.gotoMatchingShift.accelerator: Shift Ctrl<Key>m",
   "*searchMenu.findDefinition.accelerator: Ctrl<Key>d",
   "*searchMenu.findDefinition.acceleratorText: Ctrl+D",
   "*searchMenu.showCalltip.accelerator: Ctrl<Key>apostrophe",
   "*searchMenu.showCalltip.acceleratorText: Ctrl+'",
   "*preferencesMenu.mnemonic: P",
   "*preferencesMenu.statisticsLine.accelerator: Alt<Key>a",
   "*preferencesMenu.statisticsLine.acceleratorText: Alt+A",
   "*preferencesMenu.overtype.acceleratorText: Insert",
   "*shellMenu.mnemonic: l",
   "*shellMenu.filterSelection.accelerator: Alt<Key>r",
   "*shellMenu.filterSelection.acceleratorText: Alt+R",
   "*shellMenu.executeCommand.accelerator: Alt<Key>x",
   "*shellMenu.executeCommand.acceleratorText: Alt+X",
   "*shellMenu.executeCommandLine.accelerator: Ctrl<Key>KP_Enter",
   "*shellMenu.executeCommandLine.acceleratorText: Ctrl+KP Enter",
   "*shellMenu.cancelShellCommand.accelerator: Ctrl<Key>period",
   "*shellMenu.cancelShellCommand.acceleratorText: Ctrl+.",
   "*macroMenu.mnemonic: c",
   "*macroMenu.learnKeystrokes.accelerator: Alt<Key>k",
   "*macroMenu.learnKeystrokes.acceleratorText: Alt+K",
   "*macroMenu.finishLearn.accelerator: Alt<Key>k",
   "*macroMenu.finishLearn.acceleratorText: Alt+K",
   "*macroMenu.cancelLearn.accelerator: Ctrl<Key>period",
   "*macroMenu.cancelLearn.acceleratorText: Ctrl+.",
   "*macroMenu.replayKeystrokes.accelerator: Ctrl<Key>k",
   "*macroMenu.replayKeystrokes.acceleratorText: Ctrl+K",
   "*macroMenu.repeat.accelerator: Ctrl<Key>comma",
   "*macroMenu.repeat.acceleratorText: Ctrl+,",
   "*windowsMenu.mnemonic: W",
   "*windowsMenu.splitPane.accelerator: Ctrl<Key>2",
   "*windowsMenu.splitPane.acceleratorText: Ctrl+2",
   "*windowsMenu.closePane.accelerator: Ctrl<Key>1",
   "*windowsMenu.closePane.acceleratorText: Ctrl+1",
   "*helpMenu.mnemonic: H",
   "nedit.help.helpForm.sw.helpText*baseTranslations: #override\
   <Key>Tab:help-focus-buttons()\\n\
   <Key>Return:help-button-action(\"close\")\\n\
   Ctrl<Key>F:help-button-action(\"find\")\\n\
   Ctrl<Key>G:help-button-action(\"findAgain\")\\n\
   <KeyPress>osfCancel:help-button-action(\"close\")\\n\
   ~Meta~Ctrl~Shift<Btn1Down>:\
   grab-focus() help-hyperlink()\\n\
   ~Meta~Ctrl~Shift<Btn1Up>:\
   help-hyperlink(\"current\", \"process-cancel\", \"extend-end\")\\n\
   ~Meta~Ctrl~Shift<Btn2Down>:\
   process-bdrag() help-hyperlink()\\n\
   ~Meta~Ctrl~Shift<Btn2Up>:\
   help-hyperlink(\"new\", \"process-cancel\", \"copy-to\")",
   NULL
};

static const char cmdLineHelp[] =
   "Usage:  nedit [-read] [-create] [-line n | +n] [-server] [-do command]\n\
   [-tags file] [-tabs n] [-wrap] [-nowrap] [-autowrap]\n\
   [-autoindent] [-noautoindent] [-autosave] [-noautosave]\n\
   [-lm languagemode] [-rows n] [-columns n] [-font font]\n\
   [-geometry geometry] [-iconic] [-noiconic] [-svrname name]\n\
   [-display [host]:server[.screen] [-xrm resourcestring]\n\
   [-import file] [-background color] [-foreground color]\n\
   [-tabbed] [-untabbed] [-group] [-V|-version] [-h|-help]\n\
   [--] [file...]\n";

int main(int argc, char** argv)
{
   try
   {
      //Fl::get_system_colors(); // Init foreground, background, and background2 from system colors
      Fl::background(222,222,222);
      //int fountCount = Fl::set_fonts("*");
      // TODO: unsigned char* invalidBindings = NULL;

      // Save the command which was used to invoke nedit for restart command
      ArgV0 = argv[0];

      // Set up default resources if no app-defaults file is found
      for( int i = 0; fallbackResources[i]; ++i)
         AppContext.appDB.set(fallbackResources[i]);

      // Respond to -V or -version even if there is no display
      for (int i = 1; i < argc && strcmp(argv[i], "--"); i++)
      {
         if (0 == strcmp(argv[i], "-V") || 0 == strcmp(argv[i], "-version"))
         {
            PrintVersion();
            exit(EXIT_SUCCESS);
         }
      }

      // Read the preferences file and command line into a database
      Ne_Database prefDB = CreateNEditPrefDB(&argc, argv);

      // Initialize global symbols and subroutines used in the macro language
      InitMacroGlobals();
      RegisterMacroSubroutines();

      // Store preferences from the command line and .nedit file,
      // and set the appropriate preferences
      RestoreNEditPrefs(&prefDB, &AppContext.appDB);

      // More preference stuff
      LoadPrintPreferences(&AppContext.appDB, APP_NAME, true);
      SetDeleteRemap(GetPrefMapDelete());
      SetPointerCenteredDialogs(GetPrefRepositionDialogs());
      SetGetEFTextFieldRemoval(!GetPrefStdOpenDialog());
      // Merge user file preferences to app default preferences
      AppContext.appDB.merge(prefDB);

      // Set up action procedures for menu item commands
      InstallMenuActions(AppContext);

      // Add Actions for following hyperlinks in the help window
      InstallHelpLinkActions(AppContext);
      // Add actions for mouse wheel support in scrolled windows (except text area)
      InstallMouseWheelActions(AppContext);

      // Install word delimiters for regular expression matching
      SetREDefaultWordDelimiters(GetPrefDelimiters());

      // Read the nedit dynamic database of files for the Open Previous
      // command (and eventually other information as well)
      ReadNEditDB();

      // Process -import command line argument before others which might
      // open windows (loading preferences doesn't update menu settings,
      // which would then be out of sync with the real preference settings)
      for (int i=1; i<argc; i++)
      {
         if (!strcmp(argv[i], "--"))
         {
            break; /* treat all remaining arguments as filenames */
         }
         else if (!strcmp(argv[i], "-import"))
         {
            nextArg(argc, argv, &i);
            ImportPrefFile(argv[i], false);
         }
         else if (!strcmp(argv[i], "-importold"))
         {
            nextArg(argc, argv, &i);
            ImportPrefFile(argv[i], true);
         }
      }

      // Load the default tags file. Don't complain if it doesn't load, the tag
      // file resource is intended to be set and forgotten.  Running nedit in a
      // directory without a tags should not cause it to spew out errors.
      if (*GetPrefTagFile() != '\0')
      {
         AddTagsFile(GetPrefTagFile(), TAG);
      }

      if (strcmp(GetPrefServerName(), "") != 0)
      {
         IsServer = true;
      }

      // Process any command line arguments (-tags, -do, -read, -create,
      // +<line_number>, -line, -server, and files to edit) not already
      // processed by RestoreNEditPrefs.
      bool fileSpecified = false;
      bool opts = true;
      char* toDoCommand = 0;
      int editFlags = NE_CREATE;
      int tabbed = 1, group = 0;
      int nRead = 0,  lineNum = 0;
      bool gotoLine = false;
      bool iconic = false;
      char* geometry = 0, *langMode = 0;
      WindowInfo *lastFile = NULL;
      bool macroFileRead = false;

      for (int i=1; i<argc; ++i)
      {
         if (opts && !strcmp(argv[i], "--"))
         {
            opts = false; // treat all remaining arguments as filenames
            continue;
         }
         else if (opts && !strcmp(argv[i], "-tags"))
         {
            nextArg(argc, argv, &i);
            if (!AddTagsFile(argv[i], TAG))
               fprintf(stderr, "NEdit: Unable to load tags file\n");
         }
         else if (opts && !strcmp(argv[i], "-do"))
         {
            nextArg(argc, argv, &i);
            if (checkDoMacroArg(argv[i]))
               toDoCommand = argv[i];
         }
         else if (opts && !strcmp(argv[i], "-read"))
         {
            editFlags |= NE_PREF_READ_ONLY;
         }
         else if (opts && !strcmp(argv[i], "-create"))
         {
            editFlags |= NE_SUPPRESS_CREATE_WARN;
         }
         else if (opts && !strcmp(argv[i], "-tabbed"))
         {
            tabbed = 1;
            group = 0;	// override -group option
         }
         else if (opts && !strcmp(argv[i], "-untabbed"))
         {
            tabbed = 0;
            group = 0;	// override -group option
         }
         else if (opts && !strcmp(argv[i], "-group"))
         {
            group = 2; // 2: start new group, 1: in group
         }
         else if (opts && !strcmp(argv[i], "-line"))
         {
            nextArg(argc, argv, &i);
            nRead = sscanf(argv[i], "%d", &lineNum);
            if (nRead != 1)
               fprintf(stderr, "NEdit: argument to line should be a number\n");
            else
               gotoLine = true;
         }
         else if (opts && (*argv[i] == '+'))
         {
            nRead = sscanf((argv[i]+1), "%d", &lineNum);
            if (nRead != 1)
               fprintf(stderr, "NEdit: argument to + should be a number\n");
            else
               gotoLine = true;
         }
         else if (opts && !strcmp(argv[i], "-server"))
         {
            IsServer = true;
         }
         else if (opts && (!strcmp(argv[i], "-iconic") || !strcmp(argv[i], "-icon")))
         {
            iconic = true;
         }
         else if (opts && !strcmp(argv[i], "-noiconic"))
         {
            iconic = false;
         }
         else if (opts && (!strcmp(argv[i], "-geometry") || !strcmp(argv[i], "-g")))
         {
            nextArg(argc, argv, &i);
            geometry = argv[i];
         }
         else if (opts && !strcmp(argv[i], "-lm"))
         {
            nextArg(argc, argv, &i);
            langMode = argv[i];
         }
         else if (opts && !strcmp(argv[i], "-import"))
         {
            nextArg(argc, argv, &i); // already processed, skip
         }
         else if (opts && (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help")))
         {
            fprintf(stderr, "%s", cmdLineHelp);
            exit(EXIT_SUCCESS);
         }
         else if (opts && (*argv[i] == '-'))
         {
            fprintf(stderr, "nedit: Unrecognized option %s\n%s", argv[i], cmdLineHelp);
            exit(EXIT_FAILURE);
         }
         else
         {
            char filename[MAXPATHLEN] = "", pathname[MAXPATHLEN] = "";

            if (ParseFilename(argv[i], filename, pathname) == 0)
            {
               int isTabbed = 0;

               // determine if file is to be openned in new tab, by
               // factoring the options -group, -tabbed & -untabbed
               if (group == 2)
               {
                  isTabbed = 0;  // start a new window for new group
                  group = 1;     // next file will be within group
               }
               else if (group == 1)
               {  
                  isTabbed = 1;  // new tab for file in group
               }
               else              // not in group
               {  
                  isTabbed = tabbed==-1? GetPrefOpenInTab() : tabbed;
               }

               // Files are opened in background to improve opening speed
               // by defering certain time  consuiming task such as syntax
               //  highlighting. At the end of the file-opening loop, the
               // last file opened will be raised to restore those deferred
               // items. The current file may also be raised if there're
               // macros to execute on.
               WindowInfo* window = EditExistingFile(WindowList, filename, pathname, editFlags, geometry, iconic, langMode, isTabbed, true);
               fileSpecified = true;
               if (window)
               {
                  // raise the last tab of previous window
                  if (lastFile && window->mainWindow != lastFile->mainWindow)
                  {
                     RaiseDocument(lastFile);
                  }

                  if (!macroFileRead)
                  {
                     ReadMacroInitFile(WindowList);
                     macroFileRead = true;
                  }
                  if (gotoLine)
                     SelectNumberedLine(window, lineNum);
                  if (toDoCommand != NULL)
                  {
                     DoMacro(window, toDoCommand, "-do macro");
                     toDoCommand = NULL;
                     if (!IsValidWindow(window))
                        window = NULL; // window closed by macro
                     if (lastFile && !IsValidWindow(lastFile))
                        lastFile = NULL; // window closed by macro
                  }
               }

               // register last opened file for later use
               if (window)
                  lastFile = window;
            }
            else
            {
               fprintf(stderr, "nedit: file name too long: %s\n", argv[i]);
            }
         }
      }

      // Raise the last file opened
      if (lastFile)
      {
         RaiseDocument(lastFile);
      }
      CheckCloseDim();

      // If no file to edit was specified, open a window to edit "Untitled"
      if (!fileSpecified)
      {
         EditNewFile(NULL, geometry, iconic, langMode, NULL);
         ReadMacroInitFile(WindowList);
         CheckCloseDim();
         if (toDoCommand != NULL)
            DoMacro(WindowList, toDoCommand, "-do macro");
      }

      // Begin remembering last command invoked for "Repeat" menu item
      AddLastCommandActionHook(AppContext);

      // Set up communication port and write ~/.nedit_server_process file
      if (IsServer)
         InitServerCommunication();

      // Process events.
      if (IsServer)
         ServerMainLoop(AppContext);
      else
         Fl::run();

      return 0;
   }
   catch (const char* msg)
   {
      std::cerr << "NEdit++ fatal error : " << msg << std::endl;
   }
   catch (const std::string& msg)
   {
      std::cerr << "NEdit++ fatal error : " << msg << std::endl;
   }
   catch (const std::exception& e)
   {
      std::cerr << "NEdit++ fatal error : " << e.what() << std::endl;
   }
   catch (...)
   {
      std::cerr << "NEdit++ unknown fatal error" << std::endl;
   }
}
