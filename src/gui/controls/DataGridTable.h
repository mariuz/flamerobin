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

#ifndef FR_DATAGRIDTABLE_H
#define FR_DATAGRIDTABLE_H
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/grid.h>

#include <ibpp.h>
//-----------------------------------------------------------------------------
class wxMBConv;
class GridBaseCell;
//-----------------------------------------------------------------------------
class GridTableCharsetConverter
{
private:
    wxString connectionCharsetM;
    wxMBConv* converterM;
public:
    GridTableCharsetConverter();
    ~GridTableCharsetConverter();

    wxMBConv* getConverter();
    static wxString mapCharset(const wxString& connectionCharset);
    void setConnectionCharset(const wxString& connectionCharset);
};
//-----------------------------------------------------------------------------
// this event is sent after new rows have been fetched
BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_LOCAL_EVENT_TYPE(wxEVT_FRDG_ROWCOUNT_CHANGED, 42)
END_DECLARE_EVENT_TYPES()
//-----------------------------------------------------------------------------
class GridTable: public wxGridTableBase
{
private:
    bool allRowsFetchedM;
    int columnCountM;
    int maxRowToFetchM;
    int rowsFetchedM;

    wxGridCellAttr* nullAttrM;
    wxGridCellAttr* nullAttrNumericM;

    std::vector< std::vector<GridBaseCell*> > dataM;
    IBPP::Statement& statementM;
    GridTableCharsetConverter charsetConverterM;
public:
    GridTable(IBPP::Statement& s);
    ~GridTable();

    bool canFetchMoreRows();
    void fetch();
    wxString getCellValueForInsert(int row, int col);
    IBPP::SDT getColumnType(int col);
    wxString getTableName();
    void initialFetch(const wxString& connectionCharset);
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
