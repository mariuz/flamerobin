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

#include <wx/clipbrd.h>
#include <wx/fontdlg.h>
#include <wx/grid.h>
#include <wx/textbuf.h>

#include <string>

#include "config.h"
#include "frDataGrid.h"
#include "frDataGridTable.h"
#include "ugly.h"

//-----------------------------------------------------------------------------
DataGrid::DataGrid(wxWindow* parent, wxWindowID id)
    : wxGrid(parent, id)
{
    EnableEditing(false);
    SetColLabelValue(0, wxT(""));
    SetColLabelSize(GetDefaultRowSize());
    SetRowLabelSize(50);
    DisableDragRowSize();
    SetGridLineColour(*wxLIGHT_GREY);
    SetRowLabelAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);

	std::string s;
	wxFont f;
	if (config().getValue("DataGridFont", s) && !s.empty())
	{
		f.SetNativeFontInfo(std2wx(s));
		if (f.Ok())
			SetDefaultCellFont(f);
	}
	if (config().getValue("DataGridHeaderFont", s) && !s.empty())
	{
		f.SetNativeFontInfo(std2wx(s));
		if (f.Ok())
			SetLabelFont(f);
	}
}
//-----------------------------------------------------------------------------
DataGrid::~DataGrid()
{
}
//-----------------------------------------------------------------------------
void DataGrid::copyToClipboard(const wxString cbText)
{
    if (!wxTheClipboard->Open())
    {
        wxMessageBox(_("Cannot open clipboard"), _("Error"), wxOK|wxICON_EXCLAMATION);
        return;
    }

    if (!wxTheClipboard->SetData(new wxTextDataObject(cbText)))
        wxMessageBox(_("Cannot write to clipboard"), _("Error"), wxOK|wxICON_EXCLAMATION);
    wxTheClipboard->Close();
}
//-----------------------------------------------------------------------------
void DataGrid::fill()
{
    GridTable* table = dynamic_cast<GridTable*>(GetTable());
    if (!table)
        return;

    wxBusyCursor bc;
    BeginBatch();
    table->initialFetch();

    for (int i = 1; i <= table->GetNumberCols(); i++)
    {
        if (table->isNumericColumn(i))
        {
            wxGridCellAttr *ca = new wxGridCellAttr;
            ca->SetAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
            SetColAttr(i-1, ca);
        }
    }
    AutoSizeColumns();
    EndBatch();

    // event handler is only needed if not all rows have already been
    // fetched
    if (table->canFetchMoreRows())
    {
        Connect(wxID_ANY, wxEVT_IDLE,
            (wxObjectEventFunction) (wxEventFunction)
            (wxIdleEventFunction)&DataGrid::OnIdle);
    }
}
//-----------------------------------------------------------------------------
void DataGrid::showPopMenu(wxPoint cursorPos)
{
    wxMenu m(0);
    m.Append(ID_MENU_COPYTOCLIPBOARD, _("Copy"));
    m.Append(ID_MENU_COPYTOCLIPBOARDASINSERT, _("Copy as INSERT statements"));
    //	m.Append(Menu_Save2Html, _("Save selection into HTML file"));
    m.AppendSeparator();

    m.Append(ID_MENU_LABELFONT, _("Set header font"));
    m.Append(ID_MENU_CELLFONT, _("Set cell font"));
    PopupMenu(&m, cursorPos);
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(DataGrid, wxGrid)
    EVT_CONTEXT_MENU(DataGrid::OnContextMenu)
    EVT_GRID_CELL_RIGHT_CLICK(DataGrid::OnGridCellRightClick)
    EVT_GRID_LABEL_RIGHT_CLICK(DataGrid::OnGridLabelRightClick)
    EVT_MENU(DataGrid::ID_MENU_CELLFONT, DataGrid::OnMenuCellFont)
    EVT_MENU(DataGrid::ID_MENU_COPYTOCLIPBOARD, DataGrid::OnMenuCopyToCB)
    EVT_UPDATE_UI(DataGrid::ID_MENU_COPYTOCLIPBOARD, DataGrid::OnMenuUpdateIfHasSelection)
    EVT_MENU(DataGrid::ID_MENU_COPYTOCLIPBOARDASINSERT, DataGrid::OnMenuCopyToCBAsInsert)
    EVT_UPDATE_UI(DataGrid::ID_MENU_COPYTOCLIPBOARDASINSERT, DataGrid::OnMenuUpdateIfHasSelection)
    EVT_MENU(DataGrid::ID_MENU_LABELFONT, DataGrid::OnMenuLabelFont)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void DataGrid::OnContextMenu(wxContextMenuEvent& event)
{
    showPopMenu(event.GetPosition());
}
//-----------------------------------------------------------------------------
void DataGrid::OnGridCellRightClick(wxGridEvent& event)
{
    showPopMenu(event.GetPosition());
}
//-----------------------------------------------------------------------------
void DataGrid::OnGridLabelRightClick(wxGridEvent& WXUNUSED(event))
{
    showPopMenu(ScreenToClient(::wxGetMousePosition()));
}
//-----------------------------------------------------------------------------
void DataGrid::OnIdle(wxIdleEvent& event)
{
    GridTable* table = dynamic_cast<GridTable*>(GetTable());
    // disconnect event handler if nothing more to be done, will be
    // re-registered on next successfull execution of select statement
    if (!table || !table->canFetchMoreRows())
    {
        Disconnect(wxID_ANY, wxEVT_IDLE);
        return;
    }
    // fetch more rows until row cache is filled or timeslice is spent, and
    // request another wxEVT_IDLE event if row cache has not been filled
    if (table->needsMoreRowsFetched())
    {
        table->fetch();
        if (table->needsMoreRowsFetched())
            event.RequestMore();
    }
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuCellFont(wxCommandEvent& WXUNUSED(event))
{
    wxFont f = ::wxGetFontFromUser(this, GetDefaultCellFont());
    if (f.Ok())
    {
        SetDefaultCellFont(f);
        ForceRefresh();
    }
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuCopyToCB(wxCommandEvent& WXUNUSED(event))
{
    wxBusyCursor cr;
    wxString sRows;
    for (int i = 0; i < GetNumberRows(); i++)
    {
        wxString sRow;
        for (int j = 0; j < GetNumberCols(); j++)
        {
            if (IsInSelection(i, j))
            {
                // TODO: - align fields in columns ?
                //       - fields with multiline strings don't really work...
                if (!sRow.IsEmpty())
                    sRow += wxT("\t");
                sRow += GetCellValue(i, j);
            }
        }
        if (!sRow.IsEmpty())
            sRows += sRow + wxTextBuffer::GetEOL();
    }
    if (!sRows.IsEmpty())
        copyToClipboard(sRows);
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuCopyToCBAsInsert(wxCommandEvent& WXUNUSED(event))
{
    wxBusyCursor cr;

    GridTable* table = dynamic_cast<GridTable*>(GetTable());
    if (!table)
        return;
    // TODO: - using one table is not correct for JOINs or sub-SELECTs
    //       -> should probably refuse to work if not from one table
    //       - should probably refuse to create INSERT for "[...]"
    wxString tableName = table->getTableName();
    // NOTE: this has been reworked (compared to myDataGrid), because
    //       not all rows have necessarily the same fields selected
    wxString sRows;
    for (int i = 0; i < GetNumberRows(); i++)
    {
        wxString sCols;
        wxString sValues;
        for (int j = 0; j < GetNumberCols(); j++)
        {
            if (IsInSelection(i, j))
            {
                if (!sCols.IsEmpty())
                    sCols += wxT(", ");
                // NOTE: preloading the column names into a local string
                //       array might be a worthy optimization
                sCols += GetColLabelValue(j);
                if (!sValues.IsEmpty())
                    sValues += wxT(", ");
                sValues += table->getCellValueForInsert(i, j);
            }
        }
        if (!sCols.IsEmpty())
        {
            sRows += wxT("INSERT INTO ") + tableName + wxT(" (");
            sRows += sCols;
            sRows += wxT(") VALUES (");
            sRows += sValues;
            sRows += wxT(");");
            sRows += wxTextBuffer::GetEOL();
        }
    }
    if (!sRows.IsEmpty())
        copyToClipboard(sRows);
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuLabelFont(wxCommandEvent& WXUNUSED(event))
{
    wxFont f = ::wxGetFontFromUser(this, GetLabelFont());
    if (f.Ok())
    {
        SetLabelFont(f);
        AutoSizeColumns();
        ForceRefresh();
    }
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuUpdateIfHasSelection(wxUpdateUIEvent& event)
{
    event.Enable(IsSelection());
}
//-----------------------------------------------------------------------------
