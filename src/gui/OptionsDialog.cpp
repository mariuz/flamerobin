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
#include "wx/filename.h"
#include "wx/fontdlg.h"
#include "images.h"
#include "config.h"
#include "ugly.h"
#include "frutils.h"
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
	if (controls.begin() == controls.end())
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
wxBoxSizer *Setting::addToPanel(wxPanel *panel)
{
	int border = 3;
	int space = 8;

	// add horizontal boxSizer
    wxBoxSizer* sz = new wxBoxSizer(wxHORIZONTAL);

	// add indentation
	int depth = 0;
	for (Setting *st = this; st->enabledBy; st = st->enabledBy)
		depth++;
	sz->Add(15*depth, 5);

	if (description.IsEmpty())
		description = name;

	// add controls
	if (type == wxT("checkbox"))
	{
		wxCheckBox *temp = new wxCheckBox(panel, OptionsDialog::ID_checkbox, name);
		sz->Add(temp, 0, wxALL|wxFIXED_MINSIZE, border);
		temp->SetToolTip(description);
		controls.push_back(temp);
	}

	else if (type == wxT("number"))
	{
		wxStaticText *t = new wxStaticText(panel, -1, name);
		sz->Add(t, 0, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, border);
		sz->Add(space, 5);
		wxSpinCtrl *temp = new wxSpinCtrl(panel, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, min, max);
		sz->Add(temp, 0, wxALL|wxFIXED_MINSIZE, border);
		temp->SetToolTip(description);
		t->SetToolTip(description);
		controls.push_back(temp);
		controls.push_back(t);
	}
	else if (type == wxT("string"))
	{
		wxStaticText *t = new wxStaticText(panel, -1, name);
		sz->Add(t, 0, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, border);
		sz->Add(space, 5);
		wxTextCtrl *temp = new wxTextCtrl(panel, -1);
		sz->Add(temp, 0, wxALL|wxFIXED_MINSIZE, border);
		temp->SetToolTip(description);
		t->SetToolTip(description);
		controls.push_back(temp);
		controls.push_back(t);
	}
	else if (type == wxT("file"))
	{
		wxStaticText *t = new wxStaticText(panel, -1, name);
		sz->Add(t, 0, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, border);
		sz->Add(space, 5);
		wxTextCtrl *temp = new wxTextCtrl(panel, -1);
		sz->Add(temp, 1, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, border);
		wxButton *b = new wxButton(panel, OptionsDialog::ID_button_browse, _("Browse..."));
		sz->Add(b, 0, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, 1);
		temp->SetToolTip(description);
		t->SetToolTip(description);
		b->SetToolTip(description);
		controls.push_back(temp);
		controls.push_back(t);
		controls.push_back(b);
	}
	else if (type == wxT("font"))
	{
		wxStaticText *t = new wxStaticText(panel, -1, name);
		sz->Add(t, 0, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, border);
		sz->Add(space, 5);
		wxTextCtrl *temp = new wxTextCtrl(panel, -1);
		sz->Add(temp, 1, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, border);
		wxButton *b = new wxButton(panel, OptionsDialog::ID_button_font, _("Change"));
		sz->Add(b, 0, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, 1);
		t->SetToolTip(description);
		temp->SetToolTip(description);
		b->SetToolTip(description);
		controls.push_back(temp);
		controls.push_back(t);
		controls.push_back(b);
	}
	else if (type == wxT("radio"))
	{
		int size = options.size();
		wxString *opts = new wxString[size];
		int cnt = 0;
		for (std::list<Option *>::iterator it = options.begin(); it != options.end(); ++it)
			opts[cnt++] = (*it)->text;
		wxRadioBox *r = new wxRadioBox(panel, -1, name, wxDefaultPosition, wxDefaultSize, size, opts, 1, wxRA_SPECIFY_COLS);
		delete [] opts;
		sz->Add(r, 0, wxALL|wxFIXED_MINSIZE, border);
		r->SetToolTip(description);
		controls.push_back(r);
	}

	// add controls to it
	return sz;
}
//-----------------------------------------------------------------------------
OptionsDialog::OptionsDialog(wxWindow* parent):
    BaseDialog(parent, -1, wxEmptyString)
{
	// we don't want this dialog centered on parent since it is very big, and
	// some parents (ex. main frame) could even be smaller
	config().setValue(getName() + "::centerOnParent", false);

	listbook1 = new wxListbook(this, ID_listbook, wxDefaultPosition, wxDefaultSize, wxLB_DEFAULT);
	imageList.Create(32, 32);
	imageList.Add(getImage32(ntColumn));
	imageList.Add(getImage32(ntProcedure));
	imageList.Add(getImage32(ntTrigger));
	imageList.Add(getImage32(ntUnknown));
	listbook1->SetImageList(&imageList);

    set_properties();
	load();
	createPages();
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
}
//-----------------------------------------------------------------------------
void OptionsDialog::do_layout()
{
    wxBoxSizer* sizer_1 = new wxBoxSizer(wxHORIZONTAL);
    sizer_1->Add(listbook1, 1, wxEXPAND, 0);
	sizer_1->SetSizeHints(listbook1);
    //SetAutoLayout(true);
    SetSizer(sizer_1);
    Layout();

	Fit();	// this seems to fix the stuff for MSW
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
		else if (key == wxT("page"))
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
	int minw = 0, minh = 0;
	for (std::list<Page *>::iterator it = pages.begin(); it != pages.end(); ++it)
	{
		wxPanel *p = createPanel(*it);
		listbook1->AddPage(p, (*it)->name, false, (*it)->image);

		wxSize s = p->GetMinSize();		// find minimal size needed to fit all pages
		int w = s.GetWidth();
		int h = s.GetHeight();
		if (w > minw)
			minw = w;
		if (h > minh)
			minh = h;

		// initial values
		for (std::list<Setting *>::iterator i2 = (*it)->settings.begin(); i2 != (*it)->settings.end(); ++i2)
			(*i2)->loadFromConfig();
	}

	minw += 70;
	minh += 20;
    if (listbook1->HasFlag(wxLB_LEFT))	// tabs left
        minw += 100;
    else 								// tabs on top
        minh += 50;
	SetSizeHints(minw, minh);
}
//-----------------------------------------------------------------------------
wxPanel *OptionsDialog::createPanel(Page* pg)
{
    wxPanel *panel = new wxPanel(listbook1);
    wxBoxSizer* sz = new wxBoxSizer(wxVERTICAL);

	// add headline
    sz->Add(createHeadline(panel, pg->name), 0, wxALL|wxEXPAND, 5);

	// add controls
	for (std::list<Setting *>::iterator it = pg->settings.begin(); it != pg->settings.end(); ++it)
	{
		wxBoxSizer *s = (*it)->addToPanel(panel);
		if (s)
			sz->Add(s, 0, wxALL|wxEXPAND, 5);
	}

	// apply and reset buttons
	wxPanel *spacer = new wxPanel(panel);
	sz->Add(spacer, 1, wxEXPAND, 0);

    wxBoxSizer* sb = new wxBoxSizer(wxHORIZONTAL);
	sz->Add(sb, 0, wxALL|wxALIGN_RIGHT|wxALIGN_BOTTOM, 3);
	wxButton *appcl = new wxButton(panel, ID_button_save, _("Save"));
	wxButton *reset = new wxButton(panel, ID_button_cancel, _("Cancel"));
	sb->Add(appcl, 0, wxALL, 5);
	sb->Add(reset, 0, wxALL, 5);

	// layout
    panel->SetAutoLayout(true);
    panel->SetSizer(sz);
    sz->Fit(panel);
    sz->SetSizeHints(panel);
    return panel;
}
//-----------------------------------------------------------------------------
wxPanel *OptionsDialog::createHeadline(wxPanel *parentPanel, wxString text)
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
	for (std::list<Page *>::iterator pit = pages.begin(); pit != pages.end(); ++pit)
		for (std::list<Setting *>::iterator it = (*pit)->settings.begin(); it != (*pit)->settings.end(); ++it)
			for (std::list<wxControl *>::iterator i2 = (*it)->controls.begin(); i2 != (*it)->controls.end(); ++i2)
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
	bool enabled = event.IsChecked();
	for (std::list<Setting *>::iterator it = s->enables.begin(); it != s->enables.end(); ++it)
		for (std::list<wxControl *>::iterator i2 = (*it)->controls.begin(); i2 != (*it)->controls.end(); ++i2)
			(*i2)->Enable(enabled);
}
//-----------------------------------------------------------------------------
