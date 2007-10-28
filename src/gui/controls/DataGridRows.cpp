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

#include <wx/datetime.h>
#include <wx/textbuf.h>

#include <algorithm>
#include <bitset>
#include <iomanip>
#include <sstream>
#include <string>

#include "config/Config.h"
#include "core/FRError.h"
#include "core/Observer.h"
#include "core/StringUtils.h"
#include "frtypes.h"
#include "gui/controls/DataGridRowBuffer.h"
#include "gui/controls/DataGridRows.h"
#include "metadata/database.h"
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
    bool parseDate(wxString::iterator&, int& year, int& month, int& day);
    bool parseTime(wxString::iterator&, int& hr, int& mn, int& sc, int& ml);
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
        switch ((wxChar)*c)
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
bool getNumber(wxString::iterator& ci, int& toSet)
{
    wxString num;
    while (true)
    {
        wxChar c = (wxChar)*ci;
        if (c < wxChar('0') || c > wxChar('9'))
            break;
        num += c;
        ci++;
    }
    long l;
    if (num.IsEmpty() || !num.ToLong(&l))
        return false;
    toSet = l;
    return true;
}
//-----------------------------------------------------------------------------
bool GridCellFormats::parseDate(wxString::iterator& si, int& year, int& month,
    int& day)
{
    ensureLoaded();

    // allow the user to only enter D, D.M or D.M.Y
    bool dayIsSet = false;
    for (wxString::iterator c = dateFormatM.begin();
        c != dateFormatM.end(); c++)
    {
        switch ((wxChar)*c)
        {
            case 'd':
            case 'D':
                if (!getNumber(si, day))
                    return false;
                dayIsSet = true;
                break;
            case 'm':
            case 'M':
                if (!getNumber(si, month))
                    return dayIsSet;
                break;
            case 'y':
            case 'Y':
                if (!getNumber(si, year))
                    return dayIsSet;
                break;
            default:        // other characters must match
                if (*c != *si)
                    return dayIsSet;
                si++;
                break;
        }
    }
    return true;
}
//-----------------------------------------------------------------------------
wxString GridCellFormats::formatTime(int hour, int minute, int second,
    int milliSecond)
{
    ensureLoaded();

    wxString result;
    for (wxString::iterator c = timeFormatM.begin(); c != timeFormatM.end();
        c++)
    {
        switch ((wxChar)*c)
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
bool GridCellFormats::parseTime(wxString::iterator& si, int& hr, int& mn,
    int& sc, int& ml)
{
    ensureLoaded();

    // we allow the user to only enter H, or H:M or H:M:S
    bool hourIsSet = false;
    for (wxString::iterator c = timeFormatM.begin(); c != timeFormatM.end();
        c++)
    {
        switch ((wxChar)*c)
        {
            case 'h':
            case 'H':
                if (!getNumber(si, hr))
                    return false;
                hourIsSet = true;
                break;
            case 'm':
            case 'M':
                if (!getNumber(si, mn))
                    return hourIsSet;
                break;
            case 's':
            case 'S':
                if (!getNumber(si, sc))
                    return hourIsSet;
                break;
            case 'T':
                if (!getNumber(si, ml))
                    return hourIsSet;
                break;
            default:
                if (*c != *si)
                    return hourIsSet;
                si++;
                break;
        }
    }
    return true;
}
//-----------------------------------------------------------------------------
// ResultsetColumnDef class
ResultsetColumnDef::ResultsetColumnDef(const wxString& name, bool readonly,
    bool nullable)
    : nameM(name), readOnlyM(readonly), nullableM(nullable)
{
}
//-----------------------------------------------------------------------------
ResultsetColumnDef::~ResultsetColumnDef()
{
}
//-----------------------------------------------------------------------------
// needed to avoid strange date&time formatting if such column is PK/UNQ
wxString ResultsetColumnDef::getAsFirebirdString(DataGridRowBuffer* buffer)
{
    return getAsString(buffer);
}
//-----------------------------------------------------------------------------
wxString ResultsetColumnDef::getName()
{
    return nameM;
}
//-----------------------------------------------------------------------------
unsigned ResultsetColumnDef::getIndex()
{
    return 0;
}
//-----------------------------------------------------------------------------
bool ResultsetColumnDef::isNumeric()
{
    return false;
}
//-----------------------------------------------------------------------------
bool ResultsetColumnDef::isReadOnly()
{
    return readOnlyM;
}
//-----------------------------------------------------------------------------
bool ResultsetColumnDef::isNullable()
{
    return nullableM;
}
//-----------------------------------------------------------------------------
// DummyColumnDef class
class DummyColumnDef : public ResultsetColumnDef
{
public:
    DummyColumnDef(const wxString& name);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
DummyColumnDef::DummyColumnDef(const wxString& name)
    : ResultsetColumnDef(name)
{
}
//-----------------------------------------------------------------------------
wxString DummyColumnDef::getAsString(DataGridRowBuffer*)
{
    return wxT("[...]");
}
//-----------------------------------------------------------------------------
unsigned DummyColumnDef::getBufferSize()
{
    return 0;
}
//-----------------------------------------------------------------------------
void DummyColumnDef::setValue(DataGridRowBuffer* /*buffer*/, unsigned /*col*/,
    const IBPP::Statement& /*statement*/, wxMBConv* /*converter*/)
{
}
//-----------------------------------------------------------------------------
void DummyColumnDef::setFromString(DataGridRowBuffer* /* buffer */,
         const wxString& /* source */)
{
}
//-----------------------------------------------------------------------------
// IntegerColumnDef class
class IntegerColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
public:
    IntegerColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
IntegerColumnDef::IntegerColumnDef(const wxString& name, unsigned offset,
    bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset)
{
}
//-----------------------------------------------------------------------------
wxString IntegerColumnDef::getAsString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    int value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;
    return wxString::Format(wxT("%d"), value);
}
//-----------------------------------------------------------------------------
void IntegerColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    long value;
    if (!source.ToLong(&value))
        throw FRError(_("Invalid integer numeric value"));
    buffer->setValue(offsetM, (int)value);
}
//-----------------------------------------------------------------------------
unsigned IntegerColumnDef::getBufferSize()
{
    return sizeof(int);
}
//-----------------------------------------------------------------------------
bool IntegerColumnDef::isNumeric()
{
    return true;
}
//-----------------------------------------------------------------------------
void IntegerColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*)
{
    wxASSERT(buffer);
    int value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value);
}
//-----------------------------------------------------------------------------
// Int64ColumnDef class
class Int64ColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
public:
    Int64ColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
Int64ColumnDef::Int64ColumnDef(const wxString& name, unsigned offset,
    bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset)
{
}
//-----------------------------------------------------------------------------
wxString Int64ColumnDef::getAsString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    int64_t value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;
    return wxLongLong(value).ToString();
}
//-----------------------------------------------------------------------------
void Int64ColumnDef::setFromString(DataGridRowBuffer* buffer,
    const wxString& source)
{
    wxASSERT(buffer);

#if wxCHECK_VERSION(2, 8, 0)
	wxLongLong_t ll;
    if (source.ToLongLong(&ll))
    {
        buffer->setValue(offsetM, (int64_t)ll);
        return;
    }
#endif

    // perhaps underlying library doesn't support 64bit, we try 32:
    long l;
    if (!source.ToLong(&l)) // nope, that fails as well
        throw FRError(_("Invalid 64bit numeric value"));
    buffer->setValue(offsetM, (int64_t)l);
}
//-----------------------------------------------------------------------------
unsigned Int64ColumnDef::getBufferSize()
{
    return sizeof(int64_t);
}
//-----------------------------------------------------------------------------
bool Int64ColumnDef::isNumeric()
{
    return true;
}
//-----------------------------------------------------------------------------
void Int64ColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*)
{
    wxASSERT(buffer);
    int64_t value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value);
}
//-----------------------------------------------------------------------------
// DBKeyColumnDef class
class DBKeyColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
    unsigned sizeM;
public:
    DBKeyColumnDef(const wxString& name, unsigned offset, unsigned size);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
    void getDBKey(IBPP::DBKey& dbkey, DataGridRowBuffer* buffer);
};
//-----------------------------------------------------------------------------
DBKeyColumnDef::DBKeyColumnDef(const wxString& name, unsigned offset,
    unsigned size)
    : ResultsetColumnDef(name, true, false), offsetM(offset), sizeM(size)
{
}
//-----------------------------------------------------------------------------
wxString DBKeyColumnDef::getAsString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    wxString ret;
    for (int i = 0; i < (int)sizeM / 8; i++)
    {
        if (i > 0)
            ret += wxT("-");
        int v1, v2;
        buffer->getValue(offsetM+i*8, v1);
        buffer->getValue(offsetM+i*8+4, v2);
        ret += wxString::Format(wxT("%08x:%08x"), (unsigned)v1, (unsigned)v2);
    }
    return ret;
}
//-----------------------------------------------------------------------------
void DBKeyColumnDef::setFromString(DataGridRowBuffer* /*buffer*/,
	const wxString& /*source*/)
{
    // should never be editable
}
//-----------------------------------------------------------------------------
unsigned DBKeyColumnDef::getBufferSize()
{
    return sizeM;
}
//-----------------------------------------------------------------------------
bool DBKeyColumnDef::isNumeric()
{
    return false;
}
//-----------------------------------------------------------------------------
void DBKeyColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*)
{
    wxASSERT(buffer);
    IBPP::DBKey value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value);
}
//-----------------------------------------------------------------------------
void DBKeyColumnDef::getDBKey(IBPP::DBKey& dbkey, DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    buffer->getValue(offsetM, dbkey, sizeM);
}
//-----------------------------------------------------------------------------
// DateColumnDef class
class DateColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
public:
    DateColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable);
    virtual wxString getAsFirebirdString(DataGridRowBuffer* buffer);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
DateColumnDef::DateColumnDef(const wxString& name, unsigned offset,
    bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset)
{
}
//-----------------------------------------------------------------------------
wxString DateColumnDef::getAsString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    int value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;

    IBPP::Date date(value);
    int year, month, day;
    date.GetDate(year, month, day);
    return GridCellFormats::get().formatDate(year, month, day);
}
//-----------------------------------------------------------------------------
wxString DateColumnDef::getAsFirebirdString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    int value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;

    IBPP::Date date(value);
    int year, month, day;
    date.GetDate(year, month, day);
    return wxString::Format(wxT("%d-%d-%d"), year, month, day);
}
//-----------------------------------------------------------------------------
void DateColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    int y = wxDateTime::Now().GetYear();    // defaults
    int m = wxDateTime::Now().GetMonth() + 1;
    int d = wxDateTime::Now().GetDay();
    wxString temp(source);
    temp.Trim(true);
    temp.Trim(false);

    IBPP::Date idt;
    if (temp == wxT("DATE") || temp == wxT("TODAY") || temp == wxT("NOW"))
        idt.Today();
    else
    {
        wxString::iterator it = temp.begin();
        if (!GridCellFormats::get().parseDate(it, y, m, d))
            throw FRError(_("Cannot parse date"));
        idt.SetDate(y, m, d);
    }
    buffer->setValue(offsetM, idt.GetDate());
}
//-----------------------------------------------------------------------------
unsigned DateColumnDef::getBufferSize()
{
    return sizeof(int);
}
//-----------------------------------------------------------------------------
void DateColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*)
{
    wxASSERT(buffer);
    IBPP::Date value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value.GetDate());
}
//-----------------------------------------------------------------------------
// TimeColumnDef class
class TimeColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
public:
    TimeColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable);
    virtual wxString getAsFirebirdString(DataGridRowBuffer* buffer);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
TimeColumnDef::TimeColumnDef(const wxString& name, unsigned offset,
    bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset)
{
}
//-----------------------------------------------------------------------------
wxString TimeColumnDef::getAsString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    int value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;

    IBPP::Time time(value);
    int hour, minute, second, tenththousands;
    time.GetTime(hour, minute, second, tenththousands);
    return GridCellFormats::get().formatTime(hour, minute, second,
        tenththousands / 10);
}
//-----------------------------------------------------------------------------
wxString TimeColumnDef::getAsFirebirdString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    int value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;

    IBPP::Time time(value);
    int hour, minute, second, tenththousands;
    time.GetTime(hour, minute, second, tenththousands);
    return wxString::Format(wxT("%d:%d:%d.%d"), hour, minute, second,
        tenththousands / 10);
}
//-----------------------------------------------------------------------------
void TimeColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    int hr = 0, mn = 0, sc = 0, ms = 0;
    wxString temp(source);
    temp.Trim(true);
    temp.Trim(false);

    IBPP::Time itm;
    if (temp == wxT("TIME") || temp == wxT("NOW"))
        itm.Now();
    else
    {
        wxString::iterator it = temp.begin();
        if (!GridCellFormats::get().parseTime(it, hr, mn, sc, ms))
            throw FRError(_("Cannot parse time"));
        itm.SetTime(hr, mn, sc, 10 * ms);
    }
    buffer->setValue(offsetM, itm.GetTime());
}
//-----------------------------------------------------------------------------
unsigned TimeColumnDef::getBufferSize()
{
    return sizeof(int);
}
//-----------------------------------------------------------------------------
void TimeColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*)
{
    wxASSERT(buffer);
    IBPP::Time value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value.GetTime());
}
//-----------------------------------------------------------------------------
// TimestampColumnDef class
class TimestampColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
public:
    TimestampColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable);
    virtual wxString getAsFirebirdString(DataGridRowBuffer* buffer);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
TimestampColumnDef::TimestampColumnDef(const wxString& name, unsigned offset,
    bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset)
{
}
//-----------------------------------------------------------------------------
wxString TimestampColumnDef::getAsString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    int value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;
    IBPP::Date date(value);

    if (!buffer->getValue(offsetM + sizeof(int), value))
        return wxEmptyString;
    IBPP::Time time(value);

    int year, month, day, hour, minute, second, tenththousands;
    date.GetDate(year, month, day);
    time.GetTime(hour, minute, second, tenththousands);

    wxString dateStr = GridCellFormats::get().formatDate(year, month, day);
    wxString timeStr = GridCellFormats::get().formatTime(hour, minute, second,
        tenththousands / 10);
    if (timeStr.empty())
        return dateStr;
    else if (dateStr.empty())
        return timeStr;
    else
        return dateStr + wxT(", ") + timeStr;
}
//-----------------------------------------------------------------------------
wxString TimestampColumnDef::getAsFirebirdString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    int value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;
    IBPP::Date date(value);

    if (!buffer->getValue(offsetM + sizeof(int), value))
        return wxEmptyString;
    IBPP::Time time(value);

    int year, month, day, hour, minute, second, tenththousands;
    date.GetDate(year, month, day);
    time.GetTime(hour, minute, second, tenththousands);

    wxString dateStr = wxString::Format(wxT("%d-%d-%d"), year, month, day);
    wxString timeStr = wxString::Format(wxT("%d:%d:%d.%d"), hour, minute,
        second, tenththousands / 10);
    return dateStr + wxT(", ") + timeStr;
}
//-----------------------------------------------------------------------------
void TimestampColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    wxString temp(source);
    temp.Trim(true);
    temp.Trim(false);

    IBPP::Timestamp its;
    if (temp == wxT("NOW"))
        its.Now();
    else if (temp == wxT("DATE") || temp == wxT("TODAY"))
        its.Today();
    else
    {
        wxString::iterator it = temp.begin();
        // get date
        int y = wxDateTime::Now().GetYear();    // defaults
        int m = wxDateTime::Now().GetMonth() + 1;
        int d = wxDateTime::Now().GetDay();
        if (!GridCellFormats::get().parseDate(it, y, m, d))
            throw FRError(_("Cannot parse date"));
        its.SetDate(y, m, d);

        // skip spaces and commas ", "
        while ((wxChar)*it == wxChar(',') || (wxChar)*it == wxChar(' '))
            it++;

        // get time (if available)
        int hr = 0, mn = 0, sc = 0, ms = 0;
        if (GridCellFormats::get().parseTime(it, hr, mn, sc, ms))
            its.SetTime(hr, mn, sc, 10 * ms);
    }

    // all done, set the value
    buffer->setValue(offsetM, its.GetDate());
    buffer->setValue(offsetM + sizeof(int), its.GetTime());
}
//-----------------------------------------------------------------------------
unsigned TimestampColumnDef::getBufferSize()
{
    return 2 * sizeof(int);
}
//-----------------------------------------------------------------------------
void TimestampColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*)
{
    wxASSERT(buffer);
    IBPP::Timestamp value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value.GetDate());
    buffer->setValue(offsetM + sizeof(int), value.GetTime());
}
//-----------------------------------------------------------------------------
// FloatColumnDef class
class FloatColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
public:
    FloatColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
FloatColumnDef::FloatColumnDef(const wxString& name, unsigned offset,
    bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset)
{
}
//-----------------------------------------------------------------------------
wxString FloatColumnDef::getAsString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    float value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;

    std::ostringstream oss;
    oss << std::fixed << value;
    return std2wx(oss.str());
}
//-----------------------------------------------------------------------------
void FloatColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    double d;
    if (!source.ToDouble(&d))
        throw FRError(_("Invalid float numeric value"));
    buffer->setValue(offsetM, (float)d);
}
//-----------------------------------------------------------------------------
unsigned FloatColumnDef::getBufferSize()
{
    return sizeof(float);
}
//-----------------------------------------------------------------------------
bool FloatColumnDef::isNumeric()
{
    return true;
}
//-----------------------------------------------------------------------------
void FloatColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*)
{
    wxASSERT(buffer);
    float value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value);
}
//-----------------------------------------------------------------------------
// DoubleColumnDef class
class DoubleColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
    short scaleM;
public:
    DoubleColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable, short scale);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
DoubleColumnDef::DoubleColumnDef(const wxString& name, unsigned offset,
    bool readOnly, bool nullable, short scale)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset),
      scaleM(scale)
{
}
//-----------------------------------------------------------------------------
wxString DoubleColumnDef::getAsString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    double value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;

    if (scaleM)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(scaleM) << value;
        return std2wx(oss.str());
    }
    return GridCellFormats::get().formatDouble(value);
}
//-----------------------------------------------------------------------------
void DoubleColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    double d;
    if (!source.ToDouble(&d))
        throw FRError(_("Invalid double numeric value"));
    buffer->setValue(offsetM, d);
}
//-----------------------------------------------------------------------------
unsigned DoubleColumnDef::getBufferSize()
{
    return sizeof(double);
}
//-----------------------------------------------------------------------------
bool DoubleColumnDef::isNumeric()
{
    return true;
}
//-----------------------------------------------------------------------------
void DoubleColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*)
{
    wxASSERT(buffer);
    double value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value);
}
//-----------------------------------------------------------------------------
class BlobColumnDef : public ResultsetColumnDef
{
private:
    unsigned indexM;
public:
    BlobColumnDef(const wxString& name, unsigned blobIndex, bool readOnly,
        bool nullable);
    virtual unsigned getIndex();
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
BlobColumnDef::BlobColumnDef(const wxString& name, unsigned blobIndex,
    bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), indexM(blobIndex)
{
    readOnlyM = true;   // TODO: uncomment this when we make BlobDialog
}
//-----------------------------------------------------------------------------
unsigned BlobColumnDef::getIndex()
{
    return indexM;
}
//-----------------------------------------------------------------------------
wxString BlobColumnDef::getAsString(DataGridRowBuffer* /*buffer*/)
{
    // wxASSERT(buffer);
    // show some starting characters for textual blobs?
    return wxT("[BLOB]");
}
//-----------------------------------------------------------------------------
void BlobColumnDef::setFromString(DataGridRowBuffer* /*buffer*/,
	const wxString& /*source*/)
{
    // wxASSERT(buffer);
    // TODO: is this called from anywhere? - blobs will have a custom editor
    // buffer->setString(indexM, source);
}
//-----------------------------------------------------------------------------
unsigned BlobColumnDef::getBufferSize()
{
    return 0;
}
//-----------------------------------------------------------------------------
void BlobColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv* /*converter*/)
{
    wxASSERT(buffer);
    IBPP::Blob b = IBPP::BlobFactory(statement->DatabasePtr(),
        statement->TransactionPtr());
    statement->Get(col, b);
    buffer->setBlob(indexM, b);
}
//-----------------------------------------------------------------------------
// StringColumnDef class
class StringColumnDef : public ResultsetColumnDef
{
private:
    unsigned indexM;
public:
    StringColumnDef(const wxString& name, unsigned stringIndex, bool readOnly,
        bool nullable);
    virtual unsigned getIndex();
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
StringColumnDef::StringColumnDef(const wxString& name, unsigned stringIndex,
    bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), indexM(stringIndex)
{
}
//-----------------------------------------------------------------------------
unsigned StringColumnDef::getIndex()
{
    return indexM;
}
//-----------------------------------------------------------------------------
wxString StringColumnDef::getAsString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    return buffer->getString(indexM);
}
//-----------------------------------------------------------------------------
void StringColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    // TODO: if CHARACTER SET OCTETS - check if it is a valid hexdec string
    buffer->setString(indexM, source);
}
//-----------------------------------------------------------------------------
unsigned StringColumnDef::getBufferSize()
{
    return 0;
}
//-----------------------------------------------------------------------------
void StringColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv* converter)
{
    wxASSERT(buffer);
    std::string value;
    statement->Get(col, value);
    if (statement->ColumnSubtype(col) != 1)
        buffer->setString(indexM, std2wx(value, converter));
    else    // charset OCTETS
    {
        wxString val;
        for (std::string::size_type p = 0; p < value.length(); p++)
            val += wxString::Format(wxT("%02x"), uint8_t(value[p]));
        buffer->setString(indexM, val);
    }
}
//-----------------------------------------------------------------------------
// DataGridRows class
DataGridRows::DataGridRows(Database *db)
    : bufferSizeM(0), databaseM(db)
{
}
//-----------------------------------------------------------------------------
DataGridRows::~DataGridRows()
{
    clear();
}
//-----------------------------------------------------------------------------
ResultsetColumnDef* DataGridRows::getColumnDef(unsigned col)
{
    return columnDefsM[col];
}
//-----------------------------------------------------------------------------
void DataGridRows::addRow(DataGridRowBuffer* buffer)
{
    if (buffersM.size() == buffersM.capacity())
        buffersM.reserve(buffersM.capacity() + 1024);
    buffersM.push_back(buffer);
}
//-----------------------------------------------------------------------------
bool DataGridRows::addRow(const IBPP::Statement& statement,
    wxMBConv* converter)
{
    DataGridRowBuffer* buffer = new DataGridRowBuffer(columnDefsM.size());
    addRow(buffer);

    // starts with last column -> with highest buffer offset and
    // string array index to allocate all needed memory at once
    unsigned col = columnDefsM.size();
    do
    {
        // IBPP column counts are 1-based, not 0-based...
        unsigned colIBPP = col--;
        bool isNull = statement->IsNull(colIBPP);
        buffer->setFieldNull(col, isNull);
        if (!isNull)
            columnDefsM[col]->setValue(buffer, colIBPP, statement, converter);
    }
    while (col > 0);
    return true;
}
//-----------------------------------------------------------------------------
    void freeBuffer(DataGridRowBuffer* buffer) { delete buffer; }

    void freeColumnDef(ResultsetColumnDef* columnDef) { delete columnDef; }

void DataGridRows::clear()
{
    if (buffersM.size())
    {
        for_each(buffersM.begin(), buffersM.end(), freeBuffer);
        buffersM.clear();
    }
    if (columnDefsM.size())
    {
        for_each(columnDefsM.begin(), columnDefsM.end(), freeColumnDef);
        columnDefsM.clear();
    }
    statementTablesM.clear();
    deleteFromM = statementTablesM.end();
    dbKeysM.clear();
    bufferSizeM = 0;
}
//-----------------------------------------------------------------------------
wxString escapeQuotes(const wxString& input)
{
    wxString sCopy(input);
    sCopy.Replace(wxT("'"), wxT("''"));
    return sCopy;
}
//-----------------------------------------------------------------------------
bool DataGridRows::canRemoveRow(size_t row)
{
    if (row >= buffersM.size())
        return false;
    if (!buffersM[row]->isDeletableIsSet())
    {
        // find table with valid constraint
        bool tableok = false;
        for (std::map<wxString, UniqueConstraint *>::iterator it =
            statementTablesM.begin(); !tableok && it != statementTablesM.end();
            ++it)
        {
            if ((*it).second == 0)
                continue;
            // check if some of PK/UNQ columns contains N/A for that row
            tableok = true;
            for (ColumnConstraint::const_iterator ci = (*it).second->begin();
                ci != (*it).second->end(); ++ci)
            {
                for (int c2 = 1; c2 <= statementM->Columns(); ++c2)
                {
                    if ((*ci) != std2wx(statementM->ColumnName(c2)))
                        continue;
                    wxString tn(std2wx(statementM->ColumnTable(c2)));
                    if (tn == (*it).first && buffersM[row]->isFieldNA(c2-1))
                    {
                        tableok = false;
                        break;
                    }
                }
            }
        }
        buffersM[row]->setIsDeletable(tableok);
    }
    return buffersM[row]->isDeletable();
}
//-----------------------------------------------------------------------------
bool DataGridRows::removeRows(size_t from, size_t count, wxString& stm)
{
    if (statementTablesM.begin() == statementTablesM.end())
        return false;

    if (deleteFromM == statementTablesM.end())  // only ask for the first time
    {
        wxArrayString tables;
        for (std::map<wxString, UniqueConstraint *>::iterator it =
            statementTablesM.begin(); it != statementTablesM.end(); ++it)
        {
            if ((*it).second != 0)
                tables.Add((*it).first);
        }
        if (tables.GetCount() == 0) // no tables found
            return false;
        wxString tab;
        if (tables.GetCount() == 1) // exactly one table
            tab = tables[0];
        else
        {
            tab = wxGetSingleChoice(_("Select a table"),
                _("Multiple tables found"), tables, 0);
            if (tab.IsEmpty())
                return false;
        }
        deleteFromM = statementTablesM.find(tab);
    }

    for (size_t pos = 0; pos < count; ++pos)
    {
        if (pos > 0)
            stm += wxTextBuffer::GetEOL();
        wxString s = wxT("DELETE FROM ")
            + Identifier((*deleteFromM).first).getQuoted() + wxT(" WHERE ");
        addWhereAndExecute((*deleteFromM).second, s, (*deleteFromM).first,
            buffersM[from+pos]);
        stm += s + wxT(";");
    }

    std::vector<DataGridRowBuffer*>::iterator i2, it = buffersM.begin();
    from += count - 1;
    while (from--)
    {
        if (++it == buffersM.end())     // should never happen
            return false;
    }
    while (count--)
    {
        i2 = it;
        it--;
/*
        delete (*i2);
        buffersM.erase(i2);
*/
        (*i2)->setIsDeleted(true);
    }
    return true;
}
//-----------------------------------------------------------------------------
unsigned DataGridRows::getRowCount()
{
    return buffersM.size();
}
//-----------------------------------------------------------------------------
unsigned DataGridRows::getRowFieldCount()
{
    return columnDefsM.size();
}
//-----------------------------------------------------------------------------
wxString DataGridRows::getRowFieldName(unsigned col)
{
    if (col < columnDefsM.size())
        return columnDefsM[col]->getName();
    return wxEmptyString;
}
//-----------------------------------------------------------------------------
void checkColumnsPresent(const IBPP::Statement& statement,
    UniqueConstraint** locator)
{
    wxString tableName = (*locator)->getTable()->getName_();
    for (ColumnConstraint::const_iterator ci = (*locator)->begin(); ci !=
        (*locator)->end(); ++ci)
    {
        bool found = false;
        for (int c2 = 1; c2 <= statement->Columns(); ++c2)
        {
            wxString cn(std2wx(statement->ColumnName(c2)));
            wxString tn(std2wx(statement->ColumnTable(c2)));
            if (cn == (*ci) && tn == tableName)
            {
                found = true;
                break;
            }
        }
        if (!found)     // some columns missing
        {
            *locator = 0;
            break;
        }
    }
}
// We need collect all the table names and find their primary/unique keys.
// If all PK/UNQ columns are available in the list, that table's fields are
// editable (unless they are computed fields). We also read NULL info here.
// We also use RDB$DB_KEY column if present
void DataGridRows::getColumnInfo(Database *db, unsigned col, bool& readOnly,
    bool& nullable)
{
    if (statementM->ColumnType(col) == IBPP::sdString
        && statementM->ColumnSubtype(col) == 1) // charset OCTETS
    {                       // TODO: to make those editable, we need to
        readOnly = true;    // enter values as parameters. This should
        return;             // probably be done together with BLOB support
    }

    wxString tabName(std2wx(statementM->ColumnTable(col)));
    Table *t = dynamic_cast<Table *>(db->findRelation(Identifier(tabName)));
    if (!t)
    {
        readOnly = true;
        return;
    }

    UniqueConstraint *locator = 0;  // used to build WHERE clause
    std::map<wxString, UniqueConstraint *>::iterator tabIt =
        statementTablesM.find(tabName);
    if (tabIt != statementTablesM.end())    // table checked before
        locator = (*tabIt).second;
    else
    {
        locator = t->getPrimaryKey();
        if (locator)    // check if this PK is usable (all fields present)
            checkColumnsPresent(statementM, &locator);
        if (!locator)   // PK not present or not usable, try UNQ
        {
            std::vector<UniqueConstraint> *uq = t->getUniqueConstraints();
            if (uq)
            {
                for (std::vector<UniqueConstraint>::iterator ui = uq->begin();
                    ui != uq->end(); ++ui)
                {
                    locator = &(*ui);
                    checkColumnsPresent(statementM, &locator);
                    if (locator)
                        break;
                }
            }
        }
        if (!locator)   // neither PK nor UNQ found, look for RDB$DB_KEY
        {
            UniqueConstraint uc;
            uc.columnsM.push_back(wxT("DB_KEY"));
            uc.setParent(t);
            locator = &uc;
            checkColumnsPresent(statementM, &locator);
            if (locator)    // DB_KEY present
            {
                dbKeysM.push_back(uc);
                locator = &dbKeysM.back();
            }
        }
        statementTablesM.insert(
            std::pair<wxString, UniqueConstraint*>(tabName, locator));
    }

    readOnly = (locator == 0);
    nullable = false;
    if (!readOnly)  // table is not RO, but column might be, so we search
    {
        wxString cn(std2wx(statementM->ColumnName(col)));
        Column *c = 0;
        t->checkAndLoadColumns();
        for (MetadataCollection<Column>::iterator it = t->begin();
            it != t->end(); ++it)
        {
            if ((*it).getName_() == cn)    // column found
            {
                c = &(*it);
                break;
            }
        }
        readOnly = (c == 0 || !c->getComputedSource().IsEmpty());
        if (!readOnly)  // it is editable, so check if nullable
            nullable = c->isNullable();
    }

    /* wxMessageBox(wxString::Format(wxT("TABLE: %s (RO=%d), COLUMN: %s (RO=%d, NULL=%d)"),
        tabName.c_str(),
        locator ? 0 : 1,
        std2wx(statementM->ColumnName(col)).c_str(),
        readOnly ? 1 : 0,
        nullable ? 1 : 0)
    );*/
}
//-----------------------------------------------------------------------------
bool DataGridRows::initialize(const IBPP::Statement& statement, Database *db)
{
    statementM = statement;

    clear();
    // column definitions may have an index into the string array,
    // an offset into the buffer, or use no data at all
    unsigned colCount = statement->Columns();
    columnDefsM.reserve(colCount);
    bufferSizeM = 0;
    unsigned stringIndex = 0;
    unsigned blobIndex = 0;

    // Create column definitions and compute the necessary buffer size
    // and string array length when all fields contain data
    for (unsigned col = 1; col <= colCount; ++col)
    {
        bool readOnly, nullable;
        getColumnInfo(db, col, readOnly, nullable);

        wxString colName(std2wx(statement->ColumnAlias(col)));
        if (colName.empty())
            colName = std2wx(statement->ColumnName(col));

        IBPP::SDT type = statement->ColumnType(col);
        if (statement->ColumnScale(col))
            type = IBPP::sdDouble;

        ResultsetColumnDef* columnDef = 0;
        if (std::string(statement->ColumnName(col)) == "DB_KEY")
            columnDef = new DBKeyColumnDef(colName, bufferSizeM, statement->ColumnSize(col));
        else
        {
            switch (type)
            {
                case IBPP::sdDate:
                    columnDef = new DateColumnDef(colName, bufferSizeM, readOnly, nullable);
                    break;
                case IBPP::sdTime:
                    columnDef = new TimeColumnDef(colName, bufferSizeM, readOnly, nullable);
                    break;
                case IBPP::sdTimestamp:
                    columnDef = new TimestampColumnDef(colName, bufferSizeM, readOnly, nullable);
                    break;

                case IBPP::sdSmallint:
                case IBPP::sdInteger:
                    columnDef = new IntegerColumnDef(colName, bufferSizeM, readOnly, nullable);
                    break;
                case IBPP::sdLargeint:
                    columnDef = new Int64ColumnDef(colName, bufferSizeM, readOnly, nullable);
                    break;

                case IBPP::sdFloat:
                    columnDef = new FloatColumnDef(colName, bufferSizeM, readOnly, nullable);
                    break;
                case IBPP::sdDouble:
                    columnDef = new DoubleColumnDef(colName, bufferSizeM, readOnly, nullable, statement->ColumnScale(col));
                    break;

                case IBPP::sdString:
                    columnDef = new StringColumnDef(colName, stringIndex, readOnly, nullable);
                    ++stringIndex;
                    break;
                case IBPP::sdBlob:
                    columnDef = new BlobColumnDef(colName, blobIndex, readOnly, nullable);
                    ++blobIndex;
                    break;
                default:
                    // IBPP::sdArray not really handled ATM
                    columnDef = new DummyColumnDef(colName);
                    break;
            }
        }
        wxASSERT(columnDef);
        bufferSizeM += columnDef->getBufferSize();
        columnDefsM.push_back(columnDef);
    }
    return true;
}
//-----------------------------------------------------------------------------
bool DataGridRows::isColumnNumeric(unsigned col)
{
    if (col >= columnDefsM.size())
        return false;
    return columnDefsM[col]->isNumeric();
}
//-----------------------------------------------------------------------------
bool DataGridRows::isColumnReadonly(unsigned col)
{
    if (col >= columnDefsM.size())
        return false;
    return columnDefsM[col]->isReadOnly();
}
//-----------------------------------------------------------------------------
bool DataGridRows::getFieldInfo(unsigned row, unsigned col,
    DataGridFieldInfo& info)
{
    if (col >= columnDefsM.size() || row >= buffersM.size())
        return false;
    info.rowInserted = buffersM[row]->isInserted();
    info.rowDeleted = buffersM[row]->isDeleted();
    info.fieldReadOnly = info.rowDeleted
        || isColumnReadonly(col) || isFieldReadonly(row, col);
    info.fieldModified = !info.rowDeleted
        && buffersM[row]->isFieldModified(col);
    info.fieldNull = buffersM[row]->isFieldNull(col);
    info.fieldNA = buffersM[row]->isFieldNA(col);
    info.fieldNumeric = isColumnNumeric(col);
    return true;
}
//-----------------------------------------------------------------------------
bool DataGridRows::isFieldReadonly(unsigned row, unsigned col)
{
    if (col >= columnDefsM.size() || row >= buffersM.size())
        return false;
    if (columnDefsM[col]->isReadOnly())
        return true;

    // if row is loaded from the database and not inserted by user, we don't
    // need to check anything else
    if (!buffersM[row]->isInserted())
        return false;

    // TODO: this needs to be cached too

    wxString table(std2wx(statementM->ColumnTable(col+1)));
    std::map<wxString, UniqueConstraint *>::iterator it =
        statementTablesM.find(table);
    if (it == statementTablesM.end() || (*it).second == 0)
        return true;

    // check if some of PK/UNQ columns contains N/A for that row
    for (ColumnConstraint::const_iterator ci = (*it).second->begin(); ci !=
        (*it).second->end(); ++ci)
    {
        for (int c2 = 1; c2 <= statementM->Columns(); ++c2)
        {
            if ((*ci) != std2wx(statementM->ColumnName(c2)))
                continue;
            wxString tn(std2wx(statementM->ColumnTable(c2)));
            if (tn == table && buffersM[row]->isFieldNA(c2-1))
                return true;
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
wxString DataGridRows::getFieldValue(unsigned row, unsigned col)
{
    if (row >= buffersM.size() || col >= columnDefsM.size())
        return wxEmptyString;
    return columnDefsM[col]->getAsString(buffersM[row]);
}
//-----------------------------------------------------------------------------
bool DataGridRows::isFieldNull(unsigned row, unsigned col)
{
    if (row >= buffersM.size())
        return false;
    return buffersM[row]->isFieldNull(col);
}
//-----------------------------------------------------------------------------
bool DataGridRows::isFieldNA(unsigned row, unsigned col)
{
    if (row >= buffersM.size())
        return false;
    return buffersM[row]->isFieldNA(col);
}
//-----------------------------------------------------------------------------
void DataGridRows::addWhereAndExecute(UniqueConstraint* uq, wxString& stm,
    const wxString& table, DataGridRowBuffer *buffer)
{
    bool dbkey = false;
    for (ColumnConstraint::const_iterator ci = uq->begin(); ci !=
        uq->end(); ++ci)
    {
        if ((*ci) == wxT("DB_KEY"))
        {
            stm += wxT(" RDB$DB_KEY = ?");
            dbkey = true;
            break;
        }
        for (int c2 = 1; c2 <= statementM->Columns(); ++c2)
        {
            wxString cn(std2wx(statementM->ColumnName(c2)));
            wxString tn(std2wx(statementM->ColumnTable(c2)));
            if (cn == (*ci) && tn == table) // found it, add to WHERE list
            {
                if (buffer->isFieldNA(c2-1))
                    throw FRError(_("N/A value in key column."));
                if (ci != uq->begin())
                    stm += wxT(" AND ");
                stm += Identifier(cn).getQuoted() + wxT(" = '");
                stm += escapeQuotes(
                    columnDefsM[c2-1]->getAsFirebirdString(buffer));
                stm += wxT("'");
                break;
            }
        }
    }

    DatabaseToSystemCharsetConversion dtscc;
    dtscc.setConnectionCharset(databaseM->getConnectionCharset());

    IBPP::Statement st = IBPP::StatementFactory(statementM->DatabasePtr(),
        statementM->TransactionPtr());
    st->Prepare(wx2std(stm, dtscc.getConverter()));
    if (dbkey)  // find the column and set the parameter
    {
        for (int c2 = 1; c2 <= statementM->Columns(); ++c2)
        {
            wxString cn(std2wx(statementM->ColumnName(c2)));
            wxString tn(std2wx(statementM->ColumnTable(c2)));
            if (cn == wxT("DB_KEY") && tn == table)
            {
                DBKeyColumnDef *dbk =
                    dynamic_cast<DBKeyColumnDef *>(columnDefsM[c2-1]);
                if (!dbk)
                    throw FRError(_("Invalid Column"));
                if (buffer->isFieldNA(c2-1))
                    throw FRError(_("N/A value in DB_KEY column."));
                IBPP::DBKey dbkey;
                dbk->getDBKey(dbkey, buffer);
                st->Set(1, dbkey);
            }
        }
    }
    st->Execute();
}
//-----------------------------------------------------------------------------
// returns the executed SQL statement
wxString DataGridRows::setFieldValue(unsigned row, unsigned col,
    const wxString& value, bool setNull)
{
    if (columnDefsM[col]->isReadOnly())
        throw FRError(_("This column is not editable."));

    // user wants to store null
    bool newIsNull = (
        !dynamic_cast<StringColumnDef*>(columnDefsM[col]) && value.IsEmpty()
        || setNull && value == wxT("[null]") );
    if (newIsNull && !columnDefsM[col]->isNullable())
        throw FRError(_("This column does not accept NULLs."));

    // to ensure atomicity, we create a temporary buffer, try to store value
    // in it and also in database. if anything fails, we revert to the values
    // from temp buffer
    DataGridRowBuffer *oldRecord;
    // we create a copy of appropriate type
    InsertedGridRowBuffer *test =
        dynamic_cast<InsertedGridRowBuffer *>(buffersM[row]);
    if (test)
        oldRecord = new InsertedGridRowBuffer(test);
    else
        oldRecord = new DataGridRowBuffer(buffersM[row]);
    try
    {
        buffersM[row]->setFieldNA(col, false);
        if (newIsNull)
            buffersM[row]->setFieldNull(col, true);
        else
        {
            columnDefsM[col]->setFromString(buffersM[row], value);
            buffersM[row]->setFieldNull(col, false);
        }

        // run the UPDATE statement
        wxString table(std2wx(statementM->ColumnTable(col+1)));
        wxString stm = wxT("UPDATE ") + Identifier(table).getQuoted()
            + wxT(" SET ")
            + Identifier(std2wx(statementM->ColumnName(col+1))).getQuoted();
        if (newIsNull)
            stm += wxT(" = NULL WHERE ");
        else
        {
            stm += wxT(" = '") + escapeQuotes(
                columnDefsM[col]->getAsFirebirdString(buffersM[row]))
                + wxT("' WHERE ");
        }

        std::map<wxString, UniqueConstraint *>::iterator it =
            statementTablesM.find(table);

        // MB: please do not remove this check. Although it is not needed,
        //     it helped me detect some subtle bugs much easier
        if (it == statementTablesM.end() || (*it).second == 0)
            throw FRError(_("This column should not be editable"));

        addWhereAndExecute((*it).second, stm, table, oldRecord);
        delete oldRecord;
        return stm;
    }
    catch(...)
    {
        delete buffersM[row];       // delete the new record as it is invalid
        buffersM[row] = oldRecord;
        throw;
    }
}
//-----------------------------------------------------------------------------
