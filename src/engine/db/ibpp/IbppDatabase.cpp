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

#include "engine/db/ibpp/IbppDatabase.h"
#include "engine/db/ibpp/IbppTransaction.h"
#include "engine/db/ibpp/IbppStatement.h"
#include <stdexcept>

namespace fr
{

IbppDatabase::IbppDatabase()
{
}

void IbppDatabase::connect()
{
    databaseM = IBPP::DatabaseFactory("", connStrM, userM, passwordM, roleM,
        charsetM, "", clientLibM, cryptKeyDataM);
    databaseM->Connect();
}

void IbppDatabase::disconnect()
{
    if (databaseM.intf())
        databaseM->Disconnect();
}

bool IbppDatabase::isConnected()
{
    return databaseM.intf() && databaseM->Connected();
}

void IbppDatabase::setConnectionString(const std::string& connStr)
{
    connStrM = connStr;
}

void IbppDatabase::setCredentials(const std::string& user, const std::string& password)
{
    userM = user;
    passwordM = password;
}

void IbppDatabase::setRole(const std::string& role)
{
    roleM = role;
}

void IbppDatabase::setCharset(const std::string& charset)
{
    charsetM = charset;
}

void IbppDatabase::setClientLibrary(const std::string& clientLib)
{
    clientLibM = clientLib;
}

void IbppDatabase::setCryptKeyData(const std::string& cryptKeyData)
{
    cryptKeyDataM = cryptKeyData;
}

ITransactionPtr IbppDatabase::createTransaction()
{
    return std::make_shared<IbppTransaction>(databaseM);
}

IStatementPtr IbppDatabase::createStatement(ITransactionPtr tr)
{
    auto ibppTr = std::dynamic_pointer_cast<IbppTransaction>(tr);
    if (!ibppTr)
        throw std::runtime_error("Invalid transaction type for IBPP backend");
    return std::make_shared<IbppStatement>(databaseM, ibppTr->getIBPPTransaction());
}

} // namespace fr

namespace ibpp_internals
{
    bool getTimezoneNameById(int tzId, std::string& name);
}

namespace fr
{

std::string IbppDatabase::getTimezoneName(int timezoneId)
{
    std::string name;
    if (ibpp_internals::getTimezoneNameById(timezoneId, name))
        return name;
    return "";
}

} // namespace fr
