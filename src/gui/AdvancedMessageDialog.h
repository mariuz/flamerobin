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
  are Copyright (C) 2006 Milan Babuskov.

  All Rights Reserved.

  $Id:  $

  Contributor(s):
*/

#ifndef FR_ADVANCEDMESSAGEDIALOG_H
#define FR_ADVANCEDMESSAGEDIALOG_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <vector>
#include <utility>
//----------------------------------------------------------------------------
class AdvancedMessageDialogButtons
{
public:
    typedef std::pair<int,wxString> ButtonPair;
    typedef std::vector< ButtonPair > ButtonCollection;
    typedef ButtonCollection::const_iterator const_iterator;

    void add(int id, const wxString& s)
    {
        buttonsM.push_back(ButtonPair(id,s));
    }

    const_iterator begin() const
    {
        return buttonsM.begin();
    }

    const_iterator end() const
    {
        return buttonsM.end();
    }

    void clear()
    {
        buttonsM.clear();
    }

    ButtonCollection::size_type size() const
    {
        return buttonsM.size();
    }
private:
    ButtonCollection buttonsM;
};
//----------------------------------------------------------------------------
// don't create instances of this class, use the AdvancedMessageBox function
class AdvancedMessageDialog: public wxDialog
{
protected:
    wxString configKeyNameM;
    wxCheckBox *checkBoxM;

public:
    bool dontShowAgain() const;

    void OnButtonClick(wxCommandEvent& event);

    AdvancedMessageDialog(wxWindow* parent, const wxString& message,
        const wxString& caption, int style = 0,
        AdvancedMessageDialogButtons *buttons = 0,
        const wxString& name = wxEmptyString);

    //DECLARE_EVENT_TABLE()
};
//----------------------------------------------------------------------------
// you can provide regular buttons in "style" parameter,
// just like in wxMessageBox
int AdvancedMessageBox(const wxString& message, const wxString& caption,
    int style = 0, AdvancedMessageDialogButtons *buttons = 0,
    wxWindow* parent = 0, const wxString& keyname = wxEmptyString);
//----------------------------------------------------------------------------
#endif
