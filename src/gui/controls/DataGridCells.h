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
// Cell class to show wxString representation of data
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
// Cell class to show "[...]" for field data without an obvious wxString
// representation of the value (like array or blob fields)
class DataNAGridCell: public GridBaseCell
{
public:
    virtual wxString getValue();
};
//----------------------------------------------------------------------
#endif
