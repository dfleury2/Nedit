#include "fontsel.h"

#include "Ne_Font.h"
#include "Ne_FontDescription.h"
#include "misc.h"
#include "DialogF.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Check_Button.H>

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <set>

enum ListSpecifier { LS_None, LS_Font, Style, Size };

// local data structures and types
struct xfselControlBlkType
{
   Fl_Window* form;
   Fl_Button* okButton;
   Fl_Button* cancelButton;
   Fl_Browser* fontList;
   Fl_Browser* styleList;
   Fl_Browser* sizeList;
   Fl_Input* fontNameField;
   Fl_Button* propFontToggle;
   Fl_Output* dispField;
   Ne_FontDescriptions*        fontData;         /* font name info  */
   std::string sel1; // selection from list 1
   std::string sel2; // selection from list 2
   std::string sel3; // selection from list 3
   int             showPropFonts;      /* toggle state - show prop fonts */
   std::string fontName;          /* current font name */
   Ne_Font oldFont;           /* font data structure for dispSample */

   Fl_Color   sampleFG;                /* Colors for the sample field */
   Fl_Color   sampleBG;
};


/* local function prototypes */

static void setupScrollLists(ListSpecifier dontChange, const xfselControlBlkType& ctrlBlk);
static bool notPropFont(const Ne_FontDescription& font);
static bool styleMatch(const xfselControlBlkType& ctrlBlk, const Ne_FontDescription& font);
static bool sizeMatch(const xfselControlBlkType& ctrlBlk, const Ne_FontDescription& font);
static bool fontMatch(const xfselControlBlkType& ctrlBlk, const Ne_FontDescription& font);
static std::string getFamilyPart(const Ne_FontDescription& font);
static std::string getStylePart(const Ne_FontDescription& font);
static int getSizePart(const Ne_FontDescription& font);
static void propFontToggleAction(Fl_Widget* widget, void*);
static void fontListCB(Fl_Widget* widget, void* data);
static void styleListCB(Fl_Widget* widget, void* data);
static void sizeListCB(Fl_Widget* widget, void* data);
static void choiceMade(xfselControlBlkType& ctrlBlk);
static void dispSample(xfselControlBlkType& ctrlBlk);
static void cancelCB(Fl_Widget* widget, void* data);
static void okCB(Fl_Widget* widget, void* data);
static void startupFont(xfselControlBlkType& ctrlBlk, const char* font);
static void enableSample(xfselControlBlkType* ctrlBlk, bool turn_on, const Ne_Font* font);

/*******************************************************************************
*     FontSel ()                                                               *
*            Function to put up a modal font selection dialog box. The purpose *
*            of this routine is to allow the user to interactively view sample *
*            fonts and to choose a font for current use.                       *
*                                                                              *
*     Arguments:                                                               *
*                                                                              *
*            Fl_Widget*     parent          - parent widget ID                 *
*                                                                              *
*            int        showPropFont    - ONLY_FIXED : shows only fixed fonts  *
*                                                      doesn't show prop font  *
*                                                      toggle button also.     *
*                                         PREF_FIXED : can select either fixed *
*                                                      or proportional fonts;  *
*                                                      but starting option is  *
*                                                      Fixed fonts.            *
*                                         PREF_PROP  : can select either fixed *
*                                                      or proportional fonts;  *
*                                                      but starting option is  *
*                                                      proportional fonts.     *
*                                                                              *
*           char *      currFont        - ASCII string that contains the name  *
*                                         of the currently selected font.      *
*                                                                              *
*           Fl_Color   sampleFG, sampleBG      - Foreground/Background colors  *
*                                               in which to display the sample *
*                                               text.                          *
*     Returns:                                                                 *
*           pointer to an ASCII character string that contains the name of     *
*           the selected font                                                  *
*******************************************************************************/
std::string FontSel(Fl_Widget* parent, int showPropFonts, const char* currFont, Fl_Color sampleFG, Fl_Color sampleBG)
{
   xfselControlBlkType ctrlBlk;
   ctrlBlk.fontData = &Fonts;
   ctrlBlk.sampleFG = sampleFG;
   ctrlBlk.sampleBG = sampleBG;

   // Set up window sizes for form widget
   Fl_Double_Window dialog(30,50, 400, 380, "Font Selector");

   //  Create pushbutton widgets
   Fl_Group buttonLine(0, 330, 400, 50);
   buttonLine.box(FL_ENGRAVED_FRAME);
   
   Fl_Button btnOk(60, 340, 80, 25, "Ok");
   btnOk.shortcut(FL_Enter);
   btnOk.callback(okCB, &ctrlBlk);

   Fl_Button btnCancel(260, 340, 80, 25, "Cancel");
   btnCancel.shortcut(FL_Escape);
   btnCancel.callback(cancelCB, &ctrlBlk);

   buttonLine.end();

   dialog.focus(&btnOk);

   // create font name text widget and the corresponding label
   Fl_Input fontName(5, 295, 390, 25, "Font &Name");
   fontName.align(FL_ALIGN_TOP_LEFT);
   fontName.value(currFont);

   // create sample display text field widget
   Fl_Output dispField(5, 250, 390, 25, "&Sample");
   dispField.align(FL_ALIGN_TOP_LEFT);
   dispField.value("ABCDEFGHIJTUVWXYZ abcdefghijklmnwxyz 0123456789");
   dispField.color(sampleBG);
   dispField.textcolor(sampleFG);

   // create toggle button
   Fl_Check_Button* propFontToggle = 0;
   if (showPropFonts != ONLY_FIXED)
   {
      propFontToggle = new Fl_Check_Button(180, 220, 215, 25, "Show Proportional &Width Fonts");
      propFontToggle->callback(propFontToggleAction, &ctrlBlk);
   }

   // create scroll list widgets
   // "Font" list
   Fl_Hold_Browser fontList(5, 25, 200, 190, "Font:");
   fontList.align(FL_ALIGN_TOP_LEFT);
   fontList.callback(fontListCB, &ctrlBlk);

   // "Style" list
   Fl_Hold_Browser styleList(210, 25, 110, 190, "Style");
   styleList.align(FL_ALIGN_TOP_LEFT);
   styleList.callback(styleListCB, &ctrlBlk);

   // "Size" list
   Fl_Hold_Browser sizeList(325, 25, 70, 190, "Size");
   sizeList.align(FL_ALIGN_TOP_LEFT);
   sizeList.callback(sizeListCB, &ctrlBlk);

   // update application's control block structure
   ctrlBlk.form            = &dialog;
   ctrlBlk.okButton        = &btnOk;
   ctrlBlk.cancelButton    = &btnCancel;
   ctrlBlk.fontList        = &fontList;
   ctrlBlk.styleList       = &styleList;
   ctrlBlk.sizeList        = &sizeList;
   ctrlBlk.fontNameField   = &fontName;
   if (showPropFonts != ONLY_FIXED)
      ctrlBlk.propFontToggle  = propFontToggle;
   ctrlBlk.dispField       = &dispField;
   ctrlBlk.showPropFonts   = showPropFonts;
   ctrlBlk.sel1            = "";
   ctrlBlk.sel2            = "";
   ctrlBlk.sel3            = "";
   ctrlBlk.fontName        = "";

   // update scroll lists
   setupScrollLists(LS_None, ctrlBlk);    

   if (showPropFonts == PREF_PROP)
      NeToggleButtonSetState(propFontToggle, true, false);

   // Realize Widgets
   ManageDialogCenteredOnPointer(&dialog);

   // set up current font parameters
   if (currFont && currFont[0] != '\0')
      startupFont(ctrlBlk, currFont);

   // enter event loop
   dialog.resizable(&fontList);
   dialog.set_modal();
   dialog.show();
   while (dialog.shown()) Fl::wait();

   return ctrlBlk.fontName;
}

// --------------------------------------------------------------------------
// parse through the fontlist data and set up the three scroll lists
// --------------------------------------------------------------------------
static void setupScrollLists(ListSpecifier dontChange, const xfselControlBlkType& ctrlBlk)
{
   std::set<std::string> itemBuf1;
   std::set<std::string> itemBuf2;
   std::set<int> itemBuf3;
   bool fullSizeInserted = false;

   for (Ne_FontDescriptions::const_iterator font = ctrlBlk.fontData->begin(); font != ctrlBlk.fontData->end(); ++font)
   {
      if ((dontChange != LS_Font) &&
         (styleMatch(ctrlBlk, *font) && (sizeMatch(ctrlBlk, *font)) &&
            ((ctrlBlk.showPropFonts == PREF_PROP) || (notPropFont(*font)))))
      {
         std::string fontPart = getFamilyPart(*font);
         itemBuf1.insert(fontPart);
      }

      if ((dontChange != Style) &&
            (fontMatch(ctrlBlk, *font)) &&
            (sizeMatch(ctrlBlk, *font)) &&
            ((ctrlBlk.showPropFonts == PREF_PROP) || (notPropFont(*font))))
      {
         std::string stylePart = getStylePart(*font);
         itemBuf2.insert(stylePart);
      }

      if ((dontChange != Size) &&
            (fontMatch(ctrlBlk, *font)) &&
            (styleMatch(ctrlBlk, *font)) &&
            ((ctrlBlk.showPropFonts == PREF_PROP) || (notPropFont(*font))))
      {
         int sizePart = getSizePart(*font);
         if (sizePart == 0 && !fullSizeInserted)
         {
            fullSizeInserted = true;
            for (int i = 1; i <65; ++i )
               itemBuf3.insert(i);
         }
         else
            itemBuf3.insert(sizePart);
      }
   }

   //  recreate all three scroll lists where necessary
   if (dontChange != LS_Font)
   {
      ctrlBlk.fontList->clear();
      for( std::set<std::string>::const_iterator l = itemBuf1.begin(); l != itemBuf1.end(); ++l)
      {
         ctrlBlk.fontList->add(l->c_str());
         if (!ctrlBlk.sel1.empty() && (*l) == ctrlBlk.sel1)
            ctrlBlk.fontList->select(ctrlBlk.fontList->size());
      }
   }

   if (dontChange != Style)
   {
      ctrlBlk.styleList->clear();
      for( std::set<std::string>::const_iterator l = itemBuf2.begin(); l != itemBuf2.end(); ++l)
      {
         ctrlBlk.styleList->add(l->c_str());
         if (!ctrlBlk.sel2.empty() && (*l) == ctrlBlk.sel2)
            ctrlBlk.styleList->select(ctrlBlk.styleList->size());
      }
   }

   if (dontChange != Size)
   {
      ctrlBlk.sizeList->clear();
      for( std::set<int>::const_iterator l = itemBuf3.begin(); l != itemBuf3.end(); ++l)
      {
         char buf[32];
         sprintf(buf, "%d", *l);
         ctrlBlk.sizeList->add(buf);
         if (!ctrlBlk.sel3.empty() && buf == ctrlBlk.sel3)
            ctrlBlk.sizeList->select(ctrlBlk.sizeList->size());
      }
   }
}

// --------------------------------------------------------------------------
//  returns TRUE if argument is not name of a proportional font
// --------------------------------------------------------------------------
static bool notPropFont(const Ne_FontDescription& font)
{
   return font.isFixed();
}

// --------------------------------------------------------------------------
// returns TRUE if the style portion of the font matches the currently selected style
// --------------------------------------------------------------------------
static bool styleMatch(const xfselControlBlkType& ctrlBlk, const Ne_FontDescription& font)
{
   if (ctrlBlk.sel2.empty())
      return true;

   std::string stylePart = getStylePart(font);

   return (stylePart == ctrlBlk.sel2);
}

// --------------------------------------------------------------------------
// returns TRUE if the size portion of the font matches the currently selected size
// --------------------------------------------------------------------------
static bool sizeMatch(const xfselControlBlkType& ctrlBlk, const Ne_FontDescription& font)
{
   if (ctrlBlk.sel3.empty())
      return true;

   int sizePart = getSizePart(font);
   return (sizePart == 0 || sizePart == atoi(ctrlBlk.sel3.c_str()));
}

// --------------------------------------------------------------------------
// returns TRUE if the font portion of the font matches the currently selected font
// --------------------------------------------------------------------------
static bool fontMatch(const xfselControlBlkType& ctrlBlk, const Ne_FontDescription& font)
{
   if (ctrlBlk.sel1.empty())
      return true;

   std::string fontPart = getFamilyPart(font);
   return (fontPart == ctrlBlk.sel1);
}

// --------------------------------------------------------------------------
// given a font name this function returns the part used in the first scroll list
// --------------------------------------------------------------------------
static std::string getFamilyPart(const Ne_FontDescription& font)
{
   std::string name = font.name;
   if (name[0] == '@') name = "@" + font.name;

   std::string::size_type found = name.find("bold");
   if (found != std::string::npos)
      return name.substr(0, found-1);

   found = name.find("italic");
   if (found != std::string::npos)
      return name.substr(0, found-1);
   
   return name;
}

// --------------------------------------------------------------------------
//  given a font name this function returns the part used in the second croll list
// --------------------------------------------------------------------------
static std::string getStylePart(const Ne_FontDescription& font)
{
   if (font.name.find("bold italic") != std::string::npos) return "bold italic";
   if (font.name.find("italic") != std::string::npos) return "italic";
   if (font.name.find("bold") != std::string::npos) return "bold";
   return "normal";
}

// --------------------------------------------------------------------------
// given a font name this function returns the part used in the third scroll list
// --------------------------------------------------------------------------
static int getSizePart(const Ne_FontDescription& font)
{
   return font.size;
}

// --------------------------------------------------------------------------
// Call back functions start from here - suffix Action in the function name
// is for the callback function for the corresponding widget
// --------------------------------------------------------------------------
static void propFontToggleAction(Fl_Widget* widget, void* data)
{
   xfselControlBlkType* ctrlBlk = (xfselControlBlkType*)data;

   if (ctrlBlk->showPropFonts == PREF_FIXED)
      ctrlBlk->showPropFonts = PREF_PROP;
   else
      ctrlBlk->showPropFonts = PREF_FIXED;

   ctrlBlk->sel1 = "";
   ctrlBlk->sel2 = "";
   ctrlBlk->sel3 = "";

   setupScrollLists(LS_None, *ctrlBlk);

   NeTextSetString(ctrlBlk->fontNameField, "");
   enableSample(ctrlBlk, false, NULL);
}

// --------------------------------------------------------------------------
static void enableSample(xfselControlBlkType* ctrlBlk, bool turn_on, const Ne_Font* font)
{
   if (turn_on)
   {
      if (font)
      {
         ctrlBlk->dispField->textfont(font->font);
         ctrlBlk->dispField->textsize(font->size);
      }
      else throw "Font sel internal error";

      ctrlBlk->dispField->textcolor(ctrlBlk->sampleFG);
   }
   else
   {
      ctrlBlk->dispField->textcolor(ctrlBlk->sampleBG);
   }

   // Make sure the sample area gets resized if the font size changes
   ctrlBlk->dispField->redraw();
}

// --------------------------------------------------------------------------
static void fontListCB(Fl_Widget* widget, void* data)
{
   xfselControlBlkType* ctrlBlk = (xfselControlBlkType*)data;
   int index = ctrlBlk->fontList->value();
   if (index == 0) return;

   std::string sel = NeNewString(ctrlBlk->fontList->text(index));

   if (ctrlBlk->sel1.empty())
   {
      ctrlBlk->sel1 = sel;
   }
   else
   {
      if (ctrlBlk->sel1 == sel)
      {
         // Unselecting current selection
         ctrlBlk->sel1.clear();
         ctrlBlk->fontList->deselect();
      }
      else
      {
         ctrlBlk->sel1 = sel;
      }
   }

   setupScrollLists(LS_Font, *ctrlBlk);
   if ((ctrlBlk->sel1 != "") && (ctrlBlk->sel2 != "") && (ctrlBlk->sel3 != ""))
      choiceMade(*ctrlBlk);
   else
   {
      enableSample(ctrlBlk, false, NULL);
      NeTextSetString(ctrlBlk->fontNameField, "");
   }
}

// --------------------------------------------------------------------------
static void styleListCB(Fl_Widget* widget, void* data)
{
   xfselControlBlkType* ctrlBlk = (xfselControlBlkType*)data;
   int index = ctrlBlk->styleList->value();
   if (index == 0) return;

   std::string sel = ctrlBlk->styleList->text(index);

   if (ctrlBlk->sel2.empty())
   {
      ctrlBlk->sel2 = sel;
   }
   else
   {
      if (ctrlBlk->sel2 == sel)
      {
         // unselecting current selection
         ctrlBlk->sel2 = "";
         ctrlBlk->styleList->deselect();
      }
      else
      {
         ctrlBlk->sel2 = sel;
      }
   }

   setupScrollLists(Style, *ctrlBlk);
   if ((ctrlBlk->sel1 != "") && (ctrlBlk->sel2 != "") && (ctrlBlk->sel3 != ""))
      choiceMade(*ctrlBlk);
   else
   {
      enableSample(ctrlBlk, false, NULL);
      NeTextSetString(ctrlBlk->fontNameField, "");
   }
}

// --------------------------------------------------------------------------
static void sizeListCB(Fl_Widget* widget, void* data)
{
   xfselControlBlkType* ctrlBlk = (xfselControlBlkType*)data;
   int index = ctrlBlk->sizeList->value();
   if (index == 0) return;

   std::string sel = ctrlBlk->sizeList->text(index);

   if (ctrlBlk->sel3.empty())
   {
      ctrlBlk->sel3 = sel;
   }
   else
   {
      if (ctrlBlk->sel3 == sel)
      {
         // unselecting current selection
         ctrlBlk->sel3 = "";
         ctrlBlk->sizeList->deselect();
      }
      else
      {
         ctrlBlk->sel3 = sel;
      }
   }

   setupScrollLists(Size, *ctrlBlk);
   if (!ctrlBlk->sel1.empty() && (ctrlBlk->sel2 != "") && (ctrlBlk->sel3 != ""))
      choiceMade(*ctrlBlk);
   else
   {
      enableSample(ctrlBlk, false, NULL);
      NeTextSetString(ctrlBlk->fontNameField, "");
   }
}

// function called when all three choices have been made; sets up font name and displays sample font
static void choiceMade(xfselControlBlkType& ctrlBlk)
{
   ctrlBlk.fontName = "";

   for (Ne_FontDescriptions::const_iterator font = ctrlBlk.fontData->begin(); font != ctrlBlk.fontData->end(); ++font)
   {
      if (fontMatch(ctrlBlk, *font) && styleMatch(ctrlBlk, *font) && sizeMatch(ctrlBlk, *font))
      {
         ctrlBlk.fontName = font->name + " " + ctrlBlk.sel3; /* avoid converting size to string */
         break;
      }
   }

   if (ctrlBlk.fontName != "")
   {
      NeTextSetString(ctrlBlk.fontNameField, ctrlBlk.fontName.c_str());
      dispSample(ctrlBlk);
   }
   else
   {
      DialogF(DF_ERR, ctrlBlk.form, 1, "Font Specification", "Invalid Font Specification", "OK");
   }
}

// --------------------------------------------------------------------------
// loads selected font and displays sample text in that font
// --------------------------------------------------------------------------
static void dispSample(xfselControlBlkType& ctrlBlk)
{
   Ne_Font font(ctrlBlk.fontName);
   enableSample(&ctrlBlk, true, &font);
}

// --------------------------------------------------------------------------
static void cancelCB(Fl_Widget* w, void* data)
{
   xfselControlBlkType* ctrlBlk = (xfselControlBlkType*)data;

   WidgetToMainWindow(w)->hide();
}

// --------------------------------------------------------------------------
static void okCB(Fl_Widget* w, void* data)
{
   xfselControlBlkType* ctrlBlk = (xfselControlBlkType*)data;

   std::string fontPattern = ctrlBlk->fontNameField->value();
   Ne_Font font;
   if (!font.init(fontPattern))
   {
      DialogF(DF_ERR, ctrlBlk->okButton, 1, "Font Specification", "Invalid Font Specification", "OK");
   }
   else
      WidgetToMainWindow(w)->hide();
}

// --------------------------------------------------------------------------
// if current font is passed as an argument then this function is
// invoked and sets up initial entries
// --------------------------------------------------------------------------
static void startupFont(xfselControlBlkType& ctrlBlk, const char* str)
{
   Ne_Font font;
   if (!font.init(str))
      return; // Invalid Font at startup
   
   ctrlBlk.fontName = str;

   dispSample(ctrlBlk);
   NeTextSetString(ctrlBlk.fontNameField, ctrlBlk.fontName.c_str());
}
