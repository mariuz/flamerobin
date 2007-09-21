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

#ifndef FR_DATAGRIDROWBUFFER_H
#define FR_DATAGRIDROWBUFFER_H

#include "ibpp/ibpp.h"
#include "frtypes.h"
//-----------------------------------------------------------------------------
// DataGridRowBuffer class
class DataGridRowBuffer
{
private:
    std::vector<bool> nullFieldsM;
    std::vector<bool> naFieldsM;
    std::vector<uint8_t> dataM;
    std::vector<wxString> stringsM;
public:
    DataGridRowBuffer(unsigned fieldCount);
    DataGridRowBuffer(const DataGridRowBuffer* other);
    ~DataGridRowBuffer();

    wxString getString(unsigned index);
    bool getValue(unsigned offset, double& value);
    bool getValue(unsigned offset, float& value);
    bool getValue(unsigned offset, int& value);
    bool getValue(unsigned offset, int64_t& value);
    bool getValue(unsigned offset, IBPP::DBKey& value, unsigned size);
    bool isFieldNull(unsigned num);
    void setFieldNull(unsigned num, bool isNull);
    bool isFieldNA(unsigned num);
    void setFieldNA(unsigned num, bool isNA);
    void setString(unsigned num, const wxString& value);
    void setValue(unsigned offset, double value);
    void setValue(unsigned offset, float value);
    void setValue(unsigned offset, int value);
    void setValue(unsigned offset, int64_t value);
    void setValue(unsigned offset, IBPP::DBKey value);
};
//-----------------------------------------------------------------------------
#endif
