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

#pragma once
#ifndef PREFERENCESDIALOG_STYLE_H
#define PREFERENCESDIALOG_STYLE_H

#include <memory>

#include <wx/xml/xml.h>

#include "PreferencesDialog.h"
#include "gui/FRStyle.h"
#include "gui/FRStyleManager.h"
#include "gui/StyleGuide.h"


class PrefDlgStyleSetting : public PrefDlgSetting
{
public:
    PrefDlgStyleSetting(wxPanel* page, PrefDlgSetting* parent);
    ~PrefDlgStyleSetting();

    virtual bool createControl(bool ignoreerrors);
    virtual bool loadFromTargetConfig(Config& config);
    virtual bool parseProperty(wxXmlNode* xmln);
    virtual bool saveToTargetConfig(Config& config);
    virtual bool cancelChanges(Config& config);

    FRStyleManager& getStyleManager();
protected:
    wxString defaultM;

    virtual void addControlsToSizer(wxSizer* sizer);
    virtual void enableControls(bool enabled);
    //virtual wxStaticText* getLabel();
    virtual bool hasControls() const;
    //virtual void setDefault(const wxString& defValue);*/
    virtual wxArrayString getComboBoxItems();
    void loadStylers(const wxString& styleFileName);
    void loadStyles(const wxString& language);
    void loadStyle(const wxString& styleName);

    void saveStyle(const wxString& styleName);
private:
    FRStyleManager* styleManagerM;

    wxStaticText* captionBeforeM;

    wxComboBox* fileComboBoxM;
    std::unique_ptr<wxEvtHandler> fileComboBoxHandlerM;
    void OnSelectFileComboBox(wxCommandEvent& event);

    wxListBox* stylersListBoxM;
    std::unique_ptr<wxEvtHandler> stylersListBoxHandlerM;
    void OnSelectStylersListBox(wxCommandEvent& event);

    wxListBox* styleListBoxM;
    std::unique_ptr<wxEvtHandler> styleListBoxHandlerM;
    void OnSelectStyleListBox(wxCommandEvent& event);

    wxColourPickerCtrl* foregroundPickerM;
    std::unique_ptr<wxEvtHandler> foregroundPickerHandlerM;

    wxColourPickerCtrl* backgroundPickerM;
    std::unique_ptr<wxEvtHandler> backgroundPickerHandlerM;

    wxComboBox* fontNameComboBoxM;
    std::unique_ptr<wxEvtHandler> fontNameComboBoxHandlerM;
    wxComboBox* fontSizeComboBoxM;
    std::unique_ptr<wxEvtHandler> fontSizeComboBoxHandlerM;

    wxCheckBox* blodCheckBoxM;
    std::unique_ptr<wxEvtHandler> blodCheckBoxHandlerM;
    wxCheckBox* italicCheckBoxM;
    std::unique_ptr<wxEvtHandler> italicCheckBoxHandlerM;
    wxCheckBox* underlineCheckBoxM;
    std::unique_ptr<wxEvtHandler> underlineCheckBoxHandlerM;

    wxRadioBox* caseRadioBoxM;
    std::unique_ptr<wxEvtHandler> caseRadioBoxHandlerM;

    wxStaticText* captionSecondaryM;
    wxComboBox* fileSecondaryComboBoxM;

    wxRadioBox* styleAciveRadioBoxM;
    std::unique_ptr<wxEvtHandler> styleAciveRadioBoxHandlerM;


    void OnChangeStyle(wxCommandEvent& event);

};


#endif //PREFERENCESDIALOG_STYLE_H

