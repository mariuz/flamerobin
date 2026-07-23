/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#ifndef FR_SCHEMACOMPAREDIALOG_H
#define FR_SCHEMACOMPAREDIALOG_H

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>
#include "gui/BaseDialog.h"
#include "gui/SchemaDiff.h"
#include "metadata/database.h"

class SqlEditor;

class SchemaCompareDialog: public BaseDialog
{
private:
    Database* initialSourceDbM;

    wxChoice* choice_source_db;
    wxChoice* choice_target_db;

    wxCheckBox* check_tables;
    wxCheckBox* check_columns;
    wxCheckBox* check_indices;
    wxCheckBox* check_views_procs;
    wxCheckBox* check_domains_gens;
    wxCheckBox* check_drop_extra;

    wxButton* button_compare;
    wxListCtrl* list_diffs;
    SqlEditor* editor_sql;

    wxButton* button_open_editor;
    wxButton* button_copy_script;
    wxButton* button_close;

    std::vector<DatabasePtr> availableDatabasesM;
    std::vector<SchemaDiffItem> lastDiffItemsM;
    wxString generatedScriptM;

    void populateDatabaseChoices();
    void updateDiffList(const std::vector<SchemaDiffItem>& items);

    void OnCompareButtonClick(wxCommandEvent& event);
    void OnOpenEditorClick(wxCommandEvent& event);
    void OnCopyScriptClick(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
public:
    SchemaCompareDialog(wxWindow* parent, Database* sourceDb = nullptr);
    virtual ~SchemaCompareDialog();
};

#endif
