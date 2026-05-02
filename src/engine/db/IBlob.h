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

#ifndef FR_IBLOB_H
#define FR_IBLOB_H

#include "engine/db/DatabaseBackend.h"

namespace fr
{

class IBlob
{
public:
    virtual ~IBlob() = default;

    virtual void open() = 0;
    virtual void create() = 0;
    virtual void close() = 0;
    virtual void cancel() = 0;

    virtual int read(void* buffer, int size) = 0;
    virtual void write(const void* buffer, int size) = 0;
    
    virtual long getLength() = 0;
    virtual int getSegmentCount() = 0;
    virtual int getMaxSegmentSize() = 0;
};

} // namespace fr

#endif // FR_IBLOB_H
