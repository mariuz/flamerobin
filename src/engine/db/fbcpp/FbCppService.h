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

#ifndef FR_FBCPP_SERVICE_H
#define FR_FBCPP_SERVICE_H

#include "engine/db/IService.h"
#include <fb-cpp/fb-cpp.h>
#include <optional>

namespace fr
{

class FbCppService : public IService
{
public:
    FbCppService();
    virtual ~FbCppService() = default;

    virtual void connect() override;
    virtual void disconnect() override;

    virtual void setConnectionString(const std::string& connStr) override;
    virtual void setCredentials(const std::string& user, const std::string& password) override;
    virtual void setRole(const std::string& role) override;
    virtual void setCharset(const std::string& charset) override;
    virtual void setClientLibrary(const std::string& libraryPath) override;

    virtual void backup(const BackupConfig& config) override;
    virtual void restore(const RestoreConfig& config) override;

    virtual std::string getNextLine() override;

    virtual void getUsers(std::vector<UserData>& users) override;
    virtual void addUser(const UserData& user) override;
    virtual void modifyUser(const UserData& user) override;
    virtual void removeUser(const std::string& username) override;

    virtual bool versionIsHigherOrEqualTo(int major, int minor) override;
    virtual std::string getVersion() override;

private:
    std::optional<fbcpp::Client> clientM;
    std::optional<fbcpp::ServiceManager> serviceM;
    std::string connStrM;
    std::string userM;
    std::string passwordM;
    std::string roleM;
    std::string charsetM;
    std::string libraryPathM;
};

} // namespace fr

#endif // FR_FBCPP_SERVICE_H
