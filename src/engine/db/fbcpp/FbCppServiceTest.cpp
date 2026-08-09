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

#include "engine/db/DatabaseFactory.h"
#include "engine/db/IService.h"
#include "engine/db/IDatabase.h"
#include "engine/db/TestUtils.h"

int main()
{
    const char* envServer = std::getenv("IBPP_TEST_SERVER");
    const std::string serverName = envServer ? envServer : "";
    if (serverName.empty())
    {
        std::cout << "IBPP_TEST_SERVER is not set, skipping FbCppServiceTest.\n";
        return 0;
    }

    const std::string dbName = fr_test::getTestDbPath("fbcpp_svc_test");

    // Create DB using FbCpp
    try 
    {
        fr::IDatabasePtr db = fr::DatabaseFactory::createDatabase(fr::DatabaseBackend::FbCpp);
        db->setConnectionString(serverName + ":" + dbName);
        db->setCredentials("SYSDBA", "masterkey");
        db->create(8192, 3);
    }
    catch (const std::exception& e)
    {
        fr_test::printException(e, "create test database");
        return 1;
    }

    bool ok = true;
    std::cout << "Starting FbCppService tests...\n";

    try 
    {
        fr::IServicePtr svc = fr::DatabaseFactory::createService(fr::DatabaseBackend::FbCpp);
        std::cout << "  Setting connection string: " << serverName << "\n";
        svc->setConnectionString(serverName);
        svc->setCredentials("SYSDBA", "masterkey");
        
        std::cout << "  Testing getVersion...\n";
        std::string version = svc->getVersion();
        std::cout << "    Debug: Raw version string length: " << version.length() << "\n";
        std::cout << "    Version: " << version << "\n";
        ok = fr_test::check(!version.empty(), "getVersion result is not empty") && ok;

        std::cout << "  Testing getConnectedUsers (via IDatabase)...\n";
        fr::IDatabasePtr db = fr::DatabaseFactory::createDatabase(fr::DatabaseBackend::FbCpp);
        std::string fullConnStr = serverName + ":" + dbName;
        std::cout << "  Connecting to database: " << fullConnStr << "\n";
        db->setConnectionString(fullConnStr);
        db->setCredentials("SYSDBA", "masterkey");
        try {
            db->connect();
            std::cout << "    Connected successfully.\n";
        } catch (const std::exception& e) {
            std::cerr << "    FAILED to connect to " << fullConnStr << "\n";
            std::cerr << "    Exception: " << e.what() << "\n";
            throw;
        }

        std::cout << "    Engine Version: " << db->getEngineVersion() << "\n";
        fr::DatabaseInfoData info;
        std::cout << "    Fetching database info...\n";
        db->getInfo(&info);
        std::cout << "    ODS Version: " << info.ods << "." << info.odsMinor << "\n";
        std::cout << "    Page Size: " << info.pageSize << ", Pages: " << info.pages << "\n";
        std::cout << "    Next Transaction: " << info.nextTransaction << "\n";
        
        std::cout << "    Fetching connected users...\n";
        std::vector<std::string> users;
        db->getConnectedUsers(users);
        std::cout << "    Debug: Found " << users.size() << " connected users.\n";
        ok = fr_test::check(!users.empty(), "getConnectedUsers list is not empty") && ok;
        bool foundSysdba = false;
        for (const auto& u : users)
        {
            std::cout << "    Connected user: [" << u << "]\n";
            if (u == "SYSDBA") foundSysdba = true;
        }
        ok = fr_test::check(foundSysdba, "found SYSDBA in connected users list") && ok;

        std::cout << "  Testing getDialect...\n";
        int dialect = db->getDialect();
        std::cout << "    Dialect: " << dialect << "\n";
        ok = fr_test::check(dialect == 3, "getDialect returns 3") && ok;

        std::cout << "  Disconnecting...\n";
        db->disconnect();
        std::cout << "    Disconnected successfully.\n";
    }
    catch (const std::exception& e)
    {
        fr_test::printException(e, "FbCppServiceTest");
        ok = false;
    }

    // Cleanup
    try 
    {
        fr::IDatabasePtr db = fr::DatabaseFactory::createDatabase(fr::DatabaseBackend::FbCpp);
        db->setConnectionString(serverName + ":" + dbName);
        db->setCredentials("SYSDBA", "masterkey");
        db->connect();
        db->drop();
    }
    catch (...) {}

    if (ok)
    {
        std::cout << "ALL FbCppService TESTS PASSED\n";
        return 0;
    }
    else
    {
        std::cout << "SOME FbCppService TESTS FAILED\n";
        return 1;
    }
}

