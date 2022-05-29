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

#ifndef FR_FIND_DIALOG
#define FR_FIND_DIALOG

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/stc/stc.h>

#include "gui/BaseDialog.h"

namespace se    // instead of defines
{
    const unsigned int WHOLE_WORD         = 1;
    const unsigned int MATCH_CASE         = 2;
    const unsigned int REGULAR_EXPRESSION = 4;
    const unsigned int CONVERT_BACKSLASH =  8;
    const unsigned int WRAP =              16;
    const unsigned int FROM_TOP =          32;
    const unsigned int ALERT =             64;
    const unsigned int DEFAULT =          127;
};

class FindFlags
{
private:
    unsigned int flags;
public:
    FindFlags();
    bool has(unsigned int flag) const;
    void remove(unsigned int flag);
    void add(unsigned int flag);
    int asStc() const;          // returns "flags" converted to wxSTC search flags
    FindFlags& operator=(const FindFlags& source);
    void show() const;

};

class FindDialog;

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
    void centerCaret(bool doCenter);

    SearchableEditor(wxWindow *parent, wxWindowID id);
};

class FindDialog: public BaseDialog
{
protected:
    void setup();
    void do_layout();

    SearchableEditor *parentEditorM;
    wxCheckBox *checkbox_wholeword;     // gui
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
    wxButton *button_close;

public:
    enum { ID_button_replace_in_selection = 101 };
    void OnFindButtonClick(wxCommandEvent &event);
    void OnReplaceButtonClick(wxCommandEvent &event);
    void OnReplaceAllButtonClick(wxCommandEvent &event);
    void OnReplaceInSelectionButtonClick(wxCommandEvent &event);

    FindDialog(SearchableEditor *editor, wxWindow* parent,
        const wxString& title = _("Find and replace"),
        FindFlags* allowedFlags = 0);

	void SetFindText(const wxString& text);

    DECLARE_EVENT_TABLE()
};

#endif
