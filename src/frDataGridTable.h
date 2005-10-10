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

#ifndef FR_DATAGRIDTABLE_H
#define FR_DATAGRIDTABLE_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/grid.h>

#include <ibpp.h>

//-----------------------------------------------------------------------------
// this event is sent after new rows have been fetched
BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_LOCAL_EVENT_TYPE(wxEVT_FRDG_ROWCOUNT_CHANGED, 42)
END_DECLARE_EVENT_TYPES()
//-----------------------------------------------------------------------------
class GridBaseCell; // no need to include frDataGridCells.h
//-----------------------------------------------------------------------------
class GridTable: public wxGridTableBase
{
private:
    bool allRowsFetchedM;
    int columnCountM;
    int maxRowToFetchM;
    int rowsFetchedM;

    wxGridCellAttr* nullAttrM;

    std::vector< std::vector<GridBaseCell*> > dataM;
    IBPP::Statement& statementM;
public:
    GridTable(IBPP::Statement& s);
    ~GridTable();

    bool canFetchMoreRows();
    void fetch();
    wxString getCellValueForInsert(int row, int col);
    IBPP::SDT getColumnType(int col);
    wxString getTableName();
    void initialFetch();
    bool isNullCell(int row, int col);
    bool isNumericColumn(int col);
    bool needsMoreRowsFetched();

    // methods of wxGridTableBase
    virtual void Clear();
    virtual wxGridCellAttr* GetAttr(int row, int col, 
        wxGridCellAttr::wxAttrKind kind);
    virtual wxString GetColLabelValue(int col);

    // pure virtual methods of wxGridTableBase
    virtual int GetNumberCols();
    virtual int GetNumberRows();
    virtual wxString GetValue(int row, int col);
    virtual bool IsEmptyCell(int row, int col);
    virtual void SetValue(int row, int col, const wxString& value);
};
//-----------------------------------------------------------------------------
#endif
