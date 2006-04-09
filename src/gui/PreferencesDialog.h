/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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


  $Id$

*/

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H
//-----------------------------------------------------------------------------
#include <list>

#include <wx/wx.h>
#include <wx/event.h>
#include <wx/filename.h>
#include <wx/treectrl.h>
#include <wx/imaglist.h>

#include "config/Config.h"
#include "gui/BaseDialog.h"
//-----------------------------------------------------------------------------
class Optionbook;
class wxXmlNode;
//-----------------------------------------------------------------------------
class PrefDlgSetting: public wxEvtHandler
{
protected:
    PrefDlgSetting(wxPanel* page, PrefDlgSetting* parent);
public:
    virtual ~PrefDlgSetting();

    // returns true if anything has been added to sizer
    bool addToSizer(wxSizer* sizer, PrefDlgSetting* previous);
    // adjust label min size to position first (non-label) control
    void alignControl(int left);
    // returns false on error, when (!ignoreerrors)
    virtual bool createControl(bool ignoreerrors) = 0;
    // all controls with same alignment group > 0 will be left-aligned
    int getControlAlignmentGroup() const;
    // get (unadjusted) left coordinate of first (non-label) control
    int getControlLeft();
    // parent for created controls
    wxPanel* getPage() const;
    virtual bool loadFromConfig(Config& config) = 0;
    virtual bool parseProperty(wxXmlNode* xmln);
    virtual bool saveToConfig(Config& config) = 0;
protected:
    wxString captionM;
    wxString descriptionM;
    wxString keyM;

    virtual void addControlsToSizer(wxSizer* sizer) = 0;
    void addEnabledSetting(PrefDlgSetting* setting);
    bool checkConfigProperties() const;
    virtual void enableControls(bool enabled) = 0;
    void enableEnabledSettings(bool enabled);
    int getControlIndentation(int level) const;
    virtual wxStaticText* getLabel();
    int getLevel() const;
    virtual bool hasControls() const = 0;
    virtual bool isRelatedTo(PrefDlgSetting* prevSetting) const;
    virtual void setDefault(const wxString& defValue);
private:
    wxPanel* pageM;
    PrefDlgSetting* parentM;
    std::list<PrefDlgSetting*> enabledSettingsM;
    bool relatedM;
    int alignmentGroupM;
};

PrefDlgSetting* createPrefDlgSetting(wxPanel* page, const wxString& type,
    PrefDlgSetting* parent);
//-----------------------------------------------------------------------------
class PreferencesDialog: public BaseDialog {
public:
    enum {
        ID_treectrl_panes = 100,
        ID_bookctrl_panes,
        ID_button_save,
        ID_button_cancel = wxID_CANCEL
    };

    PreferencesDialog(wxWindow* parent, const wxString& title,
        Config& config, const wxString& descriptionFileName);
    ~PreferencesDialog();

    int getSelectedPage();
    bool isOk();
    bool loadFromConfig();
    bool saveToConfig();
    void selectPage(int index);

    void OnSaveButtonClick(wxCommandEvent& event);
    void OnTreeSelChanged(wxTreeEvent& event);
private:
    bool debugDescriptionM;
    bool loadSuccessM;
    Config& configM;

    wxImageList imageListM;
    wxArrayTreeItemIds treeItemsM;
    std::list<PrefDlgSetting*> settingsM;

    wxTreeCtrl* treectrl_1;
    wxPanel* panel_categ;
    wxStaticText* static_text_categ;
    Optionbook* bookctrl_1;

    wxButton* button_save;
    wxButton* button_cancel;

    bool createControlsAndAddToSizer(wxPanel* page, wxSizer* sizerPage);
    void layout();
    void loadDescriptionFile(const wxFileName& filename);
    bool parseDescriptionNode(wxTreeItemId parent, wxXmlNode* xmln);
    bool parseDescriptionSetting(wxPanel* page, wxXmlNode* xmln,
        PrefDlgSetting* enabledby);
    void setProperties();
protected:
    virtual const wxString getName() const;

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // PREFERENCESDIALOG_H
