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

#ifndef FR_DATAGRID_H
#define FR_DATAGRID_H

#include <wx/wx.h>
#include <wx/grid.h>
#include <wx/listimpl.cpp>

#include <vector>

class DataGridTable;

BEGIN_DECLARE_EVENT_TYPES()
    // this event is sent when selection is changed and values are summed up
    DECLARE_LOCAL_EVENT_TYPE(wxEVT_FRDG_SUM, 44)
END_DECLARE_EVENT_TYPES()

class DataGrid: public wxGrid
{
private:
    wxTimer timerM;
    enum { TIMER_ID = 3333 };
    bool calculateSumM;

    void copyToClipboard(const wxString cbText);
    void extendSelection(int direction);
    void notifyIfUnfetchedData();
    void showPopupMenu(wxPoint cursorPos);
    void updateRowHeights();
public:
    DataGrid(wxWindow* parent, wxWindowID id);
    ~DataGrid();

    DataGridTable* getDataGridTable();
    void fetchData(bool readonly);
private:
    void OnContextMenu(wxContextMenuEvent& event);
    void OnGridCellRightClick(wxGridEvent& event);
    void OnGridCellSelected(wxGridEvent& event);
    void OnGridLabelRightClick(wxGridEvent& event);
    void OnGridRangeSelected(wxGridRangeSelectEvent& event);
    void OnIdle(wxIdleEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnThumbRelease(wxScrollWinEvent& event);
    void OnEditorCreated(wxGridEditorCreatedEvent& event);
    void OnEditorKeyDown(wxKeyEvent& event);
    void OnTimer(wxTimerEvent& event);
    DECLARE_EVENT_TABLE()
public:
    void copyToClipboard(bool headers);
    void copyToClipboardAsInsert();
    void copyToClipboardAsInList();
    void copyToClipboardAsUpdate();
    void copyToClipboardAsUpdateInsert();
    void saveAsHTML();
    void saveAsCSV(const wxString& fileName,
        const wxChar& fieldDelimiter, const wxChar& textDelimiter);

    void refreshAndInvalidateAttributes();
    void setHeaderFont();
    void setCellFont();

    void cancelFetchAll();
    void fetchAll();

    std::vector<bool> getColumnsWithSelectedCells();
    std::vector<bool> getRowsWithSelectedCells();
    std::vector<bool> getSelectedCellsInRow(int row);
    wxGridCellCoordsArray getSelectedCells();
};

#endif
