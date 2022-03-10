/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/artprov.h>
#include <wx/display.h>
#include <wx/tokenzr.h>

#include "config/Config.h"
#include "gui/AdvancedMessageDialog.h"
#include "gui/StyleGuide.h"

AdvancedMessageDialogButtons::AdvancedMessageDialogButtons()
{
    affirmativeButtonM.id = wxID_ANY;
    alternateButtonM.id = wxID_ANY;
    negativeButtonM.id = wxID_ANY;
}

void AdvancedMessageDialogButtons::addAffirmativeButton(int id,
    const wxString& caption)
{
    affirmativeButtonM.id = id;
    affirmativeButtonM.caption = caption;
}

void AdvancedMessageDialogButtons::addAlternateButton(int id,
    const wxString& caption)
{
    alternateButtonM.id = id;
    alternateButtonM.caption = caption;
}

void AdvancedMessageDialogButtons::addNegativeButton(int id,
    const wxString& caption)
{
    negativeButtonM.id = id;
    negativeButtonM.caption = caption;
}

wxButton* AdvancedMessageDialogButtons::createAffirmativeButton(
    wxWindow* parent)
{
    if (affirmativeButtonM.id == wxID_ANY)
        return 0;
    return new wxButton(parent, affirmativeButtonM.id,
        affirmativeButtonM.caption);
}

wxButton* AdvancedMessageDialogButtons::createAlternateButton(
    wxWindow* parent)
{
    if (alternateButtonM.id == wxID_ANY)
        return 0;
    return new wxButton(parent, alternateButtonM.id,
        alternateButtonM.caption);
}

wxButton* AdvancedMessageDialogButtons::createNegativeButton(
    wxWindow* parent)
{
    if (negativeButtonM.id == wxID_ANY)
        return 0;
    return new wxButton(parent, negativeButtonM.id, negativeButtonM.caption);
}

int AdvancedMessageDialogButtons::getNumberOfButtons()
{
    int count = 0;
    if (affirmativeButtonM.id != wxID_ANY)
        ++count;
    if (alternateButtonM.id != wxID_ANY)
        ++count;
    if (negativeButtonM.id != wxID_ANY)
        ++count;
    return count;
}

AdvancedMessageDialogButtonsOk::AdvancedMessageDialogButtonsOk(
    const wxString buttonOkCaption)
{
    addAffirmativeButton(wxID_OK, buttonOkCaption);
}

AdvancedMessageDialogButtonsOkCancel::AdvancedMessageDialogButtonsOkCancel(
    const wxString buttonOkCaption, const wxString buttonCancelCaption)
{
    addAffirmativeButton(wxID_OK, buttonOkCaption);
    addNegativeButton(wxID_CANCEL, buttonCancelCaption);
}

AdvancedMessageDialogButtonsYesNoCancel::
    AdvancedMessageDialogButtonsYesNoCancel(const wxString buttonYesCaption,
    const wxString buttonNoCaption, const wxString buttonCancelCaption)
{
    addAffirmativeButton(wxID_YES, buttonYesCaption);
    addAlternateButton(wxID_NO, buttonNoCaption);
    addNegativeButton(wxID_CANCEL, buttonCancelCaption);
}

class TextWrapEngine
{
private:
    static int computeBestWrapWidth(wxDC& dc, const wxString& text,
        int wrapWidth);
    static wxSize computeWrappedExtent(wxDC& dc, const wxString& text,
        int wrapWidth);
    static wxString wrapLine(wxDC& dc, const wxString& text,
        int wrapWidth);
public:
    static void computeWordWrap(const wxString& text, const wxFont& font,
        int wrapWidth, bool minimizeWrapWidth, wxString& wrappedText,
        wxSize* wrappedTextExtent);
};

void TextWrapEngine::computeWordWrap(const wxString& text, const wxFont& font,
    int wrapWidth, bool minimizeWrapWidth, wxString& wrappedText,
    wxSize* wrappedTextExtent)
{
    // split text into lines
    wxArrayString lines;
    wxStringTokenizer tokenizer(text, "\n", wxTOKEN_RET_EMPTY);
    while (tokenizer.HasMoreTokens())
        lines.Add(tokenizer.GetNextToken().Trim());

    // used for computation of text extents
    wxScreenDC dc;
    dc.SetFont(font);

    if (wrapWidth <= 0)
    {
        dc.GetTextExtent("x", &wrapWidth, 0);
        wrapWidth *= 68;
    }

    // compute the minimum width that the wrapped text needs
    if (minimizeWrapWidth)
    {
        int bestWidth = 0;
        for (size_t i = 0; i < lines.size(); i++)
        {
            if (!lines[i].empty())
            {
                int needed = computeBestWrapWidth(dc, lines[i], wrapWidth);
                if (bestWidth < needed)
                    bestWidth = needed;
            }
        }
        if (bestWidth)
            wrapWidth = bestWidth;
    }

    // return the wrapped text
    wrappedText = wxEmptyString;
    for (size_t i = 0; i < lines.size(); i++)
    {
        if (!wrappedText.empty())
            wrappedText += "\n";
        if (!lines[i].empty())
            wrappedText += wrapLine(dc, lines[i], wrapWidth);
    }

    // optionally return the extents of the wrapped text
    if (wrappedTextExtent)
    {
        wxCoord w, h;
        dc.GetMultiLineTextExtent(wrappedText, &w, &h);
        *wrappedTextExtent = wxSize(w, h);
    }
}

int TextWrapEngine::computeBestWrapWidth(wxDC& dc, const wxString& text,
    int wrapWidth)
{
    wxSize origExtent = computeWrappedExtent(dc, text, wrapWidth);
    // don't try to wrap single-line text
    int h;
    dc.GetTextExtent("x", 0, &h);
    if (origExtent.GetHeight() == h)
        return origExtent.GetWidth();

    // binary search for smallest wrap width resulting in same height
    int smallWidth = wrapWidth / 8;
    int largeWidth = wrapWidth;
    while (largeWidth > smallWidth)
    {
        int tryWidth = (largeWidth + smallWidth) / 2;
        wxSize extent = computeWrappedExtent(dc, text, tryWidth);
        if (extent.GetHeight() > origExtent.GetHeight())
            smallWidth = tryWidth + 1;
        else
            largeWidth = tryWidth;
    }
    return largeWidth;
}

wxSize TextWrapEngine::computeWrappedExtent(wxDC& dc, const wxString& text,
    int wrapWidth)
{
    int textW = 0, textH = 0;

    const wxChar* const pos = text.c_str();
    const wxChar* p = text.c_str();
    const wxChar* pWrap = 0;
    const wxChar* r = p;
    while (p && *p)
    {
        // scan over non-whitespace
        while (*r > ' ')
            r++;
        int w;
        dc.GetTextExtent(text.Mid(p-pos, r-p), &w, 0);
        if (w <= wrapWidth) // partial line fits in wrapWidth
            pWrap = r;
        if (w > wrapWidth || *r == 0)
        {
            int h;
            dc.GetTextExtent(text.Mid(p-pos, pWrap-p), &w, &h);
            textW = (w > textW) ? w : textW;
            textH += h;
            p = pWrap;
            // scan over whitespace
            while (p && *p != 0 && *p <= ' ')
                p++;
            pWrap = 0;
        }
        // scan over whitespace
        while (*r != 0 && *r <= ' ')
            r++;
    }
    return wxSize(textW, textH);
}

wxString TextWrapEngine::wrapLine(wxDC& dc, const wxString& text,
    int wrapWidth)
{
    wxString result;

    const wxChar* const pos = text.c_str();
    const wxChar* p = text.c_str();
    const wxChar* pWrap = 0;
    const wxChar* r = p;
    while (p && *p)
    {
        // scan over non-whitespace
        while (*r > ' ')
            r++;
        wxString partialLine = text.Mid(p-pos, r-p);
        int w;
        dc.GetTextExtent(partialLine, &w, 0);
        if (w <= wrapWidth) // partial line fits in wrapWidth
            pWrap = r;
        if (w > wrapWidth || *r == 0)
        {
            if (!result.empty())
                result += "\n";
            result += text.Mid(p-pos, pWrap-p);
            p = pWrap;
            // scan over whitespace
            while (p && *p != 0 && *p <= ' ')
                p++;
            pWrap = 0;
        }
        // scan over whitespace
        while (*r != 0 && *r <= ' ')
            r++;
    }
    return result;
}

AdvancedMessageDialog::AdvancedMessageDialog(wxWindow* parent, wxArtID iconId,
        const wxString& primaryText, const wxString& secondaryText,
        AdvancedMessageDialogButtons& buttons,
        const wxString& dontShowAgainText)
    : BaseDialog(parent, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
    checkBoxM = 0;
#ifndef __WXMAC__
    SetTitle(_("FlameRobin"));
#endif
    wxBoxSizer* controlsSizer = new wxBoxSizer(wxHORIZONTAL);

    // message box icon
    wxStaticBitmap* iconBmp = new wxStaticBitmap(getControlsPanel(), wxID_ANY,
        wxArtProvider::GetBitmap(iconId, wxART_MESSAGE_BOX));
    controlsSizer->Add(iconBmp);
    controlsSizer->AddSpacer(styleguide().getMessageBoxIconMargin());

    wxBoxSizer* textSizer = new wxBoxSizer(wxVERTICAL);
    // primary and secondary texts
    wxStaticText* labelPrimary = new wxStaticText(getControlsPanel(),
        wxID_ANY, wxEmptyString /*primaryText*/);
    wxFont primaryLabelFont(labelPrimary->GetFont());
    primaryLabelFont.MakeBold();
#ifdef __WXGTK__
    primaryLabelFont.SetPointSize(primaryLabelFont.GetPointSize() * 4 / 3);
#endif
    labelPrimary->SetFont(primaryLabelFont);

    wxStaticText* labelSecondary = new wxStaticText(getControlsPanel(),
        wxID_ANY, wxEmptyString /*secondaryText*/);
    wxFont secondaryLabelFont(labelSecondary->GetFont());
#ifdef __WXMAC__
    // default font sizes 13pt and 11pt, but compute it anyway
    secondaryLabelFont.SetPointSize(secondaryLabelFont.GetPointSize() * 11 / 13);
    labelSecondary->SetFont(secondaryLabelFont);
#endif

    wxString primaryTextWrapped;
    wxSize primaryExtent;
    TextWrapEngine::computeWordWrap(primaryText, primaryLabelFont, 0, true,
        primaryTextWrapped, &primaryExtent);
    wxString secondaryTextWrapped;
    wxSize secondaryExtent;
    TextWrapEngine::computeWordWrap(secondaryText, secondaryLabelFont, 0, true,
        secondaryTextWrapped, &secondaryExtent);

    int wrapWidth = (primaryExtent.GetWidth() > secondaryExtent.GetWidth()) ?
        primaryExtent.GetWidth() : secondaryExtent.GetWidth();
    TextWrapEngine::computeWordWrap(primaryText, primaryLabelFont, wrapWidth,
        false, primaryTextWrapped, 0);
    labelPrimary->SetLabel(primaryTextWrapped);

    TextWrapEngine::computeWordWrap(secondaryText, secondaryLabelFont,
        wrapWidth, false, secondaryTextWrapped, 0);
    labelSecondary->SetLabel(secondaryTextWrapped);

    textSizer->Add(labelPrimary, 0, wxEXPAND);
    textSizer->AddSpacer(styleguide().getMessageBoxBetweenTextMargin());
    textSizer->Add(labelSecondary, 0, wxEXPAND);

    // checkbox for "Don't show/ask again"
    if (!dontShowAgainText.empty())
    {
        textSizer->AddSpacer(
            styleguide().getUnrelatedControlMargin(wxVERTICAL));
        checkBoxM = new wxCheckBox(getControlsPanel(), wxID_ANY,
            dontShowAgainText);
        textSizer->Add(checkBoxM, 0, wxEXPAND);
    }

    controlsSizer->Add(textSizer);

    // command buttons
    wxButton* affirmativeButton = buttons.createAffirmativeButton(
        getControlsPanel());
    if (affirmativeButton)
    {
        Connect(affirmativeButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(AdvancedMessageDialog::OnButtonClick));
    }
    wxButton* alternateButton = buttons.createAlternateButton(
        getControlsPanel());
    if (alternateButton)
    {
        Connect(alternateButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(AdvancedMessageDialog::OnButtonClick));
    }
    wxButton* negativeButton = buttons.createNegativeButton(
        getControlsPanel());
    if (negativeButton)
    {
        Connect(negativeButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
            wxCommandEventHandler(AdvancedMessageDialog::OnButtonClick));
    }

    if (affirmativeButton)
    {
        affirmativeButton->SetDefault();
        affirmativeButton->SetFocus();
    }
    else if (alternateButton)
    {
        alternateButton->SetDefault();
        alternateButton->SetFocus();
    }
    else if (negativeButton)
    {
        negativeButton->SetDefault();
        negativeButton->SetFocus();
    }

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* buttonSizer = styleguide().createButtonSizer(affirmativeButton,
        negativeButton, alternateButton);
    // use method in base class to set everything up
    layoutSizers(controlsSizer, buttonSizer);
}

bool AdvancedMessageDialog::getDontShowAgain() const
{
    return checkBoxM != 0 && checkBoxM->IsChecked();
}

void AdvancedMessageDialog::OnButtonClick(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case wxID_OK:
            EndModal(wxOK);
            break;
        case wxID_YES:
            EndModal(wxYES);
            break;
        case wxID_NO:
            EndModal(wxNO);
            break;
        case wxID_CANCEL:
            EndModal(wxCANCEL);
            break;
    }
}

int showAdvancedMessageDialog(wxWindow* parent, int style,
    const wxString& primaryText, const wxString& secondaryText,
    AdvancedMessageDialogButtons& buttons, bool* showNeverAgain = 0,
    const wxString& dontShowAgainText = wxEmptyString)
{
    if (!parent)
        parent = ::wxGetActiveWindow();
    if (showNeverAgain)
        *showNeverAgain = false;
    wxArtID iconId;
    switch (style)
    {
        case wxICON_INFORMATION:
            iconId = wxART_INFORMATION;
            break;
        case wxICON_ERROR:
            iconId = wxART_ERROR;
            break;
        default:
            // NOTE: wxART_QUESTION is deprecated in HIGs, so don't use it...
            iconId = wxART_WARNING;
            break;
    }

    AdvancedMessageDialog amd(parent, iconId, primaryText, secondaryText,
        buttons, dontShowAgainText);
    int res = amd.ShowModal();
    if (showNeverAgain)
        *showNeverAgain = amd.getDontShowAgain();
    return res;
}

int showAdvancedMessageDialog(wxWindow* parent, int style,
    const wxString& primaryText, const wxString& secondaryText,
    AdvancedMessageDialogButtons& buttons, Config& config,
    const wxString& configKey, const wxString& dontShowAgainText)
{
    int value;
    if (config.getValue(configKey, value))
        return value;

    bool showNeverAgain = false;
    value = showAdvancedMessageDialog(parent, style, primaryText,
        secondaryText, buttons, &showNeverAgain, dontShowAgainText);
    // wxID_CANCEL means: cancel action, so it is not treated like a regular
    // "choice"; the checkbox ("Don't show/ask again") is ignored even if set
    if (value != wxCANCEL && !configKey.empty() && showNeverAgain)
        config.setValue(configKey, value);
    return value;
}

int showInformationDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons)
{
    return showAdvancedMessageDialog(parent, wxICON_INFORMATION, primaryText,
        secondaryText, buttons);
}

int showInformationDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons,
    Config& config, const wxString& configKey,
    const wxString& dontShowAgainText)
{
    return showAdvancedMessageDialog(parent, wxICON_INFORMATION, primaryText,
        secondaryText, buttons, config, configKey, dontShowAgainText);
}

int showQuestionDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons)
{
    return showAdvancedMessageDialog(parent, wxICON_QUESTION, primaryText,
        secondaryText, buttons);
}

int showQuestionDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons,
    Config& config, const wxString& configKey,
    const wxString& dontShowAgainText)
{
    return showAdvancedMessageDialog(parent, wxICON_QUESTION, primaryText,
        secondaryText, buttons, config, configKey, dontShowAgainText);
}

int showWarningDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons)
{
    return showAdvancedMessageDialog(parent, wxICON_WARNING, primaryText,
        secondaryText, buttons);
}

int showWarningDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons,
    Config& config, const wxString& configKey,
    const wxString& dontShowAgainText)
{
    return showAdvancedMessageDialog(parent, wxICON_WARNING, primaryText,
        secondaryText, buttons, config, configKey, dontShowAgainText);
}

int showErrorDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons)
{
    return showAdvancedMessageDialog(parent, wxICON_ERROR, primaryText,
        secondaryText, buttons);
}

int showErrorDialog(wxWindow* parent, const wxString& primaryText,
    const wxString& secondaryText, AdvancedMessageDialogButtons buttons,
    Config& config, const wxString& configKey,
    const wxString& dontShowAgainText)
{
    return showAdvancedMessageDialog(parent, wxICON_ERROR, primaryText,
        secondaryText, buttons, config, configKey, dontShowAgainText);
}

