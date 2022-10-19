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

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/stc/stc.h>

#include <set>
#include <algorithm>

#include "frutils.h"
#include "gui/AdvancedSearchFrame.h"
#include "gui/ContextMenuMetadataItemVisitor.h"
#include "gui/controls/DBHTreeControl.h"
#include "gui/MainFrame.h"
#include "gui/ProgressDialog.h"
#include "metadata/column.h"
#include "metadata/CreateDDLVisitor.h"
#include "metadata/database.h"
#include "metadata/metadataitem.h"
#include "metadata/domain.h"
#include "metadata/parameter.h"
#include "metadata/procedure.h"
#include "metadata/root.h"
#include "metadata/server.h"
#include "metadata/table.h"
#include "metadata/view.h"

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

        #ifdef __WXMSW__
        // On Linux, scrollbar is beneath the header so this isn't needed.
        // If it has more than fits on one page => needs scrollbar
        // so we deduce scrollbar width
        if (GetItemCount() > GetCountPerPage())
            w -= wxSystemSettings::GetMetric(wxSYS_VSCROLL_X, this);
        #endif

        int w1 = w/3;
        SetColumnWidth(0, w1);
        w -= w1;
        if (GetColumnCount() == 3)  // result list
        {
            int wd;
            wxMemoryDC dc;
            dc.SetFont(GetFont());
            dc.GetTextExtent("PROCEDUREM", &wd, &h);
            SetColumnWidth(1, wd);
            w -= wd;
        }
        SetColumnWidth(GetColumnCount() - 1, w-4);  // last column
        event.Skip();
    };
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(AdjustableListCtrl, wxListCtrl)
    EVT_SIZE(AdjustableListCtrl::OnSize)
END_EVENT_TABLE()

AdvancedSearchFrame::AdvancedSearchFrame(MainFrame* parent, RootPtr root)
    : BaseFrame(parent, -1, _("Advanced Metadata Search"))
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
    fgSizer1 = new wxFlexGridSizer(6, 3, 0, 0);
    fgSizer1->AddGrowableCol(1);

    m_staticText1 = new wxStaticText(mainPanel, wxID_ANY, _("Type"));
    fgSizer1->Add(m_staticText1, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    wxString choices1[] =
    {
        "TABLE", "VIEW", "PROCEDURE",
        "TRIGGER", "GENERATOR", "FUNCTION", "DOMAIN",
        "ROLE", "COLUMN", "EXCEPTION"
    };
    int nchoices1 = sizeof(choices1) / sizeof(wxString);
    choice_type = new wxChoice(mainPanel, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, nchoices1, choices1, 0);
    fgSizer1->Add(choice_type, 0, wxALL|wxEXPAND, 5);
    button_add_type = new wxButton(mainPanel, ID_button_add_type, _("Add"));
    fgSizer1->Add(button_add_type, 0, wxALL, 5);
    choice_type->SetSelection(0);

    m_staticText2 = new wxStaticText(mainPanel, wxID_ANY, _("Name is"));
    fgSizer1->Add(m_staticText2, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    textctrl_name = new wxTextCtrl(mainPanel, wxID_ANY,
        _("Allows * and ? wildcards"), wxDefaultPosition, wxDefaultSize, 0);
    fgSizer1->Add(textctrl_name, 0, wxALL|wxEXPAND, 5);
    button_add_name = new wxButton(mainPanel, ID_button_add_name, _("Add"));
    fgSizer1->Add(button_add_name, 0, wxALL, 5);

    m_staticText3 = new wxStaticText(mainPanel, wxID_ANY,
        _("Description contains"));
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
    ServerPtrs servers(root->getServers());
    for (ServerPtrs::iterator its = servers.begin(); its != servers.end();
        ++its)
    {
        DatabasePtrs databases((*its)->getDatabases());
        for (DatabasePtrs::iterator itdb = databases.begin();
            itdb != databases.end(); ++itdb)
        {   // we store DB pointer, so we observe in case database is removed
            Database* db = (*itdb).get();
            choice_database->Append((*its)->getName_() + "::" +
                db->getName_(), (void*)db);
            db->attachObserver(this, false);
        }
    }
    choice_database->SetSelection(0);
    fgSizer1->Add(choice_database, 0, wxALL|wxEXPAND, 5);
    button_add_database = new wxButton(mainPanel, ID_button_add_database,
        _("Add"));
    fgSizer1->Add(button_add_database, 0, wxALL, 5);
    leftSizer->Add(fgSizer1, 0, wxEXPAND, 5);

    listctrl_criteria = new AdjustableListCtrl(mainPanel, ID_listctrl_criteria,
        wxLC_REPORT | wxLC_VRULES | wxBORDER_THEME);
    listctrl_criteria->InsertColumn(0, _("Search criteria"));
    listctrl_criteria->InsertColumn(1, _("Value"));
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
        ID_listctrl_results, wxLC_REPORT | wxLC_VRULES | wxBORDER_THEME);
    listctrl_results->InsertColumn(0, _("Database"));
    listctrl_results->InsertColumn(1, _("Type"));
    listctrl_results->InsertColumn(2, _("Name"));
    top_splitter_sizer->Add(listctrl_results, 1, wxEXPAND, 0);

    top_splitter_panel->SetSizer(top_splitter_sizer);
    bottom_splitter_panel = new wxPanel(splitter1, wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxTAB_TRAVERSAL);
    wxBoxSizer *bottom_splitter_sizer = new wxBoxSizer(wxVERTICAL);
    stc_ddl = new wxStyledTextCtrl(bottom_splitter_panel, wxID_ANY,
        wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);
    stc_ddl->SetWrapMode(wxSTC_WRAP_WORD);
    stc_ddl->SetMarginWidth(1, 0);  // turn off the folding margin
    stc_ddl->StyleSetForeground(1, *wxWHITE);
    stc_ddl->StyleSetBackground(1, *wxBLUE);
    stc_ddl->StyleSetForeground(2, *wxWHITE);
    stc_ddl->StyleSetBackground(2, *wxRED);
    stc_ddl->SetText(_("DDL for selected objects"));

    bottom_splitter_sizer->Add(stc_ddl, 1, wxEXPAND, 0);
    bottom_splitter_panel->SetSizer(bottom_splitter_sizer);
    splitter1->SplitHorizontally(top_splitter_panel,bottom_splitter_panel,0);
    rightSizer->Add(splitter1, 1, wxEXPAND, 5);
    innerSizer->Add(rightSizer, 1, wxEXPAND, 5);
    mainPanel->SetSizer(innerSizer);
    mainSizer->Add(mainPanel, 1, wxEXPAND, 0);
    SetSizerAndFit(mainSizer);
    Centre();

    // TODO: size(32, 32) missing for SEARCH icon
    #include "search.xpm"
    wxBitmap bmp = wxBitmap(search_xpm);
    wxIcon icon;
    icon.CopyFromBitmap(bmp);
    SetIcon(icon);
}

void AdvancedSearchFrame::addCriteria(CriteriaItem::Type type, wxString
    value, Database *db)
{
    if (value.IsEmpty())
        return;
    value.MakeUpper();
    if (type == CriteriaItem::ctDDL || type == CriteriaItem::ctDescription)
        value = "*" + value + "*";
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
    // send size event
    wxSizeEvent ev(listctrl_criteria->GetSize());
    listctrl_criteria->GetEventHandler()->AddPendingEvent(ev);
}

void AdvancedSearchFrame::addResult(Database* db, MetadataItem* item)
{
    int index = listctrl_results->GetItemCount();
    listctrl_results->InsertItem(index, db->getServer()->getName_()
        + "::" + db->getName_());
    listctrl_results->SetItem(index, 1, item->getTypeName());
    listctrl_results->SetItem(index, 2, item->getName_());
    results.push_back(item);
    // become observer for that Item in case it gets dropped/deleted...
    item->attachObserver(this, false);
    // send size event
    wxSizeEvent ev(listctrl_results->GetSize());
    listctrl_results->GetEventHandler()->AddPendingEvent(ev);
}

// returns true if "text" matches all criteria of type "type"
bool AdvancedSearchFrame::match(CriteriaItem::Type type, const wxString& text)
{
    for (CriteriaCollection::const_iterator ci =
        searchCriteriaM.lower_bound(type); ci !=
        searchCriteriaM.upper_bound(type); ++ci)
    {
        if (text.Upper().Matches((*ci).second.value))
            return true;
    }
    return false;
}

// OBSERVER functions
void AdvancedSearchFrame::update()
{
}

void AdvancedSearchFrame::subjectRemoved(Subject* subject)
{
    // NOTE: we can't do this dynamic_cast, since this function is called
    //       from ~Subject() and ~Database() has already been called. So we
    //       can't cast to Database (and not even MetadataItem)
    // Database *db = dynamic_cast<Database *>(subject);
    //
    // Since we can't determine what it is by dynamic_cast, we try both:
    // STEP1: Check for database
    for (int i=choice_database->GetCount()-1; i>=0; i--)
    {   // remove from choice_database
        Database *d = (Database *)choice_database->GetClientData(i);
        if (subject == d)
        {
            choice_database->Delete(i);
            choice_database->SetSelection(0);
            break;
        }
    }
    // remove from listctrl_criteria + searchCriteriaM
    bool removed_db = false;
    while (true)    // in case iterators get invalidated on delete
    {
        CriteriaCollection::iterator it = searchCriteriaM.begin();
        while (it != searchCriteriaM.end())
        {
            if ((*it).second.database == subject)
                break;
            ++it;
        }
        if (it == searchCriteriaM.end())    // none to remove
            break;
        removed_db = true;
        searchCriteriaM.erase(it);
    }
    if (removed_db)
        rebuildList();

    // STEP2: Check for criteria
    long itemIndex = 0;
    for (std::vector<MetadataItem *>::iterator it = results.begin();
        it != results.end(); ++it, itemIndex++)
    {
        if ((*it) == subject)
        {
            listctrl_results->DeleteItem(itemIndex);
            results.erase(it);
            break;
        }
    }
}

BEGIN_EVENT_TABLE(AdvancedSearchFrame, wxFrame)
    EVT_LIST_ITEM_ACTIVATED(AdvancedSearchFrame::ID_listctrl_criteria,
        AdvancedSearchFrame::OnListCtrlCriteriaActivate)
    EVT_LIST_ITEM_RIGHT_CLICK(AdvancedSearchFrame::ID_listctrl_results,
        AdvancedSearchFrame::OnListCtrlResultsRightClick)
    EVT_LIST_ITEM_SELECTED(AdvancedSearchFrame::ID_listctrl_results,
        AdvancedSearchFrame::OnListCtrlResultsItemSelected)
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

// remove item on double-click/Enter
void AdvancedSearchFrame::OnListCtrlCriteriaActivate(wxListEvent& event)
{
    long index = event.GetIndex();
    for (CriteriaCollection::iterator it = searchCriteriaM.begin();
        it != searchCriteriaM.end(); ++it)
    {
        if ((*it).second.listIndex == index)
        {
            searchCriteriaM.erase(it);
            break;
        }
    }
    rebuildList();
}

// show DDL for selected item, and select it in main tree
void AdvancedSearchFrame::OnListCtrlResultsItemSelected(wxListEvent& event)
{
    MetadataItem *m = results[event.GetIndex()];
    if (checkbox_ddl->IsChecked())
    {
        CreateDDLVisitor cdv;
        m->acceptVisitor(&cdv);
        wxString sql(cdv.getSql());
        stc_ddl->SetText(sql);
        sql.MakeUpper();
        // highlight the found text
        int color = 0;  // alternate red and blue color for different words
        for (CriteriaCollection::const_iterator ci =
            searchCriteriaM.lower_bound(CriteriaItem::ctDDL); ci !=
            searchCriteriaM.upper_bound(CriteriaItem::ctDDL); ++ci, ++color)
        {
            int len = (*ci).second.value.Length() - 2;
            wxString sfind((*ci).second.value.Mid(1, len));
            int p = -1;
            while (true)
            {
                p = sql.find(sfind, p+1);
                if (p == int(wxString::npos))
                    break;
                stc_ddl->StartStyling(p);
                stc_ddl->SetStyling(len, 1+color%2);
            }
        }
    }
    MainFrame *mf = dynamic_cast<MainFrame *>(GetParent());
    if (mf)
    {
        DBHTreeControl* tree = mf->getTreeCtrl();
        if (tree)
            tree->selectMetadataItem(m);
    }
}

void AdvancedSearchFrame::OnListCtrlResultsRightClick(wxListEvent& event)
{
    MetadataItem *m = results[event.GetIndex()];
    wxMenu MyMenu;
    ContextMenuMetadataItemVisitor cmv(&MyMenu);
    m->acceptVisitor(&cmv);
    PopupMenu(&MyMenu, ScreenToClient(wxGetMousePosition()));
}

void AdvancedSearchFrame::OnCheckboxDdlToggle(wxCommandEvent& event)
{
    if (event.IsChecked())
        splitter1->SplitHorizontally(top_splitter_panel,bottom_splitter_panel);
    else
        splitter1->Unsplit();
}

void AdvancedSearchFrame::OnButtonRemoveClick(wxCommandEvent& WXUNUSED(event))
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

void AdvancedSearchFrame::OnButtonStartClick(wxCommandEvent& WXUNUSED(event))
{
    // build list of databases to search from
    if (searchCriteriaM.count(CriteriaItem::ctDB) == 0)
    {
        wxMessageBox(_("Please select at least one database to search."),
            _("No databases selected"), wxOK|wxICON_WARNING);
        return;
    }

    results.clear();
    listctrl_results->DeleteAllItems();

    // get all types we want to match, for faster lookup later
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

    int database_count = 0;
    for (CriteriaCollection::const_iterator
        cid = searchCriteriaM.lower_bound(CriteriaItem::ctDB);
        cid != searchCriteriaM.upper_bound(CriteriaItem::ctDB); ++cid)
    {
        database_count++;
    }

    // foreach database
    int current = 0;
    ProgressDialog pd(this, _("Searching..."), 2);
    pd.doShow();
    for (CriteriaCollection::const_iterator
        cid = searchCriteriaM.lower_bound(CriteriaItem::ctDB);
        cid != searchCriteriaM.upper_bound(CriteriaItem::ctDB); ++cid)
    {
        if (pd.isCanceled())
            return;
        pd.setProgressPosition(0, 2);
        Database *db = (*cid).second.database;
        if (!db->isConnected() && !connectDatabase(db, this, &pd))
            continue;

        pd.initProgress(_("Searching database: ")+db->getName_(),
            database_count, current++, 1);

        std::vector<MetadataItem *> colls;
        db->getCollections(colls, false);   // false = not system objects
        for (std::vector<MetadataItem *>::iterator col = colls.begin(); col !=
            colls.end(); ++col)
        {
            std::vector<MetadataItem *> ch;
            (*col)->getChildren(ch);
            pd.initProgress(wxEmptyString, ch.size(), 0, 2);
            for (std::vector<MetadataItem *>::iterator it = ch.begin(); it !=
                ch.end(); ++it)
            {
                if (!types.empty() &&
                    types.find((*it)->getType()) == types.end())
                    break;
                pd.setProgressMessage(_("Searching ") + (*col)->getName_() +
                    ": " + (*it)->getName_(), 2);
                pd.stepProgress(1, 2);
                if (pd.isCanceled())
                    return;
                if (searchCriteriaM.count(CriteriaItem::ctName) > 0)
                {
                    wxString name = (*it)->getName_();
                    if (!match(CriteriaItem::ctName, name))
                        continue;
                }
                if (searchCriteriaM.count(CriteriaItem::ctDescription) > 0)
                {
                    wxString desc;
                    if (!(*it)->getDescription(desc))
                        continue;
                    if (!match(CriteriaItem::ctDescription, desc))
                        continue;
                }
                if (searchCriteriaM.count(CriteriaItem::ctDDL) > 0)
                {
                    // TODO: add ProgressDialog, and link it up in here
                    CreateDDLVisitor cdv;
                    (*it)->acceptVisitor(&cdv);
                    if (!match(CriteriaItem::ctDDL, cdv.getSql()))
                        continue;
                }
                if (searchCriteriaM.count(CriteriaItem::ctField) > 0)
                {
                    Relation* r = dynamic_cast<Relation*>(*it);
                    Procedure* p = dynamic_cast<Procedure*>(*it);
                    if (r || p)
                    {
                        (*it)->ensureChildrenLoaded();
                        bool found = false;
                        if (r)
                        {
                            found |= std::any_of(r->begin(), r->end(),
                                                 [this](ColumnPtr i) { return match(CriteriaItem::ctField, i->getName_()); });
                        }
                        if (p)
                        {
                            found |= std::any_of(p->begin(), p->end(),
                                                 [this](ParameterPtr i) { return match(CriteriaItem::ctField, i->getName_()); });
                        }
                        if (!found)     // object doesn't contain that field
                            continue;
                    }
                }
                // everything criteria is matched -> add to results
                addResult(db, *it);
            }
        }
    }
}

void AdvancedSearchFrame::OnButtonAddTypeClick(wxCommandEvent& WXUNUSED(event))
{
    addCriteria(CriteriaItem::ctType, choice_type->GetStringSelection());
}

void AdvancedSearchFrame::OnButtonAddNameClick(wxCommandEvent& WXUNUSED(event))
{
    addCriteria(CriteriaItem::ctName, textctrl_name->GetValue());
    textctrl_name->Clear();
}

void AdvancedSearchFrame::OnButtonAddDescriptionClick(wxCommandEvent& WXUNUSED(event))
{
    addCriteria(CriteriaItem::ctDescription, textctrl_description->GetValue());
    textctrl_description->Clear();
}

void AdvancedSearchFrame::OnButtonAddDDLClick(wxCommandEvent& WXUNUSED(event))
{
    addCriteria(CriteriaItem::ctDDL, textctrl_ddl->GetValue());
    textctrl_ddl->Clear();
}

void AdvancedSearchFrame::OnButtonAddFieldClick(wxCommandEvent& WXUNUSED(event))
{
    addCriteria(CriteriaItem::ctField, textctrl_field->GetValue());
    textctrl_field->Clear();
}

void AdvancedSearchFrame::OnButtonAddDatabaseClick(
    wxCommandEvent& WXUNUSED(event))
{
    if (choice_database->GetSelection() == 0)   // all connected databases
    {
        for (size_t i = 1; i < choice_database->GetCount(); ++i)
        {
            Database* db = (Database *)choice_database->GetClientData(i);
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

