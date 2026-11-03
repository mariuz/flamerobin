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

#ifndef FR_SQLEXECUTIONWORKER_H
#define FR_SQLEXECUTIONWORKER_H

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <exception>
#include <functional>
#include <string>

enum class SqlExecutionOutcome
{
    Ok,
    Cancelled,
    Error,
    SystemError
};

struct SqlWorkerResult
{
    SqlExecutionOutcome outcome = SqlExecutionOutcome::Ok;
    long elapsedMillis = 0;
    std::string errorMessage;
};

inline bool isCancelledErrorMessage(const std::string& msg)
{
    if (msg.empty())
        return false;

    std::string lower(msg);
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return lower.find("operation was cancelled") != std::string::npos
        || lower.find("isc_cancelled") != std::string::npos;
}

inline SqlWorkerResult runSqlWorkerTask(
    const std::function<void()>& task,
    const std::atomic<bool>& cancelRequested)
{
    SqlWorkerResult result;
    const auto start = std::chrono::steady_clock::now();

    if (cancelRequested.load())
    {
        result.outcome = SqlExecutionOutcome::Cancelled;
        return result;
    }

    try
    {
        if (task)
            task();

        result.outcome = cancelRequested.load()
            ? SqlExecutionOutcome::Cancelled
            : SqlExecutionOutcome::Ok;
    }
    catch (const std::exception& e)
    {
        result.errorMessage = e.what();
        if (cancelRequested.load() || isCancelledErrorMessage(result.errorMessage))
            result.outcome = SqlExecutionOutcome::Cancelled;
        else
            result.outcome = SqlExecutionOutcome::Error;
    }
    catch (...)
    {
        if (cancelRequested.load())
            result.outcome = SqlExecutionOutcome::Cancelled;
        else
            result.outcome = SqlExecutionOutcome::SystemError;
    }

    const auto end = std::chrono::steady_clock::now();
    result.elapsedMillis = static_cast<long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
    return result;
}

#endif // FR_SQLEXECUTIONWORKER_H
