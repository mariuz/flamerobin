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

#include "engine/db/fbcpp/FbCppTransaction.h"
#include "engine/db/fbcpp/FbCppDatabase.h"
#include "firebird/constants.h"
#include <wx/log.h>

namespace fr
{
FbCppTransaction::FbCppTransaction(std::shared_ptr<FbCppDatabase> database)
    : databaseM(std::move(database)),
      modeM(TransactionAccessMode::Write), 
      levelM(TransactionIsolationLevel::Concurrency),
      resolutionM(TransactionLockResolution::Wait)
{
    if (!databaseM)
        throw std::runtime_error("Database is null");
    attachmentSnapshotM = databaseM->getAttachmentPtr();
    if (!attachmentSnapshotM || !attachmentSnapshotM->isValid())
        throw std::runtime_error("Database not connected");
    attachmentGenerationM = databaseM->getAttachmentGeneration();
    transactionM = std::make_shared<fbcpp::TransactionExt>(attachmentSnapshotM->getClient());
}

void FbCppTransaction::start()
{
    if (!databaseM)
        throw std::runtime_error("Database is null");
    std::lock_guard<std::recursive_mutex> lock(databaseM->getAttachmentMutex());
    if (startedM.load(std::memory_order_acquire))
        return;

    auto att = databaseM->getAttachmentPtr();
    if (!att || !att->isValid())
        throw std::runtime_error("Database not connected");

    wxLogDebug("FbCppTransaction::start() called. Mode: %d, Level: %d, Resolution: %d",
        (int)modeM, (int)levelM, (int)resolutionM);

    auto options = fbcpp::TransactionOptions();
    if (modeM == TransactionAccessMode::Read)
        options.setAccessMode(fbcpp::TransactionAccessMode::READ_ONLY);
    else
        options.setAccessMode(fbcpp::TransactionAccessMode::READ_WRITE);

    if (levelM == TransactionIsolationLevel::Consistency)
        options.setIsolationLevel(fbcpp::TransactionIsolationLevel::CONSISTENCY);
    else if (levelM == TransactionIsolationLevel::Concurrency)
        options.setIsolationLevel(fbcpp::TransactionIsolationLevel::SNAPSHOT);
    else if (levelM == TransactionIsolationLevel::ReadCommitted)
        options.setIsolationLevel(fbcpp::TransactionIsolationLevel::READ_COMMITTED);
    else if (levelM == TransactionIsolationLevel::ReadConsistency)
    {
        // Manual TPB for Read Consistency since fb-cpp doesn't support it in high-level API yet.
        // We set the version and the mandatory tags for Read Committed Read Consistency.
        // Other options (access mode, lock resolution) will be added by fb-cpp's buildTpb.
        options.setTpb({isc_tpb_version3, isc_tpb_read_committed, isc_tpb_read_consistency});
    }
    else if (levelM == TransactionIsolationLevel::ReadDirty)
    {
        options.setIsolationLevel(fbcpp::TransactionIsolationLevel::READ_COMMITTED);
        options.setReadCommittedMode(fbcpp::TransactionReadCommittedMode::RECORD_VERSION);
    }

    if (levelM != TransactionIsolationLevel::ReadConsistency)
    {
        int odsVersion = 0;
        try
        {
            auto& client = att->getClient();
            fbcpp::impl::StatusWrapper status(client);
            unsigned char item = isc_info_ods_version;
            unsigned char buffer[256];
            att->getHandle()->getInfo(&status, 1, &item, sizeof(buffer), buffer);
            if (!(status.getState() & Firebird::IStatus::STATE_ERRORS))
            {
                if (buffer[0] == isc_info_ods_version)
                {
                    unsigned short len = buffer[1] | (buffer[2] << 8);
                    if (len == 1) odsVersion = buffer[3];
                    else if (len == 2) odsVersion = buffer[3] | (buffer[4] << 8);
                    else if (len == 4) odsVersion = buffer[3] | (buffer[4] << 8) | (buffer[5] << 16) | (buffer[6] << 24);
                }
            }
        }
        catch (...)
        {
        }

        if (odsVersion > 0 && odsVersion < 12)
        {
            // For Firebird 2.5 and older (ODS version < 12), we must use isc_tpb_version1
            // to maintain backward compatibility.
            options.setTpb({isc_tpb_version1});
        }
    }

    if (resolutionM == TransactionLockResolution::Wait)
        options.setWaitMode(fbcpp::TransactionWaitMode::WAIT);
    else
        options.setWaitMode(fbcpp::TransactionWaitMode::NO_WAIT);

    wxLogDebug("FbCppTransaction::start() initializing fbcpp::Transaction.");
    // Shared ownership ensures any older FbCppStatement / FbCppBlob holding a
    // snapshot of the previous TransactionExt never sees a freed C++ reference (#779).
    auto nextTx = std::make_shared<fbcpp::TransactionExt>(att->getClient());
    nextTx->start(*att, options);
    attachmentSnapshotM = std::move(att);
    attachmentGenerationM = databaseM->getAttachmentGeneration();
    transactionM = std::move(nextTx);
    generationM.fetch_add(1, std::memory_order_release);
    startedM.store(true, std::memory_order_release);
    wxLogDebug("FbCppTransaction::start() finished.");
}

void FbCppTransaction::commit()
{
    if (!databaseM)
        return;
    std::lock_guard<std::recursive_mutex> lock(databaseM->getAttachmentMutex());
    if (transactionM && startedM.load(std::memory_order_acquire))
    {
        wxLogDebug("FbCppTransaction::commit() called.");
        try
        {
            transactionM->commit();
            startedM.store(false, std::memory_order_release);
            generationM.fetch_add(1, std::memory_order_release);
            wxLogDebug("FbCppTransaction::commit() finished.");
        }
        catch (...)
        {
            startedM.store(false, std::memory_order_release);
            generationM.fetch_add(1, std::memory_order_release);
            // the handle may be dead (e.g. attachment aborted): replace with fresh shared_ptr
            try
            {
                if (attachmentSnapshotM)
                    transactionM = std::make_shared<fbcpp::TransactionExt>(attachmentSnapshotM->getClient());
            }
            catch (...) {}
            throw;
        }
    }
}

void FbCppTransaction::rollback()
{
    if (!databaseM)
        return;
    std::lock_guard<std::recursive_mutex> lock(databaseM->getAttachmentMutex());
    if (transactionM && startedM.load(std::memory_order_acquire))
    {
        try
        {
            transactionM->rollback();
            startedM.store(false, std::memory_order_release);
            generationM.fetch_add(1, std::memory_order_release);
        }
        catch (...)
        {
            startedM.store(false, std::memory_order_release);
            generationM.fetch_add(1, std::memory_order_release);
            // the handle may be dead (e.g. attachment aborted): replace with fresh shared_ptr
            try
            {
                if (attachmentSnapshotM)
                    transactionM = std::make_shared<fbcpp::TransactionExt>(attachmentSnapshotM->getClient());
            }
            catch (...) {}
            throw;
        }
    }
}

void FbCppTransaction::commitRetain()
{
    if (!databaseM)
        return;
    std::lock_guard<std::recursive_mutex> lock(databaseM->getAttachmentMutex());
    if (transactionM && startedM.load(std::memory_order_acquire))
        transactionM->commitRetaining();
}

void FbCppTransaction::rollbackRetain()
{
    if (!databaseM)
        return;
    std::lock_guard<std::recursive_mutex> lock(databaseM->getAttachmentMutex());
    if (transactionM && startedM.load(std::memory_order_acquire))
        transactionM->rollbackRetaining();
}

bool FbCppTransaction::isActive()
{
    return startedM.load(std::memory_order_acquire);
}

bool FbCppTransaction::isValidForGeneration(uint64_t expectedGen) const
{
    if (!startedM.load(std::memory_order_acquire))
        return false;
    if (generationM.load(std::memory_order_acquire) != expectedGen)
        return false;
    if (!transactionM || !transactionM->isValid())
        return false;
    if (!databaseM || !attachmentSnapshotM || !attachmentSnapshotM->isValid())
        return false;
    return databaseM->getAttachmentGeneration() == attachmentGenerationM;
}

void FbCppTransaction::setAccessMode(TransactionAccessMode mode)
{
    modeM = mode;
}

void FbCppTransaction::setIsolationLevel(TransactionIsolationLevel level)
{
    levelM = level;
}

void FbCppTransaction::setLockResolution(TransactionLockResolution resolution)
{
    resolutionM = resolution;
}

fbcpp::Transaction& FbCppTransaction::getFbCppTransaction()
{
    if (!transactionM)
        throw std::runtime_error("Transaction not started");
    return *transactionM;
}

} // namespace fr
