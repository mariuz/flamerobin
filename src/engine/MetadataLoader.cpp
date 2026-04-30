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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "engine/MetadataLoader.h"
#include "metadata/database.h"

MetadataLoader::MetadataLoader(Database& database, unsigned maxStatements)
    : databaseM(database.getDALDatabase()), transactionM(),
        transactionLevelM(0), statementsM(), maxStatementsM(maxStatements)
{
}

void MetadataLoader::transactionStart()
{
    ++transactionLevelM;

    if (transactionM != nullptr && !transactionM->isActive())
    {
        try
        {
            transactionM->start();
        }
        catch (...)
        {
            transactionM = nullptr;
        }
    }

    if (transactionM == nullptr)
    {
        transactionM = databaseM->createTransaction();
        transactionM->setAccessMode(fr::TransactionAccessMode::Read);
    }
    if (!transactionM->isActive())
        transactionM->start();
}

void MetadataLoader::transactionCommit()
{
    if (--transactionLevelM == 0 && transactionM != nullptr)
    {
        statementsM.clear();
        transactionM->commit();
        transactionM = nullptr;
    }
}

bool MetadataLoader::transactionStarted()
{
    return (transactionM != nullptr && transactionM->isActive());
}

fr::IStatementPtr MetadataLoader::createStatement(const std::string& sql)
{
    wxASSERT(transactionStarted());
    fr::IStatementPtr stmt = databaseM->createStatement(transactionM);
    stmt->prepare(sql);
    return stmt;
}

MetadataLoader::StatementListIterator MetadataLoader::findStatement(
    const std::string& sql)
{
    for (StatementListIterator it = statementsM.begin();
        it != statementsM.end(); ++it)
    {
        if ((*it)->getSql() == sql)
            return it;
    }
    return statementsM.end();
}

fr::IStatementPtr& MetadataLoader::getStatement(const std::string& sql)
{
    wxASSERT(transactionStarted());

    fr::IStatementPtr stmt;
    StatementListIterator it = findStatement(sql);
    if (it != statementsM.end())
    {
        stmt = (*it);
        statementsM.erase(it);
    }
    else
    {
        stmt = databaseM->createStatement(transactionM);
        stmt->prepare(sql);
    }
    statementsM.push_front(stmt);
    limitListSize();
    return statementsM.front();
}

void MetadataLoader::limitListSize()
{
    if (maxStatementsM)
    {
        while (statementsM.size() > maxStatementsM)
            statementsM.pop_back();
    }
}

void MetadataLoader::releaseStatements()
{
    statementsM.clear();
    if (transactionM != nullptr && transactionM->isActive())
    {
        transactionM->commit();
        transactionLevelM = 0;
    }
}

void MetadataLoader::setMaximumConcurrentStatements(unsigned count)
{
    if (maxStatementsM != count)
    {
        maxStatementsM = count;
        limitListSize();
    }
}

MetadataLoaderTransaction::MetadataLoaderTransaction(MetadataLoader* loader)
    : loaderM(loader)
{
    if (loaderM)
        loaderM->transactionStart();
}

MetadataLoaderTransaction::~MetadataLoaderTransaction()
{
    if (loaderM)
        loaderM->transactionCommit();
}
