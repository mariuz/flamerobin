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

#include "engine/db/fbcpp/FbCppStatement.h"
#include "engine/db/fbcpp/FbCppBlob.h"
#include "engine/db/fbcpp/FbCppDatabase.h"
#include "engine/db/fbcpp/FbCppTransaction.h"
#include "engine/db/IDatabase.h"
#include "engine/db/ITransaction.h"
#include "core/FRInt128.h"
#include <fb-cpp/Exception.h>
#include <fb-cpp/Statement.h>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <array>
#include <cctype>
#include <firebird/Interface.h>
#include <wx/log.h>

namespace fr
{

FbCppStatement::FbCppStatement(std::shared_ptr<FbCppDatabase> db, std::shared_ptr<FbCppTransaction> tr)
    : databasePtrM(std::move(db)), transactionPtrM(std::move(tr)), eofReachedM(false), rowAvailableM(false)
{
    if (!databasePtrM || !transactionPtrM)
        throw std::runtime_error("Invalid database or transaction for FbCppStatement");
    attachmentSnapshotM = transactionPtrM->getAttachmentSnapshot();
    transactionSnapshotM = transactionPtrM->getFbCppTransactionPtr();
    txGenerationM = transactionPtrM->getGeneration();
}

FbCppStatement::~FbCppStatement()
{
    try
    {
        if (databasePtrM)
        {
            std::lock_guard<std::recursive_mutex> lock(databasePtrM->getAttachmentMutex());
            close();
            statementM.reset();
        }
        else
        {
            close();
            statementM.reset();
        }
    }
    catch (...)
    {
        // Never allow destructor to throw
    }
}

IDatabasePtr FbCppStatement::getDatabase()
{
    return databasePtrM;
}

ITransactionPtr FbCppStatement::getTransaction()
{
    return transactionPtrM;
}

void FbCppStatement::prepare(const std::string& sql)
{
    if (!databasePtrM || !transactionPtrM)
        throw std::runtime_error("Invalid database or transaction");
    std::lock_guard<std::recursive_mutex> lock(databasePtrM->getAttachmentMutex());

    attachmentSnapshotM = transactionPtrM->getAttachmentSnapshot();
    transactionSnapshotM = transactionPtrM->getFbCppTransactionPtr();
    txGenerationM = transactionPtrM->getGeneration();
    if (!attachmentSnapshotM || !attachmentSnapshotM->isValid()
        || !transactionSnapshotM || !transactionPtrM->isActive())
    {
        throw std::runtime_error("Transaction or connection is not active");
    }

    sqlM = sql;
    wxLogDebug("FbCppStatement::prepare() for SQL: %s", sqlM.c_str());
    try {
        fbcpp::StatementOptions options;
        if (databasePtrM)
        {
            options.setDialect(static_cast<unsigned>(databasePtrM->getDialect()));
        }
        std::string processedSql = preprocessSql(sql);
        statementM.emplace(*attachmentSnapshotM, *transactionSnapshotM, processedSql, options);

        columnTypesM.clear();
        const auto& outDescs = statementM->getOutputDescriptors();
        columnTypesM.reserve(outDescs.size());
        for (const auto& desc : outDescs)
        {
            ColumnType type = ColumnType::Unknown;
            switch (desc.adjustedType)
            {
                case fbcpp::DescriptorAdjustedType::STRING: type = ColumnType::Varchar; break;
                case fbcpp::DescriptorAdjustedType::INT32:
                case fbcpp::DescriptorAdjustedType::INT16:
                    type = desc.scale != 0 ? ColumnType::Numeric : ColumnType::Integer;
                    break;
                case fbcpp::DescriptorAdjustedType::INT64:
                    type = desc.scale != 0 ? ColumnType::Numeric : ColumnType::BigInt;
                    break;
                case fbcpp::DescriptorAdjustedType::FLOAT: type = ColumnType::Float; break;
                case fbcpp::DescriptorAdjustedType::DOUBLE: type = ColumnType::Double; break;
                case fbcpp::DescriptorAdjustedType::TIME: type = ColumnType::Time; break;
                case fbcpp::DescriptorAdjustedType::DATE: type = ColumnType::Date; break;
                case fbcpp::DescriptorAdjustedType::TIMESTAMP: type = ColumnType::Timestamp; break;
                case fbcpp::DescriptorAdjustedType::TIME_TZ: type = ColumnType::TimeTz; break;
                case fbcpp::DescriptorAdjustedType::TIMESTAMP_TZ: type = ColumnType::TimestampTz; break;
                case fbcpp::DescriptorAdjustedType::BLOB: type = ColumnType::Blob; break;
                case fbcpp::DescriptorAdjustedType::BOOLEAN: type = ColumnType::Boolean; break;
                case fbcpp::DescriptorAdjustedType::INT128: type = ColumnType::Int128; break;
                case fbcpp::DescriptorAdjustedType::DECFLOAT16: type = ColumnType::Decfloat16; break;
                case fbcpp::DescriptorAdjustedType::DECFLOAT34: type = ColumnType::Decfloat34; break;
                default: type = ColumnType::Unknown; break;
            }
            columnTypesM.push_back(type);
        }

        parameterTypesM.clear();
        const auto& inDescs = statementM->getInputDescriptors();
        parameterTypesM.reserve(inDescs.size());
        for (const auto& desc : inDescs)
        {
            ColumnType type = ColumnType::Unknown;
            switch (desc.adjustedType)
            {
                case fbcpp::DescriptorAdjustedType::STRING: type = ColumnType::Varchar; break;
                case fbcpp::DescriptorAdjustedType::INT32:
                case fbcpp::DescriptorAdjustedType::INT16:
                    type = desc.scale != 0 ? ColumnType::Numeric : ColumnType::Integer;
                    break;
                case fbcpp::DescriptorAdjustedType::INT64:
                    type = desc.scale != 0 ? ColumnType::Numeric : ColumnType::BigInt;
                    break;
                case fbcpp::DescriptorAdjustedType::FLOAT: type = ColumnType::Float; break;
                case fbcpp::DescriptorAdjustedType::DOUBLE: type = ColumnType::Double; break;
                case fbcpp::DescriptorAdjustedType::TIME: type = ColumnType::Time; break;
                case fbcpp::DescriptorAdjustedType::DATE: type = ColumnType::Date; break;
                case fbcpp::DescriptorAdjustedType::TIMESTAMP: type = ColumnType::Timestamp; break;
                case fbcpp::DescriptorAdjustedType::TIME_TZ: type = ColumnType::TimeTz; break;
                case fbcpp::DescriptorAdjustedType::TIMESTAMP_TZ: type = ColumnType::TimestampTz; break;
                case fbcpp::DescriptorAdjustedType::BLOB: type = ColumnType::Blob; break;
                case fbcpp::DescriptorAdjustedType::BOOLEAN: type = ColumnType::Boolean; break;
                case fbcpp::DescriptorAdjustedType::INT128: type = ColumnType::Int128; break;
                case fbcpp::DescriptorAdjustedType::DECFLOAT16: type = ColumnType::Decfloat16; break;
                case fbcpp::DescriptorAdjustedType::DECFLOAT34: type = ColumnType::Decfloat34; break;
                default: type = ColumnType::Unknown; break;
            }
            parameterTypesM.push_back(type);
        }
    } catch (const std::exception&) {
        throw;
    }
    firstRowFetchedM.reset();
    eofReachedM = false;
    rowAvailableM = false;
}

std::string FbCppStatement::getSql() const
{
    return sqlM;
}

void FbCppStatement::execute()
{
    if (!statementM || !databasePtrM)
        throw std::runtime_error("Statement not prepared");
    std::lock_guard<std::recursive_mutex> lock(databasePtrM->getAttachmentMutex());

    if (!transactionPtrM || !transactionPtrM->isValidForGeneration(txGenerationM)
        || !transactionSnapshotM)
    {
        throw std::runtime_error("Transaction not active");
    }

    wxLogDebug("FbCppStatement::execute() for SQL: %s", sqlM.c_str());

    close();

    firstRowFetchedM.reset();
    eofReachedM = false;
    rowAvailableM = false;

    try {
        wxLogDebug("FbCppStatement::execute() calling library execute().");
        bool hasRow = statementM->execute(*transactionSnapshotM);
        
        // If the statement has output columns, fb-cpp might already have the first row.
        // We must capture this result so that the first fetch() call returns it.
        if (getColumnCount() > 0)
        {
            firstRowFetchedM = hasRow;
            rowAvailableM = hasRow;
            if (getType() == StatementType::ExecProcedure)
                eofReachedM = true;
            else
                eofReachedM = !hasRow;
            wxLogDebug("FbCppStatement::execute() - statement with output. hasRow: %d", (int)hasRow);
        }
    } catch (const std::exception& e) {
        wxLogDebug("FbCppStatement::execute() failed: %s", e.what());
        throw;
    }
}

bool FbCppStatement::fetch()
{
    if (!statementM || !databasePtrM)
        return false;
    std::lock_guard<std::recursive_mutex> lock(databasePtrM->getAttachmentMutex());

    if (!transactionPtrM || !transactionPtrM->isValidForGeneration(txGenerationM))
        return false;

    if (firstRowFetchedM.has_value())
    {
        bool res = *firstRowFetchedM;
        firstRowFetchedM.reset();
        rowAvailableM = res;
        return res;
    }

    if (eofReachedM)
    {
        rowAvailableM = false;
        return false;
    }

    try {
        bool res = statementM->fetchNext();
        rowAvailableM = res;
        if (!res)
            eofReachedM = true;
        return res;
    } catch (const std::exception& e) {
        wxLogDebug("FbCppStatement::fetch() failed: %s", e.what());
        eofReachedM = true;
        rowAvailableM = false;
        throw;
    }
}

void FbCppStatement::close()
{
    try
    {
        if (statementM && databasePtrM)
        {
            std::lock_guard<std::recursive_mutex> lock(databasePtrM->getAttachmentMutex());
            if (transactionPtrM && transactionPtrM->isValidForGeneration(txGenerationM))
                statementM->closeCursor();
        }
    }
    catch (...)
    {
    }
    firstRowFetchedM.reset();
    eofReachedM = false;
    rowAvailableM = false;
}

void FbCppStatement::setNull(int index)
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    if (index < 0 || (unsigned)index >= statementM->getInputDescriptors().size())
        return;
    statementM->setNull((unsigned)index);
}

void FbCppStatement::setString(int index, const std::string& value)
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    if (index < 0 || (unsigned)index >= statementM->getInputDescriptors().size())
    {
        wxLogDebug("FbCppStatement::setString(%d) - index out of range", index);
        return;
    }

    wxLogDebug("FbCppStatement::setString(%d, '%s') [len=%d]", index, value.c_str(), (int)value.length());

    ColumnType type = getParameterType(index);
    if (type == ColumnType::Blob)
    {
        wxLogDebug("FbCppStatement::setString(%d) - creating blob from string", index);
        IBlobPtr b = std::make_shared<FbCppBlob>(databasePtrM, transactionPtrM);
        b->create();
        b->write(value.c_str(), (int)value.size());
        b->close();
        setBlob(index, b);
        return;
    }

    statementM->set((unsigned)index, std::string_view(value));
}

void FbCppStatement::setInt32(int index, int32_t value)
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");

    if (index < 0 || (unsigned)index >= statementM->getInputDescriptors().size())
        return;

    ColumnType type = getParameterType(index);
    if (type == ColumnType::Boolean)
        statementM->setBool((unsigned)index, value != 0);
    else
        statementM->setInt32((unsigned)index, value);
}

void FbCppStatement::setInt64(int index, int64_t value)
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");

    if (index < 0 || (unsigned)index >= statementM->getInputDescriptors().size())
        return;

    ColumnType type = getParameterType(index);
    if (type == ColumnType::Boolean)
        statementM->setBool((unsigned)index, value != 0);
    else
        statementM->setInt64((unsigned)index, value);
}

void FbCppStatement::setDouble(int index, double value)
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    if (index < 0 || (unsigned)index >= statementM->getInputDescriptors().size())
        return;
    statementM->setDouble((unsigned)index, value);
}

void FbCppStatement::setBool(int index, bool value)
{
    if (!statementM)
        throw std::runtime_error("No statement available");

    if (index < 0 || (unsigned)index >= statementM->getInputDescriptors().size())
        return;

    ColumnType type = getParameterType(index);
    if (type == ColumnType::Boolean)
        statementM->setBool((unsigned)index, value);
    else if (type == ColumnType::Integer || type == ColumnType::BigInt || type == ColumnType::Numeric)
        statementM->setInt64((unsigned)index, value ? 1 : 0);
    else
        statementM->setBool((unsigned)index, value);
}

void FbCppStatement::setDate(int index, int year, int month, int day)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    if (index < 0 || (unsigned)index >= statementM->getInputDescriptors().size())
        return;
    statementM->setDate((unsigned)index, std::chrono::year(year) / month / day);
}

void FbCppStatement::setTime(int index, int hour, int minute, int second, int fraction)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    if (index < 0 || (unsigned)index >= statementM->getInputDescriptors().size())
        return;
    auto duration = std::chrono::hours(hour) + std::chrono::minutes(minute) +
        std::chrono::seconds(second) + std::chrono::microseconds(fraction * 100);
    statementM->setTime((unsigned)index, fbcpp::Time(duration));
}

void FbCppStatement::setTimestamp(int index, int year, int month, int day,
    int hour, int minute, int second, int fraction)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    if (index < 0 || (unsigned)index >= statementM->getInputDescriptors().size())
        return;
    auto date = std::chrono::year(year) / month / day;
    auto duration = std::chrono::hours(hour) + std::chrono::minutes(minute) +
        std::chrono::seconds(second) + std::chrono::microseconds(fraction * 100);
    statementM->setTimestamp((unsigned)index, fbcpp::Timestamp{ date, fbcpp::Time(duration) });
}

void FbCppStatement::setBytes(int index, const void* data, int size)
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    if (index < 0 || (unsigned)index >= statementM->getInputDescriptors().size())
        return;

    ColumnType type = getParameterType(index);
    if (type == ColumnType::Blob)
    {
        IBlobPtr b = std::make_shared<FbCppBlob>(databasePtrM, transactionPtrM);
        b->create();
        b->write(data, size);
        b->close();
        setBlob(index, b);
        return;
    }

    // fb-cpp's set method can take a std::string for OCTETS
    statementM->set((unsigned)index, std::string_view(static_cast<const char*>(data), size));
}

bool FbCppStatement::isNull(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    if (!rowAvailableM)
        return true;
    if (index < 0 || (unsigned)index >= statementM->getOutputDescriptors().size())
        return true;
    return statementM->isNull((unsigned)index);
}

std::string FbCppStatement::getString(int index)
{
    if (isNull(index))
        return "";

    ColumnType type = getColumnType(index);
    if (type == ColumnType::Blob)
    {
        IBlobPtr b = getBlob(index);
        if (!b)
            return "";
        try
        {
            b->open();
            std::string result;
            char buffer[8192];
            while (true)
            {
                int bytesRead = b->read(buffer, sizeof(buffer));
                if (bytesRead <= 0)
                    break;
                result.append(buffer, bytesRead);
            }
            b->close();
            return result;
        }
        catch (...)
        {
            return "";
        }
    }

    if (type == ColumnType::Int128)
    {
        auto val = statementM->get<std::optional<fbcpp::ScaledOpaqueInt128>>((unsigned)index);
        if (!val) return "";
        int128_t i128;
        std::uint64_t parts[2] = { val->value.fb_data[0], val->value.fb_data[1] };
        std::memcpy(&i128, parts, 16);
        return std::string(Int128ToString(i128).ToUTF8());
    }

    return statementM->get<std::optional<std::string>>((unsigned)index).value_or("");
}

int32_t FbCppStatement::getInt32(int index)
{
    if (isNull(index))
        return 0;

    ColumnType type = getColumnType(index);
    if (type == ColumnType::Boolean)
        return getBool(index) ? 1 : 0;
    if (type == ColumnType::Float || type == ColumnType::Double)
        return (int32_t)statementM->get<std::optional<double>>((unsigned)index).value_or(0.0);

    // For Numeric columns, we want the raw integer value as DataGrid handles the scale.
    // fb-cpp's get<int32_t> would adjust the scale to 0.
    // We use ScaledInt64 to fetch the raw value regardless of underlying integer size.
    if (type == ColumnType::Numeric || type == ColumnType::Integer || type == ColumnType::BigInt)
    {
        auto val = statementM->get<std::optional<fbcpp::ScaledInt64>>((unsigned)index);
        return val ? (int32_t)val->value : 0;
    }

    if (type == ColumnType::Int128)
    {
        auto val = statementM->get<std::optional<fbcpp::ScaledOpaqueInt128>>((unsigned)index);
        if (!val) return 0;
        int128_t i128;
        std::uint64_t parts[2] = { val->value.fb_data[0], val->value.fb_data[1] };
        std::memcpy(&i128, parts, 16);
#ifdef HAVE_INT128
        return (int32_t)i128;
#else
        return (int32_t)i128.lowPart;
#endif
    }

    return statementM->get<std::optional<std::int32_t>>((unsigned)index).value_or(0);
}

int64_t FbCppStatement::getInt64(int index)
{
    if (isNull(index))
        return 0;

    ColumnType type = getColumnType(index);
    if (type == ColumnType::Boolean)
        return getBool(index) ? 1 : 0;
    if (type == ColumnType::Float || type == ColumnType::Double)
        return (int64_t)statementM->get<std::optional<double>>((unsigned)index).value_or(0.0);

    if (type == ColumnType::Numeric || type == ColumnType::Integer || type == ColumnType::BigInt)
    {
        auto val = statementM->get<std::optional<fbcpp::ScaledInt64>>((unsigned)index);
        return val ? (int64_t)val->value : 0;
    }

    if (type == ColumnType::Int128)
    {
        auto val = statementM->get<std::optional<fbcpp::ScaledOpaqueInt128>>((unsigned)index);
        if (!val) return 0;
        int128_t i128;
        std::uint64_t parts[2] = { val->value.fb_data[0], val->value.fb_data[1] };
        std::memcpy(&i128, parts, 16);
#ifdef HAVE_INT128
        return (int64_t)i128;
#else
        return (int64_t)i128.lowPart;
#endif
    }

    return statementM->get<std::optional<std::int64_t>>((unsigned)index).value_or(0);
}

double FbCppStatement::getDouble(int index)
{
    if (isNull(index))
        return 0.0;
    return statementM->get<std::optional<double>>((unsigned)index).value_or(0.0);
}

bool FbCppStatement::getBool(int index)
{
    if (isNull(index))
        return false;

    ColumnType type = getColumnType(index);
    if (type == ColumnType::Boolean)
        return statementM->get<std::optional<bool>>((unsigned)index).value_or(false);
    if (type == ColumnType::Integer || type == ColumnType::BigInt || type == ColumnType::Numeric)
        return statementM->get<std::optional<int64_t>>((unsigned)index).value_or(0) != 0;

    return statementM->get<std::optional<bool>>((unsigned)index).value_or(false);
}

void FbCppStatement::getBytes(int index, void* data, int size)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    if (index < 0 || (unsigned)index >= statementM->getOutputDescriptors().size())
        return;

    ColumnType type = getColumnType(index);
    if (type == ColumnType::Blob)
    {
        IBlobPtr b = getBlob(index);
        if (!b)
            return;
        try
        {
            b->open();
            int totalRead = 0;
            while (totalRead < size)
            {
                int bytesRead = b->read(static_cast<char*>(data) + totalRead, size - totalRead);
                if (bytesRead <= 0)
                    break;
                totalRead += bytesRead;
            }
            b->close();
            return;
        }
        catch (...)
        {
            return;
        }
    }

    auto val = statementM->get<std::optional<std::string>>((unsigned)index);
    if (val)
    {
        int toCopy = std::min((int)val->size(), size);
        memcpy(data, val->data(), toCopy);
    }
}

IBlobPtr FbCppStatement::getBlob(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    if (index < 0 || (unsigned)index >= statementM->getOutputDescriptors().size())
        return nullptr;
    auto blobId = statementM->get<std::optional<fbcpp::BlobId>>((unsigned)index);
    if (!blobId || blobId->isEmpty())
        return nullptr;
    return std::make_shared<FbCppBlob>(databasePtrM, transactionPtrM, *blobId);
}

void FbCppStatement::setBlob(int index, IBlobPtr blob)
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    if (index < 0 || (unsigned)index >= statementM->getInputDescriptors().size())
        return;
    auto fbCppBlob = std::dynamic_pointer_cast<FbCppBlob>(blob);
    if (!fbCppBlob)
        throw std::runtime_error("Invalid blob type for fb-cpp backend");
    statementM->set((unsigned)index, fbCppBlob->getBlobId());
}

std::string FbCppStatement::getDate(int index)
{
    if (!statementM || !rowAvailableM)
        return "";
    if (index < 0 || (unsigned)index >= statementM->getOutputDescriptors().size())
        return "";
    return statementM->get<std::optional<std::string>>((unsigned)index).value_or("");
}

std::string FbCppStatement::getTime(int index)
{
    if (!statementM || !rowAvailableM)
        return "";
    if (index < 0 || (unsigned)index >= statementM->getOutputDescriptors().size())
        return "";
    return statementM->get<std::optional<std::string>>((unsigned)index).value_or("");
}

std::string FbCppStatement::getTimestamp(int index)
{
    if (!statementM || !rowAvailableM)
        return "";
    if (index < 0 || (unsigned)index >= statementM->getOutputDescriptors().size())
        return "";
    return statementM->get<std::optional<std::string>>((unsigned)index).value_or("");
}

std::string FbCppStatement::getTimeTz(int index)
{
    if (!statementM || !rowAvailableM)
        return "";
    if (index < 0 || (unsigned)index >= statementM->getOutputDescriptors().size())
        return "";
    return statementM->get<std::optional<std::string>>((unsigned)index).value_or("");
}

std::string FbCppStatement::getTimestampTz(int index)
{
    if (!statementM || !rowAvailableM)
        return "";
    if (index < 0 || (unsigned)index >= statementM->getOutputDescriptors().size())
        return "";
    return statementM->get<std::optional<std::string>>((unsigned)index).value_or("");
}

void FbCppStatement::getDate(int index, int& year, int& month, int& day)
{
    if (!statementM || !rowAvailableM)
    {
        year = 0; month = 0; day = 0;
        return;
    }
    if (index < 0 || (unsigned)index >= statementM->getOutputDescriptors().size())
    {
        year = 0; month = 0; day = 0;
        return;
    }
    auto d = statementM->get<std::optional<fbcpp::Date>>((unsigned)index).value_or(fbcpp::Date{});
    year = (int)d.year();
    month = (unsigned)d.month();
    day = (unsigned)d.day();
}

void FbCppStatement::getTime(int index, int& hour, int& minute, int& second, int& fraction)
{
    if (!statementM || !rowAvailableM)
    {
        hour = 0; minute = 0; second = 0; fraction = 0;
        return;
    }
    if (index < 0 || (unsigned)index >= statementM->getOutputDescriptors().size())
    {
        hour = 0; minute = 0; second = 0; fraction = 0;
        return;
    }
    auto t = statementM->get<std::optional<fbcpp::Time>>((unsigned)index).value_or(fbcpp::Time{});
    hour = (int)t.hours().count();
    minute = (int)t.minutes().count();
    second = (int)t.seconds().count();
    fraction = (int)std::chrono::duration_cast<std::chrono::microseconds>(t.subseconds()).count() / 100;
}

void FbCppStatement::getTimestamp(int index, int& year, int& month, int& day,
    int& hour, int& minute, int& second, int& fraction)
{
    if (!statementM || !rowAvailableM)
    {
        year = 0; month = 0; day = 0; hour = 0; minute = 0; second = 0; fraction = 0;
        return;
    }
    if (index < 0 || (unsigned)index >= statementM->getOutputDescriptors().size())
    {
        year = 0; month = 0; day = 0; hour = 0; minute = 0; second = 0; fraction = 0;
        return;
    }
    auto ts = statementM->get<std::optional<fbcpp::Timestamp>>((unsigned)index).value_or(fbcpp::Timestamp{});
    year = (int)ts.date.year();
    month = (unsigned)ts.date.month();
    day = (unsigned)ts.date.day();
    hour = (int)ts.time.hours().count();
    minute = (int)ts.time.minutes().count();
    second = (int)ts.time.seconds().count();
    fraction = (int)std::chrono::duration_cast<std::chrono::microseconds>(ts.time.subseconds()).count() / 100;
}

int FbCppStatement::getColumnCount()
{
    return (int)columnTypesM.size();
}

std::string FbCppStatement::getColumnName(int index)
{
    if (!statementM)
        return "";
    const auto& descriptors = statementM->getOutputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return "";
    return descriptors[index].alias.empty() ? descriptors[index].name : descriptors[index].alias;
}

ColumnType FbCppStatement::getColumnType(int index)
{
    if ((unsigned)index >= columnTypesM.size())
        return ColumnType::Unknown;
    return columnTypesM[index];
}

int FbCppStatement::getColumnSubtype(int index)
{
    if (!statementM)
        return 0;
    const auto& descriptors = statementM->getOutputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return 0;
    // For string columns (CHAR/VARCHAR), Firebird stores the character set ID
    // in the sqlsubtype field of XSQLVAR. The fb-cpp library separates this into
    // charSetId and subType, so we return charSetId here to match IBPP behaviour.
    if (descriptors[index].adjustedType == fbcpp::DescriptorAdjustedType::STRING)
        return (int)descriptors[index].charSetId;
    return descriptors[index].subType;
}

int FbCppStatement::getColumnScale(int index)
{
    if (!statementM)
        return 0;
    const auto& descriptors = statementM->getOutputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return 0;
    return -descriptors[index].scale;
}

int FbCppStatement::getColumnSize(int index)
{
    if (!statementM)
        return 0;
    const auto& descriptors = statementM->getOutputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return 0;
    return (int)descriptors[index].length;
}

std::string FbCppStatement::getColumnAlias(int index)
{
    if (!statementM)
        return "";
    const auto& descriptors = statementM->getOutputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return "";
    return descriptors[index].alias;
}

std::string FbCppStatement::getColumnTable(int index)
{
    if (!statementM)
        return "";
    const auto& descriptors = statementM->getOutputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return "";
    return descriptors[index].relation;
}

std::string FbCppStatement::getPlan()
{
    if (!statementM)
        return "";

    // The detailed plan is not available on older servers (e.g. Firebird 2.5),
    // so fall back to the legacy plan.
    try
    {
        std::string plan = statementM->getPlan();
        if (!plan.empty())
            return plan;
    }
    catch (const std::exception&)
    {
    }
    return statementM->getLegacyPlan();
}

StatementType FbCppStatement::getType()
{
    if (!statementM)
        return StatementType::Unknown;

#ifdef DELETE
#undef DELETE
#endif

    switch (statementM->getType())
    {
        case fbcpp::StatementType::SELECT: return StatementType::Select;
        case fbcpp::StatementType::INSERT: return StatementType::Insert;
        case fbcpp::StatementType::UPDATE: return StatementType::Update;
        case fbcpp::StatementType::DELETE: return StatementType::Delete;
        case (fbcpp::StatementType)25: return StatementType::Merge;
        case fbcpp::StatementType::DDL: return StatementType::DDL;
        case fbcpp::StatementType::EXEC_PROCEDURE: return StatementType::ExecProcedure;
        case fbcpp::StatementType::START_TRANSACTION: return StatementType::StartTransaction;
        case fbcpp::StatementType::COMMIT: return StatementType::Commit;
        case fbcpp::StatementType::ROLLBACK: return StatementType::Rollback;
        case fbcpp::StatementType::SAVEPOINT: return StatementType::Savepoint;
        case fbcpp::StatementType::SET_GENERATOR: return StatementType::SetGenerator;
        default: return StatementType::Unknown;
    }
}

int FbCppStatement::getParameterCount()
{
    return (int)parameterTypesM.size();
}

std::string FbCppStatement::getParameterName(int index)
{
    if (!parameterNamesM.empty())
    {
        if (index < 0 || (unsigned)index >= parameterNamesM.size())
            return "";
        return parameterNamesM[index];
    }

    if (!statementM)
        return "";
    const auto& descriptors = statementM->getInputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return "";
    return descriptors[index].name.empty() ? "?" : descriptors[index].name;
}

std::vector<int> FbCppStatement::findParameterIndicesByName(const std::string& name)
{
    if (!parameterNamesM.empty())
    {
        std::vector<int> result;
        std::string upperName = name;
        std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
        for (size_t i = 0; i < parameterNamesM.size(); ++i)
        {
            std::string upperParam = parameterNamesM[i];
            std::transform(upperParam.begin(), upperParam.end(), upperParam.begin(), ::toupper);
            if (upperParam == upperName)
            {
                result.push_back((int)i);
            }
        }
        return result;
    }

    std::vector<int> result;
    if (!statementM)
        return result;
    const auto& descriptors = statementM->getInputDescriptors();
    for (size_t i = 0; i < descriptors.size(); ++i)
    {
        // Case-insensitive comparison
        if (descriptors[i].name.size() == name.size())
        {
            bool match = true;
            for (size_t j = 0; j < name.size(); ++j)
            {
                if ((std::toupper)(descriptors[i].name[j]) != (std::toupper)(name[j]))
                {
                    match = false;
                    break;
                }
            }
            if (match)
                result.push_back((int)i);
        }
    }
    return result;
}

ColumnType FbCppStatement::getParameterType(int index)
{
    if ((unsigned)index >= parameterTypesM.size())
        return ColumnType::Unknown;
    return parameterTypesM[index];
}

int FbCppStatement::getParameterSubtype(int index)
{
    if (!statementM)
        return 0;
    const auto& descriptors = statementM->getInputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return 0;
    // For string parameters (CHAR/VARCHAR), Firebird stores the character set ID
    // in the sqlsubtype field of XSQLVAR. The fb-cpp library separates this into
    // charSetId and subType, so we return charSetId here to match IBPP behavior
    // and enable proper character set resolution (e.g., OCTETS detection in InsertParametersDialog).
    if (descriptors[index].adjustedType == fbcpp::DescriptorAdjustedType::STRING)
        return (int)descriptors[index].charSetId;
    return descriptors[index].subType;
}

int FbCppStatement::getParameterScale(int index)
{
    if (!statementM)
        return 0;
    const auto& descriptors = statementM->getInputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return 0;
    return descriptors[index].scale;
}

int FbCppStatement::getParameterSize(int index)
{
    if (!statementM)
        return 0;
    const auto& descriptors = statementM->getInputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return 0;
    return (int)descriptors[index].length;
}

int FbCppStatement::getAffectedRows()
{
    if (!statementM)
        return 0;

    auto& attachment = statementM->getAttachment();
    auto& client = attachment.getClient();
    fbcpp::impl::StatusWrapper status(client);

    static const unsigned char items[] = { isc_info_sql_records };
    unsigned char buffer[1024];
    memset(buffer, isc_info_end, sizeof(buffer));

    statementM->getStatementHandle()->getInfo(&status, sizeof(items), items, sizeof(buffer), buffer);
    if (status.getState() & Firebird::IStatus::STATE_ERRORS)
        return 0;

    auto decode = [](const unsigned char* p, int len) -> intptr_t {
        intptr_t v = 0;
        int shift = 0;
        if (len > (int)sizeof(intptr_t)) len = sizeof(intptr_t);
        while (len-- > 0) {
            v += (intptr_t)*p++ << shift;
            shift += 8;
        }
        return v;
    };

    int total = 0;
    for (int i = 0; i < (int)sizeof(buffer) && buffer[i] != isc_info_end; )
    {
        unsigned char item = buffer[i++];
        if (i + 2 > (int)sizeof(buffer)) break;
        int len = (int)decode(&buffer[i], 2);
        i += 2;
        
        if (item == isc_info_sql_records)
        {
            int end = i + len;
            if (end > (int)sizeof(buffer)) end = (int)sizeof(buffer);
            while (i < end)
            {
                unsigned char subitem = buffer[i++];
                if (i + 2 > end) break;
                int sublen = (int)decode(&buffer[i], 2);
                i += 2;
                if (i + sublen > end) break;
                int count = (int)decode(&buffer[i], sublen);
                i += sublen;

                if (subitem == isc_info_req_insert_count ||
                    subitem == isc_info_req_update_count ||
                    subitem == isc_info_req_delete_count)
                {
                    total += count;
                }
            }
        }
        else
        {
            i += len;
        }
    }
    return total;
}

namespace
{

bool isSqlIdentChar(char c)
{
    unsigned char uc = static_cast<unsigned char>(c);
    return std::isalnum(uc) || c == '_' || c == '$';
}

size_t skipWhitespaceAndComments(const std::string& sql, size_t pos)
{
    const size_t n = sql.size();
    while (pos < n)
    {
        unsigned char uc = static_cast<unsigned char>(sql[pos]);
        if (std::isspace(uc))
        {
            ++pos;
            continue;
        }
        if (sql[pos] == '-' && pos + 1 < n && sql[pos + 1] == '-')
        {
            pos += 2;
            while (pos < n && sql[pos] != '\n')
                ++pos;
            continue;
        }
        if (sql[pos] == '/' && pos + 1 < n && sql[pos + 1] == '*')
        {
            pos += 2;
            while (pos + 1 < n && !(sql[pos] == '*' && sql[pos + 1] == '/'))
                ++pos;
            pos = (pos + 1 < n) ? (pos + 2) : n;
            continue;
        }
        break;
    }
    return pos;
}

bool matchKeywordCI(const std::string& sql, size_t pos, const char* kwUpper, size_t& afterPos)
{
    const size_t kwLen = std::strlen(kwUpper);
    if (pos + kwLen > sql.size())
        return false;
    for (size_t k = 0; k < kwLen; ++k)
    {
        if (std::toupper(static_cast<unsigned char>(sql[pos + k])) != kwUpper[k])
            return false;
    }
    if (pos + kwLen < sql.size() && isSqlIdentChar(sql[pos + kwLen]))
        return false;
    afterPos = pos + kwLen;
    return true;
}

// Finds the exact byte index in `sql` where host parameter replacement must stop
// for EXECUTE BLOCK: host parameters (:PARAM / ?) are ONLY legal inside the
// optional `EXECUTE BLOCK ( ... )` input clause before RETURNS / AS (#779).
// Returns true if `sql` starts with EXECUTE BLOCK, setting `paramScanEnd` to the
// index one past the closing ')' of the input parameter list (or 0 if there is
// no input parameter list).
bool findExecuteBlockParamLimit(const std::string& sql, size_t& paramScanEnd)
{
    size_t pos = skipWhitespaceAndComments(sql, 0);
    size_t afterExec = 0;
    if (!matchKeywordCI(sql, pos, "EXECUTE", afterExec))
        return false;

    pos = skipWhitespaceAndComments(sql, afterExec);
    size_t afterBlock = 0;
    if (!matchKeywordCI(sql, pos, "BLOCK", afterBlock))
        return false;

    pos = skipWhitespaceAndComments(sql, afterBlock);
    if (pos >= sql.size() || sql[pos] != '(')
    {
        // EXECUTE BLOCK without an input parameter list has zero host parameters.
        paramScanEnd = 0;
        return true;
    }

    // Walk balanced parentheses from '(' to its matching ')', skipping comments and quotes.
    const size_t n = sql.size();
    int depth = 0;
    for (size_t i = pos; i < n; ++i)
    {
        char c = sql[i];
        if (c == '-' && i + 1 < n && sql[i + 1] == '-')
        {
            i += 2;
            while (i < n && sql[i] != '\n')
                ++i;
            continue;
        }
        if (c == '/' && i + 1 < n && sql[i + 1] == '*')
        {
            i += 2;
            while (i + 1 < n && !(sql[i] == '*' && sql[i + 1] == '/'))
                ++i;
            ++i; // loop ++i steps past '/'
            continue;
        }
        if (c == '\'' || c == '"')
        {
            char q = c;
            ++i;
            while (i < n)
            {
                if (sql[i] == q)
                {
                    if (i + 1 < n && sql[i + 1] == q)
                    {
                        i += 2;
                        continue;
                    }
                    break;
                }
                ++i;
            }
            continue;
        }
        if (c == '(')
        {
            ++depth;
        }
        else if (c == ')')
        {
            --depth;
            if (depth == 0)
            {
                paramScanEnd = i + 1;
                return true;
            }
        }
    }

    paramScanEnd = sql.size();
    return true;
}

bool isParameterizableStatement(const std::string& sql, bool& isExecuteBlock, size_t& executeBlockParamEnd)
{
    isExecuteBlock = false;
    executeBlockParamEnd = std::string::npos;

    if (findExecuteBlockParamLimit(sql, executeBlockParamEnd))
    {
        isExecuteBlock = true;
        return true;
    }

    size_t pos = skipWhitespaceAndComments(sql, 0);
    size_t afterKw = 0;
    if (matchKeywordCI(sql, pos, "SELECT", afterKw)
        || matchKeywordCI(sql, pos, "INSERT", afterKw)
        || matchKeywordCI(sql, pos, "UPDATE", afterKw)
        || matchKeywordCI(sql, pos, "DELETE", afterKw)
        || matchKeywordCI(sql, pos, "MERGE", afterKw)
        || matchKeywordCI(sql, pos, "WITH", afterKw))
    {
        return true;
    }

    if (matchKeywordCI(sql, pos, "EXECUTE", afterKw))
    {
        size_t nextPos = skipWhitespaceAndComments(sql, afterKw);
        size_t afterProc = 0;
        if (matchKeywordCI(sql, nextPos, "PROCEDURE", afterProc))
            return true;
    }

    return false;
}

std::string stripTrailingSemicolonAndSpace(std::string res)
{
    while (!res.empty()
        && (std::isspace(static_cast<unsigned char>(res.back())) || res.back() == ';'))
    {
        res.pop_back();
    }
    return res;
}

} // namespace

std::string FbCppStatement::preprocessSqlForParameters(
    const std::string& sql, std::vector<std::string>& outParameterNames)
{
    outParameterNames.clear();

    bool isExecuteBlock = false;
    size_t executeBlockParamEnd = std::string::npos;
    if (!isParameterizableStatement(sql, isExecuteBlock, executeBlockParamEnd))
        return sql;

    if (isExecuteBlock && executeBlockParamEnd == 0)
        return stripTrailingSemicolonAndSpace(sql);

    const size_t n = sql.length();
    const size_t scanLimit = isExecuteBlock ? executeBlockParamEnd : n;

    std::ostringstream sProcessedSQL;
    size_t i = 0;
    while (i < scanLimit)
    {
        char c = sql[i];

        // Single-line comment --
        if (c == '-' && i + 1 < scanLimit && sql[i + 1] == '-')
        {
            size_t start = i;
            i += 2;
            while (i < scanLimit && sql[i] != '\n')
                ++i;
            sProcessedSQL.write(&sql[start], static_cast<std::streamsize>(i - start));
            continue;
        }

        // Block comment /* ... */
        if (c == '/' && i + 1 < scanLimit && sql[i + 1] == '*')
        {
            size_t start = i;
            i += 2;
            while (i + 1 < scanLimit && !(sql[i] == '*' && sql[i + 1] == '/'))
                ++i;
            if (i + 1 < scanLimit)
                i += 2;
            else
                i = scanLimit;
            sProcessedSQL.write(&sql[start], static_cast<std::streamsize>(i - start));
            continue;
        }

        // Quoted string '...' or delimited identifier "..."
        if (c == '\'' || c == '"')
        {
            char q = c;
            size_t start = i++;
            while (i < scanLimit)
            {
                if (sql[i] == q)
                {
                    ++i;
                    if (i < scanLimit && sql[i] == q)
                    {
                        ++i;
                        continue;
                    }
                    break;
                }
                ++i;
            }
            sProcessedSQL.write(&sql[start], static_cast<std::streamsize>(i - start));
            continue;
        }

        if (c == '?')
        {
            outParameterNames.push_back("?");
            sProcessedSQL << '?';
            ++i;
            continue;
        }

        // Named parameter :PARAM (ignore Firebird/SQL '::' operator if ever present)
        if (c == ':')
        {
            if (i + 1 < scanLimit && sql[i + 1] == ':')
            {
                sProcessedSQL << "::";
                i += 2;
                continue;
            }
            if (i + 1 < scanLimit && isSqlIdentChar(sql[i + 1]))
            {
                ++i; // consume ':'
                std::string rawName;
                std::string upperName;
                while (i < scanLimit && isSqlIdentChar(sql[i]))
                {
                    rawName.push_back(sql[i]);
                    upperName.push_back(
                        static_cast<char>(std::toupper(static_cast<unsigned char>(sql[i]))));
                    ++i;
                }
                sProcessedSQL << "?/*:" << rawName << "*/";
                outParameterNames.push_back(std::move(upperName));
                continue;
            }
        }

        sProcessedSQL << c;
        ++i;
    }

    if (scanLimit < n)
        sProcessedSQL.write(&sql[scanLimit], static_cast<std::streamsize>(n - scanLimit));

    return stripTrailingSemicolonAndSpace(sProcessedSQL.str());
}

std::string FbCppStatement::preprocessSql(const std::string& sql)
{
    return preprocessSqlForParameters(sql, parameterNamesM);
}

} // namespace fr
