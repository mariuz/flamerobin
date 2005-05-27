/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2005 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s): _____________________
*/

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "styleguide.h"
#include "FindDialog.h"
//-----------------------------------------------------------------------------
FindFlags::FindFlags()
{
	flags = se::DEFAULT;
}
//-----------------------------------------------------------------------------
bool FindFlags::has(unsigned int flag) const
{
	return ((flags & flag) == flag);
}
//-----------------------------------------------------------------------------
void FindFlags::remove(unsigned int flag)
{
	flags &= ~flag;
}
//-----------------------------------------------------------------------------
void FindFlags::add(unsigned int flag)
{
	flags |= flag;
}
//-----------------------------------------------------------------------------
int FindFlags::asStc() const			// returns "flags" converted to wxSTC search flags
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
//-----------------------------------------------------------------------------
void FindFlags::show() const
{
	wxString retval(wxString::Format(wxT("Flags (%d) contains:\n"), flags));
	if (has(se::WHOLE_WORD))
		retval += wxT("se::WHOLE_WORD\n");
	if (has(se::MATCH_CASE))
		retval += wxT("se::MATCH_CASE\n");
	if (has(se::REGULAR_EXPRESSION))
		retval += wxT("se::REGULAR_EXPRESSION\n");
	if (has(se::FROM_TOP))
		retval += wxT("se::FROM_TOP\n");
	if (has(se::WRAP))
		retval += wxT("se::WRAP\n");
	if (has(se::CONVERT_BACKSLASH))
		retval += wxT("se::CONVERT_BACKSLASH\n");
	wxMessageBox(retval);
}
//-----------------------------------------------------------------------------
FindFlags& FindFlags::operator= (const FindFlags& source)
{
	flags = source.flags;
	return *this;
}
//-----------------------------------------------------------------------------
//! wxStyledTextCtrl with Search&Replace capability
SearchableEditor::SearchableEditor(wxWindow *parent, wxWindowID id)
	: wxStyledTextCtrl(parent, id)
{
	fd = 0;
}
//-----------------------------------------------------------------------------
wxString SearchableEditor::convertBackslashes(const wxString& source)
{
	wxString result(source);
	result.Replace(wxT("%"), wxT("%%"));        // remove chance to get %s or similar stuff
	return wxString::Format(result);
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
bool SearchableEditor::find(bool newSearch)
{
	if (!fd)
        fd = new FindDialog(this, ::wxGetTopLevelParent(this));
	if (newSearch || findTextM.IsEmpty())
	{
		fd->Show();
		return false;	// <- caller shouldn't care about this
	}
	
	int start = GetSelectionEnd();
	if (findFlagsM.has(se::FROM_TOP))
	{
		start = 0;
		findFlagsM.remove(se::ALERT);	// remove flag after first find
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
	SetSelectionStart(p);
	SetSelectionEnd(p + findTextM.Length());
	return true;
}
//-----------------------------------------------------------------------------
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
	SetSelectionEnd(start + replaceTextM.Length());		// position at end of replaced text
	find(false);
	return true;
}
//-----------------------------------------------------------------------------
int SearchableEditor::replaceAll()
{
	int caret = GetSelectionStart();		// remember position, so we return to some sensible place at end
	SetSelectionStart(0);					// start from beginning of file
	SetSelectionEnd(0);					
	findFlagsM.remove(se::FROM_TOP);
	bool alert = findFlagsM.has(se::ALERT);
	if (alert)
		findFlagsM.remove(se::ALERT);		// turn flag off temporary (we'll popup a different message)
	bool wrap = findFlagsM.has(se::WRAP);
	if (wrap)
		findFlagsM.remove(se::WRAP);		// turn it off to prevent endless loop (if replace text contains search text)
	
	int cnt = 0;
	while (replace(true))
		cnt++;
	if (alert)
	{
		wxMessageBox(wxString::Format(_("%d replacements were made."), cnt), _("Replace"), wxICON_INFORMATION|wxOK);
		findFlagsM.add(se::ALERT);			// turn it back on
	}
	if (wrap)								// turn it back on
		findFlagsM.add(se::WRAP);
	SetSelectionStart(caret);
	SetSelectionEnd(caret);
	return cnt;
}
//-----------------------------------------------------------------------------
int SearchableEditor::replaceInSelection()
{
	findFlagsM.remove(se::FROM_TOP);	// turn flag off
	bool alert = findFlagsM.has(se::ALERT);
	if (alert)
		findFlagsM.remove(se::ALERT);	// turn flag off temporary
	bool wrap = findFlagsM.has(se::WRAP);
	if (wrap)
		findFlagsM.remove(se::WRAP);		// turn it off to prevent endless loop (if replace text contains search text)
	
	int cnt = 0;
	int selstart = GetSelectionStart();
	int selend = GetSelectionEnd();
	SetSelectionEnd(selstart);	// start replace from here
	while (find(false))
	{
		int start = GetSelectionStart();
		int end =   GetSelectionEnd();
		if (end <= selend)		// in range
		{
			selend += replaceTextM.Length() - (end-start);		// expand/contract range
			ReplaceSelection(replaceTextM);
			SetSelectionEnd(start + replaceTextM.Length());		// move to appropriate position for next find()
			cnt++;
		}
		else
			break;
	}
	
	if (alert)
	{
		wxMessageBox(wxString::Format(_("%d replacements were made."), cnt), _("Replace"), wxICON_INFORMATION|wxOK);
		findFlagsM.add(se::ALERT);		// turn it back on
	}
	if (wrap)								// turn it back on
		findFlagsM.add(se::WRAP);
	SetSelectionStart(selstart);
	SetSelectionEnd(selend);
	return cnt;
}
//-----------------------------------------------------------------------------
FindDialog::FindDialog(SearchableEditor *editor, wxWindow* parent, const wxString& title, FindFlags *allowedFlags)
	:BaseDialog(parent, -1, title)
{
	parentEditorM = editor;
	FindFlags flags;
	if (allowedFlags)
		flags = *allowedFlags;	// copy settings

    label_find    = new wxStaticText(panel_controls, -1, _("Find:"));
    text_ctrl_find    = new wxTextCtrl(panel_controls, -1);
    label_replace = new wxStaticText(panel_controls, -1, _("Replace with:"));
    text_ctrl_replace = new wxTextCtrl(panel_controls, -1);

	checkbox_wholeword = checkbox_matchcase = checkbox_regexp = checkbox_convertbs = checkbox_wrap = checkbox_fromtop = 0;
	if (flags.has(se::WHOLE_WORD))
		checkbox_wholeword = new wxCheckBox(panel_controls, -1, _("Whole word only"));
	if (flags.has(se::MATCH_CASE))
		checkbox_matchcase = new wxCheckBox(panel_controls, -1, _("Match case"));
	if (flags.has(se::REGULAR_EXPRESSION))
		checkbox_regexp    = new wxCheckBox(panel_controls, -1, _("Regular expression"));
	if (flags.has(se::CONVERT_BACKSLASH))
		checkbox_convertbs = new wxCheckBox(panel_controls, -1, _("Convert backslashes"));
	if (flags.has(se::WRAP))
		checkbox_wrap      = new wxCheckBox(panel_controls, -1, _("Wrap around"));
	if (flags.has(se::FROM_TOP))
		checkbox_fromtop   = new wxCheckBox(panel_controls, -1, _("Start search from top"));

    button_find = new wxButton(panel_controls, ID_button_find, _("Find"));
    button_replace = new wxButton(panel_controls, ID_button_replace, _("Replace"));
    button_replace_all = new wxButton(panel_controls, ID_button_replace_all, _("Replace all"));
    button_replace_in_selection = new wxButton(panel_controls, ID_button_replace_in_selection, _("In selection"));

	do_layout();
}
//-----------------------------------------------------------------------------
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
	
    wxGridSizer* sizerChecks = new wxGridSizer(2, 2,
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
	
    wxBoxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    sizerControls->Add(sizerEdits, 1, wxEXPAND);
    sizerControls->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerControls->Add(sizerChecks);

    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons);
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(FindDialog, BaseDialog)
	EVT_BUTTON(FindDialog::ID_button_find, FindDialog::OnFindButtonClick)
	EVT_BUTTON(FindDialog::ID_button_replace, FindDialog::OnReplaceButtonClick)
	EVT_BUTTON(FindDialog::ID_button_replace_all, FindDialog::OnReplaceAllButtonClick)
	EVT_BUTTON(FindDialog::ID_button_replace_in_selection, FindDialog::OnReplaceInSelectionButtonClick)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void FindDialog::OnFindButtonClick(wxCommandEvent& WXUNUSED(event))
{
	setup();
	parentEditorM->find(false);
}
//-----------------------------------------------------------------------------
void FindDialog::OnReplaceButtonClick(wxCommandEvent& WXUNUSED(event))
{
	setup();
	parentEditorM->replace();
}
//-----------------------------------------------------------------------------
void FindDialog::OnReplaceAllButtonClick(wxCommandEvent& WXUNUSED(event))
{
	setup();
	parentEditorM->replaceAll();
}
//-----------------------------------------------------------------------------
void FindDialog::OnReplaceInSelectionButtonClick(wxCommandEvent& WXUNUSED(event))
{
	setup();
	parentEditorM->replaceInSelection();
}
//-----------------------------------------------------------------------------
