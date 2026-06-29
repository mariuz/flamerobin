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

#include <fb-cpp/Exception.h>
#include <stdexcept>
#include <firebird/Interface.h>
#include <wx/log.h>
#if FB_CPP_USE_BOOST_DLL != 0
#include <boost/dll.hpp>
#endif

#ifndef isc_dpb_owner
#define isc_dpb_owner 102
#endif

#ifndef isc_dpb_initial_user
#define isc_dpb_initial_user 103
#endif

// Forward declaration of the Firebird entry point
// We use ISC_EXPORT to match the declaration in Interface.h and avoid redefinition errors on MSVC
extern "C" Firebird::IMaster* ISC_EXPORT fb_get_master_interface();

namespace fr
{

std::string FbCppDatabase::clientLibStaticM;

FbCppDatabase::FbCppDatabase()
{
}

std::optional<fbcpp::Client> FbCppDatabase::clientM;

bool FbCppDatabase::isClientInitialized()
{
    return clientM.has_value();
}

fbcpp::Client& FbCppDatabase::getClient()
{
    if (!clientM)
    {
#if FB_CPP_USE_BOOST_DLL != 0
        if (!clientLibStaticM.empty())
        {
            clientM.emplace(boost::dll::fs::path(clientLibStaticM));
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
    return *clientM;
}

std::vector<uint8_t> FbCppDatabase::buildDpb(bool creating, const std::string& owner,
    const std::string& initialUser)
{
    auto status = getClient().newStatus();
    fbcpp::impl::StatusWrapper statusWrapper(getClient(), status.get());
    auto dpbBuilder = fbcpp::fbUnique(getClient().getUtil()->getXpbBuilder(&statusWrapper, 
        Firebird::IXpbBuilder::DPB, nullptr, 0));

    // Force UTF8 for filenames
    dpbBuilder->insertInt(&statusWrapper, isc_dpb_utf8_filename, 1);
    
    if (creating)
    {
        if (!charsetM.empty())
            dpbBuilder->insertString(&statusWrapper, isc_dpb_set_db_charset, charsetM.c_str());
        if (!owner.empty())
            dpbBuilder->insertString(&statusWrapper, isc_dpb_owner, owner.c_str());
        if (!initialUser.empty())
            dpbBuilder->insertString(&statusWrapper, isc_dpb_initial_user, initialUser.c_str());
    }

    std::vector<uint8_t> dpb(dpbBuilder->getBufferLength(&statusWrapper));
    memcpy(dpb.data(), dpbBuilder->getBuffer(&statusWrapper), dpb.size());
    return dpb;
}

void FbCppDatabase::connect()
{
    wxLogDebug("FbCppDatabase::connect() called for: %s", connStrM.c_str());
    auto options = fbcpp::AttachmentOptions();
    if (!charsetM.empty())
        options.setConnectionCharSet(charsetM);
    if (!userM.empty())
        options.setUserName(userM);
    if (!passwordM.empty())
        options.setPassword(passwordM);
    if (!roleM.empty())
        options.setRole(roleM);

    attachmentM.emplace(getClient(), connStrM, options);
    wxLogDebug("FbCppDatabase::connect() finished.");
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
    wxLogDebug("FbCppDatabase::create() called for: %s", connStrM.c_str());
    auto options = fbcpp::AttachmentOptions();
    if (!charsetM.empty())
        options.setConnectionCharSet(charsetM);
    if (!userM.empty())
        options.setUserName(userM);
    if (!passwordM.empty())
        options.setPassword(passwordM);
    if (!roleM.empty())
        options.setRole(roleM);
    options.setSqlDialect(static_cast<uint32_t>(dialect));
    options.setCreateDatabase(true);
    options.setDpb(buildDpb(true, owner, initialUser));

    attachmentM.emplace(getClient(), connStrM, options);
    wxLogDebug("FbCppDatabase::create() finished.");
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
        tr->setAccessMode(TransactionAccessMode::Read);
        tr->start();
        {
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
        }
        try { tr->commit(); } catch (...) {}
    }
    catch (...) {}
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
    if (!clientLib.empty())
        clientLibStaticM = clientLib;
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
        fbcpp::impl::StatusWrapper statusWrapper(getClient(), status.get());
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
    tr->setAccessMode(TransactionAccessMode::Read);
    tr->setIsolationLevel(TransactionIsolationLevel::ReadCommitted);
    tr->start();

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
    // Bypassed for now because it causes hangs with the fb-cpp backend
    // due to monitoring tables interaction issues.

    try { tr->commit(); } catch (...) {}
}

void FbCppDatabase::getStatistics(int* fetch, int* mark, int* read, int* write, int* mem)
{
    if (fetch) *fetch = 0;
    if (mark) *mark = 0;
    if (read) *read = 0;
    if (write) *write = 0;
    if (mem) *mem = 0;

    if (!attachmentM)
        return;

    auto& client = attachmentM->getClient();
    fbcpp::impl::StatusWrapper status(client);

    unsigned char items[] = {
        isc_info_fetches,
        isc_info_marks,
        isc_info_reads,
        isc_info_writes,
        isc_info_current_memory
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

            if (item == isc_info_fetches && fetch) *fetch = (int)value;
            else if (item == isc_info_marks && mark) *mark = (int)value;
            else if (item == isc_info_reads && read) *read = (int)value;
            else if (item == isc_info_writes && write) *write = (int)value;
            else if (item == isc_info_current_memory && mem) *mem = (int)value;

            p += len;
        }
    }
}

void FbCppDatabase::getCounts(int* ins, int* upd, int* del, int* ridx, int* rseq)
{
    if (ins) *ins = 0;
    if (upd) *upd = 0;
    if (del) *del = 0;
    if (ridx) *ridx = 0;
    if (rseq) *rseq = 0;

    if (!attachmentM)
        return;

    auto& client = attachmentM->getClient();
    fbcpp::impl::StatusWrapper status(client);

    unsigned char items[] = {
        isc_info_insert_count,
        isc_info_update_count,
        isc_info_delete_count,
        isc_info_read_idx_count,
        isc_info_read_seq_count
    };
    unsigned char buffer[1024];
    attachmentM->getHandle()->getInfo(&status, sizeof(items), items, sizeof(buffer), buffer);
    if (!(status.getState() & Firebird::IStatus::STATE_ERRORS))
    {
        unsigned char* p = buffer;
        while (*p != isc_info_end && p < buffer + sizeof(buffer))
        {
            unsigned char item = *p++;
            unsigned short len = p[0] | (p[1] << 8);
            p += 2;
            
            int total = 0;
            unsigned char* end_ptr = p + len;
            while (p + 6 <= end_ptr)
            {
                int val = p[2] | (p[3] << 8) | (p[4] << 16) | (p[5] << 24);
                total += val;
                p += 6;
            }

            if (item == isc_info_insert_count && ins) *ins = total;
            else if (item == isc_info_update_count && upd) *upd = total;
            else if (item == isc_info_delete_count && del) *del = total;
            else if (item == isc_info_read_idx_count && ridx) *ridx = total;
            else if (item == isc_info_read_seq_count && rseq) *rseq = total;

            p = end_ptr;
        }
    }
}

void FbCppDatabase::getDetailedCounts(std::map<int, CountInfo>& counts)
{
    counts.clear();

    if (!attachmentM)
        return;

    auto& client = attachmentM->getClient();
    fbcpp::impl::StatusWrapper status(client);

    unsigned char items[] = {
        isc_info_insert_count,
        isc_info_update_count,
        isc_info_delete_count,
        isc_info_read_idx_count,
        isc_info_read_seq_count
    };
    unsigned char buffer[2048];
    attachmentM->getHandle()->getInfo(&status, sizeof(items), items, sizeof(buffer), buffer);
    if (!(status.getState() & Firebird::IStatus::STATE_ERRORS))
    {
        unsigned char* p = buffer;
        while (*p != isc_info_end && p < buffer + sizeof(buffer))
        {
            unsigned char item = *p++;
            unsigned short len = p[0] | (p[1] << 8);
            p += 2;
            
            unsigned char* end_ptr = p + len;
            while (p + 6 <= end_ptr)
            {
                int relId = p[0] | (p[1] << 8);
                int val = p[2] | (p[3] << 8) | (p[4] << 16) | (p[5] << 24);

                CountInfo& info = counts[relId];
                if (item == isc_info_insert_count) info.inserts = val;
                else if (item == isc_info_update_count) info.updates = val;
                else if (item == isc_info_delete_count) info.deletes = val;
                else if (item == isc_info_read_idx_count) info.readIndex = val;
                else if (item == isc_info_read_seq_count) info.readSequence = val;

                p += 6;
            }

            p = end_ptr;
        }
    }
}

void FbCppDatabase::getCompiledStatementInfo(std::vector<CompiledStatementInfo>& statements)
{
    statements.clear();
    if (!attachmentM)
        return;

    try
    {
        auto tr = createTransaction();
        tr->setAccessMode(TransactionAccessMode::Read);
        tr->start();
        {
            auto st = createStatement(tr);
            st->prepare("SELECT MON$COMPILED_STATEMENT_ID, MON$SQL_TEXT, MON$CACHE_HIT, MON$CACHE_MISS "
                        "FROM MON$COMPILED_STATEMENTS "
                        "ORDER BY MON$COMPILED_STATEMENT_ID");
            st->execute();
            while (st->fetch())
            {
                CompiledStatementInfo info;
                info.id = st->getInt64(0);
                info.sqlText = st->getString(1);
                info.cacheHit = st->getInt32(2);
                info.cacheMiss = st->getInt32(3);
                statements.push_back(info);
            }
        }
        try { tr->commit(); } catch (...) {}
    }
    catch (...)
    {
    }
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

extern "C" bool fbcpp_is_client_initialized()
{
    return fr::FbCppDatabase::isClientInitialized();
}
