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
#include "wx/filename.h"
#include "wx/fontdlg.h"

#include "config.h"
#include "frutils.h"
#include "images.h"
#include "styleguide.h"
#include "ugly.h"
#include "OptionsDialog.h"

using namespace opt;
//-----------------------------------------------------------------------------
Page::Page()
{
	image = -1;
}
//-----------------------------------------------------------------------------
Page::~Page()
{
	for (std::list<Setting *>::iterator it = settings.begin(); it != settings.end(); ++it)
		delete (*it);
}
//-----------------------------------------------------------------------------
Setting::Setting()
{
	enabledBy = 0;
	name = type = key = description = wxEmptyString;
	min = max = 0;
}
//-----------------------------------------------------------------------------
Setting::~Setting()
{
	for (std::list<Option *>::iterator it = options.begin(); it != options.end(); ++it)
		delete (*it);
}
//-----------------------------------------------------------------------------
void Setting::saveToConfig()
{
	if (controls.begin() == controls.end())
		return;
	wxControl *control = *(controls.begin());

	std::string skey = wx2std(key);
	if (type == wxT("checkbox"))
	{
		wxCheckBox *c = dynamic_cast<wxCheckBox *>(control);
		if (c)
			config().setValue(skey, c->IsChecked(), false);
	}
	else if (type == wxT("radio"))
	{
		wxRadioBox *r = dynamic_cast<wxRadioBox *>(control);
		if (r)
			config().setValue(skey, r->GetSelection(), false);
	}
	else if (type == wxT("number"))
	{
		wxSpinCtrl *s = dynamic_cast<wxSpinCtrl *>(control);
		if (s)
			config().setValue(skey, s->GetValue(), false);
	}
	else	// string
	{
		wxTextCtrl *t = dynamic_cast<wxTextCtrl *>(control);
		if (t)
			config().setValue(skey, wx2std(t->GetValue()), false);
	}
}
//-----------------------------------------------------------------------------
void Setting::loadFromConfig()
{
	if (controls.empty())
		return;
	wxControl *control = *(controls.begin());

	std::string skey = wx2std(key);

	// set default value if needed
	if (!config().keyExists(skey) && !defaultValue.IsEmpty())
		config().setValue(skey, wx2std(defaultValue));

	if (type == wxT("checkbox"))
	{
		wxCheckBox *c = dynamic_cast<wxCheckBox *>(control);
		bool value;
		if (c && config().getValue(skey, value))
			c->SetValue(value);
		value = c->IsChecked();
		for (std::list<Setting *>::iterator it = enables.begin(); it != enables.end(); ++it)
			for (std::list<wxControl *>::iterator i2 = (*it)->controls.begin(); i2 != (*it)->controls.end(); ++i2)
				(*i2)->Enable(value);
	}
	else if (type == wxT("radio"))
	{
		wxRadioBox *r = dynamic_cast<wxRadioBox *>(control);
		int value;
		if (r && config().getValue(skey, value))
			r->SetSelection(value);
	}
	else if (type == wxT("number"))
	{
		wxSpinCtrl *s = dynamic_cast<wxSpinCtrl *>(control);
		std::string value;
		if (s && config().getValue(skey, value))
			s->SetValue(std2wx(value));
	}
	else	// string
	{
		wxTextCtrl *t = dynamic_cast<wxTextCtrl *>(control);
		std::string value;
		if (t && config().getValue(skey, value))
			t->SetValue(std2wx(value));
	}
}
//-----------------------------------------------------------------------------
//! be careful when using controls.push_back, since controls[0] must be the main control (containing the setting)
wxBoxSizer *Setting::addToPanel(wxPanel *panel)
{
	// add horizontal boxSizer
    wxBoxSizer* sz = new wxBoxSizer(wxHORIZONTAL);

	// add indentation
	int depth = 0;
	for (Setting *st = this; st->enabledBy; st = st->enabledBy)
		depth++;
    sz->Add(2 * depth * styleguide().getRelatedControlMargin(wxHORIZONTAL), 0);

	if (description.IsEmpty())
		description = name;

	// add controls
	if (type == wxT("checkbox"))
	{
		wxCheckBox *cb = new wxCheckBox(panel, OptionsDialog::ID_checkbox, name);
		sz->Add(cb, 0, wxFIXED_MINSIZE);
		cb->SetToolTip(description);
		controls.push_back(cb);
	}

	else if (type == wxT("number"))
	{
		wxStaticText *st = new wxStaticText(panel, -1, name);
		sz->Add(st, 0, wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL);
        sz->Add(styleguide().getControlLabelMargin(), 0);
		wxSpinCtrl *sc = new wxSpinCtrl(panel, -1, wxEmptyString,
            wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, min, max);
		sz->Add(sc, 0, wxFIXED_MINSIZE);
		st->SetToolTip(description);
		sc->SetToolTip(description);
		controls.push_back(sc);
		controls.push_back(st);
	}
	else if (type == wxT("string"))
	{
		wxStaticText *st = new wxStaticText(panel, -1, name);
		sz->Add(st, 0, wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL);
        sz->Add(styleguide().getControlLabelMargin(), 0);
		wxTextCtrl *tc = new wxTextCtrl(panel, -1);
		sz->Add(tc, 0, wxFIXED_MINSIZE);
		st->SetToolTip(description);
		tc->SetToolTip(description);
		controls.push_back(tc);
		controls.push_back(st);
	}
	else if (type == wxT("file"))
	{
		wxStaticText *st = new wxStaticText(panel, -1, name);
		sz->Add(st, 0, wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL);
        sz->Add(styleguide().getControlLabelMargin(), 0);
		wxTextCtrl *tc = new wxTextCtrl(panel, -1);
		sz->Add(tc, 1, wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL);
        sz->Add(styleguide().getBrowseButtonMargin(), 0);
		wxButton *btn = new wxButton(panel, OptionsDialog::ID_button_browse, _("Select..."));
		sz->Add(btn, 0, wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL);
		st->SetToolTip(description);
		tc->SetToolTip(description);
		btn->SetToolTip(description);
		controls.push_back(tc);
		controls.push_back(st);
		controls.push_back(btn);
	}
	else if (type == wxT("font"))
	{
		wxStaticText *st = new wxStaticText(panel, -1, name);
		sz->Add(st, 0, wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL);
        sz->Add(styleguide().getControlLabelMargin(), 0);
		wxTextCtrl *tc = new wxTextCtrl(panel, -1);
		sz->Add(tc, 1, wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL);
        sz->Add(styleguide().getBrowseButtonMargin(), 0);
		wxButton *btn = new wxButton(panel, OptionsDialog::ID_button_font, _("Select..."));
		sz->Add(btn, 0, wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL);
		st->SetToolTip(description);
		tc->SetToolTip(description);
		btn->SetToolTip(description);
		controls.push_back(tc);
		controls.push_back(st);
		controls.push_back(btn);
	}
	else if (type == wxT("radio"))
	{
        wxArrayString opts;
        for (std::list<Option *>::iterator it = options.begin(); it != options.end(); ++it)
            opts.Add((*it)->text);
        wxRadioBox *rb = new wxRadioBox(panel, -1, name, wxDefaultPosition, wxDefaultSize,
            opts, 1, wxRA_SPECIFY_COLS);
        sz->Add(rb, 0, wxFIXED_MINSIZE);
        rb->SetToolTip(description);
        controls.push_back(rb);
	}

	// add controls to it
	return sz;
}
//-----------------------------------------------------------------------------
//! recursively enable/disable controls do that multiple levels of depth are possible
void Setting::enableControls()
{
	if (type != wxT("checkbox") || controls.empty())	// only checkboxes can enable other controls
		return;

	wxCheckBox *cb = dynamic_cast<wxCheckBox *>(*(controls.begin()));
	if (!cb)
		return;
	for (std::list<Setting *>::iterator it = enables.begin(); it != enables.end(); ++it)
	{
		(*it)->enableControls();	// recursively
		for (std::list<wxControl *>::iterator i2 = (*it)->controls.begin(); i2 != (*it)->controls.end(); ++i2)
			(*i2)->Enable(cb->IsChecked());
	}
}
//-----------------------------------------------------------------------------
OptionsDialog::OptionsDialog(wxWindow* parent):
    BaseDialog(parent, -1, wxEmptyString)
{
	// we don't want this dialog centered on parent since it is very big, and
	// some parents (ex. main frame) could even be smaller
	config().setValue(getName() + "::centerOnParent", false);

	listbook1 = new wxListbook(panel_controls, ID_listbook,
        wxDefaultPosition, wxDefaultSize, wxLB_DEFAULT);
	imageList.Create(32, 32);
	imageList.Add(getImage32(ntColumn));
	imageList.Add(getImage32(ntProcedure));
	imageList.Add(getImage32(ntTrigger));
	imageList.Add(getImage32(ntUnknown));
	imageList.Add(getImage32(ntTable));
	listbook1->SetImageList(&imageList);

	load();
	createPages();

    button_save = new wxButton(panel_controls, ID_button_save, _("Save"));
    button_cancel = new wxButton(panel_controls, ID_button_cancel, _("Cancel"));

    set_properties();
    do_layout();
}
//-----------------------------------------------------------------------------
OptionsDialog::~OptionsDialog()
{
	for (std::list<Page *>::iterator it = pages.begin(); it != pages.end(); ++it)
		delete (*it);
}
//-----------------------------------------------------------------------------
const std::string OptionsDialog::getName() const
{
    return "ConfigurationOptionsDialog";
}
//-----------------------------------------------------------------------------
void OptionsDialog::set_properties()
{
    SetTitle(wxT("Preferences"));
    button_save->SetDefault();
}
//-----------------------------------------------------------------------------
void OptionsDialog::do_layout()
{
    wxBoxSizer* sizerControls = new wxBoxSizer(wxHORIZONTAL);
    sizerControls->Add(listbook1, 1, wxEXPAND, 0);
    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_save, button_cancel);
    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons, true);
}
//-----------------------------------------------------------------------------
void OptionsDialog::load()
{
	std::string path = getApplicationPath();
	if (!path.empty())
		path += "/";
	path += "config_options.xml";
	wxTextFile file(std2wx(path));
	if (!file.Open(wxConvUTF8))
	{
		wxMessageBox(_("Cannot load config_options.xml file"), _("Error."), wxICON_ERROR|wxOK);
		return;
	}

	Page *pg = 0;
	Setting *st = 0;
	Option *op = 0;

	for (wxString str = file.GetFirstLine(); !file.Eof(); str = file.GetNextLine())
	{
		int p1 = str.Find(wxT("<"));
		int p2 = str.Find(wxT(">"));
		if (p1 == -1 || p2 == -1)
			continue;
		wxString key = str.Mid(p1+1, p2-p1-1);
		str.Remove(0, p2+1);
		p1 = str.Find(wxT("</"));
		if (p1 != -1)
			str.Remove(p1);

		/* page stuff */
		if (key == wxT("page"))
		{
			pg = new Page;
			pages.push_back(pg);
		}
		else if (key == wxT("/page"))
			pg = 0;
		else if (key == wxT("pagename"))
		{
			if (pg)
				pg->name = str;
		}
		else if (key == wxT("image"))
		{
			unsigned long val;
			if (str.ToULong(&val) && pg)
				pg->image = (int)val;
		}

		/* setting stuff */
		else if (key == wxT("setting"))
		{
			if (pg)
			{
				st = new Setting;
				pg->settings.push_back(st);
			}
		}
		else if (key == wxT("/setting"))
			st = 0;
		else if (key == wxT("type"))
		{
			if (st)
				st->type = str;
		}
		else if (key == wxT("name"))
		{
			if (st)
				st->name = str;
		}
		else if (key == wxT("key"))
		{
			if (st)
				st->key = str;
		}
		else if (key == wxT("default"))
		{
			if (st)
				st->defaultValue = str;
		}
		else if (key == wxT("enables"))
		{
			if (pg)
			{
				Setting *s = new Setting;
				s->enabledBy = st;
				st->enables.push_back(s);
				pg->settings.push_back(s);
				st = s;
			}
		}
		else if (key == wxT("/enables"))
		{
			if (st && st->enabledBy)
				st = st->enabledBy;
		}

		/* number type specific */
		else if (key == wxT("min"))
		{
			unsigned long val;
			if (str.ToULong(&val) && st)
				st->min = (int)val;
		}
		else if (key == wxT("max"))
		{
			unsigned long val;
			if (str.ToULong(&val) && st)
				st->max = (int)val;
		}

		/* options - of radio buttons */
		else if (key == wxT("option"))
		{
			op = new Option;
		}
		else if (key == wxT("/option"))
		{
			if (st)
				st->options.push_back(op);
			op = 0;
		}
		else if (key == wxT("value"))
		{
			if (op)
				op->value = str;
		}
		else if (key == wxT("text"))
		{
			if (op)
				op->text = str;
		}

		/* both options & settings */
		else if (key == wxT("description"))
		{
			str.Replace(wxT("<br />"), wxT("\n"));
			if (op)
				op->description = str;
			else
				st->description = str;
		}
	}

}
//-----------------------------------------------------------------------------
// browse through all page and create controls
void OptionsDialog::createPages()
{
	for (std::list<Page *>::iterator it = pages.begin(); it != pages.end(); ++it)
	{
		listbook1->AddPage(createPanel(*it), (*it)->name, false, (*it)->image);
		// initial values
		for (std::list<Setting *>::iterator i2 = (*it)->settings.begin(); i2 != (*it)->settings.end(); ++i2)
			(*i2)->loadFromConfig();
	}
}
//-----------------------------------------------------------------------------
wxPanel *OptionsDialog::createPanel(Page* pg)
{
    wxPanel *panel = new wxPanel(listbook1);
    wxBoxSizer* sz = new wxBoxSizer(wxVERTICAL);

    // add headline only when category list is on left side
    if (!listbook1->IsVertical())
        sz->Add(createHeadline(panel, pg->name), 0, wxEXPAND);

	// add controls
	for (std::list<Setting *>::iterator it = pg->settings.begin(); it != pg->settings.end(); ++it)
	{
		wxBoxSizer *s = (*it)->addToPanel(panel);
		if (s)
        {
            int margin = styleguide().getUnrelatedControlMargin(wxVERTICAL);
            sz->Add(0, margin);
			sz->Add(s, 0, wxEXPAND);
        }
	}
	// layout
    panel->SetSizer(sz);
    sz->Fit(panel);
    sz->SetSizeHints(panel);
    return panel;
}
//-----------------------------------------------------------------------------
wxPanel *OptionsDialog::createHeadline(wxPanel *parentPanel, const wxString& text)
{
    wxPanel *temp = new wxPanel(parentPanel, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER);
    wxStaticText *headline = new wxStaticText(temp, -1, text);
    headline->SetFont(wxFont(16, wxDEFAULT, wxNORMAL, wxNORMAL, 0, wxT("")));
    wxBoxSizer* sizer3 = new wxBoxSizer(wxHORIZONTAL);
    sizer3->Add(headline, 0, wxALL|wxFIXED_MINSIZE, 3);
    temp->SetAutoLayout(true);
    temp->SetSizer(sizer3);
    sizer3->Fit(temp);
    sizer3->SetSizeHints(temp);
    return temp;
}
//-----------------------------------------------------------------------------
// finds setting for which the event took place
Setting *OptionsDialog::findSetting(wxCommandEvent& event)
{
	Setting *s = 0;
	for (std::list<Page *>::iterator pit = pages.begin(); pit != pages.end() && !s; ++pit)
		for (std::list<Setting *>::iterator it = (*pit)->settings.begin(); it != (*pit)->settings.end() && !s; ++it)
			for (std::list<wxControl *>::iterator i2 = (*it)->controls.begin(); i2 != (*it)->controls.end() && !s; ++i2)
				if ((*i2) == event.GetEventObject())
					s = (*it);
	return s;
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(OptionsDialog, wxDialog)
    EVT_BUTTON(OptionsDialog::ID_button_save, OptionsDialog::OnSaveButtonClick)
    EVT_BUTTON(OptionsDialog::ID_button_cancel, OptionsDialog::OnCancelButtonClick)
    EVT_BUTTON(OptionsDialog::ID_button_browse, OptionsDialog::OnBrowseButtonClick)
    EVT_BUTTON(OptionsDialog::ID_button_font, OptionsDialog::OnFontButtonClick)
	EVT_CHECKBOX(OptionsDialog::ID_checkbox, OptionsDialog::OnCheckbox)

    EVT_LISTBOOK_PAGE_CHANGING(ID_listbook, OptionsDialog::OnPageChanging)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void OptionsDialog::OnPageChanging(wxListbookEvent& WXUNUSED(event))
{
	/* maybe put something here? */
}
//-----------------------------------------------------------------------------
// save settings to config and close dialog
void OptionsDialog::OnSaveButtonClick(wxCommandEvent& WXUNUSED(event))
{
	for (std::list<Page *>::iterator pit = pages.begin(); pit != pages.end(); ++pit)
		for (std::list<Setting *>::iterator it = (*pit)->settings.begin(); it != (*pit)->settings.end(); ++it)
			(*it)->saveToConfig();
	config().save();
	wxMessageBox(wxGetTranslation(
		wxT("Some changes will only work on newly opened windows.\n")
		wxT("Also, some changes won't take effect until program is restarted.")),
		_("Preferences saved"),
		wxOK|wxICON_INFORMATION
	);
	Close();
}
//-----------------------------------------------------------------------------
// load settings from config
void OptionsDialog::OnCancelButtonClick(wxCommandEvent& WXUNUSED(event))
{
	//for (std::list<Page *>::iterator pit = pages.begin(); pit != pages.end(); ++pit)
	//	for (std::list<Setting *>::iterator it = (*pit)->settings.begin(); it != (*pit)->settings.end(); ++it)
	//		(*it)->loadFromConfig();
	Close();
}
//-----------------------------------------------------------------------------
void OptionsDialog::OnBrowseButtonClick(wxCommandEvent& event)
{
	Setting *s = findSetting(event);		// find the setting for this button
	if (!s)
		return;

	wxTextCtrl *t = dynamic_cast<wxTextCtrl *>(*(s->controls.begin()));
	if (!t)
		return;
	wxString path;
	wxFileName::SplitPath(t->GetValue(), &path, 0, 0);

	// browse for file and update textbox
    wxString filename = ::wxFileSelector(_("Select File"), path, wxEmptyString,
		wxEmptyString, _("All files (*.*)|*.*"), 0, this);
    if (!filename.empty())
			t->SetValue(filename);
}
//-----------------------------------------------------------------------------
void OptionsDialog::OnFontButtonClick(wxCommandEvent& event)
{
	Setting *s = findSetting(event);		// find the setting for this button
	if (!s)
		return;

	wxTextCtrl *t = dynamic_cast<wxTextCtrl *>(*(s->controls.begin()));
	if (!t)
		return;
	wxFont f;
	wxString info = t->GetValue();
	if (!info.IsEmpty())
		f.SetNativeFontInfo(info);
	wxFont f2 = ::wxGetFontFromUser(this, f);
	if (f2.Ok())
		t->SetValue(f2.GetNativeFontInfoDesc());
}
//-----------------------------------------------------------------------------
void OptionsDialog::OnCheckbox(wxCommandEvent& event)
{
	Setting *s = findSetting(event);		// find the setting for this checkbox
	if (!s)
		return;
	//s->enableControls(event.IsChecked());
	s->enableControls();
}
//-----------------------------------------------------------------------------
