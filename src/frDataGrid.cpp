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
#include <wx/fontdlg.h>

#include "frDataGrid.h"
#include "frDataGridTable.h"

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
}
//-----------------------------------------------------------------------------
DataGrid::~DataGrid()
{
}
//-----------------------------------------------------------------------------
void DataGrid::fill()
{
    GridTable* table = dynamic_cast<GridTable*>(GetTable());
    if (!table)
        return;

    wxBusyCursor bc;
   	BeginBatch();
    table->Clear();
    table->fetch();

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
    m.Append(ID_MENU_LABELFONT, _("Set header font"));
    m.Append(ID_MENU_CELLFONT, _("Set cell font"));
    PopupMenu(&m, cursorPos);
}
//-----------------------------------------------------------------------------
void DataGrid::stopFetching()
{
    Disconnect(wxID_ANY, wxEVT_IDLE);
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(DataGrid, wxGrid)
    EVT_CONTEXT_MENU(DataGrid::OnContextMenu)
    EVT_GRID_CELL_RIGHT_CLICK(DataGrid::OnGridCellRightClick)
    EVT_GRID_LABEL_RIGHT_CLICK(DataGrid::OnGridLabelRightClick)
    EVT_MENU(DataGrid::ID_MENU_CELLFONT, DataGrid::OnMenuCellFont)
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
