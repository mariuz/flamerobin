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

  The Initial Developer of the Original Code is Michael Hieke.

  Portions created by the original developer
  are Copyright (C) 2005 Michael Hieke.

  All Rights Reserved.

  $Id$

  Contributor(s):
*/

//-----------------------------------------------------------------------------
#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <wx/wx.h>
#include <wx/event.h>
#include <wx/treectrl.h>
#include <wx/imaglist.h>

#include <list>
#include <string>

#include "BaseDialog.h"
#include "config.h"

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
    virtual bool loadFromConfig(YConfig& config) = 0;
    virtual bool parseProperty(wxXmlNode* xmln);
    virtual bool saveToConfig(YConfig& config) = 0;
protected:
    wxString captionM;
    wxString descriptionM;
    std::string keyM;

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
private:
    wxPanel* pageM;
    PrefDlgSetting* parentM;
    std::list<PrefDlgSetting*> enabledSettingsM;
    bool relatedM;
    int alignmentGroupM;
};

PrefDlgSetting* createPrefDlgSetting(wxPanel* page, wxString& type,
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
        YConfig& config, const wxString& descriptionFileName);
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
    YConfig& configM;

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
    void loadDescriptionFile(const wxString& filename);
    bool parseDescriptionNode(wxTreeItemId parent, wxXmlNode* xmln);
    bool parseDescriptionSetting(wxPanel* page, wxXmlNode* xmln,
        PrefDlgSetting* enabledby);
    void setProperties();
protected:
    virtual const std::string getName() const;

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
#endif // PREFERENCESDIALOG_H
