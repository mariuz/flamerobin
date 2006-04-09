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
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif //WX_PRECOMP

#include "statementHistory.h"
#include "ExecuteSqlFrame.h"
#include "StatementHistoryFrame.h"
//-----------------------------------------------------------------------------
StatementHistoryFrame::StatementHistoryFrame(ExecuteSqlFrame *parent,
    StatementHistory *history, const wxString& title)
    :BaseFrame(parent, -1, title), historyM(history),
     isSearchingM(false)
{
    statusBarM = CreateStatusBar();
    statusBarM->SetStatusText(
        _("Searching with empty search box shows entire history."));

    wxBoxSizer *mainSizer;
    mainSizer = new wxBoxSizer(wxVERTICAL);
    m_panel1 = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL);

    wxBoxSizer *innerSizer;
    innerSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *topSizer;
    topSizer = new wxBoxSizer(wxHORIZONTAL);
    m_staticText2 = new wxStaticText(m_panel1, wxID_ANY, wxT("Search for:"),
        wxDefaultPosition, wxDefaultSize, 0);
    topSizer->Add(m_staticText2, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

    textctrl_search = new wxTextCtrl(m_panel1, wxID_ANY, wxT(""),
        wxDefaultPosition, wxDefaultSize, 0);
    topSizer->Add(textctrl_search, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5);

    button_search = new wxButton(m_panel1, ID_button_search, wxT("&Search"),
        wxDefaultPosition, wxDefaultSize, 0);
    topSizer->Add(button_search, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

    button_delete = new wxButton(m_panel1, ID_button_delete,
        wxT("&Delete Selected"), wxDefaultPosition, wxDefaultSize, 0);
    topSizer->Add(button_delete, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

    button_copy = new wxButton(m_panel1, ID_button_copy,
        wxT("&Copy to editor"), wxDefaultPosition, wxDefaultSize, 0);
    topSizer->Add(button_copy, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

    innerSizer->Add(topSizer, 0, wxEXPAND, 5);
    listbox_search = new wxListBox(m_panel1, ID_listbox_search,
        wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_MULTIPLE);
    innerSizer->Add(listbox_search, 1, wxALL|wxEXPAND, 5);

    m_panel1->SetSizer(innerSizer);
    mainSizer->Add(m_panel1, 1, wxEXPAND, 0);
    this->SetSizerAndFit(mainSizer);

    #include "history.xpm"
    wxBitmap bmp = wxBitmap(history_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);

    button_search->SetDefault();
    textctrl_search->SetFocus();
    // center on parent
    SetSize(620, 400);
    Centre();
}
//-----------------------------------------------------------------------------
void StatementHistoryFrame::setSearching(bool searching)
{
    isSearchingM = searching;
    button_delete->Enable(!searching);
    button_copy->Enable(!searching);
    if (searching)
        button_search->SetLabel(_("&Stop"));
    else
        button_search->SetLabel(_("&Search"));
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(StatementHistoryFrame, wxFrame)
    EVT_BUTTON(StatementHistoryFrame::ID_button_search,
        StatementHistoryFrame::OnButtonSearchClick)
    EVT_BUTTON(StatementHistoryFrame::ID_button_delete,
        StatementHistoryFrame::OnButtonDeleteClick)
    EVT_BUTTON(StatementHistoryFrame::ID_button_copy,
        StatementHistoryFrame::OnButtonCopyClick)
    EVT_LISTBOX_DCLICK(StatementHistoryFrame::ID_listbox_search,
        StatementHistoryFrame::OnListBoxSearchDoubleClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void StatementHistoryFrame::OnButtonSearchClick(wxCommandEvent&
    WXUNUSED(event))
{
    if (isSearchingM)
    {
        setSearching(false);
        return;
    }

    // start the search
    listbox_search->Clear();
    wxString searchString = textctrl_search->GetValue().Upper();
    setSearching(true);
    StatementHistory::Position total = historyM->size();
    for (StatementHistory::Position p = total - 1; (int)p >= 0; --p)
    {
        wxYield();
        if (!isSearchingM)
        {
            statusBarM->SetStatusText(_("Search stopped."));
            return;
        }

        statusBarM->SetStatusText(wxString::Format(
            _("Searching %d of %d (%d%%)"), (int)p, (int)total, (int)p/total));
        wxString s = historyM->get(p);
        if (searchString.IsEmpty() || s.Upper().Contains(searchString))
        {
            wxString entry;
            entry = (s.Length() > 200) ? s.Mid(0, 200) + wxT("...") : s;
            entry.Replace(wxT("\n"), wxT(" "));
            entry.Replace(wxT("\r"), wxEmptyString);
            listbox_search->Append(entry, (void *)p);
        }
    }
    setSearching(false);
    statusBarM->SetStatusText(_("Search complete."));
}
//-----------------------------------------------------------------------------
void StatementHistoryFrame::OnButtonDeleteClick(wxCommandEvent&
    WXUNUSED(event))
{
    wxArrayInt temp;
    if (listbox_search->GetSelections(temp) == 0)
    {
        wxMessageBox(_("Please select items you wish to delete"),
            _("Nothing is selected"),
            wxOK|wxICON_WARNING);
        return;
    }
    wxBusyCursor b;
    std::vector<StatementHistory::Position> vect;
    for (size_t i=0; i<temp.GetCount(); ++i)
    {
        vect.push_back(
            (StatementHistory::Position)listbox_search->GetClientData(
            temp.Item(i)));
    }
    historyM->deleteItems(vect);

    for (size_t i=temp.GetCount()-1; (int)i >= 0; --i)
        listbox_search->Delete(temp.Item(i));
}
//-----------------------------------------------------------------------------
void StatementHistoryFrame::OnButtonCopyClick(wxCommandEvent& WXUNUSED(event))
{
   // it is certain, but who knows...
    ExecuteSqlFrame *f = dynamic_cast<ExecuteSqlFrame *>(GetParent());
    if (!f)
        return;

    int sel = listbox_search->GetSelection();
    if (sel == wxNOT_FOUND)
        return;
    StatementHistory::Position item =
        (StatementHistory::Position)listbox_search->GetClientData(sel);
    f->setSql(historyM->get(item));
    Destroy();
}
//-----------------------------------------------------------------------------
void StatementHistoryFrame::OnListBoxSearchDoubleClick(wxCommandEvent& event)
{
   // it is certain, but who knows...
    ExecuteSqlFrame *f = dynamic_cast<ExecuteSqlFrame *>(GetParent());
    StatementHistory::Position item =
        (StatementHistory::Position)event.GetClientData();
    if (!f || (int)item < 0)
        return;
    f->setSql(historyM->get(item));
    Destroy();
}
//-----------------------------------------------------------------------------

