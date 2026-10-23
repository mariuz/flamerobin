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

#include "wx/wxprec.h"
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/ffile.h>

#include <algorithm>

#include "gui/BenchmarkCompareDialog.h"

bool readBenchmarkReportFile(wxWindow* parent, const wxString& path,
    fr::BenchmarkSnapshot& snapshot, wxString* text, bool* comparisonPage)
{
    if (comparisonPage)
        *comparisonPage = false;
    wxFFile file(path, "r");
    wxString content;
    fr::BenchmarkSnapshotResult result;
    if (file.IsOpened() && file.ReadAll(&content, wxConvUTF8))
        result = fr::parseBenchmarkSnapshot(content);
    if (!result.ok() && fr::isBenchmarkComparisonPage(content))
    {
        if (comparisonPage)
        {
            *comparisonPage = true;
            if (text)
                *text = content;
            return true;
        }
        wxMessageBox(path + "\n\n" + _("This is a saved comparison of two runs. It holds no "
            "results of its own. Please select a report of a single run."),
            _("Performance Benchmark & Diagnosis"), wxOK | wxICON_INFORMATION, parent);
        return false;
    }
    if (!result.ok())
    {
        // older reports are refused with a message, never compared wrongly
        wxMessageBox(path + "\n\n" + fr::getBenchmarkSnapshotProblem(result),
            _("Performance Benchmark & Diagnosis"), wxOK | wxICON_INFORMATION, parent);
        return false;
    }
    snapshot = result.snapshot;
    if (text)
        *text = content;
    return true;
}

BenchmarkCompareDialog::BenchmarkCompareDialog(wxWindow* parent,
    const fr::BenchmarkResultStore& results, int first, int second)
    : BaseDialog(parent, wxID_ANY, _("Compare results"))
{
    for (int id : results.getComparableIds())
        sourcesM.push_back(*results.find(id));

    wxPanel* panel = getControlsPanel();
    const int gap = FromDIP(8);
    wxBoxSizer* sizerMain = new wxBoxSizer(wxVERTICAL);
    wxStaticText* intro = new wxStaticText(panel, wxID_ANY, _("Compares two results. The "
        "earlier run is the reference and gets 1000 points; the later run gets more points "
        "when it is faster. The order follows the date; Swap reverses it."));
    intro->Wrap(FromDIP(520));
    sizerMain->Add(intro, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    wxFlexGridSizer* sizerChoices = new wxFlexGridSizer(2, gap, gap);
    sizerChoices->AddGrowableCol(1, 1);
    sizerChoices->Add(new wxStaticText(panel, wxID_ANY, _("&First result:")), 0,
        wxALIGN_CENTER_VERTICAL);
    choice_first = new wxChoice(panel, wxID_ANY);
    // GTK makes a list as wide as its longest entry
    choice_first->SetMinSize(wxSize(FromDIP(420), -1));
    sizerChoices->Add(choice_first, 1, wxEXPAND);
    sizerChoices->Add(new wxStaticText(panel, wxID_ANY, _("&Second result:")), 0,
        wxALIGN_CENTER_VERTICAL);
    choice_second = new wxChoice(panel, wxID_ANY);
    choice_second->SetMinSize(wxSize(FromDIP(420), -1));
    sizerChoices->Add(choice_second, 1, wxEXPAND);
    sizerMain->Add(sizerChoices, 0, wxEXPAND | wxBOTTOM, FromDIP(12));

    wxBoxSizer* sizerOrder = new wxBoxSizer(wxHORIZONTAL);
    label_order = new wxStaticText(panel, wxID_ANY, wxEmptyString);
    sizerOrder->Add(label_order, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, gap);
    button_swap = new wxButton(panel, wxID_ANY, _("S&wap"));
    button_swap->SetToolTip(_("The later result becomes the reference, e.g. for reports "
        "made in another time zone."));
    sizerOrder->Add(button_swap, 0);
    sizerMain->Add(sizerOrder, 0, wxEXPAND | wxBOTTOM, gap);
    label_check = new wxStaticText(panel, wxID_ANY, wxEmptyString);
    sizerMain->Add(label_check, 0, wxEXPAND | wxBOTTOM, gap);
    label_files = new wxStaticText(panel, wxID_ANY, _("Files chosen here are only read; "
        "they do not get a tab (Open... shows an HTML report)."));
    label_files->Wrap(FromDIP(520));
    sizerMain->Add(label_files, 0, wxEXPAND);

    wxStdDialogButtonSizer* sizerButtons = new wxStdDialogButtonSizer();
    button_compare = new wxButton(panel, wxID_OK, _("&Compare"));
    button_compare->SetDefault();
    sizerButtons->AddButton(button_compare);
    sizerButtons->AddButton(new wxButton(panel, wxID_CANCEL, _("Cancel")));
    sizerButtons->Realize();

    fillChoices();
    auto indexOf = [this](int id)
    {
        for (size_t i = 0; i < sourcesM.size(); ++i)
        {
            if (id != 0 && sourcesM[i].id == id)
                return int(i);
        }
        return int(wxNOT_FOUND);
    };
    choice_first->SetSelection(indexOf(first));
    choice_second->SetSelection(indexOf(second));
    lastFirstM = choice_first->GetSelection();
    lastSecondM = choice_second->GetSelection();
    choice_first->Bind(wxEVT_CHOICE, &BenchmarkCompareDialog::OnChoice, this);
    choice_second->Bind(wxEVT_CHOICE, &BenchmarkCompareDialog::OnChoice, this);
    button_swap->Bind(wxEVT_BUTTON, &BenchmarkCompareDialog::OnSwap, this);

    layoutSizers(sizerMain, sizerButtons);
    update();
    Centre();
    (choice_second->GetSelection() == wxNOT_FOUND ? choice_second : choice_first)->SetFocus();
}

void BenchmarkCompareDialog::fillChoices()
{
    for (wxChoice* choice : { choice_first, choice_second })
    {
        const int selection = choice->GetSelection();
        choice->Clear();
        for (const fr::BenchmarkResultEntry& source : sourcesM)
            choice->Append(fr::describeBenchmarkResult(source));
        choice->Append(_("Choose a saved report..."));
        choice->SetSelection(selection);
    }
}

const fr::BenchmarkResultEntry* BenchmarkCompareDialog::getChosen(wxChoice* choice) const
{
    const int selection = choice->GetSelection();
    if (selection < 0 || selection >= int(sourcesM.size()))
        return nullptr;
    return &sourcesM[selection];
}

fr::BenchmarkPair BenchmarkCompareDialog::getPair() const
{
    fr::BenchmarkPair pair = fr::orderBenchmarkPair(*getChosen(choice_first),
        *getChosen(choice_second));
    if (swappedM)
        std::swap(pair.older, pair.newer);
    return pair;
}

const fr::BenchmarkResultEntry& BenchmarkCompareDialog::getOlder() const
{
    return *getPair().older;
}

const fr::BenchmarkResultEntry& BenchmarkCompareDialog::getNewer() const
{
    return *getPair().newer;
}

void BenchmarkCompareDialog::update()
{
    const fr::BenchmarkResultEntry* first = getChosen(choice_first);
    const fr::BenchmarkResultEntry* second = getChosen(choice_second);
    wxString order = "--";
    wxString checks;
    bool comparable = first && second;
    if (comparable)
    {
        const fr::BenchmarkPair pair = getPair();
        for (fr::BenchmarkPairCheck check : fr::checkBenchmarkPair(*pair.older, *pair.newer))
        {
            if (check == fr::BenchmarkPairCheck::SameResult)
                comparable = false;
            checks += (checks.empty() ? "" : "\n")
                + fr::describeBenchmarkPairCheck(check, *pair.older, *pair.newer);
        }
        if (comparable)
            order = fr::describeBenchmarkPairOrder(*pair.older, *pair.newer);
    }
    label_order->SetLabel(order);
    label_order->Wrap(FromDIP(420));
    label_check->SetLabel(checks);
    label_check->Wrap(FromDIP(520));
    // only where a file of this dialog is chosen
    label_files->Show((first && first->id == 0) || (second && second->id == 0));
    button_swap->Enable(comparable);
    button_compare->Enable(comparable);
    // as high as the lines need, with the choices as wide as they were
    wxSizer* sizer = getControlsPanel()->GetSizer();
    SetMinSize(wxDefaultSize);
    sizer->Fit(this);
    sizer->SetSizeHints(this);
}

void BenchmarkCompareDialog::OnChoice(wxCommandEvent& event)
{
    wxChoice* choice = event.GetEventObject() == choice_first ? choice_first : choice_second;
    int& last = choice == choice_first ? lastFirstM : lastSecondM;
    if (choice->GetSelection() == int(sourcesM.size()))
    {
        // a saved report, read now; it gets no tab
        int selection = last;
        wxFileDialog fd(this, _("Select a saved report"), "", "",
            _("Benchmark reports (*.html;*.md)|*.html;*.htm;*.md|All files (*.*)|*.*"),
            wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        fr::BenchmarkSnapshot snapshot;
        if (fd.ShowModal() == wxID_OK && readBenchmarkReportFile(this, fd.GetPath(), snapshot))
        {
            auto it = std::find_if(sourcesM.begin(), sourcesM.end(),
                [&fd](const fr::BenchmarkResultEntry& e) { return e.path == fd.GetPath(); });
            if (it == sourcesM.end())
            {
                fr::BenchmarkResultEntry file;
                file.kind = fr::BenchmarkResultKind::SavedReport;
                file.snapshot = snapshot;
                file.path = fd.GetPath();
                file.saved = true;
                sourcesM.push_back(file);
                fillChoices();
                it = sourcesM.end() - 1;
            }
            selection = int(it - sourcesM.begin());
        }
        choice->SetSelection(selection);
    }
    last = choice->GetSelection();
    swappedM = false;
    update();
}

void BenchmarkCompareDialog::OnSwap(wxCommandEvent& WXUNUSED(event))
{
    swappedM = !swappedM;
    update();
}
