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

#ifndef FR_TEST_UTILS_H
#define FR_TEST_UTILS_H

#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>

#include <ibpp.h>
#include "fb-cpp/fb-cpp.h"

namespace fr_test
{

inline void printStatusVector(const std::intptr_t* statusVector)
{
    if (!statusVector) return;
    std::cerr << "    Status Vector: [";
    const std::intptr_t* p = statusVector;
    bool first = true;
    while (*p != isc_arg_end)
    {
        if (!first) std::cerr << ", ";
        std::cerr << *p;
        // If it's a string argument, try to print the string
        if (*p == isc_arg_string || *p == isc_arg_interpreted || *p == isc_arg_sql_state)
        {
            const char* s = reinterpret_cast<const char*>(*(p+1));
            std::cerr << " (string: \"" << (s ? s : "null") << "\")";
            p += 2;
        }
        else if (*p == isc_arg_gds || *p == isc_arg_number)
        {
             std::cerr << " (val: " << *(p+1) << ")";
             p += 2;
        }
        else
        {
             p++;
        }
        first = false;
    }
    std::cerr << ", 0]\n";
}

inline void printFbEnv()
{
    const char* envs[] = { "FIREBIRD", "FIREBIRD_MSG", "FIREBIRD_TMP", "FIREBIRD_LOCK", "LD_LIBRARY_PATH", "PATH" };
    std::cerr << "  Firebird Environment:\n";
    for (const char* env : envs)
    {
        const char* val = std::getenv(env);
        std::cerr << "    " << env << "=" << (val ? val : "(null)") << "\n";
    }
}

inline void printException(const std::exception& e, const char* context = nullptr)
{
    if (context)
        std::cerr << "EXCEPTION in " << context << ": " << e.what() << "\n";
    else
        std::cerr << "EXCEPTION: " << e.what() << "\n";

    printFbEnv();
    
    // Try to cast to IBPP::Exception
    const IBPP::Exception* ibppE = dynamic_cast<const IBPP::Exception*>(&e);
    if (ibppE)
    {
        std::cerr << "  IBPP::Exception details:\n";
        std::cerr << "    Origin:  " << ibppE->Origin() << "\n";
        
        const IBPP::SQLException* ibppSqlE = dynamic_cast<const IBPP::SQLException*>(ibppE);
        if (ibppSqlE)
        {
            std::cerr << "    SQLCode: " << ibppSqlE->SqlCode() << "\n";
            std::cerr << "    EngineCode: " << ibppSqlE->EngineCode() << "\n";
        }
    }

    // Try to cast to fbcpp::DatabaseException
    const fbcpp::DatabaseException* fbcppE = dynamic_cast<const fbcpp::DatabaseException*>(&e);
    if (fbcppE)
    {
        std::cerr << "  fbcpp::DatabaseException details:\n";
        std::cerr << "    SQLState: " << fbcppE->getSqlState() << "\n";
        std::cerr << "    ErrorCode: " << fbcppE->getErrorCode() << "\n";
        printStatusVector(fbcppE->getErrors().data());
    }
}

inline bool check(bool condition, const char* testName)
{
    if (condition)
    {
        std::cout << "  PASSED: " << testName << "\n";
        return true;
    }
    std::cerr << "  FAILED: " << testName << "\n";
    return false;
}

inline std::string getTestDbPath(const std::string& prefix)
{
    const char* envPath = std::getenv("FR_TEST_DB_PATH");
    std::string baseDir = envPath ? envPath : "/tmp";
    
    return baseDir + "/" + prefix + "_" +
        std::to_string(static_cast<long long>(std::time(0))) + "_" + 
        std::to_string(std::rand() % 1000) + ".fdb";
}

} // namespace fr_test

#endif // FR_TEST_UTILS_H
