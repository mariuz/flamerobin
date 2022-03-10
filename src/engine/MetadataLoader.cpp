/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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
    : databaseM(database.getIBPPDatabase()), transactionM(),
        transactionLevelM(0), statementsM(), maxStatementsM(maxStatements)
{
}

void MetadataLoader::transactionStart()
{
    ++transactionLevelM;

    // fix the IBPP::LogicException "No Database is attached."
    // which happens after a database reconnect
    // (this action detaches the database from all its transactions)
    if (transactionM != 0 && !transactionM->Started())
    {
        try
        {
            transactionM->Start();
        }
        catch (IBPP::LogicException&)
        {
            transactionM = 0;
        }
    }

    if (transactionM == 0)
        transactionM = IBPP::TransactionFactory(databaseM, IBPP::amRead);
    if (!transactionM->Started())
        transactionM->Start();
}

void MetadataLoader::transactionCommit()
{
    if (--transactionLevelM == 0 && transactionM != 0)
    {
        statementsM.clear();
        transactionM->Commit();
        transactionM = 0;
    }
}

bool MetadataLoader::transactionStarted()
{
    return (transactionM != 0 && transactionM->Started());
}

IBPP::Statement MetadataLoader::createStatement(const std::string& sql)
{
    wxASSERT(transactionStarted());

    return IBPP::StatementFactory(databaseM, transactionM, sql);
}

MetadataLoader::IBPPStatementListIterator MetadataLoader::findStatement(
    const std::string& sql)
{
    for (IBPPStatementListIterator it = statementsM.begin();
        it != statementsM.end(); ++it)
    {
        if ((*it)->Sql() == sql)
            return it;
    }
    return statementsM.end();
}

IBPP::Statement& MetadataLoader::getStatement(const std::string& sql)
{
    wxASSERT(transactionStarted());

    IBPP::Statement stmt;
    IBPPStatementListIterator it = findStatement(sql);
    if (it != statementsM.end())
    {
        stmt = (*it);
        statementsM.erase(it);
    }
    else
    {
        stmt = IBPP::StatementFactory(databaseM, transactionM, sql);
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
            statementsM.remove(statementsM.back());
    }
}

void MetadataLoader::releaseStatements()
{
    statementsM.clear();
    if (transactionM != 0 && transactionM->Started())
    {
        transactionM->Commit();
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

IBPP::Blob MetadataLoader::createBlob()
{
    wxASSERT(transactionStarted());

    return IBPP::BlobFactory(databaseM, transactionM);
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

