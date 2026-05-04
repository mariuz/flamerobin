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
#include <fb-cpp/Exception.h>
#include <cstring>
#include <stdexcept>

namespace fr
{

FbCppStatement::FbCppStatement(IDatabasePtr db, ITransactionPtr tr, fbcpp::Attachment& attachment, fbcpp::Transaction& transaction)
    : databasePtrM(db), transactionPtrM(tr), attachmentM(attachment), transactionM(transaction), eofReachedM(false)
{
}

void FbCppStatement::prepare(const std::string& sql)
{
    sqlM = sql;
    statementM.emplace(attachmentM, transactionM, sql);
    firstRowFetchedM.reset();
    eofReachedM = false;
}

std::string FbCppStatement::getSql() const
{
    return sqlM;
}

void FbCppStatement::execute()
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    
    firstRowFetchedM.reset();
    eofReachedM = false;

    bool hasRow = statementM->execute(transactionM);
    // Support Multiple-Row DML RETURNING (Firebird 5.0+)
    // If the statement has output columns, we must handle the first row
    // and subsequent fetches, even if it's not a SELECT statement.
    if (getColumnCount() > 0)
    {
        firstRowFetchedM = hasRow;
        eofReachedM = !hasRow;
    }
}

bool FbCppStatement::fetch()
{
    if (!statementM)
        return false;

    if (firstRowFetchedM.has_value())
    {
        bool res = *firstRowFetchedM;
        firstRowFetchedM.reset();
        return res;
    }

    if (eofReachedM)
        return false;

    bool res = statementM->fetchNext();
    if (!res)
        eofReachedM = true;
    return res;
}

void FbCppStatement::close()
{
    if (statementM)
        statementM->free();
    statementM.reset();
    firstRowFetchedM.reset();
    eofReachedM = false;
}

void FbCppStatement::setNull(int index)
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    statementM->setNull((unsigned)index);
}

void FbCppStatement::setString(int index, const std::string& value)
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    statementM->set((unsigned)index, std::string_view(value));
}

void FbCppStatement::setInt32(int index, int32_t value)
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    statementM->setInt32((unsigned)index, value);
}

void FbCppStatement::setInt64(int index, int64_t value)
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    statementM->setInt64((unsigned)index, value);
}

void FbCppStatement::setDouble(int index, double value)
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    statementM->setDouble((unsigned)index, value);
}

void FbCppStatement::setBool(int index, bool value)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    statementM->setBool((unsigned)index, value);
}

void FbCppStatement::setDate(int index, int year, int month, int day)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    statementM->setDate((unsigned)index, std::chrono::year(year) / month / day);
}

void FbCppStatement::setTime(int index, int hour, int minute, int second, int fraction)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    auto duration = std::chrono::hours(hour) + std::chrono::minutes(minute) +
        std::chrono::seconds(second) + std::chrono::microseconds(fraction * 100);
    statementM->setTime((unsigned)index, fbcpp::Time(duration));
}

void FbCppStatement::setTimestamp(int index, int year, int month, int day,
    int hour, int minute, int second, int fraction)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    auto date = std::chrono::year(year) / month / day;
    auto duration = std::chrono::hours(hour) + std::chrono::minutes(minute) +
        std::chrono::seconds(second) + std::chrono::microseconds(fraction * 100);
    statementM->setTimestamp((unsigned)index, fbcpp::Timestamp{ date, fbcpp::Time(duration) });
}

void FbCppStatement::setBytes(int index, const void* data, int size)

{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    // fb-cpp's set method can take a std::string for OCTETS
    statementM->set((unsigned)index, std::string_view(static_cast<const char*>(data), size));
}

bool FbCppStatement::isNull(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    return statementM->isNull((unsigned)index);
}

std::string FbCppStatement::getString(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        return statementM->get<std::optional<std::string>>((unsigned)index).value_or("");
    }
    catch (...)
    {
        return "";
    }
}

int32_t FbCppStatement::getInt32(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        return statementM->get<std::optional<std::int32_t>>((unsigned)index).value_or(0);
    }
    catch (...)
    {
        return 0;
    }
}

int64_t FbCppStatement::getInt64(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        return statementM->get<std::optional<std::int64_t>>((unsigned)index).value_or(0);
    }
    catch (...)
    {
        return 0;
    }
}

double FbCppStatement::getDouble(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        return statementM->get<std::optional<double>>((unsigned)index).value_or(0.0);
    }
    catch (...)
    {
        return 0.0;
    }
}

bool FbCppStatement::getBool(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        return statementM->get<std::optional<bool>>((unsigned)index).value_or(false);
    }
    catch (...)
    {
        return false;
    }
}

void FbCppStatement::getBytes(int index, void* data, int size)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        auto val = statementM->get<std::optional<std::string>>((unsigned)index);
        if (val)
        {
            int toCopy = std::min((int)val->size(), size);
            memcpy(data, val->data(), toCopy);
        }
    }
    catch (...)
    {
    }
}

IBlobPtr FbCppStatement::getBlob(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        auto blobId = statementM->get<std::optional<fbcpp::BlobId>>((unsigned)index);
        if (!blobId || blobId->isEmpty())
            return nullptr;
        return std::make_shared<FbCppBlob>(attachmentM, transactionM, *blobId);
    }
    catch (...)
    {
        return nullptr;
    }
}

void FbCppStatement::setBlob(int index, IBlobPtr blob)
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    try
    {
        auto fbCppBlob = std::dynamic_pointer_cast<FbCppBlob>(blob);
        if (!fbCppBlob)
            throw std::runtime_error("Invalid blob type for fb-cpp backend");
        statementM->set((unsigned)index, fbCppBlob->getBlobId());
    }
    catch (...)
    {
        throw;
    }
}

std::string FbCppStatement::getDate(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        return statementM->get<std::optional<std::string>>((unsigned)index).value_or("");
    }
    catch (...)
    {
        return "";
    }
}

std::string FbCppStatement::getTime(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        return statementM->get<std::optional<std::string>>((unsigned)index).value_or("");
    }
    catch (...)
    {
        return "";
    }
}

std::string FbCppStatement::getTimestamp(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        return statementM->get<std::optional<std::string>>((unsigned)index).value_or("");
    }
    catch (...)
    {
        return "";
    }
}

std::string FbCppStatement::getTimeTz(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        return statementM->get<std::optional<std::string>>((unsigned)index).value_or("");
    }
    catch (...)
    {
        return "";
    }
}

std::string FbCppStatement::getTimestampTz(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        return statementM->get<std::optional<std::string>>((unsigned)index).value_or("");
    }
    catch (...)
    {
        return "";
    }
}

void FbCppStatement::getDate(int index, int& year, int& month, int& day)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        auto d = statementM->get<std::optional<fbcpp::Date>>((unsigned)index).value_or(fbcpp::Date{});
        year = (int)d.year();
        month = (unsigned)d.month();
        day = (unsigned)d.day();
    }
    catch (...)
    {
        year = month = day = 0;
    }
}

void FbCppStatement::getTime(int index, int& hour, int& minute, int& second, int& fraction)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        auto t = statementM->get<std::optional<fbcpp::Time>>((unsigned)index).value_or(fbcpp::Time{});
        hour = (int)t.hours().count();
        minute = (int)t.minutes().count();
        second = (int)t.seconds().count();
        fraction = (int)std::chrono::duration_cast<std::chrono::microseconds>(t.subseconds()).count() / 100;
    }
    catch (...)
    {
        hour = minute = second = fraction = 0;
    }
}

void FbCppStatement::getTimestamp(int index, int& year, int& month, int& day,
    int& hour, int& minute, int& second, int& fraction)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    try
    {
        auto ts = statementM->get<std::optional<fbcpp::Timestamp>>((unsigned)index).value_or(fbcpp::Timestamp{});
        year = (int)ts.date.year();
        month = (unsigned)ts.date.month();
        day = (unsigned)ts.date.day();
        hour = (int)ts.time.hours().count();
        minute = (int)ts.time.minutes().count();
        second = (int)ts.time.seconds().count();
        fraction = (int)std::chrono::duration_cast<std::chrono::microseconds>(ts.time.subseconds()).count() / 100;
    }
    catch (...)
    {
        year = month = day = hour = minute = second = fraction = 0;
    }
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
        case fbcpp::DescriptorAdjustedType::INT32: return ColumnType::Integer;
        case fbcpp::DescriptorAdjustedType::INT16: return ColumnType::Integer;
        case fbcpp::DescriptorAdjustedType::INT64: return ColumnType::BigInt;
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
    try
    {
        return statementM->getPlan();
    }
    catch (...)
    {
        return "";
    }
}

StatementType FbCppStatement::getType()
{
    if (!statementM)
        return StatementType::Unknown;
    try
    {
        switch (statementM->getType())
        {
            case fbcpp::StatementType::SELECT: return StatementType::Select;
            case fbcpp::StatementType::INSERT: return StatementType::Insert;
            case fbcpp::StatementType::UPDATE: return StatementType::Update;
            case fbcpp::StatementType::DELETE: return StatementType::Delete;
            case fbcpp::StatementType::DDL: return StatementType::DDL;
            case fbcpp::StatementType::EXEC_PROCEDURE: return StatementType::ExecProcedure;
            case fbcpp::StatementType::START_TRANSACTION: return StatementType::StartTransaction;
            case fbcpp::StatementType::COMMIT: return StatementType::Commit;
            case fbcpp::StatementType::ROLLBACK: return StatementType::Rollback;
            case fbcpp::StatementType::SAVEPOINT: return StatementType::Savepoint;
            default: return StatementType::Unknown;
        }
    }
    catch (...)
    {
        return StatementType::Unknown;
    }
}

int FbCppStatement::getParameterCount()
{
    if (!statementM)
        return 0;
    try
    {
        return (int)statementM->getInputDescriptors().size();
    }
    catch (...)
    {
        return 0;
    }
}

std::string FbCppStatement::getParameterName(int index)
{
    if (!statementM)
        return "";
    try
    {
        const auto& descriptors = statementM->getInputDescriptors();
        if ((unsigned)index >= descriptors.size())
            return "";
        return descriptors[index].name.empty() ? "?" : descriptors[index].name;
    }
    catch (...)
    {
        return "";
    }
}

std::vector<int> FbCppStatement::findParameterIndicesByName(const std::string& name)
{
    std::vector<int> result;
    if (!statementM)
        return result;
    try
    {
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
    }
    catch (...)
    {
    }
    return result;
}

ColumnType FbCppStatement::getParameterType(int index)
{
    if (!statementM)
        return ColumnType::Unknown;
    try
    {
        const auto& descriptors = statementM->getInputDescriptors();
        if ((unsigned)index >= descriptors.size())
            return ColumnType::Unknown;

        switch (descriptors[index].adjustedType)
        {
            case fbcpp::DescriptorAdjustedType::STRING: return ColumnType::Varchar;
            case fbcpp::DescriptorAdjustedType::INT32: return ColumnType::Integer;
            case fbcpp::DescriptorAdjustedType::INT16: return ColumnType::Integer;
            case fbcpp::DescriptorAdjustedType::INT64: return ColumnType::BigInt;
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
    catch (...)
    {
        return ColumnType::Unknown;
    }
}

int FbCppStatement::getParameterSubtype(int index)
{
    if (!statementM)
        return 0;
    try
    {
        const auto& descriptors = statementM->getInputDescriptors();
        if ((unsigned)index >= descriptors.size())
            return 0;
        return descriptors[index].subType;
    }
    catch (...)
    {
        return 0;
    }
}

int FbCppStatement::getParameterScale(int index)
{
    if (!statementM)
        return 0;
    try
    {
        const auto& descriptors = statementM->getInputDescriptors();
        if ((unsigned)index >= descriptors.size())
            return 0;
        return descriptors[index].scale;
    }
    catch (...)
    {
        return 0;
    }
}

int FbCppStatement::getParameterSize(int index)
{
    if (!statementM)
        return 0;
    try
    {
        const auto& descriptors = statementM->getInputDescriptors();
        if ((unsigned)index >= descriptors.size())
            return 0;
        return (int)descriptors[index].length;
    }
    catch (...)
    {
        return 0;
    }
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

    try 
    {
        statementM->getStatementHandle()->getInfo(&status, sizeof(items), items, sizeof(buffer), buffer);
        if (status.getState() & Firebird::IStatus::STATE_ERRORS)
            return 0;
    }
    catch (...)
    {
        return 0;
    }

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
