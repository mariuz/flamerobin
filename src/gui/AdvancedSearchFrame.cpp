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

#include "dberror.h"
#include "frutils.h"
#include "metadata/metadataitem.h"
#include "metadata/root.h"
#include "metadata/server.h"
#include "metadata/database.h"
#include "metadata/CreateDDLVisitor.h"
#include "AdvancedSearchFrame.h"
//-----------------------------------------------------------------------------
// derived class since we need to catch size event
class AdjustableListCtrl: public wxListCtrl
{
public:
    AdjustableListCtrl(wxWindow *parent, wxWindowID id, long style)
        :wxListCtrl(parent, id, wxDefaultPosition, wxDefaultSize, style)
    {
    };

    void OnSize(wxSizeEvent& event)
    {
        int w, h;
        GetSize(&w, &h);
        int w1 = w/3;
        SetColumnWidth(0, w1);
        w -= w1;
        if (GetColumnCount() == 3)  // result list
        {
            SetColumnWidth(2, w1);
            w -= w1;
        }
        SetColumnWidth(1, w-4);
        event.Skip();
    };
    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(AdjustableListCtrl, wxListCtrl)
    EVT_SIZE(AdjustableListCtrl::OnSize)
END_EVENT_TABLE()
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
        wxT("TABLE"), wxT("VIEW"), wxT("PROCEDURE"),
        wxT("TRIGGER"), wxT("GENERATOR"), wxT("FUNCTION"), wxT("DOMAIN"),
        wxT("ROLE"), wxT("COLUMN"), wxT("EXCEPTION")
    };
    int nchoices1 = sizeof(choices1) / sizeof(wxString);
    choice_type = new wxChoice(mainPanel, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, nchoices1, choices1, 0);
    fgSizer1->Add(choice_type, 0, wxALL|wxEXPAND, 5);
    button_add_type = new wxButton(mainPanel, ID_button_add_type, _("Add"));
    fgSizer1->Add(button_add_type, 0, wxALL, 5);
    choice_type->SetSelection(0);

    m_staticText2 = new wxStaticText(mainPanel, wxID_ANY, _("Name"));
    fgSizer1->Add(m_staticText2, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_name = new wxTextCtrl(mainPanel, wxID_ANY,
        _("Allows * and ? wildcards"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(textctrl_name, 0, wxALL|wxEXPAND, 5);
    button_add_name = new wxButton(mainPanel, ID_button_add_name, _("Add"));
    fgSizer1->Add(button_add_name, 0, wxALL, 5);

    m_staticText3 = new wxStaticText(mainPanel, wxID_ANY, _("Description"));
    fgSizer1->Add(m_staticText3, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_description = new wxTextCtrl(mainPanel, wxID_ANY, wxEmptyString);
    fgSizer1->Add(textctrl_description, 0, wxALL|wxEXPAND, 5);
    button_add_description = new wxButton(mainPanel,
        ID_button_add_description, _("Add"));
    fgSizer1->Add(button_add_description, 0, wxALL, 5);

    m_staticText4 = new wxStaticText(mainPanel, wxID_ANY, _("DDL contains"));
    fgSizer1->Add(m_staticText4, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_ddl = new wxTextCtrl(mainPanel, wxID_ANY, wxEmptyString);
    fgSizer1->Add(textctrl_ddl, 0, wxALL|wxEXPAND, 5);
    button_add_ddl = new wxButton(mainPanel, ID_button_add_ddl, _("Add"));
    fgSizer1->Add(button_add_ddl, 0, wxALL, 5);

    m_staticText5 = new wxStaticText(mainPanel, wxID_ANY,_("Has field named"));
    fgSizer1->Add(m_staticText5, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_field = new wxTextCtrl(mainPanel, wxID_ANY, wxEmptyString);
    fgSizer1->Add(textctrl_field, 0, wxALL|wxEXPAND, 5);
    button_add_field = new wxButton(mainPanel, ID_button_add_field, _("Add"));
    fgSizer1->Add(button_add_field, 0, wxALL, 5);

    m_staticText6 = new wxStaticText(mainPanel, wxID_ANY, _("Search in"));
    fgSizer1->Add(m_staticText6, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    choice_database = new wxChoice(mainPanel, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, 0, 0, 0);
    choice_database->Append(_("[All Connected Databases]"), (void *)0);
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
    button_add_database = new wxButton(mainPanel, ID_button_add_database,
        _("Add"));
    fgSizer1->Add(button_add_database, 0, wxALL, 5);
    leftSizer->Add(fgSizer1, 0, wxEXPAND, 5);

    listctrl_criteria = new AdjustableListCtrl(mainPanel, wxID_ANY,
        wxLC_REPORT|wxLC_VRULES|wxSUNKEN_BORDER);
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
    listctrl_results = new AdjustableListCtrl(top_splitter_panel,
        ID_listctrl_results, wxLC_REPORT|wxLC_VRULES|wxSUNKEN_BORDER);
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
    Centre();

    #include "search.xpm"
    wxBitmap bmp = wxBitmap(search_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);
}
//-----------------------------------------------------------------------------
void AdvancedSearchFrame::addCriteria(CriteriaItem::Type type, const wxString&
    value, Database *db)
{
    if (value.IsEmpty())
        return;
    CriteriaItem c(value, db);
    for (CriteriaCollection::const_iterator
        it = searchCriteriaM.lower_bound(type);
        it != searchCriteriaM.upper_bound(type); ++it)
    {
        if ((*it).second == c)
        {
            wxMessageBox(_("That criteria already exists"),
                _("Warning"), wxOK|wxICON_WARNING);
            return;
        }
    }

    searchCriteriaM.insert(
        std::pair<CriteriaItem::Type, CriteriaItem>(type, c));
    rebuildList();
}
//-----------------------------------------------------------------------------
void AdvancedSearchFrame::rebuildList()
{
    listctrl_criteria->DeleteAllItems();
    long index = 0;
    for (CriteriaCollection::iterator it =
        searchCriteriaM.begin(); it != searchCriteriaM.end(); ++it)
    {
        listctrl_criteria->InsertItem(index,
            CriteriaItem::getTypeString((*it).first));
        listctrl_criteria->SetItem(index, 1, (*it).second.value);
        listctrl_criteria->SetItemData(index, index);
        (*it).second.listIndex = index;
        index++;
    }
}
//-----------------------------------------------------------------------------
void AdvancedSearchFrame::addResult(Database* db, MetadataItem* item)
{
    listctrl_results->InsertItem(0, db->getName_());
    listctrl_results->SetItem(0, 1, item->getTypeName());
    listctrl_results->SetItem(0, 2, item->getName_());
}
//-----------------------------------------------------------------------------
bool AdvancedSearchFrame::match(CriteriaItem::Type type, const wxString& text)
{
    for (CriteriaCollection::const_iterator ci =
        searchCriteriaM.lower_bound(type); ci !=
        searchCriteriaM.upper_bound(type); ++ci)
    {
        if (!text.Matches((*ci).second.value))
            return false;
    }
    return true;
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(AdvancedSearchFrame, wxFrame)
    EVT_CHECKBOX(AdvancedSearchFrame::ID_checkbox_ddl,
        AdvancedSearchFrame::OnCheckboxDdlToggle)
    EVT_BUTTON(AdvancedSearchFrame::ID_button_remove,
        AdvancedSearchFrame::OnButtonRemoveClick)
    EVT_BUTTON(AdvancedSearchFrame::ID_button_start,
        AdvancedSearchFrame::OnButtonStartClick)
    EVT_BUTTON(AdvancedSearchFrame::ID_button_add_type,
        AdvancedSearchFrame::OnButtonAddTypeClick)
    EVT_BUTTON(AdvancedSearchFrame::ID_button_add_name,
        AdvancedSearchFrame::OnButtonAddNameClick)
    EVT_BUTTON(AdvancedSearchFrame::ID_button_add_description,
        AdvancedSearchFrame::OnButtonAddDescriptionClick)
    EVT_BUTTON(AdvancedSearchFrame::ID_button_add_ddl,
        AdvancedSearchFrame::OnButtonAddDDLClick)
    EVT_BUTTON(AdvancedSearchFrame::ID_button_add_field,
        AdvancedSearchFrame::OnButtonAddFieldClick)
    EVT_BUTTON(AdvancedSearchFrame::ID_button_add_database,
        AdvancedSearchFrame::OnButtonAddDatabaseClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void AdvancedSearchFrame::OnCheckboxDdlToggle(wxCommandEvent& event)
{
    if (event.IsChecked())
        splitter1->SplitHorizontally(top_splitter_panel,bottom_splitter_panel);
    else
        splitter1->Unsplit();
}
//-----------------------------------------------------------------------------
void AdvancedSearchFrame::OnButtonRemoveClick(wxCommandEvent& event)
{
    // iterate all selected items and remove them from searchCriteriaM
    long item = -1;
    do
    {
        item = listctrl_criteria->GetNextItem(item, wxLIST_NEXT_ALL,
            wxLIST_STATE_SELECTED);
        if (item != -1)
        {
            long index = listctrl_criteria->GetItemData(item);
            for (CriteriaCollection::iterator it = searchCriteriaM.begin();
                it != searchCriteriaM.end(); ++it)
            {
                if ((*it).second.listIndex == index)
                {
                    searchCriteriaM.erase(it);
                    break;
                }
            }
        }
    }
    while (item != -1);
    rebuildList();
}
//-----------------------------------------------------------------------------
void AdvancedSearchFrame::OnButtonStartClick(wxCommandEvent& event)
{
    // build list of databases to search from
    if (searchCriteriaM.count(CriteriaItem::ctDB) == 0)
    {   // TODO: search all databases?
        return;
    }

    // get all types we want to match
    std::set<NodeType> types;
    if (searchCriteriaM.count(CriteriaItem::ctType) != 0)
    {
        for (CriteriaCollection::const_iterator
            ct = searchCriteriaM.lower_bound(CriteriaItem::ctType);
            ct != searchCriteriaM.upper_bound(CriteriaItem::ctType); ++ct)
        {
            types.insert(getTypeByName((*ct).second.value));
        }
    }

    // foreach database
    for (CriteriaCollection::const_iterator
        cid = searchCriteriaM.lower_bound(CriteriaItem::ctDB);
        cid != searchCriteriaM.upper_bound(CriteriaItem::ctDB); ++cid)
    {
        Database *db = (*cid).second.database;
        if (!db->isConnected() && !connectDatabase(db, this))
            continue;

        std::vector<MetadataItem *> colls;
        db->getCollections(colls);
        for (std::vector<MetadataItem *>::iterator col = colls.begin(); col !=
            colls.end(); ++col)
        {
            std::vector<MetadataItem *> ch;
            (*col)->getChildren(ch);
            for (std::vector<MetadataItem *>::iterator it = ch.begin(); it !=
                ch.end(); ++it)
            {
                if (!types.empty() &&
                    types.find((*it)->getType()) == types.end())
                    break;
                if (searchCriteriaM.count(CriteriaItem::ctName) > 0)
                {
                    wxString name = (*it)->getName_();
                    if (!match(CriteriaItem::ctName, name))
                        continue;
                }
                if (searchCriteriaM.count(CriteriaItem::ctDescription) > 0)
                {
                    wxString desc = (*it)->getDescription();
                    if (!match(CriteriaItem::ctDescription, desc))
                        continue;
                }
                if (searchCriteriaM.count(CriteriaItem::ctDDL) > 0)
                {
                    CreateDDLVisitor cdv;
                    (*it)->acceptVisitor(&cdv);
                    if (!match(CriteriaItem::ctDDL, cdv.getSql()))
                        continue;
                }
                if (searchCriteriaM.count(CriteriaItem::ctField) > 0)
                {
                    // TODO: check fields?
                }
                // everything ok -> add to results
                addResult(db, *it);
            }
        }
    }
}
//-----------------------------------------------------------------------------
void AdvancedSearchFrame::OnButtonAddTypeClick(wxCommandEvent& event)
{
    addCriteria(CriteriaItem::ctType, choice_type->GetStringSelection());
}
//-----------------------------------------------------------------------------
void AdvancedSearchFrame::OnButtonAddNameClick(wxCommandEvent& event)
{
    addCriteria(CriteriaItem::ctName, textctrl_name->GetValue());
}
//-----------------------------------------------------------------------------
void AdvancedSearchFrame::OnButtonAddDescriptionClick(wxCommandEvent& event)
{
    addCriteria(CriteriaItem::ctDescription, textctrl_description->GetValue());
}
//-----------------------------------------------------------------------------
void AdvancedSearchFrame::OnButtonAddDDLClick(wxCommandEvent& event)
{
    addCriteria(CriteriaItem::ctDDL, textctrl_ddl->GetValue());
}
//-----------------------------------------------------------------------------
void AdvancedSearchFrame::OnButtonAddFieldClick(wxCommandEvent& event)
{
    addCriteria(CriteriaItem::ctField, textctrl_field->GetValue());
}
//-----------------------------------------------------------------------------
void AdvancedSearchFrame::OnButtonAddDatabaseClick(wxCommandEvent& event)
{
    if (choice_database->GetSelection() == 0)   // all connected databases
    {
        for (int i=1; i<choice_database->GetCount(); ++i)
        {
            Database *db = (Database *)choice_database->GetClientData(i);
            if (db && db->isConnected())
            {
                addCriteria(CriteriaItem::ctDB, choice_database->GetString(i),
                db);
            }
        }
    }
    else    // a single database
    {
        Database *db = (Database *)
            choice_database->GetClientData(choice_database->GetSelection());
        addCriteria(CriteriaItem::ctDB, choice_database->GetStringSelection(),
            db);
    }
}
//-----------------------------------------------------------------------------
