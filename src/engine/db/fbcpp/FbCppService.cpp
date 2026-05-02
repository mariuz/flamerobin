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

#include "engine/db/fbcpp/FbCppService.h"
#include <stdexcept>
#include <firebird/Interface.h>

extern "C" Firebird::IMaster* ISC_EXPORT fb_get_master_interface();

namespace fr
{

FbCppService::FbCppService()
{
}

void FbCppService::connect()
{
    if (!clientM)
    {
        Firebird::IMaster* master = fb_get_master_interface();
        if (!master)
            throw std::runtime_error("Failed to get Firebird master interface");
        clientM.emplace(master);
    }

    auto options = fbcpp::ServiceManagerOptions()
        .setServer(connStrM)
        .setUserName(userM)
        .setPassword(passwordM);

    serviceM.emplace(*clientM, options);
}

void FbCppService::disconnect()
{
    serviceM.reset();
}

void FbCppService::setConnectionString(const std::string& connStr)
{
    connStrM = connStr;
}

void FbCppService::setCredentials(const std::string& user, const std::string& password)
{
    userM = user;
    passwordM = password;
}

void FbCppService::setRole(const std::string& role)
{
    roleM = role;
}

void FbCppService::setCharset(const std::string& charset)
{
    charsetM = charset;
}

void FbCppService::setClientLibrary(const std::string& libraryPath)
{
    libraryPathM = libraryPath;
}

void FbCppService::backup(const BackupConfig& /*config*/)
{
    // TODO: implement using fbcpp::BackupManager
    throw std::runtime_error("Backup not implemented yet in FbCppService");
}

void FbCppService::restore(const RestoreConfig& /*config*/)
{
    // TODO: implement using fbcpp::BackupManager
    throw std::runtime_error("Restore not implemented yet in FbCppService");
}

std::string FbCppService::getNextLine()
{
    // TODO: implement
    return "";
}

void FbCppService::getUsers(std::vector<UserData>& /*users*/)
{
    // TODO: implement using low-level API
}

void FbCppService::addUser(const UserData& /*user*/)
{
    // TODO: implement using low-level API
}

void FbCppService::modifyUser(const UserData& /*user*/)
{
    // TODO: implement using low-level API
}

void FbCppService::removeUser(const std::string& /*username*/)
{
    // TODO: implement using low-level API
}

bool FbCppService::versionIsHigherOrEqualTo(int /*major*/, int /*minor*/)
{
    // TODO: implement using ServiceManager::getInfo
    return true; 
}

std::string FbCppService::getVersion()
{
    // TODO: implement using ServiceManager::getInfo
    return "Firebird (fb-cpp)";
}

} // namespace fr
