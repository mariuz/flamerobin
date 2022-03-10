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
#ifndef FR_ADVANCEDSEARCHFRAME_H
#define FR_ADVANCEDSEARCHFRAME_H

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>

#include <map>

#include "core/Observer.h"
#include "gui/BaseFrame.h"

class CriteriaItem
{
public:
    enum Type { ctType, ctName, ctDescription, ctDDL, ctField, ctDB };
    wxString value;
    Database *database; // only used for ctDB type
    long listIndex;
    CriteriaItem(const wxString& v, Database *db)
        :value(v), database(db), listIndex(-1)
    {
    }
    bool operator==(const CriteriaItem& other) const
    {
        return (value == other.value && database == other.database);
    };
    static wxString getTypeString(Type type)
    {
        switch (type)
        {
            case ctType:        return _("Type is");
            case ctName:        return _("Name is");
            case ctDescription: return _("Description contains");
            case ctDDL:         return _("DDL contains");
            case ctField:       return _("Has field");
            case ctDB:          return _("In database");
        };
        return wxEmptyString;
    }
};

class Database;
class MetadataItem;
class Root;

class AdjustableListCtrl;   // declaration in cpp file
class MainFrame;
class wxStyledTextCtrl;

class AdvancedSearchFrame : public BaseFrame, public Observer
{
private:
    typedef std::multimap<CriteriaItem::Type, CriteriaItem> CriteriaCollection;
    CriteriaCollection searchCriteriaM;
    void addCriteria(CriteriaItem::Type type, wxString value,
        Database *db = 0);
    void rebuildList();
    std::vector<MetadataItem *> results;
    void addResult(Database* db, MetadataItem* item);
    bool match(CriteriaItem::Type type, const wxString& text);

    // observer stuff
    virtual void subjectRemoved(Subject* subject);
    virtual void update();

protected:
    wxPanel *mainPanel;
    wxStaticText *m_staticText1;
    wxChoice *choice_type;
    wxButton *button_add_type;
    wxStaticText *m_staticText2;
    wxTextCtrl *textctrl_name;
    wxButton *button_add_name;
    wxStaticText *m_staticText3;
    wxTextCtrl *textctrl_description;
    wxButton *button_add_description;
    wxStaticText *m_staticText4;
    wxTextCtrl *textctrl_ddl;
    wxButton *button_add_ddl;
    wxStaticText *m_staticText5;
    wxTextCtrl *textctrl_field;
    wxButton *button_add_field;
    wxStaticText *m_staticText6;
    wxChoice *choice_database;
    wxButton *button_add_database;
    AdjustableListCtrl *listctrl_criteria;
    wxButton *button_remove;
    wxButton *button_search;
    wxStaticText *label_search_results;
    wxCheckBox *checkbox_ddl;
    wxSplitterWindow *splitter1;
    wxPanel *top_splitter_panel;
    AdjustableListCtrl *listctrl_results;
    wxPanel *bottom_splitter_panel;
    wxStyledTextCtrl *stc_ddl;

public:
    AdvancedSearchFrame(MainFrame* parent, RootPtr root);
    enum
    {
        ID_button_remove=100,
        ID_button_start,
        ID_button_add_type,
        ID_button_add_name,
        ID_button_add_description,
        ID_button_add_ddl,
        ID_button_add_field,
        ID_button_add_database,
        ID_checkbox_ddl,
        ID_listctrl_criteria,
        ID_listctrl_results
    };

    // events
    void OnCheckboxDdlToggle(wxCommandEvent& event);
    void OnButtonRemoveClick(wxCommandEvent& event);
    void OnButtonStartClick(wxCommandEvent& event);
    void OnButtonAddTypeClick(wxCommandEvent& event);
    void OnButtonAddNameClick(wxCommandEvent& event);
    void OnButtonAddDescriptionClick(wxCommandEvent& event);
    void OnButtonAddDDLClick(wxCommandEvent& event);
    void OnButtonAddFieldClick(wxCommandEvent& event);
    void OnButtonAddDatabaseClick(wxCommandEvent& event);
    void OnListCtrlResultsRightClick(wxListEvent& event);
    void OnListCtrlResultsItemSelected(wxListEvent& event);
    void OnListCtrlCriteriaActivate(wxListEvent& event);
    DECLARE_EVENT_TABLE()
};

#endif
