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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "core/FRInt128.h"
#include <wx/numformatter.h>

// enable only for debugging
//#define DEBUG_DDU

// 40 half-bytes = max digits for int128-values
#define DOUBLE_DABBLE_BCD_LEN (40 / 2)
#define DOUBLE_DABBLE_INT128_LEN 16
#define DOUBLE_DABBLE_FULL_LEN (DOUBLE_DABBLE_BCD_LEN + DOUBLE_DABBLE_INT128_LEN)
#pragma pack(push)
#pragma pack(1)
typedef union _DOUBLE_DABBLE_UNION
{
    // bytes only
    uint8_t b[DOUBLE_DABBLE_FULL_LEN];
    //uint64_t i64[DOUBLE_DABBLE_FULL_LEN / sizeof(uint64_t)];
    struct
    {
        uint8_t bcd[DOUBLE_DABBLE_BCD_LEN];
        int128_t i128;
    } s;
    struct
    {
        uint8_t bcd[DOUBLE_DABBLE_BCD_LEN];
        uint64_t lowPart;
        uint64_t highPart;

    } shift;

} DOUBLE_DABBLE_UNION;
#pragma pack(pop)

#ifdef DEBUG_DDU
void DDUdbg(DOUBLE_DABBLE_UNION& ddu)
{
    int i1, xByte;
    uint8_t xBit;
    // debug output
    std::cout << std::endl;
    std::cout << _("binary") << std::endl;
    for (i1 = 0; i1 < (DOUBLE_DABBLE_FULL_LEN * 8); i1++)
    {
        xBit = 1 << (7 - (i1 % 8));
        xByte = i1 / 8;
        std::cout << ((ddu.b[xByte] & xBit) > 0) ? 1 : 0;
        if ((i1+1) == DOUBLE_DABBLE_BCD_LEN * 8)
            std::cout << _(" ");
        if ((i1+1) % 4 == 0)
            std::cout << _(" ");
        if (((i1+1) < DOUBLE_DABBLE_BCD_LEN * 8) &&
            ((i1+1) % (4*8) == 0))
            std::cout << std::endl;
        //std::cout << std::endl;
    }
    std::cout << std::endl;
}
#else
#define DDUdbg(x)
#endif

bool DDUinitFromStr(DOUBLE_DABBLE_UNION& ddu, bool &isNegative, const wxString &src, wxString& errMsg)
{
    wxString src2;
    int i1, iByte;
    uint8_t ch;
    wxChar sep1000;

    isNegative = (src.GetChar(0) == _("-"));
    if (isNegative)
        src2 = src.Mid(1);
    else
        src2 = src;

    // replace thousand separators - if used
    if (wxNumberFormatter::GetThousandsSeparatorIfUsed(&sep1000))
        src2.Replace(_(sep1000), _(""));

    // Check: numeric?
    for (i1 = 0; i1 < src2.Length(); i1++)
    {
        ch = src2.GetChar(i1);
        if ((ch < '0') || (ch > '9'))
        {
            errMsg = wxString::Format(
                _("Not numeric. Invalid char (%c) at position %d."),
                ch, i1+1);
            return false;
        }
    }

    // Check: number to big?
    // Its not really precise but prevents a buffer overflow.
    if (src2.Length() > (DOUBLE_DABBLE_BCD_LEN * 2))
    {
        errMsg = _("Int128: Value to big.");
        return false;
    }

    for (i1 = 0; i1 < src2.Length(); i1++)
    {
        iByte = DOUBLE_DABBLE_BCD_LEN - (i1 / 2) - 1;
        ch = (uint8_t)src2.GetChar(src2.Length() - i1 - 1) - (uint8_t)'0';

        if (i1 % 2 == 0)
            ddu.b[iByte] = ch;
        else
            ddu.b[iByte] |= (ch << 4);
    }

    return true;
}

void DDUinitFromI128(DOUBLE_DABBLE_UNION& ddu, bool &isNegative, const int128_t &src)
{
    int128_t src2 = src;

    isNegative = (src2 < 0);
    if (isNegative)
        src2 = -src2;

    ddu.s.i128 = src2;
}

void DDUshr(DOUBLE_DABBLE_UNION& ddu)
{
    int i1;

    // shift right the int128 part as two uint64
    // its faster than doing this byte by byte
    // and we have the correct byte order too.
    ddu.shift.lowPart >>= 1;
    if ((ddu.shift.highPart & 1) > 0)
        ddu.shift.lowPart = ddu.shift.lowPart | ((uint64_t)1 << 63);

    ddu.shift.highPart >>= 1;
    if ((ddu.shift.bcd[DOUBLE_DABBLE_BCD_LEN-1] & 1) > 0)
        ddu.shift.highPart = ddu.shift.highPart | ((uint64_t)1 << 63);

    for (i1 = DOUBLE_DABBLE_BCD_LEN - 1; i1 >= 0; i1--)
    {
        ddu.b[i1] = ddu.b[i1] >> 1;
        // transfer last bit of next uint8_t to high bit of current uint8_t
        if ((i1 > 0) &&
            ((ddu.b[i1-1] & 1) > 0))
            ddu.b[i1] = ddu.b[i1] | (1 << 7);
    }
}

void DDUshl(DOUBLE_DABBLE_UNION& ddu)
{
    int i1;

    for (i1 = 0; i1 < DOUBLE_DABBLE_BCD_LEN; i1++)
    {
        if ((i1 > 0) &&
            ((ddu.b[i1] & (1 << 7)) > 0))
            ddu.b[i1-1] = ddu.b[i1-1] | 1;

        ddu.b[i1] = ddu.b[i1] << 1;
    }
    // shift left the int128 part as two uint64
    // its faster than doing this byte by byte
    // and we have the correct byte order too.
    if ((ddu.shift.highPart & ((uint64_t)1 << 63)) > 0)
        ddu.b[DOUBLE_DABBLE_BCD_LEN-1] = ddu.b[DOUBLE_DABBLE_BCD_LEN-1] | 1;
    ddu.shift.highPart <<= 1;

    if ((ddu.shift.lowPart & ((uint64_t)1 << 63)) > 0)
        ddu.shift.highPart = ddu.shift.highPart | 1;
    ddu.shift.lowPart <<= 1;
}

void DDUsub(DOUBLE_DABBLE_UNION& ddu)
{
    int i1;

    for (i1 = 0; i1 < DOUBLE_DABBLE_BCD_LEN; i1++)
    {
        if ((ddu.b[i1] & 0x0f) >= 8)
            ddu.b[i1] -= 3;
        if ((ddu.b[i1] & 0xf0) >= (8 << 4))
            ddu.b[i1] -= (3 << 4);
    }
}

void DDUadd(DOUBLE_DABBLE_UNION& ddu)
{
    int i1;

    for (i1 = 0; i1 < DOUBLE_DABBLE_BCD_LEN; i1++)
    {
        if ((ddu.b[i1] & 0x0f) >= 5)
            ddu.b[i1] += 3;
        if ((ddu.b[i1] & 0xf0) >= (5 << 4))
            ddu.b[i1] += (3 << 4);
    }
}

bool StringToInt128(const wxString& src, int128_t* dst, wxString& errMsg)
{
    DOUBLE_DABBLE_UNION ddu = {0};
    int i1;
    bool isNegative;

    // use double dabbl algorithm (reverse)
    // initialization
    if (!DDUinitFromStr(ddu, isNegative, src, errMsg))
        return false;

    for (i1 = 0; i1 < 128; i1++)
    {
        DDUshr(ddu);
        DDUsub(ddu);
    }

    // the "src2.Length"-check in DDUinitFromStr is not really precise.
    // So we have to check if all bits could be moved into the 128-bit
    // result.
    if (ddu.shift.bcd[DOUBLE_DABBLE_BCD_LEN - 1] != 0)
    {
        errMsg = _("Int128: Value to big.");
        return false;
    }

    if (isNegative)
    {
        ddu.s.i128 = ddu.s.i128 - 1;
        ddu.shift.highPart = ddu.shift.highPart ^ 0xFFFFFFFFFFFFFFFF;
        ddu.shift.lowPart = ddu.shift.lowPart ^ 0xFFFFFFFFFFFFFFFF;
        // value to small?
        if (ddu.shift.highPart < 0x8000000000000000)
        {
            errMsg = _("Int128: Value to small.");
            return false;
        }
    }
    else
    {
        // value to big?
        if (ddu.shift.highPart >= 0x8000000000000000)
        {
            errMsg = _("Int128: Value to big.");
            return false;
        }
    }

    *dst = ddu.s.i128;
    return true;
}

wxString Int128ToString(int128_t value)
{
    DOUBLE_DABBLE_UNION ddu = {0};
    bool isNegative;
    int i1, iByte;
    uint8_t ch;
    wxString result = _("");

    DDUinitFromI128(ddu, isNegative, value);

    DDUdbg(ddu);
    for (i1 = 0; i1 < 128; i1++)
    {
        if (i1 > 0)
            DDUadd(ddu);

        DDUshl(ddu);
    }

    for (i1 = 0; i1 < (DOUBLE_DABBLE_BCD_LEN * 2); i1++)
    {
        iByte = (i1 / 2);

        if (i1 % 2 == 0)
            ch = (ddu.b[iByte] >> 4);
        else
            ch = ddu.b[iByte] & 0xF;

        // ignore leading zeros
        if ((ch == 0) &&
            (result.IsEmpty()))
            continue;

        // add char to result-string
        ch += (uint8_t)'0';
        result = result + (char)ch;
    }

    if (isNegative)
        result = _("-") + result;

    // special case ... do not return "" for 0
    if (result.IsEmpty())
        return _("0");

    return result;
}
