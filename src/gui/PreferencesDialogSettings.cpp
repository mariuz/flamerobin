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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

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

#include <vector>

#include <functional>

#include "config/Config.h"
#include "frutils.h"
#include "gui/PreferencesDialog.h"
#include "gui/StyleGuide.h"
#include "gui/FRStyle.h"
#include "metadata/column.h"
#include "metadata/relation.h"

static const wxString getNodeContent(wxXmlNode* node, const wxString& defvalue)
{
    wxString content;
    for (wxXmlNode* n = node->GetChildren(); (n); n = n->GetNext())
    {
        int type = n->GetType();
        if (type == wxXML_TEXT_NODE || type == wxXML_CDATA_SECTION_NODE)
            content += n->GetContent();
        else if (type == wxXML_ELEMENT_NODE && n->GetName() == "br")
            content += "\n";
    }
    return content.empty() ? defvalue : content;
}

// PrefDlgSetting class
PrefDlgSetting::PrefDlgSetting(wxPanel* page, PrefDlgSetting* parent)
    : pageM(page), parentM(parent), relatedM(false), alignmentGroupM(-1),
        sizerProportionM(0)
{
    if (parent)
        parent->addEnabledSetting(this);
}

PrefDlgSetting::~PrefDlgSetting()
{
}

void PrefDlgSetting::addEnabledSetting(PrefDlgSetting* setting)
{
    if (setting)
        enabledSettingsM.push_back(setting);
}

bool PrefDlgSetting::addToSizer(wxSizer* sizer, PrefDlgSetting* previous)
{
    if (!hasControls())
        return false;

    wxBoxSizer* hsizer = new wxBoxSizer(wxHORIZONTAL);
    int level = getLevel();
    if (level > 0)
        hsizer->Add(getControlIndentation(level), 0);
    addControlsToSizer(hsizer);

    if (previous)
    {
        int margin;
        if (isRelatedTo(previous))
            margin = styleguide().getRelatedControlMargin(wxVERTICAL);
        else
            margin = styleguide().getUnrelatedControlMargin(wxVERTICAL);
        sizer->Add(0, margin);
    }
    sizer->Add(hsizer, getSizerProportion(), wxEXPAND | wxFIXED_MINSIZE);
    return true;
}

void PrefDlgSetting::alignControl(int left)
{
    wxStaticText* label = getLabel();
    if (label)
    {
        left -= getControlIndentation(getLevel());
        int w = left - styleguide().getControlLabelMargin();
        wxSize sz(label->GetSize());
        if (sz.GetWidth() < w)
        {
            sz.SetWidth(w);
            label->SetSize(sz);
            label->SetMinSize(sz);
        }
    }
}

bool PrefDlgSetting::checkTargetConfigProperties() const
{
    if (keyM.empty())
    {
        wxLogError(_("Setting \"%s\" has no config setting key"),
            captionM.c_str());
        return false;
    }
    return true;
}

void PrefDlgSetting::enableEnabledSettings(bool enabled)
{
    std::list<PrefDlgSetting*>::iterator it;
    for (it = enabledSettingsM.begin(); it != enabledSettingsM.end(); it++)
        (*it)->enableControls(enabled);
}

inline int PrefDlgSetting::getControlIndentation(int level) const
{
    return level * 20;
}

int PrefDlgSetting::getControlLeft()
{
    int left = getControlIndentation(getLevel());

    wxStaticText* label = getLabel();
    if (label)
    {
        left += label->GetSize().GetWidth();
        left += styleguide().getControlLabelMargin();
    }
    return left;
}

int PrefDlgSetting::getControlAlignmentGroup() const
{
    return alignmentGroupM;
}

wxStaticText* PrefDlgSetting::getLabel()
{
    return 0;
}

int PrefDlgSetting::getLevel() const
{
    return (parentM != 0) ? parentM->getLevel() + 1 : 0;
}

wxPanel* PrefDlgSetting::getPage() const
{
    return pageM;
}

PrefDlgSetting* PrefDlgSetting::getParent() const
{
    return parentM;
}

int PrefDlgSetting::getSizerProportion() const
{
    return sizerProportionM;
}

bool PrefDlgSetting::isRelatedTo(PrefDlgSetting* prevSetting) const
{
    if (!prevSetting)
        return false;
    if (relatedM)
        return true;
    int prevLevel = prevSetting->getLevel();
    int level = getLevel();

    return (level > prevLevel) || (level > 0 && level == prevLevel);
}

bool PrefDlgSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE)
    {
        wxString name(xmln->GetName());
        wxString value(getNodeContent(xmln, wxEmptyString));

        if (name == "caption")
            captionM = value;
        else if (name == "description")
            descriptionM = value;
        else if (name == "key")
            keyM = value;
        else if (name == "default")
            setDefault(value);
        else if (name == "related")
            relatedM = true;
        else if (name == "aligngroup")
        {
            long l;
            if (value.ToLong(&l) && l > 0)
                alignmentGroupM = l;
        }
        else if (name == "proportion")
        {
            long l;
            if (value.ToLong(&l) && l >= 0)
                sizerProportionM = l;
        }
    }
    return true;
}

void PrefDlgSetting::setDefault(const wxString& WXUNUSED(defValue))
{
}

// PrefDlgEventHandler helper
typedef std::function<void (wxCommandEvent&)> CommandEventHandler;

class PrefDlgEventHandler: public wxEvtHandler
{
public:
    PrefDlgEventHandler(CommandEventHandler handler)
        : wxEvtHandler(), handlerM(handler)
    {
    }

    void OnCommandEvent(wxCommandEvent& event)
    {
        if (handlerM)
            handlerM(event);
    }
private:
    CommandEventHandler handlerM;
};

// PrefDlgCheckboxSetting class
class PrefDlgCheckboxSetting: public PrefDlgSetting
{
public:
    PrefDlgCheckboxSetting(wxPanel* page, PrefDlgSetting* parent);
    ~PrefDlgCheckboxSetting();

    virtual bool createControl(bool ignoreerrors);
    virtual bool loadFromTargetConfig(Config& config);
    virtual bool saveToTargetConfig(Config& config);
protected:
    virtual void addControlsToSizer(wxSizer* sizer);
    virtual void enableControls(bool enabled);
    virtual bool hasControls() const;
    virtual void setDefault(const wxString& defValue);
private:
    wxCheckBox* checkBoxM;
    bool defaultM;

    std::unique_ptr<wxEvtHandler> checkBoxHandlerM;
    void OnCheckBox(wxCommandEvent& event);
};

PrefDlgCheckboxSetting::PrefDlgCheckboxSetting(wxPanel* page,
        PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent), checkBoxM(0), defaultM(false)
{
}

PrefDlgCheckboxSetting::~PrefDlgCheckboxSetting()
{
    if (checkBoxM && checkBoxHandlerM.get())
        checkBoxM->PopEventHandler();
}

void PrefDlgCheckboxSetting::addControlsToSizer(wxSizer* sizer)
{
    if (checkBoxM)
        sizer->Add(checkBoxM, 0, wxFIXED_MINSIZE);
}

bool PrefDlgCheckboxSetting::createControl(bool WXUNUSED(ignoreerrors))
{
    checkBoxM = new wxCheckBox(getPage(), wxID_ANY, captionM);
    if (!descriptionM.empty())
        checkBoxM->SetToolTip(descriptionM);

    checkBoxHandlerM.reset(new PrefDlgEventHandler(
            std::bind(&PrefDlgCheckboxSetting::OnCheckBox, this,  std::placeholders::_1)
        )
    );

    checkBoxM->PushEventHandler(checkBoxHandlerM.get());

    checkBoxHandlerM->Connect(checkBoxM->GetId(),
        wxEVT_COMMAND_CHECKBOX_CLICKED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    return true;
}

void PrefDlgCheckboxSetting::enableControls(bool enabled)
{
    if (checkBoxM)
        checkBoxM->Enable(enabled);
}

bool PrefDlgCheckboxSetting::hasControls() const
{
    return checkBoxM != 0;
}

bool PrefDlgCheckboxSetting::loadFromTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    bool checked = defaultM;
    if (checkBoxM)
    {
        config.getValue(keyM, checked);
        checkBoxM->SetValue(checked);
    }
    enableEnabledSettings(checked);
    return true;
}

bool PrefDlgCheckboxSetting::saveToTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (checkBoxM)
        config.setValue(keyM, checkBoxM->GetValue());
    return true;
}

void PrefDlgCheckboxSetting::setDefault(const wxString& defValue)
{
    defaultM = (defValue == "on" || defValue == "yes");
    long l;
    if (defValue.ToLong(&l) && l != 0)
        defaultM = true;
}

void PrefDlgCheckboxSetting::OnCheckBox(wxCommandEvent& event)
{
    enableEnabledSettings(event.IsChecked());
}

// PrefDlgRadioboxSetting class
class PrefDlgRadioboxSetting: public PrefDlgSetting
{
public:
    PrefDlgRadioboxSetting(wxPanel* page, PrefDlgSetting* parent);

    virtual bool createControl(bool ignoreerrors);
    virtual bool loadFromTargetConfig(Config& config);
    virtual bool parseProperty(wxXmlNode* xmln);
    virtual bool saveToTargetConfig(Config& config);
protected:
    virtual void addControlsToSizer(wxSizer* sizer);
    virtual void enableControls(bool enabled);
    virtual bool hasControls() const;
    virtual void setDefault(const wxString& defValue);
private:
    wxRadioBox* radioBoxM;
    wxArrayString choicesM;
    int defaultM;
};

PrefDlgRadioboxSetting::PrefDlgRadioboxSetting(wxPanel* page,
        PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent), radioBoxM(0), defaultM(wxNOT_FOUND)
{
}

void PrefDlgRadioboxSetting::addControlsToSizer(wxSizer* sizer)
{
    if (radioBoxM)
        sizer->Add(radioBoxM, 1, wxFIXED_MINSIZE);
}

bool PrefDlgRadioboxSetting::createControl(bool ignoreerrors)
{
    if (choicesM.GetCount() == 0)
    {
        if (!ignoreerrors)
            wxLogError(_("Radiobox \"%s\" has no options"), captionM.c_str());
        return ignoreerrors;
    }
    radioBoxM = new wxRadioBox(getPage(), wxID_ANY, captionM,
        wxDefaultPosition, wxDefaultSize, choicesM, 1);
    if (!descriptionM.empty())
        radioBoxM->SetToolTip(descriptionM);
    return true;
}

void PrefDlgRadioboxSetting::enableControls(bool enabled)
{
    if (radioBoxM)
        radioBoxM->Enable(enabled);
}

bool PrefDlgRadioboxSetting::hasControls() const
{
    return radioBoxM != 0;
}

bool PrefDlgRadioboxSetting::loadFromTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (radioBoxM)
    {
        int value = defaultM;
        config.getValue(keyM, value);
        if (value >= 0 && value < (int)choicesM.GetCount())
            radioBoxM->SetSelection(value);
    }
    return true;
}

bool PrefDlgRadioboxSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE
        && xmln->GetName() == "option")
    {
        wxString optcaption;

        for (wxXmlNode* xmlc = xmln->GetChildren(); xmlc != 0;
            xmlc = xmlc->GetNext())
        {
            if (xmlc->GetType() != wxXML_ELEMENT_NODE)
                continue;
            wxString value(getNodeContent(xmlc, wxEmptyString));
            if (xmlc->GetName() == "caption")
                optcaption = value;
/*
            else if (xmlc->GetName() == "value"))
                optvalue = value;
*/
        }
        // for the time being values are the index of the caption in the array
        if (!optcaption.empty())
            choicesM.Add(optcaption);
    }
    return PrefDlgSetting::parseProperty(xmln);
}

bool PrefDlgRadioboxSetting::saveToTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (radioBoxM)
        config.setValue(keyM, radioBoxM->GetSelection());
    return true;
}

void PrefDlgRadioboxSetting::setDefault(const wxString& defValue)
{
    long l;
    if (defValue.ToLong(&l) && l >= 0 && l < long(choicesM.GetCount()))
        defaultM = l;
}

// PrefDlgIntEditSetting class
class PrefDlgIntEditSetting: public PrefDlgSetting
{
public:
    PrefDlgIntEditSetting(wxPanel* page, PrefDlgSetting* parent);

    virtual bool createControl(bool ignoreerrors);
    virtual bool loadFromTargetConfig(Config& config);
    virtual bool parseProperty(wxXmlNode* xmln);
    virtual bool saveToTargetConfig(Config& config);
protected:
    virtual void addControlsToSizer(wxSizer* sizer);
    virtual void enableControls(bool enabled);
    virtual wxStaticText* getLabel();
    virtual bool hasControls() const;
    virtual void setDefault(const wxString& defValue);
private:
    wxStaticText* captionAfterM;
    wxStaticText* captionBeforeM;
    wxSpinCtrl* spinctrlM;
    int maxValueM;
    int minValueM;
    int defaultM;
};

PrefDlgIntEditSetting::PrefDlgIntEditSetting(wxPanel* page,
        PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent), captionAfterM(0), captionBeforeM(0),
        spinctrlM(0), maxValueM(32767), minValueM(0), defaultM(0)
{
}

void PrefDlgIntEditSetting::addControlsToSizer(wxSizer* sizer)
{
    if (spinctrlM)
    {
        if (captionBeforeM)
        {
            sizer->Add(captionBeforeM, 0,
                wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
            sizer->Add(styleguide().getControlLabelMargin(), 0);
        }
        sizer->Add(spinctrlM, 0, wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
        if (captionAfterM)
        {
            sizer->Add(styleguide().getControlLabelMargin(), 0);
            sizer->Add(captionAfterM, 0,
                wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
        }
    }
}

bool PrefDlgIntEditSetting::createControl(bool WXUNUSED(ignoreerrors))
{
    wxString caption1(captionM);
    wxString caption2;

    int pos = captionM.Find("[VALUE]");
    if (pos >= 0)
    {
        caption1.Truncate(size_t(pos));
        caption1.Trim(true);
        caption2 = captionM.Mid(pos + 7).Trim(false);
    }

    if (!caption1.empty())
        captionBeforeM = new wxStaticText(getPage(), wxID_ANY, caption1);
    spinctrlM = new wxSpinCtrl(getPage(), wxID_ANY);
    spinctrlM->SetRange(minValueM, maxValueM);
    if (!caption2.empty())
        captionAfterM = new wxStaticText(getPage(), wxID_ANY, caption2);

    if (!descriptionM.empty())
    {
        spinctrlM->SetToolTip(descriptionM);
        if (captionBeforeM)
            captionBeforeM->SetToolTip(descriptionM);
        if (captionAfterM)
            captionAfterM->SetToolTip(descriptionM);
    }
    return true;
}

void PrefDlgIntEditSetting::enableControls(bool enabled)
{
    if (spinctrlM)
        spinctrlM->Enable(enabled);
}

wxStaticText* PrefDlgIntEditSetting::getLabel()
{
    return captionBeforeM;
}

bool PrefDlgIntEditSetting::hasControls() const
{
    return captionBeforeM != 0 || spinctrlM != 0 || captionAfterM != 0;
}

bool PrefDlgIntEditSetting::loadFromTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (spinctrlM)
    {
        int value = defaultM;
        config.getValue(keyM, value);
        if (value < minValueM)
            spinctrlM->SetValue(minValueM);
        else if (value > maxValueM)
            spinctrlM->SetValue(maxValueM);
        else
            spinctrlM->SetValue(value);
    }
    return true;
}

bool PrefDlgIntEditSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE)
    {
        wxString name(xmln->GetName());
        if (name == "minvalue" || name == "maxvalue")
        {
            wxString value(getNodeContent(xmln, wxEmptyString));
            long l;
            if (!value.empty() && value.ToLong(&l))
            {
                if (name == "maxvalue")
                    maxValueM = l;
                else
                    minValueM = l;
            }
        }
    }
    return PrefDlgSetting::parseProperty(xmln);
}

bool PrefDlgIntEditSetting::saveToTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (spinctrlM)
        config.setValue(keyM, spinctrlM->GetValue());
    return true;
}

void PrefDlgIntEditSetting::setDefault(const wxString& defValue)
{
    long l;
    if (defValue.ToLong(&l))
        defaultM = l;
}

// PrefDlgStringEditSetting class
class PrefDlgStringEditSetting: public PrefDlgSetting
{
public:
    PrefDlgStringEditSetting(wxPanel* page, PrefDlgSetting* parent);

    virtual bool createControl(bool ignoreerrors);
    virtual bool loadFromTargetConfig(Config& config);
    virtual bool parseProperty(wxXmlNode* xmln);
    virtual bool saveToTargetConfig(Config& config);
protected:
    virtual void addControlsToSizer(wxSizer* sizer);
    virtual void enableControls(bool enabled);
    virtual wxStaticText* getLabel();
    virtual bool hasControls() const;
    virtual void setDefault(const wxString& defValue);
private:
    wxStaticText* captionAfterM;
    wxStaticText* captionBeforeM;
    wxTextCtrl* textCtrlM;
    wxString defaultM;
    int expandM;
    wxString toolTipM;
};

PrefDlgStringEditSetting::PrefDlgStringEditSetting(wxPanel* page,
        PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent), captionAfterM(0), captionBeforeM(0),
        textCtrlM(0), expandM(0)
{
}

void PrefDlgStringEditSetting::addControlsToSizer(wxSizer* sizer)
{
    if (textCtrlM)
    {
        if (captionBeforeM)
        {
            sizer->Add(captionBeforeM, 0,
                wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
            sizer->Add(styleguide().getControlLabelMargin(), 0);
        }
        sizer->Add(textCtrlM, expandM,
            wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
        if (captionAfterM)
        {
            sizer->Add(styleguide().getControlLabelMargin(), 0);
            sizer->Add(captionAfterM, 0,
                wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
        }
    }
}

bool PrefDlgStringEditSetting::createControl(bool WXUNUSED(ignoreerrors))
{
    wxString caption1(captionM);
    wxString caption2;

    int pos = captionM.Find("[VALUE]");
    if (pos >= 0)
    {
        caption1.Truncate(size_t(pos));
        caption1.Trim(true);
        caption2 = captionM.Mid(pos + 7).Trim(false);
    }

    if (!caption1.empty())
        captionBeforeM = new wxStaticText(getPage(), wxID_ANY, caption1);
    textCtrlM = new wxTextCtrl(getPage(), wxID_ANY);
    if (!caption2.empty())
        captionAfterM = new wxStaticText(getPage(), wxID_ANY, caption2);

    if (!descriptionM.empty())
    {
        textCtrlM->SetToolTip(descriptionM);
        if (captionBeforeM)
            captionBeforeM->SetToolTip(descriptionM);
        if (captionAfterM)
            captionAfterM->SetToolTip(descriptionM);
    }
    textCtrlM->SetToolTip(toolTipM);

    return true;
}

void PrefDlgStringEditSetting::enableControls(bool enabled)
{
    if (textCtrlM)
        textCtrlM->Enable(enabled);
}

wxStaticText* PrefDlgStringEditSetting::getLabel()
{
    return captionBeforeM;
}

bool PrefDlgStringEditSetting::hasControls() const
{
    return (captionBeforeM) || (textCtrlM) || (captionAfterM);
}

bool PrefDlgStringEditSetting::loadFromTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (textCtrlM)
    {
        wxString value = defaultM;
        config.getValue(keyM, value);
        textCtrlM->SetValue(value);
    }
    return true;
}

bool PrefDlgStringEditSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE)
    {
        wxString name(xmln->GetName());
        if (name == "expand")
        {
            wxString value(getNodeContent(xmln, wxEmptyString));
            long l;
            if (!value.empty() && value.ToLong(&l))
                expandM = l;
        }
        else if (name == "hint")
            toolTipM = getNodeContent(xmln, wxEmptyString);
    }
    return PrefDlgSetting::parseProperty(xmln);
}

bool PrefDlgStringEditSetting::saveToTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (textCtrlM)
        config.setValue(keyM, textCtrlM->GetValue());
    return true;
}

void PrefDlgStringEditSetting::setDefault(const wxString& defValue)
{
    defaultM = defValue;
}

// PrefDlgChooserSetting class
class PrefDlgChooserSetting: public PrefDlgSetting
{
protected:
    PrefDlgChooserSetting(wxPanel* page, PrefDlgSetting* parent);
public:
    ~PrefDlgChooserSetting();

    virtual bool createControl(bool ignoreerrors);
    virtual bool loadFromTargetConfig(Config& config);
    virtual bool saveToTargetConfig(Config& config);
protected:
    wxTextCtrl* textCtrlM;
    wxButton* browsebtnM;

    virtual void addControlsToSizer(wxSizer* sizer);
    virtual void enableControls(bool enabled);
    virtual wxStaticText* getLabel();
    virtual bool hasControls() const;
    virtual void setDefault(const wxString& defValue);
private:
    wxStaticText* captionBeforeM;
    wxString defaultM;

    virtual void choose() = 0;

    std::unique_ptr<wxEvtHandler> buttonHandlerM;
    void OnBrowseButton(wxCommandEvent& event);
};

PrefDlgChooserSetting::PrefDlgChooserSetting(wxPanel* page,
        PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent), browsebtnM(0), captionBeforeM(0),
        textCtrlM(0)
{
}

PrefDlgChooserSetting::~PrefDlgChooserSetting()
{
    if (browsebtnM && buttonHandlerM.get())
        browsebtnM->PopEventHandler();
}

void PrefDlgChooserSetting::addControlsToSizer(wxSizer* sizer)
{
    if (textCtrlM)
    {
        if (captionBeforeM)
        {
            sizer->Add(captionBeforeM, 0,
                wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
            sizer->Add(styleguide().getControlLabelMargin(), 0);
        }
        sizer->Add(textCtrlM, 1, wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
        if (browsebtnM)
        {
            sizer->Add(styleguide().getBrowseButtonMargin(), 0);
            sizer->Add(browsebtnM, 0,
                wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
        }
    }
}

bool PrefDlgChooserSetting::createControl(bool WXUNUSED(ignoreerrors))
{
    if (!captionM.empty())
        captionBeforeM = new wxStaticText(getPage(), wxID_ANY, captionM);
    textCtrlM = new wxTextCtrl(getPage(), wxID_ANY);
    browsebtnM = new wxButton(getPage(), wxID_ANY, _("Select..."));

    buttonHandlerM.reset(new PrefDlgEventHandler(
        std::bind(&PrefDlgChooserSetting::OnBrowseButton, this,  std::placeholders::_1)));
    browsebtnM->PushEventHandler(buttonHandlerM.get());
    buttonHandlerM->Connect(browsebtnM->GetId(),
        wxEVT_COMMAND_BUTTON_CLICKED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    if (!descriptionM.empty())
    {
        if (captionBeforeM)
            captionBeforeM->SetToolTip(descriptionM);
        textCtrlM->SetToolTip(descriptionM);
        if (browsebtnM)
            browsebtnM->SetToolTip(descriptionM);
    }
    return true;
}

void PrefDlgChooserSetting::enableControls(bool enabled)
{
    if (textCtrlM)
        textCtrlM->Enable(enabled);
    if (browsebtnM)
        browsebtnM->Enable(enabled);
}

wxStaticText* PrefDlgChooserSetting::getLabel()
{
    return captionBeforeM;
}

bool PrefDlgChooserSetting::hasControls() const
{
    return (captionBeforeM) || (textCtrlM) ||(browsebtnM);
}

bool PrefDlgChooserSetting::loadFromTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (textCtrlM)
    {
        wxString value = defaultM;
        config.getValue(keyM, value);
        textCtrlM->SetValue(value);
    }
    enableControls(true);
    return true;
}

bool PrefDlgChooserSetting::saveToTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (textCtrlM)
        config.setValue(keyM, textCtrlM->GetValue());
    return true;
}

void PrefDlgChooserSetting::setDefault(const wxString& defValue)
{
    defaultM = defValue;
}

void PrefDlgChooserSetting::OnBrowseButton(wxCommandEvent& WXUNUSED(event))
{
    if (textCtrlM)
        choose();
}

// PrefDlgFileChooserSetting class
class PrefDlgFileChooserSetting: public PrefDlgChooserSetting
{
public:
    PrefDlgFileChooserSetting(wxPanel* page, PrefDlgSetting* parent);
protected:
    virtual bool parseProperty(wxXmlNode* xmln);
private:
    wxString dlgFilterM;
    virtual void choose();
};

PrefDlgFileChooserSetting::PrefDlgFileChooserSetting(wxPanel* page,
        PrefDlgSetting* parent)
    : PrefDlgChooserSetting(page, parent)
{
    dlgFilterM = _("All files (*.*)|*.*");
}

void PrefDlgFileChooserSetting::choose()
{
    wxString path;
    wxFileName::SplitPath(textCtrlM->GetValue(), &path, 0, 0);

    wxString filename = ::wxFileSelector(_("Select File"), path,
        wxEmptyString, wxEmptyString, dlgFilterM, wxFD_SAVE,
        ::wxGetTopLevelParent(textCtrlM));
    if (!filename.empty())
        textCtrlM->SetValue(filename);
}

bool PrefDlgFileChooserSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE
        && xmln->GetName() == "dlg_filter")
    {
        dlgFilterM = getNodeContent(xmln, wxEmptyString);
    }
    return PrefDlgChooserSetting::parseProperty(xmln);
}

// PrefDlgFontChooserSetting class
class PrefDlgFontChooserSetting: public PrefDlgChooserSetting
{
public:
    PrefDlgFontChooserSetting(wxPanel* page, PrefDlgSetting* parent);
private:
    virtual void choose();
};

PrefDlgFontChooserSetting::PrefDlgFontChooserSetting(wxPanel* page,
        PrefDlgSetting* parent)
    : PrefDlgChooserSetting(page, parent)
{
}

void PrefDlgFontChooserSetting::choose()
{
    wxFont font;
    wxString fontdesc = textCtrlM->GetValue();
    if (!fontdesc.empty())
        font.SetNativeFontInfo(fontdesc);
    wxFont font2 = ::wxGetFontFromUser(::wxGetTopLevelParent(textCtrlM), font);
    if (font2.Ok())
        textCtrlM->SetValue(font2.GetNativeFontInfoDesc());
}

// PrefDlgRelationColumnsChooserSetting class
class PrefDlgRelationColumnsChooserSetting: public PrefDlgChooserSetting
{
public:
    PrefDlgRelationColumnsChooserSetting(wxPanel* page,
        PrefDlgSetting* parent);
protected:
    virtual void enableControls(bool enabled);
    virtual bool parseProperty(wxXmlNode* xmln);
private:
    Relation* relationM;
    virtual void choose();
};

PrefDlgRelationColumnsChooserSetting::PrefDlgRelationColumnsChooserSetting(
        wxPanel* page, PrefDlgSetting* parent)
    : PrefDlgChooserSetting(page, parent), relationM(0)
{
}

void PrefDlgRelationColumnsChooserSetting::choose()
{
    wxArrayString defaultNames = ::wxStringTokenize(textCtrlM->GetValue(),
        ",");
    std::vector<wxString> list;
    for (size_t i = 0; i < defaultNames.Count(); i++)
        list.push_back(defaultNames[i].Trim(true).Trim(false));

    if (selectRelationColumnsIntoVector(relationM, getPage(), list))
    {
        std::vector<wxString>::iterator it = list.begin();
        wxString retval(*it);
        while ((++it) != list.end())
            retval += ", " + (*it);
        textCtrlM->SetValue(retval);
    }
}

void PrefDlgRelationColumnsChooserSetting::enableControls(bool enabled)
{
    if (textCtrlM)
        textCtrlM->Enable(enabled);
    if (browsebtnM)
        browsebtnM->Enable(enabled && relationM);
}

bool PrefDlgRelationColumnsChooserSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE
        && xmln->GetName() == "relation")
    {
        wxString handle(getNodeContent(xmln, wxEmptyString));
        MetadataItem* mi = MetadataItem::getObjectFromHandle(handle);
        relationM = dynamic_cast<Relation*>(mi);
    }
    return PrefDlgChooserSetting::parseProperty(xmln);
}

// PrefDlgCheckListBoxSetting class
class PrefDlgCheckListBoxSetting: public PrefDlgSetting
{
public:
    PrefDlgCheckListBoxSetting(wxPanel* page, PrefDlgSetting* parent);
    ~PrefDlgCheckListBoxSetting();

    virtual bool createControl(bool ignoreerrors);
    virtual bool loadFromTargetConfig(Config& config);
    virtual bool saveToTargetConfig(Config& config);
protected:
    wxString defaultM;
    virtual void addControlsToSizer(wxSizer* sizer);
    virtual void enableControls(bool enabled);
    virtual wxArrayString getCheckListBoxItems();
    void getItemsCheckState(bool& uncheckedItems, bool& checkedItems);
    virtual wxStaticText* getLabel();
    virtual bool hasControls() const;
    virtual bool parseProperty(wxXmlNode* xmln);
    virtual void setDefault(const wxString& defValue);
    void updateCheckBoxState();
private:
    wxStaticText* captionBeforeM;
    wxCheckListBox* checkListBoxM;
    wxCheckBox* checkBoxM;
    bool ignoreEventsM;
    wxArrayString itemsM;

    std::unique_ptr<wxEvtHandler> checkListBoxHandlerM;
    std::unique_ptr<wxEvtHandler> checkBoxHandlerM;
    void OnCheckListBox(wxCommandEvent& event);
    void OnCheckBox(wxCommandEvent& event);
};

PrefDlgCheckListBoxSetting::PrefDlgCheckListBoxSetting(wxPanel* page,
        PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent), captionBeforeM(0), checkListBoxM(0),
        checkBoxM(0), ignoreEventsM(true)
{
}

PrefDlgCheckListBoxSetting::~PrefDlgCheckListBoxSetting()
{
    if (checkListBoxM && checkListBoxHandlerM.get())
        checkListBoxM->PopEventHandler();
    if (checkBoxM && checkBoxHandlerM.get())
        checkBoxM->PopEventHandler();
}

void PrefDlgCheckListBoxSetting::addControlsToSizer(wxSizer* sizer)
{
    if (checkListBoxM)
    {
        if (captionBeforeM)
        {
            sizer->Add(captionBeforeM, 0, wxFIXED_MINSIZE | wxALIGN_TOP);
            sizer->Add(styleguide().getControlLabelMargin(), 0);
        }

        wxSizer* sizerVert = new wxBoxSizer(wxVERTICAL);
        sizerVert->Add(checkListBoxM, 1, wxEXPAND);
        sizerVert->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));
        sizerVert->Add(checkBoxM, 0, wxFIXED_MINSIZE | wxALIGN_LEFT);

        sizer->Add(sizerVert, 1, wxEXPAND | wxFIXED_MINSIZE | wxALIGN_TOP);
    }
}

bool PrefDlgCheckListBoxSetting::createControl(bool WXUNUSED(ignoreerrors))
{
    if (!captionM.empty())
        captionBeforeM = new wxStaticText(getPage(), wxID_ANY, captionM);
    checkListBoxM = new wxCheckListBox(getPage(), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, getCheckListBoxItems());
    // height needs to be at least 100 on Linux
    checkListBoxM->SetMinSize(wxSize(150, 100));
    checkBoxM = new wxCheckBox(getPage(), wxID_ANY,
        _("Select / deselect &all"), wxDefaultPosition, wxDefaultSize,
        wxCHK_3STATE);

    checkListBoxHandlerM.reset(new PrefDlgEventHandler(
        std::bind(&PrefDlgCheckListBoxSetting::OnCheckListBox, this,  std::placeholders::_1)));
    checkListBoxM->PushEventHandler(checkListBoxHandlerM.get());
    checkListBoxHandlerM->Connect(checkListBoxM->GetId(),
        wxEVT_COMMAND_CHECKLISTBOX_TOGGLED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    checkBoxHandlerM.reset(new PrefDlgEventHandler(
        std::bind(&PrefDlgCheckListBoxSetting::OnCheckBox, this,  std::placeholders::_1)));
    checkBoxM->PushEventHandler(checkBoxHandlerM.get());
    checkBoxHandlerM->Connect(checkBoxM->GetId(),
        wxEVT_COMMAND_CHECKBOX_CLICKED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));
    ignoreEventsM = false;

    if (!descriptionM.empty())
    {
        if (captionBeforeM)
            captionBeforeM->SetToolTip(descriptionM);
        checkListBoxM->SetToolTip(descriptionM);
    }
    return true;
}

void PrefDlgCheckListBoxSetting::enableControls(bool enabled)
{
    if (checkListBoxM)
        checkListBoxM->Enable(enabled);
    if (checkBoxM)
        checkBoxM->Enable(enabled);
}

bool PrefDlgCheckListBoxSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE
        && xmln->GetName() == "option")
    {
        wxString optcaption;
        for (wxXmlNode* xmlc = xmln->GetChildren(); xmlc != 0;
            xmlc = xmlc->GetNext())
        {
            if (xmlc->GetType() != wxXML_ELEMENT_NODE)
                continue;
            wxString value(getNodeContent(xmlc, wxEmptyString));
            if (xmlc->GetName() == "caption")
                optcaption = value;
/*
            else if (xmlc->GetName() == "value"))
                optvalue = value;
*/
        }
        // for the time being values are the index of the caption in the array
        if (!optcaption.empty())
            itemsM.Add(optcaption);
    }
    return PrefDlgSetting::parseProperty(xmln);
}

wxArrayString PrefDlgCheckListBoxSetting::getCheckListBoxItems()
{
    return itemsM;
}

void PrefDlgCheckListBoxSetting::getItemsCheckState(bool& uncheckedItems,
    bool& checkedItems)
{
    uncheckedItems = false;
    checkedItems = false;
    for (size_t i = 0; i < checkListBoxM->GetCount(); i++)
    {
        if (checkListBoxM->IsChecked(i))
            checkedItems = true;
        else
            uncheckedItems = true;
        if (checkedItems && uncheckedItems)
            return;
    }
}

wxStaticText* PrefDlgCheckListBoxSetting::getLabel()
{
    return captionBeforeM;
}

bool PrefDlgCheckListBoxSetting::hasControls() const
{
    return captionBeforeM != 0 || checkListBoxM != 0 || checkBoxM != 0;
}

bool PrefDlgCheckListBoxSetting::loadFromTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (checkListBoxM)
    {
        wxString value = defaultM;
        config.getValue(keyM, value);
        wxArrayString checked(::wxStringTokenize(value, ", \t\r\n",
            wxTOKEN_STRTOK));

        ignoreEventsM = true;
        for (size_t i = 0; i < checked.GetCount(); i++)
        {
            int idx = checkListBoxM->FindString(checked[i]);
            if (idx >= 0)
                checkListBoxM->Check(idx);
        }
        updateCheckBoxState();
        ignoreEventsM = false;
    }
    enableControls(true);
    return true;
}

bool PrefDlgCheckListBoxSetting::saveToTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    size_t itemCount = checkListBoxM ? checkListBoxM->GetCount() : 0;
    if (itemCount)
    {
        wxString value;
        bool first = true;
        for (size_t i = 0; i < itemCount; i++)
        {
            if (checkListBoxM->IsChecked(i))
            {
                if (!first)
                    value += wxString(", ");
                first = false;
                value += checkListBoxM->GetString(i);
            }
        }
        config.setValue(keyM, value);
    }
    return true;
}

void PrefDlgCheckListBoxSetting::setDefault(const wxString& defValue)
{
    defaultM = defValue;
}

void PrefDlgCheckListBoxSetting::updateCheckBoxState()
{
    bool uncheckedItems = false, checkedItems = false;
    getItemsCheckState(uncheckedItems, checkedItems);

    if (checkedItems && !uncheckedItems)
        checkBoxM->Set3StateValue(wxCHK_CHECKED);
    else if (!checkedItems && uncheckedItems)
        checkBoxM->Set3StateValue(wxCHK_UNCHECKED);
    else
        checkBoxM->Set3StateValue(wxCHK_UNDETERMINED);
}

// event handler
void PrefDlgCheckListBoxSetting::OnCheckListBox(
    wxCommandEvent& WXUNUSED(event))
{
    if (!ignoreEventsM && checkBoxM)
        updateCheckBoxState();
}

void PrefDlgCheckListBoxSetting::OnCheckBox(wxCommandEvent& event)
{
    if (!ignoreEventsM && checkListBoxM && checkListBoxM->GetCount())
    {
        ignoreEventsM = true;
        for (size_t i = 0; i < checkListBoxM->GetCount(); i++)
            checkListBoxM->Check(i, event.IsChecked());
        ignoreEventsM = false;
    }
}

// PrefDlgRelationColumnsListSetting class
class PrefDlgRelationColumnsListSetting: public PrefDlgCheckListBoxSetting
{
public:
    PrefDlgRelationColumnsListSetting(wxPanel* page, PrefDlgSetting* parent);
protected:
    virtual wxArrayString getCheckListBoxItems();
    virtual bool parseProperty(wxXmlNode* xmln);
private:
    Relation* relationM;
};

PrefDlgRelationColumnsListSetting::PrefDlgRelationColumnsListSetting(
        wxPanel* page, PrefDlgSetting* parent)
    : PrefDlgCheckListBoxSetting(page, parent), relationM(0)
{
}

wxArrayString PrefDlgRelationColumnsListSetting::getCheckListBoxItems()
{
    wxArrayString columns;
    if (relationM)
    {
        relationM->ensureChildrenLoaded();
        columns.Alloc(relationM->getColumnCount());
        for (ColumnPtrs::const_iterator it = relationM->begin();
            it != relationM->end(); it++)
        {
            columns.Add((*it)->getName_());
        }
    }
    return columns;
}

bool PrefDlgRelationColumnsListSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE
        && xmln->GetName() == "relation")
    {
        wxString handle(getNodeContent(xmln, wxEmptyString));
        MetadataItem* mi = MetadataItem::getObjectFromHandle(handle);
        relationM = dynamic_cast<Relation*>(mi);
    }
    return PrefDlgCheckListBoxSetting::parseProperty(xmln);
}


class PrefDlgComboBoxSetting : public PrefDlgSetting
{
public:
    PrefDlgComboBoxSetting(wxPanel* page, PrefDlgSetting* parent);
    ~PrefDlgComboBoxSetting();

    virtual bool createControl(bool ignoreerrors);
    virtual bool loadFromTargetConfig(Config& config);
    virtual bool parseProperty(wxXmlNode* xmln);
    virtual bool saveToTargetConfig(Config& config);
protected:
    wxString defaultM;
    wxArrayString itemsM;

    virtual void addControlsToSizer(wxSizer* sizer);
    virtual void enableControls(bool enabled);
    virtual wxStaticText* getLabel();
    virtual wxArrayString getComboBoxItems();
    virtual bool hasControls() const;
    virtual void setDefault(const wxString& defValue);
private:
    void OnComboBoxClick(wxCommandEvent& e) {
        comboBoxM->SetSelection(static_cast<wxComboBox*>(e.GetEventObject())->GetSelection());
    }
    wxStaticText* captionBeforeM;
    wxComboBox* comboBoxM;

    std::unique_ptr<wxEvtHandler> comboBoxHandlerM;

};

PrefDlgComboBoxSetting::PrefDlgComboBoxSetting(
    wxPanel* page, PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent), comboBoxM(0), captionBeforeM(0)
{
}

PrefDlgComboBoxSetting::~PrefDlgComboBoxSetting()
{
    /*
        if (comboBoxM && comboBoxHandlerM.get())
        comboBoxM->PopEventHandler();

    */
}

bool PrefDlgComboBoxSetting::createControl(bool WXUNUSED(ignoreerrors))
{

    if (!captionM.empty())
        captionBeforeM = new wxStaticText(getPage(), wxID_ANY, captionM);

    comboBoxM = new wxComboBox(getPage(), wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, getComboBoxItems());

    if (!descriptionM.empty())
    {
        if (captionBeforeM)
            captionBeforeM->SetToolTip(descriptionM);
        comboBoxM->SetToolTip(descriptionM);
    }

    return true;
}

bool PrefDlgComboBoxSetting::loadFromTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;

    if (comboBoxM)
    {
        wxString value = defaultM;
        config.getValue(keyM, value);
        comboBoxM->SetValue(value);
    }

    enableControls(true);
    return true;
}

bool PrefDlgComboBoxSetting::parseProperty(wxXmlNode* xmln)
{

    if (xmln->GetType() == wxXML_ELEMENT_NODE
        && xmln->GetName() == "option")
    {
        wxString optcaption;
        for (wxXmlNode* xmlc = xmln->GetChildren(); xmlc != 0;
            xmlc = xmlc->GetNext())
        {
            if (xmlc->GetType() != wxXML_ELEMENT_NODE)
                continue;
            wxString value(getNodeContent(xmlc, wxEmptyString));
            if (xmlc->GetName() == "caption")
                optcaption = value;
        }
        // for the time being values are the index of the caption in the array
        if (!optcaption.empty())
            itemsM.Add(optcaption);
    }

    return PrefDlgSetting::parseProperty(xmln);
}

bool PrefDlgComboBoxSetting::saveToTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    
    wxString value = comboBoxM->GetStringSelection();

    if (!value.IsEmpty()) {
        config.setValue(keyM, value);
    }

    return true;
}

void PrefDlgComboBoxSetting::addControlsToSizer(wxSizer* sizer)
{
    if (comboBoxM)
    {
        if (captionBeforeM)
        {
            sizer->Add(captionBeforeM, 0, wxFIXED_MINSIZE | wxALIGN_TOP);
            sizer->Add(styleguide().getControlLabelMargin(), 0);
        }

        wxSizer* sizerVert = new wxBoxSizer(wxVERTICAL);
        sizerVert->Add(comboBoxM, 1, wxEXPAND);
        sizerVert->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));

        sizer->Add(sizerVert, 1, wxEXPAND | wxFIXED_MINSIZE | wxALIGN_TOP);
    }

}

void PrefDlgComboBoxSetting::enableControls(bool enabled)
{
   if (comboBoxM)
        comboBoxM->Enable(enabled);
}

wxStaticText* PrefDlgComboBoxSetting::getLabel()
{
    return captionBeforeM;
}

wxArrayString PrefDlgComboBoxSetting::getComboBoxItems()
{
    return itemsM;
}

bool PrefDlgComboBoxSetting::hasControls() const
{
    return captionBeforeM != 0 || comboBoxM != 0;
}

void PrefDlgComboBoxSetting::setDefault(const wxString& defValue)
{
    defaultM = defValue;
}



class PrefDlgThemeComboBoxSetting : public PrefDlgComboBoxSetting
{
public:
    PrefDlgThemeComboBoxSetting(wxPanel* page, PrefDlgSetting* parent);

protected:
    virtual bool parseProperty(wxXmlNode* xmln);
    virtual bool saveToTargetConfig(Config& config);

private:
};

PrefDlgThemeComboBoxSetting::PrefDlgThemeComboBoxSetting(wxPanel* page, PrefDlgSetting* parent)
    :PrefDlgComboBoxSetting(page, parent)
{
}

bool PrefDlgThemeComboBoxSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE
        && xmln->GetName() == "file")
    {
        wxString dirName = config().getXmlStylesPath();
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
    }

    return PrefDlgComboBoxSetting::parseProperty(xmln);
}

bool PrefDlgThemeComboBoxSetting::saveToTargetConfig(Config& config)
{
    if (!PrefDlgComboBoxSetting::saveToTargetConfig(config))
        return false;

    stylerManager().loadConfig();

    return true;
}


class PrefDlgColourPickerSetting : public PrefDlgSetting
{
public:
    PrefDlgColourPickerSetting(wxPanel* page, PrefDlgSetting* parent);
    ~PrefDlgColourPickerSetting();

    virtual bool createControl(bool ignoreerrors);
    virtual bool loadFromTargetConfig(Config& config);
    virtual bool parseProperty(wxXmlNode* xmln);
    virtual bool saveToTargetConfig(Config& config);
protected:
    wxString defaultM;

    virtual void addControlsToSizer(wxSizer* sizer);
    virtual void enableControls(bool enabled);
    virtual wxStaticText* getLabel();
    virtual bool hasControls() const;
    virtual void setDefault(const wxString& defValue);
private:
    wxStaticText* captionBeforeM;
    wxColourPickerCtrl* colourPickerM;
};

PrefDlgColourPickerSetting::PrefDlgColourPickerSetting(
    wxPanel* page, PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent), colourPickerM(0), captionBeforeM(0)
{
}

PrefDlgColourPickerSetting::~PrefDlgColourPickerSetting()
{
}

bool PrefDlgColourPickerSetting::createControl(bool ignoreerrors)
{
    if (!captionM.empty())
        captionBeforeM = new wxStaticText(getPage(), wxID_ANY, captionM);

    //wxSize size = wxSize(10, 10);

    colourPickerM = new wxColourPickerCtrl(getPage(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize);

    if (!descriptionM.empty())
    {
        if (captionBeforeM)
            captionBeforeM->SetToolTip(descriptionM);
        colourPickerM->SetToolTip(descriptionM);
    }

    return true;
}

bool PrefDlgColourPickerSetting::loadFromTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;

    if (colourPickerM)
    {
        wxString value = defaultM;
        config.getValue(keyM, value);

        long result;
        value.ToLong(&result, 16);

        colourPickerM->SetColour(wxColour((_RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000)));
    }

    enableControls(true);
    return true;
}

bool PrefDlgColourPickerSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE
        && xmln->GetName() == "option")
    {
        /*wxString optcaption;
        for (wxXmlNode* xmlc = xmln->GetChildren(); xmlc != 0;
            xmlc = xmlc->GetNext())
        {
            if (xmlc->GetType() != wxXML_ELEMENT_NODE)
                continue;
            wxString value(getNodeContent(xmlc, wxEmptyString));
            if (xmlc->GetName() == "caption")
                optcaption = value;
        }
        // for the time being values are the index of the caption in the array
        if (!optcaption.empty())
            itemsM.Add(optcaption);
        */
    }

    return PrefDlgSetting::parseProperty(xmln);
}

bool PrefDlgColourPickerSetting::saveToTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;

    wxColour value = colourPickerM->GetColour();

    //if (!value.IsEmpty()) {
        config.setValue(keyM, value.GetAsString(wxC2S_CSS_SYNTAX));
   // }

    return true;
}

void PrefDlgColourPickerSetting::addControlsToSizer(wxSizer* sizer)
{
    if (colourPickerM)
    {
        if (captionBeforeM)
        {
            sizer->Add(captionBeforeM, 0, wxFIXED_MINSIZE | wxALIGN_LEFT);
            sizer->Add(styleguide().getControlLabelMargin(), 0);
        }

        wxSizer* sizerVert = new wxBoxSizer(wxVERTICAL);
        sizerVert->Add(colourPickerM, 1, wxEXPAND);
        sizerVert->Add(0, styleguide().getRelatedControlMargin(wxVERTICAL));

        sizer->Add(sizerVert, 1, wxEXPAND | wxFIXED_MINSIZE | wxALIGN_LEFT);
    }

}

void PrefDlgColourPickerSetting::enableControls(bool enabled)
{
    if (colourPickerM)
        colourPickerM->Enable(enabled);
}

wxStaticText* PrefDlgColourPickerSetting::getLabel()
{
    return captionBeforeM;
}

bool PrefDlgColourPickerSetting::hasControls() const
{
    return captionBeforeM != 0 || colourPickerM != 0;
}

void PrefDlgColourPickerSetting::setDefault(const wxString& defValue)
{
    defaultM = defValue;
}

class PrefDlgThemeSetting : public PrefDlgSetting
{
public:
    PrefDlgThemeSetting(wxPanel* page, PrefDlgSetting* parent);
    ~PrefDlgThemeSetting();

    virtual bool createControl(bool ignoreerrors);
    virtual bool loadFromTargetConfig(Config& config);
    virtual bool parseProperty(wxXmlNode* xmln);
    virtual bool saveToTargetConfig(Config& config);
    FRStyleManager* getStyleManager() { return styleManagerM; };
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

    void OnChangeStyle(wxCommandEvent& event);

};

PrefDlgThemeSetting::PrefDlgThemeSetting(wxPanel* page, PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent), styleManagerM(0), fileComboBoxM(0), captionBeforeM(0),
    stylersListBoxM(0), styleListBoxM(0), fontNameComboBoxM(0), fontSizeComboBoxM(0)
{
    const wxString STYLE = "StyleTheme";
    const wxString def = "stylers";


    styleManagerM = new FRStyleManager(wxFileName(config().getXmlStylesPath(), config().get(STYLE, def) + ".xml"));
}

PrefDlgThemeSetting::~PrefDlgThemeSetting()
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

    if (fontNameComboBoxM   && fontNameComboBoxHandlerM.get())
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

}

bool PrefDlgThemeSetting::createControl(bool WXUNUSED(ignoreerrors))
{
    captionBeforeM = new wxStaticText(getPage(), wxID_ANY, "Select theme:");

    fileComboBoxM = new wxComboBox(getPage(), wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxDefaultSize, getComboBoxItems());
    fileComboBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgThemeSetting::OnSelectFileComboBox, this, std::placeholders::_1)));
    fileComboBoxM->PushEventHandler(fileComboBoxHandlerM.get());
    fileComboBoxHandlerM->Connect(fileComboBoxM->GetId(),
        wxEVT_COMMAND_COMBOBOX_SELECTED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));


    stylersListBoxM = new wxListBox(getPage(), wxID_ANY, wxDefaultPosition, wxDefaultSize);
    stylersListBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgThemeSetting::OnSelectStylersListBox, this, std::placeholders::_1)));
    stylersListBoxM->PushEventHandler(stylersListBoxHandlerM.get());
    stylersListBoxHandlerM->Connect(stylersListBoxM->GetId(),
        wxEVT_COMMAND_LISTBOX_SELECTED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    styleListBoxM = new wxListBox(getPage(), wxID_ANY, wxDefaultPosition, wxDefaultSize);
    styleListBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgThemeSetting::OnSelectStyleListBox, this, std::placeholders::_1)));
    styleListBoxM->PushEventHandler(styleListBoxHandlerM.get());
    styleListBoxHandlerM->Connect(styleListBoxM->GetId(),
        wxEVT_COMMAND_LISTBOX_SELECTED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    foregroundPickerM = new wxColourPickerCtrl(getPage(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize);
    foregroundPickerHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgThemeSetting::OnChangeStyle, this, std::placeholders::_1)));
    foregroundPickerM->PushEventHandler(foregroundPickerHandlerM.get());
    foregroundPickerHandlerM->Connect(foregroundPickerM->GetId(),
        wxEVT_COMMAND_COLOURPICKER_CHANGED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    backgroundPickerM = new wxColourPickerCtrl(getPage(), wxID_ANY, *wxBLACK, wxDefaultPosition, wxDefaultSize);
    backgroundPickerHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgThemeSetting::OnChangeStyle, this, std::placeholders::_1)));
    backgroundPickerM->PushEventHandler(backgroundPickerHandlerM.get());
    backgroundPickerHandlerM->Connect(backgroundPickerM->GetId(),
        wxEVT_COMMAND_COLOURPICKER_CHANGED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    
    wxArrayString strArray = wxFontEnumerator::GetFacenames();
    strArray.Sort();
    fontNameComboBoxM = new wxComboBox(getPage(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, strArray);    
    fontNameComboBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgThemeSetting::OnChangeStyle, this, std::placeholders::_1)));
    fontNameComboBoxM->PushEventHandler(fontNameComboBoxHandlerM.get());
    fontNameComboBoxHandlerM->Connect(fontNameComboBoxM->GetId(),
        wxEVT_COMMAND_COMBOBOX_SELECTED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));
    
    //wxArrayString strArray;
    strArray.Clear();
    for (int i = 5; i <= 30; i++) {
        if (i <= 12 || i % 2 == 0) {
            strArray.Add(wxString::Format("%i", i));
        }
    }
    fontSizeComboBoxM = new wxComboBox(getPage(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, strArray);
    fontSizeComboBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgThemeSetting::OnChangeStyle, this, std::placeholders::_1)));
    fontSizeComboBoxM->PushEventHandler(fontSizeComboBoxHandlerM.get());
    fontSizeComboBoxHandlerM->Connect(fontSizeComboBoxM->GetId(),
        wxEVT_COMMAND_COMBOBOX_SELECTED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));


    blodCheckBoxM = new wxCheckBox(getPage(), wxID_ANY, "Bold", wxDefaultPosition, wxDefaultSize);
    blodCheckBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgThemeSetting::OnChangeStyle, this, std::placeholders::_1)));
    blodCheckBoxM->PushEventHandler(blodCheckBoxHandlerM.get());
    blodCheckBoxHandlerM->Connect(blodCheckBoxM->GetId(),
        wxEVT_COMMAND_CHECKBOX_CLICKED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    italicCheckBoxM = new wxCheckBox(getPage(), wxID_ANY, "Italic", wxDefaultPosition, wxDefaultSize);
    italicCheckBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgThemeSetting::OnChangeStyle, this, std::placeholders::_1)));
    italicCheckBoxM->PushEventHandler(italicCheckBoxHandlerM.get());
    italicCheckBoxHandlerM->Connect(italicCheckBoxM->GetId(),
        wxEVT_COMMAND_CHECKBOX_CLICKED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    underlineCheckBoxM = new wxCheckBox(getPage(), wxID_ANY, "Underline", wxDefaultPosition, wxDefaultSize);
    underlineCheckBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgThemeSetting::OnChangeStyle, this, std::placeholders::_1)));
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
    caseRadioBoxHandlerM.reset(new PrefDlgEventHandler(std::bind(&PrefDlgThemeSetting::OnChangeStyle, this, std::placeholders::_1)));
    caseRadioBoxM->PushEventHandler(caseRadioBoxHandlerM.get());
    caseRadioBoxHandlerM->Connect(caseRadioBoxM->GetId(),
        wxEVT_COMMAND_RADIOBOX_SELECTED,
        wxCommandEventHandler(PrefDlgEventHandler::OnCommandEvent));

    return true;
}

bool PrefDlgThemeSetting::loadFromTargetConfig(Config& config)
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

    enableControls(true);

    return true;
}

bool PrefDlgThemeSetting::parseProperty(wxXmlNode* xmln)
{
    /*if (xmln->GetType() == wxXML_ELEMENT_NODE
        && xmln->GetName() == "file")
    {
        wxString dirName = config().getXmlStylesPath();
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


    }*/

    return PrefDlgSetting::parseProperty(xmln);
}

bool PrefDlgThemeSetting::saveToTargetConfig(Config& config)
{
    config.setValue(keyM, fileComboBoxM->GetValue());
    getStyleManager()->saveStyle();

    return true;
}

void PrefDlgThemeSetting::addControlsToSizer(wxSizer* sizer)
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
    sizer->Add(sizerV, 1, wxEXPAND);
    sizer->SetItemMinSize(sizerV, -1, -1);
}

void PrefDlgThemeSetting::enableControls(bool enabled)
{
}

bool PrefDlgThemeSetting::hasControls() const
{
    return true;
}

wxArrayString PrefDlgThemeSetting::getComboBoxItems()
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

void PrefDlgThemeSetting::loadStylers(const wxString& styleFileName)
{
    
    getStyleManager()->setfileName(wxFileName(config().getXmlStylesPath(), styleFileName.IsEmpty() ? "stylers.xml" : styleFileName + ".xml"));
    getStyleManager()->loadStyle();

    stylersListBoxM->Clear();
    stylersListBoxM->Insert("Global Styles", 0);

    FRStylers stylers = getStyleManager()->getLexerStylers();
    
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

void PrefDlgThemeSetting::loadStyles(const wxString& language)
{
    FRStyles* styles;
    styleListBoxM->Clear();

    if (language == "Global Styles") {
        styles = getStyleManager()->getGlobalStyler();
    }else{
        styles = getStyleManager()->getLexerStylers().getStylerByDesc(language);
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

void PrefDlgThemeSetting::loadStyle(const wxString& styleName)
{
    
    FRStyle* style = getStyleManager()->getStylerByDesc(stylersListBoxM->GetString(stylersListBoxM->GetSelection()))->getStyleByName(styleName);

    fontNameComboBoxM->SetSelection(fontNameComboBoxM->FindString(style->getFontName()));
    fontSizeComboBoxM->SetSelection(fontSizeComboBoxM->FindString(wxString::Format("%i", style->getFontSize())));

    foregroundPickerM->SetColour(style->getfgColor());
    backgroundPickerM->SetColour(style->getbgColor());

    blodCheckBoxM->SetValue(style->getFontStyle() & FONTSTYLE_BOLD);
    italicCheckBoxM->SetValue(style->getFontStyle() & FONTSTYLE_ITALIC);
    underlineCheckBoxM->SetValue(style->getFontStyle() & FONTSTYLE_UNDERLINE);

    caseRadioBoxM->SetSelection(style->getCaseVisible());

}

void PrefDlgThemeSetting::saveStyle(const wxString& styleName)
{
    FRStyle* style = getStyleManager()->getStylerByDesc(stylersListBoxM->GetString(stylersListBoxM->GetSelection()))->getStyleByName(styleName);

    style->setFontName(fontNameComboBoxM->GetString(fontNameComboBoxM->GetSelection()));
    style->setFontSize(atoi(fontSizeComboBoxM->GetString(fontSizeComboBoxM->GetSelection())));

    
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


}

void PrefDlgThemeSetting::OnSelectFileComboBox(wxCommandEvent& event)
{
    loadStylers(fileComboBoxM->GetValue());
}

void PrefDlgThemeSetting::OnSelectStylersListBox(wxCommandEvent& event)
{
    loadStyles(stylersListBoxM->GetString(stylersListBoxM->GetSelection()));
}

void PrefDlgThemeSetting::OnSelectStyleListBox(wxCommandEvent& event)
{
    loadStyle(styleListBoxM->GetString(styleListBoxM->GetSelection()));
}

void PrefDlgThemeSetting::OnChangeStyle(wxCommandEvent& event)
{
    saveStyle(styleListBoxM->GetString(styleListBoxM->GetSelection()));
}


// PrefDlgSetting factory
/* static */
PrefDlgSetting* PrefDlgSetting::createPrefDlgSetting(wxPanel* page,
    const wxString& type, const wxString& style, PrefDlgSetting* parent)
{
    if (type == "checkbox")
        return new PrefDlgCheckboxSetting(page, parent);
    if (type == "checklistbox")
        return new PrefDlgCheckListBoxSetting(page, parent);
    if (type == "colour")
        return new PrefDlgColourPickerSetting(page, parent);
    if (type == "combobox")
        return new PrefDlgComboBoxSetting(page, parent);
    if (type == "themecombobox")
        return new PrefDlgThemeSetting(page, parent);
        //return new PrefDlgThemeComboBoxSetting(page, parent);
    if (type == "radiobox")
        return new PrefDlgRadioboxSetting(page, parent);
    if (type == "int")
        return new PrefDlgIntEditSetting(page, parent);
    if (type == "string")
        return new PrefDlgStringEditSetting(page, parent);
    if (type == "file")
        return new PrefDlgFileChooserSetting(page, parent);
    if (type == "font")
        return new PrefDlgFontChooserSetting(page, parent);
    if (type == "relation_columns")
    {
        if (style == "list")
            return new PrefDlgRelationColumnsListSetting(page, parent);
        return new PrefDlgRelationColumnsChooserSetting(page, parent);
    }
    return 0;
}

