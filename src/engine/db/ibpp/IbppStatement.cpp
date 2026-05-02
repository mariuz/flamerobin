/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#include "engine/db/ibpp/IbppStatement.h"
#include "engine/db/ibpp/IbppBlob.h"
#include <stdexcept>
#include "core/FRInt128.h"
#include "core/FRDecimal.h"
#include "core/StringUtils.h"

namespace fr
{

IbppStatement::IbppStatement(IDatabasePtr db, ITransactionPtr tr, IBPP::Database ibppDb, IBPP::Transaction ibppTr)
    : databasePtrM(db), transactionPtrM(tr)
{
    statementM = IBPP::StatementFactory(ibppDb, ibppTr);
}

void IbppStatement::prepare(const std::string& sql)
{
    statementM->Prepare(sql);
}

std::string IbppStatement::getSql() const
{
    return statementM->Sql();
}

void IbppStatement::execute()
{
    statementM->Execute();
}

bool IbppStatement::fetch()
{
    return statementM->Fetch();
}

void IbppStatement::close()
{
    statementM->Close();
}

void IbppStatement::setNull(int index)
{
    statementM->SetNull(index + 1);
}

void IbppStatement::setString(int index, const std::string& value)
{
    statementM->Set(index + 1, value);
}

void IbppStatement::setInt32(int index, int32_t value)
{
    statementM->Set(index + 1, value);
}

void IbppStatement::setInt64(int index, int64_t value)
{
    statementM->Set(index + 1, value);
}

void IbppStatement::setDouble(int index, double value)
{
    statementM->Set(index + 1, value);
}

void IbppStatement::setBool(int index, bool value)
{
    statementM->Set(index + 1, value);
}

void IbppStatement::setDate(int index, int year, int month, int day)
{
    IBPP::Date d;
    d.SetDate(year, month, day);
    statementM->Set(index + 1, d);
}

void IbppStatement::setTime(int index, int hour, int minute, int second, int fraction)
{
    IBPP::Time t;
    t.SetTime(IBPP::Time::tmNone, hour, minute, second, fraction, IBPP::Time::TZ_NONE, nullptr);
    statementM->Set(index + 1, t);
}

void IbppStatement::setTimestamp(int index, int year, int month, int day,
    int hour, int minute, int second, int fraction)
{
    IBPP::Timestamp ts;
    ts.SetDate(year, month, day);
    ts.SetTime(IBPP::Time::tmNone, hour, minute, second, fraction, IBPP::Time::TZ_NONE, nullptr);
    statementM->Set(index + 1, ts);
}

void IbppStatement::setBytes(int index, const void* data, int size)
{
    if (size == 8)
    {
        IBPP::DBKey key;
        key.SetKey(data, size);
        statementM->Set(index + 1, key);
    }
    else
    {
        // For other sizes, we might need a different approach or just throw
        throw std::runtime_error("setBytes only supported for size 8 (DB_KEY) in IBPP for now");
    }
}

bool IbppStatement::isNull(int index)
{
    return statementM->IsNull(index + 1);
}

std::string IbppStatement::getString(int index)
{
    ColumnType type = getColumnType(index);
    if (type == ColumnType::Blob)
    {
        IBPP::Blob b = IBPP::BlobFactory(statementM->DatabasePtr(), statementM->TransactionPtr());
        statementM->Get(index + 1, b);
        try
        {
            b->Open();
        }
        catch (...)
        {
            return "";
        }
        std::string result;
        char buffer[8192];
        while (true)
        {
            int size = b->Read(buffer, 8192 - 1);
            if (size <= 0)
                break;
            buffer[size] = 0;
            result += buffer;
        }
        b->Close();
        return result;
    }
    if (type == ColumnType::Int128)
    {
        IBPP::ibpp_int128_t value;
        statementM->Get(index + 1, value);
        return wx2std(Int128ToString(value));
    }
    if (type == ColumnType::Decfloat16)
    {
        IBPP::ibpp_dec16_t value;
        statementM->Get(index + 1, value);
        return wx2std(Dec16DPDToString(value));
    }
    if (type == ColumnType::Decfloat34)
    {
        IBPP::ibpp_dec34_t value;
        statementM->Get(index + 1, value);
        return wx2std(Dec34DPDToString(value));
    }
    if (type == fr::ColumnType::Date)
        return getDate(index);
    if (type == fr::ColumnType::Time || type == fr::ColumnType::TimeTz)
        return getTime(index);
    if (type == fr::ColumnType::Timestamp || type == fr::ColumnType::TimestampTz)
        return getTimestamp(index);

    std::string value;
    try
    {
        statementM->Get(index + 1, value);
    }
    catch (...)
    {
        // If IBPP cannot convert to string, we do it manually for common types
        switch (type)
        {
            case ColumnType::Integer: return std::to_string(getInt32(index));
            case ColumnType::BigInt: return std::to_string(getInt64(index));
            case ColumnType::Double: return std::to_string(getDouble(index));
            case ColumnType::Boolean: return getBool(index) ? "true" : "false";
            default: break;
        }
    }
    return value;
}

int32_t IbppStatement::getInt32(int index)
{
    if (statementM->ColumnType(index + 1) == IBPP::sdSmallint)
    {
        int16_t value;
        statementM->Get(index + 1, value);
        return (int32_t)value;
    }
    int32_t value;
    statementM->Get(index + 1, value);
    return value;
}

int64_t IbppStatement::getInt64(int index)
{
    IBPP::SDT type = statementM->ColumnType(index + 1);
    if (type == IBPP::sdLargeint)
    {
        int64_t value;
        statementM->Get(index + 1, value);
        return value;
    }
    if (type == IBPP::sdInteger)
    {
        int32_t value;
        statementM->Get(index + 1, value);
        return (int64_t)value;
    }
    if (type == IBPP::sdSmallint)
    {
        int16_t value;
        statementM->Get(index + 1, value);
        return (int64_t)value;
    }
    int64_t value;
    statementM->Get(index + 1, value);
    return value;
}

double IbppStatement::getDouble(int index)
{
    if (statementM->ColumnType(index + 1) == IBPP::sdFloat)
    {
        float value;
        statementM->Get(index + 1, value);
        return (double)value;
    }
    double value;
    statementM->Get(index + 1, value);
    return value;
}

bool IbppStatement::getBool(int index)
{
    bool value;
    statementM->Get(index + 1, value);
    return value;
}

void IbppStatement::getBytes(int index, void* data, int size)
{
    if (size == 8)
    {
        IBPP::DBKey key;
        statementM->Get(index + 1, key);
        key.GetKey(data, size);
    }
    else
    {
        throw std::runtime_error("getBytes only supported for size 8 (DB_KEY) in IBPP for now");
    }
}

IBlobPtr IbppStatement::getBlob(int index)
{
    auto blob = std::make_shared<IbppBlob>(statementM->DatabasePtr(), statementM->TransactionPtr());
    statementM->Get(index + 1, blob->getIBPPBlob());
    return blob;
}

void IbppStatement::setBlob(int index, IBlobPtr blob)
{
    auto ibppBlob = std::dynamic_pointer_cast<IbppBlob>(blob);
    if (!ibppBlob)
        throw std::runtime_error("Invalid blob type for IBPP backend");
    statementM->Set(index + 1, ibppBlob->getIBPPBlob());
}

std::string IbppStatement::getDate(int index)
{
    IBPP::Date value;
    statementM->Get(index + 1, value);
    int y, m, d;
    value.GetDate(y, m, d);
    return wx2std(wxString::Format("%04d-%02d-%02d", y, m, d));
}

std::string IbppStatement::getTime(int index)
{
    IBPP::Time value;
    statementM->Get(index + 1, value);
    int h, m, s, t;
    value.GetTime(h, m, s, t);
    return wx2std(wxString::Format("%02d:%02d:%02d.%04d", h, m, s, t));
}

std::string IbppStatement::getTimestamp(int index)
{
    IBPP::Timestamp value;
    statementM->Get(index + 1, value);
    int y, mo, d, h, mi, s, t;
    value.GetDate(y, mo, d);
    value.GetTime(h, mi, s, t);
    return wx2std(wxString::Format("%04d-%02d-%02d %02d:%02d:%02d.%04d", y, mo, d, h, mi, s, t));
}

} // namespace fr

namespace ibpp_internals
{
    bool getTimezoneNameById(int tzId, std::string& name);
}

namespace fr
{

std::string IbppStatement::getTimeTz(int index)
{
    IBPP::Time t;
    statementM->Get(index + 1, t);
    int h, m, s, t10;
    t.GetTime(h, m, s, t10);

    std::string tzName;
    if (t.GetTimezone() != IBPP::Time::TZ_NONE)
        ibpp_internals::getTimezoneNameById(t.GetTimezone(), tzName);

    return wx2std(wxString::Format("%02d:%02d:%02d.%04d %s", h, m, s, t10, tzName.c_str()));
}

std::string IbppStatement::getTimestampTz(int index)
{
    IBPP::Timestamp ts;
    statementM->Get(index + 1, ts);
    int y, mo, d, h, mi, s, t;
    ts.GetDate(y, mo, d);
    ts.GetTime(h, mi, s, t);

    std::string tzName;
    if (ts.GetTimezone() != IBPP::Time::TZ_NONE)
        ibpp_internals::getTimezoneNameById(ts.GetTimezone(), tzName);

    return wx2std(wxString::Format("%04d-%02d-%02d %02d:%02d:%02d.%04d %s", y, mo, d, h, mi, s, t, tzName.c_str()));
}

void IbppStatement::getDate(int index, int& year, int& month, int& day)
{
    IBPP::Date d;
    statementM->Get(index + 1, d);
    year = d.Year();
    month = d.Month();
    day = d.Day();
}

void IbppStatement::getTime(int index, int& hour, int& minute, int& second, int& fraction)
{
    IBPP::Time t;
    statementM->Get(index + 1, t);
    hour = t.Hours();
    minute = t.Minutes();
    second = t.Seconds();
    fraction = t.SubSeconds();
}

void IbppStatement::getTimestamp(int index, int& year, int& month, int& day,
    int& hour, int& minute, int& second, int& fraction)
{
    IBPP::Timestamp ts;
    statementM->Get(index + 1, ts);
    year = ts.Year();
    month = ts.Month();
    day = ts.Day();
    hour = ts.Hours();
    minute = ts.Minutes();
    second = ts.Seconds();
    fraction = ts.SubSeconds();
}

int IbppStatement::getColumnCount()
{
    try
    {
        return statementM->Columns();
    }
    catch (const IBPP::Exception&)
    {
        return 0;
    }
}

std::string IbppStatement::getColumnName(int index)
{
    return statementM->ColumnName(index + 1);
}

ColumnType IbppStatement::getColumnType(int index)
{
    IBPP::SDT type = statementM->ColumnType(index + 1);
    switch (type)
    {
        case IBPP::sdString: return ColumnType::Varchar;
        case IBPP::sdLargeint: return ColumnType::BigInt;
        case IBPP::sdInteger: return ColumnType::Integer;
        case IBPP::sdSmallint: return ColumnType::Integer;
        case IBPP::sdFloat: return ColumnType::Float;
        case IBPP::sdDouble: return ColumnType::Double;
        case IBPP::sdDate: return ColumnType::Date;
        case IBPP::sdTime: return ColumnType::Time;
        case IBPP::sdTimestamp: return ColumnType::Timestamp;
        case IBPP::sdTimeTz: return ColumnType::TimeTz;
        case IBPP::sdTimestampTz: return ColumnType::TimestampTz;
        case IBPP::sdBlob: return ColumnType::Blob;
        case IBPP::sdBoolean: return ColumnType::Boolean;
        case IBPP::sdInt128: return ColumnType::Int128;
        case IBPP::sdDec16: return ColumnType::Decfloat16;
        case IBPP::sdDec34: return ColumnType::Decfloat34;
        default: return ColumnType::Unknown;
    }
}

int IbppStatement::getColumnSubtype(int index)
{
    return statementM->ColumnSubtype(index + 1);
}

int IbppStatement::getColumnScale(int index)
{
    return statementM->ColumnScale(index + 1);
}

int IbppStatement::getColumnSize(int index)
{
    return statementM->ColumnSize(index + 1);
}

std::string IbppStatement::getColumnAlias(int index)
{
    return statementM->ColumnAlias(index + 1);
}

std::string IbppStatement::getColumnTable(int index)
{
    return statementM->ColumnTable(index + 1);
}

std::string IbppStatement::getPlan()
{
    std::string plan;
    statementM->Plan(plan);
    return plan;
}

StatementType IbppStatement::getType()
{
    IBPP::STT type = statementM->Type();
    switch (type)
    {
        case IBPP::stSelect: return StatementType::Select;
        case IBPP::stInsert: return StatementType::Insert;
        case IBPP::stUpdate: return StatementType::Update;
        case IBPP::stDelete: return StatementType::Delete;
        case IBPP::stDDL: return StatementType::DDL;
        case IBPP::stExecProcedure: return StatementType::ExecProcedure;
        case IBPP::stSetGenerator: return StatementType::SetGenerator;
        default: return StatementType::Unknown;
    }
}

int IbppStatement::getParameterCount()
{
    try
    {
        return statementM->Parameters();
    }
    catch (const IBPP::Exception&)
    {
        return 0;
    }
}

std::string IbppStatement::getParameterName(int index)
{
    return statementM->ParametersByName().at(index);
}

std::vector<int> IbppStatement::findParameterIndicesByName(const std::string& name)
{
    std::vector<int> result = statementM->FindParamsByName(name);
    if (result.empty())
    {
        std::string upperName = name;
        for (auto& c : upperName) c = (char)std::toupper(c);
        result = statementM->FindParamsByName(upperName);
    }
    for (size_t i = 0; i < result.size(); ++i)
        result[i]--;
    return result;
}

ColumnType IbppStatement::getParameterType(int index)
{
    IBPP::SDT type = statementM->ParameterType(index + 1);
    switch (type)
    {
        case IBPP::sdString: return ColumnType::Varchar;
        case IBPP::sdLargeint: return ColumnType::BigInt;
        case IBPP::sdInteger: return ColumnType::Integer;
        case IBPP::sdSmallint: return ColumnType::Integer;
        case IBPP::sdFloat: return ColumnType::Float;
        case IBPP::sdDouble: return ColumnType::Double;
        case IBPP::sdDate: return ColumnType::Date;
        case IBPP::sdTime: return ColumnType::Time;
        case IBPP::sdTimestamp: return ColumnType::Timestamp;
        case IBPP::sdTimeTz: return ColumnType::TimeTz;
        case IBPP::sdTimestampTz: return ColumnType::TimestampTz;
        case IBPP::sdBlob: return ColumnType::Blob;
        case IBPP::sdBoolean: return ColumnType::Boolean;
        case IBPP::sdInt128: return ColumnType::Int128;
        case IBPP::sdDec16: return ColumnType::Decfloat16;
        case IBPP::sdDec34: return ColumnType::Decfloat34;
        default: return ColumnType::Unknown;
    }
}

int IbppStatement::getParameterSubtype(int index)
{
    return statementM->ParameterSubtype(index + 1);
}

int IbppStatement::getParameterScale(int index)
{
    return statementM->ParameterScale(index + 1);
}

int IbppStatement::getParameterSize(int index)
{
    return statementM->ParameterSize(index + 1);
}

int IbppStatement::getAffectedRows()
{
    return statementM->AffectedRows();
}

} // namespace fr
