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

  Contributor(s): Michael Hieke
*/

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
		wxString value;					// not used currently
		wxString text;
		wxString description;			// not used currently
	};

	class Setting
	{
	public:
		std::list<wxControl *> controls;
		wxString name;
		wxString type;
		wxString key;
		wxString defaultValue;
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
        ID_listbook = 100,
        ID_button_save,
        ID_button_cancel,
        ID_button_browse,
        ID_button_font,
        ID_checkbox
    };

    OptionsDialog(wxWindow* parent);

	void OnPageChanging(wxListbookEvent& event);
	void OnSaveButtonClick(wxCommandEvent& event);
	void OnCancelButtonClick(wxCommandEvent& event);
	void OnBrowseButtonClick(wxCommandEvent& event);
	void OnFontButtonClick(wxCommandEvent& event);
	void OnCheckbox(wxCommandEvent& event);
private:
	wxImageList imageList;
	std::list<opt::Page *> pages;

    wxListbook* listbook1;
    wxButton* button_save;
    wxButton* button_cancel;

	wxPanel *createHeadline(wxPanel *parentPanel, const wxString& text);
	void createPages();
	wxPanel *createPanel(opt::Page* pg);
    void do_layout();
	void load();
	opt::Setting *findSetting(wxCommandEvent& event);

    void set_properties();

protected:
	virtual const std::string getName() const;

	~OptionsDialog();
    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // OPTIONSDIALOG_H
