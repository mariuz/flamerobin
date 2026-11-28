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

// Tracks visible-viewport-driven on-demand row prefetching (#782).
// Decoupling this from DataGridTable::GetValue() prevents non-scrolling callers
// (AutoSizeColumns, AutoSizeRows, Windows UI Automation / IAccessible, copy,
// sum timer) from triggering runaway fetch loops when GridFetchAllRecords is off.
class ViewportPrefetchTracker
{
private:
    int lastFilterTriggerBottomRowM = -1;
    int lastFilterTriggerViewRowsM = -1;

public:
    static constexpr int kLookaheadRows = 20;
    static constexpr unsigned kBatchSize = 100;

    void reset()
    {
        lastFilterTriggerBottomRowM = -1;
        lastFilterTriggerViewRowsM = -1;
    }

    unsigned evaluate(
        int firstVisibleRow,
        int lastVisibleRow,
        int cursorRow,
        int viewRowCount,
        unsigned loadedRowCount,
        unsigned currentMaxRowToFetch,
        bool allRowsFetched,
        bool fetchAllRows,
        bool hasActiveFilter)
    {
        if (allRowsFetched || fetchAllRows || viewRowCount <= 0 || lastVisibleRow < 0)
            return currentMaxRowToFetch;

        if (currentMaxRowToFetch > loadedRowCount)
            return currentMaxRowToFetch;

        int bottomRow = lastVisibleRow;
        if (cursorRow > bottomRow && cursorRow < viewRowCount)
            bottomRow = cursorRow;

        if (bottomRow + kLookaheadRows < viewRowCount)
            return currentMaxRowToFetch;

        if (hasActiveFilter)
        {
            if (firstVisibleRow <= 0)
                return currentMaxRowToFetch;
            if (bottomRow == lastFilterTriggerBottomRowM
                && viewRowCount == lastFilterTriggerViewRowsM)
            {
                return currentMaxRowToFetch;
            }
            lastFilterTriggerBottomRowM = bottomRow;
            lastFilterTriggerViewRowsM = viewRowCount;
        }
        else
        {
            lastFilterTriggerBottomRowM = -1;
            lastFilterTriggerViewRowsM = -1;
        }

        return loadedRowCount + kBatchSize;
    }
};

#endif // FR_DATAGRIDFILTERUTILS_H
