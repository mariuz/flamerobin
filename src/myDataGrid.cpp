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

  Contributor(s):
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

#include <wx/file.h>
#include <wx/clipbrd.h>
#include <wx/fontdlg.h>
#include <wx/grid.h>
#include "ugly.h"
#include "converters.h"
#include "myDataGrid.h"
//-----------------------------------------------------------------------------
myTableBase::myTableBase(IBPP::Statement &s, wxStatusBar *statusBar):
	wxGridTableBase(), statementM(s)
{
	allRowsFetchedM = false;
	statusBarM = statusBar;
	rowsFetchedM = 0;
	columnCountM = 0;
	limitFetchingM = false;
}
//-----------------------------------------------------------------------------
myTableBase::~myTableBase()
{
	Clear();
}
//-----------------------------------------------------------------------------
void myTableBase::limitFetching(bool limit)
{
	limitFetchingM = limit;
}
//-----------------------------------------------------------------------------
std::string myTableBase::getTableName()
{
	if (statementM.intf() == 0 || columnCountM == 0)
		return "";
	else
		return statementM->ColumnTable(1);
}
//-----------------------------------------------------------------------------
IBPP::SDT myTableBase::columnType(int pos)
{
	if (statementM.intf() == 0 || columnCountM < pos)
		return IBPP::sdString;	// I wish there is sdUnknown :)
	else
		return statementM->ColumnType(pos);
}
//-----------------------------------------------------------------------------
void myTableBase::initialFetch()
{
	allRowsFetchedM = false;	// new dataset
	rowsFetchedM = 0;
	limitFetchingM = true;		// make sure initial fetch doesn't try to do too much
	fetchNewRows(0);
	limitFetchingM = false;
}
//-----------------------------------------------------------------------------
//! fetch new rows if needed, minimum = row that was required for display
//! we always ask 200 rows more than required so that everything runs smooth
void myTableBase::fetchNewRows(int minimum)
{
	if (statementM.intf() == 0)
		return;

	if (allRowsFetchedM || minimum + 200 < rowsFetchedM)		// have enough?
		return;

	if (columnCountM == 0 && GetView())
	{
		columnCountM = statementM->Columns();
		wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_COLS_APPENDED, columnCountM);
		GetView()->ProcessTableMessage(msg);
	}

	wxBusyCursor cr;			// fetch new rows
	int oldrf = rowsFetchedM;
	for (int limit = (limitFetchingM ? 100 : minimum+200); rowsFetchedM < limit; ++rowsFetchedM)
	{
		try
		{
			if (!statementM->Fetch())
				allRowsFetchedM = true;
		}
		catch (IBPP::Exception &e)
		{
			allRowsFetchedM = true;
			::wxMessageBox(std2wx(e.ErrorMessage()), _("An IBPP error occurred."));
		}
		catch (...)
		{
			allRowsFetchedM = true;
			::wxMessageBox(_("A system error occurred."));
		}
		if (allRowsFetchedM)
			break;

		std::vector<myCell> s;
		for (int i=1; i<=statementM->Columns(); i++)
		{
			std::string value;
			bool IsNull = !CreateString(statementM, i, value);
			if (IsNull)
			{
				// TODO: this works slow on large datasets with a lot of NULLs.
				// perhaps we can make it a config option: coloredNulls?
				GetView()->SetCellTextColour(rowsFetchedM, i-1, *wxRED);
				value = "[null]";
			}
			myCell m;
			m.value = value;
			s.push_back(m);
		}
		dataM.push_back(s);
	}

	if (statusBarM)
		statusBarM->SetStatusText(wxString::Format(_("%d rows fetched."), rowsFetchedM), 1);

	if (rowsFetchedM > oldrf && GetView())		// notify the grid
	{
		wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, rowsFetchedM - oldrf);
		GetView()->ProcessTableMessage(msg);
	}
}
//-----------------------------------------------------------------------------
wxString myTableBase::GetColLabelValue(int col)
{
	if (statementM.intf() == 0 || col >= columnCountM)
		return wxEmptyString;
	else
		return std2wx(statementM->ColumnAlias(col+1));
}
//-----------------------------------------------------------------------------
void myTableBase::Clear()
{
	int oldrf = rowsFetchedM;
	int oldcc = columnCountM;
	rowsFetchedM = 0;
	columnCountM = 0;
	dataM.clear();
    if (GetView())
    {
		if (oldrf)
		{
			wxGridTableMessage msg(this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, oldrf);
			GetView()->ProcessTableMessage(msg);
		}
		if (oldcc)
		{
			wxGridTableMessage msg2(this, wxGRIDTABLE_NOTIFY_COLS_DELETED, 0, oldcc);
			GetView()->ProcessTableMessage(msg2);
		}
    }
}
//-----------------------------------------------------------------------------
int myTableBase::GetNumberRows()
{
	if (!allRowsFetchedM)
		fetchNewRows(0);
	return rowsFetchedM;
}
//-----------------------------------------------------------------------------
int myTableBase::GetNumberCols()
{
	return columnCountM;
}
//-----------------------------------------------------------------------------
bool myTableBase::IsEmptyCell(int row, int col)
{
	fetchNewRows(row);
	return dataM[row][col].value.empty();
}
//-----------------------------------------------------------------------------
wxString myTableBase::GetValue(int row, int col)
{
	fetchNewRows(row);
	return std2wx(dataM[row][col].value);
}
//-----------------------------------------------------------------------------
//! should never be called
void myTableBase::SetValue(int WXUNUSED(row), int WXUNUSED(col), const wxString& WXUNUSED(value))
{
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(myDataGrid, wxGrid)
	EVT_GRID_CELL_RIGHT_CLICK(myDataGrid::OnGridCellRightClick)
	//EVT_KEY_DOWN(myDataGrid::OnKeyDown)

	EVT_MENU(myDataGrid::Menu_SetFont, myDataGrid::OnMenuSetFont)
	EVT_MENU(myDataGrid::Menu_Copy2Clipboard, myDataGrid::OnMenuCopy2Clipboard)
	EVT_MENU(myDataGrid::Menu_Copy2ClipboardInsert, myDataGrid::OnMenuCopy2ClipboardInsert)
	EVT_MENU(myDataGrid::Menu_Save2Html, myDataGrid::OnMenuSave2Html)
	//EVT_MENU(myDataGrid::Menu_InsertRecord, myDataGrid::OnMenuInsertRecord)
	//EVT_MENU(myDataGrid::Menu_DeleteRecord, myDataGrid::OnMenuDeleteRecord)
	//EVT_MENU(myDataGrid::Menu_UpdateRecord, myDataGrid::OnMenuUpdateRecord)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
myDataGrid::myDataGrid(wxWindow* parent, wxWindowID id, IBPP::Statement& s, wxStatusBar *statusBar):
    wxGrid(parent, id), tableM(s, statusBar)
{
    EnableEditing(false);
    SetColLabelValue(0, wxT(""));
	SetTable(&tableM, false);

	DisableDragRowSize();
	SetColLabelSize(20);
	SetRowLabelSize(50);
	SetGridLineColour(*wxLIGHT_GREY);
	SetRowLabelAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
}
//-----------------------------------------------------------------------------
#ifdef __BORLANDC__
//! same as ~wxGrid(), except that members are set to zero after deleting
//! only needed for Borland
myDataGrid::~myDataGrid()
{
	SetTargetWindow(this);	// Must do this or ~wxScrollHelper will pop the wrong event handler
    ClearAttrCache();
    wxSafeDecRef(m_defaultCellAttr);
    if (m_ownTable)
	{
        delete m_table;
		m_table = 0;
	}
    delete m_typeRegistry;	// FR crashes if any of these remain dirty
	delete m_selection;		// Since it is a potential memory leak to remove the "delete"s
	m_selection = 0;		// I fixed it by setting members to zero
	m_typeRegistry = 0;
}
#endif
//-----------------------------------------------------------------------------
//! Builds context menu
void myDataGrid::OnGridCellRightClick(wxGridEvent &event)
{
	wxMenu MyMenu(0);
	//MyMenu.Append(Menu_InsertRecord, _("Insert new record"));
	//MyMenu.Append(Menu_DeleteRecord, _("Delete record(s)"));
	//MyMenu.Append(Menu_UpdateRecord, _("Update selected record"));
	//MyMenu.AppendSeparator();
	MyMenu.Append(Menu_Copy2Clipboard, _("Copy to clipboard"));
	MyMenu.Append(Menu_Copy2ClipboardInsert, _("Copy to clipboard as INSERTs"));
	MyMenu.Append(Menu_Save2Html, _("Save selection into HTML file"));
	MyMenu.AppendSeparator();
	MyMenu.Append(Menu_SetFont, _("Set font"));
    PopupMenu(&MyMenu, event.GetPosition());
}
//-----------------------------------------------------------------------------
//! Copy string to clipboard
void myDataGrid::copyToClipboard(wxString& s)
{
	if (s.IsEmpty())
		wxMessageBox(_("Please select the data you wish to copy."),
					 _("Error. No data."), wxOK | wxICON_EXCLAMATION);

	if (wxTheClipboard->Open())
	{
		if (!wxTheClipboard->SetData( new wxTextDataObject(s) ))
			wxMessageBox( _("Cannot write to clipboard"), _("Error"), wxOK | wxICON_EXCLAMATION);
		wxTheClipboard->Close();
	}
	else
		wxMessageBox(_("Cannot open clipboard"), _("Error"), wxOK | wxICON_EXCLAMATION);
}
//-----------------------------------------------------------------------------
// I tried with GetSelectedRows, GetSelectedCells and similar functions
// but they appear to be buggy (don't work sometimes), so don't waste your time
// trying it (at least with 2.4.0 version of wxWidgets)
void myDataGrid::OnMenuCopy2Clipboard(wxCommandEvent& WXUNUSED(event))
{
	wxBusyCursor cr;
	wxString result;
	for (int i=0; i<GetNumberRows(); i++)
	{
		bool first = true;
		for (int j = 0; j < GetNumberCols(); j++)
		{
			if (IsInSelection(i, j))
			{
				if (first)
					first = false;
				else
					result += wxT(" ");
				result += GetCellValue(i, j);
			}
		}

		if (!first)					// was one record
			result += wxT("\n");
	}

	copyToClipboard(result);
}
//-----------------------------------------------------------------------------
void myDataGrid::OnMenuSetFont(wxCommandEvent& WXUNUSED(event))
{
	wxFont f = ::wxGetFontFromUser(this, GetCellFont(1,1));
	if (f.Ok())
	{
		SetCellTextFont(f);
		ForceRefresh();
	}
}
//-----------------------------------------------------------------------------
void myDataGrid::OnMenuCopy2ClipboardInsert(wxCommandEvent& WXUNUSED(event))
{
	wxBusyCursor cr;
	wxString table_name = std2wx(tableM.getTableName());
	wxString column_list(wxT("INSERT INTO "));
	column_list += table_name + wxT(" (");

	int row = 0;
	// find the row with first selected item
	for (bool first = true; row<GetNumberRows() && first; row++)
	{
		for (int j=0; j<GetNumberCols(); j++)
		{
			if (IsInSelection(row, j))		// collect column names based on what is selected
			{
				if (first)
					first = false;
				else
					column_list += wxT(", ");
				column_list += GetColLabelValue(j);			// build insert statement
			}
		}
	}

	// do the "real" thing
	wxString result;
	for (--row; row<GetNumberRows(); row++)
	{
		bool found = false;
		for (int j=0; j<GetNumberCols(); j++)
		{
			if (IsInSelection(row, j))
			{
				if (!found)
				{
					result += column_list + wxT(") VALUES (");
					found = true;
				}
				else
					result += wxT(", ");

				if (GetCellTextColour(row, j) == *wxRED)
					result += wxT("NULL");
				else
				{
					wxString s(GetCellValue(row, j));
					s.Replace(wxT("'"), wxT("''"));		// Escape single quotes
					result += wxT("'") + s + wxT("'");
				}
			}
		}

		if (found)
			result += wxT(");\n");
	}

	copyToClipboard(result);
}
//-----------------------------------------------------------------------------
// perhaps this can be rewritten to give user dialog with options (colors, nulls, ...)
// or we can use CSS, and user can just change everything by changing the CSS style
void myDataGrid::OnMenuSave2Html(wxCommandEvent& WXUNUSED(event))
{
	wxFileDialog fd(this, _("Select file to save"), wxT(""), wxT(""),
		_("HTML files (*.htm*)|*.htm*|All files (*.*)|*.*"),
		wxSAVE | wxCHANGE_DIR
	);

	if (wxID_CANCEL == fd.ShowModal())
		return;

    wxFile f(fd.GetPath(), wxFile::write);
	if (f.IsOpened())
	{
		// header
		f.Write(wxT("<HTML><BODY><TABLE bgcolor=black cellspacing=1 cellpadding=3 border=0><TR>"));
		int row = 0;
		for (bool found = false; row<GetNumberRows() && !found; row++)
		{
			for (int j=0; j<GetNumberCols(); j++)
			{
				if (IsInSelection(row, j))		// column names based on what is selected
				{
					f.Write(wxT("<td nowrap><font color=white><b>") + GetColLabelValue(j) + wxT("<b></font></td>"));
					found = true;
				}
			}
		}
		f.Write(wxT("</TR>"));

		// data
		for (--row; row<GetNumberRows(); row++)
		{
			bool found = false;
			for (int j=0; j<GetNumberCols(); j++)
			{
				if (IsInSelection(row, j))
				{
					if (!found)
					{
						f.Write(wxT("<tr bgcolor=white>"));
						found = true;
					}

					if (GetCellTextColour(row, j) == *wxRED)
						f.Write(wxT("<td><font color=red>NULL</font></td>"));
					else
					{
						f.Write(wxT("<td"));
						int horiz, vert;
						GetCellAlignment(row, j, &horiz, &vert);
						if (horiz == wxALIGN_RIGHT)
							f.Write(wxT(" align=right"));
						f.Write(wxT(" nowrap>") + GetCellValue(row, j) + wxT("</td>"));
					}
				}
			}

			if (found)
				f.Write(wxT("</tr>\n"));
		}

		f.Write(wxT("</table></body></html>\n"));
		f.Close();
	}
}
//-----------------------------------------------------------------------------
//! clear grid, setup columns, fetch records
void myDataGrid::fill()
{
	BeginBatch();
	ClearGrid();
	tableM.initialFetch();

	for (int i=1; i<=GetNumberCols(); ++i)
	{
		wxGridCellAttr *ca;
		switch (tableM.columnType(i))
		{
			case IBPP::sdFloat:
			case IBPP::sdDouble:	// set default formatting (for display) for float/double
									// SetColFormatFloat(i-1, -1, 2);
			case IBPP::sdInteger:
			case IBPP::sdSmallint:
			case IBPP::sdLargeint:
				ca = new wxGridCellAttr;
				ca->SetAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);
				SetColAttr(i-1, ca);
				break;
			default:
				break;
		};
	}

	tableM.limitFetching(true);
	AutoSizeColumns();	// this pulls too many records
	tableM.limitFetching(false);
	EndBatch();
}
//-----------------------------------------------------------------------------
