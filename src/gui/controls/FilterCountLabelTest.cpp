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

#include <cstdlib>
#include <iostream>
#include <wx/wx.h>
#include <wx/init.h>

#include "gui/controls/ControlUtils.h"

namespace
{
bool check(bool condition, const char* testName)
{
    if (condition)
    {
        std::cout << "[PASS] " << testName << std::endl;
        return true;
    }
    std::cerr << "[FAIL] " << testName << std::endl;
    return false;
}
}

int main(int argc, char** argv)
{
    bool ok = true;

    // 1. Headless formatting & null-safety checks (always run in ctest)
    {
        wxInitializer init(argc, argv);
        if (!init.IsOk())
        {
            std::cerr << "Failed to initialize wxBase.\n";
            return 1;
        }

        ok = check(formatFilterCountLabel(0, 0) == "Rows: 0",
            "Unfiltered empty table formats as 'Rows: 0'") && ok;
        ok = check(formatFilterCountLabel(250, 250) == "Rows: 250",
            "Unfiltered populated table formats as 'Rows: 250'") && ok;
        ok = check(formatFilterCountLabel(12, 250) == "Filtered: 12 / 250 rows",
            "Filtered table formats as 'Filtered: 12 / 250 rows'") && ok;
        ok = check(!setStaticTextAndRelayout(nullptr, "Rows: 10", nullptr),
            "setStaticTextAndRelayout handles nullptr safely") && ok;
    }

    // 2. Full GUI sizer layout regression test for issue #772 when a display is present
#if defined(__WXGTK__) && !defined(__WXOSX__) && !defined(__WXMSW__)
    const bool hasDisplay = (std::getenv("DISPLAY") != nullptr && *std::getenv("DISPLAY") != '\0')
        || (std::getenv("WAYLAND_DISPLAY") != nullptr && *std::getenv("WAYLAND_DISPLAY") != '\0');
#else
    const bool hasDisplay = true;
#endif

    if (hasDisplay)
    {
        wxApp* app = new wxApp();
        wxApp::SetInstance(app);
        if (wxEntryStart(argc, argv))
        {
            wxFrame* frame = new wxFrame(nullptr, wxID_ANY, "FilterCountTest",
                wxDefaultPosition, wxSize(500, 300));
            wxPanel* pane = new wxPanel(frame, wxID_ANY);
            wxBoxSizer* sizerDataPane = new wxBoxSizer(wxVERTICAL);
            wxBoxSizer* sizerFilter = new wxBoxSizer(wxHORIZONTAL);

            sizerFilter->Add(new wxStaticText(pane, wxID_ANY, "Filter:"),
                0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
            wxTextCtrl* filterCtrl = new wxTextCtrl(pane, wxID_ANY, wxEmptyString);
            sizerFilter->Add(filterCtrl, 1, wxEXPAND | wxRIGHT, 5);
            wxButton* clearBtn = new wxButton(pane, wxID_ANY, "Clear",
                wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
            sizerFilter->Add(clearBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
            wxStaticText* countLabel = new wxStaticText(pane, wxID_ANY, "0 rows");
            sizerFilter->Add(countLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);

            sizerDataPane->Add(sizerFilter, 0, wxEXPAND | wxALL, 4);
            sizerDataPane->Add(new wxPanel(pane, wxID_ANY), 1, wxEXPAND);
            pane->SetSizer(sizerDataPane);

            wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
            frameSizer->Add(pane, 1, wxEXPAND);
            frame->SetSizer(frameSizer);
            frame->Layout();
            pane->SetSize(0, 0, 480, 260);
            pane->Layout();

            int initialFilterWidth = filterCtrl->GetSize().GetWidth();

            // Simulate initial data load ("0 rows" -> "Rows: 123456")
            bool changed1 = setStaticTextAndRelayout(
                countLabel, formatFilterCountLabel(123456, 123456), pane);
            ok = check(changed1, "Label update from '0 rows' to 'Rows: 123456' reports change") && ok;

            int paneWidth = pane->GetClientSize().GetWidth();
            int rightEdge1 = countLabel->GetPosition().x + countLabel->GetSize().GetWidth();
            int bestWidth1 = countLabel->GetBestSize().GetWidth();
            ok = check(countLabel->GetSize().GetWidth() >= bestWidth1,
                "Count label width accommodates full 'Rows: 123456' best width") && ok;
            ok = check(rightEdge1 <= paneWidth,
                "Count label right edge stays inside panel client width without window resize") && ok;

            // Simulate filtering ("Rows: 123456" -> "Filtered: 42 / 123456 rows")
            bool changed2 = setStaticTextAndRelayout(
                countLabel, formatFilterCountLabel(42, 123456), pane);
            ok = check(changed2, "Label update to 'Filtered: 42 / 123456 rows' reports change") && ok;

            int rightEdge2 = countLabel->GetPosition().x + countLabel->GetSize().GetWidth();
            int bestWidth2 = countLabel->GetBestSize().GetWidth();
            ok = check(countLabel->GetSize().GetWidth() >= bestWidth2,
                "Count label width accommodates full 'Filtered: 42 / 123456 rows' best width") && ok;
            ok = check(rightEdge2 <= paneWidth,
                "Filtered count label right edge stays inside panel client width") && ok;
            ok = check(filterCtrl->GetSize().GetWidth() < initialFilterWidth,
                "Filter text control shrank horizontally to make room for wider count label") && ok;

            // Idempotence check (same text does not trigger redundant update)
            bool changed3 = setStaticTextAndRelayout(
                countLabel, formatFilterCountLabel(42, 123456), pane);
            ok = check(!changed3, "Identical label text skips redundant relayout") && ok;

            frame->Destroy();
            wxEntryCleanup();
        }
    }

    return ok ? 0 : 1;
}
