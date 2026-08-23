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

#ifndef DATAGRIDROWS_H
#define DATAGRIDROWS_H

#include <vector>
#include <map>
#include <list>

#include "engine/db/IStatement.h"

#include <chrono>
#include <ctime>
#include <cstring>
#include <string>
#include <memory>

namespace IBPP {
    typedef int Timezone;
    const Timezone TZ_NONE = 0;

    struct ibpp_int128_t {
        uint64_t low;
        int64_t high;
    };

    enum SDT {
        sdArray,
        sdBlob,
        sdDate,
        sdTime,
        sdTimestamp,
        sdString,
        sdSmallint,
        sdInteger,
        sdLargeint,
        sdFloat,
        sdDouble,
        sdBoolean,
        sdDec16,
        sdDec34,
        sdInt128
    };

    class Date {
    private:
        int yearM, monthM, dayM;
        static int encode(int y, int m, int d)
        {
            if (y == 0 && m == 0 && d == 0)
                return 0;
            if (y >= 0)
                return y * 10000 + m * 100 + d;
            else
                return y * 10000 - (m * 100 + d);
        }
        static void decode(int val, int& y, int& m, int& d)
        {
            if (val == 0)
            {
                y = 0; m = 0; d = 0;
                return;
            }
            if (val >= 0)
            {
                y = val / 10000;
                int rem = val % 10000;
                m = rem / 100;
                d = rem % 100;
            }
            else
            {
                y = -((-val) / 10000);
                int rem = (-val) % 10000;
                m = rem / 100;
                d = rem % 100;
            }
        }
    public:
        Date() : yearM(0), monthM(0), dayM(0) {}
        Date(int val) { decode(val, yearM, monthM, dayM); }
        Date(int y, int m, int d) : yearM(y), monthM(m), dayM(d) {}
        operator int() const { return encode(yearM, monthM, dayM); }
        int GetDate() const { return encode(yearM, monthM, dayM); }
        void GetDate(int& y, int& m, int& d) const { y = yearM; m = monthM; d = dayM; }
        void SetDate(int y, int m, int d_val) { yearM = y; monthM = m; dayM = d_val; }
        void SetDate(int val) { decode(val, yearM, monthM, dayM); }
        void Today()
        {
            time_t t = ::time(nullptr);
            tm* lt = ::localtime(&t);
            yearM = lt->tm_year + 1900;
            monthM = lt->tm_mon + 1;
            dayM = lt->tm_mday;
        }
        void Add(int days)
        {
            if (yearM == 0 && monthM == 0 && dayM == 0)
                Today();
            std::chrono::year_month_day ymd{std::chrono::year{yearM}, std::chrono::month{(unsigned)monthM}, std::chrono::day{(unsigned)dayM}};
            if (ymd.ok())
            {
                auto sd = std::chrono::sys_days{ymd} + std::chrono::days{days};
                std::chrono::year_month_day res{sd};
                yearM = (int)res.year();
                monthM = (unsigned)res.month();
                dayM = (unsigned)res.day();
            }
            else
            {
                time_t t = ::time(nullptr) + days * 24 * 3600;
                tm* lt = ::localtime(&t);
                yearM = lt->tm_year + 1900;
                monthM = lt->tm_mon + 1;
                dayM = lt->tm_mday;
            }
        }
        int Year() const { return yearM; }
        int Month() const { return monthM; }
        int Day() const { return dayM; }
    };

    class Time {
    private:
        int hr, mn, sc, ms;
        int tz;
        bool fallback;
        static int encode(int h, int m, int s, int f)
        {
            return (h * 3600 + m * 60 + s) * 10000 + f;
        }
        static void decode(int val, int& h, int& m, int& s, int& f)
        {
            if (val < 0) val = 0;
            f = val % 10000;
            int totalSec = val / 10000;
            s = totalSec % 60;
            int totalMin = totalSec / 60;
            m = totalMin % 60;
            h = totalMin / 60;
        }
    public:
        enum TimezoneMode {
            tmNone = 0,
            tmTimezone = 1,
            tmTimezoneGmtFallback = 2
        };
        enum { TZ_NONE = 0 };
        Time() : hr(0), mn(0), sc(0), ms(0), tz(0), fallback(false) {}
        Time(int val) : tz(0), fallback(false) { decode(val, hr, mn, sc, ms); }
        Time(int h, int m_val, int s, int f = 0, Timezone z = TZ_NONE) : hr(h), mn(m_val), sc(s), ms(f), tz(z), fallback(false) {}
        operator int() const { return encode(hr, mn, sc, ms); }
        int GetTime() const { return encode(hr, mn, sc, ms); }
        void GetTime(int& h, int& m, int& s, int& f) const { h = hr; m = mn; s = sc; f = ms; }
        void SetTime(TimezoneMode m, int h, int m_val, int s, int f, Timezone z, const void*) {
            hr = h; mn = m_val; sc = s; ms = f; tz = z;
            fallback = (m == tmTimezoneGmtFallback);
        }
        void SetTime(TimezoneMode m, int time, int timezone) {
            decode(time, hr, mn, sc, ms);
            tz = timezone;
            fallback = (m == tmTimezoneGmtFallback);
        }
        Timezone GetTimezone() const { return tz; }
        bool IsTimeZoneGmtFallback() const { return fallback; }
        void Now()
        {
            time_t t = ::time(nullptr);
            tm* lt = ::localtime(&t);
            hr = lt->tm_hour;
            mn = lt->tm_min;
            sc = lt->tm_sec;
            ms = 0;
        }
    };

    class Timestamp {
    private:
        Date d;
        Time t;
    public:
        Timestamp() {}
        Timestamp(int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int tenththousands = 0, Timezone tz = TZ_NONE)
            : d(year, month, day), t(hour, minute, second, tenththousands, tz) {}
        Date GetDate() const { return d; }
        Time GetTime() const { return t; }
        void GetDate(int& year, int& month, int& day) const { d.GetDate(year, month, day); }
        void GetTime(int& hour, int& minute, int& second, int& tenththousands) const { t.GetTime(hour, minute, second, tenththousands); }
        void SetDate(const Date& val) { d = val; }
        void SetDate(int y, int m, int d_val) { d.SetDate(y, m, d_val); }
        void SetDate(int val) { d.SetDate(val); }
        void SetTime(Time::TimezoneMode m, int hour, int minute, int second, int tenththousands, Timezone tz, const void* tzName) {
            t.SetTime(m, hour, minute, second, tenththousands, tz, tzName);
        }
        void SetTime(Time::TimezoneMode m, int time, int timezone) {
            t.SetTime(m, time, timezone);
        }
        operator Time&() { return t; }
        operator const Time&() const { return t; }
        Timezone GetTimezone() const { return t.GetTimezone(); }
        bool IsTimeZoneGmtFallback() const { return t.IsTimeZoneGmtFallback(); }
        void Today()
        {
            d.Today();
            t.SetTime(Time::tmNone, 0, 0, 0, 0, 0, nullptr);
        }
        void Now()
        {
            d.Today();
            t.Now();
        }
        void Add(int days)
        {
            d.Add(days);
        }
        int Year() const { return d.Year(); }
        int Month() const { return d.Month(); }
        int Day() const { return d.Day(); }
    };

    class DBKey {
    private:
        char data[8];
    public:
        DBKey() { std::memset(data, 0, 8); }
        void GetKey(void* buf, int size) const { std::memcpy(buf, data, size > 8 ? 8 : size); }
        void SetKey(const void* buf, int size) { std::memcpy(data, buf, size > 8 ? 8 : size); }
    };

    class BlobImpl {
    public:
        void open() {}
        void close() {}
        void create() {}
        int getLength() { return 0; }
        int read(void*, int) { return 0; }
        void write(const void*, int) {}
    };
    typedef std::shared_ptr<BlobImpl> Blob;

    class StatementImpl {
    public:
        void Get(int, int&) const {}
        void Get(int, double&) const {}
        void Get(int, bool&) const {}
        void Get(int, std::string&) const {}
        void Get(int, Time&) const {}
        void Get(int, Date&) const {}
        void Get(int, Timestamp&) const {}
        void Get(int, DBKey&) const {}
        void Get(int, ibpp_int128_t&) const {}
        void Get(int, Blob&) const {}
        template<typename T>
        void Get(int, T&) const {}
        bool IsNull(int) const { return false; }
        void* DatabasePtr() const { return nullptr; }
        void* TransactionPtr() const { return nullptr; }
        void Execute() {}
        void Prepare(const std::string&) {}
        bool Fetch() { return false; }
        int Columns() const { return 0; }
        SDT ColumnType(int) const { return sdString; }
        int ColumnSubtype(int) const { return 0; }
        int ColumnSize(int) const { return 0; }
        std::string ColumnTable(int) const { return ""; }
        std::string ColumnName(int) const { return ""; }
        std::map<int, wxString> ParametersByName() const { return std::map<int, wxString>(); }
    };
    typedef std::shared_ptr<StatementImpl> Statement;

    class Exception : public std::exception {
    public:
        const char* what() const noexcept override { return "IBPP Exception"; }
    };
}

#include "metadata/constraints.h"
#include "config/Config.h"

class Database;
class DataGridRowBuffer;
class ProgressIndicator;
class wxMBConv;

// GridCellFormats: class to cache config data for cell formatting
class GridCellFormats: public ConfigCache
{
private:
    enum ShowTimezoneInfoType
	{
        // ** Keep in sync with radiogroup control **
        // append no timezone info
        tzNone = 0,
        // append timezone raw value (id)
        tzRawId   = 1,
        // append timezone name
        tzName = 2
	};

private:
    int floatingPointPrecisionM;
    wxString dateFormatM;
    int maxBlobKBytesM;
    bool showBinaryBlobContentM;
    bool showBlobContentM;
    wxString timeFormatM;
    wxString timestampFormatM;
    ShowTimezoneInfoType showTimezoneInfoM;
    void formatAppendTz(wxString &s, IBPP::Time &t, bool hasTz,
        Database* db);
protected:
    virtual void loadFromConfig();
public:
    GridCellFormats();

    static GridCellFormats& get();

    template<typename T>
    wxString format(T value);
    wxString formatDate(int year, int month, int day);
    wxString formatTime(IBPP::Time &t, bool hasTz, Database* db);
    wxString formatTimestamp(IBPP::Timestamp &ts, bool hasTz, Database* db);

    int maxBlobBytesToFetch();
    bool parseDate(wxString::iterator& start, wxString::iterator end,
        bool consumeAll, int& year, int& month, int& day);
    bool parseTime(wxString::iterator& start, wxString::iterator end,
        int& hr, int& mn, int& sc, int& ml);
    bool parseTimestamp(wxString::iterator& start, wxString::iterator end,
        int& year, int& month, int& day, int& hr, int& mn, int& sc, int& ml);
    bool showBinaryBlobContent();
    bool showBlobContent();
};

class ResultsetColumnDef
{
private:
    wxString nameM;
protected:
    bool readOnlyM;
    bool nullableM;
public:
    ResultsetColumnDef(const wxString& name, bool readOnly = true,
        bool nullable = false);
    virtual ~ResultsetColumnDef();

    virtual wxString getAsFirebirdString(DataGridRowBuffer* buffer);
    virtual wxString getAsString(DataGridRowBuffer* buffer, Database* db) = 0;
    virtual void setFromString(DataGridRowBuffer* buffer,
        const wxString& source) = 0;
    virtual unsigned getBufferSize() = 0;
    wxString getName();
    virtual unsigned getIndex(); // for strings and blobs
    virtual bool isNumeric();
    bool isReadOnly();
    bool isNullable();
    virtual void setValue(DataGridRowBuffer* buffer, unsigned col,
        fr::IStatementPtr statement, wxMBConv* converter, Database* db) = 0;
};

struct DataGridFieldInfo
{
    bool rowInserted;
    bool rowDeleted;
    bool fieldReadOnly;
    bool fieldModified;
    bool fieldNull;
    bool fieldNA;
    bool fieldNumeric;
    bool fieldBlob;
};
struct DataGridRowsBlob
{
    fr::IBlobPtr blob;
    fr::IStatementPtr stDAL;
    unsigned row;
    unsigned col;
};

class DataGridRows
{
private:
    Database* databaseM;
    const bool readOnlyM;
    fr::IStatementPtr statementDALM;
    std::vector<ResultsetColumnDef*> columnDefsM;
    std::vector<DataGridRowBuffer*> buffersM;
    std::map<wxString, UniqueConstraint *> statementTablesM;
    std::map<wxString, UniqueConstraint *>::iterator deleteFromM;
    std::list<UniqueConstraint> dbKeysM;
    unsigned bufferSizeM;

    void getColumnInfo(Database* db, unsigned col, bool& readOnly,
        bool& nullable);
    fr::IStatementPtr addWhereDAL(UniqueConstraint* uq, wxString& stm,
        const wxString& tableName, DataGridRowBuffer* buffer);

public:
    DataGridRows(Database* db);
    ~DataGridRows();

    void addRow(fr::IStatementPtr statement);
    DataGridRowBuffer* fetchRowBuffer(fr::IStatementPtr statement);
    void addRows(const std::vector<DataGridRowBuffer*>& rowBuffers);
    void clear();
    unsigned getRowCount();
    unsigned getRowFieldCount();
    wxString getRowFieldName(unsigned col);
    bool initialize(fr::IStatementPtr statement);

    bool isColumnNullable(unsigned col);
    bool isColumnNumeric(unsigned col);
    bool isColumnReadonly(unsigned col);
    bool isBlobColumn(unsigned col, bool* pIsTextual = 0);
    bool getFieldInfo(unsigned row, unsigned col, DataGridFieldInfo& info);
    bool isFieldReadonly(unsigned row, unsigned col);
    bool isFieldNull(unsigned row, unsigned col);
    bool isFieldNA(unsigned row, unsigned col);

    wxString getFieldValue(unsigned row, unsigned col);
    wxString getFieldFirebirdValue(unsigned row, unsigned col);
    wxString setFieldValue(unsigned row, unsigned col,
        const wxString& value, bool setNull = false);
    void importBlobFile(const wxString& filename, unsigned row, unsigned col,
        ProgressIndicator *pi);
    void exportBlobFile(const wxString& filename, unsigned row, unsigned col,
        ProgressIndicator *pi);
    bool canRemoveRow(size_t row);
    bool removeRows(size_t from, size_t count, wxString& statement);

    ResultsetColumnDef* getColumnDef(unsigned col);
    void addRow(DataGridRowBuffer* buffer);

    // BLOB-Stuff
    fr::IBlobPtr getBlob(unsigned row, unsigned col, bool validateBlob);
    DataGridRowsBlob setBlobPrepare(unsigned row, unsigned col);
    void setBlob(DataGridRowsBlob &b);
};

#endif
