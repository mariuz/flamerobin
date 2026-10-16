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

#ifndef FR_BENCHMARKCOMPAREDIALOG_H
#define FR_BENCHMARKCOMPAREDIALOG_H

#include <wx/wx.h>

#include <vector>

#include "engine/BenchmarkResults.h"
#include "gui/BaseDialog.h"

/// reads the results of a saved report (HTML or Markdown) and, if wanted,
/// its text; says why it cannot be read. A saved comparison has no results:
/// it is refused, unless comparisonPage is given, which then tells that it is one
bool readBenchmarkReportFile(wxWindow* parent, const wxString& path,
    fr::BenchmarkSnapshot& snapshot, wxString* text = nullptr,
    bool* comparisonPage = nullptr);

///
/// "Compare results": two lists with the tests and opened files of the
/// dialog and "Choose a saved report...", which reads a file without
/// giving it a tab, so that two open results, an open result and a file,
/// or two files without a test can be compared. The earlier one is the
/// reference; Swap reverses the order.
///
class BenchmarkCompareDialog : public BaseDialog
{
public:
    /// first and second: the results chosen at first, 0 for none
    BenchmarkCompareDialog(wxWindow* parent, const fr::BenchmarkResultStore& results,
        int first, int second);

    /// the chosen results in the order of the comparison, after "Compare"
    const fr::BenchmarkResultEntry& getOlder() const;
    const fr::BenchmarkResultEntry& getNewer() const;

private:
    // copies of the results, then the files chosen here (number 0)
    std::vector<fr::BenchmarkResultEntry> sourcesM;
    wxChoice* choice_first;
    wxChoice* choice_second;
    wxStaticText* label_order;
    wxButton* button_swap;
    wxStaticText* label_check;
    wxStaticText* label_files;
    wxButton* button_compare;
    // the order after Swap, as long as the choice stays
    bool swappedM = false;
    // the list of the last choice, to go back to after a cancelled file
    int lastFirstM = wxNOT_FOUND;
    int lastSecondM = wxNOT_FOUND;

    void fillChoices();
    // nullptr where "Choose a saved report..." or nothing is chosen
    const fr::BenchmarkResultEntry* getChosen(wxChoice* choice) const;
    fr::BenchmarkPair getPair() const;
    void update();
    void OnChoice(wxCommandEvent& event);
    void OnSwap(wxCommandEvent& event);
};

#endif // FR_BENCHMARKCOMPAREDIALOG_H
