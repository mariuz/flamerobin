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
#include "BaseDialog.h"

//-----------------------------------------------------------------------------
bool GetMultilineTextFromUser(const wxString& caption, wxString& value, wxWindow* parent=0);
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
    wxTextCtrl* text_ctrl_value;
    wxButton* button_ok;
    wxButton* button_cancel;
    virtual const std::string getName() const;
};
//-----------------------------------------------------------------------------
#endif // MULTILINEENTERDIALOG_H
