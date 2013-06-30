#ifndef NEDIT_TEXTSEL_H_INCLUDED
#define NEDIT_TEXTSEL_H_INCLUDED

class Ne_Text_Editor;

// TODO: void HandleXSelections(Ne_Text_Editor* textD);
// TODO: void StopHandlingXSelections(Ne_Text_Editor* textD);
void CopyToClipboard(Ne_Text_Editor* textD, double time);
void InsertPrimarySelection(Ne_Text_Editor* textD, double time, bool isColumnar);
void MovePrimarySelection(Ne_Text_Editor* textD, double time, bool isColumnar);
void SendSecondarySelection(Ne_Text_Editor* textD, double time, int removeAfter);
void ExchangeSelections(Ne_Text_Editor* textD, double time);
void InsertClipboard(Ne_Text_Editor* textD, bool isColumnar);
void TakeMotifDestination(Ne_Text_Editor* textD, double time);

#endif /* NEDIT_TEXTSEL_H_INCLUDED */
