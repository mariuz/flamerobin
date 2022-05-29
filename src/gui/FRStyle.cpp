/*
  Copyright (c) 2004-2021 The FlameRobin Development Team

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

#include "wx/filename.h"
#include "wx/xml/xml.h"

#include "FRStyle.h"



FRStyle::FRStyle()
    :styleIDM(-1), styleDescM(""), /*fgColourM(wxBLACK), bgColourM(wxWHITE),*/ colourStyleM(COLORSTYLE_ALL),
    fontNameM(""), fontStyleM(FONTSTYLE_NONE), fontSizeM(STYLE_NOT_USED), nestingM(FONTSTYLE_NONE), 
    keywordClassM(STYLE_NOT_USED), keywordsM("")
{
}

FRStyle::FRStyle(const FRStyle& style)
{
    styleIDM = style.styleIDM;
    styleDescM = style.styleDescM;
    fgColourM = style.fgColourM;
    bgColourM = style.bgColourM;
    colourStyleM = style.colourStyleM;
    fontNameM = style.fontNameM;
    fontSizeM = style.fontSizeM;
    fontStyleM = style.fontStyleM;
    keywordClassM = style.keywordClassM;
    nestingM = style.nestingM;
    keywordsM = style.keywordsM;
}

FRStyle::~FRStyle()
{
}

FRStyle& FRStyle::operator=(const FRStyle& style)
{
    styleIDM = style.styleIDM;
    styleDescM = style.styleDescM;
    fgColourM = style.fgColourM;
    bgColourM = style.bgColourM;
    colourStyleM = style.colourStyleM;
    fontNameM = style.fontNameM;
    fontSizeM = style.fontSizeM;
    fontStyleM = style.fontStyleM;
    keywordClassM = style.keywordClassM;
    nestingM = style.nestingM;
    keywordsM = style.keywordsM;

    return *this;
}

wxFont FRStyle::getFont()
{
    wxFontInfo fontInfo(getFontSize());

    if (!getFontName().IsEmpty()) 
        fontInfo.FaceName(getFontName());
    
    fontInfo.Bold(getFontStyle() & FONTSTYLE_BOLD);
    fontInfo.Italic(getFontStyle() & FONTSTYLE_ITALIC);
    fontInfo.Underlined(getFontStyle() & FONTSTYLE_UNDERLINE);

    wxFont font(fontInfo);

    return font;
}

FRStyles::FRStyles() 
{
    styleVectorM.clear();
}

FRStyles& FRStyles::operator=(const FRStyles& sa)
{
    if (this != &sa)
    {
        this->styleVectorM = sa.styleVectorM;
    }
    return *this;
}

int FRStyles::getStyleIndexByID(int id)
{
    std::vector<FRStyle*>::iterator it;
    it = std::find_if(styleVectorM.begin(), styleVectorM.end(), [id](FRStyle* style)->bool {return style->getStyleID() == id; });
   
    if (it == styleVectorM.end())
        return -1;
    else
        return std::distance(styleVectorM.begin(), it);
}

void FRStyles::addStyler(int styleID, wxXmlNode* styleNode)
{
    /*bool isUser = styleID >> 16 == L_USER;
    if (isUser)
    {
        styleID = (styleID & 0xFFFF);
        index = styleID;
        if (index >= SCE_USER_STYLE_TOTAL_STYLES || _styleArray[index]._styleID != -1)
            return;
    }*/

    FRStyle* newStyle = new FRStyle();
    newStyle->setStyleID(styleID);

    if (styleNode)
    {
        wxXmlNode* element = styleNode;

        wxString str = element->GetAttribute("name");
        if (!str.IsEmpty())
        {
            /*if (isUser)
                _styleArray[index]._styleDesc = globalMappper().styleNameMapper[index].c_str();
            else*/
            newStyle->setStyleDesc(str);
        }

        str = element->GetAttribute("fgColor");
        if (!str.IsEmpty())
        {
           
            long result; 
            str.ToLong(&result, 16);
            newStyle->setfgColour(wxColour((_RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000)));

        }

        str = element->GetAttribute("bgColor");
        if (!str.IsEmpty())
        {
            long result;
            str.ToLong(&result, 16);
            newStyle->setbgColour(wxColour((_RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000)));
        }

        str = element->GetAttribute("colorStyle");
        if (!str.IsEmpty())
        {
            long temp;
            str.ToLong(&temp, 10);
            newStyle->setColourStyle(temp);
        }

        str = element->GetAttribute("fontName");
        newStyle->setFontName(str);

        str = element->GetAttribute("fontStyle");
        if (!str.IsEmpty())
        {
            long temp;
            str.ToLong(&temp, 10);
            newStyle->setFontStyle(temp);
        }

        str = element->GetAttribute("fontSize");
        if (!str.IsEmpty())
        {
            long temp;
            str.ToLong(&temp, 10);
            newStyle->setFontSize(temp);
        }
        
        str = element->GetAttribute("caseVisible");
        if (!str.IsEmpty())
        {
            long temp;
            str.ToLong(&temp, 10);
            newStyle->setCaseVisible(temp);
        }

        str = element->GetAttribute("nesting");
        if (!str.IsEmpty())
        {
            long temp;
            str.ToLong(&temp, 10);
            newStyle->setNesting(temp);
        }

        /*str = element->GetAttribute("keywordClass");
        if (!str.IsEmpty())
        {
            newStyle->setKeywordClass(getKwClassFromName(str));
        }*/

        wxXmlNode* v = styleNode->GetChildren();
        if (v)
        {
            newStyle->setKeywords(v->GetContent());
        }
    }

    styleVectorM.push_back(newStyle);
}

void FRStyles::addStyler(int styleID, const wxString styleName)
{
    FRStyle* newStyle = new FRStyle();
    newStyle->setStyleID(styleID);
    newStyle->setStyleDesc(styleName);
    newStyle->setfgColour(wxBLACK->GetRGB());
    newStyle->setbgColour(wxWHITE->GetRGB());

    styleVectorM.push_back(newStyle);

}

int FRStyles::getStyleIndexByName(wxString styleName) 
{
    if (styleName.IsEmpty())
        return -1;

    std::vector<FRStyle*>::iterator it;
    it = std::find_if(styleVectorM.begin(), styleVectorM.end(), [styleName](FRStyle* style)->bool {return style->getStyleDesc() == styleName; });

    if (it == styleVectorM.end())
        return -1;
    else
        return std::distance(styleVectorM.begin(), it);
}

FRStyle* FRStyles::getStyle(size_t index)
{
    assert(index < wxSTC_STYLE_MAX);
    return styleVectorM[index];
}

FRStyle* FRStyles::getStyleByName(wxString styleName)
{
    int i = getStyleIndexByName(styleName);
    if (i != -1)
        return getStyle(i);
    else
        return nullptr;
}


FRStyler::FRStyler()
    :FRStyles()
{
    
}

FRStyler& FRStyler::operator=(const FRStyler& ls)
{
    if (this != &ls)
    {
        *(static_cast<FRStyles*>(this)) = ls;
        this->setStylerName(ls.getStylerName());
        this->setStylerDesc(ls.getStylerDesc());
        this->setStylerUserExt(ls.getStylerUserExt());
    }
    return *this;
}


FRStylers::FRStylers()
{
    stylerVectorM.clear();
}

FRStylers& FRStylers::operator=(const FRStylers& lsa)
{
    if (this != &lsa)
    {
        this->stylerVectorM = lsa.stylerVectorM;
    }
    return *this;
}

FRStyler* FRStylers::getStylerByName(wxString lexerName)
{
    if (!lexerName) 
        return NULL;

    std::vector<FRStyler*>::iterator it;
    it = std::find_if(stylerVectorM.begin(), stylerVectorM.end(), [lexerName](FRStyler* style)->bool {return style->getStylerName() == lexerName; });

    if (it == stylerVectorM.end())
        return NULL;
    else
        return (FRStyler*)*it;
}

int FRStylers::getStylerIndexByName(wxString lexerName)
{
    if (lexerName.IsEmpty())
        return -1;

    std::vector<FRStyler*>::iterator it;
    it = std::find_if(stylerVectorM.begin(), stylerVectorM.end(), [lexerName](FRStyler* style)->bool {return style->getStylerName() == lexerName; });
    if (it == stylerVectorM.end())
        return -1;
    else
        return std::distance(stylerVectorM.begin(), it);
}

FRStyler* FRStylers::getStylerByDesc(wxString lexerDesc)
{
    if (!lexerDesc)
        return NULL;

    std::vector<FRStyler*>::iterator it;
    it = std::find_if(stylerVectorM.begin(), stylerVectorM.end(), [lexerDesc](FRStyler* style)->bool {return style->getStylerDesc() == lexerDesc; });

    if (it == stylerVectorM.end())
        return NULL;
    else
        return (FRStyler*)*it;
}

int FRStylers::getStylerIndexByDesc(wxString lexerDesc)
{
    if (lexerDesc.IsEmpty())
        return -1;

    std::vector<FRStyler*>::iterator it;
    it = std::find_if(stylerVectorM.begin(), stylerVectorM.end(), [lexerDesc](FRStyler* style)->bool {return style->getStylerDesc() == lexerDesc; });
    if (it == stylerVectorM.end())
        return -1;
    else
        return std::distance(stylerVectorM.begin(), it);
}

void FRStylers::addStyler(wxString lexerName, wxString lexerDesc, wxString lexerUserExt, wxXmlNode* lexerNode)
{
    FRStyler* ls = new FRStyler();
    ls->setStylerName(lexerName);

    if (!lexerDesc.IsEmpty())
        ls->setStylerDesc(lexerDesc);

    if (!lexerUserExt.IsEmpty())
        ls->setStylerUserExt(lexerUserExt);

    wxXmlNode* child = lexerNode->GetChildren();
    while (child) {
        if (child->GetType() == wxXML_ELEMENT_NODE &&  child->GetName() == "WordsStyle") {
            long styleID = -1;
            if (child->GetAttribute("styleID").ToLong(&styleID)) {
                ls->addStyler(styleID, child);
            }
        }
        child = child->GetNext();
    }

    stylerVectorM.push_back(ls);
}

void FRStylers::clear()
{
    stylerVectorM.clear();    
}

FRStyleManager& stylerManager()
{
    const wxString STYLE = "StyleTheme";
    const wxString def = "stylers";

    wxFileName file = wxFileName(config().getXmlStylesPath(), config().get(STYLE, def) + ".xml");
    static FRStyleManager s(file);
    return s;
}

void FRStyleManager::loadLexerStyles(wxXmlNode* node)
{
    stylersM.clear();

    wxXmlNode* child = node->GetChildren();
    
    while (child) {
        if (child->GetType() == wxXML_ELEMENT_NODE && child->GetName() == "LexerType" 
            //&& child->GetAttribute("name")=="sql"
            ) 
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
        text->StyleSetBackground(style->getStyleID(), style->getbgColour());
    //}

    //if (style->getfgColour() & COLORSTYLE_FOREGROUND) {
        text->StyleSetForeground(style->getStyleID(), style->getfgColour());
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

FRStyleManager::FRStyleManager(wxFileName style)
{
    fileNameM = style;
    globalStylerM = new FRStyler();

    loadStyle();
}


FRStyle* FRStyleManager::getStyleByName(wxString styleName)
{
    return globalStylerM->getStyleByName(styleName);
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
            text->SetBackgroundColour(style->getbgColour());
            text->SetForegroundColour(style->getfgColour());
            assignWordStyle(text, style);
        }
        if (style->getStyleDesc() == "Default Style") {
            //defaultStyleM = style;
        }
        if (style->getStyleDesc() == "Mark colour") {}
        if (style->getStyleDesc() == "Selected text colour") {
            text->SetSelBackground(true, style->getbgColour());
            //text->SetSelForeground(true, style->getfgColor());
        }
        if (style->getStyleDesc() == "Edge colour") {
            text->SetEdgeColour(style->getfgColour());
        }
        if (style->getStyleDesc() == "Bookmark margin" ) {
            //text->SetMarginBackground()
        }
        if (style->getStyleDesc() == "Fold") {}
        if (style->getStyleDesc() == "Fold active") {}
        if (style->getStyleDesc() == "Fold margin") {
            text->SetFoldMarginColour(true, style->getbgColour());
            text->SetFoldMarginHiColour(true, style->getfgColour());
        }
        if (style->getStyleDesc() == "White space symbol") {
            text->SetWhitespaceForeground(true, style->getfgColour());
            text->SetWhitespaceBackground(true, style->getbgColour());
        }
        if (style->getStyleDesc() == "Active tab focused indicator") {}
        if (style->getStyleDesc() == "Active tab unfocused indicator") {}
        if (style->getStyleDesc() == "Active tab text") {}
        if (style->getStyleDesc() == "Inactive tabs") {}
        if (style->getStyleDesc() == "URL hovered") {}
        if (style->getStyleDesc() == "Current line background colour") {
            text->SetCaretLineBackground(style->getbgColour());
            text->SetCaretForeground(style->getfgColour());
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

void FRStyleManager::assignFold(wxStyledTextCtrl* text)
{
    FRStyle* styleFoldMargin = getGlobalStyler()->getStyleByName("Fold margin");
    FRStyle* styleFold = getGlobalStyler()->getStyleByName("Fold");
    FRStyle* styleFoldActive = getGlobalStyler()->getStyleByName("Fold active");

    text->SetProperty(wxT("fold"), wxT("1"));
    text->SetProperty(wxT("fold.comment"), wxT("1"));
    text->SetProperty(wxT("fold.sql.only.begin"), wxT("1"));
    text->SetFoldFlags(wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);
    text->SetAutomaticFold(wxSTC_AUTOMATICFOLD_SHOW | wxSTC_AUTOMATICFOLD_CLICK | wxSTC_AUTOMATICFOLD_CHANGE);

    text->SetMarginWidth(FR_FOLDMARGIN, 14);
    text->SetMarginMask(FR_FOLDMARGIN, wxSTC_MASK_FOLDERS);
    text->SetFoldMarginColour(true, styleFoldMargin->getbgColour());
    text->SetFoldMarginHiColour(true, styleFoldMargin->getfgColour());


    text->MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUSCONNECTED);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDEREND, styleFold->getfgColour());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDEREND, styleFold->getbgColour());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDEREND, styleFoldActive->getfgColour());

    text->MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPENMID, styleFold->getfgColour());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPENMID, styleFold->getbgColour());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDEROPENMID, styleFoldActive->getfgColour());

    text->MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDERMIDTAIL, styleFold->getfgColour());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDERMIDTAIL, styleFold->getbgColour());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDERMIDTAIL, styleFoldActive->getfgColour());

    text->MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_LCORNER);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDERTAIL, styleFold->getfgColour());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDERTAIL, styleFold->getbgColour());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDERTAIL, styleFoldActive->getfgColour());

    text->MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_VLINE);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDERSUB, styleFold->getfgColour());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDERSUB, styleFold->getbgColour());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDERSUB, styleFoldActive->getfgColour());

    text->MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDER, styleFold->getfgColour());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDER, styleFold->getbgColour());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDER, styleFoldActive->getfgColour());

    text->MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS);
    text->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPEN, styleFold->getfgColour());
    text->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPEN, styleFold->getbgColour());
    text->MarkerSetBackgroundSelected(wxSTC_MARKNUM_FOLDEROPEN, styleFoldActive->getfgColour());

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
    const wxString STYLE = "StyleTheme";
    const wxString def = "stylers";

    wxFileName file = wxFileName(config().getXmlStylesPath(), config().get(STYLE, def) + ".xml");
    setfileName(file);
    
    loadStyle();
}

void FRStyleManager::loadStyle()
{

    wxXmlDocument xmlDoc;
    xmlDoc.Load(fileNameM.GetFullPath());
    if (xmlDoc.IsOk()) {
        
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

}

void FRStyleManager::saveStyle()
{
    //TODO: Implemented
}
