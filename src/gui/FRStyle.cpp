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
    :styleIDM(-1), styleDescM(""), fgColorM(STYLE_NOT_USED), bgColorM(STYLE_NOT_USED), colorStyleM(COLORSTYLE_ALL),
    fontNameM(""), fontStyleM(FONTSTYLE_NONE), fontSizeM(STYLE_NOT_USED), nestingM(FONTSTYLE_NONE), 
    keywordClassM(STYLE_NOT_USED), keywordsM("")
{
}

FRStyle::FRStyle(const FRStyle& style)
{
    styleIDM = style.styleIDM;
    styleDescM = style.styleDescM;
    fgColorM = style.fgColorM;
    bgColorM = style.bgColorM;
    colorStyleM = style.colorStyleM;
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
    fgColorM = style.fgColorM;
    bgColorM = style.bgColorM;
    colorStyleM = style.colorStyleM;
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
            newStyle->setfgColor((_RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000));

        }

        str = element->GetAttribute("bgColor");
        if (!str.IsEmpty())
        {
            long result;
            str.ToLong(&result, 16);
            newStyle->setbgColor((_RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000));
        }

        str = element->GetAttribute("colorStyle");
        if (!str.IsEmpty())
        {
            long temp;
            str.ToLong(&temp, 10);
            newStyle->setColorStyle(temp);
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
    newStyle->setfgColor(wxBLACK->GetRGB());
    newStyle->setbgColor(wxWHITE->GetRGB());

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


FRLexerStyler::FRLexerStyler()
    :FRStyles()
{
    
}

FRLexerStyler& FRLexerStyler::operator=(const FRLexerStyler& ls)
{
    if (this != &ls)
    {
        *(static_cast<FRStyles*>(this)) = ls;
        this->setLexerName(ls.getLexerName());
        this->setLexerDesc(ls.getLexerDesc());
        this->setLexerUserExt(ls.getLexerUserExt());
    }
    return *this;
}


FRLexerStylers::FRLexerStylers()
{
    lexerStylerVectorM.clear();
}

FRLexerStylers& FRLexerStylers::operator=(const FRLexerStylers& lsa)
{
    if (this != &lsa)
    {
        this->lexerStylerVectorM = lsa.lexerStylerVectorM;
    }
    return *this;
}

FRLexerStyler* FRLexerStylers::getLexerStylerByName(wxString lexerName)
{
    if (!lexerName) 
        return NULL;

    std::vector<FRLexerStyler*>::iterator it;
    it = std::find_if(lexerStylerVectorM.begin(), lexerStylerVectorM.end(), [lexerName](FRLexerStyler* style)->bool {return style->getLexerName() == lexerName; });

    if (it == lexerStylerVectorM.end())
        return NULL;
    else
        return (FRLexerStyler*)&*it;

}

int FRLexerStylers::getLexerStylerIndexByName(wxString lexerName)
{
    if (lexerName.IsEmpty())
        return -1;

    std::vector<FRLexerStyler*>::iterator it;
    it = std::find_if(lexerStylerVectorM.begin(), lexerStylerVectorM.end(), [lexerName](FRLexerStyler* style)->bool {return style->getLexerName() == lexerName; });
    if (it == lexerStylerVectorM.end())
        return -1;
    else
        return std::distance(lexerStylerVectorM.begin(), it);
}

void FRLexerStylers::addLexerStyler(wxString lexerName, wxString lexerDesc, wxString lexerUserExt, wxXmlNode* lexerNode)
{
    FRLexerStyler* ls = new FRLexerStyler();
    ls->setLexerName(lexerName);

    if (!lexerDesc.IsEmpty())
        ls->setLexerDesc(lexerDesc);

    if (!lexerUserExt.IsEmpty())
        ls->setLexerUserExt(lexerUserExt);

    wxXmlNode* child = lexerNode->GetChildren();
    while (child) {
        if (child->GetName() == "WordsStyle") {
            long styleID = -1;
            if (child->GetAttribute("styleID").ToLong(&styleID)) {
                ls->addStyler(styleID, child);
            }
        }
        child = child->GetNext();
    }

    lexerStylerVectorM.push_back(ls);
}

void FRLexerStylers::eraseAll()
{
    //std::for_each(lexerStylerVectorM.begin(), lexerStylerVectorM.end(), [] {lexerStylerVectorM[].clear();});
    lexerStylerVectorM.clear();    
}

FRStyleManager& stylerManager()
{
    const wxString STYLE = "Style";
    const wxString def = "stylers.xml";

    wxFileName file = wxFileName(config().getXmlStylesPath(), config().get(STYLE, def));
    static FRStyleManager s(file);
    return s;
}

void FRStyleManager::loadLexerStyles(wxXmlNode* node)
{
    wxXmlNode* child = node->GetChildren();

    while (child) {
        if (child->GetName() == "LexerType" /*&& child->GetAttribute("name")=="sql"*/) {
            lexerStylesM.addLexerStyler(child->GetAttribute("name"), child->GetAttribute("desc"), child->GetAttribute("ext"), child);
        }
        child = child->GetNext();
    };

}

void FRStyleManager::loadGlobalStyles(wxXmlNode* node)
{
    wxXmlNode* child = node->GetChildren();

    while (child) {
        if (child->GetName() == "WidgetStyle") {
            long styleID = -1;
            if (child->GetAttribute("styleID").ToLong(&styleID)) {
                globalStylesM.addStyler(styleID, child);
            }
        }
        child = child->GetNext();
    };
  
}

void FRStyleManager::assignWordStyle(wxStyledTextCtrl* text, FRStyle* style)
{
    if (style->getbgColor() != 0){ 
        text->StyleSetBackground(style->getStyleID(), style->getbgColor());
    }

    if (style->getfgColor() != 0) {
        text->StyleSetForeground(style->getStyleID(), style->getfgColor());
    }
        
    double size = style->getFontSize() == 0 ? globalStyleM->getFontSize() : style->getFontSize();
    wxFontInfo fontInfo(size);
        
    wxString fontName = style->getFontName().IsEmpty() ? globalStyleM->getFontName() : style->getFontName();
    fontInfo.FaceName(fontName);
        
    fontInfo.Bold(style->getFontStyle() & FONTSTYLE_BOLD);
    fontInfo.Italic(style->getFontStyle() & FONTSTYLE_ITALIC);
    fontInfo.Underlined(style->getFontStyle() & FONTSTYLE_UNDERLINE);

    wxFont font(fontInfo);

    text->StyleSetFont(style->getStyleID(), font);

    if (style->getCaseVisible() != 0) {
        text->StyleSetCase(style->getStyleID(), style->getCaseVisible());
    }

}

FRStyleManager::FRStyleManager(wxFileName style)
{
    wxXmlDocument xmlDoc;
    xmlDoc.Load(style.GetFullPath());
    if (xmlDoc.IsOk()) {
        wxXmlNode* xmlNode = xmlDoc.GetRoot();
        if (xmlNode->GetName() == "Flamerobin") {
            
            wxXmlNode* child = xmlNode->GetChildren();
            while (child) {
                if (!child)
                    break;
                if (child->GetName() == "LexerStyles") {
                    loadLexerStyles(child);
                }
                if (child->GetName() == "GlobalStyles") {
                    loadGlobalStyles(child);
                }
                child = child->GetNext();
            }
        }
    }

}


FRStyle* FRStyleManager::getStyleByName(wxString styleName)
{
    return globalStylesM.getStyleByName(styleName);
}

void FRStyleManager::assignGlobal(wxStyledTextCtrl* text)
{
    //text->StyleClearAll();

    for (int i = 0; i < globalStylesM.getNbStyler(); i++) {
        FRStyle* style = globalStylesM.getStyle(i);
        if (style->getStyleID() != 0) {
            assignWordStyle(text, style);
        }
        if (style->getStyleDesc() == "Global override") {
            globalStyleM = style;
            text->StyleResetDefault();
            text->SetBackgroundColour(style->getbgColor());
            text->SetForegroundColour(style->getfgColor());
            assignWordStyle(text, style);
        }
        if (style->getStyleDesc() == "Default Style") {
            defaultStyleM = style;
        }
        if (style->getStyleDesc() == "Mark colour") {}
        if (style->getStyleDesc() == "Selected text colour") {
            text->SetSelBackground(true, style->getbgColor());
            //text->SetSelForeground(true, style->getfgColor());
        }
        if (style->getStyleDesc() == "Edge colour") {
            text->SetEdgeColour(style->getfgColor());
        }
        if (style->getStyleDesc() == "Bookmark margin") {
            //text->SetMarginBackground(wxSTC_MARGIN_SYMBOL,style->getbgColor());
            //text->SetMarginBackground(wxSTC_MARGIN_NUMBER, style->getbgColor());

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
            text->SetCaretForeground(style->getfgColor());
        }
    }

}

void FRStyleManager::assignLexer(wxStyledTextCtrl* text)
{
    //FRLexerStyler* lexer= lexerStylesM.getLexerStylerByName("sql");    lexerStylesM.
    if (lexerStylesM.getNbLexerStyler() > 0) {
        int index = lexerStylesM.getLexerStylerIndexByName("sql");
        FRLexerStyler* lexer = lexerStylesM.getLexerFromIndex(index);
        if (lexer) {
            int max = lexer->getNbStyler();
            for (int i = 0; i < max; i++) {
                assignWordStyle(text, lexer->getStyle(i));
            }
        }
    }
}
