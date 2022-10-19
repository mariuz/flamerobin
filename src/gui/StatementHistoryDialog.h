/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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
*/


#ifndef FR_STATEMENTHISTORYDIALOG_H
#define FR_STATEMENTHISTORYDIALOG_H

#include "BaseDialog.h"

class StatementHistory;
class wxStyledTextCtrl;
class wxSplitter;

class StatementHistoryDialog : public BaseDialog
{
private:
    wxString sqlM;
    bool isSearchingM;
    StatementHistory *historyM;
    wxStaticText *m_staticText2;
    wxStaticText *dateTimeTextM;
    wxTextCtrl *textctrl_search;
    wxStyledTextCtrl *textctrl_statement;
    wxButton *button_search;
    wxButton *button_delete;
    wxButton *button_copy;
    wxButton *button_cancel;
    wxListBox *listbox_search;
    wxGauge *gauge_progress;
    void setSearching(bool searching);
    wxSplitterWindow* mainSplitter;
    wxPanel* leftSplitterPanel;
    wxPanel* rightSplitterPanel;

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
    void OnListBoxSelect(wxCommandEvent& event);
    void OnListBoxSearchDoubleClick(wxCommandEvent& event);
public:
    StatementHistoryDialog(wxWindow *parent, StatementHistory *history,
        const wxString& title = "SQL Statement History");
    wxString getSql() const;

    DECLARE_EVENT_TABLE()
};

#endif
