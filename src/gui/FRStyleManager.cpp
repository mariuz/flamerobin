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

#include <algorithm>

#include "FRStyleManager.h"


FRStyleManager& stylerManager()
{
    //wxFileName file = wxFileName(config().getXmlStylesPath(), config().get(_STYLE, _def) + ".xml");
    static FRStyleManager s;
    return s;
}

void FRStyleManager::loadLexerStyles(wxXmlNode* node)
{
    stylersM.clear();

    wxXmlNode* child = node->GetChildren();

    while (child) {
        if (child->GetType() == wxXML_ELEMENT_NODE && child->GetName() == "LexerType")
        {
            stylersM.addStyler(child->GetAttribute("name"),
                child->GetAttribute("desc"), child->GetAttribute("ext"), child);
        }
        child = child->GetNext();
    };

}

void FRStyleManager::loadGlobalStyles(wxXmlNode* node)
{
    globalStylerM->clear();
    wxXmlNode* child = node->GetChildren();

    while (child) {
        if (child->GetType() == wxXML_ELEMENT_NODE && child->GetName() == "WidgetStyle") {
            long styleID = -1;
            if (child->GetAttribute("styleID").ToLong(&styleID)) {
                globalStylerM->addStyler(styleID, child);
            }
        }
        child = child->GetNext();
    };
}


void FRStyleManager::assignWordStyle(wxStyledTextCtrl* text, FRStyle* style)
{
    //if (style->getbgColour() & COLORSTYLE_BACKGROUND){
    text->StyleSetBackground(style->getStyleID(), style->getbgColor());
    //}

    //if (style->getfgColour() & COLORSTYLE_FOREGROUND) {
    text->StyleSetForeground(style->getStyleID(), style->getfgColor());
    //}


    text->StyleSetCase(style->getStyleID(), style->getCaseVisible());
    double size = style->getFontSize() == 0 ? getGlobalStyle()->getFontSize() : style->getFontSize();
    wxFontInfo fontInfo(size);

    wxString fontName = style->getFontName().IsEmpty() ? getGlobalStyle()->getFontName() : style->getFontName();
    fontInfo.FaceName(fontName);

    if (style->getFontStyle() != STYLE_NOT_USED) {
        fontInfo.Bold(style->getFontStyle() & FONTSTYLE_BOLD);
        fontInfo.Italic(style->getFontStyle() & FONTSTYLE_ITALIC);
        fontInfo.Underlined(style->getFontStyle() & FONTSTYLE_UNDERLINE);
    }

    wxFont font(fontInfo);

    text->StyleSetFont(style->getStyleID(), font);

    if (style->getCaseVisible() != 0) {
        text->StyleSetCase(style->getStyleID(), style->getCaseVisible());
    }

}

FRStyleManager::FRStyleManager()
{
    loadConfig();
    globalStylerM = new FRStyler();

    loadStyle();
}


FRStyle* FRStyleManager::getStyleByName(wxString styleName)
{
    return globalStylerM->getStyleByName(styleName);
}

wxFileName FRStyleManager::getFileName()
{
    return !getSytyleActive() ? getFileNamePrimary() : getFileNameSecondary(); /*fileNameM */;
}

void FRStyleManager::setFileName(wxFileName fileName)
{
    /*if (fileName.FileExists())
        fileNameM = fileName;
    else
        fileNameM = wxFileName(config().getXmlStylesPath(), _default + ".xml");*/
    if (!getSytyleActive())
        setFileNamePrimary(fileName);
    else
        setFileNameSecondary(fileName);
}

void FRStyleManager::setFileNamePrimary(wxFileName fileName)
{
    if (fileName.FileExists())
        fileNamePrimaryM = fileName;
    else
        fileNamePrimaryM = wxFileName(config().getXmlStylesPath(), _default + ".xml");
}

void FRStyleManager::setFileNameSecondary(wxFileName fileName)
{
    if (fileName.FileExists())
        fileNameSecondaryM = fileName;
    else
        fileNameSecondaryM = wxFileName(config().getXmlStylesPath(), _default + ".xml");
}

void FRStyleManager::setStyleActive(int style)
{
    styleActiveM = style;

    (styleActiveM == 0) ? setFileName(getFileNamePrimary()) : setFileName(getFileNameSecondary());

}

void FRStyleManager::assignGlobal(wxStyledTextCtrl* text)
{
    //text->StyleClearAll();

    for (int i = 0; i < globalStylerM->getNbStyler(); i++) {
        FRStyle* style = globalStylerM->getStyle(i);
        //if (style->getStyleID() != 0) {
        assignWordStyle(text, style);
        //}
        if (style->getStyleDesc() == "Global override") {
            //globalStyleM = style;
            text->StyleResetDefault();
            text->SetBackgroundColour(style->getbgColor());
            text->SetForegroundColour(style->getfgColor());

            assignWordStyle(text, style);
        }
        if (style->getStyleDesc() == "Default Style") {
            //defaultStyleM = style;
        }
        if (style->getStyleDesc() == "Caret colour") {
            text->SetCaretForeground(style->getfgColor());
            //text->SetCaret()
        }
        if (style->getStyleDesc() == "Mark colour") {
            //text->setbackgroun
        }
        if (style->getStyleDesc() == "Selected text colour") {
            text->SetSelBackground(true, style->getbgColor());
            //text->SetSelForeground(true, style->getfgColor());
        }
        if (style->getStyleDesc() == "Edge colour") {
            text->SetEdgeColour(style->getfgColor());
        }
        if (style->getStyleDesc() == "Bookmark margin") {
            //text->SetMarginBackground()
        }
        if (style->getStyleDesc() == "Fold") {}
        if (style->getStyleDesc() == "Fold active") {}
        if (style->getStyleDesc() == "Fold margin") {
            text->SetFoldMarginColour(true, style->getbgColor());
            text->SetFoldMarginHiColour(true, style->getfgColor());
        }
        if (style->getStyleDesc() == "White space symbol") {
            text->SetWhitespaceForeground(true, style->getfgColor());
            text->SetWhitespaceBackground(true, style->getbgColor());
        }
        if (style->getStyleDesc() == "Active tab focused indicator") {}
        if (style->getStyleDesc() == "Active tab unfocused indicator") {}
        if (style->getStyleDesc() == "Active tab text") {}
        if (style->getStyleDesc() == "Inactive tabs") {}
        if (style->getStyleDesc() == "URL hovered") {}
        if (style->getStyleDesc() == "Current line background colour") {
            text->SetCaretLineBackground(style->getbgColor());
            //text->SetCaretForeground(style->getfgColor());
        }
    }

}

void FRStyleManager::assignLexer(wxStyledTextCtrl* text)
{
    //FRLexerStyler* lexer= lexerStylesM.getLexerStylerByName("sql");    lexerStylesM.
    if (stylersM.getNbLexerStyler() > 0) {
        int index = stylersM.getStylerIndexByName("sql");
        FRStyler* lexer = stylersM.getLexerFromIndex(index);
        if (lexer) {
            int max = lexer->getNbStyler();
            for (int i = 0; i < max; i++) {
                assignWordStyle(text, lexer->getStyle(i));
            }
        }
    }
    FRStyle* style = globalStylerM->getStyleByName("Line number margin");
    if (style) {
        assignWordStyle(text, style);
    }
}

void FRStyleManager::assignMargin(wxStyledTextCtrl* text)
{
    FRStyle* styleFold = getGlobalStyler()->getStyleByName("Fold");
    FRStyle* styleFoldMargin = getGlobalStyler()->getStyleByName("Fold margin");
    FRStyle* styleFoldActive = getGlobalStyler()->getStyleByName("Fold active");

    text->SetProperty(wxT("fold"), wxT("1"));
    text->SetProperty(wxT("fold.comment"), wxT("1"));
    text->SetProperty(wxT("fold.sql.only.begin"), wxT("1"));
    text->SetFoldFlags(wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);
    text->SetAutomaticFold(wxSTC_AUTOMATICFOLD_SHOW | wxSTC_AUTOMATICFOLD_CLICK | wxSTC_AUTOMATICFOLD_CHANGE);

    text->SetMarginWidth(FR_FOLDMARGIN, 14);
    text->SetMarginMask(FR_FOLDMARGIN, wxSTC_MASK_FOLDERS);
    text->SetFoldMarginColour(true, styleFoldMargin->getbgColor());
    text->SetFoldMarginHiColour(true, styleFoldMargin->getfgColor());


    text->MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUSCONNECTED);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDEREND, styleFold->getfgColor());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDEREND, styleFold->getbgColor());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDEREND, styleFoldActive->getfgColor());

    text->MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPENMID, styleFold->getfgColor());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPENMID, styleFold->getbgColor());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDEROPENMID, styleFoldActive->getfgColor());

    text->MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDERMIDTAIL, styleFold->getfgColor());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDERMIDTAIL, styleFold->getbgColor());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDERMIDTAIL, styleFoldActive->getfgColor());

    text->MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_LCORNER);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDERTAIL, styleFold->getfgColor());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDERTAIL, styleFold->getbgColor());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDERTAIL, styleFoldActive->getfgColor());

    text->MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_VLINE);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDERSUB, styleFold->getfgColor());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDERSUB, styleFold->getbgColor());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDERSUB, styleFoldActive->getfgColor());

    text->MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDER, styleFold->getfgColor());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDER, styleFold->getbgColor());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDER, styleFoldActive->getfgColor());

    text->MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPEN, styleFold->getfgColor());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPEN, styleFold->getbgColor());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDEROPEN, styleFoldActive->getfgColor());

    //Turn the fold markers red when the caret is a line in the group (optional)
    text->MarkerEnableHighlight(true);

    //The margin will only respond to clicks if it set sensitive.  Also, connect
    //the event handler that will do the collapsing/restoring
    text->SetMarginSensitive(FR_FOLDMARGIN, true);
    //m_activeSTC->Bind(wxEVT_STC_MARGINCLICK, &myFrame::onMarginClick, this);
    //m_activeSTC->Bind(wxEVT_STC_STYLENEEDED, &myFrame::OnStyleNeeded, this);

    //set color for G-Code highlighting
    //text->StyleSetForeground(19, m_GCodecolor);


}

void FRStyleManager::loadConfig()
{

    wxString fileName = config().get(_PRYMARY, _default);
    if (fileName.IsEmpty()) {
        fileName =_default;
    }
    wxFileName file = wxFileName(config().getXmlStylesPath(), fileName + ".xml");
    setFileNamePrimary(file);

    fileName = config().get(_SECONDARY, _default);
    if (fileName.IsEmpty()) {
        fileName = _default;
    }
    file = wxFileName(config().getXmlStylesPath(), fileName + ".xml");
    setFileNameSecondary(file);

    int style = config().get(_STYLEACTIVE, 0);
    setStyleActive(style);

}

void FRStyleManager::loadStyle()
{
    wxXmlDocument xmlDoc;
    xmlDoc.Load(getFileName().GetFullPath());
    if (xmlDoc.IsOk()) {
        cleanStyles();

        wxXmlNode* xmlNode = xmlDoc.GetRoot();
        if (xmlNode->GetType() == wxXML_ELEMENT_NODE && xmlNode->GetName() == "Flamerobin") {

            wxXmlNode* child = xmlNode->GetChildren();
            while (child) {
                if (!child)
                    break;
                if (child->GetType() == wxXML_ELEMENT_NODE && child->GetName() == "LexerStyles") {
                    loadLexerStyles(child);
                }
                if (child->GetType() == wxXML_ELEMENT_NODE && child->GetName() == "GlobalStyles") {
                    loadGlobalStyles(child);
                }
                child = child->GetNext();
            }
        }
    }
    notifyObservers();
}

void FRStyleManager::saveStyle()
{
    wxXmlDocument xmlDoc;
    xmlDoc.Load(getFileName().GetFullPath());
    if (xmlDoc.IsOk()) {

        wxXmlNode* xmlNode = xmlDoc.GetRoot();
        if (xmlNode->GetType() == wxXML_ELEMENT_NODE && xmlNode->GetName() == "Flamerobin") {

            wxXmlNode* child = xmlNode->GetChildren();
            while (child) {

                if (child->GetName() == "GlobalStyles")
                    getGlobalStyler()->write2Element(child);

                if (child->GetName() == "LexerStyles")
                    getLexerStylers().write2Element(child);

                child = child->GetNext();
            }
        }

        xmlDoc.Save(getFileName().GetFullPath(), 4);
    }
}

void FRStyleManager::cleanStyles()
{
    if(globalStylerM)
        globalStylerM->clear();
    //if (stylersM)
        stylersM.clear();
}

void FRStyleManager::saveLexerStyles(wxXmlNode* node)
{
}

void FRStyleManager::saveGlobalStyles(wxXmlNode* node)
{
}
