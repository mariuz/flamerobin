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

#ifndef FR_DATAGRIDFILTERUTILS_H
#define FR_DATAGRIDFILTERUTILS_H

#include <wx/string.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Pre-lowered search needle that performs case-insensitive substring matching
// without allocating a lowered wxString copy for every scanned cell (#773).
class CaseInsensitiveNeedle
{
private:
    bool emptyM = true;
    bool asciiOnlyM = true;
    std::string asciiLowerM;
    std::vector<wxUniChar::value_type> unicodeLowerM;

public:
    explicit CaseInsensitiveNeedle(const wxString& filterText);

    bool empty() const { return emptyM; }
    bool isAsciiOnly() const { return asciiOnlyM; }
    bool matches(const wxString& haystack) const;
};

// Monotonic generation + cancellation token helper shared by DataGridTable
// and unit tests to guarantee superseded background filter runs abort promptly
// and never overwrite newer results (#773).
class FilterGenerationTracker
{
private:
    std::atomic<uint64_t> generationM{0};
    std::atomic<bool> cancelM{false};

public:
    uint64_t startNewJob()
    {
        uint64_t next = generationM.fetch_add(1, std::memory_order_relaxed) + 1;
        cancelM.store(false, std::memory_order_relaxed);
        return next;
    }

    void cancelCurrent()
    {
        generationM.fetch_add(1, std::memory_order_relaxed);
        cancelM.store(true, std::memory_order_relaxed);
    }

    bool isCancelled(uint64_t jobGeneration) const
    {
        return cancelM.load(std::memory_order_relaxed)
            || generationM.load(std::memory_order_relaxed) != jobGeneration;
    }

    uint64_t currentGeneration() const
    {
        return generationM.load(std::memory_order_relaxed);
    }
};

// Scans [startRow, endRow) using rowPred, periodically polling shouldCancel.
// Returns false and clears outMatches if cancelled mid-scan.
bool filterRowRangeCancellable(
    size_t startRow,
    size_t endRow,
    const std::function<bool(size_t)>& rowPred,
    const std::function<bool()>& shouldCancel,
    std::vector<size_t>& outMatches,
    size_t cancelCheckInterval = 256);

#endif // FR_DATAGRIDFILTERUTILS_H
