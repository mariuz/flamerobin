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
#include "engine/db/IDatabase.h"
#include "engine/db/ITransaction.h"
#include <fb-cpp/Exception.h>
#include <fb-cpp/Statement.h>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <firebird/Interface.h>
#include <wx/log.h>

namespace fr
{

FbCppStatement::FbCppStatement(IDatabasePtr db, ITransactionPtr tr, fbcpp::Attachment& attachment, fbcpp::Transaction& transaction)
    : databasePtrM(db), transactionPtrM(tr), attachmentM(attachment), transactionM(transaction), eofReachedM(false), rowAvailableM(false)
{
}

FbCppStatement::~FbCppStatement()
{
    try
    {
        close();
    }
    catch (...)
    {
        // Never allow destructor to throw
    }
}

void FbCppStatement::prepare(const std::string& sql)
{
    sqlM = sql;
    wxLogDebug("FbCppStatement::prepare() for SQL: %s", sqlM.c_str());
    try {
        statementM.emplace(attachmentM, transactionM, sql);
    } catch (const std::exception& e) {
        wxLogDebug("FbCppStatement::prepare() failed: %s", e.what());
        throw;
    }
    resultSetM.reset();
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
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    
    if (!transactionPtrM || !transactionPtrM->isActive())
        throw std::runtime_error("Transaction not active");

    wxLogDebug("FbCppStatement::execute() for SQL: %s", sqlM.c_str());
    firstRowFetchedM.reset();
    eofReachedM = false;
    rowAvailableM = false;

    try {
        wxLogDebug("FbCppStatement::execute() calling library execute().");
        bool hasRow = statementM->execute(transactionM);
        
        // If the statement has output columns but is not a SELECT (e.g. INSERT ... RETURNING),
        // fb-cpp might already have the first row.
        if (getColumnCount() > 0 && statementM->getType() != fbcpp::StatementType::SELECT)
        {
            firstRowFetchedM = hasRow;
            eofReachedM = !hasRow;
            wxLogDebug("FbCppStatement::execute() - non-SELECT statement with output. hasRow: %d", (int)hasRow);
        }
    } catch (const std::exception& e) {
        wxLogDebug("FbCppStatement::execute() failed: %s", e.what());
        throw;
    }
}

bool FbCppStatement::fetch()
{
    if (!statementM)
    {
        wxLogDebug("FbCppStatement::fetch() - no statement.");
        return false;
    }

    if (!transactionPtrM || !transactionPtrM->isActive())
    {
        wxLogDebug("FbCppStatement::fetch() - transaction not active.");
        return false;
    }

    if (firstRowFetchedM.has_value())
    {
        bool res = *firstRowFetchedM;
        firstRowFetchedM.reset();
        rowAvailableM = res;
        wxLogDebug("FbCppStatement::fetch() returning pre-fetched row: %d", (int)res);
        return res;
    }

    if (eofReachedM)
        return false;

    try {
        wxLogDebug("FbCppStatement::fetch() calling library fetchNext().");
        bool res = statementM->fetchNext();
        rowAvailableM = res;
        if (!res)
        {
            wxLogDebug("FbCppStatement::fetch() - library EOF.");
            eofReachedM = true;
        }
        else
        {
            wxLogDebug("FbCppStatement::fetch() - library row fetched.");
        }
        return res;
    } catch (const std::exception& e) {
        wxLogDebug("FbCppStatement::fetch() failed: %s", e.what());
        eofReachedM = true;
        throw;
    }
}

void FbCppStatement::close()
{
    try
    {
        if (statementM)
            statementM->closeCursor();
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
        IBlobPtr b = std::make_shared<FbCppBlob>(attachmentM, transactionM);
        b->create();
        b->open();
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
        IBlobPtr b = std::make_shared<FbCppBlob>(attachmentM, transactionM);
        b->create();
        b->open();
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

    return statementM->get<std::optional<std::string>>((unsigned)index).value_or("");
}

int32_t FbCppStatement::getInt32(int index)
{
    if (isNull(index))
        return 0;

    ColumnType type = getColumnType(index);
    if (type == ColumnType::Boolean)
        return statementM->get<std::optional<bool>>((unsigned)index).value_or(false) ? 1 : 0;
    if (type == ColumnType::Float || type == ColumnType::Double)
        return (int32_t)statementM->get<std::optional<double>>((unsigned)index).value_or(0.0);

    return statementM->get<std::optional<std::int32_t>>((unsigned)index).value_or(0);
}

int64_t FbCppStatement::getInt64(int index)
{
    if (isNull(index))
        return 0;

    ColumnType type = getColumnType(index);
    if (type == ColumnType::Boolean)
        return statementM->get<std::optional<bool>>((unsigned)index).value_or(false) ? 1 : 0;
    if (type == ColumnType::Float || type == ColumnType::Double)
        return (int64_t)statementM->get<std::optional<double>>((unsigned)index).value_or(0.0);

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
    return std::make_shared<FbCppBlob>(attachmentM, transactionM, *blobId);
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
    if (!statementM)
        return 0;
    return (int)statementM->getOutputDescriptors().size();
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
    if (!statementM)
        return ColumnType::Unknown;
    const auto& descriptors = statementM->getOutputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return ColumnType::Unknown;
    
    switch (descriptors[index].adjustedType)
    {
        case fbcpp::DescriptorAdjustedType::STRING: return ColumnType::Varchar;
        case fbcpp::DescriptorAdjustedType::INT32: 
        case fbcpp::DescriptorAdjustedType::INT16: 
            return descriptors[index].scale != 0 ? ColumnType::Numeric : ColumnType::Integer;
        case fbcpp::DescriptorAdjustedType::INT64: 
            return descriptors[index].scale != 0 ? ColumnType::Numeric : ColumnType::BigInt;
        case fbcpp::DescriptorAdjustedType::FLOAT: return ColumnType::Float;
        case fbcpp::DescriptorAdjustedType::DOUBLE: return ColumnType::Double;
        case fbcpp::DescriptorAdjustedType::TIME: return ColumnType::Time;
        case fbcpp::DescriptorAdjustedType::DATE: return ColumnType::Date;
        case fbcpp::DescriptorAdjustedType::TIMESTAMP: return ColumnType::Timestamp;
        case fbcpp::DescriptorAdjustedType::TIME_TZ: return ColumnType::TimeTz;
        case fbcpp::DescriptorAdjustedType::TIMESTAMP_TZ: return ColumnType::TimestampTz;
        case fbcpp::DescriptorAdjustedType::BLOB: return ColumnType::Blob;
        case fbcpp::DescriptorAdjustedType::BOOLEAN: return ColumnType::Boolean;
        case fbcpp::DescriptorAdjustedType::INT128: return ColumnType::Int128;
        case fbcpp::DescriptorAdjustedType::DECFLOAT16: return ColumnType::Decfloat16;
        case fbcpp::DescriptorAdjustedType::DECFLOAT34: return ColumnType::Decfloat34;
        default: return ColumnType::Unknown;
    }
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
    return statementM->getPlan();
}

StatementType FbCppStatement::getType()
{
    if (!statementM)
        return StatementType::Unknown;
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
    if (!statementM)
        return 0;
    return (int)statementM->getInputDescriptors().size();
}

std::string FbCppStatement::getParameterName(int index)
{
    if (!statementM)
        return "";
    const auto& descriptors = statementM->getInputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return "";
    return descriptors[index].name.empty() ? "?" : descriptors[index].name;
}

std::vector<int> FbCppStatement::findParameterIndicesByName(const std::string& name)
{
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
                if (std::toupper(descriptors[i].name[j]) != std::toupper(name[j]))
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
    if (!statementM)
        return ColumnType::Unknown;
    const auto& descriptors = statementM->getInputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return ColumnType::Unknown;

    switch (descriptors[index].adjustedType)
    {
        case fbcpp::DescriptorAdjustedType::STRING: return ColumnType::Varchar;
        case fbcpp::DescriptorAdjustedType::INT32: 
        case fbcpp::DescriptorAdjustedType::INT16: 
            return descriptors[index].scale != 0 ? ColumnType::Numeric : ColumnType::Integer;
        case fbcpp::DescriptorAdjustedType::INT64: 
            return descriptors[index].scale != 0 ? ColumnType::Numeric : ColumnType::BigInt;
        case fbcpp::DescriptorAdjustedType::FLOAT: return ColumnType::Float;
        case fbcpp::DescriptorAdjustedType::DOUBLE: return ColumnType::Double;
        case fbcpp::DescriptorAdjustedType::TIME: return ColumnType::Time;
        case fbcpp::DescriptorAdjustedType::DATE: return ColumnType::Date;
        case fbcpp::DescriptorAdjustedType::TIMESTAMP: return ColumnType::Timestamp;
        case fbcpp::DescriptorAdjustedType::TIME_TZ: return ColumnType::TimeTz;
        case fbcpp::DescriptorAdjustedType::TIMESTAMP_TZ: return ColumnType::TimestampTz;
        case fbcpp::DescriptorAdjustedType::BLOB: return ColumnType::Blob;
        case fbcpp::DescriptorAdjustedType::BOOLEAN: return ColumnType::Boolean;
        case fbcpp::DescriptorAdjustedType::INT128: return ColumnType::Int128;
        case fbcpp::DescriptorAdjustedType::DECFLOAT16: return ColumnType::Decfloat16;
        case fbcpp::DescriptorAdjustedType::DECFLOAT34: return ColumnType::Decfloat34;
        default: return ColumnType::Unknown;
    }
}

int FbCppStatement::getParameterSubtype(int index)
{
    if (!statementM)
        return 0;
    const auto& descriptors = statementM->getInputDescriptors();
    if ((unsigned)index >= descriptors.size())
        return 0;
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

} // namespace fr
