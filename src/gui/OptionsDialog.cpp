#include "OptionsDialog.h"

using namespace opt;
//-----------------------------------------------------------------------------
Page::~Page()
{
	for (std::list<Setting *>::iterator it = settings.begin(); it != settings.end(); ++it)
		delete (*it);
}
//-----------------------------------------------------------------------------
int getNewId() { return 1; };

wxBoxSizer *Setting::addToPanel(wxPanel *panel)
{
	// add horizontal boxSizer
    wxBoxSizer* sz = new wxBoxSizer(wxHORIZONTAL);

	wxWindow *temp = 0;
	if (type == "checkbox")
		temp = new wxCheckBox(panel, getNewId(), name);
	else
		return 0;

	// add controls to it
	sz->Add(temp, 0, wxALL|wxFIXED_MINSIZE, 3);
    temp->SetAutoLayout(true);
    temp->SetSizer(sz);
	return sz;
}
//-----------------------------------------------------------------------------
OptionsDialog::OptionsDialog(wxWindow* parent):
    BaseDialog(parent, -1, wxEmptyString)
{
    listbook1 = new wxListbook(this, ID_listbook, wxDefaultPosition, wxDefaultSize, wxNB_LEFT);

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
    SetSize(wxSize(522, 382));
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
				st->name = key;
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
	{
		int imageId = -1;
		listbook1->InsertPage(cnt++, createPanel(*it), (*it)->name, false, imageId);
	}
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
			sz->Add((wxSizer *)s, 0, wxALL|wxEXPAND, 5);
	}

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
