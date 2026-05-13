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

#ifndef FR_FBCPP_EXTENSIONS_H
#define FR_FBCPP_EXTENSIONS_H

#include <fb-cpp/fb-cpp.h>
#include <fb-cpp/Statement.h>
#include <fb-cpp/Transaction.h>

namespace fbcpp
{

class StatementExt : public Statement
{
public:
    StatementExt(Attachment& attachment, Transaction& transaction, std::string_view sql,
        const StatementOptions& options = {});

    void closeCursor();
};

class TransactionExt : public Transaction
{
public:
    explicit TransactionExt(Client& client);

    void start(Attachment& attachment, const TransactionOptions& options = {});
};

} // namespace fbcpp

#endif // FR_FBCPP_EXTENSIONS_H
