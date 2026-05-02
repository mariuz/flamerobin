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

#ifndef FR_FBCPP_BLOB_H
#define FR_FBCPP_BLOB_H

#include "engine/db/IBlob.h"
#include <fb-cpp/fb-cpp.h>
#include <optional>

namespace fr
{

class FbCppBlob : public IBlob
{
public:
    FbCppBlob(fbcpp::Attachment& attachment, fbcpp::Transaction& transaction);
    FbCppBlob(fbcpp::Attachment& attachment, fbcpp::Transaction& transaction, const fbcpp::BlobId& blobId);
    virtual ~FbCppBlob() = default;

    virtual void open() override;
    virtual void create() override;
    virtual void close() override;
    virtual void cancel() override;

    virtual int read(void* buffer, int size) override;
    virtual void write(const void* buffer, int size) override;
    
    virtual long getLength() override;
    virtual int getSegmentCount() override;
    virtual int getMaxSegmentSize() override;

    fbcpp::BlobId getBlobId() const { return blobIdM; }
    void setBlobId(const fbcpp::BlobId& blobId) { blobIdM = blobId; }

private:
    fbcpp::Attachment& attachmentM;
    fbcpp::Transaction& transactionM;
    fbcpp::BlobId blobIdM;
    std::optional<fbcpp::Blob> blobM;
};

} // namespace fr

#endif // FR_FBCPP_BLOB_H
