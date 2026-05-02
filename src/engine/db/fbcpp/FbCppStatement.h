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

#ifndef FR_FBCPP_STATEMENT_H
#define FR_FBCPP_STATEMENT_H

#include "engine/db/IStatement.h"
#include <fb-cpp/fb-cpp.h>
#include <optional>

namespace fr
{

class FbCppStatement : public IStatement
{
public:
    FbCppStatement(IDatabasePtr db, ITransactionPtr tr, fbcpp::Attachment& attachment, fbcpp::Transaction& transaction);
    virtual ~FbCppStatement() = default;

    virtual void prepare(const std::string& sql) override;
    virtual std::string getSql() const override;
    virtual void execute() override;
    virtual bool fetch() override;
    virtual void close() override;

    // Parameter binding (0-based)
    virtual void setNull(int index) override;
    virtual void setString(int index, const std::string& value) override;
    virtual void setInt32(int index, int32_t value) override;
    virtual void setInt64(int index, int64_t value) override;
    virtual void setDouble(int index, double value) override;
    virtual void setBool(int index, bool value) override;
    virtual void setDate(int index, int year, int month, int day) override;
    virtual void setTime(int index, int hour, int minute, int second, int fraction) override;
    virtual void setTimestamp(int index, int year, int month, int day,
        int hour, int minute, int second, int fraction) override;
    virtual void setBytes(int index, const void* data, int size) override;

    // Result fetching (0-based)
    virtual bool isNull(int index) override;
    virtual std::string getString(int index) override;
    virtual int32_t getInt32(int index) override;
    virtual int64_t getInt64(int index) override;
    virtual double getDouble(int index) override;
    virtual bool getBool(int index) override;

    virtual void getBytes(int index, void* data, int size) override;
    virtual IBlobPtr getBlob(int index) override;
    virtual void setBlob(int index, IBlobPtr blob) override;

    virtual std::string getDate(int index) override;
    virtual std::string getTime(int index) override;
    virtual std::string getTimestamp(int index) override;
    virtual std::string getTimeTz(int index) override;
    virtual std::string getTimestampTz(int index) override;

    virtual void getDate(int index, int& year, int& month, int& day) override;
    virtual void getTime(int index, int& hour, int& minute, int& second, int& fraction) override;
    virtual void getTimestamp(int index, int& year, int& month, int& day,
        int& hour, int& minute, int& second, int& fraction) override;

    virtual int getColumnCount() override;
    virtual std::string getColumnName(int index) override;
    virtual ColumnType getColumnType(int index) override;
    virtual int getColumnSubtype(int index) override;
    virtual int getColumnScale(int index) override;
    virtual int getColumnSize(int index) override;
    virtual std::string getColumnAlias(int index) override;
    virtual std::string getColumnTable(int index) override;

    virtual std::string getPlan() override;
    virtual StatementType getType() override;
    virtual int getParameterCount() override;
    virtual std::string getParameterName(int index) override;
    virtual std::vector<int> findParameterIndicesByName(const std::string& name) override;
    virtual ColumnType getParameterType(int index) override;
    virtual int getParameterSubtype(int index) override;
    virtual int getParameterScale(int index) override;
    virtual int getParameterSize(int index) override;

    virtual int getAffectedRows() override;

    virtual IDatabasePtr getDatabase() override { return databasePtrM; }
    virtual ITransactionPtr getTransaction() override { return transactionPtrM; }

private:
    IDatabasePtr databasePtrM;
    ITransactionPtr transactionPtrM;
    fbcpp::Attachment& attachmentM;
    fbcpp::Transaction& transactionM;
    std::string sqlM;
    std::optional<fbcpp::Statement> statementM;
    std::optional<fbcpp::RowSet> rowSetM;
};

} // namespace fr

#endif // FR_FBCPP_STATEMENT_H
