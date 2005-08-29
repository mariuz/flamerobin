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

#ifndef FR_FIND_DIALOG
#define FR_FIND_DIALOG
//-----------------------------------------------------------------------------
#include <wx/wx.h>
#include <wx/image.h>
#include <wx/stc/stc.h>

#include "gui/BaseDialog.h"
//-----------------------------------------------------------------------------
namespace se 	// instead of defines
{
	const unsigned int WHOLE_WORD         = 1;
	const unsigned int MATCH_CASE         = 2;
	const unsigned int REGULAR_EXPRESSION = 4;
	const unsigned int CONVERT_BACKSLASH =  8;
	const unsigned int WRAP =              16;
	const unsigned int FROM_TOP =          32;
	const unsigned int ALERT =             64;
	const unsigned int DEFAULT =		  127;
};
//-----------------------------------------------------------------------------
class FindFlags
{
private:
	unsigned int flags;
public:
	FindFlags();
	bool has(unsigned int flag) const;
	void remove(unsigned int flag);
	void add(unsigned int flag);
	int asStc() const;			// returns "flags" converted to wxSTC search flags
	FindFlags& operator=(const FindFlags& source);
	void show() const;

};
//-----------------------------------------------------------------------------
class FindDialog;
//-----------------------------------------------------------------------------
// this allows us to add search&replace to all wxSTC derivatives
class SearchableEditor: public wxStyledTextCtrl
{
private:
	wxString convertBackslashes(const wxString& source);
	FindDialog *fd;
	FindFlags findFlagsM;
	wxString findTextM;
	wxString replaceTextM;

	// only accessible to FindDialog
	void setupSearch(const wxString& findText, const wxString& replaceText, const FindFlags& flags);
	bool replace(bool force = false);
	int replaceAll();
	int replaceInSelection();

public:
	friend class FindDialog;
	bool find(bool newSearch);
	
	SearchableEditor(wxWindow *parent, wxWindowID id);
};
//-----------------------------------------------------------------------------
class FindDialog: public BaseDialog
{
protected:	
	void setup();
	void do_layout();

	SearchableEditor *parentEditorM;
	wxCheckBox *checkbox_wholeword;		// gui
	wxCheckBox *checkbox_matchcase;
	wxCheckBox *checkbox_regexp;
	wxCheckBox *checkbox_convertbs;
	wxCheckBox *checkbox_wrap;
	wxCheckBox *checkbox_fromtop;

	wxStaticText *label_find;
	wxStaticText *label_replace;
	wxTextCtrl *text_ctrl_find;
	wxTextCtrl *text_ctrl_replace;

	wxButton *button_find;
	wxButton *button_replace;
	wxButton *button_replace_all;
	wxButton *button_replace_in_selection;

public:
    enum {	ID_button_find = 101, 
			ID_button_replace, 
			ID_button_replace_all, 
			ID_button_replace_in_selection	
	};
	void OnFindButtonClick(wxCommandEvent &event);
	void OnReplaceButtonClick(wxCommandEvent &event);
	void OnReplaceAllButtonClick(wxCommandEvent &event);
	void OnReplaceInSelectionButtonClick(wxCommandEvent &event);
	
	FindDialog(SearchableEditor *editor, wxWindow* parent, const wxString& title = _("Find and replace"), FindFlags *allowedFlags = 0);
    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif
