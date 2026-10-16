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

#ifndef FR_BENCHMARKRESULTS_H
#define FR_BENCHMARKRESULTS_H

#include <wx/string.h>

#include <memory>
#include <optional>
#include <vector>

#include "engine/Benchmark.h"

namespace fr
{

/// what a result of the benchmark dialog is
enum class BenchmarkResultKind
{
    Run,            // a test of this session
    SavedReport,    // an HTML report that was saved earlier and opened
    Comparison      // two results compared with each other
};

///
/// One result of the benchmark dialog. It holds copies only, never a
/// pointer to a database, a server or a window, so that it outlives the
/// dialog and the registration it was made for. The report is kept as it
/// was measured: "Hide private information" only changes what is shown,
/// copied and saved.
///
struct BenchmarkResultEntry
{
    int id = 0;             // from 1, never given again; 0: not in a store
    BenchmarkResultKind kind = BenchmarkResultKind::Run;
    // Run: the report of the test, without the earlier report
    std::shared_ptr<const BenchmarkReport> report;
    // Run: the earlier report it is compared with, read once at the start
    std::optional<BenchmarkSnapshot> baseline;
    // Run and SavedReport: the results, the source of comparisons
    BenchmarkSnapshot snapshot;
    // SavedReport: the page as it was saved
    wxString html;
    // Comparison: copies of both results, which may be closed later
    std::shared_ptr<const BenchmarkComparison> comparison;
    // Comparison: where it comes from, for its name: the number of a
    // result (0 for a file that has no tab) and the short name of it
    int olderId = 0;
    int newerId = 0;
    wxString olderName;
    wxString newerName;
    // Run: cancelled, or failed after some values were measured
    bool incomplete = false;
    // saved with "Save report..." (a copy or "Open in browser" is not), or
    // opened from a file
    bool saved = false;
    // the file it was opened from or saved to last
    wxString path;
};

/// whether a test measured anything: a store keeps only those; the
/// message of a test that ended before its first value is in the log
bool hasBenchmarkData(const BenchmarkReport& report);

///
/// The results of the benchmark dialog in the order of their tabs. The main
/// window owns it, so that the results stay when the dialog is closed; it
/// is used on the GUI thread only. Nothing is written to disk.
///
class BenchmarkResultStore
{
public:
    // more tabs are hard to use; memory would allow many more
    static constexpr size_t maximumEntries = 30;

    /// the number of the new result; 0 if the store is full or the report
    /// has no data
    int addRun(const BenchmarkReport& report,
        const std::optional<BenchmarkSnapshot>& baseline);
    /// a saved HTML report; 0 if the store is full
    int addSavedReport(const BenchmarkSnapshot& snapshot, const wxString& html,
        const wxString& path);
    /// the comparison of two results (copies of both); the number of the
    /// same comparison if there is one, 0 if the store is full
    int addComparison(const BenchmarkResultEntry& older, const BenchmarkResultEntry& newer);

    /// the result to select afterwards (0: none): when the selected one is
    /// closed its right neighbour, else its left one; otherwise the
    /// selection stays
    int close(int id);
    int closeOthers(int id);
    int closeAll();

    int getSelected() const;
    bool select(int id);
    void markSaved(int id, const wxString& path);
    const std::vector<BenchmarkResultEntry>& getEntries() const;
    const BenchmarkResultEntry* find(int id) const;
    /// the tests that were not saved; files and comparisons do not count
    std::vector<int> getUnsavedIds() const;
    /// the results that can be compared: tests and opened files, in order
    std::vector<int> getComparableIds() const;
    bool isFull() const;

private:
    std::vector<BenchmarkResultEntry> entriesM;
    int lastIdM = 0;
    int selectedM = 0;
    int add(BenchmarkResultEntry entry);
};

/// which results "Close", "Close other results" or "Close all results"
/// close
enum class BenchmarkCloseScope { One, Others, All };

/// the tests that are not saved and would be lost: empty if no question
/// needs to be asked
std::vector<int> getBenchmarkCloseQuestion(const BenchmarkResultStore& store,
    BenchmarkCloseScope scope, int id);
/// "3, 5, 6"
wxString formatBenchmarkResultIds(const std::vector<int>& ids);

/// the name of the tab: "3  14:32" (number and time of the start), with
/// "* " in front if the test is not saved, "2 vs 5", or the file name
wxString getBenchmarkResultLabel(const BenchmarkResultEntry& entry);
/// one line for the tool tip and the lists: "3  Run, 2026-10-14 09:55,
/// Quick, this computer, HP ZBook (not saved)"
wxString describeBenchmarkResult(const BenchmarkResultEntry& entry);
/// above the shown result: "Result 5 - not saved"
wxString getBenchmarkResultHeader(const BenchmarkResultEntry& entry);
/// the name of a result in a sentence: "result 3" or the file name
wxString getBenchmarkResultName(const BenchmarkResultEntry& entry);
/// the suggested name of the saved file:
/// "flamerobin-benchmark-3-20261014-0955.html",
/// "flamerobin-benchmark-comparison-2-vs-5.html"
wxString getBenchmarkResultFileName(const BenchmarkResultEntry& entry);

/// the order of two results to compare: the one with the earlier date is
/// the older one; with the same or without a date, a stays the older one
struct BenchmarkPair
{
    const BenchmarkResultEntry* older;
    const BenchmarkResultEntry* newer;
};
BenchmarkPair orderBenchmarkPair(const BenchmarkResultEntry& a, const BenchmarkResultEntry& b);

/// what is to be said before two results are compared
enum class BenchmarkPairCheck
{
    SameResult,         // the same result twice: nothing to compare
    DifferentKind,      // embedded, on the server, or as a client
    DifferentIntensity,
    Incomplete          // a cancelled or failed test: only common values
};
std::vector<BenchmarkPairCheck> checkBenchmarkPair(const BenchmarkResultEntry& older,
    const BenchmarkResultEntry& newer);
/// the sentence of a check
wxString describeBenchmarkPairCheck(BenchmarkPairCheck check,
    const BenchmarkResultEntry& older, const BenchmarkResultEntry& newer);
/// "Earlier: result 3 (09:55)   Later: result 5 (10:31)"
wxString describeBenchmarkPairOrder(const BenchmarkResultEntry& older,
    const BenchmarkResultEntry& newer);

/// whether a saved page is no more than a FlameRobin report, so that it can
/// be shown in the report view: no script, frame, object, form, image or
/// style sheet of its own, no event handler, no address to load or run
/// (links within the page are allowed), no conditional comment of Internet
/// Explorer and no refresh; a file that someone changed is not shown
bool isBenchmarkReportHtmlPlain(const wxString& html);
/// the page with the theme of the dialog ("dark" or "light"; empty: the
/// theme of the system), set in the data-theme attribute of <html>
wxString applyBenchmarkReportTheme(const wxString& html, const wxString& theme);

} // namespace fr

#endif // FR_BENCHMARKRESULTS_H
