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
#include "gui/controls/DataGridRows.h"
//-----------------------------------------------------------------------------
// DataGridRowBuffer class
class DataGridRowBuffer
{
private:
    std::vector<bool> nullFieldsM;
    std::vector<uint8_t> dataM;
    std::vector<wxString> stringsM;
public:
    DataGridRowBuffer(unsigned fieldCount);
    ~DataGridRowBuffer();

    wxString getString(unsigned index);
    bool getValue(unsigned offset, double& value);
    bool getValue(unsigned offset, float& value);
    bool getValue(unsigned offset, int& value);
    bool getValue(unsigned offset, int64_t& value);
    bool isFieldNull(unsigned num);
    void setFieldNull(unsigned num, bool isNull);
    void setString(unsigned num, const wxString& value);
    void setValue(unsigned offset, double value);
    void setValue(unsigned offset, float value);
    void setValue(unsigned offset, int value);
    void setValue(unsigned offset, int64_t value);
};
//-----------------------------------------------------------------------------
DataGridRowBuffer::DataGridRowBuffer(unsigned fieldCount)
{
    // initialize with field count, all fields initially NULL
    // there's no need to preallocate the uint8 buffer or string array
    nullFieldsM.resize(fieldCount, true);
}
//-----------------------------------------------------------------------------
DataGridRowBuffer::~DataGridRowBuffer()
{
}
//-----------------------------------------------------------------------------
wxString DataGridRowBuffer::getString(unsigned index)
{
    return stringsM[index];
}
//-----------------------------------------------------------------------------
bool DataGridRowBuffer::getValue(unsigned offset, double& value)
{
    if (offset + sizeof(double) > dataM.size())
        return false;
    value = *((double*)&dataM[offset]);
    return true;
}
//-----------------------------------------------------------------------------
bool DataGridRowBuffer::getValue(unsigned offset, float& value)
{
    if (offset + sizeof(float) > dataM.size())
        return false;
    value = *((float*)&dataM[offset]);
    return true;
}
//-----------------------------------------------------------------------------
bool DataGridRowBuffer::getValue(unsigned offset, int& value)
{
    if (offset + sizeof(int) > dataM.size())
        return false;
    value = *((int*)&dataM[offset]);
    return true;
}
//-----------------------------------------------------------------------------
bool DataGridRowBuffer::getValue(unsigned offset, int64_t& value)
{
    if (offset + sizeof(int64_t) > dataM.size())
        return false;
    value = *((int64_t*)&dataM[offset]);
    return true;
}
//-----------------------------------------------------------------------------
bool DataGridRowBuffer::isFieldNull(unsigned num)
{
    return (num < nullFieldsM.size() && nullFieldsM[num]);
}
//-----------------------------------------------------------------------------
void DataGridRowBuffer::setFieldNull(unsigned num, bool isNull)
{
    if (num < nullFieldsM.size())
        nullFieldsM[num] = isNull;
}
//-----------------------------------------------------------------------------
void DataGridRowBuffer::setString(unsigned num, const wxString& value)
{
    if (num >= stringsM.size())
        stringsM.resize(num + 1, wxEmptyString);
    stringsM[num] = value;
}
//-----------------------------------------------------------------------------
void DataGridRowBuffer::setValue(unsigned offset, double value)
{
    if (offset + sizeof(double) > dataM.size())
        dataM.resize(offset + sizeof(double), 0);
    *((double*)&dataM[offset]) = value;
}
//-----------------------------------------------------------------------------
void DataGridRowBuffer::setValue(unsigned offset, float value)
{
    if (offset + sizeof(float) > dataM.size())
        dataM.resize(offset + sizeof(float), 0);
    *((float*)&dataM[offset]) = value;
}
//-----------------------------------------------------------------------------
void DataGridRowBuffer::setValue(unsigned offset, int value)
{
    if (offset + sizeof(int) > dataM.size())
        dataM.resize(offset + sizeof(int), 0);
    *((int*)&dataM[offset]) = value;
}
//-----------------------------------------------------------------------------
void DataGridRowBuffer::setValue(unsigned offset, int64_t value)
{
    if (offset + sizeof(int64_t) > dataM.size())
        dataM.resize(offset + sizeof(int64_t), 0);
    *((int64_t*)&dataM[offset]) = value;
}
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
wxString GridCellFormats::formatTime(int hour, int minute, int second,
    int milliSecond)
{
    ensureLoaded();

    wxString result;
    for (wxString::iterator c = timeFormatM.begin(); c != timeFormatM.end(); c++)
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
// ResultsetColumnDef class
ResultsetColumnDef::ResultsetColumnDef(const wxString& name)
    : nameM(name)
{
}
//-----------------------------------------------------------------------------
ResultsetColumnDef::~ResultsetColumnDef()
{
}
//-----------------------------------------------------------------------------
wxString ResultsetColumnDef::getName()
{
    return nameM;
}
//-----------------------------------------------------------------------------
bool ResultsetColumnDef::isNumeric()
{
    return false;
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
    IntegerColumnDef(const wxString& name, unsigned offset);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
IntegerColumnDef::IntegerColumnDef(const wxString& name, unsigned offset)
    : ResultsetColumnDef(name), offsetM(offset)
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
    if (source.IsEmpty())
    {
        buffer->setFieldNull(offsetM/sizeof(int), true);
        return;
    }
    else
        buffer->setFieldNull(offsetM/sizeof(int), false);
    long value;
    if (!source.ToLong(&value))
        throw FRError(_("Invalid numeric value"));
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
    Int64ColumnDef(const wxString& name, unsigned offset);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
Int64ColumnDef::Int64ColumnDef(const wxString& name, unsigned offset)
    : ResultsetColumnDef(name), offsetM(offset)
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
    if (source.IsEmpty())
    {
        buffer->setFieldNull(offsetM/sizeof(int64_t), true);
        return;
    }
    else
        buffer->setFieldNull(offsetM/sizeof(int64_t), false);
    int64_t value;
    if (!source.ToLongLong(&value))
        throw FRError(_("Invalid numeric value"));
    buffer->setValue(offsetM, value);
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
// DateColumnDef class
class DateColumnDef : public ResultsetColumnDef
{
private:
    unsigned offsetM;
public:
    DateColumnDef(const wxString& name, unsigned offset);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
DateColumnDef::DateColumnDef(const wxString& name, unsigned offset)
    : ResultsetColumnDef(name), offsetM(offset)
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
void DateColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    //buffer->setValue(offsetM, value);
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
    TimeColumnDef(const wxString& name, unsigned offset);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
TimeColumnDef::TimeColumnDef(const wxString& name, unsigned offset)
    : ResultsetColumnDef(name), offsetM(offset)
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
void TimeColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    //buffer->setValue(offsetM, value);
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
    TimestampColumnDef(const wxString& name, unsigned offset);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
TimestampColumnDef::TimestampColumnDef(const wxString& name, unsigned offset)
    : ResultsetColumnDef(name), offsetM(offset)
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
void TimestampColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    //buffer->setValue(offsetM, value);
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
    FloatColumnDef(const wxString& name, unsigned offset);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
FloatColumnDef::FloatColumnDef(const wxString& name, unsigned offset)
    : ResultsetColumnDef(name), offsetM(offset)
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
    //buffer->setValue(offsetM, value);
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
public:
    DoubleColumnDef(const wxString& name, unsigned offset);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual bool isNumeric();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
DoubleColumnDef::DoubleColumnDef(const wxString& name, unsigned offset)
    : ResultsetColumnDef(name), offsetM(offset)
{
}
//-----------------------------------------------------------------------------
wxString DoubleColumnDef::getAsString(DataGridRowBuffer* buffer)
{
    wxASSERT(buffer);
    double value;
    if (!buffer->getValue(offsetM, value))
        return wxEmptyString;

    int scale;
    if (!buffer->getValue(offsetM + sizeof(double), scale))
        scale = 0;
    if (scale)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(scale) << value;
        return std2wx(oss.str());
    }
    return GridCellFormats::get().formatDouble(value);
}
//-----------------------------------------------------------------------------
void DoubleColumnDef::setFromString(DataGridRowBuffer* buffer,
        const wxString& source)
{
    wxASSERT(buffer);
    //buffer->setValue(offsetM, value);
}
//-----------------------------------------------------------------------------
unsigned DoubleColumnDef::getBufferSize()
{
    return sizeof(double) + sizeof(int);
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
    int scale = statement->ColumnScale(col);
    buffer->setValue(offsetM + sizeof(double), scale);
}
//-----------------------------------------------------------------------------
// StringColumnDef class
class StringColumnDef : public ResultsetColumnDef
{
private:
    unsigned indexM;
public:
    StringColumnDef(const wxString& name, unsigned stringIndex);
    virtual wxString getAsString(DataGridRowBuffer* buffer);
    virtual unsigned getBufferSize();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        const IBPP::Statement& statement, wxMBConv* converter);
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source);
};
//-----------------------------------------------------------------------------
StringColumnDef::StringColumnDef(const wxString& name, unsigned stringIndex)
    : ResultsetColumnDef(name), indexM(stringIndex)
{
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
    buffer->setString(indexM, std2wx(value, converter));
}
//-----------------------------------------------------------------------------
// DataGridRows class
DataGridRows::DataGridRows()
    : bufferSizeM(0)
{
}
//-----------------------------------------------------------------------------
DataGridRows::~DataGridRows()
{
    clear();
}
//-----------------------------------------------------------------------------
bool DataGridRows::addRow(const IBPP::Statement& statement,
    wxMBConv* converter)
{
    if (buffersM.size() == buffersM.capacity())
        buffersM.reserve(buffersM.capacity() + 1024);
    DataGridRowBuffer* buffer = new DataGridRowBuffer(columnDefsM.size());
    buffersM.push_back(buffer);

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
    bufferSizeM = 0;
}
//-----------------------------------------------------------------------------
bool DataGridRows::removeRows(size_t from, size_t count)
{
    std::vector<DataGridRowBuffer*>::iterator i2, it = buffersM.begin();
    from += count - 1;
    while (from--)
        if (++it == buffersM.end())
            return false;
    while (count--)
    {
        i2 = it;
        it--;
        delete (*i2);
        buffersM.erase(i2);
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
bool DataGridRows::initialize(const IBPP::Statement& statement)
{
    clear();
    // column definitions may have an index into the string array,
    // an offset into the buffer, or use no data at all
    unsigned colCount = statement->Columns();
    columnDefsM.reserve(colCount);
    bufferSizeM = 0;
    unsigned stringIndex = 0;

    // create column definitions and compute the necessary buffer size
    // and string array length when all fields contain data
    for (unsigned col = 1; col <= colCount; ++col)
    {
        wxString colName(std2wx(statement->ColumnAlias(col)));
        if (colName.empty())
            colName = std2wx(statement->ColumnName(col));

        IBPP::SDT type = statement->ColumnType(col);
        if (statement->ColumnScale(col))
            type = IBPP::sdDouble;

        ResultsetColumnDef* columnDef = 0;
        switch (type)
        {
            case IBPP::sdDate:
                columnDef = new DateColumnDef(colName, bufferSizeM);
                break;
            case IBPP::sdTime:
                columnDef = new TimeColumnDef(colName, bufferSizeM);
                break;
            case IBPP::sdTimestamp:
                columnDef = new TimestampColumnDef(colName, bufferSizeM);
                break;

            case IBPP::sdSmallint:
            case IBPP::sdInteger:
                columnDef = new IntegerColumnDef(colName, bufferSizeM);
                break;
            case IBPP::sdLargeint:
                columnDef = new Int64ColumnDef(colName, bufferSizeM);
                break;

            case IBPP::sdFloat:
                columnDef = new FloatColumnDef(colName, bufferSizeM);
                break;
            case IBPP::sdDouble:
                columnDef = new DoubleColumnDef(colName, bufferSizeM);
                break;

            case IBPP::sdString:
                columnDef = new StringColumnDef(colName, stringIndex);
                ++stringIndex;
                break;
            default:
                // IBPP::sdArray and IBPP::sdBlob not really handled ATM
                columnDef = new DummyColumnDef(colName);
                break;
        }
        wxASSERT(columnDef);
        bufferSizeM += columnDef->getBufferSize();
        columnDefsM.push_back(columnDef);
    }
    return true;
}
//-----------------------------------------------------------------------------
bool DataGridRows::isRowFieldNumeric(unsigned col)
{
    if (col >= columnDefsM.size())
        return false;
    return columnDefsM[col]->isNumeric();
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
void DataGridRows::setFieldValue(unsigned row, unsigned col,
    const wxString& value)
{
    columnDefsM[col]->setFromString(buffersM[row], value);
}
//-----------------------------------------------------------------------------
