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

#include <wx/datetime.h>
#include <wx/ffile.h>
#include <wx/numformatter.h>
#include <wx/textbuf.h>

#include <algorithm>
#include <bitset>
#include <string>

#include "config/LocalSettings.h"
#include "core/FRError.h"
#include "core/FRInt128.h"
#include "core/Observer.h"
#include "core/ProgressIndicator.h"
#include "core/StringUtils.h"
#include "gui/controls/DataGridRowBuffer.h"
#include "gui/controls/DataGridRows.h"
#include "metadata/CharacterSet.h"
#include "metadata/column.h"
#include "metadata/database.h"
#include "metadata/table.h"


GridCellFormats::GridCellFormats()
    : ConfigCache(config())
{
}

GridCellFormats& GridCellFormats::get()
{
    static GridCellFormats gcf;
    return gcf;
}

void GridCellFormats::loadFromConfig()
{
    floatingPointPrecisionM = config().get("NumberPrecision", 2);
    if (!config().get("ReformatNumbers", false))
        floatingPointPrecisionM = -1;

    dateFormatM = config().get("DateFormat", wxString("D.M.Y"));
    timeFormatM = config().get("TimeFormat", wxString("H:M:S.T"));
    timestampFormatM = config().get("TimestampFormat",
        wxString("D.N.Y H:M:S.T"));
    showTimezoneInfoM = (ShowTimezoneInfoType)config().get("ShowTimezoneInfo", int(tzName));

    maxBlobKBytesM = config().get("DataGridFetchBlobAmount", 1);
    showBinaryBlobContentM = config().get("GridShowBinaryBlobs", false);
    showBlobContentM = config().get("DataGridFetchBlobs", true);
}

template<typename T>
wxString GridCellFormats::format(T value)
{
    ensureCacheValid();

    if (floatingPointPrecisionM >= 0 && floatingPointPrecisionM <= 18)
        return wxString::Format("%.*f", floatingPointPrecisionM, value);
    return wxString::Format("%f", value);
}

wxString GridCellFormats::formatDate(int year, int month, int day)
{
    ensureCacheValid();

    wxString result;
    for (wxString::const_iterator c = dateFormatM.begin();
        c != dateFormatM.end(); c++)
    {
        switch (wxChar(*c))
        {
            case 'd':
                result += wxString::Format("%d", day);
                break;
            case 'D':
                result += wxString::Format("%02d", day);
                break;
            case 'm':
                result += wxString::Format("%d", month);
                break;
            case 'M':
                result += wxString::Format("%02d", month);
                break;
            case 'y':
                result += wxString::Format("%02d", year % 100);
                break;
            case 'Y':
                result += wxString::Format("%04d", year);
                break;
            default:
                result += *c;
                break;
        }
    }
    return result;
}

bool getNumber(wxString::iterator& ci, wxString::iterator& end, int& toSet)
{
    wxString num;
    while (ci != end)
    {
        wxChar c = (wxChar)*ci;
        if (c < wxChar('0') || c > wxChar('9'))
            break;
        num += c;
        ++ci;
    }
    long l;
    if (num.IsEmpty() || !num.ToLong(&l))
        return false;
    toSet = l;
    return true;
}

bool GridCellFormats::parseDate(wxString::iterator& start,
    wxString::iterator end, bool consumeAll, int& year, int& month, int& day)
{
    ensureCacheValid();

    for (wxString::iterator c = dateFormatM.begin();
        c != dateFormatM.end() && start != end; ++c)
    {
        switch ((wxChar)*c)
        {
            case 'd':
            case 'D':
                if (!consumeAll && (*start < wxChar('0') || *start > wxChar('9')))
                    return true;
                if (!(getNumber(start, end, day) && day >= 1 && day <= 31))
                    return false;
                break;
            case 'm':
            case 'M':
                if (!consumeAll && (*start < wxChar('0') || *start > wxChar('9')))
                    return true;
                if (!(getNumber(start, end, month) && month >= 1 && month <= 12))
                    return false;
                break;
            case 'y':
                if (!consumeAll && (*start < wxChar('0') || *start > wxChar('9')))
                    return true;
                if (!getNumber(start, end, year))
                    return false;
                // see http://www.firebirdsql.org/doc/contrib/FirebirdDateLiterals.html
                if (year < 100)
                {
                    int thisYear = wxDateTime::Now().GetYear();
                    int cy = thisYear / 100;
                    int yearBefore = 100 * cy + year;
                    int yearAfter = yearBefore;
                    if (yearBefore > thisYear)
                        yearBefore -= 100;
                    else
                        yearAfter += 100;
                    if (thisYear - yearBefore <= yearAfter - thisYear)
                        year = yearBefore;
                    else
                        year = yearAfter;
                }
                break;
            case 'Y':
                if (!consumeAll && (*start < wxChar('0') || *start > wxChar('9')))
                    return true;
                if (!getNumber(start, end, year))
                    return false;
                break;
            default:        // other characters must match
                if (*c != *start)
                    return !consumeAll;
                ++start;
                break;
        }
    }
    return true;
}

wxString GridCellFormats::formatTime(IBPP::Time &t, bool hasTz, Database* db)
{
    ensureCacheValid();

    int hour, minute, second, tenththousands;
    t.GetTime(hour, minute, second, tenththousands);

    wxString result;
    for (wxString::iterator c = timeFormatM.begin(); c != timeFormatM.end();
        c++)
    {
        switch ((wxChar)*c)
        {
            case 'h':
                result += wxString::Format("%d", hour);
                break;
            case 'H':
                result += wxString::Format("%02d", hour);
                break;
            case 'm':
                result += wxString::Format("%d", minute);
                break;
            case 'M':
                result += wxString::Format("%02d", minute);
                break;
            case 's':
                result += wxString::Format("%d", second);
                break;
            case 'S':
                result += wxString::Format("%02d", second);
                break;
            case 'T':
                result += wxString::Format("%03d", tenththousands / 10);
                break;
            default:
                result += *c;
                break;
        }
    }
    formatAppendTz(result, t, hasTz, db);
    return result;
}

int GridCellFormats::maxBlobBytesToFetch()
{
    ensureCacheValid();
    return (maxBlobKBytesM > 0) ? 1024 * maxBlobKBytesM : INT_MAX;
}

bool GridCellFormats::parseTime(wxString::iterator& start,
    wxString::iterator end, int& hr, int& mn, int& sc, int& ml)
{
    ensureCacheValid();

    for (wxString::iterator c = timeFormatM.begin();
        c != timeFormatM.end() && start != end; c++)
    {
        switch ((wxChar)*c)
        {
            case 'h':
            case 'H':
                if (!(getNumber(start, end, hr) && hr >= 0 && hr <= 23))
                    return false;
                break;
            case 'm':
            case 'M':
                if (!(getNumber(start, end, mn) && mn >= 0 && mn <= 59))
                    return false;
                break;
            case 's':
            case 'S':
                if (!(getNumber(start, end, sc) && sc >= 0 && sc <= 59))
                    return false;
                break;
            case 'T':
                if (!(getNumber(start, end, ml) && ml >= 0 && ml <= 999))
                    return false;
                break;
            default:        // other characters must match
                if (*c != *start)
                    return false;
                start++;
                break;
        }
    }
    return true;
}

wxString GridCellFormats::formatTimestamp(IBPP::Timestamp &ts, bool hasTz,
    Database* db)
{
    ensureCacheValid();

    int year, month, day, hour, minute, second, tenththousands;
    ts.GetDate(year, month, day);
    ts.GetTime(hour, minute, second, tenththousands);

    wxString result;
    for (wxString::iterator c = timestampFormatM.begin();
        c != timestampFormatM.end(); c++)
    {
        switch ((wxChar)*c)
        {
            case 'd':
                result += wxString::Format("%d", day);
                break;
            case 'D':
                result += wxString::Format("%02d", day);
                break;
            case 'n':
                result += wxString::Format("%d", month);
                break;
            case 'N':
                result += wxString::Format("%02d", month);
                break;
            case 'y':
                result += wxString::Format("%02d", year % 100);
                break;
            case 'Y':
                result += wxString::Format("%04d", year);
                break;
            case 'h':
                result += wxString::Format("%d", hour);
                break;
            case 'H':
                result += wxString::Format("%02d", hour);
                break;
            case 'm':
                result += wxString::Format("%d", minute);
                break;
            case 'M':
                result += wxString::Format("%02d", minute);
                break;
            case 's':
                result += wxString::Format("%d", second);
                break;
            case 'S':
                result += wxString::Format("%02d", second);
                break;
            case 'T':
                result += wxString::Format("%03d", tenththousands / 10);
                break;
            default:
                result += *c;
                break;
        }
    }
    formatAppendTz(result, ts, hasTz, db);

    return result;
}

void GridCellFormats::formatAppendTz(wxString &s, IBPP::Time &t, bool hasTz,
    Database* db)
{
    if ((!hasTz) ||
        (showTimezoneInfoM == tzNone))
        return;

    int timezone = t.GetTimezone();
    if (showTimezoneInfoM == tzRawId)
    {
        if (t.IsTimeZoneGmtFallback())
            timezone = 0;
        s += wxString::Format(" (%05d)", timezone);
    }
    else
    {
        wxString tzName;
        if (t.IsTimeZoneGmtFallback())
            tzName = "GMT*";
        else
            tzName = db->getTimezoneName(timezone);
        s += wxString::Format(" (%s)", tzName);
    }
}

bool GridCellFormats::parseTimestamp(wxString::iterator& start,
    wxString::iterator end, int& year, int& month, int& day,
    int& hr, int& mn, int& sc, int& ml)
{
    ensureCacheValid();

    for (wxString::iterator c = timestampFormatM.begin();
        c != timestampFormatM.end() && start != end; ++c)
    {
        switch ((wxChar)*c)
        {
            case 'd':
            case 'D':
                if (*start < wxChar('0') || *start > wxChar('9'))
                    return true;
                if (!(getNumber(start, end, day) && day >= 1 && day <= 31))
                    return false;
                break;
            case 'n':
            case 'N':
                if (*start < wxChar('0') || *start > wxChar('9'))
                    return true;
                if (!(getNumber(start, end, month) && month >= 1 && month <= 12))
                    return false;
                break;
            case 'y':
                if (*start < wxChar('0') || *start > wxChar('9'))
                    return true;
                if (!getNumber(start, end, year))
                    return false;
                // see http://www.firebirdsql.org/doc/contrib/FirebirdDateLiterals.html
                if (year < 100)
                {
                    int thisYear = wxDateTime::Now().GetYear();
                    int cy = thisYear / 100;
                    int yearBefore = 100 * cy + year;
                    int yearAfter = yearBefore;
                    if (yearBefore > thisYear)
                        yearBefore -= 100;
                    else
                        yearAfter += 100;
                    if (thisYear - yearBefore <= yearAfter - thisYear)
                        year = yearBefore;
                    else
                        year = yearAfter;
                }
                break;
            case 'Y':
                if (*start < wxChar('0') || *start > wxChar('9'))
                    return true;
                if (!getNumber(start, end, year))
                    return false;
                break;
            case 'h':
            case 'H':
                if (!(getNumber(start, end, hr) && hr >= 0 && hr <= 23))
                    return false;
                break;
            case 'm':
            case 'M':
                if (!(getNumber(start, end, mn) && mn >= 0 && mn <= 59))
                    return false;
                break;
            case 's':
            case 'S':
                if (!(getNumber(start, end, sc) && sc >= 0 && sc <= 59))
                    return false;
                break;
            case 'T':
                if (!(getNumber(start, end, ml) && ml >= 0 && ml <= 999))
                    return false;
                break;
            default:        // other characters must match
                if (*c != *start)
                    return false;
                ++start;
                break;
        }
    }
    return true;
}

bool GridCellFormats::showBinaryBlobContent()
{
    ensureCacheValid();
    return showBinaryBlobContentM;
}

bool GridCellFormats::showBlobContent()
{
    ensureCacheValid();
    return showBlobContentM;
}

// ResultsetColumnDef class
ResultsetColumnDef::ResultsetColumnDef(const wxString& name, bool readonly,
    bool nullable)
    : nameM(name), readOnlyM(readonly), nullableM(nullable)
{
}

ResultsetColumnDef::~ResultsetColumnDef()
{
}

// needed to avoid strange date&time formatting if such column is PK/UNQ
wxString ResultsetColumnDef::getAsFirebirdString(DataGridRowBuffer* buffer)
{
    return getAsString(buffer, NULL);
}

wxString ResultsetColumnDef::getName()
{
    return nameM;
}

unsigned ResultsetColumnDef::getIndex()
{
    return 0;
}

bool ResultsetColumnDef::isNumeric()
{
    return false;
}

bool ResultsetColumnDef::isReadOnly()
{
    return readOnlyM;
}

bool ResultsetColumnDef::isNullable()
{
    return nullableM;
}

// DummyColumnDef class
class DummyColumnDef : public ResultsetColumnDef
{
public:
    DummyColumnDef(const wxString& name);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv*, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};

DummyColumnDef::DummyColumnDef(const wxString& name)
    : ResultsetColumnDef(name)
{
}

wxString DummyColumnDef::getAsString(DataGridRowBuffer*, Database*)
{
    return "[...]";
}

unsigned DummyColumnDef::getBufferSize()
{
    return 0;
}

void DummyColumnDef::setValue(DataGridRowBuffer* /*buffer*/, unsigned /*col*/,
    const IBPP::Statement& /*statement*/, wxMBConv* /*converter*/, Database* /*db*/)
{
}

void DummyColumnDef::setFromString(DataGridRowBuffer* /* buffer */,
         const wxString& /* source */)
{
}

// IntegerColumnDef class
class IntegerColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
public:
    IntegerColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};

IntegerColumnDef::IntegerColumnDef(const wxString& name, unsigned offset,
    bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset)
{
}

wxString IntegerColumnDef::getAsString(DataGridRowBuffer* buffer, Database*)
{
    wxASSERT(buffer);
    int value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;
    return wxString::Format("%d", value);
}

void IntegerColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    long value;
    if (!source.ToLong(&value))
        throw FRError(_("Invalid integer numeric value"));
    buffer->setValue(offsetM, (int)value);
}

unsigned IntegerColumnDef::getBufferSize()
{
    return sizeof(int);
}

bool IntegerColumnDef::isNumeric()
{
    return true;
}

void IntegerColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*, Database*)
{
    wxASSERT(buffer);
    int value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value);
}

// Int64ColumnDef class
class Int64ColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
public:
    Int64ColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};

Int64ColumnDef::Int64ColumnDef(const wxString& name, unsigned offset,
    bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset)
{
}

wxString Int64ColumnDef::getAsString(DataGridRowBuffer* buffer, Database*)
{
    wxASSERT(buffer);
    int64_t value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;
    return wxLongLong(value).ToString();
}

void Int64ColumnDef::setFromString(DataGridRowBuffer* buffer,
    const wxString& source)
{
    wxASSERT(buffer);

    wxLongLong_t ll;
    if (source.ToLongLong(&ll))
    {
        buffer->setValue(offsetM, (int64_t)ll);
        return;
    }

    // perhaps underlying library doesn't support 64bit, we try 32:
    long l;
    if (!source.ToLong(&l)) // nope, that fails as well
        throw FRError(_("Invalid 64bit numeric value"));
    buffer->setValue(offsetM, (int64_t)l);
}

unsigned Int64ColumnDef::getBufferSize()
{
    return sizeof(int64_t);
}

bool Int64ColumnDef::isNumeric()
{
    return true;
}

void Int64ColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*, Database*)
{
    wxASSERT(buffer);
    int64_t value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value);
}

// Int128ColumnDef class
class Int128ColumnDef : public ResultsetColumnDef
{
private:
    short scaleM;
    unsigned offsetM;
public:
    Int128ColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable, short scale);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};

Int128ColumnDef::Int128ColumnDef(const wxString& name, unsigned offset,
    bool readOnly, bool nullable, short scale)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset),
        scaleM(scale)
{
}

wxString Int128ColumnDef::getAsString(DataGridRowBuffer* buffer, Database*)
{
    wxASSERT(buffer);
    int128_t value;
    wxString result;
    int numStart;

    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;
    result = Int128ToString(value);
    if (scaleM > 0)
    {
        numStart = (result.GetChar(0) == '-') ? 1 : 0;

        while (result.length() < scaleM + numStart + 1)
            result.insert(numStart, "0");

        result.insert(result.length() - scaleM,
                      wxNumberFormatter::GetDecimalSeparator());
    }
    return result;
}

void Int128ColumnDef::setFromString(DataGridRowBuffer* buffer,
    const wxString& source)
{
    wxASSERT(buffer);
    int128_t v128 = 0;
    int decimalSeparatorIdx, localSourceScale;
    wxString errMsg;
    wxString localSource = source;
    wxChar decimalSeparator;

    if (scaleM > 0)
    {
        decimalSeparator = wxNumberFormatter::GetDecimalSeparator();
        decimalSeparatorIdx = localSource.rfind(decimalSeparator);

        if (decimalSeparatorIdx > -1)
        {
            localSource.erase(decimalSeparatorIdx,1);
            localSourceScale = localSource.Length() - decimalSeparatorIdx;
        }
        else
            localSourceScale = 0;

        // remove numbers if we are to big
        if (localSourceScale > scaleM)
            localSource.erase(localSource.Length() - localSourceScale + scaleM);
        // add 0 if we are to small
        while (localSourceScale < scaleM)
        {
            localSource = localSource + _("0");
            localSourceScale++;
        }
    }

    if (!StringToInt128(localSource, &v128, errMsg))
        throw FRError(errMsg);
    buffer->setValue(offsetM, v128);
}

unsigned Int128ColumnDef::getBufferSize()
{
    return sizeof(int128_t);
}

bool Int128ColumnDef::isNumeric()
{
    return true;
}

void Int128ColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*, Database*)
{
    wxASSERT(buffer);
    IBPP::ibpp_int128_t value;
    statement->Get(col, value);
    buffer->setValue(offsetM, *reinterpret_cast<int128_t*>(&value));
}

// DBKeyColumnDef class
class DBKeyColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
    unsigned sizeM;
public:
    DBKeyColumnDef(const wxString& name, unsigned offset, unsigned size);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
    void getDBKey(IBPP::DBKey& dbkey, DataGridRowBuffer* buffer);
};

DBKeyColumnDef::DBKeyColumnDef(const wxString& name, unsigned offset,
    unsigned size)
    : ResultsetColumnDef(name, true, false), offsetM(offset), sizeM(size)
{
}

wxString DBKeyColumnDef::getAsString(DataGridRowBuffer* buffer, Database*)
{
    wxASSERT(buffer);
    wxString ret;
    for (int i = 0; i < (int)sizeM / 8; i++)
    {
        if (i > 0)
            ret += "-";
        int v1, v2;
        buffer->getValue(offsetM+i*8, v1);
        buffer->getValue(offsetM+i*8+4, v2);
        ret += wxString::Format("%08x:%08x", (unsigned)v1, (unsigned)v2);
    }
    return ret;
}

void DBKeyColumnDef::setFromString(DataGridRowBuffer* /*buffer*/,
    const wxString& /*source*/)
{
    // should never be editable
}

unsigned DBKeyColumnDef::getBufferSize()
{
    return sizeM;
}

bool DBKeyColumnDef::isNumeric()
{
    return false;
}

void DBKeyColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*, Database*)
{
    wxASSERT(buffer);
    IBPP::DBKey value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value);
}

void DBKeyColumnDef::getDBKey(IBPP::DBKey& dbkey, DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    buffer->getValue(offsetM, dbkey, sizeM);
}

// DateColumnDef class
class DateColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
public:
    DateColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable);
    virtual wxString getAsFirebirdString(DataGridRowBuffer* buffer);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};

DateColumnDef::DateColumnDef(const wxString& name, unsigned offset,
    bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset)
{
}

wxString DateColumnDef::getAsString(DataGridRowBuffer* buffer, Database*)
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

wxString DateColumnDef::getAsFirebirdString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    int value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;

    IBPP::Date date(value);
    int year, month, day;
    date.GetDate(year, month, day);
    return wxString::Format("%d-%d-%d", year, month, day);
}

void DateColumnDef::setFromString(DataGridRowBuffer* buffer,
    const wxString& source)
{
    wxASSERT(buffer);
    IBPP::Date idt;
    idt.Today();
    int y = idt.Year();  // defaults
    int m = idt.Month();
    int d = idt.Day();

    wxString temp(source);
    temp.Trim(true).Trim(false);

    if (temp.CmpNoCase("TOMORROW") == 0)
        idt.Add(1);
    else if (temp.CmpNoCase("YESTERDAY") == 0)
        idt.Add(-1);
    else if (temp.CmpNoCase("DATE") != 0
        && temp.CmpNoCase("NOW") != 0
        && temp.CmpNoCase("TODAY") != 0
        && temp.CmpNoCase("CURRENT_DATE") != 0
        && temp.CmpNoCase("CURRENT_TIMESTAMP") != 0)
    {
        wxString::iterator it = temp.begin();
        if (!GridCellFormats::get().parseDate(it, temp.end(), true, y, m, d))
            throw FRError(_("Cannot parse date"));
        idt.SetDate(y, m, d);
    }
    buffer->setValue(offsetM, idt.GetDate());
}

unsigned DateColumnDef::getBufferSize()
{
    return sizeof(int);
}

void DateColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*, Database*)
{
    wxASSERT(buffer);
    IBPP::Date value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value.GetDate());
}

// TimeColumnDef class
class TimeColumnDef : public ResultsetColumnDef
{
private:
    bool withTimezoneM;
    unsigned offsetM;
    bool readFromBuffer(DataGridRowBuffer* buffer, IBPP::Time &t);
    void writeToBuffer(DataGridRowBuffer* buffer, IBPP::Time &t);
public:
    TimeColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable, bool withTimezone);
    virtual wxString getAsFirebirdString(DataGridRowBuffer* buffer);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};

TimeColumnDef::TimeColumnDef(const wxString& name, unsigned offset,
    bool readOnly, bool nullable, bool withTimezone)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset),
        withTimezoneM(withTimezone)
{
}

bool TimeColumnDef::readFromBuffer(DataGridRowBuffer* buffer, IBPP::Time &t)
{
    wxASSERT(buffer);
    int vTime;
    IBPP::Time::TimezoneMode tzMode;
    int vTimezone = 0;

    if (!buffer->getValue(offsetM, vTime))
        return false;

    if (withTimezoneM)
    {
        bool vIsGmtFallback;
        if (!buffer->getValue32(offsetM + sizeof(int),
                                vTimezone, vIsGmtFallback))
            return wxEmptyString;

        tzMode = IBPP::Time::tmTimezone;
        if (vIsGmtFallback)
            tzMode = IBPP::Time::tmTimezoneGmtFallback;
    }
    else
    {
        tzMode = IBPP::Time::tmNone;
        vTimezone = IBPP::Time::TZ_NONE;
    }
    t.SetTime(tzMode, vTime, vTimezone);
    return true;
}

void TimeColumnDef::writeToBuffer(DataGridRowBuffer* buffer, IBPP::Time &t)
{
    wxASSERT(buffer);
    buffer->setValue(offsetM, t.GetTime());
    if (withTimezoneM)
        buffer->setValue32(offsetM + sizeof(int),
            t.GetTimezone(), t.IsTimeZoneGmtFallback());
}

wxString TimeColumnDef::getAsString(DataGridRowBuffer* buffer, Database* db)
{
    IBPP::Time time;

    if (!readFromBuffer(buffer, time))
        return wxEmptyString;

    return GridCellFormats::get().formatTime(time, withTimezoneM, db);
}

wxString TimeColumnDef::getAsFirebirdString(DataGridRowBuffer* buffer)
{
    IBPP::Time time;

    if (!readFromBuffer(buffer, time))
        return wxEmptyString;

    int hour, minute, second, tenththousands;
    time.GetTime(hour, minute, second, tenththousands);
    return wxString::Format("%d:%d:%d.%d", hour, minute, second,
        tenththousands / 10);
}

void TimeColumnDef::setFromString(DataGridRowBuffer* buffer,
    const wxString& source)
{
    wxASSERT(buffer);
    IBPP::Time itm;
    itm.Now();

    wxString temp(source);
    temp.Trim(true).Trim(false);

    if (temp.CmpNoCase("TIME") != 0 && temp.CmpNoCase("NOW") != 0)
    {
        wxString::iterator it = temp.begin();
        int hr = 0, mn = 0, sc = 0, ms = 0;
        if (!GridCellFormats::get().parseTime(it, temp.end(), hr, mn, sc, ms))
            throw FRError(_("Cannot parse time"));
        itm.SetTime(IBPP::Time::tmNone, hr, mn, sc, 10 * ms, IBPP::Time::TZ_NONE, NULL);
    }
    writeToBuffer(buffer, itm);
}

unsigned TimeColumnDef::getBufferSize()
{
    int result = sizeof(int);
    if (withTimezoneM)
        result += sizeof(int);
    return result;
}

void TimeColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*, Database*)
{
    wxASSERT(buffer);
    IBPP::Time value;
    statement->Get(col, value);
    writeToBuffer(buffer, value);
}

// TimestampColumnDef class
class TimestampColumnDef : public ResultsetColumnDef
{
private:
    bool withTimezoneM;
    unsigned offsetM;
    bool readFromBuffer(DataGridRowBuffer* buffer, IBPP::Timestamp &ts);
    void writeToBuffer(DataGridRowBuffer* buffer, IBPP::Timestamp &ts);
public:
    TimestampColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable, bool withTimezone);
    virtual wxString getAsFirebirdString(DataGridRowBuffer* buffer);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};

TimestampColumnDef::TimestampColumnDef(const wxString& name, unsigned offset,
    bool readOnly, bool nullable, bool withTimezone)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset),
        withTimezoneM(withTimezone)
{
}

void TimestampColumnDef::writeToBuffer(DataGridRowBuffer* buffer, IBPP::Timestamp &ts)
{
    wxASSERT(buffer);
    buffer->setValue(offsetM, ts.GetDate());
    buffer->setValue(offsetM + sizeof(int), ts.GetTime());
    if (withTimezoneM)
        buffer->setValue32(offsetM + sizeof(int) * 2,
            ts.GetTimezone(), ts.IsTimeZoneGmtFallback());
}

bool TimestampColumnDef::readFromBuffer(DataGridRowBuffer* buffer, IBPP::Timestamp &ts)
{
    wxASSERT(buffer);
    int vDate, vTime;
    IBPP::Time::TimezoneMode tzMode;

    if (!buffer->getValue(offsetM, vDate))
        return false;

    if (!buffer->getValue(offsetM + sizeof(int), vTime))
        return false;

    int vTimezone = 0;
    if (withTimezoneM)
    {
        bool vIsGmtFallback;
        if (!buffer->getValue32(offsetM + sizeof(int) * 2,
                                vTimezone, vIsGmtFallback))
            return wxEmptyString;

        tzMode = IBPP::Time::tmTimezone;
        if (vIsGmtFallback)
            tzMode = IBPP::Time::tmTimezoneGmtFallback;
    }
    else
    {
        tzMode = IBPP::Time::tmNone;
        vTimezone = IBPP::Time::TZ_NONE;
    }

    ts.SetDate(vDate);
    ts.SetTime(tzMode, vTime, vTimezone);

    return true;
}

wxString TimestampColumnDef::getAsString(DataGridRowBuffer* buffer, Database* db)
{
    IBPP::Timestamp ts;

    if (!readFromBuffer(buffer, ts))
        return wxEmptyString;

    return GridCellFormats::get().formatTimestamp(ts, withTimezoneM, db);
}

wxString TimestampColumnDef::getAsFirebirdString(DataGridRowBuffer* buffer)
{
    IBPP::Timestamp ts;

    if (!readFromBuffer(buffer, ts))
        return wxEmptyString;

    int year, month, day, hour, minute, second, tenththousands;
    ts.GetDate(year, month, day);
    ts.GetTime(hour, minute, second, tenththousands);

    return wxString::Format("%d-%d-%d %d:%d:%d.%d", year, month, day,
        hour, minute, second, tenththousands / 10);
}

void TimestampColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    IBPP::Timestamp its;
    its.Today(); // defaults to no time

    wxString temp(source);
    temp.Trim(true).Trim(false);

    if (temp.CmpNoCase("TOMORROW") == 0)
        its.Add(1);
    else if (temp.CmpNoCase("YESTERDAY") == 0)
        its.Add(-1);
    else if ((temp.CmpNoCase("NOW") == 0) || 
             (temp.CmpNoCase("CURRENT_TIMESTAMP") == 0))
        its.Now(); // with time
    else if (temp.CmpNoCase("DATE") != 0
        && temp.CmpNoCase("TODAY") != 0)
    {
        // get date
        int y = its.Year();  // defaults
        int m = its.Month();
        int d = its.Day();
        int hr = 0, mn = 0, sc = 0, ms = 0;
        wxString::iterator it = temp.begin();
        if (!GridCellFormats::get().parseTimestamp(it, temp.end(),
            y, m, d, hr, mn, sc, ms))
        {
            throw FRError(_("Cannot parse timestamp"));
        }
        its.SetDate(y, m, d);
        its.SetTime(IBPP::Time::tmNone, hr, mn, sc, 10 * ms, IBPP::Time::TZ_NONE, NULL);
    }

    writeToBuffer(buffer, its);
}

unsigned TimestampColumnDef::getBufferSize()
{
    int result = 2 * sizeof(int);
    if (withTimezoneM)
        result += sizeof(int);
    return result;
}

void TimestampColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*, Database*)
{
    wxASSERT(buffer);
    IBPP::Timestamp value;
    statement->Get(col, value);
    writeToBuffer(buffer, value);
}

// FloatColumnDef class
class FloatColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
public:
    FloatColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};

FloatColumnDef::FloatColumnDef(const wxString& name, unsigned offset,
        bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset)
{
}

wxString FloatColumnDef::getAsString(DataGridRowBuffer* buffer, Database*)
{
    wxASSERT(buffer);
    float value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;

    return GridCellFormats::get().format<float>(value);
}

void FloatColumnDef::setFromString(DataGridRowBuffer* buffer,
    const wxString& source)
{
    wxASSERT(buffer);
    double d;
    if (!source.ToDouble(&d))
        throw FRError(_("Invalid float numeric value"));
    buffer->setValue(offsetM, (float)d);
}

unsigned FloatColumnDef::getBufferSize()
{
    return sizeof(float);
}

bool FloatColumnDef::isNumeric()
{
    return true;
}

void FloatColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*, Database*)
{
    wxASSERT(buffer);
    float value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value);
}

// DoubleColumnDef class
class DoubleColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
    short scaleM;
public:
    DoubleColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable, short scale);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};

DoubleColumnDef::DoubleColumnDef(const wxString& name, unsigned offset,
        bool readOnly, bool nullable, short scale)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset),
        scaleM(scale)
{
}

wxString DoubleColumnDef::getAsString(DataGridRowBuffer* buffer, Database*)
{
    wxASSERT(buffer);
    double value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;

    if (scaleM)
        return wxString::Format("%.*f", scaleM, value);
    return GridCellFormats::get().format<double>(value);
}

void DoubleColumnDef::setFromString(DataGridRowBuffer* buffer,
    const wxString& source)
{
    wxASSERT(buffer);
    double d;
    if (!source.ToDouble(&d))
        throw FRError(_("Invalid double numeric value"));
    buffer->setValue(offsetM, d);
}

unsigned DoubleColumnDef::getBufferSize()
{
    return sizeof(double);
}

bool DoubleColumnDef::isNumeric()
{
    return true;
}

void DoubleColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*, Database*)
{
    wxASSERT(buffer);
    double value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value);
}

// Dec16ColumnDef class
class Dec16ColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
public:
    Dec16ColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};

Dec16ColumnDef::Dec16ColumnDef(const wxString& name, unsigned offset,
        bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset)
{
}

wxString Dec16ColumnDef::getAsString(DataGridRowBuffer* buffer, Database*)
{
    wxASSERT(buffer);
    dec16_t value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;
    return Dec16DPDToString(value);
}

void Dec16ColumnDef::setFromString(DataGridRowBuffer* buffer,
    const wxString& source)
{
    wxASSERT(buffer);
    dec16_t value;
    wxString errMsg;
    if (!StringToDec16DPD(source, &value, errMsg))
        throw FRError(errMsg);
    buffer->setValue(offsetM, value);
}

unsigned Dec16ColumnDef::getBufferSize()
{
    return sizeof(dec16_t);
}

bool Dec16ColumnDef::isNumeric()
{
    return true;
}

void Dec16ColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*, Database*)
{
    wxASSERT(buffer);
    dec16_t value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value);
}

// Dec34ColumnDef class
class Dec34ColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
public:
    Dec34ColumnDef(const wxString& name, unsigned offset, bool readOnly,
        bool nullable);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};

Dec34ColumnDef::Dec34ColumnDef(const wxString& name, unsigned offset,
        bool readOnly, bool nullable)
    : ResultsetColumnDef(name, readOnly, nullable), offsetM(offset)
{
}

wxString Dec34ColumnDef::getAsString(DataGridRowBuffer* buffer, Database*)
{
    wxASSERT(buffer);
    dec34_t value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;
    return Dec34DPDToString(value);
}

void Dec34ColumnDef::setFromString(DataGridRowBuffer* buffer,
    const wxString& source)
{
    wxASSERT(buffer);
    dec34_t value;
    wxString errMsg;
    if (!StringToDec34DPD(source, &value, errMsg))
        throw FRError(errMsg);
    buffer->setValue(offsetM, value);
}

unsigned Dec34ColumnDef::getBufferSize()
{
    return sizeof(dec34_t);
}

bool Dec34ColumnDef::isNumeric()
{
    return true;
}

void Dec34ColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*, Database*)
{
    wxASSERT(buffer);
    dec34_t value;
    statement->Get(col, value);
    buffer->setValue(offsetM, value);
}

// BlobColumnDef class
class BlobColumnDef : public ResultsetColumnDef
{
private:
    unsigned indexM, stringIndexM;
    bool textualM;
    wxMBConv* converterM;
public:
    BlobColumnDef(const wxString& name, bool readOnly, bool nullable,
        unsigned stringIndex, unsigned blobIndex, bool textual, wxMBConv* converterM = 0);
    void reset(DataGridRowBuffer* buffer);
    virtual unsigned getIndex();
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
    bool isTextual() { return textualM; };
};

BlobColumnDef::BlobColumnDef(const wxString& name, bool readOnly,
        bool nullable, unsigned stringIndex, unsigned blobIndex, bool textual, wxMBConv* converterM)
    : ResultsetColumnDef(name, readOnly, nullable), indexM(blobIndex),
        textualM(textual), stringIndexM(stringIndex), converterM(converterM)
{
    //readOnlyM = true;   // TODO: uncomment this when we make BlobDialog
}

void BlobColumnDef::reset(DataGridRowBuffer* buffer)
{
    buffer->setStringLoaded(stringIndexM, false);
}

unsigned BlobColumnDef::getIndex()
{
    return indexM;
}

wxString BlobColumnDef::getAsString(DataGridRowBuffer* grid_buffer, Database*)
{
    wxASSERT(grid_buffer);
    if (grid_buffer->isStringLoaded(stringIndexM))
        return grid_buffer->getString(stringIndexM);
    if (!GridCellFormats::get().showBlobContent())
        return _("[BLOB]");
    if (!textualM && !GridCellFormats::get().showBinaryBlobContent())
        return _("[BINARY]");

    IBPP::Blob *b0 = grid_buffer->getBlob(indexM);
    if (!b0)
        return "";
    IBPP::Blob b = *b0;
    try
    {
        b->Open();
    }
    catch(...)
    {
        return _("[ERROR]");
    }

    std::string result;
    int bytesToFetch = GridCellFormats::get().maxBlobBytesToFetch();
    while (bytesToFetch > 0)
    {
        char buffer[1025];
        int size = b->Read((void*)buffer, 1024);
        if (size < 1)
            break;
        bytesToFetch -= size;
        if (textualM)
        {
            std::string s(buffer, size);
            result += s;    // we don't convert here due to incomplete strings
        }
        else    // binary (show as hexadecimal)
        {
            for (int i=0; i<size; i+=8)
            {
                int last = 8;
                if (i+last >= size)
                    last = size - i;
                for (int j=0; j<last; j++)
                {
                    result += wx2std(wxString::Format("%02X",
                        (unsigned char)(buffer[i+j])));
                }
                result += " ";
                if (((i + 8) % 32) == 0)
                    result += "\n";
            }
        }
    }
    b->Close();
    wxString wxs(result.c_str(), *converterM);
    if (bytesToFetch <= 0)    // there was more data to fetch
    {               // incomplete strings might not get translated properly
        while (wxs.IsEmpty() && result.length() > 0)
        {
            result.erase(result.length()-1, 1); // remove last byte
            wxs = wxString(result.c_str(), *converterM);   // try converting again
        }
    }
    grid_buffer->setString(stringIndexM, wxs);
    return wxs;
}

void BlobColumnDef::setFromString(DataGridRowBuffer* /*buffer*/,
    const wxString& /*source*/)
{
    // wxASSERT(buffer);
    // TODO: is this called from anywhere? - blobs will have a custom editor
    // buffer->setString(indexM, source);
}

unsigned BlobColumnDef::getBufferSize()
{
    return 0;
}

void BlobColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv*, Database* db)
{
    wxASSERT(buffer);
    IBPP::Blob b = IBPP::BlobFactory(statement->DatabasePtr(),
        statement->TransactionPtr());
    statement->Get(col, b);
    buffer->setBlob(indexM, b);
    converterM = db->getCharsetConverter(); // store for later when we fetch the data
}

// StringColumnDef class
class StringColumnDef : public ResultsetColumnDef
{
protected:
    unsigned indexM;
    int charSizeM;
public:
    StringColumnDef(const wxString& name, unsigned stringIndex, bool readOnly,
        bool nullable, int charSize);
    virtual unsigned getIndex();
    virtual wxString getAsFirebirdString(DataGridRowBuffer* buffer);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter, Database* db);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};

StringColumnDef::StringColumnDef(const wxString& name, unsigned stringIndex,
    bool readOnly, bool nullable, int charSize)
    : ResultsetColumnDef(name, readOnly, nullable), indexM(stringIndex),
      charSizeM(charSize)
{
}

unsigned StringColumnDef::getIndex()
{
    return indexM;
}

wxString StringColumnDef::getAsFirebirdString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    wxString s(buffer->getString(indexM));
    // SF bug #1889800: quote chars have to be escaped
    s.Replace("'", "''");
    return s;
}

wxString StringColumnDef::getAsString(DataGridRowBuffer* buffer, Database*)
{
    wxASSERT(buffer);
    return buffer->getString(indexM);
}

void StringColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    // TODO: if CHARACTER SET OCTETS - check if it is a valid hexdec string
    buffer->setString(indexM, source);
}

unsigned StringColumnDef::getBufferSize()
{
    return 0;
}

void StringColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement, wxMBConv* converter, Database* /*db*/)
{
    wxASSERT(buffer);
    if (statement->ColumnType(col) == IBPP::sdBoolean) // Firebird v3
    {
        bool value; // UGLY, must create a specific Columm (child one ?)
        statement->Get(col, value);
        wxString val = value ? "true" : "false";
        buffer->setString(indexM, val);
    }
    else if (statement->ColumnSubtype(col) == 1)   // charset OCTETS
    {
        std::string value;
        statement->Get(col, value);
        wxString val;
        for (std::string::size_type p = 0; p < value.length(); p++)
            val += wxString::Format("%02x", uint8_t(value[p]));
        buffer->setString(indexM, val);
    }
    else
    {
        std::string value;
        //wxMBConv* converter = db->getCharsetConverter();
        statement->Get(col, value);
        wxString val = wxString(value.c_str(), *converter);
        size_t trimLen = val.Strip().Length();
        if (val.Length() > size_t(charSizeM))
            val.Truncate(trimLen > size_t(charSizeM) ? trimLen : charSizeM);
        buffer->setString(indexM, val);
    }
}

class BooleanColumnDef : public StringColumnDef // Firebird v3
{
public:
    BooleanColumnDef(const wxString& name, unsigned stringIndex, bool readOnly, bool nullable);
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col, const IBPP::Statement& statement);
};

BooleanColumnDef::BooleanColumnDef(const wxString& name, unsigned stringIndex,
    bool readOnly, bool nullable) : StringColumnDef(name, stringIndex, readOnly, nullable, 5)
{
}

void BooleanColumnDef::setValue(DataGridRowBuffer* buffer, unsigned col,
    const IBPP::Statement& statement)
{
    wxASSERT(buffer);
    bool value;
    statement->Get(col, value);
    wxString val = value ? "true" : "false";
    buffer->setString(StringColumnDef::indexM, val);
}

// DataGridRows class
DataGridRows::DataGridRows(Database* db)
    : bufferSizeM(0), databaseM(db), readOnlyM(false)
{
}

DataGridRows::~DataGridRows()
{
    clear();
}

ResultsetColumnDef* DataGridRows::getColumnDef(unsigned col)
{
    return columnDefsM[col];
}

void DataGridRows::addRow(DataGridRowBuffer* buffer)
{
    if (buffersM.size() == buffersM.capacity())
        buffersM.reserve(buffersM.capacity() + 1024);
    buffersM.push_back(buffer);
}

void DataGridRows::addRow(const IBPP::Statement& statement)
{
    DataGridRowBuffer* buffer = new DataGridRowBuffer(columnDefsM.size());
    // if anything fails, make sure we release the memory
    try
    {
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
            {
                columnDefsM[col]->setValue(buffer, colIBPP, statement,
                    databaseM->getCharsetConverter(), databaseM);
            }
        }
        while (col > 0);
    }
    catch(...)
    {
        delete buffer;
        throw;
    }
    addRow(buffer);
}

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

bool DataGridRows::canRemoveRow(size_t row)
{
    if (row >= buffersM.size())
        return false;
    // check that it is safe to call statementM->Columns()
    if (statementM->Type() == IBPP::stUnknown)
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
                    wxString cn(std2wxIdentifier(statementM->ColumnName(c2),
                        databaseM->getCharsetConverter()));
                    if ((*ci) != cn)
                        continue;
                    wxString tn(std2wxIdentifier(statementM->ColumnTable(c2),
                        databaseM->getCharsetConverter()));
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
        wxString s = "DELETE FROM "
            + Identifier((*deleteFromM).first).getQuoted() + " WHERE ";
        IBPP::Statement st = addWhere((*deleteFromM).second, s,
            (*deleteFromM).first, buffersM[from+pos]);
        st->Execute();
        stm += s + ";";
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
        if (it != buffersM.begin ())//Prevents it from going before the first one
          it--;
        (*i2)->setIsDeleted(true);
    }
    return true;
}

unsigned DataGridRows::getRowCount()
{
    return buffersM.size();
}

unsigned DataGridRows::getRowFieldCount()
{
    return columnDefsM.size();
}

wxString DataGridRows::getRowFieldName(unsigned col)
{
    if (col < columnDefsM.size())
        return columnDefsM[col]->getName();
    return wxEmptyString;
}

void checkColumnsPresent(const Database* database,
    const IBPP::Statement& statement, UniqueConstraint** locator)
{
    wxString tableName = (*locator)->getTable()->getName_();
    for (ColumnConstraint::const_iterator ci = (*locator)->begin(); ci !=
        (*locator)->end(); ++ci)
    {
        bool found = false;
        for (int c2 = 1; c2 <= statement->Columns(); ++c2)
        {
            wxString cn(std2wxIdentifier(statement->ColumnName(c2),
                database->getCharsetConverter()));
            wxString tn(std2wxIdentifier(statement->ColumnTable(c2),
                database->getCharsetConverter()));
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
    if (readOnlyM) // read-only transaction
    {
        readOnly = true;
        return;
    }

    if (statementM->ColumnType(col) == IBPP::sdString
        && statementM->ColumnSubtype(col) == 1) // charset OCTETS
    {                       // TODO: to make those editable, we need to
        //readOnly = true;    // enter values as parameters. This should
        //return;             // probably be done together with BLOB support
    }

    wxString tabName(std2wxIdentifier(statementM->ColumnTable(col),
        databaseM->getCharsetConverter()));
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
            checkColumnsPresent(databaseM, statementM, &locator);
        if (!locator)   // PK not present or not usable, try UNQ
        {
            std::vector<UniqueConstraint> *uq = t->getUniqueConstraints();
            if (uq)
            {
                for (std::vector<UniqueConstraint>::iterator ui = uq->begin();
                    ui != uq->end(); ++ui)
                {
                    locator = &(*ui);
                    checkColumnsPresent(databaseM, statementM, &locator);
                    if (locator)
                        break;
                }
            }
        }
        if (!locator)   // neither PK nor UNQ found, look for RDB$DB_KEY
        {
            UniqueConstraint uc;
            uc.getColumns().push_back("DB_KEY");
            uc.setParent(t);
            locator = &uc;
            checkColumnsPresent(databaseM, statementM, &locator);
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
        wxString cn(std2wxIdentifier(statementM->ColumnName(col),
            databaseM->getCharsetConverter()));
        t->ensureChildrenLoaded();
        Column* c = t->findColumn(cn).get();
        readOnly = (c == 0 || !c->getComputedSource().empty());
        if (!readOnly)  // it is editable, so check if nullable
            nullable = c->isNullable(CheckDomainNullability);
    }

    /* wxMessageBox(wxString::Format("TABLE: %s (RO=%d), COLUMN: %s (RO=%d, NULL=%d)"),
        tabName.c_str(),
        locator ? 0 : 1,
        wxString(statementM->ColumnName(col)).c_str(),
        readOnly ? 1 : 0,
        nullable ? 1 : 0)
    );*/
}

bool DataGridRows::initialize(const IBPP::Statement& statement)
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
        getColumnInfo(databaseM, col, readOnly, nullable);

        wxString colName(statement->ColumnAlias(col),
            *databaseM->getCharsetConverter());
        if (colName.empty())
        {
            colName = wxString(statement->ColumnName(col),
                *databaseM->getCharsetConverter());
        }

        IBPP::SDT type = statement->ColumnType(col);
        short scale = statement->ColumnScale(col);
        if ((scale > 0) &&
            (type != IBPP::sdInt128) &&
            (type != IBPP::sdDec16) &&
            (type != IBPP::sdDec34))
            type = IBPP::sdDouble;

        ResultsetColumnDef* columnDef = 0;
        if (std::string(statement->ColumnName(col)) == "DB_KEY")
            columnDef = new DBKeyColumnDef(colName, bufferSizeM, statement->ColumnSize(col));
        else
        {
            switch (type)
            {
                case IBPP::sdBoolean: // Firebird v3
                    columnDef = new BooleanColumnDef(colName, stringIndex, readOnly, nullable);
                    ++stringIndex;
                    break;
                case IBPP::sdDate:
                    columnDef = new DateColumnDef(colName, bufferSizeM, readOnly, nullable);
                    break;
                case IBPP::sdTime:
                    columnDef = new TimeColumnDef(colName, bufferSizeM, readOnly, nullable, false);
                    break;
                case IBPP::sdTimeTz:
                    columnDef = new TimeColumnDef(colName, bufferSizeM, readOnly, nullable, true);
                    break;
                case IBPP::sdTimestamp:
                    columnDef = new TimestampColumnDef(colName, bufferSizeM, readOnly, nullable, false);
                    break;
                case IBPP::sdTimestampTz:
                    columnDef = new TimestampColumnDef(colName, bufferSizeM, readOnly, nullable, true);
                    break;

                case IBPP::sdSmallint:
                case IBPP::sdInteger:
                    columnDef = new IntegerColumnDef(colName, bufferSizeM, readOnly, nullable);
                    break;
                case IBPP::sdLargeint:
                    columnDef = new Int64ColumnDef(colName, bufferSizeM, readOnly, nullable);
                    break;
                case IBPP::sdInt128:
                    columnDef = new Int128ColumnDef(colName, bufferSizeM, readOnly, nullable, scale);
                    break;

                case IBPP::sdFloat:
                    columnDef = new FloatColumnDef(colName, bufferSizeM, readOnly, nullable);
                    break;
                case IBPP::sdDouble:
                    columnDef = new DoubleColumnDef(colName, bufferSizeM, readOnly, nullable, scale);
                    break;
                case IBPP::sdDec16:
                    columnDef = new Dec16ColumnDef(colName, bufferSizeM, readOnly, nullable);
                    break;
                case IBPP::sdDec34:
                    columnDef = new Dec34ColumnDef(colName, bufferSizeM, readOnly, nullable);
                    break;

                case IBPP::sdString:
                {
                    int bpc = databaseM->getCharsetById(statement->ColumnSubtype(col))->getBytesPerChar();
                    int size = statement->ColumnSize(col);
                    if (bpc)
                        size /= bpc;
                    columnDef = new StringColumnDef(colName, stringIndex, readOnly, nullable, size);
                    ++stringIndex;
                    break;
                }
                case IBPP::sdBlob:
                    columnDef = new BlobColumnDef(colName, readOnly, nullable, stringIndex, blobIndex, statement->ColumnSubtype(col) == 1, this->databaseM->getCharsetConverter());
                    ++blobIndex;    // stores blob handle
                    ++stringIndex;  // stored blob data (fetched on demand)
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

bool DataGridRows::isColumnNullable(unsigned col)
{
    if (col >= columnDefsM.size())
        return false;
    return columnDefsM[col]->isNullable();
}

bool DataGridRows::isColumnNumeric(unsigned col)
{
    if (col >= columnDefsM.size())
        return false;
    return columnDefsM[col]->isNumeric();
}

bool DataGridRows::isColumnReadonly(unsigned col)
{
    if (col >= columnDefsM.size())
        return false;
    return columnDefsM[col]->isReadOnly();
}

bool DataGridRows::getFieldInfo(unsigned row, unsigned col,
    DataGridFieldInfo& info)
{
    if (col >= columnDefsM.size() || row >= buffersM.size())
        return false;
    info.rowInserted = buffersM[row]->isInserted();
    info.rowDeleted = buffersM[row]->isDeleted();
    info.fieldReadOnly = readOnlyM || info.rowDeleted
        || isColumnReadonly(col) || isFieldReadonly(row, col);
    info.fieldModified = !info.rowDeleted
        && buffersM[row]->isFieldModified(col);
    info.fieldNull = buffersM[row]->isFieldNull(col);
    info.fieldNA = buffersM[row]->isFieldNA(col);
    info.fieldNumeric = isColumnNumeric(col);
    info.fieldBlob = isBlobColumn(col);
    return true;
}

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

    wxString table(std2wxIdentifier(statementM->ColumnTable(col + 1),
        databaseM->getCharsetConverter()));
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
            wxString cn(std2wxIdentifier(statementM->ColumnName(c2),
                databaseM->getCharsetConverter()));
            if ((*ci) != cn)
                continue;
            wxString tn(std2wxIdentifier(statementM->ColumnTable(c2),
                databaseM->getCharsetConverter()));
            if (tn == table && buffersM[row]->isFieldNA(c2-1))
                return true;
        }
    }
    return false;
}

wxString DataGridRows::getFieldValue(unsigned row, unsigned col)
{
    if (row >= buffersM.size() || col >= columnDefsM.size())
        return wxEmptyString;
    return columnDefsM[col]->getAsString(buffersM[row], databaseM);
}

bool DataGridRows::isFieldNull(unsigned row, unsigned col)
{
    if (row >= buffersM.size())
        return false;
    return buffersM[row]->isFieldNull(col);
}

bool DataGridRows::isFieldNA(unsigned row, unsigned col)
{
    if (row >= buffersM.size())
        return false;
    return buffersM[row]->isFieldNA(col);
}

IBPP::Statement DataGridRows::addWhere(UniqueConstraint* uq, wxString& stm,
    const wxString& table, DataGridRowBuffer *buffer)
{
    bool have_dbkey = false;
    for (ColumnConstraint::const_iterator ci = uq->begin(); ci !=
        uq->end(); ++ci)
    {
        if ((*ci) == "DB_KEY")
        {
            stm += " RDB$DB_KEY = ?";
            have_dbkey = true;
            break;
        }
        for (int c2 = 1; c2 <= statementM->Columns(); ++c2)
        {
            wxString cn(std2wxIdentifier(statementM->ColumnName(c2),
                databaseM->getCharsetConverter()));
            wxString tn(std2wxIdentifier(statementM->ColumnTable(c2),
                databaseM->getCharsetConverter()));
            if (cn == (*ci) && tn == table) // found it, add to WHERE list
            {
                if (buffer->isFieldNA(c2-1))
                    throw FRError(_("N/A value in key column."));
                if (ci != uq->begin())
                    stm += " AND ";
                if ((statementM->ColumnType(c2) == IBPP::SDT::sdString) && (statementM->ColumnSubtype(c2) == 1) ) //OCTET
                    stm += Identifier(cn).getQuoted() + " = x'";
                else
                    stm += Identifier(cn).getQuoted() + " = '";

                stm += columnDefsM[c2-1]->getAsFirebirdString(buffer);
                stm += "'";
                break;
            }
        }
    }

    IBPP::Statement st = IBPP::StatementFactory(statementM->DatabasePtr(),
        statementM->TransactionPtr());
    st->Prepare(wx2std(stm, databaseM->getCharsetConverter()));
    if (have_dbkey)  // find the column and set the parameter
    {
        for (int c2 = 1; c2 <= statementM->Columns(); ++c2)
        {
            wxString cn(std2wxIdentifier(statementM->ColumnName(c2),
                databaseM->getCharsetConverter()));
            wxString tn(std2wxIdentifier(statementM->ColumnTable(c2),
                databaseM->getCharsetConverter()));
            if (cn == "DB_KEY" && tn == table)
            {
                DBKeyColumnDef *dbk =
                    dynamic_cast<DBKeyColumnDef *>(columnDefsM[c2-1]);
                if (!dbk)
                    throw FRError(_("Invalid Column"));
                if (buffer->isFieldNA(c2-1))
                    throw FRError(_("N/A value in DB_KEY column."));
                IBPP::DBKey dbkey;
                dbk->getDBKey(dbkey, buffer);
                // if updating BLOB, param = 2, else param = 1
                st->Set(st->Parameters(), dbkey);
            }
        }
    }
    return st;
}

bool DataGridRows::isBlobColumn(unsigned col, bool* pIsTextual)
{
    BlobColumnDef* bcd = dynamic_cast<BlobColumnDef *>(columnDefsM[col]);
    if ((pIsTextual) && (bcd))
        *pIsTextual = bcd->isTextual();
    return (0 != bcd);
}

IBPP::Blob* DataGridRows::getBlob(unsigned row, unsigned col, bool validateBlob)
{
    if (row >= buffersM.size())
      throw FRError(_("Invalid row index."));
    if (col >= columnDefsM.size())
      throw FRError(_("Invalid col index."));
    IBPP::Blob* b0 = buffersM[row]->getBlob(columnDefsM[col]->getIndex());
    if ((validateBlob) && (!b0))
        throw FRError(_("BLOB data not valid"));
    return b0;
}

// Creates a Blob (DataGridRowBlob-struct) to save data in DataGridRowsBlob.blob
// Finally the BLOB will be set with setBlob(...)
DataGridRowsBlob DataGridRows::setBlobPrepare(unsigned row, unsigned col)
{
    wxString tn(std2wxIdentifier(statementM->ColumnTable(col + 1),
        databaseM->getCharsetConverter()));
    wxString cn(std2wxIdentifier(statementM->ColumnName(col + 1),
        databaseM->getCharsetConverter()));

    Identifier iTn(tn, databaseM->getSqlDialect());
    Identifier iCn(cn, databaseM->getSqlDialect());

    wxString stm = "UPDATE " + iTn.getQuoted()
        + " SET " + iCn.getQuoted()
        + " = ? WHERE ";
    std::map<wxString, UniqueConstraint *>::iterator it =
        statementTablesM.find(tn);
    if (it == statementTablesM.end() || (*it).second == 0)
        throw FRError(_("Blob table not found."));

    DataGridRowsBlob b;
    b.row = row;
    b.col = col;
    b.st = addWhere((*it).second, stm, tn, buffersM[row]);
    b.blob = IBPP::BlobFactory(b.st->DatabasePtr(), b.st->TransactionPtr());
    return b;
}

void DataGridRows::setBlob(DataGridRowsBlob &b)
{
    if (b.blob != 0) // b.blob is 0 if the blob is null
    {   
        b.st->Set(1, b.blob);
        b.st->Execute();  // we execute before updating internal storage
    }
    
    buffersM[b.row]->setBlob(columnDefsM[b.col]->getIndex(), b.blob);
    buffersM[b.row]->setFieldNull(b.col, (b.blob == 0));
    buffersM[b.row]->setFieldNA(b.col, false);
    BlobColumnDef *bcd = dynamic_cast<BlobColumnDef *>(columnDefsM[b.col]);
    if (!bcd)
        throw FRError(_("Not a BLOB column."));
    bcd->reset(buffersM[b.row]);  // reset cached blob data
}

void DataGridRows::exportBlobFile(const wxString& filename, unsigned row,
    unsigned col, ProgressIndicator *pi)
{
    wxFFile fl(filename, "wb+");
    if (!fl.IsOpened())
        throw FRError(_("Cannot open destination file."));
    IBPP::Blob *b0 = getBlob(row,col,true);
    IBPP::Blob b = *b0;

    b->Open();
    if (pi)
    {
        int size;
        b->Info(&size, 0, 0);
        pi->initProgress(_("Saving..."), size);
    }
    while (!pi || !pi->isCanceled())
    {
        uint8_t buffer[32768];
        int size = b->Read((void*)buffer, 32767);
        if (size < 1)
            break;
        fl.Write(buffer, size);
        if (pi)
            pi->stepProgress(size);
    }
    fl.Close();
    b->Close();
}

void DataGridRows::importBlobFile(const wxString& filename, unsigned row,
    unsigned col, ProgressIndicator *pi)
{
    wxFFile fl(filename, "rb");
    if (!fl.IsOpened())
        throw FRError(_("Cannot open BLOB file."));
    if (pi)
        pi->initProgress(_("Loading..."), fl.Length()); // wxFileOffset

    DataGridRowsBlob b = setBlobPrepare(row,col);
    b.blob->Create();
    uint8_t buffer[32768];
    while (!fl.Eof())
    {
        size_t len = fl.Read(buffer, 32767);    // slow when not 32k
        if (len < 1 || (pi && pi->isCanceled()))
            break;
        b.blob->Write(buffer, len);
        if (pi)
            pi->stepProgress(len);
    }
    fl.Close();
    b.blob->Close();
    if (pi && pi->isCanceled())
        return;

    setBlob(b);
}

// returns the executed SQL statement
wxString DataGridRows::setFieldValue(unsigned row, unsigned col,
    const wxString& value, bool setNull)
{
    LocalSettings localSet;

    wxString localValue = value;
    double localDouble = 0;
    
    if (IBPP::isRationalNumber(statementM->ColumnType(col + 1)) && localValue.ToDouble(&localDouble) && (value.Contains(",") || value.Contains(".")))
    {
        if (localValue.ToDouble(&localDouble) && localValue.Contains(","))
        {
            localSet.setDataBaseLenguage();

            localValue = std::to_string(localDouble);
            if (localValue.Contains(",")) {
                localValue.Replace(",", ".", true);
            }
        }
    }
    

    if (columnDefsM[col]->isReadOnly())
        throw FRError(_("This column is not editable."));

    // user wants to store null
    bool newIsNull = (
        (!dynamic_cast<StringColumnDef*>(columnDefsM[col]) && localValue.IsEmpty())
        || (setNull && localValue == "[null]") );
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
            columnDefsM[col]->setFromString(buffersM[row], localValue);
            buffersM[row]->setFieldNull(col, false);
        }

        // run the UPDATE statement
        wxString tn(std2wxIdentifier(statementM->ColumnTable(col + 1),
            databaseM->getCharsetConverter()));
        wxString cn(std2wxIdentifier(statementM->ColumnName(col + 1),
            databaseM->getCharsetConverter()));

        Identifier iTn(tn, databaseM->getSqlDialect());
        Identifier iCn(cn, databaseM->getSqlDialect());

        wxString stm = "UPDATE " + iTn.getQuoted()
            + " SET " + iCn.getQuoted();
        if (newIsNull)
            stm += " = NULL WHERE ";
        else
        {
            if ((statementM->ColumnType(col + 1) == IBPP::SDT::sdString) && (statementM->ColumnSubtype(col + 1) == 1)) //OCTET
                stm += " = x'";
            else
                stm += " = '";
            wxString lval = columnDefsM[col]->getAsFirebirdString(buffersM[row]);
            if (IBPP::isRationalNumber(statementM->ColumnType(col + 1))) //Fix locale problem for "," as decimal separator
                lval.Replace(",", ".");
            stm += lval
                + "' WHERE ";
        }

        std::map<wxString, UniqueConstraint *>::iterator it =
            statementTablesM.find(tn);

        // MB: please do not remove this check. Although it is not needed,
        //     it helped me detect some subtle bugs much easier
        if (it == statementTablesM.end() || (*it).second == 0)
            throw FRError(_("This column should not be editable"));

        IBPP::Statement st = addWhere((*it).second, stm, tn, oldRecord);
        st->Execute();
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

