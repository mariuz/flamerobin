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

#include "engine/db/ibpp/IbppDatabase.h"
#include "engine/db/ibpp/IbppTransaction.h"
#include "engine/db/ibpp/IbppStatement.h"
#include "engine/db/ibpp/IbppBlob.h"
#include <stdexcept>

namespace fr
{

IbppDatabase::IbppDatabase()
{
}

void IbppDatabase::connect()
{
    databaseM = IBPP::DatabaseFactory("", connStrM, userM, passwordM, roleM,
        charsetM, "", clientLibM, cryptKeyDataM);
    databaseM->Connect();
}

void IbppDatabase::disconnect()
{
    if (databaseM.intf())
        databaseM->Disconnect();
}

bool IbppDatabase::isConnected()
{
    return databaseM != 0 && databaseM->Connected();
}

void IbppDatabase::create(int pagesize, int dialect, const std::string& owner,
    const std::string& initialUser)
{
    std::string extra;
    if (pagesize > 0)
        extra = "PAGE_SIZE " + std::to_string(pagesize);
    if (!charsetM.empty())
    {
        if (!extra.empty()) extra += " ";
        extra += "DEFAULT CHARACTER SET " + charsetM;
    }
    if (!owner.empty())
    {
        if (!extra.empty()) extra += " ";
        extra += "OWNER '" + owner + "'";
    }
    if (!initialUser.empty())
    {
        if (!extra.empty()) extra += " ";
        extra += "INITIAL USER '" + initialUser + "'";
    }

    databaseM = IBPP::DatabaseFactory(clientLibM, connStrM, userM, passwordM,
        roleM, charsetM, extra, cryptKeyDataM);
    databaseM->Create(dialect);
}

void IbppDatabase::drop()
{
    if (databaseM != 0)
        databaseM->Drop();
}

int IbppDatabase::getDialect()
{
    if (databaseM != 0)
        return databaseM->Dialect();
    return 3;
}

std::string IbppDatabase::getUserPassword()
{
    if (databaseM != 0)
        return databaseM->UserPassword();
    return "";
}

std::string IbppDatabase::getUsername()
{
    if (databaseM != 0)
        return databaseM->Username();
    return "";
}

std::string IbppDatabase::getRole()
{
    if (databaseM != 0)
        return databaseM->RoleName();
    return "";
}

void IbppDatabase::getConnectedUsers(std::vector<std::string>& users)
{
    if (databaseM != 0)
        databaseM->Users(users);
}

std::string IbppDatabase::getEngineVersion()
{
    std::string version;
    if (databaseM != 0)
        databaseM->Version(version);
    return version;
}

void IbppDatabase::setConnectionString(const std::string& connStr)
{

    connStrM = connStr;
}

void IbppDatabase::setCredentials(const std::string& user, const std::string& password)
{
    userM = user;
    passwordM = password;
}

void IbppDatabase::setRole(const std::string& role)
{
    roleM = role;
}

void IbppDatabase::setCharset(const std::string& charset)
{
    charsetM = charset;
}

void IbppDatabase::setClientLibrary(const std::string& clientLib)
{
    clientLibM = clientLib;
}

void IbppDatabase::setCryptKeyData(const std::string& cryptKeyData)
{
    cryptKeyDataM = cryptKeyData;
}

ITransactionPtr IbppDatabase::createTransaction()
{
    return std::make_shared<IbppTransaction>(databaseM);
}

IStatementPtr IbppDatabase::createStatement(ITransactionPtr tr)
{
    auto ibppTr = std::dynamic_pointer_cast<IbppTransaction>(tr);
    if (!ibppTr)
        throw std::runtime_error("Invalid transaction type for IBPP backend");
    return std::make_shared<IbppStatement>(shared_from_this(), tr, databaseM, ibppTr->getIBPPTransaction());
}

} // namespace fr

namespace ibpp_internals
{
    bool getTimezoneNameById(int tzId, std::string& name);
}

namespace fr
{

std::string IbppDatabase::getTimezoneName(int timezoneId)
{
    std::string name;
    if (ibpp_internals::getTimezoneNameById(timezoneId, name))
        return name;
    return "";
}

void IbppDatabase::getInfo(DatabaseInfoData* data)
{
    if (!data)
        return;
    databaseM->Info(&data->ods, &data->odsMinor, &data->pageSize, &data->pages,
        &data->buffers, &data->sweep, &data->forcedWrites, &data->reserve, &data->readOnly);
    databaseM->TransactionInfo(&data->oldestTransaction, &data->oldestActiveTransaction,
        &data->oldestSnapshot, &data->nextTransaction);
    databaseM->CryptState(&data->cryptState);

    // Get active transactions
    try
    {
        ITransactionPtr tr = createTransaction();
        tr->start();
        IStatementPtr st = createStatement(tr);
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
        tr->commit();
    }
    catch (...) {}
}

void IbppDatabase::getStatistics(int* fetch, int* mark, int* read, int* write, int* mem)
{
    if (databaseM.intf())
        databaseM->Statistics(fetch, mark, read, write, mem);
}

void IbppDatabase::getCounts(int* ins, int* upd, int* del, int* ridx, int* rseq)
{
    if (databaseM.intf())
        databaseM->Counts(ins, upd, del, ridx, rseq);
}

void IbppDatabase::getDetailedCounts(std::map<int, CountInfo>& counts)
{
    if (!databaseM.intf())
        return;

    IBPP::DatabaseCounts ibppCounts;
    databaseM->DetailedCounts(ibppCounts);
    for (auto const& [relId, ibppInfo] : ibppCounts)
    {
        CountInfo info;
        info.inserts = ibppInfo.inserts;
        info.updates = ibppInfo.updates;
        info.deletes = ibppInfo.deletes;
        info.readIndex = ibppInfo.readIndex;
        info.readSequence = ibppInfo.readSequence;
        counts[relId] = info;
    }
}

IBlobPtr IbppDatabase::createBlob(ITransactionPtr tr)
{
    auto ibppTr = std::dynamic_pointer_cast<IbppTransaction>(tr);
    if (!ibppTr)
        throw std::runtime_error("Invalid transaction type for IBPP backend");
    return std::make_shared<IbppBlob>(databaseM, ibppTr->getIBPPTransaction());
}

} // namespace fr
