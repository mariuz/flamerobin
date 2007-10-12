/*
  Copyright (c) 2004-2007 The FlameRobin Development Team

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
#ifndef FR_MENUS_H
#define FR_MENUS_H
//----------------------------------------------------------------------------
namespace Menus
{
    enum {
        // SQL View:
        View_Editor = 401,
        View_Statistics,
        View_Data,
        View_Split_view,
        View_Wrap_long_lines,
        View_Set_editor_font,
        Find_Selected_Object,
        View_Focus_grid,
        View_Focus_editor,

        // SQL History
        History_Next,
        History_Previous,
        History_Search,

        // SQL Query
        Query_Execute,
        Query_Show_plan,
        Query_Execute_selection,
        Query_Commit,
        Query_Rollback,

        // SQL Data grid
        DataGrid_Insert_row,
        DataGrid_Delete_row,
        DataGrid_FetchAll,
        DataGrid_CancelFetchAll,
        DataGrid_Copy,
        DataGrid_Copy_as_insert,
        DataGrid_Copy_as_update,
        DataGrid_Save_as_html,
        DataGrid_Save_as_csv,
        DataGrid_Set_header_font,
        DataGrid_Set_cell_font,

        // for easier copy/paste of above items (no need to mess with comma
        Last_menu
    };
};
//----------------------------------------------------------------------------
#endif
