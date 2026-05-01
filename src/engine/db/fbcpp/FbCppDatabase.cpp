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

#include "engine/db/fbcpp/FbCppDatabase.h"
#include "engine/db/fbcpp/FbCppTransaction.h"
#include "engine/db/fbcpp/FbCppStatement.h"
#include <stdexcept>
#include <firebird/Interface.h>

// Forward declaration of the Firebird entry point
// We use ISC_EXPORT to match the declaration in Interface.h and avoid redefinition errors on MSVC
extern "C" Firebird::IMaster* ISC_EXPORT fb_get_master_interface();

namespace fr
{

FbCppDatabase::FbCppDatabase()
{
}

void FbCppDatabase::connect()
{
    if (!clientM)
    {
        Firebird::IMaster* master = fb_get_master_interface();
        if (!master)
            throw std::runtime_error("Failed to get Firebird master interface");
        clientM.emplace(master);
    }

    auto options = fbcpp::AttachmentOptions()
        .setConnectionCharSet(charsetM)
        .setUserName(userM)
        .setPassword(passwordM)
        .setRole(roleM);

    attachmentM.emplace(*clientM, connStrM, options);
}

void FbCppDatabase::disconnect()
{
    attachmentM.reset();
}

bool FbCppDatabase::isConnected()
{
    return attachmentM.has_value();
}

void FbCppDatabase::setConnectionString(const std::string& connStr)
{
    connStrM = connStr;
}

void FbCppDatabase::setCredentials(const std::string& user, const std::string& password)
{
    userM = user;
    passwordM = password;
}

void FbCppDatabase::setRole(const std::string& role)
{
    roleM = role;
}

void FbCppDatabase::setCharset(const std::string& charset)
{
    charsetM = charset;
}

void FbCppDatabase::setClientLibrary(const std::string& clientLib)
{
    clientLibM = clientLib;
}

void FbCppDatabase::setCryptKeyData(const std::string& cryptKeyData)
{
    cryptKeyDataM = cryptKeyData;
}

ITransactionPtr FbCppDatabase::createTransaction()
{
    if (!attachmentM)
        throw std::runtime_error("Database not connected");
    return std::make_shared<FbCppTransaction>(*attachmentM);
}

IStatementPtr FbCppDatabase::createStatement(ITransactionPtr tr)
{
    if (!attachmentM)
        throw std::runtime_error("Database not connected");
    auto fbTr = std::dynamic_pointer_cast<FbCppTransaction>(tr);
    if (!fbTr)
        throw std::runtime_error("Invalid transaction type for fb-cpp backend");
    return std::make_shared<FbCppStatement>(*attachmentM, fbTr->getFbCppTransaction());
}

std::string FbCppDatabase::getTimezoneName(int timezoneId)
{
    if (!clientM)
        return "";

    try
    {
        ISC_TIME_TZ iscTmTz = {};
        iscTmTz.time_zone = static_cast<ISC_USHORT>(timezoneId);
        char tzBuf[64] = {}; // FB_MAX_TIME_ZONE_NAME_LENGTH is 64
        unsigned dummyHour = 0, dummyMinute = 0, dummySecond = 0, dummyFractions = 0;
        
        auto status = clientM->newStatus();
        Firebird::ThrowStatusWrapper statusWrapper(status.get());
        clientM->getUtil()->decodeTimeTz(&statusWrapper, &iscTmTz,
            &dummyHour, &dummyMinute, &dummySecond, &dummyFractions,
            sizeof(tzBuf), tzBuf);
        
        return std::string(tzBuf);
    }
    catch (...)
    {
        return "";
    }
}

} // namespace fr
