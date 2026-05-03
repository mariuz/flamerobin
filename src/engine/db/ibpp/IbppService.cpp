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
    serviceM = IBPP::ServiceFactory(connStrM, userM, passwordM, roleM, charsetM, libraryPathM);
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

void IbppService::setRole(const std::string& role)
{
    roleM = role;
}

void IbppService::setCharset(const std::string& charset)
{
    charsetM = charset;
}

void IbppService::setClientLibrary(const std::string& libraryPath)
{
    libraryPathM = libraryPath;
}

static IBPP::BRF backupFlagsToIbpp(BackupFlags flags)
{
    int res = 0;
    if ((int)flags & (int)BackupFlags::IgnoreChecksums) res |= IBPP::brIgnoreChecksums;
    if ((int)flags & (int)BackupFlags::IgnoreLimbo) res |= IBPP::brIgnoreLimbo;
    if ((int)flags & (int)BackupFlags::MetadataOnly) res |= IBPP::brMetadataOnly;
    if ((int)flags & (int)BackupFlags::NoGarbageCollect) res |= IBPP::brNoGarbageCollect;
    if ((int)flags & (int)BackupFlags::NonTransportable) res |= IBPP::brNonTransportable;
    if ((int)flags & (int)BackupFlags::ConvertExtTables) res |= IBPP::brConvertExtTables;
    if ((int)flags & (int)BackupFlags::Expand) res |= IBPP::brExpand;
    if ((int)flags & (int)BackupFlags::OldDescriptions) res |= IBPP::brOldDescriptions;
    if ((int)flags & (int)BackupFlags::NoDBTriggers) res |= IBPP::brNoDBTriggers;
    if ((int)flags & (int)BackupFlags::Zip) res |= IBPP::brZip;
    if ((int)flags & (int)BackupFlags::Verbose) res |= IBPP::brVerbose;
    if ((int)flags & (int)BackupFlags::StatTime) res |= IBPP::brstatistics_time;
    if ((int)flags & (int)BackupFlags::StatDelta) res |= IBPP::brstatistics_delta;
    if ((int)flags & (int)BackupFlags::StatPageReads) res |= IBPP::brstatistics_pagereads;
    if ((int)flags & (int)BackupFlags::StatPageWrites) res |= IBPP::brstatistics_pagewrites;
    return (IBPP::BRF)res;
}

static IBPP::BRF restoreFlagsToIbpp(RestoreFlags flags)
{
    int res = 0;
    if ((int)flags & (int)RestoreFlags::DeactivateIndices) res |= IBPP::brDeactivateIdx;
    if ((int)flags & (int)RestoreFlags::NoShadow) res |= IBPP::brNoShadow;
    if ((int)flags & (int)RestoreFlags::NoValidityCheck) res |= IBPP::brNoValidity;
    if ((int)flags & (int)RestoreFlags::Replace) res |= IBPP::brReplace;
    if ((int)flags & (int)RestoreFlags::UseAllSpace) res |= IBPP::brUseAllSpace;
    if ((int)flags & (int)RestoreFlags::MetadataOnly) res |= IBPP::brMetadataOnly;
    if ((int)flags & (int)RestoreFlags::Verbose) res |= IBPP::brVerbose;
    if ((int)flags & (int)RestoreFlags::PerTableCommit) res |= IBPP::brPerTableCommit;
    if ((int)flags & (int)RestoreFlags::FixFssData) res |= IBPP::brFix_Fss_Data;
    if ((int)flags & (int)RestoreFlags::FixFssMetadata) res |= IBPP::brFix_Fss_Metadata;
    if ((int)flags & (int)RestoreFlags::ReadOnly) res |= IBPP::brDatabase_readonly;
    if ((int)flags & (int)RestoreFlags::StatTime) res |= IBPP::brstatistics_time;
    if ((int)flags & (int)RestoreFlags::StatDelta) res |= IBPP::brstatistics_delta;
    if ((int)flags & (int)RestoreFlags::StatPageReads) res |= IBPP::brstatistics_pagereads;
    if ((int)flags & (int)RestoreFlags::StatPageWrites) res |= IBPP::brstatistics_pagewrites;
    return (IBPP::BRF)res;
}

void IbppService::backup(const BackupConfig& config)
{
    serviceM->StartBackup(config.dbPath, config.backupPath, config.outputFile,
        config.factor, backupFlagsToIbpp(config.flags), config.cryptPlugin,
        config.keyHolder, config.keyName, config.skipData, config.includeData,
        config.interval, config.parallel);
}

void IbppService::restore(const RestoreConfig& config)
{
    serviceM->StartRestore(config.backupPath, config.dbPath, config.outputFile,
        config.pageSize, config.cacheBuffers, restoreFlagsToIbpp(config.flags),
        config.cryptPlugin, config.keyHolder, config.keyName, config.skipData,
        config.includeData, config.interval, config.parallel);
}

void IbppService::maintain(const MaintenanceConfig& config)
{
    if ((int)config.flags & (int)MaintenanceFlags::Sweep)
    {
        serviceM->Sweep(config.dbPath, config.parallel);
    }
    else
    {
        int flags = 0;
        if ((int)config.flags & (int)MaintenanceFlags::Full) flags |= IBPP::rpValidateFull;
        else if ((int)config.flags & (int)MaintenanceFlags::Validate) flags |= IBPP::rpValidatePages;
        else if ((int)config.flags & (int)MaintenanceFlags::Mend) flags |= IBPP::rpMendRecords;

        if ((int)config.flags & (int)MaintenanceFlags::ReadOnly) flags |= IBPP::rpReadOnly;
        if ((int)config.flags & (int)MaintenanceFlags::IgnoreChecksums) flags |= IBPP::rpIgnoreChecksums;
        if ((int)config.flags & (int)MaintenanceFlags::KillShadows) flags |= IBPP::rpKillShadows;

        serviceM->Repair(config.dbPath, (IBPP::RPF)flags, config.parallel);
    }
}

void IbppService::shutdown(const ShutdownConfig& config)
{
    int flags = 0;
    if (config.mode == ShutdownMode::Forced) flags = IBPP::dsForce;
    else if (config.mode == ShutdownMode::DenyTransactions) flags = IBPP::dsDenyTrans;
    else if (config.mode == ShutdownMode::DenyAttachments) flags = IBPP::dsDenyAttach;
    serviceM->Shutdown(config.dbPath, (IBPP::DSM)flags, config.timeout);
}

void IbppService::startup(const std::string& dbPath)
{
    serviceM->Restart(dbPath, (IBPP::DSM)0);
}

std::string IbppService::getNextLine()
{
    const char* line = serviceM->WaitMsg();
    return line ? line : "";
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

