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

#include "engine/db/ibpp/IbppService.h"

namespace fr
{

IbppService::IbppService()
{
}

void IbppService::connect()
{
    serviceM = IBPP::ServiceFactory(connStrM, userM, passwordM, "", "");
    serviceM->Connect();
}

void IbppService::disconnect()
{
    if (serviceM.intf())
        serviceM->Disconnect();
}

void IbppService::setConnectionString(const std::string& connStr)
{
    connStrM = connStr;
}

void IbppService::setCredentials(const std::string& user, const std::string& password)
{
    userM = user;
    passwordM = password;
}

void IbppService::backup(const std::string& dbPath, const std::string& backupPath)
{
    serviceM->StartBackup(dbPath, backupPath);
}

void IbppService::restore(const std::string& backupPath, const std::string& dbPath)
{
    serviceM->StartRestore(backupPath, dbPath);
}

void IbppService::getUsers(std::vector<UserData>& users)
{
    std::vector<IBPP::User> ibppUsers;
    serviceM->GetUsers(ibppUsers);
    for (const auto& u : ibppUsers)
    {
        UserData ud;
        ud.username = u.username;
        ud.password = u.password;
        ud.firstName = u.firstname;
        ud.middleName = u.middlename;
        ud.lastName = u.lastname;
        ud.userId = u.userid;
        ud.groupId = u.groupid;
        users.push_back(ud);
    }
}

static void userDataToIbpp(const UserData& src, IBPP::User& dest)
{
    dest.username = src.username;
    dest.password = src.password;
    dest.firstname = src.firstName;
    dest.middlename = src.middleName;
    dest.lastname = src.lastName;
    dest.userid = src.userId;
    dest.groupid = src.groupId;
}

void IbppService::addUser(const UserData& user)
{
    IBPP::User u;
    userDataToIbpp(user, u);
    serviceM->AddUser(u);
}

void IbppService::modifyUser(const UserData& user)
{
    IBPP::User u;
    userDataToIbpp(user, u);
    serviceM->ModifyUser(u);
}

void IbppService::removeUser(const std::string& username)
{
    serviceM->RemoveUser(username);
}

bool IbppService::versionIsHigherOrEqualTo(int major, int minor)
{
    return serviceM->versionIsHigherOrEqualTo(major, minor);
}

std::string IbppService::getVersion()
{
    std::string version;
    serviceM->GetVersion(version);
    return version;
}

} // namespace fr

