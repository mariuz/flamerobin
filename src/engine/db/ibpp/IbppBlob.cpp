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

#include "engine/db/ibpp/IbppBlob.h"

namespace fr
{

IbppBlob::IbppBlob(IBPP::Database db, IBPP::Transaction tr)
{
    blobM = IBPP::BlobFactory(db, tr);
}

void IbppBlob::open()
{
    blobM->Open();
}

void IbppBlob::create()
{
    blobM->Create();
}

void IbppBlob::close()
{
    blobM->Close();
}

void IbppBlob::cancel()
{
    blobM->Cancel();
}

int IbppBlob::read(void* buffer, int size)
{
    return blobM->Read(buffer, size);
}

void IbppBlob::write(const void* buffer, int size)
{
    blobM->Write(buffer, size);
}

long IbppBlob::getLength()
{
    int size, segments, maxsegment;
    blobM->Info(&size, &segments, &maxsegment);
    return (long)size;
}

int IbppBlob::getSegmentCount()
{
    int size, segments, maxsegment;
    blobM->Info(&size, &segments, &maxsegment);
    return segments;
}

int IbppBlob::getMaxSegmentSize()
{
    int size, segments, maxsegment;
    blobM->Info(&size, &segments, &maxsegment);
    return maxsegment;
}

} // namespace fr
