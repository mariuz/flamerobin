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

namespace fr
{
FbCppTransaction::FbCppTransaction(fbcpp::Attachment& attachment)
    : attachmentM(attachment), modeM(TransactionAccessMode::Write), 
      levelM(TransactionIsolationLevel::Concurrency),
      resolutionM(TransactionLockResolution::Wait)
{
}

void FbCppTransaction::start()
{
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
    else if (levelM == TransactionIsolationLevel::ReadDirty)
        options.setIsolationLevel(fbcpp::TransactionIsolationLevel::READ_COMMITTED); // fallback

    if (resolutionM == TransactionLockResolution::Wait)
        options.setWaitMode(fbcpp::TransactionWaitMode::WAIT);
    else
        options.setWaitMode(fbcpp::TransactionWaitMode::NO_WAIT);

    transactionM.emplace(attachmentM, options);
}

void FbCppTransaction::commit()
{
    if (transactionM)
    {
        transactionM->commit();
        transactionM.reset();
    }
}

void FbCppTransaction::rollback()
{
    if (transactionM)
    {
        transactionM->rollback();
        transactionM.reset();
    }
}

void FbCppTransaction::commitRetain()
{
    if (transactionM)
        transactionM->commitRetaining();
}

void FbCppTransaction::rollbackRetain()
{
    if (transactionM)
        transactionM->rollbackRetaining();
}

bool FbCppTransaction::isActive()
{
    return transactionM.has_value();
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
