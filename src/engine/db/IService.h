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

#ifndef FR_ISERVICE_H
#define FR_ISERVICE_H

#include <string>
#include <vector>
#include "engine/db/DatabaseBackend.h"

namespace fr
{

class IService
{
public:
    virtual ~IService() = default;

    virtual void connect() = 0;
    virtual void disconnect() = 0;

    virtual void setConnectionString(const std::string& connStr) = 0;
    virtual void setCredentials(const std::string& user, const std::string& password) = 0;
    virtual void setRole(const std::string& role) = 0;
    virtual void setCharset(const std::string& charset) = 0;
    virtual void setClientLibrary(const std::string& libraryPath) = 0;

    virtual void backup(const BackupConfig& config) = 0;
    virtual void restore(const RestoreConfig& config) = 0;

    virtual std::string getNextLine() = 0;

    virtual void getUsers(std::vector<UserData>& users) = 0;
    virtual void addUser(const UserData& user) = 0;
    virtual void modifyUser(const UserData& user) = 0;
    virtual void removeUser(const std::string& username) = 0;

    virtual bool versionIsHigherOrEqualTo(int major, int minor) = 0;
    virtual std::string getVersion() = 0;
};

} // namespace fr

#endif // FR_ISERVICE_H
