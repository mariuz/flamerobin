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

FbCppService::~FbCppService()
{
    if (serviceThreadM.joinable())
        serviceThreadM.join();
}

void FbCppService::pushLine(std::string_view line)
{
    std::lock_guard<std::mutex> lock(queueMutexM);
    outputQueueM.push(std::string(line));
}

void FbCppService::runService(std::function<void()> func)
{
    if (serviceThreadM.joinable())
        serviceThreadM.join();

    {
        std::lock_guard<std::mutex> lock(queueMutexM);
        while (!outputQueueM.empty())
            outputQueueM.pop();
    }

    serviceThreadM = std::thread(func);
}

void FbCppService::backup(const BackupConfig& config)
{
    if (!clientM)
        connect();

    auto options = fbcpp::BackupOptions()
        .setDatabase(config.dbPath)
        .addBackupFile(config.backupPath)
        .setVerboseOutput([this](std::string_view line) { pushLine(line); });
    
    if (config.parallel > 0)
        options.setParallelWorkers(static_cast<uint32_t>(config.parallel));

    runService([this, options]() {
        try
        {
            fbcpp::BackupManager manager(*clientM, fbcpp::ServiceManagerOptions()
                .setServer(connStrM)
                .setUserName(userM)
                .setPassword(passwordM));
            manager.backup(options);
        }
        catch (const std::exception& e)
        {
            pushLine(std::string("Error during backup: ") + e.what());
        }
        pushLine(""); // EOF marker
    });
}

void FbCppService::restore(const RestoreConfig& config)
{
    if (!clientM)
        connect();

    auto options = fbcpp::RestoreOptions()
        .setDatabase(config.dbPath)
        .addBackupFile(config.backupPath)
        .setReplace((int)config.flags & (int)RestoreFlags::Replace)
        .setVerboseOutput([this](std::string_view line) { pushLine(line); });

    if (config.parallel > 0)
        options.setParallelWorkers(static_cast<uint32_t>(config.parallel));

    runService([this, options]() {
        try
        {
            fbcpp::BackupManager manager(*clientM, fbcpp::ServiceManagerOptions()
                .setServer(connStrM)
                .setUserName(userM)
                .setPassword(passwordM));
            manager.restore(options);
        }
        catch (const std::exception& e)
        {
            pushLine(std::string("Error during restore: ") + e.what());
        }
        pushLine(""); // EOF marker
    });
}

void FbCppService::maintain(const MaintenanceConfig& config)
{
    throw std::runtime_error("Maintenance not implemented yet in FbCppService");
}

void FbCppService::shutdown(const ShutdownConfig& config)
{
    // Firebird 3.0+ shutdown using service manager is complex via low-level API.
    // fb-cpp doesn't have a direct wrapper yet, so we use a stub for now.
    // In a real implementation, we would use the low-level Service API.
    throw std::runtime_error("Shutdown not implemented yet in FbCppService");
}

void FbCppService::startup(const std::string& /*dbPath*/)
{
    throw std::runtime_error("Startup not implemented yet in FbCppService");
}

std::string FbCppService::getNextLine()
{
    std::lock_guard<std::mutex> lock(queueMutexM);
    if (outputQueueM.empty())
        return "";
    std::string line = outputQueueM.front();
    outputQueueM.pop();
    return line;
}

void FbCppService::getUsers(std::vector<UserData>& users)
{
    // Firebird user management via services uses a specific set of SPB items.
    // Since fb-cpp doesn't wrap this, we would need to go low-level.
    // For now, we'll keep it as a TODO or implement a basic version if possible.
    users.clear();
}

void FbCppService::addUser(const UserData& /*user*/)
{
}

void FbCppService::modifyUser(const UserData& /*user*/)
{
}

void FbCppService::removeUser(const std::string& /*username*/)
{
}

bool FbCppService::versionIsHigherOrEqualTo(int major, int minor)
{
    // Mocking for now, could be implemented using ServiceManager::getInfo
    return true; 
}

std::string FbCppService::getVersion()
{
    return "Firebird (fb-cpp)";
}

} // namespace fr
