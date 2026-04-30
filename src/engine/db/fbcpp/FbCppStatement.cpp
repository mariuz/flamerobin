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
#include <stdexcept>

namespace fr
{

FbCppStatement::FbCppStatement(fbcpp::Attachment& attachment, fbcpp::Transaction& transaction)
    : attachmentM(attachment), transactionM(transaction)
{
}

void FbCppStatement::prepare(const std::string& sql)
{
    sqlM = sql;
    statementM.emplace(attachmentM, transactionM, sql);
}

std::string FbCppStatement::getSql() const
{
    return sqlM;
}

void FbCppStatement::execute()
{
    if (!statementM)
        throw std::runtime_error("Statement not prepared");
    statementM->execute(transactionM);
}

bool FbCppStatement::fetch()
{
    if (!statementM)
        return false;
    return statementM->fetchNext();
}

void FbCppStatement::close()
{
    if (statementM)
        statementM->free();
    statementM.reset();
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
        throw std::runtime_error("Statement not prepared");
    statementM->setBool((unsigned)index, value);
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
    return statementM->getString((unsigned)index).value_or("");
}

int32_t FbCppStatement::getInt32(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    return statementM->getInt32((unsigned)index).value_or(0);
}

int64_t FbCppStatement::getInt64(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    return statementM->getInt64((unsigned)index).value_or(0);
}

double FbCppStatement::getDouble(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    return statementM->getDouble((unsigned)index).value_or(0.0);
}

bool FbCppStatement::getBool(int index)
{
    if (!statementM)
        throw std::runtime_error("No statement available");
    return statementM->getBool((unsigned)index).value_or(false);
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
        case fbcpp::DescriptorAdjustedType::BLOB: return ColumnType::Blob;
        case fbcpp::DescriptorAdjustedType::BOOLEAN: return ColumnType::Boolean;
        default: return ColumnType::Unknown;
    }
}

} // namespace fr
