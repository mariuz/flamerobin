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
#ifndef FR_DATAGENERATORFRAME_H
#define FR_DATAGENERATORFRAME_H

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/splitter.h>

#include <map>

#include <ibpp.h>

#include "core/Observer.h"
#include "core/StringUtils.h"
#include "gui/BaseFrame.h"

class Database;
class Column;
class Table;
class DBHTreeControl;
class GeneratorSettings;

class DataGeneratorFrame: public BaseFrame, public Observer
{
    DECLARE_EVENT_TABLE()
protected:
    bool loadingM;  // prevent updates until loaded
    std::map<wxString, GeneratorSettings *> settingsM;
    std::map<wxString, int> tableRecordsM;
    Database* databaseM;
    virtual const wxString getName() const;
    virtual const wxRect getDefaultRect() const;
    void showColumnSettings(bool show);
    GeneratorSettings* getSettings(Column *c);
    void saveSetting(wxTreeItemId item);
    void loadSetting(wxTreeItemId newitem);
    bool loadColumns(const wxString& tableName, wxChoice* c);
    bool sortTables(std::list<Table *>& order);
    void generateData(std::list<Table *>& order);

    void setParam( IBPP::Statement st, int param, GeneratorSettings* gs,
        int recNo);
    void setString(IBPP::Statement st, int param, GeneratorSettings* gs,
        int recNo);

    enum
    {
        ID_button_file = 1000,
        ID_button_save,
        ID_button_load,
        ID_button_generate,
        ID_button_copy,
        ID_checkbox_skip,
        ID_choice_value,
        ID_choice_copy
    };

    wxBoxSizer* rightPanelSizer;
    wxBoxSizer* valueSizer;
    wxBoxSizer* copySizer;

    wxPanel* outerPanel;
    wxSplitterWindow* mainSplitter;
    wxPanel* leftPanel;
    wxStaticText* leftLabel;
    DBHTreeControl* mainTree;
    wxPanel* rightPanel;
    wxStaticText* rightLabel;
    wxStaticText* tableLabel;
    wxStaticText* recordsLabel;
    wxSpinCtrl* spinRecords;
    wxCheckBox* skipCheckbox;
    wxStaticText* columnLabel;
    wxStaticText* valuetypeLabel;
    wxRadioButton* radioSkip;
    wxRadioButton* radioRange;
    wxTextCtrl* rangeText;
    wxRadioButton* radioColumn;
    wxChoice* valueChoice;
    wxChoice* valueColumnChoice;
    wxRadioButton* radioFile;
    wxTextCtrl* fileText;
    wxButton* fileButton;
    wxCheckBox* randomCheckbox;
    wxStaticText* nullLabel;
    wxSpinCtrl* nullSpin;
    wxStaticText* nullPercentLabel;
    wxStaticText* copyLabel;
    wxChoice* copyChoice;
    wxChoice* copyColumnChoice;
    wxButton* copyButton;
    wxButton* saveButton;
    wxButton* loadButton;
    wxButton* generateButton;

    void OnFileButtonClick(wxCommandEvent& event);
    void OnCopyButtonClick(wxCommandEvent& event);
    void OnSaveButtonClick(wxCommandEvent& event);
    void OnLoadButtonClick(wxCommandEvent& event);
    void OnGenerateButtonClick(wxCommandEvent& event);
    void OnSkipCheckboxClick(wxCommandEvent& event);
    void OnTableValueChoiceChange(wxCommandEvent& event);
    void OnTableCopyChoiceChange(wxCommandEvent& event);
    void OnTreeSelectionChanged(wxTreeEvent& event);
private:
    // observer stuff
    virtual void subjectRemoved(Subject* subject);
    virtual void update();
public:
    DataGeneratorFrame(wxWindow* parent, Database* db);
    ~DataGeneratorFrame();
};

#endif
