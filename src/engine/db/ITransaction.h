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

#ifndef FR_ITRANSACTION_H
#define FR_ITRANSACTION_H

#include "engine/db/DatabaseBackend.h"

namespace fr
{

class ITransaction
{
public:
    virtual ~ITransaction() = default;

    virtual void start() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
    virtual void commitRetain() = 0;
    virtual void rollbackRetain() = 0;

    virtual bool isActive() = 0;

    virtual void setAccessMode(TransactionAccessMode mode) = 0;
    virtual void setIsolationLevel(TransactionIsolationLevel level) = 0;
    virtual void setLockResolution(TransactionLockResolution resolution) = 0;
};

} // namespace fr

#endif // FR_ITRANSACTION_H
