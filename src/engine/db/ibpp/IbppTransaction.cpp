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

#include "engine/db/ibpp/IbppTransaction.h"

namespace fr
{
IbppTransaction::IbppTransaction(IBPP::Database db)
    : databaseM(db), modeM(TransactionAccessMode::Write), 
      levelM(TransactionIsolationLevel::Concurrency),
      resolutionM(TransactionLockResolution::Wait)
{
}

void IbppTransaction::start()
{
    IBPP::TAM am = (modeM == TransactionAccessMode::Read) ? IBPP::amRead : IBPP::amWrite;
    IBPP::TIL il = IBPP::ilConcurrency;
    if (levelM == TransactionIsolationLevel::Consistency) il = IBPP::ilConsistency;
    else if (levelM == TransactionIsolationLevel::Concurrency) il = IBPP::ilConcurrency;
    else if (levelM == TransactionIsolationLevel::ReadCommitted) il = IBPP::ilReadCommitted;
    else if (levelM == TransactionIsolationLevel::ReadDirty) il = IBPP::ilReadDirty;

    IBPP::TLR lr = (resolutionM == TransactionLockResolution::Wait) ? IBPP::lrWait : IBPP::lrNoWait;

    transactionM = IBPP::TransactionFactory(databaseM, am, il, lr);
    transactionM->Start();
}

void IbppTransaction::commit()
{
    if (transactionM.intf())
    {
        transactionM->Commit();
        transactionM = nullptr;
    }
}

void IbppTransaction::rollback()
{
    if (transactionM.intf())
    {
        transactionM->Rollback();
        transactionM = nullptr;
    }
}

void IbppTransaction::commitRetain()
{
    if (transactionM.intf())
        transactionM->CommitRetain();
}

void IbppTransaction::rollbackRetain()
{
    if (transactionM.intf())
        transactionM->RollbackRetain();
}

bool IbppTransaction::isActive()
{
    return transactionM.intf() && transactionM->Started();
}

void IbppTransaction::setAccessMode(TransactionAccessMode mode)
{
    modeM = mode;
}

void IbppTransaction::setIsolationLevel(TransactionIsolationLevel level)
{
    levelM = level;
}

void IbppTransaction::setLockResolution(TransactionLockResolution resolution)
{
    resolutionM = resolution;
}

IBPP::Transaction IbppTransaction::getIBPPTransaction()
{
    return transactionM;
}

} // namespace fr
