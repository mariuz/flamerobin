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

// Unit and regression tests for GitHub issue #780:
// "Statistics tab should show only the current statement + separate
//  Statistics History tab with better formatting"

#include <iostream>
#include <wx/wx.h>
#include <wx/init.h>

#include "gui/StatsOutputModel.h"

namespace
{

bool check(bool condition, const char* testName)
{
    if (condition)
    {
        std::cout << "[PASS] " << testName << "\n";
        return true;
    }
    std::cerr << "[FAIL] " << testName << "\n";
    return false;
}

bool hasSpanWithStyle(const StatsFormattedLine& line, StatsStyle target)
{
    for (const auto& s : line.spans)
    {
        if (s.style == target && s.byteLength > 0)
            return true;
    }
    return false;
}

} // namespace

int main(int argc, char** argv)
{
    wxInitializer init(argc, argv);
    if (!init.IsOk())
    {
        std::cerr << "Failed to initialize wxBase.\n";
        return 1;
    }

    bool ok = true;
    std::cout << "Running StatsOutputModel tests (Issue #780)...\n";

    // Test 1: Run label & SQL preview formatting (single vs multi-statement script)
    {
        ok = check(StatsFormatter::formatRunLabel(1, 1, 1) == "#1",
            "Single-statement run label formats as #1") && ok;
        ok = check(StatsFormatter::formatRunLabel(2, 1, 3) == "#2.1"
            && StatsFormatter::formatRunLabel(2, 2, 3) == "#2.2"
            && StatsFormatter::formatRunLabel(2, 3, 3) == "#2.3",
            "Multi-statement script run labels format as #2.1, #2.2, #2.3") && ok;

        wxString multiLineSql = "\n  \tselect * from MON$DATABASE\nwhere MON$PAGE_SIZE > 4096;";
        ok = check(StatsFormatter::extractSqlPreview(multiLineSql) == "select * from MON$DATABASE",
            "SQL preview extracts trimmed first non-empty line") && ok;

        StatsFormattedLine hdr = StatsFormatter::formatRunHeaderLine(
            "#2", "14:32:10", "select * from MON$DATABASE", "0.061s", false);
        ok = check(hdr.foldDepth == 0 && hdr.isFoldHeader && hdr.defaultExpanded,
            "Run header line is a top-level fold header (depth 0)") && ok;
        ok = check(hdr.text == "#2  14:32:10  select * from MON$DATABASE  [0.061s]",
            "Run header text includes run number, timestamp, SQL preview, and elapsed time") && ok;
        ok = check(hasSpanWithStyle(hdr, StatsStyle::RunHeader),
            "Run header carries RunHeader style") && ok;

        StatsFormattedLine errHdr = StatsFormatter::formatRunHeaderLine(
            "#3", "14:33:00", "select * from UNKNOWN_TBL", "0.004s - ERROR", true);
        ok = check(hasSpanWithStyle(errHdr, StatsStyle::Error),
            "Failed run header highlights error status with Error style") && ok;
    }

    // Test 2: Default expand/collapse preferences & collapsible groups
    {
        StatsExpandPrefs defaults;
        ok = check(defaults.clearStatsOnExecute && defaults.showStatsHistory,
            "Defaults clear Statistics on execute and show Statistics History tab") && ok;
        ok = check(!defaults.expandPrepare && !defaults.expandFields
            && !defaults.expandParameters && !defaults.expandExecution
            && defaults.expandPlan && defaults.expandBufferMetrics
            && defaults.expandRecordMetrics && defaults.expandDmlMetrics
            && defaults.expandMemoryMetrics && defaults.expandTableReads,
            "Default group expand flags collapse Prepare/Fields/Parameters and expand Plan/Metrics") && ok;

        auto prep = StatsFormatter::formatPrepareGroup(
            "select * from MON$DATABASE", true, "0.000s", false, defaults.expandPrepare);
        ok = check(prep.size() == 4 && prep[0].foldDepth == 1
            && prep[0].isFoldHeader && !prep[0].defaultExpanded,
            "Prepare group header is collapsible at depth 1 and collapsed by default") && ok;
        ok = check(prep[2].foldDepth == 2 && hasSpanWithStyle(prep[2], StatsStyle::Sql),
            "Prepare detail line is at depth 2 with SQL syntax styling") && ok;

        std::vector<wxString> fields = {
            "Field #01: MON$DATABASE.MON$DATABASE_NAME Alias:MON$DATABASE_NAME Type:STRING(1020) len: 1020 scale: 0",
            "Field #02: MON$DATABASE.MON$PAGE_SIZE Alias:MON$PAGE_SIZE Type:INTEGER len: 4 scale: 0"
        };
        auto fldGroup = StatsFormatter::formatFieldsGroup(fields, defaults.expandFields);
        ok = check(fldGroup.size() == 3 && fldGroup[0].text.Contains("Fields (2)")
            && !fldGroup[0].defaultExpanded,
            "Fields group shows field count in header and starts collapsed by default") && ok;
    }

    // Test 3: Highlighting of non-zero physical reads, sequential scans, and natural plans
    {
        auto planLines = StatsFormatter::formatPlanGroup(
            "Select Expression\n    -> Table \"MON$DATABASE\" Full Scan", true);
        ok = check(planLines.size() == 3
            && hasSpanWithStyle(planLines[0], StatsStyle::MetricWarning)
            && hasSpanWithStyle(planLines[2], StatsStyle::MetricWarning),
            "Query Plan highlights Full Scan / NATURAL scan in header and plan line") && ok;

        auto zeroBuf = StatsFormatter::formatBufferMetricsGroup(7, 0, 0, 0, true);
        ok = check(!hasSpanWithStyle(zeroBuf[1], StatsStyle::MetricWarning),
            "Page Buffer Metrics with 0 physical reads/writes has no warning highlight") && ok;

        auto diskBuf = StatsFormatter::formatBufferMetricsGroup(50, 12, 3, 1, true);
        ok = check(hasSpanWithStyle(diskBuf[0], StatsStyle::MetricWarning)
            && hasSpanWithStyle(diskBuf[1], StatsStyle::MetricWarning),
            "Page Buffer Metrics highlights non-zero physical reads and writes") && ok;

        auto seqIo = StatsFormatter::formatRecordMetricsGroup(1, 250, true);
        ok = check(hasSpanWithStyle(seqIo[0], StatsStyle::MetricWarning)
            && hasSpanWithStyle(seqIo[1], StatsStyle::MetricWarning),
            "Record I/O Metrics highlights non-zero sequential scans (rseq)") && ok;

        std::vector<StatsTableActivityItem> tblItems = {
            {"RDB$SECURITY_CLASSES", 0, 0, 0, 1, 0},
            {"LARGE_LOG", 0, 0, 0, 0, 500}
        };
        auto tblGroup = StatsFormatter::formatTableActivityGroup(tblItems, true);
        ok = check(tblGroup.size() == 3
            && !hasSpanWithStyle(tblGroup[1], StatsStyle::MetricWarning)
            && hasSpanWithStyle(tblGroup[2], StatsStyle::MetricWarning),
            "Per-Table Activity highlights sequential table reads while keeping indexed reads normal") && ok;
    }

    // Test 4: UTF-8 byte span accuracy on non-ASCII identifiers
    {
        std::vector<StatsTableActivityItem> unicodeItems = {
            {wxString::FromUTF8("KÜNDEN_ÜBERSICHT"), 2, 0, 0, 5, 0}
        };
        auto lines = StatsFormatter::formatTableActivityGroup(unicodeItems, true);
        size_t totalUtf8Bytes = lines[1].text.ToUTF8().length();
        size_t spanEnd = lines[1].spans.back().byteOffset + lines[1].spans.back().byteLength;
        ok = check(totalUtf8Bytes == spanEnd,
            "Style spans use exact UTF-8 byte offsets for multi-byte identifiers") && ok;
    }

    // Test 5: SessionStatsHistoryIndex lookup (including consecutive duplicate SQL
    // and multi-statement script line range preservation)
    {
        SessionStatsHistoryIndex index;

        // Run #1: single statement stored at StatementHistory pos 10
        index.startRun(1, "14:31:58", 0);
        index.updateRunEndLine(1, 18);
        index.bindRunToHistory(1, 10, "select * from MON$DATABASE");

        // Run #2: identical SQL executed again -> StatementHistory::add() reuses pos 10
        index.startRun(2, "14:32:10", 20);
        index.updateRunEndLine(2, 38);
        index.bindRunToHistory(2, 10, "select * from MON$DATABASE");

        // Run #3: multi-statement script (#3.1 at line 40, #3.2 at line 55..70) at pos 11
        index.startRun(3, "14:35:00", 40);
        index.updateRunEndLine(3, 53);
        index.startRun(3, "14:35:01", 55); // second statement in same script run #3
        index.updateRunEndLine(3, 70);
        index.bindRunToHistory(3, 11, "insert into t values (1);\ninsert into t values (2);");

        const SessionStatsRunRecord* matchDup =
            index.findMatchingRun(10, "select * from MON$DATABASE");
        ok = check(matchDup != nullptr && matchDup->runId == 2
            && matchDup->startLine == 20 && matchDup->endLine == 38,
            "History lookup returns most recent session run (#2) for consecutive duplicate SQL") && ok;

        const SessionStatsRunRecord* matchScript =
            index.findMatchingRun(11, "insert into t values (1);\r\ninsert into t values (2);  ");
        ok = check(matchScript != nullptr && matchScript->runId == 3
            && matchScript->startLine == 40 && matchScript->endLine == 70,
            "Multi-statement script run preserves full [startLine..endLine] across all sub-statements") && ok;

        // Simulate StatementHistory::deleteItems shifting pos 11 -> pos 9
        const SessionStatsRunRecord* matchShifted =
            index.findMatchingRun(9, "insert into t values (1);\ninsert into t values (2);");
        ok = check(matchShifted != nullptr && matchShifted->runId == 3,
            "History lookup falls back to normalized SQL if StatementHistory indices shifted") && ok;

        // Historical SQL from a previous FlameRobin session not run in this session
        const SessionStatsRunRecord* noMatch =
            index.findMatchingRun(3, "select * from OLD_SESSION_TABLE");
        ok = check(noMatch == nullptr,
            "History lookup returns nullptr for statements not executed in current session") && ok;
    }

    std::cout << (ok ? "All StatsOutputModel tests PASSED.\n"
                     : "Some StatsOutputModel tests FAILED.\n");
    return ok ? 0 : 1;
}
