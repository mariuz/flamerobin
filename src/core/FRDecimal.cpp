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

#include <wx/numformatter.h>

#include "core/FRInt128.h"
#include "core/FRDecimal.h"

#pragma pack(push)
#pragma pack(1)
typedef union _DECFLOAT128_UNION
{
    // dpb = densely packed decimal (firebid)
    struct
    {
        // if higher bits of comb 00/01/10
#ifdef _MSC_VER
        uint64_t declet0 : 10;
        uint64_t declet1 : 10;
        uint64_t declet2 : 10;
        uint64_t declet3 : 10;
        uint64_t declet4 : 10;
        uint64_t declet5 : 10;
        uint64_t declet6a : 4;
        uint64_t declet6b : 6;
        uint64_t declet7 : 10;
        uint64_t declet8 : 10;
        uint64_t declet9 : 10;
        uint64_t decletA : 10;
        uint64_t expLo12 : 12;
        uint64_t decletB : 3; // 0xxx (0-7)
        uint64_t expHi2 : 2;
        uint64_t sign : 1;
#else
        __uint128_t declet0 : 10;
        __uint128_t declet1 : 10;
        __uint128_t declet2 : 10;
        __uint128_t declet3 : 10;
        __uint128_t declet4 : 10;
        __uint128_t declet5 : 10;
        __uint128_t declet6 : 10;
        __uint128_t declet7 : 10;
        __uint128_t declet8 : 10;
        __uint128_t declet9 : 10;
        __uint128_t decletA : 10;
        __uint128_t expLo12 : 12;
        __uint128_t decletB :  3; // 0xxx (0-7)
        __uint128_t expHi2  :  2;
        __uint128_t sign    :  1;
#endif
    } dpd1;
    struct
    {
        // if higher bits of comb 11
#ifdef _MSC_VER
        uint64_t declet0 : 10;
        uint64_t declet1 : 10;
        uint64_t declet2 : 10;
        uint64_t declet3 : 10;
        uint64_t declet4 : 10;
        uint64_t declet5 : 10;
        uint64_t declet6a : 4;
        uint64_t declet6b : 6;
        uint64_t declet7 : 10;
        uint64_t declet8 : 10;
        uint64_t declet9 : 10;
        uint64_t decletA : 10;
        uint64_t expLo12 : 12;
        uint64_t decletB : 1; // 100x (8-9)
        uint64_t expHi2 : 2;
        uint64_t comb11 : 2;
        uint64_t sign : 1;
#else
        __uint128_t declet0 : 10;
        __uint128_t declet1 : 10;
        __uint128_t declet2 : 10;
        __uint128_t declet3 : 10;
        __uint128_t declet4 : 10;
        __uint128_t declet5 : 10;
        __uint128_t declet6 : 10;
        __uint128_t declet7 : 10;
        __uint128_t declet8 : 10;
        __uint128_t declet9 : 10;
        __uint128_t decletA : 10;
        __uint128_t expLo12 : 12;
        __uint128_t decletB :  1; // 100x (8-9)
        __uint128_t expHi2  :  2;
        __uint128_t comb11  :  2;
        __uint128_t sign    :  1;
#endif
    } dpd2;
    struct
    {
        // comb = 1111 infinite (1111) / NaN (
#ifdef _MSC_VER
        uint64_t unused1 : 64;
        uint64_t unused2 : 58;
        uint64_t nantype : 1; // 0 quiet / 1 signalling
        uint64_t nan : 1; // 1 = nan / 0 = infinity
        uint64_t comb1111 : 4;
        uint64_t sign : 1;
#else
        __uint128_t unused : 122;
        __uint128_t nantype : 1; // 0 quiet / 1 signalling
        __uint128_t nan : 1; // 1 = nan / 0 = infinity
        __uint128_t comb1111 : 4;
        __uint128_t sign : 1;
#endif
    } dpd3;
    dec34_t value;
} DECFLOAT128_UNION;

typedef union _DECFLOAT64_UNION
{
    // dpb = densely packed decimal (firebid)
    struct
    {
        // if higher bits of comb 00/01/10
        uint64_t declet0 : 10;
        uint64_t declet1 : 10;
        uint64_t declet2 : 10;
        uint64_t declet3 : 10;
        uint64_t declet4 : 10;
        uint64_t expLo8  :  8;
        uint64_t declet5 :  3; // 0xxx (0-7)
        uint64_t expHi2  :  2;
        uint64_t sign    :  1;
    } dpd1;
    struct
    {
        // if higher bits of comb 11
        uint64_t declet0 : 10;
        uint64_t declet1 : 10;
        uint64_t declet2 : 10;
        uint64_t declet3 : 10;
        uint64_t declet4 : 10;
        uint64_t expLo8  :  8;
        uint64_t declet5 :  1; // 100x (8-9)
        uint64_t expHi2  :  2;
        uint64_t comb11  :  2;
        uint64_t sign    :  1;
    } dpd2;
    struct
    {
        // comb = 1111 infinite (1111) / NaN (
        uint64_t unused1 : 58;
        uint64_t nantype : 1; // 0 quiet / 1 signalling
        uint64_t nan : 1; // 1 = nan / 0 = infinity
        uint64_t comb1111 : 4;
        uint64_t sign : 1;
    } dpd3;
    dec16_t value;
} DECFLOAT64_UNION;
#pragma pack(pop)

typedef struct _DECFLOAT_DEFINITION
{
    uint16_t bitCount;
    uint16_t digitCount;
    uint16_t minExp;
    uint16_t maxExp;
} DECFLOAT_DEFINITION;

const
    DECFLOAT_DEFINITION CDec34DPDDef =
    {
        /*bitCount:*/ 128,
        /*digitCount:*/ 34,
        /*minExp:*/ 6176,
        /* maxExp:*/ 6111
    };
    DECFLOAT_DEFINITION CDec16DPDDef =
    {
        /*bitCount:*/ 64,
        /*digitCount:*/ 16,
        /*minExp:*/ 398,
        /*maxExp:*/ 369
    };

typedef struct _DECFLOAT_DECINFO
{
    bool negative;
    bool isNaN;
    bool isInfinity;
    wxString mantStr;
    int32_t exp;
} DECFLOAT_DECINFO;

// DPD = densely packed decimal
const char* decCharLookup07 = "01234567";
const char* decCharLookup89 = "89";
void AppendDecletCharToStr(char c, wxString& s)
{
    if (!s.IsEmpty() || c != '0')
        s = s + c;
}

void AppendDecletToStr(uint16_t declet, wxString& s)
{
    char decChar[3];
    if ((declet & 0x08) == 0x00) // 0xxx
    {
        // abc/def/0/ghi
        // abc=0-7/def=0-7/ghi=0-7
        decChar[0] = decCharLookup07[(declet >> 7) & 0x07];
        decChar[1] = decCharLookup07[(declet >> 4) & 0x07];
        decChar[2] = decCharLookup07[(declet >> 0) & 0x07];
    }
    else if ((declet & 0x0E) == 0x08) // 100x
    {
        // abc/def/100/i
        // abc=0-7/def=0-7/i=89
        decChar[0] = decCharLookup07[(declet >> 7) & 0x07];
        decChar[1] = decCharLookup07[(declet >> 4) & 0x07];
        decChar[2] = decCharLookup89[(declet >> 0) & 0x01];
    }
    else if ((declet & 0x0E) == 0x0A) // 101x
    {
        // abc/gh/f/101/i
        // abc=0-7/f=89/ghi=0-7
        decChar[0] = decCharLookup07[(declet >> 7) & 0x07];
        decChar[1] = decCharLookup89[(declet >> 4) & 0x01];
        decChar[2] = decCharLookup07[((declet >> 4) & 0x06) | ((declet >> 0) & 0x1)];
    }
    else if ((declet & 0x0E) == 0x0C) // 110x
    {
        // gh/c/def/110/i
        // c=89/def=0-7/ghi=0-7
        decChar[0] = decCharLookup89[(declet >> 7) & 0x01];
        decChar[1] = decCharLookup07[(declet >> 4) & 0x07];
        decChar[2] = decCharLookup07[((declet >> 7) & 0x06) | ((declet >> 0) & 0x1)];
    }
    else if ((declet & 0x6E) == 0x0E) // 00x111x
    {
        // gh/c/00/f/111/i
        // c=89/f=89/ghi=0-7
        decChar[0] = decCharLookup89[(declet >> 7) & 0x01];
        decChar[1] = decCharLookup89[(declet >> 4) & 0x01];
        decChar[2] = decCharLookup07[((declet >> 7) & 0x06) | ((declet >> 0) & 0x1)];
    }
    else if ((declet & 0x6E) == 0x2E) // 01x111x
    {
        // de/c/01/f/111/i
        // c=89/def=0-7/i=89
        decChar[0] = decCharLookup89[(declet >> 7) & 0x01];
        decChar[1] = decCharLookup07[((declet >> 7) & 0x06) | ((declet >> 4) & 0x1)];
        decChar[2] = decCharLookup89[(declet >> 0) & 0x01];
    }
    else if ((declet & 0x6E) == 0x4E) // 10x111x
    {
        // abc/10/f/111/i
        // abc=0-7/f=89/i=89
        decChar[0] = decCharLookup07[(declet >> 7) & 0x07];
        decChar[1] = decCharLookup89[(declet >> 4) & 0x01];
        decChar[2] = decCharLookup89[(declet >> 0) & 0x01];
    }
    else // if ((declet & 0x6E) == 0x6E) // 11x111x
    {
        // --c/11/f/111/i
        // c=89/f=89/i=89
        decChar[0] = decCharLookup89[(declet >> 7) & 0x01];
        decChar[1] = decCharLookup89[(declet >> 4) & 0x01];
        decChar[2] = decCharLookup89[(declet >> 0) & 0x01];
    }
    AppendDecletCharToStr(decChar[0], s);
    AppendDecletCharToStr(decChar[1], s);
    AppendDecletCharToStr(decChar[2], s);
}

std::string ToBits(int128_t v)
{
    std::string bits = "";

    if (v < 0)
    {
        v = -v;
        bits = "1";
    }
    else bits = "0";

#ifdef RELEASE
    int x;
    fixme <<-operator
    for (x = 126; x > 0; x--)
    {
        if ((v & ((int128_t)1 << x)) > 0)
            bits = bits + "1";
        else
            bits = bits + "0";

        if (x > 0)
        {
            if ((x % 32) == 0)
                bits = bits + ":";
            else if ((x % 16) == 0)
                bits = bits + ".";
            else if ((x % 8) == 0)
                bits = bits + " ";
        }
    }
#endif
    return bits;
}

void Dec34DPDToDecInfo(const dec34_t src, DECFLOAT_DECINFO* info)
{
    DECFLOAT128_UNION dfu = {0};
    uint16_t decletB;
    int32_t exp;
    wxString mantStr = _("");

    *info = {0};

    dfu.value = src;
    if (dfu.dpd3.comb1111 == 0xf)
    {
        if (dfu.dpd3.nan == 1)
            info->isNaN = true;
        else
            info->isInfinity = true;
        return;
    }

    // the highest is a special case
    if (dfu.dpd2.comb11 == 0x3)
    {
        exp = dfu.dpd2.expLo12 | (dfu.dpd2.expHi2 << 12);
        decletB = dfu.dpd2.decletB | 8;
    }
    else
    {
        exp = dfu.dpd1.expLo12 | (dfu.dpd1.expHi2 << 12);
        decletB = dfu.dpd1.decletB;
    }
    exp -= CDec34DPDDef.minExp;

    // process declets
    AppendDecletToStr(decletB, mantStr);
    AppendDecletToStr(dfu.dpd1.decletA, mantStr);
    AppendDecletToStr(dfu.dpd1.declet9, mantStr);
    AppendDecletToStr(dfu.dpd1.declet8, mantStr);
    AppendDecletToStr(dfu.dpd1.declet7, mantStr);
#ifdef _MSC_VER
    AppendDecletToStr(dfu.dpd1.declet6a || dfu.dpd1.declet6b << 4, mantStr);
#else
    AppendDecletToStr(dfu.dpd1.declet6, mantStr);
#endif
    AppendDecletToStr(dfu.dpd1.declet5, mantStr);
    AppendDecletToStr(dfu.dpd1.declet4, mantStr);
    AppendDecletToStr(dfu.dpd1.declet3, mantStr);
    AppendDecletToStr(dfu.dpd1.declet2, mantStr);
    AppendDecletToStr(dfu.dpd1.declet1, mantStr);
    AppendDecletToStr(dfu.dpd1.declet0, mantStr);

    // special case ... manStr "" -> 0
    if (mantStr.IsEmpty())
        mantStr = _("0");

    info->exp = exp;
    info->mantStr = mantStr;
    info->negative = (dfu.dpd1.sign == 1);
}

void Dec16DPDToDecInfo(const dec16_t src, DECFLOAT_DECINFO* info)
{
    DECFLOAT64_UNION dfu = {0};
    uint16_t declet5;
    int32_t exp;
    wxString mantStr = _("");

    *info = {0};

    dfu.value = src;
    if (dfu.dpd3.comb1111 == 0xf)
    {
        if (dfu.dpd3.nan == 1)
            info->isNaN = true;
        else
            info->isInfinity = true;
        return;
    }

    // the highest is a special case
    if (dfu.dpd2.comb11 == 0x3)
    {
        exp = dfu.dpd2.expLo8 | (dfu.dpd2.expHi2 << 8);
        declet5 = dfu.dpd2.declet5 | 8;
    }
    else
    {
        exp = dfu.dpd1.expLo8 | (dfu.dpd1.expHi2 << 8);
        declet5 = dfu.dpd1.declet5;
    }
    exp -= CDec16DPDDef.minExp;

    // process declets
    AppendDecletToStr(declet5, mantStr);
    AppendDecletToStr(dfu.dpd1.declet4, mantStr);
    AppendDecletToStr(dfu.dpd1.declet3, mantStr);
    AppendDecletToStr(dfu.dpd1.declet2, mantStr);
    AppendDecletToStr(dfu.dpd1.declet1, mantStr);
    AppendDecletToStr(dfu.dpd1.declet0, mantStr);

    // special case ... manStr "" -> 0
    if (mantStr.IsEmpty())
        mantStr = _("0");

    info->exp = exp;
    info->mantStr = mantStr;
    info->negative = (dfu.dpd1.sign == 1);
}

wxString DecInfoToString(const DECFLOAT_DEFINITION& def, const DECFLOAT_DECINFO info)
{
    wxString result;
    int exp;

    if (info.isNaN)
        return "NaN";
    if (info.isInfinity)
        return "Infinity";

    result = info.mantStr;
    // add sign
    if (info.negative)
        result = _("-") + result;

    exp = info.exp;

    // set decimalseparator
    // if we can make a exponent of 0 we will do it ...
    if ((exp < 0) &&
        (-exp < def.digitCount))
    {
        while (-exp >= result.Length())
            result = _("0") + result;

        result.insert(result.Length() + exp,
                      wxNumberFormatter::GetDecimalSeparator());
        exp = 0;
    }
    if (exp != 0)
        result = result + _(" E") << exp;

    return result;
}

wxString Dec34DPDToString(dec34_t value)
{
    DECFLOAT_DECINFO info;

    Dec34DPDToDecInfo(value, &info);
    return DecInfoToString(CDec34DPDDef, info);
}

// input definition + src
// ouput dstInfo
bool StringToDecParse(const DECFLOAT_DEFINITION& def,
    const wxString& srcStr, DECFLOAT_DECINFO* dstInfo)
{
    wxString valueStr;
    wxString expStr;
    wxString srcStrL;
    wxChar ch;
    int i1, iStart;
    long int tmpVal;
    int DecimalSeparatorPos;
    int DecimalDigits;
    bool NeedExponent;
    wxChar DecimalSeparator = wxNumberFormatter::GetDecimalSeparator();

    *dstInfo = {0};

    if (srcStr.IsEmpty())
        return false;

    srcStrL = srcStr.Lower();
    if (srcStrL == _("nan"))
    {
        dstInfo->isNaN = true;
        return true;
    }
    else if (srcStrL == _("infinity"))
    {
        dstInfo->isInfinity = true;
        return true;
    }

    dstInfo->negative = false;
    ch = srcStr.GetChar(0);
    iStart = 0;
    if ((ch == '+') || (ch == '-'))
    {
        dstInfo->negative = (ch == '-');
        iStart = 1;
    }

    // eat value
    DecimalSeparatorPos = -1;
    valueStr = _("");
    NeedExponent = false;
    for (i1 = iStart; i1 < srcStr.Length(); i1++)
    {
        ch = srcStr.GetChar(i1);
        if (ch == DecimalSeparator)
        {
            // more than one dot?
            if (DecimalSeparatorPos != -1)
                return false;
            DecimalSeparatorPos = i1 - iStart;
            continue;
        }
        if ((ch == ' ') || (ch == 'e') || (ch == 'E'))
        {
            // exponent expected
            NeedExponent = true;
            iStart = i1;
            if (ch == ' ')
                iStart++;
            break;
        }
        // not numeric
        if ((ch < '0') || (ch > '9'))
            return false;
        valueStr = valueStr + ch;
    }
    if (iStart < i1)
        iStart = i1;

    if (iStart >= srcStr.Length())
    {
        // exponent expected (e.g becase trailing space like "123 ")
        if (NeedExponent)
            return false;
        expStr = _("0");
    }
    else
    {
        // eat exponent
        ch = srcStr.GetChar(iStart);
        // E expected ...
        if ((ch != 'e') && (ch != 'E'))
            return false;
        iStart++;

        expStr = _("");
        for (i1 = iStart; i1 < srcStr.Length(); i1++)
        {
            ch = srcStr.GetChar(i1);
            if ((i1 == iStart) &&
                ((ch == '-') || (ch == '+')))
            {
                expStr += ch;
                continue;
            }
            // numeric?
            if ((ch < '0') || (ch > '9'))
                return false;
            expStr += ch;
        }
    }


    if (!expStr.ToLong(&tmpVal, 10))
        return false;

    // Add decimaldigits to exponent
    DecimalDigits = valueStr.Length() - DecimalSeparatorPos;
    tmpVal -= DecimalDigits;

    dstInfo->exp = tmpVal;
    dstInfo->mantStr = valueStr;
    return true;
}

uint8_t GetDigitValueOr0(const wxString& str, int index, int& count89)
{
    uint8_t result = 0;

    if (index >= 0)
        result = (uint8_t)(str.GetChar(index) - '0');

    if (result >= 8)
        count89++;

    return result;
}

#define DECLET89(c, shift) ((c == 9) ? 1 << shift : 0)

uint16_t Str3ToDeclet(const wxString& str, int& offset)
{
    uint8_t decChar[3];
    int count89 = 0;
    uint16_t declet = 0;

    decChar[0] = GetDigitValueOr0(str, offset--, count89);
    decChar[1] = GetDigitValueOr0(str, offset--, count89);
    decChar[2] = GetDigitValueOr0(str, offset--, count89);

    //count89 = decChar[0].is89

    if (count89 == 0) // 0xxx
    {
        // abc/def/0/ghi
        // abc=0-7/def=0-7/ghi=0-7
        declet = (decChar[2] << 7) |
                 (decChar[1] << 4) |
                 0x00 |
                 (decChar[0] << 0);
    }
    else if (count89 == 1)
    {
        if (decChar[0] >= 8) // 100x
        {
            // abc/def/100/i
            // abc=0-7/def=0-7/i=89
            declet = (decChar[2] << 7) |
                     (decChar[1] << 4) |
                     0x08 |
                     (DECLET89(decChar[0], 0));
        }
        else if (decChar[1] >= 8) // 101x
        {
            // abc/gh/f/101/i
            // abc=0-7/f=89/ghi=0-7
            declet = (decChar[2] << 7) |
                     ((decChar[0] & 0x6) << 4) | (DECLET89(decChar[1], 4)) |
                     0x0A |
                     ((decChar[0] & 0x1) << 0);
        }
        else // if (decChar[2] >= 8) // 110x
        {
            // gh/c/def/110/i
            // c=89/def=0-7/ghi=0-7
            declet = ((decChar[0] & 0x6) << 7) | (DECLET89(decChar[2], 7)) |
                     (decChar[1] << 4) |
                     0x0C |
                     ((decChar[0] & 0x1) << 0);
        }
    }
    else if (count89 == 2)
    {
        if (decChar[0] < 8) // 00x111x
        {
            // gh/c/00/f/111/i
            // c=89/f=89/ghi=0-7
            declet = ((decChar[0] & 0x6) << 7) | (DECLET89(decChar[2], 7)) |
                     0x00 |
                     (DECLET89(decChar[1], 4)) |
                     0x0E |
                     (decChar[0] << 0);
        }
        else if (decChar[1] < 8) // 01x111x
        {
            // de/c/01/f/111/i
            // c=89/def=0-7/i=89
            declet = ((decChar[1] & 0x6) << 7) | (DECLET89(decChar[2], 7)) |
                     0x20 |
                     ((decChar[1] & 0x1) << 4) |
                     0x0E |
                     (DECLET89(decChar[0], 0));
        }
        else // if (decChar[2] < 8) // 10x111x
        {
            // abc/10/f/111/i
            // abc=0-7/f=89/i=89
            declet = (decChar[2] << 7) |
                     0x40 |
                     (DECLET89(decChar[1], 4)) |
                     0x0E |
                     (DECLET89(decChar[0], 0));
        }
    }
    else // if (count89 == 2)
    {
        // --c/11/f/111/i
        declet = (DECLET89(decChar[2], 7)) |
                 0x60 |
                 (DECLET89(decChar[1], 4)) |
                 0x0E |
                 (DECLET89(decChar[0], 0));
    }

    return declet;
}

bool DecInfoToDec34DPD(const DECFLOAT_DECINFO &info, dec34_t* dst)
{
    DECFLOAT128_UNION dfu = {0};
    DECFLOAT_DEFINITION def = CDec34DPDDef;
    int mantOfs = info.mantStr.Length()-1;
    uint16_t decletB;

    if (info.isNaN)
    {
        dfu.dpd3.comb1111 = 0xf;
        dfu.dpd3.nan      = 1;
        *dst = dfu.value;
        return true;
    }
    else if (info.isInfinity)
    {
        dfu.dpd3.comb1111 = 0xf;
        dfu.dpd3.nan      = 0;
        *dst = dfu.value;
        return true;
    }

    // exponent in range?
    if ((info.exp < -def.minExp) ||
        (info.exp > def.maxExp))
        return false;

    uint32_t exp = info.exp + def.minExp;

    dfu.dpd1.sign = (info.negative ? 1 : 0);
    dfu.dpd1.declet0 = Str3ToDeclet(info.mantStr, mantOfs);
    dfu.dpd1.declet1 = Str3ToDeclet(info.mantStr, mantOfs);
    dfu.dpd1.declet2 = Str3ToDeclet(info.mantStr, mantOfs);
    dfu.dpd1.declet3 = Str3ToDeclet(info.mantStr, mantOfs);
    dfu.dpd1.declet4 = Str3ToDeclet(info.mantStr, mantOfs);
    dfu.dpd1.declet5 = Str3ToDeclet(info.mantStr, mantOfs);
#ifdef _MSC_VER
    uint32_t declet6 = Str3ToDeclet(info.mantStr, mantOfs);
    dfu.dpd1.declet6a = declet6 & 0xf;
    dfu.dpd1.declet6b = (declet6 & 0x3f) >> 4; 
#else
    dfu.dpd1.declet6 = Str3ToDeclet(info.mantStr, mantOfs);
#endif
    dfu.dpd1.declet7 = Str3ToDeclet(info.mantStr, mantOfs);
    dfu.dpd1.declet8 = Str3ToDeclet(info.mantStr, mantOfs);
    dfu.dpd1.declet9 = Str3ToDeclet(info.mantStr, mantOfs);
    dfu.dpd1.decletA = Str3ToDeclet(info.mantStr, mantOfs);
    decletB = Str3ToDeclet(info.mantStr, mantOfs);
    if ((exp & 0x3FFF) != 0x3000)
    {
        dfu.dpd1.decletB = decletB;
        dfu.dpd1.expLo12 = exp & 0x0FFF;
        dfu.dpd1.expHi2  = (exp & 0x3FFF) >> 12;
    }
    else
    {
        dfu.dpd2.decletB = decletB;
        dfu.dpd2.expLo12 = exp & 0x0FFF;
        dfu.dpd2.expHi2  = (exp & 0x3FFF) >> 12;
    }
    *dst = dfu.value;
    return true;
}

bool DecInfoToDec16DPD(const DECFLOAT_DECINFO &info, dec16_t* dst)
{
    DECFLOAT64_UNION dfu = {0};
    DECFLOAT_DEFINITION def = CDec16DPDDef;
    int mantOfs = info.mantStr.Length()-1;
    uint16_t declet5;

    if (info.isNaN)
    {
        dfu.dpd3.comb1111 = 0xf;
        dfu.dpd3.nan      = 1;
        *dst = dfu.value;
        return true;
    }
    else if (info.isInfinity)
    {
        dfu.dpd3.comb1111 = 0xf;
        dfu.dpd3.nan      = 0;
        *dst = dfu.value;
        return true;
    }

    // exponent in range?
    if ((info.exp < -def.minExp) ||
        (info.exp > def.maxExp))
        return false;

    uint32_t exp = info.exp + def.minExp;

    dfu.dpd1.sign = (info.negative ? 1 : 0);
    dfu.dpd1.declet0 = Str3ToDeclet(info.mantStr, mantOfs);
    dfu.dpd1.declet1 = Str3ToDeclet(info.mantStr, mantOfs);
    dfu.dpd1.declet2 = Str3ToDeclet(info.mantStr, mantOfs);
    dfu.dpd1.declet3 = Str3ToDeclet(info.mantStr, mantOfs);
    dfu.dpd1.declet4 = Str3ToDeclet(info.mantStr, mantOfs);
    declet5 = Str3ToDeclet(info.mantStr, mantOfs);
    if ((exp & 0x3FFF) != 0x3000)
    {
        dfu.dpd1.declet5 = declet5;
        dfu.dpd1.expLo8  = exp & 0x00FF;
        dfu.dpd1.expHi2  = (exp & 0x03FF) >> 8;
    }
    else
    {
        dfu.dpd2.declet5 = declet5;
        dfu.dpd2.expLo8  = exp & 0x00FF;
        dfu.dpd2.expHi2  = (exp & 0x03FF) >> 8;
    }
    *dst = dfu.value;
    return true;
}

bool StringToDec34DPD(const wxString& src, dec34_t* dst)
{
    DECFLOAT_DECINFO info;
    if (!StringToDecParse(CDec34DPDDef, src, &info))
        return false;
    if (!DecInfoToDec34DPD(info, dst))
        return false;
    return true;
}

wxString Dec16DPDToString(dec16_t value)
{
    DECFLOAT_DECINFO info;

    Dec16DPDToDecInfo(value, &info);
    return DecInfoToString(CDec16DPDDef, info);
}

bool StringToDec16DPD(const wxString& src, dec16_t* dst)
{
    DECFLOAT_DECINFO info;
    if (!StringToDecParse(CDec34DPDDef, src, &info))
        return false;
    if (!DecInfoToDec16DPD(info, dst))
        return false;
    return true;
}
