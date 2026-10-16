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

#ifndef FR_BENCHMARKPROGRESSPANEL_H
#define FR_BENCHMARKPROGRESSPANEL_H

#include <wx/wx.h>
#include <wx/bmpbndl.h>
#include <wx/timer.h>

#include <chrono>
#include <vector>

#include "engine/Benchmark.h"

class BenchmarkLogo;
class BenchmarkStageStrip;

// the colours of the benchmark dialog come from the system, so that they
// follow its dark mode and its high contrast: a mix of two colours with
// this share of the first one, the contrast ratio of WCAG 2, and a colour
// moved towards the text colour until it has the contrast on the background
wxColour mixBenchmarkColours(const wxColour& first, const wxColour& second, double share);
double getBenchmarkContrast(const wxColour& first, const wxColour& second);
wxColour getReadableBenchmarkColour(const wxColour& colour, const wxColour& text,
    const wxColour& background, double contrast = 4.5);

///
/// The progress of a benchmark run: the FlameRobin logo, which fills from
/// the bottom with the share of the run, the share in percent, the step,
/// and the stages in a row, each with a mark of its own shape and its word.
/// Drawn from bitmaps
/// with wxDC only; one timer moves the edge of the filling and lets the
/// robin blink while nothing is measured that it could disturb.
///
class BenchmarkProgressPanel : public wxPanel
{
public:
    using Outcome = fr::BenchmarkStageTracker::Outcome;

    explicit BenchmarkProgressPanel(wxWindow* parent);
    ~BenchmarkProgressPanel();

    /// before a run: every stage of the plan waits, the hint says how long
    /// the run takes
    void showReady(const std::vector<fr::BenchmarkStagePlan>& plan, const wxString& hint);
    bool isReady() const { return readyM; }
    void start(const std::vector<fr::BenchmarkStagePlan>& plan);
    void update(const fr::BenchmarkProgress& progress);
    void cancelRequested();
    /// the end of a run: message is the line below the percentage
    void finish(Outcome outcome, bool cleanupFailed, const wxString& message);

private:
    fr::BenchmarkStageTracker trackerM;
    bool readyM = true;
    bool runningM = false;
    bool mutedM = false;
    std::chrono::steady_clock::time_point startM;
    long elapsedSecondsM = -1;

    // what the animation depends on, see getBenchmarkAnimation()
    fr::BenchmarkAnimationContext animationM;
    fr::BenchmarkAnimation modeM = fr::BenchmarkAnimation::Still;
    wxTimer timerM;
    int intervalM = 0;
    // the pictures since the movement started
    int ticksM = 0;

    BenchmarkLogo* logoM;
    wxStaticText* percentM;
    wxStaticText* elapsedM;
    wxStaticBitmap* markM;
    wxStaticText* wordM;
    wxStaticText* stepM;
    BenchmarkStageStrip* stripM;

    // the marks of the end of a run: finished, cancelled, failed
    wxBitmapBundle marksM[3];
    int markIndexM = -1;
    void createMarks();
    void setStepLine(int mark, const wxString& word, const wxColour& wordColour,
        const wxString& text, const wxColour& textColour);
    void setFraction(double fraction);
    void updateStages();
    void readEnvironment();
    void updateTimer();
    void updateElapsed();
    void OnTimer(wxTimerEvent& event);
    void OnDPIChanged(wxDPIChangedEvent& event);
};

#endif // FR_BENCHMARKPROGRESSPANEL_H
