/*
  Copyright (c) 2004-2011 The FlameRobin Development Team

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
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/fontdlg.h>
#include <wx/spinctrl.h>
#include <wx/tokenzr.h>
#include <wx/xml/xml.h>

#include <vector>

#include "config/Config.h"
#include "frutils.h"
#include "gui/PreferencesDialog.h"
#include "gui/StyleGuide.h"
#include "metadata/database.h"
#include "metadata/relation.h"
//-----------------------------------------------------------------------------
static const wxString getNodeContent(wxXmlNode* node, const wxString& defvalue)
{
    wxString content;
    for (wxXmlNode* n = node->GetChildren(); (n); n = n->GetNext())
    {
        int type = n->GetType();
        if (type == wxXML_TEXT_NODE || type == wxXML_CDATA_SECTION_NODE)
            content += n->GetContent();
        else if (type == wxXML_ELEMENT_NODE && n->GetName() == wxT("br"))
            content += wxT("\n");
    }
    return content.IsEmpty() ? defvalue : content;
}
//-----------------------------------------------------------------------------
// PrefDlgSetting class
PrefDlgSetting::PrefDlgSetting(wxPanel* page, PrefDlgSetting* parent)
{
    pageM = page;
    parentM = parent;
    if (parent)
        parent->addEnabledSetting(this);
    relatedM = false;
    alignmentGroupM = -1;
}
//-----------------------------------------------------------------------------
PrefDlgSetting::~PrefDlgSetting()
{
}
//-----------------------------------------------------------------------------
void PrefDlgSetting::addEnabledSetting(PrefDlgSetting* setting)
{
    if (setting)
        enabledSettingsM.push_back(setting);
}
//-----------------------------------------------------------------------------
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
    sizer->Add(hsizer, 0, wxEXPAND | wxFIXED_MINSIZE);
    return true;
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
void PrefDlgSetting::enableEnabledSettings(bool enabled)
{
    std::list<PrefDlgSetting*>::iterator it;
    for (it = enabledSettingsM.begin(); it != enabledSettingsM.end(); it++)
        (*it)->enableControls(enabled);
}
//-----------------------------------------------------------------------------
inline int PrefDlgSetting::getControlIndentation(int level) const
{
    return level * 20;
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
int PrefDlgSetting::getControlAlignmentGroup() const
{
    return alignmentGroupM;
}
//-----------------------------------------------------------------------------
wxStaticText* PrefDlgSetting::getLabel()
{
    return 0;
}
//-----------------------------------------------------------------------------
int PrefDlgSetting::getLevel() const
{
    if (parentM)
        return parentM->getLevel() + 1;
    else
        return 0;
}
//-----------------------------------------------------------------------------
wxPanel* PrefDlgSetting::getPage() const
{
    return pageM;
}
//-----------------------------------------------------------------------------
bool PrefDlgSetting::isRelatedTo(PrefDlgSetting* prevSetting) const
{
    if (!prevSetting)
        return false;
    if (relatedM)
        return true;
    int prevLevel = prevSetting->getLevel();
    int level = getLevel();

    return (level > prevLevel)
        || (level > 0 && level == prevLevel);
}
//-----------------------------------------------------------------------------
bool PrefDlgSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE)
    {
        wxString name(xmln->GetName());
        wxString value(getNodeContent(xmln, wxEmptyString));

        if (name == wxT("caption"))
            captionM = value;
        else if (name == wxT("description"))
            descriptionM = value;
        else if (name == wxT("key"))
            keyM = value;
        else if (name == wxT("default"))
            setDefault(value);
        else if (name == wxT("related"))
            relatedM = true;
        else if (name == wxT("aligngroup"))
        {
            long l;
            if (value.ToLong(&l) && l > 0)
                alignmentGroupM = l;
        }
    }
    return true;
}
//-----------------------------------------------------------------------------
void PrefDlgSetting::setDefault(const wxString& WXUNUSED(defValue))
{
}
//-----------------------------------------------------------------------------
// PrefDlgCheckboxSetting class
class PrefDlgCheckboxSetting: public PrefDlgSetting
{
public:
    PrefDlgCheckboxSetting(wxPanel* page, PrefDlgSetting* parent);
    ~PrefDlgCheckboxSetting();

    virtual bool createControl(bool ignoreerrors);
    virtual bool loadFromTargetConfig(Config& config);
    virtual bool saveToTargetConfig(Config& config);

    void OnCheckbox(wxCommandEvent& event);
protected:
    virtual void addControlsToSizer(wxSizer* sizer);
    virtual void enableControls(bool enabled);
    virtual bool hasControls() const;
    virtual void setDefault(const wxString& defValue);
private:
    wxCheckBox* checkboxM;
    bool defaultM;

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
PrefDlgCheckboxSetting::PrefDlgCheckboxSetting(wxPanel* page, PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent)
{
    checkboxM = 0;
    defaultM = false;
}
//-----------------------------------------------------------------------------
PrefDlgCheckboxSetting::~PrefDlgCheckboxSetting()
{
    if (checkboxM)
        checkboxM->PopEventHandler();
}
//-----------------------------------------------------------------------------
void PrefDlgCheckboxSetting::addControlsToSizer(wxSizer* sizer)
{
    if (checkboxM)
        sizer->Add(checkboxM, 1, wxFIXED_MINSIZE);
}
//-----------------------------------------------------------------------------
bool PrefDlgCheckboxSetting::createControl(bool WXUNUSED(ignoreerrors))
{
    checkboxM = new wxCheckBox(getPage(), wxID_ANY, captionM);
    if (!descriptionM.IsEmpty())
        checkboxM->SetToolTip(descriptionM);
    checkboxM->PushEventHandler(this);
    return true;
}
//-----------------------------------------------------------------------------
void PrefDlgCheckboxSetting::enableControls(bool enabled)
{
    if (checkboxM)
        checkboxM->Enable(enabled);
}
//-----------------------------------------------------------------------------
bool PrefDlgCheckboxSetting::hasControls() const
{
    return checkboxM != 0;
}
//-----------------------------------------------------------------------------
bool PrefDlgCheckboxSetting::loadFromTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    bool checked = defaultM;
    if (checkboxM)
    {
        config.getValue(keyM, checked);
        checkboxM->SetValue(checked);
    }
    enableEnabledSettings(checked);
    return true;
}
//-----------------------------------------------------------------------------
bool PrefDlgCheckboxSetting::saveToTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (checkboxM)
        config.setValue(keyM, checkboxM->GetValue());
    return true;
}
//-----------------------------------------------------------------------------
void PrefDlgCheckboxSetting::setDefault(const wxString& defValue)
{
    defaultM = (defValue == wxT("on") || defValue == wxT("yes"));
    long l;
    if (defValue.ToLong(&l) && l != 0)
        defaultM = true;
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(PrefDlgCheckboxSetting, wxEvtHandler)
    EVT_CHECKBOX(wxID_ANY, PrefDlgCheckboxSetting::OnCheckbox)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void PrefDlgCheckboxSetting::OnCheckbox(wxCommandEvent& event)
{
    enableEnabledSettings(event.IsChecked());
}
//-----------------------------------------------------------------------------
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
    wxRadioBox* radioboxM;
    wxArrayString choicesM;
    int defaultM;
};
//-----------------------------------------------------------------------------
PrefDlgRadioboxSetting::PrefDlgRadioboxSetting(wxPanel* page, PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent)
{
    radioboxM = 0;
    defaultM = wxNOT_FOUND;
}
//-----------------------------------------------------------------------------
void PrefDlgRadioboxSetting::addControlsToSizer(wxSizer* sizer)
{
    if (radioboxM)
        sizer->Add(radioboxM, 1, wxFIXED_MINSIZE);
}
//-----------------------------------------------------------------------------
bool PrefDlgRadioboxSetting::createControl(bool ignoreerrors)
{
    if (choicesM.GetCount() == 0)
    {
        if (!ignoreerrors)
            wxLogError(_("Radiobox \"%s\" has no options"), captionM.c_str());
        return ignoreerrors;
    }
    radioboxM = new wxRadioBox(getPage(), wxID_ANY, captionM,
        wxDefaultPosition, wxDefaultSize, choicesM, 1);
    if (!descriptionM.IsEmpty())
        radioboxM->SetToolTip(descriptionM);
    return true;
}
//-----------------------------------------------------------------------------
void PrefDlgRadioboxSetting::enableControls(bool enabled)
{
    if (radioboxM)
        radioboxM->Enable(enabled);
}
//-----------------------------------------------------------------------------
bool PrefDlgRadioboxSetting::hasControls() const
{
    return radioboxM != 0;
}
//-----------------------------------------------------------------------------
bool PrefDlgRadioboxSetting::loadFromTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (radioboxM)
    {
        int value = defaultM;
        config.getValue(keyM, value);
        if (value >= 0 && value < (int)choicesM.GetCount())
            radioboxM->SetSelection(value);
    }
    return true;
}
//-----------------------------------------------------------------------------
bool PrefDlgRadioboxSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE
        && xmln->GetName() == wxT("option"))
    {
        wxString optcaption;

        for (wxXmlNode* xmlc = xmln->GetChildren(); (xmlc); xmlc = xmlc->GetNext())
        {
            if (xmlc->GetType() != wxXML_ELEMENT_NODE)
                continue;
            wxString value(getNodeContent(xmlc, wxEmptyString));
            if (xmlc->GetName() == wxT("caption"))
                optcaption = value;
/*
            else if (xmlc->GetName() == wxT("value"))
                optvalue = value;
*/
        }
        // for the time being values are the index of the caption in the array
        if (!optcaption.IsEmpty())
            choicesM.Add(optcaption);
    }
    return PrefDlgSetting::parseProperty(xmln);
}
//-----------------------------------------------------------------------------
bool PrefDlgRadioboxSetting::saveToTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (radioboxM)
        config.setValue(keyM, radioboxM->GetSelection());
    return true;
}
//-----------------------------------------------------------------------------
void PrefDlgRadioboxSetting::setDefault(const wxString& defValue)
{
    long l;
    if (defValue.ToLong(&l) && l >= 0 && l < long(choicesM.GetCount()))
        defaultM = l;
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
PrefDlgIntEditSetting::PrefDlgIntEditSetting(wxPanel* page, PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent)
{
    captionAfterM = 0;
    captionBeforeM = 0;
    spinctrlM = 0;
    maxValueM = 100;
    minValueM = 0;
    defaultM = 0;
}
//-----------------------------------------------------------------------------
void PrefDlgIntEditSetting::addControlsToSizer(wxSizer* sizer)
{
    if (spinctrlM)
    {
        if (captionBeforeM)
        {
            sizer->Add(captionBeforeM, 0, wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
            sizer->Add(styleguide().getControlLabelMargin(), 0);
        }
        sizer->Add(spinctrlM, 0, wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
        if (captionAfterM)
        {
            sizer->Add(styleguide().getControlLabelMargin(), 0);
            sizer->Add(captionAfterM, 0, wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
        }
    }
}
//-----------------------------------------------------------------------------
bool PrefDlgIntEditSetting::createControl(bool WXUNUSED(ignoreerrors))
{
    wxString caption1(captionM);
    wxString caption2;

    int pos = captionM.Find(wxT("[VALUE]"));
    if (pos >= 0)
    {
        caption1.Truncate(size_t(pos));
        caption1.Trim(true);
        caption2 = captionM.Mid(pos + 7).Trim(false);
    }

    if (!caption1.IsEmpty())
        captionBeforeM = new wxStaticText(getPage(), wxID_ANY, caption1);
    spinctrlM = new wxSpinCtrl(getPage(), wxID_ANY);
    spinctrlM->SetRange(minValueM, maxValueM);
    if (!caption2.IsEmpty())
        captionAfterM = new wxStaticText(getPage(), wxID_ANY, caption2);

    if (!descriptionM.IsEmpty())
    {
        spinctrlM->SetToolTip(descriptionM);
        if (captionBeforeM)
            captionBeforeM->SetToolTip(descriptionM);
        if (captionAfterM)
            captionAfterM->SetToolTip(descriptionM);
    }
    return true;
}
//-----------------------------------------------------------------------------
void PrefDlgIntEditSetting::enableControls(bool enabled)
{
    if (spinctrlM)
        spinctrlM->Enable(enabled);
}
//-----------------------------------------------------------------------------
wxStaticText* PrefDlgIntEditSetting::getLabel()
{
    return captionBeforeM;
}
//-----------------------------------------------------------------------------
bool PrefDlgIntEditSetting::hasControls() const
{
    return (captionBeforeM) || (spinctrlM) || (captionAfterM);
}
//-----------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------
bool PrefDlgIntEditSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE)
    {
        wxString name(xmln->GetName());
        if (name == wxT("minvalue") || name == wxT("maxvalue"))
        {
            wxString value(getNodeContent(xmln, wxEmptyString));
            long l;
            if (!value.IsEmpty() && value.ToLong(&l))
            {
                if (name == wxT("maxvalue"))
                    maxValueM = l;
                else
                    minValueM = l;
            }
        }
    }
    return PrefDlgSetting::parseProperty(xmln);
}
//-----------------------------------------------------------------------------
bool PrefDlgIntEditSetting::saveToTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (spinctrlM)
        config.setValue(keyM, spinctrlM->GetValue());
    return true;
}
//-----------------------------------------------------------------------------
void PrefDlgIntEditSetting::setDefault(const wxString& defValue)
{
    long l;
    if (defValue.ToLong(&l))
        defaultM = l;
}
//-----------------------------------------------------------------------------
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
    wxTextCtrl* textctrlM;
    wxString defaultM;
    int expandM;
};
//-----------------------------------------------------------------------------
PrefDlgStringEditSetting::PrefDlgStringEditSetting(wxPanel* page, PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent)
{
    captionAfterM = 0;
    captionBeforeM = 0;
    textctrlM = 0;
    expandM = 0;
}
//-----------------------------------------------------------------------------
void PrefDlgStringEditSetting::addControlsToSizer(wxSizer* sizer)
{
    if (textctrlM)
    {
        if (captionBeforeM)
        {
            sizer->Add(captionBeforeM, 0, wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
            sizer->Add(styleguide().getControlLabelMargin(), 0);
        }
        sizer->Add(textctrlM, expandM, wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
        if (captionAfterM)
        {
            sizer->Add(styleguide().getControlLabelMargin(), 0);
            sizer->Add(captionAfterM, 0, wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
        }
    }
}
//-----------------------------------------------------------------------------
bool PrefDlgStringEditSetting::createControl(bool WXUNUSED(ignoreerrors))
{
    wxString caption1(captionM);
    wxString caption2;

    int pos = captionM.Find(wxT("[VALUE]"));
    if (pos >= 0)
    {
        caption1.Truncate(size_t(pos));
        caption1.Trim(true);
        caption2 = captionM.Mid(pos + 7).Trim(false);
    }

    if (!caption1.IsEmpty())
        captionBeforeM = new wxStaticText(getPage(), wxID_ANY, caption1);
    textctrlM = new wxTextCtrl(getPage(), wxID_ANY);
    if (!caption2.IsEmpty())
        captionAfterM = new wxStaticText(getPage(), wxID_ANY, caption2);

    if (!descriptionM.IsEmpty())
    {
        textctrlM->SetToolTip(descriptionM);
        if (captionBeforeM)
            captionBeforeM->SetToolTip(descriptionM);
        if (captionAfterM)
            captionAfterM->SetToolTip(descriptionM);
    }
    return true;
}
//-----------------------------------------------------------------------------
void PrefDlgStringEditSetting::enableControls(bool enabled)
{
    if (textctrlM)
        textctrlM->Enable(enabled);
}
//-----------------------------------------------------------------------------
wxStaticText* PrefDlgStringEditSetting::getLabel()
{
    return captionBeforeM;
}
//-----------------------------------------------------------------------------
bool PrefDlgStringEditSetting::hasControls() const
{
    return (captionBeforeM) || (textctrlM) || (captionAfterM);
}
//-----------------------------------------------------------------------------
bool PrefDlgStringEditSetting::loadFromTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (textctrlM)
    {
        wxString value = defaultM;
        config.getValue(keyM, value);
        textctrlM->SetValue(value);
    }
    return true;
}
//-----------------------------------------------------------------------------
bool PrefDlgStringEditSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE)
    {
        wxString name(xmln->GetName());
        if (name == wxT("expand"))
        {
            wxString value(getNodeContent(xmln, wxEmptyString));
            long l;
            if (!value.IsEmpty() && value.ToLong(&l))
                expandM = l;
        }
    }
    return PrefDlgSetting::parseProperty(xmln);
}
//-----------------------------------------------------------------------------
bool PrefDlgStringEditSetting::saveToTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (textctrlM)
        config.setValue(keyM, textctrlM->GetValue());
    return true;
}
//-----------------------------------------------------------------------------
void PrefDlgStringEditSetting::setDefault(const wxString& defValue)
{
    defaultM = defValue;
}
//-----------------------------------------------------------------------------
// PrefDlgChooserSetting class
class PrefDlgChooserSetting: public PrefDlgSetting
{
public:
    enum {choosefile, choosefont, chooserelcolumns};

    PrefDlgChooserSetting(int style, wxPanel* page, PrefDlgSetting* parent);
    ~PrefDlgChooserSetting();

    virtual bool createControl(bool ignoreerrors);
    virtual bool loadFromTargetConfig(Config& config);
    virtual bool saveToTargetConfig(Config& config);
    virtual bool parseProperty(wxXmlNode* xmln);
    void OnBrowseButton(wxCommandEvent& event);
protected:
    virtual void addControlsToSizer(wxSizer* sizer);
    virtual void enableControls(bool enabled);
    virtual wxStaticText* getLabel();
    virtual bool hasControls() const;
    virtual void setDefault(const wxString& defValue);
private:
    const int styleM;
    wxButton* browsebtnM;
    wxStaticText* captionBeforeM;
    wxTextCtrl* textctrlM;
    wxString defaultM;
    Relation* relationM;

    void chooseFile();
    void chooseFont();
    void chooseRelationColumns();

    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
PrefDlgChooserSetting::PrefDlgChooserSetting(int style, wxPanel* page, PrefDlgSetting* parent)
    : PrefDlgSetting(page, parent), styleM(style)
{
    browsebtnM = 0;
    captionBeforeM = 0;
    textctrlM = 0;
}
//-----------------------------------------------------------------------------
PrefDlgChooserSetting::~PrefDlgChooserSetting()
{
    if (browsebtnM)
        browsebtnM->PopEventHandler();
}
//-----------------------------------------------------------------------------
void PrefDlgChooserSetting::addControlsToSizer(wxSizer* sizer)
{
    if (textctrlM)
    {
        if (captionBeforeM)
        {
            sizer->Add(captionBeforeM, 0, wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
            sizer->Add(styleguide().getControlLabelMargin(), 0);
        }
        sizer->Add(textctrlM, 1, wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
        if (browsebtnM)
        {
            sizer->Add(styleguide().getBrowseButtonMargin(), 0);
            sizer->Add(browsebtnM, 0, wxFIXED_MINSIZE | wxALIGN_CENTER_VERTICAL);
        }
    }
}
//-----------------------------------------------------------------------------
void PrefDlgChooserSetting::chooseFile()
{
    wxString path;
    wxFileName::SplitPath(textctrlM->GetValue(), &path, 0, 0);

    wxString filename = ::wxFileSelector(_("Select File"), path,
        wxEmptyString, wxEmptyString, _("All files (*.*)|*.*"),
        wxFD_SAVE, ::wxGetTopLevelParent(textctrlM));
    if (!filename.IsEmpty())
        textctrlM->SetValue(filename);
}
//-----------------------------------------------------------------------------
void PrefDlgChooserSetting::chooseFont()
{
    wxFont font;
    wxString fontdesc = textctrlM->GetValue();
    if (!fontdesc.IsEmpty())
        font.SetNativeFontInfo(fontdesc);
    wxFont font2 = ::wxGetFontFromUser(::wxGetTopLevelParent(textctrlM), font);
    if (font2.Ok())
        textctrlM->SetValue(font2.GetNativeFontInfoDesc());
}
//-----------------------------------------------------------------------------
void PrefDlgChooserSetting::chooseRelationColumns()
{
    wxArrayString defaultNames = ::wxStringTokenize(textctrlM->GetValue(),
        wxT(","));
    std::vector<wxString> list;
    for (wxString::size_type i = 0; i < defaultNames.Count(); i++)
        list.push_back(defaultNames[i].Trim(true).Trim(false));

    if (selectRelationColumnsIntoVector(relationM, getPage(), list))
    {
        std::vector<wxString>::iterator it = list.begin();
        wxString retval(*it);
        while ((++it) != list.end())
            retval += wxT(", ") + (*it);
        textctrlM->SetValue(retval);
    }
}
//-----------------------------------------------------------------------------
bool PrefDlgChooserSetting::createControl(bool WXUNUSED(ignoreerrors))
{
    if (!captionM.IsEmpty())
        captionBeforeM = new wxStaticText(getPage(), wxID_ANY, captionM);
    textctrlM = new wxTextCtrl(getPage(), wxID_ANY);
    browsebtnM = new wxButton(getPage(), wxID_ANY, _("Select..."));
    browsebtnM->PushEventHandler(this);

    if (!descriptionM.IsEmpty())
    {
        if (captionBeforeM)
            captionBeforeM->SetToolTip(descriptionM);
        textctrlM->SetToolTip(descriptionM);
        if (browsebtnM)
            browsebtnM->SetToolTip(descriptionM);
    }
    return true;
}
//-----------------------------------------------------------------------------
void PrefDlgChooserSetting::enableControls(bool enabled)
{
    if (textctrlM)
        textctrlM->Enable(enabled);
    if (browsebtnM)
    {
        if ((styleM == chooserelcolumns) && !relationM)
            browsebtnM->Enable(false);
        else
            browsebtnM->Enable(enabled);
    }
}
//-----------------------------------------------------------------------------
wxStaticText* PrefDlgChooserSetting::getLabel()
{
    return captionBeforeM;
}
//-----------------------------------------------------------------------------
bool PrefDlgChooserSetting::hasControls() const
{
    return (captionBeforeM) || (textctrlM) ||(browsebtnM);
}
//-----------------------------------------------------------------------------
bool PrefDlgChooserSetting::loadFromTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (textctrlM)
    {
        wxString value = defaultM;
        config.getValue(keyM, value);
        textctrlM->SetValue(value);
    }
    enableControls(true);
    return true;
}
//-----------------------------------------------------------------------------
bool PrefDlgChooserSetting::saveToTargetConfig(Config& config)
{
    if (!checkTargetConfigProperties())
        return false;
    if (textctrlM)
        config.setValue(keyM, textctrlM->GetValue());
    return true;
}
//-----------------------------------------------------------------------------
void PrefDlgChooserSetting::setDefault(const wxString& defValue)
{
    defaultM = defValue;
}
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(PrefDlgChooserSetting, wxEvtHandler)
    EVT_BUTTON(wxID_ANY, PrefDlgChooserSetting::OnBrowseButton)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
void PrefDlgChooserSetting::OnBrowseButton(wxCommandEvent& WXUNUSED(event))
{
    if (textctrlM == 0)
        return;
    if (styleM == choosefile)
        chooseFile();
    else if (styleM == choosefont)
        chooseFont();
    else if (styleM == chooserelcolumns)
        chooseRelationColumns();
}
//-----------------------------------------------------------------------------
bool PrefDlgChooserSetting::parseProperty(wxXmlNode* xmln)
{
    if (xmln->GetType() == wxXML_ELEMENT_NODE)
    {
        wxString name(xmln->GetName());
        if (name == wxT("relation"))
        {
            wxString handle(getNodeContent(xmln, wxEmptyString));
            relationM = dynamic_cast<Relation*>(
                MetadataItem::getObjectFromHandle(handle));
        }
    }
    return PrefDlgSetting::parseProperty(xmln);
}
//-----------------------------------------------------------------------------
// PrefDlgSetting factory
PrefDlgSetting* createPrefDlgSetting(wxPanel* page, const wxString& type,
    PrefDlgSetting* parent)
{
    if (type == wxT("checkbox"))
        return new PrefDlgCheckboxSetting(page, parent);
    if (type == wxT("radiobox"))
        return new PrefDlgRadioboxSetting(page, parent);
    if (type == wxT("int"))
        return new PrefDlgIntEditSetting(page, parent);
    if (type == wxT("string"))
        return new PrefDlgStringEditSetting(page, parent);
    if (type == wxT("file"))
        return new PrefDlgChooserSetting(PrefDlgChooserSetting::choosefile, page, parent);
    if (type == wxT("font"))
        return new PrefDlgChooserSetting(PrefDlgChooserSetting::choosefont, page, parent);
    if (type == wxT("relation_columns"))
        return new PrefDlgChooserSetting(PrefDlgChooserSetting::chooserelcolumns, page, parent);
    return 0;
}
//-----------------------------------------------------------------------------
