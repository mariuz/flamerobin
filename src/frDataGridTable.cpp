/*
The contents of this file are subject to the Initial Developer's Public
License Version 1.0 (the "License"); you may not use this file except in
compliance with the License. You may obtain a copy of the License here:
http://www.flamerobin.org/license.html.

Software distributed under the License is distributed on an "AS IS"
basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
License for the specific language governing rights and limitations under
the License.

The Original Code is FlameRobin (TM).

The Initial Developer of the Original Code is Milan Babuskov.

Portions created by the original developer
are Copyright (C) 2004 Milan Babuskov.

All Rights Reserved.

$Id$

Contributor(s): Michael Hieke
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

#include "converters.h"
#include "frDataGridCells.h"
#include "frDataGridTable.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
GridTable::GridTable(IBPP::Statement& s)
    : wxGridTableBase(), statementM(s)
{
    allRowsFetchedM = false;
    columnCountM = 0;
    maxRowToFetchM = 100;
    rowsFetchedM = 0;

    nullAttrM = new wxGridCellAttr();
    nullAttrM->SetTextColour(*wxRED);
}
//-----------------------------------------------------------------------------
GridTable::~GridTable()
{
    Clear();
    nullAttrM->DecRef();
}
//-----------------------------------------------------------------------------
// implementation methods
bool GridTable::canFetchMoreRows()
{
    if (allRowsFetchedM || statementM->Type() != IBPP::stSelect)
        return false;
    // could this really happen?
    if (statementM.intf() == 0)
        return false;
    // there should be a better way here...
    IBPP::ITransaction* tran = statementM->Transaction();
    return (tran && tran->Started());
}
//-----------------------------------------------------------------------------
void GridTable::Clear()
{
    int oldrf = rowsFetchedM;
    int oldcc = columnCountM;

    allRowsFetchedM = true;
    columnCountM = 0;
    rowsFetchedM = 0;

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
void GridTable::fetch()
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
                _("An IBPP error occurred."));
        }
        catch (...)
        {
            allRowsFetchedM = true;
            ::wxMessageBox(_("A system error occurred!"));
        }
        if (allRowsFetchedM)
            break;
        rowsFetchedM++;

        std::vector<GridBaseCell*> s;
        s.reserve(columnCountM);
        for (int i=1; i<=statementM->Columns(); i++)
        {
            std::string value;
            if (CreateString(statementM, i, value))
                s.push_back(new DataGridCell(std2wx(value)));
            else
                s.push_back(0);
        }
        dataM.push_back(s);

        if (!initial && (::wxGetLocalTimeMillis() - startms > 100))
            break;
    } 
    while (rowsFetchedM < maxRowToFetchM);

    if (rowsFetchedM > oldrf && GetView())		// notify the grid
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
wxGridCellAttr* GridTable::GetAttr(int row, int col, 
    wxGridCellAttr::wxAttrKind kind)
{
    if (row < rowsFetchedM && col < columnCountM && !dataM[row][col])
    {
        nullAttrM->IncRef();
        return nullAttrM;
    }
    return wxGridTableBase::GetAttr(row, col, kind);
}
//-----------------------------------------------------------------------------
wxString GridTable::getCellValueForInsert(int row, int col)
{
    if (row >= (int)dataM.size())
        return wxEmptyString;
    if (col >= (int)dataM[row].size())
        return wxEmptyString;

    GridBaseCell* cell = dataM[row][col];
    if (!cell)
        return wxT("NULL");
    // return quoted text, but escape embedded quotes
    wxString s(cell->getValue());
    s.Replace(wxT("'"), wxT("''"));		
    return wxT("'") + s + wxT("'");
}
//-----------------------------------------------------------------------------
wxString GridTable::GetColLabelValue(int col)
{
    if (col < columnCountM && statementM.intf())
        return std2wx(statementM->ColumnAlias(col+1));
    else
        return wxEmptyString;
}
//-----------------------------------------------------------------------------
IBPP::SDT GridTable::getColumnType(int col)
{
    if (col > columnCountM || statementM.intf() == 0)
        return IBPP::sdString;	// I wish there is sdUnknown :)
    else
        return statementM->ColumnType(col);
}
//-----------------------------------------------------------------------------
int GridTable::GetNumberCols()
{
    return columnCountM;
}
//-----------------------------------------------------------------------------
int GridTable::GetNumberRows()
{
    return rowsFetchedM;
}
//-----------------------------------------------------------------------------
wxString GridTable::getTableName()
{
    // TODO: using one table is not correct for JOINs or sub-SELECTs, so it 
    //       should take e.g. the one that occurs most often
    if (statementM.intf() == 0 || columnCountM == 0)
        return wxEmptyString;
    else
        return std2wx(statementM->ColumnTable(1));
}
//-----------------------------------------------------------------------------
wxString GridTable::GetValue(int row, int col)
{
    if (row >= (int)dataM.size())
        return wxEmptyString;
    if (col >= (int)dataM[row].size())
        return wxEmptyString;

    // keep up to 200 more rows fetched for better responsiveness
    if (maxRowToFetchM < row + 200)
        maxRowToFetchM = row + 200;

    GridBaseCell* cell = dataM[row][col];
    if (cell)
        return cell->getValue();
    else
        return wxT("[null]");
}
//-----------------------------------------------------------------------------
void GridTable::initialFetch()
{
    Clear();
    allRowsFetchedM = false;
    maxRowToFetchM = 100;
    fetch();
}
//-----------------------------------------------------------------------------
bool GridTable::IsEmptyCell(int row, int col)
{
    return row >= rowsFetchedM || col >= columnCountM;
}
//-----------------------------------------------------------------------------
bool GridTable::isNumericColumn(int col)
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
bool GridTable::needsMoreRowsFetched()
{
    // more rows to fetch in OnIdle() ?
    return (rowsFetchedM < maxRowToFetchM && !allRowsFetchedM);
}
//-----------------------------------------------------------------------------
void GridTable::SetValue(int WXUNUSED(row), int WXUNUSED(col), 
    const wxString& WXUNUSED(value))
{
    // needs to be implemented for editable grid
}
//-----------------------------------------------------------------------------
DEFINE_EVENT_TYPE(wxEVT_FRDG_ROWCOUNT_CHANGED)
//-----------------------------------------------------------------------------
