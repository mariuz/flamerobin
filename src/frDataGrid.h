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

#ifndef FR_DATAGRID_H
#define FR_DATAGRID_H

#include <wx/wx.h>
#include <wx/grid.h>

//----------------------------------------------------------------------
class DataGrid: public wxGrid {
public:
    DataGrid(wxWindow* parent, wxWindowID id);
    ~DataGrid();

    void fill();

public:
    enum { ID_MENU_CELLFONT, ID_MENU_LABELFONT };

    void OnContextMenu(wxContextMenuEvent& event);
    void OnGridCellRightClick(wxGridEvent& event);
    void OnGridLabelRightClick(wxGridEvent& event);
    void OnIdle(wxIdleEvent& event);
    void OnMenuCellFont(wxCommandEvent& event);
    void OnMenuLabelFont(wxCommandEvent& event);
private:
    void showPopMenu(wxPoint cursorPos);

    DECLARE_EVENT_TABLE()
};
//----------------------------------------------------------------------
#endif
