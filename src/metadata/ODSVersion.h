/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#ifndef FR_ODSVERSION_H
#define FR_ODSVERSION_H

namespace fr
{

struct ODSVersion
{
    static constexpr int ODS_FB15_MAJOR = 10;
    static constexpr int ODS_FB15_MINOR = 1;

    static constexpr int ODS_FB20_MAJOR = 11;
    static constexpr int ODS_FB20_MINOR = 0;

    static constexpr int ODS_FB21_MAJOR = 11;
    static constexpr int ODS_FB21_MINOR = 1;

    static constexpr int ODS_FB25_MAJOR = 11;
    static constexpr int ODS_FB25_MINOR = 2;

    static constexpr int ODS_FB30_MAJOR = 12;
    static constexpr int ODS_FB30_MINOR = 0;

    static constexpr int ODS_FB40_MAJOR = 13;
    static constexpr int ODS_FB40_MINOR = 0;

    static constexpr int ODS_FB50_MAJOR = 13;
    static constexpr int ODS_FB50_MINOR = 1;

    static constexpr int ODS_FB60_MAJOR = 14;
    static constexpr int ODS_FB60_MINOR = 0;
};

} // namespace fr

#endif
