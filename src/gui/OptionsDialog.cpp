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
#include "images.h"
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
wxBoxSizer *Setting::addToPanel(wxPanel *panel, int id)
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

	// add controls
	if (type == wxT("checkbox"))
	{
		wxCheckBox *temp = new wxCheckBox(panel, id, name);
		sz->Add(temp, 0, wxALL|wxFIXED_MINSIZE, border);
		temp->SetToolTip(description);
	}

	else if (type == wxT("number"))
	{
		wxStaticText *t = new wxStaticText(panel, -1, name);
		sz->Add(t, 0, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, border);
		sz->Add(space, 5);
		wxSpinCtrl *temp = new wxSpinCtrl(panel, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, min, max);
		sz->Add(temp, 0, wxALL|wxFIXED_MINSIZE, border);
		temp->SetToolTip(description);
		t->SetToolTip(description);
	}
	else if (type == wxT("string"))
	{
		wxStaticText *t = new wxStaticText(panel, -1, name);
		sz->Add(t, 0, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, border);
		sz->Add(space, 5);
		wxTextCtrl *temp = new wxTextCtrl(panel, id);
		sz->Add(temp, 0, wxALL|wxFIXED_MINSIZE, border);
		temp->SetToolTip(description);
		t->SetToolTip(description);
	}
	else if (type == wxT("file"))
	{
		wxStaticText *t = new wxStaticText(panel, -1, name);
		sz->Add(t, 0, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, border);
		sz->Add(space, 5);
		wxTextCtrl *temp = new wxTextCtrl(panel, -1);
		sz->Add(temp, 1, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, border);
		wxButton *b = new wxButton(panel, id, _("Browse..."));
		sz->Add(b, 0, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, 1);
		temp->SetToolTip(description);
		t->SetToolTip(description);
		b->SetToolTip(description);
	}
	else if (type == wxT("font"))
	{
		wxStaticText *t = new wxStaticText(panel, -1, name);
		sz->Add(t, 0, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, border);
		sz->Add(space, 5);
		wxTextCtrl *temp = new wxTextCtrl(panel, -1);
		sz->Add(temp, 1, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, border);
		wxButton *b = new wxButton(panel, id, _("Change"));
		sz->Add(b, 0, wxALL|wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL, 1);
		t->SetToolTip(description);
		temp->SetToolTip(description);
		b->SetToolTip(description);
	}
	else if (type == wxT("radio"))
	{
		int size = options.size();
		wxString *opts = new wxString[size];
		int cnt = 0;
		for (std::list<Option *>::iterator it = options.begin(); it != options.end(); ++it)
			opts[cnt++] = (*it)->text;
		wxRadioBox *r = new wxRadioBox(panel, id, name, wxDefaultPosition, wxDefaultSize, size, opts, 0, wxRA_SPECIFY_ROWS);
		delete [] opts;
		sz->Add(r, 0, wxALL|wxFIXED_MINSIZE, border);
		r->SetToolTip(description);
	}

	// add controls to it
	return sz;
}
//-----------------------------------------------------------------------------
OptionsDialog::OptionsDialog(wxWindow* parent):
    BaseDialog(parent, -1, wxEmptyString)
{
	listbook1 = new wxListbook(this, ID_listbook, wxDefaultPosition, wxDefaultSize, wxNB_LEFT);
	idGenerator = 200;	// give IDs to new controls starting from 200

	imageList.Create(32, 32);
	imageList.Add(getImage32(ntColumn));
	imageList.Add(getImage32(ntProcedure));
	imageList.Add(getImage32(ntTrigger));
	imageList.Add(getImage32(ntUnknown));

	listbook1->SetImageList(&imageList);

    set_properties();
    do_layout();
	load();
	createPages();
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
    SetTitle(wxT("Options"));
    SetSize(wxSize(530, 430));
}
//-----------------------------------------------------------------------------
void OptionsDialog::do_layout()
{
    wxBoxSizer* sizer_1 = new wxBoxSizer(wxHORIZONTAL);
    sizer_1->Add(listbook1, 1, wxEXPAND, 0);
    SetAutoLayout(true);
    SetSizer(sizer_1);
    Layout();
}
//-----------------------------------------------------------------------------
void OptionsDialog::load()
{
	wxTextFile file(wxT("config_options.xml"));
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
			str.Replace(wxT("<br>"), wxT("\n"));
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
	int cnt = 0;
	for (std::list<Page *>::iterator it = pages.begin(); it != pages.end(); ++it)
		listbook1->InsertPage(cnt++, createPanel(*it), (*it)->name, false, (*it)->image);
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
		wxBoxSizer *s = (*it)->addToPanel(panel, idGenerator++);
		if (s)
			sz->Add(s, 0, wxALL|wxEXPAND, 5);
	}

	// apply and reset buttons
	wxPanel *spacer = new wxPanel(panel);
	sz->Add(spacer, 1, wxEXPAND, 0);

    wxBoxSizer* sb = new wxBoxSizer(wxHORIZONTAL);
	sz->Add(sb, 0, wxALL|wxALIGN_RIGHT|wxALIGN_BOTTOM, 3);
	wxButton *apply = new wxButton(panel, ID_button_apply, _("Apply"));
	wxButton *reset = new wxButton(panel, ID_button_reset, _("Reset"));
	sb->Add(apply, 0, wxALL, 5);
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
    wxPanel *temp = new wxPanel(parentPanel, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL);
    wxStaticText *headline = new wxStaticText(temp, -1, text);
	headline->SetFont(wxFont(16, wxDEFAULT, wxNORMAL, wxNORMAL, 0, ""));
    wxBoxSizer* sizer3 = new wxBoxSizer(wxHORIZONTAL);
    sizer3->Add(headline, 0, wxALL|wxFIXED_MINSIZE, 3);
    temp->SetAutoLayout(true);
    temp->SetSizer(sizer3);
    sizer3->Fit(temp);
    sizer3->SetSizeHints(temp);
	return temp;
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(OptionsDialog, wxDialog)
    EVT_LISTBOOK_PAGE_CHANGING(ID_listbook, OptionsDialog::OnPageChanging)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void OptionsDialog::OnPageChanging(wxListbookEvent& event)
{
	if (false /* TODO: check if there are unsaved changes */)
	{
		int res = wxMessageBox(_("Do you wish to apply changes?"), _("There are unsaved changes"), wxYES_NO|wxCANCEL|wxICON_QUESTION);
		if (res == wxID_CANCEL)
			event.Veto();
		else if (res == wxID_YES)
		{
			//apply();
		}
	}
}
//-----------------------------------------------------------------------------
