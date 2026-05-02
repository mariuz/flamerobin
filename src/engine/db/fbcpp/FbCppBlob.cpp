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
#include <stdexcept>

namespace fr
{

FbCppBlob::FbCppBlob(fbcpp::Attachment& attachment, fbcpp::Transaction& transaction)
    : attachmentM(attachment), transactionM(transaction)
{
}

FbCppBlob::FbCppBlob(fbcpp::Attachment& attachment, fbcpp::Transaction& transaction, const fbcpp::BlobId& blobId)
    : attachmentM(attachment), transactionM(transaction), blobIdM(blobId)
{
}

void FbCppBlob::open()
{
    if (blobIdM.isEmpty())
        throw std::runtime_error("Blob ID is empty");
    blobM.emplace(attachmentM, transactionM, blobIdM);
}

void FbCppBlob::create()
{
    blobM.emplace(attachmentM, transactionM);
    blobIdM = blobM->getId();
}

void FbCppBlob::close()
{
    if (blobM)
    {
        blobM->close();
        blobM.reset();
    }
}

void FbCppBlob::cancel()
{
    if (blobM)
    {
        blobM->cancel();
        blobM.reset();
    }
}

int FbCppBlob::read(void* buffer, int size)
{
    if (!blobM)
        throw std::runtime_error("Blob not open");
    return (int)blobM->read(std::span<char>(static_cast<char*>(buffer), size));
}

void FbCppBlob::write(const void* buffer, int size)
{
    if (!blobM)
        throw std::runtime_error("Blob not open");
    blobM->write(std::span<const char>(static_cast<const char*>(buffer), size));
}

long FbCppBlob::getLength()
{
    if (!blobM)
    {
        open();
        long len = (long)blobM->getLength();
        close();
        return len;
    }
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
