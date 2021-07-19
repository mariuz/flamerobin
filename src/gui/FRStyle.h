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

#pragma once
#ifndef FR_STYLE_H
#define FR_STYLE_H

#include <vector>
#include <algorithm>

#include <wx/colour.h>
#include <wx/stc/stc.h>
#include <wx/xml/xml.h>

#include "config/Config.h"

typedef DWORD   COLORREF;

const int FONTSTYLE_NONE = 0;
const int FONTSTYLE_BOLD = 1;
const int FONTSTYLE_ITALIC = 2;
const int FONTSTYLE_UNDERLINE = 4;

const int STYLE_NOT_USED = 0;

const int COLORSTYLE_FOREGROUND = 0x01;
const int COLORSTYLE_BACKGROUND = 0x02;
const int COLORSTYLE_ALL = COLORSTYLE_FOREGROUND | COLORSTYLE_BACKGROUND;

const int MAX_LEXER_STYLE = 100;



class FRStyle
{
protected:
    int styleIDM;
    wxString styleDescM;

    COLORREF fgColorM;
    COLORREF bgColorM;
    int colorStyleM;

    wxString fontNameM;
    int fontStyleM;
    int fontSizeM;
    int caseVisibleM;
    int nestingM;

    int keywordClassM;
    wxString keywordsM;
public:
    FRStyle();
    FRStyle(const FRStyle& style);
    ~FRStyle();

    FRStyle& operator = (const FRStyle& style);


    int getStyleID() { return styleIDM; };
    void setStyleID(int id) { styleIDM = id; };
    
    wxString getStyleDesc() { return styleDescM; };
    void setStyleDesc(wxString name) { styleDescM = name; };


    COLORREF getfgColor() { return fgColorM; };
    void setfgColor(COLORREF color) { fgColorM = color; };
    
    COLORREF getbgColor() { return bgColorM; };
    void setbgColor(COLORREF color) { bgColorM = color; };
    
    int getColorStyle() { return colorStyleM; };
    void setColorStyle(int color) { colorStyleM = color; };



    wxString getFontName() { return fontNameM; };
    void setFontName(wxString name) { fontNameM = name; };
    
    int getFontStyle() { return fontStyleM; };
    void setFontStyle(int font) { fontStyleM = font; };
    
    int getFontSize() { return fontSizeM; };
    void setFontSize(int size) { fontSizeM = size; };

    int getCaseVisible() { return caseVisibleM; };
    void setCaseVisible(int caseVisible) { caseVisibleM = caseVisible; };

    int getNesting() { return nestingM; };
    void setNesting(int nesting) { nestingM = nesting; };

    int getKeywordClass() { return keywordClassM; };
    void setKeywordClass(int keyword) { keywordClassM = keyword; };
    
    wxString getKeywords() { return keywordsM; };
    void setKeywords(wxString keywords) { keywordsM = keywords; };

    wxFont getFont();
         
};


class FRStyles
{
    
protected:
    std::vector<FRStyle *> styleVectorM;
    
public:
    FRStyles();

    FRStyles& operator=(const FRStyles& sa);

    int getNbStyler() const { return styleVectorM.size(); };
    //void setNbStyler(int nb) { nbStylerM = nb; };

    FRStyle* getStyle(size_t index);
    FRStyle* getStyleByName(wxString styleName);

    bool hasEnoughSpace() { return (getNbStyler() < wxSTC_STYLE_MAX); };
    void addStyler(int styleID, wxXmlNode* styleNode);
    void addStyler(int styleID, const wxString styleName);

    int getStyleIndexByID(int id);

    int getStyleIndexByName(wxString styleName);
    void clear() { styleVectorM.clear(); };

};


class FRLexerStyler : public FRStyles
{
private:
    wxString lexerNameM;
    wxString lexerDescM;
    wxString lexerUserExtM;

public:
    FRLexerStyler();
    FRLexerStyler& operator=(const FRLexerStyler& ls);

    wxString getLexerName() const { return lexerNameM; };
    void setLexerName(wxString lexerName) { lexerNameM = lexerName; };

    void setLexerDesc(wxString lexerDesc) { lexerDescM = lexerDesc; };
    wxString getLexerDesc() const { return lexerDescM; };

    void setLexerUserExt(wxString lexerUserExt) { lexerUserExtM = lexerUserExt; };
    wxString getLexerUserExt() const { return lexerUserExtM; };

};


class FRLexerStylers
{
private:
    std::vector<FRLexerStyler*> lexerStylerVectorM;
protected:

public:
    FRLexerStylers();

    FRLexerStylers& operator=(const FRLexerStylers& lsa);

    int getNbLexerStyler() const { return lexerStylerVectorM.size(); };
    //void setNbLexerStyler(int nbLexer) { nbLexerStylerM = nbLexer; };
    bool hasEnoughSpace() { return (getNbLexerStyler() < MAX_LEXER_STYLE); };



    FRLexerStyler* getLexerFromIndex(int index) { return lexerStylerVectorM[index]; };

    wxString getLexerNameFromIndex(int index) const { return lexerStylerVectorM[index]->getLexerName(); };
    wxString getLexerDescFromIndex(int index) const { return lexerStylerVectorM[index]->getLexerDesc(); };

    FRLexerStyler* getLexerStylerByName(wxString lexerName);
    int getLexerStylerIndexByName(wxString lexerName);

    void addLexerStyler(wxString lexerName, wxString lexerDesc, wxString lexerUserExt, wxXmlNode* lexerNode);

    void eraseAll();
};

class FRStyleManager {
private:
    FRStyles globalStylesM;
    FRLexerStylers lexerStylesM;
    FRStyle* globalStyleM;
    FRStyle* defaultStyleM;
protected:
    void loadLexerStyles(wxXmlNode* node);
    void loadGlobalStyles(wxXmlNode* node);

    void assignWordStyle(wxStyledTextCtrl* text, FRStyle* style);
public:
    FRStyleManager(wxFileName style);
    
    FRStyles getGlobalStyles() { return globalStylesM; };

    FRStyle* getGlobalStyle() { return globalStyleM; };
    FRStyle* getDefaultStyle() { return defaultStyleM; };
    FRStyle* getStyleByName(wxString styleName);

    void assignGlobal(wxStyledTextCtrl* text);

    void assignLexer(wxStyledTextCtrl* text);

};

FRStyleManager& stylerManager();



#endif
