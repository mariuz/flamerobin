/*
  Copyright (c) 2023-2023 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <vector>
#include <functional>
#include <algorithm>

#include <wx/checklst.h>
#include <wx/clrpicker.h>
#include <wx/dir.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/fontdlg.h>
#include <wx/fontenum.h>
#include <wx/gbsizer.h>
#include <wx/listbox.h>
#include <wx/spinctrl.h>
#include <wx/tokenzr.h>
#include <wx/xml/xml.h>


#include "PreferencesDialogStyle.h"
#include "FRStyleManager.h"


PrefDlgStyleSetting::PrefDlgStyleSetting(wxPanel* page, PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent), styleManagerM(0), fileComboBoxM(0), captionBeforeM(0),
    stylersListBoxM(0), styleListBoxM(0), fontNameComboBoxM(0), fontSizeComboBoxM(0)
{
    styleManagerM = new FRStyleManager; 
}

PrefDlgStyleSetting::~PrefDlgStyleSetting()
{
    if (fileComboBoxM && fileComboBoxHandlerM.get())
        fileComboBoxM->PopEventHandler();
    if (stylersListBoxM && stylersListBoxHandlerM.get())
        stylersListBoxM->PopEventHandler();
    if (styleListBoxM && styleListBoxHandlerM.get())
        styleListBoxM->PopEventHandler();

    if (foregroundPickerM && foregroundPickerHandlerM.get())
        foregroundPickerM->PopEventHandler();
    if (backgroundPickerM && backgroundPickerHandlerM.get())
        backgroundPickerM->PopEventHandler();

    if (fontNameComboBoxM && fontNameComboBoxHandlerM.get())
        fontNameComboBoxM->PopEventHandler();
    if (fontSizeComboBoxM && fontSizeComboBoxHandlerM.get())
        fontSizeComboBoxM->PopEventHandler();

    if (blodCheckBoxM && blodCheckBoxHandlerM.get())
        blodCheckBoxM->PopEventHandler();
    if (italicCheckBoxM && italicCheckBoxHandlerM.get())
        italicCheckBoxM->PopEventHandler();
    if (underlineCheckBoxM && underlineCheckBoxHandlerM.get())
        underlineCheckBoxM->PopEventHandler();

    if (caseRadioBoxM && caseRadioBoxHandlerM.get())
        caseRadioBoxM->PopEventHandler();

    if (styleAciveRadioBoxM && styleAciveRadioBoxHandlerM.get())
        styleAciveRadioBoxM->PopEventHandler();

}

bool PrefDlgStyleSetting::createControl(bool WXUNUSED(ignoreerrors))
{
    //captionBeforeM = new wxStaticText(getPage(), wxID_ANY, "Select theme:");
    captionBeforeM =  new wxStaticText(getPage(), wxID_ANY, captionM); 
    if (!descriptionM.empty())
        captionBeforeM->SetToolTip(descriptionM);

    fileComboBoxM = new wxComboBox(getPage(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, getComboBoxItems());
    if (!descriptionM.empty())
        fileComboBoxM->SetToolTip(descriptionM);
    fileComboBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgStyleSetting::OnSelectFileComboBox, this, std::placeholders::_1)));
    fileComboBoxM->PushEventHandler(fileComboBoxHandlerM.get());
    fileComboBoxHandlerM->Connect(fileComboBoxM->GetId(),
        wxEVT_COMMAND_COMBOBOX_SELECTED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));


    stylersListBoxM = new wxListBox(getPage(), wxID_ANY, wxDefaultPosition, wxDefaultSize);
    stylersListBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgStyleSetting::OnSelectStylersListBox, this, std::placeholders::_1)));
    stylersListBoxM->PushEventHandler(stylersListBoxHandlerM.get());
    stylersListBoxHandlerM->Connect(stylersListBoxM->GetId(),
        wxEVT_COMMAND_LISTBOX_SELECTED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    styleListBoxM = new wxListBox(getPage(), wxID_ANY, wxDefaultPosition, wxDefaultSize);
    styleListBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgStyleSetting::OnSelectStyleListBox, this, std::placeholders::_1)));
    styleListBoxM->PushEventHandler(styleListBoxHandlerM.get());
    styleListBoxHandlerM->Connect(styleListBoxM->GetId(),
        wxEVT_COMMAND_LISTBOX_SELECTED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    foregroundPickerM = new wxColourPickerCtrl(getPage(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize);
    foregroundPickerHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgStyleSetting::OnChangeStyle, this, std::placeholders::_1)));
    foregroundPickerM->PushEventHandler(foregroundPickerHandlerM.get());
    foregroundPickerHandlerM->Connect(foregroundPickerM->GetId(),
        wxEVT_COMMAND_COLOURPICKER_CHANGED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    backgroundPickerM = new wxColourPickerCtrl(getPage(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize);
    backgroundPickerHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgStyleSetting::OnChangeStyle, this, std::placeholders::_1)));
    backgroundPickerM->PushEventHandler(backgroundPickerHandlerM.get());
    backgroundPickerHandlerM->Connect(backgroundPickerM->GetId(),
        wxEVT_COMMAND_COLOURPICKER_CHANGED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));


    wxArrayString strArray = wxFontEnumerator::GetFacenames();
    strArray.Sort();
    fontNameComboBoxM = new wxComboBox(getPage(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, strArray, wxCB_READONLY);
    fontNameComboBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgStyleSetting::OnChangeStyle, this, std::placeholders::_1)));
    fontNameComboBoxM->PushEventHandler(fontNameComboBoxHandlerM.get());
    fontNameComboBoxHandlerM->Connect(fontNameComboBoxM->GetId(),
        wxEVT_COMMAND_COMBOBOX_SELECTED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    strArray.Clear();
    for (int i = 5; i <= 30; i++) {
        if (i <= 12 || i % 2 == 0) {
            strArray.Add(wxString::Format("%i", i));
        }
    }
    fontSizeComboBoxM = new wxComboBox(getPage(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, strArray, wxCB_READONLY);
    fontSizeComboBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgStyleSetting::OnChangeStyle, this, std::placeholders::_1)));
    fontSizeComboBoxM->PushEventHandler(fontSizeComboBoxHandlerM.get());
    fontSizeComboBoxHandlerM->Connect(fontSizeComboBoxM->GetId(),
        wxEVT_COMMAND_COMBOBOX_SELECTED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));


    blodCheckBoxM = new wxCheckBox(getPage(), wxID_ANY, "Bold", wxDefaultPosition, wxDefaultSize);
    blodCheckBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgStyleSetting::OnChangeStyle, this, std::placeholders::_1)));
    blodCheckBoxM->PushEventHandler(blodCheckBoxHandlerM.get());
    blodCheckBoxHandlerM->Connect(blodCheckBoxM->GetId(),
        wxEVT_COMMAND_CHECKBOX_CLICKED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    italicCheckBoxM = new wxCheckBox(getPage(), wxID_ANY, "Italic", wxDefaultPosition, wxDefaultSize);
    italicCheckBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgStyleSetting::OnChangeStyle, this, std::placeholders::_1)));
    italicCheckBoxM->PushEventHandler(italicCheckBoxHandlerM.get());
    italicCheckBoxHandlerM->Connect(italicCheckBoxM->GetId(),
        wxEVT_COMMAND_CHECKBOX_CLICKED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    underlineCheckBoxM = new wxCheckBox(getPage(), wxID_ANY, "Underline", wxDefaultPosition, wxDefaultSize);
    underlineCheckBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgStyleSetting::OnChangeStyle, this, std::placeholders::_1)));
    underlineCheckBoxM->PushEventHandler(underlineCheckBoxHandlerM.get());
    underlineCheckBoxHandlerM->Connect(underlineCheckBoxM->GetId(),
        wxEVT_COMMAND_CHECKBOX_CLICKED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    wxArrayString caseChoices;
    caseChoices.Add("Mixed");
    caseChoices.Add("Upper");
    caseChoices.Add("Lower");
    caseChoices.Add("Camel");
    caseRadioBoxM = new wxRadioBox(getPage(), wxID_ANY, "Font Case", wxDefaultPosition, wxDefaultSize, caseChoices, 4);
    caseRadioBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgStyleSetting::OnChangeStyle, this, std::placeholders::_1)));
    caseRadioBoxM->PushEventHandler(caseRadioBoxHandlerM.get());
    caseRadioBoxHandlerM->Connect(caseRadioBoxM->GetId(),
        wxEVT_COMMAND_RADIOBOX_SELECTED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    captionSecondaryM = new wxStaticText(getPage(), wxID_ANY, "Select secondary theme:");
    fileSecondaryComboBoxM = new wxComboBox(getPage(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, getComboBoxItems());

    caseChoices.Clear();
    caseChoices.Add("Primary");
    caseChoices.Add("Secondary");
    styleAciveRadioBoxM = new wxRadioBox(getPage(), wxID_ANY, "Style Active", wxDefaultPosition, wxDefaultSize, caseChoices, 2);
    styleAciveRadioBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgStyleSetting::OnChangeStyle, this, std::placeholders::_1)));
    styleAciveRadioBoxM->PushEventHandler(styleAciveRadioBoxHandlerM.get());
    styleAciveRadioBoxHandlerM->Connect(styleAciveRadioBoxM->GetId(),
        wxEVT_COMMAND_RADIOBOX_SELECTED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));


    return true;
}

bool PrefDlgStyleSetting::loadFromTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;

    if (fileComboBoxM)
    {
        wxString value = defaultM;
        config.getValue(keyM, value);
        fileComboBoxM->SetValue(value);

        loadStylers(fileComboBoxM->GetValue());
    }

    if (fileSecondaryComboBoxM)
    {
        wxString value = defaultM;
        config.getValue("StyleThemeSecondary", value);
        fileSecondaryComboBoxM->SetValue(value);
    }

    if (styleAciveRadioBoxM)
    {
        int value = 0;
        config.getValue("StyleActive", value);
        styleAciveRadioBoxM->SetSelection(value);
    }


    enableControls(true);

    return true;
}

bool PrefDlgStyleSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE
        //&& xmln->GetName() == "file"
        )
    {
        /*wxString dirName = config().getXmlStylesPath();
        wxString fileSpec = _T("*.xml");
        wxArrayString files;
        itemsM.clear();

        if (wxDir::GetAllFiles(dirName, &files, fileSpec, wxDIR_FILES) > 0) {
            wxString name, ext;
            wxString allFileNames;
            for (size_t i = 0; i < files.GetCount(); i++) {
                wxFileName::SplitPath(files[i], NULL, &name, &ext);
                itemsM.Add(name);
            }
        }

        */
    }

    return PrefDlgSetting::parseProperty(xmln);
}

bool PrefDlgStyleSetting::saveToTargetConfig(Config& config)
{
    config.setValue(keyM, fileComboBoxM->GetValue());
    config.setValue("StyleThemeSecondary", fileSecondaryComboBoxM->GetValue());
    config.setValue("StyleActive", styleAciveRadioBoxM->GetSelection());

    wxString userStyleFolder = config.getUserLocalDataDir() + wxFileName::GetPathSeparator() + "xml-styles" + wxFileName::GetPathSeparator();
    if (!wxDirExists(userStyleFolder))
    {
        wxFileName mFile = getStyleManager().getFileNamePrimary();
        mFile.SetPath(userStyleFolder);

        wxString sourceDirName = config.getHomePath() + "xml-styles" + wxFileName::GetPathSeparator();

        wxString fileSpec = _T("*.xml");
        wxArrayString files;
        if (wxDir::GetAllFiles(sourceDirName, &files, fileSpec, wxDIR_FILES) > 0) {
            wxMkdir(userStyleFolder);
            wxString name, ext;
            for (size_t i = 0; i < files.GetCount(); i++) {
                wxFileName::SplitPath(files[i], NULL, &name, &ext);
                wxCopyFile(files[i], userStyleFolder + name + "." + ext);
            }
        }
        getStyleManager().setFileNamePrimary(mFile);
    }

    getStyleManager().saveStyle();
    stylerManager().loadConfig();
    stylerManager().loadStyle();

    return true;
}

bool PrefDlgStyleSetting::cancelChanges(Config& )
{
    stylerManager().loadConfig();
    stylerManager().loadStyle();

    return true;
}

FRStyleManager& PrefDlgStyleSetting::getStyleManager()
{
    return stylerManager();
}

void PrefDlgStyleSetting::addControlsToSizer(wxSizer* sizer)
{

    wxSizer* sizerV = new wxBoxSizer(wxVERTICAL);
    sizerV->Add(0, styleguide().getFrameMargin(wxTOP));
    {
        wxSizer* sz = new wxBoxSizer(wxHORIZONTAL);
        sz->Add(styleguide().getFrameMargin(wxLEFT), 0);
        if (captionBeforeM)
        {
            sz->Add(captionBeforeM, 0, wxALIGN_CENTER_VERTICAL);
            sz->Add(styleguide().getControlLabelMargin(), 0);
        }
        sz->Add(fileComboBoxM, 1, wxALIGN_CENTER_VERTICAL);

        sizerV->Add(sz, 0, wxEXPAND);
    }

    {
        wxSizer* sz = new wxBoxSizer(wxHORIZONTAL);
        sz->Add(styleguide().getFrameMargin(wxLEFT), 0);
        {
            wxStaticBoxSizer* sbz = new wxStaticBoxSizer(new wxStaticBox(getPage(), -1, "Language"), wxVERTICAL);
            sbz->Add(0, styleguide().getFrameMargin(wxTOP));
            sbz->Add(stylersListBoxM, 1, wxEXPAND);
            sbz->Add(0, styleguide().getFrameMargin(wxBOTTOM));
            sz->Add(sbz, 1, wxEXPAND);

        }
        sz->Add(0, styleguide().getRelatedControlMargin(wxHORIZONTAL));
        {
            wxStaticBoxSizer* sbz = new wxStaticBoxSizer(new wxStaticBox(getPage(), -1, "Style"), wxVERTICAL);
            sbz->Add(0, styleguide().getFrameMargin(wxTOP));
            sbz->Add(styleListBoxM, 0, wxEXPAND);
            sbz->Add(0, styleguide().getFrameMargin(wxBOTTOM));
            sz->Add(sbz, 1, wxEXPAND);
        }
        sz->Add(styleguide().getFrameMargin(wxRIGHT), 0);
        sizerV->Add(sz, 1, wxEXPAND);
    }

    {
        wxSizer* sz = new wxBoxSizer(wxHORIZONTAL);
        sz->Add(styleguide().getFrameMargin(wxLEFT), 0);
        {
            wxStaticBoxSizer* sbz = new wxStaticBoxSizer(new wxStaticBox(getPage(), -1, "Color Style"), wxVERTICAL);
            sbz->Add(0, styleguide().getFrameMargin(wxTOP));
            {
                wxSizer* s = new wxBoxSizer(wxHORIZONTAL);
                s->Add(new wxStaticText(getPage(), -1, "Foreground Color"), 0, wxALIGN_CENTER_VERTICAL);
                s->Add(styleguide().getControlLabelMargin(), 0);
                s->Add(foregroundPickerM, 0, wxALIGN_CENTER_VERTICAL);
                sbz->Add(s, 0, wxEXPAND);
            }
            sbz->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
            {
                wxSizer* s = new wxBoxSizer(wxHORIZONTAL);
                s->Add(new wxStaticText(getPage(), -1, "Background Color"), 0, wxALIGN_CENTER_VERTICAL);
                s->Add(styleguide().getControlLabelMargin(), 0);
                s->Add(backgroundPickerM, 0, wxALIGN_CENTER_VERTICAL);
                sbz->Add(s, 0, wxEXPAND);
            }

            sbz->Add(0, styleguide().getFrameMargin(wxBOTTOM));
            sz->Add(sbz, 0, wxEXPAND);

        }
        sz->Add(0, styleguide().getUnrelatedControlMargin(wxHORIZONTAL));
        {
            wxStaticBoxSizer* sbz = new wxStaticBoxSizer(new wxStaticBox(getPage(), -1, "Font Style"), wxVERTICAL);
            sbz->Add(0, styleguide().getFrameMargin(wxTOP));
            {
                wxSizer* s = new wxBoxSizer(wxHORIZONTAL);
                s->Add(styleguide().getFrameMargin(wxLEFT), 0);
                s->Add(new wxStaticText(getPage(), -1, "Font Name"), 0, wxALIGN_CENTER_VERTICAL);
                s->Add(styleguide().getControlLabelMargin(), 0);
                s->Add(fontNameComboBoxM, 1, wxALIGN_CENTER_VERTICAL);
                sbz->Add(s, 0, wxEXPAND);
            }
            sbz->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
            {
                wxSizer* s = new wxBoxSizer(wxHORIZONTAL);
                s->Add(styleguide().getFrameMargin(wxLEFT), 0);
                s->Add(new wxStaticText(getPage(), -1, "Font Size"), 0, wxALIGN_CENTER_VERTICAL);
                s->Add(styleguide().getControlLabelMargin(), 0);
                s->Add(fontSizeComboBoxM, 1, wxALIGN_CENTER_VERTICAL);
                sbz->Add(s, 0, wxEXPAND);
            }
            sbz->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
            {
                wxGridSizer* gz = new wxGridSizer(1, 3,
                    styleguide().getCheckboxSpacing(),
                    styleguide().getUnrelatedControlMargin(wxHORIZONTAL));

                gz->Add(blodCheckBoxM, 0, wxEXPAND);
                gz->Add(italicCheckBoxM, 0, wxEXPAND);
                gz->Add(underlineCheckBoxM, 0, wxEXPAND);
                sbz->Add(gz, 0, wxEXPAND);
            }
            sbz->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
            {
                wxSizer* s = new wxBoxSizer(wxHORIZONTAL);
                s->Add(styleguide().getFrameMargin(wxLEFT), 0);
                s->Add(styleguide().getControlLabelMargin(), 0);
                s->Add(caseRadioBoxM, 1, wxALIGN_CENTER_VERTICAL);
                sbz->Add(s, 0, wxEXPAND);
            }

            sbz->Add(0, styleguide().getFrameMargin(wxBOTTOM));
            sz->Add(sbz, 0, wxEXPAND);

        }

        sz->Add(styleguide().getFrameMargin(wxRIGHT), 0);
        sizerV->Add(sz, 1, wxEXPAND);


    }

    {
        wxSizer* sz = new wxBoxSizer(wxHORIZONTAL);
        sz->Add(styleguide().getFrameMargin(wxLEFT), 0);
        if (captionSecondaryM)
        {
            sz->Add(captionSecondaryM, 0, wxALIGN_CENTER_VERTICAL);
            sz->Add(styleguide().getControlLabelMargin(), 0);
        }

        sz->Add(fileSecondaryComboBoxM, 1, wxALIGN_CENTER_VERTICAL);
        sizerV->Add(sz, 0, wxEXPAND);

    }
    {
        wxSizer* sz = new wxBoxSizer(wxHORIZONTAL);
        sz->Add(styleguide().getFrameMargin(wxLEFT), 0);
        sz->Add(styleguide().getControlLabelMargin(), 0);
        sz->Add(styleAciveRadioBoxM, 1, wxALIGN_CENTER_VERTICAL);
        sz->Add(styleguide().getFrameMargin(wxLEFT), 0);
        sizerV->Add(sz, 0, wxEXPAND);

    }
    sizer->Add(sizerV, 1, wxEXPAND);
    sizer->SetItemMinSize(sizerV, -1, -1);
}

void PrefDlgStyleSetting::enableControls(bool /*enabled*/)
{
}

bool PrefDlgStyleSetting::hasControls() const
{
    return true;
}

wxArrayString PrefDlgStyleSetting::getComboBoxItems()
{
    wxArrayString lItems = wxArrayString();

    wxString dirName = config().getXmlStylesPath();
    wxString fileSpec = _T("*.xml");
    wxArrayString files;
    lItems.clear();

    if (wxDir::GetAllFiles(dirName, &files, fileSpec, wxDIR_FILES) > 0) {
        wxString name, ext;
        wxString allFileNames;
        for (size_t i = 0; i < files.GetCount(); i++) {
            wxFileName::SplitPath(files[i], NULL, &name, &ext);
            lItems.Add(name);
        }
    }

    return lItems;
}

void PrefDlgStyleSetting::loadStylers(const wxString& styleFileName)
{
    getStyleManager().setFileNamePrimary(wxFileName(config().getXmlStylesPath(), styleFileName.IsEmpty() ? "stylers.xml" : styleFileName + ".xml"));
    getStyleManager().loadStyle();


    stylersListBoxM->Clear();
    stylersListBoxM->Insert("Global Styles", 0);

    FRStylers stylers = getStyleManager().getLexerStylers();

    //if (stylers) 
    {
        int i = 0;
        for (FRStyler* styler : stylers.getStylers()) {
            stylersListBoxM->Insert(styler->getStylerDesc(), ++i);
        }
    }

    stylersListBoxM->Select(0);
    loadStyles(stylersListBoxM->GetString(stylersListBoxM->GetSelection()));

}

void PrefDlgStyleSetting::loadStyles(const wxString& language)
{
    FRStyles* styles;
    styleListBoxM->Clear();

    if (language == "Global Styles") {
        styles = getStyleManager().getGlobalStyler();
    }
    else {
        styles = getStyleManager().getLexerStylers().getStylerByDesc(language);
    }

    if (styles) {
        int i = 0;
        for (FRStyle* style : styles->getStyles()) {
            styleListBoxM->Insert(style->getStyleDesc(), i++);
        }
    }
    styleListBoxM->Select(0);
    loadStyle(styleListBoxM->GetString(styleListBoxM->GetSelection()));

}

void PrefDlgStyleSetting::loadStyle(const wxString& styleName)
{

    FRStyle* style = getStyleManager().getStylerByDesc(stylersListBoxM->GetString(stylersListBoxM->GetSelection()))->getStyleByName(styleName);

    wxString fontName = style->getFontName();
    fontNameComboBoxM->SetSelection(fontNameComboBoxM->FindString(fontName));
    fontSizeComboBoxM->SetSelection(fontSizeComboBoxM->FindString(wxString::Format("%i", style->getFontSize())));

    foregroundPickerM->SetColour(style->getfgColor());
    backgroundPickerM->SetColour(style->getbgColor());

    blodCheckBoxM->SetValue(style->getFontStyle() & FONTSTYLE_BOLD);
    italicCheckBoxM->SetValue(style->getFontStyle() & FONTSTYLE_ITALIC);
    underlineCheckBoxM->SetValue(style->getFontStyle() & FONTSTYLE_UNDERLINE);

    caseRadioBoxM->SetSelection(style->getCaseVisible());

}

void PrefDlgStyleSetting::saveStyle(const wxString& styleName)
{
    FRStyle* style = getStyleManager().getStylerByDesc(stylersListBoxM->GetString(stylersListBoxM->GetSelection()))->getStyleByName(styleName);

    int i = fontNameComboBoxM->GetSelection();
    if (i>0)
        style->setFontName(fontNameComboBoxM->GetString(fontNameComboBoxM->GetSelection()));
    style->setFontSize(atoi(fontSizeComboBoxM->GetString(fontSizeComboBoxM->GetSelection()).ToStdString().c_str()));

    style->setfgColor(foregroundPickerM->GetColour());
    style->setbgColor(backgroundPickerM->GetColour());

    int fontStyle = 0;
    if (blodCheckBoxM->IsChecked())
        fontStyle |= FONTSTYLE_BOLD;
    if (italicCheckBoxM->IsChecked())
        fontStyle |= FONTSTYLE_ITALIC;
    if (underlineCheckBoxM->IsChecked())
        fontStyle |= FONTSTYLE_UNDERLINE;

    style->setFontStyle(fontStyle);

    style->setCaseVisible(caseRadioBoxM->GetSelection());

    getStyleManager().notifyObservers();

}

void PrefDlgStyleSetting::OnSelectFileComboBox(wxCommandEvent& /*event*/)
{
    loadStylers(fileComboBoxM->GetValue());
}

void PrefDlgStyleSetting::OnSelectStylersListBox(wxCommandEvent& /*event*/)
{
    loadStyles(stylersListBoxM->GetString(stylersListBoxM->GetSelection()));
}

void PrefDlgStyleSetting::OnSelectStyleListBox(wxCommandEvent& /*event*/)
{
    loadStyle(styleListBoxM->GetString(styleListBoxM->GetSelection()));
}

void PrefDlgStyleSetting::OnChangeStyle(wxCommandEvent& /*event*/)
{
    saveStyle(styleListBoxM->GetString(styleListBoxM->GetSelection()));
}



