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

#ifndef FR_STATSOUTPUTCTRL_H
#define FR_STATSOUTPUTCTRL_H

#include <wx/stc/stc.h>

#include <functional>
#include <vector>

#include "gui/StatsOutputModel.h"

class Config;

StatsExpandPrefs loadStatsExpandPrefs(Config& cfg);

class StatsOutputCtrl : public wxStyledTextCtrl
{
public:
    static constexpr int kFoldMarginIndex = 2;
    static constexpr int kIndicatorHistoryMatch = 9;

    StatsOutputCtrl(wxWindow* parent, wxWindowID id, bool isHistoryPane = false);

    void setupStyles();
    void setClearHistoryCallback(std::function<void()> cb);

    void clearAllContent();

    int beginStatementRun(
        const wxString& runLabel,
        const wxString& timeStr,
        const wxString& sqlPreview);

    void finishStatementRun(
        const wxString& statusText,
        bool isError = false);

    void beginPrepareGroup(const wxString& sql, bool startedTransaction);
    void completePrepareGroup(
        const wxString& sql,
        bool startedTransaction,
        const wxString& elapsedStr,
        bool defaultExpanded);

    void beginExecutionGroup();
    void completeExecutionGroup(
        const wxString& elapsedStr,
        const wxString& affectedRowsMsg,
        bool defaultExpanded);

    void appendGroupLines(const std::vector<StatsFormattedLine>& lines);
    void appendStandaloneLine(
        const wxString& text,
        StatsStyle style = StatsStyle::Default,
        int foldDepth = 0);

    int getLastContentLine() const;

    void expandAll();
    void collapseAll(bool collapseTopLevelRuns);

    void highlightLineRange(int startLine, int endLine);
    void clearHistoryHighlight();

private:
    bool isHistoryPaneM = false;
    std::function<void()> clearHistoryCallbackM;

    int activeRunHeaderLineM = -1;
    wxString activeRunLabelM;
    wxString activeRunTimeStrM;
    wxString activeRunSqlPreviewM;

    int activePrepareStartLineM = -1;
    int activePrepareLineCountM = 0;

    int activeExecStartLineM = -1;
    int activeExecLineCountM = 0;

    int appendFormattedLineInternal(const StatsFormattedLine& line);
    void styleAndFoldLine(int lineIdx, const StatsFormattedLine& line);
    void replaceLineBlock(
        int startLine,
        int oldLineCount,
        const std::vector<StatsFormattedLine>& newLines);
    void applyGroupFoldState(int headerLine, bool expanded);

    void OnContextMenu(wxContextMenuEvent& event);
};

#endif // FR_STATSOUTPUTCTRL_H
