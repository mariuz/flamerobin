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

#ifndef FR_IDATABASE_H
#define FR_IDATABASE_H

#include "engine/db/DatabaseBackend.h"
#include <memory>
#include <map>

namespace fr
{

class IDatabase : public std::enable_shared_from_this<IDatabase>
{
public:
    virtual ~IDatabase() = default;

    virtual void connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() = 0;
    virtual void create(int pagesize, int dialect) = 0;
    virtual void drop() = 0;
    virtual int getDialect() = 0;
    virtual std::string getUserPassword() = 0;
    virtual std::string getUsername() = 0;
    virtual std::string getRole() = 0;

    virtual void getConnectedUsers(std::vector<std::string>& users) = 0;
    virtual std::string getEngineVersion() = 0;

    virtual void setConnectionString(const std::string& connStr) = 0;
    virtual void setCredentials(const std::string& user, const std::string& password) = 0;
    virtual void setRole(const std::string& role) = 0;
    virtual void setCharset(const std::string& charset) = 0;
    virtual void setClientLibrary(const std::string& clientLib) = 0;
    virtual void setCryptKeyData(const std::string& cryptKeyData) = 0;

    virtual ITransactionPtr createTransaction() = 0;
    virtual IStatementPtr createStatement(ITransactionPtr tr) = 0;

    virtual std::string getTimezoneName(int timezoneId) = 0;
    virtual void getInfo(DatabaseInfoData* data) = 0;

    virtual void getStatistics(int* fetch, int* mark, int* read, int* write, int* mem) = 0;
    virtual void getCounts(int* ins, int* upd, int* del, int* ridx, int* rseq) = 0;
    virtual void getDetailedCounts(std::map<int, CountInfo>& counts) = 0;

    virtual IBlobPtr createBlob(ITransactionPtr tr) = 0;

    virtual DatabaseBackend getBackendType() const = 0;
};

} // namespace fr

#endif // FR_IDATABASE_H
