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

#ifndef FR_INSERTDIALOG_H
#define FR_INSERTDIALOG_H

#include <wx/wx.h>
#include <wx/image.h>

#include <ibpp.h>

#include "gui/BaseDialog.h"
#include "config/LocalSettings.h"

class Column;
class Database;
class DataGridTable;
class InsertedGridRowBuffer;
class ResultsetColumnDef;
// link all column info in the same place:
class InsertColumnInfo
{
public:
    InsertColumnInfo(int gridRow, Column *f,
        ResultsetColumnDef *r, int idx)
        :row(gridRow), column(f), columnDef(r), index(idx)
    {
    };
    int row;    // insert grid row
    Column *column;
    ResultsetColumnDef *columnDef;
    int index;  // column index in dataset
};

class InsertDialog: public BaseDialog
{
public:
    InsertDialog(wxWindow* parent, const wxString& tableName, DataGridTable *,
        IBPP::Statement& st, Database *db);
    virtual ~InsertDialog();
    void OnOkButtonClick(wxCommandEvent& event);
    void OnCancelButtonClick(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnGridCellChange(wxGridEvent& event);

    void OnCellEditorCreated(wxGridEditorCreatedEvent& event);
    void OnEditorKeyDown(wxKeyEvent& event);

    enum { ID_Choice = 1001, ID_Grid };

private:
    Database *databaseM;
    LocalSettings localSetM;
    void storeValues();
    void preloadSpecialColumns();
    IBPP::Statement& statementM;
    std::vector<InsertColumnInfo> columnsM;
    DataGridTable *gridTableM;
    InsertedGridRowBuffer *bufferM;
    wxString tableNameM;
    void updateControls(int row);
    void setStringOption(InsertColumnInfo& ici, const wxString& s);
    void set_properties();
    void do_layout();

protected:
    wxStaticText* labelFieldName;
    wxStaticText* labelDataType;
    wxStaticText* labelModifiers;
    wxStaticText* labelValue;
    wxCheckBox* checkboxInsertAnother;
    wxGrid *gridM;

    wxButton* button_ok;
    wxButton* button_cancel;
    virtual const wxString getName() const;
    virtual bool getConfigStoresWidth() const;
    virtual bool getConfigStoresHeight() const;

    DECLARE_EVENT_TABLE()
};

#endif
