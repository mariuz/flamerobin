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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
  #include "wx/wx.h"
#endif

#include <wx/grid.h>

#include <algorithm>
#include <set>

#include "config/Config.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "gui/controls/DataGridRows.h"
#include "gui/controls/DataGridTable.h"
#include "engine/db/IDatabase.h"
#include "engine/db/ITransaction.h"
#include "engine/db/IStatement.h"
#include "engine/db/IBlob.h"
#include "gui/AdvancedMessageDialog.h"
#include "gui/FRLayoutConfig.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/table.h"

DataGridTable::DataGridTable(IBPP::Statement& s, Database* db)
    : wxGridTableBase(), statementM(s), statementDALM(), databaseM(db),
        nullFlagM(false), rowsM(db)
{
    allRowsFetchedM = false;
    fetchAllRowsM = false;
    readOnlyM = false;
    canInsertRowsIsSetM = false;
    canInsertRowsM = false;
    config().getValue("GridFetchAllRecords", fetchAllRowsM);
    maxRowToFetchM = 100;
    cellAttriM = new wxGridCellAttr();
}

DataGridTable::DataGridTable(fr::IStatementPtr s, Database* db)
    : wxGridTableBase(), statementM(), statementDALM(s), databaseM(db),
        nullFlagM(false), rowsM(db)
{
    allRowsFetchedM = false;
    fetchAllRowsM = false;
    readOnlyM = false;
    canInsertRowsIsSetM = false;
    canInsertRowsM = false;
    config().getValue("GridFetchAllRecords", fetchAllRowsM);
    maxRowToFetchM = 100;
    cellAttriM = new wxGridCellAttr();
}

DataGridTable::~DataGridTable()
{
    Clear();
    cellAttriM->DecRef();
}

void DataGridTable::setNullFlag(bool isNull)
{
    nullFlagM = isNull;
}

// implementation methods
bool DataGridTable::canFetchMoreRows()
{
    // this will also handle a closed result set
    return !allRowsFetchedM && getStatementColCount() > 0;
}

void DataGridTable::Clear()
{
    nullFlagM = false;

    allRowsFetchedM = true;
    fetchAllRowsM = false;
    canInsertRowsIsSetM = false;
    config().getValue("GridFetchAllRecords", fetchAllRowsM);

    unsigned oldCols = rowsM.getRowFieldCount();
    unsigned oldRows = rowsM.getRowCount();
    rowsM.clear();

    if (GetView() && oldRows > 0)
    {
        wxGridTableMessage rowMsg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED,
            0, oldRows);
        GetView()->ProcessTableMessage(rowMsg);
    }
    if (GetView() && oldCols > 0)
    {
        wxGridTableMessage colMsg(this, wxGRIDTABLE_NOTIFY_COLS_DELETED,
            0, oldCols);
        GetView()->ProcessTableMessage(colMsg);
    }
}

void DataGridTable::fetchOne()
{
    if (statementDALM)
        rowsM.addRow(statementDALM);
    else
        rowsM.addRow(statementM);
    allRowsFetchedM = true;

    if (GetView())   // notify the grid
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1);
        GetView()->ProcessTableMessage(msg);
        // used in frame to update status bar
        wxCommandEvent evt(wxEVT_FRDG_ROWCOUNT_CHANGED, GetView()->GetId());
        evt.SetExtraLong(1);
        wxPostEvent(GetView(), evt);
    }
}

void DataGridTable::fetch()
{
    if (!canFetchMoreRows())
        return;

    wxLogDebug("DataGridTable::fetch starting.");
    // fetch the first 100 rows no matter how long it takes
    unsigned oldRows = rowsM.getRowCount();
    bool initial = oldRows == 0;
    // fetch more rows until maxRowToFetchM reached or 100 ms elapsed
    wxLongLong startms = ::wxGetLocalTimeMillis();
    do
    {
        try
        {
            if (statementDALM)
            {
                if (!statementDALM->fetch())
                    allRowsFetchedM = true;
            }
            else
            {
                if (!statementM->Fetch())
                    allRowsFetchedM = true;
            }
        }
        catch (const std::exception& e)
        {
            allRowsFetchedM = true;
            wxLogError("Error fetching row %u: %s", rowsM.getRowCount(), e.what());
            ::wxMessageBox(wxString::FromUTF8(e.what()),
                _("A database error occurred."), wxOK|wxICON_ERROR);
        }
        catch (...)
        {
            allRowsFetchedM = true;
            wxLogError("Unknown error fetching row %u.", rowsM.getRowCount());
            ::wxMessageBox(_("A system error occurred!"), _("Error"),
                wxOK|wxICON_ERROR);
        }
        if (allRowsFetchedM)
            break;

        if (statementDALM)
            rowsM.addRow(statementDALM);
        else
            rowsM.addRow(statementM);

        if (!initial && (::wxGetLocalTimeMillis() - startms > 100))
            break;
    }
    while ((fetchAllRowsM && !initial) || rowsM.getRowCount() < maxRowToFetchM);

    unsigned newRows = rowsM.getRowCount() - oldRows;
    wxLogDebug("DataGridTable::fetch finished. Fetched %u new rows.", newRows);

    if (newRows > 0 && GetView())   // notify the grid
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
            newRows);
        GetView()->ProcessTableMessage(msg);
        // used in frame to update status bar
        wxCommandEvent evt(wxEVT_FRDG_ROWCOUNT_CHANGED, GetView()->GetId());
        evt.SetExtraLong(rowsM.getRowCount());
        wxPostEvent(GetView(), evt);
    }
}

void DataGridTable::addRow(DataGridRowBuffer *buffer, const wxString& sql)
{
    rowsM.addRow(buffer);
    if (GetView())  // notify the grid
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1);
        GetView()->ProcessTableMessage(msg);
        // used in frame to update status bar
        wxCommandEvent evt(wxEVT_FRDG_ROWCOUNT_CHANGED, GetView()->GetId());
        evt.SetExtraLong(rowsM.getRowCount());
        wxPostEvent(GetView(), evt);

        // used in frame to show executed statements
        wxCommandEvent evt2(wxEVT_FRDG_STATEMENT, GetView()->GetId());
        evt2.SetString(sql);
        wxPostEvent(GetView(), evt2);
    }
}

wxGridCellAttr* DataGridTable::GetAttr(int row, int col,
    wxGridCellAttr::wxAttrKind kind)
{
    DataGridFieldInfo info;
    if (!rowsM.getFieldInfo(row, col, info))
        return wxGridTableBase::GetAttr(row, col, kind);

    bool useAttri = readOnlyM || info.rowInserted || info.rowDeleted
        || info.fieldReadOnly || info.fieldModified || info.fieldNull
        || info.fieldNA || info.fieldNumeric || info.fieldBlob;
    if (!useAttri)
        return wxGridTableBase::GetAttr(row, col, kind);

    // text colour
    wxColour textCol;
    if (info.fieldNull || info.fieldNA)
        textCol = *wxRED;
    else if (info.fieldModified)
        textCol = *wxBLUE;
    else
        //textCol = stylerManager().getDefaultStyle()->getfgColour() != 0 ? stylerManager().getDefaultStyle()->getfgColour() : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
        textCol = stylerManager().getDefaultStyle()->getfgColor();
    cellAttriM->SetTextColour(textCol);

    // background colour
    wxColour bgCol;
    if (info.rowDeleted)
        bgCol = wxColour(255, 208, 208);
    else if (info.rowInserted)
        bgCol = wxColour(235, 255, 200);
    else if (readOnlyM || info.fieldReadOnly || info.fieldBlob)
        bgCol = frlayoutconfig().getReadonlyColour();
    else
        //bgCol = stylerManager().getDefaultStyle()->getbgColour() != 0 ? stylerManager().getDefaultStyle()->getbgColour() : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
        bgCol = stylerManager().getDefaultStyle()->getbgColor();
    cellAttriM->SetBackgroundColour(bgCol);

    // text alignment
    if (info.fieldNumeric)
        cellAttriM->SetAlignment(wxALIGN_RIGHT, wxALIGN_TOP);
    else
        cellAttriM->SetAlignment(wxALIGN_LEFT, wxALIGN_TOP);

    cellAttriM->SetReadOnly(info.fieldReadOnly || info.fieldBlob);

    cellAttriM->SetOverflow(false);

    cellAttriM->IncRef();
    return cellAttriM;
}

wxString DataGridTable::getCellValue(int row, int col)
{
    if (!isValidCellPos(row, col))
        return wxEmptyString;

    if (rowsM.isFieldNA(row, col))
        return "N/A";
    if (rowsM.isFieldNull(row, col))
        return "[null]";
    return rowsM.getFieldValue(row, col);
}

wxString DataGridTable::getCellValueForInsert(int row, int col)
{
    if (!isValidCellPos(row, col) || rowsM.isFieldNA(row, col))
        return wxEmptyString;

    if (rowsM.isFieldNull(row, col))
        return "NULL";
    // return quoted text, but escape embedded quotes
    wxString s(rowsM.getFieldValue(row, col));
    // Normalize decimal separator for SQL: Firebird always uses dot.
    // If the display value uses comma as decimal separator (locale-
    // dependent formatting) and has no dot, replace comma with dot.
    // Note: values with both comma and dot (e.g. "1,234.56") are left
    // unchanged as the dot is already the decimal separator.
    if (rowsM.isColumnNumeric(col)
            && s.Contains(",") && !s.Contains("."))
        s.Replace(",", ".");
    s.Replace("'", "''");
    return "'" + s + "'";
}

wxString DataGridTable::getCellValueForCSV(int row, int col,
    const wxChar& textDelimiter)
{
    if (!isValidCellPos(row, col) || rowsM.isFieldNA(row, col))
        return wxEmptyString;

    const wxString sTextDelim =
        (textDelimiter != '\0') ? wxString(textDelimiter) : wxString("");

    if (rowsM.isFieldNull(row, col))
        return sTextDelim + "NULL" + sTextDelim;
    wxString s(rowsM.getFieldValue(row, col));
    if (rowsM.isColumnNumeric(col))
        return s;

    // wxTextOutputStream (which is used to write the CSV file) will convert
    // '\n' to the proper EOL sequence while writing the stream
    // so make sure '\r' isn't doubled on Windows
    s.Replace("\r\n", "\n");

    if (textDelimiter == '\0')
        return s;
    // return quoted text, but escape embedded quotes
    s.Replace(sTextDelim, sTextDelim + sTextDelim);
    return sTextDelim + s + sTextDelim;
}

wxString DataGridTable::GetColLabelValue(int col)
{
    return rowsM.getRowFieldName(col);
}

bool DataGridTable::getFetchAllRows()
{
    return fetchAllRowsM;
}

int DataGridTable::GetNumberCols()
{
    return rowsM.getRowFieldCount();
}

int DataGridTable::GetNumberRows()
{
    return rowsM.getRowCount();
}

int DataGridTable::getStatementColCount()
{
    if (statementDALM)
    {
        // Support any statement that returns columns (Select, Insert/Update/Delete Returning, ExecProcedure)
        return statementDALM->getColumnCount();
    }

    if (statementM == 0)
        return 0;
    switch (statementM->Type())
    {
        case IBPP::stSelect:
        case IBPP::stSelectUpdate:
            return statementM->Columns();
        default:
            return 0;
    }
}

wxString DataGridTable::getTableName()
{
    if (getStatementColCount() == 0)
        return wxEmptyString;
    if (statementDALM)
    {
        return std2wxIdentifier(statementDALM->getColumnTable(0),
            databaseM->getCharsetConverter());
    }
    return std2wxIdentifier(statementM->ColumnTable(1),
        databaseM->getCharsetConverter());
}

void DataGridTable::getTableNames(wxArrayString& tables)
{
    int colCount = getStatementColCount();
    if (colCount == 0)
    {
        tables.clear();
        return;
    }
    for (int i = 0; i < colCount; i++)
    {
        wxString tn;
        wxString cn;
        if (statementDALM)
        {
            tn = std2wxIdentifier(statementDALM->getColumnTable(i),
                databaseM->getCharsetConverter());
            cn = std2wxIdentifier(statementDALM->getColumnName(i),
                databaseM->getCharsetConverter());
        }
        else
        {
            tn = std2wxIdentifier(statementM->ColumnTable(i + 1),
                databaseM->getCharsetConverter());
            cn = std2wxIdentifier(statementM->ColumnName(i + 1),
                databaseM->getCharsetConverter());
        }

        if (wxNOT_FOUND == tables.Index(tn))
        {
            // check if table exists in metadata
            Table *t = dynamic_cast<Table *>(databaseM->findRelation(
                Identifier(tn)));
            if (!t)
                continue;
            t->ensureChildrenLoaded();

            // check if table's column is 'real'
            ColumnPtr c = t->findColumn(cn);
            if (c && c->getComputedSource().empty())
            {
                tables.Add(tn);
                break;
            }
        }
    }
}

void DataGridTable::getFields(const wxString& table,
    DataGridTable::FieldSet& flds)
{
    int colCount = getStatementColCount();
    if (colCount == 0)
        return;
    Table *t = dynamic_cast<Table *>(databaseM->findRelation(
        Identifier(table)));
    if (!t)
        return;
    t->ensureChildrenLoaded();

    typedef std::pair<ResultsetColumnDef *, int> TempPair;
    typedef std::map<Column *, TempPair> TempMap;
    TempMap fields;
    for (int i = 0; i < colCount; i++)
    {
        wxString tn;
        wxString cn;
        if (statementDALM)
        {
            tn = std2wxIdentifier(statementDALM->getColumnTable(i),
                databaseM->getCharsetConverter());
            cn = std2wxIdentifier(statementDALM->getColumnName(i),
                databaseM->getCharsetConverter());
        }
        else
        {
            tn = std2wxIdentifier(statementM->ColumnTable(i + 1),
                databaseM->getCharsetConverter());
            cn = std2wxIdentifier(statementM->ColumnName(i + 1),
                databaseM->getCharsetConverter());
        }

        if (tn != table)
            continue;
        // check if field exists in the table (and is not computed)
        ColumnPtr c = t->findColumn(cn);
        if (c && c->getComputedSource().empty()
            && fields.find(c.get()) == fields.end())
        {
            TempPair p(rowsM.getColumnDef(i), i);
            fields.insert(std::pair<Column *, TempPair> (c.get(), p));
        }
    }

    // we have item list sorted by column *, but user probably expects the
    // same order as in the grid, so we sort it
    typedef std::pair<ResultsetColumnDef *, Column *> FinalPair;
    for (TempMap::iterator it = fields.begin(); it != fields.end(); ++it)
    {
        FinalPair p((*it).second.first, (*it).first);
        flds.insert(std::pair<int,FinalPair>((*it).second.second, p));
    }
}

Database *DataGridTable::getDatabase()
{
    return databaseM;
}

wxString DataGridTable::GetValue(int row, int col)
{
    if (!isValidCellPos(row, col))
        return wxEmptyString;

    // keep between 200 and 250 more rows fetched for better responsiveness
    // (but make the count of fetched rows a multiple of 50)
    unsigned maxRowToFetch = 50 * (row / 50 + 5);
    if (maxRowToFetchM < maxRowToFetch)
        maxRowToFetchM = maxRowToFetch;

    if (rowsM.isFieldNA(row, col))
        return "N/A";
    if (rowsM.isFieldNull(row, col))
        return "[null]";
    // limit returned string to first line (speeds up output in grid)
    wxString s(rowsM.getFieldValue(row, col));
    size_t eol = s.find_first_of("\r\n");
    if (eol != wxString::npos)
        s.erase(eol);
    return s;
}

void DataGridTable::initialFetch(bool readonly)
{
    Clear();
    allRowsFetchedM = false;
    readOnlyM = readonly;
    canInsertRowsIsSetM = false;
    canInsertRowsM = false;
    maxRowToFetchM = 100;

    try
    {
        if (statementDALM)
            rowsM.initialize(statementDALM);
        else
            rowsM.initialize(statementM);
    }
    catch (std::exception& e)
    {
        ::wxMessageBox(wxString::FromUTF8(e.what()),
            _("A database error occurred."), wxOK | wxICON_ERROR);
    }
    catch (...)
    {
        ::wxMessageBox(_("A system error occurred!"), _("Error"),
            wxOK | wxICON_ERROR);
    }

    if (GetView())
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_APPENDED,
            rowsM.getRowFieldCount());
        GetView()->ProcessTableMessage(msg);
    }

    bool execProc = false;
    if (statementDALM)
        execProc = (statementDALM->getType() == fr::StatementType::ExecProcedure);
    else
        execProc = (statementM->Type() == IBPP::stExecProcedure);

    if (execProc)
        fetchOne();
    else
        fetch();
}

bool DataGridTable::IsEmptyCell(int row, int col)
{
    return !isValidCellPos(row, col);
}

bool DataGridTable::isNullableColumn(int col)
{
    return rowsM.isColumnNullable(col);
}

bool DataGridTable::isNullCell(int row, int col)
{
    return rowsM.isFieldNull(row, col);
}

bool DataGridTable::isNumericColumn(int col)
{
    return rowsM.isColumnNumeric(col);
}

bool DataGridTable::isReadonlyColumn(int col)
{
    return readOnlyM || rowsM.isColumnReadonly(col);
}

bool DataGridTable::isValidCellPos(int row, int col)
{
    return (row >= 0 && col >= 0 && row < (int)rowsM.getRowCount()
        && col < (int)rowsM.getRowFieldCount());
}

bool DataGridTable::canInsertRows()
{
    if (!canInsertRowsIsSetM)
    {
        wxArrayString tables;
        getTableNames(tables);
        canInsertRowsIsSetM = true;
        canInsertRowsM = tables.GetCount() > 0;
    }
    return canInsertRowsM;
}

bool DataGridTable::canRemoveRow(size_t row)
{
    return rowsM.canRemoveRow(row);
}

bool DataGridTable::needsMoreRowsFetched()
{
    if (allRowsFetchedM)
        return false;
    // true if all rows are to be fetched, or more rows should be cached
    // for more responsive grid scrolling
    return (fetchAllRowsM || rowsM.getRowCount() < maxRowToFetchM);
}

void DataGridTable::setFetchAllRecords(bool fetchall)
{
    fetchAllRowsM = fetchall;
}

fr::IBlobPtr DataGridTable::getBlob(unsigned row, unsigned col, bool validateBlob)
{
    return rowsM.getBlob(row, col, validateBlob);
}

DataGridRowsBlob DataGridTable::setBlobPrepare(unsigned row, unsigned col)
{
    return rowsM.setBlobPrepare(row, col);
}

void DataGridTable::setBlob(DataGridRowsBlob &b)
{
    rowsM.setBlob(b);
}

void DataGridTable::importBlobFile(const wxString& filename, int row, int col,
    ProgressIndicator *pi)
{
    rowsM.importBlobFile(filename, row, col, pi);
}

void DataGridTable::exportBlobFile(const wxString& filename, int row, int col,
    ProgressIndicator *pi)
{
    rowsM.exportBlobFile(filename, row, col, pi);
}

bool DataGridTable::isBlobColumn(int col, bool* pIsTextual)
{
    return rowsM.isBlobColumn(col, pIsTextual);
}

void DataGridTable::SetValue(int row, int col, const wxString& value)
{
    // We need explicit exception handling here since wxGrid gets
    // into inconsistent state if exeption is thrown from here
    // (as this is not regular event-handler, but a virtual function)
    // An exception may be thrown when user string cannot be converted
    // to the actual column's data type, or when Firebird rejects the
    // UPDATE statement. See bug report #1882666 at sf.net.
    try
    {
        wxString statement = rowsM.setFieldValue(row, col, value,
            nullFlagM);
        nullFlagM = false;  // reset

        if (wxGrid* grid = GetView())
        {
            // used in frame to show executed statements
            wxCommandEvent evt(wxEVT_FRDG_STATEMENT, grid->GetId());
            evt.SetString(statement);
            wxPostEvent(grid, evt);

            // used in frame to repaint cell (text color may have changed)
            wxCommandEvent evt2(wxEVT_FRDG_INVALIDATEATTR, grid->GetId());
            wxPostEvent(grid, evt2);
        }
    }
    catch (const FRError& err)
    {
        showErrorDialog(wxGetTopLevelParent(wxGetActiveWindow()),
            _("Invalid data"), err.what(),
            AdvancedMessageDialogButtonsOk());
    }
    catch (const IBPP::Exception& e)
    {
        showErrorDialog(wxGetTopLevelParent(wxGetActiveWindow()),
            _("Database error"), e.what(),
            AdvancedMessageDialogButtonsOk());
    }
    catch (...)
    {
        showErrorDialog(wxGetTopLevelParent(wxGetActiveWindow()),
            _("System error"), _("Unhandled exception"),
            AdvancedMessageDialogButtonsOk());
    }
}

void DataGridTable::setValueToNull(int row, int col)
{
    setNullFlag(true);
    SetValue(row, col, "[null]");
    if (isBlobColumn(col,0))
    {
        // set blob to null
        DataGridRowsBlob b;
        b.blob = 0;
        b.col  = col;
        b.row  = row;
        b.st   = statementM;
        rowsM.setBlob(b);
    }
}

bool DataGridTable::DeleteRows(size_t pos, size_t numRows)
{
    // Needs explicit exception handling (see comment for SetValue)
    try
    {
        // remove rows from internal storage
        wxString statement;
        if (!rowsM.removeRows(pos, numRows, statement))
            return false;

        // used in frame to show executed statements
        wxGrid* grid = GetView();
        wxCommandEvent evt2(wxEVT_FRDG_STATEMENT, grid->GetId());
        evt2.SetString(statement);
        wxPostEvent(grid, evt2);

        if (numRows > 0)
            grid->ForceRefresh();
        return true;
    }
    catch (const IBPP::Exception& e)
    {
        showErrorDialog(wxGetTopLevelParent(wxGetActiveWindow()),
            _("Database error"), e.what(),
            AdvancedMessageDialogButtonsOk());
    }
    catch (...)
    {
        showErrorDialog(wxGetTopLevelParent(wxGetActiveWindow()),
            _("System error"), _("Unhandled exception"),
            AdvancedMessageDialogButtonsOk());
    }
    return false;
}

DEFINE_EVENT_TYPE(wxEVT_FRDG_ROWCOUNT_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_FRDG_STATEMENT)
DEFINE_EVENT_TYPE(wxEVT_FRDG_INVALIDATEATTR)

