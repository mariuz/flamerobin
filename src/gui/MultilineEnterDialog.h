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

The Initial Developer of the Original Code is Milan Babuskov.

Portions created by the original developer
are Copyright (C) 2004 Milan Babuskov.

All Rights Reserved.

$Id$

Contributor(s): Michael Hieke, Nando Dessena
*/

#ifndef MULTILINEENTERDIALOG_H
#define MULTILINEENTERDIALOG_H

#include <wx/wx.h>
#include <wx/stc/stc.h>
#include "BaseDialog.h"

//-----------------------------------------------------------------------------
bool GetMultilineTextFromUser(const wxString& caption, wxString& value, wxWindow* parent=0);
//-----------------------------------------------------------------------------
class TextCtrlWithContextMenu: public wxStyledTextCtrl
{
private:
	enum { myId = 500 };
public:
	TextCtrlWithContextMenu(wxWindow* parent, const wxString& initialText = wxEmptyString);

	// standard context menu for edit controls. Needed since wxTextCtrl on GTK1 does not provide it
	enum { 
		ID_MENU_UNDO = 300, ID_MENU_REDO, ID_MENU_CUT, ID_MENU_COPY, ID_MENU_PASTE, ID_MENU_DELETE, ID_MENU_SELECT_ALL
	};
	void OnStartDrag(wxStyledTextEvent& event);
	void OnContextMenu(wxContextMenuEvent& event);
	void OnMenuUndo(wxCommandEvent& event);
	void OnMenuRedo(wxCommandEvent& event);
	void OnMenuCut(wxCommandEvent& event);
	void OnMenuCopy(wxCommandEvent& event);
	void OnMenuPaste(wxCommandEvent& event);
	void OnMenuDelete(wxCommandEvent& event);
	void OnMenuSelectAll(wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
//! normally you shouldn't need to create objects of this class, just use the GetMultilineTextFromUser function
class MultilineEnterDialog: public BaseDialog {
public:
    enum {
        ID_button_ok = wxID_OK,
        ID_button_cancel = wxID_CANCEL
    };
	
    wxString getText() const;
    MultilineEnterDialog(wxWindow* parent, const wxString& title, const wxString& initialText);

private:
    void do_layout();
    void set_properties();

protected:
    TextCtrlWithContextMenu* text_ctrl_value;
    wxButton* button_ok;
    wxButton* button_cancel;
    virtual const std::string getName() const;
};
//-----------------------------------------------------------------------------
#endif // MULTILINEENTERDIALOG_H
