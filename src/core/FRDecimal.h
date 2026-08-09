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

#ifndef FR_FRDECIMAL_H
#define FR_FRDECIMAL_H

#include <wx/wx.h>
#include <cstdint>

#ifdef HAVE_DECIMAL128
#include <decimal/decimal>
typedef std::decimal::decimal64 dec16_t;
typedef std::decimal::decimal128 dec34_t;
#else
struct dec16_t
{
    int64_t lowPart;
};
struct dec34_t
{
    uint64_t lowPart;
    uint64_t highPart;
};
#endif

wxString Dec34DPDToString(dec34_t value);
bool StringToDec34DPD(const wxString& src, dec34_t* dst, wxString& errMsg);
wxString Dec16DPDToString(dec16_t value);
bool StringToDec16DPD(const wxString& src, dec16_t* dst, wxString& errMsg);

#endif // FR_FRDECIMAL_H
