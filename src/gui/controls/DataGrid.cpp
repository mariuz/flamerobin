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

#include <wx/clipbrd.h>
#include <wx/fontdlg.h>
#include <wx/grid.h>
#include <wx/textbuf.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include "config/Config.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "gui/AdvancedMessageDialog.h"
#include "gui/ExecuteSqlFrame.h"
#include "gui/controls/DataGrid.h"
#include "gui/controls/DataGridTable.h"
#include "metadata/database.h"
#include "metadata/table.h"
//-----------------------------------------------------------------------------
DataGrid::DataGrid(wxWindow* parent, wxWindowID id)
    : wxGrid(parent, id)
{
    EnableEditing(true);
    SetColLabelValue(0, wxT(""));
    SetRowLabelSize(50);
    DisableDragRowSize();
    SetGridLineColour(*wxLIGHT_GREY);
    SetRowLabelAlignment(wxALIGN_RIGHT, wxALIGN_CENTRE);

    wxString s;
    wxFont f;
    if (config().getValue(wxT("DataGridFont"), s) && !s.empty())
    {
        f.SetNativeFontInfo(s);
        if (f.Ok())
            SetDefaultCellFont(f);
    }
    if (config().getValue(wxT("DataGridHeaderFont"), s) && !s.empty())
    {
        f.SetNativeFontInfo(s);
        if (f.Ok())
            SetLabelFont(f);
    }
    updateRowHeights();
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
void DataGrid::fetchData(wxMBConv* conv)
{
    DataGridTable* table = getDataGridTable();
    if (!table)
        return;

    wxBusyCursor bc;
    BeginBatch();
    table->initialFetch(conv);

    for (int i = 0; i < table->GetNumberCols(); i++)
    {
        wxGridCellAttr *ca = new wxGridCellAttr;
        ca->SetAlignment(
            (table->isNumericColumn(i)) ? wxALIGN_RIGHT : wxALIGN_LEFT,
            wxALIGN_CENTRE);
        ca->SetBackgroundColour(
            (table->isReadonlyColumn(i)) ? wxColour(240, 240, 240):*wxWHITE);
        SetColAttr(i, ca);
    }
    AutoSizeColumns();
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
//-----------------------------------------------------------------------------
DataGridTable* DataGrid::getDataGridTable()
{
    DataGridTable* table = dynamic_cast<DataGridTable*>(GetTable());
    return table;
}
//-----------------------------------------------------------------------------
void DataGrid::notifyIfUnfetchedData()
{
    DataGridTable* table = getDataGridTable();
    if (table && table->canFetchMoreRows())
    {
        showInformationDialog(wxGetTopLevelParent(this),
            _("Not all records in the result set are copied to the clipboard."),
            _("The result set has unfetched data. Only the records that have already been fetched will be copied. Use the \"Fetch all data\" command in the popup menu first if you want to copy the complete data set."),
            AdvancedMessageDialogButtonsOk(), config(), wxT("DIALOG_InfoClipboardCopyUnfetched"),
            _("Do not show this information again"));
    }
}
//-----------------------------------------------------------------------------
void DataGrid::showPopMenu(wxPoint cursorPos)
{
    wxMenu m(0);
    m.Append(ID_MENU_FETCHALL, _("Fetch all records"));
    m.Append(ID_MENU_CANCELFETCHALL, _("Stop fetching all records"));
    m.AppendSeparator();

    m.Append(ID_MENU_COPYTOCLIPBOARD, _("Copy"));
    m.Append(ID_MENU_COPYTOCLIPBOARDASINSERT, _("Copy as INSERT statements"));
    m.Append(ID_MENU_COPYTOCLIPBOARDASUPDATE, _("Copy as UPDATE statements"));
    m.Append(ID_MENU_SAVEASHTML, _("Save as HTML file..."));
    m.Append(ID_MENU_SAVEASCSV, _("Save as CSV file..."));
    m.AppendSeparator();

    m.Append(ID_MENU_LABELFONT, _("Set header font"));
    m.Append(ID_MENU_CELLFONT, _("Set cell font"));
    PopupMenu(&m, cursorPos);
}
//-----------------------------------------------------------------------------
void DataGrid::updateRowHeights()
{
    // HACK alert: this is taken straight from wxWidgets grid.cpp...
#if defined(__WXMOTIF__) || defined(__WXGTK__)
    int extraHeight = 8;
#else
    int extraHeight = 4;
#endif

    wxScreenDC dc;
    // adjust height of grid header row (extra 2 pixels for border)
    dc.SetFont(GetLabelFont());
    SetColLabelSize(dc.GetCharHeight() + extraHeight + 2);

    // adjust height of rows, and make grid scroll by that amount
    dc.SetFont(GetDefaultCellFont());
    int h = dc.GetCharHeight() + extraHeight;
    SetRowMinimalAcceptableHeight(h);
    SetDefaultRowSize(h, true);
    SetScrollLineY(h);
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(DataGrid, wxGrid)
    EVT_CONTEXT_MENU(DataGrid::OnContextMenu)
    EVT_GRID_CELL_RIGHT_CLICK(DataGrid::OnGridCellRightClick)
    EVT_GRID_LABEL_RIGHT_CLICK(DataGrid::OnGridLabelRightClick)
//    EVT_GRID_SELECT_CELL(DataGrid::OnGridSelectCell)
    EVT_MENU(DataGrid::ID_MENU_CANCELFETCHALL, DataGrid::OnMenuCancelFetchAll)
    EVT_UPDATE_UI(DataGrid::ID_MENU_CANCELFETCHALL, DataGrid::OnMenuUpdateCancelFetchAll)
    EVT_MENU(DataGrid::ID_MENU_CELLFONT, DataGrid::OnMenuCellFont)
    EVT_MENU(DataGrid::ID_MENU_COPYTOCLIPBOARD, DataGrid::OnMenuCopyToCB)
    EVT_UPDATE_UI(DataGrid::ID_MENU_COPYTOCLIPBOARD, DataGrid::OnMenuUpdateIfHasSelection)
    EVT_MENU(DataGrid::ID_MENU_COPYTOCLIPBOARDASINSERT, DataGrid::OnMenuCopyToCBAsInsert)
    EVT_UPDATE_UI(DataGrid::ID_MENU_COPYTOCLIPBOARDASINSERT, DataGrid::OnMenuUpdateIfHasSelection)
    EVT_MENU(DataGrid::ID_MENU_COPYTOCLIPBOARDASUPDATE, DataGrid::OnMenuCopyToCBAsUpdate)
    EVT_UPDATE_UI(DataGrid::ID_MENU_COPYTOCLIPBOARDASUPDATE, DataGrid::OnMenuUpdateIfHasSelection)
    EVT_MENU(DataGrid::ID_MENU_FETCHALL, DataGrid::OnMenuFetchAll)
    EVT_UPDATE_UI(DataGrid::ID_MENU_FETCHALL, DataGrid::OnMenuUpdateFetchAll)
//	EVT_GRID_EDITOR_HIDDEN( DataGrid::OnEditorHidden )
    EVT_MENU(DataGrid::ID_MENU_LABELFONT, DataGrid::OnMenuLabelFont)
    EVT_MENU(DataGrid::ID_MENU_SAVEASHTML, DataGrid::OnMenuSaveAsHTML)
    EVT_UPDATE_UI(DataGrid::ID_MENU_SAVEASHTML, DataGrid::OnMenuUpdateIfHasSelection)
    EVT_MENU(DataGrid::ID_MENU_SAVEASCSV, DataGrid::OnMenuSaveAsCSV)
    EVT_UPDATE_UI(DataGrid::ID_MENU_SAVEASCSV, DataGrid::OnMenuUpdateIfHasSelection)

#ifdef __WXGTK__
    EVT_MOUSEWHEEL(DataGrid::OnMouseWheel)
    EVT_SCROLLWIN_THUMBRELEASE(DataGrid::OnThumbRelease)
#endif
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
/*
void DataGrid::OnGridSelectCell(wxGridEvent& event)
{
    FR_TRY

    DataGridTable* table = getDataGridTable();
    if (table)
        table->saveEditorChanges(event.GetRow());
    event.Skip();

    FR_CATCH
}*/
//-----------------------------------------------------------------------------
void DataGrid::OnIdle(wxIdleEvent& event)
{
    FR_TRY

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

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuCellFont(wxCommandEvent& WXUNUSED(event))
{
    wxFont f = ::wxGetFontFromUser(this, GetDefaultCellFont());
    if (f.Ok())
    {
        SetDefaultCellFont(f);
        config().setValue(wxT("DataGridFont"), f.GetNativeFontInfoDesc());
        updateRowHeights();
        ForceRefresh();
    }
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuCopyToCB(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    DataGridTable* table = getDataGridTable();
    if (!table)
        return;

    bool all = true;
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
                    sRow += table->getCellValue(i, j);
                }
                else
                    all = false;
            }
            if (!sRow.IsEmpty())
                sRows += sRow + wxTextBuffer::GetEOL();
        }
        if (!sRows.IsEmpty())
            copyToClipboard(sRows);
    }

    if (all)
        notifyIfUnfetchedData();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuCopyToCBAsInsert(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    DataGridTable* table = getDataGridTable();
    if (!table)
        return;

    bool all = true;
    {
        wxBusyCursor cr;

    // TODO: - using one table is not correct for JOINs or sub-SELECTs
    //       - should probably refuse to work if not from one table
    //       - should probably refuse to create INSERT for "[...]"
    //       - table&PK info is available in DataGridRows::statementTablesM
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
                // NOTE: preloading the column names into a local wxString
                //       array might be a worthy optimization
                sCols += GetColLabelValue(j);
                if (!sValues.IsEmpty())
                    sValues += wxT(", ");
                sValues += table->getCellValueForInsert(i, j);
            }
            else
                all = false;
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
    if (all)
        notifyIfUnfetchedData();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuCopyToCBAsUpdate(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    DataGridTable* table = getDataGridTable();
    if (!table)
        return;

    bool all = true;
    {
        wxBusyCursor cr;
    // TODO: - using one table is not correct for JOINs or sub-SELECTs
    //       - should probably refuse to work if not from one table
    //       - should probably refuse to create UPDATE for "[...]"
    //       - we have this info in DataGridRows::statementTablesM
    wxString tableName = table->getTableName();
    wxString sRows;
    for (int i = 0; i < GetNumberRows(); i++)
    {
        wxString str;
        for (int j = 0; j < GetNumberCols(); j++)
        {
            if (IsInSelection(i, j))
            {
                if (!str.IsEmpty())
                    str += wxT(", ");
                // NOTE: preloading the column names into a local wxString
                //       array might be a worthy optimization
                str += wxTextBuffer::GetEOL() + GetColLabelValue(j)
                    + wxT(" = ") + table->getCellValueForInsert(i, j);
            }
            else
                all = false;
        }
        if (!str.IsEmpty())
        {
            wxString where;
            // find primary key (otherwise use all values)
            Table *t = 0;
            wxWindow* parent = GetParent();
            while (parent)
            {
                if (dynamic_cast<ExecuteSqlFrame *>(parent))
                    break;
                parent = parent->GetParent();
            }
            ExecuteSqlFrame* frame = dynamic_cast<ExecuteSqlFrame *>(parent);
            if (frame)
            {
                Database* db = frame->getDatabase();
                if (db)
                {
                    t = dynamic_cast<Table *>(
                        db->findByNameAndType(ntTable, tableName));
                }
            }
            if (!t)
            {
                wxMessageBox(
                    wxString::Format(_("Table %s cannot be found in database."),
                        tableName.c_str()),
                    _("Error"),
                    wxOK|wxICON_ERROR);
                return;
            }
            PrimaryKeyConstraint *pkc = t->getPrimaryKey();
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
                                where += wxT(" AND ");
                            where += (*ci) + wxT(" = ") +
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

            sRows += wxT("UPDATE ") + tableName + wxT(" SET ") + str
                + wxTextBuffer::GetEOL() + wxT("WHERE ") + where
                + wxT(";") + wxTextBuffer::GetEOL();

        }
    }
    if (!sRows.IsEmpty())
        copyToClipboard(sRows);
    }
    if (all)
        notifyIfUnfetchedData();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuLabelFont(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    wxFont f = ::wxGetFontFromUser(this, GetLabelFont());
    if (f.Ok())
    {
        SetLabelFont(f);
        config().setValue(wxT("DataGridHeaderFont"),
            f.GetNativeFontInfoDesc());
        updateRowHeights();
        AutoSizeColumns();
        ForceRefresh();
    }

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuSaveAsCSV(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    DataGridTable* table = getDataGridTable();
    if (!table)
        return;

    wxString fname = ::wxFileSelector(_("Save data in selected cells as"),
        wxEmptyString, wxEmptyString, wxT("*.csv"),
        _("CSV files (*.csv)|*.csv|All files (*.*)|*.*"),
#if wxCHECK_VERSION(2, 8, 0)
        wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT, this);
#else
        wxSAVE | wxCHANGE_DIR | wxOVERWRITE_PROMPT, this);
#endif
    if (fname.empty())
        return;

    bool all = true;
    {
        wxBusyCursor cr;
        // find all columns that have at least one cell selected
        std::vector<bool> selCols;
        int cols = GetNumberCols();
        selCols.reserve(cols);
        for (int j = 0; j < cols; j++)
            selCols.push_back(false);

        int rows = GetNumberRows();
        for (int i = 0; i < rows; i++)
        {
            for (int j = 0; j < cols; j++)
            {
                if (IsInSelection(i, j))
                    selCols[j] = true;
            }
        }

        const wxString sCOMMA(wxT(","));
        const wxString sTAB(wxT("\t"));
        const wxString sAPS(wxT("\""));
        const wxString sEOL(wxT("\n"));
        wxString sDLM = sTAB;

        bool dlmiscomma = false;
        config().getValue(wxT("CSVDelimiterIsComma"), dlmiscomma);
        if (dlmiscomma)
            sDLM = sCOMMA;

        // write CSV file
        wxFileOutputStream fos(fname);
        if (!fos.Ok()) // TODO: report error
            return;
        wxTextOutputStream outStr(fos);

        wxString sHeader;
        for (int j = 0; j < cols; j++)
        {
            if (selCols[j])
            {
                if (!sHeader.IsEmpty())
                    sHeader += sDLM;
                if (sDLM == sCOMMA)
                    sHeader += sAPS + GetColLabelValue(j) + sAPS;
                else
                    sHeader += GetColLabelValue(j);
            }
        }
        if (!sHeader.IsEmpty())
            outStr.WriteString(sHeader + sEOL);

        for (int i = 0; i < rows; i++)
        {
            wxString sRow;
            for (int j = 0; j < cols; j++)
            {
                if (selCols[j])
                {
                    if (!sRow.IsEmpty())
                        sRow += sDLM;
                    if (sDLM == sCOMMA)
                        sRow += table->getCellValueForCSV(i, j);
                    else
                        sRow += GetCellValue(i, j);
                }
                else
                    all = false;
            }
            if (!sRow.IsEmpty())
                outStr.WriteString(sRow + sEOL); // wxTextBuffer::GetEOL();
        }
    }

    if (all)
        notifyIfUnfetchedData();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuSaveAsHTML(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    wxString fname = ::wxFileSelector(_("Save data in selected cells as"),
        wxEmptyString, wxEmptyString, wxT("*.html"),
        _("HTML files (*.html)|*.html|All files (*.*)|*.*"),
#if wxCHECK_VERSION(2, 8, 0)
        wxFD_SAVE | wxFD_CHANGE_DIR | wxFD_OVERWRITE_PROMPT, this);
#else
        wxSAVE|wxCHANGE_DIR|wxOVERWRITE_PROMPT, this);
#endif
    if (fname.empty())
        return;

    // find all columns that have at least one cell selected
    std::vector<bool> selCols;
    int cols = GetNumberCols();
    selCols.reserve(cols);
    for (int i = 0; i < cols; i++)
        selCols.push_back(false);

    int rows = GetNumberRows();
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            if (IsInSelection(i, j))
                selCols[j] = true;
        }
    }

    // write HTML file
    wxFileOutputStream fos(fname);
    if (!fos.Ok()) // TODO: report error
        return;
    wxTextOutputStream outStr(fos);

    outStr.WriteString(wxT(
        "<html><head><META \
            HTTP-EQUIV=\"Content-Type\" content=\"text/html; charset="));
    outStr.WriteString(getHtmlCharset());
    outStr.WriteString(wxT("\"></head>\
        <body bgcolor=white>\
        <table bgcolor=black cellspacing=1 cellpadding=3 border=0>\
            <tr>\n"));
    // write table header
    outStr.WriteString(wxT("<tr>"));
    for (int i = 0; i < cols; i++)
    {
        if (selCols[i])
        {
            outStr.WriteString(wxT("<td nowrap><font color=white><b>"));
            outStr.WriteString(GetColLabelValue(i));
            outStr.WriteString(wxT("</b></font></td>"));
        }
    }
    outStr.WriteString(wxT("</tr>\n"));

    DataGridTable* table = getDataGridTable();
    // write table data
    for (int i = 0; i < rows; i++)
    {
        // check if at least one cell in this row selected
        int selcnt = 0;
        std::vector<bool> selCells(selCols);
        for (int j = 0; j < cols; j++)
        {
            if (IsInSelection(i, j))
                selcnt++;
            else // skip cell even if selection contains this column
                selCells[j] = false;
        }
        if (!selcnt)
            continue;

        outStr.WriteString(wxT("<tr bgcolor=white>"));
        // write data for selected grid cells only
        for (int j = 0; j < cols; j++)
        {
            if (!selCols[j])
                continue;
            if (!selCells[j])
                outStr.WriteString(wxT("<td bgcolor=silver>"));
            else if (table->isNullCell(i, j))
                outStr.WriteString(wxT("<td><font color=red>NULL</font>"));
            else
            {
                outStr.WriteString(wxT("<td"));
                int halign, valign;
                GetCellAlignment(i, j, &halign, &valign);
                if (halign == wxALIGN_RIGHT)
                    outStr.WriteString(wxT(" align=right"));
                outStr.WriteString(wxT(" nowrap>"));
                outStr.WriteString(escapeHtmlChars(table->getCellValue(i, j)));
            }
            outStr.WriteString(wxT("</td>"));
        }
        outStr.WriteString(wxT("</tr>\n"));
    }
    outStr.WriteString(wxT("</table></body></html>\n"));

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuUpdateIfHasSelection(wxUpdateUIEvent& event)
{
    event.Enable(IsSelection());
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuCancelFetchAll(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    DataGridTable* table = getDataGridTable();
    if (table)
        table->setFetchAllRecords(false);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuUpdateCancelFetchAll(wxUpdateUIEvent& event)
{
    FR_TRY

    DataGridTable* table = getDataGridTable();
    event.Enable(table && table->canFetchMoreRows()
        && table->getFetchAllRows());

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuFetchAll(wxCommandEvent& WXUNUSED(event))
{
    FR_TRY

    DataGridTable* table = getDataGridTable();
    if (table)
        table->setFetchAllRecords(true);

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGrid::OnMenuUpdateFetchAll(wxUpdateUIEvent& event)
{
    FR_TRY

    DataGridTable* table = getDataGridTable();
    event.Enable(table && table->canFetchMoreRows()
        && !table->getFetchAllRows());

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGrid::OnMouseWheel(wxMouseEvent& event)
{
    FR_TRY

    int wheelrotation = event.GetWheelRotation();
    int x, y;
    GetViewStart(&x, &y);
    if (wheelrotation < 0)
        y += 5;
    else
        y -= 5;
    Scroll(x,y);
    AdjustScrollbars();

    FR_CATCH
}
//-----------------------------------------------------------------------------
void DataGrid::OnThumbRelease(wxScrollWinEvent& event)
{
    FR_TRY

    wxIdleEvent dummy;
    OnIdle(dummy);
    event.Skip();

    FR_CATCH
}
//-----------------------------------------------------------------------------
