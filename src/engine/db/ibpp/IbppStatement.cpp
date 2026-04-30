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
#include <stdexcept>

namespace fr
{

IbppStatement::IbppStatement(IBPP::Database db, IBPP::Transaction tr)
{
    statementM = IBPP::StatementFactory(db, tr);
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

bool IbppStatement::isNull(int index)
{
    return statementM->IsNull(index + 1);
}

std::string IbppStatement::getString(int index)
{
    if (getColumnType(index) == ColumnType::Blob)
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
    std::string value;
    statementM->Get(index + 1, value);
    return value;
}

int32_t IbppStatement::getInt32(int index)
{
    int32_t value;
    statementM->Get(index + 1, value);
    return value;
}

int64_t IbppStatement::getInt64(int index)
{
    int64_t value;
    statementM->Get(index + 1, value);
    return value;
}

double IbppStatement::getDouble(int index)
{
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

int IbppStatement::getColumnCount()
{
    return statementM->Columns();
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
        case IBPP::sdBlob: return ColumnType::Blob;
        case IBPP::sdBoolean: return ColumnType::Boolean;
        default: return ColumnType::Unknown;
    }
}

} // namespace fr
