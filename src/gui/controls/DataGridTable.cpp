/*
  Copyright (c) 2004-2007 The FlameRobin Development Team

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


  $Id$

*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
  #include "wx/wx.h"
#endif

#include <wx/grid.h>

#include "config/Config.h"
#include "core/StringUtils.h"
#include "gui/controls/DataGridTable.h"
//-----------------------------------------------------------------------------
DataGridTable::DataGridTable(IBPP::Statement& s)
    : wxGridTableBase(), statementM(s)
{
    allRowsFetchedM = false;
    fetchAllRowsM = false;
    config().getValue(wxT("GridFetchAllRecords"), fetchAllRowsM);
    maxRowToFetchM = 100;

    nullAttrM = new wxGridCellAttr();
    nullAttrM->SetTextColour(*wxRED);
    nullAttrM->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);
    nullAttrNumericM = new wxGridCellAttr();
    nullAttrNumericM->SetTextColour(*wxRED);
    nullAttrNumericM->SetAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
}
//-----------------------------------------------------------------------------
DataGridTable::~DataGridTable()
{
    Clear();
    nullAttrNumericM->DecRef();
    nullAttrM->DecRef();
}
//-----------------------------------------------------------------------------
// implementation methods
bool DataGridTable::canFetchMoreRows()
{
    if (allRowsFetchedM || statementM->Type() != IBPP::stSelect)
        return false;
    // could this really happen?
    if (statementM == 0)
        return false;
    // there should be a better way here...
    IBPP::Transaction tran = statementM->TransactionPtr();
    return (tran != 0 && tran->Started());
}
//-----------------------------------------------------------------------------
void DataGridTable::Clear()
{
    allRowsFetchedM = true;
    fetchAllRowsM = false;
    config().getValue(wxT("GridFetchAllRecords"), fetchAllRowsM);

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
//-----------------------------------------------------------------------------
void DataGridTable::fetch()
{
    if (!canFetchMoreRows())
        return;

    // fetch the first 100 rows no matter how long it takes
    unsigned oldRows = rowsM.getRowCount();
    bool initial = oldRows == 0;
    // fetch more rows until maxRowToFetchM reached or 100 ms elapsed
    wxLongLong startms = ::wxGetLocalTimeMillis();
    do
    {
        try
        {
            if (!statementM->Fetch())
                allRowsFetchedM = true;
        }
        catch (IBPP::Exception& e)
        {
            allRowsFetchedM = true;
            ::wxMessageBox(std2wx(e.ErrorMessage()),
                _("An IBPP error occurred."), wxOK|wxICON_ERROR);
        }
        catch (...)
        {
            allRowsFetchedM = true;
            ::wxMessageBox(_("A system error occurred!"), _("Error"),
                wxOK|wxICON_ERROR);
        }
        if (allRowsFetchedM)
            break;
        rowsM.addRow(statementM, charsetConverterM);

        if (!initial && (::wxGetLocalTimeMillis() - startms > 100))
            break;
    }
    while ((fetchAllRowsM && !initial) || rowsM.getRowCount() < maxRowToFetchM);

    if (rowsM.getRowCount() > oldRows && GetView())   // notify the grid
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
            rowsM.getRowCount() - oldRows);
        GetView()->ProcessTableMessage(msg);
        // used in frame to update status bar
        wxCommandEvent evt(wxEVT_FRDG_ROWCOUNT_CHANGED, GetView()->GetId());
        evt.SetExtraLong(rowsM.getRowCount());
        wxPostEvent(GetView(), evt);
    }
}
//-----------------------------------------------------------------------------
wxGridCellAttr* DataGridTable::GetAttr(int row, int col,
    wxGridCellAttr::wxAttrKind kind)
{
    if (rowsM.isFieldNull(row, col))
    {
        if (rowsM.isRowFieldNumeric(col))
        {
            nullAttrNumericM->IncRef();
            return nullAttrNumericM;
        }
        nullAttrM->IncRef();
        return nullAttrM;
    }
    return wxGridTableBase::GetAttr(row, col, kind);
}
//-----------------------------------------------------------------------------
wxString DataGridTable::getCellValue(int row, int col)
{
    if (!isValidCellPos(row, col))
        return wxEmptyString;

    if (rowsM.isFieldNull(row, col))
        return wxT("[null]");
    return rowsM.getFieldValue(row, col);
}
//-----------------------------------------------------------------------------
wxString DataGridTable::getCellValueForInsert(int row, int col)
{
    if (!isValidCellPos(row, col))
        return wxEmptyString;

    if (rowsM.isFieldNull(row, col))
        return wxT("NULL");
    // return quoted text, but escape embedded quotes
    wxString s(rowsM.getFieldValue(row, col));
    s.Replace(wxT("'"), wxT("''"));
    return wxT("'") + s + wxT("'");
}
//-----------------------------------------------------------------------------
wxString DataGridTable::getCellValueForCSV(int row, int col)
{
    if (!isValidCellPos(row, col))
        return wxEmptyString;

    if (rowsM.isFieldNull(row, col))
        return wxT("\"NULL\"");
    wxString s(rowsM.getFieldValue(row, col));
    if (rowsM.isRowFieldNumeric(col))
        return s;

    // return quoted text, but escape embedded quotes
    s.Replace(wxT("\""), wxT("\"\""));
    return wxT("\"") + s + wxT("\"");
}
//-----------------------------------------------------------------------------
wxString DataGridTable::GetColLabelValue(int col)
{
    return rowsM.getRowFieldName(col);
}
//-----------------------------------------------------------------------------
bool DataGridTable::getFetchAllRows()
{
    return fetchAllRowsM;
}
//-----------------------------------------------------------------------------
int DataGridTable::GetNumberCols()
{
    return rowsM.getRowFieldCount();
}
//-----------------------------------------------------------------------------
int DataGridTable::GetNumberRows()
{
    return rowsM.getRowCount();
}
//-----------------------------------------------------------------------------
wxString DataGridTable::getTableName()
{
    // TODO: using one table is not correct for JOINs or sub-SELECTs, so it
    //       should take e.g. the one that occurs most often
    if (statementM == 0 || statementM->Columns() == 0)
        return wxEmptyString;
    return std2wx(statementM->ColumnTable(1));
}
//-----------------------------------------------------------------------------
wxString DataGridTable::GetValue(int row, int col)
{
    if (!isValidCellPos(row, col))
        return wxEmptyString;

    // keep between 200 and 250 more rows fetched for better responsiveness
    // (but make the count of fetched rows a multiple of 50)
    unsigned maxRowToFetch = 50 * (row / 50 + 5);
    if (maxRowToFetchM < maxRowToFetch)
        maxRowToFetchM = maxRowToFetch;

    if (rowsM.isFieldNull(row, col))
        return wxT("[null]");
    wxString cellValue(rowsM.getFieldValue(row, col));

    // return first line of multi-line string only
    int nl = cellValue.Find(wxT("\n"));
    if (nl != wxNOT_FOUND)
    {
        cellValue.Truncate(nl);
        cellValue.Trim();
        cellValue += wxT(" [...]"); // and show that there is more data...
    }
    return cellValue;
}
//-----------------------------------------------------------------------------
void DataGridTable::initialFetch(wxMBConv* conv)
{
    Clear();
    allRowsFetchedM = false;
    maxRowToFetchM = 100;

    if (conv)
        charsetConverterM = conv;
    else
        charsetConverterM = wxConvCurrent;

    try
    {
        rowsM.initialize(statementM);
    }
    catch (IBPP::Exception& e)
    {
        ::wxMessageBox(std2wx(e.ErrorMessage()),
            _("An IBPP error occurred."), wxOK | wxICON_ERROR);
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
    fetch();
}
//-----------------------------------------------------------------------------
bool DataGridTable::IsEmptyCell(int row, int col)
{
    return !isValidCellPos(row, col);
}
//-----------------------------------------------------------------------------
bool DataGridTable::isNullCell(int row, int col)
{
    return rowsM.isFieldNull(row, col);
}
//-----------------------------------------------------------------------------
bool DataGridTable::isNumericColumn(int col)
{
    return rowsM.isRowFieldNumeric(col);
}
//-----------------------------------------------------------------------------
bool DataGridTable::isValidCellPos(int row, int col)
{
    return (row >= 0 && col >= 0 && row < (int)rowsM.getRowCount()
        && col < (int)rowsM.getRowFieldCount());
}
//-----------------------------------------------------------------------------
bool DataGridTable::needsMoreRowsFetched()
{
    if (allRowsFetchedM)
        return false;
    // true if all rows are to be fetched, or more rows should be cached
    // for more responsive grid scrolling
    return (fetchAllRowsM || rowsM.getRowCount() < maxRowToFetchM);
}
//-----------------------------------------------------------------------------
void DataGridTable::setFetchAllRecords(bool fetchall)
{
    fetchAllRowsM = fetchall;
}
//-----------------------------------------------------------------------------
void DataGridTable::SetValue(int row, int col, const wxString& value)
{
    // needs to be implemented for editable grid

    // 1. write a new value to data
    // 2. flag row&cell as 'dirty'
    rowsM.setFieldValue(row, col, value);
}
//-----------------------------------------------------------------------------
bool DataGridTable::DeleteRows(size_t pos, size_t numRows)
{
    // execute the DELETE statement
}
//-----------------------------------------------------------------------------
DEFINE_EVENT_TYPE(wxEVT_FRDG_ROWCOUNT_CHANGED)
//-----------------------------------------------------------------------------
