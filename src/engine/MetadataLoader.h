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

#ifndef FR_METADATALOADER_H
#define FR_METADATALOADER_H

#include <list>
#include <string>

#include <ibpp.h>

class Database;
class MetadataLoaderTransaction;

class MetadataLoader
{
private:
    typedef std::list<IBPP::Statement> IBPPStatementList;
    typedef std::list<IBPP::Statement>::iterator IBPPStatementListIterator;

    friend class MetadataLoaderTransaction;

    IBPP::Database databaseM;
    IBPP::Transaction transactionM;
    unsigned transactionLevelM;

    std::list<IBPP::Statement> statementsM;
    unsigned maxStatementsM;
    // Returns an iterator to the prepared IBPP::Statement object
    // for the sql statement if available, else returns statementsM.end().
    IBPPStatementListIterator findStatement(const std::string& sql);
    // Releases any assigned statement objects beyond the list size limit.
    void limitListSize();

    // A read-only transaction is used to read metadata from the database.
    // The first call of transactionStart() starts the transaction, further
    // calls only increment transactionLevelM.  Calls to transactionCommit()
    // decrease transactionLevelM, when it reaches 0 the transaction itself
    // is committed.
    // Methods are private, use of MetadataLoaderTransaction class is
    // exception-safe and allows for proper synchronization with locks
    // on metadata items (first unlock the object, then commit transaction)
    void transactionStart();
    void transactionCommit();
    bool transactionStarted();

public:
    // Creates MetadataLoader object for the database, which will use
    // a maximum of maxStatements assigned IBPP::Statement objects to
    // improve load times of metadata items.
    // Setting the parameter maxStatements to 0 will disable the size limit
    // of the statementsM list, and could possibly consume a lot of the
    // available server ressources!
    MetadataLoader(Database& database, unsigned maxStatements = 1);

    // Creates a prepared IBPP::Statement object for the sql statement.
    // Should be used in cases where sql is unique and can not be reused,
    // but should still use the same transaction.  This way the cached
    // statements will not be replaced.
    IBPP::Statement createStatement(const std::string& sql);
    // returns a reference to a prepared IBPP::Statement object for the
    // sql statement, either recycled, newly created, or newly prepared
    // using the least recently used IBPP::Statement in statementsM
    IBPP::Statement& getStatement(const std::string& sql);
    // releases any assigned IBPP::Statement objects while keeping the maximum
    // number of objects untouched
    void releaseStatements();
    // readjusts maximum number of IBPP::Statement objects in statementsM,
    // releasing any assigned statement objects beyond the new limit
    // setting the parameter count to 0 will disable the size limit of the
    // statementsM list, and could possibly consume a lot of the available
    // server ressources!
    void setMaximumConcurrentStatements(unsigned count);

    // Creates an IBPP::Blob object using the database and transaction
    IBPP::Blob createBlob();
};

class MetadataLoaderTransaction
{
private:
    MetadataLoader* loaderM;
public:
    MetadataLoaderTransaction(MetadataLoader* loader);
    ~MetadataLoaderTransaction();
};

#endif //FR_METADATALOADER_H
