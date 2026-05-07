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
#include "engine/db/fbcpp/FbCppBlob.h"
#include "core/StringUtils.h"

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

fbcpp::Client& FbCppDatabase::getClient()
{
    static std::optional<fbcpp::Client> client;
    if (!client)
    {
        Firebird::IMaster* master = fb_get_master_interface();
        if (!master)
            throw std::runtime_error("Failed to get Firebird master interface");
        client.emplace(master);
    }
    return *client;
}

void FbCppDatabase::connect()
{
    auto options = fbcpp::AttachmentOptions()
        .setConnectionCharSet(charsetM)
        .setUserName(userM)
        .setPassword(passwordM)
        .setRole(roleM);

    attachmentM.emplace(getClient(), connStrM, options);
}

void FbCppDatabase::disconnect()
{
    attachmentM.reset();
}

bool FbCppDatabase::isConnected()
{
    return attachmentM.has_value();
}

void FbCppDatabase::create(int /*pagesize*/, int dialect, const std::string& owner,
    const std::string& initialUser)
{
    auto options = fbcpp::AttachmentOptions()
        .setConnectionCharSet(charsetM)
        .setUserName(userM)
        .setPassword(passwordM)
        .setRole(roleM)
        .setSqlDialect(static_cast<uint32_t>(dialect))
        .setOwner(owner)
        .setInitialUser(initialUser)
        .setCreateDatabase(true);

    attachmentM.emplace(getClient(), connStrM, options);
}

void FbCppDatabase::drop()
{
    if (attachmentM)
    {
        attachmentM->dropDatabase();
        attachmentM.reset();
    }
}

int FbCppDatabase::getDialect()
{
    if (!attachmentM)
        return 3;

    auto& client = attachmentM->getClient();
    fbcpp::impl::StatusWrapper status(client);
    unsigned char item = isc_info_db_sql_dialect;
    unsigned char buffer[16];

    try
    {
        attachmentM->getHandle()->getInfo(&status, 1, &item, sizeof(buffer), buffer);
        if (status.getState() & Firebird::IStatus::STATE_ERRORS)
            return 3;

        if (buffer[0] == isc_info_db_sql_dialect)
        {
            int len = buffer[1] | (buffer[2] << 8);
            if (len == 1)
                return buffer[3];
            if (len == 4)
                return buffer[3] | (buffer[4] << 8) | (buffer[5] << 16) | (buffer[6] << 24);
        }
    }
    catch (...)
    {
    }
    return 3;
}

std::string FbCppDatabase::getUserPassword()
{
    return passwordM;
}

std::string FbCppDatabase::getUsername()
{
    return userM;
}

std::string FbCppDatabase::getRole()
{
    return roleM;
}

void FbCppDatabase::getConnectedUsers(std::vector<std::string>& users)
{
    users.clear();
    if (!attachmentM)
        return;

    try
    {
        auto tr = createTransaction();
        tr->start();
        auto st = createStatement(tr);
        st->prepare("SELECT DISTINCT MON$USER FROM MON$ATTACHMENTS");
        st->execute();
        while (st->fetch())
        {
            std::string user = st->getString(0);
            // Trim trailing spaces
            size_t last = user.find_last_not_of(" ");
            if (last != std::string::npos)
                user.erase(last + 1);
            else if (user.size() > 0 && user[0] == ' ')
                user.clear();
            users.push_back(user);
        }
        tr->commit();
    }
    catch (...)
    {
    }
}

std::string FbCppDatabase::getEngineVersion()
{
    if (!attachmentM)
        return "";

    auto& client = attachmentM->getClient();
    fbcpp::impl::StatusWrapper status(client);
    unsigned char item = isc_info_version;
    unsigned char buffer[256];

    try
    {
        attachmentM->getHandle()->getInfo(&status, 1, &item, sizeof(buffer), buffer);
        if (status.getState() & Firebird::IStatus::STATE_ERRORS)
            return "";

        if (buffer[0] == isc_info_version)
        {
            int len = buffer[1] | (buffer[2] << 8);
            return std::string((char*)&buffer[3], len);
        }
    }
    catch (...)
    {
    }
    return "";
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
    return std::make_shared<FbCppStatement>(shared_from_this(), tr, *attachmentM, fbTr->getFbCppTransaction());
}

std::string FbCppDatabase::getTimezoneName(int timezoneId)
{
    try
    {
        ISC_TIME_TZ iscTmTz = {};
        iscTmTz.time_zone = static_cast<ISC_USHORT>(timezoneId);
        char tzBuf[64] = {}; // FB_MAX_TIME_ZONE_NAME_LENGTH is 64
        unsigned dummyHour = 0, dummyMinute = 0, dummySecond = 0, dummyFractions = 0;
        
        auto status = getClient().newStatus();
        Firebird::ThrowStatusWrapper statusWrapper(status.get());
        getClient().getUtil()->decodeTimeTz(&statusWrapper, &iscTmTz,
            &dummyHour, &dummyMinute, &dummySecond, &dummyFractions,
            sizeof(tzBuf), tzBuf);
        
        return std::string(tzBuf);
    }
    catch (...)
    {
        return "";
    }
}

void FbCppDatabase::getInfo(DatabaseInfoData* data)
{
    if (!data || !attachmentM)
        return;

    *data = {};
    auto tr = createTransaction();
    tr->start();
    auto st = createStatement(tr);

    auto& client = attachmentM->getClient();
    fbcpp::impl::StatusWrapper status(client);

    // Get basic DB info
    unsigned char items[] = {
        isc_info_ods_version,
        isc_info_ods_minor_version,
        isc_info_page_size,
        isc_info_allocation,
        isc_info_db_read_only,
        isc_info_oldest_transaction,
        isc_info_oldest_active,
        isc_info_oldest_snapshot,
        isc_info_next_transaction,
        fb_info_crypt_state
    };
    unsigned char buffer[256];
    attachmentM->getHandle()->getInfo(&status, sizeof(items), items, sizeof(buffer), buffer);
    if (!(status.getState() & Firebird::IStatus::STATE_ERRORS))
    {
        unsigned char* p = buffer;
        while (*p != isc_info_end && p < buffer + sizeof(buffer))
        {
            unsigned char item = *p++;
            unsigned short len = p[0] | (p[1] << 8);
            p += 2;
            int64_t value = 0;
            if (len == 1) value = *p;
            else if (len == 2) value = p[0] | (p[1] << 8);
            else if (len == 4) value = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
            else if (len == 8) value = (int64_t)p[0] | ((int64_t)p[1] << 8) | ((int64_t)p[2] << 16) | ((int64_t)p[3] << 24)
                                    | ((int64_t)p[4] << 32) | ((int64_t)p[5] << 40) | ((int64_t)p[6] << 48) | ((int64_t)p[7] << 56);

            if (item == isc_info_ods_version) data->ods = (int)value;
            else if (item == isc_info_ods_minor_version) data->odsMinor = (int)value;
            else if (item == isc_info_page_size) data->pageSize = (int)value;
            else if (item == isc_info_allocation) data->pages = (int)value;
            else if (item == isc_info_db_read_only) data->readOnly = (value != 0);
            else if (item == isc_info_oldest_transaction) data->oldestTransaction = (int)value;
            else if (item == isc_info_oldest_active) data->oldestActiveTransaction = (int)value;
            else if (item == isc_info_oldest_snapshot) data->oldestSnapshot = (int)value;
            else if (item == isc_info_next_transaction) data->nextTransaction = (int)value;
            else if (item == fb_info_crypt_state) data->cryptState = (int)value;

            p += len;
        }
    }

    // Get active transactions
    try
    {
        st->prepare("SELECT MON$TRANSACTION_ID, MON$ISOLATION_MODE, MON$READ_ONLY, MON$WAIT_MODE "
                    "FROM MON$TRANSACTIONS WHERE MON$ATTACHMENT_ID = CURRENT_CONNECTION");
        st->execute();
        while (st->fetch())
        {
            TransactionInfo info;
            info.id = st->getInt32(0);
            int mode = st->getInt32(1);
            switch (mode)
            {
                case 0: info.isolationLevel = TransactionIsolationLevel::Consistency; break;
                case 1: info.isolationLevel = TransactionIsolationLevel::Concurrency; break;
                case 2: info.isolationLevel = TransactionIsolationLevel::ReadDirty; break;
                case 3: info.isolationLevel = TransactionIsolationLevel::ReadCommitted; break;
                case 4: info.isolationLevel = TransactionIsolationLevel::ReadConsistency; break;
                default: info.isolationLevel = TransactionIsolationLevel::Concurrency; break;
            }
            info.readOnly = st->getBool(2);
            info.wait = (st->getInt32(3) != 0);
            data->activeTransactions.push_back(info);
        }
    }
    catch (...) {}

    tr->commit();
}

void FbCppDatabase::getStatistics(int* fetch, int* mark, int* read, int* write, int* mem)
{
    if (fetch) *fetch = 0;
    if (mark) *mark = 0;
    if (read) *read = 0;
    if (write) *write = 0;
    if (mem) *mem = 0;
}

void FbCppDatabase::getCounts(int* ins, int* upd, int* del, int* ridx, int* rseq)
{
    if (ins) *ins = 0;
    if (upd) *upd = 0;
    if (del) *del = 0;
    if (ridx) *ridx = 0;
    if (rseq) *rseq = 0;
}

void FbCppDatabase::getDetailedCounts(std::map<int, CountInfo>& /*counts*/)
{
}

IBlobPtr FbCppDatabase::createBlob(ITransactionPtr tr)
{
    if (!attachmentM)
        throw std::runtime_error("Database not connected");
    auto fbCppTr = std::dynamic_pointer_cast<FbCppTransaction>(tr);
    if (!fbCppTr)
        throw std::runtime_error("Invalid transaction type for fb-cpp backend");
    return std::make_shared<FbCppBlob>(*attachmentM, fbCppTr->getFbCppTransaction());
}

} // namespace fr
