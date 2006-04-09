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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <fstream>
#include <sstream>
#include <iomanip>

#include "config/Config.h"
#include "converters.h"
#include "ugly.h"
//--------------------------------------------------------------------------------------
//! formats date according to DateFormat wxString
wxString GetHumanDate(int year, int month, int day, wxString DateFormat)
{
    wxString value;
    for (wxString::iterator c = DateFormat.begin(); c != DateFormat.end(); c++)
    {
        int var = -1;
        wxString format = wxT("%02d");
        switch (*c)
        {
            case 'd':    format = wxT("%d");
            case 'D':    var = day;            break;
            case 'm':    format = wxT("%d");
            case 'M':    var = month;        break;
            case 'Y':    format = wxT("%04d");
            case 'y':    var = year;            break;
            default:    value += *c;
        }

        if (*c == 'y')
            var %= 100;

        if (var != -1)
        {
            char str[10];
            sprintf(str, wx2std(format).c_str(), var);
            value += std2wx(str);
        }
    }
    return value;
}
//--------------------------------------------------------------------------------------
//! formats time according to TimeFormat wxString
wxString GetHumanTime(int hour, int minute, int second, wxString TimeFormat)
{
    wxString value;
    for (wxString::iterator c = TimeFormat.begin(); c != TimeFormat.end(); c++)
    {
        int var = -1;
        wxString format = wxT("%02d");
        switch (*c)
        {
            case 'h':    format = wxT("%d");
            case 'H':    var = hour;            break;
            case 'm':    format = wxT("%d");
            case 'M':    var = minute;        break;
            case 's':    format = wxT("%d");
            case 'S':    var = second;        break;
            default:    value += *c;
        }

        if (var != -1)
        {
            char str[10];
            sprintf(str, wx2std(format).c_str(), var);
            value += std2wx(str);
        }
    }
    return value;
}
//--------------------------------------------------------------------------------------
//! formats timestamp according to DateFormat and TimeFormat strings
wxString GetHumanTimestamp(IBPP::Timestamp ts, wxString DateFormat, wxString TimeFormat)
{
    int year, month, day, hour, minute, second;
    ts.GetDate(year, month, day);
    ts.GetTime(hour, minute, second);

    wxString value = GetHumanTime(hour, minute, second, TimeFormat);
    if (value != wxT(""))
        value = wxT(" ") + value;
    return GetHumanDate(year, month, day, DateFormat) + value;
}
//--------------------------------------------------------------------------------------
//! takes value from column "col" of Statement and converts it into wxString
// returns false if value is null
bool CreateString(IBPP::Statement& st, int col, wxString& value)
{
    std::ostringstream svalue;

    if (st->IsNull(col))
        return false;

    double dval;
    float fval;
    int64_t int64val;
    int x;
    IBPP::Date d;
    IBPP::Time t;
    IBPP::Timestamp ts;
    int year, month, day, hour, minute, second;

    IBPP::SDT DataType = st->ColumnType(col);
    if (st->ColumnScale(col))
        DataType = IBPP::sdDouble;

    // load these only if needed for formatting the value
    bool reformatNumbers = false;
    int numberPrecision = 2;
    if (DataType == IBPP::sdDouble)
    {
        reformatNumbers = config().get(wxT("ReformatNumbers"), false);
        // defaults to 2 decimal digits
        numberPrecision = config().get(wxT("NumberPrecision"), 2);
    }

    wxString dateFormat;
    wxString timeFormat;
    if (DataType == IBPP::sdDate || DataType == IBPP::sdTimestamp)
        dateFormat = config().get(wxT("DateFormat"), wxString(wxT("D.M.Y")));
    if (DataType == IBPP::sdTime || DataType == IBPP::sdTimestamp)
        timeFormat = config().get(wxT("TimeFormat"), wxString(wxT("H:M:S")));

    value = wxT("");
    std::string tempValue;
    switch (DataType)
    {
        case IBPP::sdString:
            st->Get(col, tempValue);
            value = std2wx(tempValue);
            return true;
        case IBPP::sdInteger:
        case IBPP::sdSmallint:
            st->Get(col, &x);
            value << x;
            return true;
        case IBPP::sdDate:
            st->Get(col, d);
            d.GetDate(year, month, day);
            value = GetHumanDate(year, month, day, dateFormat);
            return true;
        case IBPP::sdTime:
            st->Get(col, t);
            t.GetTime(hour, minute, second);
            value = GetHumanTime(hour, minute, second, timeFormat);
            return true;
        case IBPP::sdTimestamp:
            st->Get(col, ts);
            value = GetHumanTimestamp(ts, dateFormat, timeFormat);
            return true;
        case IBPP::sdFloat:
            st->Get(col, &fval);
            svalue << std::fixed << fval;
            value = std2wx(svalue.str());
            return true;
        case IBPP::sdDouble:
            st->Get(col, &dval);
            if (st->ColumnScale(col))
                svalue << std::fixed << std::setprecision(st->ColumnScale(col)) << dval;
            else
            {
                if (reformatNumbers)
                    svalue << std::fixed << std::setprecision(numberPrecision) << dval;
                else
                    svalue << std::fixed << dval;
            }
            value = std2wx(svalue.str());
            return true;

        case IBPP::sdLargeint:
            st->Get(col, &int64val);
            svalue << int64val;
            value = std2wx(svalue.str());
            return true;

        default:
            value = wxT("[...]");
            return true;
    };

    //return true;
}
//--------------------------------------------------------------------------------------
