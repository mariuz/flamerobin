#include <wx/wx.h>
#include <wx/image.h>

#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <wx/listbook.h>
#include <wx/textfile.h>
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
		wxString name;
		wxString type;
		wxString key;
		Setting *enabledBy;
		std::list<Setting *>enables;
		std::list<Option *> options;	// for "radio" type
		wxString description;

		wxBoxSizer *addToPanel(wxPanel *panel);
		Setting(): enabledBy(0), name(wxEmptyString), type(wxEmptyString), key(wxEmptyString), description(wxEmptyString) {};
	};

	class Page
	{
	public:
		wxString name;
		int image;
		std::list<Setting *> settings;
		~Page();
	};
}
//-----------------------------------------------------------------------------
class OptionsDialog: public BaseDialog {
public:
    enum {
        ID_listbook = 100
    };

	std::list<opt::Page *> pages;
    OptionsDialog(wxWindow* parent);
	void OnPageChanging(wxListbookEvent& event);

private:
	void load();
	void createPages();
	wxPanel *createPanel(opt::Page* pg);
	wxPanel *createHeadline(wxPanel *parentPanel, wxString text);

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
