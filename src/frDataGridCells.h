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

  The Initial Developer of the Original Code is Michael Hieke.

  Portions created by the original developer
  are Copyright (C) 2004 Michael Hieke.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/

#ifndef FR_DATAGRIDCELLS_H
#define FR_DATAGRIDCELLS_H

#include <wx/wx.h>

//----------------------------------------------------------------------
// Abstract cell base class
class GridBaseCell
{
public:
    virtual ~GridBaseCell();
    virtual wxString getValue() = 0;
};
//----------------------------------------------------------------------
// Cell class to show string representation of data
//   Note that a class containing the data itself would both be cheaper
//   (both memory and cycles) and allow for on-thy-fly changes to the 
//   display format).  Probably also easier to edit in an editable grid.
//   Needs reconsideration in the future...
class DataGridCell: public GridBaseCell
{
private:
    wxString valueM;
public:
    DataGridCell(const wxString& value);
    virtual wxString getValue();
};
//----------------------------------------------------------------------
// Cell class to show "[...]" for field data without an obvious string 
// representation of the value (like array or blob fields)
class DataNAGridCell: public GridBaseCell
{
public:
    virtual wxString getValue();
};
//----------------------------------------------------------------------
#endif
