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

#ifndef FR_STATSOUTPUTMODEL_H
#define FR_STATSOUTPUTMODEL_H

#include <wx/string.h>

#include <cstddef>
#include <vector>

enum class StatsStyle : int
{
    Default = 0,
    Error = 1,
    Sql = 2,
    RunHeader = 3,
    GroupHeader = 4,
    Plan = 5,
    MetricLabel = 6,
    MetricValue = 7,
    MetricWarning = 8
};

struct StatsStyleSpan
{
    size_t byteOffset = 0;
    size_t byteLength = 0;
    StatsStyle style = StatsStyle::Default;
};

struct StatsFormattedLine
{
    wxString text;
    int foldDepth = 0;
    bool isFoldHeader = false;
    bool defaultExpanded = true;
    std::vector<StatsStyleSpan> spans;
};

struct StatsExpandPrefs
{
    bool clearStatsOnExecute = true;
    bool showStatsHistory = true;
    bool expandPrepare = false;
    bool expandFields = false;
    bool expandPlan = true;
    bool expandParameters = false;
    bool expandExecution = false;
    bool expandBufferMetrics = true;
    bool expandRecordMetrics = true;
    bool expandDmlMetrics = true;
    bool expandMemoryMetrics = true;
    bool expandTableReads = true;
};

struct StatsTableActivityItem
{
    wxString relationName;
    int inserts = 0;
    int updates = 0;
    int deletes = 0;
    int indexedReads = 0;
    int sequentialReads = 0;
};

struct SessionStatsRunRecord
{
    int runId = 0;
    bool hasHistoryPosition = false;
    size_t historyPosition = 0;
    wxString normalizedSql;
    wxString timeStr;
    int startLine = 0;
    int endLine = 0;
};

class SessionStatsHistoryIndex
{
private:
    std::vector<SessionStatsRunRecord> recordsM;

public:
    static wxString normalizeSql(const wxString& sql);

    void clear();
    void startRun(int runId, const wxString& timeStr, int startLine);
    void updateRunEndLine(int runId, int endLine);
    void bindRunToHistory(int runId, size_t historyPos, const wxString& sql);

    const SessionStatsRunRecord* findMatchingRun(
        size_t historyPos, const wxString& sql) const;

    const std::vector<SessionStatsRunRecord>& getRecords() const
    {
        return recordsM;
    }
};

namespace StatsFormatter
{
wxString extractSqlPreview(const wxString& sql, size_t maxChars = 72);
wxString formatRunLabel(int runId, int statementIdx, int totalStatementsInRun);

StatsFormattedLine formatRunHeaderLine(
    const wxString& runLabel,
    const wxString& timeStr,
    const wxString& sqlPreview,
    const wxString& statusText,
    bool isError = false);

StatsFormattedLine formatStandaloneLine(
    const wxString& text,
    StatsStyle style = StatsStyle::Default,
    int foldDepth = 0);

std::vector<StatsFormattedLine> formatPrepareGroup(
    const wxString& sql,
    bool startedTransaction,
    const wxString& elapsedStr,
    bool inProgress,
    bool defaultExpanded);

std::vector<StatsFormattedLine> formatFieldsGroup(
    const std::vector<wxString>& fieldLines,
    bool defaultExpanded);

std::vector<StatsFormattedLine> formatPlanGroup(
    const wxString& planText,
    bool defaultExpanded);

std::vector<StatsFormattedLine> formatParametersGroup(
    int paramCount,
    bool defaultExpanded);

std::vector<StatsFormattedLine> formatExecutionGroup(
    const wxString& elapsedStr,
    const wxString& affectedRowsMsg,
    bool inProgress,
    bool defaultExpanded);

std::vector<StatsFormattedLine> formatBufferMetricsGroup(
    int fetches,
    int reads,
    int writes,
    int marks,
    bool defaultExpanded);

std::vector<StatsFormattedLine> formatRecordMetricsGroup(
    int indexedReads,
    int sequentialScans,
    bool defaultExpanded);

std::vector<StatsFormattedLine> formatDmlMetricsGroup(
    int inserts,
    int updates,
    int deletes,
    bool defaultExpanded);

std::vector<StatsFormattedLine> formatMemoryMetricsGroup(
    int deltaBytes,
    bool defaultExpanded);

std::vector<StatsFormattedLine> formatTableActivityGroup(
    const std::vector<StatsTableActivityItem>& items,
    bool defaultExpanded);
} // namespace StatsFormatter

#endif // FR_STATSOUTPUTMODEL_H
