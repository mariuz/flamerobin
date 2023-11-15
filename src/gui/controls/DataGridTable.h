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

#ifndef FR_DATAGRIDTABLE_H
#define FR_DATAGRIDTABLE_H

#include <wx/wx.h>
#include <wx/grid.h>

#include <ibpp.h>

#include "gui/controls/DataGridRows.h"
#include "gui/FRStyleManager.h"

class Column;
class Database;
class DataGridCell;
class ResultsetColumnDef;
class DataGridRowBuffer;
class ProgressIndicator;

BEGIN_DECLARE_EVENT_TYPES()
    // this event is sent after new rows have been fetched
    DECLARE_LOCAL_EVENT_TYPE(wxEVT_FRDG_ROWCOUNT_CHANGED, 42)
    // this event is sent when statement is executed because of user edits
    DECLARE_LOCAL_EVENT_TYPE(wxEVT_FRDG_STATEMENT, 43)
    // this event is sent to cause the attribute cache to be invalidated
    // after a field value has changed
    DECLARE_LOCAL_EVENT_TYPE(wxEVT_FRDG_INVALIDATEATTR, 44)
END_DECLARE_EVENT_TYPES()

class DataGridTable: public wxGridTableBase
{
private:
    bool allRowsFetchedM;
    bool fetchAllRowsM;
    unsigned maxRowToFetchM;
    bool readOnlyM;
    bool canInsertRowsIsSetM;
    bool canInsertRowsM;

    wxGridCellAttr* cellAttriM;
    DataGridRows rowsM;

    bool nullFlagM;

    Database *databaseM;
    IBPP::Statement& statementM;
    wxMBConv* charsetConverterM;

    int getStatementColCount();
    bool isValidCellPos(int row, int col);
public:
    DataGridTable(IBPP::Statement& s, Database* db);
    ~DataGridTable();

    bool canFetchMoreRows();
    void fetch();
    void fetchOne();
    void addRow(DataGridRowBuffer *buffer, const wxString& sql);
    wxString getCellValue(int row, int col);
    wxString getCellValueForInsert(int row, int col);
    wxString getCellValueForCSV(int row, int col, const wxChar& textDelimiter);
    bool getFetchAllRows();

    // TODO: these should be replaced with a better function that covers all
    wxString getTableName();
    void getTableNames(wxArrayString& tables);
    typedef std::map<int, std::pair<ResultsetColumnDef*,Column *> > FieldSet;
    void getFields(const wxString& table, FieldSet& fields);
    Database *getDatabase();

    void initialFetch(bool readonly);
    bool isNullableColumn(int col);
    bool isNullCell(int row, int col);
    bool isNumericColumn(int col);
    bool isReadonlyColumn(int col);
    bool isBlobColumn(int col, bool* pIsTextual = 0);
    bool needsMoreRowsFetched();
    void setFetchAllRecords(bool fetchall);
    bool canInsertRows();
    bool canRemoveRow(size_t row);

    void setNullFlag(bool isNull);

    // methods of wxGridTableBase
    virtual void Clear();
    virtual wxGridCellAttr* GetAttr(int row, int col,
        wxGridCellAttr::wxAttrKind kind);
    virtual wxString GetColLabelValue(int col);

    // pure virtual methods of wxGridTableBase
    virtual int GetNumberCols();
    virtual int GetNumberRows();
    virtual wxString GetValue(int row, int col);
    virtual bool IsEmptyCell(int row, int col);
    virtual void SetValue(int row, int col, const wxString& value);
    virtual bool DeleteRows(size_t pos, size_t numRows);

    IBPP::Blob* getBlob(unsigned row, unsigned col, bool validateBlob);
    DataGridRowsBlob setBlobPrepare(unsigned row, unsigned col);
    void setBlob(DataGridRowsBlob &b);
    void setValueToNull(int row, int col);
    // BLOBs can be huge, so we don't use SetValue for that
    void importBlobFile(const wxString& filename, int row, int col,
        ProgressIndicator *pi = 0);
    void exportBlobFile(const wxString& filename, int row, int col,
        ProgressIndicator *pi = 0);
};

#endif
