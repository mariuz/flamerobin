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
#include <cstdio>
#include <chrono>
#include <thread>
#include <fb-cpp/BackupManager.h>
#include <fb-cpp/DatabaseManager.h>
#include <fb-cpp/Exception.h>
#if FB_CPP_USE_BOOST_DLL != 0
#include <boost/dll.hpp>
#endif

extern "C" Firebird::IMaster* ISC_EXPORT fb_get_master_interface();

namespace fr
{

static unsigned short readVal16(const unsigned char* p)
{
    return (unsigned short)(p[0] | (p[1] << 8));
}

static uint32_t readVal32(const unsigned char* p)
{
    return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
}

static void waitService(Firebird::IService* svc, fbcpp::impl::StatusWrapper* status)
{
    unsigned char request[] = { isc_info_svc_line };
    unsigned char result[1024];
    for (;;)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        svc->query(status, 0, nullptr, sizeof(request), request, sizeof(result), result);
        if (status->isDirty())
            fbcpp::impl::StatusWrapper::checkException(status);

        if (result[0] != isc_info_svc_line)
            break;

        unsigned short len = readVal16(result + 1);
        if (len == 0)
            break;
    }
}

FbCppService::FbCppService()
{
}

void FbCppService::connect()
{
    if (!clientM)
    {
#if FB_CPP_USE_BOOST_DLL != 0
        if (!libraryPathM.empty())
        {
            clientM.emplace(boost::dll::fs::path(libraryPathM));
        }
        else
#endif
        {
            Firebird::IMaster* master = fb_get_master_interface();
            if (!master)
                throw std::runtime_error("Failed to get Firebird master interface");
            clientM.emplace(master);
        }
    }

    auto options = fbcpp::ServiceManagerOptions()
        .setServer(connStrM)
        .setUserName(userM)
        .setPassword(passwordM);
    if (!roleM.empty())
        options.setRole(roleM);

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
            auto svcOptions = fbcpp::ServiceManagerOptions()
                .setServer(connStrM)
                .setUserName(userM)
                .setPassword(passwordM);
            if (!roleM.empty())
                svcOptions.setRole(roleM);
            fbcpp::BackupManager manager(*clientM, svcOptions);
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
            auto svcOptions = fbcpp::ServiceManagerOptions()
                .setServer(connStrM)
                .setUserName(userM)
                .setPassword(passwordM);
            if (!roleM.empty())
                svcOptions.setRole(roleM);
            fbcpp::BackupManager manager(*clientM, svcOptions);
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
    if (!clientM)
        connect();

    runService([this, config]() {
        try
        {
            pushLine("Running maintenance on " + config.dbPath + "...");

            auto svcOptions = fbcpp::ServiceManagerOptions()
                .setServer(connStrM)
                .setUserName(userM)
                .setPassword(passwordM);
            if (!roleM.empty())
                svcOptions.setRole(roleM);

            auto repairOptions = fbcpp::DatabaseRepairOptions()
                .setDatabase(config.dbPath)
                .setVerboseOutput([this](std::string_view line) { pushLine(line); });

            if ((int)config.flags & (int)MaintenanceFlags::Sweep)
                repairOptions.setSweep(true);
            if ((int)config.flags & (int)MaintenanceFlags::Validate)
                repairOptions.setValidate(true);
            if ((int)config.flags & (int)MaintenanceFlags::Mend)
                repairOptions.setMend(true);
            if ((int)config.flags & (int)MaintenanceFlags::IgnoreChecksums)
                repairOptions.setIgnoreChecksum(true);
            if ((int)config.flags & (int)MaintenanceFlags::KillShadows)
                repairOptions.setKillShadows(true);
            if ((int)config.flags & (int)MaintenanceFlags::Full)
                repairOptions.setFull(true);
            if ((int)config.flags & (int)MaintenanceFlags::UpgradeODS)
                repairOptions.setUpgradeDb(true);

            if (config.parallel > 0)
                repairOptions.setParallelWorkers(static_cast<uint32_t>(config.parallel));

            fbcpp::DatabaseManager manager(*clientM, svcOptions);
            manager.repair(repairOptions);

            pushLine("Maintenance done.");
        }
        catch (const std::exception& e)
        {
            pushLine(std::string("Error during maintenance: ") + e.what());
        }
        pushLine(""); // EOF marker
    });
}

void FbCppService::setReplicaMode(const std::string& dbPath, int mode)
{
    if (!clientM)
        connect();

    runService([this, dbPath, mode]() {
        try
        {
            pushLine("Setting replica mode for " + dbPath + "...");

            auto svcOptions = fbcpp::ServiceManagerOptions()
                .setServer(connStrM)
                .setUserName(userM)
                .setPassword(passwordM);
            if (!roleM.empty())
                svcOptions.setRole(roleM);

            fbcpp::ReplicaMode fbMode;
            switch (mode)
            {
                case 1:  fbMode = fbcpp::ReplicaMode::READ_ONLY;  break;
                case 2:  fbMode = fbcpp::ReplicaMode::READ_WRITE; break;
                default: fbMode = fbcpp::ReplicaMode::NONE;       break;
            }

            auto propOptions = fbcpp::DatabasePropertiesOptions()
                .setDatabase(dbPath)
                .setReplicaMode(fbMode);

            fbcpp::DatabaseManager manager(*clientM, svcOptions);
            manager.setProperties(propOptions);

            pushLine("Replica mode successfully set to " + std::to_string(mode));
        }
        catch (const std::exception& e)
        {
            pushLine(std::string("Error setting replica mode: ") + e.what());
        }
        pushLine(""); // EOF marker
    });
}

void FbCppService::shutdown(const ShutdownConfig& config)
{
    if (!clientM)
        connect();

    runService([this, config]() {
        try
        {
            pushLine("Shutting down database " + config.dbPath + "...");

            auto svcOptions = fbcpp::ServiceManagerOptions()
                .setServer(connStrM)
                .setUserName(userM)
                .setPassword(passwordM);
            if (!roleM.empty())
                svcOptions.setRole(roleM);

            fbcpp::ShutdownType fbShutdownType;
            switch (config.mode)
            {
                case ShutdownMode::DenyTransactions:
                    fbShutdownType = fbcpp::ShutdownType::DENY_TRANSACTIONS;
                    break;
                case ShutdownMode::DenyAttachments:
                    fbShutdownType = fbcpp::ShutdownType::DENY_ATTACHMENTS;
                    break;
                case ShutdownMode::Forced:
                default:
                    fbShutdownType = fbcpp::ShutdownType::FORCED;
                    break;
            }

            auto propOptions = fbcpp::DatabasePropertiesOptions()
                .setDatabase(config.dbPath)
                .setShutdownType(fbShutdownType)
                .setShutdownTimeout(config.timeout);

            fbcpp::DatabaseManager manager(*clientM, svcOptions);
            manager.setProperties(propOptions);

            pushLine("Shutdown done.");
        }
        catch (const std::exception& e)
        {
            pushLine(std::string("Error during shutdown: ") + e.what());
        }
        pushLine(""); // EOF marker
    });
}

void FbCppService::startup(const std::string& dbPath)
{
    if (!clientM)
        connect();

    runService([this, dbPath]() {
        try
        {
            pushLine("Starting up database " + dbPath + "...");

            auto svcOptions = fbcpp::ServiceManagerOptions()
                .setServer(connStrM)
                .setUserName(userM)
                .setPassword(passwordM);
            if (!roleM.empty())
                svcOptions.setRole(roleM);

            auto propOptions = fbcpp::DatabasePropertiesOptions()
                .setDatabase(dbPath)
                .setOnline(true);

            fbcpp::DatabaseManager manager(*clientM, svcOptions);
            manager.setProperties(propOptions);

            pushLine("Startup done.");
        }
        catch (const std::exception& e)
        {
            pushLine(std::string("Error during startup: ") + e.what());
        }
        pushLine(""); // EOF marker
    });
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
    users.clear();
    if (!clientM)
        connect();

    Firebird::IUtil* utl = clientM->getUtil();
    fbcpp::impl::StatusWrapper status(*clientM);
    Firebird::IXpbBuilder* spb = utl->getXpbBuilder(&status, Firebird::IXpbBuilder::SPB_START, nullptr, 0);
    if (status.isDirty())
        fbcpp::impl::StatusWrapper::checkException(&status);

    try
    {
        spb->insertTag(&status, isc_action_svc_display_user);
        if (status.isDirty())
            fbcpp::impl::StatusWrapper::checkException(&status);

        auto svc = serviceM->getHandle();
        svc->start(&status, spb->getBufferLength(&status), spb->getBuffer(&status));
        if (status.isDirty())
            fbcpp::impl::StatusWrapper::checkException(&status);
    }
    catch (...)
    {
        spb->dispose();
        throw;
    }
    spb->dispose();

    unsigned char request[] = { isc_info_svc_get_users };
    std::vector<unsigned char> result(65535);
    auto svc = serviceM->getHandle();
    svc->query(&status, 0, nullptr, sizeof(request), request, result.size(), result.data());
    if (status.isDirty())
        fbcpp::impl::StatusWrapper::checkException(&status);

    if (result[0] != isc_info_svc_get_users)
        throw std::runtime_error("Service query returned unexpected answer");

    unsigned short totalLen = (unsigned short)(result[1] | (result[2] << 8));
    const unsigned char* p = result.data();
    const unsigned char* pEnd = p + 3 + totalLen;
    p += 3; // Skip tag and length

    UserData user;
    while (p < pEnd && *p != isc_info_end)
    {
        unsigned char tag = *p;
        if (tag == isc_spb_sec_userid)
        {
            user.userId = (int)readVal32(p + 1);
            p += 5;
        }
        else if (tag == isc_spb_sec_groupid)
        {
            user.groupId = (int)readVal32(p + 1);
            p += 5;
        }
        else
        {
            unsigned short len = readVal16(p + 1);
            std::string val((const char*)(p + 3), len);
            switch (tag)
            {
                case isc_spb_sec_username:
                    if (!user.username.empty())
                    {
                        users.push_back(user);
                        user = UserData();
                    }
                    user.username = val;
                    break;
                case isc_spb_sec_password:
                    user.password = val;
                    break;
                case isc_spb_sec_firstname:
                    user.firstName = val;
                    break;
                case isc_spb_sec_middlename:
                    user.middleName = val;
                    break;
                case isc_spb_sec_lastname:
                    user.lastName = val;
                    break;
            }
            p += 3 + len;
        }
    }
    if (!user.username.empty())
        users.push_back(user);
}

void FbCppService::addUser(const UserData& user)
{
    if (!clientM)
        connect();

    Firebird::IUtil* utl = clientM->getUtil();
    fbcpp::impl::StatusWrapper status(*clientM);
    Firebird::IXpbBuilder* spb = utl->getXpbBuilder(&status, Firebird::IXpbBuilder::SPB_START, nullptr, 0);
    if (status.isDirty())
        fbcpp::impl::StatusWrapper::checkException(&status);

    try
    {
        spb->insertTag(&status, isc_action_svc_add_user);
        spb->insertString(&status, isc_spb_sec_username, user.username.c_str());
        spb->insertString(&status, isc_spb_sec_password, user.password.c_str());
        if (!user.firstName.empty())
            spb->insertString(&status, isc_spb_sec_firstname, user.firstName.c_str());
        if (!user.middleName.empty())
            spb->insertString(&status, isc_spb_sec_middlename, user.middleName.c_str());
        if (!user.lastName.empty())
            spb->insertString(&status, isc_spb_sec_lastname, user.lastName.c_str());
        if (user.userId != 0)
            spb->insertInt(&status, isc_spb_sec_userid, user.userId);
        if (user.groupId != 0)
            spb->insertInt(&status, isc_spb_sec_groupid, user.groupId);

        if (status.isDirty())
            fbcpp::impl::StatusWrapper::checkException(&status);

        auto svc = serviceM->getHandle();
        svc->start(&status, spb->getBufferLength(&status), spb->getBuffer(&status));
        if (status.isDirty())
            fbcpp::impl::StatusWrapper::checkException(&status);

        waitService(svc.get(), &status);
    }
    catch (...)
    {
        spb->dispose();
        throw;
    }
    spb->dispose();
}

void FbCppService::modifyUser(const UserData& user)
{
    if (!clientM)
        connect();

    Firebird::IUtil* utl = clientM->getUtil();
    fbcpp::impl::StatusWrapper status(*clientM);
    Firebird::IXpbBuilder* spb = utl->getXpbBuilder(&status, Firebird::IXpbBuilder::SPB_START, nullptr, 0);
    if (status.isDirty())
        fbcpp::impl::StatusWrapper::checkException(&status);

    try
    {
        spb->insertTag(&status, isc_action_svc_modify_user);
        spb->insertString(&status, isc_spb_sec_username, user.username.c_str());
        if (!user.password.empty())
            spb->insertString(&status, isc_spb_sec_password, user.password.c_str());
        if (!user.firstName.empty())
            spb->insertString(&status, isc_spb_sec_firstname, user.firstName.c_str());
        if (!user.middleName.empty())
            spb->insertString(&status, isc_spb_sec_middlename, user.middleName.c_str());
        if (!user.lastName.empty())
            spb->insertString(&status, isc_spb_sec_lastname, user.lastName.c_str());
        if (user.userId != 0)
            spb->insertInt(&status, isc_spb_sec_userid, user.userId);
        if (user.groupId != 0)
            spb->insertInt(&status, isc_spb_sec_groupid, user.groupId);

        if (status.isDirty())
            fbcpp::impl::StatusWrapper::checkException(&status);

        auto svc = serviceM->getHandle();
        svc->start(&status, spb->getBufferLength(&status), spb->getBuffer(&status));
        if (status.isDirty())
            fbcpp::impl::StatusWrapper::checkException(&status);

        waitService(svc.get(), &status);
    }
    catch (...)
    {
        spb->dispose();
        throw;
    }
    spb->dispose();
}

void FbCppService::removeUser(const std::string& username)
{
    if (!clientM)
        connect();

    Firebird::IUtil* utl = clientM->getUtil();
    fbcpp::impl::StatusWrapper status(*clientM);
    Firebird::IXpbBuilder* spb = utl->getXpbBuilder(&status, Firebird::IXpbBuilder::SPB_START, nullptr, 0);
    if (status.isDirty())
        fbcpp::impl::StatusWrapper::checkException(&status);

    try
    {
        spb->insertTag(&status, isc_action_svc_delete_user);
        spb->insertString(&status, isc_spb_sec_username, username.c_str());

        if (status.isDirty())
            fbcpp::impl::StatusWrapper::checkException(&status);

        auto svc = serviceM->getHandle();
        svc->start(&status, spb->getBufferLength(&status), spb->getBuffer(&status));
        if (status.isDirty())
            fbcpp::impl::StatusWrapper::checkException(&status);

        waitService(svc.get(), &status);
    }
    catch (...)
    {
        spb->dispose();
        throw;
    }
    spb->dispose();
}

bool FbCppService::versionIsHigherOrEqualTo(int major, int minor)
{
    std::string version = getVersion();
    // Firebird version string typically contains "V" followed by major.minor
    // Examples: "WI-V3.0.7.33374 Firebird 3.0", "LI-V4.0.0.2496 Firebird 4.0"
    size_t pos = version.find("-V");
    if (pos == std::string::npos)
        pos = version.find(" V");
    
    if (pos != std::string::npos)
    {
        int vMajor = 0, vMinor = 0;
        if (sscanf(version.c_str() + pos + 2, "%d.%d", &vMajor, &vMinor) >= 1)
        {
            if (vMajor > major) return true;
            if (vMajor < major) return false;
            return vMinor >= minor;
        }
    }
    return true; 
}

std::string FbCppService::getVersion()
{
    try
    {
        if (!serviceM)
            connect();

        auto& client = serviceM->getClient();
        fbcpp::impl::StatusWrapper status(client);

        auto receiveBuilder = fbcpp::fbUnique(client.getUtil()->getXpbBuilder(&status, Firebird::IXpbBuilder::SPB_RECEIVE, nullptr, 0));
        receiveBuilder->insertTag(&status, isc_info_svc_server_version);

        const auto receiveLength = receiveBuilder->getBufferLength(&status);
        const auto* receiveBuffer = receiveBuilder->getBuffer(&status);

        std::vector<std::uint8_t> buffer(1024);
        auto sendBuilder = fbcpp::fbUnique(client.getUtil()->getXpbBuilder(&status, Firebird::IXpbBuilder::SPB_SEND, nullptr, 0));

        serviceM->getHandle()->query(&status, sendBuilder->getBufferLength(&status),
            sendBuilder->getBuffer(&status), receiveLength, receiveBuffer, static_cast<unsigned>(buffer.size()),
            buffer.data());

        if (status.getState() & Firebird::IStatus::STATE_ERRORS)
            return "Firebird (fb-cpp)";

        // Manual parsing of the info buffer. 
        // Service info items like isc_info_svc_server_version use 2-byte length.
        const unsigned char* p = buffer.data();
        const unsigned char* end = p + buffer.size();
        while (p < end)
        {
            unsigned char item = *p++;
            if (item == isc_info_end || item == 0)
                break;
            
            // All svc info items should have a 2-byte length
            if (p + 2 > end)
                break;
            unsigned short len = p[0] | (p[1] << 8);
            p += 2;

            if (p + len > end)
                break;

            if (item == isc_info_svc_server_version)
                return std::string(reinterpret_cast<const char*>(p), len);
            
            p += len;
        }
    }
    catch (...)
    {
    }

    return "Firebird (fb-cpp)";
}

} // namespace fr
