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

  Contributor(s):
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
//-----------------------------------------------------------------------------
#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/listbook.h>
#include <wx/textfile.h>
#include <wx/spinctrl.h>
#include <list>
#include "BaseDialog.h"
//-----------------------------------------------------------------------------
// I wanted to make sure that it doesn't mixup with some other
// code that may want to use Page, Setting or Option class
namespace opt
{
	class Option
	{
	public:
		wxString value;
		wxString text;
		wxString description;
	};

	class Setting
	{
	public:
		std::list<wxControl *> controls;
		wxString name;
		wxString type;
		wxString key;
		Setting *enabledBy;
		std::list<Setting *>enables;
		wxString description;

		std::list<Option *> options;	// for "radio" type
		int min, max;					// for "number" type

		wxBoxSizer *addToPanel(wxPanel *panel);
		void loadFromConfig();
		void saveToConfig();
		Setting();
		~Setting();
	};

	class Page
	{
	public:
		wxString name;
		int image;
		std::list<Setting *> settings;
		Page();
		~Page();
	};
}
//-----------------------------------------------------------------------------
class OptionsDialog: public BaseDialog {
public:
    enum {
        ID_listbook      = 100,
		ID_button_apply  = 101,
		ID_button_reset  = 102,
		ID_button_close  = 103,
		ID_button_browse = 104,
		ID_button_font   = 105,
		ID_checkbox      = 106
    };

	std::list<opt::Page *> pages;
    OptionsDialog(wxWindow* parent);

	void OnPageChanging(wxListbookEvent& event);
	void OnApplyButtonClick(wxCommandEvent& event);
	void OnApplyCloseButtonClick(wxCommandEvent& event);
	void OnResetButtonClick(wxCommandEvent& event);
	void OnBrowseButtonClick(wxCommandEvent& event);
	void OnFontButtonClick(wxCommandEvent& event);
	void OnCheckbox(wxCommandEvent& event);
private:
	wxImageList imageList;

	void load();
	void createPages();
	wxPanel *createPanel(opt::Page* pg);
	wxPanel *createHeadline(wxPanel *parentPanel, wxString text);
	opt::Setting *findSetting(wxCommandEvent& event);

    void set_properties();
    void do_layout();

protected:
	virtual const std::string getName() const;
    wxListbook* listbook1;

	~OptionsDialog();
    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // OPTIONSDIALOG_H
