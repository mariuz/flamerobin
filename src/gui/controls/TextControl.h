/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Michael Hieke.

  Portions created by the original developer
  are Copyright (C) 2005 Michael Hieke.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/

//-----------------------------------------------------------------------------
#ifndef TEXTCONTROL_H
#define TEXTCONTROL_H

#include <wx/wx.h>
#include <wx/stc/stc.h>
//-----------------------------------------------------------------------------
// Base class for multiline text controls in FlameRobin.  Based on 
// wxStyledTextCtrl instead of wxTextCtrl, because that works better/faster 
// on some systems, and provides popup menu on wxGTK.  
class TextControl: public wxStyledTextCtrl {
protected:
    void resetStyles();

public:
    TextControl(wxWindow *parent, wxWindowID id = wxID_ANY, 
        long style = wxSUNKEN_BORDER);

	void OnCommandUndo(wxCommandEvent& event);
	void OnCommandRedo(wxCommandEvent& event);
	void OnCommandCut(wxCommandEvent& event);
	void OnCommandCopy(wxCommandEvent& event);
	void OnCommandPaste(wxCommandEvent& event);
	void OnCommandDelete(wxCommandEvent& event);
	void OnCommandSelectAll(wxCommandEvent& event);
	void OnContextMenu(wxContextMenuEvent& event);
	void OnStartDrag(wxStyledTextEvent& event);
    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif
