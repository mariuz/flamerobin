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

#ifndef FR_ISTATEMENT_H
#define FR_ISTATEMENT_H

#include <string>
#include <cstdint>
#include <optional>
#include "engine/db/DatabaseBackend.h"

namespace fr
{

class IStatement
{
public:
    virtual ~IStatement() = default;

    virtual void prepare(const std::string& sql) = 0;
    virtual std::string getSql() const = 0;
    virtual void execute() = 0;
    virtual bool fetch() = 0;
    virtual void close() = 0;

    // Parameter binding
    virtual void setNull(int index) = 0;
    virtual void setString(int index, const std::string& value) = 0;
    virtual void setInt32(int index, int32_t value) = 0;
    virtual void setInt64(int index, int64_t value) = 0;
    virtual void setDouble(int index, double value) = 0;
    virtual void setBool(int index, bool value) = 0;
    virtual void setDate(int index, int year, int month, int day) = 0;
    virtual void setTime(int index, int hour, int minute, int second, int fraction) = 0;
    virtual void setTimestamp(int index, int year, int month, int day,
        int hour, int minute, int second, int fraction) = 0;
    virtual void setBytes(int index, const void* data, int size) = 0;

    // Result fetching (0-based)
    virtual bool isNull(int index) = 0;
    virtual std::string getString(int index) = 0;
    virtual int32_t getInt32(int index) = 0;
    virtual int64_t getInt64(int index) = 0;
    virtual double getDouble(int index) = 0;
    virtual bool getBool(int index) = 0;

    // Binary and BLOB support
    virtual void getBytes(int index, void* data, int size) = 0;
    virtual IBlobPtr getBlob(int index) = 0;
    virtual void setBlob(int index, IBlobPtr blob) = 0;

    // Firebird specific types (as strings for common exchange)
    virtual std::string getDate(int index) = 0;
    virtual std::string getTime(int index) = 0;
    virtual std::string getTimestamp(int index) = 0;
    virtual std::string getTimeTz(int index) = 0;
    virtual std::string getTimestampTz(int index) = 0;

    virtual void getDate(int index, int& year, int& month, int& day) = 0;
    virtual void getTime(int index, int& hour, int& minute, int& second, int& fraction) = 0;
    virtual void getTimestamp(int index, int& year, int& month, int& day,
        int& hour, int& minute, int& second, int& fraction) = 0;

    virtual int getColumnCount() = 0;
    virtual std::string getColumnName(int index) = 0;
    virtual ColumnType getColumnType(int index) = 0;
    virtual int getColumnSubtype(int index) = 0;
    virtual int getColumnScale(int index) = 0;
    virtual int getColumnSize(int index) = 0;
    virtual std::string getColumnAlias(int index) = 0;
    virtual std::string getColumnTable(int index) = 0;

    virtual std::string getPlan() = 0;
    virtual StatementType getType() = 0;
    virtual int getParameterCount() = 0;
    virtual std::string getParameterName(int index) = 0;
    virtual std::vector<int> findParameterIndicesByName(const std::string& name) = 0;
    virtual ColumnType getParameterType(int index) = 0;
    virtual int getParameterSubtype(int index) = 0;
    virtual int getParameterScale(int index) = 0;
    virtual int getParameterSize(int index) = 0;

    virtual int getAffectedRows() = 0;

    virtual IDatabasePtr getDatabase() = 0;
    virtual ITransactionPtr getTransaction() = 0;
};

} // namespace fr

#endif // FR_ISTATEMENT_H
