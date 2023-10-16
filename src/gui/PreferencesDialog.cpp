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

#include <algorithm>
#include <vector>

#include <wx/artprov.h>
#include <wx/bookctrl.h>
#include <wx/tokenzr.h>
#include <wx/sstream.h>
#include <wx/wfstream.h>
#include <wx/xml/xml.h>

#include "config/Config.h"
#include "config/LocalSettings.h"
#include "core/ArtProvider.h"
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "gui/ConfdefTemplateProcessor.h"
#include "gui/PreferencesDialog.h"
#include "gui/StyleGuide.h"

static bool hasParamNode(wxXmlNode* node, const wxString& param)
{
    for (wxXmlNode* n = node->GetChildren(); (n); n = n->GetNext())
    {
        if (n->GetType() == wxXML_ELEMENT_NODE && n->GetName() == param)
            return true;
    }
    return false;
}

static const wxString getNodeContent(wxXmlNode* node, const wxString& defvalue)
{
    for (wxXmlNode* n = node->GetChildren(); (n); n = n->GetNext())
    {
        if (n->GetType() == wxXML_TEXT_NODE
            || n->GetType() == wxXML_CDATA_SECTION_NODE)
        {
            return n->GetContent();
        }
    }
    return defvalue;
}

//! return wxString for comparison, used to limit features to certain platforms
wxString getPlatformName()
{
#ifdef __WINDOWS__
    return "win";
#elif defined(__MAC__) || defined(__APPLE__)
    return "mac";
#elif defined(__UNIX__)
    return "unix";
#else
    return "undefined";
#endif
}

wxString getPlatformArchitecture()
{
#if defined(_WIN64)
    return "x64";
#else
    return "x86";
#endif
}

static void processPlatformAttribute(wxXmlNode *node)
{
    wxString s;
    bool isok, isokarch;

    wxXmlNode *c = node->GetChildren();
    while (c)
    {
        isok = false;
        if (!c->GetAttribute("platform", &s))
            isok = true;
        else
        {
            wxStringTokenizer tkn(s, " |");

            while (!isok && tkn.HasMoreTokens())
            {
                if (tkn.GetNextToken().compare(getPlatformName()) == 0)
                    isok = true;
            }
        }
        isokarch = false;
        if (!c->GetAttribute("arch", &s))
            isokarch = true;
        else
        {
            wxStringTokenizer tkn(s, " |");

            while (!isokarch && tkn.HasMoreTokens())
            {
                if (tkn.GetNextToken().compare(getPlatformArchitecture()) == 0)
                    isokarch = true;
            }
        }

        if (isok && isokarch)
        {
            processPlatformAttribute(c);
            c = c->GetNext();
        }
        else
        {
            wxXmlNode *c2 = c->GetNext();
            node->RemoveChild(c);
            delete c;
            c = c2;
        }
    }
}

// Optionbook class
class Optionbook: public wxBookCtrlBase {
public:
    Optionbook()
    {
        Init();
    }
    Optionbook(wxWindow *parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0, const wxString& name = wxEmptyString)
    {
        Init();
        (void)Create(parent, id, pos, size, style, name);
    }

    bool Create(wxWindow *parent, wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0, const wxString& name = wxEmptyString);

    virtual wxSize CalcSizeFromPage(const wxSize& sizePage) const { return sizePage; }
    virtual int GetSelection() const { return m_selection; }
    // this is sloppy, SetSelection should actually send a notification event,
    // while ChangeSelection() should not...
    virtual int SetSelection(size_t n);
    virtual int ChangeSelection(size_t n) { return SetSelection(n); }
    virtual wxString GetPageText(size_t n) const;
    virtual bool SetPageText(size_t n, const wxString& strText);
    virtual int GetPageImage(size_t /*n*/) const { return -1; }
    virtual bool SetPageImage(size_t /*n*/, int /*imageId*/) { return false; }
    virtual bool InsertPage(size_t n, wxWindow *page,
        const wxString& text, bool bSelect = false, int imageId = -1);

    void OnSetFocus(wxFocusEvent& event);
    void OnSize(wxSizeEvent& event);

protected:
    virtual wxWindow *DoRemovePage(size_t page);
    wxRect GetPageRect() const { return GetClientRect(); }

    wxArrayString pageTextsM;
    int m_selection;

private:
    void Init();

    DECLARE_EVENT_TABLE()
};

bool Optionbook::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos,
    const wxSize& size, long style, const wxString& name)
{
    style &= ~wxBORDER_MASK;
    style |= wxBORDER_NONE | wxBK_LEFT;
    return wxControl::Create(parent, id, pos, size, style,
        wxDefaultValidator, name);
}

wxWindow* Optionbook::DoRemovePage(size_t page)
{
    const int page_count = GetPageCount();
    wxWindow *win = wxBookCtrlBase::DoRemovePage(page);

    pageTextsM.RemoveAt(page);

    if (win && m_selection >= (int)page)
    {
        // force new sel valid if possible
        int sel = m_selection - 1;
        if (page_count == 1)
            sel = wxNOT_FOUND;
        else if ((page_count == 2) || (sel == -1))
            sel = 0;

        // force sel invalid if deleting current page - don't try to hide it
        m_selection = (m_selection == (int)page) ? wxNOT_FOUND : m_selection - 1;

        if (sel != wxNOT_FOUND && sel != m_selection)
            SetSelection(sel);
    }
    return win;
}

wxString Optionbook::GetPageText(size_t n) const
{
    if (n < GetPageCount())
        return pageTextsM[n];
    else
        return wxEmptyString;
}

void Optionbook::Init()
{
    m_selection = wxNOT_FOUND;
}

bool Optionbook::InsertPage(size_t n, wxWindow *page, const wxString& text,
    bool bSelect, int imageId)
{
    if (!wxBookCtrlBase::InsertPage(n, page, text, bSelect, imageId))
        return false;

    pageTextsM.Insert(text, n);

    // if the inserted page is before the selected one, we must update the
    // index of the selected page
    if (int(n) <= m_selection)
    {
        // one extra page added
        m_selection++;
    }

    // some page should be selected: either this one or the first one if there
    // is still no selection
    int selNew = wxNOT_FOUND;
    if (bSelect)
        selNew = n;
    else if (m_selection == wxNOT_FOUND)
        selNew = 0;

    if (selNew != m_selection)
        page->Hide();

    if (selNew != wxNOT_FOUND)
        SetSelection(selNew);

    InvalidateBestSize();
    return true;
}

bool Optionbook::SetPageText(size_t n, const wxString& strText)
{
    wxCHECK((n >= pageTextsM.GetCount()), false);
    pageTextsM[n] = strText;
    return true;
}

int Optionbook::SetSelection(size_t n)
{
    wxCHECK((n < GetPageCount()), wxNOT_FOUND);

    const int oldSel = m_selection;
    if (int(n) != m_selection)
    {
        if (m_selection != wxNOT_FOUND)
            m_pages[m_selection]->Hide();

        wxWindow *page = m_pages[n];
        page->SetSize(GetPageRect());
        page->Show();

        m_selection = n;
    }
    return oldSel;
}

BEGIN_EVENT_TABLE(Optionbook, wxBookCtrlBase)
    EVT_SET_FOCUS(Optionbook::OnSetFocus)
    EVT_SIZE(Optionbook::OnSize)
END_EVENT_TABLE()

void Optionbook::OnSetFocus(wxFocusEvent& event)
{
    if (m_selection != wxNOT_FOUND)
    {
        wxWindow *page = m_pages[m_selection];
        if (page)
            page->SetFocus();
    }
    event.Skip();
}

void Optionbook::OnSize(wxSizeEvent& event)
{
    if (m_selection != wxNOT_FOUND)
    {
        wxWindow *page = m_pages[m_selection];
        if (page)
            page->SetSize(GetPageRect());
    }
    event.Skip();
}

// PreferencesDialog class
PreferencesDialog::PreferencesDialog(wxWindow* parent, const wxString& title,
        Config& targetConfig, const wxFileName& confDefFileName,
        const wxString& saveButtonCaption, const wxString& dialogName)
    : BaseDialog(parent, -1, title), targetConfigM(targetConfig),
    dialogNameM(dialogName)
{
    initControls(saveButtonCaption);
    loadConfDefFile(confDefFileName);
    setControlLayout();
}

PreferencesDialog::PreferencesDialog(wxWindow* parent, const wxString& title,
        Config& targetConfig, const wxString& confDefData,
        const wxString& saveButtonCaption, const wxString& dialogName)
    : BaseDialog(parent, -1, title), targetConfigM(targetConfig),
    dialogNameM(dialogName)
{
    initControls(saveButtonCaption);
    loadConfDef(confDefData);
    setControlLayout();
}

void PreferencesDialog::initControls(const wxString& saveButtonCaption)
{
    // we don't want this dialog centered on parent since it is very big, and
    // some parents (ex. main frame) could even be smaller.
    // Don't use targetConfig here, the setting must go in the global instance.
    config().setValue(getStorageName() + Config::pathSeparator + "centerDialogOnParent", false);

    treectrl_1 = new wxTreeCtrl(getControlsPanel(), ID_treectrl_panes,
        wxDefaultPosition, wxDefaultSize,
        wxBORDER_THEME | wxTR_DEFAULT_STYLE | wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT);
    panel_categ = new wxPanel(getControlsPanel(), wxID_ANY, wxDefaultPosition,
        wxDefaultSize, wxBORDER_THEME);
    static_text_categ = new wxStaticText(panel_categ, wxID_ANY, "Dummy min size text");
    bookctrl_1 = new Optionbook(getControlsPanel(), ID_bookctrl_panes,
        wxDefaultPosition, wxDefaultSize);

    button_save = new wxButton(getControlsPanel(), wxID_SAVE, saveButtonCaption);
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, _("Cancel"));
}

void PreferencesDialog::setControlLayout()
{
    // order of these is important: first create all controls, then set
    // their properties (may affect min size), then create sizer layout
    setProperties();
    layout();
    bookctrl_1->SetFocus();
    // do this last, otherwise default button style may be lost on MSW
    button_save->SetDefault();
}

PreferencesDialog::~PreferencesDialog()
{
    std::list<PrefDlgSetting*>::iterator it;
    for (it = settingsM.begin(); it != settingsM.end(); ++it)
        delete (*it);
}

bool PreferencesDialog::createControlsAndAddToSizer(wxPanel* page, wxSizer* sizerPage)
{
    // get a list of all settings belonging to this page
    std::list<PrefDlgSetting*> pageSettings;
    std::list<PrefDlgSetting*>::iterator it;
    for (it = settingsM.begin(); it != settingsM.end(); it++)
    {
        if ((*it)->getPage() == page)
            pageSettings.push_back((*it));
    }

    // create all controls on this page, creation order equals tab order
    for (it = pageSettings.begin(); pageSettings.end() != it; ++it)
    {
        if (!(*it)->createControl(!debugDescriptionM))
            return false;
    }

    // align controls after all controls have been created
    std::vector<int> groups;
    for (it = pageSettings.begin(); pageSettings.end() != it; ++it)
    {
        int group = (*it)->getControlAlignmentGroup();
        if (group > 0
            && std::find(groups.begin(), groups.end(), group) == groups.end())
        {
            groups.push_back(group);
            // get maximum left coordinate of controls in this group
            std::list<PrefDlgSetting*>::iterator it2;
            int left = 0;
            for (it2 = it; pageSettings.end() != it2; ++it2)
            {
                if ((*it2)->getControlAlignmentGroup() == group)
                    left = std::max(left, (*it2)->getControlLeft());
            }
            // set maximum left coordinate of controls in this group
            for (it2 = it; pageSettings.end() != it2; ++it2)
            {
                if ((*it2)->getControlAlignmentGroup() == group)
                    (*it2)->alignControl(left);
            }
        }
    }

    // add controls to sizer, ignore empty settings
    PrefDlgSetting* previous = 0;
    for (it = pageSettings.begin(); pageSettings.end() != it; ++it)
    {
        if ((*it)->addToSizer(sizerPage, previous))
            previous = (*it);
    }
    return true;
}

const wxString PreferencesDialog::getName() const
{
    if (!dialogNameM.IsEmpty())
        return dialogNameM;
    else
        return "PreferencesDialog";
}

int PreferencesDialog::getSelectedPage()
{
    return (bookctrl_1) ? bookctrl_1->GetSelection() : wxNOT_FOUND;
}

bool PreferencesDialog::isOk()
{
    return loadSuccessM;
}

void PreferencesDialog::layout()
{
    wxBoxSizer* sizerCateg = new wxBoxSizer(wxHORIZONTAL);
    sizerCateg->Add(static_text_categ, 1, wxEXPAND|wxALL|wxFIXED_MINSIZE, 5);
    panel_categ->SetAutoLayout(true);
    panel_categ->SetSizerAndFit(sizerCateg);

    wxBoxSizer* sizerRight = new wxBoxSizer(wxVERTICAL);
    sizerRight->Add(panel_categ, 0, wxEXPAND);
    sizerRight->Add(0, styleguide().getUnrelatedControlMargin(wxVERTICAL));
    sizerRight->Add(bookctrl_1, 1, wxEXPAND);

    wxBoxSizer* sizerControls = new wxBoxSizer(wxHORIZONTAL);
    int bookProportion = 5;
    if (bookctrl_1->GetPageCount() > 1)
    {
        int treeProportion = 2;
        // for some reason the tree width isn't calculated correctly on the Mac
        // use proportional widths for tree and book controls there
        if (wxPlatformInfo::Get().GetOperatingSystemId() & wxOS_MAC)
        {
            treeProportion = 2;
            bookProportion = 5;
        }
        sizerControls->Add(treectrl_1, treeProportion, wxEXPAND);
        sizerControls->Add(styleguide().getUnrelatedControlMargin(wxHORIZONTAL), 0);
    }
    sizerControls->Add(sizerRight, bookProportion, wxEXPAND);

    // create sizer for buttons -> styleguide class will align it correctly
    wxSizer* sizerButtons = styleguide().createButtonSizer(button_save,
        button_cancel);
    // use method in base class to set everything up
    layoutSizers(sizerControls, sizerButtons, true);
}

void PreferencesDialog::loadConfDefFile(const wxFileName& filename)
{
    loadSuccessM = false;

    if (!filename.FileExists())
    {
        wxString msg;
        msg.Printf(_("The support file:\n%s\ndoes not exist!\n\nThis means there is a problem with your installation of FlameRobin."),
            filename.GetFullPath().c_str());
        wxMessageBox(msg, _("Support file not found"), wxOK | wxICON_ERROR);
        return;
    }
    loadConfDef(loadEntireFile(filename));
}

void PreferencesDialog::loadConfDef(const wxString& confDefData)
{
    wxStringInputStream stream(confDefData);
    wxXmlDocument doc;
    if (!doc.Load(stream))
        return;
    wxXmlNode* xmlr = doc.GetRoot();
    if (xmlr->GetName() != "root")
    {
        wxLogError(_("Invalid root node in confdef."));
        return;
    }
    processPlatformAttribute(xmlr);
    debugDescriptionM = hasParamNode(xmlr, "debug");

    wxTreeItemId root = treectrl_1->AddRoot(wxEmptyString);
    for (wxXmlNode* xmln = doc.GetRoot()->GetChildren();
        (xmln); xmln = xmln->GetNext())
    {
        if (xmln->GetType() == wxXML_ELEMENT_NODE
            && xmln->GetName() == "node")
        {
            if (!parseDescriptionNode(root, xmln))
                return;
        }
    }
    loadSuccessM = true;
}

bool PreferencesDialog::loadFromTargetConfig()
{
    std::list<PrefDlgSetting*>::iterator it;
    for (it = settingsM.begin(); it != settingsM.end(); it++)
    {
        if (!(*it)->loadFromTargetConfig(targetConfigM))
            return false;
    }
    return true;
}

bool PreferencesDialog::parseDescriptionNode(wxTreeItemId parent, wxXmlNode* xmln)
{
    // ignore empty nodes
    if (!xmln->GetChildren())
        return true;

    wxTreeItemId item = treectrl_1->AppendItem(parent, wxEmptyString, -1, -1);
    wxPanel* page = new wxPanel(bookctrl_1);
    wxString caption;
    wxString description;

    // parse subnodes and settings (recursively)
    for (xmln = xmln->GetChildren(); (xmln); xmln = xmln->GetNext())
    {
        if (xmln->GetType() != wxXML_ELEMENT_NODE)
            continue;
        if (xmln->GetName() == "caption")
            caption = getNodeContent(xmln, wxEmptyString);
        else if (xmln->GetName() == "description")
            description = getNodeContent(xmln, wxEmptyString);
        else if (xmln->GetName() == "image")
        {
            wxString value(getNodeContent(xmln, wxEmptyString));
            long l;
            if (!value.IsEmpty() && value.ToLong(&l))
                treectrl_1->SetItemImage(item, l);
        }
        else if (xmln->GetName() == "node")
        {
            if (!parseDescriptionNode(item, xmln))
                return false;
        }
        else if (xmln->GetName() == "setting")
        {
            if (!parseDescriptionSetting(page, xmln, 0))
                return false;
        }
    }

    if (!caption.IsEmpty())
    {
        treectrl_1->SetItemText(item, caption);
        if (description.IsEmpty())
            description = caption;
    }
    treectrl_1->ExpandAllChildren(item);

    // add all settings controls of this page to a sizer
    wxBoxSizer* sizerPage = new wxBoxSizer(wxVERTICAL);
    bool controlsok = createControlsAndAddToSizer(page, sizerPage);
    page->SetSizerAndFit(sizerPage);

    // add this page to the bookctrl, and use the tree item index
    // in treeItemsM to select the page (see OnTreeSelChanged() )
    bookctrl_1->AddPage(page, description);
    treeItemsM.Add(item);
    return controlsok;
}

bool PreferencesDialog::parseDescriptionSetting(wxPanel* page, wxXmlNode* xmln,
    PrefDlgSetting* enabledby)
{
    wxString type(xmln->GetAttribute("type", wxEmptyString));
    wxString style(xmln->GetAttribute("style", wxEmptyString));
    PrefDlgSetting* setting = PrefDlgSetting::createPrefDlgSetting(page,
        type, style, enabledby);
    // ignore unknown settings unless debug mode is active
    if (setting == 0)
    {
        if (!debugDescriptionM)
            return true;
        wxLogError(_("Unknown config setting \"%s\" in description"),
            type.c_str());
        return false;
    }

    settingsM.push_back(setting);
    for (xmln = xmln->GetChildren(); (xmln); xmln = xmln->GetNext())
    {
        if (xmln->GetType() != wxXML_ELEMENT_NODE)
            continue;
        if (xmln->GetName() == "enables")
        {
            for (wxXmlNode* xmlc = xmln->GetChildren();
                (xmlc); xmlc = xmlc->GetNext())
            {
                if (xmlc->GetType() == wxXML_ELEMENT_NODE
                    && xmlc->GetName() == "setting")
                {
                    if (!parseDescriptionSetting(page, xmlc, setting))
                        return false;
                }
            }
        }
        else
        {
            if (!setting->parseProperty(xmln))
                return false;
        }
    }
    return true;
}

bool PreferencesDialog::saveToTargetConfig()
{
    // Avoid flushing the config object at each write.
    SubjectLocker locker(&targetConfigM);

    std::list<PrefDlgSetting*>::iterator it;
    for (it = settingsM.begin(); it != settingsM.end(); it++)
    {
        if (!(*it)->saveToTargetConfig(targetConfigM))
            return false;
    }
    return true;
}

void PreferencesDialog::selectPage(int index)
{
    if (bookctrl_1 && index >= 0 && index < (int)treeItemsM.GetCount())
    {
        treectrl_1->SelectItem(treeItemsM[index]);
        bookctrl_1->SetSelection(index);
        bookctrl_1->SetFocus();
    }
}

void PreferencesDialog::setProperties()
{
    // setup image list for tree control
    wxSize sz(16, 16);
    imageListM.Create(sz.GetWidth(), sz.GetHeight());
    imageListM.Add(wxArtProvider::GetIcon(ART_Column, wxART_OTHER, sz));
    imageListM.Add(wxArtProvider::GetIcon(ART_Procedure, wxART_OTHER, sz));
    imageListM.Add(wxArtProvider::GetIcon(ART_Trigger, wxART_OTHER, sz));
    imageListM.Add(wxArtProvider::GetIcon(ART_Object, wxART_OTHER, sz));
    imageListM.Add(wxArtProvider::GetIcon(ART_Table, wxART_OTHER, sz));
    imageListM.Add(wxArtProvider::GetIcon(ART_Domain, wxART_OTHER, sz));
    treectrl_1->SetImageList(&imageListM);
    treectrl_1->SetQuickBestSize(false);

    // show category description in colors for highlighted text
    panel_categ->SetBackgroundColour(
        wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));

    static_text_categ->SetForegroundColour(
        wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
    wxFont font(static_text_categ->GetFont());
    font.SetPointSize(15);
    font.MakeBold();
    static_text_categ->SetFont(font);
}

BEGIN_EVENT_TABLE(PreferencesDialog, wxDialog)
    EVT_BUTTON(wxID_SAVE, PreferencesDialog::OnSaveButtonClick)
    EVT_TREE_SEL_CHANGED(PreferencesDialog::ID_treectrl_panes,
        PreferencesDialog::OnTreeSelChanged)
END_EVENT_TABLE()

void PreferencesDialog::OnSaveButtonClick(wxCommandEvent& WXUNUSED(event))
{
    wxBusyCursor wait;
    if (saveToTargetConfig()) {
        LocalSettings locSet;
        EndModal(wxID_OK);
    }
}

void PreferencesDialog::OnTreeSelChanged(wxTreeEvent& event)
{
    wxTreeItemId item = event.GetItem();
    // search page for selected tree item, select it, and update description
    for (size_t i = 0; i < treeItemsM.GetCount(); i++)
    {
        if (treeItemsM[i] == item)
        {
            bookctrl_1->SetSelection(i);
            static_text_categ->SetLabel(bookctrl_1->GetPageText(i));
            break;
        }
    }
}

bool PreferencesDialog::Show(bool show)
{
    bool r = BaseDialog::Show(show);
    if (show && bookctrl_1->GetPageCount() <= 1)
        treectrl_1->Hide();
    if (bookctrl_1->GetPageCount())
        static_text_categ->SetLabel(bookctrl_1->GetPageText(0));
    return r;
}

class PreferencesDialogTemplateCmdHandler: public TemplateCmdHandler
{
private:
    static const PreferencesDialogTemplateCmdHandler handlerInstance; // singleton; registers itself on creation.
public:
    PreferencesDialogTemplateCmdHandler() {};
    virtual void handleTemplateCmd(TemplateProcessor *tp,
        const wxString& cmdName, const TemplateCmdParams& cmdParams,
        ProcessableObject* object, wxString& processedText);
};

const PreferencesDialogTemplateCmdHandler PreferencesDialogTemplateCmdHandler::handlerInstance;

void PreferencesDialogTemplateCmdHandler::handleTemplateCmd(TemplateProcessor *tp,
    const wxString& cmdName, const TemplateCmdParams& /*cmdParams*/,
    ProcessableObject* object, wxString& /*processedText*/)
{
    // {%edit_conf%} shows a gui to set config params based on <template name>.confdef.
    // {%edit_info%} shows a gui to set template info based on template_info.confdef
    if (cmdName == "edit_conf" || cmdName == "edit_info")
    {
        wxFileName defFileName = tp->getCurrentTemplateFileName();
        wxString dialogName = defFileName.GetName();
        defFileName.SetExt("confdef");
        if (cmdName == "edit_info")
            defFileName.SetName("template_info");

        // Expand commands in confdef file.
        ConfdefTemplateProcessor ctp(object, tp->getWindow());
        wxString confDefData;
        ctp.processTemplateFile(confDefData, defFileName, object, tp->getProgressIndicator());

        // Show dialog for expanded confdef data.
        wxString dialogTitle(_("Code generation"));
        Config& config(tp->getConfig());
        if (cmdName == "edit_info")
        {
            dialogTitle = _("Code template metadata");
            config = tp->getInfo();
        }
        PreferencesDialog pd(tp->getWindow(), dialogTitle,
            config, confDefData, _("Continue"), dialogName);

        if (pd.isOk() && pd.loadFromTargetConfig())
            if (pd.ShowModal() != wxID_OK)
                throw FRAbort();
    }
}
