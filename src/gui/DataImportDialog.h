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

#ifndef FR_DATAIMPORTDIALOG_H
#define FR_DATAIMPORTDIALOG_H

#include "gui/BaseDialog.h"
#include "core/DataTransferHelper.h"

class Database;
class Table;
class wxChoice;
class wxSpinCtrl;
class wxScrolledWindow;

class DataImportDialog: public BaseDialog
{
public:
    DataImportDialog(wxWindow* parent, Database* db, Table* table);

private:
    Database* dbM;
    Table* tableM;

    std::vector<wxString> columnNamesM;
    std::vector<wxString> columnTypesM;
    std::vector<bool> columnComputedM;
    std::vector<wxChoice*> choiceMappingsM;
    std::vector<wxString> currentFileFieldsM;

    wxTextCtrl* textctrl_file;
    wxButton* button_browse;
    wxChoice* choice_format;

    wxScrolledWindow* scroll_mappings;
    wxFlexGridSizer* sizer_mappings;

    wxButton* button_automap;
    wxButton* button_clear_mapping;

    wxSpinCtrl* spin_batch_size;
    wxChoice* choice_error_handling;

    wxButton* button_import;
    wxButton* button_cancel;

    void createControls();
    void layoutControls();
    void setControlProperties();

    void loadFileFields();
    void updateMappingChoices();
    void runAutoMapping();

    void OnBrowse(wxCommandEvent& event);
    void OnFileText(wxCommandEvent& event);
    void OnFormatChanged(wxCommandEvent& event);
    void OnAutoMap(wxCommandEvent& event);
    void OnClearMapping(wxCommandEvent& event);
    void OnImport(wxCommandEvent& event);

protected:
    virtual const wxString getName() const override;

    enum
    {
        ID_textctrl_file = 10200,
        ID_button_browse,
        ID_choice_format,
        ID_button_automap,
        ID_button_clear_mapping,
        ID_spin_batch_size,
        ID_choice_error_handling,
        ID_button_import
    };

    DECLARE_EVENT_TABLE()
};

#endif // FR_DATAIMPORTDIALOG_H
