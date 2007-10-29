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

#include <algorithm>
#include <set>

#include "config/Config.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "gui/controls/DataGridRows.h"
#include "gui/controls/DataGridTable.h"
#include "metadata/database.h"
//-----------------------------------------------------------------------------
class RgbHsvConversion
{
private:
    float redM, greenM, blueM;
    float hueM, satM, valM;
    bool rgbValidM, hsvValidM;
    void calcHsvFromRgb();
    void calcRgbFromHsv();
public:
    RgbHsvConversion(const wxColour& colour);
    wxColour getColour();
    float getValue();
    void setValue(float value);
};
//-----------------------------------------------------------------------------
RgbHsvConversion::RgbHsvConversion(const wxColour& colour)
{
    redM = colour.Red() / 255.0;
    greenM = colour.Green() / 255.0;
    blueM = colour.Blue() / 255.0;
#if wxCHECK_VERSION(2, 8, 0)
    rgbValidM = colour.IsOk();
#else
    rgbValidM = colour.Ok();
#endif
    hueM = 0.0;
    satM = 0.0;
    valM = 0.0;
    hsvValidM = false;
}
//-----------------------------------------------------------------------------
// Adapted from public domain code by Zack Booth Simpson
// http://www.mine-control.com/zack/code/zrgbhsv.cpp
void RgbHsvConversion::calcHsvFromRgb()
{
    float rgbMin = std::min(redM, std::min(greenM, blueM));
    float rgbMax = std::max(redM, std::max(greenM, blueM));
    valM = rgbMax;
    float delta = rgbMax - rgbMin;
    if (delta == 0.0)
    {
        // gray value, saturation is 0, hue is undefined
        hueM = -1.0;
        satM = 0.0;
        hsvValidM = true;
        return;
    }

    satM = delta / rgbMax;
    if (redM == rgbMax) // between yellow & magenta
        hueM = (greenM - blueM) / delta;
    else if (greenM == rgbMax) // between cyan & yellow
        hueM = 2 + (blueM - redM) / delta;
    else // between magenta & cyan
        hueM = 4 + (redM - greenM) / delta;
    hueM *= 60; // degrees
    if (hueM < 0.0)
        hueM += 360.0;
    hueM /= 360.0;
    hsvValidM = true;
}
//-----------------------------------------------------------------------------
// Adapted from public domain code by Zack Booth Simpson
// http://www.mine-control.com/zack/code/zrgbhsv.cpp
void RgbHsvConversion::calcRgbFromHsv()
{
    if (satM == 0.0) // gray value, saturation is 0, hue is undefined
    {
        redM = greenM = blueM = valM;
        rgbValidM = true;
        return;
    }

    float f, p, q, t;
    float h = 6.0 * hueM;
    int sector = (int) floor(h);
    f = h - sector;
    p = valM * (1.0 - satM);
    q = valM * (1.0 - satM * f);
    t = valM * (1.0 - satM * (1.0 - f));

    switch (sector)
    {
        case 0:
            redM = valM;
            greenM = t;
            blueM = p;
            break;
        case 1:
            redM = q;
            greenM = valM;
            blueM = p;
            break;
        case 2:
            redM = p;
            greenM = valM;
            blueM = t;
            break;
        case 3:
            redM = p;
            greenM = q;
            blueM = valM;
            break;
        case 4:
            redM = t;
            greenM = p;
            blueM = valM;
            break;
        default:
            redM = valM;
            greenM = p;
            blueM = q;
            break;
    }
    rgbValidM = true;
}
//-----------------------------------------------------------------------------
wxColour RgbHsvConversion::getColour()
{
    if (!rgbValidM)
    {
        wxASSERT(hsvValidM);
        calcRgbFromHsv();
    }
    return wxColour((unsigned char)(255.0 * redM),
        (unsigned char)(255.0 * greenM), (unsigned char)(255.0 * blueM));
}
//-----------------------------------------------------------------------------
float RgbHsvConversion::getValue()
{
    if (!hsvValidM)
    {
        wxASSERT(rgbValidM);
        calcHsvFromRgb();
    }
    return valM;
}
//-----------------------------------------------------------------------------
void RgbHsvConversion::setValue(float value)
{
    if (value < 0.0 || value > 1.0)
        return;
    if (rgbValidM && !hsvValidM)
        calcHsvFromRgb();
    if (valM != value)
    {
        valM = value;
        rgbValidM = false;
    }
}
//-----------------------------------------------------------------------------
DataGridTable::DataGridTable(IBPP::Statement& s, Database *db)
    : wxGridTableBase(), statementM(s), databaseM(db), nullFlagM(false),
      rowsM(db)
{
    allRowsFetchedM = false;
    fetchAllRowsM = false;
    canInsertRowsIsSetM = false;
    canInsertRowsM = false;
    config().getValue(wxT("GridFetchAllRecords"), fetchAllRowsM);
    maxRowToFetchM = 100;
    cellAttriM = new wxGridCellAttr();
}
//-----------------------------------------------------------------------------
DataGridTable::~DataGridTable()
{
    Clear();
    cellAttriM->DecRef();
}
//-----------------------------------------------------------------------------
void DataGridTable::setNullFlag(bool isNull)
{
    nullFlagM = isNull;
}
//-----------------------------------------------------------------------------
// implementation methods
bool DataGridTable::canFetchMoreRows()
{
    // this will also handle a closed result set
    return !allRowsFetchedM && getStatementColCount() > 0;
}
//-----------------------------------------------------------------------------
void DataGridTable::Clear()
{
    FR_TRY

    nullFlagM = false;

    allRowsFetchedM = true;
    fetchAllRowsM = false;
    canInsertRowsIsSetM = false;
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

    FR_CATCH
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
//-----------------------------------------------------------------------------
wxGridCellAttr* DataGridTable::GetAttr(int row, int col,
    wxGridCellAttr::wxAttrKind kind)
{
    DataGridFieldInfo info;
    if (!rowsM.getFieldInfo(row, col, info))
        return wxGridTableBase::GetAttr(row, col, kind);

    bool useAttri = info.rowInserted || info.rowDeleted || info.fieldReadOnly
        || info.fieldModified || info.fieldNull || info.fieldNA
        || info.fieldNumeric;
    if (!useAttri)
        return wxGridTableBase::GetAttr(row, col, kind);

    // text colour
    wxColour textCol;
    if (info.fieldNull || info.fieldNA)
        textCol = *wxRED;
    else if (info.fieldModified)
        textCol = *wxBLUE;
    else
        textCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    cellAttriM->SetTextColour(textCol);

    // background colour
    wxColour bgCol;
    if (info.rowDeleted)
        bgCol = wxColour(255, 208, 208);
    else if (info.rowInserted)
        bgCol = wxColour(235, 255, 200);
    else if (info.fieldReadOnly)
        bgCol = getReadonlyColour();
    else
        bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    cellAttriM->SetBackgroundColour(bgCol);

    // text alignment
    if (info.fieldNumeric)
        cellAttriM->SetAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
    else
        cellAttriM->SetAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);

    cellAttriM->SetReadOnly(info.fieldReadOnly);

    cellAttriM->IncRef();
    return cellAttriM;
}
//-----------------------------------------------------------------------------
wxString DataGridTable::getCellValue(int row, int col)
{
    if (!isValidCellPos(row, col))
        return wxEmptyString;

    if (rowsM.isFieldNA(row, col))
        return wxT("N/A");
    if (rowsM.isFieldNull(row, col))
        return wxT("[null]");
    return rowsM.getFieldValue(row, col);
}
//-----------------------------------------------------------------------------
wxString DataGridTable::getCellValueForInsert(int row, int col)
{
    if (!isValidCellPos(row, col) || rowsM.isFieldNA(row, col))
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
    if (!isValidCellPos(row, col) || rowsM.isFieldNA(row, col))
        return wxEmptyString;

    if (rowsM.isFieldNull(row, col))
        return wxT("\"NULL\"");
    wxString s(rowsM.getFieldValue(row, col));
    if (rowsM.isColumnNumeric(col))
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
wxColour DataGridTable::getReadonlyColour()
{
    static wxColour colourReadOnly;
#if wxCHECK_VERSION(2, 8, 0)
    if (!colourReadOnly.IsOk())
#else
    if (!colourReadOnly.Ok())
#endif
    {
        // first try to compute a colour that is between "white" and "gray"
        // (but use the actual system colours instead of hard-coded values)
        wxColour clWnd(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
        int r1 = clWnd.Red(), g1 = clWnd.Green(), b1 = clWnd.Blue();
        wxColour clBtn = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
        int r2 = clBtn.Red(), g2 = clBtn.Green(), b2 = clBtn.Blue();
        int distance = abs(r1 - r2) + abs(g1 - g2) + abs(b1 - b2);
        if (distance >= 72)
        {
            // start at 50 %, and use progressively lighter colours for larger
            // distances between white and gray
            int scale = (distance >= 192) ? distance / 64 : 2;
            colourReadOnly.Set(r1 + (r2 - r1) / scale,
                g1 + (g2 - g1) / scale, b1 + (b2 - b1) / scale);
        }
        else
        {
            // wxSYS_COLOUR_WINDOW and wxSYS_COLOUR_BTNFACE are too similar
            // compute a darker shade of wxSYS_COLOUR_WINDOW
            RgbHsvConversion rgbhsv(clWnd);
            rgbhsv.setValue(std::max(0.0, rgbhsv.getValue() - 0.05));
            colourReadOnly = rgbhsv.getColour();
        }
    }
    return colourReadOnly;
}
//-----------------------------------------------------------------------------
int DataGridTable::getStatementColCount()
{
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
//-----------------------------------------------------------------------------
wxString DataGridTable::getTableName()
{
    // TODO: using one table is not correct for JOINs or sub-SELECTs, so we
    //       should build separate statements for each table
    //       DataGridRows::statementTablesM contains that list
    //       (together with PK/UNQ info)
    if (getStatementColCount() == 0)
        return wxEmptyString;
    return std2wx(statementM->ColumnTable(1));
}
//-----------------------------------------------------------------------------
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
        wxString tn(std2wx(statementM->ColumnTable(i+1)));
        if (wxNOT_FOUND == tables.Index(tn))
        {
            // check if table exists in metadata
            Table *t = dynamic_cast<Table *>(databaseM->findRelation(
                Identifier(tn)));
            if (!t)
                continue;
            t->checkAndLoadColumns();

            // check if table's column is 'real'
            wxString cn(std2wx(statementM->ColumnName(i+1)));
            for (MetadataCollection<Column>::iterator it = t->begin();
                it != t->end(); ++it)
            {
                if ((*it).getName_() == cn
                    && (*it).getComputedSource().IsEmpty())
                {
                    tables.Add(tn);
                    break;
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------
// all fields of that table
void DataGridTable::getFields(const wxString& table,
    DataGridTable::FieldSet& flds)
{
    int colCount = getStatementColCount();
    if (colCount == 0)
        return;
    Table *t = dynamic_cast<Table *>(databaseM->
        findRelation(Identifier(table)));
    if (!t)
        return;
    typedef std::map<Column *, std::pair<ResultsetColumnDef*,int> > TempMap;
    TempMap fields;
    for (int i = 0; i < colCount; i++)
    {
        wxString tn(std2wx(statementM->ColumnTable(i+1)));
        if (tn != table)
            continue;
        wxString fn(std2wx(statementM->ColumnName(i+1)));
        // check if field exists in the table (and is not computed)
        for (MetadataCollection<Column>::iterator it = t->begin();
            it != t->end(); ++it)
        {
            if ((*it).getName_() == fn && (*it).getComputedSource().IsEmpty())
            {
                // field found in table and not computed
                if (fields.find(&(*it)) == fields.end())
                {
                    std::pair<ResultsetColumnDef *, int>
                        p(rowsM.getColumnDef(i), i);
                    fields.insert
                    (
                        std::pair
                        <
                            Column *,
                            std::pair<ResultsetColumnDef *, int>
                        >
                        (&(*it), p)
                    );
                }
                break;
            }
        }
    }

    // we have item list sorted by column *, but user probably expects the
    // same order as in the grid, so we sort it
    for (TempMap::iterator it = fields.begin(); it != fields.end(); ++it)
    {
        std::pair<ResultsetColumnDef *, Column *> p(
            (*it).second.first, (*it).first);
        flds.insert
        (
            std::pair
            <
                int,
                std::pair<ResultsetColumnDef *, Column *>
            >
            ((*it).second.second, p)
        );
    }
}
//-----------------------------------------------------------------------------
Database *DataGridTable::getDatabase()
{
    return databaseM;
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

    if (rowsM.isFieldNA(row, col))
        return wxT("N/A");
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
    canInsertRowsIsSetM = false;
    canInsertRowsM = false;
    maxRowToFetchM = 100;

    if (conv)
        charsetConverterM = conv;
    else
        charsetConverterM = wxConvCurrent;

    try
    {
        rowsM.initialize(statementM, databaseM);
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
    return rowsM.isColumnNumeric(col);
}
//-----------------------------------------------------------------------------
bool DataGridTable::isReadonlyColumn(int col)
{
    return rowsM.isColumnReadonly(col);
}
//-----------------------------------------------------------------------------
bool DataGridTable::isValidCellPos(int row, int col)
{
    return (row >= 0 && col >= 0 && row < (int)rowsM.getRowCount()
        && col < (int)rowsM.getRowFieldCount());
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
bool DataGridTable::canRemoveRow(size_t row)
{
    return rowsM.canRemoveRow(row);
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
    FR_TRY

    wxString statement = rowsM.setFieldValue(row, col, value, nullFlagM);
    nullFlagM = false;  // reset

    // used in frame to show executed statements
    wxCommandEvent evt(wxEVT_FRDG_STATEMENT, GetView()->GetId());
    evt.SetString(statement);
    wxPostEvent(GetView(), evt);

    FR_CATCH
}
//-----------------------------------------------------------------------------
bool DataGridTable::DeleteRows(size_t pos, size_t numRows)
{
    FR_TRY

    // remove rows from internal storage
    wxString statement;
    if (!rowsM.removeRows(pos, numRows, statement))
        return false;

/*
    // notify visual control
    if (GetView() && numRows > 0)
    {
        GetView()->ForceRefresh();
        wxGridTableMessage rowMsg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED,
            pos, numRows);
        GetView()->ProcessTableMessage(rowMsg);

        // used in frame to update status bar
        wxCommandEvent evt(wxEVT_FRDG_ROWCOUNT_CHANGED, GetView()->GetId());
        evt.SetExtraLong(rowsM.getRowCount());
        wxPostEvent(GetView(), evt);
    }
*/
    // used in frame to show executed statements
    wxCommandEvent evt2(wxEVT_FRDG_STATEMENT, GetView()->GetId());
    evt2.SetString(statement);
    wxPostEvent(GetView(), evt2);

    if (GetView() && numRows > 0)
        GetView()->ForceRefresh();

    return true;

    FR_CATCH

    return false;
}
//-----------------------------------------------------------------------------
DEFINE_EVENT_TYPE(wxEVT_FRDG_ROWCOUNT_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_FRDG_STATEMENT)
//-----------------------------------------------------------------------------
