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

  $Id$

  Contributor(s):
*/
//-----------------------------------------------------------------------------
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "metadata/root.h"
#include "metadata/server.h"
#include "metadata/database.h"
#include "AdvancedSearchFrame.h"
//-----------------------------------------------------------------------------
AdvancedSearchFrame::AdvancedSearchFrame(wxWindow *parent)
    :BaseFrame(parent, -1, _("Advanced Metadata Search"))
{
    wxBoxSizer *mainSizer;
    mainSizer = new wxBoxSizer(wxVERTICAL);
    mainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    wxBoxSizer *innerSizer;
    innerSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *leftSizer;
    leftSizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer *fgSizer1;
    fgSizer1 = new wxFlexGridSizer(2, 3, 0, 0);
    fgSizer1->AddGrowableCol(1);

    m_staticText1 = new wxStaticText(mainPanel, wxID_ANY, wxT("Type"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(m_staticText1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    // TODO: add all types
    wxString choices1[] = { wxT("[Any]") };
    int nchoices1 = sizeof(choices1) / sizeof(wxString);
    choice_type = new wxChoice(mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, nchoices1, choices1, 0);
    fgSizer1->Add(choice_type, 0, wxALL|wxEXPAND, 5);
    button_add_type = new wxButton(mainPanel, wxID_ANY, wxT("Add"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(button_add_type, 0, wxALL, 5);

    m_staticText2 = new wxStaticText(mainPanel, wxID_ANY, wxT("Name"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(m_staticText2, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_name = new wxTextCtrl(mainPanel, wxID_ANY, wxT("Allows * and ? wildcards"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(textctrl_name, 0, wxALL|wxEXPAND, 5);
    button_add_name = new wxButton(mainPanel, wxID_ANY, wxT("Add"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(button_add_name, 0, wxALL, 5);

    m_staticText3 = new wxStaticText(mainPanel, wxID_ANY, wxT("Description"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(m_staticText3, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_description = new wxTextCtrl(mainPanel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(textctrl_description, 0, wxALL|wxEXPAND, 5);
    button_add_description = new wxButton(mainPanel, wxID_ANY, wxT("Add"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(button_add_description, 0, wxALL, 5);

    m_staticText4 = new wxStaticText(mainPanel, wxID_ANY, wxT("DDL contains"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(m_staticText4, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_ddl = new wxTextCtrl(mainPanel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(textctrl_ddl, 0, wxALL|wxEXPAND, 5);
    button_add_ddl = new wxButton(mainPanel, wxID_ANY, wxT("Add"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(button_add_ddl, 0, wxALL, 5);

    m_staticText5 = new wxStaticText(mainPanel, wxID_ANY, wxT("Has field named"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(m_staticText5, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_field = new wxTextCtrl(mainPanel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(textctrl_field, 0, wxALL|wxEXPAND, 5);
    button_add_field = new wxButton(mainPanel, wxID_ANY, wxT("Add"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(button_add_field, 0, wxALL, 5);

    m_staticText6 = new wxStaticText(mainPanel, wxID_ANY, wxT("Search in"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(m_staticText6, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    choice_database = new wxChoice(mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, 0, 0);
    choice_database->Append(_("[Connected databases]"), (void *)0);
    choice_database->Append(_("[All databases]"), (void *)0);
    Root& r = getGlobalRoot();  // add all databases
    std::vector<MetadataItem*> servers;
    r.getChildren(servers);
    for (std::vector<MetadataItem*>::iterator it = servers.begin();
        it != servers.end(); ++it)
    {
        Server *s = dynamic_cast<Server *>(*it);
        if (!s)
            continue;
        MetadataCollection<Database> *dbs = s->getDatabases();
        for (MetadataCollection<Database>::iterator i2 = dbs->begin();
            i2 != dbs->end(); ++i2)
        {   // FIXME: we store database pointer, so this frame should be made
            //        observer in case database is dropped/unregistered
            choice_database->Append((*it)->getName_() + wxT("::") + (*i2).getName_(), (void *)(&(*i2)));
        }
    }
    fgSizer1->Add(choice_database, 0, wxALL|wxEXPAND, 5);
    button_add_database = new wxButton(mainPanel, wxID_ANY, wxT("Add"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(button_add_database, 0, wxALL, 5);
    leftSizer->Add(fgSizer1, 0, wxEXPAND, 5);

    label_search_criteria = new wxStaticText(mainPanel, wxID_ANY, wxT("SEARCH CRITERIA"), wxDefaultPosition, wxDefaultSize, 0);
    leftSizer->Add(label_search_criteria, 0, wxALL, 5);
    listctrl_criteria = new wxListCtrl(mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_ICON);
    leftSizer->Add(listctrl_criteria, 1, wxALL|wxEXPAND, 5);

    wxBoxSizer *bSizer5;
    bSizer5 = new wxBoxSizer(wxHORIZONTAL);
    button_remove = new wxButton(mainPanel, ID_button_remove, wxT("Remove selected"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer5->Add(button_remove, 0, wxALL, 5);
    bSizer5->Add(2, 2, 1, 0, 0);
    button_search = new wxButton(mainPanel, ID_button_start, wxT("Start search"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer5->Add(button_search, 0, wxALL, 5);
    leftSizer->Add(bSizer5, 0, wxEXPAND, 5);
    innerSizer->Add(leftSizer, 1, wxEXPAND, 5);

    wxBoxSizer *rightSizer;
    rightSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *bSizer7;
    bSizer7 = new wxBoxSizer(wxHORIZONTAL);
    label_search_results = new wxStaticText(mainPanel, wxID_ANY, wxT("SEARCH RESULTS"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer7->Add(label_search_results, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    bSizer7->Add(20, 2, 1, 0, 0);
    checkbox_ddl = new wxCheckBox(mainPanel, ID_checkbox_ddl, wxT("Show DDL for selected objects"), wxDefaultPosition, wxDefaultSize, 0);
    checkbox_ddl->SetValue(true);   // checked by default
    bSizer7->Add(checkbox_ddl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    rightSizer->Add(bSizer7, 0, wxEXPAND, 5);

    splitter1 = new wxSplitterWindow(mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D);
    top_splitter_panel = new wxPanel(splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    wxBoxSizer *top_splitter_sizer = new wxBoxSizer(wxVERTICAL);
    listctrl_results = new wxListCtrl(top_splitter_panel, ID_listctrl_results, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);

    top_splitter_sizer->Add(listctrl_results, 1, wxALL|wxEXPAND, 5);
    top_splitter_panel->SetSizer(top_splitter_sizer);
    bottom_splitter_panel = new wxPanel(splitter1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
    wxBoxSizer *bottom_splitter_sizer = new wxBoxSizer(wxVERTICAL);
    stc_ddl = new wxTextCtrl(bottom_splitter_panel, wxID_ANY, wxT("DDL for selected objects"), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_WORDWRAP);
    bottom_splitter_sizer->Add(stc_ddl, 1, wxALL|wxEXPAND, 5);
    bottom_splitter_panel->SetSizer(bottom_splitter_sizer);
    splitter1->SplitHorizontally(top_splitter_panel,bottom_splitter_panel,0);
    rightSizer->Add(splitter1, 1, wxEXPAND, 5);
    innerSizer->Add(rightSizer, 1, wxEXPAND, 5);
    mainPanel->SetSizer(innerSizer);
    mainSizer->Add(mainPanel, 1, wxEXPAND, 0);
    SetSizerAndFit(mainSizer);

    #include "search.xpm"
    wxBitmap bmp = wxBitmap(search_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);
}
//-----------------------------------------------------------------------------

