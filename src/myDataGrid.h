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

#include <wx/wx.h>
#include <wx/grid.h>
#include "ibpp.h"

#ifndef FR_MYDATAGRID_H
#define FR_MYDATAGRID_H

//----------------------------------------------------------------------
//! could be a struct, be we may want to add some functions, like numeric/blob/array/etc.
class myCell
{
public:
	std::string value;
};
//----------------------------------------------------------------------
//! data provider
class myTableBase: public wxGridTableBase
{
private:
	bool limitFetchingM;						// limit the initial fetching (AutosizeCols is slow)
	std::vector< std::vector<myCell> > dataM;
	int rowsFetchedM;							// row count
	int columnCountM;							// column count
	bool allRowsFetchedM;
	void fetchNewRows(int minimum);

	IBPP::Statement& statementM;
	wxStatusBar *statusBarM;

public:
	~myTableBase();
	myTableBase(IBPP::Statement& s, wxStatusBar *statusBar = 0);
	void initialFetch();
	void limitFetching(bool limit = true);

	IBPP::SDT columnType(int pos);
	std::string getTableName();

	int GetNumberRows();
	int GetNumberCols();
	bool IsEmptyCell(int row, int col);
	wxString GetValue(int row, int col);

	void Clear();
	void SetValue(int row, int col, const wxString& value);
	wxString GetColLabelValue(int col);

	// we may need these later to support blobs and arrays in grid
	//wxString GetTypeName(int row, int col) { return wxGRID_VALUE_STRING; };	// not neccessary to overload
	//void* GetValueAsCustom(int row, int col, const wxString& typeName);
	//void SetValueAsCustom(int row, int col, const wxString& typeName, void* value);
};
//----------------------------------------------------------------------
class myDataGrid: public wxGrid {
private:
	myTableBase tableM;
	void copyToClipboard(wxString& s);

	// adding new item to this enum enables PHP code to generate c++ code (menuevents.php)
	// begin PHP enum
    enum {	Menu_Copy2Clipboard, Menu_Copy2ClipboardInsert,
			Menu_Save2Html, Menu_InsertRecord, Menu_DeleteRecord, Menu_UpdateRecord,
			Menu_SetFont
	// end PHP enum
	};

public:
	void fill();
    myDataGrid(wxWindow* parent, wxWindowID id, IBPP::Statement& s, wxStatusBar *statusBar = 0);
#ifdef __BORLANDC__
	~myDataGrid();		// only needed for borland (FR crashes otherwise)
#endif

protected:
	//void OnKeyDown(wxKeyEvent& event);
	void OnGridCellRightClick(wxGridEvent &event);

    //void OnMenuInsertRecord(wxCommandEvent& event);
    //void OnMenuDeleteRecord(wxCommandEvent& event);
    //void OnMenuUpdateRecord(wxCommandEvent& event);
    void OnMenuSetFont(wxCommandEvent& event);
	void OnMenuCopy2Clipboard(wxCommandEvent& event);
    void OnMenuCopy2ClipboardInsert(wxCommandEvent& event);
    void OnMenuSave2Html(wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
};
//----------------------------------------------------------------------
#endif
