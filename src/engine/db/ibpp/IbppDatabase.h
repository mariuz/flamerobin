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

#ifndef FR_IBPP_DATABASE_H
#define FR_IBPP_DATABASE_H

#include "engine/db/IDatabase.h"
#include <ibpp.h>

namespace fr
{

class IbppDatabase : public IDatabase
{
public:
    IbppDatabase();
    virtual ~IbppDatabase() = default;

    virtual void connect() override;
    virtual void disconnect() override;
    virtual bool isConnected() override;
    virtual void create(int pagesize, int dialect) override;
    virtual void drop() override;
    virtual int getDialect() override;
    virtual std::string getUserPassword() override;
    virtual std::string getUsername() override;
    virtual std::string getRole() override;

    virtual void getConnectedUsers(std::vector<std::string>& users) override;

    virtual void setConnectionString(const std::string& connStr) override;
    virtual void setCredentials(const std::string& user, const std::string& password) override;
    virtual void setRole(const std::string& role) override;
    virtual void setCharset(const std::string& charset) override;
    virtual void setClientLibrary(const std::string& clientLib) override;
    virtual void setCryptKeyData(const std::string& cryptKeyData) override;

    virtual ITransactionPtr createTransaction() override;
    virtual IStatementPtr createStatement(ITransactionPtr tr) override;

    virtual std::string getTimezoneName(int timezoneId) override;
    virtual void getInfo(DatabaseInfoData* data) override;

    virtual DatabaseBackend getBackendType() const override { return DatabaseBackend::IBPP; }

    IBPP::Database getIBPPDatabase() { return databaseM; }

private:
    IBPP::Database databaseM;
    std::string connStrM;
    std::string userM;
    std::string passwordM;
    std::string roleM;
    std::string charsetM;
    std::string clientLibM;
    std::string cryptKeyDataM;
};

} // namespace fr

#endif // FR_IBPP_DATABASE_H
