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

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>

#include "ibpp/ibpp.h"

namespace
{

std::string makeIdentifier(const std::string& prefix, char fill, size_t len)
{
    std::string id(prefix);
    if (id.length() < len)
        id.append(len - id.length(), fill);
    return id;
}

std::string quoteIdentifier(const std::string& id)
{
    return "\"" + id + "\"";
}

bool check(bool condition, const char* testName)
{
    if (condition)
        return true;
    std::cerr << testName << " failed.\n";
    return false;
}

bool checkStr(const char* actual, const std::string& expected, const char* testName)
{
    const std::string actualStr(actual ? actual : "");
    if (actualStr == expected)
        return true;
    std::cerr << testName << " failed.\n"
        << "  Expected: [" << expected << "]\n"
        << "  Actual:   [" << actualStr << "]\n";
    return false;
}

} // namespace

int main()
{
    bool ok = true;

    const char* envServer = std::getenv("IBPP_TEST_SERVER");
    const std::string serverName = envServer ? envServer : "";
    if (serverName.empty())
    {
        std::cout << "IBPP_TEST_SERVER is not set, skipping ibpp_row_metadata_test.\n";
        return 0;
    }

    const std::string dbName = "/tmp/flamerobin_row_metadata_test_" +
        std::to_string(static_cast<long long>(std::time(0))) + ".fdb";

    const std::string tableName = makeIdentifier("TBL_", 'T', 32);
    const std::string columnName = makeIdentifier("COL_", 'C', 32);
    const std::string aliasName = makeIdentifier("ALIAS_", 'A', 32);

    IBPP::Database db;

    try
    {
        db = IBPP::DatabaseFactory(serverName, dbName, "SYSDBA", "masterkey");
        db->Create(3);

        IBPP::Transaction tr = IBPP::TransactionFactory(db);
        tr->Start();

        IBPP::Statement st = IBPP::StatementFactory(db, tr);
        st->Execute("CREATE TABLE " + quoteIdentifier(tableName) + " (" +
            quoteIdentifier(columnName) + " INTEGER)");
        st->Execute("INSERT INTO " + quoteIdentifier(tableName) + " (" +
            quoteIdentifier(columnName) + ") VALUES (1)");

        IBPP::Statement query = IBPP::StatementFactory(db, tr);
        query->Prepare("SELECT t." + quoteIdentifier(columnName) + " AS " +
            quoteIdentifier(aliasName) + " FROM " + quoteIdentifier(tableName) + " t");
        query->Execute();

        ok = check(query->Fetch(), "fetch result row") && ok;
        ok = check(query->Columns() == 1, "single result column") && ok;

        ok = checkStr(query->ColumnName(1), columnName, "column name is not truncated") && ok;
        ok = checkStr(query->ColumnAlias(1), aliasName, "column alias is not truncated") && ok;
        ok = checkStr(query->ColumnTable(1), tableName, "column table is not truncated") && ok;

        ok = check(query->ColumnNum(columnName) == 1, "ColumnNum by name") && ok;
        ok = check(query->ColumnNum(aliasName) == 1, "ColumnNum by alias") && ok;
        ok = check(query->ColumnNum(makeIdentifier("alias_", 'a', 32)) == 1,
            "ColumnNum is case-insensitive for aliases") && ok;

        tr->Rollback();
        db->Drop();
    }
    catch (const IBPP::Exception& e)
    {
        std::cerr << "IBPP exception: " << e.what() << "\n";
        ok = false;
        try
        {
            if (db != 0 && db->Connected())
                db->Drop();
        }
        catch (...)
        {
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
        ok = false;
        try
        {
            if (db != 0 && db->Connected())
                db->Drop();
        }
        catch (...)
        {
        }
    }
    catch (...)
    {
        std::cerr << "Unknown exception.\n";
        ok = false;
        try
        {
            if (db != 0 && db->Connected())
                db->Drop();
        }
        catch (...)
        {
        }
    }

    return ok ? 0 : 1;
}
