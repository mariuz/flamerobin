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
    mainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTAB_TRAVERSAL);
    wxBoxSizer *innerSizer;
    innerSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *leftSizer;
    leftSizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer *fgSizer1;
    fgSizer1 = new wxFlexGridSizer(2, 3, 0, 0);
    fgSizer1->AddGrowableCol(1);

    m_staticText1 = new wxStaticText(mainPanel, wxID_ANY, _("Type"));
    fgSizer1->Add(m_staticText1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    wxString choices1[] =
    {
        _("[Any]"), wxT("TABLE"), wxT("VIEW"), wxT("PROCEDURE"),
        wxT("TRIGGER"), wxT("GENERATOR"), wxT("FUNCTION"), wxT("DOMAIN"),
        wxT("ROLE"), wxT("COLUMN"), wxT("EXCEPTION")
    };
    int nchoices1 = sizeof(choices1) / sizeof(wxString);
    choice_type = new wxChoice(mainPanel, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, nchoices1, choices1, 0);
    fgSizer1->Add(choice_type, 0, wxALL|wxEXPAND, 5);
    button_add_type = new wxButton(mainPanel, wxID_ANY, _("Add"));
    fgSizer1->Add(button_add_type, 0, wxALL, 5);

    m_staticText2 = new wxStaticText(mainPanel, wxID_ANY, _("Name"));
    fgSizer1->Add(m_staticText2, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_name = new wxTextCtrl(mainPanel, wxID_ANY,
        _("Allows * and ? wildcards"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(textctrl_name, 0, wxALL|wxEXPAND, 5);
    button_add_name = new wxButton(mainPanel, wxID_ANY, _("Add"));
    fgSizer1->Add(button_add_name, 0, wxALL, 5);

    m_staticText3 = new wxStaticText(mainPanel, wxID_ANY, _("Description"));
    fgSizer1->Add(m_staticText3, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_description = new wxTextCtrl(mainPanel, wxID_ANY, wxEmptyString);
    fgSizer1->Add(textctrl_description, 0, wxALL|wxEXPAND, 5);
    button_add_description = new wxButton(mainPanel, wxID_ANY, _("Add"));
    fgSizer1->Add(button_add_description, 0, wxALL, 5);

    m_staticText4 = new wxStaticText(mainPanel, wxID_ANY, _("DDL contains"));
    fgSizer1->Add(m_staticText4, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_ddl = new wxTextCtrl(mainPanel, wxID_ANY, wxEmptyString);
    fgSizer1->Add(textctrl_ddl, 0, wxALL|wxEXPAND, 5);
    button_add_ddl = new wxButton(mainPanel, wxID_ANY, _("Add"));
    fgSizer1->Add(button_add_ddl, 0, wxALL, 5);

    m_staticText5 = new wxStaticText(mainPanel, wxID_ANY,_("Has field named"));
    fgSizer1->Add(m_staticText5, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_field = new wxTextCtrl(mainPanel, wxID_ANY, wxEmptyString);
    fgSizer1->Add(textctrl_field, 0, wxALL|wxEXPAND, 5);
    button_add_field = new wxButton(mainPanel, wxID_ANY, _("Add"));
    fgSizer1->Add(button_add_field, 0, wxALL, 5);

    m_staticText6 = new wxStaticText(mainPanel, wxID_ANY, _("Search in"));
    fgSizer1->Add(m_staticText6, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    choice_database = new wxChoice(mainPanel, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, 0, 0, 0);
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
            choice_database->Append((*it)->getName_() + wxT("::") +
                (*i2).getName_(), (void *)(&(*i2)));
        }
    }
    choice_database->SetSelection(0);
    fgSizer1->Add(choice_database, 0, wxALL|wxEXPAND, 5);
    button_add_database = new wxButton(mainPanel, wxID_ANY, _("Add"));
    fgSizer1->Add(button_add_database, 0, wxALL, 5);
    leftSizer->Add(fgSizer1, 0, wxEXPAND, 5);

    listctrl_criteria = new wxListCtrl(mainPanel, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxLC_REPORT|wxLC_VRULES|wxSUNKEN_BORDER);
    wxListItem itemCol;
    itemCol.SetImage(-1);
    itemCol.SetText(_("Search criteria"));
    listctrl_criteria->InsertColumn(0, itemCol);
    itemCol.SetText(_("Value"));
    listctrl_criteria->InsertColumn(1, itemCol);
    leftSizer->Add(listctrl_criteria, 1, wxALL|wxEXPAND, 5);

    wxBoxSizer *bSizer5;
    bSizer5 = new wxBoxSizer(wxHORIZONTAL);
    button_remove = new wxButton(mainPanel, ID_button_remove,
        _("Remove selected"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer5->Add(button_remove, 0, wxALL, 5);
    bSizer5->Add(2, 2, 1, 0, 0);
    button_search = new wxButton(mainPanel, ID_button_start,
        _("Start search"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer5->Add(button_search, 0, wxALL, 5);
    leftSizer->Add(bSizer5, 0, wxEXPAND, 5);
    innerSizer->Add(leftSizer, 1, wxEXPAND, 5);

    wxBoxSizer *rightSizer;
    rightSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *bSizer7;
    bSizer7 = new wxBoxSizer(wxHORIZONTAL);
    label_search_results = new wxStaticText(mainPanel, wxID_ANY,
        _("SEARCH RESULTS"), wxDefaultPosition, wxDefaultSize, 0);
    bSizer7->Add(label_search_results, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    bSizer7->Add(20, 2, 1, 0, 0);
    checkbox_ddl = new wxCheckBox(mainPanel, ID_checkbox_ddl,
        _("Show DDL for selected objects"));
    checkbox_ddl->SetValue(true);   // checked by default
    bSizer7->Add(checkbox_ddl, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    rightSizer->Add(bSizer7, 0, wxEXPAND, 5);

    splitter1 = new wxSplitterWindow(mainPanel, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxSP_3D);
    top_splitter_panel = new wxPanel(splitter1, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxTAB_TRAVERSAL);
    wxBoxSizer *top_splitter_sizer = new wxBoxSizer(wxVERTICAL);
    listctrl_results = new wxListCtrl(top_splitter_panel, ID_listctrl_results,
        wxDefaultPosition, wxDefaultSize,
        wxLC_REPORT|wxLC_VRULES|wxSUNKEN_BORDER);
    itemCol.SetText(_("Database"));
    listctrl_results->InsertColumn(0, itemCol);
    itemCol.SetText(_("Type"));
    listctrl_results->InsertColumn(1, itemCol);
    itemCol.SetText(_("Name"));
    listctrl_results->InsertColumn(2, itemCol);
    top_splitter_sizer->Add(listctrl_results, 1, wxALL|wxEXPAND, 5);

    top_splitter_panel->SetSizer(top_splitter_sizer);
    bottom_splitter_panel = new wxPanel(splitter1, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxTAB_TRAVERSAL);
    wxBoxSizer *bottom_splitter_sizer = new wxBoxSizer(wxVERTICAL);
    stc_ddl = new wxTextCtrl(bottom_splitter_panel, wxID_ANY,
        _("DDL for selected objects"), wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE|wxTE_WORDWRAP);
    bottom_splitter_sizer->Add(stc_ddl, 1, wxALL|wxEXPAND, 5);
    bottom_splitter_panel->SetSizer(bottom_splitter_sizer);
    splitter1->SplitHorizontally(top_splitter_panel,bottom_splitter_panel,0);
    rightSizer->Add(splitter1, 1, wxEXPAND, 5);
    innerSizer->Add(rightSizer, 1, wxEXPAND, 5);
    mainPanel->SetSizer(innerSizer);
    mainSizer->Add(mainPanel, 1, wxEXPAND, 0);
    SetSizerAndFit(mainSizer);

    Layout();
    int w, h;
    listctrl_criteria->GetSize(&w, &h);
    listctrl_criteria->SetColumnWidth( 0, w/2 );
    listctrl_criteria->SetColumnWidth( 1, w/2-3 ); // -3 for sunken border
    listctrl_results->GetSize(&w, &h);
    listctrl_results->SetColumnWidth( 0, w/3 );
    listctrl_results->SetColumnWidth( 1, w/3-3 );
    listctrl_results->SetColumnWidth( 2, w/3 );

    #include "search.xpm"
    wxBitmap bmp = wxBitmap(search_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);
}
//-----------------------------------------------------------------------------

