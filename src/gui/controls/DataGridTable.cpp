/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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

#include <wx/fontmap.h>
#include <wx/grid.h>
#include <wx/strconv.h>

#include "core/StringUtils.h"
#include "gui/controls/DataGridCells.h"
#include "gui/controls/DataGridTable.h"
//-----------------------------------------------------------------------------
DataGridTableCharsetConverter::DataGridTableCharsetConverter()
{
    converterM = 0;
}
//-----------------------------------------------------------------------------
DataGridTableCharsetConverter::~DataGridTableCharsetConverter()
{
    delete converterM;
}
//-----------------------------------------------------------------------------
wxMBConv* DataGridTableCharsetConverter::getConverter()
{
    if (converterM)
        return converterM;
    else
        return wxConvCurrent;
}
//-----------------------------------------------------------------------------
wxString DataGridTableCharsetConverter::mapCharset(
    const wxString& connectionCharset)
{
    wxString charset(connectionCharset.Upper().Trim());
    charset.Trim(false);

    // Firebird charsets WIN125X need to be replaced with either
    // WINDOWS125X or CP125X - we take the latter
    if (charset.Mid(0, 5) == wxT("WIN12"))
        return wxT("CP12") + charset.Mid(5);

    // Firebird charsets ISO8859-X (and some others) are recognized as-is
    // all other mappings need to be added here...
    struct CharsetMapping { const wxChar* connCS; const wxChar* convCS; };
    static const CharsetMapping mappings[] = {
        { wxT("UTF8"), wxT("UTF-8") }, { wxT("UNICODE_FSS"), wxT("UTF-8") }
    };
    int mappingCount = sizeof(mappings) / sizeof(CharsetMapping);
    for (int i = 0; i < mappingCount; i++)
    {
        if (mappings[i].connCS == charset)
            return mappings[i].convCS;
    }

    return charset;
}
//-----------------------------------------------------------------------------
void DataGridTableCharsetConverter::setConnectionCharset(
    const wxString& connectionCharset)
{
    if (connectionCharsetM != connectionCharset)
    {
        if (converterM)
        {
            delete converterM;
            converterM = 0;
        }

        connectionCharsetM = connectionCharset;
        wxFontEncoding fe = wxFontMapperBase::Get()->CharsetToEncoding(
            mapCharset(connectionCharset), false);
        if (fe != wxFONTENCODING_SYSTEM)
            converterM = new wxCSConv(fe);
    }
}
//-----------------------------------------------------------------------------
DataGridTable::DataGridTable(IBPP::Statement& s)
    : wxGridTableBase(), statementM(s)
{
    allRowsFetchedM = false;
    columnCountM = 0;
    fetchAllRowsM = false;
    maxRowToFetchM = 100;
    rowsFetchedM = 0;

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
    int oldrf = rowsFetchedM;
    int oldcc = columnCountM;

    allRowsFetchedM = true;
    columnCountM = 0;
    rowsFetchedM = 0;
    fetchAllRowsM = false;

    for (size_t i = 0; i < dataM.size(); i++)
    {
        for (size_t j = 0; j < dataM[i].size(); j++)
            delete dataM[i][j];
    }
    dataM.clear();

    if (GetView())
    {
        if (oldrf)
        {
            wxGridTableMessage rowMsg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED,
                0, oldrf);
            GetView()->ProcessTableMessage(rowMsg);
        }
        if (oldcc)
        {
            wxGridTableMessage colMsg(this, wxGRIDTABLE_NOTIFY_COLS_DELETED,
                0, oldcc);
            GetView()->ProcessTableMessage(colMsg);
        }
    }
}
//-----------------------------------------------------------------------------
void DataGridTable::fetch()
{
    if (!canFetchMoreRows())
        return;
    if (columnCountM == 0 && GetView())
    {
        columnCountM = statementM->Columns();
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_APPENDED,
            columnCountM);
        GetView()->ProcessTableMessage(msg);
    }

    int oldrf = rowsFetchedM;
    // fetch the first 100 rows no matter how long it takes
    bool initial = !rowsFetchedM;
    // fetch more rows until maxRowToFetchM reached or 100 ms elapsed
    wxLongLong startms = ::wxGetLocalTimeMillis();
    wxMBConv* converter = charsetConverterM.getConverter();
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
        rowsFetchedM++;

        std::vector<DataGridCell*> s;
        s.reserve(columnCountM);

        for (int i = 1; i <= columnCountM; i++)
            s.push_back(DataGridCell::createCell(statementM, i, converter));
        dataM.push_back(s);

        if (!initial && (::wxGetLocalTimeMillis() - startms > 100))
            break;
    }
    while (fetchAllRowsM || rowsFetchedM < maxRowToFetchM);

    if (rowsFetchedM > oldrf && GetView())        // notify the grid
    {
        wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
            rowsFetchedM - oldrf);
        GetView()->ProcessTableMessage(msg);
        // used in frame to update status bar
        wxCommandEvent evt(wxEVT_FRDG_ROWCOUNT_CHANGED, GetView()->GetId());
        evt.SetExtraLong(rowsFetchedM);
        wxPostEvent(GetView(), evt);
    }
}
//-----------------------------------------------------------------------------
wxGridCellAttr* DataGridTable::GetAttr(int row, int col,
    wxGridCellAttr::wxAttrKind kind)
{
    if (row < rowsFetchedM && col < columnCountM && !dataM[row][col])
    {
        // wxGrid columns run from 0 to columnCountM - 1
        // IBPP::Statement columns run from 1 to Columns()
        if (isNumericColumn(col + 1))
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
    if (row >= (int)dataM.size())
        return wxEmptyString;
    if (col >= (int)dataM[row].size())
        return wxEmptyString;

    DataGridCell* cell = dataM[row][col];
    if (cell)
        return cell->getValue();
    else
        return wxT("[null]");
}
//-----------------------------------------------------------------------------
wxString DataGridTable::getCellValueForInsert(int row, int col)
{
    if (row >= (int)dataM.size())
        return wxEmptyString;
    if (col >= (int)dataM[row].size())
        return wxEmptyString;

    DataGridCell* cell = dataM[row][col];
    if (!cell)
        return wxT("NULL");
    // return quoted text, but escape embedded quotes
    wxString s(cell->getValue());
    s.Replace(wxT("'"), wxT("''"));
    return wxT("'") + s + wxT("'");
}
//-----------------------------------------------------------------------------
wxString DataGridTable::GetColLabelValue(int col)
{
    if (col < columnCountM && statementM != 0)
        return std2wx(statementM->ColumnAlias(col+1));
    else
        return wxEmptyString;
}
//-----------------------------------------------------------------------------
IBPP::SDT DataGridTable::getColumnType(int col)
{
    if (statementM == 0 || columnCountM == 0)
        return IBPP::sdString;    // I wish there is sdUnknown :)
    else
    {
        try
        {
            return statementM->ColumnType(col);
        }
        catch (IBPP::Exception& e)
        {
            // perhaps we should clear the statement, since something is obviously wrong
            columnCountM = col - 1;
            ::wxMessageBox(std2wx(e.ErrorMessage()),
                            _("An IBPP error occurred."), wxOK|wxICON_ERROR);
            return IBPP::sdString;
        }
    }
}
//-----------------------------------------------------------------------------
bool DataGridTable::getFetchAllRows()
{
    return fetchAllRowsM;
}
//-----------------------------------------------------------------------------
int DataGridTable::GetNumberCols()
{
    return columnCountM;
}
//-----------------------------------------------------------------------------
int DataGridTable::GetNumberRows()
{
    return rowsFetchedM;
}
//-----------------------------------------------------------------------------
wxString DataGridTable::getTableName()
{
    // TODO: using one table is not correct for JOINs or sub-SELECTs, so it
    //       should take e.g. the one that occurs most often
    if (statementM == 0 || columnCountM == 0)
        return wxEmptyString;
    else
        return std2wx(statementM->ColumnTable(1));
}
//-----------------------------------------------------------------------------
wxString DataGridTable::GetValue(int row, int col)
{
    if (row >= (int)dataM.size())
        return wxEmptyString;
    if (col >= (int)dataM[row].size())
        return wxEmptyString;

    // keep between 200 and 250 more rows fetched for better responsiveness
    // (but make the count of fetched rows a multiple of 50)
    int maxRowToFetch = 50 * (row / 50 + 5);
    if (maxRowToFetchM < maxRowToFetch)
        maxRowToFetchM = maxRowToFetch;

    DataGridCell* cell = dataM[row][col];
    if (cell)
    {
        wxString cellValue(cell->getValue());
        // return first line of multi-line string only
        for (size_t n = 0; n < cellValue.size(); ++n)
        {
            const wxChar c = cellValue[n];
            if (c == '\r' || c == '\n')
            {
                cellValue.Truncate(n);
                cellValue += wxT(" [...]");
                break;
            }
        }
        return cellValue;
    }
    else
        return wxT("[null]");
}
//-----------------------------------------------------------------------------
void DataGridTable::initialFetch(const wxString& connectionCharset)
{
    Clear();
    allRowsFetchedM = false;
    maxRowToFetchM = 100;

    charsetConverterM.setConnectionCharset(connectionCharset);
    fetch();
}
//-----------------------------------------------------------------------------
bool DataGridTable::IsEmptyCell(int row, int col)
{
    return row >= rowsFetchedM || col >= columnCountM;
}
//-----------------------------------------------------------------------------
bool DataGridTable::isNullCell(int row, int col)
{
    if (row >= (int)dataM.size())
        return false;
    if (col >= (int)dataM[row].size())
        return false;
    return (0 == dataM[row][col]);
}
//-----------------------------------------------------------------------------
bool DataGridTable::isNumericColumn(int col)
{
    switch (getColumnType(col))
    {
        case IBPP::sdFloat:
        case IBPP::sdDouble:
        case IBPP::sdInteger:
        case IBPP::sdSmallint:
        case IBPP::sdLargeint:
            return true;
        default:
            return false;
    }
}
//-----------------------------------------------------------------------------
bool DataGridTable::needsMoreRowsFetched()
{
    if (allRowsFetchedM)
        return false;
    // true if all rows are to be fetched, or more rows should be cached
    // for more responsive grid scrolling
    return (fetchAllRowsM || rowsFetchedM < maxRowToFetchM);
}
//-----------------------------------------------------------------------------
void DataGridTable::setFetchAllRecords(bool fetchall)
{
    fetchAllRowsM = fetchall;
}
//-----------------------------------------------------------------------------
void DataGridTable::SetValue(int WXUNUSED(row), int WXUNUSED(col),
    const wxString& WXUNUSED(value))
{
    // needs to be implemented for editable grid
}
//-----------------------------------------------------------------------------
DEFINE_EVENT_TYPE(wxEVT_FRDG_ROWCOUNT_CHANGED)
//-----------------------------------------------------------------------------
