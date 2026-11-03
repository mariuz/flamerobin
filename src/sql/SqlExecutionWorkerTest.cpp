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

#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "sql/SqlExecutionWorker.h"

namespace
{
bool check(bool condition, const char* testName)
{
    if (condition)
        return true;
    std::cerr << "FAIL: " << testName << "\n";
    return false;
}
} // namespace

int main()
{
    bool ok = true;
    std::cout << "Starting SqlExecutionWorker tests..." << std::endl;

    // Test 1: Successful execution maps to SqlExecutionOutcome::Ok
    {
        std::atomic<bool> cancelRequested{false};
        bool executed = false;
        SqlWorkerResult res = runSqlWorkerTask([&]() {
            executed = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }, cancelRequested);

        ok = check(executed, "Test 1: task was executed") && ok;
        ok = check(res.outcome == SqlExecutionOutcome::Ok, "Test 1: outcome is Ok") && ok;
        ok = check(res.errorMessage.empty(), "Test 1: errorMessage is empty") && ok;
        ok = check(res.elapsedMillis >= 0, "Test 1: elapsedMillis is non-negative") && ok;
    }

    // Test 2: Standard exception without cancel maps to SqlExecutionOutcome::Error
    {
        std::atomic<bool> cancelRequested{false};
        SqlWorkerResult res = runSqlWorkerTask([]() {
            throw std::runtime_error("Dynamic SQL Error: Table unknown FOO");
        }, cancelRequested);

        ok = check(res.outcome == SqlExecutionOutcome::Error, "Test 2: outcome is Error") && ok;
        ok = check(res.errorMessage == "Dynamic SQL Error: Table unknown FOO",
            "Test 2: errorMessage preserved") && ok;
    }

    // Test 3: Pre-cancelled task does not execute and maps to SqlExecutionOutcome::Cancelled
    {
        std::atomic<bool> cancelRequested{true};
        bool executed = false;
        SqlWorkerResult res = runSqlWorkerTask([&]() {
            executed = true;
        }, cancelRequested);

        ok = check(!executed, "Test 3: pre-cancelled task not executed") && ok;
        ok = check(res.outcome == SqlExecutionOutcome::Cancelled,
            "Test 3: pre-cancelled outcome is Cancelled") && ok;
    }

    // Test 4: Cooperative cancellation set during task execution (throwing server exception)
    // maps to SqlExecutionOutcome::Cancelled rather than Error
    {
        std::atomic<bool> cancelRequested{false};
        SqlWorkerResult res = runSqlWorkerTask([&]() {
            cancelRequested.store(true);
            throw std::runtime_error("aborted by cancel request");
        }, cancelRequested);

        ok = check(res.outcome == SqlExecutionOutcome::Cancelled,
            "Test 4: exception with cancelRequested=true maps to Cancelled") && ok;
    }

    // Test 5: Firebird fb_cancel_raise error message ("operation was cancelled" / "isc_cancelled")
    // maps to SqlExecutionOutcome::Cancelled even if cancelRequested was not observed first
    {
        std::atomic<bool> cancelRequested{false};
        SqlWorkerResult res1 = runSqlWorkerTask([]() {
            throw std::runtime_error("Statement failed, SQLSTATE = HY008\noperation was cancelled");
        }, cancelRequested);
        ok = check(res1.outcome == SqlExecutionOutcome::Cancelled,
            "Test 5a: 'operation was cancelled' maps to Cancelled") && ok;

        SqlWorkerResult res2 = runSqlWorkerTask([]() {
            throw std::runtime_error("Firebird error: isc_cancelled (335544794)");
        }, cancelRequested);
        ok = check(res2.outcome == SqlExecutionOutcome::Cancelled,
            "Test 5b: 'isc_cancelled' maps to Cancelled") && ok;
    }

    // Test 6: Cooperative cancellation where task finishes normally after cancelRequested=true
    {
        std::atomic<bool> cancelRequested{false};
        SqlWorkerResult res = runSqlWorkerTask([&]() {
            cancelRequested.store(true);
        }, cancelRequested);
        ok = check(res.outcome == SqlExecutionOutcome::Cancelled,
            "Test 6: normal return after cancelRequested=true maps to Cancelled") && ok;
    }

    // Test 7: Non-std::exception maps to SqlExecutionOutcome::SystemError (or Cancelled if cancelled)
    {
        std::atomic<bool> cancelRequested{false};
        SqlWorkerResult res1 = runSqlWorkerTask([]() {
            throw 42;
        }, cancelRequested);
        ok = check(res1.outcome == SqlExecutionOutcome::SystemError,
            "Test 7a: unknown exception maps to SystemError") && ok;

        cancelRequested.store(true);
        SqlWorkerResult res2 = runSqlWorkerTask([]() {
            throw 42;
        }, cancelRequested);
        ok = check(res2.outcome == SqlExecutionOutcome::Cancelled,
            "Test 7b: unknown exception when cancelled maps to Cancelled") && ok;
    }

    // Test 8: Background thread execution and cancellation integration
    {
        std::atomic<bool> cancelRequested{false};
        std::atomic<bool> workerStarted{false};
        SqlWorkerResult workerRes;

        std::thread worker([&]() {
            workerRes = runSqlWorkerTask([&]() {
                workerStarted.store(true);
                while (!cancelRequested.load())
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
                throw std::runtime_error("operation was cancelled");
            }, cancelRequested);
        });

        while (!workerStarted.load())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        cancelRequested.store(true);
        worker.join();

        ok = check(workerRes.outcome == SqlExecutionOutcome::Cancelled,
            "Test 8: background worker thread cancellation outcome is Cancelled") && ok;
    }

    if (ok)
        std::cout << "All SqlExecutionWorker tests PASSED." << std::endl;
    return ok ? 0 : 1;
}
