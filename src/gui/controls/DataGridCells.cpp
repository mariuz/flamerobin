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

#include <iomanip>
#include <sstream>
#include <string>

#include "config/Config.h"
#include "core/Observer.h"
#include "core/Subject.h"
#include "gui/controls/DataGridCells.h"
#include "ugly.h"
//-----------------------------------------------------------------------------
// GridCellFormats: class to cache config data for cell formatting
class GridCellFormats: public Observer
{
private:
    bool loadedM;
    int precisionForDoubleM;
    wxString dateFormatM;
    wxString timeFormatM;
    void ensureLoaded();
public:
    GridCellFormats();

    static GridCellFormats& get();
    virtual void update();

    wxString formatDouble(double value);
    wxString formatDate(int year, int month, int day);
    wxString formatTime(int hour, int minute, int second, int milliSecond);
};
//-----------------------------------------------------------------------------
GridCellFormats::GridCellFormats()
    : loadedM(false)
{
    config().attachObserver(this);
}
//-----------------------------------------------------------------------------
GridCellFormats& GridCellFormats::get()
{
    static GridCellFormats gcf;
    return gcf;
}
//-----------------------------------------------------------------------------
void GridCellFormats::update()
{
    // we observe config() object, so we better do as little as possible
    // I think this here is fast enough ;-)
    loadedM = false;
}
//-----------------------------------------------------------------------------
void GridCellFormats::ensureLoaded()
{
    if (!loadedM)
    {
        precisionForDoubleM = config().get(wxT("NumberPrecision"), 2);
        if (!config().get(wxT("ReformatNumbers"), false))
            precisionForDoubleM = -1;

        dateFormatM = config().get(wxT("DateFormat"), wxString(wxT("D.M.Y")));
        timeFormatM = config().get(wxT("TimeFormat"), wxString(wxT("H:M:S.T")));

        loadedM = true;
    }
}
//-----------------------------------------------------------------------------
wxString GridCellFormats::formatDouble(double value)
{
    ensureLoaded();

    std::ostringstream oss;
    oss << std::fixed;
    if (precisionForDoubleM >= 0 && precisionForDoubleM <= 18)
        oss << std::setprecision(precisionForDoubleM);
    oss << value;
    return std2wx(oss.str());
}
//-----------------------------------------------------------------------------
wxString GridCellFormats::formatDate(int year, int month, int day)
{
    ensureLoaded();

    wxString result;
    for (wxString::iterator c = dateFormatM.begin(); c != dateFormatM.end(); c++)
    {
        switch (*c)
        {
            case 'd':
                result += wxString::Format(wxT("%d"), day);
                break;
            case 'D':
                result += wxString::Format(wxT("%02d"), day);
                break;
            case 'm':
                result += wxString::Format(wxT("%d"), month);
                break;
            case 'M':
                result += wxString::Format(wxT("%02d"), month);
                break;
            case 'y':
                result += wxString::Format(wxT("%02d"), year % 100);
                break;
            case 'Y':
                result += wxString::Format(wxT("%04d"), year);
                break;
            default:
                result += *c;
                break;
        }
    }
    return result;
}
//-----------------------------------------------------------------------------
wxString GridCellFormats::formatTime(int hour, int minute, int second,
    int milliSecond)
{
    ensureLoaded();

    wxString result;
    for (wxString::iterator c = timeFormatM.begin(); c != timeFormatM.end(); c++)
    {
        switch (*c)
        {
            case 'h':
                result += wxString::Format(wxT("%d"), hour);
                break;
            case 'H':
                result += wxString::Format(wxT("%02d"), hour);
                break;
            case 'm':
                result += wxString::Format(wxT("%d"), minute);
                break;
            case 'M':
                result += wxString::Format(wxT("%02d"), minute);
                break;
            case 's':
                result += wxString::Format(wxT("%d"), second);
                break;
            case 'S':
                result += wxString::Format(wxT("%02d"), second);
                break;
            case 'T':
                result += wxString::Format(wxT("%03d"), milliSecond);
                break;
            default:
                result += *c;
                break;
        }
    }
    return result;
}
//-----------------------------------------------------------------------------
// StringGridCell: class with wxString representation of data
class StringGridCell: public GridCell
{
private:
    wxString valueM;
public:
    StringGridCell(const wxString& value);
    virtual wxString getValue();
};
//-----------------------------------------------------------------------------
StringGridCell::StringGridCell(const wxString& value)
    : valueM(value)
{
}
//-----------------------------------------------------------------------------
wxString StringGridCell::getValue()
{
    return valueM;
}
//-----------------------------------------------------------------------------
inline GridCell* createCellForString(IBPP::Statement& statement, int col,
    wxMBConv* converter)
{
    std::string cv;
    statement->Get(col, cv);

    wxASSERT(converter != 0);
    return new StringGridCell(wxString(cv.c_str(), *converter));
}
//-----------------------------------------------------------------------------
// IntegerGridCell: class for integer and smallint data
class IntegerGridCell: public GridCell
{
private:
    int valueM;
public:
    IntegerGridCell(int value);
    virtual wxString getValue();
};
//-----------------------------------------------------------------------------
IntegerGridCell::IntegerGridCell(int value)
    : GridCell(), valueM(value) 
{
}
//-----------------------------------------------------------------------------
wxString IntegerGridCell::getValue()
{
    return wxString::Format(wxT("%d"), valueM); 
}
//-----------------------------------------------------------------------------
inline GridCell* createCellForInteger(IBPP::Statement& statement, int col)
{
    int cv;
    statement->Get(col, cv);
    return new IntegerGridCell(cv);
}
//-----------------------------------------------------------------------------
// LargeintGridCell: class for big integer (64 bit) data
class LargeintGridCell: public GridCell
{
private:
    int64_t valueM;
public:
    LargeintGridCell(int64_t value);
    virtual wxString getValue();
};
//-----------------------------------------------------------------------------
LargeintGridCell::LargeintGridCell(int64_t value)
    : GridCell(), valueM(value) 
{
}
//-----------------------------------------------------------------------------
wxString LargeintGridCell::getValue()
{
    return wxLongLong(valueM).ToString(); 
}
//-----------------------------------------------------------------------------
inline GridCell* createCellForLargeint(IBPP::Statement& statement, int col)
{
    int64_t cv;
    statement->Get(col, cv);
    return new LargeintGridCell(cv);
}
//-----------------------------------------------------------------------------
// FloatGridCell class: class for single precision floating point data
class FloatGridCell: public GridCell
{
private:
    float valueM;
public:
    FloatGridCell(float value);
    virtual wxString getValue();
};
//-----------------------------------------------------------------------------
FloatGridCell::FloatGridCell(float value)
    : GridCell(), valueM(value)
{
}
//-----------------------------------------------------------------------------
wxString FloatGridCell::getValue()
{ 
    std::ostringstream oss;
    oss << std::fixed << valueM;
    return std2wx(oss.str());
}
//-----------------------------------------------------------------------------
inline GridCell* createCellForFloat(IBPP::Statement& statement, int col)
{
    float cv;
    statement->Get(col, cv);
    return new FloatGridCell(cv);
}
//-----------------------------------------------------------------------------
// DoubleGridCell class: class for double precision floating point and 
//                       scaled integer (like INTEGER(18, 4)) data
class DoubleGridCell: public GridCell
{
private:
    double valueM;
    int scaleM;
public:
    DoubleGridCell(double value, int scale);
    virtual wxString getValue();
};
//-----------------------------------------------------------------------------
DoubleGridCell::DoubleGridCell(double value, int scale)
    : GridCell(), valueM(value), scaleM(scale)
{
}
//-----------------------------------------------------------------------------
wxString DoubleGridCell::getValue()
{
    if (scaleM)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(scaleM) << valueM;  
        return std2wx(oss.str());
    }
    return GridCellFormats::get().formatDouble(valueM);
}
//-----------------------------------------------------------------------------
inline GridCell* createCellForDouble(IBPP::Statement& statement, int col)
{
    double cv;
    statement->Get(col, cv);
    return new DoubleGridCell(cv, statement->ColumnScale(col));
}
//-----------------------------------------------------------------------------
// DateGridCell: class for date data
class DateGridCell: public GridCell
{
private:
    int yearM, monthM, dayM;
public:
    DateGridCell(int year, int month, int day);
    virtual wxString getValue();
};
//-----------------------------------------------------------------------------
DateGridCell::DateGridCell(int year, int month, int day)
    : GridCell(), yearM(year), monthM(month), dayM(day)
{
}
//-----------------------------------------------------------------------------
wxString DateGridCell::getValue()
{
    return GridCellFormats::get().formatDate(yearM, monthM, dayM); 
}
//-----------------------------------------------------------------------------
inline GridCell* createCellForDate(IBPP::Statement& statement, int col)
{
    IBPP::Date cv;
    statement->Get(col, cv);
    int year, month, day;
    cv.GetDate(year, month, day);
    return new DateGridCell(year, month, day);
}
//-----------------------------------------------------------------------------
// TimeGridCell: class for time data
class TimeGridCell: public GridCell
{
private:
    int hourM, minuteM, secondM, milliSecondM;
public:
    TimeGridCell(int hour, int minute, int second, int milliSecond);
    virtual wxString getValue();
};
//-----------------------------------------------------------------------------
TimeGridCell::TimeGridCell(int hour, int minute, int second, int milliSecond)
    : GridCell(), hourM(hour), minuteM(minute), secondM(second)
    , milliSecondM(milliSecond)
{
}
//-----------------------------------------------------------------------------
wxString TimeGridCell::getValue()
{
    return GridCellFormats::get().formatTime(hourM, minuteM, secondM,
        milliSecondM); 
}
//-----------------------------------------------------------------------------
inline GridCell* createCellForTime(IBPP::Statement& statement, int col)
{
    IBPP::Time cv;
    statement->Get(col, cv);
    int hour, minute, second, tenThousandths;
    cv.GetTime(hour, minute, second, tenThousandths);
    return new TimeGridCell(hour, minute, second, tenThousandths / 10);
}
//-----------------------------------------------------------------------------
// TimestampGridCell: class for timestamp (date and time) data
class TimestampGridCell: public GridCell
{
private:
    int yearM, monthM, dayM;
    int hourM, minuteM, secondM, milliSecondM;
public:
    TimestampGridCell(int year, int month, int day, 
        int hour, int minute, int second, int milliSecond);
    virtual wxString getValue();
};
//-----------------------------------------------------------------------------
TimestampGridCell::TimestampGridCell(int year, int month, int day,
        int hour, int minute, int second, int milliSecond)
    : GridCell(), yearM(year), monthM(month), dayM(day)
    , hourM(hour), minuteM(minute), secondM(second), milliSecondM(milliSecond)
{
}
//-----------------------------------------------------------------------------
wxString TimestampGridCell::getValue()
{
    wxString date = GridCellFormats::get().formatDate(yearM, monthM, dayM);
    wxString time = GridCellFormats::get().formatTime(hourM, minuteM, secondM,
        milliSecondM);
    if (time.empty())
        return date;
    else if (date.empty())
        return time;
    else
        return date + wxT(" ") + time;
}
//-----------------------------------------------------------------------------
inline GridCell* createCellForTimestamp(IBPP::Statement& statement, int col)
{
    IBPP::Timestamp cv;
    statement->Get(col, cv);
    int year, month, day;
    cv.GetDate(year, month, day);
    int hour, minute, second, tenThousandths;
    cv.GetTime(hour, minute, second, tenThousandths);
    return new TimestampGridCell(year, month, day, hour, minute, second,
        tenThousandths / 10); // 
}
//-----------------------------------------------------------------------------
// DataNAGridCell: class to show "[...]" for fields without an obvious
// wxString representation of the value (like array or blob fields)
class DataNAGridCell: public GridCell
{
public:
    virtual wxString getValue();
};
//-----------------------------------------------------------------------------
wxString DataNAGridCell::getValue()
{
    return wxString(wxT("[...]"));
}
//-----------------------------------------------------------------------------
// GridCell: abstract base class
GridCell::~GridCell()
{
}
//-----------------------------------------------------------------------------
// static method to create cell objects
GridCell* GridCell::createCell(IBPP::Statement& statement, int col,
    wxMBConv* converter)
{
    if (statement->IsNull(col))
        return 0;

    IBPP::SDT dataType = statement->ColumnType(col);
    if (statement->ColumnScale(col))
        dataType = IBPP::sdDouble;
    switch (dataType)
    {
        case IBPP::sdString:
            return createCellForString(statement, col, converter);
        case IBPP::sdInteger:
        case IBPP::sdSmallint:
            return createCellForInteger(statement, col);
        case IBPP::sdLargeint:
            return createCellForLargeint(statement, col);
        case IBPP::sdFloat:
            return createCellForFloat(statement, col);
        case IBPP::sdDouble:
            return createCellForDouble(statement, col);
        case IBPP::sdDate:
            return createCellForDate(statement, col);
        case IBPP::sdTime:
            return createCellForTime(statement, col);
        case IBPP::sdTimestamp:
            return createCellForTimestamp(statement, col);
        default:
            return new DataNAGridCell();
    }
}
//-----------------------------------------------------------------------------
