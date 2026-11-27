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

#include "engine/db/fbcpp/FbCppBlob.h"
#include "engine/db/fbcpp/FbCppDatabase.h"
#include "engine/db/fbcpp/FbCppTransaction.h"
#include <stdexcept>

namespace fr
{

FbCppBlob::FbCppBlob(std::shared_ptr<FbCppDatabase> database,
    std::shared_ptr<FbCppTransaction> transaction)
    : databaseM(std::move(database)), transactionM(std::move(transaction))
{
    if (transactionM)
    {
        attachmentSnapshotM = transactionM->getAttachmentSnapshot();
        transactionSnapshotM = transactionM->getFbCppTransactionPtr();
        txGenerationM = transactionM->getGeneration();
    }
}

FbCppBlob::FbCppBlob(std::shared_ptr<FbCppDatabase> database,
    std::shared_ptr<FbCppTransaction> transaction,
    const fbcpp::BlobId& blobId)
    : databaseM(std::move(database)), transactionM(std::move(transaction)),
      blobIdM(blobId)
{
    if (transactionM)
    {
        attachmentSnapshotM = transactionM->getAttachmentSnapshot();
        transactionSnapshotM = transactionM->getFbCppTransactionPtr();
        txGenerationM = transactionM->getGeneration();
    }
}

FbCppBlob::~FbCppBlob() noexcept
{
    if (!blobM)
        return;
    try
    {
        if (databaseM)
        {
            std::lock_guard<std::recursive_mutex> lock(databaseM->getAttachmentMutex());
            blobM.reset();
        }
        else
        {
            blobM.reset();
        }
    }
    catch (...)
    {
    }
}

void FbCppBlob::ensureContextValid() const
{
    if (!databaseM || !transactionM
        || !transactionM->isValidForGeneration(txGenerationM)
        || !attachmentSnapshotM || !transactionSnapshotM)
    {
        throw std::runtime_error("Blob transaction is no longer active");
    }
}

void FbCppBlob::open()
{
    if (!databaseM)
        throw std::runtime_error("Blob database is null");
    std::lock_guard<std::recursive_mutex> lock(databaseM->getAttachmentMutex());
    ensureContextValid();
    if (blobIdM.isEmpty())
        throw std::runtime_error("Blob ID is empty");
    blobM.emplace(*attachmentSnapshotM, *transactionSnapshotM, blobIdM);
}

void FbCppBlob::create()
{
    if (!databaseM)
        throw std::runtime_error("Blob database is null");
    std::lock_guard<std::recursive_mutex> lock(databaseM->getAttachmentMutex());
    ensureContextValid();
    blobM.emplace(*attachmentSnapshotM, *transactionSnapshotM);
    blobIdM = blobM->getId();
}

void FbCppBlob::close()
{
    if (!blobM)
        return;
    if (!databaseM)
    {
        blobM.reset();
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(databaseM->getAttachmentMutex());
    try
    {
        if (transactionM && transactionM->isValidForGeneration(txGenerationM))
            blobM->close();
    }
    catch (...)
    {
        blobM.reset();
        throw;
    }
    blobM.reset();
}

void FbCppBlob::cancel()
{
    if (!blobM)
        return;
    if (!databaseM)
    {
        blobM.reset();
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(databaseM->getAttachmentMutex());
    try
    {
        if (transactionM && transactionM->isValidForGeneration(txGenerationM))
            blobM->cancel();
    }
    catch (...)
    {
        blobM.reset();
        throw;
    }
    blobM.reset();
}

int FbCppBlob::read(void* buffer, int size)
{
    if (!blobM || !databaseM)
        throw std::runtime_error("Blob not open");
    std::lock_guard<std::recursive_mutex> lock(databaseM->getAttachmentMutex());
    ensureContextValid();
    return (int)blobM->read(std::span<char>(static_cast<char*>(buffer), size));
}

void FbCppBlob::write(const void* buffer, int size)
{
    if (!blobM || !databaseM)
        throw std::runtime_error("Blob not open");
    std::lock_guard<std::recursive_mutex> lock(databaseM->getAttachmentMutex());
    ensureContextValid();
    blobM->write(std::span<const char>(static_cast<const char*>(buffer), size));
}

long FbCppBlob::getLength()
{
    if (!databaseM)
        throw std::runtime_error("Blob database is null");
    std::lock_guard<std::recursive_mutex> lock(databaseM->getAttachmentMutex());
    if (!blobM)
    {
        open();
        long len = 0;
        try
        {
            len = (long)blobM->getLength();
        }
        catch (...)
        {
            try { close(); } catch (...) {}
            throw;
        }
        close();
        return len;
    }
    ensureContextValid();
    return (long)blobM->getLength();
}

int FbCppBlob::getSegmentCount()
{
    // fb-cpp doesn't seem to expose segment count directly via the high-level API
    // We might need to implement it using getInfo if needed.
    return 0;
}

int FbCppBlob::getMaxSegmentSize()
{
    // Same as above
    return 0;
}

} // namespace fr
