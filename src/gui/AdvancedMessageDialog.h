/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


  $Id$

*/

#ifndef FR_ADVANCEDMESSAGEDIALOG_H
#define FR_ADVANCEDMESSAGEDIALOG_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <vector>
#include <utility>
#include "gui/BaseDialog.h"
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
class AdvancedMessageDialog: public BaseDialog
{
protected:
    wxString configKeyNameM;
    wxCheckBox* checkBoxM;

public:
    AdvancedMessageDialog(wxWindow* parent, const wxString& message,
        const wxString& caption, int style = 0,
        AdvancedMessageDialogButtons* buttons = 0,
        const wxString& name = wxEmptyString);

    bool getDontShowAgain() const;

    void OnButtonClick(wxCommandEvent& event);
};
//----------------------------------------------------------------------------
// you can provide regular buttons in "style" parameter,
// just like in wxMessageBox
int AdvancedMessageBox(const wxString& message, const wxString& caption,
    int style = 0, AdvancedMessageDialogButtons* buttons = 0,
    wxWindow* parent = 0, const wxString& keyname = wxEmptyString);
//----------------------------------------------------------------------------
#endif
