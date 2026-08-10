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

#ifndef FR_FRINT128_H
#define FR_FRINT128_H

#include <wx/wx.h>
#include <cstdint>

#ifdef HAVE_INT128
typedef __int128 int128_t;
#else
struct int128_t
{
    uint64_t lowPart;
    int64_t highPart;

    int128_t() : lowPart(0), highPart(0) {}
    int128_t(int64_t val)
    {
        if (val >= 0)
        {
            lowPart = val;
            highPart = 0;
        }
        else
        {
            lowPart = (uint64_t)val;
            highPart = -1;
        }
    }
    int128_t(int val) : int128_t((int64_t)val) {}

    bool operator<(const int128_t& other) const
    {
        if (highPart != other.highPart)
            return highPart < other.highPart;
        return lowPart < other.lowPart;
    }

    bool operator>(const int128_t& other) const
    {
        return other < *this;
    }

    bool operator==(const int128_t& other) const
    {
        return lowPart == other.lowPart && highPart == other.highPart;
    }

    bool operator!=(const int128_t& other) const
    {
        return !(*this == other);
    }

    bool operator>=(const int128_t& other) const
    {
        return !(*this < other);
    }

    bool operator<=(const int128_t& other) const
    {
        return !(other < *this);
    }

    int128_t operator-() const
    {
        int128_t res;
        res.lowPart = ~lowPart;
        res.highPart = ~highPart;
        res.lowPart++;
        if (res.lowPart == 0)
        {
            res.highPart++;
        }
        return res;
    }

    int128_t operator+(const int128_t& other) const
    {
        int128_t res;
        res.lowPart = lowPart + other.lowPart;
        res.highPart = highPart + other.highPart;
        if (res.lowPart < lowPart)
        {
            res.highPart++;
        }
        return res;
    }

    int128_t operator-(const int128_t& other) const
    {
        return *this + (-other);
    }

    int128_t operator<<(int shift) const
    {
        if (shift <= 0) return *this;
        if (shift >= 128) return int128_t(0);
        
        int128_t res;
        if (shift >= 64)
        {
            res.highPart = (int64_t)(lowPart << (shift - 64));
            res.lowPart = 0;
        }
        else
        {
            res.highPart = (highPart << shift) | (int64_t)(lowPart >> (64 - shift));
            res.lowPart = lowPart << shift;
        }
        return res;
    }

    int128_t operator&(const int128_t& other) const
    {
        int128_t res;
        res.lowPart = lowPart & other.lowPart;
        res.highPart = highPart & other.highPart;
        return res;
    }
};
#endif

wxString Int128ToString(int128_t value);
bool StringToInt128(const wxString& src, int128_t* dst, wxString& errMsg);

#endif // FR_FRINT128_H
