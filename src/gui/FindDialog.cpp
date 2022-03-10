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

#include <wx/utils.h>

#include "gui/FindDialog.h"
#include "gui/StyleGuide.h"

FindFlags::FindFlags()
{
    flags = se::DEFAULT;
}

bool FindFlags::has(unsigned int flag) const
{
    return ((flags & flag) == flag);
}

void FindFlags::remove(unsigned int flag)
{
    flags &= ~flag;
}

void FindFlags::add(unsigned int flag)
{
    flags |= flag;
}

int FindFlags::asStc() const            // returns "flags" converted to wxSTC search flags
{
    int retval = 0;
    if (has(se::WHOLE_WORD))
        retval |= wxSTC_FIND_WHOLEWORD;
    if (has(se::MATCH_CASE))
        retval |= wxSTC_FIND_MATCHCASE;
    if (has(se::REGULAR_EXPRESSION))
        retval |= wxSTC_FIND_REGEXP;
    return retval;
}

// Used for debugging
void FindFlags::show() const
{
    wxString retval(wxString::Format("Flags (%d) contains:\n", flags));
    if (has(se::WHOLE_WORD))
        retval += "se::WHOLE_WORD\n";
    if (has(se::MATCH_CASE))
        retval += "se::MATCH_CASE\n";
    if (has(se::REGULAR_EXPRESSION))
        retval += "se::REGULAR_EXPRESSION\n";
    if (has(se::FROM_TOP))
        retval += "se::FROM_TOP\n";
    if (has(se::WRAP))
        retval += "se::WRAP\n";
    if (has(se::CONVERT_BACKSLASH))
        retval += "se::CONVERT_BACKSLASH\n";
    wxMessageBox(retval);
}

FindFlags& FindFlags::operator= (const FindFlags& source)
{
    flags = source.flags;
    return *this;
}

//! wxStyledTextCtrl with Search&Replace capability
SearchableEditor::SearchableEditor(wxWindow *parent, wxWindowID id)
    : wxStyledTextCtrl(parent, id, wxDefaultPosition, wxDefaultSize,
        wxBORDER_THEME), fd(0)
{
}

wxString SearchableEditor::convertBackslashes(const wxString& source)
{
    wxString result(source);
    result.Replace("\\n", "\n");
    result.Replace("\\r", "\r");
    result.Replace("\\t", "\t");
    while (true)    // hexadecimal bytes can be given if format: \xNN where NN is in range from 00 to ff
    {
        int p = result.Find("\\x");
        if (p == -1)
            break;
        unsigned long number;
        if (result.Mid(p+2, 2).ToULong(&number, 16))
        {
            result.Remove(p, 3);
            result[p] = (wxChar)number;
        }
        else
        {
            wxMessageBox(_("Only single-byte hexadecimal numbers are allowed\nin format: \\xNN, where NN is in range 00-ff"), _("Error in escape sequence"), wxICON_WARNING|wxOK);
            break;
        }
    }
    result.Replace("\\\\", "\\");
    return result;
}

void SearchableEditor::setupSearch(const wxString& findText, const wxString& replaceText, const FindFlags& flags)
{
    findFlagsM = flags;
    if (findFlagsM.has(se::CONVERT_BACKSLASH))
    {
        findTextM = convertBackslashes(findText);
        replaceTextM = convertBackslashes(replaceText);
    }
    else
    {
        findTextM = findText;
        replaceTextM = replaceText;
    }
}

bool SearchableEditor::find(bool newSearch)
{
    if (!fd)
        fd = new FindDialog(this, ::wxGetTopLevelParent(this));
    if (newSearch || findTextM.empty())
    {
        if (newSearch)
        {
            // find selected text
            wxString findText(GetSelectedText());
            // failing that initialize with the word at the caret
            if (findText.empty())
            {
                int pos = GetCurrentPos();
                int start = WordStartPosition(pos, true);
                int end = WordEndPosition(pos, true);
                if (end > start)
                    findText = GetTextRange(start, end);
            }
            fd->SetFindText(findText);
        }

        // do not re-center dialog if it is already visible
        if (!fd->IsShown())
            fd->Show();
        fd->SetFocus();
        return false;    // <- caller shouldn't care about this
    }

    int start = GetSelectionEnd();
    if (findFlagsM.has(se::FROM_TOP))
    {
        start = 0;
        findFlagsM.remove(se::ALERT);    // remove flag after first find
    }

    int end = GetTextLength();
    int p = FindText(start, end, findTextM, findFlagsM.asStc());
    if (p == -1)
    {
        if (findFlagsM.has(se::WRAP))
            p = FindText(0, end, findTextM, findFlagsM.asStc());
        if (p == -1)
        {
            if (findFlagsM.has(se::ALERT))
                wxMessageBox(_("No more matches"), _("Search complete"), wxICON_INFORMATION|wxOK);
            return false;
        }
    }
    centerCaret(true);
    GotoPos(p);
    GotoPos(p + findTextM.Length());
    SetSelectionStart(p);
    SetSelectionEnd(p + findTextM.Length());
    centerCaret(false);
    return true;
}

bool SearchableEditor::replace(bool force)
{
    int start = GetSelectionStart();
    int end = GetSelectionEnd();
    if (start == end || FindText(start, end, findTextM, findFlagsM.asStc()) == -1)
    {
        if (!find(false))
            return false;
        if (!force)
            return false;
    }

    // we have selection here
    start = GetSelectionStart();
    ReplaceSelection(replaceTextM);
    SetSelectionEnd(start + replaceTextM.Length());        // position at end of replaced text
    find(false);
    return true;
}

int SearchableEditor::replaceAll()
{
    int caret = GetSelectionStart();        // remember position, so we return to some sensible place at end
    SetSelectionStart(0);                    // start from beginning of file
    SetSelectionEnd(0);
    findFlagsM.remove(se::FROM_TOP);
    bool alert = findFlagsM.has(se::ALERT);
    if (alert)
        findFlagsM.remove(se::ALERT);        // turn flag off temporary (we'll popup a different message)
    bool wrap = findFlagsM.has(se::WRAP);
    if (wrap)
        findFlagsM.remove(se::WRAP);        // turn it off to prevent endless loop (if replace text contains search text)

    int cnt = 0;
    while (replace(true))
        cnt++;
    if (alert)
    {
        wxMessageBox(wxString::Format(_("%d replacements were made."), cnt), _("Replace"), wxICON_INFORMATION|wxOK);
        findFlagsM.add(se::ALERT);            // turn it back on
    }
    if (wrap)                                // turn it back on
        findFlagsM.add(se::WRAP);
    SetSelectionStart(caret);
    SetSelectionEnd(caret);
    return cnt;
}

int SearchableEditor::replaceInSelection()
{
    findFlagsM.remove(se::FROM_TOP);    // turn flag off
    bool alert = findFlagsM.has(se::ALERT);
    if (alert)
        findFlagsM.remove(se::ALERT);    // turn flag off temporary
    bool wrap = findFlagsM.has(se::WRAP);
    if (wrap)
        findFlagsM.remove(se::WRAP);        // turn it off to prevent endless loop (if replace text contains search text)

    int cnt = 0;
    int selstart = GetSelectionStart();
    int selend = GetSelectionEnd();
    SetSelectionEnd(selstart);    // start replace from here
    while (find(false))
    {
        int start = GetSelectionStart();
        int end =   GetSelectionEnd();
        if (end <= selend)        // in range
        {
            selend += replaceTextM.Length() - (end-start);        // expand/contract range
            ReplaceSelection(replaceTextM);
            SetSelectionEnd(start + replaceTextM.Length());        // move to appropriate position for next find()
            cnt++;
        }
        else
            break;
    }

    if (alert)
    {
        wxMessageBox(wxString::Format(_("%d replacements were made."), cnt), _("Replace"), wxICON_INFORMATION|wxOK);
        findFlagsM.add(se::ALERT);        // turn it back on
    }
    if (wrap)                                // turn it back on
        findFlagsM.add(se::WRAP);
    SetSelectionStart(selstart);
    SetSelectionEnd(selend);
    return cnt;
}

void SearchableEditor::centerCaret(bool doCenter)
{
    if (doCenter)       // try to keep it in center of screen
    {
        SetXCaretPolicy(wxSTC_CARET_STRICT|wxSTC_CARET_EVEN, 0);
        SetYCaretPolicy(wxSTC_CARET_STRICT|wxSTC_CARET_EVEN, 0);
    }
    else
    {
        SetXCaretPolicy(wxSTC_CARET_SLOP|wxSTC_CARET_EVEN|wxSTC_CARET_STRICT, 50);
        SetYCaretPolicy(wxSTC_CARET_SLOP|wxSTC_CARET_EVEN|wxSTC_CARET_STRICT, 2);
    }
}

FindDialog::FindDialog(SearchableEditor *editor, wxWindow* parent, const wxString& title, FindFlags *allowedFlags)
    :BaseDialog(parent, -1, title)
{
    parentEditorM = editor;
    FindFlags flags;
    if (allowedFlags)
        flags = *allowedFlags;    // copy settings

    label_find = new wxStaticText(getControlsPanel(), -1, _("Fi&nd:"));
    text_ctrl_find = new wxTextCtrl(getControlsPanel(), -1);
    label_replace = new wxStaticText(getControlsPanel(), -1, _("Re&place with:"));
    text_ctrl_replace = new wxTextCtrl(getControlsPanel(), -1);

    checkbox_wholeword = checkbox_matchcase = checkbox_regexp = checkbox_convertbs = checkbox_wrap = checkbox_fromtop = 0;
    if (flags.has(se::WHOLE_WORD))
        checkbox_wholeword = new wxCheckBox(getControlsPanel(), -1, _("Whole &word only"));
    if (flags.has(se::MATCH_CASE))
        checkbox_matchcase = new wxCheckBox(getControlsPanel(), -1, _("&Match case"));
    if (flags.has(se::REGULAR_EXPRESSION))
        checkbox_regexp    = new wxCheckBox(getControlsPanel(), -1, _("Regular e&xpression"));
    if (flags.has(se::CONVERT_BACKSLASH))
        checkbox_convertbs = new wxCheckBox(getControlsPanel(), -1, _("Convert &backslashes"));
    if (flags.has(se::WRAP))
        checkbox_wrap      = new wxCheckBox(getControlsPanel(), -1, _("Wrap ar&ound"));
    if (flags.has(se::FROM_TOP))
        checkbox_fromtop   = new wxCheckBox(getControlsPanel(), -1, _("Start search from &top"));

    button_find = new wxButton(getControlsPanel(), wxID_FIND, _("&Find"));
    button_replace = new wxButton(getControlsPanel(), wxID_REPLACE,
        _("&Replace"));
    button_replace_all = new wxButton(getControlsPanel(), wxID_REPLACE_ALL,
        _("Replace &all"));
    button_replace_in_selection = new wxButton(getControlsPanel(),
        ID_button_replace_in_selection, _("In &selection"));
    button_close = new wxButton(getControlsPanel(), wxID_CANCEL, _("&Close"));

    do_layout();
    button_find->SetDefault();
    text_ctrl_find->SetFocus();
}

void FindDialog::do_layout()
{
    wxFlexGridSizer* sizerEdits = new wxFlexGridSizer(2, 2,
        styleguide().getRelatedControlMargin(wxVERTICAL),
        styleguide().getControlLabelMargin());

    sizerEdits->Add(label_find, 0, wxALIGN_CENTER_VERTICAL);
    sizerEdits->Add(text_ctrl_find, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    sizerEdits->Add(label_replace, 0, wxALIGN_CENTER_VERTICAL);
    sizerEdits->Add(text_ctrl_replace, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL);
    sizerEdits->AddGrowableCol(1, 1);

    wxGridSizer* sizerChecks = new wxGridSizer(3, 2,
        styleguide().getCheckboxSpacing(),
        styleguide().getUnrelatedControlMargin(wxHORIZONTAL));
    if (checkbox_wholeword)
        sizerChecks->Add(checkbox_wholeword);
    if (checkbox_matchcase)
        sizerChecks->Add(checkbox_matchcase);
    if (checkbox_regexp)
        sizerChecks->Add(checkbox_regexp);
    if (checkbox_convertbs)
        sizerChecks->Add(checkbox_convertbs);
    if (checkbox_wrap)
        sizerChecks->Add(checkbox_wrap);
    if (checkbox_fromtop)
        sizerChecks->Add(checkbox_fromtop);

    wxBoxSizer* sizerButtons = new wxBoxSizer(wxHORIZONTAL);
    sizerButtons->Add(0, 0, 1, wxEXPAND);
    sizerButtons->Add(button_find);
    sizerButtons->Add(styleguide().getBetweenButtonsMargin(wxHORIZONTAL), 0);
    sizerButtons->Add(button_replace);
    sizerButtons->Add(styleguide().getBetweenButtonsMargin(wxHORIZONTAL), 0);
    sizerButtons->Add(button_replace_all);
    sizerButtons->Add(styleguide().getBetweenButtonsMargin(wxHORIZONTAL), 0);
    sizerButtons->Add(button_replace_in_selection);
    sizerButtons->Add(styleguide().getBetweenButtonsMargin(wxHORIZONTAL), 0);
    sizerButtons->Add(button_close);

    wxBoxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    sizerControls->Add(sizerEdits, 1, wxEXPAND);
    sizerControls->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerControls->Add(sizerChecks);

    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons);
}

void FindDialog::setup()
{
    FindFlags flags;
    if (!checkbox_wholeword->IsChecked())
        flags.remove(se::WHOLE_WORD);
    if (!checkbox_matchcase->IsChecked())
        flags.remove(se::MATCH_CASE);
    if (!checkbox_regexp->IsChecked())
        flags.remove(se::REGULAR_EXPRESSION);
    if (!checkbox_convertbs->IsChecked())
        flags.remove(se::CONVERT_BACKSLASH);
    if (!checkbox_wrap->IsChecked())
        flags.remove(se::WRAP);
    if (!checkbox_fromtop->IsChecked())
        flags.remove(se::FROM_TOP);

    wxString find = text_ctrl_find->GetValue();
    wxString replace = text_ctrl_replace->GetValue();
    parentEditorM->setupSearch(find, replace, flags);
    if (flags.has(se::FROM_TOP))
        checkbox_fromtop->SetValue(false);
}

void FindDialog::SetFindText(const wxString& text)
{
    // don't overwrite search text with empty or multiline text
    if (text.empty() || text.find_first_of('\n') != wxString::npos)
        return;
    text_ctrl_find->ChangeValue(text);
    text_ctrl_find->SelectAll();
}

BEGIN_EVENT_TABLE(FindDialog, BaseDialog)
    EVT_BUTTON(wxID_FIND, FindDialog::OnFindButtonClick)
    EVT_BUTTON(wxID_REPLACE, FindDialog::OnReplaceButtonClick)
    EVT_BUTTON(wxID_REPLACE_ALL, FindDialog::OnReplaceAllButtonClick)
    EVT_BUTTON(FindDialog::ID_button_replace_in_selection, FindDialog::OnReplaceInSelectionButtonClick)
END_EVENT_TABLE()

void FindDialog::OnFindButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxBusyCursor wait;
    setup();
    parentEditorM->find(false);
}

void FindDialog::OnReplaceButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxBusyCursor wait;
    setup();
    parentEditorM->replace();
}

void FindDialog::OnReplaceAllButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxBusyCursor wait;
    setup();
    parentEditorM->replaceAll();
}

void FindDialog::OnReplaceInSelectionButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxBusyCursor wait;
    setup();
    parentEditorM->replaceInSelection();
}

