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

#include "_ibpp.h"

#ifndef HAVE_INT128

using namespace IBPP;

void IBPP_INT128_T::AddPart128(uint32_t* dst,
    const uint32_t& toadd, bool* overflow)
{
    uint64_t sum;
    sum = (uint64_t)*dst + toadd;
    if (*overflow)
        sum += 1;
    *dst = (uint32_t)sum;
    *overflow = (sum & 0x100000000);
}

void IBPP_INT128_T::Add128(IBPP_INT128_T* T1,
    const IBPP_INT128_T& T2, bool* overflow)
{
    *overflow = false;
    AddPart128(&T1->data.s4.llPart, T2.data.s4.llPart, overflow);
    AddPart128(&T1->data.s4.hlPart, T2.data.s4.hlPart, overflow);
    AddPart128(&T1->data.s4.lhPart, T2.data.s4.lhPart, overflow);
    AddPart128(&T1->data.s4.hhPart, T2.data.s4.hhPart, overflow);
}

IBPP_INT128_T::IBPP_INT128_T(const int64_t value)
{
    if (value >= 0)
    {
        data.s2.lowPart = value;
        data.s2.highPart = 0;
    }
    else
    {
        data.s2.lowPart = -value;
        data.s2.highPart = 0xFFFFFFFFFFFFFFFF;
    }
}

IBPP_INT128_T IBPP_INT128_T::operator-()
{
    IBPP_INT128_T neg;
    IBPP_INT128_T one = 1;
    bool overflow;

    neg = *this;
    neg.data.us2.highPart = neg.data.us2.highPart ^ 0xFFFFFFFFFFFFFFFF;
    neg.data.us2.lowPart = neg.data.us2.lowPart ^ 0xFFFFFFFFFFFFFFFF;
    Add128(&neg, one, &overflow);
    if (overflow)
      throw std::overflow_error("number to big!");
    return neg;
}

IBPP_INT128_T IBPP_INT128_T::operator-(const IBPP_INT128_T& T2)
{
    bool overflow;
    ibpp_int128_t negT2 = T2;
    Add128(this, -negT2, &overflow);
    if (!overflow)
      throw std::overflow_error("number to big!");
    return *this;
}

bool IBPP_INT128_T::operator<(const IBPP_INT128_T& T2) const
{
    return (data.s2.highPart < T2.data.s2.highPart) ||
           ((data.s2.highPart == T2.data.s2.highPart) &&
            (data.s2.lowPart < T2.data.s2.lowPart));
}

#endif
