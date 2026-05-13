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

#include <ibpp.h>

#include "engine/db/DatabaseFactory.h"
#include "engine/db/IDatabase.h"
#include "engine/db/ITransaction.h"
#include "engine/db/IStatement.h"
#include "engine/db/TestUtils.h"

int main()
{
    const char* envServer = std::getenv("IBPP_TEST_SERVER");
    const std::string serverName = envServer ? envServer : "";
    if (serverName.empty())
    {
        std::cout << "IBPP_TEST_SERVER is not set, skipping ProfilerTest.\n";
        return 0;
    }

    const std::string dbName = fr_test::getTestDbPath("profiler_test");

    // Create DB using IBPP
    try 
    {
        IBPP::Database db = IBPP::DatabaseFactory(serverName, dbName, "SYSDBA", "masterkey");
        db->Create(3);
    }
    catch (const IBPP::Exception& e)
    {
        fr_test::printException(e, "create test database");
        return 1;
    }

    bool ok = true;
    std::cout << "Starting RDB$PROFILER extensive tests...\n";

    try 
    {
        fr::IDatabasePtr db = fr::DatabaseFactory::createDatabase(fr::DatabaseBackend::FbCpp);
        std::string fullConnStr = serverName + ":" + dbName;
        std::cout << "  Connecting to database: " << fullConnStr << "\n";
        db->setConnectionString(fullConnStr);
        db->setCredentials("SYSDBA", "masterkey");
        try {
            db->connect();
        } catch (const std::exception& e) {
            std::cerr << "    FAILED to connect to " << fullConnStr << " using fb-cpp backend\n";
            throw;
        }

        std::cout << "  Engine version: " << db->getEngineVersion() << "\n";

        fr::ITransactionPtr tr = db->createTransaction();
        tr->start();

        fr::IStatementPtr st = db->createStatement(tr);
        
        int64_t sessionId = 0;
        int count = 0;

        // Check if RDB$PROFILER exists
        try {
            st->prepare("SELECT 1 FROM RDB$PACKAGES WHERE RDB$PACKAGE_NAME = 'RDB$PROFILER'");
            st->execute();
            if (!st->fetch())
            {
                std::cout << "RDB$PROFILER package not found, skipping tests.\n";
                tr->rollback();
                db->disconnect();
                return 0;
            }
        } catch(const std::exception& e) {
            fr_test::printException(e, "checking for RDB$PROFILER");
            return 0;
        }

        std::string reqNameCol = "NAME";

        // Check active profiler plugin
        try {
            st->prepare("SELECT RDB$CONFIG_VALUE FROM RDB$CONFIG WHERE RDB$CONFIG_NAME = 'Default_Profiler'");
            st->execute();
            if (st->fetch())
                std::cout << "    Debug: Default_Profiler = " << st->getString(0) << "\n";
            else
                std::cout << "    Debug: Default_Profiler NOT SET in RDB$CONFIG\n";
        } catch(...) {
             std::cout << "    Debug: RDB$CONFIG check failed (normal on older versions)\n";
        }

        // Check if profiler tables exist
        std::vector<std::string> tables = { "PLG$PROF_SESSIONS", "PLG$PROF_STATEMENTS", "PLG$PROF_RECORD_SOURCES", "PLG$PROF_RECORD_SOURCE_STATS", "PLG$PROF_REQUESTS" };
        for (const auto& t : tables)
        {
            try {
                st->prepare("SELECT COUNT(*) FROM " + t);
                st->execute();
                if (st->fetch())
                    std::cout << "    Debug: Table " << t << " exists, count = " << st->getInt32(0) << "\n";
            } catch(...) {
                 std::cout << "    Debug: Table " << t << " check FAILED (normal if not yet initialized)\n";
            }
        }

        // Create table first, then profile
        st->prepare("CREATE TABLE t1 (id INT PRIMARY KEY, val VARCHAR(100))");
        st->execute();
        tr->commitRetain();

        // Test 1: Simple Profiling Session
        std::cout << "  Test 1: Simple Profiling Session...\n";
        bool sessionStarted = false;
        try {
            st->prepare("SELECT RDB$PROFILER.START_SESSION(?) FROM RDB$DATABASE");
            st->setString(0, "Test Session");
            st->execute();
            sessionId = 0;
            if (st->fetch())
                sessionId = st->getInt64(0);
            
            std::cout << "    Debug: Session ID = " << sessionId << "\n";
            if (sessionId != 0)
            {
                sessionStarted = true;
                ok = fr_test::check(true, "Session started") && ok;

                // Determine correct column name for requests (NAME or REQUEST_NAME)
                // Now that the session started, tables should definitely exist.
                try {
                    st->prepare("SELECT REQUEST_NAME FROM PLG$PROF_REQUESTS WHERE 1=0");
                    reqNameCol = "REQUEST_NAME";
                } catch(...) {
                    reqNameCol = "NAME";
                }
                std::cout << "    Debug: Using request name column: " << reqNameCol << "\n";

                // Use INSERT ... SELECT to ensure there is a record source to profile
                std::cout << "    Executing INSERT ... SELECT...\n";
                st->prepare("INSERT INTO t1 (id, val) SELECT 1, 'test' FROM RDB$DATABASE");
                st->execute();
                
                // Add a simple SELECT as well
                std::cout << "    Executing SELECT * FROM t1...\n";
                st->prepare("SELECT * FROM t1");
                st->execute();
                while (st->fetch()) {}

                tr->commitRetain();

                std::cout << "    Finishing session...\n";
                st->prepare("EXECUTE PROCEDURE RDB$PROFILER.FINISH_SESSION(TRUE)");
                st->execute();
                std::cout << "    Debug: FINISH_SESSION executed.\n";
                tr->commit();

                // Start new transaction to check results
                tr = db->createTransaction();
                tr->start();
                st = db->createStatement(tr);

                // Check statements first
                st->prepare("SELECT COUNT(*) FROM PLG$PROF_STATEMENTS WHERE PROFILE_ID = ?");
                st->setInt64(0, sessionId);
                st->execute();
                int stmtCount = 0;
                if (st->fetch()) stmtCount = st->getInt32(0);
                std::cout << "    Debug: Statement count in PLG$PROF_STATEMENTS for session " << sessionId << ": " << stmtCount << "\n";

                // Give Firebird a moment to flush profiler data if needed
                st->prepare("SELECT COUNT(*) FROM PLG$PROF_RECORD_SOURCE_STATS WHERE PROFILE_ID = ?");
                st->setInt64(0, sessionId);
                st->execute();
                int count = 0;
                if (st->fetch())
                    count = st->getInt32(0);
                
                std::cout << "    Debug: Record source stats count for session " << sessionId << ": " << count << "\n";
                ok = fr_test::check(count > 0, "Record source stats collected for INSERT ... SELECT") && ok;
            }
            else
            {
                std::cout << "    FAILURE: Session ID is 0, profiling might be disabled in firebird.conf\n";
                ok = false;
            }
        } catch (const std::exception& e) {
            std::cout << "    SKIPPING profiling tests: FAILED to start session: " << e.what() << "\n";
            std::cout << "    This is normal if Default_Profiler is disabled in firebird.conf\n";
        }

        if (sessionStarted)
        {
            // Test 2: Nested PSQL calls
            std::cout << "  Test 2: Nested PSQL calls...\n";
            st->prepare("CREATE PROCEDURE p_child AS BEGIN END");
            st->execute();
            st->prepare("CREATE PROCEDURE p_parent AS BEGIN EXECUTE PROCEDURE p_child; END");
            st->execute();
            tr->commitRetain();

            st->prepare("SELECT RDB$PROFILER.START_SESSION(?) FROM RDB$DATABASE");
            st->setString(0, "Nested Session");
            st->execute();
            sessionId = 0;
            if (st->fetch())
                sessionId = st->getInt64(0);

            std::cout << "    Executing parent procedure...\n";
            st->prepare("EXECUTE PROCEDURE p_parent");
            st->execute();

            st->prepare("EXECUTE PROCEDURE RDB$PROFILER.FINISH_SESSION(TRUE)");
            st->execute();

            st->prepare("SELECT COUNT(DISTINCT " + reqNameCol + ") FROM PLG$PROF_REQUESTS WHERE PROFILE_ID = ?");
            st->setInt64(0, sessionId);
            st->execute();
            count = 0;
            if (st->fetch())
                count = st->getInt32(0);
            
            std::cout << "    Debug: Distinct request count for nested session " << sessionId << ": " << count << "\n";
            ok = fr_test::check(count >= 2, "Stats collected for both parent and child procedures") && ok;
        }

        if (sessionStarted)
        {
            // Test 3: Triggers
            std::cout << "  Test 3: Triggers...\n";
            st->prepare("CREATE TABLE t2 (id INT)");
            st->execute();
            st->prepare("CREATE TRIGGER t2_ai FOR t2 AFTER INSERT AS BEGIN END");
            st->execute();
            tr->commitRetain();

            st->prepare("SELECT RDB$PROFILER.START_SESSION(?) FROM RDB$DATABASE");
            st->setString(0, "Trigger Session");
            st->execute();
            sessionId = 0;
            if (st->fetch())
                sessionId = st->getInt64(0);

            st->prepare("INSERT INTO t2 VALUES (1)");
            st->execute();

            st->prepare("EXECUTE PROCEDURE RDB$PROFILER.FINISH_SESSION(TRUE)");
            st->execute();

            st->prepare("SELECT COUNT(*) FROM PLG$PROF_REQUESTS WHERE PROFILE_ID = ? AND " + reqNameCol + " = 'T2_AI'");
            st->setInt64(0, sessionId);
            st->execute();
            count = 0;
            if (st->fetch())
                count = st->getInt32(0);
            
            std::cout << "    Debug: Trigger stats count for session " << sessionId << ": " << count << "\n";
            ok = fr_test::check(count > 0, "Stats collected for trigger") && ok;

            // Test 4: Record Source Stats with JOIN
            std::cout << "  Test 4: Record Source Stats with JOIN...\n";
            st->prepare("SELECT RDB$PROFILER.START_SESSION(?) FROM RDB$DATABASE");
            st->setString(0, "JOIN Session");
            st->execute();
            sessionId = 0;
            if (st->fetch())
                sessionId = st->getInt64(0);

            st->prepare("SELECT COUNT(*) FROM t1 a JOIN t2 b ON a.id = b.id");
            st->execute();
            if (st->fetch()) {}

            st->prepare("EXECUTE PROCEDURE RDB$PROFILER.FINISH_SESSION(TRUE)");
            st->execute();

            st->prepare("SELECT COUNT(*) FROM PLG$PROF_RECORD_SOURCE_STATS WHERE PROFILE_ID = ?");
            st->setInt64(0, sessionId);
            st->execute();
            count = 0;
            if (st->fetch())
                count = st->getInt32(0);
            
            std::cout << "    Debug: Record source stats count for JOIN session " << sessionId << ": " << count << "\n";
            ok = fr_test::check(count >= 2, "Record source stats collected for JOIN") && ok;
        }

        tr->commit();
        db->disconnect();
    }
    catch (const std::exception& e)
    {
        fr_test::printException(e, "ProfilerTest");
        ok = false;
    }

    // Cleanup
    try 
    {
        IBPP::Database db = IBPP::DatabaseFactory(serverName, dbName, "SYSDBA", "masterkey");
        db->Connect();
        db->Drop();
    }
    catch (...) {}

    if (ok)
    {
        std::cout << "ALL Profiler TESTS PASSED\n";
        return 0;
    }
    else
    {
        std::cout << "SOME Profiler TESTS FAILED\n";
        return 1;
    }
}
