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

  The Initial Developer of the Original Code is Milan Babuskov

  Portions created by the original developer
  are Copyright (C) 2006 Milan Babuskov

  All Rights Reserved.

  $Id$

  Contributor(s):
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
        isSearchingM = false;
        button_search->SetLabel(_("&Search"));
        button_delete->Enable(true);
        return;
    }

    // start the search
    listbox_search->Clear();
    wxString searchString = textctrl_search->GetValue().Upper();
    button_delete->Enable(false);
    isSearchingM = true;
    button_search->SetLabel(_("&Stop"));
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
            s.Replace(wxT("\n"), wxT(" "));
            s.Replace(wxT("\r"), wxEmptyString);
            if (s.Length() > 150)
                listbox_search->Append(s.Mid(0, 150), (void *)p);
            listbox_search->Append(s, (void *)p);
        }
    }
    isSearchingM = false;
    button_delete->Enable(true);
    button_search->SetLabel(_("&Search"));
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
    if (!f)
        return;
    StatementHistory::Position item =
        (StatementHistory::Position)event.GetClientData();
    f->setSql(historyM->get(item));
    Destroy();
}
//-----------------------------------------------------------------------------

