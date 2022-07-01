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

#include <wx/clipbrd.h>
#include <wx/fontdlg.h>
#include <wx/grid.h>
#include <wx/textbuf.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/intl.h>

#include "config/Config.h"
#include "config/LocalSettings.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "gui/AdvancedMessageDialog.h"
#include "gui/CommandIds.h"
#include "gui/controls/DataGrid.h"
#include "gui/controls/DataGridTable.h"
#include "gui/FRLayoutConfig.h"
#include "metadata/database.h"
#include "metadata/table.h"

DataGrid::DataGrid(wxWindow* parent, wxWindowID id)
    : wxGrid(parent, id), timerM(this, TIMER_ID), calculateSumM(true)
{
    // this is necessary for wxWidgets 3.0, otherwise grid will be as wide
    // as the sum of column widths
    SetMinSize(wxSize(100, 50));

    EnableEditing(true);
    SetColLabelValue(0, "");
    SetRowLabelSize(50);
    DisableDragRowSize();
    SetGridLineColour(*wxLIGHT_GREY);
    SetColLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);
    SetRowLabelAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);

    wxString s;
    wxFont f;
    if (config().getValue("DataGridFont", s) && !s.empty())
    {
        f.SetNativeFontInfo(s);
        if (f.Ok())
            SetDefaultCellFont(f);
    }
    if (config().getValue("DataGridHeaderFont", s) && !s.empty())
    {
        f.SetNativeFontInfo(s);
        if (f.Ok())
            SetLabelFont(f);
    }
    updateRowHeights();
}

DataGrid::~DataGrid()
{
}

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

void DataGrid::extendSelection(int direction)
{
    // extend selection of cell selection blocks
    wxGridCellCoordsArray tlCells(GetSelectionBlockTopLeft());
    wxGridCellCoordsArray brCells(GetSelectionBlockBottomRight());
    wxASSERT(tlCells.GetCount() == brCells.GetCount());
    for (size_t i = 0; i < tlCells.GetCount(); ++i)
    {
        wxGridCellCoords tl = tlCells[i];
        wxGridCellCoords br = brCells[i];
        if (direction & wxHORIZONTAL)
        {
            for (int r = tl.GetRow(); r <= br.GetRow(); ++r)
                SelectRow(r, true);
        }
        if (direction & wxVERTICAL)
        {
            for (int c = tl.GetCol(); c <= br.GetCol(); ++c)
                SelectCol(c, true);
        }
    }

    // extend selection of single selected cells
    // if no other selection then extend selection of active cell
    wxGridCellCoordsArray selCells(GetSelectedCells());
    if (!tlCells.GetCount() && !brCells.GetCount() && !selCells.GetCount())
        selCells.Add(wxGridCellCoords(GetGridCursorRow(), GetGridCursorCol()));
    for (size_t i = 0; i < selCells.GetCount(); ++i)
    {
        wxGridCellCoords cc = selCells[i];
        if (direction & wxHORIZONTAL)
            SelectRow(cc.GetRow(), true);
        if (direction & wxVERTICAL)
            SelectCol(cc.GetCol(), true);
    }
}

void DataGrid::fetchData(bool readonly)
{
    DataGridTable* table = getDataGridTable();
    if (!table)
        return;

    wxBusyCursor bc;
    BeginBatch();
    table->initialFetch(readonly);

    for (int i = 0; i < table->GetNumberCols(); i++)
    {
        wxGridCellAttr *ca = new wxGridCellAttr;
        ca->SetAlignment(
            (table->isNumericColumn(i)) ? wxALIGN_RIGHT : wxALIGN_LEFT,
            wxALIGN_TOP);
        if (readonly || table->isReadonlyColumn(i) || table->isBlobColumn(i))
        {
            ca->SetReadOnly(true);
            ca->SetBackgroundColour(frlayoutconfig().getReadonlyColour());
        }
        ca->SetOverflow(false);
        SetColAttr(i, ca);
    }
    AutoSizeColumns(false);
    EndBatch();

    // event handler is only needed if not all rows have already been
    // fetched
    if (table->canFetchMoreRows())
        Connect(wxID_ANY, wxEVT_IDLE, wxIdleEventHandler(DataGrid::OnIdle));

#ifdef __WXGTK__
    // needed to make scrollbars show on large datasets
    Layout();
#endif
}

DataGridTable* DataGrid::getDataGridTable()
{
    DataGridTable* table = dynamic_cast<DataGridTable*>(GetTable());
    return table;
}

void DataGrid::notifyIfUnfetchedData()
{
    DataGridTable* table = getDataGridTable();
    if (table && table->canFetchMoreRows())
    {
        showInformationDialog(wxGetTopLevelParent(this),
            _("Not all records in the result set are copied to the clipboard."),
            _("The result set has unfetched data. Only the records that have already been fetched will be copied. Use the \"Fetch all data\" command in the popup menu first if you want to copy the complete data set."),
            AdvancedMessageDialogButtonsOk(), config(), "DIALOG_InfoClipboardCopyUnfetched",
            _("Do not show this information again"));
    }
}

void DataGrid::showPopupMenu(wxPoint cursorPos)
{
    SetFocus();

    wxMenu m;
    // TODO: merge this with ExecuteSqlFrame's menu
    m.Append(Cmds::DataGrid_FetchAll, _("Fetch all records"));
    m.Append(Cmds::DataGrid_CancelFetchAll, _("Stop fetching all records"));
    m.AppendSeparator();

    m.Append(wxID_COPY, _("Copy"));
    m.Append(Cmds::DataGrid_Copy_with_header, _("Copy with headers"));
    m.Append(Cmds::DataGrid_Copy_as_insert, _("Copy as INSERT statements"));
    m.Append(Cmds::DataGrid_Copy_as_update, _("Copy as UPDATE statements"));
    m.Append(Cmds::DataGrid_Copy_as_inList, _("Copy as IN list"));
    m.Append(Cmds::DataGrid_Save_as_html, _("Save as HTML file..."));
    m.Append(Cmds::DataGrid_Save_as_csv, _("Save as CSV file..."));
    m.AppendSeparator();

    m.Append(Cmds::DataGrid_EditBlob, _("Edit BLOB..."));
    m.Append(Cmds::DataGrid_ImportBlob, _("Import BLOB from file..."));
    m.Append(Cmds::DataGrid_ExportBlob, _("Save BLOB to file..."));
    m.AppendSeparator();

    m.Append(Cmds::DataGrid_SetFieldToNULL, _("Set field to NULL"));
    m.AppendSeparator();

    m.Append(Cmds::DataGrid_Set_header_font, _("Set header font"));
    m.Append(Cmds::DataGrid_Set_cell_font, _("Set cell font"));
    PopupMenu(&m, cursorPos);
}

void DataGrid::updateRowHeights()
{
    // HACK alert: this is taken straight from wxWidgets grid.cpp...
#if defined(__WXGTK__)
    int extraHeight = 8;
#else
    int extraHeight = 4;
#endif

    wxScreenDC dc;
    // adjust height of grid header row (extra 3 pixels for border)
    dc.SetFont(GetLabelFont());
    SetColLabelSize(dc.GetCharHeight() + extraHeight + 3);

    // adjust height of rows (extra pixel for grid lines)
    // and make grid scroll by that amount
    dc.SetFont(GetDefaultCellFont());
    int h = dc.GetCharHeight() + extraHeight + 1;
    SetRowMinimalAcceptableHeight(h);
    SetDefaultRowSize(h, true);
    SetScrollLineY(h);
}

void DataGrid::refreshAndInvalidateAttributes()
{
    ClearAttrCache();
    Refresh();
}

void DataGrid::setCellFont()
{
    wxFont f = ::wxGetFontFromUser(this, GetDefaultCellFont());
    if (f.Ok())
    {
        SetDefaultCellFont(f);
        config().setValue("DataGridFont", f.GetNativeFontInfoDesc());
        updateRowHeights();
        ForceRefresh();
    }
}

void DataGrid::copyToClipboard(bool headers)
{
    DataGridTable* table = getDataGridTable();
    if (!table)
        return;

    bool all = true;
    bool any = false;
    {
        wxBusyCursor cr;
        wxString sRows;
		wxString sHeaders;
		bool bHeaders = headers;
		for (int i = 0; i < GetNumberRows(); i++)
        {
            wxString sRow;
			wxString sHeader;
            bool first = true;
            for (int j = 0; j < GetNumberCols(); j++)
            {
                if (IsInSelection(i, j))
                {
                    // TODO: - align fields in columns ?
                    //       - fields with multiline strings don't really work...
					if (!first)
					{
						sRow += "\t";
						if (bHeaders)
							sHeader += "\t";
					}
                    first = false;
                    sRow += table->getCellValue(i, j);
					if (bHeaders)
						sHeader += table->GetColLabelValue(j);
                    any = true;
                }
                else
                    all = false;
            }
			if (!first) 
			{
				sRows += sRow + wxTextBuffer::GetEOL();
				if (bHeaders) {
					sHeaders += sHeader + wxTextBuffer::GetEOL();
					bHeaders = false;
				}
			}
        }
        if (!sRows.IsEmpty())
            copyToClipboard(sHeaders + sRows);
    }

    if (!any)   // no cells selected -> copy a single cell
    {
        copyToClipboard(table->getCellValue(GetGridCursorRow(),
            GetGridCursorCol()));
    }
    if (all)
        notifyIfUnfetchedData();
}

void DataGrid::copyToClipboardAsInsert()
{
    DataGridTable* table = getDataGridTable();
    if (!table)
        return;

    if (!IsSelection()) // no selection -> select the current cell
    {
        SelectBlock(GetGridCursorRow(), GetGridCursorCol(),
            GetGridCursorRow(), GetGridCursorCol());
    }

    bool all = true;
    {   // begin busy cursor
        wxBusyCursor cr;
        int sqlDialect = table->getDatabase()->getSqlDialect();

        // TODO: - using one table is not correct for JOINs or sub-SELECTs
        //       - should probably refuse to work if not from one table
        //       - should probably refuse to create INSERT for "[...]"
        //       - table&PK info is available in DataGridRows::statementTablesM
        Identifier tableId(table->getTableName(), sqlDialect);

        // load the (quoted if necessary) column names into an array
        wxArrayString columnNames;
        columnNames.Alloc(GetNumberCols());
        for (int i = 0; i < GetNumberCols(); i++)
        {
            Identifier colId(GetColLabelValue(i), sqlDialect);
            columnNames.Add(colId.getQuoted());
        }

        // NOTE: this has been reworked (compared to myDataGrid), because
        //       not all rows have necessarily the same fields selected
        {
            wxString sRows;

            LocalSettings localSet;

            localSet.setDataBaseLenguage();
            
            
            for (int i = 0; i < GetNumberRows(); i++)
            {
                wxString sCols;
                wxString sValues;
                for (int j = 0; j < GetNumberCols(); j++)
                {
                    if (IsInSelection(i, j))
                    {
                        if (!sCols.IsEmpty())
                            sCols += ", ";
                        sCols += columnNames[j];
                        if (!sValues.IsEmpty())
                            sValues += ", ";
                        sValues += table->getCellValueForInsert(i, j);
                    }
                    else
                        all = false;
                }
                if (!sCols.IsEmpty())
                {
                    sRows += "INSERT INTO " + tableId.getQuoted() + " (";
                    sRows += sCols;
                    sRows += ") VALUES (";
                    sRows += sValues;
                    sRows += ");";
                    sRows += wxTextBuffer::GetEOL();
                }
            }
            if (!sRows.IsEmpty())
                copyToClipboard(sRows);
        }
    }   // end busy cursor
    if (all)
        notifyIfUnfetchedData();
}

void DataGrid::copyToClipboardAsInList()
{
    DataGridTable* table = getDataGridTable();
    if (!table)
        return;

    if (!IsSelection()) // no selection -> select the current cell
    {
        SelectBlock(GetGridCursorRow(), GetGridCursorCol(),
            GetGridCursorRow(), GetGridCursorCol());
    }

    bool all = true;
    {   // begin busy cursor
        wxBusyCursor cr;

        wxString s, sLine;
        for (int i = 0; i < GetNumberRows(); i++)
        {
            for (int j = 0; j < GetNumberCols(); j++)
            {
                if (IsInSelection(i, j))
                {
                    if (!sLine.IsEmpty())
                        sLine += ", ";
                    wxString v(table->getCellValueForInsert(i, j));
                    if (sLine.Length() + v.Length() > 80)   // new line
                    {
                        s += sLine + wxTextBuffer::GetEOL();
                        sLine = v;
                    }
                    else
                        sLine += v;
                }
                else
                    all = false;
            }
        }
        s += sLine;   // add the last line
        if (!s.IsEmpty())
            copyToClipboard(s);
    }   // end busy cursor
    if (all)
        notifyIfUnfetchedData();
}

void DataGrid::copyToClipboardAsUpdate()
{

    DataGridTable* table = getDataGridTable();
    if (!table)
        return;

    if (!IsSelection()) // no selection -> select the current cell
    {
        SelectBlock(GetGridCursorRow(), GetGridCursorCol(),
            GetGridCursorRow(), GetGridCursorCol());
    }

    bool all = true;
    {   // begin busy cursor
        wxBusyCursor cr;
        int sqlDialect = table->getDatabase()->getSqlDialect();
        // TODO: - using one table is not correct for JOINs or sub-SELECTs
        //       - should probably refuse to work if not from one table
        //       - should probably refuse to create UPDATE for "[...]"
        //       - we have this info in DataGridRows::statementTablesM
        Identifier tableId(table->getTableName(), sqlDialect);

        // load the (quoted if necessary) column names into an array
        wxArrayString columnNames;
        columnNames.Alloc(GetNumberCols());
        for (int i = 0; i < GetNumberCols(); i++)
        {
            Identifier colId(GetColLabelValue(i), sqlDialect);
            columnNames.Add(colId.getQuoted());
        }

        {
            LocalSettings localSet;
            localSet.setDataBaseLenguage();

            wxString sRows;


            for (int i = 0; i < GetNumberRows(); i++)
            {
                wxString str;
                for (int j = 0; j < GetNumberCols(); j++)
                {
                    if (IsInSelection(i, j))
                    {
                        if (!str.IsEmpty())
                            str += ", ";
                        str += wxTextBuffer::GetEOL() + columnNames[j]
                            + " = " + table->getCellValueForInsert(i, j);
                    }
                    else
                        all = false;
                }
                if (!str.IsEmpty())
                {
                    wxString where;
                    // find primary key (otherwise use all values)
                    Table* t = 0;
                    Database* db = table->getDatabase();
                    if (db)
                    {
                        t = dynamic_cast<Table*>(
                            db->findByNameAndType(ntTable, tableId.get()));
                    }
                    if (!t)
                    {
                        wxMessageBox(wxString::Format(
                            _("Table %s cannot be found in database."),
                            tableId.get().c_str()),
                            _("Error"), wxOK | wxICON_ERROR);
                        return;
                    }
                    PrimaryKeyConstraint* pkc = t->getPrimaryKey();
                    // check if all PK components are available
                    if (pkc)
                    {
                        for (ColumnConstraint::const_iterator ci = pkc->begin();
                            ci != pkc->end(); ++ci)
                        {
                            bool found = false;
                            for (int k = 0; k < GetNumberCols(); k++)
                            {
                                if ((*ci) == GetColLabelValue(k))
                                {
                                    if (!where.IsEmpty())
                                        where += " AND ";
                                    where += (*ci) + " = " +
                                        table->getCellValueForInsert(i, k);
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                pkc = 0;    // as if PK doesn't exists
                                break;
                            }
                        }
                    }
                    // TODO: if (!pkc)   // WHERE all_cols = all_vals

                    sRows += "UPDATE " + tableId.getQuoted() + " SET "
                        + str + wxTextBuffer::GetEOL() + "WHERE " + where
                        + ";" + wxTextBuffer::GetEOL();
                }
            }
            if (!sRows.IsEmpty())
                copyToClipboard(sRows);
        }

    }   // end busy cursor
    if (all)
        notifyIfUnfetchedData();
}

void DataGrid::setHeaderFont()
{
    wxFont f = ::wxGetFontFromUser(this, GetLabelFont());
    if (f.Ok())
    {
        SetLabelFont(f);
        config().setValue("DataGridHeaderFont",
            f.GetNativeFontInfoDesc());
        updateRowHeights();
        AutoSizeColumns(false);
        ForceRefresh();
    }
}

void DataGrid::saveAsCSV(const wxString& fileName,
    const wxChar& fieldDelimiter, const wxChar& textDelimiter)
{
    DataGridTable* table = getDataGridTable();
    if (!table)
        return;

    if (fileName.empty())
        return;

    bool all = true;
    {
        wxBusyCursor cr;
        // find all columns that have at least one cell selected
        std::vector<bool> selCols(getColumnsWithSelectedCells());
        if (std::find(selCols.begin(), selCols.end(), false) != selCols.end())
            all = false;

        // write CSV file
        wxFileOutputStream fos(fileName);
        if (!fos.Ok()) // TODO: report error
            return;
        wxTextOutputStream outStr(fos);

        // wxTextOutputStream (which is used to write the CSV file) will
        // convert '\n' to the proper EOL sequence while writing the stream
        const wxString sEOL("\n");
        const wxString sFieldDelim(fieldDelimiter);
        const wxString sTextDelim =
            (textDelimiter != '\0') ? wxString(textDelimiter) : "";

        wxString sHeader;
        for (size_t col = 0; col < selCols.size(); col++)
        {
            if (!sHeader.empty())
                sHeader += sFieldDelim;
            sHeader += sTextDelim + GetColLabelValue(col) + sTextDelim;
        }
        if (!sHeader.empty())
            outStr.WriteString(sHeader + sEOL);

        // find all rows that have at least one cell selected
        std::vector<bool> selRows(getRowsWithSelectedCells());
        if (std::find(selRows.begin(), selRows.end(), false) != selRows.end())
            all = false;
        for (size_t row = 0; row < selRows.size(); row++)
        {
            // export only selected rows
            if (!selRows[row])
                continue;
            wxString sRow;
            for (size_t col = 0; col < selCols.size(); col++)
            {
                if (selCols[col])
                {
                    if (!sRow.empty())
                        sRow += sFieldDelim;
                    sRow += table->getCellValueForCSV(row, col, textDelimiter);
                }
            }
            if (!sRow.empty())
                outStr.WriteString(sRow + sEOL);
        }
    }
    if (all)
        notifyIfUnfetchedData();
}

void DataGrid::saveAsHTML()
{
    wxString fname = ::wxFileSelector(_("Save data in selected cells as"),
        wxEmptyString, wxEmptyString, "*.html",
        _("HTML files (*.html)|*.html|All files (*.*)|*.*"),
        wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT, this);
    if (fname.empty())
        return;

    // find all columns that have at least one cell selected
    std::vector<bool> selCols(getColumnsWithSelectedCells());

    // write HTML file
    wxFileOutputStream fos(fname);
    if (!fos.Ok()) // TODO: report error
        return;
    wxTextOutputStream outStr(fos);

    outStr.WriteString(
        "<html><head><META \
            HTTP-EQUIV=\"Content-Type\" content=\"text/html; charset=");
    outStr.WriteString(getHtmlCharset());
    outStr.WriteString("\"></head>\
        <body bgcolor=white>\
        <table bgcolor=black cellspacing=1 cellpadding=3 border=0>\
            <tr>\n");
    // write table header
    outStr.WriteString("<tr>");
    int cols = GetNumberCols();
    for (int i = 0; i < cols; i++)
    {
        if (selCols[i])
        {
            outStr.WriteString("<td nowrap><font color=white><b>");
            outStr.WriteString(GetColLabelValue(i));
            outStr.WriteString("</b></font></td>");
        }
    }
    outStr.WriteString("</tr>\n");

    DataGridTable* table = getDataGridTable();
    // write table data
    int rows = GetNumberRows();
    for (int i = 0; i < rows; i++)
    {
        std::vector<bool> selCells(getSelectedCellsInRow(i));
        if (std::count(selCells.begin(), selCells.end(), true) == 0)
            continue;

        outStr.WriteString("<tr bgcolor=white>");
        // write data for selected grid cells only
        for (int j = 0; j < cols; j++)
        {
            if (!selCols[j])
                continue;
            if (!selCells[j])
                outStr.WriteString("<td bgcolor=silver>");
            else if (table->isNullCell(i, j))
                outStr.WriteString("<td><font color=red>NULL</font>");
            else
            {
                outStr.WriteString("<td");
                int halign, valign;
                GetCellAlignment(i, j, &halign, &valign);
                if (halign == wxALIGN_RIGHT)
                    outStr.WriteString(" align=right");
                outStr.WriteString(" nowrap>");
                outStr.WriteString(escapeHtmlChars(table->getCellValue(i, j)));
            }
            outStr.WriteString("</td>");
        }
        outStr.WriteString("</tr>\n");
    }
    outStr.WriteString("</table></body></html>\n");
}

void DataGrid::fetchAll()
{
    DataGridTable* table = getDataGridTable();
    if (table)
        table->setFetchAllRecords(true);
}

void DataGrid::cancelFetchAll()
{
    DataGridTable* table = getDataGridTable();
    if (table)
        table->setFetchAllRecords(false);
}

std::vector<bool> DataGrid::getColumnsWithSelectedCells()
{
    // fully selected rows cause all columns to have selected cells
    if (GetSelectedRows().size() > 0)
        return std::vector<bool>(GetNumberCols(), true);

    std::vector<bool> ret(GetNumberCols(), false);
    // first mark all completely selected columns
    wxArrayInt cols(GetSelectedCols());
    for (size_t i = 0; i < cols.size(); i++)
        ret[cols[i]] = true;

    // now mark all columns contained in selected blocks
    wxGridCellCoordsArray blocksTL(GetSelectionBlockTopLeft());
    wxGridCellCoordsArray blocksBR(GetSelectionBlockBottomRight());
    for (size_t i = 0; i < blocksTL.size(); i++)
    {
        const wxGridCellCoords& tl = blocksTL[i];
        const wxGridCellCoords& br = blocksBR[i];
        for (int c = tl.GetCol(); c <= br.GetCol(); c++)
            ret[c] = true;
    }

    // now mark all columns of selected single cells
    wxGridCellCoordsArray cells(GetSelectedCells());
    for (size_t i = 0; i < cells.size(); i++)
    {
        const wxGridCellCoords& c = cells[i];
        ret[c.GetCol()] = true;
    }

    return ret;
}

std::vector<bool> DataGrid::getRowsWithSelectedCells()
{
    // fully selected columns cause all rows to have selected cells
    if (GetSelectedCols().size() > 0)
        return std::vector<bool>(GetNumberRows(), true);

    std::vector<bool> ret(GetNumberRows(), false);
    // first mark all completely selected rows
    wxArrayInt rows(GetSelectedRows());
    for (size_t i = 0; i < rows.size(); i++)
        ret[rows[i]] = true;

    // now mark all rows contained in selected blocks
    wxGridCellCoordsArray blocksTL(GetSelectionBlockTopLeft());
    wxGridCellCoordsArray blocksBR(GetSelectionBlockBottomRight());
    for (size_t i = 0; i < blocksTL.size(); i++)
    {
        const wxGridCellCoords& tl = blocksTL[i];
        const wxGridCellCoords& br = blocksBR[i];
        for (int r = tl.GetRow(); r <= br.GetRow(); r++)
            ret[r] = true;
    }

    // now mark all columns of selected single cells
    wxGridCellCoordsArray cells(GetSelectedCells());
    for (size_t i = 0; i < cells.size(); i++)
    {
        const wxGridCellCoords& c = cells[i];
        ret[c.GetRow()] = true;
    }

    return ret;
}

std::vector<bool> DataGrid::getSelectedCellsInRow(int row)
{
    // check whether row is fully selected
    wxArrayInt rows(GetSelectedRows());
    for (size_t i = 0; i < rows.size(); i++)
    {
        if (rows[i] == row)
            return std::vector<bool>(GetNumberCols(), true);
    }

    std::vector<bool> ret(GetNumberCols(), false);
    // first mark cells of all completely selected columns
    wxArrayInt cols(GetSelectedCols());
    for (size_t i = 0; i < cols.size(); i++)
        ret[cols[i]] = true;

    // now mark all columns contained in selected blocks
    wxGridCellCoordsArray blocksTL(GetSelectionBlockTopLeft());
    wxGridCellCoordsArray blocksBR(GetSelectionBlockBottomRight());
    for (size_t i = 0; i < blocksTL.size(); i++)
    {
        const wxGridCellCoords& tl = blocksTL[i];
        const wxGridCellCoords& br = blocksBR[i];
        if (tl.GetRow() <= row && row <= br.GetRow())
        {
            for (int c = tl.GetCol(); c <= br.GetCol(); c++)
                ret[c] = true;
        }
    }

    // now mark all selected single cells
    wxGridCellCoordsArray cells(GetSelectedCells());
    for (size_t i = 0; i < cells.size(); i++)
    {
        const wxGridCellCoords& c = cells[i];
        if (c.GetRow() == row)
            ret[c.GetCol()] = true;
    }

    return ret;
}

wxGridCellCoordsArray DataGrid::getSelectedCells()
{
    wxGridCellCoordsArray result;
    result = wxGrid::GetSelectedCells();

    // add rows in selection blocks that span all columns
    wxGridCellCoordsArray tlCells(wxGrid::GetSelectionBlockTopLeft());
    wxGridCellCoordsArray brCells(wxGrid::GetSelectionBlockBottomRight());
    wxASSERT(tlCells.GetCount() == brCells.GetCount());
    for (size_t i = 0; i < tlCells.GetCount(); ++i)
    {
        wxGridCellCoords tl = tlCells[i];
        wxGridCellCoords br = brCells[i];
        for (int iCol = tl.GetCol(); iCol <= br.GetCol(); iCol++)
        for (int iRow = tl.GetRow(); iRow <= br.GetRow(); iRow++)
        {
            result.Add(wxGridCellCoords(iRow,iCol));
        }
    }

    if (result.size() == 0)
        result.Add(wxGridCellCoords(wxGrid::GetGridCursorRow(),wxGrid::GetGridCursorCol()));

    return result;
}

BEGIN_EVENT_TABLE(DataGrid, wxGrid)
    EVT_CONTEXT_MENU(DataGrid::OnContextMenu)
    EVT_GRID_CELL_RIGHT_CLICK(DataGrid::OnGridCellRightClick)
    EVT_GRID_LABEL_RIGHT_CLICK(DataGrid::OnGridLabelRightClick)
    EVT_GRID_EDITOR_CREATED(DataGrid::OnEditorCreated)
    EVT_GRID_SELECT_CELL(DataGrid::OnGridCellSelected)
    EVT_GRID_RANGE_SELECT(DataGrid::OnGridRangeSelected)
    //  EVT_GRID_EDITOR_HIDDEN( DataGrid::OnEditorHidden )
    EVT_KEY_DOWN(DataGrid::OnKeyDown)
    EVT_TIMER(DataGrid::TIMER_ID, DataGrid::OnTimer)
#ifdef __WXGTK__
    EVT_MOUSEWHEEL(DataGrid::OnMouseWheel)
    EVT_SCROLLWIN_THUMBRELEASE(DataGrid::OnThumbRelease)
#endif
END_EVENT_TABLE()

void DataGrid::OnGridCellSelected(wxGridEvent& event)
{
    timerM.Start(500, wxTIMER_ONE_SHOT);
    event.Skip();
}

void DataGrid::OnGridRangeSelected(wxGridRangeSelectEvent& event)
{
    timerM.Start(500, wxTIMER_ONE_SHOT);
    event.Skip();
}

DEFINE_EVENT_TYPE(wxEVT_FRDG_SUM)
void DataGrid::OnTimer(wxTimerEvent& WXUNUSED(event))
{
    // calculate sum for all selected fields and show in status bar
    DataGridTable* table = getDataGridTable();
    if (!table || !calculateSumM)
        return;

    double sum = 0;
    bool any = false;
    bool alert = true;
    wxStopWatch sw;;
    for (int j = 0; j < GetNumberCols(); j++)
    {
        if (!table->isNumericColumn(j))
            continue;
        for (int i = 0; i < GetNumberRows(); i++)
        {
            if (IsInSelection(i, j))
            {
                double d;
                wxString val = table->getCellValue(i, j);
                if (val.ToDouble(&d))
                {
                    sum += d;
                    any = true;
                }
            }
            if (alert && sw.Time() > 4000)
            {
                AdvancedMessageDialogButtonsYesNoCancel amb(_("&Abort"),
                    _("&Disable for this grid"), _("&Continue"));
                int res = showQuestionDialog(this,
                    _("Calculating the sum takes too long"),
                    _("FlameRobin automatically calculates the sum of selected numeric values. However, the current calculation seems to be taking too long. Would you like to abort it?"),
                    amb, config(), "DIALOG_DisableGridSum",
                    _("Do not ask this question again"));
                if (res == wxNO)
                    calculateSumM = false;
                if (res == wxYES || res == wxNO)
                    return;
                alert = false;
            }
        }
    }

    if (any)
    {
        // used in frame to update status bar
        wxCommandEvent evt(wxEVT_FRDG_SUM, GetId());
        wxString ss = wxString::Format("Sum: %f", sum);
        // strip trailing zeroes
        ss.Truncate(1 + ss.find_last_not_of("0"));
        ss.Truncate(1 + ss.find_last_not_of("."));
        evt.SetString(ss);
        wxPostEvent(this, evt);
    }
}

void DataGrid::OnEditorCreated(wxGridEditorCreatedEvent& event)
{
    wxTextCtrl *editor = dynamic_cast<wxTextCtrl *>(event.GetControl());
    if (!editor)
        return;

    editor->Connect(wxEVT_KEY_DOWN,
        wxKeyEventHandler(DataGrid::OnEditorKeyDown),
        0, this);
}

void DataGrid::OnEditorKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_BACK)
    {
        wxTextCtrl* editor = dynamic_cast<wxTextCtrl *>(
            event.GetEventObject());
        DataGridTable* table = getDataGridTable();
        if (editor && table && editor->GetValue().empty())
        {
            table->setNullFlag(true);
            editor->SetValue("[null]");
            editor->SetSelection(-1,-1);
            return; // prevent control from deleting [null]
        }
    }
    event.Skip();
}

void DataGrid::OnContextMenu(wxContextMenuEvent& event)
{
    if (IsCellEditControlEnabled())
    {
        event.Skip();
        return;
    }

    // this doesn't work properly when cell editor is active
    //  showPopMenu(event.GetPosition());
    showPopupMenu(ScreenToClient(::wxGetMousePosition()));
}

void DataGrid::OnGridCellRightClick(wxGridEvent& event)
{
    if (IsCellEditControlEnabled())
    {
        event.Skip();
        return;
    }
    showPopupMenu(event.GetPosition());
}

void DataGrid::OnGridLabelRightClick(wxGridEvent& WXUNUSED(event))
{
    showPopupMenu(ScreenToClient(::wxGetMousePosition()));
}

/*
void DataGrid::OnGridSelectCell(wxGridEvent& event)
{
    DataGridTable* table = getDataGridTable();
    if (table)
        table->saveEditorChanges(event.GetRow());
    event.Skip();
}*/

void DataGrid::OnIdle(wxIdleEvent& event)
{
    DataGridTable* table = getDataGridTable();
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
        AdjustScrollbars();
    }
}

void DataGrid::OnKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_SPACE)
    {
        if (event.GetModifiers() == wxMOD_CONTROL)
        {
            extendSelection(wxVERTICAL);
            return;
        }
        if (event.GetModifiers() == wxMOD_SHIFT)
        {
            extendSelection(wxHORIZONTAL);
            return;
        }
    }

    if ((event.GetKeyCode() == WXK_HOME || event.GetKeyCode() == WXK_END)
        && !event.ControlDown())
    {
        wxGridCellCoords c(GetGridCursorRow(),
            (event.GetKeyCode() == WXK_END) ? GetNumberCols() - 1 : 0);
        SetCurrentCell(c);
        MakeCellVisible(c);
        return;
    }

    event.Skip();
}

void DataGrid::OnMouseWheel(wxMouseEvent& event)
{
    int wheelrotation = event.GetWheelRotation();
    int x, y;
    GetViewStart(&x, &y);
    if (wheelrotation < 0)
        y += 5;
    else
        y -= 5;
    Scroll(x,y);
    AdjustScrollbars();
}

void DataGrid::OnThumbRelease(wxScrollWinEvent& event)
{
    wxIdleEvent dummy;
    OnIdle(dummy);
    event.Skip();
}
