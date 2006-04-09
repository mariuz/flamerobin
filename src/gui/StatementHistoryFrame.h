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

//-----------------------------------------------------------------------------
#ifndef FR_StatementHistoryFrame_H
#define FR_StatementHistoryFrame_H

#include "BaseFrame.h"

class StatementHistory;
class ExecuteSqlFrame;
//-----------------------------------------------------------------------------
class StatementHistoryFrame : public BaseFrame
{
private:
    bool isSearchingM;
    StatementHistory *historyM;
    wxStatusBar *statusBarM;
    wxPanel *m_panel1;
    wxStaticText *m_staticText2;
    wxTextCtrl *textctrl_search;
    wxButton *button_search;
    wxButton *button_delete;
    wxButton *button_copy;
    wxListBox *listbox_search;
    void setSearching(bool searching);

    enum    // event handling
    {
        ID_button_search = 101,
        ID_button_delete,
        ID_button_copy,
        ID_listbox_search
    };
    void OnButtonSearchClick(wxCommandEvent& event);
    void OnButtonDeleteClick(wxCommandEvent& event);
    void OnButtonCopyClick(wxCommandEvent& event);
    void OnListBoxSearchDoubleClick(wxCommandEvent& event);
public:
    StatementHistoryFrame(ExecuteSqlFrame *parent, StatementHistory *history,
        const wxString& title = wxT("SQL Statement History"));

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif
