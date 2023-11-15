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

typedef unsigned short      _WORD;
typedef unsigned long       _DWORD;
typedef unsigned char       _BYTE;
typedef _DWORD   _COLORREF;
typedef unsigned long _ULONG_PTR, *_PULONG_PTR;
typedef _ULONG_PTR _DWORD_PTR;

#define _RGB(r,g,b)          ((_COLORREF)(((_BYTE)(r)|((_WORD)((_BYTE)(g))<<8))|(((_DWORD)(_BYTE)(b))<<16)))
#define _HIWORD(l)           ((_WORD)((((_DWORD_PTR)(l)) >> 16) & 0xffff))
#define _HIBYTE(w)           ((_BYTE)((((_DWORD_PTR)(w)) >> 8) & 0xff))


const int FONTSTYLE_NONE = 0;
const int FONTSTYLE_BOLD = 1;
const int FONTSTYLE_ITALIC = 2;
const int FONTSTYLE_UNDERLINE = 4;

const int STYLE_NOT_USED = -1;

const int COLORSTYLE_FOREGROUND = 0x01;
const int COLORSTYLE_BACKGROUND = 0x02;
const int COLORSTYLE_ALL = COLORSTYLE_FOREGROUND | COLORSTYLE_BACKGROUND;

const int MAX_LEXER_STYLE = 100;

const int  FR_LINENUMBERNARGIN = 0;
const int  FR_SYMBOLEMARGIN = 1;
#define FR_FOLDMARGIN 2

// <WordsStyle> or <WidgetStyle>
class FRStyle
{
protected:
    int styleIDM;
    wxString styleDescM;

    wxColour fgColorM;
    wxColour bgColorM;
    int colorStyleM;

    bool isFontEnabledM;
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
    bool operator == (const FRStyle& style);

    void write2Element(wxXmlNode* element);


    int getStyleID() { return styleIDM; };
    void setStyleID(int id) { styleIDM = id; };
    
    wxString getStyleDesc() { return styleDescM; };
    void setStyleDesc(wxString name) { styleDescM = name; };


    wxColour getfgColor() { return fgColorM; };
    void setfgColor(wxColour color) { fgColorM = color; };
    
    wxColour getbgColor() { return bgColorM; };
    void setbgColor(wxColour color) { bgColorM = color; };
    
    int getColorStyle() { return colorStyleM; };
    void setColorStyle(int color) { colorStyleM = color; };


    bool getisFontEnable() { return isFontEnabledM; };
    void setisFontEnable(bool value) { isFontEnabledM = value; };

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


//List of <WordsStyle> or <WidgetStyle> for GlobalStyles
class FRStyles
{
    
protected:
    std::vector<FRStyle *> styleVectorM;
    
public:
    FRStyles();

    FRStyles& operator=(const FRStyles& sa);
    bool operator==(const FRStyles& sa);

    void write2Element(wxXmlNode* element);

    int getNbStyler() const { return styleVectorM.size(); };
    //void setNbStyler(int nb) { nbStylerM = nb; };

    FRStyle* getStyle(size_t index);
    FRStyle* getStyleByName(wxString styleName);
    std::vector<FRStyle*> getStyles() { return styleVectorM; }

    bool hasEnoughSpace() { return (getNbStyler() < wxSTC_STYLE_MAX); };
    void addStyler(int styleID, wxXmlNode* styleNode);
    void addStyler(int styleID, const wxString styleName);

    int getStyleIndexByID(int id);

    int getStyleIndexByName(wxString styleName);
    void clear() { styleVectorM.clear(); };

};

// <LexerType>
class FRStyler : public FRStyles
{
private:
    wxString stylerNameM;
    wxString stylerDescM;
    wxString stylerUserExtM;

public:
    FRStyler();
    FRStyler& operator=(const FRStyler& ls);

    wxString getStylerName() const { return stylerNameM; };
    void setStylerName(wxString lexerName) { stylerNameM = lexerName; };

    void setStylerDesc(wxString lexerDesc) { stylerDescM = lexerDesc; };
    wxString getStylerDesc() const { return stylerDescM; };

    void setStylerUserExt(wxString lexerUserExt) { stylerUserExtM = lexerUserExt; };
    wxString getStylerUserExt() const { return stylerUserExtM; };

};

// <LexerStyles>
class FRStylers
{
private:
    std::vector<FRStyler*> stylerVectorM;
protected:

public:
    FRStylers();

    FRStylers& operator=(const FRStylers& lsa);
    void write2Element(wxXmlNode* element);

    int getNbLexerStyler() const { return stylerVectorM.size(); };
    //void setNbLexerStyler(int nbLexer) { nbLexerStylerM = nbLexer; };
    bool hasEnoughSpace() { return (getNbLexerStyler() < MAX_LEXER_STYLE); };



    FRStyler* getLexerFromIndex(int index) { return stylerVectorM[index]; };
    std::vector<FRStyler*> getStylers() { return stylerVectorM; };

    wxString getStylerNameFromIndex(int index) const { return stylerVectorM[index]->getStylerName(); };
    wxString getStylerDescFromIndex(int index) const { return stylerVectorM[index]->getStylerDesc(); };

    FRStyler* getStylerByName(wxString lexerName);
    int getStylerIndexByName(wxString lexerName);

    FRStyler* getStylerByDesc(wxString lexerDesc);
    int getStylerIndexByDesc(wxString lexerDesc);


    void addStyler(wxString lexerName, wxString lexerDesc, wxString lexerUserExt, wxXmlNode* lexerNode);

    void clear();
};



#endif
