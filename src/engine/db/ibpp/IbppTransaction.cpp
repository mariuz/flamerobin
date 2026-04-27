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
    : modeM(TransactionAccessMode::Read), levelM(TransactionIsolationLevel::ReadCommitted)
{
    transactionM = IBPP::TransactionFactory(db);
}

void IbppTransaction::start()
{
    IBPP::TAM am = (modeM == TransactionAccessMode::Read) ? IBPP::amRead : IBPP::amWrite;
    IBPP::TIL il = IBPP::ilReadCommitted;
    if (levelM == TransactionIsolationLevel::Consistency) il = IBPP::ilConsistency;
    else if (levelM == TransactionIsolationLevel::Concurrency) il = IBPP::ilConcurrency;
    
    // In IBPP, you must attach databases before starting, but TransactionFactory(db) already attached one.
    transactionM->Start();
}

void IbppTransaction::commit()
{
    transactionM->Commit();
}

void IbppTransaction::rollback()
{
    transactionM->Rollback();
}

void IbppTransaction::commitRetain()
{
    transactionM->CommitRetain();
}

void IbppTransaction::rollbackRetain()
{
    transactionM->RollbackRetain();
}

bool IbppTransaction::isActive()
{
    return transactionM->Started();
}

void IbppTransaction::setAccessMode(TransactionAccessMode mode)
{
    modeM = mode;
}

void IbppTransaction::setIsolationLevel(TransactionIsolationLevel level)
{
    levelM = level;
}

} // namespace fr
