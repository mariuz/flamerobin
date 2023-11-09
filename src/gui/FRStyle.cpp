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


int RGB2int(_COLORREF color)
{
    return (((((_DWORD)color) & 0x0000FF) << 16) | ((((_DWORD)color) & 0x00FF00)) | ((((_DWORD)color) & 0xFF0000) >> 16));
}

FRStyle::FRStyle()
    :styleIDM(STYLE_NOT_USED), styleDescM(""), 
    fgColorM(STYLE_NOT_USED), bgColorM(STYLE_NOT_USED), colorStyleM(COLORSTYLE_ALL), 
    isFontEnabledM(false), fontNameM(""), fontStyleM(FONTSTYLE_NONE), fontSizeM(STYLE_NOT_USED), caseVisibleM(0),
    nestingM(FONTSTYLE_NONE), 
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
    
    isFontEnabledM = style.isFontEnabledM;
    fontNameM = style.fontNameM;
    fontSizeM = style.fontSizeM;
    fontStyleM = style.fontStyleM;
    caseVisibleM = style.caseVisibleM;
    
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

    isFontEnabledM = style.isFontEnabledM;
    fontNameM = style.fontNameM;
    fontSizeM = style.fontSizeM;
    fontStyleM = style.fontStyleM;
    caseVisibleM = style.caseVisibleM;

    keywordClassM = style.keywordClassM;

    nestingM = style.nestingM;
    keywordsM = style.keywordsM;

    return *this;
}

bool FRStyle::operator==(const FRStyle& style)
{
    return
        styleIDM == style.styleIDM &&
        styleDescM == style.styleDescM &&

        fgColorM == style.fgColorM &&
        bgColorM == style.bgColorM &&
        colorStyleM == style.colorStyleM &&

        isFontEnabledM == style.isFontEnabledM &&
        fontNameM == style.fontNameM &&
        fontSizeM == style.fontSizeM &&
        fontStyleM == style.fontStyleM &&
        caseVisibleM == style.caseVisibleM &&

        keywordClassM == style.keywordClassM &&

        nestingM == style.nestingM &&
        keywordsM == style.keywordsM
        ;
}

void FRStyle::write2Element(wxXmlNode* element)
{
    wxXmlAttribute* lAttribute = element->GetAttributes();
    while (lAttribute) {
        wxString lName = lAttribute->GetName();
        if (lName == "fgColor") {
            if (_HIBYTE(_HIWORD(fgColorM.GetRGB())) != 0xFF)
            {
                int rgbVal = RGB2int(fgColorM.GetRGB());
                //element->AddAttribute(TEXT("fgColor"), wxString::Format(wxT("%.6X"), rgbVal));
                lAttribute->SetValue(wxString::Format(wxT("%.6X"), rgbVal));
                
            }
        }
        if (lName == "bgColor") {
            if (_HIBYTE(_HIWORD(bgColorM.GetRGB())) != 0xFF)
            {
                int rgbVal = RGB2int(bgColorM.GetRGB());
                //element->AddAttribute(TEXT("bgColor"), wxString::Format(wxT("%.6X"), rgbVal));
                lAttribute->SetValue(wxString::Format(wxT("%.6X"), rgbVal));
            }
        }
        if (lName == "colorStyle") {
            if (colorStyleM != COLORSTYLE_ALL)
                lAttribute->SetValue(wxString::Format(wxT("%i"), colorStyleM));
        }
        if (lName == "fontName") {
            if (!fontNameM.empty())
            {
                wxString oldFontName = element->GetAttribute(wxT("fontName"));
                if (!oldFontName.IsEmpty() && oldFontName != fontNameM)
                    lAttribute->SetValue(fontNameM);
            }
        }
        if (lName == "fontSize") {
            if (fontSizeM != STYLE_NOT_USED)
            {
                if (!fontSizeM)
                    lAttribute->SetValue(wxT(""));
                else
                    lAttribute->SetValue(wxString::Format(wxT("%i"), fontSizeM));
            }
        }
        if (lName == "fontStyle") {
            if (fontStyleM != STYLE_NOT_USED)
                lAttribute->SetValue(wxString::Format(wxT("%i"), fontStyleM));
        }
        if (lName == "caseVisible") {
            if (caseVisibleM != STYLE_NOT_USED)
                lAttribute->SetValue(wxString::Format(wxT("%i"), caseVisibleM));
        }
        /*
        if (!keywordsM.empty())
        {
        }
        */

        lAttribute = lAttribute->GetNext();
    }
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

bool FRStyles::operator==(const FRStyles& /*sa*/)
{
    return false;
}

void FRStyles::write2Element(wxXmlNode* element)
{
    wxXmlNode* child = element->GetChildren();
    while (child) {
        if (child->GetType() == wxXML_ELEMENT_NODE && 
            (child->GetName() == "WidgetStyle") || (child->GetName() == "WordsStyle")) {
            wxString lName = child->GetAttribute("name");
            FRStyle* lStyle = getStyleByName(lName);
            if (lStyle) {
                lStyle->write2Element(child);
            }
        }
        child = child->GetNext();
    }
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
            //newStyle->setfgColour((_RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000));

            wxColour colour (result);
            colour.SetRGB((_RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000));
            newStyle->setfgColor(colour);

        }

        str = element->GetAttribute("bgColor");
        if (!str.IsEmpty())
        {
            long result;
            str.ToLong(&result, 16);
            //newStyle->setbgColour((_RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000));

            wxColour colour(result);
            colour.SetRGB((_RGB((result >> 16) & 0xFF, (result >> 8) & 0xFF, result & 0xFF)) | (result & 0xFF000000));
            newStyle->setbgColor(colour);

        }

        str = element->GetAttribute("colorStyle");
        if (!str.IsEmpty())
        {
            long temp;
            str.ToLong(&temp, 10);
            newStyle->setColorStyle(temp);
        }

        str = element->GetAttribute("fontName");
        if (!str.IsEmpty()) {
            newStyle->setFontName(str);
            newStyle->setisFontEnable(TRUE);
        }

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

void FRStylers::write2Element(wxXmlNode* element)
{
    wxXmlNode* child = element->GetChildren();
    while (child) {
        if (child->GetType() == wxXML_ELEMENT_NODE &&
            child->GetName() == "LexerType") {
            wxString lName = child->GetAttribute("name");
            FRStyles* lStyles = getStylerByName(lName);
            if (lStyles) {
                lStyles->write2Element(child);
            }
        }
        child = child->GetNext();
    }
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

FRStyleManager::FRStyleManager(wxFileName style)
{
    setfileName(style);
    globalStylerM = new FRStyler();

    loadStyle();
}


FRStyle* FRStyleManager::getStyleByName(wxString styleName)
{
    return globalStylerM->getStyleByName(styleName);
}

void FRStyleManager::setfileName(wxFileName fileName)
{
    if (fileName.FileExists())
        fileNameM = fileName;
    else
        fileNameM = wxFileName(config().getXmlStylesPath(), "stylers.xml");
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
        if (style->getStyleDesc() == "Bookmark margin" ) {
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
    const wxString STYLE = "StyleTheme";
    const wxString def = "stylers";
    wxString fileName = config().get(STYLE, def);
    if (fileName.IsEmpty()) {
        fileName = def;
    }

    wxFileName file = wxFileName(config().getXmlStylesPath(), fileName + ".xml");
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
    wxXmlDocument xmlDoc;
    xmlDoc.Load(fileNameM.GetFullPath());
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

        xmlDoc.Save(fileNameM.GetFullPath(), 4);
    }    
}
