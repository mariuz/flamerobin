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

#ifndef FR_DATAEXPORTDIALOG_H
#define FR_DATAEXPORTDIALOG_H

#include "gui/BaseDialog.h"
#include "core/DataTransferHelper.h"

class Database;
class Relation;
class DataGrid;
class wxCheckListBox;
class wxRadioBox;

class DataExportDialog: public BaseDialog
{
public:
    DataExportDialog(wxWindow* parent, Database* db, Relation* relation);
    DataExportDialog(wxWindow* parent, DataGrid* grid);

private:
    Database* dbM;
    Relation* relationM;
    DataGrid* gridM;

    wxRadioBox* radio_format;
    wxCheckListBox* check_list_columns;
    wxButton* button_select_all;
    wxButton* button_deselect_all;
    wxTextCtrl* textctrl_file;
    wxButton* button_browse;
    wxChoice* choice_encoding;
    wxCheckBox* checkbox_pretty;
    wxCheckBox* checkbox_base64;
    wxChoice* choice_null_handling;
    wxStaticText* label_root_tag;
    wxTextCtrl* textctrl_root_tag;
    wxStaticText* label_row_tag;
    wxTextCtrl* textctrl_row_tag;
    wxButton* button_export;
    wxButton* button_cancel;

    void createControls();
    void layoutControls();
    void setControlProperties();
    void updateFormatUI();

    void OnFormatChanged(wxCommandEvent& event);
    void OnBrowse(wxCommandEvent& event);
    void OnSelectAll(wxCommandEvent& event);
    void OnDeselectAll(wxCommandEvent& event);
    void OnExport(wxCommandEvent& event);

protected:
    virtual const wxString getName() const override;

    enum
    {
        ID_radio_format = 10100,
        ID_check_list_columns,
        ID_button_select_all,
        ID_button_deselect_all,
        ID_textctrl_file,
        ID_button_browse,
        ID_choice_encoding,
        ID_checkbox_pretty,
        ID_checkbox_base64,
        ID_choice_null_handling,
        ID_textctrl_root_tag,
        ID_textctrl_row_tag,
        ID_button_export
    };

    DECLARE_EVENT_TABLE()
};

#endif // FR_DATAEXPORTDIALOG_H
