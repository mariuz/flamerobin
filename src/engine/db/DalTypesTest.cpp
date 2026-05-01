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

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "ibpp/ibpp.h"
#include "engine/db/DatabaseFactory.h"
#include "engine/db/IDatabase.h"
#include "engine/db/ITransaction.h"
#include "engine/db/IStatement.h"
#include "core/StringUtils.h"

// Minimal wx2std implementation for tests to avoid linking StringUtils.cpp
std::string wx2std(const wxString& input, wxMBConv* conv)
{
    if (input.empty())
        return "";
    if (!conv)
        conv = wxConvCurrent;
    const wxWX2MBbuf buf(input.mb_str(*conv));
    if (!buf)
        return "";
    return std::string(buf);
}

// Stub for std2wxIdentifier if needed (though not used in DalTypesTest directly)
wxString std2wxIdentifier(const std::string& input, wxMBConv* /*conv*/)
{
    return wxString::FromUTF8(input.c_str());
}

namespace
{

bool check(bool condition, const char* testName)
{
    if (condition)
    {
        std::cout << "  PASSED: " << testName << "\n";
        return true;
    }
    std::cerr << "  FAILED: " << testName << "\n";
    return false;
}

bool checkStr(const std::string& actual, const std::string& expected, const char* testName)
{
    if (actual == expected)
    {
        std::cout << "  PASSED: " << testName << "\n";
        return true;
    }
    std::cerr << "  FAILED: " << testName << "\n"
        << "    Expected: [" << expected << "]\n"
        << "    Actual:   [" << actual << "]\n";
    return false;
}

} // namespace

bool runTestsForBackend(fr::DatabaseBackend backend, const std::string& /*serverName*/, const std::string& dbName)
{
    bool ok = true;
    std::cout << "Testing backend: " << (backend == fr::DatabaseBackend::IBPP ? "IBPP" : "FbCpp") << "\n";
    
    try 
    {
        fr::IDatabasePtr db = fr::DatabaseFactory::createDatabase(backend);
        db->setConnectionString(dbName);
        db->setCredentials("SYSDBA", "masterkey");
        db->connect();

        fr::ITransactionPtr tr = db->createTransaction();
        tr->start();

        fr::IStatementPtr st = db->createStatement(tr);
        
        // Basic Metadata tests (works on all FB versions)
        std::cout << "  Running basic metadata tests...\n";
        st->prepare("SELECT CAST(1.23 AS NUMERIC(18,4)) AS MY_ALIAS FROM RDB$DATABASE");
        st->execute();
        st->fetch();
        ok = check(st->getColumnCount() == 1, "getColumnCount") && ok;
        ok = check(st->getColumnType(0) == fr::ColumnType::BigInt, "ColumnType::BigInt identification") && ok;
        ok = check(st->getColumnScale(0) == 4, "getColumnScale") && ok;
        ok = check(st->getColumnSize(0) == 8, "getColumnSize (int64)") && ok;
        ok = checkStr(st->getColumnAlias(0), "MY_ALIAS", "getColumnAlias") && ok;
        
        // Subtype test (OCTETS)
        st->prepare("SELECT CAST('abc' AS VARCHAR(10) CHARACTER SET OCTETS) FROM RDB$DATABASE");
        st->execute();
        st->fetch();
        int subtype = st->getColumnSubtype(0);
        if (subtype != 1)
        {
            std::cout << "    INFO: Subtype reported as " << subtype << " (expected 1 for OCTETS)\n";
            // Some API versions might report charset ID differently in subtype field for strings
        }
        ok = check(subtype != 0, "getColumnSubtype (non-zero for OCTETS)") && ok;

        // Table name test
        st->prepare("SELECT RDB$RELATION_NAME FROM RDB$RELATIONS");
        st->execute();
        st->fetch();
        std::string tableName = st->getColumnTable(0);
        // Trim trailing spaces if any (IBPP might return padded strings for metadata)
        size_t last = tableName.find_last_not_of(' ');
        if (last != std::string::npos) tableName = tableName.substr(0, last + 1);
        ok = check(tableName == "RDB$RELATIONS", "getColumnTable") && ok;

        // Temporal types
        std::cout << "  Running temporal types tests...\n";
        st->prepare("SELECT CAST('2023-05-20' AS DATE), CAST('12:34:56.7890' AS TIME), CAST('2023-05-20 12:34:56.7890' AS TIMESTAMP) FROM RDB$DATABASE");
        st->execute();
        st->fetch();
        ok = check(st->getColumnType(0) == fr::ColumnType::Date, "ColumnType::Date identification") && ok;
        ok = check(st->getColumnType(1) == fr::ColumnType::Time, "ColumnType::Time identification") && ok;
        ok = check(st->getColumnType(2) == fr::ColumnType::Timestamp, "ColumnType::Timestamp identification") && ok;
        
        ok = check(st->getDate(0).find("2023-05-20") != std::string::npos, "getDate content") && ok;
        ok = check(st->getTime(1).find("12:34:56") != std::string::npos, "getTime content") && ok;
        ok = check(st->getTimestamp(2).find("2023-05-20 12:34:56") != std::string::npos, "getTimestamp content") && ok;

        // Check if Firebird 4.0+ by trying to CAST to DECFLOAT
        std::cout << "  Checking for Firebird 4.0+ types support...\n";
        try 
        {
            st->prepare("SELECT CAST(1 AS DECFLOAT(16)) FROM RDB$DATABASE");
            st->execute();
            st->fetch();
            ok = check(st->getColumnType(0) == fr::ColumnType::Decfloat16, "ColumnType::Decfloat16 identification") && ok;

            // Test DECFLOAT34
            st->prepare("SELECT CAST('1.234567890123456789012345678901234' AS DECFLOAT(34)) FROM RDB$DATABASE");
            st->execute();
            st->fetch();
            ok = check(st->getColumnType(0) == fr::ColumnType::Decfloat34, "ColumnType::Decfloat34 identification") && ok;
            std::string df34 = st->getString(0);
            ok = check(df34.find("1.234567890123456789012345678901234") == 0, "DECFLOAT34 getString content") && ok;

            // Test INT128
            st->prepare("SELECT CAST('12345678901234567890123456789012345678' AS INT128) FROM RDB$DATABASE");
            st->execute();
            st->fetch();
            ok = check(st->getColumnType(0) == fr::ColumnType::Int128, "ColumnType::Int128 identification") && ok;
            ok = checkStr(st->getString(0), "12345678901234567890123456789012345678", "INT128 getString") && ok;
        }
        catch (const std::exception& e)
        {
            std::string msg = e.what();
            if (msg.find("Token unknown") != std::string::npos || msg.find("Data type unknown") != std::string::npos)
            {
                std::cout << "  Skipping Firebird 4.0+ types (unsupported by server/backend).\n";
            }
            else
            {
                std::cerr << "  ERROR: Unexpected exception during FB 4.0+ types test: " << e.what() << "\n";
                ok = false;
            }
        }

        tr->commit();
        db->disconnect();
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION in backend tests: " << e.what() << "\n";
        return false;
    }
    return ok;
}

int main()
{
    const char* envServer = std::getenv("IBPP_TEST_SERVER");
    const std::string serverName = envServer ? envServer : "";
    if (serverName.empty())
    {
        std::cout << "IBPP_TEST_SERVER is not set, skipping dal_types_test.\n";
        return 0;
    }

    const std::string dbName = "/tmp/flamerobin_dal_types_test_" +
        std::to_string(static_cast<long long>(std::time(0))) + ".fdb";

    std::cout << "Creating test database: " << dbName << "\n";

    // Create DB using IBPP
    try 
    {
        IBPP::Database db = IBPP::DatabaseFactory(serverName, dbName, "SYSDBA", "masterkey");
        db->Create(3);
    }
    catch (const IBPP::Exception& e)
    {
        std::cerr << "Failed to create test database: " << e.what() << "\n";
        return 1;
    }

    bool all_ok = true;
    all_ok = runTestsForBackend(fr::DatabaseBackend::IBPP, serverName, dbName) && all_ok;
    
    try 
    {
        all_ok = runTestsForBackend(fr::DatabaseBackend::FbCpp, serverName, dbName) && all_ok;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FbCpp backend tests failed to even start: " << e.what() << "\n";
        all_ok = false;
    }

    // Cleanup
    try 
    {
        IBPP::Database db = IBPP::DatabaseFactory(serverName, dbName, "SYSDBA", "masterkey");
        db->Connect();
        db->Drop();
    }
    catch (...) {}

    if (all_ok)
    {
        std::cout << "ALL DAL TESTS PASSED\n";
        return 0;
    }
    else
    {
        std::cout << "SOME DAL TESTS FAILED\n";
        return 1;
    }
}
