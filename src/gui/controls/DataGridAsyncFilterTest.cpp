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

// Regression test for issue #773:
// Debounced, cancellable background filtering with non-allocating case-insensitive
// matching and read-only cached DataGridRowBuffer access.

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/init.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "gui/controls/DataGridFilterUtils.h"
#include "gui/controls/DataGridRowBuffer.h"

namespace
{

bool check(bool condition, const char* testName)
{
    if (condition)
        return true;
    std::cerr << "FAIL: " << testName << "\n";
    return false;
}

bool testCaseInsensitiveNeedleMatching()
{
    bool ok = true;

    {
        CaseInsensitiveNeedle emptyNeedle("");
        ok = check(emptyNeedle.empty(), "empty needle is marked empty") && ok;
        ok = check(emptyNeedle.matches(""), "empty needle matches empty haystack") && ok;
        ok = check(emptyNeedle.matches("anything"), "empty needle matches non-empty haystack") && ok;
    }

    {
        CaseInsensitiveNeedle ascii("PeTeR");
        ok = check(!ascii.empty() && ascii.isAsciiOnly(), "ASCII needle detected") && ok;
        ok = check(!ascii.matches(""), "non-empty needle rejects empty haystack") && ok;
        ok = check(!ascii.matches("pet"), "needle longer than haystack rejected") && ok;
        ok = check(ascii.matches("PETER"), "exact uppercase match") && ok;
        ok = check(ascii.matches("peter"), "exact lowercase match") && ok;
        ok = check(ascii.matches("Dr. Peter Leonhardt"), "substring match in middle") && ok;
        ok = check(ascii.matches(wxString::FromUTF8("Grüße an pEtEr aus München")),
            "ASCII needle matches inside UTF-8 haystack containing umlauts") && ok;
        ok = check(!ascii.matches(wxString::FromUTF8("Grüße aus München")),
            "ASCII needle rejects non-matching UTF-8 haystack") && ok;
    }

    {
        CaseInsensitiveNeedle unicode(wxString::FromUTF8("müller"));
        ok = check(!unicode.empty() && !unicode.isAsciiOnly(), "Unicode needle detected") && ok;
        ok = check(unicode.matches(wxString::FromUTF8("Hans MÜLLER GmbH")),
            "Unicode needle matches uppercase umlaut") && ok;
        ok = check(unicode.matches(wxString::FromUTF8("müller")),
            "Unicode needle matches lowercase umlaut") && ok;
        ok = check(!unicode.matches(wxString::FromUTF8("Mueller")),
            "Unicode needle rejects ASCII transliteration") && ok;
    }

    return ok;
}

bool testRowBufferConstCachedAccess()
{
    bool ok = true;

    // Simulate a 3-column row: col 0 = VARCHAR, col 1 = NULL VARCHAR, col 2 = unloaded BLOB
    DataGridRowBuffer row(3);
    row.setFieldNull(0, false);
    row.setString(0, "Customer Invoice #10492");
    row.setFieldNull(1, true);
    row.setFieldNull(2, false);
    // Notice setString is NOT called for string index 2 (unloaded BLOB)

    const DataGridRowBuffer& constRow = row;
    ok = check(!constRow.isFieldNull(0), "col 0 is non-null") && ok;
    ok = check(constRow.isFieldNull(1), "col 1 is null") && ok;
    ok = check(!constRow.isStringLoaded(2), "unloaded BLOB string not marked loaded") && ok;
    ok = check(constRow.getStringRef(0) == "Customer Invoice #10492",
        "getStringRef returns stored string") && ok;
    ok = check(constRow.getStringRef(99).empty(),
        "out-of-bounds getStringRef returns empty reference safely") && ok;

    CaseInsensitiveNeedle needle("invoice #104");
    ok = check(needle.matches(constRow.getStringRef(0)),
        "needle matches const row string ref directly") && ok;

    return ok;
}

bool testBackgroundFilterCancellationAndGenerations()
{
    bool ok = true;

    // Build 120,000 synthetic rows with large text payloads to exercise realistic
    // multi-threaded scanning and mid-scan cancellation.
    const size_t totalRows = 120000;
    std::vector<std::unique_ptr<DataGridRowBuffer>> storage;
    std::vector<const DataGridRowBuffer*> snapshot;
    storage.reserve(totalRows);
    snapshot.reserve(totalRows);

    const wxString filler(
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
        "incididunt ut labore et dolore magna aliqua. Record payload segment: ");

    for (size_t i = 0; i < totalRows; ++i)
    {
        auto buf = std::make_unique<DataGridRowBuffer>(2);
        buf->setFieldNull(0, false);
        if (i % 1000 == 42)
            buf->setString(0, filler + "TARGET_ALPHA_MATCH_" + wxString::Format("%zu", i));
        else if (i % 500 == 7)
            buf->setString(0, filler + "TARGET_FINAL_KEY_" + wxString::Format("%zu", i));
        else
            buf->setString(0, filler + wxString::Format("%zu", i));

        buf->setFieldNull(1, true);
        snapshot.push_back(buf.get());
        storage.push_back(std::move(buf));
    }

    FilterGenerationTracker tracker;
    std::mutex resultMutex;
    uint64_t appliedGeneration = 0;
    std::vector<size_t> appliedMatches;
    std::atomic<int> cancelledRuns{0};

    auto runWorker = [&](uint64_t gen, const wxString& query, bool slowFirstBatch) {
        CaseInsensitiveNeedle needle(query);
        std::vector<size_t> localMatches;
        bool completed = filterRowRangeCancellable(
            0,
            snapshot.size(),
            [&](size_t r) {
                if (slowFirstBatch && r < 2048 && (r % 256 == 0))
                    std::this_thread::sleep_for(std::chrono::microseconds(200));
                const DataGridRowBuffer* b = snapshot[r];
                if (b->isFieldNull(0))
                    return false;
                return needle.matches(b->getStringRef(0));
            },
            [&]() { return tracker.isCancelled(gen); },
            localMatches,
            128);

        if (!completed)
        {
            cancelledRuns.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        std::lock_guard<std::mutex> lock(resultMutex);
        if (!tracker.isCancelled(gen))
        {
            appliedGeneration = gen;
            appliedMatches = std::move(localMatches);
        }
    };

    // Start several superseded filter runs as if the user is typing rapidly:
    // "t" -> "ta" -> "target_alpha" -> then finally "target_final_key"
    const wxString intermediateQueries[] = {
        "lorem", "target", "target_alpha", "target_alpha_match"
    };
    for (const wxString& q : intermediateQueries)
    {
        uint64_t gen = tracker.startNewJob();
        std::thread worker(runWorker, gen, q, true);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        tracker.cancelCurrent();
        worker.join();
    }

    ok = check(cancelledRuns.load() >= 1,
        "superseded intermediate filter workers abort cleanly upon cancellation") && ok;
    ok = check(appliedMatches.empty(),
        "cancelled filter runs never publish stale results") && ok;

    // Now run the final settled query to completion
    const uint64_t finalGen = tracker.startNewJob();
    std::thread finalWorker(runWorker, finalGen, "target_final_key", false);
    finalWorker.join();

    const size_t expectedFinalCount = totalRows / 500; // i % 500 == 7 across 120,000 rows -> 240
    ok = check(appliedGeneration == finalGen,
        "final filter generation recorded") && ok;
    ok = check(appliedMatches.size() == expectedFinalCount,
        "final background filter produced exact expected match count") && ok;
    if (!appliedMatches.empty())
    {
        ok = check(appliedMatches.front() == 7 && appliedMatches.back() == totalRows - 500 + 7,
            "matched row indices preserve original order") && ok;
    }

    return ok;
}

} // anonymous namespace

int main()
{
    wxInitializer initializer;
    if (!initializer.IsOk())
    {
        std::cerr << "Failed to initialize wxBase.\n";
        return 1;
    }

    bool ok = true;
    ok = testCaseInsensitiveNeedleMatching() && ok;
    ok = testRowBufferConstCachedAccess() && ok;
    ok = testBackgroundFilterCancellationAndGenerations() && ok;

    if (ok)
        std::cout << "All DataGridAsyncFilterTest checks passed.\n";
    return ok ? 0 : 1;
}
